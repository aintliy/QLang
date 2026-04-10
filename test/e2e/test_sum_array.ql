// Array sum (LRM 8.2)
int32 sum(int32[5] arr) {
    int32 total = 0;
    for (i = 0; i < 5; i = i + 1) {
        total = total + arr[i];
    }
    return total;
}

void main() {
    int32[5] nums = {1, 2, 3, 4, 5};
    int32 s = sum(nums);
}