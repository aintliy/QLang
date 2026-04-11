{** 测试: 位运算运算符不支持 **}
void main() {
    int32 a;
    a = 1 & 2;   {** ERROR: unsupported binary operator '&' **}
}