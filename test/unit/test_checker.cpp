#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include "Parser.h"
#include "Lexer.h"
#include "AST.h"
#include "Checker.h"

std::unique_ptr<ProgramNode> parse(const std::string& src) {
    Lexer lexer(src);
    Parser parser(lexer);
    return parser.parseProgram();
}

// Run checker in child process and capture stderr
// Returns: true if checker exited with error (expected), false if no error
bool checkerShouldFail(const std::string& src, const std::string& expectedError) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process - run checker
        auto program = parse(src);
        Checker checker;
        checker.check(program.get());
        exit(0);  // Success - no error detected (but we expected one)
    } else {
        // Parent process - wait and check result
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 1) {
            return true;  // Checker exited with error as expected
        }
        return false;
    }
}

// Test: int32 to float64 implicit conversion is forbidden (via variable declaration)
void testInt32ToFloat64Forbidden() {
    std::string src = R"(
void test() {
    float64 x = 42;  {** ERROR: cannot convert 'int32' to 'float64' **}
}
void main() {}
)";
    bool gotError = checkerShouldFail(src, "cannot convert 'int32' to 'float64'");
    if (!gotError) {
        std::cout << "FAIL: testInt32ToFloat64Forbidden - expected error for int32->float64" << std::endl;
        exit(1);
    }
    std::cout << "PASS: testInt32ToFloat64Forbidden" << std::endl;
}

// Test: float64 to int32 implicit conversion is forbidden (via variable declaration)
void testFloat64ToInt32Forbidden() {
    std::string src = R"(
void test() {
    int32 x = 3.14;  {** ERROR: cannot convert 'float64' to 'int32' **}
}
void main() {}
)";
    bool gotError = checkerShouldFail(src, "cannot convert 'float64' to 'int32'");
    if (!gotError) {
        std::cout << "FAIL: testFloat64ToInt32Forbidden - expected error for float64->int32" << std::endl;
        exit(1);
    }
    std::cout << "PASS: testFloat64ToInt32Forbidden" << std::endl;
}

// Test: bool to int32 implicit conversion is forbidden (via variable declaration)
void testBoolToInt32Forbidden() {
    std::string src = R"(
void test() {
    int32 x = true;  {** ERROR: cannot convert 'bool' to 'int32' **}
}
void main() {}
)";
    bool gotError = checkerShouldFail(src, "cannot convert 'bool' to 'int32'");
    if (!gotError) {
        std::cout << "FAIL: testBoolToInt32Forbidden - expected error for bool->int32" << std::endl;
        exit(1);
    }
    std::cout << "PASS: testBoolToInt32Forbidden" << std::endl;
}

// Test: int32 to bool implicit conversion is forbidden (via variable declaration)
void testInt32ToBoolForbidden() {
    std::string src = R"(
void test() {
    bool x = 42;  {** ERROR: cannot convert 'int32' to 'bool' **}
}
void main() {}
)";
    bool gotError = checkerShouldFail(src, "cannot convert 'int32' to 'bool'");
    if (!gotError) {
        std::cout << "FAIL: testInt32ToBoolForbidden - expected error for int32->bool" << std::endl;
        exit(1);
    }
    std::cout << "PASS: testInt32ToBoolForbidden" << std::endl;
}

// Test: switch expression must be int32
void testSwitchExpressionMustBeInt32() {
    std::string src = R"(
void main() {
    float64 f;
    switch (f) {
        case 1: break;
    }
}
)";
    bool gotError = checkerShouldFail(src, "switch expression must be int32");
    if (!gotError) {
        std::cout << "FAIL: testSwitchExpressionMustBeInt32 - expected error for float64 switch" << std::endl;
        exit(1);
    }
    std::cout << "PASS: testSwitchExpressionMustBeInt32" << std::endl;
}

// Test: case labels must be constant integer expressions
void testCaseLabelMustBeConstantInt() {
    std::string src = R"(
int32 x;
void main() {
    switch (x) {
        case x: break;
    }
}
)";
    bool gotError = checkerShouldFail(src, "switch case label must be constant int expression");
    if (!gotError) {
        std::cout << "FAIL: testCaseLabelMustBeConstantInt - expected error for non-constant case" << std::endl;
        exit(1);
    }
    std::cout << "PASS: testCaseLabelMustBeConstantInt" << std::endl;
}

// Test: return type compatibility checking - float64 returned when int32 expected
void testReturnTypeMismatch() {
    std::string src = R"(
int32 test() {
    return 1.5;
}
void main() {}
)";
    bool gotError = checkerShouldFail(src, "cannot convert return type 'float64' to 'int32'");
    if (!gotError) {
        std::cout << "FAIL: testReturnTypeMismatch - expected error for return type mismatch" << std::endl;
        exit(1);
    }
    std::cout << "PASS: testReturnTypeMismatch" << std::endl;
}

// Test: valid int32 assignment should pass
void testValidInt32Assignment() {
    std::string src = R"(
void test() {
    int32 x;
    int32 y = 42;
}
void main() {}
)";
    // This should not fail
    auto program = parse(src);
    Checker checker;
    checker.check(program.get());
    std::cout << "PASS: testValidInt32Assignment" << std::endl;
}

// Test: valid switch with int32 should pass
void testValidInt32Switch() {
    std::string src = R"(
int32 x;
void main() {
    switch (x) {
        case 1: break;
        case 2: break;
        default: break;
    }
}
)";
    auto program = parse(src);
    Checker checker;
    checker.check(program.get());
    std::cout << "PASS: testValidInt32Switch" << std::endl;
}

int main() {
    // Tests that expect errors (run in subprocess)
    testInt32ToFloat64Forbidden();
    testFloat64ToInt32Forbidden();
    testBoolToInt32Forbidden();
    testInt32ToBoolForbidden();
    testSwitchExpressionMustBeInt32();
    testCaseLabelMustBeConstantInt();
    testReturnTypeMismatch();

    // Tests that should pass without errors
    testValidInt32Assignment();
    testValidInt32Switch();

    std::cout << "All checker tests passed!" << std::endl;
    return 0;
}