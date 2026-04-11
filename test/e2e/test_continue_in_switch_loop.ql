{** 测试: continue 在 switch 内但外层有循环时跳转到循环 increment **}
void main() {
    int32 i = 0;
    for (;;) {
        i = i + 1;
        switch (i) {
            case 10:
                continue;   {** OK: 跳转到 for 的 increment (i = i + 1) **}
        }
        if (i > 100) {
            break;
        }
    }
}