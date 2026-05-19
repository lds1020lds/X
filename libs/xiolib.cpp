#include "XScriptVM.h"
#include "xiolib.h"
#include <errno.h>
#include <windows.h>
#include <direct.h>
#include <string>
#include <thread>
#include <vector>

void init_io_lib()
{

	std::vector<HostFunction> iofuncVec;
	iofuncVec.push_back(HostFunction("input", host_io_input));
	iofuncVec.push_back(HostFunction("output",  host_io_output));
	gScriptVM.RegisterHostLib("io", iofuncVec);


	std::vector<HostFunction> filefuncVec;
	filefuncVec.push_back(HostFunction("open", file_open));
	filefuncVec.push_back(HostFunction("close", file_close));
	filefuncVec.push_back(HostFunction("seek", file_seek));
	filefuncVec.push_back(HostFunction("read", file_read));
	filefuncVec.push_back(HostFunction("write", file_write));
	filefuncVec.push_back(HostFunction("readline", file_readline));
	filefuncVec.push_back(HostFunction("writeline", file_writeline));
	filefuncVec.push_back(HostFunction("writelines", file_writelines));
	filefuncVec.push_back(HostFunction("flush", file_flush));
	filefuncVec.push_back(HostFunction("eof", file_eof));
	filefuncVec.push_back(HostFunction("size", file_size));
	filefuncVec.push_back(HostFunction("tell", file_ftell));
	gScriptVM.RegisterUserClass("File", NULL, filefuncVec);

	gScriptVM.RegisterHostApi("read_file_async", file_read_async);
	gScriptVM.RegisterHostApi("write_file_async", file_write_async);
	gScriptVM.RegisterHostApi("append_file_async", file_append_async);
	gScriptVM.RegisterHostApi("file_exists_async", file_exists_async);
	gScriptVM.RegisterHostApi("list_dir_async", list_dir_async);
	gScriptVM.RegisterHostApi("mkdir_async", mkdir_async);
	gScriptVM.RegisterHostApi("remove_file_async", remove_file_async);

}


void host_io_input(XScriptVM* vm)
{
	char buffer[250] = { 0 };
	if (fgets(buffer, sizeof(buffer), stdin) != 0)
	{
		int len = (int)strlen(buffer);
		if (len > 0 && buffer[len - 1] == '\n')
			buffer[len - 1] = 0;
		vm->setReturnAsStr(buffer, 0);
	}
	else
	{
		vm->setReturnAsStr("", 0);
	}
}

void host_io_output(XScriptVM* vm)
{
	char* varName = 0;
	if (vm->getParamAsString(0, varName))
	{
		fputs(varName, stderr);
	}

}

void file_open(XScriptVM* vm)
{
	CheckParam(file_open, 1, fileName, OP_TYPE_STRING);
	CheckParam(file_open, 2, mode, OP_TYPE_STRING);

	FILE* f = NULL;
	fopen_s(&f, stringRawValue(&fileName), stringRawValue(&mode));
	if (f != NULL)
	{
		vm->setReturnAsUserData("File", f);
	}
	else
	{
		vm->setReturnAsNil(0);
	}
}

void file_close(XScriptVM* vm)
{
	CheckUserTypeParam(file_close, 0, file, FILE, "File");
	fclose(file);
}

void file_seek(XScriptVM* vm)
{
	CheckUserTypeParam(file_seek, 0, file , FILE, "File");
	CheckParam(file_seek, 1, offset, OP_TYPE_INT);
	CheckParam(file_seek, 2, origin, OP_TYPE_INT);
	fseek(file, (long)offset.iIntValue, (int)origin.iIntValue);
}

static unsigned int getFileSize(FILE* file)
{
	long curPos = ftell(file);
	fseek(file, 0, SEEK_END);
	unsigned int size = ftell(file);
	fseek(file, curPos, SEEK_SET);
	return size;
}

void file_size(XScriptVM* vm)
{
	CheckUserTypeParam(file.size, 0, file, FILE, "File");
	unsigned int size = getFileSize(file);
	vm->setReturnAsInt(size);
}

