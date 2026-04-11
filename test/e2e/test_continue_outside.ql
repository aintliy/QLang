{** 测试: continue 在循环外是编译错误 **}
void main() {
    continue;   {** ERROR: continue outside loop **}
}