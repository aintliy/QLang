{** 测试: 全局变量在使用前必须声明 **}
int32 a;

void use_a() {
    int32 t = a;    {** OK: a 已在使用前声明 **}
}

int32 b = a + 1;    {** OK: a 已声明 **}

void main() {
}