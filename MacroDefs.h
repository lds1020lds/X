#pragma once
#ifndef MacroDefs_h__
#define MacroDefs_h__

#define	 IsPNumberType(t)	((t).type == OP_TYPE_INT || (t).type == OP_TYPE_FLOAT)
#define  PNumberValue(t)		((t).type == OP_TYPE_INT ? (t).iIntValue : (t).fFloatValue)


#define		 ResolveOpPointer(index, T)			\
	Value T = *(Value*)&mCurInstr->mOpList[index];		\
	switch (T.type)	\
	{	\
	case ROT_Stack_Index:	\
	{							\
		int stackIndex = mCurInstr->mOpList[index].iStackIndex;	\
		T = ResoveStackIndexWithEnv(stackIndex);\
		break;	\
	}\
	case ROT_Table:\
	case ROT_UpValue_Table:\
	{	\
		resolveTableValue(&mCurInstr->mOpList[index], &T);	\
	}	\
	break;	\
	case ROT_UpVal_Index: \
	{	\
			int stackIndex = mCurInstr->mOpList[index].iStackIndex;	\
			T = *mCurXScriptState->mCurFunction->funcUnion.luaFunc.mUpVals[stackIndex]->pValue;	\
	}	\
	break;	\
	case ROT_Reg:	\
	{	\
		T = mRegValue[T.iRegIndex];	\
	}	\
	break; \
	case ROT_Nil: \
	{ \
		T = mNilValue; \
	} \
	}	

#define CopyValue(destValue, srcValue)		*(destValue) = (srcValue);

#define		EXEC_INSTR_TEST_E(OP)		\
	ResolveOpPointer(1, firstValue);	\
	ResolveOpPointer(2, secondValue);	\
	XInt iResult = 0;						\
	if (IsPNumberType(firstValue) && IsPNumberType(secondValue))	\
	{	\
		iResult = (PNumberValue(firstValue) OP PNumberValue(secondValue)) ? 1 : 0;	\
	}	\
	else if (firstValue.type == secondValue.type)	\
	{	\
		switch (firstValue.type)	\
		{	\
		case OP_TYPE_STRING:	\
		{	\
			iResult = (firstValue.stringValue OP secondValue.stringValue) ? 1 : 0;	\
		}	\
		break;	\
		case OP_TYPE_USERDATA: \
		case OP_TYPE_LIST:	\
		case OP_TYPE_TABLE:	\
		{	\
			iResult = (firstValue.tableData OP secondValue.tableData) ? 1 : 0;	\
			if(iResult == 0)	\
			{	\
				Value resultValue;	\
				if (CalByTagMethod(&resultValue, &firstValue, &secondValue, MMT_Equal))	\
				{	\
					if (resultValue.type == OP_TYPE_NIL || (IsPNumberType(resultValue) && PNumberValue(resultValue) == 0))	\
					{	\
						iResult = 0;	\
					}	\
					else \
					{	\
						iResult = 1;	\
					}	\
				}	\
				if (1 OP 0)	\
				{	\
					iResult = 1 - iResult;	\
				}	\
			}	\
		}	\
		break;	\
		case OP_LIGHTUSERDATA:	\
		{	\
			iResult = (firstValue.func OP secondValue.func) ? 1 : 0;	\
		}	\
		break;	\
		case OP_TYPE_NIL:	\
		{	\
			iResult = (1 OP 1) ? 1 : 0;	\
		}	\
		break;	\
		default:	\
			iResult = 0;	\
			break;	\
		}	\
	}	\
	else \
	{	\
		iResult = (1 OP 0) ? 1 : 0;	\
	}		\
	SetOpValue(0, ConstructValue(iResult));


