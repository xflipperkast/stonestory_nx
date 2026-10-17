/* runtime storage. */
#define _GNU_SOURCE 1
#include "runtime.h"

/* ---- storage policy ---- */

#include <string.h>

bool nx_storage_path_allowed(const char *path, bool write) {
    if (!path || !*path || strlen(path) >= 1024 || strchr(path, '\\')) return false;
    const char *p = path;
    if (!strncmp(p, "sdmc:", 5)) p += 5;
    if (*p != '/' || strchr(p, ':')) return false;
    char normalized[1024] = "/";
    size_t n = 1;
    while (*p) {
        while (*p == '/') ++p;
        const char *start = p;
        while (*p && *p != '/') ++p;
        size_t len = (size_t)(p - start);
        if (!len || (len == 1 && *start == '.')) continue;
        if (len == 2 && start[0] == '.' && start[1] == '.') {
            while (n > 1 && normalized[n - 1] != '/') --n;
            if (n > 1) --n;
            normalized[n] = 0;
            continue;
        }
        /* FAT ignores trailing dots/spaces. Reject aliases at the boundary. */
        if (start[len - 1] == '.' || start[len - 1] == ' ') return false;
        if (n > 1) normalized[n++] = '/';
        memcpy(normalized + n, start, len);
        n += len;
        normalized[n] = 0;
    }
    static const char root[] = "/switch/stonestory_nx";
    size_t rn = sizeof(root) - 1;
    if (!strncmp(normalized, root, rn) && (!normalized[rn] || normalized[rn] == '/')) return true;
    /* Recursive mkdir helpers stat the existing ancestors. They cannot alter them. */
    return !write && (!strcmp(normalized, "/") || !strcmp(normalized, "/switch"));
}



/* ---- storage switch ---- */
#pragma push_macro("ONE")

#ifdef NX_STORAGE_HOST_TEST
#include "../tests/fsdev_mock.h"
#else
#include <switch.h>
#include <sys/iosupport.h>
#endif
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/* Install before any runtime threads or guest descriptors exist. Preserve
 * fsdev's data and descriptor ABI; gate every path operation on the SD mount. */
static devoptab_t original, storage_switch_guarded;
static int storage_switch_permit(struct _reent *r, const char *path, bool write) {
    if (nx_storage_path_allowed(path, write)) return 1;
    r->_errno = EACCES;
    return 0;
}
static int storage_switch_guarded_open(struct _reent *r, void *fd, const char *p, int flags, int mode) {
    /* Reads are isolated too, so reopening an fd cannot grant access elsewhere. */
    return storage_switch_permit(r, p, false) && storage_switch_permit(r, p, true) ? original.open_r(r, fd, p, flags, mode) : -1;
}
#define ONE(name, member, write) \
static int name(struct _reent *r, const char *p) { \
    return storage_switch_permit(r, p, write) ? original.member(r, p) : -1; \
}
ONE(guarded_unlink, unlink_r, true)
ONE(guarded_chdir, chdir_r, true)
ONE(guarded_rmdir, rmdir_r, true)
static int storage_switch_guarded_stat(struct _reent *r, const char *p, struct stat *s) {
    return storage_switch_permit(r, p, false) ? original.stat_r(r, p, s) : -1;
}
static int storage_switch_guarded_lstat(struct _reent *r, const char *p, struct stat *s) {
    return storage_switch_permit(r, p, false) ? original.lstat_r(r, p, s) : -1;
}
static int storage_switch_guarded_mkdir(struct _reent *r, const char *p, int mode) {
    if (nx_storage_path_allowed(p, true)) return original.mkdir_r(r, p, mode);
    if (nx_storage_path_allowed(p, false)) { r->_errno = EEXIST; return -1; }
    r->_errno = EACCES; return -1;
}
static int storage_switch_guarded_rename(struct _reent *r, const char *a, const char *b) {
    return storage_switch_permit(r, a, true) && storage_switch_permit(r, b, true) ? original.rename_r(r, a, b) : -1;
}
static int storage_switch_guarded_link(struct _reent *r, const char *a, const char *b) {
    return storage_switch_permit(r, a, true) && storage_switch_permit(r, b, true) ? original.link_r(r, a, b) : -1;
}
static int storage_switch_guarded_chmod(struct _reent *r, const char *p, mode_t mode) {
    return storage_switch_permit(r, p, true) ? original.chmod_r(r, p, mode) : -1;
}
static int storage_switch_guarded_utimes(struct _reent *r, const char *p, const struct timeval times[2]) {
    return storage_switch_permit(r, p, true) ? original.utimes_r(r, p, times) : -1;
}
static DIR_ITER *storage_switch_guarded_diropen(struct _reent *r, DIR_ITER *d, const char *p) {
    return storage_switch_permit(r, p, true) ? original.diropen_r(r, d, p) : NULL;
}
static int storage_switch_guarded_statvfs(struct _reent *r, const char *p, struct statvfs *s) {
    return storage_switch_permit(r, p, false) ? original.statvfs_r(r, p, s) : -1;
}
static long storage_switch_guarded_pathconf(struct _reent *r, const char *p, int name) {
    return storage_switch_permit(r, p, false) ? original.pathconf_r(r, p, name) : -1;
}

bool nx_storage_init(void) {
    const char *dirs[] = {NX_ROOT, NX_SAVES, NX_ROOT "/game_data",
        NX_ROOT "/config", NX_ROOT "/cache", NX_ROOT "/cache/runtime",
        NX_RUNTIME_APPS, NX_ROOT "/logs"};
    /* libnx mounts SD and initializes Horizon services before main. */
    for (size_t i = 0; i < sizeof(dirs) / sizeof(dirs[0]); ++i) {
        struct stat s;
        if (mkdir(dirs[i], 0777) != 0 &&
            (errno != EEXIST || stat(dirs[i], &s) != 0 || !S_ISDIR(s.st_mode))) return false;
    }
    int device = FindDevice("sdmc:");
    if (device < 0 || !devoptab_list[device]) return false;
    original = *devoptab_list[device];
    storage_switch_guarded = original;
    storage_switch_guarded.open_r = storage_switch_guarded_open;
    storage_switch_guarded.stat_r = storage_switch_guarded_stat;
    if (original.lstat_r) storage_switch_guarded.lstat_r = storage_switch_guarded_lstat;
    storage_switch_guarded.mkdir_r = storage_switch_guarded_mkdir;
    storage_switch_guarded.rename_r = storage_switch_guarded_rename;
    storage_switch_guarded.unlink_r = guarded_unlink;
    storage_switch_guarded.chdir_r = guarded_chdir;
    storage_switch_guarded.rmdir_r = guarded_rmdir;
    if (original.link_r) storage_switch_guarded.link_r = storage_switch_guarded_link;
    if (original.chmod_r) storage_switch_guarded.chmod_r = storage_switch_guarded_chmod;
    if (original.utimes_r) storage_switch_guarded.utimes_r = storage_switch_guarded_utimes;
    if (original.diropen_r) storage_switch_guarded.diropen_r = storage_switch_guarded_diropen;
    if (original.statvfs_r) storage_switch_guarded.statvfs_r = storage_switch_guarded_statvfs;
    if (original.pathconf_r) storage_switch_guarded.pathconf_r = storage_switch_guarded_pathconf;
    /* fsdev has no symlinks. Do not admit future link-based escapes. */
    storage_switch_guarded.symlink_r = NULL;
    storage_switch_guarded.readlink_r = NULL;
    devoptab_list[device] = &storage_switch_guarded;
    return chdir(NX_ROOT) == 0;
}

#pragma pop_macro("ONE")




/* ---- directory_fd ---- */
#ifndef NX_STORAGE_HOST_TEST



#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __SWITCH__
#include <switch/runtime/devices/fs_dev.h>
#include <sys/iosupport.h>

/* fsdev exposes FsFile descriptors and separate directory iterators. A devoptab
 * bridge gives the latter a real low-numbered host fd, not a pointer cast. */
typedef struct directory_fd_SsrnxDirectoryFd {
    DIR *directory;
    struct stat metadata;
    char path[1024];
} directory_fd_SsrnxDirectoryFd;

static int directory_fd_directory_error(struct _reent *r, int error) { r->_errno = error; return -1; }

static int directory_fd_directory_open(struct _reent *r, void *data, const char *path, int flags, int mode) {
    (void)mode;
    directory_fd_SsrnxDirectoryFd *entry = data;
    if (!strncmp(path, "apn64dir:", 9)) path += 9;
    if ((flags & O_ACCMODE) != O_RDONLY || (flags & (O_CREAT | O_TRUNC)))
        return directory_fd_directory_error(r, EISDIR);
    if (strlen(path) >= sizeof(entry->path)) return directory_fd_directory_error(r, ENAMETOOLONG);
    if (stat(path, &entry->metadata) != 0) return directory_fd_directory_error(r, errno);
    if (!S_ISDIR(entry->metadata.st_mode)) return directory_fd_directory_error(r, ENOTDIR);
    entry->directory = opendir(path);
    if (!entry->directory) return directory_fd_directory_error(r, errno);
    strcpy(entry->path, path);
    return 0;
}

static int directory_fd_directory_close(struct _reent *r, void *data) {
    directory_fd_SsrnxDirectoryFd *entry = data;
    if (!entry->directory) return directory_fd_directory_error(r, EBADF);
    const int rc = closedir(entry->directory);
    entry->directory = NULL;
    return rc == 0 ? 0 : directory_fd_directory_error(r, errno);
}

static ssize_t directory_fd_directory_read(struct _reent *r, void *data, char *buffer, size_t size) {
    (void)data; (void)buffer; (void)size;
    return directory_fd_directory_error(r, EISDIR);
}

static ssize_t directory_fd_directory_write(struct _reent *r, void *data, const char *buffer, size_t size) {
    (void)data; (void)buffer; (void)size;
    return directory_fd_directory_error(r, EBADF);
}

static off_t directory_fd_directory_seek(struct _reent *r, void *data, off_t offset, int whence) {
    directory_fd_SsrnxDirectoryFd *entry = data;
    if (!entry->directory) return directory_fd_directory_error(r, EBADF);
    if (offset != 0 || whence != SEEK_SET) return directory_fd_directory_error(r, EINVAL);
    rewinddir(entry->directory);
    return 0;
}

static int directory_fd_directory_stat(struct _reent *r, void *data, struct stat *metadata) {
    const directory_fd_SsrnxDirectoryFd *entry = data;
    if (!entry->directory) return directory_fd_directory_error(r, EBADF);
    /* Retain the opened object's identity after rename/unlink. Live directory
     * timestamps and getdents cookies need a richer directory backend. */
    *metadata = entry->metadata;
    return 0;
}

static int directory_fd_directory_sync(struct _reent *r, void *data) {
    const directory_fd_SsrnxDirectoryFd *entry = data;
    if (!entry->directory) return directory_fd_directory_error(r, EBADF);
    const char *colon = strchr(entry->path, ':');
    if (!colon || colon == entry->path || (size_t)(colon - entry->path) >= 32)
        return directory_fd_directory_error(r, EINVAL);
    char device[32];
    const size_t length = (size_t)(colon - entry->path);
    memcpy(device, entry->path, length); device[length] = 0;
    /* Same ordering barrier as SSRNX's SQLite VFS: commit the real filesystem,
     * never claim directory durability merely because a handle was opened. */
    const uint32_t result = fsdevCommitDevice(device);
    static unsigned reports, failures;
    if (__atomic_fetch_add(result ? &failures : &reports, 1u, __ATOMIC_RELAXED) < 8)
        SSRNX_LOGCI("FILE", "directory sync device=%s hostResult=%#x", device, (unsigned)result);
    return result == 0 ? 0 : directory_fd_directory_error(r, EIO);
}

static const devoptab_t directory_fd_g_directory_device = {
    .name = "apn64dir", .structSize = sizeof(directory_fd_SsrnxDirectoryFd),
    .open_r = directory_fd_directory_open, .close_r = directory_fd_directory_close,
    .read_r = directory_fd_directory_read, .write_r = directory_fd_directory_write,
    .seek_r = directory_fd_directory_seek, .fstat_r = directory_fd_directory_stat, .fsync_r = directory_fd_directory_sync,
};
static pthread_mutex_t directory_fd_g_directory_device_lock = PTHREAD_MUTEX_INITIALIZER;

int ssrnx_directory_open(const char *path, int flags) {
    const int saved = errno;
    struct stat metadata;
    if (stat(path, &metadata) != 0 || !S_ISDIR(metadata.st_mode)) { errno = saved; return -2; }
    if ((flags & O_ACCMODE) != O_RDONLY || (flags & (O_CREAT | O_TRUNC))) { errno = EISDIR; return -1; }
    pthread_mutex_lock(&directory_fd_g_directory_device_lock);
    int device = FindDevice("apn64dir:");
    if (device < 0) device = AddDevice(&directory_fd_g_directory_device);
    const int registration_error = errno;
    pthread_mutex_unlock(&directory_fd_g_directory_device_lock);
    if (device < 0) { errno = registration_error; return -1; }
    char target[sizeof(((directory_fd_SsrnxDirectoryFd *)0)->path) + 10];
    const int length = snprintf(target, sizeof(target), "apn64dir:%s", path);
    if (length < 0 || (size_t)length >= sizeof(target)) { errno = ENAMETOOLONG; return -1; }
    const int fd = open(target, flags);
    const int error = errno;
    static unsigned reports, failures;
    if (__atomic_fetch_add(fd < 0 ? &failures : &reports, 1u, __ATOMIC_RELAXED) < 16)
        SSRNX_LOGCI("FILE", "directory open host=%s flags=%#x result=%d errno=%d", path, flags, fd, fd < 0 ? error : 0);
    errno = error;
    return fd;
}
#else
int ssrnx_directory_open(const char *path, int flags) { (void)path; (void)flags; return -2; }
#endif



/* ---- path_lookup ---- */
#pragma push_macro("LOOKUP_MISS_CAP")
#pragma push_macro("LOOKUP_PATH_CAP")

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define LOOKUP_PATH_CAP 512u
#define LOOKUP_MISS_CAP 16u
static pthread_mutex_t path_lookup_lookup_lock = PTHREAD_MUTEX_INITIALIZER;
static char path_lookup_lookup_root[LOOKUP_PATH_CAP];
static char path_lookup_lookup_missing[LOOKUP_MISS_CAP][LOOKUP_PATH_CAP];
/* Positive parent hints only avoid repeating discovery I/O. They never answer
 * a guest query, so a deleted hinted directory can only lose this optimization. */
static char path_lookup_lookup_parent_hints[LOOKUP_MISS_CAP][LOOKUP_PATH_CAP];
static uint64_t path_lookup_lookup_epoch;
static size_t path_lookup_lookup_next;
static size_t path_lookup_lookup_hint_next;

