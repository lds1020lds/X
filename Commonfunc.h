#ifndef  COMMON_FUNC_H_
#define  COMMON_FUNC_H_

#include <vector>
#include <string>
#include <map>
#ifndef TRUE
#define  TRUE	1
#endif
#ifndef FALSE
#define  FALSE	0
#endif

// ============================================================================
// 关键数值配置区 (可根据需要调整以下数值)
// ============================================================================

#define		MAX_FUNC_REG			8		// 函数寄存器数量
#define		MAX_ERRORINFO_SIZE		100		// 错误信息缓冲区最大长度
#define		MAX_NUMBER_STRING_SIZE	(64)	// 数字转字符串的最大长度
#define		MAX_INSTR_NAME_SIZE		(16)	// 指令名称最大长度
#define		MAX_TOKEN_NAME_SIZE		(32)	// Token名称最大长度
#define		MAX_IDENT_SIZE			(256)	// 标识符(变量名/函数名等)最大长度
#define		MAX_STRING_SIZE			(256)	// 字符串字面量最大长度
#define		MAX_FUNC_NAME_SIZE		64		// 函数名最大长度
#define		MAX_PARAM_NUM			128		// 函数最大参数数量
#define		MAX_TAGMETHOD_LOOP		10		// 元方法最大递归调用深度
#define		UPVALMASK				0x1000000	// UpValue索引掩码


#define USE_HIGH_PRECIOUS_NUMBER		//是否使用高精度数值
// ============================================================================

enum ICodeNodeType
{
	ICODE_NODE_INSTR = 0,        // 指令节点
	ICODE_NODE_SOURCE_LINE = 1,  // 源码行标注
	ICODE_NODE_JUMP_TARGET = 2,  // 跳转目标
};

// ---- I-Code Instruction Opcodes --------------------------------------------------------

enum InstrCode
{
	INSTR_MOV,
	INSTR_ADD,
	INSTR_SUB,
	INSTR_MUL,
	INSTR_DIV,
	INSTR_MOD,
	INSTR_EXP,
	INSTR_NEG,
	INSTR_INC,
	INSTR_DEC,
	INSTR_AND,
	INSTR_OR,
	INSTR_XOR,
	INSTR_SHL,
	INSTR_SHR,
	INSTR_CONCAT,
	INSTR_JMP,
	INSTR_JE,
	INSTR_JNE,
	INSTR_JG,
	INSTR_JL,
	INSTR_JGE,
	INSTR_JLE,
	INSTR_JFALSE,
	INSTR_JTRUE,
	INSTR_PUSH,
	INSTR_POP,
	INSTR_CALL,
	INSTR_RET,
	INSTR_TYPE,
	INSTR_ADD_TO,
	INSTR_SUB_TO,
	INSTR_MUL_TO,
	INSTR_DIV_TO,
	INSTR_MOD_TO,
	INSTR_EXP_TO,
	INSTR_AND_TO,
	INSTR_OR_TO,
	INSTR_XOR_TO,
	INSTR_NOT_TO,
	INSTR_SHL_TO,
	INSTR_SHR_TO,
	INSTR_CONCAT_TO,
	INSTR_LOGIC_NOT,
	INSTR_TEST_E,
	INSTR_TEST_NE,
	INSTR_TEST_G,
	INSTR_TEST_L,
	INSTR_TEST_GE,
	INSTR_TEST_LE,
	INSTR_LOGIC_AND,
	INSTR_LOGIC_OR,
	INSTR_FUNC,
	INSTR_LOADNIL,
	INSTR_APPEND,
	INSTR_AWAIT,
	INSTR_ENTERCATCH,
	INSTR_EXITCATCH,
	INSTR_THROW,
	INSTR_J_ARGS_G, //参数数量大于操作数后跳转
	INSTR_DEFER, //绑定defer函数
	INSTR_YIELD,  //yield
	INSTR_SWITCH,
	INSTR_J_NOT_TYPE, //不是某种类型就调整
};

