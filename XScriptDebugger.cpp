#include <WinSock2.h>
#include <WS2tcpip.h>
#include "XScriptDebugger.h"
#include "XScriptVM.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")

static SOCKET ToSocket(uintptr_t value)
{
	return (SOCKET)value;
}

static uintptr_t FromSocket(SOCKET value)
{
	return (uintptr_t)value;
}


static bool IsInternalIdent(const std::string& name)
{
	return name.compare(0, strlen(XSCRIPT_INTERNAL_IDENT_PREFIX), XSCRIPT_INTERNAL_IDENT_PREFIX) == 0;
}

static bool ParseBracketIndex(const std::string& name, XInt& index)
{
	if (name.size() < 3 || name[0] != '[' || name[name.size() - 1] != ']')
		return false;

	std::string content = name.substr(1, name.size() - 2);
	char* endPtr = NULL;
	long long value = strtoll(content.c_str(), &endPtr, 10);
	if (endPtr == content.c_str() || *endPtr != '\0')
		return false;

	index = (XInt)value;
	return true;
}

XScriptDebugger::XScriptDebugger()
	: mVM(NULL)
	, mEnabled(false)
	, mClientConnected(false)
	, mPaused(false)
	, mWinsockStarted(false)
	, mStepMode(StepNone)
	, mStepDepth(0)
	, mBreakOnStateSwitch(false)
	, mPort(0)
	, mListenSocket(FromSocket(INVALID_SOCKET))
	, mClientSocket(FromSocket(INVALID_SOCKET))
	, mRecvBuffer()
	, mNextVarRef(1)
{
}

XScriptDebugger::~XScriptDebugger()
{
	StopServer();
}

void XScriptDebugger::AttachVM(XScriptVM* vm)
{
	mVM = vm;
}

bool XScriptDebugger::StartServer(int port, bool waitDebugger)
{
	mPort = port;
	mEnabled = true;

	WSADATA wsaData;
	if (!mWinsockStarted)
	{
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			fprintf(stderr, "XScript debug server WSAStartup failed\n");
			mEnabled = false;
			return false;
		}
		mWinsockStarted = true;
	}

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		fprintf(stderr, "XScript debug server socket failed\n");
		mEnabled = false;
		return false;
	}

	int reuse = 1;
	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons((u_short)port);

	if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		fprintf(stderr, "XScript debug server bind failed on port %d\n", port);
		closesocket(listenSocket);
		mEnabled = false;
		return false;
	}

	if (listen(listenSocket, 1) == SOCKET_ERROR)
	{
		fprintf(stderr, "XScript debug server listen failed\n");
		closesocket(listenSocket);
		mEnabled = false;
		return false;
	}

	mListenSocket = FromSocket(listenSocket);

	u_long nonBlocking = 1;
	ioctlsocket(listenSocket, FIONBIO, &nonBlocking);

	fprintf(stderr, "XScript debug server listening on 127.0.0.1:%d\n", port);
	fflush(stderr);

	if (waitDebugger)
	{
		fprintf(stderr, "XScript debug server waiting for VSCode adapter...\n");
		fflush(stderr);
		while (mEnabled && !mClientConnected)
		{
			fd_set readSet;
			FD_ZERO(&readSet);
			FD_SET(listenSocket, &readSet);
			int sel = select(0, &readSet, NULL, NULL, NULL);
			if (sel > 0)
			{
				TryAcceptClient();
			}
			else
			{
				fprintf(stderr, "XScript debug server wait failed\n");
				StopServer();
				return false;
			}
		}
		
		// 连接成功后，等待VSCode发送configurationDone信号
		if (mClientConnected)
		{
			fprintf(stderr, "XScript debug server waiting for configurationDone signal...\n");
			fflush(stderr);
			
			// 等待VSCode发送configurationDone信号（最长等待5秒）
			bool receivedConfigurationDone = false;
			for (int i = 0; i < 100 && mEnabled && !receivedConfigurationDone; i++)
			{
				XScriptDebugCommand command;
				if (ReadCommand(command, false))
				{
					if (command.command == "configurationDone")
					{
						receivedConfigurationDone = true;
						SendResponse(command, true);
						break;
					}
					DispatchRunningCommand(command);
				}
				Sleep(50);
			}
			
			if (receivedConfigurationDone)
			{
				fprintf(stderr, "XScript debug server received configurationDone, ready for execution\n");
			}
			else
			{
				fprintf(stderr, "XScript debug server timeout waiting for configurationDone, proceeding with execution\n");
			}
			fflush(stderr);
		}
	}

	return true;
}

void XScriptDebugger::StopServer()
{
	mStepMode = StepNone;
	mBreakOnStateSwitch = false;
	mPaused = false;
	mClientConnected = false;
	mEnabled = false;

	if (ToSocket(mClientSocket) != INVALID_SOCKET)
	{
		closesocket(ToSocket(mClientSocket));
		mClientSocket = FromSocket(INVALID_SOCKET);
	}
	if (ToSocket(mListenSocket) != INVALID_SOCKET)
	{
		closesocket(ToSocket(mListenSocket));
		mListenSocket = FromSocket(INVALID_SOCKET);
	}
	if (mWinsockStarted)
	{
		WSACleanup();
		mWinsockStarted = false;
	}
}

void XScriptDebugger::ClearBreakpoints(const std::string& file)
{
	std::string normalized = NormalizePath(file);
	unsigned int hash = HashFilePath(normalized);
	mBreakpoints[normalized].clear();
	mBreakpointsByHash[hash].clear();
	mBreakpointConditions[hash].clear();

	// Release pre-compiled condition FuncStates (restore to GC-collectible)
	std::map<int, FuncState*>& compiledMap = mCompiledConditions[hash];
	for (std::map<int, FuncState*>::iterator it = compiledMap.begin(); it != compiledMap.end(); ++it)
	{
		if (it->second != NULL)
			GC_SetWhite(it->second);
	}
	compiledMap.clear();
}