static bool below(const char *path, const char *parent) {
    const size_t n = strlen(parent);
    return n && !strncmp(path, parent, n) && path[n] == '/';
}

static bool eligible(const char *path, const char *root) {
    if (!path || !below(path, root) || strlen(path) >= LOOKUP_PATH_CAP) return false;
    const char *part = path + strlen(root) + 1;
    /* These app-private namespaces are owned exclusively by this runtime.
     * Package extraction/system snapshots and shared mounts remain uncached. */
    if (strncmp(part, "files/", 6) && strncmp(part, "cache/", 6)) return false;
    /* External storage is shared, and noncanonical paths must still be walked
     * by the filesystem (missing/../file must not be lexically simplified). */
    if (!strncmp(part, "external", 8) && (!part[8] || part[8] == '/')) return false;
    for (; *part;) {
        const char *end = strchr(part, '/');
        size_t n = end ? (size_t)(end - part) : strlen(part);
        if (!n || (n == 1 && part[0] == '.') || (n == 2 && part[0] == '.' && part[1] == '.')) return false;
        if (!end) break;
        part = end + 1;
    }
    return true;
}

void ssrnx_path_lookup_set_root(const char *root) {
    const int saved = errno;
    pthread_mutex_lock(&path_lookup_lookup_lock);
    path_lookup_lookup_root[0] = 0;
    if (root && strlen(root) < sizeof(path_lookup_lookup_root)) {
        strcpy(path_lookup_lookup_root, root);
        size_t n = strlen(path_lookup_lookup_root);
        while (n && path_lookup_lookup_root[n-1] == '/') path_lookup_lookup_root[--n] = 0;
    }
    memset(path_lookup_lookup_missing, 0, sizeof(path_lookup_lookup_missing));
    memset(path_lookup_lookup_parent_hints, 0, sizeof(path_lookup_lookup_parent_hints));
    path_lookup_lookup_next = 0;
    path_lookup_lookup_hint_next = 0;
    ++path_lookup_lookup_epoch;
    pthread_mutex_unlock(&path_lookup_lookup_lock);
    errno = saved;
}

bool ssrnx_path_lookup_missing_parent(const char *path) {
    const int saved = errno;
    bool missing = false;
    pthread_mutex_lock(&path_lookup_lookup_lock);
    if (eligible(path, path_lookup_lookup_root))
        for (size_t i = 0; i < LOOKUP_MISS_CAP && !missing; ++i)
            missing = below(path, path_lookup_lookup_missing[i]);
    pthread_mutex_unlock(&path_lookup_lookup_lock);
    errno = missing ? ENOENT : saved;
    return missing;
}

void ssrnx_path_lookup_record_missing_parent(const char *path) {
    const int saved = errno;
    char root[LOOKUP_PATH_CAP], probe[LOOKUP_PATH_CAP];
    uint64_t epoch;
    pthread_mutex_lock(&path_lookup_lookup_lock);
    bool valid = eligible(path, path_lookup_lookup_root);
    strcpy(root, path_lookup_lookup_root);
    epoch = path_lookup_lookup_epoch;
    pthread_mutex_unlock(&path_lookup_lookup_lock);
    if (!valid) return;
    strcpy(probe, path);
    char *leaf = strrchr(probe, '/');
    if (!leaf || leaf <= probe + strlen(root)) return;
    *leaf = 0;
    pthread_mutex_lock(&path_lookup_lookup_lock);
    bool hinted = false;
    for (size_t i = 0; i < LOOKUP_MISS_CAP && !hinted; ++i)
        hinted = !strcmp(probe, path_lookup_lookup_parent_hints[i]);
    pthread_mutex_unlock(&path_lookup_lookup_lock);
    if (hinted) return;
    struct stat parent;
    if (lstat(probe, &parent) == 0) {
        pthread_mutex_lock(&path_lookup_lookup_lock);
        if (epoch == path_lookup_lookup_epoch)
            strcpy(path_lookup_lookup_parent_hints[path_lookup_lookup_hint_next++ % LOOKUP_MISS_CAP], probe);
        pthread_mutex_unlock(&path_lookup_lookup_lock);
        errno = saved;
        return;
    }
    if (errno != ENOENT) { errno = saved; return; }
    *leaf = '/';
    /* Prove the first missing ancestor, not the missing leaf. Never cache
     * ENOTDIR/EACCES/EIO, or paths through symlinks. No filesystem I/O under
     * the cache lock; a concurrent topology change invalidates this proof. */
    for (char *slash = strchr(probe + strlen(root) + 1, '/'); slash; slash = strchr(slash + 1, '/')) {
        *slash = 0;
        struct stat st;
        int rc = lstat(probe, &st), error = errno;
        if (rc != 0 && error == ENOENT) {
            pthread_mutex_lock(&path_lookup_lookup_lock);
            if (epoch == path_lookup_lookup_epoch) {
                strcpy(path_lookup_lookup_missing[path_lookup_lookup_next++ % LOOKUP_MISS_CAP], probe);
            }
            pthread_mutex_unlock(&path_lookup_lookup_lock);
            break;
        }
        *slash = '/';
        if (rc != 0 || !S_ISDIR(st.st_mode)) break;
    }
    errno = saved;
}

void ssrnx_path_lookup_changed(const char *path) {
    const int saved = errno;
    if (!path) return;
    pthread_mutex_lock(&path_lookup_lookup_lock);
    ++path_lookup_lookup_epoch;
    /* Topology changes are much rarer than probes. Clearing this small cache
     * also handles case aliases, dot components and renamed subtrees without
     * adding another pathname resolver. Ordinary file writes don't call here. */
    memset(path_lookup_lookup_missing, 0, sizeof(path_lookup_lookup_missing));
    pthread_mutex_unlock(&path_lookup_lookup_lock);
    errno = saved;
}

#pragma pop_macro("LOOKUP_PATH_CAP")
#pragma pop_macro("LOOKUP_MISS_CAP")


/* ---- proc_memory ---- */

#ifdef __SWITCH__
#include <switch.h>
#include <sys/iosupport.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

typedef struct proc_memory_ProcMemoryFile {
    char text[256];
    size_t length;
    off_t position;
} proc_memory_ProcMemoryFile;

/* Horizon mappings are resident, without Linux demand paging. Count mapped RAM
 * per virtual mapping (including aliases), excluding IO and reserved holes. */
static int proc_memory_proc_memory_capture(proc_memory_ProcMemoryFile *file) {
    u64 base = 0, size = 0;
    if (R_FAILED(svcGetInfo(&base, InfoType_AslrRegionAddress, CUR_PROCESS_HANDLE, 0)) ||
        R_FAILED(svcGetInfo(&size, InfoType_AslrRegionSize, CUR_PROCESS_HANDLE, 0)) ||
        !size || size > UINT64_MAX - base) { errno = EIO; return -1; }
    const u64 end = base + size;
    u64 address = base, mapped = 0, resident = 0, shared = 0, text = 0, data = 0;
    unsigned queries = 0;
    while (address < end) {
        MemoryInfo info; u32 page;
        if (++queries > 8192) { errno = EOVERFLOW; return -1; }
        if (R_FAILED(svcQueryMemory(&info, &page, address)) || info.addr > address ||
            !info.size || info.size > UINT64_MAX - info.addr || info.addr + info.size <= address) {
            errno = EIO; return -1;
        }
        const u64 next = info.addr + info.size, bytes = (next < end ? next : end) - address;
        const unsigned type = info.type & MemState_Type;
        if (type != MemType_Unmapped && type != MemType_Reserved) {
            mapped += bytes;
            if (type != MemType_Io) {
                resident += bytes;
                if (type == MemType_SharedMem) shared += bytes;
                if (info.perm & Perm_X) text += bytes;
                if (info.perm & Perm_W) data += bytes;
            }
        }
        address = next;
    }
    /* Linux statm uses page counts; its obsolete library/dirty fields are zero. */
    const int length = snprintf(file->text, sizeof(file->text),
        "%" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " 0 %" PRIu64 " 0\n",
        mapped / 4096u, resident / 4096u, shared / 4096u, text / 4096u, data / 4096u);
    if (length < 0 || (size_t)length >= sizeof(file->text)) { errno = EOVERFLOW; return -1; }
    file->length = (size_t)length;
    file->position = 0;
    return 0;
}

static bool proc_memory_proc_memory_path(const char *path) {
    const char *suffix = path ? strchr(path, ':') : NULL;
    return suffix && strcmp(suffix + 1, "/statm") == 0;
}
static int proc_memory_proc_memory_open(struct _reent *r, void *data, const char *path, int flags, int mode) {
    (void)mode;
    if (!proc_memory_proc_memory_path(path)) { r->_errno = ENOENT; return -1; }
    if ((flags & O_ACCMODE) != O_RDONLY || (flags & (O_TRUNC | O_APPEND))) {
        r->_errno = EACCES; return -1;
    }
    const int rc = proc_memory_proc_memory_capture(data);
    if (rc < 0) r->_errno = errno;
    return rc;
}
static int proc_memory_proc_memory_close(struct _reent *r, void *data) { (void)r; (void)data; return 0; }
static ssize_t proc_memory_proc_memory_read(struct _reent *r, void *data, char *buffer, size_t count) {
    proc_memory_ProcMemoryFile *file = data;
    if (count && !buffer) { r->_errno = EFAULT; return -1; }
    if ((uint64_t)file->position >= file->length) return 0;
    const size_t left = file->length - (size_t)file->position;
    if (count > left) count = left;
    if (count) memcpy(buffer, file->text + file->position, count);
    file->position += (off_t)count;
    return (ssize_t)count;
}
static ssize_t proc_memory_proc_memory_write(struct _reent *r, void *data, const char *buffer, size_t count) {
    (void)data; (void)buffer; (void)count; r->_errno = EBADF; return -1;
}
static off_t proc_memory_proc_memory_seek(struct _reent *r, void *data, off_t offset, int whence) {
    proc_memory_ProcMemoryFile *file = data;
    off_t next = offset;
    if (whence == SEEK_CUR && __builtin_add_overflow(file->position, offset, &next)) {
        r->_errno = EOVERFLOW; return -1;
    }
    if ((whence != SEEK_SET && whence != SEEK_CUR) || next < 0) { r->_errno = EINVAL; return -1; }
    if (next == 0 && file->position != 0 && proc_memory_proc_memory_capture(file) < 0) {
        r->_errno = errno; return -1;
    }
    file->position = next;
    return next;
}
static int proc_memory_proc_memory_fstat(struct _reent *r, void *data, struct stat *out) {
    (void)r; (void)data;
    memset(out, 0, sizeof(*out));
    out->st_mode = S_IFREG | 0444; out->st_nlink = 1; out->st_blksize = 4096;
    out->st_ino = 1; out->st_dev = 0x9fa0;
    return 0;
}
static int proc_memory_proc_memory_stat(struct _reent *r, const char *path, struct stat *out) {
    if (!proc_memory_proc_memory_path(path)) { r->_errno = ENOENT; return -1; }
    return proc_memory_proc_memory_fstat(r, NULL, out);
}
static const devoptab_t proc_memory_g_proc_memory_device = {
    .name = "apn64proc", .structSize = sizeof(proc_memory_ProcMemoryFile), .open_r = proc_memory_proc_memory_open,
    .close_r = proc_memory_proc_memory_close, .read_r = proc_memory_proc_memory_read, .write_r = proc_memory_proc_memory_write,
    .seek_r = proc_memory_proc_memory_seek, .fstat_r = proc_memory_proc_memory_fstat, .stat_r = proc_memory_proc_memory_stat,
};
static pthread_mutex_t proc_memory_g_proc_memory_device_lock = PTHREAD_MUTEX_INITIALIZER;
bool ssrnx_proc_memory_map(const char *guest, char *out, size_t capacity) {
    if (!guest || strcmp(guest, "/proc/self/statm") != 0) return false;
    const char *path = "apn64proc:/statm";
    if (!out || capacity <= strlen(path)) { errno = ENAMETOOLONG; return false; }
    pthread_mutex_lock(&proc_memory_g_proc_memory_device_lock);
    int device = FindDevice("apn64proc:");
    if (device < 0) device = AddDevice(&proc_memory_g_proc_memory_device);
    pthread_mutex_unlock(&proc_memory_g_proc_memory_device_lock);
    if (device < 0) return false;
    memcpy(out, path, strlen(path) + 1);
    return true;
}
#else
bool ssrnx_proc_memory_map(const char *guest, char *out, size_t capacity) {
    (void)guest; (void)out; (void)capacity; return false;
}
#endif



/* ---- file_contract ---- */
#pragma push_macro("SSRNX_FILE_PATH_CAP")
#pragma push_macro("SSRNX_FILE_TRACE_MAX")




#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>

#define SSRNX_FILE_TRACE_MAX 128u
#define SSRNX_FILE_PATH_CAP  512u

extern void ssrnx_android_file_observer_notify_event(const char *, int) __attribute__((weak));

typedef struct file_contract_SsrnxFileTrace {
    bool used;
    bool package_apk;
    int fd;
    int android_flags;
    int host_flags;
    int descriptor_flags;
    uint64_t bytes_read;
    uint64_t bytes_written;
    uint64_t writes;
    bool access_notified;
    bool modify_notified;
    char guest_path[SSRNX_FILE_PATH_CAP];
    char host_path[SSRNX_FILE_PATH_CAP];
} file_contract_SsrnxFileTrace;

static file_contract_SsrnxFileTrace file_contract_g_files[SSRNX_FILE_TRACE_MAX];
static pthread_mutex_t file_contract_g_file_lock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t file_contract_g_trace_lines;
/* Host descriptors fit the Linux fd_set range used by SSRNX's socket layer.
 * State is per descriptor, independent of socket status/open-file flags. */
static unsigned char file_contract_g_socket_descriptor_flags[1024];

static file_contract_SsrnxFileTrace *file_contract_find_locked(int fd) {
    for (size_t i = 0; i < SSRNX_FILE_TRACE_MAX; ++i)
        if (file_contract_g_files[i].used && file_contract_g_files[i].fd == fd) return &file_contract_g_files[i];
    return NULL;
}

static bool file_contract_trace_slot(void) {
    return __atomic_fetch_add(&file_contract_g_trace_lines, 1u, __ATOMIC_RELAXED) < 192u;
}

void ssrnx_file_contract_reset(void) {
    pthread_mutex_lock(&file_contract_g_file_lock);
    memset(file_contract_g_files, 0, sizeof(file_contract_g_files));
    memset(file_contract_g_socket_descriptor_flags, 0, sizeof(file_contract_g_socket_descriptor_flags));
    pthread_mutex_unlock(&file_contract_g_file_lock);
    __atomic_store_n(&file_contract_g_trace_lines, 0u, __ATOMIC_RELAXED);
}