#define		EXEC_INSTR_J(OP, mmt)	\
ResolveOpPointer(0, firstValue);	\
ResolveOpPointer(1, secondValue);	\
if (firstValue.type == OP_TYPE_INT)		\
{	\
	if (secondValue.type == OP_TYPE_INT)	\
	{	\
		if (firstValue.iIntValue OP secondValue.iIntValue)	\
			mCurXScriptState->mInstrIndex = mCurInstr->mOpList[2].iInstrIndex;	\
	}	\
	else if (secondValue.type == OP_TYPE_FLOAT)	\
	{	\
		if (firstValue.iIntValue OP secondValue.fFloatValue)	\
			mCurXScriptState->mInstrIndex = mCurInstr->mOpList[2].iInstrIndex;	\
	}	\
	else \
	{	\
ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
	}	\
}	\
else if (firstValue.type == OP_TYPE_FLOAT)	\
{	\
	if (secondValue.type == OP_TYPE_INT)	\
	{	\
		if (firstValue.fFloatValue OP secondValue.iIntValue)	\
			mCurXScriptState->mInstrIndex = mCurInstr->mOpList[2].iInstrIndex;	\
	}	\
	else if (secondValue.type == OP_TYPE_FLOAT)	\
	{	\
		if (firstValue.fFloatValue OP secondValue.fFloatValue)	\
			mCurXScriptState->mInstrIndex = mCurInstr->mOpList[2].iInstrIndex;	\
	}	\
	else \
	{	\
		ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
	}	\
}	\
else if (firstValue.type == OP_TYPE_STRING)	\
{	\
	if (secondValue.type == OP_TYPE_STRING)	\
	{	\
		if (strcmp(stringRawValue(&firstValue), stringRawValue(&secondValue)) OP 0 )	\
			mCurXScriptState->mInstrIndex = mCurInstr->mOpList[2].iInstrIndex;	\
	}	\
	else \
	{	\
		ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
	}	\
}	\
else if (firstValue.type == secondValue.type)	\
{	\
	Value resultValue;	\
	if (CalByTagMethod(&resultValue, &firstValue, &secondValue, mmt))	\
	{	\
		if (resultValue.type == OP_TYPE_NIL || (IsPNumberType(resultValue) && PNumberValue(resultValue) == 0))	\
		{	\
		}	\
		else \
		{	\
			mCurXScriptState->mInstrIndex = mCurInstr->mOpList[2].iInstrIndex;	\
		}	\
	}	\
	else \
		ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
}	\
else \
{	\
	ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
}


#define  EXEC_INSTR_JE(OP)	\
	ResolveOpPointer(0, firstValue);	\
	ResolveOpPointer(1, secondValue);	\
	ResolveOpPointer(2, thirdValue);	\
	int ret;	\
	if (IsPNumberType(firstValue) && IsPNumberType(secondValue))	\
	{	\
		ret = (PNumberValue(firstValue) OP PNumberValue(secondValue)) ? 1 : 0;	\
	}	\
	else if (firstValue.type == secondValue.type)	\
	{	\
	switch (firstValue.type)	\
	{	\
	case OP_TYPE_STRING:	\
	{	\
		ret = (firstValue.stringValue OP secondValue.stringValue) ? 1 : 0;	\
	}	\
	break;	\
	case OP_TYPE_USERDATA: \
	case OP_TYPE_TABLE:	\
	{	\
		ret = (firstValue.tableData OP secondValue.tableData) ? 1 : 0;	\
		if(ret == 0)	\
		{	\
			Value resultValue;	\
			if (CalByTagMethod(&resultValue, &firstValue, &secondValue, MMT_Equal))	\
			{	\
				if (resultValue.type == OP_TYPE_NIL || (IsPNumberType(resultValue) && PNumberValue(resultValue) == 0))	\
				{	\
					ret = 0;	\
				}	\
				else \
				{	\
					ret = 1;	\
				}	\
			}	\
			if (1 OP 0)	\
			{	\
				ret = 1 - ret;	\
			}	\
		}	\
	}	\
	break;	\
	case OP_LIGHTUSERDATA:	\
	{	\
		ret = (firstValue.lightUserData OP secondValue.lightUserData) ? 1 : 0;	\
	}	\
	break;	\
	case OP_TYPE_NIL:	\
	{	\
		ret = (1 OP 1) ? 1 : 0;	\
	}	\
	break;	\
	default:	\
		ret = 0;	\
		break;	\
	}	\
	}	\
	else \
	{	\
		ret = 1 OP 0 ? 1 : 0;	\
	}	\
	if (ret)	\
	{	\
		mCurXScriptState->mInstrIndex = thirdValue.iInstrIndex;	\
	}	

