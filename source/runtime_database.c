/* runtime database. */
#define _GNU_SOURCE 1
#include "runtime_base.h"


/* ---- sqlite_host ---- */
#pragma push_macro("SSRNX_VFS_JOURNAL_BUFFER_SIZE")
#pragma push_macro("SSRNX_VFS_PATH_CAP")
#pragma push_macro("SQLITE_DEFAULT_FILE_PERMISSIONS")
#pragma push_macro("SQLITE_DEFAULT_MEMSTATUS")
#pragma push_macro("SQLITE_ENABLE_FTS3")
#pragma push_macro("SQLITE_ENABLE_FTS3_PARENTHESIS")
#pragma push_macro("SQLITE_ENABLE_FTS4")
#pragma push_macro("SQLITE_ENABLE_RTREE")
#pragma push_macro("SQLITE_NATIVE_FN")
#pragma push_macro("SQLITE_OMIT_LOAD_EXTENSION")
#pragma push_macro("SQLITE_OMIT_SHARED_CACHE")
#pragma push_macro("SQLITE_OS_OTHER")
#pragma push_macro("SQLITE_TEMP_STORE")
#pragma push_macro("SQLITE_THREADSAFE")
/* SQLite (third_party/sqlite amalgamation, public domain) for Android's
 * android.database.sqlite, built with an SSRNX VFS instead of SQLite's unix
 * layer:
 *
 * - Android guest paths (/data/data/<pkg>/databases/...) map to host storage
 *   through the same namespace as every other file API.
 * - Horizon offers no fcntl byte-range locks. SSRNX is one Android process,
 *   so SQLite's file locks are kept in an in-process lock table with the
 *   standard SHARED/RESERVED/PENDING/EXCLUSIVE semantics between connections.
 * - The VFS has no shared-memory methods, so journal_mode=WAL is refused and
 *   databases stay in rollback-journal mode (Android's own fallback).
 * - SQLITE_OS_OTHER selects SQLite's no-op mutexes; ssrnx_sqlite_initialize()
 *   installs pthread mutexes (SQLITE_CONFIG_MUTEX) before first use, giving
 *   Android's serialized threading mode. */

#define SQLITE_OS_OTHER 1
#define SQLITE_THREADSAFE 1
#define SQLITE_TEMP_STORE 3
#define SQLITE_DEFAULT_MEMSTATUS 0
#define SQLITE_OMIT_LOAD_EXTENSION 1
#define SQLITE_OMIT_SHARED_CACHE 1
#define SQLITE_ENABLE_FTS3 1
#define SQLITE_ENABLE_FTS3_PARENTHESIS 1
#define SQLITE_ENABLE_FTS4 1
#define SQLITE_ENABLE_RTREE 1
#define SQLITE_DEFAULT_FILE_PERMISSIONS 0600
#include "../third_party/sqlite/sqlite3.c"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>


#ifdef __SWITCH__
/* Avoid including switch.h after sqlite3.c: both expose a private u64 typedef
 * with ABI-equivalent but C-incompatible underlying integer spellings. */
extern uint32_t fsdevCommitDevice(const char *name);
extern void ssrnx_log_category(const char *category, const char *level, const char *fmt, ...) __attribute__((weak));
#endif

extern bool ssrnx_compat_map_guest_path(const char *guest_path, char *out, size_t cap) __attribute__((weak));
extern void ssrnx_fd_random_fill(void *buffer, size_t count) __attribute__((weak));
extern void ssrnx_file_contract_open(int fd, const char *guest_path, const char *host_path,
                                     int android_flags, int host_flags, bool package_apk) __attribute__((weak));
extern void ssrnx_file_contract_close(int fd) __attribute__((weak));
extern int ssrnx_file_contract_fd_for_host_path(const char *host_path) __attribute__((weak));

#define SSRNX_VFS_PATH_CAP 1024
#define SSRNX_VFS_JOURNAL_BUFFER_SIZE 8192

static void sqlite_host_ssrnx_vfs_host_path(const char *guest, char *out, size_t cap) {
    if (!ssrnx_compat_map_guest_path || !ssrnx_compat_map_guest_path(guest, out, cap))
        snprintf(out, cap, "%s", guest);
}

/* ---- in-process lock table (one node per host database path) ---- */

typedef struct sqlite_host_SsrnxVfsLock {
    char path[SSRNX_VFS_PATH_CAP];
    int shared;             /* connections holding at least SHARED */
    const void *reserved;   /* owner of RESERVED */
    const void *pending;    /* owner of PENDING */
    const void *exclusive;  /* owner of EXCLUSIVE */
    int refs;
    struct sqlite_host_SsrnxVfsLock *next;
} sqlite_host_SsrnxVfsLock;

static pthread_mutex_t sqlite_host_g_vfs_lock = PTHREAD_MUTEX_INITIALIZER;
static sqlite_host_SsrnxVfsLock *sqlite_host_g_vfs_locks;

static sqlite_host_SsrnxVfsLock *sqlite_host_ssrnx_vfs_lock_acquire(const char *path) {
    pthread_mutex_lock(&sqlite_host_g_vfs_lock);
    sqlite_host_SsrnxVfsLock *node = sqlite_host_g_vfs_locks;
    while (node && strcmp(node->path, path) != 0) node = node->next;
    if (!node && (node = calloc(1, sizeof(*node))) != NULL) {
        snprintf(node->path, sizeof(node->path), "%s", path);
        node->next = sqlite_host_g_vfs_locks;
        sqlite_host_g_vfs_locks = node;
    }
    if (node) node->refs++;
    pthread_mutex_unlock(&sqlite_host_g_vfs_lock);
    return node;
}

static void sqlite_host_ssrnx_vfs_lock_release(sqlite_host_SsrnxVfsLock *node) {
    if (!node) return;
    pthread_mutex_lock(&sqlite_host_g_vfs_lock);
    if (--node->refs == 0) {
        sqlite_host_SsrnxVfsLock **link = &sqlite_host_g_vfs_locks;
        while (*link && *link != node) link = &(*link)->next;
        if (*link) *link = node->next;
        free(node);
    }
    pthread_mutex_unlock(&sqlite_host_g_vfs_lock);
}

typedef struct sqlite_host_SsrnxVfsFile {
    sqlite3_file base;
    int fd;
    int level;               /* SQLITE_LOCK_* held by this connection */
    bool delete_on_close;
    sqlite_host_SsrnxVfsLock *lock;      /* main databases only */
    pthread_mutex_t io_lock;
    bool dir_sync_pending;   /* first durable sync after journal/super-journal creation */
    unsigned char *write_buffer; /* rollback journal only; SQLite owns sync ordering */
    int buffered;
    sqlite3_int64 buffer_offset;
    char host_path[SSRNX_VFS_PATH_CAP];
} sqlite_host_SsrnxVfsFile;

static int sqlite_host_ssrnx_vfs_flush_locked(sqlite_host_SsrnxVfsFile *p);

static int sqlite_host_ssrnx_vfs_errno_to_sqlite(int err, int fallback) {
    if (err == ENOSPC) return SQLITE_FULL;
    if (err == ENOMEM) return SQLITE_NOMEM;
    if (err == EACCES || err == EPERM || err == EROFS) return SQLITE_READONLY;
    return fallback;
}

