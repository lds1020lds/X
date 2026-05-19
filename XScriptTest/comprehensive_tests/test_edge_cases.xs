// ============================================================
// XScript 综合测试 - 边界情况与高级场景
// 覆盖：字符串转义、协程高级用法、元表继承链、递归闭包、
//        IIFE、表引用语义、错误边界、复杂表达式、
//        链式方法调用、全局变量交互
// ============================================================

// ============================================================
// 1. 字符串转义序列
// ============================================================
function test_string_escape()
{
    printf("--- test_string_escape ---");

    // 基本转义
    var tab = "\t";
    var newline = "\n";
    var backslash = "\\";
    var quote = "\"";

    assertEqual(string.len(tab), 1, "tab escape is single char");
    assertEqual(string.len(newline), 1, "newline escape is single char");
    assertEqual(string.len(backslash), 1, "backslash escape is single char");
    assertEqual(string.len(quote), 1, "quote escape is single char");

    // 转义在字符串中间
    var mixed = "a\tb\nc";
    assertEqual(string.len(mixed), 5, "mixed escape length");
    assertEqual(string.sub(mixed, 0, 1), "a", "mixed escape first char");

    
    // 连续转义
    var multi = "\t\t\n\n";
    assertEqual(string.len(multi), 4, "multiple escapes length");

    // 转义与普通字符混合
    var path = "C:\\Users\\test";
    assert(string.find(path, "\\") >= 0, "backslash in path");
}

// ============================================================
// 2. 协程高级用法 - resume 传参给 yield
// ============================================================
function test_coroutine_resume_args()
{
    printf("--- test_coroutine_resume_args ---");

    function worker(initVal)
    {
        var received = coroutine.yield(initVal * 2);
        var received2 = coroutine.yield(received + 10);
        return received2 * 3;
    }

    var co = coroutine.create(worker);

    // 第一次 resume 传入初始参数
    var ok, val = coroutine.resume(co, 5);
    assertEqual(ok, 1, "coro args: first resume ok");
    assertEqual(val, 10, "coro args: first yield value (5*2)");

    // 第二次 resume 传入值给 yield 返回
    ok, val = coroutine.resume(co, 42);
    assertEqual(ok, 1, "coro args: second resume ok");
    assertEqual(val, 52, "coro args: second yield value (42+10)");

    // 第三次 resume 传入值给最后的 yield 返回
    ok, val = coroutine.resume(co, 7);
    assertEqual(ok, 1, "coro args: third resume ok");
    assertEqual(val, 21, "coro args: return value (7*3)");

    // 协程已结束
    assertEqual(coroutine.status(co), "dead", "coro args: dead after return");
}

// ============================================================
// 3. 协程作为迭代器（生成器模式）
// ============================================================
function test_coroutine_generator()
{
    printf("--- test_coroutine_generator ---");

    function range(start, stop, step)
    {
        var i = start;
        while (i < stop)
        {
            coroutine.yield(i);
            i += step;
        }
    }

    var co = coroutine.create(range);
    var results = [];

    var ok, val = coroutine.resume(co, 0, 10, 3);
    while (ok == 1 && coroutine.status(co) != "dead")
    {
        results:append(val);
        ok, val = coroutine.resume(co);
    }

    assertEqual(list.size(results), 4, "generator: produced 4 values");
    assertEqual(results[0], 0, "generator: first value");
    assertEqual(results[1], 3, "generator: second value");
    assertEqual(results[2], 6, "generator: third value");
    assertEqual(results[3], 9, "generator: fourth value");
}

// ============================================================
// 4. 元表继承链（多级原型链）
// ============================================================
function test_metatable_chain()
{
    printf("--- test_metatable_chain ---");

    // 三级继承链: child -> parent -> grandparent
    var grandparent = {species = "animal", breathe = "yes"};
    grandparent.__index = grandparent;

    var parent = {legs = 4, sound = "bark"};
    parent.__index = parent;
    setmetatable(parent, grandparent);

    var child = {name = "Rex"};
    setmetatable(child, parent);

    // 直接属性
    assertEqual(child.name, "Rex", "chain: direct property");
    // 一级继承
    assertEqual(child.legs, 4, "chain: parent property");
    assertEqual(child.sound, "bark", "chain: parent property 2");
    // 二级继承
    assertEqual(child.species, "animal", "chain: grandparent property");
    assertEqual(child.breathe, "yes", "chain: grandparent property 2");

    // 修改不影响原型
    child.sound = "woof";
    assertEqual(child.sound, "woof", "chain: override property");
    assertEqual(parent.sound, "bark", "chain: parent unchanged");
}

