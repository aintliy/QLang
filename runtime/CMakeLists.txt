# Runtime library
add_library(qlang_runtime qlang_runtime.cpp)
target_include_directories(qlang_runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
