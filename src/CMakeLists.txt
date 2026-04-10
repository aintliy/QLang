if(NOT LLVM_DIR)
    set(LLVM_DIR "/usr/lib/llvm-18")
endif()
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM: ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVM: ${LLVM_BINARY_DIR}")

add_compile_options(-std=c++17)

# Token
add_library(qlang_token STATIC Token.h Token.cpp)
target_include_directories(qlang_token PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Diagnostics
add_library(qlang_diagnostics STATIC Diagnostics.h Diagnostics.cpp)
target_include_directories(qlang_diagnostics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Lexer
add_library(qlang_lexer STATIC Lexer.h Lexer.cpp)
target_include_directories(qlang_lexer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qlang_lexer PUBLIC qlang_token qlang_diagnostics)

# Driver (for testing)
add_executable(qlang_driver Driver/main.cpp)
target_link_libraries(qlang_driver PRIVATE qlang_lexer qlang_token qlang_diagnostics LLVM::Core)