## 为什么

XScript 已经有了基本的 `require` 函数和内部的 `mModuleTable`，但当前实现与 Lua 的模块系统相比还不完善。具体来说：

1. **`require` 不返回值** — 在 Lua 中，`require("mod")` 会返回模块导出的值（通常是一个表）。而在 XScript 中，`host_require` 调用 `RequireMoudle`，后者调用 `doFile`，但从未向调用方脚本返回任何内容。
2. **没有将 `package.loaded` 暴露给脚本** — 内部的 `mModuleTable` 无法从脚本代码中访问，因此脚本无法检查、预填充或使已缓存的模块失效。
3. **模块返回值未被捕获** — `doFile` 执行模块的主函数但丢弃了其返回值。模块脚本没有办法将表或值"导出"回请求方脚本。

这些缺陷使得 XScript 无法支持像 Lua 中 `local M = require("mylib")` 这样的惯用模块化编程模式。

## 变更内容

- **增强 `require()` 使其返回模块导出值** — 编译并执行模块文件后，捕获其返回值并存储到模块缓存中。后续调用时直接返回缓存的值。
- **将 `package.loaded` 暴露为脚本可访问的表** — 注册一个 `package` 库表，包含 `loaded` 字段，将模块名映射到其缓存的返回值，与 Lua 的 `package.loaded` 对应。
- **支持模块搜索路径** — 添加 `package.path` 来配置 `require` 查找 `.xs` 文件的位置（例如 `"./?.xs;./libs/?.xs"`）。
- **在 `doFile` 流程中捕获模块返回值** — 修改内部执行路径，使模块脚本返回值时能够通过 `require` 传播回来。

## 能力

### 新增能力
- `module-require`：核心 `require()` 函数行为 — 模块加载、缓存、返回值传播和错误处理。
- `package-library`：暴露给脚本的 `package` 表 — `package.loaded`、`package.path` 及相关工具函数。

### 修改的能力
_（无 — 没有需要修改的现有规格）_

## 影响范围

- **`XScriptVM.cpp`** — `RequireMoudle()` 和 `doFile()` 需要捕获并返回模块的返回值。`mModuleTable` 的用途从存储 FuncState 变为存储模块返回值。
- **`XScriptVM.h`** — 增强后的 require 流程需要新的方法签名；`mModuleTable` 语义变更。
- **`libs/xbaselib.cpp`** — `host_require()` 需要设置返回值。新增 `package` 库注册，包含 `loaded` 和 `path` 字段。
- **`libs/xbaselib.h`** — package 库函数的新函数声明。
- **测试文件** — 新增测试脚本，验证 `require` 返回值、`package.loaded` 缓存机制、以及循环/重复 require 的正确处理。
