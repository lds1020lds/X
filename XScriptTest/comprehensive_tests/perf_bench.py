import time


def report(name, start, value):
    elapsed = time.perf_counter() - start
    print(f"BENCH {name} {elapsed:.6f} {value}")


def bench_sum_mod():
    start = time.perf_counter()
    mod = 1_000_000_007
    total = 0
    for i in range(1, 100_000_000 + 1):
        total += i
        if total >= mod:
            total %= mod
    report("sum_mod_100m", start, total)


def bench_function_call():
    def add1(x):
        return x + 1

    start = time.perf_counter()
    value = 0
    for _ in range(5_000_000):
        value = add1(value)
    report("function_call_5m", start, value)


def bench_list_access():
    start = time.perf_counter()
    mod = 1_000_000_007
    arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    total = 0
    for i in range(10_000_000):
        total += arr[i % 10]
        if total >= mod:
            total %= mod
    report("list_access_10m", start, total)


def bench_table_access():
    start = time.perf_counter()
    mod = 1_000_000_007
    t = {"a": 1, "b": 2, "c": 3}
    total = 0
    for _ in range(10_000_000):
        total += t["a"]
        total += t["b"]
        total += t["c"]
        if total >= mod:
            total %= mod
    report("table_access_10m", start, total)


def bench_recursive_fib():
    def fib(n):
        if n <= 1:
            return n
        return fib(n - 1) + fib(n - 2)

    start = time.perf_counter()
    value = fib(25)
    report("recursive_fib_25", start, value)


if __name__ == "__main__":
    bench_sum_mod()
    bench_function_call()
    bench_list_access()
    bench_table_access()
    bench_recursive_fib()
