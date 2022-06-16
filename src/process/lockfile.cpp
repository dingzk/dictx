#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "lockfile.h"

int lock_file;
static char lockfile_name[MAXPATHLEN];

void
create_lock(char *lockfile_path) {
    int val;

    snprintf(lockfile_name, sizeof(lockfile_name), "%s/%sXXXXXX", lockfile_path, LOCK_FILENAME_PREFIX);
    lock_file = mkstemp(lockfile_name);
    fchmod(lock_file, 0666);
    if (lock_file == -1) {
        perror("Unable to create lock file");
    }

    val = fcntl(lock_file, F_GETFD, 0);
    val |= FD_CLOEXEC;
    fcntl(lock_file, F_SETFD, val);

    unlink(lockfile_name);
}

int
lockfile(int fd) {
    FLOCK_STRUCT(write_lock, F_WRLCK, SEEK_SET, 0, 0);

    return fcntl(fd, F_SETLK, &write_lock);
}

int
rw_lock() {
    return lockfile(lock_file);
}

void
rw_wait_lock() {
    FLOCK_STRUCT(write_lock, F_WRLCK, SEEK_SET, 0, 0);

    while (1) {
        if (fcntl(lock_file, F_SETLKW, &write_lock) == -1) {
            if (errno == EINTR)
                continue;
            sched_yield(); /*release cpu time*/
        }
        break;
    }

}

void
unlock() {
    FLOCK_STRUCT(write_unlock, F_UNLCK, SEEK_SET, 0, 0);

    if (fcntl(lock_file, F_SETLK, &write_unlock) == -1) {
        perror("unlock field");
    }

}


