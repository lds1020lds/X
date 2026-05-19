#include "Parser.h"
#include "SourceFile.h"
#include "Lexer.h"
#include "MidCode.h"
#include "SymbolTable.h"
#include "XScriptVM.h"

// 去掉内部标识符前缀，使中间代码输出更简洁
static const char* StripInternalPrefix(const char* name)
{
	const size_t prefixLen = strlen(XSCRIPT_INTERNAL_IDENT_PREFIX);
	if (strncmp(name, XSCRIPT_INTERNAL_IDENT_PREFIX, prefixLen) == 0)
		return name + prefixLen;
	return name;
}

// 输出中间代码到文件（用于调试）
void CParser::OutPutCode(CMidCode *midCode, CSymbolTable* symbolTable, char* outputName)
{
	FILE *fp = NULL;
	fopen_s(&fp, outputName, "wb");
	char  codeText[256] = {0};
	for (int iFuncIndex = 0; iFuncIndex < (int)mSymbolTable->mFuncTable.size(); iFuncIndex++)
	{
		int curFuncIndex = mSymbolTable->mFuncTable[iFuncIndex].iIndex;

		snprintf(codeText, 256, "FUNC	%s\r\n", StripInternalPrefix(mSymbolTable->mFuncTable[iFuncIndex].funcName));
		fwrite(codeText, 1, strlen(codeText), fp);
		if (iFuncIndex > 0)
		{
			std::string code;
			bool isFirst = true;
			int curScope = mSymbolTable->mFuncTable[iFuncIndex].iIndex;
			const ScopeVarTable* scopeVars = symbolTable->GetScopeVars(curScope);
			if (scopeVars != nullptr)
			{
				for (auto* var : scopeVars->varList)
				{
					if (var->iType == IDENT_TYPE_PARAM)
					{
						if (isFirst)
							code += "PARAM	";
						else
							code += ",	";
						isFirst = false;
						code += StripInternalPrefix(var->varName);
					}
				}
			}
			if (!isFirst)
			{
				code += "\r\n";
				fwrite(code.c_str(), 1, code.size(), fp);
			}
			code = "";
			isFirst = true;
			if (scopeVars != nullptr)
			{
				for (auto* var : scopeVars->varList)
				{
					if (var->iType == IDENT_TYPE_VAR || var->iType == IDENT_TYPE_INTERNAL_VAR)
					{
						if (isFirst)
							code += "VAR\t";
						else
							code += ",\t";
						isFirst = false;
						code += StripInternalPrefix(var->varName);
					}
				}
			}
			if (!isFirst)
			{
				code += "\r\n";
				fwrite(code.c_str(), 1, code.size(), fp);
			}		}
		

		int numCodeSize = (int)mMidCode->mCodeList[curFuncIndex].size();
		//int iCodeStart = mSymbolTable->mFuncTable[iFuncIndex].iCodeStartIndex;
		//int iCodeEnd  = mSymbolTable->mFuncTable[iFuncIndex].iCodeEndIndex;
		for (int iInstrIndex = 0; iInstrIndex < numCodeSize; iInstrIndex++)
		{
			for (int i = 0; i < (int)mMidCode->mJumpList[curFuncIndex].size(); i++)
			{
				if (mMidCode->mJumpList[curFuncIndex][i].codeIndex == iInstrIndex)
				{
					snprintf(codeText, 256, "  Label%d:\r\n", mMidCode->mJumpList[curFuncIndex][i].jumpIndex);
					fwrite(codeText, 1, strlen(codeText), fp);
				}
			}

			

			ICode code = mMidCode->mCodeList[curFuncIndex][iInstrIndex];
			snprintf(codeText, 256, "%d: [line %d]\t%s", iInstrIndex, code.lineIndex, GetCodeText(code, curFuncIndex));


			fwrite(codeText, 1, strlen(codeText), fp);
		}
		strncpy_s(codeText, "END  FUNC\r\n\r\n", 256);
		fwrite(codeText, 1, strlen(codeText), fp);
	}
	fclose(fp);
}

