# 为什么值得使用 XScript

这是一份面向使用者、项目维护者和潜在集成方的 XScript 安利指南。它不强调内部实现细节，而是回答一个更直接的问题：XScript 有什么值得别人用的点？

## 1. 一句话介绍

XScript 是一个由 C++ 实现的嵌入式脚本语言，既有脚本语言的灵活表达力，又方便接入宿主程序、扩展业务逻辑和快速迭代功能。

如果你的项目需要“让一部分逻辑可以不用重新编译 C++ 就能调整”，XScript 就很适合。

## 2. 它适合解决什么问题

XScript 特别适合这些场景：

- 在 C++ 程序中嵌入脚本能力。
- 把配置逻辑、业务规则、自动化流程从宿主程序中拆出来。
- 快速写工具脚本、测试脚本、数据处理脚本。
- 通过 `require` 组织脚本模块。
- 用内置库处理字符串、文件、JSON、路径、时间、HTTP、Socket 等常见任务。
- 通过函数、闭包、表和元表构建轻量抽象。
- 使用异步、协程、生成器表达复杂流程。

## 3. 上手成本低

XScript 的基本语法非常接近常见脚本语言。

```xs
var name = "XScript";

function hello(who)
{
    printf("hello " $ who);
}

hello(name);
```

你可以很快理解：

- `var` 声明变量。
- `function` 定义函数。
- `{}` 表示代码块或表。
- `[]` 表示列表。
- `if`、`while`、`for` 都是熟悉的控制流。

不同点也很少，最重要的是：字符串拼接用 `$`，三元表达式用 `? a or b`。

## 4. 语法表达力已经比较完整

XScript 不只是一个“能跑表达式”的小语言，它已经具备完整脚本语言常见能力。

### 4.1 控制流完整

```xs
for (var i = 0; i < 10; i++)
{
    if (i % 2 == 0)
        continue;

    if (i > 7)
        break;

    printf(i);
}
```

支持：

- `if / else`
- `while`
- `for`
- `foreach`
- `iterator`
- `break / continue`
- 三元表达式

### 4.2 函数是一等值

函数可以赋给变量、放进表和列表、作为参数传递、作为返回值返回。

```xs
var add = function(a, b) { return a + b; };
var double = lambda x: x * 2;

function apply(fn, value)
{
    return fn(value);
}

printf(apply(double, 21));
```

这让 XScript 很适合写回调、策略、过滤、映射、命令表等逻辑。

### 4.3 闭包让状态封装很自然

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
printf(counter());
printf(counter());
```

闭包可以把状态和行为封在一起，不一定需要重型类系统。

### 4.4 多返回值和多重赋值很实用

```xs
function parsePair()
{
    return "key", "value";
}

var k, v = parsePair();
```

多返回值非常适合：

- 返回 `ok, value`。
- 返回 `key, value`。
- 交换变量。
- 拆分函数结果。

## 5. 表和列表覆盖大部分数据结构需求

XScript 的表可以当字典、对象、结构体使用。

```xs
var user = {
    name = "Tom",
    age = 18,
    tags = ["new", "vip"]
};

user.city = "Shanghai";
printf(user.name, user.tags[0]);
```

列表适合数组和队列式数据。

```xs
var items = [1, 2, 3];
items:append(4);
items:sort();
```

这两种基础结构足以覆盖大量脚本场景。

## 6. 对对象风格编程很友好

XScript 支持 `:` 方法调用，自动传入 `self`。

```xs
var account = { balance = 100 };

function account:deposit(amount)
{
    self.balance += amount;
}

