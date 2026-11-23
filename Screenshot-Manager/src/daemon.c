#include "release_strings.h"
#include "daemon.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <unistd.h>

static const char suffix[] = "/ScreenshotManager/screenshot";

/* Match this application in either card slot. */
static int is_daemon(pid_t pid) {
    char path[64], target[PATH_MAX];
    snprintf(path, sizeof(path), sm_proc_exe, (long)pid);
    ssize_t n = readlink(path, target, sizeof(target) - 1);
    if (n < 0) {
        if (errno == ENOENT || errno == ESRCH) return 0; /* exited or kernel thread */
        return -1;
    }
    target[n] = 0;
    const char deleted[] = " (deleted)";
    if ((size_t)n >= sizeof(deleted) - 1 &&
        !strcmp(target + n - sizeof(deleted) + 1, deleted))
        target[n -= sizeof(deleted) - 1] = 0;
    return (size_t)n >= sizeof(suffix) - 1 &&
           !strcasecmp(target + n - sizeof(suffix) + 1, suffix);
}

static int scan(int stop) {
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
    DIR *proc = opendir("/proc");
    if (!proc) return -1;
    struct dirent *entry;
    int found = 0, failed = 0;
    for (;;) {
        errno = 0;
        entry = readdir(proc);
        if (!entry) { if (errno) failed = 1; break; }
        char *end;
        long pid = strtol(entry->d_name, &end, 10);
        if (*end || pid <= 1 || pid > INT_MAX) continue;
        /* Only inspect executables named screenshot. Other user IDs can have
           unreadable exe links, which say nothing about our process. */
        char path[64], name[32];
        snprintf(path, sizeof(path), "/proc/%ld/comm", pid);
        FILE *f = fopen(path, sm_read_mode);
        if (!f) { if (errno != ENOENT && errno != ESRCH) failed = 1; continue; }
        int match = fgets(name, sizeof(name), f) && !strcmp(name, sm_comm_name);
        fclose(f);
        if (!match) continue;
        int known = is_daemon((pid_t)pid);
        if (known < 0) { failed = 1; continue; }
        if (!known) continue;
        found = 1;
        if (!stop) break;
        if (kill((pid_t)pid, SIGTERM) < 0 && errno != ESRCH) failed = 1;
    }
    closedir(proc);
    return failed && (stop || !found) ? -1 : found;
}

int daemon_status(void) { return scan(0); }
int daemon_stop(void) { return scan(1) < 0 ? -1 : 0; }

int daemon_start(void) {
    int running = daemon_status();
    if (running != 0) return running > 0 ? 0 : -1;
    if (access(sm_daemon_check, X_OK) < 0) return -1;
    pid_t p = fork();
    if (p < 0) return -1;
    if (p == 0) {
        if (setsid() < 0 || chdir(sm_daemon_cwd) < 0) _exit(126);
        int fd = open(sm_null_device, O_RDWR);
        if (fd < 0) _exit(126);
        for (int i = 0; i < 3; ++i) if (dup2(fd, i) < 0) _exit(126);
        long limit = sysconf(_SC_OPEN_MAX);
        if (limit < 0) limit = 1024;
        for (long i = 3; i < limit; ++i) close(i);
        execl("./screenshot", sm_daemon_arg, (char *)NULL);
        _exit(127);
    }
    return 0;
}
