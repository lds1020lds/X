#include "XScriptVM.h"

extern XInt stringUsedMem;

// 创建列表对象（带默认容量和元表）
ListValue*		XScriptVM::CreateList()
{
	ListValue* table = new ListValue();
	table->mListData = new Value[LIST_DEFAULT_SIZE];
	table->mCapacity = LIST_DEFAULT_SIZE;
	table->mMetaTable = mListMetaTable;

	GC(table)->next = mRootCG;
	mRootCG = GC(table);
	GC(table)->type = OP_TYPE_LIST;
	GC(table)->marked = MS_White;
	return table;
}

// 创建表对象（加入GC链表）
TableValue*		XScriptVM::CreateTable()
{
	TableValue* table = new TableValue();
	table->mMetaTable = mTableMetaTable;
	GC(table)->next = mRootCG;
	mRootCG = GC(table);
	GC(table)->type = OP_TYPE_TABLE;
	GC(table)->marked = MS_White;
	return table;
}

// 创建C函数闭包（带上值）
Function*		XScriptVM::CreateCFunction(int numUpvals)
{
	Function* table = new Function();
	memset(&table->funcUnion, 0, sizeof(FuncUnion));
	GC(table)->next = mRootCG;
	mRootCG = GC(table);
	GC(table)->type = OP_TYPE_FUNC;
	GC(table)->marked = MS_White;

	table->isCFunc = true;
	table->funcUnion.cFunc.mNumUpVal = numUpvals;
	if (numUpvals > 0)
		table->funcUnion.cFunc.mUpVal = new Value[numUpvals];
	return table;
}


// 创建用户数据对象
UserData*		XScriptVM::CreateUserData(int size)
{
	if (size < 1)
		size = 1;
	int realSize = sizeof(UserData) + size - 1;
	UserData* userData = (UserData*)malloc(realSize);
	userData->mSize = size;
	userData->mMetaTable = NULL;
	GC(userData)->next = mRootCG;
	mRootCG = GC(userData);
	GC(userData)->type = OP_TYPE_USERDATA;
	GC(userData)->marked = MS_White;
	return userData;
}

// 创建函数对象（加入GC链表）
Function*		XScriptVM::CreateFunction()
{
	Function* table = new Function();
	memset(&table->funcUnion, 0, sizeof(FuncUnion));
	GC(table)->next = mRootCG;
	mRootCG = GC(table);
	GC(table)->type = OP_TYPE_FUNC;
	GC(table)->marked = MS_White;
	return table;
}

// 创建函数原型（加入GC链表，标记为固定不回收）
FuncState*		XScriptVM::CreateFunctionState()
{
	FuncState* table = new FuncState();
	GC(table)->next = mRootCG;
	mRootCG = GC(table);
	GC(table)->type = OP_TYPE_PROTO;
	GC(table)->marked = MS_Fixed;
	return table;
}

// 创建上值对象（加入GC链表）
UpValue*		XScriptVM::CreateUpVal()
{
	UpValue* table = new UpValue();
	GC(table)->next = mRootCG;
	mRootCG = GC(table);
	GC(table)->type = OP_TYPE_UPVAL;
	GC(table)->marked = MS_Fixed;
	return table;
}

// 创建Future对象（加入GC链表）
XFuture*		XScriptVM::CreateFuture()
{
	XFuture* future = new XFuture();
	GC(future)->next = mRootCG;
	mRootCG = GC(future);
	GC(future)->type = OP_TYPE_FUTURE;
	GC(future)->marked = MS_White;
	return future;
}

// 创建Promise写端，同时创建其对应的Future读端
XPromise*		XScriptVM::CreatePromise()
{
	return new XPromise(this, CreateFuture());
}

