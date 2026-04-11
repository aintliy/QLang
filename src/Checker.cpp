#include "Checker.h"
#include "Diagnostics.h"
#include <algorithm>
#include <iostream>

Checker::Checker(std::unique_ptr<ProgramNode> ast) : ast(std::move(ast)) {}

void Checker::check() {
    // 第一遍：收集定义
    pass1_collectDefinitions(ast.get());

    // 第二遍：类型检查函数体
    pass2_checkBody(ast.get());
}

void Checker::pass1_collectDefinitions(ASTNode* node) {
    auto* program = dynamic_cast<ProgramNode*>(node);
    if (!program) return;

    for (auto& decl : program->declarations) {
        if (auto* structDef = dynamic_cast<StructDefNode*>(decl.get())) {
            StructInfo info;
            info.name = structDef->name;
            info.fields = structDef->fields;
            auto result = structs.emplace(structDef->name, std::move(info));
            if (!result.second) {
                error(0, 0, "semantic error: duplicate struct definition '" + structDef->name + "'");
            }
        } else if (auto* funcDef = dynamic_cast<FuncDefNode*>(decl.get())) {
            FuncInfo info;
            info.returnType = funcDef->returnType;
            info.name = funcDef->name;
            info.params = funcDef->params;
            auto result = funcs.emplace(funcDef->name, std::move(info));
            if (!result.second) {
                error(0, 0, "semantic error: duplicate function definition '" + funcDef->name + "'");
            }
        }
    }
}

void Checker::pass2_checkBody(ASTNode* node) {
    auto* program = dynamic_cast<ProgramNode*>(node);
    if (!program) return;

    for (auto& decl : program->declarations) {
        if (auto* funcDef = dynamic_cast<FuncDefNode*>(decl.get())) {
            currentFuncReturnType = funcDef->returnType;
            if (funcDef->body) {
                checkBlock(dynamic_cast<BlockStmt*>(funcDef->body.get()));
            }
            currentFuncReturnType = "";
        }
    }
}

void Checker::checkBlock(BlockStmt* stmt) {
    for (auto& item : stmt->items) {
        checkStmt(item.get());
    }
}

void Checker::checkStmt(ASTNode* stmt) {
    if (!stmt) return;

    if (auto* varDecl = dynamic_cast<VarDeclNode*>(stmt)) {
        checkVarDecl(varDecl);
    } else if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        checkIf(ifStmt);
    } else if (auto* whileStmt = dynamic_cast<WhileStmt*>(stmt)) {
        checkWhile(whileStmt);
    } else if (auto* forStmt = dynamic_cast<ForStmt*>(stmt)) {
        checkFor(forStmt);
    } else if (auto* switchStmt = dynamic_cast<SwitchStmt*>(stmt)) {
        checkSwitch(switchStmt);
    } else if (auto* returnStmt = dynamic_cast<ReturnStmt*>(stmt)) {
        checkReturn(returnStmt);
    } else if (auto* breakStmt = dynamic_cast<BreakStmt*>(stmt)) {
        checkBreak(breakStmt);
    } else if (auto* continueStmt = dynamic_cast<ContinueStmt*>(stmt)) {
        checkContinue(continueStmt);
    } else if (auto* exprStmt = dynamic_cast<ExprStmt*>(stmt)) {
        checkExprStmt(exprStmt);
    } else if (auto* blockStmt = dynamic_cast<BlockStmt*>(stmt)) {
        checkBlock(blockStmt);
    }
    // anything else - ignore
}

void Checker::checkVarDecl(VarDeclNode* decl) {
    if (decl->initializer) {
        checkExpr(decl->initializer.get());
    }
}

void Checker::checkIf(IfStmt* stmt) {
    if (stmt->condition) {
        if (!isBool(stmt->condition.get())) {
            error(0, 0, "semantic error: if condition must be bool");
        }
        checkExpr(stmt->condition.get());
    }
    if (stmt->thenBranch) {
        checkStmt(stmt->thenBranch.get());
    }
    if (stmt->elseBranch) {
        checkStmt(stmt->elseBranch.get());
    }
}

void Checker::checkWhile(WhileStmt* stmt) {
    if (stmt->condition) {
        if (!isBool(stmt->condition.get())) {
            error(0, 0, "semantic error: while condition must be bool");
        }
        checkExpr(stmt->condition.get());
    }
    bool prevInLoop = inLoop;
    inLoop = true;
    if (stmt->body) {
        checkStmt(stmt->body.get());
    }
    inLoop = prevInLoop;
}

