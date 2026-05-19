function assertEqual(a, b, msg)
{
    if (a != b)
    {
        printf("FAIL", msg, "expected", b, "got", a);
    }
    else
    {
        printf("OK", msg);
    }
}

function assertLog(log, expected, msg)
{
    assertEqual(log:size(), expected:size(), msg);

    var i = 0;
    while (i < expected:size())
    {
        assertEqual(log[i], expected[i], msg);
        i++;
    }
}

function appendLog(log, value)
{
    log:append(value);
}

function test_defer_runs_on_normal_exit()
{
    printf("--- test_defer_runs_on_normal_exit ---");

    var log = [];

    function body()
    {
        appendLog(log, "body");
        defer
        {
            appendLog(log, "defer");
        }
        appendLog(log, "after defer");
        return 11;
    }

    var result = body();

    assertEqual(result, 11, "normal exit keeps return value");
    assertLog(log, ["body", "after defer", "defer"], "normal exit log");
}

function test_defer_lifo_order()
{
    printf("--- test_defer_lifo_order ---");

    var log = [];

    function body()
    {
        defer
        {
            appendLog(log, "defer1");
        }
        defer
        {
            appendLog(log, "defer2");
        }
        defer
        {
            appendLog(log, "defer3");
        }
        appendLog(log, "body");
        return 22;
    }

    var result = body();

    assertEqual(result, 22, "lifo keeps return value");
    assertLog(log, ["body", "defer3", "defer2", "defer1"], "lifo log");
}

function test_defer_runs_on_early_return()
{
    printf("--- test_defer_runs_on_early_return ---");

    var log = [];

    function body(flag)
    {
        defer
        {
            appendLog(log, "outer defer");
        }
        appendLog(log, "before if");

        if (flag)
        {
            defer
            {
                appendLog(log, "inner defer");
            }
            appendLog(log, "before return");
            return 33;
        }

        appendLog(log, "after if");
        return 44;
    }

    var result = body(true);

    assertEqual(result, 33, "early return value");
    assertLog(log, ["before if", "before return", "inner defer", "outer defer"], "early return log");
}

function test_defer_block_statement()
{
    printf("--- test_defer_block_statement ---");

    var log = [];
    var x = 1;

    function body()
    {
        defer
        {
            appendLog(log, "block begin");
            x = x + 10;
            appendLog(log, "block end");
        }

        x = x + 1;
        appendLog(log, "body");
        return x;
    }

    var result = body();

    assertEqual(result, 2, "block defer does not change saved return value");
    assertEqual(x, 12, "block defer can mutate captured variable");
    assertLog(log, ["body", "block begin", "block end"], "block defer log");
}

function test_defer_captures_latest_value()
{
    printf("--- test_defer_captures_latest_value ---");

    var log = [];

    function body()
    {
        var x = 1;
        defer
        {
            appendLog(log, x);
        }
        x = 5;
        return x;
    }

    var result = body();

    assertEqual(result, 5, "capture latest return value");
    assertLog(log, [5], "capture latest log");
}

function test_defer_nested_functions_are_independent()
{
    printf("--- test_defer_nested_functions_are_independent ---");

    var log = [];

    function child()
    {
        defer
        {
            appendLog(log, "child defer");
        }
        appendLog(log, "child body");
        return 2;
    }

    function parent()
    {
        defer
        {
            appendLog(log, "parent defer");
        }
        appendLog(log, "parent before child");
        var value = child();
        appendLog(log, "parent after child");
        return value + 1;
    }

    var result = parent();

    assertEqual(result, 3, "nested functions return value");
    assertLog(log, ["parent before child", "child body", "child defer", "parent after child", "parent defer"], "nested functions log");
}

function test_defer_runs_during_exception_unwind()
{
    printf("--- test_defer_runs_during_exception_unwind ---");

    var log = [];
    var caught = 0;

    function child()
    {
        defer
        {
            appendLog(log, "child defer1");
        }
        defer
        {
            appendLog(log, "child defer2");
        }
        appendLog(log, "child before throw");
        throw 77;
        appendLog(log, "child after throw");
        return 1;
    }

    try
    {
        child();
        appendLog(log, "try after child");
    }
    catch e
    {
        caught = e;
        appendLog(log, "catch");
    }

    assertEqual(caught, 77, "exception caught value");
    assertLog(log, ["child before throw", "child defer2", "child defer1", "catch"], "exception unwind log");
}

