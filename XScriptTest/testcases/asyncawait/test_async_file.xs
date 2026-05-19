async function main()
{
    printf("file async begin");

    var writeRes = await write_file_async("async_file_test.tmp", "hello");
    printf("write", writeRes.ok, writeRes.size);

    var appendRes = await append_file_async("async_file_test.tmp", " world");
    printf("append", appendRes.ok, appendRes.size);

    try
    {
        var readFuture = read_file_async("async_file_test2121.tmp");
        printf("read future returned");
        var readRes = await readFuture;
        printf("read", readRes.ok, readRes.size, readRes.data);

        var missing = await read_file_async("missing_async_file.tmp");
        printf("missing", missing.ok, missing.error);
    }
    catch e
    {
        printf("Read file Rejected");
        printf("result：", toString(e));
    }

    



    return 12;
}

var result = run_until_complete(main());
printf("file async result", result);
