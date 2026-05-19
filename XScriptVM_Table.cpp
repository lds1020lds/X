#include "XScriptVM.h"

// 判断两个值是否相等（按类型比较）
bool    isValueEqual(const Value& value1, const Value& value2)
{
	if (value1.type == value2.type)
	{
		switch (value1.type)
		{
		case OP_TYPE_FLOAT:
		{
			return value1.fFloatValue == value2.fFloatValue;
		}
		case OP_TYPE_INT:
		{
			return  value1.iIntValue == value2.iIntValue;
		}
		case OP_TYPE_STRING:
		{
			return  value1.stringValue == value2.stringValue;
		}
		case OP_TYPE_TABLE:
		{
			return value1.tableData != NULL && value2.tableData == value1.tableData;
		}
		case OP_LIGHTUSERDATA:
		{
			return value1.lightUserData == value2.lightUserData;
		}
		case OP_TYPE_USERDATA:
		{
			return value1.userData == value2.userData;
		}
		case OP_TYPE_LIST:
		{
			return value1.listData == value2.listData;
		}
		case OP_TYPE_FUNC:
		{
			return value1.func == value2.func;
		}
		}
		return false;
	}
	else
		return false;
}

// 获取值的元方法
Value			XScriptVM::GetMetaMethod(Value* value, MetaMethodType type)
{
	TableValue* metaTable = GetMetaTable(value);

	if (metaTable != NULL)
	{
		Value result;
		getTableValue(metaTable, ConstructValue(MetaMetodString(type)), result);
		return result;
	}
	else
		return mNilValue;
}

// 获取元方法名称字符串
const char*		XScriptVM::MetaMetodString(MetaMethodType type)
{
	switch (type)
	{
	case MMT_Index:
		return "__index";
		break;
	case MMT_NewIndex:
		return "__newindex";
		break;
	case MMT_Equal:
		return "__equal";
		break;
	case MMT_Add:
		return "__add";
		break;
	case MMT_Sub:
		return "__sub";
		break;
	case MMT_Mul:
		return "__mul";
		break;
	case MMT_Div:
		return "__div";
		break;
	case MMT_Mod:
		return "__mod";
		break;
	case MMT_Pow:
		return "__pow";
		break;
	case MMT_Neg:
		return "__neg";
		break;
	case MMT_Len:
		return "__len";
		break;
	case NMT_Less:
		return "__less";
		break;
	case NMT_LessEqual:
		return "__lessequal";
		break;
	case MMT_Concat:
		return "__concat";
		break;
	case MMT_Call:
		return "__call";
		break;
	case MMT_ToString:
		return "__tostring";
		break;
	default:
		break;
	}
	return "";
}

// 获取类型名称字符串
const char* getTypeName(int type)
{
	char* str = "nil";
	if (type == OP_TYPE_INT)
	{
		str = "int";
	}
	else if (type == OP_TYPE_FLOAT)
	{
		str = "float";
	}
	else if (type == OP_TYPE_STRING)
	{
		str = "string";
	}
	else if (type == OP_TYPE_TABLE)
	{
		str = "table";
	}
	else if (type == OP_TYPE_LIST)
	{
		str = "list";
	}
	else if (type == OP_LIGHTUSERDATA)
	{
		str = "lightuserdata";
	}
	else if (type == OP_TYPE_FUNC)
	{
		str = "function";
	}
	else 	if (type == OP_TYPE_THREAD)
	{
		str = "thread";
	}
	else if (type == OP_TYPE_FUTURE)
	{
		str = "future";
	}
	else if (type == OP_TYPE_USERDATA)
	{
		str = "userdata";
	}
	return str;
}

// 获取表的下一个键值对（用于foreach迭代）
bool	XScriptVM::GetNextKey(TableValue* tableData, const Value &keyValue, Value& nextKey, Value& nextValue)
{
	XInt index = FindKeyIndex(tableData, keyValue);
	if (index < -1)
		return false;
	index++;

	if (index < tableData->mArraySize)
	{
		XSetIntValue(&nextKey, index);
		nextValue = tableData->mArrayData[index];
		return true;
	}
	else
	{
		index -= tableData->mArraySize;
		while (index < tableData->mNodeCapacity)
		{
			if (!IsValueNil(&tableData->mNodeData[index].value))
			{
				nextKey = tableData->mNodeData[index].key.keyVal;
				nextValue = tableData->mNodeData[index].value;
				return true;
			}
			else
			{
				index++;
			}
		}

		return false;

	}
}

