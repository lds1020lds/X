async function delayed(v)
{
    await set_timeout(20);
    return v;
}

async function side_effect()
{
    await set_timeout(5);
    await write_file_async("spawn_side_effect.tmp", "spawned");
    return 1;
}

spawn(side_effect());
run_for(30);
var side = run_until_complete(read_file_async("spawn_side_effect.tmp"));
printf("spawn side effect", side.ok, side.data);

var f = spawn(delayed(42));
printf("spawn state", future_state(f), future_done(f));
async_tick();
printf("after tick", future_state(f), future_done(f));
run_for(50);
printf("after run_for", future_state(f), future_done(f), future_result(f));

async function main()
{
    var a = await next_tick();
    return 7;
}

var result = run_until_complete(main());
printf("run until complete", result);
