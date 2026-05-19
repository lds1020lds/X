#include "XScriptVM.h"
#include "xoslib.h"
#include <vector>
#include <string>
#include <direct.h>

void init_os_lib()
{
	std::vector<HostFunction> osfuncVec;
	osfuncVec.push_back(HostFunction("listdir", xos_listdir));
	osfuncVec.push_back(HostFunction("remove", xos_remove));
	osfuncVec.push_back(HostFunction("rmdir", xos_rmdir));
	osfuncVec.push_back(HostFunction("mkdir", xos_mkdir));
	osfuncVec.push_back(HostFunction("isfile", xos_isfile));
	osfuncVec.push_back(HostFunction("exists", xos_exists));

	osfuncVec.push_back(HostFunction("getsize", xos_getsize));
	osfuncVec.push_back(HostFunction("getatime", xos_getatime));
	osfuncVec.push_back(HostFunction("getmtime", xos_getmtime));
	osfuncVec.push_back(HostFunction("getctime", xos_getctime));

	osfuncVec.push_back(HostFunction("getpwd", xos_getpwd));
	osfuncVec.push_back(HostFunction("setpwd", xos_setpwd));
	osfuncVec.push_back(HostFunction("exec", xos_process_exec));
	osfuncVec.push_back(HostFunction("wait", xos_process_wait));
	osfuncVec.push_back(HostFunction("system", xos_system));
	osfuncVec.push_back(HostFunction("copydir", xos_copyDir));
	osfuncVec.push_back(HostFunction("copyfile", xos_copyFile));
	gScriptVM.RegisterHostLib("os", osfuncVec);
}

void GenerateFileList(const std::string& dir, std::vector<std::string>& fileList, std::vector<std::string>&  dirList)
{
	HANDLE file;
	WIN32_FIND_DATA fileData;
	std::string curDir = dir;
	curDir += "/*";
	dirList.push_back(dir);
	file = FindFirstFile(curDir.c_str(), &fileData);

	while (FindNextFile(file, &fileData))
	{
		if (strcmp(fileData.cFileName, ".") == 0 ||
			strcmp(fileData.cFileName, "..") == 0)
		{
			continue;
		}

		int isDir = fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

		if (isDir != 0)
		{
			std::string subDir = dir;
			subDir += "/";
			subDir += fileData.cFileName;
			fileList.push_back(subDir);
			GenerateFileList(subDir, fileList, dirList);
		}
		else
		{
			std::string filePath = dir;
			filePath += "/";
			filePath += fileData.cFileName;
			fileList.push_back(filePath);
		}
	}
	FindClose(file);
}

bool DeleteDir(const std::string& dir)
{
	HANDLE file;
	WIN32_FIND_DATA fileData;
	std::string curDir = dir;
	curDir += "/*";

	file = FindFirstFile(curDir.c_str(), &fileData);

	while (FindNextFile(file, &fileData))
	{
		if (strcmp(fileData.cFileName, ".") == 0 ||
			strcmp(fileData.cFileName, "..") == 0)
		{
			continue;
		}

		int isDir = fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

		if (isDir != 0)
		{
			std::string subDir = dir;
			subDir += "/";
			subDir += fileData.cFileName;
			if (!DeleteDir(subDir))
			{
				FindClose(file);
				return false;
			}
		}
		else
		{
			std::string filePath = dir;
			filePath += "/";
			filePath += fileData.cFileName;
			if (DeleteFile(filePath.c_str()) == 0)
			{
				FindClose(file);
				return false;
			}
		}
	}
	FindClose(file);
	BOOL ret = RemoveDirectory(dir.c_str());
	return ret == TRUE;
}

void xos_listdir(XScriptVM* vm)
{
	CheckParam(xos_listdir, 0, dirName, OP_TYPE_STRING);
	std::vector<std::string> fileList;
	std::vector<std::string> dirList;
	GenerateFileList(stringRawValue(&dirName), fileList, dirList);
	if (fileList.size() > 0)
	{
		ListValue* List = vm->CreateList();
		vm->ListResize(List, (XInt)fileList.size());
		for (int i = 0; i < (int)fileList.size(); i++)
		{
			List->mListData[i] = vm->ConstructValue(fileList[i].c_str());
		}

		vm->setReturnAsValue(vm->ConstructValue(List));
	}
	else
	{
		vm->setReturnAsNil(0);
	}

	if (dirList.size() > 0)
	{
		ListValue* List = vm->CreateList();
		vm->ListResize(List, (XInt)dirList.size());
		for (int i = 0; i < (int)dirList.size(); i++)
		{
			List->mListData[i] = vm->ConstructValue(dirList[i].c_str());
		}

		vm->setReturnAsValue(vm->ConstructValue(List), 1);
	}
	else
	{
		vm->setReturnAsNil(1);
	}
}

