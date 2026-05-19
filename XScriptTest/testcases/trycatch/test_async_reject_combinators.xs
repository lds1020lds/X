async function fail_later(v)
{
    await sleep(1);
    var x = nil;
    x();
    return v;
}

async function ok_later(v)
{
    await sleep(2);
    return v;
}

async function test_all_reject()
{
    var marker = 1;
    try
    {
        var values = await all([ok_later(10), fail_later(20)]);
        marker = 99;
    }
    catch e
    {
        printf("all rejected", marker);
        return marker;
    }
    return -1;
}

async function test_race_reject()
{
    var marker = 2;
    try
    {
        var value = await race([fail_later(30), ok_later(40)]);
        marker = 99;
    }
    catch e
    {
        printf("race rejected", marker);
        return marker;
    }
    return -1;
}

async function test_timeout_source_reject()
{
    var marker = 3;
    try
    {
        var value = await timeout(fail_later(50), 100);
        marker = 99;
    }
    catch e
    {
        printf("timeout source rejected", marker);
        return marker;
    }
    return -1;
}

var a = run_until_complete(test_all_reject());
printf("all reject result", a);
var b = run_until_complete(test_race_reject());
printf("race reject result", b);
var c = run_until_complete(test_timeout_source_reject());
printf("timeout reject result", c);

try 
{
    run_until_complete(fail_later(1));
    printf("run_until_complete success")
}
catch e
{
    printf("top rejected caught", e);
}

