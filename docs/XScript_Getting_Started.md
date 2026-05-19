# XScript 新手上手教程

这份教程面向第一次接触 XScript 的用户。你不需要先理解 VM、Parser 或 C++ 宿主绑定，只要跟着示例写，就能快速掌握 XScript 的日常用法。

## 1. XScript 是什么

XScript 是一个 C++ 实现的嵌入式脚本语言。它的语法风格接近 Lua 和常见脚本语言，适合用来：

- 给 C++ 程序增加脚本扩展能力。
- 编写自动化脚本。
- 处理文本、文件、JSON、路径、时间、网络请求等任务。
- 在脚本里快速表达业务逻辑，而不用频繁重新编译宿主程序。

一个最小脚本如下：

```xs
printf("hello XScript");
```

## 2. 第一个脚本

创建一个文件，例如 `hello.xs`：

```xs
function main()
{
    printf("hello XScript");
}

main();
```

这个脚本做了三件事：

1. 定义 `main` 函数。
2. 在函数中调用 `printf` 输出文本。
3. 在文件底部调用 `main()`。

## 3. 变量和基础类型

```xs
var name = "Tom";
var age = 18;
var height = 1.75;
var enabled = true;
var empty = nil;

printf(name, age, height, enabled, empty);
```

常用类型：

- `nil`：空值。
- 整数：如 `1`、`100`、`0xff`。
- 浮点数：如 `3.14`。
- 字符串：如 `"hello"`。
- 表：如 `{ name = "Tom" }`。
- 列表：如 `[1, 2, 3]`。
- 函数：普通函数、匿名函数、lambda。

## 4. 字符串拼接

XScript 使用 `$` 拼接字符串，不使用 `+`。

```xs
var name = "XScript";
var text = "hello " $ name;
text $= "!";
printf(text);
```

如果要把数字拼进字符串，可以用 `toString`。

```xs
var n = 10;
printf("n=" $ toString(n));
```

## 5. 条件判断

```xs
var score = 75;

if (score >= 60)
{
    printf("pass");
}
else
{
    printf("fail");
}
```

三元表达式写法：

```xs
var level = (score >= 60) ? "pass" or "fail";
```

注意这里是 `? trueValue or falseValue`，不是 `? trueValue : falseValue`。

## 6. 循环

### 6.1 while

```xs
var i = 0;
while (i < 3)
{
    printf(i);
    i++;
}
```

### 6.2 for

```xs
for (var i = 0; i < 3; i++)
{
    printf(i);
}
```

### 6.3 break 和 continue

```xs
for (var i = 0; i < 10; i++)
{
    if (i == 2)
        continue;

    if (i == 5)
        break;

    printf(i);
}
```

## 7. 函数

```xs
function add(a, b)
{
    return a + b;
}

var r = add(1, 2);
printf(r);
```

函数可以返回多个值：

```xs
function swap(a, b)
{
    return b, a;
}

var x, y = swap(1, 2);
printf(x, y);
```

参数不够时会补 `nil`：

```xs
function show(a, b)
{
    printf(a, b);
}

show(1); // b 是 nil
```

## 8. Lambda 和闭包

Lambda 适合写短函数。

```xs
var double = lambda x: x * 2;
printf(double(5));
```

函数可以捕获外层变量，这叫闭包。

```xs
function makeCounter()
{
    var count = 0;
    return function()
    {
        count++;
        return count;
    };
}

var counter = makeCounter();
printf(counter()); // 1
printf(counter()); // 2
```

## 9. 表：像字典也像对象

表使用 `{}`。

```xs
var user = {
    name = "Tom",
    age = 18
};

printf(user.name);
printf(user["age"]);
```

可以动态添加字段：

```xs
user.city = "Shanghai";
user["score"] = 100;
```

表也可以包含函数，用来模拟对象方法。

```xs
var obj = { value = 10 };

obj.add = function(self, x)
{
    return self.value + x;
};

printf(obj:add(5));
```

`obj:add(5)` 会把 `obj` 自动作为第一个参数 `self` 传入。

## 10. 列表：动态数组

列表使用 `[]`，索引从 `0` 开始。

```xs
var arr = [1, 2, 3];
printf(arr[0]);

arr:append(4);
printf(arr:size());
```

常用操作：

```xs
arr:insert(1, 100);
arr:remove(2);
arr:reverse();
arr:sort();
```

## 11. 遍历数据

### 11.1 foreach

```xs
var t = { a = 10, b = 20 };

foreach (k, v in pairs(t))
{
    printf(k, v);
}
```

遍历列表：

```xs
var arr = [10, 20, 30];
foreach (i, v in ipairs(arr))
{
    printf(i, v);
}
```

### 11.2 iterator

`iterator` 可以遍历实现了 `__iterator__` / `__next__` 协议的对象，也能遍历列表、表和生成器。

```xs
iterator(v in [1, 2, 3])
{
    printf(v);
}
```

## 12. 解构赋值

表解构：

