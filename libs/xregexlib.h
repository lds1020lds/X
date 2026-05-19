#ifndef xregexlib_h__
#define xregexlib_h__

class XScriptVM;

void init_regex_lib();
void xregex_search(XScriptVM* vm);
void xregex_match(XScriptVM* vm);

#endif
