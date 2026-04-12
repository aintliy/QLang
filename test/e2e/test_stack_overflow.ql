{** 栈溢出测试：程序应该 abort **}
int32 fib(int32 n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

void main() {
    int32 r;
    r = fib(10);
    println_int(r);
}