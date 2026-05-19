#pragma once


#include "CommonFunc.h"
#include <vector>
#include <map>
class CLexer;

class CMidCode
{
public:
	friend class XScriptVM;

	CMidCode();
	~CMidCode(void);

	int	 GetCurCodeIndex() { return (int)mCodeList[mCurFuncIndex].size(); }
	void	SetLex(CLexer* lex) { mCurLex = lex; }
	void	setCurFuncIndex(int index) { mCurFuncIndex = index; }
	int   GetNextJumpIndex();
	void  AddJumpTarget(int iJumpIndex);
	int   AddInstr(int OprCode);
	void  AddEmptyTableOperand(int instrIndex);
	void  AddIntOperand(int instrIndex, XInt opr);
	void  AddFloatOperand(int instrIndex, XFloat opr);
	void  AddStringIndexOperand(int instrIndex, int opr);
	void  AddJumpIndexOperand(int instrIndex, int opr);
	void  AddVarOperand(int instrIndex, int opr);
	void  AddNilOperand(int instrIndex);
	void  AddTableIndexOperand(int instrIndex, int iVarSymbolIndex, int index);

	void  AddTableFloatOperand(int instrIndex, int iVarSymbolIndex, XFloat fValue);
	void  AddTableIntOperand(int instrIndex, int iVarSymbolIndex, XInt value);
	void  AddTableStringOperand(int instrIndex, int iVarSymbolIndex, int stringIndex);

	void  AddTableRegOperand(int instrIndex, int iVarSymbolIndex, int index);

	void  AddRegOperand(int instrIndex, int iRegIndex = 0);
	int   getLastInstrOp();
	void  setIngoreInstr(bool ingore) { mIsIngoreInstr = ingore;}
public:
	std::map<int, std::vector<ICode> >    mCodeList;
	std::map<int, std::vector<JumpTarget> >   mJumpList;

	int						mCurFuncIndex;
	bool  mIsIngoreInstr;
	CLexer*					mCurLex;
};
