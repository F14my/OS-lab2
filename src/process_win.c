#include "process.h"

#ifdef _WIN32

#include <stdio.h>
#include <stdlib.h>

int start_child(ChildProcess *child, const char *executable_path,
                const char *output_filename) {
    SECURITY_ATTRIBUTES attributes = {sizeof(attributes), NULL, TRUE};
    STARTUPINFOA startup = {0};
    PROCESS_INFORMATION process = {0};
    HANDLE read_handle = NULL;
    HANDLE write_handle = NULL;
    HANDLE output_handle = INVALID_HANDLE_VALUE;
    DWORD error;

    child->process_handle = NULL;
    child->write_handle = NULL;
    child->started = 0;

    output_handle = CreateFileA(output_filename, GENERIC_WRITE, FILE_SHARE_READ,
                                &attributes, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, NULL);
    if (output_handle == INVALID_HANDLE_VALUE) {
        goto failed;
    }

    if (!CreatePipe(&read_handle, &write_handle, &attributes, 0)) {
        goto failed;
    }

    if (!SetHandleInformation(write_handle, HANDLE_FLAG_INHERIT, 0)) {
        goto failed;
    }

    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = read_handle;
    startup.hStdOutput = output_handle;
    startup.hStdError = output_handle;

    if (!CreateProcessA(executable_path, NULL, NULL, NULL, TRUE, 0, NULL, NULL,
                        &startup, &process)) {
        goto failed;
    }

    CloseHandle(process.hThread);
    CloseHandle(read_handle);
    CloseHandle(output_handle);
    child->process_handle = process.hProcess;
    child->write_handle = write_handle;
    child->started = 1;
    return 0;

failed:
    error = GetLastError();
    if (read_handle != NULL) CloseHandle(read_handle);
    if (write_handle != NULL) CloseHandle(write_handle);
    if (output_handle != INVALID_HANDLE_VALUE) CloseHandle(output_handle);
    SetLastError(error);
    return -1;
}

int write_all(ChildProcess *child, const char *buffer, size_t size) {
    size_t total_written = 0;

    while (total_written < size) {
        size_t remaining = size - total_written;
        DWORD requested = remaining > 65536u ? 65536u : (DWORD)remaining;
        DWORD written = 0;

        if (!WriteFile(child->write_handle, buffer + total_written,
                       requested, &written, NULL)) {
            return -1;
        }
        if (written == 0) {
            SetLastError(ERROR_WRITE_FAULT);
            return -1;
        }
        total_written += written;
    }
    return 0;
}

int finish_child(ChildProcess *child) {
    DWORD exit_code = EXIT_SUCCESS;
    DWORD error = ERROR_SUCCESS;

    if (child->write_handle != NULL) {
        if (!CloseHandle(child->write_handle)) {
            error = GetLastError();
        }
        child->write_handle = NULL;
    }

    if (WaitForSingleObject(child->process_handle, INFINITE) != WAIT_OBJECT_0) {
        if (error == ERROR_SUCCESS) error = GetLastError();
    } else if (!GetExitCodeProcess(child->process_handle, &exit_code)) {
        if (error == ERROR_SUCCESS) error = GetLastError();
    }
    if (!CloseHandle(child->process_handle) && error == ERROR_SUCCESS) {
        error = GetLastError();
    }
    child->process_handle = NULL;
    child->started = 0;

    if (error != ERROR_SUCCESS) {
        SetLastError(error);
        return -1;
    }
    if (exit_code != EXIT_SUCCESS) {
        SetLastError(ERROR_PROCESS_ABORTED);
        return -1;
    }
    return 0;
}

void print_process_error(const char *message) {
    fprintf(stderr, "%s: ошибка Windows %lu\n",
            message, (unsigned long)GetLastError());
}

#endif
