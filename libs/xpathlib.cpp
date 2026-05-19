#include "XScriptVM.h"
#include "xpathlib.h"
#include <string>
#include <vector>
#include <cstdlib>
#include <direct.h>
#include <cctype>
#include <cstring>

static std::string path_slash(const std::string& s)
{
	std::string r = s;
	for (size_t i = 0; i < r.size(); i++)
	{
		if (r[i] == '\\')
			r[i] = '/';
	}
	return r;
}

static bool path_is_abs_str(const std::string& p)
{
	return p.size() >= 1 && (p[0] == '/' || p[0] == '\\')
		|| (p.size() >= 3 && isalpha((unsigned char)p[0]) && p[1] == ':' && (p[2] == '/' || p[2] == '\\'));
}

static std::string path_normalize_str(const std::string& input)
{
	std::string p = path_slash(input);
	std::string prefix;
	size_t start = 0;
	bool absolute = false;
	if (p.size() >= 2 && isalpha((unsigned char)p[0]) && p[1] == ':')
	{
		prefix = p.substr(0, 2);
		start = 2;
		if (start < p.size() && p[start] == '/')
		{
			absolute = true;
			start++;
		}
	}
	else if (!p.empty() && p[0] == '/')
	{
		absolute = true;
		start = 1;
	}

	std::vector<std::string> parts;
	std::string cur;
	for (size_t i = start; i <= p.size(); i++)
	{
		char c = (i < p.size()) ? p[i] : '/';
		if (c == '/')
		{
			if (cur.empty() || cur == ".")
			{
			}
			else if (cur == "..")
			{
				if (!parts.empty() && parts.back() != "..")
					parts.pop_back();
				else if (!absolute)
					parts.push_back(cur);
			}
			else
			{
				parts.push_back(cur);
			}
			cur.clear();
		}
		else
		{
			cur += c;
		}
	}

	std::string out = prefix;
	if (absolute)
		out += "/";
	for (size_t i = 0; i < parts.size(); i++)
	{
		if (i > 0 || (!out.empty() && out[out.size() - 1] != '/'))
			out += "/";
		out += parts[i];
	}
	if (out.empty())
		out = absolute ? "/" : ".";
	return out;
}

static std::vector<std::string> path_split(const std::string& p)
{
	std::string n = path_normalize_str(p);
	std::vector<std::string> parts;
	std::string cur;
	for (size_t i = 0; i <= n.size(); i++)
	{
		char c = (i < n.size()) ? n[i] : '/';
		if (c == '/')
		{
			if (!cur.empty() && cur != ".")
				parts.push_back(cur);
			cur.clear();
		}
		else if (c != ':')
		{
			cur += c;
		}
	}
	return parts;
}

void init_path_lib()
{
	std::vector<HostFunction> funcs;
	funcs.push_back(HostFunction("join", xpath_join));
	funcs.push_back(HostFunction("normalize", xpath_normalize));
	funcs.push_back(HostFunction("basename", xpath_basename));
	funcs.push_back(HostFunction("dirname", xpath_dirname));
	funcs.push_back(HostFunction("extname", xpath_extname));
	funcs.push_back(HostFunction("isabs", xpath_isabs));
	funcs.push_back(HostFunction("abspath", xpath_abspath));
	funcs.push_back(HostFunction("relative", xpath_relative));
	gScriptVM.RegisterHostLib("path", funcs);
}

void xpath_join(XScriptVM* vm)
{
	std::string out;
	for (int i = 0; i < vm->getNumParam(); i++)
	{
		Value v = vm->getParamValue(i);
		if (v.type != OP_TYPE_STRING)
			continue;
		std::string part = stringRawValue(&v);
		if (part.empty())
			continue;
		if (out.empty() || path_is_abs_str(part))
			out = part;
		else
		{
			if (out[out.size() - 1] != '/' && out[out.size() - 1] != '\\')
				out += "/";
			out += part;
		}
	}
	vm->setReturnAsStr(path_normalize_str(out).c_str());
}

void xpath_normalize(XScriptVM* vm)
{
	CheckParam(path.normalize, 0, p, OP_TYPE_STRING);
	vm->setReturnAsStr(path_normalize_str(stringRawValue(&p)).c_str());
}

void xpath_basename(XScriptVM* vm)
{
	CheckParam(path.basename, 0, p, OP_TYPE_STRING);
	std::string n = path_normalize_str(stringRawValue(&p));
	if (n.size() > 1 && n[n.size() - 1] == '/')
		n.erase(n.size() - 1);
	size_t pos = n.find_last_of('/');
	vm->setReturnAsStr((pos == std::string::npos ? n : n.substr(pos + 1)).c_str());
}

void xpath_dirname(XScriptVM* vm)
{
	CheckParam(path.dirname, 0, p, OP_TYPE_STRING);
	std::string n = path_normalize_str(stringRawValue(&p));
	if (n.size() > 1 && n[n.size() - 1] == '/')
		n.erase(n.size() - 1);
	size_t pos = n.find_last_of('/');
	if (pos == std::string::npos)
		vm->setReturnAsStr(".");
	else if (pos == 0)
		vm->setReturnAsStr("/");
	else
		vm->setReturnAsStr(n.substr(0, pos).c_str());
}

void xpath_extname(XScriptVM* vm)
{
	CheckParam(path.extname, 0, p, OP_TYPE_STRING);
	std::string n = path_slash(stringRawValue(&p));
	size_t slash = n.find_last_of('/');
	size_t dot = n.find_last_of('.');
	if (dot == std::string::npos || (slash != std::string::npos && dot < slash) || dot == slash + 1)
		vm->setReturnAsStr("");
	else
		vm->setReturnAsStr(n.substr(dot).c_str());
}

void xpath_isabs(XScriptVM* vm)
{
	CheckParam(path.isabs, 0, p, OP_TYPE_STRING);
	vm->setReturnAsInt(path_is_abs_str(stringRawValue(&p)) ? 1 : 0);
}

void xpath_abspath(XScriptVM* vm)
{
	CheckParam(path.abspath, 0, p, OP_TYPE_STRING);
	char buf[_MAX_PATH];
	if (_fullpath(buf, stringRawValue(&p), _MAX_PATH) != NULL)
		vm->setReturnAsStr(path_slash(buf).c_str());
	else
		vm->setReturnAsNil(0);
}

void xpath_relative(XScriptVM* vm)
{
	CheckParam(path.relative, 0, from, OP_TYPE_STRING);
	CheckParam(path.relative, 1, to, OP_TYPE_STRING);
	std::string f = path_normalize_str(stringRawValue(&from));
	std::string t = path_normalize_str(stringRawValue(&to));
	std::vector<std::string> fp = path_split(f);
	std::vector<std::string> tp = path_split(t);
	size_t i = 0;
	while (i < fp.size() && i < tp.size() && _stricmp(fp[i].c_str(), tp[i].c_str()) == 0)
		i++;
	std::string out;
	for (size_t j = i; j < fp.size(); j++)
	{
		if (!out.empty()) out += "/";
		out += "..";
	}
	for (size_t j = i; j < tp.size(); j++)
	{
		if (!out.empty()) out += "/";
		out += tp[j];
	}
	if (out.empty()) out = ".";
	vm->setReturnAsStr(out.c_str());
}
