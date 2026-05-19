#include "XScriptVM.h"
#include "xjsonlib.h"
#include "xbaselib.h"
#include <string>
#include <set>
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <cstring>

static void json_skip_ws(const std::string& s, size_t& pos)
{
	while (pos < s.size() && isspace((unsigned char)s[pos]))
		pos++;
}

static void json_append_escaped(std::string& out, const char* str)
{
	out += '"';
	for (const unsigned char* p = (const unsigned char*)str; *p; p++)
	{
		switch (*p)
		{
		case '"': out += "\\\""; break;
		case '\\': out += "\\\\"; break;
		case '\b': out += "\\b"; break;
		case '\f': out += "\\f"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default:
			if (*p < 0x20)
			{
				char buf[8];
				snprintf(buf, sizeof(buf), "\\u%04x", *p);
				out += buf;
			}
			else
			{
				out += (char)*p;
			}
			break;
		}
	}
	out += '"';
}

static void json_encode_value(XScriptVM* vm, const Value& value, std::string& out, std::set<const void*>& seen)
{
	switch (value.type)
	{
	case OP_TYPE_NIL:
		out += "null";
		break;
	case OP_TYPE_INT:
		out += std::to_string((long long)value.iIntValue);
		break;
	case OP_TYPE_FLOAT:
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "%.15g", (double)value.fFloatValue);
		out += buf;
		break;
	}
	case OP_TYPE_STRING:
		json_append_escaped(out, stringRawValue(&value));
		break;
	case OP_TYPE_LIST:
	{
		if (seen.find(value.listData) != seen.end())
		{
			vm->ExecError("json.encode: circular list reference");
			return;
		}
		seen.insert(value.listData);
		out += '[';
		for (XInt i = 0; i < value.listData->mListSize; i++)
		{
			if (i > 0)
				out += ',';
			json_encode_value(vm, value.listData->mListData[i], out, seen);
		}
		out += ']';
		seen.erase(value.listData);
		break;
	}
	case OP_TYPE_TABLE:
	{
		if (seen.find(value.tableData) != seen.end())
		{
			vm->ExecError("json.encode: circular table reference");
			return;
		}
		seen.insert(value.tableData);
		out += '{';
		bool first = true;
		for (XInt i = 0; i < value.tableData->mArraySize; i++)
		{
			if (IsValueNil(&value.tableData->mArrayData[i]))
				continue;
			if (!first) out += ',';
			first = false;
			json_append_escaped(out, std::to_string((long long)i).c_str());
			out += ':';
			json_encode_value(vm, value.tableData->mArrayData[i], out, seen);
		}
		for (int i = 0; i < value.tableData->mNodeCapacity; i++)
		{
			TableNode& node = value.tableData->mNodeData[i];
			if (IsValueNil(&node.value))
				continue;
			if (!first) out += ',';
			first = false;
			if (node.key.keyVal.type == OP_TYPE_STRING)
				json_append_escaped(out, stringRawValue(&node.key.keyVal));
			else if (node.key.keyVal.type == OP_TYPE_INT)
				json_append_escaped(out, std::to_string((long long)node.key.keyVal.iIntValue).c_str());
			else
				json_append_escaped(out, getValueDescString(vm, node.key.keyVal).c_str());
			out += ':';
			json_encode_value(vm, node.value, out, seen);
		}
		out += '}';
		seen.erase(value.tableData);
		break;
	}
	default:
		vm->ExecError("json.encode: unsupported value type %s", getTypeName(value.type));
		break;
	}
}

class JsonParser
{
public:
	JsonParser(XScriptVM* vm, const std::string& text) : mVM(vm), mText(text), mPos(0) {}

	Value parse()
	{
		json_skip_ws(mText, mPos);
		Value ret = parseValue();
		json_skip_ws(mText, mPos);
		if (mPos != mText.size())
			mVM->ExecError("json.decode: trailing characters at %d", (int)mPos);
		return ret;
	}

private:
	Value parseValue()
	{
		json_skip_ws(mText, mPos);
		if (mPos >= mText.size())
			mVM->ExecError("json.decode: unexpected end");

		char c = mText[mPos];
		if (c == '"') return mVM->ConstructValue(parseString().c_str());
		if (c == '{') return mVM->ConstructValue(parseObject());
		if (c == '[') return mVM->ConstructValue(parseArray());
		if (c == 'n') { expectLiteral("null"); Value v; XSetNilValue(&v); return v; }
		if (c == 't') { expectLiteral("true"); return mVM->ConstructValue((XInt)1); }
		if (c == 'f') { expectLiteral("false"); return mVM->ConstructValue((XInt)0); }
		if (c == '-' || isdigit((unsigned char)c)) return parseNumber();

		mVM->ExecError("json.decode: unexpected character '%c' at %d", c, (int)mPos);
		Value v; XSetNilValue(&v); return v;
	}

