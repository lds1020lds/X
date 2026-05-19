function test_same_function()
{
    var caught = 0;
    try
    {
        var err = 123;
        throw err;
        caught = 99;
    }
    catch e
    {
        printf("same catch", e);
        caught = e;
    }
    printf("same after", caught);
    return caught;
}

function test_no_throw()
{
    var x = 1;
    try
    {
        x = x + 1;
    }
    catch e
    {
        x = 99;
    }
    printf("no throw", x);
    return x;
}

var a = test_same_function();
var b = test_no_throw();
printf("basic result", a, b);
