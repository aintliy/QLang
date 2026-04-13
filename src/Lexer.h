#pragma once
#include "Token.h"
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(const std::string& source);

    Token nextToken();
    Token peekToken();  // Look at next token without consuming
    const std::string& source;
    size_t pos = 0;
    int line = 1;
    int col = 1;

    char peek() const;
    char peekNext() const;
    char advance();

    bool isAtEnd() const;
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;

    void skipWhitespace();
    void skipComment();

    Token makeToken(TokenKind kind, const std::string& lexeme) const;
    Token errorToken(const std::string& message) const;

    Token scanString();
    Token scanNumber(char firstChar);
    Token scanIdentifierOrKeyword(char firstChar);
};