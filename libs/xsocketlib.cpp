#include "xsocketlib.h"
#include "XScriptVM.h"
#include <string>
#include <thread>

#pragma comment(lib,"ws2_32.lib")
void init_socket_lib()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	std::vector<HostFunction> socketfuncVec;
	socketfuncVec.push_back(HostFunction("create", xsocket_create));
	socketfuncVec.push_back(HostFunction("bind", xsocket_bind));
	socketfuncVec.push_back(HostFunction("connect", xsocket_connect));
	socketfuncVec.push_back(HostFunction("listen", xsocket_listen));
	socketfuncVec.push_back(HostFunction("recv", xsocket_recv));
	socketfuncVec.push_back(HostFunction("send", xsocket_send));
	socketfuncVec.push_back(HostFunction("accept", xsocket_accept));
	socketfuncVec.push_back(HostFunction("recvfrom", xsocket_recvfrom));
	socketfuncVec.push_back(HostFunction("sendto", xsocket_sendto));
	socketfuncVec.push_back(HostFunction("setsockopt", xsocket_setsockopt));
	socketfuncVec.push_back(HostFunction("getsockopt", xsocket_getsockopt));
	socketfuncVec.push_back(HostFunction("ioctlsocket", xsocket_ioctlsocket));
	socketfuncVec.push_back(HostFunction("close", xsocket_closesocket));
	socketfuncVec.push_back(HostFunction("getlasterror", xsocket_getlasterror));

	std::vector<ConstantData> constantVec;
	constantVec.push_back(ConstantData("AF_INET", AF_INET));
	
	constantVec.push_back(ConstantData("FIONREAD", FIONREAD));
	constantVec.push_back(ConstantData("FIONBIO", FIONBIO));
	constantVec.push_back(ConstantData("FIOASYNC", FIOASYNC));
	constantVec.push_back(ConstantData("SIOCSHIWAT", SIOCSHIWAT));
	constantVec.push_back(ConstantData("SIOCGHIWAT", SIOCGHIWAT));
	constantVec.push_back(ConstantData("SIOCSLOWAT", SIOCSLOWAT));
	constantVec.push_back(ConstantData("SIOCGLOWAT", SIOCGLOWAT));
	constantVec.push_back(ConstantData("SIOCATMARK", SIOCATMARK));

	constantVec.push_back(ConstantData("IPPROTO_IP", IPPROTO_IP));
	constantVec.push_back(ConstantData("IPPROTO_ICMP", IPPROTO_ICMP));
	constantVec.push_back(ConstantData("IPPROTO_IGMP", IPPROTO_IGMP));
	constantVec.push_back(ConstantData("IPPROTO_GGP", IPPROTO_GGP));
	constantVec.push_back(ConstantData("IPPROTO_TCP", IPPROTO_TCP));
	constantVec.push_back(ConstantData("IPPROTO_PUP", IPPROTO_PUP));
	constantVec.push_back(ConstantData("IPPROTO_UDP", IPPROTO_UDP));
	constantVec.push_back(ConstantData("IPPROTO_IDP", IPPROTO_IDP));
	constantVec.push_back(ConstantData("IPPROTO_ND", IPPROTO_ND));
	constantVec.push_back(ConstantData("IPPROTO_RAW", IPPROTO_RAW));


	constantVec.push_back(ConstantData("SOCK_STREAM", SOCK_STREAM));
	constantVec.push_back(ConstantData("SOCK_DGRAM", SOCK_DGRAM));
	constantVec.push_back(ConstantData("SOCK_RAW", SOCK_RAW));
	constantVec.push_back(ConstantData("SOCK_RDM", SOCK_RDM));

	constantVec.push_back(ConstantData("SOCK_SEQPACKET", SOCK_SEQPACKET));
	constantVec.push_back(ConstantData("SIOCATMARK", SIOCATMARK));
	constantVec.push_back(ConstantData("SIOCATMARK", SIOCATMARK));
	constantVec.push_back(ConstantData("SIOCATMARK", SIOCATMARK));

	constantVec.push_back(ConstantData("SO_DEBUG", SO_DEBUG));
	constantVec.push_back(ConstantData("SO_ACCEPTCONN", SO_ACCEPTCONN));
	constantVec.push_back(ConstantData("SO_REUSEADDR", SO_REUSEADDR));
	constantVec.push_back(ConstantData("SO_KEEPALIVE", SO_KEEPALIVE));

	constantVec.push_back(ConstantData("SO_DONTROUTE", SO_DONTROUTE));
	constantVec.push_back(ConstantData("SO_BROADCAST", SO_BROADCAST));
	constantVec.push_back(ConstantData("SO_USELOOPBACK", SO_USELOOPBACK));
	constantVec.push_back(ConstantData("SO_LINGER", SO_LINGER));
	constantVec.push_back(ConstantData("SO_OOBINLINE", SO_OOBINLINE));

	constantVec.push_back(ConstantData("SO_DONTLINGER", SO_DONTLINGER));
	constantVec.push_back(ConstantData("SO_SNDBUF", SO_SNDBUF));
	constantVec.push_back(ConstantData("SO_RCVBUF", SO_RCVBUF));
	constantVec.push_back(ConstantData("SO_SNDLOWAT", SO_SNDLOWAT));
	constantVec.push_back(ConstantData("SO_RCVLOWAT", SO_RCVLOWAT));

	constantVec.push_back(ConstantData("SO_ERROR", SO_ERROR));
	constantVec.push_back(ConstantData("SO_TYPE", SO_TYPE));
	constantVec.push_back(ConstantData("SO_CONNDATA", SO_CONNDATA));
	constantVec.push_back(ConstantData("SO_CONNOPT", SO_CONNOPT));
	constantVec.push_back(ConstantData("SO_DISCDATA", SO_DISCDATA));
	constantVec.push_back(ConstantData("SO_DISCOPT", SO_DISCOPT));

	constantVec.push_back(ConstantData("SO_CONNDATALEN", SO_CONNDATALEN));
	constantVec.push_back(ConstantData("SO_CONNOPTLEN", SO_CONNOPTLEN));
	constantVec.push_back(ConstantData("SO_DISCDATALEN", SO_DISCDATALEN));
	constantVec.push_back(ConstantData("SO_DISCOPTLEN", SO_DISCOPTLEN));

	constantVec.push_back(ConstantData("SO_OPENTYPE", SO_OPENTYPE));
	constantVec.push_back(ConstantData("SO_SYNCHRONOUS_ALERT", SO_SYNCHRONOUS_ALERT));
	constantVec.push_back(ConstantData("SO_SYNCHRONOUS_NONALERT", SO_SYNCHRONOUS_NONALERT));
	constantVec.push_back(ConstantData("SO_MAXDG", SO_MAXDG));
	constantVec.push_back(ConstantData("SO_MAXPATHDG", SO_MAXPATHDG));
	constantVec.push_back(ConstantData("SO_UPDATE_ACCEPT_CONTEXT", SO_UPDATE_ACCEPT_CONTEXT));

	constantVec.push_back(ConstantData("TCP_NODELAY", TCP_NODELAY));
	constantVec.push_back(ConstantData("TCP_BSDURGENT", TCP_BSDURGENT));

	gScriptVM.RegisterUserClass("socket", NULL, socketfuncVec, constantVec);

	gScriptVM.RegisterHostApi("gethostbyname", xsocket_gethostbyname);
	gScriptVM.RegisterHostApi("tcp_connect_async", xsocket_connect_async);
	gScriptVM.RegisterHostApi("socket_read_async", xsocket_recv_async);
	gScriptVM.RegisterHostApi("socket_write_async", xsocket_send_async);
}

