#include "Checker.h"
#include "Diagnostics.h"
#include <algorithm>
#include <iostream>

Checker::Checker() {}

void Checker::check(ProgramNode* program) {
    // 第一遍：收集定义
    pass1_collectDefinitions(program);

    // 第二遍：类型检查函数体
    pass2_checkBody(program);
}

void Checker::pass1_collectDefinitions(ASTNode* node) {
    auto* program = dynamic_cast<ProgramNode*>(node);
    if (!program) return;

    for (auto& decl : program->declarations) {
        if (auto* structDef = dynamic_cast<StructDefNode*>(decl.get())) {
            // 检查递归结构体
            for (auto& field : structDef->fields) {
                // field.first can be "Node" or "struct Node"
                if (field.first == structDef->name || field.first == ("struct " + structDef->name)) {
                    error(0, 0, "semantic error: recursive struct definition '" + structDef->name + "'");
                }
            }

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

    // 进入全局作用域
    enterScope();

    for (auto& decl : program->declarations) {
        if (auto* varDecl = dynamic_cast<VarDeclNode*>(decl.get())) {
            // 全局变量：添加到全局符号表
            globalVars.insert(varDecl->name);
            // 添加到当前作用域（用于函数体内引用）
            addToScope(varDecl->name, varDecl->type);
        }
        if (auto* funcDef = dynamic_cast<FuncDefNode*>(decl.get())) {
            currentFuncReturnType = funcDef->returnType;
            // Add function parameters to scope before checking body
            for (auto& param : funcDef->params) {
                addToScope(param.second, param.first); // name, type
            }
            if (funcDef->body) {
                auto* blockBody = dynamic_cast<BlockStmt*>(funcDef->body.get());
                if (blockBody) {
                    checkBlock(blockBody);
                }
            }
            // 检查非 void 函数是否有返回路径
            if (funcDef->returnType != "void" && !checkFunctionReturnPaths(funcDef)) {
                error(0, 0, "semantic error: non-void function '" + funcDef->name +
                          "' must have a return statement");
            }
            currentFuncReturnType = "";
        }
    }

    // 退出全局作用域
    exitScope();
}

void Checker::checkBlock(BlockStmt* stmt) {
    enterScope();
    for (auto& item : stmt->items) {
        checkStmt(item.get());
    }
    exitScope();
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
    } else if (auto* assign = dynamic_cast<AssignExpr*>(stmt)) {
        checkAssignExpr(assign);
    } else if (auto* blockStmt = dynamic_cast<BlockStmt*>(stmt)) {
        checkBlock(blockStmt);
    }
    // anything else - ignore
}

void Checker::checkVarDecl(VarDeclNode* decl) {
    // 检查矩阵类型维度限制
    if (isMatrixType(decl->type)) {
        auto [rows, cols] = parseMatrixDims(decl->type);

        // 检查维度必须为正整数
        if (rows <= 0 || cols <= 0) {
            error(0, 0, "semantic error: matrix dimensions must be positive integers");
        }

        // 检查总元素数不超过限制
        int64_t totalElements = static_cast<int64_t>(rows) * static_cast<int64_t>(cols);
        if (totalElements > MAX_ARRAY_SIZE) {
            error(0, 0, "semantic error: matrix total elements " +
                      std::to_string(totalElements) +
                      " exceeds maximum " + std::to_string(MAX_ARRAY_SIZE));
        }
    }

    // 检查数组大小限制
    checkArraySizeLimit(decl->type, 0, 0);
    if (decl->initializer) {
        checkExpr(decl->initializer.get());
        // Verify initializer type is compatible with declared type
        std::string valueType = getExprType(decl->initializer.get());
        std::string targetType = decl->type;

        // 禁止矩阵与普通二维数组互转
        if (isMatrixType(targetType) && is2DArrayType(valueType)) {
            error(0, 0, "semantic error: cannot convert array to matrix");
        }
        if (is2DArrayType(targetType) && isMatrixType(valueType)) {
            error(0, 0, "semantic error: cannot convert matrix to array");
        }

        if (!canImplicitConvert(valueType, targetType)) {
            error(0, 0, "semantic error: cannot convert '" + valueType +
                  "' to '" + targetType + "'");
        }
        // Validate struct init list if applicable
        if (auto* initList = dynamic_cast<InitListExpr*>(decl->initializer.get())) {
            validateStructInit(decl->type, initList, 0, 0);
        }
    }
    // Add variable to current scope for type resolution
    addToScope(decl->name, decl->type);
}

void Checker::checkIf(IfStmt* stmt) {
    if (stmt->condition) {
        std::string condType = getExprType(stmt->condition.get());
        if (isMatrixType(condType)) {
            error(0, 0, "semantic error: matrix cannot be used as if condition");
        }
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
        std::string condType = getExprType(stmt->condition.get());
        if (isMatrixType(condType)) {
            error(0, 0, "semantic error: matrix cannot be used as while condition");
        }
        if (!isBool(stmt->condition.get())) {
            error(0, 0, "semantic error: while condition must be bool");
        }
        checkExpr(stmt->condition.get());
    }
    bool prevInLoop = inLoop;
    int prevLoopDepth = loopDepth;
    inLoop = true;
    loopDepth++;
    if (stmt->body) {
        checkStmt(stmt->body.get());
    }
    inLoop = prevInLoop;
    loopDepth = prevLoopDepth;
}

void Checker::checkFor(ForStmt* stmt) {
    if (stmt->init) {
        checkStmt(stmt->init.get());
    }
    if (stmt->cond) {
        std::string condType = getExprType(stmt->cond.get());
        if (isMatrixType(condType)) {
            error(0, 0, "semantic error: matrix cannot be used as for condition");
        }
        if (!isBool(stmt->cond.get())) {
            error(0, 0, "semantic error: for condition must be bool");
        }
        checkExpr(stmt->cond.get());
    }
    if (stmt->update) {
        checkExpr(stmt->update.get());
    }
    bool prevInLoop = inLoop;
    int prevLoopDepth = loopDepth;
    inLoop = true;
    loopDepth++;
    if (stmt->body) {
        checkStmt(stmt->body.get());
    }
    inLoop = prevInLoop;
    loopDepth = prevLoopDepth;
}

void Checker::checkSwitch(SwitchStmt* stmt) {
    if (stmt->expr) {
        if (!isInt32(stmt->expr.get())) {
            error(0, 0, "semantic error: switch expression must be int32");
        }
        checkExpr(stmt->expr.get());
    }
    bool prevInSwitch = inSwitch;
    inSwitch = true;
    // NOTE: Do NOT reset inLoop or loopDepth here!
    // continue 在 switch 内但外层有循环时是有效的（跳转到外层循环的 increment）

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
        // 检查 case 的最后一条语句是否为 break 或 continue
        // continue 在 switch 内跳转到外层循环 increment，也算是有效结束（不会 fall-through）
        if (!casePair.second.empty()) {
            auto* lastStmt = casePair.second.back().get();
            bool hasBreakOrContinue = false;
            if (dynamic_cast<BreakStmt*>(lastStmt) || dynamic_cast<ContinueStmt*>(lastStmt)) {
                hasBreakOrContinue = true;
            }
            if (!hasBreakOrContinue) {
                error(0, 0, "semantic error: switch case must end with break or continue");
            }
        }
    }

    // 检查 default 分支（如果有）最后一条语句是否为 break
    if (!stmt->defaultBody.empty()) {
        for (auto& defaultItem : stmt->defaultBody) {
            checkStmt(defaultItem.get());
        }
        auto* lastDefaultStmt = stmt->defaultBody.back().get();
        bool hasBreak = false;
        if (auto* breakStmt = dynamic_cast<BreakStmt*>(lastDefaultStmt)) {
            hasBreak = true;
        }
        if (!hasBreak) {
            error(0, 0, "semantic error: switch default must end with break");
        }
    }

    inSwitch = prevInSwitch;
    // Note: inLoop and loopDepth are NOT restored because we didn't save them
    // They remain unchanged inside switch, allowing continue to work if outer loop exists
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
        checkBinary(bin);
        return;  // checkBinary already calls checkExpr on children
    } else if (auto* un = dynamic_cast<UnaryExpr*>(expr)) {
        checkUnary(un);
        return;  // checkUnary already calls checkExpr on children
    } else if (auto* call = dynamic_cast<CallExpr*>(expr)) {
        // 进入函数调用，增加嵌套深度
        currentNestingDepth++;
        checkNestingDepth(0, 0);  // 0,0 是占位位置
        for (auto& arg : call->args) {
            checkExpr(arg.get());
        }
        // 函数调用参数检查完毕，减少嵌套深度
        currentNestingDepth--;
    } else if (auto* cast = dynamic_cast<CastExpr*>(expr)) {
        checkExpr(cast->operand.get());
    } else if (auto* assign = dynamic_cast<AssignExpr*>(expr)) {
        checkAssignExpr(assign);
        return;  // checkAssignExpr already calls checkExpr on children
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
    if (auto* ident = dynamic_cast<IdentExpr*>(expr)) {
        // Look up identifier type from scope chain
        std::string type = lookupVar(ident->name);
        if (!type.empty()) {
            return type;
        }
        // Check if it's a global variable not yet in scope (forward reference)
        if (globalVars.count(ident->name)) {
            error(0, 0, "semantic error: global variable '" + ident->name + "' used before declaration");
        } else {
            error(0, 0, "semantic error: use of undeclared identifier '" + ident->name + "'");
        }
        return "int32"; // Return something to allow continued analysis
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

        // 矩阵运算返回矩阵类型
        if (isMatrixType(left) || isMatrixType(right)) {
            // 数乘：常量 * 矩阵 或 矩阵 * 常量 -> 返回矩阵类型
            if (bin->op == "*") {
                if (isMatrixType(left)) return left;
                if (isMatrixType(right)) return right;
            }
            // 矩阵加法/减法返回左操作数类型（两者维度相同）
            if (bin->op == "+" || bin->op == "-") {
                return left;
            }
        }

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
        std::string baseType = getExprType(idx->base.get());
        // 矩阵下标访问 mat[i][j] -> 返回元素类型
        if (isMatrixType(baseType)) {
            return getMatrixElementType(baseType);
        }
        // 数组下标访问 arr[i] -> 目前假设返回 int32
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
    if (auto* initList = dynamic_cast<InitListExpr*>(expr)) {
        // InitListExpr 本身不返回类型，需要从上下文确定
        // 目前返回 int32 作为占位
        return "int32";
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

bool Checker::isSupportedBinaryOp(const std::string& op) {
    // 支持的二元运算符
    return op == "+" || op == "-" || op == "*" || op == "/" || op == "%" ||
           op == "==" || op == "!=" || op == "<" || op == "<=" ||
           op == ">" || op == ">=" ||
           op == "&&" || op == "||";
}

bool Checker::isSupportedUnaryOp(const std::string& op) {
    // 支持的一元运算符：! (逻辑非) 和 - (负数)
    return op == "!" || op == "-";
}

bool Checker::isLiteralZero(ASTNode* expr) {
    if (auto* lit = dynamic_cast<LiteralExpr*>(expr)) {
        if (lit->literal.kind == TokenKind::INT_LIT) {
            try {
                int value = std::stoi(lit->literal.lexeme);
                return value == 0;
            } catch (...) {
                return false;
            }
        }
    }
    return false;
}

bool Checker::checkLiteralDivZero(BinaryExpr* expr) {
    if (expr->op == "/" || expr->op == "%") {
        if (isLiteralZero(expr->right.get())) {
            error(expr->line, expr->col, "semantic error: division by literal zero");
            return true;
        }
    }
    return false;
}

int64_t Checker::parseArraySize(const std::string& type) {
    // 解析类型字符串如 "int32[100]" 返回维度大小
    // 查找第一个 '[' 和 ']'
    size_t leftBracket = type.find('[');
    size_t rightBracket = type.find(']');
    if (leftBracket == std::string::npos || rightBracket == std::string::npos) {
        return 0;  // 不是数组类型
    }
    std::string sizeStr = type.substr(leftBracket + 1, rightBracket - leftBracket - 1);
    try {
        return std::stoll(sizeStr);
    } catch (...) {
        return 0;
    }
}

bool Checker::checkArraySizeLimit(const std::string& type, int line, int col) {
    // 检查数组大小是否超过限制
    size_t leftBracket = type.find('[');
    size_t rightBracket = type.find(']');
    if (leftBracket == std::string::npos || rightBracket == std::string::npos) {
        return false;  // 不是数组类型
    }
    std::string sizeStr = type.substr(leftBracket + 1, rightBracket - leftBracket - 1);
    try {
        int64_t size = std::stoll(sizeStr);
        if (size > MAX_ARRAY_SIZE) {
            error(line, col, "semantic error: array size " + std::to_string(size) +
                      " exceeds maximum " + std::to_string(MAX_ARRAY_SIZE));
            return true;
        }
    } catch (...) {
        // 解析失败，忽略
    }
    return false;
}

void Checker::checkNestingDepth(int line, int col) {
    if (currentNestingDepth > MAX_NESTING_DEPTH) {
        error(line, col, "semantic error: function call nesting depth " +
                  std::to_string(currentNestingDepth) +
                  " exceeds maximum " + std::to_string(MAX_NESTING_DEPTH));
    }
}

void Checker::checkBinary(BinaryExpr* expr) {
    if (!isSupportedBinaryOp(expr->op)) {
        error(expr->line, expr->col, "semantic error: unsupported binary operator '" + expr->op + "'");
    }

    // 检查字面量零除数
    checkLiteralDivZero(expr);

    std::string leftType = getExprType(expr->left.get());
    std::string rightType = getExprType(expr->right.get());

    // 检查矩阵比较（==/!=）
    if (expr->op == "==" || expr->op == "!=") {
        if (isMatrixType(leftType) || isMatrixType(rightType)) {
            error(expr->line, expr->col, "semantic error: matrix cannot be compared");
        }
        if (isArrayOrStructType(leftType) || isArrayOrStructType(rightType)) {
            error(expr->line, expr->col, "semantic error: array or struct cannot be compared");
        }
        checkExpr(expr->left.get());
        checkExpr(expr->right.get());
        return;
    }

    // 检查矩阵加减法维度匹配
    if (expr->op == "+" || expr->op == "-") {
        if (isMatrixType(leftType) && isMatrixType(rightType)) {
            // 矩阵加法/减法：检查维度相同
            checkMatrixDims(leftType, rightType, expr->line, expr->col,
                           expr->op == "+" ? "addition" : "subtraction");
            // 检查元素类型相同（禁止混合类型）
            if (getMatrixElementType(leftType) != getMatrixElementType(rightType)) {
                error(expr->line, expr->col, "semantic error: cannot perform matrix " + expr->op + " between int32 matrix and float64 matrix");
            }
            checkExpr(expr->left.get());
            checkExpr(expr->right.get());
            return;
        }
        // 数乘：int32/float64 * matrix 或 matrix * int32/float64
        if ((isMatrixType(leftType) && !isMatrixType(rightType) && !isArrayOrStructType(rightType)) ||
            (!isMatrixType(leftType) && !isArrayOrStructType(leftType) && isMatrixType(rightType))) {
            // 数乘类型已在 getExprType 中处理
            checkExpr(expr->left.get());
            checkExpr(expr->right.get());
            return;
        }
    }

    // 检查矩阵乘法维度匹配
    if (expr->op == "*") {
        if (isMatrixType(leftType) && isMatrixType(rightType)) {
            // 矩阵乘法：左矩阵列数 == 右矩阵行数
            auto [lRows, lCols] = parseMatrixDims(leftType);
            auto [rRows, rCols] = parseMatrixDims(rightType);
            if (lCols != rRows) {
                error(expr->line, expr->col, "semantic error: matrix multiplication requires left columns (" + std::to_string(lCols) + ") == right rows (" + std::to_string(rRows) + ")");
            }
            // 检查元素类型相同
            if (getMatrixElementType(leftType) != getMatrixElementType(rightType)) {
                error(expr->line, expr->col, "semantic error: cannot perform matrix " + expr->op + " between int32 matrix and float64 matrix");
            }
            checkExpr(expr->left.get());
            checkExpr(expr->right.get());
            return;
        }
        }

    checkExpr(expr->left.get());
    checkExpr(expr->right.get());
}

void Checker::checkUnary(UnaryExpr* expr) {
    if (!isSupportedUnaryOp(expr->op)) {
        error(0, 0, "semantic error: unsupported unary operator '" + expr->op + "'");
    }

    std::string operandType = getExprType(expr->operand.get());

    // 检查矩阵求负（-matrix）：无维度限制
    if (expr->op == "-") {
        if (isMatrixType(operandType)) {
            // 矩阵求负合法，无需额外检查
            checkExpr(expr->operand.get());
            return;
        }
    }

    // 检查矩阵不能用于逻辑非
    if (expr->op == "!") {
        if (isMatrixType(operandType)) {
            error(0, 0, "semantic error: matrix cannot be used with logical not");
        }
    }

    checkExpr(expr->operand.get());
}

void Checker::checkAssignExpr(AssignExpr* expr) {
    // QLang 不支持复合赋值（+=, -=, *=, /=, %=）
    // 赋值运算符只有简单的 =
    // checkAssignExpr 目前只需要检查子表达式
    checkExpr(expr->target.get());
    checkExpr(expr->value.get());
}

bool Checker::isStructType(const std::string& typeName) {
    // typeName can be "Point" or "struct Point"
    if (structs.find(typeName) != structs.end()) return true;
    // Check with "struct " prefix stripped
    if (typeName.substr(0, 7) == "struct ") {
        return structs.find(typeName.substr(7)) != structs.end();
    }
    return false;
}

bool Checker::isArrayOrStructType(const std::string& typeName) {
    // 数组类型包含 "["，例如 "int32[]"
    if (typeName.find('[') != std::string::npos) {
        return true;
    }
    // 结构体类型在 structs 符号表中
    return isStructType(typeName);
}

bool Checker::validateStructInit(const std::string& structName, InitListExpr* init, int line, int col) {
    auto it = structs.find(structName);
    if (it == structs.end()) return true; // 不是结构体，不检查

    const auto& fields = it->second.fields;
    if (init->elements.size() != fields.size()) {
        error(line, col, "semantic error: struct '" + structName +
                  "' initialization requires " + std::to_string(fields.size()) +
                  " values but got " + std::to_string(init->elements.size()));
        return false;
    }

    // 字段必须按顺序初始化（QLang 规则）
    // 目前只检查元素数量，顺序由 AST 结构保证
    return true;
}

bool Checker::hasReturnStatement(ASTNode* stmt) {
    if (!stmt) return false;

    // ReturnStmt 本身算作返回
    if (dynamic_cast<ReturnStmt*>(stmt)) {
        return true;
    }

    // BlockStmt 检查其中每条语句
    if (auto* block = dynamic_cast<BlockStmt*>(stmt)) {
        for (auto& item : block->items) {
            if (hasReturnStatement(item.get())) {
                return true;
            }
        }
        return false;
    }

    // IfStmt: 检查 then 和 else 分支
    if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        bool thenHasReturn = hasReturnStatement(ifStmt->thenBranch.get());
        bool elseHasReturn = hasReturnStatement(ifStmt->elseBranch.get());
        // 两个分支都有 return 才算有返回
        return thenHasReturn && elseHasReturn;
    }

    // WhileStmt/ForStmt: 循环内的 return 不算作函数返回路径保证
    // 因为循环可能不执行

    // 其他语句不包含 return
    return false;
}

bool Checker::checkFunctionReturnPaths(FuncDefNode* func) {
    if (func->returnType == "void") {
        return true;  // void 函数不需要返回
    }

    if (!func->body) {
        return false;  // 无函数体视为无返回
    }

    return hasReturnStatement(func->body.get());
}

bool Checker::isMatrixType(const std::string& typeName) {
    // 矩阵类型格式: "mat<int32> 2x3"
    return typeName.substr(0, 4) == "mat<";
}

bool Checker::is2DArrayType(const std::string& typeName) {
    // 二维数组格式: "int32[2][3]" - 包含两个 [ ]
    size_t firstBracket = typeName.find('[');
    if (firstBracket == std::string::npos) return false;
    size_t secondBracket = typeName.find('[', firstBracket + 1);
    return secondBracket != std::string::npos;
}

std::pair<int, int> Checker::parseMatrixDims(const std::string& matrixType) {
    // 解析 "mat<int32> 2x3" 返回 {2, 3}
    // 格式: "mat<element_type> rowsxcols"
    size_t spacePos = matrixType.find(' ');
    if (spacePos == std::string::npos) return {0, 0};
    std::string dims = matrixType.substr(spacePos + 1);  // "2x3"

    size_t xPos = dims.find('x');
    if (xPos == std::string::npos) return {0, 0};

    try {
        int rows = std::stoi(dims.substr(0, xPos));
        int cols = std::stoi(dims.substr(xPos + 1));
        return {rows, cols};
    } catch (...) {
        return {0, 0};
    }
}

std::string Checker::getMatrixElementType(const std::string& matrixType) {
    // 解析 "mat<int32> 2x3" 返回 "int32"
    if (!isMatrixType(matrixType)) return "";
    size_t ltPos = matrixType.find('<');
    size_t gtPos = matrixType.find('>');
    if (ltPos == std::string::npos || gtPos == std::string::npos) return "";
    return matrixType.substr(ltPos + 1, gtPos - ltPos - 1);
}

bool Checker::checkMatrixDims(const std::string& dims1, const std::string& dims2,
                               int line, int col, const std::string& opName) {
    auto [r1, c1] = parseMatrixDims(dims1);
    auto [r2, c2] = parseMatrixDims(dims2);
    if (r1 != r2 || c1 != c2) {
        error(line, col, "semantic error: matrix " + opName +
                  " requires same dimensions (" + dims1 + " vs " + dims2 + ")");
        return false;
    }
    return true;
}