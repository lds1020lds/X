#include "windows.h"
#include "XScriptVM.h"
#include "XScriptDebugger.h"
#include "Parser.h"
#include "MidCode.h"
#include "SymbolTable.h"
#include <math.h>
#include "libs/xmathlib.h"
#include "libs/xbaselib.h"
#include "libs/xstringlib.h"
#include "libs/xdebuglib.h"
#include "libs/xiolib.h"
#include "libs/xxmllib.h"
#include "libs/xoslib.h"
#include "libs/xlistlib.h"
#include "libs/xsocketlib.h"
#include "libs/xjsonlib.h"
#include "libs/xpathlib.h"
#include "libs/xtimelib.h"
#include "libs/xtablelib.h"
#include "libs/xregexlib.h"
#include "libs/xhttplib.h"
#include "SourceFile.h"
#include <set>


XScriptVM gScriptVM;

// 虚拟机构造函数
XScriptVM::XScriptVM()
	: mCurXScriptState(NULL)
{
	mAsyncRuntime.reset(new AsyncRuntime());
}

XScriptVM::~XScriptVM()
{
	if (mAsyncRuntime)
	{
		std::lock_guard<std::mutex> lock(mAsyncRuntime->mutex);
		mAsyncRuntime->alive = false;
		while (!mAsyncRuntime->completions.empty())
			mAsyncRuntime->completions.pop();
	}
}

// 挂接 XScript 调试器
void XScriptVM::SetDebugger(XScriptDebugger* debugger)
{
	mDebugger = debugger;
	if (mDebugger != NULL)
		mDebugger->AttachVM(this);
}

// 分配下一个全局变量槽位索引（只增不减，O(1)）
int		XScriptVM::AllocGlobalVarIndex()
{
	if (mGlobalVarCount >= MAX_GLOBAL_DATASIZE)
	{
		ExecError("too many global variables, max=%d", MAX_GLOBAL_DATASIZE);
		return -1;
	}
	return mGlobalVarCount++;
}

// 获取变量的全局栈索引（局部变量返回栈索引，全局变量返回编码后的索引）
int		XScriptVM::GetGlobalVarIndex(VariantST* var)
{
	if (var->iScope >= 0)
		return var->stackIndex;

	if (mGlobalVarMap.find(var->varName) != mGlobalVarMap.end())
	{
		int globalIndex = mGlobalVarMap[var->varName];
		int nameIndex = AddString(var->varName);
		return MakeGlobalIndex(globalIndex, nameIndex);
	}

	return -1;
}


// 加载宿主库：注册所有内置函数库（数学、基础、调试、IO、XML、OS、网络等）
void  XScriptVM::loadHostLibs()
{
	InitTableBuiltinMetaClass();
	InitListBuiltinMetaClass();
	InitStringBuildinMetaClass();
	init_math_lib();
	init_base_lib();
	init_debug_lib();
	init_io_lib();
	init_xml_lib();
	init_os_lib();
	init_socket_lib();
	init_json_lib();
	init_path_lib();
	init_time_lib();
	init_regex_lib();
	init_http_lib();
	init_package_lib();
}

// 注册全局变量名，返回其全局槽位索引
int		XScriptVM::RegisterGlobalValue(const std::string& name)
{
	if (mGlobalVarMap.find(name) == mGlobalVarMap.end())
	{
		mGlobalVarMap[name] = AllocGlobalVarIndex();
	}

	return mGlobalVarMap[name];
}

// 编译字符串为可执行的函数原型
FuncState*	XScriptVM::CompileString(const std::string& code)
{
	CParser parser;
	CMidCode midCode;
	CSymbolTable symbolTable;
	symbolTable.AddFunction("_Main", 0, -1);

	CSourceFile sourefile;
	sourefile.LoadFromString(code.c_str());
	bool succ = parser.ParseFile(&sourefile, &midCode, &symbolTable);
	if (!succ)
	{
		return NULL;
	}

	RegisterGlobalValues(symbolTable);

	map<int, FuncState*> FuncMap;

	ConvertMidCodeToInstr(symbolTable, midCode, "", FuncMap);

	FuncState* mainFunc = FuncMap[0];
	mainFunc->marked = MS_White;
	return mainFunc;
}

#ifdef _DEBUG
#define  OUTPUT_MIDCODE
#endif // !_DEBUG

