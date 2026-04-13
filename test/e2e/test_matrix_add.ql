{** Test matrix addition **}

void main() {
    mat<int32> 2x2 a;
    mat<int32> 2x2 b;
    mat<int32> 2x2 c;

    a[0][0] = 1; a[0][1] = 2;
    a[1][0] = 3; a[1][1] = 4;

    b[0][0] = 5; b[0][1] = 6;
    b[1][0] = 7; b[1][1] = 8;

    c = a + b;

    println_int(c[0][0]);
    println_int(c[0][1]);
    println_int(c[1][0]);
    println_int(c[1][1]);
}