void xos_getpwd(XScriptVM* vm)
{
	char pwd[256] = { 0 };
	GetCurrentDirectory(256, pwd);
	vm->setReturnAsStr(pwd);
}

void xos_setpwd(XScriptVM* vm)
{
	CheckParam(xos_setpwd, 0, pwd, OP_TYPE_STRING);
	SetCurrentDirectory(stringRawValue(&pwd));
}

void xos_system(XScriptVM* vm)
{
	CheckParam(xos_system, 0, cmd, OP_TYPE_STRING);
	system(stringRawValue(&cmd));
}

void xos_remove(XScriptVM* vm)
{
	CheckParam(xos_remove, 0, fileName, OP_TYPE_STRING);
	vm->setReturnAsInt(DeleteFile(stringRawValue(&fileName)));
}

void xos_rmdir(XScriptVM* vm)
{
	CheckParam(xos_rmdir, 0, dir, OP_TYPE_STRING);

	WIN32_FIND_DATA fileData;
	HANDLE file = FindFirstFile(stringRawValue(&dir), &fileData);
	if (file != INVALID_HANDLE_VALUE)
	{
		vm->setReturnAsInt(DeleteDir(stringRawValue(&dir)));
	}
	else
	{
		vm->setReturnAsInt(1);
	}
	FindClose(file);

}

void xos_mkdir(XScriptVM* vm)
{
	CheckParam(xos_mkdir, 0, dir, OP_TYPE_STRING);
	vm->setReturnAsInt(CreateDirectory(stringRawValue(&dir), NULL));
}

