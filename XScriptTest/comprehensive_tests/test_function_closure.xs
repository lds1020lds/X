// ============================================================
// XScript 综合测试 - 函数与闭包
// 覆盖：函数定义与调用、作用域、闭包与UpValue、
//        Lambda表达式、嵌套闭包与变量捕获
// ============================================================

// ============================================================
// 11. 函数定义与调用
// ============================================================


function test_function()
{
    printf("--- test_function ---");

    function add(a, b)
    {
        return a + b;
    }
    assertEqual(add(3, 4), 7, "simple func");

    // 多返回值
    function swap(a, b)
    {
        return b, a;
    }
    var x, y = swap(1, 2);
    assertEqual(x, 2, "multi return 1");
    assertEqual(y, 1, "multi return 2");

    // 无返回值
    function noop() {}
    var r = noop();
    assertEqual(r, nil, "no return");

    function explicit_empty_return()
    {
        return;
    }
    assertEqual(explicit_empty_return(), nil, "explicit empty return is nil");

    function explicit_empty_return_no_semicolon()
    {
        return
    }
    assertEqual(explicit_empty_return_no_semicolon(), nil, "explicit empty return without semicolon is nil");

    var emptyA, emptyB = explicit_empty_return();
    assertEqual(emptyA, nil, "empty return multi assign first nil");
    assertEqual(emptyB, nil, "empty return multi assign second nil");

    // 递归
    function factorial(n)
    {
        if (n <= 1) return 1;
        return n * factorial(n - 1);
    }
    assertEqual(factorial(5), 120, "recursive factorial");
    assertEqual(factorial(10), 3628800, "recursive factorial 10");

    // 嵌套调用
    function double_it(n) { return n * 2; }
    function quad(n) { return double_it(double_it(n)); }
    assertEqual(quad(3), 12, "nested call");
}

// ============================================================
// 12. 局部函数和作用域
// ============================================================
function test_scope()
{
    printf("--- test_scope ---");

    var x = 10;
    if (1)
    {
        var x = 20;
        assertEqual(x, 20, "inner scope");
    }
    assertEqual(x, 10, "outer scope preserved");

    // 函数内部局部变量不影响外部
    var a = 100;
    function modify()
    {
        var a = 200;
    }
    modify();
    assertEqual(a, 100, "func local scope");
}

// ============================================================
// 13. 闭包与 UpValue
// ============================================================
function test_closure()
{
    printf("--- test_closure ---");

    function make_counter()
    {
        var count = 0;
        function inc()
        {
            count++;
            return count;
        }
        return inc;
    }

    var counter = make_counter();
    assertEqual(counter(), 1, "closure 1");
    assertEqual(counter(), 2, "closure 2");
    assertEqual(counter(), 3, "closure 3");

    // 闭包独立
    var counter2 = make_counter();
    assertEqual(counter2(), 1, "closure independent");

    // 多闭包共享 UpValue
    function make_pair()
    {
        var val = 0;
        function get() { return val; }
        function set(v) { val = v; }
        return get, set;
    }

    var getter, setter = make_pair();
    assertEqual(getter(), 0, "shared upval get init");
    setter(42);
    assertEqual(getter(), 42, "shared upval get after set");

    // 闭包捕获参数
    function adder(x)
    {
        return lambda y: x + y;
    }
    var add5 = adder(5);
    assertEqual(add5(3), 8, "closure capture param");
}

// ============================================================
// 16. Lambda 表达式
// ============================================================
function test_lambda()
{
    printf("--- test_lambda ---");

    var double_it = lambda x: x * 2;
    assertEqual(double_it(5), 10, "lambda simple");

    // lambda 闭包
    var base = 100;
    var add_base = lambda x:base + x;
    assertEqual(add_base(23), 123, "lambda closure");

    // lambda 作为参数
    function apply(f, val)
    {
        return f(val);
    }
    assertEqual(apply(lambda x: x * x, 7), 49, "lambda as arg");

    var mixed = (3 * 4) + apply(lambda x: x * x, 5) + (6 * 7);
    assertEqual(mixed, 79, "lambda nested in expression preserves outer temps");
}