void file_read(XScriptVM* vm)
{
	CheckUserTypeParam(file_read, 0, file, FILE, "File");
	unsigned int readSize = 0;
	if (vm->getNumParam() > 1)
	{
		CheckParam(file_read, 1, size, OP_TYPE_INT);
		readSize = (unsigned int)size.iIntValue;
	}
	else
	{
		readSize = getFileSize(file);
	}
	char* buff = new char[readSize];
	int realSize = (int)fread(buff, 1, readSize, file );
	XString* bufStr = vm->NewXString((const char*)buff, realSize);
	delete buff;
	vm->setReturnAsValue(vm->ConstructValue(bufStr));
}

void file_write(XScriptVM* vm)
{
	CheckUserTypeParam(file_write, 0, file, FILE, "File");
	CheckParam(file_write, 1, bufStr, OP_TYPE_STRING);
	int rawLen = (int)stringRawLen(&bufStr);
	int realSize = (int)fwrite(stringRawValue(&bufStr), 1, rawLen, file);
	vm->setReturnAsInt(realSize);
}
 
void file_readline(XScriptVM* vm)
{
	CheckUserTypeParam(file_readline, 0, file, FILE, "File");
	CheckParam(file_readline, 1, maxSize, OP_TYPE_INT);
	char* buff = new char[(size_t)maxSize.iIntValue];
	
	if (fgets(buff, (int)maxSize.iIntValue, file))
	{

		int len = (int)strlen((const char*)buff);
		if (len > 0 && buff[len - 1] == '\n')
		{
			buff[len - 1] = 0;
			len--;
		}
		XString* bufStr = vm->NewXString((const char*)buff, len);
		vm->setReturnAsValue(vm->ConstructValue(bufStr));
	}
	else
	{
		vm->setReturnAsNil(0);
	}

	delete[]buff;
}


void file_writeline(XScriptVM* vm)
{
	CheckUserTypeParam(file_writeline, 0, file, FILE, "File");
	CheckParam(file_writeline, 1, bufStr, OP_TYPE_STRING);

	int realSize = fputs(stringRawValue(&bufStr), file);
	vm->setReturnAsInt(realSize);
}

void file_writelines(XScriptVM* vm)
{
	CheckUserTypeParam(file_writelines, 0, file, FILE, "File");
	CheckParam(file_writelines, 1, bufStr, OP_TYPE_LIST);

	for (int i = 0; i < bufStr.listData->mListSize; i++)
	{
		if ( IsValueString(&bufStr.listData->mListData[i]) )
			fputs(stringRawValue(&bufStr.listData->mListData[i]), file);
	}
}


void file_flush(XScriptVM* vm)
{
	CheckUserTypeParam(file_flush, 0, file, FILE, "File");
	fflush(file);
}

void file_eof(XScriptVM* vm)
{
	CheckUserTypeParam(file_flush, 0, file, FILE, "File");
	vm->setReturnAsInt(feof(file));
}

void file_ftell(XScriptVM* vm)
{
	CheckUserTypeParam(file_flush, 0, file, FILE, "File");
	vm->setReturnAsInt(ftell(file));
}

struct AsyncFileResult
{
	XPromise* promise;
	bool ok;
	int error;
	int size;
	std::string data;

	AsyncFileResult()
		: promise(NULL)
		, ok(false)
		, error(0)
		, size(0)
	{
	}
};

static void resolve_file_result_on_vm(XScriptVM* vm, const AsyncFileResult& result)
{
	TABLE table = vm->newTable();
	if (result.ok)
	{
		vm->setTableValue(table, "ok", (XInt)1);
		vm->setTableValue(table, "size", (XInt)result.size);
		if (!result.data.empty())
		{
			XString* data = vm->NewXString(result.data.data(), (int)result.data.size());
			vm->setTableValue(table, vm->ConstructValue("data"), vm->ConstructValue(data));
		}
	}
	else
	{
		vm->setTableValue(table, "ok", (XInt)0);
		vm->setTableValue(table, "error", (XInt)result.error);
	}
	if (result.ok)
	{
		result.promise->Resolve(vm->ConstructValue(table));
	}
	else
	{
		result.promise->Reject(vm->ConstructValue(table));
	}
	vm->RemovePendingPromise(result.promise);
	delete result.promise;
}

