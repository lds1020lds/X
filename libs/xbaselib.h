#ifndef _HOST_SYSTEM_API_H
#define _HOST_SYSTEM_API_H
#include <string>
class XScriptVM;
struct Value;
std::string  getValueDescString(XScriptVM* vm, const Value& value);

void init_base_lib();
void init_package_lib();
void host_sleep(XScriptVM* vm);
void host_runUntilComplete(XScriptVM* vm);
void host_asyncTick(XScriptVM* vm);
void host_runFor(XScriptVM* vm);
void host_spawn(XScriptVM* vm);
void host_futureDone(XScriptVM* vm);
void host_futureResult(XScriptVM* vm);
void host_futureState(XScriptVM* vm);
void host_nextTick(XScriptVM* vm);
void host_setTimeout(XScriptVM* vm);
void host_execAsync(XScriptVM* vm);
void host_futureAll(XScriptVM* vm);
void host_futureRace(XScriptVM* vm);
void host_futureTimeout(XScriptVM* vm);
void host_pause(XScriptVM* vm);
void host_type(XScriptVM* vm);
void host_toString(XScriptVM* vm);
void host_toNumber(XScriptVM* vm);
void host_array(XScriptVM* vm);
void host_inext(XScriptVM*);
void host_ipairs(XScriptVM* vm);

void list_next(XScriptVM*);
void list_enum(XScriptVM* vm);

void host_gc(XScriptVM* vm);
void host_print(XScriptVM* vm);
void host_prints(XScriptVM* vm);
void xbase_messageBox(XScriptVM* vm);

void host_GetCurrentTime(XScriptVM* vm);
void host_require(XScriptVM* vm);

void host_pcall(XScriptVM* vm);
void host_xpcall(XScriptVM* vm);
void host_setEnvTable(XScriptVM* vm);
void host_getEnvTable(XScriptVM* vm);
void host_loadstring(XScriptVM* vm);
void host_loadfile(XScriptVM* vm);

void host_coCreate(XScriptVM* vm);
void host_coResume(XScriptVM* vm);
void host_coYield(XScriptVM* vm);
void host_coStatus(XScriptVM* vm);
void host_coWrapCreate(XScriptVM* vm);

void host_coWrapResume(XScriptVM* vm);
void host_setmetadata(XScriptVM* vm);
void host_getmetadata(XScriptVM* vm);

void host_settable(XScriptVM* vm);
void host_gettable(XScriptVM* vm);
void host_setglobal(XScriptVM* vm);
void host_getglobal(XScriptVM* vm);
void host_hasglobal(XScriptVM* vm);
void host_delglobal(XScriptVM* vm);
void host_globals(XScriptVM* vm);

void xstruct_pack(XScriptVM* vm);
void xstruct_unpack(XScriptVM* vm);
void xstruct_calFormatSize(XScriptVM* vm);
#endif