// ============================================================
// 41. 嵌套闭包与变量捕获
// ============================================================
function test_complex_temp_register_expressions()
{
    printf("--- test_complex_temp_register_expressions ---");

    function add(a, b) { return a + b; }
    function mul(a, b) { return a * b; }
    function pick(c, a, b) { if (c) return a; return b; }

    var a = 3;
    var b = 5;
    var c = 7;
    var d = 11;

    assertEqual((a + b) * (c - d) + (a * c) - (b * d), -66, "complex temp: arithmetic reuse");
    assertEqual(add(mul(a + b, c - 2), mul(d - c, b + 1)) + (a + d) * (b - c), 36, "complex temp: nested calls");
    assertEqual(pick((a + b) > c, add(a, mul(b, c)), add(d, mul(a, b))), 38, "complex temp: call args");

    var t = {x = 10, y = 20, nested = {a = 2, b = 4, c = 8}};
    assertEqual(t.nested.a * (t.x + t.y) + t.nested.c / t.nested.b + add(t.x, t.y), 92, "complex temp: table access");

    var key1 = "nested";
    var key2 = "c";
    assertEqual(t[key1][key2] + t["nested"]["a"] * (t.x - t.y + 30), 48, "complex temp: dynamic table keys");

    var f1 = lambda x: x * 2 + 1;
    var f2 = lambda x: x + 10;
    assertEqual(f1(a + b) + f2(c * d) - f1(b), 93, "complex temp: lambda calls");
    assertEqual(add((lambda x: x * x)(a + b), (lambda y: y + 1)(c * d)), 142, "complex temp: inline lambda calls");

    var iifeResult = (function(x, y)
    {
        var local = x * 10 + y;
        return local + (x + y) * (x - y);
    })(a + b, c - a);
    assertEqual(iifeResult + add(a, b), 140, "complex temp: iife assignment");

    assertEqual(add((function(x) { return x * x; })(a + b), mul((function(y) { return y + 1; })(c), d - b)), 112, "complex temp: iife call args");

    var called = 0;
    function touch(v)
    {
        called++;
        return v;
    }

    assertEqual(1 || touch((a + b) * (c + d)), 1, "complex temp: or skips rhs");
    assertEqual(called, 0, "complex temp: or skip count");
    assertEqual(0 && touch((a + b) * (c + d)), 0, "complex temp: and skips rhs");
    assertEqual(called, 0, "complex temp: and skip count");
    assertEqual((0 || touch(a + b)) + (1 && touch(c + d)), 26, "complex temp: selected rhs");
    assertEqual(called, 2, "complex temp: selected rhs count");

    assertEqual(((a + b) > c) ? add(a, b * c) or mul(c, d), 38, "complex temp: ternary true");
    assertEqual(((a + b) < c) ? add(a, b * c) or mul(c, d), 77, "complex temp: ternary false");

    var list = [1, 2, 3, 4, 5];
    assertEqual(list[a - 2] + list[b - 2] * list[c - 3] + list[0], 23, "complex temp: list index");

    function makeAdder(base)
    {
        return lambda x: base + x;
    }
    var add100 = makeAdder(100);
    assertEqual(add100((a + b) * (c - 2)) + add100(d - a), 248, "complex temp: closure calls");

    var obj = {value = 9, get = function(self, x) { return self.value + x; }};
    assertEqual(obj:get((a + b) * (c - 5)) + obj:get(d - c), 38, "complex temp: method calls");

    var maybe = nil;
    assertEqual((maybe?.x || 10) + (t?.nested?.c || 0) + (nil?.abc || 5), 23, "complex temp: optional fallback");

    var chained = {next = {next = {value = 42}}};
    assertEqual(chained?.next?.next?.value + (nil?.next?.value || 8), 50, "complex temp: nested optional");

    var deep = add(mul(add(a, b), add(c, d)), add(mul(add(a, c), add(b, d)), mul(add(a, d), add(b, c))));
    assertEqual(deep, 472, "complex temp: many nested temps");
}