#define  EXEC_INSTR_TEST(OP, mmt)	\
ResolveOpPointer(1, firstValue);	\
ResolveOpPointer(2, secondValue);	\
XInt iResult = 0;	\
if (firstValue.type == OP_TYPE_INT)	\
{	\
	if (secondValue.type == OP_TYPE_INT)	\
	{	\
		iResult = (firstValue.iIntValue OP secondValue.iIntValue) ? 1 : 0;	\
	}	\
	else if (secondValue.type == OP_TYPE_FLOAT)	\
	{	\
		iResult = (firstValue.iIntValue OP secondValue.fFloatValue) ? 1 : 0;	\
	}	\
	else \
	{	\
		ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
	}	\
}	\
else if (firstValue.type == OP_TYPE_FLOAT)	\
{	\
	if (secondValue.type == OP_TYPE_INT)	\
	{	\
		iResult = (firstValue.fFloatValue OP secondValue.iIntValue) ? 1 : 0;	\
	}	\
	else if (secondValue.type == OP_TYPE_FLOAT)	\
	{	\
		iResult = (firstValue.fFloatValue OP secondValue.fFloatValue) ? 1 : 0;	\
	}	\
	else \
	{	\
		ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
	}	\
}	\
else if (firstValue.type == OP_TYPE_STRING)	\
{	\
	if (secondValue.type == OP_TYPE_STRING)	\
	{	\
		iResult = (strcmp(stringRawValue(&firstValue), stringRawValue(&secondValue)) OP 0) ? 1 : 0;	\
	}	\
	else \
	{	\
		ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
	}	\
}	\
else if (firstValue.type == secondValue.type)	\
{	\
	Value resultValue;	\
	if (CalByTagMethod(&resultValue, &firstValue, &secondValue, mmt))	\
	{	\
		if (resultValue.type == OP_TYPE_NIL || (IsPNumberType(resultValue) && PNumberValue(resultValue) == 0))	\
		{	\
			iResult = 0;	\
		}	\
		else \
		{	\
			iResult = 1;	\
		}	\
	}	\
	else \
		ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
}	\
else \
{	\
	ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
}	\
SetOpValue(0, ConstructValue(iResult));




#define _NO_CHECK(v)
#define _CHECK_DIVISOR_ZERO(v) \
	if (IsPNumberType(v) && PNumberValue(v) == 0) ExecError("attempt to divide by zero");

#define EXEC_INSTR_MATH(OP, mmt, CHK)	\
	ResolveOpPointer(0, destValue);	\
	ResolveOpPointer(1, secondValue);	\
	CHK(secondValue)	\
	Value iResultValue;	if (destValue.type == OP_TYPE_INT)	\
	{	\
		if (secondValue.type == OP_TYPE_INT)	\
		{	\
			iResultValue.type = OP_TYPE_INT;	\
			iResultValue.iIntValue = destValue.iIntValue OP secondValue.iIntValue;	\
		}	\
		else if (secondValue.type == OP_TYPE_FLOAT)	\
		{	\
			iResultValue.fFloatValue = (XFloat)destValue.iIntValue OP secondValue.fFloatValue;	\
			iResultValue.type = OP_TYPE_FLOAT;	\
		}	\
		else if (secondValue.type == OP_TYPE_STRING)	\
		{	\
			XFloat value;	\
			if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
			{	\
				iResultValue.fFloatValue = (XFloat)destValue.iIntValue OP value;	\
				iResultValue.type = OP_TYPE_FLOAT;	\
			}	\
		}	\
	}	\
	else if (destValue.type == OP_TYPE_FLOAT)	\
	{	\
		iResultValue.type = OP_TYPE_FLOAT;	\
		if (secondValue.type == OP_TYPE_INT)	\
			iResultValue.fFloatValue = destValue.fFloatValue OP secondValue.iIntValue;	\
		else if (secondValue.type == OP_TYPE_FLOAT)	\
		{	\
			iResultValue.fFloatValue = destValue.fFloatValue OP secondValue.fFloatValue;	\
		}	\
		else if (secondValue.type == OP_TYPE_STRING)	\
		{	\
			XFloat value;	\
			if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
			{	\
				iResultValue.fFloatValue = destValue.fFloatValue OP value;	\
			}	\
		}	\
	}	\
	else if (destValue.type == OP_TYPE_STRING)	\
	{	\
		XFloat destFloatValue;	\
		if (CastStrToFloat(stringRawValue(&destValue), &destFloatValue))	\
		{	\
			iResultValue.type = OP_TYPE_FLOAT;	\
			if (secondValue.type == OP_TYPE_INT)	\
				iResultValue.fFloatValue = destFloatValue OP (XFloat)secondValue.iIntValue;	\
			else if (secondValue.type == OP_TYPE_FLOAT)	\
			{	\
				iResultValue.fFloatValue = destFloatValue OP secondValue.fFloatValue;	\
			}	\
			else if (secondValue.type == OP_TYPE_STRING)	\
			{	\
				XFloat value;	\
				if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
				{	\
					iResultValue.fFloatValue = destFloatValue OP value;	\
				}	\
			}	\
		}	\
	}	\
	if (iResultValue.type == OP_TYPE_NIL)	\
	{	\
		if (!CalByTagMethod(&iResultValue, &destValue, &secondValue, mmt))	\
		{	\
			ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
		}	\
	}	\
	SetOpValue(0, iResultValue);


