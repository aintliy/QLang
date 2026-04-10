#include <iostream>
#include <fstream>
#include <string>
#include "Lexer.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.ql>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Cannot open file: " << argv[1] << std::endl;
        return 1;
    }

    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    Lexer lexer(source);
    while (true) {
        Token token = lexer.nextToken();
        std::cout << "Token: " << tokenKindToString(token.kind)
                  << " \"" << token.lexeme << "\""
                  << " at " << token.line << ":" << token.col << std::endl;
        if (token.kind == TokenKind::EOF) break;
    }

    return 0;
}