void XScriptDebugger::SetBreakpoints(const std::string& file, const std::vector<int>& lines, const std::vector<std::string>& conditions)
{
	std::string normalized = NormalizePath(file);
	unsigned int hash = HashFilePath(normalized);

	std::set<int>& fileBreakpoints = mBreakpoints[normalized];
	fileBreakpoints.clear();
	std::set<int>& hashBreakpoints = mBreakpointsByHash[hash];
	hashBreakpoints.clear();
	std::map<int, std::string>& condMap = mBreakpointConditions[hash];
	condMap.clear();

	// Release old pre-compiled conditions (restore to GC-collectible)
	std::map<int, FuncState*>& compiledMap = mCompiledConditions[hash];
	for (std::map<int, FuncState*>::iterator it = compiledMap.begin(); it != compiledMap.end(); ++it)
	{
		if (it->second != NULL)
			GC_SetWhite(it->second);
	}
	compiledMap.clear();

	for (size_t i = 0; i < lines.size(); i++)
	{
		int line = lines[i];
		fileBreakpoints.insert(line);
		hashBreakpoints.insert(line);
		if (i < conditions.size() && !conditions[i].empty())
		{
			condMap[line] = conditions[i];

			// Pre-compile condition expression and mark as GC-fixed
			if (mVM)
			{
				FuncState* compiled = mVM->CompileConditionString(conditions[i].c_str());
				if (compiled != NULL)
				{
					GC_SetFixed(compiled);
					compiledMap[line] = compiled;
				}
			}
		}
	}
}

bool XScriptDebugger::CheckBreakpointHit(unsigned int fileHash, int line)
{
	// 检查是否命中断点行
	std::map<unsigned int, std::set<int> >::const_iterator it = mBreakpointsByHash.find(fileHash);
	if (it == mBreakpointsByHash.end() || it->second.find(line) == it->second.end())
		return false;

	// 命中断点行，检查是否有条件
	std::map<unsigned int, std::map<int, std::string> >::const_iterator condIt = mBreakpointConditions.find(fileHash);
	if (condIt != mBreakpointConditions.end())
	{
		std::map<int, std::string>::const_iterator lineCondIt = condIt->second.find(line);
		if (lineCondIt != condIt->second.end() && !lineCondIt->second.empty())
		{
			// 该行有条件断点，评估条件
			if (mVM)
			{
				TableValue* evalEnv = BuildEvalEnvTable(0);
				if (evalEnv != NULL)
					mVM->SetEnvTable(evalEnv);

				bool condResult = false;

				// 优先使用预编译的条件表达式（快速路径）
				std::map<unsigned int, std::map<int, FuncState*> >::const_iterator compiledIt = mCompiledConditions.find(fileHash);
				if (compiledIt != mCompiledConditions.end())
				{
					std::map<int, FuncState*>::const_iterator lineIt = compiledIt->second.find(line);
					if (lineIt != compiledIt->second.end() && lineIt->second != NULL)
						condResult = mVM->EvalCompiledCondition(lineIt->second);
					else
						condResult = mVM->EvalConditionString(lineCondIt->second.c_str());
				}
				else
				{
					condResult = mVM->EvalConditionString(lineCondIt->second.c_str());
				}

				mVM->SetEnvTable(NULL);

				if (!condResult)
					return false;
			}
		}
	}

	return true;
}

bool XScriptDebugger::ShouldPause(unsigned int fileHash, int line, int stackDepth)
{
	// 协程切换后的下一行只在单步状态下中断
	
	if (mStepMode != StepNone && mBreakOnStateSwitch)
	{
		mBreakOnStateSwitch = false;
		return true;
	}
	// 步进模式优先（不受条件断点影响）
	if (mStepMode == StepIn)
		return true;
	if (mStepMode == StepOver && stackDepth <= mStepDepth)
		return true;
	if (mStepMode == StepOut && stackDepth < mStepDepth)
		return true;

	// 检查断点命中（含条件评估）
	return CheckBreakpointHit(fileHash, line);
}

void XScriptDebugger::OnLine(unsigned int fileHash, const std::string& file, int line, int stackDepth)
{
	if (!mEnabled)
		return;
	// 已经处于中断状态时（如 evaluate 触发的代码执行），不再进入中断
	if (mPaused)
		return;
	TryAcceptClient();
	if (!mClientConnected)
		return;
	PollRunningCommands();

	if (!ShouldPause(fileHash, line, stackDepth))
		return;

	std::string reason = "breakpoint";
	if (mStepMode != StepNone)
		reason = "step";
	mStepMode = StepNone;
	PauseAndDispatch(reason, file, line);

}

void XScriptDebugger::OnScriptStateSwitch()
{
	if (mStepMode != StepNone)
		mBreakOnStateSwitch = true;
}

void XScriptDebugger::OnException(const std::string& reason, const std::string& file, int line)
{
	if (mPaused)
		return;

	PauseAndDispatch(reason, file, line);
}

void XScriptDebugger::SendStoppedEvent(const std::string& reason, const std::string& file, int line)
{
	std::string msg = "{\"type\":\"event\",\"event\":\"stopped\",\"reason\":\"";
	msg += EscapeJson(reason);
	msg += "\",\"threadId\":1,\"file\":\"";
	msg += EscapeJson(file);
	msg += "\",\"line\":";
	msg += ConvertToString(line);
	msg += "}";
	SendLine(msg);
}

void XScriptDebugger::PauseAndDispatch(const std::string& reason, const std::string& file, int line)
{
	ClearVariableReferences();
	SendStoppedEvent(reason, file, line);
	mPaused = true;

	while (mPaused && mEnabled)
	{
		XScriptDebugCommand command;
		if (!ReadCommandBlocking(command))
		{
			mPaused = false;
			break;
		}
		DispatchCommand(command);
	}
}

