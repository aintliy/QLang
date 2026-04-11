#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "AST.h"

// 结构体定义符号表条目
struct StructInfo {
    std::string name;
    std::vector<std::pair<std::string, std::string>> fields; // type, name
};

// 函数定义符号表条目
struct FuncInfo {
    std::string returnType;
    std::string name;
    std::vector<std::pair<std::string, std::string>> params; // type, name
};

class Checker {
public:
    explicit Checker(std::unique_ptr<ProgramNode> ast);
    void check();  // 执行两遍检查

    // 供下游使用（codegen）
    const std::unordered_map<std::string, StructInfo>& getStructs() const { return structs; }
    const std::unordered_map<std::string, FuncInfo>& getFuncs() const { return funcs; }

private:
    std::unique_ptr<ProgramNode> ast;

    // 符号表（在第一遍填充）
    std::unordered_map<std::string, StructInfo> structs;
    std::unordered_map<std::string, FuncInfo> funcs;

    // 作用域栈（用于块作用域）
    std::vector<std::unordered_map<std::string, std::string>> scopeStack;

    void enterScope() { scopeStack.push_back({}); }
    void exitScope() { if (!scopeStack.empty()) scopeStack.pop_back(); }
    void addToScope(const std::string& name, const std::string& type) {
        if (!scopeStack.empty()) scopeStack.back()[name] = type;
    }

    // 在当前作用域链中查找变量类型（从内到外）
    std::string lookupVar(const std::string& name) {
        for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
            auto f = it->find(name);
            if (f != it->end()) return f->second;
        }
        return "";
    }

    // 当前函数上下文（用于返回类型检查）
    std::string currentFuncReturnType;
    bool inLoop = false;    // 用于 break/continue
    bool inSwitch = false;  // 用于 break

    // 第一遍：收集结构体和函数定义
    void pass1_collectDefinitions(ASTNode* node);

    // 第二遍：类型检查函数体
    void pass2_checkBody(ASTNode* node);

    // 类型工具函数
    std::string getExprType(ASTNode* expr);
    bool isInt32(ASTNode* expr);
    bool isFloat64(ASTNode* expr);
    bool isBool(ASTNode* expr);
    bool canImplicitConvert(const std::string& from, const std::string& to);

    // 验证辅助函数
    void checkAssignCompatibility(ASTNode* target, ASTNode* value, int line, int col);
    void checkReturn(ASTNode* ret, int line, int col);
    bool isConstantIntExpr(ASTNode* expr);

    // 语句检查
    void checkStmt(ASTNode* stmt);
    void checkVarDecl(VarDeclNode* decl);
    void checkIf(IfStmt* stmt);
    void checkWhile(WhileStmt* stmt);
    void checkFor(ForStmt* stmt);
    void checkSwitch(SwitchStmt* stmt);
    void checkReturn(ReturnStmt* stmt);
    void checkBreak(BreakStmt* stmt);
    void checkContinue(ContinueStmt* stmt);
    void checkExprStmt(ExprStmt* stmt);
    void checkBlock(BlockStmt* stmt);

    // 表达式检查
    void checkExpr(ASTNode* expr);
    void checkBinary(BinaryExpr* expr);
    void checkUnary(UnaryExpr* expr);
    void checkCall(CallExpr* expr);
    void checkCast(CastExpr* expr);
    void checkAssignExpr(AssignExpr* expr);

    // 结构体相关检查
    bool isStructType(const std::string& typeName);
    bool validateStructInit(const std::string& structName, InitListExpr* init, int line, int col);
};