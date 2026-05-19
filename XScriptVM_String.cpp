#include "XScriptVM.h"

XInt stringUsedMem = 0;

// 尝试将字符串转换为浮点数
bool	XScriptVM::CastStrToFloat(const char *s, XFloat *result) {
	char *endptr;
	*result = (XFloat)strtod(s, &endptr);
	if (endptr == s) return 0;  /* conversion failed */
	if (*endptr == 'x' || *endptr == 'X')  /* maybe an hexadecimal constant? */
		*result = (XFloat)(strtoul(s, &endptr, 16));
	if (*endptr == '\0')
		return 1;  /* most common case */
	while (isspace(*endptr))
		endptr++;
	if (*endptr != '\0') return 0;  /* invalid trailing characters? */
	return 1;
}

// AP哈希函数：用于字符串哈希计算
unsigned int APHash(const char *str, int len)
{
	unsigned int hash = 0;
	int i;

	for (i = 0; i < len && i < 32; i++)
	{
		if ((i & 1) == 0)
		{
			hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
		}
		else
		{
			hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
		}
	}

	return (hash & 0x7FFFFFFF);
}


// 创建新的字符串对象（字符串驻留：相同内容复用同一对象）
XString*		XScriptVM::NewXString(const char* str, int len)
{
	unsigned int hash = APHash(str, len);

	int hashIndex = hash % mStringHashSize;
	XString* nextStr = mStringHashTable[hashIndex];
	while (nextStr != NULL)
	{
		if (nextStr->len == len && memcmp(&nextStr->value, str, len) == 0)
		{
			return nextStr;
		}

		nextStr = (XString*)nextStr->next;
	}

	int strSize = sizeof(XString) + (len + 1) * sizeof(char);
	char* buffer = new char[strSize];
	XString* newStr = (XString*)buffer;
	
	stringUsedMem += strSize;

	newStr->hash = hash;
	newStr->len = len;
	memcpy(&newStr->value, str, len);
	((char*)(&newStr->value))[len] = 0;
	((char*)(&newStr->value))[len + 1] = 0;
	GC_SetWhite(newStr);
	newStr->type = OP_TYPE_STRING;

	newStr->next = (CGObject*)mStringHashTable[hashIndex];
	mStringHashTable[hashIndex] = newStr;
	mStringHashUsedSize++;

	if (mStringHashUsedSize > (mStringHashSize / 2))
	{
		ResizeHashTable();
	}

	return newStr;
}



// 创建新的字符串对象（自动计算长度）
XString*	XScriptVM::NewXString(const char* str)
{
	int len = (int)strlen(str);
	return NewXString(str, len);
}

// 调整字符串哈希表大小（扩容为原来的2倍）
void XScriptVM::ResizeHashTable()
{
	int newHashSize = mStringHashSize * 2;
	XString** newHashTable = new XString*[newHashSize];
	memset(newHashTable, 0, sizeof(XString*) * newHashSize);

	for (int i = 0; i < mStringHashSize; i++)
	{
		XString* nextStr = mStringHashTable[i];
		while (nextStr != NULL)
		{
			CGObject* saved = nextStr->next;
			int newHashIndex = nextStr->hash % newHashSize;
			nextStr->next = (CGObject*)newHashTable[newHashIndex];
			newHashTable[newHashIndex] = nextStr;

			nextStr = (XString*)saved;
		}
	}

	delete[]mStringHashTable;

	mStringHashTable = newHashTable;
	mStringHashSize = newHashSize;
}
