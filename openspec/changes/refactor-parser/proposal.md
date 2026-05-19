## Why

`Parser.cpp` 是 XScript 脚本引擎的核心语法解析器，当前有 3127 行代码。文件中存在大量冗余的 if-else 链、重复的模式代码、以及职责混杂的问题，导致代码难以维护和扩展。具体表现为：

1. **大量冗余的 if-else/switch 映射**：`ComputeFactors`（约200行）、`GetOpInstr`/`GetOpInstrTo`（各约60行）、`ParserAssignRight`（约30行 if-else）、`GetCodeText`（约120行 switch）、`hasOperandBeenUsed`（约40行 case 列表）等函数中，存在大量可用查找表替代的分支逻辑。
2. **重复的参数解析模式**：`ParseFunction` 和 `ParseLambda` 中的参数解析逻辑几乎完全相同（检查标识符、处理可变参数 `...`、逗号分隔），可以提取为公共方法。
3. **重复的函数尾部代码**：`ParseFunction` 和 `ParseLambda` 在函数体解析完成后，都有相同的"恢复函数上下文 + 生成 INSTR_FUNC 指令"的代码模式。
4. **调试输出代码占比过大**：`OutPutCode` 和 `GetCodeText` 两个调试函数合计约 250 行，占文件总量的 8%，且与核心解析逻辑无关。
5. **`ParseExpr` 中逻辑运算的 OR/AND 分支高度对称**：两个分支的代码结构几乎一致，仅跳转指令和常量值不同（JNE/JE、0/1），可以参数化合并。
6. **`optimizeCode` 函数过长**：单个函数约 150 行，包含多种不同的优化 pass，缺乏结构化分解。

## What Changes

- 将 `ComputeFactors` 中的 if-else 链重构为查找表 + 函数指针/lambda 映射
- 将 `GetOpInstr` 和 `GetOpInstrTo` 重构为静态映射表
- 将 `ParserAssignRight` 中的复合赋值运算符映射重构为查找表
- 提取 `ParseFunction` 和 `ParseLambda` 中重复的参数解析逻辑为 `ParseParamList` 方法
- 提取函数定义尾部的"恢复上下文 + 生成 INSTR_FUNC"为 `EmitFunctionClosure` 方法
- 将 `ParseExpr` 中 OR/AND 的对称分支合并为参数化的统一逻辑
- 将 `OutPutCode` 和 `GetCodeText` 移至独立的调试输出文件（如 `ParserDebug.cpp`）
- 将 `optimizeCode`、`insertSaveRegCode` 等后处理函数移至独立文件（如 `ParserOptimize.cpp`）
- 将 `GetCodeText` 中的 switch 重构为静态映射表
- 将 `hasOperandBeenUsed` 中的 case 列表重构为集合查找

## Capabilities

### New Capabilities
- `lookup-table-refactor`: 将 if-else/switch 映射链重构为查找表，涵盖 ComputeFactors、GetOpInstr、GetOpInstrTo、ParserAssignRight、GetCodeText、hasOperandBeenUsed
- `extract-common-patterns`: 提取重复的代码模式为公共方法，包括参数解析（ParseParamList）、函数闭包生成（EmitFunctionClosure）、逻辑运算合并
- `file-split`: 将 Parser.cpp 按职责拆分为多个文件：核心解析（Parser.cpp）、调试输出（ParserDebug.cpp）、代码优化（ParserOptimize.cpp）

### Modified Capabilities
（无已有 spec 需要修改）

## Impact

- **受影响的文件**：
  - `Parser.cpp`（主要重构目标，3127行 → 预计约 2000 行）
  - `Parser.h`（新增提取的方法声明）
  - 新增 `ParserDebug.cpp`（调试输出相关代码）
  - 新增 `ParserOptimize.cpp`（代码优化相关代码）
- **编译/构建**：需要将新文件加入项目构建系统
- **功能影响**：纯重构，不改变任何外部行为和解析结果
- **风险**：重构涉及核心解析逻辑，需要确保所有语法结构的解析结果不变
