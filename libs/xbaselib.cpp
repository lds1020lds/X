#include "XScriptVM.h"
#include "xbaselib.h"
#include "Xutility.h"
#include<ctime>
#include <AccCtrl.h>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>
void init_base_lib()
{
	gScriptVM.RegisterHostApi("system_pause",	host_pause);
	gScriptVM.RegisterHostApi("sleep",			host_sleep);
	gScriptVM.RegisterHostApi("run_until_complete", host_runUntilComplete);
	gScriptVM.RegisterHostApi("async_tick", host_asyncTick);
	gScriptVM.RegisterHostApi("run_for", host_runFor);
	gScriptVM.RegisterHostApi("spawn", host_spawn);
	gScriptVM.RegisterHostApi("future_done", host_futureDone);
	gScriptVM.RegisterHostApi("future_result", host_futureResult);
	gScriptVM.RegisterHostApi("future_state", host_futureState);
	gScriptVM.RegisterHostApi("next_tick", host_nextTick);
	gScriptVM.RegisterHostApi("set_timeout", host_setTimeout);
	gScriptVM.RegisterHostApi("exec_async", host_execAsync);
	gScriptVM.RegisterHostApi("all", host_futureAll);
	gScriptVM.RegisterHostApi("race", host_futureRace);
	gScriptVM.RegisterHostApi("timeout", host_futureTimeout);
	gScriptVM.RegisterHostApi("type",			host_type);
	gScriptVM.RegisterHostApi("toString",		host_toString);
	gScriptVM.RegisterHostApi("toNumber",		host_toNumber);
	gScriptVM.RegisterHostApi("array",			host_array);
	gScriptVM.RegisterHostApi("printf",			 host_print);
	gScriptVM.RegisterHostApi("prints",			host_prints);
	gScriptVM.RegisterHostApi("messageBox", xbase_messageBox);
	gScriptVM.RegisterHostApi("getCurrentTime",  host_GetCurrentTime);

	gScriptVM.RegisterHostApi("ipairs",			 host_ipairs, host_inext);


	gScriptVM.RegisterHostApi("lenum", list_enum, list_next);

	gScriptVM.RegisterHostApi("gettable",  host_gettable);
	gScriptVM.RegisterHostApi("settable",  host_settable);
	gScriptVM.RegisterHostApi("setglobal", host_setglobal);
	gScriptVM.RegisterHostApi("getglobal", host_getglobal);
	gScriptVM.RegisterHostApi("hasglobal", host_hasglobal);
	gScriptVM.RegisterHostApi("delglobal", host_delglobal);
	gScriptVM.RegisterHostApi("globals", host_globals);

	gScriptVM.RegisterHostApi("garbageCollect", host_gc);

	gScriptVM.RegisterHostApi("require", host_require);
	gScriptVM.RegisterHostApi("pcall",	 host_pcall);
	gScriptVM.RegisterHostApi("xpcall",	 host_xpcall);

	gScriptVM.RegisterHostApi("setenvtable",  host_setEnvTable);
	gScriptVM.RegisterHostApi("getenvtable",  host_getEnvTable);
	gScriptVM.RegisterHostApi("loadstring", host_loadstring);
	gScriptVM.RegisterHostApi("loadfile", host_loadfile);

	gScriptVM.RegisterHostApi("setmetatable", host_setmetadata);
	gScriptVM.RegisterHostApi("getmetatable", host_getmetadata);

	std::vector<HostFunction> coVec;

	coVec.push_back(HostFunction("create", host_coCreate));
	coVec.push_back(HostFunction("resume", host_coResume));
	coVec.push_back(HostFunction("yield",  host_coYield));
	coVec.push_back(HostFunction("status", host_coStatus));
	coVec.push_back(HostFunction("wrap", host_coWrapCreate));

	gScriptVM.RegisterHostLib("coroutine", coVec);

	std::vector<HostFunction> structVec;

	structVec.push_back(HostFunction("pack", xstruct_pack));
	structVec.push_back(HostFunction("unpack", xstruct_unpack));
	structVec.push_back(HostFunction("calFormatSize", xstruct_calFormatSize));

	gScriptVM.RegisterHostLib("struct", structVec);
}

void host_coWrapResume(XScriptVM* vm)
{
	Function* func = vm->GetCurCFunction();
	XScriptState* threadData = func->funcUnion.cFunc.mUpVal[0].threadData;
	vm->ResumeCoroutie(threadData, 0);
}

