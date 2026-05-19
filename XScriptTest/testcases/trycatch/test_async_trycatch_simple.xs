async function main()
{
    var a = 5;
    try
    {
        throw "err";
        a = 99;
    }
    catch e
    {
        printf("caught async simple", a, e);
        return a;
    }
    return -1;
}

var r = run_until_complete(main());
printf("async simple result", r);
