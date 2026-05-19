#include "Parser.h"
#include "SourceFile.h"
#include "Lexer.h"
#include "MidCode.h"
#include "SymbolTable.h"
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include "XScriptVM.h"
jmp_buf setjmp_buffer;


// 构造函数
CParser::CParser(void)
	: mRecoverMode(false)
	, mMaxRecoverErrors(50)
	, mLastFreeIndex(0)
	, mRegAllocFuncIndex(0)
	, mCurTryCatchBlockIndex(-1)
{
}

// 析构函数
CParser::~CParser(void)
{
}

// 判断变量索引是否为寄存器变量
bool  CParser::IsRegVar(int varIndex)
{
	for (int i = 0; i < MAX_REG_NUM; i++)
	{
		if (mReg[i].mVarIndex == varIndex)
		{
			return true;
		}
	}

	return false;
}

// 获取一个空闲寄存器，返回其对应的变量索引
int    CParser::GetFreeRegImpl(const char* file, int line)
{
	int lastIndex = -1;
	int lasFreeIndex = -1;
	for (int i = 0; i < MAX_REG_NUM; i++)
	{
		if (mReg[i].bUsed == false)
		{
			if (mReg[i].mLastIndex > lasFreeIndex)
			{
				lastIndex = i;
				lasFreeIndex = mReg[i].mLastIndex;
			}
			
		}
	}
	
	if (lastIndex >= 0)
	{
		if (mReg[lastIndex].mVarIndex < 0)
		{
			char regName[64] = { 0 };
			snprintf(regName, 64, "%sR_%d", XSCRIPT_INTERNAL_IDENT_PREFIX, lastIndex);
			mReg[lastIndex].mVarIndex = mSymbolTable->AddVariant(regName, mRegAllocFuncIndex, 0, IDENT_TYPE_INTERNAL_VAR);
		}
		mReg[lastIndex].bUsed = true;
#ifdef _DEBUG
		mReg[lastIndex].allocFile = file;
		mReg[lastIndex].allocLine = line;
#endif
		return mReg[lastIndex].mVarIndex;
	}
	else
	{
		Error("internal error: no free register available, expression may be too complex");
	}

	return lastIndex;
}

// 确保因子是变量类型，必要时生成MOV指令存入临时寄存器
void CParser::EnsureFactorIsVar(Factor& factor)
{
	if (factor.type != FACTOR_VAR)
	{
		int instrIndex = mMidCode->AddInstr(INSTR_MOV);
		int freeReg = GET_FREE_REG();
		mMidCode->AddVarOperand(instrIndex, freeReg);
		AddOperandByFactor(instrIndex, factor);
		FreeFactorReg(factor);
		factor.type = FACTOR_VAR;
		factor.varIndex = freeReg;
	}
}

// 固化延迟消费的函数返回值，避免后续函数调用覆盖共享返回寄存器
void CParser::StabilizePendingFunctionReturns(std::vector<Factor>& factors)
{
	for (auto& factor : factors)
	{
		if (factor.type == FACTOR_FUNC)
		{
			EnsureFactorIsVar(factor);
		}
	}
}

// 注册当前解析作用域中需要保护的延迟消费因子列表
void CParser::RegisterPendingFunctionReturnFactors(std::vector<Factor>& factors)
{
	mPendingFunctionReturnFactorStack.push_back(&factors);
}

CParser::PendingFunctionReturnFactorScope::PendingFunctionReturnFactorScope(CParser* _parser, std::vector<Factor>& _factors)
	: parser(_parser), factors(_factors)
{
	parser->RegisterPendingFunctionReturnFactors(factors);
}

CParser::PendingFunctionReturnFactorScope::~PendingFunctionReturnFactorScope()
{
	parser->UnregisterPendingFunctionReturnFactors(factors);
}

// 注册当前解析作用域中需要保护的单个延迟消费因子
void CParser::RegisterPendingFunctionReturnFactor(Factor& factor)
{
	mPendingFunctionReturnSingleFactorStack.push_back(&factor);
}

CParser::PendingFunctionReturnSingleFactorScope::PendingFunctionReturnSingleFactorScope(CParser* _parser, Factor& _factor)
	: parser(_parser), factor(_factor)
{
	parser->RegisterPendingFunctionReturnFactor(factor);
}

CParser::PendingFunctionReturnSingleFactorScope::~PendingFunctionReturnSingleFactorScope()
{
	parser->UnregisterPendingFunctionReturnFactor(factor);
}

// 反注册当前解析作用域中需要保护的单个延迟消费因子
void CParser::UnregisterPendingFunctionReturnFactor(Factor& factor)
{
	for (int i = (int)mPendingFunctionReturnSingleFactorStack.size() - 1; i >= 0; i--)
	{
		if (mPendingFunctionReturnSingleFactorStack[i] == &factor)
		{
			mPendingFunctionReturnSingleFactorStack.erase(mPendingFunctionReturnSingleFactorStack.begin() + i);
			return;
		}
	}
}

// 反注册当前解析作用域中需要保护的延迟消费因子列表
void CParser::UnregisterPendingFunctionReturnFactors(std::vector<Factor>& factors)
{
	for (int i = (int)mPendingFunctionReturnFactorStack.size() - 1; i >= 0; i--)
	{
		if (mPendingFunctionReturnFactorStack[i] == &factors)
		{
			mPendingFunctionReturnFactorStack.erase(mPendingFunctionReturnFactorStack.begin() + i);
			return;
		}
	}
}

// 发射函数调用前固化所有活跃作用域中延迟消费的函数返回值
void CParser::BeforeEmitCall()
{
	//录制状态 不会影响到寄存器， 要忽略掉，否则会得到一个-1的变量
	if (mIsRecordToken)
		return;

	for (auto factor : mPendingFunctionReturnSingleFactorStack)
	{
		if (factor->type == FACTOR_FUNC)
		{
			EnsureFactorIsVar(*factor);
		}
	}

	for (auto factors : mPendingFunctionReturnFactorStack)
	{
		StabilizePendingFunctionReturns(*factors);
	}
}

// 折叠表索引：将表[索引]访问的结果存入临时寄存器
void CParser::FoldTableIndex(ChainedAccess& access)
{
	if (access.HasAccessor())
	{
		int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
		int req = GET_FREE_REG();
		mMidCode->AddVarOperand(iInstrIndex, req);
		AddTableFactor(access.accessor, iInstrIndex, access.subject.varIndex);
		FreeFactorReg(access.accessor);
		FreeFactorReg(access.subject);
		access.subject.type = FACTOR_VAR;
		access.subject.varIndex = req;
		access.accessor.type = FACTOR_INVALID;
	}
}

// 释放因子占用的寄存器资源
void		CParser::FreeFactorReg(const Factor& factor)
{
	if (factor.type == FACTOR_VAR)
		FREE_REG(factor.varIndex);
	
	if (factor.type == FACTOR_TABLE)
	{
		FREE_REG(factor.varIndex);

		if (factor.iOffset == ROT_Stack_Index)
		{
			FREE_REG((int)factor.intTableValue);
		}
	}

}

// 释放指定寄存器，标记为未使用
void CParser::FreeRegImpl(int reg, const char* file, int line)
{
	for (int i = 0; i < MAX_REG_NUM; i++)
	{
		if (mReg[i].mVarIndex == reg)
		{
			mReg[i].bUsed = false;
			mReg[i].mLastIndex = mLastFreeIndex;
			mLastFreeIndex++;
#ifdef _DEBUG
			mReg[i].allocFile = nullptr;
			mReg[i].allocLine = 0;
#endif
		}
	}
}

#ifdef _DEBUG
// 检查当前是否有寄存器泄漏，输出未释放寄存器的分配位置
void CParser::CheckRegLeak(const char* context)
{
	for (int i = 0; i < MAX_REG_NUM; i++)
	{
		if (mReg[i].bUsed)
		{
			const char* fileName = mReg[i].allocFile ? mReg[i].allocFile : "unknown";
			int lineNum = mReg[i].allocLine;
			fprintf(stderr, "[REG LEAK] Register _R%d (varIndex=%d) leaked at %s:%d [context: %s]\n",
				i, mReg[i].mVarIndex, fileName, lineNum, context);
		}
	}
}
#endif

void CParser::InitRegAllocContextForFunction(int funcIndex)
{
	mRegAllocFuncIndex = funcIndex;
	mLastFreeIndex = 0;
	for (int i = 0; i < MAX_REG_NUM; i++)
	{
		mReg[i].bUsed = false;
		mReg[i].mLastIndex = 0;
		mReg[i].mVarIndex = -1;
#ifdef _DEBUG
		mReg[i].allocFile = nullptr;
		mReg[i].allocLine = 0;
#endif
	}
}

CParser::RegAllocContext CParser::SaveRegAllocContext() const
{
	RegAllocContext context;
	for (int i = 0; i < MAX_REG_NUM; i++)
		context.reg[i] = mReg[i];
	context.lastFreeIndex = mLastFreeIndex;
	context.funcIndex = mRegAllocFuncIndex;
#ifdef _DEBUG
	for (int i = 0; i < MAX_REG_NUM; i++)
	{
		context.allocFiles[i] = mReg[i].allocFile;
		context.allocLines[i] = mReg[i].allocLine;
	}
#endif
	return context;
}

void CParser::RestoreRegAllocContext(const RegAllocContext& context)
{
#ifdef _DEBUG
	// 恢复前检查是否有寄存器泄漏
	CheckRegLeak("function return");
#endif
	for (int i = 0; i < MAX_REG_NUM; i++)
		mReg[i] = context.reg[i];
	mLastFreeIndex = context.lastFreeIndex;
	mRegAllocFuncIndex = context.funcIndex;
#ifdef _DEBUG
	for (int i = 0; i < MAX_REG_NUM; i++)
	{
		mReg[i].allocFile = context.allocFiles[i];
		mReg[i].allocLine = context.allocLines[i];
	}
#endif
}

// 常量折叠：在编译期对两个常量因子进行算术/比较运算
// 比较运算辅助函数：对两个值执行比较运算，返回 0 或 1，不匹配返回 -1
template<typename T>
static int ComputeCompare(int computeWay, T v1, T v2)
{
	switch (computeWay)
	{
	case OP_TYPE_EQUAL:         return (v1 == v2) ? 1 : 0;
	case OP_TYPE_NOT_EQUAL:     return (v1 != v2) ? 1 : 0;
	case OP_TYPE_LESS:          return (v1 <  v2) ? 1 : 0;
	case OP_TYPE_GREATER:       return (v1 >  v2) ? 1 : 0;
	case OP_TYPE_LESS_EQUAL:    return (v1 <= v2) ? 1 : 0;
	case OP_TYPE_GREATER_EQUAL: return (v1 >= v2) ? 1 : 0;
	default: return -1;
	}
}

CParser::Factor CParser::ComputeFactors(int computeWay, Factor const& factor1, Factor const & factor2)
{
	Factor ret;

	// Both operands are INT: compute directly with integer arithmetic
	if (factor1.type == FACTOR_INT && factor2.type == FACTOR_INT)
	{
		XInt v1 = factor1.intValue;
		XInt v2 = factor2.intValue;

		if (computeWay == OP_TYPE_ADD)
		{
			ret.type = FACTOR_INT;
			ret.intValue = v1 + v2;
		}
		else if (computeWay == OP_TYPE_SUB)
		{
			ret.type = FACTOR_INT;
			ret.intValue = v1 - v2;
		}
		else if (computeWay == OP_TYPE_MUL)
		{
			ret.type = FACTOR_INT;
			ret.intValue = v1 * v2;
		}
		else if (computeWay == OP_TYPE_DIV)
		{
			// Integer division may produce float result
			if (v2 != 0)
			{
				ret.type = FACTOR_INT;
				ret.intValue = v1 / v2;
			}
		}
		else if (computeWay == OP_TYPE_MOD)
		{
			ret.type = FACTOR_INT;
			ret.intValue = v1 % v2;
		}
		else if (computeWay == OP_TYPE_EXP)
		{
			XFloat fResult = pow((XFloat)v1, (XFloat)v2);
			if (v2 >= 0 && fResult == (XInt)fResult)
			{
				ret.type = FACTOR_INT;
				ret.intValue = (XInt)fResult;
			}
			else
			{
				ret.type = FACTOR_FLOAT;
				ret.floatValue = fResult;
			}
		}
		else if (computeWay == OP_TYPE_BITWISE_AND)
		{
			ret.type = FACTOR_INT;
			ret.intValue = v1 & v2;
		}
		else if (computeWay == OP_TYPE_BITWISE_OR)
		{
			ret.type = FACTOR_INT;
			ret.intValue = v1 | v2;
		}
		else if (computeWay == OP_TYPE_BITWISE_XOR)
		{
			ret.type = FACTOR_INT;
			ret.intValue = v1 ^ v2;
		}
		else if (computeWay == OP_TYPE_BITWISE_SHIFT_LEFT)
		{
			ret.type = FACTOR_INT;
			ret.intValue = v1 << v2;
		}
		else if (computeWay == OP_TYPE_BITWISE_SHIFT_RIGHT)
		{
			ret.type = FACTOR_INT;
			ret.intValue = v1 >> v2;
		}
		else
		{
			int cmpResult = ComputeCompare(computeWay, v1, v2);
			if (cmpResult >= 0)
			{
				ret.type = FACTOR_INT;
				ret.intValue = cmpResult;
			}
		}

		return ret;
	}

	// At least one operand is FLOAT: promote to float arithmetic
	XFloat fv1 = (factor1.type == FACTOR_FLOAT) ? factor1.floatValue : (XFloat)factor1.intValue;
	XFloat fv2 = (factor2.type == FACTOR_FLOAT) ? factor2.floatValue : (XFloat)factor2.intValue;
	XFloat fResult = 0.0f;

	if (computeWay == OP_TYPE_ADD)
		fResult = fv1 + fv2;
	else if (computeWay == OP_TYPE_SUB)
		fResult = fv1 - fv2;
	else if (computeWay == OP_TYPE_MUL)
		fResult = fv1 * fv2;
	else if (computeWay == OP_TYPE_DIV)
		fResult = fv1 / fv2;
	else if (computeWay == OP_TYPE_MOD)
		fResult = XFMod(fv1, fv2);
	else if (computeWay == OP_TYPE_EXP)
		fResult = pow(fv1, fv2);
	else if (computeWay == OP_TYPE_BITWISE_AND)
	{
		ret.type = FACTOR_INT;
		ret.intValue = ((XInt)fv1) & ((XInt)fv2);
		return ret;
	}
	else if (computeWay == OP_TYPE_BITWISE_OR)
	{
		ret.type = FACTOR_INT;
		ret.intValue = ((XInt)fv1) | ((XInt)fv2);
		return ret;
	}
	else if (computeWay == OP_TYPE_BITWISE_XOR)
	{
		ret.type = FACTOR_INT;
		ret.intValue = ((XInt)fv1) ^ ((XInt)fv2);
		return ret;
	}
	else if (computeWay == OP_TYPE_BITWISE_SHIFT_LEFT)
	{
		ret.type = FACTOR_INT;
		ret.intValue = ((XInt)fv1) << ((XInt)fv2);
		return ret;
	}
	else if (computeWay == OP_TYPE_BITWISE_SHIFT_RIGHT)
	{
		ret.type = FACTOR_INT;
		ret.intValue = ((XInt)fv1) >> ((XInt)fv2);
		return ret;
	}
	else
	{
		int cmpResult = ComputeCompare(computeWay, fv1, fv2);
		if (cmpResult >= 0)
		{
			ret.type = FACTOR_INT;
			ret.intValue = cmpResult;
			return ret;
		}
	}

	// Check if float result is actually an integer
	if (fResult == (XInt)fResult)
	{
		ret.type = FACTOR_INT;
		ret.intValue = (XInt)fResult;
	}
	else
	{
		ret.type = FACTOR_FLOAT;
		ret.floatValue = fResult;
	}

	return ret;
}

// 解析源文件入口：初始化词法分析器、符号表、寄存器，然后逐条解析语句
bool CParser::ParseFile(CSourceFile* sourefile, CMidCode* midCode, CSymbolTable* symbol)
{
	return ParseFile(sourefile, midCode, symbol, false, false);
}

bool CParser::ParseFile(CSourceFile* sourefile, CMidCode* midCode, CSymbolTable* symbol, bool recoverMode)
{
	return ParseFile(sourefile, midCode, symbol, recoverMode, false);
}

// 解析源文件入口：初始化词法分析器、符号表、寄存器，然后逐条解析语句
bool  CParser::ParseFile(CSourceFile* sourefile, CMidCode* midCode, CSymbolTable* symbol, bool recoverMode, bool syntaxOnly)
{
	CLexer lexer(sourefile);
	mMidCode= midCode;
	mLexer = &lexer;
	mOriginLexer = &lexer;
	mSymbolTable = symbol;
	mRecoverMode = recoverMode;
	mParseErrors.clear();
	midCode->SetLex(mLexer);

	mCurFuncIndex = 0;
	InitRegAllocContextForFunction(mCurFuncIndex);

	if (!mRecoverMode)
	{
		if (setjmp(setjmp_buffer))
		{
			return false;
		}
		
		while (lexer.LookNextToken() != TOKEN_TYPE_END_OF_STREAM)
		{
			ParseStateMent();
		}
	}
	else
	{
		while (lexer.LookNextToken() != TOKEN_TYPE_END_OF_STREAM && (int)mParseErrors.size() < mMaxRecoverErrors)
		{
			ParseStateMentWithRecovery();
		}
		if (!mParseErrors.empty())
			return false;
	}
	
	mMidCode->setCurFuncIndex(0);
	if (mMidCode->getLastInstrOp() != INSTR_RET)
	{ 
		int iInstrIndex = mMidCode->AddInstr(INSTR_RET);
		mMidCode->AddIntOperand(iInstrIndex, 0);
	}

	if (!syntaxOnly)
	{
		mSymbolTable->computeParmamStackIndex();
		optimizeCode();
	}
#ifdef _DEBUG
	CheckRegLeak("parse file end");
#endif
	return true;
}
// 记录最近一次解析错误
void CParser::RecordLastParseError()
{
	ParseError err;
	err.line = gLastParseErrorLine;
	err.character = gLastParseErrorChar;
	err.message = gLastParseErrorInfo;
	mParseErrors.push_back(err);
}

// 判断 token 是否可能是语句起点
bool CParser::IsStatementStartToken(TOKEN token)
{
	return token == TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE
		|| token == TOKEN_TYPE_RSRVD_ASYNC
		|| token == TOKEN_TYPE_RSRVD_GENERATOR
		|| token == TOKEN_TYPE_RSRVD_FUNC
		|| token == TOKEN_TYPE_RSRVD_VAR
		|| token == TOKEN_TYPE_RSRVD_IF
		|| token == TOKEN_TYPE_RSRVD_WHILE
		|| token == TOKEN_TYPE_RSRVD_FOR
		|| token == TOKEN_TYPE_RSRVD_FOREACH
		|| token == TOKEN_TYPE_RSRVD_BREAK
		|| token == TOKEN_TYPE_RSRVD_CONTINUE
		|| token == TOKEN_TYPE_RSRVD_RETURN
		|| token == TOKEN_TYPE_RSRVD_AWAIT
		|| token == TOKEN_TYPE_IDENT;
}

// 跳过当前错误语句，恢复到下一个安全同步点
void CParser::SynchronizeStatement()
{
	if( mLexer->isLastLookToken() )
		mLexer->GetNextToken();
	bool consumed = false;
	while (true)
	{
		TOKEN token = mLexer->LookNextToken();
		if (token == TOKEN_TYPE_END_OF_STREAM)
			return;
		if (token == TOKEN_TYPE_DELIM_SEMICOLON)
		{
			mLexer->GetNextToken();
			return;
		}
		if (token == TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE)
			return;
		if (consumed && IsStatementStartToken(token))
			return;
		mLexer->GetNextToken();
		consumed = true;
	}
}

// 带错误恢复地解析一条语句
bool CParser::ParseStateMentWithRecovery()
{
	if (!mRecoverMode)
		return ParseStateMent();
	if (setjmp(setjmp_buffer) == 0)
	{
		return ParseStateMent();
	}
	RecordLastParseError();
	SynchronizeStatement();
	mLexer = mOriginLexer;
	return false;
}