enum TokenType
{
	TOKEN_TYPE_END_OF_STREAM,
	TOKEN_TYPE_INVALID,
	TOKEN_TYPE_INT,
	TOKEN_TYPE_FLOAT,
	TOKEN_TYPE_IDENT,
	TOKEN_TYPE_RSRVD_VAR,
	TOKEN_TYPE_RSRVD_TRUE,
	TOKEN_TYPE_RSRVD_FALSE,
	TOKEN_TYPE_RSRVD_IF,
	TOKEN_TYPE_RSRVD_ELSE,
	TOKEN_TYPE_RSRVD_BREAK,
	TOKEN_TYPE_RSRVD_CONTINUE,
	TOKEN_TYPE_RSRVD_FOR,
	TOKEN_TYPE_RSRVD_WHILE,
	TOKEN_TYPE_RSRVD_FUNC,
	TOKEN_TYPE_RSRVD_RETURN,
	TOKEN_TYPE_RSRVD_NIL,
	TOKEN_TYPE_OP,
	TOKEN_TYPE_DELIM_COMMA,			//,
	TOKEN_TYPE_DELIM_OPEN_PAREN,//(
	TOKEN_TYPE_DELIM_CLOSE_PAREN,//)
	TOKEN_TYPE_DELIM_OPEN_BRACE,	//[
	TOKEN_TYPE_DELIM_CLOSE_BRACE, //]
	TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE, //{
	TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE,//}
	TOKEN_TYPE_DELIM_SEMICOLON,			//；
	TOKEN_TYPE_DELIM_JINGHAO,
	TOKEN_TYPE_DELIM_COLON,				//：
	TOKEN_TYPE_DELIM_INTERROGATION,
	TOKEN_TYPE_DELIM_POINT,
	TOKEN_TYPE_DELIM_OPTIONAL_DOT,		// ?. 可选链字段访问
	TOKEN_TYPE_DELIM_OPTIONAL_CALL,		// ?( 可选链函数调用
	TOKEN_TYPE_DELIM_OPTIONAL_INDEX,	// ?[ 可选链索引访问
	TOKEN_TYPE_DELIM_OPTIONAL_COLON,	// ?: 可选链方法调用
	TOKEN_TYPE_DELIM_THREE_POINT,
	TOKEN_TYPE_STRING,
	TOKEN_TYPE_RSRVD_FOREACH,
	TOKEN_TYPE_RSRVD_IN,
	TOKEN_TYPE_RSRVD_LAMBDA,
	TOKEN_TYPE_RSRVD_OR,
	TOKEN_TYPE_RSRVD_ASYNC,
	TOKEN_TYPE_RSRVD_AWAIT,
	TOKEN_TYPE_RSRVD_TRY,
	TOKEN_TYPE_RSRVD_CATCH,
	TOKEN_TYPE_RSRVD_THROW,
	TOKEN_TYPE_RSRVD_FINALLY,
	TOKEN_TYPE_RSRVD_DEFER, //函数退出前调用
	TOKEN_TYPE_FSTRING,		//fstring
	TOKEN_TYPE_RSRVD_ITERATOR, //iterator 循环迭代器
	TOKEN_TYPE_RSRVD_GENERATOR, //generator 生成器
	TOKEN_TYPE_RSRVD_YIELD, //yield
	TOKEN_TYPE_RSRVD_SWITCH,
	TOKEN_TYPE_RSRVD_CASE,
	TOKEN_TYPE_RSRVD_DEFAULT,
	TOKEN_TYPE_RSRVD_MATCH,			//match
	TOKEN_TYPE_DELM_UNDERSCORE,			//_
	TOKEN_TYPE_DELM_FATARROW,		//=>
};



enum OpType
{
	// ---- 算术运算符
	OP_TYPE_ADD = 0,                // +
	OP_TYPE_SUB = 1,                // -
	OP_TYPE_MUL = 2,                // *
	OP_TYPE_DIV = 3,                // /
	OP_TYPE_MOD = 4,                // %
	OP_TYPE_EXP = 5,                // ^
	OP_TYPE_CONCAT = 35,            // $

	// ---- 自增自减
	OP_TYPE_INC = 15,               // ++
	OP_TYPE_DEC = 17,               // --

	// ---- 赋值运算符
	OP_TYPE_ASSIGN = 11,            // =
	OP_TYPE_ASSIGN_ADD = 14,        // +=
	OP_TYPE_ASSIGN_SUB = 16,        // -=
	OP_TYPE_ASSIGN_MUL = 18,        // *=
	OP_TYPE_ASSIGN_DIV = 19,        // /=
	OP_TYPE_ASSIGN_MOD = 20,        // %=
	OP_TYPE_ASSIGN_EXP = 21,        // ^=
	OP_TYPE_ASSIGN_CONCAT = 36,     // $=

	// ---- 位运算符
	OP_TYPE_BITWISE_AND = 6,        // &
	OP_TYPE_BITWISE_OR = 7,         // |
	OP_TYPE_BITWISE_XOR = 8,        // #
	OP_TYPE_BITWISE_NOT = 9,        // ~
	OP_TYPE_BITWISE_SHIFT_LEFT = 30,  // <<
	OP_TYPE_BITWISE_SHIFT_RIGHT = 32, // >>