// 编译文件为可执行的函数原型（可选输出中间码）
FuncState*	XScriptVM::CompileFile(const std::string& fileName)
{
	CParser parser;
	CMidCode midCode;
	CSymbolTable symbolTable;
	symbolTable.AddFunction("_Main", 0, -1);

	CSourceFile sourefile;
	if (!sourefile.LoadSourceFile(fileName.c_str()))
	{
		return NULL;
	}

	bool succ = parser.ParseFile(&sourefile, &midCode, &symbolTable);
	if (!succ)
	{
		return NULL;
	}
#ifdef OUTPUT_MIDCODE
	std::string midCodeFileName = fileName + ".code";
	if (!midCodeFileName.empty())
	{
		parser.OutPutCode(&midCode, &symbolTable, (char*)midCodeFileName.c_str());
	}
#endif 
	RegisterGlobalValues(symbolTable);

	// Normalize file path to absolute path (for debugger breakpoint matching)
	std::string absFileName = XScriptDebugger::NormalizePath(fileName);

	map<int, FuncState*> FuncMap;

	ConvertMidCodeToInstr(symbolTable, midCode, absFileName, FuncMap);

	FuncState* mainFunc = FuncMap[0];
	mainFunc->marked = MS_White;
	return mainFunc;
}


// 执行脚本文件：编译并运行
bool  XScriptVM::doFile(const std::string& fileName)
{
	std::string	moudleName = fileName;

	FuncState* mainFunc = CompileFile(fileName);
	if(!mainFunc)
	{
		return false;
	}


	Value fValue = ConstructValue(mainFunc);
	setTableValue(mModuleTable, ConstructValue(moudleName.c_str()), fValue);
	std::string errorDesc;
	if (ProtectCallFunction(fValue.func, 0, errorDesc) != 0)
	{
		printf(errorDesc.c_str());
		return false;
	}

	return true;
}


// 获取操作数的名称描述（用于错误信息）
std::string	XScriptVM::GetOperatorName(RuntimeOperator* op, Value* value)
{
	std::string desc;
	if (op->varNameIndex >= 0)
	{
		desc = GetString(op->varNameIndex);
		desc += "(a " + std::string(getTypeName(value->type)) + " value)";
	}
	else
	{
		desc = getTypeName(value->type);
	}

	return desc;
}

// 获取操作数名称（通过索引）
std::string	XScriptVM::GetOperatorName(int opIndex)
{
	ResolveOpPointer(opIndex, value);
	std::string desc;
	if (mCurInstr->mOpList[opIndex].varNameIndex >= 0)
	{
		desc = GetString(mCurInstr->mOpList[opIndex].varNameIndex);
		desc += "(a " + std::string(getTypeName(value.type)) + " value)";
	}
	else
	{

		desc = getTypeName(value.type);
	}

	return desc;
}


// 获取调用栈回溯信息（用于错误报告）
std::string XScriptVM::stackBackTrace()
{
	std::string stackTraceBack;
	stackTraceBack = "stack traceback:\n";
	int callIndex = mCurXScriptState->mCurCallIndex;
	while (callIndex >= 0)
	{
		if (mCurXScriptState->mCallInfoBase[callIndex].mCurFunctionState != NULL)
		{
			stackTraceBack += "\t" + mCurXScriptState->mCallInfoBase[callIndex].mCurFunctionState->sourceFileName + ":line(";
			if (callIndex == mCurXScriptState->mCurCallIndex)
				stackTraceBack += ConvertToString(mCurInstr->lineIndex);
			else
				stackTraceBack += ConvertToString(mCurXScriptState->mCallInfoBase[callIndex + 1].mCurLine);

			stackTraceBack += "): in function \"" + mCurXScriptState->mCallInfoBase[callIndex].mCurFunctionState->funcName + "\"\n";
		}
		callIndex--;
	}
	stackTraceBack += "\t[C]:?\n";

	return stackTraceBack;
}

