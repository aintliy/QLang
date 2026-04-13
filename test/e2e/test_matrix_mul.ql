{** Test matrix multiplication: 2x3 * 3x2 = 2x2 **}

void main() {
    mat<int32> 2x3 a;
    mat<int32> 3x2 b;
    mat<int32> 2x2 c;

    a[0][0] = 1; a[0][1] = 2; a[0][2] = 3;
    a[1][0] = 4; a[1][1] = 5; a[1][2] = 6;

    b[0][0] = 1; b[0][1] = 2;
    b[1][0] = 3; b[1][1] = 4;
    b[2][0] = 5; b[2][1] = 6;

    c = a * b;

    println_int(c[0][0]);
    println_int(c[0][1]);
    println_int(c[1][0]);
    println_int(c[1][1]);
}