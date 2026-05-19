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

function test_switch_int_match()
{
    printf("--- test_switch_int_match ---");

    var a = 2;
    var result = "";

    switch (a)
    {
        case 1:
            result = "one";
            break;
        case 2:
            result = "two";
            break;
        default:
            result = "default";
            break;
    }

    assertEqual(result, "two", "int case match");
}

function test_switch_float_match()
{
    printf("--- test_switch_float_match ---");

    var a = 2.5;
    var result = "";

    switch (a)
    {
        case 1:
            result = "one";
            break;
        case 2.5:
            result = "two point five";
            break;
        default:
            result = "default";
            break;
    }

    assertEqual(result, "two point five", "float case match");
}

function test_switch_string_match()
{
    printf("--- test_switch_string_match ---");

    var a = "SS";
    var result = "";

    switch (a)
    {
        case "aa":
            result = "aa";
            break;
        case "ss":
        case "SS":
            result = "ss";
            break;
        default:
            result = "default";
            break;
    }

    assertEqual(result, "ss", "string case match and stacked cases");
}

function test_switch_string_intern_match()
{
    printf("--- test_switch_string_intern_match ---");

    var a = "S" $ "S";
    var result = "";

    switch (a)
    {
        case "SS":
            result = "intern matched";
            break;
        default:
            result = "default";
            break;
    }

    assertEqual(result, "intern matched", "dynamic string should match string case through interned XString pointer");
}

function test_switch_default_match()
{
    printf("--- test_switch_default_match ---");

    var a = "missing";
    var result = "";

    switch (a)
    {
        case 1:
            result = "one";
            break;
        case "ss":
            result = "ss";
            break;
        default:
            result = "default";
            break;
    }

    assertEqual(result, "default", "default case match");
}

function test_switch_no_default_no_match()
{
    printf("--- test_switch_no_default_no_match ---");

    var a = 99;
    var result = "unchanged";

    switch (a)
    {
        case 1:
            result = "one";
            break;
        case 2:
            result = "two";
            break;
    }

    assertEqual(result, "unchanged", "no default and no matched case should skip switch body");
}

function test_switch_fallthrough_without_break()
{
    printf("--- test_switch_fallthrough_without_break ---");

    var a = 1;
    var result = "";

    switch (a)
    {
        case 1:
            result = result $ "one";
        case 2:
            result = result $ ",two";
            break;
        default:
            result = result $ ",default";
            break;
    }

    assertEqual(result, "one,two", "case without break falls through to next case");
}

function test_switch_block_body()
{
    printf("--- test_switch_block_body ---");

    var a = 3;
    var result = 0;

    switch (a)
    {
        case 3:
        {
            var localValue = 10;
            result = localValue + 5;
        }
            break;
        default:
            result = -1;
            break;
    }

    assertEqual(result, 15, "case block supports multiple statements");
}

function test_nested_switch()
{
    printf("--- test_nested_switch ---");

    var outer = "a";
    var inner = 2;
    var result = "";

    switch (outer)
    {
        case "a":
        {
            switch (inner)
            {
                case 1:
                    result = "a1";
                    break;
                case 2:
                    result = "a2";
                    break;
                default:
                    result = "a-default";
                    break;
            }
        }
            break;
        default:
            result = "outer-default";
            break;
    }

    assertEqual(result, "a2", "nested switch case match");
}

function run_all()
{
    test_switch_int_match();
    test_switch_float_match();
    test_switch_string_match();
    test_switch_string_intern_match();
    test_switch_default_match();
    test_switch_no_default_no_match();
    test_switch_fallthrough_without_break();
    test_switch_block_body();
    test_nested_switch();

    printf("--- switch case tests done ---");
}

run_all();
