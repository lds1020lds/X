#include "xstringlib.h"
#include "XScriptVM.h"
#include "regex.h"
#include "Xutility.h"


void xstring_regex_match(XScriptVM* vm)
{

}

void xstring_regex_search(XScriptVM* vm)
{
	CheckParam(string.reg_search, 0, pattern, OP_TYPE_STRING);
	CheckParam(string.reg_search, 1, matchStr, OP_TYPE_STRING);

	Regex regex((char*)stringRawValue(&pattern));

	const std::vector<MatchData>& retVec = regex.Search((char*)stringRawValue(&matchStr));
	if (retVec.size() > 0)
	{
		TABLE table = vm->newTable();
		for (int i = 0; i < (int)retVec.size(); i++)
		{
			TABLE matchData = vm->newTable();
			vm->setTableValue(matchData, (char*)"content", (char*)retVec[i].matchedValue.c_str());
			if (retVec[i].groupIDMap.size() > 0)
			{
				TABLE matchIDData = vm->newTable();
				std::map<int, std::string>::const_iterator it = retVec[i].groupIDMap.begin();
				for (; it != retVec[i].groupIDMap.end(); it++)
				{
					vm->setTableValue(matchIDData, it->first, (char*)it->second.c_str());
				}

				vm->setTableValue(matchData, "groupID", matchIDData);
			}

			if (retVec[i].groupNameMap.size() > 0)
			{
				TABLE matchIDData = vm->newTable();
				std::map<std::string, std::string>::const_iterator it = retVec[i].groupNameMap.begin();
				for (; it != retVec[i].groupNameMap.end(); it++)
				{
					vm->setTableValue(matchIDData, (char*)it->first.c_str(), (char*)it->second.c_str());
				}

				vm->setTableValue(matchData, "groupName", matchIDData);
			}

			vm->setTableValue(table, i, matchData);
		}

		vm->setReturnAsTable(table);
	}
	else
	{
		vm->setReturnAsNil(0);
	}
		
}


void xstring_len(XScriptVM* vm)
{
	CheckParam(string.len, 0, str, OP_TYPE_STRING);
	vm->setReturnAsInt((int)strlen(stringRawValue(&str)));
}

void xstring_rawlen(XScriptVM* vm)
{
	CheckParam(string.len, 0, str, OP_TYPE_STRING);
	vm->setReturnAsInt(stringRawLen(&str));
}

void xstring_compare(XScriptVM* vm)
{
	CheckParam(string.compare, 0, str1, OP_TYPE_STRING);
	CheckParam(string.compare, 1, str2, OP_TYPE_STRING);

	vm->setReturnAsInt(strcmp(stringRawValue(&str1), stringRawValue(&str2)));

}

void xstring_lower(XScriptVM* vm)
{
	CheckParam(string.lower, 0, str, OP_TYPE_STRING);
	int len = (int)strlen(stringRawValue(&str));
	char* newStr = new char[len + 1];
	newStr[len] = 0;
	for (int i = 0; i < len + 1; i++)
		newStr[i] = tolower(stringRawValue(&str)[i]);
	
	vm->setReturnAsStr(newStr);
	delete newStr;
}

void xstring_upper(XScriptVM* vm)
{
	CheckParam(string.upper, 0, str, OP_TYPE_STRING);
	int len = (int)strlen(stringRawValue(&str));
	char* newStr = new char[len + 1];
	newStr[len] = 0;
	for (int i = 0; i < len + 1; i++)
		newStr[i] = toupper(stringRawValue(&str)[i]);

	vm->setReturnAsStr(newStr);
	delete newStr;
}

void xstring_split(XScriptVM* vm)
{
	CheckParam(string.split, 0, str, OP_TYPE_STRING);
	CheckParam(string.split, 1, key, OP_TYPE_STRING);

	std::vector<std::string> result;
	std::string temp(stringRawValue(&str));
	while (true)
	{
		size_t pos = temp.find(stringRawValue(&key));
		if (pos == std::string::npos)
			break;

		result.push_back(temp.substr(0, pos));
		temp.erase(0, pos + 1);
	}

	if (!temp.empty())
		result.push_back(temp);

	ListValue* List = vm->CreateList();
	vm->ListResize(List, (int)result.size());
	for (int i = 0; i < (int)result.size(); i++)
	{
		List->mListData[i] =  vm->ConstructValue(result[i].c_str());
	}

	vm->setReturnAsValue(vm->ConstructValue(List));
}

void xstring_rfind(XScriptVM* vm)
{
	CheckParam(string.rfind, 0, str, OP_TYPE_STRING);
	CheckParam(string.rfind, 1, subStr, OP_TYPE_STRING);

	size_t startPos = std::string::npos;
	if (vm->getNumParam() > 2)
	{
		CheckParam(xstring_rfind, 2, pos, OP_TYPE_INT);
		startPos = (size_t)pos.iIntValue;
	}

	std::string tempStr = stringRawValue(&str);
	int ret = (int)tempStr.rfind(stringRawValue(&subStr), startPos);
	vm->setReturnAsInt(ret);
}

