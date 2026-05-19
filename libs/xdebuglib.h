#pragma once
#ifndef xdebug_h__
#define xdebug_h__
#include "xbaselib.h"

void init_debug_lib();

void host_stackbacktrace(XScriptVM* vm);
void host_getLocalvar(XScriptVM* vm);
void host_setLocalvar(XScriptVM* vm);
void host_getLocalvars(XScriptVM* vm);
void host_getGlobalVar(XScriptVM* vm);
void host_getLocalvarByName(XScriptVM* vm);
void host_setLocalvarByName(XScriptVM* vm);
void host_debug(XScriptVM* vm);
void host_hook(XScriptVM* vm);
void host_getDebugInfo(XScriptVM* vm);
void host_getStackDepth(XScriptVM* vm);

#endif // xdebug_h__