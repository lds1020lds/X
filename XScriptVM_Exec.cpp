#include "XScriptVM.h"
#include "XScriptDebugger.h"

// 指令执行主循环：逐条取指令并执行，处理所有指令类型
void  XScriptVM::ExecuteFunction()
{
	int lineIndex = -1;
	while (true)
	{
#ifdef INSTR_PC_ADD_NEW
		mCurInstr = &mCurXScriptState->mCurFunctionState->mInstrStream.instrs[mCurXScriptState->mInstrIndex];
		mCurXScriptState->mInstrIndex++;
#else
		int lastInstrIndex = mCurXScriptState->mInstrIndex;
		mCurInstr = &mCurXScriptState->mCurFunctionState->mInstrStream.instrs[lastInstrIndex];
		int lastCallIndex = mCurXScriptState->mCurCallIndex;
		// 		if (lineIndex == -1 && mAllowHook && (mHookMask & MASK_HOOKCALL))
		// 		{
		// 			CallHookFunction(HE_HookCall, lineIndex);
		// 		}

		if (mCurInstr->lineIndex != lineIndex && mCurInstr->lineIndex >= 0)
		{
			lineIndex = mCurInstr->lineIndex;

			if (mDebugger != NULL && mDebugger->IsEnabled())
			{
				mDebugger->OnLine(mCurXScriptState->mCurFunctionState->sourceFileHash, mCurXScriptState->mCurFunctionState->sourceFileName, lineIndex, GetStackDepth());
			}

			if (mAllowHook && (mHookMask & MASK_HOOKLINE))
			{
				CallHookFunction(HE_HookLine, lineIndex);
			}
		}
#endif
		
		switch (mCurInstr->opType)
		{
		case INSTR_TYPE:
		{
			ResolveOpPointer(1, secondValue);
			if (secondValue.iIntValue == OP_TYPE_TABLE)
				SetOpValue(0, ConstructValue(CreateTable()));
			else if (secondValue.iIntValue == OP_TYPE_LIST)
				SetOpValue(0, ConstructValue(CreateList()));
			break;
		}
		case INSTR_LOADNIL:
		{
			SetOpValue(0, mNilValue);
		}
		break;
		case INSTR_MOV:
		{
			ResolveOpPointer(1, srcValue);
			SetOpValue(0, srcValue);
			break;
		}
		case INSTR_ADD_TO:
		{
			EXEC_INSTR_MATH_TO(+, MMT_Add, _NO_CHECK);
			//ExecInstr_AddTo();
		}
		break;
		case INSTR_ADD:
		{
			EXEC_INSTR_MATH(+, MMT_Add, _NO_CHECK);
			//ExecInstr_Add();
			break;
		}
		case INSTR_SUB_TO:
		{
			EXEC_INSTR_MATH_TO(-, MMT_Sub, _NO_CHECK);
			//ExecInstr_SubTo();
		}
		break;
		case INSTR_SUB:
		{
			EXEC_INSTR_MATH(-, MMT_Sub, _NO_CHECK);
			//ExecInstr_Sub();
			break;
		}
		case INSTR_MUL_TO:
		{
			EXEC_INSTR_MATH_TO(*, MMT_Mul, _NO_CHECK);
			//ExecInstr_MULTO();
		}
		break;
		case INSTR_MUL:
		{
			EXEC_INSTR_MATH(*, MMT_Mul, _NO_CHECK);
			//ExecInstr_Mul();
			break;
		}
		case INSTR_DIV_TO:
		{
			EXEC_INSTR_MATH_TO(/ , MMT_Div, _CHECK_DIVISOR_ZERO);
			//ExecInstr_DIVTO();
		}
		break;
		case INSTR_DIV:
		{
			EXEC_INSTR_MATH(/ , MMT_Div, _CHECK_DIVISOR_ZERO);
			//ExecInstr_Div();
			break;
		}
		case INSTR_MOD_TO:
		{
			EXEC_INSTR_MATH_INTOP_TO(XFMod, %, MMT_Mod, _CHECK_DIVISOR_ZERO);
			//ExecInstr_MODTO();
		}
		break;
		case INSTR_MOD:
		{
			EXEC_INSTR_MATH_INTOP(XFMod, %, MMT_Mod, _CHECK_DIVISOR_ZERO);
			//ExecInstr_Mod();
			break;
		}
		case INSTR_EXP_TO:
		{
			EXEC_INSTR_MATH_OP_TO(pow, MMT_Pow);
			//ExecInstr_ExpTo();
		}
		break;
		case INSTR_EXP:
		{
			EXEC_INSTR_MATH_OP(pow, MMT_Pow);
			//ExecInstr_Pow();
			break;
		}
		case INSTR_NEG:
		{
			ExecInstr_Neg();
			break;
		}
		case INSTR_INC:
		{
			EXEC_INSTR_MATH_INC(+);
			//ExecInstr_Inc();
			break;
		}
		case INSTR_DEC:
		{
			EXEC_INSTR_MATH_INC(-);
			//ExecInstr_Dec();
			break;
		}
		case INSTR_AND_TO:
		{
			EXEC_INSTR_LOGIC_OP_TO(&);
			//ExecInstr_AndTO();
		}
		break;
		case INSTR_AND:
		{
			EXEC_INSTR_LOGIC_OP(&);
			//ExecInstr_And();
			break;
		}
		case INSTR_OR_TO:
		{
			EXEC_INSTR_LOGIC_OP_TO(| );
			//ExecInstr_ORTO();
		}
		break;
		case INSTR_OR:
		{
			EXEC_INSTR_LOGIC_OP(| );
			//ExecInstr_Or();
			break;
		}
		case INSTR_XOR_TO:
		{
			EXEC_INSTR_LOGIC_OP_TO(^);
			//ExecInstr_XORTO();
		}
		break;
		case INSTR_XOR:
		{
			EXEC_INSTR_LOGIC_OP(^);
			//ExecInstr_Xor();
			break;
		}
		case INSTR_SHL_TO:
		{
			EXEC_INSTR_BIT_OP_TO(<< );
			//ExecInstr_SHLTO();
		}
		break;
		case INSTR_SHL:
		{
			EXEC_INSTR_BIT_OP(<< );
			//ExecInstr_SHL();
			break;
		}
		case INSTR_SHR_TO:
		{
			EXEC_INSTR_BIT_OP_TO(>> );
			//ExecInstr_SHRTO();
		}
		break;
		case INSTR_SHR:
		{
			EXEC_INSTR_BIT_OP(>> );
			//ExecInstr_SHR();
			break;
		}
		case INSTR_JMP:
		{
			//ResolveOpPointer(0, destValue);
			mCurXScriptState->mInstrIndex = mCurInstr->mOpList[0].iInstrIndex;
			break;
		}
		case INSTR_JE:
		{
			EXEC_INSTR_JE(==);

			//ExecInstr_JE();
			break;
		}
		case INSTR_JNE:
		{
			EXEC_INSTR_JE(!=);
			//ExecInstr_JNE();
			break;
		}
		case INSTR_JG:
		{
			EXEC_INSTR_J(>, MMT_Great);
			//ExecInstr_JG();
			break;
		}
		case INSTR_JGE:
		{
			EXEC_INSTR_J(>=, MMT_GreatEqual);
			//ExecInstr_JGE();
			break;
		}
		case INSTR_JL:
		{
			EXEC_INSTR_J(<, NMT_Less);
			break;
		}
		case INSTR_JLE:
		{
			EXEC_INSTR_J(<=, NMT_LessEqual);
			//ExecInstr_JLE();
			break;
		}
		case INSTR_JFALSE:
		{
			ResolveOpPointer(0, firstValue);
			if (XIsValueFalse(firstValue))
				mCurXScriptState->mInstrIndex = mCurInstr->mOpList[1].iInstrIndex;
			break;
		}
		case INSTR_JTRUE:
		{
			ResolveOpPointer(0, firstValue);
			if (!XIsValueFalse(firstValue))
				mCurXScriptState->mInstrIndex = mCurInstr->mOpList[1].iInstrIndex;
			break;
		}
		case INSTR_PUSH:
		{
			ResolveOpPointer(0, firstValue);
			mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex] = firstValue;
			mCurXScriptState->mTopIndex++;
			break;
		}
		case INSTR_CONCAT_TO:
		{
			ExecInstr_Concat_To();
		}
		break;
		case INSTR_CONCAT:
		{
			ExecInstr_Concat();
			break;
		}
		case INSTR_POP:
		{
			//ResolveOpPointer(0, firstValue);
			mCurXScriptState->mTopIndex--;
			SetOpValue(0, mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex]);
			break;
		}
		case INSTR_CALL:
		{
			ResolveOpPointer(0, firstValue);
			ResolveOpPointer(1, secondValue);
			int numParam = (int)secondValue.iIntValue;

			Function* func = NULL;
			if (firstValue.type != OP_TYPE_FUNC)
			{
				Value callTagMethod = GetMetaMethod(&firstValue, MMT_Call);
				if (IsValueFunction(&callTagMethod))
					func = callTagMethod.func;
				else
				{
					std::string oprName = GetOperatorName(0);

					ExecError("attempt to call a non-function value (type: %s)", oprName.c_str());
				}
			}
			else
			{
				func = firstValue.func;
			}
			
			CallFunctionInLua(func, numParam);

			if (mCurXScriptState->mStatus == CS_Suspend)
			{
#ifndef INSTR_PC_ADD_NEW
				mCurXScriptState->mInstrIndex++;
#endif
				return;
			}
		}
		break;
		case INSTR_AWAIT:
		{
			ExecInstr_Await();
			if (mCurXScriptState->mStatus == CS_Suspend)
			{
#ifndef INSTR_PC_ADD_NEW
				mCurXScriptState->mInstrIndex++;
#endif
				return;
			}
		}
		break;
		case INSTR_RET:
		{
			ExecInstr_RET();

			if (mCurXScriptState->mCurCallIndex <= mCurXScriptState->mCurCCallLuaCallIndex)
			{
				return;
			}
			else
			{
#ifndef INSTR_PC_ADD_NEW
				mCurXScriptState->mInstrIndex++;
#endif
			}
			break;
		}
		case INSTR_YIELD:
		{
			ResolveOpPointer(0, firstValue);

			if (mCurXScriptState == &mMainXScriptState)
			{
				ExecError("cannot yield from main thread: yield must be called inside a coroutine");
			}
			if (mCurXScriptState->mCCallIndex > 2)
			{
				ExecError("cannot yield across C-call boundary: ensure yield is not called from within a C function");
			}
			if (mCurXScriptState->mCoroutineType != ECoroutineType::Generator)
			{
				ExecError("emit can only be executed in generator context ");
			}

			mCurXScriptState->mStatus = CS_Suspend;
			mCurXScriptState->mSuspendReason = SUSPEND_GEN_YIELD;
			mCurXScriptState->mInstrIndex++;
			//yeild 指令以后要直接return， 否则虚拟机就坏了
			return;
		}
		break;
		case INSTR_SWITCH:
		{
			ResolveOpPointer(0, firstValue);
			ResolveOpPointer(1, secondValue);
			int switchIndex = (int)secondValue.iIntValue;
			RuntimeSwitchData& switchData = mCurXScriptState->mCurFunctionState->mSwitchVec[switchIndex];
			int instrIndex = switchData.defaultInstr;
			if (firstValue.type == ValueType::OP_TYPE_INT)
			{
				auto it = switchData.intInstrMap.find(firstValue.iIntValue);
				if (it != switchData.intInstrMap.end())
				{
					instrIndex = it->second;
				}
			}
			else if (firstValue.type == ValueType::OP_TYPE_FLOAT)
			{
				auto it = switchData.floatInstrMap.find(firstValue.fFloatValue);
				if (it != switchData.floatInstrMap.end())
				{
					instrIndex = it->second;
				}
			}
			else if (firstValue.type == ValueType::OP_TYPE_STRING)
			{
				auto it = switchData.strInstrMap.find(firstValue.stringValue);
				if (it != switchData.strInstrMap.end())
				{
					instrIndex = it->second;
				}
			}

			if (instrIndex >= 0)
			{
				mCurXScriptState->mInstrIndex = instrIndex;
			}
		}
		break;
		case INSTR_LOGIC_NOT:
		{
			ExecInstr_Logic_Not();
		}
		break;
		case INSTR_TEST_E:
		{
			EXEC_INSTR_TEST_E(== );
			//ExecInstr_Test_E();
		}
		break;
		case INSTR_TEST_NE:
		{
			EXEC_INSTR_TEST_E(!= );
			//ExecInstr_Test_NE();
		}
		break;
		case INSTR_TEST_G:
		{
			EXEC_INSTR_TEST(>, MMT_Great);
			//ExecInstr_Test_G();
		}
		break;
		case INSTR_TEST_L:
		{
			EXEC_INSTR_TEST(<, NMT_Less);
			//ExecInstr_Test_L();
		}
		break;
		case INSTR_TEST_GE:
		{
			EXEC_INSTR_TEST(>=, MMT_GreatEqual);
		}
		break;
		case INSTR_TEST_LE:
		{
			EXEC_INSTR_TEST(<=, NMT_LessEqual);
		}
		break;
		case INSTR_FUNC:
		{
			ExecInstr_Func();
		}
		break;
		case INSTR_LOGIC_AND:
		{
			ExecInstr_Logic_And();
		}
		break;
		case INSTR_LOGIC_OR:
		{
			ExecInstr_Logic_Or();
		}
		break;
		case INSTR_ENTERCATCH:
		{
			ResolveOpPointer(0, firstValue);
			int catchBlockIndex = (int)firstValue.iIntValue;
			mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex].mCurTryCatchIndex = catchBlockIndex;

		}
		break;
		case INSTR_EXITCATCH:
		{
			CallInfo& callInfo = mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex];
			int blockIndex = callInfo.mCurTryCatchIndex;
			if (blockIndex >= 0&&  mCurXScriptState->mCurFunctionState && blockIndex < (int)mCurXScriptState->mCurFunctionState->m_tryCatchBlock.size())
			{
				const TryCatchBlock& blockInfo = mCurXScriptState->mCurFunctionState->m_tryCatchBlock[blockIndex];
				int outPC = blockInfo.finallypc;
				if (outPC < 0)
				{
					outPC = blockInfo.outpc;
				}
				mCurXScriptState->mInstrIndex = outPC;
				int parentBlockIndex = blockInfo.parentBlockIndex;
				callInfo.mCurTryCatchIndex = parentBlockIndex;
			}
		}
		break;
		case INSTR_THROW:
		{
			ResolveOpPointer(0, firstValue);

			ExecInstr_Throw(firstValue);
		}
		break;
		case INSTR_J_ARGS_G:
		{
			ResolveOpPointer(0, firstValue);
			ResolveOpPointer(1, secondValue);

			if (mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex].mNumActualArgs > firstValue.iIntValue)
			{
				mCurXScriptState->mInstrIndex = secondValue.iInstrIndex;
			}
		}
		break;
		case INSTR_J_NOT_TYPE:
		{
			ResolveOpPointer(0, firstValue);
			ResolveOpPointer(1, secondValue);
			ResolveOpPointer(2, ThirdValue);
			if (!MatchValueType((int)firstValue.type, (int)secondValue.iIntValue))
			{
				mCurXScriptState->mInstrIndex = ThirdValue.iInstrIndex;
			}
		}
		break;
		case INSTR_DEFER:
		{
			ResolveOpPointer(0, firstValue);
			if (IsValueFunction(&firstValue))
			{
				mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex].mDeferFuncVec.push_back(firstValue.func);
			}
		}
		break;
		case INSTR_APPEND:
		{
			ResolveOpPointer(0, firstValue);
			ResolveOpPointer(1, secondValue);
			if (!IsValueList(&firstValue))
			{
				ExecError("attempt to use 'append' operator on a non-list value (type: %s)", GetOperatorName(0).c_str());
			}

			ListAppend(firstValue.listData, secondValue);
		}
		break;
		}

