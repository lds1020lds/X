#pragma once
#ifndef xstring_h__
#define xstring_h__
class XScriptVM;

void xstring_len(XScriptVM* vm);
void xstring_rawlen(XScriptVM* vm);

void xstring_find(XScriptVM* vm);
void xstring_sub(XScriptVM* vm);

void xstring_compare(XScriptVM* vm);
void xstring_lower(XScriptVM* vm);
void xstring_upper(XScriptVM* vm);
void xstring_split(XScriptVM* vm);
void xstring_rfind(XScriptVM* vm);
void xstring_replace(XScriptVM* vm);
void xstring_trim(XScriptVM* vm);
void xstring_trimLeft (XScriptVM* vm);
void xstring_trimRight(XScriptVM* vm);
void xstring_startWith(XScriptVM* vm);
void xstring_endWith(XScriptVM* vm);
void xstring_isalpha(XScriptVM* vm);
void xstring_isdigit(XScriptVM* vm);
void xstring_isspace(XScriptVM* vm);
void xstring_islower(XScriptVM* vm);
void xstring_isupper(XScriptVM* vm);
void xstring_atoi(XScriptVM* vm);
void xstring_atof(XScriptVM* vm);
void xstring_regex_search(XScriptVM* vm);
void xstring_regex_match(XScriptVM* vm);
void xstring_utf8_to_gbk(XScriptVM* vm);
void xstring_gbk_to_utf8(XScriptVM* vm);

void xstring_unicode_to_utf8(XScriptVM* vm);
void xstring_utf8_to_unicode(XScriptVM* vm);
void xstring_mul(XScriptVM* vm);
#endif // xstring_h__