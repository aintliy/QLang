#include "Parser.h"
#include "Diagnostics.h"

Parser::Parser(Lexer& lexer) : lexer(lexer), current(TokenKind::END_OF_FILE, "", 0, 0) {
    advance();
}

void Parser::advance() {
    current = lexer.nextToken();
}

bool Parser::check(TokenKind kind) const {
    return current.kind == kind;
}

bool Parser::match(TokenKind kind) {
    if (check(kind)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenKind kind, const std::string& errorMsg) {
    if (check(kind)) {
        Token t = current;
        advance();
        return t;
    }
    error(current.line, current.col, errorMsg);
    return Token(TokenKind::ERROR, "", current.line, current.col); // never reached
}

std::unique_ptr<ProgramNode> Parser::parseProgram() {
    auto program = std::make_unique<ProgramNode>();
    while (!check(TokenKind::END_OF_FILE)) {
        program->declarations.push_back(parseTopLevelDecl());
    }
    return program;
}

std::unique_ptr<ASTNode> Parser::parseTopLevelDecl() {
    if (check(TokenKind::KEYWORD_STRUCT)) {
        return parseStructDef();
    }

    // Function or variable - parse type first
    std::string type = parseType();
    std::string name = consume(TokenKind::IDENT, "expected identifier").lexeme;

    if (match(TokenKind::LPAREN)) {
        // Function definition
        auto node = std::make_unique<FuncDefNode>();
        node->returnType = type;
        node->name = name;
        // Parse params
        if (!check(TokenKind::RPAREN)) {
            do {
                std::string paramType = parseType();
                std::string paramName = consume(TokenKind::IDENT, "expected parameter name").lexeme;
                node->params.emplace_back(paramType, paramName);
            } while (match(TokenKind::COMMA));
        }
        consume(TokenKind::RPAREN, "expected ')'");
        node->body = parseBlock();
        return node;
    } else {
        // Variable declaration
        auto node = std::make_unique<VarDeclNode>();
        node->type = type;
        node->name = name;
        if (match(TokenKind::ASSIGN)) {
            node->initializer = parseInitializer();
        }
        consume(TokenKind::SEMICOLON, "expected ';'");
        return node;
    }
}

std::unique_ptr<StructDefNode> Parser::parseStructDef() {
    consume(TokenKind::KEYWORD_STRUCT, "expected 'struct'");
    Token name = consume(TokenKind::IDENT, "expected struct name");
    consume(TokenKind::LBRACE, "expected '{'");

    auto node = std::make_unique<StructDefNode>();
    node->name = name.lexeme;

    while (!check(TokenKind::RBRACE) && !check(TokenKind::END_OF_FILE)) {
        std::string fieldType = parseType();
        Token fieldName = consume(TokenKind::IDENT, "expected field name");
        consume(TokenKind::SEMICOLON, "expected ';'");
        node->fields.emplace_back(fieldType, fieldName.lexeme);
    }

    consume(TokenKind::RBRACE, "expected '}'");
    return node;
}

std::unique_ptr<FuncDefNode> Parser::parseFuncDef() {
    error(current.line, current.col, "parseFuncDef not implemented");
    return nullptr;
}

std::unique_ptr<VarDeclNode> Parser::parseVarDecl() {
    std::string type = parseType();
    Token name = consume(TokenKind::IDENT, "expected variable name");

    auto node = std::make_unique<VarDeclNode>();
    node->type = type;
    node->name = name.lexeme;

    if (match(TokenKind::ASSIGN)) {
        node->initializer = parseInitializer();
    }
    consume(TokenKind::SEMICOLON, "expected ';'");
    return node;
}

std::string Parser::parseType() {
    Token typeToken;

    if (check(TokenKind::KEYWORD_INT32)) {
        typeToken = consume(TokenKind::KEYWORD_INT32, "expected int32");
    } else if (check(TokenKind::KEYWORD_FLOAT64)) {
        typeToken = consume(TokenKind::KEYWORD_FLOAT64, "expected float64");
    } else if (check(TokenKind::KEYWORD_BOOL)) {
        typeToken = consume(TokenKind::KEYWORD_BOOL, "expected bool");
    } else if (check(TokenKind::KEYWORD_STRING)) {
        typeToken = consume(TokenKind::KEYWORD_STRING, "expected string");
    } else if (check(TokenKind::KEYWORD_VOID)) {
        typeToken = consume(TokenKind::KEYWORD_VOID, "expected void");
    } else if (check(TokenKind::KEYWORD_STRUCT)) {
        consume(TokenKind::KEYWORD_STRUCT, "expected struct");
        Token structName = consume(TokenKind::IDENT, "expected struct name");
        std::string type = "struct " + structName.lexeme;
        // Parse array suffixes
        while (match(TokenKind::LBRACKET)) {
            Token size = consume(TokenKind::INT_LIT, "expected array size");
            consume(TokenKind::RBRACKET, "expected ']'");
            type += "[" + size.lexeme + "]";
        }
        return type;
    } else {
        error(current.line, current.col, "expected type");
        return "";
    }

    std::string type = typeToken.lexeme;

    // Parse array suffixes int32[3][4]
    while (match(TokenKind::LBRACKET)) {
        Token size = consume(TokenKind::INT_LIT, "expected array size");
        consume(TokenKind::RBRACKET, "expected ']'");
        type += "[" + size.lexeme + "]";
    }

    return type;
}

std::unique_ptr<ASTNode> Parser::parseStatement() {
    if (check(TokenKind::KEYWORD_IF)) return parseIfStatement();
    if (check(TokenKind::KEYWORD_WHILE)) return parseWhileStatement();
    if (check(TokenKind::KEYWORD_FOR)) return parseForStatement();
    if (check(TokenKind::KEYWORD_SWITCH)) return parseSwitchStatement();
    if (check(TokenKind::KEYWORD_RETURN)) return parseReturnStatement();
    if (check(TokenKind::KEYWORD_BREAK)) return parseBreakStatement();
    if (check(TokenKind::KEYWORD_CONTINUE)) return parseContinueStatement();
    if (check(TokenKind::LBRACE)) return parseBlock();

    // Assignment or expression statement
    auto expr = parseExpr();
    if (match(TokenKind::ASSIGN)) {
        auto assign = std::make_unique<AssignExpr>();
        assign->target = std::move(expr);
        assign->value = parseExpr();
        consume(TokenKind::SEMICOLON, "expected ';'");
        return assign;
    }
    consume(TokenKind::SEMICOLON, "expected ';'");
    return expr;
}

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    consume(TokenKind::LBRACE, "expected '{'");
    auto block = std::make_unique<BlockStmt>();

    while (!check(TokenKind::RBRACE) && !check(TokenKind::END_OF_FILE)) {
        // block_item = var_decl ";" | statement
        if (check(TokenKind::KEYWORD_INT32) || check(TokenKind::KEYWORD_FLOAT64) ||
            check(TokenKind::KEYWORD_BOOL) || check(TokenKind::KEYWORD_STRING) ||
            check(TokenKind::KEYWORD_STRUCT)) {
            block->items.push_back(parseVarDecl());
        } else {
            block->items.push_back(parseStatement());
        }
    }

    consume(TokenKind::RBRACE, "expected '}'");
    return block;
}