bool XScriptDebugger::TryAcceptClient()
{
	if (mClientConnected)
		return true;
	if (ToSocket(mListenSocket) == INVALID_SOCKET)
		return false;

	SOCKET client = accept(ToSocket(mListenSocket), NULL, NULL);
	if (client == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		return err == WSAEWOULDBLOCK ? false : false;
	}

	u_long nonBlocking = 1;
	ioctlsocket(client, FIONBIO, &nonBlocking);
	mClientSocket = FromSocket(client);
	mClientConnected = true;
	fprintf(stderr, "XScript debug client connected\n");
	fflush(stderr);
	return true;
}

void XScriptDebugger::PollRunningCommands()
{
	XScriptDebugCommand command;
	while (ReadCommand(command, false))
	{
		DispatchRunningCommand(command);
	}
}

void XScriptDebugger::DispatchRunningCommand(const XScriptDebugCommand& command)
{
	// Running mode only handles breakpoint updates. Other queries are handled after stop.
	if (command.command == "setBreakpoints")
	{
		HandleSetBreakpoints(command);
		return;
	}

	if (command.command == "configurationDone")
	{
		SendResponse(command, true);
		return;
	}

	SendErrorResponse(command, "debug command is only available while paused: " + command.command);
}

bool XScriptDebugger::ReadCommandBlocking(XScriptDebugCommand& command)
{
	return ReadCommand(command, true);
}

bool XScriptDebugger::ReadCommand(XScriptDebugCommand& command, bool block)
{
	if (!mClientConnected && !TryAcceptClient())
		return false;

	while (true)
	{
		size_t lineEnd = mRecvBuffer.find('\n');
		if (lineEnd != std::string::npos)
		{
			std::string line = mRecvBuffer.substr(0, lineEnd);
			mRecvBuffer.erase(0, lineEnd + 1);
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);
			return ParseCommandLine(line, command);
		}

		char buffer[512];
		int ret = recv(ToSocket(mClientSocket), buffer, sizeof(buffer), 0);
		if (ret > 0)
		{
			mRecvBuffer.append(buffer, ret);
			continue;
		}

		if (ret == 0)
		{
			mClientConnected = false;
			return false;
		}

		int err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
		{
			if (!block)
				return false;

			fd_set readSet;
			FD_ZERO(&readSet);
			FD_SET(ToSocket(mClientSocket), &readSet);
			int sel = select(0, &readSet, NULL, NULL, NULL);
			if (sel <= 0)
				return false;
			continue;
		}

		mClientConnected = false;
		return false;
	}
}

bool XScriptDebugger::ParseCommandLine(const std::string& line, XScriptDebugCommand& command)
{
	command.seq = ExtractStringField(line, "seq");
	command.command = ExtractStringField(line, "cmd");
	if (command.command.empty())
		command.command = ExtractStringField(line, "command");
	command.file = ExtractStringField(line, "file");
	command.line = ExtractIntField(line, "line", -1);
	command.frameId = ExtractIntField(line, "frameId", 0);
	command.objectId = ExtractIntField(line, "objectId", 0);
	command.lines = ExtractIntArrayField(line, "lines");
	command.conditions = ExtractStringArrayField(line, "conditions");
	command.expression = ExtractStringField(line, "expression");
	command.kind = ExtractStringField(line, "kind");
	command.name = ExtractStringField(line, "name");
	command.value = ExtractStringField(line, "value");
	command.context = ExtractStringField(line, "context");
	return !command.command.empty();
}

std::string XScriptDebugger::ExtractStringField(const std::string& json, const char* field) const
{
	std::string key = "\"" + std::string(field) + "\"";
	size_t pos = json.find(key);
	if (pos == std::string::npos)
		return "";
	pos = json.find(':', pos + key.size());
	if (pos == std::string::npos)
		return "";
	pos = json.find('"', pos + 1);
	if (pos == std::string::npos)
		return "";
	std::string value;
	for (size_t i = pos + 1; i < json.size(); i++)
	{
		char c = json[i];
		if (c == '\\' && i + 1 < json.size())
		{
			char next = json[++i];
			if (next == 'n') value += '\n';
			else if (next == 'r') value += '\r';
			else if (next == 't') value += '\t';
			else value += next;
		}
		else if (c == '"')
		{
			break;
		}
		else
		{
			value += c;
		}
	}
	return value;
}

int XScriptDebugger::ExtractIntField(const std::string& json, const char* field, int defaultValue) const
{
	std::string key = "\"" + std::string(field) + "\"";
	size_t pos = json.find(key);
	if (pos == std::string::npos)
		return defaultValue;
	pos = json.find(':', pos + key.size());
	if (pos == std::string::npos)
		return defaultValue;
	pos++;
	while (pos < json.size() && isspace((unsigned char)json[pos])) pos++;
	return atoi(json.c_str() + pos);
}

std::vector<int> XScriptDebugger::ExtractIntArrayField(const std::string& json, const char* field) const
{
	std::vector<int> values;
	std::string key = "\"" + std::string(field) + "\"";
	size_t pos = json.find(key);
	if (pos == std::string::npos)
		return values;
	pos = json.find('[', pos + key.size());
	if (pos == std::string::npos)
		return values;
	size_t end = json.find(']', pos + 1);
	if (end == std::string::npos)
		return values;
	std::string content = json.substr(pos + 1, end - pos - 1);
	size_t start = 0;
	while (start < content.size())
	{
		while (start < content.size() && (isspace((unsigned char)content[start]) || content[start] == ',')) start++;
		if (start >= content.size()) break;
		values.push_back(atoi(content.c_str() + start));
		while (start < content.size() && content[start] != ',') start++;
	}
	return values;
}

