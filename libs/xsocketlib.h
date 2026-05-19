#pragma once
#ifndef xsocketlib_h__
#define xsocketlib_h__

class XScriptVM;

void init_socket_lib();

void xsocket_create(XScriptVM* vm);
void xsocket_bind(XScriptVM* vm);
void xsocket_connect(XScriptVM* vm);
void xsocket_listen(XScriptVM* vm);
void xsocket_recv(XScriptVM* vm);
void xsocket_send(XScriptVM* vm);
void xsocket_accept(XScriptVM* vm);
void xsocket_recvfrom(XScriptVM* vm);
void xsocket_sendto(XScriptVM* vm);
void xsocket_setsockopt(XScriptVM* vm);
void xsocket_getsockopt(XScriptVM* vm);
void xsocket_ioctlsocket(XScriptVM* vm);
void xsocket_gethostbyname(XScriptVM* vm);
void xsocket_closesocket(XScriptVM* vm);
void xsocket_getlasterror(XScriptVM* vm);
void xsocket_connect_async(XScriptVM* vm);
void xsocket_recv_async(XScriptVM* vm);
void xsocket_send_async(XScriptVM* vm);

#endif // xsocketlib_h__