#include "XScriptVM.h"


void XScriptVM::ExecInstr_JMP()
{
	ResolveOpPointer(0, destValue);
	if (destValue.type == ROT_Instr_Index)
	{
		mCurXScriptState->mInstrIndex = destValue.iInstrIndex;
	}
	else
	{
		ExecError("internal error: operand is not an instruction index");
	}
}

void XScriptVM::ExecInstr_Concat()
{
	ResolveOpPointer(0, firstValue);
	ResolveOpPointer(1, secondValue);

	if (firstValue.type != OP_TYPE_STRING && !IsPNumberType(firstValue))
	{
		if (CalByTagMethod(&firstValue, &firstValue, &secondValue, MMT_Concat))
		{
			SetOpValue(0, firstValue);
			return;
		}
	}

	if (firstValue.type != OP_TYPE_STRING && !IsPNumberType(firstValue))
	{
		if (!CalByTagMethod(&firstValue, &firstValue, nullptr, MMT_ToString))
		{
			ExecError("attempt to perform '$' operator on a non-string-convertible value (type: %s)", getTypeName(firstValue.type));
		}
	}

	if (secondValue.type != OP_TYPE_STRING && !IsPNumberType(secondValue))
	{
		if (!CalByTagMethod(&secondValue, &secondValue, nullptr, MMT_ToString))
		{
			ExecError("attempt to perform '$' operator on a non-string-convertible value (type: %s)", getTypeName(secondValue.type));
		}
	}


	Value resultValue;
	if (firstValue.type == OP_TYPE_STRING || IsPNumberType(firstValue))
	{
		//目标操作数如果是数字类型 需要先转换为字符串
		if (IsPNumberType(firstValue))
		{
			char numberBuffer[64] = { 0 };

			if (IsValueInt(&firstValue))
			{
				snprintf(numberBuffer, MAX_NUMBER_STRING_SIZE, XIntConFmt, firstValue.iIntValue);
			}
			else
			{
				snprintf(numberBuffer, MAX_NUMBER_STRING_SIZE, XFloatConFmt, firstValue.fFloatValue);
			}

			firstValue.type = OP_TYPE_STRING;
			firstValue.stringValue = NewXString(numberBuffer);
		}

		if (secondValue.type == OP_TYPE_STRING)
		{
			int strLength = stringRawLen(&firstValue) + stringRawLen(&secondValue);
			CheckStrBuffer(strLength);

			memcpy(mStrBuffer, stringRawValue(&firstValue), stringRawLen(&firstValue));
			memcpy(mStrBuffer + stringRawLen(&firstValue), stringRawValue(&secondValue), stringRawLen(&secondValue));
			resultValue.type = OP_TYPE_STRING;
			resultValue.stringValue = NewXString(mStrBuffer, strLength);
		}
		else if (IsPNumberType(secondValue))
		{
			char numberBuffer[64] = { 0 };

			if (IsValueInt(&secondValue))
			{
				snprintf(numberBuffer, MAX_NUMBER_STRING_SIZE, XIntConFmt, secondValue.iIntValue);
			}
			else
			{
				snprintf(numberBuffer, MAX_NUMBER_STRING_SIZE, XFloatConFmt, secondValue.fFloatValue);
			}

			int strLength = (int)(stringRawLen(&firstValue) + strlen(numberBuffer));
			CheckStrBuffer(strLength);

			memcpy(mStrBuffer, stringRawValue(&firstValue), stringRawLen(&firstValue));
			memcpy(mStrBuffer + stringRawLen(&firstValue), numberBuffer, strlen(numberBuffer));
			resultValue.type = OP_TYPE_STRING;
			resultValue.stringValue = NewXString(mStrBuffer, strLength);
		}
	}
	
	if (IsValueNil(&resultValue))
	{
		if (!CalByTagMethod(&resultValue, &firstValue, &secondValue, MMT_Concat))
		{	
			EXEC_OP_ERROR($, 0, 1);
		}	
	}

	SetOpValue(0, resultValue);
}


