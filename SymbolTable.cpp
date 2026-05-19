#include "SymbolTable.h"
#include "CommonFunc.h"
#include <algorithm>
CSymbolTable::CSymbolTable(void)
{
	mGlobalDataSize = 1;
	mVarIndex = 0;
	mStringIndex = 0;
	mFuncIndex = 0;
}

CSymbolTable::~CSymbolTable(void)
{
	// Free all allocated VariantST
	for (auto* var : mAllVars)
	{
		delete var;
	}
	mAllVars.clear();
}

int		CSymbolTable::AddTryCatchBlock(int funcIndex, int parentBlockIndex)
{
	int index = (int)mFuncTable[funcIndex].tryCatchBlockVec.size();
	TryCatchBlock block;
	block.funcIndex = funcIndex;
	block.parentBlockIndex = parentBlockIndex;
	mFuncTable[funcIndex].tryCatchBlockVec.push_back(block);
	return index;
}

int CSymbolTable::AddSwitchCase(int funcIndex)
{
	int index = (int)mFuncTable[funcIndex].switchStructVec.size();
	SwitchStruct switchStruct;
	mFuncTable[funcIndex].switchStructVec.push_back(switchStruct);
	return index;
}

SwitchStruct&	CSymbolTable::GetSwitchStruct(int funcIndex, int switchIndex)
{
	return mFuncTable[funcIndex].switchStructVec[switchIndex];
}

TryCatchBlock& CSymbolTable::GetTryCatchBlock(int funcIndex, int blockIndex)
{
	return mFuncTable[funcIndex].tryCatchBlockVec[blockIndex];

}

int   CSymbolTable::AddFunction(const char* pFuncName, int iParamNum, int parentFuncIndex, EFunctionType funcType)
{
	if (mIsRecordToken)
		return -1;

	FunctionST func;
	strncpy_s(func.funcName,  pFuncName, MAX_IDENT_SIZE);
	func.iParamSize = iParamNum;
	func.localDataSize = 0;
	func.curParamIndex = 0;
	func.curVarIndex = 0;
	func.iIndex = mFuncIndex;
	func.hasVarArgs = false;
	func.funcType = funcType;
	func.parentIndex = parentFuncIndex;
	mFuncIndex++;
	mFuncTable.push_back(func);
	return func.iIndex;
}

void  CSymbolTable::SetHasVarArgs(int funcIndex)
{
	for (int i = 0; i < (int)mFuncTable.size(); i++)
	{
		if (mFuncTable[i].iIndex == funcIndex)
			mFuncTable[i].hasVarArgs = true;
	}
}


void	CSymbolTable::GetFunctionVars(int scope, std::vector<std::string>& varVec)
{
	auto it = mScopeVarMap.find(scope);
	if (it != mScopeVarMap.end())
	{
		for (auto* var : it->second.varList)
		{
			varVec.push_back(var->varName);
		}
	}
}

void CSymbolTable::GetFunctionLocalDebugInfos(int scope, std::vector<LocalVarDebugInfo>& debugInfos)
{
	auto it = mScopeVarMap.find(scope);
	if (it == mScopeVarMap.end())
		return;

	const std::vector<VariantST*>& vars = it->second.varList;
	for (int i = 0; i < (int)vars.size(); i++)
	{
		VariantST* var = vars[i];
		LocalVarDebugInfo info;
		info.name = var->varName;
		info.localIndex = i;
		info.startPc = var->debugStartPc;
		info.endPc = var->debugEndPc;
		debugInfos.push_back(info);
	}
}

void CSymbolTable::UpdateDebugPcOffset(int scope, int startIndex, int offset, bool includeStart)
{
	auto it = mScopeVarMap.find(scope);
	if (it == mScopeVarMap.end())
		return;

	for (auto* var : it->second.varList)
	{
		if (var->debugStartPc > startIndex || (includeStart && var->debugStartPc == startIndex))
			var->debugStartPc += offset;
		if (var->debugEndPc >= 0 && (var->debugEndPc > startIndex || (includeStart && var->debugEndPc == startIndex)))
			var->debugEndPc += offset;
	}
}

ScopeVarTable& CSymbolTable::GetOrCreateScope(int scope)
{
	return mScopeVarMap[scope];
}

const ScopeVarTable* CSymbolTable::GetScopeVars(int scope) const
{
	auto it = mScopeVarMap.find(scope);
	if (it != mScopeVarMap.end())
		return &it->second;
	return nullptr;
}

