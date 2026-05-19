#pragma once 
#include "VMDefs.h"
#include <setjmp.h>
#include <queue>
#include <functional>
#include <mutex>
#include <memory>

// 脚本执行状态：每个协程拥有独立的执行状态
class XScriptDebugger;
class XScriptVM;

struct AsyncRuntime
{
	std::mutex mutex;
	std::queue<std::function<void(XScriptVM*)> > completions;
	bool alive;

	AsyncRuntime()
		: alive(true)
	{
	}
};

class XScriptState
{
public:
	UpValue*									mNextRefUpVals;		// 引用的上值链表
	CallInfo*									mCallInfoBase;		// 调用信息栈基址
	int											mCurCallIndex;		// 当前调用深度

	FuncState*									mCurFunctionState;	// 当前函数原型
	Function*									mCurFunction;		// 当前执行的函数对象

	Value*										mStackElements;		// 运行时栈数组
	int											mStackSize;			// 栈容量
	int											mTopIndex;			// 栈顶索引
	int											mFrameIndex;		// 当前栈帧索引

	int											mInstrIndex;		// 当前指令索引
	int											mCCallIndex;		// C函数调用深度
	int											mCurCCallLuaCallIndex; //启动当前C Call的lua call index
	int											mLastProtectCallLuaCallIndex; //上次启动 pcall 依赖的lua Call index

	int											mStatus;			// 协程状态
	int											mSuspendReason;		// 挂起原因：yield或await
	bool										mIsAsyncTask;		// 是否为async函数创建的任务协程
	ECoroutineType								mCoroutineType;
	XFuture*									mWaitingFuture;		// await时等待的Future
	XFuture*									mOwnerTask;			// async函数返回给调用方的Future
	Value										mAwaitResumeValue;	// await恢复值
	bool										mHasAwaitResumeValue;
	bool										mIsRejected;
	int								mAsyncStartParamCount;
	Function*                       mCurrentCFunction;  // 当前C函数
	Function*                       mStartFunction;     // 协程入口函数
};

typedef TableValue*			TABLE;

// XScript虚拟机：负责脚本的编译、执行、内存管理和宿主交互
class	XScriptVM
{
public:
	XScriptVM();
	~XScriptVM();

	// 初始化虚拟机
	bool			init();

	// 初始化列表内置元方法
	void InitListBuiltinMetaClass();

	// 初始化表内置元方法
	void InitTableBuiltinMetaClass();

	// 初始化字符串内置元方法
	void InitStringBuildinMetaClass();

	// 加载宿主库（注册内置函数）
	void			loadHostLibs();
	// 执行脚本文件
	bool			doFile(const std::string& fileName);

	// 将中间码转换为虚拟机指令
	void			ConvertMidCodeToInstr(CSymbolTable &symbolTable, CMidCode &midCode, const std::string& fileName, std::map<int, FuncState*>& funcMap);

	// 注册全局变量到虚拟机
	void			RegisterGlobalValues(CSymbolTable &symbolTable);
	// 获取环境变量值
	Value			GetEnvValue(int index);
	// 写入环境变量值
	void			SetEnvValue(int index, const Value& value);
	
	// 编译字符串为函数原型
	FuncState*		CompileString(const std::string& code);
	// 编译文件为函数原型
	FuncState*		CompileFile(const std::string& fileName);
	// 设置全局环境表
	void			SetEnvTable(TableValue* envTable) { mEnvTable = envTable; }
	// 获取全局环境表
	TableValue*		GetEnvTable() { return mEnvTable; }
	// 构建指定调用帧的当前可见变量表（局部变量 + upvalue），用于环境表和调试
	TableValue*		BuildFrameVarsTable(int callIndex, int currentPc = -1);

	// 分配下一个全局变量槽位索引（只增不减）
	int				AllocGlobalVarIndex();
	// 获取全局变量索引
	int				GetGlobalVarIndex(VariantST* var);
	// 根据变量名获取全局变量值指针
	Value*			GetGlobalValue(const std::string& varName);
	// 获取全局变量名到索引的映射（调试器用）
	const std::map<std::string, int>& GetGlobalVarMap() const { return mGlobalVarMap; }
	// 获取全局变量栈元素指针（调试器用）
	const Value*	GetGlobalStackElements() const { return mGlobalStackElements; }

	// 设置全局栈中指定索引的值
	void			setGlobalStackValue(int index, const Value& value);
	// 获取栈中指定索引的值（只读）
	const Value&    getStackValue(int index);
	// 获取栈中指定索引的值指针（可修改）
	Value*		    getStackValueRef(int index);

