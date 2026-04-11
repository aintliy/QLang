{** 表达式代码生成测试 **}

void main() {
    // 算术运算
    println_int(10 + 20);
    println_int(30 - 5);
    println_int(6 * 7);
    println_int(40 / 8);
    println_int(25 % 7);
    println_int(-42);

    // 关系运算
    println_int(10 < 20);
    println_int(10 <= 10);
    println_int(5 > 3);
    println_int(5 >= 5);
    println_int(3 == 3);
    println_int(3 != 4);

    // 逻辑运算
    println_int(1 && 1);
    println_int(0 && 1);
    println_int(1 || 0);
    println_int(0 || 0);
    println_int(!0);
    println_int(!1);

    // 浮点运算
    println_float64(3.14 + 2.86);
    println_float64(10.0 - 3.5);
    println_float64(2.0 * 3.5);
    println_float64(10.0 / 2.5);
}