void ssrnx_file_contract_open(int fd, const char *guest_path, const char *host_path,
                              int android_flags, int host_flags, bool package_apk) {
    if (fd < 0 || !guest_path || !host_path) return;
    pthread_mutex_lock(&file_contract_g_file_lock);
    if ((unsigned)fd < sizeof(file_contract_g_socket_descriptor_flags)) file_contract_g_socket_descriptor_flags[fd] = 0;
    file_contract_SsrnxFileTrace *t = file_contract_find_locked(fd);
    if (!t) {
        for (size_t i = 0; i < SSRNX_FILE_TRACE_MAX; ++i) {
            if (!file_contract_g_files[i].used) { t = &file_contract_g_files[i]; break; }
        }
    }
    if (t) {
        memset(t, 0, sizeof(*t));
        t->used = true;
        t->package_apk = package_apk;
        t->fd = fd;
        t->android_flags = android_flags;
        t->host_flags = host_flags;
#ifdef O_CLOEXEC
        t->descriptor_flags = (host_flags & O_CLOEXEC) ? FD_CLOEXEC : 0;
#endif
        snprintf(t->guest_path, sizeof(t->guest_path), "%s", guest_path);
        snprintf(t->host_path, sizeof(t->host_path), "%s", host_path);
    }
    pthread_mutex_unlock(&file_contract_g_file_lock);
    if (ssrnx_android_file_observer_notify_event)
        ssrnx_android_file_observer_notify_event(host_path, SSRNX_FO_OPEN);
}

void ssrnx_file_contract_dup(int oldfd, int newfd) {
    if (oldfd < 0 || newfd < 0 || oldfd == newfd) return;
    bool copied = false;
    pthread_mutex_lock(&file_contract_g_file_lock);
    if ((unsigned)newfd < sizeof(file_contract_g_socket_descriptor_flags))
        file_contract_g_socket_descriptor_flags[newfd] = (unsigned)oldfd < sizeof(file_contract_g_socket_descriptor_flags) &&
            file_contract_g_socket_descriptor_flags[oldfd] ? 1 : 0; /* dup clears CLOEXEC. */
    file_contract_SsrnxFileTrace *src = file_contract_find_locked(oldfd);
    file_contract_SsrnxFileTrace *dst = file_contract_find_locked(newfd);
    if (!dst) {
        for (size_t i = 0; i < SSRNX_FILE_TRACE_MAX; ++i) {
            if (!file_contract_g_files[i].used) { dst = &file_contract_g_files[i]; break; }
        }
    }
    if (src && dst) {
        file_contract_SsrnxFileTrace copy = *src;
        copy.fd = newfd;
        copy.descriptor_flags = 0; /* dup/dup2 clear flags on the new descriptor. */
        copy.bytes_read = 0;
        copy.bytes_written = 0;
        copy.writes = 0;
        *dst = copy;
        copied = true;
    } else if (!src && dst) {
        /* dup2() replaces newfd's previous open-file description. If the
         * source is untracked, any old provenance for newfd is now stale. */
        memset(dst, 0, sizeof(*dst));
    }
    pthread_mutex_unlock(&file_contract_g_file_lock);
    if (copied && file_contract_trace_slot()) SSRNX_LOGCI("FILE", "dup oldfd=%d newfd=%d", oldfd, newfd);
}


int ssrnx_file_contract_status_flags(int fd) {
    pthread_mutex_lock(&file_contract_g_file_lock);
    file_contract_SsrnxFileTrace *t = file_contract_find_locked(fd);
    if (!t) { pthread_mutex_unlock(&file_contract_g_file_lock); return -2; }
    struct stat st;
    int result = fstat(fd, &st);
    if (result == 0) {
#ifdef __SWITCH__
        /* fsdev has no fcntl callback; validate the live descriptor and use
         * the access/status flags captured at its successful open. */
        result = t->host_flags & (O_ACCMODE | O_APPEND | O_NONBLOCK);
#ifdef O_SYNC
        result |= t->host_flags & O_SYNC;
#endif
#else
        result = fcntl(fd, F_GETFL, 0);
#endif
    }
    const int saved = errno;
    pthread_mutex_unlock(&file_contract_g_file_lock);
    errno = saved;
    return result;
}

static int file_contract_file_allocate_locked(int fd, int64_t offset, int64_t length, bool *changed) {
    struct stat st;
    if (fstat(fd, &st) != 0) return errno;
    if (S_ISFIFO(st.st_mode)) return ESPIPE;
    if (!S_ISREG(st.st_mode)) return ENODEV;
#ifdef __SWITCH__
    const file_contract_SsrnxFileTrace *t = file_contract_find_locked(fd);
    if (!t) return EOPNOTSUPP;
    if ((t->host_flags & O_ACCMODE) == O_RDONLY) return EBADF;
    /* SD FAT/exFAT file data has no sparse holes. Other fsdev mounts need an
     * allocation contract before an existing range can be reported reserved. */
    if (strncmp(t->host_path, "sdmc:/", 6) != 0) return EOPNOTSUPP;
    const int64_t end = offset + length;
    if (st.st_size >= end) return 0;
    if (st.st_size < 0 || (int64_t)(off_t)end != end) return EFBIG;
    const off_t original = lseek(fd, 0, SEEK_CUR);
    if (original == (off_t)-1) return errno;
    int error = 0;
    if (lseek(fd, st.st_size, SEEK_SET) == (off_t)-1) error = errno;
    int64_t position = st.st_size;
    static const unsigned char zero[16384];
    /* Materialize the entire extension, including a gap before offset. Merely
     * changing the file size is not a portable storage reservation. */
    while (!error && position < end) {
        size_t count = end - position < (int64_t)sizeof(zero) ? (size_t)(end - position) : sizeof(zero);
        const ssize_t written = write(fd, zero, count);
        if (written < 0) { if (errno != EINTR) error = errno; }
        else if (!written) error = EIO;
        else { position += written; *changed = true; }
    }
    if (!error && fsync(fd) != 0) error = errno;
    if (lseek(fd, original, SEEK_SET) == (off_t)-1 && !error) error = errno;
    return error;
#else
    const int error = posix_fallocate(fd, (off_t)offset, (off_t)length);
    *changed = !error && st.st_size < offset + length;
    return error;
#endif
}

int ssrnx_file_contract_allocate(int fd, int64_t offset, int64_t length) {
    const int saved = errno;
    int error;
    bool changed = false;
    char host_path[SSRNX_FILE_PATH_CAP] = {0};
    if (offset < 0 || length <= 0) error = EINVAL;
    else if (offset > INT64_MAX - length) error = EFBIG;
    else {
        pthread_mutex_lock(&file_contract_g_file_lock); /* Pins tracked descriptors against close. */
        error = file_contract_file_allocate_locked(fd, offset, length, &changed);
        const file_contract_SsrnxFileTrace *t = file_contract_find_locked(fd);
        if (changed && t) snprintf(host_path, sizeof(host_path), "%s", t->host_path);
        pthread_mutex_unlock(&file_contract_g_file_lock);
    }
    if (host_path[0] && ssrnx_android_file_observer_notify_event)
        ssrnx_android_file_observer_notify_event(host_path, SSRNX_FO_MODIFY);
    errno = saved;
    return error;
}

int ssrnx_file_contract_socket_open(int fd, bool cloexec) {
    if (fd < 0) { errno = EBADF; return -1; }
    if ((unsigned)fd >= sizeof(file_contract_g_socket_descriptor_flags)) { errno = EMFILE; return -1; }
    int type = 0;
    socklen_t length = sizeof(type);
    if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &length) != 0) return -1;
#ifndef __SWITCH__
    if (fcntl(fd, F_SETFD, cloexec ? FD_CLOEXEC : 0) < 0) return -1;
#endif
    pthread_mutex_lock(&file_contract_g_file_lock);
    file_contract_g_socket_descriptor_flags[fd] = cloexec ? 3 : 1;
    pthread_mutex_unlock(&file_contract_g_file_lock);
    return 0;
}

int ssrnx_file_contract_descriptor_flags(int fd, bool set, int flags) {
    pthread_mutex_lock(&file_contract_g_file_lock);
    if ((unsigned)fd < sizeof(file_contract_g_socket_descriptor_flags) && file_contract_g_socket_descriptor_flags[fd]) {
        int type = 0;
        socklen_t length = sizeof(type);
        int result = getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &length);
        if (result >= 0) {
#ifdef __SWITCH__
            if (set) file_contract_g_socket_descriptor_flags[fd] = (flags & FD_CLOEXEC) ? 3 : 1;
            result = set ? 0 : (file_contract_g_socket_descriptor_flags[fd] & 2 ? FD_CLOEXEC : 0);
#else
            result = fcntl(fd, set ? F_SETFD : F_GETFD, flags & FD_CLOEXEC);
#endif
        }
        const int saved = errno;
        pthread_mutex_unlock(&file_contract_g_file_lock);
        errno = saved;
        return result;
    }
    file_contract_SsrnxFileTrace *t = file_contract_find_locked(fd);
    if (!t) { pthread_mutex_unlock(&file_contract_g_file_lock); return -2; }
    struct stat st;
    int result = fstat(fd, &st);
    if (result == 0) {
#ifdef __SWITCH__
        /* fsdev has no fcntl callback. Keep per-descriptor guest state; SSRNX
         * rejects exec, so no host process-image transition can leak the fd. */
        if (set) t->descriptor_flags = flags & FD_CLOEXEC;
        result = set ? 0 : t->descriptor_flags;
#else
        result = fcntl(fd, set ? F_SETFD : F_GETFD, flags & FD_CLOEXEC);
        if (result >= 0) t->descriptor_flags = set ? flags & FD_CLOEXEC : result;
#endif
    }
    const int saved = errno;
    pthread_mutex_unlock(&file_contract_g_file_lock);
    errno = saved;
    return result;
}

int ssrnx_file_contract_reopen_duplicate(int oldfd, int minimum, bool cloexec) {
    if (oldfd < 0) { errno = EBADF; return -1; }
    if (minimum < 0) { errno = EINVAL; return -1; }

    file_contract_SsrnxFileTrace src;
    bool found = false;
    pthread_mutex_lock(&file_contract_g_file_lock);
    file_contract_SsrnxFileTrace *tracked = file_contract_find_locked(oldfd);
    if (tracked) {
        src = *tracked;
        found = true;
    }
    pthread_mutex_unlock(&file_contract_g_file_lock);
    if (!found) { errno = ENOENT; return -2; }

    struct stat st;
    if (fstat(oldfd, &st) != 0) {
        const int saved = errno;
        if (file_contract_trace_slot())
            SSRNX_LOGCI("FILE", "dup-reopen-fail oldfd=%d guest=%s step=fstat errno=%d",
                        oldfd, src.guest_path, saved);
        errno = saved;
        return -1;
    }
    if (!S_ISREG(st.st_mode)) { errno = ENOTSUP; return -1; }

    /* Do not query F_GETFL here. libnx/newlib fsdev can service read/fstat/lseek
     * for a regular file while its fcntl path is incomplete. Preview67 proved
     * that relying on F_GETFL can make dup() fail even though the source fd is
     * valid. The exact host open flags were captured when SSRNX opened the
     * descriptor, so rebuild only the status/access flags that are meaningful
     * for a duplicated open file description. Never replay O_CREAT/O_EXCL/
     * O_TRUNC while duplicating an already-open file. */
    int open_flags = src.host_flags & O_ACCMODE;
#ifdef O_APPEND
    if (src.host_flags & O_APPEND) open_flags |= O_APPEND;
#endif
#ifdef O_NONBLOCK
    if (src.host_flags & O_NONBLOCK) open_flags |= O_NONBLOCK;
#endif

    const off_t old_offset = lseek(oldfd, 0, SEEK_CUR);
    if (old_offset == (off_t)-1) {
        const int saved = errno;
        if (file_contract_trace_slot())
            SSRNX_LOGCI("FILE", "dup-reopen-fail oldfd=%d guest=%s step=lseek errno=%d",
                        oldfd, src.guest_path, saved);
        errno = saved;
        return -1;
    }

    /* open() gives us a valid devoptab-backed descriptor. Keep descriptors
     * below the requested F_DUPFD minimum temporarily occupied until the
     * first valid descriptor at/above the minimum is obtained. */
    int *held = NULL;
    size_t held_count = 0;
    size_t held_cap = 0;
    int out = -1;
    for (;;) {
        out = open(src.host_path, open_flags);
        if (out < 0) break;
        if (out >= minimum) break;

        if (held_count == held_cap) {
            size_t next_cap = held_cap ? held_cap * 2u : 8u;
            if (next_cap > 4096u) next_cap = 4096u;
            if (next_cap <= held_cap) {
                close(out); out = -1; errno = EMFILE; break;
            }
            int *next = (int *)realloc(held, next_cap * sizeof(*held));
            if (!next) {
                close(out); out = -1; errno = ENOMEM; break;
            }
            held = next;
            held_cap = next_cap;
        }
        held[held_count++] = out;
    }

    const int open_error = errno;
    for (size_t i = 0; i < held_count; ++i) close(held[i]);
    free(held);
    if (out < 0) {
        if (file_contract_trace_slot())
            SSRNX_LOGCI("FILE", "dup-reopen-fail oldfd=%d guest=%s step=open minimum=%d errno=%d",
                        oldfd, src.guest_path, minimum, open_error);
        errno = open_error;
        return -1;
    }

    if (lseek(out, old_offset, SEEK_SET) == (off_t)-1) {
        const int saved = errno;
        if (file_contract_trace_slot())
            SSRNX_LOGCI("FILE", "dup-reopen-fail oldfd=%d newfd=%d guest=%s step=restore-offset errno=%d",
                        oldfd, out, src.guest_path, saved);
        close(out);
        errno = saved;
        return -1;
    }
    struct stat verify;
    if (fstat(out, &verify) != 0) {
        const int saved = errno;
        if (file_contract_trace_slot())
            SSRNX_LOGCI("FILE", "dup-reopen-fail oldfd=%d newfd=%d guest=%s step=verify-fstat errno=%d",
                        oldfd, out, src.guest_path, saved);
        close(out);
        errno = saved;
        return -1;
    }

    ssrnx_file_contract_dup(oldfd, out);
    const int flag_result = ssrnx_file_contract_descriptor_flags(out, true, cloexec ? FD_CLOEXEC : 0);
    if (flag_result < 0) {
        const int saved = flag_result == -2 ? EMFILE : errno;
        SSRNX_LOGCW("FILE", "dup-reopen-fail oldfd=%d newfd=%d guest=%s step=descriptor-flags errno=%d", oldfd, out, src.guest_path, saved);
        ssrnx_file_contract_close(out);
        close(out);
        errno = saved;
        return -1;
    }
    if (file_contract_trace_slot()) {
        SSRNX_LOGCI("FILE", "dup-reopen oldfd=%d newfd=%d guest=%s size=%lld offset=%lld cloexec=%d",
                    oldfd, out, src.guest_path, (long long)verify.st_size,
                    (long long)old_offset, cloexec ? 1 : 0);
    }
    return out;
}