	// 构造Value系列方法：将各种C++类型包装为脚本Value
	Value			ConstructValue(XInt value);
	Value			ConstructValue(XFloat value);
	Value			ConstructValue(const char* value);
	Value			ConstructValue(XScriptState* state);
	Value			ConstructValue(XString* str);
	Value			ConstructValue(TABLE value);
	Value			ConstructValue(FuncState* mainFunc);
	Value			ConstructValue(ListValue* list);
	Value			ConstructValue(XFuture* future);
	XPromise*		CreatePromise();
	void			ResolveFuture(XFuture* future, const Value& value);
	void			RejectFuture(XFuture* future, const Value& value);

	void			AsyncReady(XScriptState* state, const Value& value, bool isRejected = false);
	void			AsyncTick();
	void			RunUntilComplete(XFuture* future);
	void			AddAsyncTimer(int ms, XPromise* promise);
	void			AddPendingPromise(XPromise* promise);
	void			RemovePendingPromise(XPromise* promise);
	void			QueueAsyncCompletion(const std::function<void(XScriptVM*)>& completion);
	static bool		QueueAsyncCompletion(std::weak_ptr<AsyncRuntime> runtime, const std::function<void(XScriptVM*)>& completion);
	std::weak_ptr<AsyncRuntime> GetAsyncRuntime() const;
	void			SpawnFuture(XFuture* future);
	bool			IsCurrentAsyncTask() const;

	// 创建新表（可指定数组部分初始大小）
	TABLE			newTable(int arraySize = 0);

	// 表取值系列方法（字符串键）
	bool			getTableValue(TABLE table, const  char* key, XInt& value);
	bool			getTableValue(TABLE table, const char* key, XFloat& value);
	bool			getTableValue(TABLE table, const char* key, const char* &value);
	bool			getTableValue(TABLE table, const  char* key, TABLE& value);
	bool			getTableValue(TABLE table, const  char* key, ListValue* &value);

	// 表取值系列方法（整数键）
	bool			getTableValue(TABLE table, XInt key, XInt& value);
	bool			getTableValue(TABLE table, XInt key, XFloat& value);
	bool			getTableValue(TABLE table, XInt key, char* &value);
	bool			getTableValue(TABLE table, XInt key, TABLE& value);
	bool			getTableValue(TABLE table, const  XInt key, ListValue*& value);

	// 表赋值系列方法（字符串键）
	void			setTableValue(TABLE table, const char* key, XInt value);
	void			setTableValue(TABLE table, const char* key, XFloat value);
	void			setTableValue(TABLE table, const char* key, const char* str);
	void			setTableValue(TABLE table, const char* key, TABLE t);

	// 表赋值系列方法（整数键）
	void			setTableValue(TABLE table, XInt key, XInt value);
	void			setTableValue(TABLE table, XInt key, XFloat value);
	void			setTableValue(TABLE table, XInt key, const char* str);
	void			setTableValue(TABLE table, XInt key, TABLE t);
	// 通用表赋值（Value键和Value值）
	void			setTableValue(TABLE table, const Value& key, const Value& value);

	// 通用表取值（Value键）
	bool			getTableValue(TableValue* table, const Value &keyValue, Value& resultValue);
	// 获取表中值的引用指针
	Value*			GetTableValueRef(TableValue* tableDate, const Value &keyValue);

	// ===== 列表操作方法 =====
	void			ListAppend(ListValue* list, const Value& value);		// 追加元素
	void			ListInsert(ListValue* list, XInt pos, const Value& value);// 在指定位置插入
	void			ListRemove(ListValue* list, const Value& value);		// 按值删除
	void			ListRemoveAtPos(ListValue* list, XInt pos, XInt num = 1);// 按位置删除
	void			ListReallocate(ListValue* list, XInt size = -1);		// 重新分配内存
	void			ListShrink(ListValue* list, XInt size = -1);			// 收缩内存
	void			ListResize(ListValue* list, XInt size);					// 调整大小
	void			InsertSort(ListValue* list, bool isReverse);			// 插入排序

	void			InsertSortWithCompFunc(ListValue* list, bool isReverse, Function* sortFunc);// 自定义比较函数排序
	void			InsertSortWithKeyFunc(ListValue* list, bool isReverse, Function* keyFunc);// 自定义键函数排序
	void			ListReverse(ListValue* list);							// 反转列表
	XInt			ListCount(ListValue* list, const Value& value);			// 统计元素出现次数

