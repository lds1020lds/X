## 背景

`Parser.cpp` 是 XScript 脚本引擎的核心语法解析器，当前有 3127 行、约 80KB。文件中混合了核心解析逻辑、调试输出、代码优化三大职责，且存在大量可用查找表替代的 if-else/switch 分支链和重复的代码模式。本次重构旨在提升代码的可维护性和可读性，不改变任何外部行为。

## 目标 / 非目标

**目标：**
- 将 `Parser.cpp` 从 3127 行缩减至约 2000 行
- 消除重复的代码模式（参数解析、函数闭包生成、逻辑运算分支）
- 将 if-else/switch 映射链替换为静态查找表
- 按职责将文件拆分为核心解析、调试输出、代码优化三个文件

**非目标：**
- 不改变解析器的外部行为和生成的中间码
- 不重构 `CParser` 类的公共接口（方法签名保持不变）
- 不修改 `Lexer`、`MidCode`、`SymbolTable` 等依赖模块
- 不引入新的第三方依赖

## 设计决策

### 决策 1：查找表使用 `static const` 数组而非 `std::map`

**选择**：使用 `static const` 结构体数组 + 线性查找，而非 `std::unordered_map`。

**理由**：
- 映射表条目数量较少（最多约 20 项），线性查找性能足够
- 避免引入 `<unordered_map>` 头文件，保持与现有代码风格一致
- `static const` 数组在编译期初始化，无运行时开销
- 代码更简洁直观，易于维护

**替代方案**：`std::unordered_map` — O(1) 查找但初始化开销大，对于小表无优势。

### 决策 2：`ComputeFactors` 保留 if-else 结构，仅提取浮点分支的公共模式

**选择**：不将 `ComputeFactors` 完全改为查找表，而是提取整数分支和浮点分支中的公共比较运算模式。

**理由**：
- `ComputeFactors` 中的整数运算分支有特殊逻辑（如除零检查、幂运算的整数/浮点判断），不适合简单的查找表
- 浮点分支中的位运算和比较运算有独立的返回路径，结构不统一
- 但比较运算（`==`、`!=`、`<`、`>`、`<=`、`>=`）在整数和浮点分支中模式完全一致，可以提取为辅助函数

**替代方案**：完全查找表化 — 需要为每种运算定义 lambda/函数指针，代码反而更复杂。

### 决策 3：`GetOpInstr` / `GetOpInstrTo` / `ParserAssignRight` 使用静态映射表

**选择**：将这三个函数中的 switch/if-else 替换为静态映射表。

**理由**：
- `GetOpInstr`（12 个映射）和 `GetOpInstrTo`（18 个映射）是纯粹的一对一映射，无特殊逻辑
- `ParserAssignRight` 中的复合赋值运算符映射（15 个 if-else）同样是纯映射
- 使用 `struct { int opType; int instrCode; }` 数组，代码量从 60+ 行降至 15 行

### 决策 4：提取 `ParseParamList` 公共方法

**选择**：从 `ParseFunction` 和 `ParseLambda` 中提取参数解析循环为 `ParseParamList(int funcIndex, TOKEN endToken)` 方法。

**理由**：
- 两个函数中的参数解析循环几乎完全相同：检查标识符、处理可变参数 `...`、逗号分隔
- 唯一差异是结束 Token 不同（`ParseFunction` 用 `)`，`ParseLambda` 用 `:`）
- 通过 `endToken` 参数化，可以消除约 30 行重复代码

### 决策 5：提取 `EmitFunctionClosure` 公共方法

**选择**：提取函数定义尾部的"恢复上下文 + 生成 INSTR_FUNC"为 `EmitFunctionClosure(int lastFuncIndex, Factor assignFactor)` 方法。

**理由**：
- `ParseFunction` 和 `ParseLambda` 在函数体解析完成后，都执行相同的序列：恢复 `mCurFuncIndex`、调用 `mMidCode->setCurFuncIndex`、生成 `INSTR_FUNC` 指令
- `ParseFunction` 的尾部有额外的表赋值逻辑（`tableFactor`/`symbIndex`），但可以通过参数区分

### 决策 6：`ParseExpr` 中 OR/AND 分支参数化合并

**选择**：将 OR 和 AND 两个对称分支合并为一个，通过参数区分跳转指令和常量值。

**理由**：
- OR 分支使用 `INSTR_JNE` + 常量 `0`（假值跳转）+ 结果 `1`（真值）
- AND 分支使用 `INSTR_JE` + 常量 `0`（假值跳转）+ 结果 `0`（假值）
- 两者结构完全对称，仅 3 个值不同：`jumpInstr`（JNE/JE）、`shortCircuitValue`（0/1）、`resultValue`（1/0）
- 合并后消除约 50 行重复代码

### 决策 7：文件拆分策略

**选择**：
- `ParserDebug.cpp`：`OutPutCode`、`GetCodeText`、`FindVarNameBySymbolIndex`（约 360 行）
- `ParserOptimize.cpp`：`optimizeCode`、`insertSaveRegCode`、`updateSymbolsOffset`、`isJumpTarget`、`hasOperandBeenUsed`、`isOperandRelated`（约 400 行）
- `Parser.cpp`：保留核心解析逻辑（约 2300 行）

**理由**：
- 调试输出和代码优化是独立的后处理阶段，与核心解析逻辑无耦合
- 拆分后每个文件职责单一，便于独立修改和测试
- 所有拆分出的函数都是 `CParser` 的成员方法，无需修改头文件的类声明

**替代方案**：使用独立的辅助类 — 需要大量重构接口，风险过高。

### 决策 8：`GetCodeText` 和 `hasOperandBeenUsed` 使用集合/映射替代 switch

**选择**：
- `GetCodeText` 中的 switch（约 50 个 case）替换为 `static const` 映射表
- `hasOperandBeenUsed` 中的 case 列表替换为 `static const` 集合（数组 + 查找）

**理由**：
- `GetCodeText` 的 switch 是纯粹的指令码→文本映射，无特殊逻辑
- `hasOperandBeenUsed` 的 case 列表是纯粹的"是否属于写入指令"判断

## 风险 / 权衡

- **[风险] 重构引入解析行为变化** → 缓解：每个重构步骤后运行现有测试脚本验证中间码输出一致
- **[风险] 文件拆分导致编译依赖问题** → 缓解：新文件 `#include "Parser.h"` 及相同的依赖头文件，与原文件保持一致
- **[权衡] 查找表的可读性** → 查找表虽然更紧凑，但对于不熟悉模式的开发者可能需要额外理解成本；通过充分的注释缓解
- **[权衡] `ComputeFactors` 未完全表驱动** → 保留部分 if-else 是因为特殊逻辑（除零、幂运算类型推断）不适合简单映射，避免过度抽象