// 执行字符串拼接赋值指令
void XScriptVM::ExecInstr_Concat_To()
{
	Value destValue;
	ResolveOpPointer(1, firstValue);
	ResolveOpPointer(2, secondValue);

	if (firstValue.type != OP_TYPE_STRING && !IsPNumberType(firstValue))
	{
		if (CalByTagMethod(&destValue, &firstValue, &secondValue, MMT_Concat))
		{
			SetOpValue(0, destValue);
			return;
		}
	}

	if (firstValue.type != OP_TYPE_STRING && !IsPNumberType(firstValue))
	{
		if (!CalByTagMethod(&firstValue, &firstValue, nullptr, MMT_ToString))
		{
			ExecError("attempt to perform '$' operator on a non-string-convertible value (type: %s)", getTypeName(firstValue.type));
		}
	}

	if (secondValue.type != OP_TYPE_STRING && !IsPNumberType(secondValue))
	{
		if (!CalByTagMethod(&secondValue, &secondValue, nullptr, MMT_ToString))
		{
			ExecError("attempt to perform '$' operator on a non-string-convertible value (type: %s)", getTypeName(secondValue.type));
		}
	}

	//目标操作数如果是数字类型 需要先转换为字符串
	if (IsPNumberType(firstValue))
	{
		char numberBuffer[64] = { 0 };

		if (IsValueInt(&firstValue))
		{
			snprintf(numberBuffer, MAX_NUMBER_STRING_SIZE, XIntConFmt, firstValue.iIntValue);
		}
		else
		{
			snprintf(numberBuffer, MAX_NUMBER_STRING_SIZE, XFloatConFmt, firstValue.fFloatValue);
		}

		firstValue.type = OP_TYPE_STRING;
		firstValue.stringValue = NewXString(numberBuffer);
	}

	if (secondValue.type == OP_TYPE_STRING)
	{
		int strLength = stringRawLen(&firstValue) + stringRawLen(&secondValue);
		CheckStrBuffer(strLength);

		memcpy(mStrBuffer, stringRawValue(&firstValue), stringRawLen(&firstValue));
		memcpy(mStrBuffer + stringRawLen(&firstValue), stringRawValue(&secondValue), stringRawLen(&secondValue));

		destValue.type = OP_TYPE_STRING;
		destValue.stringValue = NewXString(mStrBuffer, strLength);
	}
	else if (IsPNumberType(secondValue))
	{
		char numberBuffer[64] = { 0 };

		if (IsValueInt(&secondValue))
		{
			snprintf(numberBuffer, MAX_NUMBER_STRING_SIZE, XIntConFmt, secondValue.iIntValue);
		}
		else
		{
			snprintf(numberBuffer, MAX_NUMBER_STRING_SIZE, XFloatConFmt, secondValue.fFloatValue);
		}

		int strLength = (int)(stringRawLen(&firstValue) + strlen(numberBuffer));
		CheckStrBuffer(strLength);

		memcpy(mStrBuffer, stringRawValue(&firstValue), stringRawLen(&firstValue));
		memcpy(mStrBuffer + stringRawLen(&firstValue), numberBuffer, strlen(numberBuffer));

		destValue.type = OP_TYPE_STRING;
		destValue.stringValue = NewXString(mStrBuffer, strLength);
	}


	SetOpValue(0, destValue);
}



void XScriptVM::ExecInstr_Await()
{
	if (mCurXScriptState->mCoroutineType != ECoroutineType::AsyncTask)
	{
		ExecError("await can only be used in async function");
	}

	if (mCurXScriptState->mCurCCallLuaCallIndex >= 0 && mCurXScriptState->mCurCallIndex > mCurXScriptState->mCurCCallLuaCallIndex)
	{
		ExecError("cannot await across C-call boundary");
	}

	ResolveOpPointer(0, firstValue);
	if (!IsValueFuture(&firstValue))
	{
		ExecError("attempt to await a non-future value");
	}

	XFuture* future = firstValue.futureData;
	if (future == NULL)
	{
		ExecError("attempt to await a null future");
	}

	if (future->IsDone())
	{
		mRegValue[0] = future->result;
		return;
	}
	else if(future->IsRejected())
	{
		ExecInstr_Throw(future->result);
		return;
	}

	mCurXScriptState->mStatus = CS_Suspend;
	mCurXScriptState->mSuspendReason = SUSPEND_AWAIT;
	mCurXScriptState->mWaitingFuture = future;
	future->AddWaiter(mCurXScriptState);
}

void XScriptVM::ExecInstr_RET()
{
	Value asyncReturnValue = mRegValue[0];
	if (mAllowHook && (mHookMask & MASK_HOOKCALL))
	{
		CallHookFunction(HE_HookRet, -1);
	}

	bool resolveAsyncTask = mCurXScriptState->mCoroutineType == ECoroutineType::AsyncTask && mCurXScriptState->mCurCallIndex == 0 && mCurXScriptState->mOwnerTask != NULL;

	ReturnCurFunction(EFunctionReturnType::Normal);

	if (resolveAsyncTask)
	{
		ResolveFuture(mCurXScriptState->mOwnerTask, asyncReturnValue);
	}
}

