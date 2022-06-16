#ifndef __LOCKFILE_H
#define __LOCKFILE_H

#include <unistd.h>
#include <fcntl.h>


#define LOCK_FILENAME_PREFIX ".dictx."

extern int lock_file;

#ifndef MAXPATHLEN
    #define MAXPATHLEN     4096
#endif

#define FLOCK_STRUCT(name, type, whence, start, len) \
struct flock name = {type, whence, start, len} 

#ifdef __cplusplus
extern "C"
{
#endif

void create_lock(char *lockfile_path);

int lockfile(int fd);

int rw_lock();

void rw_wait_lock();

void unlock();

#ifdef __cplusplus
}
#endif

#define USE_LOCKING

#ifdef USE_LOCKING
#  define RW_LOCKW(fd)                             \
    do {                                            \
        struct flock lock;                          \
        lock.l_type = F_WRLCK;                      \
        lock.l_start = 0;                           \
        lock.l_whence = SEEK_SET;                   \
        lock.l_len = 0;                             \
        if (fcntl(fd, F_SETLKW, &lock) != -1) {     \
            break;                                  \
        } else if (errno != EINTR) {                \
            return;                                 \
        }                                           \
    } while (1)

#  define RW_UNLOCK(fd)                           \
    do {                                            \
        int orig_errno = errno;                     \
        while (1) {                                 \
            struct flock lock;                      \
            lock.l_type = F_UNLCK;                  \
            lock.l_start = 0;                       \
            lock.l_whence = SEEK_SET;               \
            lock.l_len = 0;                         \
            if (fcntl(fd, F_SETLK, &lock) != -1) {  \
                break;                              \
            } else if (errno != EINTR) {            \
                return;                             \
            }                                       \
        }                                           \
        errno = orig_errno;                         \
    } while (0)
# else
#  define RW_LOCK(fd)
#  define RW_UNLOCK(fd)
# endif



#endif