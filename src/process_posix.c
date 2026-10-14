#include "process.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static void child_error_and_exit(const char *message) {
    perror(message);
    _exit(EXIT_FAILURE);
}

int start_child(ChildProcess *child, const char *executable_path,
                const char *output_filename) {
    int pipe_fds[2];
    int output_fd;
    pid_t pid;

    child->pid = -1;
    child->write_fd = -1;
    child->started = 0;

    output_fd = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd == -1) {
        return -1;
    }

    if (pipe(pipe_fds) == -1) {
        int saved_errno = errno;
        close(output_fd);
        errno = saved_errno;
        return -1;
    }

    pid = fork();
    if (pid == -1) {
        int saved_errno = errno;
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        close(output_fd);
        errno = saved_errno;
        return -1;
    }

    if (pid == 0) {
        close(pipe_fds[1]);

        if (dup2(pipe_fds[0], STDIN_FILENO) == -1) {
            child_error_and_exit("Не удалось перенаправить stdin");
        }
        if (dup2(output_fd, STDOUT_FILENO) == -1) {
            child_error_and_exit("Не удалось перенаправить stdout");
        }

        close(pipe_fds[0]);
        close(output_fd);

        execl(executable_path, executable_path, (char *)NULL);
        child_error_and_exit("Не удалось запустить дочернюю программу");
    }

    close(pipe_fds[0]);
    close(output_fd);


    if (fcntl(pipe_fds[1], F_SETFD, FD_CLOEXEC) == -1) {
        int saved_errno = errno;
        close(pipe_fds[1]);
        while (waitpid(pid, NULL, 0) == -1 && errno == EINTR) {
        }
        errno = saved_errno;
        return -1;
    }

    child->pid = pid;
    child->write_fd = pipe_fds[1];
    child->started = 1;
    return 0;
}

int write_all(ChildProcess *child, const char *buffer, size_t size) {
    size_t total_written = 0;

    while (total_written < size) {
        ssize_t written = write(child->write_fd, buffer + total_written,
                                size - total_written);

        if (written == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (written == 0) {
            errno = EIO;
            return -1;
        }
        total_written += (size_t)written;
    }
    return 0;
}

int finish_child(ChildProcess *child) {
    int status;
    int close_error = 0;

    if (child->write_fd != -1) {
        if (close(child->write_fd) == -1) {
            close_error = errno;
        }
        child->write_fd = -1;
    }

    while (waitpid(child->pid, &status, 0) == -1) {
        if (errno != EINTR) {
            return -1;
        }
    }
    child->started = 0;

    if (close_error != 0) {
        errno = close_error;
        return -1;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
        errno = EIO;
        return -1;
    }

    return 0;
}

void print_process_error(const char *message) {
    perror(message);
}