void host_coWrapCreate(XScriptVM* vm)
{
	Value value = vm->getParamValue(0);
	if (IsValueLuaFunction(&value))
	{
		XScriptState* threadData = vm->CreateCoroutie(value.func, ECoroutineType::Normal);
		Function* func = vm->CreateCFunction(1);
		func->funcUnion.cFunc.mUpVal[0] = vm->ConstructValue(threadData);
		func->funcUnion.cFunc.pfnAddr = host_coWrapResume;
		Value funcValue;
		funcValue.func = func;
		funcValue.type = OP_TYPE_FUNC;
		vm->setReturnAsValue(funcValue, 0);
	}
	else
	{
		vm->setReturnAsNil(0);
	}
}

void host_coStatus(XScriptVM* vm)
{
	Value stackValue = vm->getParamValue(0);
	//ExecArgsCheck(IsValueThread(stackValue), 0, "coroutine expect");

	if (IsValueThread(&stackValue))
	{
		const char* status = vm->GetCoroutieStatusName(stackValue.threadData);
		vm->setReturnAsStr(status);
	}
}

void host_coCreate(XScriptVM* vm)
{
	Value value = vm->getParamValue(0);
	if (IsValueLuaFunction(&value))
	{
		XScriptState* xsState = vm->CreateCoroutie(value.func, ECoroutineType::Normal);
		vm->setReturnAsValue(vm->ConstructValue(xsState));
	}
	else
	{
		vm->setReturnAsNil(0);
	}
	
}

void host_coResume(XScriptVM* vm)
{
	Value value = vm->getParamValue(0);
	if (IsValueThread(&value))
	{
		vm->ResumeCoroutie(value.threadData, 1);
	}
	
}

void host_coYield(XScriptVM* vm)
{
	vm->YieldCoroutie();
}

void host_setEnvTable(XScriptVM* vm)
{
	Value stackValue = vm->getParamValue(0);
	if ( IsValueTable(&stackValue) )
	{
		vm->SetEnvTable(stackValue.tableData);
	}
	else if (IsValueNil(&stackValue))
	{
		vm->SetEnvTable(NULL);
	}
}

void host_getEnvTable(XScriptVM* vm)
{
	TABLE table = vm->GetEnvTable();
	vm->setReturnAsTable(table);
}

void host_loadstring(XScriptVM* vm)
{
	char* code = NULL;
	if (vm->getParamAsString(0, code))
	{
		FuncState* func = vm->CompileString(code);
		if (func != NULL)
		{
			vm->setReturnAsInt(1);
			vm->setReturnAsValue(vm->ConstructValue(func), 1);
		}
		else
		{
			vm->setReturnAsInt(0);
		}
	}
	else
	{
		vm->setReturnAsInt(0);
	}
}

void host_loadfile(XScriptVM* vm)
{
	char* fileName = NULL;
	if (vm->getParamAsString(0, fileName))
	{
		FuncState* func = vm->CompileFile(fileName);
		if (func != NULL)
		{
			vm->setReturnAsInt(1);
			vm->setReturnAsValue(vm->ConstructValue(func), 1);
		}
		else
		{
			vm->setReturnAsInt(0);
		}
	}
	else
	{
		vm->setReturnAsInt(0);
	}
}

std::string  getValueDescString(XScriptVM* vm, const Value& value)
{
	std::string desc;
	char text[256] = { 0 };
	if (value.type == OP_TYPE_INT)
	{
		snprintf(text, 256, XIntConFmt, value.iIntValue);
		desc = text;
	}
	else if (value.type == OP_TYPE_FLOAT)
	{
		snprintf(text, 256, XFloatConFmt, value.fFloatValue);
		desc = text;
	}
	else if (value.type == OP_TYPE_STRING)
	{
		desc = "\"";
		desc += stringRawValue(&value) ;
		desc += "\"";
	}
	else if (IsValueFunction(&value))
	{
		if (value.func->isCFunc)
		{
			snprintf(text, 256, "C function: 0x%p", value.func->funcUnion.cFunc.pfnAddr);
		}
		else
		{
			snprintf(text, 256, "lua function: %s", value.func->funcUnion.luaFunc.proto->funcName.c_str());
		}
		desc = text;
	}
	else if (IsUserType(value.type))
	{
		std::string localType = vm->GetString(UserDataType(value.type));
		snprintf(text, 256, "User Type: %s, 0x%p", localType.c_str(), value.lightUserData);
		desc = text;
	}
	else if (value.type == OP_TYPE_LIST)
	{
		desc += "[";
		for (int i = 0; i < (int)value.listData->mListSize; i++)
		{
			desc += getValueDescString(vm, value.listData->mListData[i]);
			if (i < value.listData->mListSize - 1)
			{
				desc += ",";
			}
		}
		desc += "]";
	}
	else if (value.type == OP_TYPE_TABLE)
	{
		desc += "{";
		
		for (int i = 0; i < value.tableData->mArraySize; i++)
		{
			desc += ConvertToString(i);
			desc += "=";

			desc += getValueDescString(vm, value.tableData->mArrayData[i]);
			if (i < value.tableData->mArraySize - 1 || value.tableData->mNodeCapacity > 0)
			{
				desc += ",";
			}
		}

		for (int i = 0; i < value.tableData->mNodeCapacity; i++)
		{
			if (!IsValueNil(&value.tableData->mNodeData[i].key.keyVal))
			{
				desc += getValueDescString(vm, value.tableData->mNodeData[i].key.keyVal);
				desc += "=";
				desc += getValueDescString(vm, value.tableData->mNodeData[i].value);

				if (i < value.tableData->mNodeCapacity - 1)
					desc += ",";
			}
			
		}

		desc += "}";
	}
	else if (value.type == OP_TYPE_NIL)
	{
		desc = "nil";
	}
	else if (value.type == OP_TYPE_THREAD)
	{
		desc = "Coroutie Value";
	}
	else if (value.type == OP_TYPE_FUTURE)
	{
		desc = "Future Value";
	}
	return desc;
}



