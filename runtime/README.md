# QLang Runtime

QLang 运行时库（`libqlang_runtime.a`）是一个静态 C 库，为编译后的 QLang 程序提供 I/O 原语和字符串支持。

## 文件

| 文件 | 说明 |
|------|------|
| `qlang_runtime.c` | 运行时函数实现 |
| `CMakeLists.txt` | 构建 `libqlang_runtime.a` |

## 提供的函数

### 整数 I/O

| 函数签名 | 说明 |
|---------|------|
| `void print_int(int32_t v)` | 输出整数，不换行 |
| `void println_int(int32_t v)` | 输出整数并换行 |
| `int32_t read_int()` | 从标准输入读取一个整数 |

### 浮点 I/O

| 函数签名 | 说明 |
|---------|------|
| `void print_float64(double v)` | 输出浮点数，不换行 |
| `void println_float64(double v)` | 输出浮点数并换行 |
| `double read_float64()` | 从标准输入读取一个浮点数 |

### 字符串 I/O

| 函数签名 | 说明 |
|---------|------|
| `void print_string(qlang_string* s)` | 输出字符串，不换行 |
| `void println_string(qlang_string* s)` | 输出字符串并换行 |

### 布尔 I/O

| 函数签名 | 说明 |
|---------|------|
| `void print_bool(int32_t v)` | 输出 `true` 或 `false`，不换行 |
| `void println_bool(int32_t v)` | 输出 `true` 或 `false` 并换行 |

## 字符串类型

运行时内部使用以下 C 结构体表示 QLang 字符串，与 LLVM IR 中的 `%string` 类型一一对应：

```c
typedef struct {
    const char *data;  // 字符串数据指针
    int32_t     len;   // 字符串长度（字节数）
} qlang_string;
```

## 构建

运行时随整个项目一起构建：

```bash
bash init.sh build
```

构建产物位于 `build/runtime/libqlang_runtime.a`，在链接阶段由 `clang` 自动链接到目标程序。
