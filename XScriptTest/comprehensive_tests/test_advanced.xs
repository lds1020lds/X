// ============================================================
// XScript 综合测试 - 高级特性
// 覆盖：字符串库、数学库、type/toString、pcall/xpcall、
//        元表、协程、递归算法、排序、字符串处理、
//        三元表达式、链式访问、多返回值、loadstring、
//        表达式优先级、GC、深度递归、toNumber、
//        字符串方法、true/false关键字
// ============================================================

// ============================================================
// 17. 字符串库
// ============================================================

function test_string_lib()
{
    printf("--- test_string_lib ---");

    // len
    assertEqual(string.len("hello"), 5, "string.len");

    // find
    assertEqual(string.find("hello world", "world"), 6, "string.find");

    // sub (注册名为 "sub")
    assertEqual(string.sub("hello world", 0, 5), "hello", "string.sub");

    // upper / lower
    assertEqual(string.upper("hello"), "HELLO", "string.upper");
    assertEqual(string.lower("HELLO"), "hello", "string.lower");

    // split
    var parts = string.split("a,b,c", ",");
    assertEqual(parts[0], "a", "string.split 0");
    assertEqual(parts[1], "b", "string.split 1");
    assertEqual(parts[2], "c", "string.split 2");

    // replace
    assertEqual(string.replace("hello world", "world", "xscript"), "hello xscript", "string.replace");

    // trim
    assertEqual(string.trim("  hello  "), "hello", "string.trim");

    // atoi
    assertEqual(string.atoi("42"), 42, "string.atoi");

    // atof
    var f = string.atof("3.14");
    assert(f > 3.1 && f < 3.2, "string.atof");

    // startWith / endWith
    assertEqual(string.startWith("hello", "hel"), 1, "string.startWith");
    assertEqual(string.endWith("hello", "llo"), 1, "string.endWith");

    // compare
    assertEqual(string.compare("abc", "abc"), 0, "string.compare equal");
    assert(string.compare("abc", "def") < 0, "string.compare less");

    // isalpha / isdigit
    assertEqual(string.isalpha("abc"), 1, "string.isalpha");
    assertEqual(string.isdigit("123"), 1, "string.isdigit");

    // rfind
    assertEqual(string.rfind("abcabc", "abc"), 3, "string.rfind");

    // trimLeft / trimRight
    assertEqual(string.trimLeft("  hello"), "hello", "string.trimLeft");
    assertEqual(string.trimRight("hello  "), "hello", "string.trimRight");
}

// ============================================================
// 18. 数学库
// ============================================================
function test_math_lib()
{
    printf("--- test_math_lib ---");

    // sqrt
    var s = math.sqrt(4);
    assert(s > 1.99 && s < 2.01, "math.sqrt(4)");

    // sin/cos 近似比较
    var sin0 = math.sin(0);
    assert(sin0 > -0.001 && sin0 < 0.001, "math.sin(0) ~= 0");

    var cos0 = math.cos(0);
    assert(cos0 > 0.999 && cos0 < 1.001, "math.cos(0) ~= 1");

    // exp
    var e = math.exp(0);
    assert(e > 0.999 && e < 1.001, "math.exp(0) ~= 1");
}

// ============================================================
// 19. type() 和 toString()
// ============================================================
function test_type_and_tostring()
{
    printf("--- test_type_and_tostring ---");

    assertEqual(type(42), "int", "type int");
    assertEqual(type(3.14), "float", "type float");
    assertEqual(type("hello"), "string", "type string");
    assertEqual(type(nil), "nil", "type nil");
    assertEqual(type({}), "table", "type table");

    assertEqual(toString(42), "42", "toString int");
    assertEqual(toString("abc"), "abc", "toString string");
}

// ============================================================
// 20. pcall 错误处理
// ============================================================
function test_pcall()
{
    printf("--- test_pcall ---");

    // 正常调用：返回 (0, "") 表示成功
    var ok, value = pcall(function()
    {
        return 42;
    });
    assertEqual(ok, 0, "pcall success");
    assertEqual(value, 42, "pcall return value error");

    // 异常调用：返回 (非0, 错误描述) 表示失败
    var ok2, err = pcall(function()
    {
        var x = nil;
        var y = x + 1;
        return y;
    });
    assert(ok2 != 0, "pcall caught error");
    assert(string.len(err) > 0, "pcall has error desc");
}

