#include <math.h>
#include "XScriptVM.h"
#include "xmathlib.h"

void init_math_lib()
{
	std::vector<HostFunction> funcVec;
	funcVec.push_back(HostFunction("random", host_random));
	funcVec.push_back(HostFunction("acos",  host_acos));
	funcVec.push_back(HostFunction("asin",  host_asin));
	funcVec.push_back(HostFunction("atan", host_atan));
	funcVec.push_back(HostFunction("cos",  host_cos));
	funcVec.push_back(HostFunction("exp",  host_exp));
	funcVec.push_back(HostFunction("log",  host_log));
	funcVec.push_back(HostFunction("log10",  host_log10));
	funcVec.push_back(HostFunction("sin",  host_sin));
	funcVec.push_back(HostFunction("sqrt",  host_sqrt));
	funcVec.push_back(HostFunction("tan",  host_tan));
	gScriptVM.RegisterHostLib("math", funcVec);
}

void  host_random(XScriptVM* vm)
{
	int r = rand();
	vm->setReturnAsInt(r);
}


void  host_acos(XScriptVM* vm)
{
	XFloat fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat((XFloat)acos(fValue));
	}
}


void  host_asin(XScriptVM* vm)
{
	XFloat fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat((XFloat)asin(fValue));
	}
}


void  host_atan(XScriptVM* vm)
{
	XFloat fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat((XFloat)atan(fValue));
	}
	
}


void  host_cos(XScriptVM* vm)
{
	XFloat fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat((XFloat)cos(fValue));
	}
	
}


void  host_exp(XScriptVM* vm)
{
	XFloat fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat((XFloat)exp(fValue));
	}

}


void  host_log(XScriptVM* vm)
{
	XFloat fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat((XFloat)log(fValue));
	}

}


void  host_log10(XScriptVM* vm)
{
	XFloat fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat((XFloat)log10(fValue));
	}
	
}


void  host_sin(XScriptVM* vm)
{
	XFloat fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat((XFloat)sin(fValue));
	}

}


void  host_sqrt(XScriptVM* vm)
{
	XFloat fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat((XFloat)sqrt(fValue));
	}
}


void  host_tan(XScriptVM* vm)
{
	XFloat fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat((XFloat)tan(fValue));
	}
	
}
