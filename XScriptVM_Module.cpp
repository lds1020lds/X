#include "XScriptVM.h"

// 通过 package.path 搜索模块文件路径
// 将路径模板中的 '?' 替换为模块名，按 ';' 分割逐个尝试
static std::string SearchModulePath(XScriptVM* vm, const std::string& moduleName)
{
	// 获取 package.path
	std::string searchPath = "./?.xs";
	Value* packageValue = vm->GetGlobalValue("package");
	if (packageValue && packageValue->type == OP_TYPE_TABLE)
	{
		Value pathValue;
		if (vm->getTableValue(packageValue->tableData, vm->ConstructValue("path"), pathValue))
		{
			if (pathValue.type == OP_TYPE_STRING)
			{
				searchPath = stringRawValue(&pathValue);
			}
		}
	}

	// 按 ';' 分割路径模板，逐个尝试
	std::string::size_type start = 0;
	while (start < searchPath.size())
	{
		std::string::size_type end = searchPath.find(';', start);
		std::string tmpl;
		if (end == std::string::npos)
		{
			tmpl = searchPath.substr(start);
			start = searchPath.size();
		}
		else
		{
			tmpl = searchPath.substr(start, end - start);
			start = end + 1;
		}

		// 将 '?' 替换为模块名
		std::string filePath;
		for (size_t i = 0; i < tmpl.size(); i++)
		{
			if (tmpl[i] == '?')
				filePath += moduleName;
			else
				filePath += tmpl[i];
		}

		// 尝试打开文件
		FILE* f = fopen(filePath.c_str(), "r");
		if (f)
		{
			fclose(f);
			return filePath;
		}
	}

	return "";
}

// 加载模块并返回模块导出值（新接口）
// 流程：查缓存 -> 设占位值 -> 搜索路径 -> 编译 -> 执行 -> 捕获返回值 -> 存缓存 -> 返回
Value	XScriptVM::RequireModule(const char* moduleName)
{
	// 规范化模块名（补 .xs 后缀）
	std::string modName = moduleName;
	if (modName.find(".xs", 0) == std::string::npos)
	{
		modName += ".xs";
	}

	// 查缓存：如果已加载则直接返回缓存值
	Value cachedValue;
	if (getTableValue(mModuleTable, ConstructValue(modName.c_str()), cachedValue))
	{
		return cachedValue;
	}

	// 设置占位值 true（防止循环依赖导致无限递归）
	Value trueValue;
	trueValue.type = OP_TYPE_INT;
	trueValue.iIntValue = 1;
	setTableValue(mModuleTable, ConstructValue(modName.c_str()), trueValue);

	// 通过 package.path 搜索模块文件
	// 搜索时使用不带 .xs 后缀的模块名（因为路径模板中已包含 .xs）
	std::string searchName = moduleName;
	// 如果用户传入的模块名已带 .xs，去掉后缀用于搜索
	if (searchName.size() > 3 && searchName.substr(searchName.size() - 3) == ".xs")
	{
		searchName = searchName.substr(0, searchName.size() - 3);
	}

	std::string filePath = SearchModulePath(this, searchName);
	if (filePath.empty())
	{
		// 搜索失败，移除占位值
		Value nilValue;
		nilValue.type = OP_TYPE_NIL;
		nilValue.iIntValue = 0;
		setTableValue(mModuleTable, ConstructValue(modName.c_str()), nilValue);
		ExecError("module '%s' not found", moduleName);
		return trueValue; // 不会执行到这里，ExecError 会跳转
	}

	// 编译模块文件
	FuncState* mainFunc = CompileFile(filePath);
	if (!mainFunc)
	{
		// 编译失败，移除占位值
		Value nilValue;
		nilValue.type = OP_TYPE_NIL;
		nilValue.iIntValue = 0;
		setTableValue(mModuleTable, ConstructValue(modName.c_str()), nilValue);
		ExecError("failed to compile module '%s'", moduleName);
		return trueValue; // 不会执行到这里
	}

	// 执行模块主函数
	Value fValue = ConstructValue(mainFunc);
	std::string errorDesc;
	if (ProtectCallFunction(fValue.func, 0, errorDesc) != 0)
	{
		// 执行失败，移除占位值
		Value nilValue;
		nilValue.type = OP_TYPE_NIL;
		nilValue.iIntValue = 0;
		setTableValue(mModuleTable, ConstructValue(modName.c_str()), nilValue);
		ExecError("failed to load module '%s': %s", moduleName, errorDesc.c_str());
		return trueValue; // 不会执行到这里
	}

	// 从 mRegValue[0] 读取模块返回值
	Value returnValue = mRegValue[0];

	// 若返回值为 nil，则使用 true 作为最终缓存值
	if (returnValue.type == OP_TYPE_NIL)
	{
		returnValue = trueValue;
	}

	// 灏嗘渶缁堝€煎瓨鍏?mModuleTable锛堣鐩栧崰浣嶅€硷級
	setTableValue(mModuleTable, ConstructValue(modName.c_str()), returnValue);

	return returnValue;
}
