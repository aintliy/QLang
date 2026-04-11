{** 测试: switch 需要 int32 表达式 **}
void main() {
    float64 f = 1.5;
    switch (f) {  {** ERROR: switch expression must be int32 **}
        case 1: break;
    }
}