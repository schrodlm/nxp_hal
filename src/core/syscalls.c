/**
 * @file syscalls.c
 * @ingroup core
 * @brief Newlib syscalls implementation
 *
 * Provides minimal implementations of newlib system calls to support
 * printf/scanf via debug UART. Implements _read, _write, and other
 * required stubs for embedded operation.
 */

#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>

#include <core/debug_uart.h>


/* Variables */
void __io_putchar(int ch) {
    debug_uart_putchar(ch);
}

int __io_getchar(void) {
    return debug_uart_getchar();
}


int _read(int file, char* ptr, int len) {
    (void)file;
    int DataIdx;

    for (DataIdx = 0; DataIdx < len; DataIdx++) {
        *ptr++ = __io_getchar();
    }

    return len;
}

int _write(int file, char* ptr, int len) {
    (void)file;
    int DataIdx;

    for (DataIdx = 0; DataIdx < len; DataIdx++) {
        __io_putchar(*ptr++);
    }
    return len;
}

int _close(int file) {
    (void)file;
    return -1;
}


int _fstat(int file, struct stat* st) {
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir) {
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}
