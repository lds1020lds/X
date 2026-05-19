#pragma once

#include <map>
#include <set>
#include <stdint.h>
#include <string>
#include <vector>

class XScriptVM;
struct Value;
class TableValue;
class ListValue;
class FuncState;

struct XScriptDebugCommand
{
	std::string seq;
	std::string command;
	std::string file;
	std::string kind;
	int line;
	int frameId;
	int objectId;
	std::vector<int> lines;
	std::vector<std::string> conditions;	// condition expressions per breakpoint (parallel to lines)
	std::string expression;
	std::string name;		// variable name (for setVariable)
	std::string value;		// variable value string (for setVariable)
	std::string context;	// evaluate context (watch/repl/hover)

	XScriptDebugCommand()
		: line(-1)
		, frameId(0)
		, objectId(0)
	{
	}
};

class XScriptDebugger
{
public:
	enum StepMode
	{
		StepNone,
		StepIn,
		StepOver,
		StepOut,
	};

	XScriptDebugger();
	~XScriptDebugger();

	void AttachVM(XScriptVM* vm);
	bool StartServer(int port, bool waitDebugger = false);
	void StopServer();
	bool IsEnabled() const { return mEnabled; }
	bool HasClient() const { return mClientConnected; }

	void ClearBreakpoints(const std::string& file);
	void SetBreakpoints(const std::string& file, const std::vector<int>& lines, const std::vector<std::string>& conditions);
	bool CheckBreakpointHit(unsigned int fileHash, int line);
	bool ShouldPause(unsigned int fileHash, int line, int stackDepth);
	void OnLine(unsigned int fileHash, const std::string& file, int line, int stackDepth);
	void OnScriptStateSwitch();

	// Convert relative path to absolute path and normalize separators
	static std::string NormalizePath(const std::string& path);
	// Compute hash of file path (for fast comparison)
	static unsigned int HashFilePath(const std::string& normalizedPath);

	void SendStoppedEvent(const std::string& reason, const std::string& file, int line);
	void PauseAndDispatch(const std::string& reason, const std::string& file, int line);
	void OnException(const std::string& reason, const std::string& file, int line);
private:
	bool TryAcceptClient();
	void PollRunningCommands();
	void DispatchRunningCommand(const XScriptDebugCommand& command);
	bool ReadCommand(XScriptDebugCommand& command, bool block);
	bool ReadCommandBlocking(XScriptDebugCommand& command);
	bool ParseCommandLine(const std::string& line, XScriptDebugCommand& command);
	std::string ExtractStringField(const std::string& json, const char* field) const;
	int ExtractIntField(const std::string& json, const char* field, int defaultValue) const;
	std::vector<int> ExtractIntArrayField(const std::string& json, const char* field) const;
	std::vector<std::string> ExtractStringArrayField(const std::string& json, const char* field) const;
	void DispatchCommand(const XScriptDebugCommand& command);
	void SendLine(const std::string& text);
	void SendResponse(const XScriptDebugCommand& command, bool success, const std::string& bodyJson = "{}");
	void SendErrorResponse(const XScriptDebugCommand& command, const std::string& message);
	std::string EscapeJson(const std::string& text) const;

	void HandleSetBreakpoints(const XScriptDebugCommand& command);
	void HandleContinue(const XScriptDebugCommand& command);
	void HandleStepIn(const XScriptDebugCommand& command);
	void HandleNext(const XScriptDebugCommand& command);
	void HandleStepOut(const XScriptDebugCommand& command);
	void HandleStackTrace(const XScriptDebugCommand& command);
	void HandleVariables(const XScriptDebugCommand& command);
	void HandleEvaluate(const XScriptDebugCommand& command);
	void HandleSetVariable(const XScriptDebugCommand& command);

	// Helper: format a Value as a display string for the debugger
	std::string FormatValue(const Value& value) const;
	// Helper: get the type name string for a Value
	std::string GetValueTypeName(const Value& value) const;
	// Allocate a unique reference ID for expandable values (table/list)
	int AllocVariablesReference(const Value& value);
	// Handle objectId expand request, enumerate table/list children
	void HandleExpandObject(int objectId, std::string& varsJson);
	// Enumerate all key-value pairs of a table, append to varsJson
	void ExpandTable(TableValue* table, std::string& varsJson);
	// Enumerate all elements of a list, append to varsJson
	void ExpandList(ListValue* list, std::string& varsJson);
	// Append a variable entry to varsJson
	void AppendVariable(std::string& varsJson, const std::string& name, const Value& val);
	// Clear variable reference map (called on each pause)
	void ClearVariableReferences();
	// Parse user input value string to Value (supports int/float/string/nil/true/false)
	bool ParseValueString(const std::string& str, Value& outValue);
	// Find variable pointer by name in specified frame/scope/object
	Value* FindVariableByName(const std::string& name, int frameId, std::string& outKind);
	Value* FindVariableInScope(const std::string& kind, const std::string& name, int frameId, int objectId, std::string& outKind);
	// Check if a variable name exists as a local but is out of scope at the current PC
	bool IsOutOfScopeLocal(const std::string& name, int frameId);
	Value* FindVariableInObject(int objectId, const std::string& name);
	Value* FindGlobalVariable(const std::string& name);
	Value* FindEnvVariable(TableValue* envTable, const std::string& name);
	// Build an env table containing locals/upvalues for eval in the specified frame
	TableValue* BuildEvalEnvTable(int frameId);
	// Write back modified global variables to the original local/upvalue positions in the specified frame
	void WriteBackGlobalToFrame(int frameId);

private:
	XScriptVM* mVM;
	bool mEnabled;
	bool mClientConnected;
	bool mPaused;
	bool mWinsockStarted;
	StepMode mStepMode;
	int mStepDepth;
	bool mBreakOnStateSwitch;
	int mPort;
	uintptr_t mListenSocket;
	uintptr_t mClientSocket;
	std::string mRecvBuffer;
	// Breakpoint storage: file -> set of lines (for fast lookup)
	std::map<std::string, std::set<int> > mBreakpoints;
	std::map<unsigned int, std::set<int> > mBreakpointsByHash;
	// Conditional breakpoint: (fileHash, line) -> condition expression string
	std::map<unsigned int, std::map<int, std::string> > mBreakpointConditions;
	// Pre-compiled condition functions: (fileHash, line) -> compiled FuncState (marked MS_Fixed for GC safety)
	std::map<unsigned int, std::map<int, FuncState*> > mCompiledConditions;

	// Variable expand: objectId -> expandable object pointer map
	int mNextVarRef;
	std::map<int, TableValue*> mVarRefTableMap;
	std::map<int, ListValue*> mVarRefListMap;
};
