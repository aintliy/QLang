{** 测试: 递归结构体定义是编译错误 **}
struct Node {
    int32 value;
    struct Node next;   {** ERROR: recursive struct definition 'Node' **}
}

void main() {
}