// 解析单条语句：根据下一个Token类型分发到对应的解析函数
bool  CParser::ParseStateMent()
{
	TOKEN token = mLexer->LookNextToken();
	switch(token)
	{
	case TOKEN_TYPE_DELIM_SEMICOLON:      //; 锟斤拷锟斤拷锟?
		{
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_SEMICOLON, -1, "expected ';' for empty statement");
			break;
		}
	case TOKEN_TYPE_END_OF_STREAM:
		{
			Error("unexpected end of file while parsing statement");
			break;
		}
	case TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE:
		{
			ParseBlock();
			break;
		}
	case TOKEN_TYPE_RSRVD_FUNC:
		{
			Factor factor;
			ParseFunction(factor);
			break;
		}
	case TOKEN_TYPE_RSRVD_ASYNC:
		{
			mLexer->ExpectToken(TOKEN_TYPE_RSRVD_ASYNC, -1, "expected 'async' before async function declaration");
			Factor factor;
			ParseFunction(factor, false, EFunctionType::Async);
			break;
		}
	case TOKEN_TYPE_RSRVD_GENERATOR:
		{
			mLexer->ExpectToken(TOKEN_TYPE_RSRVD_GENERATOR, -1, "expected 'generator' before generator function declaration");
			Factor factor;
			ParseFunction(factor, false, EFunctionType::Generator);
			break;
		}
	case TOKEN_TYPE_RSRVD_VAR:
		{
			ParseVar( );
			break;
		}
	case TOKEN_TYPE_RSRVD_IF:
		{
			ParseIf();
			break;
		}
	case TOKEN_TYPE_RSRVD_WHILE:
		ParseWhile ();
		break;
	case TOKEN_TYPE_RSRVD_FOR:
		ParseFor ();
		break;
	case TOKEN_TYPE_RSRVD_FOREACH:
		ParseForeach();
		break;
	case TOKEN_TYPE_RSRVD_ITERATOR:
		ParseIter();
		break;
	case TOKEN_TYPE_RSRVD_BREAK:
		ParseBreak ();
		break;
	case TOKEN_TYPE_RSRVD_CONTINUE:
		ParseContinue ();
		break;
	case TOKEN_TYPE_RSRVD_SWITCH:
		ParseSwitchCase();
		break;
	case TOKEN_TYPE_RSRVD_MATCH:
		ParseMatch();
		break;
	case TOKEN_TYPE_RSRVD_RETURN:
		ParseReturnOrYield(false);
		break;
	case TOKEN_TYPE_RSRVD_YIELD:
		ParseReturnOrYield(true);
		break;
	case TOKEN_TYPE_RSRVD_AWAIT:
		{
			Factor ret = ParseExpr(false);
			FreeFactorReg(ret);
			mLexer->TestToken(TOKEN_TYPE_DELIM_SEMICOLON);
			break;
		}
	case TOKEN_TYPE_RSRVD_TRY:
		ParseTryCatch();
		break;
	case TOKEN_TYPE_RSRVD_THROW:
		ParseThrow();
		break;
	case TOKEN_TYPE_RSRVD_DEFER:
		ParseDefer();
		break;
	case TOKEN_TYPE_IDENT:
	case TOKEN_TYPE_DELIM_OPEN_PAREN:
	case TOKEN_TYPE_DELIM_OPEN_BRACE:
		{
			ParserIdent();
			break;
		}
	default:
		Error("unexpected token '%s', expected a statement (e.g. variable, function call, if, for, while, return, break, continue)", mLexer->GetCurLexeme());
		break;
	}
	return true;
}

// 解析标识符开头的语句：可能是赋值、函数调用或多重赋值
void CParser::ParserIdent(bool bEnd)
{
	ChainedAccess access;
	ParserVariableAndFunction(access);

	if (mLexer->LookNextToken() == TOKEN_TYPE_DELIM_COMMA)
	{
		std::vector<ChainedAccess> varVec;
		varVec.push_back(access);

		while (mLexer->LookNextToken() == TOKEN_TYPE_DELIM_COMMA)
		{
			mLexer->GetNextToken();

			ChainedAccess nextAccess;
			ParserVariableAndFunction(nextAccess);
			varVec.push_back(nextAccess);
		}

		mLexer->ExpectToken(TOKEN_TYPE_OP, OP_TYPE_ASSIGN, "multiple assignment requires '=' between left-hand variables and right-hand values");

		std::vector<Factor> retVec;
		PendingFunctionReturnFactorScope pendingScope(this, retVec);
		ParseRetList(retVec);
		MultiAssignment(retVec, varVec);

		//mLexer->ExpectToken(TOKEN_TYPE_DELIM_SEMICOLON);
	}
	else if (mLexer->LookNextToken() == TOKEN_TYPE_OP)
	{
		ParserAssignRight(access, bEnd);
	}
	else if (bEnd && mLexer->LookNextToken() == TOKEN_TYPE_DELIM_SEMICOLON)
	{
		mLexer->GetNextToken();
		if (access.subject.type != FACTOR_FUNC)
		{
			Error("incomplete statement: expected an assignment operator or function call after identifier");
		}
	}
	else 
	{
		if (access.subject.type != FACTOR_FUNC)
		{
			Error("incomplete statement: expected an assignment operator or function call after identifier");
		}
	}
}

// 解析返回值/表达式列表（逗号分隔的多个表达式）
void CParser::ParseRetList(std::vector<Factor> &retVec)
{
	while (true)
	{
		Factor ret = ParseExpr(false);
		retVec.push_back(ret);
		if (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_COMMA)
			break;

		mLexer->GetNextToken();
	}
}

// 多重赋值：先快照右侧表达式，再写入左侧变量，保证 a, b = b, a 语义正确
void CParser::MultiAssignment(std::vector<Factor> &retVec, std::vector<ChainedAccess> &varVec)
{
	std::vector<Factor> assignVec;
	assignVec.reserve(varVec.size());

	bool expandLastFunc = retVec.size() > 0 && retVec.back().type == FACTOR_FUNC && retVec.size() <= varVec.size();
	int funcStartIndex = expandLastFunc ? (int)retVec.size() - 1 : -1;

	for (int i = 0; i < (int)varVec.size(); i++)
	{
		Factor assignFactor;
		assignFactor.type = FACTOR_INVALID;

		if (expandLastFunc && i >= funcStartIndex)
		{
			int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
			int freeReg = GET_FREE_REG();
			mMidCode->AddVarOperand(iInstrIndex, freeReg);
			mMidCode->AddRegOperand(iInstrIndex, i - funcStartIndex);

			assignFactor.type = FACTOR_VAR;
			assignFactor.varIndex = freeReg;
		}
		else if (i < (int)retVec.size())
		{
			Factor& ret = retVec[i];
			if (ret.type == FACTOR_INT || ret.type == FACTOR_FLOAT || ret.type == FACTOR_STRING || ret.type == FACTOR_NIL)
			{
				assignFactor = ret;
			}
			else
			{
				int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
				int freeReg = GET_FREE_REG();
				mMidCode->AddVarOperand(iInstrIndex, freeReg);
				AddOperandByFactor(iInstrIndex, ret);
				FreeFactorReg(ret);
				assignFactor.type = FACTOR_VAR;
				assignFactor.varIndex = freeReg;
			}
		}
		else
		{
			assignFactor.type = FACTOR_NIL;
		}

		assignVec.push_back(assignFactor);
	}

	for (int i = 0; i < (int)retVec.size(); i++)
		FreeFactorReg(retVec[i]);

	for (int i = 0; i < (int)varVec.size(); i++)
		AddAssignCode(varVec[i], assignVec[i]);

	for (int i = 0; i < (int)assignVec.size(); i++)
		FreeFactorReg(assignVec[i]);
}

// 根据链式访问结构向指令添加操作数
void  CParser::AddOperandByAccess(ChainedAccess& access, int iInstrIndex)
{
	if (access.HasAccessor())
		AddTableFactor(access.accessor, iInstrIndex, access.subject.varIndex);
	else
		AddOperandByFactor(iInstrIndex, access.subject);
}

// 解析函数调用参数列表，返回参数总数
int   CParser::ParseCallArgs(int iParamStart)
{
	int iParamNum = iParamStart;
	if (mCurPipelineFactorStack.size() > 0)
	{
		PipeLineData& pipelineData = mCurPipelineFactorStack.back();
		//自动填充参数
		if (pipelineData.autoFill && !pipelineData.hasFilled)
		{
			int instrIndex = mMidCode->AddInstr(INSTR_PUSH);
			AddOperandByFactor(instrIndex, pipelineData.parentFactor);
			pipelineData.hasFilled = true;
			iParamNum++;
		}
	}
	
	int token = mLexer->LookNextToken();
	while (token != TOKEN_TYPE_DELIM_CLOSE_PAREN)
	{
		iParamNum++;
		ParseExpr();
		token = mLexer->LookNextToken();
		if (token == TOKEN_TYPE_DELIM_CLOSE_PAREN)
			break;
		mLexer->ExpectToken(TOKEN_TYPE_DELIM_COMMA, -1, "expected ',' between function call arguments or ')' to close the argument list");
	}
	return iParamNum;
}

// 生成赋值指令：将表达式值赋给变量
void  CParser::AddAssignCode(ChainedAccess& var, Factor& expr)
{
	int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
	AddOperandByAccess(var, iInstrIndex);

	AddOperandByFactor(iInstrIndex, expr);
}

// 解析代码块：花括号包围的语句序列 { ... }
bool  CParser::ParseBlock()
{
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE, -1, "expected '{' to start a statement block");
	mSymbolTable->EnterBlock(mCurFuncIndex, (int)mMidCode->GetCurCodeIndex());
	while (mLexer->LookNextToken() 
		!= TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE)
	{
	if (mLexer->LookNextToken() == TOKEN_TYPE_END_OF_STREAM)
			Error("unexpected end of file, missing '}' to close block");
		ParseStateMentWithRecovery();
	}
	mSymbolTable->LeaveBlock(mCurFuncIndex, (int)mMidCode->GetCurCodeIndex());
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE, -1, "expected '}' to close the current statement block");
	return true;
}
// 报告语法错误并终止解析
void  CParser::Error(const char *errInfo, ...)
{
	char  buffer[512] = { 0 };
	va_list  args;
	va_start(args, errInfo);
	int len = vsnprintf(buffer, 512, errInfo, args);
	va_end(args);
	buffer[len] = '\0';
	if(!mLexer->isLastLookToken())
		mLexer->RewindToken();

	ExitOnError(buffer, mLexer->GetLine() + 1, mLexer->GetChar());
}

// 报告标识符重定义错误
void  CParser::ErrorIdentRedefine(const char* ident)
{
	char errInfo[MAX_ERRORINFO_SIZE] = {0};
	snprintf(errInfo, MAX_ERRORINFO_SIZE, "identifier '%s' is already defined in this scope", ident);
	Error(errInfo);
}

// 检查用户声明/引用的标识符不能使用编译器内部前缀
void CParser::ValidateUserIdentName(const char* ident)
{
	if (strncmp(ident, XSCRIPT_INTERNAL_IDENT_PREFIX, strlen(XSCRIPT_INTERNAL_IDENT_PREFIX)) == 0)
	{
		Error("identifier '%s' is reserved for compiler internal use, please choose a different name", ident);
	}
}

// 解析函数定义的参数列表：检查标识符、处理可变参数、逗号分隔
int CParser::ParseParamList(int funcIndex, int endToken)
{
	int iParamNum = 0;
	bool hasDefaultArgs = false;
	bool hasVarArgs = false;
	int nextJumpIndex = -1;
	while (mLexer->LookNextToken() != endToken)
	{
		TOKEN token = mLexer->GetNextToken();
		if (token == TOKEN_TYPE_IDENT)
		{
			if (hasVarArgs)
			{
				Error("Already have varArgs, should not have more args");
			}

			ValidateUserIdentName(mLexer->GetCurLexeme());
			if (mSymbolTable->GetVarByName(mLexer->GetCurLexeme(), funcIndex, false) != NULL)
			{
				ErrorIdentRedefine(mLexer->GetCurLexeme());
			}
			int varIndex = mSymbolTable->AddVariant(mLexer->GetCurLexeme(), funcIndex, 0, IDENT_TYPE_PARAM, 0);
			if (mLexer->LookNextToken() == TOKEN_TYPE_OP && mLexer->GetCurOprType() == OpType::OP_TYPE_ASSIGN)
			{
				mLexer->GetNextToken();

				if (nextJumpIndex >= 0)
				{
					mMidCode->AddJumpTarget(nextJumpIndex);
				}

				hasDefaultArgs = true;
				int jumpIndex =  mMidCode->GetNextJumpIndex();
				int instrIndex = mMidCode->AddInstr(INSTR_J_ARGS_G);
				mMidCode->AddIntOperand(instrIndex, iParamNum);
				mMidCode->AddJumpIndexOperand(instrIndex, jumpIndex);
				nextJumpIndex = jumpIndex;

				Factor factor = ParseExpr(false);

				instrIndex = mMidCode->AddInstr(INSTR_MOV);
				mMidCode->AddVarOperand(instrIndex, varIndex);
				AddOperandByFactor(instrIndex, factor);
				FreeFactorReg(factor);
			}
			else if(hasDefaultArgs)
			{
				Error("have default args before, should not appear normal args");
			}
		}
		else if (token == TOKEN_TYPE_DELIM_THREE_POINT)
		{
			mSymbolTable->AddVariant(ARGS, funcIndex, 0, IDENT_TYPE_PARAM, 0);
			mSymbolTable->SetHasVarArgs(funcIndex);
			hasVarArgs = true;
		}

		iParamNum++;

		//if (token == TOKEN_TYPE_DELIM_THREE_POINT)
		//	break;

		if (mLexer->LookNextToken() == endToken)
			break;

		mLexer->ExpectToken(TOKEN_TYPE_DELIM_COMMA, -1, "expected ',' between function parameters");
	}
	
	if (nextJumpIndex >= 0)
	{
		mMidCode->AddJumpTarget(nextJumpIndex);
	}

	return iParamNum;
}

// 生成函数闭包尾部代码：恢复函数上下文 + 生成 INSTR_FUNC 指令
void CParser::EmitFunctionClosure(int lastFuncIndex, Factor assignFactor, Factor tableFactor, int symbIndex, int funcDefLine)
{
	mCurFuncIndex = lastFuncIndex;
	mMidCode->setCurFuncIndex(mCurFuncIndex);
	int iInstrIndex = mMidCode->AddInstr(INSTR_FUNC);

	// 修正INSTR_FUNC指令的行号为函数定义开始位置，避免调试时跳到函数体结束位置
	if (funcDefLine >= 0 && iInstrIndex >= 0)
		mMidCode->mCodeList[mCurFuncIndex][iInstrIndex].lineIndex = funcDefLine;

	int funcIndex = (int)mSymbolTable->GetFunctionByIndex(mCurFuncIndex)->subIndexVec.size() - 1;

	if (assignFactor.type == FACTOR_INVALID && tableFactor.type != FACTOR_INVALID)
	{
		// 表赋值模式：如 obj.method = func
		AddTableFactor(tableFactor, iInstrIndex, symbIndex);
	}
	else if (assignFactor.type == FACTOR_INVALID && symbIndex >= 0)
	{
		// 直接变量赋值模式：如 funcName = func
		mMidCode->AddVarOperand(iInstrIndex, symbIndex);
	}
	else
	{
		AddOperandByFactor(iInstrIndex, assignFactor);
	}

	mMidCode->AddIntOperand(iInstrIndex, funcIndex);
	FREE_REG(symbIndex);
	FreeFactorReg(tableFactor);
}

// 解析Lambda表达式：匿名函数的简写形式 (params: expr)
void	CParser::ParseLambda(Factor assignFactor)
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_LAMBDA, -1, "expected 'lambda' to start lambda expression");
	int funcDefLine = mLexer->GetLine() + 1; // 记录lambda关键字所在行号
	std::string funcName = GetNextLamdaFuncName();
	
	int iFuncIndex = mSymbolTable->AddFunction(funcName.c_str(), 0, mCurFuncIndex);
	mSymbolTable->AddSubFunction(mCurFuncIndex, iFuncIndex);

	int iParamNum = ParseParamList(iFuncIndex, TOKEN_TYPE_DELIM_COLON);


	mLexer->ExpectToken(TOKEN_TYPE_DELIM_COLON, -1, "lambda parameter list must be followed by ':' before the return expression");

	mSymbolTable->SetFunctionParamNum(iFuncIndex, iParamNum);

	int lastFuncIndex = mCurFuncIndex;
	RegAllocContext lastRegContext = SaveRegAllocContext();
	mCurFuncIndex = iFuncIndex;
	mMidCode->setCurFuncIndex(iFuncIndex);
	InitRegAllocContextForFunction(iFuncIndex);
	
	Factor ret = ParseExpr(false);
	int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
	mMidCode->AddRegOperand(iInstrIndex, 0);
	AddOperandByFactor(iInstrIndex, ret);
	FreeFactorReg(ret);
	iInstrIndex = mMidCode->AddInstr(INSTR_RET);
	mMidCode->AddIntOperand(iInstrIndex, 1);

	RestoreRegAllocContext(lastRegContext);
	EmitFunctionClosure(lastFuncIndex, assignFactor, Factor(), -1, funcDefLine);

}

// 解析函数定义：支持命名函数、匿名函数、方法定义（含self参数）
bool  CParser::ParseFunction(Factor assignFactor, bool isLocal, EFunctionType funcType)
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_FUNC, -1, "expected 'function' to start function definition");
	int funcDefLine = mLexer->GetLine() + 1; // 记录function关键字所在行号
	std::string funcName;
	Factor tableFactor;

	bool hasSelf = false;
	int symbIndex = -1;
	if (assignFactor.type == FACTOR_INVALID)
	{
		if (isLocal)
		{
			mLexer->ExpectToken(TOKEN_TYPE_IDENT, -1, "local function declaration requires a function name after 'var function'");
			ValidateUserIdentName(mLexer->GetCurLexeme());
			funcName = mLexer->GetCurLexeme();

			int index = mSymbolTable->AddVariant(funcName.c_str(), mCurFuncIndex, 1, IDENT_TYPE_VAR, (int)mMidCode->GetCurCodeIndex());
			assignFactor.type = FACTOR_VAR;
			assignFactor.varIndex = index;
		}
		else
		{
			// 使用ParserVariableAndFunction统一解析变量/表访问链，accessOnly=true只解析.和[]访问，不处理()和:
			ChainedAccess access;
			ParserVariableAndFunction(access, true);

			// 从access中提取symbIndex和tableFactor
			// access.subject = 被访问的主体变量
			// access.accessor = 最后一层索引键（如果有表访问的话）
			EnsureFactorIsVar(access.subject);
			symbIndex = access.subject.varIndex;
			if (access.HasAccessor())
			{
				tableFactor = access.accessor;
			}

			if (mLexer->LookNextToken() == TOKEN_TYPE_DELIM_COLON)
			{
				mLexer->GetNextToken();
				mLexer->ExpectToken(TOKEN_TYPE_IDENT, -1, "method definition requires a method name after ':'");
				ValidateUserIdentName(mLexer->GetCurLexeme());

				// 如果已有表访问（如 a.b:c），需要先折叠 a.b 为临时变量
				if (access.HasAccessor())
				{
					FoldTableIndex(access);
					symbIndex = access.subject.varIndex;
				}

				std::string name = mLexer->GetCurLexeme();
				int strIndex = mSymbolTable->AddString(name.c_str());

				tableFactor.type = FACTOR_STRING;
				tableFactor.strIndex = strIndex;

				hasSelf = true;
				funcName = name;
			}
			else
			{
				// 无冒号：检查是否为简单变量名（无表访问），用作函数名
				if (!access.HasAccessor() && !IsRegVar(symbIndex))
				{
					const char* varName = FindVarNameBySymbolIndex(*mSymbolTable, symbIndex, mCurFuncIndex);
					if (varName)
						funcName = varName;
				}
			}
		}
	}

	if (funcName.empty())
	{
		funcName = GetNextAnonymousFuncName();
	}

	int iFuncIndex = mSymbolTable->AddFunction(funcName.c_str(), 0, mCurFuncIndex, funcType);
	mSymbolTable->AddSubFunction(mCurFuncIndex, iFuncIndex);
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN, -1, "function name must be followed by '(' to start parameter list");


	int lastFuncIndex = mCurFuncIndex;
	int lastTryCatchBlockIndex = mCurTryCatchBlockIndex;
	mCurTryCatchBlockIndex = -1;
	RegAllocContext lastRegContext = SaveRegAllocContext();
	mCurFuncIndex = iFuncIndex;
	mMidCode->setCurFuncIndex(iFuncIndex);
	InitRegAllocContextForFunction(iFuncIndex);

	int iParamNum = 0;

	if (hasSelf)
	{
		iParamNum++;
		mSymbolTable->AddVariant("self", iFuncIndex, 0, IDENT_TYPE_PARAM, 0);
	}

	iParamNum += ParseParamList(iFuncIndex, TOKEN_TYPE_DELIM_CLOSE_PAREN);

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN, -1, "function parameter list must be closed with ')'");

	mSymbolTable->SetFunctionParamNum(iFuncIndex, iParamNum);



	ParseBlock();
 	if (mMidCode->getLastInstrOp() != INSTR_RET)
	{ 
 		int iInstrIndex =  mMidCode->AddInstr(INSTR_RET);
		mMidCode->AddIntOperand(iInstrIndex, 0);

	}

	RestoreRegAllocContext(lastRegContext);
	EmitFunctionClosure(lastFuncIndex, assignFactor, tableFactor, symbIndex, funcDefLine);

	mCurTryCatchBlockIndex = lastTryCatchBlockIndex;
	return true;
}


