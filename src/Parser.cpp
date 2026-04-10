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
    error(current.line, current.col, "parseTopLevelDecl not implemented");
    return nullptr;
}

std::unique_ptr<StructDefNode> Parser::parseStructDef() {
    error(current.line, current.col, "parseStructDef not implemented");
    return nullptr;
}

std::unique_ptr<FuncDefNode> Parser::parseFuncDef() {
    error(current.line, current.col, "parseFuncDef not implemented");
    return nullptr;
}

std::unique_ptr<VarDeclNode> Parser::parseVarDecl() {
    error(current.line, current.col, "parseVarDecl not implemented");
    return nullptr;
}

std::string Parser::parseType() {
    error(current.line, current.col, "parseType not implemented");
    return "";
}

std::unique_ptr<ASTNode> Parser::parseStatement() {
    error(current.line, current.col, "parseStatement not implemented");
    return nullptr;
}

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    error(current.line, current.col, "parseBlock not implemented");
    return nullptr;
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
    error(current.line, current.col, "parseInitializer not implemented");
    return nullptr;
}