#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Token.h"

// All AST nodes base class
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void dump() const = 0;
};

// ===== Declarations =====

class ProgramNode : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> declarations;
    void dump() const override;
};

class StructDefNode : public ASTNode {
public:
    std::string name;
    std::vector<std::pair<std::string, std::string>> fields; // type, name
    void dump() const override;
};

class FuncDefNode : public ASTNode {
public:
    std::string returnType;
    std::string name;
    std::vector<std::pair<std::string, std::string>> params; // type, name
    std::unique_ptr<ASTNode> body;
    void dump() const override;
};

class VarDeclNode : public ASTNode {
public:
    std::string type;
    std::string name;
    std::unique_ptr<ASTNode> initializer;
    void dump() const override;
};

// ===== Statements =====

class BlockStmt : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> items;
    void dump() const override;
};

class IfStmt : public ASTNode {
public:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> thenBranch;
    std::unique_ptr<ASTNode> elseBranch; // may be nullptr
    void dump() const override;
};

class WhileStmt : public ASTNode {
public:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;
    void dump() const override;
};

class ForStmt : public ASTNode {
public:
    std::unique_ptr<ASTNode> init;
    std::unique_ptr<ASTNode> cond;
    std::unique_ptr<ASTNode> update;
    std::unique_ptr<ASTNode> body;
    void dump() const override;
};

class SwitchStmt : public ASTNode {
public:
    std::unique_ptr<ASTNode> expr;
    std::vector<std::pair<std::unique_ptr<ASTNode>, std::vector<std::unique_ptr<ASTNode>>>> cases;
    std::vector<std::unique_ptr<ASTNode>> defaultBody;
    void dump() const override;
};

class ReturnStmt : public ASTNode {
public:
    std::unique_ptr<ASTNode> value; // may be nullptr
    void dump() const override;
};

class BreakStmt : public ASTNode {
public:
    void dump() const override;
};

class ContinueStmt : public ASTNode {
public:
    void dump() const override;
};

class ExprStmt : public ASTNode {
public:
    std::unique_ptr<ASTNode> expr;
    void dump() const override;
};

// ===== Expressions =====

class LiteralExpr : public ASTNode {
public:
    Token literal;
    void dump() const override;
};

class IdentExpr : public ASTNode {
public:
    std::string name;
    void dump() const override;
};

class BinaryExpr : public ASTNode {
public:
    std::string op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    int line;
    int col;
    void dump() const override;
};

class UnaryExpr : public ASTNode {
public:
    std::string op;
    std::unique_ptr<ASTNode> operand;
    int line;
    int col;
    void dump() const override;
};

class CallExpr : public ASTNode {
public:
    std::string callee;
    std::vector<std::unique_ptr<ASTNode>> args;
    void dump() const override;
};

class IndexExpr : public ASTNode {
public:
    std::unique_ptr<ASTNode> base;
    std::unique_ptr<ASTNode> index;
    void dump() const override;
};

class MemberExpr : public ASTNode {
public:
    std::unique_ptr<ASTNode> object;
    std::string member;
    void dump() const override;
};

class CastExpr : public ASTNode {
public:
    std::string targetType;
    std::unique_ptr<ASTNode> operand;
    void dump() const override;
};

class AssignExpr : public ASTNode {
public:
    std::unique_ptr<ASTNode> target;
    std::unique_ptr<ASTNode> value;
    void dump() const override;
};

class InitListExpr : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> elements;
    void dump() const override;
};