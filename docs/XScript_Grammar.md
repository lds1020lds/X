# XScript 语法指南

本文档是一份面向 XScript 使用者和维护者的语法指南，描述当前项目中已经实现并经过测试覆盖的主要语言特性。它不是抽象设计稿，而是以当前 Lexer、Parser、VM 和测试用例中的实际行为为准。

## 1. XScript 程序结构

一个 XScript 源文件由一系列语句组成，解释器会从文件顶部开始顺序编译并执行。

```xs
require("libs/helper.xs");

var version = "1.0";

function main()
{
    printf("hello XScript", version);
}

main();
```

常见顶层内容包括：

- 变量声明
- 函数声明
- 表和列表初始化
- 模块加载
- 普通函数调用
- 控制流语句

语句通常可以用分号结束；在很多位置分号可省略，但推荐在简单语句后保留分号，便于阅读和减少歧义。

## 2. 注释、标识符与字面量

### 2.1 注释

```xs
// 单行注释

/*
   多行注释
*/
```

### 2.2 标识符

标识符可以由字母、数字、下划线和高字节字符组成。由于词法器会优先识别数字，标识符不能以数字开头。

```xs
var name = "xscript";
var 名称 = "脚本";
```

以下前缀由编译器内部保留，用户不要用它创建变量、参数或函数名：

```text
__xscript_internal_
```

### 2.3 基础字面量

```xs
var i = 123;
var hex = 0xff;
var f = 3.14;
var s = "hello\nworld";
var t = true;
var fa = false;
var empty = nil;
```

说明：

- `true` 当前按整数 `1` 表示。
- `false` 当前按整数 `0` 表示。
- `nil` 表示空值。
- 字符串使用双引号，支持 `\t`、`\r`、`\n`、`\"`、`\\` 等常见转义。
- 三引号字符串使用 `"""..."""`，适合编写多行文本，结果仍然是普通字符串。

### 2.4 三引号多行字符串

三引号字符串使用 `"""..."""` 形式。它和普通双引号字符串一样产生普通字符串值，但允许跨越多行，并且内部普通双引号不需要转义。

```xs
var text = """line1
line2 with "quotes"
line3""";
```

物理换行会保留为字符串内容中的 `\n`。因此上面的字符串等价于：

```xs
var text = "line1\nline2 with \"quotes\"\nline3";
```

三引号字符串也支持普通字符串中的常见转义：

```xs
var text = """tab:\t newline:\n slash:\\""";
```

说明：

- 三引号字符串会进入现有字符串常量和字符串驻留机制，运行时类型与普通字符串相同。
- 三引号字符串不支持插值；需要插值时使用 `f"..."` 或 `$` 拼接。
- 如果字符串内容中需要出现连续三个双引号，应使用转义写法，例如 `\"\"\"`。
- 当前 Lexer 的单个 token 长度仍受 `MAX_LEXEME_SIZE` 限制，超长文本建议拆分或从文件读取。

### 2.5 f-string 字符串插值

f-string 使用 `f"..."` 形式，可以在字符串中用 `{expr}` 插入表达式结果。

```xs
var name = "Bob";
var age = 18;

var text = f"name={name}, age={age}";
var sum = f"1 + 2 = {1 + 2}";
var nested = f"user={f\"{name}:{age}\"}";
```

插值表达式按普通表达式解析，因此可以使用变量、运算、函数调用、字段访问和索引访问。

```xs
var obj = { name = "box", value = 42 };
var arr = ["zero", "one"];

var s1 = f"obj={obj.name}:{obj.value}";
var s2 = f"arr[1]={arr[1]}";
var s3 = f"call={toString(123)}";
```

说明：

- 没有插值的 `f"hello"` 等价于普通字符串。
- 空插值 `{}` 会被忽略，例如 `f"a{}b"` 得到 `"ab"`。
- `{{` 可以输出一个左花括号 `{`。
- 未匹配的 `{` 或 `}` 会按普通文本保留。
- f-string 内部会按字符串拼接处理，插值结果会参与 `$` 风格的字符串拼接。

## 3. 变量声明与赋值

### 3.1 基本声明

```xs
var a;
var b = 1;
var c, d = 1, 2;
```

未初始化的变量值为 `nil`。

### 3.2 多重赋值

```xs
var a, b = 1, 2;
a, b = b, a;
```

多重赋值会先求出右侧结果，再写入左侧变量，因此可以安全交换变量。

### 3.3 右值不足与多返回值

右值不足时，多余左值会被填成 `nil`。

```xs
var a, b, c = 1, 2;
// c == nil
```

函数可以返回多个值，赋值语句会接收这些返回值。

```xs
function pair()
{
    return "left", "right";
}

var l, r = pair();
```

### 3.4 复合赋值

```xs
x += 1;
x -= 1;
x *= 2;
x /= 2;
x %= 3;
x ^= 2;
x $= "suffix";
x &= 1;
x |= 2;
x #= 3;
x <<= 1;
x >>= 1;
x++;
x--;
```

