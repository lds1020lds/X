
// ============================================================
// 测试 1：require 返回模块导出的表
// ============================================================
function test_require_return_table()
{
    printf("-- test_require_return_table --");
    var M = require("comprehensive_tests/test_mylib");
    assert(M != nil, "require should return non-nil value");
    assertEqual(M.name, "mylib", "module name field");
    assertEqual(M.version, 42, "module version field");
    assertEqual(M.add(1, 2), 3, "module add function");
    assertEqual(M.mul(3, 4), 12, "module mul function");
}

// ============================================================
// 测试 2：require 返回数字
// ============================================================
function test_require_return_number()
{
    printf("-- test_require_return_number --");
    var v = require("comprehensive_tests/test_nummod");
    assertEqual(v, 99, "require should return number 99");
}

// ============================================================
// 测试 3：没有返回值的模块返回 true (1)
// ============================================================
function test_require_no_return()
{
    printf("-- test_require_no_return --");
    var r = require("comprehensive_tests/test_noreturn");
    assertEqual(r, 1, "require with no return should give true (1)");
}

// ============================================================
// 测试 4：重复 require 返回缓存值（同一引用）
// ============================================================
function test_require_cache()
{
    printf("-- test_require_cache --");
    var a = require("comprehensive_tests/test_mylib");
    var b = require("comprehensive_tests/test_mylib");
    assert(a == b, "repeated require should return same reference");
    // 修改 a 后 b 也应该看到变化（同一引用）
    a.extra = 123;
    assertEqual(b.extra, 123, "cached module should be same object");
}

// ============================================================
// 测试 5：package.loaded 可读取
// ============================================================
function test_package_loaded_read()
{
    printf("-- test_package_loaded_read --");
    // 确保 package 表存在
    assert(package != nil, "package should exist");
    assert(package.loaded != nil, "package.loaded should exist");
    
    // 之前已经 require 过 test_mylib，应该在 package.loaded 中
    var cached = package.loaded["comprehensive_tests/test_mylib.xs"];
    assert(cached != nil, "package.loaded should contain loaded module");
    assertEqual(cached.name, "mylib", "package.loaded value should be module");
}

// ============================================================
// 测试 6：package.loaded 预填充（模拟模块）
// ============================================================
function test_package_loaded_prefill()
{
    printf("-- test_package_loaded_prefill --");
    // 预填充一个模拟模块
    package.loaded["mock_module.xs"] = {foo = 42, bar = "hello"};
    var mock = require("mock_module");
    assertEqual(mock.foo, 42, "prefilled module foo field");
    assertEqual(mock.bar, "hello", "prefilled module bar field");
}

// ============================================================
// 测试 7：package.path 可读取和修改
// ============================================================
function test_package_path()
{
    printf("-- test_package_path --");
    assert(package.path != nil, "package.path should exist");
    // 默认路径应包含 ./?.xs
    assertEqual(package.path, "./?.xs", "default package.path");
}

// ============================================================
// 主函数
// ============================================================
function main()
{
    printf("========================================");
    printf("  XScript Require Module Test Suite");
    printf("========================================");

    test_require_return_table();
    test_require_return_number();
    test_require_no_return();
    test_require_cache();
    test_package_loaded_read();
    test_package_loaded_prefill();
    test_package_path();

}

main();