void xstring_replace(XScriptVM* vm)
{
	CheckParam(string.replace, 0, str, OP_TYPE_STRING);
	CheckParam(string.replace, 1, pattern, OP_TYPE_STRING);
	CheckParam(string.replace, 2, replaceStr, OP_TYPE_STRING);

	std::string tempStr = stringRawValue(&str);

	const size_t nsize = stringRealLen(&replaceStr);
	const size_t psize = stringRealLen(&pattern);

	for (size_t pos = tempStr.find(stringRawValue(&pattern), 0);
	pos != std::string::npos; pos = tempStr.find(stringRawValue(&pattern), pos + nsize))
	{
		tempStr.replace(pos, psize, stringRawValue(&replaceStr));
	}

	vm->setReturnAsStr(tempStr.c_str());
	
}

void xstring_trim(XScriptVM* vm)
{
	CheckParam(string.trim, 0, str, OP_TYPE_STRING);

	int startPos = 0;
	int len = (int)stringRealLen(&str);
	for (int i = 0; i < len; i++)
	{
		if (isspace(stringRawValue(&str)[i]))
		{
			startPos++;
		}
		else
		{
			break;
		}
	}

	int endPos = len;
	for (int i = len - 1; i >= 0; i--)
	{
		if (isspace(stringRawValue(&str)[i]))
		{
			endPos--;
		}
		else
		{
			break;
		}
	}

	int retLen = endPos - startPos;
	if (retLen > 0)
	{
		char* szResult = new char[retLen + 1];
		szResult[retLen] = 0;
		memcpy(szResult, stringRawValue(&str) + startPos, retLen);
		vm->setReturnAsStr(szResult);
		delete szResult;
	}
	else
	{
		vm->setReturnAsStr("");
	}
}

void xstring_trimLeft(XScriptVM* vm)
{
	CheckParam(string.trimLeft, 0, str, OP_TYPE_STRING);

	int startPos = 0;
	int len = (int)stringRealLen(&str);
	for (int i = 0; i < len; i++)
	{
		if (isspace(stringRawValue(&str)[i]))
		{
			startPos++;
		}
		else
		{
			break;
		}
	}

	int retLen = len - startPos;
	if (retLen > 0)
	{
		char* szResult = new char[retLen + 1];
		szResult[retLen] = 0;
		memcpy(szResult, stringRawValue(&str) + startPos, retLen);
		vm->setReturnAsStr(szResult);
		delete szResult;
	}
	else
	{
		vm->setReturnAsStr("");
	}
}

void xstring_trimRight(XScriptVM* vm)
{
	CheckParam(string.trimRight, 0, str, OP_TYPE_STRING);

	int len = (int)stringRealLen(&str);
	int endPos = len;
	for (int i = len - 1; i >= 0; i--)
	{
		if (isspace(stringRawValue(&str)[i]))
		{
			endPos--;
		}
		else
		{
			break;
		}
	}

	if (endPos > 0)
	{
		char* szResult = new char[endPos + 1];
		szResult[endPos] = 0;
		memcpy(szResult, stringRawValue(&str), endPos);
		vm->setReturnAsStr(szResult);
		delete szResult;
	}
	else
	{
		vm->setReturnAsStr("");
	}
}

void xstring_startWith(XScriptVM* vm)
{
	CheckParam(string.startWith, 0, str, OP_TYPE_STRING);
	CheckParam(string.startWith, 1, startStr, OP_TYPE_STRING);
	int startlen = (int)stringRealLen(&startStr);
	int len = (int)stringRealLen(&str);
	int ret = 0;
	if (startlen <= len)
	{
		ret = memcmp(stringRawValue(&str), stringRawValue(&startStr), startlen) == 0;
	}

	vm->setReturnAsInt(ret);
}

void xstring_endWith(XScriptVM* vm)
{
	CheckParam(string.endWith, 0, str, OP_TYPE_STRING);
	CheckParam(string.endWith, 1, endStr, OP_TYPE_STRING);
	int endlen = (int)stringRealLen(&endStr);
	int len = (int)stringRealLen(&str);
	int ret = 0;
	if (endlen <= len)
	{
		ret = memcmp(stringRawValue(&str) + len - endlen, stringRawValue(&endStr), endlen) == 0;
	}

	vm->setReturnAsInt(ret);
}


bool IsStringAllByFunc(const char* str, int (*func)(int))
{
	int len = (int)strlen(str);
	bool ret = len > 0;
	for (int i = 0; i < len; i++)
	{
		if (!func(str[i]))
		{
			ret = false;
			break;
		}
	}
	return ret;
}

void xstring_isalpha(XScriptVM* vm)
{
	CheckParam(string.isalpha, 0, str, OP_TYPE_STRING);
	vm->setReturnAsInt(IsStringAllByFunc(stringRawValue(&str), isalpha));
}