void Checker::checkFor(ForStmt* stmt) {
    if (stmt->init) {
        checkStmt(stmt->init.get());
    }
    if (stmt->cond) {
        if (!isBool(stmt->cond.get())) {
            error(0, 0, "semantic error: for condition must be bool");
        }
        checkExpr(stmt->cond.get());
    }
    if (stmt->update) {
        checkExpr(stmt->update.get());
    }
    bool prevInLoop = inLoop;
    inLoop = true;
    if (stmt->body) {
        checkStmt(stmt->body.get());
    }
    inLoop = prevInLoop;
}

void Checker::checkSwitch(SwitchStmt* stmt) {
    if (stmt->expr) {
        if (!isInt32(stmt->expr.get())) {
            error(0, 0, "semantic error: switch expression must be int32");
        }
        checkExpr(stmt->expr.get());
    }
    for (auto& casePair : stmt->cases) {
        if (casePair.first) {
            if (!isConstantIntExpr(casePair.first.get())) {
                error(0, 0, "semantic error: switch case label must be constant int expression");
            }
            checkExpr(casePair.first.get());
        }
        for (auto& caseItem : casePair.second) {
            checkStmt(caseItem.get());
        }
    }
    bool prevInSwitch = inSwitch;
    inSwitch = true;
    for (auto& defaultItem : stmt->defaultBody) {
        checkStmt(defaultItem.get());
    }
    inSwitch = prevInSwitch;
}

void Checker::checkBreak(BreakStmt* stmt) {
    if (!inSwitch && !inLoop) {
        error(0, 0, "semantic error: break outside switch or loop");
    }
}

void Checker::checkContinue(ContinueStmt* stmt) {
    if (!inLoop) {
        error(0, 0, "semantic error: continue outside loop");
    }
}

void Checker::checkExprStmt(ExprStmt* stmt) {
    if (stmt->expr) {
        checkExpr(stmt->expr.get());
    }
}

void Checker::checkExpr(ASTNode* expr) {
    if (!expr) return;

    if (auto* bin = dynamic_cast<BinaryExpr*>(expr)) {
        checkExpr(bin->left.get());
        checkExpr(bin->right.get());
    } else if (auto* un = dynamic_cast<UnaryExpr*>(expr)) {
        checkExpr(un->operand.get());
    } else if (auto* call = dynamic_cast<CallExpr*>(expr)) {
        for (auto& arg : call->args) {
            checkExpr(arg.get());
        }
    } else if (auto* cast = dynamic_cast<CastExpr*>(expr)) {
        checkExpr(cast->operand.get());
    } else if (auto* assign = dynamic_cast<AssignExpr*>(expr)) {
        checkExpr(assign->target.get());
        checkExpr(assign->value.get());
    } else if (auto* idx = dynamic_cast<IndexExpr*>(expr)) {
        checkExpr(idx->base.get());
        checkExpr(idx->index.get());
    } else if (auto* member = dynamic_cast<MemberExpr*>(expr)) {
        checkExpr(member->object.get());
    } else if (auto* initList = dynamic_cast<InitListExpr*>(expr)) {
        for (auto& elem : initList->elements) {
            checkExpr(elem.get());
        }
    }
    // LiteralExpr, IdentExpr don't have sub-expressions to check
}

