#include "XScriptVM.h"
#include "xtimelib.h"
#include <ctime>
#include <windows.h>

static time_t xtime_param_time(XScriptVM* vm, int index)
{
	if (vm->getNumParam() <= index)
		return time(NULL);
	Value value = vm->getParamValue(index);
	if (value.type == OP_TYPE_INT)
		return (time_t)value.iIntValue;
	if (value.type == OP_TYPE_FLOAT)
		return (time_t)value.fFloatValue;
	return time(NULL);
}

static void xtime_return_tm(XScriptVM* vm, const tm& tmValue)
{
	TABLE t = vm->newTable();
	vm->setTableValue(t, "year", (XInt)(tmValue.tm_year + 1900));
	vm->setTableValue(t, "month", (XInt)(tmValue.tm_mon + 1));
	vm->setTableValue(t, "day", (XInt)tmValue.tm_mday);
	vm->setTableValue(t, "hour", (XInt)tmValue.tm_hour);
	vm->setTableValue(t, "min", (XInt)tmValue.tm_min);
	vm->setTableValue(t, "sec", (XInt)tmValue.tm_sec);
	vm->setTableValue(t, "wday", (XInt)tmValue.tm_wday);
	vm->setTableValue(t, "yday", (XInt)tmValue.tm_yday);
	vm->setTableValue(t, "isdst", (XInt)tmValue.tm_isdst);
	vm->setReturnAsTable(t);
}

void init_time_lib()
{
	std::vector<HostFunction> funcs;
	funcs.push_back(HostFunction("now", xtime_now));
	funcs.push_back(HostFunction("clock", xtime_clock));
	funcs.push_back(HostFunction("sleep", xtime_sleep));
	funcs.push_back(HostFunction("format", xtime_format));
	funcs.push_back(HostFunction("localtime", xtime_localtime));
	funcs.push_back(HostFunction("utc", xtime_utc));
	gScriptVM.RegisterHostLib("time", funcs);
}

void xtime_now(XScriptVM* vm)
{
	vm->setReturnAsInt((XInt)time(NULL));
}

void xtime_clock(XScriptVM* vm)
{
	vm->setReturnAsfloat((XFloat)clock() / CLOCKS_PER_SEC);
}

void xtime_sleep(XScriptVM* vm)
{
	CheckParam(time.sleep, 0, ms, OP_TYPE_INT);
	Sleep((DWORD)ms.iIntValue);
}

void xtime_format(XScriptVM* vm)
{
	CheckParam(time.format, 0, fmt, OP_TYPE_STRING);
	time_t t = xtime_param_time(vm, 1);
	tm tmValue;
	localtime_s(&tmValue, &t);
	char buf[256];
	if (strftime(buf, sizeof(buf), stringRawValue(&fmt), &tmValue) == 0)
		vm->setReturnAsStr("");
	else
		vm->setReturnAsStr(buf);
}

void xtime_localtime(XScriptVM* vm)
{
	time_t t = xtime_param_time(vm, 0);
	tm tmValue;
	localtime_s(&tmValue, &t);
	xtime_return_tm(vm, tmValue);
}

void xtime_utc(XScriptVM* vm)
{
	time_t t = xtime_param_time(vm, 0);
	tm tmValue;
	gmtime_s(&tmValue, &t);
	xtime_return_tm(vm, tmValue);
}
