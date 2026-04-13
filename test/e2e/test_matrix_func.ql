{** Test matrix as function parameter and return value **}

mat<int32> 2x2 identity() {
    mat<int32> 2x2 m;
    m[0][0] = 1;
    m[1][1] = 1;
    return m;
}

int32 trace(mat<int32> 2x2 mm) {
    return mm[0][0] + mm[1][1];
}

void main() {
    mat<int32> 2x2 id;
    int32 t;

    id = identity();
    t = trace(id);

    println_int(id[0][0]);
    println_int(id[1][1]);
    println_int(t);
}