#define EXEC_INSTR_MATH_TO(OP, mmt, CHK)	\
	Value iResultValue;	\
	ResolveOpPointer(1, destValue);	\
	ResolveOpPointer(2, secondValue);	\
	CHK(secondValue)	\
	if (destValue.type == OP_TYPE_INT)	{	\
		if (secondValue.type == OP_TYPE_INT)	\
		{	\
			iResultValue.type = OP_TYPE_INT;	\
			iResultValue.iIntValue = destValue.iIntValue OP secondValue.iIntValue;	\
		}	\
		else if (secondValue.type == OP_TYPE_FLOAT)	\
		{	\
			iResultValue.fFloatValue = (XFloat)destValue.iIntValue OP secondValue.fFloatValue;	\
			iResultValue.type = OP_TYPE_FLOAT;	\
		}	\
		else if (secondValue.type == OP_TYPE_STRING)	\
		{	\
			XFloat value;	\
			if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
			{		\
				iResultValue.fFloatValue = (XFloat)destValue.iIntValue OP value;	\
				iResultValue.type = OP_TYPE_FLOAT;	\
			}	\
		}	\
	}	\
	else if (destValue.type == OP_TYPE_FLOAT)	\
	{	\
		iResultValue.type = OP_TYPE_FLOAT;	\
		if (secondValue.type == OP_TYPE_INT)	\
			iResultValue.fFloatValue = destValue.fFloatValue OP secondValue.iIntValue;	\
		else if (secondValue.type == OP_TYPE_FLOAT)	\
		{	\
			iResultValue.fFloatValue = destValue.fFloatValue OP secondValue.fFloatValue;	\
		}	\
		else if (secondValue.type == OP_TYPE_STRING)	\
		{	\
			XFloat value;	\
			if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
			{	\
				iResultValue.fFloatValue = destValue.fFloatValue OP value;	\
			}	\
		}	\
	}	\
	else if (destValue.type == OP_TYPE_STRING)	\
	{	\
		XFloat destFloatValue;	\
		if (CastStrToFloat(stringRawValue(&destValue), &destFloatValue))	\
		{	\
			iResultValue.type = OP_TYPE_FLOAT;	\
			if (secondValue.type == OP_TYPE_INT)	\
				iResultValue.fFloatValue = destFloatValue OP (XFloat)secondValue.iIntValue;	\
			else if (secondValue.type == OP_TYPE_FLOAT)	\
			{	\
				iResultValue.fFloatValue = destFloatValue OP secondValue.fFloatValue;	\
			}	\
			else if (secondValue.type == OP_TYPE_STRING)	\
			{	\
				XFloat value;	\
				if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
				{	\
					iResultValue.fFloatValue = destFloatValue OP value;	\
				}	\
			}	\
		}	\
	}	\
	if (iResultValue.type == OP_TYPE_NIL)	\
	{	\
		if (!CalByTagMethod(&iResultValue, &destValue, &secondValue, mmt))	\
		{	\
			ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
		}	\
	}	\
	SetOpValue(0, iResultValue);
	


