#include "Lexer.h"
#include "sourcefile.h"
#include "CommonFunc.h"

static int sOpTypes[] = 
{
	OP_TYPE_ADD,
	OP_TYPE_SUB,
	OP_TYPE_MUL ,
	OP_TYPE_DIV,
	OP_TYPE_MOD ,
	OP_TYPE_EXP ,
	OP_TYPE_CONCAT ,

	OP_TYPE_INC,
	OP_TYPE_DEC,

	OP_TYPE_ASSIGN,
	OP_TYPE_ASSIGN_ADD,
	OP_TYPE_ASSIGN_SUB,
	OP_TYPE_ASSIGN_MUL,
	OP_TYPE_ASSIGN_DIV,
	OP_TYPE_ASSIGN_MOD ,
	OP_TYPE_ASSIGN_EXP,
	OP_TYPE_ASSIGN_CONCAT,

	// ---- Bitwise

	OP_TYPE_BITWISE_AND,
	OP_TYPE_BITWISE_OR ,
	OP_TYPE_BITWISE_XOR ,
	OP_TYPE_BITWISE_NOT  ,
	OP_TYPE_BITWISE_SHIFT_LEFT,
	OP_TYPE_BITWISE_SHIFT_RIGHT,

	OP_TYPE_ASSIGN_AND,
	OP_TYPE_ASSIGN_OR     ,
	OP_TYPE_ASSIGN_XOR  ,
	OP_TYPE_ASSIGN_SHIFT_LEFT,
	OP_TYPE_ASSIGN_SHIFT_RIGHT,


	OP_TYPE_LOGICAL_AND,
	OP_TYPE_LOGICAL_OR,
	OP_TYPE_LOGICAL_NOT ,

	// ---- Relational

	OP_TYPE_EQUAL ,
	OP_TYPE_NOT_EQUAL ,
	OP_TYPE_LESS  ,
	OP_TYPE_GREATER,
	OP_TYPE_LESS_EQUAL ,
	OP_TYPE_GREATER_EQUAL,
	OP_TYPE_PIPE,
	OP_TYPE_PIPE_RIGHT,
};

static struct ProrityData {
	int op;  /* left priority for each binary operator */
	int priority; /* right priority */
} priorityMap[] = {  /* ORDER OPR */
	{ OP_TYPE_ADD, 4 },{ OP_TYPE_SUB, 4 },{ OP_TYPE_MUL, 3 },{ OP_TYPE_DIV, 3 },{ OP_TYPE_MOD, 3 },  /* `+' `-' `/' `%' */
	{ OP_TYPE_EXP, 3 },{ OP_TYPE_CONCAT, 4 },                 /* power and concat (right associative) */
	{ OP_TYPE_BITWISE_AND, 8 },{ OP_TYPE_BITWISE_OR, 10 },                  /* equality and inequality */
	{ OP_TYPE_BITWISE_XOR, 9 },{ OP_TYPE_BITWISE_SHIFT_LEFT, 5 },{ OP_TYPE_BITWISE_SHIFT_RIGHT, 5 },{ OP_TYPE_LOGICAL_AND, 11 },  /* order */
	{ OP_TYPE_LOGICAL_OR, 12 },{ OP_TYPE_EQUAL, 7 },{ OP_TYPE_NOT_EQUAL, 7 },{ OP_TYPE_LESS, 6 },{ OP_TYPE_GREATER, 6 },{ OP_TYPE_LESS_EQUAL, 6 },{ OP_TYPE_GREATER_EQUAL, 6 },                  /* logical (and/or) */
};
static char* sOpNames[] = 
{
	"+",
	"-",
	"*",
	"/",
	"%",
	"^",
	"$",
	"++",
	"--",
	"=",
	"+=",
	"-=",
	"*=",
	"/=",
	"%=",
	"^=",
	"$=",
	"&",
	"|",
	"#",
	"~",
	"<<",
	">>",
	"&=",
	"|=",
	"#=",
	"<<=",
	">>=",
	"&&",
	"||",
	"!",
	"==",
	"!=",
	"<",
	">",
	"<=",
	">=",
	"=>",
	"|>",
	"|>>"
};



// ---- Delimiters ------------------------------------------------------------------------