// 查找键在表中的索引位置
XInt	XScriptVM::FindKeyIndex(TableValue* tableData, const Value& keyValue)
{
	if (IsValueNil(&keyValue))
		return -1;

	if (keyValue.type == OP_TYPE_INT && keyValue.iIntValue >= 0 && keyValue.iIntValue < tableData->mArraySize)
	{
		return keyValue.iIntValue;
	}
	else
	{
		bool found = false;
		if (tableData->mNodeCapacity > 0)
		{
			int hashPos = 0;
			GetHashPos(hashPos, tableData, &keyValue);
			TableNode* tableNode = &tableData->mNodeData[hashPos];
			while (tableNode != NULL)
			{
				if (!IsValueNil(&tableNode->value) && isValueEqual(tableNode->key.keyVal, keyValue))
				{
					return (XInt)(tableNode - tableData->mNodeData) + tableData->mArraySize;
				}
				tableNode = tableNode->key.next;
			}

		}
	}

	ExecError("invalid key for next");
	return -2;

}

static void SetTableNodeValue(TABLE table, TableNode* tableNode, const Value& value)
{
	tableNode->value = value;
	if (IsValueNil(&value))
	{
		int nodeIndex = (int)(tableNode - table->mNodeData);
		if (nodeIndex > table->lastFreePos)
		{
			table->lastFreePos = nodeIndex;
		}
	}
}

// 设置表中的键值对（内部实现）
void	XScriptVM::SetTableValue(TableValue* tableData, const Value& keyValue, const Value& value)
{
	if (IsValueNil(&keyValue))
		return;

	if (keyValue.type == OP_TYPE_INT && keyValue.iIntValue >= 0 && keyValue.iIntValue < tableData->mArraySize)
	{
		tableData->mArrayData[keyValue.iIntValue] = value;
	}
	else
	{
		bool found = false;
		if (tableData->mNodeCapacity > 0)
		{
			int hashPos = 0;
			GetHashPos(hashPos, tableData, &keyValue);
			TableNode* tableNode = &tableData->mNodeData[hashPos];
			while (tableNode != NULL)
			{
				if (isValueEqual(tableNode->key.keyVal, keyValue))
				{
					SetTableNodeValue(tableData, tableNode, value);
					found = true;
					break;
				}
				tableNode = tableNode->key.next;
			}

		}

		if (!found && !IsValueNil(&value))
			NewTableKey(tableData, &keyValue)->value = value;
	}
}

// 获取表中值的引用指针
Value*	XScriptVM::GetTableValueRef(TableValue* tableData, const Value &keyValue)
{
	if (keyValue.type == OP_TYPE_INT && keyValue.iIntValue >= 0 && keyValue.iIntValue < tableData->mArraySize)
	{
		return &tableData->mArrayData[keyValue.iIntValue];
	}
	else if (keyValue.type == OP_TYPE_NIL)
	{
		return NULL;
	}
	else
	{
		if (tableData->mNodeCapacity > 0)
		{
			int hashPos = 0;
			GetHashPos(hashPos, tableData, &keyValue);
			TableNode* tableNode = &tableData->mNodeData[hashPos];
			while (tableNode != NULL)
			{
				if (!IsValueNil(&tableNode->value) && isValueEqual(tableNode->key.keyVal, keyValue))
				{
					return &tableNode->value;
				}
				tableNode = tableNode->key.next;
			}

		}

		return NULL;
	}
}

// 获取表中的值（通过Value键）
bool XScriptVM::getTableValue(TableValue* tableData, const Value& keyValue, Value& resultValue)
{
	Value* ref = GetTableValueRef(tableData, keyValue);
	if (ref != NULL)
	{
		resultValue = *ref;
		return true;
	}

	return false;
}

