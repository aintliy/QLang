# QLang 编译器源码

本目录包含 QLang 编译器的全部源码，按编译阶段拆分为独立的 CMake 库，最终由 `Driver` 组装成可执行文件 `qlang`。

## 目录结构

```
src/
├── Token.h / Token.cpp          词法单元定义
├── Lexer.h / Lexer.cpp          词法分析器
├── AST.h / AST.cpp              抽象语法树节点定义
├── Parser.h / Parser.cpp        递归下降语法分析器
├── Checker.h / Checker.cpp      两遍语义分析器
├── CodeGen.h / CodeGen.cpp      LLVM IR 代码生成器
├── Diagnostics.h / Diagnostics.cpp  错误报告
└── Driver/
    └── main.cpp                 编译器入口（可执行文件为 `build/src/qlang_driver`）
```

## 编译阶段

### 1. 词法分析（Lexer）

`Lexer` 逐字符扫描源文件，产生 `Token` 流。

- `TokenKind` 枚举涵盖关键字、标识符、字面量、运算符、分隔符、EOF、ERROR
- 每个 `Token` 携带 `kind`、`lexeme`、`line`、`col`
- 支持块注释（`{** ... **}`）、整数字面量、浮点字面量、字符串字面量（含转义序列）

### 2. 语法分析（Parser）

`Parser` 以递归下降方式将 Token 流解析为 AST。表达式部分使用优先级爬升（precedence climbing）处理二元运算符。

支持的 AST 节点类型：

| 类别 | 节点 |
|------|------|
| 顶层声明 | `ProgramNode`、`StructDefNode`、`FuncDefNode`、`VarDeclNode` |
| 语句 | `BlockStmt`、`IfStmt`、`WhileStmt`、`ForStmt`、`SwitchStmt`、`ReturnStmt`、`BreakStmt`、`ContinueStmt`、`ExprStmt` |
| 表达式 | `LiteralExpr`、`IdentExpr`、`BinaryExpr`、`UnaryExpr`、`CallExpr`、`IndexExpr`、`MemberExpr`、`CastExpr`、`AssignExpr`、`InitListExpr` |

### 3. 语义分析（Checker）

`Checker` 对 AST 做两遍分析：

- **第一遍**：收集所有结构体和函数定义，建立全局符号表
- **第二遍**：逐语句检查类型、隐式转换、控制流（`break`/`continue` 合法性、所有路径必须有 `return`）

附加限制：
- 数组大小不得超过 1 MB
- 递归深度不得超过 1024 层（运行时检查）

### 4. 代码生成（CodeGen）

`CodeGen` 使用 LLVM C++ API 将语义检查后的 AST 转换为 LLVM IR。

- 管理符号表（`alloca` 栈变量）
- 使用 `sret` 参数传递大型结构体返回值
- 生成运行时安全检查：数组越界、除零、栈深度溢出

### 5. 入口（Driver）

`Driver/main.cpp` 串联整个流水线：

```
源文件路径 → Lexer → Parser → Checker → CodeGen → 输出 .ll 文件
```

## CMake 依赖链

```
qlang_token
  └─ qlang_diagnostics
       └─ qlang_lexer
            └─ qlang_ast
                 └─ qlang_parser
                      └─ qlang_checker
                           └─ qlang_codegen
                                └─ qlang_driver  →  qlang（可执行文件）
```

## 构建

```bash
bash init.sh build
```

构建产物位于 `build/` 目录，可执行文件为 `build/src/Driver/qlang_driver`。
