{** 栈溢出测试：程序应该 abort **}
{** countdown(2000) 递归深度为 2000，超过 1024 限制 **}
int32 countdown(int32 n) {
    if (n <= 0) {
        return 0;
    }
    return countdown(n - 1);
}

void main() {
    int32 r;
    r = countdown(2000);
    println_int(r);
}