void host_pcall(XScriptVM* vm)
{
	int numParam = vm->getNumParam();
	Value fValue = vm->getParamValue(0);
	if (numParam > 0 &&  IsValueFunction(&fValue))
	{
		for (int i = 1; i < numParam; i++)
		{
			vm->push(vm->getParamValue(i));
		}
		std::string errorDesc;
		int ret = vm->ProtectCallFunction(fValue.func, numParam - 1, errorDesc);
		if (ret == 0)
		{
			for (int i = MAX_FUNC_REG - 2; i >= 0; i--)
			{
				//函数返回值传递
				vm->setReturnAsValue(*vm->getReturnRegValue(i),  i + 1);
			}
		}
		else
		{
			vm->setReturnAsStr(errorDesc.c_str(), 1);
		}

		vm->setReturnAsInt(ret, 0);
	}
}

void host_xpcall(XScriptVM* vm)
{
	Value fValue1 = vm->getParamValue(0);
	Value fValue2 = vm->getParamValue(1);

	if ( IsValueFunction(&fValue1) && IsValueFunction(&fValue2))
	{
		std::string errorDesc;
		int errorFuncIndex = vm->getParamStackIndex(1);
		int ret = vm->ProtectCallFunction(fValue1.func, 0, errorDesc, errorFuncIndex);
		vm->setReturnAsInt(ret, 0);
		vm->setReturnAsStr(errorDesc.c_str(), 1);
	}

	//vm->xpcall();
}

void host_require(XScriptVM* vm)
{
	char* moudleName = NULL;
	if (vm->getParamAsString(0, moudleName))
	{
		Value result = vm->RequireModule(moudleName);
		vm->setReturnAsValue(result, 0);
	}
	
}

void host_settable(XScriptVM* vm)
{
	TABLE table;
	vm->getParamAsTable(0, table);
	Value key = vm->getParamValue(1);
	Value value = vm->getParamValue(2);
	vm->setTableValue(table, key, value);
}

void host_gettable(XScriptVM* vm)
{
	TABLE table;
	vm->getParamAsTable(0, table);
	Value key = vm->getParamValue(1);
	Value ret;
	vm->getTableValue(table, key, ret);
	vm->setReturnAsValue(ret);
}

void host_setglobal(XScriptVM* vm)
{
	CheckParam(setglobal, 0, name, OP_TYPE_STRING);
	Value value = vm->getParamValue(1);
	int index = vm->RegisterGlobalValue(stringRawValue(&name));
	vm->setGlobalStackValue(index, value);
}

void host_getglobal(XScriptVM* vm)
{
	CheckParam(getglobal, 0, name, OP_TYPE_STRING);
	Value* value = vm->GetGlobalValue(stringRawValue(&name));
	if (value != NULL)
	{
		vm->setReturnAsValue(*value);
	}
	else
	{
		vm->setReturnAsNil(0);
	}
}

void host_hasglobal(XScriptVM* vm)
{
	CheckParam(hasglobal, 0, name, OP_TYPE_STRING);
	Value* value = vm->GetGlobalValue(stringRawValue(&name));
	vm->setReturnAsInt(value != NULL && !IsValueNil(value) ? 1 : 0);
}

void host_delglobal(XScriptVM* vm)
{
	CheckParam(delglobal, 0, name, OP_TYPE_STRING);
	Value* value = vm->GetGlobalValue(stringRawValue(&name));
	if (value != NULL)
	{
		XSetNilValue(value);
	}
}