#define EXEC_INSTR_MATH_OP_TO(OP, mmt)	\
	Value iResultValue;	\
	ResolveOpPointer(1, destValue);	\
	ResolveOpPointer(2, secondValue);	\
	if (destValue.type == OP_TYPE_INT)	\
	{	\
		if (secondValue.type == OP_TYPE_INT)	\
		{	\
			iResultValue.type = OP_TYPE_INT;	\
			iResultValue.iIntValue = (XInt)(XFloat)(OP((XFloat)destValue.iIntValue, (XFloat)secondValue.iIntValue));	\
		}	\
		else if (secondValue.type == OP_TYPE_FLOAT)	\
		{	\
			iResultValue.fFloatValue = (XFloat)(OP((XFloat)destValue.iIntValue, secondValue.fFloatValue));	\
			iResultValue.type = OP_TYPE_FLOAT;	\
		}	\
		else if (secondValue.type == OP_TYPE_STRING)	\
		{	\
			XFloat value;	\
			if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
			{		\
				iResultValue.fFloatValue = (XFloat)(OP((XFloat)destValue.iIntValue, value));	\
				iResultValue.type = OP_TYPE_FLOAT;	\
			}	\
		}	\
	}	\
	else if (destValue.type == OP_TYPE_FLOAT)	\
	{	\
		iResultValue.type = OP_TYPE_FLOAT;	\
		if (secondValue.type == OP_TYPE_INT)	\
			iResultValue.fFloatValue = (XFloat)(OP(destValue.fFloatValue, (XFloat)secondValue.iIntValue));	\
		else if (secondValue.type == OP_TYPE_FLOAT)	\
		{	\
			iResultValue.fFloatValue = (XFloat)(OP(destValue.fFloatValue, secondValue.fFloatValue));	\
		}	\
		else if (secondValue.type == OP_TYPE_STRING)	\
		{	\
			XFloat value;	\
			if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
			{	\
				iResultValue.fFloatValue = (XFloat)(OP(destValue.fFloatValue, value));	\
			}	\
		}	\
	}	\
	else if (destValue.type == OP_TYPE_STRING)	\
	{	\
		XFloat destFloatValue;	\
		if (CastStrToFloat(stringRawValue(&destValue), &destFloatValue))	\
		{	\
			iResultValue.type = OP_TYPE_FLOAT;	\
			if (secondValue.type == OP_TYPE_INT)	\
				iResultValue.fFloatValue = (XFloat)(OP(destFloatValue, (XFloat)secondValue.iIntValue));	\
			else if (secondValue.type == OP_TYPE_FLOAT)	\
			{	\
				iResultValue.fFloatValue = (XFloat)(OP(destFloatValue, secondValue.fFloatValue));	\
			}	\
			else if (secondValue.type == OP_TYPE_STRING)	\
			{	\
				XFloat value;	\
				if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
				{	\
					iResultValue.fFloatValue = (XFloat)(OP(destFloatValue, value));	\
				}	\
			}	\
		}	\
	}	\
	if (iResultValue.type == OP_TYPE_NIL)	\
	{	\
		if (!CalByTagMethod(&iResultValue, &destValue, &secondValue, mmt))	\
		{	\
			ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
		}	\
	}	\
	SetOpValue(0, iResultValue);