std::vector<std::string> XScriptDebugger::ExtractStringArrayField(const std::string& json, const char* field) const
{
	std::vector<std::string> values;
	std::string key = "\"" + std::string(field) + "\"";
	size_t pos = json.find(key);
	if (pos == std::string::npos)
		return values;
	pos = json.find('[', pos + key.size());
	if (pos == std::string::npos)
		return values;

	// Parse array of strings: ["str1","str2",...]
	size_t i = pos + 1;
	while (i < json.size())
	{
		// Skip whitespace and commas
		while (i < json.size() && (isspace((unsigned char)json[i]) || json[i] == ','))
			i++;
		if (i >= json.size() || json[i] == ']')
			break;

		if (json[i] == '"')
		{
			// Parse a JSON string value (with escape handling)
			std::string val;
			i++; // skip opening quote
			while (i < json.size() && json[i] != '"')
			{
				if (json[i] == '\\' && i + 1 < json.size())
				{
					char next = json[++i];
					if (next == 'n') val += '\n';
					else if (next == 'r') val += '\r';
					else if (next == 't') val += '\t';
					else val += next;
				}
				else
				{
					val += json[i];
				}
				i++;
			}
			if (i < json.size()) i++; // skip closing quote
			values.push_back(val);
		}
		else if (json[i] == 'n' && i + 3 < json.size() && json.substr(i, 4) == "null")
		{
			// null -> empty string (no condition)
			values.push_back("");
			i += 4;
		}
		else
		{
			// Skip unexpected token
			while (i < json.size() && json[i] != ',' && json[i] != ']')
				i++;
		}
	}
	return values;
}

void XScriptDebugger::DispatchCommand(const XScriptDebugCommand& command)
{
	if (command.command == "setBreakpoints")
		HandleSetBreakpoints(command);
	else if (command.command == "continue")
		HandleContinue(command);
	else if (command.command == "stepIn")
		HandleStepIn(command);
	else if (command.command == "next")
		HandleNext(command);
	else if (command.command == "stepOut")
		HandleStepOut(command);
	else if (command.command == "stackTrace")
		HandleStackTrace(command);
	else if (command.command == "variables")
		HandleVariables(command);
	else if (command.command == "evaluate")
		HandleEvaluate(command);
	else if (command.command == "setVariable")
		HandleSetVariable(command);
	else
		SendErrorResponse(command, "unknown debug command: " + command.command);
}

void XScriptDebugger::SendLine(const std::string& text)
{
	std::string line = text + "\n";
	if (mClientConnected)
	{
		send(ToSocket(mClientSocket), line.c_str(), (int)line.size(), 0);
	}
}

void XScriptDebugger::SendResponse(const XScriptDebugCommand& command, bool success, const std::string& bodyJson)
{
	std::string msg = "{\"type\":\"response\",\"seq\":\"";
	msg += EscapeJson(command.seq);
	msg += "\",\"cmd\":\"";
	msg += EscapeJson(command.command);
	msg += "\",\"success\":";
	msg += success ? "true" : "false";
	msg += ",\"body\":";
	msg += bodyJson.empty() ? "{}" : bodyJson;
	msg += "}";
	SendLine(msg);
}

void XScriptDebugger::SendErrorResponse(const XScriptDebugCommand& command, const std::string& message)
{
	std::string body = "{\"message\":\"" + EscapeJson(message) + "\"}";
	SendResponse(command, false, body);
}

std::string XScriptDebugger::EscapeJson(const std::string& text) const
{
	std::string out;
	for (char c : text)
	{
		switch (c)
		{
		case '\\': out += "\\\\"; break;
		case '"': out += "\\\""; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default: out += c; break;
		}
	}
	return out;
}

void XScriptDebugger::HandleSetBreakpoints(const XScriptDebugCommand& command)
{
	SetBreakpoints(command.file, command.lines, command.conditions);
	// TODO: Return verified breakpoint list after line validation is available.
	SendResponse(command, true);
}

void XScriptDebugger::HandleContinue(const XScriptDebugCommand& command)
{
	mStepMode = StepNone;
	mBreakOnStateSwitch = false;
	mPaused = false;
	SendResponse(command, true);
}

void XScriptDebugger::HandleStepIn(const XScriptDebugCommand& command)
{
	mStepMode = StepIn;
	mStepDepth = mVM ? mVM->GetStackDepth() : 0;
	mBreakOnStateSwitch = false;
	mPaused = false;
	SendResponse(command, true);
}

void XScriptDebugger::HandleNext(const XScriptDebugCommand& command)
{
	mStepMode = StepOver;
	mStepDepth = mVM ? mVM->GetStackDepth() : 0;
	mBreakOnStateSwitch = false;
	mPaused = false;
	SendResponse(command, true);
}

void XScriptDebugger::HandleStepOut(const XScriptDebugCommand& command)
{
	mStepMode = StepOut;
	mStepDepth = mVM ? mVM->GetStackDepth() : 0;
	mBreakOnStateSwitch = false;
	mPaused = false;
	SendResponse(command, true);
}

void XScriptDebugger::HandleStackTrace(const XScriptDebugCommand& command)
{
	if (!mVM)
	{
		SendResponse(command, true, "{\"frames\":[]}");
		return;
	}

	int depth = mVM->GetStackDepth();
	std::string framesJson = "[";

	// Walk the call stack from top (current) to bottom
	for (int i = depth; i >= 0; i--)
	{
		CallInfo* ci = mVM->GetCallInfo(i);
		if (ci->mCurFunctionState == NULL)
			continue;

		FuncState* fs = ci->mCurFunctionState;

		// Determine the current line for this frame
		int frameLine = 0;
		if (i == depth)
		{
			// Top frame: use the current instruction's line
			Instr* curInstr = mVM->GetCurrentInstr();
			if (curInstr)
				frameLine = curInstr->lineIndex;
		}
		else
		{
			// Caller frames: the line is saved when the call was made
			frameLine = mVM->GetCallInfo(i + 1)->mCurLine;
		}

		// Frame id = call index (used by variablesRequest to identify the frame)
		int frameId = depth - i;

		if (framesJson.size() > 1)
			framesJson += ",";

		framesJson += "{\"id\":";
		framesJson += ConvertToString(frameId);
		framesJson += ",\"name\":\"";
		framesJson += EscapeJson(fs->funcName.empty() ? "<main>" : fs->funcName);
		framesJson += "\",\"file\":\"";
		framesJson += EscapeJson(fs->sourceFileName);
		framesJson += "\",\"line\":";
		framesJson += ConvertToString(frameLine);
		framesJson += ",\"column\":1}";
	}

	framesJson += "]";

	std::string body = "{\"frames\":" + framesJson + "}";
	SendResponse(command, true, body);
}

