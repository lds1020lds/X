## 1. package 表基础设施

- [x] 1.1 在 `XScriptVM.h` 中声明 `init_package_lib()` 函数和 `RequireModule` 方法（返回 `Value`）
- [x] 1.2 在 `XScriptVM::init()` 中调用 `init_package_lib()`，创建 `package` 全局表，将 `mModuleTable` 设为 `package.loaded`，设置 `package.path` 默认值 `"./?.xs"`
- [x] 1.3 验证脚本中可访问 `package`、`package.loaded`、`package.path`

## 2. 搜索路径解析

- [x] 2.1 在 `XScriptVM.cpp` 中实现路径搜索辅助函数：解析 `package.path`，按 `;` 分割模板，将 `?` 替换为模块名，逐个尝试 `fopen` 检查文件是否存在，返回第一个找到的文件路径
- [x] 2.2 处理模块名已包含 `.xs` 后缀的情况（不重复追加）

## 3. 核心 RequireModule 实现

- [x] 3.1 实现 `Value XScriptVM::RequireModule(const char* moduleName)`：查缓存 → 设占位值 → 搜索路径 → 编译 → 执行 → 捕获返回值 → 存缓存 → 返回
- [x] 3.2 在模块执行前向 `mModuleTable` 写入占位值 `true`，防止循环依赖导致无限递归
- [x] 3.3 模块执行后从 `mRegValue[0]` 读取返回值；若为 nil 则使用 `true` 作为最终缓存值
- [x] 3.4 加载失败时（编译错误或运行时错误）从 `mModuleTable` 移除占位值，传播错误

## 4. host_require 返回值传递

- [x] 4.1 修改 `host_require`：调用 `RequireModule` 获取返回值，通过 `setReturnAsValue` 将其设为宿主函数返回值
- [x] 4.2 确保现有不使用 `require` 返回值的脚本代码向后兼容

## 5. 测试

- [x] 5.1 编写测试脚本：验证 `require` 返回模块导出的表，且可通过返回的表调用模块函数
- [x] 5.2 编写测试脚本：验证重复 `require` 同一模块返回缓存值（同一引用）
- [x] 5.3 编写测试脚本：验证 `package.loaded` 可读取、预填充、清除缓存
- [x] 5.4 编写测试脚本：验证 `package.path` 多路径搜索和模块未找到时的错误报告
- [x] 5.5 运行现有综合测试套件，确保无回归
