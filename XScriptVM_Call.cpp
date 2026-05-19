#include "XScriptVM.h"
#include "XScriptDebugger.h"

#define			Genkey_Thread		"__generator_thread__"
#define			Genkey_ParamNum     "__generator_param_num__"

// 准备Lua可变参数：将多余参数打包为表
bool XScriptVM::PrepareLuaVarArgs(Function* funcValue, int numParam)
{
	FuncState* func = funcValue->funcUnion.luaFunc.proto;
	int fixedParamNum = func->localParamNum - 1;
	int numVarArgs = numParam - fixedParamNum;

	// 参数不足时，补 nil 到固定参数数量
	if (numVarArgs < 0)
	{
		for (int i = numVarArgs; i < 0; i++)
			push(mNilValue);
		numVarArgs = 0;
	}

	TABLE t = newTable();
	setTableValue(t, "n", (XInt)numVarArgs);
	for (XInt i = numVarArgs; i > 0; i--)
	{
		Value value = pop();
		setTableValue(t, ConstructValue(i - 1), value);
	}
	push(ConstructValue(t));
	return true;
}

// 调整调用参数数量：参数不足补nil，参数多余则丢弃
void XScriptVM::AdjustCallArgs(int numParam, int expectedParam)
{
	if (numParam < expectedParam)
	{
		// 参数不足，补 nil
		for (int i = numParam; i < expectedParam; i++)
			push(mNilValue);
	}
	else if (numParam > expectedParam)
	{
		// 参数多余，丢弃
		mCurXScriptState->mTopIndex -= (numParam - expectedParam);
	}
}

// 通用函数调用：根据函数类型分发到Lua函数或宿主函数
// 内部自动保存/恢复 mCurInstr 和跨 C 的 Lua 调用边界，调用方无需关心
void XScriptVM::CallFunction(Function* funcValue, int numParam)
{
	Instr* savedInstr = mCurInstr;
	if (!funcValue->isCFunc)
	{
		int lastCurCCallLuaCallIndex = mCurXScriptState->mCurCCallLuaCallIndex;
		mCurXScriptState->mCurCCallLuaCallIndex = mCurXScriptState->mCurCallIndex;
		mCurXScriptState->mCCallIndex++;
		Function* lastCFunction = mCurXScriptState->mCurrentCFunction;
		mCurXScriptState->mCurrentCFunction = nullptr;
		
		FuncState* func = funcValue->funcUnion.luaFunc.proto;
		if (func->hasVarArgs)
		{
			if (PrepareLuaVarArgs(funcValue, numParam))
			{
				ExecLuaFunction(funcValue, numParam);
			}
		}
		else
		{
			AdjustCallArgs(numParam, func->localParamNum);
			ExecLuaFunction(funcValue, numParam);
		}
		mCurXScriptState->mCurrentCFunction = lastCFunction;
		mCurXScriptState->mCCallIndex--;
		mCurXScriptState->mCurCCallLuaCallIndex = lastCurCCallLuaCallIndex;
	}
	else
	{
		CallHostFunc(funcValue, funcValue->funcUnion.cFunc.pfnAddr, numParam);
	}
	mCurInstr = savedInstr;
}