std::unique_ptr<IfStmt> Parser::parseIfStatement() {
    consume(TokenKind::KEYWORD_IF, "expected 'if'");
    consume(TokenKind::LPAREN, "expected '('");
    auto condition = parseExpr();
    consume(TokenKind::RPAREN, "expected ')'");

    auto thenBranch = parseStatement();

    std::unique_ptr<ASTNode> elseBranch;
    if (match(TokenKind::KEYWORD_ELSE)) {
        elseBranch = parseStatement();
    }

    auto node = std::make_unique<IfStmt>();
    node->condition = std::move(condition);
    node->thenBranch = std::move(thenBranch);
    node->elseBranch = std::move(elseBranch);
    return node;
}

std::unique_ptr<WhileStmt> Parser::parseWhileStatement() {
    consume(TokenKind::KEYWORD_WHILE, "expected 'while'");
    consume(TokenKind::LPAREN, "expected '('");
    auto condition = parseExpr();
    consume(TokenKind::RPAREN, "expected ')'");

    auto node = std::make_unique<WhileStmt>();
    node->condition = std::move(condition);
    node->body = parseStatement();
    return node;
}

std::unique_ptr<ForStmt> Parser::parseForStatement() {
    consume(TokenKind::KEYWORD_FOR, "expected 'for'");
    consume(TokenKind::LPAREN, "expected '('");

    auto node = std::make_unique<ForStmt>();

    // init - can be assignment or empty
    if (!check(TokenKind::SEMICOLON)) {
        auto target = parseExpr();
        consume(TokenKind::ASSIGN, "expected '='");
        auto value = parseExpr();
        auto assign = std::make_unique<AssignExpr>();
        assign->target = std::move(target);
        assign->value = std::move(value);
        node->init = std::move(assign);
    }
    consume(TokenKind::SEMICOLON, "expected ';'");

    // condition - can be empty
    if (!check(TokenKind::SEMICOLON)) {
        node->cond = parseExpr();
    }
    consume(TokenKind::SEMICOLON, "expected ';'");

    // update - can be empty
    if (!check(TokenKind::RPAREN)) {
        auto target = parseExpr();
        consume(TokenKind::ASSIGN, "expected '='");
        auto value = parseExpr();
        auto assign = std::make_unique<AssignExpr>();
        assign->target = std::move(target);
        assign->value = std::move(value);
        node->update = std::move(assign);
    }
    consume(TokenKind::RPAREN, "expected ')'");

    node->body = parseStatement();
    return node;
}