void XScriptDebugger::HandleVariables(const XScriptDebugCommand& command)
{
	if (!mVM)
	{
		SendResponse(command, true, "{\"variables\":[]}");
		return;
	}

	std::string varsJson = "[";

	if (command.objectId > 0)
	{
		// Expand table/list children
		HandleExpandObject(command.objectId, varsJson);
	}
	else if (command.kind == "globals")
	{
		// If env table exists, return env table variables; otherwise return global stack variables
		TableValue* envTable = mVM->GetEnvTable();
		if (envTable != NULL)
		{
			ExpandTable(envTable, varsJson);
		}
		else
		{
			// No env table, get from global variable stack
			const std::map<std::string, int>& globalVarMap = mVM->GetGlobalVarMap();
			const Value* globalStack = mVM->GetGlobalStackElements();
			for (std::map<std::string, int>::const_iterator it = globalVarMap.begin();
				it != globalVarMap.end(); ++it)
			{
				if (IsInternalIdent(it->first))
					continue;
				AppendVariable(varsJson, it->first, globalStack[it->second]);
			}
		}
	}
	else if (command.kind == "locals" || command.kind.empty())
	{
		// Return local variables for the specified frame
		int depth = mVM->GetStackDepth();
		int callIndex = depth - command.frameId;

		if (callIndex >= 0 && callIndex <= depth)
		{
			CallInfo* ci = mVM->GetCallInfo(callIndex);
			FuncState* fs = ci->mCurFunctionState;

			if (fs != NULL)
			{
				int currentPc = mVM->GetFrameInstrIndex(callIndex);
				std::set<std::string> addedNames;
				for (int i = (int)fs->m_localVarDebugInfoVec.size() - 1; i >= 0; i--)
				{
					const LocalVarDebugInfo& info = fs->m_localVarDebugInfoVec[i];
					if (info.localIndex < 0 || info.localIndex >= fs->stackFrameSize)
						continue;
					if (IsInternalIdent(info.name))
						continue;
					if (info.startPc > currentPc || (info.endPc >= 0 && currentPc >= info.endPc))
						continue;
					if (addedNames.find(info.name) != addedNames.end())
						continue;

					int stackPos = ci->mFrameIndex + info.localIndex - fs->stackFrameSize;
					const Value& val = mVM->getStackValue(stackPos);
					AppendVariable(varsJson, info.name, val);
					addedNames.insert(info.name);
				}
			}
		}
	}
	else if (command.kind == "upvalues")
	{
		// Return upvalues for the specified frame
		int depth = mVM->GetStackDepth();
		int callIndex = depth - command.frameId;

		if (callIndex >= 0 && callIndex <= depth)
		{
			CallInfo* ci = mVM->GetCallInfo(callIndex);
			Function* func = ci->mCurFunction;
			FuncState* fs = ci->mCurFunctionState;

			if (func != NULL && !func->isCFunc && fs != NULL)
			{
				LuaFunction& luaFunc = func->funcUnion.luaFunc;
				for (int i = 0; i < luaFunc.mNumUpVals; i++)
				{
					UpValue* uv = luaFunc.mUpVals[i];
					if (uv == NULL || uv->pValue == NULL)
						continue;

					// Get upvalue name from FuncState's compile-time info
					std::string uvName;
					if (i < (int)fs->m_upValueVec.size() && !fs->m_upValueVec[i].name.empty())
						uvName = fs->m_upValueVec[i].name;
					else
					{
						char buf[32];
						snprintf(buf, sizeof(buf), "upval_%d", i);
						uvName = buf;
					}

					AppendVariable(varsJson, uvName, *uv->pValue);
				}
			}
		}
	}

	varsJson += "]";
	std::string body = "{\"variables\":" + varsJson + "}";
	SendResponse(command, true, body);
}

void XScriptDebugger::HandleEvaluate(const XScriptDebugCommand& command)
{
	if (!mVM || command.expression.empty())
	{
		SendErrorResponse(command, "no expression to evaluate");
		return;
	}

	Value result;
	std::string kind;
	Value* variable = FindVariableByName(command.expression, command.frameId, kind);
	if (variable != NULL)
	{
		result = *variable;
	}
	else if (command.context == "hover" && IsOutOfScopeLocal(command.expression, command.frameId))
	{
		// 变量存在于当前函数中，但不在当前作用域内，返回成功响应以便 VSCode 显示提示
		std::string body = "{\"result\":{\"value\":\"<not in scope>\",\"type\":\"nil\",\"variablesReference\":0}}";
		SendResponse(command, true, body);
		return;
	}
	else
	{
		TableValue* evalEnv = BuildEvalEnvTable(command.frameId);
		if (evalEnv != NULL)
			mVM->SetEnvTable(evalEnv);

		std::string errorDesc;
		bool ok = false;
		if (command.context == "repl")
		{
			// repl 模式：先尝试作为表达式求值（获取返回值），再 fallback 到语句块执行
			EvalResult evalResult = mVM->EvalString(command.expression, result, errorDesc);
			ok = (evalResult == EVAL_OK);
			if (ok)
			{
				// 将调试环境表中被修改的值回写到原始的局部变量/upvalue 位置
				WriteBackGlobalToFrame(command.frameId);
			}
		}
		else
		{
			// watch/hover 模式：只读求值，不允许修改执行环境
			ok = (mVM->EvalReadOnlyExpression(command.expression, result, errorDesc) == EVAL_OK);
		}

		mVM->SetEnvTable(NULL);

		if (!ok)
		{
			SendErrorResponse(command, errorDesc.empty() ? "eval failed" : errorDesc);
			return;
		}
	}

	int varRef = AllocVariablesReference(result);
	std::string body = "{\"result\":{\"value\":\"";
	body += EscapeJson(FormatValue(result));
	body += "\",\"type\":\"";
	body += EscapeJson(GetValueTypeName(result));
	body += "\",\"variablesReference\":";
	body += ConvertToString(varRef);
	body += "}}";
	SendResponse(command, true, body);
}