	// 重置脚本执行状态
	void			resetScriptState(XScriptState* state);
	// 注册宿主API函数
	void			RegisterHostApi(const char* apiName, HOST_FUNC pfnAddr);
	void			RegisterHostApi(const char* apiName, HOST_FUNC pfnAddr, HOST_FUNC pfnAddr2);

	// 注册用户自定义类（支持继承）
	void			RegisterUserClass(const char* className, const char* bassClassName, const std::vector<HostFunction>& funcVec, const std::vector<ConstantData>& constantVec = std::vector<ConstantData>());

	// ===== 宿主函数参数获取接口 =====
	int				getNumParam();							// 获取参数数量
	int				getParamType(int paramIndex);			// 获取参数类型

	Value			getParamValue(int paramIndex);			// 获取参数Value
	int				getParamStackIndex(int paramIndex);		// 获取参数栈索引

	Value*			getReturnRegValue(int index);			// 获取返回值寄存器

	bool			getParamAsInt(int paramIndex, XInt& value);		// 获取整数参数
	bool			getParamAsFloat(int paramIndex, XFloat& value);	// 获取浮点参数
	bool			getParamAsString(int paramIndex, char* &value);	// 获取字符串参数
	void*			getParamAsObj(int paramIndex, char* userType);	// 获取用户对象参数
	bool			getParamAsTable(int paramIndex, TABLE& table);	// 获取表参数

	// ===== 宿主函数返回值设置接口 =====
	void			resetReturnValue();									// 重置返回值

	void			setReturnAsNil(int regIndex);						// 返回nil
	void			setReturnAsTable(const TABLE& table, int regIndex = 0);// 返回表
	void			setReturnAsValue(const Value& table, int regIndex = 0);// 返回Value

	void			setReturnAsInt(XInt iResult, int regIndex = 0);		// 返回整数
	void			setReturnAsfloat(XFloat fResult, int regIndex = 0);	// 返回浮点数
	void			setReturnAsStr(const char* strResult, int regIndex = 0);// 返回字符串

	void			setReturnAsUserData(const char* className, void* pThis, int regIndex = 0);// 返回用户数据

	// 创建类用户数据对象
	UserData*		CreateClassUserData(const char* className, void* pThis);

	// 设置调试钩子掩码
	void			SetHookMask(int mask) { mHookMask = mask; }
	// 挂接 XScript 调试器
	void			SetDebugger(XScriptDebugger* debugger);
	// 获取 XScript 调试器
	XScriptDebugger* GetDebugger() { return mDebugger; }

	// ===== 从宿主调用脚本函数的接口 =====
	void			beginCall();					// 开始调用（准备参数）
	void			endCall();						// 结束调用

	void			pushIntParam(XInt iValue);		// 压入整数参数
	void			pushFloatParam(XFloat fValue);	// 压入浮点参数
	void			pushStrParam(const char* strValue);// 压入字符串参数

	// 根据索引获取字符串常量
	const			std::string&	GetString(int index) { return index < (int)m_stringVec.size() ? m_stringVec[index] : mEmptyStr; }

	// 添加字符串常量到字符串池（去重）
	int				AddString(const std::string& funcName);

	// 注册全局变量名
	int				RegisterGlobalValue(const std::string& name);

	// 注册宿主函数库
	void			RegisterHostLib(const char* libName, std::vector<HostFunction>& hostFuncs);

	// 将宿主函数列表创建为表
	void			CreateFunctionTable(const std::vector<HostFunction> &hostFuncVec, TABLE table);

	// 执行垃圾回收
	void			GarbageCollect();
	// 加载模块并返回模块导出值

	Value			RequireModule(const char* moduleName);
	
	// 获取调用栈回溯信息
	std::string		stackBackTrace();

	// 根据变量名获取栈中的值（调试用）
	Value*			GetStackValueByName(int stackIndex, const std::string& name);
	// 根据变量索引获取栈中的值（调试用）
	Value*			GetStackValueByIndex(int stackIndex, int varIndex, std::string& name);

	// 调用调试钩子函数
	void			CallHookFunction(int event, int curLine);
	// 获取当前调用栈深度
	int				GetStackDepth() { return mCurXScriptState->mCurCallIndex; }
	// 获取当前活跃的脚本执行状态
	XScriptState*	GetCurrentScriptState() { return mCurXScriptState; }
	// 设置当前活跃的脚本执行状态
	void			SetCurrentXScriptState(XScriptState* state);
	// 获取指定层级的调用信息
	CallInfo*		GetCallInfo(int index) { return &mCurXScriptState->mCallInfoBase[index]; }
	// 获取当前正在执行的指令
	Instr*			GetCurrentInstr() { return mCurInstr; }
	// 获取指定调用帧当前对应的指令索引
	int				GetFrameInstrIndex(int callIndex);
	// 保护模式调用函数（捕获错误）
	int				ProtectCallFunction(Function* firstValue, int numParam, std::string& errorDesc, int errorFunc = -1);