void xstring_isdigit(XScriptVM* vm)
{
	CheckParam(string.isdigit, 0, str, OP_TYPE_STRING);
	vm->setReturnAsInt(IsStringAllByFunc(stringRawValue(&str), isdigit));
}

void xstring_isspace(XScriptVM* vm)
{
	CheckParam(string.isspace, 0, str, OP_TYPE_STRING);
	vm->setReturnAsInt(IsStringAllByFunc(stringRawValue(&str), isspace));
}

void xstring_islower(XScriptVM* vm)
{
	CheckParam(string.islower, 0, str, OP_TYPE_STRING);
	vm->setReturnAsInt(IsStringAllByFunc(stringRawValue(&str), islower));
}

void xstring_isupper(XScriptVM* vm)
{
	CheckParam(string.isupper, 0, str, OP_TYPE_STRING);
	vm->setReturnAsInt(IsStringAllByFunc(stringRawValue(&str), isupper));
}

void xstring_atoi(XScriptVM* vm)
{
	CheckParam(string.atoi, 0, str, OP_TYPE_STRING);
	vm->setReturnAsInt(StrToXInt(stringRawValue(&str)));
}

void xstring_atof(XScriptVM* vm)
{
	CheckParam(string.atof, 0, str, OP_TYPE_STRING);
	vm->setReturnAsfloat(StrToXFloat(stringRawValue(&str)));
}

void xstring_find(XScriptVM* vm)
{
	CheckParam(string.find, 0, str, OP_TYPE_STRING);
	CheckParam(string.find, 1, subStr, OP_TYPE_STRING);

	int startPos = 0;
	if (vm->getNumParam() > 2)
	{
		CheckParam(string.find, 2, pos, OP_TYPE_INT);
		startPos = (int)pos.iIntValue;
	}

	std::string tempStr = stringRawValue(&str);
	int ret = (int)tempStr.find(stringRawValue(&subStr), startPos);
	vm->setReturnAsInt(ret);
}

void xstring_sub(XScriptVM* vm)
{
	CheckParam(string.substr, 0, str, OP_TYPE_STRING);
	CheckParam(string.substr, 1, startPos, OP_TYPE_INT);

	int iSubLen = -1;
	if (vm->getNumParam() > 2)
	{
		CheckParam(string.substr, 2, subLen, OP_TYPE_INT);
		iSubLen = (int)subLen.iIntValue;
	}
	

	int len = (int)stringRealLen(&str);

	if (startPos.iIntValue >= 0 && startPos.iIntValue < len)
	{
		int endPos = len;
		if (iSubLen > 0 && startPos.iIntValue + iSubLen < endPos)
		{
			endPos = (int)startPos.iIntValue + iSubLen;
		}

		int retLen = endPos - (int)startPos.iIntValue;
		char* szRet = new char[retLen + 1];
		szRet[retLen] = 0;
		memcpy(szRet, stringRawValue(&str) + startPos.iIntValue, retLen);

		vm->setReturnAsStr(szRet, 0);
		delete []szRet;
	}
	else
		vm->setReturnAsStr("");
}



void xstring_utf8_to_gbk(XScriptVM* vm)
{
	CheckParam(string.utf8togbk, 0, value, OP_TYPE_STRING);

	char* result = Utf8ToGBK(stringRawValue(&value));
	vm->setReturnAsStr(result);
	delete[] result;
	
}

void xstring_gbk_to_utf8(XScriptVM* vm)
{
	CheckParam(string.gbktoutf8, 0, value, OP_TYPE_STRING);

	char* result = GBKToUtf8(stringRawValue(&value));
	vm->setReturnAsStr(result);
	delete[] result;
}

void xstring_unicode_to_utf8(XScriptVM* vm)
{
	CheckParam(string.unicodetoutf8, 0, str, OP_TYPE_STRING);
	std::wstring s((wchar_t*)stringRawValue(&str));
	std::string result = UnicodeToUTF8(s);
	vm->setReturnAsStr(result.c_str());
}

void xstring_utf8_to_unicode(XScriptVM* vm)
{
	CheckParam(string.utf8tounicode, 0, str, OP_TYPE_STRING);
	std::string s(stringRawValue(&str));
	std::wstring result = UTF8ToUnicode(s);
	XString* x = vm->NewXString((char*)result.c_str(), (int)(result.length() * sizeof(wchar_t)));
	vm->setReturnAsValue(vm->ConstructValue(x));
}


void xstring_mul(XScriptVM* vm)
{
	CheckParam(xstring.mul, 0, str, OP_TYPE_STRING);
	CheckParam(xstring.mul, 1, times, OP_TYPE_INT);

	std::string s(stringRawValue(&str));
	std::string result;
	for (int i = 0; i < times.iIntValue; i++)
	{
		result += s;
	}
	vm->setReturnAsValue(vm->ConstructValue(result.c_str()));
}