// ============================================================
// 21. 元表 (MetaTable)
// ============================================================
function test_metatable()
{
    printf("--- test_metatable ---");

    // __index 函数
    var meta = {__index = function(table, key) { return "fallback";} };
    var obj2 = {};
    setmetatable(obj2, meta);
    assertEqual(obj2.missing, "fallback", "__index function");
	
    // __index 表查找
    var proto = {greet = "hello"};
    var obj = {name = "world"};
    proto.__index = proto;
    setmetatable(obj, proto);
    assertEqual(obj.greet, "hello", "__index table");



    // __newindex
    var meta2 = {
        __newindex = function(table, key, val)
        {
			settable(table, key, val $ "_logged")
        }
    };
    var log = {};
    setmetatable(log, meta2);
    log.x = "test";
    assertEqual(log.x, "test_logged", "__newindex");

    // __add
    var meta3 = {
        __add = function(a, b) {return a.val + b.val}
    };
    var obj3 = {val = 10};
    var obj4 = {val = 20};
    setmetatable(obj3, meta3);
    setmetatable(obj4, meta3);
    assertEqual(obj3 + obj4, 30, "__add");

    // __call
    var meta4 = {
        __call = function(callValue)
        {
            return "called:" $ toString(callValue);
        }
    };
    var obj5 = {};
    setmetatable(obj5, meta4);
    var callResult = obj5(42);
    assertEqual(callResult, "called:42", "__call");
}

// ============================================================
// 22. 协程基础
// ============================================================
function test_coroutine_basic()
{
    printf("--- test_coroutine_basic ---");

    function coro_body()
    {
        coroutine.yield(1);
        coroutine.yield(2);
        return 3;
    }

    var co = coroutine.create(coro_body);
    var ok, val = coroutine.resume(co);
    assertEqual(ok, 1, "coroutine resume 1");
    assertEqual(val, 1, "coroutine yield 1");

    ok, val = coroutine.resume(co);
    assertEqual(ok, 1, "coroutine resume 2");
    assertEqual(val, 2, "coroutine yield 2");

    ok, val = coroutine.resume(co);
    assertEqual(ok, 1, "coroutine resume 3");
    assertEqual(val, 3, "coroutine return");
}

// ============================================================
// 23. 递归：斐波那契
// ============================================================
function test_fibonacci()
{
    printf("--- test_fibonacci ---");

    function fib(n)
    {
        if (n <= 0) return 0;
        if (n == 1) return 1;
        return fib(n - 1) + fib(n - 2);
    }

    assertEqual(fib(0), 0, "fib 0");
    assertEqual(fib(1), 1, "fib 1");
    assertEqual(fib(5), 5, "fib 5");
    assertEqual(fib(10), 55, "fib 10");
}

// ============================================================
// 24. 递归：汉诺塔
// ============================================================
function test_hanoi()
{
    printf("--- test_hanoi ---");

    var moves = 0;
    function hanoi(n, from, to, via)
    {
        if (n == 1)
        {
            moves++;
            return;
        }
        hanoi(n - 1, from, via, to);
        moves++;
        hanoi(n - 1, via, to, from);
    }
    hanoi(3, "A", "C", "B");
    assertEqual(moves, 7, "hanoi 3 disks");

    moves = 0;
    hanoi(5, "A", "C", "B");
    assertEqual(moves, 31, "hanoi 5 disks");
}

