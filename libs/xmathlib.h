#ifndef _HOST_MATH_API_H
#define _HOST_MATH_API_H
class XScriptVM;


void  init_math_lib();

void  host_random(XScriptVM* vm);

void  host_acos(XScriptVM* vm);
void  host_asin(XScriptVM* vm);
void  host_atan(XScriptVM* vm);
void  host_cos(XScriptVM* vm);
void  host_exp(XScriptVM* vm);
void  host_log(XScriptVM* vm);
void  host_log10(XScriptVM* vm);
void  host_sin(XScriptVM* vm);
void  host_sqrt(XScriptVM* vm);
void  host_tan(XScriptVM* vm);

#endif