#include "Lexer.h"
#include "Diagnostics.h"
#include <cctype>

Lexer::Lexer(const std::string& source) : source(source) {}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[pos];
}

char Lexer::peekNext() const {
    if (pos + 1 >= source.size()) return '\0';
    return source[pos + 1];
}

Token Lexer::peekToken() {
    // Save state
    size_t savedPos = pos;
    int savedLine = line;
    int savedCol = col;

    // Get next token
    Token token = nextToken();

    // Restore state
    pos = savedPos;
    line = savedLine;
    col = savedCol;

    return token;
}

char Lexer::advance() {
    char c = source[pos++];
    if (c == '\n') {
        line++;
        col = 1;
    } else {
        col++;
    }
    return c;
}

bool Lexer::isAtEnd() const {
    return pos >= source.size();
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

Token Lexer::nextToken() {
    skipWhitespace();

    if (isAtEnd()) {
        return makeToken(TokenKind::END_OF_FILE, "");
    }

    char c = advance();

    // Single-character tokens
    switch (c) {
        case '(': return makeToken(TokenKind::LPAREN, "(");
        case ')': return makeToken(TokenKind::RPAREN, ")");
        case '{': return makeToken(TokenKind::LBRACE, "{");
        case '}': return makeToken(TokenKind::RBRACE, "}");
        case '[': return makeToken(TokenKind::LBRACKET, "[");
        case ']': return makeToken(TokenKind::RBRACKET, "]");
        case ',': return makeToken(TokenKind::COMMA, ",");
        case ';': return makeToken(TokenKind::SEMICOLON, ";");
        case ':': return makeToken(TokenKind::COLON, ":");
        case '.': return makeToken(TokenKind::DOT, ".");
        case '+': return makeToken(TokenKind::PLUS, "+");
        case '-': return makeToken(TokenKind::MINUS, "-");
        case '*': return makeToken(TokenKind::STAR, "*");
        case '/':
            if (peek() == '*') {
                advance();  // consume '*'
                skipComment();
                return nextToken();
            }
            return makeToken(TokenKind::SLASH, "/");
        case '%': return makeToken(TokenKind::PERCENT, "%");
    }

    // Two-character tokens
    if (c == '=' && peek() == '=') {
        advance();
        return makeToken(TokenKind::EQ, "==");
    }
    if (c == '!' && peek() == '=') {
        advance();
        return makeToken(TokenKind::NE, "!=");
    }
    if (c == '<' && peek() == '=') {
        advance();
        return makeToken(TokenKind::LE, "<=");
    }
    if (c == '>' && peek() == '=') {
        advance();
        return makeToken(TokenKind::GE, ">=");
    }
    if (c == '&' && peek() == '&') {
        advance();
        return makeToken(TokenKind::AND, "&&");
    }
    if (c == '|' && peek() == '|') {
        advance();
        return makeToken(TokenKind::OR, "||");
    }

    // Single '='
    if (c == '=') {
        return makeToken(TokenKind::ASSIGN, "=");
    }

    // Single '<' and '>'
    if (c == '<') return makeToken(TokenKind::LT, "<");
    if (c == '>') return makeToken(TokenKind::GT, ">");

    // Single '!'
    if (c == '!') return makeToken(TokenKind::NOT, "!");

    // String literal
    if (c == '"') {
        return scanString();
    }

    // Number
    if (isDigit(c)) {
        return scanNumber(c);
    }

    // Identifier or keyword
    if (isAlpha(c)) {
        return scanIdentifierOrKeyword(c);
    }

    return errorToken("unexpected character: " + std::string(1, c));
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
            advance();
        } else if (c == '/' && peekNext() == '*') {
            advance();  // consume '/'
            skipComment();  // handles /** ... **/
        } else if (c == '{' && peekNext() == '*') {
            advance();  // consume '{'
            skipComment();  // handles {** ... **}
        } else {
            break;
        }
    }
}

void Lexer::skipComment() {
    // Skip until **} (end of comment)
    // The opening / or { has been consumed, now at first '*'
    while (!isAtEnd()) {
        if (peek() == '*' && peekNext() == '*') {
            // Check if this is **} (end of comment)
            if (pos + 2 < source.size() && source[pos + 2] == '}') {
                advance();  // consume '*'
                advance();  // consume '*'
                advance();  // consume '}'
                return;
            }
        }
        advance();
    }
    // Unterminated comment
    error(line, col, "unterminated comment");
}