// 将中间码转换为虚拟机指令：创建函数原型、解析操作数、建立上值引用
void XScriptVM::ConvertMidCodeToInstr(CSymbolTable &symbolTable, CMidCode &midCode, const std::string& fileName, std::map<int, FuncState*>& funcMap)
{
	for (int iFuncIndex = 0; iFuncIndex < (int)symbolTable.mFuncTable.size(); iFuncIndex++)
	{
		FuncState* funcState = CreateFunctionState();
		funcMap[symbolTable.mFuncTable[iFuncIndex].iIndex] = funcState;
		funcState->sourceFileName = fileName;
		funcState->sourceFileHash = XScriptDebugger::HashFilePath(fileName);
	}

	FuncState* mainFunc = NULL;

	for (int iFuncIndex = 0; iFuncIndex < (int)symbolTable.mFuncTable.size(); iFuncIndex++)
	{
		FunctionST funcST = symbolTable.mFuncTable[iFuncIndex];
		FuncState* funcState = funcMap[symbolTable.mFuncTable[iFuncIndex].iIndex];

		int functionIndex = funcST.iIndex;
		funcState->funcName = funcST.funcName;
		funcState->localDataSize = funcST.localDataSize;
		funcState->localParamNum = funcST.iParamSize;
		funcState->stackFrameSize = funcST.localDataSize + funcST.iParamSize;
		funcState->hasVarArgs = funcST.hasVarArgs;
		funcState->funcType = funcST.funcType;
		funcState->m_tryCatchBlock = funcST.tryCatchBlockVec;

		for (auto& d: funcST.switchStructVec)
		{
			RuntimeSwitchData rtSwitchData;
			rtSwitchData.defaultInstr = d.defaultInstr;
			rtSwitchData.intInstrMap = d.intValueInstrMap;
			rtSwitchData.floatInstrMap = d.floatValueInstrMap;
			for (auto& keyval : d.stringValueInstrMap)
			{
				XString* str = NewXString(keyval.first.c_str());
				rtSwitchData.strInstrMap[str] = keyval.second;
			}
			funcState->mSwitchVec.push_back(rtSwitchData);
		}


		symbolTable.GetFunctionVars(functionIndex, funcState->m_localVarVec);
		symbolTable.GetFunctionLocalDebugInfos(functionIndex, funcState->m_localVarDebugInfoVec);
		if (mainFunc == NULL)
			mainFunc = funcState;

		for (int upValueIndex = 0; upValueIndex < (int)funcST.upValueVec.size(); upValueIndex++)
		{
			UpValueST up = funcST.upValueVec[upValueIndex];
			if (up.type == VLOCAL)
			{
				VariantST* var = symbolTable.GetVarByIndex(up.index);
				up.index = var->stackIndex;
			}

			funcState->m_upValueVec.push_back(up);
		}


		for (int subFuncIndex = 0; subFuncIndex < (int)funcST.subIndexVec.size(); subFuncIndex++)
		{
			funcState->m_subFuncVec.push_back(funcMap[funcST.subIndexVec[subFuncIndex]]);
		}

		if (funcST.parentIndex >= 0)
		{
			funcState->m_parentFunc = funcMap[funcST.parentIndex];
		}

		funcState->mInstrStream.numInstr = (int)midCode.mCodeList[funcST.iIndex].size();
		if (funcState->mInstrStream.numInstr > 0)
			funcState->mInstrStream.instrs = new Instr[funcState->mInstrStream.numInstr];


		for (int i = 0; i < (int)midCode.mCodeList[funcST.iIndex].size(); i++)
		{
			Code code = midCode.mCodeList[funcST.iIndex][i].code;
			funcState->mInstrStream.instrs[i].opType = code.iCodeOpr;
			funcState->mInstrStream.instrs[i].numOpCount = (int)code.oprList.size();
			if (code.oprList.size() > 0)
				funcState->mInstrStream.instrs[i].mOpList = new RuntimeOperator[code.oprList.size()];

			int lineIndex = midCode.mCodeList[funcST.iIndex][i].lineIndex;
			funcState->mInstrStream.instrs[i].lineIndex = lineIndex;

			for (int op = 0; op < (int)code.oprList.size(); op++)
			{
				Operand operand = code.oprList[op];
				RuntimeOperator* value = &funcState->mInstrStream.instrs[i].mOpList[op];
				value->varNameIndex = -1;
				//value->type = operand.operandType;
				switch (operand.operandType)
				{
				case OP_TYPE_INT:
				{
					value->type = ROT_Int;
					value->iIntValue = operand.iIntValue;
					break;
				}
				case OP_TYPE_FLOAT:
				{
					value->type = ROT_Float;
					value->fFloatValue = operand.fFloatValue;
					break;
				}
				case PST_String_Index:
				{
					value->type = ROT_String;
					value->stringValue = NewXString(symbolTable.mStringTable[operand.iStringIndex].str);
					GC_SetFixed(value->stringValue);
					break;
				}
				case PST_Var_Index:
				{
					if (operand.iSymbolIndex & UPVALMASK)
					{
						int upvalIndex = operand.iSymbolIndex - UPVALMASK;
						value->type = ROT_UpVal_Index;
						value->iStackIndex = upvalIndex;
						value->varNameIndex = AddString(CParser::FindVarNameBySymbolIndex(symbolTable, operand.iSymbolIndex, functionIndex));
					}
					else
					{
						value->type = ROT_Stack_Index;
						VariantST* var = symbolTable.GetVarByIndex(operand.iSymbolIndex);
						value->iStackIndex = GetGlobalVarIndex(var);
						value->varNameIndex = AddString(var->varName);
					}

					break;
				}
				case PST_Nil:
				{
					value->type = ROT_Nil;
					break;
				}
				case PST_JumpTarget_Index:
				{
					value->type = ROT_Instr_Index;

					int jumpInstrIndex = -1;
					for (int j = 0; j < (int)midCode.mJumpList[functionIndex].size(); j++)
					{
						if (midCode.mJumpList[functionIndex][j].jumpIndex == operand.iJumppIndex)
						{
							jumpInstrIndex = midCode.mJumpList[functionIndex][j].codeIndex;
							break;
						}
					}
					value->iInstrIndex = jumpInstrIndex;
					break;
				}
				case PST_Table:
				{
					if (operand.iSymbolIndex != -1)
					{
						std::string varName;
						if (operand.iSymbolIndex & UPVALMASK)
						{
							int upvalIndex = operand.iSymbolIndex - UPVALMASK;
							value->type = ROT_UpValue_Table;
							value->iStackIndex = upvalIndex;
							varName = CParser::FindVarNameBySymbolIndex(symbolTable, operand.iSymbolIndex, functionIndex);
						}
						else
						{
							VariantST* var = symbolTable.GetVarByIndex(operand.iSymbolIndex);
						value->iStackIndex = GetGlobalVarIndex(var);
						varName = var->varName;

							value->type = ROT_Table;
						}

						char temp[64] = { 0 };
						std::string subName;
						value->tableIndexType = operand.tableIndexType;
						switch (operand.tableIndexType)
						{
						case ROT_Float:
							value->fFloatTableValue = operand.fFloatTableValue;
							snprintf(temp, 64, XFloatConFmt2, operand.fFloatTableValue);
							subName = temp;
							break;
						case ROT_Int:
							value->iIntTableValue = operand.iIntTableValue;
							snprintf(temp, 64, XIntConFmt2, operand.iIntTableValue);
							subName = temp;
							break;
						case ROT_String:
							value->strTableValue = NewXString(symbolTable.mStringTable[(int)operand.iIntTableValue].str);
							GC_SetFixed(value->strTableValue);
							snprintf(temp, 64, "[%s]", symbolTable.mStringTable[(int)operand.iIntTableValue].str);
							subName = temp;
							break;
						case ROT_Stack_Index:
						{
							if (operand.iIntTableValue & UPVALMASK)
							{
								int upvalIndex = (int)operand.iIntTableValue - UPVALMASK;
								value->tableIndexType = ROT_UpVal_Index;
								value->iIntTableValue = upvalIndex;

								subName = "[" + std::string(CParser::FindVarNameBySymbolIndex(symbolTable, (int)operand.iIntTableValue, functionIndex)) + "]";
							}
							else
							{
								VariantST* var = symbolTable.GetVarByIndex((int)operand.iIntTableValue);
							value->iIntTableValue = GetGlobalVarIndex(var);
								subName = "[" + std::string(var->varName) + "]";
							}
						}

						break;
						case ROT_Reg:
							value->iIntTableValue = operand.iIntTableValue;
							break;
						default:
							break;
						}

						varName += subName;
						value->varNameIndex = AddString(varName);
					}
					break;
				}
				case PST_Reg:
				{
					value->type = ROT_Reg;
					value->iRegIndex = operand.iRegIndex;
					break;
				}
				}
			}

		}

	}
}

