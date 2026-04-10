{** 数组元素求和 **}

int32 sum(int32[5] arr) {
    int32 total;
    total = 0;
    int32 i;
    for (i = 0; i < 5; i = i + 1) {
        total = total + arr[i];
    }
    return total;
}

void main() {
    int32[5] numbers;
    numbers[0] = 10;
    numbers[1] = 20;
    numbers[2] = 30;
    numbers[3] = 40;
    numbers[4] = 50;
    println_int(sum(numbers));
}