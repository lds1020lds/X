#include "XScriptVM.h"
#include "XScriptDebugger.h"
#include <windows.h>

void XScriptVM::SetCurrentXScriptState(XScriptState* state)
{
	if (mCurXScriptState == state)
		return;

	if (mDebugger != NULL)
		mDebugger->OnScriptStateSwitch();

	mCurXScriptState = state;
}

// 创建协程：分配新的执行状态
XScriptState*	XScriptVM::CreateCoroutie(Function* func, ECoroutineType type)
{
	XScriptState* state = new XScriptState();
	resetScriptState(state);
	state->mCoroutineType = type;
	state->mStartFunction = func;
	return state;
}


// 恢复协程执行：传递参数并切换执行状态
void	XScriptVM::ResumeCoroutie(XScriptState* threadState, int offset)
{
	if (GetCoroutieStatus(threadState) != CS_Suspend)
	{
		setReturnAsInt(0, 0);
		char errorDesc[128] = { 0 };
		snprintf(errorDesc, 128, "cannot resume coroutine: current status is '%s'", GetCoroutieStatusName(threadState));
		setReturnAsStr(errorDesc, 1);
	}
	else
	{
		if (threadState->mSuspendReason == SUSPEND_AWAIT)
		{
			setReturnAsInt(0, 0);
			setReturnAsStr("cannot resume coroutine: coroutine is waiting for future", 1);
			return;
		}

		if (threadState->mSuspendReason == SUSPEND_GEN_YIELD)
		{
			setReturnAsInt(0, 0);
			setReturnAsStr("cannot resume coroutine: coroutine is for generator", 1);
			return;
		}

		if (threadState->mStatus == CS_Normal)
		{
			for (int i = offset; i < mNumHostFuncParam; i++)
			{
				threadState->mStackElements[threadState->mTopIndex] = getParamValue(i);
				threadState->mTopIndex++;
			}

			XScriptState* lastState = mCurXScriptState;

			SetCurrentXScriptState(threadState);

			std::string errorDesc;
			int status = ProtectCallFunction(threadState->mStartFunction, mNumHostFuncParam - offset, errorDesc);

			if (status == 0)
			{
				if (threadState->mStatus == CS_Normal)
				{
					for (int i = MAX_FUNC_REG - 1; i > 0; i--)
					{
						mRegValue[i] = mRegValue[i - 1];
					}

					threadState->mStatus = CS_Dead;
				}

				setReturnAsInt(1, 0);
			}
			else
			{
				mCurXScriptState->mStatus = CS_Dead;
				resetReturnValue();
				setReturnAsInt(0, 0);
				setReturnAsStr(errorDesc.c_str(), 1);
			}

			SetCurrentXScriptState(lastState);
		}
		else if (threadState->mStatus == CS_Suspend)
		{
			threadState->mSuspendReason = SUSPEND_NONE;
			for (int i = offset; i < mNumHostFuncParam; i++)
			{
				mRegValue[i - offset] = getParamValue(i);
			}

			XScriptState* lastState = mCurXScriptState;
			SetCurrentXScriptState(threadState);
			mCurXScriptState->mStatus = CS_Normal;

			std::string errorDesc;
			int status = ProtectResume(errorDesc);
			if (status == 0)
			{
				if (threadState->mStatus == CS_Normal)
				{
					for (int i = MAX_FUNC_REG - 1; i > 0; i--)
					{
						mRegValue[i] = mRegValue[i - 1];
					}

					threadState->mStatus = CS_Dead;
				}

				setReturnAsInt(1, 0);
			}
			else
			{
				mCurXScriptState->mStatus = CS_Dead;
				resetReturnValue();

				setReturnAsInt(0, 0);
				setReturnAsStr(errorDesc.c_str(), 1);
			}

			SetCurrentXScriptState(lastState);
		}
	}

}