// 注册符号表中的全局变量到虚拟机
void XScriptVM::RegisterGlobalValues(CSymbolTable &symbolTable)
{
	const ScopeVarTable* globalScope = symbolTable.GetScopeVars(-1);
	if (globalScope != nullptr)
	{
		for (auto* var : globalScope->varList)
		{
			RegisterGlobalValue(var->varName);
		}
	}
}

// 初始化虚拟机：分配内存、初始化哈希表、全局栈、注册内置库
bool  XScriptVM::init()
{

	mStringHashSize = 32;
	mStringHashTable = new XString*[mStringHashSize];
	memset(mStringHashTable, 0, sizeof(XString*) * mStringHashSize);

	mStrBufferSize = 128;
	mStrBuffer = new char[mStrBufferSize];

	mStringHashUsedSize = 0;
	mEnvTable = NULL;
	mSavedRegDepth = 0;
	mAllowHook = true;
	mHookMask = 0;
	mDebugger = NULL;
	mLongJmp = NULL;
	mNumHostFuncParam = 0;
	mIsInCallScriptFunc = false;
	mNumScriptFuncParams = 0;
	if (!mAsyncRuntime)
		mAsyncRuntime.reset(new AsyncRuntime());
	else
		mAsyncRuntime->alive = true;

	mGlobalVarCount = 0;

	SetCurrentXScriptState(&mMainXScriptState);
	resetScriptState(mCurXScriptState);

	mGlobalStackElements = new Value[MAX_GLOBAL_DATASIZE];
	memset(mGlobalStackElements, 0, sizeof(Value) * MAX_GLOBAL_DATASIZE);
	for (int i = 0; i < MAX_GLOBAL_DATASIZE; i++)
		mGlobalStackElements[i].type = OP_TYPE_NIL;

	mCurInstr = NULL;
	mRootCG = NULL;
	mTableMetaTable = NULL;
	mListMetaTable = NULL;
	mStringMetaTable = NULL;
	mNilValue.type = OP_TYPE_NIL;
	mNilValue.iIntValue = 0;

	mMetaTable = CreateTable();
	mModuleTable = CreateTable();


	loadHostLibs();
	
	//每一个表都必须有metaTable
	mMetaTable->mMetaTable = mTableMetaTable;
	mModuleTable->mMetaTable = mTableMetaTable;

	// 将 mModuleTable 设置为 package.loaded（共享同一个表对象）
	Value* packageValue = GetGlobalValue("package");
	if (packageValue && packageValue->type == OP_TYPE_TABLE)
	{
		setTableValue(packageValue->tableData, "loaded", mModuleTable);
	}

	return true;
}


