require("comprehensive_tests/test_helper.xs");

var a = { }

var b = a?.b?() ?? 100

function test_match_constant_and_multi_case()
{
    printf("--- test_match_constant_and_multi_case ---");

    var result = "none";
    var value = 5;

    match value
    {
        1, 2, 3 =>
        {
            result = "small";
        }

        4, 5, 6 =>
        {
            result = "middle";
        }

        "5" =>
        {
            result = "string-five";
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "middle", "match multi constant hits second group");

    result = "none";
    value = "5";
    match value
    {
        5 =>
        {
            result = "number-five";
        }

        "5" =>
        {
            result = "string-five";
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "string-five", "match int and string constants should not be equal");
}

function test_match_order_and_default()
{
    printf("--- test_match_order_and_default ---");

    var result = "none";
    var value = 999;

    match value
    {
        _ =>
        {
            result = "default-first";
        }

        x if (x > 100) =>
        {
            result = "guard-after-default";
        }
    }

    assertEqual(result, "default-first", "match default branch should short-circuit later cases");

    result = "none";
    match value
    {
        x if (x > 1000) =>
        {
            result = "too-large";
        }

        x if (x > 100) =>
        {
            result = "large";
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "large", "match guard false should continue to next guard");
}

function test_match_list_basic()
{
    printf("--- test_match_list_basic ---");

    var result = "none";
    var a = [123, 466, 100];

    match a
    {
        [x, y, 100] =>
        {
            result = toString(x) $ ":" $ toString(y);
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "123:466", "match list destructuring binds x and y");

    result = "none";
    var b = [123, 466, 101];
    match b
    {
        [x, y, 100] =>
        {
            result = "wrong";
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "default", "match list constant mismatch falls through");
}

function test_match_list_length_boundary()
{
    printf("--- test_match_list_length_boundary ---");

    var result = "none";
    var extra = [1, 2, 100, 999];

    match extra
    {
        [x, y, 100] =>
        {
            result = "matched-prefix";
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "default", "match list pattern should not match list with extra elements unless prefix match is intended");

    result = "none";
    var shortList = [1, 2];
    match shortList
    {
        [x, y, 100] =>
        {
            result = "wrong";
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "default", "match list pattern should not match missing element");
}

function test_match_nested_list_table()
{
    printf("--- test_match_nested_list_table ---");

    var result = "none";
    var data = [{kind = "point", pos = [10, 20]}, 100];

    match data
    {
        [{kind: "point", pos: [x, y]}, 100] =>
        {
            result = toString(x) $ "," $ toString(y);
        }
        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "10,20", "match nested table/list destructuring");
}

function test_match_table_basic_and_partial_boundary()
{
    printf("--- test_match_table_basic_and_partial_boundary ---");

    var result = "none";
    var data = {name = "tom", age = 18, tag = "vip"};

    match data
    {
        {name: who, age: 18} =>
        {
            result = who;
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "tom", "match table field binding and constant field");

    result = "none";
    match data
    {
        {name: "tom"} =>
        {
            result = "matched-partial";
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "matched-partial", "match table pattern currently behaves as partial field match");
}

function test_match_structure_guard()
{
    printf("--- test_match_structure_guard ---");

    var result = "none";
    var data = [20, 5, 100];

    match data
    {
        [x, y, 100] if (x < y) =>
        {
            result = "wrong";
        }

        [x, y, 100] if (x > y) =>
        {
            result = "right";
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "right", "match structure guard false should continue to next case");
}

function test_match_var_scope()
{
    printf("--- test_match_var_scope ---");

    var x = "outer";
    var result = "none";

    match [7, 8, 100]
    {
        [x, y, 100] =>
        {
            result = toString(x) $ ":" $ toString(y);
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "7:8", "match branch binding should be usable inside branch");
    assertEqual(x, "outer", "match branch binding should not overwrite outer variable after branch");
}

function test_match_guard_runtime_error()
{
    printf("--- test_match_guard_runtime_error ---");

    var ok, err = pcall(function()
    {
        var result = "none";
        match "abc"
        {
            x if (x > 1000) =>
            {
                result = "large";
            }

            _ =>
            {
                result = "default";
            }
        }
        return result;
    });

    assert(ok != 0, "match guard type error should be reported instead of silently falling through");
}

function test_match_list_wildcard_pattern_compile()
{
    printf("--- test_match_list_wildcard_pattern_compile ---");

    var ok, fn = loadstring("var r = \"none\"; var a = [1, 2, 3]; match a { [1, _, 3] => { r = \"hit\"; } _ => { r = \"default\"; } } return r;");
    assertEqual(ok, 1, "match list '_' wildcard pattern should compile");

    if (ok == 1)
    {
        assertEqual(fn(), "hit", "match list '_' wildcard pattern should match non-nil element");
    }
}

function test_match_complex_regression()
{
    test_match_constant_and_multi_case();
    test_match_order_and_default();
    test_match_list_basic();
    test_match_list_length_boundary();
    test_match_nested_list_table();
    test_match_table_basic_and_partial_boundary();
    test_match_structure_guard();
    test_match_var_scope();
    test_match_guard_runtime_error();
    test_match_list_wildcard_pattern_compile();
}

test_match_complex_regression();
summary();
return 0;
