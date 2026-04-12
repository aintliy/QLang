#include "CodeGen.h"
#include "Diagnostics.h"
#include <llvm/IR/Verifier.h>
#include <llvm/IR/DerivedTypes.h>
#include <iostream>

CodeGen::CodeGen() {
    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>("qlang", *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
}

CodeGen::~CodeGen() = default;

std::unique_ptr<llvm::Module> CodeGen::codegen(ProgramNode* program) {
    // 生成 string 类型定义
    // { i8*, i32 } - { data pointer, length }
    llvm::StructType* stringTy = llvm::StructType::create(*context, "string");
    std::vector<llvm::Type*> stringFields = {
        llvm::PointerType::get(builder->getInt8Ty(), 0),
        builder->getInt32Ty()
    };
    stringTy->setBody(stringFields);

    // 声明运行时函数
    declareRuntimeFunctions();

    // 第一遍：收集所有结构体定义和函数调用关系
    for (auto& decl : program->declarations) {
        if (auto* structNode = dynamic_cast<StructDefNode*>(decl.get())) {
            structDefNodes[structNode->name] = structNode;
        } else if (auto* funcNode = dynamic_cast<FuncDefNode*>(decl.get())) {
            // 收集函数调用
            collectFunctionCalls(funcNode);
        }
    }

    // 为每个声明生成 IR
    for (auto& decl : program->declarations) {
        codegen(decl.get());
    }

    // 验证模块
    std::string errorMsg;
    llvm::raw_string_ostream errorStream(errorMsg);
    if (llvm::verifyModule(*module, &errorStream)) {
        errorStream.flush();
        error("IR module verification failed: " + errorMsg);
    }

    return std::move(module);
}

void CodeGen::declareRuntimeFunctions() {
    // 输出函数声明
    // void println_int(i32)
    // void println_float64(double)
    // void print_bool(i1)
    // void println_string(%string*)
    // ... 其他

    llvm::FunctionType* void_i32 = llvm::FunctionType::get(
        builder->getVoidTy(), llvm::ArrayRef<llvm::Type*>(builder->getInt32Ty()), false);
    module->getOrInsertFunction("println_int", void_i32);

    llvm::FunctionType* void_float64 = llvm::FunctionType::get(
        builder->getVoidTy(), llvm::ArrayRef<llvm::Type*>(builder->getDoubleTy()), false);
    module->getOrInsertFunction("println_float64", void_float64);

    llvm::FunctionType* void_bool = llvm::FunctionType::get(
        builder->getVoidTy(), llvm::ArrayRef<llvm::Type*>(builder->getInt1Ty()), false);
    module->getOrInsertFunction("println_bool", void_bool);

    // %string* 参数
    llvm::PointerType* stringPtrTy = llvm::PointerType::get(
        llvm::StructType::getTypeByName(*context, "string"), 0);
    llvm::FunctionType* void_string = llvm::FunctionType::get(
        builder->getVoidTy(), llvm::ArrayRef<llvm::Type*>(stringPtrTy), false);
    module->getOrInsertFunction("println_string", void_string);

    // memcmp 声明
    // i32 @memcmp(i8*, i8*, i64)
    llvm::PointerType* int8PtrTy = llvm::PointerType::get(builder->getInt8Ty(), 0);
    llvm::Type* memcmpParams[3] = {int8PtrTy, int8PtrTy, builder->getInt64Ty()};
    llvm::FunctionType* memcmpType = llvm::FunctionType::get(
        builder->getInt32Ty(), llvm::ArrayRef<llvm::Type*>(memcmpParams, 3), false);
    module->getOrInsertFunction("memcmp", memcmpType);

    // abort 声明
    llvm::FunctionType* abortType = llvm::FunctionType::get(
        builder->getVoidTy(), false);
    module->getOrInsertFunction("abort", abortType);

    // Runtime stack depth tracking for overflow detection
    // @stack_depth: current stack depth counter
    // @stack_limit: maximum allowed stack depth (1024)
    llvm::Type* stackDepthType = builder->getInt32Ty();
    llvm::GlobalVariable* stackDepth = new llvm::GlobalVariable(
        *module, stackDepthType, false, llvm::GlobalValue::ExternalLinkage,
        builder->getInt32(0), "stack_depth");
    stackDepth->setDSOLocal(true);

    // Stack depth limit = 1024 (matches compile-time nesting depth limit)
    llvm::GlobalVariable* stackLimit = new llvm::GlobalVariable(
        *module, builder->getInt32Ty(), false, llvm::GlobalValue::ExternalLinkage,
        builder->getInt32(1024), "stack_limit");
    stackLimit->setDSOLocal(true);
}

llvm::Type* CodeGen::getInt32Type() { return builder->getInt32Ty(); }
llvm::Type* CodeGen::getFloat64Type() { return builder->getDoubleTy(); }
llvm::Type* CodeGen::getBoolType() { return builder->getInt1Ty(); }

llvm::Type* CodeGen::getStringType() {
    return llvm::StructType::getTypeByName(*context, "string");
}

llvm::AllocaInst* CodeGen::createEntryBlockAlloca(llvm::Function* func, const std::string& name, llvm::Type* type) {
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, name);
}

void CodeGen::createBoundsCheck(llvm::Value* idx, int64_t arraySize, const std::string& name) {
    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* okBB = llvm::BasicBlock::Create(*context, "bounds.ok", func);
    llvm::BasicBlock* errorBB = llvm::BasicBlock::Create(*context, "bounds.error", func);

    // 检查 idx < 0
    llvm::Value* cmpLow = builder->CreateICmpSLT(idx, builder->getInt32(0), "cmp.low");
    // 检查 idx >= arraySize
    llvm::Value* cmpHigh = builder->CreateICmpSGE(idx, builder->getInt32(arraySize), "cmp.high");
    // out of bounds = low || high
    llvm::Value* cmpOut = builder->CreateOr(cmpLow, cmpHigh, "cmp.out");

    builder->CreateCondBr(cmpOut, errorBB, okBB);

    // errorBB: 调用 abort
    builder->SetInsertPoint(errorBB);
    llvm::Function* abortFunc = module->getFunction("abort");
    builder->CreateCall(abortFunc);
    builder->CreateUnreachable();

    // okBB: 继续执行
    builder->SetInsertPoint(okBB);
}

void CodeGen::createDivZeroCheck(llvm::Value* divisor) {
    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* okBB = llvm::BasicBlock::Create(*context, "divzero.ok", func);
    llvm::BasicBlock* errorBB = llvm::BasicBlock::Create(*context, "divzero.error", func);

    // 检查 divisor == 0
    llvm::Value* cmpZero = builder->CreateICmpEQ(divisor, builder->getInt32(0), "cmp.zero");

    builder->CreateCondBr(cmpZero, errorBB, okBB);

    // errorBB: 调用 abort
    builder->SetInsertPoint(errorBB);
    llvm::Function* abortFunc = module->getFunction("abort");
    builder->CreateCall(abortFunc);
    builder->CreateUnreachable();

    // okBB: 继续执行
    builder->SetInsertPoint(okBB);
}