void xsocket_create(XScriptVM* vm)
{
	CheckParam(socket.create, 1, af, OP_TYPE_INT);
	CheckParam(socket.create, 2, type, OP_TYPE_INT);
	CheckParam(socket.create, 3, protocol, OP_TYPE_INT);

	SOCKET s = socket((int)af.iIntValue, (int)type.iIntValue, (int)protocol.iIntValue);
	if (s == INVALID_SOCKET)
	{
		vm->setReturnAsNil(0);
	}
	else
	{
		vm->setReturnAsUserData("socket", (void*)s);
	}
	
}

void xsocket_bind(XScriptVM* vm)
{
	CheckUserValueParam(socket.bind, 0, s, SOCKET, "socket");
	CheckParam(socket.bind, 1, port, OP_TYPE_INT);
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons((u_short)port.iIntValue);

	int err = bind(s, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
	vm->setReturnAsInt((XInt)err);
}

void xsocket_connect(XScriptVM* vm)
{
	CheckUserValueParam(socket.connect, 0, s, SOCKET, "socket");
	CheckParam(socket.connect, 1, host, OP_TYPE_STRING);
	CheckParam(socket.connect, 2, port, OP_TYPE_INT);

	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = inet_addr(stringRawValue(&host));
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons((u_short)port.iIntValue);

	int err = connect(s, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
	vm->setReturnAsInt((XInt)err);
}

void xsocket_listen(XScriptVM* vm)
{
	CheckUserValueParam(socket.listen, 0, s, SOCKET, "socket");
	CheckParam(socket.listen, 1, backlog, OP_TYPE_INT);

	int err = listen(s, (int)backlog.iIntValue);
	vm->setReturnAsInt((XInt)err);
}

void xsocket_recv(XScriptVM* vm)
{
	CheckUserValueParam(socket.recv, 0, s, SOCKET, "socket");
	CheckParam(socket.recv, 1, len, OP_TYPE_INT);
	int readSize = (int)len.iIntValue;
	char* buff = new char[readSize];
	int realSize = recv(s, buff, readSize, 0);
	vm->setReturnAsInt(realSize);
	if (realSize > 0)
	{
		XString* bufStr = vm->NewXString((const char*)buff, realSize);
		vm->setReturnAsValue(vm->ConstructValue(bufStr), 1);
	}
	delete buff;
}

void xsocket_send(XScriptVM* vm)
{
	CheckUserValueParam(socket.send, 0, s, SOCKET, "socket");
	CheckParam(socket.send, 1, buf, OP_TYPE_STRING);
	int err = send(s, stringRawValue(&buf), stringRawLen(&buf), 0);
	vm->setReturnAsInt((XInt)err);
}

void xsocket_recvfrom(XScriptVM* vm)
{
	CheckUserValueParam(socket.recvfrom, 0, s, SOCKET, "socket");
	CheckParam(socket.recvfrom, 1, len, OP_TYPE_INT);

	CheckParam(socket.recvfrom, 2, host, OP_TYPE_STRING);
	CheckParam(socket.recvfrom, 3, port, OP_TYPE_INT);

	int addrLen = sizeof(SOCKADDR);

	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = inet_addr(stringRawValue(&host));
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons((u_short)port.iIntValue);

	int readSize = (int)len.iIntValue;
	char* buff = new char[readSize];
	int realSize = recvfrom(s, buff, readSize, 0, (SOCKADDR*)&addrSrv, &addrLen);
	XString* bufStr = vm->NewXString((const char*)buff, realSize);
	delete buff;

	vm->setReturnAsValue(vm->ConstructValue(bufStr));
}

void xsocket_sendto(XScriptVM* vm)
{
	CheckUserValueParam(socket.sendto, 0, s, SOCKET, "socket");
	CheckParam(socket.sendto, 1, buf, OP_TYPE_STRING);
	CheckParam(socket.sendto, 2, host, OP_TYPE_STRING);
	CheckParam(socket.sendto, 3, port, OP_TYPE_INT);

	int addrLen = sizeof(SOCKADDR);

	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = inet_addr(stringRawValue(&host));
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons((u_short)port.iIntValue);

	int err = sendto(s, stringRawValue(&buf), stringRawLen(&buf), 0, (SOCKADDR*)&addrSrv, addrLen);
	vm->setReturnAsInt((XInt)err);
}

void xsocket_accept(XScriptVM* vm)
{
	CheckUserValueParam(socket.accept, 0, s, SOCKET, "socket");
	int addrLen = sizeof(SOCKADDR);
	SOCKADDR_IN addrSrv;
	SOCKET rs = accept(s, (SOCKADDR*)&addrSrv, &addrLen);
	if (rs == INVALID_SOCKET)
	{
		vm->setReturnAsNil(0);
	}
	else
	{
		char* host = inet_ntoa(addrSrv.sin_addr);
		u_short port = ntohs(addrSrv.sin_port);
		vm->setReturnAsUserData("socket", (void*)rs, 0);
		vm->setReturnAsStr(host, 1);
		vm->setReturnAsInt((XInt)port, 2);
	}
}

void xsocket_setsockopt(XScriptVM* vm)
{
	CheckUserValueParam(socket.sendto, 0, s, SOCKET, "socket");
	CheckParam(socket.sendto, 1, level, OP_TYPE_INT);
	CheckParam(socket.sendto, 2, opt, OP_TYPE_INT);
	CheckParam(socket.sendto, 3, value, OP_TYPE_INT);
	int iValue = (int)value.iIntValue;
	int err = setsockopt(s, (int)level.iIntValue, (int)opt.iIntValue, (const char*)&iValue, sizeof(iValue));
	vm->setReturnAsInt((XInt)err);
}

void xsocket_getsockopt(XScriptVM* vm)
{
	CheckUserValueParam(socket.sendto, 0, s, SOCKET, "socket");
	CheckParam(socket.sendto, 1, level, OP_TYPE_INT);
	CheckParam(socket.sendto, 2, opt, OP_TYPE_INT);
	int iValue = 0;
	int iLen = sizeof(iValue);
	getsockopt(s, (int)level.iIntValue, (int)level.iIntValue, (char*)&iValue, &iLen);
	vm->setReturnAsInt((XInt)iValue);
}

void xsocket_ioctlsocket(XScriptVM* vm)
{
	CheckUserValueParam(socket.sendto, 0, s, SOCKET, "socket");
	CheckParam(socket.sendto, 1, cmd, OP_TYPE_INT);
	CheckParam(socket.sendto, 2, arg, OP_TYPE_INT);
	u_long uarg = (u_long)arg.iIntValue;
	int err = ioctlsocket(s, (long)cmd.iIntValue, &uarg);
	vm->setReturnAsInt((XInt)err);
}

void xsocket_gethostbyname(XScriptVM* vm)
{
	CheckParam(gethostbyname, 0, name, OP_TYPE_STRING);
	hostent* host = gethostbyname(stringRawValue(&name));
	if (host != NULL)
	{
		char* ip = inet_ntoa(*((struct in_addr *)host->h_addr));
		vm->setReturnAsStr(ip);
	}
	else
	{
		vm->setReturnAsNil(0);
	}
}

void xsocket_closesocket(XScriptVM* vm)
{
	CheckUserValueParam(socket.sendto, 0, s, SOCKET, "socket");
	closesocket(s);
}

void xsocket_getlasterror(XScriptVM* vm)
{
	vm->setReturnAsInt((XInt)WSAGetLastError());
}

void xsocket_connect_async(XScriptVM* vm)
{
	CheckParam(tcp_connect_async, 0, host, OP_TYPE_STRING);
	CheckParam(tcp_connect_async, 1, port, OP_TYPE_INT);
	std::string hostCopy = stringRawValue(&host);
	int portCopy = (int)port.iIntValue;
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));

	std::weak_ptr<AsyncRuntime> runtime = vm->GetAsyncRuntime();
	std::thread([runtime, promise, hostCopy, portCopy]() {
		SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		int err = 0;
		if (s == INVALID_SOCKET)
		{
			err = WSAGetLastError();
		}
		else
		{
			SOCKADDR_IN addr;
			addr.sin_addr.S_un.S_addr = inet_addr(hostCopy.c_str());
			addr.sin_family = AF_INET;
			addr.sin_port = htons((u_short)portCopy);
			err = connect(s, (SOCKADDR*)&addr, sizeof(SOCKADDR));
			if (err != 0)
			{
				err = WSAGetLastError();
				closesocket(s);
				s = INVALID_SOCKET;
			}
		}
		XScriptVM::QueueAsyncCompletion(runtime, [promise, s, err](XScriptVM* owner) {
			TABLE table = owner->newTable();
			owner->setTableValue(table, "ok", (XInt)(s != INVALID_SOCKET ? 1 : 0));
			if (s != INVALID_SOCKET)
			{
				Value sockVal;
				SetUserDataValue(&sockVal, owner->CreateClassUserData("socket", (void*)s));
				owner->setTableValue(table, owner->ConstructValue("socket"), sockVal);
			}
			else
			{
				owner->setTableValue(table, "error", (XInt)err);
			}
			promise->Resolve(owner->ConstructValue(table));
			owner->RemovePendingPromise(promise);
			delete promise;
		});
	}).detach();
}