/* SQLite uses directory sync as an ordering barrier when rollback journals are
 * created/deleted. On Switch there is no POSIX directory fd exposed by fsdev,
 * so commit the mounted filesystem. On host regressions, fsync the parent
 * directory directly. */
static int sqlite_host_ssrnx_vfs_sync_parent_dir(const char *host_path) {
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    if (!host_path || !host_path[0]) return SQLITE_OK;
#ifdef __SWITCH__
    const char *colon = strchr(host_path, ':');
    if (!colon || colon == host_path || (size_t)(colon - host_path) >= 32u)
        return SQLITE_OK;
    char device[32];
    const size_t len = (size_t)(colon - host_path);
    memcpy(device, host_path, len);
    device[len] = '\0';
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    const uint32_t rc = fsdevCommitDevice(device);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    const long long us = (long long)(t1.tv_sec - t0.tv_sec) * 1000000LL + (t1.tv_nsec - t0.tv_nsec) / 1000;
    if (us >= 20000)
        if (ssrnx_log_category)
            ssrnx_log_category("PERF", "INFO", "slow SQLite directory sync (device commit) device=%s elapsedUs=%lld", device, us);
    return rc == 0 ? SQLITE_OK : SQLITE_IOERR_DIR_FSYNC;
#else
    char dir[SSRNX_VFS_PATH_CAP];
    snprintf(dir, sizeof(dir), "%s", host_path);
    char *slash = strrchr(dir, '/');
    if (!slash) snprintf(dir, sizeof(dir), ".");
    else if (slash == dir) slash[1] = '\0';
    else *slash = '\0';
    int flags = O_RDONLY;
#ifdef O_DIRECTORY
    flags |= O_DIRECTORY;
#endif
    const int dfd = open(dir, flags);
    if (dfd < 0) return SQLITE_OK; /* matches SQLite unix VFS open-dir fallback */
    const int rc = fsync(dfd);
    const int saved_errno = errno;
    close(dfd);
    if (rc == 0) return SQLITE_OK;
    errno = saved_errno;
    return SQLITE_IOERR_DIR_FSYNC;
#endif
}

static int sqlite_host_ssrnx_vfs_close(sqlite3_file *f) {
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    sqlite_host_SsrnxVfsFile *p = (sqlite_host_SsrnxVfsFile *)f;
    pthread_mutex_lock(&p->io_lock);
    int rc = sqlite_host_ssrnx_vfs_flush_locked(p);
    pthread_mutex_unlock(&p->io_lock);
    if (p->lock) {
        pthread_mutex_lock(&sqlite_host_g_vfs_lock);
        if (p->level >= SQLITE_LOCK_SHARED && p->lock->shared > 0) p->lock->shared--;
        if (p->lock->reserved == p) p->lock->reserved = NULL;
        if (p->lock->pending == p) p->lock->pending = NULL;
        if (p->lock->exclusive == p) p->lock->exclusive = NULL;
        pthread_mutex_unlock(&sqlite_host_g_vfs_lock);
        sqlite_host_ssrnx_vfs_lock_release(p->lock);
        p->lock = NULL;
    }
    if (p->fd >= 0) {
        const int fd = p->fd;
        if (ssrnx_file_contract_close) ssrnx_file_contract_close(fd);
        if (close(fd) != 0 && rc == SQLITE_OK) rc = SQLITE_IOERR_CLOSE;
    }
    p->fd = -1;
    if (p->delete_on_close) (void)unlink(p->host_path);
    free(p->write_buffer);
    pthread_mutex_destroy(&p->io_lock);
    return rc;
}

static int sqlite_host_ssrnx_vfs_read(sqlite3_file *f, void *buf, int amount, sqlite3_int64 offset) {
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    SSRNX_IO_TIMING();
    sqlite_host_SsrnxVfsFile *p = (sqlite_host_SsrnxVfsFile *)f;
    pthread_mutex_lock(&p->io_lock);
    int got = 0;
    int rc = sqlite_host_ssrnx_vfs_flush_locked(p);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&p->io_lock);
        return rc;
    }
    if (lseek(p->fd, (off_t)offset, SEEK_SET) == (off_t)-1) {
        rc = SQLITE_IOERR_SEEK;
    } else {
        while (got < amount) {
            const ssize_t n = read(p->fd, (char *)buf + got, (size_t)(amount - got));
            if (n < 0) {
                if (errno == EINTR) continue;
                /* libnx fsdev can surface EIO where POSIX read() would report
                 * EOF for a just-created/open file. Only normalize that host
                 * artefact when fstat proves the requested position is at or
                 * beyond the current end of file; real in-range EIO remains a
                 * SQLite read failure. */
                if (errno == EIO) {
                    struct stat st;
                    if (fstat(p->fd, &st) == 0 && offset + got >= (sqlite3_int64)st.st_size)
                        break;
                }
                rc = SQLITE_IOERR_READ;
                break;
            }
            if (n == 0) break;
            got += (int)n;
        }
    }
    pthread_mutex_unlock(&p->io_lock);
    if (rc != SQLITE_OK) return rc;
    if (got < amount) {
        memset((char *)buf + got, 0, (size_t)(amount - got));
        return SQLITE_IOERR_SHORT_READ;
    }
    return SQLITE_OK;
}

static int sqlite_host_ssrnx_vfs_direct_write_locked(sqlite_host_SsrnxVfsFile *p, const void *buf, int amount, sqlite3_int64 offset) {
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    SSRNX_IO_TIMING();
    const uint64_t begin = ssrnx_io_time_us();
    int done = 0;
    int rc = SQLITE_OK;
    const off_t position = lseek(p->fd, (off_t)offset, SEEK_SET);
    const uint64_t sought = ssrnx_io_time_us();
    if (position == (off_t)-1) {
        rc = SQLITE_IOERR_SEEK;
    } else {
        while (done < amount) {
            const ssize_t n = write(p->fd, (const char *)buf + done, (size_t)(amount - done));
            if (n < 0) {
                if (errno == EINTR) continue;
                rc = sqlite_host_ssrnx_vfs_errno_to_sqlite(errno, SQLITE_IOERR_WRITE);
                break;
            }
            if (n == 0) {
                rc = SQLITE_IOERR_WRITE;
                break;
            }
            done += (int)n;
        }
    }
    const uint64_t finished = ssrnx_io_time_us();
    const int saved_errno = errno;
    static unsigned reports;
    if (finished - begin >= 50000 && ssrnx_log_category &&
        __atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 16u)
        ssrnx_log_category("PERF", "INFO",
                           "SQLite physical write journal=%d bytes=%d offset=%lld seekUs=%llu writeUs=%llu completed=%d rc=%d errno=%d",
                           p->write_buffer != NULL, amount, (long long)offset,
                           (unsigned long long)(sought - begin), (unsigned long long)(finished - sought),
                           done, rc, rc == SQLITE_OK ? 0 : saved_errno);
    errno = saved_errno;
    return rc;
}