function test_nested_closure()
{
    printf("--- test_nested_closure ---");

    function outer(x)
    {
        function middle(y)
        {
            function inner(z)
            {
                return x + y + z;
            }
            return inner;
        }
        return middle;
    }

    var f = outer(10)(20);
    assertEqual(f(30), 60, "nested closure");
}

// ============================================================
// 47. 函数与闭包边界情况
// ============================================================
function test_function_edges()
{
    printf("--- test_function_edges ---");

    function make_accumulator(start)
    {
        var total = start;
        function add(delta)
        {
            total += delta;
            return total;
        }
        function get()
        {
            return total;
        }
        return add, get;
    }

    var addA, getA = make_accumulator(10);
    var addB, getB = make_accumulator(100);
    assertEqual(addA(5), 15, "closure accumulator A add");
    assertEqual(getA(), 15, "closure accumulator A get");
    assertEqual(addB(7), 107, "closure accumulator B independent");
    assertEqual(getA(), 15, "closure accumulator A still independent");

    var ops = {};
    ops.add = function(a, b) { return a + b; };
    ops.mul = function(a, b) { return a * b; };
    assertEqual(ops.add(2, 3), 5, "function stored in table add");
    assertEqual(ops.mul(4, 5), 20, "function stored in table mul");

    var funcs = [lambda x: x + 1, lambda x: x * 2];
    var f0 = funcs[0];
    var f1 = funcs[1];
    assertEqual(f0(9), 10, "lambda stored in list first");
    assertEqual(f1(9), 18, "lambda stored in list second");

    function choose(flag)
    {
        if (flag) return lambda x: x + 10;
        return lambda x: x - 10;
    }
    assertEqual(choose(1)(5), 15, "return lambda true branch");
    assertEqual(choose(0)(5), -5, "return lambda false branch");
}

// ============================================================
// 48. 可变参数
// ============================================================
function test_varargs()
{
    printf("--- test_varargs ---");

    function sum_all(...)
    {
        var total = 0;
        for (var i = 0; i < __xscript_varargs.n; i++)
        {
            total += __xscript_varargs[i];
        }
        return total;
    }
    assertEqual(sum_all(), 0, "varargs no args");
    assertEqual(sum_all(1, 2, 3, 4), 10, "varargs sum all");

    function join_prefix(prefix, ...)
    {
        var result = prefix;
        for (var i = 0; i < __xscript_varargs.n; i++)
        {
            result $= __xscript_varargs[i];
        }
        return result;
    }
    assertEqual(join_prefix("v", 1, ":", 2), "v1:2", "varargs fixed param plus extras");
    assertEqual(join_prefix("only"), "only", "varargs fixed param no extras");

    function count_and_nil(...)
    {
        return __xscript_varargs.n, __xscript_varargs[0], __xscript_varargs[1], __xscript_varargs[2];
    }
    var n, a, b, c = count_and_nil(1, nil, 3);
    assertEqual(n, 3, "varargs count includes nil");
    assertEqual(a, 1, "varargs first value");
    assertEqual(b, nil, "varargs nil value");
    assertEqual(c, 3, "varargs value after nil");

    var args = "ordinary";
    function has_user_args(args, ...)
    {
        return args, __xscript_varargs.n, __xscript_varargs[0];
    }
    var fixed, extraN, extra0 = has_user_args("fixed", "extra");
    assertEqual(args, "ordinary", "varargs internal name does not affect outer args");
    assertEqual(fixed, "fixed", "varargs allows user param named args");
    assertEqual(extraN, 1, "varargs renamed table count");
    assertEqual(extra0, "extra", "varargs renamed table first value");

    var lambda_sum = lambda ...: __xscript_varargs[0] + __xscript_varargs[1] + __xscript_varargs.n;
    assertEqual(lambda_sum(4, 5), 11, "lambda varargs");

    var obj = {};
    obj.collect = function(self, base, ...)
    {
        var total = self.seed + base;
        for (var i = 0; i < __xscript_varargs.n; i++)
        {
            total += __xscript_varargs[i];
        }
        return total;
    };
    obj.seed = 10;
    assertEqual(obj:collect(1, 2, 3), 16, "method self plus varargs");
}