// 重新哈希表：扩容并重新插入所有节点
void	XScriptVM::RehashTable(TABLE table)
{
	int nodeCapacity = table->mNodeCapacity;
	TableNode* oldTableNode = table->mNodeData;

	table->mNodeCapacity *= 2;
	table->mNodeData = new TableNode[table->mNodeCapacity];
	table->lastFreePos = table->mNodeCapacity - 1;

	for (int i = 0; i < nodeCapacity; i++)
	{
		if (!IsValueNil(&oldTableNode[i].value))
		{
			TableNode* node = NewTableKey(table, &oldTableNode[i].key.keyVal);
			node->value = oldTableNode[i].value;
		}
	}

	delete[]oldTableNode;
}

// 在表中创建新键节点（处理哈希冲突）
TableNode* XScriptVM::NewTableKey(TABLE table, const Value* key)
{
	if (table->mNodeCapacity == 0)
	{
		table->mNodeCapacity = 2;
		table->mNodeData = new TableNode[table->mNodeCapacity];

		table->lastFreePos = table->mNodeCapacity - 1;
	}

	int hashPos = 0;
	GetHashPos(hashPos, table, key);
	TableNode* mainPos = &table->mNodeData[hashPos];
	if (IsValueNil(&mainPos->value))
	{
		CopyValue(&mainPos->key.keyVal, *key);
		return mainPos;
	}
	else
	{
		TableNode* deletedNode = NULL;
		TableNode* tableNode = mainPos;
		while (tableNode != NULL)
		{
			if (isValueEqual(tableNode->key.keyVal, *key))
			{
				return tableNode;
			}
			if (deletedNode == NULL && IsValueNil(&tableNode->value))
			{
				deletedNode = tableNode;
			}
			tableNode = tableNode->key.next;
		}

		if (deletedNode != NULL)
		{
			CopyValue(&deletedNode->key.keyVal, *key);
			return deletedNode;
		}

		TableNode* freeNode = NULL;

		while (table->lastFreePos >= 0)
		{
			if (IsValueNil(&table->mNodeData[table->lastFreePos].value))
			{
				freeNode = &table->mNodeData[table->lastFreePos];
				break;
			}

			table->lastFreePos--;
		}

		if (freeNode == NULL)
		{
			RehashTable(table);

			return NewTableKey(table, key);
		}
		else
		{
			int shouldHashPos = 0;
			GetHashPos(shouldHashPos, table, &table->mNodeData[hashPos].key.keyVal);
			TableNode* otherMainPos = &table->mNodeData[shouldHashPos];
			if (shouldHashPos == hashPos)
			{
				CopyValue(&freeNode->key.keyVal, *key);
				freeNode->key.next = table->mNodeData[hashPos].key.next;
				table->mNodeData[hashPos].key.next = freeNode;
				return freeNode;
			}
			else
			{
				while (otherMainPos->key.next != mainPos)
				{
					otherMainPos = otherMainPos->key.next;
				}

				otherMainPos->key.next = freeNode;

				CopyValue(&freeNode->value, mainPos->value);
				CopyValue(&freeNode->key.keyVal, mainPos->key.keyVal);
				freeNode->key.next = mainPos->key.next;

				mainPos->key.next = NULL;
				CopyValue(&mainPos->key.keyVal, *key);

				return mainPos;

			}

		}
	}


}

// 获取环境变量值：三层查找 debugEnvTable → envTable → 全局栈
Value	XScriptVM::GetEnvValue(int index)
{
	int globalIndex = GlobalVarStackIndex(index);

	// 第一层：调试环境表（优先级最高）
	if (mEnvTable != NULL)
	{
		Value key = ConstructValue((char*)m_stringVec[GlobalVarNameIndex(index)].c_str());
		Value pValue;
		if (getTableValue(mEnvTable, key, pValue))
			return pValue;
	}
	// 第三层：全局栈
	return mGlobalStackElements[globalIndex];
}

// 写入环境变量值：三层查找写入，找到变量在哪层就写入哪层
void	XScriptVM::SetEnvValue(int index, const Value& value)
{
	// 第一层：调试环境表中已存在则更新
	if (mEnvTable != NULL)
	{
		Value key = ConstructValue((char*)m_stringVec[GlobalVarNameIndex(index)].c_str());
		Value* ref = GetTableValueRef(mEnvTable, key);
		if (ref != NULL)
		{
			*ref = value;
			return;
		}
	}

	// 第三层：变量在环境表中都不存在，写入全局栈
	int globalIndex = GlobalVarStackIndex(index);
	mGlobalStackElements[globalIndex] = value;
}

