#pragma once
#ifndef xlistlib_h__
#define xlistlib_h__
class XScriptVM;

void xlist_Append(XScriptVM* vm);
void xlist_Insert(XScriptVM* vm);
void xlist_Remove(XScriptVM* vm);
void xlist_RemoveAtPos(XScriptVM* vm);
void xlist_Resize(XScriptVM* vm);
void xlist_Size(XScriptVM* vm);
void xlist_Count(XScriptVM* vm);
void xlist_Capacity(XScriptVM* vm);
void xlist_Reverse(XScriptVM* vm);
void xlist_Sort(XScriptVM* vm);
void xlist_SortWithKey(XScriptVM* vm);
void xlist_SortWithCmp(XScriptVM* vm);
void xlist_Iterator(XScriptVM* vm);
void xlist_next(XScriptVM* vm);

//list meta function
void xlist_op_Add(XScriptVM* vm);
void xlist_op_Mul(XScriptVM* vm);
void xlist_op_ToString(XScriptVM* vm);
#endif // xlistlib_h__