int		CLexer::GetOpPrority(int op)
{
	int count = sizeof(priorityMap) / sizeof(ProrityData);
	for (int i = 0; i < count; i++)
	{
		if (priorityMap[i].op == op)
		{
			return priorityMap[i].priority;
		}
	}

	return -1;
}

CLexer::CLexer(CSourceFile *pSourceFile)
{
	mLastCharIndex = 0;
	mLastLineIndex = 0;
	mLastReadTokenCharIndex = 0;
	mLastReadTokenLineIndex = 0;
	mSourceFile = pSourceFile;

	mIsRecordToken = false;
	mIsReadTokenFromRecord = false;

	mRecordTokenIndex = 0;
	mReadTokeIndex = 0;
	mRecordingBufferIndex = -1;
	mReadingBufferIndex = -1;
	mLastRecordBufferIndex = -1;

	mLastLookToken = false;
	mLastLookTokenInRead = false;
	mLastIsLookTokenReal = false;
	mCurRecordDepth = 0;
}

CLexer::~CLexer()
{
	
}
TOKEN   CLexer::LookNextToken()
{
	
	if (mIsReadTokenFromRecord)
		mLastLookTokenInRead = true;

	if (mIsReadTokenFromRecord)
	{
		const TokenRecord* record = GetReadTokenRecord(0);
		if (!record)
		{
			return TOKEN_TYPE_INVALID;
		}
		return record->token;
	}

	int iLastLineIndex = mLastLineIndex;
	int iLastCharIndex = mLastCharIndex;
	int LastReadTokenCharIndex =  mLastReadTokenCharIndex;
	int LastReadTokenLineIndex =  mLastReadTokenLineIndex;

	TOKEN token = GetNextToken();

	if (mIsRecordToken)
	{	
		mLastLookToken = true;
	}


	RewindToken();
	mLastLineIndex = iLastLineIndex;
	mLastCharIndex = iLastCharIndex;
	mLastReadTokenCharIndex = LastReadTokenCharIndex;
	mLastReadTokenLineIndex = LastReadTokenLineIndex;
	mLastIsLookTokenReal = true;
	return token;
}


