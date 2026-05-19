async function callback_with_await()
{
    await sleep(1);
    return 1;
}

async function main()
{
    var ok = 0;
    try
    {
        pcall(callback_with_await);
        ok = 99;
    }
    catch e
    {
        printf("caught c boundary", e);
        ok = 1;
    }
    return ok;
}

var result = run_until_complete(main());
printf("c boundary await result", result);
