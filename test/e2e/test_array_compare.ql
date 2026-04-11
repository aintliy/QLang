{** 测试: 数组不能比较 **}
void main() {
    int32[3] a;
    int32[3] b;
    bool result;
    result = a == b;   {** ERROR: array or struct cannot be compared **}
}