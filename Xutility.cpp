#include "Xutility.h"
#include <AccCtrl.h>

char* Utf8ToGBK(const char* utf8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
	if (wstr)
		delete[] wstr;
	return str;
}

char* GBKToUtf8(const char* gb2312)
{
	int len = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_ACP, 0, gb2312, -1, wstr, len);
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
	if (wstr)
		delete[] wstr;
	return str;
}


std::string UnicodeToUTF8(const std::wstring&	 src)
{
	std::string result;

	for (int i = 0; i < (int)src.length(); ++i)
	{
		if (src[i] < 0x80)
		{
			result += (char)src[i];
		}
		else
			if (src[i] < 0x800)
			{
				result += (char)(0xC0 | (src[i] >> 6));
				result += (char)(0x80 | (src[i] & 0x3F));
			}
			else
				if (src[i] < 0x10000)
				{
					result += (char)(0xE0 | (src[i] >> 12));
					result += (char)(0x80 | ((src[i] >> 6) & 0x3F));
					result += (char)(0x80 | (src[i] & 0x3F));
				}
	}

	return result;
}

std::wstring	UTF8ToUnicode(const std::string& src)
{
	std::wstring result;

	for (int i = 0; i < (int)src.length(); )
	{
		wchar_t w;

		if ((src[i] & 0x80) == 0)							// U-00000000 - U-0000007F : 0xxxxxxx 
		{
			w = (wchar_t)(src[i]);
			i += 1;
		}
		else
			if ((src[i] & 0xe0) == 0xc0 && i + 1 < (int)src.length())	// U-00000080 - U-000007FF : 110xxxxx 10xxxxxx 
			{
				w = (wchar_t)(src[i + 0] & 0x3f) << 6;
				w |= (wchar_t)(src[i + 1] & 0x3f);
				i += 2;
			}
			else
				if ((src[i] & 0xf0) == 0xe0 && i + 2 < (int)src.length())	// U-00000800 - U-0000FFFF : 1110xxxx 10xxxxxx 10xxxxxx 
				{
					w = (wchar_t)(src[i + 0] & 0x1f) << 12;
					w |= (wchar_t)(src[i + 1] & 0x3f) << 6;
					w |= (wchar_t)(src[i + 2] & 0x3f);
					i += 3;
				}
				else
					if ((src[i] & 0xf8) == 0xf0)// U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx , this is not Chinese
					{
						w = 0x20;
						i += 4;
					}
					else
						if ((src[i] & 0xfc) == 0xf8)// U-00200000 - U-03FFFFFF: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx , this is not Chinese
						{
							w = 0x20;
							i += 5;
						}
						else						// U-04000000 - U-7FFFFFFF: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx , this is not Chinese
						{
							w = 0x20;
							i += 6;
						}

		//add
		result += w;
	}

	return result;
}