/* SQLite's embedded VFS example documents coalescing the journal's 4-byte
 * page numbers/checksums and page data. Flush at every visibility/durability
 * boundary, before a nonsequential write, and when the fixed buffer fills.
 * Database pages remain direct writes. Caller holds io_lock. */
static int sqlite_host_ssrnx_vfs_flush_locked(sqlite_host_SsrnxVfsFile *p) {
    if (!p->buffered) return SQLITE_OK;
    const int rc = sqlite_host_ssrnx_vfs_direct_write_locked(p, p->write_buffer, p->buffered, p->buffer_offset);
    if (rc == SQLITE_OK) p->buffered = 0; /* retain all data on partial/error writes */
    return rc;
}

static int sqlite_host_ssrnx_vfs_write(sqlite3_file *f, const void *buf, int amount, sqlite3_int64 offset) {
    SSRNX_IO_TIMING();
    sqlite_host_SsrnxVfsFile *p = (sqlite_host_SsrnxVfsFile *)f;
    pthread_mutex_lock(&p->io_lock);
    int rc = SQLITE_OK;
    if (!p->write_buffer) {
        rc = sqlite_host_ssrnx_vfs_direct_write_locked(p, buf, amount, offset);
    } else {
        const unsigned char *source = buf;
        while (amount > 0) {
            if (p->buffered == SSRNX_VFS_JOURNAL_BUFFER_SIZE ||
                (p->buffered && p->buffer_offset + p->buffered != offset)) {
                rc = sqlite_host_ssrnx_vfs_flush_locked(p);
                if (rc != SQLITE_OK) break;
            }
            if (!p->buffered) p->buffer_offset = offset;
            int chunk = SSRNX_VFS_JOURNAL_BUFFER_SIZE - p->buffered;
            if (chunk > amount) chunk = amount;
            memcpy(p->write_buffer + p->buffered, source, (size_t)chunk);
            p->buffered += chunk;
            source += chunk;
            offset += chunk;
            amount -= chunk;
        }
    }
    pthread_mutex_unlock(&p->io_lock);
    return rc;
}

static int sqlite_host_ssrnx_vfs_truncate(sqlite3_file *f, sqlite3_int64 size) {
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    sqlite_host_SsrnxVfsFile *p = (sqlite_host_SsrnxVfsFile *)f;
    pthread_mutex_lock(&p->io_lock);
    int rc = sqlite_host_ssrnx_vfs_flush_locked(p);
    if (rc == SQLITE_OK && ftruncate(p->fd, (off_t)size) != 0) rc = SQLITE_IOERR_TRUNCATE;
    pthread_mutex_unlock(&p->io_lock);
    return rc;
}

static int sqlite_host_ssrnx_vfs_sync(sqlite3_file *f, int flags) {
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    SSRNX_IO_TIMING();
    (void)flags;
    sqlite_host_SsrnxVfsFile *p = (sqlite_host_SsrnxVfsFile *)f;
    pthread_mutex_lock(&p->io_lock);
    int rc = sqlite_host_ssrnx_vfs_flush_locked(p);
    if (rc == SQLITE_OK && fsync(p->fd) != 0) rc = SQLITE_IOERR_FSYNC;
    pthread_mutex_unlock(&p->io_lock);
    if (rc != SQLITE_OK) return rc;

    if (p->dir_sync_pending) {
        const int dir_rc = sqlite_host_ssrnx_vfs_sync_parent_dir(p->host_path);
        if (dir_rc != SQLITE_OK) return dir_rc;
        p->dir_sync_pending = false;
    }
    return SQLITE_OK;
}

static int sqlite_host_ssrnx_vfs_file_size(sqlite3_file *f, sqlite3_int64 *size) {
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    sqlite_host_SsrnxVfsFile *p = (sqlite_host_SsrnxVfsFile *)f;
    struct stat st;
    pthread_mutex_lock(&p->io_lock);
    const int flush_rc = sqlite_host_ssrnx_vfs_flush_locked(p);
    if (flush_rc != SQLITE_OK) {
        pthread_mutex_unlock(&p->io_lock);
        return flush_rc;
    }
    int rc = fstat(p->fd, &st);
    if (rc != 0 && errno == EIO) rc = stat(p->host_path, &st);
    pthread_mutex_unlock(&p->io_lock);
    if (rc != 0) return SQLITE_IOERR_FSTAT;
    *size = (sqlite3_int64)st.st_size;
    return SQLITE_OK;
}

static int sqlite_host_ssrnx_vfs_lock(sqlite3_file *f, int level) {
    sqlite_host_SsrnxVfsFile *p = (sqlite_host_SsrnxVfsFile *)f;
    if (!p->lock || level <= p->level) return SQLITE_OK;
    if (level < SQLITE_LOCK_SHARED || level > SQLITE_LOCK_EXCLUSIVE)
        return SQLITE_IOERR_LOCK;

    int rc = SQLITE_OK;
    pthread_mutex_lock(&sqlite_host_g_vfs_lock);
    sqlite_host_SsrnxVfsLock *l = p->lock;
    const int self_shared = p->level >= SQLITE_LOCK_SHARED ? 1 : 0;

    switch (level) {
        case SQLITE_LOCK_SHARED:
            if ((l->pending && l->pending != p) || (l->exclusive && l->exclusive != p)) {
                rc = SQLITE_BUSY;
            } else {
                l->shared++;
                p->level = SQLITE_LOCK_SHARED;
            }
            break;

        case SQLITE_LOCK_RESERVED:
            if (p->level < SQLITE_LOCK_SHARED) {
                rc = SQLITE_IOERR_LOCK;
            } else if ((l->reserved && l->reserved != p) ||
                       (l->pending && l->pending != p) ||
                       (l->exclusive && l->exclusive != p)) {
                rc = SQLITE_BUSY;
            } else {
                l->reserved = p;
                p->level = SQLITE_LOCK_RESERVED;
            }
            break;

        case SQLITE_LOCK_PENDING:
            if (p->level < SQLITE_LOCK_SHARED) {
                rc = SQLITE_IOERR_LOCK;
            } else if ((l->reserved && l->reserved != p) ||
                       (l->pending && l->pending != p) ||
                       (l->exclusive && l->exclusive != p)) {
                rc = SQLITE_BUSY;
            } else {
                l->pending = p;
                p->level = SQLITE_LOCK_PENDING;
            }
            break;

        case SQLITE_LOCK_EXCLUSIVE:
            /* Hot-journal recovery goes SHARED -> PENDING -> EXCLUSIVE
             * without RESERVED, which would incorrectly mark it non-hot. */
            if (p->level < SQLITE_LOCK_SHARED) {
                rc = SQLITE_IOERR_LOCK;
            } else if ((l->reserved && l->reserved != p) ||
                       (l->pending && l->pending != p) ||
                       (l->exclusive && l->exclusive != p) ||
                       l->shared > self_shared) {
                /* Preserve PENDING while waiting for existing SHARED readers,
                 * exactly as rollback-journal locking requires. */
                if ((!l->reserved || l->reserved == p) &&
                    (!l->pending || l->pending == p) &&
                    (!l->exclusive || l->exclusive == p)) {
                    l->pending = p;
                    p->level = SQLITE_LOCK_PENDING;
                }
                rc = SQLITE_BUSY;
            } else {
                l->pending = p;
                l->exclusive = p;
                p->level = SQLITE_LOCK_EXCLUSIVE;
            }
            break;
    }
    pthread_mutex_unlock(&sqlite_host_g_vfs_lock);
    return rc;
}