// ============================================================
// 函数调用参数自动调整测试
// 参数不足自动补nil，参数多余自动丢弃
// ============================================================
function test_call_args_adjust()
{
    printf("--- test_call_args_adjust ---");

    // --- 参数不足，自动补 nil ---
    function two_args(a, b)
    {
        return a, b;
    }
    var a, b = two_args(1);
    assertEqual(a, 1, "fewer args: first param ok");
    assertEqual(b, nil, "fewer args: missing param is nil");

    var c, d = two_args();
    assertEqual(c, nil, "no args: first param is nil");
    assertEqual(d, nil, "no args: second param is nil");

    function three_args(a, b, c)
    {
        if (c == nil) return a + b;
        return a + b + c;
    }
    assertEqual(three_args(10, 20), 30, "fewer args: nil in arithmetic guard");
    assertEqual(three_args(10, 20, 30), 60, "exact args: all present");

    // --- 参数多余，自动丢弃 ---
    function one_arg(a)
    {
        return a;
    }
    assertEqual(one_arg(42, 99, 100), 42, "extra args: only first used");

    function no_arg()
    {
        return 1;
    }
    assertEqual(no_arg(10, 20), 1, "extra args to no-param func");

    // --- 可变参数函数：参数不足时固定参数补 nil ---
    function varfunc(a, b, ...)
    {
        return a, b, __xscript_varargs.n;
    }
    var va, vb, vn = varfunc(1);
    assertEqual(va, 1, "varargs fewer: first fixed param ok");
    assertEqual(vb, nil, "varargs fewer: missing fixed param is nil");
    assertEqual(vn, 0, "varargs fewer: no varargs");

    var va2, vb2, vn2 = varfunc();
    assertEqual(va2, nil, "varargs no args: first is nil");
    assertEqual(vb2, nil, "varargs no args: second is nil");
    assertEqual(vn2, 0, "varargs no args: varargs count 0");

    // --- 可变参数函数：多余参数进入 varargs ---
    var va3, vb3, vn3 = varfunc(1, 2, 3, 4);
    assertEqual(va3, 1, "varargs extra: first fixed ok");
    assertEqual(vb3, 2, "varargs extra: second fixed ok");
    assertEqual(vn3, 2, "varargs extra: 2 extra in varargs");

    // --- 回调/高阶函数场景 ---
    function apply(fn, x)
    {
        return fn(x);
    }
    function add_default(a, b)
    {
        if (b == nil) b = 10;
        return a + b;
    }
    assertEqual(apply(add_default, 5), 15, "callback with fewer args uses default");

    // --- 方法调用参数不足 ---
    var obj = {};
    obj.greet = function(self, name, greeting)
    {
        if (greeting == nil) greeting = "hello";
        return greeting $ " " $ name;
    };
    assertEqual(obj:greet("world"), "hello world", "method call fewer args default");
    assertEqual(obj:greet("world", "hi"), "hi world", "method call exact args");

    // --- 嵌套调用参数调整 ---
    function outer(a, b, c)
    {
        return a + (b || 0) + (c || 0);
    }
    function inner()
    {
        return outer(1);
    }
    assertEqual(inner(), 1, "nested call with fewer args");

    // --- 递归中参数不足 ---
    function countdown(n, acc)
    {
        if (acc == nil) acc = 0;
        if (n <= 0) return acc;
        return countdown(n - 1, acc + 1);
    }
    assertEqual(countdown(5), 5, "recursive with missing param default");
}