#ifndef INSTR_PC_ADD_NEW
		if (lastInstrIndex == mCurXScriptState->mInstrIndex && mCurXScriptState->mCurCallIndex == lastCallIndex)
			mCurXScriptState->mInstrIndex++;
#endif
	}
}

void XScriptVM::ScriptThrow(const Value& value)
{
	ExecInstr_Throw(value);
}

void XScriptVM::ExecInstr_Throw(const Value& firstValue)
{
	CallInfo& curCallInfo = mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex];
	int blockIndex = curCallInfo.mCurTryCatchIndex;
	if (blockIndex >= 0)
	{
		//当前函数有 try block，最简单直接跳转到catch指令处
		TryCatchBlock& blockInfo = mCurXScriptState->mCurFunctionState->m_tryCatchBlock[blockIndex];
		mCurXScriptState->mInstrIndex = blockInfo.catchpc;
		int parentBlockIndex = blockInfo.parentBlockIndex;
		curCallInfo.mCurTryCatchIndex = parentBlockIndex;
		CopyValue(&mRegValue[0], firstValue);
	}
	else
	{
		//try catch lua函数时， 需要一次把前面的lu函数一次return， 最后如果try catch 不能跨C++调用层，否则无法处理
		int catchpc = -1;
		int catchCallIndex = -1;
		for (int i = mCurXScriptState->mCurCallIndex - 1; i > mCurXScriptState->mCurCCallLuaCallIndex; i--)
		{
			int blockIndex = mCurXScriptState->mCallInfoBase[i].mCurTryCatchIndex;
			if (blockIndex >= 0)
			{
				catchpc = mCurXScriptState->mCallInfoBase[i].mCurFunctionState->m_tryCatchBlock[blockIndex].catchpc;
				catchCallIndex = i;
				break;
			}
		}

		if (catchpc > 0)
		{
			for (int i = mCurXScriptState->mCurCallIndex; i > catchCallIndex; i--)
			{
				//不用考虑是否是异步函数，因为异步函数 肯定会C Call
				//bool resolveAsyncTask = mCurXScriptState->mIsAsyncTask && mCurXScriptState->mCurCallIndex == 0 && mCurXScriptState->mOwnerTask != NULL;
				ReturnCurFunction(EFunctionReturnType::Throw);
			}
			mCurXScriptState->mInstrIndex = catchpc;
			CopyValue(&mRegValue[0], firstValue);
		}
		else
		{
			ExecError("uncaught exepction", GetOperatorName(0).c_str());
		}

	}
}