// 初始化表类型的内置元方法
void XScriptVM::InitTableBuiltinMetaClass()
{
	mTableMetaTable = CreateTable();


	//不能设置metatable为自己了， 否则死循环
	//mTableMetaTable->mMetaTable = mTableMetaTable;
	std::vector<HostFunction> tableMetaVec;
	tableMetaVec.push_back(HostFunction(ITER_FUNC, xtable_Iterator));
	tableMetaVec.push_back(HostFunction("keys", xtable_keys));
	tableMetaVec.push_back(HostFunction("values", xtable_values));
	tableMetaVec.push_back(HostFunction("size", xtable_size));
	tableMetaVec.push_back(HostFunction("contains", xtable_contains));
	tableMetaVec.push_back(HostFunction("clear", xtable_clear));
	tableMetaVec.push_back(HostFunction("clone", xtable_clone));
	tableMetaVec.push_back(HostFunction("merge", xtable_merge));
	tableMetaVec.push_back(HostFunction((char*)MetaMetodString(MMT_ToString), xtable_tostring));

	CreateFunctionTable(tableMetaVec, mTableMetaTable);
	setTableValue(mTableMetaTable, (char*)MetaMetodString(MMT_Index), mTableMetaTable);

	int index = RegisterGlobalValue("table");
	setGlobalStackValue(index, ConstructValue(mTableMetaTable));
}

// 初始化列表类型的内置元方法（append/insert/remove/sort等）
void XScriptVM::InitListBuiltinMetaClass()
{
	mListMetaTable = CreateTable();
	std::vector<HostFunction> listVec;
	listVec.push_back(HostFunction("append", xlist_Append));
	listVec.push_back(HostFunction("insert", xlist_Insert));
	listVec.push_back(HostFunction("remove", xlist_Remove));
	listVec.push_back(HostFunction("removeAt", xlist_RemoveAtPos));
	listVec.push_back(HostFunction("resize", xlist_Resize));
	listVec.push_back(HostFunction("size", xlist_Size));
	listVec.push_back(HostFunction("count", xlist_Count));
	listVec.push_back(HostFunction("capacity", xlist_Capacity));
	listVec.push_back(HostFunction("reverse", xlist_Reverse));
	listVec.push_back(HostFunction("sort", xlist_Sort));
	listVec.push_back(HostFunction("sortWithKey", xlist_SortWithKey));
	listVec.push_back(HostFunction("sortWithCmp", xlist_SortWithCmp));
	listVec.push_back(HostFunction(ITER_FUNC, xlist_Iterator));

	listVec.push_back(HostFunction(MetaMetodString(MMT_Add), xlist_op_Add));
	listVec.push_back(HostFunction(MetaMetodString(MMT_Mul), xlist_op_Mul));
	listVec.push_back(HostFunction(MetaMetodString(MMT_ToString), xlist_op_ToString));

	CreateFunctionTable(listVec, mListMetaTable);
	setTableValue(mListMetaTable, (char*)MetaMetodString(MMT_Index), mListMetaTable);
	int index = RegisterGlobalValue("list");
	setGlobalStackValue(index, ConstructValue(mListMetaTable));

}

