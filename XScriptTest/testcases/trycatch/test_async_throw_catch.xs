async function main()
{
    var a = 5;
    try
    {
        await sleep(1);
        throw "err";
        a = 99;
    }
    catch e
    {
        printf("caught async throw", a, e);
        return a;
    }
    return -1;
}

var r = run_until_complete(main());
printf("async throw catch result", r);