```xs
var user = { name = "Tom", age = 18 };
var { name, age } = user;
printf(name, age);
```

列表解构：

```xs
var arr = [10, 20, 30];
var [a, b, c] = arr;
printf(a, b, c);
```

默认值：

```xs
var { nick = "anonymous" } = user;
```

## 13. 可选链和默认值

当对象可能是 `nil` 时，可以用可选链避免报错，并用 `??` 提供默认值。

```xs
var user = nil;
var name = user?.name ?? "anonymous";
printf(name);
```

常见形式：

```xs
obj?.field
obj?[key]
callback?(arg)
obj?:method(arg)
```

`??` 只在左侧是 `nil` 时使用右侧默认值，所以会保留 `0` 和空字符串这类有效值。

```xs
var count = maybeCount ?? 0;
var name = user?.profile?.name ?? "anonymous";
```

如果希望把 `0`、空字符串等假值也当成缺失值，可以使用 `||`。

## 14. 模块化

可以把常用函数放进单独文件，然后用 `require` 加载。

例如 `math_util.xs`：

```xs
function add(a, b)
{
    return a + b;
}

function mul(a, b)
{
    return a * b;
}

return {
    add = add,
    mul = mul
};
```

使用模块：

```xs
var mathUtil = require("math_util.xs");
printf(mathUtil.add(1, 2));
printf(mathUtil.mul(3, 4));
```

## 15. 错误处理

### 15.1 try / catch / finally

```xs
try
{
    throw "something wrong";
}
catch e
{
    printf("caught", e);
}
finally
{
    printf("cleanup");
}
```

### 15.2 pcall

```xs
var ok, result = pcall(function()
{
    throw "failed";
});

if (!ok)
    printf("error", result);
```

## 16. defer：离开函数前自动清理

`defer` 后面的语句会在当前函数退出前执行，适合关闭文件、释放资源或恢复状态。

```xs
function readText(path)
{
    var f = file.open(path, "r");
    defer f:close();

    return f:read("*a");
}
```

## 17. 异步 async / await

异步函数使用 `async function`。

```xs
async function main()
{
    var data = await read_file_async("data.txt");
    printf(data);
}
```

`await` 只能写在异步函数内部。

异步错误可以用 `try / catch` 捕获：

```xs
async function main()
{
    try
    {
        var res = await http.get("http://example.com");
        printf(res.ok);
    }
    catch e
    {
        printf("request failed", e);
    }
}
```

## 18. 生成器 generator

生成器函数可以用 `emit` 逐个产生值。

```xs
generator function range(start, end)
{
    var i = start;
    while (i < end)
    {
        emit i;
        i++;
    }
}

iterator(v in range(1, 4))
{
    printf(v);
}
```

输出：

```text
1
2
3
```

## 19. 常用标准库

### 19.1 字符串

```xs
var s = " hello XScript ";
printf(s:trim());
printf(string.upper(s));
printf(string.split("a,b,c", ","));
```

字符串可以用方法形式，也可以用库函数形式，具体取决于注册的函数和元表。

### 19.2 数学

```xs
printf(math.sqrt(16));
printf(math.random(1, 10));
```

### 19.3 JSON

```xs
var obj = json.decode("{\"name\":\"Tom\"}");
printf(obj.name);

var text = json.encode({ ok = true, value = 123 });
printf(text);
```

### 19.4 文件和路径

```xs
if (os.exists("data.txt"))
{
    var f = file.open("data.txt", "r");
    defer f:close();
    printf(f:read("*a"));
}
```

## 20. 一个完整小例子

下面示例统计一组用户中成年人数量，并格式化输出。

```xs
var users = [
    { name = "Tom", age = 18 },
    { name = "Jerry", age = 16 },
    { name = "Alice", age = 22 }
];

function isAdult(user)
{
    return user.age >= 18;
}

function main()
{
    var count = 0;

    iterator(user in users)
    {
        var label = isAdult(user) ? "adult" or "child";
        printf(user.name $ " is " $ label);

        if (isAdult(user))
            count++;
    }

    printf("adult count=" $ toString(count));
}

main();
```

## 21. 新手常见坑

- 字符串拼接用 `$`，不是 `+`。
- 表和列表的顺序索引从 `0` 开始。
- 三元表达式是 `cond ? a or b`，不是 `cond ? a : b`。
- `await` 只能在 `async function` 内部使用。
- 方法调用 `obj:method(x)` 会自动传入 `self`。
- 可变参数通过 `__xscript_varargs` 读取。
- 对可能为空的对象优先用 `?.` 和 `??` 默认值；只有希望替换所有假值时才用 `||`。
- 复杂表达式建议加括号。

## 22. 下一步建议

学完本文后，可以继续阅读：

- `XScript_Grammar.md`：完整语法细节。
- `XScript_Technical_Analysis.md`：引擎实现、VM、GC、内置库和宿主绑定。
- `XScriptTest/comprehensive_tests/`：大量可运行示例。
- `XScriptTest/testcases/`：按语言特性拆分的测试用例。
