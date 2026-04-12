{** 字符串测试 **}
void main() {
    string s1;
    string s2;
    s1 = "hello";
    s2 = "world";
    println_string(s1);
    {** 测试字符串比较 **}
    if (s1 == s1) {
        println_int(1);
    } else {
        println_int(0);
    }
}