Token Lexer::scanString() {
    int startLine = line;
    int startCol = col - 1;  // column of opening quote
    std::string value;

    while (!isAtEnd() && peek() != '"') {
        char c = advance();
        if (c == '\\') {
            if (peek() == 'n') {
                advance();
                value += '\n';
            } else if (peek() == '\\') {
                advance();
                value += '\\';
            } else if (peek() == '"') {
                advance();
                value += '"';
            } else {
                error(line, col, "invalid escape sequence: \\" + std::string(1, peek()));
            }
        } else if (c == '\n') {
            error(line, col, "newline in string literal");
        } else if (c == '\0') {
            error(line, col, "string literal contains null character");
        } else {
            value += c;
        }
    }

    if (isAtEnd()) {
        error(startLine, startCol, "unterminated string literal");
    }

    advance();  // closing "
    return Token(TokenKind::STRING_LIT, value, startLine, startCol);
}

Token Lexer::scanNumber(char firstChar) {
    int startLine = line;
    int startCol = col - 1;
    std::string value;
    value += firstChar;

    // Integer part
    while (!isAtEnd() && isDigit(peek())) {
        value += advance();
    }

    // Check for matrix dimension literal (e.g., 2x3, 3x4)
    if (peek() == 'x' && isDigit(peekNext())) {
        value += advance();  // consume 'x'
        while (!isAtEnd() && isDigit(peek())) {
            value += advance();
        }
        return Token(TokenKind::MAT_DIM, value, startLine, startCol);
    }

    // Float part
    if (peek() == '.' && isDigit(peekNext())) {
        value += advance();  // '.'
        while (!isAtEnd() && isDigit(peek())) {
            value += advance();
        }
        return Token(TokenKind::FLOAT_LIT, value, startLine, startCol);
    }

    return Token(TokenKind::INT_LIT, value, startLine, startCol);
}

Token Lexer::scanIdentifierOrKeyword(char firstChar) {
    int startLine = line;
    int startCol = col - 1;
    std::string value;
    value += firstChar;

    while (!isAtEnd() && isAlphaNumeric(peek())) {
        value += advance();
    }

    // Check for keywords
    if (value == "bool") return makeToken(TokenKind::KEYWORD_BOOL, value);
    if (value == "break") return makeToken(TokenKind::KEYWORD_BREAK, value);
    if (value == "case") return makeToken(TokenKind::KEYWORD_CASE, value);
    if (value == "continue") return makeToken(TokenKind::KEYWORD_CONTINUE, value);
    if (value == "default") return makeToken(TokenKind::KEYWORD_DEFAULT, value);
    if (value == "else") return makeToken(TokenKind::KEYWORD_ELSE, value);
    if (value == "false") return makeToken(TokenKind::KEYWORD_FALSE, value);
    if (value == "float64") return makeToken(TokenKind::KEYWORD_FLOAT64, value);
    if (value == "for") return makeToken(TokenKind::KEYWORD_FOR, value);
    if (value == "if") return makeToken(TokenKind::KEYWORD_IF, value);
    if (value == "int32") return makeToken(TokenKind::KEYWORD_INT32, value);
    if (value == "return") return makeToken(TokenKind::KEYWORD_RETURN, value);
    if (value == "string") return makeToken(TokenKind::KEYWORD_STRING, value);
    if (value == "struct") return makeToken(TokenKind::KEYWORD_STRUCT, value);
    if (value == "switch") return makeToken(TokenKind::KEYWORD_SWITCH, value);
    if (value == "true") return makeToken(TokenKind::KEYWORD_TRUE, value);
    if (value == "void") return makeToken(TokenKind::KEYWORD_VOID, value);
    if (value == "while") return makeToken(TokenKind::KEYWORD_WHILE, value);
    if (value == "mat") return makeToken(TokenKind::KEYWORD_MAT, value);

    return Token(TokenKind::IDENT, value, startLine, startCol);
}

Token Lexer::makeToken(TokenKind kind, const std::string& lexeme) const {
    return Token(kind, lexeme, line, col);
}

Token Lexer::errorToken(const std::string& message) const {
    return Token(TokenKind::ERROR, message, line, col);
}