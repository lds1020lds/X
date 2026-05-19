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

function failWhenCalled()
{
    throw "right operand should not be evaluated";
}

function countAndReturn(value)
{
    gCallCount++;
    return value;
}

function test_null_coalescing_basic()
{
    printf("--- test_null_coalescing_basic ---");

    var missing = nil;
    var value = "ready";

    assertEqual(missing ?? "fallback", "fallback", "nil left operand should use fallback");
    assertEqual(value ?? "fallback", "ready", "non-nil left operand should be preserved");
    assertEqual(nil ?? 1 + 2 * 3, 7, "right operand should be a full expression");
}

function test_null_coalescing_keeps_false_like_values()
{
    printf("--- test_null_coalescing_keeps_false_like_values ---");

    assertEqual(0 ?? 100, 0, "zero should not trigger null coalescing fallback");
    assertEqual("" ?? "fallback", "", "empty string should not trigger null coalescing fallback");

    assertEqual(0 || 100, 100, "logical-or still treats zero as false");
    assertEqual("" || "fallback", "fallback", "logical-or still treats empty string as false");
}

function test_null_coalescing_short_circuit()
{
    printf("--- test_null_coalescing_short_circuit ---");

    gCallCount = 0;
    assertEqual("left" ?? countAndReturn("right"), "left", "non-nil left operand should short-circuit right operand");
    assertEqual(gCallCount, 0, "right operand should not be evaluated when left is non-nil");

    gCallCount = 0;
    assertEqual(nil ?? countAndReturn("right"), "right", "nil left operand should evaluate right operand");
    assertEqual(gCallCount, 1, "right operand should be evaluated once when left is nil");

    assertEqual(123 ?? failWhenCalled(), 123, "short-circuit should avoid right operand runtime error");
}

function test_null_coalescing_chain_and_precedence()
{
    printf("--- test_null_coalescing_chain_and_precedence ---");

    assertEqual(nil ?? nil ?? "last", "last", "chained null coalescing should continue until first non-nil value");
    assertEqual("first" ?? nil ?? "last", "first", "chained null coalescing should keep first non-nil value");

    assertEqual(nil ?? 1 + 2, 3, "arithmetic should bind tighter than null coalescing on right side");
    assertEqual((nil ?? 1) + 2, 3, "parenthesized null coalescing result should compose with arithmetic");

    assertEqual(0 || "or-fallback" ?? "coalesce-fallback", "or-fallback", "logical-or should bind tighter than null coalescing");
    assertEqual(5 || "or-fallback" ?? "coalesce-fallback", 5, "logical-or result should be used as null coalescing left operand");

    assertEqual(nil ?? 0 ? "true" or "false", "false", "null coalescing should bind tighter than conditional expression");
    assertEqual(nil ?? 1 ? "true" or "false", "true", "conditional expression should see null coalescing result");
}

function test_null_coalescing_optional_chain()
{
    printf("--- test_null_coalescing_optional_chain ---");

    var user = nil;
    assertEqual(user?.profile?.name ?? "anonymous", "anonymous", "optional chain nil should be replaced by null coalescing fallback");

    user = {profile = {name = ""}};
    assertEqual(user?.profile?.name ?? "anonymous", "", "empty string from optional chain should be preserved");

    user = {profile = {name = "tom"}};
    assertEqual(user?.profile?.name ?? "anonymous", "tom", "existing optional chain value should be preserved");
}

function run_all()
{
    test_null_coalescing_basic();
    test_null_coalescing_keeps_false_like_values();
    test_null_coalescing_short_circuit();
    test_null_coalescing_chain_and_precedence();
    test_null_coalescing_optional_chain();

    printf("--- null coalescing tests done ---");
}

run_all();
