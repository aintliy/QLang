{** switch 语句测试 **}
void main() {
    int32 day;
    day = 2;
    switch (day) {
        case 1:
            println_int(1);
            break;
        case 2:
            println_int(2);
            break;
        case 3:
            println_int(3);
            break;
        default:
            println_int(0);
            break;
    }
}