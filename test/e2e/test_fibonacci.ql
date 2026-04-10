// Fibonacci sequence (LRM 8.1)
int32 fib(int32 n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

void main() {
    int32 result = fib(10);
}