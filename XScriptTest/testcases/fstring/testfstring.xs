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

function assertTrue(value, msg)
{
    assertEqual(value, true, msg);
}

function makeName()
{
    return "Alice";
}

function add(a, b)
{
    return a + b;
}

function test_fstring_plain_text()
{
    printf("--- test_fstring_plain_text ---");

    assertEqual(f"hello", "hello", "plain f-string");
    assertEqual(f"hello world", "hello world", "plain f-string with spaces");
}

function test_fstring_basic_interpolation()
{
    printf("--- test_fstring_basic_interpolation ---");

    var name = "Bob";
    var age = 18;
    var enabled = true;

    assertEqual(f"name={name}", "name=Bob", "string variable interpolation");
    assertEqual(f"age={age}", "age=18", "integer variable interpolation");
    assertEqual(f"enabled={enabled}", "enabled=1", "bool variable interpolation");
    assertEqual(f"{name}", "Bob", "single expression only");
    assertEqual(f"prefix {name}", "prefix Bob", "leading literal plus expression");
    assertEqual(f"{name} suffix", "Bob suffix", "expression plus trailing literal");
}

function test_fstring_multiple_interpolations()
{
    printf("--- test_fstring_multiple_interpolations ---");

    var first = "A";
    var second = "B";
    var third = "C";

    assertEqual(f"{first}-{second}-{third}", "A-B-C", "multiple adjacent interpolation with separators");
    assertEqual(f"[{first}][{second}][{third}]", "[A][B][C]", "multiple interpolation with literals");
    assertEqual(f"{first}{second}{third}", "ABC", "adjacent interpolation without literals");
}

function test_fstring_expression_interpolation()
{
    printf("--- test_fstring_expression_interpolation ---");

    var a = 3;
    var b = 4;

    assertEqual(f"sum={a + b}", "sum=7", "addition expression");
    assertEqual(f"mul={a * b}", "mul=12", "multiplication expression");
    assertEqual(f"paren={(a + b) * 2}", "paren=14", "parenthesized expression");
    assertEqual(f"compare={a < b}", "compare=1", "comparison expression");
    assertEqual(f"logic={(a < b) && (b > 0)}", "logic=1", "logic expression");
}

function test_fstring_function_call_interpolation()
{
    printf("--- test_fstring_function_call_interpolation ---");

    assertEqual(f"name={makeName()}", "name=Alice", "function call interpolation");
    assertEqual(f"add={add(10, 20)}", "add=30", "function call with arguments");
    assertEqual(f"nested={add(add(1, 2), add(3, 4))}", "nested=10", "nested function call interpolation");
}

function test_fstring_nested_interpolation()
{
    printf("--- test_fstring_nested_interpolation ---");

    var name = "Bob";
    var age = 18;
    var left = "A";
    var right = "B";
    var list = ["zero", "one"];

    assertEqual(f"outer={f\"name={name}\"}", "outer=name=Bob", "nested f-string as interpolation expression");
    assertEqual(f"pair={f\"{left}-{right}\"}", "pair=A-B", "nested f-string with multiple interpolations");
    assertEqual(f"user={f\"{name}:{age}\"}", "user=Bob:18", "nested f-string with mixed value types");
    assertEqual(f"list={f\"{list[0]}|{list[1]}\"}", "list=zero|one", "nested f-string with index access");
    assertEqual(f"wrapped={{ {f\"name={name}\"} }}", "wrapped={ name=Bob }}", "nested f-string with escaped open brace");
}

function test_fstring_access_interpolation()
{
    printf("--- test_fstring_access_interpolation ---");

    var list = ["zero", "one", "two"];
    var obj = {name = "box", value = 42, child = {tag = "leaf"}};

    assertEqual(f"list0={list[0]}", "list0=zero", "list index interpolation");
    assertEqual(f"list2={list[2]}", "list2=two", "list index interpolation second value");
    assertEqual(f"obj={obj.name}:{obj.value}", "obj=box:42", "table field interpolation");
    assertEqual(f"child={obj.child.tag}", "child=leaf", "nested table field interpolation");
}

function test_fstring_escaped_braces()
{
    printf("--- test_fstring_escaped_braces ---");

    var name = "Bob";

    assertEqual(f"value={{ {name} }}", "value={ Bob }}", "escaped open brace around interpolation");
    assertEqual(f"map={{key:{name}}}", "map={key:Bob}}", "escaped open brace with interpolation content");
}

function test_fstring_in_assignment_and_loop()
{
    printf("--- test_fstring_in_assignment_and_loop ---");

    var s = f"start";
    s = f"{s}-next";
    assertEqual(s, "start-next", "assignment from f-string");

    var list = [];
    var i = 0;
    while (i < 3)
    {
        list:append(f"item-{i}");
        i++;
    }

    assertEqual(list:size(), 3, "loop generated list size");
    assertEqual(list[0], "item-0", "loop item 0");
    assertEqual(list[1], "item-1", "loop item 1");
    assertEqual(list[2], "item-2", "loop item 2");
}

function test_fstring_empty_and_unmatched_expression_edges()
{
    printf("--- test_fstring_empty_and_unmatched_expression_edges ---");

    assertEqual(f"before{}after", "beforeafter", "empty interpolation is ignored");
    assertEqual(f"before{after", "before{after", "unmatched open brace falls back to literal text");
    assertEqual(f"before}after", "before}after", "unmatched close brace remains literal text");
}

function test_fstring_with_pcall()
{
    printf("--- test_fstring_with_pcall ---");

    var ok, value = pcall(function()
    {
        var x = 10;
        return f"x={x}";
    });

    assertEqual(ok, 0, "pcall f-string succeeds");
    assertEqual(value, "x=10", "pcall f-string return value");
}

function run_all()
{
    test_fstring_plain_text();
    test_fstring_basic_interpolation();
    test_fstring_multiple_interpolations();
    test_fstring_expression_interpolation();
    test_fstring_function_call_interpolation();
    test_fstring_nested_interpolation();
    test_fstring_access_interpolation();
    test_fstring_escaped_braces();
    test_fstring_in_assignment_and_loop();
    test_fstring_empty_and_unmatched_expression_edges();
    test_fstring_with_pcall();

    printf("--- f-string tests done ---");
}

run_all();