static int sqlite_host_ssrnx_vfs_unlock(sqlite3_file *f, int level) {
    sqlite_host_SsrnxVfsFile *p = (sqlite_host_SsrnxVfsFile *)f;
    if (!p->lock || level >= p->level) return SQLITE_OK;
    if (level != SQLITE_LOCK_SHARED && level != SQLITE_LOCK_NONE)
        return SQLITE_IOERR_UNLOCK;

    pthread_mutex_lock(&sqlite_host_g_vfs_lock);
    sqlite_host_SsrnxVfsLock *l = p->lock;
    if (l->exclusive == p) l->exclusive = NULL;
    if (l->pending == p) l->pending = NULL;
    if (l->reserved == p) l->reserved = NULL;
    if (level == SQLITE_LOCK_NONE && p->level >= SQLITE_LOCK_SHARED && l->shared > 0)
        l->shared--;
    p->level = level;
    pthread_mutex_unlock(&sqlite_host_g_vfs_lock);
    return SQLITE_OK;
}

static int sqlite_host_ssrnx_vfs_check_reserved(sqlite3_file *f, int *out) {
    sqlite_host_SsrnxVfsFile *p = (sqlite_host_SsrnxVfsFile *)f;
    if (!out) return SQLITE_IOERR_CHECKRESERVEDLOCK;
    pthread_mutex_lock(&sqlite_host_g_vfs_lock);
    *out = p->lock && (p->lock->reserved || p->lock->pending || p->lock->exclusive) ? 1 : 0;
    pthread_mutex_unlock(&sqlite_host_g_vfs_lock);
    return SQLITE_OK;
}

static int sqlite_host_ssrnx_vfs_file_control(sqlite3_file *f, int op, void *arg) {
    (void)f; (void)op; (void)arg;
    return SQLITE_NOTFOUND;
}

static int sqlite_host_ssrnx_vfs_sector_size(sqlite3_file *f) {
    (void)f;
    return 4096;
}

static int sqlite_host_ssrnx_vfs_device_characteristics(sqlite3_file *f) {
    (void)f;
    return 0;
}

static const sqlite3_io_methods sqlite_host_k_ssrnx_io = {
    1, sqlite_host_ssrnx_vfs_close, sqlite_host_ssrnx_vfs_read, sqlite_host_ssrnx_vfs_write, sqlite_host_ssrnx_vfs_truncate, sqlite_host_ssrnx_vfs_sync,
    sqlite_host_ssrnx_vfs_file_size, sqlite_host_ssrnx_vfs_lock, sqlite_host_ssrnx_vfs_unlock, sqlite_host_ssrnx_vfs_check_reserved,
    sqlite_host_ssrnx_vfs_file_control, sqlite_host_ssrnx_vfs_sector_size, sqlite_host_ssrnx_vfs_device_characteristics,
    NULL, NULL, NULL, NULL, NULL, NULL,
};

static int sqlite_host_ssrnx_vfs_open(sqlite3_vfs *vfs, sqlite3_filename name, sqlite3_file *f, int flags, int *out_flags) {
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    (void)vfs;
    sqlite_host_SsrnxVfsFile *p = (sqlite_host_SsrnxVfsFile *)f;
    memset(p, 0, sizeof(*p));
    p->fd = -1;
    /* SQLITE_TEMP_STORE=3 keeps temporary tables in memory; nameless files
     * are only requested for them. */
    if (!name) return SQLITE_CANTOPEN;
    sqlite_host_ssrnx_vfs_host_path(name, p->host_path, sizeof(p->host_path));
    struct stat preopen_st;
    const bool existed_before = stat(p->host_path, &preopen_st) == 0;
    int oflags = (flags & SQLITE_OPEN_READWRITE) ? O_RDWR : O_RDONLY;
    if (flags & SQLITE_OPEN_CREATE) oflags |= O_CREAT;
    if (flags & SQLITE_OPEN_EXCLUSIVE) oflags |= O_EXCL;
    p->fd = open(p->host_path, oflags, 0600);
    if (p->fd < 0 && (flags & SQLITE_OPEN_READWRITE) && !(flags & SQLITE_OPEN_CREATE) &&
        (errno == EACCES || errno == EROFS)) {
        p->fd = open(p->host_path, O_RDONLY);
        flags = (flags & ~SQLITE_OPEN_READWRITE) | SQLITE_OPEN_READONLY;
    }
    if (p->fd < 0) return SQLITE_CANTOPEN;
    if (pthread_mutex_init(&p->io_lock, NULL) != 0) {
        close(p->fd);
        p->fd = -1;
        return SQLITE_NOMEM;
    }
    if (ssrnx_file_contract_open)
        ssrnx_file_contract_open(p->fd, name, p->host_path, 0, oflags, false);
    p->delete_on_close = (flags & SQLITE_OPEN_DELETEONCLOSE) != 0;
    p->dir_sync_pending = !existed_before && (flags & SQLITE_OPEN_CREATE) != 0 &&
        (flags & (SQLITE_OPEN_MAIN_JOURNAL | SQLITE_OPEN_SUPER_JOURNAL | SQLITE_OPEN_WAL)) != 0;
    if (flags & SQLITE_OPEN_MAIN_DB) {
        p->lock = sqlite_host_ssrnx_vfs_lock_acquire(p->host_path);
        if (!p->lock) {
            if (ssrnx_file_contract_close) ssrnx_file_contract_close(p->fd);
            close(p->fd);
            p->fd = -1;
            pthread_mutex_destroy(&p->io_lock);
            return SQLITE_NOMEM;
        }
    }
    if (flags & SQLITE_OPEN_MAIN_JOURNAL)
        p->write_buffer = malloc(SSRNX_VFS_JOURNAL_BUFFER_SIZE); /* OOM: direct writes */
    p->base.pMethods = &sqlite_host_k_ssrnx_io;
    if (out_flags) *out_flags = flags;
    return SQLITE_OK;
}

static int sqlite_host_ssrnx_vfs_delete(sqlite3_vfs *vfs, const char *name, int sync_dir) {
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    (void)vfs;
    char host[SSRNX_VFS_PATH_CAP];
    sqlite_host_ssrnx_vfs_host_path(name, host, sizeof(host));
    const bool deleted = unlink(host) == 0;
    if (!deleted && errno != ENOENT) return SQLITE_IOERR_DELETE;
    if (sync_dir && deleted) return sqlite_host_ssrnx_vfs_sync_parent_dir(host);
    return SQLITE_OK;
}

