#include "XScriptVM.h"
#include "xregexlib.h"
#include "regex.h"

static TABLE regex_match_table(XScriptVM* vm, const MatchData& match)
{
	TABLE matchData = vm->newTable();
	vm->setTableValue(matchData, "content", match.matchedValue.c_str());
	if (match.groupIDMap.size() > 0)
	{
		TABLE groupID = vm->newTable();
		std::map<int, std::string>::const_iterator it = match.groupIDMap.begin();
		for (; it != match.groupIDMap.end(); it++)
			vm->setTableValue(groupID, (XInt)it->first, it->second.c_str());
		vm->setTableValue(matchData, "groupID", groupID);
	}
	if (match.groupNameMap.size() > 0)
	{
		TABLE groupName = vm->newTable();
		std::map<std::string, std::string>::const_iterator it = match.groupNameMap.begin();
		for (; it != match.groupNameMap.end(); it++)
			vm->setTableValue(groupName, it->first.c_str(), it->second.c_str());
		vm->setTableValue(matchData, "groupName", groupName);
	}
	return matchData;
}

void init_regex_lib()
{
	std::vector<HostFunction> funcs;
	funcs.push_back(HostFunction("search", xregex_search));
	funcs.push_back(HostFunction("regmatch", xregex_match));
	gScriptVM.RegisterHostLib("regex", funcs);
}

void xregex_search(XScriptVM* vm)
{
	CheckParam(regex.search, 0, pattern, OP_TYPE_STRING);
	CheckParam(regex.search, 1, text, OP_TYPE_STRING);
	Regex regex((char*)stringRawValue(&pattern));
	const std::vector<MatchData>& retVec = regex.Search((char*)stringRawValue(&text));
	if (retVec.empty())
	{
		vm->setReturnAsNil(0);
		return;
	}
	ListValue* list = vm->CreateList();
	for (int i = 0; i < (int)retVec.size(); i++)
		vm->ListAppend(list, vm->ConstructValue(regex_match_table(vm, retVec[i])));
	vm->setReturnAsValue(vm->ConstructValue(list));
}

void xregex_match(XScriptVM* vm)
{
	CheckParam(regex.match, 0, pattern, OP_TYPE_STRING);
	CheckParam(regex.match, 1, text, OP_TYPE_STRING);
	Regex regex((char*)stringRawValue(&pattern));
	vm->setReturnAsInt(regex.Match((char*)stringRawValue(&text)) ? 1 : 0);
}