std::unique_ptr<SwitchStmt> Parser::parseSwitchStatement() {
    consume(TokenKind::KEYWORD_SWITCH, "expected 'switch'");
    consume(TokenKind::LPAREN, "expected '('");
    auto node = std::make_unique<SwitchStmt>();
    node->expr = parseExpr();
    consume(TokenKind::RPAREN, "expected ')'");
    consume(TokenKind::LBRACE, "expected '{'");

    while (check(TokenKind::KEYWORD_CASE) && !check(TokenKind::END_OF_FILE)) {
        advance(); // consume 'case'
        auto label = parseCaseLabel();
        consume(TokenKind::COLON, "expected ':'");

        auto caseBody = std::make_unique<BlockStmt>();
        while (!check(TokenKind::KEYWORD_CASE) && !check(TokenKind::KEYWORD_DEFAULT) && !check(TokenKind::RBRACE)) {
            caseBody->items.push_back(parseStatement());
        }
        // parse 'break ;'
        if (check(TokenKind::KEYWORD_BREAK)) {
            advance();
            consume(TokenKind::SEMICOLON, "expected ';'");
        }

        node->cases.emplace_back(std::move(label), std::move(caseBody->items));
    }

    if (match(TokenKind::KEYWORD_DEFAULT)) {
        consume(TokenKind::COLON, "expected ':'");
        while (!check(TokenKind::RBRACE)) {
            node->defaultBody.push_back(parseStatement());
        }
        if (check(TokenKind::KEYWORD_BREAK)) {
            advance();
            consume(TokenKind::SEMICOLON, "expected ';'");
        }
    }

    consume(TokenKind::RBRACE, "expected '}'");
    return node;
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStatement() {
    consume(TokenKind::KEYWORD_RETURN, "expected 'return'");
    auto node = std::make_unique<ReturnStmt>();

    if (!check(TokenKind::SEMICOLON)) {
        node->value = parseExpr();
    }
    consume(TokenKind::SEMICOLON, "expected ';'");
    return node;
}

std::unique_ptr<BreakStmt> Parser::parseBreakStatement() {
    consume(TokenKind::KEYWORD_BREAK, "expected 'break'");
    consume(TokenKind::SEMICOLON, "expected ';'");
    return std::make_unique<BreakStmt>();
}

std::unique_ptr<ContinueStmt> Parser::parseContinueStatement() {
    consume(TokenKind::KEYWORD_CONTINUE, "expected 'continue'");
    consume(TokenKind::SEMICOLON, "expected ';'");
    return std::make_unique<ContinueStmt>();
}

std::unique_ptr<ASTNode> Parser::parseExpr() {
    return parseOrExpr();
}

std::unique_ptr<ASTNode> Parser::parseOrExpr() {
    auto left = parseAndExpr();
    while (check(TokenKind::OR)) {
        int line = current.line;
        int col = current.col;
        advance();
        auto node = std::make_unique<BinaryExpr>();
        node->op = "||";
        node->line = line;
        node->col = col;
        node->left = std::move(left);
        node->right = parseAndExpr();
        left = std::move(node);
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseAndExpr() {
    auto left = parseEqualityExpr();
    while (check(TokenKind::AND)) {
        int line = current.line;
        int col = current.col;
        advance();
        auto node = std::make_unique<BinaryExpr>();
        node->op = "&&";
        node->line = line;
        node->col = col;
        node->left = std::move(left);
        node->right = parseEqualityExpr();
        left = std::move(node);
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseEqualityExpr() {
    auto left = parseRelationalExpr();
    while (true) {
        if (check(TokenKind::EQ)) {
            int line = current.line;
            int col = current.col;
            advance();
            auto node = std::make_unique<BinaryExpr>();
            node->op = "==";
            node->line = line;
            node->col = col;
            node->left = std::move(left);
            node->right = parseRelationalExpr();
            left = std::move(node);
        } else if (check(TokenKind::NE)) {
            int line = current.line;
            int col = current.col;
            advance();
            auto node = std::make_unique<BinaryExpr>();
            node->op = "!=";
            node->line = line;
            node->col = col;
            node->left = std::move(left);
            node->right = parseRelationalExpr();
            left = std::move(node);
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseRelationalExpr() {
    auto left = parseAdditiveExpr();
    while (true) {
        if (check(TokenKind::LT)) {
            int line = current.line;
            int col = current.col;
            advance();
            auto node = std::make_unique<BinaryExpr>();
            node->op = "<";
            node->line = line;
            node->col = col;
            node->left = std::move(left);
            node->right = parseAdditiveExpr();
            left = std::move(node);
        } else if (check(TokenKind::LE)) {
            int line = current.line;
            int col = current.col;
            advance();
            auto node = std::make_unique<BinaryExpr>();
            node->op = "<=";
            node->line = line;
            node->col = col;
            node->left = std::move(left);
            node->right = parseAdditiveExpr();
            left = std::move(node);
        } else if (check(TokenKind::GT)) {
            int line = current.line;
            int col = current.col;
            advance();
            auto node = std::make_unique<BinaryExpr>();
            node->op = ">";
            node->line = line;
            node->col = col;
            node->left = std::move(left);
            node->right = parseAdditiveExpr();
            left = std::move(node);
        } else if (check(TokenKind::GE)) {
            int line = current.line;
            int col = current.col;
            advance();
            auto node = std::make_unique<BinaryExpr>();
            node->op = ">=";
            node->line = line;
            node->col = col;
            node->left = std::move(left);
            node->right = parseAdditiveExpr();
            left = std::move(node);
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseAdditiveExpr() {
    auto left = parseMultiplicativeExpr();
    while (true) {
        if (check(TokenKind::PLUS)) {
            int line = current.line;
            int col = current.col;
            advance();
            auto node = std::make_unique<BinaryExpr>();
            node->op = "+";
            node->line = line;
            node->col = col;
            node->left = std::move(left);
            node->right = parseMultiplicativeExpr();
            left = std::move(node);
        } else if (check(TokenKind::MINUS)) {
            int line = current.line;
            int col = current.col;
            advance();
            auto node = std::make_unique<BinaryExpr>();
            node->op = "-";
            node->line = line;
            node->col = col;
            node->left = std::move(left);
            node->right = parseMultiplicativeExpr();
            left = std::move(node);
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseMultiplicativeExpr() {
    auto left = parseUnaryExpr();
    while (true) {
        if (check(TokenKind::STAR)) {
            int line = current.line;
            int col = current.col;
            advance();
            auto node = std::make_unique<BinaryExpr>();
            node->op = "*";
            node->line = line;
            node->col = col;
            node->left = std::move(left);
            node->right = parseUnaryExpr();
            left = std::move(node);
        } else if (check(TokenKind::SLASH)) {
            int line = current.line;
            int col = current.col;
            advance();
            auto node = std::make_unique<BinaryExpr>();
            node->op = "/";
            node->line = line;
            node->col = col;
            node->left = std::move(left);
            node->right = parseUnaryExpr();
            left = std::move(node);
        } else if (check(TokenKind::PERCENT)) {
            int line = current.line;
            int col = current.col;
            advance();
            auto node = std::make_unique<BinaryExpr>();
            node->op = "%";
            node->line = line;
            node->col = col;
            node->left = std::move(left);
            node->right = parseUnaryExpr();
            left = std::move(node);
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseUnaryExpr() {
    if (check(TokenKind::NOT)) {
        int line = current.line;
        int col = current.col;
        advance();
        auto node = std::make_unique<UnaryExpr>();
        node->op = "!";
        node->line = line;
        node->col = col;
        node->operand = parseUnaryExpr();
        return node;
    }
    if (check(TokenKind::MINUS)) {
        int line = current.line;
        int col = current.col;
        advance();
        auto node = std::make_unique<UnaryExpr>();
        node->op = "-";
        node->line = line;
        node->col = col;
        node->operand = parseUnaryExpr();
        return node;
    }
    if (match(TokenKind::LPAREN)) {
        // Cast expression: (type) expr
        std::string targetType = parseType();
        consume(TokenKind::RPAREN, "expected ')'");
        auto node = std::make_unique<CastExpr>();
        node->targetType = targetType;
        node->operand = parseUnaryExpr();
        return node;
    }
    return parsePostfixExpr();
}

std::unique_ptr<ASTNode> Parser::parsePostfixExpr() {
    auto expr = parsePrimary();

    while (true) {
        if (match(TokenKind::LBRACKET)) {
            // Array index arr[i]
            auto node = std::make_unique<IndexExpr>();
            node->base = std::move(expr);
            node->index = parseExpr();
            consume(TokenKind::RBRACKET, "expected ']'");
            expr = std::move(node);
        } else if (match(TokenKind::DOT)) {
            // Member access obj.field
            auto node = std::make_unique<MemberExpr>();
            node->object = std::move(expr);
            Token memberName = consume(TokenKind::IDENT, "expected member name");
            node->member = memberName.lexeme;
            expr = std::move(node);
        } else if (match(TokenKind::LPAREN)) {
            // Function call func(args)
            auto call = std::make_unique<CallExpr>();
            if (auto* ident = dynamic_cast<IdentExpr*>(expr.get())) {
                call->callee = ident->name;
            } else {
                error(current.line, current.col, "expected function name");
            }
            // Parse args
            if (!check(TokenKind::RPAREN)) {
                do {
                    call->args.push_back(parseExpr());
                } while (match(TokenKind::COMMA));
            }
            consume(TokenKind::RPAREN, "expected ')'");
            expr = std::move(call);
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<ASTNode> Parser::parsePrimary() {
    if (check(TokenKind::INT_LIT)) {
        auto node = std::make_unique<LiteralExpr>();
        node->literal = consume(TokenKind::INT_LIT, "expected integer literal");
        return node;
    }
    if (check(TokenKind::FLOAT_LIT)) {
        auto node = std::make_unique<LiteralExpr>();
        node->literal = consume(TokenKind::FLOAT_LIT, "expected float literal");
        return node;
    }
    if (check(TokenKind::KEYWORD_TRUE) || check(TokenKind::KEYWORD_FALSE)) {
        auto node = std::make_unique<LiteralExpr>();
        node->literal = consume(check(TokenKind::KEYWORD_TRUE) ? TokenKind::KEYWORD_TRUE : TokenKind::KEYWORD_FALSE, "expected boolean literal");
        return node;
    }
    if (check(TokenKind::STRING_LIT)) {
        auto node = std::make_unique<LiteralExpr>();
        node->literal = consume(TokenKind::STRING_LIT, "expected string literal");
        return node;
    }
    if (check(TokenKind::IDENT)) {
        auto node = std::make_unique<IdentExpr>();
        node->name = consume(TokenKind::IDENT, "expected identifier").lexeme;
        return node;
    }
    if (check(TokenKind::LPAREN)) {
        advance();
        auto expr = parseExpr();
        consume(TokenKind::RPAREN, "expected ')'");
        return expr;
    }

    error(current.line, current.col, "unexpected token: " + current.lexeme);
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parseCaseLabel() {
    bool negative = false;
    if (match(TokenKind::MINUS)) {
        negative = true;
    }

    if (!check(TokenKind::INT_LIT)) {
        error(current.line, current.col, "expected integer literal in case label");
    }
    Token num = current;
    advance();
    int value = std::stoi(num.lexeme);
    if (negative) value = -value;

    std::unique_ptr<ASTNode> literal = std::make_unique<LiteralExpr>();
    static_cast<LiteralExpr*>(literal.get())->literal = Token(TokenKind::INT_LIT, std::to_string(value), num.line, num.col);

    while (match(TokenKind::PLUS) || match(TokenKind::MINUS)) {
        Token op = current;
        bool neg2 = false;
        if (op.kind == TokenKind::MINUS) neg2 = true;
        if (match(TokenKind::MINUS)) {} // consume minus
        else match(TokenKind::PLUS); // consume plus

        if (!check(TokenKind::INT_LIT)) {
            error(current.line, current.col, "expected integer literal");
        }
        Token num2 = current;
        advance();
        int val2 = std::stoi(num2.lexeme);
        if (neg2) val2 = -val2;

        auto binOp = std::make_unique<BinaryExpr>();
        binOp->op = (op.kind == TokenKind::PLUS) ? "+" : "-";
        binOp->left = std::move(literal);
        auto rhs = std::make_unique<LiteralExpr>();
        rhs->literal = Token(TokenKind::INT_LIT, std::to_string(val2), num2.line, num2.col);
        binOp->right = std::move(rhs);
        literal = std::move(binOp);
    }
    return literal;
}

std::unique_ptr<ASTNode> Parser::parseInitializer() {
    if (check(TokenKind::LBRACE)) {
        // Aggregate initialization {1, 2, 3}
        return parseBlock();
    }
    return parseExpr();
}