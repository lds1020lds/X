#pragma once
class CLexer;
class CSymbolTable;
class CMidCode;
class CSourceFile;
class XScriptVM;
struct ICode;
#include <stack>
#include "CommonFunc.h"

// 语法解析器类：负责将词法分析后的Token流解析为中间码
class CParser
{
public:
	struct ParseError
	{
		int line;
		int character;
		std::string message;
	};

public:
	friend class XScriptVM;

	// 循环结构信息：记录循环的跳转目标索引，用于break/continue语句
	struct  LoopStruct
	{
		int iStartJumpIndex;        // 循环开始位置的跳转索引
		int iEndJumpIndex;          // 循环结束位置的跳转索引（break跳转目标）
		int iContinueJumpIndex;     // continue跳转目标索引
		LoopStruct(int iStartIndex, int iEndIndex, int iContinueIndex = -1)
		{
			iStartJumpIndex    = iStartIndex;
			iEndJumpIndex      = iEndIndex;
			iContinueJumpIndex = (iContinueIndex >= 0) ? iContinueIndex : iStartIndex;
		}
		LoopStruct(){}
	};

	// 最大寄存器数量
	enum 
	{
		MAX_REG_NUM = 16
	};

	// 表达式因子类型枚举
	enum EFactorType
	{
		FACTOR_INVALID,     // 无效
		FACTOR_INT,         // 整数常量
		FACTOR_FLOAT,       // 浮点数常量
		FACTOR_VAR,         // 变量
		FACTOR_TABLE,       // 表（字典）
		FACTOR_FUNC,        // 函数
		FACTOR_STRING,      // 字符串常量
		FACTOR_NIL,         // 空值
	};

	// 表达式因子：表示表达式中的一个操作数
	struct Factor
	{
		EFactorType type;       // 因子类型
		union
		{
			XInt		intValue;       // 整数值
			XFloat		floatValue;     // 浮点值
			int			varIndex;       // 变量索引
			int			strIndex;       // 字符串索引
		};
		
		int iOffset;            // 偏移量
		union
		{
			XInt		intTableValue;  // 表的整数索引值
			XFloat		fTableValue;    // 表的浮点索引值
		};

		Factor()
		{
			type = FACTOR_INVALID;
		}
	};

	// 延迟消费函数返回值保护作用域：构造时注册，析构时反注册
	struct PendingFunctionReturnFactorScope
	{
		CParser* parser;
		std::vector<Factor>& factors;

		PendingFunctionReturnFactorScope(CParser* _parser, std::vector<Factor>& _factors);
		~PendingFunctionReturnFactorScope();
	};

	// 单个延迟消费函数返回值保护作用域：用于链式调用参数递归解析期间保护 callee/accessor
	struct PendingFunctionReturnSingleFactorScope
	{
		CParser* parser;
		Factor& factor;

		PendingFunctionReturnSingleFactorScope(CParser* _parser, Factor& _factor);
		~PendingFunctionReturnSingleFactorScope();
	};

	// 因子计算结果：用于常量折叠优化
	struct FactorComputeResult
	{
		int valueType;          // 结果值类型
		union
		{
			float floatValue;   // 浮点结果
			int   intValue;     // 整数结果
		};
	};

	// 寄存器状态
	struct REG
	{
		int mVarIndex;      // 关联的变量索引
		bool bUsed;         // 是否正在使用
		int mLastIndex;     // 最后使用的指令索引
#ifdef _DEBUG
		const char* allocFile;  // 分配时的源文件名
		int allocLine;          // 分配时的源文件行号
#endif
	};

	struct RegAllocContext
	{
		REG reg[MAX_REG_NUM];
		int lastFreeIndex;
		int funcIndex;
#ifdef _DEBUG
		const char* allocFiles[MAX_REG_NUM]; // 保存各寄存器的分配文件名
		int allocLines[MAX_REG_NUM];         // 保存各寄存器的分配行号
#endif
	};

	// 链式访问结构：表示可链式访问的表达式，如"a"、"a.b"、"a[b]"、"a.b(c)"
	// subject: 被访问的主体实体（变量/函数返回值）
	// accessor: 索引键（FACTOR_INVALID 表示无索引，否则为索引键）
	struct ChainedAccess
	{
		Factor subject;   // 被访问的主体
		Factor accessor;  // 索引键（INVALID = 无索引）

		ChainedAccess() {}

		ChainedAccess(const Factor& _subject, const Factor& _accessor)
			: subject(_subject), accessor(_accessor)
		{
		}

		bool HasAccessor() const { return accessor.type != FACTOR_INVALID; }
	};

