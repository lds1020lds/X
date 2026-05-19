// ============================================================
// XScript 综合测试 - Block 变量作用域
// 覆盖：block内变量声明、变量shadow、嵌套block、
//        block退出后变量不可见、循环中的block变量
// ============================================================

// ============================================================
// 1. 基本 block 变量声明与可见性
// ============================================================
function test_block_basic()
{
    printf("--- test_block_basic ---");

    var x = 10;
    {
        var y = 20;
        assertEqual(x, 10, "block: outer var visible inside");
        assertEqual(y, 20, "block: inner var declared");
    }
    // y should not be visible here
    assertEqual(x, 10, "block: outer var preserved after block");
}

// ============================================================
// 2. 变量 shadow（内层block覆盖外层同名变量）
// ============================================================
function test_block_shadow()
{
    printf("--- test_block_shadow ---");

    var x = 100;
    assertEqual(x, 100, "shadow: outer x initial");
    {
        var x = 200;
        assertEqual(x, 200, "shadow: inner x shadows outer");
        x = 250;
        assertEqual(x, 250, "shadow: inner x modified");
    }
    assertEqual(x, 100, "shadow: outer x restored after block");
}

// ============================================================
// 3. 多层嵌套 block
// ============================================================
function test_block_nested()
{
    printf("--- test_block_nested ---");
    
    var a = 1;
    {
        var b = 2;
        assertEqual(a, 1, "nested: level1 sees outer a");
        assertEqual(b, 2, "nested: level1 var b");
        {
            var c = 3;
            assertEqual(a, 1, "nested: level2 sees outer a");
            assertEqual(b, 2, "nested: level2 sees level1 b");
            assertEqual(c, 3, "nested: level2 var c");
            {
                var d = 4;
                assertEqual(a, 1, "nested: level3 sees outer a");
                assertEqual(b, 2, "nested: level3 sees level1 b");
                assertEqual(c, 3, "nested: level3 sees level2 c");
                assertEqual(d, 4, "nested: level3 var d");
            }
            // d not visible here
            assertEqual(c, 3, "nested: level2 c after level3 exit");
        }
        // c not visible here
        assertEqual(b, 2, "nested: level1 b after level2 exit");
    }
    // b not visible here
    assertEqual(a, 1, "nested: outer a after all blocks exit");
}

// ============================================================
// 4. 多层 shadow 同名变量
// ============================================================
function test_block_multi_shadow()
{
    printf("--- test_block_multi_shadow ---");

    var x = 1;
    assertEqual(x, 1, "multi_shadow: outer x=1");
    {
        var x = 2;
        assertEqual(x, 2, "multi_shadow: level1 x=2");
        {
            var x = 3;
            assertEqual(x, 3, "multi_shadow: level2 x=3");
            {
                var x = 4;
                assertEqual(x, 4, "multi_shadow: level3 x=4");
            }
            assertEqual(x, 3, "multi_shadow: back to level2 x=3");
        }
        assertEqual(x, 2, "multi_shadow: back to level1 x=2");
    }
    assertEqual(x, 1, "multi_shadow: back to outer x=1");
}

// ============================================================
// 5. block 内修改外层变量
// ============================================================
function test_block_modify_outer()
{
    printf("--- test_block_modify_outer ---");

    var x = 10;
    var y = 20;
    {
        x = 50;
        assertEqual(x, 50, "modify_outer: x changed inside block");
        y = 60;
    }
    assertEqual(x, 50, "modify_outer: x change persists");
    assertEqual(y, 60, "modify_outer: y change persists");
}

// ============================================================
// 6. block 内声明多个变量
// ============================================================
function test_block_multi_vars()
{
    printf("--- test_block_multi_vars ---");

    var result = 0;
    {
        var a = 10;
        var b = 20;
        var c = 30;
        result = a + b + c;
        assertEqual(a, 10, "multi_vars: a");
        assertEqual(b, 20, "multi_vars: b");
        assertEqual(c, 30, "multi_vars: c");
    }
    assertEqual(result, 60, "multi_vars: result computed in block");
}

// ============================================================
// 7. if/else 中的 block 变量
// ============================================================
function test_block_if_else()
{
    printf("--- test_block_if_else ---");

    var x = 5;
    var result = 0;

    if (x > 3)
    {
        var temp = x * 2;
        result = temp;
        assertEqual(temp, 10, "if_block: temp in if");
    }
    else
    {
        var temp = x * 3;
        result = temp;
    }
    assertEqual(result, 10, "if_block: result from if branch");

    // Both branches can declare same-named var independently
    if (x < 3)
    {
        var msg = "less";
    }
    else
    {
        var msg = "greater_or_equal";
        assertEqual(msg, "greater_or_equal", "if_block: msg in else");
    }
}