static int sqlite_host_ssrnx_vfs_access(sqlite3_vfs *vfs, const char *name, int flags, int *out) {
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    (void)vfs;
    if (!out) return SQLITE_IOERR_ACCESS;
    *out = 0;
    char host[SSRNX_VFS_PATH_CAP];
    sqlite_host_ssrnx_vfs_host_path(name, host, sizeof(host));

    struct stat st;
    bool exists = stat(host, &st) == 0;
    if (!exists && errno == EIO && ssrnx_file_contract_fd_for_host_path) {
        const int fd = ssrnx_file_contract_fd_for_host_path(host);
        if (fd >= 0) exists = fstat(fd, &st) == 0;
    }
    if (!exists) {
        if (errno == ENOENT || errno == ENOTDIR || errno == EACCES || errno == EPERM || errno == EROFS || errno == EIO)
            return SQLITE_OK;
        return SQLITE_IOERR_ACCESS;
    }

    if (flags == SQLITE_ACCESS_EXISTS) {
        *out = 1;
        return SQLITE_OK;
    }
    if (flags == SQLITE_ACCESS_READ) {
        *out = access(host, R_OK) == 0 ? 1 : 0;
        return SQLITE_OK;
    }
    if (flags == SQLITE_ACCESS_READWRITE) {
        *out = access(host, R_OK | W_OK) == 0 ? 1 : 0;
        return SQLITE_OK;
    }
    return SQLITE_IOERR_ACCESS;
}

static int sqlite_host_ssrnx_vfs_full_pathname(sqlite3_vfs *vfs, const char *name, int n, char *out) {
    (void)vfs;
    return snprintf(out, (size_t)n, "%s", name) < n ? SQLITE_OK : SQLITE_CANTOPEN;
}

static int sqlite_host_ssrnx_vfs_randomness(sqlite3_vfs *vfs, int n, char *out) {
    (void)vfs;
    if (ssrnx_fd_random_fill) {
        ssrnx_fd_random_fill(out, (size_t)n);
    } else {
        for (int i = 0; i < n; ++i) out[i] = (char)rand();
    }
    return n;
}

static int sqlite_host_ssrnx_vfs_sleep(sqlite3_vfs *vfs, int micros) {
    (void)vfs;
    struct timespec ts = {micros / 1000000, (long)(micros % 1000000) * 1000L};
    nanosleep(&ts, NULL);
    return micros;
}

/* Julian day number in milliseconds (SQLite's xCurrentTimeInt64 contract). */
static int sqlite_host_ssrnx_vfs_current_time64(sqlite3_vfs *vfs, sqlite3_int64 *out) {
    (void)vfs;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    *out = (sqlite3_int64)210866760000000LL + (sqlite3_int64)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return SQLITE_OK;
}

static int sqlite_host_ssrnx_vfs_current_time(sqlite3_vfs *vfs, double *out) {
    sqlite3_int64 ms = 0;
    sqlite_host_ssrnx_vfs_current_time64(vfs, &ms);
    *out = (double)ms / 86400000.0;
    return SQLITE_OK;
}

static int sqlite_host_ssrnx_vfs_last_error(sqlite3_vfs *vfs, int n, char *out) {
    (void)vfs;
    if (n > 0) snprintf(out, (size_t)n, "%s", strerror(errno));
    return errno;
}

static sqlite3_vfs sqlite_host_g_ssrnx_vfs = {
    2, (int)sizeof(sqlite_host_SsrnxVfsFile), SSRNX_VFS_PATH_CAP, NULL, "apn64", NULL,
    sqlite_host_ssrnx_vfs_open, sqlite_host_ssrnx_vfs_delete, sqlite_host_ssrnx_vfs_access, sqlite_host_ssrnx_vfs_full_pathname,
    NULL, NULL, NULL, NULL,
    sqlite_host_ssrnx_vfs_randomness, sqlite_host_ssrnx_vfs_sleep, sqlite_host_ssrnx_vfs_current_time, sqlite_host_ssrnx_vfs_last_error,
    sqlite_host_ssrnx_vfs_current_time64, NULL, NULL, NULL,
};

/* ---- SQLITE_CONFIG_MUTEX over pthreads ---- */

typedef struct sqlite_host_SsrnxSqliteMutex {
    pthread_mutex_t mutex;
    int id;
} sqlite_host_SsrnxSqliteMutex;

static sqlite_host_SsrnxSqliteMutex sqlite_host_g_sqlite_static_mutexes[12];
static pthread_once_t sqlite_host_g_sqlite_static_once = PTHREAD_ONCE_INIT;

static void sqlite_host_ssrnx_sqlite_static_mutex_init(void) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    for (size_t i = 0; i < sizeof(sqlite_host_g_sqlite_static_mutexes) / sizeof(sqlite_host_g_sqlite_static_mutexes[0]); ++i) {
        pthread_mutex_init(&sqlite_host_g_sqlite_static_mutexes[i].mutex, &attr);
        sqlite_host_g_sqlite_static_mutexes[i].id = (int)i + 2;
    }
    pthread_mutexattr_destroy(&attr);
}

static int sqlite_host_ssrnx_sqlite_mutex_init(void) {
    pthread_once(&sqlite_host_g_sqlite_static_once, sqlite_host_ssrnx_sqlite_static_mutex_init);
    return SQLITE_OK;
}

static int sqlite_host_ssrnx_sqlite_mutex_end(void) {
    return SQLITE_OK;
}

static sqlite3_mutex *sqlite_host_ssrnx_sqlite_mutex_alloc(int id) {
    if (id == SQLITE_MUTEX_FAST || id == SQLITE_MUTEX_RECURSIVE) {
        sqlite_host_SsrnxSqliteMutex *m = malloc(sizeof(*m)); /* not sqlite3_malloc: runs inside sqlite3_initialize */
        if (!m) return NULL;
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, id == SQLITE_MUTEX_RECURSIVE ? PTHREAD_MUTEX_RECURSIVE
                                                                       : PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&m->mutex, &attr);
        pthread_mutexattr_destroy(&attr);
        m->id = id;
        return (sqlite3_mutex *)m;
    }
    const int index = id - 2;
    if (index < 0 || index >= (int)(sizeof(sqlite_host_g_sqlite_static_mutexes) / sizeof(sqlite_host_g_sqlite_static_mutexes[0])))
        return NULL;
    pthread_once(&sqlite_host_g_sqlite_static_once, sqlite_host_ssrnx_sqlite_static_mutex_init);
    return (sqlite3_mutex *)&sqlite_host_g_sqlite_static_mutexes[index];
}

static void sqlite_host_ssrnx_sqlite_mutex_free(sqlite3_mutex *p) {
    sqlite_host_SsrnxSqliteMutex *m = (sqlite_host_SsrnxSqliteMutex *)p;
    if (m->id != SQLITE_MUTEX_FAST && m->id != SQLITE_MUTEX_RECURSIVE) return;
    pthread_mutex_destroy(&m->mutex);
    free(m);
}

static void sqlite_host_ssrnx_sqlite_mutex_enter(sqlite3_mutex *p) {
    pthread_mutex_lock(&((sqlite_host_SsrnxSqliteMutex *)p)->mutex);
}

static int sqlite_host_ssrnx_sqlite_mutex_try(sqlite3_mutex *p) {
    return pthread_mutex_trylock(&((sqlite_host_SsrnxSqliteMutex *)p)->mutex) == 0 ? SQLITE_OK : SQLITE_BUSY;
}