其中 `$=` 是字符串拼接赋值，`#=` 是按位异或赋值。

## 4. 表达式与运算符

### 4.1 算术和比较

```xs
var a = 1 + 2 * 3;
var b = (1 + 2) * 3;
var ok = a >= b;
```

支持：

```text
+  -  *  /  %  ^
== != < > <= >=
```

### 4.2 字符串拼接

XScript 使用 `$` 做字符串拼接。

```xs
var name = "XScript";
var text = "hello " $ name;
text $= "!";
```

### 4.3 位运算

```xs
var a = 0x0f & 0xf0;
var b = 1 | 2;
var c = 1 # 3;
var d = 1 << 4;
var e = 16 >> 2;
```

说明：`#` 是按位异或。

### 4.4 逻辑运算

```xs
var a = left && right;
var b = left || right;
var c = !flag;
```

`&&` 和 `||` 支持短路求值，并返回被选中的原始操作数，而不是总是返回 `0` 或 `1`。

```xs
var name = inputName || "anonymous";
var result = obj && obj.value;
```

`||` 使用逻辑真假判断，`nil`、`0`、空字符串等假值都会触发右侧默认值。如果只想在左侧为 `nil` 时使用默认值，应使用 `??`。

### 4.5 空值合并操作符

`??` 用于为空值提供默认值。它只在左侧结果为 `nil` 时求值并返回右侧表达式；如果左侧不是 `nil`，直接返回左侧原始值，并且不会求值右侧。

```xs
var name = user?.profile?.name ?? "anonymous";
var count = maybeCount ?? 0;
```

`??` 与 `||` 的区别是：`??` 只检查 `nil`，不会把 `0` 或空字符串当成缺失值。

```xs
var a = 0 ?? 100;        // 0
var b = "" ?? "empty";  // ""
var c = nil ?? "fallback";

var d = 0 || 100;        // 100
var e = "" || "empty";  // "empty"
```

`??` 支持短路求值，右侧可以是完整表达式，也可以继续链式使用。

```xs
var value = primary ?? secondary ?? "default";
var total = maybeNumber ?? 1 + 2 * 3;
```

### 4.6 管道操作符

管道操作符用于把左侧表达式结果传入右侧调用表达式，让连续的数据处理流程更容易阅读。当前支持自动填充管道 `|>` 和手动占位管道 `|>>` 两种形式。

#### 自动填充管道 `|>`

`left |> call(args...)` 会把 `left` 的结果自动填充为右侧第一个调用的第一个显式参数。

```xs
function add(a, b)
{
    return a + b;
}

function square(x)
{
    return x * x;
}

var a = 3 |> add(4);        // 等价于 add(3, 4)
var b = 3 |> add(4) |> square();
```

方法调用中，`obj:method(args...)` 的接收者 `obj` 仍然作为隐式 `self` 参数传入，管道值填充第一个显式参数。

```xs
var obj = {base = 10, add = function(self, x) { return self.base + x; }};
var value = 5 |> obj:add(); // 等价于 obj:add(5)
```

自动填充只会在当前右侧表达式中填充一次。如果需要在多个位置使用管道值，应使用 `|>>`。

#### 手动占位管道 `|>>`

`|>>` 进入手动占位模式，右侧表达式中可以使用 `_` 表示管道源值。

```xs
function sum3(a, b, c)
{
    return a + b + c;
}

var value = 4 |>> sum3(_ * _, _ + 1, _); // 16 + 5 + 4
```

`_` 只能在 `|>>` 右侧表达式中使用；在普通表达式或 `|>` 自动填充模式中使用 `_` 是非法写法。每次 `_` 出现都会复制一份当前管道源值，因此多个 `_` 不共享同一个临时寄存器，后续运算不会污染原始管道源值。

```xs
var value = 7 |>> sum3(_ + 1, _ * 2, _); // 8 + 14 + 7
```

管道可以嵌套。内层管道有自己的源值，结束后外层 `|>>` 的 `_` 仍然指向外层源值。

```xs
var nested = 3 |>> add(10 |>> add(_, 1), _); // add(11, 3)
```

说明：

- 管道按从左到右的顺序求值，可以连续链式使用。
- 管道优先级低于 `??`，高于三元表达式。
- 左侧表达式会先完整求值，再进入右侧管道表达式。
- 右侧通常应是函数调用或方法调用；自动填充依赖右侧调用参数位置。
- `|>>` 适合多个参数、多次引用、复杂表达式和嵌套调用场景。

### 4.7 三元表达式

XScript 的三元表达式使用 `or` 分隔真假分支：

```xs
var max = (a > b) ? a or b;
var label = name ? name or "anonymous";
```

注意：这里不是 C 风格的 `? :`。当 `?` 后面紧跟 `(`、`[` 或 `:` 时，建议写空格，避免与可选链 token 冲突。

