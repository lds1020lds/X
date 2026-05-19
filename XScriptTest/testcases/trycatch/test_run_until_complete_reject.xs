async function fail_task()
{
    await sleep(1);
    var x = nil;
    x();
    return 1;
}

function test_catch_run_until_complete()
{
    var marker = 12;
    try
    {
        marker = run_until_complete(fail_task());
        printf("unexpected run_until_complete result", marker);
    }
    catch e
    {
        printf("caught run_until_complete reject", marker);
        return marker;
    }
    return -1;
}

var result = test_catch_run_until_complete();
printf("run_until_complete reject result", result);
