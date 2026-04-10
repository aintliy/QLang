// Struct with switch (LRM 8.3)
struct Shape {
    int32 kind;
    int32 size;
}

void main() {
    Shape s;
    s.kind = 1;
    s.size = 10;

    switch (s.kind) {
        case 0: s.size = 0;
        case 1: s.size = s.size + 1; break;
        default: s.size = -1; break;
    }
}