```xs
var value = cond ? (a + b) or 0;
```

### 4.8 运算符优先级

从高到低大致为：

| 优先级 | 运算符 | 说明 |
| --- | --- | --- |
| 1 | `()` `[]` `.` `:` 可选链 | 调用、索引、字段、方法 |
| 2 | 一元 `+` `-` `!` | 一元运算 |
| 3 | `*` `/` `%` `^` | 乘除、取模、幂 |
| 4 | `+` `-` `$` | 加减、拼接 |
| 5 | `<<` `>>` | 位移 |
| 6 | `<` `>` `<=` `>=` | 大小比较 |
| 7 | `==` `!=` | 相等比较 |
| 8 | `&` | 按位与 |
| 9 | `#` | 按位异或 |
| 10 | `|` | 按位或 |
| 11 | `&&` | 逻辑与 |
| 12 | `||` | 逻辑或 |
| 13 | `??` | 空值合并 |
| 14 | `|>` `|>>` | 管道操作符 |
| 15 | `? expr or expr` | 三元表达式 |

复杂表达式建议主动加括号。

## 5. 代码块与作用域

代码块使用 `{}`。

```xs
{
    var x = 10;
    printf(x);
}
```

块会创建新的局部作用域。内层变量可以遮蔽外层变量。

```xs
var x = 1;
{
    var x = 2;
    printf(x); // 2
}
printf(x);     // 1
```

函数、循环、条件语句内部也会涉及作用域。闭包可以捕获外层局部变量。

## 6. 条件语句

```xs
if (score >= 60)
{
    printf("pass");
}
else
{
    printf("fail");
}
```

单语句可以不写块，但推荐使用块提升可读性。

```xs
if (ok) printf("ok");
else printf("bad");
```

### 6.1 switch / case

`switch` 用于按一个表达式的值分派到不同分支。

```xs
var value = "SS";

switch (value)
{
    case "AA":
        printf("aa");
        break;
    case "SS":
        printf("ss");
        break;
    default:
        printf("default");
        break;
}
```

`case` 当前支持整数、浮点数和字符串字面量。`default` 是可选分支，当没有任何 `case` 命中时执行。

```xs
var code = 2;

switch (code)
{
    case 1:
        printf("one");
        break;
    case 2:
        printf("two");
        break;
}
```

多个连续 `case` 可以共享同一段分支语句。

```xs
switch (value)
{
    case "yes":
    case "YES":
        printf("yes");
        break;
    default:
        printf("no");
        break;
}
```

`case` 分支可以直接包含多条语句，不要求额外使用 `{}` 包裹。

```xs
switch (code)
{
    case 1:
        printf("begin");
        code += 10;
        printf(code);
        break;
}
```

如果分支没有执行 `break`，会继续顺序执行后续分支，也就是支持显式 fallthrough。

```xs
switch (code)
{
    case 1:
        printf("one");
    case 2:
        printf("two");
        break;
}
```

说明：

- `break` 可以出现在 `switch` 分支内的任意嵌套位置，执行后跳出当前 `switch`。
- 嵌套 `switch` 中的 `break` 只跳出最内层 `switch`。
- `default` 最多只能出现一次。
- 重复的 `case` 值是非法写法。
- `float case` 使用精确相等匹配，不做误差容忍；不建议用浮点计算结果匹配 `case 0.3` 这类值。
- `string case` 依赖 VM 的字符串驻留机制，相同内容字符串必须通过 `NewXString()` 共享同一个 `XString*`。

### 6.2 match 模式匹配

`match` 用于按值的结构、常量、类型和条件进行模式匹配。基本格式如下：

```xs
match value
{
    pattern if (guard) =>
    {
        // 命中 pattern 且 guard 为真时执行
    }

    _ =>
    {
        // 默认分支
    }
}
```

每个分支左侧可以写一个或多个 pattern，多个 pattern 之间用 `,` 分隔；每个 pattern 可以拥有自己的 `if guard`。

```xs
match value
{
    x(number) if (x > 100) =>
    {
        printf("large number");
    }

    x(string) if (string.len(x) > 10) =>
    {
        printf("long string");
    }

    1, 2, 3 =>
    {
        printf("small constant");
    }
}
```

执行顺序是从上到下、从左到右依次尝试：

1. pattern 本身匹配；
2. 如果 pattern 带类型限制，先检查类型；
3. 如果 pattern 带 guard，再执行 guard；
4. 以上都成功才进入右侧分支块；任一步失败都会继续尝试下一个 pattern。

支持的 pattern 包括：

