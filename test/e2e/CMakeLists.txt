# test/e2e/CMakeLists.txt - QLang 端到端测试

set(E2E_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(E2E_RUNNER ${E2E_DIR}/run_e2e_test.sh)
set(EXPECTED_DIR ${E2E_DIR}/expected)

include(CMakeParseArguments)

# add_e2e_output_test(name): expects zero exit, verifies output
macro(add_e2e_output_test name)
    add_test(
        NAME e2e_${name}
        COMMAND ${E2E_RUNNER}
            $<TARGET_FILE:qlang_driver>
            ${E2E_DIR}/${name}.ql
            $<TARGET_FILE_DIR:qlang_runtime>/libqlang_runtime.a
            ${EXPECTED_DIR}/${name}.expected
            0
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    set_tests_properties(e2e_${name} PROPERTIES TIMEOUT 30)
endmacro()

# add_e2e_abort_test(name): expects non-zero exit (abort)
macro(add_e2e_abort_test name)
    add_test(
        NAME e2e_${name}
        COMMAND ${E2E_RUNNER}
            $<TARGET_FILE:qlang_driver>
            ${E2E_DIR}/${name}.ql
            $<TARGET_FILE_DIR:qlang_runtime>/libqlang_runtime.a
            ""
            nonzero
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    set_tests_properties(e2e_${name} PROPERTIES TIMEOUT 30)
endmacro()

# 输出类测试
add_e2e_output_test(test_fibonacci)
add_e2e_output_test(test_sum_array)
add_e2e_output_test(test_struct_switch)
add_e2e_output_test(test_string)
add_e2e_output_test(test_global)

# abort 类测试
add_e2e_abort_test(test_array_bounds)
add_e2e_abort_test(test_divzero)
add_e2e_abort_test(test_stack_overflow)