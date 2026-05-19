local clock = os.clock

local function report(name, start_time, value)
    local elapsed = clock() - start_time
    print(string.format("BENCH %s %.6f %s", name, elapsed, tostring(value)))
end

local function bench_sum_mod()
    local start_time = clock()
    local mod = 1000000007
    local total = 0
    for i = 1, 100000000 do
        total = total + i
        if total >= mod then
            total = total % mod
        end
    end
    report("sum_mod_100m", start_time, total)
end

local function bench_function_call()
    local function add1(x)
        return x + 1
    end

    local start_time = clock()
    local value = 0
    for i = 1, 5000000 do
        value = add1(value)
    end
    report("function_call_5m", start_time, value)
end

local function bench_list_access()
    local start_time = clock()
    local mod = 1000000007
    local arr = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
    local total = 0
    for i = 0, 9999999 do
        total = total + arr[(i % 10) + 1]
        if total >= mod then
            total = total % mod
        end
    end
    report("list_access_10m", start_time, total)
end

local function bench_table_access()
    local start_time = clock()
    local mod = 1000000007
    local t = {a = 1, b = 2, c = 3}
    local total = 0
    for i = 1, 10000000 do
        total = total + t.a
        total = total + t.b
        total = total + t.c
        if total >= mod then
            total = total % mod
        end
    end
    report("table_access_10m", start_time, total)
end

local function bench_recursive_fib()
    local function fib(n)
        if n <= 1 then
            return n
        end
        return fib(n - 1) + fib(n - 2)
    end

    local start_time = clock()
    local value = fib(25)
    report("recursive_fib_25", start_time, value)
end

bench_sum_mod()
bench_function_call()
bench_list_access()
bench_table_access()
bench_recursive_fib()