// 解析变量声明语句：var x, y = expr1, expr2
bool  CParser::ParseVar( )
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_VAR, -1, "expected 'var' to start variable declaration");
	int nextToken = mLexer->LookNextToken();
	if (nextToken == TOKEN_TYPE_RSRVD_FUNC)
	{
		ParseFunction(Factor(), true);
	}
	else if (nextToken == TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE) //{
	{
		std::vector<TableDeconstructData> tableVec;
		std::vector<ListDeconstructData> ListVec;
		TableDeconstructData tableData;

		ParseTableDeconstruct(tableVec, ListVec, tableData, true);

		int nextToken = mLexer->LookNextToken();
		if (nextToken != TOKEN_TYPE_OP || mLexer->GetCurOprType() != OP_TYPE_ASSIGN)
		{
			Error("deconstruct error, should be followed by =");
		}
		mLexer->GetNextToken();
		Factor factor = ParseExpr(false, true);
		EnsureFactorIsVar(factor);
		TableDeconstructAssign(tableVec, ListVec, tableData, factor.varIndex);
		FreeFactorReg(factor);
		//mLexer->clearAllTokenBuffers();
	}
	else if (nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE) //[
	{
		std::vector<TableDeconstructData> tableVec;
		std::vector<ListDeconstructData> ListVec;
		ListDeconstructData listData;

		ParseListDeconstruct(tableVec, ListVec, listData, true);

		int nextToken = mLexer->LookNextToken();
		if (nextToken != TOKEN_TYPE_OP || mLexer->GetCurOprType() != OP_TYPE_ASSIGN)
		{
			Error("deconstruct error, should be followed by =");
		}
		mLexer->GetNextToken();
		Factor factor = ParseExpr(false, true);
		EnsureFactorIsVar(factor);

		ListDeconstructAssign(tableVec, ListVec, listData, factor.varIndex);
		FreeFactorReg(factor);
		//mLexer->clearAllTokenBuffers();
	}
	else
	{
		std::vector<ChainedAccess> varVec;

		do
		{
			mLexer->ExpectToken(TOKEN_TYPE_IDENT, -1, "variable declaration requires an identifier after 'var'");
			ValidateUserIdentName(mLexer->GetCurLexeme());
			int iVarIndex = mSymbolTable->AddVariant(mLexer->GetCurLexeme(), mCurFuncIndex, 0, IDENT_TYPE_VAR, (int)mMidCode->GetCurCodeIndex());

			Factor factor;
			factor.type = FACTOR_VAR;
			factor.varIndex = iVarIndex;
			varVec.push_back(ChainedAccess(factor, Factor()));

			if (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_COMMA)
			{
				break;
			}
			mLexer->GetNextToken();
		} while (true);


		if (mLexer->LookNextToken() == TOKEN_TYPE_OP && mLexer->GetCurOprType() == OP_TYPE_ASSIGN)
		{
			mLexer->GetNextToken();

			std::vector<Factor> retVec;
			PendingFunctionReturnFactorScope pendingScope(this, retVec);
			ParseRetList(retVec);
			MultiAssignment(retVec, varVec);

		}
		mLexer->TestToken(TOKEN_TYPE_DELIM_SEMICOLON);
	}

	return true;
}

// 根据因子类型向指令添加对应的操作数
void  CParser::AddOperandByFactor(int iInstrIndex, Factor ret)
{
	switch(ret.type)
	{
	case FACTOR_INT:
		{
			mMidCode->AddIntOperand(iInstrIndex, ret.intValue);
			break;
		}
	case FACTOR_NIL:
		{
			mMidCode->AddNilOperand(iInstrIndex);
			break;
		}
	case FACTOR_FLOAT:
		{
			mMidCode->AddFloatOperand(iInstrIndex, ret.floatValue);
			break;
		}
	case FACTOR_VAR:
		{
			mMidCode->AddVarOperand(iInstrIndex, ret.varIndex);
			break;
		}
	case FACTOR_TABLE:
		{
			switch (ret.iOffset)
			{
			case ROT_Int:
				mMidCode->AddTableIntOperand(iInstrIndex, ret.varIndex, ret.intTableValue);
				break;
			case ROT_String:
				mMidCode->AddTableStringOperand(iInstrIndex, ret.varIndex, (int)ret.intTableValue);
				break;
			case ROT_Float:
				mMidCode->AddTableFloatOperand(iInstrIndex, ret.varIndex, ret.fTableValue);
				break;
			case ROT_Stack_Index:
				mMidCode->AddTableIndexOperand(iInstrIndex, ret.varIndex, (int)ret.intTableValue);
				break;
			case ROT_Reg:
				mMidCode->AddTableRegOperand(iInstrIndex, ret.varIndex, 0);
				break;
			}
			break;
		}
	case FACTOR_FUNC:
		{
			mMidCode->AddRegOperand(iInstrIndex);
			break;
		}
	case FACTOR_STRING:
		{
			mMidCode->AddStringIndexOperand(iInstrIndex, ret.strIndex);
			break;
		}
	default:;
	}
}

// 将运算符类型映射为对应的算术指令（原地修改版本）
// 运算符类型 → 算术指令码 的静态映射表
static const struct { int opType; int instrCode; } s_opInstrMap[] = {
	{ OP_TYPE_ADD,                 INSTR_ADD    },
	{ OP_TYPE_SUB,                 INSTR_SUB    },
	{ OP_TYPE_CONCAT,              INSTR_CONCAT },
	{ OP_TYPE_BITWISE_AND,         INSTR_AND    },
	{ OP_TYPE_BITWISE_OR,          INSTR_OR     },
	{ OP_TYPE_BITWISE_XOR,         INSTR_XOR    },
	{ OP_TYPE_MUL,                 INSTR_MUL    },
	{ OP_TYPE_DIV,                 INSTR_DIV    },
	{ OP_TYPE_MOD,                 INSTR_MOD    },
	{ OP_TYPE_EXP,                 INSTR_EXP    },
	{ OP_TYPE_BITWISE_SHIFT_LEFT,  INSTR_SHL    },
	{ OP_TYPE_BITWISE_SHIFT_RIGHT, INSTR_SHR    },
};

int	GetOpInstr(int op)
{
	for (const auto& entry : s_opInstrMap)
	{
		if (entry.opType == op)
			return entry.instrCode;
	}
	return -1;
}

// 将运算符类型映射为对应的赋值运算指令（结果存入新变量）
// 运算符类型 → 赋值运算指令码 的静态映射表（含比较运算符）
static const struct { int opType; int instrCode; } s_opInstrToMap[] = {
	{ OP_TYPE_ADD,                 INSTR_ADD_TO    },
	{ OP_TYPE_SUB,                 INSTR_SUB_TO    },
	{ OP_TYPE_CONCAT,              INSTR_CONCAT_TO },
	{ OP_TYPE_BITWISE_AND,         INSTR_AND_TO    },
	{ OP_TYPE_BITWISE_OR,          INSTR_OR_TO     },
	{ OP_TYPE_BITWISE_XOR,         INSTR_XOR_TO    },
	{ OP_TYPE_MUL,                 INSTR_MUL_TO    },
	{ OP_TYPE_DIV,                 INSTR_DIV_TO    },
	{ OP_TYPE_MOD,                 INSTR_MOD_TO    },
	{ OP_TYPE_EXP,                 INSTR_EXP_TO    },
	{ OP_TYPE_BITWISE_SHIFT_LEFT,  INSTR_SHL_TO    },
	{ OP_TYPE_BITWISE_SHIFT_RIGHT, INSTR_SHR_TO    },
	{ OP_TYPE_EQUAL,               INSTR_TEST_E    },
	{ OP_TYPE_NOT_EQUAL,           INSTR_TEST_NE   },
	{ OP_TYPE_LESS,                INSTR_TEST_L    },
	{ OP_TYPE_GREATER,             INSTR_TEST_G    },
	{ OP_TYPE_LESS_EQUAL,          INSTR_TEST_LE   },
	{ OP_TYPE_GREATER_EQUAL,       INSTR_TEST_GE   },
};

int	GetOpInstrTo(int op)
{
	for (const auto& entry : s_opInstrToMap)
	{
		if (entry.opType == op)
			return entry.instrCode;
	}
	return -1;
}


// 解析单个因子：数字、字符串、变量、函数调用、表/列表初始化等
bool  CParser::ParseFactor(Factor& factor, bool onlyFactor)
{
	bool bNegative = false;
	bool isNot = false;

	int token = mLexer->GetNextToken();
	if (token == TOKEN_TYPE_OP)
	{
		if ((mLexer->GetCurOprType() == OP_TYPE_ADD || mLexer->GetCurOprType() == OP_TYPE_SUB))
		{
			if (mLexer->GetCurOprType() == OP_TYPE_SUB)
				bNegative = true;

			token = mLexer->GetNextToken();
		}
		else if (mLexer->GetCurOprType() == OP_TYPE_LOGICAL_NOT)
		{
			isNot = true;
			token = mLexer->GetNextToken();
		}
	}

	factor.type = FACTOR_INVALID;
	switch(token)
	{
	case TOKEN_TYPE_RSRVD_NIL:
		{
			factor.type = FACTOR_NIL;
		}
		break;
	case TOKEN_TYPE_INT:
		{
			factor.type = FACTOR_INT;
			factor.intValue = mLexer->GetCurIntValue();
			break;
		}
	case TOKEN_TYPE_FLOAT:
		{
			factor.type = FACTOR_FLOAT;
			factor.floatValue = StrToXFloat(mLexer->GetCurLexeme());
			break;
		}
	case TOKEN_TYPE_RSRVD_TRUE:
		{
			factor.type  = FACTOR_INT;
			factor.intValue = 1;
			break;
		}
	case TOKEN_TYPE_RSRVD_FALSE:
		{
			factor.type = FACTOR_INT;
			factor.intValue = 0;
			break;
		}
	case TOKEN_TYPE_DELM_UNDERSCORE:
		{
			if (mCurPipelineFactorStack.size() > 0 && mCurPipelineFactorStack.back().autoFill == false)
			{
				int freeReg = GET_FREE_REG();
				int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
				mMidCode->AddVarOperand(iInstrIndex, freeReg);
				AddOperandByFactor(iInstrIndex, mCurPipelineFactorStack.back().parentFactor);

				factor.type = FACTOR_VAR;
				factor.varIndex = freeReg;
			}
			else
			{
				if (mCurPipelineFactorStack.size() > 0 && mCurPipelineFactorStack.back().autoFill)
				{
					Error("current pipeline is auto fill mode, can't fill manually, if you want use '_', please use '|>>'");
				}
				else
				{
					Error("have no pipeline, you can't use '_'.");
				}
			}
			break;
		}
	case TOKEN_TYPE_STRING:
		{
			factor.type = FACTOR_STRING;
			factor.strIndex = mSymbolTable->AddString(mLexer->GetCurLexeme());
			break;
		}
	case TOKEN_TYPE_FSTRING:
		{
			ParseFstring(factor);
		}
		break;
	case TOKEN_TYPE_RSRVD_AWAIT:
		{
			FunctionST* func = mSymbolTable->GetFunctionByIndex(mCurFuncIndex);
			if (func == NULL || func->funcType != EFunctionType::Async )
			{
				Error("await can only be used in async function");
			}

			Factor awaited;
			ParseFactor(awaited);

			int awaitInstr = mMidCode->AddInstr(INSTR_AWAIT);
			AddOperandByFactor(awaitInstr, awaited);
			FreeFactorReg(awaited);

			int freeReg = GET_FREE_REG();
			int movInstr = mMidCode->AddInstr(INSTR_MOV);
			mMidCode->AddVarOperand(movInstr, freeReg);
			mMidCode->AddRegOperand(movInstr, 0);

			factor.type = FACTOR_VAR;
			factor.varIndex = freeReg;
			break;
		}
	case TOKEN_TYPE_IDENT:
		{
			mLexer->RewindToken();
			ChainedAccess access;
			ParserVariableAndFunction(access);
			ConvertAccessorToFactor(access, factor);

			break;
		}
	case TOKEN_TYPE_DELIM_OPEN_PAREN:
		{
			if (onlyFactor)
				ParseFactor(factor, true);
			else
				factor = ParseExpr(false, true);
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN, -1, "parenthesized expression must be closed with ')'");

			break;
		}
	case TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE:
		{
			mLexer->RewindToken();
			int tableReg = ParseTableInit();
			factor.type = FACTOR_VAR;
			factor.varIndex = tableReg;
		}
		break;
	case TOKEN_TYPE_DELIM_OPEN_BRACE:
		{
			mLexer->RewindToken();
			int tableReg = ParseListInit();
			factor.type = FACTOR_VAR;
			factor.varIndex = tableReg;
		}
		break;
	case TOKEN_TYPE_RSRVD_FUNC:
		{
			mLexer->RewindToken();
			int freeReg = GET_FREE_REG();
			factor.type = FACTOR_VAR;
			factor.varIndex = freeReg;
			ParseFunction(factor);
		}
		break;
	case TOKEN_TYPE_RSRVD_ASYNC:
		{
			int freeReg = GET_FREE_REG();
			factor.type = FACTOR_VAR;
			factor.varIndex = freeReg;
			ParseFunction(factor, false, EFunctionType::Async);
		}
		break;
	case TOKEN_TYPE_RSRVD_GENERATOR:
		{
			int freeReg = GET_FREE_REG();
			factor.type = FACTOR_VAR;
			factor.varIndex = freeReg;
			ParseFunction(factor, false, EFunctionType::Generator);
		}
		break;
	case TOKEN_TYPE_RSRVD_LAMBDA:
		{
			mLexer->RewindToken();
			int freeReg = GET_FREE_REG();
			factor.type = FACTOR_VAR;
			factor.varIndex = freeReg;
			ParseLambda(factor);
		}
	break;
	break;
	default:
		Error("unexpected token '%s', expected an expression (e.g. number, string, identifier, '(')", mLexer->GetCurLexeme());
		break;
	}

	TOKEN nextToken = mLexer->LookNextToken();
	if (nextToken == TOKEN_TYPE_DELIM_OPEN_PAREN
		|| nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE
		|| nextToken == TOKEN_TYPE_DELIM_POINT
		|| nextToken == TOKEN_TYPE_DELIM_COLON
		|| nextToken == TOKEN_TYPE_DELIM_OPTIONAL_DOT
		|| nextToken == TOKEN_TYPE_DELIM_OPTIONAL_CALL
		|| nextToken == TOKEN_TYPE_DELIM_OPTIONAL_INDEX
		|| nextToken == TOKEN_TYPE_DELIM_OPTIONAL_COLON)
	{
		bool isOptionalChain = (nextToken == TOKEN_TYPE_DELIM_OPTIONAL_DOT
			|| nextToken == TOKEN_TYPE_DELIM_OPTIONAL_CALL
			|| nextToken == TOKEN_TYPE_DELIM_OPTIONAL_INDEX
			|| nextToken == TOKEN_TYPE_DELIM_OPTIONAL_COLON);

		if (factor.type == FACTOR_INT || factor.type == FACTOR_FLOAT || factor.type == FACTOR_STRING)
		{
			Error("literal value (number, string) cannot be followed by '.', ':', '[' or '(' operator");
		}
		else if (factor.type == FACTOR_NIL && !isOptionalChain)
		{
			Error("nil cannot be followed by '.', ':', '[' or '(' operator (use ?. for optional chaining)");
		}

		ChainedAccess access(factor, Factor());
		ParsePostfix(access);
		ConvertAccessorToFactor(access, factor);
	}

	if (bNegative)
	{
		if (factor.type == FACTOR_NIL)
		{

		}
		if (factor.type == FACTOR_INT)
		{
			factor.intValue = -factor.intValue;
		}
		else if (factor.type == FACTOR_FLOAT)
		{
			factor.floatValue = -factor.floatValue;
		}
		else if (factor.type == FACTOR_STRING)
		{
			Error("cannot apply unary '-' operator to a string value");
		}
		else
		{
			int freeReg = GET_FREE_REG();
			int instr = mMidCode->AddInstr(INSTR_NEG);
			mMidCode->AddVarOperand(instr, freeReg);
			AddOperandByFactor(instr, factor);
			factor.type = FACTOR_VAR;
			factor.varIndex = freeReg;
		}
	}
	else if (isNot)
	{
		if (factor.type == FACTOR_INT)
		{
			factor.intValue = factor.intValue == 0;
		}
		else if (factor.type == FACTOR_NIL)
		{
			factor.intValue = 1;
			factor.type = FACTOR_INT;
		}
		else if (factor.type == FACTOR_FLOAT)
		{
			factor.intValue = factor.floatValue == 0.0f;
			factor.type = FACTOR_INT;
		}
		else if (factor.type == FACTOR_STRING)
		{
			Error("cannot apply '!' operator to a string value");
		}
		else
		{
			int freeReg = GET_FREE_REG();
			int iInstrIndex = mMidCode->AddInstr(INSTR_LOGIC_NOT);
			mMidCode->AddVarOperand(iInstrIndex, freeReg);
			AddOperandByFactor(iInstrIndex, factor);
			factor.type = FACTOR_VAR;
			factor.varIndex = freeReg;
		}


	}

	return true;
}

// 将链式访问结构转换为因子：处理表索引的各种类型
void CParser::ConvertAccessorToFactor(ChainedAccess& access, Factor& factor)
{
	if (access.HasAccessor())
	{
		factor.type = FACTOR_TABLE;
		factor.varIndex = access.subject.varIndex;

		if (access.accessor.type == FACTOR_INT)
		{
			factor.iOffset = ROT_Int;
			factor.intTableValue = access.accessor.intValue;
		}
		else if (access.accessor.type == FACTOR_STRING)
		{
			factor.iOffset = ROT_String;
			factor.intTableValue = access.accessor.strIndex;
		}
		else if (access.accessor.type == FACTOR_FLOAT)
		{
			factor.iOffset = ROT_Float;
			factor.floatValue = access.accessor.floatValue;
		}
		else if (access.accessor.type == FACTOR_VAR)
		{
			factor.iOffset = ROT_Stack_Index;
			factor.intTableValue = access.accessor.varIndex;
		}
		else if (access.accessor.type == FACTOR_FUNC)
		{
			factor.iOffset = ROT_Reg;
			factor.intTableValue = access.accessor.varIndex;
		}
		else
		{
			Error("invalid accessor type in table/member access expression");
		}
	}
	else
	{
		factor = access.subject;
	}
}