void xsocket_recv_async(XScriptVM* vm)
{
	CheckUserValueParam(socket_read_async, 0, s, SOCKET, "socket");
	CheckParam(socket_read_async, 1, len, OP_TYPE_INT);
	int lenCopy = (int)len.iIntValue;
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));

	std::weak_ptr<AsyncRuntime> runtime = vm->GetAsyncRuntime();
	std::thread([runtime, promise, s, lenCopy]() {
		std::string data;
		int err = 0;
		int realSize = -1;
		if (lenCopy > 0)
		{
			data.resize((size_t)lenCopy);
			realSize = recv(s, &data[0], lenCopy, 0);
			if (realSize >= 0)
				data.resize((size_t)realSize);
			else
				err = WSAGetLastError();
		}
		XScriptVM::QueueAsyncCompletion(runtime, [promise, data, realSize, err](XScriptVM* owner) {
			TABLE table = owner->newTable();
			owner->setTableValue(table, "ok", (XInt)(realSize >= 0 ? 1 : 0));
			owner->setTableValue(table, "size", (XInt)realSize);
			if (realSize >= 0)
			{
				XString* str = owner->NewXString(data.data(), (int)data.size());
				owner->setTableValue(table, owner->ConstructValue("data"), owner->ConstructValue(str));
			}
			else
			{
				owner->setTableValue(table, "error", (XInt)err);
			}
			promise->Resolve(owner->ConstructValue(table));
			owner->RemovePendingPromise(promise);
			delete promise;
		});
	}).detach();
}