- 常量 pattern：整数、浮点数、字符串等常量值。
- 变量绑定 pattern：`x`，匹配当前值并绑定到变量 `x`。
- 通配 pattern：`_`，匹配任意值但不绑定变量。
- 列表 pattern：`[x, y, 100]`，按元素顺序匹配 list 前缀；当前允许被匹配 list 包含额外尾部元素。
- 表 pattern：`{name: who, age: 18}`，按字段匹配 table；当前 table pattern 是部分字段匹配，不要求 table 只包含这些字段。
- 嵌套 pattern：list 和 table pattern 可以互相嵌套。

#### 类型限制

变量绑定 pattern 和通配 pattern 可以使用 `name(type)` 或 `_(type)` 限制可接受的数据类型。

```xs
match value
{
    x(number) if (x > 0) =>
    {
        printf("positive number");
    }

    _(string) =>
    {
        printf("some string");
    }
}
```

类型限制可以写多个类型名，用逗号分隔。

```xs
match value
{
    x(int, float) =>
    {
        printf("number value");
    }

    x(string, table) =>
    {
        printf("string or table");
    }
}
```

`number` 是组合类型，等价于 `int` 或 `float`。类型限制会在 guard 之前执行，因此可以避免 guard 对错误类型做比较或字段访问。

#### 多 pattern 绑定规则

同一个分支如果包含多个会绑定变量的 pattern，这些 pattern 必须绑定**相同数量、相同顺序、相同名字**的变量。

```xs
match data
{
    [name(string), age(number)] if (age < 18),
    {name: name(string), age: age(number)} if (age < 18) =>
    {
        printf(name);
    }
}
```

下面这些写法是非法的：

```xs
match data
{
    [x, y], [x] => { }        // 变量数量不同
    [x, y], [x, z] => { }     // 变量名字不同
    [x, y], [y, x] => { }     // 变量顺序不同
    1, x(number) => { }       // 常量 pattern 和变量 pattern 混用
}
```

#### 注意事项

- `pattern if guard, pattern if guard` 表示每个 pattern 拥有自己的 guard，不存在多个 pattern 共用同一个 guard 的语义。
- `guard` 只在 pattern 和类型限制都成功后执行；guard 运行时出错不会被当成匹配失败吞掉。
- list pattern 当前是前缀匹配，不支持 `[x, y, ..rest]` 这类 rest pattern；如需精确长度，需要额外用 guard 检查 list 长度。
- table pattern 使用 `{field: pattern}` 写字段匹配。为避免和字段重命名冲突，不支持 `{x:number}` 这类类型限制简写；请写成 `{x: x(number)}`。
- 分支绑定变量只在当前分支块内生效，不会覆盖外层同名变量。

## 7. 循环

### 7.1 while

```xs
var i = 0;
while (i < 5)
{
    printf(i);
    i++;
}
```

### 7.2 for

```xs
for (var i = 0; i < 5; i++)
{
    printf(i);
}
```

`for` 的初始化部分可以是 `var` 声明或赋值，增量部分使用赋值/自增语句。

### 7.3 foreach

`foreach` 使用 Lua 风格的三值迭代协议，常用于 `pairs`、`ipairs`、`lenum` 等库函数返回的迭代器。

```xs
var t = { a = 10, b = 20 };
foreach (k, v in pairs(t))
{
    printf(k, v);
}
```

### 7.4 iterator

`iterator` 是面向对象风格的迭代语法。被迭代对象需要提供 `__iterator__()`，返回的迭代器对象需要提供 `__next__()`。

```xs
iterator(v in [1, 2, 3])
{
    printf(v);
}
```

多值迭代：

```xs
iterator(k, v in tableValue)
{
    printf(k, v);
}
```

约定：

- `__iterator__()` 返回迭代器对象。
- `__next__()` 返回 `ok, value...`。
- `ok == 1` 表示继续。
- `ok == 0` 表示结束。
- `iterator` 支持 `break` 和 `continue`。

### 7.5 break 与 continue

```xs
while (1)
{
    if (done)
        break;
    if (skip)
        continue;
}
```

`break` 可以在循环或 `switch` 内部使用；在循环中跳出当前循环，在 `switch` 中跳出当前 `switch`。`continue` 只能在循环内部使用。

## 8. 函数

### 8.1 普通函数

```xs
function add(a, b)
{
    return a + b;
}

var r = add(1, 2);
```

### 8.2 局部函数

```xs
var function helper(x)
{
    return x * 2;
}
```

### 8.3 匿名函数

```xs
var fn = function(a, b)
{
    return a + b;
};

var r = (function(x) { return x * x; })(5);
```

### 8.4 方法定义和方法调用

使用 `:` 定义方法时，Parser 会自动添加 `self` 参数。

```xs
var obj = { value = 10 };

function obj:add(x)
{
    return self.value + x;
}

var r = obj:add(5);
```

可以理解为：

```xs
obj.add(obj, 5);
```

### 8.5 Lambda

Lambda 用于编写短函数表达式。

```xs
var add = lambda a, b: a + b;
var inc = lambda x: x + 1;
var noArg = lambda : 42;
```

Lambda 可以捕获外部变量，也可以作为参数或返回值。