void host_globals(XScriptVM* vm)
{
	TABLE table = vm->newTable();
	const std::map<std::string, int>& globalMap = vm->GetGlobalVarMap();
	std::map<std::string, int>::const_iterator it = globalMap.begin();
	for (; it != globalMap.end(); it++)
	{
		Value* value = vm->GetGlobalValue(it->first);
		if (value != NULL && !IsValueNil(value))
		{
			vm->setTableValue(table, vm->ConstructValue(it->first.c_str()), *value);
		}
	}
	vm->setReturnAsTable(table);
}

void list_next(XScriptVM* vm)
{
	CheckParam(lnext, 0, list, OP_TYPE_LIST);
	CheckParam(lnext, 1, index, OP_TYPE_INT);

	XInt iIndex = index.iIntValue;
	iIndex++;
	if (iIndex < list.listData->mListSize)
	{
		vm->setReturnAsInt(iIndex, 0);
		vm->setReturnAsValue(list.listData->mListData[iIndex], 1);
	}
	else
	{
		vm->setReturnAsNil(0);
	}
}

void list_enum(XScriptVM* vm)
{
	CheckParam(lenum, 0, list, OP_TYPE_LIST);
	Function* func = vm->GetCurCFunction();
	vm->setReturnAsValue(func->funcUnion.cFunc.mUpVal[0]);
	vm->setReturnAsValue(list, 1);
	vm->setReturnAsInt(-1, 2);
}

void host_inext(XScriptVM* vm)
{
	CheckParam(ipairs, 0, table, OP_TYPE_TABLE);
	Value key = vm->getParamValue(1);
	Value nextKey, nextValue;
	if (vm->GetNextKey(table.tableData, key, nextKey, nextValue))
	{
		vm->setReturnAsValue(nextKey, 0);
		vm->setReturnAsValue(nextValue, 1);
	}
	else
	{
		vm->setReturnAsNil(0);
	}
}

void host_ipairs(XScriptVM* vm)
{
	CheckParam(ipairs, 0, table, OP_TYPE_TABLE);
	Function* func = vm->GetCurCFunction();
	vm->setReturnAsValue(func->funcUnion.cFunc.mUpVal[0]);
	vm->setReturnAsValue(table, 1);
	vm->setReturnAsNil(2);
}

void host_prints(XScriptVM* vm)
{
	char* str = NULL;
	if (vm->getParamAsString(0, str))
	{
		printf(str);
		printf("\n");
	}
}


void xbase_messageBox(XScriptVM* vm)
{
	CheckParam(messageBox, 0, msg, OP_TYPE_STRING);
	MessageBox(NULL, stringRawValue(&msg), "锟斤拷示", 0);
}

void  host_print(XScriptVM* vm)
{
	std::string desc;
	int numParam = vm->getNumParam();
	for (int i = 0; i < numParam; i++)
	{
		if (i > 0)
			desc += "\t";

		desc += getValueDescString(vm, vm->getParamValue(i));
	}
	desc += "\n";
	printf(desc.c_str());
}

void  host_GetCurrentTime(XScriptVM* vm)
{
	XFloat fTime = clock() * 1.0f / CLOCKS_PER_SEC;
	vm->setReturnAsfloat(fTime);
}


void host_sleep(XScriptVM* vm)
{
	XInt spleepTime = 0;
	vm->getParamAsInt(0, spleepTime);
	if (vm->IsCurrentAsyncTask())
	{
		XPromise* promise = vm->CreatePromise();
		vm->AddAsyncTimer((int)spleepTime, promise);
		vm->setReturnAsValue(vm->ConstructValue(promise->future));
		return;
	}
	::Sleep((int)spleepTime);
}

void host_runUntilComplete(XScriptVM* vm)
{
	Value future = vm->getParamValue(0);
	if (!IsValueFuture(&future))
	{
		vm->setReturnAsNil(0);
		return;
	}
	vm->RunUntilComplete(future.futureData);
	if (future.futureData != NULL && future.futureData->IsDone())
	{
		vm->setReturnAsValue(future.futureData->result);
	}
	else if (future.futureData != NULL && future.futureData->IsRejected())
	{
		vm->ScriptThrow(future.futureData->result);
	}
	else
	{
		vm->setReturnAsNil(0);
	}
}

void host_asyncTick(XScriptVM* vm)
{
	vm->AsyncTick();
}

void host_runFor(XScriptVM* vm)
{
	CheckParam(run_for, 0, ms, OP_TYPE_INT);
	unsigned long long endTime = GetTickCount64() + (unsigned long long)(ms.iIntValue < 0 ? 0 : ms.iIntValue);
	do
	{
		vm->AsyncTick();
		Sleep(1);
	} while (GetTickCount64() < endTime);
}

