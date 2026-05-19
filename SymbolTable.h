#pragma once

#include "CommonFunc.h"
#include <vector>
#include <unordered_map>
#include <string>

class CMidCode;
class CParser;
using namespace std;

// Per-scope variable container: holds variables belonging to one scope (function or block)
struct ScopeVarTable
{
	std::unordered_map<std::string, VariantST*> nameMap;  // name -> var (visible at current block level)
	std::vector<VariantST*> varList;                       // ordered list for iteration (all vars including shadowed)
};

class CSymbolTable
{
friend class XScriptVM;
friend class CParser;
public:
	CSymbolTable(void);
	~CSymbolTable(void);
	static CSymbolTable * GetInstance()
	{
		static CSymbolTable instance;
		return &instance;
	}

	void			SetIsRecord( bool isRecord) { mIsRecordToken = isRecord; }

	TryCatchBlock&  GetTryCatchBlock(int funcIndex, int blockIndex);
	int				AddTryCatchBlock(int funcIndex, int parentBlockIndex);

	int				AddSwitchCase(int funcIndex);
	SwitchStruct&	GetSwitchStruct(int funcIndex, int switchIndex );

	void			SetHasVarArgs(int funcIndex);
	int				AddFunction(const char* pFuncName, int iParamNum = 0, int parentFuncIndex = 0, EFunctionType funcType = EFunctionType::Normal);
	void			AddSubFunction(int funcIndex, int subFuncIndex);

	int				AddVariant(const char* pVarName, int iScope = 0, int iSize = 1, int iType = 0, int startPc = -1);
	int				AddString(const char* str);
	char*			getString(int index);

	void			 SetFunctionParamNum(int iIndex, int iParamNum);
	FunctionST*		 GetFunctionByName(const char* pFuncName);

	VariantST*		 GetVarByName(const char* pVarName, int iScope, bool global = false);
	FunctionST*		 GetFunctionByIndex(int iIndex);

	// Block scope management
	void			 EnterBlock(int iScope, int currentPc = -1);
	void			 LeaveBlock(int iScope, int currentPc = -1);

	int				 SearchValue(const char* pVarName, int iScope);
	int				 SearchUpValue(const char* pVarName, int iScope, int& index);

	void			 GetFunctionVars(int scope, std::vector<std::string>& varVec);
	void			 GetFunctionLocalDebugInfos(int scope, std::vector<LocalVarDebugInfo>& debugInfos);
	void			 UpdateDebugPcOffset(int scope, int startIndex, int offset, bool includeStart = false);

	// Get all variables in a given scope (for iteration)
	const ScopeVarTable* GetScopeVars(int scope) const;

	VariantST*		GetVarByIndex(int iIndex);
	void			computeParmamStackIndex();
private:
	ScopeVarTable&  GetOrCreateScope(int scope);

	std::unordered_map<int, ScopeVarTable>  mScopeVarMap;    // scope -> variables in that scope
	std::unordered_map<int, int>            mBlockLevelMap;  // scope -> current block level
	std::vector<VariantST*>                 mAllVars;        // owns all VariantST, indexed by iIndex (mAllVars[i]->iIndex == i)

	vector<FunctionST>  mFuncTable;
	vector<StringST>    mStringTable;
	int                 mGlobalDataSize;

	int					mStringIndex;
	int					mVarIndex;
	int					mFuncIndex;

	bool				mIsRecordToken = false;
	friend class CParser;
};