	// 保护模式恢复协程
	int				ProtectResume( std::string & errorDesc);

	void			BeforeExecuteResume();
	// 压栈
	void			push(const Value& value);
	// 弹栈
	Value			pop();
	// 编译条件字符串表达式（不执行），返回编译后的 FuncState，失败返回 NULL
	// 编译成功后会进行只读检查，如果表达式包含对执行环境的写入操作则返回 NULL
	FuncState*		CompileConditionString(const char* condstr);
	// 执行已编译的条件表达式，返回布尔结果
	bool			EvalCompiledCondition(FuncState* compiledFunc);
	// 静态检查编译后的函数是否为只读（不修改执行环境）
	// 宽松策略：不检查函数调用(INSTR_CALL)的副作用，由用户自行负责
	bool			IsReadOnlyCode(FuncState* func);
	// 编译并执行条件字符串表达式（一步完成，未预编译时使用）
	bool			EvalConditionString(const char* condstr);
	// 计算字符串表达式并返回结果值（调试器用）
	EvalResult		EvalString(const std::string& code, Value& outResult, std::string& errorDesc);
	// 只按表达式求值（自动加 return，不执行语句 fallback）
	EvalResult		EvalExpressionString(const std::string& code, Value& outResult, std::string& errorDesc);
	// 只读表达式求值（带只读检查，用于 watch/hover 等不允许修改环境的场景）
	EvalResult		EvalReadOnlyExpression(const std::string& code, Value& outResult, std::string& errorDesc);
	// 按语句块执行（不自动加 return）
	EvalResult		ExecString(const std::string& code, Value& outResult, std::string& errorDesc);

private:
	// 内部辅助：保存/恢复寄存器并执行编译后的函数
	int				CallCompiledFunc(FuncState* mainFunc, Value& outResult, std::string& errorDesc);

	void			ReturnCurFunction(EFunctionReturnType returnType, std::vector<DeferErrorInfo>* deferErrorVec = nullptr);
public:

	// ===== 协程相关接口 =====
	XScriptState*	CreateCoroutie(Function* func, ECoroutineType type);			// 创建协程
	void			ResumeCoroutie(XScriptState* threadState, int offset);// 恢复协程
	void			YieldCoroutie();							// 挂起协程
	CoroutineStatus	GetCoroutieStatus(XScriptState* xsState);	// 获取协程状态
	const char*		GetCoroutieStatusName(XScriptState* xsState);// 获取协程状态名

	// 扩展栈空间
	void			GrowStack(XScriptState* xsState, int growSize);
	// 创建C函数闭包
	Function*		CreateCFunction(int numUpvals);
	// 创建宿主C函数Value（创建函数对象、绑定函数指针、包装为Value）
	Value			CreateCHostFunction(HOST_FUNC pfnAddr, int numUpvals);
	// 获取当前C函数
	Function*		GetCurCFunction() { return mCurXScriptState->mCurrentCFunction; }

	// ===== 元方法相关 =====
	// 通过元方法执行运算
	bool			CalByTagMethod(Value* result, Value* value1, Value* value2, MetaMethodType mmtType );
	// 获取元方法名称字符串
	const char*		MetaMetodString(MetaMethodType type);
	// 获取值的元方法
	Value			GetMetaMethod(Value* value, MetaMethodType type);

	// 获取表的下一个键值对（用于迭代）
	bool			GetNextKey(TableValue* tableData, const Value &keyValue, Value& nextKey, Value& nextValue);

	// 纯净版抛出运行时错误,不再退栈处理，以及不再执行error函数，仅执行long jump
	void			ThrowErrorDirectly(const char* errorStr, ...);

	// 抛出运行时错误
	void			ExecError(const char* errorStr, ...);
	// 抛出脚本异常（可被 try/catch 捕获）
	void			ScriptThrow(const Value& value);
	// 创建脚本字符串对象
	XString*		NewXString(const char* str);
	XString*		NewXString(const char* str, int len);
	// 获取值的元表
	TableValue*		GetMetaTable(Value* value);

	// 创建列表对象
	ListValue*		CreateList();

	// ===== 对象创建 =====
	TableValue*		CreateTable();			// 创建表对象
	Function*		CreateFunction();		// 创建函数对象
	FuncState*		CreateFunctionState();	// 创建函数原型
	UpValue*		CreateUpVal();			// 创建上值对象
	XFuture*		CreateFuture();			// 创建异步Future对象


