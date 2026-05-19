## 上下文

XScript 虚拟机已有基本的模块加载机制：`host_require` → `RequireMoudle` → `doFile`。当前实现存在以下局限：

- `doFile` 编译并执行模块文件，将 `FuncState*` 存入 `mModuleTable` 作为"已加载"标记，但**丢弃了模块的返回值**
- `host_require` 调用 `RequireMoudle` 后不设置任何返回值，脚本侧 `require()` 返回 `nil`
- `mModuleTable` 是 VM 内部的 C++ 成员，脚本代码无法访问

现有库注册模式（`RegisterHostLib`）已成熟：`io`、`os`、`string`、`math` 等库均通过 `RegisterHostLib` 注册为全局表。`package` 库可复用此模式。

## 目标 / 非目标

**目标：**
- 使 `require()` 返回模块导出值，支持 `var M = require("mylib")` 惯用模式
- 暴露 `package.loaded` 表，使脚本可检查、预填充、清除模块缓存
- 提供 `package.path` 搜索路径机制，支持多目录模块查找
- 处理循环依赖，避免无限递归
- 加载失败时正确报错且不污染缓存

**非目标：**
- 不实现 Lua 的 `package.searchers` / `package.preload` 高级加载器机制
- 不实现 C 动态库加载（`package.loadlib` / `package.cpath`）
- 不修改现有 `doFile` 的公共 API 语义（仅内部增强）
- 不实现模块沙箱或独立环境表

## 决策

### 决策 1：`mModuleTable` 直接作为 `package.loaded`

**选择**：让 `package.loaded` 和 `mModuleTable` 指向同一个 TABLE 对象。

**理由**：
- 天然保持双向同步，无需额外同步逻辑
- `mModuleTable` 已在 `init()` 中通过 `CreateTable()` 创建
- 只需在 `package` 表注册时将 `mModuleTable` 设为 `package` 表的 `"loaded"` 字段即可

**替代方案**：维护两个独立表并手动同步 → 复杂度高、容易不一致，放弃。

### 决策 2：`mModuleTable` 语义从存储 FuncState 变为存储模块返回值

**选择**：`mModuleTable` 中的值从 `FuncState*`（函数原型）改为模块执行后的返回值。

**理由**：
- 当前 `doFile` 将 `ConstructValue(mainFunc)` 存入 `mModuleTable`，仅用于判断"是否已加载"
- 改为存储模块返回值后，`require` 可直接从缓存中取出返回值
- 缓存判断逻辑不变：`getTableValue` 返回 `false` 表示未加载

**影响**：现有 `doFile` 调用方不依赖 `mModuleTable` 中的 FuncState 值，改动安全。

### 决策 3：通过 `mRegValue` 寄存器传递模块返回值

**选择**：模块主函数执行后，从 `mRegValue[0]` 读取返回值。

**理由**：
- XScript 的函数返回值通过 `mRegValue` 寄存器传递（`setReturnAsValue` 写入 `mRegValue[regIndex]`）
- `ProtectCallFunction` 调用 `CallFunction` 执行模块主函数后，返回值已在 `mRegValue[0]` 中
- `host_require` 可通过 `setReturnAsValue(mRegValue[0], 0)` 将模块返回值传递给脚本调用方

**替代方案**：修改 `ProtectCallFunction` 签名增加输出参数 → 侵入性大，影响 `pcall`/`xpcall` 等现有调用方，放弃。

### 决策 4：新增 `RequireModule` 方法，返回 Value

**选择**：新增 `Value RequireModule(const char* moduleName)` 方法，保留原 `RequireMoudle` 兼容性或直接重构。

**理由**：
- 当前 `RequireMoudle` 返回 `void`，无法传递模块返回值
- 新方法负责完整流程：查缓存 → 搜索路径 → 编译执行 → 捕获返回值 → 存缓存 → 返回
- `host_require` 调用新方法并通过 `setReturnAsValue` 设置返回值

**实现要点**：
```
Value RequireModule(const char* moduleName):
  1. 规范化模块名（补 .xs 后缀）
  2. 查 mModuleTable 缓存，命中则直接返回
  3. 在 mModuleTable 中设置占位值 true（防循环依赖）
  4. 通过 package.path 搜索文件路径
  5. CompileFile 编译
  6. ProtectCallFunction 执行
  7. 从 mRegValue[0] 读取返回值
  8. 若返回值为 nil，则使用 true 作为最终值
  9. 将最终值存入 mModuleTable
  10. 返回最终值
  失败时：从 mModuleTable 移除占位值，传播错误
```

### 决策 5：`package` 表注册方式

**选择**：在 `init_base_lib()` 中手动创建 `package` 表并注册为全局变量，不使用 `RegisterHostLib`。

**理由**：
- `package` 表不仅包含函数，还包含数据字段（`loaded` 是表、`path` 是字符串）
- `RegisterHostLib` 只支持函数表，不适合混合数据
- 手动创建表后，通过 `setTableValue` 设置 `loaded` 和 `path` 字段

**实现要点**：
```
init_package_lib():
  1. 创建 package 表
  2. package["loaded"] = mModuleTable（直接引用）
  3. package["path"] = "./?.xs"（默认搜索路径）
  4. 注册 "package" 为全局变量
```

### 决策 6：搜索路径解析策略

**选择**：在 `RequireModule` 内部实现路径搜索，不单独抽取搜索器。

**理由**：
- XScript 只需文件系统搜索，不需要 Lua 那样的多搜索器架构
- 路径解析逻辑简单：分割 `;`，替换 `?`，尝试 `fopen` 检查文件是否存在
- 当模块名已包含路径分隔符（如 `"libs/mymod"`）时，`?` 替换整个模块名

## 风险 / 权衡

| 风险 | 缓解措施 |
|------|----------|
| `mRegValue[0]` 在 `ProtectCallFunction` 返回后可能被覆盖 | 在 `ProtectCallFunction` 返回后立即读取并保存 `mRegValue[0]` 的值 |
| 循环依赖时占位值 `true` 可能不满足调用方预期 | 与 Lua 行为一致，文档中明确说明；调用方可通过 `package.loaded` 预填充来控制 |
| `package.path` 修改后影响所有后续 require | 这是预期行为，与 Lua 一致 |
| 现有使用 `require` 的脚本不期望返回值 | 向后兼容：不使用返回值的代码不受影响 |
| `doFile` 语义变更可能影响非 require 调用路径 | `doFile` 仍返回 `bool`，内部增加返回值捕获但不改变外部接口；`RequireModule` 是新方法 |
