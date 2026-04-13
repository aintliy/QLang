#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* QLang 字符串类型：与 LLVM IR %string = type { ptr, i32 } 对应 */
typedef struct {
    const char* data;
    int32_t len;
} qlang_string;

/* ===== 整数 ===== */
void print_int(int32_t val) { printf("%d", val); }
void println_int(int32_t val) { printf("%d\n", val); }

/* ===== 浮点 ===== */
void print_float64(double val) { printf("%g", val); }
void println_float64(double val) { printf("%g\n", val); }

/* ===== 布尔 ===== */
void print_bool(int val) { printf("%s", val ? "true" : "false"); }
void println_bool(int val) { printf("%s\n", val ? "true" : "false"); }

/* ===== 字符串 ===== */
void print_string(qlang_string* s) {
    if (s && s->data && s->len > 0) {
        fwrite(s->data, 1, (size_t)s->len, stdout);
    }
}

void println_string(qlang_string* s) {
    if (s && s->data && s->len > 0) {
        fwrite(s->data, 1, (size_t)s->len, stdout);
    }
    printf("\n");
}

/* ===== 输入 ===== */
int32_t read_int(void) {
    int32_t val = 0;
    scanf("%d", &val);
    return val;
}

double read_float64(void) {
    double val = 0.0;
    scanf("%lf", &val);
    return val;
}

/* ===== 矩阵打印 ===== */
// 打印 int32 矩阵
void println_matrix_int(int32_t* matrix, int32_t rows, int32_t cols) {
    for (int32_t i = 0; i < rows; i++) {
        for (int32_t j = 0; j < cols; j++) {
            if (j > 0) printf(" ");
            printf("%d", matrix[i * cols + j]);
        }
        printf("\n");
    }
}

// 打印 float64 矩阵
void println_matrix_float(double* matrix, int32_t rows, int32_t cols) {
    for (int32_t i = 0; i < rows; i++) {
        for (int32_t j = 0; j < cols; j++) {
            if (j > 0) printf(" ");
            printf("%.1f", matrix[i * cols + j]);
        }
        printf("\n");
    }
}