async function delayed_value(v, ms)
{
    await sleep(ms);
    return v;
}

async function double_await()
{
    var a = await delayed_value(3, 5);
    var b = await delayed_value(4, 5);
    return a + b;
}

async function main()
{
    var result = await double_await();
    printf("double await", result);

    var already = await delayed_value(9, 0);
    printf("zero sleep", already);

    return result + already;
}

var final = run_until_complete(main());
printf("async more result", final);