// ============================================================
// 5. 元表 __index 函数与表结合
// ============================================================
function test_metatable_index_complex()
{
    printf("--- test_metatable_index_complex ---");

    var defaults = {color = "red", size = 10, visible = 1};
    var meta = {
        __index = function(table, key)
        {
            return defaults[key];
        }
    };

    var obj = {name = "widget"};
    setmetatable(obj, meta);

    assertEqual(obj.name, "widget", "index_complex: direct field");
    assertEqual(obj.color, "red", "index_complex: default color");
    assertEqual(obj.size, 10, "index_complex: default size");
    assertEqual(obj.visible, 1, "index_complex: default visible");

    // 设置后不再走 __index
    obj.color = "blue";
    assertEqual(obj.color, "blue", "index_complex: overridden field");
}

// ============================================================
// 6. 递归闭包（闭包中递归调用自身）
// ============================================================
function test_recursive_closure()
{
    printf("--- test_recursive_closure ---");

    // 通过变量引用实现递归闭包
    var factorial;
    factorial = function(n)
    {
        if (n <= 1) return 1;
        return n * factorial(n - 1);
    };

    assertEqual(factorial(1), 1, "recursive closure: fac(1)");
    assertEqual(factorial(5), 120, "recursive closure: fac(5)");
    assertEqual(factorial(7), 5040, "recursive closure: fac(7)");

    // 互递归闭包
    var isEven;
    var isOdd;
    isEven = function(n)
    {
        if (n == 0) return 1;
        return isOdd(n - 1);
    };
    isOdd = function(n)
    {
        if (n == 0) return 0;
        return isEven(n - 1);
    };

    assertEqual(isEven(0), 1, "mutual recursion: isEven(0)");
    assertEqual(isEven(4), 1, "mutual recursion: isEven(4)");
    assertEqual(isOdd(3), 1, "mutual recursion: isOdd(3)");
    assertEqual(isOdd(4), 0, "mutual recursion: isOdd(4)");
}

// ============================================================
// 7. 匿名函数赋值后立即调用
// ============================================================
function test_iife()
{
    printf("--- test_iife ---");

    // 匿名函数赋值后调用
    var fn1 = function() { return 42; };
    var result = fn1();
    assertEqual(result, 42, "iife: simple return");

    // 括号包裹匿名函数后立即调用
    var direct = (function() { return 42; })();
    assertEqual(direct, 42, "iife: direct call");

    // 带参数的匿名函数
    var fn2 = function(a, b) { return a + b; };
    var sum = fn2(10, 20);
    assertEqual(sum, 30, "iife: with args");

    // 匿名函数创建隔离作用域
    var makeCounter = function()
    {
        var count = 0;
        return {
            inc = function() { count++; return count; },
            get = function() { return count; }
        };
    };
    var counter = makeCounter();

    assertEqual(counter.get(), 0, "iife scope: initial");
    counter.inc();
    counter.inc();
    counter.inc();
    assertEqual(counter.get(), 3, "iife scope: after 3 inc");
}

// ============================================================
// 8. 表引用语义
// ============================================================
function test_table_reference()
{
    printf("--- test_table_reference ---");

    // 表作为函数参数是引用传递
    function modify(t)
    {
        t.x = 100;
        t.y = 200;
    }

    var obj = {x = 1, y = 2};
    modify(obj);
    assertEqual(obj.x, 100, "table ref: modified x");
    assertEqual(obj.y, 200, "table ref: modified y");

    // 列表作为参数也是引用传递
    function appendItems(lst)
    {
        lst:append(4);
        lst:append(5);
    }

    var myList = [1, 2, 3];
    appendItems(myList);
    assertEqual(list.size(myList), 5, "list ref: size after append");
    assertEqual(myList[3], 4, "list ref: appended value 1");
    assertEqual(myList[4], 5, "list ref: appended value 2");

    // 重新赋值参数不影响外部
    function reassign(t)
    {
        t = {x = 999};
    }

    var obj2 = {x = 1};
    reassign(obj2);
    assertEqual(obj2.x, 1, "table ref: reassign does not affect outer");
}

// ============================================================
// 9. 错误边界 - pcall 捕获各种错误
// ============================================================
function test_error_boundaries()
{
    printf("--- test_error_boundaries ---");

    // nil 上调用方法
    var ok, err = pcall(function()
    {
        var x = nil;
        return x.field;
    });
    assert(ok != 0, "error: nil field access caught");

    // nil 算术运算
    ok, err = pcall(function()
    {
        var x = nil;
        return x + 1;
    });
    assert(ok != 0, "error: nil arithmetic caught");

    // 调用非函数值
    ok, err = pcall(function()
    {
        var x = 42;
        return x();
    });
    assert(ok != 0, "error: call non-function caught");

    // 嵌套 pcall - 内层错误不影响外层
    var outerOk, outerResult = pcall(function()
    {
        var innerOk, innerErr = pcall(function()
        {
            var x = nil;
            return x.y.z;
        });
        assert(innerOk != 0, "nested error: inner caught");
        return 999;
    });
    assertEqual(outerOk, 0, "nested error: outer success");
}