static void sqlite_host_ssrnx_sqlite_mutex_leave(sqlite3_mutex *p) {
    pthread_mutex_unlock(&((sqlite_host_SsrnxSqliteMutex *)p)->mutex);
}

static const sqlite3_mutex_methods sqlite_host_k_ssrnx_sqlite_mutex = {
    sqlite_host_ssrnx_sqlite_mutex_init, sqlite_host_ssrnx_sqlite_mutex_end, sqlite_host_ssrnx_sqlite_mutex_alloc, sqlite_host_ssrnx_sqlite_mutex_free,
    sqlite_host_ssrnx_sqlite_mutex_enter, sqlite_host_ssrnx_sqlite_mutex_try, sqlite_host_ssrnx_sqlite_mutex_leave, NULL, NULL,
};

static pthread_once_t sqlite_host_g_sqlite_init_once = PTHREAD_ONCE_INIT;
static int sqlite_host_g_sqlite_init_rc = SQLITE_ERROR;

static void sqlite_host_ssrnx_sqlite_initialize_once(void) {
    (void)sqlite3_config(SQLITE_CONFIG_MUTEX, &sqlite_host_k_ssrnx_sqlite_mutex);
    (void)sqlite3_config(SQLITE_CONFIG_SERIALIZED);
    sqlite_host_g_sqlite_init_rc = sqlite3_initialize();
}

int ssrnx_sqlite_initialize(void) {
    pthread_once(&sqlite_host_g_sqlite_init_once, sqlite_host_ssrnx_sqlite_initialize_once);
    return sqlite_host_g_sqlite_init_rc;
}

int sqlite3_os_init(void) {
    return sqlite3_vfs_register(&sqlite_host_g_ssrnx_vfs, 1);
}

int sqlite3_os_end(void) {
    return SQLITE_OK;
}

/* ---- com.apn64.core.SQLite boot natives (android.database.sqlite) ----
 * Handles are sqlite3* / sqlite3_stmt* pointers carried as Java longs. Errors
 * throw the android.database.sqlite exception matching the primary result
 * code, as the platform's android_database_SQLiteCommon does. */

#ifndef SSRNX_SQLITE_VFS_TEST


static const char *sqlite_host_ssrnx_sqlite_exception_class(int code) {
    switch (code & 0xff) {
        case SQLITE_IOERR: return "android/database/sqlite/SQLiteDiskIOException";
        case SQLITE_CORRUPT:
        case SQLITE_NOTADB: return "android/database/sqlite/SQLiteDatabaseCorruptException";
        case SQLITE_CONSTRAINT: return "android/database/sqlite/SQLiteConstraintException";
        case SQLITE_ABORT: return "android/database/sqlite/SQLiteAbortException";
        case SQLITE_DONE: return "android/database/sqlite/SQLiteDoneException";
        case SQLITE_FULL: return "android/database/sqlite/SQLiteFullException";
        case SQLITE_MISUSE: return "android/database/sqlite/SQLiteMisuseException";
        case SQLITE_PERM: return "android/database/sqlite/SQLiteAccessPermException";
        case SQLITE_BUSY:
        case SQLITE_LOCKED: return "android/database/sqlite/SQLiteDatabaseLockedException";
        case SQLITE_READONLY: return "android/database/sqlite/SQLiteReadOnlyDatabaseException";
        case SQLITE_CANTOPEN: return "android/database/sqlite/SQLiteCantOpenDatabaseException";
        case SQLITE_TOOBIG: return "android/database/sqlite/SQLiteBlobTooBigException";
        case SQLITE_RANGE: return "android/database/sqlite/SQLiteBindOrColumnIndexOutOfRangeException";
        case SQLITE_NOMEM: return "android/database/sqlite/SQLiteOutOfMemoryException";
        case SQLITE_MISMATCH: return "android/database/sqlite/SQLiteDatatypeMismatchException";
        case SQLITE_INTERRUPT: return "android/os/OperationCanceledException";
        default: return "android/database/sqlite/SQLiteException";
    }
}

static void sqlite_host_ssrnx_sqlite_throw(void *env, sqlite3 *db, int code, const char *context) {
    const int ext = db ? sqlite3_extended_errcode(db) : code;
    const char *msg = db ? sqlite3_errmsg(db) : sqlite3_errstr(code);
    char text[768];
    snprintf(text, sizeof(text), "%s (code %d)%s%s", msg ? msg : "unknown error", ext,
             context ? ": " : "", context ? context : "");
    (void)ssrnx_jni_throw_new_by_name(env, sqlite_host_ssrnx_sqlite_exception_class(code), text);
}

static sqlite3 *sqlite_host_sqlite_db(int64_t handle) { return (sqlite3 *)(uintptr_t)handle; }
static sqlite3_stmt *sqlite_host_sqlite_stmt(int64_t handle) { return (sqlite3_stmt *)(uintptr_t)handle; }

/* Android's connection defaults: extended result codes and a busy timeout
 * (the platform retries SQLITE_BUSY rather than failing immediately). */
static int64_t sqlite_host_sqlite_native_open(void *env, void *clazz, void *path_ref, int32_t flags) {
    (void)clazz;
    if (ssrnx_sqlite_initialize() != SQLITE_OK) {
        sqlite_host_ssrnx_sqlite_throw(env, NULL, SQLITE_ERROR, "sqlite3_initialize");
        return 0;
    }
    char *path = ssrnx_jni_string_utf_dup(path_ref);
    if (!path) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/NullPointerException", "path");
        return 0;
    }
    sqlite3 *db = NULL;
    const int rc = sqlite3_open_v2(path, &db, flags, "apn64");
    if (rc != SQLITE_OK) {
        sqlite_host_ssrnx_sqlite_throw(env, db, rc, path);
        sqlite3_close_v2(db);
        free(path);
        return 0;
    }
    free(path);
    sqlite3_extended_result_codes(db, 1);
    sqlite3_busy_timeout(db, 2500);
    return (int64_t)(uintptr_t)db;
}

static void sqlite_host_sqlite_native_close(void *env, void *clazz, int64_t db) {
    (void)env; (void)clazz;
    if (db) sqlite3_close_v2(sqlite_host_sqlite_db(db));
}

static int64_t sqlite_host_sqlite_native_prepare(void *env, void *clazz, int64_t db, void *sql_ref) {
    (void)clazz;
    int32_t len = 0;
    uint16_t *sql = ssrnx_jni_string_utf16_dup_host(sql_ref, &len);
    if (!sql && len == 0 && ssrnx_jni_string_length_host(sql_ref) == 0) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/IllegalArgumentException", "empty SQL statement");
        return 0;
    }
    if (!sql) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/NullPointerException", "sql");
        return 0;
    }
    sqlite3_stmt *stmt = NULL;
    const int rc = sqlite3_prepare16_v2(sqlite_host_sqlite_db(db), sql, len * 2, &stmt, NULL);
    free(sql);
    if (rc != SQLITE_OK) {
        char *utf8 = ssrnx_jni_string_utf_dup(sql_ref);
        sqlite_host_ssrnx_sqlite_throw(env, sqlite_host_sqlite_db(db), rc, utf8);
        free(utf8);
        return 0;
    }
    if (!stmt) {
        /* Only whitespace or comments. */
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/IllegalArgumentException", "empty SQL statement");
        return 0;
    }
    return (int64_t)(uintptr_t)stmt;
}

