#pragma once
#ifndef xio_h__
#define xio_h__

class XScriptVM;

void init_io_lib();

void host_io_input(XScriptVM* vm);
void host_io_output(XScriptVM* vm);

void file_open(XScriptVM* vm);
void file_close(XScriptVM* vm);
void file_seek(XScriptVM* vm);
void file_read(XScriptVM* vm);
void file_write(XScriptVM* vm);
void file_readline(XScriptVM* vm);
void file_writeline(XScriptVM* vm);
void file_writelines(XScriptVM* vm);
void file_flush(XScriptVM* vm);
void file_eof(XScriptVM* vm);
void file_size(XScriptVM* vm);
void file_ftell(XScriptVM* vm);
void file_read_async(XScriptVM* vm);
void file_write_async(XScriptVM* vm);
void file_append_async(XScriptVM* vm);
void file_exists_async(XScriptVM* vm);
void list_dir_async(XScriptVM* vm);
void mkdir_async(XScriptVM* vm);
void remove_file_async(XScriptVM* vm);

#endif // xio_h__