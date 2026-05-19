## 1. 查找表重构

- [x] 1.1 将 `GetOpInstr` 函数中的 switch-case 替换为 `static const` 映射表数组，保持返回值不变
- [x] 1.2 将 `GetOpInstrTo` 函数中的 switch-case 替换为 `static const` 映射表数组，保持返回值不变
- [x] 1.3 将 `ParserAssignRight` 中复合赋值运算符的 if-else 链替换为 `static const` 映射表查找
- [x] 1.4 将 `GetCodeText` 中指令码→文本名称的 switch-case（约 50 个 case）替换为 `static const` 映射表
- [x] 1.5 将 `hasOperandBeenUsed` 中判断写入指令的 case 列表替换为 `static const` 集合数组查找
- [x] 1.6 提取 `ComputeFactors` 中整数和浮点分支的比较运算公共模式为辅助函数

## 2. 公共模式提取

- [x] 2.1 在 `Parser.h` 中声明 `ParseParamList(int funcIndex, TOKEN endToken)` 方法
- [x] 2.2 在 `Parser.cpp` 中实现 `ParseParamList`，从 `ParseFunction` 和 `ParseLambda` 的参数解析循环中提取
- [x] 2.3 修改 `ParseFunction` 调用 `ParseParamList(iFuncIndex, TOKEN_TYPE_DELIM_CLOSE_PAREN)`
- [x] 2.4 修改 `ParseLambda` 调用 `ParseParamList(iFuncIndex, TOKEN_TYPE_DELIM_COLON)`
- [x] 2.5 在 `Parser.h` 中声明 `EmitFunctionClosure` 方法
- [x] 2.6 在 `Parser.cpp` 中实现 `EmitFunctionClosure`，从 `ParseFunction` 和 `ParseLambda` 的尾部代码中提取
- [x] 2.7 修改 `ParseFunction` 和 `ParseLambda` 调用 `EmitFunctionClosure`
- [x] 2.8 将 `ParseExpr` 中 OR/AND 两个对称分支合并为参数化的统一逻辑（`jumpInstr`、`shortCircuitValue`、`resultValue`）

## 3. 文件拆分

- [x] 3.1 创建 `ParserDebug.cpp`，包含正确的头文件引用（`Parser.h`、`MidCode.h`、`SymbolTable.h`、`XScriptVM.h`）
- [x] 3.2 将 `OutPutCode`、`GetCodeText`、`FindVarNameBySymbolIndex` 从 `Parser.cpp` 移至 `ParserDebug.cpp`
- [x] 3.3 创建 `ParserOptimize.cpp`，包含正确的头文件引用
- [x] 3.4 将 `optimizeCode`、`insertSaveRegCode`、`updateSymbolsOffset`、`isJumpTarget`、`hasOperandBeenUsed`、`isOperandRelated` 从 `Parser.cpp` 移至 `ParserOptimize.cpp`
- [x] 3.5 将 `IsLogicJmp`、`IsLogicTest` 辅助函数移至 `ParserOptimize.cpp`
- [x] 3.6 确认 `IsLogicOp` 保留在 `Parser.cpp` 中
- [x] 3.7 将 `ParserDebug.cpp` 和 `ParserOptimize.cpp` 加入项目构建系统（`.vcxproj` / `CMakeLists.txt`）

## 4. 验证

- [x] 4.1 编译验证：确保所有文件编译无错误、链接无错误
- [ ] 4.2 功能验证：运行现有测试脚本，确认中间码输出与重构前完全一致
- [x] 4.3 确认 `Parser.cpp` 行数降至约 2300 行以内（实际 2260 行）
