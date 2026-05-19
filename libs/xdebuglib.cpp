#include "xdebuglib.h"
#include "XScriptVM.h"

void init_debug_lib()
{
	std::vector<HostFunction> debugFuncVec;

	debugFuncVec.push_back(HostFunction("stackbacktrace", host_stackbacktrace));
	debugFuncVec.push_back(HostFunction("getlocalvar", host_getLocalvar));
	debugFuncVec.push_back(HostFunction("setlocalvar",  host_setLocalvar));

	debugFuncVec.push_back(HostFunction("getlocalvarbyname",  host_getLocalvarByName));
	debugFuncVec.push_back(HostFunction("setlocalvarbyname",  host_setLocalvarByName));

	debugFuncVec.push_back(HostFunction("debug",  host_debug));
	debugFuncVec.push_back(HostFunction("sethook",  host_hook));
	debugFuncVec.push_back(HostFunction("getlocalvars", host_getLocalvars));
	debugFuncVec.push_back(HostFunction("getdebuginfo",  host_getDebugInfo));
	debugFuncVec.push_back(HostFunction("getglobalvar", host_getGlobalVar));
	debugFuncVec.push_back(HostFunction("getstackdepth", host_getStackDepth));
	gScriptVM.RegisterHostLib("debug", debugFuncVec);
}

void host_getLocalvar(XScriptVM* vm)
{
	XInt stackIndex = 0;
	XInt varIndex = 0;
	vm->getParamAsInt(0, stackIndex);
	vm->getParamAsInt(1, varIndex);
	std::string name;
	Value* retValue = vm->GetStackValueByIndex((int)stackIndex, (int)varIndex, name);
	
	if (retValue != NULL)
	{
		vm->setReturnAsInt(1, 0);
		vm->setReturnAsStr(name.c_str(), 1);
		vm->setReturnAsValue(*retValue, 2);
	}
	else
	{
		vm->setReturnAsInt(0);
	}
}

void host_stackbacktrace(XScriptVM* vm)
{
	std::string stackBack = vm->stackBackTrace();
	vm->setReturnAsStr(stackBack.c_str());
}

void host_getStackDepth(XScriptVM* vm)
{
	vm->setReturnAsInt(vm->GetStackDepth());
}

void host_getLocalvars(XScriptVM* vm)
{
	XInt stackIndex = 0;
	if (vm->getParamAsInt(0, stackIndex))
	{
		int callIndex = vm->GetStackDepth() - (int)stackIndex;
		TableValue* table = vm->BuildFrameVarsTable(callIndex);
		if (table != NULL)
		{
			vm->setReturnAsInt(1, 0);
			vm->setReturnAsTable(table, 1);
		}
		else
		{
			vm->setReturnAsInt(0, 0);
		}
	}
	else
	{
		vm->setReturnAsInt(0, 0);
	}
}

void host_getDebugInfo(XScriptVM* vm)
{
	Value pValue = vm->getParamValue(0);

	if (pValue.type == OP_TYPE_INT)
	{
		int callIndex = vm->GetStackDepth() - (int)pValue.iIntValue;
		if (callIndex >= 0 && callIndex <= vm->GetStackDepth())
		{
			FuncState* funcState = vm->GetCallInfo(callIndex)->mCurFunctionState;
			TABLE table = vm->newTable();
			vm->setTableValue(table, (char*)"source", (char*)funcState->sourceFileName.c_str());
			vm->setTableValue(table, (char*)"what", (char*)"lua");
			vm->setTableValue(table, (char*)"name", (char*)funcState->funcName.c_str());
			vm->setTableValue(table, (char*)"nups", (XInt)funcState->m_upValueVec.size());
			vm->setReturnAsInt(1, 0);
			vm->setReturnAsValue(vm->ConstructValue(table), 1);
		}
		else
		{
			vm->setReturnAsInt(0, 0);
		}
	}
	else if (pValue.type == OP_TYPE_FUNC)
	{
		TABLE table = vm->newTable();
		if (pValue.func->isCFunc)
		{
			vm->setTableValue(table, (char*)"what", (char*)"C");
			vm->setTableValue(table, (char*)"nups", (XInt)pValue.func->funcUnion.cFunc.mNumUpVal);
		}
		else
		{
			FuncState* funcState = pValue.func->funcUnion.luaFunc.proto;
			vm->setTableValue(table, (char*)"source", (char*)funcState->sourceFileName.c_str());
			vm->setTableValue(table, (char*)"what", (char*)"lua");
			vm->setTableValue(table, (char*)"name", (char*)funcState->funcName.c_str());
			vm->setTableValue(table, (char*)"nups", (XInt)funcState->m_upValueVec.size());
		}

		vm->setReturnAsInt(1, 0);
		vm->setReturnAsValue(vm->ConstructValue(table), 1);
	}
	else
	{
		vm->setReturnAsInt(0, 0);
	}
	
}

