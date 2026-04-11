{** 测试: 字面量零除数是编译错误 **}
void main() {
    int32 a;
    a = 0 / 0;   {** ERROR: division by literal zero **}
}