// 保护模式恢复协程：捕获执行错误并恢复状态
int	XScriptVM::ProtectResume(std::string & errorDesc)
{
	int ccallIndex = mCurXScriptState->mCCallIndex;
	int frameIndex = mCurXScriptState->mFrameIndex;
	int lastCallInfoIndex = mCurXScriptState->mCurCallIndex;
	int lastInstrIndex = -1;
	if (mCurXScriptState->mCurFunctionState != NULL)
		lastInstrIndex = mCurXScriptState->mInstrIndex;
	int lastTopIndex = mCurXScriptState->mTopIndex;
	int lastCurCCallLuaCallIndex = mCurXScriptState->mCurCCallLuaCallIndex;
	//resume 无需设置mCurXScriptState->mLastProtectCallLuaCallIndex， 因为在 pcall时已经保存过了
	XScript_LongJmp jmp;
	jmp.previous = mLongJmp;
	jmp.errorCode = 0;
	jmp.errorFunc = -1;
	mLongJmp = &jmp;

	Instr*  lastInstr = mCurInstr;

	if (setjmp(jmp.j) == 0)
	{
		BeforeExecuteResume();
		ExecuteFunction();
	}

	mLongJmp = jmp.previous;
	mCurXScriptState->mCurCCallLuaCallIndex = lastCurCCallLuaCallIndex;

	if (jmp.errorCode != 0)
	{
		mCurXScriptState->mCurCallIndex = lastCallInfoIndex;
		mCurXScriptState->mCurFunction = mCurXScriptState->mCallInfoBase[lastCallInfoIndex].mCurFunction;
		mCurXScriptState->mCurFunctionState = mCurXScriptState->mCallInfoBase[lastCallInfoIndex].mCurFunctionState;
		if (mCurXScriptState->mCurFunctionState != NULL)
			mCurXScriptState->mInstrIndex = lastInstrIndex;
		mCurXScriptState->mFrameIndex = frameIndex;
		mCurXScriptState->mTopIndex = lastTopIndex;
		mCurXScriptState->mCCallIndex = ccallIndex;
		mCurXScriptState->mCurCCallLuaCallIndex = lastCurCCallLuaCallIndex;
		RemoveStackUpVals(lastTopIndex);

		errorDesc = jmp.mErrorDesc;
	}
	mCurInstr = lastInstr;	// ProtectResume 直接调用 ExecuteFunction，需要自行保护

	return jmp.errorCode;
}

// 保护模式调用函数：使用setjmp/longjmp捕获运行时错误
int	XScriptVM::ProtectCallFunction(Function* firstValue, int numParam, std::string& errorDesc, int errorFunc)
{
	int ccallIndex = mCurXScriptState->mCCallIndex;
	int frameIndex = mCurXScriptState->mFrameIndex;
	int lastCallInfoIndex = mCurXScriptState->mCurCallIndex;
	int lastInstrIndex = -1;
	if (mCurXScriptState->mCurFunctionState != NULL)
		lastInstrIndex = mCurXScriptState->mInstrIndex;

	int lastTopIndex = mCurXScriptState->mTopIndex - numParam;
	Instr* lastInstr = mCurInstr;
	int lastCurCCallLuaCallIndex = mCurXScriptState->mCurCCallLuaCallIndex;

	//设置protect call 函数的 lua Call index, 下次遇到异常处理时需要退栈到这里， 同时要保存上一次 mLastProtectCallLuaCallIndex
	//执行完这次pcall 需要恢复上一次pcall的callindex
	int savedLastProtectCallLuaCallIndex = mCurXScriptState->mLastProtectCallLuaCallIndex;
	mCurXScriptState->mLastProtectCallLuaCallIndex = lastCallInfoIndex;

	XScript_LongJmp jmp;
	jmp.previous = mLongJmp;
	jmp.errorCode = 0;
	jmp.errorFunc = errorFunc;
	mLongJmp = &jmp;

	if (setjmp(jmp.j) == 0)
	{
		CallFunction(firstValue, numParam);
	}

	mLongJmp = jmp.previous;

	if (jmp.errorCode != 0)
	{
		mCurXScriptState->mCurCallIndex = lastCallInfoIndex;
		mCurXScriptState->mCurFunction = mCurXScriptState->mCallInfoBase[lastCallInfoIndex].mCurFunction;
		mCurXScriptState->mCurFunctionState = mCurXScriptState->mCallInfoBase[lastCallInfoIndex].mCurFunctionState;
		if (mCurXScriptState->mCurFunctionState != NULL)
			mCurXScriptState->mInstrIndex = lastInstrIndex;
		mCurXScriptState->mFrameIndex = frameIndex;
		mCurXScriptState->mTopIndex = lastTopIndex;
		mCurXScriptState->mCCallIndex = ccallIndex;
		mCurXScriptState->mCurCCallLuaCallIndex = lastCurCCallLuaCallIndex;
		errorDesc = jmp.mErrorDesc;

		RemoveStackUpVals(lastTopIndex);
		mCurInstr = lastInstr;
	}
	mCurXScriptState->mLastProtectCallLuaCallIndex = savedLastProtectCallLuaCallIndex;
	return jmp.errorCode;
}

