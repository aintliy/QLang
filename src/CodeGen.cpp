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
    llvm::StructType::create(*context, "string");

    // 声明运行时函数
    declareRuntimeFunctions();

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

llvm::Value* CodeGen::codegen(ASTNode* node) {
    if (auto* n = dynamic_cast<StructDefNode*>(node)) return codegen(n);
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

// 存根实现 - 后续任务会填充
llvm::Value* CodeGen::codegen(StructDefNode* node) { return nullptr; }
llvm::Value* CodeGen::codegen(FuncDefNode* node) {
    // 确定返回类型
    llvm::Type* retType;
    if (node->returnType == "int32") {
        retType = builder->getInt32Ty();
    } else if (node->returnType == "float64") {
        retType = builder->getDoubleTy();
    } else if (node->returnType == "bool") {
        retType = builder->getInt1Ty();
    } else if (node->returnType == "void") {
        retType = builder->getVoidTy();
    } else if (node->returnType == "string") {
        retType = llvm::PointerType::get(getStringType(), 0);
    } else {
        // 结构体类型 - 在 context 中查找
        llvm::StructType* structTy = llvm::StructType::getTypeByName(*context, node->returnType);
        if (!structTy) {
            error("FuncDefNode: unknown return type: " + node->returnType);
            return nullptr;
        }
        retType = structTy;
    }

    // 构建函数类型
    std::vector<llvm::Type*> paramTypes;
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

    llvm::FunctionType* funcType = llvm::FunctionType::get(retType, paramTypes, false);
    llvm::Function* func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, node->name, module.get());

    // 设置参数名称
    size_t i = 0;
    for (auto& arg : func->args()) {
        if (i < node->params.size()) {
            arg.setName(node->params[i].second);
        }
        i++;
    }

    // 创建入口 block
    currentFunction = func;
    namedValues.clear();
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(entry);

    // 为参数创建 alloca
    i = 0;
    for (auto& arg : func->args()) {
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
            builder->CreateRetVoid();
        }
    }

    currentFunction = nullptr;
    return func;
}
llvm::Value* CodeGen::codegen(VarDeclNode* node) {
    if (!currentFunction) {
        error("VarDeclNode: no current function");
        return nullptr;
    }

    // 获取类型
    llvm::Type* varType;
    if (node->type == "int32") {
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
        builder->CreateRet(retVal);
    } else {
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
        if (left->getType()->isIntegerTy(32)) return builder->CreateSDiv(left, right, "div.tmp");
        else if (left->getType()->isDoubleTy()) return builder->CreateFDiv(left, right, "div.tmp");
    } else if (node->op == "%") {
        if (left->getType()->isIntegerTy(32)) return builder->CreateSRem(left, right, "rem.tmp");
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
    } else if (node->op == "!=") {
        if (left->getType()->isIntegerTy(1)) return builder->CreateICmpNE(left, right, "cmp.tmp");
        else if (left->getType()->isIntegerTy(32)) return builder->CreateICmpNE(left, right, "cmp.tmp");
        else if (left->getType()->isDoubleTy()) return builder->CreateFCmpONE(left, right, "cmp.tmp");
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
    llvm::Function* func = module->getFunction(node->callee);
    if (!func) {
        error("CallExpr: undefined function: " + node->callee);
        return nullptr;
    }
    std::vector<llvm::Value*> args;
    for (auto& argExpr : node->args) {
        llvm::Value* argVal = codegen(argExpr.get());
        if (!argVal) {
            error("CallExpr: failed to codegen argument");
            return nullptr;
        }
        args.push_back(argVal);
    }
    llvm::CallInst* call = builder->CreateCall(func, args);
    if (!func->getReturnType()->isVoidTy()) {
        call->setName("call.tmp");
    }
    return call;
}
llvm::Value* CodeGen::codegen(IndexExpr* node) { return nullptr; }
llvm::Value* CodeGen::codegen(MemberExpr* node) { return nullptr; }
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
    // 对于标识符直接查符号表，获取 alloca
    llvm::AllocaInst* alloca = nullptr;
    if (auto* ident = dynamic_cast<IdentExpr*>(node->target.get())) {
        auto it = namedValues.find(ident->name);
        if (it != namedValues.end()) {
            alloca = it->second;
        }
    }

    // 其他情况（MemberExpr, IndexExpr）需要通过 codegen 获取地址
    if (!alloca) {
        llvm::Value* target = codegen(node->target.get());
        if (!target) {
            error("AssignExpr: left side evaluation failed");
            return nullptr;
        }
        if (!llvm::isa<llvm::AllocaInst>(target)) {
            error("AssignExpr: left side must be a variable");
            return nullptr;
        }
        alloca = llvm::cast<llvm::AllocaInst>(target);
    }

    llvm::Value* value = codegen(node->value.get());
    if (!value) {
        error("AssignExpr: failed to codegen value");
        return nullptr;
    }

    builder->CreateStore(value, alloca);
    return value;
}
llvm::Value* CodeGen::codegen(InitListExpr* node) { return nullptr; }
