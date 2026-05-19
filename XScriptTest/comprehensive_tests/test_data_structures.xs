// ============================================================
// XScript 综合测试 - 数据结构
// 覆盖：表(Table)、列表(List)、表构造器、表操作、集合
// ============================================================

// ============================================================
// 14. 表（Table）
// ============================================================
function test_table()
{
    printf("--- test_table ---");

    // 字面量创建
    var t = {a=1, b=2, c=3};
    assertEqual(t.a, 1, "table dot access");
    assertEqual(t["b"], 2, "table bracket access");

    // 赋值
    t.d = 4;
    assertEqual(t.d, 4, "table assign new key");

    t["e"] = 5;
    assertEqual(t["e"], 5, "table assign bracket");

    // 数值键
    var arr = {};
    arr[1] = 10;
    arr[2] = 20;
    arr[3] = 30;
    assertEqual(arr[1], 10, "table numeric key");

    // 嵌套表
    var outer = {inner = {val = 99}};
    assertEqual(outer.inner.val, 99, "nested table");

    // 链式访问
    outer.inner.val = 100;
    assertEqual(outer.inner.val, 100, "nested table assign");

    // 表构造器数组部分
    var list = {10, 20, 30};
    assertEqual(list[0], 10, "table array 0");
    assertEqual(list[1], 20, "table array 1");
    assertEqual(list[2], 30, "table array 2");

    // type
    assertEqual(type(t), "table", "type table");
}

// ============================================================
// 15. 列表（List）
// ============================================================
function test_list()
{
    printf("--- test_list ---");

    var a = [1, 2, 3, 4, 5];
    assertEqual(a[0], 1, "list index 0");
    assertEqual(a[4], 5, "list index 4");

    // 列表方法 (冒号调用)
    a:append(6);
    assertEqual(a[5], 6, "list append");

    // 列表大小
    assertEqual(list.size(a), 6, "list size");

    // 列表排序
    var b = [3, 1, 4, 1, 5, 9, 2, 6];
    b:sort();
    assertEqual(b[0], 1, "list sort first");
    assertEqual(b[7], 9, "list sort last");

    // 反转
    var c = [1, 2, 3];
    c:reverse();
    assertEqual(c[0], 3, "list reverse");
    assertEqual(c[2], 1, "list reverse last");

    // 列表遍历
    var sum = 0;
    foreach (key, value in lenum(c))
    {
        sum += value;
    }
    assertEqual(sum, 6, "list foreach sum");

    // 列表 insert
    var d = [10, 20, 30];
    d:insert(1, 15);
    assertEqual(d[1], 15, "list insert");

    // 列表 removeAt
    d:removeAt(1);
    assertEqual(d[1], 20, "list removeAt");

    // 列表 count
    var e = [1, 2, 1, 3, 1];
    assertEqual(list.count(e, 1), 3, "list count");
}

// ============================================================
// 31. 表构造器复杂情况
// ============================================================
function test_table_constructor()
{
    printf("--- test_table_constructor ---");

    // 混合键值对和数组
    var t = {10, 20, 30, name="test", value=42};
    assertEqual(t[0], 10, "mixed table 0");
    assertEqual(t.name, "test", "mixed table named");
    assertEqual(t.value, 42, "mixed table value");

    // 空表
    var empty = {};
    assertEqual(type(empty), "table", "empty table type");

    // 嵌套构造
    var nested = {a = {b = {c = "deep"}}};
    assertEqual(nested.a.b.c, "deep", "nested constructor");
}

// ============================================================
// 39. table gettable/settable
// ============================================================
function test_table_ops()
{
    printf("--- test_table_ops ---");

    var t = {a = 1, b = 2};
    var val = gettable(t, "a");
    assertEqual(val, 1, "gettable");

    settable(t, "c", 3);
    var val2 = gettable(t, "c");
    assertEqual(val2, 3, "settable then gettable");
}