// 移除栈顶以上的上值引用：将开放的上值关闭（复制值到上值对象内部）
void XScriptVM::RemoveStackUpVals(int topIndex)
{
	Value* top = &mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex];
	UpValue* nextRefUpVals = mCurXScriptState->mNextRefUpVals;
	UpValue* lastUpVals = NULL;
	while (nextRefUpVals != NULL)
	{
		if (nextRefUpVals->pValue >= top)
		{
			CopyValue(&nextRefUpVals->value, *nextRefUpVals->pValue);
			nextRefUpVals->pValue = &nextRefUpVals->value;

			UpValue* oldValue = nextRefUpVals;
			if (lastUpVals != NULL)
			{
				lastUpVals->nextValue = nextRefUpVals->nextValue;
			}
			else
			{
				mCurXScriptState->mNextRefUpVals = nextRefUpVals->nextValue;
			}

			nextRefUpVals = nextRefUpVals->nextValue;
			oldValue->nextValue = NULL;

		}
		else
		{
			lastUpVals = nextRefUpVals;
			nextRefUpVals = nextRefUpVals->nextValue;
		}
	}
}

// 在Lua中调用函数（与CallFunction类似，但不启动新的执行循环）
void XScriptVM::CallFunctionInLua(Function* funcValue, int numParam)
{
	if (!funcValue->isCFunc)
	{
		FuncState* func = funcValue->funcUnion.luaFunc.proto;
		switch (func->funcType)
		{
		case EFunctionType::Async:
		{
			OnCallAsyncFunction(funcValue, numParam);
		}
		break;
		case EFunctionType::Normal:
		{
			if (func->hasVarArgs)
			{
				if (PrepareLuaVarArgs(funcValue, numParam))
				{
					CallLuaFunction(funcValue, numParam);
				}
			}
			else
			{
				AdjustCallArgs(numParam, func->localParamNum);
				CallLuaFunction(funcValue, numParam);
			}
		}
		break;
		case EFunctionType::Generator:
		{
			OnCallGeneratorFunction(funcValue, numParam);
		}
		break;
		default:
			break;
		}
	}
	else
	{
		CallHostFunc(funcValue, funcValue->funcUnion.cFunc.pfnAddr, numParam);
	}
}

void	XScriptVM::OnCallAsyncFunction(Function* funcValue, int numParam)
{
	XScriptState* asyncState = CreateCoroutie(funcValue, ECoroutineType::AsyncTask);
	asyncState->mAsyncStartParamCount = numParam;
	asyncState->mOwnerTask = CreateFuture();
	asyncState->mOwnerTask->ownerState = asyncState;

	int argStart = mCurXScriptState->mTopIndex - numParam;
	for (int i = 0; i < numParam; i++)
	{
		asyncState->mStackElements[asyncState->mTopIndex++] = mCurXScriptState->mStackElements[argStart + i];
	}
	//堆栈上已经把参数push进来了， 我们接管了函数调用，需要自己处理好退栈，保持堆栈平很
	mCurXScriptState->mTopIndex -= numParam;

	Value taskValue = ConstructValue(asyncState->mOwnerTask);
	mRegValue[0] = taskValue;
	Value emptyValue;
	AsyncReady(asyncState, emptyValue);
}

static void  generator_iterator(XScriptVM* vm)
{
	CheckParam(generator_iterator, 0, generatorTable, OP_TYPE_TABLE);
	vm->setReturnAsValue(generatorTable);
}

static void  generator_next(XScriptVM* vm)
{
	CheckParam(generator_next, 0, generatorTable, OP_TYPE_TABLE);
	vm->CoroutieGeneratorNext(generatorTable);
}

