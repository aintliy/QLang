# QLang 测试套件

本目录包含 QLang 编译器的全部测试，分为**单元测试**（`unit/`）和**端到端测试**（`e2e/`）两类。单元测试和部分端到端测试由 CTest 自动运行。

## 目录结构

```
test/
├── unit/                        单元测试
│   ├── test_lexer.cpp           词法分析器测试
│   ├── test_lexer_errors.cpp    词法错误处理测试
│   ├── test_parser.cpp          语法分析器测试
│   ├── test_checker.cpp         语义分析器测试
│   └── CMakeLists.txt
└── e2e/                         端到端测试
    ├── test_*.ql                QLang 测试源程序
    ├── expected/                各测试的预期标准输出
    │   └── *.expected
    ├── run_e2e_test.sh          端到端测试运行脚本
    └── CMakeLists.txt
```

## 运行测试

运行全部测试：

```bash
bash init.sh test
```

或手动调用 CTest：

```bash
cd build && ctest --output-on-failure
```

## 单元测试（`unit/`）

使用 **Google Test** 框架，针对编译器各阶段独立测试。

| 测试文件 | 覆盖内容 |
|---------|---------|
| `test_lexer.cpp` | 关键字、标识符、整数、浮点、字符串、运算符、注释的 Token 化 |
| `test_lexer_errors.cpp` | 未闭合字符串、非法字符等错误处理 |
| `test_parser.cpp` | 各类语句和表达式的 AST 生成 |
| `test_checker.cpp` | 类型检查、隐式转换、控制流合法性 |

## 端到端测试（`e2e/`）

端到端测试验证完整的编译到执行流程：

```
.ql 源文件 → qlang 编译 → .ll → llc → .o → clang 链接 → 执行 → 比对输出
```

### 输出测试（由 CTest 自动运行）

| 测试文件 | 验证内容 |
|---------|---------|
| `test_fibonacci.ql` | 递归函数、整数运算 |
| `test_sum_array.ql` | 数组声明、下标访问、循环求和 |
| `test_struct_switch.ql` | 结构体字段访问、switch 语句 |
| `test_string.ql` | 字符串字面量与输出 |
| `test_global.ql` | 全局变量读写 |
| `test_literal_logical.ql` | 逻辑运算（&&、\|\|、!）|

### 异常终止测试（由 CTest 自动运行）

| 测试文件 | 验证内容 |
|---------|---------|
| `test_array_bounds.ql` | 数组越界运行时检查 |
| `test_divzero.ql` | 整数除零运行时检查 |
| `test_stack_overflow.ql` | 递归深度超限检查 |

### 其他测试文件

`test/e2e/` 目录下还有大量 `.ql` 文件（语义分析、代码生成、边界情况等），这些文件由开发者手动运行，不纳入 CTest 自动测试套件。如需验证某个文件，可手动执行：

```bash
bash init.sh run test/e2e/<文件名>.ql
```

部分可用测试文件：

| 文件 | 说明 |
|------|------|
| `test_comprehensive.ql` | 综合测试，覆盖所有主要功能 |
| `test_matrix_add.ql` | 矩阵加法 |
| `test_matrix_sub.ql` | 矩阵减法 |
| `test_matrix_scalar.ql` | 矩阵标量乘法 |
| `test_matrix_neg.ql` | 矩阵求负 |
| `test_matrix_mul.ql` | 矩阵乘法 |
| `test_matrix_index.ql` | 矩阵下标访问 |
| `test_matrix_func.ql` | 矩阵作为函数参数和返回值 |
| `test_matrix_print.ql` | 矩阵打印 |
| `test_matrix_index_bounds.ql` | 矩阵下标越界检查 |
| `test_float_arith.ql` | 浮点运算 |
| `test_string_matrix_interaction.ql` | 字符串与矩阵运算交互 |
| `test_shadow.ql` | 作用域与变量遮蔽 |

### 添加新的端到端测试

1. 在 `test/e2e/` 下创建 `test_<name>.ql`
2. 在 `test/e2e/expected/` 下创建 `test_<name>.expected`，写入预期的标准输出
3. 在 `test/e2e/CMakeLists.txt` 中用 `add_e2e_output_test`（正常退出）或 `add_e2e_abort_test`（异常终止）注册测试
4. 运行 `bash init.sh test` 验证
