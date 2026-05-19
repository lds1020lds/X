#include "Parser.h"
#include "SourceFile.h"
#include "Lexer.h"
#include "MidCode.h"
#include "SymbolTable.h"
#include "XScriptVM.h"
#include <vector>
using std::vector;

// 在函数入口/出口插入寄存器保存/恢复代码
void  CParser::insertSaveRegCode()
{
	for (int i = 0; i < (int)mSymbolTable->mFuncTable.size(); i++)
	{
		const FunctionST& func = mSymbolTable->mFuncTable[i];
		int funcIndex = func.iIndex;
		int saveRegs[MAX_REG_NUM];
		int numNeedSaveReg = 0;
		for (int codeIndex = 0; codeIndex < (int)mMidCode->mCodeList[funcIndex].size(); codeIndex++)
		{
			Code code = mMidCode->mCodeList[funcIndex][codeIndex].code;
			for (int op = 0; op < (int)code.oprList.size(); op++)
			{
				Operand operand = code.oprList[op];

				if(operand.operandType == PST_Var_Index)
				{
					int regIndex = -1;
					for (int i = 0; i < MAX_REG_NUM; i++)
					{
						if (mReg[i].mVarIndex == operand.iSymbolIndex)
						{
							regIndex = i;
							break;
						}
					}

					if (regIndex >= 0)
					{
						bool hasFound = false;
						for (int j = 0; j < numNeedSaveReg; j++)
						{
							if (saveRegs[j] == regIndex)
							{
								hasFound = true;
								break;
							}
						}

						if (!hasFound)
						{
							saveRegs[numNeedSaveReg] = regIndex;
							numNeedSaveReg++;
						}
					}
				}
			}
		}

		int codeSize = (int)mMidCode->mCodeList[funcIndex].size();
		vector<ICode>::iterator beginIt = mMidCode->mCodeList[funcIndex].begin();
		vector<ICode>::iterator nextIt = beginIt;
		int curInstrIndex = 0;
		for (int reg = 0; reg < numNeedSaveReg; reg++)
		{
			ICode code;
			code.iCodeType = INSTR_TYPE_CODE;
			code.code.iCodeOpr = INSTR_PUSH;
			Operand operand;
			operand.operandType = PST_Var_Index;
			operand.iSymbolIndex = mReg[saveRegs[reg]].mVarIndex;
			code.lineIndex = -1;
			code.code.oprList.push_back(operand);
			nextIt = mMidCode->mCodeList[funcIndex].insert(nextIt, code);
		}
		updateSymbolsOffset(curInstrIndex, numNeedSaveReg, funcIndex, true);
		curInstrIndex += numNeedSaveReg;
		int numOrginalCode = 0;
		nextIt += numNeedSaveReg;

		int numInstrs = codeSize;
		for (int j = 0; j < numInstrs; j++)
		{
			ICode instr = *nextIt;
			if (instr.code.iCodeOpr == INSTR_RET)
			{

				for (int reg = 0; reg < numNeedSaveReg; reg++)
				{
					ICode code;
					code.iCodeType = INSTR_TYPE_CODE;
					code.code.iCodeOpr = INSTR_POP;
					Operand operand;
					operand.operandType = PST_Var_Index;
					code.lineIndex = -1;
					operand.iSymbolIndex = mReg[saveRegs[numNeedSaveReg - reg - 1]].mVarIndex;
					code.code.oprList.push_back(operand);
					nextIt = mMidCode->mCodeList[funcIndex].insert(nextIt, code);
				}

				updateSymbolsOffset(curInstrIndex, numNeedSaveReg, funcIndex);
				curInstrIndex += numNeedSaveReg;

				nextIt += numNeedSaveReg;
			}
			nextIt++;
			curInstrIndex++;
		}
		
	}
}