void host_spawn(XScriptVM* vm)
{
	Value future = vm->getParamValue(0);
	if (IsValueFuture(&future) && future.futureData != NULL)
	{
		vm->SpawnFuture(future.futureData);
		vm->setReturnAsValue(future);
	}
	else
	{
		vm->setReturnAsNil(0);
	}
}

void host_futureDone(XScriptVM* vm)
{
	Value future = vm->getParamValue(0);
	vm->setReturnAsInt((IsValueFuture(&future) && future.futureData != NULL && future.futureData->IsCompleted()) ? 1 : 0);
}

void host_futureResult(XScriptVM* vm)
{
	Value future = vm->getParamValue(0);
	if (IsValueFuture(&future) && future.futureData != NULL && future.futureData->IsDone())
		vm->setReturnAsValue(future.futureData->result);
	else
		vm->setReturnAsNil(0);
}

void host_futureState(XScriptVM* vm)
{
	Value future = vm->getParamValue(0);
	if (!IsValueFuture(&future) || future.futureData == NULL)
	{
		vm->setReturnAsStr("invalid");
		return;
	}
	vm->setReturnAsStr(future.futureData->IsDone() ? "fulfilled" : (future.futureData->IsRejected() ? "rejected" : "pending"));
}

void host_nextTick(XScriptVM* vm)
{
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));
	vm->QueueAsyncCompletion([promise](XScriptVM* owner) {
		promise->Resolve(Value());
		owner->RemovePendingPromise(promise);
		delete promise;
	});
}

void host_setTimeout(XScriptVM* vm)
{
	CheckParam(set_timeout, 0, ms, OP_TYPE_INT);
	XPromise* promise = vm->CreatePromise();
	vm->AddAsyncTimer((int)ms.iIntValue, promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));
}

void host_execAsync(XScriptVM* vm)
{
	CheckParam(exec_async, 0, cmd, OP_TYPE_STRING);
	std::string cmdCopy = stringRawValue(&cmd);
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));

	std::weak_ptr<AsyncRuntime> runtime = vm->GetAsyncRuntime();
	std::thread([runtime, promise, cmdCopy]() {
		std::string output;
		int code = -1;
		FILE* pipe = _popen(cmdCopy.c_str(), "r");
		if (pipe != NULL)
		{
			char buffer[512];
			while (fgets(buffer, sizeof(buffer), pipe) != NULL)
				output += buffer;
			code = _pclose(pipe);
		}

		XScriptVM::QueueAsyncCompletion(runtime, [promise, output, code](XScriptVM* owner) {
			TABLE table = owner->newTable();
			owner->setTableValue(table, "ok", (XInt)(code == 0 ? 1 : 0));
			owner->setTableValue(table, "code", (XInt)code);
			owner->setTableValue(table, "output", output.c_str());
			promise->Resolve(owner->ConstructValue(table));
			owner->RemovePendingPromise(promise);
			delete promise;
		});
	}).detach();
}

void host_futureAll(XScriptVM* vm)
{
	CheckParam(all, 0, futures, OP_TYPE_LIST);
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));

	int count = (int)futures.listData->mListSize;
	std::shared_ptr<std::vector<Value> > results(new std::vector<Value>(count));
	std::shared_ptr<int> remaining(new int(count));
	std::shared_ptr<bool> resolved(new bool(false));

	if (count == 0)
	{
		ListValue* list = vm->CreateList();
		promise->Resolve(vm->ConstructValue(list));
		vm->RemovePendingPromise(promise);
		delete promise;
		return;
	}

	auto complete = [promise, results, remaining, resolved](XScriptVM* owner, int index, XFuture::State state, const Value& value) {
		if (*resolved)
			return;
		if (state == XFuture::Rejected)
		{
			*resolved = true;
			promise->Reject(value);
			owner->RemovePendingPromise(promise);
			delete promise;
			return;
		}
		(*results)[index] = value;
		(*remaining)--;
		if (*remaining == 0)
		{
			*resolved = true;
			ListValue* list = owner->CreateList();
			for (int j = 0; j < (int)results->size(); j++)
				owner->ListAppend(list, (*results)[j]);
			promise->Resolve(owner->ConstructValue(list));
			owner->RemovePendingPromise(promise);
			delete promise;
		}
	};

	for (int i = 0; i < count; i++)
	{
		if (*resolved)
			break;
		Value item = futures.listData->mListData[i];
		if (!IsValueFuture(&item) || item.futureData == NULL)
		{
			(*results)[i] = item;
			(*remaining)--;
			continue;
		}

		XFuture* future = item.futureData;
		if (future->IsCompleted())
			complete(vm, i, future->state, future->result);
		else
		{
			future->callbacks.push_back([complete, i](XScriptVM* owner, XFuture::State state, const Value& value) {
				complete(owner, i, state, value);
			});
		}
	}

	if (*remaining == 0 && !*resolved)
	{
		*resolved = true;
		ListValue* list = vm->CreateList();
		for (int j = 0; j < (int)results->size(); j++)
			vm->ListAppend(list, (*results)[j]);
		promise->Resolve(vm->ConstructValue(list));
		vm->RemovePendingPromise(promise);
		delete promise;
	}
}

