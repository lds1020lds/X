function old_co()
{
    coroutine.yield(7);
    return 8;
}

var co = coroutine.create(old_co);
var ok, value = coroutine.resume(co);
printf("old coroutine first", ok, value);
ok, value = coroutine.resume(co);
printf("old coroutine second", ok, value);

async function child()
{
    printf("child begin");
    await sleep(10);
    printf("child after sleep");
    //x();
    return 10;
}

async function main()
{
    printf("main begin");
    
    var x
    try 
    {
        x = await child();
        await sleep(10);
    }
    catch e
    {
        printf("main catch");
        printf(e);
    }

    printf("main after child", x);
    return x + 1;
}

var result = run_until_complete(main());
printf("async result", result);
