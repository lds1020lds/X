#include "xlistlib.h"
#include "xbaselib.h"
#include "XScriptVM.h"


void xlist_Append(XScriptVM* vm)
{
	CheckParam(list.append, 0, list, OP_TYPE_LIST);
	for (int i = 1; i < vm->getNumParam(); i++)
		vm->ListAppend(list.listData, vm->getParamValue(i));
}

void xlist_Insert(XScriptVM* vm)
{
	CheckParam(list.insert, 0, list, OP_TYPE_LIST);
	CheckParam(list.insert, 1, pos, OP_TYPE_INT);

	XInt insertPos = pos.iIntValue;
	for (int i = 2; i < vm->getNumParam(); i++)
	{
		vm->ListInsert(list.listData, insertPos, vm->getParamValue(i));
		insertPos++;
	}

}

void xlist_Remove(XScriptVM* vm)
{
	CheckParam(list.remove, 0, list, OP_TYPE_LIST);
	for (int i = 1; i < vm->getNumParam(); i++)
		vm->ListRemove(list.listData, vm->getParamValue(i));
}

void xlist_RemoveAtPos(XScriptVM* vm)
{
	CheckParam(list.removeAt, 0, list, OP_TYPE_LIST);
	CheckParam(list.removeAt, 1, pos, OP_TYPE_INT);
	XInt removeNum = 1;
	if (vm->getNumParam() > 2)
	{
		CheckParam(list.removeAt, 2, num, OP_TYPE_INT);
		removeNum = num.iIntValue;
	}
	vm->ListRemoveAtPos(list.listData, pos.iIntValue, removeNum);
}

void xlist_Resize(XScriptVM* vm)
{
	CheckParam(list.resize, 0, list, OP_TYPE_LIST);
	CheckParam(list.resize, 1, size, OP_TYPE_INT);
	vm->ListResize(list.listData, size.iIntValue);
}

void xlist_Count(XScriptVM* vm)
{
	CheckParam(list.count, 0, list, OP_TYPE_LIST);
	vm->setReturnAsInt(vm->ListCount(list.listData, vm->getParamValue(1)));
}

void xlist_Size(XScriptVM* vm)
{
	CheckParam(list.size, 0, list, OP_TYPE_LIST);
	vm->setReturnAsInt(list.listData->mListSize);
}

void xlist_Capacity(XScriptVM* vm)
{
	CheckParam(list.capacity, 0, list, OP_TYPE_LIST);
	vm->setReturnAsInt(list.listData->mCapacity);
}

void xlist_Sort(XScriptVM* vm)
{
	CheckParam(list.sort, 0, list, OP_TYPE_LIST);
	bool isReverse = false;
	if (vm->getNumParam() > 1)
	{
		CheckParam(list.sort, 1, r, OP_TYPE_INT);
		isReverse = (r.iIntValue != 0);
	}

	vm->InsertSort(list.listData, isReverse);
}

void xlist_SortWithKey(XScriptVM* vm)
{
	CheckParam(list.sortWithKey, 0, list, OP_TYPE_LIST);
	CheckParam(list.sortWithKey, 1, func, OP_TYPE_FUNC);
	bool isReverse = false;
	if (vm->getNumParam() > 2)
	{
		CheckParam(list.sort, 2, r, OP_TYPE_INT);
		isReverse = (r.iIntValue != 0);
	}
	vm->InsertSortWithKeyFunc(list.listData, isReverse, func.func);
}

void xlist_SortWithCmp(XScriptVM* vm)
{
	CheckParam(list.sortWithCmp, 0, list, OP_TYPE_LIST);
	CheckParam(list.sortWithCmp, 1, func, OP_TYPE_FUNC);
	bool isReverse = false;
	if (vm->getNumParam() > 2)
	{
		CheckParam(list.sort, 2, r, OP_TYPE_INT);
		isReverse = (r.iIntValue != 0);
	}
	vm->InsertSortWithCompFunc(list.listData, isReverse, func.func);
}

void xlist_Reverse(XScriptVM* vm)
{
	CheckParam(list.sortWithCmp, 0, list, OP_TYPE_LIST);
	vm->ListReverse(list.listData);
}

void xlist_next(XScriptVM* vm)
{
	CheckParam(list.next, 0, iterTable, OP_TYPE_TABLE);

	XInt index = 0;
	Value listValue;

	if (vm->getTableValue(iterTable.tableData, "index", index) && vm->getTableValue(iterTable.tableData, vm->ConstructValue("list"), listValue))
	{
		if (IsValueList(&listValue))
		{
			index++;
			vm->setTableValue(iterTable.tableData, "index", index);
			if (index < listValue.listData->mListSize)
			{
				vm->setReturnAsInt(1);
				vm->setReturnAsValue(listValue.listData->mListData[index], 1);
			}
			else
			{
				vm->setReturnAsInt(0);
			}
		}
		else
		{
			vm->setReturnAsInt(0);
		}
	}
	else
	{
		vm->setReturnAsInt(0);
	}
}

void xlist_Iterator(XScriptVM* vm)
{
	CheckParam(list.Iterator, 0, list, OP_TYPE_LIST);

	TableValue* iterTable = vm->CreateTable();

	//设置next函数
	Value nextFunction = vm->CreateCHostFunction(xlist_next, 0);
	vm->setTableValue(iterTable, vm->ConstructValue(ITER_NEXT), nextFunction); 

	//初始化索引
	vm->setTableValue(iterTable, "index", (XInt) -1);

	// 设置数据源
	vm->setTableValue(iterTable, vm->ConstructValue("list"), list);

	vm->setReturnAsTable(iterTable);
}

// list的__add元方法：实现list + list（拼接）或 list + element（追加元素）
void xlist_op_Add(XScriptVM* vm)
{
	CheckParam(list.__add, 0, list, OP_TYPE_LIST);

	// 创建新列表作为结果
	ListValue* resultList = vm->CreateList();

	// 先拷贝左操作数（list）的所有元素
	for (XInt i = 0; i < list.listData->mListSize; i++)
	{
		vm->ListAppend(resultList, list.listData->mListData[i]);
	}

	// 处理右操作数
	Value rightValue = vm->getParamValue(1);
	if (IsValueList(&rightValue))
	{
		// list + list：拼接两个列表
		for (XInt i = 0; i < rightValue.listData->mListSize; i++)
		{
			vm->ListAppend(resultList, rightValue.listData->mListData[i]);
		}
	}
	else
	{
		// list + element：追加单个元素
		vm->ListAppend(resultList, rightValue);
	}

	vm->setReturnAsValue(vm->ConstructValue(resultList));
}

// list的__mul元方法：实现list * int（重复列表）
void xlist_op_Mul(XScriptVM* vm)
{
	CheckParam(list.__mul, 0, list, OP_TYPE_LIST);
	CheckParam(list.__mul, 1, count, OP_TYPE_INT);

	XInt repeatCount = count.iIntValue;
	if (repeatCount < 0)
		repeatCount = 0;

	// 创建新列表作为结果
	ListValue* resultList = vm->CreateList();

	// 重复添加元素
	for (XInt i = 0; i < repeatCount; i++)
	{
		for (XInt j = 0; j < list.listData->mListSize; j++)
		{
			vm->ListAppend(resultList, list.listData->mListData[j]);
		}
	}

	vm->setReturnAsValue(vm->ConstructValue(resultList));
}

void xlist_op_ToString(XScriptVM* vm)
{
	CheckParam(list.__tostring, 0, list, OP_TYPE_LIST);
	std::string valueDesc = getValueDescString(vm, list);
	vm->setReturnAsStr(valueDesc.c_str());

}