void host_futureRace(XScriptVM* vm)
{
	CheckParam(race, 0, futures, OP_TYPE_LIST);
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));
	std::shared_ptr<bool> resolved(new bool(false));

	auto complete = [promise, resolved](XScriptVM* owner, XFuture::State state, const Value& value) {
		if (*resolved)
			return;
		*resolved = true;
		if (state == XFuture::Rejected)
			promise->Reject(value);
		else
			promise->Resolve(value);
		owner->RemovePendingPromise(promise);
		delete promise;
	};

	for (int i = 0; i < futures.listData->mListSize; i++)
	{
		Value item = futures.listData->mListData[i];
		if (!IsValueFuture(&item) || item.futureData == NULL)
			continue;
		XFuture* future = item.futureData;
		if (future->IsCompleted())
		{
			complete(vm, future->state, future->result);
			break;
		}
		else
			future->callbacks.push_back(complete);
	}
}

void host_futureTimeout(XScriptVM* vm)
{
	CheckParam(timeout, 0, futureValue, OP_TYPE_FUTURE);
	CheckParam(timeout, 1, ms, OP_TYPE_INT);
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));
	std::shared_ptr<bool> resolved(new bool(false));

	XFuture* source = futureValue.futureData;
	auto sourceComplete = [promise, resolved](XScriptVM* owner, XFuture::State state, const Value& value) {
		if (*resolved)
			return;
		*resolved = true;
		if (state == XFuture::Rejected)
			promise->Reject(value);
		else
			promise->Resolve(value);
		owner->RemovePendingPromise(promise);
		delete promise;
	};
	if (source != NULL)
	{
		if (source->IsCompleted())
			sourceComplete(vm, source->state, source->result);
		else
			source->callbacks.push_back(sourceComplete);
	}
	if (*resolved)
		return;

	XPromise* timerPromise = vm->CreatePromise();
	vm->AddAsyncTimer((int)ms.iIntValue, timerPromise);
	timerPromise->future->callbacks.push_back([promise, resolved](XScriptVM* owner, XFuture::State, const Value&) {
		if (*resolved)
			return;
		*resolved = true;
		TABLE table = owner->newTable();
		owner->setTableValue(table, "ok", (XInt)0);
		owner->setTableValue(table, "timeout", (XInt)1);
		promise->Resolve(owner->ConstructValue(table));
		owner->RemovePendingPromise(promise);
		delete promise;
	});
}

void	host_array(XScriptVM* vm)
{
	XInt arraySize = 0;
	if (vm->getParamAsInt(0, arraySize) && arraySize > 0)
	{
		TableValue* table = vm->newTable((int)arraySize);
		vm->setReturnAsTable(table);
	}
}


void host_toNumber(XScriptVM* vm)
{
	Value stackValue = vm->getParamValue(0);
	if (IsValueNumber(&stackValue))
	{
		vm->setReturnAsInt((XInt)PNumberValue(stackValue));
	}
	else if (IsValueString(&stackValue))
	{
		vm->setReturnAsInt(StrToXInt(stringRawValue(&stackValue)));
	}
	else
		vm->setReturnAsNil(0);

}

void host_toString(XScriptVM* vm)
{
	Value stackValue = vm->getParamValue(0);
	if (stackValue.type == OP_TYPE_STRING)
	{
		vm->setReturnAsStr(stringRawValue(&stackValue));
	}
	else
	{
		std::string desc = getValueDescString(vm, stackValue);
		vm->setReturnAsStr(desc.c_str());
	}
}

void host_pause(XScriptVM* vm)
{
	system("pause");
}

void host_getmetadata(XScriptVM* vm)
{
	Value param1 = vm->getParamValue(0);
	TableValue* metaTable = vm->GetMetaTable(&param1);
	if (metaTable != NULL)
	{
		vm->setReturnAsTable(metaTable);
	}
	else
	{
		vm->setReturnAsNil(0);
	}
	
}

