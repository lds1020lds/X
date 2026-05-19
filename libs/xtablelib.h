#ifndef xtablelib_h__
#define xtablelib_h__

class XScriptVM;

void xtable_keys(XScriptVM* vm);
void xtable_values(XScriptVM* vm);
void xtable_size(XScriptVM* vm);
void xtable_contains(XScriptVM* vm);
void xtable_clear(XScriptVM* vm);
void xtable_clone(XScriptVM* vm);
void xtable_merge(XScriptVM* vm);
void xtable_Iterator(XScriptVM* vm);
void xtable_next(XScriptVM* vm);
void xtable_tostring(XScriptVM* vm);
#endif
