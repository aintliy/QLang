# QLang 测试套件

本目录包含 QLang 编译器的全部测试，分为**单元测试**（`unit/`）和**端到端测试**（`e2e/`）两类。所有测试由 CTest 自动运行。

## 目录结构

```
test/
├── unit/                        单元测试
│   ├── test_lexer.cpp           词法分析器测试
│   ├── test_lexer_errors.cpp   词法错误处理测试
│   ├── test_parser.cpp         语法分析器测试
│   ├── test_checker.cpp        语义分析器测试
│   └── CMakeLists.txt
└── e2e/                         端到端测试
    ├── test_*.ql               QLang 测试源程序（50 个）
    ├── expected/                各测试的预期标准输出
    │   └── *.expected           （34 个输出测试的预期输出）
    ├── run_e2e_test.sh         测试运行脚本
    └── CMakeLists.txt           测试注册（50 个测试）
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

### 输出测试（34 个）

正常编译运行，验证标准输出是否与 `.expected` 文件一致：

| 测试文件 | 验证内容 |
|---------|---------|
| `test_fibonacci.ql` | 递归函数、整数运算 |
| `test_sum_array.ql` | 数组声明、下标访问、循环求和 |
| `test_struct_switch.ql` | 结构体字段访问、switch 语句 |
| `test_string.ql` | 字符串字面量与输出 |
| `test_global.ql` | 全局变量读写 |
| `test_literal_logical.ql` | 逻辑运算（&&、\|\|、!）|
| `test_codegen_control.ql` | if/else、while、for、break、continue |
| `test_codegen_for.ql` | for 循环 |
| `test_codegen_if.ql` | if-else 分支 |
| `test_codegen_return.ql` | 函数返回值 |
| `test_codegen_switch.ql` | switch 语句 |
| `test_codegen_while.ql` | while 循环 |
| `test_matrix_add.ql` | 矩阵加法 |
| `test_matrix_func.ql` | 矩阵作为函数参数和返回值 |
| `test_matrix_index.ql` | 矩阵下标访问 |
| `test_matrix_mul.ql` | 矩阵乘法 |
| `test_matrix_neg.ql` | 矩阵求负 |
| `test_matrix_print.ql` | 矩阵打印 |
| `test_matrix_scalar.ql` | 矩阵标量乘法 |
| `test_matrix_sub.ql` | 矩阵减法 |
| `test_matrix_semantic_valid.ql` | 合法的矩阵语义操作 |
| `test_array.ql` | 数组声明、下标访问、整体赋值 |
| `test_float_arith.ql` | 浮点运算 |
| `test_global_order.ql` | 全局变量声明顺序 |
| `test_shadow.ql` | 作用域与变量遮蔽 |
| `test_scope_block.ql` | 块作用域 |
| `test_memcpy.ql` | 数组整体赋值（memcpy 语义）|
| `test_continue_in_switch_loop.ql` | continue 在 switch 内的跳转 |
| `test_comprehensive.ql` | 综合测试，覆盖所有主要功能 |
| `test_struct.ql` | 结构体字段访问 |
| `test_struct_return.ql` | 结构体作为函数返回值 |
| `test_string_matrix_interaction.ql` | 字符串与矩阵运算交互 |
| `test_continue_in_switch_loop.ql` | continue 在 switch 内但外层有循环 |

### 异常终止测试（16 个）

期望编译失败或运行时 abort（退出码非零）：

| 测试文件 | 验证内容 |
|---------|---------|
| `test_array_bounds.ql` | 数组越界运行时检查 |
| `test_divzero.ql` | 整数除零运行时检查 |
| `test_stack_overflow.ql` | 递归深度超限检查 |
| `test_matrix_index_bounds.ql` | 矩阵下标越界运行时检查 |
| `test_nesting_depth_1025.ql` | 递归深度超限（1024 层）检查 |
| `test_unsupported_op_bitwise.ql` | 不支持的位运算符 `&` |
| `test_unsupported_op_minus_minus.ql` | 不支持的自增 `--` |
| `test_unsupported_op_plus_plus.ql` | 不支持的自增 `++` |
| `test_unsupported_op_unary_plus.ql` | 不支持的一元 `+` |
| `test_continue_outside.ql` | continue 在循环外（编译错误）|
| `test_break_outside.ql` | break 在循环外（编译错误）|
| `test_switch_case_no_break.ql` | switch case 未以 break 结尾（编译错误）|
| `test_no_return_path.ql` | 非 void 函数缺少 return（编译错误）|
| `test_struct_recursive.ql` | 递归结构体定义（编译错误）|
| `test_array_size_limit.ql` | 数组大小超限（编译错误）|
| `test_div_zero_literal.ql` | 字面量零除数（编译错误）|
| `test_semantic_switch.ql` | switch 表达式类型错误（编译错误）|
| `test_semantic_return.ql` | 返回类型不匹配（编译错误）|

## 添加新的端到端测试

### 1. 创建测试源文件

在 `test/e2e/` 下创建 `test_<name>.ql`：

```ql
void main() {
    println_int(42);
}
```

### 2. 确定测试类型

**输出测试**：程序正常退出，验证标准输出
- 在 `test/e2e/expected/` 下创建 `test_<name>.expected`，写入预期输出

**异常终止测试**：编译失败或运行时 abort
- 无需 expected 文件

### 3. 注册测试

在 `test/e2e/CMakeLists.txt` 中添加：

```cmake
# 输出测试（期望 exit 0）
add_e2e_output_test(test_<name>)

# 或异常终止测试（期望非零退出码）
add_e2e_abort_test(test_<name>)
```

### 4. 验证

```bash
bash init.sh build && bash init.sh test
```
