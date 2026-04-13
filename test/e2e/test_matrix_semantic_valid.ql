{** Test valid matrix operations - should compile without errors **}

void main() {
    {** 1. 矩阵加减法（同维度） **}
    mat<int32> 2x2 a;
    mat<int32> 2x2 b;
    mat<int32> 2x2 c;
    c = a + b;
    c = a - b;

    {** 2. 矩阵乘法（维度匹配） **}
    mat<int32> 2x3 d;
    mat<int32> 3x4 e;
    mat<int32> 2x4 f;
    f = d * e;

    {** 3. 数乘运算 **}
    mat<int32> 2x3 g;
    mat<int32> 2x3 h;
    int32 k;
    k = 2;
    h = k * g;
    h = g * k;

    {** 4. 矩阵求负 **}
    mat<int32> 2x2 i;
    mat<int32> 2x2 j;
    j = -i;

    {** 5. 矩阵下标访问 **}
    mat<int32> 2x3 m;
    int32 x;
    x = m[0][1];
    m[1][2] = 10;

    {** 6. 矩阵作为函数参数 **}
    {**
    int32 trace(mat<int32> 2x2 mm) {
        return mm[0][0] + mm[1][1];
    }
    **}
}