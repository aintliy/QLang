{** memcpy 整体赋值测试 **}
void main() {
    int32[3] a;
    int32[3] b;
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    b = a;
    println_int(b[0]);
    println_int(b[1]);
    println_int(b[2]);
}