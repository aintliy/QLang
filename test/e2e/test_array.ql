{** 数组测试 **}
void main() {
    int32[5] arr;
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    println_int(arr[0]);
    println_int(arr[1]);
    println_int(arr[2]);
    {** 数组整体赋值 **}
    int32[5] arr2;
    arr2 = arr;
    println_int(arr2[0]);
    println_int(arr2[1]);
}