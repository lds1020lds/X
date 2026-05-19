async function value_after(v, ms)
{
    await set_timeout(ms);
    return v;
}

async function main()
{
    printf("utilities begin");

    await next_tick();
    printf("after next tick");

    var list = await all([value_after(1, 5), value_after(2, 10)]);
    printf("all done", list[0], list[1]);

    var first = await race([value_after(3, 20), value_after(4, 5)]);
    printf("race done", first);

    var timeoutRes = await timeout(value_after(5, 30), 1);
    printf("timeout done", timeoutRes.ok, timeoutRes.timeout);

    var execRes = await exec_async("cmd /c echo async_exec");
    printf("exec done", execRes.ok, execRes.code);

    var mk = await mkdir_async("async_tmp_dir");
    printf("mkdir", mk.ok);

    var exists = await file_exists_async("async_tmp_dir");
    printf("exists", exists.ok);

    var writeRes = await write_file_async("async_tmp_dir/util.txt", "util data");
    printf("write util", writeRes.ok, writeRes.size);

    var dirRes = await list_dir_async("async_tmp_dir");
    printf("list dir", dirRes.ok);

    var removeRes = await remove_file_async("async_tmp_dir/util.txt");
    printf("remove", removeRes.ok);

    return first;
}

var result = run_until_complete(main());
printf("utilities result", result);