```xs
function makeAdder(base)
{
    return lambda x: base + x;
}

var add10 = makeAdder(10);
printf(add10(5));
```

### 8.6 可变参数

函数参数中可以使用 `...`。额外参数会被收集到内部表 `__xscript_varargs`。

```xs
function sum(...)
{
    var total = 0;
    for (var i = 0; i < __xscript_varargs.n; i++)
    {
        total += __xscript_varargs[i];
    }
    return total;
}
```

固定参数和可变参数可以混用。

```xs
function join(prefix, ...)
{
    var result = prefix;
    for (var i = 0; i < __xscript_varargs.n; i++)
        result $= __xscript_varargs[i];
    return result;
}
```

### 8.7 函数默认参数

函数参数可以使用 `=` 指定默认值。调用时如果实参数量不足，对应参数会在函数入口处用默认表达式初始化。

```xs
var global_default = 10;

function f(a, b = 2, c = b + global_default)
{
    return a + b + c;
}

var r1 = f(1, 3, 4); // 8
var r2 = f(1, 3);    // c 使用 b + global_default，结果 14
var r3 = f(1);       // b 使用 2，c 使用 12，结果 15
```

默认值可以是普通表达式，也可以引用前面已经声明的参数或外层变量。

```xs
function makeRange(start, end = start + 10)
{
    return start, end;
}
```

默认参数可以和可变参数一起使用，`...` 必须放在参数列表末尾。

```xs
function log(prefix = "LOG", ...)
{
    printf(prefix, __xscript_varargs.n);
}
```

限制和注意事项：

- 一旦某个参数有默认值，后续普通参数也必须有默认值，或者使用末尾的 `...`。
- `function invalid(a = 1, b)` 是非法写法。
- 显式传入 `nil` 算作已经传参，不会触发默认值。
- 默认表达式在函数调用时按当前环境求值，而不是函数定义时提前固定。

### 8.8 参数自动调整

调用参数不足时，没有默认值的缺失参数为 `nil`；参数过多时，多余参数会被丢弃，除非函数声明了 `...`。

```xs
function f(a, b)
{
    return a, b;
}

var a, b = f(1); // b == nil
```

## 9. 闭包和高阶函数

函数可以捕获外层变量，并在外层函数返回后继续使用。

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

函数可以存入表或列表，也可以作为参数和返回值传递。

```xs
var ops = {
    add = function(a, b) { return a + b; },
    mul = function(a, b) { return a * b; }
};

var funcs = [lambda x: x + 1, lambda x: x * 2];
```

## 10. 表 Table

表使用 `{}`，可以作为字典、对象或混合结构。

```xs
var user = {
    name = "Tom",
    age = 18
};

printf(user.name);
printf(user["age"]);
```

### 10.1 顺序元素

```xs
var t = {10, 20, 30};
printf(t[0]);
```

顺序元素使用从 `0` 开始的整数 key。

### 10.2 动态 key

```xs
var key = "name";
var t = {
    (key) = "XScript",
    (1 + 2) = "three"
};
```

### 10.3 嵌套表

```xs
var config = {
    db = {
        host = "127.0.0.1",
        port = 3306
    }
};
```

## 11. 列表 List

列表使用 `[]`。

```xs
var arr = [1, 2, 3];
arr[0] = 10;
arr:append(4);
printf(arr:size());
```

列表常用方法包括：

- `append`
- `insert`
- `remove`
- `removeAt`
- `resize`
- `size`
- `count`
- `capacity`
- `reverse`
- `sort`
- `sortWithKey`
- `sortWithCmp`

### 11.1 列表和表推导式

推导式用于在表达式位置通过 `iterator` 协议生成新的列表或表，适合把“遍历、过滤、转换、填充容器”写成一个紧凑表达式。

列表推导式使用 `[]`：

```xs
var nums = [1, 2, 3, 4];
var squares = [x * x iterator(x in nums)];
var evens = [x iterator(x in nums) if x % 2 == 0];
```

表推导式使用 `{}`，左侧必须使用括号包裹 key 表达式，右侧是 value 表达式：

```xs
var nums = [1, 2, 3, 4];
var tableValue = {(toString(x)) = x * x iterator(x in nums)};
var evenTable = {(toString(x)) = x iterator(x in nums) if x % 2 == 0};
```

其中 `(keyExpr)` 表示“计算 key”：

```xs
var nums = [1, 2, 3];
var tableValue = {(x) = x * x iterator(x in nums)};
```

`{x = value}` 在普通表初始化中表示字段名 `x`，不是变量 `x` 的值。表推导式要求写成 `{(x) = value iterator(...)}`，用于明确表达 key 来自表达式求值结果。

推导式也支持多值迭代，变量列表与 `iterator` 语句一致：

```xs
var pairsTable = {"a" = 10, "b" = 20};
var values = [k $ ":" $ toString(v) iterator(k, v in pairsTable)];
```