// 解析表写入操作：处理元方法 __newindex
void   XScriptVM::resolveSetTableValue(RuntimeOperator* value, const Value& tableValue)
{
	Value* table = NULL;
	Value envTableValue;
	if (value->type == ROT_UpValue_Table)
	{
		table = mCurXScriptState->mCurFunction->funcUnion.luaFunc.mUpVals[value->iStackIndex]->pValue;
	}
	else
	{
		envTableValue = ResoveStackIndexWithEnv(value->iStackIndex);
		table = &envTableValue;
	}

	Value keyValue;
	switch (value->tableIndexType)
	{
	case ROT_Int:
		keyValue.type = OP_TYPE_INT;
		keyValue.iIntValue = value->iIntTableValue;
		break;
	case ROT_Float:
		keyValue.type = OP_TYPE_FLOAT;
		keyValue.fFloatValue = value->fFloatTableValue;
		break;
	case ROT_String:
		keyValue.type = OP_TYPE_STRING;
		keyValue.stringValue = value->strTableValue;
		break;
	case ROT_Stack_Index:
		keyValue = ResoveStackIndexWithEnv((int)value->iIntTableValue);;
		break;
	case ROT_Reg:
		keyValue = mRegValue[value->iIntTableValue];
		break;
	case ROT_UpVal_Index:
		keyValue = *mCurXScriptState->mCurFunction->funcUnion.luaFunc.mUpVals[value->iIntTableValue]->pValue;
	default:
		break;
	}

	{
		Value curTable = *table;
		for (int i = 0; i < MAX_TAGMETHOD_LOOP; i++)
		{
			Value tagMethod;
			if (curTable.type == OP_TYPE_TABLE)
			{
				Value* tvalRef = GetTableValueRef(curTable.tableData, keyValue);

				if (tvalRef != NULL)
				{
					*tvalRef = tableValue;
					return;
				}
				else
				{
					tagMethod = GetMetaMethod(&curTable, MMT_NewIndex);
					if (IsValueNil(&tagMethod))
					{
						SetTableValue(curTable.tableData, keyValue, tableValue);
						return;
					}
				}
			}
			else if (curTable.type == OP_TYPE_LIST)
			{
				if (IsValueInt(&keyValue) && keyValue.iIntValue < curTable.listData->mListSize)
				{
					curTable.listData->mListData[(int)keyValue.iIntValue] = tableValue;
					return;
				}
				else
				{
					tagMethod = GetMetaMethod(&curTable, MMT_NewIndex);
					if (IsValueNil(&tagMethod))
					{
						ExecError("failed to do index operation on %s with a %s value, list overflowed!", GetOperatorName(value, table).c_str(), getTypeName(keyValue.type));
					}
				}

			}
			else
			{
				tagMethod = GetMetaMethod(&curTable, MMT_NewIndex);
				if (IsValueNil(&tagMethod))
				{
					ExecError("failed to do index operation on %s with a %s value", GetOperatorName(value, table).c_str(), getTypeName(keyValue.type));
				}
			}

		if (IsValueFunction(&tagMethod))
		{
			push(curTable);
			push(keyValue);
			push(tableValue);
			CallFunction(tagMethod.func, 3);
			return;
		}
			else
			{
				curTable = tagMethod;
			}
		}

		ExecError("too many loops in __index on %s", GetOperatorName(value, table).c_str());
	}

}