std::string XScriptDebugger::NormalizePath(const std::string& path)
{
	if (path.empty())
		return path;

	// Convert relative path to absolute path
	char fullPathBuf[_MAX_PATH];
	std::string result = path;
	if (_fullpath(fullPathBuf, path.c_str(), _MAX_PATH) != NULL)
	{
		result = fullPathBuf;
	}

	// Normalize separators to '/' and convert to lowercase (Windows paths are case-insensitive)
	for (size_t idx = 0; idx < result.size(); idx++)
	{
		if (result[idx] == '\\')
			result[idx] = '/';
		result[idx] = (char)tolower((unsigned char)result[idx]);
	}

	return result;
}

unsigned int XScriptDebugger::HashFilePath(const std::string& normalizedPath)
{
	// FNV-1a hash - fast and well-distributed
	unsigned int hash = 2166136261u;
	for (size_t idx = 0; idx < normalizedPath.size(); idx++)
	{
		hash ^= (unsigned char)normalizedPath[idx];
		hash *= 16777619u;
	}
	return hash;
}

std::string XScriptDebugger::FormatValue(const Value& value) const
{
	char text[256] = { 0 };
	switch (value.type)
	{
	case OP_TYPE_NIL:
		return "nil";
	case OP_TYPE_INT:
		snprintf(text, sizeof(text), XIntConFmt, value.iIntValue);
		return text;
	case OP_TYPE_FLOAT:
		snprintf(text, sizeof(text), XFloatConFmt, value.fFloatValue);
		return text;
	case OP_TYPE_STRING:
		return std::string("\"") + stringRawValue(&value) + "\"";
	case OP_TYPE_FUNC:
		if (value.func && value.func->isCFunc)
		{
			snprintf(text, sizeof(text), "C function: 0x%p", value.func->funcUnion.cFunc.pfnAddr);
		}
		else if (value.func)
		{
			snprintf(text, sizeof(text), "function: %s", value.func->funcUnion.luaFunc.proto->funcName.c_str());
		}
		else
		{
			return "function: nil";
		}
		return text;
	case OP_TYPE_TABLE:
		if (value.tableData)
		{
			snprintf(text, sizeof(text), "table: 0x%p", value.tableData);
			return text;
		}
		return "table: nil";
	case OP_TYPE_LIST:
		if (value.listData)
		{
			snprintf(text, sizeof(text), "list[%lld]", (long long)value.listData->mListSize);
			return text;
		}
		return "list: nil";
	case OP_TYPE_THREAD:
		snprintf(text, sizeof(text), "thread: 0x%p", value.threadData);
		return text;
	case OP_TYPE_FUTURE:
		snprintf(text, sizeof(text), "future: 0x%p", value.futureData);
		return text;
	case OP_TYPE_USERDATA:
		snprintf(text, sizeof(text), "userdata: 0x%p", value.lightUserData);
		return text;
	default:
		return getTypeName(value.type);
	}
}

std::string XScriptDebugger::GetValueTypeName(const Value& value) const
{
	return getTypeName(value.type);
}

int XScriptDebugger::AllocVariablesReference(const Value& value)
{
	if (value.type == OP_TYPE_TABLE && value.tableData != NULL)
	{
		int ref = mNextVarRef++;
		mVarRefTableMap[ref] = value.tableData;
		return ref;
	}
	if (value.type == OP_TYPE_LIST && value.listData != NULL)
	{
		int ref = mNextVarRef++;
		mVarRefListMap[ref] = value.listData;
		return ref;
	}
	return 0;
}

void XScriptDebugger::HandleExpandObject(int objectId, std::string& varsJson)
{
	std::map<int, TableValue*>::iterator tableIt = mVarRefTableMap.find(objectId);
	if (tableIt != mVarRefTableMap.end())
	{
		ExpandTable(tableIt->second, varsJson);
		return;
	}

	std::map<int, ListValue*>::iterator listIt = mVarRefListMap.find(objectId);
	if (listIt != mVarRefListMap.end())
	{
		ExpandList(listIt->second, varsJson);
		return;
	}
}

void XScriptDebugger::ExpandTable(TableValue* table, std::string& varsJson)
{
	// Output array part first
	for (XInt i = 0; i < table->mArraySize; i++)
	{
		if (table->mArrayData[i].type == OP_TYPE_NIL)
			continue;
		char indexBuf[32];
		snprintf(indexBuf, sizeof(indexBuf), "[%lld]", (long long)i);
		AppendVariable(varsJson, indexBuf, table->mArrayData[i]);
	}

	// Then output hash part
	for (int i = 0; i < table->mNodeCapacity; i++)
	{
		Value& val = table->mNodeData[i].value;
		if (val.type == OP_TYPE_NIL)
			continue;

		Value& keyVal = table->mNodeData[i].key.keyVal;
		std::string name;
		if (keyVal.type == OP_TYPE_STRING)
		{
			name = stringRawValue(&keyVal);
			if (IsInternalIdent(name))
				continue;
		}
		else
			name = FormatValue(keyVal);

		AppendVariable(varsJson, name, val);
	}
}

void XScriptDebugger::ExpandList(ListValue* list, std::string& varsJson)
{
	for (XInt i = 0; i < list->mListSize; i++)
	{
		char indexBuf[32];
		snprintf(indexBuf, sizeof(indexBuf), "[%lld]", (long long)i);
		AppendVariable(varsJson, indexBuf, list->mListData[i]);
	}
}

