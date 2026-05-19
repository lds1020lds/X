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

function TestServer()
{
	s = socket:create(socket.AF_INET, socket.SOCK_STREAM, 0)
	err = s:bind(30001);
	if (err != 0)
	{
		printf("bind error ", socket.getlasterror(), "exit");
		return;
	}
	
	err = s:listen(3);
	if (err != 0)
	{
		printf("listen error ", socket.getlasterror(), "exit");
		return;
	}
	printf("wait for connect...")
	s2, host, arr = s:accept();
	if (s2 == nil)
	{
		printf("accept error ", socket.getlasterror(), "exit");
		return;
	}
	
	printf("accepted ", s2, host, arr);
	printf("wait for msg...");
	if (s != nil)
	{
		while (1)
		{
			ret, buf = s2:recv(1024);
			if (ret <= 0)
			{
				printf("recv error ", socket.getlasterror(), " exit");
				break;
			}
			printf(buf);
			
			io.output("> ");
			var cmd = io.input();
			if (cmd == "exit")
			{
				break;
			}
			r = s2:send(cmd);
			if (ret < 0)
			{
				printf("send error ", socket.getlasterror(), " exit");
				break;
			}
		}
	}
	s2:close();
	s:close();
}

TestServer();

