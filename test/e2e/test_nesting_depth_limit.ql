{** 测试: 函数调用嵌套深度超过 1024 是编译错误 **}
int32 id(int32 x) { return x; }

int32 deep() {
    int32 result;
    result = id(id(id(id(id(id(id(id(id(id(1))))))))));
    return result;
}

void main() {
    int32 result;
    result = deep();
}