static AsyncFileResult read_file_raw(XPromise* promise, const std::string& path)
{
	AsyncFileResult result;
	result.promise = promise;
	FILE* file = NULL;
	if (fopen_s(&file, path.c_str(), "rb") != 0 || file == NULL)
	{
		result.error = errno;
		return result;
	}

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (size < 0)
	{
		result.error = errno;
		fclose(file);
		return result;
	}

	result.data.resize((size_t)size);
	if (size > 0)
	{
		result.size = (int)fread(&result.data[0], 1, (size_t)size, file);
		result.data.resize((size_t)result.size);
	}
	else
	{
		result.size = 0;
	}
	fclose(file);
	result.ok = true;
	return result;
}

static AsyncFileResult write_file_raw(XPromise* promise, const std::string& path, const std::string& data, const char* mode)
{
	AsyncFileResult result;
	result.promise = promise;
	FILE* file = NULL;
	if (fopen_s(&file, path.c_str(), mode) != 0 || file == NULL)
	{
		result.error = errno;
		return result;
	}

	if (!data.empty())
		result.size = (int)fwrite(data.data(), 1, data.size(), file);
	else
		result.size = 0;
	fclose(file);
	result.ok = result.size == (int)data.size();
	if (!result.ok)
		result.error = errno;
	return result;
}

void file_read_async(XScriptVM* vm)
{
	CheckParam(read_file_async, 0, path, OP_TYPE_STRING);
	std::string pathCopy = stringRawValue(&path);
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));

	std::weak_ptr<AsyncRuntime> runtime = vm->GetAsyncRuntime();
	std::thread([runtime, promise, pathCopy]() {
		AsyncFileResult result = read_file_raw(promise, pathCopy);
		XScriptVM::QueueAsyncCompletion(runtime, [result](XScriptVM* owner) {
			resolve_file_result_on_vm(owner, result);
		});
	}).detach();
}

void file_write_async(XScriptVM* vm)
{
	CheckParam(write_file_async, 0, path, OP_TYPE_STRING);
	CheckParam(write_file_async, 1, data, OP_TYPE_STRING);
	std::string pathCopy = stringRawValue(&path);
	std::string dataCopy(stringRawValue(&data), (size_t)stringRawLen(&data));
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));

	std::weak_ptr<AsyncRuntime> runtime = vm->GetAsyncRuntime();
	std::thread([runtime, promise, pathCopy, dataCopy]() {
		AsyncFileResult result = write_file_raw(promise, pathCopy, dataCopy, "wb");
		XScriptVM::QueueAsyncCompletion(runtime, [result](XScriptVM* owner) {
			resolve_file_result_on_vm(owner, result);
		});
	}).detach();
}

void file_append_async(XScriptVM* vm)
{
	CheckParam(append_file_async, 0, path, OP_TYPE_STRING);
	CheckParam(append_file_async, 1, data, OP_TYPE_STRING);
	std::string pathCopy = stringRawValue(&path);
	std::string dataCopy(stringRawValue(&data), (size_t)stringRawLen(&data));
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));

	std::weak_ptr<AsyncRuntime> runtime = vm->GetAsyncRuntime();
	std::thread([runtime, promise, pathCopy, dataCopy]() {
		AsyncFileResult result = write_file_raw(promise, pathCopy, dataCopy, "ab");
		XScriptVM::QueueAsyncCompletion(runtime, [result](XScriptVM* owner) {
			resolve_file_result_on_vm(owner, result);
		});
	}).detach();
}

static void resolve_bool_result_on_vm(XScriptVM* vm, XPromise* promise, bool ok, int error)
{
	TABLE table = vm->newTable();
	vm->setTableValue(table, "ok", (XInt)(ok ? 1 : 0));
	if (!ok)
		vm->setTableValue(table, "error", (XInt)error);
	promise->Resolve(vm->ConstructValue(table));
	vm->RemovePendingPromise(promise);
	delete promise;
}

