{** 基础代码生成测试：全局变量和字符串字面量 **}
int32 global_var;
float64 global_float;
string global_str;

void main() {
    global_var = 42;
    global_float = 3.14;
    global_str = "hello";
    println_int(global_var);
    println_float64(global_float);
    println_string(global_str);
}