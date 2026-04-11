{** 测试: 一元加运算符不支持 **}
void main() {
    int32 a;
    a = +5;   {** ERROR: unsupported unary operator '+' **}
}