// ============================================================
// 10. 复杂表达式组合
// ============================================================
function test_complex_expressions()
{
    printf("--- test_complex_expressions ---");

    // 三元表达式嵌套在函数调用中
    function pick(flag, a, b)
    {
        return (flag) ? a or b;
    }
    assertEqual(pick(1, 10, 20), 10, "complex: ternary in func true");
    assertEqual(pick(0, 10, 20), 20, "complex: ternary in func false");

    // 表达式作为表键值
    var t = {};
    t[1 + 2] = "three";
    t[2 * 3] = "six";
    assertEqual(t[3], "three", "complex: computed key 1+2");
    assertEqual(t[6], "six", "complex: computed key 2*3");

    // 函数调用结果作为索引
    function getIndex() { return 2; }
    var arr = [10, 20, 30, 40];
    assertEqual(arr[getIndex()], 30, "complex: func result as index");

    // 链式比较逻辑
    var x = 5;
    var inRange = (x >= 1) && (x <= 10);
    assertEqual(inRange, 1, "complex: range check true");
    var outRange = (x >= 10) && (x <= 20);
    assertEqual(outRange, 0, "complex: range check false");

    // 复合表达式优先级
    var v = 2 + 3 * 4 - 1;
    assertEqual(v, 13, "complex: 2+3*4-1 = 13");
    var v2 = (2 + 3) * (4 - 1);
    assertEqual(v2, 15, "complex: (2+3)*(4-1) = 15");
}

// ============================================================
// 11. 链式方法调用（冒号语法）
// ============================================================
function test_method_chaining()
{
    printf("--- test_method_chaining ---");

    // 构建一个支持链式调用的 builder
    function make_builder()
    {
        var data = {items = [], separator = ","};

        data.add = function(self, item)
        {
            self.items:append(item);
            return self;
        };

        data.sep = function(self, s)
        {
            self.separator = s;
            return self;
        };

        data.build = function(self)
        {
            var result = "";
            for (var i = 0; i < list.size(self.items); i++)
            {
                if (i > 0) result $= self.separator;
                result $= toString(self.items[i]);
            }
            return result;
        };

        return data;
    }

    var builder = make_builder();
    builder:add("a");
    builder:add("b");
    builder:add("c");
    assertEqual(builder:build(), "a,b,c", "chain: default separator");

    var builder2 = make_builder();
    builder2:sep("-");
    builder2:add(1);
    builder2:add(2);
    builder2:add(3);
    assertEqual(builder2:build(), "1-2-3", "chain: custom separator");
}

// ============================================================
// 12. 全局变量与作用域交互
// ============================================================
function test_global_scope()
{
    printf("--- test_global_scope ---");

    // 函数内修改全局变量
    gTestCounter = 0;

    function incrementGlobal()
    {
        gTestCounter++;
    }

    incrementGlobal();
    incrementGlobal();
    incrementGlobal();
    assertEqual(gTestCounter, 3, "global: increment from function");

    // 闭包捕获全局变量
    function makeGlobalReader()
    {
        return function() { return gTestCounter; };
    }

    var reader = makeGlobalReader();
    assertEqual(reader(), 3, "global: closure reads global");
    gTestCounter = 100;
    assertEqual(reader(), 100, "global: closure sees global change");

    // 运行时字符串形式的动态全局
    setglobal("gDynamicValue", 321);
    assertEqual(getglobal("gDynamicValue"), 321, "global: getglobal after setglobal");
    assertEqual(hasglobal("gDynamicValue"), 1, "global: hasglobal true");
    assertEqual(gDynamicValue, 321, "global: dynamic value visible by identifier");

    var allGlobals = globals();
    assertEqual(allGlobals.gDynamicValue, 321, "global: globals snapshot contains value");

    delglobal("gDynamicValue");
    assertEqual(getglobal("gDynamicValue"), nil, "global: getglobal after delglobal");
    assertEqual(hasglobal("gDynamicValue"), 0, "global: hasglobal false after delglobal");
}