std::string		XScriptVM::GetDeferErrorDesc(std::vector<DeferErrorInfo>* deferErrorVec)
{
	std::string deferErrorDesc = "defer failed during function return.\r\nDefer errors:";
	if (deferErrorVec)
	{
		for (auto& d : *deferErrorVec)
		{
			deferErrorDesc += std::string("[Defer #") + std::to_string(d.deferIndex) +std::string("]") + d.errorDesc + "\r\n";
		}
	}
	return deferErrorDesc;

}

void	XScriptVM::ReturnCurFunction(EFunctionReturnType returnType, std::vector<DeferErrorInfo>* deferErrorVec)
{
	std::string defferErrorDesc;
	bool hasNormalReturnDeferError = false;
	const CallInfo& lastCallInfo = mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex];
	//如果当前函数有defer 语句块，需要先一次执行defer 函数，从后往前执行
	if (lastCallInfo.mDeferFuncVec.size() > 0)
	{
		//执行defer前需要先保存 函数返回值， 因为执行到这里时 
		int startIndex = (int)mSavedRegValueStack.size();
		for (size_t i = 0; i < MAX_FUNC_REG; i++)
		{
			mSavedRegValueStack.push_back(mRegValue[i]);
		}

		std::vector<DeferErrorInfo> localDeferErrorVec;

		//需要保护执行defer 函数否则遇到exec error 会直接跳过后续的defer函数
		for (int i = (int)lastCallInfo.mDeferFuncVec.size() - 1; i >= 0; i--)
		{
			std::string errorDesc;
			int errorCode = ProtectCallFunction(lastCallInfo.mDeferFuncVec[i], 0, errorDesc);
			if (errorCode != 0)
			{
				DeferErrorInfo errorInfo;
				errorInfo.callIndex = mCurXScriptState->mCurCallIndex;
				errorInfo.deferIndex = i;
				errorInfo.errorDesc = errorDesc;
				localDeferErrorVec.push_back(errorInfo);
			}
		}

		if (deferErrorVec)
		{
			*deferErrorVec = localDeferErrorVec;
		}

		if ((int)mSavedRegValueStack.size() < startIndex + MAX_FUNC_REG)
		{
			ThrowErrorDirectly("Execute defer function internal error, saved reg is invalid!");
		}
		else
		{
			for (size_t i = 0; i < MAX_FUNC_REG; i++)
			{
				mRegValue[i] = mSavedRegValueStack[startIndex + i];
			}
			mSavedRegValueStack.resize(startIndex);
		}

		if (returnType == EFunctionReturnType::Normal && localDeferErrorVec.size() > 0)
		{
			//正常的退出遇到 defer error 不能直接 ExecError， 因为这样会导致死循环， 也不能直接调用ThrowErrorDirectly， 因为这样会导致父函数的defer函数不再调用，
			//所以只能先退栈，然后继续调用ExecError， 这样可以避免再次对当前函数调用退栈
			hasNormalReturnDeferError = true;
			defferErrorDesc = GetDeferErrorDesc(&localDeferErrorVec);
		}
	}


	int lastInstrIndex = lastCallInfo.mInstrIndex;
	mCurXScriptState->mCurCallIndex--;

	//如果是异常退栈，我们需要跳过hostfunction 的调用堆栈，因此我们需要直接设置上一个函数的frameindex 为堆栈的topindex，
	//普通退栈不可以这样，因为普通退栈c hostfunction 还会退自己的调用栈, 其实throw类型也有这个问题， 但是因为我们设置throw不能跨C调用， 因此避免了这个问题
	if (returnType == EFunctionReturnType::Exeception)
	{
		if (mCurXScriptState->mCurCallIndex >= 0)
		{
			const auto& callInfo = mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex];
			mCurXScriptState->mTopIndex = callInfo.mFrameIndex;
		}
		else
		{
			mCurXScriptState->mTopIndex = 0;
		}
	}
	else
	{
		mCurXScriptState->mTopIndex -= (mCurXScriptState->mCurFunctionState->stackFrameSize);
	}

	RemoveStackUpVals(mCurXScriptState->mTopIndex);

	if (mCurXScriptState->mCurCallIndex >= 0)
	{
		const auto& callInfo = mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex];
		mCurXScriptState->mCurFunction = callInfo.mCurFunction;
		mCurXScriptState->mCurFunctionState = callInfo.mCurFunctionState;
		if (mCurXScriptState->mCurFunctionState != NULL)
			mCurXScriptState->mInstrIndex = lastInstrIndex;
		mCurXScriptState->mFrameIndex = callInfo.mFrameIndex;
	}
	else
	{
		mCurXScriptState->mCurFunction = NULL;
		mCurXScriptState->mCurFunctionState = NULL;
		mCurXScriptState->mFrameIndex = 0;
	}

	if (hasNormalReturnDeferError)
	{
		ExecError("%s", defferErrorDesc.c_str());
	}
}