#define EXEC_INSTR_MATH_OP(OP, mmt)	\
	ResolveOpPointer(0, destValue);	\
	Value iResultValue;				\
	ResolveOpPointer(1, secondValue);	\
	if (destValue.type == OP_TYPE_INT)	\
	{	\
		if (secondValue.type == OP_TYPE_INT)	\
		{	\
			iResultValue.type = OP_TYPE_INT;	\
			iResultValue.iIntValue = (XInt)(XFloat)(OP((XFloat)destValue.iIntValue, (XFloat)secondValue.iIntValue));	\
		}	\
		else if (secondValue.type == OP_TYPE_FLOAT)	\
		{	\
			iResultValue.fFloatValue = (XFloat)(OP((XFloat)destValue.iIntValue, secondValue.fFloatValue));	\
			iResultValue.type = OP_TYPE_FLOAT;	\
		}	\
		else if (secondValue.type == OP_TYPE_STRING)	\
		{	\
			XFloat value;	\
			if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
			{	\
				iResultValue.fFloatValue = (XFloat)(OP((XFloat)destValue.iIntValue, value));	\
				iResultValue.type = OP_TYPE_FLOAT;	\
			}	\
		}	\
	}	\
	else if (destValue.type == OP_TYPE_FLOAT)	\
	{	\
		iResultValue.type = OP_TYPE_FLOAT;	\
		if (secondValue.type == OP_TYPE_INT)	\
		iResultValue.fFloatValue = (XFloat)(OP(destValue.fFloatValue,  (XFloat)secondValue.iIntValue));	\
		else if (secondValue.type == OP_TYPE_FLOAT)	\
		{	\
			iResultValue.fFloatValue = (XFloat)(OP(destValue.fFloatValue, secondValue.fFloatValue));	\
		}	\
		else if (secondValue.type == OP_TYPE_STRING)	\
		{	\
			XFloat value;	\
			if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
			{	\
				iResultValue.fFloatValue = (XFloat)(OP(destValue.fFloatValue, value));	\
			}	\
		}	\
	}	\
	else if (destValue.type == OP_TYPE_STRING)	\
	{	\
		XFloat destFloatValue;	\
		if (CastStrToFloat(stringRawValue(&destValue), &destFloatValue))	\
		{	\
			iResultValue.type = OP_TYPE_FLOAT;	\
			if (secondValue.type == OP_TYPE_INT)	\
				iResultValue.fFloatValue = (XFloat)(OP(destFloatValue, (XFloat)secondValue.iIntValue));	\
			else if (secondValue.type == OP_TYPE_FLOAT)	\
			{	\
				iResultValue.fFloatValue = (XFloat)(OP(destFloatValue, secondValue.fFloatValue));	\
			}	\
			else if (secondValue.type == OP_TYPE_STRING)	\
			{	\
				XFloat value;	\
				if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
				{	\
					iResultValue.fFloatValue = (XFloat)(OP(destFloatValue, value));	\
				}	\
			}	\
		}	\
	}	\
	if (iResultValue.type == OP_TYPE_NIL)	\
	{	\
		if (!CalByTagMethod(&iResultValue, &destValue, &secondValue, mmt))	\
		{	\
			ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
		}	\
	}	\
	SetOpValue(0, iResultValue);



