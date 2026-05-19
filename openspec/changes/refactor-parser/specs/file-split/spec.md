## ADDED Requirements

### Requirement: 调试输出代码拆分到 ParserDebug.cpp
`OutPutCode`、`GetCodeText`、`FindVarNameBySymbolIndex` 三个函数必须从 `Parser.cpp` 移至新文件 `ParserDebug.cpp`。

#### Scenario: ParserDebug.cpp 包含正确的头文件
- **WHEN** 编译 `ParserDebug.cpp`
- **THEN** 文件包含 `Parser.h` 及所有必要的依赖头文件（`MidCode.h`、`SymbolTable.h`、`XScriptVM.h`），编译无错误

#### Scenario: OutPutCode 功能不变
- **WHEN** 调用 `CParser::OutPutCode` 输出中间码到文件
- **THEN** 输出内容与重构前完全一致

#### Scenario: GetCodeText 功能不变
- **WHEN** 调用 `CParser::GetCodeText` 获取指令文本
- **THEN** 返回的文本与重构前完全一致

#### Scenario: FindVarNameBySymbolIndex 功能不变
- **WHEN** 调用 `CParser::FindVarNameBySymbolIndex` 查找变量名
- **THEN** 返回的变量名与重构前完全一致

### Requirement: 代码优化逻辑拆分到 ParserOptimize.cpp
`optimizeCode`、`insertSaveRegCode`、`updateSymbolsOffset`、`isJumpTarget`、`hasOperandBeenUsed`、`isOperandRelated` 六个函数必须从 `Parser.cpp` 移至新文件 `ParserOptimize.cpp`。

#### Scenario: ParserOptimize.cpp 包含正确的头文件
- **WHEN** 编译 `ParserOptimize.cpp`
- **THEN** 文件包含 `Parser.h` 及所有必要的依赖头文件，编译无错误

#### Scenario: optimizeCode 功能不变
- **WHEN** 调用 `CParser::optimizeCode` 进行中间码优化
- **THEN** 优化结果与重构前完全一致

#### Scenario: insertSaveRegCode 功能不变
- **WHEN** 调用 `CParser::insertSaveRegCode` 插入寄存器保存/恢复代码
- **THEN** 插入的代码与重构前完全一致

### Requirement: 辅助自由函数随对应代码迁移
`IsLogicJmp`、`IsLogicTest`、`IsLogicOp` 等非成员辅助函数必须随其调用者迁移到对应的新文件中。

#### Scenario: IsLogicJmp 和 IsLogicTest 迁移到 ParserOptimize.cpp
- **WHEN** `optimizeCode` 调用 `IsLogicJmp` 或 `IsLogicTest`
- **THEN** 这两个函数在 `ParserOptimize.cpp` 中定义，编译链接无错误

#### Scenario: IsLogicOp 保留在 Parser.cpp
- **WHEN** `ParseExpr` 相关逻辑调用 `IsLogicOp`
- **THEN** 该函数保留在 `Parser.cpp` 中

### Requirement: Parser.cpp 仅保留核心解析逻辑
拆分完成后，`Parser.cpp` 必须仅包含核心语法解析相关的函数。

#### Scenario: Parser.cpp 行数显著减少
- **WHEN** 拆分完成后
- **THEN** `Parser.cpp` 的行数从约 3127 行减少至约 2300 行以内

#### Scenario: 构建系统更新
- **WHEN** 新文件 `ParserDebug.cpp` 和 `ParserOptimize.cpp` 创建后
- **THEN** 项目构建系统（如 `.vcxproj`）中包含这两个新文件，编译链接成功

### Requirement: Parser.h 头文件无需修改类声明
文件拆分不得修改 `CParser` 类的声明结构。

#### Scenario: 头文件保持不变
- **WHEN** 拆分完成后
- **THEN** `Parser.h` 中 `CParser` 类的成员方法声明不因拆分而增减（仅因提取新方法而新增声明）
