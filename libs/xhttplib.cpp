#include "XScriptVM.h"
#include "xhttplib.h"
#include "xbaselib.h"
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <thread>
#pragma comment(lib, "winhttp.lib")

static std::wstring http_utf8_to_wide(const char* s)
{
	if (s == NULL)
		return L"";
	int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
	if (len <= 0)
		len = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
	std::wstring out((size_t)len, 0);
	if (MultiByteToWideChar(CP_UTF8, 0, s, -1, &out[0], len) <= 0)
		MultiByteToWideChar(CP_ACP, 0, s, -1, &out[0], len);
	if (!out.empty() && out[out.size() - 1] == 0)
		out.resize(out.size() - 1);
	return out;
}

static std::string http_wide_to_utf8(const wchar_t* s)
{
	if (s == NULL)
		return "";
	int len = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
	std::string out((size_t)len, 0);
	WideCharToMultiByte(CP_UTF8, 0, s, -1, &out[0], len, NULL, NULL);
	if (!out.empty() && out[out.size() - 1] == 0)
		out.resize(out.size() - 1);
	return out;
}

static std::wstring http_headers_from_table(XScriptVM* vm, const Value& headers)
{
	if (headers.type != OP_TYPE_TABLE)
		return L"";
	std::string raw;
	for (int i = 0; i < headers.tableData->mNodeCapacity; i++)
	{
		TableNode& node = headers.tableData->mNodeData[i];
		if (IsValueNil(&node.value) || node.key.keyVal.type != OP_TYPE_STRING)
			continue;
		raw += stringRawValue(&node.key.keyVal);
		raw += ": ";
		if (node.value.type == OP_TYPE_STRING)
			raw += stringRawValue(&node.value);
		else
			raw += getValueDescString(vm, node.value);
		raw += "\r\n";
	}
	return http_utf8_to_wide(raw.c_str());
}

static void http_fail(XScriptVM* vm, DWORD err)
{
	vm->setReturnAsNil(0);
	vm->setReturnAsInt((XInt)err, 1);
}

struct HttpRawResult
{
	bool ok;
	DWORD error;
	DWORD status;
	std::string body;
	std::string headers;

	HttpRawResult()
		: ok(false)
		, error(0)
		, status(0)
	{
	}
};

static HttpRawResult http_request_raw(const char* method, const std::string& url, const std::string& body, const std::wstring& headers)
{
	HttpRawResult result;
	std::wstring wurl = http_utf8_to_wide(url.c_str());
	URL_COMPONENTS parts;
	memset(&parts, 0, sizeof(parts));
	parts.dwStructSize = sizeof(parts);
	wchar_t host[256];
	wchar_t path[2048];
	memset(host, 0, sizeof(host));
	memset(path, 0, sizeof(path));
	parts.lpszHostName = host;
	parts.dwHostNameLength = sizeof(host) / sizeof(host[0]);
	parts.lpszUrlPath = path;
	parts.dwUrlPathLength = sizeof(path) / sizeof(path[0]);
	parts.dwSchemeLength = (DWORD)-1;
	parts.dwExtraInfoLength = (DWORD)-1;
	if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &parts))
	{
		result.error = GetLastError();
		return result;
	}

	std::wstring object = path;
	if (parts.lpszExtraInfo != NULL && parts.dwExtraInfoLength > 0)
		object.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
	if (object.empty())
		object = L"/";

	HINTERNET session = WinHttpOpen(L"XScript/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (session != NULL)
		WinHttpSetTimeouts(session, 5000, 5000, 5000, 5000);
	if (session == NULL)
	{
		result.error = GetLastError();
		return result;
	}
	HINTERNET connect = WinHttpConnect(session, std::wstring(host, parts.dwHostNameLength).c_str(), parts.nPort, 0);
	if (connect == NULL)
	{
		result.error = GetLastError();
		WinHttpCloseHandle(session);
		return result;
	}
	DWORD flags = (parts.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
	std::wstring wmethod = http_utf8_to_wide(method);
	HINTERNET request = WinHttpOpenRequest(connect, wmethod.c_str(), object.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
	if (request == NULL)
	{
		result.error = GetLastError();
		WinHttpCloseHandle(connect);
		WinHttpCloseHandle(session);
		return result;
	}

	DWORD bodySize = (DWORD)body.size();
	BOOL ok = WinHttpSendRequest(request,
		headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str(),
		headers.empty() ? 0 : (DWORD)-1,
		bodySize > 0 ? (LPVOID)body.data() : WINHTTP_NO_REQUEST_DATA,
		bodySize,
		bodySize,
		0);
	if (ok)
		ok = WinHttpReceiveResponse(request, NULL);
	if (!ok)
	{
		result.error = GetLastError();
		WinHttpCloseHandle(request);
		WinHttpCloseHandle(connect);
		WinHttpCloseHandle(session);
		return result;
	}

	DWORD status = 0;
	DWORD statusSize = sizeof(status);
	WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
	result.status = status;

	DWORD rawSize = 0;
	WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &rawSize, WINHTTP_NO_HEADER_INDEX);
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && rawSize > 0)
	{
		std::vector<wchar_t> raw((size_t)rawSize / sizeof(wchar_t) + 1);
		if (WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, &raw[0], &rawSize, WINHTTP_NO_HEADER_INDEX))
			result.headers = http_wide_to_utf8(&raw[0]);
	}

	DWORD available = 0;
	while (WinHttpQueryDataAvailable(request, &available) && available > 0)
	{
		std::vector<char> buffer((size_t)available + 1);
		DWORD read = 0;
		if (!WinHttpReadData(request, &buffer[0], available, &read))
			break;
		result.body.append(&buffer[0], read);
	}

	WinHttpCloseHandle(request);
	WinHttpCloseHandle(connect);
	WinHttpCloseHandle(session);
	result.ok = true;
	return result;
}