// ============================================================
// 13. 表的动态键与遍历
// ============================================================
function test_table_dynamic()
{
    printf("--- test_table_dynamic ---");

    // 动态生成键
    var t = {};
    for (var i = 0; i < 5; i++)
    {
        var key = "key" $ toString(i);
        t[key] = i * 10;
    }
    assertEqual(t["key0"], 0, "dynamic key: key0");
    assertEqual(t["key3"], 30, "dynamic key: key3");
    assertEqual(t["key4"], 40, "dynamic key: key4");

    // 表遍历求和
    var sum = 0;
    foreach (k, v in ipairs(t))
    {
        sum += v;
    }
    assertEqual(sum, 100, "dynamic key: sum of values");

    // 嵌套表动态构建
    var matrix = {};
    for (var row = 0; row < 3; row++)
    {
        matrix[row] = {};
        for (var col = 0; col < 3; col++)
        {
            matrix[row][col] = row * 3 + col;
        }
    }
    assertEqual(matrix[0][0], 0, "matrix: [0][0]");
    assertEqual(matrix[1][2], 5, "matrix: [1][2]");
    assertEqual(matrix[2][2], 8, "matrix: [2][2]");
}

// ============================================================
// 14. 高阶函数模式
// ============================================================
function test_higher_order()
{
    printf("--- test_higher_order ---");

    // map 函数
    function map(lst, fn)
    {
        var result = [];
        for (var i = 0; i < list.size(lst); i++)
        {
            result:append(fn(lst[i]));
        }
        return result;
    }

    var doubled = map([1, 2, 3, 4, 5], lambda x: x * 2);
    assertEqual(doubled[0], 2, "map: first doubled");
    assertEqual(doubled[4], 10, "map: last doubled");

    // filter 函数
    function filter(lst, pred)
    {
        var result = [];
        for (var i = 0; i < list.size(lst); i++)
        {
            if (pred(lst[i]))
            {
                result:append(lst[i]);
            }
        }
        return result;
    }

    var evens = filter([1, 2, 3, 4, 5, 6], lambda x: x % 2 == 0);
    assertEqual(list.size(evens), 3, "filter: 3 evens");
    assertEqual(evens[0], 2, "filter: first even");
    assertEqual(evens[2], 6, "filter: last even");

    // reduce 函数
    function reduce(lst, fn, init)
    {
        var acc = init;
        for (var i = 0; i < list.size(lst); i++)
        {
            acc = fn(acc, lst[i]);
        }
        return acc;
    }

    var total = reduce([1, 2, 3, 4, 5], lambda acc, x: acc + x, 0);
    assertEqual(total, 15, "reduce: sum");

    var product = reduce([1, 2, 3, 4, 5], lambda acc, x: acc * x, 1);
    assertEqual(product, 120, "reduce: product");

    // compose 函数
    function compose(f, g)
    {
        return function(x) { return f(g(x)); };
    }

    var double_then_add1 = compose(lambda x: x + 1, lambda x: x * 2);
    assertEqual(double_then_add1(5), 11, "compose: (5*2)+1 = 11");
}

// ============================================================
// 15. 面向对象模式（基于元表）
// ============================================================
function test_oop_pattern()
{
    printf("--- test_oop_pattern ---");

    // 类定义
    function newClass()
    {
        var cls = {};
        cls.__index = cls;

        cls.new = function(self, name, age)
        {
            var instance = {name = name, age = age};
            setmetatable(instance, self);
            return instance;
        };

        cls.greet = function(self)
        {
            return "Hello, I am " $ self.name;
        };

        cls.getAge = function(self)
        {
            return self.age;
        };

        return cls;
    }

    var Person = newClass();
    var p1 = Person:new("Alice", 30);
    var p2 = Person:new("Bob", 25);

    assertEqual(p1.name, "Alice", "oop: p1 name");
    assertEqual(p2.name, "Bob", "oop: p2 name");
    assertEqual(p1:greet(), "Hello, I am Alice", "oop: p1 greet");
    assertEqual(p2:greet(), "Hello, I am Bob", "oop: p2 greet");
    assertEqual(p1:getAge(), 30, "oop: p1 age");
    assertEqual(p2:getAge(), 25, "oop: p2 age");

    // 实例独立性
    p1.age = 31;
    assertEqual(p1:getAge(), 31, "oop: p1 age modified");
    assertEqual(p2:getAge(), 25, "oop: p2 age unchanged");
}

