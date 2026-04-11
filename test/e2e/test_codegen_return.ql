{** return 语句测试 **}
int32 get_val() {
    return 42;
}

int32 sum(int32 a, int32 b) {
    return a + b;
}

void test_return() {
    return;
}

void main() {
    println_int(get_val());
    println_int(sum(10, 20));
    test_return();
}