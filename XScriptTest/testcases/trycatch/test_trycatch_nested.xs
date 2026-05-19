function test_nested_inner_catches()
{
    var x = 0;
    try
    {
        try
        {
            var err = 11;
            throw err;
            x = 100;
        }
        catch e
        {
            printf("inner catch", e);
            x = e;
        }
        x = x + 1;
    }
    catch e
    {
        printf("outer unexpected", e);
        x = 999;
    }
    printf("nested result", x);
    return x;
}

function test_nested_outer_catches()
{
    var x = 0;
    try
    {
        try
        {
            var err = 21;
            throw err;
        }
        catch e
        {
            printf("inner rethrow", e);
            throw e;
        }
    }
    catch e
    {
        printf("outer catch", e);
        x = e;
    }
    printf("nested rethrow result", x);
    return x;
}

var a = test_nested_inner_catches();
printf("nested final", a);

// 当前原型这里会死循环：catch 内 rethrow 仍被同一个 catch 捕获，
// 需要 throw 时先弹出/禁用当前 catch handler，或把 pc 指到外层 handler。
// var b = test_nested_outer_catches();
// printf("nested rethrow final", b);
