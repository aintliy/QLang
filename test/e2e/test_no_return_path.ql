{** 测试: 非 void 函数必须存在 return 代码路径 **}
int32 foo() {   {** ERROR: non-void function must have return **}
    int32 x = 1;
}

void main() {
}