执行语义：

- `iterator(...)` 后面的迭代对象按现有 `__iterator__` / `__next__` 协议执行。
- 每次迭代先绑定迭代变量，再执行可选的 `if` 过滤条件。
- 没有 `if` 条件，或 `if` 条件为真时，才计算填充值。
- 列表推导式把填充值追加到结果列表。
- 表推导式先计算 key，再计算 value，并写入结果表。
- 推导式中的迭代变量只在推导式内部生效，不会泄漏或覆盖外层同名变量。
- 推导式表达式本身返回新创建的列表或表，不会修改原始迭代对象，除非表达式内部显式调用了有副作用的函数。

注意事项：

- 推导式识别依赖填充表达式后是否出现 `iterator`，普通列表和表初始化仍保持原有语义。
- 表推导式的 key 必须写成 `(keyExpr)` 计算 key 形式，例如 `{(x) = x * x iterator(x in nums)}`；`{x = ...}` 保留为普通表字段初始化语义。
- 表推导式的 key/value 分隔符沿用 XScript 表初始化的 `=`，不是 Python 风格的 `:`。
- 推导式内部不直接支持 `break`、`continue`、`return` 这类语句；如需复杂控制流，应改写成普通 `iterator` 语句块。
- 如果 key 重复，后写入的 value 会覆盖前面的同名 key，行为与普通表赋值一致。

## 12. 字段访问、索引访问和链式调用

```xs
obj.name
obj["name"]
arr[0]
obj.a.b.c
obj[expr].name
foo().bar
obj:method(1, 2)
```

`.` 字段访问等价于字符串 key 访问。

```xs
obj.name
// 近似等价于 obj["name"]
```

## 13. 可选链

可选链用于安全访问可能为 `nil` 或假值的对象、函数或方法。普通的 `.`、`[]`、`()`、`:` 后缀要求左侧对象有效；可选链会在执行当前后缀前先检查左侧值，如果左侧为假值则跳过后续访问或调用。

### 13.1 可选字段访问 `?.`

```xs
var name = user?.name;
var city = user?.profile?.address?.city;
```

`obj?.field` 表示：只有 `obj` 为真时才继续读取 `field`。它适合访问可能不存在的对象字段。

### 13.2 可选索引访问 `?[expr]`

```xs
var key = "name";
var value = table?[key];
var first = maybeList?[0];
```

`obj?[expr]` 表示：只有 `obj` 为真时才计算并访问索引。动态 key、列表索引和表达式索引都可以使用这种形式。

### 13.3 可选函数调用 `?(...)`

```xs
var ret = callback?(arg1, arg2);
var result = data?.getUser?()?.name;
```

`fn?(args)` 表示：只有 `fn` 为真时才调用函数。它常用于可选回调、可选工厂函数或链式调用中的中间函数。

### 13.4 可选方法调用 `?:method(...)`

```xs
var text = object?:toString();
var saved = service?:save(data);
```

`obj?:method(args)` 表示：只有 `obj` 为真时才执行方法调用。它和普通 `obj:method(args)` 一样会把 `obj` 作为 `self` 传入方法。

### 13.5 可选链组合与默认值

可选链可以和普通后缀混用，也可以连续使用。

```xs
var title = article?.meta?.title ?? "untitled";
var text = object?:toString() ?? "";
var deep = root?.child?[key]?:format() ?? "missing";
```

常见搭配是与 `??` 提供默认值。可选链访问失败时结果为 `nil`，`??` 只在结果为 `nil` 时使用右侧默认值，因此可以保留 `0`、空字符串等有效但逻辑上为假的值。

如果希望把所有假值都当作缺失值，也可以继续使用 `||`。

```xs
var keepEmptyName = user?.name ?? "anonymous";
var replaceEmptyName = user?.name || "anonymous";
```

注意事项：

- 可选链检查的是当前后缀左侧的值。
- 普通 `nil.field`、`nil[index]`、`nil(args)` 是非法访问；如果对象可能为空，应使用可选链。
- 字面量后不能直接跟字段、索引、调用或方法后缀。
- 三元表达式使用 `? expr or expr`，当 `?` 后紧跟 `.`、`[`、`(`、`:` 时会被识别为可选链 token；必要时加空格或括号避免歧义。

## 14. 解构赋值

XScript 支持表和列表解构，并支持嵌套结构和默认值。

### 14.1 表解构

```xs
var user = { name = "Tom", age = 18 };
var { name, age } = user;
```

字段改名：

```xs
var { name: userName } = user;
```

默认值：

```xs
var { nick = "anonymous" } = user;
```

嵌套解构：

```xs
var data = { user = { name = "Tom" } };
var { user: { name } } = data;
```

### 14.2 列表解构

```xs
var arr = [10, 20, 30];
var [a, b, c] = arr;
```

跳过元素：

```xs
var [first, , third] = arr;
```