// 初始化字符串类型的内置元方法（len/find/sub/split等）
void XScriptVM::InitStringBuildinMetaClass()
{
	mStringMetaTable = CreateTable();
	std::vector<HostFunction> strfuncVec;
	strfuncVec.push_back(HostFunction("len", xstring_len));
	strfuncVec.push_back(HostFunction("rawlen", xstring_rawlen));
	strfuncVec.push_back(HostFunction("find", xstring_find));
	strfuncVec.push_back(HostFunction("sub", xstring_sub));
	strfuncVec.push_back(HostFunction("reg_search", xstring_regex_search));

	strfuncVec.push_back(HostFunction("compare", xstring_compare));
	strfuncVec.push_back(HostFunction("lower", xstring_lower));
	strfuncVec.push_back(HostFunction("upper", xstring_upper));
	strfuncVec.push_back(HostFunction("split", xstring_split));
	strfuncVec.push_back(HostFunction("rfind", xstring_rfind));
	strfuncVec.push_back(HostFunction("replace", xstring_replace));
	strfuncVec.push_back(HostFunction("trim", xstring_trim));
	strfuncVec.push_back(HostFunction("trimLeft", xstring_trimLeft));
	strfuncVec.push_back(HostFunction("trimRight", xstring_trimRight));
	strfuncVec.push_back(HostFunction("startWith", xstring_startWith));
	strfuncVec.push_back(HostFunction("endWith", xstring_endWith));
	strfuncVec.push_back(HostFunction("isalpha", xstring_isalpha));
	strfuncVec.push_back(HostFunction("isdigit", xstring_isdigit));

	strfuncVec.push_back(HostFunction("isspace", xstring_isspace));
	strfuncVec.push_back(HostFunction("islower", xstring_islower));
	strfuncVec.push_back(HostFunction("isupper", xstring_isupper));
	strfuncVec.push_back(HostFunction("atoi", xstring_atoi));
	strfuncVec.push_back(HostFunction("atof", xstring_atof));
	strfuncVec.push_back(HostFunction("utf8togbk", xstring_utf8_to_gbk));
	strfuncVec.push_back(HostFunction("gbktoutf8", xstring_gbk_to_utf8));
	strfuncVec.push_back(HostFunction("unicodetoutf8", xstring_unicode_to_utf8));
	strfuncVec.push_back(HostFunction("utf8tounicode", xstring_utf8_to_unicode));
	strfuncVec.push_back(HostFunction((char*)MetaMetodString(MMT_Mul), xstring_mul));
	CreateFunctionTable(strfuncVec, mStringMetaTable);

	int index = RegisterGlobalValue("string");
	setGlobalStackValue(index, ConstructValue(mStringMetaTable));

	setTableValue(mStringMetaTable, (char*)MetaMetodString(MMT_Index), mStringMetaTable);
}

// 重置脚本执行状态：初始化栈、调用信息、指令索引等
void  XScriptVM::resetScriptState(XScriptState* state)
{
	state->mCurrentCFunction = NULL;
	state->mStartFunction = NULL;
	state->mNextRefUpVals = NULL;
	state->mCurFunction = NULL;
	state->mCurFunctionState = NULL;
	state->mCurCallIndex = -1;
	state->mCallInfoBase = new CallInfo[MAX_LUA_CALL_STACK_DEPTH];
	state->mStatus = CS_Normal;
	state->mSuspendReason = SUSPEND_NONE;
	state->mCoroutineType = ECoroutineType::Normal;
	state->mWaitingFuture = NULL;
	state->mOwnerTask = NULL;
	state->mHasAwaitResumeValue = false;
	state->mAsyncStartParamCount = 0;
	state->mTopIndex = 0;
	state->mFrameIndex = 0;
	state->mStackSize = MIN_STACK_SIZE;
	state->mCCallIndex = 0;
	state->mInstrIndex = 0;
	state->mIsRejected = false;
	state->mCurCCallLuaCallIndex = -1;
	state->mLastProtectCallLuaCallIndex = -1;
	state->mStackElements = new Value[mCurXScriptState->mStackSize];
	memset(&state->mStackElements[0], 0, sizeof(Value) * state->mStackSize);
	for (int i = 0; i < mCurXScriptState->mStackSize; i++)
		state->mStackElements[i].type = OP_TYPE_NIL;

}

int XScriptVM::GetFrameInstrIndex(int callIndex)
{
	int depth = GetStackDepth();
	if (callIndex < 0 || callIndex > depth)
		return 0;
	if (callIndex == depth)
		return mCurXScriptState->mInstrIndex;
	if (callIndex + 1 <= depth)
		return mCurXScriptState->mCallInfoBase[callIndex + 1].mInstrIndex;
	return mCurXScriptState->mCallInfoBase[callIndex].mInstrIndex;
}

