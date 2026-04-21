#include <iostream>
#include <fstream>
#include <string>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include "Lexer.h"
#include "Parser.h"
#include "Checker.h"
#include "CodeGen.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.ql> [-O2] [-S -o output.ll]" << std::endl;
        return 1;
    }

    std::string inputPath = argv[1];

    // 读取源文件
    std::ifstream file(inputPath);
    if (!file) {
        std::cerr << "Cannot open file: " << inputPath << std::endl;
        return 1;
    }

    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    // 词法分析
    Lexer lexer(source);
    Parser parser(lexer);
    std::unique_ptr<ProgramNode> program = parser.parseProgram();

    // 语义分析
    Checker checker;
    checker.check(program.get());

    // 代码生成
    CodeGen codegen;
    std::unique_ptr<llvm::Module> module = codegen.codegen(program.get());

    // 输出 IR
    bool enableO2 = false;
    std::string outputPath = "output.ll";
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-O2") {
            enableO2 = true;
        } else if (std::string(argv[i]) == "-o" && i + 1 < argc) {
            outputPath = argv[i + 1];
        }
    }

    if (enableO2) {
        llvm::PassBuilder pb;
        llvm::LoopAnalysisManager lam;
        llvm::FunctionAnalysisManager fam;
        llvm::CGSCCAnalysisManager cgam;
        llvm::ModuleAnalysisManager mam;

        pb.registerModuleAnalyses(mam);
        pb.registerCGSCCAnalyses(cgam);
        pb.registerFunctionAnalyses(fam);
        pb.registerLoopAnalyses(lam);
        pb.crossRegisterProxies(lam, fam, cgam, mam);

        llvm::ModulePassManager mpm =
            pb.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);
        mpm.run(*module, mam);
    }

    std::error_code EC;
    llvm::raw_fd_ostream output(outputPath, EC);
    if (EC) {
        std::cerr << "Cannot open output file: " << outputPath << std::endl;
        return 1;
    }

    module->print(output, nullptr);
    std::cout << "Generated IR to " << outputPath << std::endl;

    return 0;
}
