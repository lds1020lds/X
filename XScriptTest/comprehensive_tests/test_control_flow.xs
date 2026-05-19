// ============================================================
// XScript 综合测试 - 控制流
// 覆盖：if/else、while、for、foreach...in
// ============================================================

// ============================================================
// 7. if/else 控制流
// ============================================================
function test_if_else()
{
    printf("--- test_if_else ---");

    var a = 10;
    var result = 0;

    if (a > 5)
    {
        result = 1;
    }
    assertEqual(result, 1, "if true");

    if (a < 5)
    {
        result = 2;
    }
    else
    {
        result = 3;
    }
    assertEqual(result, 3, "if else");

    if (a == 10)
    {
        result = 10;
    }
    else if (a == 20)
    {
        result = 20;
    }
    else
    {
        result = 30;
    }
    assertEqual(result, 10, "if else-if first branch");

    if (a == 5)
    {
        result = 5;
    }
    else if (a == 10)
    {
        result = 10;
    }
    else
    {
        result = 30;
    }
    assertEqual(result, 10, "if else-if second branch");
}

// ============================================================
// 8. while 循环
// ============================================================
function test_while()
{
    printf("--- test_while ---");

    var sum = 0;
    var i = 1;
    while (i <= 10)
    {
        sum += i;
        i++;
    }
    assertEqual(sum, 55, "while sum 1..10");

    // break
    sum = 0;
    i = 1;
    while (1)
    {
        if (i > 5) break;
        sum += i;
        i++;
    }
    assertEqual(sum, 15, "while break");

    // continue
    sum = 0;
    i = 0;
    while (i < 10)
    {
        i++;
        if (i % 2 == 0) continue;
        sum += i;
    }
    assertEqual(sum, 25, "while continue (odd sum)");
}

// ============================================================
// 9. for 循环
// ============================================================
function test_for()
{
    printf("--- test_for ---");

    var sum = 0;
    for (i = 1; i <= 10; i++)
    {
        sum += i;
    }
    assertEqual(sum, 55, "for sum 1..10");

    // break in for
    sum = 0;
    for (i = 1; i <= 100; i++)
    {
        if (i > 5) break;
        sum += i;
    }
    assertEqual(sum, 15, "for break");

    // continue in for
    sum = 0;
    for (i = 1; i <= 10; i++)
    {
        if (i % 2 == 0) continue;
        sum += i;
    }
    assertEqual(sum, 25, "for continue (odd sum)");

    // nested for
    var count = 0;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            count++;
        }
    }
    assertEqual(count, 9, "nested for");

    // for init local variable
    var outerI = 100;
    sum = 0;
    for (var outerI = 0; outerI < 5; outerI++)
    {
        sum += outerI;
    }
    assertEqual(sum, 10, "for local var visible in body");
    assertEqual(outerI, 100, "for local var does not modify outer var");

    // nested for init local variables
    sum = 0;
    for (var row = 1; row <= 3; row++)
    {
        for (var col = 1; col <= 2; col++)
        {
            sum += row * col;
        }
    }
    assertEqual(sum, 18, "nested for local vars");
}

// ============================================================
// 10. foreach...in 遍历
// ============================================================
function test_foreach()
{
    printf("--- test_foreach ---");

    var t = {a=1, b=2, c=3};
    var sum = 0;
    foreach (key, value in ipairs(t))
    {
        sum += value;
    }
    assertEqual(sum, 6, "foreach table");

    // ipairs 遍历数组部分
    var t2 = {10, 20, 30};
    sum = 0;
    foreach (key, value in ipairs(t2))
    {
        sum += value;
    }
    assertEqual(sum, 60, "foreach ipairs");
}

// ============================================================
// 11. 控制流边界情况
// ============================================================
function test_control_flow_edges()
{
    printf("--- test_control_flow_edges ---");

    var count = 0;
    for (var i = 10; i < 5; i++)
    {
        count++;
    }
    assertEqual(count, 0, "for zero iterations");

    var single = 0;
    for (var k = 0; k < 3; k++) single += k;
    assertEqual(single, 3, "for single statement body");

    var nested = 0;
    for (var outer = 0; outer < 3; outer++)
    {
        for (var inner = 0; inner < 3; inner++)
        {
            if (inner == 1) break;
            nested++;
        }
    }
    assertEqual(nested, 3, "inner break does not break outer loop");

    var oddSum = 0;
    var n = 0;
    while (n < 8)
    {
        n++;
        if (n % 2 == 0) continue;
        if (n > 5) break;
        oddSum += n;
    }
    assertEqual(oddSum, 9, "while continue then break");

    var branch = 0;
    if (0)
    {
        branch = 1;
    }
    else if (0)
    {
        branch = 2;
    }
    else if (1)
    {
        branch = 3;
    }
    else
    {
        branch = 4;
    }
    assertEqual(branch, 3, "else-if first true branch");
}

// ============================================================
// 12. for 循环空语句（C++风格）
// ============================================================
function test_for_empty_parts()
{
    printf("--- test_for_empty_parts ---");

    // 空初始化: for(; cond; incr)
    var i = 0;
    var sum = 0;
    for (; i < 5; i++)
    {
        sum += i;
    }
    assertEqual(sum, 10, "for empty init");
    assertEqual(i, 5, "for empty init final value");

    // 空递增: for(init; cond;)
    sum = 0;
    for (var j = 0; j < 5;)
    {
        sum += j;
        j++;
    }
    assertEqual(sum, 10, "for empty increment");

    // 空初始化 + 空递增: for(; cond;)
    var k = 0;
    sum = 0;
    for (; k < 5;)
    {
        sum += k;
        k++;
    }
    assertEqual(sum, 10, "for empty init and increment");
    assertEqual(k, 5, "for empty init and increment final value");

    // 空初始化 + break
    var m = 10;
    sum = 0;
    for (; m > 0; m--)
    {
        if (m <= 5) break;
        sum += m;
    }
    assertEqual(sum, 40, "for empty init with break");

    // 空递增 + continue
    sum = 0;
    for (var n = 0; n < 10;)
    {
        n++;
        if (n % 2 == 0) continue;
        sum += n;
    }
    assertEqual(sum, 25, "for empty increment with continue");

    // 嵌套：外层空初始化，内层空递增
    var total = 0;
    var x = 0;
    for (; x < 3; x++)
    {
        for (var y = 0; y < 3;)
        {
            total += 1;
            y++;
        }
    }
    assertEqual(total, 9, "nested for with empty parts");
}
