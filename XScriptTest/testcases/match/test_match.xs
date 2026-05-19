function assertEqual(a, b, msg)
{
    if (a != b)
    {
        printf("FAIL", msg, "expected", b, "got", a);
        throw msg;
    }
    else
    {
        printf("OK", msg);
    }
}

function assertTrue(value, msg)
{
    if (!value)
    {
        printf("FAIL", msg);
        throw msg;
    }
    else
    {
        printf("OK", msg);
    }
}

function assertLoadFail(code, desc)
{
    var ok, fn = loadstring(code);
    assertEqual(ok, 0, desc);
}

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

    assertEqual(result, "matched-prefix", "match list pattern currently behaves as prefix element match");

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

    assertEqual(ok, -1, "match guard runtime type error should be reported instead of silently falling through");
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

function test_match_type_limit_fallthrough()
{
    printf("--- test_match_type_limit_fallthrough ---");

    var result = "none";
    var value = "hello";

    match value
    {
        x(number) if (x > 100) =>
        {
            result = "wrong-number";
        }

        x(string) if (string.len(x) == 5) =>
        {
            result = "string-five";
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "string-five", "type mismatch should fall through to next typed pattern");

    result = "none";
    value = 3.5;
    match value
    {
        x(int) =>
        {
            result = "int";
        }

        x(float) if (x > 3.0) =>
        {
            result = "float";
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "float", "int type limit should not match float, then float branch should match");
}

function test_match_number_mask_and_guard_order()
{
    printf("--- test_match_number_mask_and_guard_order ---");

    var result = "none";
    var value = 20;

    match value
    {
        x(number) if (x < 10) =>
        {
            result = "small";
        }

        x(number) if (x >= 10 && x < 100) =>
        {
            result = "middle";
        }

        x(string) =>
        {
            result = "wrong-string";
        }
    }

    assertEqual(result, "middle", "number type limit should include int and use each pattern guard independently");

    result = "none";
    value = 2.5;
    match value
    {
        x(number) if (x > 10) =>
        {
            result = "wrong-large";
        }

        x(number) if (x > 2.0) =>
        {
            result = "float-number";
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "float-number", "number type limit should include float and later guard should still run");
}

function test_match_underscore_type_limit()
{
    printf("--- test_match_underscore_type_limit ---");

    var result = "none";
    var value = "abc";

    match value
    {
        _(number) =>
        {
            result = "wrong-number";
        }

        _(string) =>
        {
            result = "string";
        }
    }

    assertEqual(result, "string", "typed wildcard should fall through when type does not match");
}

function test_match_multi_binding_same_names()
{
    printf("--- test_match_multi_binding_same_names ---");

    var result = "none";
    var data = ["bob", 35];

    match data
    {
        [name(string), age(number)] if (age < 18) =>
        {
            result = "minor:" $ name;
        }

        [name(string), age(number)] if (age >= 18) =>
        {
            result = "adult:" $ name $ ":" $ toString(age);
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "adult:bob:35", "multiple patterns with same binding names should share branch variables");

    result = "none";
    data = {name = "kid", age = 12};
    match data
    {
        {name: name(string), age: age(number)} if (age >= 18) =>
        {
            result = "adult";
        }

        {name: name(string), age: age(number)} if (age < 18) =>
        {
            result = "minor:" $ name;
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "minor:kid", "table patterns with same binding names and independent guards should work");
}

function test_match_nested_type_limit_and_fallback()
{
    printf("--- test_match_nested_type_limit_and_fallback ---");

    var result = "none";
    var data = {kind = "point", payload = [10, "20"]};

    match data
    {
        {kind: "point", payload: [x(number), y(number)]} =>
        {
            result = "wrong-both-number";
        }

        {kind: "point", payload: [x(number), y(string)]} if (string.len(y) == 2) =>
        {
            result = toString(x) $ ":" $ y;
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "10:20", "nested type mismatch should jump to next pattern without runtime error");
}

function test_match_typed_structure_runtime_safety()
{
    printf("--- test_match_typed_structure_runtime_safety ---");

    var result = "none";
    var data = {name = "tom", age = "old"};

    match data
    {
        {name: name(string), age: age(number)} if (age > 10) =>
        {
            result = "wrong-number-age";
        }

        {name: name(string), age: age(string)} if (string.len(age) == 3) =>
        {
            result = name $ ":" $ age;
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "tom:old", "field type limit should prevent guard runtime type error and continue matching");
}

function test_match_deep_mixed_patterns()
{
    printf("--- test_match_deep_mixed_patterns ---");

    var result = "none";
    var data = {
        kind = "event",
        payload = [
            {name = "click", score = 7.5},
            {name = "meta", tags = ["ui", "button"]}
        ]
    };

    match data
    {
        {kind: "event", payload: [{name: eventName(string), score: score(int)}, _]} =>
        {
            result = "wrong-int-score";
        }

        {kind: "event", payload: [{name: eventName(string), score: score(number)}, {tags: [firstTag(string), secondTag(string)]}]} if (score > 7 && secondTag == "button") =>
        {
            result = eventName $ ":" $ firstTag $ ":" $ secondTag $ ":" $ toString(score);
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "click:ui:button:7.500000", "deep nested mixed pattern should combine constants, type limits and guard");
}

function test_match_multiple_type_names()
{
    printf("--- test_match_multiple_type_names ---");

    var result = "none";
    var value = "ready";

    match value
    {
        x(int, float) =>
        {
            result = "number";
        }

        x(string, table) if (string.len(x) == 5) =>
        {
            result = "string-or-table";
        }

        _ =>
        {
            result = "default";
        }
    }

    assertEqual(result, "string-or-table", "type limit should allow multiple type names in one pattern");
}

function test_match_case_binding_order_invalid()
{
    printf("--- test_match_case_binding_order_invalid ---");

    assertLoadFail("match [1, 2] { [x, y], [x] => { } }", "multi pattern binding count mismatch should fail to compile");
    assertLoadFail("match [1, 2] { [x, y], [x, z] => { } }", "multi pattern binding names mismatch should fail to compile");
    assertLoadFail("match [1, 2] { [x, y], [y, x] => { } }", "multi pattern binding order mismatch should fail to compile");
    assertLoadFail("match 10 { x(number), y(number) => { } }", "top-level variable binding name mismatch should fail to compile");
    assertLoadFail("match 10 { 1, x(number) => { } }", "constant and variable pattern mixing in one branch should fail to compile");
    assertLoadFail("match 10 { x(unknown_type) => { } }", "unknown type limit should fail to compile");
}

function run_all()
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
    test_match_type_limit_fallthrough();
    test_match_number_mask_and_guard_order();
    test_match_underscore_type_limit();
    test_match_multi_binding_same_names();
    test_match_nested_type_limit_and_fallback();
    test_match_typed_structure_runtime_safety();
    test_match_deep_mixed_patterns();
    test_match_multiple_type_names();
    test_match_case_binding_order_invalid();

    printf("--- match tests done ---");
}

run_all();