	void			CoroutieGeneratorNext(const Value& generatorTable);
private:
	// 获取操作数名称（用于错误信息）
	std::string		GetOperatorName(RuntimeOperator* op, Value* value);
	std::string		GetOperatorName(int opIndex);

	// 字符串转浮点数
	bool			CastStrToFloat(const char *s, XFloat *result);

	// 根据类名和函数名查找宿主类方法
	HostFunction*	getClassFuncByName(const std::string & className, const std::string& funcName);

	// ===== 指令执行核心 =====
	void			ExecuteFunction();	// 执行函数主循环

	void			ExecInstr_Throw(const Value& firstValue);

	void			ExecInstr_Func();		// 执行FUNC指令（函数调用）
	void			ExecInstr_Concat_To();	// 执行字符串拼接赋值
	void			ExecInstr_Logic_Not();	// 执行逻辑非
	void			ExecInstr_Logic_And();	// 执行逻辑与
	void			ExecInstr_Logic_Or();	// 执行逻辑或
	void			ExecInstr_JMP();		// 执行跳转指令
	void			ExecInstr_Concat();		// 执行字符串拼接
	void			ExecInstr_RET();		// 执行返回指令

	// 移除栈顶以上的上值引用
	void			RemoveStackUpVals(int topIndex);

	void			ExecInstr_Neg();		// 执行取负指令
	void			ExecInstr_Await();		// 执行await指令

	// ===== 函数调用机制 =====
	void			CallLuaFunction(Function* func, int numActualArgs);			// 调用Lua函数（设置栈帧）
	void			ExecLuaFunction(Function* func, int numActualArgs);			// 执行Lua函数
	bool			PrepareLuaVarArgs(Function* funcValue, int numParam);	// 打包Lua可变参数
	void			CallFunctionInLua(Function* firstValue, int numParam);// 在Lua中调用函数
	void			CallFunction(Function* funcValue, int numParam);		// 通用函数调用
	void			AdjustCallArgs(int numParam, int expectedParam);		// 调整调用参数数量（不足补nil，多余丢弃）
	void			CallHostFunc(Function* func, HOST_FUNC pfnAddr, int numParam);// 调用宿主函数
	void			OnCallAsyncFunction(Function* funcValue, int numParam);
	void			OnCallGeneratorFunction(Function* funcValue, int numPara);

	// 设置操作数的值
	void			SetOpValue(int index, const Value& value);

	// 栈帧管理
	void			pushFrame(int frameSize);	// 压入栈帧
	void			popFrame(int frameSize);	// 弹出栈帧

	// 解析表访问（读取/写入）
	void			resolveTableValue(RuntimeOperator* value, Value* resultValue);
	void			resolveSetTableValue(RuntimeOperator* value, const Value &tableVale);

	// 设置表中的键值对
	void			SetTableValue(TableValue* table, const Value &keyValue, const Value& value);
	// 查找键在表中的索引
	XInt			FindKeyIndex(TableValue* tableData, const Value &keyValue);


	// ===== 垃圾回收（标记-清除） =====
	void			MarkValue(Value* value);	// 标记值
	void			MarkTable(TABLE table);		// 标记表
	void			MarkProto(FuncState* func);	// 标记函数原型
	void			MarkScriptState(XScriptState* state);	// 标记协程执行栈
	void			MarkObjects();				// 标记阶段
	void			SweepObjects();				// 清除阶段
	void			FreeObject(CGObject* obj);	// 释放对象内存

	// 表哈希操作
	TableNode*		NewTableKey(TABLE table, const Value* key);	// 创建新键节点
	void			RehashTable(TABLE table);					// 重新哈希

	// 创建用户数据对象
	UserData*		CreateUserData( int size );

	// 调整字符串哈希表大小
	void			ResizeHashTable();

	// 值比较（用于排序）
	bool			CompareValue(Value& firstValue,Value& secondValue, bool isReverse);
	bool			CompareValueWithCompFunc(Value& firstValue,Value& secondValue, Function* func, bool isReverse);
	bool			CompareValueWithKeyFunc(Value& firstValue,Value& secondValue, Function* func, bool isReverse);

private:
	// 长跳转结构：用于保护模式下的错误恢复
	struct XScript_LongJmp
	{
		XScript_LongJmp *previous;	// 上一层保护点
		jmp_buf			j;			// 跳转缓冲区
		int				errorCode;	// 错误码
		int				errorFunc;	// 错误处理函数索引
		std::string		mErrorDesc;	// 错误描述
	};

