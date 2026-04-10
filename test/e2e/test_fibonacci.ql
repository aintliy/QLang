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