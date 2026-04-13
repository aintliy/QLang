#pragma once
#include <string>

enum class TokenKind {
    // Keywords
    KEYWORD_BOOL,
    KEYWORD_BREAK,
    KEYWORD_CASE,
    KEYWORD_CONTINUE,
    KEYWORD_DEFAULT,
    KEYWORD_ELSE,
    KEYWORD_FALSE,
    KEYWORD_FLOAT64,
    KEYWORD_FOR,
    KEYWORD_IF,
    KEYWORD_INT32,
    KEYWORD_RETURN,
    KEYWORD_STRING,
    KEYWORD_STRUCT,
    KEYWORD_SWITCH,
    KEYWORD_TRUE,
    KEYWORD_VOID,
    KEYWORD_WHILE,
    KEYWORD_MAT,    // 新增：mat 关键字

    // Identifiers
    IDENT,

    // Literals
    INT_LIT,
    FLOAT_LIT,
    STRING_LIT,
    MAT_DIM,        // 新增：矩阵维度字面量（如 2x3, 3x4）

    // Operators
    PLUS,       // +
    MINUS,      // -
    STAR,       // *
    SLASH,      // /
    PERCENT,    // %

    // Delimiters
    LPAREN,     // (
    RPAREN,     // )
    LBRACE,     // {
    RBRACE,     // }
    LBRACKET,   // [
    RBRACKET,   // ]
    COMMA,      // ,
    SEMICOLON,  // ;
    COLON,      // :
    DOT,        // .

    // Assignment
    ASSIGN,     // =

    // Comparison
    EQ,         // ==
    NE,         // !=
    LT,         // <
    LE,         // <=
    GT,         // >
    GE,         // >=

    // Logical
    AND,        // &&
    OR,         // ||
    NOT,        // !

    // Special
    END_OF_FILE,
    ERROR
};

struct Token {
    TokenKind kind;
    std::string lexeme;
    int line;
    int col;

    Token() : kind(TokenKind::END_OF_FILE), lexeme(""), line(0), col(0) {}
    Token(TokenKind kind, std::string lexeme, int line, int col)
        : kind(kind), lexeme(std::move(lexeme)), line(line), col(col) {}
};

const char* tokenKindToString(TokenKind kind);