void XScriptDebugger::AppendVariable(std::string& varsJson, const std::string& name, const Value& val)
{
	if (varsJson.size() > 1)
		varsJson += ",";

	varsJson += "{\"name\":\"";
	varsJson += EscapeJson(name);
	varsJson += "\",\"value\":\"";
	varsJson += EscapeJson(FormatValue(val));
	varsJson += "\",\"type\":\"";
	varsJson += EscapeJson(GetValueTypeName(val));
	varsJson += "\",\"variablesReference\":";
	varsJson += ConvertToString(AllocVariablesReference(val));
	varsJson += "}";
}

void XScriptDebugger::ClearVariableReferences()
{
	mNextVarRef = 1;
	mVarRefTableMap.clear();
	mVarRefListMap.clear();
}

bool XScriptDebugger::ParseValueString(const std::string& str, Value& outValue)
{
	if (str == "nil")
	{
		outValue.type = OP_TYPE_NIL;
		return true;
	}
	if (str == "true")
	{
		outValue.type = OP_TYPE_INT;
		outValue.iIntValue = 1;
		return true;
	}
	if (str == "false")
	{
		outValue.type = OP_TYPE_INT;
		outValue.iIntValue = 0;
		return true;
	}

	// String: wrapped in double quotes
	if (str.size() >= 2 && str[0] == '"' && str[str.size() - 1] == '"')
	{
		std::string content = str.substr(1, str.size() - 2);
		outValue = mVM->ConstructValue(content.c_str());
		return true;
	}

	// Try to parse as integer
	char* endPtr = NULL;
	long long intVal = strtoll(str.c_str(), &endPtr, 0);
	if (endPtr != str.c_str() && *endPtr == '\0')
	{
		outValue.type = OP_TYPE_INT;
		outValue.iIntValue = (XInt)intVal;
		return true;
	}

	// Try to parse as float
	double floatVal = strtod(str.c_str(), &endPtr);
	if (endPtr != str.c_str() && *endPtr == '\0')
	{
		outValue.type = OP_TYPE_FLOAT;
		outValue.fFloatValue = (XFloat)floatVal;
		return true;
	}

	// Unquoted string: treat as string
	outValue = mVM->ConstructValue(str.c_str());
	return true;
}

TableValue* XScriptDebugger::BuildEvalEnvTable(int frameId)
{
	if (!mVM)
		return NULL;

	int depth = mVM->GetStackDepth();
	int callIndex = depth - frameId;
	return mVM->BuildFrameVarsTable(callIndex, mVM->GetFrameInstrIndex(callIndex));
}

void XScriptDebugger::WriteBackGlobalToFrame(int frameId)
{
	if (!mVM)
		return;

	int depth = mVM->GetStackDepth();
	int callIndex = depth - frameId;
	if (callIndex < 0 || callIndex > depth)
		return;

	CallInfo* ci = mVM->GetCallInfo(callIndex);
	FuncState* fs = ci->mCurFunctionState;
	if (fs == NULL)
		return;

	int currentPc = mVM->GetFrameInstrIndex(callIndex);
	std::set<std::string> writtenNames;

	// 从调试环境表读取修改后的值（repl 模式下写入的是 debugEnvTable）
	TableValue* envTable = mVM->GetEnvTable();
	for (int i = (int)fs->m_localVarDebugInfoVec.size() - 1; i >= 0; i--)
	{
		const LocalVarDebugInfo& info = fs->m_localVarDebugInfoVec[i];
		if (info.localIndex < 0 || info.localIndex >= fs->stackFrameSize)
			continue;
		if (IsInternalIdent(info.name))
			continue;
		if (info.startPc > currentPc || (info.endPc >= 0 && currentPc >= info.endPc))
			continue;
		if (writtenNames.find(info.name) != writtenNames.end())
			continue;

		// 只从环境表读取修改后的值，环境表中不存在则跳过（不回写未被环境表管理的变量）
		Value* modifiedVal = FindEnvVariable(envTable, info.name);
		if (modifiedVal == NULL)
			continue;

		// Write the modified value back to the local variable's stack position
		int stackPos = ci->mFrameIndex + info.localIndex - fs->stackFrameSize;
		Value* localVal = mVM->getStackValueRef(stackPos);
		if (localVal != NULL)
			*localVal = *modifiedVal;

		writtenNames.insert(info.name);
	}

	// Write back to upvalues
	Function* func = ci->mCurFunction;
	if (func != NULL && !func->isCFunc)
	{
		LuaFunction& luaFunc = func->funcUnion.luaFunc;
		for (int i = 0; i < luaFunc.mNumUpVals; i++)
		{
			UpValue* uv = luaFunc.mUpVals[i];
			if (uv == NULL || uv->pValue == NULL)
				continue;

			std::string uvName;
			if (i < (int)fs->m_upValueVec.size() && !fs->m_upValueVec[i].name.empty())
				uvName = fs->m_upValueVec[i].name;
			else
				continue;

			if (writtenNames.find(uvName) != writtenNames.end())
				continue;

			// 只从环境表读取修改后的值，环境表中不存在则跳过
			Value* modifiedVal = FindEnvVariable(envTable, uvName);
			if (modifiedVal == NULL)
				continue;

			*uv->pValue = *modifiedVal;
			writtenNames.insert(uvName);
		}
	}
}

Value* XScriptDebugger::FindEnvVariable(TableValue* envTable, const std::string& name)
{
	if (envTable == NULL)
		return NULL;

	Value key = mVM->ConstructValue(name.c_str());
	return mVM->GetTableValueRef(envTable, key);
}

Value* XScriptDebugger::FindGlobalVariable(const std::string& name)
{
	TableValue* envTable = mVM->GetEnvTable();
	if (envTable != NULL)
	{
		for (int i = 0; i < envTable->mNodeCapacity; i++)
		{
			Value& keyVal = envTable->mNodeData[i].key.keyVal;
			if (keyVal.type == OP_TYPE_STRING && stringRawValue(&keyVal) == name)
				return &envTable->mNodeData[i].value;
		}
	}

	return mVM->GetGlobalValue(name);
}

