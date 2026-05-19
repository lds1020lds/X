#include "XScriptVM.h"

// 收缩列表内存
void	XScriptVM::ListShrink(ListValue* list, XInt size)
{
	//if (list->mListSize < list->mCapacity - 1)
	{
		XInt oldSize = list->mCapacity;
		XInt newSize = size > 0 ? size : oldSize / 2;
		Value* newList = new Value[(size_t)newSize];

		memcpy(newList, list->mListData, sizeof(Value) * (size_t)newSize);
		delete[]list->mListData;

		list->mListData = newList;
		list->mCapacity = newSize;
	}
}

// 调整列表大小
void	XScriptVM::ListResize(ListValue* list, XInt size)
{
	if (size < 0)
	{
		ExecError("list.resize: invalid size %d, size must be >= 0", size);
		return;
	}

	if (size > list->mCapacity)
	{
		ListReallocate(list, size);
		for (XInt i = list->mListSize; i < size; i++)
		{
			XSetNilValue(&list->mListData[i]);
		}
	}
	else if (size < list->mListSize)
	{
		ListShrink(list, size);
	}
	else
	{
		for (XInt i = list->mListSize; i < size; i++)
		{
			XSetNilValue(&list->mListData[i]);
		}
	}

	list->mListSize = size;
}


// 重新分配列表内存（扩容）
void	XScriptVM::ListReallocate(ListValue* list, XInt size)
{
	XInt oldSize = list->mCapacity;
	XInt newSize = size > 0 ? size : oldSize * 2;

	Value* newList = new Value[(size_t)newSize];
	memcpy(newList, list->mListData, sizeof(Value) * (size_t)list->mListSize);
	delete []list->mListData;
	
	list->mListData = newList;
	list->mCapacity = newSize;
}

// 向列表末尾追加元素
void	XScriptVM::ListAppend(ListValue* list, const Value& value)
{
	if (list->mListSize >= list->mCapacity - 1)
		ListReallocate(list);

	list->mListData[list->mListSize++] = value;
}

// 在列表指定位置插入元素
void	XScriptVM::ListInsert(ListValue* list, XInt pos, const Value& value)
{
	if (pos < 0 || pos >= list->mListSize)
	{
		ExecError("list.insert: index %d out of range [0, %d)", pos, list->mListSize);
		return;
	}

	if (list->mListSize >= list->mCapacity - 1)
		ListReallocate(list);

	list->mListSize++;
	for (XInt i = list->mListSize; i > pos; i--)
		list->mListData[i] = list->mListData[i - 1];
	list->mListData[pos] = value;

}

// 按值删除列表中的元素
void	XScriptVM::ListRemove(ListValue* list, const Value& value)
{
	for (int i = 0; i < list->mListSize; i++)
	{
		if (isValueEqual(list->mListData[i], value))
		{
			ListRemoveAtPos(list, i);
			break;
		}
	}
}

// 按位置删除列表中的元素
void	XScriptVM::ListRemoveAtPos(ListValue* list, XInt pos, XInt num)
{
	if (pos < 0 || pos >= list->mListSize)
	{
		ExecError("list.remove: index %d out of range [0, %d)", pos, list->mListSize);
		return;
	}

	if (num + pos > list->mListSize)
	{
		num = list->mListSize - pos;
	}

	for (XInt i = pos; i < list->mListSize - num; i++)
	{
		if (i + num < list->mListSize)
			list->mListData[i] = list->mListData[i + num];
	}

	for (XInt i = 0; i < num; i++)
	{
		XSetNilValue(&list->mListData[list->mListSize - i - 1]);
	}

	list->mListSize -= num;

	if (list->mListSize < list->mCapacity - 1)
		ListShrink(list);
}

// 使用自定义比较函数进行值比较
bool	XScriptVM::CompareValueWithCompFunc(Value& firstValue,Value& secondValue, Function* func, bool isReverse)
{
	push(firstValue);
	push(secondValue);
	push(ConstructValue((XInt)(isReverse ? 1 : 0)));

	CallFunction(func, 3);
	return (!XIsValueFalse(mRegValue[0]));
}

// 使用自定义键函数进行值比较
bool	XScriptVM::CompareValueWithKeyFunc(Value& firstValue,Value& secondValue, Function* func, bool isReverse)
{
	push(firstValue);
	CallFunction(func, 1);

	Value firstValueKey = mRegValue[0];

	push(secondValue);
	CallFunction(func, 1);

	Value secondValueKey = mRegValue[0];
	return CompareValue(firstValueKey, secondValueKey, isReverse);
}

