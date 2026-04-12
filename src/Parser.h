#pragma once
#include <memory>
#include <string>
#include "Lexer.h"
#include "AST.h"

class Parser {
public:
    explicit Parser(Lexer& lexer);
    std::unique_ptr<ProgramNode> parseProgram();

private:
    Lexer& lexer;
    Token current;

    void advance();
    bool check(TokenKind kind) const;
    bool match(TokenKind kind);
    Token consume(TokenKind kind, const std::string& errorMsg);

    // Top-level
    std::unique_ptr<ASTNode> parseTopLevelDecl();

    // Declarations
    std::unique_ptr<StructDefNode> parseStructDef(Token* preConsumedName = nullptr);
    std::unique_ptr<FuncDefNode> parseFuncDef();
    std::unique_ptr<VarDeclNode> parseVarDecl();

    // Type
    std::string parseType();

    // Statements
    std::unique_ptr<ASTNode> parseStatement();
    std::unique_ptr<BlockStmt> parseBlock();
    std::unique_ptr<IfStmt> parseIfStatement();
    std::unique_ptr<WhileStmt> parseWhileStatement();
    std::unique_ptr<ForStmt> parseForStatement();
    std::unique_ptr<SwitchStmt> parseSwitchStatement();
    std::unique_ptr<ReturnStmt> parseReturnStatement();
    std::unique_ptr<BreakStmt> parseBreakStatement();
    std::unique_ptr<ContinueStmt> parseContinueStatement();

    // Expressions (precedence climbing)
    std::unique_ptr<ASTNode> parseExpr();
    std::unique_ptr<ASTNode> parseOrExpr();
    std::unique_ptr<ASTNode> parseAndExpr();
    std::unique_ptr<ASTNode> parseEqualityExpr();
    std::unique_ptr<ASTNode> parseRelationalExpr();
    std::unique_ptr<ASTNode> parseAdditiveExpr();
    std::unique_ptr<ASTNode> parseMultiplicativeExpr();
    std::unique_ptr<ASTNode> parseUnaryExpr();
    std::unique_ptr<ASTNode> parsePostfixExpr();
    std::unique_ptr<ASTNode> parsePrimary();

    // Case label
    std::unique_ptr<ASTNode> parseCaseLabel();

    // Helper
    std::unique_ptr<ASTNode> parseInitializer();
};