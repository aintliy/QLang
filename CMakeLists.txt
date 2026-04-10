cmake_minimum_required(VERSION 3.16)
project(QLang CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# LLVM 配置
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# 包含 LLVM 组件
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# 运行时目录
add_subdirectory(runtime)

# 源代码目录 (占位，待实现)
add_subdirectory(src)

# 测试
enable_testing()
add_subdirectory(test)
