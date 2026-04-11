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
llvm::Value* CodeGen::codegen(FuncDefNode* node) { return nullptr; }
llvm::Value* CodeGen::codegen(VarDeclNode* node) { return nullptr; }
llvm::Value* CodeGen::codegen(BlockStmt* node) { return nullptr; }
llvm::Value* CodeGen::codegen(IfStmt* node) { return nullptr; }
llvm::Value* CodeGen::codegen(WhileStmt* node) { return nullptr; }
llvm::Value* CodeGen::codegen(ForStmt* node) { return nullptr; }
llvm::Value* CodeGen::codegen(SwitchStmt* node) { return nullptr; }
llvm::Value* CodeGen::codegen(ReturnStmt* node) { return nullptr; }
llvm::Value* CodeGen::codegen(BreakStmt* node) { return nullptr; }
llvm::Value* CodeGen::codegen(ContinueStmt* node) { return nullptr; }
llvm::Value* CodeGen::codegen(ExprStmt* node) { return nullptr; }
llvm::Value* CodeGen::codegen(LiteralExpr* node) { return nullptr; }
llvm::Value* CodeGen::codegen(IdentExpr* node) { return nullptr; }
llvm::Value* CodeGen::codegen(BinaryExpr* node) { return nullptr; }
llvm::Value* CodeGen::codegen(UnaryExpr* node) { return nullptr; }
llvm::Value* CodeGen::codegen(CallExpr* node) { return nullptr; }
llvm::Value* CodeGen::codegen(IndexExpr* node) { return nullptr; }
llvm::Value* CodeGen::codegen(MemberExpr* node) { return nullptr; }
llvm::Value* CodeGen::codegen(CastExpr* node) { return nullptr; }
llvm::Value* CodeGen::codegen(AssignExpr* node) { return nullptr; }
llvm::Value* CodeGen::codegen(InitListExpr* node) { return nullptr; }