int ssrnx_file_contract_duplicate(int oldfd, int minimum, bool cloexec) {
    if (oldfd < 0) { errno = EBADF; return -1; }
    if (minimum < 0) { errno = EINVAL; return -1; }
#ifdef __SWITCH__
    /* shortcut: fsdev reopen copies the initial offset, not a shared open-file
     * description; upgrade the common fd layer for cross-duplicate seeking. */
    const int reopened = ssrnx_file_contract_reopen_duplicate(oldfd, minimum, cloexec);
    if (reopened != -2) return reopened;
#endif
    int fd;
#ifdef F_DUPFD_CLOEXEC
    fd = fcntl(oldfd, cloexec ? F_DUPFD_CLOEXEC : F_DUPFD, minimum);
#else
    fd = fcntl(oldfd, F_DUPFD, minimum);
    if (fd >= 0 && cloexec && fcntl(fd, F_SETFD, FD_CLOEXEC) != 0) {
        const int saved = errno; close(fd); errno = saved; return -1;
    }
#endif
    if (fd >= 0) {
        ssrnx_file_contract_dup(oldfd, fd);
        pthread_mutex_lock(&file_contract_g_file_lock);
        file_contract_SsrnxFileTrace *t = file_contract_find_locked(fd);
        if (t) t->descriptor_flags = cloexec ? FD_CLOEXEC : 0;
        if ((unsigned)fd < sizeof(file_contract_g_socket_descriptor_flags) && file_contract_g_socket_descriptor_flags[fd])
            file_contract_g_socket_descriptor_flags[fd] = cloexec ? 3 : 1;
        pthread_mutex_unlock(&file_contract_g_file_lock);
    }
    return fd;
}

/* ---------- package APK entry access trace ----------
 * Engines read APK content through their own ZIP readers on raw descriptors,
 * so the asset name being loaded is invisible at the syscall layer. For
 * descriptors on the package APK, the first read that lands inside each ZIP
 * entry is logged with the entry name (from the central directory), which
 * shows which StreamingAssets/bundles/catalogs an engine actually loads. */
typedef struct file_contract_ApkEntrySpan {
    uint32_t lho; uint32_t size; uint32_t name_len; bool logged; char *name;
    uint64_t bytes;      /* data bytes read so far */
    uint64_t next_mark;  /* next cumulative-bytes milestone to report */
} file_contract_ApkEntrySpan;
static struct {
    char path[SSRNX_FILE_PATH_CAP];
    file_contract_ApkEntrySpan *spans;   /* sorted by local header offset */
    size_t count;
    bool failed;
} file_contract_g_apk_index;
static uint32_t file_contract_g_apk_entry_lines;

static uint32_t file_contract_rd16le(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8); }
static uint32_t file_contract_rd32le(const uint8_t *p) { return file_contract_rd16le(p) | (file_contract_rd16le(p + 2) << 16); }
static int file_contract_span_cmp(const void *a, const void *b) {
    const uint32_t x = ((const file_contract_ApkEntrySpan *)a)->lho, y = ((const file_contract_ApkEntrySpan *)b)->lho;
    return x < y ? -1 : x > y;
}

/* Caller holds g_file_lock. */
static bool file_contract_apk_index_load_locked(const char *host_path) {
    if (file_contract_g_apk_index.spans && !strcmp(file_contract_g_apk_index.path, host_path)) return true;
    if (file_contract_g_apk_index.failed && !strcmp(file_contract_g_apk_index.path, host_path)) return false;
    for (size_t i = 0; i < file_contract_g_apk_index.count; ++i) free(file_contract_g_apk_index.spans[i].name);
    free(file_contract_g_apk_index.spans);
    memset(&file_contract_g_apk_index, 0, sizeof(file_contract_g_apk_index));
    snprintf(file_contract_g_apk_index.path, sizeof(file_contract_g_apk_index.path), "%s", host_path);
    file_contract_g_apk_index.failed = true;
    FILE *fp = fopen(host_path, "rb");
    if (!fp) return false;
    uint8_t tail[65557];
    long size = 0;
    if (fseek(fp, 0, SEEK_END) != 0 || (size = ftell(fp)) < 22) { fclose(fp); return false; }
    const long want = size < (long)sizeof(tail) ? size : (long)sizeof(tail);
    if (fseek(fp, size - want, SEEK_SET) != 0 || fread(tail, 1, (size_t)want, fp) != (size_t)want) { fclose(fp); return false; }
    long eocd = -1;
    for (long i = want - 22; i >= 0; --i)
        if (file_contract_rd32le(tail + i) == 0x06054b50u) { eocd = i; break; }
    if (eocd < 0) { fclose(fp); return false; }
    const uint32_t entries = file_contract_rd16le(tail + eocd + 10), cd_size = file_contract_rd32le(tail + eocd + 12), cd_off = file_contract_rd32le(tail + eocd + 16);
    uint8_t *cd = malloc(cd_size ? cd_size : 1u);
    file_contract_ApkEntrySpan *spans = calloc(entries ? entries : 1u, sizeof(*spans));
    if (!cd || !spans || fseek(fp, (long)cd_off, SEEK_SET) != 0 || fread(cd, 1, cd_size, fp) != cd_size) {
        free(cd); free(spans); fclose(fp); return false;
    }
    fclose(fp);
    size_t n = 0;
    for (uint32_t pos = 0; n < entries && pos + 46u <= cd_size && file_contract_rd32le(cd + pos) == 0x02014b50u;) {
        const uint32_t name_len = file_contract_rd16le(cd + pos + 28), extra = file_contract_rd16le(cd + pos + 30), comment = file_contract_rd16le(cd + pos + 32);
        if (pos + 46u + name_len > cd_size) break;
        spans[n].size = file_contract_rd32le(cd + pos + 20);
        spans[n].lho = file_contract_rd32le(cd + pos + 42);
        spans[n].name_len = name_len;
        spans[n].name = malloc(name_len + 1u);
        if (spans[n].name) { memcpy(spans[n].name, cd + pos + 46, name_len); spans[n].name[name_len] = '\0'; }
        ++n;
        pos += 46u + name_len + extra + comment;
    }
    free(cd);
    qsort(spans, n, sizeof(*spans), file_contract_span_cmp);
    file_contract_g_apk_index.spans = spans;
    file_contract_g_apk_index.count = n;
    file_contract_g_apk_index.failed = false;
    return true;
}

static void file_contract_apk_entry_note_read_locked(const file_contract_SsrnxFileTrace *t, int64_t offset, int64_t length) {
    if (offset < 0 || __atomic_load_n(&file_contract_g_apk_entry_lines, __ATOMIC_RELAXED) >= 2000u) return;
    if (!file_contract_apk_index_load_locked(t->host_path) || !file_contract_g_apk_index.count) return;
    /* Last entry whose local header starts at or before offset. */
    size_t lo = 0, hi = file_contract_g_apk_index.count;
    while (hi - lo > 1u) {
        const size_t mid = (lo + hi) / 2u;
        if ((int64_t)file_contract_g_apk_index.spans[mid].lho <= offset) lo = mid; else hi = mid;
    }
    file_contract_ApkEntrySpan *e = &file_contract_g_apk_index.spans[lo];
    /* Reads inside the 30-byte local header + name are ZIP directory walks
     * (an engine locating entries), not content loads. */
    if (offset < (int64_t)e->lho + 30 + (int64_t)e->name_len) return;
    e->bytes += (uint64_t)length;
    if (!e->logged) {
        e->logged = true;
        e->next_mark = 256u * 1024u;
        __atomic_fetch_add(&file_contract_g_apk_entry_lines, 1u, __ATOMIC_RELAXED);
        SSRNX_LOGCI("FILE", "apk-entry data-read fd=%d entry=%s entrySize=%u offset=%lld len=%lld",
                    t->fd, e->name ? e->name : "?", e->size, (long long)offset, (long long)length);
        return;
    }
    /* Progress through larger entries: one line per 256 KiB of data read. */
    if (e->bytes >= e->next_mark) {
        while (e->next_mark <= e->bytes) e->next_mark += 256u * 1024u;
        __atomic_fetch_add(&file_contract_g_apk_entry_lines, 1u, __ATOMIC_RELAXED);
        SSRNX_LOGCI("FILE", "apk-entry progress fd=%d entry=%s read=%llu/%u offset=%lld",
                    t->fd, e->name ? e->name : "?", (unsigned long long)e->bytes, e->size, (long long)offset);
    }
}

/* offset < 0: read() at the current descriptor offset. */
static void file_contract_contract_read_common(int fd, int64_t offset, int64_t result) {
    if (result <= 0) return;
    char host_path[SSRNX_FILE_PATH_CAP] = {0};
    bool notify = false;
    pthread_mutex_lock(&file_contract_g_file_lock);
    file_contract_SsrnxFileTrace *t = file_contract_find_locked(fd);
    if (t && t->package_apk) {
        if (offset < 0) {
            /* The descriptor offset has already advanced by result. */
            const off_t now = lseek(fd, 0, SEEK_CUR);
            offset = now == (off_t)-1 ? -1 : (int64_t)now - result;
        }
        file_contract_apk_entry_note_read_locked(t, offset, result);
    }
    if (t) {
        t->bytes_read += (uint64_t)result;
        if (!t->access_notified) {
            t->access_notified = true;
            snprintf(host_path, sizeof(host_path), "%s", t->host_path);
            notify = true;
        }
    }
    pthread_mutex_unlock(&file_contract_g_file_lock);
    if (notify && ssrnx_android_file_observer_notify_event)
        ssrnx_android_file_observer_notify_event(host_path, SSRNX_FO_ACCESS);
}

void ssrnx_file_contract_read(int fd, size_t requested, int64_t result) {
    (void)requested;
    file_contract_contract_read_common(fd, -1, result);
}

void ssrnx_file_contract_read_at(int fd, int64_t offset, int64_t result) {
    file_contract_contract_read_common(fd, offset, result);
}

void ssrnx_file_contract_write(int fd, size_t requested, int64_t result) {
    if (result <= 0) return;
    char host_path[SSRNX_FILE_PATH_CAP] = {0};
    bool notify = false;
    pthread_mutex_lock(&file_contract_g_file_lock);
    file_contract_SsrnxFileTrace *t = file_contract_find_locked(fd);
    if (t) {
        t->bytes_written += (uint64_t)result;
        ++t->writes;
        if (!t->modify_notified) {
            t->modify_notified = true;
            snprintf(host_path, sizeof(host_path), "%s", t->host_path);
            notify = true;
        }
    }
    pthread_mutex_unlock(&file_contract_g_file_lock);
    if (notify && ssrnx_android_file_observer_notify_event)
        ssrnx_android_file_observer_notify_event(host_path, SSRNX_FO_MODIFY);
    (void)requested;
}

void ssrnx_file_contract_ftruncate(int fd, int64_t requested_size, int result, int error_code) {
    char path[SSRNX_FILE_PATH_CAP] = {0};
    char host_path[SSRNX_FILE_PATH_CAP] = {0};
    pthread_mutex_lock(&file_contract_g_file_lock);
    file_contract_SsrnxFileTrace *t = file_contract_find_locked(fd);
    if (t) {
        snprintf(path, sizeof(path), "%s", t->guest_path);
        snprintf(host_path, sizeof(host_path), "%s", t->host_path);
    }
    pthread_mutex_unlock(&file_contract_g_file_lock);
    if (result == 0 && host_path[0] && ssrnx_android_file_observer_notify_event)
        ssrnx_android_file_observer_notify_event(host_path, SSRNX_FO_MODIFY);
    if (!path[0] || !file_contract_trace_slot()) return;
    struct stat st;
    const int src = fstat(fd, &st);
    const int saved = errno;
    SSRNX_LOGCI("FILE", "ftruncate fd=%d guest=%s requested=%lld result=%d finalSize=%lld stat=%d errno=%d",
                fd, path, (long long)requested_size, result,
                (long long)(src == 0 ? st.st_size : -1), src,
                result == 0 ? (src == 0 ? 0 : saved) : error_code);
}

void ssrnx_file_contract_fstat(int fd, int result, int64_t size, uint32_t mode, int error_code) {
    char path[SSRNX_FILE_PATH_CAP] = {0};
    pthread_mutex_lock(&file_contract_g_file_lock);
    file_contract_SsrnxFileTrace *t = file_contract_find_locked(fd);
    if (t) snprintf(path, sizeof(path), "%s", t->guest_path);
    pthread_mutex_unlock(&file_contract_g_file_lock);
    if (!path[0] || !file_contract_trace_slot()) return;
    SSRNX_LOGCI("FILE", "fstat fd=%d guest=%s result=%d size=%lld mode=%#o errno=%d",
                fd, path, result, (long long)size, (unsigned)mode,
                result == 0 ? 0 : error_code);
}

void ssrnx_file_contract_mmap(int fd, size_t length, int64_t offset,
                              const void *result, int error_code) {
    char path[SSRNX_FILE_PATH_CAP] = {0};
    pthread_mutex_lock(&file_contract_g_file_lock);
    file_contract_SsrnxFileTrace *t = file_contract_find_locked(fd);
    if (t) snprintf(path, sizeof(path), "%s", t->guest_path);
    pthread_mutex_unlock(&file_contract_g_file_lock);
    if (!path[0] || !file_contract_trace_slot()) return;
    struct stat st;
    const int src = fstat(fd, &st);
    const int saved = errno;
    SSRNX_LOGCI("FILE", "mmap fd=%d guest=%s requested=%zu offset=%lld hostSize=%lld stat=%d result=%p errno=%d",
                fd, path, length, (long long)offset,
                (long long)(src == 0 ? st.st_size : -1), src, result,
                error_code ? error_code : (src == 0 ? 0 : saved));
}

bool ssrnx_file_contract_host_path(int fd, char *out, size_t cap) {
    if (!out || cap == 0) return false;
    bool found = false;
    pthread_mutex_lock(&file_contract_g_file_lock);
    const file_contract_SsrnxFileTrace *t = file_contract_find_locked(fd);
    if (t && t->host_path[0]) found = snprintf(out, cap, "%s", t->host_path) < (int)cap;
    pthread_mutex_unlock(&file_contract_g_file_lock);
    return found;
}

