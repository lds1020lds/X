#include "MidCode.h"
#include "Lexer.h"
using namespace std;
CMidCode::CMidCode()
{
	mIsIngoreInstr = false;
	mCurFuncIndex = 0;
}

CMidCode::~CMidCode(void)
{
}


int   CMidCode::GetNextJumpIndex()
{
	static int iJumpNum = 0;
	return iJumpNum++;
}


void   CMidCode::AddJumpTarget(int iJumpIndex)
{
	if (mIsIngoreInstr)
		return;

	JumpTarget jump;
	jump.jumpIndex = iJumpIndex;
	jump.codeIndex = (int)mCodeList[mCurFuncIndex].size();
	mJumpList[mCurFuncIndex].push_back(jump);
}

int   CMidCode::AddInstr(int OprCode)
{
	if (mIsIngoreInstr)
		return -1;

	ICode code;
	code.lineIndex = mCurLex->GetLine() + 1;
	code.iCodeType = INSTR_TYPE_CODE;
	code.code.iCodeOpr = OprCode;
	mCodeList[mCurFuncIndex].push_back(code);
	return (int)mCodeList[mCurFuncIndex].size() - 1;
}
void  CMidCode::AddIntOperand(int instrIndex, XInt opr)
{
	if (mIsIngoreInstr)
		return;

	Operand operand;
	operand.operandType = PST_Int;
	operand.iIntValue = opr;
	mCodeList[mCurFuncIndex][instrIndex].code.oprList.push_back(operand);
}
void  CMidCode::AddFloatOperand(int instrIndex, XFloat opr)
{
	if (mIsIngoreInstr)
		return;

	Operand operand;
	operand.operandType = PST_Float;
	operand.fFloatValue = opr;
	mCodeList[mCurFuncIndex][instrIndex].code.oprList.push_back(operand);
}
void  CMidCode::AddStringIndexOperand(int instrIndex, int opr)
{
	if (mIsIngoreInstr)
		return;

	Operand operand;
	operand.operandType = PST_String_Index;
	operand.iStringIndex = opr;
	mCodeList[mCurFuncIndex][instrIndex].code.oprList.push_back(operand);
}
void  CMidCode::AddJumpIndexOperand(int instrIndex, int opr)
{
	if (mIsIngoreInstr)
		return;

	Operand operand;
	operand.operandType = PST_JumpTarget_Index;
	operand.iJumppIndex = opr;
	mCodeList[mCurFuncIndex][instrIndex].code.oprList.push_back(operand);
}

void  CMidCode::AddVarOperand(int instrIndex, int opr)
{
	if (mIsIngoreInstr)
		return;

	Operand operand;
	operand.operandType = PST_Var_Index;
	operand.iSymbolIndex = opr;
	mCodeList[mCurFuncIndex][instrIndex].code.oprList.push_back(operand);
}

void  CMidCode::AddNilOperand(int instrIndex)
{
	if (mIsIngoreInstr)
		return;

	Operand operand;
	operand.operandType = PST_Nil;
	mCodeList[mCurFuncIndex][instrIndex].code.oprList.push_back(operand);
}

void  CMidCode::AddTableIndexOperand(int instrIndex, int iVarSymbolIndex,  int index)
{
	if (mIsIngoreInstr)
		return;

	Operand operand;
	operand.operandType = PST_Table;
	operand.iSymbolIndex = iVarSymbolIndex;
	operand.tableIndexType = ROT_Stack_Index;
	operand.iIntTableValue = index;
	mCodeList[mCurFuncIndex][instrIndex].code.oprList.push_back(operand);
}

void  CMidCode::AddTableRegOperand(int instrIndex, int iVarSymbolIndex, int index)
{
	if (mIsIngoreInstr)
		return;
	Operand operand;
	operand.operandType = PST_Table;
	operand.iSymbolIndex = iVarSymbolIndex;
	operand.tableIndexType = ROT_Reg;
	operand.iIntTableValue = index;
	mCodeList[mCurFuncIndex][instrIndex].code.oprList.push_back(operand);
}

void  CMidCode::AddTableFloatOperand(int instrIndex, int iVarSymbolIndex, XFloat fValue)
{
	if (mIsIngoreInstr)
		return;

	Operand operand;
	operand.operandType = PST_Table;
	operand.iSymbolIndex = iVarSymbolIndex;
	operand.tableIndexType = ROT_Float;
	operand.fFloatTableValue = fValue;
	mCodeList[mCurFuncIndex][instrIndex].code.oprList.push_back(operand);
}

void  CMidCode::AddTableIntOperand(int instrIndex, int iVarSymbolIndex, XInt value)
{
	if (mIsIngoreInstr)
		return;

	Operand operand;
	operand.operandType = PST_Table;
	operand.iSymbolIndex = iVarSymbolIndex;
	operand.tableIndexType = ROT_Int;
	operand.iIntTableValue = value;
	mCodeList[mCurFuncIndex][instrIndex].code.oprList.push_back(operand);
}

void  CMidCode::AddTableStringOperand(int instrIndex, int iVarSymbolIndex, int stringIndex)
{
	if (mIsIngoreInstr)
		return;

	Operand operand;
	operand.operandType = PST_Table;
	operand.iSymbolIndex = iVarSymbolIndex;
	operand.tableIndexType = ROT_String;
	operand.iIntTableValue = stringIndex;
	mCodeList[mCurFuncIndex][instrIndex].code.oprList.push_back(operand);
}

void  CMidCode::AddRegOperand(int instrIndex, int iRegIndex)
{
	if (mIsIngoreInstr)
		return;

	Operand operand;
	operand.operandType = PST_Reg;
	operand.iRegIndex = iRegIndex;
	mCodeList[mCurFuncIndex][instrIndex].code.oprList.push_back(operand);
}

void  CMidCode::AddEmptyTableOperand(int instrIndex)
{
	if (mIsIngoreInstr)
		return;

	Operand operand;
	operand.operandType = PST_Table;
	operand.iSymbolIndex = -1;
	mCodeList[mCurFuncIndex][instrIndex].code.oprList.push_back(operand);
}



int CMidCode::getLastInstrOp()
 {
	 if (mCodeList[mCurFuncIndex].size() > 0)
	 {
		 return mCodeList[mCurFuncIndex][mCodeList[mCurFuncIndex].size() - 1].code.iCodeOpr;
	 }

	 return -1;
	 
 }