void	XScriptVM::CoroutieGeneratorNext(const Value& generatorTable)
{
	Value CoroutieValue;
	getTableValue(generatorTable.tableData, ConstructValue(Genkey_Thread), CoroutieValue);
	if (!IsValueThread(&CoroutieValue))
	{
		setReturnAsInt(0, 0);
		return;
	}
	XScriptState* threadState = CoroutieValue.threadData;
	if (GetCoroutieStatus(threadState) != CS_Suspend)
	{
		setReturnAsInt(0);
	}
	else
	{

		if (threadState->mStatus == CS_Normal)
		{
			XScriptState* lastState = mCurXScriptState;

			SetCurrentXScriptState(threadState);

			Value NumHostFuncParam;
			if (!getTableValue(generatorTable.tableData, ConstructValue(Genkey_ParamNum), NumHostFuncParam) || !IsValueInt(&NumHostFuncParam))
			{
				SetCurrentXScriptState(lastState);
				setReturnAsInt(0);
				return;
			}

			std::string errorDesc;
			int status = ProtectCallFunction(threadState->mStartFunction, (int)NumHostFuncParam.iIntValue, errorDesc);

			if (status == 0)
			{
				//如果还是继续执行状态，表示是return 返回的，这个结果要丢弃，我只接受yield的值
				if (threadState->mStatus == CS_Normal)
				{
					threadState->mStatus = CS_Dead;
					setReturnAsInt(0, 0);
				}
				else
				{
					memmove(&mRegValue[1], &mRegValue[0], sizeof(Value) * (MAX_FUNC_REG - 1));
					setReturnAsInt(1, 0);
				}
			}
			else
			{
				mCurXScriptState->mStatus = CS_Dead;
				setReturnAsInt(0, 0);
			}

			SetCurrentXScriptState(lastState);
		}
		else if (threadState->mStatus == CS_Suspend)
		{
			XScriptState* lastState = mCurXScriptState;
			SetCurrentXScriptState(threadState);
			mCurXScriptState->mStatus = CS_Normal;

			std::string errorDesc;
			int status = ProtectResume(errorDesc);
			if (status == 0)
			{
				//如果还是继续执行状态，表示是return 返回的，这个结果要丢弃，我只接受yield的值
				if (threadState->mStatus == CS_Normal)
				{
					threadState->mStatus = CS_Dead;
					setReturnAsInt(0, 0);
				}
				else
				{
					memmove(&mRegValue[1], &mRegValue[0], sizeof(Value) * (MAX_FUNC_REG - 1));
					setReturnAsInt(1, 0);
				}
			}
			else
			{
				mCurXScriptState->mStatus = CS_Dead;
				setReturnAsInt(0, 0);
			}

			SetCurrentXScriptState(lastState);
		}
	}
}

void	XScriptVM::OnCallGeneratorFunction(Function* funcValue, int numPara)
{
	TableValue* generatorTable = CreateTable();

	XScriptState* generateState = CreateCoroutie(funcValue, ECoroutineType::Generator);
	setTableValue(generatorTable, ConstructValue(Genkey_ParamNum), ConstructValue((XInt)numPara));
	setTableValue(generatorTable, ConstructValue(Genkey_Thread), ConstructValue(generateState));
	setTableValue(generatorTable, ConstructValue(ITER_FUNC), CreateCHostFunction(generator_iterator, 0));
	setTableValue(generatorTable, ConstructValue(ITER_NEXT), CreateCHostFunction(generator_next, 0));


	int argStart = mCurXScriptState->mTopIndex - numPara;
	for (int i = 0; i < numPara; i++)
	{
		generateState->mStackElements[generateState->mTopIndex++] = mCurXScriptState->mStackElements[argStart + i];
	}
	//堆栈上已经把参数push进来了， 我们接管了函数调用，需要自己处理好退栈，保持堆栈平很
	mCurXScriptState->mTopIndex -= numPara;

	mRegValue[0] = ConstructValue(generatorTable);
}


// 纯净版抛出运行时错误,不再退栈处理，以及不再执行error函数，仅执行long jump
void	XScriptVM::ThrowErrorDirectly(const char* errorStr, ...)
{
	if (mLongJmp != NULL)
	{
		char  buffer[512] = { 0 };
		va_list  args;
		va_start(args, errorStr);
		int len = vsnprintf(buffer, 512, errorStr, args);
		va_end(args);
		if (len < 0)
		{
			buffer[0] = '\0';
		}
		else if (len >= sizeof(buffer))
		{
			// 说明被截断了
			buffer[sizeof(buffer) - 1] = '\0';
		}

		char buffer2[1200] = { 0 };
		snprintf(buffer2, 1200, "Runtime Error: %s\n%s", buffer, stackBackTrace().c_str());
		mLongJmp->mErrorDesc = buffer2;
		mLongJmp->errorCode = -1;
		longjmp(mLongJmp->j, 1);
	}

}