int CodeGen::getStructFieldIndex(llvm::StructType* structTy, const std::string& fieldName) {
    // 查找 StructDefNode
    auto it = llvmStructToDef.find(structTy);
    if (it == llvmStructToDef.end()) {
        return -1;
    }

    StructDefNode* structNode = it->second;
    // 遍历字段，找到字段名的索引
    for (size_t i = 0; i < structNode->fields.size(); ++i) {
        if (structNode->fields[i].second == fieldName) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

llvm::Value* CodeGen::codegen(ASTNode* node) {
    if (auto* n = dynamic_cast<StructDefNode*>(node)) { codegen(n); return nullptr; }
    if (auto* n = dynamic_cast<FuncDefNode*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<VarDeclNode*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<BlockStmt*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<IfStmt*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<WhileStmt*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<ForStmt*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<SwitchStmt*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<ReturnStmt*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<BreakStmt*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<ContinueStmt*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<ExprStmt*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<LiteralExpr*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<IdentExpr*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<BinaryExpr*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<UnaryExpr*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<CallExpr*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<IndexExpr*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<MemberExpr*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<CastExpr*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<AssignExpr*>(node)) return codegen(n);
    if (auto* n = dynamic_cast<InitListExpr*>(node)) return codegen(n);

    error("codegen: unknown AST node type");
    return nullptr;
}

llvm::Type* CodeGen::codegen(StructDefNode* node) {
    // 构建结构体类型
    std::vector<llvm::Type*> fieldTypes;
    for (auto& field : node->fields) {
        llvm::Type* fieldType;
        if (field.first == "int32") {
            fieldType = builder->getInt32Ty();
        } else if (field.first == "float64") {
            fieldType = builder->getDoubleTy();
        } else if (field.first == "bool") {
            fieldType = builder->getInt1Ty();
        } else if (field.first == "string") {
            fieldType = getStringType();
        } else {
            // 嵌套结构体类型
            llvm::StructType* structTy = llvm::StructType::getTypeByName(*context, field.first);
            if (!structTy) {
                error("StructDefNode: unknown field type: " + field.first);
                return nullptr;
            }
            fieldType = structTy;
        }
        fieldTypes.push_back(fieldType);
    }

    // 创建结构体类型
    llvm::StructType* structTy = llvm::StructType::create(*context, "struct " + node->name);
    structTy->setBody(fieldTypes);

    // 存储 LLVM StructType 到 StructDefNode 的映射
    llvmStructToDef[structTy] = node;

    // 存储 struct 类型按名字映射（用于 VarDeclNode 查找）
    structTypes["struct " + node->name] = structTy;

    return structTy;
}
llvm::Value* CodeGen::codegen(FuncDefNode* node) {
    // 确定返回类型
    llvm::Type* retType;
    std::vector<llvm::Type*> paramTypes;
    size_t i = 0;
    if (node->returnType == "int32") {
        retType = builder->getInt32Ty();
    } else if (node->returnType == "float64") {
        retType = builder->getDoubleTy();
    } else if (node->returnType == "bool") {
        retType = builder->getInt1Ty();
    } else if (node->returnType == "void") {
        // void main() 特殊处理：返回 i32
        if (node->name == "main") {
            retType = builder->getInt32Ty();
        } else {
            retType = builder->getVoidTy();
        }
    } else if (node->returnType == "string") {
        retType = llvm::PointerType::get(getStringType(), 0);
    } else {
        // 结构体类型 - sret 约定
        llvm::StructType* structTy = llvm::StructType::getTypeByName(*context, "struct " + node->returnType);
        if (!structTy) {
            // 尝试不带前缀
            structTy = llvm::StructType::getTypeByName(*context, node->returnType);
        }
        if (!structTy) {
            error("FuncDefNode: unknown return type: " + node->returnType);
            return nullptr;
        }
        retType = builder->getVoidTy();  // sret: 返回 void

        // 构建 sret 参数类型
        llvm::PointerType* sretTy = llvm::PointerType::get(structTy, 0);

        // 构建新的参数列表，开头插入 sret 指针
        std::vector<llvm::Type*> newParamTypes;
        newParamTypes.insert(newParamTypes.begin(), sretTy);

        // 添加原始参数
        for (auto& param : node->params) {
            if (param.first == "int32") {
                newParamTypes.push_back(builder->getInt32Ty());
            } else if (param.first == "float64") {
                newParamTypes.push_back(builder->getDoubleTy());
            } else if (param.first == "bool") {
                newParamTypes.push_back(builder->getInt1Ty());
            } else if (param.first == "string") {
                newParamTypes.push_back(llvm::PointerType::get(getStringType(), 0));
            } else {
                // 结构体类型参数 - 传引用
                llvm::StructType* paramStructTy = llvm::StructType::getTypeByName(*context, "struct " + param.first);
                if (!paramStructTy) {
                    paramStructTy = llvm::StructType::getTypeByName(*context, param.first);
                }
                if (paramStructTy) {
                    newParamTypes.push_back(llvm::PointerType::get(paramStructTy, 0));
                } else {
                    error("FuncDefNode: unknown param type: " + param.first);
                    return nullptr;
                }
            }
        }

        paramTypes = newParamTypes;
        // 设置标志表示这是 sret 函数
        currentFunctionIsSret = true;
        currentSretType = structTy;
        // 存储 sret 类型映射
        sretTypes[node->name] = structTy;
    }

    // 构建函数类型 (非 sret 情况)
    if (!currentFunctionIsSret) {
        for (auto& param : node->params) {
            if (param.first == "int32") {
                paramTypes.push_back(builder->getInt32Ty());
            } else if (param.first == "float64") {
                paramTypes.push_back(builder->getDoubleTy());
            } else if (param.first == "bool") {
                paramTypes.push_back(builder->getInt1Ty());
            } else if (param.first == "string") {
                paramTypes.push_back(llvm::PointerType::get(getStringType(), 0));
            } else {
                // 结构体类型参数 - 传引用
                llvm::StructType* structTy = llvm::StructType::getTypeByName(*context, param.first);
                if (structTy) {
                    paramTypes.push_back(llvm::PointerType::get(structTy, 0));
                } else {
                    error("FuncDefNode: unknown param type: " + param.first);
                    return nullptr;
                }
            }
        }
    }

    llvm::FunctionType* funcType = llvm::FunctionType::get(retType, paramTypes, false);
    llvm::Function* func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, node->name, module.get());

    // 设置参数名称
    i = 0;
    for (auto& arg : func->args()) {
        if (currentFunctionIsSret) {
            // sret 情况：跳过第一个参数（sret指针），其余对应 node->params
            if (i == 0) {
                i++;
                continue;
            }
            // i-1 因为第一个参数是 sret
            if (i - 1 < node->params.size()) {
                arg.setName(node->params[i - 1].second);
            }
        } else {
            if (i < node->params.size()) {
                arg.setName(node->params[i].second);
            }
        }
        i++;
    }

    // 创建入口 block
    currentFunction = func;
    namedValues.clear();
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(entry);

    // 检查是否是递归函数
    bool isRecursive = isFunctionRecursive(node->name);

    // 如果是递归函数，插入栈深度检查
    if (isRecursive) {
        // 获取 @stack_depth 和 @stack_limit
        llvm::GlobalVariable* stackDepth = module->getNamedGlobal("stack_depth");
        llvm::GlobalVariable* stackLimit = module->getNamedGlobal("stack_limit");

        // %depth = load i32, i32* @stack_depth
        llvm::Value* depth = builder->CreateLoad(builder->getInt32Ty(), stackDepth, "depth");

        // %cmp.depth = icmp sge i32 %depth, @stack_limit
        llvm::Value* limit = builder->CreateLoad(builder->getInt32Ty(), stackLimit, "limit");
        llvm::Value* cmpDepth = builder->CreateICmpSGE(depth, limit, "cmp.depth");

        llvm::Function* func = currentFunction;
        llvm::BasicBlock* okBB = llvm::BasicBlock::Create(*context, "stack.ok", func);
        llvm::BasicBlock* overflowBB = llvm::BasicBlock::Create(*context, "stack.overflow", func);

        builder->CreateCondBr(cmpDepth, overflowBB, okBB);

        // stack.overflow: 调用 abort
        builder->SetInsertPoint(overflowBB);
        llvm::Function* abortFunc = module->getFunction("abort");
        builder->CreateCall(abortFunc);
        builder->CreateUnreachable();

        // stack.ok: 继续，增加计数器
        builder->SetInsertPoint(okBB);
        llvm::Value* newDepth = builder->CreateAdd(depth, builder->getInt32(1), "new.depth");
        builder->CreateStore(newDepth, stackDepth);

        // 设置标志，表示这是递归函数
        isRecursiveFunction = true;
    }

    // 为参数创建 alloca
    i = 0;
    for (auto& arg : func->args()) {
        if (i == 0 && currentFunctionIsSret) {
            // sret 参数：跳过，不创建 alloca
            // 保存 sret 指针
            currentSretArg = &arg;
            i++;
            continue;
        }
        std::string argName = arg.getName().str();
        llvm::AllocaInst* alloca = createEntryBlockAlloca(func, argName, arg.getType());
        builder->CreateStore(&arg, alloca);
        namedValues[argName] = alloca;
        i++;
    }

    // 生成函数体
    if (node->body) {
        codegen(node->body.get());
    }

    // void 函数如果没生成 ret，则添加 ret void
    if (node->returnType == "void") {
        if (!builder->GetInsertBlock()->getTerminator()) {
            // void main() 特殊处理：返回 i32 0
            if (node->name == "main") {
                builder->CreateRet(builder->getInt32(0));
            } else {
                builder->CreateRetVoid();
            }
        }
    }

    currentFunction = nullptr;
    currentFunctionIsSret = false;
    currentSretType = nullptr;
    currentSretArg = nullptr;
    return func;
}
llvm::Value* CodeGen::codegen(VarDeclNode* node) {
    if (!currentFunction) {
        error("VarDeclNode: no current function");
        return nullptr;
    }

    // 获取类型
    llvm::Type* varType;

    // 检查是否是数组类型 (e.g., "int32[5]")
    size_t bracketPos = node->type.find('[');
    if (bracketPos != std::string::npos) {
        // 解析数组类型，支持多维数组如 int32[2][3]
        std::string baseTypeStr = node->type.substr(0, bracketPos);
        size_t firstBracketClose = node->type.find(']');
        bool isMultiDim = node->type.find('[', firstBracketClose + 1) != std::string::npos;

        // 获取基础类型
        llvm::Type* baseType = nullptr;
        if (baseTypeStr == "int32") {
            baseType = builder->getInt32Ty();
        } else if (baseTypeStr == "float64") {
            baseType = builder->getDoubleTy();
        } else if (baseTypeStr == "bool") {
            baseType = builder->getInt1Ty();
        } else {
            error("VarDeclNode: unsupported array element type: " + baseTypeStr);
            return nullptr;
        }

        if (isMultiDim) {
            // 多维数组：解析所有维度
            std::string dims = node->type.substr(bracketPos);
            std::vector<unsigned> sizes;
            size_t pos = 1; // skip first '['
            while (pos < dims.size()) {
                size_t comma = dims.find(',', pos);
                size_t rbracket = dims.find(']', pos);
                std::string dim = dims.substr(pos, comma < rbracket ? comma - pos : rbracket - pos);
                sizes.push_back(std::stoi(dim));
                pos = rbracket + 2; // skip ']' and possibly ','
            }

            // 从内到外构建数组类型: int32[2][3] -> [2 x [3 x i32]]
            llvm::Type* elemType = baseType;
            for (int i = sizes.size() - 1; i >= 0; --i) {
                elemType = llvm::ArrayType::get(elemType, sizes[i]);
            }
            varType = elemType;
        } else {
            // 简单数组 int32[5]
            std::string sizeStr = node->type.substr(bracketPos + 1, firstBracketClose - bracketPos - 1);
            int64_t arraySize = std::stoll(sizeStr);
            varType = llvm::ArrayType::get(baseType, arraySize);
        }
    } else if (node->type == "int32") {
        varType = builder->getInt32Ty();
    } else if (node->type == "float64") {
        varType = builder->getDoubleTy();
    } else if (node->type == "bool") {
        varType = builder->getInt1Ty();
    } else if (node->type == "string") {
        varType = llvm::PointerType::get(getStringType(), 0);
    } else {
        // 结构体类型
        llvm::StructType* structTy = llvm::StructType::getTypeByName(*context, node->type);
        if (!structTy) {
            error("VarDeclNode: unknown type: " + node->type);
            return nullptr;
        }
        varType = structTy;
    }

    // 在入口块创建 alloca
    llvm::Function* func = currentFunction;
    llvm::AllocaInst* alloca = createEntryBlockAlloca(func, node->name, varType);

    // 处理初始化器
    if (node->initializer) {
        llvm::Value* initVal = codegen(node->initializer.get());
        if (!initVal) {
            error("VarDeclNode: failed to codegen initializer");
            return nullptr;
        }
        // 类型转换如果需要
        if (initVal->getType() != varType) {
            // 尝试数值转换
            if (varType->isIntegerTy() && initVal->getType()->isIntegerTy()) {
                if (varType->isIntegerTy(32) && initVal->getType()->isIntegerTy(1)) {
                    initVal = builder->CreateZExt(initVal, varType, " initializer.conv");
                }
            }
        }
        builder->CreateStore(initVal, alloca);
    }

    // 添加到符号表
    namedValues[node->name] = alloca;
    varDeclNodes[node->name] = node;
    return alloca;
}
llvm::Value* CodeGen::codegen(BlockStmt* node) {
    // 遍历块中的每个语句并生成代码
    for (auto& item : node->items) {
        codegen(item.get());
        // 检查是否生成了 return/break/continue 等控制流语句
        // 如果当前 block 已有 terminator，则停止生成
        if (builder->GetInsertBlock()->getTerminator()) {
            break;
        }
    }
    return nullptr;
}
llvm::Value* CodeGen::codegen(IfStmt* node) {
    llvm::Value* cond = codegen(node->condition.get());
    if (!cond) {
        error("IfStmt: failed to codegen condition");
        return nullptr;
    }

    // 将条件转换为 i1
    if (cond->getType()->isIntegerTy(32)) {
        cond = builder->CreateICmpNE(cond, builder->getInt32(0), "cond.tobool");
    } else if (cond->getType()->isDoubleTy()) {
        cond = builder->CreateFCmpONE(cond, llvm::ConstantFP::get(*context, llvm::APFloat(0.0)), "cond.tobool");
    }

    llvm::Function* func = builder->GetInsertBlock()->getParent();

    // 创建 basic blocks
    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context, "then", func);
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(*context, "else", func);
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*context, "if.end", func);

    // 条件分支
    builder->CreateCondBr(cond, thenBB, elseBB);

    // then 分支
    builder->SetInsertPoint(thenBB);
    if (node->thenBranch) {
        codegen(node->thenBranch.get());
    }
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(endBB);
    }

    // else 分支
    builder->SetInsertPoint(elseBB);
    if (node->elseBranch) {
        codegen(node->elseBranch.get());
    }
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(endBB);
    }

    // 继续执行点
    builder->SetInsertPoint(endBB);

    return nullptr;
}
llvm::Value* CodeGen::codegen(WhileStmt* node) {
    llvm::Function* func = builder->GetInsertBlock()->getParent();

    // 创建 basic blocks
    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(*context, "while.cond", func);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(*context, "while.body", func);
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*context, "while.end", func);

    // 保存当前循环目标
    LoopTarget target = {endBB, condBB};
    loopTargetStack.push_back(target);

    // 跳转到条件检查
    builder->CreateBr(condBB);

    // 条件检查
    builder->SetInsertPoint(condBB);
    llvm::Value* cond = codegen(node->condition.get());
    if (!cond) {
        error("WhileStmt: failed to codegen condition");
        return nullptr;
    }
    // 转换为 i1
    if (cond->getType()->isIntegerTy(32)) {
        cond = builder->CreateICmpNE(cond, builder->getInt32(0), "cond.tobool");
    } else if (cond->getType()->isDoubleTy()) {
        cond = builder->CreateFCmpONE(cond, llvm::ConstantFP::get(*context, llvm::APFloat(0.0)), "cond.tobool");
    }
    builder->CreateCondBr(cond, bodyBB, endBB);

    // 循环体
    builder->SetInsertPoint(bodyBB);
    if (node->body) {
        codegen(node->body.get());
    }
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(condBB);
    }

    // 循环结束
    builder->SetInsertPoint(endBB);

    // 弹出循环目标
    loopTargetStack.pop_back();

    return nullptr;
}
llvm::Value* CodeGen::codegen(ForStmt* node) {
    llvm::Function* func = builder->GetInsertBlock()->getParent();

    // 先执行 init（注意：init 不允许变量声明，只允许表达式语句）
    if (node->init) {
        codegen(node->init.get());
    }

    // 创建 basic blocks
    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(*context, "for.cond", func);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(*context, "for.body", func);
    llvm::BasicBlock* incBB = llvm::BasicBlock::Create(*context, "for.inc", func);
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*context, "for.end", func);

    // 保存当前循环目标
    LoopTarget target = {endBB, incBB};
    loopTargetStack.push_back(target);

    // 跳转到条件检查
    builder->CreateBr(condBB);

    // 条件检查
    builder->SetInsertPoint(condBB);
    llvm::Value* cond = nullptr;
    if (node->cond) {
        cond = codegen(node->cond.get());
        if (cond) {
            // 转换为 i1
            if (cond->getType()->isIntegerTy(32)) {
                cond = builder->CreateICmpNE(cond, builder->getInt32(0), "cond.tobool");
            } else if (cond->getType()->isDoubleTy()) {
                cond = builder->CreateFCmpONE(cond, llvm::ConstantFP::get(*context, llvm::APFloat(0.0)), "cond.tobool");
            }
        }
    } else {
        // 空条件视为 true（for(;;) 无限循环）
        cond = builder->getTrue();
    }
    builder->CreateCondBr(cond, bodyBB, endBB);

    // 循环体
    builder->SetInsertPoint(bodyBB);
    if (node->body) {
        codegen(node->body.get());
    }
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(incBB);
    }

    // increment 部分
    builder->SetInsertPoint(incBB);
    if (node->update) {
        codegen(node->update.get());
    }
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(condBB);
    }

    // 循环结束
    builder->SetInsertPoint(endBB);

    // 弹出循环目标
    loopTargetStack.pop_back();

    return nullptr;
}
llvm::Value* CodeGen::codegen(SwitchStmt* node) {
    llvm::Value* cond = codegen(node->expr.get());
    if (!cond) {
        error("SwitchStmt: failed to codegen expression");
        return nullptr;
    }

    // 确保条件是 i32
    if (!cond->getType()->isIntegerTy(32)) {
        error("SwitchStmt: switch expression must be int32");
        return nullptr;
    }

    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*context, "switch.end", func);

    // 保存当前循环目标（switch 也有 break 目标）
    LoopTarget target = {endBB, endBB}; // continue 在 switch 中跳到 end
    loopTargetStack.push_back(target);

    // 创建 switch 指令
    llvm::SwitchInst* switchInst = builder->CreateSwitch(cond, endBB, node->cases.size());

    // 为每个 case 生成代码
    for (auto& casePair : node->cases) {
        // casePair.first 是整数字面量表达式
        llvm::Value* caseVal = codegen(casePair.first.get());
        if (!caseVal || !caseVal->getType()->isIntegerTy(32)) {
            error("SwitchStmt: case must be integer literal");
            return nullptr;
        }

        llvm::BasicBlock* caseBB = llvm::BasicBlock::Create(*context, "case", func);
        switchInst->addCase(llvm::cast<llvm::ConstantInt>(caseVal), caseBB);

        builder->SetInsertPoint(caseBB);
        for (auto& stmt : casePair.second) {
            codegen(stmt.get());
            if (builder->GetInsertBlock()->getTerminator()) {
                break;
            }
        }
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(endBB);
        }
    }

    // 处理 default 分支
    if (!node->defaultBody.empty()) {
        llvm::BasicBlock* defaultBB = llvm::BasicBlock::Create(*context, "default", func);
        switchInst->setDefaultDest(defaultBB);

        builder->SetInsertPoint(defaultBB);
        for (auto& stmt : node->defaultBody) {
            codegen(stmt.get());
            if (builder->GetInsertBlock()->getTerminator()) {
                break;
            }
        }
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(endBB);
        }
    }

    // switch 结束
    builder->SetInsertPoint(endBB);

    // 弹出循环目标
    loopTargetStack.pop_back();

    return nullptr;
}
llvm::Value* CodeGen::codegen(ReturnStmt* node) {
    if (node->value) {
        llvm::Value* retVal = codegen(node->value.get());
        if (!retVal) {
            error("ReturnStmt: failed to codegen return value");
            return nullptr;
        }

        // 检查是否是 sret 函数
        if (currentFunctionIsSret) {
            // sret: 使用 memcpy 将 retVal 复制到 sret 指针
            llvm::Value* sretPtr = currentSretArg;
            if (sretPtr) {
                // 对于 sret，我们需要的是返回值的地址，而不是值本身
                // retVal 可能是加载后的值，我们需要获取它的地址
                // 如果返回值是一个变量（IdentExpr），我们可以从 namedValues 获取其地址
                llvm::Value* srcPtr = nullptr;

                // 检查是否是 IdentExpr
                if (auto* ident = dynamic_cast<IdentExpr*>(node->value.get())) {
                    auto it = namedValues.find(ident->name);
                    if (it != namedValues.end()) {
                        srcPtr = it->second;  // 获取 alloca 的地址
                    }
                }

                // 如果找不到地址（如表达式结果），则使用 retVal
                if (!srcPtr) {
                    srcPtr = retVal;
                }

                // 计算结构体大小
                llvm::DataLayout dataLayout(module.get());
                uint64_t structSize = dataLayout.getTypeAllocSize(currentSretType);

                // 创建 memcpy - 使用 LLVM intrinsic
                llvm::PointerType* i8PtrTy = llvm::PointerType::get(builder->getInt8Ty(), 0);
                llvm::Value* destI8 = builder->CreateBitCast(sretPtr, i8PtrTy, "dest.i8");
                llvm::Value* srcI8 = builder->CreateBitCast(srcPtr, i8PtrTy, "src.i8");

                llvm::Function* memcpyFunc = llvm::Intrinsic::getDeclaration(
                    module.get(), llvm::Intrinsic::memcpy,
                    {i8PtrTy, i8PtrTy, builder->getInt64Ty()});

                builder->CreateCall(memcpyFunc, {
                    destI8,
                    srcI8,
                    builder->getInt64(structSize),
                    builder->getInt1(false)
                });
            }
            // 如果是递归函数，返回前递减栈深度
            if (isRecursiveFunction) {
                llvm::GlobalVariable* stackDepth = module->getNamedGlobal("stack_depth");
                llvm::Value* depth = builder->CreateLoad(builder->getInt32Ty(), stackDepth, "depth");
                llvm::Value* newDepth = builder->CreateSub(depth, builder->getInt32(1), "dec.depth");
                builder->CreateStore(newDepth, stackDepth);
            }
            builder->CreateRetVoid();
        } else {
            // 如果是递归函数，返回前递减栈深度
            if (isRecursiveFunction) {
                llvm::GlobalVariable* stackDepth = module->getNamedGlobal("stack_depth");
                llvm::Value* depth = builder->CreateLoad(builder->getInt32Ty(), stackDepth, "depth");
                llvm::Value* newDepth = builder->CreateSub(depth, builder->getInt32(1), "dec.depth");
                builder->CreateStore(newDepth, stackDepth);
            }
            builder->CreateRet(retVal);
        }
    } else {
        // 如果是递归函数，返回前递减栈深度
        if (isRecursiveFunction) {
            llvm::GlobalVariable* stackDepth = module->getNamedGlobal("stack_depth");
            llvm::Value* depth = builder->CreateLoad(builder->getInt32Ty(), stackDepth, "depth");
            llvm::Value* newDepth = builder->CreateSub(depth, builder->getInt32(1), "dec.depth");
            builder->CreateStore(newDepth, stackDepth);
        }
        builder->CreateRetVoid();
    }
    return nullptr;
}
llvm::Value* CodeGen::codegen(BreakStmt* node) {
    if (loopTargetStack.empty()) {
        error("BreakStmt: break must be inside loop or switch");
        return nullptr;
    }
    LoopTarget& target = loopTargetStack.back();
    builder->CreateBr(target.endBB);
    return nullptr;
}
llvm::Value* CodeGen::codegen(ContinueStmt* node) {
    if (loopTargetStack.empty()) {
        error("ContinueStmt: continue must be inside loop");
        return nullptr;
    }
    LoopTarget& target = loopTargetStack.back();
    builder->CreateBr(target.incBB);
    return nullptr;
}
llvm::Value* CodeGen::codegen(ExprStmt* node) {
    return codegen(node->expr.get());
}
llvm::Value* CodeGen::codegen(LiteralExpr* node) {
    switch (node->literal.kind) {
    case TokenKind::INT_LIT: {
        int32_t value = std::stoi(node->literal.lexeme);
        return builder->getInt32(value);
    }
    case TokenKind::FLOAT_LIT: {
        double value = std::stod(node->literal.lexeme);
        return llvm::ConstantFP::get(*context, llvm::APFloat(value));
    }
    case TokenKind::STRING_LIT: {
        std::string str = node->literal.lexeme;
        str = str.substr(1, str.size() - 2);
        std::vector<llvm::Constant*> chars;
        for (char c : str) {
            chars.push_back(builder->getInt8(c));
        }
        chars.push_back(builder->getInt8(0));
        llvm::ArrayType* arrayTy = llvm::ArrayType::get(builder->getInt8Ty(), chars.size());
        llvm::Constant* init = llvm::ConstantArray::get(arrayTy, chars);
        llvm::GlobalVariable* gv = new llvm::GlobalVariable(
            *module, arrayTy, true, llvm::GlobalValue::PrivateLinkage, init, ".str");
        llvm::StructType* stringTy = static_cast<llvm::StructType*>(getStringType());
        llvm::PointerType* int8PtrTy = llvm::PointerType::get(builder->getInt8Ty(), 0);
        llvm::Value* strPtr = builder->CreateBitCast(gv, int8PtrTy);
        llvm::Value* len = builder->getInt32(str.size());
        llvm::Value* result = builder->CreateAlloca(stringTy);
        llvm::Value* dataPtr = builder->CreateStructGEP(stringTy, result, 0);
        builder->CreateStore(strPtr, dataPtr);
        llvm::Value* lenPtr = builder->CreateStructGEP(stringTy, result, 1);
        builder->CreateStore(len, lenPtr);
        return result;
    }
    case TokenKind::KEYWORD_TRUE:
        return builder->getTrue();
    case TokenKind::KEYWORD_FALSE:
        return builder->getFalse();
    default:
        error("LiteralExpr: unknown literal kind");
        return nullptr;
    }
}
llvm::Value* CodeGen::codegen(IdentExpr* node) {
    auto it = namedValues.find(node->name);
    if (it == namedValues.end()) {
        error("IdentExpr: undefined variable: " + node->name);
        return nullptr;
    }
    // 如果是赋值左侧上下文，返回变量的地址（alloca）
    if (leftSide) {
        return it->second;
    }
    return builder->CreateLoad(it->second->getAllocatedType(), it->second, node->name);
}
llvm::Value* CodeGen::codegen(BinaryExpr* node) {
    llvm::Value* left = codegen(node->left.get());
    llvm::Value* right = codegen(node->right.get());
    if (!left || !right) {
        error("BinaryExpr: failed to codegen operands");
        return nullptr;
    }
    if (node->op == "+") {
        if (left->getType()->isIntegerTy(32)) return builder->CreateAdd(left, right, "add.tmp");
        else if (left->getType()->isDoubleTy()) return builder->CreateFAdd(left, right, "add.tmp");
    } else if (node->op == "-") {
        if (left->getType()->isIntegerTy(32)) return builder->CreateSub(left, right, "sub.tmp");
        else if (left->getType()->isDoubleTy()) return builder->CreateFSub(left, right, "sub.tmp");
    } else if (node->op == "*") {
        if (left->getType()->isIntegerTy(32)) return builder->CreateMul(left, right, "mul.tmp");
        else if (left->getType()->isDoubleTy()) return builder->CreateFMul(left, right, "mul.tmp");
    } else if (node->op == "/") {
        if (left->getType()->isIntegerTy(32)) {
            // 整数除法需要除零检查
            createDivZeroCheck(right);
            return builder->CreateSDiv(left, right, "div.tmp");
        }
        else if (left->getType()->isDoubleTy()) return builder->CreateFDiv(left, right, "div.tmp");
    } else if (node->op == "%") {
        if (left->getType()->isIntegerTy(32)) {
            // 取模需要除零检查
            createDivZeroCheck(right);
            return builder->CreateSRem(left, right, "rem.tmp");
        }
    }
    // 关系运算
    else if (node->op == "<") {
        if (left->getType()->isIntegerTy(32)) return builder->CreateICmpSLT(left, right, "cmp.tmp");
        else if (left->getType()->isDoubleTy()) return builder->CreateFCmpOLT(left, right, "cmp.tmp");
    } else if (node->op == "<=") {
        if (left->getType()->isIntegerTy(32)) return builder->CreateICmpSLE(left, right, "cmp.tmp");
        else if (left->getType()->isDoubleTy()) return builder->CreateFCmpOLE(left, right, "cmp.tmp");
    } else if (node->op == ">") {
        if (left->getType()->isIntegerTy(32)) return builder->CreateICmpSGT(left, right, "cmp.tmp");
        else if (left->getType()->isDoubleTy()) return builder->CreateFCmpOGT(left, right, "cmp.tmp");
    } else if (node->op == ">=") {
        if (left->getType()->isIntegerTy(32)) return builder->CreateICmpSGE(left, right, "cmp.tmp");
        else if (left->getType()->isDoubleTy()) return builder->CreateFCmpOGE(left, right, "cmp.tmp");
    } else if (node->op == "==") {
        if (left->getType()->isIntegerTy(1)) return builder->CreateICmpEQ(left, right, "cmp.tmp");
        else if (left->getType()->isIntegerTy(32)) return builder->CreateICmpEQ(left, right, "cmp.tmp");
        else if (left->getType()->isDoubleTy()) return builder->CreateFCmpOEQ(left, right, "cmp.tmp");
        // 字符串比较 - 变量是指针，literal是struct值
        else if (left->getType()->isPointerTy() || left->getType()->isStructTy()) {
            llvm::StructType* structTy = nullptr;
            llvm::Value* leftPtr = left;
            llvm::Value* rightPtr = right;

            if (left->getType()->isPointerTy()) {
                // For pointer types, check if it's a pointer to string struct
                structTy = llvm::StructType::getTypeByName(*context, "string");
            } else if (left->getType()->isStructTy()) {
                structTy = static_cast<llvm::StructType*>(left->getType());
            }

            if (structTy && structTy->getName() == "string") {
                llvm::PointerType* int8PtrTy = llvm::PointerType::get(builder->getInt8Ty(), 0);
                // left 和 right 是指向 string struct 的指针，直接使用 CreateStructGEP
                // 获取 data 和 len
                llvm::Value* leftDataPtr = builder->CreateStructGEP(structTy, leftPtr, 0, "left.data.ptr");
                llvm::Value* leftData = builder->CreateLoad(int8PtrTy, leftDataPtr, "left.data");
                llvm::Value* leftLenPtr = builder->CreateStructGEP(structTy, leftPtr, 1, "left.len.ptr");
                llvm::Value* leftLen = builder->CreateLoad(builder->getInt32Ty(), leftLenPtr, "left.len");

                llvm::Value* rightDataPtr = builder->CreateStructGEP(structTy, rightPtr, 0, "right.data.ptr");
                llvm::Value* rightData = builder->CreateLoad(int8PtrTy, rightDataPtr, "right.data");
                llvm::Value* rightLenPtr = builder->CreateStructGEP(structTy, rightPtr, 1, "right.len.ptr");
                llvm::Value* rightLen = builder->CreateLoad(builder->getInt32Ty(), rightLenPtr, "right.len");

                // 比较长度
                llvm::Value* lenEq = builder->CreateICmpEQ(leftLen, rightLen, "len.eq");

                // 如果长度相同，比较数据
                llvm::BasicBlock* currentBB = builder->GetInsertBlock();
                llvm::Function* func = currentBB->getParent();
                llvm::BasicBlock* cmpDataBB = llvm::BasicBlock::Create(*context, "cmp.data", func);
                llvm::BasicBlock* cmpEndBB = llvm::BasicBlock::Create(*context, "cmp.end", func);

                builder->CreateCondBr(lenEq, cmpDataBB, cmpEndBB);

                builder->SetInsertPoint(cmpDataBB);
                llvm::Function* memcmpFunc = module->getFunction("memcmp");
                llvm::Value* leftLen64 = builder->CreateZExt(leftLen, builder->getInt64Ty(), "len.zext");
                llvm::CallInst* memcmpResult = builder->CreateCall(memcmpFunc, {leftData, rightData, leftLen64}, "memcmp.result");
                llvm::Value* dataEq = builder->CreateICmpEQ(memcmpResult, builder->getInt32(0), "data.eq");
                builder->CreateBr(cmpEndBB);

                builder->SetInsertPoint(cmpEndBB);
                llvm::PHINode* result = builder->CreatePHI(builder->getInt1Ty(), 2, "str.eq");
                result->addIncoming(builder->getFalse(), currentBB);
                result->addIncoming(dataEq, cmpDataBB);

                return result;
            }
        }
    } else if (node->op == "!=") {
        if (left->getType()->isIntegerTy(1)) return builder->CreateICmpNE(left, right, "cmp.tmp");
        else if (left->getType()->isIntegerTy(32)) return builder->CreateICmpNE(left, right, "cmp.tmp");
        else if (left->getType()->isDoubleTy()) return builder->CreateFCmpONE(left, right, "cmp.tmp");
        // 字符串比较
        else if (left->getType()->isPointerTy() || left->getType()->isStructTy()) {
            llvm::StructType* structTy = nullptr;
            llvm::Value* leftPtr = left;
            llvm::Value* rightPtr = right;

            if (left->getType()->isPointerTy()) {
                // For pointer types, check if it's a pointer to string struct
                structTy = llvm::StructType::getTypeByName(*context, "string");
            } else if (left->getType()->isStructTy()) {
                structTy = static_cast<llvm::StructType*>(left->getType());
            }

            if (structTy && structTy->getName() == "string") {
                llvm::PointerType* int8PtrTy = llvm::PointerType::get(builder->getInt8Ty(), 0);
                // left 和 right 是指向 string struct 的指针，直接使用 CreateStructGEP
                // 获取 data 和 len
                llvm::Value* leftDataPtr = builder->CreateStructGEP(structTy, leftPtr, 0, "left.data.ptr");
                llvm::Value* leftData = builder->CreateLoad(int8PtrTy, leftDataPtr, "left.data");
                llvm::Value* leftLenPtr = builder->CreateStructGEP(structTy, leftPtr, 1, "left.len.ptr");
                llvm::Value* leftLen = builder->CreateLoad(builder->getInt32Ty(), leftLenPtr, "left.len");

                llvm::Value* rightDataPtr = builder->CreateStructGEP(structTy, rightPtr, 0, "right.data.ptr");
                llvm::Value* rightData = builder->CreateLoad(int8PtrTy, rightDataPtr, "right.data");
                llvm::Value* rightLenPtr = builder->CreateStructGEP(structTy, rightPtr, 1, "right.len.ptr");
                llvm::Value* rightLen = builder->CreateLoad(builder->getInt32Ty(), rightLenPtr, "right.len");

                // 比较长度
                llvm::Value* lenEq = builder->CreateICmpEQ(leftLen, rightLen, "len.eq");

                // 如果长度相同，比较数据
                llvm::BasicBlock* currentBB = builder->GetInsertBlock();
                llvm::Function* func = currentBB->getParent();
                llvm::BasicBlock* cmpDataBB = llvm::BasicBlock::Create(*context, "cmp.data", func);
                llvm::BasicBlock* cmpEndBB = llvm::BasicBlock::Create(*context, "cmp.end", func);

                builder->CreateCondBr(lenEq, cmpDataBB, cmpEndBB);

                // In cmp.data, compute the "not equal" result
                builder->SetInsertPoint(cmpDataBB);
                llvm::Function* memcmpFunc = module->getFunction("memcmp");
                llvm::Value* leftLen64 = builder->CreateZExt(leftLen, builder->getInt64Ty(), "len.zext");
                llvm::CallInst* memcmpResult = builder->CreateCall(memcmpFunc, {leftData, rightData, leftLen64}, "memcmp.result");
                llvm::Value* dataEq = builder->CreateICmpEQ(memcmpResult, builder->getInt32(0), "data.eq");
                // Compute "not equal" as XOR with true
                llvm::Value* dataNe = builder->CreateXor(dataEq, builder->getTrue(), "data.ne");
                builder->CreateBr(cmpEndBB);

                // In cmp.end, use PHI to select: true from entry (lengths differ) or data.ne from cmp.data
                builder->SetInsertPoint(cmpEndBB);
                llvm::PHINode* result = builder->CreatePHI(builder->getInt1Ty(), 2, "str.ne");
                result->addIncoming(builder->getTrue(), currentBB);
                result->addIncoming(dataNe, cmpDataBB);

                return result;
            }
        }
    } else if (node->op == "&&") {
        llvm::BasicBlock* entryBB = builder->GetInsertBlock();
        llvm::Function* func = entryBB->getParent();
        llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(*context, "and.rhs", func);
        llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*context, "and.end", func);
        llvm::Value* leftVal = left;
        if (left->getType()->isIntegerTy(32)) leftVal = builder->CreateICmpNE(left, builder->getInt32(0), "a.tobool");
        else if (left->getType()->isDoubleTy()) leftVal = builder->CreateFCmpONE(left, llvm::ConstantFP::get(*context, llvm::APFloat(0.0)), "a.tobool");
        builder->CreateCondBr(leftVal, rhsBB, endBB);
        builder->SetInsertPoint(rhsBB);
        llvm::Value* rightVal = codegen(node->right.get());
        llvm::Value* rightBool = rightVal;
        if (rightVal->getType()->isIntegerTy(32)) rightBool = builder->CreateICmpNE(rightVal, builder->getInt32(0), "b.tobool");
        else if (rightVal->getType()->isDoubleTy()) rightBool = builder->CreateFCmpONE(rightVal, llvm::ConstantFP::get(*context, llvm::APFloat(0.0)), "b.tobool");
        builder->CreateBr(endBB);
        builder->SetInsertPoint(endBB);
        llvm::PHINode* phi = builder->CreatePHI(builder->getInt1Ty(), 2, "and.result");
        phi->addIncoming(builder->getFalse(), entryBB);
        phi->addIncoming(rightBool, rhsBB);
        return phi;
    } else if (node->op == "||") {
        llvm::BasicBlock* entryBB = builder->GetInsertBlock();
        llvm::Function* func = entryBB->getParent();
        llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(*context, "or.rhs", func);
        llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*context, "or.end", func);
        llvm::Value* leftVal = left;
        if (left->getType()->isIntegerTy(32)) leftVal = builder->CreateICmpNE(left, builder->getInt32(0), "a.tobool");
        else if (left->getType()->isDoubleTy()) leftVal = builder->CreateFCmpONE(left, llvm::ConstantFP::get(*context, llvm::APFloat(0.0)), "a.tobool");
        builder->CreateCondBr(leftVal, endBB, rhsBB);
        builder->SetInsertPoint(rhsBB);
        llvm::Value* rightVal = codegen(node->right.get());
        llvm::Value* rightBool = rightVal;
        if (rightVal->getType()->isIntegerTy(32)) rightBool = builder->CreateICmpNE(rightVal, builder->getInt32(0), "b.tobool");
        else if (rightVal->getType()->isDoubleTy()) rightBool = builder->CreateFCmpONE(rightVal, llvm::ConstantFP::get(*context, llvm::APFloat(0.0)), "b.tobool");
        builder->CreateBr(endBB);
        builder->SetInsertPoint(endBB);
        llvm::PHINode* phi = builder->CreatePHI(builder->getInt1Ty(), 2, "or.result");
        phi->addIncoming(builder->getTrue(), entryBB);
        phi->addIncoming(rightBool, rhsBB);
        return phi;
    }
    error("BinaryExpr: unknown operator: " + node->op);
    return nullptr;
}
llvm::Value* CodeGen::codegen(UnaryExpr* node) {
    llvm::Value* operand = codegen(node->operand.get());
    if (!operand) {
        error("UnaryExpr: failed to codegen operand");
        return nullptr;
    }
    if (node->op == "-") {
        if (operand->getType()->isIntegerTy(32)) return builder->CreateNeg(operand, "neg.tmp");
        else if (operand->getType()->isDoubleTy()) return builder->CreateFNeg(operand, "neg.tmp");
    } else if (node->op == "!") {
        if (operand->getType()->isIntegerTy(1)) return builder->CreateXor(operand, builder->getTrue(), "not.tmp");
        else if (operand->getType()->isIntegerTy(32)) {
            llvm::Value* boolVal = builder->CreateICmpNE(operand, builder->getInt32(0), "tobool");
            return builder->CreateXor(boolVal, builder->getTrue(), "not.tmp");
        }
    }
    error("UnaryExpr: unknown operator: " + node->op);
    return nullptr;
}
llvm::Value* CodeGen::codegen(CallExpr* node) {
    llvm::Function* callee = module->getFunction(node->callee);
    if (!callee) {
        error("CallExpr: undefined function: " + node->callee);
        return nullptr;
    }

    // 检查是否是 sret 函数（返回 void 但第一个参数是 sret 指针）
    bool isSret = callee->getReturnType()->isVoidTy() &&
                   !callee->arg_empty() &&
                   callee->getArg(0)->getType()->isPointerTy();

    llvm::StructType* sretStructTy = nullptr;
    if (isSret) {
        auto it = sretTypes.find(node->callee);
        if (it != sretTypes.end()) {
            sretStructTy = it->second;
        }
    }

    std::vector<llvm::Value*> args;

    if (isSret) {
        // 为 sret 分配临时空间
        llvm::AllocaInst* tempRet = createEntryBlockAlloca(
            currentFunction, "ret.tmp", sretStructTy);
        args.push_back(tempRet);
    }

    for (auto& argExpr : node->args) {
        llvm::Value* argVal = codegen(argExpr.get());
        if (!argVal) {
            error("CallExpr: failed to codegen argument");
            return nullptr;
        }
        args.push_back(argVal);
    }

    llvm::CallInst* call = builder->CreateCall(callee, args);

    if (isSret) {
        // sret 调用返回 void，加载临时空间的值作为结果
        llvm::Value* result = builder->CreateLoad(sretStructTy, args[0], "ret.val");
        return result;
    } else {
        if (!callee->getReturnType()->isVoidTy()) {
            call->setName("call.tmp");
        }
        return call;
    }
}
llvm::Value* CodeGen::codegen(IndexExpr* node) {
    // 获取数组地址
    llvm::Value* arr = nullptr;
    int64_t arraySize = -1;  // 数组大小，用于越界检查

    // 如果 base 是 IdentExpr，直接获取其地址（alloca），不要加载
    if (auto* ident = dynamic_cast<IdentExpr*>(node->base.get())) {
        auto it = namedValues.find(ident->name);
        if (it == namedValues.end()) {
            error("IndexExpr: undefined variable: " + ident->name);
            return nullptr;
        }
        arr = it->second;  // 获取 alloca (指针)
    } else if (auto* innerIndex = dynamic_cast<IndexExpr*>(node->base.get())) {
        // 多维数组下标如 matrix[0][1]
        // base 是 IndexExpr (matrix[0])，需要特殊处理
        // 对于多维数组，matrix[0] 返回的是指向一行的指针 [3 x i32]*

        // 获取下标
        llvm::Value* idx = codegen(node->index.get());
        if (!idx) {
            error("IndexExpr: failed to codegen index");
            return nullptr;
        }

        // 确保下标是 i32
        if (idx->getType()->isIntegerTy(1)) {
            idx = builder->CreateZExt(idx, builder->getInt32Ty(), "idx.zext");
        }

        // 调用 codegen 处理 innerIndex (matrix[0])
        // 设置 leftSide=false 以获取加载的值（行数组），而不是指针
        bool oldLeftSide = leftSide;
        leftSide = false;
        llvm::Value* innerResult = codegen(innerIndex);
        leftSide = oldLeftSide;

        if (!innerResult) {
            error("IndexExpr: failed to codegen inner index");
            return nullptr;
        }

        llvm::Type* innerType = innerResult->getType();

        // innerResult 应该是加载的行数组 [3 x i32]
        if (innerType->isArrayTy()) {
            llvm::ArrayType* rowArrayTy = llvm::cast<llvm::ArrayType>(innerType);
            llvm::Type* elemTy = rowArrayTy->getElementType();

            // 将 row 存到临时变量（使用 i8* 类型以便字节偏移计算）
            llvm::Type* i8PtrTy = llvm::PointerType::get(builder->getInt8Ty(), 0);
            llvm::AllocaInst* temp = createEntryBlockAlloca(currentFunction, "row.tmp", llvm::ArrayType::get(builder->getInt8Ty(), rowArrayTy->getNumElements() * (elemTy->isIntegerTy(32) ? 4 : (elemTy->isDoubleTy() ? 8 : 4))));
            llvm::Value* rowAsI8 = builder->CreateBitCast(temp, i8PtrTy, "row.i8");

            // 创建行大小的值（用于回填）
            int64_t rowSize = rowArrayTy->getNumElements() * (elemTy->isIntegerTy(32) ? 4 : (elemTy->isDoubleTy() ? 8 : 4));

            // 使用 memset 存储整行（简化版：逐字节存储）
            // 实际上，我们使用 bitcast 后直接用 GEP
            // 先将 innerResult 存储到一个 i8* 临时
            llvm::AllocaInst* i8Temp = createEntryBlockAlloca(currentFunction, "row.i8.tmp", llvm::ArrayType::get(builder->getInt8Ty(), rowSize));
            builder->CreateStore(innerResult, i8Temp);
            llvm::Value* i8TempPtr = builder->CreateBitCast(i8Temp, i8PtrTy, "row.i8.cast");

            // 计算字节偏移：每列 elemSize 字节
            int64_t elemSize = elemTy->isIntegerTy(32) ? 4 : (elemTy->isDoubleTy() ? 8 : 4);
            llvm::Value* elemSizeVal = builder->getInt32(elemSize);
            llvm::Value* byteOffset = builder->CreateMul(idx, elemSizeVal, "elem.offset");

            // 使用 CreatePtrAdd 计算元素地址
            llvm::Value* elemPtr = builder->CreatePtrAdd(i8TempPtr, byteOffset, "elem.ptr");

            if (leftSide) {
                // 返回元素指针用于赋值
                llvm::PointerType* elemPtrTy = llvm::PointerType::get(elemTy, 0);
                return builder->CreateBitCast(elemPtr, elemPtrTy, "elem.ptr.cast");
            } else {
                // 返回元素值
                llvm::PointerType* elemPtrTy = llvm::PointerType::get(elemTy, 0);
                llvm::Value* elemPtrCast = builder->CreateBitCast(elemPtr, elemPtrTy, "elem.ptr.cast");
                return builder->CreateLoad(elemTy, elemPtrCast, "elem.val");
            }
        }

        error("IndexExpr: inner result must be array type for multi-dimensional indexing");
        return nullptr;
    } else {
        arr = codegen(node->base.get());
        if (!arr) {
            error("IndexExpr: failed to codegen base");
            return nullptr;
        }
    }

    // 获取下标
    llvm::Value* idx = codegen(node->index.get());
    if (!idx) {
        error("IndexExpr: failed to codegen index");
        return nullptr;
    }

    // 确保下标是 i32
    if (idx->getType()->isIntegerTy(1)) {
        idx = builder->CreateZExt(idx, builder->getInt32Ty(), "idx.zext");
    }

    // 获取数组类型
    llvm::Type* arrType = arr->getType();

    // 确定数组元素类型
    llvm::Type* elemType = nullptr;

    if (arrType->isPointerTy()) {
        // 指针类型：尝试从 base 的 IdentExpr 获取类型信息
        // 对于多维数组，需要特殊处理
        if (auto* ident = dynamic_cast<IdentExpr*>(node->base.get())) {
            auto varIt = varDeclNodes.find(ident->name);
            if (varIt != varDeclNodes.end()) {
                VarDeclNode* varDecl = varIt->second;
                std::string typeStr = varDecl->type;
                // 检查是否是数组类型
                size_t bracketPos = typeStr.find('[');
                if (bracketPos != std::string::npos) {
                    size_t firstBracketClose = typeStr.find(']');
                    bool isMultiDim = typeStr.find('[', firstBracketClose + 1) != std::string::npos;

                    std::string baseTypeStr = typeStr.substr(0, bracketPos);
                    llvm::Type* baseType = nullptr;
                    if (baseTypeStr == "int32") {
                        baseType = builder->getInt32Ty();
                    } else if (baseTypeStr == "float64") {
                        baseType = builder->getDoubleTy();
                    } else if (baseTypeStr == "bool") {
                        baseType = builder->getInt1Ty();
                    }

                    if (isMultiDim) {
                        // 多维数组 int32[2][3]：解析维度
                        std::string dims = typeStr.substr(bracketPos);
                        std::vector<unsigned> sizes;
                        size_t pos = 1;
                        while (pos < dims.size()) {
                            size_t comma = dims.find(',', pos);
                            size_t rbracket = dims.find(']', pos);
                            std::string dim = dims.substr(pos, comma < rbracket ? comma - pos : rbracket - pos);
                            sizes.push_back(std::stoi(dim));
                            pos = rbracket + 2;
                        }

                        // 计算行大小的字节数
                        llvm::Type* rowType = nullptr;
                        if (sizes.size() >= 2) {
                            llvm::Type* t = baseType;
                            for (size_t i = 1; i < sizes.size(); ++i) {
                                t = llvm::ArrayType::get(t, sizes[i]);
                            }
                            rowType = t;
                        } else {
                            rowType = baseType;
                        }
                        int64_t rowSize = sizes[1] * (baseType->isIntegerTy(32) ? 4 : (baseType->isDoubleTy() ? 8 : 4));
                        llvm::Value* rowSizeVal = builder->getInt32(rowSize);
                        llvm::Value* byteOffset = builder->CreateMul(idx, rowSizeVal, "row.offset");

                        // 使用 i8* 字节偏移计算
                        llvm::Type* i8PtrTy = llvm::PointerType::get(builder->getInt8Ty(), 0);
                        llvm::Value* arrAsI8 = builder->CreateBitCast(arr, i8PtrTy, "arr.i8");
                        llvm::Value* rowPtrI8 = builder->CreateGEP(builder->getInt8Ty(), arrAsI8, byteOffset, "row.ptr.i8");
                        llvm::PointerType* rowPtrTy = llvm::PointerType::get(rowType, 0);
                        llvm::Value* rowPtr = builder->CreateBitCast(rowPtrI8, rowPtrTy, "row.ptr");

                        if (leftSide) {
                            return rowPtr;
                        } else {
                            // 加载整行
                            return builder->CreateLoad(rowType, rowPtr, "row.val");
                        }
                    } else {
                        // 简单数组 int32[5]
                        if (baseTypeStr == "int32") {
                            elemType = builder->getInt32Ty();
                        } else if (baseTypeStr == "float64") {
                            elemType = builder->getDoubleTy();
                        } else if (baseTypeStr == "bool") {
                            elemType = builder->getInt1Ty();
                        }
                        // 提取数组大小用于越界检查
                        arraySize = std::stoi(typeStr.substr(bracketPos + 1, firstBracketClose - bracketPos - 1));
                    }
                }
            }
        }
        if (!elemType) {
            elemType = builder->getInt32Ty();  // 默认 int32
        }

        // 越界检查
        if (arraySize > 0) {
            createBoundsCheck(idx, arraySize, "arr.bounds");
        }

        // 使用字节偏移计算
        int64_t elemSize = 4;
        if (elemType->isIntegerTy()) {
            elemSize = elemType->getIntegerBitWidth() / 8;
        } else if (elemType->isDoubleTy()) {
            elemSize = 8;
        } else if (elemType->isFloatTy()) {
            elemSize = 4;
        }
        llvm::Value* byteOffset = builder->CreateMul(idx, builder->getInt32(elemSize), "idx.bytes");
        llvm::Value* elemPtr = builder->CreatePtrAdd(arr, byteOffset, "elem.ptr");

        if (leftSide) {
            return elemPtr;
        } else {
            return builder->CreateLoad(elemType, elemPtr, "elem.val");
        }
    } else if (arrType->isArrayTy()) {
        // 数组类型：直接使用 CreateGEP
        llvm::ArrayType* arrayTy = static_cast<llvm::ArrayType*>(arrType);
        elemType = arrayTy->getElementType();

        llvm::Value* zero = builder->getInt32(0);
        llvm::Value* elemPtr = builder->CreateGEP(arrType, arr, {zero, idx}, "elem.ptr");

        if (leftSide) {
            return elemPtr;
        } else {
            return builder->CreateLoad(elemType, elemPtr, "elem.val");
        }
    } else {
        error("IndexExpr: base must be pointer or array type");
        return nullptr;
    }
}
llvm::Value* CodeGen::codegen(MemberExpr* node) {
    // 获取对象基地址
    llvm::Value* obj = nullptr;
    llvm::StructType* structTy = nullptr;

    // 如果对象是 IdentExpr，需要特殊处理 leftSide
    if (auto* ident = dynamic_cast<IdentExpr*>(node->object.get())) {
        auto it = namedValues.find(ident->name);
        if (it == namedValues.end()) {
            error("MemberExpr: undefined variable: " + ident->name);
            return nullptr;
        }
        obj = it->second; // 获取 alloca

        // 查找结构体类型
        auto varIt = varDeclNodes.find(ident->name);
        if (varIt != varDeclNodes.end()) {
            VarDeclNode* varDecl = varIt->second;
            // 查找 struct 类型
            auto structIt = structTypes.find(varDecl->type);
            if (structIt != structTypes.end()) {
                structTy = structIt->second;
            }
        }

        if (!structTy) {
            error("MemberExpr: could not find struct type for: " + ident->name);
            return nullptr;
        }
    } else {
        // 其他情况，递归调用 codegen
        obj = codegen(node->object.get());
        if (!obj) {
            error("MemberExpr: failed to codegen object");
            return nullptr;
        }

        // 尝试获取结构体类型
        llvm::Type* objType = obj->getType();
        if (objType->isPointerTy()) {
            objType = objType->getScalarType();
        }
        if (objType->isStructTy()) {
            structTy = static_cast<llvm::StructType*>(objType);
        }
    }

    if (!structTy) {
        error("MemberExpr: object must be struct type");
        return nullptr;
    }

    // 查找字段索引
    int fieldIndex = getStructFieldIndex(structTy, node->member);
    if (fieldIndex < 0) {
        error("MemberExpr: unknown member: " + node->member);
        return nullptr;
    }

    // 获取成员地址
    llvm::Value* memberPtr;
    if (obj->getType()->isPointerTy()) {
        // obj 是指针，直接使用 CreateStructGEP
        memberPtr = builder->CreateStructGEP(structTy, obj, fieldIndex, "member.ptr");
    } else {
        // obj 是值，需要先加载
        llvm::Type* objType = obj->getType();
        llvm::Value* objLoad = builder->CreateLoad(objType, obj, "obj.load");
        memberPtr = builder->CreateStructGEP(structTy, objLoad, fieldIndex, "member.ptr");
    }

    if (leftSide) {
        // 赋值左侧，返回地址
        return memberPtr;
    } else {
        // 赋值右侧，加载值
        llvm::Type* fieldType = structTy->getElementType(fieldIndex);
        return builder->CreateLoad(fieldType, memberPtr, "member.val");
    }
}
llvm::Value* CodeGen::codegen(CastExpr* node) {
    llvm::Value* operand = codegen(node->operand.get());
    if (!operand) {
        error("CastExpr: failed to codegen operand");
        return nullptr;
    }
    if (node->targetType == "int32") {
        if (operand->getType()->isDoubleTy()) {
            return builder->CreateFPToSI(operand, builder->getInt32Ty(), "fptosi.tmp");
        }
    } else if (node->targetType == "float64") {
        if (operand->getType()->isIntegerTy(32)) {
            return builder->CreateSIToFP(operand, builder->getDoubleTy(), "sitofp.tmp");
        }
    }
    error("CastExpr: unsupported conversion to " + node->targetType);
    return nullptr;
}
llvm::Value* CodeGen::codegen(AssignExpr* node) {
    // 赋值目标地址
    llvm::Value* destPtr = nullptr;

    // 对于标识符直接查符号表，获取 alloca
    if (auto* ident = dynamic_cast<IdentExpr*>(node->target.get())) {
        auto it = namedValues.find(ident->name);
        if (it != namedValues.end()) {
            destPtr = it->second;
        }
    }

    // 其他情况（MemberExpr, IndexExpr）需要通过 codegen 获取地址
    if (!destPtr) {
        // 设置 leftSide 标记，处理赋值目标
        bool oldLeftSide = leftSide;
        leftSide = true;
        llvm::Value* target = codegen(node->target.get());
        leftSide = oldLeftSide;

        if (!target) {
            error("AssignExpr: left side evaluation failed");
            return nullptr;
        }
        if (!target->getType()->isPointerTy()) {
            error("AssignExpr: left side must be a pointer");
            return nullptr;
        }
        destPtr = target;
    }

    llvm::Value* value = codegen(node->value.get());
    if (!value) {
        error("AssignExpr: failed to codegen value");
        return nullptr;
    }

    // 检查是否是字符串类型，进行浅拷贝
    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(destPtr)) {
        if (alloca->getAllocatedType()->isStructTy()) {
            llvm::StructType* structTy = static_cast<llvm::StructType*>(alloca->getAllocatedType());
            if (structTy->getName() == "string") {
                // 字符串赋值：浅拷贝 {指针, 长度}
                // 获取源字符串的 data 和 len
                llvm::PointerType* int8PtrTy = llvm::PointerType::get(builder->getInt8Ty(), 0);
                llvm::Value* srcDataPtr = builder->CreateStructGEP(structTy, value, 0, "src.data.ptr");
                llvm::Value* srcData = builder->CreateLoad(int8PtrTy, srcDataPtr, "src.data");
                llvm::Value* srcLenPtr = builder->CreateStructGEP(structTy, value, 1, "src.len.ptr");
                llvm::Value* srcLen = builder->CreateLoad(builder->getInt32Ty(), srcLenPtr, "src.len");

                // 获取目标字符串的 data 和 len 指针
                llvm::Value* dstDataPtr = builder->CreateStructGEP(structTy, destPtr, 0, "dst.data.ptr");
                llvm::Value* dstLenPtr = builder->CreateStructGEP(structTy, destPtr, 1, "dst.len.ptr");

                // 存储
                builder->CreateStore(srcData, dstDataPtr);
                builder->CreateStore(srcLen, dstLenPtr);
                return value;
            }
        }
    }

    // 检查是否是数组类型，进行整体赋值（llvm.memcpy）
    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(destPtr)) {
        if (alloca->getAllocatedType()->isArrayTy()) {
            llvm::ArrayType* arrTy = static_cast<llvm::ArrayType*>(alloca->getAllocatedType());
            llvm::Type* elemTy = arrTy->getElementType();

            // 计算大小
            uint64_t totalSize = arrTy->getNumElements() * (elemTy->isIntegerTy(32) ? 4 :
                                      elemTy->isDoubleTy() ? 8 :
                                      elemTy->isFloatTy() ? 4 : elemTy->getScalarSizeInBits() / 8);

            // 获取源地址（使用 leftSide = true 获取地址而非加载值）
            bool oldLeftSide = leftSide;
            leftSide = true;
            llvm::Value* srcAlloca = codegen(node->value.get());
            leftSide = oldLeftSide;

            if (!srcAlloca) {
                error("AssignExpr: failed to codegen array source");
                return nullptr;
            }

            // 获取元素指针
            llvm::Value* zero = builder->getInt32(0);
            llvm::Value* destPtrArr = builder->CreateGEP(arrTy, alloca, {zero, zero}, "dest.ptr");
            llvm::Value* srcPtr = builder->CreateGEP(arrTy, srcAlloca, {zero, zero}, "src.ptr");

            // 转换为 i8*
            llvm::PointerType* i8PtrTy = llvm::PointerType::get(builder->getInt8Ty(), 0);
            llvm::Value* destI8 = builder->CreateBitCast(destPtrArr, i8PtrTy, "dest.i8");
            llvm::Value* srcI8 = builder->CreateBitCast(srcPtr, i8PtrTy, "src.i8");

            // 调用 memcpy - 使用正确的 LLVM intrinsic
            llvm::Function* memcpyFunc = llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::memcpy,
                {i8PtrTy, i8PtrTy, builder->getInt64Ty()});

            builder->CreateCall(memcpyFunc, {
                destI8,
                srcI8,
                builder->getInt64(totalSize),
                builder->getInt1(false)
            });

            return srcAlloca;
        }
    }

    builder->CreateStore(value, destPtr);
    return value;
}
llvm::Value* CodeGen::codegen(InitListExpr* node) {
    // 聚合初始化使用 insertvalue 指令
    llvm::Type* aggType = nullptr;
    llvm::Value* result = nullptr;

    for (size_t i = 0; i < node->elements.size(); ++i) {
        llvm::Value* elemVal = codegen(node->elements[i].get());
        if (!elemVal) {
            error("InitListExpr: failed to codegen element");
            return nullptr;
        }

        if (i == 0) {
            // 第一个元素，确定类型并创建 undef
            aggType = elemVal->getType();
            result = llvm::UndefValue::get(aggType);
        }

        if (aggType->isStructTy()) {
            result = builder->CreateInsertValue(result, elemVal, i, "init.val");
        } else if (aggType->isArrayTy()) {
            result = builder->CreateInsertValue(result, elemVal, i, "init.val");
        }
    }

    return result;
}