void host_hook(XScriptVM* vm)
{
	int hookIndex = vm->RegisterGlobalValue("_hook");
	Value pValue0 = vm->getParamValue(0);

	if (IsValueString(&pValue0))
	{
		const char* szMask = stringRawValue(&pValue0);
		int mask = 0;
		if (strstr(szMask, "r") != NULL)
			mask |=  MASK_HOOKRET;
		if (strstr(szMask, "l") != NULL)
			mask |=  MASK_HOOKLINE;
		if (strstr(szMask, "c") != NULL)
			mask |=  MASK_HOOKCALL;

		vm->SetHookMask(mask);
	}
	else if (IsValueNil(&pValue0))
	{
		vm->SetHookMask(0);
	}

	Value pValue = vm->getParamValue(1);
	if (IsValueFunction(&pValue) || IsValueNil(&pValue))
	{
	vm->setGlobalStackValue(hookIndex, pValue);
	}
}

void host_getGlobalVar(XScriptVM* vm)
{
	char* varName = 0;
	if (vm->getParamAsString(0, varName))
	{
		Value* pvalue = vm->GetGlobalValue(varName);
		if (pvalue != NULL)
		{
			Value newValue;
			CopyValue(&newValue, *pvalue);
			vm->setReturnAsInt(1, 0);
			vm->setReturnAsValue(newValue, 1);
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

void host_debug(XScriptVM* vm)
{
	while (true)
	{
		char buffer[250];
		fputs("xs_debug> ", stderr);
		if (fgets(buffer, sizeof(buffer), stdin) == 0 ||
			strcmp(buffer, "exit\n") == 0 || strcmp(buffer, "\n") == 0)
			break;
		/*
		if (vm->loadString(buffer))
		{
			for (int i = 0; i < MAX_FUNC_REG; i++)
			{
				Value* p = vm->getReturnRegValue(i);
				if (p->type != OP_TYPE_NIL)
				{
					printf(getValueDescString(vm, p).c_str());
					printf("\t");
				}
			}

			printf("\n");
		}
		*/

	}
}

void host_getLocalvarByName(XScriptVM* vm)
{
	XInt stackIndex = 0;
	char* varName = 0;
	if (vm->getParamAsInt(0, stackIndex) && vm->getParamAsString(1, varName))
	{
		Value* pValue = vm->GetStackValueByName((int)stackIndex, varName);
		if (pValue != NULL)
		{
			vm->setReturnAsInt(1, 0);
			Value retValue;
			CopyValue(&retValue, *pValue);
			vm->setReturnAsValue(retValue, 1);
		}
		else
		{
			vm->setReturnAsInt(0, 0);
		}
	}
	else
	{
		vm->setReturnAsInt(0, 0);
	}

}

void host_setLocalvarByName(XScriptVM* vm)
{
	XInt stackIndex = 0;
	char* varName = 0;
	if (vm->getParamAsInt(0, stackIndex) && vm->getParamAsString(1, varName))
	{
		Value setValue = vm->getParamValue(2);
		Value* pValue = vm->GetStackValueByName((int)stackIndex, varName);
		if (pValue != NULL)
		{
			vm->setReturnAsInt(1, 0);
			*pValue = setValue;
		}
		else
		{
			vm->setReturnAsInt(0, 0);
		}
	}
	else
	{
		vm->setReturnAsInt(0, 0);
	}
}

void host_setLocalvar(XScriptVM* vm)
{
	XInt stackIndex = 0;
	XInt varIndex = 0;
	vm->getParamAsInt(0, stackIndex);
	vm->getParamAsInt(1, varIndex);
	Value setValue = vm->getParamValue(2);

	std::string name;
	
	Value* value = vm->GetStackValueByIndex((int)stackIndex, (int)varIndex, name);
	if (value != NULL)
	{
		*value = setValue; 
		vm->setReturnAsInt(1, 0);
		vm->setReturnAsStr(name.c_str(), 1);
	}
	else
	{
		vm->setReturnAsInt(0);
	}
	
}