	CGObject*									mRootCG;				// GC对象链表根节点
	std::vector<std::string>				m_stringVec;			// 字符串常量池
	std::string								mEmptyStr;				// 空字符串（用于安全返回）
	Instr*										mCurInstr;				// 当前执行的指令

	XString**									mStringHashTable;		// 字符串哈希表（字符串驻留）
	int											mStringHashSize;		// 哈希表容量
	int											mStringHashUsedSize;	// 哈希表已用数量
	std::map<std::string, UserClassData>		mUserClassDataMap;		// 用户注册类数据
	std::map<std::string, int>					mGlobalVarMap;			// 全局变量名到索引的映射
	int											mGlobalVarCount;					// 已分配的全局变量数量（只增不减）
	Value										mRegValue[MAX_FUNC_REG];// 函数返回值寄存器
	Value										mSavedRegValues[MAX_FUNC_REG];// 暂存的寄存器值（调试 evaluate 时保护，参与 GC 扫描）
	int											mSavedRegDepth;// 暂存寄存器的嵌套深度（>0 时 mSavedRegValues 有效）

	std::vector<Value>							mSavedRegValueStack;

	Value										mNilValue;				// nil值常量
	XScript_LongJmp*							mLongJmp;				// 当前错误恢复点

	char*										mStrBuffer;				// 字符串拼接缓冲区
	int											mStrBufferSize;			// 缓冲区大小

	struct AsyncTimer
	{
		unsigned long long dueTime;
		XPromise* promise;
	};
	std::queue<XScriptState*>			mAsyncReadyQueue;
	std::vector<AsyncTimer>				mAsyncTimers;
	std::vector<XPromise*>				mPendingPromises;
	std::vector<XFuture*>				mSpawnedFutures;
	std::shared_ptr<AsyncRuntime>		mAsyncRuntime;

	TABLE										mEnvTable;				// 全局环境表
	TABLE										mModuleTable;			// 已加载模块表
	TABLE										mMetaTable;				// 全局元表
	TABLE										mTableMetaTable;		// 表类型元表
	TABLE										mStringMetaTable;		// 字符串类型元表
	TABLE										mListMetaTable;			// 列表类型元表

	int											mHookMask;				// 调试钩子掩码
	bool										mAllowHook;				// 是否允许触发钩子
	XScriptDebugger*						mDebugger;				// 可选 VSCode 调试器
	int											mNumHostFuncParam;		// 当前宿主函数参数数量
	bool										mIsInCallScriptFunc;	// 是否正在从宿主调用脚本
	int											mNumScriptFuncParams;	// 脚本函数参数数量

	Value*										mGlobalStackElements;	// 全局变量栈

	XScriptState								mMainXScriptState;		// 主执行状态
	XScriptState*								mCurXScriptState;		// 当前活跃的执行状态


	std::string									GetDeferErrorDesc(std::vector<DeferErrorInfo>* deferErrorVec);

};

const char* getTypeName(int type);

bool isValueEqual(const Value& value1, const Value& value2);

inline int 	XScriptVM::AddString(const std::string& funcName)
{
	for (int i = 0; i < (int)m_stringVec.size(); i++)
	{
		if (funcName == m_stringVec[i])
			return i;
	}

	m_stringVec.push_back(funcName);
	return (int)(m_stringVec.size() - 1);

}


inline void		XScriptVM::SetOpValue(int index, const Value& value)
{
	switch (mCurInstr->mOpList[index].type)
	{
	case ROT_Stack_Index:
	{
		int stackIndex = mCurInstr->mOpList[index].iStackIndex;
		if (stackIndex >= 0 && (mEnvTable != NULL))
		{
			// 有环境表时写入环境表，保持与 ResoveStackIndexWithEnv 读取对称
			SetEnvValue(stackIndex, value);
		}
		else
		{
			Value* pValue = ResoveStackIndex(stackIndex);
			*pValue = value;
		}
		break;
	}
	case ROT_Table:
	case ROT_UpValue_Table:
	{
		resolveSetTableValue(&mCurInstr->mOpList[index], value);
		break;
	}
	case ROT_Reg:
	{
		mRegValue[mCurInstr->mOpList[index].iRegIndex] = value;
		break;
	}
	case ROT_UpVal_Index:
	{
		int stackIndex = mCurInstr->mOpList[index].iStackIndex;
		Value* pValue = mCurXScriptState->mCurFunction->funcUnion.luaFunc.mUpVals[stackIndex]->pValue;
		*pValue = value;
		break;
	}
	}
}




inline void   XScriptVM::push(const Value& value)
{
	CopyValue(&mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex], value);
	mCurXScriptState->mTopIndex++;
}