TOKEN   CLexer::GetNextToken()
{
	mLastIsLookTokenReal = false;
	if (mIsReadTokenFromRecord)
	{
		mLastLookTokenInRead = false;
		const TokenRecord* record = GetReadTokenRecord(0);
		if (!record)
		{
			return TOKEN_TYPE_INVALID;
		}

		mReadTokeIndex++;
		mCurToken = record->token;
		mOprType = record->opType;
		strncpy_s(mCurLexeme, record->lexeme.c_str(), MAX_LEXEME_SIZE);
		mLastReadTokenLineIndex = record->line;
		mLastReadTokenCharIndex = record->character;
		if (mIsRecordToken)
		{
			int startStackIndex = mReadBufferStack.empty() ? 0 : mReadBufferStack.back().recordingStackSize;
			AddRecordToken(mCurToken, startStackIndex);
			mLastLookToken = false;
		}
		return mCurToken;
	}
	mLastLineIndex = mSourceFile->GetCurLineIndex();
	mLastCharIndex = mSourceFile->GetCurCharIndex();

	bool bAdd = true;
	bool bIsDone = false;
	bool bRewindChar = true;
	int  state = LEX_STATE_START;
	
	char lexeme[MAX_LEXEME_SIZE] = {0};
	int  iLexemeIndex = 0;
	int  iOprLevel = 0;
	int  iLastOprIndex = 0;
	bool isHex;
	bool enterFirstUseChar = false;
	bool isfstring = false;
	int stringEscapeReturnState = LEX_STATE_STRING;
	while(!bIsDone)
	{
		bAdd = true;
		char curChar = mSourceFile->GetNextChar();
		if (curChar == '\0')
		{
			bAdd = false;
			bRewindChar = false;
			break;
		}
		switch(state)
		{
		case LEX_STATE_START:
			{
				bool isCharUseless = IsCharWhitespace(curChar);
				if (!isCharUseless && !enterFirstUseChar)
				{
					mLastReadTokenLineIndex = mSourceFile->GetCurLineIndex();
					mLastReadTokenCharIndex = mSourceFile->GetCurCharIndex() - 1;
					enterFirstUseChar = true;
				}
				if (isCharUseless)
				{
					bAdd = false;
					break;
				} 
				else if (IsCharNumeric(curChar))
				{
					state = LEX_STATE_INT;
					isHex = false;
					if (curChar == '0' &&  mSourceFile->LookNextChar() == 'x')
					{
						lexeme[iLexemeIndex++] = curChar;
						lexeme[iLexemeIndex++] = mSourceFile->GetNextChar();
						isHex = true;
						bAdd = false;
					}
				}
				else if (IsCharIdent(curChar))
				{	
					if (curChar == '_' && !IsCharIdent(mSourceFile->LookNextChar()))
					{
						state = LEX_STATE_DELIM;
					}
					else if (curChar == 'f' && mSourceFile->LookNextChar() == '\"')
					{
						mSourceFile->GetNextChar();
						bAdd = false;
						state = LEX_STATE_STRING;
						isfstring = true;
					}
					else
					{
						state = LEX_STATE_IDENT;
					}
					
					
				}
				else if (IsCharDelim(curChar))
				{
					state = LEX_STATE_DELIM;
				}
				else if (curChar == '"')
				{
					bAdd = false;
					state = LEX_STATE_STRING;
					if (mSourceFile->LookNextChar() == '"')
					{
						mSourceFile->GetNextChar();
						if (mSourceFile->LookNextChar() == '"')
						{
							mSourceFile->GetNextChar();
							state = LEX_STATE_TRIPLE_STRING;
						}
						else
						{
							mSourceFile->ReWindChar();
						}
					}
				}
				else if (curChar == '.')
				{
					state = LEX_STATE_POINT;
				}
				if (curChar == '/' 
					&& (mSourceFile->LookNextChar() == '/'
					|| mSourceFile->LookNextChar() == '*'))
				{
					if (mSourceFile->GetNextChar() == '/')
					{
						state = LEX_STATE_LINE_COMMENT;
					}
					else
					{
						state = LEX_STATE_SEGMENT_COMMENT;
					}
					bAdd = false;
				}
				else if (IsCharOpChar(curChar, iLastOprIndex, iOprLevel) >= 0)
				{
					iLastOprIndex = IsCharOpChar(curChar, iLastOprIndex, iOprLevel);
					iOprLevel++;
					state = LEX_STATE_OP;
				}
				break;
			}
		case LEX_STATE_POINT:
			{
				if (curChar == '.')
				{
					state = LEX_STATE_TWO_POINT;
				}
				else
				{
					bIsDone = true;
					bAdd = false;
				}
			}
			break;
		case LEX_STATE_TWO_POINT:
		{
			if (curChar == '.')
			{
				state = LEX_STATE_THREE_POINT;
			}
			else
			{
				bAdd = false;
				state = LEX_STATE_UNKNOWN;
			}
		}
		break;
		case LEX_STATE_THREE_POINT:
		{
			bIsDone = true;
			bAdd = false;
		}
		break;
		case LEX_STATE_DELIM:
			bIsDone = true;
			bAdd = false;
			break;
		case LEX_STATE_INT:
			{
				
				if(IsCharNumeric(curChar) || (((curChar >= 'a' && curChar <= 'f') || (curChar >= 'A' && curChar <= 'F')) && isHex))
				{
					state = LEX_STATE_INT;
				}
				else if (curChar == '.' && !isHex)
				{
					state = LEX_STATE_FLOAT; 
				}
				else if (IsCharIdent(curChar))
				{
					bAdd = false;
					state = LEX_STATE_UNKNOWN;
				}
				else 
				{
					bAdd = false;
					bIsDone = true;
				}
				break;
			}
		case LEX_STATE_FLOAT:
			{
				if(IsCharNumeric(curChar))
				{
					state = LEX_STATE_FLOAT;
				}
				else if (!IsCharIdent(curChar))
				{
					bAdd = false;
					bIsDone = true;
				}
				else
				{
					state = LEX_STATE_UNKNOWN;
				}
				break;
			}
		case LEX_STATE_IDENT:
			{
				if (!IsCharIdent(curChar))
				{
					bAdd = false;
					bIsDone = true;
				}
				break;
			}
		case LEX_STATE_STRING:
			{
				if (curChar == '"')
				{
					bAdd = false;
					bRewindChar = false;
					bIsDone = true;
				}
				else if (curChar == '\\')
				{
					bAdd = false;
					stringEscapeReturnState = state;
					state = LEX_STATE_STRING_CLOSE_QUOTE;
				}
				break;
			}
		case LEX_STATE_STRING_CLOSE_QUOTE:
			{
				if (curChar == 't')
				{
					curChar = '\t';
				}
				else if (curChar == 'r')
				{
					curChar = '\r';
				}
				else if (curChar == 'n')
				{
					curChar = '\n';
				}
				state = stringEscapeReturnState;
				break;
			}
		case LEX_STATE_TRIPLE_STRING:
			{
				if (curChar == '"' && mSourceFile->LookNextChar() == '"')
				{
					mSourceFile->GetNextChar();
					if (mSourceFile->LookNextChar() == '"')
					{
						mSourceFile->GetNextChar();
						bAdd = false;
						bRewindChar = false;
						bIsDone = true;
					}
					else
					{
						if (iLexemeIndex < MAX_LEXEME_SIZE)
							lexeme[iLexemeIndex++] = curChar;
						else
						{
							char errInfo[128] = { 0 };
							sprintf(errInfo, "token '%.*s...' exceeds maximum length (%d characters)", 20, lexeme, MAX_LEXEME_SIZE);
							ExitOnError(errInfo, mLastLineIndex + 1, mLastCharIndex + 1);
						}
						curChar = '"';
					}
				}
				else if (curChar == '\\')
				{
					bAdd = false;
					stringEscapeReturnState = state;
					state = LEX_STATE_STRING_CLOSE_QUOTE;
				}
				break;
			}
		case LEX_STATE_OP:
			{
				if(IsCharOpChar(curChar, iLastOprIndex, iOprLevel) >= 0)
				{
					state = LEX_STATE_OP;
					iLastOprIndex = IsCharOpChar(curChar, iLastOprIndex, iOprLevel);
					iOprLevel++;
				}
				else
				{
					mOprType = GetOprType(iOprLevel - 1, iLastOprIndex);
					bIsDone = true;
					bAdd    = false;
				}
				break;
			}
		case LEX_STATE_LINE_COMMENT:
			{
				bAdd = false;
				if (curChar == 10)
				{
					state = LEX_STATE_START; 
				}
				break;
			}
		case LEX_STATE_SEGMENT_COMMENT:
			{
				bAdd = false;
				if (curChar == '*' && mSourceFile->GetNextChar() == '/')
				{
					state = LEX_STATE_START;
				}
				break;
			}
		default:
			break;
		}

		if (LEX_STATE_UNKNOWN == state)
		{
			char errInfo[128] = { 0 };
			sprintf(errInfo, "invalid token '%s%c', unexpected character in this context", lexeme, curChar);
			ExitOnError(errInfo, mSourceFile->GetCurLineIndex() + 1, mSourceFile->GetCurCharIndex() + 1);
		}
		if (bAdd)
		{
			if (iLexemeIndex < MAX_LEXEME_SIZE)
				lexeme[iLexemeIndex++] = curChar;
			else
			{
				char errInfo[128] = { 0 };
				sprintf(errInfo, "token '%.*s...' exceeds maximum length (%d characters)", 20, lexeme, MAX_LEXEME_SIZE);
				ExitOnError(errInfo, mLastLineIndex + 1, mLastCharIndex + 1);
			}
		}
	}
	if (bRewindChar)
		mSourceFile->ReWindChar();
	lexeme[iLexemeIndex] = '\0';


	TOKEN  token;
	switch(state)
	{
	case LEX_STATE_START:
		token = TOKEN_TYPE_END_OF_STREAM;
		break;
	case LEX_STATE_INT:
		token = TOKEN_TYPE_INT;
		break;
	case LEX_STATE_FLOAT:
		token = TOKEN_TYPE_FLOAT;
		break;
	case LEX_STATE_STRING:
		token = isfstring ? TOKEN_TYPE_FSTRING:TOKEN_TYPE_STRING;
		break;
	case LEX_STATE_TRIPLE_STRING:
		token = TOKEN_TYPE_STRING;
		break;
	case LEX_STATE_POINT:
		token = TOKEN_TYPE_DELIM_POINT;
		break;
	case LEX_STATE_THREE_POINT:
		token = TOKEN_TYPE_DELIM_THREE_POINT;
		break;
	case LEX_STATE_DELIM:
		{
			if (lexeme[0] == ',')
				token = TOKEN_TYPE_DELIM_COMMA;
			else if(lexeme[0] == '(')
				token = TOKEN_TYPE_DELIM_OPEN_PAREN;
			else if(lexeme[0] == ')')
				token = TOKEN_TYPE_DELIM_CLOSE_PAREN;
			else if(lexeme[0] == '[')
				token = TOKEN_TYPE_DELIM_OPEN_BRACE;
			else if(lexeme[0] == ']')
				token = TOKEN_TYPE_DELIM_CLOSE_BRACE;
			else if(lexeme[0] == '{')
				token = TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE;
			else if(lexeme[0] == '}')
				token = TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE;
			else if(lexeme[0] == ';')
				token = TOKEN_TYPE_DELIM_SEMICOLON;
			else if (lexeme[0] == '_')
				token = TOKEN_TYPE_DELM_UNDERSCORE;
			else if(lexeme[0] == '#')
				token = TOKEN_TYPE_DELIM_JINGHAO;
			else if(lexeme[0] == '?')
			{
				// 问号相关操作符：?? ?. ?( ?[ ?:
				char nextCh = mSourceFile->LookNextChar();
				if (nextCh == '?')
				{
					mSourceFile->GetNextChar(); // consume '?'
					mOprType = OP_TYPE_NULL_COALESCING;
					token = TOKEN_TYPE_OP;
				}
				else if (nextCh == '.')
				{
					mSourceFile->GetNextChar(); // consume '.'
					token = TOKEN_TYPE_DELIM_OPTIONAL_DOT;
				}
				else if (nextCh == '(')
				{
					// 不consume '('，只标记token类型
					token = TOKEN_TYPE_DELIM_OPTIONAL_CALL;
				}
				else if (nextCh == '[')
				{
					// 不consume '['，只标记token类型
					token = TOKEN_TYPE_DELIM_OPTIONAL_INDEX;
				}
				else if (nextCh == ':')
				{
					mSourceFile->GetNextChar(); // consume ':'
					token = TOKEN_TYPE_DELIM_OPTIONAL_COLON;
				}
				else
				{
					token = TOKEN_TYPE_DELIM_INTERROGATION;
				}
			}
			else if(lexeme[0] == ':')
				token = TOKEN_TYPE_DELIM_COLON;
			break;
		}
	case LEX_STATE_IDENT:
		{
			token = GetReserveType(lexeme);
			if (token == TOKEN_TYPE_INVALID)
			{
				token = TOKEN_TYPE_IDENT;
			}
			break;
		}
	case LEX_STATE_OP:
		{
			if (mOprType == OP_TYPE_FATARROW)
				token = TOKEN_TYPE_DELM_FATARROW;
			else
				token = TOKEN_TYPE_OP;
			break;
		}
	case LEX_STATE_LINE_COMMENT:
		{
			token = TOKEN_TYPE_END_OF_STREAM;
			break;
		}
	case LEX_STATE_SEGMENT_COMMENT:
		{
			token = TOKEN_TYPE_END_OF_STREAM;
			break;
		}
	default:
		token = TOKEN_TYPE_INVALID;
		break;
	}
	strncpy_s(mCurLexeme, lexeme, MAX_LEXEME_SIZE);
	mCurToken = token;

	if (mIsRecordToken)
	{
		AddRecordToken(token);
		mLastLookToken = false;
	}

	return token;
}