	// ---- 位赋值运算符
	OP_TYPE_ASSIGN_AND = 22,        // &=
	OP_TYPE_ASSIGN_OR = 24,         // |=
	OP_TYPE_ASSIGN_XOR = 26,        // #=
	OP_TYPE_ASSIGN_SHIFT_LEFT = 33, // <<=
	OP_TYPE_ASSIGN_SHIFT_RIGHT = 34,// >>=

	// ---- 逻辑运算符
	OP_TYPE_LOGICAL_AND = 23,       // &&
	OP_TYPE_LOGICAL_OR = 25,        // ||
	OP_TYPE_LOGICAL_NOT = 10,       // !
	OP_TYPE_NULL_COALESCING = 45,   // ??

	// ---- 关系运算符
	OP_TYPE_EQUAL = 28,             // ==
	OP_TYPE_NOT_EQUAL = 27,         // !=
	OP_TYPE_LESS = 12,              // <
	OP_TYPE_GREATER = 13,           // >
	OP_TYPE_LESS_EQUAL = 29,        // <=
	OP_TYPE_GREATER_EQUAL = 31,     // >=

	// ---- 其他
	OP_TYPE_POINTER = 40,           // :
	OP_TYPE_DOUBLECOLON = 41,       // ::
	OP_TYPE_FATARROW = 42,          // =>
	OP_TYPE_PIPE = 43,              // |>
	OP_TYPE_PIPE_RIGHT = 44,        // |>>
};



enum RuntimeOperatorType
{
	ROT_Int,
	ROT_Float,
	ROT_String,
	ROT_Table,
	ROT_UpValue_Table,
	ROT_UpVal_Index,
	ROT_Stack_Index,
	ROT_Instr_Index,
	ROT_Reg,
	ROT_Nil,
};

enum ParseOperandType
{
	PST_Int,
	PST_Float,
	PST_String_Index,
	PST_Var_Index,
	PST_JumpTarget_Index,
	PST_Reg,
	PST_Table,
	PST_Nil,
};

enum ValueType
{
	OP_TYPE_NIL = -1,
	OP_TYPE_INT = 0,
	OP_TYPE_FLOAT,
	OP_TYPE_STRING,
	OP_TYPE_TABLE,
	OP_LIGHTUSERDATA,
	OP_TYPE_FUNC,
	OP_TYPE_PROTO,
	OP_TYPE_UPVAL,
	OP_TYPE_THREAD,
	OP_TYPE_USERDATA,
	OP_TYPE_LIST,
	OP_TYPE_FUTURE,
};


enum MetaMethodType
{
	MMT_Index,
	MMT_NewIndex,
	MMT_Equal,
	MMT_Add,
	MMT_Sub,
	MMT_Mul,
	MMT_Div,
	MMT_Mod,
	MMT_Pow,
	MMT_Neg,
	MMT_Len,
	NMT_Less,
	NMT_LessEqual,
	MMT_Concat,
	MMT_Call,
	MMT_Great,
	MMT_GreatEqual,
	MMT_ToString,
	MMT_Count,
};



enum InstrType
{
	INSTR_TYPE_CODE = 1,
	INSTR_TYPE_JUMPTARGET = 2,
};


enum EVarType
{
	IDENT_TYPE_PARAM = 1,
	IDENT_TYPE_VAR = 2,
	IDENT_TYPE_INTERNAL_VAR = 3,
};
#define		XSCRIPT_INTERNAL_IDENT_PREFIX "__xscript_internal_"

#define		IsUserType(type)		((type >> 16) == OP_LIGHTUSERDATA)
#define		UserDataType(type)		(type & 0xffff)
#define		MAKE_USERTYPE(index)	((OP_LIGHTUSERDATA << 16) + index)

#define		ARGS					("__xscript_varargs")

#if _MSC_VER <= 1700
#define snprintf	sprintf_s
#define strtoll		_strtoi64
#define atoll		_atoi64
#endif

#ifdef USE_HIGH_PRECIOUS_NUMBER

typedef	signed long long		XInt;
typedef	double					XFloat;

#define		XIntConFmt			"%lld"
#define		XFloatConFmt		"%lf"
#define		XIntConFmt2			"[%lld]"
#define		XFloatConFmt2		"[%lf]"
#define		StrToXInt			atoll
#define		StrToXFloat			atof

#define		StrToXIntWithBase			strtoll
#define		XFMod				fmod
#else