// 构建指定调用帧的变量表（局部变量 + upvalue），过滤内部变量
TableValue* XScriptVM::BuildFrameVarsTable(int callIndex, int currentPc)
{
	if (callIndex < 0 || callIndex > GetStackDepth())
		return NULL;

	CallInfo* ci = GetCallInfo(callIndex);
	FuncState* fs = ci->mCurFunctionState;
	if (fs == NULL)
		return NULL;

	if (currentPc < 0)
		currentPc = GetFrameInstrIndex(callIndex);

	TableValue* table = newTable();
	std::set<std::string> addedNames;

	// 添加当前 PC 可见的局部变量；倒序保证 shadow 变量优先
	for (int i = (int)fs->m_localVarDebugInfoVec.size() - 1; i >= 0; i--)
	{
		const LocalVarDebugInfo& info = fs->m_localVarDebugInfoVec[i];
		if (info.localIndex < 0 || info.localIndex >= fs->stackFrameSize)
			continue;
		if (info.name.compare(0, strlen(XSCRIPT_INTERNAL_IDENT_PREFIX), XSCRIPT_INTERNAL_IDENT_PREFIX) == 0)
			continue;
		if (info.startPc > currentPc || (info.endPc >= 0 && currentPc >= info.endPc))
			continue;
		if (addedNames.find(info.name) != addedNames.end())
			continue;

		int stackPos = ci->mFrameIndex + info.localIndex - fs->stackFrameSize;
		Value stackValue = getStackValue(stackPos);
		Value newValue;
		CopyValue(&newValue, stackValue);
		setTableValue(table, ConstructValue(info.name.c_str()), newValue);
		addedNames.insert(info.name);
	}

	// 添加 upvalue
	Function* func = ci->mCurFunction;
	if (func != NULL && !func->isCFunc)
	{
		LuaFunction& luaFunc = func->funcUnion.luaFunc;
		for (int i = 0; i < luaFunc.mNumUpVals; i++)
		{
			UpValue* uv = luaFunc.mUpVals[i];
			if (uv == NULL || uv->pValue == NULL)
				continue;

			if (i >= (int)fs->m_upValueVec.size() || fs->m_upValueVec[i].name.empty())
				continue;

			const std::string& uvName = fs->m_upValueVec[i].name;
			if (uvName.compare(0, strlen(XSCRIPT_INTERNAL_IDENT_PREFIX), XSCRIPT_INTERNAL_IDENT_PREFIX) == 0)
				continue;

			Value newValue;
			CopyValue(&newValue, *uv->pValue);
			setTableValue(table, ConstructValue(uvName.c_str()), newValue);
		}
	}

	return table;
}

// 内部辅助：保存/恢复寄存器并执行编译后的函数
// outResult 在恢复寄存器前从 mRegValue[0] 中取出
int XScriptVM::CallCompiledFunc(FuncState* mainFunc, Value& outResult, std::string& errorDesc)
{
	// 保存返回值寄存器，防止调试中断期间 evaluate 破坏 VM 状态
	// 只在最外层保存，避免嵌套调用时重复覆盖
	if (mSavedRegDepth == 0)
		memcpy(mSavedRegValues, mRegValue, sizeof(mRegValue));
	mSavedRegDepth++;

	Value fValue = ConstructValue(mainFunc);
	// 临时标记为固定，防止 GC 回收（fValue 只在 C++ 栈上，不在 GC 根集中）
	GC_SetFixed(fValue.func);
	int err = ProtectCallFunction(fValue.func, 0, errorDesc);
	// 恢复为可回收，让下次 GC 清理
	GC(fValue.func)->marked = MS_White;

	// 在恢复寄存器前取出返回值
	outResult = mRegValue[0];

	mSavedRegDepth--;
	if (mSavedRegDepth == 0)
		memcpy(mRegValue, mSavedRegValues, sizeof(mRegValue));

	return err;
}

// 编译条件字符串表达式（不执行），返回编译后的 FuncState
// 编译成功后会进行只读检查，如果表达式包含对执行环境的写入操作则返回 NULL
FuncState* XScriptVM::CompileConditionString(const char* condstr)
{
	FuncState* func = CompileString(std::string("return ") + condstr);
	if (func && !IsReadOnlyCode(func))
	{
		// 条件表达式不允许修改执行环境
		snprintf(gLastParseErrorInfo, sizeof(gLastParseErrorInfo), "condition expression must be read-only: cannot modify execution environment");
		return NULL;
	}
	return func;
}

// 执行已编译的条件表达式，返回布尔结果
bool XScriptVM::EvalCompiledCondition(FuncState* compiledFunc)
{
	if (!compiledFunc)
		return false;

	std::string errorDesc;
	Value result;
	int err = CallCompiledFunc(compiledFunc, result, errorDesc);
	return (err == 0) && !XIsValueFalse(result);
}

// 编译并执行条件字符串表达式（一步完成）
bool XScriptVM::EvalConditionString(const char* condstr)
{
	FuncState* mainFunc = CompileConditionString(condstr);
	return EvalCompiledCondition(mainFunc);
}

