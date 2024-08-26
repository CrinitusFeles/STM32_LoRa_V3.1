#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "time.h"
#include "stm32l4xx.h"
#include "uart.h"

typedef int FILEHANDLE;

#pragma import(__use_no_semihosting_swi)
#pragma import(_main_redirection)

const char __stdin_name[150];
const char __stdout_name[150];
const char __stderr_name[150];

FILEHANDLE _sys_open(const char *name, int openmode)
{
    return 0;
}

int _sys_close(FILEHANDLE fh)
{
    return 0;
}

int _sys_write(FILEHANDLE fh, const unsigned char *buf, unsigned len, int mode)
{
    return 0;
}

int _sys_read(FILEHANDLE fh, unsigned char *buf, unsigned len, int mode)
{
    return 0;
}

// Check whether the handle is a terminal

int _sys_istty(FILEHANDLE fh)
{
    return 0;
}

int _sys_seek(FILEHANDLE fh, long pos)
{
    return 0;
}

// Flushes the buffer associated with the handle

int _sys_ensure(FILEHANDLE fh)
{
    return 0;
}

// Returns the current length of the file

long _sys_flen(FILEHANDLE fh)
{
    return 0;
}

void _sys_exit(int status)
{
    // while(1);
}

int _sys_tmpnam(char *name, int fileno, unsigned maxlength)
{
    return 0;
}

// Write a character to the console

void _ttywrch(int ch)
{
}

// int remove(const char *filename)
// {
//     return 0;
// }

char *_sys_command_string(char *cmd, int len)
{
    return NULL;
}

time_t time(time_t *time)
{
    return 0;
}

// Redefine fputc function
int fputc(int ch, FILE *f)
{
    UART_tx(UART_tx, (uint8_t)ch);
    return ch;
}

size_t fwrite(const void *buf, size_t size, size_t count, FILE *stream) {
    stream = stream;
    size_t len = size * count;
    const char *s = (char*)(buf);

    for (size_t i = 0; i < len; i++) {
        UART_tx(UART_tx, (s[i]));
    }

    return len;
}

// int mc_fflush () {
//     uint32_t len = buf_p;
//     buf_p = 0;
//     if (uart_1.tx(tx_buf, len, 100) != mc_interfaces::res::ok) {
//         errno = EIO;
//         return -1;
//     }

//     return 0;
// }