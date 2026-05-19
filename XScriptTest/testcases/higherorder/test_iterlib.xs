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

var iter = require("libs/iter");

generator function genValues()
{
    emit 2;
    emit 4;
    emit 6;
}

function test_range_collect()
{
    printf("--- test_range_collect ---");

    var values = iter.collect(iter.range(1, 5, 1));

    assertEqual(list.size(values), 4, "range collect size");
    assertEqual(values[0], 1, "range collect 0");
    assertEqual(values[3], 4, "range collect 3");
}

function test_range_desc_collect()
{
    printf("--- test_range_desc_collect ---");

    var values = iter.collect(iter.range(5, 1, -2));

    assertEqual(list.size(values), 2, "range desc size");
    assertEqual(values[0], 5, "range desc 0");
    assertEqual(values[1], 3, "range desc 1");
}

function test_map_list()
{
    printf("--- test_map_list ---");

    var values = iter.map([1, 2, 3], function(v, k)
    {
        return v * 10;
    });

    assertEqual(list.size(values), 3, "map list size");
    assertEqual(values[0], 10, "map list 0");
    assertEqual(values[2], 30, "map list 2");
}

function test_filter_list()
{
    printf("--- test_filter_list ---");

    var values = iter.filter([1, 2, 3, 4, 5], function(v, k)
    {
        return v % 2 == 1;
    });

    assertEqual(list.size(values), 3, "filter list size");
    assertEqual(values[0], 1, "filter list 0");
    assertEqual(values[2], 5, "filter list 2");
}

function test_reduce_list()
{
    printf("--- test_reduce_list ---");

    var sum = iter.reduce([1, 2, 3, 4], function(acc, v, k)
    {
        return acc + v;
    }, 0);

    assertEqual(sum, 10, "reduce list sum");
}

function test_reduce_without_init()
{
    printf("--- test_reduce_without_init ---");

    var value = iter.reduce([2, 3, 4], function(acc, v, k)
    {
        return acc * v;
    });

    assertEqual(value, 24, "reduce without init");
}

function test_foreach_generator()
{
    printf("--- test_foreach_generator ---");

    var sum = 0;
    var count = iter.each(genValues(), function(v, k)
    {
        sum += v;
    });

    assertEqual(count, 3, "foreach generator count");
    assertEqual(sum, 12, "foreach generator sum");

    var aliasCount = iter["foreach"]([1, 2], function(v, k)
    {
        sum += v;
    });

    assertEqual(aliasCount, 2, "foreach alias count");
    assertEqual(sum, 15, "foreach alias sum");
}

function test_table_key_value()
{
    printf("--- test_table_key_value ---");

    var t = { a = 1, b = 2 };
    var values = iter.map(t, function(v, k)
    {
        return k $ ":" $ toString(v);
    });

    assertEqual(list.size(values), 2, "table map size");
}

function test_enumerate()
{
    printf("--- test_enumerate ---");

    var values = iter.enumerate(["a", "b"]);

    assertEqual(list.size(values), 2, "enumerate size");
    assertEqual(values[0].index, 0, "enumerate index 0");
    assertEqual(values[0].value, "a", "enumerate value 0");
    assertEqual(values[1].index, 1, "enumerate index 1");
    assertEqual(values[1].value, "b", "enumerate value 1");
}

function test_zip()
{
    printf("--- test_zip ---");

    var values = iter.zip([1, 2, 3], ["a", "b"]);

    assertEqual(list.size(values), 2, "zip shortest size");
    assertEqual(values[0].left, 1, "zip left 0");
    assertEqual(values[0].right, "a", "zip right 0");
    assertEqual(values[1].left, 2, "zip left 1");
    assertEqual(values[1].right, "b", "zip right 1");
}

function run_all()
{
    test_range_collect();
    test_range_desc_collect();
    test_map_list();
    test_filter_list();
    test_reduce_list();
    test_reduce_without_init();
    test_foreach_generator();
    test_table_key_value();
    test_enumerate();
    test_zip();

    printf("--- iterlib tests done ---");
}

run_all();