void xos_isfile(XScriptVM* vm)
{
	CheckParam(xos_isfile, 0, fileName, OP_TYPE_STRING);
	WIN32_FIND_DATA fileData;
	HANDLE file = FindFirstFile(stringRawValue(&fileName), &fileData);
	vm->setReturnAsInt(file != INVALID_HANDLE_VALUE && !(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
	FindClose(file);
}

void xos_isdir(XScriptVM* vm)
{
	CheckParam(xos_isdir, 0, fileName, OP_TYPE_STRING);
	WIN32_FIND_DATA fileData;
	HANDLE file = FindFirstFile(stringRawValue(&fileName), &fileData);
	vm->setReturnAsInt(file != INVALID_HANDLE_VALUE && (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
	FindClose(file);
}

void xos_exists(XScriptVM* vm)
{
	CheckParam(xos_exists, 0, fileName, OP_TYPE_STRING);
	WIN32_FIND_DATA fileData;
	HANDLE file = FindFirstFile(stringRawValue(&fileName), &fileData);
	vm->setReturnAsInt(file != INVALID_HANDLE_VALUE);
	FindClose(file);
}

void xos_getsize(XScriptVM* vm)
{
	CheckParam(xos_getsize, 0, fileName, OP_TYPE_STRING);
	struct _stat info = {};
	_stat(stringRawValue(&fileName), &info);
	vm->setReturnAsInt((XInt)info.st_size);
}

void xos_getctime(XScriptVM* vm)
{
	CheckParam(xos_getsize, 0, fileName, OP_TYPE_STRING);
	struct _stat info = {};
	_stat(stringRawValue(&fileName), &info);
	vm->setReturnAsInt((XInt)info.st_ctime);
}

void xos_getmtime(XScriptVM* vm)
{
	CheckParam(xos_getsize, 0, fileName, OP_TYPE_STRING);
	struct _stat info = {};
	_stat(stringRawValue(&fileName), &info);
	vm->setReturnAsInt((XInt)info.st_mtime);
}

void xos_getatime(XScriptVM* vm)
{
	CheckParam(xos_getsize, 0, fileName, OP_TYPE_STRING);
	struct _stat info = {};
	_stat(stringRawValue(&fileName), &info);
	vm->setReturnAsInt((XInt)info.st_atime);
}

/**copy from Google-glog*/
bool SafeFNMatch(const char* pattern, size_t patt_len, const char* str, size_t str_len)
{
	size_t p = 0;
	size_t s = 0;
	while (1)
	{
		if (p == patt_len && s == str_len)
			return true;
		if (p == patt_len)
			return false;
		if (s == str_len)
			return p + 1 == patt_len && pattern[p] == '*';
		if (pattern[p] == str[s] || pattern[p] == '?')
		{
			p += 1;
			s += 1;
			continue;
		}
		if (pattern[p] == '*')
		{
			if (p + 1 == patt_len) return true;
			do
			{
				if (SafeFNMatch(pattern + (p + 1), patt_len - (p + 1), str + s, str_len - s))
				{
					return true;
				}
				s += 1;
			} while (s != str_len);

			return false;
		}

		return false;
	}
}

int fnmatch(const char *pattern, const char *name)
{
	return (SafeFNMatch(pattern, strlen(pattern), name, strlen(name)));
}

static bool CopyDir(const char * pSrc, const char *pDes, const char* filter, std::vector<std::string>& excludeList )
{
	bool bC = CreateDirectory(pDes, NULL) == TRUE;
	char dir[MAX_PATH] = { 0 };

	char srcFileName[MAX_PATH] = { 0 };
	char desFileName[MAX_PATH] = { 0 };

	char *str = "/*.*";
	if (filter != NULL)
	{
		strncpy_s(dir, pSrc, MAX_PATH);
		strncat_s(dir, "/", MAX_PATH);
		strncat_s(dir, filter, MAX_PATH);
		
	}
	else
	{
		strncpy_s(dir, pSrc, MAX_PATH);
		strncat_s(dir, str, MAX_PATH);
	}
	
	

	HANDLE hFile;
	WIN32_FIND_DATA fileinfo;
	if ((hFile = FindFirstFile(dir, &fileinfo)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			bool hasExclude = false;
			for (int i = 0; i < (int)excludeList.size(); i++)
			{
				if (fnmatch(excludeList[i].c_str(), fileinfo.cFileName))
				{
					hasExclude = true;
				}
			}

			if (!hasExclude)
			{
				strncpy_s(srcFileName, pSrc, MAX_PATH);
				strncat_s(srcFileName, "/", MAX_PATH);
				strncat_s(srcFileName, fileinfo.cFileName, MAX_PATH);
				strncpy_s(desFileName, pDes, MAX_PATH);
				strncat_s(desFileName, "/", MAX_PATH);
				strncat_s(desFileName, fileinfo.cFileName, MAX_PATH);
				if (!(fileinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					if (!CopyFile(srcFileName, desFileName, FALSE))
					{
						FindClose(hFile);
						return false;
					}
				}
				else
				{
					if (strcmp(fileinfo.cFileName, ".") != 0 && strcmp(fileinfo.cFileName, "..") != 0)
					{
						if (!CopyDir(srcFileName, desFileName, filter, excludeList))
						{
							FindClose(hFile);
							return false;
						}
					}
				}
			}
			
		} while (FindNextFile(hFile, &fileinfo) != 0);

		FindClose(hFile);
	}

	return true;
}

void xos_copyFile(XScriptVM* vm)
{
	CheckParam(xos_copyFile, 0, src, OP_TYPE_STRING);
	CheckParam(xos_copyFile, 1, desc, OP_TYPE_STRING);

	bool ret = CopyFile(stringRawValue(&src), stringRawValue(&desc), false) == TRUE;
	vm->setReturnAsInt(ret);
}

void xos_process_exec(XScriptVM* vm)
{
	CheckParam(xos_process_exec, 0, src, OP_TYPE_STRING);
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcess(NULL,  
		(char*)stringRawValue(&src), 
		NULL,         
		NULL,         
		FALSE,        
		0,            
		NULL,         
		NULL,         
		&si,          
		&pi)          
		)
	{
		vm->setReturnAsNil(0);
	}
	else
	{
		vm->setReturnAsInt((XInt)(intptr_t)pi.hProcess);
	}

	

}

void xos_process_wait(XScriptVM* vm)
{
	CheckParam(xos_copyDir, 0, process, OP_TYPE_INT);
	XInt waitTime = INFINITE;
	if (vm->getNumParam() > 1)
	{
		CheckParam(xos_copyDir, 0, t, OP_TYPE_INT);
		waitTime = t.iIntValue;
	}

	vm->setReturnAsInt(WaitForSingleObject((HANDLE)(intptr_t)process.iIntValue, (DWORD)waitTime));
}

void xos_copyDir(XScriptVM* vm)
{
	CheckParam(xos_copyDir, 0, src, OP_TYPE_STRING);
	CheckParam(xos_copyDir, 1, desc, OP_TYPE_STRING);

	std::vector<std::string> excludeList;
	const char* szfilter = NULL;
	if (vm->getNumParam() > 2)
	{
		if (vm->getParamValue(2).type != OP_TYPE_NIL)
		{
			CheckParam(xos_copyDir, 2, filter, OP_TYPE_STRING);
			szfilter = stringRawValue(&filter);
		}
	}

	if (vm->getNumParam() > 3)
	{
		CheckParam(xos_copyDir, 3, exclude, OP_TYPE_LIST);
		for (int i = 0; i < exclude.listData->mListSize; i++)
		{
			if (IsValueString(&exclude.listData->mListData[i]))
			excludeList.push_back(stringRawValue(&exclude.listData->mListData[i]));
		}	
	}
	
	bool ret = CopyDir(stringRawValue(&src), stringRawValue(&desc), szfilter, excludeList);
	vm->setReturnAsInt(ret);
}