std::string Checker::getExprType(ASTNode* expr) {
    if (auto* lit = dynamic_cast<LiteralExpr*>(expr)) {
        const auto& tk = lit->literal;
        if (tk.kind == TokenKind::INT_LIT) return "int32";
        if (tk.kind == TokenKind::FLOAT_LIT) return "float64";
        if (tk.kind == TokenKind::STRING_LIT) return "string";
        if (tk.kind == TokenKind::KEYWORD_TRUE || tk.kind == TokenKind::KEYWORD_FALSE) return "bool";
    }
    if (dynamic_cast<IdentExpr*>(expr)) {
        // 标识符需要从符号表追踪类型
        // 目前假设为 int32（在后续任务中完善）
        return "int32";
    }
    if (auto* bin = dynamic_cast<BinaryExpr*>(expr)) {
        if (bin->op == "==" || bin->op == "!=" || bin->op == "<" ||
            bin->op == ">" || bin->op == "<=" || bin->op == ">=" ||
            bin->op == "&&" || bin->op == "||") {
            return "bool";
        }
        // 算术运算 - 如果任一操作数是 float64，结果是 float64，否则是 int32
        std::string left = getExprType(bin->left.get());
        std::string right = getExprType(bin->right.get());
        if (left == "float64" || right == "float64") return "float64";
        return "int32";
    }
    if (auto* un = dynamic_cast<UnaryExpr*>(expr)) {
        if (un->op == "!") return "bool";
        if (un->op == "-") return getExprType(un->operand.get());
    }
    if (auto* call = dynamic_cast<CallExpr*>(expr)) {
        auto it = funcs.find(call->callee);
        if (it != funcs.end()) return it->second.returnType;
        return "int32";
    }
    if (auto* idx = dynamic_cast<IndexExpr*>(expr)) {
        // 数组元素类型 - 目前假设为 int32
        return "int32";
    }
    if (auto* member = dynamic_cast<MemberExpr*>(expr)) {
        // 结构体成员类型 - 在后续支持结构体时实现
        return "int32";
    }
    if (auto* cast = dynamic_cast<CastExpr*>(expr)) {
        return cast->targetType;
    }
    if (auto* assign = dynamic_cast<AssignExpr*>(expr)) {
        return getExprType(assign->value.get());
    }
    if (dynamic_cast<InitListExpr*>(expr)) {
        return "int32"; // TODO: 从上下文推导
    }
    return "int32";
}

bool Checker::isInt32(ASTNode* expr) { return getExprType(expr) == "int32"; }
bool Checker::isFloat64(ASTNode* expr) { return getExprType(expr) == "float64"; }
bool Checker::isBool(ASTNode* expr) { return getExprType(expr) == "bool"; }

bool Checker::isConstantIntExpr(ASTNode* expr) {
    if (auto* lit = dynamic_cast<LiteralExpr*>(expr)) {
        return lit->literal.kind == TokenKind::INT_LIT;
    }
    if (auto* un = dynamic_cast<UnaryExpr*>(expr)) {
        if (un->op == "-") return isConstantIntExpr(un->operand.get());
    }
    if (auto* bin = dynamic_cast<BinaryExpr*>(expr)) {
        if (bin->op == "+" || bin->op == "-" || bin->op == "*" || bin->op == "/") {
            return isConstantIntExpr(bin->left.get()) && isConstantIntExpr(bin->right.get());
        }
    }
    return false;
}

bool Checker::canImplicitConvert(const std::string& from, const std::string& to) {
    // 同类型总是允许
    if (from == to) return true;

    // int32 <-> float64: 永远不允许隐式转换
    if ((from == "int32" && to == "float64") || (from == "float64" && to == "int32")) {
        return false;
    }

    // bool <-> int32: 永远不允许隐式转换
    if ((from == "bool" && to == "int32") || (from == "int32" && to == "bool")) {
        return false;
    }

    // string 不能与其他类型互转
    return false;
}

void Checker::checkAssignCompatibility(ASTNode* target, ASTNode* value, int line, int col) {
    std::string targetType = "int32"; // TODO: 从符号表解析
    std::string valueType = getExprType(value);

    if (!canImplicitConvert(valueType, targetType)) {
        error(line, col, "semantic error: cannot convert '" + valueType +
              "' to '" + targetType + "'");
    }
}

void Checker::checkReturn(ASTNode* ret, int line, int col) {
    if (!ret) {
        // return; 在 void 函数中 - OK
        if (currentFuncReturnType != "void") {
            error(line, col, "semantic error: return with no value in non-void function returning '" +
                      currentFuncReturnType + "'");
        }
        return;
    }

    // return expr;
    std::string exprType = getExprType(ret);
    std::string retType = currentFuncReturnType;

    // void 函数带返回值
    if (retType == "void") {
        error(line, col, "semantic error: return with value in void function");
        return;
    }

    // 非 void 函数无返回路径 - 在函数级别处理
    if (!canImplicitConvert(exprType, retType)) {
        error(line, col, "semantic error: cannot convert return type '" + exprType +
                  "' to '" + retType + "'");
    }
}

// Overload for ReturnStmt - extracts value and delegates
void Checker::checkReturn(ReturnStmt* stmt) {
    // Extract line/col from stmt if it has location info, otherwise use 0,0
    checkReturn(stmt->value.get(), 0, 0);
}