void file_exists_async(XScriptVM* vm)
{
	CheckParam(file_exists_async, 0, path, OP_TYPE_STRING);
	std::string pathCopy = stringRawValue(&path);
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));
	std::weak_ptr<AsyncRuntime> runtime = vm->GetAsyncRuntime();
	std::thread([runtime, promise, pathCopy]() {
		DWORD attr = GetFileAttributesA(pathCopy.c_str());
		bool exists = attr != INVALID_FILE_ATTRIBUTES;
		int error = exists ? 0 : (int)GetLastError();
		XScriptVM::QueueAsyncCompletion(runtime, [promise, exists, error](XScriptVM* owner) {
			resolve_bool_result_on_vm(owner, promise, exists, error);
		});
	}).detach();
}

void mkdir_async(XScriptVM* vm)
{
	CheckParam(mkdir_async, 0, path, OP_TYPE_STRING);
	std::string pathCopy = stringRawValue(&path);
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));
	std::weak_ptr<AsyncRuntime> runtime = vm->GetAsyncRuntime();
	std::thread([runtime, promise, pathCopy]() {
		bool ok = _mkdir(pathCopy.c_str()) == 0 || errno == EEXIST;
		int error = ok ? 0 : errno;
		XScriptVM::QueueAsyncCompletion(runtime, [promise, ok, error](XScriptVM* owner) {
			resolve_bool_result_on_vm(owner, promise, ok, error);
		});
	}).detach();
}

void remove_file_async(XScriptVM* vm)
{
	CheckParam(remove_file_async, 0, path, OP_TYPE_STRING);
	std::string pathCopy = stringRawValue(&path);
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));
	std::weak_ptr<AsyncRuntime> runtime = vm->GetAsyncRuntime();
	std::thread([runtime, promise, pathCopy]() {
		bool ok = remove(pathCopy.c_str()) == 0;
		int error = ok ? 0 : errno;
		XScriptVM::QueueAsyncCompletion(runtime, [promise, ok, error](XScriptVM* owner) {
			resolve_bool_result_on_vm(owner, promise, ok, error);
		});
	}).detach();
}

void list_dir_async(XScriptVM* vm)
{
	CheckParam(list_dir_async, 0, path, OP_TYPE_STRING);
	std::string pathCopy = stringRawValue(&path);
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));
	std::weak_ptr<AsyncRuntime> runtime = vm->GetAsyncRuntime();
	std::thread([runtime, promise, pathCopy]() {
		std::vector<std::string> names;
		std::string pattern = pathCopy;
		if (!pattern.empty())
		{
			char last = pattern[pattern.size() - 1];
			if (last != '/' && last != '\\')
				pattern += "/*";
			else
				pattern += "*";
		}
		WIN32_FIND_DATAA data;
		HANDLE h = FindFirstFileA(pattern.c_str(), &data);
		int error = 0;
		bool ok = h != INVALID_HANDLE_VALUE;
		if (ok)
		{
			do
			{
				if (strcmp(data.cFileName, ".") != 0 && strcmp(data.cFileName, "..") != 0)
					names.push_back(data.cFileName);
			} while (FindNextFileA(h, &data));
			FindClose(h);
		}
		else
		{
			error = (int)GetLastError();
		}
		XScriptVM::QueueAsyncCompletion(runtime, [promise, ok, error, names](XScriptVM* owner) {
			TABLE table = owner->newTable();
			owner->setTableValue(table, "ok", (XInt)(ok ? 1 : 0));
			if (ok)
			{
				ListValue* list = owner->CreateList();
				for (int i = 0; i < (int)names.size(); i++)
					owner->ListAppend(list, owner->ConstructValue(names[i].c_str()));
				owner->setTableValue(table, owner->ConstructValue("files"), owner->ConstructValue(list));
			}
			else
			{
				owner->setTableValue(table, "error", (XInt)error);
			}
			promise->Resolve(owner->ConstructValue(table));
			owner->RemovePendingPromise(promise);
			delete promise;
		});
	}).detach();
}