inline Value   XScriptVM::pop()
{
	mCurXScriptState->mTopIndex--;
	return mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex];
}


inline void     XScriptVM::pushFrame(int frameSize)
{
	for(int i = 0; i < frameSize; i++)
	{
		mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex + i].type = OP_TYPE_NIL;
		mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex + i].iIntValue = 0;
	}

	mCurXScriptState->mTopIndex += frameSize;
	mCurXScriptState->mFrameIndex = mCurXScriptState->mTopIndex;
}


inline void     XScriptVM::popFrame(int frameSize)
{
	mCurXScriptState->mTopIndex -= frameSize;
}

inline void     XScriptVM::setGlobalStackValue(int index, const Value& value)
{
	mGlobalStackElements[index] = value;
}


inline const Value&    XScriptVM::getStackValue(int index)
{
	return mCurXScriptState->mStackElements[resoveStackIndex(index)];
}

inline Value*		  XScriptVM::getStackValueRef(int index)
{
	return &mCurXScriptState->mStackElements[resoveStackIndex(index)];
}

inline int   XScriptVM::getNumParam()
{
	return mNumHostFuncParam;
}

inline void  XScriptVM::beginCall()
{
	_ASSERT(!mIsInCallScriptFunc);
	mIsInCallScriptFunc = true;
	mNumScriptFuncParams = 0;
}

inline void  XScriptVM::endCall()
{
	_ASSERT(mIsInCallScriptFunc);
	mIsInCallScriptFunc = false;
}

inline void  XScriptVM::pushIntParam(XInt iValue)
{
	_ASSERT(mIsInCallScriptFunc);
	if (!mIsInCallScriptFunc)
		return;
	push(ConstructValue(iValue));
	mNumScriptFuncParams++;
}

inline void  XScriptVM::pushFloatParam(XFloat fValue)
{
	_ASSERT(mIsInCallScriptFunc);
	if (!mIsInCallScriptFunc)
		return;

	push(ConstructValue(fValue));
	mNumScriptFuncParams++;
}


inline void  XScriptVM::pushStrParam(const char* strValue)
{
	_ASSERT(mIsInCallScriptFunc);
	if (!mIsInCallScriptFunc)
		return;

	push(ConstructValue(strValue));
	mNumScriptFuncParams++;
}


inline int		XScriptVM::getParamStackIndex(int paramIndex)
{
	return mCurXScriptState->mTopIndex - (mNumHostFuncParam - paramIndex);
}


inline bool	XScriptVM::getTableValue(TABLE table, XInt key, XInt& value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_INT)
		{
			value = subValue.iIntValue;
			return true;
		}
	}

	return false;
}

inline bool	XScriptVM::getTableValue(TABLE table, XInt key, XFloat& value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_FLOAT)
		{
			value = subValue.fFloatValue;
			return true;
		}
	}

	return false;
}

inline bool	XScriptVM::getTableValue(TABLE table, XInt key, char* &value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_STRING)
		{
			stringRawValue(&subValue);
			return true;
		}
	}

	return false;
}

inline Value	XScriptVM::ConstructValue(XString* str)
{
	Value tableValue;
	tableValue.stringValue = str;
	tableValue.type = OP_TYPE_STRING;
	return tableValue;
}

inline Value	XScriptVM::ConstructValue(TABLE value)
{
	Value tableValue;
	tableValue.tableData = value;
	tableValue.type = OP_TYPE_TABLE;
	return tableValue;
}

inline bool	XScriptVM::getTableValue(TABLE table, XInt key, TABLE& value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_TABLE)
		{
			value = subValue.tableData;
			return true;
		}
	}

	return false;
}

inline bool	XScriptVM::getTableValue(TABLE table, const char* key, XInt& value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_INT)
		{
			value = subValue.iIntValue;
			return true;
		}
	}

	return false;
}

inline bool	XScriptVM::getTableValue(TABLE table, const char* key, XFloat& value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_FLOAT)
		{
			value = subValue.fFloatValue;
			return true;
		}
	}

	return false;
}

inline bool	XScriptVM::getTableValue(TABLE table, const char* key, const char* &value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_STRING)
		{
			value = stringRawValue(&subValue);
			return true;
		}
	}

	return false;
}

inline bool	XScriptVM::getTableValue(TABLE table, const char* key, TABLE& value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_TABLE)
		{
			value = subValue.tableData;
			return true;
		}
	}

	return false;
}

inline bool	XScriptVM::getTableValue(TABLE table, const  char* key, ListValue*& value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_LIST)
		{
			value = subValue.listData;
			return true;
		}
	}

	return false;
}