// ============================================================
// 16. 复杂闭包场景 - 闭包工厂
// ============================================================
function test_closure_factory()
{
    printf("--- test_closure_factory ---");

    // 生成一系列带状态的闭包
    function makeOperators(base)
    {
        return {
            add = function(x) { return base + x; },
            sub = function(x) { return base - x; },
            mul = function(x) { return base * x; },
            getBase = function() { return base; }
        };
    }

    var ops10 = makeOperators(10);
    var ops20 = makeOperators(20);

    assertEqual(ops10.add(5), 15, "factory: ops10.add(5)");
    assertEqual(ops10.sub(3), 7, "factory: ops10.sub(3)");
    assertEqual(ops10.mul(4), 40, "factory: ops10.mul(4)");
    assertEqual(ops20.add(5), 25, "factory: ops20.add(5)");
    assertEqual(ops20.getBase(), 20, "factory: ops20.getBase()");

    // 闭包数组 - 使用函数包装创建独立绑定
    function makeCapture(v) { return lambda : v; }
    var funcs = [];
    for (var i = 0; i < 5; i++)
    {
        funcs:append(makeCapture(i));
    }
    assertEqual(funcs[0](), 0, "closure array: [0]");
    assertEqual(funcs[2](), 2, "closure array: [2]");
    assertEqual(funcs[4](), 4, "closure array: [4]");
}

// ============================================================
// 17. 多返回值高级用法
// ============================================================
function test_multi_return_advanced()
{
    printf("--- test_multi_return_advanced ---");

    // 多返回值直接用于函数参数
    function getCoords()
    {
        return 10, 20;
    }

    function distance(x, y)
    {
        return x + y;
    }

    var x, y = getCoords();
    assertEqual(distance(x, y), 30, "multi return: as args");

    // 多返回值丢弃多余值
    function threeValues()
    {
        return 1, 2, 3;
    }

    var a, b = threeValues();
    assertEqual(a, 1, "multi return: discard extra a");
    assertEqual(b, 2, "multi return: discard extra b");

    // 单变量接收多返回值只取第一个
    var single = threeValues();
    assertEqual(single, 1, "multi return: single var gets first");
}

// ============================================================
// 18. 字符串作为不可变值
// ============================================================
function test_string_immutable()
{
    printf("--- test_string_immutable ---");

    var s = "hello";
    var s2 = s;
    s2 = s2 $ " world";

    assertEqual(s, "hello", "string immutable: original unchanged");
    assertEqual(s2, "hello world", "string immutable: new string created");

    // 字符串比较
    var a = "test";
    var b = "te" $ "st";
    assertEqual(a == b, 1, "string: concatenated equals literal");

    // 空字符串操作
    var empty = "";
    assertEqual(empty $ "abc", "abc", "string: empty concat");
    assertEqual("abc" $ empty, "abc", "string: concat empty");
    assertEqual(empty $ empty, "", "string: empty concat empty");
}

// ============================================================
// 19. 数值边界
// ============================================================
function test_numeric_edges()
{
    printf("--- test_numeric_edges ---");

    // 大数运算
    var big = 1000000 * 1000;
    assertEqual(big, 1000000000, "numeric: billion");

    // 负数运算
    assertEqual(-10 * -10, 100, "numeric: neg * neg");
    assertEqual(-10 * 10, -100, "numeric: neg * pos");
    assertEqual(10 / -2, -5, "numeric: pos / neg");

    // 零相关
    assertEqual(0 * 999999, 0, "numeric: zero mul");
    assertEqual(0 + 0, 0, "numeric: zero add zero");

    // 浮点精度
    var f = 0.1 + 0.2;
    assert(f > 0.29 && f < 0.31, "numeric: float precision 0.1+0.2");

    // 整数与浮点混合运算
    var mixed = 10 + 0.5;
    assert(mixed > 10.4 && mixed < 10.6, "numeric: int + float");
}

// ============================================================
// 20. for 循环高级模式
// ============================================================
function test_for_advanced()
{
    printf("--- test_for_advanced ---");

    // 倒序循环
    var result = [];
    for (var i = 5; i >= 1; i--)
    {
        result:append(i);
    }
    assertEqual(result[0], 5, "for reverse: first");
    assertEqual(result[4], 1, "for reverse: last");
    assertEqual(list.size(result), 5, "for reverse: count");

    // 步长大于1
    var evens = [];
    for (var i = 0; i <= 10; i += 2)
    {
        evens:append(i);
    }
    assertEqual(list.size(evens), 6, "for step2: count");
    assertEqual(evens[0], 0, "for step2: first");
    assertEqual(evens[5], 10, "for step2: last");

    // 循环中创建闭包 - 使用函数包装创建独立绑定
    function makeMul10(v) { return function() { return v * 10; }; }
    var closures = [];
    for (var i = 0; i < 3; i++)
    {
        closures:append(makeMul10(i));
    }
    assertEqual(closures[0](), 0, "for closure: [0]");
    assertEqual(closures[1](), 10, "for closure: [1]");
    assertEqual(closures[2](), 20, "for closure: [2]");
}