static void http_request_impl(XScriptVM* vm, const char* method, const char* url, const char* body, const Value& headers)
{
	std::wstring wurl = http_utf8_to_wide(url);
	URL_COMPONENTS parts;
	memset(&parts, 0, sizeof(parts));
	parts.dwStructSize = sizeof(parts);
	wchar_t host[256];
	wchar_t path[2048];
	memset(host, 0, sizeof(host));
	memset(path, 0, sizeof(path));
	parts.lpszHostName = host;
	parts.dwHostNameLength = sizeof(host) / sizeof(host[0]);
	parts.lpszUrlPath = path;
	parts.dwUrlPathLength = sizeof(path) / sizeof(path[0]);
	parts.dwSchemeLength = (DWORD)-1;
	parts.dwExtraInfoLength = (DWORD)-1;
	if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &parts))
	{
		http_fail(vm, GetLastError());
		return;
	}

	std::wstring object = path;
	if (parts.lpszExtraInfo != NULL && parts.dwExtraInfoLength > 0)
		object.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
	if (object.empty())
		object = L"/";

	HINTERNET session = WinHttpOpen(L"XScript/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (session != NULL)
		WinHttpSetTimeouts(session, 5000, 5000, 5000, 5000);
	if (session == NULL)
	{
		http_fail(vm, GetLastError());
		return;
	}
	HINTERNET connect = WinHttpConnect(session, std::wstring(host, parts.dwHostNameLength).c_str(), parts.nPort, 0);
	if (connect == NULL)
	{
		DWORD err = GetLastError();
		WinHttpCloseHandle(session);
		http_fail(vm, err);
		return;
	}
	DWORD flags = (parts.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
	std::wstring wmethod = http_utf8_to_wide(method);
	HINTERNET request = WinHttpOpenRequest(connect, wmethod.c_str(), object.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
	if (request == NULL)
	{
		DWORD err = GetLastError();
		WinHttpCloseHandle(connect);
		WinHttpCloseHandle(session);
		http_fail(vm, err);
		return;
	}

	std::wstring wheaders = http_headers_from_table(vm, headers);
	DWORD bodySize = body != NULL ? (DWORD)strlen(body) : 0;
	BOOL ok = WinHttpSendRequest(request,
		wheaders.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : wheaders.c_str(),
		wheaders.empty() ? 0 : (DWORD)-1,
		bodySize > 0 ? (LPVOID)body : WINHTTP_NO_REQUEST_DATA,
		bodySize,
		bodySize,
		0);
	if (ok)
		ok = WinHttpReceiveResponse(request, NULL);
	if (!ok)
	{
		DWORD err = GetLastError();
		WinHttpCloseHandle(request);
		WinHttpCloseHandle(connect);
		WinHttpCloseHandle(session);
		http_fail(vm, err);
		return;
	}

	DWORD status = 0;
	DWORD statusSize = sizeof(status);
	WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);

	DWORD rawSize = 0;
	WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &rawSize, WINHTTP_NO_HEADER_INDEX);
	std::string rawHeaders;
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && rawSize > 0)
	{
		std::vector<wchar_t> raw((size_t)rawSize / sizeof(wchar_t) + 1);
		if (WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, &raw[0], &rawSize, WINHTTP_NO_HEADER_INDEX))
			rawHeaders = http_wide_to_utf8(&raw[0]);
	}

	std::string response;
	DWORD available = 0;
	while (WinHttpQueryDataAvailable(request, &available) && available > 0)
	{
		std::vector<char> buffer((size_t)available + 1);
		DWORD read = 0;
		if (!WinHttpReadData(request, &buffer[0], available, &read))
			break;
		response.append(&buffer[0], read);
	}

	TABLE ret = vm->newTable();
	vm->setTableValue(ret, "status", (XInt)status);
	vm->setTableValue(ret, "body", response.c_str());
	vm->setTableValue(ret, "headers", rawHeaders.c_str());
	vm->setReturnAsTable(ret);

	WinHttpCloseHandle(request);
	WinHttpCloseHandle(connect);
	WinHttpCloseHandle(session);
}