// 默认值比较（支持数字、字符串、元方法）
bool	XScriptVM::CompareValue(Value&  firstValue,Value& secondValue, bool isReverse)
{
	if (isReverse)
	{
		XInt iResult = 0;	
		if (IsPNumberType(firstValue) && IsPNumberType(secondValue))
		{
			return PNumberValue(firstValue) >  PNumberValue(secondValue);
		}
		else if (IsValueString(&firstValue) && IsValueString(&secondValue))
		{
			return strcmp(stringRawValue(&firstValue), stringRawValue(&secondValue)) > 0;
		}

		else if (firstValue.type == secondValue.type)	
		{	
			Value resultValue;	
			if (CalByTagMethod(&resultValue, &firstValue, &secondValue, MMT_Great))	
			{	
				return (!XIsValueFalse(resultValue));
			}
			else
				ExecError("attempt to compare incompatible types: %s and %s",  getTypeName(firstValue.type), getTypeName(secondValue.type));
		}

				ExecError("attempt to compare incompatible types: %s and %s",  getTypeName(firstValue.type), getTypeName(secondValue.type));

		return false;
	}
	else
	{
		XInt iResult = 0;	
		if (IsPNumberType(firstValue) && IsPNumberType(secondValue))
		{
			return PNumberValue(firstValue) <  PNumberValue(secondValue);
		}
		else if (IsValueString(&firstValue) && IsValueString(&secondValue))
		{
			return strcmp(stringRawValue(&firstValue), stringRawValue(&secondValue)) < 0;
		}

		else if (firstValue.type == secondValue.type)	
		{	
			Value resultValue;	
			if (CalByTagMethod(&resultValue, &firstValue, &secondValue, NMT_Less))	
			{	
				if (resultValue.type == OP_TYPE_NIL || (IsPNumberType(resultValue) && PNumberValue(resultValue) == 0))	
					return false;
				else
					return true;
			}
			else
				ExecError("attempt to compare incompatible types: %s and %s",  getTypeName(firstValue.type), getTypeName(secondValue.type));
		}

				ExecError("attempt to compare incompatible types: %s and %s",  getTypeName(firstValue.type), getTypeName(secondValue.type));

		return false;
	}
		
}

// 统计元素在列表中出现的次数
XInt	XScriptVM::ListCount(ListValue* list, const Value& value)
{
	XInt count = 0;
	for (XInt i = 0; i < list->mListSize; i++)
	{
		if (isValueEqual(value, list->mListData[i]))
		{
			count++;
		}
	}

	return count;
}

// 反转列表
void	XScriptVM::ListReverse(ListValue* list)
{
	for (XInt i = 0; i < list->mListSize / 2; i++)
	{
		Value tmp = list->mListData[i];
		list->mListData[i] = list->mListData[list->mListSize - 1 - i];
		list->mListData[list->mListSize - 1 - i] = tmp;
	}
}

// 插入排序（使用自定义键函数）
void	XScriptVM::InsertSortWithKeyFunc(ListValue* list, bool isReverse, Function* keyFunc)
{
	for(XInt i = 1; i < list->mListSize; i++)
	{  
		if(CompareValueWithKeyFunc(list->mListData[i], list->mListData[i - 1], keyFunc, isReverse))
		{               
			XInt j = i-1;   
			Value x = list->mListData[i];
			list->mListData[i] = list->mListData[i-1];
			while(j >= 0 && CompareValueWithKeyFunc(x, list->mListData[j], keyFunc, isReverse)){
				list->mListData[j+1] = list->mListData[j];  
				j--;
			}  
			list->mListData[j+1] = x;
		}  
	}  

}

// 插入排序（使用自定义比较函数）
void	XScriptVM::InsertSortWithCompFunc(ListValue* list, bool isReverse, Function* sortFunc)
{
	for(XInt i = 1; i < list->mListSize; i++)
	{  
		if(CompareValueWithCompFunc(list->mListData[i], list->mListData[i - 1], sortFunc, isReverse))
		{               
			XInt j = i-1;   
			Value x = list->mListData[i];
			list->mListData[i] = list->mListData[i-1];
			while(j >= 0 && CompareValueWithCompFunc(x, list->mListData[j], sortFunc, isReverse)){
				list->mListData[j+1] = list->mListData[j];  
				j--;
			}  
			list->mListData[j+1] = x;
		}  
	}  

}

// 插入排序（默认比较）
void	XScriptVM::InsertSort(ListValue* list, bool isReverse)
{
	for(XInt i = 1; i < list->mListSize; i++)
	{  
		if(CompareValue(list->mListData[i], list->mListData[i - 1], isReverse))
		{               
			XInt j = i-1;   
			Value x = list->mListData[i];
			list->mListData[i] = list->mListData[i-1];
			while(j >= 0 && CompareValue(x, list->mListData[j], isReverse)){
				list->mListData[j+1] = list->mListData[j];  
				j--;
			}  
			list->mListData[j+1] = x;
		}  
	}  

}