void xsocket_send_async(XScriptVM* vm)
{
	CheckUserValueParam(socket_write_async, 0, s, SOCKET, "socket");
	CheckParam(socket_write_async, 1, buf, OP_TYPE_STRING);
	std::string dataCopy(stringRawValue(&buf), (size_t)stringRawLen(&buf));
	XPromise* promise = vm->CreatePromise();
	vm->AddPendingPromise(promise);
	vm->setReturnAsValue(vm->ConstructValue(promise->future));

	std::weak_ptr<AsyncRuntime> runtime = vm->GetAsyncRuntime();
	std::thread([runtime, promise, s, dataCopy]() {
		int sent = send(s, dataCopy.data(), (int)dataCopy.size(), 0);
		int err = sent >= 0 ? 0 : WSAGetLastError();
		XScriptVM::QueueAsyncCompletion(runtime, [promise, sent, err](XScriptVM* owner) {
			TABLE table = owner->newTable();
			owner->setTableValue(table, "ok", (XInt)(sent >= 0 ? 1 : 0));
			owner->setTableValue(table, "size", (XInt)sent);
			if (sent < 0)
				owner->setTableValue(table, "error", (XInt)err);
			promise->Resolve(owner->ConstructValue(table));
			owner->RemovePendingPromise(promise);
			delete promise;
		});
	}).detach();
}