{** 结构体测试 **}
struct Point {
    int32 x;
    int32 y;
}

void main() {
    struct Point p;
    p.x = 42;
    p.y = 99;
    println_int(p.x);
    println_int(p.y);
}