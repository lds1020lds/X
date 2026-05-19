#pragma once
#ifndef VMDefs_h__
#define VMDefs_h__

#include "CommonFunc.h"
#include <map>
#include <functional>
#include "MacroDefs.h"
#include <AccCtrl.h>

#define		MAX_GLOBAL_DATASIZE				8192
#define		MAX_STACK_SIZE					0xffff
#define		MIN_STACK_SIZE					256
#define		MAX_LUA_CALL_STACK_DEPTH		1024

#define		LIST_DEFAULT_SIZE				4

#define		ITER_FUNC					"__iterator__"
#define		ITER_NEXT					"__next__"
enum
{
	MS_White = 1 << 0,
	MS_Black = 1 << 1,
	MS_Gray = 1 << 2,
	MS_Fixed = 1 << 3,
};

enum HookEvent
{
	HE_HookLine = 0,
	HE_HookCall,
	HE_HookRet,
};

// 字符串求值/执行的结果类型
enum EvalResult
{
	EVAL_OK,				// 成功
	EVAL_COMPILE_ERROR,		// 编译失败
	EVAL_RUNTIME_ERROR,		// 运行时错误
};

#define	 MASK_HOOKLINE	(1 << 0)
#define	 MASK_HOOKCALL	(1 << 1)
#define	 MASK_HOOKRET	(1 << 2)



#define  resoveStackIndex(index)		(index < 0 ? (mCurXScriptState->mFrameIndex + index) : index)

#define	 GC(o)							((CGObject*)o)
#define	 GC_SetBlack(o)					((CGObject*)o)->marked = MS_Black;
#define	 GC_SetWhite(o)					((CGObject*)o)->marked = MS_White;
#define	 GC_SetFixed(o)					((CGObject*)o)->marked = MS_Fixed;

#define  GCCommonHeader					CGObject* next;unsigned char type;unsigned char marked;		


#define	 IsValueCFunction(o)					((o)->type == OP_TYPE_FUNC && (o)->func->isCFunc)
#define	 IsValueLuaFunction(o)					((o)->type == OP_TYPE_FUNC && !(o)->func->isCFunc)

#define	 IsValueThread(o)					((o)->type == OP_TYPE_THREAD)
#define	 IsValueFunction(o)					((o)->type == OP_TYPE_FUNC)
#define	 IsValueString(o)					((o)->type == OP_TYPE_STRING)
#define	 IsValueNumber(o)					((o)->type == OP_TYPE_INT || (o)->type == OP_TYPE_FLOAT)

#define	 IsValueNil(o)						((o)->type == OP_TYPE_NIL)
#define	 IsValueInt(o)						((o)->type == OP_TYPE_INT)
#define	 IsValueFloat(o)					((o)->type == OP_TYPE_FLOAT)
#define	 IsValueTable(o)					((o)->type == OP_TYPE_TABLE)
#define	 IsValueList(o)					((o)->type == OP_TYPE_LIST)
#define	 IsValueFuture(o)				((o)->type == OP_TYPE_FUTURE)


#define	 XSetListValue(o, n)				(o)->type = OP_TYPE_LIST;(o)->listData = n;
#define	 XSetFutureValue(o, n)			(o)->type = OP_TYPE_FUTURE;(o)->futureData = n;
#define	 XSetFuncValue(o, n)				(o)->type = OP_TYPE_FUNC;(o)->func = n;
#define	 XSetIntValue(o, n)					(o)->type = OP_TYPE_INT;(o)->iIntValue = n;
#define	 SetFloatValue(o, n)				(o)->type = OP_TYPE_FLOAT;(o)->fFloatValue = n;
#define	 SetUserDataValue(o, n)				(o)->type = OP_TYPE_USERDATA;(o)->userData = n;
#define	 XSetNilValue(o)					(o)->type = OP_TYPE_NIL;(o)->iIntValue = 0;

#define	 IsValueLightUserData(o)			(((o)->type >> 16) == OP_LIGHTUSERDATA)
#define	 IsValueUserData(o)					((o)->type == OP_TYPE_USERDATA)

#define		MakeGlobalIndex(stackIndex, nameIndex)	((stackIndex << 16) + nameIndex )
#define		GlobalVarStackIndex(o)						(o >> 16)
#define		GlobalVarNameIndex(o)						(o & 0xffff)


#define  EXEC_OP_ERROR(op, op1, op2)		ExecError("attempt to perform '%s' operator on incompatible types: %s and %s", #op, GetOperatorName(op1).c_str(), GetOperatorName(op2).c_str());

#define  ExecArgsCheck(cond,op, expect)	\
		if (!(cond))	\
			ExecError("bad argument #%d to '%s' (%s expected)", op, GetOperatorName(op).c_str(), (expect));

#define  ResoveStackIndexWithEnv(index)		( index < 0 ? mCurXScriptState->mStackElements[(mCurXScriptState->mFrameIndex + index)] : (( mEnvTable != NULL) ? GetEnvValue(index) :  mGlobalStackElements[GlobalVarStackIndex(index)]))