void init_http_lib()
{
	std::vector<HostFunction> funcs;
	funcs.push_back(HostFunction("request", xhttp_request));
	funcs.push_back(HostFunction("get", xhttp_get));
	funcs.push_back(HostFunction("post", xhttp_post));
	gScriptVM.RegisterHostLib("http", funcs);
	gScriptVM.RegisterHostApi("http_get", xhttp_get_async);
	gScriptVM.RegisterHostApi("http_post", xhttp_post_async);
}

void xhttp_request(XScriptVM* vm)
{
	CheckParam(http.request, 0, method, OP_TYPE_STRING);
	CheckParam(http.request, 1, url, OP_TYPE_STRING);
	const char* body = NULL;
	if (vm->getNumParam() > 2)
	{
		Value bodyValue = vm->getParamValue(2);
		if (bodyValue.type == OP_TYPE_STRING)
			body = stringRawValue(&bodyValue);
	}
	Value headers;
	if (vm->getNumParam() > 3)
		headers = vm->getParamValue(3);
	http_request_impl(vm, stringRawValue(&method), stringRawValue(&url), body, headers);
}

void xhttp_get(XScriptVM* vm)
{
	CheckParam(http.get, 0, url, OP_TYPE_STRING);
	Value headers;
	if (vm->getNumParam() > 1)
		headers = vm->getParamValue(1);
	http_request_impl(vm, "GET", stringRawValue(&url), NULL, headers);
}

void xhttp_post(XScriptVM* vm)
{
	CheckParam(http.post, 0, url, OP_TYPE_STRING);
	CheckParam(http.post, 1, body, OP_TYPE_STRING);
	Value headers;
	if (vm->getNumParam() > 2)
		headers = vm->getParamValue(2);
	http_request_impl(vm, "POST", stringRawValue(&url), stringRawValue(&body), headers);
}

static void resolve_http_result_on_vm(XScriptVM* vm, XPromise* promise, const HttpRawResult& result)
{
	TABLE table = vm->newTable();
	if (result.ok)
	{
		vm->setTableValue(table, "ok", (XInt)1);
		vm->setTableValue(table, "status", (XInt)result.status);
		vm->setTableValue(table, "body", result.body.c_str());
		vm->setTableValue(table, "headers", result.headers.c_str());
	}
	else
	{
		vm->setTableValue(table, "ok", (XInt)0);
		vm->setTableValue(table, "error", (XInt)result.error);
	}

	promise->Resolve(vm->ConstructValue(table));
	vm->RemovePendingPromise(promise);
	delete promise;
}

void xhttp_get_async(XScriptVM* vm)
{
	CheckParam(http_get, 0, url, OP_TYPE_STRING);
	Value headers;
	if (vm->getNumParam() > 1)
		headers = vm->getParamValue(1);

	std::string urlCopy = stringRawValue(&url);
	std::wstring headersCopy = http_headers_from_table(vm, headers);
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));

	std::weak_ptr<AsyncRuntime> runtime = vm->GetAsyncRuntime();
	std::thread([runtime, promise, urlCopy, headersCopy]() {
		HttpRawResult result = http_request_raw("GET", urlCopy, "", headersCopy);
		XScriptVM::QueueAsyncCompletion(runtime, [promise, result](XScriptVM* owner) {
			resolve_http_result_on_vm(owner, promise, result);
		});
	}).detach();
}

void xhttp_post_async(XScriptVM* vm)
{
	CheckParam(http_post, 0, url, OP_TYPE_STRING);
	CheckParam(http_post, 1, body, OP_TYPE_STRING);
	Value headers;
	if (vm->getNumParam() > 2)
		headers = vm->getParamValue(2);

	std::string urlCopy = stringRawValue(&url);
	std::string bodyCopy = stringRawValue(&body);
	std::wstring headersCopy = http_headers_from_table(vm, headers);
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));

	std::weak_ptr<AsyncRuntime> runtime = vm->GetAsyncRuntime();
	std::thread([runtime, promise, urlCopy, bodyCopy, headersCopy]() {
		HttpRawResult result = http_request_raw("POST", urlCopy, bodyCopy, headersCopy);
		XScriptVM::QueueAsyncCompletion(runtime, [promise, result](XScriptVM* owner) {
			resolve_http_result_on_vm(owner, promise, result);
		});
	}).detach();
}
