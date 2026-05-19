#include "XScriptVM.h"

// 获取参数类型
int   XScriptVM::getParamType(int paramIndex)
{
	Value value = getParamValue(paramIndex);

	int type = value.type;
	if (IsValueLightUserData(&value))
		type = OP_LIGHTUSERDATA;
	return type;
}


// 获取参数Value
Value XScriptVM::getParamValue(int paramIndex)
{
	if (paramIndex < mNumHostFuncParam)
	{
		int index = mCurXScriptState->mTopIndex - (mNumHostFuncParam - paramIndex);
		return mCurXScriptState->mStackElements[index];
	}
	else
	{
		return Value();
	}
}

// 获取整数参数
bool  XScriptVM::getParamAsInt(int paramIndex, XInt& value)
{
	Value stackValue = getParamValue(paramIndex);
	if (IsValueNumber(&stackValue))
	{
		value = (XInt)PNumberValue(stackValue);
		return true;
	}
	return false;
}

// 获取浮点参数
bool  XScriptVM::getParamAsFloat(int paramIndex, XFloat& value)
{
	Value stackValue = getParamValue(paramIndex);
	if (IsValueNumber(&stackValue))
	{
		value = PNumberValue(stackValue);
		return true;
	}
	return false;
}

// 获取字符串参数
bool  XScriptVM::getParamAsString(int paramIndex, char* &value)
{
	Value stackValue = getParamValue(paramIndex);
	if (IsValueString(&stackValue))
	{
		value = (char*)stringRawValue(&stackValue);
		return true;
	}

	return false;
}


// 获取用户对象参数（带类型检查和继承链验证）
void*  XScriptVM::getParamAsObj(int paramIndex, char* userType)
{
	Value stackValue = getParamValue(paramIndex);
	if (stackValue.type == OP_TYPE_USERDATA)
	{
		TABLE metaTable = NULL;
		if (getTableValue(mMetaTable, userType, metaTable))
		{
			bool isValid = false;
			TABLE valueMeta = stackValue.userData->mMetaTable;
			while (valueMeta != NULL)
			{
				if (valueMeta == metaTable)
				{
					isValid = true;
					break;
				}
				else
				{
					valueMeta = valueMeta->mMetaTable;
				}
			}

			if (isValid)
			{
				void* pThis = NULL;
				memcpy(&pThis, stackValue.userData->value, sizeof(void*));
				return pThis;
			}
			
		}
	}

	return NULL;
}

// 获取表参数
bool  XScriptVM::getParamAsTable(int paramIndex, TABLE& table)
{
	int index = mCurXScriptState->mTopIndex - (mNumHostFuncParam - paramIndex);
	Value& value = mCurXScriptState->mStackElements[index];
	if (value.type == OP_TYPE_TABLE)
	{
		table = value.tableData;
		return true;
	}

	return false;
}

// 设置返回值为nil
void  XScriptVM::setReturnAsNil(int regIndex)
{
	mRegValue[regIndex].type = OP_TYPE_NIL;
	mRegValue[regIndex].iIntValue = 0;
}


// 设置返回值为Value
void  XScriptVM::setReturnAsValue(const Value& table, int regIndex)
{
	CopyValue(&mRegValue[regIndex], table);
}

// 设置返回值为表
void  XScriptVM::setReturnAsTable(const TABLE& table, int regIndex)
{
	CopyValue(&mRegValue[regIndex], ConstructValue(table));
}

// 设置返回值为整数
void  XScriptVM::setReturnAsInt(XInt iResult, int regIndex)
{
	Value value;
	value.type = OP_TYPE_INT;
	value.iIntValue = iResult;
	CopyValue(&mRegValue[regIndex], value);
}

// 重置所有返回值寄存器为nil
void  XScriptVM::resetReturnValue()
{
	for (int i = 0; i < MAX_FUNC_REG; i++)
	{
		mRegValue[i].type = OP_TYPE_NIL;
	}

}

// 设置返回值为用户数据对象
void  XScriptVM::setReturnAsUserData(const char* className, void* pThis, int regIndex)
{
	if (pThis != NULL)
	{
		SetUserDataValue(&mRegValue[regIndex], CreateClassUserData(className, pThis));
	}
	else
	{
		setReturnAsNil(regIndex);
	}
}