// ============================================================
// 21. 列表高级操作
// ============================================================
function test_list_advanced()
{
    printf("--- test_list_advanced ---");

    // 列表作为栈使用 (push/pop via append/removeAt)
    var stack = [];
    stack:append(1);
    stack:append(2);
    stack:append(3);
    var top = stack[list.size(stack) - 1];
    stack:removeAt(list.size(stack) - 1);
    assertEqual(top, 3, "list stack: pop top");
    assertEqual(list.size(stack), 2, "list stack: size after pop");

    // 列表作为队列使用
    var queue = [];
    queue:append("a");
    queue:append("b");
    queue:append("c");
    var front = queue[0];
    queue:removeAt(0);
    assertEqual(front, "a", "list queue: dequeue front");
    assertEqual(queue[0], "b", "list queue: new front");
    assertEqual(list.size(queue), 2, "list queue: size after dequeue");

    // 列表嵌套
    var nested = [[1, 2], [3, 4], [5, 6]];
    assertEqual(nested[0][0], 1, "nested list: [0][0]");
    assertEqual(nested[1][1], 4, "nested list: [1][1]");
    assertEqual(nested[2][0], 5, "nested list: [2][0]");

    // 列表拷贝（浅拷贝）
    function copyList(src)
    {
        var dst = [];
        for (var i = 0; i < list.size(src); i++)
        {
            dst:append(src[i]);
        }
        return dst;
    }

    var original = [10, 20, 30];
    var copied = copyList(original);
    copied[0] = 99;
    assertEqual(original[0], 10, "list copy: original unchanged");
    assertEqual(copied[0], 99, "list copy: copy modified");
}

// ============================================================
// 22. 协程与闭包交互
// ============================================================
function test_coroutine_closure()
{
    printf("--- test_coroutine_closure ---");

    // 协程体是闭包
    var shared = 0;

    function makeCoroutine()
    {
        return coroutine.create(function()
        {
            shared = 10;
            coroutine.yield();
            shared = 20;
            coroutine.yield();
            shared = 30;
        });
    }

    var co = makeCoroutine();
    assertEqual(shared, 0, "coro closure: initial");

    coroutine.resume(co);
    assertEqual(shared, 10, "coro closure: after first yield");

    coroutine.resume(co);
    assertEqual(shared, 20, "coro closure: after second yield");

    coroutine.resume(co);
    assertEqual(shared, 30, "coro closure: after finish");
}

// ============================================================
// 23. 算法：快速排序
// ============================================================
function test_quicksort()
{
    printf("--- test_quicksort ---");

    function quicksort(arr, low, high)
    {
        if (low < high)
        {
            var pivot = arr[high];
            var i = low - 1;
            for (var j = low; j < high; j++)
            {
                if (arr[j] <= pivot)
                {
                    i++;
                    var tmp = arr[i];
                    arr[i] = arr[j];
                    arr[j] = tmp;
                }
            }
            var tmp = arr[i + 1];
            arr[i + 1] = arr[high];
            arr[high] = tmp;
            var pi = i + 1;

            quicksort(arr, low, pi - 1);
            quicksort(arr, pi + 1, high);
        }
    }

    var data = [38, 27, 43, 3, 9, 82, 10];
    quicksort(data, 0, list.size(data) - 1);

    assertEqual(data[0], 3, "quicksort: first");
    assertEqual(data[1], 9, "quicksort: second");
    assertEqual(data[2], 10, "quicksort: third");
    assertEqual(data[6], 82, "quicksort: last");

    // 验证有序性
    var sorted = 1;
    for (var i = 0; i < list.size(data) - 1; i++)
    {
        if (data[i] > data[i + 1])
        {
            sorted = 0;
            break;
        }
    }
    assertEqual(sorted, 1, "quicksort: fully sorted");
}

// ============================================================
// 24. 算法：二分查找
// ============================================================
function test_binary_search()
{
    printf("--- test_binary_search ---");
    function binarySearch(arr, target)
    {
        var low = 0;
        var high = list.size(arr) - 1;
        while (low <= high)
        {
            var mid = (low + high) / 2;
            if (arr[mid] == target)
            {
                return mid;
            }
            else if (arr[mid] < target)
            {
                low = mid + 1;
            }
            else
            {
                high = mid - 1;
            }
        }
        return -1;
    }

    var sorted = [2, 5, 8, 12, 16, 23, 38, 56, 72, 91];
    assertEqual(binarySearch(sorted, 23), 5, "bsearch: found 23 at index 5");
    assertEqual(binarySearch(sorted, 2), 0, "bsearch: found 2 at index 0");
    assertEqual(binarySearch(sorted, 91), 9, "bsearch: found 91 at index 9");
    assertEqual(binarySearch(sorted, 50), -1, "bsearch: not found returns -1");
}

