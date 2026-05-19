#ifndef xtimelib_h__
#define xtimelib_h__

class XScriptVM;

void init_time_lib();
void xtime_now(XScriptVM* vm);
void xtime_clock(XScriptVM* vm);
void xtime_sleep(XScriptVM* vm);
void xtime_format(XScriptVM* vm);
void xtime_localtime(XScriptVM* vm);
void xtime_utc(XScriptVM* vm);

#endif