void    CLexer::RewindToken()
{
	if (mIsReadTokenFromRecord)
	{
		mReadTokeIndex--;
		if (mIsRecordToken)
		{
			int startStackIndex = mReadBufferStack.empty() ? 0 : mReadBufferStack.back().recordingStackSize;
			RewindRecordToken(startStackIndex);
		}
	} 
	else
	{
		mSourceFile->SetCurLineIndex(mLastLineIndex);
		mSourceFile->SetCurCharIndex(mLastCharIndex);
		if (mIsRecordToken)
		{
			RewindRecordToken();
		}
	}

}
TOKEN    CLexer::GetCurToken()
{
	return mCurToken;
}
const char*  CLexer::GetCurLexeme()
{
	if (mIsReadTokenFromRecord)
	{
		const TokenRecord* record = GetReadTokenRecord(mLastLookTokenInRead ? 0 : -1);
		if (record)
			return record->lexeme.c_str();
	}
	return mCurLexeme;
}
char    CLexer::GetAheadChar()
{
	GetNextToken();
	char c = mCurLexeme[0];
	RewindToken();
	return c;
}


int      CLexer::GetReserveType(const char *ident)
{
	struct ReserveTokenData
	{
		const char* name;
		int token;
	};

	static ReserveTokenData reserveTokens[] =
	{
		{ "var", TOKEN_TYPE_RSRVD_VAR },
		{ "true", TOKEN_TYPE_RSRVD_TRUE },
		{ "false", TOKEN_TYPE_RSRVD_FALSE },
		{ "if", TOKEN_TYPE_RSRVD_IF },
		{ "else", TOKEN_TYPE_RSRVD_ELSE },
		{ "break", TOKEN_TYPE_RSRVD_BREAK },
		{ "continue", TOKEN_TYPE_RSRVD_CONTINUE },
		{ "for", TOKEN_TYPE_RSRVD_FOR },
		{ "while", TOKEN_TYPE_RSRVD_WHILE },
		{ "function", TOKEN_TYPE_RSRVD_FUNC },
		{ "return", TOKEN_TYPE_RSRVD_RETURN },
		{ "nil", TOKEN_TYPE_RSRVD_NIL },
		{ "foreach", TOKEN_TYPE_RSRVD_FOREACH },
		{ "in", TOKEN_TYPE_RSRVD_IN },
		{ "lambda", TOKEN_TYPE_RSRVD_LAMBDA },
		{ "or", TOKEN_TYPE_RSRVD_OR },
		{ "async", TOKEN_TYPE_RSRVD_ASYNC },
		{ "await", TOKEN_TYPE_RSRVD_AWAIT },
		{ "try", TOKEN_TYPE_RSRVD_TRY },
		{ "catch", TOKEN_TYPE_RSRVD_CATCH },
		{ "throw", TOKEN_TYPE_RSRVD_THROW },
		{ "finally", TOKEN_TYPE_RSRVD_FINALLY },
		{ "defer", TOKEN_TYPE_RSRVD_DEFER },
		{ "iterator", TOKEN_TYPE_RSRVD_ITERATOR },
		{ "generator", TOKEN_TYPE_RSRVD_GENERATOR },
		{ "emit", TOKEN_TYPE_RSRVD_YIELD },
		{ "switch", TOKEN_TYPE_RSRVD_SWITCH },
		{ "case", TOKEN_TYPE_RSRVD_CASE },
		{ "default", TOKEN_TYPE_RSRVD_DEFAULT },
		{ "match", TOKEN_TYPE_RSRVD_MATCH},
	};

	int count = sizeof(reserveTokens) / sizeof(ReserveTokenData);
	for (int i = 0; i < count; i++)
	{
		if (_stricmp(ident, reserveTokens[i].name) == 0)
		{
			return reserveTokens[i].token;
		}
	}

	return TOKEN_TYPE_INVALID;
}