// 执行FUNC指令：创建函数闭包，绑定上值
void XScriptVM::ExecInstr_Func()
{
	//ResolveOpPointer(0, destValue);
	ResolveOpPointer(1, firstValue);

	if (firstValue.type == OP_TYPE_INT)
	{
		FuncState* proto = mCurXScriptState->mCurFunctionState->m_subFuncVec[(int)firstValue.iIntValue];
		Function* func = CreateFunction();
		func->isCFunc = false;
		func->funcUnion.luaFunc.proto = proto;
		func->funcUnion.luaFunc.mNumUpVals = (int)proto->m_upValueVec.size();
		if (func->funcUnion.luaFunc.mNumUpVals > 0)
		{
			func->funcUnion.luaFunc.mUpVals = new UpValue*[func->funcUnion.luaFunc.mNumUpVals];

			for (int i = 0; i < (int)proto->m_upValueVec.size(); i++)
			{
				if (proto->m_upValueVec[i].type == VUPVALUE)
				{
					func->funcUnion.luaFunc.mUpVals[i] = mCurXScriptState->mCurFunction->funcUnion.luaFunc.mUpVals[proto->m_upValueVec[i].index];
				}
				else
				{
					Value* pReferValue = &mCurXScriptState->mStackElements[mCurXScriptState->mFrameIndex + proto->m_upValueVec[i].index];
					bool hasFound = false;
					UpValue* nextUpValue = mCurXScriptState->mNextRefUpVals;
					while (nextUpValue != NULL)
					{
						if (nextUpValue->pValue == pReferValue)
						{
							func->funcUnion.luaFunc.mUpVals[i] = nextUpValue;
							hasFound = true;
							break;
						}

						nextUpValue = nextUpValue->nextValue;
					}

					if (!hasFound)
					{
						UpValue* newUpVal = CreateUpVal();
						newUpVal->pValue = pReferValue;
						newUpVal->nextValue = mCurXScriptState->mNextRefUpVals;
						mCurXScriptState->mNextRefUpVals = newUpVal;
						func->funcUnion.luaFunc.mUpVals[i] = newUpVal;
					}
				}
			}
		}

		Value destValue;
		destValue.type = OP_TYPE_FUNC;
		destValue.func = func;
		SetOpValue(0, destValue);

	}
	else
	{
		ExecError("internal error: invalid operand in FUNC instruction");
	}
}
