#include "CommonFunc.h"
#include <setjmp.h>


struct OpState
{
	char cChar;
	int  iFirst;
	int  iNum;
	int  iIndex;
};

OpState g_OpChars0 [] = { { '+', 0, 2, 0 }, { '-', 2, 2, 1 }, { '*', 4, 1, 2 }, { '/', 5, 1, 3 },
{ '%', 6, 1, 4 }, { '^', 7, 1, 5 }, { '&', 8, 2, 6 }, { '|', 10, 3, 7 },
{ '#', 13, 1, 8 }, { '~', 0, 0, 9 }, { '!', 14, 1, 10 }, { '=', 15, 2, 11 },
{ '<', 17, 2, 12 }, { '>', 19, 2, 13 }, { '$', 21, 1, 35 } };

// ---- Second operator characters

OpState g_OpChars1 [] = { { '=', 0, 0, 14 }, { '+', 0, 0, 15 },     // +=, ++
{ '=', 0, 0, 16 }, { '-', 0, 0, 17 },     // -=, --
{ '=', 0, 0, 18 },                        // *=
{ '=', 0, 0, 19 },                        // /=
{ '=', 0, 0, 20 },                        // %=
{ '=', 0, 0, 21 },                        // ^=
{ '=', 0, 0, 22 }, { '&', 0, 0, 23 },     // &=, &&
{ '=', 0, 0, 24 }, { '|', 0, 0, 25 }, { '>', 2, 1, 43 },     // |=, ||, |>
{ '=', 0, 0, 26 },                        // #=
{ '=', 0, 0, 27 },                        // !=
{ '=', 0, 0, 28 }, { '>', 0, 0, 42 },     // ==, =>
{ '=', 0, 0, 29 }, { '<', 0, 1, 30 },     // <=, <<
{ '=', 0, 0, 31 }, { '>', 1, 1, 32 },     // >=, >>
{ '=', 0, 0, 36 } };                      // $=

// ---- Third operator characters

OpState g_OpChars2 [] = { { '=', 0, 0, 33 }, { '=', 0, 0, 34 }, { '>', 0, 0, 44 } }; // <<=, >>=, |>>



int IsCharDelim ( char cChar )
{
	static char cDelims [] = { ',', '(', ')', '[', ']', '{', '}', ';', '#', '?', ':'};
	// Loop through each delimiter in the array and compare it to the specified character
	for ( int iCurrDelimIndex = 0; iCurrDelimIndex < sizeof(cDelims); ++ iCurrDelimIndex )
	{
		// Return TRUE if a match was found
		if ( cChar == cDelims [ iCurrDelimIndex ] )
			return TRUE;
	}

	// The character is not a delimiter, so return FALSE

	return FALSE;
}




// ---- Second operator characters

/******************************************************************************************
*
*	IsCharWhitespace ()
*
*	Returns a nonzero if the given character is whitespace, or zero otherwise.
*/

int IsCharWhitespace ( char cChar )
{
	// Return true if the character is a space or tab.

	if ( cChar == ' ' || cChar == '\t' || cChar == '\n' || cChar == '\r')
		return TRUE;
	else
		return FALSE;
}

/******************************************************************************************
*
*	IsCharNumeric ()
*
*	Returns a nonzero if the given character is numeric, or zero otherwise.
*/

int IsCharNumeric ( char cChar )
{
	// Return true if the character is between 0 and 9 inclusive.

	if ( cChar >= '0' && cChar <= '9' )
		return TRUE;
	else
		return FALSE;
}

/******************************************************************************************
*
*	IsCharIdent ()
*
*	Returns a nonzero if the given character is part of a valid identifier, meaning it's an
*	alphanumeric or underscore. Zero is returned otherwise.
*/

int IsCharIdent ( char cChar )
{
	// Return true if the character is between 0 or 9 inclusive or is an uppercase or
	// lowercase letter or underscore, or is a high-byte character (for UTF-8/GBK support)

	if ( ( cChar >= '0' && cChar <= '9' ) ||
		( cChar >= 'A' && cChar <= 'Z' ) ||
		( cChar >= 'a' && cChar <= 'z' ) ||
		cChar == '_' ||
		( (unsigned char)cChar >= 0x80 ) )
		return TRUE;
	else
		return FALSE;
}

