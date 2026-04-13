{** Test matrix subscript out of bounds detection **}

void main() {
    mat<int32> 2x3 m;
    int32 x;

    {** Valid access - should work **}
    m[0][0] = 100;
    m[1][2] = 200;
    x = m[0][0];
    println_int(x);
    x = m[1][2];
    println_int(x);

    {** Invalid access - should abort **}
    x = m[2][0];
    println_int(x);
}