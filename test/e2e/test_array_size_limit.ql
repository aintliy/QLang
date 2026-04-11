{** 测试: 数组大小超过 1048576 是编译错误 **}
void main() {
    int32[1048577] arr;   {** ERROR: array size 1048577 exceeds maximum 1048576 **}
}