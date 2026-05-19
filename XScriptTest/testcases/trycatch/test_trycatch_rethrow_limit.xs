// 当前原型的已知失败用例：catch 内 rethrow 会被同一个 catch 反复捕获，形成死循环。
// 用 timeout 手动运行验证，不放入通过套件。

function test_nested_outer_catches()
{
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
        return e;
    }
}

var b = test_nested_outer_catches();
printf("nested rethrow final", b);