inline bool	XScriptVM::getTableValue(TABLE table, XInt key, ListValue*& value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_LIST)
		{
			value = subValue.listData;
			return true;
		}
	}

	return false;
}


inline Value	XScriptVM::ConstructValue(XInt value)
{
	Value subValue;
	subValue.type = OP_TYPE_INT;
	subValue.iIntValue = value;
	return subValue;
}


inline Value XScriptVM::ConstructValue(FuncState* mainFunc)
{
	Function* func = CreateFunction();
	func->isCFunc = false;
	func->funcUnion.luaFunc.proto = mainFunc;
	Value fValue;
	fValue.type = OP_TYPE_FUNC;
	fValue.func = func;
	return fValue;
}

inline Value	XScriptVM::ConstructValue(XFloat value)
{
	Value subValue;
	subValue.type = OP_TYPE_FLOAT;
	subValue.fFloatValue = value;
	return subValue;
}

inline Value	XScriptVM::ConstructValue(const char* value)
{
	Value subValue;
	subValue.type = OP_TYPE_STRING;
	subValue.stringValue = NewXString(value);
	return subValue;
}

inline Value	XScriptVM::ConstructValue(XScriptState* state)
{
	Value subValue;
	subValue.type = OP_TYPE_THREAD;
	subValue.threadData = state;
	return subValue;
}

inline Value			XScriptVM::ConstructValue(ListValue* list)
{
	Value subValue;
	subValue.type = OP_TYPE_LIST;
	subValue.listData = list;
	return subValue;
}

inline Value			XScriptVM::ConstructValue(XFuture* future)
{
	Value subValue;
	subValue.type = OP_TYPE_FUTURE;
	subValue.futureData = future;
	return subValue;
}


inline TABLE	XScriptVM::newTable(int arraySize)
{
	TABLE table = CreateTable();
	if (arraySize > 0)
	{
		table->mArraySize = arraySize;
		table->mArrayData = new Value[arraySize];
	}

	return table;
}

inline void	XScriptVM::setTableValue(TABLE table, const Value& key, const Value& value)
{
	SetTableValue(table, key, value);
}

inline void	XScriptVM::setTableValue(TABLE table, XInt key, XInt value)
{
	setTableValue(table, ConstructValue(key), ConstructValue(value));
}

inline void	XScriptVM::setTableValue(TABLE table, XInt key, XFloat value)
{
	setTableValue(table, ConstructValue(key), ConstructValue(value));
}

inline void	XScriptVM::setTableValue(TABLE table, XInt key, const char* str)
{
	setTableValue(table, ConstructValue(key), ConstructValue(str));
}

inline void	XScriptVM::setTableValue(TABLE table, XInt key, TABLE t)
{
	setTableValue(table, ConstructValue(key), ConstructValue(t));
}

inline void	XScriptVM::setTableValue(TABLE table, const char* key, XInt value)
{
	setTableValue(table, ConstructValue(key), ConstructValue(value));
}

inline void	XScriptVM::setTableValue(TABLE table, const char* key, XFloat value)
{
	setTableValue(table, ConstructValue(key), ConstructValue(value));
}

inline void	XScriptVM::setTableValue(TABLE table, const char* key, const char* str)
{
	setTableValue(table, ConstructValue(key), ConstructValue(str));
}

inline void	XScriptVM::setTableValue(TABLE table, const char* key, TABLE t)
{
	setTableValue(table, ConstructValue(key), ConstructValue(t));
}

inline Value*	XScriptVM::GetGlobalValue(const std::string& varName)
{
	std::map<std::string, int>::iterator it = mGlobalVarMap.find(varName);
	if (it != mGlobalVarMap.end())
	{
		return &mGlobalStackElements[it->second];
	}

	return NULL;
}

inline Value*	XScriptVM::getReturnRegValue(int index)
{
	return &mRegValue[index];
}

inline TableValue*		XScriptVM::GetMetaTable(Value* value)
{
	switch (value->type)
	{
	case ValueType::OP_TYPE_USERDATA:
	{
		return value->userData->mMetaTable;
	}
	case ValueType::OP_TYPE_LIST:
	{
		return value->listData->mMetaTable;
	}
	case ValueType::OP_TYPE_TABLE:
	{
		return value->tableData->mMetaTable;
	}
	case ValueType::OP_TYPE_STRING:
	{
		//所有list 共用一个metatable
		return mStringMetaTable;
	}
	default:
		break;
	}
	return NULL;
}



extern XScriptVM gScriptVM;