void XScriptVM::ResolveFuture(XFuture* future, const Value& value)
{
	if (future == NULL || future->state != XFuture::Pending)
		return;

	future->state = XFuture::Fulfilled;
	future->result = value;
	future->ownerState = NULL;

	for (int i = 0; i < (int)future->waiters.size(); i++)
	{
		XScriptState* state = future->waiters[i];
		if (state != NULL && state->mSuspendReason == SUSPEND_AWAIT && state->mWaitingFuture == future)
		{
			state->mWaitingFuture = NULL;
			AsyncReady(state, value, false);
		}
	}
	future->waiters.clear();

	std::vector<std::function<void(XScriptVM*, XFuture::State, const Value&)> > callbacks = future->callbacks;
	future->callbacks.clear();
	for (int i = 0; i < (int)callbacks.size(); i++)
	{
		callbacks[i](this, XFuture::Fulfilled, value);
	}
}

void XScriptVM::RejectFuture(XFuture* future, const Value& value)
{
	if (future == NULL || future->state != XFuture::Pending)
		return;

	future->state = XFuture::Rejected;
	future->result = value;
	future->ownerState = NULL;

	for (int i = 0; i < (int)future->waiters.size(); i++)
	{
		XScriptState* state = future->waiters[i];
		if (state != NULL && state->mSuspendReason == SUSPEND_AWAIT && state->mWaitingFuture == future)
		{
			state->mWaitingFuture = NULL;
			AsyncReady(state, value, true);
		}
	}
	future->waiters.clear();

	std::vector<std::function<void(XScriptVM*, XFuture::State, const Value&)> > callbacks = future->callbacks;
	future->callbacks.clear();
	for (int i = 0; i < (int)callbacks.size(); i++)
	{
		callbacks[i](this, XFuture::Rejected, value);
	}
}

void XPromise::Resolve(const Value& value)
{
	if (vm != NULL)
		vm->ResolveFuture(future, value);
}

void XPromise::Reject(const Value& value)
{
	if (vm != NULL)
		vm->RejectFuture(future, value);
}

// GC标记阶段：标记函数原型及其子函数
void	XScriptVM::MarkProto(FuncState* func)
{
	GC_SetBlack(func);

	for (auto& d : func->mSwitchVec)
	{
		for (auto& keyval : d.strInstrMap)
		{
			GC_SetBlack(keyval.first);
		}
	}

	for (int i = 0; i < (int)func->m_subFuncVec.size(); i++)
	{
		MarkProto(func->m_subFuncVec[i]);
	}
}

void XScriptVM::MarkScriptState(XScriptState* state)
{
	if (state == NULL)
		return;

	for (int i = 0; i < state->mTopIndex; i++)
	{
		MarkValue(&state->mStackElements[i]);
	}

	if (state->mStartFunction != NULL)
	{
		Value funcValue;
		funcValue.type = OP_TYPE_FUNC;
		funcValue.func = state->mStartFunction;
		MarkValue(&funcValue);
	}
	if (state->mCurFunction != NULL)
	{
		Value funcValue;
		funcValue.type = OP_TYPE_FUNC;
		funcValue.func = state->mCurFunction;
		MarkValue(&funcValue);
	}
}