// 更新跳转目标的偏移量（插入/删除指令后调整）
void   CParser::updateSymbolsOffset(int startIndex, int offset, int funcIndex, bool includeStart)
{
	mSymbolTable->UpdateDebugPcOffset(funcIndex, startIndex, offset, includeStart);
	for (int i = 0; i < (int)mMidCode->mJumpList[funcIndex].size(); i++)
	{
		int codeIndex = mMidCode->mJumpList[funcIndex][i].codeIndex;
		if (codeIndex > startIndex || (includeStart && codeIndex == startIndex))
			mMidCode->mJumpList[funcIndex][i].codeIndex += offset;
	}

	FunctionST* func = mSymbolTable->GetFunctionByIndex(funcIndex);
	if (func != NULL)
	{
		auto updateInstrIndex = [startIndex, offset, includeStart](int& instrIndex)
		{
			if (instrIndex >= 0 && (instrIndex > startIndex || (includeStart && instrIndex == startIndex)))
				instrIndex += offset;
		};

		for (auto& switchData : func->switchStructVec)
		{
			updateInstrIndex(switchData.defaultInstr);
			for (auto& item : switchData.intValueInstrMap)
				updateInstrIndex(item.second);
			for (auto& item : switchData.floatValueInstrMap)
				updateInstrIndex(item.second);
			for (auto& item : switchData.stringValueInstrMap)
				updateInstrIndex(item.second);
		}

		for (auto& tryCatchBlock : func->tryCatchBlockVec)
		{
			updateInstrIndex(tryCatchBlock.startpc);
			updateInstrIndex(tryCatchBlock.catchpc);
			updateInstrIndex(tryCatchBlock.finallypc);
			updateInstrIndex(tryCatchBlock.outpc);
		}
	}
}

// 判断是否为条件跳转指令
bool	IsLogicJmp(int op)
{
	return (op == INSTR_JE || op == INSTR_JNE || op == INSTR_JG || op == INSTR_JL || op == INSTR_JGE || op == INSTR_JLE);
}

// 判断是否为比较测试指令
bool	IsLogicTest(int op)
{
	return (op == INSTR_TEST_E || op == INSTR_TEST_NE || op == INSTR_TEST_G || op == INSTR_TEST_L || op == INSTR_TEST_GE || op == INSTR_TEST_LE);
}

