// ============================================================
// XScript 综合测试 - 基础语法
// 覆盖：变量声明、算术运算符、位运算符、比较运算符、
//        逻辑运算符、字符串拼接
// ============================================================

// ============================================================
// 1. 变量声明与赋值
// ============================================================

// ============================================================
// XScript 综合测试 - 公共辅助函数
// ============================================================


function test_variable()
{
    printf("--- test_variable ---");
    // var 声明
    var a = 10;
    assertEqual(a, 10, "var int");

    var b = 3.14;
    // float 近似比较
    assert(b > 3.0 && b < 4.0, "var float");

    var s = "hello";
    assertEqual(s, "hello", "var string");
    
    // 多变量声明
    var x, y = 1, 2;
    assertEqual(x, 1, "multi var x");
    assertEqual(y, 2, "multi var y");

    // nil
    var n = nil;
    assertEqual(n, nil, "var nil");

    // 重新赋值
    a = 20;
    assertEqual(a, 20, "reassign");

    // 全局变量赋值（顶层变量自动成为全局变量）
    gTestGlobal = 42;
    assertEqual(gTestGlobal, 42, "global var");
}

function test_nil_constant_regression()
{
    printf("--- test_nil_constant_regression ---");

    var before = nil;
    assertEqual(type(before), "nil", "nil constant: local literal type");

    setglobal("nil", 123);
    assertEqual(getglobal("nil"), 123, "nil constant: dynamic global slot can be changed");
    assertEqual(type(nil), "nil", "nil constant: literal ignores dynamic global slot");
    assertEqual(nil == before, 1, "nil constant: existing literal remains nil");

    var after = nil;
    assertEqual(type(after), "nil", "nil constant: new literal remains nil");
    assertEqual(after || "fallback", "fallback", "nil constant: logical or still treats nil as false");

    delglobal("nil");
    assertEqual(getglobal("nil"), nil, "nil constant: dynamic global slot deleted");
    assertEqual(type(nil), "nil", "nil constant: literal remains nil after delete");
}

// ============================================================
// 2. 算术运算符
// ============================================================
function test_arithmetic()
{
    printf("--- test_arithmetic ---");

    assertEqual(3 + 4, 7, "add");
    assertEqual(10 - 3, 7, "sub");
    assertEqual(6 * 7, 42, "mul");
    assertEqual(15 / 4, 3, "div int");
    assertEqual(17 % 5, 2, "mod");
    assertEqual(2 ^ 10, 1024, "pow");

    // 负数
    assertEqual(-5 + 3, -2, "neg");

    // 自增自减
    var a = 5;
    a++;
    assertEqual(a, 6, "inc");
    a--;
    assertEqual(a, 5, "dec");

    // 复合赋值
    var b = 10;
    b += 5;
    assertEqual(b, 15, "+=");
    b -= 3;
    assertEqual(b, 12, "-=");
    b *= 2;
    assertEqual(b, 24, "*=");
    b /= 4;
    assertEqual(b, 6, "/=");
    b %= 4;
    assertEqual(b, 2, "%=");
}