// 抛出运行时错误：格式化错误信息并通过longjmp跳转到保护点
void  XScriptVM::ExecError(const char* errorStr, ...)
{
	if (mLongJmp != NULL)
	{
		char  buffer[1024] = { 0 };
		va_list  args;
		va_start(args, errorStr);
		int len = vsnprintf(buffer, 1024, errorStr, args);
		va_end(args);
		if (len < 0)
		{
			buffer[0] = '\0';
		}
		else if (len >= sizeof(buffer))
		{
			// 说明被截断了
			buffer[sizeof(buffer) - 1] = '\0';
		}

		char buffer2[1200] = { 0 };
		snprintf(buffer2, 1200, "Runtime Error: %s\n%s", buffer, stackBackTrace().c_str());

		if (mDebugger != NULL && mDebugger->IsEnabled())
		{
			int lineIndex = mCurInstr ? mCurInstr->lineIndex:-1;
			mDebugger->OnException(buffer2, mCurXScriptState->mCurFunctionState->sourceFileName, lineIndex);
		}

		mLongJmp->errorCode = -1;

		std::string deferErrorDesc;
		//依次退栈到pcall调用点，这样才有机会调用defer函数，清理堆栈空间
		for (int i = mCurXScriptState->mCurCallIndex; i > mCurXScriptState->mLastProtectCallLuaCallIndex; i--)
		{
			std::vector<DeferErrorInfo> deferErrorVec;
			ReturnCurFunction(EFunctionReturnType::Exeception, &deferErrorVec);
			if (deferErrorVec.size() > 0)
			{
				deferErrorDesc += GetDeferErrorDesc(&deferErrorVec) + "\r\n";
			}
		}

		std::string finalErrorDesc = std::string(buffer2) + deferErrorDesc;

		std::string erorFuncErrorDesc;
		if (mLongJmp->errorFunc >= 0)
		{
			Value errValue = getStackValue(mLongJmp->errorFunc);
			if (errValue.type == OP_TYPE_FUNC)
			{
				push(ConstructValue((XInt)mLongJmp->errorCode));
				push(ConstructValue(finalErrorDesc.c_str()));
				std::string errorDesc;
				int errorCode = ProtectCallFunction(errValue.func, 2, errorDesc);
				if (errorCode != 0)
				{
					erorFuncErrorDesc = std::string("\r\nError function execute failed, ") + errorDesc;
				}
			}
		}


		mLongJmp->mErrorDesc = finalErrorDesc + erorFuncErrorDesc;

		longjmp(mLongJmp->j, 1);
	}
	
	
}

// 执行Lua函数：设置栈帧并启动执行循环
void	XScriptVM::ExecLuaFunction(Function* func, int numActualArgs)
{
	CallLuaFunction(func, numActualArgs);
	ExecuteFunction();
}