// 解析赋值语句右侧：处理=、+=、-=、*=、/=、++、--等运算符
void CParser::ParserAssignRight(ChainedAccess& access, bool bEnd)
{
	mLexer->ExpectToken(TOKEN_TYPE_OP, -1, "assignment statement requires an assignment operator such as '=', '+=', '-=', '*=', '/=', '++' or '--'");
	int oprType = mLexer->GetCurOprType();
	bool isTable = access.HasAccessor();
	Factor ret;

	if (access.subject.type == FACTOR_NIL)
	{
		//nil不能被赋值
		Error("'nil' cannot be used as an assignment target");
	}

	if (oprType != OP_TYPE_INC && oprType != OP_TYPE_DEC)
	{
		ret = ParseExpr(false);
	}

	{
		// 复合赋值运算符 → 指令码 的静态映射表
		static const struct { int oprType; int instrCode; } s_assignOpMap[] = {
			{ OP_TYPE_ASSIGN,             INSTR_MOV    },
			{ OP_TYPE_ASSIGN_ADD,         INSTR_ADD    },
			{ OP_TYPE_ASSIGN_SUB,         INSTR_SUB    },
			{ OP_TYPE_ASSIGN_MUL,         INSTR_MUL    },
			{ OP_TYPE_ASSIGN_DIV,         INSTR_DIV    },
			{ OP_TYPE_ASSIGN_MOD,         INSTR_MOD    },
			{ OP_TYPE_ASSIGN_EXP,         INSTR_EXP    },
			{ OP_TYPE_ASSIGN_CONCAT,      INSTR_CONCAT },
			{ OP_TYPE_ASSIGN_AND,         INSTR_AND    },
			{ OP_TYPE_ASSIGN_OR,          INSTR_OR     },
			{ OP_TYPE_ASSIGN_XOR,         INSTR_XOR    },
			{ OP_TYPE_ASSIGN_SHIFT_LEFT,  INSTR_SHL    },
			{ OP_TYPE_ASSIGN_SHIFT_RIGHT, INSTR_SHR    },
			{ OP_TYPE_INC,                INSTR_INC    },
			{ OP_TYPE_DEC,                INSTR_DEC    },
		};

		int assignInstrIndex = -1;
		bool found = false;
		for (const auto& entry : s_assignOpMap)
		{
			if (entry.oprType == oprType)
			{
				assignInstrIndex = mMidCode->AddInstr(entry.instrCode);
				found = true;
				break;
			}
		}
		if (!found)
			Error("unsupported assignment operator in this context");

		if (isTable)
		{
			AddTableFactor(access.accessor, assignInstrIndex, access.subject.varIndex);
		}
		else
			AddOperandByFactor(assignInstrIndex, access.subject);

		if (OP_TYPE_INC != oprType  && OP_TYPE_DEC != oprType)
		{
			AddOperandByFactor(assignInstrIndex, ret);
		}

		FreeFactorReg(ret);
	}

	if (bEnd)
		mLexer->TestToken(TOKEN_TYPE_DELIM_SEMICOLON);

	FreeFactorReg(access.subject);
	FreeFactorReg(access.accessor);
}

// 判断运算符是否为逻辑/比较运算符
bool IsLogicOp(int op)
{
	return OP_TYPE_LOGICAL_AND == op
		|| OP_TYPE_LOGICAL_OR == op
		|| OP_TYPE_EQUAL == op
		|| OP_TYPE_NOT_EQUAL == op
		|| OP_TYPE_LESS == op
		|| OP_TYPE_GREATER == op
		|| OP_TYPE_LESS_EQUAL == op
		|| OP_TYPE_GREATER_EQUAL == op;
}

// 解析表达式：处理逻辑短路、三元运算符、压栈
CParser::Factor  CParser::ParseExpr(bool push, bool noTable)
{
	Factor resultFactor = ParsePipeline();

	if (mLexer->LookNextToken() == TOKEN_TYPE_DELIM_INTERROGATION)
	{
		mLexer->ExpectToken(TOKEN_TYPE_DELIM_INTERROGATION, -1, "conditional expression requires '?' after the condition");

		int iJumpIndexFalse = mMidCode->GetNextJumpIndex();
		int iJumpIndexEnd = mMidCode->GetNextJumpIndex();

		int iInstrIndex = mMidCode->AddInstr(INSTR_JFALSE);
		AddOperandByFactor(iInstrIndex, resultFactor);
		mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndexFalse);
		FreeFactorReg(resultFactor);

		int freeReg = GET_FREE_REG();
		Factor exprFactor1 = ParseExpr(false);
		iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
		mMidCode->AddVarOperand(iInstrIndex, freeReg);
		AddOperandByFactor(iInstrIndex, exprFactor1);
		FreeFactorReg(exprFactor1);
		mLexer->ExpectToken(TOKEN_TYPE_RSRVD_OR, -1, "conditional expression must use 'or' between true and false branches");

		iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
		mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndexEnd);

		mMidCode->AddJumpTarget(iJumpIndexFalse);

		Factor exprFactor2 = ParseExpr(false);
		iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
		mMidCode->AddVarOperand(iInstrIndex, freeReg);
		AddOperandByFactor(iInstrIndex, exprFactor2);
		FreeFactorReg(exprFactor2);

		mMidCode->AddJumpTarget(iJumpIndexEnd);

		resultFactor.type = FACTOR_VAR;
		resultFactor.varIndex = freeReg;
	}

	if (push)
	{
		int iInstrIndex = mMidCode->AddInstr(INSTR_PUSH);
		AddOperandByFactor(iInstrIndex, resultFactor);
		FreeFactorReg(resultFactor);
	}
	else if (noTable && resultFactor.type == FACTOR_TABLE)
	{
		int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
		int freeReg = GET_FREE_REG();
		mMidCode->AddVarOperand(iInstrIndex, freeReg);
		AddOperandByFactor(iInstrIndex, resultFactor);
		FreeFactorReg(resultFactor);
		resultFactor.type = FACTOR_VAR;
		resultFactor.varIndex = freeReg;
	}

	return resultFactor;
}

// 获取短路操作符优先级，数值越大优先级越高
int GetShortCircuitPriority(int op)
{
	switch (op)
	{
	case OP_TYPE_LOGICAL_AND:
		return 30;
	case OP_TYPE_LOGICAL_OR:
		return 20;
	case OP_TYPE_NULL_COALESCING:
		return 10;
	default:
		return -1;
	}
}

// 解析短路表达式：&&、||、?? 都返回原操作数，并按操作符优先级保持短路求值
CParser::Factor CParser::ParseShortCircuitExpr(int minPriority)
{
	Factor resultFactor = ParseSubExpr();
	while (mLexer->LookNextToken() == TOKEN_TYPE_OP)
	{
		int op = mLexer->GetCurOprType();
		int priority = GetShortCircuitPriority(op);
		if (priority < minPriority)
			break;

		mLexer->ExpectToken(TOKEN_TYPE_OP, -1, "short-circuit expression requires an operator between operands");

		int freeReg = -1;
		int iInstrIndex = -1;
		if (resultFactor.type == FACTOR_VAR && IsRegVar(resultFactor.varIndex))
		{
			freeReg = resultFactor.varIndex;
		}
		else
		{
			freeReg = GET_FREE_REG();
			iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
			mMidCode->AddVarOperand(iInstrIndex, freeReg);
			AddOperandByFactor(iInstrIndex, resultFactor);
			FreeFactorReg(resultFactor);
		}

		int iJumpIndexEnd = mMidCode->GetNextJumpIndex();
		if (op == OP_TYPE_LOGICAL_OR)
		{
			iInstrIndex = mMidCode->AddInstr(INSTR_JTRUE);
			mMidCode->AddVarOperand(iInstrIndex, freeReg);
			mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndexEnd);
		}
		else if (op == OP_TYPE_LOGICAL_AND)
		{
			iInstrIndex = mMidCode->AddInstr(INSTR_JFALSE);
			mMidCode->AddVarOperand(iInstrIndex, freeReg);
			mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndexEnd);
		}
		else
		{
			iInstrIndex = mMidCode->AddInstr(INSTR_JNE);
			mMidCode->AddVarOperand(iInstrIndex, freeReg);
			mMidCode->AddNilOperand(iInstrIndex);
			mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndexEnd);
		}

		Factor rightFactor = ParseShortCircuitExpr(priority + 1);
		iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
		mMidCode->AddVarOperand(iInstrIndex, freeReg);
		AddOperandByFactor(iInstrIndex, rightFactor);
		FreeFactorReg(rightFactor);

		mMidCode->AddJumpTarget(iJumpIndexEnd);

		resultFactor.type = FACTOR_VAR;
		resultFactor.varIndex = freeReg;
	}
	return resultFactor;
}

CParser::Factor	CParser::ParsePipeline()
{
	Factor resultFactor = ParseShortCircuitExpr();
	while (mLexer->LookNextToken() == TOKEN_TYPE_OP && (mLexer->GetCurOprType() == OP_TYPE_PIPE || mLexer->GetCurOprType() == OP_TYPE_PIPE_RIGHT))
	{
		mLexer->GetNextToken();

		EnsureFactorIsVar(resultFactor);
		PipeLineData pipeline;
		pipeline.autoFill = mLexer->GetCurOprType() == OP_TYPE_PIPE;
		pipeline.parentFactor = resultFactor;
		pipeline.hasFilled = false;
		mCurPipelineFactorStack.push_back(pipeline);
		Factor rightFactor = ParseShortCircuitExpr();
		mCurPipelineFactorStack.pop_back( );
		bool isSameReg = resultFactor.type == FACTOR_VAR && rightFactor.type == FACTOR_VAR && resultFactor.varIndex == rightFactor.varIndex;
		if (!isSameReg)
		{
			FreeFactorReg(resultFactor);
		}
		resultFactor = rightFactor;
	}

	return resultFactor;
}

// 解析子表达式：处理算术/比较运算符优先级
CParser::Factor  CParser::ParseSubExpr()
{
	std::vector<Factor> retVec;
	std::vector<int>	opVec;
	Factor ret2;
	ParseFactor(ret2);

	bool isFirst = true;

	int freeReg = -1;

	retVec.push_back(ret2);
	PendingFunctionReturnFactorScope pendingScope(this, retVec);
	while (mLexer->LookNextToken() == TOKEN_TYPE_OP && GetOpInstrTo(mLexer->GetCurOprType()) >= 0)
	{
		mLexer->ExpectToken(TOKEN_TYPE_OP, -1, "binary expression requires an operator between operands");
		int op = mLexer->GetCurOprType();

		while (opVec.size() > 0 && mLexer->GetOpPrority(op) >= mLexer->GetOpPrority(opVec.back()))
		{
			DoFactorOperation(retVec, opVec);
		}

		Factor ret2;
		ParseFactor(ret2);

		retVec.push_back(ret2);
		opVec.push_back(op);
	}

	while (opVec.size() > 0)
	{
		DoFactorOperation(retVec, opVec);
	}

	Factor resultRet = retVec[0];
	return resultRet;
}

// 执行因子运算：弹出两个因子和一个运算符，生成运算指令或常量折叠
void CParser::DoFactorOperation(std::vector<Factor> &retVec, std::vector<int> &opVec)
{
	Factor f2 = retVec.back();
	retVec.pop_back();
	Factor f1 = retVec.back();
	retVec.pop_back();

	int lastOp = opVec.back();
	opVec.pop_back();

	bool isReg = f1.type == FACTOR_VAR && IsRegVar(f1.varIndex);
	Factor ret = f1;
	if (!IsLogicOp(lastOp) && isReg)
	{
		int instr = GetOpInstr(lastOp);
		int iInstrIndex = mMidCode->AddInstr(instr);
		AddOperandByFactor(iInstrIndex, f1);
		AddOperandByFactor(iInstrIndex, f2);
		FreeFactorReg(f2);
	}
	else
	{
		bool hasResolved = false;
		if ((f1.type == FACTOR_FLOAT || f1.type == FACTOR_INT )
			&& (f2.type == FACTOR_FLOAT || f2.type == FACTOR_INT))
		{
			ret = ComputeFactors(lastOp, f1, f2);
			hasResolved = ret.type != FACTOR_INVALID;
		}

		if(!hasResolved)
		{
			int freeReg = GET_FREE_REG();
			int instr = GetOpInstrTo(lastOp);
			int iInstrIndex = mMidCode->AddInstr(instr);
			mMidCode->AddVarOperand(iInstrIndex, freeReg);
			AddOperandByFactor(iInstrIndex, f1);
			AddOperandByFactor(iInstrIndex, f2);
			ret.type = FACTOR_VAR;
			ret.varIndex = freeReg;
			FreeFactorReg(f1);
			FreeFactorReg(f2);
		}
	
	}

	retVec.push_back(ret);
}

// 解析链式访问结构 a.b[c]().d：将连续的属性访问、索引、函数调用解析为ChainedAccess
void  CParser::ParserVariableAndFunction(ChainedAccess& access, bool accessOnly)
{
	TOKEN nextToken = mLexer->LookNextToken();

	if (nextToken == TOKEN_TYPE_IDENT)
	{
		// 标识符开头: a.b, foo() 等
		mLexer->ExpectToken(TOKEN_TYPE_IDENT, -1, "access expression must start with an identifier");
		ValidateUserIdentName(mLexer->GetCurLexeme());
		std::string firstIdentName = mLexer->GetCurLexeme();

		access.subject.type = FACTOR_VAR;
		access.subject.varIndex = mSymbolTable->SearchValue(firstIdentName.c_str(), mCurFuncIndex);
		access.accessor.type = FACTOR_INVALID;
	}
	else if (nextToken == TOKEN_TYPE_DELIM_OPEN_PAREN
		|| nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE
		|| nextToken == TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE)
	{
		// 非标识符开头的primary表达式: (expr), [list], {table}
		Factor factor;
		if (nextToken == TOKEN_TYPE_DELIM_OPEN_PAREN)
		{
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN, -1, "expected '(' to start parenthesized access expression");
			ParseFactor(factor, true);
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN, -1, "parenthesized access expression must be closed with ')'");
		}
		else if (nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE)
		{
			int reg = ParseListInit();
			factor.type = FACTOR_VAR;
			factor.varIndex = reg;
		}
		else // TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE
		{
			int reg = ParseTableInit();
			factor.type = FACTOR_VAR;
			factor.varIndex = reg;
		}
		EnsureFactorIsVar(factor);
		access.subject = factor;
		access.accessor.type = FACTOR_INVALID;
	}
	else
	{
		Error("expected identifier, '(', '[' or '{' at the beginning of an access expression");
	}

	ParsePostfix(access, accessOnly);
}

// 解析任意表达式后的后缀访问/调用链，如(a).b、foo()、(function(){})()
// accessOnly=true时只处理.和[]访问，不处理()调用和:方法调用（用于函数定义场景）
void CParser::ParsePostfix(ChainedAccess& access, bool accessOnly)
{
	// 可选链跳转回填列表：记录所有 ?. ?( ?[ ?: 生成的 JFALSE 跳转索引
	// 当链结束时，统一回填跳转目标到链末尾
	std::vector<int> optionalChainJumps;

	while (true)
	{
		TOKEN nextToken = mLexer->LookNextToken();

		// 判断是否为可选链 token
		bool isOptional = (nextToken == TOKEN_TYPE_DELIM_OPTIONAL_DOT
			|| nextToken == TOKEN_TYPE_DELIM_OPTIONAL_CALL
			|| nextToken == TOKEN_TYPE_DELIM_OPTIONAL_INDEX
			|| nextToken == TOKEN_TYPE_DELIM_OPTIONAL_COLON);

		if (isOptional)
		{
			// 可选链：先将当前 access 折叠为变量，然后生成 nil 检查跳转
			EnsureFactorIsVar(access.subject);
			FoldTableIndex(access);

			// 生成 JFALSE subject, :end（跳转目标待回填）
			int iJumpIndex = mMidCode->GetNextJumpIndex();
			int iJFalseInstr = mMidCode->AddInstr(INSTR_JFALSE);
			AddOperandByFactor(iJFalseInstr, access.subject);
			mMidCode->AddJumpIndexOperand(iJFalseInstr, iJumpIndex);
			optionalChainJumps.push_back(iJumpIndex);

			// 将可选链 token 映射为对应的普通 token，继续走正常逻辑
			mLexer->GetNextToken(); // consume 可选链 token
			if (nextToken == TOKEN_TYPE_DELIM_OPTIONAL_DOT)
				nextToken = TOKEN_TYPE_DELIM_POINT;
			else if (nextToken == TOKEN_TYPE_DELIM_OPTIONAL_CALL)
				nextToken = TOKEN_TYPE_DELIM_OPEN_PAREN;
			else if (nextToken == TOKEN_TYPE_DELIM_OPTIONAL_INDEX)
				nextToken = TOKEN_TYPE_DELIM_OPEN_BRACE;
			else if (nextToken == TOKEN_TYPE_DELIM_OPTIONAL_COLON)
				nextToken = TOKEN_TYPE_DELIM_COLON;
		}

		if (nextToken == TOKEN_TYPE_DELIM_OPEN_PAREN)
		{
			// 函数定义场景下遇到'('停止，由调用者处理参数列表
			if (accessOnly)
				break;

			if (!isOptional)
				mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN, -1, "function call must start its argument list with '('");
			else
				mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN, -1, "optional function call must be followed by '(' for arguments"); // consume '(' (未被可选链token消耗)

			int iParamNum = 0;
			{
				PendingFunctionReturnSingleFactorScope pendingSubjectScope(this, access.subject);
				PendingFunctionReturnSingleFactorScope pendingAccessorScope(this, access.accessor);
				iParamNum = ParseCallArgs();
			}


			BeforeEmitCall();
			int instrIndex = mMidCode->AddInstr(INSTR_CALL);

			AddOperandByAccess(access, instrIndex);

			mMidCode->AddIntOperand(instrIndex, iParamNum);
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN, -1, "function call argument list must be closed with ')'");

			FreeFactorReg(access.subject);
			FreeFactorReg(access.accessor);

			access.subject.type = FACTOR_FUNC;
			access.accessor.type = FACTOR_INVALID;
		}
		else if (nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE || nextToken == TOKEN_TYPE_DELIM_POINT)
		{ 
			if (!isOptional)
			{
				EnsureFactorIsVar(access.subject);
				FoldTableIndex(access);
			}
			// 可选链时已在上面完成 EnsureFactorIsVar + FoldTableIndex

			if (nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE)
			{
				if (!isOptional)
					mLexer->GetNextToken();
				else
					mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_BRACE, -1, "optional index access must be followed by '['"); // consume '[' (未被可选链token消耗)

				access.accessor = ParseExpr(false, true);
				mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_BRACE, -1, "index access expression must be closed with ']'");
			}
			else
			{
				if (!isOptional)
					mLexer->GetNextToken(); // consume '.'
				// 可选链时 '.' 已被 ?. token 包含，无需再消耗

				mLexer->ExpectToken(TOKEN_TYPE_IDENT, -1, "member access requires an identifier after '.'");
				std::string name = mLexer->GetCurLexeme();
				int strIndex = mSymbolTable->AddString(name.c_str());

				access.accessor.type = FACTOR_STRING;
				access.accessor.strIndex = strIndex;
			}

		}
		else if (nextToken == TOKEN_TYPE_DELIM_COLON)
		{
			// 函数定义场景下遇到':'停止，由调用者处理方法定义
			if (accessOnly)
				break;

			if (!isOptional)
				mLexer->GetNextToken(); // consume ':'
			// 可选链时 ':' 已被 ?: token 包含，无需再消耗

			mLexer->ExpectToken(TOKEN_TYPE_IDENT, -1, "method call requires a method name after ':'");
			std::string classFuncName = mLexer->GetCurLexeme();

			mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN, -1, "method call must be followed by '(' to start argument list");

			if (!isOptional)
			{
				EnsureFactorIsVar(access.subject);
				FoldTableIndex(access);
			}
			// 可选链时已在上面完成 EnsureFactorIsVar + FoldTableIndex

			int iInstrIndex = mMidCode->AddInstr(INSTR_PUSH);
			AddOperandByFactor(iInstrIndex, access.subject);

			int iParamNum = 0;
			{
				PendingFunctionReturnSingleFactorScope pendingSubjectScope(this, access.subject);
				PendingFunctionReturnSingleFactorScope pendingAccessorScope(this, access.accessor);
				iParamNum = ParseCallArgs(1);
			}
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN, -1, "method call argument list must be closed with ')'");

			BeforeEmitCall();
			int instrIndex = mMidCode->AddInstr(INSTR_CALL);
			int strIndex = mSymbolTable->AddString(classFuncName.c_str());
			mMidCode->AddTableStringOperand(instrIndex, access.subject.varIndex, strIndex);
			mMidCode->AddIntOperand(instrIndex, iParamNum);
		
			FreeFactorReg(access.subject);
			FreeFactorReg(access.accessor);

			access.subject.type = FACTOR_FUNC;
			access.accessor.type = FACTOR_INVALID;

		}
		else
		{
			break;
		}
	}

	// 回填所有可选链的 JFALSE 跳转目标到链末尾
	if (!optionalChainJumps.empty())
	{
		// 将当前 access 折叠为一个变量（确保结果在一个寄存器/变量中）
		EnsureFactorIsVar(access.subject);
		FoldTableIndex(access);

		// 生成一个 JMP 跳过 LOADNIL（正常路径跳过 nil 赋值）
		int iSkipNilJump = mMidCode->GetNextJumpIndex();
		int iJmpInstr = mMidCode->AddInstr(INSTR_JMP);
		mMidCode->AddJumpIndexOperand(iJmpInstr, iSkipNilJump);

		// 回填所有 JFALSE 跳转到这里：将结果设为 nil
		for (int jumpIdx : optionalChainJumps)
		{
			mMidCode->AddJumpTarget(jumpIdx);
		}

		// 生成 LOADNIL subject（将结果设为 nil）
		int iLoadNilInstr = mMidCode->AddInstr(INSTR_LOADNIL);
		Factor factor;
		ConvertAccessorToFactor(access, factor);
		AddOperandByFactor(iLoadNilInstr, factor);
		// 正常路径跳转目标（跳过 LOADNIL）
		mMidCode->AddJumpTarget(iSkipNilJump);
	}
}