#define EXEC_INSTR_MATH_INTOP_TO(OP, IntOP, mmt, CHK)	\
	Value iResultValue;				\
	ResolveOpPointer(1, destValue);	\
	ResolveOpPointer(2, secondValue);	\
	CHK(secondValue)	\
	if (destValue.type == OP_TYPE_INT)	{	\
		if (secondValue.type == OP_TYPE_INT)	\
		{	\
			iResultValue.type = OP_TYPE_INT;	\
			iResultValue.iIntValue = destValue.iIntValue IntOP secondValue.iIntValue;	\
		}	\
		else if (secondValue.type == OP_TYPE_FLOAT)	\
		{	\
			iResultValue.fFloatValue = OP((XFloat)destValue.iIntValue, secondValue.fFloatValue);	\
			iResultValue.type = OP_TYPE_FLOAT;	\
		}	\
		else if (secondValue.type == OP_TYPE_STRING)	\
		{	\
			XFloat value;	\
			if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
			{		\
				iResultValue.fFloatValue = OP((XFloat)destValue.iIntValue, value);	\
				iResultValue.type = OP_TYPE_FLOAT;	\
			}	\
		}	\
	}	\
	else if (destValue.type == OP_TYPE_FLOAT)	\
	{	\
		iResultValue.type = OP_TYPE_FLOAT;	\
		if (secondValue.type == OP_TYPE_INT)	\
			iResultValue.fFloatValue = OP(destValue.fFloatValue, (XFloat)secondValue.iIntValue);	\
		else if (secondValue.type == OP_TYPE_FLOAT)	\
		{	\
			iResultValue.fFloatValue = OP(destValue.fFloatValue, secondValue.fFloatValue);	\
		}	\
		else if (secondValue.type == OP_TYPE_STRING)	\
		{	\
			XFloat value;	\
			if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
			{	\
				iResultValue.fFloatValue = OP(destValue.fFloatValue, value);	\
			}	\
		}	\
	}	\
	else if (destValue.type == OP_TYPE_STRING)	\
	{	\
		XFloat destFloatValue;	\
		if (CastStrToFloat(stringRawValue(&destValue), &destFloatValue))	\
		{	\
			iResultValue.type = OP_TYPE_FLOAT;	\
			if (secondValue.type == OP_TYPE_INT)	\
				iResultValue.fFloatValue = OP(destFloatValue, (XFloat)secondValue.iIntValue);	\
			else if (secondValue.type == OP_TYPE_FLOAT)	\
			{	\
				iResultValue.fFloatValue = OP(destFloatValue, secondValue.fFloatValue);	\
			}	\
			else if (secondValue.type == OP_TYPE_STRING)	\
			{	\
				XFloat value;	\
				if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
				{	\
					iResultValue.fFloatValue = OP(destFloatValue, value);	\
				}	\
			}	\
		}	\
	}	\
	if (iResultValue.type == OP_TYPE_NIL)	\
	{	\
		if (!CalByTagMethod(&iResultValue, &destValue, &secondValue, mmt))	\
		{	\
			ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
		}	\
	}	\
	SetOpValue(0, iResultValue);


#define EXEC_INSTR_MATH_INTOP(OP, IntOP, mmt, CHK)	\
	Value iResultValue;		\
	ResolveOpPointer(0, destValue);	\
	ResolveOpPointer(1, secondValue);	\
	CHK(secondValue)	\
	if (destValue.type == OP_TYPE_INT)	{	\
		if (secondValue.type == OP_TYPE_INT)	\
		{	\
			iResultValue.type = OP_TYPE_INT;	\
			iResultValue.iIntValue = destValue.iIntValue IntOP secondValue.iIntValue;	\
		}	\
		else if (secondValue.type == OP_TYPE_FLOAT)	\
		{	\
			iResultValue.fFloatValue = OP((XFloat)destValue.iIntValue, secondValue.fFloatValue);	\
			iResultValue.type = OP_TYPE_FLOAT;	\
		}	\
		else if (secondValue.type == OP_TYPE_STRING)	\
		{	\
			XFloat value;	\
			if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
			{	\
				iResultValue.fFloatValue = OP((XFloat)destValue.iIntValue, value);	\
				iResultValue.type = OP_TYPE_FLOAT;	\
			}	\
		}	\
	}	\
	else if (destValue.type == OP_TYPE_FLOAT)	\
	{	\
		iResultValue.type = OP_TYPE_FLOAT;	\
		if (secondValue.type == OP_TYPE_INT)	\
			iResultValue.fFloatValue = OP(destValue.fFloatValue,  (XFloat)secondValue.iIntValue);	\
		else if (secondValue.type == OP_TYPE_FLOAT)	\
		{	\
			iResultValue.fFloatValue = OP(destValue.fFloatValue, secondValue.fFloatValue);	\
		}	\
		else if (secondValue.type == OP_TYPE_STRING)	\
		{	\
			XFloat value;	\
			if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
			{	\
				iResultValue.fFloatValue = OP(destValue.fFloatValue, value);	\
			}	\
		}	\
	}	\
	else if (destValue.type == OP_TYPE_STRING)	\
	{	\
		XFloat destFloatValue;	\
		if (CastStrToFloat(stringRawValue(&destValue), &destFloatValue))	\
		{	\
			iResultValue.type = OP_TYPE_FLOAT;	\
			if (secondValue.type == OP_TYPE_INT)	\
				iResultValue.fFloatValue = (XFloat)(OP(destFloatValue, (XFloat)secondValue.iIntValue));	\
			else if (secondValue.type == OP_TYPE_FLOAT)	\
			{	\
				iResultValue.fFloatValue = (XFloat)(OP(destFloatValue, secondValue.fFloatValue));	\
			}	\
			else if (secondValue.type == OP_TYPE_STRING)	\
			{	\
				XFloat value;	\
				if (CastStrToFloat(stringRawValue(&secondValue), &value))	\
				{	\
					iResultValue.fFloatValue = (XFloat)(OP(destFloatValue, value));	\
				}	\
			}	\
		}	\
	}	\
	if (iResultValue.type == OP_TYPE_NIL)	\
	{	\
		if (!CalByTagMethod(&iResultValue, &destValue, &secondValue, mmt))	\
		{	\
			ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
		}	\
	}	\
	SetOpValue(0, iResultValue);