	void expectLiteral(const char* literal)
	{
		size_t len = strlen(literal);
		if (mText.compare(mPos, len, literal) != 0)
			mVM->ExecError("json.decode: expected %s at %d", literal, (int)mPos);
		mPos += len;
	}

	std::string parseString()
	{
		std::string out;
		mPos++;
		while (mPos < mText.size())
		{
			char c = mText[mPos++];
			if (c == '"')
				return out;
			if (c == '\\')
			{
				if (mPos >= mText.size())
					mVM->ExecError("json.decode: invalid escape");
				char e = mText[mPos++];
				switch (e)
				{
				case '"': out += '"'; break;
				case '\\': out += '\\'; break;
				case '/': out += '/'; break;
				case 'b': out += '\b'; break;
				case 'f': out += '\f'; break;
				case 'n': out += '\n'; break;
				case 'r': out += '\r'; break;
				case 't': out += '\t'; break;
				case 'u':
					if (mPos + 4 > mText.size())
						mVM->ExecError("json.decode: invalid unicode escape");
					out += '?';
					mPos += 4;
					break;
				default:
					mVM->ExecError("json.decode: invalid escape \\%c", e);
				}
			}
			else
			{
				out += c;
			}
		}
		mVM->ExecError("json.decode: unterminated string");
		return out;
	}

	Value parseNumber()
	{
		size_t start = mPos;
		if (mText[mPos] == '-') mPos++;
		while (mPos < mText.size() && isdigit((unsigned char)mText[mPos])) mPos++;
		bool isFloat = false;
		if (mPos < mText.size() && mText[mPos] == '.')
		{
			isFloat = true;
			mPos++;
			while (mPos < mText.size() && isdigit((unsigned char)mText[mPos])) mPos++;
		}
		if (mPos < mText.size() && (mText[mPos] == 'e' || mText[mPos] == 'E'))
		{
			isFloat = true;
			mPos++;
			if (mPos < mText.size() && (mText[mPos] == '+' || mText[mPos] == '-')) mPos++;
			while (mPos < mText.size() && isdigit((unsigned char)mText[mPos])) mPos++;
		}
		std::string num = mText.substr(start, mPos - start);
		if (isFloat)
			return mVM->ConstructValue((XFloat)atof(num.c_str()));
		return mVM->ConstructValue((XInt)_strtoi64(num.c_str(), NULL, 10));
	}

	ListValue* parseArray()
	{
		ListValue* list = mVM->CreateList();
		mPos++;
		json_skip_ws(mText, mPos);
		if (mPos < mText.size() && mText[mPos] == ']')
		{
			mPos++;
			return list;
		}
		while (true)
		{
			Value v = parseValue();
			mVM->ListAppend(list, v);
			json_skip_ws(mText, mPos);
			if (mPos >= mText.size())
				mVM->ExecError("json.decode: unterminated array");
			if (mText[mPos] == ']')
			{
				mPos++;
				return list;
			}
			if (mText[mPos] != ',')
				mVM->ExecError("json.decode: expected ',' in array");
			mPos++;
		}
	}

	TableValue* parseObject()
	{
		TableValue* table = mVM->newTable();
		mPos++;
		json_skip_ws(mText, mPos);
		if (mPos < mText.size() && mText[mPos] == '}')
		{
			mPos++;
			return table;
		}
		while (true)
		{
			json_skip_ws(mText, mPos);
			if (mPos >= mText.size() || mText[mPos] != '"')
				mVM->ExecError("json.decode: expected object key");
			std::string key = parseString();
			json_skip_ws(mText, mPos);
			if (mPos >= mText.size() || mText[mPos] != ':')
				mVM->ExecError("json.decode: expected ':' after object key");
			mPos++;
			Value val = parseValue();
			mVM->setTableValue(table, mVM->ConstructValue(key.c_str()), val);
			json_skip_ws(mText, mPos);
			if (mPos >= mText.size())
				mVM->ExecError("json.decode: unterminated object");
			if (mText[mPos] == '}')
			{
				mPos++;
				return table;
			}
			if (mText[mPos] != ',')
				mVM->ExecError("json.decode: expected ',' in object");
			mPos++;
		}
	}

	XScriptVM* mVM;
	const std::string& mText;
	size_t mPos;
};

void init_json_lib()
{
	std::vector<HostFunction> funcs;
	funcs.push_back(HostFunction("encode", xjson_encode));
	funcs.push_back(HostFunction("decode", xjson_decode));
	gScriptVM.RegisterHostLib("json", funcs);
}

void xjson_encode(XScriptVM* vm)
{
	Value value = vm->getParamValue(0);
	std::string out;
	std::set<const void*> seen;
	json_encode_value(vm, value, out, seen);
	vm->setReturnAsStr(out.c_str());
}

void xjson_decode(XScriptVM* vm)
{
	CheckParam(json.decode, 0, text, OP_TYPE_STRING);
	std::string raw = stringRawValue(&text);
	JsonParser parser(vm, raw);
	Value ret = parser.parse();
	vm->setReturnAsValue(ret);
}
