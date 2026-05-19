## ADDED Requirements

### Requirement: 提取 ParseParamList 公共方法
必须从 `ParseFunction` 和 `ParseLambda` 中提取参数解析循环为 `ParseParamList(int funcIndex, TOKEN endToken)` 方法，返回解析到的参数数量。

#### Scenario: ParseFunction 调用 ParseParamList
- **WHEN** `ParseFunction` 解析函数参数列表
- **THEN** 调用 `ParseParamList(iFuncIndex, TOKEN_TYPE_DELIM_CLOSE_PAREN)` 完成参数解析，生成的中间码与重构前完全一致

#### Scenario: ParseLambda 调用 ParseParamList
- **WHEN** `ParseLambda` 解析 lambda 参数列表
- **THEN** 调用 `ParseParamList(iFuncIndex, TOKEN_TYPE_DELIM_COLON)` 完成参数解析，生成的中间码与重构前完全一致

#### Scenario: 处理可变参数
- **WHEN** 参数列表中包含 `...`（三点省略号）
- **THEN** 添加 `ARGS` 变量并设置 `HasVarArgs` 标志，然后立即终止参数解析循环

#### Scenario: 检测参数重定义
- **WHEN** 参数列表中出现重复的标识符名称
- **THEN** 调用 `ErrorIdentRedefine` 报告错误

#### Scenario: 空参数列表
- **WHEN** 下一个 Token 就是结束 Token（`)` 或 `:`）
- **THEN** 返回 0（或 `hasSelf` 时返回已有的参数数量），不添加任何参数

### Requirement: 提取 EmitFunctionClosure 公共方法
必须从 `ParseFunction` 和 `ParseLambda` 中提取函数定义尾部的"恢复上下文 + 生成 INSTR_FUNC 指令"为 `EmitFunctionClosure` 方法。

#### Scenario: ParseLambda 使用 EmitFunctionClosure
- **WHEN** `ParseLambda` 完成 lambda 体解析后
- **THEN** 调用 `EmitFunctionClosure` 恢复 `mCurFuncIndex`、设置 `mMidCode` 的当前函数索引、生成 `INSTR_FUNC` 指令，结果与重构前完全一致

#### Scenario: ParseFunction 使用 EmitFunctionClosure
- **WHEN** `ParseFunction` 完成函数体解析后
- **THEN** 调用 `EmitFunctionClosure` 完成相同的上下文恢复和指令生成，包括处理表赋值（`tableFactor`/`symbIndex`）的特殊情况

### Requirement: ParseExpr 中 OR/AND 分支参数化合并
`ParseExpr` 中处理 `OP_TYPE_LOGICAL_OR` 和 `OP_TYPE_LOGICAL_AND` 的两个对称分支必须合并为一个参数化的统一逻辑。

#### Scenario: OR 短路求值
- **WHEN** 表达式包含 `||` 运算符
- **THEN** 使用 `INSTR_JNE` 进行短路跳转，短路结果为 `1`（真），非短路结果为 `0`（假），生成的中间码与重构前完全一致

#### Scenario: AND 短路求值
- **WHEN** 表达式包含 `&&` 运算符
- **THEN** 使用 `INSTR_JE` 进行短路跳转，短路结果为 `0`（假），非短路结果为 `1`（真），生成的中间码与重构前完全一致

#### Scenario: push 模式下的逻辑运算
- **WHEN** `push` 参数为 `true` 且不是后续逻辑运算
- **THEN** 使用 `INSTR_PUSH` 替代 `INSTR_MOV` 将结果压栈，行为与重构前完全一致

#### Scenario: 连续逻辑运算
- **WHEN** 逻辑运算后面还有逻辑运算符（`&&` 或 `||`）
- **THEN** 使用寄存器存储中间结果（`isNextLogic` 分支），行为与重构前完全一致