	struct TableVarPair
	{
		std::string		Key;
		std::string		VarName;
		int				ReferenceIndex = -1;
		bool			IsList = false;
		int				DefaultAssignBufferIndex = -1;
	};

	struct ListVarPair
	{
		std::string		VarName;
		int				ReferenceIndex = -1;
		bool			IsList = false;
		int				varIndex;
		int				DefaultAssignBufferIndex = -1;
	};

	struct  TableDeconstructData
	{
		std::vector<TableVarPair>	 keyVarPairVec;
	};

	struct  ListDeconstructData
	{
		std::vector<ListVarPair>	listPairVec;
	};

	struct PipeLineData
	{
		Factor		parentFactor;
		bool		autoFill; //是否是自动填充模式， 还是 _ 通配
		bool		hasFilled;//是否已经填充， autofill 只会填充第一个函数调用 一次 
	};

protected:
	CLexer*			mOriginLexer = nullptr;
	CLexer*			mLexer;         // 词法分析器
	CSymbolTable*	mSymbolTable;   // 符号表
	CMidCode*		mMidCode;       // 中间码
	int				mCurFuncIndex;  // 当前函数索引
	REG				mReg[MAX_REG_NUM];  // 寄存器数组
	int				mRegAllocFuncIndex; // 当前寄存器分配所属函数
	bool			mIsRegInUse;    // 是否有寄存器正在使用
	Factor*			mRegFactor;     // 寄存器对应的因子
	std::stack<LoopStruct>  mLoopStack; // 循环嵌套栈
	bool            mRecoverMode;   // 是否启用错误恢复模式
	std::vector<ParseError> mParseErrors; // 恢复模式收集到的错误
	int             mMaxRecoverErrors; // 最大恢复错误数
	std::vector<std::vector<Factor>*> mPendingFunctionReturnFactorStack; // 当前解析作用域中需要保护的延迟消费因子列表
	std::vector<Factor*> mPendingFunctionReturnSingleFactorStack; // 当前解析作用域中需要保护的单个延迟消费因子

	std::vector<PipeLineData> mCurPipelineFactorStack;

	int			  mLastFreeIndex;   // 最后释放的寄存器索引
	int				mCurTryCatchBlockIndex = -1;
	bool			mIsRecordToken = false;
	int				mRecordTokenDepth = 0;
	bool			mIsReadFromRecord = false;
	// 常量折叠：对两个因子进行编译期计算
	Factor ComputeFactors(int computeWay, Factor const& factor1, Factor const & factor2);
	// 根据因子类型向指令添加操作数
	void   AddOperandByFactor(int iInstrIndex, Factor ret);
	// 获取一个空闲寄存器（带调试跟踪信息的内部实现）
	int    GetFreeRegImpl(const char* file, int line);
	// 释放寄存器（带调试跟踪信息的内部实现）
	void   FreeRegImpl(int reg, const char* file, int line);
	// 检查当前是否有寄存器泄漏（仅在 _DEBUG 模式下有效）
	void   CheckRegLeak(const char* context);
	// 初始化当前函数的寄存器分配上下文
	void   InitRegAllocContextForFunction(int funcIndex);
	// 保存当前寄存器分配上下文
	RegAllocContext SaveRegAllocContext() const;
	// 恢复寄存器分配上下文
	void   RestoreRegAllocContext(const RegAllocContext& context);
	// 插入保存寄存器的代码
	void   insertSaveRegCode();
	// 更新跳转目标偏移量；includeStart=true 表示 startIndex 本身也跟随移动
	void   updateSymbolsOffset(int startIndex, int offset, int funcIndex, bool includeStart = false);
	// 记录最近一次解析错误
	void   RecordLastParseError();
	// 跳过当前错误语句，恢复到下一个安全同步点
	void   SynchronizeStatement();
	// 判断 token 是否可能是语句起点
	bool   IsStatementStartToken(int token);
	// 带错误恢复地解析一条语句
	bool   ParseStateMentWithRecovery();
public:
	CParser(void);
	~CParser(void); 

	// 报告语法错误
	void  Error(const char *errInfo, ...);
	// 报告标识符重定义错误
	void  ErrorIdentRedefine(const char* ident);
	// 检查用户声明/引用的标识符不能使用编译器内部前缀
	void  ValidateUserIdentName(const char* ident);