// 创建类用户数据对象：分配内存并绑定元表
UserData* XScriptVM::CreateClassUserData(const char* className, void* pThis)
{
	UserData* userData = CreateUserData(sizeof(void*));
	TABLE metaTable;
	if (getTableValue(mMetaTable, (char*)className, metaTable))
	{
		userData->mMetaTable = metaTable;
	}

	memcpy(userData->value, &pThis, sizeof(void*));
	return userData;
}

// 设置返回值为浮点数
void  XScriptVM::setReturnAsfloat(XFloat fResult, int regIndex)
{
	Value value;
	value.type = OP_TYPE_FLOAT;
	value.fFloatValue = fResult;
	CopyValue(&mRegValue[regIndex], value);
}


// 设置返回值为字符串
void  XScriptVM::setReturnAsStr(const char* strResult, int regIndex)
{
	Value value = ConstructValue((char*)strResult);
	CopyValue(&mRegValue[regIndex], value);
}

// 创建宿主C函数Value（创建函数对象、绑定函数指针、包装为Value）
Value XScriptVM::CreateCHostFunction(HOST_FUNC pfnAddr, int numUpvals)
{
	Function* f = CreateCFunction(numUpvals);
	f->funcUnion.cFunc.pfnAddr = pfnAddr;
	Value v;
	XSetFuncValue(&v, f);
	return v;
}

// 注册宿主API函数（带上值闭包版本）
void	XScriptVM::RegisterHostApi(const char* apiName, HOST_FUNC pfnAddr, HOST_FUNC pfnAddr2)
{
	int index = RegisterGlobalValue(apiName);

	Value v = CreateCHostFunction(pfnAddr, 1);
	setGlobalStackValue(index, v);

	Value v2 = CreateCHostFunction(pfnAddr2, 0);
	v.func->funcUnion.cFunc.mUpVal[0] = v2;
}


// 注册宿主API函数
void  XScriptVM::RegisterHostApi(const char* apiName, HOST_FUNC pfnAddr)
{
	int index = RegisterGlobalValue(apiName);
	Value v = CreateCHostFunction(pfnAddr, 0);
	setGlobalStackValue(index, v);
}

// 注册用户自定义类：创建元表、注册方法和常量、处理继承关系
void	XScriptVM::RegisterUserClass(const char* className, const char* bassClassName, const std::vector<HostFunction>& funcVec, const std::vector<ConstantData>& constantVec)
{
	TableValue* metaTable;
	if (!getTableValue(mMetaTable, (char*)className, metaTable))
	{
		metaTable = CreateTable();
		setTableValue(mMetaTable, (char*)className, metaTable);
		setTableValue(metaTable, (char*)MetaMetodString(MMT_Index), metaTable);
	}

	
	CreateFunctionTable(funcVec, metaTable);
	
	for (int i = 0; i < (int)constantVec.size(); i++)
		setTableValue(metaTable, constantVec[i].constantName.c_str(), (XInt)constantVec[i].constantValue);
	
	if (bassClassName != NULL)
	{
		Value baseTable;
		if (getTableValue(mMetaTable, ConstructValue(bassClassName), baseTable) 
			&& IsValueTable(&baseTable))
		{
			metaTable->mMetaTable = baseTable.tableData;
		}
		else
		{
			TableValue*	baseMetaTable = CreateTable();
			setTableValue(mMetaTable, (char*)bassClassName, baseMetaTable);
			setTableValue(baseMetaTable, (char*)MetaMetodString(MMT_Index), baseMetaTable);
			metaTable->mMetaTable = baseMetaTable;
		}
	}
	
	RegisterGlobalValue(className);
	Value* pValue = GetGlobalValue(className);
	UserData* userData = CreateUserData(1);
	userData->mMetaTable = metaTable;
	SetUserDataValue(pValue, userData);
	
}

// 注册宿主函数库：创建表并注册为全局变量
void	XScriptVM::RegisterHostLib(const char* libName, std::vector<HostFunction>& hostFuncVec)
{
	int index = RegisterGlobalValue(libName);
	TABLE table = newTable();
	CreateFunctionTable(hostFuncVec, table);
	setGlobalStackValue(index, ConstructValue(table));
}