// 根据表索引因子类型向指令添加表操作数
void CParser::AddTableFactor(Factor &lastRet, int iInstrIndex, int& symbIndex)
{
	if (lastRet.type == FACTOR_INT)
	{
		mMidCode->AddTableIntOperand(iInstrIndex, symbIndex, lastRet.intValue);
	}
	else if (lastRet.type == FACTOR_FLOAT)
	{
		mMidCode->AddTableFloatOperand(iInstrIndex, symbIndex, lastRet.floatValue);
	}
	else if (lastRet.type == FACTOR_STRING)
	{
		mMidCode->AddTableStringOperand(iInstrIndex, symbIndex, lastRet.strIndex);
	}
	else if (lastRet.type == FACTOR_VAR)
	{
		mMidCode->AddTableIndexOperand(iInstrIndex, symbIndex, lastRet.varIndex);
	}
	else if (lastRet.type == FACTOR_FUNC)
	{
		mMidCode->AddTableRegOperand(iInstrIndex, symbIndex, 0);
	}
	else if(lastRet.type == FACTOR_INVALID)
	{
		mMidCode->AddVarOperand(iInstrIndex, symbIndex);
	}
}

// 解析if条件语句：if (expr) stmt [else stmt]
bool  CParser::ParseIf()
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_IF, -1, "expected 'if' to start if statement");
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN, -1, "if condition must start with '('");
	Factor ret = ParseExpr(false);
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN, -1, "if condition must be closed with ')'");

	int iJumpIndex1 = mMidCode->GetNextJumpIndex();
	int iJumpIndex2;
	int iInstrIndex = mMidCode->AddInstr(INSTR_JFALSE);
	AddOperandByFactor(iInstrIndex, ret);
	mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex1);

	FreeFactorReg(ret);
	ParseStateMent();

	bool hasReturnEarly = false;
	bool bHasElse = false;
	if (mLexer->LookNextToken() == TOKEN_TYPE_RSRVD_ELSE)
	{
		int lastOp = mMidCode->getLastInstrOp();
		if (lastOp == INSTR_RET)
			hasReturnEarly = true;

		if (!hasReturnEarly)
		{
			iJumpIndex2 = mMidCode->GetNextJumpIndex();
			iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
			mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex2);
		}
		bHasElse = true;

	}
	mMidCode->AddJumpTarget(iJumpIndex1);
	if (bHasElse)
	{
		mLexer->ExpectToken(TOKEN_TYPE_RSRVD_ELSE, -1, "expected 'else' before else branch");
		ParseStateMent();
		if (!hasReturnEarly)
		   mMidCode->AddJumpTarget(iJumpIndex2);
	}
	return true;
}
// 解析return语句：支持多返回值
bool  CParser::ParseReturnOrYield(bool isYield)
{
	mLexer->GetNextToken();

	int token = mLexer->LookNextToken();
	if (token == TOKEN_TYPE_DELIM_SEMICOLON || token == TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE || token == TOKEN_TYPE_END_OF_STREAM)
	{
		if (token == TOKEN_TYPE_DELIM_SEMICOLON)
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_SEMICOLON, -1, "empty return/yield statement must end with ';'");
		int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
		mMidCode->AddRegOperand(iInstrIndex);
		mMidCode->AddNilOperand(iInstrIndex);
		if (isYield)
		{
			iInstrIndex = mMidCode->AddInstr(INSTR_YIELD);
		}
		else
		{
			iInstrIndex = mMidCode->AddInstr(INSTR_RET);
		}
		mMidCode->AddIntOperand(iInstrIndex, 0);
	}
	else
	{
		std::vector<Factor> retVec;
		PendingFunctionReturnFactorScope pendingScope(this, retVec);
		while (true)
		{
			Factor ret = ParseExpr(false);
			retVec.push_back(ret);
			if (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_COMMA)
				break;

			mLexer->GetNextToken();
		}

		if (retVec.size() >= MAX_FUNC_REG)
		{
			if(isYield)
				Error("too many yield values: maximum allowed is %d, but got %d", MAX_FUNC_REG, (int)retVec.size());
			else
				Error("too many return values: maximum allowed is %d, but got %d", MAX_FUNC_REG, (int)retVec.size());
		}

		mLexer->TestToken(TOKEN_TYPE_DELIM_SEMICOLON);
		if (retVec.size() > 1)
			StabilizePendingFunctionReturns(retVec);
		int iInstrIndex;
		for (int i = 0; i < (int)retVec.size(); i++)
		{
			iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
			mMidCode->AddRegOperand(iInstrIndex, i);
			AddOperandByFactor(iInstrIndex, retVec[i]);
			FreeFactorReg(retVec[i]);
		}
		
		if (isYield)
		{
			iInstrIndex = mMidCode->AddInstr(INSTR_YIELD);
		}
		else
		{
			iInstrIndex = mMidCode->AddInstr(INSTR_RET);
		}
		mMidCode->AddIntOperand(iInstrIndex, (int)retVec.size());
	}
	return true;
}

// 解析while循环：while (expr) stmt
bool  CParser::ParseWhile()
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_WHILE, -1, "expected 'while' to start while loop");
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN, -1, "while condition must start with '('");
	int iJumpIndex1 = mMidCode->GetNextJumpIndex();
	int iJumpIndex2 = mMidCode->GetNextJumpIndex();
	mMidCode->AddJumpTarget(iJumpIndex1);
	Factor ret = ParseExpr(false);
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN, -1, "while condition must be closed with ')'");

	//int iInstrIndex = mMidCode->AddInstr(INSTR_POP);
	//mMidCode->AddVarOperand(iInstrIndex, mSymbolIndexT0);

	int iInstrIndex = mMidCode->AddInstr(INSTR_JFALSE);
	AddOperandByFactor(iInstrIndex, ret);
	mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex2);
	mLoopStack.push(LoopStruct(iJumpIndex1, iJumpIndex2));
	FreeFactorReg(ret);
	ParseStateMent();
	mLoopStack.pop();

	iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
	mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex1);
	mMidCode->AddJumpTarget(iJumpIndex2);
	return true;
}

// 解析break语句：跳出当前循环
bool CParser::ParseBreak()
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_BREAK, -1, "expected 'break' statement");
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_SEMICOLON, -1, "break statement must end with ';'");
	if (mLoopStack.empty())
		Error("'break' statement can only be used inside a loop (for, while, foreach)");
	int iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
	mMidCode->AddJumpIndexOperand(iInstrIndex, mLoopStack.top().iEndJumpIndex);
	return true;
}

// 解析continue语句：跳到循环开头继续执行
bool CParser::ParseContinue()
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_CONTINUE, -1, "expected 'continue' statement");
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_SEMICOLON, -1, "continue statement must end with ';'");
	if (mLoopStack.empty())
		Error("'continue' statement can only be used inside a loop (for, while, foreach)");
	int iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
	mMidCode->AddJumpIndexOperand(iInstrIndex, mLoopStack.top().iContinueJumpIndex);
	return true;
}

// 解析foreach迭代语句：foreach (var1, var2 in expr) stmt
bool  CParser::ParseForeach()
{
	/*
	foreach (var_1, ..., var_n in <explist>) { <block> } -- 锟酵等硷拷锟斤拷锟斤拷锟铰达拷锟诫：
		do
			local _f, _s, _var = <explist>    --锟斤拷锟截碉拷锟斤拷锟斤拷锟斤拷锟斤拷锟姐定状态锟酵匡拷锟狡憋拷锟斤拷锟侥筹拷值
			while true do
				local var_1, ..., var_n = _f(_s, _var)
				_var = var_1
				if _var == nil then break end
					<block>
					end
					end
					end

	*/
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_FOREACH, -1, "expected 'foreach' to start foreach loop");
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN, -1, "foreach loop header must start with '('");

	std::vector<int>	localValVec;
	while (true)
	{
		mLexer->ExpectToken(TOKEN_TYPE_IDENT, -1, "foreach loop requires an iteration variable name before 'in'");
		ValidateUserIdentName(mLexer->GetCurLexeme());
		std::string identName = mLexer->GetCurLexeme();
		localValVec.push_back(mSymbolTable->AddVariant(identName.c_str(), mCurFuncIndex, 1, IDENT_TYPE_VAR, (int)mMidCode->GetCurCodeIndex()));
		if (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_COMMA)
			break;
		mLexer->GetNextToken();
	}

	if (localValVec.size() > MAX_FUNC_REG)
	{
		Error("too many iteration variables in foreach: maximum allowed is %d, but got %d", MAX_FUNC_REG, (int)localValVec.size());
	}

	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_IN, -1, "foreach loop requires 'in' between iteration variables and iterable expression");

	std::vector<Factor> retVec;
	{
		PendingFunctionReturnFactorScope pendingScope(this, retVec);
		while (true)
		{
			Factor ret = ParseExpr(false);
			retVec.push_back(ret);
			if (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_COMMA)
				break;
			mLexer->GetNextToken();
		}
	}

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN, -1, "foreach loop header must be closed with ')'");

	char funcVarName[128] = { 0 }, stateVarName[128] = { 0 }, initVarName[128] = { 0 };
	snprintf(funcVarName, 128, "%sforeach_func_%d", XSCRIPT_INTERNAL_IDENT_PREFIX, (int)mLoopStack.size());
	snprintf(stateVarName, 128, "%sforeach_state_%d", XSCRIPT_INTERNAL_IDENT_PREFIX, (int)mLoopStack.size());
	snprintf(initVarName, 128, "%sforeach_init_%d", XSCRIPT_INTERNAL_IDENT_PREFIX, (int)mLoopStack.size());

	mSymbolTable->EnterBlock(mCurFuncIndex, mMidCode->GetCurCodeIndex());
	int foreachStartPc = (int)mMidCode->GetCurCodeIndex();
	int funcVarIndex = mSymbolTable->AddVariant(funcVarName, mCurFuncIndex, 1, IDENT_TYPE_VAR, foreachStartPc);
	int stateVarIndex = mSymbolTable->AddVariant(stateVarName, mCurFuncIndex, 1, IDENT_TYPE_VAR, foreachStartPc);
	int initVarIndex = mSymbolTable->AddVariant(initVarName, mCurFuncIndex, 1, IDENT_TYPE_VAR, foreachStartPc);
	Factor factor1, factor2, factor3;
	factor1.type = FACTOR_VAR;
	factor1.varIndex = funcVarIndex;
	factor2.type = FACTOR_VAR;
	factor2.varIndex = stateVarIndex;
	factor3.type = FACTOR_VAR;
	factor3.varIndex = initVarIndex;

	std::vector<ChainedAccess> varVec;
	varVec.push_back(ChainedAccess(factor1, Factor()));
	varVec.push_back(ChainedAccess(factor2, Factor()));
	varVec.push_back(ChainedAccess(factor3, Factor()));
	MultiAssignment(retVec, varVec);

	int iJumpIndex1 = mMidCode->GetNextJumpIndex();
	int iJumpIndex2 = mMidCode->GetNextJumpIndex();

	mMidCode->AddJumpTarget(iJumpIndex1);

	int iInstrIndex = mMidCode->AddInstr(INSTR_PUSH);
	mMidCode->AddVarOperand(iInstrIndex, stateVarIndex);

	iInstrIndex = mMidCode->AddInstr(INSTR_PUSH);
	mMidCode->AddVarOperand(iInstrIndex, initVarIndex);


	BeforeEmitCall();
	iInstrIndex = mMidCode->AddInstr(INSTR_CALL);
	mMidCode->AddVarOperand(iInstrIndex, funcVarIndex);
	mMidCode->AddIntOperand(iInstrIndex, 2);

	for (int i = 0; i < (int)localValVec.size(); i++)
	{
		iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
		mMidCode->AddVarOperand(iInstrIndex, localValVec[i]);
		mMidCode->AddRegOperand(iInstrIndex, i);
	}

	iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
	mMidCode->AddVarOperand(iInstrIndex, initVarIndex);
	mMidCode->AddVarOperand(iInstrIndex, localValVec[0]);

	iInstrIndex = mMidCode->AddInstr(INSTR_JE);
	mMidCode->AddVarOperand(iInstrIndex, initVarIndex);
	mMidCode->AddNilOperand(iInstrIndex);
	mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex2);


	mLoopStack.push(LoopStruct(iJumpIndex1, iJumpIndex2));
	ParseStateMent();
	mLoopStack.pop();

	iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
	mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex1);
	mMidCode->AddJumpTarget(iJumpIndex2);


	mSymbolTable->LeaveBlock(mCurFuncIndex, mMidCode->GetCurCodeIndex());

	return true;
}

// 解析迭代器
bool CParser::ParseIter()
{
	/*
	iterator(var_1, ..., var_n in (expr)) { <block> }  
		do
			local _iter = (expr).__iterator__()
			while true do
				local ok, var_1, ..., var_n = _iter:__next__()
				if ok != 1 then
					break
				end
				<block>
					
			end
		end
	*/

	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_ITERATOR, -1, "expected 'iterator' to start iterator loop");
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN, -1, "iterator loop header must start with '('");

	mSymbolTable->EnterBlock(mCurFuncIndex, mMidCode->GetCurCodeIndex());

	std::vector<int>	localValVec;
	while (true)
	{
		mLexer->ExpectToken(TOKEN_TYPE_IDENT, -1, "iterator loop requires an iteration variable name before 'in'");
		ValidateUserIdentName(mLexer->GetCurLexeme());
		std::string identName = mLexer->GetCurLexeme();
		localValVec.push_back(mSymbolTable->AddVariant(identName.c_str(), mCurFuncIndex, 1, IDENT_TYPE_VAR, (int)mMidCode->GetCurCodeIndex()));
		if (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_COMMA)
			break;
		mLexer->GetNextToken();
	}
	if (localValVec.size() > MAX_FUNC_REG - 1)
	{
		Error("too many iteration variables in Iterator: maximum allowed is %d, but got %d", MAX_FUNC_REG - 1, (int)localValVec.size());
	}
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_IN, -1, "iterator loop requires 'in' between iteration variables and iterable expression");

	Factor exprFactor = ParseExpr(false, true);

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN, -1, "iterator loop header must be closed with ')'");

	EnsureFactorIsVar(exprFactor);


	int funcStrIndex = mSymbolTable->AddString(ITER_FUNC);
	int nextStrIndex = mSymbolTable->AddString(ITER_NEXT);

	//自身压栈
	int instrIndex = mMidCode->AddInstr(INSTR_PUSH);
	mMidCode->AddVarOperand(instrIndex, exprFactor.varIndex);

	//调用 __iterator__ 函数
	BeforeEmitCall();
	instrIndex = mMidCode->AddInstr(INSTR_CALL);
	mMidCode->AddTableStringOperand(instrIndex,exprFactor.varIndex, funcStrIndex);
	mMidCode->AddIntOperand(instrIndex, 1);

	//缓存_iter, (expr).__iterator__() 的函数返回
	int iterVarIndex = GET_FREE_REG();
	instrIndex = mMidCode->AddInstr(INSTR_MOV);
	mMidCode->AddVarOperand(instrIndex, iterVarIndex);
	mMidCode->AddRegOperand(instrIndex);


	//缓存__next__ 函数
	int nextVarIndex = GET_FREE_REG();
	instrIndex = mMidCode->AddInstr(INSTR_MOV);
	mMidCode->AddVarOperand(instrIndex, nextVarIndex);
	mMidCode->AddTableStringOperand(instrIndex, iterVarIndex, nextStrIndex);

	int iJumpIndex1 = mMidCode->GetNextJumpIndex();
	int iJumpIndex2 = mMidCode->GetNextJumpIndex();
	mMidCode->AddJumpTarget(iJumpIndex1);

	instrIndex = mMidCode->AddInstr(INSTR_PUSH);
	mMidCode->AddVarOperand(instrIndex, iterVarIndex);

	BeforeEmitCall();
	instrIndex = mMidCode->AddInstr(INSTR_CALL);
	mMidCode->AddVarOperand(instrIndex, nextVarIndex);
	mMidCode->AddIntOperand(instrIndex, 1);

	instrIndex = mMidCode->AddInstr(INSTR_JE);
	mMidCode->AddRegOperand(instrIndex, 0);
	mMidCode->AddIntOperand(instrIndex, 0);

	mMidCode->AddJumpIndexOperand(instrIndex, iJumpIndex2);

	for (int i = 0; i < (int)localValVec.size(); i++)
	{
		instrIndex = mMidCode->AddInstr(INSTR_MOV);
		mMidCode->AddVarOperand(instrIndex, localValVec[i]);
		mMidCode->AddRegOperand(instrIndex, i + 1);
	}


	mLoopStack.push(LoopStruct(iJumpIndex1, iJumpIndex2));
	ParseStateMent();
	mLoopStack.pop();

	instrIndex = mMidCode->AddInstr(INSTR_JMP);
	mMidCode->AddJumpIndexOperand(instrIndex, iJumpIndex1);
	mMidCode->AddJumpTarget(iJumpIndex2);
	mSymbolTable->LeaveBlock(mCurFuncIndex, mMidCode->GetCurCodeIndex());
	FREE_REG(iterVarIndex);
	FREE_REG(nextVarIndex);
	FreeFactorReg(exprFactor);
	return true;
}

