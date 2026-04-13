{** Test matrix semantic errors **}

{** 1. 测试矩阵维度不匹配错误 **}
void test1() {
    mat<int32> 2x3 a;
    mat<int32> 3x2 b;
    mat<int32> 2x2 c;
    c = a + b;  {** ERROR: matrix dimension mismatch **}
}

{** 2. 测试矩阵乘法维度不匹配错误 **}
void test2() {
    mat<int32> 2x3 d;
    mat<int32> 2x4 e;
    mat<int32> 2x4 f;
    f = d * e;  {** ERROR: matrix dimension mismatch for multiplication **}
}

{** 3. 测试禁止矩阵比较 **}
void test3() {
    mat<int32> 2x2 g;
    mat<int32> 2x2 h;
    if (g == h) { }  {** ERROR: matrix cannot be compared **}
}

{** 4. 测试禁止矩阵作为条件表达式 **}
void test4() {
    mat<int32> 1x1 i;
    if (i) { }  {** ERROR: matrix cannot be used as condition **}
}

{** 5. 测试矩阵维度必须为正整数 **}
void test5() {
    mat<int32> 0x3 j;  {** ERROR: matrix dimension must be positive **}
}

{** 6. 测试矩阵总元素数限制 **}
void test6() {
    mat<int32> 1024x1025 m;  {** ERROR: matrix size exceeds limit **}
}

void main() { }