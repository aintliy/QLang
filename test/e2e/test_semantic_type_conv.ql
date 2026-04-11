{** 测试: int32 到 float64 隐式转换应报错 **}
void test() {
    float64 x;
    int32 y;
    x = y;  {** ERROR: cannot convert 'int32' to 'float64' **}
}

void main() {
    test();
}