int ssrnx_file_contract_fd_for_host_path(const char *host_path) {
    int fd = -1;
    if (!host_path || !host_path[0]) return -1;
    pthread_mutex_lock(&file_contract_g_file_lock);
    for (size_t i = 0; i < SSRNX_FILE_TRACE_MAX; ++i) {
        if (file_contract_g_files[i].used && strcmp(file_contract_g_files[i].host_path, host_path) == 0) { fd = file_contract_g_files[i].fd; break; }
    }
    pthread_mutex_unlock(&file_contract_g_file_lock);
    return fd;
}

int ssrnx_file_contract_truncate_open(const char *host_path, int64_t length) {
    if (!host_path || length < 0) { errno = EINVAL; return -1; }
    const off_t host_length = (off_t)length;
    if ((int64_t)host_length != length) { errno = EFBIG; return -1; }
    int rc = -2, saved = 0, fd = -1;
    int64_t final_size = -1;
    pthread_mutex_lock(&file_contract_g_file_lock);
    for (size_t i = 0; i < SSRNX_FILE_TRACE_MAX; ++i) {
        const file_contract_SsrnxFileTrace *t = &file_contract_g_files[i];
        if (!t->used || (t->host_flags & O_ACCMODE) == O_RDONLY ||
            strcmp(t->host_path, host_path) != 0) continue;
        struct stat st;
        if (fstat(t->fd, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        fd = t->fd;
        /* close removes its entry under this same lock before closing the host
         * fd. Holding the lock pins the descriptor without another FS handle. */
        rc = ftruncate(fd, host_length);
        saved = errno;
        if (fstat(fd, &st) == 0) final_size = (int64_t)st.st_size;
        break;
    }
    pthread_mutex_unlock(&file_contract_g_file_lock);
    if (rc != -2) {
        if (rc == 0 && ssrnx_android_file_observer_notify_event)
            ssrnx_android_file_observer_notify_event(host_path, SSRNX_FO_MODIFY);
        SSRNX_LOGCI("FILE", "truncate-open fd=%d host=%s requested=%lld result=%d finalSize=%lld errno=%d",
                    fd, host_path, (long long)length, rc, (long long)final_size, rc == 0 ? 0 : saved);
    }
    errno = saved;
    return rc;
}

void ssrnx_file_contract_close(int fd) {
    file_contract_SsrnxFileTrace copy;
    bool found = false;
    pthread_mutex_lock(&file_contract_g_file_lock);
    if ((unsigned)fd < sizeof(file_contract_g_socket_descriptor_flags)) file_contract_g_socket_descriptor_flags[fd] = 0;
    file_contract_SsrnxFileTrace *t = file_contract_find_locked(fd);
    if (t) {
        copy = *t;
        memset(t, 0, sizeof(*t));
        found = true;
    }
    pthread_mutex_unlock(&file_contract_g_file_lock);
    if (!found) return;

    if (ssrnx_android_file_observer_notify_event) {
        const int access = copy.host_flags & O_ACCMODE;
        ssrnx_android_file_observer_notify_event(
            copy.host_path, access == O_RDONLY ? SSRNX_FO_CLOSE_NOWRITE : SSRNX_FO_CLOSE_WRITE);
    }

    if (!file_contract_trace_slot()) return;
    struct stat st;
    const int src = fstat(fd, &st);
    const int saved = errno;
    SSRNX_LOGCI("FILE", "close fd=%d guest=%s reads=%llu writes=%llu writeCalls=%llu finalSize=%lld stat=%d errno=%d",
                fd, copy.guest_path,
                (unsigned long long)copy.bytes_read,
                (unsigned long long)copy.bytes_written,
                (unsigned long long)copy.writes,
                (long long)(src == 0 ? st.st_size : -1), src, src == 0 ? 0 : saved);
}


#pragma pop_macro("SSRNX_FILE_TRACE_MAX")
#pragma pop_macro("SSRNX_FILE_PATH_CAP")


/* ---- fd_compat ---- */
#pragma push_macro("SSRNX_VFD_BASE")
#pragma push_macro("SSRNX_VPIPE_CAPACITY")
#pragma push_macro("SSRNX_VPIPE_MAX")
#pragma push_macro("SSRNX_VRANDOM_BASE")
#pragma push_macro("SSRNX_VRANDOM_MAX")


extern int ssrnx_eventfd_poll(int, short) __attribute__((weak));
extern ssize_t ssrnx_eventfd_read(int, void *, size_t) __attribute__((weak));
extern ssize_t ssrnx_eventfd_write(int, const void *, size_t) __attribute__((weak));
extern int ssrnx_eventfd_close(int) __attribute__((weak));

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifdef __SWITCH__
#include <switch.h>
#endif

#define SSRNX_VPIPE_MAX 16
#define SSRNX_VPIPE_CAPACITY 512
#define SSRNX_VFD_BASE 0x64000000
#define SSRNX_VRANDOM_BASE 0x64100000
#define SSRNX_VRANDOM_MAX 8

typedef struct fd_compat_SsrnxVirtualPipe {
    bool used;
    bool read_open;
    bool write_open;
    uint8_t data[SSRNX_VPIPE_CAPACITY];
    size_t head;
    size_t count;
} fd_compat_SsrnxVirtualPipe;

static fd_compat_SsrnxVirtualPipe fd_compat_g_pipes[SSRNX_VPIPE_MAX];
static bool fd_compat_g_random_fds[SSRNX_VRANDOM_MAX];
static uint64_t fd_compat_g_random_state = UINT64_C(0x61706b3634726e67);
static pthread_mutex_t fd_compat_g_pipe_lock = PTHREAD_MUTEX_INITIALIZER;

static int fd_compat_make_fd(size_t slot, bool write_end) {
    return SSRNX_VFD_BASE + (int)(slot * 2u) + (write_end ? 1 : 0);
}

static bool fd_compat_decode_fd(int fd, size_t *out_slot, bool *out_write_end) {
    if (fd < SSRNX_VFD_BASE) return false;
    const unsigned value = (unsigned)(fd - SSRNX_VFD_BASE);
    const size_t slot = (size_t)(value / 2u);
    if (slot >= SSRNX_VPIPE_MAX) return false;
    if (out_slot) *out_slot = slot;
    if (out_write_end) *out_write_end = (value & 1u) != 0;
    return true;
}


static bool fd_compat_decode_random_fd(int fd, size_t *out_slot) {
    if (fd < SSRNX_VRANDOM_BASE) return false;
    const unsigned value = (unsigned)(fd - SSRNX_VRANDOM_BASE);
    if (value >= SSRNX_VRANDOM_MAX) return false;
    if (out_slot) *out_slot = value;
    return true;
}

#ifndef __SWITCH__
static uint64_t fd_compat_random_step(void) {
    uint64_t x = fd_compat_g_random_state;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    if (!x) x = UINT64_C(0x9e3779b97f4a7c15);
    fd_compat_g_random_state = x;
    return x;
}
#endif

int ssrnx_fd_open_urandom(void) {
    pthread_mutex_lock(&fd_compat_g_pipe_lock);
    for (size_t i = 0; i < SSRNX_VRANDOM_MAX; ++i) {
        if (fd_compat_g_random_fds[i]) continue;
        fd_compat_g_random_fds[i] = true;
        struct timespec ts = {0};
        (void)clock_gettime(CLOCK_REALTIME, &ts);
        fd_compat_g_random_state ^= (uint64_t)ts.tv_sec ^ ((uint64_t)ts.tv_nsec << 19) ^ (uintptr_t)&fd_compat_g_random_fds[i];
        pthread_mutex_unlock(&fd_compat_g_pipe_lock);
        return SSRNX_VRANDOM_BASE + (int)i;
    }
    pthread_mutex_unlock(&fd_compat_g_pipe_lock);
    errno = EMFILE;
    return -1;
}

bool ssrnx_fd_is_random(int fd) {
    size_t slot = 0;
    if (!fd_compat_decode_random_fd(fd, &slot)) return false;
    pthread_mutex_lock(&fd_compat_g_pipe_lock);
    const bool valid = fd_compat_g_random_fds[slot];
    pthread_mutex_unlock(&fd_compat_g_pipe_lock);
    return valid;
}

bool ssrnx_fd_is_pipe(int fd) {
    size_t slot = 0;
    bool write_end = false;
    if (!fd_compat_decode_fd(fd, &slot, &write_end)) return false;
    pthread_mutex_lock(&fd_compat_g_pipe_lock);
    const bool valid = fd_compat_g_pipes[slot].used &&
        (write_end ? fd_compat_g_pipes[slot].write_open : fd_compat_g_pipes[slot].read_open);
    pthread_mutex_unlock(&fd_compat_g_pipe_lock);
    return valid;
}

bool ssrnx_fd_is_virtual(int fd) {
    if (ssrnx_eventfd_poll && ssrnx_eventfd_poll(fd, 0) >= 0) return true;
    size_t random_slot = 0;
    if (fd_compat_decode_random_fd(fd, &random_slot)) {
        pthread_mutex_lock(&fd_compat_g_pipe_lock);
        const bool valid = fd_compat_g_random_fds[random_slot];
        pthread_mutex_unlock(&fd_compat_g_pipe_lock);
        return valid;
    }
    size_t slot = 0;
    bool write_end = false;
    if (!fd_compat_decode_fd(fd, &slot, &write_end)) return false;
    pthread_mutex_lock(&fd_compat_g_pipe_lock);
    const bool valid = fd_compat_g_pipes[slot].used &&
        (write_end ? fd_compat_g_pipes[slot].write_open : fd_compat_g_pipes[slot].read_open);
    pthread_mutex_unlock(&fd_compat_g_pipe_lock);
    return valid;
}

int ssrnx_fd_pipe(int fds[2]) {
    if (!fds) {
        errno = EFAULT;
        return -1;
    }
    pthread_mutex_lock(&fd_compat_g_pipe_lock);
    for (size_t i = 0; i < SSRNX_VPIPE_MAX; ++i) {
        if (fd_compat_g_pipes[i].used) continue;
        memset(&fd_compat_g_pipes[i], 0, sizeof(fd_compat_g_pipes[i]));
        fd_compat_g_pipes[i].used = true;
        fd_compat_g_pipes[i].read_open = true;
        fd_compat_g_pipes[i].write_open = true;
        fds[0] = fd_compat_make_fd(i, false);
        fds[1] = fd_compat_make_fd(i, true);
        pthread_mutex_unlock(&fd_compat_g_pipe_lock);
        return 0;
    }
    pthread_mutex_unlock(&fd_compat_g_pipe_lock);
    errno = EMFILE;
    return -1;
}

/* Kernel-grade randomness (/dev/urandom, getentropy, arc4random): the
 * Horizon CSRNG on Switch; a deterministic xorshift only in host tests. */
void ssrnx_fd_random_fill(void *buffer, size_t count) {
    if (!buffer || !count) return;
#ifdef __SWITCH__
    randomGet(buffer, count);
#else
    unsigned char *dst = (unsigned char *)buffer;
    pthread_mutex_lock(&fd_compat_g_pipe_lock);
    for (size_t i = 0; i < count; ) {
        uint64_t x = fd_compat_random_step();
        for (unsigned j = 0; j < 8 && i < count; ++j, ++i) dst[i] = (unsigned char)(x >> (j * 8));
    }
    pthread_mutex_unlock(&fd_compat_g_pipe_lock);
#endif
}

ssize_t ssrnx_fd_read(int fd, void *buffer, size_t count) {
    if (ssrnx_eventfd_poll && ssrnx_eventfd_poll(fd, 0) >= 0) return ssrnx_eventfd_read(fd, buffer, count);
    size_t random_slot = 0;
    if (fd_compat_decode_random_fd(fd, &random_slot)) {
        if (!buffer && count) { errno = EFAULT; return -1; }
        pthread_mutex_lock(&fd_compat_g_pipe_lock);
        const bool valid = fd_compat_g_random_fds[random_slot];
        pthread_mutex_unlock(&fd_compat_g_pipe_lock);
        if (!valid) { errno = EBADF; return -1; }
        ssrnx_fd_random_fill(buffer, count);
        return (ssize_t)count;
    }
    size_t slot = 0;
    bool write_end = false;
    if (!fd_compat_decode_fd(fd, &slot, &write_end) || write_end) {
        errno = EBADF;
        return -1;
    }
    if (!buffer && count != 0) {
        errno = EFAULT;
        return -1;
    }
    if (count == 0) return 0;

    pthread_mutex_lock(&fd_compat_g_pipe_lock);
    fd_compat_SsrnxVirtualPipe *pipe_state = &fd_compat_g_pipes[slot];
    if (!pipe_state->used || !pipe_state->read_open) {
        pthread_mutex_unlock(&fd_compat_g_pipe_lock);
        errno = EBADF;
        return -1;
    }
    if (pipe_state->count == 0) {
        const bool writer_closed = !pipe_state->write_open;
        pthread_mutex_unlock(&fd_compat_g_pipe_lock);
        if (writer_closed) return 0; /* EOF */
        errno = EAGAIN;              /* callers poll before reading */
        return -1;
    }

    const size_t amount = count < pipe_state->count ? count : pipe_state->count;
    uint8_t *dst = (uint8_t *)buffer;
    for (size_t i = 0; i < amount; ++i) {
        dst[i] = pipe_state->data[pipe_state->head];
        pipe_state->head = (pipe_state->head + 1u) % SSRNX_VPIPE_CAPACITY;
    }
    pipe_state->count -= amount;
    pthread_mutex_unlock(&fd_compat_g_pipe_lock);
    return (ssize_t)amount;
}

ssize_t ssrnx_fd_write(int fd, const void *buffer, size_t count) {
    if (ssrnx_eventfd_poll && ssrnx_eventfd_poll(fd, 0) >= 0) return ssrnx_eventfd_write(fd, buffer, count);
    size_t random_slot = 0;
    if (fd_compat_decode_random_fd(fd, &random_slot)) { (void)buffer; (void)count; errno = EBADF; return -1; }
    size_t slot = 0;
    bool write_end = false;
    if (!fd_compat_decode_fd(fd, &slot, &write_end) || !write_end) {
        errno = EBADF;
        return -1;
    }
    if (!buffer && count != 0) {
        errno = EFAULT;
        return -1;
    }
    if (count == 0) return 0;

    pthread_mutex_lock(&fd_compat_g_pipe_lock);
    fd_compat_SsrnxVirtualPipe *pipe_state = &fd_compat_g_pipes[slot];
    if (!pipe_state->used || !pipe_state->write_open) {
        pthread_mutex_unlock(&fd_compat_g_pipe_lock);
        errno = EBADF;
        return -1;
    }
    if (!pipe_state->read_open) {
        pthread_mutex_unlock(&fd_compat_g_pipe_lock);
        errno = EPIPE;
        return -1;
    }
    const size_t space = SSRNX_VPIPE_CAPACITY - pipe_state->count;
    if (space == 0) {
        pthread_mutex_unlock(&fd_compat_g_pipe_lock);
        errno = EAGAIN;
        return -1;
    }

    const size_t amount = count < space ? count : space;
    const uint8_t *src = (const uint8_t *)buffer;
    size_t tail = (pipe_state->head + pipe_state->count) % SSRNX_VPIPE_CAPACITY;
    for (size_t i = 0; i < amount; ++i) {
        pipe_state->data[tail] = src[i];
        tail = (tail + 1u) % SSRNX_VPIPE_CAPACITY;
    }
    pipe_state->count += amount;
    pthread_mutex_unlock(&fd_compat_g_pipe_lock);
    return (ssize_t)amount;
}

int ssrnx_fd_close(int fd) {
    if (ssrnx_eventfd_poll && ssrnx_eventfd_poll(fd, 0) >= 0) return ssrnx_eventfd_close(fd);
    size_t random_slot = 0;
    if (fd_compat_decode_random_fd(fd, &random_slot)) {
        pthread_mutex_lock(&fd_compat_g_pipe_lock);
        if (!fd_compat_g_random_fds[random_slot]) { pthread_mutex_unlock(&fd_compat_g_pipe_lock); errno = EBADF; return -1; }
        fd_compat_g_random_fds[random_slot] = false;
        pthread_mutex_unlock(&fd_compat_g_pipe_lock);
        return 0;
    }
    size_t slot = 0;
    bool write_end = false;
    if (!fd_compat_decode_fd(fd, &slot, &write_end)) {
        errno = EBADF;
        return -1;
    }
    pthread_mutex_lock(&fd_compat_g_pipe_lock);
    fd_compat_SsrnxVirtualPipe *pipe_state = &fd_compat_g_pipes[slot];
    bool *open_flag = write_end ? &pipe_state->write_open : &pipe_state->read_open;
    if (!pipe_state->used || !*open_flag) {
        pthread_mutex_unlock(&fd_compat_g_pipe_lock);
        errno = EBADF;
        return -1;
    }
    *open_flag = false;
    if (!pipe_state->read_open && !pipe_state->write_open) {
        memset(pipe_state, 0, sizeof(*pipe_state));
    }
    pthread_mutex_unlock(&fd_compat_g_pipe_lock);
    return 0;
}

short ssrnx_fd_poll_revents(int fd, short requested_events) {
    if (ssrnx_eventfd_poll) {
        const int events = ssrnx_eventfd_poll(fd, requested_events);
        if (events >= 0) return (short)events;
    }
    size_t random_slot = 0;
    if (fd_compat_decode_random_fd(fd, &random_slot)) {
        pthread_mutex_lock(&fd_compat_g_pipe_lock);
        const bool valid = fd_compat_g_random_fds[random_slot];
        pthread_mutex_unlock(&fd_compat_g_pipe_lock);
        if (!valid) return POLLNVAL;
        return requested_events & (POLLIN | POLLRDNORM);
    }
    size_t slot = 0;
    bool write_end = false;
    if (!fd_compat_decode_fd(fd, &slot, &write_end)) return POLLNVAL;

    pthread_mutex_lock(&fd_compat_g_pipe_lock);
    fd_compat_SsrnxVirtualPipe *pipe_state = &fd_compat_g_pipes[slot];
    short revents = 0;
    if (!pipe_state->used || (write_end ? !pipe_state->write_open : !pipe_state->read_open)) {
        revents = POLLNVAL;
    } else if (write_end) {
        if (!pipe_state->read_open) revents |= POLLERR;
        if ((requested_events & POLLOUT) && pipe_state->read_open &&
            pipe_state->count < SSRNX_VPIPE_CAPACITY) revents |= POLLOUT;
    } else {
        if ((requested_events & POLLIN) && pipe_state->count > 0) revents |= POLLIN;
        if (!pipe_state->write_open) revents |= POLLHUP;
    }
    pthread_mutex_unlock(&fd_compat_g_pipe_lock);
    return revents;
}

void ssrnx_fd_reset(void) {
    pthread_mutex_lock(&fd_compat_g_pipe_lock);
    memset(fd_compat_g_pipes, 0, sizeof(fd_compat_g_pipes));
    memset(fd_compat_g_random_fds, 0, sizeof(fd_compat_g_random_fds));
    pthread_mutex_unlock(&fd_compat_g_pipe_lock);
}

#pragma pop_macro("SSRNX_VRANDOM_MAX")
#pragma pop_macro("SSRNX_VRANDOM_BASE")
#pragma pop_macro("SSRNX_VPIPE_MAX")
#pragma pop_macro("SSRNX_VPIPE_CAPACITY")
#pragma pop_macro("SSRNX_VFD_BASE")


/* ---- epoll_fd ---- */
#pragma push_macro("EPOLL_ONESHOT")

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef __SWITCH__
#include <sys/iosupport.h>
#endif

extern bool ssrnx_jni_process_exit_requested(char *,size_t) __attribute__((weak));
enum { EPOLL_FDS=1024, EPOLL_WATCHES=1024, EPOLL_CLOEXEC=0x80000 };
#define EPOLL_ONESHOT UINT32_C(0x40000000)
typedef struct epoll_fd_EpollWatch {
    int key;
    uint64_t identity, serial;
    uint32_t events;
    uint64_t data;
    bool disabled;
    struct epoll_fd_EpollWatch *next;
} epoll_fd_EpollWatch;
typedef struct epoll_fd_EpollSet {
    unsigned references, count;
    int status_flags;
    uint64_t cursor;
    epoll_fd_EpollWatch *watches;
} epoll_fd_EpollSet;
typedef struct epoll_fd_EpollDescriptor { epoll_fd_EpollSet *set; bool cloexec; } epoll_fd_EpollDescriptor;
typedef struct epoll_fd_EpollAlias { int fd; uint64_t identity; struct epoll_fd_EpollAlias *next; } epoll_fd_EpollAlias;
static epoll_fd_EpollDescriptor epoll_fd_g_epoll_fds[EPOLL_FDS];
static epoll_fd_EpollAlias *epoll_fd_g_epoll_aliases;
static uint64_t epoll_fd_g_epoll_serial;
static pthread_mutex_t epoll_fd_g_epoll_lock=PTHREAD_MUTEX_INITIALIZER;
#ifdef __SWITCH__
static pthread_mutex_t epoll_fd_g_epoll_device_lock=PTHREAD_MUTEX_INITIALIZER;
#endif
static epoll_fd_EpollSet *epoll_fd_epoll_set(int fd) { return (unsigned)fd<EPOLL_FDS ? epoll_fd_g_epoll_fds[fd].set : NULL; }
static epoll_fd_EpollAlias *epoll_fd_epoll_alias(int fd) {
    for (epoll_fd_EpollAlias *a=epoll_fd_g_epoll_aliases;a;a=a->next) if (a->fd==fd) return a;
    return NULL;
}
static int epoll_fd_epoll_alias_fd(uint64_t identity) {
    for (epoll_fd_EpollAlias *a=epoll_fd_g_epoll_aliases;a;a=a->next) if (a->identity==identity) return a->fd;
    return -1;
}
static epoll_fd_EpollAlias *epoll_fd_epoll_alias_ensure(int fd) {
    epoll_fd_EpollAlias *a=epoll_fd_epoll_alias(fd);
    if (!a) {
        a=calloc(1,sizeof(*a));
        if (a) { a->fd=fd;a->identity=++epoll_fd_g_epoll_serial;a->next=epoll_fd_g_epoll_aliases;epoll_fd_g_epoll_aliases=a; }
    }
    return a;
}
static void epoll_fd_epoll_release(epoll_fd_EpollSet *set) {
    if (--set->references) return;
    while (set->watches) { epoll_fd_EpollWatch *w=set->watches;set->watches=w->next;free(w); }
    free(set);
}
void ssrnx_epoll_closed(int fd) {
    const int saved=errno;
    pthread_mutex_lock(&epoll_fd_g_epoll_lock);
    epoll_fd_EpollAlias **link=&epoll_fd_g_epoll_aliases;
    while (*link && (*link)->fd!=fd) link=&(*link)->next;
    if (*link) {
        epoll_fd_EpollAlias *a=*link;const uint64_t identity=a->identity;*link=a->next;free(a);
        if (epoll_fd_epoll_alias_fd(identity)<0) for (unsigned i=0;i<EPOLL_FDS;++i) if (epoll_fd_g_epoll_fds[i].set) {
            epoll_fd_EpollSet *s=epoll_fd_g_epoll_fds[i].set;epoll_fd_EpollWatch **watch=&s->watches;
            while (*watch) {
                if ((*watch)->identity==identity) { epoll_fd_EpollWatch *w=*watch;*watch=w->next;free(w);--s->count; }
                else watch=&(*watch)->next;
            }
        }
    }
    pthread_mutex_unlock(&epoll_fd_g_epoll_lock);errno=saved;
}
void ssrnx_epoll_duplicated(int oldfd,int newfd) {
    if (oldfd==newfd) return;
    const int saved=errno;
    ssrnx_epoll_closed(newfd);
    pthread_mutex_lock(&epoll_fd_g_epoll_lock);
    epoll_fd_EpollAlias *old=epoll_fd_epoll_alias_ensure(oldfd);
    epoll_fd_EpollAlias *next=old ? epoll_fd_epoll_alias_ensure(newfd) : NULL;
    if (next) next->identity=old->identity;
    pthread_mutex_unlock(&epoll_fd_g_epoll_lock);errno=saved;
}
static void epoll_fd_epoll_detach(int fd) {
    ssrnx_epoll_closed(fd);
    pthread_mutex_lock(&epoll_fd_g_epoll_lock);
    epoll_fd_EpollSet *set=epoll_fd_epoll_set(fd);
    if (set) { memset(&epoll_fd_g_epoll_fds[fd],0,sizeof(epoll_fd_g_epoll_fds[fd]));epoll_fd_epoll_release(set); }
    pthread_mutex_unlock(&epoll_fd_g_epoll_lock);
}
#ifdef __SWITCH__
typedef struct epoll_fd_EpollHandle { int fd; } epoll_fd_EpollHandle;
static int epoll_fd_epoll_open(struct _reent *r,void *data,const char *path,int flags,int mode) {
    (void)r;(void)path;(void)flags;(void)mode;((epoll_fd_EpollHandle *)data)->fd=-1;return 0;
}
static int epoll_fd_epoll_close(struct _reent *r,void *data) { (void)r;epoll_fd_epoll_detach(((epoll_fd_EpollHandle *)data)->fd);return 0; }
static ssize_t epoll_fd_epoll_read(struct _reent *r,void *data,char *p,size_t n) { (void)data;(void)p;(void)n;r->_errno=EINVAL;return -1; }
static ssize_t epoll_fd_epoll_write(struct _reent *r,void *data,const char *p,size_t n) { (void)data;(void)p;(void)n;r->_errno=EINVAL;return -1; }
static off_t epoll_fd_epoll_seek(struct _reent *r,void *data,off_t off,int whence) { (void)r;(void)data;(void)off;(void)whence;return 0; }
static int epoll_fd_epoll_fstat(struct _reent *r,void *data,struct stat *out) {
    if (ssrnx_epoll_stat(((epoll_fd_EpollHandle *)data)->fd,out)==0) return 0;
    r->_errno=EBADF;return -1;
}
static int epoll_fd_epoll_sync(struct _reent *r,void *data) { (void)data;r->_errno=EINVAL;return -1; }
static const devoptab_t epoll_fd_g_epoll_device={.name="apn64epoll",.structSize=sizeof(epoll_fd_EpollHandle),
    .open_r=epoll_fd_epoll_open,.close_r=epoll_fd_epoll_close,.read_r=epoll_fd_epoll_read,.write_r=epoll_fd_epoll_write,
    .seek_r=epoll_fd_epoll_seek,.fstat_r=epoll_fd_epoll_fstat,.fsync_r=epoll_fd_epoll_sync};
#endif
static int epoll_fd_epoll_attach(epoll_fd_EpollSet *set,int minimum,bool cloexec) {
    int held[EPOLL_FDS],count=0,fd=-1,error=EMFILE;
    for (;;) {
#ifdef __SWITCH__
        fd=open("apn64epoll:",O_RDWR);
#else
        fd=open("/dev/null",O_RDWR); /* Host fixtures use the same set engine. */
#endif
        if (fd<0) { error=errno;break; }
        if (fd>=EPOLL_FDS) { close(fd);fd=-1;break; }
        if (fd>=minimum) break;
        held[count++]=fd;
    }
    if (fd>=0) {
        pthread_mutex_lock(&epoll_fd_g_epoll_lock);epoll_fd_g_epoll_fds[fd]=(epoll_fd_EpollDescriptor){set,cloexec};pthread_mutex_unlock(&epoll_fd_g_epoll_lock);
#ifdef __SWITCH__
        ((epoll_fd_EpollHandle *)__get_handle(fd)->fileStruct)->fd=fd;
#endif
    } else { pthread_mutex_lock(&epoll_fd_g_epoll_lock);epoll_fd_epoll_release(set);pthread_mutex_unlock(&epoll_fd_g_epoll_lock); }
    while (count) close(held[--count]);
    if (fd<0) errno=error;
    return fd;
}
int ssrnx_epoll_create(int flags) {
    if (flags & ~EPOLL_CLOEXEC) { errno=EINVAL;return -1; }
#ifdef __SWITCH__
    pthread_mutex_lock(&epoll_fd_g_epoll_device_lock);
    int device=FindDevice("apn64epoll:");if (device<0) device=AddDevice(&epoll_fd_g_epoll_device);
    pthread_mutex_unlock(&epoll_fd_g_epoll_device_lock);
    if (device<0) return -1;
#endif
    epoll_fd_EpollSet *set=calloc(1,sizeof(*set));if (!set) { errno=ENOMEM;return -1; }
    set->references=1;return epoll_fd_epoll_attach(set,0,(flags&EPOLL_CLOEXEC)!=0);
}
int ssrnx_epoll_control(int fd,int command,int argument) {
    pthread_mutex_lock(&epoll_fd_g_epoll_lock);epoll_fd_EpollSet *s=epoll_fd_epoll_set(fd);int result=-2,error=0;
    if (s) switch (command) {
        case 1:result=epoll_fd_g_epoll_fds[fd].cloexec;break;
        case 2:epoll_fd_g_epoll_fds[fd].cloexec=(argument&1)!=0;result=0;break;
        case 3:result=2|s->status_flags;break;
        case 4:s->status_flags=argument&0xc00;result=0;break; /* O_APPEND/O_NONBLOCK shared by dup. */
        default:error=EINVAL;result=-1;break;
    }
    pthread_mutex_unlock(&epoll_fd_g_epoll_lock);if (error) errno=error;return result;
}
int ssrnx_epoll_duplicate(int fd,int minimum,bool cloexec) {
    if (minimum<0) { errno=EINVAL;return -1; }
    pthread_mutex_lock(&epoll_fd_g_epoll_lock);epoll_fd_EpollSet *s=epoll_fd_epoll_set(fd);if (s) ++s->references;pthread_mutex_unlock(&epoll_fd_g_epoll_lock);
    return s ? epoll_fd_epoll_attach(s,minimum,cloexec) : -2;
}
int ssrnx_epoll_stat(int fd,struct stat *out) {
    pthread_mutex_lock(&epoll_fd_g_epoll_lock);bool found=epoll_fd_epoll_set(fd)!=NULL;
    if (found) { memset(out,0,sizeof(*out));out->st_mode=0600;out->st_nlink=1;out->st_blksize=4096; }
    pthread_mutex_unlock(&epoll_fd_g_epoll_lock);return found ? 0 : -2;
}
int ssrnx_epoll_ctl(int epfd,int op,int fd,const SsrnxEpollEvent *event) {
    pthread_mutex_lock(&epoll_fd_g_epoll_lock);epoll_fd_EpollSet *s=epoll_fd_epoll_set(epfd);int error=0;
    if (!s) { errno=fcntl(epfd,F_GETFD)<0 ? EBADF : EINVAL;pthread_mutex_unlock(&epoll_fd_g_epoll_lock);return -1; }
    if (fd<0) error=EBADF;
    else if (epoll_fd_epoll_set(fd)==s) error=EINVAL;
    else if (op!=1 && op!=2 && op!=3) error=EINVAL;
    else if (op!=2 && !event) error=EFAULT;
    /* Poll cannot recreate kernel edge queues or nested-epoll graphs. Fail
     * explicitly instead of silently providing level semantics for EPOLLET. */
    else if (epoll_fd_epoll_set(fd)) error=EOPNOTSUPP;
    else if (op!=2 && (event->events & ~(UINT32_C(0x1f)|EPOLL_ONESHOT))) error=EOPNOTSUPP;
    epoll_fd_EpollAlias *alias=error ? NULL : epoll_fd_epoll_alias_ensure(fd);
    if (!error && !alias) error=ENOMEM;
    epoll_fd_EpollWatch **link=&s->watches;
    if (alias) while (*link && ((*link)->key!=fd || (*link)->identity!=alias->identity)) link=&(*link)->next;
    if (!error && op==1 && *link) error=EEXIST;
    if (!error && op!=1 && !*link) error=ENOENT;
    if (!error && op==1) {
        if (s->count==EPOLL_WATCHES) error=ENOSPC;
        else { *link=calloc(1,sizeof(**link));if (!*link) error=ENOMEM;else {++s->count;(*link)->key=fd;(*link)->identity=alias->identity;} }
    }
    if (!error) {
        if (op==2) { epoll_fd_EpollWatch *w=*link;*link=w->next;free(w);--s->count; }
        else {
            epoll_fd_EpollWatch *w=*link;w->events=event->events;w->data=event->data;w->serial=++epoll_fd_g_epoll_serial;w->disabled=false;
            if (op==3 && w->next) { *link=w->next;while (*link) link=&(*link)->next;*link=w;w->next=NULL; }
        }
    }
    pthread_mutex_unlock(&epoll_fd_g_epoll_lock);if (error) { errno=error;return -1; }return 0;
}
static uint64_t epoll_fd_epoll_ms(void) {
    struct timespec t={0};clock_gettime(CLOCK_MONOTONIC,&t);
    return (uint64_t)t.tv_sec*1000+(uint64_t)t.tv_nsec/1000000;
}
static int epoll_fd_epoll_collect(epoll_fd_EpollSet *s,SsrnxEpollEvent *events,int maximum,SsrnxEpollPoll poller,bool consume) {
    /* Snapshot across host polling; callbacks never run under the registry
     * lock. Serials reject stale events after concurrent MOD/DEL/close. */
    typedef struct Probe { uint64_t serial; } Probe;
    const unsigned capacity=s->count;
    if (!capacity) return 0;
    Probe *probes=calloc(capacity,sizeof(*probes));
    SsrnxEpollPollfd *fds=calloc(capacity,sizeof(*fds));
    if (!probes || !fds) { free(probes);free(fds);errno=ENOMEM;return -1; }
    unsigned used=0;
    const uint64_t cursor=s->cursor;
    for (unsigned pass=0;pass<2;++pass) for (epoll_fd_EpollWatch *w=s->watches;w;w=w->next) {
        if (w->disabled || (pass==0 ? w->serial<=cursor : w->serial>cursor)) continue;
        const int fd=epoll_fd_epoll_alias_fd(w->identity);if (fd<0) continue;
        probes[used].serial=w->serial;fds[used++]=(SsrnxEpollPollfd){fd,(int16_t)(w->events&0x1f),0};
    }
    pthread_mutex_unlock(&epoll_fd_g_epoll_lock);
    const int rc=poller(fds,used,0),error=errno;
    pthread_mutex_lock(&epoll_fd_g_epoll_lock);
    int count=0;
    if (rc>=0) for (unsigned i=0;i<used && count<maximum;++i) if (fds[i].revents) {
        for (epoll_fd_EpollWatch *w=s->watches;w;w=w->next) if (w->serial==probes[i].serial && !w->disabled) {
            const uint32_t ready=(uint16_t)fds[i].revents & (w->events|0x18);
            if (ready) {
                events[count++]=(SsrnxEpollEvent){ready,0,w->data};
                if (consume) { s->cursor=w->serial;if (w->events&EPOLL_ONESHOT) w->disabled=true; }
            }
            break;
        }
    }
    free(probes);free(fds);errno=error;return rc<0 ? -1 : count;
}
int ssrnx_epoll_wait(int epfd,SsrnxEpollEvent *events,int maximum,int timeout,SsrnxEpollPoll poller) {
    if (maximum<=0 || maximum>INT_MAX/(int)sizeof(*events)) { errno=EINVAL;return -1; }
    if (!events || !poller) { errno=EFAULT;return -1; }
    const uint64_t begin=epoll_fd_epoll_ms();
    pthread_mutex_lock(&epoll_fd_g_epoll_lock);epoll_fd_EpollSet *s=epoll_fd_epoll_set(epfd);
    if (!s) { pthread_mutex_unlock(&epoll_fd_g_epoll_lock);errno=fcntl(epfd,F_GETFD)<0 ? EBADF : EINVAL;return -1; }
    ++s->references;
    int result;
    for (;;) {
        result=epoll_fd_epoll_collect(s,events,maximum,poller,true);
        if (result || timeout==0) break;
        const uint64_t elapsed=epoll_fd_epoll_ms()-begin;
        if (timeout>0 && elapsed>=(uint64_t)timeout) break;
        pthread_mutex_unlock(&epoll_fd_g_epoll_lock);
        if (ssrnx_jni_process_exit_requested && ssrnx_jni_process_exit_requested(NULL,0)) { errno=EINTR;result=-1; }
        else result=poller(NULL,0,1); /* Bounded sleep lets ADD/MOD/virtual wakeups progress. */
        const int error=errno;
        pthread_mutex_lock(&epoll_fd_g_epoll_lock);
        if (result<0) { errno=error;break; }
        if (epoll_fd_epoll_set(epfd)!=s) { errno=EBADF;result=-1;break; }
    }
    const int error=errno;
    epoll_fd_epoll_release(s);pthread_mutex_unlock(&epoll_fd_g_epoll_lock);errno=error;return result;
}
int ssrnx_epoll_poll(int fd,short events,SsrnxEpollPoll poller) {
    pthread_mutex_lock(&epoll_fd_g_epoll_lock);epoll_fd_EpollSet *s=epoll_fd_epoll_set(fd);int result=-1;
    if (s) {
        result=0;
        if (events&POLLIN) { ++s->references;SsrnxEpollEvent event;const int ready=epoll_fd_epoll_collect(s,&event,1,poller,false);result=ready>0 ? POLLIN : 0;epoll_fd_epoll_release(s); }
    }
    pthread_mutex_unlock(&epoll_fd_g_epoll_lock);return result;
}
void ssrnx_epoll_reset(void) {
    for (int fd=0;fd<EPOLL_FDS;++fd) {
        pthread_mutex_lock(&epoll_fd_g_epoll_lock);const bool owned=epoll_fd_epoll_set(fd)!=NULL;pthread_mutex_unlock(&epoll_fd_g_epoll_lock);
        if (owned) { epoll_fd_epoll_detach(fd);close(fd); }
    }
    pthread_mutex_lock(&epoll_fd_g_epoll_lock);
    while (epoll_fd_g_epoll_aliases) { epoll_fd_EpollAlias *a=epoll_fd_g_epoll_aliases;epoll_fd_g_epoll_aliases=a->next;free(a); }
    pthread_mutex_unlock(&epoll_fd_g_epoll_lock);
}

#pragma pop_macro("EPOLL_ONESHOT")


/* ---- event_fd ---- */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef __SWITCH__
#include <sys/iosupport.h>
extern int ssrnx_signal_safe_cond_timedwait(pthread_cond_t *, pthread_mutex_t *, const struct timespec *) __attribute__((weak));
extern bool ssrnx_jni_process_exit_requested(char *, size_t) __attribute__((weak));

typedef struct event_fd_EventCounter {
    uint64_t count;
    unsigned references, flags;
    bool shutdown;
    bool timer;
    int clock;
    uint64_t deadline, interval;
} event_fd_EventCounter;
typedef struct event_fd_EventDescriptor { event_fd_EventCounter *counter; bool cloexec; } event_fd_EventDescriptor;
typedef struct event_fd_EventHandle { int fd; } event_fd_EventHandle;
enum { EVENT_SEMAPHORE = 1, EVENT_NONBLOCK = 0x800, EVENT_CLOEXEC = 0x80000, EVENT_FDS = 1024 };
static event_fd_EventDescriptor event_fd_g_event_fds[EVENT_FDS];
static pthread_mutex_t event_fd_g_event_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t event_fd_g_event_device_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t event_fd_g_event_changed = PTHREAD_COND_INITIALIZER;
static uint64_t event_fd_event_timer_now(int clock) {
    struct timespec t={0};
    if (clock_gettime(clock==0 ? CLOCK_REALTIME : CLOCK_MONOTONIC,&t)!=0) return 0;
    return (uint64_t)t.tv_sec*UINT64_C(1000000000)+(uint64_t)t.tv_nsec;
}
static void event_fd_event_timer_update(event_fd_EventCounter *counter) {
    if (!counter->timer || !counter->deadline) return;
    const uint64_t now=event_fd_event_timer_now(counter->clock);
    if (now<counter->deadline) return;
    const uint64_t expired=counter->interval ? (now-counter->deadline)/counter->interval+1 : 1;
    counter->count=expired; /* Reading clears accumulated expirations. */
}

static event_fd_EventCounter *event_fd_event_counter_locked(int fd) {
    return (unsigned)fd < EVENT_FDS ? event_fd_g_event_fds[fd].counter : NULL;
}
static void event_fd_event_release_locked(event_fd_EventCounter *counter) {
    if (!--counter->references) free(counter);
}

static ssize_t event_fd_event_transfer(int fd, void *buffer, size_t size, bool writing) {
    if (size < 8) { errno = EINVAL; return -1; }
    if (!buffer) { errno = EFAULT; return -1; }
    uint64_t value = 0;
    if (writing) {
        memcpy(&value, buffer, 8);
        if (value == UINT64_MAX) { errno = EINVAL; return -1; }
    }
    pthread_mutex_lock(&event_fd_g_event_lock);
    event_fd_EventCounter *counter = event_fd_event_counter_locked(fd);
    if (!counter) { pthread_mutex_unlock(&event_fd_g_event_lock); errno = EBADF; return -1; }
    if (writing && counter->timer) { pthread_mutex_unlock(&event_fd_g_event_lock);errno=EINVAL;return -1; }
    event_fd_event_timer_update(counter);
    ++counter->references; /* Blocking I/O survives descriptor close/reuse. */
    const bool nonblock = (counter->flags & EVENT_NONBLOCK) != 0;
    int error = 0;
    while (writing ? value > UINT64_MAX - 1 - counter->count : counter->count == 0) {
        if (counter->shutdown) { error = EINTR; break; }
        if (nonblock) { error = EAGAIN; break; }
        pthread_mutex_unlock(&event_fd_g_event_lock);
        const bool exiting = ssrnx_jni_process_exit_requested && ssrnx_jni_process_exit_requested(NULL, 0);
        pthread_mutex_lock(&event_fd_g_event_lock);
        if (exiting || counter->shutdown) { error = EINTR; break; }
        /* Recheck after dropping the lock so a producer wake cannot be lost. */
        if (writing ? value <= UINT64_MAX - 1 - counter->count : counter->count != 0) break;
        struct timespec deadline;
        if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) { error = errno; break; }
        deadline.tv_nsec += 50000000;
        if (deadline.tv_nsec >= 1000000000) { ++deadline.tv_sec; deadline.tv_nsec -= 1000000000; }
        const int rc = ssrnx_signal_safe_cond_timedwait
            ? ssrnx_signal_safe_cond_timedwait(&event_fd_g_event_changed, &event_fd_g_event_lock, &deadline)
            : pthread_cond_timedwait(&event_fd_g_event_changed, &event_fd_g_event_lock, &deadline);
        if (rc && rc != ETIMEDOUT) { error = rc; break; }
        event_fd_event_timer_update(counter);
    }
    if (!error) {
        if (writing) counter->count += value;
        else {
            value = (counter->flags & EVENT_SEMAPHORE) ? 1 : counter->count;
            counter->count -= value;
            if (counter->timer) {
                if (counter->interval && value<=(UINT64_MAX-counter->deadline)/counter->interval)
                    counter->deadline+=value*counter->interval;
                else counter->deadline=0;
            }
            memcpy(buffer, &value, 8);
        }
        pthread_cond_broadcast(&event_fd_g_event_changed);
    }
    event_fd_event_release_locked(counter);
    pthread_mutex_unlock(&event_fd_g_event_lock);
    if (error) { errno = error; return -1; }
    return 8;
}
ssize_t ssrnx_eventfd_read(int fd, void *buffer, size_t size) { return event_fd_event_transfer(fd, buffer, size, false); }
ssize_t ssrnx_eventfd_write(int fd, const void *buffer, size_t size) { return event_fd_event_transfer(fd, (void *)buffer, size, true); }

int ssrnx_eventfd_poll(int fd, short events) {
    pthread_mutex_lock(&event_fd_g_event_lock);
    event_fd_EventCounter *counter = event_fd_event_counter_locked(fd);
    if (counter) event_fd_event_timer_update(counter);
    int result = -1;
    if (counter) result = ((counter->count ? POLLIN : 0) |
                          (!counter->timer && counter->count < UINT64_MAX - 1 ? POLLOUT : 0)) & events;
    pthread_mutex_unlock(&event_fd_g_event_lock);
    return result;
}
int ssrnx_eventfd_control(int fd, int command, int argument) {
    pthread_mutex_lock(&event_fd_g_event_lock);
    event_fd_EventCounter *counter = event_fd_event_counter_locked(fd);
    int result = -2, error = 0;
    if (counter) {
        switch (command) {
            case 1: result = event_fd_g_event_fds[fd].cloexec ? 1 : 0; break; /* F_GETFD */
            case 2: event_fd_g_event_fds[fd].cloexec = (argument & 1) != 0; result = 0; break;
            case 3: result = 2 | (counter->flags & EVENT_NONBLOCK); break; /* F_GETFL: O_RDWR */
            case 4: counter->flags = (counter->flags & ~EVENT_NONBLOCK) | (argument & EVENT_NONBLOCK); result = 0; break;
            default: result = -1; error = EINVAL; break;
        }
    }
    pthread_mutex_unlock(&event_fd_g_event_lock);
    if (error) errno = error;
    return result;
}
int ssrnx_eventfd_stat(int fd, struct stat *out) {
    pthread_mutex_lock(&event_fd_g_event_lock);
    const bool found = event_fd_event_counter_locked(fd) != NULL;
    if (found) {
        memset(out, 0, sizeof(*out));
        out->st_mode = 0600; /* Linux anon_inode has no regular/pipe file type. */
        out->st_dev = 0xefd; out->st_ino = 1; out->st_nlink = 1; out->st_blksize = 4096;
    }
    pthread_mutex_unlock(&event_fd_g_event_lock);
    return found ? 0 : -2;
}

static int event_fd_event_open(struct _reent *r, void *data, const char *path, int flags, int mode) {
    (void)r; (void)path; (void)flags; (void)mode;
    ((event_fd_EventHandle *)data)->fd = -1;
    return 0;
}
static int event_fd_event_close(struct _reent *r, void *data) {
    (void)r;
    const int fd = ((event_fd_EventHandle *)data)->fd;
    extern void ssrnx_epoll_closed(int) __attribute__((weak));
    if (ssrnx_epoll_closed) ssrnx_epoll_closed(fd);
    pthread_mutex_lock(&event_fd_g_event_lock);
    event_fd_EventCounter *counter = event_fd_event_counter_locked(fd);
    if (counter) {
        memset(&event_fd_g_event_fds[fd], 0, sizeof(event_fd_g_event_fds[fd]));
        event_fd_event_release_locked(counter);
    }
    pthread_mutex_unlock(&event_fd_g_event_lock);
    return 0;
}
static ssize_t event_fd_event_read(struct _reent *r, void *data, char *buffer, size_t size) {
    const ssize_t rc = ssrnx_eventfd_read(((event_fd_EventHandle *)data)->fd, buffer, size);
    if (rc < 0) r->_errno = errno;
    return rc;
}
static ssize_t event_fd_event_write(struct _reent *r, void *data, const char *buffer, size_t size) {
    const ssize_t rc = ssrnx_eventfd_write(((event_fd_EventHandle *)data)->fd, buffer, size);
    if (rc < 0) r->_errno = errno;
    return rc;
}
static int event_fd_event_stat(struct _reent *r, void *data, struct stat *out) {
    const int rc = ssrnx_eventfd_stat(((event_fd_EventHandle *)data)->fd, out);
    if (rc == -2) { r->_errno = EBADF; return -1; }
    return rc;
}
static off_t event_fd_event_seek(struct _reent *r, void *data, off_t offset, int whence) {
    (void)r; (void)data; (void)offset; (void)whence;
    return 0; /* Linux 5.4 eventfd uses noop_llseek. */
}
static int event_fd_event_sync(struct _reent *r, void *data) { (void)data; r->_errno = EINVAL; return -1; }
static const devoptab_t event_fd_g_event_device = {
    .name = "apn64event", .structSize = sizeof(event_fd_EventHandle), .open_r = event_fd_event_open, .close_r = event_fd_event_close,
    .read_r = event_fd_event_read, .write_r = event_fd_event_write, .seek_r = event_fd_event_seek, .fstat_r = event_fd_event_stat, .fsync_r = event_fd_event_sync,
};

/* counter owns a reference on entry; it transfers to the new descriptor. */
static int event_fd_event_attach(event_fd_EventCounter *counter, int minimum, bool cloexec) {
    int held[EVENT_FDS], count = 0, fd = -1, error = EMFILE;
    for (;;) {
        fd = open("apn64event:", O_RDWR);
        if (fd < 0) { error = errno; break; }
        if (fd >= EVENT_FDS) { close(fd); fd = -1; break; }
        if (fd >= minimum) break;
        held[count++] = fd;
    }
    if (fd >= 0) {
        event_fd_EventHandle *handle = __get_handle(fd)->fileStruct;
        pthread_mutex_lock(&event_fd_g_event_lock);
        event_fd_g_event_fds[fd] = (event_fd_EventDescriptor){counter, cloexec};
        handle->fd = fd;
        pthread_mutex_unlock(&event_fd_g_event_lock);
    } else {
        pthread_mutex_lock(&event_fd_g_event_lock); event_fd_event_release_locked(counter); pthread_mutex_unlock(&event_fd_g_event_lock);
    }
    while (count) close(held[--count]);
    if (fd < 0) errno = error;
    return fd;
}
int ssrnx_eventfd_create(unsigned initial, int flags) {
    if (flags & ~(EVENT_NONBLOCK | EVENT_CLOEXEC | EVENT_SEMAPHORE)) { errno = EINVAL; return -1; }
    event_fd_EventCounter *counter = calloc(1, sizeof(*counter));
    if (!counter) { errno = ENOMEM; return -1; }
    *counter = (event_fd_EventCounter){.count = initial, .references = 1, .flags = (unsigned)flags & ~EVENT_CLOEXEC};
    pthread_mutex_lock(&event_fd_g_event_device_lock);
    int device = FindDevice("apn64event:");
    if (device < 0) device = AddDevice(&event_fd_g_event_device);
    const int error = errno;
    pthread_mutex_unlock(&event_fd_g_event_device_lock);
    if (device < 0) { free(counter); errno = error; return -1; }
    return event_fd_event_attach(counter, 0, (flags & EVENT_CLOEXEC) != 0);
}
static bool event_fd_event_timer_ns(int64_t sec,int64_t nsec,uint64_t *out) {
    if (sec<0 || nsec<0 || nsec>=1000000000 || (uint64_t)sec>(UINT64_MAX-(uint64_t)nsec)/1000000000) return false;
    *out=(uint64_t)sec*1000000000+(uint64_t)nsec;return true;
}
static void event_fd_event_timer_spec(event_fd_EventCounter *counter,SsrnxTimerSpec *out) {
    event_fd_event_timer_update(counter);
    uint64_t remaining=0;
    if (counter->deadline) {
        const uint64_t now=event_fd_event_timer_now(counter->clock);
        if (counter->deadline>now) remaining=counter->deadline-now;
        else if (counter->interval) remaining=counter->interval-(now-counter->deadline)%counter->interval;
    }
    *out=(SsrnxTimerSpec){(int64_t)(counter->interval/1000000000),(int64_t)(counter->interval%1000000000),
                         (int64_t)(remaining/1000000000),(int64_t)(remaining%1000000000)};
}
int ssrnx_timerfd_create(int clock,int flags) {
    /* CLOCK_BOOTTIME needs a suspend-inclusive clock source. */
    if (clock==7) { errno=EOPNOTSUPP;return -1; }
    if (clock!=0 && clock!=1) { errno=(clock==8 || clock==9) ? EPERM : EINVAL;return -1; }
    if (flags&~(EVENT_NONBLOCK|EVENT_CLOEXEC)) { errno=EINVAL;return -1; }
    const int fd=ssrnx_eventfd_create(0,flags);
    if (fd<0) return -1;
    pthread_mutex_lock(&event_fd_g_event_lock);event_fd_EventCounter *counter=event_fd_event_counter_locked(fd);
    counter->timer=true;counter->clock=clock;pthread_mutex_unlock(&event_fd_g_event_lock);return fd;
}
int ssrnx_timerfd_settime(int fd,int flags,const SsrnxTimerSpec *value,SsrnxTimerSpec *old) {
    uint64_t interval,deadline;
    if (!value) { errno=EFAULT;return -1; }
    if (flags&~3 || !event_fd_event_timer_ns(value->interval_sec,value->interval_nsec,&interval) ||
        !event_fd_event_timer_ns(value->value_sec,value->value_nsec,&deadline)) { errno=EINVAL;return -1; }
    if (flags&2) { errno=EOPNOTSUPP;return -1; } /* No realtime clock-change notifications. */
    pthread_mutex_lock(&event_fd_g_event_lock);event_fd_EventCounter *counter=event_fd_event_counter_locked(fd);
    int error=0;
    if (!counter) error=fcntl(fd,F_GETFD)<0 ? EBADF : EINVAL;
    else if (!counter->timer) error=EINVAL;
    else {
        if (!(flags&1) && deadline) {
            const uint64_t now=event_fd_event_timer_now(counter->clock);
            if (deadline>UINT64_MAX-now) error=EINVAL;else deadline+=now;
        }
        if (!error) {
            if (old) event_fd_event_timer_spec(counter,old);
            counter->interval=interval;counter->deadline=deadline;counter->count=0;
            pthread_cond_broadcast(&event_fd_g_event_changed);
        }
    }
    pthread_mutex_unlock(&event_fd_g_event_lock);if (error) { errno=error;return -1; }return 0;
}
int ssrnx_timerfd_gettime(int fd,SsrnxTimerSpec *value) {
    if (!value) { errno=EFAULT;return -1; }
    pthread_mutex_lock(&event_fd_g_event_lock);event_fd_EventCounter *counter=event_fd_event_counter_locked(fd);
    int error=!counter ? (fcntl(fd,F_GETFD)<0 ? EBADF : EINVAL) : !counter->timer ? EINVAL : 0;
    if (!error) event_fd_event_timer_spec(counter,value);
    pthread_mutex_unlock(&event_fd_g_event_lock);if (error) { errno=error;return -1; }return 0;
}
int ssrnx_eventfd_duplicate(int fd, int minimum, bool cloexec) {
    if (minimum < 0) { errno = EINVAL; return -1; }
    pthread_mutex_lock(&event_fd_g_event_lock);
    event_fd_EventCounter *counter = event_fd_event_counter_locked(fd);
    if (counter) ++counter->references;
    pthread_mutex_unlock(&event_fd_g_event_lock);
    return counter ? event_fd_event_attach(counter, minimum, cloexec) : -2;
}
int ssrnx_eventfd_close(int fd) { return close(fd); }
void ssrnx_eventfd_reset(void) {
    int descriptors[EVENT_FDS], count = 0;
    pthread_mutex_lock(&event_fd_g_event_lock);
    for (int fd = 0; fd < EVENT_FDS; ++fd) {
        event_fd_EventCounter *counter = event_fd_event_counter_locked(fd);
        if (counter) { counter->shutdown = true; descriptors[count++] = fd; }
    }
    pthread_cond_broadcast(&event_fd_g_event_changed);
    pthread_mutex_unlock(&event_fd_g_event_lock);
    while (count) close(descriptors[--count]);
}
#else
#ifdef __linux__
#include <sys/eventfd.h>
#include <sys/timerfd.h>
int ssrnx_eventfd_create(unsigned initial, int flags) { return eventfd(initial, flags); }
int ssrnx_timerfd_create(int clock,int flags) { return timerfd_create(clock,flags); }
int ssrnx_timerfd_settime(int fd,int flags,const SsrnxTimerSpec *value,SsrnxTimerSpec *old) {
    if (!value) { errno=EFAULT;return -1; }
    struct itimerspec in={{value->interval_sec,value->interval_nsec},{value->value_sec,value->value_nsec}},previous;
    int rc=timerfd_settime(fd,flags,&in,old ? &previous : NULL);
    if (!rc && old) *old=(SsrnxTimerSpec){previous.it_interval.tv_sec,previous.it_interval.tv_nsec,previous.it_value.tv_sec,previous.it_value.tv_nsec};
    return rc;
}
int ssrnx_timerfd_gettime(int fd,SsrnxTimerSpec *out) {
    if (!out) { errno=EFAULT;return -1; }
    struct itimerspec value;int rc=timerfd_gettime(fd,&value);
    if (!rc) *out=(SsrnxTimerSpec){value.it_interval.tv_sec,value.it_interval.tv_nsec,value.it_value.tv_sec,value.it_value.tv_nsec};
    return rc;
}
#else
int ssrnx_eventfd_create(unsigned initial, int flags) { (void)initial; (void)flags; errno = ENOSYS; return -1; }
int ssrnx_timerfd_create(int clock,int flags) { (void)clock;(void)flags;errno=ENOSYS;return -1; }
int ssrnx_timerfd_settime(int fd,int flags,const SsrnxTimerSpec *value,SsrnxTimerSpec *old) {
    (void)fd;(void)flags;(void)value;(void)old;errno=ENOSYS;return -1;
}
int ssrnx_timerfd_gettime(int fd,SsrnxTimerSpec *value) { (void)fd;(void)value;errno=ENOSYS;return -1; }
#endif
int ssrnx_eventfd_poll(int fd, short events) { (void)fd; (void)events; return -1; }
ssize_t ssrnx_eventfd_read(int fd, void *p, size_t n) { return read(fd, p, n); }
ssize_t ssrnx_eventfd_write(int fd, const void *p, size_t n) { return write(fd, p, n); }
int ssrnx_eventfd_close(int fd) { return close(fd); }
int ssrnx_eventfd_control(int fd, int cmd, int arg) { (void)fd; (void)cmd; (void)arg; return -2; }
int ssrnx_eventfd_duplicate(int fd, int min, bool cloexec) { (void)fd; (void)min; (void)cloexec; return -2; }
int ssrnx_eventfd_stat(int fd, struct stat *out) { (void)fd; (void)out; return -2; }
void ssrnx_eventfd_reset(void) {}
#endif


#endif /* NX_STORAGE_HOST_TEST */
