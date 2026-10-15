#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/types.h>
#endif

typedef struct {
#ifdef _WIN32
    HANDLE process_handle;
    HANDLE write_handle;
#else
    pid_t pid;
    int write_fd;
#endif
    int started;
} ChildProcess;


int start_child(ChildProcess *child, const char *executable_path,
                const char *output_filename);

int write_all(ChildProcess *child, const char *buffer, size_t size);

int finish_child(ChildProcess *child);

void print_process_error(const char *message);

#endif