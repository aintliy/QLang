{** 测试: ++ 运算符不支持 **}
void main() {
    int32 a;
    a++;   {** ERROR: unsupported binary operator '+' or unsupported unary operator '++' **}
}