int   CSymbolTable::AddVariant(const char* pVarName, int iScope, int iSize, int iType, int startPc)
{
	if (mIsRecordToken)
		return -1;

	// Global scope (iScope < 0) has no block nesting, always use level 0
	int blockLevel = (iScope >= 0) ? mBlockLevelMap[iScope] : 0;

	if (strlen(pVarName) > 0)
	{
		VariantST* lastVar = GetVarByName(pVarName, iScope);
		if (lastVar != NULL && (iScope < 0 || lastVar->blockLevel == blockLevel))
		{
			// Global: always reuse (no block shadowing)
			// Local: reuse only if same block level
			return lastVar->iIndex;
		}
	}
	

	VariantST* var = new VariantST();
	var->iScope = iScope;
	var->iType = iType;
	var->blockLevel = blockLevel;
	var->debugStartPc = iScope >= 0 ? (startPc >= 0 ? startPc : 0) : -1;
	var->debugEndPc = -1;
	strncpy_s(var->varName,  pVarName, MAX_IDENT_SIZE);

	var->iIndex = mVarIndex;
	mVarIndex++;

	if (iType == IDENT_TYPE_VAR || iType == IDENT_TYPE_INTERNAL_VAR)
	{
		if (iScope >= 0)
		{
			FunctionST* func = GetFunctionByIndex(iScope);
			func->localDataSize += 1;
		}

	}

	// Add to all containers (mAllVars[iIndex] == var)
	mAllVars.push_back(var);

	ScopeVarTable& scopeTable = GetOrCreateScope(iScope);
	scopeTable.varList.push_back(var);
	if (strlen(pVarName) > 0)
	{
		// This may shadow a variable from an outer block level
		scopeTable.nameMap[pVarName] = var;
	}

	return var->iIndex;
}

int   CSymbolTable::AddString(const char* str)
{
	for (int i = 0; i < (int)mStringTable.size(); i++)
	{
		if (strcmp(mStringTable[i].str, str) == 0)
			return i;
	}


	StringST strST;
	strncpy_s(strST.str,  str, MAX_STRING_SIZE);
	strST.iIndex = mStringIndex;
	mStringTable.push_back(strST);
	mStringIndex++;
	return strST.iIndex;
}


char* CSymbolTable::getString(int index)
{
	return mStringTable[index].str;
}

FunctionST*   CSymbolTable::GetFunctionByName(const char* pFuncName)
{
	for (int i = 0; i < (int)mFuncTable.size(); i++)
	{
		if (strcmp(mFuncTable[i].funcName, pFuncName) == 0)
			return &mFuncTable[i];
	}
	return NULL;
}


int	 CSymbolTable::SearchValue(const char* pVarName, int iScope)
{
	int index = -1;
	int type = SearchUpValue(pVarName, iScope, index);
	if (type == VUPVALUE)
	{
		return (index | UPVALMASK);
	}

	return index;
}

int	CSymbolTable::SearchUpValue(const char* pVarName, int iScope, int& index)
{
	// Look up in the scope's name map (O(1) instead of linear scan)
	// Only finds variables visible at current block level (managed by EnterBlock/LeaveBlock)
	auto scopeIt = mScopeVarMap.find(iScope);
	if (scopeIt != mScopeVarMap.end())
	{
		auto varIt = scopeIt->second.nameMap.find(pVarName);
		if (varIt != scopeIt->second.nameMap.end())
		{
			index = varIt->second->iIndex;
		}
	}

	if (index >= 0)
	{
		return VLOCAL;
	}
	else
	{
		FunctionST* func = GetFunctionByIndex(iScope);
		if (func->parentIndex >= 0)
		{
			int type = SearchUpValue(pVarName, func->parentIndex, index);
			if (type == VGLOBAL)
			{
				return VGLOBAL;
			}
			else
			{
				int newIndex = -1;
				for (int i = 0; i < (int)func->upValueVec.size(); i++)
				{
					if (type == func->upValueVec[i].type
						&& index == func->upValueVec[i].index)
					{
						newIndex = i;
						break;
					}
				}
				if (newIndex < 0)
				{
					UpValueST up(type, index, pVarName);
					newIndex = (int)func->upValueVec.size();
					func->upValueVec.push_back(up);
				}

				index = newIndex;
				return VUPVALUE;
			}
		}
		else
		{
			index = AddVariant(pVarName, -1);
			return VGLOBAL;
		}
		
	}
}