// GC标记阶段：根据值类型递归标记引用的对象
void	XScriptVM::MarkValue(Value* value)
{
	switch (value->type)
	{
	case OP_TYPE_USERDATA:
	{
		GC_SetBlack(value->userData);
	}
	break;
	case OP_TYPE_STRING:
	{
		GC_SetBlack(value->stringValue);
	}
	break;
	case OP_TYPE_LIST:
	{
		GC_SetBlack(value->listData);
		for (int i = 0; i < value->listData->mListSize; i++)
		{
			MarkValue(&value->listData->mListData[i]);
		}
	}
	break;
	case OP_TYPE_TABLE:
	{
		MarkTable(value->tableData);
	}
	break;

	case OP_TYPE_FUTURE:
	{
		if (value->futureData != NULL && value->futureData->marked != MS_Black)
		{
			GC_SetBlack(value->futureData);
			MarkValue(&value->futureData->result);
			if (value->futureData->ownerState != NULL)
			{
				MarkScriptState(value->futureData->ownerState);
			}
			for (int i = 0; i < (int)value->futureData->waiters.size(); i++)
			{
				MarkScriptState(value->futureData->waiters[i]);
			}
		}
	}
	break;
	case OP_TYPE_FUNC:
	{
		GC_SetBlack(value->func);
		if (!value->func->isCFunc)
		{
			MarkProto(value->func->funcUnion.luaFunc.proto);

			for (int i = 0; i < value->func->funcUnion.luaFunc.mNumUpVals; i++)
			{
				GC_SetBlack(value->func->funcUnion.luaFunc.mUpVals[i]);
			}
		}
		else
		{
			for (int i = 0; i < value->func->funcUnion.cFunc.mNumUpVal; i++)
			{
				MarkValue(&value->func->funcUnion.cFunc.mUpVal[i]);
			}
		}

		break;
	}
	case OP_TYPE_THREAD:
	{
		GC_SetBlack(value->threadData);
		for (int i = MAX_GLOBAL_DATASIZE; i < value->threadData->mTopIndex; i++)
		{
			MarkValue(&value->threadData->mStackElements[i]);
		}

		GC_SetBlack(value->threadData->mStartFunction);
	}
	break;
	}
}

// GC标记阶段：标记表及其所有键值对
void XScriptVM::MarkTable(TABLE table)
{
	if (table->marked == MS_Black)
		return;

	GC_SetBlack(table);
	for (int i = 0; i < table->mArraySize; i++)
	{
		MarkValue(&table->mArrayData[i]);
	}

	for (int i = 0; i < table->mNodeCapacity; i++)
	{
		if (!IsValueNil(&table->mNodeData[i].value))
		{
			MarkValue(&table->mNodeData[i].value);
		}

		if (!IsValueNil(&table->mNodeData[i].key.keyVal))
		{
			MarkValue(&table->mNodeData[i].key.keyVal);
		}
	}
}

// GC标记阶段：从根集开始标记所有可达对象
void	XScriptVM::MarkObjects()
{
	std::map<std::string, int>::iterator it = mGlobalVarMap.begin();
	for (; it != mGlobalVarMap.end(); it++)
	{
		MarkValue(&mGlobalStackElements[it->second]);
	}

	for (int i = 0; i < mMainXScriptState.mTopIndex; i++)
	{
		MarkValue(&mMainXScriptState.mStackElements[i]);
	}
	if (mEnvTable != NULL)
		MarkTable(mEnvTable);
	if (mModuleTable != NULL)
		MarkTable(mModuleTable);
	if (mMetaTable != NULL)
		MarkTable(mMetaTable);
	if (mTableMetaTable != NULL)
		MarkTable(mTableMetaTable);
	if (mListMetaTable != NULL)
		MarkTable(mListMetaTable);
	if (mStringMetaTable != NULL)
		MarkTable(mStringMetaTable);

	for (int i = 0; i < (int)mPendingPromises.size(); i++)
	{
		if (mPendingPromises[i] != NULL && mPendingPromises[i]->future != NULL)
		{
			Value futureValue = ConstructValue(mPendingPromises[i]->future);
			MarkValue(&futureValue);
		}
	}

	for (int i = 0; i < (int)mSpawnedFutures.size(); i++)
	{
		if (mSpawnedFutures[i] != NULL)
		{
			Value futureValue = ConstructValue(mSpawnedFutures[i]);
			MarkValue(&futureValue);
		}
	}

	std::queue<XScriptState*> readyQueue = mAsyncReadyQueue;
	while (!readyQueue.empty())
	{
		MarkScriptState(readyQueue.front());
		readyQueue.pop();
	}

	// 扫描调试 evaluate 期间暂存的寄存器值，防止被 GC 回收
	if (mSavedRegDepth > 0)
	{
		for (int i = 0; i < MAX_FUNC_REG; i++)
		{
			MarkValue(&mSavedRegValues[i]);
		}
	}

	for (auto& d: mSavedRegValueStack)
	{
		MarkValue(&d);
	}
}

