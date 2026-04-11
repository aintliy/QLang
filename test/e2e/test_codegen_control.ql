{** break/continue 控制流测试 **}
void main() {
    int32 i;
    {** 测试 break **}
    i = 0;
    while (i < 10) {
        if (i >= 5) {
            break;
        }
        println_int(i);
        i = i + 1;
    }
    {** 测试 continue **}
    i = 0;
    while (i < 5) {
        i = i + 1;
        if (i == 3) {
            continue;
        }
        println_int(i);
    }
}