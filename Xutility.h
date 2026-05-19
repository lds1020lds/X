#ifndef Xutility_h__
#define Xutility_h__
#include <string>

char* Utf8ToGBK(const char* utf8);
char* GBKToUtf8(const char* gb2312);
std::string UnicodeToUTF8(const std::wstring&	 src);
std::wstring UTF8ToUnicode(const std::string& src);

#endif // Xutility_h__