function test_defer_unwinds_multiple_lua_frames()
{
    printf("--- test_defer_unwinds_multiple_lua_frames ---");

    var log = [];
    var caught = 0;

    function leaf()
    {
        defer
        {
            appendLog(log, "leaf defer");
        }
        appendLog(log, "leaf body");
        throw 88;
    }

    function middle()
    {
        defer
        {
            appendLog(log, "middle defer");
        }
        appendLog(log, "middle before leaf");
        leaf();
        appendLog(log, "middle after leaf");
        return 1;
    }

    try
    {
        middle();
        appendLog(log, "try after middle");
    }
    catch e
    {
        caught = e;
        appendLog(log, "catch");
    }

    assertEqual(caught, 88, "multi-frame exception caught value");
    assertLog(log, ["middle before leaf", "leaf body", "leaf defer", "middle defer", "catch"], "multi-frame unwind log");
}

function test_defer_registered_in_loop()
{
    printf("--- test_defer_registered_in_loop ---");

    var log = [];

    function body()
    {
        var i = 0;
        while (i < 3)
        {
            var value = i;
            defer
            {
                appendLog(log, value);
            }
            i++;
        }
        appendLog(log, "body");
        return 99;
    }

    var result = body();

    assertEqual(result, 99, "loop defer return value");
    assertLog(log, ["body", 2, 2, 2], "loop defer log");
}

function test_defer_can_call_helper_function()
{
    printf("--- test_defer_can_call_helper_function ---");

    var log = [];

    function appendValue(value)
    {
        appendLog(log, value);
    }

    function body()
    {
        defer appendValue("helper defer");
        appendLog(log, "body");
        return 123;
    }

    var result = body();

    assertEqual(result, 123, "helper defer return value");
    assertLog(log, ["body", "helper defer"], "helper defer log");
}

function test_defer_inside_defer()
{
    printf("--- test_defer_inside_defer ---");

    var log = [];

    function body()
    {
        defer
        {
            defer
            {
                appendLog(log, "nested defer");
            }
            appendLog(log, "outer defer body");
        }

        appendLog(log, "body");
        return 7;
    }

    var result = body();

    assertEqual(result, 7, "defer inside defer return value");
    assertLog(log, ["body", "outer defer body", "nested defer"], "defer inside defer log");
}

function test_defer_does_not_run_before_registration()
{
    printf("--- test_defer_does_not_run_before_registration ---");

    var log = [];

    function body(flag)
    {
        if (flag)
        {
            return 1;
        }

        defer
        {
            appendLog(log, "unregistered defer");
        }
        return 2;
    }

    var result = body(true);

    assertEqual(result, 1, "unregistered defer return value");
    assertLog(log, [], "unregistered defer log");
}

function test_defer_runs_on_runtime_error_current_frame()
{
    printf("--- test_defer_runs_on_runtime_error_current_frame ---");

    var log = [];

    function body()
    {
        defer
        {
            appendLog(log, "defer1");
        }
        defer
        {
            appendLog(log, "defer2");
        }
        appendLog(log, "before error");
        var a = 1;
        var value = a / 0;
        appendLog(log, value);
    }

    var ok, err = pcall(body);

    assertEqual(ok != 0, true, "runtime error is caught by pcall");
    assertEqual(string.len(err) > 0, true, "runtime error has error desc");
    assertLog(log, ["before error", "defer2", "defer1"], "runtime error current frame log");
}

function test_defer_runs_on_runtime_error_multiple_frames()
{
    printf("--- test_defer_runs_on_runtime_error_multiple_frames ---");

    var log = [];

    function leaf()
    {
        defer
        {
            appendLog(log, "leaf defer");
        }
        appendLog(log, "leaf before error");
         var a = 1;
        var value = a / 0;
        appendLog(log, value);
    }

    function middle()
    {
        defer
        {
            appendLog(log, "middle defer");
        }
        appendLog(log, "middle before leaf");
        leaf();
        appendLog(log, "middle after leaf");
    }

    var ok, err = pcall(middle);

    assertEqual(ok != 0, true, "multi-frame runtime error is caught by pcall");
    assertEqual(string.len(err) > 0, true, "multi-frame runtime error has error desc");
    assertLog(log, ["middle before leaf", "leaf before error", "leaf defer", "middle defer"], "runtime error multi-frame log");
}