void  CParser::ParseTableListComprehension(int listVarIndex, int bufferIndex, int valueBufferIndex)
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_ITERATOR, -1, "expected 'iterator' to start iterator loop");
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN, -1, "iterator loop header must start with '('");

	mSymbolTable->EnterBlock(mCurFuncIndex, mMidCode->GetCurCodeIndex());

	std::vector<int>	localValVec;
	while (true)
	{
		mLexer->ExpectToken(TOKEN_TYPE_IDENT, -1, "iterator loop requires an iteration variable name before 'in'");
		ValidateUserIdentName(mLexer->GetCurLexeme());
		std::string identName = mLexer->GetCurLexeme();
		localValVec.push_back(mSymbolTable->AddVariant(identName.c_str(), mCurFuncIndex, 1, IDENT_TYPE_VAR, (int)mMidCode->GetCurCodeIndex()));
		if (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_COMMA)
			break;
		mLexer->GetNextToken();
	}
	if (localValVec.size() > MAX_FUNC_REG - 1)
	{
		Error("too many iteration variables in Iterator: maximum allowed is %d, but got %d", MAX_FUNC_REG - 1, (int)localValVec.size());
	}
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_IN, -1, "iterator loop requires 'in' between iteration variables and iterable expression");

	Factor exprFactor = ParseExpr(false, true);

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN, -1, "iterator loop header must be closed with ')'");

	EnsureFactorIsVar(exprFactor);


	int funcStrIndex = mSymbolTable->AddString(ITER_FUNC);
	int nextStrIndex = mSymbolTable->AddString(ITER_NEXT);

	//自身压栈
	int instrIndex = mMidCode->AddInstr(INSTR_PUSH);
	mMidCode->AddVarOperand(instrIndex, exprFactor.varIndex);

	//调用 __iterator__ 函数
	BeforeEmitCall();
	instrIndex = mMidCode->AddInstr(INSTR_CALL);
	mMidCode->AddTableStringOperand(instrIndex, exprFactor.varIndex, funcStrIndex);
	mMidCode->AddIntOperand(instrIndex, 1);

	//缓存_iter, (expr).__iterator__() 的函数返回
	int iterVarIndex = GET_FREE_REG();
	instrIndex = mMidCode->AddInstr(INSTR_MOV);
	mMidCode->AddVarOperand(instrIndex, iterVarIndex);
	mMidCode->AddRegOperand(instrIndex);


	//缓存__next__ 函数
	int nextVarIndex = GET_FREE_REG();
	instrIndex = mMidCode->AddInstr(INSTR_MOV);
	mMidCode->AddVarOperand(instrIndex, nextVarIndex);
	mMidCode->AddTableStringOperand(instrIndex, iterVarIndex, nextStrIndex);

	int iJumpIndex1 = mMidCode->GetNextJumpIndex();
	int iJumpIndex2 = mMidCode->GetNextJumpIndex();
	mMidCode->AddJumpTarget(iJumpIndex1);

	instrIndex = mMidCode->AddInstr(INSTR_PUSH);
	mMidCode->AddVarOperand(instrIndex, iterVarIndex);

	BeforeEmitCall();
	instrIndex = mMidCode->AddInstr(INSTR_CALL);
	mMidCode->AddVarOperand(instrIndex, nextVarIndex);
	mMidCode->AddIntOperand(instrIndex, 1);

	instrIndex = mMidCode->AddInstr(INSTR_JE);
	mMidCode->AddRegOperand(instrIndex, 0);
	mMidCode->AddIntOperand(instrIndex, 0);

	mMidCode->AddJumpIndexOperand(instrIndex, iJumpIndex2);

	for (int i = 0; i < (int)localValVec.size(); i++)
	{
		instrIndex = mMidCode->AddInstr(INSTR_MOV);
		mMidCode->AddVarOperand(instrIndex, localValVec[i]);
		mMidCode->AddRegOperand(instrIndex, i + 1);
	}
	int iJumpIndex3 = -1;
	//如果有守卫条件
	if (mLexer->TestToken(TOKEN_TYPE_RSRVD_IF))
	{
		iJumpIndex3  = mMidCode->GetNextJumpIndex();
		Factor ifFactor = ParseExpr(false);
		instrIndex = mMidCode->AddInstr(INSTR_JE);
		AddOperandByFactor(instrIndex, ifFactor);
		mMidCode->AddIntOperand(instrIndex, 0);
		mMidCode->AddJumpIndexOperand(instrIndex, iJumpIndex3);
		FreeFactorReg(ifFactor);
	}
	if (valueBufferIndex >= 0)
	{
		beginParseFromBuffer(bufferIndex);
		//key 需要保证不是 双操作数
		Factor keyFactor = ParseExpr(false, true);
		endParseFromBuffer();

		PendingFunctionReturnSingleFactorScope pendingKeyFactorScope(this, keyFactor);
		beginParseFromBuffer(valueBufferIndex);
		Factor valueFactor = ParseExpr(false);
		endParseFromBuffer();
		
		instrIndex = mMidCode->AddInstr(INSTR_MOV);
		AddTableFactor(keyFactor, instrIndex, listVarIndex);
		AddOperandByFactor(instrIndex, valueFactor);

		FreeFactorReg(keyFactor);
		FreeFactorReg(valueFactor);
	}
	else
	{
		beginParseFromBuffer(bufferIndex);
		Factor assignFactor = ParseExpr(false);

		instrIndex = mMidCode->AddInstr(INSTR_APPEND);
		mMidCode->AddVarOperand(instrIndex, listVarIndex);
		AddOperandByFactor(instrIndex, assignFactor);
		FreeFactorReg(assignFactor);
		endParseFromBuffer();
	}

	if (iJumpIndex3 >= 0)
	{
		mMidCode->AddJumpTarget(iJumpIndex3);
	}

	instrIndex = mMidCode->AddInstr(INSTR_JMP);
	mMidCode->AddJumpIndexOperand(instrIndex, iJumpIndex1);
	mMidCode->AddJumpTarget(iJumpIndex2);
	mSymbolTable->LeaveBlock(mCurFuncIndex, mMidCode->GetCurCodeIndex());
	FREE_REG(iterVarIndex);
	FREE_REG(nextVarIndex);
	FreeFactorReg(exprFactor);
}


// 解析for循环：for (init; condition; increment) stmt
bool  CParser::ParseFor()
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_FOR, -1, "expected 'for' to start for loop");
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN, -1, "for loop header must start with '('");
	mSymbolTable->EnterBlock(mCurFuncIndex, (int)mMidCode->GetCurCodeIndex());
	if (mLexer->LookNextToken() == TOKEN_TYPE_DELIM_SEMICOLON)
	{
		// 空初始化语句: for(; ...)
		mLexer->GetNextToken();
	}
	else if (mLexer->LookNextToken() == TOKEN_TYPE_RSRVD_VAR)
	{
		ParseVar();
	}
	else
	{
		ParserIdent();
	}
	int iJumpIndex1 = mMidCode->GetNextJumpIndex();
	int iJumpIndex2 = mMidCode->GetNextJumpIndex();
	int iJumpIndex3 = mMidCode->GetNextJumpIndex(); // continue target (before increment)
	mMidCode->AddJumpTarget(iJumpIndex1);
	Factor ret = ParseExpr(false);
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_SEMICOLON, -1, "for loop condition must be followed by ';' before increment expression");
	//int iInstrIndex = mMidCode->AddInstr(INSTR_POP);
	//mMidCode->AddVarOperand(iInstrIndex, mSymbolIndexT0);

	int iInstrIndex = mMidCode->AddInstr(INSTR_JFALSE);
	AddOperandByFactor(iInstrIndex, ret);
	mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex2);
	mLoopStack.push(LoopStruct(iJumpIndex1, iJumpIndex2, iJumpIndex3));
	FreeFactorReg(ret);

	int incrementBufferIndex = beginRecordTokens();
	bool hasIncrement = (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_CLOSE_PAREN);
	if (hasIncrement)
	{
		ParserIdent(false);
	}
	endRecordTokens();

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN, -1, "for loop header must be closed with ')'");
	ParseStateMent();
	mLoopStack.pop();

	mMidCode->AddJumpTarget(iJumpIndex3);

	if (hasIncrement)
	{
		beginParseFromBuffer(incrementBufferIndex);
		ParserIdent(false);
		endParseFromBuffer();
	}

	iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
	mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex1);
	mMidCode->AddJumpTarget(iJumpIndex2);
	mSymbolTable->LeaveBlock(mCurFuncIndex, (int)mMidCode->GetCurCodeIndex());
	return true;
}

// 解析列表初始化表达式 [a, b, c]
int   CParser::ParseListInit(int initReg)
{
	int tableReg = initReg;
	int instrIndex = -1;
	if (tableReg < 0)
	{
		tableReg = GET_FREE_REG();
	}

	instrIndex = mMidCode->AddInstr(INSTR_TYPE);
	mMidCode->AddVarOperand(instrIndex, tableReg);
	mMidCode->AddIntOperand(instrIndex, OP_TYPE_LIST);
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_BRACE, -1, "list initializer must start with '['");
	
	//是否是推导式
	bool isComprehension = false;
	bool isFirstElement = true;
	TOKEN nextToken = mLexer->LookNextToken();
	int bufferIndex = -1;
	while (true)
	{
		
		if (nextToken != TOKEN_TYPE_DELIM_CLOSE_BRACE)
		{
			Factor ret;
			if (isFirstElement)
			{
				bufferIndex = beginRecordTokens();
				ret = ParseExpr(false);
				FreeFactorReg(ret);
				endRecordTokens();
				nextToken = mLexer->LookNextToken();
				//如果发现是推导式， 果断跳出 走推导式流程
				if (nextToken == (TOKEN_TYPE_RSRVD_ITERATOR))
				{
					isComprehension = true;
					break;
				}
				else
				{
					beginParseFromBuffer(bufferIndex);
					ret = ParseExpr(false);
					endParseFromBuffer();
				}
			}
			else
			{
				ret = ParseExpr(false);
			}
			instrIndex = mMidCode->AddInstr(INSTR_APPEND);
			mMidCode->AddVarOperand(instrIndex, tableReg);
			AddOperandByFactor(instrIndex, ret);
			FreeFactorReg(ret);

			isFirstElement = false;
		}
		
		nextToken = mLexer->LookNextToken();
		if (nextToken == TOKEN_TYPE_DELIM_CLOSE_BRACE)
		{
			break;
		}
		mLexer->ExpectToken(TOKEN_TYPE_DELIM_COMMA, -1, "list initializer items must be separated by ',' or closed with ']'");
	}

	if(isComprehension)
	{
		ParseTableListComprehension(tableReg, bufferIndex, -1);
	}

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_BRACE, -1, "list initializer must be closed with ']'");
	return tableReg;
}

// 解析表（字典）初始化表达式 {key=value, ...}
int  CParser::ParseTableInit(int initReg)
{
	int tableReg = initReg;
	int instrIndex = -1;
	if (tableReg < 0)
	{
		tableReg = GET_FREE_REG();
	}

	instrIndex = mMidCode->AddInstr(INSTR_TYPE);
	mMidCode->AddVarOperand(instrIndex, tableReg);
	mMidCode->AddIntOperand(instrIndex, OP_TYPE_TABLE);

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE, -1, "table initializer must start with '{'");

	int tableIndex = 0;
	bool isFirstElement = true;
	int keyBufferIndex = -1;
	int valueBufferIndex = -1;
	bool isComprehension = false;
	while (true)
	{
		TOKEN nextToken = mLexer->LookNextToken();

		if (nextToken == TOKEN_TYPE_DELIM_OPEN_PAREN)
		{
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN, -1, "computed table key must start with '('");

			Factor keyFactor;
			Factor valueFactor;
			if (isFirstElement)
			{
				keyBufferIndex = beginRecordTokens();
				keyFactor = ParseExpr(false, true);
				FreeFactorReg(keyFactor);
				endRecordTokens();

				mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN, -1, "computed table key must be closed with ')' before '='");
				mLexer->ExpectToken(TOKEN_TYPE_OP, OP_TYPE_ASSIGN, "computed table key must be followed by '=' before its value");

				valueBufferIndex = beginRecordTokens();
				valueFactor = ParseExpr(false);
				FreeFactorReg(valueFactor);
				endRecordTokens();

				int nextToken2 = mLexer->LookNextToken();
				if (nextToken2 == TOKEN_TYPE_RSRVD_ITERATOR)
				{
					isComprehension = true;
					break;
				}
				else
				{
					beginParseFromBuffer(keyBufferIndex);
					keyFactor = ParseExpr(false, true);
					endParseFromBuffer();
					//保证 函数返回寄存器不被破坏
					PendingFunctionReturnSingleFactorScope pendingSubjectScope(this, keyFactor);
					beginParseFromBuffer(valueBufferIndex);
					valueFactor = ParseExpr(false);
					endParseFromBuffer();

				}
			}
			else
			{
				keyFactor = ParseExpr(false, true);
				mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN, -1, "computed table key must be closed with ')' before '='");
				mLexer->ExpectToken(TOKEN_TYPE_OP, OP_TYPE_ASSIGN, "computed table key must be followed by '=' before its value");

				//保证 函数返回寄存器不被破坏
				PendingFunctionReturnSingleFactorScope pendingSubjectScope(this, keyFactor);
				valueFactor = ParseExpr(false);
			}

			instrIndex = mMidCode->AddInstr(INSTR_MOV);
			AddTableFactor(keyFactor, instrIndex, tableReg);
			AddOperandByFactor(instrIndex, valueFactor);
			FreeFactorReg(keyFactor);
			FreeFactorReg(valueFactor);

			isFirstElement = false;
		}
		else
		{
			bool needParseExpr = true;
			if (nextToken == TOKEN_TYPE_IDENT)
			{
				if (isFirstElement)
				{
					keyBufferIndex = beginRecordTokens();
				}

				mLexer->GetNextToken();
				if (isFirstElement && keyBufferIndex >= 0)
				{
					endRecordTokens();
				}
				std::string identName = mLexer->GetCurLexeme();

				nextToken = mLexer->LookNextToken();
				if (nextToken == TOKEN_TYPE_OP && mLexer->GetCurOprType() == OP_TYPE_ASSIGN)
				{
					mLexer->GetNextToken();
					Factor ret2 = ParseExpr(false);
					needParseExpr = false;
					instrIndex = mMidCode->AddInstr(INSTR_MOV);
					int strIndex = mSymbolTable->AddString(identName.c_str());
					mMidCode->AddTableStringOperand(instrIndex, tableReg, strIndex);
					AddOperandByFactor(instrIndex, ret2);
					FreeFactorReg(ret2);
					isFirstElement = false;
				}
				else
				{
					mLexer->RewindToken();
					nextToken = mLexer->LookNextToken();
				}
			}

			if (nextToken != TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE && needParseExpr)
			{
				Factor ret = ParseExpr(false);
				instrIndex = mMidCode->AddInstr(INSTR_MOV);
				mMidCode->AddTableIntOperand(instrIndex, tableReg, tableIndex);
				AddOperandByFactor(instrIndex, ret);
				FreeFactorReg(ret);
				tableIndex++;
				isFirstElement = false;
			}
		}

		nextToken = mLexer->LookNextToken();
		if (nextToken == TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE)
		{
			break;
		}

		mLexer->ExpectToken(TOKEN_TYPE_DELIM_COMMA, -1, "table initializer entries must be separated by ',' or closed with '}'");
	}

	if (isComprehension)
	{
		ParseTableListComprehension(tableReg, keyBufferIndex, valueBufferIndex);
	}

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE, -1, "table initializer must be closed with '}'");
	return tableReg;

}


// 开始记录Token（用于for循环的增量表达式延迟解析）

int  CParser::beginRecordTokens()
{
	mRecordTokenDepth++;
	mIsRecordToken = true;
	int bufferIndex = mLexer->beginRecordTokenBuffer();
	mMidCode->setIngoreInstr(true);
	mSymbolTable->SetIsRecord(true);
	return bufferIndex;
}

// 结束记录Token
void  CParser::endRecordTokens()
{
	mLexer->endRecordTokenBuffer();
	if (mRecordTokenDepth > 0)
		mRecordTokenDepth--;
	mIsRecordToken = (mRecordTokenDepth > 0);
	mMidCode->setIngoreInstr(mIsRecordToken);
	mSymbolTable->SetIsRecord(mIsRecordToken);
}

// 从指定缓冲区回放Token开始解析

void  CParser::beginParseFromBuffer(int bufferIndex)
{
	mIsReadFromRecord = true;
	mLexer->beginReadTokenFromBuffer(bufferIndex);
}

// 结束从缓冲区回放解析

void  CParser::endParseFromBuffer()
{
	mLexer->endReadTokenFromRecord();
	mIsReadFromRecord = mLexer->isReadTokenFromRecord();
}




void  CParser::ParseTryCatch()
{

	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_TRY, -1, "expected 'try' to start try/catch statement");

	int startPC = mMidCode->GetCurCodeIndex();
	int blockIndex =  mSymbolTable->AddTryCatchBlock(mCurFuncIndex, mCurTryCatchBlockIndex);

	int lastCurTryCatchBlockIndex = mCurTryCatchBlockIndex;
	mCurTryCatchBlockIndex = blockIndex;

	int enterInstrIndex = mMidCode->AddInstr(INSTR_ENTERCATCH);

	mMidCode->AddIntOperand(enterInstrIndex, blockIndex);

	ParseBlock();

	int exitInstrIndex = mMidCode->AddInstr(INSTR_EXITCATCH);

	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_CATCH, -1, "try block must be followed by 'catch'");

	mLexer->ExpectToken(TOKEN_TYPE_IDENT, -1, "catch clause requires an error variable name after 'catch'");

	int catchPC = mMidCode->GetCurCodeIndex();

	 
	mSymbolTable->EnterBlock(mCurFuncIndex, (int)mMidCode->GetCurCodeIndex());

	int catchVar = mSymbolTable->AddVariant(mLexer->GetCurLexeme(), mCurFuncIndex, 0, IDENT_TYPE_VAR, mMidCode->GetCurCodeIndex());
	int movInstrIndex = mMidCode->AddInstr(INSTR_MOV);
	mMidCode->AddVarOperand(movInstrIndex, catchVar);
	mMidCode->AddRegOperand(movInstrIndex);

	ParseBlock();
	mSymbolTable->LeaveBlock(mCurFuncIndex, (int)mMidCode->GetCurCodeIndex());

	int finallyPC = -1;
	if (mLexer->LookNextToken() == TOKEN_TYPE_RSRVD_FINALLY)
	{
		mLexer->ExpectToken(TOKEN_TYPE_RSRVD_FINALLY, -1, "expected 'finally' before finally block");
		finallyPC = mMidCode->GetCurCodeIndex();
		ParseBlock();
	}

	int outpc = mMidCode->GetCurCodeIndex();

	TryCatchBlock& block = mSymbolTable->GetTryCatchBlock(mCurFuncIndex, blockIndex);
	block.blockIndex = blockIndex;
	block.funcIndex = mCurFuncIndex;
	block.startpc = startPC;
	block.catchpc = catchPC;
	block.finallypc = finallyPC;
	block.outpc = outpc;


	mCurTryCatchBlockIndex = lastCurTryCatchBlockIndex;
}

void  CParser::ParseThrow()
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_THROW, -1, "expected 'throw' to start throw statement");
	
	Factor factor = ParseExpr(false);

	int instrIndex = mMidCode->AddInstr(INSTR_THROW);
	AddOperandByFactor(instrIndex, factor);
	mLexer->TestToken(TOKEN_TYPE_DELIM_SEMICOLON);
	FreeFactorReg(factor);
}

void	CParser::ParseTableDeconstruct(std::vector<TableDeconstructData>& tableVec, std::vector<ListDeconstructData>& ListVec, TableDeconstructData& tableData, bool isVarInit)
{
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE, -1, "table destructuring pattern must start with '{'");
	while (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE)
	{
		mLexer->ExpectToken(TOKEN_TYPE_IDENT, -1, "table destructuring item requires a field name");

		TableVarPair tableVarPairData;
		tableVarPairData.ReferenceIndex = -1;
		tableVarPairData.DefaultAssignBufferIndex = -1;
		tableVarPairData.Key = mLexer->GetCurLexeme();
		tableVarPairData.VarName = tableVarPairData.Key;
		int nextToken = mLexer->LookNextToken();
		if (nextToken == TOKEN_TYPE_DELIM_COLON)
		{
			mLexer->GetNextToken();
			nextToken = mLexer->LookNextToken();
			if (nextToken == TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE)
			{
				TableDeconstructData newTableData;
				ParseTableDeconstruct(tableVec, ListVec, newTableData, isVarInit);
				
				tableVarPairData.ReferenceIndex = (int)tableVec.size();
				tableVarPairData.IsList = false;
				tableVec.push_back(newTableData);
			}
			else if(nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE)
			{
				ListDeconstructData newListData;
				ParseListDeconstruct(tableVec, ListVec, newListData, isVarInit);

				tableVarPairData.ReferenceIndex = (int)ListVec.size();
				tableVarPairData.IsList = true;
				ListVec.push_back(newListData);
			}
			else if (nextToken == TOKEN_TYPE_IDENT)
			{
				mLexer->GetNextToken();
				tableVarPairData.VarName = mLexer->GetCurLexeme();
			}
			else
			{
				Error("Deconstruct assignment format error, expect varname, [], {}");
			}
		}

		if (isVarInit && tableVarPairData.ReferenceIndex < 0)
		{
			ValidateUserIdentName(tableVarPairData.VarName.c_str());
			mSymbolTable->AddVariant(tableVarPairData.VarName.c_str(), mCurFuncIndex, 1, IDENT_TYPE_VAR, mMidCode->GetCurCodeIndex());
		}

		nextToken = mLexer->LookNextToken();
		if (nextToken == TOKEN_TYPE_OP && mLexer->GetCurOprType() == OP_TYPE_ASSIGN)
		{
			mLexer->GetNextToken();

			tableVarPairData.DefaultAssignBufferIndex = beginRecordTokens();
			Factor factor = ParseExpr(false);
			endRecordTokens();
			FreeFactorReg(factor);
			nextToken = mLexer->LookNextToken();
		}

		tableData.keyVarPairVec.push_back(tableVarPairData);
		
		if (nextToken == TOKEN_TYPE_DELIM_COMMA)
		{
			mLexer->GetNextToken();
		}
		else if (nextToken != TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE)
		{
			Error("table destructuring items must be separated by ',' or closed with '}'");
		}
		
	}

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE, -1, "table destructuring pattern must be closed with '}'");

}


