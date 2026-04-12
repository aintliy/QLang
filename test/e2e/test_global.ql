{** 全局变量测试 **}
int32 counter;
int32 value;

void increment() {
    counter = counter + 1;
}

void main() {
    counter = 0;
    increment();
    increment();
    increment();
    println_int(counter);
    value = 100;
    println_int(value);
}
