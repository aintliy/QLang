{** Test matrix subtraction **}

void main() {
    mat<int32> 2x2 a;
    mat<int32> 2x2 b;
    mat<int32> 2x2 c;

    a[0][0] = 10; a[0][1] = 20;
    a[1][0] = 30; a[1][1] = 40;

    b[0][0] = 1; b[0][1] = 2;
    b[1][0] = 3; b[1][1] = 4;

    c = a - b;

    println_int(c[0][0]);
    println_int(c[0][1]);
    println_int(c[1][0]);
    println_int(c[1][1]);
}