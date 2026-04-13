{** Test matrix type declarations **}
mat<int32> 2x3 m1;
mat<float64> 3x4 m2;
mat<int32> 1x1 single;

void main() {
    mat<int32> 2x2 local_mat;
    local_mat[0][0] = 1;
    local_mat[0][1] = 2;
    local_mat[1][0] = 3;
    local_mat[1][1] = 4;
}