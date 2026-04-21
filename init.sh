#!/bin/bash
# QLang Compiler Build and Run Script (CMake)
# 用法: ./init.sh [选项]
#   ./init.sh          - 编译所有源文件
#   ./init.sh test     - 运行所有测试
#   ./init.sh clean    - 清理构建产物
#   ./init.sh run FILE [-O2] - 编译并运行 FILE.ql（可选 -O2 优化）

set -e

# LLVM 工具链路径 (Ubuntu WSL)
# LLVM 通过 find_package(LLVM) 自动发现，此处仅用于 PATH
LLVM_DIR="${LLVM_DIR:-/usr/lib/llvm-18}"
LLVM_BIN="${LLVM_DIR}/bin"
export PATH="$LLVM_BIN:$PATH"

# 目录结构
SRC_DIR="src"
BUILD_DIR="build"
RUNTIME_DIR="runtime"
TEST_DIR="test/e2e"
OUT_DIR="."

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# 检查 LLVM 工具链
check_llvm() {
    if ! command -v llc &> /dev/null; then
        log_error "llc not found. Please install LLVM toolchain."
        exit 1
    fi
    if ! command -v lli &> /dev/null; then
        log_warn "lli not found. Install LLVM for direct IR execution."
    fi
    log_info "LLVM version: $(llc --version | head -1)"
}

# CMake 构建
build_cmake() {
    log_info "Building with CMake..."
    mkdir -p "$BUILD_DIR"
    cmake -B "$BUILD_DIR" -S . -DLLVM_DIR="$LLVM_DIR"
    cmake --build "$BUILD_DIR" --parallel
    log_info "Build complete."
}

# 编译 .ql 文件
compile_qlang() {
    local input="$1"
    local output="${2:-a.out}"
    shift 2
    local extra_args=("$@")

    if [ ! -f "$input" ]; then
        log_error "File not found: $input"
        return 1
    fi

    log_info "Compiling $input..."

    # Step 1: Lexer + Parser + Sema + Codegen -> IR
    "$BUILD_DIR/src/qlang_driver" "$input" -S -o "$BUILD_DIR/output.ll" "${extra_args[@]}"

    # Step 2: Compile IR -> object (use --filetype=obj for relocatable object)
    llc --filetype=obj "$BUILD_DIR/output.ll" -o "$BUILD_DIR/output.o"

    # Step 3: Link
    # Find clang (prefer system clang, fall back to LLVM's clang)
    if command -v clang &> /dev/null; then
        local CLANG="clang"
    else
        local CLANG="$LLVM_BIN/clang"
    fi
    if [ "$(uname -s)" = "Windows_NT" ]; then
        $CLANG -no-pie "$BUILD_DIR/output.o" "$BUILD_DIR/runtime/libqlang_runtime.a" -o "$output"
    else
        $CLANG -no-pie "$BUILD_DIR/output.o" "$BUILD_DIR/runtime/libqlang_runtime.a" -o "$output"
    fi

    log_info "Output written to: $output"
}

# 运行测试
run_tests() {
    log_info "Running tests via CTest..."
    cd "$BUILD_DIR" && ctest --output-on-failure
    log_info "All tests passed!"
}

# 清理
clean() {
    log_info "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    log_info "Clean complete."
}

# 主逻辑
case "${1:-build}" in
    build)
        check_llvm
        build_cmake
        ;;
    test)
        run_tests
        ;;
    clean)
        clean
        ;;
    run)
        if [ -z "$2" ]; then
            log_error "Usage: $0 run FILE [-O2]"
            exit 1
        fi
        check_llvm
        build_cmake
        opt_args=()
        if [ "${3:-}" = "-O2" ]; then
            opt_args+=("-O2")
            log_info "Optimization: -O2 enabled"
        fi
        # 支持 test/e2e/FILE.ql 或直接 FILE.ql（优先 test/e2e/）
        if [ -f "test/e2e/$2.ql" ]; then
            compile_qlang "test/e2e/$2.ql" "$2" "${opt_args[@]}"
        else
            compile_qlang "$2.ql" "$2" "${opt_args[@]}"
        fi
        ;;
    *)
        echo "Usage: $0 {build|test|clean|run FILE [-O2]}"
        exit 1
        ;;
esac
