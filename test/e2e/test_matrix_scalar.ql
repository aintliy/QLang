{** Test scalar multiplication **}

void main() {
    mat<int32> 2x2 a;
    mat<int32> 2x2 b;
    int32 k;

    a[0][0] = 1; a[0][1] = 2;
    a[1][0] = 3; a[1][1] = 4;

    k = 3;
    b = k * a;

    println_int(b[0][0]);
    println_int(b[0][1]);
    println_int(b[1][0]);
    println_int(b[1][1]);
}