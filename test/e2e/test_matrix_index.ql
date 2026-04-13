{** Test matrix subscript access **}

void main() {
    mat<int32> 2x3 m;
    int32 x;

    m[0][0] = 10;
    m[0][1] = 20;
    m[0][2] = 30;
    m[1][0] = 40;
    m[1][1] = 50;
    m[1][2] = 60;

    x = m[0][0];
    println_int(x);

    x = m[1][2];
    println_int(x);
}