account:deposit(50);
printf(account.balance);
```

用表 + 函数 + 元表，可以实现轻量对象模型、原型继承、默认字段、运算符重载等能力。

## 7. 可选链让脚本更健壮

脚本里经常遇到“某个字段可能不存在”的情况。可选链可以减少大量防御式判断。

```xs
var title = article?.meta?.title || "untitled";
var result = callback?(data);
var text = obj?:toString() || "";
```

这对配置、JSON、外部接口数据特别有用。

## 8. 解构让数据处理更舒服

表解构：

```xs
var user = { name = "Tom", age = 18 };
var { name, age } = user;
```

列表解构：

```xs
var pair = ["x", 100];
var [key, value] = pair;
```

支持默认值、跳过元素和嵌套结构后，处理表和列表会更直接。

## 9. 异常、defer 和 pcall 覆盖错误处理需求

XScript 既支持结构化异常，也支持安全调用。

```xs
try
{
    risky();
}
catch e
{
    printf("error", e);
}
finally
{
    printf("cleanup");
}
```

`defer` 很适合资源清理：

```xs
function read(path)
{
    var f = file.open(path, "r");
    defer f:close();
    return f:read("*a");
}
```

`pcall` 则适合把错误转成返回值：

```xs
var ok, result = pcall(function()
{
    return risky();
});
```

这几种机制组合起来，可以覆盖不同风格的错误处理需求。

## 10. 原生支持异步流程

XScript 支持 `async function` 和 `await`，可以把异步任务写得像同步流程一样清晰。

```xs
async function main()
{
    var text = await read_file_async("data.txt");
    var res = await http.get("http://example.com");
    printf(text, res.status);
}
```

异步脚本适合：

- 文件异步读写。
- HTTP 请求。
- Socket 连接。
- 定时器。
- 并发任务组合。

如果配合 `all`、`race`、`timeout` 等异步工具函数，可以写出更高层次的并发逻辑。

## 11. 协程和生成器适合表达流程控制

协程提供显式暂停和恢复能力。

```xs
function worker()
{
    coroutine.yield("step1");
    return "done";
}
```

生成器则适合按需产生数据。

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

iterator(v in range(1, 5))
{
    printf(v);
}
```

这让 XScript 在写状态机、数据流、惰性遍历和任务调度时更自然。

## 12. 标准库覆盖面广

XScript 当前已经具备比较实用的内置库生态。

| 能力 | 示例 |
| --- | --- |
| 字符串 | `string.len`、`split`、`replace`、`trim`、大小写、编码转换 |
| 数学 | `math.random`、三角函数、平方根、指数、对数 |
| 文件 IO | `file.open`、`read`、`write`、`seek`、`flush`、`close` |
| 操作系统 | 文件目录判断、创建、删除、复制、当前路径、系统命令 |
| JSON | 编码和解码 JSON 数据 |
| 路径 | 路径拼接、拆分、标准化等 |
| 时间 | 时间获取和格式化 |
| 表工具 | 表查询、包含、处理辅助 |
| 正则 | 正则搜索和匹配 |
| HTTP | GET、POST、通用请求 |
| Socket | TCP/UDP 网络能力 |
| XML | XML 解析和操作 |
| Debug | 调用栈、局部变量、调试辅助 |
| Coroutine | 协程创建、恢复、挂起、状态查询 |

这意味着很多脚本不需要额外绑定宿主函数，就能完成实际工作。

## 13. 模块机制便于组织项目

`require` 支持把脚本拆成模块。

```xs
var iter = require("libs/iter.xs");
var values = iter.map([1, 2, 3], lambda x: x * 2);
```

模块可以返回表作为导出对象：

```xs
return {
    add = add,
    remove = remove
};
```

这让 XScript 可以从“小脚本”逐步组织成“脚本项目”。

## 14. 和 C++ 宿主集成是核心价值

XScript 的定位不是替代 C++，而是补充 C++。

典型分工是：

- C++ 负责性能敏感、平台相关、底层资源和核心系统。
- XScript 负责业务逻辑、配置流程、测试脚本、热更新逻辑和用户可扩展逻辑。

宿主侧可以注册：

- C 函数。
- C++ 类。
- 用户数据对象。
- 自定义库。

脚本侧则可以像调用普通函数和对象方法一样使用这些能力。

## 15. 调试能力比较完备

XScript 不只是“能打印日志”的脚本引擎，项目里已经有比较完整的调试支持，适合真实嵌入式脚本项目排查问题。

调试能力包括：

- **调用栈回溯**：运行时错误或手动检查时，可以获取脚本调用栈，定位错误发生的函数、文件和行号。
- **栈帧查看**：可以查询当前调用栈深度，并按栈帧查看执行上下文。
- **局部变量读取和修改**：支持按索引或名称获取局部变量，也支持在调试过程中修改局部变量。
- **全局变量查看**：可以读取全局变量，方便检查脚本运行状态。
- **函数调试信息**：可以查询函数来源文件、函数名、upvalue 数量等调试信息。
- **断点调试**：调试器支持按文件和行号设置断点。
- **条件断点**：断点可以绑定条件表达式，只有条件满足时才暂停。
- **单步执行**：支持 step in、step over、step out 等常见单步调试模式。
- **暂停与继续**：脚本运行过程中可以暂停、派发调试事件并继续执行。