#define  ResoveStackIndex(index)		( index < 0 ? &mCurXScriptState->mStackElements[(mCurXScriptState->mFrameIndex + index)] : &mGlobalStackElements[GlobalVarStackIndex(index)])

#define		stringRawValue(o)				((const char*)&((o)->stringValue->value))  

#define		stringRealLen(o)				strlen((const char*)&((o)->stringValue->value))  

#define		stringRawLen(o)					((o)->stringValue->len)

#define		XIsValueFalse(o)			(o.type == OP_TYPE_NIL || (IsPNumberType(o) && PNumberValue(o) == 0) || (o.type == OP_TYPE_STRING && (o).stringValue->len == 0))


#define		CheckStrBuffer(size)		\
		if (mStrBufferSize < size)	\
		{	\
			delete []mStrBuffer;	\
			mStrBufferSize = size;	\
			mStrBuffer = new char[mStrBufferSize];	\
		}	


#define CheckParam(F, N, PN, T)	\
	Value PN = vm->getParamValue(N);	\
	if (PN.type != T)	\
	{	\
		vm->ExecError("bad argument #%d to '%s': expected %s, but got %s", N, #F, getTypeName(T), getTypeName(PN.type));	\
	}

#define CheckUserTypeParam(F, N, PN, T, TN)	\
	T* PN = (T*)(vm->getParamAsObj(N, TN));	\
	if (PN == NULL)	\
	{	\
		vm->ExecError("bad argument #%d to '%s': expected usertype '%s'", N, #F, TN);	\
	}

#define CheckUserValueParam(F, N, PN, T, TN)	\
	T PN = (T)(vm->getParamAsObj(N, TN));	\
	if (PN == NULL)	\
	{	\
		vm->ExecError("bad argument #%d to '%s': expected usertype '%s'", N, #F, TN);	\
	}

class FuncState;

class CSymbolTable;
class CMidCode;

class Function;
struct ValuePair;
struct Value;
class TableValue;
struct TableNode;
class XScriptState;
class XScriptVM;
class ListValue;
class XFuture;
class XPromise;
enum CoroutineStatus
{
	CS_Normal,
	CS_Running,
	CS_Suspend,
	CS_Dead,
};

enum XSuspendReason
{
	SUSPEND_NONE,
	SUSPEND_YIELD,
	SUSPEND_AWAIT,
	SUSPEND_GEN_YIELD, //生成器暂停
};

enum class EFunctionReturnType
{
	Normal, //正常的函数调用退出
	Throw,  //抛出异常退出
	Exeception, //exec error 退出
};

enum class ECoroutineType
{
	Normal, //正常的通过coroutine 创建的协程

	AsyncTask, //async function 创建的协程

	Generator, //generator  function 创建的协程
};

class CGObject
{
public:
	GCCommonHeader;
};

struct XString
{
	GCCommonHeader;

	unsigned int	hash;
	int				len;
	char			value[1];

	bool operator ==(const XString& other)
	{
		return (hash == other.hash && len == other.len && memcmp(&other.value, &value, len) == 0);
	}
};


struct UserData
{
	GCCommonHeader;
	TableValue*		mMetaTable;
	int				mSize;
	char			value[1];
};


struct Value
{
	int type;
	union
	{
		XInt			iIntValue;
		XFloat			fFloatValue;

		int				iFunctionValue;
		int				iInstrIndex;
		int				iRegIndex;
		XString*		stringValue;
		Function*		func;
		TableValue*		tableData;
		void*			lightUserData;
		XScriptState*	threadData;
		UserData*		userData;
		ListValue*		listData;
		XFuture*		futureData;
	};

	Value()
	{
		type = OP_TYPE_NIL;
		lightUserData = NULL;
		iIntValue = 0;
	}
};

class XFuture
{
public:
	GCCommonHeader;

	enum State
	{
		Pending,
		Fulfilled,
		Rejected,
	};

	State					state;
	Value					result;
	XScriptState*			ownerState;
	std::vector<XScriptState*>	waiters;
	std::vector<std::function<void(XScriptVM*, State, const Value&)> > callbacks;

	XFuture()
		: state(Pending)
		, ownerState(NULL)
	{
	}

	bool IsCompleted() const { return state != Pending; }
	bool IsDone() const { return state == Fulfilled; }
	bool IsRejected() const { return state == Rejected;}
	void AddWaiter(XScriptState* state) { waiters.push_back(state); }
};

class XPromise
{
public:
	XScriptVM*	vm;
	XFuture*	future;

	XPromise(XScriptVM* owner, XFuture* value)
		: vm(owner)
		, future(value)
	{
	}

	void	Resolve(const Value& value);
	void	Reject(const Value& value);
};

class ListValue
{
public:
	GCCommonHeader;
	XInt			mCapacity;
	XInt			mListSize;
	Value*			mListData;
	TableValue*		mMetaTable;

	ListValue()
		: mListSize(NULL)
		, mListData(NULL)
		, mCapacity(0)
		, mMetaTable(NULL)
	{

	}
};


struct TableKey
{
	Value		keyVal;
	TableNode*	next;

	TableKey()
		: next(NULL)
	{

	}
};

struct TableNode
{
	TableKey	key;
	Value		value;
};

class TableValue
{
public:
	GCCommonHeader;
	int			mArraySize;
	Value*		mArrayData;
	//ValuePair*	nextPair;


	int			mNodeCapacity;
	TableNode*	mNodeData;
	int			lastFreePos;

	TableValue*	mMetaTable;
	TableValue()
		: mArraySize(0)
		, mArrayData(NULL)
		//	, nextPair(NULL)
		, mNodeData(NULL)
		, mNodeCapacity(0)
		, lastFreePos(0)
		, mMetaTable(NULL)
	{

	}

};


struct RuntimeSwitchData
{
	std::map<XString*, int>		strInstrMap;
	std::map<XInt, int>			intInstrMap;
	std::map<XFloat, int>		floatInstrMap;
	int							defaultInstr; //默认分支
};



class UpValue
{
public:
	GCCommonHeader;

	Value*		pValue;
	UpValue*	nextValue;
	Value		value;
	UpValue()
		: nextValue(NULL)
		, pValue(NULL)
	{


	}

};

class ConstantData
{
public:
	std::string		constantName;
	int				constantValue;

	ConstantData(const std::string& name, XInt value)
		: constantName(name), constantValue((int)value)
	{

	}
};


class HostFunction
{
public:
	std::string funcName;
	HOST_FUNC	pfnAddr;

	HostFunction(const std::string&	 _funcName, HOST_FUNC _pfn)
		: funcName(_funcName)
		, pfnAddr(_pfn)
	{

	}
};

class CFunction
{
public:
	Value*				mUpVal;
	int					mNumUpVal;
	HOST_FUNC			pfnAddr;

};

class LuaFunction
{
public:
	FuncState*					proto;
	UpValue**					mUpVals;
	int							mNumUpVals;
};

typedef union FuncUnion
{
	CFunction		cFunc;
	LuaFunction		luaFunc;
}FuncUnion;


class Function
{
public:
	GCCommonHeader;

	bool				isCFunc;
	FuncUnion			funcUnion;
	Function()
	{
		isCFunc = false;
	}
};



struct RuntimeOperator
{
	int type;
	union
	{
		XInt		iIntValue;
		XFloat		fFloatValue;
		int			iStackIndex;
		int			iFunctionValue;
		int			iInstrIndex;
		XString*	stringValue;
		int			iRegIndex;
	};
	int				tableIndexType;
	union
	{
		XInt		iIntTableValue;
		XFloat		fFloatTableValue;
		XString*	strTableValue;
	};

	int		varNameIndex;

	RuntimeOperator()
	{
		varNameIndex = -1;
		type = -1;
		tableIndexType = 0;
		iIntTableValue = 0;
	}
};



struct ValuePair
{
	Value key;
	Value value;
	ValuePair* nextPair;
	ValuePair()
	{
		nextPair = NULL;
	}
};

struct Instr
{
	int opType;
	int numOpCount;
	RuntimeOperator* mOpList;
	int lineIndex;
};

class InstrStream
{
public:
	int numInstr;
	Instr* instrs;
	bool mHasJump;
};




class FuncState
{
public:
	GCCommonHeader;

	int	iIndex;
	InstrStream mInstrStream;
	int localDataSize;
	int localParamNum;
	int stackFrameSize;
	bool hasVarArgs;
	EFunctionType funcType;
	std::string funcName;
	std::string	sourceFileName;
	unsigned int sourceFileHash;
	std::vector<FuncState*>	m_subFuncVec;
	FuncState*				m_parentFunc;
	std::vector<UpValueST>	m_upValueVec;

	std::vector<std::string>		m_localVarVec;
	std::vector<LocalVarDebugInfo>	m_localVarDebugInfoVec;

	std::vector<TryCatchBlock>		m_tryCatchBlock;
	std::vector<RuntimeSwitchData>	mSwitchVec;
	FuncState()
	{
		iIndex = -1;
		localDataSize = 0;
		localParamNum = 0;
		stackFrameSize = 0;
		hasVarArgs = false;
		funcType =	EFunctionType::Normal;
		sourceFileHash = 0;
		m_parentFunc = NULL;
	}
};

class UserClassData
{
public:
	std::string					mClassName;
	std::string					mBassClassName;
	std::vector<HostFunction>	mHostFunctions;
};

class CallInfo
{
public:
	FuncState*				mCurFunctionState;
	Function*				mCurFunction;
	int						mInstrIndex;
	int						mCurLine;
	int						mFrameIndex;
	int						mCurTryCatchIndex;
	int						mNumActualArgs;
	std::vector<Function*>	mDeferFuncVec;		
};

struct DeferErrorInfo
{
	int					callIndex;
	int					deferIndex;
	std::string			errorDesc;
};

#endif // VMDefs_h__