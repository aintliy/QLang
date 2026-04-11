{** for 循环测试 **}
void main() {
    int32 i;
    for (i = 0; i < 3; i = i + 1) {
        println_int(i);
    }
    {** 测试无限循环 break **}
    for (;;) {
        println_int(99);
        break;
    }
}