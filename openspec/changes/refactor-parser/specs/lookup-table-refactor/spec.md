## ADDED Requirements

### Requirement: GetOpInstr 使用静态映射表
`GetOpInstr` 函数必须使用静态映射表将运算符类型映射为对应的算术指令码，替代现有的 switch-case 链。

#### Scenario: 查找已知运算符
- **WHEN** 传入 `OP_TYPE_ADD` 等已知运算符类型
- **THEN** 返回对应的指令码（如 `INSTR_ADD`），结果与重构前完全一致

#### Scenario: 查找未知运算符
- **WHEN** 传入映射表中不存在的运算符类型
- **THEN** 返回 `-1`

### Requirement: GetOpInstrTo 使用静态映射表
`GetOpInstrTo` 函数必须使用静态映射表将运算符类型映射为对应的赋值运算指令码，替代现有的 switch-case 链。

#### Scenario: 查找已知运算符（含比较运算符）
- **WHEN** 传入 `OP_TYPE_ADD`、`OP_TYPE_EQUAL` 等已知运算符类型
- **THEN** 返回对应的指令码（如 `INSTR_ADD_TO`、`INSTR_TEST_E`），结果与重构前完全一致

#### Scenario: 查找未知运算符
- **WHEN** 传入映射表中不存在的运算符类型
- **THEN** 返回 `-1`

### Requirement: ParserAssignRight 使用静态映射表
`ParserAssignRight` 中的复合赋值运算符到指令码的映射必须使用静态映射表，替代现有的 if-else 链。

#### Scenario: 普通赋值
- **WHEN** 运算符为 `OP_TYPE_ASSIGN`
- **THEN** 生成 `INSTR_MOV` 指令

#### Scenario: 复合赋值运算符
- **WHEN** 运算符为 `OP_TYPE_ASSIGN_ADD`、`OP_TYPE_ASSIGN_SUB` 等复合赋值运算符
- **THEN** 生成对应的算术指令（如 `INSTR_ADD`、`INSTR_SUB`），结果与重构前完全一致

#### Scenario: 自增自减运算符
- **WHEN** 运算符为 `OP_TYPE_INC` 或 `OP_TYPE_DEC`
- **THEN** 生成 `INSTR_INC` 或 `INSTR_DEC` 指令，且不解析右侧表达式

#### Scenario: 非法运算符
- **WHEN** 运算符不在映射表中
- **THEN** 调用 `Error("illegal operator")` 报告错误

### Requirement: GetCodeText 使用静态映射表
`GetCodeText` 中的指令码到文本名称的映射必须使用静态映射表，替代现有的 switch-case 链（约 50 个 case）。

#### Scenario: 已知指令码
- **WHEN** 传入 `INSTR_MOV`、`INSTR_ADD` 等已知指令码
- **THEN** 返回对应的文本名称（如 `"MOV"`、`"ADD"`），结果与重构前完全一致

#### Scenario: 共享文本名称的指令码
- **WHEN** 传入 `INSTR_ADD` 或 `INSTR_ADD_TO`
- **THEN** 都返回 `"ADD"`（保持现有行为）

### Requirement: hasOperandBeenUsed 使用集合替代 case 列表
`hasOperandBeenUsed` 中判断"写入目标操作数的指令"的 case 列表必须替换为静态集合查找。

#### Scenario: 写入指令判断
- **WHEN** 检查某操作数是否在指定范围内被写入
- **THEN** 使用静态集合判断指令是否为写入指令，结果与重构前完全一致

### Requirement: ComputeFactors 提取比较运算公共模式
`ComputeFactors` 中整数和浮点分支的比较运算（`==`、`!=`、`<`、`>`、`<=`、`>=`）必须提取为辅助函数或模板，消除重复代码。

#### Scenario: 整数比较
- **WHEN** 两个操作数都是整数，运算符为比较运算符
- **THEN** 返回整数类型的 0 或 1，结果与重构前完全一致

#### Scenario: 浮点比较
- **WHEN** 至少一个操作数是浮点数，运算符为比较运算符
- **THEN** 返回整数类型的 0 或 1，结果与重构前完全一致
