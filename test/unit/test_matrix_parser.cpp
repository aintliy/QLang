#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include "Parser.h"
#include "Lexer.h"
#include "AST.h"

void testMatrixTypeDeclaration() {
    std::cout << "Testing matrix type declaration..." << std::endl;
    std::string source = "mat<int32> 2x3 m;";
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parseProgram();
    assert(program->declarations.size() == 1);
    auto var = dynamic_cast<VarDeclNode*>(program->declarations[0].get());
    assert(var != nullptr);
    assert(var->type == "mat<int32> 2x3");
    assert(var->name == "m");
    std::cout << "PASS: testMatrixTypeDeclaration" << std::endl;
}

void testMatrixFloat64Type() {
    std::cout << "Testing matrix float64 type..." << std::endl;
    std::string source = "mat<float64> 3x4 m;";
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parseProgram();
    auto var = dynamic_cast<VarDeclNode*>(program->declarations[0].get());
    assert(var != nullptr);
    assert(var->type == "mat<float64> 3x4");
    std::cout << "PASS: testMatrixFloat64Type" << std::endl;
}

void testMatrixWithInitializer() {
    std::cout << "Testing matrix with initializer..." << std::endl;
    std::string source = "mat<int32> 2x2 m = {{1, 2}, {3, 4}};";
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parseProgram();
    auto var = dynamic_cast<VarDeclNode*>(program->declarations[0].get());
    assert(var != nullptr);
    assert(var->type == "mat<int32> 2x2");
    assert(var->initializer != nullptr);
    // initializer 是嵌套的 InitListExpr
    auto initList = dynamic_cast<InitListExpr*>(var->initializer.get());
    assert(initList != nullptr);
    assert(initList->elements.size() == 2); // 2 rows
    std::cout << "PASS: testMatrixWithInitializer" << std::endl;
}

void testMatrixLocalVariable() {
    std::cout << "Testing matrix local variable in block..." << std::endl;
    std::string source = "void main() { mat<int32> 2x3 m; }";
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parseProgram();
    auto func = dynamic_cast<FuncDefNode*>(program->declarations[0].get());
    assert(func != nullptr);
    auto block = dynamic_cast<BlockStmt*>(func->body.get());
    assert(block != nullptr);
    assert(block->items.size() == 1);
    auto var = dynamic_cast<VarDeclNode*>(block->items[0].get());
    assert(var != nullptr);
    assert(var->type == "mat<int32> 2x3");
    std::cout << "PASS: testMatrixLocalVariable" << std::endl;
}

void testMatrixFunctionReturnType() {
    std::cout << "Testing matrix as function return type..." << std::endl;
    std::string source = "mat<int32> 2x2 identity() { mat<int32> 2x2 m; return m; }";
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parseProgram();
    auto func = dynamic_cast<FuncDefNode*>(program->declarations[0].get());
    assert(func != nullptr);
    assert(func->returnType == "mat<int32> 2x2");
    std::cout << "PASS: testMatrixFunctionReturnType" << std::endl;
}

void testMatrixFunctionParameter() {
    std::cout << "Testing matrix as function parameter..." << std::endl;
    std::string source = "int32 trace(mat<int32> 2x2 m) { return m[0][0] + m[1][1]; }";
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parseProgram();
    auto func = dynamic_cast<FuncDefNode*>(program->declarations[0].get());
    assert(func != nullptr);
    assert(func->params.size() == 1);
    assert(func->params[0].first == "mat<int32> 2x2");
    std::cout << "PASS: testMatrixFunctionParameter" << std::endl;
}

int main() {
    testMatrixTypeDeclaration();
    testMatrixFloat64Type();
    testMatrixWithInitializer();
    testMatrixLocalVariable();
    testMatrixFunctionReturnType();
    testMatrixFunctionParameter();
    std::cout << "\nAll matrix parser tests passed!" << std::endl;
    return 0;
}