	// 解析源文件入口
	bool  ParseFile(CSourceFile* sourefile, CMidCode* midCode, CSymbolTable* symbol);
	// 解析源文件入口（可选错误恢复模式）
	bool  ParseFile(CSourceFile* sourefile, CMidCode* midCode, CSymbolTable* symbol, bool recoverMode);
	// 解析源文件入口（可选错误恢复模式和纯语法解析模式）
	bool  ParseFile(CSourceFile* sourefile, CMidCode* midCode, CSymbolTable* symbol, bool recoverMode, bool syntaxOnly);
	// 获取恢复模式收集到的解析错误
	const std::vector<ParseError>& GetParseErrors() const { return mParseErrors; }
	// 解析单条语句
	bool  ParseStateMent();

	// 解析标识符（变量或函数调用）
	void ParserIdent(bool bEnd = true);

	// 解析返回值列表
	void ParseRetList(std::vector<Factor> &retVec);

	// 多重赋值处理
	void MultiAssignment(std::vector<Factor> &retVec, std::vector<ChainedAccess> &varVec);

	// 解析代码块（花括号包围的语句序列）
	bool  ParseBlock();
	// 解析变量声明
	bool  ParseVar( );
	// 解析函数定义
	bool  ParseFunction(Factor assignFactor, bool isLocal = false, EFunctionType funcType = EFunctionType::Normal);

	// 解析Lambda表达式
	void  ParseLambda(Factor assignFactor);

	// 解析表达式，push表示是否将结果压栈，noTable表示是否禁止表初始化
	Factor  ParseExpr(bool push = true, bool noTable = false);
	// 解析短路表达式（&&、||、??），按优先级保持短路求值并返回原操作数
	Factor  ParseShortCircuitExpr(int minPriority = 0);

	Factor	ParsePipeline();

	// 解析子表达式（处理运算符优先级）
	Factor  ParseSubExpr();

	// 执行因子运算（常量折叠）
	void DoFactorOperation(std::vector<Factor> &retVec, std::vector<int> &opVec);
	// 解析单个因子（数字、字符串、变量等），onlyFactor=true时括号内也只解析因子不解析完整表达式
	bool  ParseFactor(Factor & factor, bool onlyFactor = false);

	void  ParseFstring(Factor& factor);

	// 解析变量和函数调用（含链式访问如a.b.c），accessOnly=true时只解析.和[]访问链，不处理()调用和:方法调用
	void  ParserVariableAndFunction(ChainedAccess& access, bool accessOnly = false);
	// 解析任意表达式后的后缀访问/调用链，如(a).b、foo()、(function(){})()，accessOnly=true时只处理.和[]访问
	void  ParsePostfix(ChainedAccess& access, bool accessOnly = false);

	// 解析赋值语句右侧表达式
	void ParserAssignRight(ChainedAccess& access, bool bEnd);

	// 添加表索引因子到指令
	void AddTableFactor(Factor &lastRet, int iInstrIndex, int& symbIndex);

	// 解析if条件语句
	bool  ParseIf();
	// 解析return语句
	bool  ParseReturnOrYield(bool isYield);
	// 解析while循环
	bool  ParseWhile();
	// 解析break语句
	bool  ParseBreak();
	// 解析continue语句
	bool  ParseContinue();
	// 解析for循环
	bool  ParseFor();

	// 解析迭代器
	bool  ParseIter();

	void  ParseTableListComprehension( int listVarIndex,  int bufferIndex, int valueBufferIndex );

	// 解析表（字典）初始化 {key=value, ...}
	int   ParseTableInit( int initReg = -1 );
	// 解析列表初始化 [a, b, c]
	int   ParseListInit(int initReg = -1);

	// 解析foreach迭代语句
	bool  ParseForeach();

	void  ParseTryCatch();

	void  ParseThrow();

	void  ParseDefer();

	void  ParseSwitchCase();

	void  ParseMatch();

	void  ParseMatchIf(int jumpInIndex);

	void  ParseMatchType(int& nextCaseJumpIndex, const Factor& factor);

	void  ParseMatchCase(Factor& matchFactor, int matchOutJumpIndex);
	
	bool  ParseMatchList(int tableVarIndex, int jumpOutIndex, std::vector<int>& bindVarVec );
	bool  ParseMatchTable(int tableVarIndex, int jumpOutIndex, std::vector<int>& bindVarVec);

	// 开始记录Token（用于延迟解析）
	int   beginRecordTokens();
	// 结束记录Token
	void  endRecordTokens();
	// 从指定缓冲区开始解析（回放记录的Token）
	void  beginParseFromBuffer(int bufferIndex);
	// 结束从缓冲区解析
	void  endParseFromBuffer();

