{** Variable shadowing regression test **}
void main() {
    int32 x;
    x = 1;
    println_int(x);
    {
        int32 x;
        x = 2;
        println_int(x);
    }
    println_int(x);
    {
        int32 x;
        x = 3;
        println_int(x);
    }
    println_int(x);
}