// 中间码优化：消除冗余的PUSH/POP、MOV指令，合并比较-跳转
void  CParser::optimizeCode()
{
	bool hasOptimize = true;

	while(hasOptimize)
	{
		hasOptimize = false;
		for (int i = 0; i < (int)mSymbolTable->mFuncTable.size(); i++)
		{
			FunctionST& func = mSymbolTable->mFuncTable[i];
			int iFuncIndex = func.iIndex;
			int numNeedSaveReg = 0;
			for (int codeIndex = 0; codeIndex < (int)mMidCode->mCodeList[iFuncIndex].size(); codeIndex++)
			{
				Code code = mMidCode->mCodeList[iFuncIndex][codeIndex].code;
				if (code.iCodeOpr == INSTR_PUSH && code.oprList[0].operandType != PST_Reg && !isJumpTarget(codeIndex, iFuncIndex))
				{

					int nextPopIndex = codeIndex + 1;
					if (nextPopIndex < (int)mMidCode->mCodeList[iFuncIndex].size())
					{
						Code& nextCode = mMidCode->mCodeList[iFuncIndex][nextPopIndex].code;
						if (nextCode.iCodeOpr == INSTR_POP)
						{
							bool hasJumpTarget = false;
							for (int c = codeIndex + 1; c <= nextPopIndex; c++)
							{
								if (isJumpTarget(c, iFuncIndex))
								{
									hasJumpTarget = true;
									break;
								}
							}

							if (!hasJumpTarget && !hasOperandBeenUsed(code.oprList[0], codeIndex + 1, nextPopIndex, iFuncIndex))
							{
								updateSymbolsOffset(codeIndex, -1, iFuncIndex);
								mMidCode->mCodeList[iFuncIndex].erase(mMidCode->mCodeList[iFuncIndex].begin() + codeIndex);
								mMidCode->mCodeList[iFuncIndex][nextPopIndex - 1].code.iCodeOpr = INSTR_MOV;
								mMidCode->mCodeList[iFuncIndex][nextPopIndex - 1].code.oprList.push_back(code.oprList[0]);
								codeIndex--;
								hasOptimize = true;
							}
						}
					}
				
				}
				else if (code.iCodeOpr == INSTR_MOV && code.oprList[1].operandType == PST_Var_Index && IsRegVar(code.oprList[1].iSymbolIndex) && codeIndex > 0)
				{
					Code lastCode = mMidCode->mCodeList[iFuncIndex][codeIndex - 1].code;
					if (lastCode.oprList[0].operandType == PST_Var_Index && code.oprList[1].iSymbolIndex == lastCode.oprList[0].iSymbolIndex
						&& !isJumpTarget(codeIndex, iFuncIndex))
					{
						if (lastCode.iCodeOpr == INSTR_MOV ||
							lastCode.iCodeOpr == INSTR_ADD_TO ||
							lastCode.iCodeOpr == INSTR_SUB_TO ||
							lastCode.iCodeOpr == INSTR_MUL_TO ||
							lastCode.iCodeOpr == INSTR_MOD_TO ||
							lastCode.iCodeOpr == INSTR_DIV_TO ||
							lastCode.iCodeOpr == INSTR_EXP_TO ||
							lastCode.iCodeOpr == INSTR_AND_TO ||
							lastCode.iCodeOpr == INSTR_OR_TO ||
							lastCode.iCodeOpr == INSTR_XOR_TO ||
							lastCode.iCodeOpr == INSTR_NOT_TO ||
							lastCode.iCodeOpr == INSTR_SHL_TO ||
							lastCode.iCodeOpr == INSTR_SHR_TO ||
							lastCode.iCodeOpr == INSTR_CONCAT_TO ||
							lastCode.iCodeOpr == INSTR_TEST_E ||
							lastCode.iCodeOpr == INSTR_TEST_NE ||
							lastCode.iCodeOpr == INSTR_TEST_G ||
							lastCode.iCodeOpr == INSTR_TEST_L ||
							lastCode.iCodeOpr == INSTR_TEST_GE ||
							lastCode.iCodeOpr == INSTR_TEST_LE ||
							lastCode.iCodeOpr == INSTR_LOGIC_AND ||
							lastCode.iCodeOpr == INSTR_LOGIC_OR )
						{
							updateSymbolsOffset(codeIndex, -1, iFuncIndex);
							mMidCode->mCodeList[iFuncIndex].erase(mMidCode->mCodeList[iFuncIndex].begin() + codeIndex);
							codeIndex--;
							mMidCode->mCodeList[iFuncIndex][codeIndex].code.oprList[0] = code.oprList[0];
						}
					}

				}
				else if (!isJumpTarget(codeIndex, iFuncIndex) && codeIndex > 0 && code.iCodeOpr == INSTR_JE && code.oprList[0].operandType == PST_Var_Index && IsRegVar(code.oprList[0].iSymbolIndex) && code.oprList[1].operandType == PST_Int && code.oprList[1].iIntValue == 0)
				{
					Code lastCode = mMidCode->mCodeList[iFuncIndex][codeIndex - 1].code;
					bool nextCodeUsesTestReg = false;
					if (codeIndex + 1 < (int)mMidCode->mCodeList[iFuncIndex].size())
					{
						Code nextCode = mMidCode->mCodeList[iFuncIndex][codeIndex + 1].code;
						nextCodeUsesTestReg = nextCode.oprList.size() > 0
							&& nextCode.oprList[0].operandType == PST_Var_Index
							&& nextCode.oprList[0].iSymbolIndex == code.oprList[0].iSymbolIndex;
					}
					if (!nextCodeUsesTestReg && IsLogicTest(lastCode.iCodeOpr) && lastCode.oprList[0].operandType == PST_Var_Index && IsRegVar(lastCode.oprList[0].iSymbolIndex) && lastCode.oprList[0].iSymbolIndex == code.oprList[0].iSymbolIndex)
					{

						updateSymbolsOffset(codeIndex, -1, iFuncIndex);

						int instrOp = -1;
						switch (lastCode.iCodeOpr)
						{
							case INSTR_TEST_E:
								instrOp = INSTR_JNE;
								break;
							case INSTR_TEST_NE:
								instrOp = INSTR_JE;
								break;
							case INSTR_TEST_G:
								instrOp = INSTR_JLE;
								break;
							case INSTR_TEST_L:
								instrOp = INSTR_JGE;
								break;
							case INSTR_TEST_GE:
								instrOp = INSTR_JL;
								break;
							case INSTR_TEST_LE:
								instrOp = INSTR_JG;
								break;
						}

						mMidCode->mCodeList[iFuncIndex].erase(mMidCode->mCodeList[iFuncIndex].begin() + codeIndex);
						codeIndex--;

						mMidCode->mCodeList[iFuncIndex][codeIndex].code.iCodeOpr = instrOp;
						mMidCode->mCodeList[iFuncIndex][codeIndex].code.oprList[0] = lastCode.oprList[1];
						mMidCode->mCodeList[iFuncIndex][codeIndex].code.oprList[1] = lastCode.oprList[2];
						mMidCode->mCodeList[iFuncIndex][codeIndex].code.oprList[2] = code.oprList[2];
					}
				}
			}
		}
	}

}

