#include "XScriptVM.h"
#include "xtablelib.h"
#include "xbaselib.h"
#include <set>


#define			Iterator_Table			("__iterator_table__")
#define			Iterator_Key			("__iterator_key__")

static void table_append_key_value(XScriptVM* vm, ListValue* keys, ListValue* values, const Value& key, const Value& value)
{
	if (keys != NULL)
		vm->ListAppend(keys, key);
	if (values != NULL)
		vm->ListAppend(values, value);
}

static XInt table_count(TableValue* table)
{
	XInt count = 0;
	for (XInt i = 0; i < table->mArraySize; i++)
	{
		if (!IsValueNil(&table->mArrayData[i]))
			count++;
	}
	for (int i = 0; i < table->mNodeCapacity; i++)
	{
		if (!IsValueNil(&table->mNodeData[i].value))
			count++;
	}
	return count;
}

static Value table_clone_value(XScriptVM* vm, const Value& value, bool deep, std::set<const void*>& seen);

static TableValue* table_clone_table(XScriptVM* vm, TableValue* src, bool deep, std::set<const void*>& seen)
{
	if (seen.find(src) != seen.end())
	{
		vm->ExecError("table.clone: circular table reference");
		return NULL;
	}
	seen.insert(src);
	TableValue* dst = vm->newTable(src->mArraySize);
	for (XInt i = 0; i < src->mArraySize; i++)
	{
		if (!IsValueNil(&src->mArrayData[i]))
			dst->mArrayData[i] = table_clone_value(vm, src->mArrayData[i], deep, seen);
	}
	for (int i = 0; i < src->mNodeCapacity; i++)
	{
		TableNode& node = src->mNodeData[i];
		if (IsValueNil(&node.value))
			continue;
		Value key = table_clone_value(vm, node.key.keyVal, deep, seen);
		Value val = table_clone_value(vm, node.value, deep, seen);
		vm->setTableValue(dst, key, val);
	}
	seen.erase(src);
	return dst;
}

static ListValue* table_clone_list(XScriptVM* vm, ListValue* src, bool deep, std::set<const void*>& seen)
{
	if (seen.find(src) != seen.end())
	{
		vm->ExecError("table.clone: circular list reference");
		return NULL;
	}
	seen.insert(src);
	ListValue* dst = vm->CreateList();
	for (XInt i = 0; i < src->mListSize; i++)
		vm->ListAppend(dst, table_clone_value(vm, src->mListData[i], deep, seen));
	seen.erase(src);
	return dst;
}

static Value table_clone_value(XScriptVM* vm, const Value& value, bool deep, std::set<const void*>& seen)
{
	if (!deep)
		return value;
	if (value.type == OP_TYPE_TABLE)
		return vm->ConstructValue(table_clone_table(vm, value.tableData, deep, seen));
	if (value.type == OP_TYPE_LIST)
		return vm->ConstructValue(table_clone_list(vm, value.listData, deep, seen));
	return value;
}

void xtable_next(XScriptVM* vm)
{
	CheckParam(table.next, 0, iterTable, OP_TYPE_TABLE);

	Value tableValue;
	Value keyValue;
	if (!vm->getTableValue(iterTable.tableData, vm->ConstructValue(Iterator_Table), tableValue) || !IsValueTable(&tableValue))
	{
		vm->setReturnAsInt(0);
		return;
	}

	if (!vm->getTableValue(iterTable.tableData, vm->ConstructValue(Iterator_Key), keyValue))
	{
		XSetNilValue(&keyValue);
	}

	Value nextKey;
	Value nextValue;
	if (vm->GetNextKey(tableValue.tableData, keyValue, nextKey, nextValue))
	{
		vm->setTableValue(iterTable.tableData, vm->ConstructValue(Iterator_Key), nextKey);
		vm->setReturnAsInt(1, 0);
		vm->setReturnAsValue(nextKey, 1);
		vm->setReturnAsValue(nextValue, 2);
	}
	else
	{
		vm->setReturnAsInt(0);
	}
}

void xtable_Iterator(XScriptVM* vm)
{
	CheckParam(table.Iterator, 0, tableValue, OP_TYPE_TABLE);

	TableValue* iterTable = vm->CreateTable();

	Value nextFunction = vm->CreateCHostFunction(xtable_next, 0);
	vm->setTableValue(iterTable, vm->ConstructValue(ITER_NEXT), nextFunction);

	Value nilValue;
	XSetNilValue(&nilValue);
	vm->setTableValue(iterTable, vm->ConstructValue(Iterator_Key), nilValue);
	vm->setTableValue(iterTable, vm->ConstructValue(Iterator_Table), tableValue);

	vm->setReturnAsTable(iterTable);
}

