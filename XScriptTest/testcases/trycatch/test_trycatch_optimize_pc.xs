function test_optimized_throw_path()
{
    var marker = "prefix" $ "-" $ "throw";
    var caught = "none";
    var finalized = 0;
    var after = 0;

    try
    {
        var local = marker $ "-local";
        throw local;
        after = -100;
    }
    catch e
    {
        caught = e;
    }
    finally
    {
        finalized = finalized + 1;
    }

    after = 7;
    printf("optimized throw path", caught, finalized, after);
    return caught == "prefix-throw-local" && finalized == 1 && after == 7;
}

function test_optimized_no_throw_path()
{
    var text = "prefix" $ "-" $ "ok";
    var caught = "none";
    var finalized = 0;
    var after = 0;

    try
    {
        var local = text $ "-local";
        after = 3;
    }
    catch e
    {
        caught = e;
        after = -100;
    }
    finally
    {
        finalized = finalized + 1;
    }

    after = after + 4;
    printf("optimized no throw path", caught, finalized, after);
    return caught == "none" && finalized == 1 && after == 7;
}

var a = test_optimized_throw_path();
var b = test_optimized_no_throw_path();
printf("trycatch optimize pc result", a, b);

if (!a || !b)
{
    throw "trycatch optimize pc failed";
}
