#include <iostream>
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "Lexer.h"
#include "Token.h"

// Test cases where the lexer calls exit() on error
bool test_exit_case(const char* input, const char* test_name) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        Lexer lexer(input);
        lexer.nextToken();
        // If we get here, the lexer did not error
        std::cerr << "FAIL: " << test_name << " - should have errored" << std::endl;
        _exit(1);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 1) {
            std::cout << "PASS: " << test_name << " detected" << std::endl;
            return true;
        } else {
            std::cerr << "FAIL: " << test_name << " - unexpected exit status" << std::endl;
            return false;
        }
    }
}

// Test cases where the lexer returns an ERROR token
bool test_error_token_case(const char* input, const char* test_name) {
    Lexer lexer(input);
    Token token = lexer.nextToken();
    if (token.kind == TokenKind::ERROR) {
        std::cout << "PASS: " << test_name << " detected" << std::endl;
        return true;
    } else {
        std::cerr << "FAIL: " << test_name << " - should have returned ERROR token" << std::endl;
        return false;
    }
}

int main() {
    std::cout << "Testing lexer error handling..." << std::endl;

    bool all_passed = true;

    // These cases call exit() on error
    all_passed &= test_exit_case("\"\\q\"", "invalid escape sequence");
    all_passed &= test_exit_case("\"hello", "unterminated string");
    all_passed &= test_exit_case("{** comment", "unterminated comment");
    all_passed &= test_exit_case("\"hello\0world\"", "null character in string");

    // This case returns an ERROR token
    all_passed &= test_error_token_case("@", "invalid character");

    if (all_passed) {
        std::cout << "All error tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << "Some tests failed!" << std::endl;
        return 1;
    }
}