static void sqlite_host_sqlite_native_finalize(void *env, void *clazz, int64_t stmt) {
    (void)env; (void)clazz;
    if (stmt) sqlite3_finalize(sqlite_host_sqlite_stmt(stmt));
}

static void sqlite_host_sqlite_check_bind(void *env, int64_t stmt, int rc) {
    if (rc != SQLITE_OK) sqlite_host_ssrnx_sqlite_throw(env, sqlite3_db_handle(sqlite_host_sqlite_stmt(stmt)), rc, "bind");
}

static void sqlite_host_sqlite_native_bind_null(void *env, void *clazz, int64_t stmt, int32_t index) {
    (void)clazz;
    sqlite_host_sqlite_check_bind(env, stmt, sqlite3_bind_null(sqlite_host_sqlite_stmt(stmt), index));
}

static void sqlite_host_sqlite_native_bind_long(void *env, void *clazz, int64_t stmt, int32_t index, int64_t value) {
    (void)clazz;
    sqlite_host_sqlite_check_bind(env, stmt, sqlite3_bind_int64(sqlite_host_sqlite_stmt(stmt), index, value));
}

static void sqlite_host_sqlite_native_bind_double_bits(void *env, void *clazz, int64_t stmt, int32_t index, int64_t bits) {
    (void)clazz;
    double value;
    memcpy(&value, &bits, sizeof(value));
    sqlite_host_sqlite_check_bind(env, stmt, sqlite3_bind_double(sqlite_host_sqlite_stmt(stmt), index, value));
}

static void sqlite_host_sqlite_native_bind_string(void *env, void *clazz, int64_t stmt, int32_t index, void *value) {
    (void)clazz;
    static const uint16_t k_empty = 0;
    int32_t len = 0;
    uint16_t *chars = ssrnx_jni_string_utf16_dup_host(value, &len);
    if (!chars && ssrnx_jni_string_length_host(value) != 0) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/IllegalArgumentException", "bind value");
        return;
    }
    sqlite_host_sqlite_check_bind(env, stmt, sqlite3_bind_text16(sqlite_host_sqlite_stmt(stmt), index, chars ? chars : &k_empty,
                                                     len * 2, SQLITE_TRANSIENT));
    free(chars);
}

static void sqlite_host_sqlite_native_bind_blob(void *env, void *clazz, int64_t stmt, int32_t index, void *value) {
    (void)clazz;
    const int32_t len = ssrnx_jni_byte_array_length(value);
    if (len < 0) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/IllegalArgumentException", "bind value");
        return;
    }
    int8_t *bytes = malloc(len ? (size_t)len : 1u);
    if (!bytes || (len > 0 && ssrnx_jni_byte_array_read(value, 0, len, bytes) != SSRNX_OK)) {
        free(bytes);
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/OutOfMemoryError", "blob");
        return;
    }
    sqlite_host_sqlite_check_bind(env, stmt, sqlite3_bind_blob(sqlite_host_sqlite_stmt(stmt), index, bytes, len, SQLITE_TRANSIENT));
    free(bytes);
}

static void sqlite_host_sqlite_native_reset(void *env, void *clazz, int64_t stmt, uint8_t clear_bindings) {
    (void)env; (void)clazz;
    sqlite3_reset(sqlite_host_sqlite_stmt(stmt));
    if (clear_bindings) sqlite3_clear_bindings(sqlite_host_sqlite_stmt(stmt));
}

/* 1 = row available, 0 = done; errors throw. */
static int32_t sqlite_host_sqlite_native_step(void *env, void *clazz, int64_t stmt) {
    SSRNX_IO_TIMING();
    (void)clazz;
    const int before_steps=sqlite3_stmt_status(sqlite_host_sqlite_stmt(stmt),SQLITE_STMTSTATUS_VM_STEP,0);
    const int rc = sqlite3_step(sqlite_host_sqlite_stmt(stmt));
    const uint64_t now=ssrnx_io_time_us();
    const uint64_t elapsed=now>=ssrnx_io_scope_.begin ? now-ssrnx_io_scope_.begin : 0;
    static unsigned reports;
    if (elapsed>=50000 && ssrnx_log_category && __atomic_fetch_add(&reports,1u,__ATOMIC_RELAXED)<32u) {
        const int saved=errno;
        ssrnx_log_category("SQLITE","INFO","slow step elapsedUs=%llu result=%d readonly=%d vmSteps=%u fullScanSteps=%d sorts=%d statementBytes=%d",
            (unsigned long long)elapsed,rc,sqlite3_stmt_readonly(sqlite_host_sqlite_stmt(stmt)),
            (unsigned)sqlite3_stmt_status(sqlite_host_sqlite_stmt(stmt),SQLITE_STMTSTATUS_VM_STEP,0)-(unsigned)before_steps,
            sqlite3_stmt_status(sqlite_host_sqlite_stmt(stmt),SQLITE_STMTSTATUS_FULLSCAN_STEP,0),
            sqlite3_stmt_status(sqlite_host_sqlite_stmt(stmt),SQLITE_STMTSTATUS_SORT,0),
            sqlite3_stmt_status(sqlite_host_sqlite_stmt(stmt),SQLITE_STMTSTATUS_MEMUSED,0));
        errno=saved;
    }
    if (rc == SQLITE_ROW) return 1;
    if (rc == SQLITE_DONE) return 0;
    sqlite3 *db = sqlite3_db_handle(sqlite_host_sqlite_stmt(stmt));
    char *sql = sqlite3_expanded_sql(sqlite_host_sqlite_stmt(stmt));
    sqlite_host_ssrnx_sqlite_throw(env, db, rc, sql);
    sqlite3_free(sql);
    sqlite3_reset(sqlite_host_sqlite_stmt(stmt));
    return -1;
}

static int32_t sqlite_host_sqlite_native_column_count(void *env, void *clazz, int64_t stmt) {
    (void)env; (void)clazz;
    return sqlite3_column_count(sqlite_host_sqlite_stmt(stmt));
}

static int32_t sqlite_host_sqlite_native_bind_parameter_count(void *env, void *clazz, int64_t stmt) {
    (void)env; (void)clazz;
    return sqlite3_bind_parameter_count(sqlite_host_sqlite_stmt(stmt));
}

static uint8_t sqlite_host_sqlite_native_is_read_only(void *env, void *clazz, int64_t stmt) {
    (void)env; (void)clazz;
    return sqlite3_stmt_readonly(sqlite_host_sqlite_stmt(stmt)) ? 1 : 0;
}

static void *sqlite_host_sqlite_utf16_string(void *env, const void *text, int bytes) {
    static const uint16_t k_empty = 0;
    return ssrnx_jni_new_local_string_utf16(env, text ? (const uint16_t *)text : &k_empty, bytes / 2);
}

static void *sqlite_host_sqlite_native_column_name(void *env, void *clazz, int64_t stmt, int32_t index) {
    (void)clazz;
    const uint16_t *name = sqlite3_column_name16(sqlite_host_sqlite_stmt(stmt), index);
    if (!name) return NULL;
    int units = 0;
    while (name[units]) units++;
    return sqlite_host_sqlite_utf16_string(env, name, units * 2);
}

