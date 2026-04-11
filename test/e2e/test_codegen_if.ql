{** if-else 语句测试 **}
void main() {
    int32 x;
    x = 10;
    if (x > 5) {
        println_int(1);
    } else {
        println_int(0);
    }
    if (x < 5) {
        println_int(0);
    } else {
        println_int(1);
    }
}