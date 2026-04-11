{** 测试: break 在循环或 switch 外是编译错误 **}
void main() {
    break;   {** ERROR: break outside switch or loop **}
}