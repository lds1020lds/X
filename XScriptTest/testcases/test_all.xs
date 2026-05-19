// ============================================================
// XScript testcases 聚合入口
// 运行本文件即可按顺序执行 testcases 目录下的全部 .xs 测试脚本。
// ============================================================

gTestcasePassCount = 0;
gTestcaseFailCount = 0;
function runTestcase(path)
{
    printf("========================================");
    printf("RUN " $ path);
    printf("========================================");

    var ok, err = pcall(function()
    {
        require(path);
    });

    if (ok == 0)
    {
        gTestcasePassCount++;
        printf("[PASS] " $ path);
    }
    else
    {
        gTestcaseFailCount++;
        printf("[FAIL] " $ path);
        printf(err);
    }
}

function runExpectedFailTestcase(path)
{
    printf("========================================");
    printf("RUN EXPECTED FAIL " $ path);
    printf("========================================");

    var ok, err = pcall(function()
    {
        require(path);
    });

    if (ok != 0)
    {
        gTestcasePassCount++;
        printf("[PASS] " $ path $ " failed as expected");
        printf(err);
    }
    else
    {
        gTestcaseFailCount++;
        printf("[FAIL] " $ path $ " should fail but passed");
    }
}

function runAllTestcases()
{
    printf("========================================");
    printf("  XScript testcases all-in-one runner");
    printf("========================================");

    runTestcase("testcases/asyncawait/test_async_await.xs");
    runTestcase("testcases/asyncawait/test_async_file.xs");
    runTestcase("testcases/asyncawait/test_async_http.xs");
    runTestcase("testcases/asyncawait/test_async_more.xs");
    runTestcase("testcases/asyncawait/test_async_run_modes.xs");
    runTestcase("testcases/asyncawait/test_async_socket.xs");
    runTestcase("testcases/asyncawait/test_async_utilities.xs");

    runTestcase("testcases/deconstruction/test_decontruction.xs");

    runTestcase("testcases/comprehension/test_comprehension.xs");

    runTestcase("testcases/defaultargs/test_default_args.xs");
    runExpectedFailTestcase("testcases/defaultargs/test_default_args_invalid.xs");
    runTestcase("testcases/defaultargs/test_default_args_more.xs");

    runTestcase("testcases/defer/test_defer.xs");

    runTestcase("testcases/fstring/testfstring.xs");

    runTestcase("testcases/iterator/test_iterator.xs");

    runTestcase("testcases/match/test_match.xs");

    runTestcase("testcases/nullcoalescing/test_null_coalescing.xs");

    runTestcase("testcases/returnregs/test_return_regs.xs");

    runTestcase("testcases/higherorder/test_iterlib.xs");

    runTestcase("testcases/switchcase/test_switch_case.xs");

    runTestcase("testcases/triple_string/test_triple_string.xs");

    runTestcase("testcases/trycatch/test_async_await_c_boundary.xs");
    runTestcase("testcases/trycatch/test_async_reject_catch.xs");
    runTestcase("testcases/trycatch/test_async_reject_combinators.xs");
    runTestcase("testcases/trycatch/test_async_reject_top.xs");
    runTestcase("testcases/trycatch/test_async_throw_catch.xs");
    runTestcase("testcases/trycatch/test_async_trycatch_simple.xs");
    runTestcase("testcases/trycatch/test_run_until_complete_reject.xs");
    runTestcase("testcases/trycatch/test_trycatch_basic.xs");
    runTestcase("testcases/trycatch/test_trycatch_cross_lua.xs");
    runTestcase("testcases/trycatch/test_trycatch_limitations.xs");
    runTestcase("testcases/trycatch/test_trycatch_nested.xs");
    runTestcase("testcases/trycatch/test_trycatch_optimize_pc.xs");
    runTestcase("testcases/trycatch/test_trycatch_rethrow_limit.xs");
    runTestcase("testcases/trycatch/test_trycatch_values.xs");

    printf("========================================");
    printf("testcases summary: " $ toString(gTestcasePassCount) $ " passed, " $ toString(gTestcaseFailCount) $ " failed");
    printf("========================================");
}

runAllTestcases();