bool	CLexer::TestToken(int token)
{
	if (token == LookNextToken())
	{
		GetNextToken();
		return true;
	}
	return false;
}

bool    CLexer::ExpectToken(int token, int op, const char* errorDesc)
{
	if(token  != GetNextToken() || (op >= 0 && GetCurOprType() != op ))
	{
		char tokeName[MAX_TOKEN_NAME_SIZE] = {0};
		switch (token)
		{
		case TOKEN_TYPE_INT:
			{
				strncpy_s(tokeName, "int", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_FLOAT:
			{
				strncpy_s(tokeName, "float", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_IDENT:
			{
				strncpy_s(tokeName, "identifier", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_RSRVD_VAR:
			{
				strncpy_s(tokeName, "var", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_RSRVD_IF:
			{
				strncpy_s(tokeName, "if", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_RSRVD_ELSE:
			{
				strncpy_s(tokeName, "else", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_RSRVD_FOR:
			{
				strncpy_s(tokeName, "for", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_RSRVD_RETURN:
			{
				strncpy_s(tokeName, "return", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_DELIM_COMMA:
			{
				strncpy_s(tokeName, ",", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_DELIM_OPEN_PAREN:
			{
				strncpy_s(tokeName, "(", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_DELIM_CLOSE_PAREN:
			{
				strncpy_s(tokeName, ")", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_DELIM_OPEN_BRACE:
			{
				strncpy_s(tokeName, "[", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE:
			{
				strncpy_s(tokeName, "{", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE:
			{
				strncpy_s(tokeName, "}", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_DELIM_SEMICOLON:
			{
				strncpy_s(tokeName, ";", MAX_TOKEN_NAME_SIZE);
				break;
			}
		}
		char errInfo[256] = {0};
		if (errorDesc == nullptr)
		{ 
			sprintf(errInfo, "expected '%s', but got '%s'", tokeName, mCurLexeme[0] ? mCurLexeme : "<end of file>");
			ExitOnError(errInfo, mLastReadTokenLineIndex + 1, mLastReadTokenCharIndex + 1);
		}
		else
		{
			ExitOnError(errorDesc, mLastReadTokenLineIndex + 1, mLastReadTokenCharIndex + 1);
		}
		
	}
	return false;
}

void	CLexer::BackToken()
{
	RewindToken();
	LookNextToken();
}

void    CLexer::CopyLexme(char* buffer)
{
	_ASSERT(buffer);
	strcpy(buffer, mCurLexeme);
}

int     CLexer::GetCurOprType()
{
	if (mIsReadTokenFromRecord)
	{
		const TokenRecord* record = GetReadTokenRecord(mLastLookTokenInRead ? 0 : -1);
		if (record)
			return record->opType;
		return -1;
	}
	return mOprType;
}

XInt	CLexer::GetCurIntValue()
{
	const char* number = GetCurLexeme();
	if (strstr(number, "x") != NULL)
	{
		return (XInt)strtoll(GetCurLexeme(), NULL, 16);
	}
	else
	{
		return (XInt)strtoll(GetCurLexeme(), NULL, 10);
	}
}





void    CLexer::AddRecordToken(TOKEN token, int startStackIndex)
{
	if (mRecordingBufferStack.empty())
		return;

	if (startStackIndex < 0)
		startStackIndex = 0;
	if (startStackIndex >= (int)mRecordingBufferStack.size())
		return;

	TokenRecord record;
	record.token = token;
	record.lexeme = mCurLexeme;
	record.opType = mOprType;
	record.line = mLastReadTokenLineIndex;
	record.character = mLastReadTokenCharIndex;

	for (int i = startStackIndex; i < (int)mRecordingBufferStack.size(); i++)
	{
		int bufferIndex = mRecordingBufferStack[i];
		if (bufferIndex < 0 || bufferIndex >= (int)mTokenBuffers.size())
			continue;

		TokenBuffer& buffer = mTokenBuffers[bufferIndex];
		if (buffer.recordIndex < (int)buffer.tokens.size())
			buffer.tokens[buffer.recordIndex] = record;
		else
			buffer.tokens.push_back(record);
		buffer.recordIndex++;
	}

	SyncRecordTokenState();
}

void    CLexer::RewindRecordToken(int startStackIndex)
{
	if (startStackIndex < 0)
		startStackIndex = 0;
	if (startStackIndex >= (int)mRecordingBufferStack.size())
		return;

	for (int i = startStackIndex; i < (int)mRecordingBufferStack.size(); i++)
	{
		int bufferIndex = mRecordingBufferStack[i];
		if (bufferIndex < 0 || bufferIndex >= (int)mTokenBuffers.size())
			continue;

		TokenBuffer& buffer = mTokenBuffers[bufferIndex];
		if (buffer.recordIndex > 0)
			buffer.recordIndex--;
	}

	SyncRecordTokenState();
}

void    CLexer::SyncRecordTokenState()
{
	mIsRecordToken = !mRecordingBufferStack.empty();
	if (mIsRecordToken)
	{
		mRecordingBufferIndex = mRecordingBufferStack.back();
		if (mRecordingBufferIndex >= 0 && mRecordingBufferIndex < (int)mTokenBuffers.size())
			mRecordTokenIndex = mTokenBuffers[mRecordingBufferIndex].recordIndex;
		else
			mRecordTokenIndex = 0;
	}
	else
	{
		mRecordingBufferIndex = -1;
		mRecordTokenIndex = 0;
	}
}

const CLexer::TokenRecord* CLexer::GetReadTokenRecord(int offset) const
{
	if (mReadingBufferIndex < 0 || mReadingBufferIndex >= (int)mTokenBuffers.size())
		return NULL;

	int index = mReadTokeIndex + offset;
	const TokenBuffer& buffer = mTokenBuffers[mReadingBufferIndex];
	if (index < 0 || index >= (int)buffer.tokens.size())
		return NULL;

	return &buffer.tokens[index];
}

int    CLexer::beginRecordTokenBuffer()
{
	mCurRecordDepth++;
	mLastLookToken = false;

	TokenBuffer buffer;
	mTokenBuffers.push_back(buffer);
	mRecordingBufferIndex = (int)mTokenBuffers.size() - 1;
	mRecordingBufferStack.push_back(mRecordingBufferIndex);
	mLastRecordBufferIndex = mRecordingBufferIndex;
	SyncRecordTokenState();
	return mRecordingBufferIndex;
}

void    CLexer::endRecordTokenBuffer()
{
	if (mRecordingBufferIndex >= 0 && mRecordingBufferIndex < (int)mTokenBuffers.size())
	{
		TokenBuffer& buffer = mTokenBuffers[mRecordingBufferIndex];
		if (buffer.recordIndex < (int)buffer.tokens.size())
			buffer.tokens.resize(buffer.recordIndex);
	}
	if (!mRecordingBufferStack.empty())
		mRecordingBufferStack.pop_back();
	SyncRecordTokenState();
	mLastLookToken = false;
}

void    CLexer::beginReadTokenFromBuffer(int bufferIndex)
{
	if (mIsReadTokenFromRecord && !mReadBufferStack.empty())
	{
		ReadBufferState& currentState = mReadBufferStack.back();
		currentState.bufferIndex = mReadingBufferIndex;
		currentState.readIndex = mReadTokeIndex;
		currentState.lastLookTokenInRead = mLastLookTokenInRead;
	}

	ReadBufferState state;
	state.bufferIndex = bufferIndex;
	state.readIndex = 0;
	state.lastLookTokenInRead = false;
	state.recordingStackSize = (int)mRecordingBufferStack.size();
	mReadBufferStack.push_back(state);

	mLastLookTokenInRead = false;
	mIsReadTokenFromRecord = true;
	mReadTokeIndex = 0;
	mReadingBufferIndex = bufferIndex;
}

void    CLexer::clearTokenBuffer(int bufferIndex)
{
	if (bufferIndex < 0 || bufferIndex >= (int)mTokenBuffers.size())
		return;
	mTokenBuffers[bufferIndex].tokens.clear();
	mTokenBuffers[bufferIndex].recordIndex = 0;
}

void    CLexer::clearAllTokenBuffers()
{
	mTokenBuffers.clear();
	mRecordingBufferStack.clear();
	mReadBufferStack.clear();
	mRecordTokenIndex = 0;
	mReadTokeIndex = 0;
	mRecordingBufferIndex = -1;
	mReadingBufferIndex = -1;
	mLastRecordBufferIndex = -1;
	mLastLookToken = false;
	mLastLookTokenInRead = false;
	mIsRecordToken = false;
	mIsReadTokenFromRecord = false;
}

void    CLexer::beginRecordToken()
{
	beginRecordTokenBuffer();
}


void    CLexer::endRecordToken()
{
	endRecordTokenBuffer();
}

void    CLexer::beginReadTokenFromRecord()
{
	beginReadTokenFromBuffer(mLastRecordBufferIndex);
}

void    CLexer::endReadTokenFromRecord()
{
	if (!mReadBufferStack.empty())
	{
		mReadBufferStack.pop_back();
	}

	if (!mReadBufferStack.empty())
	{
		ReadBufferState state = mReadBufferStack.back();
		mReadingBufferIndex = state.bufferIndex;
		mReadTokeIndex = state.readIndex;
		mLastLookTokenInRead = state.lastLookTokenInRead;
		mIsReadTokenFromRecord = true;
	}
	else
	{
		mIsReadTokenFromRecord = false;
		mReadTokeIndex = 0;
		mReadingBufferIndex = -1;
		mLastLookTokenInRead = false;
	}

	if (mCurRecordDepth > 0)
		mCurRecordDepth--;
}
