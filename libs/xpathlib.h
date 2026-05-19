#ifndef xpathlib_h__
#define xpathlib_h__

class XScriptVM;

void init_path_lib();
void xpath_join(XScriptVM* vm);
void xpath_normalize(XScriptVM* vm);
void xpath_basename(XScriptVM* vm);
void xpath_dirname(XScriptVM* vm);
void xpath_extname(XScriptVM* vm);
void xpath_isabs(XScriptVM* vm);
void xpath_abspath(XScriptVM* vm);
void xpath_relative(XScriptVM* vm);

#endif
