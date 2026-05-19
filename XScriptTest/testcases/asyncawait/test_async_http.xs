async function main()
{
    printf("http before call");
    var f = http_get("http://127.0.0.1:1/");
    printf("http after call");
    var res = await f;
    printf("http after await", res.ok, res.error);
    return res.ok;
}

var result = run_until_complete(main());
printf("http async result", result);
