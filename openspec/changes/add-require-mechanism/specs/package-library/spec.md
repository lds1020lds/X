## ADDED Requirements

### Requirement: 须注册全局 package 表
系统须在 VM 初始化时注册一个名为 `package` 的全局表，脚本代码可通过 `package` 直接访问该表及其字段。

#### Scenario: 脚本中访问 package 表
- **WHEN** 脚本代码执行 `var p = package`
- **THEN** `p` 须为一个表类型值，且不为 `nil`

#### Scenario: package 表在 VM 启动后即可用
- **WHEN** VM 完成初始化并开始执行脚本
- **THEN** `package` 须已存在于全局作用域中

### Requirement: package.loaded 须映射模块缓存
`package.loaded` 须为一个表，将模块名（字符串）映射到其缓存的返回值。`require` 函数须使用 `package.loaded` 作为模块缓存的唯一来源。

#### Scenario: require 加载模块后更新 package.loaded
- **WHEN** 调用 `require("mylib")` 成功加载模块并返回值 `M`
- **THEN** `package.loaded["mylib"]` 须等于 `M`

#### Scenario: 脚本可读取 package.loaded 检查模块是否已加载
- **WHEN** 模块 `mylib` 已被 require 加载
- **THEN** 脚本代码 `var cached = package.loaded["mylib"]` 须获得该模块的缓存值

#### Scenario: 脚本可预填充 package.loaded 以模拟模块
- **WHEN** 脚本代码执行 `package.loaded["mock"] = {foo = 42}` 后调用 `require("mock")`
- **THEN** `require("mock")` 须直接返回预填充的表 `{foo = 42}`，不得尝试加载文件

#### Scenario: 脚本可通过设置 nil 使模块缓存失效
- **WHEN** 模块 `mylib` 已加载，脚本执行 `package.loaded["mylib"] = nil`
- **THEN** 下一次调用 `require("mylib")` 须重新编译并执行模块文件

### Requirement: package.path 须定义模块搜索路径
`package.path` 须为一个字符串，包含以分号 `;` 分隔的路径模板。每个模板中的 `?` 字符在搜索时须被替换为模块名。系统须提供合理的默认值。

#### Scenario: 默认 package.path 值
- **WHEN** VM 初始化完成且脚本未修改 `package.path`
- **THEN** `package.path` 须包含至少 `"./?.xs"` 模板，确保当前目录下的 `.xs` 文件可被找到

#### Scenario: 脚本可修改 package.path
- **WHEN** 脚本代码执行 `package.path = "./?.xs;./modules/?.xs"`
- **THEN** 后续 `require` 调用须使用新的路径模板进行文件查找

#### Scenario: 路径模板中的问号替换
- **WHEN** `package.path` 为 `"./libs/?.xs"` 且调用 `require("utils")`
- **THEN** 须尝试打开文件 `./libs/utils.xs`

### Requirement: package.loaded 与内部 mModuleTable 须保持同步
`package.loaded` 表须与 VM 内部的 `mModuleTable` 为同一个表对象（或保持双向同步）。通过 C++ 侧修改 `mModuleTable` 须反映到脚本侧的 `package.loaded`，反之亦然。

#### Scenario: C++ 侧加载模块后脚本侧可见
- **WHEN** VM 内部通过 `RequireMoudle` 加载了模块 `"foo"`
- **THEN** 脚本代码访问 `package.loaded["foo"]` 须获得该模块的缓存值

#### Scenario: 脚本侧修改后 C++ 侧可见
- **WHEN** 脚本代码设置 `package.loaded["bar"] = someValue`
- **THEN** VM 内部的 `mModuleTable` 中须包含键 `"bar"` 对应的值
