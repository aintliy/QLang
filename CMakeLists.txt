cmake_minimum_required(VERSION 3.16)
project(QLang C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create stub targets for LLVM optional dependencies that may not be found
add_library(ZLIB::ZLIB INTERFACE IMPORTED)
set_target_properties(ZLIB::ZLIB PROPERTIES INTERFACE_LINK_LIBRARIES "/usr/lib/x86_64-linux-gnu/libz.so.1")

add_library(zstd::libzstd_shared INTERFACE IMPORTED)
set_target_properties(zstd::libzstd_shared PROPERTIES INTERFACE_LINK_LIBRARIES "/usr/lib/x86_64-linux-gnu/libzstd.so.1")

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