// 挂起协程：保存返回值并设置挂起状态
void	XScriptVM::YieldCoroutie()
{
	if (mCurXScriptState == &mMainXScriptState)
	{
		ExecError("cannot yield from main thread: yield must be called inside a coroutine");
	}
	if (mCurXScriptState->mCCallIndex > 2)
	{
		ExecError("cannot yield across C-call boundary: ensure yield is not called from within a C function");
	}
	if (mCurXScriptState->mCoroutineType != ECoroutineType::Normal)
	{
		ExecError("can only yield manually inside coroutine.create function");
	}


	for (int i = 0; i < mNumHostFuncParam; i++)
	{
		mRegValue[i + 1] = getParamValue(i);
	}

	mCurXScriptState->mStatus = CS_Suspend;
	mCurXScriptState->mSuspendReason = SUSPEND_YIELD;

}

bool XScriptVM::IsCurrentAsyncTask() const
{
	return mCurXScriptState != NULL && mCurXScriptState->mCoroutineType == ECoroutineType::AsyncTask;
}

void XScriptVM::AsyncReady(XScriptState* state, const Value& value, bool isRejected)
{
	if (state == NULL)
		return;
	state->mAwaitResumeValue = value;
	state->mHasAwaitResumeValue = true;
	state->mIsRejected = isRejected;
	mAsyncReadyQueue.push(state);
}

void XScriptVM::AddPendingPromise(XPromise* promise)
{
	if (promise != NULL)
		mPendingPromises.push_back(promise);
}

void XScriptVM::RemovePendingPromise(XPromise* promise)
{
	for (std::vector<XPromise*>::iterator it = mPendingPromises.begin(); it != mPendingPromises.end(); ++it)
	{
		if (*it == promise)
		{
			mPendingPromises.erase(it);
			return;
		}
	}
}

void XScriptVM::QueueAsyncCompletion(const std::function<void(XScriptVM*)>& completion)
{
	QueueAsyncCompletion(GetAsyncRuntime(), completion);
}

bool XScriptVM::QueueAsyncCompletion(std::weak_ptr<AsyncRuntime> runtime, const std::function<void(XScriptVM*)>& completion)
{
	std::shared_ptr<AsyncRuntime> asyncRuntime = runtime.lock();
	if (!asyncRuntime)
		return false;

	std::lock_guard<std::mutex> lock(asyncRuntime->mutex);
	if (!asyncRuntime->alive)
		return false;

	asyncRuntime->completions.push(completion);
	return true;
}

std::weak_ptr<AsyncRuntime> XScriptVM::GetAsyncRuntime() const
{
	return mAsyncRuntime;
}

void XScriptVM::SpawnFuture(XFuture* future)
{
	if (future == NULL)
		return;
	for (int i = 0; i < (int)mSpawnedFutures.size(); i++)
	{
		if (mSpawnedFutures[i] == future)
			return;
	}
	mSpawnedFutures.push_back(future);
}

void XScriptVM::AddAsyncTimer(int ms, XPromise* promise)
{
	AsyncTimer timer;
	timer.dueTime = GetTickCount64() + (unsigned long long)(ms < 0 ? 0 : ms);
	timer.promise = promise;
	AddPendingPromise(promise);
	mAsyncTimers.push_back(timer);
}