VariantST*    CSymbolTable::GetVarByName(const char* pVarName, int iScope, bool global)
{
	// O(1) lookup in scope's name map (only sees variables visible at current block level)
	auto scopeIt = mScopeVarMap.find(iScope);
	if (scopeIt != mScopeVarMap.end())
	{
		auto varIt = scopeIt->second.nameMap.find(pVarName);
		if (varIt != scopeIt->second.nameMap.end())
			return varIt->second;
	}

	if (global)
	{
		// Look in global scope (scope == -1)
		auto globalIt = mScopeVarMap.find(-1);
		if (globalIt != mScopeVarMap.end())
		{
			auto varIt = globalIt->second.nameMap.find(pVarName);
			if (varIt != globalIt->second.nameMap.end())
				return varIt->second;
		}
	}

	return NULL;
}

FunctionST*   CSymbolTable::GetFunctionByIndex(int iIndex)
{
	if (iIndex < 0 && mIsRecordToken)
	{
		static FunctionST st;
		return &st;
	}
	for (int i = 0; i < (int)mFuncTable.size(); i++)
	{
		if (mFuncTable[i].iIndex == iIndex)
			return &mFuncTable[i];
	}
	return NULL;
}

void  CSymbolTable::AddSubFunction(int funcIndex, int subFuncIndex)
{
	if (subFuncIndex < 0 && mIsRecordToken)
	{
		return;
	}

	FunctionST *func = GetFunctionByIndex(funcIndex);
	if (func != NULL)
		func->subIndexVec.push_back(subFuncIndex);
}



VariantST*    CSymbolTable::GetVarByIndex(int iIndex)
{
	// O(1) lookup by index (mAllVars is ordered by iIndex)
	if (iIndex >= 0 && iIndex < (int)mAllVars.size())
		return mAllVars[iIndex];
	return NULL;
}

void CSymbolTable::EnterBlock(int iScope, int currentPc)
{
	(void)currentPc;
	mBlockLevelMap[iScope]++;
}

void CSymbolTable::LeaveBlock(int iScope, int currentPc)
{
	int curLevel = mBlockLevelMap[iScope];
	auto scopeIt = mScopeVarMap.find(iScope);
	if (scopeIt != mScopeVarMap.end())
	{
		ScopeVarTable& scopeTable = scopeIt->second;
		// Remove variables declared at the current block level from nameMap
		// (they become invisible, but their stack slots remain allocated)
		for (auto* var : scopeTable.varList)
		{
			if (var->blockLevel == curLevel && strlen(var->varName) > 0)
			{
				if (currentPc >= 0 && var->debugEndPc < 0)
					var->debugEndPc = currentPc;

				// Check if this var is still the one in nameMap (could have been shadowed)
				auto it = scopeTable.nameMap.find(var->varName);
				if (it != scopeTable.nameMap.end() && it->second == var)
				{
					// Restore the previous variable with the same name from an outer block level
					VariantST* restored = nullptr;
					for (auto* candidate : scopeTable.varList)
					{
						if (candidate != var
							&& candidate->blockLevel < curLevel
							&& strcmp(candidate->varName, var->varName) == 0)
						{
							if (restored == nullptr || candidate->blockLevel > restored->blockLevel)
								restored = candidate;
						}
					}
					if (restored)
						scopeTable.nameMap[var->varName] = restored;
					else
						scopeTable.nameMap.erase(it);
				}
			}
		}
	}
	mBlockLevelMap[iScope]--;
}
void          CSymbolTable::SetFunctionParamNum(int iIndex, int iParamNum)
{
	FunctionST *func =  GetFunctionByIndex(iIndex);
	if (func != NULL)
		func->iParamSize = iParamNum;
}


void  CSymbolTable::computeParmamStackIndex()
{
	for (int i = 0; i < (int)mFuncTable.size(); i++)
	{
		FunctionST& func = mFuncTable[i];
		func.curParamIndex = 0;
		func.curVarIndex = 0;

		auto scopeIt = mScopeVarMap.find(func.iIndex);
		if (scopeIt == mScopeVarMap.end())
			continue;

		ScopeVarTable& scopeTable = scopeIt->second;
		std::stable_partition(scopeTable.varList.begin(), scopeTable.varList.end(), [](VariantST* var) {
			return var->iType != IDENT_TYPE_INTERNAL_VAR;
		});

		for (auto* var : scopeTable.varList)
		{
			if (var->iType == IDENT_TYPE_PARAM)
			{
				var->stackIndex = -(func.localDataSize + func.iParamSize - func.curParamIndex);
				func.curParamIndex++;
			}
			else if (var->iType == IDENT_TYPE_VAR || var->iType == IDENT_TYPE_INTERNAL_VAR)
			{
				var->stackIndex = -func.localDataSize + func.curVarIndex;
				func.curVarIndex++;
			}
		}
	}
}