function test_defer_runtime_error_does_not_run_before_registration()
{
    printf("--- test_defer_runtime_error_does_not_run_before_registration ---");

    var log = [];

    function body()
    {
        appendLog(log, "before error");
        var value = 1 / 0;
        appendLog(log, value);

        defer
        {
            appendLog(log, "unregistered defer");
        }
    }

    var ok, err = pcall(body);

    assertEqual(ok != 0, true, "runtime error before defer is caught by pcall");
    assertEqual(string.len(err) > 0, true, "runtime error before defer has error desc");
    assertLog(log, ["before error"], "runtime error unregistered defer log");
}

function test_defer_error_does_not_skip_remaining_defers_on_return()
{
    printf("--- test_defer_error_does_not_skip_remaining_defers_on_return ---");

    var log = [];

    function body()
    {
        defer
        {
            appendLog(log, "defer1");
        }
        defer
        {
            appendLog(log, "defer2 before error");
            var a = 1;
            var value = a / 0;
            appendLog(log, value);
        }
        defer
        {
            appendLog(log, "defer3");
        }

        appendLog(log, "body");
        return 66;
    }

    var ok, err = pcall(body);

    assertEqual(ok != 0, true, "defer error on return is caught by pcall");
    assertEqual(string.len(err) > 0, true, "defer error on return has error desc");
    assertLog(log, ["body", "defer3", "defer2 before error", "defer1"], "defer error on return log");
}

function test_defer_error_on_return_unwinds_parent_defers()
{
    printf("--- test_defer_error_on_return_unwinds_parent_defers ---");

    var log = [];

    function child()
    {
        defer
        {
            appendLog(log, "child defer before error");
            var a = 1;
            var value = a / 0;
            appendLog(log, value);
        }

        appendLog(log, "child body");
        return 1;
    }

    function parent()
    {
        defer
        {
            appendLog(log, "parent defer");
        }

        appendLog(log, "parent before child");
        child();
        appendLog(log, "parent after child");
    }

    var ok, err = pcall(parent);

    assertEqual(ok != 0, true, "defer error on return unwinds to pcall");
    assertEqual(string.len(err) > 0, true, "defer error on return unwind has error desc");
    assertLog(log, ["parent before child", "child body", "child defer before error", "parent defer"], "defer error on return unwinds parent defer");
}

function test_defer_error_does_not_skip_remaining_defers_on_runtime_error()
{
    printf("--- test_defer_error_does_not_skip_remaining_defers_on_runtime_error ---");

    var log = [];

    function body()
    {
        defer
        {
            appendLog(log, "defer1");
        }
        defer
        {
            appendLog(log, "defer2 before error");
            var value = 1 / 0;
            appendLog(log, value);
        }
        defer
        {
            appendLog(log, "defer3");
        }

        appendLog(log, "body before error");
        var value = 1 / 0;
        appendLog(log, value);
    }

    var ok, err = pcall(body);

    assertEqual(ok != 0, true, "runtime error with defer error is caught by pcall");
    assertEqual(string.len(err) > 0, true, "runtime error with defer error has error desc");
    assertLog(log, ["body before error", "defer3", "defer2 before error", "defer1"], "defer error during runtime unwind log");
}

function run_all()
{
    test_defer_runs_on_normal_exit();
    test_defer_lifo_order();
    test_defer_runs_on_early_return();
    test_defer_block_statement();
    test_defer_captures_latest_value();
    test_defer_nested_functions_are_independent();
    test_defer_runs_during_exception_unwind();
            
    test_defer_unwinds_multiple_lua_frames();
    
    test_defer_registered_in_loop();
    test_defer_can_call_helper_function();
    
    test_defer_inside_defer();
    
    test_defer_does_not_run_before_registration();
    
    test_defer_runs_on_runtime_error_current_frame();
    
    test_defer_runs_on_runtime_error_multiple_frames();
    
    test_defer_runtime_error_does_not_run_before_registration();
    
    test_defer_error_does_not_skip_remaining_defers_on_return();
    
    test_defer_error_on_return_unwinds_parent_defers();
    
    test_defer_error_does_not_skip_remaining_defers_on_runtime_error();
    
    printf("--- defer tests done ---");
}

run_all();