// ============================================================
// 25. 冒泡排序
// ============================================================
function test_bubble_sort()
{
    printf("--- test_bubble_sort ---");

    var arr = [64, 34, 25, 12, 22, 11, 90];
    var n = list.size(arr);
    for (i = 0; i < n - 1; i++)
    {
        for (j = 0; j < n - i - 1; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                var tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
    assertEqual(arr[0], 11, "bubble sort first");
    assertEqual(arr[6], 90, "bubble sort last");
}

// ============================================================
// 26. 字符串处理综合
// ============================================================
function test_string_operations()
{
    printf("--- test_string_operations ---");

    // 字符串拼接与循环
    var result = "";
    for (i = 0; i < 5; i++)
    {
        if (i > 0) result = result $ ",";
        result = result $ toString(i);
    }
    assertEqual(result, "0,1,2,3,4", "join loop");

    // rfind
    assertEqual(string.rfind("abcabc", "abc"), 3, "string.rfind");

    // trimLeft / trimRight
    assertEqual(string.trimLeft("  hello"), "hello", "string.trimLeft");
    assertEqual(string.trimRight("hello  "), "hello", "string.trimRight");

    // compare
    assert(string.compare("abc", "abd") < 0, "string.compare less");
    assert(string.compare("abd", "abc") > 0, "string.compare greater");
}

// ============================================================
// 27. 三元条件表达式
// ============================================================
function test_ternary()
{
    printf("--- test_ternary ---");

    var a = 10;
    var b = 20;
    var max = (a > b) ? a or b;
    assertEqual(max, 20, "ternary max");

    var min = (a < b) ? a or b;
    assertEqual(min, 10, "ternary min");

    // 嵌套三元
    var mid = (a > b) ? a or ((a < b) ? b or 0);
    assertEqual(mid, 20, "ternary nested");
}

// ============================================================
// 28. 嵌套表访问链
// ============================================================
function test_chained_access()
{
    printf("--- test_chained_access ---");

    var a = {};
    a.b = {};
    a.b.c = {};
    a.b.c.d = 42;
    assertEqual(a.b.c.d, 42, "deep chain access");
    assertEqual((a).b.c.d, 42, "parenthesized subject chain access");

    // bracket + dot mixed
    var t = {};
    t["key1"] = {};
    t["key1"].key2 = "value";
    assertEqual(t["key1"].key2, "value", "mixed chain");

    // function call on table member
    var obj = {val = 10};
    obj.get_double = function() { return obj.val * 2; };
    assertEqual(obj.get_double(), 20, "table method");
}

// ============================================================
// 29. 多返回值与多重赋值
// ============================================================
function test_multi_return()
{
    printf("--- test_multi_return ---");

    function divmod(a, b)
    {
        return a / b, a % b;
    }
    var q, r = divmod(17, 5);
    assertEqual(q, 3, "divmod quotient");
    assertEqual(r, 2, "divmod remainder");

    // 多重赋值会先快照右侧表达式，再写入左侧变量
    var a, b = 1, 2;
    a, b = b, a;
    assertEqual(a, 2, "multi assign swap a");
    assertEqual(b, 1, "multi assign swap b");

    var x, y, z = divmod(17, 5);
    assertEqual(x, 3, "multi assign func return first");
    assertEqual(y, 2, "multi assign func return second");
    assertEqual(z, nil, "multi assign missing return nil");
}

// ============================================================
// 30. loadstring 动态执行
// ============================================================
function test_loadstring()
{
    printf("--- test_loadstring ---");

    var ret, func = loadstring("return 1 + 2");
    if (ret > 0)
    {
        var result = func();
        assertEqual(result, 3, "loadstring result");
    }
    else
    {
        assert(false, "loadstring failed");
    }
}

// ============================================================
// 32. 复杂表达式优先级
// ============================================================
function test_expression_priority()
{
    printf("--- test_expression_priority ---");

    var a = 2 + 3 * 4;
    assertEqual(a, 14, "priority: mul before add");

    var b = (2 + 3) * 4;
    assertEqual(b, 20, "priority: parens");

    var c = 10 - 3 - 2;
    assertEqual(c, 5, "left associativity");

    // XScript 中 ^ 是左结合的，2^3^2 = (2^3)^2 = 64
    var d = 2 ^ 3 ^ 2;
    assertEqual(d, 64, "left associativity pow");

    var e = 1 && (1 > 0);
    assertEqual(e, 1, "logical with comparison");
}

// ============================================================
// 33. GC 测试
// ============================================================
function test_gc()
{
    printf("--- test_gc ---");

    // 创建大量临时表，不应内存泄漏
    for (i = 0; i < 100; i++)
    {
        var tmp = {a = i, b = i * 2};
    }
    garbageCollect();
    assert(true, "gc no crash");
}

// ============================================================
// 34. 深度递归压力测试
// ============================================================
function test_deep_recursion()
{
    printf("--- test_deep_recursion ---");

    function depth(n)
    {
        if (n <= 0) return 0;
        return 1 + depth(n - 1);
    }
    var result = depth(100);
    assertEqual(result, 100, "depth 100");
}

// ============================================================
// 35. setmetatable/getmetatable
// ============================================================
function test_metatable_ops()
{
    printf("--- test_metatable_ops ---");

    var t = {x = 1};
    var mt = {y = 2};
    mt.__index = mt;
    setmetatable(t, mt);
    var retrieved = getmetatable(t);
    assertEqual(retrieved.y, 2, "getmetatable");
}

// ============================================================
// 36. 协程 wrap
// ============================================================
function test_coroutine_wrap()
{
    printf("--- test_coroutine_wrap ---");

    var f = coroutine.wrap(function()
    {
        coroutine.yield(10);
        coroutine.yield(20);
        return 30;
    });

    // coroutine.wrap 返回的函数也会返回 (1, yield_value)
    // 第一个返回值是成功标志1，第二个是yield/return值
    var ok1, v1 = f();
    assertEqual(ok1, 1, "coroutine wrap ok 1");
    assertEqual(v1, 10, "coroutine wrap 1");

    var ok2, v2 = f();
    assertEqual(ok2, 1, "coroutine wrap ok 2");
    assertEqual(v2, 20, "coroutine wrap 2");

    var ok3, v3 = f();
    assertEqual(ok3, 1, "coroutine wrap ok 3");
    assertEqual(v3, 30, "coroutine wrap 3");
}

// ============================================================
// 37. xpcall 错误处理
// ============================================================
function test_xpcall()
{
    printf("--- test_xpcall ---");

    // xpcall 正常调用
    var ok, desc = xpcall(function()
    {
        return 42;
    }, function(code, msg)
    {
        return "handled";
    });
    assertEqual(ok, 0, "xpcall success");

    // xpcall 异常调用：返回 (非0, 错误描述)
    var ok2, err = xpcall(function()
    {
        var x = nil;
        return x + 1;
    }, function(code, msg)
    {
        return "handled: " $ msg;
    });
    assert(ok2 != 0, "xpcall caught error");
}

// ============================================================
// 38. list sortWithKey / sortWithCmp
// ============================================================
function test_list_sort_advanced()
{
    printf("--- test_list_sort_advanced ---");

    // sortWithKey
    var a = ["1", "19", "2", "3", "11"];
    a:sortWithKey(lambda v:string.atoi(v));
    assertEqual(string.atoi(a[0]), 1, "sortWithKey first");
    assertEqual(string.atoi(a[4]), 19, "sortWithKey last");

    // sortWithCmp
    var b = [5, 3, 8, 1, 2];
    b:sortWithCmp(lambda v1, v2, reverse: v1 > v2);
    assertEqual(b[0], 8, "sortWithCmp desc first");
    assertEqual(b[4], 1, "sortWithCmp desc last");
}

// ============================================================
// 40. toNumber 转换
// ============================================================
function test_toNumber()
{
    printf("--- test_toNumber ---");

    var n = toNumber("123");
    assertEqual(n, 123, "toNumber string to int");
}

// ============================================================
// 42. 列表作为函数参数与返回值
// ============================================================
function test_list_func()
{
    printf("--- test_list_func ---");

    function reverse_list(lst)
    {
        var result = [];
        for (i = list.size(lst) - 1; i >= 0; i--)
        {
            result:append(lst[i]);
        }
        return result;
    }

    var original = [1, 2, 3, 4, 5];
    var reversed = reverse_list(original);
    assertEqual(reversed[0], 5, "list func reverse 0");
    assertEqual(reversed[4], 1, "list func reverse 4");
}

// ============================================================
// 43. 字符串方法冒号调用
// ============================================================
function test_string_method()
{
    printf("--- test_string_method ---");

    var s = "hello world";
    // 字符串冒号调用方法
    assertEqual(s:len(), 11, "string:len()");
    assertEqual(s:upper(), "HELLO WORLD", "string:upper()");
    assertEqual(s:sub(0, 5), "hello", "string:sub()");
}

// ============================================================
// 44. true/false 关键字
// ============================================================
function test_true_false()
{
    printf("--- test_true_false ---");

    assertEqual(true, 1, "true == 1");
    assertEqual(false, 0, "false == 0");
    assertEqual(!true, 0, "!true == 0");
    assertEqual(!false, 1, "!false == 1");

    if (true)
    {
        assert(true, "if true");
    }
    else
    {
        assert(false, "if true should not go here");
    }

    if (false)
    {
        assert(false, "if false should not go here");
    }
    else
    {
        assert(true, "if false else");
    }
}

// ============================================================
// 48. 字符串边界情况
// ============================================================
function test_string_edge_cases()
{
    printf("--- test_string_edge_cases ---");

    assertEqual(string.len(""), 0, "empty string len");
    assertEqual(string.rawlen("abc"), 3, "rawlen ascii");
    assertEqual(string.find("abc", "z"), -1, "find missing returns -1");
    assertEqual(string.rfind("abc", "z"), -1, "rfind missing returns -1");
    assertEqual(string.sub("abc", 5, 1), "", "sub out of range returns empty");

    var parts = string.split("abc", ",");
    assertEqual(list.size(parts), 1, "split missing delimiter size");
    assertEqual(parts[0], "abc", "split missing delimiter value");

    var emptyParts = string.split("", ",");
    assertEqual(list.size(emptyParts), 0, "split empty string size");

    assertEqual(string.replace("aaaa", "aa", "b"), "bb", "replace repeated pattern");
    assertEqual(string.trim("\t hello \n"), "hello", "trim tab newline");
    assertEqual(string.isspace(" \t\n"), 1, "isspace true");
    assertEqual(string.islower("abc"), 1, "islower true");
    assertEqual(string.isupper("ABC"), 1, "isupper true");
    assertEqual(string.isalpha("abc1"), 0, "isalpha false");
    assertEqual(string.isdigit("12a"), 0, "isdigit false");
}

// ============================================================
// 49. 数学库扩展
// ============================================================
function test_math_edges()
{
    printf("--- test_math_edges ---");

    var logE = math.log(math.exp(1));
    assert(logE > 0.99 && logE < 1.01, "log(exp(1)) ~= 1");

    var log10Value = math.log10(1000);
    assert(log10Value > 2.99 && log10Value < 3.01, "log10(1000) ~= 3");

    var tan0 = math.tan(0);
    assert(tan0 > -0.001 && tan0 < 0.001, "tan(0) ~= 0");

    var asin0 = math.asin(0);
    assert(asin0 > -0.001 && asin0 < 0.001, "asin(0) ~= 0");

    var acos1 = math.acos(1);
    assert(acos1 > -0.001 && acos1 < 0.001, "acos(1) ~= 0");

    var atan1 = math.atan(1);
    assert(atan1 > 0.78 && atan1 < 0.79, "atan(1) ~= pi/4");
}

// ============================================================
// 50. 元表运算符扩展
// ============================================================
function test_metatable_operator_edges()
{
    printf("--- test_metatable_operator_edges ---");

    var meta = {
        __sub = function(a, b) { return a.val - b.val; },
        __mul = function(a, b) { return a.val * b.val; },
        __div = function(a, b) { return a.val / b.val; },
        __mod = function(a, b) { return a.val % b.val; },
        __pow = function(a, b) { return a.val ^ b.val; },
        __equal = function(a, b) { return a.val == b.val; },
        __less = function(a, b) { return a.val < b.val; },
        __lessequal = function(a, b) { return a.val <= b.val; }
    };

    var a = {val = 10, name = "A"};
    var b = {val = 3, name = "B"};
    var c = {val = 10, name = "C"};
    setmetatable(a, meta);
    setmetatable(b, meta);
    setmetatable(c, meta);

    assertEqual(a - b, 7, "metatable __sub");
    assertEqual(a * b, 30, "metatable __mul");
    assertEqual(a / b, 3, "metatable __div");
    assertEqual(a % b, 1, "metatable __mod");
    assertEqual(b ^ b, 27, "metatable __pow");
    assertEqual(a == c, 1, "metatable __equal true");
    assertEqual(b < a, 1, "metatable __less true");
    assertEqual(a <= c, 1, "metatable __lessequal true");
}

function test_metatable_full_coverage()
{
    printf("--- test_metatable_full_coverage ---");

    var opMeta = {
        __add = function(a, b) { return {val = a.val + b.val}; },
        __sub = function(a, b) { return {val = a.val - b.val}; },
        __mul = function(a, b) { return {val = a.val * b.val}; },
        __div = function(a, b) { return {val = a.val / b.val}; },
        __mod = function(a, b) { return {val = a.val % b.val}; },
        __pow = function(a, b) { return {val = a.val ^ b.val}; },
        __neg = function(a) { return {val = -a.val}; },
        __concat = function(a, b) { return a.name $ ":" $ b.name; },
        __equal = function(a, b) { return a.val == b.val; },
        __less = function(a, b) { return a.val < b.val; },
        __lessequal = function(a, b) { return a.val <= b.val; },
        __call = function(a, b) { return a + b; }
    };

    function make_meta_value(name, val)
    {
        var obj = {name = name, val = val};
        setmetatable(obj, opMeta);
        return obj;
    }

    var a = make_meta_value("a", 8);
    var b = make_meta_value("b", 3);
    var c = make_meta_value("c", 8);

    assertEqual((a + b).val, 11, "full metatable __add returns object");
    assertEqual((a - b).val, 5, "full metatable __sub returns object");
    assertEqual((a * b).val, 24, "full metatable __mul returns object");
    assertEqual((a / b).val, 2, "full metatable __div returns object");
    assertEqual((a % b).val, 2, "full metatable __mod returns object");
    assertEqual((b ^ b).val, 27, "full metatable __pow returns object");
    assertEqual((-a).val, -8, "full metatable __neg");
    assertEqual(a $ b, "a:b", "full metatable __concat");
    assertEqual(a == c, 1, "full metatable __equal true");
    assertEqual(a == b, 0, "full metatable __equal false");
    assertEqual(b < a, 1, "full metatable __less true");
    assertEqual(a < b, 0, "full metatable __less false");
    assertEqual(a <= c, 1, "full metatable __lessequal equal");
    assertEqual(a > b, 1, "full metatable __greater maps to __less");
    assertEqual(a >= c, 1, "full metatable __greaterequal maps to __lessequal");
    assertEqual(a(4, 5), 9, "full metatable __call with args");
}

function test_metatable_index_newindex_edges()
{
    printf("--- test_metatable_index_newindex_edges ---");

    var defaults = {kind = "default", shared = 7};
    var proto = {
        own = 1,
        __index = defaults,
        __newindex = defaults
    };
    var obj = {own = 2};
    setmetatable(obj, proto);
    assertEqual(obj.own, 2, "metatable existing key bypasses __index");
    assertEqual(obj.kind, "default", "metatable __index table fallback");
    obj.created = 99;
    assertEqual(defaults.created, 99, "metatable __newindex table target");
    assertEqual(obj.created, 99, "metatable __index sees __newindex target");

    var log = [];
    var meta = {
        __index = function(tableValue, key)
        {
            log:append("get:" $ key);
            return "missing:" $ key;
        },
        __newindex = function(tableValue, key, value)
        {
            log:append("set:" $ key $ "=" $ toString(value));
            settable(tableValue, "stored_" $ key, value);
        }
    };
    var tracked = {};
    setmetatable(tracked, meta);
    assertEqual(tracked.foo, "missing:foo", "metatable __index function fallback");
    tracked.bar = 123;
    assertEqual(tracked.stored_bar, 123, "metatable __newindex function stored value");
    assertEqual(list.size(log), 2, "metatable __index/__newindex log count");
    assertEqual(log[0], "get:foo", "metatable __index log value");
    assertEqual(log[1], "set:bar=123", "metatable __newindex log value");
}

function test_metatable_gc_edges()
{
    printf("--- test_metatable_gc_edges ---");

    var meta = {
        __index = function(tableValue, key)
        {
            return "gc:" $ key;
        },
        __add = function(a, b)
        {
            return a.value + b.value;
        },
        __call = function()
        {
            return 42;
        }
    };

    var obj = {value = 21};
    var other = {value = 5};
    setmetatable(obj, meta);
    setmetatable(other, meta);
    garbageCollect();
    assertEqual(obj.missing, "gc:missing", "metatable survives gc __index");
    assertEqual(obj + other, 26, "metatable survives gc __add");
    assertEqual(obj(), 42, "metatable survives gc __call");

    var holder = {child = obj};
    garbageCollect();
    assertEqual(holder.child.missing2, "gc:missing2", "nested metatable survives gc");
}

// ============================================================
// 51. 协程状态
// ============================================================
function test_coroutine_status_edges()
{
    printf("--- test_coroutine_status_edges ---");

    var co = coroutine.create(function()
    {
        coroutine.yield("pause");
        return "done";
    });

    assertEqual(coroutine.status(co), "suspend", "coroutine initial status");

    var ok, val = coroutine.resume(co);
    assertEqual(ok, 1, "coroutine status resume ok");
    assertEqual(val, "pause", "coroutine status yield value");
    assertEqual(coroutine.status(co), "suspend", "coroutine status after yield");

    ok, val = coroutine.resume(co);
    assertEqual(ok, 1, "coroutine status final resume ok");
    assertEqual(val, "done", "coroutine status return value");
    assertEqual(coroutine.status(co), "dead", "coroutine dead after return");
}

// ============================================================
// 52. loadstring 与 pcall 边界情况
// ============================================================
function test_dynamic_error_edges()
{
    printf("--- test_dynamic_error_edges ---");

    var ret, func = loadstring("var x = 10; return x * 2");
    assertEqual(ret, 1, "loadstring statement chunk success");
    assertEqual(func(), 20, "loadstring statement chunk result");

    var badRet = loadstring(nil);
    assertEqual(badRet, 0, "loadstring nil returns 0");

    var internalRet = loadstring("var __xscript_internal_reg_0 = 1; return __xscript_internal_reg_0");
    assertEqual(internalRet, 0, "internal identifier prefix is reserved");

    var fieldRet, fieldFunc = loadstring("var t = {}; t.__xscript_internal_reg_0 = 7; return t.__xscript_internal_reg_0");
    assertEqual(fieldRet, 1, "internal prefix allowed as table field");
    assertEqual(fieldFunc(), 7, "internal prefix table field value");

    var literalFieldRet = loadstring("return (1).x");
    assertEqual(literalFieldRet, 0, "int literal cannot use dot postfix");
    var literalCallRet = loadstring("return (1)()");
    assertEqual(literalCallRet, 0, "int literal cannot use call postfix");
    var literalStringRet = loadstring("return (\"abc\").x");
    assertEqual(literalStringRet, 0, "string literal cannot use dot postfix");
    var literalNilRet = loadstring("return nil.x");
    assertEqual(literalNilRet, 0, "nil literal cannot use dot postfix");

    var outerOk, outerErr = pcall(function()
    {
        var innerOk, innerErr = pcall(function()
        {
            var x = nil;
            return x + 1;
        });
        assert(innerOk != 0, "nested pcall inner catches error");
        assert(string.len(innerErr) > 0, "nested pcall inner has error text");
        return 123;
    });
    assertEqual(outerOk, 0, "nested pcall outer success");
    assertEqual(outerErr, 123, "nested pcall outer no error text");
}

// ============================================================
// 53. GC 与复杂对象压力
// ============================================================
function test_gc_edges()
{
    printf("--- test_gc_edges ---");

    var checksum = 0;
    for (var i = 0; i < 50; i++)
    {
        var node = {id = i, items = []};
        for (var j = 0; j < 10; j++)
        {
            node.items:append({value = i * j});
            checksum += node.items[j].value;
        }
    }
    garbageCollect();
    assertEqual(checksum, 55125, "gc nested object checksum");
}