void	CParser::ParseListDeconstruct(std::vector<TableDeconstructData>& tableVec, std::vector<ListDeconstructData>& ListVec, ListDeconstructData& listData, bool isVarInit)
{
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_BRACE, -1, "list destructuring pattern must start with '['");

	int listIndex = 0;
	while (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_CLOSE_BRACE)
	{

		ListVarPair listVarPairData;
		listVarPairData.ReferenceIndex = -1;
		listVarPairData.DefaultAssignBufferIndex = -1;
		listVarPairData.varIndex = listIndex;
		bool skip = false;

		int nextToken = mLexer->LookNextToken();
		if (nextToken == TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE)
		{
			TableDeconstructData newTableData;
			ParseTableDeconstruct(tableVec, ListVec, newTableData, isVarInit);

			listVarPairData.ReferenceIndex = (int)tableVec.size();
			listVarPairData.IsList = false;
			tableVec.push_back(newTableData);
		}
		else if (nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE)
		{
			ListDeconstructData newListData;
			ParseListDeconstruct(tableVec, ListVec, newListData, isVarInit);

			listVarPairData.ReferenceIndex = (int)ListVec.size();
			listVarPairData.IsList = true;
			ListVec.push_back(newListData);
		}
		else if (nextToken == TOKEN_TYPE_IDENT)
		{
			mLexer->GetNextToken();
			listVarPairData.VarName = mLexer->GetCurLexeme();
		}
		else if(nextToken == TOKEN_TYPE_DELIM_COMMA)
		{
			skip = true;
			// ","跳过
		}

		if (isVarInit && listVarPairData.ReferenceIndex < 0 && !skip)
		{
			ValidateUserIdentName(listVarPairData.VarName.c_str());
			mSymbolTable->AddVariant(listVarPairData.VarName.c_str(), mCurFuncIndex, 1, IDENT_TYPE_VAR, mMidCode->GetCurCodeIndex());
		}

		nextToken = mLexer->LookNextToken();
		if (!skip)
		{
			if (nextToken == TOKEN_TYPE_OP && mLexer->GetCurOprType() == OP_TYPE_ASSIGN)
			{
				mLexer->GetNextToken();

				listVarPairData.DefaultAssignBufferIndex = beginRecordTokens();
				Factor factor = ParseExpr(false);
				FreeFactorReg(factor);
				endRecordTokens();
				nextToken = mLexer->LookNextToken();
			}

			listData.listPairVec.push_back(listVarPairData);
		}
		listIndex++;
		
		if (nextToken == TOKEN_TYPE_DELIM_COMMA)
		{
			mLexer->GetNextToken();
		}
		else if (nextToken != TOKEN_TYPE_DELIM_CLOSE_BRACE)
		{
			Error("list destructuring items must be separated by ',' or closed with ']'");
		}
	}

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_BRACE, -1, "list destructuring pattern must be closed with ']'");
}

void	CParser::EmitDefaultAssignIfNil(int varIndex, int defaultAssignBufferIndex)
{
	if (defaultAssignBufferIndex < 0)
	{
		return;
	}

	int jumpIndex = mMidCode->GetNextJumpIndex();
	int instrIndex = mMidCode->AddInstr(INSTR_JNE);
	mMidCode->AddVarOperand(instrIndex, varIndex);
	mMidCode->AddNilOperand(instrIndex);
	mMidCode->AddJumpIndexOperand(instrIndex, jumpIndex);

	beginParseFromBuffer(defaultAssignBufferIndex);

	Factor factor = ParseExpr(false);
	instrIndex = mMidCode->AddInstr(INSTR_MOV);
	mMidCode->AddVarOperand(instrIndex, varIndex);
	AddOperandByFactor(instrIndex, factor);
	FreeFactorReg(factor);

	endParseFromBuffer();
	mMidCode->AddJumpTarget(jumpIndex);
}

void	CParser::TableDeconstructAssign(const std::vector<TableDeconstructData>& tableVec, const std::vector<ListDeconstructData>& ListVec, const TableDeconstructData& tableData, int assignVarIndex)
{
	for (const auto& keyVarPair: tableData.keyVarPairVec)
	{
		if (keyVarPair.ReferenceIndex >= 0)
		{
			int freeRegIndex = GET_FREE_REG();
			int instrIndex = mMidCode->AddInstr(INSTR_MOV);
			mMidCode->AddVarOperand(instrIndex, freeRegIndex);
			mMidCode->AddTableStringOperand(instrIndex, assignVarIndex, mSymbolTable->AddString(keyVarPair.Key.c_str()));
			EmitDefaultAssignIfNil(freeRegIndex, keyVarPair.DefaultAssignBufferIndex);
			if (keyVarPair.IsList)
			{
				ListDeconstructAssign(tableVec, ListVec, ListVec[keyVarPair.ReferenceIndex], freeRegIndex);
			}
			else
			{
				TableDeconstructAssign(tableVec, ListVec, tableVec[keyVarPair.ReferenceIndex], freeRegIndex);
			}
			FREE_REG(freeRegIndex);

		}
		else
		{
			int varIndex = mSymbolTable->SearchValue(keyVarPair.VarName.c_str(), mCurFuncIndex);
			int instrIndex = mMidCode->AddInstr(INSTR_MOV);
			mMidCode->AddVarOperand(instrIndex, varIndex);
			mMidCode->AddTableStringOperand(instrIndex, assignVarIndex, mSymbolTable->AddString(keyVarPair.Key.c_str()));
			EmitDefaultAssignIfNil(varIndex, keyVarPair.DefaultAssignBufferIndex);
		}
	}
}

void	CParser::ListDeconstructAssign(const std::vector<TableDeconstructData>& tableVec, const std::vector<ListDeconstructData>& ListVec, const ListDeconstructData& listData, int assignVarIndex)
{
	for (const auto& listPair : listData.listPairVec)
	{
		if (listPair.ReferenceIndex >= 0)
		{
			int freeRegIndex = GET_FREE_REG();
			int instrIndex = mMidCode->AddInstr(INSTR_MOV);
			mMidCode->AddVarOperand(instrIndex, freeRegIndex);
			mMidCode->AddTableIntOperand(instrIndex, assignVarIndex, listPair.varIndex);
			EmitDefaultAssignIfNil(freeRegIndex, listPair.DefaultAssignBufferIndex);
		
			if (listPair.IsList)
			{
				ListDeconstructAssign(tableVec, ListVec, ListVec[listPair.ReferenceIndex], freeRegIndex);
			}
			else
			{
				TableDeconstructAssign(tableVec, ListVec, tableVec[listPair.ReferenceIndex], freeRegIndex);
			}
			FREE_REG(freeRegIndex);
		}
		else
		{
			int varIndex = mSymbolTable->SearchValue(listPair.VarName.c_str(), mCurFuncIndex);
			int instrIndex = mMidCode->AddInstr(INSTR_MOV);
			mMidCode->AddVarOperand(instrIndex, varIndex);
			mMidCode->AddTableIntOperand(instrIndex, assignVarIndex, listPair.varIndex);
			EmitDefaultAssignIfNil(varIndex, listPair.DefaultAssignBufferIndex);

		}
	}
}

std::string CParser::GetNextAnonymousFuncName()
{
	static int sNumAnonymousFunc = 0;
	char szFuncName[128] = { 0 };
	snprintf(szFuncName, 128, "%sanonymous_func_%d", XSCRIPT_INTERNAL_IDENT_PREFIX, sNumAnonymousFunc);
	sNumAnonymousFunc++;
	return szFuncName;
}

std::string	CParser::GetNextLamdaFuncName()
{
	static int sNumLambdaFunc = 0;
	char szFuncName[128] = { 0 };
	snprintf(szFuncName, 128, "%slambda_func_%d", XSCRIPT_INTERNAL_IDENT_PREFIX, sNumLambdaFunc);
	sNumLambdaFunc++;
	return szFuncName;
}


std::string	CParser::GetNextDeforFuncName()
{
	static int sNumDeferFunc = 0;
	char szFuncName[128] = { 0 };
	snprintf(szFuncName, 128, "%sdefer_func_%d", XSCRIPT_INTERNAL_IDENT_PREFIX, sNumDeferFunc);
	sNumDeferFunc++;
	return szFuncName;


}

void  CParser::ParseDefer()
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_DEFER, -1, "expected 'defer' to start defer statement");

	int funcDefLine = mLexer->GetLine() + 1; // 记录lambda关键字所在行号
	std::string funcName = GetNextDeforFuncName();

	int iFuncIndex = mSymbolTable->AddFunction(funcName.c_str(), 0, mCurFuncIndex);
	mSymbolTable->AddSubFunction(mCurFuncIndex, iFuncIndex);
	mSymbolTable->SetFunctionParamNum(iFuncIndex, 0);

	static int sDeferIndex = 0;
	char regName[64] = { 0 };
	snprintf(regName, 64, "%sDefer_%d", XSCRIPT_INTERNAL_IDENT_PREFIX, sDeferIndex);
	sDeferIndex++;
	int deferFunctionVarIndex = mSymbolTable->AddVariant(regName, mRegAllocFuncIndex, 0, IDENT_TYPE_INTERNAL_VAR);

	int lastFuncIndex = mCurFuncIndex;
	RegAllocContext lastRegContext = SaveRegAllocContext();
	mCurFuncIndex = iFuncIndex;
	mMidCode->setCurFuncIndex(iFuncIndex);
	InitRegAllocContextForFunction(iFuncIndex);

	ParseStateMent();

	int instrIndex = mMidCode->AddInstr(INSTR_RET);
	mMidCode->AddIntOperand(instrIndex, 0);
	RestoreRegAllocContext(lastRegContext);
	EmitFunctionClosure(lastFuncIndex, Factor(), Factor(), deferFunctionVarIndex, funcDefLine);

	
	instrIndex = mMidCode->AddInstr(INSTR_DEFER);
	mMidCode->AddVarOperand(instrIndex, deferFunctionVarIndex);
}


static void replace_all(std::string& str, const std::string& from, const std::string& to) 
{
	if (from.empty()) 
		return;
	size_t pos = 0;
	while ((pos = str.find(from, pos)) != std::string::npos)
	{
		str.replace(pos, from.length(), to);
		pos += to.length(); // 关键：跳过已替换部分，防止重入
	}
}


void  CParser::ParseFstring(Factor& resultFactor)
{
	std::string fStr = mLexer->GetCurLexeme();
	size_t strSize = fStr.size();

	std::vector<Factor> fstrFactorVec;
	size_t startPos = 0;
	size_t strStartPos = 0;

	std::string curcollectStr;
	bool hasFactor = false;
	while (startPos < strSize)
	{
		size_t nexpos = fStr.find_first_of('{', startPos);
		if (nexpos != string::npos)
		{
			if (nexpos + 1 < strSize && fStr[nexpos + 1] == '{') //{{
			{
				curcollectStr = fStr.substr(strStartPos, nexpos - strStartPos + 1);
				strStartPos = nexpos + 2;
				startPos = nexpos + 2;
			}
			else //{
			{
				curcollectStr = curcollectStr + fStr.substr(strStartPos, nexpos - strStartPos);
				if (curcollectStr.size() > 0)
				{
					Factor strFactor;
					strFactor.type = EFactorType::FACTOR_STRING;
					strFactor.strIndex = mSymbolTable->AddString(curcollectStr.c_str());
					fstrFactorVec.push_back(strFactor);
				}
				
				curcollectStr.clear();


				int factorEndIndex = -1;

				//递归解析factor, 需要把 "{{" 退化为"{", 处理嵌套
				std::string factorStr = fStr.substr(nexpos + 1);
				//使用解析出来的字符串重新建立词法解析器
				CSourceFile sourceFile;
				sourceFile.LoadFromString(factorStr);

				CLexer lex(&sourceFile);

				//如果是空token流就无需处理了
				int nexttoken = lex.LookNextToken();
				if (nexttoken != TOKEN_TYPE_END_OF_STREAM && nexttoken != TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE)
				{
					int expConsumeLen = 0;
					PendingFunctionReturnFactorScope pendingScope(this, fstrFactorVec);
					CLexer* lastLex = mLexer;
					mLexer = &lex;
					Factor factor = ParseExpr(false);
					mLexer = lastLex;

					
					hasFactor = true;
					expConsumeLen = sourceFile.GetCurReadedLen();
					factorEndIndex = (int)fStr.find_first_of('}', nexpos + 1 + expConsumeLen);

					bool useFactor = false; 
					//如果下一个token不是'}'， 说明语法有问题， 我们直接退化为字符串
					if (factorEndIndex >= 0 && lex.LookNextToken() == TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE)
					{
						fstrFactorVec.push_back(factor);
						useFactor = true;
					}
					else if(factorEndIndex >= 0) //如果搜索到'{', 但是语法有问题， 直接退化为字符串
					{
						std::string exprStr =  fStr.substr(nexpos + 1, factorEndIndex - nexpos - 1);
						Factor strFactor;
						strFactor.type = EFactorType::FACTOR_STRING;
						strFactor.strIndex = mSymbolTable->AddString(exprStr.c_str());
						fstrFactorVec.push_back(strFactor);
					}
					if (!useFactor)
					{
						FreeFactorReg(factor);
					}
				}
				else if(nexttoken == TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE) //这表示是一个空括号
				{
					hasFactor = true;
					factorEndIndex = (int)fStr.find_first_of('}', nexpos + 1);
				}

				//找不到匹配的'}', 直接全部退化为字符串
				if(factorEndIndex < 0 )
				{
					curcollectStr = fStr.substr(nexpos);

					Factor strFactor;
					strFactor.type = EFactorType::FACTOR_STRING;
					strFactor.strIndex = mSymbolTable->AddString(curcollectStr.c_str());
					fstrFactorVec.push_back(strFactor);
					break;
				}
				else
				{
					startPos = factorEndIndex + 1;
					strStartPos = factorEndIndex + 1;
				}
			}
			
		}
		else//没有找到新的{， 处理最后的字符串 
		{
			curcollectStr = curcollectStr + fStr.substr(strStartPos);
			if (curcollectStr.size() > 0)
			{
				Factor strFactor;
				strFactor.type = EFactorType::FACTOR_STRING;
				strFactor.strIndex = mSymbolTable->AddString(curcollectStr.c_str());
				fstrFactorVec.push_back(strFactor);
			}
			break;
		}
	}

	if (fstrFactorVec.size() > 0 && hasFactor)
	{
		resultFactor = fstrFactorVec[0];

		for (size_t i = 1; i < fstrFactorVec.size(); i++)
		{
			int freeRegIndex = GET_FREE_REG();
			int instrIndex = mMidCode->AddInstr(INSTR_CONCAT_TO);
			mMidCode->AddVarOperand(instrIndex, freeRegIndex);
			AddOperandByFactor(instrIndex, resultFactor);
			AddOperandByFactor(instrIndex, fstrFactorVec[i]);

			FreeFactorReg(resultFactor);
			FreeFactorReg(fstrFactorVec[i]);

			resultFactor.type = FACTOR_VAR;
			resultFactor.varIndex = freeRegIndex;

		}
	}
	else //还需要处理空串
	{
		resultFactor.type = FACTOR_STRING;
		resultFactor.strIndex = mSymbolTable->AddString(fStr.c_str());
	}
	
}

void  CParser::ParseSwitchCase()
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_SWITCH, -1, "expected 'switch' to start switch statement");

	Factor switchFactor = ParseExpr( false );
	int switchOutJumpindex = mMidCode->GetNextJumpIndex();
	int blockIndex = mSymbolTable->AddSwitchCase(mCurFuncIndex);
	SwitchStruct& switchData = mSymbolTable->GetSwitchStruct(mCurFuncIndex, blockIndex);
	int iInstrIndex = mMidCode->AddInstr(INSTR_SWITCH);
	AddOperandByFactor(iInstrIndex, switchFactor);
	mMidCode->AddIntOperand(iInstrIndex, blockIndex);
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE, -1, "switch body must start with '{' after switch condition");

	iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
	mMidCode->AddJumpIndexOperand(iInstrIndex, switchOutJumpindex);
	

	mLoopStack.push(LoopStruct(switchOutJumpindex, switchOutJumpindex));


	bool hasDefault = false;
	std::vector<XInt> lastIntCaseValueVec;
	std::vector<XFloat> lastFloatCaseValueVec;
	std::vector<std::string> lastStringCaseValueVec;
	std::map<XInt, bool> intValueMap;
	std::map<XFloat, bool> floatValueMap;
	std::map<std::string, bool> stringValueMap;
	while (true)
	{
		int nextToken = mLexer->LookNextToken();
		
		if (nextToken == TOKEN_TYPE_RSRVD_CASE)
		{
			
			mLexer->GetNextToken();
			nextToken = mLexer->GetNextToken();
			if (nextToken == TOKEN_TYPE_INT)
			{
				XInt intValue = mLexer->GetCurIntValue();
				if (intValueMap.count(intValue) > 0)
				{
					Error("switch case value '%d' dupplicated", (int)intValue);
				}
				intValueMap[intValue] = true;
				lastIntCaseValueVec.push_back(intValue);

			}
			else if (nextToken == TOKEN_TYPE_FLOAT)
			{
				XFloat floatValue = StrToXFloat(mLexer->GetCurLexeme());
				if (floatValueMap.count(floatValue) > 0)
				{
					Error("switch case value '%s' dupplicated", mLexer->GetCurLexeme());
				}
				floatValueMap[floatValue] = true;

				lastFloatCaseValueVec.push_back(floatValue);
			}
			else if (nextToken == TOKEN_TYPE_STRING)
			{
				std::string strValue = mLexer->GetCurLexeme();
				if (stringValueMap.count(strValue) > 0)
				{
					Error("switch case value '%s' dupplicated", strValue.c_str());
				}
				stringValueMap[strValue] = true;

				lastStringCaseValueVec.push_back(mLexer->GetCurLexeme());
			}
			else
			{
				Error("switch case value can only be int, float or string");
			}
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_COLON, -1, "case (expr) should follow by ':' ");
			nextToken = mLexer->LookNextToken();

			if (nextToken == TOKEN_TYPE_RSRVD_CASE)
			{
				//如果是case连着case
			}
			else
			{
				for (auto& d : lastIntCaseValueVec)
				{
					switchData.intValueInstrMap[d] = mMidCode->GetCurCodeIndex();
				}

				for (auto& d : lastStringCaseValueVec)
				{
					switchData.stringValueInstrMap[d] = mMidCode->GetCurCodeIndex();
				}

				for (auto& d : lastFloatCaseValueVec)
				{
					switchData.floatValueInstrMap[d] = mMidCode->GetCurCodeIndex();
				}

				mSymbolTable->EnterBlock(mCurFuncIndex, mMidCode->GetCurCodeIndex());
				while(nextToken != TOKEN_TYPE_RSRVD_CASE
					&& nextToken != TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE
					&& nextToken != TOKEN_TYPE_RSRVD_DEFAULT)
				{ 
					ParseStateMent();
					nextToken = mLexer->LookNextToken();
				}
				mSymbolTable->LeaveBlock(mCurFuncIndex, mMidCode->GetCurCodeIndex());
				lastIntCaseValueVec.clear();
				lastStringCaseValueVec.clear();
				lastFloatCaseValueVec.clear();
			}
			

		}
		else if(nextToken == TOKEN_TYPE_RSRVD_DEFAULT)
		{
			mLexer->GetNextToken();
			if (hasDefault)
			{
				Error("switch already have default branch");
			}

			mLexer->ExpectToken(TOKEN_TYPE_DELIM_COLON, -1, "default should follow by ':' ");

			hasDefault = true;

			switchData.defaultInstr = mMidCode->GetCurCodeIndex();
			mSymbolTable->EnterBlock(mCurFuncIndex, mMidCode->GetCurCodeIndex());
			nextToken = mLexer->LookNextToken();
			while (nextToken != TOKEN_TYPE_RSRVD_CASE
				&& nextToken != TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE
				&& nextToken != TOKEN_TYPE_RSRVD_DEFAULT)
			{
				ParseStateMent();
				nextToken = mLexer->LookNextToken();
			}
			mSymbolTable->LeaveBlock(mCurFuncIndex, mMidCode->GetCurCodeIndex());
		}
		//遇到'}', switch语句终结
		else if (nextToken == TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE)
		{
			mLexer->GetNextToken();
			break;
		}
		else
		{
			Error("switch branch should start with 'case', 'default'");
		}
	}

	mMidCode->AddJumpTarget(switchOutJumpindex);
	mLoopStack.pop();

}

void  CParser::ParseMatch()
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_MATCH);
	Factor factor = ParseExpr(false);
	PendingFunctionReturnSingleFactorScope pendingSubjectScope(this, factor);

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE, -1, "match expression must be followed by '{'");

	int matchOutJumpIndex = mMidCode->GetNextJumpIndex();
	while (true)
	{
		int nextToken = mLexer->LookNextToken();
		if(nextToken == TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE)
		{
			mLexer->GetNextToken();
			break;
		}
		else //尝试匹配 case
		{
			ParseMatchCase(factor, matchOutJumpIndex);
		}
	}
	FreeFactorReg(factor);
	mMidCode->AddJumpTarget(matchOutJumpIndex);
}

