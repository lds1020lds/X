
function TestGet()
{
	s = socket:create(socket.AF_INET, socket.SOCK_STREAM, 0)
printf(s)
r = s:connect(gethostbyname("www.g.cn"), 80);
printf(r)
r = s:send("GET / HTTP/1.1\r\n\r\n");
printf(r)
ret, buf = s:recv(1024);
printf(buf)
s:close();
os.system("pause")
}

function TestClient()
{
	s = socket:create(socket.AF_INET, socket.SOCK_STREAM, 0)
	r = s:connect("127.0.0.1", 30001);
	printf("connect ", r, socket.getlasterror());
	if (r == 0)
	{
		printf("pls enter msg")
		while (1)
	{
		io.output("> ");
		var cmd = io.input();
		if (cmd == "exit")
		{
			break;
		}
	
		r = s:send(cmd);
		if (r < 0)
		{
			printf("send error ", socket.getlasterror(), " exit");
			break;
		}
		ret, buf = s:recv(1024);
		if (ret <= 0)
		{
			printf("recv error ", socket.getlasterror(), " exit");
			break;
		}
		else
		{
			printf("recv:", buf);
		}
		
	}
	}
	
	
	s:close();
	os.system("pause")
}


TestClient();