void XScriptVM::AsyncTick()
{
	std::queue<std::function<void(XScriptVM*)> > completions;
	if (mAsyncRuntime)
	{
		std::lock_guard<std::mutex> lock(mAsyncRuntime->mutex);
		std::swap(completions, mAsyncRuntime->completions);
	}
	while (!completions.empty())
	{
		completions.front()(this);
		completions.pop();
	}

	unsigned long long now = GetTickCount64();
	for (int i = 0; i < (int)mAsyncTimers.size();)
	{
		if (mAsyncTimers[i].dueTime <= now)
		{
			XPromise* promise = mAsyncTimers[i].promise;
			mAsyncTimers.erase(mAsyncTimers.begin() + i);
			if (promise != NULL)
			{
				promise->Resolve(mNilValue);
				RemovePendingPromise(promise);
				delete promise;
			}
		}
		else
		{
			i++;
		}
	}

	for (int i = 0; i < (int)mSpawnedFutures.size();)
	{
		if (mSpawnedFutures[i] == NULL || mSpawnedFutures[i]->IsCompleted())
			mSpawnedFutures.erase(mSpawnedFutures.begin() + i);
		else
			i++;
	}

	while (!mAsyncReadyQueue.empty())
	{
		XScriptState* state = mAsyncReadyQueue.front();
		mAsyncReadyQueue.pop();
		if (state == NULL || state->mStatus == CS_Dead)
			continue;

		XScriptState* lastState = mCurXScriptState;
		SetCurrentXScriptState(state);
		mCurXScriptState->mStatus = CS_Normal;
		mCurXScriptState->mSuspendReason = SUSPEND_NONE;
		if (mCurXScriptState->mHasAwaitResumeValue)
		{
			mRegValue[0] = mCurXScriptState->mAwaitResumeValue;
			mCurXScriptState->mHasAwaitResumeValue = false;
		}

		int errCode = 0;
		std::string errorDesc;
		if (mCurXScriptState->mCurFunctionState == NULL && mCurXScriptState->mStartFunction != NULL)
		{
			errCode = ProtectCallFunction(mCurXScriptState->mStartFunction, mCurXScriptState->mAsyncStartParamCount, errorDesc);
		}
		else
		{
			errCode = ProtectResume(errorDesc);
		}
		
		if (errCode != 0 && mCurXScriptState->mOwnerTask)
		{
			RejectFuture(mCurXScriptState->mOwnerTask, ConstructValue(errorDesc.c_str()));
		}

		if ((mCurXScriptState->mStatus == CS_Normal && mCurXScriptState->mCurFunctionState == NULL) || errCode != 0)
			mCurXScriptState->mStatus = CS_Dead;


		SetCurrentXScriptState(lastState);
	}
}

void	XScriptVM::BeforeExecuteResume()
{
	//如果异步等待被reject， 抛出一个异常
	if (mCurXScriptState->mIsRejected)
	{
		ExecInstr_Throw(mCurXScriptState->mAwaitResumeValue);
	}
}

void XScriptVM::RunUntilComplete(XFuture* future)
{
	while (future != NULL && !future->IsCompleted())
	{
		AsyncTick();
		Sleep(1);
	}
}

// 获取协程状态名称字符串
const char*		XScriptVM::GetCoroutieStatusName(XScriptState* xsState)
{
	static const char *const statNames[] =
	{ "normal", "running", "suspend", "dead" };

	return statNames[GetCoroutieStatus(xsState)];
}

// 获取协程状态
CoroutineStatus	XScriptVM::GetCoroutieStatus(XScriptState* xsState)
{
	if (xsState->mStatus == CS_Normal)
	{
		if (xsState == mCurXScriptState)
		{
			return CS_Running;
		}
		else
			return CS_Suspend;
	}
	else
		return (CoroutineStatus)xsState->mStatus;
}


// 扩展栈空间：分配更大的栈并迁移数据，更新上值指针
void	XScriptVM::GrowStack(XScriptState* xsState, int growSize)
{
	int newSize = xsState->mStackSize * 2;

	if (growSize > xsState->mStackSize)
	{
		newSize = xsState->mStackSize + growSize;
	}

	Value* newStackElem = new Value[newSize];

	memcpy(newStackElem, xsState->mStackElements, sizeof(Value) * xsState->mStackSize);

	for (int i = xsState->mStackSize; i < newSize; i++)
	{
		newStackElem[i].type = OP_TYPE_NIL;
		newStackElem[i].iIntValue = 0;
	}

	UpValue* nextUpVals = xsState->mNextRefUpVals;
	while (nextUpVals != NULL)
	{
		nextUpVals->pValue = newStackElem - xsState->mStackElements + nextUpVals->pValue;
		nextUpVals = nextUpVals->nextValue;
	}
	delete []xsState->mStackElements;

	xsState->mStackSize = newSize;
	xsState->mStackElements = newStackElem;
}