#define EXEC_INSTR_MATH_INC(OP)	ResolveOpPointer(0, destValue);	\
	if (destValue.type == OP_TYPE_INT)	\
		SetOpValue(0, ConstructValue(destValue.iIntValue OP 1));	\
	else \
		ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\


#define EXEC_INSTR_LOGIC_OP_TO(OP)	\
	ResolveOpPointer(1, second);	\
	ResolveOpPointer(2, third);		\
	if (second.type == OP_TYPE_INT && third.type == OP_TYPE_INT)	\
	{	\
		SetOpValue(0, ConstructValue((XInt)(second.iIntValue OP third.iIntValue)));	\
	}	\
	else \
	{	\
		ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
	}

#define EXEC_INSTR_LOGIC_OP(OP)	\
	ResolveOpPointer(0, destValue);	\
	ResolveOpPointer(1, second);	\
	if (destValue.type == OP_TYPE_INT && second.type == OP_TYPE_INT)	\
	{	\
		SetOpValue(0, ConstructValue((XInt)(destValue.iIntValue OP second.iIntValue)));	\
	}	\
	else \
	{	\
		ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
	}

#define EXEC_INSTR_BIT_OP(OP)	\
	ResolveOpPointer(0, destValue);	\
	ResolveOpPointer(1, second);	\
	if (destValue.type == OP_TYPE_INT && second.type == OP_TYPE_INT)	\
	{	\
		SetOpValue(0, ConstructValue((XInt)(destValue.iIntValue OP second.iIntValue)));	\
	}	\
	else \
	{	\
		ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
	}

#define EXEC_INSTR_BIT_OP_TO(OP)	\
	ResolveOpPointer(1, second);	\
	ResolveOpPointer(2, third);		\
	if (second.type == OP_TYPE_INT && third.type == OP_TYPE_INT)	\
	{	\
		SetOpValue(0, ConstructValue((XInt)(second.iIntValue OP third.iIntValue)));	\
	}	\
	else \
	{	\
		ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
	}


#define GetHashPos(hashPos,t, key)	\
	hashPos = 0;	\
	if (IsUserType((key)->type))	\
	{	\
		hashPos = (int)((intptr_t)(key)->lightUserData % (t)->mNodeCapacity);	\
	}	\
	else \
	{	\
		switch ((key)->type)	\
		{	\
		case OP_TYPE_INT:	\
		case OP_TYPE_FLOAT:	\
			hashPos = (int)PNumberValue(*(key)) % (t)->mNodeCapacity;	\
			break;	\
		case OP_TYPE_STRING:	\
			hashPos = (int)((key)->stringValue->hash % ((t)->mNodeCapacity));	\
			break;	\
		case OP_TYPE_TABLE:	\
			hashPos = (int)((intptr_t)(key)->tableData % ((t)->mNodeCapacity - 1));	\
		break;	\
		case OP_LIGHTUSERDATA:	\
		hashPos = (int)((intptr_t)(key)->lightUserData % ((t)->mNodeCapacity - 1));	\
		break;	\
		case OP_TYPE_FUNC:	\
		hashPos = (int)((intptr_t)(key)->func % ((t)->mNodeCapacity - 1));	\
		break;	\
		default:	\
			break;	\
		}	\
	}	


#endif // MacroDefs_h__