void host_setmetadata(XScriptVM* vm)
{
	CheckParam(setmetatable, 1, param2, OP_TYPE_TABLE);

	Value param1 = vm->getParamValue(0);
	if (IsValueUserData(&param1))
	{
		param1.userData->mMetaTable = param2.tableData;
	}
	else if (IsValueTable(&param1))
	{
		param1.tableData->mMetaTable = param2.tableData;
	}
	else if (IsValueList(&param1))
	{
		param1.listData->mMetaTable = param2.tableData;
	}
	
}

void host_type(XScriptVM* vm)
{
	int type = vm->getParamType(0);
	vm->setReturnAsStr(getTypeName(type));
}

void host_gc(XScriptVM* vm)
{
	vm->GarbageCollect();
}

int	CalFormatSize(const Value& format)
{
	int bufferLen = 0;
	int lastCount = 1;
	int formatLen = (int)stringRealLen(&format);
	for (int i = 0; i < formatLen; i++)
	{
		if (isdigit(stringRawValue(&format)[i]))
		{
			lastCount = stringRawValue(&format)[i] - '0';

			while (i < formatLen - 1 && isdigit(stringRawValue(&format)[i + 1]))
			{
				lastCount = lastCount * 10 + (stringRawValue(&format)[i + 1] - '0');
				i++;
			}
		}
		else
		{
			switch (stringRawValue(&format)[i])
			{
			case 'c':
			{
				bufferLen += lastCount * sizeof(char);
			}
			break;
			case 'h':
			{
				bufferLen += lastCount * sizeof(short);
			}
			break;
			case 'i':
			{
				bufferLen += lastCount * sizeof(int);
			}
			break;
			case 'l':
			{
				bufferLen += lastCount * sizeof(long long);
			}
			break;
			case 'f':
			{
				bufferLen += lastCount * sizeof(float);
			}
			break;
			case 'd':
			{
				bufferLen += lastCount * sizeof(double);
			}
			break;
			case 's':
			{
				bufferLen += lastCount * sizeof(char);
			}
			break;
			}

			lastCount = 1;
		}

	}

	return bufferLen;
}

void xstruct_pack(XScriptVM* vm)
{
	CheckParam(struct.pack, 0, format, OP_TYPE_STRING);
	int len = CalFormatSize(format);

	char* buffer = new char[len];
	char* p = buffer;

	int lastCount = 1;
	int curParamIndex = 1;
	int formatLen = (int)stringRealLen(&format);
	for (int i = 0; i < formatLen; i++)
	{
		if (isdigit(stringRawValue(&format)[i]))
		{
			lastCount = stringRawValue(&format)[i] - '0';

			while (i < formatLen -1 && isdigit(stringRawValue(&format)[i + 1]))
			{
				lastCount = lastCount * 10 + (stringRawValue(&format)[i + 1] - '0');
				i++;
			}
		}
		else
		{
			switch (stringRawValue(&format)[i])
			{
			case 'c':
			{
				for (int index = 0; index < lastCount; index++)
				{
					CheckParam(struct.pack, curParamIndex+index, value, OP_TYPE_INT);
					*(char*)p = (char)value.iIntValue;
					p += sizeof(char);
				}

				curParamIndex += lastCount;
			}
			break;
			case 'h':
			{
				for (int index = 0; index < lastCount; index++)
				{
					CheckParam(struct.pack, curParamIndex+index, value, OP_TYPE_INT);
					*(short*)p = (short)value.iIntValue;
					p += sizeof(short);
				}

				curParamIndex += lastCount;
			}
			break;
			case 'i':
			{
				for (int index = 0; index < lastCount; index++)
				{
					CheckParam(struct.pack, curParamIndex+index, value, OP_TYPE_INT);
					*(int*)p = (int)value.iIntValue;
					p += sizeof(int);
				}

				curParamIndex += lastCount;
			}
			break;
			case 'l':
			{
				for (int index = 0; index < lastCount; index++)
				{
					CheckParam(struct.pack, curParamIndex + index, value, OP_TYPE_INT);
					*(long long*)p = (long long)value.iIntValue;
					p += sizeof(long long);
				}
				curParamIndex += lastCount;
			}
			break;
			case 'f':
			{
				for (int index = 0; index < lastCount; index++)
				{
					CheckParam(struct.pack, curParamIndex + index, value, OP_TYPE_FLOAT);
					*(float*)p = (float)value.fFloatValue;
					p += sizeof(float);
				}
				curParamIndex += lastCount;
			}
			break;
			case 'd':
			{
				for (int index = 0; index < lastCount; index++)
				{
					CheckParam(struct.pack, curParamIndex + index, value, OP_TYPE_FLOAT);
					*(double*)p = (double)value.fFloatValue;
					p += sizeof(double);
				}
				curParamIndex += lastCount;
			}
			break;
			case 's':
			{
				CheckParam(struct.pack, curParamIndex, value, OP_TYPE_STRING);

				memset(p, 0, sizeof(char)*lastCount);
				int strLen = (int)stringRealLen(&value);
				memcpy(p, stringRawValue(&value), lastCount < strLen ? lastCount : strLen);
				p += lastCount;
				curParamIndex++;
			}
			break;
			default:
				vm->ExecError("struct.pack: unsupported format specifier '%c'", stringRawValue(&format)[i]);
				break;
			}

			lastCount = 1;
		}
		
	}

	vm->setReturnAsValue(vm->ConstructValue(vm->NewXString(buffer, len)));
}