// ============================================================
// 8. while 循环中的 block 变量
// ============================================================
function test_block_while()
{
    printf("--- test_block_while ---");

    var sum = 0;
    var i = 0;
    while (i < 5)
    {
        var temp = i * 2;
        sum += temp;
        i++;
    }
    assertEqual(sum, 20, "while_block: sum of i*2 for i=0..4");
    // temp not visible here
}

// ============================================================
// 9. for 循环中的 block 变量
// ============================================================
function test_block_for()
{
    printf("--- test_block_for ---");

    var total = 0;
    for (i = 0; i < 5; i++)
    {
        var squared = i * i;
        total += squared;
    }
    assertEqual(total, 30, "for_block: sum of squares 0..4");
    // squared not visible here
}

// ============================================================
// 10. block 变量与函数调用交互
// ============================================================
function test_block_with_function()
{
    printf("--- test_block_with_function ---");

    function helper(val)
    {
        return val + 1;
    }

    var result = 0;
    {
        var x = 10;
        var y = helper(x);
        result = y;
        assertEqual(y, 11, "block_func: helper called in block");
    }
    assertEqual(result, 11, "block_func: result preserved");
}

// ============================================================
// 11. 连续 block（同级别多个block）
// ============================================================
function test_block_sequential()
{
    printf("--- test_block_sequential ---");

    var sum = 0;
    {
        var x = 10;
        sum += x;
    }
    {
        var x = 20;
        sum += x;
    }
    {
        var x = 30;
        sum += x;
    }
    assertEqual(sum, 60, "sequential: same name in consecutive blocks");
}

// ============================================================
// 12. block 中的复合赋值运算
// ============================================================
function test_block_compound_assign()
{
    printf("--- test_block_compound_assign ---");

    var x = 100;
    {
        var x = 10;
        x += 5;
        assertEqual(x, 15, "compound: inner x += 5");
        x *= 2;
        assertEqual(x, 30, "compound: inner x *= 2");
    }
    assertEqual(x, 100, "compound: outer x unchanged");
}

// ============================================================
// 13. block 变量与闭包交互
// ============================================================
function test_block_closure()
{
    printf("--- test_block_closure ---");

    var funcs = {};
    var x = 0;
    {
        var captured = 42;
        funcs.get = lambda : captured;
    }
    // captured not visible here, but closure should still work
    assertEqual(funcs.get(), 42, "block_closure: lambda captures block var");
}

// ============================================================
// 14. 深层嵌套 + shadow + 修改外层
// ============================================================
function test_block_complex()
{
    printf("--- test_block_complex ---");

    var a = 1;
    var b = 2;
    var c = 3;
    {
        var a = 10;
        b = 20;
        assertEqual(a, 10, "complex: inner a=10");
        assertEqual(b, 20, "complex: outer b modified to 20");
        assertEqual(c, 3, "complex: outer c visible");
        {
            var c = 30;
            a = 100;
            assertEqual(a, 100, "complex: level2 modifies level1 a");
            assertEqual(c, 30, "complex: level2 c=30 shadows outer");
        }
        assertEqual(a, 100, "complex: level1 a was modified by level2");
        assertEqual(c, 3, "complex: outer c restored after level2");
    }
    assertEqual(a, 1, "complex: outer a restored");
    assertEqual(b, 20, "complex: outer b was modified");
    assertEqual(c, 3, "complex: outer c unchanged");
}

// ============================================================
// 15. block 中的循环嵌套
// ============================================================
function test_block_loop_nested()
{
    printf("--- test_block_loop_nested ---");

    var total = 0;
    {
        var multiplier = 3;
        for (i = 1; i <= 4; i++)
        {
            var temp = i * multiplier;
            total += temp;
        }
        assertEqual(total, 30, "loop_nested: sum of i*3 for i=1..4");
    }
    assertEqual(total, 30, "loop_nested: total preserved");
}

// ============================================================
// 16. block 中声明与外层同名变量并在循环中使用
// ============================================================
function test_block_shadow_in_loop()
{
    printf("--- test_block_shadow_in_loop ---");

    var x = 999;
    var sum = 0;
    for (i = 0; i < 3; i++)
    {
        var x = i;
        sum += x;
    }
    assertEqual(sum, 3, "shadow_loop: sum of shadowed x in loop");
    assertEqual(x, 999, "shadow_loop: outer x unchanged");
}
