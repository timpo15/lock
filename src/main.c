//
// Created by timpo on 29/04/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "main.h"

int is_running = 0;
char *LOCK_EXT = ".lock";

int lock(char *filename) {
    int fd = -1;
    while (fd < 0) {
        fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        if (errno == EEXIST) {
            continue;
        } else {
            if (fd == -1) {
                return -1;
            }
        }
    }
    char pid[10];
    int len = sprintf(pid, "%d", getpid());


    if (write(fd, pid, len) < 0) {
        return -1;
    }

    if (close(fd) < 0) {
        return -1;
    }
    return 0;
}

int release_lock(char *filename) {
    int lock_fd = open(filename, O_RDONLY);
    if (lock_fd < 0) {
        return -1;
    }

    char pid[10];
    int len = read(lock_fd, pid, 10);
    if (len <= 0 || atoi(pid) != getpid()) {
        return -1;
    }
    if (remove(filename) < 0) {
        return -1;
    }
    return 0;
}

void graceful_shutdown(int sig) {
    is_running = 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./lock <file> \n");
        return -1;
    }

    if (signal(SIGINT, graceful_shutdown) == SIG_ERR) {
        return -1;
    }
    int suc = 0;
    int fail = 0;
    is_running = 1;

    char *locked_file = argv[1];
    strcat(locked_file, LOCK_EXT);
    while (is_running) {
        int res = lock(locked_file);
        if (res == -1) {
            return -1;
        }
        sleep(1);
        int fd = open(argv[1], O_WRONLY | O_APPEND);
        if (fd < 0) {
            return -1;
        }
        res = release_lock(locked_file);
        if (res == -1) {
            return -1;
        }
        close(fd);
        suc++;
    }
    record_statistics(suc, fail);
}

int record_statistics(int suc, int fail) {
    int res = lock("stats.lck");
    if (res == -1) {
        return -1;
    }
    int fd = open("stat.txt", O_APPEND | O_CREAT | O_RDWR, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));

    if (fd != -1) {
        char str[128];

        snprintf(str, 128, "PID: %d: GOOD: %d, BAD: %d\n", getpid(), suc, fail);
        int res = write(fd, str, strlen(str));
        if (res == -1) {
            release_lock("stats.lck");
            return -1;
        }
        if (close(fd) < 0) {
            release_lock("stats.lck");
            return -1;
        }
    } else {
        release_lock("stats.lck");
        return -1;
    }

    res = release_lock("stats.lck");
    if (res == -1) {
        return -1;
    }
    return 0;
}