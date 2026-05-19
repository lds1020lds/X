require("comprehensive_tests/test_helper.xs");

function assertLoadFail(code, desc)
{
    var ok, fn = loadstring(code);
    assertEqual(ok, 0, desc);
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

function test_match_invalid_binding_sets()
{
    printf("--- test_match_invalid_binding_sets ---");

    assertLoadFail("match [1, 2] { [x, y], [x] => { } }", "multi pattern binding count mismatch should fail to compile");
    assertLoadFail("match [1, 2] { [x, y], [x, z] => { } }", "multi pattern binding names mismatch should fail to compile");
    assertLoadFail("match [1, 2] { [x, y], [y, x] => { } }", "multi pattern binding order mismatch should fail to compile");
    assertLoadFail("match 10 { x(number), y(number) => { } }", "top-level variable binding name mismatch should fail to compile");
    assertLoadFail("match 10 { 1, x(number) => { } }", "constant and variable pattern mixing in one branch should fail to compile");
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

function test_match_complex_regression()
{
    test_match_type_limit_fallthrough();
    test_match_number_mask_and_guard_order();
    test_match_underscore_type_limit();
    test_match_multi_binding_same_names();
    test_match_nested_type_limit_and_fallback();
    test_match_invalid_binding_sets();
    test_match_typed_structure_runtime_safety();
}

test_match_complex_regression();
summary();
return 0;