{** 测试: 结构体不能比较 **}
struct Point {
    int32 x;
    int32 y;
}

void main() {
    struct Point p1;
    struct Point p2;
    bool result;
    result = p1 == p2;   {** ERROR: array or struct cannot be compared **}
}