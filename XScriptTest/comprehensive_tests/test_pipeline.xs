function pipeline_square(value)
{
    return value * value;
}

function pipeline_add(left, right)
{
    return left + right;
}

function pipeline_sum3(a, b, c)
{
    return a + b + c;
}

function pipeline_weighted_sum(a, b, c, d)
{
    return a * 1000 + b * 100 + c * 10 + d;
}

function pipeline_make_pair(left, right)
{
    return {left = left, right = right};
}

function pipeline_pair_score(pair)
{
    return pair.left * 10 + pair.right;
}

function pipeline_select_second(first, second)
{
    return second;
}

function pipeline_apply(value, fn)
{
    return fn(value);
}

function pipeline_make_adder(delta)
{
    return function(value)
    {
        return value + delta;
    };
}

function pipeline_method_add(self, value)
{
    return self.base + value;
}

function pipeline_method_mix(self, value, extra)
{
    return self.base + value * 10 + extra;
}

function pipeline_method_weight(self, first, second, third)
{
    return self.base * 1000 + first * 100 + second * 10 + third;
}

function test_pipeline()
{
    printf("--- test_pipeline ---");

    var a = 1;
    var b = 2;
    var c = a + b |> pipeline_square();
    assertEqual(c, 9, "pipeline auto fill first explicit argument");

    var manual = c |>> pipeline_add(_ * _, 4);
    assertEqual(manual, 85, "pipeline manual placeholder can be used multiple times");

    var complex = c |>> pipeline_add((_ * _) + (1 + 2 + 3), 5);
    assertEqual(complex, 92, "pipeline placeholder register remains valid across later temporaries");

    var obj = {base = 10, add = pipeline_method_add, mix = pipeline_method_mix, weight = pipeline_method_weight};
    var methodResult = 5 |> obj:add();
    assertEqual(methodResult, 15, "pipeline method call fills first explicit argument");

    // 链式管道：自动填充结果继续作为下一段管道源值
    var chained = 3 |> pipeline_add(4) |> pipeline_square() |> pipeline_add(10);
    assertEqual(chained, 59, "pipeline chained auto fill keeps left-to-right flow");

    // 优先级：管道应低于算术表达式，让左侧表达式先完整求值
    var priorityArithmetic = 1 + 2 * 3 |> pipeline_add(10);
    assertEqual(priorityArithmetic, 17, "pipeline priority is lower than arithmetic operators");

    var priorityManual = 2 + 3 |>> pipeline_add(_ * _, 1);
    assertEqual(priorityManual, 26, "manual pipeline priority keeps complete left expression");

    // 自动填充只插入到右侧第一个调用的第一个显式参数
    var outerAutoFill = 3 |> pipeline_sum3(pipeline_square(2), pipeline_square(3));
    assertEqual(outerAutoFill, 16, "pipeline auto fill targets outer first call before nested calls");

    var autoFillOnce = 4 |> pipeline_select_second(pipeline_add(10, 20));
    assertEqual(autoFillOnce, 30, "pipeline auto fill happens only once in right expression");

    // 手动占位符和复杂临时寄存器组合
    var registerPressure = 4 |>> pipeline_sum3(_ * _, (_ + 1) * (_ + 2), pipeline_square(_));
    assertEqual(registerPressure, 62, "pipeline placeholder survives complex temporary register reuse");

    var weighted = 7 |>> pipeline_weighted_sum(_ + 1, _ * 2, pipeline_add(_, 3), pipeline_square(_ - 5));
    assertEqual(weighted, 9504, "pipeline placeholder can feed several argument expressions");

    // 嵌套管道：内层管道应使用自己的源值，结束后外层占位符继续有效
    var nestedManual = 3 |>> pipeline_add(10 |>> pipeline_add(_, 1), _);
    assertEqual(nestedManual, 14, "nested manual pipeline restores outer placeholder after inner pipeline");

    var nestedAuto = 2 |> pipeline_add(3 |> pipeline_square());
    assertEqual(nestedAuto, 11, "nested auto pipeline evaluates inner pipeline as normal argument");

    // 冒号方法调用：receiver 是隐式第一个参数，管道值填充第一个显式参数
    var methodWithArgs = 5 |> obj:mix(3);
    assertEqual(methodWithArgs, 63, "pipeline method call inserts value after receiver");

    var methodManual = 4 |>> obj:weight(_ + 1, _ * 2, pipeline_square(_));
    assertEqual(methodManual, 10596, "manual pipeline placeholders work inside method call arguments");

    // 管道结果可以继续进行表访问和后续计算
    var pairScore = 8 |> pipeline_make_pair(9) |> pipeline_pair_score();
    assertEqual(pairScore, 89, "pipeline result can be table and feed another pipeline call");

    // 闭包和高阶函数：管道值可以传给变量保存的函数和函数参数
    var add5 = pipeline_make_adder(5);
    var closureResult = 8 |> add5();
    assertEqual(closureResult, 13, "pipeline auto fill works with closure stored in variable");

    var applied = 6 |> pipeline_apply(add5);
    assertEqual(applied, 11, "pipeline can flow through higher order function call");

    var composed = 2 |> pipeline_add(3) |>> pipeline_apply(_, pipeline_make_adder(_ * 2));
    assertEqual(composed, 15, "manual pipeline can build closure from the same source value");
}