typedef		int				XInt;
typedef		float					XFloat;
#define		XIntConFmt			"%d"
#define		XFloatConFmt		"%f"
#define		XIntConFmt2			"[%d]"
#define		XFloatConFmt2		"[%f]"
#define		StrToXInt			atoi
#define		StrToXFloat			(float)atof
#define		XFMod				fmodf
#define		StrToXIntWithBase			strtol

#endif

enum
{
	VLOCAL,
	VUPVALUE,
	VGLOBAL,
};

enum class EFunctionType
{
	Normal, //普通函数
	Async,  //async函数
	Generator, //生成器函数
};

struct Operand
{
	ParseOperandType operandType;
	union
	{
		XInt		iIntValue;
		XFloat		fFloatValue;
		int			iSymbolIndex;
		int			iJumppIndex;
		int			iStringIndex;
		int			iRegIndex;
		int			iFunData;
	};

	int tableIndexType;

	union 
	{
		XInt		iIntTableValue;
		XFloat		fFloatTableValue;
	};

	Operand()
	{
		tableIndexType = 0;
	}
};

struct  Code
{
	int iCodeOpr;
	std::vector<Operand> oprList;
};

struct JumpTarget
{
	int jumpIndex;
	int codeIndex;
};

struct ICode
{
	int iCodeType;
	Code code;
	int  JumpIndex;
	int	 lineIndex;
};

class XScriptVM;
typedef void(*HOST_FUNC)(XScriptVM* vm);

struct LocalVarDebugInfo
{
	std::string name;
	int localIndex;
	int startPc;
	int endPc;

	LocalVarDebugInfo()
		: localIndex(-1)
		, startPc(0)
		, endPc(-1)
	{
	}
};

struct TryCatchBlock
{
	int		funcIndex;
	int		blockIndex;
	int		parentBlockIndex;
	int		startpc;
	int		catchpc;
	int		finallypc;
	int     outpc;
};



struct VariantST
{
	int  iIndex;
	char varName[MAX_IDENT_SIZE];
	int  iScope;
	int  iType;
	int  stackIndex;
	int  blockLevel;  // block scope level (0 = function level, >0 = nested block)
	int  debugStartPc;
	int  debugEndPc;
	std::vector<Operand> initValues;
};

class UpValueST
{
public:
	UpValueST(int _type, int _index, const char* _name = "")
		: type(_type), index(_index), name(_name ? _name : "")
	{

	}
	int type;
	int index;
	std::string name;
};


struct SwitchStruct
{
	std::map<XFloat, int>				floatValueInstrMap;
	std::map<XInt, int>					intValueInstrMap;
	std::map<std::string, int>			stringValueInstrMap;
	int									defaultInstr = -1; //默认分支
};

struct FunctionST
{
	int					iIndex;
	char				funcName[MAX_IDENT_SIZE];
	int					iParamSize;
	int					localDataSize;
	int					curParamIndex;
	bool				hasVarArgs;
	EFunctionType		funcType;
	int					curVarIndex;
	int					parentIndex;
	std::vector<int>	subIndexVec;
	std::vector<UpValueST>	upValueVec;
	std::vector<TryCatchBlock> tryCatchBlockVec; //异常块

	std::vector<SwitchStruct> switchStructVec;
};


struct HostFunctionST
{
	int		    iIndex;
	char		funcName[MAX_IDENT_SIZE];
	int			iParamSize;
	HOST_FUNC	pfnAddr;
};

struct StringST
{
	int         iIndex;
	char        str[MAX_STRING_SIZE];
};




inline std::string ConvertToString(const int n)
{
	char text[32] = { 0 };
	snprintf(text, 32, "%d", n);
	return text;
}




int IsCharDelim ( char cChar );
int IsCharWhitespace ( char cChar );
int IsCharNumeric ( char cChar );
int IsCharIdent ( char cChar );
int IsCharOpChar (char curChar, int preIndex, int iLevel);

// High-byte characters (>= 0x80) are handled by IsCharIdent directly.
// Since SourceFile.cpp converts GBK to UTF-8 on load, all multi-byte
// characters in the lexer are UTF-8. Their bytes are all >= 0x80,
// so IsCharIdent naturally accepts them without any special logic.

extern char gLastParseErrorInfo[512];
extern int gLastParseErrorLine;
extern int gLastParseErrorChar;

void ExitOnError(const char* info, int iLine, int iChar);
int  GetOprType(int iLevel,  int iIndex);
bool IsRelativeOpr(int opr);
bool IsLogicalOpr(int opr);

int	GetValueTypeMask(int valueType);
bool MatchValueType(int valueType, int mask);
int	GetValueMaskByName(const std::string&  name);

#endif