// 将宿主函数列表创建为表中的函数条目
void XScriptVM::CreateFunctionTable(const std::vector<HostFunction> &hostFuncVec, TABLE table)
{
	for (int i = 0; i < (int)hostFuncVec.size(); i++)
	{
		Value fValue = CreateCHostFunction(hostFuncVec[i].pfnAddr, 0);
		setTableValue(table, ConstructValue(hostFuncVec[i].funcName.c_str()), fValue);
	}
}

// 根据变量名获取栈中的值（调试用）
Value*	XScriptVM::GetStackValueByName(int stackIndex, const std::string& name)
{
	int callIndex = mCurXScriptState->mCurCallIndex - stackIndex;
	if (callIndex >= 0 && callIndex <= mCurXScriptState->mCurCallIndex)
	{
		bool hasFound = false;
		FuncState* funcState = mCurXScriptState->mCallInfoBase[callIndex].mCurFunctionState;
		if (funcState != NULL)
		{
			int varIndex = -1;
			for (int i = 0; i < (int)funcState->m_localVarVec.size(); i++)
			{
				if (funcState->m_localVarVec[i] == name)
				{
					varIndex = i;
					break;
				}
			}

			if (varIndex >= 0 && varIndex < funcState->stackFrameSize)
			{
				int stackPos = mCurXScriptState->mCallInfoBase[callIndex].mFrameIndex + varIndex - funcState->stackFrameSize;
				return getStackValueRef(stackPos);
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			return NULL;
		}

	}
	else
	{
		return NULL;
	}
}

// 根据变量索引获取栈中的值（调试用，支持可变参数）
Value*	XScriptVM::GetStackValueByIndex(int stackIndex, int varIndex, std::string& name)
{
	int callIndex = mCurXScriptState->mCurCallIndex - stackIndex;
	if (callIndex >= 0 && callIndex <= mCurXScriptState->mCurCallIndex)
	{
		bool hasFound = false;
		FuncState* funcState = mCurXScriptState->mCallInfoBase[callIndex].mCurFunctionState;
		if (funcState->hasVarArgs && varIndex >= funcState->localParamNum - 1)
		{
			int varArgStackIndex = mCurXScriptState->mCallInfoBase[callIndex].mFrameIndex + funcState->localParamNum - 1 - funcState->stackFrameSize;

			Value value = getStackValue(varArgStackIndex);
			if (value.type == OP_TYPE_TABLE)
			{
				XInt numVarArgs = -1;
				if (getTableValue(value.tableData, "n", numVarArgs))
				{
					if (varIndex < funcState->localParamNum - 1 + numVarArgs)
					{
						hasFound = true;
						XInt argsIndex = varIndex + 1 - funcState->localParamNum;

						name = funcState->m_localVarVec[funcState->localParamNum - 1];

						return GetTableValueRef(value.tableData, ConstructValue(argsIndex));
					}
					else
					{
						varIndex = varIndex - (int)numVarArgs + 1;
					}
				}
			}
		}

		if (!hasFound)
		{
			if (varIndex >= 0 && varIndex < funcState->stackFrameSize)
			{
				int stackPos = mCurXScriptState->mCallInfoBase[callIndex].mFrameIndex + varIndex - funcState->stackFrameSize;
				name = funcState->m_localVarVec[varIndex];
				return getStackValueRef(stackPos);
			}
			else
			{
				return false;
			}

		}
	}

	
	return NULL;
	
}
// 调用调试钩子函数（传入事件类型、行号、文件名）
void	XScriptVM::CallHookFunction(int event, int curLine)
{
	Value* pValue = GetGlobalValue("_hook");
	if (pValue != NULL && pValue->type == OP_TYPE_FUNC)
	{
		push(ConstructValue((XInt)event));
		push(ConstructValue((XInt)curLine));
		push(ConstructValue(mCurXScriptState->mCurFunctionState->sourceFileName.c_str()));

		mAllowHook = false;
		CallFunction(pValue->func, 3);
		mAllowHook = true;
	}
}
