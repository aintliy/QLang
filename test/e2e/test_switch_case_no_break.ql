{** 测试: switch case 不以 break 结尾是编译错误 **}
void main() {
    int32 x = 1;
    switch (x) {
        case 1:
            x = 2;   {** ERROR: case must end with break **}
        default:
            x = 3;
    }
}