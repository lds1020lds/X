async function main()
{
    printf("socket async begin");
    var res = await timeout(tcp_connect_async("127.0.0.1", 1), 2000);
    if (res.timeout)
    {
        printf("socket timeout", res.timeout);
        return 0;
    }
    printf("socket connect", res.ok, res.error);
    return res.ok;
}

var result = run_until_complete(main());
printf("socket async result", result);