// 判断某指令索引是否为跳转目标（不能被优化删除）
bool  CParser::isJumpTarget(int instrIndex, int funcIndex)
{
	int numJumpTarget = (int)mMidCode->mJumpList[funcIndex].size();
	for (int i  = 0; i < numJumpTarget; i++)
	{
		if (mMidCode->mJumpList[funcIndex][i].codeIndex == instrIndex)
			return true;
	}

	FunctionST* func = mSymbolTable->GetFunctionByIndex(funcIndex);
	if (func != NULL)
	{
		for (auto& switchData : func->switchStructVec)
		{
			if (switchData.defaultInstr == instrIndex)
				return true;
			for (auto& item : switchData.intValueInstrMap)
			{
				if (item.second == instrIndex)
					return true;
			}
			for (auto& item : switchData.floatValueInstrMap)
			{
				if (item.second == instrIndex)
					return true;
			}
			for (auto& item : switchData.stringValueInstrMap)
			{
				if (item.second == instrIndex)
					return true;
			}
		}

		for (auto& tryCatchBlock : func->tryCatchBlockVec)
		{
			if (tryCatchBlock.startpc == instrIndex
				|| tryCatchBlock.catchpc == instrIndex
				|| tryCatchBlock.finallypc == instrIndex
				|| tryCatchBlock.outpc == instrIndex)
				return true;
		}
	}

	return false;
}

// 判断操作数在指定指令范围内是否被写入（用于优化安全性检查）
bool  CParser::hasOperandBeenUsed(const Operand& op, int fromIndex, int endIndex, int funcIndex)
{

	if (op.operandType == PST_Int
		|| op.operandType == PST_Float
		|| op.operandType == PST_String_Index
		|| op.operandType == PST_JumpTarget_Index
		|| op.operandType == PST_Nil)
	{
		return false;
	}

	// 写入目标操作数的指令集合
	static const int s_writeInstrSet[] = {
		INSTR_MOV, INSTR_ADD, INSTR_SUB, INSTR_MUL, INSTR_DIV, INSTR_MOD,
		INSTR_EXP, INSTR_NEG, INSTR_INC, INSTR_DEC, INSTR_AND, INSTR_OR,
		INSTR_XOR, INSTR_SHL, INSTR_SHR,
		INSTR_ADD_TO, INSTR_SUB_TO, INSTR_MUL_TO, INSTR_DIV_TO, INSTR_MOD_TO,
		INSTR_EXP_TO, INSTR_AND_TO, INSTR_OR_TO, INSTR_XOR_TO, INSTR_NOT_TO,
		INSTR_SHL_TO, INSTR_SHR_TO, INSTR_CONCAT_TO,
		INSTR_TEST_E, INSTR_TEST_NE, INSTR_TEST_G, INSTR_TEST_L,
		INSTR_TEST_GE, INSTR_TEST_LE,
		INSTR_LOGIC_AND, INSTR_LOGIC_OR, INSTR_APPEND,
	};

	for (int codeIndex = fromIndex; codeIndex < endIndex; codeIndex++)
	{
		Code& code = mMidCode->mCodeList[funcIndex][codeIndex].code;
		bool isWriteInstr = false;
		for (int instrCode : s_writeInstrSet)
		{
			if (code.iCodeOpr == instrCode)
			{
				isWriteInstr = true;
				break;
			}
		}
		if (isWriteInstr)
		{
			if (isOperandRelated(op, code.oprList[0]))
				return true;
		}
	}

	return false;
}

// 判断两个操作数是否引用同一变量
bool  CParser::isOperandRelated(const Operand& op1, const Operand& op2)
{
	_ASSERT(op1.operandType == PST_Table  ||op1.operandType == PST_Var_Index || op1.operandType == PST_Reg);
	_ASSERT(op2.operandType == PST_Table ||op2.operandType == PST_Var_Index || op2.operandType == PST_Reg);

	bool op1IsVar = (op1.operandType == PST_Var_Index || op1.operandType == PST_Table);
	bool op2IsVar = (op2.operandType == PST_Var_Index || op2.operandType == PST_Table);
	if (op1IsVar != op2IsVar)
		return false;
	if (!op1IsVar)
		return true;
	return op1.iSymbolIndex == op2.iSymbolIndex;
}
