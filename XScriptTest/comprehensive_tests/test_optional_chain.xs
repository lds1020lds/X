// ============================================================
// 可选链操作符 (?.) 测试脚本
// 测试 ?. ?() ?[] ?:method() 四种可选链语法
// ============================================================

// ---- 测试1: 基本的 ?. 字段访问 ----
printf("=== Test 1: Basic ?. field access ===");

var obj = { name = "hello", value = 42 };
var result1 = obj?.name;
printf("obj?.name = " $  toString(result1));   // 应输出 "hello"

var nilObj = nil;
var result2 = nilObj?.name;
printf("nilObj?.name = " $ toString(result2)); // 应输出 nil

// ---- 测试2: 链式 ?. 访问 ----
printf("\n=== Test 2: Chained ?. access ===");

var deep = { a = { b = { c = "found" } } };
var result3 = deep?.a?.b?.c;
printf("deep?.a?.b?.c = " $ result3); // 应输出 "found"

var partial = { a = nil };
var result4 = partial?.a?.b?.c;
printf("partial?.a?.b?.c = " $ toString(result4)); // 应输出 nil (a 为 nil，短路)

var result5 = nil?.a?.b?.c;
printf("nil?.a?.b?.c = " $ toString(result5)); // 应输出 nil

// ---- 测试3: ?() 可选函数调用 ----
printf("\n=== Test 3: ?() optional function call ===");

var funcObj = { 
    greet = function() { return "hi"; }
};
var result6 = funcObj?.greet?();
printf("funcObj?.greet?() = " $ toString(result6)); // 应输出 "hi"

var noFunc = { greet = nil };
var result7 = noFunc?.greet?();
printf("noFunc?.greet?() = " $ toString(result7)); // 应输出 nil

var result8 = nil?.greet?();
printf("nil?.greet?() = " $ toString(result8)); // 应输出 nil

// ---- 测试4: ?:method() 可选方法调用 ----
printf("\n=== Test 4: ?:method() optional method call ===");

var player = {
    name = "warrior",
    getName = function(self) { return self.name; }
};
var result9 = player?:getName();
printf("player?:getName() = " $ toString(result9)); // 应输出 "warrior"

var nilPlayer = nil;
var result10 = nilPlayer?:getName();
printf("nilPlayer?:getName() = " $ toString(result10)); // 应输出 nil

// ---- 测试5: 混合链式调用 ----
printf("\n=== Test 5: Mixed chaining ===");

var data = {
    getUser = function() {
        return { 
            name = "Alice",
            getAge = function(self) { return 25; }
        };
    }
};

var result11 = data?.getUser?()?.name;
printf("data?.getUser?()?.name = " $ toString(result11)); // 应输出 "Alice"

var result12 = nil?.getUser?()?.name;
printf("nil?.getUser?()?.name = " $ toString(result12)); // 应输出 nil

// ---- 测试6: 可选链与普通访问混合 ----
printf("\n=== Test 6: Optional chain mixed with normal access ===");

var config = { db = { host = "localhost" } };
var result13 = config?.db.host;
printf("config?.db.host = " $ toString(result13)); // 应输出 "localhost"

printf("\n=== All tests completed ===");
