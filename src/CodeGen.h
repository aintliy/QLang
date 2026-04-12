#pragma once
#include "AST.h"
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

class CodeGen {
public:
    explicit CodeGen();
    ~CodeGen();

    // 生成整个程序的 IR
    std::unique_ptr<llvm::Module> codegen(ProgramNode* program);

    // 类型映射
    llvm::Type* getInt32Type();
    llvm::Type* getFloat64Type();
    llvm::Type* getBoolType();
    llvm::Type* getStringType();

private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    // 符号表：变量名 -> AllocaInst*
    std::map<std::string, llvm::AllocaInst*> namedValues;

    // 当前函数
    llvm::Function* currentFunction = nullptr;

    // 运行时库声明
    void declareRuntimeFunctions();

    // 辅助方法
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* func, const std::string& name, llvm::Type* type);
    llvm::Value* codegen(ASTNode* node);

    // 声明处理
    llvm::Type* codegen(StructDefNode* node);
    llvm::Value* codegen(FuncDefNode* node);
    llvm::Value* codegen(VarDeclNode* node);

    // 语句处理
    llvm::Value* codegen(BlockStmt* node);
    llvm::Value* codegen(IfStmt* node);
    llvm::Value* codegen(WhileStmt* node);
    llvm::Value* codegen(ForStmt* node);
    llvm::Value* codegen(SwitchStmt* node);
    llvm::Value* codegen(ReturnStmt* node);
    llvm::Value* codegen(BreakStmt* node);
    llvm::Value* codegen(ContinueStmt* node);
    llvm::Value* codegen(ExprStmt* node);

    // break/continue 跳转目标栈
    struct LoopTarget {
        llvm::BasicBlock* endBB;   // break 跳转目标
        llvm::BasicBlock* incBB;   // continue 跳转目标（while 没有 increment）
    };
    std::vector<LoopTarget> loopTargetStack;

    // 表达式处理
    llvm::Value* codegen(LiteralExpr* node);
    llvm::Value* codegen(IdentExpr* node);
    llvm::Value* codegen(BinaryExpr* node);
    llvm::Value* codegen(UnaryExpr* node);
    llvm::Value* codegen(CallExpr* node);
    llvm::Value* codegen(IndexExpr* node);
    llvm::Value* codegen(MemberExpr* node);
    llvm::Value* codegen(CastExpr* node);
    llvm::Value* codegen(AssignExpr* node);
    llvm::Value* codegen(InitListExpr* node);
};