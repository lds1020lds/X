// 这个文件记录当前原型尚未支持/需要后续确认的场景。
// 不建议放入自动通过套件；用于手动验证改进点。

function test_throw_literal_string()
{
    try
    {
        throw "literal error";
    }
    catch e
    {
        printf("literal string catch", e);
    }
}

function test_finally_placeholder()
{
    try
    {
        throw 1;
    }
    catch e
    {
        printf("catch before finally", e);
    }
    finally
    {
        printf("finally block");
    }
}

 test_throw_literal_string();
 test_finally_placeholder();