/* Cursor.FIELD_TYPE_*: NULL 0, INTEGER 1, FLOAT 2, STRING 3, BLOB 4. */
static int32_t sqlite_host_sqlite_native_column_type(void *env, void *clazz, int64_t stmt, int32_t index) {
    (void)env; (void)clazz;
    switch (sqlite3_column_type(sqlite_host_sqlite_stmt(stmt), index)) {
        case SQLITE_INTEGER: return 1;
        case SQLITE_FLOAT: return 2;
        case SQLITE_TEXT: return 3;
        case SQLITE_BLOB: return 4;
        default: return 0;
    }
}

static int64_t sqlite_host_sqlite_native_column_long(void *env, void *clazz, int64_t stmt, int32_t index) {
    (void)env; (void)clazz;
    return sqlite3_column_int64(sqlite_host_sqlite_stmt(stmt), index);
}

static int64_t sqlite_host_sqlite_native_column_double_bits(void *env, void *clazz, int64_t stmt, int32_t index) {
    (void)env; (void)clazz;
    const double value = sqlite3_column_double(sqlite_host_sqlite_stmt(stmt), index);
    int64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static void *sqlite_host_sqlite_native_column_string(void *env, void *clazz, int64_t stmt, int32_t index) {
    (void)clazz;
    if (sqlite3_column_type(sqlite_host_sqlite_stmt(stmt), index) == SQLITE_NULL) return NULL;
    const void *text = sqlite3_column_text16(sqlite_host_sqlite_stmt(stmt), index);
    return sqlite_host_sqlite_utf16_string(env, text, sqlite3_column_bytes16(sqlite_host_sqlite_stmt(stmt), index));
}

static void *sqlite_host_sqlite_native_column_blob(void *env, void *clazz, int64_t stmt, int32_t index) {
    (void)clazz;
    if (sqlite3_column_type(sqlite_host_sqlite_stmt(stmt), index) == SQLITE_NULL) return NULL;
    const void *data = sqlite3_column_blob(sqlite_host_sqlite_stmt(stmt), index);
    const int len = sqlite3_column_bytes(sqlite_host_sqlite_stmt(stmt), index);
    void *array = ssrnx_jni_new_local_byte_array(env, len);
    if (array && len > 0) (void)ssrnx_jni_byte_array_write(array, 0, len, (const int8_t *)data);
    return array;
}

static int32_t sqlite_host_sqlite_native_changes(void *env, void *clazz, int64_t db) {
    (void)env; (void)clazz;
    return sqlite3_changes(sqlite_host_sqlite_db(db));
}

static int64_t sqlite_host_sqlite_native_last_insert_rowid(void *env, void *clazz, int64_t db) {
    (void)env; (void)clazz;
    return sqlite3_last_insert_rowid(sqlite_host_sqlite_db(db));
}

static uint8_t sqlite_host_sqlite_native_in_transaction(void *env, void *clazz, int64_t db) {
    (void)env; (void)clazz;
    return sqlite3_get_autocommit(sqlite_host_sqlite_db(db)) ? 0 : 1;
}

static void sqlite_host_sqlite_native_interrupt(void *env, void *clazz, int64_t db) {
    (void)env; (void)clazz;
    sqlite3_interrupt(sqlite_host_sqlite_db(db));
}

typedef struct sqlite_host_SqliteNative {
    const char *name;
    const char *signature;
    void *fn;
} sqlite_host_SqliteNative;

#define SQLITE_NATIVE_FN(f) ((void *)(uintptr_t)(f))

static const sqlite_host_SqliteNative sqlite_host_k_sqlite_natives[] = {
    {"open", "(Ljava/lang/String;I)J", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_open)},
    {"close", "(J)V", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_close)},
    {"prepare", "(JLjava/lang/String;)J", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_prepare)},
    {"finalizeStatement", "(J)V", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_finalize)},
    {"bindNull", "(JI)V", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_bind_null)},
    {"bindLong", "(JIJ)V", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_bind_long)},
    {"bindDoubleBits", "(JIJ)V", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_bind_double_bits)},
    {"bindString", "(JILjava/lang/String;)V", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_bind_string)},
    {"bindBlob", "(JI[B)V", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_bind_blob)},
    {"reset", "(JZ)V", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_reset)},
    {"step", "(J)I", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_step)},
    {"columnCount", "(J)I", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_column_count)},
    {"bindParameterCount", "(J)I", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_bind_parameter_count)},
    {"isReadOnly", "(J)Z", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_is_read_only)},
    {"columnName", "(JI)Ljava/lang/String;", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_column_name)},
    {"columnType", "(JI)I", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_column_type)},
    {"columnLong", "(JI)J", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_column_long)},
    {"columnDoubleBits", "(JI)J", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_column_double_bits)},
    {"columnString", "(JI)Ljava/lang/String;", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_column_string)},
    {"columnBlob", "(JI)[B", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_column_blob)},
    {"changes", "(J)I", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_changes)},
    {"lastInsertRowid", "(J)J", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_last_insert_rowid)},
    {"inTransaction", "(J)Z", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_in_transaction)},
    {"interrupt", "(J)V", SQLITE_NATIVE_FN(sqlite_host_sqlite_native_interrupt)},
};

/* com/apn64/core/SQLite natives; consulted by ssrnx_core_native_lookup(). */
bool ssrnx_sqlite_native_lookup(const char *name, const char *signature, void **out_fn) {
    for (size_t i = 0; i < sizeof(sqlite_host_k_sqlite_natives) / sizeof(sqlite_host_k_sqlite_natives[0]); ++i) {
        if (strcmp(sqlite_host_k_sqlite_natives[i].name, name) == 0 &&
            strcmp(sqlite_host_k_sqlite_natives[i].signature, signature) == 0) {
            *out_fn = sqlite_host_k_sqlite_natives[i].fn;
            return true;
        }
    }
    return false;
}

#endif /* SSRNX_SQLITE_VFS_TEST */

#pragma pop_macro("SQLITE_THREADSAFE")
#pragma pop_macro("SQLITE_TEMP_STORE")
#pragma pop_macro("SQLITE_OS_OTHER")
#pragma pop_macro("SQLITE_OMIT_SHARED_CACHE")
#pragma pop_macro("SQLITE_OMIT_LOAD_EXTENSION")
#pragma pop_macro("SQLITE_NATIVE_FN")
#pragma pop_macro("SQLITE_ENABLE_RTREE")
#pragma pop_macro("SQLITE_ENABLE_FTS4")
#pragma pop_macro("SQLITE_ENABLE_FTS3_PARENTHESIS")
#pragma pop_macro("SQLITE_ENABLE_FTS3")
#pragma pop_macro("SQLITE_DEFAULT_MEMSTATUS")
#pragma pop_macro("SQLITE_DEFAULT_FILE_PERMISSIONS")
#pragma pop_macro("SSRNX_VFS_PATH_CAP")
#pragma pop_macro("SSRNX_VFS_JOURNAL_BUFFER_SIZE")
