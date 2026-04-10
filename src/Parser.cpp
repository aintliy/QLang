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
        typeToken = advance();
    } else if (check(TokenKind::KEYWORD_FLOAT64)) {
        typeToken = advance();
    } else if (check(TokenKind::KEYWORD_BOOL)) {
        typeToken = advance();
    } else if (check(TokenKind::KEYWORD_STRING)) {
        typeToken = advance();
    } else if (check(TokenKind::KEYWORD_VOID)) {
        typeToken = advance();
    } else if (check(TokenKind::KEYWORD_STRUCT)) {
        advance();
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
    error(current.line, current.col, "parseStatement not implemented");
    return nullptr;
}

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    consume(TokenKind::LBRACE, "expected '{'");
    auto block = std::make_unique<BlockStmt>();

    while (!check(TokenKind::RBRACE) && !check(TokenKind::END_OF_FILE)) {
        // block_item = var_decl ";" | statement
        if (check(TokenKind::KEYWORD_INT32) || check(TokenKind::KEYWORD_FLOAT64) ||
            check(TokenKind::KEYWORD_BOOL) || check(TokenKind::KEYWORD_STRING)) {
            block->items.push_back(parseVarDecl());
        } else {
            block->items.push_back(parseStatement());
        }
    }

    consume(TokenKind::RBRACE, "expected '}'");
    return block;
}

std::unique_ptr<IfStmt> Parser::parseIfStatement() {
    error(current.line, current.col, "parseIfStatement not implemented");
    return nullptr;
}

std::unique_ptr<WhileStmt> Parser::parseWhileStatement() {
    error(current.line, current.col, "parseWhileStatement not implemented");
    return nullptr;
}

std::unique_ptr<ForStmt> Parser::parseForStatement() {
    error(current.line, current.col, "parseForStatement not implemented");
    return nullptr;
}

std::unique_ptr<SwitchStmt> Parser::parseSwitchStatement() {
    error(current.line, current.col, "parseSwitchStatement not implemented");
    return nullptr;
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStatement() {
    error(current.line, current.col, "parseReturnStatement not implemented");
    return nullptr;
}

std::unique_ptr<BreakStmt> Parser::parseBreakStatement() {
    error(current.line, current.col, "parseBreakStatement not implemented");
    return nullptr;
}

std::unique_ptr<ContinueStmt> Parser::parseContinueStatement() {
    error(current.line, current.col, "parseContinueStatement not implemented");
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parseExpr() {
    error(current.line, current.col, "parseExpr not implemented");
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parseOrExpr() {
    error(current.line, current.col, "parseOrExpr not implemented");
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parseAndExpr() {
    error(current.line, current.col, "parseAndExpr not implemented");
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parseEqualityExpr() {
    error(current.line, current.col, "parseEqualityExpr not implemented");
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parseRelationalExpr() {
    error(current.line, current.col, "parseRelationalExpr not implemented");
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parseAdditiveExpr() {
    error(current.line, current.col, "parseAdditiveExpr not implemented");
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parseMultiplicativeExpr() {
    error(current.line, current.col, "parseMultiplicativeExpr not implemented");
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parseUnaryExpr() {
    error(current.line, current.col, "parseUnaryExpr not implemented");
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parsePostfixExpr() {
    error(current.line, current.col, "parsePostfixExpr not implemented");
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parsePrimary() {
    error(current.line, current.col, "parsePrimary not implemented");
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parseCaseLabel() {
    error(current.line, current.col, "parseCaseLabel not implemented");
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parseInitializer() {
    if (check(TokenKind::LBRACE)) {
        // Aggregate initialization {1, 2, 3}
        return parseBlock();
    }
    return parseExpr();
}