void CodeGen::collectFunctionCalls(FuncDefNode* node) {
    // 遍历函数体，收集所有函数调用
    std::vector<std::string> calls;

    // 使用递归遍历 AST 收集 CallExpr
    std::function<void(ASTNode*)> collect = [&](ASTNode* n) {
        if (auto* call = dynamic_cast<CallExpr*>(n)) {
            calls.push_back(call->callee);
        }
        // 递归遍历所有子节点
        if (auto* block = dynamic_cast<BlockStmt*>(n)) {
            for (auto& item : block->items) collect(item.get());
        } else if (auto* ifStmt = dynamic_cast<IfStmt*>(n)) {
            collect(ifStmt->condition.get());
            if (ifStmt->thenBranch) collect(ifStmt->thenBranch.get());
            if (ifStmt->elseBranch) collect(ifStmt->elseBranch.get());
        } else if (auto* whileStmt = dynamic_cast<WhileStmt*>(n)) {
            collect(whileStmt->condition.get());
            if (whileStmt->body) collect(whileStmt->body.get());
        } else if (auto* forStmt = dynamic_cast<ForStmt*>(n)) {
            if (forStmt->init) collect(forStmt->init.get());
            if (forStmt->cond) collect(forStmt->cond.get());
            if (forStmt->update) collect(forStmt->update.get());
            if (forStmt->body) collect(forStmt->body.get());
        } else if (auto* switchStmt = dynamic_cast<SwitchStmt*>(n)) {
            collect(switchStmt->expr.get());
            for (auto& casePair : switchStmt->cases) {
                collect(casePair.first.get());
                for (auto& stmt : casePair.second) collect(stmt.get());
            }
            for (auto& stmt : switchStmt->defaultBody) collect(stmt.get());
        } else if (auto* returnStmt = dynamic_cast<ReturnStmt*>(n)) {
            if (returnStmt->value) collect(returnStmt->value.get());
        } else if (auto* exprStmt = dynamic_cast<ExprStmt*>(n)) {
            collect(exprStmt->expr.get());
        } else if (auto* binary = dynamic_cast<BinaryExpr*>(n)) {
            collect(binary->left.get());
            collect(binary->right.get());
        } else if (auto* unary = dynamic_cast<UnaryExpr*>(n)) {
            collect(unary->operand.get());
        } else if (auto* assign = dynamic_cast<AssignExpr*>(n)) {
            collect(assign->target.get());
            collect(assign->value.get());
        } else if (auto* index = dynamic_cast<IndexExpr*>(n)) {
            collect(index->base.get());
            collect(index->index.get());
        } else if (auto* member = dynamic_cast<MemberExpr*>(n)) {
            collect(member->object.get());
        } else if (auto* callExpr = dynamic_cast<CallExpr*>(n)) {
            calls.push_back(callExpr->callee);
            for (auto& arg : callExpr->args) collect(arg.get());
        } else if (auto* cast = dynamic_cast<CastExpr*>(n)) {
            collect(cast->operand.get());
        } else if (auto* initList = dynamic_cast<InitListExpr*>(n)) {
            for (auto& elem : initList->elements) collect(elem.get());
        }
    };

    if (node->body) collect(node->body.get());
    functionCalls[node->name] = calls;
}

bool CodeGen::isFunctionRecursive(const std::string& funcName) {
    auto it = functionCalls.find(funcName);
    if (it == functionCalls.end()) return false;
    for (const auto& callee : it->second) {
        if (callee == funcName) return true;
    }
    return false;
}
