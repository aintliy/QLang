{** Test matrix negation **}

void main() {
    mat<int32> 2x2 a;
    mat<int32> 2x2 b;

    a[0][0] = 1; a[0][1] = -2;
    a[1][0] = 3; a[1][1] = -4;

    b = -a;

    println_int(b[0][0]);
    println_int(b[0][1]);
    println_int(b[1][0]);
    println_int(b[1][1]);
}