嵌套和默认值也可以组合使用。

## 15. 模块与 require

`require` 是运行时库函数，不是语法关键字。

```xs
var mathEx = require("libs/math_ex.xs");
var result = mathEx.add(1, 2);
```

模块文件可以通过 `return` 返回导出表。

```xs
function add(a, b)
{
    return a + b;
}

return {
    add = add
};
```

VM 内部会维护 `package.loaded`，重复 `require` 同一模块时会复用已加载结果。

## 16. 元表和元方法

XScript 支持类似 Lua 的元表机制，可以定制表的索引、赋值、运算和调用行为。

```xs
var obj = {};
var meta = {
    __index = function(t, k)
    {
        return "missing:" $ k;
    }
};

setmetatable(obj, meta);
printf(obj.name);
```

常见元方法包括：

- `__index`
- `__newindex`
- `__equal`
- `__add`
- `__sub`
- `__mul`
- `__div`
- `__mod`
- `__pow`
- `__neg`
- `__len`
- `__less`
- `__lessequal`
- `__concat`
- `__call`

## 17. 异常处理

### 17.1 try / catch / finally

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

`catch` 后面跟异常变量名。`finally` 是可选块。

### 17.2 throw

```xs
function fail(msg)
{
    throw msg;
}
```

可以抛出表达式结果。

### 17.3 pcall / xpcall

基础库还提供安全调用函数。

```xs
var ok, result = pcall(function()
{
    return risky();
});

if (!ok)
    printf("failed", result);
```

## 18. defer

`defer` 用于注册当前函数退出前执行的清理语句。

```xs
function work()
{
    var f = file.open("data.txt", "r");
    defer f:close();

    return f:read("*a");
}
```

`defer` 后面跟一条语句，常用于释放资源、关闭文件、恢复状态。

## 19. 协程

XScript 内置 `coroutine` 库。

```xs
function worker()
{
    coroutine.yield("step1");
    return "done";
}

var co = coroutine.create(worker);
var ok, value = coroutine.resume(co);
printf(ok, value);
```

常用 API：

- `coroutine.create(func)`
- `coroutine.resume(co, ...)`
- `coroutine.yield(...)`
- `coroutine.status(co)`
- `coroutine.wrap(func)`

## 20. async / await

XScript 支持异步函数和 `await`。

```xs
async function load()
{
    var text = await read_file_async("data.txt");
    return text;
}

async function main()
{
    var data = await load();
    printf(data);
}
```

规则：

- `await` 只能在 `async function` 内使用。
- `await expr` 会等待异步结果完成，并把完成值作为表达式结果。
- 异步错误可以配合 `try / catch` 处理。

```xs
async function main()
{
    try
    {
        var res = await http.get("http://example.com");
        printf(res.status);
    }
    catch e
    {
        printf("request failed", e);
    }
}
```

## 21. generator / emit

XScript 支持生成器函数。生成器函数通过 `emit` 产生迭代值，通常和 `iterator` 搭配使用。

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

`emit` 可以产生多个值。

```xs
generator function pairs3()
{
    emit 1, "a";
    emit 2, "b";
}

iterator(k, v in pairs3())
{
    printf(k, v);
}
```

生成器的普通 `return` 不作为一次迭代值。

## 22. 内置库概览

当前 VM 初始化会注册多组内置库，常用能力包括：

| 库 | 说明 |
| --- | --- |
| 基础库 | `print`、`printf`、`type`、`toString`、`toNumber`、`pcall`、`xpcall`、`gc`、`setmetatable`、`getmetatable`、`require` |
| `string` | 长度、查找、截取、分割、替换、trim、大小写、编码转换等 |
| `math` | 随机数、三角函数、平方根、指数、对数等 |
| `io` / `file` | 文件打开、读取、写入、定位、刷新、关闭等 |
| `os` | 文件系统、目录、路径、命令执行等 |
| `socket` | TCP/UDP 相关网络能力 |
| `json` | JSON 编码和解析 |
| `path` | 路径处理 |
| `time` | 时间处理 |
| `table` | 表操作辅助函数 |
| `regex` | 正则搜索和匹配 |
| `http` | HTTP 请求 |
| `debug` | 调试、栈和变量检查 |
| `coroutine` | 协程 |
| `struct` | 二进制结构打包/解包 |

## 23. 推荐编码风格

- 简单语句后保留分号。
- 复杂表达式主动加括号。
- 三元表达式写成 `(cond) ? a or b`，避免可选链歧义。
- 表字段访问优先使用 `obj.name`；动态 key 使用 `obj[key]`。
- 方法调用使用 `obj:method(args)`，方法内部使用 `self`。
- 资源清理优先使用 `defer`。
- 可能失败的异步调用配合 `try / catch`。
- 对可为空对象使用可选链和 `??` 默认值；只有希望把 `0`、空字符串等假值也替换掉时才使用 `||`。