function test_table_delete_reuse()
{
    printf("--- test_table_delete_reuse ---");

    var t = {};
    t.a = 1;
    t.b = 2;
    t.c = 3;
    t.b = nil;
    assertEqual(t.b, nil, "deleted table key reads nil");

    var count = 0;
    var sawDeleted = 0;
    foreach (k, v in ipairs(t))
    {
        count++;
        if (k == "b")
        {
            sawDeleted = 1;
        }
        assert(v != nil, "table iterator skips deleted value");
    }
    assertEqual(sawDeleted, 0, "table iterator skips deleted key");
    assertEqual(count, 2, "table iterator count after delete");

    t.b = 20;
    assertEqual(t.b, 20, "table key can be reinserted after delete");

    var churn = {};
    for (var i = 0; i < 128; i++)
    {
        churn[i] = i + 1000;
    }
    for (var j = 32; j < 128; j++)
    {
        churn[j] = nil;
    }
    for (var n = 128; n < 224; n++)
    {
        churn[n] = n + 1000;
    }

    for (var a = 0; a < 32; a++)
    {
        assertEqual(churn[a], a + 1000, "table churn keeps existing key");
    }
    for (var b = 32; b < 128; b++)
    {
        assertEqual(churn[b], nil, "table churn deleted key reads nil");
    }
    for (var c = 128; c < 224; c++)
    {
        assertEqual(churn[c], c + 1000, "table churn inserts new key");
    }

    var churnCount = 0;
    var nilCount = 0;
    foreach (ck, cv in ipairs(churn))
    {
        churnCount++;
        if (cv == nil)
        {
            nilCount++;
        }
    }
    assertEqual(nilCount, 0, "table churn iterator has no nil values");
    assertEqual(churnCount, 128, "table churn iterator count");
}

// ============================================================
// 45. 复合表操作：实现简单集合
// ============================================================
function test_set_like()
{
    printf("--- test_set_like ---");

    function make_set()
    {
        var data = {};
        var s = {};
        s.add = function(val)
        {
            data[val] = 1;
        };
        s.contains = function(val)
        {
            return data[val] != nil;
        };
        s.remove = function(val)
        {
            data[val] = nil;
        };
        return s;
    }

    var mySet = make_set();
    mySet.add("apple");
    mySet.add("banana");
    assert(mySet.contains("apple"), "set contains apple");
    assert(mySet.contains("banana"), "set contains banana");
    assert(!mySet.contains("cherry"), "set not contains cherry");

    mySet.remove("apple");
    assert(!mySet.contains("apple"), "set after remove apple");
}

// ============================================================
// 46. 数据结构边界情况
// ============================================================
function test_data_structure_edges()
{
    printf("--- test_data_structure_edges ---");

    var t = {};
    t[0] = "zero";
    t["0"] = "string-zero";
    assertEqual(t[0], "zero", "table int key separate from string key");
    assertEqual(t["0"], "string-zero", "table string key separate from int key");

    t.name = "xscript";
    t.name = nil;
    assertEqual(t.name, nil, "table key can be cleared to nil");

    var parent = {child = {value = 1}};
    var childRef = parent.child;
    childRef.value = 9;
    assertEqual(parent.child.value, 9, "nested table reference mutation");

    var lst = [1];
    lst:append(2, 3, 4);
    assertEqual(list.size(lst), 4, "list append multiple values");
    assertEqual(lst[3], 4, "list append multiple last value");

    lst:insert(2, 99, 100);
    assertEqual(lst[2], 99, "list insert multiple first");
    assertEqual(lst[3], 100, "list insert multiple second");
    assertEqual(list.size(lst), 6, "list insert multiple size");

    lst:remove(99);
    assertEqual(list.count(lst, 99), 0, "list remove value");

    lst:resize(8);
    assertEqual(list.size(lst), 8, "list resize grow");
    lst[7] = 77;
    assertEqual(lst[7], 77, "list assign after resize");
    assert(list.capacity(lst) >= list.size(lst), "list capacity >= size");

    var mixed = [{id = 1}, {id = 2}, {id = 3}];
    mixed[1].id = 20;
    assertEqual(mixed[1].id, 20, "list of tables mutation");
}