// 只按表达式求值（自动加 return，不执行语句 fallback）
EvalResult XScriptVM::EvalExpressionString(const std::string& code, Value& outResult, std::string& errorDesc)
{
	FuncState* mainFunc = CompileString(std::string("return ") + code);
	if (!mainFunc)
		return EVAL_COMPILE_ERROR;

	int err = CallCompiledFunc(mainFunc, outResult, errorDesc);
	return (err == 0) ? EVAL_OK : EVAL_RUNTIME_ERROR;
}

// 只读表达式求值（自动加 return，带只读检查）
// 用于 watch/hover 等不允许修改执行环境的场景
EvalResult XScriptVM::EvalReadOnlyExpression(const std::string& code, Value& outResult, std::string& errorDesc)
{
	FuncState* mainFunc = CompileString(std::string("return ") + code);
	if (!mainFunc)
		return EVAL_COMPILE_ERROR;

	if (!IsReadOnlyCode(mainFunc))
	{
		errorDesc = "expression has side effects and is not allowed in read-only context";
		return EVAL_COMPILE_ERROR;
	}

	int err = CallCompiledFunc(mainFunc, outResult, errorDesc);
	return (err == 0) ? EVAL_OK : EVAL_RUNTIME_ERROR;
}

// 按语句块执行（不自动加 return）
EvalResult XScriptVM::ExecString(const std::string& code, Value& outResult, std::string& errorDesc)
{
	FuncState* mainFunc = CompileString(code);
	if (!mainFunc)
		return EVAL_COMPILE_ERROR;

	int err = CallCompiledFunc(mainFunc, outResult, errorDesc);
	return (err == 0) ? EVAL_OK : EVAL_RUNTIME_ERROR;
}

// 计算字符串表达式并返回结果值（调试器用）
EvalResult XScriptVM::EvalString(const std::string& code, Value& outResult, std::string& errorDesc)
{
	// 先尝试作为表达式求值（自动加 return）
	EvalResult result = EvalExpressionString(code, outResult, errorDesc);
	if (result != EVAL_COMPILE_ERROR)
		return result;

	// 表达式编译失败，尝试作为语句块执行
	errorDesc.clear();
	return ExecString(code, outResult, errorDesc);
}

// 判断指令是否为写入类指令（第0个操作数为写入目标）
static bool IsWriteInstr(int opType)
{
	switch (opType)
	{
	case INSTR_MOV:
	case INSTR_ADD_TO:
	case INSTR_SUB_TO:
	case INSTR_MUL_TO:
	case INSTR_DIV_TO:
	case INSTR_MOD_TO:
	case INSTR_EXP_TO:
	case INSTR_AND_TO:
	case INSTR_OR_TO:
	case INSTR_XOR_TO:
	case INSTR_NOT_TO:
	case INSTR_SHL_TO:
	case INSTR_SHR_TO:
	case INSTR_CONCAT_TO:
	case INSTR_INC:
	case INSTR_DEC:
	case INSTR_LOADNIL:
	case INSTR_TYPE:
		return true;
	default:
		return false;
	}
}

// 静态检查编译后的函数是否为只读（不修改执行环境）
// 只读定义：不写入全局变量、不写入表字段、不写入 UpValue
// 宽松策略：函数调用(INSTR_CALL)不检查，由用户自行负责
// 注意：编译器临时寄存器（GetFreeReg）是以函数内部变量形式分配的，
//       其名称以 XSCRIPT_INTERNAL_IDENT_PREFIX 开头，写入这些变量是安全的
bool XScriptVM::IsReadOnlyCode(FuncState* func)
{
	if (!func)
		return true;

	for (int i = 0; i < func->mInstrStream.numInstr; i++)
	{
		Instr& instr = func->mInstrStream.instrs[i];

		// 检查 INSTR_APPEND：修改 list 内容，视为写入
		if (instr.opType == INSTR_APPEND)
			return false;

		// 检查有写入目标的指令
		if (IsWriteInstr(instr.opType) && instr.numOpCount > 0)
		{
			RuntimeOperator& dest = instr.mOpList[0];

			// 写入全局变量（正索引）-> 需进一步判断是否为内部临时变量
			if ((dest.type == ROT_Stack_Index || dest.type == ROT_Table) && dest.iStackIndex >= 0)
			{
				// 内部临时变量（编译器寄存器）允许写入
				return false;
			}

			// 写入表字段 -> 不安全
			if (dest.type == ROT_UpValue_Table)
				return false;

			// 写入 UpValue -> 不安全
			if (dest.type == ROT_UpVal_Index)
				return false;

			// 写入局部变量（负索引）或寄存器 -> 安全，继续
		}
	}

	return true;
}
