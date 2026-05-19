function bench_sum_mod()
{
    var start = getCurrentTime();
    var mod = 1000000007;
    var sum = 0;
    for (var i = 1; i <= 100000000; i++)
    {
        sum += i;
        if (sum >= mod)
        {
            sum %= mod;
        }
    }
    var elapsed = getCurrentTime() - start;
    printf("BENCH sum_mod_100m " $ elapsed $ " " $ sum);
}

function bench_function_call()
{
    function add1(x)
    {
        return x + 1;
    }

    var start = getCurrentTime();
    var value = 0;
    for (var i = 0; i < 5000000; i++)
    {
        value = add1(value);
    }
    var elapsed = getCurrentTime() - start;
    printf("BENCH function_call_5m " $ elapsed $ " " $ value);
}

function bench_list_access()
{
    var start = getCurrentTime();
    var mod = 1000000007;
    var arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    var sum = 0;
    for (var i = 0; i < 10000000; i++)
    {
        sum += arr[i % 10];
        if (sum >= mod)
        {
            sum %= mod;
        }
    }
    var elapsed = getCurrentTime() - start;
    printf("BENCH list_access_10m " $ elapsed $ " " $ sum);
}

function bench_table_access()
{
    var start = getCurrentTime();
    var mod = 1000000007;
    var t = {a = 1, b = 2, c = 3};
    var sum = 0;
    for (var i = 0; i < 10000000; i++)
    {
        sum += t.a;
        sum += t.b;
        sum += t.c;
        if (sum >= mod)
        {
            sum %= mod;
        }
    }
    var elapsed = getCurrentTime() - start;
    printf("BENCH table_access_10m " $ elapsed $ " " $ sum);
}

function bench_recursive_fib()
{
    function fib(n)
    {
        if (n <= 1) return n;
        return fib(n - 1) + fib(n - 2);
    }

    var start = getCurrentTime();
    var value = fib(25);
    var elapsed = getCurrentTime() - start;
    printf("BENCH recursive_fib_25 " $ elapsed $ " " $ value);
}

bench_sum_mod();
bench_function_call();
bench_list_access();
bench_table_access();
bench_recursive_fib();