void  CParser::ParseMatchCase(Factor& matchFactor, int matchOutJumpIndex)
{
	mSymbolTable->EnterBlock(mCurFuncIndex, mMidCode->GetCurCodeIndex());
	int iInstrIndex = -1;
	int jumpOutIndex = mMidCode->GetNextJumpIndex();
	int jumpInIndex = mMidCode->GetNextJumpIndex();
	int nextCaseJumpIndex = -1;

	bool isConstant = false;
	std::vector<int> bindVarVec;
	bool hasUnderscore = false;
	while (true)
	{
		int nextToken = mLexer->LookNextToken();

		if (nextCaseJumpIndex >= 0)
		{
			mMidCode->AddJumpTarget(nextCaseJumpIndex);
			nextCaseJumpIndex = -1;
		}

		if (nextToken == TOKEN_TYPE_INT || nextToken == TOKEN_TYPE_FLOAT || nextToken == TOKEN_TYPE_STRING)
		{
			if (bindVarVec.size() > 0)
			{
				Error("match branch cannot mix constant patterns with binding patterns");
			}
			if (hasUnderscore)
			{
				Error("match branch cannot mix '_' wildcard with constant patterns");
			}
			isConstant = true;
			mLexer->GetNextToken();
			iInstrIndex = mMidCode->AddInstr(INSTR_JE);
			AddOperandByFactor(iInstrIndex, matchFactor);
			if (nextToken == TOKEN_TYPE_FLOAT)
			{
				mMidCode->AddFloatOperand(iInstrIndex, StrToXFloat(mLexer->GetCurLexeme()));
			}
			else if (nextToken == TOKEN_TYPE_INT)
			{
				mMidCode->AddIntOperand(iInstrIndex, mLexer->GetCurIntValue());
			}
			else
			{
				mMidCode->AddStringIndexOperand(iInstrIndex,mSymbolTable->AddString(mLexer->GetCurLexeme()));
			}
			
			mMidCode->AddJumpIndexOperand(iInstrIndex, jumpInIndex);
		}
		else if (nextToken == TOKEN_TYPE_IDENT)  //匹配了 n if n > 10
		{	
			if (isConstant)
			{
				Error("match branch cannot mix binding patterns with constant patterns");
			}
			if (hasUnderscore)
			{
				Error("match branch cannot mix binding patterns with '_' wildcard");
			}
			mLexer->GetNextToken();

			int caseVarIndex = mSymbolTable->AddVariant(mLexer->GetCurLexeme(), mCurFuncIndex, 1, IDENT_TYPE_VAR, mMidCode->GetCurCodeIndex());

			//如果有变量 保证变量名和数量要一致
			if (bindVarVec.size() > 0)
			{
				if (bindVarVec.size() != 1 || bindVarVec[0] != caseVarIndex)
				{
					Error("all binding patterns in the same match branch must bind the same variable names in the same order");
				}
			}
			else
			{
				bindVarVec.push_back(caseVarIndex);
			}
			//解析变量类型限制
			ParseMatchType(nextCaseJumpIndex, matchFactor);
			
			iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
			mMidCode->AddVarOperand(iInstrIndex, caseVarIndex );
			AddOperandByFactor(iInstrIndex, matchFactor);

			ParseMatchIf(jumpInIndex);
		}
		else if (nextToken == TOKEN_TYPE_DELM_UNDERSCORE)
		{
			mLexer->GetNextToken();
			if (isConstant || bindVarVec.size() > 0)
			{
				Error("match branch cannot mix '_' wildcard with constant or binding patterns");
			}
			hasUnderscore = true;

			//解析变量类型限制
			ParseMatchType(nextCaseJumpIndex, matchFactor);

			int iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
			mMidCode->AddJumpIndexOperand(iInstrIndex, jumpInIndex);

		}
		else if (nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE
			|| nextToken == TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE)
		{
			if (hasUnderscore)
			{
				Error("match branch cannot mix structure patterns with '_' wildcard");
			}

			nextCaseJumpIndex = mMidCode->GetNextJumpIndex();
			EnsureFactorIsVar(matchFactor);

			iInstrIndex = mMidCode->AddInstr(INSTR_J_NOT_TYPE);
			AddOperandByFactor(iInstrIndex, matchFactor);
			mMidCode->AddIntOperand(iInstrIndex, GetValueTypeMask(nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE ? ValueType::OP_TYPE_LIST : ValueType::OP_TYPE_TABLE)  );
			mMidCode->AddJumpIndexOperand(iInstrIndex, nextCaseJumpIndex);

			std::vector<int> newBindVarVec;
			if (nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE)
				ParseMatchList(matchFactor.varIndex, nextCaseJumpIndex, newBindVarVec);
			else
				ParseMatchTable(matchFactor.varIndex, nextCaseJumpIndex, newBindVarVec);



			if (newBindVarVec.size() > 0 && isConstant)
			{
				Error("match branch cannot mix structure patterns that bind variables with constant-only patterns");
			}
			else if (newBindVarVec.size() == 0)
			{
				isConstant = true;
			}

			if (bindVarVec.size() > 0)
			{
				if (newBindVarVec.size() == 0)
				{
					Error("match branch cannot mix binding patterns with constant-only patterns");
				}
				else if (newBindVarVec.size() != bindVarVec.size())
				{
					Error("all binding patterns in the same match branch must bind the same number of variables");
				}
				else
				{
					for (size_t i = 0; i < bindVarVec.size(); i++)
					{
						if (bindVarVec[i] != newBindVarVec[i])
						{
							Error("all binding patterns in the same match branch must bind the same variable names in the same order");
						}
					}
				}
			}
			else
			{
				bindVarVec = newBindVarVec;
			}


			ParseMatchIf(jumpInIndex);
		}
		else
		{
			Error("invalid match pattern: expected a constant, binding name, '_', list pattern '[...]' or table pattern '{...}'");
		}

		if (mLexer->TestToken(TOKEN_TYPE_DELM_FATARROW))
		{
			//如果前面直接跳到jumpInIndex的条件不满足，说明都无法匹配， 跳转到下一个匹配case
			iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
			mMidCode->AddJumpIndexOperand(iInstrIndex, jumpOutIndex);

			mMidCode->AddJumpTarget(jumpInIndex);
			ParseStateMent();

			//匹配成功， 跳转到match 语句外层
			iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
			mMidCode->AddJumpIndexOperand(iInstrIndex, matchOutJumpIndex);

			break;
		}
		else
		{	
			//如果没有','， 那就表示条件结束， 需要出现 =>
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_COMMA, -1, "match branch patterns must be separated by ',' or followed by '=>' before the branch body");
		}
	}

	if (nextCaseJumpIndex >= 0)
	{
		mMidCode->AddJumpTarget(nextCaseJumpIndex);
		nextCaseJumpIndex = -1;
	}

	mMidCode->AddJumpTarget(jumpOutIndex);

	mSymbolTable->LeaveBlock(mCurFuncIndex, mMidCode->GetCurCodeIndex());
}


void  CParser::ParseMatchIf(int jumpInIndex)
{
	//如果后面跟着if, 先判定条件，否则直接命中
	if (mLexer->TestToken(TOKEN_TYPE_RSRVD_IF))
	{
		mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN, -1, "if condition must start with '('");
		Factor ifRet = ParseExpr(false);
		mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN, -1, "if condition must be closed with ')'");

		int iInstrIndex = mMidCode->AddInstr(INSTR_JTRUE);
		AddOperandByFactor(iInstrIndex, ifRet);
		mMidCode->AddJumpIndexOperand(iInstrIndex, jumpInIndex);

		FreeFactorReg(ifRet);
	}
	else
	{
		int iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
		mMidCode->AddJumpIndexOperand(iInstrIndex, jumpInIndex);
	}
}

void  CParser::ParseMatchType(int& nextCaseJumpIndex, const Factor& factor )
{
	if (mLexer->TestToken(TOKEN_TYPE_DELIM_OPEN_PAREN))
	{
		if (nextCaseJumpIndex < 0)
		{
			nextCaseJumpIndex = mMidCode->GetNextJumpIndex();
		}

		int valueMask = 0;
		while (true)
		{
			mLexer->ExpectToken(TOKEN_TYPE_IDENT, -1, "match type limit requires a type name such as number, int, float or string");
			std::string varType = mLexer->GetCurLexeme();
			int newValueMask = GetValueMaskByName(varType);
			if (newValueMask == 0)
			{
				Error("unknown match type limit: expected one of number, nil, int, float, string, table, list, func, function, lightuserdata, userdata, proto, upval, thread or future");
			}

			valueMask |= newValueMask;

			int nextToken = mLexer->LookNextToken();
			if (nextToken == (TOKEN_TYPE_DELIM_CLOSE_PAREN))
			{
				mLexer->GetNextToken();
				break;
			}
			else 
			{
				mLexer->ExpectToken(TOKEN_TYPE_DELIM_COMMA, -1, "match type limit entries must be separated by ',' or closed with ')'");
			}
		}

		int iInstrIndex = mMidCode->AddInstr(INSTR_J_NOT_TYPE);
		AddOperandByFactor(iInstrIndex, factor);
		mMidCode->AddIntOperand(iInstrIndex, valueMask);
		mMidCode->AddJumpIndexOperand(iInstrIndex, nextCaseJumpIndex);

	}
}

static CParser::Factor  ConsturctFactorByTableAndString(int tableVarIndex, int stringIndex)
{
	CParser::Factor factor;
	factor.type = CParser::FACTOR_TABLE;
	factor.varIndex = tableVarIndex;
	factor.iOffset = ROT_String;
	factor.intTableValue = stringIndex;
	return factor;
}

static CParser::Factor  ConsturctFactorByTableAndInt(int tableVarIndex, int tableIndex)
{
	CParser::Factor factor;
	factor.type = CParser::FACTOR_TABLE;
	factor.varIndex = tableVarIndex;
	factor.iOffset = ROT_Int;
	factor.intTableValue = tableIndex;
	return factor;
}

bool  CParser::ParseMatchList(int tableVarIndex, int jumpOutIndex, std::vector<int>& bindVarVec)
{
	bool hasCreateVar = false;
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_BRACE);

	int listIndex = 0;
	while (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_CLOSE_BRACE)
	{
		int nextToken = mLexer->LookNextToken();
		if (nextToken == TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE
			|| nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE)
		{
			//先判定类型， 类型不对，直接跳出， 否则就会运行时错误
			int iInstrIndex = mMidCode->AddInstr(INSTR_J_NOT_TYPE);
			mMidCode->AddTableIntOperand(iInstrIndex, tableVarIndex, listIndex);
			if (nextToken == TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE)
				mMidCode->AddIntOperand(iInstrIndex, GetValueTypeMask(ValueType::OP_TYPE_TABLE));
			else
				mMidCode->AddIntOperand(iInstrIndex, GetValueTypeMask(ValueType::OP_TYPE_LIST));
			mMidCode->AddJumpIndexOperand(iInstrIndex, jumpOutIndex);


			int freeRegIndex = GET_FREE_REG();
			iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
			mMidCode->AddVarOperand(iInstrIndex, freeRegIndex);
			mMidCode->AddTableIntOperand(iInstrIndex, tableVarIndex, listIndex);
			bool hasSubCreateVar = false;
			if (nextToken == TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE)
				hasSubCreateVar = ParseMatchTable(freeRegIndex, jumpOutIndex, bindVarVec);
			else
				hasSubCreateVar = ParseMatchList(freeRegIndex, jumpOutIndex, bindVarVec);
			FREE_REG(freeRegIndex);
			if (hasSubCreateVar)
				hasCreateVar = true;
		}
		else if (nextToken == TOKEN_TYPE_IDENT)
		{
			mLexer->GetNextToken();

			std::string VarName = mLexer->GetCurLexeme();
			int varIndex = mSymbolTable->AddVariant(VarName.c_str(), mCurFuncIndex, 1, IDENT_TYPE_VAR, mMidCode->GetCurCodeIndex());

			//解析变量类型限制
			ParseMatchType(jumpOutIndex, ConsturctFactorByTableAndInt(tableVarIndex, listIndex));

			int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
			mMidCode->AddVarOperand(iInstrIndex, varIndex);
			mMidCode->AddTableIntOperand(iInstrIndex, tableVarIndex, listIndex);
			hasCreateVar = true;

			bindVarVec.push_back(varIndex);

		}
		else if (nextToken == TOKEN_TYPE_INT || nextToken == TOKEN_TYPE_FLOAT || nextToken == TOKEN_TYPE_STRING)
		{
			mLexer->GetNextToken();
			//如果是常量需要进行比较
			int iInstrIndex = mMidCode->AddInstr(INSTR_JNE);
			mMidCode->AddTableIntOperand(iInstrIndex, tableVarIndex, listIndex);
			if (nextToken == TOKEN_TYPE_INT)
			{
				mMidCode->AddIntOperand(iInstrIndex, mLexer->GetCurIntValue());
			}
			else if (nextToken == TOKEN_TYPE_FLOAT)
			{
				mMidCode->AddFloatOperand(iInstrIndex, StrToXFloat(mLexer->GetCurLexeme()));
			}
			else
			{
				mMidCode->AddStringIndexOperand(iInstrIndex, mSymbolTable->AddString(mLexer->GetCurLexeme()));
			}

			mMidCode->AddJumpIndexOperand(iInstrIndex, jumpOutIndex);
		}
		else if (nextToken == TOKEN_TYPE_DELM_UNDERSCORE)
		{
			//list 中的'_' 匹配 非空元素
			mLexer->GetNextToken();
			int iInstrIndex = mMidCode->AddInstr(INSTR_JE);
			mMidCode->AddTableIntOperand(iInstrIndex, tableVarIndex, listIndex);
			mMidCode->AddNilOperand(iInstrIndex);
			mMidCode->AddJumpIndexOperand(iInstrIndex, jumpOutIndex);

		}
		else if (nextToken == TOKEN_TYPE_DELIM_COMMA)
		{
			// ","跳过
		}

		listIndex++;
		nextToken = mLexer->LookNextToken();
		if (nextToken == TOKEN_TYPE_DELIM_COMMA)
		{
			mLexer->GetNextToken();
		}
		else if (nextToken != TOKEN_TYPE_DELIM_CLOSE_BRACE)
		{
			Error("match list pattern items must be separated by ',' or closed with ']'");
		}
	}

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_BRACE, -1, "match list pattern must be closed with ']'");
	return hasCreateVar;
}

bool  CParser::ParseMatchTable(int tableVarIndex, int jumpOutIndex, std::vector<int>& bindVarVec)
{
	bool hasCreateVar = false;
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE);
	while (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE)
	{
		mLexer->ExpectToken(TOKEN_TYPE_IDENT, -1, "match table pattern field requires a field name");
		std::string KeyName = mLexer->GetCurLexeme();
		int nextToken = mLexer->LookNextToken();
		if (nextToken == TOKEN_TYPE_DELIM_COLON)
		{
			mLexer->GetNextToken();
			nextToken = mLexer->LookNextToken();
			if (nextToken == TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE || nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE)
			{
				//先判定类型， 类型不对，直接跳出， 否则就会运行时错误
				int keyStrIndex = mSymbolTable->AddString(KeyName.c_str());
				int iInstrIndex = mMidCode->AddInstr(INSTR_J_NOT_TYPE);
				mMidCode->AddTableStringOperand(iInstrIndex, tableVarIndex, keyStrIndex);
				if (nextToken == TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE)
					mMidCode->AddIntOperand(iInstrIndex, GetValueTypeMask(ValueType::OP_TYPE_TABLE));
				else
					mMidCode->AddIntOperand(iInstrIndex, GetValueTypeMask(ValueType::OP_TYPE_LIST));
				mMidCode->AddJumpIndexOperand(iInstrIndex, jumpOutIndex);

				int freeRegIndex = GET_FREE_REG();
				iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
				mMidCode->AddVarOperand(iInstrIndex, freeRegIndex);
				mMidCode->AddTableStringOperand(iInstrIndex, tableVarIndex, keyStrIndex);
				bool hasSubCreateVar = false;
				if (nextToken == TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE)
					hasSubCreateVar = ParseMatchTable(freeRegIndex, jumpOutIndex, bindVarVec);
				else
					hasSubCreateVar = ParseMatchList(freeRegIndex, jumpOutIndex, bindVarVec);

				if(hasSubCreateVar)
					hasCreateVar = true;
				FREE_REG(freeRegIndex);

			}
			else if (nextToken == TOKEN_TYPE_IDENT)
			{
				//如果是变量，添加变量, 赋值给变量
				mLexer->GetNextToken();
				int varIndex = mSymbolTable->AddVariant(mLexer->GetCurLexeme(), mCurFuncIndex, 1, IDENT_TYPE_VAR, mMidCode->GetCurCodeIndex());

				int keyStrIndex = mSymbolTable->AddString(KeyName.c_str());
				//解析变量类型限制
				ParseMatchType(jumpOutIndex, ConsturctFactorByTableAndString(tableVarIndex, keyStrIndex));


				int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
				mMidCode->AddVarOperand(iInstrIndex, varIndex);
				mMidCode->AddTableStringOperand(iInstrIndex, tableVarIndex, keyStrIndex);
				hasCreateVar = true;
				bindVarVec.push_back(varIndex);

			}
			else if(nextToken == TOKEN_TYPE_INT || nextToken == TOKEN_TYPE_FLOAT || nextToken == TOKEN_TYPE_STRING)
			{
				mLexer->GetNextToken();
				//如果是常量需要进行比较
				int iInstrIndex = mMidCode->AddInstr(INSTR_JNE);
				mMidCode->AddTableStringOperand(iInstrIndex, tableVarIndex, mSymbolTable->AddString(KeyName.c_str()));
				if (nextToken == TOKEN_TYPE_INT)
				{
					mMidCode->AddIntOperand(iInstrIndex, mLexer->GetCurIntValue());
				}
				else if (nextToken == TOKEN_TYPE_FLOAT)
				{
					mMidCode->AddFloatOperand(iInstrIndex, StrToXFloat(mLexer->GetCurLexeme()));
				}
				else
				{
					mMidCode->AddStringIndexOperand(iInstrIndex, mSymbolTable->AddString(mLexer->GetCurLexeme()));
				}

				mMidCode->AddJumpIndexOperand(iInstrIndex, jumpOutIndex);
			}
			else if (nextToken == TOKEN_TYPE_DELM_UNDERSCORE)
			{
				mLexer->GetNextToken();

				int keyStrIndex = mSymbolTable->AddString(KeyName.c_str());
				//解析变量类型限制
				ParseMatchType(jumpOutIndex, ConsturctFactorByTableAndString(tableVarIndex, keyStrIndex));


				//table 中的'_' 匹配 非空元素
				int iInstrIndex = mMidCode->AddInstr(INSTR_JE);
				mMidCode->AddTableStringOperand(iInstrIndex, tableVarIndex, keyStrIndex);
				mMidCode->AddNilOperand(iInstrIndex);
				mMidCode->AddJumpIndexOperand(iInstrIndex, jumpOutIndex);

			}
			else
			{
				Error("invalid match table field pattern: expected a constant, binding name, '_', list pattern '[...]' or table pattern '{...}' after ':'");
			}
		}
		else // 后面没有跟':',表示就是直接的变量， key和变量同名
		{
			//如果是变量，添加变量, 赋值给变量
			int varIndex = mSymbolTable->AddVariant(KeyName.c_str(), mCurFuncIndex, 1, IDENT_TYPE_VAR, mMidCode->GetCurCodeIndex());


			int keyStrIndex = mSymbolTable->AddString(KeyName.c_str());
			//解析变量类型限制
			ParseMatchType(jumpOutIndex, ConsturctFactorByTableAndString(tableVarIndex, keyStrIndex));

			int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
			mMidCode->AddVarOperand(iInstrIndex, varIndex);
			mMidCode->AddTableStringOperand(iInstrIndex, tableVarIndex, keyStrIndex);
			bindVarVec.push_back(varIndex);
			hasCreateVar = true;
		}

		nextToken = mLexer->LookNextToken();
		if (nextToken == TOKEN_TYPE_DELIM_COMMA)
		{
			mLexer->GetNextToken();
		}
		else if (nextToken != TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE)
		{
			Error("match table pattern fields must be separated by ',' or closed with '}'");
		}

	}

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE, -1, "match table pattern must be closed with '}'");
	return hasCreateVar;
}