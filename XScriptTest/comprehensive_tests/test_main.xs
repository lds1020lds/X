// ============================================================
// XScript 综合测试 - 主入口
// ============================================================
    // 立即调用匿名函数
//ts = (a).b

 var outer = {inner = {val = 99}};
var ref1 = {value = 10};



//var result = (function() { return 42; })();

require("comprehensive_tests/test_helper.xs");
require("comprehensive_tests/test_basics.xs");
require("comprehensive_tests/test_control_flow.xs");
require("comprehensive_tests/test_data_structures.xs");
require("comprehensive_tests/test_function_closure.xs");
require("comprehensive_tests/test_advanced.xs");
require("comprehensive_tests/test_block_scope.xs");
require("comprehensive_tests/test_edge_cases.xs");
require("comprehensive_tests/test_stdlibs.xs");
require("comprehensive_tests/test_pipeline.xs");

function main()
{
    printf("========================================");
    printf("  XScript Comprehensive Test Suite");
    printf("========================================");

    // 基础语法
    test_variable();
    test_nil_constant_regression();
    test_arithmetic();
    test_bitwise();
    test_comparison();
    test_logical();
    test_string_concat();
    test_variable_edges();
    test_operator_edges();
    // 控制流
    test_if_else();
    test_while();
    test_for();
    test_foreach();
    test_control_flow_edges();
    test_for_empty_parts();

    // 函数与闭包
    test_function();
    test_scope();
    test_closure();
    test_lambda();
    test_complex_temp_register_expressions();
    test_nested_closure();
    test_function_edges();
    test_varargs();
    test_call_args_adjust();

    // 数据结构
    test_table();
    test_list();
    test_table_constructor();
    test_table_ops();
    test_table_delete_reuse();
    test_set_like();
    test_data_structure_edges();

    // 高级特性
    test_string_lib();
    test_math_lib();
    test_type_and_tostring();
    test_pcall();
    test_metatable();
    test_coroutine_basic();
    test_fibonacci();
    test_hanoi();
    test_bubble_sort();
    test_string_operations();
    test_ternary();
    test_chained_access();
    test_multi_return();
    test_loadstring();
    test_expression_priority();
    test_gc();
    test_deep_recursion();
    test_metatable_ops();
    test_coroutine_wrap();
    test_xpcall();
    test_list_sort_advanced();
    test_toNumber();
    test_list_func();
    test_string_method();
    test_true_false();
    test_string_edge_cases();
    test_math_edges();
    test_metatable_operator_edges();
    test_metatable_full_coverage();
    test_metatable_index_newindex_edges();
    test_metatable_gc_edges();
    test_coroutine_status_edges();
    test_dynamic_error_edges();
    test_gc_edges();
    test_pipeline();

    // Block 变量作用域
    test_block_basic();
    test_block_shadow();
    test_block_nested();
    test_block_multi_shadow();
    test_block_modify_outer();
    test_block_multi_vars();
    test_block_if_else();
    test_block_while();
    test_block_for();
    test_block_with_function();
    test_block_sequential();
    test_block_compound_assign();
    test_block_closure();
    test_block_complex();
    test_block_loop_nested();
    test_block_shadow_in_loop();
    // 边界情况与高级场景
    test_string_escape();
    test_coroutine_resume_args();
    test_coroutine_generator();
    test_metatable_chain();
    test_metatable_index_complex();
    test_recursive_closure();
    test_iife();
    test_table_reference();
    test_error_boundaries();
    test_complex_expressions();
    test_method_chaining();
    test_global_scope();
    test_table_dynamic();
    test_higher_order();
    test_oop_pattern();
    test_closure_factory();
    test_multi_return_advanced();
    test_string_immutable();
    test_numeric_edges();
    test_for_advanced();
    test_list_advanced();
    test_coroutine_closure();
    test_quicksort();
    test_binary_search();
    test_state_machine();
    test_numeric_bruteforce();
    test_table_list_stress();
    test_closure_logic_stress();
    test_loadstring_boundary_stress();
    test_paren_expr_statement();

    // 新标准库
    test_json_lib();
    test_path_lib();
    test_time_lib();
    test_table_lib();
    test_regex_lib();
    test_http_lib();

	require("comprehensive_tests/test_require_main.xs");

    summary();
}

main();
