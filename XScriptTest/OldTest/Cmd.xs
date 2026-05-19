prints("xscript 5.1.4")
while (1)
{
	io.output("> ");
	var cmd = io.input();
	if (cmd == "exit")
	{
		break;
	}
	var ret, func = loadstring(cmd);
	if (ret > 0)
	{
		var iRet, errorDesc = pcall(func);
		if (iRet != 0)
		{
			prints(errorDesc);
		}
	}
}
	