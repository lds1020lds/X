#ifndef xjsonlib_h__
#define xjsonlib_h__

class XScriptVM;

void init_json_lib();
void xjson_encode(XScriptVM* vm);
void xjson_decode(XScriptVM* vm);

#endif
