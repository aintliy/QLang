# QLang

QLang 是一门静态类型的编译型语言，编译器以 C++ 实现，后端基于 LLVM。编译器将 `.ql` 源文件编译为 LLVM IR（`.ll`），再经 `llc` 和 `clang` 生成原生可执行文件。

## 特性一览

- 静态类型系统，支持 `int32`、`float64`、`bool`、`string`、数组、结构体
- 完整控制流：`if/else`、`while`、`for`、`switch`
- 函数定义与递归调用
- 全局变量
- 运行时安全检查：数组越界、除零、递归深度限制
- C 运行时库提供 I/O 原语（`print_int`、`println_string` 等）

## 项目结构

```
QLang/
├── src/            编译器源码（词法、语法、语义、代码生成）
├── runtime/        QLang C 运行时库
├── test/           单元测试与端到端测试
├── build/          CMake 构建输出（不纳入版本控制）
├── CMakeLists.txt  根 CMake 配置
├── init.sh         构建 / 测试 / 运行脚本
```

## 快速开始

### 构建

```bash
bash init.sh build
```

### 运行一个 QLang 程序

```bash
bash init.sh run path/to/program.ql
```

### 运行测试套件

```bash
bash init.sh test
```

### 清理构建产物

```bash
bash init.sh clean
```

## 编译流程

```
.ql 源文件
   └─► Lexer（词法分析）→ Token 流
           └─► Parser（语法分析）→ AST
                   └─► Checker（语义分析）→ 类型检查后的 AST
                           └─► CodeGen（代码生成）→ LLVM IR（.ll）
                                   └─► llc → 目标文件（.o）
                                           └─► clang + libqlang_runtime.a → 可执行文件
```

## 依赖

| 工具 | 版本要求 |
|------|---------|
| CMake | ≥ 3.16 |
| LLVM / Clang | ≥ 14 |
| GCC / G++ | ≥ 11（支持 C++17） |
| Google Test | 用于单元测试 |

## 示例

```ql
{** 斐波那契数列 - 递归实现 **}

int32 fib(int32 n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

void main() {
    int32 result;
    result = fib(10);
    print_string("Fibonacci(10) = ");
    println_int(result);
}
```
