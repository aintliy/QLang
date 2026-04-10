#include "AST.h"
#include <iostream>

// ===== Declarations =====

void ProgramNode::dump() const {
    std::cerr << "ProgramNode(declarations=" << declarations.size() << ")" << std::endl;
    for (auto& decl : declarations) {
        decl->dump();
    }
}

void StructDefNode::dump() const {
    std::cerr << "StructDefNode(name=" << name << ", fields=" << fields.size() << ")" << std::endl;
    for (auto& field : fields) {
        std::cerr << "  field: " << field.first << " " << field.second << std::endl;
    }
}

void FuncDefNode::dump() const {
    std::cerr << "FuncDefNode(returnType=" << returnType << ", name=" << name << ", params=" << params.size() << ")" << std::endl;
    for (auto& param : params) {
        std::cerr << "  param: " << param.first << " " << param.second << std::endl;
    }
    if (body) {
        body->dump();
    }
}

void VarDeclNode::dump() const {
    std::cerr << "VarDeclNode(type=" << type << ", name=" << name << ")" << std::endl;
    if (initializer) {
        initializer->dump();
    }
}

// ===== Statements =====

void BlockStmt::dump() const {
    std::cerr << "BlockStmt(items=" << items.size() << ")" << std::endl;
    for (auto& item : items) {
        item->dump();
    }
}

void IfStmt::dump() const {
    std::cerr << "IfStmt()" << std::endl;
    std::cerr << "  condition:" << std::endl;
    if (condition) condition->dump();
    std::cerr << "  thenBranch:" << std::endl;
    if (thenBranch) thenBranch->dump();
    if (elseBranch) {
        std::cerr << "  elseBranch:" << std::endl;
        elseBranch->dump();
    }
}

void WhileStmt::dump() const {
    std::cerr << "WhileStmt()" << std::endl;
    std::cerr << "  condition:" << std::endl;
    if (condition) condition->dump();
    std::cerr << "  body:" << std::endl;
    if (body) body->dump();
}

void ForStmt::dump() const {
    std::cerr << "ForStmt()" << std::endl;
    std::cerr << "  init:" << std::endl;
    if (init) init->dump();
    std::cerr << "  cond:" << std::endl;
    if (cond) cond->dump();
    std::cerr << "  update:" << std::endl;
    if (update) update->dump();
    std::cerr << "  body:" << std::endl;
    if (body) body->dump();
}

void SwitchStmt::dump() const {
    std::cerr << "SwitchStmt()" << std::endl;
    std::cerr << "  expr:" << std::endl;
    if (expr) expr->dump();
    std::cerr << "  cases=" << cases.size() << std::endl;
    for (auto& casePair : cases) {
        std::cerr << "    case:" << std::endl;
        if (casePair.first) casePair.first->dump();
        std::cerr << "    body:" << std::endl;
        for (auto& stmt : casePair.second) {
            if (stmt) stmt->dump();
        }
    }
    std::cerr << "  defaultBody=" << defaultBody.size() << std::endl;
    for (auto& stmt : defaultBody) {
        if (stmt) stmt->dump();
    }
}

void ReturnStmt::dump() const {
    std::cerr << "ReturnStmt()" << std::endl;
    if (value) value->dump();
}

void BreakStmt::dump() const {
    std::cerr << "BreakStmt()" << std::endl;
}

void ContinueStmt::dump() const {
    std::cerr << "ContinueStmt()" << std::endl;
}

void ExprStmt::dump() const {
    std::cerr << "ExprStmt()" << std::endl;
    if (expr) expr->dump();
}

// ===== Expressions =====

void LiteralExpr::dump() const {
    std::cerr << "LiteralExpr(literal=" << literal.lexeme << ")" << std::endl;
}

void IdentExpr::dump() const {
    std::cerr << "IdentExpr(name=" << name << ")" << std::endl;
}

void BinaryExpr::dump() const {
    std::cerr << "BinaryExpr(op=" << op << ")" << std::endl;
    std::cerr << "  left:" << std::endl;
    if (left) left->dump();
    std::cerr << "  right:" << std::endl;
    if (right) right->dump();
}

void UnaryExpr::dump() const {
    std::cerr << "UnaryExpr(op=" << op << ")" << std::endl;
    std::cerr << "  operand:" << std::endl;
    if (operand) operand->dump();
}

void CallExpr::dump() const {
    std::cerr << "CallExpr(callee=" << callee << ", args=" << args.size() << ")" << std::endl;
    for (auto& arg : args) {
        if (arg) arg->dump();
    }
}

void IndexExpr::dump() const {
    std::cerr << "IndexExpr()" << std::endl;
    std::cerr << "  base:" << std::endl;
    if (base) base->dump();
    std::cerr << "  index:" << std::endl;
    if (index) index->dump();
}

void MemberExpr::dump() const {
    std::cerr << "MemberExpr(member=" << member << ")" << std::endl;
    std::cerr << "  object:" << std::endl;
    if (object) object->dump();
}

void CastExpr::dump() const {
    std::cerr << "CastExpr(targetType=" << targetType << ")" << std::endl;
    std::cerr << "  operand:" << std::endl;
    if (operand) operand->dump();
}

void AssignExpr::dump() const {
    std::cerr << "AssignExpr()" << std::endl;
    std::cerr << "  target:" << std::endl;
    if (target) target->dump();
    std::cerr << "  value:" << std::endl;
    if (value) value->dump();
}

void InitListExpr::dump() const {
    std::cerr << "InitListExpr(elements=" << elements.size() << ")" << std::endl;
    for (auto& elem : elements) {
        if (elem) elem->dump();
    }
}