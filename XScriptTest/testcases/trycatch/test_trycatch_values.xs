function test_throw_values()
{
    var sum = 0;
    var msg = "unset";

    try
    {
        var err = 5;
        throw err;
    }
    catch e
    {
        sum = sum + e;
    }

    try
    {
        var value = 6;
        throw value;
    }
    catch e
    {
        sum = sum + e;
    }

    // 当前原型如果只支持 throw 标识符/访问链，不支持 throw 字符串字面量，
    // 这里使用变量抛字符串，避免把这个测试变成语法测试。
    try
    {
        var s = "hello";
        throw s;
    }
    catch e
    {
        msg = e;
    }

    printf("values", sum, msg);
    return sum;
}

var r = test_throw_values();
printf("values final", r);
