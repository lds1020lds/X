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

function makePairs(count)
{
    return
    {
        __iterator__ = function(self)
        {
            var index = 0;

            return
            {
                __next__ = function(t)
                {
                    if (index < count)
                    {
                        index++;
                        return 1, index, index * 10;
                    }

                    return 0;
                }
            };
        }
    };
}

function test_list_comprehension_basic()
{
    printf("--- test_list_comprehension_basic ---");

    var nums = [1, 2, 3, 4];
    var values = [x * x + 1 iterator(x in nums)];

    assertEqual(list.size(values), 4, "list comprehension size");
    assertEqual(values[0], 2, "list comprehension value 0");
    assertEqual(values[1], 5, "list comprehension value 1");
    assertEqual(values[2], 10, "list comprehension value 2");
    assertEqual(values[3], 17, "list comprehension value 3");
}

function test_list_comprehension_guard()
{
    printf("--- test_list_comprehension_guard ---");

    var nums = [1, 2, 3, 4, 5, 6];
    var values = [x * x + 2 iterator(x in nums) if x % 2 == 1];

    assertEqual(list.size(values), 3, "guarded list comprehension size");
    assertEqual(values[0], 3, "guarded list comprehension value 0");
    assertEqual(values[1], 11, "guarded list comprehension value 1");
    assertEqual(values[2], 27, "guarded list comprehension value 2");
}

function test_table_comprehension_basic()
{
    printf("--- test_table_comprehension_basic ---");

    var nums = [1, 2, 3, 4, 5, 6];
    var tableValues = {(toString(x * 2)) = x * x * x iterator(x in nums) if x % 2 == 0};

    assertEqual(tableValues["4"], 8, "table comprehension key 4");
    assertEqual(tableValues["8"], 64, "table comprehension key 8");
    assertEqual(tableValues["12"], 216, "table comprehension key 12");
    assertEqual(table.contains(tableValues, "2"), 0, "table comprehension guard skips odd 1");
    assertEqual(table.contains(tableValues, "6"), 0, "table comprehension guard skips odd 3");
    assertEqual(table.contains(tableValues, "10"), 0, "table comprehension guard skips odd 5");
}

function test_comprehension_multi_iterator_values()
{
    printf("--- test_comprehension_multi_iterator_values ---");

    var values = [k + v iterator(k, v in makePairs(3)) if k != 2];
    var tableValues = {(toString(k)) = v + 1 iterator(k, v in makePairs(3)) if v >= 20};

    assertEqual(list.size(values), 2, "multi iterator list size");
    assertEqual(values[0], 11, "multi iterator list first");
    assertEqual(values[1], 33, "multi iterator list second");

    assertEqual(table.contains(tableValues, "1"), 0, "multi iterator table guard skips first");
    assertEqual(tableValues["2"], 21, "multi iterator table key 2");
    assertEqual(tableValues["3"], 31, "multi iterator table key 3");
}

function test_comprehension_scope_does_not_leak()
{
    printf("--- test_comprehension_scope_does_not_leak ---");

    var x = 100;
    var nums = [1, 2, 3];
    var values = [x + 10 iterator(x in nums)];

    assertEqual(values[0], 11, "inner iterator variable first value");
    assertEqual(values[1], 12, "inner iterator variable second value");
    assertEqual(values[2], 13, "inner iterator variable third value");
    assertEqual(x, 100, "iterator variable does not leak to outer scope");
}

function test_normal_initializers_still_work()
{
    printf("--- test_normal_initializers_still_work ---");

    var nums = [1 + 2, 3 * 4, 5];
    var tableValues = {name = "xscript", ("k" $ "ey") = 7, 9};

    assertEqual(list.size(nums), 3, "normal list initializer size");
    assertEqual(nums[0], 3, "normal list initializer value 0");
    assertEqual(nums[1], 12, "normal list initializer value 1");
    assertEqual(nums[2], 5, "normal list initializer value 2");

    assertEqual(tableValues.name, "xscript", "normal table named field");
    assertEqual(tableValues["key"], 7, "normal table computed key");
    assertEqual(tableValues[0], 9, "normal table array-style value");
}