void xtable_keys(XScriptVM* vm)
{
	CheckParam(table.keys, 0, tableValue, OP_TYPE_TABLE);
	ListValue* list = vm->CreateList();
	for (XInt i = 0; i < tableValue.tableData->mArraySize; i++)
	{
		if (!IsValueNil(&tableValue.tableData->mArrayData[i]))
			vm->ListAppend(list, vm->ConstructValue(i));
	}
	for (int i = 0; i < tableValue.tableData->mNodeCapacity; i++)
	{
		TableNode& node = tableValue.tableData->mNodeData[i];
		if (!IsValueNil(&node.value))
			vm->ListAppend(list, node.key.keyVal);
	}
	vm->setReturnAsValue(vm->ConstructValue(list));
}

void xtable_values(XScriptVM* vm)
{
	CheckParam(table.values, 0, tableValue, OP_TYPE_TABLE);
	ListValue* list = vm->CreateList();
	for (XInt i = 0; i < tableValue.tableData->mArraySize; i++)
	{
		if (!IsValueNil(&tableValue.tableData->mArrayData[i]))
			vm->ListAppend(list, tableValue.tableData->mArrayData[i]);
	}
	for (int i = 0; i < tableValue.tableData->mNodeCapacity; i++)
	{
		TableNode& node = tableValue.tableData->mNodeData[i];
		if (!IsValueNil(&node.value))
			vm->ListAppend(list, node.value);
	}
	vm->setReturnAsValue(vm->ConstructValue(list));
}

void xtable_size(XScriptVM* vm)
{
	CheckParam(table.size, 0, tableValue, OP_TYPE_TABLE);
	vm->setReturnAsInt(table_count(tableValue.tableData));
}

void xtable_contains(XScriptVM* vm)
{
	CheckParam(table.contains, 0, tableValue, OP_TYPE_TABLE);
	Value key = vm->getParamValue(1);
	Value val;
	vm->setReturnAsInt(vm->getTableValue(tableValue.tableData, key, val) && !IsValueNil(&val) ? 1 : 0);
}

void xtable_clear(XScriptVM* vm)
{
	CheckParam(table.clear, 0, tableValue, OP_TYPE_TABLE);
	for (XInt i = 0; i < tableValue.tableData->mArraySize; i++)
	{
		XSetNilValue(&tableValue.tableData->mArrayData[i]);
	}
	for (int i = 0; i < tableValue.tableData->mNodeCapacity; i++)
	{
		XSetNilValue(&tableValue.tableData->mNodeData[i].key.keyVal);
		XSetNilValue(&tableValue.tableData->mNodeData[i].value);
		tableValue.tableData->mNodeData[i].key.next = NULL;
	}
	tableValue.tableData->lastFreePos = tableValue.tableData->mNodeCapacity - 1;
	vm->setReturnAsValue(tableValue);
}

void xtable_clone(XScriptVM* vm)
{
	Value value = vm->getParamValue(0);
	bool deep = false;
	if (vm->getNumParam() > 1)
	{
		Value deepValue = vm->getParamValue(1);
		deep = !XIsValueFalse(deepValue);
	}
	std::set<const void*> seen;
	Value ret = table_clone_value(vm, value, deep, seen);
	vm->setReturnAsValue(ret);
}

void xtable_merge(XScriptVM* vm)
{
	CheckParam(table.merge, 0, dst, OP_TYPE_TABLE);
	CheckParam(table.merge, 1, src, OP_TYPE_TABLE);
	for (XInt i = 0; i < src.tableData->mArraySize; i++)
	{
		if (!IsValueNil(&src.tableData->mArrayData[i]))
			vm->setTableValue(dst.tableData, vm->ConstructValue(i), src.tableData->mArrayData[i]);
	}
	for (int i = 0; i < src.tableData->mNodeCapacity; i++)
	{
		TableNode& node = src.tableData->mNodeData[i];
		if (!IsValueNil(&node.value))
			vm->setTableValue(dst.tableData, node.key.keyVal, node.value);
	}
	vm->setReturnAsValue(dst);
}

void xtable_tostring(XScriptVM* vm)
{
	CheckParam(table.tostring, 0, srcTable, OP_TYPE_TABLE);
	std::string desc = getValueDescString(vm, srcTable);
	vm->setReturnAsStr(desc.c_str());
}