脚本侧也提供了 `debug` 库，方便在脚本里主动获取调试信息：

```xs
printf(debug.stackbacktrace());

var depth = debug.getstackdepth();
var ok, vars = debug.getlocalvars(0);
```

这些能力对嵌入式脚本尤其重要：宿主程序可以让业务脚本热迭代，同时仍然保留定位问题、检查现场和分析调用链的能力。

## 16. 对测试和行为验证比较友好

项目中已经有综合测试和按特性拆分的测试用例。

这对语言维护很重要，因为脚本语言最怕“行为说不清”。XScript 当前很多特性都有测试脚本作为可运行示例，例如：

- 基础语法
- 控制流
- 函数和闭包
- 表和列表
- 元表
- 协程
- 异步
- 异常
- defer
- 解构
- iterator / generator
- 标准库

使用者不仅能看文档，还能直接看测试学习用法。

## 17. 语法选择有辨识度

XScript 有一些明确的语言风格选择：

- 用 `$` 做字符串拼接，避免和数值加法混淆。
- 用 `? a or b` 做三元表达式，更贴近脚本语义。
- 用表和元表构建对象模型，而不是引入复杂类系统。
- 用 `iterator` 和生成器统一迭代协议。
- 用 `async / await` 表达异步流程。
- 用 `defer` 管理资源清理。

这些设计让语言保持轻量，同时又能覆盖现代脚本常见需求。

## 18. 什么时候特别推荐 XScript

如果你的需求符合下面任意几条，XScript 很值得考虑：

- 你的主项目是 C++。
- 你需要让部分逻辑可配置、可脚本化。
- 你希望脚本语言足够轻量，不想引入大型运行时。
- 你需要闭包、表、元表、协程等高级脚本能力。
- 你希望写自动化脚本、测试脚本、业务规则脚本。
- 你希望脚本能直接使用文件、JSON、HTTP、Socket 等标准能力。
- 你需要逐步扩展语言，而不是被第三方语言运行时完全限制。

## 19. 什么时候不一定适合

客观地说，XScript 并不适合所有场景。

如果你需要：

- 大规模第三方包生态。
- 完整静态类型系统。
- JIT 级别性能。
- 跨平台成熟 IDE 生态。
- 和 Python / JavaScript 一样丰富的社区资源。

那通用语言可能更合适。

但如果目标是“嵌入宿主程序、可控、轻量、脚本化”，XScript 的定位就非常清晰。

## 20. 最能打动人的几个卖点

- **轻量嵌入**：核心由 C++ 实现，适合嵌到现有 C++ 项目。
- **语法完整**：变量、函数、闭包、表、列表、控制流、多返回值都具备。
- **现代特性**：支持可选链、解构、lambda、defer、async/await、generator。
- **脚本友好**：字符串、JSON、文件、路径、时间、HTTP、Socket 等常用库齐全。
- **扩展方便**：宿主可以注册函数、类和用户数据。
- **测试扎实**：大量综合测试和特性测试可作为行为参考。
- **适合热迭代**：业务逻辑可以脚本化，减少 C++ 重新编译成本。

## 21. 一个完整示例：小型数据处理脚本

```xs
var users = [
    { name = "Tom", age = 18, city = "Shanghai" },
    { name = "Jerry", age = 16 },
    { name = "Alice", age = 22, city = "Beijing" }
];

function describe(user)
{
    var city = user?.city || "unknown";
    var type = (user.age >= 18) ? "adult" or "child";
    return user.name $ "(" $ type $ ", " $ city $ ")";
}

function main()
{
    iterator(user in users)
    {
        printf(describe(user));
    }
}

main();
```

这个例子同时展示了：

- 表和列表。
- 函数。
- 可选链。
- 三元表达式。
- 字符串拼接。
- iterator 遍历。

语法不复杂，但表达力足够强。

## 22. 总结

XScript 的优势不在于“成为最大最全的通用语言”，而在于它非常适合作为 C++ 项目的脚本层：

- 足够轻量。
- 足够灵活。
- 足够可控。
- 语言特性已经覆盖大多数脚本需求。
- 内置库和宿主绑定能力让它能处理真实任务。

如果你正在做一个 C++ 项目，并希望给它加一层可快速迭代、可扩展、可配置的脚本能力，XScript 是一个值得投入的选择。