// ============================================================
// 3. 位运算符
// ============================================================
function test_bitwise()
{
    printf("--- test_bitwise ---");

    // AND
    assertEqual(12 & 10, 8, "bitwise and");
    // OR
    assertEqual(12 | 10, 14, "bitwise or");
    // XOR (#)
    assertEqual(12 # 10, 6, "bitwise xor");

    // 左移
    assertEqual(1 << 4, 16, "shift left");
    // 右移
    assertEqual(16 >> 2, 4, "shift right");

    // 复合赋值
    var a = 15;
    a &= 6;
    assertEqual(a, 6, "&=");
    a |= 8;
    assertEqual(a, 14, "|=");
    a #= 3;
    assertEqual(a, 13, "#=");
}

// ============================================================
// 4. 比较运算符
// ============================================================
function test_comparison()
{
    printf("--- test_comparison ---");

    assertEqual(5 == 5, 1, "eq int");
    assertEqual(5 != 3, 1, "neq int");
    assertEqual(3 < 5, 1, "lt");
    assertEqual(5 > 3, 1, "gt");
    assertEqual(3 <= 3, 1, "le equal");
    assertEqual(3 <= 5, 1, "le less");
    assertEqual(5 >= 5, 1, "ge equal");
    assertEqual(5 >= 3, 1, "ge greater");

    // string comparison
    assertEqual("abc" == "abc", 1, "eq str");
    assertEqual("abc" != "def", 1, "neq str");

    // nil comparison
    assertEqual(nil == nil, 1, "eq nil");

    var ts1 = 1
    if(ts1 >0 || ts2:T() < 0)
    {
        printf("test || Success")
    }

    if(ts1 < 0 && ts2:T() < 0)
    {
    
    }
    else
    {
        printf("test && Success")
    }

}

// ============================================================
// 5. 逻辑运算符
// ============================================================
function test_logical()
{
    printf("--- test_logical ---");

    assertEqual(1 && 1, 1, "and true");
    assertEqual(1 && 0, 0, "and false");
    assertEqual(0 || 1, 1, "or true");
    assertEqual(0 || 0, 0, "or false");
    assertEqual("left" || "right", "left", "or returns left truthy operand");
    assertEqual(nil || "fallback", "fallback", "or returns rhs when left nil");
    assertEqual("left" && "right", "right", "and returns rhs when left truthy");
    assertEqual(nil && "right", nil, "and returns left nil without rhs");
    assertEqual(!0, 1, "not true");
    assertEqual(!1, 0, "not false");

    var called = 0;
    function touchLogical()
    {
        called++;
        return "touched";
    }
    assertEqual(1 || 0 && touchLogical(), 1, "and has higher priority than or");
    assertEqual(called, 0, "priority and short circuit skip rhs");
    assertEqual(0 && touchLogical() || "fallback", "fallback", "and/or return selected operand");
    assertEqual(called, 0, "and short circuit keeps rhs skipped before or");

    assertEqual("a" && "b" && "c", "c", "and chain returns last truthy operand");
    assertEqual(0 || nil || "last", "last", "or chain skips false operands");
    assertEqual("first" || touchLogical() || "last", "first", "or chain returns first truthy operand");
    assertEqual(called, 0, "or chain skips later calls");
    assertEqual("x" && 0 || "fallback", "fallback", "zero false inside mixed logic");
    assertEqual(nil || "a" && "b", "b", "and priority higher than or with values");

    var fallbackTable = {name = "fallback"};
    var selectedTable = nil || fallbackTable;
    assertEqual(selectedTable.name, "fallback", "or returns table operand");
    var selectedRight = fallbackTable && {name = "right"};
    assertEqual(selectedRight.name, "right", "and returns table rhs operand");

    var stressCount = 0;
    function stressTouch(value)
    {
        stressCount++;
        return value;
    }
    for (var i = 0; i < 20; i++)
    {
        assertEqual(1 || stressTouch(i), 1, "stress or skips rhs");
        assertEqual(0 && stressTouch(i), 0, "stress and skips rhs");
        assertEqual(0 || stressTouch(i), i, "stress or evaluates rhs");
    }
    assertEqual(stressCount, 20, "stress short circuit call count");

    var nilBranch = 0;
    if (nil)
    {
        nilBranch = 1;
    }
    assertEqual(nilBranch, 0, "nil is false in condition");

}

// ============================================================
// 6. 字符串拼接 ($)
// ============================================================
function test_string_concat()
{
    printf("--- test_string_concat ---");

    var s = "hello" $ " " $ "world";
    assertEqual(s, "hello world", "concat");

    var a = "foo";
    a $= "bar";
    assertEqual(a, "foobar", "$=");

    // 数字拼接
    var n = "value=" $ 42;
    assertEqual(n, "value=42", "concat with int");
}

// ============================================================
// 7. 变量边界情况
// ============================================================
function test_variable_edges()
{
    printf("--- test_variable_edges ---");

    var noInit;
    assertEqual(noInit, nil, "var without init is nil");

    var a, b, c = 1, 2, 3;
    assertEqual(a, 1, "multi var edge a");
    assertEqual(b, 2, "multi var edge b");
    assertEqual(c, 3, "multi var edge c");

    var copy = a;
    copy = 99;
    assertEqual(a, 1, "primitive copy unchanged");

    var ref1 = {value = 10};
    var ref2 = ref1;
    ref2.value = 20;
    assertEqual(ref1.value, 20, "table assignment keeps reference");

    var 中文变量 = 12;
    assertEqual(中文变量, 12, "chinese identifier variable");

    function 中文加法(甲, 乙)
    {
        return 甲 + 乙;
    }
    assertEqual(中文加法(3, 4), 7, "chinese identifier function");
}

// ============================================================
// 8. 运算符边界情况
// ============================================================
function test_operator_edges()
{
    printf("--- test_operator_edges ---");

    assertEqual(0 - -5, 5, "double negative arithmetic");
    assertEqual((1 << 3) >> 1, 4, "shift chain");
    assertEqual(5 == 3, 0, "eq false");
    assertEqual(5 != 5, 0, "neq false");
    assertEqual("abc" < "abd", 1, "string less than");

    var f = 7.5 / 2.5;
    assert(f > 2.99 && f < 3.01, "float division precision");

    var called = 0;
    function touch()
    {
        called++;
        return 1;
    }

    var v1 = 1 || touch();
    assertEqual(called, 0, "or short circuit skips rhs");

    var v2 = 0 && touch();
    assertEqual(called, 0, "and short circuit skips rhs");

    var v3 = 0 || touch();
    assertEqual(called, 1, "or evaluates rhs when needed");
    assertEqual(v3, 1, "or returns rhs truthy value");
}