int GetOprType(int iLevel,  int iIndex)
{
	if (iLevel == 0)
	{
		return g_OpChars0[iIndex].iIndex;
	}
	else if (iLevel == 1)
	{
		return g_OpChars1[iIndex].iIndex;
	}
	else 
	{
		return g_OpChars2[iIndex].iIndex;
	}
}
int IsCharOpChar (char curChar, int preIndex, int iLevel)
{


	int iStartIndex, iOprNum;
	if (iLevel == 0)
	{
		iStartIndex = 0;
		iOprNum = sizeof(g_OpChars0) / sizeof(OpState);
	}
	else
	{
		if (iLevel == 1)
		{
			iStartIndex = g_OpChars0[preIndex].iFirst;
			iOprNum     = g_OpChars0[preIndex].iNum;
		}
		else if (iLevel == 2)
		{
			iStartIndex = g_OpChars1[preIndex].iFirst;
			iOprNum     = g_OpChars1[preIndex].iNum;
		}
		else if (iLevel == 3)
			return -1;
	}

	//static char OpChars[] = { '+', '-', '*', '/', '%', '^', '&', '|', '#', '~', '!', '=', '<', '>', '$'};
	// Loop through each state in the specified character index and look for a match
	for ( int iCurrOpStateIndex = iStartIndex; iCurrOpStateIndex < iStartIndex + iOprNum; ++ iCurrOpStateIndex )
	{
		// Get the current state at the specified character index
		char c;
		if (iLevel == 0)
			c = g_OpChars0[iCurrOpStateIndex].cChar;
		else if (iLevel == 1)
			c = g_OpChars1[iCurrOpStateIndex].cChar;
		else
			c = g_OpChars2[iCurrOpStateIndex].cChar;
		if ( curChar == c)
			return iCurrOpStateIndex;
	}

	// Return FALSE if no match is found
	return -1;
}


char gLastParseErrorInfo[512] = { 0 };
int gLastParseErrorLine = 0;
int gLastParseErrorChar = 0;

void ExitOnError(const char* info, int iLine, int iChar)
{
	extern jmp_buf setjmp_buffer;
	strncpy_s(gLastParseErrorInfo, info, sizeof(gLastParseErrorInfo) - 1);
	gLastParseErrorLine = iLine;
	gLastParseErrorChar = iChar;
	fprintf(stderr,"Compile Error: %s [line %d, col %d]\n", info, iLine, iChar);
	longjmp(setjmp_buffer, 1);
}

bool IsRelativeOpr(int opr)
{
	if (OP_TYPE_EQUAL == opr 
		|| OP_TYPE_NOT_EQUAL == opr
		|| OP_TYPE_LESS == opr
		|| OP_TYPE_GREATER == opr
		|| OP_TYPE_LESS_EQUAL == opr
		|| OP_TYPE_GREATER_EQUAL == opr)
		return true;
	return false;
}
bool IsLogicalOpr(int opr)
{
	if (OP_TYPE_LOGICAL_AND == opr
		|| OP_TYPE_LOGICAL_OR == opr
		|| OP_TYPE_LOGICAL_NOT == opr)
	{
		return true;
	}
	return false;
}

int	GetValueTypeMask(int valueType)
{
	if (IsUserType(valueType))
		valueType = OP_LIGHTUSERDATA;

	if (valueType < OP_TYPE_NIL || valueType > OP_TYPE_FUTURE)
		return 0;

	return 1 << (valueType + 1);
}

bool MatchValueType(int valueType, int mask)
{
	return (GetValueTypeMask(valueType) & mask) != 0;
}

int	GetValueMaskByName(const std::string& name)
{
	if (name == "number")
		return GetValueTypeMask(OP_TYPE_INT) | GetValueTypeMask(OP_TYPE_FLOAT);
	else if (name == "nil")
		return GetValueTypeMask(OP_TYPE_NIL);
	else if (name == "int")
		return GetValueTypeMask(OP_TYPE_INT);
	else if (name == "float")
		return GetValueTypeMask(OP_TYPE_FLOAT);
	else if (name == "string")
		return GetValueTypeMask(OP_TYPE_STRING);
	else if (name == "table")
		return GetValueTypeMask(OP_TYPE_TABLE);
	else if (name == "lightuserdata")
		return GetValueTypeMask(OP_LIGHTUSERDATA);
	else if (name == "func" || name == "function")
		return GetValueTypeMask(OP_TYPE_FUNC);
	else if (name == "proto")
		return GetValueTypeMask(OP_TYPE_PROTO);
	else if (name == "upval")
		return GetValueTypeMask(OP_TYPE_UPVAL);
	else if (name == "thread")
		return GetValueTypeMask(OP_TYPE_THREAD);
	else if (name == "userdata")
		return GetValueTypeMask(OP_TYPE_USERDATA);
	else if (name == "list")
		return GetValueTypeMask(OP_TYPE_LIST);
	else if (name == "future")
		return GetValueTypeMask(OP_TYPE_FUTURE);

	return 0;
}
