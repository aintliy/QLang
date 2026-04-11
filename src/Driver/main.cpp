#include <iostream>
#include <fstream>
#include <string>
#include "Lexer.h"
#include "Parser.h"
#include "Checker.h"

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

    // Lex
    Lexer lexer(source);

    // Parse
    Parser parser(lexer);
    auto ast = parser.parseProgram();

    // Semantic check
    Checker checker(std::move(ast));
    checker.check();  // Errors will be printed by Diagnostics::error

    return 0;
}