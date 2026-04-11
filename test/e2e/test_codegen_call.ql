{int** 测试函数调用 **}
int32 add(int32 a, int32 b) {
    return a + b;
}

void main() {
    println_int(add(10, 20));
    println_int(add(30, add(5, 5)));
}