function test_nested_comprehension_record_tokens()
{
    printf("--- test_nested_comprehension_record_tokens ---");

    var nums = [1, 2, 3];
    var rows = {(toString(x)) = [y + x iterator(y in nums) if y <= x] iterator(x in nums)};
    var tables = {(toString(x)) = {(toString(y)) = x * 10 + y iterator(y in nums) if y != 2} iterator(x in nums)};

    assertEqual(list.size(rows["1"]), 1, "nested table/list comprehension row 1 size");
    assertEqual(rows["1"][0], 2, "nested table/list comprehension row 1 value");
    assertEqual(list.size(rows["3"]), 3, "nested table/list comprehension row 3 size");
    assertEqual(rows["3"][2], 6, "nested table/list comprehension row 3 value");

    assertEqual(tables["1"]["1"], 11, "nested table/table comprehension first value");
    assertEqual(table.contains(tables["1"], "2"), 0, "nested table/table comprehension guard skips value");
    assertEqual(tables["3"]["3"], 33, "nested table/table comprehension last value");
}

function test_deep_nested_list_comprehensions()
{
    printf("--- test_deep_nested_list_comprehensions ---");

    var nums = [1, 2, 3, 4];
    var cube = [[[x * 100 + y * 10 + z iterator(z in nums) if z <= y] iterator(y in nums) if y <= x] iterator(x in nums) if x >= 2];

    assertEqual(list.size(cube), 3, "deep nested list outer size");
    assertEqual(list.size(cube[0]), 2, "deep nested list x=2 middle size");
    assertEqual(list.size(cube[0][0]), 1, "deep nested list x=2 y=1 inner size");
    assertEqual(cube[0][0][0], 211, "deep nested list first value");
    assertEqual(cube[0][1][1], 222, "deep nested list x=2 y=2 last value");
    assertEqual(list.size(cube[2]), 4, "deep nested list x=4 middle size");
    assertEqual(cube[2][3][3], 444, "deep nested list last value");
}

function test_deep_nested_table_list_comprehensions()
{
    printf("--- test_deep_nested_table_list_comprehensions ---");

    var nums = [1, 2, 3, 4];
    var grouped = {(toString(x)) = {(toString(y)) = [x * 100 + y * 10 + z iterator(z in nums) if z <= y] iterator(y in nums) if y <= x} iterator(x in nums) if x != 2};

    assertEqual(table.contains(grouped, "2"), 0, "deep nested table skips outer x=2");
    assertEqual(list.size(grouped["1"]["1"]), 1, "deep nested table/list x=1 y=1 size");
    assertEqual(grouped["1"]["1"][0], 111, "deep nested table/list x=1 y=1 value");
    assertEqual(list.size(grouped["3"]["2"]), 2, "deep nested table/list x=3 y=2 size");
    assertEqual(grouped["3"]["2"][1], 322, "deep nested table/list x=3 y=2 value");
    assertEqual(list.size(grouped["4"]["4"]), 4, "deep nested table/list x=4 y=4 size");
    assertEqual(grouped["4"]["4"][3], 444, "deep nested table/list x=4 y=4 value");
}

function test_nested_comprehensions_inside_normal_initializers()
{
    printf("--- test_nested_comprehensions_inside_normal_initializers ---");

    var nums = [1, 2, 3, 4];
    var mixed = [
        {name = "first", values = [x * 10 iterator(x in nums) if x <= 2]},
        {name = "second", values = {(toString(x)) = [x + y iterator(y in nums) if y < x] iterator(x in nums) if x >= 3}}
    ];
    var indexed = {("row" $ toString(list.size([y iterator(y in nums) if y <= x]))) = {total = list.size([z iterator(z in nums) if z <= x]), values = [x + v iterator(v in nums) if v < x]} iterator(x in nums) if x >= 2};

    assertEqual(mixed[0].name, "first", "normal list contains table field");
    assertEqual(list.size(mixed[0].values), 2, "normal list nested values size");
    assertEqual(mixed[0].values[1], 20, "normal list nested values item");
    assertEqual(mixed[1].values["3"][1], 5, "normal initializer nested table/list value");
    assertEqual(list.size(mixed[1].values["4"]), 3, "normal initializer nested table/list size");

    assertEqual(indexed["row2"].total, 2, "computed key with nested list size total");
    assertEqual(list.size(indexed["row3"].values), 2, "computed key nested values size");
    assertEqual(indexed["row4"].values[2], 7, "computed key nested values last");
}

function run_all()
{
    test_list_comprehension_basic();
    test_list_comprehension_guard();
    test_table_comprehension_basic();
    test_comprehension_multi_iterator_values();
    test_comprehension_scope_does_not_leak();
    test_normal_initializers_still_work();
    test_nested_comprehension_record_tokens();
    test_deep_nested_list_comprehensions();
    test_deep_nested_table_list_comprehensions();
    test_nested_comprehensions_inside_normal_initializers();

    printf("--- comprehension tests done ---");
}

run_all();
