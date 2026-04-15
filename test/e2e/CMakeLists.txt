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

# 输出类测试 (existing)
add_e2e_output_test(test_fibonacci)
add_e2e_output_test(test_sum_array)
add_e2e_output_test(test_struct_switch)
add_e2e_output_test(test_string)
add_e2e_output_test(test_global)
add_e2e_output_test(test_literal_logical)

# 输出类测试 (new)
add_e2e_output_test(test_codegen_control)
add_e2e_output_test(test_codegen_for)
add_e2e_output_test(test_codegen_if)
add_e2e_output_test(test_codegen_return)
add_e2e_output_test(test_codegen_switch)
add_e2e_output_test(test_codegen_while)
add_e2e_output_test(test_matrix_add)
add_e2e_output_test(test_matrix_func)
add_e2e_output_test(test_matrix_index)
add_e2e_output_test(test_matrix_mul)
add_e2e_output_test(test_matrix_neg)
add_e2e_output_test(test_matrix_print)
add_e2e_output_test(test_matrix_scalar)
add_e2e_output_test(test_matrix_sub)
add_e2e_output_test(test_matrix_semantic_valid)
add_e2e_output_test(test_array)
add_e2e_output_test(test_float_arith)
add_e2e_output_test(test_global_order)
add_e2e_output_test(test_shadow)
add_e2e_output_test(test_scope_block)
add_e2e_output_test(test_memcpy)
add_e2e_output_test(test_continue_in_switch_loop)
add_e2e_output_test(test_comprehensive)
add_e2e_output_test(test_struct)
add_e2e_output_test(test_struct_return)
add_e2e_output_test(test_string_matrix_interaction)

# abort 类测试 (existing)
add_e2e_abort_test(test_array_bounds)
add_e2e_abort_test(test_divzero)
add_e2e_abort_test(test_stack_overflow)

# abort 类测试 (new - runtime)
add_e2e_abort_test(test_matrix_index_bounds)
add_e2e_abort_test(test_nesting_depth_1025)

# abort 类测试 (new - compile error)
add_e2e_abort_test(test_unsupported_op_bitwise)
add_e2e_abort_test(test_unsupported_op_minus_minus)
add_e2e_abort_test(test_unsupported_op_plus_plus)
add_e2e_abort_test(test_unsupported_op_unary_plus)
add_e2e_abort_test(test_continue_outside)
add_e2e_abort_test(test_break_outside)
add_e2e_abort_test(test_switch_case_no_break)
add_e2e_abort_test(test_no_return_path)
add_e2e_abort_test(test_struct_recursive)
add_e2e_abort_test(test_array_size_limit)
add_e2e_abort_test(test_div_zero_literal)
add_e2e_abort_test(test_semantic_switch)
add_e2e_abort_test(test_semantic_return)
