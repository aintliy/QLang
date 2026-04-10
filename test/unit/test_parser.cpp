#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include "Parser.h"
#include "Lexer.h"
#include "AST.h"
#include "Token.h"

void testStructDef() {
    std::string source = "struct Point { int32 x; int32 y; }";
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parseProgram();
    assert(program->declarations.size() == 1);
    auto structNode = dynamic_cast<StructDefNode*>(program->declarations[0].get());
    assert(structNode != nullptr);
    assert(structNode->name == "Point");
    assert(structNode->fields.size() == 2);
    std::cout << "PASS: testStructDef" << std::endl;
}

void testVarDecl() {
    std::string source = "int32 x;";
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parseProgram();
    assert(program->declarations.size() == 1);
    auto var = dynamic_cast<VarDeclNode*>(program->declarations[0].get());
    assert(var != nullptr);
    assert(var->name == "x");
    assert(var->type == "int32");
    std::cout << "PASS: testVarDecl" << std::endl;
}

void testFuncDef() {
    std::string source = "int32 main() {}";
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parseProgram();
    assert(program->declarations.size() == 1);
    auto func = dynamic_cast<FuncDefNode*>(program->declarations[0].get());
    assert(func != nullptr);
    assert(func->name == "main");
    assert(func->returnType == "int32");
    std::cout << "PASS: testFuncDef" << std::endl;
}

void testIntLiteral() {
    // Test int32 literal in variable declaration: int32 x = 42;
    std::string source = "int32 x = 42;";
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parseProgram();
    assert(program->declarations.size() == 1);
    auto var = dynamic_cast<VarDeclNode*>(program->declarations[0].get());
    assert(var != nullptr);
    auto lit = dynamic_cast<LiteralExpr*>(var->initializer.get());
    assert(lit != nullptr);
    std::cout << "PASS: testIntLiteral" << std::endl;
}

void testBinaryExpr() {
    // Test binary expression in variable declaration: int32 result = 1 + 2;
    std::string source = "int32 result = 1 + 2;";
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parseProgram();
    assert(program->declarations.size() == 1);
    auto var = dynamic_cast<VarDeclNode*>(program->declarations[0].get());
    assert(var != nullptr);
    auto bin = dynamic_cast<BinaryExpr*>(var->initializer.get());
    assert(bin != nullptr);
    assert(bin->op == "+");
    std::cout << "PASS: testBinaryExpr" << std::endl;
}

void testIfStatement() {
    // if statement inside function body
    std::string source = "void foo() { if (true) x = 1; }";
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parseProgram();
    assert(program->declarations.size() == 1);
    auto func = dynamic_cast<FuncDefNode*>(program->declarations[0].get());
    assert(func != nullptr);
    assert(func->body != nullptr);
    auto bodyBlock = dynamic_cast<BlockStmt*>(func->body.get());
    assert(bodyBlock != nullptr);
    assert(bodyBlock->items.size() == 1);
    auto ifStmt = dynamic_cast<IfStmt*>(bodyBlock->items[0].get());
    assert(ifStmt != nullptr);
    std::cout << "PASS: testIfStatement" << std::endl;
}

void testWhileStatement() {
    // while statement inside function body
    std::string source = "void foo() { while (true) { x = x + 1; } }";
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parseProgram();
    assert(program->declarations.size() == 1);
    auto func = dynamic_cast<FuncDefNode*>(program->declarations[0].get());
    assert(func != nullptr);
    assert(func->body != nullptr);
    auto bodyBlock = dynamic_cast<BlockStmt*>(func->body.get());
    assert(bodyBlock != nullptr);
    assert(bodyBlock->items.size() == 1);
    auto whileStmt = dynamic_cast<WhileStmt*>(bodyBlock->items[0].get());
    assert(whileStmt != nullptr);
    std::cout << "PASS: testWhileStatement" << std::endl;
}

void testForLoop() {
    // for loop inside function body
    std::string source = "void foo() { for (i = 0; i < 10; i = i + 1) { x = x + i; } }";
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parseProgram();
    assert(program->declarations.size() == 1);
    auto func = dynamic_cast<FuncDefNode*>(program->declarations[0].get());
    assert(func != nullptr);
    assert(func->body != nullptr);
    auto bodyBlock = dynamic_cast<BlockStmt*>(func->body.get());
    assert(bodyBlock != nullptr);
    assert(bodyBlock->items.size() == 1);
    auto forStmt = dynamic_cast<ForStmt*>(bodyBlock->items[0].get());
    assert(forStmt != nullptr);
    std::cout << "PASS: testForLoop" << std::endl;
}

int main() {
    testStructDef();
    testVarDecl();
    testFuncDef();
    testIntLiteral();
    testBinaryExpr();
    testIfStatement();
    testWhileStatement();
    testForLoop();
    std::cout << "All parser tests passed!" << std::endl;
    return 0;
}