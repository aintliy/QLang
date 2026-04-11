{** 测试: 返回类型不匹配 **}
int32 test() {
    return 1.5;  {** ERROR: cannot convert 'float64' to 'int32' **}
}

void main() {
}