// ============================================================
// 25. 状态机模式
// ============================================================
function test_state_machine()
{
    printf("--- test_state_machine ---");

    function createStateMachine()
    {
        var state = "idle";
        var log = [];

        var machine = {};
        machine.transition = function(self, event)
        {
            if (state == "idle" && event == "start")
            {
                state = "running";
            }
            else if (state == "running" && event == "pause")
            {
                state = "paused";
            }
            else if (state == "paused" && event == "resume")
            {
                state = "running";
            }
            else if (state == "running" && event == "stop")
            {
                state = "stopped";
            }
            log:append(state);
            return self;
        };

        machine.getState = function(self)
        {
            return state;
        };

        machine.getLog = function(self)
        {
            return log;
        };

        return machine;
    }

    var sm = createStateMachine();
    assertEqual(sm:getState(), "idle", "state machine: initial");

    sm:transition("start");
    assertEqual(sm:getState(), "running", "state machine: after start");

    sm:transition("pause");
    assertEqual(sm:getState(), "paused", "state machine: after pause");

    sm:transition("resume");
    assertEqual(sm:getState(), "running", "state machine: after resume");

    sm:transition("stop");
    assertEqual(sm:getState(), "stopped", "state machine: after stop");

    var log = sm:getLog();
    assertEqual(list.size(log), 4, "state machine: log has 4 entries");
}

// ============================================================
// 26. 数值与表达式暴力测试
// ============================================================
function test_numeric_bruteforce()
{
    printf("--- test_numeric_bruteforce ---");

    var sum = 0;
    var evenSum = 0;
    var oddSum = 0;
    for (var i = 0; i < 100; i++)
    {
        sum += i;
        if (i % 2 == 0)
        {
            evenSum += i;
        }
        else
        {
            oddSum += i;
        }
    }
    assertEqual(sum, 4950, "numeric brute: sum 0..99");
    assertEqual(evenSum, 2450, "numeric brute: even sum");
    assertEqual(oddSum, 2500, "numeric brute: odd sum");

    var checksum = 0;
    for (var j = 0; j < 120; j++)
    {
        checksum += (j * j) % 37;
    }
    assertEqual(checksum, 2128, "numeric brute: square modulo checksum");

    var precedenceChecksum = 0;
    for (var k = 0; k < 50; k++)
    {
        precedenceChecksum += k + k * 2 - (k % 3) * 4;
    }
    assertEqual(precedenceChecksum, 3479, "numeric brute: precedence checksum");
}

// ============================================================
// 27. 表和列表压力测试
// ============================================================
function test_table_list_stress()
{
    printf("--- test_table_list_stress ---");

    var arr = [];
    for (var i = 0; i < 120; i++)
    {
        arr:append((i * i) % 37);
    }
    assertEqual(list.size(arr), 120, "list stress: size after append");

    var arrChecksum = 0;
    for (i = 0; i < list.size(arr); i++)
    {
        arrChecksum += arr[i];
    }
    assertEqual(arrChecksum, 2128, "list stress: checksum");

    var dict = {};
    for (i = 0; i < 150; i++)
    {
        var key = "k" $ i;
        dict[key] = (i * 7 + 3) % 41;
    }

    var dictChecksum = 0;
    for (i = 0; i < 150; i++)
    {
        key = "k" $ i;
        dictChecksum += dict[key];
    }
    assertEqual(dictChecksum, 2989, "table stress: dynamic key checksum");

    var root = {value = 0};
    var cur = root;
    for (i = 1; i <= 30; i++)
    {
        cur.next = {value = i};
        cur = cur.next;
    }

    cur = root.next;
    var depthSum = 0;
    while (cur != nil)
    {
        depthSum += cur.value;
        cur = cur.next;
    }
    assertEqual(depthSum, 465, "table stress: nested chain checksum");
}

// ============================================================
// 28. 闭包和短路副作用压力测试
// ============================================================
function test_closure_logic_stress()
{
    printf("--- test_closure_logic_stress ---");

    function makeAccumulator(init)
    {
        var total = init;
        return function(delta)
        {
            total += delta;
            return total;
        };
    }

    var acc = makeAccumulator(0);
    var closureChecksum = 0;
    for (var i = 1; i <= 50; i++)
    {
        closureChecksum += acc(i);
    }
    assertEqual(closureChecksum, 22100, "closure stress: accumulated triangular checksum");

    var calls = 0;
    function side(value)
    {
        calls++;
        return value;
    }

    var logicChecksum = 0;
    for (i = 0; i < 30; i++)
    {
        var left = i % 3;
        logicChecksum += left || side(i + 100);
    }
    assertEqual(logicChecksum, 1165, "logic stress: or selected operand checksum");
    assertEqual(calls, 10, "logic stress: or side effect count");

    calls = 0;
    var andChecksum = 0;
    for (i = 0; i < 30; i++)
    {
        left = i % 4;
        andChecksum += left && side(i + 1);
    }
    assertEqual(calls, 22, "logic stress: and side effect count");
    assertEqual(andChecksum, 345, "logic stress: and selected operand checksum");
}