Value* XScriptDebugger::FindVariableInObject(int objectId, const std::string& name)
{
	std::map<int, TableValue*>::iterator tableIt = mVarRefTableMap.find(objectId);
	if (tableIt != mVarRefTableMap.end())
	{
		TableValue* table = tableIt->second;
		XInt index = 0;
		if (ParseBracketIndex(name, index))
			return mVM->GetTableValueRef(table, mVM->ConstructValue(index));
		return mVM->GetTableValueRef(table, mVM->ConstructValue(name.c_str()));
	}

	std::map<int, ListValue*>::iterator listIt = mVarRefListMap.find(objectId);
	if (listIt != mVarRefListMap.end())
	{
		XInt index = 0;
		if (ParseBracketIndex(name, index) && index >= 0 && index < listIt->second->mListSize)
			return &listIt->second->mListData[index];
	}

	return NULL;
}

Value* XScriptDebugger::FindVariableInScope(const std::string& kind, const std::string& name, int frameId, int objectId, std::string& outKind)
{
	if (!mVM)
		return NULL;

	if (objectId > 0)
	{
		outKind = "object";
		return FindVariableInObject(objectId, name);
	}

	if (kind == "globals")
	{
		outKind = "globals";
		return FindGlobalVariable(name);
	}

	int depth = mVM->GetStackDepth();
	int callIndex = depth - frameId;
	if (callIndex < 0 || callIndex > depth)
		return NULL;

	CallInfo* ci = mVM->GetCallInfo(callIndex);
	FuncState* fs = ci->mCurFunctionState;

	if (kind == "locals")
	{
		if (fs == NULL)
			return NULL;

		int currentPc = mVM->GetFrameInstrIndex(callIndex);
		for (int i = (int)fs->m_localVarDebugInfoVec.size() - 1; i >= 0; i--)
		{
			const LocalVarDebugInfo& info = fs->m_localVarDebugInfoVec[i];
			if (info.name != name)
				continue;
			if (info.localIndex < 0 || info.localIndex >= fs->stackFrameSize)
				continue;
			if (info.startPc > currentPc || (info.endPc >= 0 && currentPc >= info.endPc))
				continue;

			int stackPos = ci->mFrameIndex + info.localIndex - fs->stackFrameSize;
			outKind = "locals";
			return mVM->getStackValueRef(stackPos);
		}
		return NULL;
	}

	if (kind == "upvalues")
	{
		Function* func = ci->mCurFunction;
		if (func == NULL || func->isCFunc || fs == NULL)
			return NULL;

		LuaFunction& luaFunc = func->funcUnion.luaFunc;
		for (int i = 0; i < luaFunc.mNumUpVals; i++)
		{
			UpValue* uv = luaFunc.mUpVals[i];
			if (uv == NULL || uv->pValue == NULL)
				continue;

			std::string uvName;
			if (i < (int)fs->m_upValueVec.size() && !fs->m_upValueVec[i].name.empty())
				uvName = fs->m_upValueVec[i].name;

			if (uvName == name)
			{
				outKind = "upvalues";
				return uv->pValue;
			}
		}
		return NULL;
	}

	return FindVariableByName(name, frameId, outKind);
}

bool XScriptDebugger::IsOutOfScopeLocal(const std::string& name, int frameId)
{
	if (!mVM)
		return false;

	int depth = mVM->GetStackDepth();
	int callIndex = depth - frameId;
	if (callIndex < 0 || callIndex > depth)
		return false;

	CallInfo* ci = mVM->GetCallInfo(callIndex);
	FuncState* fs = ci->mCurFunctionState;
	if (fs == NULL)
		return false;

	int currentPc = mVM->GetFrameInstrIndex(callIndex);
	bool foundInScope = false;
	bool foundOutOfScope = false;

	for (int i = (int)fs->m_localVarDebugInfoVec.size() - 1; i >= 0; i--)
	{
		const LocalVarDebugInfo& info = fs->m_localVarDebugInfoVec[i];
		if (info.name != name)
			continue;
		if (info.localIndex < 0 || info.localIndex >= fs->stackFrameSize)
			continue;
		if (IsInternalIdent(info.name))
			continue;

		// 找到了同名变量，检查是否在作用域内
		if (info.startPc <= currentPc && (info.endPc < 0 || currentPc < info.endPc))
			foundInScope = true;
		else
			foundOutOfScope = true;
	}

	// 存在同名变量但全部不在作用域内
	return !foundInScope && foundOutOfScope;
}

Value* XScriptDebugger::FindVariableByName(const std::string& name, int frameId, std::string& outKind)
{
	Value* value = FindVariableInScope("locals", name, frameId, 0, outKind);
	if (value != NULL)
		return value;

	value = FindVariableInScope("upvalues", name, frameId, 0, outKind);
	if (value != NULL)
		return value;

	outKind = "globals";
	return FindGlobalVariable(name);
}

void XScriptDebugger::HandleSetVariable(const XScriptDebugCommand& command)
{
	if (!mVM)
	{
		SendErrorResponse(command, "VM not available");
		return;
	}

	std::string kind;
	Value* pValue = FindVariableInScope(command.kind, command.name, command.frameId, command.objectId, kind);
	if (pValue == NULL)
	{
		SendErrorResponse(command, "variable not found: " + command.name);
		return;
	}

	Value newValue;
	if (!ParseValueString(command.value, newValue))
	{
		SendErrorResponse(command, "cannot parse value: " + command.value);
		return;
	}

	*pValue = newValue;

	int varRef = AllocVariablesReference(newValue);
	std::string body = "{\"value\":\"";
	body += EscapeJson(FormatValue(newValue));
	body += "\",\"type\":\"";
	body += EscapeJson(GetValueTypeName(newValue));
	body += "\",\"variablesReference\":";
	body += ConvertToString(varRef);
	body += "}";
	SendResponse(command, true, body);
}
