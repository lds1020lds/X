#ifndef xhttplib_h__
#define xhttplib_h__

class XScriptVM;

void init_http_lib();
void xhttp_request(XScriptVM* vm);
void xhttp_get(XScriptVM* vm);
void xhttp_post(XScriptVM* vm);
void xhttp_get_async(XScriptVM* vm);
void xhttp_post_async(XScriptVM* vm);

#endif