// 获取单条中间码的文本表示（用于调试输出）
char*  CParser::GetCodeText(const ICode  &code, int funcIndex)
{
	// 指令码 -> 文本名称 的静态映射表
	static const struct { int instrCode; const char* text; } s_instrTextMap[] = {
		{ INSTR_CONCAT_TO, "CONCAT" }, { INSTR_CONCAT,    "CONCAT" },
		{ INSTR_FUNC,      "FUNC"   },
		{ INSTR_TEST_NE,   "TNE"    }, { INSTR_TEST_E,    "TE"     },
		{ INSTR_TEST_G,    "TG"     }, { INSTR_TEST_L,    "TL"     },
		{ INSTR_TEST_GE,   "TGE"    }, { INSTR_TEST_LE,   "TLE"    },
		{ INSTR_LOGIC_AND, "LOGICAND" }, { INSTR_LOGIC_OR, "LOGICOR" },
		{ INSTR_APPEND,    "APPEND" },
		{ INSTR_MOV,       "MOV"    },
		{ INSTR_ADD,       "ADD"    }, { INSTR_ADD_TO,    "ADD"    },
		{ INSTR_SUB_TO,    "SUB"    }, { INSTR_SUB,       "SUB"    },
		{ INSTR_MUL_TO,    "MUL"    }, { INSTR_MUL,       "MUL"    },
		{ INSTR_DIV_TO,    "DIV"    }, { INSTR_DIV,       "DIV"    },
		{ INSTR_MOD_TO,    "MOD"    }, { INSTR_MOD,       "MOD"    },
		{ INSTR_EXP_TO,    "EXP"    }, { INSTR_EXP,       "EXP"    },
		{ INSTR_NEG,       "NEC"    },
		{ INSTR_INC,       "INC"    }, { INSTR_DEC,       "DEC"    },
		{ INSTR_AND,       "AND"    },
		{ INSTR_OR_TO,     "OR"     }, { INSTR_OR,        "OR"     },
		{ INSTR_XOR_TO,    "XOR"    }, { INSTR_XOR,       "XOR"    },
		{ INSTR_SHL_TO,    "SHL"    }, { INSTR_SHL,       "SHL"    },
		{ INSTR_SHR_TO,    "SHR"    }, { INSTR_SHR,       "SHR"    },
		{ INSTR_JMP,       "JMP"    },
		{ INSTR_JE,        "JE"     }, { INSTR_JNE,       "JNE"    },
		{ INSTR_JG,        "JG"     }, { INSTR_JL,        "JL"     },
		{ INSTR_JGE,       "JGE"    }, { INSTR_JLE,       "JLE"    },
		{ INSTR_JFALSE,    "JF"     }, { INSTR_JTRUE,     "JT"     },
		{ INSTR_PUSH,      "PUSH"   }, { INSTR_POP,       "POP"    },
		{ INSTR_CALL,      "CALL"   }, { INSTR_RET,       "RET"    },
		{ INSTR_TYPE,      "TYPE"   },
		{ INSTR_AND_TO,    "AND"    }, { INSTR_NOT_TO,    "NOT"    },
		{ INSTR_LOGIC_NOT, "LNOT"   }, { INSTR_ENTERCATCH, "ENTER_TRY"},
		{ INSTR_LOADNIL,   "LOADNIL"},  { INSTR_EXITCATCH, "EXIT_TRY"},
		{ INSTR_THROW, "THROW"},	{ INSTR_AWAIT, "AWAIT"},
		{ INSTR_J_ARGS_G, "J_ARGS_G"},{ INSTR_DEFER, "DEFER"},
		{ INSTR_YIELD, "YIELD"},	{ INSTR_SWITCH, "SWITCH"},
		{ INSTR_J_NOT_TYPE, "JNTYPE"},
	};

	char oprText[32] = {0};
	for (const auto& entry : s_instrTextMap)
	{
		if (entry.instrCode == code.code.iCodeOpr)
		{
			strncpy_s(oprText, entry.text, 32);
			break;
		}
	}

	static char codeText[1024];
	memset(codeText, 0, sizeof(codeText));
	strcat_s(codeText, oprText);
	for (int paramIndex = 0; paramIndex < (int)code.code.oprList.size(); paramIndex++)
	{
		Operand operand = code.code.oprList[paramIndex];
		char operandText[256] = {0};
		if (operand.operandType == PST_Int)
		{
#ifdef USE_HIGH_PRECIOUS_NUMBER
			snprintf(operandText, 256, "	%lld", operand.iIntValue);
#else
			snprintf(operandText, 256, "	%d", operand.iIntValue);
#endif

		}
		else if (operand.operandType == PST_Float)
		{
#ifdef USE_HIGH_PRECIOUS_NUMBER
				snprintf(operandText, 256, "	%lf", operand.fFloatValue);
#else
				snprintf(operandText, 256, "	%f", operand.fFloatValue);
#endif
			
		}
		else if (operand.operandType == PST_String_Index)
		{
			snprintf(operandText, 256, "	\"%s\"", mSymbolTable->mStringTable[operand.iStringIndex].str);
		}
		else if (operand.operandType == PST_Var_Index)
		{
			const char* varName = StripInternalPrefix(FindVarNameBySymbolIndex(*mSymbolTable, operand.iSymbolIndex, funcIndex));
			snprintf(operandText, 256, "	%s", varName);
		}
		else if (operand.operandType == PST_Table)
		{
			const char* varName1 = StripInternalPrefix(FindVarNameBySymbolIndex(*mSymbolTable, operand.iSymbolIndex, funcIndex));
			switch (operand.tableIndexType)
			{
			case ROT_Float:
			{
#ifdef USE_HIGH_PRECIOUS_NUMBER
				snprintf(operandText, 256, "	%s[%lf]", varName1, operand.fFloatTableValue);
#else
				snprintf(operandText, 256, "	%s[%f]", varName1, operand.fFloatTableValue);
#endif
				
			}
			break;
			case ROT_Int:
			{
#ifdef USE_HIGH_PRECIOUS_NUMBER
				snprintf(operandText, 256, "	%s[%lld]", varName1, operand.iIntTableValue);
#else
				snprintf(operandText, 256, "	%s[%d]", varName1, operand.iIntTableValue);
#endif
			}
			break;
			case ROT_String:
			{
				snprintf(operandText, 256, "	%s[\"%s\"]", varName1, mSymbolTable->getString((int)operand.iIntTableValue));
			}
			break;
			case ROT_Stack_Index:
			{
				const char* varName2 = StripInternalPrefix(FindVarNameBySymbolIndex(*mSymbolTable, (int)operand.iIntTableValue, funcIndex));
				snprintf(operandText, 256, "	%s[%s]", varName1, varName2);
			}
			break;
			case ROT_Reg:
			{
				snprintf(operandText, 256, "	%s[REG%d]", varName1, (int)operand.iIntTableValue);
			}
			}
		}
		else if (operand.operandType == PST_JumpTarget_Index)
		{
			snprintf(operandText, 256, "	Label%d", operand.iJumppIndex);
		}
		else if (operand.operandType == PST_Reg)
		{
			snprintf(operandText, 256, "	 REG%d", operand.iRegIndex);
		}
		else if (operand.operandType == PST_Nil)
		{
			snprintf(operandText, 256, "	nil");
		}

		if (paramIndex < (int)code.code.oprList.size() - 1)
			strncat_s(operandText, ",", 256);
		strncat_s(codeText, operandText, 256);
	}

	if (code.code.iCodeOpr == INSTR_FUNC && code.code.oprList.size() > 1)
	{
		FunctionST* func = mSymbolTable->GetFunctionByIndex(funcIndex);
		int subFuncIndex = func->subIndexVec[(int)code.code.oprList[1].iIntValue];
		FunctionST* subFunc = mSymbolTable->GetFunctionByIndex(subFuncIndex);
		std::string desc = " (" + std::string(StripInternalPrefix(subFunc->funcName)) + ")";
		strncat_s(codeText, desc.c_str(), 256);
	}

	strncat_s(codeText, "\r\n", 256);

	
	return codeText;
}


// 根据符号索引查找变量名（支持上值查找）
char* CParser::FindVarNameBySymbolIndex(CSymbolTable& symbolTable, int iSymbolIndex, int funcIndex)
{
	if (iSymbolIndex & UPVALMASK)
	{
		int searchFuncIndex = funcIndex;
		int upindex = iSymbolIndex - UPVALMASK;

		FunctionST* func = symbolTable.GetFunctionByIndex(funcIndex);
		while (func->upValueVec[upindex].type != VLOCAL && func->parentIndex >= 0)
		{
			upindex = func->upValueVec[upindex].index;
			func = symbolTable.GetFunctionByIndex(func->parentIndex);
		}

		if (func->parentIndex >= 0 && func->upValueVec[upindex].type == VLOCAL)
		{
			VariantST* var = symbolTable.GetVarByIndex(func->upValueVec[upindex].index);
			return var->varName;
		}
		return "";
	}
	else
	{
		VariantST* var = symbolTable.GetVarByIndex(iSymbolIndex);
		return var->varName;
	}
}