#pragma once
#ifndef xxmllib_h__
#define xxmllib_h__

class XScriptVM;
void init_xml_lib();

void xmlnode_Clear(XScriptVM* vm);
void xmlnode_Parent(XScriptVM* vm);
void xmlnode_FirstChild(XScriptVM* vm);
void xmlnode_LastChild(XScriptVM* vm);
void xmlnode_IterateChildren(XScriptVM* vm);
void xmlnode_InsertEndChild(XScriptVM* vm);
void xmlnode_LinkEndChild(XScriptVM* vm);
void xmlnode_InsertBeforeChild(XScriptVM* vm);
void xmlnode_InsertAfterChild(XScriptVM* vm);
void xmlnode_ReplaceChild(XScriptVM* vm);
void xmlnode_RemoveChild(XScriptVM* vm);
void xmlnode_PreviousSibling(XScriptVM* vm);
void xmlnode_NextSibling(XScriptVM* vm);
void xmlnode_NextSiblingElement(XScriptVM* vm);
void xmlnode_FirstChildElement(XScriptVM* vm);
void xmlnode_GetDocument(XScriptVM* vm);
void xmlnode_NoChildren(XScriptVM* vm);

void xmlnode_ChildNodeCount(XScriptVM* vm);
void xmlnode_ChildElementCount(XScriptVM* vm);

void xmlnode_ChildNodeList(XScriptVM* vm);
void xmlnode_ChildElementList(XScriptVM* vm);

void xmlnode_CreateText(XScriptVM* vm);

void xmlelement_Attribute(XScriptVM* vm);
void xmlelement_SetAttribute(XScriptVM* vm);
void xmlelement_RemoveAttribute(XScriptVM* vm);
void xmlelement_GetText(XScriptVM* vm);
void xmlelement_CreateElement(XScriptVM* vm);


void xmldocument_OpenFile(XScriptVM* vm);
void xmldocument_SaveFile(XScriptVM* vm);
void xmldocument_RootElement(XScriptVM* vm);
void xmldocument_DestroyDoc(XScriptVM* vm);
void xmldocument_CreateDocument(XScriptVM* vm);


#endif // xxmllib_h__