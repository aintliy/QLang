#!/usr/bin/env python3
"""Generate QLang file with deep nesting to test 1024 depth limit."""

def generate_nested_calls(n):
    """Generate n nested id() calls around a literal 1."""
    return "id(" * n + "1" + ")" * n

def main():
    nesting_depth = 1025

    code = f"""{{** 测试: 函数调用嵌套深度超过 1024 是编译错误 **}}
int32 id(int32 x) {{ return x; }}

void main() {{
    int32 result;
    result = {generate_nested_calls(nesting_depth)};
}}
"""
    output_file = "test/e2e/test_nesting_depth_1025.ql"
    with open(output_file, "w") as f:
        f.write(code)
    print(f"Generated {output_file} with {nesting_depth} nested id() calls")

if __name__ == "__main__":
    main()