	// 判断某指令索引是否为跳转目标
	bool  isJumpTarget(int instrIndex, int funcIndex);
	// 判断操作数在指定范围内是否被使用（用于优化）
	bool  hasOperandBeenUsed(const Operand& op, int fromIndex, int endIndex, int funcIndex);
	// 判断两个操作数是否相关（引用同一变量）
	bool  isOperandRelated(const Operand& op1, const Operand& op2);
	// 判断变量索引是否为寄存器变量
	bool  IsRegVar(int varIndex);
	// 生成赋值指令
	void  AddAssignCode(ChainedAccess& var, Factor& expr);
	// 根据链式访问添加操作数
	void  AddOperandByAccess(ChainedAccess& access, int iInstrIndex);
	// 解析函数调用参数列表，返回参数数量
	int   ParseCallArgs(int iParamStart = 0);

	// 解析函数定义的参数列表（提取自 ParseFunction/ParseLambda 的公共逻辑）
	int   ParseParamList(int funcIndex, int endToken);

	// 生成函数闭包尾部代码：恢复函数上下文 + 生成 INSTR_FUNC 指令
	// funcDefLine: 函数定义关键字所在行号，用于修正指令行号（避免调试时跳到函数体结束位置）
	void  EmitFunctionClosure(int lastFuncIndex, Factor assignFactor, Factor tableFactor = Factor(), int symbIndex = -1, int funcDefLine = -1);

	// 确保因子是变量类型（必要时生成临时变量）
	void  EnsureFactorIsVar(Factor& factor);
	// 固化延迟消费的函数返回值，避免后续函数调用覆盖共享返回寄存器
	void  StabilizePendingFunctionReturns(std::vector<Factor>& factors);
	// 注册/反注册当前解析作用域中需要保护的延迟消费因子列表
	void  RegisterPendingFunctionReturnFactors(std::vector<Factor>& factors);
	void  UnregisterPendingFunctionReturnFactors(std::vector<Factor>& factors);
	// 注册/反注册当前解析作用域中需要保护的单个延迟消费因子
	void  RegisterPendingFunctionReturnFactor(Factor& factor);
	void  UnregisterPendingFunctionReturnFactor(Factor& factor);
	// 发射函数调用前固化所有活跃作用域中延迟消费的函数返回值
	void  BeforeEmitCall();
	// 折叠表索引（优化表访问）
	void  FoldTableIndex(ChainedAccess& access);
	// 将链式访问的accessor转换为独立因子
	void  ConvertAccessorToFactor(CParser::ChainedAccess& access, CParser::Factor& factor);
public:
	// 根据符号索引查找变量名（用于调试输出）
	static char* FindVarNameBySymbolIndex(CSymbolTable& symbolTable,  int symbolIndex, int funcIndex);

	// 输出中间码到文件
	void  OutPutCode(CMidCode *, CSymbolTable*, char*);
	// 获取单条中间码的文本表示
	char* GetCodeText(const ICode  &code, int funcIndex);
	// 中间码优化
	void  optimizeCode();
	// 释放因子占用的寄存器
	void		FreeFactorReg(const Factor& factor);

	void		ParseTableDeconstruct(std::vector<TableDeconstructData>& tableVec, std::vector<ListDeconstructData>& ListVec, TableDeconstructData& tableData, bool isVarInit);
	void		ParseListDeconstruct(std::vector<TableDeconstructData>& tableVec, std::vector<ListDeconstructData>& ListVec, ListDeconstructData& listData, bool isVarInit);

	void		TableDeconstructAssign(const std::vector<TableDeconstructData>& tableVec, const std::vector<ListDeconstructData>& ListVec, const TableDeconstructData& tableData, int assignVarIndex);

	void		EmitDefaultAssignIfNil(int varIndex, int defaultAssignBufferIndex);

	void		ListDeconstructAssign(const std::vector<TableDeconstructData>& tableVec, const std::vector<ListDeconstructData>& ListVec, const ListDeconstructData& listData, int assignVarIndex);

	std::string	GetNextAnonymousFuncName();
	std::string	GetNextLamdaFuncName();
	std::string	GetNextDeforFuncName();
};

// 寄存器分配/释放宏：在调试版本中自动记录分配位置（文件名和行号）
#ifdef _DEBUG
	#define GET_FREE_REG()  GetFreeRegImpl(__FILE__, __LINE__)
	#define FREE_REG(reg)   FreeRegImpl((reg), __FILE__, __LINE__)
#else
	#define GET_FREE_REG()  GetFreeRegImpl(nullptr, 0)
	#define FREE_REG(reg)   FreeRegImpl((reg), nullptr, 0)
#endif