// 释放GC对象内存（根据类型分别处理）
void	XScriptVM::FreeObject(CGObject* obj)
{
	switch (obj->type)
	{
	case OP_TYPE_STRING:
	{
		char* str = (char*)obj;
		delete str;
	}
	break;
	case OP_TYPE_LIST:
	{
		ListValue* list = (ListValue*)obj;
		delete []list->mListData;
		delete list;
	}
	break;
	case OP_TYPE_USERDATA:
	{
		char* userData = (char*)obj;
		free(userData);
	}
	break;
	case OP_TYPE_TABLE:
	{
		TableValue* table = (TableValue*)obj;
		if (table->mArrayData != NULL)
			delete[] table->mArrayData;

		if (table->mNodeData != NULL)
			delete[]table->mNodeData;
	}
	break;
	case OP_TYPE_UPVAL:
	{
		UpValue* p = (UpValue*)obj;
		delete p;
	}
	break;
	case OP_TYPE_PROTO:
	{
		FuncState* funcState = (FuncState*)obj;
		delete[] funcState->mInstrStream.instrs;
		delete funcState;
	}
	break;
	case OP_TYPE_FUTURE:
	{
		XFuture* future = (XFuture*)obj;
		delete future;
	}
	break;
	case OP_TYPE_FUNC:
	{
		Function* func = (Function*)obj;
		if (func->isCFunc)
		{
			delete[]func->funcUnion.cFunc.mUpVal;
		}
		else
		{
			delete[]func->funcUnion.luaFunc.mUpVals;
		}
		delete func;
		func = NULL;
	}
	break;
	case OP_TYPE_THREAD:
	{
		XScriptState* xscriptState = (XScriptState*)obj;

		delete[]xscriptState->mStackElements;
		delete[]xscriptState->mCallInfoBase;

	}
	break;
	default:
		break;
	}
}

// 判断上值是否仍然开放（引用栈上的值）
static bool	IsUpValOpen(CGObject* obj)
{
	UpValue* p = (UpValue*)obj;
	return (p->pValue != &p->value);
}

static XInt sFreeStringMem = 0;

// GC清除阶段：回收未标记的对象和字符串
void	XScriptVM::SweepObjects()
{
	CGObject* lastObj = NULL;
	CGObject*	gcObj = mRootCG;
	while (gcObj != NULL)
	{
		if (gcObj->marked != MS_White || (gcObj->type == OP_TYPE_UPVAL && IsUpValOpen(gcObj)))
		{
			GC_SetWhite(gcObj);

			lastObj = gcObj;
			gcObj = gcObj->next;
		}
		else
		{
			CGObject* p = gcObj->next;
			FreeObject(gcObj);
			if (lastObj != NULL)
			{
				lastObj->next = p;
			}
			else
			{
				mRootCG = p;
			}

			gcObj = p;
		}
	}

	for (int i = 0; i < mStringHashSize; i++)
	{
		XString* first = mStringHashTable[i];
		XString* preStr = NULL;
		while (first != NULL)
		{
			if (first->marked != MS_White)
			{
				preStr = first;
				first = (XString*)first->next;
			}
			else
			{
				int memLen = sizeof(XString) + (first->len + 1) * sizeof(char);

				stringUsedMem -= memLen;
				sFreeStringMem += memLen;
				mStringHashUsedSize--;
				if (preStr == NULL)
				{
					XString* old = first;
					first = (XString*)first->next;
					FreeObject((CGObject*)old);

					mStringHashTable[i] = first;
				}
				else
				{
					preStr->next = first->next;
					XString* old = first;
					first = (XString*)first->next;
					FreeObject((CGObject*)old);				
				}
			}
		}
	}

}

// 执行垃圾回收：标记-清除算法
void	XScriptVM::GarbageCollect()
{
	MarkObjects();
	SweepObjects();
}
