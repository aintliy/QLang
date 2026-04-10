#include <iostream>
#include <string>
#include <cassert>
#include "Lexer.h"
#include "Token.h"

void testKeywords() {
    std::string source = "bool break case continue default else false float64 for if int32 return string struct switch true void while";
    Lexer lexer(source);

    TokenKind expected[] = {
        TokenKind::KEYWORD_BOOL, TokenKind::KEYWORD_BREAK, TokenKind::KEYWORD_CASE,
        TokenKind::KEYWORD_CONTINUE, TokenKind::KEYWORD_DEFAULT, TokenKind::KEYWORD_ELSE,
        TokenKind::KEYWORD_FALSE, TokenKind::KEYWORD_FLOAT64, TokenKind::KEYWORD_FOR,
        TokenKind::KEYWORD_IF, TokenKind::KEYWORD_INT32, TokenKind::KEYWORD_RETURN,
        TokenKind::KEYWORD_STRING, TokenKind::KEYWORD_STRUCT, TokenKind::KEYWORD_SWITCH,
        TokenKind::KEYWORD_TRUE, TokenKind::KEYWORD_VOID, TokenKind::KEYWORD_WHILE
    };

    for (size_t i = 0; i < 18; i++) {
        Token token = lexer.nextToken();
        std::cout << "testKeywords: " << tokenKindToString(token.kind) << std::endl;
        assert(token.kind == expected[i]);
    }
}

void testIdentifiers() {
    std::string source = "foo bar _test myVar123";
    Lexer lexer(source);

    std::string expected[] = {"foo", "bar", "_test", "myVar123"};
    for (const auto& exp : expected) {
        Token token = lexer.nextToken();
        std::cout << "testIdentifiers: " << token.lexeme << std::endl;
        assert(token.kind == TokenKind::IDENT);
        assert(token.lexeme == exp);
    }
}

void testIntegers() {
    std::string source = "0 42 123 999";
    Lexer lexer(source);

    std::string expected[] = {"0", "42", "123", "999"};
    for (const auto& exp : expected) {
        Token token = lexer.nextToken();
        std::cout << "testIntegers: " << token.lexeme << std::endl;
        assert(token.kind == TokenKind::INT_LIT);
        assert(token.lexeme == exp);
    }
}

void testFloats() {
    std::string source = "0.5 3.14 2.0 0.123";
    Lexer lexer(source);

    std::string expected[] = {"0.5", "3.14", "2.0", "0.123"};
    for (const auto& exp : expected) {
        Token token = lexer.nextToken();
        std::cout << "testFloats: " << token.lexeme << std::endl;
        assert(token.kind == TokenKind::FLOAT_LIT);
        assert(token.lexeme == exp);
    }
}

void testStrings() {
    std::string source = "\"hello\" \"world\" \"\\n\" \"\\\\\" \"\\\"\"";
    Lexer lexer(source);

    std::string expected[] = {"hello", "world", "\n", "\\", "\""};
    for (const auto& exp : expected) {
        Token token = lexer.nextToken();
        std::cout << "testStrings: '" << token.lexeme << "'" << std::endl;
        assert(token.kind == TokenKind::STRING_LIT);
        assert(token.lexeme == exp);
    }
}

void testOperators() {
    std::string source = "+ - * / % ( ) { } [ ] , ; . == != < <= > >= && || ! =";
    Lexer lexer(source);

    TokenKind expected[] = {
        TokenKind::PLUS, TokenKind::MINUS, TokenKind::STAR, TokenKind::SLASH,
        TokenKind::PERCENT, TokenKind::LPAREN, TokenKind::RPAREN, TokenKind::LBRACE,
        TokenKind::RBRACE, TokenKind::LBRACKET, TokenKind::RBRACKET, TokenKind::COMMA,
        TokenKind::SEMICOLON, TokenKind::DOT, TokenKind::EQ, TokenKind::NE,
        TokenKind::LT, TokenKind::LE, TokenKind::GT, TokenKind::GE,
        TokenKind::AND, TokenKind::OR, TokenKind::NOT, TokenKind::ASSIGN
    };

    for (size_t i = 0; i < 24; i++) {
        Token token = lexer.nextToken();
        std::cout << "testOperators: " << tokenKindToString(token.kind) << std::endl;
        assert(token.kind == expected[i]);
    }
}

void testComments() {
    std::string source = "{** comment **}42{** multi\nline **}99";
    Lexer lexer(source);

    Token t1 = lexer.nextToken();
    assert(t1.kind == TokenKind::INT_LIT);
    assert(t1.lexeme == "42");

    Token t2 = lexer.nextToken();
    assert(t2.kind == TokenKind::INT_LIT);
    assert(t2.lexeme == "99");
}

int main() {
    testKeywords();
    testIdentifiers();
    testIntegers();
    testFloats();
    testStrings();
    testOperators();
    testComments();

    std::cout << "All lexer tests passed!" << std::endl;
    return 0;
}