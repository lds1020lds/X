## ADDED Requirements

### Requirement: require 函数须返回模块导出值
`require(modname)` 调用须编译并执行指定模块文件，捕获模块脚本的返回值，并将该返回值作为 `require` 的返回值传递给调用方。模块脚本可以通过顶层 `return` 语句导出任意值（表、函数、数字、字符串等）。

#### Scenario: 模块返回一个表
- **WHEN** 模块文件 `mylib.xs` 的顶层代码以 `return {add = function(a,b) { return a+b; }}` 结尾
- **THEN** `var M = require("mylib")` 须使 `M` 为该表，且 `M.add(1,2)` 返回 `3`

#### Scenario: 模块返回一个数字
- **WHEN** 模块文件 `version.xs` 的顶层代码以 `return 42` 结尾
- **THEN** `var v = require("version")` 须使 `v` 等于 `42`

#### Scenario: 模块没有显式返回值
- **WHEN** 模块文件 `init.xs` 的顶层代码没有 `return` 语句
- **THEN** `var r = require("init")` 须使 `r` 等于 `true`（布尔真值 `1`），表示模块已成功加载

### Requirement: require 须对已加载模块进行缓存
`require` 须维护一个模块缓存。首次加载模块时，将模块返回值存入缓存；后续对同一模块名的 `require` 调用须直接返回缓存值，不得重新编译或执行模块文件。

#### Scenario: 重复 require 同一模块返回缓存值
- **WHEN** 第一次调用 `require("mylib")` 成功加载并返回模块值 `M`
- **THEN** 第二次调用 `require("mylib")` 须返回与 `M` 相同的值，且模块文件不被重新执行

#### Scenario: 缓存值为引用类型时保持同一引用
- **WHEN** 模块返回一个表 `T`，且 `var a = require("mod"); var b = require("mod")`
- **THEN** `a` 和 `b` 须引用同一个表对象（修改 `a.x` 后 `b.x` 也随之改变）

### Requirement: require 须自动补全 .xs 扩展名
当传入的模块名不包含 `.xs` 后缀时，`require` 须自动追加 `.xs` 后缀进行文件查找。若模块名已包含 `.xs` 后缀，则不得重复追加。

#### Scenario: 不带扩展名的模块名
- **WHEN** 调用 `require("mylib")`
- **THEN** 须查找文件 `mylib.xs`

#### Scenario: 已带扩展名的模块名
- **WHEN** 调用 `require("mylib.xs")`
- **THEN** 须查找文件 `mylib.xs`，不得查找 `mylib.xs.xs`

### Requirement: require 须通过搜索路径查找模块文件
`require` 须使用 `package.path` 中定义的路径模板来查找模块文件。路径模板以分号 `;` 分隔，模板中的 `?` 须被替换为模块名。须按顺序尝试每个模板，使用第一个找到的文件。

#### Scenario: 在默认路径下找到模块
- **WHEN** `package.path` 为 `"./?.xs"` 且当前目录下存在 `mylib.xs`
- **THEN** `require("mylib")` 须成功加载 `./mylib.xs`

#### Scenario: 在多路径中查找模块
- **WHEN** `package.path` 为 `"./?.xs;./libs/?.xs"` 且 `mylib.xs` 仅存在于 `./libs/` 目录下
- **THEN** `require("mylib")` 须成功加载 `./libs/mylib.xs`

#### Scenario: 所有路径均未找到模块
- **WHEN** `package.path` 中所有模板替换后的文件路径均不存在
- **THEN** `require` 须产生运行时错误，指明模块未找到

### Requirement: require 须处理循环依赖
当模块 A require 模块 B，而模块 B 又 require 模块 A 时，须避免无限递归。在模块开始执行前，须先在缓存中放置一个占位标记；若在执行过程中再次 require 同一模块，须返回该占位标记值。

#### Scenario: 两个模块互相 require
- **WHEN** 模块 A 在执行过程中调用 `require("B")`，而模块 B 在执行过程中调用 `require("A")`
- **THEN** 模块 B 中的 `require("A")` 须返回模块 A 当前的占位值（`true`），不得导致无限递归或崩溃

### Requirement: require 须在模块加载失败时报告错误
当模块文件编译失败或执行过程中发生运行时错误时，`require` 须将错误传播给调用方，且不得将失败的模块存入缓存。

#### Scenario: 模块文件不存在
- **WHEN** 调用 `require("nonexistent")` 且该文件在所有搜索路径中均不存在
- **THEN** 须产生运行时错误

#### Scenario: 模块文件存在语法错误
- **WHEN** 调用 `require("bad_syntax")` 且该文件包含语法错误
- **THEN** 须产生编译错误，且 `package.loaded["bad_syntax"]` 不得被设置

#### Scenario: 模块执行时发生运行时错误
- **WHEN** 调用 `require("runtime_error")` 且该文件在执行过程中抛出错误
- **THEN** 须传播该运行时错误，且 `package.loaded["runtime_error"]` 不得被设置