## 24. 当前实现注意事项

- Parser 是递归下降实现，并且边解析边生成中间码，不是 AST 架构。
- `true` / `false` 当前按整数 `1` / `0` 编译。
- `&&` / `||` 是短路表达式，并返回原始操作数。
- `??` 是短路表达式，只在左侧为 `nil` 时求值右侧，并保留 `0`、空字符串等非 `nil` 左值。
- 三元表达式使用 `? trueExpr or falseExpr`。
- 字符串拼接使用 `$`，不是 `+`。
- `switch case` 支持整数、浮点数和字符串字面量；浮点数使用精确匹配，字符串匹配依赖 `NewXString()` 字符串驻留。
- 表和列表的顺序索引从 `0` 开始。
- `await` 只能在异步函数中使用。
- `iterator` 依赖对象的 `__iterator__` / `__next__` 协议。
- 列表和表推导式复用 `iterator` 协议，语法分别为 `[expr iterator(vars in source) if cond]` 和 `{(keyExpr) = valueExpr iterator(vars in source) if cond}`。
- 可变参数通过 `__xscript_varargs` 访问。
- 用户不要定义以 `__xscript_internal_` 开头的变量名。

## 25. 语法速查

```ebnf
program          ::= statement* EOF

statement        ::= varDecl
                   | functionDecl
                   | asyncFunctionDecl
                   | generatorFunctionDecl
                   | ifStatement
                   | switchStatement
                   | whileStatement
                   | forStatement
                   | foreachStatement
                   | iteratorStatement
                   | tryCatchStatement
                   | throwStatement
                   | deferStatement
                   | returnStatement
                   | breakStatement
                   | continueStatement
                   | block
                   | assignmentOrCall

block            ::= '{' statement* '}'
varDecl          ::= 'var' identifierList ('=' exprList)?
identifierList   ::= identifier (',' identifier)*
exprList         ::= expr (',' expr)*

functionDecl     ::= 'function' functionTarget '(' paramList? ')' block
asyncFunctionDecl::= 'async' 'function' functionTarget '(' paramList? ')' block
generatorFunctionDecl ::= 'generator' 'function' functionTarget '(' paramList? ')' block
lambdaExpr       ::= 'lambda' paramList ':' expr
paramList        ::= param (',' param)*
param            ::= identifier ('=' expr)? | '...'

ifStatement      ::= 'if' '(' expr ')' statement
switchStatement  ::= 'switch' '(' expr ')' '{' switchSection* defaultSection? switchSection* '}'
switchSection    ::= 'case' switchCaseValue ':' statement*
defaultSection   ::= 'default' ':' statement*
switchCaseValue  ::= integerLiteral | floatLiteral | stringLiteral
whileStatement   ::= 'while' '(' expr ')' statement
forStatement     ::= 'for' '(' init ';' expr ';' step ')' statement
foreachStatement ::= 'foreach' '(' identifierList 'in' exprList ')' statement
iteratorStatement::= 'iterator' '(' identifierList 'in' expr ')' statement

tryCatchStatement::= 'try' block 'catch' identifier block ('finally' block)?
throwStatement   ::= 'throw' expr
deferStatement   ::= 'defer' statement
returnStatement  ::= 'return' exprList?

expr             ::= ternaryExpr
ternaryExpr      ::= pipelineExpr ('?' expr 'or' expr)?
pipelineExpr     ::= nullCoalescingExpr (('|>' | '|>>') nullCoalescingExpr)*
nullCoalescingExpr ::= logicOrExpr ('??' logicOrExpr)*
logicOrExpr      ::= logicAndExpr ('||' logicAndExpr)*
logicAndExpr     ::= binaryExpr ('&&' binaryExpr)*
postfixExpr      ::= primary postfix*
postfix          ::= '(' args? ')'
                   | '[' expr ']'
                   | '.' identifier
                   | ':' identifier '(' args? ')'
                   | '?.' identifier
                   | '?(' args? ')'
                   | '?[' expr ']'
                   | '?:' identifier '(' args? ')'
tripleStringLiteral ::= '"""' tripleStringChar* '"""'
fStringLiteral   ::= 'f"' fStringPart* '"'
primary          ::= literal
                   | tripleStringLiteral
                   | fStringLiteral
                   | identifier
                   | '(' expr ')'
                   | functionExpr
                   | asyncFunctionExpr
                   | generatorFunctionExpr
                   | lambdaExpr
                   | tableLiteral
                   | listLiteral
                   | awaitExpr

listLiteral      ::= '[' (exprList | listComprehension)? ']'
tableLiteral     ::= '{' (tableFieldList | tableComprehension)? '}'
listComprehension::= expr 'iterator' '(' identifierList 'in' expr ')' ('if' expr)?
tableComprehension ::= '(' expr ')' '=' expr 'iterator' '(' identifierList 'in' expr ')' ('if' expr)?