// ============================================================
// 29. 动态编译边界压力测试
// ============================================================
function test_loadstring_boundary_stress()
{
    printf("--- test_loadstring_boundary_stress ---");

    var ok, fn = loadstring("var sum = 0; for (var i = 0; i < 20; i++) { sum += i; } return sum");
    assertEqual(ok, 1, "loadstring stress: compile loop chunk");
    assertEqual(fn(), 190, "loadstring stress: loop chunk result");

    var bad = 0;
    bad += loadstring("var a = ;");
    bad += loadstring("return (1).x");
    bad += loadstring("return nil.x");
    bad += loadstring("var __xscript_internal_reg_1 = 2;");
    assertEqual(bad, 0, "loadstring stress: invalid chunks return 0");
}

// ============================================================
// 30. 括号表达式语句测试 (paren expression as statement)
// 测试 (var).field 赋值、复合赋值、多重赋值、列表/表字面量访问等
// ============================================================
function test_paren_expr_statement()
{
    printf("--- test_paren_expr_statement ---");

    // --- 基本读取 ---
    var t = {a = 1, b = 2, c = 3};
    assertEqual((t).a, 1, "paren read: (t).a");
    assertEqual((t)["b"], 2, "paren read: (t)[b]");

    // --- 基本赋值 ---
    (t).a = 10;
    assertEqual(t.a, 10, "paren assign: (t).a = 10");

    (t)["b"] = 20;
    assertEqual(t.b, 20, "paren assign: (t)[b] = 20");

    // --- 复合赋值 ---
    (t).a += 5;
    assertEqual(t.a, 15, "paren compound: (t).a += 5");

    (t).c -= 1;
    assertEqual(t.c, 2, "paren compound: (t).c -= 1");

    (t).a *= 2;
    assertEqual(t.a, 30, "paren compound: (t).a *= 2");

    // --- 自增自减 ---
    (t).a++;
    assertEqual(t.a, 31, "paren postfix: (t).a++");

    (t).a--;
    assertEqual(t.a, 30, "paren postfix: (t).a--");

    // --- 多重赋值 ---
    (t).a, (t).b = 100, 200;
    assertEqual(t.a, 100, "paren multi assign: (t).a");
    assertEqual(t.b, 200, "paren multi assign: (t).b");

    // --- 嵌套括号 ---
    ((t)).c = 99;
    assertEqual(t.c, 99, "paren nested: ((t)).c = 99");

    (((t))).a = 77;
    assertEqual(t.a, 77, "paren nested: (((t))).a = 77");

    // --- 嵌套表访问 ---
    var outer = {inner = {val = 0}};
    (outer).inner.val = 42;
    assertEqual(outer.inner.val, 42, "paren nested table: (outer).inner.val");

    (outer)["inner"]["val"] = 88;
    assertEqual(outer.inner.val, 88, "paren nested table bracket: (outer)[inner][val]");

    // --- 方法调用 ---
    var obj = {x = 10};
    obj.getX = function() { return obj.x; };
    assertEqual((obj).getX(), 10, "paren method call: (obj).getX()");

    // --- 列表字面量下标访问（表达式中） ---
    var v = [10, 20, 30][1];
    assertEqual(v, 20, "list literal index: [10,20,30][1]");

    var v2 = ["a", "b", "c"][0];
    assertEqual(v2, "a", "list literal index: [a,b,c][0]");

    // --- 表字面量字段访问（表达式中） ---
    var v3 = {name = "test", val = 42}.val;
    assertEqual(v3, 42, "table literal field: {}.val");

    var v4 = {name = "test", val = 42}["name"];
    assertEqual(v4, "test", "table literal bracket: {}[name]");

    // --- 函数返回值的括号访问 ---
    function makeTable() { return {x = 5, y = 10}; }
    var fx = (makeTable()).x;
    assertEqual(fx, 5, "paren func return: (makeTable()).x");

    // --- 混合场景：括号变量 + 冒号调用 ---
    var lst = [3, 1, 2];
    (lst):sort();
    assertEqual(lst[0], 1, "paren colon call: (lst):sort() first");
    assertEqual(lst[2], 3, "paren colon call: (lst):sort() last");

    // --- 括号变量 + append ---
    var lst2 = [1, 2];
    (lst2):append(3);
    assertEqual(list.size(lst2), 3, "paren colon call: (lst2):append size");
    assertEqual(lst2[2], 3, "paren colon call: (lst2):append value");
}
