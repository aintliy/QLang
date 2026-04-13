#include <iostream>
#include <string>
#include <cassert>
#include "Lexer.h"
#include "Token.h"

void testMatKeyword() {
    std::cout << "Testing 'mat' keyword..." << std::endl;
    Lexer lexer("mat");
    Token token = lexer.nextToken();
    assert(token.kind == TokenKind::KEYWORD_MAT);
    assert(token.lexeme == "mat");
    std::cout << "PASS: mat keyword recognized" << std::endl;
}

void testMatrixDimensionLiterals() {
    std::cout << "Testing matrix dimension literals..." << std::endl;

    // Test various dimension formats
    struct TestCase {
        std::string input;
        std::string expectedLexeme;
    };

    TestCase tests[] = {
        {"2x3", "2x3"},
        {"3x4", "3x4"},
        {"1024x1024", "1024x1024"},
        {"1x1", "1x1"},
        {"10x20", "10x20"},
    };

    for (const auto& test : tests) {
        Lexer lexer(test.input);
        Token token = lexer.nextToken();
        std::cout << "  Testing '" << test.input << "' -> " << token.lexeme << std::endl;
        assert(token.kind == TokenKind::MAT_DIM);
        assert(token.lexeme == test.expectedLexeme);
    }
    std::cout << "PASS: all dimension literals recognized" << std::endl;
}

void testMatWithType() {
    std::cout << "Testing 'mat<int32> 2x3'..." << std::endl;
    std::string source = "mat<int32> 2x3 m;";
    Lexer lexer(source);

    Token t1 = lexer.nextToken();
    assert(t1.kind == TokenKind::KEYWORD_MAT);
    assert(t1.lexeme == "mat");

    Token t2 = lexer.nextToken();
    assert(t2.kind == TokenKind::LT);  // '<'

    Token t3 = lexer.nextToken();
    assert(t3.kind == TokenKind::KEYWORD_INT32);

    Token t4 = lexer.nextToken();
    assert(t4.kind == TokenKind::GT);  // '>'

    Token t5 = lexer.nextToken();
    assert(t5.kind == TokenKind::MAT_DIM);
    assert(t5.lexeme == "2x3");

    std::cout << "PASS: mat<int32> 2x3 parsed correctly" << std::endl;
}

void testOrdinaryNumbersUnaffected() {
    std::cout << "Testing ordinary numbers unaffected..." << std::endl;
    Lexer lexer("42 3.14 99");

    Token t1 = lexer.nextToken();
    assert(t1.kind == TokenKind::INT_LIT);
    assert(t1.lexeme == "42");

    Token t2 = lexer.nextToken();
    assert(t2.kind == TokenKind::FLOAT_LIT);
    assert(t2.lexeme == "3.14");

    Token t3 = lexer.nextToken();
    assert(t3.kind == TokenKind::INT_LIT);
    assert(t3.lexeme == "99");

    std::cout << "PASS: ordinary numbers work correctly" << std::endl;
}

int main() {
    testMatKeyword();
    testMatrixDimensionLiterals();
    testMatWithType();
    testOrdinaryNumbersUnaffected();

    std::cout << "\nAll matrix lexer tests passed!" << std::endl;
    return 0;
}