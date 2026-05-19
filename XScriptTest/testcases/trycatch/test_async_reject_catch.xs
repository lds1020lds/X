async function fail_later()
{
    await sleep(1);
    var x = nil;
    x();
    return 123;
}

async function fail_now()
{
    var x = nil;
    x();
    return 456;
}

async function catches_rejected_after_suspend()
{
    var a = 10;
    try
    {
        a = await fail_later();
        printf("unexpected after fail_later", a);
    }
    catch e
    {
        printf("caught later", a);
        return a;
    }
    return -1;
}

async function catches_already_rejected()
{
    var f = fail_now();
    run_for(10);
    var a = 20;
    try
    {
        a = await f;
        printf("unexpected after fail_now", a);
    }
    catch e
    {
        printf("caught already", a);
        return a;
    }
    return -1;
}

var r1 = run_until_complete(catches_rejected_after_suspend());
printf("reject later result", r1);
var r2 = run_until_complete(catches_already_rejected());
printf("reject already result", r2);