// 解析表读取操作：处理元方法 __index
void   XScriptVM::resolveTableValue(RuntimeOperator* value, Value* resultValue)
{
	Value* table = NULL;
	Value envTableValue;
	if (value->type == ROT_UpValue_Table)
	{
		table = mCurXScriptState->mCurFunction->funcUnion.luaFunc.mUpVals[value->iStackIndex]->pValue;
	}
	else
	{
		envTableValue = ResoveStackIndexWithEnv(value->iStackIndex);
		table = &envTableValue;
	}

	Value keyValue;
	switch (value->tableIndexType)
	{
	case ROT_Int:
		keyValue.type = OP_TYPE_INT;
		keyValue.iIntValue = value->iIntTableValue;
		break;
	case ROT_Float:
		keyValue.type = OP_TYPE_FLOAT;
		keyValue.fFloatValue = value->fFloatTableValue;
		break;
	case ROT_String:
		keyValue.type = OP_TYPE_STRING;
		keyValue.stringValue = value->strTableValue;
		break;
	case ROT_Stack_Index:
		keyValue = ResoveStackIndexWithEnv((int)value->iIntTableValue);;
		break;
	case ROT_Reg:
		keyValue = mRegValue[value->iIntTableValue];
		break;
	case ROT_UpVal_Index:
		keyValue = *mCurXScriptState->mCurFunction->funcUnion.luaFunc.mUpVals[value->iIntTableValue]->pValue;
	default:
		break;
	}

	Value curTable = *table;
	for (int i = 0; i < MAX_TAGMETHOD_LOOP; i++)
	{
		Value tagMethod;

		if (curTable.type == OP_TYPE_TABLE)
		{
			Value tval;

			if (getTableValue(curTable.tableData, keyValue, tval))
			{
				*resultValue = tval;
				return;
			}
			else
			{
				tagMethod = GetMetaMethod(&curTable, MMT_Index);
				if (IsValueNil(&tagMethod))
				{
					//if (IsValueList(table))
					//{
					//	ExecError("failed to do index operation on %s with a %s value", GetOperatorName(value, table).c_str(), getTypeName(keyValue.type));
					//}
					//else
					{
						XSetNilValue(resultValue);
					}

					return;
				}
			}
		}
		else if (curTable.type == OP_TYPE_STRING)
		{
			if (IsValueInt(&keyValue) && keyValue.iIntValue < stringRawLen(&curTable))
			{
				XSetIntValue(resultValue, (unsigned char)stringRawValue(&curTable)[keyValue.iIntValue]);
				return;
			}
			else
			{
				if (!getTableValue(mStringMetaTable, ConstructValue(MetaMetodString(MMT_Index)), tagMethod))
				{
					ExecError("failed to do index operation on %s with a %s value", GetOperatorName(value, table).c_str(), getTypeName(keyValue.type));
				}
			}
		}
		else if (curTable.type == OP_TYPE_LIST)
		{
			if (IsValueInt(&keyValue) && keyValue.iIntValue < curTable.listData->mListSize)
			{
				*resultValue = curTable.listData->mListData[(int)keyValue.iIntValue];
				return;
			}
			else
			{
				tagMethod = GetMetaMethod(&curTable, MMT_Index);
				if (IsValueNil(&tagMethod))
				{
					ExecError("failed to do index operation on %s with a %s value", GetOperatorName(value, table).c_str(), getTypeName(keyValue.type));
				}
			}
		}
		else
		{
			tagMethod = GetMetaMethod(&curTable, MMT_Index);
			if (IsValueNil(&tagMethod))
			{
				ExecError("failed to do index operation on %s with a %s value", GetOperatorName(value, table).c_str(), getTypeName(keyValue.type));
			}
		}

		if (IsValueFunction(&tagMethod))
		{
			push(curTable);
			push(keyValue);
			CallFunction(tagMethod.func, 2);
			*resultValue = mRegValue[0];
			return;
		}
		else
		{
			curTable = tagMethod;
		}
	}

	ExecError("too many loops in __index");
	resultValue->type = OP_TYPE_NIL;
	resultValue->iIntValue = 0;
}

// 根据类名和函数名查找宿主类方法（支持继承链查找）
HostFunction* XScriptVM::getClassFuncByName(const std::string& className, const std::string& funcName)
{
	HostFunction* function = NULL;

	std::string searchClassName = className;
	while (!searchClassName.empty() && mUserClassDataMap.find(searchClassName) != mUserClassDataMap.end())
	{
		UserClassData& userClassData = mUserClassDataMap[searchClassName];
		for (int i = 0; i < (int)userClassData.mHostFunctions.size(); i++)
		{
			if (userClassData.mHostFunctions[i].funcName == funcName)
			{
				return &userClassData.mHostFunctions[i];
			}

		}
		searchClassName = userClassData.mBassClassName;
	}

	return NULL;
}