void xstruct_calFormatSize(XScriptVM* vm)
{
	CheckParam(struct.unpack, 0, format, OP_TYPE_STRING);

	vm->setReturnAsInt(CalFormatSize(format));
}

void xstruct_unpack(XScriptVM* vm)
{
	CheckParam(struct.unpack, 0, format, OP_TYPE_STRING);
	CheckParam(struct.unpack, 1, buffer, OP_TYPE_STRING);

	int bufferLen = CalFormatSize(format);

	if (stringRawLen(&buffer) < bufferLen)
	{
		vm->ExecError("struct.unpack: buffer too short, expected at least %d bytes, but got %d", bufferLen, stringRawLen(&buffer));
	}

	const char* p = stringRawValue(&buffer);
	int lastCount = 1;
	int curParamIndex = 0;
	for (int i = 0; i < (int)stringRealLen(&format); i++)
	{
		if (isdigit(stringRawValue(&format)[i]))
		{
			lastCount = stringRawValue(&format)[i] - '0';
		}
		else
		{
			switch (stringRawValue(&format)[i])
			{
			case 'c':
			{
				for (int index = 0; index < lastCount; index++)
				{
					vm->setReturnAsInt(*(char*)p, curParamIndex + index);
					p += sizeof(char);
				}
				curParamIndex += lastCount;
			}
			break;
			case 'h':
			{
				for (int index = 0; index < lastCount; index++)
				{
					vm->setReturnAsInt(*(short*)p, curParamIndex + index);
					p += sizeof(short);
				}
				curParamIndex += lastCount;
			}
			break;
			case 'i':
			{
				for (int index = 0; index < lastCount; index++)
				{
					vm->setReturnAsInt(*(int*)p, curParamIndex + index);
					p += sizeof(int);
				}
				curParamIndex += lastCount;
			}
			break;
			case 'l':
			{
				for (int index = 0; index < lastCount; index++)
				{
				vm->setReturnAsInt((XInt)*(long long*)p, curParamIndex + index);
					p += sizeof(long long);
				}
				curParamIndex += lastCount;
			}
			break;
			case 'f':
			{
				for (int index = 0; index < lastCount; index++)
				{
					vm->setReturnAsfloat(*(float*)p, curParamIndex + index);
					p += sizeof(float);
				}
				curParamIndex += lastCount;
			}
			break;
			case 'd':
			{
				for (int index = 0; index < lastCount; index++)
				{
				vm->setReturnAsfloat((XFloat)*(double*)p, curParamIndex + index);
					p += sizeof(double);
				}
				curParamIndex += lastCount;
			}
			break;
			case 's':
			{
				char* strBuf = new char[lastCount];
				memcpy(strBuf, p, lastCount);
				vm->setReturnAsValue(vm->ConstructValue(vm->NewXString(strBuf, lastCount)), curParamIndex);
				delete []strBuf;

				p += lastCount;
				curParamIndex++;
			}
			break;
			default:
				vm->ExecError("struct.unpack: unsupported format specifier '%c'", stringRawValue(&format)[i]);
				break;
			}

			lastCount = 1;
		}

	}
}

// 初始化 package 库：注册 package 全局表，包含 loaded 和 path 字段
void init_package_lib()
{
	// 创建 package 表
	TABLE packageTable = gScriptVM.newTable();

	// 将 VM 内部的 mModuleTable 设为 package.loaded（共享同一个表对象）
	// 需要通过公共接口获取 mModuleTable，这里直接在 init 中处理
	// package.path 默认搜索路径
	gScriptVM.setTableValue(packageTable, "path", "./?.xs");

	// 注册 package 为全局变量
	int index = gScriptVM.RegisterGlobalValue("package");
	gScriptVM.setGlobalStackValue(index, gScriptVM.ConstructValue(packageTable));
}
