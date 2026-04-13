{** Test string before matrix multiplication **}
void main() {
    {** String first **}
    string s1;
    s1 = "hello";

    {** Then matrix multiplication **}
    mat<int32> 2x3 left;
    mat<int32> 3x2 right;
    mat<int32> 2x2 result;

    left[0][0] = 1; left[0][1] = 2; left[0][2] = 3;
    left[1][0] = 4; left[1][1] = 5; left[1][2] = 6;

    right[0][0] = 1; right[0][1] = 2;
    right[1][0] = 3; right[1][1] = 4;
    right[2][0] = 5; right[2][1] = 6;

    result = left * right;
    println_int(result[0][0]);
    println_int(result[0][1]);
    println_int(result[1][0]);
    println_int(result[1][1]);
}