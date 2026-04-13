{** Test matrix type declarations - local variables only **}
void main() {
    mat<int32> 2x2 local_mat;
    local_mat[0][0] = 1;
    local_mat[0][1] = 2;
    local_mat[1][0] = 3;
    local_mat[1][1] = 4;
}