// 调用Lua函数：设置栈帧、初始化局部变量、保存调用信息
void	XScriptVM::CallLuaFunction(Function* func, int numActualArgs)
{
	if (mCurXScriptState->mCurCallIndex >= MAX_LUA_CALL_STACK_DEPTH - 1)
	{
		ExecError("stack overflow: call stack depth exceeds maximum (%d)", MAX_LUA_CALL_STACK_DEPTH);
	}

	int lastInstrIndex = -1;
	if (mCurXScriptState->mCurFunctionState != NULL)
		lastInstrIndex = mCurXScriptState->mInstrIndex;

	mCurXScriptState->mCurFunction = func;
	mCurXScriptState->mCurFunctionState = func->funcUnion.luaFunc.proto;

	if (mCurXScriptState->mTopIndex + MAX_PARAM_NUM + mCurXScriptState->mCurFunctionState->localDataSize > mCurXScriptState->mStackSize)
	{
		int growSize = mCurXScriptState->mTopIndex + MAX_PARAM_NUM + mCurXScriptState->mCurFunctionState->localDataSize - mCurXScriptState->mStackSize;
		GrowStack(mCurXScriptState, growSize);
	}

	for (int i = 0; i < mCurXScriptState->mCurFunctionState->localDataSize; i++)
	{
		mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex + i].type = OP_TYPE_NIL;
	}

	resetReturnValue();

	mCurXScriptState->mTopIndex += mCurXScriptState->mCurFunctionState->localDataSize;
	mCurXScriptState->mFrameIndex = mCurXScriptState->mTopIndex;
	mCurXScriptState->mInstrIndex = 0;
	mCurXScriptState->mCurCallIndex++;

	auto& callInfo = mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex];
	callInfo.mCurFunction = mCurXScriptState->mCurFunction;
	callInfo.mCurFunctionState = mCurXScriptState->mCurFunctionState;
	if (mCurInstr != NULL)
	{
		callInfo.mInstrIndex = lastInstrIndex;
		callInfo.mCurLine = mCurInstr->lineIndex;
	}
	callInfo.mFrameIndex = mCurXScriptState->mFrameIndex;
	callInfo.mCurTryCatchIndex = -1;
	callInfo.mNumActualArgs = numActualArgs;
	callInfo.mDeferFuncVec.clear();
}

// 调用宿主函数：执行C函数并清理参数栈
void  XScriptVM::CallHostFunc(Function* func, HOST_FUNC pfnAddr, int numParam)
{
	resetReturnValue();
	int lastParamNum = mNumHostFuncParam;
	mNumHostFuncParam = numParam;
	mCurXScriptState->mCCallIndex++;
	int lastCurCCallLuaCallIndex = mCurXScriptState->mCurCCallLuaCallIndex;
	mCurXScriptState->mCurCCallLuaCallIndex = mCurXScriptState->mCurCallIndex;
	Function* lastCFunction = mCurXScriptState->mCurrentCFunction;
	mCurXScriptState->mCurrentCFunction = func;

	if (pfnAddr != NULL)
	{
		(*pfnAddr)(this);
	}

	mCurXScriptState->mCurrentCFunction = lastCFunction;
	mCurXScriptState->mCurCCallLuaCallIndex = lastCurCCallLuaCallIndex;
	mCurXScriptState->mCCallIndex--;
	mNumHostFuncParam = lastParamNum;
	popFrame(numParam);
}

// 通过元方法执行运算（支持算术、比较、拼接等）
bool	XScriptVM::CalByTagMethod(Value* result, Value* value1, Value* value2, MetaMethodType mmtType)
{
	int numParam = 2;
	Value tagMethod;
	if (mmtType == MMT_Add || mmtType == MMT_Sub || mmtType == MMT_Mul || mmtType == MMT_Div 
		|| mmtType == MMT_Mod || mmtType == MMT_Pow || mmtType == MMT_Concat)
	{
		tagMethod = GetMetaMethod(value1, mmtType);
	}
	else if (mmtType == NMT_Less || mmtType == NMT_LessEqual ||
		mmtType == MMT_Great || mmtType == MMT_GreatEqual ||
		mmtType == MMT_Equal)
	{
		if (mmtType == MMT_Great || mmtType == MMT_GreatEqual)
		{
			Value* tmp = value1;
			value1 = value2;
			value2 = tmp;
			if (mmtType == MMT_Great)
				mmtType = NMT_Less;
			else
				mmtType = NMT_LessEqual;
		}

		Value tagMethod1 = GetMetaMethod(value1, mmtType);
		Value tagMethod2 = GetMetaMethod(value2, mmtType);

		if (!IsValueNil(&tagMethod1) && !IsValueNil(&tagMethod2) && isValueEqual(tagMethod1, tagMethod2))
		{
			tagMethod = tagMethod1;
		}
	}
	else if (mmtType == MMT_Neg || mmtType == MMT_Len || mmtType == MMT_ToString)
	{
		tagMethod = GetMetaMethod(value1, mmtType);
		numParam = 1;
	}
	

	if (!IsValueFunction(&tagMethod))
		return false;

	push(*value1);
	if (numParam > 1)
		push(*value2);

	CallFunction(tagMethod.func, numParam);
	*result = mRegValue[0];
	return true;
}
