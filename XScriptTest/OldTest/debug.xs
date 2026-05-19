breakPoints = array(10);
numTotalBreakPoints = 0;
step = 0;
stepOver = 0;
lastStackIndex = -1;
function addBreakPoint(f, l, condition)
{
	foreach(key, value in ipairs(breakPoints))
	{
		if (value == nil)
		{
			breakPoints[key] = {f = f, l = l, active = 1, condition = condition};
			return key;
		}
	}
	
	return -1;
}

function removeBreakPoint(id)
{
	breakPoints[id] = nil;
}

function distableBreakPoint(id)
{
	if (breakPoints[id] != nil)
	{
		breakPoints[id].active = 0;
	}
}

function enableBreakPoint(id)
{
	if (breakPoints[id] != nil)
	{
		breakPoints[id].active = 1;
	}
}

function removeAllBreakPoints()
{
	foreach(key, value in ipairs(breakPoints))
	{
		if (value != nil)
		{
			breakPoints[key] = nil;
		}
	}
}

function listAllBreakPoints()
{
	foreach(key, value in ipairs(breakPoints))
	{
		if (value != nil)
		{
			printf(key, value);
		}
	}
}

function  getVarValueByName(varName, startStackIndex)
{
	var stackDepth = debug.getstackdepth();
	var i = 0;
	var ret, value;
	for ( i = startStackIndex; i <= stackDepth; i++)
	{
		ret, value = debug.getlocalvarbyname(i, varName);
		if(ret > 0)
		{	
			return value;
		}
	}
	
	ret, value = debug.getglobalvar(varName);
	return value;
}

function PrintHelp()
{
	prints("h		help infomations");
	prints("c		continue execute");
	prints("s		step without step over function");
	prints("n		step with step over function");
	prints("p var		print var value");
	prints("b src:line	add a breakpoint with source file and line number");
	prints("d id		delete breakpoint with the id");	
	prints("bl 		list all of the breakpoints");
	prints("be id		enbale breakpoint with the id");
	prints("bd id		disbale breakpoint with the id");
	prints("bt		print the stack backtrack info");
	prints("? 		execute the input string");
}

function ldb( )
{
	step = 0;
	stepOver = 0;
	while (1)
	{
		io.output("ldb> ");
		var cmd = io.input();
		if (cmd == "c")
		{
			break;
		}
		else if (string.len(cmd) == 0)
		{
			prints(cmd);
			break;
		}
		else if (cmd == "h")
		{
			PrintHelp();
		}
		else if (cmd == "s")
		{
			step = 1;
			break;
		}
		else if (cmd == "n")
		{
			stepOver = 1;
			break;
		}
		else if (cmd == "bl")
		{
			listAllBreakPoints();
		}
		else if (cmd == "bt")
		{	
			prints(debug.stackbacktrace());
		}
		else 
		{
			var pos = string.find(cmd, " ", 0);
			if (pos > 0)
			{
				var prefix = string.sub(cmd, 0, pos);
				var subfix = string.sub(cmd, pos + 1, -1);
				var id;
				if (prefix == "p")
				{
					var vars = subfix:split(" ");
					foreach(i, f in lenum(vars))
					{
						f = f:trim();
						if (f:len() > 0)
						{
							var value = getVarValueByName(f, 3);
							printf(f $ " = " $  value);
						}
					}
					
				}
				else if (prefix == "b")
				{
					var dotpos = string.find(subfix, " ", 0);
					var conditionFunc = nil;
					if (dotpos >= 0)
					{
						var condition = string.sub(subfix, dotpos + 1, -1);
						subfix = string.sub(subfix, 0, dotpos);
						condition = "return " $ condition;
						var cRet, func;
						cRet,  func = loadstring(condition);
						if (cRet > 0)
						{
							conditionFunc = func;
						}
					}
					pos = string.find(subfix, ":", 0);
					var srcFile =  string.sub(subfix, 0, pos);
					var lineStr =  string.sub(subfix, pos + 1, -1);
					lineStr = toNumber(lineStr);
					
					if (lineStr != nil)
					{
						addBreakPoint(srcFile, lineStr, conditionFunc);
					}
				}
				else if (prefix == "d")
				{
					id = toNumber(subfix);
					if (id != nil)
					{
						removeBreakPoint(id);
					}
				}
				else if (prefix == "be")
				{
					id = toNumber(subfix);
					if (id != nil)
					{
						enableBreakPoint(id);
					}
				}
				else if (prefix == "bd")
				{
					id = toNumber(subfix);
					if (id != nil)
					{
						distableBreakPoint(id);
					}
				}
				else
				{
					ret, func = loadstring(cmd);
					if (ret > 0)
					{
						var iRet, errorDesc = pcall(func);
						if (iRet != 0)
						{
							printf(errorDesc);
						}
					}
				}
			}
			else
			{
				ret, func = loadstring(cmd);
				if (ret > 0)
				{
					var iRet, errorDesc = pcall(func);
					if (iRet != 0)
					{
						printf(errorDesc);
					}
				}
			}
		}
		
	}
}

function DebugHook(event, l, file)
{
	if (event == 0)
	{
		var ret, info;
		
		var hasEnterLdb = 0;
		foreach(key, value in ipairs(breakPoints))
		{
			if (value != nil)
			{
				if ((value.f == file) && (value.l == l) && value.active)
				{
					var conditionOK = 1;
					if (value.condition != nil)
					{	
						var localVars;
						ret, localVars = debug.getlocalvars(1);
						setenvtable(localVars);
						conditionOK = value.condition();
						setenvtable(nil);
					}
					
					if (conditionOK)
					{
						prints("Enter breakpoint at file " $ toString(file) $ ", line:" $ toString(l));
						ldb();
						hasEnterLdb = 1;
					}
					
				}
			}
			
		}

		var stackDepth = debug.getstackdepth();
		if (step && hasEnterLdb == 0)
		{
			step = 0;
			prints("Enter breakpoint at file " $ toString(file) $ ", line:" $ toString(l));
			ldb();
			
			hasEnterLdb = 1;
			lastStackIndex = stackDepth;
		}
		else if (stepOver && hasEnterLdb == 0)
		{
			if (stackDepth <= lastStackIndex)
			{
				stepOver = 0;
				if (hasEnterLdb == 0)
				{
					prints("Enter breakpoint at file " $ toString(file) $ ", line:" $ toString(l));
					ldb();
				}
				lastStackIndex = stackDepth;
			}
		}
		else
		{
			lastStackIndex = stackDepth;
		}
	}
}

debug.sethook(nil, nil);
ldb();
debug.sethook("l", DebugHook);