void XScriptVM::ExecInstr_Neg()
{
	ResolveOpPointer(1, secondValue);
	if (secondValue.type == OP_TYPE_INT)
	{
		SetOpValue(0, ConstructValue((XInt)-secondValue.iIntValue));
	}
	else if (secondValue.type == OP_TYPE_FLOAT)
	{
		SetOpValue(0, ConstructValue((XFloat)-secondValue.fFloatValue));
	}
	else 
	{
		Value resultValue;	
		if(CalByTagMethod(&resultValue, &secondValue, NULL, MMT_Neg))
		{
			SetOpValue(0, resultValue);
		}
		else
		{
			ExecError("attempt to perform unary '-' on a non-numeric value (type: %s)", GetOperatorName(1).c_str());
		}
	}
}

void XScriptVM::ExecInstr_Logic_Not()
{
	XInt iRet = 0;
	ResolveOpPointer(1, secondValue);
	if (secondValue.type == OP_TYPE_INT)
	{
		iRet = (secondValue.iIntValue == 0) ? 1 : 0;
	}
	else if (secondValue.type == OP_TYPE_FLOAT)
	{
		iRet = (secondValue.fFloatValue == 0.0f) ? 1 : 0;
	}
	else
	{
		iRet = (secondValue.type == OP_TYPE_NIL) ? 1 : 0;
	}
	SetOpValue(0, ConstructValue(iRet));
}



void XScriptVM::ExecInstr_Logic_And()
{
	ResolveOpPointer(1, firstValue);
	ResolveOpPointer(2, secondValue);
	XInt value = 0;
	if (firstValue.type == OP_TYPE_INT)
	{
		value = ((firstValue.iIntValue != 0) ? 1 : 0);
	}
	else if (firstValue.type == OP_TYPE_FLOAT)
	{
		value = ((firstValue.fFloatValue != 0) ? 1 : 0);
	}
	else if (IsUserType(firstValue.type))
	{
		value = ((firstValue.lightUserData != NULL) ? 1 : 0);
	}
	else
	{
		ExecError("attempt to perform '&&' operator on a non-boolean-convertible value (type: %s)", getTypeName(firstValue.type));
	}

	if (value == 1)
	{
		if (secondValue.type == OP_TYPE_INT)
		{
			value = ((secondValue.iIntValue != 0) ? 1 : 0);
		}
		else if (secondValue.type == OP_TYPE_FLOAT)
		{
			value = ((secondValue.fFloatValue != 0) ? 1 : 0);
		}
		else if (IsUserType(secondValue.type))
		{
			value = ((secondValue.lightUserData != NULL) ? 1 : 0);
		}
		else
		{
			ExecError("attempt to perform '&&' operator on a non-boolean-convertible value (type: %s)", getTypeName(secondValue.type));
		}
	}
	SetOpValue(0, ConstructValue(value));
}

void XScriptVM::ExecInstr_Logic_Or()
{
	ResolveOpPointer(1, firstValue);
	ResolveOpPointer(2, secondValue);
	XInt value = 0;
	if (firstValue.type == OP_TYPE_INT)
	{
		value = ((firstValue.iIntValue != 0) ? 1 : 0);
	}
	else if (firstValue.type == OP_TYPE_FLOAT)
	{
		value = ((firstValue.fFloatValue != 0) ? 1 : 0);
	}
	else if (IsUserType(firstValue.type))
	{
		value = ((firstValue.lightUserData != NULL) ? 1 : 0);
	}
	else
	{
		ExecError("attempt to perform '||' operator on a non-boolean-convertible value (type: %s)", GetOperatorName(1).c_str());
	}

	if (value == 0)
	{
		if (secondValue.type == OP_TYPE_INT)
		{
			value = ((secondValue.iIntValue != 0) ? 1 : 0);
		}
		else if (secondValue.type == OP_TYPE_FLOAT)
		{
			value = ((secondValue.fFloatValue != 0) ? 1 : 0);
		}
		else if (IsUserType(secondValue.type))
		{
			value = ((secondValue.lightUserData != NULL) ? 1 : 0);
		}
		else
		{
			ExecError("attempt to perform '||' operator on a non-boolean-convertible value (type: %s)", GetOperatorName(2).c_str());
		}
	}

	SetOpValue(0, ConstructValue(value));
}
