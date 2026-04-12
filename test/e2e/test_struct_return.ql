{** 结构体返回值测试 **}
struct Point {
    int32 x;
    int32 y;
}

struct Point create_point(int32 x, int32 y) {
    struct Point p;
    p.x = x;
    p.y = y;
    return p;
}

void main() {
    struct Point pt;
    pt = create_point(10, 20);
    println_int(pt.x);
    println_int(pt.y);
}