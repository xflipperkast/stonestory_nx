/* runtime bionic. */
#define _GNU_SOURCE 1
#include "runtime.h"


/* ---- compat ---- */
#pragma push_macro("SSRNX_ANDROID_MSG_CTRUNC")
#pragma push_macro("SSRNX_ANDROID_MSG_DONTWAIT")
#pragma push_macro("SSRNX_ANDROID_MSG_EOR")
#pragma push_macro("SSRNX_ANDROID_MSG_OOB")
#pragma push_macro("SSRNX_ANDROID_MSG_TRUNC")
#pragma push_macro("SSRNX_ANDROID_MSG_WAITFORONE")
#pragma push_macro("SSRNX_ANDROID_SIGABRT")
#pragma push_macro("SSRNX_ANDROID_UIO_MAXIOV")
#pragma push_macro("SSRNX_ARM64_HWCAP")
#pragma push_macro("SSRNX_ATFORK_MAX")
#pragma push_macro("SSRNX_BIONIC_MUTEXATTR_PROTOCOL_MASK")
#pragma push_macro("SSRNX_BIONIC_MUTEXATTR_PROTOCOL_SHIFT")
#pragma push_macro("SSRNX_BIONIC_MUTEXATTR_SHARED_MASK")
#pragma push_macro("SSRNX_BIONIC_MUTEXATTR_TYPE_MASK")
#pragma push_macro("SSRNX_BIONIC_RANDOM_DEG")
#pragma push_macro("SSRNX_BIONIC_RANDOM_SEP")
#pragma push_macro("SSRNX_BIONIC_RTLD_GLOBAL")
#pragma push_macro("SSRNX_BIONIC_RTLD_NOLOAD")
#pragma push_macro("SSRNX_CPU_FREQ_KHZ")
#pragma push_macro("SSRNX_DATA_ADDR")
#pragma push_macro("SSRNX_DLOPEN_GLOBAL_HANDLE")
#pragma push_macro("SSRNX_DLOPEN_VIRTUAL_HANDLE")
#pragma push_macro("SSRNX_FLOCK_MAX")
#pragma push_macro("SSRNX_FNV")
#pragma push_macro("SSRNX_FUNC_ADDR")
#pragma push_macro("SSRNX_GUEST_STDERR_RING_CAP")
#pragma push_macro("SSRNX_MALLINFO_COPY")
#pragma push_macro("SSRNX_MEMTRACE_CALLER")
#pragma push_macro("SSRNX_MEMTRACE_LARGE_THRESHOLD")
#pragma push_macro("SSRNX_MM_MADV_DONTNEED")
#pragma push_macro("SSRNX_MM_MADV_REMOVE")
#pragma push_macro("SSRNX_MM_MAP_ANONYMOUS")
#pragma push_macro("SSRNX_MM_MAP_FIXED")
#pragma push_macro("SSRNX_MM_MAP_FIXED_NOREPLACE")
#pragma push_macro("SSRNX_MM_MAP_PRIVATE")
#pragma push_macro("SSRNX_MM_MAP_SHARED")
#pragma push_macro("SSRNX_MM_MAP_SHARED_VALIDATE")
#pragma push_macro("SSRNX_MM_MAP_TYPE")
#pragma push_macro("SSRNX_MM_PAGE")
#pragma push_macro("SSRNX_MM_PROT_EXEC")
#pragma push_macro("SSRNX_MM_PROT_WRITE")
#pragma push_macro("SSRNX_MREMAP_DONTUNMAP")
#pragma push_macro("SSRNX_MREMAP_FIXED")
#pragma push_macro("SSRNX_MREMAP_MAYMOVE")
#pragma push_macro("SSRNX_NETWORK_SOCKET_TRACE_MAX")
#pragma push_macro("SSRNX_STORAGE_PATH_CAP")
#pragma push_macro("SSRNX_STORAGE_STREAM_MAX")
#pragma push_macro("SSRNX_TMPFILE_CAP")
#pragma push_macro("SSRNX_WX_GROUP_PAGES")
#pragma push_macro("SSRNX_WX_SLOTS")
#pragma push_macro("SSRNX_WX_ZERO_STEP")
#pragma push_macro("MM_FOR_EACH_PAGE")
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif









extern int ssrnx_epoll_create(int) __attribute__((weak));
extern int ssrnx_epoll_ctl(int,int,int,const SsrnxEpollEvent *) __attribute__((weak));
extern int ssrnx_epoll_wait(int,SsrnxEpollEvent *,int,int,SsrnxEpollPoll) __attribute__((weak));
extern int ssrnx_epoll_control(int,int,int) __attribute__((weak));
extern int ssrnx_epoll_duplicate(int,int,bool) __attribute__((weak));
extern int ssrnx_epoll_stat(int,struct stat *) __attribute__((weak));
extern int ssrnx_epoll_poll(int,short,SsrnxEpollPoll) __attribute__((weak));
extern void ssrnx_epoll_closed(int) __attribute__((weak));
extern void ssrnx_epoll_duplicated(int,int) __attribute__((weak));
extern void ssrnx_epoll_reset(void) __attribute__((weak));

extern bool ssrnx_proc_memory_map(const char *, char *, size_t) __attribute__((weak));
extern int ssrnx_eventfd_create(unsigned, int) __attribute__((weak));
extern int ssrnx_eventfd_poll(int, short) __attribute__((weak));
extern int ssrnx_eventfd_control(int, int, int) __attribute__((weak));
extern int ssrnx_eventfd_duplicate(int, int, bool) __attribute__((weak));
extern int ssrnx_eventfd_stat(int, struct stat *) __attribute__((weak));
extern void ssrnx_eventfd_reset(void) __attribute__((weak));











extern int ssrnx_directory_open(const char *, int) __attribute__((weak));

extern void ssrnx_path_lookup_set_root(const char *) __attribute__((weak));
extern bool ssrnx_path_lookup_missing_parent(const char *) __attribute__((weak));
extern void ssrnx_path_lookup_record_missing_parent(const char *) __attribute__((weak));
extern void ssrnx_path_lookup_changed(const char *) __attribute__((weak));



#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <ctype.h>
#include <fnmatch.h>
#include <dirent.h>
#include <elf.h>
#include <math.h>
#include <limits.h>
#include <malloc.h>
#include <locale.h>
#include <netdb.h>
#include <stddef.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>
#include <zlib.h>

#ifdef __SWITCH__
#include <switch.h>

#include <sys/filio.h>
#include <sys/iosupport.h>
#endif

/* compat.c is also linked by focused host ABI tests without diagnostics/log.c.
 * Production provides these symbols; keep diagnostic tracing optional so the
 * compatibility layer does not acquire a new mandatory runtime dependency. */
extern bool ssrnx_diag_full_trace_enabled(void) __attribute__((weak));
extern void ssrnx_diag_flush(void) __attribute__((weak));
/* file_contract.c is part of production, while focused compat.c host tests may
 * link this translation unit alone. Keep the diagnostics hooks optional there. */
extern void ssrnx_file_contract_reset(void) __attribute__((weak));
extern void ssrnx_file_contract_open(int, const char *, const char *, int, int, bool) __attribute__((weak));
extern void ssrnx_file_contract_dup(int, int) __attribute__((weak));
extern int ssrnx_file_contract_reopen_duplicate(int, int, bool) __attribute__((weak));
extern int ssrnx_file_contract_descriptor_flags(int, bool, int) __attribute__((weak));
extern int ssrnx_file_contract_status_flags(int) __attribute__((weak));
extern int ssrnx_file_contract_allocate(int, int64_t, int64_t) __attribute__((weak));
extern int ssrnx_file_contract_socket_open(int, bool) __attribute__((weak));
extern void ssrnx_file_contract_read(int, size_t, int64_t) __attribute__((weak));
extern void ssrnx_file_contract_read_at(int, int64_t, int64_t) __attribute__((weak));
extern void ssrnx_file_contract_write(int, size_t, int64_t) __attribute__((weak));
extern void ssrnx_file_contract_ftruncate(int, int64_t, int, int) __attribute__((weak));
extern void ssrnx_file_contract_fstat(int, int, int64_t, uint32_t, int) __attribute__((weak));
extern bool ssrnx_file_contract_host_path(int, char *, size_t) __attribute__((weak));
extern void ssrnx_file_contract_mmap(int, size_t, int64_t, const void *, int) __attribute__((weak));
extern void ssrnx_file_contract_close(int) __attribute__((weak));
extern int ssrnx_file_contract_fd_for_host_path(const char *) __attribute__((weak));
extern int ssrnx_file_contract_truncate_open(const char *, int64_t) __attribute__((weak));
extern void ssrnx_android_file_observer_notify_event(const char *, int) __attribute__((weak));
extern void ssrnx_android_file_observer_notify_move(const char *, const char *) __attribute__((weak));

static bool compat_ssrnx_memtrace_enabled(void) {
    return ssrnx_diag_full_trace_enabled && ssrnx_diag_full_trace_enabled();
}

static void compat_ssrnx_memtrace_flush(void) {
    if (ssrnx_diag_flush) ssrnx_diag_flush();
}

/* Root-cause diagnostic: keep the tail of guest stderr without logging from
 * the write path itself.  Native runtimes such as Boehm GC commonly print a
 * fatal reason to fd 2 immediately before calling bare abort().  Logging from
 * write(2) can recurse or perturb signal-handler timing, so capture bytes into
 * a lock-free ring and emit them only once abort() is already in progress. */
#define SSRNX_GUEST_STDERR_RING_CAP 8192u
static unsigned char compat_g_guest_stderr_ring[SSRNX_GUEST_STDERR_RING_CAP];
static uint64_t compat_g_guest_stderr_seq = 0;

static void compat_ssrnx_guest_stderr_capture(const void *data, size_t size) {
    if (!data || size == 0) return;
    const unsigned char *src = (const unsigned char *)data;
    const uint64_t start = __atomic_fetch_add(&compat_g_guest_stderr_seq, (uint64_t)size, __ATOMIC_RELAXED);
    for (size_t i = 0; i < size; ++i)
        compat_g_guest_stderr_ring[(size_t)((start + (uint64_t)i) % SSRNX_GUEST_STDERR_RING_CAP)] = src[i];
}

static void compat_ssrnx_guest_stderr_capture_vformat(const char *fmt, va_list ap) {
    if (!fmt) return;
    char tmp[1024];
    va_list copy;
    va_copy(copy, ap);
    const int n = vsnprintf(tmp, sizeof(tmp), fmt, copy);
    va_end(copy);
    if (n <= 0) return;
    const size_t used = (size_t)n < sizeof(tmp) ? (size_t)n : sizeof(tmp) - 1u;
    compat_ssrnx_guest_stderr_capture(tmp, used);
}

static void compat_ssrnx_guest_stderr_dump_tail(void) {
    const uint64_t end = __atomic_load_n(&compat_g_guest_stderr_seq, __ATOMIC_RELAXED);
    if (end == 0) {
        SSRNX_LOGCE("GUESTERR", "stderr-tail: empty");
        return;
    }
    uint64_t begin = end > 4096u ? end - 4096u : 0u;
    char line[640];
    size_t pos = 0;
    unsigned line_no = 0;
    for (uint64_t seq = begin; seq < end; ++seq) {
        const unsigned char c = compat_g_guest_stderr_ring[(size_t)(seq % SSRNX_GUEST_STDERR_RING_CAP)];
        if (c == '\n' || pos + 5u >= sizeof(line)) {
            if (pos) {
                line[pos] = '\0';
                SSRNX_LOGCE("GUESTERR", "stderr-tail #%u: %s", line_no++, line);
                pos = 0;
            }
            if (c == '\n') continue;
        }
        if (c == '\r') continue;
        if (c == '\t') { line[pos++] = ' '; continue; }
        if (c >= 0x20u && c <= 0x7eu) line[pos++] = (char)c;
        else {
            static const char hex[] = "0123456789abcdef";
            if (pos + 4u < sizeof(line)) {
                line[pos++] = '\\'; line[pos++] = 'x';
                line[pos++] = hex[(c >> 4) & 0xfu]; line[pos++] = hex[c & 0xfu];
            }
        }
    }
    if (pos) {
        line[pos] = '\0';
        SSRNX_LOGCE("GUESTERR", "stderr-tail #%u: %s", line_no, line);
    }
}

/* Linux/Bionic AArch64 open(2) flag values used by Android's UAPI headers.
 * Translate explicitly instead of assuming newlib/libnx constants match.
 */
enum {
    SSRNX_ANDROID_O_ACCMODE   = 00000003,
    SSRNX_ANDROID_O_RDONLY    = 00000000,
    SSRNX_ANDROID_O_WRONLY    = 00000001,
    SSRNX_ANDROID_O_RDWR      = 00000002,
    SSRNX_ANDROID_O_CREAT     = 00000100,
    SSRNX_ANDROID_O_EXCL      = 00000200,
    SSRNX_ANDROID_O_NOCTTY    = 00000400,
    SSRNX_ANDROID_O_TRUNC     = 00001000,
    SSRNX_ANDROID_O_APPEND    = 00002000,
    SSRNX_ANDROID_O_NONBLOCK  = 00004000,
    SSRNX_ANDROID_O_LARGEFILE = 00400000,
    SSRNX_ANDROID_O_CLOEXEC   = 02000000,
    SSRNX_ANDROID_O_DIRECTORY = 00040000,
    SSRNX_ANDROID_O_NOFOLLOW  = 00100000,
    SSRNX_ANDROID_O_DIRECT    = 00200000,
    SSRNX_ANDROID_O_TMPFILE   = 020000000 | SSRNX_ANDROID_O_DIRECTORY
};

typedef struct compat_SsrnxAndroidTimespec {
    int64_t tv_sec;
    int64_t tv_nsec;
} compat_SsrnxAndroidTimespec;

/* Android/Bionic LP64 iovec ABI.  Keep this SSRNX-owned instead of including
 * <sys/uio.h>: devkitA64/newlib does not provide that host header, while the
 * guest ABI is simply an AArch64 pointer followed by size_t. */
typedef struct compat_SsrnxAndroidIovec {
    void *iov_base;
    size_t iov_len;
} compat_SsrnxAndroidIovec;
_Static_assert(sizeof(compat_SsrnxAndroidIovec) == 16, "Bionic iovec LP64 ABI size");
_Static_assert(offsetof(compat_SsrnxAndroidIovec, iov_base) == 0, "Bionic iovec base offset");
_Static_assert(offsetof(compat_SsrnxAndroidIovec, iov_len) == 8, "Bionic iovec length offset");

typedef struct compat_SsrnxCompatExport {
    const char *name;
    uintptr_t address;
    const char *provider;
} compat_SsrnxCompatExport;

#define SSRNX_FUNC_ADDR(fn) ((uintptr_t)(void (*)(void))(fn))
#define SSRNX_DATA_ADDR(obj) ((uintptr_t)(const void *)&(obj))

static void compat_ssrnx_fortify_fail(const char *fn);

/* Bionic __errno(): the guest's own errno slot, holding Linux numbering.
 * Host errno raised by any SSRNX path is imported (and translated) lazily;
 * see ssrnx_guest_errno_location(). */
static int *compat_ssrnx_bionic_errno(void) {
    return ssrnx_guest_errno_location();
}


/* Active guest link graph for libdl's dl_iterate_phdr. SSRNX currently runs a
 * single guest process at a time, so a process-global graph matches Android's
 * process-scoped loader semantics. */
static SsrnxLinker *compat_g_active_linker;
void ssrnx_compat_set_active_linker(SsrnxLinker *linker) {
    compat_g_active_linker = linker;
}
void ssrnx_compat_pin_guest_address(const void *address) {
    const uintptr_t value = (uintptr_t)address;
    if (!compat_g_active_linker || value == 0) return;
    for (size_t i = 0; i < ssrnx_linker_module_count(compat_g_active_linker); ++i) {
        const SsrnxModule *module = ssrnx_linker_module_at(compat_g_active_linker, i);
        if (!module || !module->mapped || !ssrnx_module_runtime_contains(module, value, 1)) continue;
        /* Bionic keeps a DSO resident while C++ thread_local destructors can
         * still call into it. Phase 3 pins conservatively for process lifetime;
         * a later lifetime pass may release the pin after the final TLS dtor. */
        (void)ssrnx_linker_pin_module(compat_g_active_linker, module);
        SSRNX_LOGCI("CXA", "pin TLS-dtor DSO module=%s address=%p", module->path, address);
        return;
    }
}

/* Android-facing filesystem namespace. Keep guest paths Android-like while
 * translating only at the libc boundary, matching how an Android process sees
 * /data/data/<package> even though SSRNX persists it on the SD card. */
#define SSRNX_STORAGE_PATH_CAP 1024
static char compat_g_storage_package[192];
static char compat_g_storage_app_root[SSRNX_STORAGE_PATH_CAP];
static char compat_g_storage_external_root[SSRNX_STORAGE_PATH_CAP];
static char compat_g_package_apk_host[SSRNX_STORAGE_PATH_CAP];
static char compat_g_package_apk_guest[SSRNX_STORAGE_PATH_CAP];
static uint64_t compat_g_storage_mapped_paths;
static uint64_t compat_g_storage_write_opens;
static uint64_t compat_g_storage_mutations;
static uint64_t compat_g_storage_log_count;
static uint32_t compat_g_storage_query_trace_count;

#define SSRNX_STORAGE_STREAM_MAX 32u
typedef struct compat_SsrnxStorageStreamTrace {
    FILE *file;
    uint64_t bytes_written;
    int host_flags;
    char host_path[SSRNX_STORAGE_PATH_CAP];
} compat_SsrnxStorageStreamTrace;
static compat_SsrnxStorageStreamTrace compat_g_storage_streams[SSRNX_STORAGE_STREAM_MAX];
static pthread_mutex_t compat_g_storage_stream_lock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t compat_g_storage_stream_trace_count;

/* libnx/newlib does not provide pread()/pwrite().  Positional I/O is therefore
 * emulated with seek + I/O + offset restore.  All guest regular-file operations
 * that touch the shared file offset participate in this lock so a concurrent
 * read/write/lseek cannot observe the temporary positional seek.  Sockets and
 * SSRNX virtual descriptors never take this lock. */
static pthread_mutex_t compat_g_regular_file_io_lock = PTHREAD_MUTEX_INITIALIZER;

static bool compat_ssrnx_host_fd_is_regular_file(int fd) {
    if (fd < 0 || ssrnx_fd_is_virtual(fd)) return false;
    const int saved = errno;
    struct stat st;
    const bool regular = fstat(fd, &st) == 0 && S_ISREG(st.st_mode);
    errno = saved;
    return regular;
}

static bool compat_ssrnx_regular_file_io_lock(int fd) {
#ifdef __SWITCH__
    /* Inspect only after locking: rename may temporarily rebind fsdev data. */
    if (fd < 0 || ssrnx_fd_is_virtual(fd)) return false;
    pthread_mutex_lock(&compat_g_regular_file_io_lock);
    if (compat_ssrnx_host_fd_is_regular_file(fd)) return true;
    pthread_mutex_unlock(&compat_g_regular_file_io_lock);
    return false;
#else
    if (!compat_ssrnx_host_fd_is_regular_file(fd)) return false;
    pthread_mutex_lock(&compat_g_regular_file_io_lock);
    return true;
#endif
}

static void compat_ssrnx_regular_file_io_unlock(bool locked) {
    if (locked) pthread_mutex_unlock(&compat_g_regular_file_io_lock);
}

/* Process-wide Android network state. The guest and Java framework must share
 * the same socket/NIFM view, just as they would inside one Android process. */
static int compat_g_network_socket_state;
static uint32_t compat_g_network_trace_count;
static uint32_t compat_g_network_poll_trace_count;
static uint32_t compat_g_network_io_trace_count;

#define SSRNX_NETWORK_SOCKET_TRACE_MAX 64u
typedef struct compat_SsrnxNetworkSocketTrace {
    int fd;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t io_calls, recv_wouldblock, send_wouldblock;
    uint64_t last_wouldblock_ms, last_io_log_ms;
    bool io_failure_seen;
    uintptr_t last_wouldblock_caller;
    int last_wouldblock_tid;
    bool recv_blocked, send_blocked;
    bool eof_seen;
    bool used;
} compat_SsrnxNetworkSocketTrace;
static compat_SsrnxNetworkSocketTrace compat_g_network_sockets[SSRNX_NETWORK_SOCKET_TRACE_MAX];
static pthread_mutex_t compat_g_network_socket_trace_lock = PTHREAD_MUTEX_INITIALIZER;
static uint64_t compat_g_network_wouldblock_epoch;
static uint32_t compat_g_network_connectivity_trace_count;
static uint32_t compat_g_network_ca_redirect_count;
#ifdef __SWITCH__
static pthread_mutex_t compat_g_network_lock = PTHREAD_MUTEX_INITIALIZER;
static int compat_g_network_nifm_state;
static int compat_g_network_ca_state;
static uint64_t compat_g_network_status_tick;
static int compat_g_network_status_cached = -1;
static int compat_g_network_transport_cached = 1; /* Android TRANSPORT_WIFI */
typedef struct compat_SsrnxNetworkCaRoot {
    uint8_t *der;
    size_t size;
} compat_SsrnxNetworkCaRoot;
static compat_SsrnxNetworkCaRoot *compat_g_network_ca_roots;
static size_t compat_g_network_ca_root_count;
#endif
static char compat_g_network_ca_bundle[SSRNX_STORAGE_PATH_CAP];
static char compat_g_network_ca_dir[SSRNX_STORAGE_PATH_CAP];

static void compat_ssrnx_network_prepare_ca_bundle(void);
#ifdef __SWITCH__
static bool compat_ssrnx_prepare_android_ca_dir(const char *root, char *out, size_t cap);
static void compat_ssrnx_network_free_ca_roots_locked(void) {
    for (size_t i = 0; i < compat_g_network_ca_root_count; ++i) free(compat_g_network_ca_roots[i].der);
    free(compat_g_network_ca_roots);
    compat_g_network_ca_roots = NULL;
    compat_g_network_ca_root_count = 0;
}
#endif

/* FAST diagnostics intentionally trace only the first bounded set of network
 * boundary operations. This is enough to identify whether a guest stalls at
 * Android connectivity, DNS, socket/connect, poll/SO_ERROR, or trust-store
 * discovery without turning send/recv traffic into per-packet log spam. */
static bool compat_ssrnx_network_trace_slot(void) {
    return __atomic_fetch_add(&compat_g_network_trace_count, 1u, __ATOMIC_RELAXED) < 256u;
}

static bool compat_ssrnx_network_poll_trace_slot(void) {
    return __atomic_fetch_add(&compat_g_network_poll_trace_count, 1u, __ATOMIC_RELAXED) < 48u;
}

static bool compat_ssrnx_network_io_trace_slot(void) {
    return __atomic_fetch_add(&compat_g_network_io_trace_count, 1u, __ATOMIC_RELAXED) < 96u;
}

static void compat_ssrnx_network_socket_register(int fd) {
    if (fd < 0) return;
    pthread_mutex_lock(&compat_g_network_socket_trace_lock);
    for (size_t i = 0; i < SSRNX_NETWORK_SOCKET_TRACE_MAX; ++i) {
        if (compat_g_network_sockets[i].used && compat_g_network_sockets[i].fd == fd) {
            compat_g_network_sockets[i] = (compat_SsrnxNetworkSocketTrace){ .used = true, .fd = fd };
            pthread_mutex_unlock(&compat_g_network_socket_trace_lock);
            return;
        }
    }
    for (size_t i = 0; i < SSRNX_NETWORK_SOCKET_TRACE_MAX; ++i) {
        if (!compat_g_network_sockets[i].used) {
            compat_g_network_sockets[i].used = true;
            compat_g_network_sockets[i].fd = fd;
            compat_g_network_sockets[i].rx_bytes = 0;
            compat_g_network_sockets[i].tx_bytes = 0;
            compat_g_network_sockets[i].eof_seen = false;
            break;
        }
    }
    pthread_mutex_unlock(&compat_g_network_socket_trace_lock);
}

static bool compat_ssrnx_network_socket_take(int fd, uint64_t *rx_total, uint64_t *tx_total, bool *eof_seen) {
    bool found = false;
    pthread_mutex_lock(&compat_g_network_socket_trace_lock);
    for (size_t i = 0; i < SSRNX_NETWORK_SOCKET_TRACE_MAX; ++i) {
        if (!compat_g_network_sockets[i].used || compat_g_network_sockets[i].fd != fd) continue;
        if (rx_total) *rx_total = compat_g_network_sockets[i].rx_bytes;
        if (tx_total) *tx_total = compat_g_network_sockets[i].tx_bytes;
        if (eof_seen) *eof_seen = compat_g_network_sockets[i].eof_seen;
        memset(&compat_g_network_sockets[i], 0, sizeof(compat_g_network_sockets[i]));
        found = true;
        break;
    }
    pthread_mutex_unlock(&compat_g_network_socket_trace_lock);
    return found;
}

static bool compat_ssrnx_network_connectivity_trace_slot(void) {
    return __atomic_fetch_add(&compat_g_network_connectivity_trace_count, 1u, __ATOMIC_RELAXED) < 32u;
}

static bool compat_ssrnx_storage_join(char *out, size_t cap, const char *root, const char *suffix) {
    if (!out || !cap || !root || !root[0]) return false;
    if (!suffix) suffix = "";
    while (*suffix == '/') ++suffix;
    /* suffix is a guest name: store FAT-reserved characters reversibly. */
    char encoded[SSRNX_STORAGE_PATH_CAP];
    if (!ssrnx_host_name_encode(suffix, encoded, sizeof(encoded))) {
        errno = ENAMETOOLONG;
        return false;
    }
    suffix = encoded;
    const size_t rn = strlen(root), sn = strlen(suffix);
    const bool slash = sn != 0 && rn != 0 && root[rn - 1] != '/';
    if (rn + (slash ? 1u : 0u) + sn + 1u > cap) {
        errno = ENAMETOOLONG;
        return false;
    }
    memcpy(out, root, rn);
    size_t w = rn;
    if (slash) out[w++] = '/';
    memcpy(out + w, suffix, sn + 1u);
    return true;
}

/* procfs */
/* ---------- Linux CPU/memory topology (procfs/sysfs) ----------
 * Android native code sizes worker pools, affinity masks and memory budgets
 * from the Linux kernel ABI: /sys/devices/system/cpu/{possible,present,online}
 * and per-CPU cpufreq/cpu_capacity nodes (Documentation/ABI/testing/
 * sysfs-devices-system-cpu), /proc/cpuinfo and /proc/meminfo (proc(5)).
 * Horizon has no procfs/sysfs, so SSRNX materializes a read-only snapshot
 * derived from the process core mask, the architectural timer and the
 * process memory limits, and maps exactly these guest paths to it. */
static char compat_g_kernel_view_root[SSRNX_STORAGE_PATH_CAP];
static bool compat_ssrnx_path_prefix_boundary(const char *path, const char *prefix, const char **out_suffix);
static uint32_t compat_ssrnx_bionic_getuid(void);
static uint32_t compat_ssrnx_bionic_getgid(void);
static int compat_ssrnx_bionic_getppid(void);
static int compat_ssrnx_bionic_prctl(int, unsigned long, unsigned long, unsigned long, unsigned long);

static bool compat_ssrnx_kernel_view_host_path(const char *host) {
    return compat_g_kernel_view_root[0] && host &&
           compat_ssrnx_path_prefix_boundary(host, compat_g_kernel_view_root, NULL);
}

/* Identity of the modeled VFS, independent of its SD snapshot backend. */
static uint64_t compat_ssrnx_kernel_view_fs_type(const char *host) {
    if (host && strcmp(host, "apn64proc:/statm") == 0) return UINT64_C(0x9fa0);
    const char *suffix = NULL;
    if (!compat_ssrnx_kernel_view_host_path(host) ||
        !compat_ssrnx_path_prefix_boundary(host, compat_g_kernel_view_root, &suffix)) return 0;
    if (compat_ssrnx_path_prefix_boundary(suffix, "proc", NULL)) return UINT64_C(0x9fa0);
    if (compat_ssrnx_path_prefix_boundary(suffix, "sys", NULL)) return UINT64_C(0x62656572);
    return 0;
}

/* These procfs nodes stream content even though Linux reports st_size=0.
 * The SD snapshot's length and writable mode are backend details. */
static void compat_ssrnx_kernel_view_stat(const char *host, struct stat *st) {
    const char *suffix = NULL;
    const bool memory = host && strcmp(host, "apn64proc:/statm") == 0;
    if (!S_ISREG(st->st_mode) || (!memory && (!compat_ssrnx_kernel_view_host_path(host) ||
        !compat_ssrnx_path_prefix_boundary(host, compat_g_kernel_view_root, &suffix) ||
        strncmp(suffix, "proc/", 5) != 0))) return;
    st->st_size = 0;
    st->st_blocks = 0;
    st->st_mode = S_IFREG | 0444;
    const bool self = memory || strncmp(suffix, "proc/self/", 10) == 0;
    st->st_uid = self ? compat_ssrnx_bionic_getuid() : 0;
    st->st_gid = self ? compat_ssrnx_bionic_getgid() : 0;
}

/* Cortex-A57 (Tegra X1) stock application CPU clock in kHz. */
#define SSRNX_CPU_FREQ_KHZ 1020000u

static uint64_t compat_ssrnx_process_cpu_mask(void) {
#ifdef __SWITCH__
    u64 mask = 0;
    if (R_SUCCEEDED(svcGetInfo(&mask, InfoType_CoreMask, CUR_PROCESS_HANDLE, 0)) && mask) return mask;
    return 1u;
#else
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    if (n < 1) n = 1;
    if (n > 63) n = 63;
    return (UINT64_C(1) << n) - 1u;
#endif
}

static unsigned compat_ssrnx_process_cpu_count(void) {
    return (unsigned)__builtin_popcountll(compat_ssrnx_process_cpu_mask());
}

unsigned ssrnx_compat_cpu_count(void) {
    const unsigned count = compat_ssrnx_process_cpu_count();
    return count ? count : 1u;
}

static void compat_ssrnx_process_memory(uint64_t *out_total, uint64_t *out_available) {
    const int saved = errno;
    uint64_t total = 0, used = 0;
#ifdef __SWITCH__
    u64 value = 0;
    if (R_SUCCEEDED(svcGetInfo(&value, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0))) total = value;
    if (R_SUCCEEDED(svcGetInfo(&value, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0))) used = value;
    else used = total;
#else
    const long pages = sysconf(_SC_PHYS_PAGES), avail = sysconf(_SC_AVPHYS_PAGES);
    const long page = sysconf(_SC_PAGESIZE);
    if (pages > 0 && page > 0) total = (uint64_t)pages * (uint64_t)page;
    if (avail > 0 && page > 0) used = total - (uint64_t)avail * (uint64_t)page;
#endif
    if (used > total) used = total;
    uint64_t available = total - used;
#ifdef __SWITCH__
    /* The loader reserves the newlib heap up front. Its unclaimed tail and
     * free allocator blocks remain usable by the guest, despite UsedMemorySize.
     * Sample the allocator without changing the break or growing the heap. */
    extern char *fake_heap_start, *fake_heap_end;
    /* SSRNX's newlib lock is recursive and does not use the reent argument. */
    __malloc_lock(NULL);
    const struct mallinfo heap = mallinfo();
    const uintptr_t start = (uintptr_t)fake_heap_start, end = (uintptr_t)fake_heap_end;
    const uintptr_t current = (uintptr_t)sbrk(0);
    __malloc_unlock(NULL);
    if (start && end >= start && current >= start && current <= end) {
        const uint64_t unclaimed = end - current;
        uint64_t free_blocks = heap.fordblks;
        if (free_blocks > heap.arena) free_blocks = heap.arena;
        if (free_blocks > current - start) free_blocks = current - start;
        available += unclaimed < total - available ? unclaimed : total - available;
        available += free_blocks < total - available ? free_blocks : total - available;
    }
#endif
    if (out_total) *out_total = total;
    if (out_available) *out_available = available;
    errno = saved;
}

/* Shared with the Java framework (ActivityManager.MemoryInfo, Runtime) so
 * every Android memory view reports the same process budget as /proc/meminfo. */
void ssrnx_compat_process_memory(uint64_t *out_total, uint64_t *out_available) {
    compat_ssrnx_process_memory(out_total, out_available);
}

/* Linux cpulist format (e.g. "0-2" or "0,2-3"). */
static void compat_ssrnx_cpu_list_text(uint64_t mask, char *out, size_t cap) {
    size_t pos = 0;
    out[0] = '\0';
    for (unsigned cpu = 0; cpu < 64 && pos + 1 < cap; ++cpu) {
        if (!(mask & (UINT64_C(1) << cpu))) continue;
        unsigned last = cpu;
        while (last + 1 < 64 && (mask & (UINT64_C(1) << (last + 1)))) ++last;
        const int n = last == cpu
            ? snprintf(out + pos, cap - pos, "%s%u", pos ? "," : "", cpu)
            : snprintf(out + pos, cap - pos, "%s%u-%u", pos ? "," : "", cpu, last);
        if (n < 0 || (size_t)n >= cap - pos) break;
        pos += (size_t)n;
        cpu = last;
    }
}

static bool compat_ssrnx_kernel_mkdir(const char *path) {
    return mkdir(path, 0777) == 0 || errno == EEXIST;
}

/* True when `path` already holds exactly `n` bytes of `text`. */
static bool compat_ssrnx_file_content_equals(const char *path, const char *text, size_t n) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    char buf[512];
    size_t off = 0;
    bool same = true;
    for (;;) {
        const size_t got = fread(buf, 1, sizeof(buf), f);
        if (!got) break;
        if (got > n - off || memcmp(buf, text + off, got) != 0) { same = false; break; }
        off += got;
    }
    same = same && !ferror(f) && off == n;
    fclose(f);
    return same;
}

/* The kernel view is regenerated on every launch, but its nodes rarely
 * change. Rewriting an unchanged node costs an SD metadata commit, so an
 * identical node is left in place. */
static bool compat_ssrnx_kernel_write(const char *root, const char *relative, const char *text) {
    char path[SSRNX_STORAGE_PATH_CAP];
    if (!compat_ssrnx_storage_join(path, sizeof(path), root, relative)) return false;
    if (compat_ssrnx_file_content_equals(path, text, strlen(text))) return true;
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    const size_t n = strlen(text);
    bool ok = fwrite(text, 1, n, f) == n;
    ok = fclose(f) == 0 && ok;
    return ok;
}

static bool compat_ssrnx_kernel_write_maps(const char *root) {
    if (!compat_g_active_linker) return compat_ssrnx_kernel_write(root, "proc/self/maps", "");
    size_t load_segments = 0;
    const size_t modules = ssrnx_linker_module_count(compat_g_active_linker);
    for (size_t i = 0; i < modules; ++i) {
        const SsrnxModule *module = ssrnx_linker_module_at(compat_g_active_linker, i);
        if (!module || !module->mapped || !module->ehdr || !module->phdrs) continue;
        for (size_t p = 0; p < module->ehdr->e_phnum; ++p)
            if (module->phdrs[p].p_type == PT_LOAD && module->phdrs[p].p_memsz) ++load_segments;
    }
    const size_t line_cap = (size_t)SSRNX_STORAGE_PATH_CAP + 128u;
    if (load_segments > (SIZE_MAX - 1u) / line_cap) return false;
    const size_t cap = load_segments ? load_segments * line_cap + 1u : 1u;
    char *text = calloc(1, cap);
    if (!text) return false;
    size_t pos = 0;
    for (size_t i = 0; i < modules && pos + 1u < cap; ++i) {
        const SsrnxModule *module = ssrnx_linker_module_at(compat_g_active_linker, i);
        if (!module || !module->mapped || !module->ehdr || !module->phdrs) continue;
        const char *base = strrchr(module->path, '/');
        base = base ? base + 1 : module->path;
        char guest_path[SSRNX_STORAGE_PATH_CAP];
        const int path_len = compat_g_storage_package[0] && base[0]
            ? snprintf(guest_path, sizeof(guest_path), "/data/app/%s/lib/arm64/%s", compat_g_storage_package, base)
            : snprintf(guest_path, sizeof(guest_path), "%s", base[0] ? base : "[apn64-guest]");
        if (path_len < 0 || (size_t)path_len >= sizeof(guest_path)) { free(text); return false; }
        for (size_t p = 0; p < module->ehdr->e_phnum; ++p) {
            const Elf64_Phdr *ph = &module->phdrs[p];
            if (ph->p_type != PT_LOAD || !ph->p_memsz) continue;
            const uintptr_t raw_start = module->load_bias + (uintptr_t)ph->p_vaddr;
            const uintptr_t raw_end = raw_start + (uintptr_t)ph->p_memsz;
            const uintptr_t start = raw_start & ~(uintptr_t)0xfffu;
            const uintptr_t end = (raw_end + 0xfffu) & ~(uintptr_t)0xfffu;
            const uint64_t offset = ph->p_offset & ~UINT64_C(0xfff);
            const char perms[5] = {
                (ph->p_flags & PF_R) ? 'r' : '-', (ph->p_flags & PF_W) ? 'w' : '-',
                (ph->p_flags & PF_X) ? 'x' : '-', 'p', '\0'
            };
            const int wrote = snprintf(text + pos, cap - pos,
                                       "%012" PRIxPTR "-%012" PRIxPTR " %s %08" PRIx64 " 00:00 0 %s\n",
                                       start, end, perms, offset, guest_path);
            if (wrote < 0 || (size_t)wrote >= cap - pos) { free(text); return false; }
            pos += (size_t)wrote;
        }
    }
    const bool ok = compat_ssrnx_kernel_write(root, "proc/self/maps", text);
    free(text);
    return ok;
}

/* Identity fields come from the same guest model as libc. No guest tracer
 * backend exists, so TracerPid is zero; this does not grant ptrace support.
 * Omit Linux accounting fields for which SSRNX has no authoritative source. */
static bool compat_ssrnx_kernel_write_status(const char *root) {
    const uint32_t uid = compat_ssrnx_bionic_getuid(), gid = compat_ssrnx_bionic_getgid();
    const int pid = (int)getpid();
    char text[512];
    const int n = snprintf(text, sizeof(text),
                          "Name:\t%.15s\n"
                          "Tgid:\t%d\nPid:\t%d\nPPid:\t%d\n"
                          "TracerPid:\t0\n"
                          "Uid:\t%u\t%u\t%u\t%u\n"
                          "Gid:\t%u\t%u\t%u\t%u\n"
                          "NoNewPrivs:\t%d\n",
                          compat_g_storage_package, pid, pid, compat_ssrnx_bionic_getppid(),
                          uid, uid, uid, uid, gid, gid, gid, gid,
                          compat_ssrnx_bionic_prctl(39, 0, 0, 0, 0));
    if (n < 0 || (size_t)n >= sizeof(text)) { errno = EOVERFLOW; return false; }
    return compat_ssrnx_kernel_write(root, "proc/self/status", text);
}

static bool compat_ssrnx_kernel_write_meminfo(const char *root) {
    uint64_t total = 0, available = 0;
    compat_ssrnx_process_memory(&total, &available);
    char text[512];
    snprintf(text, sizeof(text),
             "MemTotal:       %8" PRIu64 " kB\n"
             "MemFree:        %8" PRIu64 " kB\n"
             "MemAvailable:   %8" PRIu64 " kB\n"
             "Buffers:               0 kB\n"
             "Cached:                0 kB\n"
             "SwapCached:            0 kB\n"
             "SwapTotal:             0 kB\n"
             "SwapFree:              0 kB\n",
             total / 1024u, available / 1024u, available / 1024u);
    return compat_ssrnx_kernel_write(root, "proc/meminfo", text);
}

static void compat_ssrnx_kernel_view_prepare(void) {
    compat_g_kernel_view_root[0] = '\0';
    /* The package runtime directory is the parent of the guest-visible app
     * root, so the snapshot never appears inside /data/data/<package>. */
    char runtime_root[SSRNX_STORAGE_PATH_CAP], root[SSRNX_STORAGE_PATH_CAP];
    snprintf(runtime_root, sizeof(runtime_root), "%s", compat_g_storage_app_root);
    char *slash = strrchr(runtime_root, '/');
    if (!slash || slash == runtime_root) return;
    *slash = '\0';
    static const char *const dirs[] = {
        "", "proc", "proc/self", "sys", "sys/devices", "sys/devices/system", "sys/devices/system/cpu",
    };
    if (!compat_ssrnx_storage_join(root, sizeof(root), runtime_root, "kernel")) return;
    for (size_t i = 0; i < sizeof(dirs) / sizeof(dirs[0]); ++i) {
        char dir[SSRNX_STORAGE_PATH_CAP];
        if (!compat_ssrnx_storage_join(dir, sizeof(dir), root, dirs[i]) || !compat_ssrnx_kernel_mkdir(dir)) {
            SSRNX_LOGCW("COMPAT", "kernel view unavailable dir=%s errno=%d", dir, errno);
            return;
        }
    }

    const uint64_t mask = compat_ssrnx_process_cpu_mask();
    char list[128], text[256];
    compat_ssrnx_cpu_list_text(mask, list, sizeof(list));
    snprintf(text, sizeof(text), "%s\n", list);
    bool ok = compat_ssrnx_kernel_write(root, "sys/devices/system/cpu/possible", text) &&
              compat_ssrnx_kernel_write(root, "sys/devices/system/cpu/present", text) &&
              compat_ssrnx_kernel_write(root, "sys/devices/system/cpu/online", text);

    /* /proc/cpuinfo in the arm64 kernel format (arch/arm64/kernel/cpuinfo.c).
     * BogoMIPS derives from the architectural counter frequency; Features
     * matches AT_HWCAP. */
#ifdef __SWITCH__
    const uint64_t timer_hz = armGetSystemTickFreq();
#else
    const uint64_t timer_hz = 19200000u;
#endif
    const size_t cpuinfo_cap = 64u * 256u;
    char *cpuinfo = calloc(1, cpuinfo_cap);
    size_t pos = 0;
    for (unsigned cpu = 0; ok && cpuinfo && cpu < 64; ++cpu) {
        if (!(mask & (UINT64_C(1) << cpu))) continue;
        char dir[96];
        snprintf(dir, sizeof(dir), "sys/devices/system/cpu/cpu%u", cpu);
        char cpu_dir[SSRNX_STORAGE_PATH_CAP], freq_dir[SSRNX_STORAGE_PATH_CAP], rel[160];
        ok = compat_ssrnx_storage_join(cpu_dir, sizeof(cpu_dir), root, dir) && compat_ssrnx_kernel_mkdir(cpu_dir) &&
             compat_ssrnx_storage_join(freq_dir, sizeof(freq_dir), cpu_dir, "cpufreq") && compat_ssrnx_kernel_mkdir(freq_dir);
        static const char *const freq_nodes[] = {
            "cpuinfo_min_freq", "cpuinfo_max_freq", "scaling_min_freq", "scaling_max_freq", "scaling_cur_freq",
        };
        snprintf(text, sizeof(text), "%u\n", SSRNX_CPU_FREQ_KHZ);
        for (size_t i = 0; ok && i < sizeof(freq_nodes) / sizeof(freq_nodes[0]); ++i) {
            snprintf(rel, sizeof(rel), "%s/cpufreq/%s", dir, freq_nodes[i]);
            ok = compat_ssrnx_kernel_write(root, rel, text);
        }
        snprintf(rel, sizeof(rel), "%s/cpu_capacity", dir);
        ok = ok && compat_ssrnx_kernel_write(root, rel, "1024\n");
        snprintf(rel, sizeof(rel), "%s/online", dir);
        ok = ok && compat_ssrnx_kernel_write(root, rel, "1\n");
        const int n = snprintf(cpuinfo + pos, cpuinfo_cap - pos,
                               "processor\t: %u\n"
                               "BogoMIPS\t: %" PRIu64 ".%02" PRIu64 "\n"
                               "Features\t: fp asimd aes pmull sha1 sha2 crc32\n"
                               "CPU implementer\t: 0x41\n"
                               "CPU architecture: 8\n"
                               "CPU variant\t: 0x1\n"
                               "CPU part\t: 0xd07\n"
                               "CPU revision\t: 1\n\n",
                               cpu, (timer_hz * 2u) / 1000000u, ((timer_hz * 2u) / 10000u) % 100u);
        if (n > 0 && (size_t)n < cpuinfo_cap - pos) pos += (size_t)n;
    }
    ok = ok && cpuinfo && compat_ssrnx_kernel_write(root, "proc/cpuinfo", cpuinfo) &&
         compat_ssrnx_kernel_write_meminfo(root);
    free(cpuinfo);
    if (!ok) {
        SSRNX_LOGCW("COMPAT", "kernel view incomplete root=%s errno=%d", root, errno);
        return;
    }
    snprintf(compat_g_kernel_view_root, sizeof(compat_g_kernel_view_root), "%s", root);
    uint64_t total = 0;
    compat_ssrnx_process_memory(&total, NULL);
    SSRNX_LOGCI("COMPAT", "kernel view ready cpus=%s count=%u memTotalKb=%" PRIu64 " root=%s",
                list, compat_ssrnx_process_cpu_count(), total / 1024u, root);
}

/* Map the modeled guest procfs/sysfs nodes. /proc/meminfo is refreshed at
 * most once per second so MemFree/MemAvailable follow process usage without
 * turning a polling guest into continuous SD writes. */
static bool compat_ssrnx_map_kernel_view_path(const char *guest, char *out, size_t cap) {
    if (!compat_g_kernel_view_root[0] || !guest) return false;
    /* /proc/<pid> and /proc/self refer to this same modeled guest process.
     * Other process IDs remain unavailable rather than aliasing to self. */
    char process_prefix[48], self_path[SSRNX_STORAGE_PATH_CAP];
    snprintf(process_prefix, sizeof(process_prefix), "/proc/%d", (int)getpid());
    const char *process_suffix = NULL;
    if (compat_ssrnx_path_prefix_boundary(guest, process_prefix, &process_suffix)) {
        const int n = snprintf(self_path, sizeof(self_path), "/proc/self/%s", process_suffix);
        if (n < 0 || (size_t)n >= sizeof(self_path)) { errno = ENAMETOOLONG; return false; }
        guest = self_path;
    }
    const char *suffix = NULL;
    const char *relative = NULL;
    char sys_relative[SSRNX_STORAGE_PATH_CAP];
    if (ssrnx_proc_memory_map && ssrnx_proc_memory_map(guest, out, cap)) return true;
    if (strcmp(guest, "/proc/cpuinfo") == 0) {
        relative = "proc/cpuinfo";
    } else if (strcmp(guest, "/proc/self/maps") == 0) {
        if (!compat_ssrnx_kernel_write_maps(compat_g_kernel_view_root)) return false;
        relative = "proc/self/maps";
    } else if (strcmp(guest, "/proc/self/status") == 0) {
        if (!compat_ssrnx_kernel_write_status(compat_g_kernel_view_root)) return false;
        relative = "proc/self/status";
    } else if (strcmp(guest, "/proc/meminfo") == 0) {
        static uint64_t last_refresh_s;
        struct timespec now = {0, 0};
        (void)clock_gettime(CLOCK_MONOTONIC, &now);
        const uint64_t now_s = (uint64_t)now.tv_sec;
        if (__atomic_exchange_n(&last_refresh_s, now_s, __ATOMIC_RELAXED) != now_s)
            (void)compat_ssrnx_kernel_write_meminfo(compat_g_kernel_view_root);
        relative = "proc/meminfo";
    } else if (compat_ssrnx_path_prefix_boundary(guest, "/sys/devices/system/cpu", &suffix)) {
        if (!compat_ssrnx_storage_join(sys_relative, sizeof(sys_relative), "sys/devices/system/cpu", suffix))
            return false;
        relative = sys_relative;
    }
    return relative && compat_ssrnx_storage_join(out, cap, compat_g_kernel_view_root, relative);
}

void ssrnx_compat_set_app_storage(const char *package_name, const char *app_root) {
    if (ssrnx_epoll_reset) ssrnx_epoll_reset();
    if (ssrnx_eventfd_reset) ssrnx_eventfd_reset();
    if (ssrnx_path_lookup_set_root) ssrnx_path_lookup_set_root(NULL);
    compat_g_kernel_view_root[0] = '\0';
    compat_g_storage_package[0] = '\0';
    compat_g_storage_app_root[0] = '\0';
    compat_g_storage_external_root[0] = '\0';
    compat_g_package_apk_host[0] = '\0';
    compat_g_package_apk_guest[0] = '\0';
    compat_g_storage_mapped_paths = 0;
    compat_g_storage_write_opens = 0;
    compat_g_storage_mutations = 0;
    compat_g_storage_log_count = 0;
    compat_g_storage_query_trace_count = 0;
    compat_g_storage_stream_trace_count = 0;
    if (ssrnx_file_contract_reset) ssrnx_file_contract_reset();
    pthread_mutex_lock(&compat_g_storage_stream_lock);
    memset(compat_g_storage_streams, 0, sizeof(compat_g_storage_streams));
    pthread_mutex_unlock(&compat_g_storage_stream_lock);
    compat_g_network_trace_count = 0;
    compat_g_network_poll_trace_count = 0;
    compat_g_network_wouldblock_epoch = 0;
    compat_g_network_io_trace_count = 0;
    pthread_mutex_lock(&compat_g_network_socket_trace_lock);
    memset(compat_g_network_sockets, 0, sizeof(compat_g_network_sockets));
    pthread_mutex_unlock(&compat_g_network_socket_trace_lock);
    compat_g_network_connectivity_trace_count = 0;
    compat_g_network_ca_redirect_count = 0;
    compat_g_network_ca_bundle[0] = '\0';
    compat_g_network_ca_dir[0] = '\0';
#ifdef __SWITCH__
    pthread_mutex_lock(&compat_g_network_lock);
    compat_g_network_ca_state = 0;
    compat_ssrnx_network_free_ca_roots_locked();
    pthread_mutex_unlock(&compat_g_network_lock);
#endif
    if (!package_name || !package_name[0] || !app_root || !app_root[0]) return;
    if (snprintf(compat_g_storage_package, sizeof(compat_g_storage_package), "%s", package_name) >=
            (int)sizeof(compat_g_storage_package) ||
        snprintf(compat_g_storage_app_root, sizeof(compat_g_storage_app_root), "%s", app_root) >=
            (int)sizeof(compat_g_storage_app_root) ||
        !compat_ssrnx_storage_join(compat_g_storage_external_root, sizeof(compat_g_storage_external_root), app_root, "external")) {
        compat_g_storage_package[0] = '\0';
        compat_g_storage_app_root[0] = '\0';
        compat_g_storage_external_root[0] = '\0';
        return;
    }
    SSRNX_LOGCI("STORAGE", "guest app storage package=%s hostRoot=%s", compat_g_storage_package,
                compat_g_storage_app_root);
    if (ssrnx_path_lookup_set_root) ssrnx_path_lookup_set_root(compat_g_storage_app_root);
    (void)setenv("ANDROID_ROOT", "/system", 0);
    compat_ssrnx_network_prepare_ca_bundle();
    compat_ssrnx_kernel_view_prepare();
}

bool ssrnx_compat_set_package_apk(const char *host_apk_path) {
    compat_g_package_apk_host[0] = '\0';
    compat_g_package_apk_guest[0] = '\0';
    if (!host_apk_path || !host_apk_path[0] || !compat_g_storage_package[0]) return false;
    if (snprintf(compat_g_package_apk_host, sizeof(compat_g_package_apk_host), "%s", host_apk_path) >=
            (int)sizeof(compat_g_package_apk_host) ||
        snprintf(compat_g_package_apk_guest, sizeof(compat_g_package_apk_guest),
                 "/data/app/%s/base.apk", compat_g_storage_package) >=
            (int)sizeof(compat_g_package_apk_guest)) {
        compat_g_package_apk_host[0] = '\0';
        compat_g_package_apk_guest[0] = '\0';
        return false;
    }
    SSRNX_LOGCI("STORAGE", "package code path guest=%s host=%s",
                compat_g_package_apk_guest, compat_g_package_apk_host);
    return true;
}

const char *ssrnx_compat_guest_package_apk(void) {
    return compat_g_package_apk_guest[0] ? compat_g_package_apk_guest : NULL;
}

void ssrnx_compat_get_storage_stats(SsrnxCompatStorageStats *out_stats) {
    if (!out_stats) return;
    out_stats->mapped_paths = __atomic_load_n(&compat_g_storage_mapped_paths, __ATOMIC_RELAXED);
    out_stats->write_opens = __atomic_load_n(&compat_g_storage_write_opens, __ATOMIC_RELAXED);
    out_stats->mutations = __atomic_load_n(&compat_g_storage_mutations, __ATOMIC_RELAXED);
}

static bool compat_ssrnx_normalize_absolute_guest_path(const char *path, char *out, size_t cap) {
    if (!path || !out || cap < 2u || path[0] != '/') return false;
    size_t out_len = 1u;
    out[0] = '/';
    out[1] = '\0';
    const char *p = path + 1;
    while (*p) {
        while (*p == '/') ++p;
        if (!*p) break;
        const char *start = p;
        while (*p && *p != '/') ++p;
        const size_t len = (size_t)(p - start);
        if (len == 1u && start[0] == '.') continue;
        if (len == 2u && start[0] == '.' && start[1] == '.') {
            while (out_len > 1u && out[out_len - 1u] != '/') --out_len;
            if (out_len > 1u) --out_len;
            out[out_len] = '\0';
            continue;
        }
        const size_t needed = out_len + (out_len > 1u ? 1u : 0u) + len + 1u;
        if (needed > cap) return false;
        if (out_len > 1u) out[out_len++] = '/';
        memcpy(out + out_len, start, len);
        out_len += len;
        out[out_len] = '\0';
    }
    return true;
}

/* Android starts application processes with "/" as the working directory.
 * Relative guest paths resolve against this guest cwd (maintained by chdir and
 * reported by getcwd), never against the Switch host process cwd. */
static pthread_mutex_t compat_g_guest_cwd_lock = PTHREAD_MUTEX_INITIALIZER;
static char compat_g_guest_cwd[SSRNX_STORAGE_PATH_CAP] = "/";

static bool compat_ssrnx_guest_absolute_path(const char *path, char *out, size_t cap) {
    if (!path || !path[0]) return false;
    if (path[0] == '/') return compat_ssrnx_normalize_absolute_guest_path(path, out, cap);
    char joined[SSRNX_STORAGE_PATH_CAP];
    pthread_mutex_lock(&compat_g_guest_cwd_lock);
    const int n = snprintf(joined, sizeof(joined), "%s/%s", compat_g_guest_cwd, path);
    pthread_mutex_unlock(&compat_g_guest_cwd_lock);
    if (n <= 0 || (size_t)n >= sizeof(joined)) return false;
    return compat_ssrnx_normalize_absolute_guest_path(joined, out, cap);
}

static bool compat_ssrnx_path_prefix_boundary(const char *path, const char *prefix, const char **out_suffix) {
    const size_t n = strlen(prefix);
    if (strncmp(path, prefix, n) != 0) return false;
    if (path[n] != '\0' && path[n] != '/') return false;
    const char *suffix = path + n;
    while (*suffix == '/') ++suffix;
    if (out_suffix) *out_suffix = suffix;
    return true;
}

typedef enum compat_SsrnxGuestStorageDomain {
    SSRNX_STORAGE_DOMAIN_OTHER = 0,
    SSRNX_STORAGE_DOMAIN_PRIVATE,
    SSRNX_STORAGE_DOMAIN_EXTERNAL,
} compat_SsrnxGuestStorageDomain;

static compat_SsrnxGuestStorageDomain compat_ssrnx_guest_storage_domain(const char *path) {
    if (!path) return SSRNX_STORAGE_DOMAIN_OTHER;
    char normalized[SSRNX_STORAGE_PATH_CAP];
    const char *guest = path;
    if (compat_ssrnx_guest_absolute_path(path, normalized, sizeof(normalized)))
        guest = normalized;
    static const char *private_roots[] = {"/data/data", "/data/user/0", "/data/user_de/0"};
    static const char *external_roots[] = {"/sdcard", "/mnt/sdcard", "/storage/emulated/0", "/storage/self/primary"};
    for (size_t i = 0; i < sizeof(private_roots) / sizeof(private_roots[0]); ++i)
        if (compat_ssrnx_path_prefix_boundary(guest, private_roots[i], NULL)) return SSRNX_STORAGE_DOMAIN_PRIVATE;
    for (size_t i = 0; i < sizeof(external_roots) / sizeof(external_roots[0]); ++i)
        if (compat_ssrnx_path_prefix_boundary(guest, external_roots[i], NULL)) return SSRNX_STORAGE_DOMAIN_EXTERNAL;
    return SSRNX_STORAGE_DOMAIN_OTHER;
}

static bool compat_ssrnx_external_app_namespace_foreign(const char *path, const char *root,
                                                  const char *kind) {
    if (!path || !root || !kind || !compat_g_storage_package[0]) return false;
    char prefix[384];
    const int n = snprintf(prefix, sizeof(prefix), "%s/Android/%s/", root, kind);
    if (n <= 0 || (size_t)n >= sizeof(prefix)) return false;
    if (strncmp(path, prefix, (size_t)n) != 0) return false;
    const char *pkg = path + (size_t)n;
    const size_t active_len = strlen(compat_g_storage_package);
    return strncmp(pkg, compat_g_storage_package, active_len) != 0 ||
           (pkg[active_len] != '\0' && pkg[active_len] != '/');
}

static bool compat_font_config_path(const char *path) {
    return !strcmp(path,"/system/etc") || !strcmp(path,"/system/etc/fonts.xml") || !strcmp(path,"/system/etc/fallback_fonts.xml") ||
           !strcmp(path,"/system/etc/system_fonts.xml");
}

static bool compat_ssrnx_guest_path_is_readonly_system(const char *path) {
    if (!path) return false;
    char normalized[SSRNX_STORAGE_PATH_CAP];
    const char *guest = path;
    if (compat_ssrnx_guest_absolute_path(path, normalized, sizeof(normalized)))
        guest = normalized;
    const char *suffix = NULL;
    return compat_font_config_path(guest) ||
        compat_ssrnx_path_prefix_boundary(guest, "/system/etc/security/cacerts", &suffix) ||
        compat_ssrnx_path_prefix_boundary(guest, "/system/fonts", &suffix) ||
        compat_ssrnx_path_prefix_boundary(guest, "/fonts", &suffix);
}

static const char *compat_ssrnx_map_guest_path_resolved(const char *path, char *out, size_t cap, bool *out_mapped) {
    if (out_mapped) *out_mapped = false;
    if (!path || !out || !cap || !compat_g_storage_package[0] || !compat_g_storage_app_root[0]) return path;
    /* Preserve explicit host-native paths for legacy/runtime-internal callers.
     * Android Java APIs now expose guest-native absolute paths instead. */
    if (strncmp(path, "sdmc:", 5) == 0 || strncmp(path, "romfs:", 6) == 0) return path;

    char normalized[SSRNX_STORAGE_PATH_CAP];
    const char *guest = path;
    if (compat_ssrnx_guest_absolute_path(path, normalized, sizeof(normalized)))
        guest = normalized;

    if (compat_ssrnx_map_kernel_view_path(guest, out, cap)) {
        if (out_mapped) *out_mapped = true;
        return out;
    }

    /* ApplicationInfo.sourceDir / Context.getPackageCodePath() are Android
     * guest paths, never Switch host paths. Map the exact base APK path back
     * to the installed package so native engines can mount/read the APK. */
    if (compat_g_package_apk_guest[0] && compat_g_package_apk_host[0] &&
        strcmp(guest, compat_g_package_apk_guest) == 0) {
        if (snprintf(out, cap, "%s", compat_g_package_apk_host) >= (int)cap) return path;
        if (out_mapped) *out_mapped = true;
        __atomic_fetch_add(&compat_g_storage_mapped_paths, 1u, __ATOMIC_RELAXED);
        return out;
    }
    /* The code directory holding base.apk exists on Android too. Runtimes
     * check it before opening the APK (Mono/IL2CPP FileStream calls
     * Directory.Exists(dirname) and throws DirectoryNotFoundException), so
     * the directory itself maps to the installed package directory; nothing
     * below it other than base.apk is exposed. */
    if (compat_g_package_apk_guest[0] && compat_g_package_apk_host[0]) {
        const char *guest_slash = strrchr(compat_g_package_apk_guest, '/');
        const char *host_slash = strrchr(compat_g_package_apk_host, '/');
        const size_t guest_dir_len = guest_slash ? (size_t)(guest_slash - compat_g_package_apk_guest) : 0u;
        const size_t host_dir_len = host_slash ? (size_t)(host_slash - compat_g_package_apk_host) : 0u;
        if (guest_dir_len && host_dir_len && host_dir_len < cap &&
            strlen(guest) == guest_dir_len && strncmp(guest, compat_g_package_apk_guest, guest_dir_len) == 0) {
            memcpy(out, compat_g_package_apk_host, host_dir_len);
            out[host_dir_len] = '\0';
            if (out_mapped) *out_mapped = true;
            __atomic_fetch_add(&compat_g_storage_mapped_paths, 1u, __ATOMIC_RELAXED);
            return out;
        }
    }

    const char *suffix = NULL;
    static const char *private_roots[] = {"/data/data/", "/data/user/0/", "/data/user_de/0/"};
    if (compat_font_config_path(guest)) {
        if (snprintf(out,cap,"%s%s",NX_PLATFORM_ROOT,guest) >= (int)cap) return path;
        if (out_mapped) *out_mapped = true;
        __atomic_fetch_add(&compat_g_storage_mapped_paths, 1u, __ATOMIC_RELAXED);
        return out;
    }
    if (compat_ssrnx_path_prefix_boundary(guest, "/system/fonts", &suffix) ||
        compat_ssrnx_path_prefix_boundary(guest, "/fonts", &suffix)) {
#ifdef __SWITCH__
        const char *font_root = NX_PLATFORM_ROOT "/system/fonts";
#else
        const char *font_root = "romfs/system/fonts";
#endif
        if (!compat_ssrnx_storage_join(out, cap, font_root, suffix)) return path;
        if (out_mapped) *out_mapped = true;
        __atomic_fetch_add(&compat_g_storage_mapped_paths, 1u, __ATOMIC_RELAXED);
        return out;
    }
    char prefix[384];
    for (size_t i = 0; i < sizeof(private_roots) / sizeof(private_roots[0]); ++i) {
        const int n = snprintf(prefix, sizeof(prefix), "%s%s", private_roots[i], compat_g_storage_package);
        if (n <= 0 || (size_t)n >= sizeof(prefix)) continue;
        if (compat_ssrnx_path_prefix_boundary(guest, prefix, &suffix)) {
            if (!compat_ssrnx_storage_join(out, cap, compat_g_storage_app_root, suffix)) return path;
            if (out_mapped) *out_mapped = true;
            __atomic_fetch_add(&compat_g_storage_mapped_paths, 1u, __ATOMIC_RELAXED);
            return out;
        }
    }

    /* Android system trust-store namespace. SSRNX exports the Switch firmware
     * roots into a read-only guest namespace and maps only this standard
     * Android path. No endpoint, hostname, or application certificate decision
     * is overridden here; normal TLS verification and pinning remain guest
     * responsibilities. */
    if (compat_g_network_ca_dir[0] &&
        compat_ssrnx_path_prefix_boundary(guest, "/system/etc/security/cacerts", &suffix)) {
        if (!compat_ssrnx_storage_join(out, cap, compat_g_network_ca_dir, suffix)) return path;
        if (out_mapped) *out_mapped = true;
        __atomic_fetch_add(&compat_g_storage_mapped_paths, 1u, __ATOMIC_RELAXED);
        return out;
    }

    /* Old Android/embedded OpenSSL builds often retain an absolute build-time
     * OPENSSLDIR. Redirect only well-known CA bundle suffixes to SSRNX's
     * firmware-derived trust bundle; ordinary absolute paths remain untouched. */
    if (compat_g_network_ca_bundle[0]) {
        static const char *ca_suffixes[] = {
            "/ssl/cert.pem",
            "/etc/ssl/cert.pem",
            "/etc/ssl/certs/ca-certificates.crt",
        };
        const size_t path_len = strlen(guest);
        for (size_t i = 0; i < sizeof(ca_suffixes) / sizeof(ca_suffixes[0]); ++i) {
            const size_t suffix_len = strlen(ca_suffixes[i]);
            if (path_len >= suffix_len &&
                strcmp(guest + path_len - suffix_len, ca_suffixes[i]) == 0) {
                if (snprintf(out, cap, "%s", compat_g_network_ca_bundle) >= (int)cap) return path;
                if (out_mapped) *out_mapped = true;
                __atomic_fetch_add(&compat_g_storage_mapped_paths, 1u, __ATOMIC_RELAXED);
                if (__atomic_fetch_add(&compat_g_network_ca_redirect_count, 1u, __ATOMIC_RELAXED) < 8u)
                    SSRNX_LOGCI("NET", "TLS CA redirect guest=%s host=%s", path, out);
                return out;
            }
        }
    }

    static const char *external_roots[] = {"/sdcard", "/mnt/sdcard", "/storage/emulated/0", "/storage/self/primary"};
    /* Context.getExternalFilesDir()/getExternalCacheDir() expose standard
     * /storage/.../Android/data/<package>/... guest paths.  Keep those paths
     * stable for Java/native code while mapping the package-private subtree to
     * SSRNX's existing appRoot/external backing directory. */
    for (size_t i = 0; i < sizeof(external_roots) / sizeof(external_roots[0]); ++i) {
        const int n = snprintf(prefix, sizeof(prefix), "%s/Android/data/%s",
                               external_roots[i], compat_g_storage_package);
        if (n > 0 && (size_t)n < sizeof(prefix) &&
            compat_ssrnx_path_prefix_boundary(guest, prefix, &suffix)) {
            if (!compat_ssrnx_storage_join(out, cap, compat_g_storage_external_root, suffix)) return path;
            if (out_mapped) *out_mapped = true;
            __atomic_fetch_add(&compat_g_storage_mapped_paths, 1u, __ATOMIC_RELAXED);
            return out;
        }
    }
    /* OBB files live in <external>/Android/obb/<package>, not beneath
     * Android/data. Map that Android-visible namespace to SSRNX's extracted
     * package OBB directory so Java, NativeActivity and direct libc access all
     * converge on the same bytes. */
    for (size_t i = 0; i < sizeof(external_roots) / sizeof(external_roots[0]); ++i) {
        const int n = snprintf(prefix, sizeof(prefix), "%s/Android/obb/%s",
                               external_roots[i], compat_g_storage_package);
        if (n > 0 && (size_t)n < sizeof(prefix) &&
            compat_ssrnx_path_prefix_boundary(guest, prefix, &suffix)) {
            char obb_root[SSRNX_STORAGE_PATH_CAP];
            if (!compat_ssrnx_storage_join(obb_root, sizeof(obb_root), compat_g_storage_app_root, "obb") ||
                !compat_ssrnx_storage_join(out, cap, obb_root, suffix)) return path;
            if (out_mapped) *out_mapped = true;
            __atomic_fetch_add(&compat_g_storage_mapped_paths, 1u, __ATOMIC_RELAXED);
            return out;
        }
    }
    /* Modern Android does not turn another package's app-specific external
     * namespace into the caller's private backing store. Leaving foreign
     * Android/data and Android/obb paths unmapped prevents accidental aliasing
     * while still allowing ordinary public shared-storage paths below. */
    for (size_t i = 0; i < sizeof(external_roots) / sizeof(external_roots[0]); ++i) {
        if (compat_ssrnx_external_app_namespace_foreign(guest, external_roots[i], "data") ||
            compat_ssrnx_external_app_namespace_foreign(guest, external_roots[i], "obb"))
            return path;
    }
    for (size_t i = 0; i < sizeof(external_roots) / sizeof(external_roots[0]); ++i) {
        if (compat_ssrnx_path_prefix_boundary(guest, external_roots[i], &suffix)) {
            if (!compat_ssrnx_storage_join(out, cap, compat_g_storage_external_root, suffix)) return path;
            if (out_mapped) *out_mapped = true;
            __atomic_fetch_add(&compat_g_storage_mapped_paths, 1u, __ATOMIC_RELAXED);
            return out;
        }
    }
    return path;
}

/* Relative guest paths that do not land in a mapped namespace still name the
 * absolute guest path under the guest cwd, not a path relative to the host. */
static const char *compat_ssrnx_map_guest_path(const char *path, char *out, size_t cap, bool *out_mapped) {
    const char *host = compat_ssrnx_map_guest_path_resolved(path, out, cap, out_mapped);
    if (host != path || !path || !path[0] || path[0] == '/' || !out || !cap ||
        !compat_g_storage_package[0] || strncmp(path, "sdmc:", 5) == 0 || strncmp(path, "romfs:", 6) == 0)
        return host;
    char absolute[SSRNX_STORAGE_PATH_CAP];
    if (!compat_ssrnx_guest_absolute_path(path, absolute, sizeof(absolute)) ||
        snprintf(out, cap, "%s", absolute) >= (int)cap)
        return path;
    return out;
}

static bool compat_ssrnx_storage_host_path(const char *path) {
    if (!path || !compat_g_storage_app_root[0]) return false;
    const size_t n = strlen(compat_g_storage_app_root);
    return strncmp(path, compat_g_storage_app_root, n) == 0 && (path[n] == '\0' || path[n] == '/');
}

static void compat_ssrnx_storage_log(const char *op, const char *guest, const char *host, int rc) {
    const uint64_t index = __atomic_fetch_add(&compat_g_storage_log_count, 1u, __ATOMIC_RELAXED);
    if (index >= 48u) return;
    SSRNX_LOGCI("STORAGE", "%s guest=%s host=%s result=%d errno=%d", op,
                guest ? guest : "(null)", host ? host : "(null)", rc, rc < 0 ? errno : 0);
}




/* bionic abi */
/* Historical Bionic FILE ABI. NDK-built applications can still index __sF
 * directly, so preserve the LP64 object size/layout for stdin/out/err. Host
 * FILE* returned by fopen remain opaque and are passed through unchanged. */
typedef struct compat_SsrnxBionicSbuf { unsigned char *_base; int32_t _size; int32_t _pad; } compat_SsrnxBionicSbuf;
typedef struct compat_SsrnxBionicFile {
    unsigned char *_p;
    int32_t _r;
    int32_t _w;
    int32_t _flags;
    int32_t _file;
    compat_SsrnxBionicSbuf _bf;
    int32_t _lbfsize;
    int32_t _pad0;
    void *_cookie;
    void *_close;
    void *_read;
    void *_seek;
    void *_write;
    compat_SsrnxBionicSbuf _ext;
    unsigned char *_up;
    int32_t _ur;
    unsigned char _ubuf[3];
    unsigned char _nbuf[1];
    compat_SsrnxBionicSbuf _lb;
    int32_t _blksize;
    int32_t _pad1;
    int64_t _offset;
} compat_SsrnxBionicFile;
_Static_assert(sizeof(compat_SsrnxBionicFile) == 152, "Bionic LP64 FILE ABI size");

static compat_SsrnxBionicFile compat_g_bionic_sF[3] = {
    { ._flags = 0x0004, ._file = 0 },
    { ._flags = 0x0008, ._file = 1 },
    { ._flags = 0x0008 | 0x0002, ._file = 2 },
};

/* Bionic exports stdin/stdout/stderr as FILE* data symbols in addition to __sF.
 * Relocations therefore need the address of a pointer variable whose value is
 * the corresponding guest-visible Bionic FILE object. */
static compat_SsrnxBionicFile *compat_g_bionic_stdin_ptr = &compat_g_bionic_sF[0];
static compat_SsrnxBionicFile *compat_g_bionic_stdout_ptr = &compat_g_bionic_sF[1];
static compat_SsrnxBionicFile *compat_g_bionic_stderr_ptr = &compat_g_bionic_sF[2];

/* Android/Bionic exposes sys_signame as a data symbol for BSD compatibility.
 * The ABI is an NSIG-sized pointer table: signal 0 and real-time signals are
 * null; standard Linux signal numbers map to their short names without SIG. */
static const char *const compat_g_bionic_sys_signame[65] = {
    [1] = "HUP", [2] = "INT", [3] = "QUIT", [4] = "ILL", [5] = "TRAP",
    [6] = "ABRT", [7] = "BUS", [8] = "FPE", [9] = "KILL", [10] = "USR1",
    [11] = "SEGV", [12] = "USR2", [13] = "PIPE", [14] = "ALRM", [15] = "TERM",
    [16] = "STKFLT", [17] = "CHLD", [18] = "CONT", [19] = "STOP", [20] = "TSTP",
    [21] = "TTIN", [22] = "TTOU", [23] = "URG", [24] = "XCPU", [25] = "XFSZ",
    [26] = "VTALRM", [27] = "PROF", [28] = "WINCH", [29] = "IO", [30] = "PWR",
    [31] = "SYS",
};

/* Stage 14.6: Android/Bionic LP64 signal ABI. Do not pass these guest
 * structures to newlib/libnx signal APIs: Bionic deliberately exposes its own
 * LP64 struct sigaction layout, and Switch is not a Linux signal host.
 * Dispositions, masks, pending sets, alternate stacks and delivery live in the
 * signal runtime (compat_threads.c); this file keeps the sigset helpers. */
typedef uint64_t compat_SsrnxAndroidSigset;
typedef void (*compat_SsrnxAndroidSignalHandler)(int);
typedef void (*compat_SsrnxAndroidSigactionHandler)(int, void *, void *);

typedef struct compat_SsrnxAndroidSigaction {
    int32_t sa_flags;
    int32_t _pad_flags;
    union {
        compat_SsrnxAndroidSignalHandler sa_handler;
        compat_SsrnxAndroidSigactionHandler sa_sigaction;
        uintptr_t raw;
    } action;
    compat_SsrnxAndroidSigset sa_mask;
    void (*sa_restorer)(void);
} compat_SsrnxAndroidSigaction;

typedef struct compat_SsrnxDlInfo {
    const char *dli_fname;
    void *dli_fbase;
    const char *dli_sname;
    void *dli_saddr;
} compat_SsrnxDlInfo;

_Static_assert(sizeof(compat_SsrnxAndroidSigset) == 8, "Android arm64 sigset_t ABI size");
_Static_assert(sizeof(compat_SsrnxAndroidSigaction) == 32, "Android arm64 sigaction ABI size");
_Static_assert(sizeof(compat_SsrnxDlInfo) == 32, "Android arm64 Dl_info ABI size");

static __thread char compat_g_bionic_strsignal_buf[64];

/* Android/Bionic random(3) state. Bionic's rand()/srand() are wrappers around
 * random()/srandom(), and its default generator uses the BSD/NetBSD TYPE_3
 * x**31 + x**3 + 1 additive-feedback generator with 128 bytes of state.
 * Keep this state SSRNX-owned so Switch/newlib RNG behavior cannot leak across
 * the guest ABI. */
#define SSRNX_BIONIC_RANDOM_DEG 31
#define SSRNX_BIONIC_RANDOM_SEP 3
static pthread_mutex_t compat_g_bionic_random_lock = PTHREAD_MUTEX_INITIALIZER;
static int32_t compat_g_bionic_random_state[SSRNX_BIONIC_RANDOM_DEG];
static int compat_g_bionic_random_fidx;
static int compat_g_bionic_random_ridx;
static bool compat_g_bionic_random_initialized;

static long compat_ssrnx_bionic_random_unlocked(void) {
    const uint32_t sum = (uint32_t)compat_g_bionic_random_state[compat_g_bionic_random_fidx] +
                         (uint32_t)compat_g_bionic_random_state[compat_g_bionic_random_ridx];
    compat_g_bionic_random_state[compat_g_bionic_random_fidx] = (int32_t)sum;
    const long result = (long)((sum >> 1) & 0x7fffffffU);
    if (++compat_g_bionic_random_fidx == SSRNX_BIONIC_RANDOM_DEG) compat_g_bionic_random_fidx = 0;
    if (++compat_g_bionic_random_ridx == SSRNX_BIONIC_RANDOM_DEG) compat_g_bionic_random_ridx = 0;
    return result;
}

static void compat_ssrnx_bionic_srandom_unlocked(unsigned int seed) {
    compat_g_bionic_random_state[0] = (int32_t)seed;
    for (int i = 1; i < SSRNX_BIONIC_RANDOM_DEG; ++i) {
        const int32_t x = compat_g_bionic_random_state[i - 1];
        const int32_t hi = x / 127773;
        const int32_t lo = x % 127773;
        int32_t next = (int32_t)(16807 * lo - 2836 * hi);
        if (next <= 0) next += INT32_MAX;
        compat_g_bionic_random_state[i] = next;
    }
    compat_g_bionic_random_fidx = SSRNX_BIONIC_RANDOM_SEP;
    compat_g_bionic_random_ridx = 0;
    for (int i = 0; i < 10 * SSRNX_BIONIC_RANDOM_DEG; ++i) {
        (void)compat_ssrnx_bionic_random_unlocked();
    }
    compat_g_bionic_random_initialized = true;
}

static void compat_ssrnx_bionic_srandom(unsigned int seed) {
    pthread_mutex_lock(&compat_g_bionic_random_lock);
    compat_ssrnx_bionic_srandom_unlocked(seed);
    pthread_mutex_unlock(&compat_g_bionic_random_lock);
}

static long compat_ssrnx_bionic_random(void) {
    pthread_mutex_lock(&compat_g_bionic_random_lock);
    if (!compat_g_bionic_random_initialized) compat_ssrnx_bionic_srandom_unlocked(1);
    const long result = compat_ssrnx_bionic_random_unlocked();
    pthread_mutex_unlock(&compat_g_bionic_random_lock);
    return result;
}

static void compat_ssrnx_bionic_srand(unsigned int seed) {
    compat_ssrnx_bionic_srandom(seed);
}

static int compat_ssrnx_bionic_rand(void) {
    return (int)compat_ssrnx_bionic_random();
}

/* Android/Bionic sys_siglist ABI.  Signal 0 and real-time signal slots are
 * null; the standard Linux signal numbers carry Bionic's public descriptions.
 * strsignal() returns these same constant strings for ordinary signals. */
static const char *const compat_g_bionic_sys_siglist[SSRNX_BIONIC_NSIG] = {
    [1] = "Hangup", [2] = "Interrupt", [3] = "Quit", [4] = "Illegal instruction",
    [5] = "Trap", [6] = "Aborted", [7] = "Bus error",
    [8] = "Floating point exception", [9] = "Killed", [10] = "User signal 1",
    [11] = "Segmentation fault", [12] = "User signal 2", [13] = "Broken pipe",
    [14] = "Alarm clock", [15] = "Terminated", [16] = "Stack fault",
    [17] = "Child exited", [18] = "Continue", [19] = "Stopped (signal)",
    [20] = "Stopped", [21] = "Stopped (tty input)", [22] = "Stopped (tty output)",
    [23] = "Urgent I/O condition", [24] = "CPU time limit exceeded",
    [25] = "File size limit exceeded", [26] = "Virtual timer expired",
    [27] = "Profiling timer expired", [28] = "Window size changed", [29] = "I/O possible",
    [30] = "Power failure", [31] = "Bad system call",
};

static int compat_ssrnx_bionic_sigemptyset(compat_SsrnxAndroidSigset *set) {
    if (!set) { errno = EINVAL; return -1; }
    *set = 0;
    return 0;
}

static char *compat_ssrnx_bionic_strsignal(int signo) {
    if (signo > 0 && signo < SSRNX_BIONIC_NSIG && compat_g_bionic_sys_siglist[signo]) {
        return (char *)compat_g_bionic_sys_siglist[signo];
    }
    const char *prefix = (signo >= 32 && signo <= 64) ? "Real-time" : "Unknown";
    const int number = (signo >= 32 && signo <= 64) ? signo - 32 : signo;
    snprintf(compat_g_bionic_strsignal_buf, sizeof(compat_g_bionic_strsignal_buf), "%s signal %d", prefix, number);
    return compat_g_bionic_strsignal_buf;
}

/* sched_yield(2): relinquish the CPU so the thread may be rescheduled,
 * including onto another CPU. Horizon's plain zero-length sleep only yields to
 * equal-priority threads on the same core; spinning job workers then keep
 * their core while ready threads wait. Yield with core migration instead. */
static void compat_ssrnx_host_yield(void) {
#ifdef __SWITCH__
    svcSleepThread(YieldType_WithCoreMigration);
#else
    const struct timespec ts = {0, 0};
    (void)nanosleep(&ts, NULL);
#endif
}

static int compat_ssrnx_bionic_sched_yield(void) {
    compat_ssrnx_host_yield();
    return 0;
}

/* Android wchar_t is a 32-bit code unit on arm64. Keep this ABI independent
 * of the host C library's wchar_t typedef. */
static uint32_t *compat_ssrnx_bionic_wmemcpy(uint32_t *dst, const uint32_t *src, size_t count) {
    if (count > SIZE_MAX / sizeof(uint32_t)) { errno = EOVERFLOW; return dst; }
    return (uint32_t *)memcpy(dst, src, count * sizeof(uint32_t));
}
static uint32_t *compat_ssrnx_bionic_wmemmove(uint32_t *dst, const uint32_t *src, size_t count) {
    if (count > SIZE_MAX / sizeof(uint32_t)) { errno = EOVERFLOW; return dst; }
    return (uint32_t *)memmove(dst, src, count * sizeof(uint32_t));
}
static uint32_t *compat_ssrnx_bionic_wmemset(uint32_t *dst, uint32_t value, size_t count) {
    if (!dst) return NULL;
    for (size_t i = 0; i < count; ++i) dst[i] = value;
    return dst;
}

static int compat_ssrnx_bionic_dladdr(const void *address, compat_SsrnxDlInfo *info) {
    if (!address || !info || !compat_g_active_linker) return 0;
    memset(info, 0, sizeof(*info));
    const uintptr_t target = (uintptr_t)address;
    const size_t module_count = ssrnx_linker_module_count(compat_g_active_linker);
    for (size_t mi = 0; mi < module_count; ++mi) {
        const SsrnxModule *module = ssrnx_linker_module_at(compat_g_active_linker, mi);
        if (!module || !ssrnx_module_runtime_contains(module, target, 1)) continue;
        info->dli_fname = module->path;
        info->dli_fbase = (void *)module->load_bias;

        uintptr_t best_addr = 0;
        const char *best_name = NULL;
        for (size_t si = 0; si < module->dynamic.symbol_count; ++si) {
            const Elf64_Sym *symbol = NULL;
            const char *name = NULL;
            if (ssrnx_symbol_get(module, si, &symbol, &name) != SSRNX_OK || !symbol || !name || !name[0]) continue;
            if (symbol->st_shndx == SHN_UNDEF) continue;
            const unsigned bind = ELF64_ST_BIND(symbol->st_info);
            const unsigned vis = ELF64_ST_VISIBILITY(symbol->st_other);
            if ((bind != STB_GLOBAL && bind != STB_WEAK) ||
                (vis != STV_DEFAULT && vis != STV_PROTECTED)) continue;
            uintptr_t symbol_addr;
            if (symbol->st_shndx == SHN_ABS) symbol_addr = (uintptr_t)symbol->st_value;
            else if (symbol->st_value >= module->min_vaddr && symbol->st_value < module->max_vaddr)
                symbol_addr = module->load_bias + (uintptr_t)symbol->st_value;
            else continue;
            if (symbol_addr <= target && (!best_addr || symbol_addr > best_addr)) {
                best_addr = symbol_addr;
                best_name = name;
            }
        }
        if (best_addr) {
            info->dli_saddr = (void *)best_addr;
            info->dli_sname = best_name;
        }
        return 1;
    }
    return 0;
}

bool ssrnx_compat_describe_guest_address(uintptr_t address, char *out, size_t cap) {
    if (!out || cap == 0) return false;
    compat_SsrnxDlInfo info;
    if (!address || !compat_ssrnx_bionic_dladdr((const void *)address, &info) || !info.dli_fname) {
        snprintf(out, cap, "0x%" PRIxPTR, address);
        return false;
    }
    const char *base = strrchr(info.dli_fname, '/');
    base = base ? base + 1 : info.dli_fname;
    const uintptr_t offset = address - (uintptr_t)info.dli_fbase;
    if (info.dli_sname)
        snprintf(out, cap, "%s+0x%" PRIxPTR " (%s+0x%" PRIxPTR ")", base, offset,
                 info.dli_sname, address - (uintptr_t)info.dli_saddr);
    else
        snprintf(out, cap, "%s+0x%" PRIxPTR, base, offset);
    return true;
}

/* Stage 14.7: Android/Linux socket ABI translation.  Bionic exposes Linux
 * constants/layouts while libnx/newlib follows BSD-style networking in a few
 * places (notably AF_INET6 and SOL_SOCKET values), so do not pass guest values
 * through blindly. */
enum {
    SSRNX_ANDROID_AF_UNSPEC = 0,
    SSRNX_ANDROID_AF_UNIX = 1,
    SSRNX_ANDROID_AF_INET = 2,
    SSRNX_ANDROID_AF_INET6 = 10,
    SSRNX_ANDROID_SOCK_STREAM = 1,
    SSRNX_ANDROID_SOCK_DGRAM = 2,
    SSRNX_ANDROID_SOCK_RAW = 3,
    SSRNX_ANDROID_SOCK_RDM = 4,
    SSRNX_ANDROID_SOCK_SEQPACKET = 5,
    SSRNX_ANDROID_SOCK_NONBLOCK = 0x800,
    SSRNX_ANDROID_SOCK_CLOEXEC = 0x80000,
    SSRNX_ANDROID_SOL_SOCKET = 1,
    SSRNX_ANDROID_IPPROTO_IP = 0,
    SSRNX_ANDROID_IPPROTO_TCP = 6,
    SSRNX_ANDROID_IPPROTO_IPV6 = 41,
    SSRNX_ANDROID_SO_REUSEADDR = 2,
    SSRNX_ANDROID_SO_TYPE = 3,
    SSRNX_ANDROID_SO_ERROR = 4,
    SSRNX_ANDROID_SO_BROADCAST = 6,
    SSRNX_ANDROID_SO_SNDBUF = 7,
    SSRNX_ANDROID_SO_RCVBUF = 8,
    SSRNX_ANDROID_SO_KEEPALIVE = 9,
    SSRNX_ANDROID_SO_OOBINLINE = 10,
    SSRNX_ANDROID_SO_LINGER = 13,
    SSRNX_ANDROID_SO_REUSEPORT = 15,
    SSRNX_ANDROID_SO_RCVLOWAT = 18,
    SSRNX_ANDROID_SO_SNDLOWAT = 19,
    SSRNX_ANDROID_SO_RCVTIMEO = 20,
    SSRNX_ANDROID_SO_SNDTIMEO = 21,
    SSRNX_ANDROID_TCP_NODELAY = 1,
    SSRNX_ANDROID_TCP_KEEPIDLE = 4,
    SSRNX_ANDROID_TCP_KEEPINTVL = 5,
    SSRNX_ANDROID_TCP_KEEPCNT = 6,
    SSRNX_ANDROID_IPV6_V6ONLY = 26,
    SSRNX_ANDROID_F_DUPFD = 0,
    SSRNX_ANDROID_F_GETFD = 1,
    SSRNX_ANDROID_F_SETFD = 2,
    SSRNX_ANDROID_F_GETFL = 3,
    SSRNX_ANDROID_F_SETFL = 4,
    SSRNX_ANDROID_F_GETLK = 5,
    SSRNX_ANDROID_F_SETLK = 6,
    SSRNX_ANDROID_F_SETLKW = 7,
    SSRNX_ANDROID_F_RDLCK = 0,
    SSRNX_ANDROID_F_WRLCK = 1,
    SSRNX_ANDROID_F_UNLCK = 2,
    SSRNX_ANDROID_F_DUPFD_CLOEXEC = 1030,
    SSRNX_ANDROID_FD_CLOEXEC = 1,
    SSRNX_ANDROID_FD_SETSIZE = 1024,
};

typedef struct compat_SsrnxAndroidSockaddr {
    uint16_t sa_family;
    unsigned char sa_data[14];
} compat_SsrnxAndroidSockaddr;

typedef struct compat_SsrnxAndroidSockaddrIn {
    uint16_t sin_family;
    uint16_t sin_port;
    uint32_t sin_addr;
    unsigned char sin_zero[8];
} compat_SsrnxAndroidSockaddrIn;

typedef struct compat_SsrnxAndroidSockaddrIn6 {
    uint16_t sin6_family;
    uint16_t sin6_port;
    uint32_t sin6_flowinfo;
    unsigned char sin6_addr[16];
    uint32_t sin6_scope_id;
} compat_SsrnxAndroidSockaddrIn6;

typedef struct compat_SsrnxAndroidTimeval {
    int64_t tv_sec;
    int64_t tv_usec;
} compat_SsrnxAndroidTimeval;

typedef struct compat_SsrnxAndroidFdSet {
    uint64_t bits[SSRNX_ANDROID_FD_SETSIZE / 64];
} compat_SsrnxAndroidFdSet;

typedef struct compat_SsrnxAndroidAddrinfo {
    int32_t ai_flags;
    int32_t ai_family;
    int32_t ai_socktype;
    int32_t ai_protocol;
    uint32_t ai_addrlen;
    uint32_t _pad;
    /* Bionic LP64 order is canonname, addr, next.  This intentionally differs
     * from several BSD/libc host layouts, so never mirror host struct addrinfo
     * by field order.  Guest code dereferences these exact ABI offsets. */
    char *ai_canonname;
    compat_SsrnxAndroidSockaddr *ai_addr;
    struct compat_SsrnxAndroidAddrinfo *ai_next;
} compat_SsrnxAndroidAddrinfo;

_Static_assert(sizeof(compat_SsrnxAndroidSockaddr) == 16, "Android sockaddr ABI size");
_Static_assert(sizeof(compat_SsrnxAndroidSockaddrIn) == 16, "Android sockaddr_in ABI size");
_Static_assert(sizeof(compat_SsrnxAndroidSockaddrIn6) == 28, "Android sockaddr_in6 ABI size");
_Static_assert(sizeof(compat_SsrnxAndroidFdSet) == 128, "Android fd_set ABI size");
_Static_assert(sizeof(compat_SsrnxAndroidAddrinfo) == 48, "Android addrinfo ABI size");
_Static_assert(offsetof(compat_SsrnxAndroidAddrinfo, ai_canonname) == 24, "Android addrinfo ai_canonname ABI offset");
_Static_assert(offsetof(compat_SsrnxAndroidAddrinfo, ai_addr) == 32, "Android addrinfo ai_addr ABI offset");
_Static_assert(offsetof(compat_SsrnxAndroidAddrinfo, ai_next) == 40, "Android addrinfo ai_next ABI offset");

static int compat_ssrnx_host_errno_to_android(int value) {
    return ssrnx_errno_host_to_android(value);
}

/* Publish the pending host errno into the guest slot now. Host errno is
 * never rewritten with Bionic numbers (that would be translated twice). */
static void compat_ssrnx_network_fix_errno(void) {
    ssrnx_guest_errno_publish();
}

static int compat_ssrnx_network_result_int(int rc) {
    if (rc < 0) compat_ssrnx_network_fix_errno();
    return rc;
}

static ssize_t compat_ssrnx_network_result_ssize(ssize_t rc) {
    if (rc < 0) compat_ssrnx_network_fix_errno();
    return rc;
}

#ifdef __SWITCH__
static bool compat_ssrnx_file_nonempty(const char *path) {
    struct stat st;
    return path && stat(path, &st) == 0 && st.st_size > 0;
}

static bool compat_ssrnx_mkdir_if_needed(const char *path) {
    if (!path || !path[0]) return false;
    if (mkdir(path, 0777) == 0) {
        if (ssrnx_path_lookup_changed) ssrnx_path_lookup_changed(path);
        return true;
    }
    if (errno == EEXIST) return true;
    return false;
}

static bool compat_ssrnx_prepare_android_ca_dir(const char *root, char *out, size_t cap) {
    char p1[SSRNX_STORAGE_PATH_CAP], p2[SSRNX_STORAGE_PATH_CAP], p3[SSRNX_STORAGE_PATH_CAP];
    if (!root || !out || !cap) return false;
    if (!compat_ssrnx_mkdir_if_needed(root) ||
        !compat_ssrnx_storage_join(p1, sizeof(p1), root, "system") || !compat_ssrnx_mkdir_if_needed(p1) ||
        !compat_ssrnx_storage_join(p2, sizeof(p2), p1, "etc") || !compat_ssrnx_mkdir_if_needed(p2) ||
        !compat_ssrnx_storage_join(p3, sizeof(p3), p2, "security") || !compat_ssrnx_mkdir_if_needed(p3) ||
        !compat_ssrnx_storage_join(out, cap, p3, "cacerts") || !compat_ssrnx_mkdir_if_needed(out))
        return false;
    return true;
}

static int compat_ssrnx_write_pem_certificate(FILE *f, const unsigned char *der, size_t len) {
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (!f || !der || !len || fputs("-----BEGIN CERTIFICATE-----\n", f) < 0) return -1;
    unsigned column = 0;
    for (size_t i = 0; i < len; i += 3) {
        const size_t left = len - i;
        const uint32_t v = (uint32_t)der[i] << 16 |
                           (left > 1 ? (uint32_t)der[i + 1] << 8 : 0) |
                           (left > 2 ? (uint32_t)der[i + 2] : 0);
        const char out[4] = {
            b64[(v >> 18) & 63], b64[(v >> 12) & 63],
            left > 1 ? b64[(v >> 6) & 63] : '=', left > 2 ? b64[v & 63] : '='
        };
        if (fwrite(out, 1, sizeof(out), f) != sizeof(out)) return -1;
        column += 4;
        if (column == 64) { if (fputc('\n', f) == EOF) return -1; column = 0; }
    }
    if (column && fputc('\n', f) == EOF) return -1;
    return fputs("-----END CERTIFICATE-----\n", f) < 0 ? -1 : 0;
}

static bool compat_ssrnx_ca_cert_exported(const SslBuiltInCertificateInfo *cert) {
    return cert->cert_id >= 1000 && cert->status == SslTrustedCertStatus_EnabledTrusted &&
           cert->cert_data && cert->cert_size != 0;
}

/* FNV-1a over everything the exported bundle encodes: the trusted root set
 * (id, size, DER) and the per-certificate directory. */
static uint64_t compat_ssrnx_ca_export_fingerprint(const SslBuiltInCertificateInfo *certs, u32 total,
                                            const char *android_ca_dir) {
    uint64_t h = UINT64_C(1469598103934665603);
    const unsigned char *p;
    size_t n;
#define SSRNX_FNV(ptr, len) \
    for (p = (const unsigned char *)(ptr), n = (len); n; --n, ++p) { h ^= *p; h *= UINT64_C(1099511628211); }
    if (android_ca_dir) SSRNX_FNV(android_ca_dir, strlen(android_ca_dir) + 1u);
    for (u32 i = 0; i < total; ++i) {
        if (!compat_ssrnx_ca_cert_exported(&certs[i])) continue;
        const uint32_t meta[2] = {(uint32_t)certs[i].cert_id, (uint32_t)certs[i].cert_size};
        SSRNX_FNV(meta, sizeof(meta));
        SSRNX_FNV(certs[i].cert_data, (size_t)certs[i].cert_size);
    }
#undef SSRNX_FNV
    return h;
}

/* The exported bundle is current when its stamp records the same
 * fingerprint and every per-certificate file is present. Launches then skip
 * rewriting ~50 PEM files to the SD card. */
static bool compat_ssrnx_ca_export_current(const char *path, const char *android_ca_dir, uint64_t fingerprint,
                                    const SslBuiltInCertificateInfo *certs, u32 total) {
    char stamp[SSRNX_STORAGE_PATH_CAP], text[32];
    if (!compat_ssrnx_file_nonempty(path) ||
        snprintf(stamp, sizeof(stamp), "%s.stamp", path) >= (int)sizeof(stamp)) return false;
    snprintf(text, sizeof(text), "%016" PRIx64 "\n", fingerprint);
    if (!compat_ssrnx_file_content_equals(stamp, text, strlen(text))) return false;
    if (!android_ca_dir || !android_ca_dir[0]) return true;
    u32 expected = 0, present = 0;
    for (u32 i = 0; i < total; ++i) if (compat_ssrnx_ca_cert_exported(&certs[i])) ++expected;
    DIR *dir = opendir(android_ca_dir);
    if (!dir) return false;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        unsigned id = 0;
        char tail[8] = {0};
        if (sscanf(ent->d_name, "apn64-%8u.%4s", &id, tail) != 2 || strcmp(tail, "pem") != 0) continue;
        for (u32 i = 0; i < total; ++i) {
            if (compat_ssrnx_ca_cert_exported(&certs[i]) && (unsigned)certs[i].cert_id == id) { ++present; break; }
        }
    }
    closedir(dir);
    return present == expected;
}

static void compat_ssrnx_ca_export_stamp(const char *path, uint64_t fingerprint, bool valid) {
    char stamp[SSRNX_STORAGE_PATH_CAP];
    if (snprintf(stamp, sizeof(stamp), "%s.stamp", path) >= (int)sizeof(stamp)) return;
    if (!valid) { (void)remove(stamp); return; }
    FILE *f = fopen(stamp, "wb");
    if (!f) return;
    const int wrote = fprintf(f, "%016" PRIx64 "\n", fingerprint);
    if (fclose(f) != 0 || wrote <= 0) (void)remove(stamp);
}

static bool compat_ssrnx_export_firmware_ca_bundle(const char *path, const char *android_ca_dir) {
    const bool had_bundle = compat_ssrnx_file_nonempty(path);
    Result rc = sslInitialize(1);
    if (R_FAILED(rc)) return had_bundle;

    u32 ids[1] = {(u32)SslCaCertificateId_All};
    u32 size = 0, total = 0;
    rc = sslGetCertificateBufSize(ids, 1, &size);
    if (R_FAILED(rc) || size < sizeof(SslBuiltInCertificateInfo)) { sslExit(); return had_bundle; }

    const size_t alloc_size = (size + 0xfffu) & ~(size_t)0xfffu;
    void *buffer = memalign(0x1000, alloc_size);
    if (!buffer) { sslExit(); return had_bundle; }
    memset(buffer, 0, alloc_size);
    rc = sslGetCertificates(buffer, size, ids, 1, &total);
    if (R_FAILED(rc)) { free(buffer); sslExit(); return had_bundle; }

    /* Retain the exact firmware DER roots as process-local Android trust
     * anchors. Android Java TLS clients obtain these through
     * TrustManagerFactory/X509TrustManager rather than SSL_CERT_FILE. */
    SslBuiltInCertificateInfo *certs = (SslBuiltInCertificateInfo *)buffer;
    compat_SsrnxNetworkCaRoot *roots = total ? calloc((size_t)total, sizeof(*roots)) : NULL;
    size_t root_count = 0;
    if (!total || roots) {
        for (u32 i = 0; i < total; ++i) {
            if (!compat_ssrnx_ca_cert_exported(&certs[i])) continue;
            uint8_t *copy = malloc((size_t)certs[i].cert_size);
            if (!copy) continue;
            memcpy(copy, certs[i].cert_data, (size_t)certs[i].cert_size);
            roots[root_count].der = copy;
            roots[root_count].size = (size_t)certs[i].cert_size;
            ++root_count;
        }
        compat_ssrnx_network_free_ca_roots_locked();
        compat_g_network_ca_roots = roots;
        compat_g_network_ca_root_count = root_count;
        roots = NULL;
    }
    free(roots);

    const uint64_t fingerprint = compat_ssrnx_ca_export_fingerprint(certs, total, android_ca_dir);
    if (compat_ssrnx_ca_export_current(path, android_ca_dir, fingerprint, certs, total)) {
        free(buffer);
        sslExit();
        SSRNX_LOGCI("NET", "firmware CA bundle current path=%s roots=%zu", path, compat_g_network_ca_root_count);
        return true;
    }
    /* Invalidate first: an interrupted rewrite must not look current. */
    compat_ssrnx_ca_export_stamp(path, fingerprint, false);

    char tmp[SSRNX_STORAGE_PATH_CAP], old[SSRNX_STORAGE_PATH_CAP];
    if (snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= (int)sizeof(tmp) ||
        snprintf(old, sizeof(old), "%s.old", path) >= (int)sizeof(old)) {
        free(buffer); sslExit(); return had_bundle;
    }
    (void)remove(tmp);
    FILE *f = fopen(tmp, "wb");
    unsigned written = 0;
    if (f) {
        for (u32 i = 0; i < total; ++i) {
            if (!compat_ssrnx_ca_cert_exported(&certs[i])) continue;
            if (compat_ssrnx_write_pem_certificate(f, certs[i].cert_data, (size_t)certs[i].cert_size) != 0) break;
            if (android_ca_dir && android_ca_dir[0]) {
                char cert_path[SSRNX_STORAGE_PATH_CAP];
                if (snprintf(cert_path, sizeof(cert_path), "%s/apn64-%08u.pem",
                             android_ca_dir, (unsigned)certs[i].cert_id) < (int)sizeof(cert_path)) {
                    FILE *single = fopen(cert_path, "wb");
                    if (single) {
                        const int cert_rc = compat_ssrnx_write_pem_certificate(single, certs[i].cert_data,
                                                                         (size_t)certs[i].cert_size);
                        const int cert_flush = fflush(single);
                        const int cert_close = fclose(single);
                        if (cert_rc != 0 || cert_flush != 0 || cert_close != 0) (void)remove(cert_path);
                    }
                }
            }
            ++written;
        }
        const int flush_failed = fflush(f) != 0 || ferror(f);
        const int close_failed = fclose(f) != 0;
        if (flush_failed || close_failed) written = 0;
    }
    free(buffer);
    sslExit();
    if (!written) { (void)remove(tmp); return had_bundle; }

    (void)remove(old);
    const bool moved_old = rename(path, old) == 0;
    if (rename(tmp, path) != 0) {
        if (moved_old) (void)rename(old, path);
        (void)remove(tmp);
        return compat_ssrnx_file_nonempty(path);
    }
    (void)remove(old);
    compat_ssrnx_ca_export_stamp(path, fingerprint, true);
    SSRNX_LOGCI("NET", "firmware CA bundle ready path=%s roots=%u", path, written);
    return true;
}
#endif

static void compat_ssrnx_network_prepare_ca_bundle(void) {
    compat_g_network_ca_bundle[0] = '\0';
    compat_g_network_ca_dir[0] = '\0';
    if (!compat_g_storage_app_root[0]) return;
#ifdef __SWITCH__
    if (!compat_ssrnx_prepare_android_ca_dir(compat_g_storage_app_root, compat_g_network_ca_dir, sizeof(compat_g_network_ca_dir))) {
        compat_g_network_ca_dir[0] = '\0';
        SSRNX_LOGCW("NET", "Android CA directory unavailable root=%s errno=%d", compat_g_storage_app_root, errno);
    }
#endif
    char cache_dir[SSRNX_STORAGE_PATH_CAP];
    if (!compat_ssrnx_storage_join(cache_dir, sizeof(cache_dir), compat_g_storage_app_root, "cache")) return;
    /* set_app_storage runs immediately before android_init creates the normal
     * framework directories. Create just the cache leaf here so the trust
     * bundle can be exported early enough for native constructors/OpenSSL. */
    if (mkdir(cache_dir, 0777) != 0 && errno != EEXIST) {
        SSRNX_LOGCW("NET", "CA cache directory unavailable path=%s errno=%d", cache_dir, errno);
        return;
    }
    if (!compat_ssrnx_storage_join(compat_g_network_ca_bundle, sizeof(compat_g_network_ca_bundle),
                            cache_dir, "apn64-cacert.pem")) {
        compat_g_network_ca_bundle[0] = '\0';
        return;
    }
#ifdef __SWITCH__
    pthread_mutex_lock(&compat_g_network_lock);
    if (compat_g_network_ca_state == 0)
        compat_g_network_ca_state = compat_ssrnx_export_firmware_ca_bundle(compat_g_network_ca_bundle, compat_g_network_ca_dir) ? 1 : -1;
    if (compat_g_network_ca_state < 0) { compat_g_network_ca_bundle[0] = '\0'; compat_g_network_ca_dir[0] = '\0'; }
    pthread_mutex_unlock(&compat_g_network_lock);
    if (compat_g_network_ca_bundle[0]) {
        /* Respect ordinary OpenSSL/libcurl discovery before resorting to the
         * absolute-path redirect below. This is process-local Android runtime
         * state and contains no title-specific endpoint knowledge. */
        (void)setenv("SSL_CERT_FILE", compat_g_network_ca_bundle, 1);
        SSRNX_LOGCI("NET", "TLS trust configured SSL_CERT_FILE=%s androidCaDir=/system/etc/security/cacerts",
                    compat_g_network_ca_bundle);
    }
#else
    compat_g_network_ca_bundle[0] = '\0';
    compat_g_network_ca_dir[0] = '\0';
#endif
}

const char *ssrnx_compat_network_ca_bundle(void) {
    return compat_g_network_ca_bundle[0] ? compat_g_network_ca_bundle : NULL;
}

/* Host directory behind the guest path /system/etc/security/cacerts. */
const char *ssrnx_compat_network_ca_dir(void) {
    return compat_g_network_ca_dir[0] ? compat_g_network_ca_dir : NULL;
}

size_t ssrnx_compat_network_ca_count(void) {
#ifdef __SWITCH__
    pthread_mutex_lock(&compat_g_network_lock);
    const size_t count = compat_g_network_ca_root_count;
    pthread_mutex_unlock(&compat_g_network_lock);
    return count;
#else
    return 0;
#endif
}

bool ssrnx_compat_network_ca_copy(size_t index, uint8_t *out, size_t capacity, size_t *out_size) {
#ifdef __SWITCH__
    bool ok = false;
    pthread_mutex_lock(&compat_g_network_lock);
    if (index < compat_g_network_ca_root_count && compat_g_network_ca_roots[index].der) {
        const size_t size = compat_g_network_ca_roots[index].size;
        if (out_size) *out_size = size;
        if (!out || capacity >= size) {
            if (out && size) memcpy(out, compat_g_network_ca_roots[index].der, size);
            ok = true;
        }
    }
    pthread_mutex_unlock(&compat_g_network_lock);
    return ok;
#else
    (void)index; (void)out; (void)capacity;
    if (out_size) *out_size = 0;
    return false;
#endif
}

static int compat_ssrnx_network_ensure_initialized(void) {
#ifdef __SWITCH__
    pthread_mutex_lock(&compat_g_network_lock);
    if (compat_g_network_socket_state == 0) {
        SocketInitConfig config = *socketGetDefaultInitConfig();
        /* Default transfer memory exhausts during ordinary parallel server probes. */
        if (config.tcp_tx_buf_max_size < 0x100000) config.tcp_tx_buf_max_size = 0x100000;
        if (config.tcp_rx_buf_max_size < 0x100000) config.tcp_rx_buf_max_size = 0x100000;
        Result rc = socketInitialize(&config);
        compat_g_network_socket_state = R_SUCCEEDED(rc) ? 1 : -1;
        if (compat_g_network_socket_state < 0)
            SSRNX_LOGCE("NET", "socketInitialize failed rc=0x%08x", rc);
        else
            SSRNX_LOGCI("NET", "socket backend initialized source=socketInitialize tcpTx=%u tcpRx=%u tcpTxMax=%u tcpRxMax=%u buffers=%u sessions=%u",
                        config.tcp_tx_buf_size, config.tcp_rx_buf_size, config.tcp_tx_buf_max_size,
                        config.tcp_rx_buf_max_size, config.sb_efficiency, config.num_bsd_sessions);
    }
    if (compat_g_network_nifm_state == 0) {
        Result rc = nifmInitialize(NifmServiceType_User);
        compat_g_network_nifm_state = R_SUCCEEDED(rc) ? 1 : -1;
        if (compat_g_network_nifm_state < 0)
            SSRNX_LOGCW("NET", "nifmInitialize unavailable rc=0x%08x; BSD fallback remains enabled", rc);
        else
            SSRNX_LOGCI("NET", "NIFM initialized service=user");
    }
    const int ok = compat_g_network_socket_state > 0;
    pthread_mutex_unlock(&compat_g_network_lock);
    if (!ok) { errno = ENETDOWN; return -1; }
#else
    compat_g_network_socket_state = 1;
#endif
    return 0;
}

/* Release BSD sockets and NIFM before the NRO returns to hbloader. libnx does
 * not close them at exit; leaked bsd:u sessions and their transfer memory
 * stay bound to the hbloader process and break the next homebrew's sockets. */
int ssrnx_compat_network_initialize(void) {
    return compat_ssrnx_network_ensure_initialized();
}

void ssrnx_compat_network_shutdown(void) {
#ifdef __SWITCH__
    pthread_mutex_lock(&compat_g_network_lock);
    const bool sockets = compat_g_network_socket_state > 0;
    const bool nifm = compat_g_network_nifm_state > 0;
    if (sockets) socketExit();
    if (nifm) nifmExit();
    compat_g_network_socket_state = 0;
    compat_g_network_nifm_state = 0;
    pthread_mutex_unlock(&compat_g_network_lock);
    if (sockets || nifm) SSRNX_LOGCI("NET", "network shutdown sockets=%d nifm=%d", sockets, nifm);
#endif
}

bool ssrnx_compat_network_is_available(void) {
    if (compat_ssrnx_network_ensure_initialized() != 0) {
        SSRNX_LOGCW("NET", "connectivity query available=0 reason=socket-init errno=%d", errno);
        return false;
    }
#ifdef __SWITCH__
    pthread_mutex_lock(&compat_g_network_lock);
    if (compat_g_network_nifm_state <= 0) {
        pthread_mutex_unlock(&compat_g_network_lock);
        if (compat_ssrnx_network_connectivity_trace_slot())
            SSRNX_LOGCI("NET", "connectivity query socketInit=1 nifm=unavailable available=1 source=bsd-fallback");
        return true; /* BSD initialized but old/limited NIFM service unavailable. */
    }
    const uint64_t now = armGetSystemTick();
    const uint64_t freq = armGetSystemTickFreq();
    if (compat_g_network_status_cached < 0 || !compat_g_network_status_tick || now - compat_g_network_status_tick >= freq) {
        NifmInternetConnectionType type = 0;
        NifmInternetConnectionStatus status = 0;
        u32 strength = 0;
        Result rc = nifmGetInternetConnectionStatus(&type, &strength, &status);
        /* If NIFM status probing itself is unavailable, allow the already
         * initialized BSD stack to make the authoritative connect attempt. */
        compat_g_network_status_cached = R_FAILED(rc) ? 1 : (status == NifmInternetConnectionStatus_Connected);
        if (R_SUCCEEDED(rc)) {
            compat_g_network_transport_cached = type == NifmInternetConnectionType_Ethernet ? 3 : 1;
        }
        compat_g_network_status_tick = now;
        if (compat_ssrnx_network_connectivity_trace_slot())
            SSRNX_LOGCI("NET",
                        "connectivity refresh rc=0x%08x type=%u strength=%u status=%u connected=%d fallback=%d",
                        rc, (unsigned)type, (unsigned)strength, (unsigned)status,
                        compat_g_network_status_cached > 0 ? 1 : 0, R_FAILED(rc) ? 1 : 0);
    }
    const bool available = compat_g_network_status_cached > 0;
    pthread_mutex_unlock(&compat_g_network_lock);
    if (compat_ssrnx_network_connectivity_trace_slot())
        SSRNX_LOGCI("NET", "connectivity query socketInit=1 nifm=1 available=%d", available ? 1 : 0);
    return available;
#else
    if (compat_ssrnx_network_connectivity_trace_slot())
        SSRNX_LOGCI("NET", "connectivity query socketInit=1 available=1 source=host");
    return true;
#endif
}

int ssrnx_compat_network_transport(void) {
    if (!ssrnx_compat_network_is_available()) return -1;
#ifdef __SWITCH__
    pthread_mutex_lock(&compat_g_network_lock);
    const int transport = compat_g_network_transport_cached;
    pthread_mutex_unlock(&compat_g_network_lock);
    return transport;
#else
    return 1;
#endif
}

/* network proxy */
/* Android's default-network HTTP proxy is a snapshot of the NIFM profile. */
int ssrnx_compat_network_proxy(char *host, size_t capacity, uint16_t *port) {
    if (!host || !capacity || !port) { errno = EINVAL; return -1; }
    host[0] = '\0'; *port = 0;
#ifdef __SWITCH__
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    NifmNetworkProfileData profile = {0};
    pthread_mutex_lock(&compat_g_network_lock);
    const Result rc = compat_g_network_nifm_state > 0 ? nifmGetCurrentNetworkProfile(&profile)
        : MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    pthread_mutex_unlock(&compat_g_network_lock);
    if (compat_ssrnx_network_connectivity_trace_slot())
        SSRNX_LOGCI("NET", "proxy profile rc=0x%08x enabled=%u", rc,
                    R_SUCCEEDED(rc) ? profile.ip_setting_data.proxy_setting.enabled : 0);
    if (R_FAILED(rc)) { errno = EIO; return -1; }
    const NifmProxySetting *proxy = &profile.ip_setting_data.proxy_setting;
    if (!proxy->enabled) return 0;
    const char *end = memchr(proxy->server, '\0', sizeof(proxy->server));
    if (!end || end == proxy->server || !proxy->port) { errno = EINVAL; return -1; }
    const size_t length = (size_t)(end - proxy->server);
    if (length >= capacity) { errno = ENAMETOOLONG; return -1; }
    memcpy(host, proxy->server, length + 1);
    *port = proxy->port;
    return 1;
#else
    /* The desktop backend has no Android system/default-network proxy store. */
    return 0;
#endif
}



static size_t compat_ssrnx_network_dns_add(uint32_t *out, size_t capacity, size_t count, uint32_t addr) {
    if (addr == 0 || count >= capacity) return count;
    for (size_t i = 0; i < count; ++i)
        if (out[i] == addr) return count;
    out[count] = addr;
    return count + 1u;
}

size_t ssrnx_compat_network_dns_servers(uint32_t *out, size_t capacity) {
    if (!out || capacity == 0 || !ssrnx_compat_network_is_available()) return 0;
    size_t count = 0;
#ifdef __SWITCH__
    pthread_mutex_lock(&compat_g_network_lock);
    const bool nifm_ready = compat_g_network_nifm_state > 0;
    pthread_mutex_unlock(&compat_g_network_lock);
    u32 addr = 0, mask = 0, gateway = 0, dns1 = 0, dns2 = 0;
    const Result rc = nifm_ready ? nifmGetCurrentIpConfigInfo(&addr, &mask, &gateway, &dns1, &dns2)
                                 : MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (R_SUCCEEDED(rc)) {
        count = compat_ssrnx_network_dns_add(out, capacity, count, dns1);
        count = compat_ssrnx_network_dns_add(out, capacity, count, dns2);
    }
    if (compat_ssrnx_network_connectivity_trace_slot())
        SSRNX_LOGCI("NET", "dns servers rc=0x%08x count=%zu", rc, count);
#else
    FILE *f = fopen("/etc/resolv.conf", "r");
    char line[256];
    while (f && fgets(line, sizeof(line), f)) {
        char addr_text[64];
        struct in_addr addr;
        if (sscanf(line, " nameserver %63s", addr_text) == 1 && inet_pton(AF_INET, addr_text, &addr) == 1)
            count = compat_ssrnx_network_dns_add(out, capacity, count, addr.s_addr);
    }
    if (f) fclose(f);
#endif
    return count;
}

static int compat_ssrnx_android_family_to_host(int family) {
    switch (family) {
        case SSRNX_ANDROID_AF_UNSPEC: return AF_UNSPEC;
        case SSRNX_ANDROID_AF_INET: return AF_INET;
        case SSRNX_ANDROID_AF_INET6: return AF_INET6;
        default: return -1;
    }
}

static int compat_ssrnx_host_family_to_android(int family) {
    if (family == AF_UNSPEC) return SSRNX_ANDROID_AF_UNSPEC;
    if (family == AF_INET) return SSRNX_ANDROID_AF_INET;
    if (family == AF_INET6) return SSRNX_ANDROID_AF_INET6;
    return -1;
}

static int compat_ssrnx_android_socktype_to_host(int type, bool *out_nonblock) {
    const bool nonblock = (type & SSRNX_ANDROID_SOCK_NONBLOCK) != 0;
    int base = type & ~(SSRNX_ANDROID_SOCK_NONBLOCK | SSRNX_ANDROID_SOCK_CLOEXEC);
    int host = -1;
    switch (base) {
        case SSRNX_ANDROID_SOCK_STREAM: host = SOCK_STREAM; break;
        case SSRNX_ANDROID_SOCK_DGRAM: host = SOCK_DGRAM; break;
        case SSRNX_ANDROID_SOCK_RAW: host = SOCK_RAW; break;
#ifdef SOCK_RDM
        case SSRNX_ANDROID_SOCK_RDM: host = SOCK_RDM; break;
#endif
#ifdef SOCK_SEQPACKET
        case SSRNX_ANDROID_SOCK_SEQPACKET: host = SOCK_SEQPACKET; break;
#endif
        default: break;
    }
    if (out_nonblock) *out_nonblock = nonblock;
    return host;
}

static int compat_ssrnx_host_socktype_to_android(int type) {
    int base = type;
#ifdef SOCK_NONBLOCK
    base &= ~SOCK_NONBLOCK;
#endif
#ifdef SOCK_CLOEXEC
    base &= ~SOCK_CLOEXEC;
#endif
    if (base == SOCK_STREAM) return SSRNX_ANDROID_SOCK_STREAM;
    if (base == SOCK_DGRAM) return SSRNX_ANDROID_SOCK_DGRAM;
    if (base == SOCK_RAW) return SSRNX_ANDROID_SOCK_RAW;
#ifdef SOCK_RDM
    if (base == SOCK_RDM) return SSRNX_ANDROID_SOCK_RDM;
#endif
#ifdef SOCK_SEQPACKET
    if (base == SOCK_SEQPACKET) return SSRNX_ANDROID_SOCK_SEQPACKET;
#endif
    return 0;
}

static int compat_ssrnx_android_sockaddr_to_host(const compat_SsrnxAndroidSockaddr *guest, uint32_t guest_len,
                                          struct sockaddr_storage *out, socklen_t *out_len) {
    if (!guest || !out || !out_len) { errno = EFAULT; return -1; }
    memset(out, 0, sizeof(*out));
    if (guest->sa_family == SSRNX_ANDROID_AF_INET) {
        if (guest_len < sizeof(compat_SsrnxAndroidSockaddrIn)) { errno = EINVAL; return -1; }
        const compat_SsrnxAndroidSockaddrIn *g = (const compat_SsrnxAndroidSockaddrIn *)guest;
        struct sockaddr_in *h = (struct sockaddr_in *)out;
#ifdef __SWITCH__
        h->sin_len = (uint8_t)sizeof(*h);
#endif
        h->sin_family = AF_INET;
        h->sin_port = g->sin_port;
        h->sin_addr.s_addr = g->sin_addr;
        *out_len = sizeof(*h);
        return 0;
    }
    if (guest->sa_family == SSRNX_ANDROID_AF_INET6) {
        if (guest_len < sizeof(compat_SsrnxAndroidSockaddrIn6)) { errno = EINVAL; return -1; }
        const compat_SsrnxAndroidSockaddrIn6 *g = (const compat_SsrnxAndroidSockaddrIn6 *)guest;
        struct sockaddr_in6 *h = (struct sockaddr_in6 *)out;
        /* devkitPro/libnx does not expose sin6_len on struct sockaddr_in6.
           The explicit socklen_t passed to connect/bind carries the address
           length, so do not assume a BSD sin6_len field exists. */
        h->sin6_family = AF_INET6;
        h->sin6_port = g->sin6_port;
        h->sin6_flowinfo = g->sin6_flowinfo;
        memcpy(&h->sin6_addr, g->sin6_addr, 16);
        h->sin6_scope_id = g->sin6_scope_id;
        *out_len = sizeof(*h);
        return 0;
    }
    errno = EAFNOSUPPORT;
    return -1;
}

static void compat_ssrnx_network_format_endpoint(const struct sockaddr *addr, socklen_t addr_len,
                                          char *out, size_t cap) {
    if (!out || !cap) return;
    out[0] = '\0';
    if (!addr) { (void)snprintf(out, cap, "<null>"); return; }
    if (addr->sa_family == AF_INET && addr_len >= (socklen_t)sizeof(struct sockaddr_in)) {
        const struct sockaddr_in *a = (const struct sockaddr_in *)addr;
        char ip[INET_ADDRSTRLEN] = {0};
        if (!inet_ntop(AF_INET, &a->sin_addr, ip, sizeof(ip))) (void)snprintf(ip, sizeof(ip), "?");
        (void)snprintf(out, cap, "%s:%u", ip, (unsigned)ntohs(a->sin_port));
        return;
    }
    if (addr->sa_family == AF_INET6 && addr_len >= (socklen_t)sizeof(struct sockaddr_in6)) {
        const struct sockaddr_in6 *a = (const struct sockaddr_in6 *)addr;
        char ip[INET6_ADDRSTRLEN] = {0};
        if (!inet_ntop(AF_INET6, &a->sin6_addr, ip, sizeof(ip))) (void)snprintf(ip, sizeof(ip), "?");
        (void)snprintf(out, cap, "[%s]:%u", ip, (unsigned)ntohs(a->sin6_port));
        return;
    }
    (void)snprintf(out, cap, "family=%d len=%u", (int)addr->sa_family, (unsigned)addr_len);
}

static int compat_ssrnx_host_sockaddr_to_android(const struct sockaddr *host, socklen_t host_len,
                                          compat_SsrnxAndroidSockaddr *guest, uint32_t *inout_len) {
    if (!host || !inout_len) { errno = EFAULT; return -1; }
    uint32_t need = 0;
    if (host->sa_family == AF_INET) need = sizeof(compat_SsrnxAndroidSockaddrIn);
    else if (host->sa_family == AF_INET6) need = sizeof(compat_SsrnxAndroidSockaddrIn6);
    else { errno = EAFNOSUPPORT; return -1; }
    const uint32_t cap = *inout_len;
    *inout_len = need;
    if (!guest) return 0;
    if (cap < need) { errno = EINVAL; return -1; }
    memset(guest, 0, need);
    if (host->sa_family == AF_INET) {
        if (host_len < sizeof(struct sockaddr_in)) { errno = EINVAL; return -1; }
        const struct sockaddr_in *h = (const struct sockaddr_in *)host;
        compat_SsrnxAndroidSockaddrIn *g = (compat_SsrnxAndroidSockaddrIn *)guest;
        g->sin_family = SSRNX_ANDROID_AF_INET; g->sin_port = h->sin_port; g->sin_addr = h->sin_addr.s_addr;
    } else {
        if (host_len < sizeof(struct sockaddr_in6)) { errno = EINVAL; return -1; }
        const struct sockaddr_in6 *h = (const struct sockaddr_in6 *)host;
        compat_SsrnxAndroidSockaddrIn6 *g = (compat_SsrnxAndroidSockaddrIn6 *)guest;
        g->sin6_family = SSRNX_ANDROID_AF_INET6; g->sin6_port = h->sin6_port;
        g->sin6_flowinfo = h->sin6_flowinfo; memcpy(g->sin6_addr, &h->sin6_addr, 16); g->sin6_scope_id = h->sin6_scope_id;
    }
    return 0;
}

static int compat_ssrnx_socket_descriptor_open(int fd, bool cloexec) {
    if (ssrnx_file_contract_socket_open) return ssrnx_file_contract_socket_open(fd, cloexec);
    if (!cloexec) return 0;
    const int flags = fcntl(fd, F_GETFD, 0);
    return flags < 0 ? -1 : fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

static void compat_ssrnx_socket_close_setup(int fd) {
    const int saved = errno;
    if (ssrnx_file_contract_close) ssrnx_file_contract_close(fd);
    uint64_t rx, tx; bool eof;
    (void)compat_ssrnx_network_socket_take(fd, &rx, &tx, &eof);
    close(fd);
    errno = saved;
}

static int compat_ssrnx_bionic_socket(int domain, int type, int protocol) {
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    const int host_domain = compat_ssrnx_android_family_to_host(domain);
    bool nonblock = false;
    const int host_type = compat_ssrnx_android_socktype_to_host(type, &nonblock);
    if (host_domain < 0 || host_type < 0) {
        errno = EAFNOSUPPORT;
        SSRNX_LOGCW("NET", "socket rejected domain=%d type=%#x protocol=%d errno=%d", domain, type, protocol, errno);
        return -1;
    }
    int fd = socket(host_domain, host_type, protocol);
    if (fd < 0) {
        compat_ssrnx_network_fix_errno();
        /* Horizon has no IPv6 sockets: EAFNOSUPPORT, as on a Linux kernel
         * without IPv6; callers fall back to IPv4. Log the guest errno (the
         * host errno now holds the "already imported" marker). */
        const int guest_errno = *ssrnx_guest_errno_location();
        if (domain == SSRNX_ANDROID_AF_INET6 && guest_errno == 97 /* Bionic EAFNOSUPPORT */)
            SSRNX_LOGCI("NET", "socket domain=%d type=%#x protocol=%d result=-1 errno=%d (no IPv6 on host)",
                        domain, type, protocol, guest_errno);
        else
            SSRNX_LOGCW("NET", "socket domain=%d type=%#x protocol=%d result=-1 errno=%d",
                        domain, type, protocol, guest_errno);
        return -1;
    }
    if (nonblock) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
            int saved = errno; close(fd); errno = saved;
            SSRNX_LOGCW("NET", "socket nonblock setup fd=%d result=-1 errno=%d", fd, errno);
            return -1;
        }
    }
    if (compat_ssrnx_socket_descriptor_open(fd, (type & SSRNX_ANDROID_SOCK_CLOEXEC) != 0) != 0) {
        compat_ssrnx_socket_close_setup(fd);
        SSRNX_LOGCW("NET", "socket descriptor setup fd=%d result=-1 errno=%d", fd, errno);
        compat_ssrnx_network_fix_errno();
        return -1;
    }
    compat_ssrnx_network_socket_register(fd);
    if (compat_ssrnx_network_trace_slot())
        SSRNX_LOGCI("NET", "socket domain=%d type=%#x protocol=%d hostDomain=%d hostType=%d fd=%d nonblock=%d",
                    domain, type, protocol, host_domain, host_type, fd, nonblock ? 1 : 0);
    return fd;
}




/* network diagnostics */
/* Ongoing FAST diagnostics: bounded bursts followed by one sample/second.
 * Socket counters outlive the old process-global startup trace budgets. */
typedef struct compat_SsrnxNetworkWaitTrace {
    uint64_t calls, ready, timeouts, errors, last_log_ms, wouldblock_epoch;
} compat_SsrnxNetworkWaitTrace;

typedef struct compat_SsrnxNetworkWaitCall {
    uint64_t sequence, started_ms;
    int tid;
    bool emit;
} compat_SsrnxNetworkWaitCall;

static uint64_t compat_ssrnx_network_diag_clock_ms(void) {
    const int saved = errno;
    struct timespec ts = {0};
    (void)clock_gettime(CLOCK_MONOTONIC, &ts);
    errno = saved;
    return (uint64_t)ts.tv_sec * UINT64_C(1000) + (uint64_t)ts.tv_nsec / UINT64_C(1000000);
}

static bool compat_ssrnx_network_socket_snapshot(int fd, compat_SsrnxNetworkSocketTrace *out) {
    const int saved = errno;
    bool found = false;
    pthread_mutex_lock(&compat_g_network_socket_trace_lock);
    for (size_t i = 0; i < SSRNX_NETWORK_SOCKET_TRACE_MAX; ++i) {
        if (compat_g_network_sockets[i].used && compat_g_network_sockets[i].fd == fd) {
            *out = compat_g_network_sockets[i]; found = true; break;
        }
    }
    pthread_mutex_unlock(&compat_g_network_socket_trace_lock);
    errno = saved;
    return found;
}

static compat_SsrnxNetworkWaitCall compat_ssrnx_network_wait_begin(compat_SsrnxNetworkWaitTrace *state,
    const char *api, uint32_t count, int64_t timeout_us, uintptr_t caller) {
    const int saved = errno;
    const uint64_t now = compat_ssrnx_network_diag_clock_ms();
    const uint64_t epoch = __atomic_load_n(&compat_g_network_wouldblock_epoch, __ATOMIC_RELAXED);
    compat_SsrnxNetworkWaitCall call = { .sequence = ++state->calls, .started_ms = now };
    call.emit = call.sequence <= 16u || epoch != state->wouldblock_epoch ||
                now - state->last_log_ms >= 1000u || compat_ssrnx_memtrace_enabled();
    state->wouldblock_epoch = epoch;
    if (call.emit) {
        state->last_log_ms = now;
        call.tid = ssrnx_bionic_gettid();
        char site[192] = {0}, name[16] = {0};
        (void)ssrnx_compat_describe_guest_address(caller, site, sizeof(site));
        (void)ssrnx_bionic_thread_get_current_name(name);
        SSRNX_LOGCI("NETWAIT", "begin api=%s tid=%d name=%s sequence=%llu atMs=%llu count=%u timeoutUs=%lld pc=%p caller=%s",
                    api, call.tid, name, (unsigned long long)call.sequence,
                    (unsigned long long)now, count, (long long)timeout_us, (void *)caller, site);
    }
    errno = saved;
    return call;
}

static void compat_ssrnx_network_wait_fd(const compat_SsrnxNetworkWaitCall *call, const char *api,
    const char *phase, uint32_t index, int fd, unsigned events, unsigned revents,
    unsigned host_events, unsigned host_revents) {
    if (!call->emit) return;
    const int saved = errno;
    compat_SsrnxNetworkSocketTrace socket = {0};
    (void)compat_ssrnx_network_socket_snapshot(fd, &socket);
    const uint64_t age = socket.last_wouldblock_ms
        ? compat_ssrnx_network_diag_clock_ms() - socket.last_wouldblock_ms : 0;
    SSRNX_LOGCI("NETWAIT", "fd api=%s phase=%s tid=%d sequence=%llu index=%u fd=%d events=%#x revents=%#x hostEvents=%#x hostRevents=%#x virtual=%d rxBytes=%llu txBytes=%llu recvWouldBlock=%llu sendWouldBlock=%llu wouldBlockAgeMs=%llu wouldBlockTid=%d wouldBlockPc=%p",
                api, phase, call->tid, (unsigned long long)call->sequence, index, fd,
                events, revents, host_events, host_revents, ssrnx_fd_is_virtual(fd),
                (unsigned long long)socket.rx_bytes, (unsigned long long)socket.tx_bytes,
                (unsigned long long)socket.recv_wouldblock, (unsigned long long)socket.send_wouldblock,
                (unsigned long long)age, socket.last_wouldblock_tid, (void *)socket.last_wouldblock_caller);
    errno = saved;
}

static void compat_ssrnx_network_wait_end(compat_SsrnxNetworkWaitTrace *state,
    const compat_SsrnxNetworkWaitCall *call, const char *api, int result, int host_errno,
    uint64_t host_calls, int host_result, int host_wait_ms) {
    const int saved = errno;
    if (result > 0) ++state->ready;
    else if (result == 0) ++state->timeouts;
    else ++state->errors;
    if (call->emit) {
        SSRNX_LOGCI("NETWAIT", "end api=%s tid=%d sequence=%llu result=%d hostResult=%d hostErrno=%d guestErrno=%d elapsedMs=%llu hostCalls=%llu lastHostWaitMs=%d calls=%llu readyCalls=%llu timeouts=%llu errors=%llu",
                    api, call->tid, (unsigned long long)call->sequence, result, host_result,
                    host_errno, result < 0 ? compat_ssrnx_host_errno_to_android(host_errno) : 0,
                    (unsigned long long)(compat_ssrnx_network_diag_clock_ms() - call->started_ms),
                    (unsigned long long)host_calls, host_wait_ms, (unsigned long long)state->calls,
                    (unsigned long long)state->ready, (unsigned long long)state->timeouts,
                    (unsigned long long)state->errors);
    }
    errno = saved;
}

static bool compat_ssrnx_network_socket_io_trace(const char *api, int fd, size_t requested,
    ssize_t result, int flags, bool receive, bool trace_untracked, uintptr_t caller) {
    const int saved = errno;
    const int guest_errno = result < 0 ? *ssrnx_guest_errno_location() : 0;
    const uint64_t now = compat_ssrnx_network_diag_clock_ms();
    const int wouldblock_tid = guest_errno == 11 ? ssrnx_bionic_gettid() : 0;
    compat_SsrnxNetworkSocketTrace snapshot = {0};
    bool tracked = false, emit = false;
    pthread_mutex_lock(&compat_g_network_socket_trace_lock);
    for (size_t i = 0; i < SSRNX_NETWORK_SOCKET_TRACE_MAX; ++i) {
        compat_SsrnxNetworkSocketTrace *s = &compat_g_network_sockets[i];
        if (!s->used || s->fd != fd) continue;
        tracked = true;
        ++s->io_calls;
        if (result > 0) {
            if (receive) s->rx_bytes += (uint64_t)result;
            else s->tx_bytes += (uint64_t)result;
        }
        const bool first_eof = receive && result == 0 && requested && !s->eof_seen;
        const bool io_failure = result < 0 && guest_errno != 11;
        const bool first_failure = io_failure && !s->io_failure_seen;
        if (first_eof) s->eof_seen = true;
        if (io_failure) s->io_failure_seen = true;
        bool first_wouldblock = false;
        bool *blocked = receive ? &s->recv_blocked : &s->send_blocked;
        if (result < 0 && guest_errno == 11) {
            uint64_t *count = receive ? &s->recv_wouldblock : &s->send_wouldblock;
            ++*count;
            first_wouldblock = !*blocked;
            *blocked = true;
            s->last_wouldblock_ms = now;
            s->last_wouldblock_tid = wouldblock_tid;
            s->last_wouldblock_caller = caller;
            if (first_wouldblock)
                __atomic_add_fetch(&compat_g_network_wouldblock_epoch, 1u, __ATOMIC_RELAXED);
        } else *blocked = false;
        emit = s->io_calls <= 16u || first_wouldblock || first_eof || first_failure ||
               now - s->last_io_log_ms >= 1000u;
        if (emit) s->last_io_log_ms = now;
        snapshot = *s;
        break;
    }
    pthread_mutex_unlock(&compat_g_network_socket_trace_lock);
    if ((tracked && (emit || compat_ssrnx_memtrace_enabled())) ||
        (!tracked && trace_untracked && compat_ssrnx_network_io_trace_slot())) {
        char site[192] = {0};
        (void)ssrnx_compat_describe_guest_address(caller, site, sizeof(site));
        SSRNX_LOGCI("NET", "%s fd=%d tid=%d requested=%zu result=%zd flags=%#x errno=%d atMs=%llu pc=%p caller=%s ioCalls=%llu rxBytes=%llu txBytes=%llu recvWouldBlock=%llu sendWouldBlock=%llu",
                    api, fd, ssrnx_bionic_gettid(), requested, result, flags, guest_errno,
                    (unsigned long long)now, (void *)caller, site, (unsigned long long)snapshot.io_calls,
                    (unsigned long long)snapshot.rx_bytes, (unsigned long long)snapshot.tx_bytes,
                    (unsigned long long)snapshot.recv_wouldblock, (unsigned long long)snapshot.send_wouldblock);
    }
    errno = saved;
    return tracked;
}



/* blocking io network */

static void compat_ssrnx_flock_release_fd(int fd); /* libc_runtime.inc: flock(2) table */

/* Host calls that can block indefinitely on behalf of guest code are signal
 * safe points (ssrnx_signal_safe_call): a thread parked in one must still
 * take a thread-directed signal, e.g. a collector's stop-the-world request. */
typedef struct compat_SsrnxSafeHostCall {
    int fd;
    int flags;
    void *buf;
    size_t len;
    struct sockaddr *addr;
    socklen_t *addr_len;
    const struct sockaddr *peer;
    socklen_t peer_len;
    struct msghdr *msg;
    struct pollfd *pfds;
    nfds_t nfds;
    int timeout_ms;
    fd_set *rfds;
    fd_set *wfds;
    fd_set *efds;
    struct timeval *tv;
    struct timespec *req;
    struct timespec *rem;
} compat_SsrnxSafeHostCall;

static long compat_ssrnx_safe_connect_run(void *c) { compat_SsrnxSafeHostCall *s = c; return connect(s->fd, s->peer, s->peer_len); }
static long compat_ssrnx_safe_accept_run(void *c) { compat_SsrnxSafeHostCall *s = c; return accept(s->fd, s->addr, s->addr_len); }
static long compat_ssrnx_safe_recv_run(void *c) { compat_SsrnxSafeHostCall *s = c; return (long)recv(s->fd, s->buf, s->len, s->flags); }
static long compat_ssrnx_safe_recvfrom_run(void *c) {
    compat_SsrnxSafeHostCall *s = c;
    return (long)recvfrom(s->fd, s->buf, s->len, s->flags, s->addr, s->addr_len);
}
static long compat_ssrnx_safe_recvmsg_run(void *c) { compat_SsrnxSafeHostCall *s = c; return (long)recvmsg(s->fd, s->msg, s->flags); }
static long compat_ssrnx_safe_poll_run(void *c) { compat_SsrnxSafeHostCall *s = c; return poll(s->pfds, s->nfds, s->timeout_ms); }
static long compat_ssrnx_safe_select_run(void *c) {
    compat_SsrnxSafeHostCall *s = c;
    return select(s->fd, s->rfds, s->wfds, s->efds, s->tv);
}
static long compat_ssrnx_safe_nanosleep_run(void *c) { compat_SsrnxSafeHostCall *s = c; return nanosleep(s->req, s->rem); }

static int compat_ssrnx_safe_connect(int fd, const struct sockaddr *peer, socklen_t len) {
    compat_SsrnxSafeHostCall s = { .fd = fd, .peer = peer, .peer_len = len };
    return (int)ssrnx_signal_safe_call(compat_ssrnx_safe_connect_run, &s);
}
static int compat_ssrnx_safe_accept(int fd, struct sockaddr *addr, socklen_t *addr_len) {
    compat_SsrnxSafeHostCall s = { .fd = fd, .addr = addr, .addr_len = addr_len };
    return (int)ssrnx_signal_safe_call(compat_ssrnx_safe_accept_run, &s);
}
static ssize_t compat_ssrnx_safe_recv(int fd, void *buf, size_t len, int flags) {
    compat_SsrnxSafeHostCall s = { .fd = fd, .buf = buf, .len = len, .flags = flags };
    return (ssize_t)ssrnx_signal_safe_call(compat_ssrnx_safe_recv_run, &s);
}
static ssize_t compat_ssrnx_safe_recvfrom(int fd, void *buf, size_t len, int flags,
                                   struct sockaddr *addr, socklen_t *addr_len) {
    compat_SsrnxSafeHostCall s = { .fd = fd, .buf = buf, .len = len, .flags = flags, .addr = addr, .addr_len = addr_len };
    return (ssize_t)ssrnx_signal_safe_call(compat_ssrnx_safe_recvfrom_run, &s);
}
static ssize_t compat_ssrnx_safe_recvmsg(int fd, struct msghdr *msg, int flags) {
    compat_SsrnxSafeHostCall s = { .fd = fd, .msg = msg, .flags = flags };
    return (ssize_t)ssrnx_signal_safe_call(compat_ssrnx_safe_recvmsg_run, &s);
}
static int compat_ssrnx_safe_poll(struct pollfd *fds, nfds_t nfds, int timeout_ms) {
    compat_SsrnxSafeHostCall s = { .pfds = fds, .nfds = nfds, .timeout_ms = timeout_ms };
    return (int)ssrnx_signal_safe_call(compat_ssrnx_safe_poll_run, &s);
}
static int compat_ssrnx_safe_select(int nfds, fd_set *r, fd_set *w, fd_set *e, struct timeval *tv) {
    compat_SsrnxSafeHostCall s = { .fd = nfds, .rfds = r, .wfds = w, .efds = e, .tv = tv };
    return (int)ssrnx_signal_safe_call(compat_ssrnx_safe_select_run, &s);
}
static int compat_ssrnx_safe_nanosleep(struct timespec *req, struct timespec *rem) {
    compat_SsrnxSafeHostCall s = { .req = req, .rem = rem };
    return (int)ssrnx_signal_safe_call(compat_ssrnx_safe_nanosleep_run, &s);
}

static int compat_ssrnx_bionic_bind(int fd, const compat_SsrnxAndroidSockaddr *addr, uint32_t len) {
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    struct sockaddr_storage h; socklen_t hl = 0;
    if (compat_ssrnx_android_sockaddr_to_host(addr, len, &h, &hl) != 0) return -1;
    return compat_ssrnx_network_result_int(bind(fd, (const struct sockaddr *)&h, hl));
}

static int compat_ssrnx_bionic_connect(int fd, const compat_SsrnxAndroidSockaddr *addr, uint32_t len) {
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    struct sockaddr_storage h; socklen_t hl = 0;
    if (compat_ssrnx_android_sockaddr_to_host(addr, len, &h, &hl) != 0) {
        SSRNX_LOGCW("NET", "connect fd=%d address-translation failed errno=%d", fd, errno);
        return -1;
    }
    char endpoint[INET6_ADDRSTRLEN + 32];
    compat_ssrnx_network_format_endpoint((const struct sockaddr *)&h, hl, endpoint, sizeof(endpoint));
    int rc = compat_ssrnx_safe_connect(fd, (const struct sockaddr *)&h, hl);
    if (rc != 0) compat_ssrnx_network_fix_errno();
    const int guest_errno = rc == 0 ? 0 : *ssrnx_guest_errno_location(); /* Android errno */
    if (compat_ssrnx_network_trace_slot() || (rc != 0 && guest_errno != 115 && guest_errno != 114))
        SSRNX_LOGCI("NET", "connect fd=%d endpoint=%s result=%d errno=%d", fd, endpoint, rc, guest_errno);
    return rc;
}

static int compat_ssrnx_bionic_listen(int fd, int backlog) {
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    return compat_ssrnx_network_result_int(listen(fd, backlog));
}

static int compat_ssrnx_bionic_accept(int fd, compat_SsrnxAndroidSockaddr *addr, uint32_t *addr_len) {
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    struct sockaddr_storage h; socklen_t hl = sizeof(h);
    int out = compat_ssrnx_safe_accept(fd, addr ? (struct sockaddr *)&h : NULL, addr ? &hl : NULL);
    if (out < 0) { compat_ssrnx_network_fix_errno(); return -1; }
    if (addr && addr_len && compat_ssrnx_host_sockaddr_to_android((struct sockaddr *)&h, hl, addr, addr_len) != 0) {
        int saved = errno; close(out); errno = saved; return -1;
    }
    if (compat_ssrnx_socket_descriptor_open(out, false) != 0) {
        compat_ssrnx_socket_close_setup(out); compat_ssrnx_network_fix_errno(); return -1;
    }
    compat_ssrnx_network_socket_register(out);
    return out;
}

static int compat_ssrnx_bionic_accept4(int fd, compat_SsrnxAndroidSockaddr *addr, uint32_t *addr_len, int flags) {
    const int allowed = SSRNX_ANDROID_SOCK_NONBLOCK | SSRNX_ANDROID_SOCK_CLOEXEC;
    if (flags & ~allowed) { errno = EINVAL; return -1; }
    int out = compat_ssrnx_bionic_accept(fd, addr, addr_len);
    if (out < 0) return -1;
    if (flags & SSRNX_ANDROID_SOCK_NONBLOCK) {
        int host_flags = fcntl(out, F_GETFL, 0);
        if (host_flags < 0 || fcntl(out, F_SETFL, host_flags | O_NONBLOCK) != 0) {
            int saved = errno;
            compat_ssrnx_socket_close_setup(out); errno = saved; return -1;
        }
    }
    if (flags & SSRNX_ANDROID_SOCK_CLOEXEC) {
        if (compat_ssrnx_socket_descriptor_open(out, true) != 0) {
            int saved = errno;
            compat_ssrnx_socket_close_setup(out); errno = saved; return -1;
        }
    }
    return out;
}

static int compat_ssrnx_bionic_shutdown(int fd, int how) {
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    return compat_ssrnx_network_result_int(shutdown(fd, how));
}

static int compat_ssrnx_host_socket_level(int level) {
    if (level == SSRNX_ANDROID_SOL_SOCKET) return SOL_SOCKET;
    if (level == SSRNX_ANDROID_IPPROTO_IP) return IPPROTO_IP;
    if (level == SSRNX_ANDROID_IPPROTO_TCP) return IPPROTO_TCP;
#ifdef IPPROTO_IPV6
    if (level == SSRNX_ANDROID_IPPROTO_IPV6) return IPPROTO_IPV6;
#endif
    return -1;
}

static int compat_ssrnx_host_socket_opt(int level, int opt) {
    if (level == SSRNX_ANDROID_SOL_SOCKET) {
        if (opt == SSRNX_ANDROID_SO_REUSEADDR) return SO_REUSEADDR;
        if (opt == SSRNX_ANDROID_SO_TYPE) return SO_TYPE;
        if (opt == SSRNX_ANDROID_SO_ERROR) return SO_ERROR;
        if (opt == SSRNX_ANDROID_SO_BROADCAST) return SO_BROADCAST;
        if (opt == SSRNX_ANDROID_SO_SNDBUF) return SO_SNDBUF;
        if (opt == SSRNX_ANDROID_SO_RCVBUF) return SO_RCVBUF;
        if (opt == SSRNX_ANDROID_SO_KEEPALIVE) return SO_KEEPALIVE;
#ifdef SO_OOBINLINE
        if (opt == SSRNX_ANDROID_SO_OOBINLINE) return SO_OOBINLINE;
#endif
        if (opt == SSRNX_ANDROID_SO_LINGER) return SO_LINGER;
#ifdef SO_REUSEPORT
        if (opt == SSRNX_ANDROID_SO_REUSEPORT) return SO_REUSEPORT;
#endif
#ifdef SO_RCVLOWAT
        if (opt == SSRNX_ANDROID_SO_RCVLOWAT) return SO_RCVLOWAT;
#endif
#ifdef SO_SNDLOWAT
        if (opt == SSRNX_ANDROID_SO_SNDLOWAT) return SO_SNDLOWAT;
#endif
        if (opt == SSRNX_ANDROID_SO_RCVTIMEO) return SO_RCVTIMEO;
        if (opt == SSRNX_ANDROID_SO_SNDTIMEO) return SO_SNDTIMEO;
    } else if (level == SSRNX_ANDROID_IPPROTO_TCP) {
        if (opt == SSRNX_ANDROID_TCP_NODELAY) return TCP_NODELAY;
#ifdef TCP_KEEPIDLE
        if (opt == SSRNX_ANDROID_TCP_KEEPIDLE) return TCP_KEEPIDLE;
#endif
#ifdef TCP_KEEPINTVL
        if (opt == SSRNX_ANDROID_TCP_KEEPINTVL) return TCP_KEEPINTVL;
#endif
#ifdef TCP_KEEPCNT
        if (opt == SSRNX_ANDROID_TCP_KEEPCNT) return TCP_KEEPCNT;
#endif
    } else if (level == SSRNX_ANDROID_IPPROTO_IPV6) {
#ifdef IPV6_V6ONLY
        if (opt == SSRNX_ANDROID_IPV6_V6ONLY) return IPV6_V6ONLY;
#endif
    }
    return -1;
}

static int compat_ssrnx_bionic_setsockopt(int fd, int level, int opt, const void *value, uint32_t value_len) {
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    int hl = compat_ssrnx_host_socket_level(level), ho = compat_ssrnx_host_socket_opt(level, opt);
    if (hl < 0 || ho < 0) {
        /* Do not fake success for unsupported socket options. Bionic/Linux
         * reports ENOPROTOOPT for an option that is not implemented at this
         * protocol level. The guest must be allowed to execute its own normal
         * fallback path. */
        errno = ENOPROTOOPT;
        return -1;
    }
    if (level == SSRNX_ANDROID_SOL_SOCKET &&
        (opt == SSRNX_ANDROID_SO_RCVTIMEO || opt == SSRNX_ANDROID_SO_SNDTIMEO)) {
        if (!value || value_len < sizeof(compat_SsrnxAndroidTimeval)) { errno = EINVAL; return -1; }
        const compat_SsrnxAndroidTimeval *g = (const compat_SsrnxAndroidTimeval *)value;
        struct timeval h = {(time_t)g->tv_sec, (long)g->tv_usec};
        return compat_ssrnx_network_result_int(setsockopt(fd, hl, ho, &h, sizeof(h)));
    }
    return compat_ssrnx_network_result_int(setsockopt(fd, hl, ho, value, (socklen_t)value_len));
}

static int compat_ssrnx_bionic_getsockopt(int fd, int level, int opt, void *value, uint32_t *value_len) {
    if (!value_len) { errno = EFAULT; return -1; }
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    int hl = compat_ssrnx_host_socket_level(level), ho = compat_ssrnx_host_socket_opt(level, opt);
    if (hl < 0 || ho < 0) {
        errno = ENOPROTOOPT;
        return -1;
    }
    if (level == SSRNX_ANDROID_SOL_SOCKET &&
        (opt == SSRNX_ANDROID_SO_RCVTIMEO || opt == SSRNX_ANDROID_SO_SNDTIMEO)) {
        struct timeval h; socklen_t hs = sizeof(h);
        if (getsockopt(fd, hl, ho, &h, &hs) != 0) { compat_ssrnx_network_fix_errno(); return -1; }
        if (!value || *value_len < sizeof(compat_SsrnxAndroidTimeval)) { *value_len = sizeof(compat_SsrnxAndroidTimeval); errno = EINVAL; return -1; }
        compat_SsrnxAndroidTimeval *g = (compat_SsrnxAndroidTimeval *)value;
        g->tv_sec = (int64_t)h.tv_sec; g->tv_usec = (int64_t)h.tv_usec; *value_len = sizeof(*g); return 0;
    }
    if (level == SSRNX_ANDROID_SOL_SOCKET && opt == SSRNX_ANDROID_SO_TYPE) {
        int host_type = 0; socklen_t hs = sizeof(host_type);
        if (getsockopt(fd, hl, ho, &host_type, &hs) != 0) { compat_ssrnx_network_fix_errno(); return -1; }
        if (!value || *value_len < sizeof(int)) { *value_len = sizeof(int); errno = EINVAL; return -1; }
        *(int *)value = compat_ssrnx_host_socktype_to_android(host_type);
        *value_len = sizeof(int);
        return 0;
    }
    socklen_t hs = (socklen_t)*value_len;
    int rc = getsockopt(fd, hl, ho, value, &hs); *value_len = (uint32_t)hs;
    if (rc != 0) { compat_ssrnx_network_fix_errno(); return rc; }
    if (level == SSRNX_ANDROID_SOL_SOCKET && opt == SSRNX_ANDROID_SO_ERROR &&
        value && *value_len >= sizeof(int)) {
        const int saved = errno;
        *(int *)value = compat_ssrnx_host_errno_to_android(*(int *)value);
        if (compat_ssrnx_network_trace_slot())
            SSRNX_LOGCI("NET", "getsockopt SO_ERROR fd=%d value=%d tid=%d pc=%p", fd, *(int *)value,
                        ssrnx_bionic_gettid(), __builtin_extract_return_addr(__builtin_return_address(0)));
        errno = saved;
    }
    return 0;
}

static int compat_ssrnx_android_send_flags_to_host(int flags) {
    int h = 0;
#ifdef MSG_OOB
    if (flags & 0x1) h |= MSG_OOB;
#endif
#ifdef MSG_PEEK
    if (flags & 0x2) h |= MSG_PEEK;
#endif
#ifdef MSG_DONTROUTE
    if (flags & 0x4) h |= MSG_DONTROUTE;
#endif
#ifdef MSG_DONTWAIT
    if (flags & 0x40) h |= MSG_DONTWAIT;
#endif
#ifdef MSG_EOR
    if (flags & 0x80) h |= MSG_EOR;
#endif
#ifdef MSG_WAITALL
    if (flags & 0x100) h |= MSG_WAITALL;
#endif
#ifdef MSG_NOSIGNAL
    if (flags & 0x4000) h |= MSG_NOSIGNAL;
#endif
    return h;
}

static ssize_t compat_ssrnx_bionic_send(int fd, const void *buf, size_t len, int flags) {
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    ssize_t rc = send(fd, buf, len, compat_ssrnx_android_send_flags_to_host(flags));
    if (rc < 0) compat_ssrnx_network_fix_errno();
    (void)compat_ssrnx_network_socket_io_trace("send", fd, len, rc, flags, false, true,
        (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)));
    return rc;
}

static ssize_t compat_ssrnx_bionic_recv(int fd, void *buf, size_t len, int flags) {
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    ssize_t rc = compat_ssrnx_safe_recv(fd, buf, len, compat_ssrnx_android_send_flags_to_host(flags));
    if (rc < 0) compat_ssrnx_network_fix_errno();
    (void)compat_ssrnx_network_socket_io_trace("recv", fd, len, rc, flags, true, true,
        (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)));
    return rc;
}

static int compat_ssrnx_bionic_fcntl(int fd, int cmd, ...) {
    va_list ap;
    if (ssrnx_epoll_control && ssrnx_epoll_control(fd,1,0)!=-2) {
        int arg=0;
        if (cmd==2 || cmd==4 || cmd==0 || cmd==1030) { va_start(ap,cmd);arg=va_arg(ap,int);va_end(ap); }
        return cmd==0 || cmd==1030 ? ssrnx_epoll_duplicate(fd,arg,cmd==1030) : ssrnx_epoll_control(fd,cmd,arg);
    }
    if (ssrnx_eventfd_poll && ssrnx_eventfd_poll(fd, 0) >= 0) {
        int argument = 0;
        if (cmd == SSRNX_ANDROID_F_SETFD || cmd == SSRNX_ANDROID_F_SETFL ||
            cmd == SSRNX_ANDROID_F_DUPFD || cmd == SSRNX_ANDROID_F_DUPFD_CLOEXEC) {
            va_start(ap, cmd); argument = va_arg(ap, int); va_end(ap);
        }
        int result = (cmd == SSRNX_ANDROID_F_DUPFD || cmd == SSRNX_ANDROID_F_DUPFD_CLOEXEC)
            ? ssrnx_eventfd_duplicate(fd, argument, cmd == SSRNX_ANDROID_F_DUPFD_CLOEXEC)
            : ssrnx_eventfd_control(fd, cmd, argument);
        if (result>=0 && (cmd==SSRNX_ANDROID_F_DUPFD || cmd==SSRNX_ANDROID_F_DUPFD_CLOEXEC) && ssrnx_epoll_duplicated)
            ssrnx_epoll_duplicated(fd,result);
        if (result == -2) { errno = EBADF; result = -1; }
        return result;
    }
    if (ssrnx_fd_is_virtual(fd)) {
        if (cmd == SSRNX_ANDROID_F_GETFL) return SSRNX_ANDROID_O_RDONLY;
        if (cmd == SSRNX_ANDROID_F_SETFL || cmd == SSRNX_ANDROID_F_SETFD) {
            va_start(ap, cmd); (void)va_arg(ap, int); va_end(ap); return 0;
        }
        if (cmd == SSRNX_ANDROID_F_GETFD) return 0;
        errno = EINVAL; return -1;
    }
    if (cmd == SSRNX_ANDROID_F_DUPFD || cmd == SSRNX_ANDROID_F_DUPFD_CLOEXEC) {
        va_start(ap, cmd); int minimum = va_arg(ap, int); va_end(ap);
        if (minimum < 0) { errno = EINVAL; return -1; }
        const bool want_cloexec = cmd == SSRNX_ANDROID_F_DUPFD_CLOEXEC;

        /* libnx fsdev has no regular-file duplicate callback. Its generic
         * F_DUPFD path can return a descriptor number that is not usable by
         * fstat/read/mmap. For SSRNX-tracked path-backed regular files, use
         * the generic file-contract fallback which reopens the same path and
         * restores the current offset. */
        if (ssrnx_file_contract_reopen_duplicate) {
            const int reopened = ssrnx_file_contract_reopen_duplicate(fd, minimum, want_cloexec);
            if (reopened >= 0) return reopened;
            if (reopened == -1) return -1;
            /* -2 means untracked: retain the host path for sockets/devices. */
        }

        int host_cmd = F_DUPFD;
#ifdef F_DUPFD_CLOEXEC
        if (want_cloexec) host_cmd = F_DUPFD_CLOEXEC;
#endif
        int out = fcntl(fd, host_cmd, minimum);
#ifndef F_DUPFD_CLOEXEC
        if (out >= 0 && want_cloexec && fcntl(out, F_SETFD, FD_CLOEXEC) != 0) {
            const int saved = errno; close(out); errno = saved; return -1;
        }
#endif
        if (out < 0) { compat_ssrnx_network_fix_errno(); return -1; }
        if (ssrnx_epoll_duplicated) ssrnx_epoll_duplicated(fd,out);
        if (ssrnx_file_contract_dup) ssrnx_file_contract_dup(fd, out);
        return out;
    }
    if (cmd == SSRNX_ANDROID_F_GETFL) {
        int h = ssrnx_file_contract_status_flags ? ssrnx_file_contract_status_flags(fd) : -2;
        if (h == -2) h = fcntl(fd, F_GETFL, 0);
        if (h < 0) { compat_ssrnx_network_fix_errno(); return -1; }
        int g = h & O_ACCMODE;
        if (h & O_NONBLOCK) g |= SSRNX_ANDROID_O_NONBLOCK;
        if (h & O_APPEND) g |= SSRNX_ANDROID_O_APPEND;
        return g;
    }
    if (cmd == SSRNX_ANDROID_F_SETFL) {
        va_start(ap, cmd); int g = va_arg(ap, int); va_end(ap);
        int h = ssrnx_file_contract_status_flags ? ssrnx_file_contract_status_flags(fd) : -2;
        if (h == -2) h = fcntl(fd, F_GETFL, 0);
        if (h < 0) { compat_ssrnx_network_fix_errno(); return -1; }
        h &= ~(O_NONBLOCK | O_APPEND);
        if (g & SSRNX_ANDROID_O_NONBLOCK) h |= O_NONBLOCK;
        if (g & SSRNX_ANDROID_O_APPEND) h |= O_APPEND;
        const int rc = compat_ssrnx_network_result_int(fcntl(fd, F_SETFL, h));
        const int saved = errno;
        compat_SsrnxNetworkSocketTrace socket = {0};
        if (compat_ssrnx_network_socket_snapshot(fd, &socket))
            SSRNX_LOGCI("NET", "fcntl SETFL fd=%d tid=%d guestFlags=%#x hostFlags=%#x nonblock=%d result=%d errno=%d pc=%p",
                        fd, ssrnx_bionic_gettid(), g, h, (h & O_NONBLOCK) != 0, rc,
                        rc < 0 ? *ssrnx_guest_errno_location() : 0,
                        __builtin_extract_return_addr(__builtin_return_address(0)));
        errno = saved;
        return rc;
    }
    if (cmd == SSRNX_ANDROID_F_GETFD) {
        if (ssrnx_file_contract_descriptor_flags) {
            const int flags = ssrnx_file_contract_descriptor_flags(fd, false, 0);
            if (flags >= 0) return (flags & FD_CLOEXEC) ? SSRNX_ANDROID_FD_CLOEXEC : 0;
            if (flags == -1) { compat_ssrnx_network_fix_errno(); return -1; }
        }
        int h = fcntl(fd, F_GETFD, 0); if (h < 0) { compat_ssrnx_network_fix_errno(); return -1; }
        return (h & FD_CLOEXEC) ? SSRNX_ANDROID_FD_CLOEXEC : 0;
    }
    if (cmd == SSRNX_ANDROID_F_SETFD) {
        va_start(ap, cmd); int g = va_arg(ap, int); va_end(ap);
        if (ssrnx_file_contract_descriptor_flags) {
            const int rc = ssrnx_file_contract_descriptor_flags(fd, true, (g & SSRNX_ANDROID_FD_CLOEXEC) ? FD_CLOEXEC : 0);
            if (rc != -2) return compat_ssrnx_network_result_int(rc);
        }
        return compat_ssrnx_network_result_int(fcntl(fd, F_SETFD, (g & SSRNX_ANDROID_FD_CLOEXEC) ? FD_CLOEXEC : 0));
    }
    if (cmd == SSRNX_ANDROID_F_GETLK || cmd == SSRNX_ANDROID_F_SETLK || cmd == SSRNX_ANDROID_F_SETLKW) {
        /* POSIX record locks belong to the process, and locks of one process
         * never conflict with each other. SSRNX hosts a single Android
         * process, so every valid request is granted and F_GETLK always
         * reports the range unlocked. Access-mode checks match Linux. */
        typedef struct { int16_t l_type, l_whence; int64_t l_start, l_len; int32_t l_pid; } compat_SsrnxAndroidFlock;
        _Static_assert(sizeof(compat_SsrnxAndroidFlock) == 32, "Bionic arm64 struct flock size");
        va_start(ap, cmd); compat_SsrnxAndroidFlock *lock = va_arg(ap, compat_SsrnxAndroidFlock *); va_end(ap);
        int h = ssrnx_file_contract_status_flags ? ssrnx_file_contract_status_flags(fd) : -2;
        if (h == -2) h = fcntl(fd, F_GETFL, 0);
        if (h < 0) {
            const int saved = errno;
            static unsigned reports;
            if (__atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 16u)
                SSRNX_LOGCW("FILE", "fcntl record-lock fd=%d cmd=%d status-query failed hostErrno=%d", fd, cmd, saved);
            errno = saved;
            compat_ssrnx_network_fix_errno(); return -1;
        }
        if (!lock) { errno = EFAULT; return -1; }
        if (lock->l_whence < SEEK_SET || lock->l_whence > SEEK_END ||
            lock->l_type < SSRNX_ANDROID_F_RDLCK || lock->l_type > SSRNX_ANDROID_F_UNLCK) {
            errno = EINVAL; return -1;
        }
        int64_t base = 0, start, end;
        if (lock->l_whence == SEEK_CUR) {
            const off_t offset = lseek(fd, 0, SEEK_CUR);
            if (offset == (off_t)-1) { compat_ssrnx_network_fix_errno(); return -1; }
            base = offset;
        } else if (lock->l_whence == SEEK_END) {
            struct stat st;
            if (fstat(fd, &st) != 0) { compat_ssrnx_network_fix_errno(); return -1; }
            base = st.st_size;
        }
        if (__builtin_add_overflow(base, lock->l_start, &start) ||
            __builtin_add_overflow(start, lock->l_len > 0 ? lock->l_len - 1 : lock->l_len, &end)) {
            errno = EOVERFLOW; return -1;
        }
        if (start < 0 || end < 0) { errno = EINVAL; return -1; }
        if (cmd == SSRNX_ANDROID_F_GETLK) {
            if (lock->l_type == SSRNX_ANDROID_F_UNLCK) { errno = EINVAL; return -1; }
            lock->l_type = SSRNX_ANDROID_F_UNLCK;
            return 0;
        }
        const int access = h & O_ACCMODE;
        if ((lock->l_type == SSRNX_ANDROID_F_RDLCK && access == O_WRONLY) ||
            (lock->l_type == SSRNX_ANDROID_F_WRLCK && access == O_RDONLY)) {
            errno = EBADF; return -1;
        }
        return 0;
    }
    errno = EINVAL;
    return -1;
}

static bool compat_ssrnx_android_fd_isset(const compat_SsrnxAndroidFdSet *set, int fd) {
    return set && fd >= 0 && fd < SSRNX_ANDROID_FD_SETSIZE &&
           (set->bits[(unsigned)fd / 64u] & (UINT64_C(1) << ((unsigned)fd & 63u))) != 0;
}

static void compat_ssrnx_android_fd_set(compat_SsrnxAndroidFdSet *set, int fd) {
    set->bits[(unsigned)fd / 64u] |= UINT64_C(1) << ((unsigned)fd & 63u);
}

static void compat_ssrnx_bionic_FD_SET_chk(int fd, compat_SsrnxAndroidFdSet *set, size_t set_size) {
    if (!set || fd < 0 || fd >= SSRNX_ANDROID_FD_SETSIZE || set_size < sizeof(*set)) compat_ssrnx_fortify_fail("__FD_SET_chk");
    compat_ssrnx_android_fd_set(set, fd);
}

static int compat_ssrnx_bionic_FD_ISSET_chk(int fd, const compat_SsrnxAndroidFdSet *set, size_t set_size) {
    if (!set || fd < 0 || fd >= SSRNX_ANDROID_FD_SETSIZE || set_size < sizeof(*set))
        compat_ssrnx_fortify_fail("__FD_ISSET_chk");
    return compat_ssrnx_android_fd_isset(set, fd) ? 1 : 0;
}

static int compat_ssrnx_bionic_select_poll(int nfds, compat_SsrnxAndroidFdSet *readfds, compat_SsrnxAndroidFdSet *writefds,
                                    compat_SsrnxAndroidFdSet *exceptfds, compat_SsrnxAndroidTimeval *timeout, uintptr_t caller);

static int compat_ssrnx_bionic_select(int nfds, compat_SsrnxAndroidFdSet *readfds, compat_SsrnxAndroidFdSet *writefds,
                               compat_SsrnxAndroidFdSet *exceptfds, compat_SsrnxAndroidTimeval *timeout) {
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    if (nfds < 0 || nfds > SSRNX_ANDROID_FD_SETSIZE ||
        (timeout && (timeout->tv_sec < 0 || timeout->tv_usec < 0 || timeout->tv_usec >= 1000000))) {
        static unsigned invalid_reports;
        if (__atomic_fetch_add(&invalid_reports, 1u, __ATOMIC_RELAXED) < 8u)
            SSRNX_LOGCW("NET", "select rejected nfds=%d guestLimit=%d hostLimit=%d timeoutSec=%lld timeoutUs=%lld",
                        nfds, SSRNX_ANDROID_FD_SETSIZE, FD_SETSIZE,
                        timeout ? (long long)timeout->tv_sec : -1LL,
                        timeout ? (long long)timeout->tv_usec : -1LL);
        errno = EINVAL;
        return -1;
    }
    /* newlib fd_set has only 64 bits; Bionic callers can use all 1024. */
    if (nfds > FD_SETSIZE)
        return compat_ssrnx_bionic_select_poll(nfds, readfds, writefds, exceptfds, timeout,
            (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)));
    extern int ssrnx_compat_epoll_poll(int,short) __attribute__((weak));
    for (int fd=0;fd<nfds && ssrnx_compat_epoll_poll;++fd)
        if ((compat_ssrnx_android_fd_isset(readfds,fd) || compat_ssrnx_android_fd_isset(writefds,fd) || compat_ssrnx_android_fd_isset(exceptfds,fd)) &&
            ssrnx_compat_epoll_poll(fd,0)>=0)
            return compat_ssrnx_bionic_select_poll(nfds,readfds,writefds,exceptfds,timeout,(uintptr_t)__builtin_return_address(0));
    for (int fd = 0; fd < nfds && ssrnx_eventfd_poll; ++fd)
        if ((compat_ssrnx_android_fd_isset(readfds, fd) || compat_ssrnx_android_fd_isset(writefds, fd) ||
             compat_ssrnx_android_fd_isset(exceptfds, fd)) && ssrnx_eventfd_poll(fd, 0) >= 0)
            return compat_ssrnx_bionic_select_poll(nfds, readfds, writefds, exceptfds, timeout,
                (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)));
    int64_t trace_timeout_us = timeout ? -2 : -1;
    if (timeout && timeout->tv_sec >= 0 && timeout->tv_sec < INT64_MAX / INT64_C(1000000) &&
        timeout->tv_usec >= 0 && timeout->tv_usec < INT64_C(1000000))
        trace_timeout_us = timeout->tv_sec * INT64_C(1000000) + timeout->tv_usec;
    static _Thread_local compat_SsrnxNetworkWaitTrace trace_state;
    const compat_SsrnxNetworkWaitCall trace = compat_ssrnx_network_wait_begin(&trace_state, "select", (uint32_t)nfds,
        trace_timeout_us,
        (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)));
    fd_set hr, hw, he; fd_set *pr = NULL, *pw = NULL, *pe = NULL;
    if (readfds) { FD_ZERO(&hr); pr = &hr; }
    if (writefds) { FD_ZERO(&hw); pw = &hw; }
    if (exceptfds) { FD_ZERO(&he); pe = &he; }
    for (int fd = 0; fd < nfds; ++fd) {
        if (readfds && compat_ssrnx_android_fd_isset(readfds, fd)) FD_SET(fd, &hr);
        if (writefds && compat_ssrnx_android_fd_isset(writefds, fd)) FD_SET(fd, &hw);
        if (exceptfds && compat_ssrnx_android_fd_isset(exceptfds, fd)) FD_SET(fd, &he);
        const unsigned events = (compat_ssrnx_android_fd_isset(readfds, fd) ? 1u : 0u) |
                                (compat_ssrnx_android_fd_isset(writefds, fd) ? 4u : 0u) |
                                (compat_ssrnx_android_fd_isset(exceptfds, fd) ? 2u : 0u);
        if (events) compat_ssrnx_network_wait_fd(&trace, "select", "begin", (uint32_t)fd, fd, events, 0, events, 0);
    }
    struct timeval ht, *pt = NULL;
    if (timeout) { ht.tv_sec = (time_t)timeout->tv_sec; ht.tv_usec = (long)timeout->tv_usec; pt = &ht; }
    int rc = compat_ssrnx_safe_select(nfds, pr, pw, pe, pt);
    const int host_errno = rc < 0 ? errno : 0;
    if (timeout) { timeout->tv_sec = (int64_t)ht.tv_sec; timeout->tv_usec = (int64_t)ht.tv_usec; }
    compat_ssrnx_network_wait_end(&trace_state, &trace, "select", rc, host_errno, 1, rc, -1);
    for (int fd = 0; trace.emit && fd < nfds; ++fd) {
        const unsigned events = (compat_ssrnx_android_fd_isset(readfds, fd) ? 1u : 0u) |
                                (compat_ssrnx_android_fd_isset(writefds, fd) ? 4u : 0u) |
                                (compat_ssrnx_android_fd_isset(exceptfds, fd) ? 2u : 0u);
        const unsigned revents = rc < 0 ? 0u :
            ((pr && FD_ISSET(fd, &hr) ? 1u : 0u) | (pw && FD_ISSET(fd, &hw) ? 4u : 0u) |
             (pe && FD_ISSET(fd, &he) ? 2u : 0u));
        if (events) compat_ssrnx_network_wait_fd(&trace, "select", rc < 0 ? "error" : "end",
                                         (uint32_t)fd, fd, events, revents, events, revents);
    }
    if (rc < 0) {
        compat_ssrnx_network_fix_errno();
        const int saved = errno;
        SSRNX_LOGCW("NET", "select nfds=%d result=%d errno=%d", nfds, rc, errno);
        errno = saved;
        return rc;
    }
    if (readfds) memset(readfds, 0, sizeof(*readfds));
    if (writefds) memset(writefds, 0, sizeof(*writefds));
    if (exceptfds) memset(exceptfds, 0, sizeof(*exceptfds));
    for (int fd = 0; fd < nfds; ++fd) {
        if (readfds && FD_ISSET(fd, &hr)) compat_ssrnx_android_fd_set(readfds, fd);
        if (writefds && FD_ISSET(fd, &hw)) compat_ssrnx_android_fd_set(writefds, fd);
        if (exceptfds && FD_ISSET(fd, &he)) compat_ssrnx_android_fd_set(exceptfds, fd);
    }
    const int saved = errno;
    if (compat_ssrnx_network_trace_slot())
        SSRNX_LOGCI("NET", "select nfds=%d result=%d timeoutRemainingUs=%lld",
                    nfds, rc, timeout ? (long long)(timeout->tv_sec * INT64_C(1000000) + timeout->tv_usec) : -1LL);
    errno = saved;
    return rc;
}

static int compat_ssrnx_android_ai_flags_to_host(int flags) {
    int h = 0;
#ifdef AI_PASSIVE
    if (flags & 0x1) h |= AI_PASSIVE;
#endif
#ifdef AI_CANONNAME
    if (flags & 0x2) h |= AI_CANONNAME;
#endif
#ifdef AI_NUMERICHOST
    if (flags & 0x4) h |= AI_NUMERICHOST;
#endif
#ifdef AI_V4MAPPED
    if (flags & 0x800) h |= AI_V4MAPPED;
#endif
#ifdef AI_ALL
    if (flags & 0x100) h |= AI_ALL;
#endif
#ifdef AI_ADDRCONFIG
    if (flags & 0x400) h |= AI_ADDRCONFIG;
#endif
#ifdef AI_NUMERICSERV
    if (flags & 0x8) h |= AI_NUMERICSERV;
#endif
    return h;
}

static int compat_ssrnx_host_eai_to_android(int rc) {
    if (rc == 0) return 0;
#ifdef EAI_BADFLAGS
    if (rc == EAI_BADFLAGS) return 3;
#endif
#ifdef EAI_NONAME
    if (rc == EAI_NONAME) return 8;
#endif
#ifdef EAI_AGAIN
    if (rc == EAI_AGAIN) return 2;
#endif
#ifdef EAI_FAIL
    if (rc == EAI_FAIL) return 4;
#endif
#ifdef EAI_FAMILY
    if (rc == EAI_FAMILY) return 5;
#endif
#ifdef EAI_SOCKTYPE
    if (rc == EAI_SOCKTYPE) return 10;
#endif
#ifdef EAI_SERVICE
    if (rc == EAI_SERVICE) return 9;
#endif
#ifdef EAI_MEMORY
    if (rc == EAI_MEMORY) return 6;
#endif
#ifdef EAI_SYSTEM
    if (rc == EAI_SYSTEM) return 11;
#endif
#ifdef EAI_OVERFLOW
    if (rc == EAI_OVERFLOW) return 14;
#endif
#ifdef EAI_ADDRFAMILY
    if (rc == EAI_ADDRFAMILY) return 1;
#endif
#ifdef EAI_NODATA
    if (rc == EAI_NODATA) return 7;
#endif
#ifdef EAI_BADHINTS
    if (rc == EAI_BADHINTS) return 12;
#endif
#ifdef EAI_PROTOCOL
    if (rc == EAI_PROTOCOL) return 13;
#endif
    return 4;
}

static void compat_ssrnx_bionic_freeaddrinfo(compat_SsrnxAndroidAddrinfo *list) {
    while (list) {
        compat_SsrnxAndroidAddrinfo *next = list->ai_next;
        free(list->ai_addr); free(list->ai_canonname); free(list); list = next;
    }
}

static int compat_ssrnx_host_ai_flags_to_android(int flags) {
    int g = 0;
#ifdef AI_PASSIVE
    if (flags & AI_PASSIVE) g |= 0x1;
#endif
#ifdef AI_CANONNAME
    if (flags & AI_CANONNAME) g |= 0x2;
#endif
#ifdef AI_NUMERICHOST
    if (flags & AI_NUMERICHOST) g |= 0x4;
#endif
#ifdef AI_NUMERICSERV
    if (flags & AI_NUMERICSERV) g |= 0x8;
#endif
#ifdef AI_ALL
    if (flags & AI_ALL) g |= 0x100;
#endif
#ifdef AI_ADDRCONFIG
    if (flags & AI_ADDRCONFIG) g |= 0x400;
#endif
#ifdef AI_V4MAPPED
    if (flags & AI_V4MAPPED) g |= 0x800;
#endif
    return g;
}

/* Numeric endpoints need no DNS IPC. -1 means this request needs the host resolver. */
static int compat_ssrnx_getaddrinfo_numeric(const char *node, const char *service,
                                     const compat_SsrnxAndroidAddrinfo *hints, compat_SsrnxAndroidAddrinfo **out) {
    if (!node) return -1;
    const int flags = hints ? hints->ai_flags : 0;
    const int family = hints ? hints->ai_family : 0;
    const int type = hints ? hints->ai_socktype : 0;
    const int protocol = hints ? hints->ai_protocol : 0;
    if (type != 0 && type != 1 && type != 2) return -1;
    if (protocol != 0 && protocol != IPPROTO_TCP && protocol != IPPROTO_UDP) return -1;
#ifndef __SWITCH__
    if (flags & 0x400) return -1; /* The host can inspect all its configured families. */
#endif
    unsigned char address[16] = {0};
    int actual_family;
    if (inet_pton(AF_INET, node, address) == 1) actual_family = 2;
    else if (inet_pton(AF_INET6, node, address) == 1) actual_family = 10;
    else {
        if ((flags & 4) && !strchr(node, '%')) return 8;
        return -1; /* Scoped literals and names retain native resolution. */
    }
    if (family && family != actual_family) {
        if (family != 10 || actual_family != 2 || !(flags & 0x800)) return 7;
        memmove(address + 12, address, 4);
        memset(address, 0, 10); address[10] = address[11] = 0xff;
        actual_family = 10;
    }
#ifdef __SWITCH__
    /* Horizon's BSD socket service has no configured IPv6 transport. */
    if ((flags & 0x400) && actual_family == 10) return 7;
#endif
    unsigned port = 0;
    if (service) {
        if (!service[0]) return -1;
        for (const unsigned char *p = (const unsigned char *)service; *p; ++p) {
            if (*p < '0' || *p > '9') return flags & 8 ? 8 : -1;
            port = port * 10u + (*p - '0');
            if (port > 65535u) return 8;
        }
    }
    if ((type == 1 && protocol == IPPROTO_UDP) || (type == 2 && protocol == IPPROTO_TCP)) return 12;
    compat_SsrnxAndroidAddrinfo **tail = out;
    for (int socket_type = 1; socket_type <= 2; ++socket_type) {
        const int socket_protocol = socket_type == 1 ? IPPROTO_TCP : IPPROTO_UDP;
        if ((type && type != socket_type) || (protocol && protocol != socket_protocol)) continue;
        compat_SsrnxAndroidAddrinfo *g = calloc(1, sizeof(*g));
        if (!g) goto memory_error;
        g->ai_flags = flags; g->ai_family = actual_family;
        g->ai_socktype = socket_type; g->ai_protocol = socket_protocol;
        g->ai_addrlen = actual_family == 2 ? sizeof(compat_SsrnxAndroidSockaddrIn) : sizeof(compat_SsrnxAndroidSockaddrIn6);
        g->ai_addr = calloc(1, g->ai_addrlen);
        if (!g->ai_addr) { free(g); goto memory_error; }
        if (actual_family == 2) {
            compat_SsrnxAndroidSockaddrIn *sa = (compat_SsrnxAndroidSockaddrIn *)g->ai_addr;
            sa->sin_family = 2; sa->sin_port = htons((uint16_t)port);
            memcpy(&sa->sin_addr, address, 4);
        } else {
            compat_SsrnxAndroidSockaddrIn6 *sa = (compat_SsrnxAndroidSockaddrIn6 *)g->ai_addr;
            sa->sin6_family = 10; sa->sin6_port = htons((uint16_t)port);
            memcpy(sa->sin6_addr, address, 16);
        }
        if ((flags & 2) && !*out) {
            g->ai_canonname = strdup(node);
            if (!g->ai_canonname) { free(g->ai_addr); free(g); goto memory_error; }
        }
        *tail = g; tail = &g->ai_next;
    }
    return 0;
memory_error:
    compat_ssrnx_bionic_freeaddrinfo(*out); *out = NULL;
    return 6;
}

static int compat_ssrnx_bionic_getaddrinfo(const char *node, const char *service,
                                    const compat_SsrnxAndroidAddrinfo *guest_hints,
                                    compat_SsrnxAndroidAddrinfo **out) {
    if (!out) { errno = EINVAL; compat_ssrnx_network_fix_errno(); return 11; }
    *out = NULL;
    if (!node && !service) return 8;
    if (compat_ssrnx_network_ensure_initialized() != 0) return 11;
    struct addrinfo hh, *php = NULL;
    memset(&hh, 0, sizeof(hh));
    if (guest_hints) {
        if (guest_hints->ai_addrlen || guest_hints->ai_canonname || guest_hints->ai_addr || guest_hints->ai_next) return 12;
        if (guest_hints->ai_flags & ~0xd0f) return 3;
        if (guest_hints->ai_socktype & (SSRNX_ANDROID_SOCK_NONBLOCK | SSRNX_ANDROID_SOCK_CLOEXEC)) return 10;
        hh.ai_flags = compat_ssrnx_android_ai_flags_to_host(guest_hints->ai_flags);
        hh.ai_family = compat_ssrnx_android_family_to_host(guest_hints->ai_family);
        bool ignored = false;
        hh.ai_socktype = guest_hints->ai_socktype ? compat_ssrnx_android_socktype_to_host(guest_hints->ai_socktype, &ignored) : 0;
        hh.ai_protocol = guest_hints->ai_protocol;
        if (hh.ai_family < 0) return 5;
        if (guest_hints->ai_socktype && hh.ai_socktype < 0) return 10;
        php = &hh;
    }
    struct addrinfo *host = NULL;
    const bool trace = compat_ssrnx_network_trace_slot();
    const int before_trace_errno = errno;
    if (trace)
        SSRNX_LOGCI("NET", "getaddrinfo begin host=%s service=%s family=%d socktype=%d protocol=%d guestFlags=0x%x hostFlags=0x%x",
                    node ? node : "<null>", service ? service : "<null>",
                    php ? php->ai_family : 0, php ? php->ai_socktype : 0, php ? php->ai_protocol : 0,
                    guest_hints ? guest_hints->ai_flags : 0, hh.ai_flags);
    errno = before_trace_errno;
    const int numeric_rc = compat_ssrnx_getaddrinfo_numeric(node, service, guest_hints, out);
    if (numeric_rc != -1) {
        const int saved = errno;
        if (trace || numeric_rc)
            SSRNX_LOGCI("NET", "getaddrinfo numeric host=%s service=%s guestRc=%d family=%d",
                        node, service ? service : "<null>", numeric_rc, *out ? (*out)->ai_family : 0);
        errno = saved;
        return numeric_rc;
    }
    int hrc = getaddrinfo(node, service, php, &host);
    const int host_errno = errno;
#ifdef EAI_SYSTEM
    if (hrc == EAI_SYSTEM) compat_ssrnx_network_fix_errno();
#endif
    if (hrc != 0) {
        const int guest_rc = compat_ssrnx_host_eai_to_android(hrc);
        const int saved = errno;
        SSRNX_LOGCW("NET", "getaddrinfo failed host=%s service=%s hostRc=%d guestRc=%d hostErrno=%d guestErrno=%d",
                    node ? node : "<null>", service ? service : "<null>", hrc, guest_rc, host_errno,
                    hrc == EAI_SYSTEM ? *ssrnx_guest_errno_location() : 0);
        errno = saved;
        return guest_rc;
    }
    compat_SsrnxAndroidAddrinfo **tail = out;
    unsigned result_count = 0;
    for (struct addrinfo *it = host; it; it = it->ai_next) {
        int gf = compat_ssrnx_host_family_to_android(it->ai_family);
        if (gf < 0) continue;
        compat_SsrnxAndroidAddrinfo *g = (compat_SsrnxAndroidAddrinfo *)calloc(1, sizeof(*g));
        if (!g) { freeaddrinfo(host); compat_ssrnx_bionic_freeaddrinfo(*out); *out = NULL; return 6; }
        g->ai_flags = compat_ssrnx_host_ai_flags_to_android(it->ai_flags); g->ai_family = gf;
        g->ai_socktype = compat_ssrnx_host_socktype_to_android(it->ai_socktype); g->ai_protocol = it->ai_protocol;
        uint32_t gl = it->ai_family == AF_INET ? sizeof(compat_SsrnxAndroidSockaddrIn) : sizeof(compat_SsrnxAndroidSockaddrIn6);
        g->ai_addr = (compat_SsrnxAndroidSockaddr *)calloc(1, gl);
        if (!g->ai_addr) { free(g); freeaddrinfo(host); compat_ssrnx_bionic_freeaddrinfo(*out); *out = NULL; return 6; }
        uint32_t cap = gl;
        if (compat_ssrnx_host_sockaddr_to_android(it->ai_addr, (socklen_t)it->ai_addrlen, g->ai_addr, &cap) != 0) {
            free(g->ai_addr); free(g); continue;
        }
        g->ai_addrlen = cap;
        if (it->ai_canonname) {
            size_t n = strlen(it->ai_canonname) + 1u; g->ai_canonname = (char *)malloc(n);
            if (!g->ai_canonname) { free(g->ai_addr); free(g); freeaddrinfo(host); compat_ssrnx_bionic_freeaddrinfo(*out); *out = NULL; return 6; }
            memcpy(g->ai_canonname, it->ai_canonname, n);
        }
        *tail = g; tail = &g->ai_next;
        ++result_count;
    }
    freeaddrinfo(host);
    if (!*out) {
        SSRNX_LOGCW("NET", "getaddrinfo produced no Android-compatible addresses host=%s service=%s",
                    node ? node : "<null>", service ? service : "<null>");
        return 5;
    }
    if (trace)
        SSRNX_LOGCI("NET", "getaddrinfo complete host=%s service=%s results=%u",
                    node ? node : "<null>", service ? service : "<null>", result_count);
    return 0;
}

static void compat_ssrnx_storage_stream_open(FILE *file, const char *host_path, const char *mode) {
    if (!file || !host_path || !host_path[0]) return;
    pthread_mutex_lock(&compat_g_storage_stream_lock);
    for (size_t i = 0; i < SSRNX_STORAGE_STREAM_MAX; ++i) {
        if (!compat_g_storage_streams[i].file) {
            compat_g_storage_streams[i].file = file;
            compat_g_storage_streams[i].bytes_written = 0;
            compat_g_storage_streams[i].host_flags = strchr(mode, '+') ? O_RDWR :
                (mode[0] == 'r' ? O_RDONLY : O_WRONLY);
            if (mode[0] == 'a') compat_g_storage_streams[i].host_flags |= O_APPEND;
            snprintf(compat_g_storage_streams[i].host_path, sizeof(compat_g_storage_streams[i].host_path), "%s", host_path);
            break;
        }
    }
    pthread_mutex_unlock(&compat_g_storage_stream_lock);
}

static void compat_ssrnx_storage_stream_add(FILE *file, uint64_t bytes) {
    if (!file || !bytes) return;
    pthread_mutex_lock(&compat_g_storage_stream_lock);
    for (size_t i = 0; i < SSRNX_STORAGE_STREAM_MAX; ++i) {
        if (compat_g_storage_streams[i].file == file) {
            compat_g_storage_streams[i].bytes_written += bytes;
            break;
        }
    }
    pthread_mutex_unlock(&compat_g_storage_stream_lock);
}

static bool compat_ssrnx_storage_stream_take(FILE *file, char *host_path, size_t host_cap, uint64_t *bytes) {
    bool found = false;
    if (host_path && host_cap) host_path[0] = '\0';
    if (bytes) *bytes = 0;
    pthread_mutex_lock(&compat_g_storage_stream_lock);
    for (size_t i = 0; i < SSRNX_STORAGE_STREAM_MAX; ++i) {
        if (compat_g_storage_streams[i].file != file) continue;
        if (host_path && host_cap)
            snprintf(host_path, host_cap, "%s", compat_g_storage_streams[i].host_path);
        if (bytes) *bytes = compat_g_storage_streams[i].bytes_written;
        memset(&compat_g_storage_streams[i], 0, sizeof(compat_g_storage_streams[i]));
        found = true;
        break;
    }
    pthread_mutex_unlock(&compat_g_storage_stream_lock);
    return found;
}

/* POSIX stat() of a file that is open elsewhere in the process must succeed.
 * Horizon's fsdev stat() opens the file a second time to query its size, which
 * the filesystem rejects (EIO) while another handle holds it open for writing.
 * Answer from the already-open descriptor instead of surfacing that host
 * locking artefact. */
static int compat_ssrnx_storage_stat_open_file(const char *host_path, struct stat *st) {
    int fd = -1;
    pthread_mutex_lock(&compat_g_storage_stream_lock);
    for (size_t i = 0; i < SSRNX_STORAGE_STREAM_MAX; ++i) {
        if (compat_g_storage_streams[i].file && strcmp(compat_g_storage_streams[i].host_path, host_path) == 0) {
            fd = fileno(compat_g_storage_streams[i].file);
            if (fd >= 0) {
                const int rc = fstat(fd, st);
                pthread_mutex_unlock(&compat_g_storage_stream_lock);
                return rc;
            }
        }
    }
    pthread_mutex_unlock(&compat_g_storage_stream_lock);
    fd = ssrnx_file_contract_fd_for_host_path ? ssrnx_file_contract_fd_for_host_path(host_path) : -1;
    if (fd < 0) { errno = EIO; return -1; }
    return fstat(fd, st);
}

static int compat_ssrnx_storage_stat(const char *host_path, struct stat *st) {
    if (ssrnx_path_lookup_missing_parent && ssrnx_path_lookup_missing_parent(host_path)) return -1;
    int rc = stat(host_path, st);
    if (rc != 0 && errno == ENOENT && ssrnx_path_lookup_record_missing_parent)
        ssrnx_path_lookup_record_missing_parent(host_path);
    if (rc != 0 && errno == EIO) {
        const int saved = errno;
        rc = compat_ssrnx_storage_stat_open_file(host_path, st);
        if (rc != 0) errno = saved;
    }
    return rc;
}

static FILE *compat_ssrnx_host_file(void *guest_file) {
    if (guest_file == &compat_g_bionic_sF[0]) return stdin;
    if (guest_file == &compat_g_bionic_sF[1]) return stdout;
    if (guest_file == &compat_g_bionic_sF[2]) return stderr;
    return (FILE *)guest_file;
}
/* tmpfile(3): an anonymous w+ file removed when closed. Horizon cannot delete
 * a file that is still open, so the path is kept and removed by fclose. */
#define SSRNX_TMPFILE_CAP 32u
typedef struct compat_SsrnxTmpFile {
    FILE *file;
    char path[SSRNX_STORAGE_PATH_CAP];
} compat_SsrnxTmpFile;
static compat_SsrnxTmpFile compat_g_tmpfiles[SSRNX_TMPFILE_CAP];
static pthread_mutex_t compat_g_tmpfile_lock = PTHREAD_MUTEX_INITIALIZER;

static void *compat_ssrnx_bionic_tmpfile(void) {
    char dir[SSRNX_STORAGE_PATH_CAP], path[SSRNX_STORAGE_PATH_CAP];
    if (!compat_g_storage_app_root[0] || !compat_ssrnx_storage_join(dir, sizeof(dir), compat_g_storage_app_root, "cache") ||
        !compat_ssrnx_storage_join(path, sizeof(path), dir, "tmpfile.XXXXXX")) {
        errno = ENOENT;
        return NULL;
    }
    if (mkdir(dir, 0777) == 0) {
        if (ssrnx_path_lookup_changed) ssrnx_path_lookup_changed(dir);
    } else if (errno != EEXIST) return NULL;
    const int fd = mkstemp(path);
    if (fd < 0) return NULL;
    FILE *f = fdopen(fd, "w+");
    if (!f) {
        const int saved = errno;
        close(fd);
        unlink(path);
        errno = saved;
        return NULL;
    }
    if (unlink(path) == 0) return f; /* hosts that allow it drop the name now */
    SSRNX_PERF_LOCK(&compat_g_tmpfile_lock);
    for (size_t i = 0; i < SSRNX_TMPFILE_CAP; ++i) {
        if (compat_g_tmpfiles[i].file) continue;
        compat_g_tmpfiles[i].file = f;
        snprintf(compat_g_tmpfiles[i].path, sizeof(compat_g_tmpfiles[i].path), "%s", path);
        pthread_mutex_unlock(&compat_g_tmpfile_lock);
        return f;
    }
    pthread_mutex_unlock(&compat_g_tmpfile_lock);
    return f; /* table full: the file outlives the stream, as a leaked temp would */
}

static bool compat_ssrnx_tmpfile_take(FILE *f, char *out_path, size_t out_cap) {
    bool found = false;
    SSRNX_PERF_LOCK(&compat_g_tmpfile_lock);
    for (size_t i = 0; i < SSRNX_TMPFILE_CAP && !found; ++i) {
        if (compat_g_tmpfiles[i].file != f) continue;
        snprintf(out_path, out_cap, "%s", compat_g_tmpfiles[i].path);
        compat_g_tmpfiles[i].file = NULL;
        found = true;
    }
    pthread_mutex_unlock(&compat_g_tmpfile_lock);
    return found;
}

static int compat_ssrnx_bionic_fclose(void *stream) {
    SSRNX_IO_TIMING();
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    FILE *f = compat_ssrnx_host_file(stream);
    if (f == stdin || f == stdout || f == stderr) return fflush(f);
    char host_path[SSRNX_STORAGE_PATH_CAP], tmp_path[SSRNX_STORAGE_PATH_CAP];
    uint64_t bytes = 0;
    const bool tracked = compat_ssrnx_storage_stream_take(f, host_path, sizeof(host_path), &bytes);
    const bool temporary = compat_ssrnx_tmpfile_take(f, tmp_path, sizeof(tmp_path));
    const int rc = fclose(f);
    const int close_errno = rc == 0 ? 0 : errno;
    if (temporary) (void)unlink(tmp_path);
    if (tracked && __atomic_fetch_add(&compat_g_storage_stream_trace_count, 1u, __ATOMIC_RELAXED) < 32u) {
        struct stat st;
        const int src = stat(host_path, &st);
        const int stat_errno = src == 0 ? 0 : errno;
        SSRNX_LOGCI("STORAGE",
                    "fclose-write host=%s bytes=%llu finalSize=%lld result=%d stat=%d errno=%d",
                    host_path, (unsigned long long)bytes,
                    (long long)(src == 0 ? st.st_size : -1), rc, src,
                    rc != 0 ? close_errno : stat_errno);
    }
    if (rc != 0) errno = close_errno;
    return rc;
}
static size_t compat_ssrnx_bionic_fread(void *ptr, size_t size, size_t nmemb, void *stream) {
    SSRNX_IO_TIMING();
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    return fread(ptr, size, nmemb, compat_ssrnx_host_file(stream));
}
static size_t compat_ssrnx_bionic_fwrite(const void *ptr, size_t size, size_t nmemb, void *stream) {
    FILE *f = compat_ssrnx_host_file(stream);
    const size_t rc = fwrite(ptr, size, nmemb, f);
    if (f == stderr && rc && size && rc <= SIZE_MAX / size)
        compat_ssrnx_guest_stderr_capture(ptr, rc * size);
    if (rc && size && rc <= UINT64_MAX / size)
        compat_ssrnx_storage_stream_add(f, (uint64_t)rc * (uint64_t)size);
    return rc;
}
static int compat_ssrnx_bionic_fseek(void *stream, long offset, int whence) {
    return fseek(compat_ssrnx_host_file(stream), offset, whence);
}
static long compat_ssrnx_bionic_ftell(void *stream) { return ftell(compat_ssrnx_host_file(stream)); }
static int compat_ssrnx_bionic_fseeko(void *stream, int64_t offset, int whence) {
    if (sizeof(long) < sizeof(int64_t) && (offset > LONG_MAX || offset < LONG_MIN)) {
        errno = EOVERFLOW; return -1;
    }
    return fseek(compat_ssrnx_host_file(stream), (long)offset, whence);
}
static int64_t compat_ssrnx_bionic_ftello(void *stream) {
    long v = ftell(compat_ssrnx_host_file(stream));
    return v < 0 ? -1 : (int64_t)v;
}
static int compat_ssrnx_bionic_fseeko64(void *stream, int64_t offset, int whence) {
    return compat_ssrnx_bionic_fseeko(stream, offset, whence);
}
static int64_t compat_ssrnx_bionic_ftello64(void *stream) {
    return compat_ssrnx_bionic_ftello(stream);
}
static void compat_ssrnx_bionic_rewind(void *stream) { rewind(compat_ssrnx_host_file(stream)); }
static int compat_ssrnx_bionic_fflush(void *stream) { return fflush(stream ? compat_ssrnx_host_file(stream) : NULL); }
static int compat_ssrnx_bionic_fsync(int fd) {
    SSRNX_IO_TIMING();
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    if (ssrnx_fd_is_virtual(fd)) { errno = EINVAL; return -1; }
    const bool locked = compat_ssrnx_regular_file_io_lock(fd);
    const int rc = fsync(fd);
    const int saved = errno;
    compat_ssrnx_regular_file_io_unlock(locked);
    errno = saved;
    return rc;
}
/* fdatasync may not be exposed by every newlib/libnx combination. fsync is a
 * valid stronger durability primitive for the same guest contract. */
static int compat_ssrnx_bionic_fdatasync(int fd) { return compat_ssrnx_bionic_fsync(fd); }
static int compat_ssrnx_bionic_fgetc(void *stream) { return fgetc(compat_ssrnx_host_file(stream)); }
static int compat_ssrnx_bionic_ungetc(int ch, void *stream) { return ungetc(ch, compat_ssrnx_host_file(stream)); }
static int compat_ssrnx_bionic_feof(void *stream) { return feof(compat_ssrnx_host_file(stream)); }
static int compat_ssrnx_bionic_ferror(void *stream) { return ferror(compat_ssrnx_host_file(stream)); }
static int compat_ssrnx_bionic_fscanf(void *stream, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int rc = vfscanf(compat_ssrnx_host_file(stream), fmt, ap);
    va_end(ap); return rc;
}
static int compat_ssrnx_bionic_fprintf(void *stream, const char *fmt, ...) {
    FILE *f = compat_ssrnx_host_file(stream);
    va_list ap; va_start(ap, fmt);
    if (f == stderr) compat_ssrnx_guest_stderr_capture_vformat(fmt, ap);
    int rc = vfprintf(f, fmt, ap);
    va_end(ap); return rc;
}



static void compat_ssrnx_stack_chk_fail(void) {
    SSRNX_LOGCE("BIONIC", "guest stack protector failure");
    abort();
}

/* Bionic arm64 struct stat is a fixed 128-byte ABI. Translate instead of
 * writing a newlib struct stat directly into guest memory. */
typedef struct compat_SsrnxAndroidStat {
    uint64_t st_dev;
    uint64_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_rdev;
    uint64_t __pad1;
    int64_t st_size;
    int32_t st_blksize;
    int32_t __pad2;
    int64_t st_blocks;
    compat_SsrnxAndroidTimespec st_atim;
    compat_SsrnxAndroidTimespec st_mtim;
    compat_SsrnxAndroidTimespec st_ctim;
    uint32_t __unused4;
    uint32_t __unused5;
} compat_SsrnxAndroidStat;
_Static_assert(sizeof(compat_SsrnxAndroidStat) == 128, "Bionic arm64 stat ABI size");
_Static_assert(offsetof(compat_SsrnxAndroidStat, st_size) == 48, "Bionic arm64 stat.st_size offset");
_Static_assert(offsetof(compat_SsrnxAndroidStat, st_blksize) == 56, "Bionic arm64 stat.st_blksize offset");
_Static_assert(offsetof(compat_SsrnxAndroidStat, st_blocks) == 64, "Bionic arm64 stat.st_blocks offset");
_Static_assert(offsetof(compat_SsrnxAndroidStat, st_atim) == 72, "Bionic arm64 stat.st_atim offset");

typedef struct compat_SsrnxAndroidStatfs {
    uint64_t f_type, f_bsize, f_blocks, f_bfree, f_bavail, f_files, f_ffree;
    int32_t f_fsid[2];
    uint64_t f_namelen, f_frsize, f_flags, f_spare[4];
} compat_SsrnxAndroidStatfs;
_Static_assert(sizeof(compat_SsrnxAndroidStatfs) == 120, "Bionic arm64 statfs ABI size");
_Static_assert(offsetof(compat_SsrnxAndroidStatfs, f_namelen) == 64, "Bionic statfs.f_namelen offset");

/* Linux identifies a file by (st_dev, st_ino); runtimes rely on it, e.g. the
 * Mono/IL2CPP FileShare table keys open files on that pair. libnx fsdev
 * reports st_ino 0 for every file, making all files alias each other. Give
 * such files a stable inode derived from the host path. The SD card's FAT
 * namespace is case-insensitive, so fold ASCII case. */
static uint64_t compat_ssrnx_host_path_inode(const char *host_path) {
    uint64_t h = UINT64_C(14695981039346656037);
    for (const unsigned char *s = (const unsigned char *)host_path; s && *s; ++s) {
        unsigned char c = *s;
        if (c >= 'A' && c <= 'Z') c = (unsigned char)(c - 'A' + 'a');
        if (c == '/' && s[1] == 0 && s != (const unsigned char *)host_path) break; /* trailing slash */
        h = (h ^ c) * UINT64_C(1099511628211);
    }
    return h ? h : 1u;
}

static int compat_ssrnx_bionic_stat(const char *path, compat_SsrnxAndroidStat *out) {
    SSRNX_IO_TIMING_PATH(path);
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    if (!path || !out) { errno = EFAULT; return -1; }
    if (strcmp(path, "/dev/urandom") == 0 || strcmp(path, "/dev/random") == 0) {
        memset(out, 0, sizeof(*out));
        out->st_mode = S_IFCHR | 0444;
        out->st_nlink = 1;
        out->st_blksize = 4096;
        return 0;
    }
    char mapped[SSRNX_STORAGE_PATH_CAP];
    const char *host_path = compat_ssrnx_map_guest_path(path, mapped, sizeof(mapped), NULL);
    struct stat st;
    const int src = compat_ssrnx_storage_stat(host_path, &st);
    if (compat_ssrnx_storage_host_path(host_path) &&
        __atomic_fetch_add(&compat_g_storage_query_trace_count, 1u, __ATOMIC_RELAXED) < 96u) {
        const int saved = errno;
        SSRNX_LOGCI("STORAGE", "stat guest=%s host=%s result=%d size=%lld mode=%#o errno=%d",
                    path, host_path, src, (long long)(src == 0 ? st.st_size : -1),
                    src == 0 ? (unsigned)st.st_mode : 0u, src == 0 ? 0 : saved);
        errno = saved;
    }
    if (src != 0) return -1;
    compat_ssrnx_kernel_view_stat(host_path, &st);
    memset(out, 0, sizeof(*out));
    out->st_dev = (uint64_t)st.st_dev;
    out->st_ino = st.st_ino ? (uint64_t)st.st_ino : compat_ssrnx_host_path_inode(host_path);
    out->st_mode = (uint32_t)st.st_mode;
    out->st_nlink = (uint32_t)st.st_nlink;
    out->st_uid = (uint32_t)st.st_uid;
    out->st_gid = (uint32_t)st.st_gid;
    out->st_rdev = (uint64_t)st.st_rdev;
    out->st_size = (int64_t)st.st_size;
    out->st_blksize = 4096;
    out->st_blocks = (out->st_size + 511) / 512;
    out->st_atim.tv_sec = (int64_t)st.st_atime;
    out->st_mtim.tv_sec = (int64_t)st.st_mtime;
    out->st_ctim.tv_sec = (int64_t)st.st_ctime;
    return 0;
}

typedef struct compat_SsrnxAndroidTm {
    int32_t tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday, tm_yday, tm_isdst;
    int32_t _pad;
    int64_t tm_gmtoff;
    const char *tm_zone;
} compat_SsrnxAndroidTm;
static __thread compat_SsrnxAndroidTm compat_g_android_tm;
static __thread char compat_g_android_local_zone[64];

static int64_t compat_ssrnx_tm_offset_seconds(const struct tm *src) {
    char off[16] = {0};
    if (!src || strftime(off, sizeof(off), "%z", src) == 0) return 0;
    if (off[0] != '+' && off[0] != '-') return 0;
    const int sign = off[0] == '-' ? -1 : 1;
    int hour = 0, minute = 0;
    if (isdigit((unsigned char)off[1]) && isdigit((unsigned char)off[2])) {
        hour = (off[1]-'0')*10 + (off[2]-'0');
        const char *m = off + 3;
        if (*m == ':') ++m;
        if (isdigit((unsigned char)m[0]) && isdigit((unsigned char)m[1]))
            minute = (m[0]-'0')*10 + (m[1]-'0');
    }
    return (int64_t)sign * (int64_t)(hour * 3600 + minute * 60);
}

static void compat_ssrnx_copy_local_tm(compat_SsrnxAndroidTm *out, const struct tm *src) {
    memset(out, 0, sizeof(*out));
    out->tm_sec=src->tm_sec; out->tm_min=src->tm_min; out->tm_hour=src->tm_hour;
    out->tm_mday=src->tm_mday; out->tm_mon=src->tm_mon; out->tm_year=src->tm_year;
    out->tm_wday=src->tm_wday; out->tm_yday=src->tm_yday; out->tm_isdst=src->tm_isdst;
    out->tm_gmtoff=compat_ssrnx_tm_offset_seconds(src);
    if (strftime(compat_g_android_local_zone, sizeof(compat_g_android_local_zone), "%Z", src) == 0)
        compat_g_android_local_zone[0]='\0';
    out->tm_zone=compat_g_android_local_zone;
}

static compat_SsrnxAndroidTm *compat_ssrnx_bionic_localtime(const int64_t *guest_time) {
    if (!guest_time) { errno = EINVAL; return NULL; }
    time_t t = (time_t)*guest_time;
    struct tm *src = localtime(&t);
    if (!src) return NULL;
    compat_ssrnx_copy_local_tm(&compat_g_android_tm, src);
    return &compat_g_android_tm;
}

static compat_SsrnxAndroidTm *compat_ssrnx_bionic_localtime_r(const int64_t *guest_time, compat_SsrnxAndroidTm *out) {
    if (!guest_time || !out) { errno = EINVAL; return NULL; }
    time_t t = (time_t)*guest_time;
    struct tm host_tm;
    if (!localtime_r(&t, &host_tm)) return NULL;
    compat_ssrnx_copy_local_tm(out, &host_tm);
    return out;
}

static int64_t compat_ssrnx_bionic_mktime(compat_SsrnxAndroidTm *guest_tm) {
    if (!guest_tm) { errno = EINVAL; return -1; }
    struct tm host_tm;
    memset(&host_tm, 0, sizeof(host_tm));
    host_tm.tm_sec=guest_tm->tm_sec; host_tm.tm_min=guest_tm->tm_min; host_tm.tm_hour=guest_tm->tm_hour;
    host_tm.tm_mday=guest_tm->tm_mday; host_tm.tm_mon=guest_tm->tm_mon; host_tm.tm_year=guest_tm->tm_year;
    host_tm.tm_wday=guest_tm->tm_wday; host_tm.tm_yday=guest_tm->tm_yday; host_tm.tm_isdst=guest_tm->tm_isdst;
    const time_t result = mktime(&host_tm);
    compat_ssrnx_copy_local_tm(guest_tm, &host_tm);
    return (int64_t)result;
}

static void compat_ssrnx_bionic__exit(int status) {
    SSRNX_LOGCI("BIONIC", "guest _exit(%d) requested", status);
    SsrnxANativeActivity *activity = ssrnx_native_activity();
    if (activity) ANativeActivity_finish(activity);
    pthread_exit((void *)(intptr_t)status);
    __builtin_unreachable();
}

static void compat_ssrnx_bionic_exit(int status) {
    /* ISO C exit flushes stdio and runs registered termination handlers;
     * _exit deliberately skips both. SSRNX's cxa registry is the guest-side
     * atexit/destructor registry. */
    (void)fflush(NULL);
    ssrnx_bionic_cxa_finalize(NULL);
    compat_ssrnx_bionic__exit(status);
}

/* Linux AArch64 syscall numbers from asm-generic. Keep the compatibility
 * surface intentionally narrow and translate the Android ABI explicitly;
 * Android syscall numbers must never be forwarded to Horizon/newlib. */
static ssize_t compat_ssrnx_bionic_process_vm_readv(int pid,
    const compat_SsrnxAndroidIovec *local_iov, unsigned long liovcnt,
    const compat_SsrnxAndroidIovec *remote_iov, unsigned long riovcnt, unsigned long flags);

static int compat_ssrnx_bionic_clock_gettime(int, compat_SsrnxAndroidTimespec *);
static int compat_ssrnx_bionic_clock_getres(int, compat_SsrnxAndroidTimespec *);
static bool compat_ssrnx_process_vm_range_valid(const void *, size_t, bool);
static ssize_t compat_ssrnx_bionic_getrandom(void *, size_t, unsigned);
static int compat_ssrnx_bionic_openat(int, const char *, int, ...);
static int compat_ssrnx_bionic_close(int);
static int64_t compat_ssrnx_bionic_lseek(int, int64_t, int);
static ssize_t compat_ssrnx_bionic_read(int, void *, size_t);
static ssize_t compat_ssrnx_bionic_write(int, const void *, size_t);
static ssize_t compat_ssrnx_bionic_pread(int, void *, size_t, int64_t);
static ssize_t compat_ssrnx_bionic_pwrite(int, const void *, size_t, int64_t);
static int compat_ssrnx_bionic_fstat(int, compat_SsrnxAndroidStat *);
static int compat_ssrnx_bionic_fstatfs(int, compat_SsrnxAndroidStatfs *);
static int compat_ssrnx_bionic_prctl(int, unsigned long, unsigned long, unsigned long, unsigned long);
static long compat_ssrnx_bionic_ptrace(int, int, void *, void *);
static int compat_ssrnx_bionic_getppid(void);

/* A raw syscall must reject bad guest pointers before a host/VFS wrapper
 * dereferences them. Probe page-sized chunks, stopping at the first NUL. */
static bool compat_ssrnx_syscall_path_valid(const char *path) {
    if (!path) { errno = EFAULT; return false; }
    uintptr_t address = (uintptr_t)path;
    for (size_t remaining = 4096u; remaining;) {
        size_t n = 4096u - (address & 4095u);
        if (n > remaining) n = remaining;
        if (!compat_ssrnx_process_vm_range_valid((const void *)address, n, false)) {
            errno = EFAULT; return false;
        }
        if (memchr((const void *)address, 0, n)) return true;
        address += n;
        remaining -= n;
    }
    errno = ENAMETOOLONG;
    return false;
}

static long compat_ssrnx_bionic_syscall_dispatch(long number, const uintptr_t args[6],
                                         uintptr_t caller, const void *frame) {
    long result = -1;

    switch (number) {
        case 20: case 21: case 22: {                /* epoll_create1/ctl/pwait */
            extern long ssrnx_epoll_syscall(long,const uintptr_t *) __attribute__((weak));
            if (ssrnx_epoll_syscall) result=ssrnx_epoll_syscall(number,args);
            else errno=ENOSYS;
            break;
        }
        case 85: case 86: case 87: {                /* timerfd_create/settime/gettime */
            extern long ssrnx_timerfd_syscall(long,const uintptr_t *) __attribute__((weak));
            if (ssrnx_timerfd_syscall) result=ssrnx_timerfd_syscall(number,args);
            else errno=ENOSYS;
            break;
        }
        case 278:                                  /* getrandom */
            result = compat_ssrnx_bionic_getrandom((void *)args[0], (size_t)args[1], (unsigned)args[2]);
            break;
        case 44: {                                  /* fstatfs: arm64 120-byte layout */
            if (!compat_ssrnx_process_vm_range_valid((void *)args[1], sizeof(compat_SsrnxAndroidStatfs), true)) {
                errno = EFAULT; break;
            }
            result = compat_ssrnx_bionic_fstatfs((int)args[0], (compat_SsrnxAndroidStatfs *)args[1]);
            const int saved_errno = errno;
            static unsigned fs_traces;
            if (result < 0 || __atomic_fetch_add(&fs_traces, 1u, __ATOMIC_RELAXED) < 12u)
                SSRNX_LOGCI("SYSCALL", "fstatfs fd=%d result=%ld type=%#llx errno=%d caller=%p",
                            (int)args[0], result, result == 0 ? (unsigned long long)((compat_SsrnxAndroidStatfs *)args[1])->f_type : 0,
                            result < 0 ? saved_errno : 0, (void *)caller);
            errno = saved_errno;
            break;
        }
        case 56: {                                  /* openat */
            if (!compat_ssrnx_syscall_path_valid((const char *)args[1])) break;
            result = compat_ssrnx_bionic_openat((int)args[0], (const char *)args[1],
                                         (int)args[2], (int)args[3]);
            const int saved_errno = errno;
            static unsigned openat_traces;
            if (result < 0 || __atomic_fetch_add(&openat_traces, 1u, __ATOMIC_RELAXED) < 24u) {
                char site[192];
                (void)ssrnx_compat_describe_guest_address(caller, site, sizeof(site));
                SSRNX_LOGCI("SYSCALL", "openat dirfd=%d path=%s flags=%#x result=%ld errno=%d caller=%s",
                            (int)args[0], (const char *)args[1], (unsigned)args[2],
                            result, result < 0 ? saved_errno : 0, site);
            }
            errno = saved_errno;
            break;
        }
        case 57: result = compat_ssrnx_bionic_close((int)args[0]); break;
        case 62:                                    /* lseek (64-bit off_t) */
            result = compat_ssrnx_bionic_lseek((int)args[0], (int64_t)args[1], (int)args[2]);
            break;
        case 63: case 64: case 67: case 68: {        /* read/write/pread64/pwrite64 */
            const bool reading = number == 63 || number == 67;
            if (args[2] > (SIZE_MAX >> 1)) { errno = EINVAL; break; }
            if (!compat_ssrnx_process_vm_range_valid((const void *)args[1], (size_t)args[2], reading)) {
                errno = EFAULT; break;
            }
            if (number == 63) result = compat_ssrnx_bionic_read((int)args[0], (void *)args[1], (size_t)args[2]);
            else if (number == 64) result = compat_ssrnx_bionic_write((int)args[0], (const void *)args[1], (size_t)args[2]);
            else if (number == 67) result = compat_ssrnx_bionic_pread((int)args[0], (void *)args[1], (size_t)args[2], (int64_t)args[3]);
            else result = compat_ssrnx_bionic_pwrite((int)args[0], (const void *)args[1], (size_t)args[2], (int64_t)args[3]);
            break;
        }
        case 80:                                    /* fstat: Bionic arm64 layout */
            if (!compat_ssrnx_process_vm_range_valid((void *)args[1], sizeof(compat_SsrnxAndroidStat), true)) {
                errno = EFAULT; break;
            }
            result = compat_ssrnx_bionic_fstat((int)args[0], (compat_SsrnxAndroidStat *)args[1]);
            break;
        case 98: {                                   /* futex */
            uint32_t *uaddr = (uint32_t *)args[0];
            const int op = (int)args[1];
            const uint32_t val = (uint32_t)(int)args[2];
            const void *timeout = (const void *)args[3];
            uint32_t *uaddr2 = (uint32_t *)args[4];
            const uint32_t val3 = (uint32_t)(int)args[5];
            if (compat_ssrnx_memtrace_enabled()) {
                SSRNX_LOGCI("SYSCALL", "futex uaddr=%p op=0x%x val=%u timeout=%p uaddr2=%p val3=0x%x tid=%d",
                            (void *)uaddr, op, val, timeout, (void *)uaddr2, val3,
                            ssrnx_bionic_gettid());
                compat_ssrnx_memtrace_flush();
            }
            result = ssrnx_bionic_futex_at(uaddr, op, val, timeout, uaddr2, val3,
                         caller, frame);
            if (result < 0 && errno == ENOSYS)
                SSRNX_LOGCW("SYSCALL", "unsupported futex operation op=0x%x", op);
            break;
        }
        case 113:                                   /* clock_gettime */
            result = compat_ssrnx_bionic_clock_gettime((int)args[0], (compat_SsrnxAndroidTimespec *)args[1]);
            break;
        case 114:                                   /* clock_getres */
            result = compat_ssrnx_bionic_clock_getres((int)args[0], (compat_SsrnxAndroidTimespec *)args[1]);
            break;
        case 117:                                   /* ptrace: no tracer backend */
            result = compat_ssrnx_bionic_ptrace((int)args[0], (int)args[1], (void *)args[2], (void *)args[3]);
            break;
        case 122: {                                  /* sched_setaffinity */
            const int pid = (int)args[0];
            const size_t cpusetsize = (size_t)args[1];
            const void *mask = (const void *)args[2];
            result = ssrnx_bionic_sched_setaffinity(pid, cpusetsize, mask);
            if (compat_ssrnx_memtrace_enabled()) {
                SSRNX_LOGCI("SYSCALL", "sched_setaffinity pid=%d cpusetsize=%zu mask=%p result=%ld errno=%d",
                            pid, cpusetsize, mask, result, result < 0 ? errno : 0);
                compat_ssrnx_memtrace_flush();
            }
            break;
        }
        case 124: {                                  /* sched_yield */
            compat_ssrnx_host_yield();
            result = 0;
            break;
        }
        case 135: {                                  /* rt_sigprocmask */
            const int how = (int)args[0];
            const uint64_t *set = (const uint64_t *)args[1];
            uint64_t *old_set = (uint64_t *)args[2];
            const size_t sigset_size = (size_t)args[3];
            if (sigset_size != sizeof(uint64_t)) { errno = EINVAL; result = -1; break; }
            result = ssrnx_bionic_sigprocmask(how, set, old_set);
            break;
        }
        case 167: {                                  /* prctl: shared process/thread model */
            if ((int)args[0] == 15) {                 /* PR_SET_NAME: at most 15 bytes */
                const char *name = (const char *)args[1];
                bool valid = name != NULL;
                for (size_t i = 0; valid && i < 15u; ++i) {
                    valid = compat_ssrnx_process_vm_range_valid(name + i, 1u, false);
                    if (valid && !name[i]) break;
                }
                if (!valid) { errno = EFAULT; break; }
            } else if ((int)args[0] == 16 &&
                       !compat_ssrnx_process_vm_range_valid((void *)args[1], 16u, true)) {
                errno = EFAULT; break;
            }
            result = compat_ssrnx_bionic_prctl((int)args[0], args[1], args[2], args[3], args[4]);
            const int saved_errno = errno;
            static unsigned prctl_traces;
            if (result < 0 || __atomic_fetch_add(&prctl_traces, 1u, __ATOMIC_RELAXED) < 16u)
                SSRNX_LOGCI("SYSCALL", "prctl option=%#lx result=%ld errno=%d caller=%p",
                            (unsigned long)args[0], result, result < 0 ? saved_errno : 0, (void *)caller);
            errno = saved_errno;
            break;
        }
        case 172: result = (long)getpid(); break;      /* getpid */
        case 173: result = (long)compat_ssrnx_bionic_getppid(); break;
        case 178: result = (long)ssrnx_bionic_gettid(); break; /* gettid */
        case 270: {                                  /* process_vm_readv */
            const int pid = (int)args[0];
            const compat_SsrnxAndroidIovec *local = (const compat_SsrnxAndroidIovec *)args[1];
            const unsigned long local_count = (unsigned long)args[2];
            const compat_SsrnxAndroidIovec *remote = (const compat_SsrnxAndroidIovec *)args[3];
            const unsigned long remote_count = (unsigned long)args[4];
            const unsigned long flags = (unsigned long)args[5];
            result = compat_ssrnx_bionic_process_vm_readv(pid, local, local_count, remote, remote_count, flags);
            const int saved_errno = errno;
            static unsigned trace_count;
            if (__atomic_fetch_add(&trace_count, 1u, __ATOMIC_RELAXED) < 8u || compat_ssrnx_memtrace_enabled()) {
                char site[192];
                (void)ssrnx_compat_describe_guest_address(
                    caller, site, sizeof(site));
                SSRNX_LOGCI("SYSCALL", "process_vm_readv pid=%d localCount=%lu remoteCount=%lu flags=%lu result=%ld errno=%d caller=%s",
                            pid, local_count, remote_count, flags, result, result < 0 ? saved_errno : 0, site);
            }
            errno = saved_errno;
            break;
        }
        default:
            SSRNX_LOGCW("SYSCALL", "unsupported Android arm64 syscall=%ld tid=%d pc=%p",
                        number, ssrnx_bionic_gettid(),
                        (void *)caller);
            errno = ENOSYS;
            result = -1;
            break;
    }

    return result;
}

static long compat_ssrnx_bionic_syscall(long number, ...) {
    uintptr_t args[6] = {0};
    va_list ap;
    va_start(ap, number);
    /* Read only supplied arguments, with the same ABI types as before. */
    switch (number) {
        case 20: args[0]=(uintptr_t)va_arg(ap,int);break;
        case 21:
            args[0]=(uintptr_t)va_arg(ap,int);args[1]=(uintptr_t)va_arg(ap,int);
            args[2]=(uintptr_t)va_arg(ap,int);args[3]=(uintptr_t)va_arg(ap,void *);break;
        case 22:
            args[0]=(uintptr_t)va_arg(ap,int);args[1]=(uintptr_t)va_arg(ap,void *);
            args[2]=(uintptr_t)va_arg(ap,int);args[3]=(uintptr_t)va_arg(ap,int);
            args[4]=(uintptr_t)va_arg(ap,const void *);args[5]=va_arg(ap,size_t);break;
        case 85: args[0]=(uintptr_t)va_arg(ap,int);args[1]=(uintptr_t)va_arg(ap,int);break;
        case 86:
            args[0]=(uintptr_t)va_arg(ap,int);args[1]=(uintptr_t)va_arg(ap,int);
            args[2]=(uintptr_t)va_arg(ap,const void *);args[3]=(uintptr_t)va_arg(ap,void *);break;
        case 87: args[0]=(uintptr_t)va_arg(ap,int);args[1]=(uintptr_t)va_arg(ap,void *);break;
        case 44:
            args[0] = (uintptr_t)va_arg(ap, int);
            args[1] = (uintptr_t)va_arg(ap, compat_SsrnxAndroidStatfs *);
            break;
        case 56:
            args[0] = (uintptr_t)va_arg(ap, int);
            args[1] = (uintptr_t)va_arg(ap, const char *);
            args[2] = (uintptr_t)va_arg(ap, int);
            if (args[2] & SSRNX_ANDROID_O_CREAT) args[3] = (uintptr_t)va_arg(ap, int);
            break;
        case 57:
            args[0] = (uintptr_t)va_arg(ap, int);
            break;
        case 62:
            args[0] = (uintptr_t)va_arg(ap, int);
            args[1] = (uintptr_t)va_arg(ap, int64_t);
            args[2] = (uintptr_t)va_arg(ap, int);
            break;
        case 63: case 64: case 67: case 68:
            args[0] = (uintptr_t)va_arg(ap, int);
            args[1] = (uintptr_t)va_arg(ap, void *);
            args[2] = va_arg(ap, size_t);
            if (number == 67 || number == 68) args[3] = (uintptr_t)va_arg(ap, int64_t);
            break;
        case 80:
            args[0] = (uintptr_t)va_arg(ap, int);
            args[1] = (uintptr_t)va_arg(ap, compat_SsrnxAndroidStat *);
            break;
        case 98:
            args[0] = (uintptr_t)va_arg(ap, uint32_t *);
            args[1] = (uintptr_t)va_arg(ap, int);
            args[2] = (uintptr_t)va_arg(ap, int);
            args[3] = (uintptr_t)va_arg(ap, const void *);
            args[4] = (uintptr_t)va_arg(ap, uint32_t *);
            args[5] = (uintptr_t)va_arg(ap, int);
            break;
        case 113: case 114:
            args[0] = (uintptr_t)va_arg(ap, int);
            args[1] = (uintptr_t)va_arg(ap, compat_SsrnxAndroidTimespec *);
            break;
        case 117:
            args[0] = (uintptr_t)va_arg(ap, int);
            args[1] = (uintptr_t)va_arg(ap, int);
            args[2] = (uintptr_t)va_arg(ap, void *);
            args[3] = (uintptr_t)va_arg(ap, void *);
            break;
        case 167: {
            const int option = va_arg(ap, int);
            args[0] = (uintptr_t)option;
            /* Unused prctl arguments are ignored by Linux. GET_DUMPABLE can
             * be called with the option alone; do not read absent varargs. */
            if (option == 4 || option == 15 || option == 16)
                args[1] = va_arg(ap, unsigned long);
            else if (option == 38 || option == 39 || option == 0x53564d41)
                for (unsigned i = 1; i < 5; ++i) args[i] = va_arg(ap, unsigned long);
            break;
        }
        case 122:
            args[0] = (uintptr_t)va_arg(ap, int);
            args[1] = va_arg(ap, size_t);
            args[2] = (uintptr_t)va_arg(ap, const void *);
            break;
        case 135:
            args[0] = (uintptr_t)va_arg(ap, int);
            args[1] = (uintptr_t)va_arg(ap, const uint64_t *);
            args[2] = (uintptr_t)va_arg(ap, uint64_t *);
            args[3] = va_arg(ap, size_t);
            break;
        case 270:
            args[0] = (uintptr_t)va_arg(ap, int);
            args[1] = (uintptr_t)va_arg(ap, const compat_SsrnxAndroidIovec *);
            args[2] = va_arg(ap, unsigned long);
            args[3] = (uintptr_t)va_arg(ap, const compat_SsrnxAndroidIovec *);
            args[4] = va_arg(ap, unsigned long);
            args[5] = va_arg(ap, unsigned long);
            break;
        default: break;
    }
    va_end(ap);
    return compat_ssrnx_bionic_syscall_dispatch(number, args,
        (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)),
        __builtin_frame_address(0));
}

long ssrnx_bionic_raw_syscall(long number, uintptr_t a0, uintptr_t a1,
                             uintptr_t a2, uintptr_t a3, uintptr_t a4,
                             uintptr_t a5, uintptr_t caller) {
    const uintptr_t args[6] = {a0, a1, a2, a3, a4, a5};
    const int saved_host = errno;
    int *guest_errno = ssrnx_guest_errno_location();
    const int saved_guest = *guest_errno;
    long result = compat_ssrnx_bionic_syscall_dispatch(number, args, caller, NULL);
    const int error = result == -1 ? ssrnx_errno_host_to_android(errno) : 0;
    static unsigned traces;
    if (error || __atomic_fetch_add(&traces, 1u, __ATOMIC_RELAXED) < 16u || compat_ssrnx_memtrace_enabled())
        SSRNX_LOGCI("SYSCALL", "inline number=%ld caller=%p result=%ld linuxErrno=%d",
                    number, (void *)caller, result, error);
    errno = saved_host;
    *guest_errno = saved_guest;
    return error ? -(long)error : result;
}

typedef struct compat_SsrnxDlPhdrInfo {
    uintptr_t dlpi_addr;
    const char *dlpi_name;
    const Elf64_Phdr *dlpi_phdr;
    Elf64_Half dlpi_phnum;
    Elf64_Half _pad16;
    uint32_t _pad32;
    uint64_t dlpi_adds;
    uint64_t dlpi_subs;
    size_t dlpi_tls_modid;
    void *dlpi_tls_data;
} compat_SsrnxDlPhdrInfo;

/* Return the program-header table as it exists in the loaded image when the
 * ELF maps its headers through a PT_LOAD segment.  Bionic's dl_iterate_phdr()
 * exposes loaded-image metadata, not the linker's private file buffer.  Static
 * libunwind/libc++abi implementations use this view to discover
 * PT_GNU_EH_FRAME across DSO boundaries. */
static const Elf64_Phdr *compat_ssrnx_loaded_phdr_table(const SsrnxModule *m) {
    if (!m || !m->mapped || !m->ehdr || !m->phdrs || !m->ehdr->e_phnum) return NULL;

    const uint64_t phoff = m->ehdr->e_phoff;
    const uint64_t phbytes = (uint64_t)m->ehdr->e_phnum * sizeof(Elf64_Phdr);
    if (phbytes / sizeof(Elf64_Phdr) != m->ehdr->e_phnum || phoff > UINT64_MAX - phbytes) {
        return NULL;
    }
    const uint64_t phend = phoff + phbytes;

    /* PT_PHDR is the authoritative in-memory address when present. */
    for (uint16_t i = 0; i < m->ehdr->e_phnum; ++i) {
        const Elf64_Phdr *ph = &m->phdrs[i];
        if (ph->p_type != PT_PHDR || ph->p_memsz < phbytes) continue;
        const Elf64_Phdr *runtime = (const Elf64_Phdr *)ssrnx_module_runtime_address(m, ph->p_vaddr);
        if (runtime && ssrnx_module_runtime_contains(m, (uintptr_t)runtime, (size_t)phbytes)) return runtime;
    }

    /* Some Android DSOs omit PT_PHDR but include the ELF/PHDR headers in the
     * first PT_LOAD. Translate the file offset into that segment's vaddr. */
    for (uint16_t i = 0; i < m->ehdr->e_phnum; ++i) {
        const Elf64_Phdr *ph = &m->phdrs[i];
        if (ph->p_type != PT_LOAD || phoff < ph->p_offset) continue;
        if (ph->p_offset > UINT64_MAX - ph->p_filesz) continue;
        if (phend > ph->p_offset + ph->p_filesz) continue;
        const uint64_t delta = phoff - ph->p_offset;
        if (ph->p_vaddr > UINT64_MAX - delta) continue;
        const uint64_t vaddr = ph->p_vaddr + delta;
        const Elf64_Phdr *runtime = (const Elf64_Phdr *)ssrnx_module_runtime_address(m, vaddr);
        if (runtime && ssrnx_module_runtime_contains(m, (uintptr_t)runtime, (size_t)phbytes)) return runtime;
    }

    return NULL;
}

static int compat_ssrnx_dl_iterate_phdr(int (*callback)(compat_SsrnxDlPhdrInfo *, size_t, void *), void *data) {
    if (!callback || !compat_g_active_linker) return 0;

    const size_t node_count = ssrnx_linker_module_count(compat_g_active_linker);
    size_t loaded_count = 0;
    for (size_t i = 0; i < node_count; ++i) {
        const SsrnxModule *m = ssrnx_linker_module_at(compat_g_active_linker, i);
        if (m && m->mapped && m->ehdr && m->phdrs) ++loaded_count;
    }

    static uint32_t trace_count;
    for (size_t i = 0; i < node_count; ++i) {
        const SsrnxModule *m = ssrnx_linker_module_at(compat_g_active_linker, i);
        if (!m || !m->mapped || !m->ehdr || !m->phdrs) continue;

        const Elf64_Phdr *runtime_phdrs = compat_ssrnx_loaded_phdr_table(m);
        const Elf64_Phdr *reported_phdrs = runtime_phdrs ? runtime_phdrs : m->phdrs;

        uintptr_t eh_frame_hdr = 0;
        size_t eh_frame_hdr_size = 0;
        for (uint16_t pi = 0; pi < m->ehdr->e_phnum; ++pi) {
            const Elf64_Phdr *ph = &m->phdrs[pi];
            if (ph->p_type != PT_GNU_EH_FRAME) continue;
            void *runtime = ssrnx_module_runtime_address(m, ph->p_vaddr);
            if (runtime && ssrnx_module_runtime_contains(m, (uintptr_t)runtime, (size_t)ph->p_memsz)) {
                eh_frame_hdr = (uintptr_t)runtime;
                eh_frame_hdr_size = (size_t)ph->p_memsz;
            }
            break;
        }

        if (trace_count < 48u) {
            SSRNX_LOGCI("LIBDL", "dl_iterate_phdr module=%s addr=%p phdr=%p phnum=%u runtimePhdr=%u ehFrameHdr=%p ehFrameBytes=%zu loaded=%zu",
                        m->path ? m->path : "", (void *)m->load_bias, (const void *)reported_phdrs,
                        (unsigned)m->ehdr->e_phnum, runtime_phdrs ? 1u : 0u,
                        (void *)eh_frame_hdr, eh_frame_hdr_size, loaded_count);
            ++trace_count;
        }

        compat_SsrnxDlPhdrInfo info;
        memset(&info, 0, sizeof(info));
        info.dlpi_addr = m->load_bias;
        info.dlpi_name = m->path;
        info.dlpi_phdr = reported_phdrs;
        info.dlpi_phnum = m->ehdr->e_phnum;
        info.dlpi_adds = loaded_count;
        if (m->tls.present && m->tls.registered) {
            info.dlpi_tls_modid = m->tls.module_id;
            info.dlpi_tls_data = ssrnx_bionic_tls_get_addr(m->tls.module_id, 0);
        }
        int rc = callback(&info, sizeof(info), data);
        if (rc != 0) return rc;
    }
    return 0;
}

static int compat_translate_open_flags(int android_flags, int *out_host_flags) {
    if (!out_host_flags) {
        errno = EINVAL;
        return -1;
    }

    int host_flags = 0;
    switch (android_flags & SSRNX_ANDROID_O_ACCMODE) {
        case SSRNX_ANDROID_O_RDONLY: host_flags |= O_RDONLY; break;
        case SSRNX_ANDROID_O_WRONLY: host_flags |= O_WRONLY; break;
        case SSRNX_ANDROID_O_RDWR:   host_flags |= O_RDWR; break;
        default:
            errno = EINVAL;
            return -1;
    }

    int known = SSRNX_ANDROID_O_ACCMODE | SSRNX_ANDROID_O_CREAT |
                SSRNX_ANDROID_O_EXCL | SSRNX_ANDROID_O_TRUNC |
                SSRNX_ANDROID_O_APPEND | SSRNX_ANDROID_O_NONBLOCK |
                SSRNX_ANDROID_O_LARGEFILE | SSRNX_ANDROID_O_CLOEXEC |
                SSRNX_ANDROID_O_DIRECTORY | SSRNX_ANDROID_O_NOFOLLOW |
                SSRNX_ANDROID_O_NOCTTY;
    if (android_flags & ~known) {
        errno = EINVAL;
        return -1;
    }

    if (android_flags & SSRNX_ANDROID_O_CREAT) host_flags |= O_CREAT;
    if (android_flags & SSRNX_ANDROID_O_EXCL) host_flags |= O_EXCL;
    if (android_flags & SSRNX_ANDROID_O_TRUNC) host_flags |= O_TRUNC;
    if (android_flags & SSRNX_ANDROID_O_APPEND) host_flags |= O_APPEND;
#ifdef O_NOCTTY
    if (android_flags & SSRNX_ANDROID_O_NOCTTY) host_flags |= O_NOCTTY;
#else
    /* Horizon has no controlling-terminal acquisition to suppress. */
#endif
#ifdef O_DIRECTORY
    if (android_flags & SSRNX_ANDROID_O_DIRECTORY) host_flags |= O_DIRECTORY;
#endif
#ifdef O_NOFOLLOW
    if (android_flags & SSRNX_ANDROID_O_NOFOLLOW) host_flags |= O_NOFOLLOW;
#else
    if (android_flags & SSRNX_ANDROID_O_NOFOLLOW) { errno = ENOTSUP; return -1; }
#endif
#ifdef O_NONBLOCK
    if (android_flags & SSRNX_ANDROID_O_NONBLOCK) host_flags |= O_NONBLOCK;
#else
    if (android_flags & SSRNX_ANDROID_O_NONBLOCK) {
        errno = EINVAL;
        return -1;
    }
#endif
#ifdef O_CLOEXEC
    if (android_flags & SSRNX_ANDROID_O_CLOEXEC) host_flags |= O_CLOEXEC;
#else
    /* There is no exec() boundary in the current Horizon runtime, so the
       Android close-on-exec intent has no observable effect here. */
    (void)SSRNX_ANDROID_O_CLOEXEC;
#endif
    /* O_LARGEFILE is a no-op for the 64-bit guest ABI. */

    *out_host_flags = host_flags;
    return 0;
}

static int compat_ssrnx_bionic_open(const char *path, int android_flags, ...);

static int compat_ssrnx_bionic_open_2(const char *path, int android_flags) {
    /* Bionic FORTIFY __open_2 is only valid when open() does not require a
     * third mode argument. O_TMPFILE, like O_CREAT, requires mode_t. */
    if ((android_flags & SSRNX_ANDROID_O_CREAT) ||
        ((android_flags & SSRNX_ANDROID_O_TMPFILE) == SSRNX_ANDROID_O_TMPFILE)) {
        compat_ssrnx_fortify_fail("__open_2");
    }
    return compat_ssrnx_bionic_open(path, android_flags);
}

static int compat_ssrnx_bionic_open(const char *path, int android_flags, ...) {
    SSRNX_IO_TIMING_PATH(path);
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    int host_flags = 0;
    if (!path) { errno = EFAULT; return -1; }
    if ((strcmp(path, "/dev/urandom") == 0 || strcmp(path, "/dev/random") == 0) &&
        (android_flags & SSRNX_ANDROID_O_ACCMODE) == SSRNX_ANDROID_O_RDONLY) {
        if (android_flags & SSRNX_ANDROID_O_DIRECTORY) { errno = ENOTDIR; return -1; }
        return ssrnx_fd_open_urandom();
    }
    if (compat_translate_open_flags(android_flags, &host_flags) != 0) return -1;
    const bool writing = (android_flags & SSRNX_ANDROID_O_ACCMODE) != SSRNX_ANDROID_O_RDONLY ||
                         (android_flags & (SSRNX_ANDROID_O_CREAT | SSRNX_ANDROID_O_TRUNC | SSRNX_ANDROID_O_APPEND)) != 0;
    if (writing && compat_ssrnx_guest_path_is_readonly_system(path)) { errno = EROFS; return -1; }
    char mapped[SSRNX_STORAGE_PATH_CAP];
    bool path_mapped = false;
    const char *host_path = compat_ssrnx_map_guest_path(path, mapped, sizeof(mapped), &path_mapped);

    if (writing && compat_ssrnx_kernel_view_host_path(host_path)) { errno = EACCES; return -1; }
    if (!writing && ssrnx_path_lookup_missing_parent && ssrnx_path_lookup_missing_parent(host_path)) return -1;

    if (android_flags & SSRNX_ANDROID_O_DIRECTORY) {
        struct stat directory;
        if (compat_ssrnx_storage_stat(host_path, &directory) != 0) return -1;
        if (!S_ISDIR(directory.st_mode)) { errno = ENOTDIR; return -1; }
    }

    int rc;
    if (android_flags & SSRNX_ANDROID_O_CREAT) {
        va_list ap;
        va_start(ap, android_flags);
        const int mode = va_arg(ap, int);
        va_end(ap);
        rc = open(host_path, host_flags, mode);
    } else {
        rc = open(host_path, host_flags);
    }
    /* Only probe directories after a native failure; regular-file opens must
     * not gain an extra SD metadata query. */
    if (rc < 0 && ssrnx_directory_open) {
        const int directory = ssrnx_directory_open(host_path, host_flags);
        if (directory != -2) rc = directory;
    }
    if (rc < 0 && !writing && errno == ENOENT && ssrnx_path_lookup_record_missing_parent)
        ssrnx_path_lookup_record_missing_parent(host_path);
    if (strncmp(path, "/proc/", 6) == 0) {
        const int saved = errno;
        const uintptr_t caller = (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0));
        char site[192];
        (void)ssrnx_compat_describe_guest_address(caller, site, sizeof(site));
        SSRNX_LOGCI("PROC", "open path=%s mapped=%d host=%s flags=0x%x result=%d errno=%d caller=%s",
                    path, path_mapped ? 1 : 0, host_path, android_flags, rc,
                    rc >= 0 ? 0 : saved, site);
        errno = saved;
    }
    if (writing && compat_ssrnx_storage_host_path(host_path)) {
        __atomic_fetch_add(&compat_g_storage_write_opens, 1u, __ATOMIC_RELAXED);
        const int saved = errno;
        compat_ssrnx_storage_log("open-write", path, host_path, rc);
        errno = saved;
    } else if (!writing && compat_ssrnx_storage_host_path(host_path) &&
               __atomic_fetch_add(&compat_g_storage_query_trace_count, 1u, __ATOMIC_RELAXED) < 96u) {
        const int saved = errno;
        SSRNX_LOGCI("STORAGE", "open-read guest=%s host=%s result=%d errno=%d",
                    path, host_path, rc, rc >= 0 ? 0 : saved);
        errno = saved;
    }
    if (rc >= 0 && ssrnx_file_contract_open &&
        (compat_ssrnx_storage_host_path(host_path) || compat_ssrnx_kernel_view_host_path(host_path) ||
         (compat_g_package_apk_host[0] && strcmp(host_path, compat_g_package_apk_host) == 0))) {
        const bool package_apk = compat_g_package_apk_host[0] && strcmp(host_path, compat_g_package_apk_host) == 0;
        ssrnx_file_contract_open(rc, path, host_path, android_flags, host_flags, package_apk);
    }
    (void)path_mapped;
    return rc;
}

int ssrnx_compat_open_guest_readonly(const char *path) {
    return compat_ssrnx_bionic_open(path, SSRNX_ANDROID_O_RDONLY);
}

/* Host path of an Android guest path (app-private dirs, external storage,
 * package APK, kernel views); unmapped paths are copied unchanged. */
bool ssrnx_compat_map_guest_path(const char *guest_path, char *out, size_t cap) {
    if (!guest_path || !out || !cap) return false;
    char mapped[SSRNX_STORAGE_PATH_CAP];
    bool path_mapped = false;
    const char *host = compat_ssrnx_map_guest_path(guest_path, mapped, sizeof(mapped), &path_mapped);
    return snprintf(out, cap, "%s", host) < (int)cap;
}

static int compat_ssrnx_bionic_dup(int oldfd) {
    if (ssrnx_epoll_duplicate) { const int rc=ssrnx_epoll_duplicate(oldfd,0,false);if (rc!=-2) return rc; }
    if (ssrnx_eventfd_duplicate) {
        const int result = ssrnx_eventfd_duplicate(oldfd, 0, false);
        if (result != -2) { if (result>=0 && ssrnx_epoll_duplicated) ssrnx_epoll_duplicated(oldfd,result);return result; }
    }
    if (ssrnx_fd_is_virtual(oldfd)) { errno = ENOTSUP; return -1; }

    /* Prefer the path-backed regular-file duplicate layer on Switch/newlib.
     * This avoids accepting a nominal F_DUPFD result that cannot subsequently
     * be used with fstat/read/mmap on fsdev descriptors. */
    if (ssrnx_file_contract_reopen_duplicate) {
        const int reopened = ssrnx_file_contract_reopen_duplicate(oldfd, 0, false);
        if (reopened >= 0) return reopened;
        if (reopened == -1) return -1;
    }
#ifdef F_DUPFD
    int out = fcntl(oldfd, F_DUPFD, 0);
    if (out < 0) compat_ssrnx_network_fix_errno();
    else if (ssrnx_file_contract_dup) ssrnx_file_contract_dup(oldfd, out);
    if (out>=0 && ssrnx_epoll_duplicated) ssrnx_epoll_duplicated(oldfd,out);
    return out;
#else
    (void)oldfd;
    errno = ENOTSUP;
    return -1;
#endif
}

static int compat_ssrnx_bionic_dup2(int oldfd, int newfd) {
    if (oldfd < 0 || newfd < 0) { errno = EBADF; return -1; }
    if (ssrnx_epoll_control && (ssrnx_epoll_control(oldfd,1,0)!=-2 || ssrnx_epoll_control(newfd,1,0)!=-2)) {
        if (oldfd==newfd) return oldfd;
        errno=EOPNOTSUPP;return -1;
    }
    if (ssrnx_eventfd_poll && ssrnx_eventfd_poll(oldfd, 0) >= 0) {
        if (oldfd == newfd) return oldfd;
        errno = ENOTSUP; return -1;
    }
    if (ssrnx_fd_is_virtual(oldfd) || ssrnx_fd_is_virtual(newfd)) { errno = ENOTSUP; return -1; }
    if (oldfd != newfd) compat_ssrnx_flock_release_fd(newfd); /* dup2 closes newfd's description */
    const int out = dup2(oldfd, newfd);
    if (out < 0) { compat_ssrnx_network_fix_errno(); return -1; }
    if (ssrnx_file_contract_dup) ssrnx_file_contract_dup(oldfd, out);
    if (ssrnx_epoll_duplicated) ssrnx_epoll_duplicated(oldfd,out);
    return out;
}

/* random stdio */
/* Virtual random-device FILE streams use the same guest fd/backend as read(). */
typedef struct compat_SsrnxRandomStream {
    FILE *file;
    int fd;
    struct compat_SsrnxRandomStream *next;
} compat_SsrnxRandomStream;
static compat_SsrnxRandomStream *compat_g_random_streams;
static pthread_mutex_t compat_g_random_stream_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned compat_g_random_stream_reads;

static bool compat_ssrnx_random_stdio_path(const char *path) {
    return path && (!strcmp(path, "/dev/urandom") || !strcmp(path, "/dev/random"));
}
static bool compat_ssrnx_random_stdio_mode(const char *mode) {
    if (!mode || (*mode != 'r' && *mode != 'w' && *mode != 'a')) { errno = EINVAL; return false; }
    /* Horizon's CSRNG has no entropy-pool write interface. */
    if (*mode != 'r' || strchr(mode, '+')) { errno = ENOTSUP; return false; }
    return true;
}
static ssize_t compat_ssrnx_random_stdio_read(void *cookie, char *buffer, size_t count) {
    compat_SsrnxRandomStream *p = cookie;
    const ssize_t rc = ssrnx_fd_read(p->fd, buffer, count);
    const int saved = errno;
    if (rc < 0 || __atomic_fetch_add(&compat_g_random_stream_reads, 1u, __ATOMIC_RELAXED) < 8u)
        SSRNX_LOGCI("ENTROPY", "stdio read fd=%d requested=%zu result=%lld errno=%d",
                    p->fd, count, (long long)rc, rc < 0 ? saved : 0);
    errno = saved;
    return rc;
}
#ifdef __SWITCH__
typedef off_t compat_SsrnxRandomCookieOffset;
#else
typedef off64_t compat_SsrnxRandomCookieOffset;
#endif
static int compat_ssrnx_random_stdio_seek(void *cookie, compat_SsrnxRandomCookieOffset *offset, int whence) {
    compat_SsrnxRandomStream *p = cookie;
    if (!ssrnx_fd_is_random(p->fd)) { errno = EBADF; return -1; }
    if (whence < 0 || whence > 4) { errno = EINVAL; return -1; }
    *offset = 0; /* Linux random_fops uses noop_llseek. */
    return 0;
}
static int compat_ssrnx_random_stdio_close(void *cookie) {
    compat_SsrnxRandomStream *p = cookie;
    pthread_mutex_lock(&compat_g_random_stream_lock);
    compat_SsrnxRandomStream **link = &compat_g_random_streams;
    while (*link && *link != p) link = &(*link)->next;
    if (*link) *link = p->next;
    pthread_mutex_unlock(&compat_g_random_stream_lock);
    const int rc = ssrnx_fd_close(p->fd);
    const int saved = errno;
    free(p);
    errno = saved;
    return rc;
}
static int compat_ssrnx_random_stdio_fileno(FILE *file) {
    int fd = -1;
    pthread_mutex_lock(&compat_g_random_stream_lock);
    for (compat_SsrnxRandomStream *p = compat_g_random_streams; p; p = p->next)
        if (p->file == file) { fd = p->fd; break; }
    pthread_mutex_unlock(&compat_g_random_stream_lock);
    return fd;
}
static FILE *compat_ssrnx_random_stdio_attach(int fd, const char *mode) {
    if (!compat_ssrnx_random_stdio_mode(mode)) return NULL;
    if (!ssrnx_fd_is_random(fd)) { errno = EBADF; return NULL; }
    compat_SsrnxRandomStream *p = calloc(1, sizeof(*p));
    if (!p) { errno = ENOMEM; return NULL; }
    p->fd = fd;
    const cookie_io_functions_t callbacks = {
        .read = compat_ssrnx_random_stdio_read, .seek = compat_ssrnx_random_stdio_seek, .close = compat_ssrnx_random_stdio_close
    };
    FILE *file = fopencookie(p, "r", callbacks);
    if (!file) { const int saved = errno; free(p); errno = saved; return NULL; }
    p->file = file;
    pthread_mutex_lock(&compat_g_random_stream_lock);
    p->next = compat_g_random_streams; compat_g_random_streams = p;
    pthread_mutex_unlock(&compat_g_random_stream_lock);
    return file;
}
static FILE *compat_ssrnx_random_stdio_open(const char *mode) {
    if (!compat_ssrnx_random_stdio_mode(mode)) return NULL;
    const int fd = ssrnx_fd_open_urandom();
    if (fd < 0) return NULL;
    FILE *file = compat_ssrnx_random_stdio_attach(fd, mode);
    if (!file) { const int saved = errno; (void)ssrnx_fd_close(fd); errno = saved; }
    return file;
}
static void *compat_ssrnx_bionic_fdopen(int fd, const char *mode) {
    if (ssrnx_fd_is_random(fd)) return compat_ssrnx_random_stdio_attach(fd, mode);
    if (ssrnx_fd_is_virtual(fd)) { errno = EBADF; return NULL; }
    return fdopen(fd, mode);
}



static void *compat_ssrnx_bionic_fopen(const char *path, const char *mode) {
    SSRNX_IO_TIMING_PATH(path);
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    if (!path || !mode) { errno = EINVAL; return NULL; }
    if (compat_ssrnx_random_stdio_path(path)) {
        FILE *file = compat_ssrnx_random_stdio_open(mode);
        const int saved = errno;
        SSRNX_LOGCI("ENTROPY", "fopen path=%s mode=%s result=%p errno=%d caller=%p",
                    path, mode, (void *)file, file ? 0 : saved, __builtin_return_address(0));
        errno = saved;
        return file;
    }
    const bool writing = strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+');
    if (writing && compat_ssrnx_guest_path_is_readonly_system(path)) { errno = EROFS; return NULL; }
    char mapped[SSRNX_STORAGE_PATH_CAP];
    bool path_mapped = false;
    const char *host_path = compat_ssrnx_map_guest_path(path, mapped, sizeof(mapped), &path_mapped);
    if (writing && compat_ssrnx_kernel_view_host_path(host_path)) { errno = EACCES; return NULL; }
    if (!writing && ssrnx_path_lookup_missing_parent && ssrnx_path_lookup_missing_parent(host_path)) return NULL;
    FILE *f = fopen(host_path, mode);
    if (f && compat_ssrnx_storage_host_path(host_path)) compat_ssrnx_storage_stream_open(f, host_path, mode);
    if (!f && !writing && errno == ENOENT && ssrnx_path_lookup_record_missing_parent)
        ssrnx_path_lookup_record_missing_parent(host_path);
    if (strncmp(path, "/proc/", 6) == 0) {
        const int saved = errno;
        const uintptr_t caller = (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0));
        char site[192];
        (void)ssrnx_compat_describe_guest_address(caller, site, sizeof(site));
        SSRNX_LOGCI("PROC", "fopen path=%s mapped=%d host=%s mode=%s result=%p errno=%d caller=%s",
                    path, path_mapped ? 1 : 0, host_path, mode, (void *)f,
                    f ? 0 : saved, site);
        errno = saved;
    }
    if (writing && compat_ssrnx_storage_host_path(host_path)) {
        __atomic_fetch_add(&compat_g_storage_write_opens, 1u, __ATOMIC_RELAXED);
        const int saved = errno;
        compat_ssrnx_storage_log("fopen-write", path, host_path, f ? 0 : -1);
        errno = saved;
    } else if (!writing && compat_ssrnx_storage_host_path(host_path) &&
               __atomic_fetch_add(&compat_g_storage_query_trace_count, 1u, __ATOMIC_RELAXED) < 96u) {
        const int saved = errno;
        SSRNX_LOGCI("STORAGE", "fopen-read guest=%s host=%s result=%d errno=%d",
                    path, host_path, f ? 0 : -1, f ? 0 : saved);
        errno = saved;
    }
    return f;
}

static void *compat_ssrnx_bionic_freopen(const char *path, const char *mode, void *stream) {
    if (!path || !mode || !stream) { errno = EFAULT; return NULL; }
    if (compat_ssrnx_guest_path_is_readonly_system(path) && (strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+'))) {
        errno = EROFS;
        return NULL;
    }
    if (compat_ssrnx_random_stdio_path(path) || compat_ssrnx_random_stdio_fileno(compat_ssrnx_host_file(stream)) >= 0) {
        (void)compat_ssrnx_bionic_fclose(stream);
        return compat_ssrnx_bionic_fopen(path, mode);
    }
    char mapped[SSRNX_STORAGE_PATH_CAP];
    const char *host_path = compat_ssrnx_map_guest_path(path, mapped, sizeof(mapped), NULL);
    if ((strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+')) &&
        compat_ssrnx_kernel_view_host_path(host_path)) { errno = EACCES; return NULL; }

    FILE *file = compat_ssrnx_host_file(stream);
    char old_tmp[SSRNX_STORAGE_PATH_CAP] = {0};
    char old_path[SSRNX_STORAGE_PATH_CAP];
    uint64_t old_bytes = 0;
    const bool was_tmp = compat_ssrnx_tmpfile_take(file, old_tmp, sizeof(old_tmp));
    (void)compat_ssrnx_storage_stream_take(file, old_path, sizeof(old_path), &old_bytes);
    FILE *reopened = freopen(host_path, mode, file);
    const int saved_errno = errno;
    if (was_tmp) (void)unlink(old_tmp);
    if (!reopened) { errno = saved_errno; return NULL; }

    const bool writing = strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+');
    if (compat_ssrnx_storage_host_path(host_path)) {
        compat_ssrnx_storage_stream_open(reopened, host_path, mode);
    }
    if (writing && compat_ssrnx_storage_host_path(host_path)) {
        __atomic_fetch_add(&compat_g_storage_write_opens, 1u, __ATOMIC_RELAXED);
    }
    if (old_path[0] && old_bytes && __atomic_fetch_add(&compat_g_storage_stream_trace_count, 1u, __ATOMIC_RELAXED) < 32u)
        SSRNX_LOGCI("STORAGE", "freopen replaced host=%s bytes=%llu result=ok",
                    old_path, (unsigned long long)old_bytes);
    return reopened;
}

static int compat_ssrnx_bionic_mkdir(const char *path, mode_t mode) {
    if (!path) { errno = EFAULT; return -1; }
    if (compat_ssrnx_guest_path_is_readonly_system(path)) { errno = EROFS; return -1; }
    char mapped[SSRNX_STORAGE_PATH_CAP];
    const char *host_path = compat_ssrnx_map_guest_path(path, mapped, sizeof(mapped), NULL);
    const int rc = mkdir(host_path, mode);
    if (rc == 0 && ssrnx_path_lookup_changed) ssrnx_path_lookup_changed(host_path);
    if (rc == 0 && ssrnx_android_file_observer_notify_event)
        ssrnx_android_file_observer_notify_event(host_path, SSRNX_FO_CREATE);
    if (compat_ssrnx_storage_host_path(host_path)) {
        __atomic_fetch_add(&compat_g_storage_mutations, 1u, __ATOMIC_RELAXED);
        const int saved = errno; compat_ssrnx_storage_log("mkdir", path, host_path, rc); errno = saved;
    }
    return rc;
}

static int compat_ssrnx_bionic_unlink(const char *path) {
    SSRNX_IO_TIMING();
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    if (!path) { errno = EFAULT; return -1; }
    if (compat_ssrnx_guest_path_is_readonly_system(path)) { errno = EROFS; return -1; }
    char mapped[SSRNX_STORAGE_PATH_CAP];
    const char *host_path = compat_ssrnx_map_guest_path(path, mapped, sizeof(mapped), NULL);
    const int rc = unlink(host_path);
    if (rc == 0 && ssrnx_android_file_observer_notify_event)
        ssrnx_android_file_observer_notify_event(host_path, SSRNX_FO_DELETE);
    if (compat_ssrnx_storage_host_path(host_path)) {
        __atomic_fetch_add(&compat_g_storage_mutations, 1u, __ATOMIC_RELAXED);
        const int saved = errno; compat_ssrnx_storage_log("unlink", path, host_path, rc); errno = saved;
    }
    return rc;
}

static int compat_ssrnx_bionic_remove(const char *path) {
    if (!path) { errno = EFAULT; return -1; }
    if (compat_ssrnx_guest_path_is_readonly_system(path)) { errno = EROFS; return -1; }
    char mapped[SSRNX_STORAGE_PATH_CAP];
    const char *host_path = compat_ssrnx_map_guest_path(path, mapped, sizeof(mapped), NULL);
    const int rc = remove(host_path);
    if (rc == 0 && ssrnx_android_file_observer_notify_event)
        ssrnx_android_file_observer_notify_event(host_path, SSRNX_FO_DELETE);
    if (compat_ssrnx_storage_host_path(host_path)) {
        __atomic_fetch_add(&compat_g_storage_mutations, 1u, __ATOMIC_RELAXED);
        const int saved = errno; compat_ssrnx_storage_log("remove", path, host_path, rc); errno = saved;
    }
    return rc;
}

/* storage rename */
#ifdef __SWITCH__
typedef struct compat_SsrnxRenameStream {
    compat_SsrnxStorageStreamTrace *stream;
    const devoptab_t *device;
    void *data;
    off_t offset;
    bool open;
} compat_SsrnxRenameStream;

static int compat_ssrnx_rename_stream_rebind(compat_SsrnxRenameStream *s, const char *path) {
    struct _reent call = *_REENT;
    call.deviceData = s->device->deviceData;
    if (s->device->open_r(&call, s->data, path, s->stream->host_flags, 0) < 0) {
        errno = call._errno;
        return -1;
    }
    s->open = true;
    if (s->device->seek_r(&call, s->data, s->offset, SEEK_SET) < 0) {
        const int saved = call._errno;
        (void)s->device->close_r(&call, s->data);
        s->open = false;
        errno = saved;
        return -1;
    }
    return 0;
}

/* fsdev locks names while open. Rebind its opaque backend data, keeping the
 * newlib descriptor and FILE (including buffers/error state) alive throughout. */
static int compat_ssrnx_storage_rename_open_streams(const char *old_path, const char *new_path,
                                            unsigned *host_result) {
    compat_SsrnxRenameStream streams[SSRNX_STORAGE_STREAM_MAX];
    size_t count = 0;
    int rc = -1, error = 0, restore_error = 0;
    bool io_locked = false, moved = false, backed_up = false;
    char backup[SSRNX_STORAGE_PATH_CAP] = {0};
retry:
    count = 0;
    pthread_mutex_lock(&compat_g_storage_stream_lock);
    /* Replacing an open destination needs inode retention, not rebinding its
     * readers to the source. Leave that unsupported case as the real error. */
    for (size_t i = 0; i < SSRNX_STORAGE_STREAM_MAX; ++i) {
        if (compat_g_storage_streams[i].file && strcmp(compat_g_storage_streams[i].host_path, new_path) == 0 &&
            strcmp(old_path, new_path) != 0) {
            rc = -2;
            goto done;
        }
    }
    for (size_t i = 0; i < SSRNX_STORAGE_STREAM_MAX; ++i) {
        compat_SsrnxStorageStreamTrace *s = &compat_g_storage_streams[i];
        if (!s->file || strcmp(s->host_path, old_path) != 0) continue;
        /* A caller may hold flockfile while fwrite updates the registry. */
        if (ftrylockfile(s->file) != 0) {
            for (size_t j = 0; j < count; ++j) funlockfile(streams[j].stream->file);
            pthread_mutex_unlock(&compat_g_storage_stream_lock);
            compat_ssrnx_host_yield();
            goto retry;
        }
        streams[count++] = (compat_SsrnxRenameStream){.stream = s, .open = true};
    }
    if (!count) { rc = -2; goto done; }
    if (strlen(new_path) >= SSRNX_STORAGE_PATH_CAP) { error = ENAMETOOLONG; goto done; }
    pthread_mutex_lock(&compat_g_regular_file_io_lock);
    io_locked = true;
    for (size_t i = 0; i < count; ++i) {
        compat_SsrnxRenameStream *s = &streams[i];
        __handle *handle = __get_handle(fileno(s->stream->file));
        const devoptab_t *device = handle && handle->device < STD_MAX ? devoptab_list[handle->device] : NULL;
        if (!handle || handle->refcount != 1 || !device || device != GetDeviceOpTab(old_path) ||
            !device->open_r || !device->close_r || !device->seek_r || !device->fstat_r) {
            rc = -2;
            goto done;
        }
        s->device = device;
        s->data = handle->fileStruct;
        struct _reent call = *_REENT;
        call.deviceData = device->deviceData;
        struct stat st;
        if (device->fstat_r(&call, s->data, &st) < 0) { error = call._errno; goto done; }
        if (!S_ISREG(st.st_mode)) { rc = -2; goto done; }
        s->offset = device->seek_r(&call, s->data, 0, SEEK_CUR);
        if (s->offset < 0) { error = call._errno; goto done; }
    }
    if (strcmp(old_path, new_path) == 0) { rc = 0; *host_result = 0; goto done; }
    struct stat destination;
    const int exists = lstat(new_path, &destination);
    if (exists < 0 && errno != ENOENT) { error = errno; goto done; }
    if (exists == 0) {
        if (S_ISDIR(destination.st_mode)) { error = EISDIR; goto done; }
        for (unsigned n = 0; n < 32u; ++n) {
            if (snprintf(backup, sizeof(backup), "%s.apn64-rbak-%u", new_path, n) >= (int)sizeof(backup)) {
                error = ENAMETOOLONG; goto done;
            }
            struct stat st;
            if (lstat(backup, &st) < 0 && errno == ENOENT) break;
            if (n == 31u) { error = EEXIST; goto done; }
        }
    }
    for (size_t i = 0; i < count; ++i) {
        struct _reent call = *_REENT;
        call.deviceData = streams[i].device->deviceData;
        if (streams[i].device->close_r(&call, streams[i].data) < 0) { error = call._errno; goto restore; }
        streams[i].open = false;
    }
    if (exists == 0) {
        if (rename(new_path, backup) < 0) { error = errno; *host_result = fsdevGetLastResult(); goto restore; }
        backed_up = true;
    }
    if (rename(old_path, new_path) < 0) { error = errno; *host_result = fsdevGetLastResult(); goto restore; }
    moved = true;
    for (size_t i = 0; i < count; ++i) {
        if (compat_ssrnx_rename_stream_rebind(&streams[i], new_path) < 0) {
            error = errno;
            *host_result = fsdevGetLastResult();
            goto restore;
        }
    }
    for (size_t i = 0; i < count; ++i)
        snprintf(streams[i].stream->host_path, SSRNX_STORAGE_PATH_CAP, "%s", new_path);
    if (backed_up) (void)remove(backup);
    rc = 0;
    *host_result = 0;
    goto done;

restore:
    if (moved) {
        for (size_t i = 0; i < count; ++i) {
            if (!streams[i].open) continue;
            struct _reent call = *_REENT;
            call.deviceData = streams[i].device->deviceData;
            (void)streams[i].device->close_r(&call, streams[i].data);
            streams[i].open = false;
        }
        if (rename(new_path, old_path) == 0) moved = false;
        else restore_error = errno;
    }
    if (backed_up && !moved && rename(backup, new_path) < 0) restore_error = errno;
    for (size_t i = 0; i < count; ++i) {
        if (!streams[i].open && compat_ssrnx_rename_stream_rebind(&streams[i], moved ? new_path : old_path) < 0)
            restore_error = errno;
        if (moved) snprintf(streams[i].stream->host_path, SSRNX_STORAGE_PATH_CAP, "%s", new_path);
    }
done:
    if (io_locked) pthread_mutex_unlock(&compat_g_regular_file_io_lock);
    if (rc != -2) {
        static unsigned reports;
        if (restore_error || __atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 32u)
            SSRNX_LOGCI("STORAGE", "rename-open source=%s destination=%s streams=%u result=%d errno=%d hostResult=%#x restoreErrno=%d",
                        old_path, new_path, (unsigned)count, rc, error, *host_result, restore_error);
    }
    for (size_t i = 0; i < count; ++i) funlockfile(streams[i].stream->file);
    pthread_mutex_unlock(&compat_g_storage_stream_lock);
    if (error) errno = error;
    return rc;
}
#endif



static int compat_ssrnx_bionic_rename(const char *old_path, const char *new_path) {
    SSRNX_IO_TIMING();
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    if (!old_path || !new_path) { errno = EFAULT; return -1; }
    if (compat_ssrnx_guest_path_is_readonly_system(old_path) || compat_ssrnx_guest_path_is_readonly_system(new_path)) { errno = EROFS; return -1; }
    const compat_SsrnxGuestStorageDomain old_domain = compat_ssrnx_guest_storage_domain(old_path);
    const compat_SsrnxGuestStorageDomain new_domain = compat_ssrnx_guest_storage_domain(new_path);
    if (old_domain != SSRNX_STORAGE_DOMAIN_OTHER &&
        new_domain != SSRNX_STORAGE_DOMAIN_OTHER && old_domain != new_domain) {
        errno = EXDEV;
        return -1;
    }
    char old_mapped[SSRNX_STORAGE_PATH_CAP], new_mapped[SSRNX_STORAGE_PATH_CAP];
    const char *old_host = compat_ssrnx_map_guest_path(old_path, old_mapped, sizeof(old_mapped), NULL);
    const char *new_host = compat_ssrnx_map_guest_path(new_path, new_mapped, sizeof(new_mapped), NULL);
    int rc = rename(old_host, new_host);
    unsigned host_result = 0;
    int rename_errno = errno;
#ifdef __SWITCH__
    if (rc < 0) host_result = fsdevGetLastResult();
    if (rc < 0 && host_result == 0xe02u &&
        compat_ssrnx_storage_host_path(old_host) && compat_ssrnx_storage_host_path(new_host)) {
        const int retried = compat_ssrnx_storage_rename_open_streams(old_host, new_host, &host_result);
        if (retried != -2) { rc = retried; rename_errno = errno; }
        errno = rename_errno;
    }
    /* POSIX/Android rename() replaces an existing non-directory destination.
     * Horizon's fsdev rename path may instead report EEXIST/ENOTEMPTY.  Restore
     * the guest contract for SSRNX-owned storage without changing unrelated
     * host paths. */
    if (rc < 0 && (errno == EEXIST || errno == ENOTEMPTY) &&
        (compat_ssrnx_storage_host_path(old_host) || compat_ssrnx_storage_host_path(new_host))) {
        char backup[SSRNX_STORAGE_PATH_CAP];
        bool backup_ready = false;
        for (unsigned attempt = 0; attempt < 32u; ++attempt) {
            if (snprintf(backup, sizeof(backup), "%s.apn64-rbak-%u", new_host, attempt) >=
                    (int)sizeof(backup)) break;
            struct stat st;
            if (lstat(backup, &st) != 0 && errno == ENOENT) { backup_ready = true; break; }
        }
        if (backup_ready) {
            bool moved_destination = false;
            struct stat dst;
            if (lstat(new_host, &dst) == 0) {
                if (!S_ISDIR(dst.st_mode) && rename(new_host, backup) == 0)
                    moved_destination = true;
            } else if (errno == ENOENT) {
                moved_destination = false;
            }
            if (moved_destination || errno == ENOENT) {
                rc = rename(old_host, new_host);
                const int replacement_errno = errno;
                rename_errno = replacement_errno;
                host_result = rc < 0 ? fsdevGetLastResult() : 0;
                if (rc == 0) {
                    if (moved_destination) (void)remove(backup);
                } else if (moved_destination) {
                    (void)rename(backup, new_host);
                    errno = replacement_errno;
                }
            }
        }
    }
#endif
    if (ssrnx_path_lookup_changed) {
        /* Failed overwrite attempts can temporarily move and restore a node.
         * A miss observed during that interval must not survive rollback. */
        ssrnx_path_lookup_changed(old_host);
        ssrnx_path_lookup_changed(new_host);
    }
    if (rc == 0 && ssrnx_android_file_observer_notify_move)
        ssrnx_android_file_observer_notify_move(old_host, new_host);
    if (compat_ssrnx_storage_host_path(old_host) || compat_ssrnx_storage_host_path(new_host)) {
        __atomic_fetch_add(&compat_g_storage_mutations, 1u, __ATOMIC_RELAXED);
        const int saved = rc < 0 ? rename_errno : errno;
        const uint64_t index = __atomic_fetch_add(&compat_g_storage_log_count, 1u, __ATOMIC_RELAXED);
        static unsigned failure_reports;
        if (index < 48u || (rc < 0 && __atomic_fetch_add(&failure_reports, 1u, __ATOMIC_RELAXED) < 16u)) {
            int source_fd = -1, destination_fd = -1;
#ifdef __SWITCH__
            pthread_mutex_lock(&compat_g_storage_stream_lock);
            for (size_t i = 0; i < SSRNX_STORAGE_STREAM_MAX; ++i) {
                if (!compat_g_storage_streams[i].file) continue;
                if (strcmp(compat_g_storage_streams[i].host_path, old_host) == 0)
                    source_fd = fileno(compat_g_storage_streams[i].file);
                if (strcmp(compat_g_storage_streams[i].host_path, new_host) == 0)
                    destination_fd = fileno(compat_g_storage_streams[i].file);
            }
            pthread_mutex_unlock(&compat_g_storage_stream_lock);
            if (ssrnx_file_contract_fd_for_host_path) {
                if (source_fd < 0) source_fd = ssrnx_file_contract_fd_for_host_path(old_host);
                if (destination_fd < 0) destination_fd = ssrnx_file_contract_fd_for_host_path(new_host);
            }
#endif
            SSRNX_LOGCI("STORAGE", "rename guest=%s -> %s host=%s -> %s result=%d errno=%d hostResult=%#x sourceOpenFd=%d destinationOpenFd=%d",
                        old_path, new_path, old_host, new_host, rc, rc < 0 ? saved : 0,
                        host_result, source_fd, destination_fd);
        }
        errno = saved;
    }
    if (rc < 0) errno = rename_errno;
    return rc;
}

static int compat_ssrnx_bionic_renameat(int old_dirfd, const char *old_path,
                                  int new_dirfd, const char *new_path) {
    if (!old_path || !new_path) { errno = EFAULT; return -1; }
    if ((old_path[0] != '/' && old_dirfd != -100) ||
        (new_path[0] != '/' && new_dirfd != -100)) {
        errno = ENOTSUP;
        return -1;
    }
    return compat_ssrnx_bionic_rename(old_path, new_path);
}

static DIR *compat_ssrnx_bionic_opendir(const char *path) {
    if (!path) { errno = EFAULT; return NULL; }
    char mapped[SSRNX_STORAGE_PATH_CAP];
    return opendir(compat_ssrnx_map_guest_path(path, mapped, sizeof(mapped), NULL));
}

static int compat_ssrnx_bionic_access(const char *path, int mode) {
    if (!path) { errno = EFAULT; return -1; }
    if ((mode & W_OK) && compat_ssrnx_guest_path_is_readonly_system(path)) { errno = EACCES; return -1; }
    if ((strcmp(path, "/dev/urandom") == 0 || strcmp(path, "/dev/random") == 0) &&
        (mode == F_OK || (mode & ~(R_OK)) == 0)) return 0;
    char mapped[SSRNX_STORAGE_PATH_CAP];
    const char *host_path = compat_ssrnx_map_guest_path(path, mapped, sizeof(mapped), NULL);
    int rc = access(host_path, mode);
    if (rc != 0 && errno == EIO) {
        /* Same open-for-write lock artefact as stat(): the file exists. */
        struct stat st;
        if (compat_ssrnx_storage_stat_open_file(host_path, &st) == 0) rc = 0;
        else errno = EIO;
    }
    if (compat_ssrnx_storage_host_path(host_path) &&
        __atomic_fetch_add(&compat_g_storage_query_trace_count, 1u, __ATOMIC_RELAXED) < 96u) {
        const int saved = errno;
        SSRNX_LOGCI("STORAGE", "access guest=%s host=%s mode=%d result=%d errno=%d",
                    path, host_path, mode, rc, rc == 0 ? 0 : saved);
        errno = saved;
    }
    return rc;
}


static int compat_ssrnx_bionic_faccessat(int dirfd, const char *path, int mode, int flags) {
    if (!path) { errno = EFAULT; return -1; }
    if (flags != 0) { errno = ENOTSUP; return -1; }
    if (path[0] != '/' && dirfd != -100) { errno = ENOTSUP; return -1; }
    return compat_ssrnx_bionic_access(path, mode);
}

/* chdir/getcwd operate on the guest working directory used by
 * ssrnx_map_guest_path(); the Switch host cwd is not part of the guest ABI. */
static int compat_ssrnx_bionic_chdir(const char *path) {
    if (!path) { errno = EFAULT; return -1; }
    if (!path[0]) { errno = ENOENT; return -1; }
    char absolute[SSRNX_STORAGE_PATH_CAP];
    if (!compat_ssrnx_guest_absolute_path(path, absolute, sizeof(absolute))) { errno = ENAMETOOLONG; return -1; }
    char mapped[SSRNX_STORAGE_PATH_CAP];
    struct stat st;
    if (compat_ssrnx_storage_stat(compat_ssrnx_map_guest_path(absolute, mapped, sizeof(mapped), NULL), &st) != 0) return -1;
    if (!S_ISDIR(st.st_mode)) { errno = ENOTDIR; return -1; }
    pthread_mutex_lock(&compat_g_guest_cwd_lock);
    memcpy(compat_g_guest_cwd, absolute, strlen(absolute) + 1u);
    pthread_mutex_unlock(&compat_g_guest_cwd_lock);
    return 0;
}

static char *compat_ssrnx_bionic_getcwd(char *buf, size_t size) {
    if (buf && size == 0) { errno = EINVAL; return NULL; }
    char cwd[SSRNX_STORAGE_PATH_CAP];
    pthread_mutex_lock(&compat_g_guest_cwd_lock);
    memcpy(cwd, compat_g_guest_cwd, strlen(compat_g_guest_cwd) + 1u);
    pthread_mutex_unlock(&compat_g_guest_cwd_lock);
    const size_t need = strlen(cwd) + 1u;
    if (!buf) {
        /* Bionic extension: allocate max(size, need) bytes for the caller. */
        buf = malloc(size > need ? size : need);
        if (!buf) { errno = ENOMEM; return NULL; }
    } else if (size < need) {
        errno = ERANGE;
        return NULL;
    }
    memcpy(buf, cwd, need);
    return buf;
}

static int64_t compat_ssrnx_bionic_lseek(int fd, int64_t offset, int whence) {
    if (ssrnx_eventfd_poll && ssrnx_eventfd_poll(fd, 0) >= 0) return 0;
    if (ssrnx_fd_is_random(fd)) {
        if (whence < 0 || whence > 4) { errno = EINVAL; return -1; }
        return 0;
    }
    if (ssrnx_fd_is_virtual(fd)) { errno = ESPIPE; return -1; }
    const bool locked = compat_ssrnx_regular_file_io_lock(fd);
    const off_t result = lseek(fd, (off_t)offset, whence);
    const int saved = errno;
    compat_ssrnx_regular_file_io_unlock(locked);
    errno = saved;
    if (result == (off_t)-1) { compat_ssrnx_network_fix_errno(); return -1; }
    return (int64_t)result;
}

static ssize_t compat_ssrnx_bionic_pread(int fd, void *buffer, size_t count, int64_t offset) {
    SSRNX_IO_TIMING();
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    if (ssrnx_fd_is_virtual(fd)) { errno = ESPIPE; return -1; }
    if (offset < 0) { errno = EINVAL; return -1; }

    const bool locked = compat_ssrnx_regular_file_io_lock(fd);
    const off_t original = lseek(fd, 0, SEEK_CUR);
    if (original == (off_t)-1) {
        const int saved = errno;
        compat_ssrnx_regular_file_io_unlock(locked);
        errno = saved;
        compat_ssrnx_network_fix_errno();
        return -1;
    }
    if (lseek(fd, (off_t)offset, SEEK_SET) == (off_t)-1) {
        const int saved = errno;
        compat_ssrnx_regular_file_io_unlock(locked);
        errno = saved;
        compat_ssrnx_network_fix_errno();
        return -1;
    }

    ssize_t rc = read(fd, buffer, count);
    int saved = errno;
    if (lseek(fd, original, SEEK_SET) == (off_t)-1 && rc >= 0) {
        rc = -1;
        saved = errno;
    }
    compat_ssrnx_regular_file_io_unlock(locked);
    errno = saved;
    if (rc < 0) compat_ssrnx_network_fix_errno();
    if (ssrnx_file_contract_read_at) ssrnx_file_contract_read_at(fd, offset, (int64_t)rc);
    else if (ssrnx_file_contract_read) ssrnx_file_contract_read(fd, count, (int64_t)rc);
    return rc;
}

static ssize_t compat_ssrnx_bionic_pwrite(int fd, const void *buffer, size_t count, int64_t offset) {
    SSRNX_IO_TIMING();
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    if (ssrnx_fd_is_virtual(fd)) { errno = ESPIPE; return -1; }
    if (offset < 0) { errno = EINVAL; return -1; }

    const bool locked = compat_ssrnx_regular_file_io_lock(fd);
    const off_t original = lseek(fd, 0, SEEK_CUR);
    if (original == (off_t)-1) {
        const int saved = errno;
        compat_ssrnx_regular_file_io_unlock(locked);
        errno = saved;
        compat_ssrnx_network_fix_errno();
        return -1;
    }

    int old_flags = ssrnx_file_contract_status_flags ? ssrnx_file_contract_status_flags(fd) : -2;
    if (old_flags == -2) old_flags = fcntl(fd, F_GETFL, 0);
    const bool had_append = old_flags >= 0 && (old_flags & O_APPEND) != 0;
    bool append_cleared = false;
    if (had_append) {
        if (fcntl(fd, F_SETFL, old_flags & ~O_APPEND) != 0) {
            const int saved = errno;
            compat_ssrnx_regular_file_io_unlock(locked);
            errno = saved;
            compat_ssrnx_network_fix_errno();
            return -1;
        }
        append_cleared = true;
    }

    ssize_t rc = -1;
    int saved = 0;
    if (lseek(fd, (off_t)offset, SEEK_SET) == (off_t)-1) {
        saved = errno;
    } else {
        rc = write(fd, buffer, count);
        saved = errno;
    }

    if (lseek(fd, original, SEEK_SET) == (off_t)-1 && rc >= 0) {
        rc = -1;
        saved = errno;
    }
    if (append_cleared && fcntl(fd, F_SETFL, old_flags) != 0 && rc >= 0) {
        rc = -1;
        saved = errno;
    }
    compat_ssrnx_regular_file_io_unlock(locked);
    errno = saved;
    if (rc < 0) compat_ssrnx_network_fix_errno();
    if (ssrnx_file_contract_write) ssrnx_file_contract_write(fd, count, (int64_t)rc);
    return rc;
}

static int compat_ssrnx_bionic_isatty(int fd) {
    if (ssrnx_fd_is_virtual(fd)) { errno = ENOTTY; return 0; }
    const int rc = isatty(fd);
    if (rc == 0 && errno != 0) compat_ssrnx_network_fix_errno();
    return rc;
}

static int compat_ssrnx_bionic_pipe(int fds[2]) {
    const int rc = ssrnx_fd_pipe(fds), saved = errno;
    static unsigned reports;
    if (__atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 16u)
        SSRNX_LOGCI("BIONIC", "pipe result=%d fds=%d,%d hostErrno=%d caller=%p", rc,
                    rc == 0 ? fds[0] : -1, rc == 0 ? fds[1] : -1, rc < 0 ? saved : 0,
                    __builtin_extract_return_addr(__builtin_return_address(0)));
    errno = saved;
    return rc;
}

/* The virtual pipe backend reports EAGAIN when empty and integrates with
 * poll(). CLOEXEC has no effect in the current single-process Horizon model. */
static int compat_ssrnx_bionic_pipe2(int fds[2], int flags) {
    enum { SSRNX_O_NONBLOCK = 00004000, SSRNX_O_CLOEXEC = 02000000 };
    if (flags & ~(SSRNX_O_NONBLOCK | SSRNX_O_CLOEXEC)) { errno = EINVAL; return -1; }
    const int rc = ssrnx_fd_pipe(fds), saved = errno;
    static unsigned reports;
    if (__atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 16u)
        SSRNX_LOGCI("BIONIC", "pipe2 flags=%#x result=%d fds=%d,%d hostErrno=%d caller=%p", flags, rc,
                    rc == 0 ? fds[0] : -1, rc == 0 ? fds[1] : -1, rc < 0 ? saved : 0,
                    __builtin_extract_return_addr(__builtin_return_address(0)));
    errno = saved;
    return rc;
}

/* Android paths and descriptors have SSRNX ownership; never expose a host
 * popen child or claim that a Horizon shell command ran. */
static void *compat_ssrnx_bionic_popen(const char *command, const char *mode) {
    if (!command || !mode) { errno = EFAULT; return NULL; }
    if ((strcmp(mode, "r") != 0 && strcmp(mode, "w") != 0) ||
        strchr(mode, '+') || strchr(mode, 'b')) { errno = EINVAL; return NULL; }
    errno = ENOSYS;
    return NULL;
}

static int compat_ssrnx_bionic_pclose(void *stream) {
    (void)stream;
    errno = ECHILD;
    return -1;
}

static int compat_ssrnx_bionic_getrlimit(int resource, void *limit) {
    if (!limit) { errno = EFAULT; return -1; }
    if (resource < 0 || resource > 15) { errno = EINVAL; return -1; }
    errno = ENOSYS;
    return -1;
}

static int compat_ssrnx_bionic_setitimer(int which, const void *value, void *old_value) {
    if (which < 0 || which > 2) { errno = EINVAL; return -1; }
    if (!value) { errno = EFAULT; return -1; }
    (void)old_value;
    errno = ENOSYS;
    return -1;
}

static int compat_ssrnx_bionic_mkstemp(char *path) {
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    if (!path) { errno = EFAULT; return -1; }
    char guest_path[SSRNX_STORAGE_PATH_CAP], host_path[SSRNX_STORAGE_PATH_CAP];
    const size_t length = strnlen(path, sizeof(guest_path));
    if (length == 0 || length >= sizeof(guest_path)) { errno = ENAMETOOLONG; return -1; }
    const char *leaf = strrchr(path, '/');
    leaf = leaf ? leaf + 1 : path;
    if (strlen(leaf) < 6 || strcmp(leaf + strlen(leaf) - 6, "XXXXXX") != 0) {
        errno = EINVAL;
        return -1;
    }
    memcpy(guest_path, path, length + 1);
    char mapped[SSRNX_STORAGE_PATH_CAP];
    const char *mapped_path = compat_ssrnx_map_guest_path(guest_path, mapped, sizeof(mapped), NULL);
    const size_t host_length = strnlen(mapped_path, sizeof(host_path));
    if (host_length >= sizeof(host_path)) { errno = ENAMETOOLONG; return -1; }
    memcpy(host_path, mapped_path, host_length + 1);
    const size_t suffix = host_length - 6;
    for (unsigned attempt = 0; attempt < 128; ++attempt) {
        unsigned char random[6];
        ssrnx_fd_random_fill(random, sizeof(random));
        for (size_t i = 0; i < sizeof(random); ++i)
            host_path[suffix + i] = alphabet[random[i] % (sizeof(alphabet) - 1u)];
        const int fd = open(host_path, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            memcpy(path + length - 6, host_path + suffix, 6);
            const char *host_root = compat_ssrnx_storage_host_path(host_path) ? host_path : NULL;
            if (ssrnx_file_contract_open && host_root)
                ssrnx_file_contract_open(fd, path, host_path,
                                         SSRNX_ANDROID_O_RDWR | SSRNX_ANDROID_O_CREAT | SSRNX_ANDROID_O_EXCL,
                                         O_RDWR | O_CREAT | O_EXCL, false);
            if (ssrnx_android_file_observer_notify_event && host_root)
                ssrnx_android_file_observer_notify_event(host_path, SSRNX_FO_CREATE);
            return fd;
        }
        if (errno != EEXIST) return -1;
    }
    errno = EEXIST;
    return -1;
}

static ssize_t compat_ssrnx_bionic_read(int fd, void *buffer, size_t count) {
    SSRNX_IO_TIMING();
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    if (ssrnx_fd_is_virtual(fd)) return ssrnx_fd_read(fd, buffer, count);
    const bool locked = compat_ssrnx_regular_file_io_lock(fd);
    ssize_t rc = read(fd, buffer, count);
    const int saved = errno;
    compat_ssrnx_regular_file_io_unlock(locked);
    errno = saved;
    if (rc < 0) compat_ssrnx_network_fix_errno();
    (void)compat_ssrnx_network_socket_io_trace("read", fd, count, rc, 0, true, false,
        (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)));
    if (ssrnx_file_contract_read) ssrnx_file_contract_read(fd, count, (int64_t)rc);
    return rc;
}

static ssize_t compat_ssrnx_bionic_write(int fd, const void *buffer, size_t count) {
    SSRNX_IO_TIMING();
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    if (ssrnx_fd_is_virtual(fd)) return ssrnx_fd_write(fd, buffer, count);
    const bool locked = compat_ssrnx_regular_file_io_lock(fd);
    ssize_t rc = write(fd, buffer, count);
    const int saved = errno;
    if (fd == STDERR_FILENO && rc > 0) compat_ssrnx_guest_stderr_capture(buffer, (size_t)rc);
    compat_ssrnx_regular_file_io_unlock(locked);
    errno = saved;
    if (rc < 0) compat_ssrnx_network_fix_errno();
    (void)compat_ssrnx_network_socket_io_trace("write", fd, count, rc, 0, false, false,
        (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)));
    if (ssrnx_file_contract_write) ssrnx_file_contract_write(fd, count, (int64_t)rc);
    return rc;
}

static int compat_ssrnx_bionic_close(int fd) {
    SSRNX_IO_TIMING();
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    compat_ssrnx_flock_release_fd(fd);
    if (ssrnx_epoll_closed) ssrnx_epoll_closed(fd);
    if (ssrnx_fd_is_virtual(fd)) return ssrnx_fd_close(fd);
    const bool locked = compat_ssrnx_regular_file_io_lock(fd);
    uint64_t rx = 0, tx = 0; bool eof = false;
    const bool tracked = compat_ssrnx_network_socket_take(fd, &rx, &tx, &eof);
    if (ssrnx_file_contract_close) ssrnx_file_contract_close(fd);
    const int rc = close(fd);
    const int close_errno = errno;
    compat_ssrnx_regular_file_io_unlock(locked);
    errno = close_errno;
    if (rc < 0) compat_ssrnx_network_fix_errno();
    if (tracked)
        SSRNX_LOGCI("NET", "close fd=%d rxBytes=%llu txBytes=%llu eof=%d result=%d errno=%d",
                    fd, (unsigned long long)rx, (unsigned long long)tx, eof ? 1 : 0,
                    rc, rc < 0 ? *ssrnx_guest_errno_location() : 0); /* Android errno */
    return rc;
}

static int compat_ssrnx_bionic_clock_gettime(int android_clock_id, compat_SsrnxAndroidTimespec *out) {
    /* Android/Bionic clock IDs are ABI values, not host enum values.  Cocos,
     * curl and OpenSSL may request coarse/raw/boottime/CPU clocks while driving
     * timeouts even when CLOCK_REALTIME/MONOTONIC are the common case.  Map the
     * complete Android 0..9 surface to the closest host clock instead of
     * rejecting valid Android IDs with EINVAL. */
    clockid_t host_clock;
    switch (android_clock_id) {
        case 0: /* CLOCK_REALTIME */
        case 5: /* CLOCK_REALTIME_COARSE */
        case 8: /* CLOCK_REALTIME_ALARM */
            host_clock = CLOCK_REALTIME;
            break;
        case 1: /* CLOCK_MONOTONIC */
        case 6: /* CLOCK_MONOTONIC_COARSE */
            host_clock = CLOCK_MONOTONIC;
            break;
        case 2: /* CLOCK_PROCESS_CPUTIME_ID */
#ifdef CLOCK_PROCESS_CPUTIME_ID
            host_clock = CLOCK_PROCESS_CPUTIME_ID;
#else
            host_clock = CLOCK_MONOTONIC;
#endif
            break;
        case 3: /* CLOCK_THREAD_CPUTIME_ID */
#ifdef CLOCK_THREAD_CPUTIME_ID
            host_clock = CLOCK_THREAD_CPUTIME_ID;
#else
            host_clock = CLOCK_MONOTONIC;
#endif
            break;
        case 4: /* CLOCK_MONOTONIC_RAW */
#ifdef CLOCK_MONOTONIC_RAW
            host_clock = CLOCK_MONOTONIC_RAW;
#else
            host_clock = CLOCK_MONOTONIC;
#endif
            break;
        case 7: /* CLOCK_BOOTTIME */
        case 9: /* CLOCK_BOOTTIME_ALARM */
#ifdef CLOCK_BOOTTIME
            host_clock = CLOCK_BOOTTIME;
#else
            host_clock = CLOCK_MONOTONIC;
#endif
            break;
        default:
            errno = EINVAL;
            return -1;
    }

    if (!compat_ssrnx_process_vm_range_valid(out, sizeof(*out), true)) {
        errno = EFAULT;
        return -1;
    }
    struct timespec ts;
    if (clock_gettime(host_clock, &ts) != 0) return -1;
    out->tv_sec = (int64_t)ts.tv_sec;
    out->tv_nsec = (int64_t)ts.tv_nsec;
    return 0;
}

static int compat_ssrnx_bionic_clock_getres(int android_clock_id, compat_SsrnxAndroidTimespec *out) {
    clockid_t host_clock;
    switch (android_clock_id) {
        case 0: case 5: case 8: host_clock = CLOCK_REALTIME; break;
        case 1: case 6: host_clock = CLOCK_MONOTONIC; break;
        case 2:
#ifdef CLOCK_PROCESS_CPUTIME_ID
            host_clock = CLOCK_PROCESS_CPUTIME_ID;
#else
            host_clock = CLOCK_MONOTONIC;
#endif
            break;
        case 3:
#ifdef CLOCK_THREAD_CPUTIME_ID
            host_clock = CLOCK_THREAD_CPUTIME_ID;
#else
            host_clock = CLOCK_MONOTONIC;
#endif
            break;
        case 4:
#ifdef CLOCK_MONOTONIC_RAW
            host_clock = CLOCK_MONOTONIC_RAW;
#else
            host_clock = CLOCK_MONOTONIC;
#endif
            break;
        case 7: case 9:
#ifdef CLOCK_BOOTTIME
            host_clock = CLOCK_BOOTTIME;
#else
            host_clock = CLOCK_MONOTONIC;
#endif
            break;
        default: errno = EINVAL; return -1;
    }
    /* Linux permits a null result pointer for a valid clock_getres request. */
    if (!out) return 0;
    if (!compat_ssrnx_process_vm_range_valid(out, sizeof(*out), true)) {
        errno = EFAULT;
        return -1;
    }
    struct timespec ts;
    if (clock_getres(host_clock, &ts) != 0) return -1;
    out->tv_sec = (int64_t)ts.tv_sec;
    out->tv_nsec = (int64_t)ts.tv_nsec;
    return 0;
}




/* libc runtime */
/* Stage 13.2: libc++_shared pulls in a much wider libc/Bionic surface than the
 * game itself.  Keep FILE* crossing through the existing Bionic stream bridge
 * and implement locale/Android-specific calls here instead of aliasing guest
 * ABIs blindly to newlib. */
static int compat_ssrnx_bionic_vfprintf(void *stream, const char *fmt, va_list ap) {
    FILE *f = compat_ssrnx_host_file(stream);
    if (f == stderr) compat_ssrnx_guest_stderr_capture_vformat(fmt, ap);
    return vfprintf(f, fmt, ap);
}
static int compat_ssrnx_bionic_fputc(int ch, void *stream) {
    FILE *f = compat_ssrnx_host_file(stream);
    if (f == stderr) { const unsigned char c = (unsigned char)ch; compat_ssrnx_guest_stderr_capture(&c, 1u); }
    return fputc(ch, f);
}
static int compat_ssrnx_bionic_getc(void *stream) {
    return fgetc(compat_ssrnx_host_file(stream));
}
static int compat_ssrnx_bionic_putc(int ch, void *stream) {
    FILE *f = compat_ssrnx_host_file(stream);
    if (f == stderr) { const unsigned char c = (unsigned char)ch; compat_ssrnx_guest_stderr_capture(&c, 1u); }
    return fputc(ch, f);
}
static wint_t compat_ssrnx_bionic_getwc(void *stream) {
    return fgetwc(compat_ssrnx_host_file(stream));
}
static wint_t compat_ssrnx_bionic_putwc(wchar_t wc, void *stream) {
    return fputwc(wc, compat_ssrnx_host_file(stream));
}
static wint_t compat_ssrnx_bionic_ungetwc(wint_t wc, void *stream) {
    return ungetwc(wc, compat_ssrnx_host_file(stream));
}
static int compat_ssrnx_bionic_setvbuf(void *stream, char *buf, int mode, size_t size) {
    return setvbuf(compat_ssrnx_host_file(stream), buf, mode, size);
}
static int compat_ssrnx_bionic_vsprintf(char *dst, const char *fmt, va_list ap) {
    return vsprintf(dst, fmt, ap);
}
static int compat_ssrnx_bionic_vasprintf(char **out, const char *fmt, va_list ap) {
    if (!out || !fmt) { errno = EINVAL; return -1; }
    *out = NULL;
    va_list copy;
    va_copy(copy, ap);
    int needed = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (needed < 0) return -1;
    char *buf = (char *)malloc((size_t)needed + 1u);
    if (!buf) { errno = ENOMEM; return -1; }
    va_copy(copy, ap);
    int written = vsnprintf(buf, (size_t)needed + 1u, fmt, copy);
    va_end(copy);
    if (written < 0) { free(buf); return -1; }
    *out = buf;
    return written;
}
static int compat_ssrnx_bionic_sscanf(const char *src, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int rc = vsscanf(src, fmt, ap);
    va_end(ap); return rc;
}
static char *compat_ssrnx_bionic_strerror(int err) {
    return strerror(ssrnx_errno_android_to_host(err));
}

static int compat_ssrnx_bionic_strerror_r(int err, char *buf, size_t cap) {
    if (!buf || cap == 0) return ERANGE;
    const char *msg = strerror(ssrnx_errno_android_to_host(err));
    if (!msg) msg = "Unknown error";
    size_t n = strlen(msg);
    if (n >= cap) {
        memcpy(buf, msg, cap - 1u);
        buf[cap - 1u] = '\0';
        return ERANGE;
    }
    memcpy(buf, msg, n + 1u);
    return 0;
}

static char *compat_ssrnx_bionic_gnu_strerror_r(int err, char *buf, size_t cap) {
    /* Bionic's GNU compatibility entry point always returns the caller's
     * buffer. strerror_r supplies the bounded copy and NUL termination. */
    if (!buf || cap == 0) return buf;
    (void)compat_ssrnx_bionic_strerror_r(err, buf, cap);
    return buf;
}

static char compat_g_abort_message[512];
static void compat_ssrnx_android_set_abort_message(const char *msg) {
    size_t n = msg ? strlen(msg) : 0;
    if (n >= sizeof(compat_g_abort_message)) n = sizeof(compat_g_abort_message) - 1u;
    if (n) memcpy(compat_g_abort_message, msg, n);
    compat_g_abort_message[n] = '\0';
    SSRNX_LOGCW("BIONIC", "android_set_abort_message: %s", compat_g_abort_message);
}

/* Optional: jni.c owns the process-exit request; compat host tests link
 * without it. */
extern void ssrnx_jni_request_process_exit(const char *reason) __attribute__((weak));

/* Log the AArch64 frame-record chain (x29 -> {saved x29, x30}) of the current
 * guest thread, bounded to its registered stack. */
static unsigned compat_ssrnx_log_guest_backtrace(const char *what, uintptr_t fp) {
    uintptr_t low = 0, high = 0;
    if (!ssrnx_bionic_current_stack_bounds(&low, &high)) return 0;
    unsigned logged = 0;
    for (unsigned depth = 0; depth < 16u; ++depth) {
        if (fp < low || fp > high - 2u * sizeof(uintptr_t) || (fp & 15u) != 0) break;
        const uintptr_t *record = (const uintptr_t *)fp;
        const uintptr_t next = record[0], ret = record[1];
        if (!ret) break;
        char site[192];
        (void)ssrnx_compat_describe_guest_address(ret, site, sizeof(site));
        SSRNX_LOGCE("BIONIC", "%s backtrace #%u %s", what, depth, site);
        logged++;
        if (next <= fp) break;
        fp = next;
    }
    return logged;
}

/* Optimized AArch64 code may omit frame records, which can truncate x29-chain
 * diagnostics exactly where a fatal runtime path matters most. When that
 * happens, conservatively scan a bounded slice of the registered guest stack
 * for values that resolve inside loaded guest ELF images. These are labelled
 * candidates rather than asserted call frames; they improve post-mortem
 * evidence without changing guest state or unwinding through host frames. */
static void compat_ssrnx_log_guest_stack_candidates(const char *what, uintptr_t start) {
    uintptr_t low = 0, high = 0;
    if (!ssrnx_bionic_current_stack_bounds(&low, &high)) return;
    if (start < low) start = low;
    if (start >= high) return;
    start = (start + sizeof(uintptr_t) - 1u) & ~(uintptr_t)(sizeof(uintptr_t) - 1u);
    uintptr_t end = start + 32768u;
    if (end < start || end > high) end = high;
    uintptr_t seen[12] = {0};
    unsigned count = 0;
    for (uintptr_t at = start; at <= end - sizeof(uintptr_t) && count < 12u; at += sizeof(uintptr_t)) {
        const uintptr_t candidate = *(const uintptr_t *)at;
        if (!candidate) continue;
        bool duplicate = false;
        for (unsigned i = 0; i < count; ++i) if (seen[i] == candidate) { duplicate = true; break; }
        if (duplicate) continue;
        char site[192];
        if (!ssrnx_compat_describe_guest_address(candidate, site, sizeof(site))) continue;
        seen[count] = candidate;
        SSRNX_LOGCE("BIONIC", "%s stack-candidate #%u slot=+0x%" PRIxPTR " %s",
                    what, count, at - start, site);
        count++;
    }
}

/* Fatal-path diagnostic only: dump a small executable window around a guest
 * program counter.  This deliberately does not attempt to disassemble on the
 * Switch; preserving the exact instruction bytes lets the host-side analysis
 * identify the real stripped call site on the next run. */
static void compat_ssrnx_log_guest_code_window(const char *what, uintptr_t pc) {
    if (!compat_g_active_linker || !pc) return;
    const size_t module_count = ssrnx_linker_module_count(compat_g_active_linker);
    for (size_t mi = 0; mi < module_count; ++mi) {
        const SsrnxModule *module = ssrnx_linker_module_at(compat_g_active_linker, mi);
        if (!module || !module->mapped || !module->ehdr || !module->phdrs) continue;
        if (!ssrnx_module_runtime_contains(module, pc, 1)) continue;

        uintptr_t seg_low = 0, seg_high = 0;
        for (uint16_t pi = 0; pi < module->ehdr->e_phnum; ++pi) {
            const Elf64_Phdr *ph = &module->phdrs[pi];
            if (ph->p_type != PT_LOAD || ph->p_memsz == 0) continue;
            const uintptr_t low = module->load_bias + (uintptr_t)ph->p_vaddr;
            const uintptr_t high = low + (uintptr_t)ph->p_memsz;
            if (pc >= low && pc < high) { seg_low = low; seg_high = high; break; }
        }
        if (!seg_low || seg_high <= seg_low) return;

        uintptr_t begin = pc > 48u ? pc - 48u : seg_low;
        if (begin < seg_low) begin = seg_low;
        begin &= ~(uintptr_t)3u;
        uintptr_t end = pc + 48u;
        if (end < pc || end > seg_high) end = seg_high;
        end &= ~(uintptr_t)3u;
        if (end <= begin) return;

        char site[192];
        (void)ssrnx_compat_describe_guest_address(pc, site, sizeof(site));
        SSRNX_LOGCE("BIONIC", "%s code-window pc=%s range=0x%" PRIxPTR "..0x%" PRIxPTR,
                    what, site, begin, end);
        for (uintptr_t at = begin; at < end; at += 16u) {
            char hex[16u * 3u + 1u];
            size_t pos = 0;
            const size_t n = (size_t)((end - at) < 16u ? (end - at) : 16u);
            const unsigned char *bytes = (const unsigned char *)at;
            for (size_t i = 0; i < n && pos + 3u < sizeof(hex); ++i) {
                const int wrote = snprintf(hex + pos, sizeof(hex) - pos, "%02x%s",
                                           bytes[i], i + 1u == n ? "" : " ");
                if (wrote <= 0) break;
                pos += (size_t)wrote;
            }
            hex[pos < sizeof(hex) ? pos : sizeof(hex) - 1u] = '\0';
            SSRNX_LOGCE("BIONIC", "%s code offset=%+" PRIdPTR " @0x%" PRIxPTR ": %s",
                        what, (intptr_t)(at - pc), at, hex);
        }
        return;
    }
}

#define SSRNX_ANDROID_SIGABRT 6 /* Linux arm64 */

/* Bionic abort(3): unblock and raise SIGABRT so a handler the app installed
 * (crash reporters) runs, then the process dies without returning. Handing
 * the host's abort() to the guest would instead tear down the whole runtime
 * from a guest thread while the main loop still runs. After a returning handler
 * we must terminate atomically: the main thread can itself be aborting. */
__attribute__((noreturn, noinline)) static void compat_ssrnx_bionic_abort(void) {
    const uintptr_t caller = (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0));
    char site[192];
    (void)ssrnx_compat_describe_guest_address(caller, site, sizeof(site));
    SSRNX_LOGCE("BIONIC", "abort() caller=%s message=%s", site,
                compat_g_abort_message[0] ? compat_g_abort_message : "-");
    {
        char thread_name[16] = {0};
        uintptr_t stack_low = 0, stack_high = 0;
        const int name_rc = ssrnx_bionic_thread_get_current_name(thread_name);
        const bool have_stack = ssrnx_bionic_current_stack_bounds(&stack_low, &stack_high);
        SSRNX_LOGCE("BIONIC",
                    "abort thread tid=%d guestPthread=0x%" PRIx64 " name=%s tp=%p stack=%s0x%" PRIxPTR "..0x%" PRIxPTR,
                    ssrnx_bionic_gettid(), (uint64_t)ssrnx_bionic_pthread_self(),
                    name_rc == 0 && thread_name[0] ? thread_name : "-",
                    ssrnx_bionic_tls_current_base(), have_stack ? "" : "unknown:",
                    have_stack ? stack_low : 0, have_stack ? stack_high : 0);
    }
    compat_ssrnx_guest_stderr_dump_tail();
    const uintptr_t frame = (uintptr_t)__builtin_frame_address(0);
    const unsigned frames = compat_ssrnx_log_guest_backtrace("abort", frame);
    if (frames < 4u) compat_ssrnx_log_guest_stack_candidates("abort", frame);
    compat_ssrnx_log_guest_code_window("abort", caller);
    const uint64_t sigabrt = UINT64_C(1) << (SSRNX_ANDROID_SIGABRT - 1);
    SSRNX_LOGCE("BIONIC", "abort delivering SIGABRT");
    (void)ssrnx_bionic_pthread_sigmask(1 /* SIG_UNBLOCK */, &sigabrt, NULL);
    (void)ssrnx_bionic_raise(SSRNX_ANDROID_SIGABRT);
    SSRNX_LOGCE("BIONIC", "abort SIGABRT handler returned; terminating process atomically");

    char reason[600];
    snprintf(reason, sizeof(reason), "abort(): %s", compat_g_abort_message[0] ? compat_g_abort_message : site);
    if (ssrnx_jni_request_process_exit) ssrnx_jni_request_process_exit(reason);
    else ssrnx_bionic_set_process_terminating(true);
    ssrnx_diag_flush();
#ifdef __SWITCH__
    svcExitProcess();
#else
    _Exit(128 + SSRNX_ANDROID_SIGABRT);
#endif
    __builtin_unreachable();
}
static void compat_ssrnx_bionic_openlog(const char *ident, int option, int facility) {
    (void)option; (void)facility;
    SSRNX_LOGCI("SYSLOG", "openlog ident=%s", ident ? ident : "");
}
static void compat_ssrnx_bionic_syslog(int priority, const char *fmt, ...) {
    (void)priority;
    char msg[768];
    va_list ap; va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt ? fmt : "", ap);
    va_end(ap);
    SSRNX_LOGCI("SYSLOG", "%s", msg);
}
static void compat_ssrnx_bionic_closelog(void) {}
static void compat_ssrnx_bionic_assert2(const char *file, int line, const char *function, const char *expr) {
    SSRNX_LOGCE("ASSERT", "%s:%d %s: assertion '%s' failed",
                file ? file : "?", line, function ? function : "?", expr ? expr : "?");
    abort();
}

#define SSRNX_MEMTRACE_LARGE_THRESHOLD (1024u * 1024u)
static uint64_t compat_g_memory_failure_events;
#if defined(__GNUC__) || defined(__clang__)
#define SSRNX_MEMTRACE_CALLER() ((uintptr_t)__builtin_return_address(0))
#else
#define SSRNX_MEMTRACE_CALLER() ((uintptr_t)0)
#endif

static void compat_ssrnx_memory_failure(const char *op, size_t size, uintptr_t caller, int error) {
    const int saved = errno;
    const uint64_t sequence = __atomic_add_fetch(&compat_g_memory_failure_events, 1, __ATOMIC_RELAXED);
    if (sequence <= 32) {
        SSRNX_LOGCW("MEMORY", "boundary failure sequence=%" PRIu64 " op=%s size=%zu errno=%d caller=%p",
                    sequence, op, size, error, (void *)caller);
        compat_ssrnx_memtrace_flush();
    }
    errno = saved;
}

void ssrnx_compat_log_memory_state(const char *reason) {
    const int saved = errno;
    uint64_t total = 0, available = 0;
    compat_ssrnx_process_memory(&total, &available);
#ifdef __SWITCH__
    /* Available includes the unused reserved heap. Arena/free blocks describe
     * allocator fragmentation separately; none is a largest-allocation promise. */
    const struct mallinfo heap = mallinfo();
    SSRNX_LOGCI("MEMORY", "snapshot reason=%s processBudgetBytes=%" PRIu64
                " processAvailableBytes=%" PRIu64 " heapArenaBytes=%zu heapUsedBytes=%zu"
                " heapFreeBytes=%zu heapTopFreeBytes=%zu boundaryFailures=%" PRIu64,
                reason ? reason : "-", total, available, heap.arena,
                heap.uordblks, heap.fordblks, heap.keepcost,
                __atomic_load_n(&compat_g_memory_failure_events, __ATOMIC_RELAXED));
#else
    SSRNX_LOGCI("MEMORY", "snapshot reason=%s processBudgetBytes=%" PRIu64
                " processAvailableBytes=%" PRIu64 " boundaryFailures=%" PRIu64,
                reason ? reason : "-", total, available,
                __atomic_load_n(&compat_g_memory_failure_events, __ATOMIC_RELAXED));
#endif
    errno = saved;
}

static void compat_ssrnx_memtrace_large_result(const char *op, size_t size, void *result,
                                        uintptr_t caller, int saved_errno) {
    if (!compat_ssrnx_memtrace_enabled() || size < SSRNX_MEMTRACE_LARGE_THRESHOLD) return;
    SSRNX_LOGCI("MEMTRACE",
                "%s size=%zu result=%p errno=%d caller=%p tid=%d pthread=%p tp=%p",
                op ? op : "alloc", size, result, saved_errno,
                (void *)caller, ssrnx_bionic_gettid(),
                (void *)(uintptr_t)pthread_self(), ssrnx_bionic_tls_current_base());
    compat_ssrnx_memtrace_flush();
}

static void *compat_ssrnx_bionic_malloc(size_t size) {
    const uintptr_t caller = SSRNX_MEMTRACE_CALLER();
    void *p = malloc(size);
    const int saved = errno;
    if (!p && size) compat_ssrnx_memory_failure("malloc", size, caller, saved);
    compat_ssrnx_memtrace_large_result("malloc", size, p, caller, saved);
    errno = saved;
    return p;
}

static void *compat_ssrnx_bionic_calloc(size_t count, size_t size) {
    const uintptr_t caller = SSRNX_MEMTRACE_CALLER();
    size_t total = 0;
    const bool overflow = size != 0 && count > SIZE_MAX / size;
    if (!overflow) total = count * size;
    void *p = calloc(count, size);
    const int saved = errno;
    if (!p && count && size) compat_ssrnx_memory_failure("calloc", overflow ? SIZE_MAX : total, caller, saved);
    if (compat_ssrnx_memtrace_enabled() && (overflow || total >= SSRNX_MEMTRACE_LARGE_THRESHOLD)) {
        SSRNX_LOGCI("MEMTRACE",
                    "calloc count=%zu size=%zu total=%s%zu result=%p errno=%d caller=%p tid=%d pthread=%p tp=%p",
                    count, size, overflow ? "overflow:" : "", total, p, saved,
                    (void *)caller, ssrnx_bionic_gettid(),
                    (void *)(uintptr_t)pthread_self(), ssrnx_bionic_tls_current_base());
        compat_ssrnx_memtrace_flush();
    }
    errno = saved;
    return p;
}

static void *compat_ssrnx_bionic_realloc(void *old_ptr, size_t size) {
    const uintptr_t caller = SSRNX_MEMTRACE_CALLER();
    const bool trace = compat_ssrnx_memtrace_enabled() && size >= SSRNX_MEMTRACE_LARGE_THRESHOLD;
    if (trace) {
        SSRNX_LOGCI("MEMTRACE",
                    "realloc BEGIN old=%p size=%zu caller=%p tid=%d pthread=%p tp=%p",
                    old_ptr, size, (void *)caller, ssrnx_bionic_gettid(),
                    (void *)(uintptr_t)pthread_self(), ssrnx_bionic_tls_current_base());
        compat_ssrnx_memtrace_flush();
    }
    void *p = realloc(old_ptr, size);
    const int saved = errno;
    if (!p && size) compat_ssrnx_memory_failure("realloc", size, caller, saved);
    if (trace) {
        SSRNX_LOGCI("MEMTRACE",
                    "realloc END size=%zu result=%p errno=%d caller=%p tid=%d pthread=%p tp=%p",
                    size, p, saved, (void *)caller, ssrnx_bionic_gettid(),
                    (void *)(uintptr_t)pthread_self(), ssrnx_bionic_tls_current_base());
        compat_ssrnx_memtrace_flush();
    }
    errno = saved;
    return p;
}

static int compat_ssrnx_bionic_posix_memalign(void **out, size_t alignment, size_t size) {
    if (!out) return EINVAL;
    if (alignment < sizeof(void *) || (alignment & (alignment - 1u)) != 0 ||
        (alignment % sizeof(void *)) != 0) return EINVAL;

    /* devkitA64/newlib does not export posix_memalign(), but it does provide
     * memalign() and memory returned by memalign() is released by ordinary
     * free(). Keep Android/POSIX semantics at the guest ABI boundary without
     * importing a host symbol that does not exist on Switch. */
    const uintptr_t caller = SSRNX_MEMTRACE_CALLER();
    void *p = memalign(alignment, size ? size : 1u);
    const int saved = errno;
    if (!p) {
        compat_ssrnx_memory_failure("posix_memalign", size, caller, ENOMEM);
        compat_ssrnx_memtrace_large_result("posix_memalign", size, NULL, caller, saved ? saved : ENOMEM);
        return ENOMEM;
    }
    *out = p;
    if (compat_ssrnx_memtrace_enabled() && size >= SSRNX_MEMTRACE_LARGE_THRESHOLD) {
        SSRNX_LOGCI("MEMTRACE",
                    "posix_memalign align=%zu size=%zu result=%p errno=%d caller=%p tid=%d pthread=%p tp=%p",
                    alignment, size, p, saved, (void *)caller, ssrnx_bionic_gettid(),
                    (void *)(uintptr_t)pthread_self(), ssrnx_bionic_tls_current_base());
        compat_ssrnx_memtrace_flush();
    }
    return 0;
}

static void *compat_ssrnx_bionic_memalign(size_t alignment, size_t size) {
    const uintptr_t caller = SSRNX_MEMTRACE_CALLER();
    void *out = NULL;
    const int rc = compat_ssrnx_bionic_posix_memalign(&out, alignment, size);
    if (rc != 0) {
        errno = rc;
        if (compat_ssrnx_memtrace_enabled() && size >= SSRNX_MEMTRACE_LARGE_THRESHOLD) {
            SSRNX_LOGCI("MEMTRACE",
                        "memalign align=%zu size=%zu result=(nil) errno=%d caller=%p tid=%d pthread=%p tp=%p",
                        alignment, size, rc, (void *)caller, ssrnx_bionic_gettid(),
                        (void *)(uintptr_t)pthread_self(), ssrnx_bionic_tls_current_base());
            compat_ssrnx_memtrace_flush();
        }
        return NULL;
    }
    if (compat_ssrnx_memtrace_enabled() && size >= SSRNX_MEMTRACE_LARGE_THRESHOLD) {
        SSRNX_LOGCI("MEMTRACE",
                    "memalign align=%zu size=%zu result=%p errno=0 caller=%p tid=%d pthread=%p tp=%p",
                    alignment, size, out, (void *)caller, ssrnx_bionic_gettid(),
                    (void *)(uintptr_t)pthread_self(), ssrnx_bionic_tls_current_base());
        compat_ssrnx_memtrace_flush();
    }
    return out;
}


/* libc++ creates C/POSIX locale objects even when an application never changes
 * locale.  Android's locale_t is opaque to the guest, so SSRNX can keep a tiny
 * process-local C-locale token and implement the _l family against it. */
typedef struct compat_SsrnxLocaleToken { uint32_t magic; } compat_SsrnxLocaleToken;
static compat_SsrnxLocaleToken compat_g_c_locale = { 0x4c364143u };
static __thread void *compat_g_thread_locale = &compat_g_c_locale;

static void *compat_ssrnx_bionic_newlocale(int mask, const char *name, void *base) {
    (void)mask; (void)base;
    if (!name || !*name || strcmp(name, "C") == 0 || strcmp(name, "POSIX") == 0 ||
        strcmp(name, "C.UTF-8") == 0 || strcmp(name, "C.UTF8") == 0) return &compat_g_c_locale;
    errno = ENOENT;
    return NULL;
}
static void *compat_ssrnx_bionic_uselocale(void *locale) {
    void *old = compat_g_thread_locale;
    if (locale) compat_g_thread_locale = locale;
    return old;
}
static void compat_ssrnx_bionic_freelocale(void *locale) { (void)locale; }
static char *compat_ssrnx_bionic_setlocale(int category, const char *locale) {
    (void)category;
    static char c_locale[] = "C.UTF-8";
    if (!locale) return c_locale;
    if (!*locale || strcmp(locale,"C")==0 || strcmp(locale,"POSIX")==0 ||
        strcmp(locale,"C.UTF-8")==0 || strcmp(locale,"C.UTF8")==0) return c_locale;
    return NULL;
}
static int compat_ssrnx_bionic_isxdigit_l(int c, void *loc) { (void)loc; return isxdigit((unsigned char)c); }
static int compat_ssrnx_bionic_isdigit_l(int c, void *loc) { (void)loc; return isdigit((unsigned char)c); }
static int compat_ssrnx_bionic_islower_l(int c, void *loc) { (void)loc; return islower((unsigned char)c); }
static int compat_ssrnx_bionic_isupper_l(int c, void *loc) { (void)loc; return isupper((unsigned char)c); }
static int compat_ssrnx_bionic_toupper_l(int c, void *loc) { (void)loc; return toupper((unsigned char)c); }
static int compat_ssrnx_bionic_tolower_l(int c, void *loc) { (void)loc; return tolower((unsigned char)c); }
static int compat_ssrnx_bionic_iswlower_l(wint_t c, void *loc) { (void)loc; return iswlower(c); }
static int compat_ssrnx_bionic_iswspace_l(wint_t c, void *loc) { (void)loc; return iswspace(c); }
static int compat_ssrnx_bionic_iswprint_l(wint_t c, void *loc) { (void)loc; return iswprint(c); }
static int compat_ssrnx_bionic_iswcntrl_l(wint_t c, void *loc) { (void)loc; return iswcntrl(c); }
static int compat_ssrnx_bionic_iswupper_l(wint_t c, void *loc) { (void)loc; return iswupper(c); }
static int compat_ssrnx_bionic_iswalpha_l(wint_t c, void *loc) { (void)loc; return iswalpha(c); }
static int compat_ssrnx_bionic_iswdigit_l(wint_t c, void *loc) { (void)loc; return iswdigit(c); }
static int compat_ssrnx_bionic_iswpunct_l(wint_t c, void *loc) { (void)loc; return iswpunct(c); }
static int compat_ssrnx_bionic_iswxdigit_l(wint_t c, void *loc) { (void)loc; return iswxdigit(c); }
static int compat_ssrnx_bionic_iswblank_l(wint_t c, void *loc) { (void)loc; return c == L' ' || c == L'\t'; }
static wint_t compat_ssrnx_bionic_towupper_l(wint_t c, void *loc) { (void)loc; return towupper(c); }
static wint_t compat_ssrnx_bionic_towlower_l(wint_t c, void *loc) { (void)loc; return towlower(c); }
static int compat_ssrnx_bionic_strcoll_l(const char *a, const char *b, void *loc) { (void)loc; return strcmp(a,b); }
static size_t compat_ssrnx_bionic_strxfrm_l(char *dst, const char *src, size_t n, void *loc) {
    (void)loc; size_t len = strlen(src);
    if (dst && n) { size_t c = len < n-1u ? len : n-1u; memcpy(dst,src,c); dst[c]=0; }
    return len;
}
static int compat_ssrnx_bionic_wcscoll_l(const wchar_t *a, const wchar_t *b, void *loc) { (void)loc; return wcscmp(a,b); }
static size_t compat_ssrnx_bionic_wcsxfrm_l(wchar_t *dst, const wchar_t *src, size_t n, void *loc) {
    (void)loc; size_t len = wcslen(src);
    if (dst && n) { size_t c = len < n-1u ? len : n-1u; wmemcpy(dst,src,c); dst[c]=0; }
    return len;
}
static long long compat_ssrnx_bionic_strtoll_l(const char *s, char **e, int b, void *loc) { (void)loc; return strtoll(s,e,b); }
static unsigned long long compat_ssrnx_bionic_strtoull_l(const char *s, char **e, int b, void *loc) { (void)loc; return strtoull(s,e,b); }
static long double compat_ssrnx_bionic_strtold_l(const char *s, char **e, void *loc) { (void)loc; return strtold(s,e); }

static size_t compat_ssrnx_bionic_strftime_l(char *dst, size_t cap, const char *fmt, const compat_SsrnxAndroidTm *gtm, void *loc) {
    (void)loc;
    if (!gtm) { errno = EINVAL; return 0; }
    struct tm tmv;
    memset(&tmv,0,sizeof(tmv));
    tmv.tm_sec=gtm->tm_sec; tmv.tm_min=gtm->tm_min; tmv.tm_hour=gtm->tm_hour;
    tmv.tm_mday=gtm->tm_mday; tmv.tm_mon=gtm->tm_mon; tmv.tm_year=gtm->tm_year;
    tmv.tm_wday=gtm->tm_wday; tmv.tm_yday=gtm->tm_yday; tmv.tm_isdst=gtm->tm_isdst;
    return strftime(dst,cap,fmt,&tmv);
}

static void compat_ssrnx_android_tm_to_host(struct tm *out, const compat_SsrnxAndroidTm *gtm) {
    memset(out, 0, sizeof(*out));
    if (!gtm) return;
    out->tm_sec=gtm->tm_sec; out->tm_min=gtm->tm_min; out->tm_hour=gtm->tm_hour;
    out->tm_mday=gtm->tm_mday; out->tm_mon=gtm->tm_mon; out->tm_year=gtm->tm_year;
    out->tm_wday=gtm->tm_wday; out->tm_yday=gtm->tm_yday; out->tm_isdst=gtm->tm_isdst;
}
static size_t compat_ssrnx_bionic_strftime(char *dst, size_t cap, const char *fmt, const compat_SsrnxAndroidTm *gtm) {
    if (!gtm) { errno = EINVAL; return 0; }
    struct tm tmv;
    compat_ssrnx_android_tm_to_host(&tmv, gtm);
    return strftime(dst, cap, fmt, &tmv);
}
static size_t compat_ssrnx_bionic_wcsftime(wchar_t *dst, size_t cap, const wchar_t *fmt, const compat_SsrnxAndroidTm *gtm) {
    if (!dst || !fmt || !gtm || cap == 0) { errno = EINVAL; return 0; }
    char nfmt[256];
    size_t fi=0;
    for (; fmt[fi] && fi + 1u < sizeof(nfmt); ++fi) {
        if ((uint32_t)fmt[fi] > 0x7fu) { errno = EILSEQ; return 0; }
        nfmt[fi]=(char)fmt[fi];
    }
    if (fmt[fi]) { errno = E2BIG; return 0; }
    nfmt[fi]='\0';

    char tmp[1024];
    struct tm tmv;
    compat_ssrnx_android_tm_to_host(&tmv, gtm);
    size_t n=strftime(tmp,sizeof(tmp),nfmt,&tmv);
    if (n==0 || n>=cap) return 0;
    for (size_t i=0;i<=n;++i) dst[i]=(wchar_t)(unsigned char)tmp[i];
    return n;
}
static int compat_ssrnx_bionic_strcoll(const char *a, const char *b) { return strcmp(a,b); }
static size_t compat_ssrnx_bionic_strxfrm(char *dst, const char *src, size_t n) {
    size_t len=strlen(src);
    if (dst && n) { size_t c=len<n-1u?len:n-1u; memcpy(dst,src,c); dst[c]='\0'; }
    return len;
}
static int compat_ssrnx_bionic_wcscoll(const wchar_t *a, const wchar_t *b) { return wcscmp(a,b); }
static size_t compat_ssrnx_bionic_wcsxfrm(wchar_t *dst, const wchar_t *src, size_t n) {
    size_t len=wcslen(src);
    if (dst && n) { size_t c=len<n-1u?len:n-1u; wmemcpy(dst,src,c); dst[c]=0; }
    return len;
}

enum {
    SSRNX_WCTYPE_INVALID=0, SSRNX_WCTYPE_ALNUM=1, SSRNX_WCTYPE_ALPHA=2,
    SSRNX_WCTYPE_BLANK=3, SSRNX_WCTYPE_CNTRL=4, SSRNX_WCTYPE_DIGIT=5,
    SSRNX_WCTYPE_GRAPH=6, SSRNX_WCTYPE_LOWER=7, SSRNX_WCTYPE_PRINT=8,
    SSRNX_WCTYPE_PUNCT=9, SSRNX_WCTYPE_SPACE=10, SSRNX_WCTYPE_UPPER=11,
    SSRNX_WCTYPE_XDIGIT=12
};
static long compat_ssrnx_bionic_wctype(const char *name) {
    if (!name) return SSRNX_WCTYPE_INVALID;
    static const char *const names[]={"","alnum","alpha","blank","cntrl","digit","graph","lower","print","punct","space","upper","xdigit"};
    for (long i=1;i<=SSRNX_WCTYPE_XDIGIT;++i) if (strcmp(name,names[i])==0) return i;
    return SSRNX_WCTYPE_INVALID;
}
static int compat_ssrnx_bionic_iswctype(wint_t wc, long type) {
    switch (type) {
        case SSRNX_WCTYPE_ALNUM: return iswalnum(wc);
        case SSRNX_WCTYPE_ALPHA: return iswalpha(wc);
        case SSRNX_WCTYPE_BLANK: return wc==L' ' || wc==L'\t';
        case SSRNX_WCTYPE_CNTRL: return iswcntrl(wc);
        case SSRNX_WCTYPE_DIGIT: return iswdigit(wc);
        case SSRNX_WCTYPE_GRAPH: return iswgraph(wc);
        case SSRNX_WCTYPE_LOWER: return iswlower(wc);
        case SSRNX_WCTYPE_PRINT: return iswprint(wc);
        case SSRNX_WCTYPE_PUNCT: return iswpunct(wc);
        case SSRNX_WCTYPE_SPACE: return iswspace(wc);
        case SSRNX_WCTYPE_UPPER: return iswupper(wc);
        case SSRNX_WCTYPE_XDIGIT: return iswxdigit(wc);
        default: return 0;
    }
}

/* Stateless UTF-8 conversions are sufficient for libc++'s C.UTF-8 facet
 * setup and normal game strings.  Android mbstate_t storage is accepted but
 * intentionally not inspected, avoiding a host/newlib mbstate_t ABI leak. */
static size_t compat_ssrnx_utf8_decode(wchar_t *out, const char *s, size_t n) {
    if (!s || n == 0) return (size_t)-2;
    const unsigned char *p=(const unsigned char*)s;
    uint32_t cp; size_t need;
    if (p[0] < 0x80) { cp=p[0]; need=1; }
    else if ((p[0]&0xe0)==0xc0) { cp=p[0]&0x1f; need=2; }
    else if ((p[0]&0xf0)==0xe0) { cp=p[0]&0x0f; need=3; }
    else if ((p[0]&0xf8)==0xf0) { cp=p[0]&0x07; need=4; }
    else { errno=EILSEQ; return (size_t)-1; }
    if (n < need) return (size_t)-2;
    for(size_t i=1;i<need;i++){ if((p[i]&0xc0)!=0x80){errno=EILSEQ;return(size_t)-1;} cp=(cp<<6)|(p[i]&0x3f); }
    if ((need==2 && cp<0x80) || (need==3 && cp<0x800) || (need==4 && cp<0x10000) ||
        cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff)) { errno=EILSEQ; return (size_t)-1; }
    if(out)*out=(wchar_t)cp;
    return cp==0 ? 0 : need;
}
static size_t compat_ssrnx_utf8_encode(char *out, wchar_t wc) {
    uint32_t cp=(uint32_t)wc;
    if(cp<=0x7f){if(out)out[0]=(char)cp;return 1;}
    if(cp<=0x7ff){if(out){out[0]=(char)(0xc0|(cp>>6));out[1]=(char)(0x80|(cp&0x3f));}return 2;}
    if(cp>=0xd800&&cp<=0xdfff){errno=EILSEQ;return(size_t)-1;}
    if(cp<=0xffff){if(out){out[0]=(char)(0xe0|(cp>>12));out[1]=(char)(0x80|((cp>>6)&0x3f));out[2]=(char)(0x80|(cp&0x3f));}return 3;}
    if(cp<=0x10ffff){if(out){out[0]=(char)(0xf0|(cp>>18));out[1]=(char)(0x80|((cp>>12)&0x3f));out[2]=(char)(0x80|((cp>>6)&0x3f));out[3]=(char)(0x80|(cp&0x3f));}return 4;}
    errno=EILSEQ;return(size_t)-1;
}
static size_t compat_ssrnx_bionic_mbrtowc(wchar_t *out, const char *s, size_t n, void *state) {
    (void)state; if(!s)return 0; return compat_ssrnx_utf8_decode(out,s,n);
}
static int compat_ssrnx_bionic_mbtowc(wchar_t *out, const char *s, size_t n) {
    if (!s) return 0;
    size_t r = compat_ssrnx_utf8_decode(out, s, n);
    return r == (size_t)-1 || r == (size_t)-2 ? -1 : (int)r;
}
static size_t compat_ssrnx_bionic_mbrlen(const char *s, size_t n, void *state) {
    return compat_ssrnx_bionic_mbrtowc(NULL,s,n,state);
}
static size_t compat_ssrnx_bionic_wcrtomb(char *out, wchar_t wc, void *state) {
    (void)state; if(!out)return 1; return compat_ssrnx_utf8_encode(out,wc);
}
static wint_t compat_ssrnx_bionic_btowc(int c) { return (c==EOF || (unsigned)c>0x7fU) ? WEOF : (wint_t)(unsigned char)c; }
static int compat_ssrnx_bionic_wctob(wint_t c) { return (c<=0x7fU) ? (int)c : EOF; }
static size_t compat_ssrnx_bionic_mbsnrtowcs(wchar_t *dst,const char **src,size_t nms,size_t len,void *state) {
    (void)state; if(!src||!*src){errno=EINVAL;return(size_t)-1;} const char *p=*src;size_t used=0,outn=0;
    while(used<nms){wchar_t wc=0;size_t r=compat_ssrnx_utf8_decode(&wc,p+used,nms-used);if(r==(size_t)-1)return(size_t)-1;if(r==(size_t)-2)break;
        size_t step=r?r:1;if(wc==0){if(dst&&outn<len)dst[outn]=0;*src=NULL;return outn;}if(dst){if(outn>=len)break;dst[outn]=wc;}outn++;used+=step;}
    *src=p+used;return outn;
}
static size_t compat_ssrnx_bionic_mbsrtowcs(wchar_t *dst,const char **src,size_t len,void *state) {
    return compat_ssrnx_bionic_mbsnrtowcs(dst,src,SIZE_MAX,len,state);
}
static size_t compat_ssrnx_bionic_wcsnrtombs(char *dst,const wchar_t **src,size_t nwc,size_t len,void *state) {
    (void)state; if(!src||!*src){errno=EINVAL;return(size_t)-1;} const wchar_t *p=*src;size_t i=0,outn=0;
    while(i<nwc){wchar_t wc=p[i];if(wc==0){if(dst&&outn<len)dst[outn]=0;*src=NULL;return outn;}char enc[4];size_t r=compat_ssrnx_utf8_encode(enc,wc);if(r==(size_t)-1)return(size_t)-1;
        if(dst){if(outn+r>len)break;memcpy(dst+outn,enc,r);}outn+=r;i++;}
    *src=p+i;return outn;
}
static size_t compat_ssrnx_bionic_ctype_mb_cur_max(void) { return 4; }

/* sysconf(3) with Bionic's _SC_* numbering (NDK <bits/sysconf.h>); the guest
 * passes Android constants, which differ from newlib's and glibc's. Limits
 * follow Bionic/Linux defaults; processor and memory values come from the
 * same process core mask and memory limits exported through /proc and /sys.
 * Unknown names fail with EINVAL as POSIX requires. */
static long compat_ssrnx_bionic_sysconf(int name) {
    enum { POSIX_2008 = 200809L };
    switch (name) {
        case 0x00: return 2097152;                    /* _SC_ARG_MAX */
        case 0x01: case 0x02: case 0x04: return 99;  /* _SC_BC_{BASE,DIM,STRING}_MAX */
        case 0x03: return 99;                         /* _SC_BC_SCALE_MAX */
        case 0x05: return 1024;                       /* _SC_CHILD_MAX */
        case 0x06: return 100;                        /* _SC_CLK_TCK */
        case 0x07: return 2;                          /* _SC_COLL_WEIGHTS_MAX */
        case 0x08: return 32;                         /* _SC_EXPR_NEST_MAX */
        case 0x09: return 2048;                       /* _SC_LINE_MAX */
        case 0x0a: return 65536;                      /* _SC_NGROUPS_MAX */
        case 0x0b: return 1024;                       /* _SC_OPEN_MAX */
        case 0x0c: return 128;                        /* _SC_PASS_MAX */
        case 0x0d: case 0x0e: case 0x14: case 0x16: case 0x19: return POSIX_2008;
        case 0x0f: case 0x10: case 0x11: case 0x12: case 0x13: case 0x15: return -1;
        case 0x17: case 0x18: return 1;               /* _SC_JOB_CONTROL, _SC_SAVED_IDS */
        case 0x1a: return 255;                        /* _SC_RE_DUP_MAX */
        case 0x1b: return 20;                         /* _SC_STREAM_MAX (FOPEN_MAX) */
        case 0x1c: return 6;                          /* _SC_TZNAME_MAX */
        case 0x1d: case 0x1f: case 0x22: case 0x23: case 0x24: case 0x29: return -1;
        case 0x1e: return 1;                          /* _SC_XOPEN_ENH_I18N */
        case 0x20: return 700;                        /* _SC_XOPEN_VERSION */
        case 0x21: return 4;                          /* _SC_XOPEN_XCU_VERSION */
        case 0x25: return LONG_MAX;                   /* _SC_ATEXIT_MAX */
        case 0x26: return 1024;                       /* _SC_IOV_MAX */
        case 0x27: case 0x28: return 4096;            /* _SC_PAGESIZE, _SC_PAGE_SIZE */
        case 0x2a: case 0x2b: return -1;              /* _SC_XBS5_ILP32_* */
        case 0x2c: return 1;                          /* _SC_XBS5_LP64_OFF64 */
        case 0x2d: return -1;                         /* _SC_XBS5_LPBIG_OFFBIG */
        case 0x2e: case 0x2f: case 0x30: return -1;   /* _SC_AIO_* */
        case 0x31: return INT_MAX;                    /* _SC_DELAYTIMER_MAX */
        case 0x32: return -1;                         /* _SC_MQ_OPEN_MAX */
        case 0x33: return 32768;                      /* _SC_MQ_PRIO_MAX */
        case 0x34: return 33;                         /* _SC_RTSIG_MAX */
        case 0x35: return 256;                        /* _SC_SEM_NSEMS_MAX */
        case 0x36: return 0x3fffffffL;                /* _SC_SEM_VALUE_MAX */
        case 0x37: return 32;                         /* _SC_SIGQUEUE_MAX */
        case 0x38: return -1;                         /* _SC_TIMER_MAX */
        case 0x39: case 0x3c: case 0x3d: case 0x3f: case 0x40: case 0x44: case 0x45: return -1;
        case 0x3a: case 0x3b: case 0x3e: case 0x41: case 0x42: case 0x43: case 0x46:
            return POSIX_2008;
        case 0x47: case 0x48: return 1024;            /* _SC_GET{GR,PW}_R_SIZE_MAX */
        case 0x49: return 256;                        /* _SC_LOGIN_NAME_MAX */
        case 0x4a: return 4;                          /* _SC_THREAD_DESTRUCTOR_ITERATIONS */
        case 0x4b: return 128;                        /* _SC_THREAD_KEYS_MAX */
        case 0x4c: return 16384;                      /* _SC_THREAD_STACK_MIN */
        case 0x4d: return -1;                         /* _SC_THREAD_THREADS_MAX */
        case 0x4e: return 32;                         /* _SC_TTY_NAME_MAX */
        case 0x4f: case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55:
            return POSIX_2008;                        /* _SC_THREADS.._SC_THREAD_SAFE_FUNCTIONS */
        case 0x60: case 0x61: return (long)compat_ssrnx_process_cpu_count(); /* _SC_NPROCESSORS_* */
        case 0x62: case 0x63: {                       /* _SC_PHYS_PAGES, _SC_AVPHYS_PAGES */
            uint64_t total = 0, available = 0;
            compat_ssrnx_process_memory(&total, &available);
            return (long)((name == 0x62 ? total : available) / 4096u);
        }
        case 0x64: case 0x6c: case 0x6d: case 0x6e: case 0x70: case 0x71: case 0x72: case 0x73:
        case 0x74: case 0x76: case 0x7a: case 0x7b: case 0x7f:
            return POSIX_2008;
        case 0x6f: return 255;                        /* _SC_HOST_NAME_MAX */
        case 0x79: return 40;                         /* _SC_SYMLOOP_MAX */
        case 0x8b: return 1;                          /* _SC_V7_LP64_OFF64 */
        case 0x91: case 0x94: case 0x97: return 64;   /* L1I/L1D/L2 line size */
        case 0x9e: return 65;                         /* _SC_NSIG */
        default:
            /* Remaining valid names are unsupported options or unknown
             * cache geometry: -1 or 0 without errno, as Bionic reports. */
            if (name >= 0x65 && name <= 0x9d) return name >= 0x8f ? 0 : -1;
            errno = EINVAL;
            return -1;
    }
}
static int compat_ssrnx_bionic_nanosleep(const compat_SsrnxAndroidTimespec *req, compat_SsrnxAndroidTimespec *rem) {
    if(!req){errno=EFAULT;return -1;} struct timespec hreq={(time_t)req->tv_sec,(long)req->tv_nsec},hrem;
    int rc=compat_ssrnx_safe_nanosleep(&hreq,rem?&hrem:NULL); if(rem){rem->tv_sec=(int64_t)hrem.tv_sec;rem->tv_nsec=(int64_t)hrem.tv_nsec;} return rc;
}
static int compat_ssrnx_bionic_ioctl(int fd, unsigned long request, ...) {
    enum { SSRNX_LINUX_FIONREAD = 0x541B, SSRNX_LINUX_FIONBIO = 0x5421 };
    va_list ap;
    va_start(ap, request);
    void *arg = va_arg(ap, void *);
    va_end(ap);
    if (!arg) { errno = EFAULT; return -1; }

    if (ssrnx_fd_is_virtual(fd)) {
        if (request == SSRNX_LINUX_FIONBIO) return 0;
        if (request == SSRNX_LINUX_FIONREAD) { *(int *)arg = 1; return 0; }
        errno = ENOTTY;
        return -1;
    }
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    unsigned long host_request;
    if (request == SSRNX_LINUX_FIONBIO) host_request = FIONBIO;
    else if (request == SSRNX_LINUX_FIONREAD) host_request = FIONREAD;
    else { errno = ENOTTY; return -1; }
    return compat_ssrnx_network_result_int(ioctl(fd, host_request, arg));
}

static void compat_ssrnx_fill_android_stat(compat_SsrnxAndroidStat *out,const struct stat *st) {
    memset(out,0,sizeof(*out));out->st_dev=(uint64_t)st->st_dev;out->st_ino=(uint64_t)st->st_ino;
    out->st_mode=(uint32_t)st->st_mode;out->st_nlink=(uint32_t)st->st_nlink;out->st_uid=(uint32_t)st->st_uid;out->st_gid=(uint32_t)st->st_gid;
    out->st_rdev=(uint64_t)st->st_rdev;out->st_size=(int64_t)st->st_size;out->st_blksize=4096;out->st_blocks=(out->st_size+511)/512;
    out->st_atim.tv_sec=(int64_t)st->st_atime;out->st_mtim.tv_sec=(int64_t)st->st_mtime;out->st_ctim.tv_sec=(int64_t)st->st_ctime;
}
static int compat_ssrnx_bionic_fstat(int fd, compat_SsrnxAndroidStat *out) {
    if(!out){errno=EFAULT;return -1;}
    if (ssrnx_fd_is_virtual(fd)) {
        struct stat st; memset(&st, 0, sizeof(st));
        if (ssrnx_eventfd_stat && ssrnx_eventfd_stat(fd, &st) == 0) {
            compat_ssrnx_fill_android_stat(out, &st); return 0;
        }
        st.st_mode = ssrnx_fd_is_random(fd) ? (S_IFCHR | 0444) : (S_IFIFO | 0600);
        st.st_nlink = 1; st.st_blksize = 4096;
        compat_ssrnx_fill_android_stat(out, &st);
        return 0;
    }
    struct stat st;
    const bool locked = compat_ssrnx_regular_file_io_lock(fd);
    const int rc = fstat(fd, &st);
    const int saved = errno;
    compat_ssrnx_regular_file_io_unlock(locked);
    char host_path[SSRNX_STORAGE_PATH_CAP];
    const bool tracked_path = rc == 0 && ssrnx_file_contract_host_path &&
                              ssrnx_file_contract_host_path(fd, host_path, sizeof(host_path));
    if (tracked_path) compat_ssrnx_kernel_view_stat(host_path, &st);
    if (ssrnx_file_contract_fstat)
        ssrnx_file_contract_fstat(fd, rc, rc == 0 ? (int64_t)st.st_size : -1,
                                  rc == 0 ? (uint32_t)st.st_mode : 0u,
                                  rc == 0 ? 0 : saved);
    errno = saved;
    if (rc != 0) return -1;
    compat_ssrnx_fill_android_stat(out, &st);
    if (out->st_ino == 0 && tracked_path)
        out->st_ino = compat_ssrnx_host_path_inode(host_path);
    return 0;
}
static int compat_ssrnx_bionic_lstat(const char *path, compat_SsrnxAndroidStat *out) { return compat_ssrnx_bionic_stat(path,out); }

typedef struct compat_SsrnxAndroidDirent {
    uint64_t d_ino; int64_t d_off; uint16_t d_reclen; uint8_t d_type; char d_name[256];
} compat_SsrnxAndroidDirent;
static __thread compat_SsrnxAndroidDirent compat_g_android_dirent;
static compat_SsrnxAndroidDirent *compat_ssrnx_bionic_readdir(DIR *dir) {
    struct dirent *e=readdir(dir);if(!e)return NULL;memset(&compat_g_android_dirent,0,sizeof(compat_g_android_dirent));
    compat_g_android_dirent.d_ino=(uint64_t)e->d_ino;compat_g_android_dirent.d_reclen=(uint16_t)sizeof(compat_g_android_dirent);
#ifdef _DIRENT_HAVE_D_TYPE
    compat_g_android_dirent.d_type=(uint8_t)e->d_type;
#endif
    ssrnx_host_name_decode(e->d_name,compat_g_android_dirent.d_name,sizeof(compat_g_android_dirent.d_name));return &compat_g_android_dirent;
}
static int compat_ssrnx_bionic_readdir_r(DIR *dir, compat_SsrnxAndroidDirent *entry, compat_SsrnxAndroidDirent **result) {
    if (!dir || !entry || !result) return EINVAL;
    errno = 0;
    compat_SsrnxAndroidDirent *next = compat_ssrnx_bionic_readdir(dir);
    if (!next) {
        const int saved = errno;
        *result = NULL;
        return saved;
    }
    *entry = *next;
    *result = entry;
    return 0;
}
static DIR *compat_ssrnx_bionic_fdopendir(int fd) { (void)fd; errno=ENOSYS; return NULL; }

/* Bionic realpath(): resolve against the guest cwd, collapse "."/".." and
 * require the result to exist. SSRNX guest storage has no symlinks to expand.
 * Host-native "sdmc:" paths from runtime-internal callers pass through. */
static char *compat_ssrnx_bionic_realpath(const char *path,char *resolved) {
    if(!path){errno=EINVAL;return NULL;}
    if(!path[0]){errno=ENOENT;return NULL;}
    char absolute[SSRNX_STORAGE_PATH_CAP];
    if(strncmp(path,"sdmc:",5)==0){
        if(snprintf(absolute,sizeof(absolute),"%s",path)>=(int)sizeof(absolute)){errno=ENAMETOOLONG;return NULL;}
    } else if(!compat_ssrnx_guest_absolute_path(path,absolute,sizeof(absolute))){errno=ENAMETOOLONG;return NULL;}
    char mapped[SSRNX_STORAGE_PATH_CAP];struct stat st;
    if(compat_ssrnx_storage_stat(compat_ssrnx_map_guest_path(absolute,mapped,sizeof(mapped),NULL),&st)!=0)return NULL;
    const size_t need=strlen(absolute)+1u;
    if(resolved && need>4096u){errno=ENAMETOOLONG;return NULL;} /* caller buffer is PATH_MAX */
    char *out=resolved?resolved:(char*)malloc(need);if(!out){errno=ENOMEM;return NULL;}
    memcpy(out,absolute,need);return out;
}
static int compat_ssrnx_bionic_symlink(const char *target,const char *linkpath){(void)target;(void)linkpath;errno=ENOTSUP;return -1;}
static int compat_ssrnx_bionic_link(const char *oldpath,const char *newpath){(void)oldpath;(void)newpath;errno=ENOTSUP;return -1;}
static int compat_ssrnx_bionic_readlink(const char *path,char *buf,size_t cap){(void)path;(void)buf;(void)cap;errno=EINVAL;return -1;}
static int compat_ssrnx_bionic_chmod(const char *path, unsigned mode) {
    if (!path) { errno = EFAULT; return -1; }
    if (compat_ssrnx_guest_path_is_readonly_system(path)) { errno = EROFS; return -1; }
    char mapped[SSRNX_STORAGE_PATH_CAP];
    const char *host = compat_ssrnx_map_guest_path(path, mapped, sizeof(mapped), NULL);
    const int rc = chmod(host, (mode_t)mode);
    if (compat_ssrnx_storage_host_path(host)) {
        const int saved = errno;
        compat_ssrnx_storage_log("chmod", path, host, rc);
        errno = saved;
    }
    return rc;
}
static int compat_ssrnx_bionic_fchmod(int fd, unsigned mode){
    if (ssrnx_fd_is_virtual(fd)) { errno = EINVAL; return -1; }
    return fchmod(fd, (mode_t)mode);
}
static int compat_ssrnx_bionic_fchmodat(int dirfd,const char *path,unsigned mode,int flags){
    if(!path){errno=EFAULT;return -1;}
    if(flags!=0){errno=ENOTSUP;return -1;}
    if(path[0]!='/' && dirfd!=-100){errno=ENOTSUP;return -1;}
    return compat_ssrnx_bionic_chmod(path,mode);
}
enum {
    SSRNX_ANDROID_AT_FDCWD = -100,
    SSRNX_ANDROID_AT_SYMLINK_NOFOLLOW = 0x100,
    SSRNX_ANDROID_UTIME_NOW = 1073741823,
    SSRNX_ANDROID_UTIME_OMIT = 1073741822
};

static int compat_ssrnx_bionic_utimensat(int dirfd, const char *path,
                                   const compat_SsrnxAndroidTimespec times[2], int flags) {
    if (dirfd != SSRNX_ANDROID_AT_FDCWD) { errno = ENOTSUP; return -1; }
    if (!path) { errno = EFAULT; return -1; }
    if (flags & ~SSRNX_ANDROID_AT_SYMLINK_NOFOLLOW) { errno = EINVAL; return -1; }
    if (compat_ssrnx_guest_path_is_readonly_system(path)) { errno = EROFS; return -1; }

    char mapped[SSRNX_STORAGE_PATH_CAP];
    const char *host_path = compat_ssrnx_map_guest_path(path, mapped, sizeof(mapped), NULL);

    /* UTIME_OMIT for both fields is an explicit no-op, but pathname lookup
     * still has to succeed. This is the only timestamp operation Horizon's
     * mounted filesystem can represent without lying about persistence. */
    if (times && times[0].tv_nsec == SSRNX_ANDROID_UTIME_OMIT &&
                 times[1].tv_nsec == SSRNX_ANDROID_UTIME_OMIT) {
        struct stat st;
        return compat_ssrnx_storage_stat(host_path, &st);
    }

#ifdef __SWITCH__
    /* fsdev/libnx exposes timestamp queries but no general POSIX timestamp
     * mutation primitive. Never report success for an update we did not do. */
    (void)times;
    errno = ENOTSUP;
    return -1;
#else
    struct timespec host_times[2];
    struct timespec *host_times_ptr = NULL;
    if (times) {
        for (int i = 0; i < 2; ++i) {
            if (times[i].tv_nsec < 0 ||
                (times[i].tv_nsec >= 1000000000LL &&
                 times[i].tv_nsec != SSRNX_ANDROID_UTIME_NOW &&
                 times[i].tv_nsec != SSRNX_ANDROID_UTIME_OMIT)) {
                errno = EINVAL;
                return -1;
            }
            host_times[i].tv_sec = (time_t)times[i].tv_sec;
            if (times[i].tv_nsec == SSRNX_ANDROID_UTIME_NOW) host_times[i].tv_nsec = UTIME_NOW;
            else if (times[i].tv_nsec == SSRNX_ANDROID_UTIME_OMIT) host_times[i].tv_nsec = UTIME_OMIT;
            else host_times[i].tv_nsec = (long)times[i].tv_nsec;
        }
        host_times_ptr = host_times;
    }
    int host_flags = 0;
#ifdef AT_SYMLINK_NOFOLLOW
    if (flags & SSRNX_ANDROID_AT_SYMLINK_NOFOLLOW) host_flags |= AT_SYMLINK_NOFOLLOW;
#else
    if (flags & SSRNX_ANDROID_AT_SYMLINK_NOFOLLOW) { errno = ENOTSUP; return -1; }
#endif
    return utimensat(AT_FDCWD, host_path, host_times_ptr, host_flags);
#endif
}
static int compat_ssrnx_bionic_openat(int dirfd,const char *path,int flags,...){
    if (!path) { errno = EFAULT; return -1; }
    /* Linux ignores dirfd for absolute paths, including invalid descriptors. */
    if (path[0] != '/' && dirfd != -100) { errno = ENOSYS; return -1; }
    if(flags&SSRNX_ANDROID_O_CREAT){va_list ap;va_start(ap,flags);int mode=va_arg(ap,int);va_end(ap);return compat_ssrnx_bionic_open(path,flags,mode);}return compat_ssrnx_bionic_open(path,flags);
}
static int compat_ssrnx_bionic_unlinkat(int dirfd,const char *path,int flags){if(dirfd!=-100){errno=ENOSYS;return -1;}if(!path){errno=EFAULT;return -1;}if(compat_ssrnx_guest_path_is_readonly_system(path)){errno=EROFS;return -1;}char mapped[SSRNX_STORAGE_PATH_CAP];const char *host=compat_ssrnx_map_guest_path(path,mapped,sizeof(mapped),NULL);int rc=(flags&0x200)?rmdir(host):compat_ssrnx_bionic_unlink(path);if((flags&0x200)&&compat_ssrnx_storage_host_path(host)){__atomic_fetch_add(&compat_g_storage_mutations,1u,__ATOMIC_RELAXED);const int saved=errno;compat_ssrnx_storage_log("rmdir",path,host,rc);errno=saved;}return rc;}
static int compat_ssrnx_bionic_ftruncate(int fd, int64_t length) {
    if (ssrnx_fd_is_virtual(fd)) { errno = EINVAL; return -1; }
    if (length < 0) { errno = EINVAL; return -1; }
    const off_t host_length = (off_t)length;
    if ((int64_t)host_length != length) { errno = EFBIG; return -1; }
    const bool locked = compat_ssrnx_regular_file_io_lock(fd);
    const int rc = ftruncate(fd, host_length);
    const int saved = errno;
    compat_ssrnx_regular_file_io_unlock(locked);
    errno = saved;
    if (ssrnx_file_contract_ftruncate) ssrnx_file_contract_ftruncate(fd, length, rc, rc == 0 ? 0 : saved);
    errno = saved;
    return rc;
}
static int compat_ssrnx_bionic_truncate(const char *path, int64_t length) {
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    if (!path) { errno = EFAULT; return -1; }
    if (length < 0) { errno = EINVAL; return -1; }
    const off_t host_length = (off_t)length;
    if ((int64_t)host_length != length) { errno = EFBIG; return -1; }
    if (compat_ssrnx_guest_path_is_readonly_system(path)) { errno = EROFS; return -1; }
    char mapped[SSRNX_STORAGE_PATH_CAP];
    const char *host = compat_ssrnx_map_guest_path(path, mapped, sizeof(mapped), NULL);
    int fd = open(host, O_WRONLY);
    int saved = errno, rc = -1;
    const char *step = "open";
    unsigned host_result = 0;
#ifdef __SWITCH__
    if (fd < 0) host_result = fsdevGetLastResult();
    /* Horizon rejects a second handle while a file is open for writing. POSIX
     * truncate(path) must still resize that file. Use the tracked writer only
     * for this fsdev conflict, pinning its lifetime inside the registry. */
    if (fd < 0 && saved == EIO && compat_ssrnx_storage_host_path(host) &&
        ssrnx_file_contract_truncate_open) {
        rc = ssrnx_file_contract_truncate_open(host, length);
        if (rc != -2) { saved = errno; step = "tracked-writer"; }
        else { rc = -1; errno = saved; }
    }
#endif
    if (fd >= 0) {
        step = "resize";
        rc = compat_ssrnx_bionic_ftruncate(fd, length);
        saved = errno;
#ifdef __SWITCH__
        if (rc < 0) host_result = fsdevGetLastResult();
#endif
        (void)close(fd);
    }
    if (rc == 0 && compat_ssrnx_storage_host_path(host))
        __atomic_fetch_add(&compat_g_storage_mutations, 1u, __ATOMIC_RELAXED);
    /* Failures must survive the startup storage trace budget. Preserve the
     * host result before any logger/observer can perform another FS operation. */
    static uint32_t trace_count;
    if (rc < 0 || __atomic_fetch_add(&trace_count, 1u, __ATOMIC_RELAXED) < 16u)
        SSRNX_LOGCI("FILE", "truncate guest=%s host=%s requested=%lld step=%s result=%d errno=%d hostResult=%#x",
                    path, host, (long long)length, step, rc, rc == 0 ? 0 : saved, host_result);
    errno = saved;
    return rc;
}
static int compat_ssrnx_bionic_rmdir(const char *path){if(!path){errno=EFAULT;return -1;}if(compat_ssrnx_guest_path_is_readonly_system(path)){errno=EROFS;return -1;}char mapped[SSRNX_STORAGE_PATH_CAP];const char *host=compat_ssrnx_map_guest_path(path,mapped,sizeof(mapped),NULL);int rc=rmdir(host);int saved=errno;if(compat_ssrnx_storage_host_path(host)){if(rc==0)__atomic_fetch_add(&compat_g_storage_mutations,1u,__ATOMIC_RELAXED);compat_ssrnx_storage_log("rmdir",path,host,rc);}errno=saved;return rc;}
static ssize_t compat_ssrnx_bionic_sendfile(int outfd,int infd,int64_t *offset,size_t count){
    int64_t saved=-1;if(offset){saved=compat_ssrnx_bionic_lseek(infd,0,SEEK_CUR);if(compat_ssrnx_bionic_lseek(infd,*offset,SEEK_SET)<0)return -1;}
    char buf[16384];size_t done=0;while(done<count){size_t want=count-done;if(want>sizeof(buf))want=sizeof(buf);ssize_t r=compat_ssrnx_bionic_read(infd,buf,want);if(r<=0)break;ssize_t w=compat_ssrnx_bionic_write(outfd,buf,(size_t)r);if(w<0){done=(size_t)-1;break;}done+=(size_t)w;if(w<r)break;}
    if(offset){int64_t now=compat_ssrnx_bionic_lseek(infd,0,SEEK_CUR);if(now>=0)*offset=now;if(saved>=0)(void)compat_ssrnx_bionic_lseek(infd,saved,SEEK_SET);}return done==(size_t)-1?-1:(ssize_t)done;
}
static long compat_ssrnx_bionic_pathconf(const char *path,int name){(void)name;if(!path){errno=EFAULT;return -1;}char mapped[SSRNX_STORAGE_PATH_CAP];struct stat st;if(stat(compat_ssrnx_map_guest_path(path,mapped,sizeof(mapped),NULL),&st)!=0)return -1;return 255;}

/* Android/Linux flock(2): whole-file advisory locks owned by an open file
 * description. Linux arbitrates them between descriptors of one process too:
 * a second open() of a file is an independent description whose requests
 * conflict with the first (runtimes rely on this for cross-thread cache and
 * database locks, retrying LOCK_NB until granted). Horizon/fsdev has no lock
 * primitive and SSRNX is the only process, so the table lives here, keyed by
 * the host file. Locks follow the descriptor (path-backed dup() reopens the
 * file, so a duplicate holds no lock of its own) and are dropped on close. */
enum { SSRNX_FLOCK_SH = 1, SSRNX_FLOCK_EX = 2, SSRNX_FLOCK_NB = 4, SSRNX_FLOCK_UN = 8 };
#define SSRNX_FLOCK_MAX 128u
typedef struct compat_SsrnxFlockRecord {
    char *key;   /* NULL: free slot */
    int fd;
    int mode;    /* SSRNX_FLOCK_SH or SSRNX_FLOCK_EX */
} compat_SsrnxFlockRecord;
static pthread_mutex_t compat_g_flock_lock = PTHREAD_MUTEX_INITIALIZER;
static compat_SsrnxFlockRecord compat_g_flock_records[SSRNX_FLOCK_MAX];
static unsigned compat_g_flock_held; /* records in use; lets close() skip the table */
static unsigned compat_g_flock_trace_count;

static int compat_ssrnx_flock_find_locked(int fd) {
    for (unsigned i = 0; i < SSRNX_FLOCK_MAX; ++i)
        if (compat_g_flock_records[i].key && compat_g_flock_records[i].fd == fd) return (int)i;
    return -1;
}

static bool compat_ssrnx_flock_conflicts_locked(const char *key, int fd, int mode) {
    for (unsigned i = 0; i < SSRNX_FLOCK_MAX; ++i) {
        const compat_SsrnxFlockRecord *r = &compat_g_flock_records[i];
        if (!r->key || r->fd == fd || strcmp(r->key, key) != 0) continue;
        if (mode == SSRNX_FLOCK_EX || r->mode == SSRNX_FLOCK_EX) return true;
    }
    return false;
}

static void compat_ssrnx_flock_drop_locked(int index) {
    free(compat_g_flock_records[index].key);
    compat_g_flock_records[index].key = NULL;
    __atomic_sub_fetch(&compat_g_flock_held, 1u, __ATOMIC_RELEASE);
}

/* close(2) releases the descriptor's lock. */
static void compat_ssrnx_flock_release_fd(int fd) {
    if (!__atomic_load_n(&compat_g_flock_held, __ATOMIC_ACQUIRE)) return;
    pthread_mutex_lock(&compat_g_flock_lock);
    const int index = compat_ssrnx_flock_find_locked(fd);
    if (index >= 0) compat_ssrnx_flock_drop_locked(index);
    pthread_mutex_unlock(&compat_g_flock_lock);
}

/* Identity of the file behind fd: its inode when the backend reports one,
 * otherwise (Horizon FAT/exFAT) the host path of tracked app/storage files.
 * A descriptor with neither (sockets, pipes, untracked inode-less files)
 * locks privately: Linux grants those requests too, and no other descriptor
 * can name the same file through the table. */
static int compat_ssrnx_flock_key(int fd, char *out, size_t cap) {
    if (ssrnx_fd_is_virtual(fd)) return snprintf(out, cap, "v:%d", fd) < (int)cap ? 0 : ENAMETOOLONG;
    struct stat st;
    const bool have_stat = fstat(fd, &st) == 0;
    if (!have_stat && fcntl(fd, F_GETFL, 0) < 0) return EBADF;
    if (have_stat && st.st_ino)
        return snprintf(out, cap, "i:%llu:%llu", (unsigned long long)st.st_dev,
                        (unsigned long long)st.st_ino) < (int)cap ? 0 : ENAMETOOLONG;
    char host[SSRNX_STORAGE_PATH_CAP];
    if (ssrnx_file_contract_host_path && ssrnx_file_contract_host_path(fd, host, sizeof(host)))
        return snprintf(out, cap, "p:%s", host) < (int)cap ? 0 : ENAMETOOLONG;
    return snprintf(out, cap, "d:%d", fd) < (int)cap ? 0 : ENAMETOOLONG;
}

static int compat_ssrnx_bionic_flock(int fd, int operation) {
    if (operation & ~(SSRNX_FLOCK_SH | SSRNX_FLOCK_EX | SSRNX_FLOCK_NB | SSRNX_FLOCK_UN)) { errno = EINVAL; return -1; }
    const int mode = operation & (SSRNX_FLOCK_SH | SSRNX_FLOCK_EX | SSRNX_FLOCK_UN);
    if (mode != SSRNX_FLOCK_SH && mode != SSRNX_FLOCK_EX && mode != SSRNX_FLOCK_UN) { errno = EINVAL; return -1; }
    if (fd < 0) { errno = EBADF; return -1; }
    char key[SSRNX_STORAGE_PATH_CAP + 32];
    const int key_error = compat_ssrnx_flock_key(fd, key, sizeof(key));
    if (key_error) { errno = key_error; return -1; }
    if (mode == SSRNX_FLOCK_UN) {
        compat_ssrnx_flock_release_fd(fd);
        return 0;
    }

    bool waited = false;
    for (;;) {
        pthread_mutex_lock(&compat_g_flock_lock);
        /* A held lock is kept while a conversion waits; Linux may drop it
         * first, which callers cannot rely on either way. */
        if (!compat_ssrnx_flock_conflicts_locked(key, fd, mode)) {
            int index = compat_ssrnx_flock_find_locked(fd);
            if (index >= 0 && strcmp(compat_g_flock_records[index].key, key) != 0) {
                compat_ssrnx_flock_drop_locked(index); /* fd number reused without close() */
                index = -1;
            }
            if (index < 0) {
                for (unsigned i = 0; i < SSRNX_FLOCK_MAX && index < 0; ++i)
                    if (!compat_g_flock_records[i].key) index = (int)i;
                char *copy = index >= 0 ? strdup(key) : NULL;
                if (!copy) {
                    pthread_mutex_unlock(&compat_g_flock_lock);
                    errno = ENOLCK;
                    return -1;
                }
                compat_g_flock_records[index].key = copy;
                compat_g_flock_records[index].fd = fd;
                __atomic_add_fetch(&compat_g_flock_held, 1u, __ATOMIC_RELEASE);
            }
            compat_g_flock_records[index].mode = mode;
            pthread_mutex_unlock(&compat_g_flock_lock);
            if (__atomic_fetch_add(&compat_g_flock_trace_count, 1u, __ATOMIC_RELAXED) < 16u)
                SSRNX_LOGCI("FILE", "flock fd=%d op=%#x key=%s result=0 waited=%d", fd, operation, key, waited ? 1 : 0);
            return 0;
        }
        pthread_mutex_unlock(&compat_g_flock_lock);
        if (operation & SSRNX_FLOCK_NB) { errno = EWOULDBLOCK; return -1; }
        /* Blocking request: poll at a signal-safe point until granted. */
        waited = true;
        struct timespec nap = { 0, 2 * 1000 * 1000 };
        (void)compat_ssrnx_safe_nanosleep(&nap, NULL);
    }
}

static int compat_ssrnx_bionic_statfs_host(const char *host_path, compat_SsrnxAndroidStatfs *out) {
    const uint64_t modeled_type = compat_ssrnx_kernel_view_fs_type(host_path);
    if (modeled_type) {
        memset(out, 0, sizeof(*out));
        out->f_type = modeled_type;
        out->f_bsize = out->f_frsize = 4096u;
        out->f_namelen = 255u;
        /* Read-only modeled proc/sys namespace: RDONLY/NOSUID/NODEV/NOEXEC
         * plus Linux's ST_VALID marker. No host SD capacity leaks through. */
        out->f_flags = 0x2fu;
        return 0;
    }
    struct statvfs host;
    if (statvfs(host_path, &host) != 0) return -1;
    memset(out, 0, sizeof(*out));
    /* statvfs does not expose a filesystem magic. f_type=0 means SSRNX does
     * not invent an Android filesystem identity while all capacity/name/flag
     * values remain host-backed. */
    out->f_bsize = (uint64_t)host.f_bsize;
    out->f_blocks = (uint64_t)host.f_blocks;
    out->f_bfree = (uint64_t)host.f_bfree;
    out->f_bavail = (uint64_t)host.f_bavail;
    out->f_files = (uint64_t)host.f_files;
    out->f_ffree = (uint64_t)host.f_ffree;
    out->f_namelen = (uint64_t)host.f_namemax;
    out->f_frsize = (uint64_t)(host.f_frsize ? host.f_frsize : host.f_bsize);
    out->f_flags = (uint64_t)host.f_flag | 0x20u; /* Linux ST_VALID */
    return 0;
}

static int compat_ssrnx_bionic_statfs(const char *path, compat_SsrnxAndroidStatfs *out) {
    if (!compat_ssrnx_process_vm_range_valid(out, sizeof(*out), true)) { errno = EFAULT; return -1; }
    if (!compat_ssrnx_syscall_path_valid(path)) return -1;
    char mapped[SSRNX_STORAGE_PATH_CAP];
    const char *host_path = compat_ssrnx_map_guest_path(path, mapped, sizeof(mapped), NULL);
    struct stat st;
    if (compat_ssrnx_storage_stat(host_path, &st) != 0) return -1;
    return compat_ssrnx_bionic_statfs_host(host_path, out);
}

static int compat_ssrnx_bionic_fstatfs(int fd, compat_SsrnxAndroidStatfs *out) {
    if (!compat_ssrnx_process_vm_range_valid(out, sizeof(*out), true)) { errno = EFAULT; return -1; }
    if (ssrnx_fd_is_virtual(fd)) { errno = ENOSYS; return -1; }
    struct stat st;
    if (fstat(fd, &st) != 0) return -1; /* Validate the open descriptor first. */
    char host[SSRNX_STORAGE_PATH_CAP];
    if (!ssrnx_file_contract_host_path || !ssrnx_file_contract_host_path(fd, host, sizeof(host))) {
        errno = ENOSYS; return -1;
    }
    /* Newlib has statvfs(path), not fstatvfs(fd). A regular descriptor's
     * parent stays usable after unlink, and identifies its fsdev volume. */
    if (!compat_ssrnx_kernel_view_fs_type(host) && !S_ISDIR(st.st_mode)) {
        char *slash = strrchr(host, '/');
        if (!slash) { errno = ENOSYS; return -1; }
        if (slash == host) slash[1] = '\0'; else *slash = '\0';
    }
    return compat_ssrnx_bionic_statfs_host(host, out);
}

typedef struct compat_SsrnxAndroidUtimbuf { int64_t actime; int64_t modtime; } compat_SsrnxAndroidUtimbuf;
static int compat_ssrnx_bionic_utime(const char *path, const compat_SsrnxAndroidUtimbuf *times) {
    if (!times) return compat_ssrnx_bionic_utimensat(SSRNX_ANDROID_AT_FDCWD, path, NULL, 0);
    compat_SsrnxAndroidTimespec ts[2] = {
        { times->actime, 0 }, { times->modtime, 0 }
    };
    return compat_ssrnx_bionic_utimensat(SSRNX_ANDROID_AT_FDCWD, path, ts, 0);
}

/* utimes(2): microsecond timeval pair, validated as Linux does. */
static int compat_ssrnx_bionic_utimes(const char *path, const compat_SsrnxAndroidTimeval times[2]) {
    if (!times) return compat_ssrnx_bionic_utimensat(SSRNX_ANDROID_AT_FDCWD, path, NULL, 0);
    compat_SsrnxAndroidTimespec ts[2];
    for (int i = 0; i < 2; ++i) {
        if (times[i].tv_usec < 0 || times[i].tv_usec >= INT64_C(1000000)) { errno = EINVAL; return -1; }
        ts[i].tv_sec = times[i].tv_sec;
        ts[i].tv_nsec = times[i].tv_usec * INT64_C(1000);
    }
    return compat_ssrnx_bionic_utimensat(SSRNX_ANDROID_AT_FDCWD, path, ts, 0);
}

/* futimens(2) follows utimensat(): an all-UTIME_OMIT request only validates
 * the descriptor, and Switch reports ENOTSUP rather than claiming an update
 * its filesystem layer cannot perform. */
static int compat_ssrnx_bionic_futimens(int fd, const compat_SsrnxAndroidTimespec times[2]) {
    if (times) {
        for (int i = 0; i < 2; ++i) {
            if (times[i].tv_nsec != SSRNX_ANDROID_UTIME_NOW && times[i].tv_nsec != SSRNX_ANDROID_UTIME_OMIT &&
                (times[i].tv_nsec < 0 || times[i].tv_nsec >= INT64_C(1000000000))) {
                errno = EINVAL;
                return -1;
            }
        }
    }
    const bool omit_both = times && times[0].tv_nsec == SSRNX_ANDROID_UTIME_OMIT &&
                           times[1].tv_nsec == SSRNX_ANDROID_UTIME_OMIT;
    if (ssrnx_fd_is_virtual(fd)) {
        if (omit_both) return 0;
        errno = ENOTSUP;
        return -1;
    }
    if (omit_both) {
        struct stat st;
        return fstat(fd, &st);
    }
#ifdef __SWITCH__
    errno = ENOTSUP;
    return -1;
#else
    struct timespec host_times[2];
    if (times) {
        for (int i = 0; i < 2; ++i) {
            host_times[i].tv_sec = (time_t)times[i].tv_sec;
            if (times[i].tv_nsec == SSRNX_ANDROID_UTIME_NOW) host_times[i].tv_nsec = UTIME_NOW;
            else if (times[i].tv_nsec == SSRNX_ANDROID_UTIME_OMIT) host_times[i].tv_nsec = UTIME_OMIT;
            else host_times[i].tv_nsec = (long)times[i].tv_nsec;
        }
    }
    return futimens(fd, times ? host_times : NULL);
#endif
}

static int compat_g_prctl_dumpable;
static int compat_g_prctl_no_new_privs;
static int compat_ssrnx_bionic_prctl(int option, unsigned long arg2, unsigned long arg3,
                              unsigned long arg4, unsigned long arg5) {
    (void)arg4; (void)arg5;
    enum {
        A_PR_GET_DUMPABLE = 3, A_PR_SET_DUMPABLE = 4,
        A_PR_SET_NAME = 15, A_PR_GET_NAME = 16,
        A_PR_SET_NO_NEW_PRIVS = 38, A_PR_GET_NO_NEW_PRIVS = 39,
        A_PR_SET_VMA = 0x53564d41, A_PR_SET_VMA_ANON_NAME = 0
    };
    switch (option) {
        case A_PR_SET_NAME: {
            const char *input = (const char *)(uintptr_t)arg2;
            if (!input) { errno = EFAULT; return -1; }
            /* prctl truncates comm to 15 bytes; pthread_setname_np rejects
             * longer input instead. Pass a bounded copy to the shared model. */
            char name[16] = {0};
            memcpy(name, input, strnlen(input, sizeof(name) - 1u));
            return ssrnx_bionic_thread_set_current_name(name);
        }
        case A_PR_GET_NAME:
            return ssrnx_bionic_thread_get_current_name((char *)(uintptr_t)arg2);
        case A_PR_GET_DUMPABLE:
            return compat_g_prctl_dumpable;
        case A_PR_SET_DUMPABLE:
            if (arg2 > 1) { errno = EINVAL; return -1; }
            compat_g_prctl_dumpable = (int)arg2; return 0;
        case A_PR_GET_NO_NEW_PRIVS:
            return compat_g_prctl_no_new_privs;
        case A_PR_SET_NO_NEW_PRIVS:
            if (arg2 != 1 || arg3 != 0 || arg4 != 0 || arg5 != 0) { errno = EINVAL; return -1; }
            compat_g_prctl_no_new_privs = 1; return 0;
        case A_PR_SET_VMA:
            /* Android uses PR_SET_VMA/ANON_NAME only to annotate /proc maps.
             * SSRNX has no guest /proc maps surface; validate the subcommand
             * and treat the unobservable annotation as complete. */
            if (arg2 != A_PR_SET_VMA_ANON_NAME) { errno = EINVAL; return -1; }
            return 0;
        default:
            errno = EINVAL; return -1;
    }
}

/* Memory probes must return EFAULT rather than faulting in the host copy.
 * Query every Horizon region, including the iovec descriptors themselves. */
static bool compat_ssrnx_process_vm_range_valid(const void *ptr, size_t size, bool writing) {
    uintptr_t address = (uintptr_t)ptr;
    if (!size) return true;
    if (!address || size > UINTPTR_MAX - address) return false;
#ifdef __SWITCH__
    while (size) {
        MemoryInfo info;
        u32 page_info = 0;
        if (R_FAILED(svcQueryMemory(&info, &page_info, (u64)address)) ||
            (info.perm & (writing ? Perm_W : Perm_R)) == 0 || address < info.addr ||
            address - info.addr >= info.size) return false;
        const uint64_t available = info.size - (address - info.addr);
        const size_t chunk = available < size ? (size_t)available : size;
        address += chunk;
        size -= chunk;
    }
#else
    (void)writing;
#endif
    return true;
}

static ssize_t compat_ssrnx_bionic_process_vm_readv(int pid,
                                              const compat_SsrnxAndroidIovec *local_iov, unsigned long liovcnt,
                                              const compat_SsrnxAndroidIovec *remote_iov, unsigned long riovcnt,
                                              unsigned long flags) {
    if (flags != 0) { errno = EINVAL; return -1; }
    if (liovcnt > 1024ul || riovcnt > 1024ul) { errno = EINVAL; return -1; }
    if (!liovcnt || !riovcnt) return 0;
    if (!compat_ssrnx_process_vm_range_valid(local_iov, liovcnt * sizeof(*local_iov), false) ||
        !compat_ssrnx_process_vm_range_valid(remote_iov, riovcnt * sizeof(*remote_iov), false)) {
        errno = EFAULT; return -1;
    }
    size_t local_size = 0, remote_size = 0;
    for (unsigned long i = 0; i < liovcnt; ++i) {
        if (local_iov[i].iov_len > (SIZE_MAX >> 1) - local_size) { errno = EINVAL; return -1; }
        local_size += local_iov[i].iov_len;
    }
    for (unsigned long i = 0; i < riovcnt; ++i) {
        if (remote_iov[i].iov_len > (SIZE_MAX >> 1) - remote_size) { errno = EINVAL; return -1; }
        remote_size += remote_iov[i].iov_len;
    }
    if (!local_size || !remote_size) return 0;
    for (unsigned long i = 0; i < liovcnt; ++i) {
        if (!compat_ssrnx_process_vm_range_valid(local_iov[i].iov_base, local_iov[i].iov_len, true)) {
            errno = EFAULT; return -1;
        }
    }
    if (pid != (int)getpid()) { errno = ESRCH; return -1; }
    size_t li = 0, ri = 0, loff = 0, roff = 0, total = 0;
    while (li < liovcnt && ri < riovcnt) {
        const size_t lrem = local_iov[li].iov_len - loff;
        const size_t rrem = remote_iov[ri].iov_len - roff;
        size_t n = lrem < rrem ? lrem : rrem;
        if (n) {
            const uintptr_t local_base = (uintptr_t)local_iov[li].iov_base;
            const uintptr_t remote_base = (uintptr_t)remote_iov[ri].iov_base;
            /* Linux can return a partial read when a later remote page faults. */
            const size_t page_remaining = 4096u - ((remote_base + roff) & 4095u);
            if (n > page_remaining) n = page_remaining;
            if (loff > UINTPTR_MAX - local_base || roff > UINTPTR_MAX - remote_base ||
                !compat_ssrnx_process_vm_range_valid((void *)(local_base + loff), n, true) ||
                !compat_ssrnx_process_vm_range_valid((const void *)(remote_base + roff), n, false)) {
                if (total) return (ssize_t)total;
                errno = EFAULT; return -1;
            }
            memmove((unsigned char *)local_iov[li].iov_base + loff,
                    (const unsigned char *)remote_iov[ri].iov_base + roff, n);
            total += n; loff += n; roff += n;
        }
        if (loff == local_iov[li].iov_len) { ++li; loff = 0; }
        if (roff == remote_iov[ri].iov_len) { ++ri; roff = 0; }
    }
    return (ssize_t)total;
}

static long compat_ssrnx_bionic_ptrace(int request, int pid, void *addr, void *data) {
    (void)request; (void)pid; (void)addr; (void)data;
    /* SSRNX intentionally has no guest tracer/process-control facility. Keep
     * the syscall ABI present but fail visibly if a caller actually relies on it. */
    errno = ENOSYS;
    return -1;
}

typedef struct compat_SsrnxAndroidStatvfs {
    uint64_t f_bsize,f_frsize,f_blocks,f_bfree,f_bavail,f_files,f_ffree,f_favail,f_fsid,f_flag,f_namemax;
    uint32_t f_spare[6];
} compat_SsrnxAndroidStatvfs;
static int compat_ssrnx_bionic_statvfs(const char *path, compat_SsrnxAndroidStatvfs *out) {
    if (!path || !out) { errno = EFAULT; return -1; }
    char mapped[SSRNX_STORAGE_PATH_CAP];
    const char *host_path = compat_ssrnx_map_guest_path(path, mapped, sizeof(mapped), NULL);
    struct statvfs host;
    if (statvfs(host_path, &host) != 0) return -1;
    memset(out, 0, sizeof(*out));
    out->f_bsize = (uint64_t)host.f_bsize;
    out->f_frsize = (uint64_t)host.f_frsize;
    out->f_blocks = (uint64_t)host.f_blocks;
    out->f_bfree = (uint64_t)host.f_bfree;
    out->f_bavail = (uint64_t)host.f_bavail;
    out->f_files = (uint64_t)host.f_files;
    out->f_ffree = (uint64_t)host.f_ffree;
    out->f_favail = (uint64_t)host.f_favail;
    out->f_fsid = (uint64_t)host.f_fsid;
    out->f_flag = (uint64_t)host.f_flag;
    out->f_namemax = (uint64_t)host.f_namemax;
    return 0;
}

typedef struct compat_SsrnxBionicPropInfo {
    const char *name;
    const char *value;
} compat_SsrnxBionicPropInfo;

static const compat_SsrnxBionicPropInfo compat_g_bionic_properties[] = {
    {"ro.build.version.sdk", NULL},
    {"ro.build.version.release", "11"},
    {"ro.product.cpu.abi", NULL},
    {"ro.product.cpu.abilist", NULL},
    {"ro.product.model", "Nintendo Switch"},
    /* GLES 3.2 (0x00030002); ConfigurationInfo.reqGlEsVersion reports the same. */
    {"ro.opengles.version", "196610"},
};

static int compat_ssrnx_bionic_android_get_device_api_level(void) {
    return ssrnx_android_compat_platform_api();
}

static int compat_ssrnx_bionic_android_get_application_target_sdk_version(void) {
    return ssrnx_android_compat_profile_is_configured()
         ? ssrnx_android_compat_target_sdk()
         : SSRNX_ANDROID_API_FUTURE;
}

static const char *compat_ssrnx_system_property_value(const compat_SsrnxBionicPropInfo *info,
                                                char scratch[32]) {
    if (!info) return "";
    if (strcmp(info->name, "ro.build.version.sdk") == 0) {
        snprintf(scratch, 32, "%d", ssrnx_android_compat_platform_api());
        return scratch;
    }
    if (strcmp(info->name, "ro.product.cpu.abi") == 0 ||
        strcmp(info->name, "ro.product.cpu.abilist") == 0) {
        return ssrnx_android_compat_abi_name(ssrnx_android_compat_abi());
    }
    return info->value;
}

static const compat_SsrnxBionicPropInfo *compat_ssrnx_system_property_lookup(const char *key) {
    if (!key) return NULL;
    for (size_t i = 0; i < sizeof(compat_g_bionic_properties) / sizeof(compat_g_bionic_properties[0]); ++i) {
        if (strcmp(key, compat_g_bionic_properties[i].name) == 0) return &compat_g_bionic_properties[i];
    }
    return NULL;
}

static int compat_ssrnx_system_property_get(const char *key, char *value) {
    if (!value) return 0;
    const compat_SsrnxBionicPropInfo *info = compat_ssrnx_system_property_lookup(key);
    char scratch[32];
    const char *v = compat_ssrnx_system_property_value(info, scratch);
    size_t n = strlen(v);
    if (n > 91u) n = 91u;
    memcpy(value, v, n);
    value[n] = '\0';
    return (int)n;
}

static const compat_SsrnxBionicPropInfo *compat_ssrnx_system_property_find(const char *key) {
    return compat_ssrnx_system_property_lookup(key);
}

static int compat_ssrnx_system_property_read(const compat_SsrnxBionicPropInfo *info, char *name, char *value) {
    if (!info) return 0;
    if (name) {
        size_t n = strlen(info->name);
        if (n > 31u) n = 31u;
        memcpy(name, info->name, n);
        name[n] = '\0';
    }
    char scratch[32];
    const char *v = compat_ssrnx_system_property_value(info, scratch);
    if (!value) return (int)strlen(v);
    size_t n = strlen(v);
    if (n > 91u) n = 91u;
    memcpy(value, v, n);
    value[n] = '\0';
    return (int)n;
}


static uint32_t compat_ssrnx_system_property_serial(const compat_SsrnxBionicPropInfo *info) {
    if (!info) return 0;
    char scratch[32];
    size_t n = strlen(compat_ssrnx_system_property_value(info, scratch));
    if (n > 255u) n = 255u;
    /* Android serials encode the visible value length in the high byte. SSRNX
     * properties in this table are immutable for the process lifetime, so the
     * low 24-bit generation can remain a stable non-dirty value. */
    return ((uint32_t)n << 24) | 2u;
}

static uint32_t compat_ssrnx_system_property_area_serial(void) {
    return 1u; /* immutable virtual property area */
}

static const compat_SsrnxBionicPropInfo *compat_ssrnx_system_property_find_nth(unsigned n) {
    return n < (unsigned)(sizeof(compat_g_bionic_properties) / sizeof(compat_g_bionic_properties[0]))
        ? &compat_g_bionic_properties[n] : NULL;
}

static void compat_ssrnx_system_property_read_callback(
        const compat_SsrnxBionicPropInfo *info,
        void (*callback)(void *cookie, const char *name, const char *value, uint32_t serial),
        void *cookie) {
    if (!info || !callback) return;
    char scratch[32];
    callback(cookie, info->name, compat_ssrnx_system_property_value(info, scratch),
             compat_ssrnx_system_property_serial(info));
}

static int compat_ssrnx_system_property_foreach(
        void (*callback)(const compat_SsrnxBionicPropInfo *info, void *cookie), void *cookie) {
    if (!callback) return -1;
    for (size_t i = 0; i < sizeof(compat_g_bionic_properties) / sizeof(compat_g_bionic_properties[0]); ++i)
        callback(&compat_g_bionic_properties[i], cookie);
    return 0;
}

static int compat_ssrnx_bionic_getpagesize(void) {
    /* SSRNX's guest VM and mmap compatibility surface use Android's classic
     * 4 KiB userspace page contract on AArch64. */
    return 4096;
}

static size_t compat_ssrnx_bionic_strlcpy(char *dst, const char *src, size_t cap) {
    if (!src) return 0;
    const size_t n = strlen(src);
    if (cap && dst) {
        const size_t copy = n < cap - 1u ? n : cap - 1u;
        if (copy) memcpy(dst, src, copy);
        dst[copy] = '\0';
    }
    return n;
}

typedef struct compat_SsrnxAtforkEntry {
    void (*prepare)(void);
    void (*parent)(void);
    void (*child)(void);
    void *dso;
} compat_SsrnxAtforkEntry;

#define SSRNX_ATFORK_MAX 64u
static compat_SsrnxAtforkEntry compat_g_atfork_entries[SSRNX_ATFORK_MAX];
static size_t compat_g_atfork_count;
static pthread_mutex_t compat_g_atfork_lock = PTHREAD_MUTEX_INITIALIZER;

static int compat_ssrnx_bionic_register_atfork(void (*prepare)(void), void (*parent)(void),
                                        void (*child)(void), void *dso) {
    pthread_mutex_lock(&compat_g_atfork_lock);
    if (compat_g_atfork_count >= SSRNX_ATFORK_MAX) {
        pthread_mutex_unlock(&compat_g_atfork_lock);
        return ENOMEM;
    }
    compat_g_atfork_entries[compat_g_atfork_count++] = (compat_SsrnxAtforkEntry){prepare, parent, child, dso};
    pthread_mutex_unlock(&compat_g_atfork_lock);
    return 0;
}

static void compat_ssrnx_bionic_unregister_atfork(void *dso) {
    pthread_mutex_lock(&compat_g_atfork_lock);
    size_t out = 0;
    for (size_t i = 0; i < compat_g_atfork_count; ++i) {
        if (compat_g_atfork_entries[i].dso == dso) continue;
        if (out != i) compat_g_atfork_entries[out] = compat_g_atfork_entries[i];
        ++out;
    }
    compat_g_atfork_count = out;
    pthread_mutex_unlock(&compat_g_atfork_lock);
}
static unsigned char compat_g_aux_random[16];
static pthread_once_t compat_aux_random_once = PTHREAD_ONCE_INIT;
static void compat_init_aux_random(void) {
    ssrnx_fd_random_fill(compat_g_aux_random, sizeof(compat_g_aux_random));
}
static const char compat_g_aux_platform[]="aarch64";

/* AT_HWCAP for the Cortex-A57 cores (asm/hwcap.h): FP, ASIMD, AES, PMULL,
 * SHA1, SHA2, CRC32 -- the same set /proc/cpuinfo lists as Features. */
#define SSRNX_ARM64_HWCAP ((1u << 0) | (1u << 1) | (1u << 3) | (1u << 4) | (1u << 5) | (1u << 6) | (1u << 7))

unsigned long ssrnx_bionic_getauxval(unsigned long type){
    if (type == 25) pthread_once(&compat_aux_random_once, compat_init_aux_random);
    switch(type){case 6:return 4096;case 11:case 12:case 13:case 14:return 0;case 15:return(unsigned long)(uintptr_t)compat_g_aux_platform;
        case 16:return SSRNX_ARM64_HWCAP;case 26:return 0;case 23:return 0;case 25:return(unsigned long)(uintptr_t)compat_g_aux_random;default:return 0;}
}



static int compat_ssrnx_android_log_vprint(int priority, const char *tag, const char *fmt, va_list ap) {
    (void)priority;
    char msg[768];
    vsnprintf(msg, sizeof(msg), fmt ? fmt : "", ap);
    SSRNX_LOGCI("GUESTLOG", "%s: %s", tag ? tag : "android", msg);
    return (int)strlen(msg);
}

static int compat_ssrnx_android_log_print(int priority, const char *tag, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    const int rc = compat_ssrnx_android_log_vprint(priority, tag, fmt, ap);
    va_end(ap); return rc;
}

static int compat_ssrnx_android_log_write(int priority, const char *tag, const char *text) {
    (void)priority; SSRNX_LOGCI("GUESTLOG", "%s: %s", tag ? tag : "android", text ? text : "");
    return text ? (int)strlen(text) : 0;
}

static int compat_ssrnx_android_log_buf_write(int buffer_id, int priority, const char *tag, const char *text) {
    (void)buffer_id;
    return compat_ssrnx_android_log_write(priority, tag, text);
}

static void compat_ssrnx_android_log_assert(const char *cond, const char *tag, const char *fmt, ...) {
    char msg[512] = {0};
    va_list ap; va_start(ap, fmt); if (fmt) vsnprintf(msg, sizeof(msg), fmt, ap); va_end(ap);
    SSRNX_LOGCE("GUESTLOG", "ASSERT tag=%s cond=%s %s", tag ? tag : "android", cond ? cond : "", msg);
    abort();
}

/* Bionic arm64 pthread_attr_t is a public 56-byte ABI. Match Bionic's
 * observable defaults rather than zero-initializing the guest object. SSRNX
 * currently presents a 4 KiB Android page size. */
int ssrnx_bionic_pthread_attr_init(void *attr) {
    if (!attr) return EINVAL;
    SsrnxBionicPthreadAttrLayout *a = (SsrnxBionicPthreadAttrLayout *)attr;
    memset(a, 0, sizeof(*a));
    a->stack_size = SSRNX_BIONIC_PTHREAD_STACK_DEFAULT;
    a->guard_size = SSRNX_BIONIC_PAGE_SIZE;
    a->sched_policy = 0;   /* Android SCHED_NORMAL. */
    a->sched_priority = 0;
    return 0;
}
int ssrnx_bionic_pthread_attr_destroy(void *attr) {
    if (!attr) return EINVAL;
    memset(attr, 0x42, SSRNX_BIONIC_PTHREAD_ATTR_SIZE);
    return 0;
}
int ssrnx_bionic_pthread_attr_setdetachstate(void *attr, int state) {
    if (!attr || (state != 0 && state != 1)) return EINVAL;
    SsrnxBionicPthreadAttrLayout *a = (SsrnxBionicPthreadAttrLayout *)attr;
    if (state == 1) a->flags |= SSRNX_BIONIC_PTHREAD_ATTR_DETACHED;
    else a->flags &= ~SSRNX_BIONIC_PTHREAD_ATTR_DETACHED;
    return 0;
}
int ssrnx_bionic_pthread_attr_getdetachstate(const void *attr, int *state) {
    if (!attr || !state) return EINVAL;
    const SsrnxBionicPthreadAttrLayout *a = (const SsrnxBionicPthreadAttrLayout *)attr;
    *state = (a->flags & SSRNX_BIONIC_PTHREAD_ATTR_DETACHED) ? 1 : 0;
    return 0;
}
int ssrnx_bionic_pthread_attr_setstacksize(void *attr, size_t stack_size) {
    if (!attr || stack_size < SSRNX_BIONIC_PTHREAD_STACK_MIN) return EINVAL;
    /* Android 11 Bionic stores the requested size verbatim here. pthread_create
     * performs page rounding when it allocates the actual stack. */
    ((SsrnxBionicPthreadAttrLayout *)attr)->stack_size = stack_size;
    return 0;
}
int ssrnx_bionic_pthread_attr_getstacksize(const void *attr, size_t *stack_size) {
    if (!attr || !stack_size) return EINVAL;
    *stack_size = ((const SsrnxBionicPthreadAttrLayout *)attr)->stack_size;
    return 0;
}
int ssrnx_bionic_pthread_attr_setguardsize(void *attr, size_t guard_size) {
    if (!attr) return EINVAL;
    /* Android 11 Bionic keeps the requested guard size verbatim; allocation
     * rounds it to a page boundary later during pthread_create. */
    ((SsrnxBionicPthreadAttrLayout *)attr)->guard_size = guard_size;
    return 0;
}
int ssrnx_bionic_pthread_attr_getguardsize(const void *attr, size_t *guard_size) {
    if (!attr || !guard_size) return EINVAL;
    *guard_size = ((const SsrnxBionicPthreadAttrLayout *)attr)->guard_size;
    return 0;
}
int ssrnx_bionic_pthread_attr_getstack(const void *attr, void **stack_base, size_t *stack_size) {
    if (!attr || !stack_base || !stack_size) return EINVAL;
    const SsrnxBionicPthreadAttrLayout *a = (const SsrnxBionicPthreadAttrLayout *)attr;
    *stack_base = a->stack_base;
    *stack_size = a->stack_size;
    return 0;
}
int ssrnx_bionic_pthread_attr_setstack(void *attr, void *stack_base, size_t stack_size) {
    if (!attr || !stack_base) return EINVAL;
    if (stack_size < SSRNX_BIONIC_PTHREAD_STACK_MIN || (stack_size & (SSRNX_BIONIC_PAGE_SIZE - 1u)) != 0) return EINVAL;
    if (((uintptr_t)stack_base & (SSRNX_BIONIC_PAGE_SIZE - 1u)) != 0) return EINVAL;
    SsrnxBionicPthreadAttrLayout *a = (SsrnxBionicPthreadAttrLayout *)attr;
    a->stack_base = stack_base;
    a->stack_size = stack_size;
    return 0;
}
int ssrnx_bionic_pthread_attr_setinheritsched(void *attr, int inherit) {
    if (!attr) return EINVAL;
    SsrnxBionicPthreadAttrLayout *a = (SsrnxBionicPthreadAttrLayout *)attr;
    /* Android/Bionic: PTHREAD_INHERIT_SCHED=0, PTHREAD_EXPLICIT_SCHED=1. */
    if (inherit == 0) {
        a->flags |= SSRNX_BIONIC_PTHREAD_ATTR_INHERIT;
        a->flags &= ~SSRNX_BIONIC_PTHREAD_ATTR_EXPLICIT;
    } else if (inherit == 1) {
        a->flags &= ~SSRNX_BIONIC_PTHREAD_ATTR_INHERIT;
        a->flags |= SSRNX_BIONIC_PTHREAD_ATTR_EXPLICIT;
    } else {
        return EINVAL;
    }
    return 0;
}
int ssrnx_bionic_pthread_attr_getinheritsched(const void *attr, int *inherit) {
    if (!attr || !inherit) return EINVAL;
    const SsrnxBionicPthreadAttrLayout *a = (const SsrnxBionicPthreadAttrLayout *)attr;
    if (a->flags & SSRNX_BIONIC_PTHREAD_ATTR_INHERIT) *inherit = 0;
    else if (a->flags & SSRNX_BIONIC_PTHREAD_ATTR_EXPLICIT) *inherit = 1;
    else *inherit = (a->sched_policy != 0) ? 1 : 0; /* Historical pre-P Bionic behavior. */
    return 0;
}
int ssrnx_bionic_pthread_attr_setschedpolicy(void *attr, int policy) {
    if (!attr) return EINVAL;
    ((SsrnxBionicPthreadAttrLayout *)attr)->sched_policy = policy;
    return 0;
}
int ssrnx_bionic_pthread_attr_getschedpolicy(const void *attr, int *policy) {
    if (!attr || !policy) return EINVAL;
    *policy = ((const SsrnxBionicPthreadAttrLayout *)attr)->sched_policy;
    return 0;
}
int ssrnx_bionic_pthread_attr_setschedparam(void *attr, const struct sched_param *param) {
    if (!attr || !param) return EINVAL;
    ((SsrnxBionicPthreadAttrLayout *)attr)->sched_priority = param->sched_priority;
    return 0;
}
int ssrnx_bionic_pthread_attr_getschedparam(const void *attr, struct sched_param *param) {
    if (!attr || !param) return EINVAL;
    memset(param, 0, sizeof(*param));
    param->sched_priority = ((const SsrnxBionicPthreadAttrLayout *)attr)->sched_priority;
    return 0;
}
int ssrnx_bionic_pthread_attr_setscope(void *attr, int scope) {
    (void)attr;
    if (scope == 0) return 0;       /* PTHREAD_SCOPE_SYSTEM */
    if (scope == 1) return ENOTSUP; /* PTHREAD_SCOPE_PROCESS */
    return EINVAL;
}
int ssrnx_bionic_pthread_attr_getscope(const void *attr, int *scope) {
    if (!attr || !scope) return EINVAL;
    *scope = 0; /* PTHREAD_SCOPE_SYSTEM */
    return 0;
}
int ssrnx_bionic_pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void)) {
    return compat_ssrnx_bionic_register_atfork(prepare, parent, child, NULL);
}

#define SSRNX_BIONIC_MUTEXATTR_TYPE_MASK     0x0fL
#define SSRNX_BIONIC_MUTEXATTR_SHARED_MASK   0x10L
#define SSRNX_BIONIC_MUTEXATTR_PROTOCOL_MASK 0x20L
#define SSRNX_BIONIC_MUTEXATTR_PROTOCOL_SHIFT 5u

static int compat_ssrnx_bionic_pthread_mutexattr_init(void *attr) {
    if (!attr) return EINVAL;
    *(long *)attr = 0; /* PTHREAD_MUTEX_NORMAL | PTHREAD_PROCESS_PRIVATE */
    return 0;
}
static int compat_ssrnx_bionic_pthread_mutexattr_gettype(const void *attr, int *type) {
    if (!attr || !type) return EINVAL;
    const int value = (int)(*(const long *)attr & SSRNX_BIONIC_MUTEXATTR_TYPE_MASK);
    if (value < 0 || value > 2) return EINVAL;
    *type = value;
    return 0;
}
static int compat_ssrnx_bionic_pthread_mutexattr_settype(void *attr, int type) {
    if (!attr || type < 0 || type > 2) return EINVAL;
    long *value = (long *)attr;
    *value = (*value & ~SSRNX_BIONIC_MUTEXATTR_TYPE_MASK) | (long)type;
    return 0;
}
static int compat_ssrnx_bionic_pthread_mutexattr_getpshared(const void *attr, int *pshared) {
    if (!attr || !pshared) return EINVAL;
    *pshared = (*(const long *)attr & SSRNX_BIONIC_MUTEXATTR_SHARED_MASK) ? 1 : 0;
    return 0;
}
static int compat_ssrnx_bionic_pthread_mutexattr_setpshared(void *attr, int pshared) {
    if (!attr || (pshared != 0 && pshared != 1)) return EINVAL;
    long *value = (long *)attr;
    if (pshared) *value |= SSRNX_BIONIC_MUTEXATTR_SHARED_MASK;
    else *value &= ~SSRNX_BIONIC_MUTEXATTR_SHARED_MASK;
    return 0;
}
static int compat_ssrnx_bionic_pthread_mutexattr_setprotocol(void *attr, int protocol) {
    if (!attr || (protocol != 0 && protocol != 1)) return EINVAL; /* NONE / INHERIT */
    long *value = (long *)attr;
    *value = (*value & ~SSRNX_BIONIC_MUTEXATTR_PROTOCOL_MASK) |
             ((long)protocol << SSRNX_BIONIC_MUTEXATTR_PROTOCOL_SHIFT);
    return 0;
}
static int compat_ssrnx_bionic_pthread_mutexattr_getprotocol(const void *attr, int *protocol) {
    if (!attr || !protocol) return EINVAL;
    *protocol = (int)((*(const long *)attr & SSRNX_BIONIC_MUTEXATTR_PROTOCOL_MASK) >>
                      SSRNX_BIONIC_MUTEXATTR_PROTOCOL_SHIFT);
    return 0;
}
static int compat_ssrnx_bionic_pthread_mutexattr_destroy(void *attr) {
    if (!attr) return EINVAL;
    *(long *)attr = -1;
    return 0;
}

/* Common Bionic/Clang FORTIFY entry points. The compiler may emit these even
 * when source calls ordinary libc functions. Preserve the bounds check rather
 * than aliasing blindly to the host function. */
static void compat_ssrnx_fortify_fail(const char *fn) {
    SSRNX_LOGCE("FORTIFY", "%s detected a guest buffer overflow", fn ? fn : "fortify");
    compat_ssrnx_memtrace_flush(); /* preserve bounds diagnostics before host abort */
    abort();
}

static void *compat_ssrnx_memcpy_chk(void *dst, const void *src, size_t count, size_t dst_len) {
    if (count > dst_len) compat_ssrnx_fortify_fail("__memcpy_chk");
    return memcpy(dst, src, count);
}
static void *compat_ssrnx_memmove_chk(void *dst, const void *src, size_t count, size_t dst_len) {
    if (count > dst_len) compat_ssrnx_fortify_fail("__memmove_chk");
    return memmove(dst, src, count);
}
static void *compat_ssrnx_memset_chk(void *dst, int value, size_t count, size_t dst_len) {
    if (count > dst_len) compat_ssrnx_fortify_fail("__memset_chk");
    return memset(dst, value, count);
}

/* Bionic FORTIFY strlen. Android's implementation checks that the NUL byte
 * lies strictly within the compiler-supplied object bound. Use a bounded scan
 * so SSRNX never reads beyond mapped guest memory while preserving the same
 * success/failure contract for well-formed guest objects. */
static size_t compat_ssrnx_strlen_chk(const char *s, size_t s_len) {
    if (s_len == SIZE_MAX) return strlen(s);
    const char *nul = (const char *)memchr(s, '\0', s_len);
    if (!nul) compat_ssrnx_fortify_fail("__strlen_chk");
    return (size_t)(nul - s);
}
static ssize_t compat_ssrnx_read_chk(int fd, void *buf, size_t count, size_t buf_len) {
    if (count > buf_len) compat_ssrnx_fortify_fail("__read_chk");
    return compat_ssrnx_bionic_read(fd, buf, count);
}
static ssize_t compat_ssrnx_write_chk(int fd, const void *buf, size_t count, size_t buf_len) {
    if (count > buf_len) compat_ssrnx_fortify_fail("__write_chk");
    return compat_ssrnx_bionic_write(fd, buf, count);
}

/* formatted fortify */
/* Bionic's __vsprintf_chk treats SIZE_MAX as the compiler's unknown-bound
 * sentinel (AOSP fb3873d). newlib rejects a vsnprintf capacity > INT_MAX;
 * translate only that sentinel, retaining checks on every known object. */
static int compat_ssrnx_vsnprintf_chk(char *dst, size_t size, int flags, size_t dst_len, const char *fmt, va_list ap) {
    (void)flags;
    if (size > dst_len) {
        SSRNX_LOGCE("FORTIFY", "__vsnprintf_chk size=%zu bound=%zu", size, dst_len);
        compat_ssrnx_fortify_fail("__vsnprintf_chk");
    }
    return vsnprintf(dst, size, fmt, ap);
}
static int compat_ssrnx_snprintf_chk(char *dst, size_t size, int flags, size_t dst_len, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const int rc = compat_ssrnx_vsnprintf_chk(dst, size, flags, dst_len, fmt, ap);
    va_end(ap);
    return rc;
}
static int compat_ssrnx_vsprintf_chk(char *dst, int flags, size_t dst_len, const char *fmt, va_list ap) {
    (void)flags;
    const size_t capacity = dst_len == SIZE_MAX ? (size_t)INT_MAX : dst_len;
    const int rc = vsnprintf(dst, capacity, fmt, ap);
    /* A formatting/encoding error is returned to the caller, not an overflow.
     * Compare only nonnegative lengths, including the terminating NUL. */
    if (rc >= 0 && (size_t)rc >= dst_len) {
        SSRNX_LOGCE("FORTIFY", "__vsprintf_chk bound=%zu capacity=%zu result=%d errno=%d", dst_len, capacity, rc, errno);
        compat_ssrnx_fortify_fail("__vsprintf_chk");
    }
    return rc;
}
static int compat_ssrnx_sprintf_chk(char *dst, int flags, size_t dst_len, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const int rc = compat_ssrnx_vsprintf_chk(dst, flags, dst_len, fmt, ap);
    va_end(ap);
    return rc;
}



/* Bionic libm exports sincos/sincosf. Implement them from the host scalar
 * primitives instead of depending on a non-portable host sincos symbol (which
 * devkitA64/newlib is not required to provide). GCC can recognize adjacent
 * sin+cos calls and fold them back into sincos, so route through volatile
 * scalar function pointers to keep the final object dependent only on the
 * already-supported sin/cos and sinf/cosf symbols. */
typedef double (*compat_SsrnxUnaryDoubleMathFn)(double);
typedef float (*compat_SsrnxUnaryFloatMathFn)(float);
static compat_SsrnxUnaryDoubleMathFn volatile compat_g_ssrnx_host_sin = sin;
static compat_SsrnxUnaryDoubleMathFn volatile compat_g_ssrnx_host_cos = cos;
static compat_SsrnxUnaryFloatMathFn volatile compat_g_ssrnx_host_sinf = sinf;
static compat_SsrnxUnaryFloatMathFn volatile compat_g_ssrnx_host_cosf = cosf;

static void compat_ssrnx_bionic_sincos(double x, double *out_sin, double *out_cos) {
    const double s = compat_g_ssrnx_host_sin(x);
    const double c = compat_g_ssrnx_host_cos(x);
    *out_sin = s;
    *out_cos = c;
}

static void compat_ssrnx_bionic_sincosf(float x, float *out_sin, float *out_cos) {
    const float s = compat_g_ssrnx_host_sinf(x);
    const float c = compat_g_ssrnx_host_cosf(x);
    *out_sin = s;
    *out_cos = c;
}

static unsigned int compat_ssrnx_bionic_sleep(unsigned int seconds) {
    struct timespec req = { (time_t)seconds, 0 }, rem = { 0, 0 };
    return compat_ssrnx_safe_nanosleep(&req, &rem) == 0 ? 0u : (unsigned int)rem.tv_sec;
}
static int compat_ssrnx_bionic_usleep(unsigned int usec) {
    struct timespec req = { (time_t)(usec / 1000000u), (long)(usec % 1000000u) * 1000L };
    return compat_ssrnx_safe_nanosleep(&req, NULL);
}




/* dynamic fortify */
/* libdl must not claim that an arbitrary Android system DSO exists.  The old
 * implementation returned one fake non-NULL handle for every name.  FMOD then
 * successfully "opened" libaaudio.so, received NULL for every AAudio dlsym,
 * and later called so_AAudio_createStreamBuilder == NULL.  Android's loader
 * would instead fail dlopen() when the DSO is unavailable, allowing FMOD to
 * fall back to another output backend (OpenSL ES in SSRNX's virtual surface).
 *
 * Keep handles intentionally simple: virtual Android DSOs share one sentinel;
 * already-loaded guest modules use their stable SsrnxModule address. */
#define SSRNX_DLOPEN_GLOBAL_HANDLE  ((void *)(uintptr_t)UINT64_C(0xA640D100))
#define SSRNX_DLOPEN_VIRTUAL_HANDLE ((void *)(uintptr_t)UINT64_C(0xA640D101))

static __thread char compat_g_ssrnx_dlerror_text[192];
static __thread bool compat_g_ssrnx_dlerror_pending;

static void compat_ssrnx_dlerror_set(const char *fmt, const char *name) {
    if (!fmt) { compat_g_ssrnx_dlerror_pending = false; compat_g_ssrnx_dlerror_text[0] = '\0'; return; }
    snprintf(compat_g_ssrnx_dlerror_text, sizeof(compat_g_ssrnx_dlerror_text), fmt, name ? name : "<null>");
    compat_g_ssrnx_dlerror_pending = true;
}

static const char *compat_ssrnx_compat_path_basename(const char *path) {
    if (!path) return "";
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    const char *base = slash;
    if (!base || (backslash && backslash > base)) base = backslash;
    return base ? base + 1 : path;
}

static const SsrnxModule *compat_ssrnx_dlopen_find_guest(const char *name) {
    if (!compat_g_active_linker || !name || !name[0]) return NULL;
    const bool explicit_path = strchr(name, '/') != NULL || strchr(name, '\\') != NULL;
    const char *wanted = compat_ssrnx_compat_path_basename(name);
    for (size_t i = 0; i < ssrnx_linker_module_count(compat_g_active_linker); ++i) {
        const SsrnxModule *module = ssrnx_linker_module_at(compat_g_active_linker, i);
        if (!module) continue;
        if (explicit_path) {
            if (strcmp(module->path, name) == 0) return module;
        } else if (strcmp(compat_ssrnx_compat_path_basename(module->path), wanted) == 0) {
            return module;
        }
    }
    return NULL;
}

static const SsrnxModule *compat_ssrnx_dlopen_module_from_handle(void *handle) {
    if (!compat_g_active_linker || !handle) return NULL;
    for (size_t i = 0; i < ssrnx_linker_module_count(compat_g_active_linker); ++i) {
        const SsrnxModule *module = ssrnx_linker_module_at(compat_g_active_linker, i);
        if ((const void *)module == handle) return module;
    }
    return NULL;
}

static bool compat_ssrnx_dlopen_is_app_library_path(const char *name) {
    if (!name || !compat_g_active_linker || !compat_g_active_linker->library_dir[0]) return false;
    const size_t dir_len = strlen(compat_g_active_linker->library_dir);
    return strncmp(name, compat_g_active_linker->library_dir, dir_len) == 0 &&
           name[dir_len] == '/' && strchr(name + dir_len + 1u, '/') == NULL;
}

/* Bionic <dlfcn.h>: identical value on LP64 and ILP32. */
#define SSRNX_BIONIC_RTLD_NOLOAD 0x00004
#define SSRNX_BIONIC_RTLD_GLOBAL 0x00100

/* Android's dlopen() returns only after the object and its DT_NEEDED closure
 * have run DT_INIT/DT_INIT_ARRAY. The Java-route bootstrap pre-maps the native
 * graph with constructors deferred to the load site, so a mapped module is not
 * yet loaded in the Android-observable sense. A native loader that dlopen()s
 * such a module and then calls one of its exports (typically JNI_OnLoad) must
 * observe fully constructed static state. */
static void *compat_ssrnx_dlopen_complete_guest(const SsrnxModule *module, const char *name, int flags) {
    if (!ssrnx_linker_module_constructed(compat_g_active_linker, module) &&
        (flags & SSRNX_BIONIC_RTLD_NOLOAD)) {
        compat_ssrnx_dlerror_set("library \"%s\" wasn't loaded and RTLD_NOLOAD prevented it", name);
        return NULL;
    }
    const SsrnxResult rc = ssrnx_linker_acquire_module(compat_g_active_linker, module);
    if (rc != SSRNX_OK) {
        compat_ssrnx_dlerror_set("APN64 dlopen: load/construct failed for %s", name);
        SSRNX_LOGCE("COMPAT", "dlopen guest library failed name=%s rc=%s",
                    name, ssrnx_result_string(rc));
        return NULL;
    }
    if (flags & SSRNX_BIONIC_RTLD_GLOBAL)
        (void)ssrnx_linker_promote_global(compat_g_active_linker, module, "RTLD_GLOBAL");
    SSRNX_LOGCI("COMPAT", "dlopen guest library name=%s handle=%p flags=0x%x",
                name, (void *)(uintptr_t)module, flags);
    return (void *)(uintptr_t)module;
}

static void *compat_ssrnx_dlopen(const char *name, int flags) {
    compat_ssrnx_dlerror_set(NULL, NULL);
    if (!name) return SSRNX_DLOPEN_GLOBAL_HANDLE;
    const char *base = compat_ssrnx_compat_path_basename(name);
    if (ssrnx_compat_is_virtual_library(name) || ssrnx_compat_is_virtual_library(base))
        return SSRNX_DLOPEN_VIRTUAL_HANDLE;
    const SsrnxModule *module = compat_ssrnx_dlopen_find_guest(name);
    if (module) return compat_ssrnx_dlopen_complete_guest(module, name, flags);

    /* Android permits dlopen() of package-private DSOs after process startup.
     * SSRNX already has an incremental linker for System.loadLibrary(); route
     * the same contract through dlopen for a bare SONAME or the exact installed
     * app nativeLibraryDir. Paths outside that directory remain fail-closed. */
    const bool bare = strchr(name, '/') == NULL && strchr(name, '\\') == NULL;
    if (flags & SSRNX_BIONIC_RTLD_NOLOAD) {
        compat_ssrnx_dlerror_set("library \"%s\" wasn't loaded and RTLD_NOLOAD prevented it", name);
        return NULL;
    }
    if (compat_g_active_linker && base[0] && (bare || compat_ssrnx_dlopen_is_app_library_path(name))) {
        SsrnxModule *loaded = NULL;
        bool newly_loaded = false;
        SsrnxResult rc = ssrnx_linker_load_library(compat_g_active_linker, base, &loaded, &newly_loaded);
        if (rc == SSRNX_OK && loaded) {
            SSRNX_LOGCI("COMPAT", "dlopen app library name=%s soname=%s newlyLoaded=%u flags=0x%x global=%u",
                        name, base, newly_loaded ? 1u : 0u, flags,
                        (flags & SSRNX_BIONIC_RTLD_GLOBAL) ? 1u : 0u);
            /* An existing node matched by DT_SONAME may still be pre-mapped. */
            return compat_ssrnx_dlopen_complete_guest(loaded, name, flags);
        }
        SSRNX_LOGCI("COMPAT", "dlopen app library failed name=%s soname=%s rc=%s",
                    name, base, ssrnx_result_string(rc));
    }

    compat_ssrnx_dlerror_set("APN64 dlopen: unavailable shared object %s", name);
    SSRNX_LOGCI("COMPAT", "dlopen unavailable name=%s", name);
    return NULL;
}

static void *compat_ssrnx_dlsym_diagnostics(const SsrnxModule *provider, const char *name, uintptr_t address) {
    return (void *)address;
}

static void *compat_ssrnx_dlsym(void *handle, const char *name) {
    compat_ssrnx_dlerror_set(NULL, NULL);
    if (!name || !name[0]) {
        compat_ssrnx_dlerror_set("APN64 dlsym: invalid symbol %s", name);
        return NULL;
    }

    uintptr_t addr = 0;
    if (!handle || handle == SSRNX_DLOPEN_GLOBAL_HANDLE || handle == SSRNX_DLOPEN_VIRTUAL_HANDLE) {
        if (ssrnx_compat_resolve(name, &addr, NULL) == SSRNX_OK) return (void *)addr;
        if ((!handle || handle == SSRNX_DLOPEN_GLOBAL_HANDLE) && compat_g_active_linker) {
            const SsrnxModule *provider = NULL;
            if (ssrnx_linker_find_global(compat_g_active_linker, name, NULL, &addr, &provider) == SSRNX_OK)
                return compat_ssrnx_dlsym_diagnostics(provider, name, addr);
        }
    } else {
        const SsrnxModule *module = compat_ssrnx_dlopen_module_from_handle(handle);
        const SsrnxModule *provider = NULL;
        if (module && ssrnx_linker_module_handle_live(compat_g_active_linker, module) &&
            ssrnx_linker_find_in_handle(compat_g_active_linker, module, name, NULL, &addr, &provider) == SSRNX_OK) {
            return compat_ssrnx_dlsym_diagnostics(provider, name, addr);
        }
    }

    compat_ssrnx_dlerror_set("APN64 dlsym: symbol not found %s", name);
    return NULL;
}

static int compat_ssrnx_dlclose(void *handle) {
    compat_ssrnx_dlerror_set(NULL, NULL);
    if (!handle || handle == SSRNX_DLOPEN_GLOBAL_HANDLE || handle == SSRNX_DLOPEN_VIRTUAL_HANDLE) return 0;
    const SsrnxModule *module = compat_ssrnx_dlopen_module_from_handle(handle);
    if (!module || !ssrnx_linker_module_handle_live(compat_g_active_linker, module)) {
        compat_ssrnx_dlerror_set("APN64 dlclose: invalid handle %s", "<handle>");
        return -1;
    }
    const SsrnxResult rc = ssrnx_linker_release_module(compat_g_active_linker, module);
    if (rc != SSRNX_OK) {
        compat_ssrnx_dlerror_set("APN64 dlclose: release failed for %.140s", module->path);
        return -1;
    }
    return 0;
}

static const char *compat_ssrnx_dlerror(void) {
    if (!compat_g_ssrnx_dlerror_pending) return NULL;
    compat_g_ssrnx_dlerror_pending = false;
    return compat_g_ssrnx_dlerror_text;
}


/* Stage 14.8: Cocos/Bionic compatibility batch proven by the 14.7.1
 * hardware preflight. These implementations are runtime-wide Android ABI
 * bridges; none key off package/game names. */
static uintptr_t compat_g_bionic_stack_chk_guard_export = UINT64_C(0x9e3779b97f4a7c15);
static uint32_t compat_g_bionic_umask = 0022u;


#ifdef __aarch64__
extern int ssrnx_guest__setjmp(void *env);
extern int ssrnx_guest_setjmp(void *env);
extern int ssrnx_guest_sigsetjmp(void *env, int save_mask);
extern void ssrnx_guest_longjmp(void *env, int value) __attribute__((noreturn));
extern void ssrnx_guest_siglongjmp(void *env, int value) __attribute__((noreturn));
#else
static int ssrnx_guest__setjmp(void *env) { (void)env; return 0; }
static int ssrnx_guest_setjmp(void *env) { (void)env; return 0; }
static int ssrnx_guest_sigsetjmp(void *env, int save_mask) { (void)env; (void)save_mask; return 0; }
static void ssrnx_guest_longjmp(void *env, int value) { (void)env; (void)value; abort(); }
static void ssrnx_guest_siglongjmp(void *env, int value) { (void)env; (void)value; abort(); }
#endif

static char *compat_ssrnx_bionic_strdup(const char *s) {
    if (!s) { errno = EFAULT; return NULL; }
    size_t n = strlen(s) + 1u; char *p = (char *)malloc(n); if (p) memcpy(p, s, n); return p;
}
static long compat_ssrnx_bionic_atol(const char *s) { return strtol(s, NULL, 10); }
static div_t compat_ssrnx_bionic_div(int numerator, int denominator) { return div(numerator, denominator); }
static size_t compat_ssrnx_bionic_strnlen(const char *s, size_t maxlen) {
    const void *end = memchr(s, '\0', maxlen);
    return end ? (size_t)((const char *)end - s) : maxlen;
}
static char *compat_ssrnx_bionic_strncat(char *dst, const char *src, size_t n) { return strncat(dst, src, n); }
static char *compat_ssrnx_bionic_strcasestr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    const size_t nlen = strlen(needle);
    for (const char *p = haystack; *p; ++p) {
        if (strncasecmp(p, needle, nlen) == 0) return (char *)p;
    }
    return NULL;
}
static int compat_ssrnx_bionic_fnmatch(const char *pattern, const char *string, int guest_flags) {
    int host_flags = 0;
    if (guest_flags & 0x01) host_flags |= FNM_NOESCAPE;
    if (guest_flags & 0x02) host_flags |= FNM_PATHNAME;
    if (guest_flags & 0x04) host_flags |= FNM_PERIOD;
#ifdef FNM_LEADING_DIR
    if (guest_flags & 0x08) host_flags |= FNM_LEADING_DIR;
#else
    if (guest_flags & 0x08) return FNM_NOMATCH;
#endif
#ifdef FNM_CASEFOLD
    if (guest_flags & 0x10) host_flags |= FNM_CASEFOLD;
#else
    if (guest_flags & 0x10) return FNM_NOMATCH;
#endif
    return fnmatch(pattern, string, host_flags) == 0 ? 0 : FNM_NOMATCH;
}
/* Observe templates rather than formatting a second time: %n must run once. */
static int compat_ssrnx_bionic_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const int result = vprintf(fmt, ap);
    va_end(ap);
    const int saved = errno;
    static unsigned reports;
    if (__atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 256u)
        SSRNX_LOGCI("STDOUT", "printf caller=%p result=%d format=%.256s",
                    __builtin_return_address(0), result, fmt ? fmt : "<null>");
    errno = saved;
    return result;
}
static int compat_ssrnx_bionic_puts(const char *s) { return puts(s); }
static int compat_ssrnx_bionic_vprintf(const char *fmt, va_list ap) { return vprintf(fmt, ap); }
static void *compat_ssrnx_bionic_memrchr(const void *s, int c, size_t n) {
    const unsigned char *p = (const unsigned char *)s;
    while (n) { --n; if (p[n] == (unsigned char)c) return (void *)(p+n); }
    return NULL;
}
static int compat_ssrnx_bionic_isascii(int c) { return ((unsigned)c & ~0x7fu) == 0; }
static int compat_ssrnx_bionic_isnanf(float v) { return isnan(v) ? 1 : 0; }
static int compat_ssrnx_bionic_isnan(double v) { return isnan(v) ? 1 : 0; }
static int compat_ssrnx_bionic_isinf(double v) { return isinf(v) ? 1 : 0; }
static int compat_ssrnx_bionic_signbit(double v) { return signbit(v) ? 1 : 0; }
/* Bionic exposes _ctype_ as a data symbol containing a pointer to a
 * 257-byte classification table. Legacy Android code may reference this
 * directly instead of calling isalpha()/isdigit(). Build the ASCII portion
 * using Bionic's documented flag values before guest constructors execute. */
static char compat_g_bionic_ctype_table[257];
static const char *compat_g_bionic_ctype_export = compat_g_bionic_ctype_table;
static bool compat_g_bionic_ctype_ready;
static void compat_ssrnx_bionic_init_ctype_table(void) {
    if (compat_g_bionic_ctype_ready) return;
    enum { U=0x01,L=0x02,D=0x04,S=0x08,P=0x10,C=0x20,X=0x40,B=0x80 };
    memset(compat_g_bionic_ctype_table,0,sizeof(compat_g_bionic_ctype_table));
    for (unsigned ch=0;ch<256;++ch) {
        unsigned char f=0;
        if (ch<0x20u || ch==0x7fu) f|=C;
        if (ch==' ' || ch=='\t' || ch=='\n' || ch=='\v' || ch=='\f' || ch=='\r') f|=S;
        if (ch==' ') f|=B;
        if (ch>='0'&&ch<='9') f|=D;
        if (ch>='A'&&ch<='Z') { f|=U; if (ch<='F') f|=X; }
        if (ch>='a'&&ch<='z') { f|=L; if (ch<='f') f|=X; }
        if (ch>=0x21u && ch<=0x7eu && !(f&(U|L|D))) f|=P;
        compat_g_bionic_ctype_table[ch+1u]=(char)f;
    }
    compat_g_bionic_ctype_ready=true;
}

static ssize_t compat_ssrnx_bionic_writev(int fd, const compat_SsrnxAndroidIovec *iov, int iovcnt) {
    if (iovcnt < 0 || (iovcnt > 0 && !iov)) { errno=EINVAL; return -1; }
    ssize_t total=0;
    for (int i=0;i<iovcnt;++i) {
        const unsigned char *p=(const unsigned char *)iov[i].iov_base;
        size_t left=iov[i].iov_len;
        while (left) {
            ssize_t n=compat_ssrnx_bionic_write(fd,p,left);
            if (n<0) return total?total:-1;
            if (n==0) return total;
            total+=n; p+=(size_t)n; left-=(size_t)n;
            if ((size_t)n < left+(size_t)n) return total;
        }
    }
    return total;
}

/* Stage 14.8.52 buildfix1: SSRNX owns the Bionic regex ABI. Do not make
 * exported Android symbols depend on optional host/newlib regex objects. */
_Static_assert(sizeof(SsrnxBionicRegex) == 32, "Bionic regex_t LP64 ABI size");
_Static_assert(offsetof(SsrnxBionicRegex, re_magic) == 0, "regex_t re_magic offset");
_Static_assert(offsetof(SsrnxBionicRegex, re_nsub) == 8, "regex_t re_nsub offset");
_Static_assert(offsetof(SsrnxBionicRegex, re_endp) == 16, "regex_t re_endp offset");
_Static_assert(offsetof(SsrnxBionicRegex, re_g) == 24, "regex_t re_g offset");
_Static_assert(sizeof(SsrnxBionicRegmatch) == 16, "Bionic regmatch_t LP64 ABI size");
_Static_assert(sizeof(SsrnxBionicRegoff) == 8, "Bionic regoff_t LP64 ABI size");

static int compat_ssrnx_bionic_fpclassifyd(double x) {
    if (isnan(x)) return 0x02;      /* Bionic FP_NAN */
    if (isinf(x)) return 0x01;      /* Bionic FP_INFINITE */
    if (x==0.0) return 0x10;        /* Bionic FP_ZERO */
    if (fabs(x)<DBL_MIN) return 0x08; /* Bionic FP_SUBNORMAL */
    return 0x04;                    /* Bionic FP_NORMAL */
}

static int compat_ssrnx_bionic_ctype_call(int c, int which) {
    unsigned char u=(unsigned char)c;
    switch(which){case 0:return isprint(u);case 1:return isalnum(u);case 2:return islower(u);case 3:return isalpha(u);
    case 4:return isxdigit(u);case 5:return isupper(u);case 6:return isspace(u);case 7:return isgraph(u);default:return 0;}
}
static int compat_ssrnx_bionic_isprint(int c){return compat_ssrnx_bionic_ctype_call(c,0);} static int compat_ssrnx_bionic_isalnum(int c){return compat_ssrnx_bionic_ctype_call(c,1);}
static int compat_ssrnx_bionic_islower(int c){return compat_ssrnx_bionic_ctype_call(c,2);} static int compat_ssrnx_bionic_isalpha(int c){return compat_ssrnx_bionic_ctype_call(c,3);}
static int compat_ssrnx_bionic_isxdigit(int c){return compat_ssrnx_bionic_ctype_call(c,4);} static int compat_ssrnx_bionic_isupper(int c){return compat_ssrnx_bionic_ctype_call(c,5);}
static int compat_ssrnx_bionic_isspace(int c){return compat_ssrnx_bionic_ctype_call(c,6);} static int compat_ssrnx_bionic_isgraph(int c){return compat_ssrnx_bionic_ctype_call(c,7);}
static int compat_ssrnx_bionic_tolower(int c){return tolower((unsigned char)c);} static int compat_ssrnx_bionic_toupper(int c){return toupper((unsigned char)c);}

static int compat_ssrnx_bionic_sigaddset(compat_SsrnxAndroidSigset *set,int signo){if(!set||signo<=0||signo>=SSRNX_BIONIC_NSIG){errno=EINVAL;return -1;}*set|=UINT64_C(1)<<(signo-1);return 0;}
static int compat_ssrnx_bionic_sigdelset(compat_SsrnxAndroidSigset *set,int signo){if(!set||signo<=0||signo>=SSRNX_BIONIC_NSIG){errno=EINVAL;return -1;}*set&=~(UINT64_C(1)<<(signo-1));return 0;}
static int compat_ssrnx_bionic_sigfillset(compat_SsrnxAndroidSigset *set){if(!set){errno=EINVAL;return -1;}*set=UINT64_MAX;return 0;}
static compat_SsrnxAndroidSignalHandler compat_ssrnx_bionic_signal(int signo,compat_SsrnxAndroidSignalHandler handler){compat_SsrnxAndroidSigaction n={0},o={0};n.action.sa_handler=handler;if(ssrnx_bionic_sigaction(signo,&n,&o)!=0)return (compat_SsrnxAndroidSignalHandler)(intptr_t)-1;return o.action.sa_handler;}
static int compat_ssrnx_bionic_kill(int pid,int signo){if(pid==0||pid==(int)getpid()){if(signo==0)return 0;return ssrnx_bionic_raise(signo);}errno=ESRCH;return -1;}
static unsigned compat_ssrnx_bionic_alarm(unsigned seconds){(void)seconds;return 0;}

static uint32_t compat_ssrnx_bionic_getuid(void){return 10000u;} static uint32_t compat_ssrnx_bionic_geteuid(void){return 10000u;}
static uint32_t compat_ssrnx_bionic_getgid(void){return 10000u;} static uint32_t compat_ssrnx_bionic_getegid(void){return 10000u;}
static int compat_ssrnx_bionic_setuid(uint32_t id){if(id==10000u)return 0;errno=EPERM;return -1;}
static int compat_ssrnx_bionic_setgid(uint32_t id){if(id==10000u)return 0;errno=EPERM;return -1;}
static int compat_ssrnx_bionic_initgroups(const char *user,uint32_t gid){(void)user;(void)gid;errno=EPERM;return -1;}

typedef struct compat_SsrnxAndroidPasswd { char *pw_name; char *pw_passwd; uint32_t pw_uid,pw_gid; char *pw_dir; char *pw_shell; } compat_SsrnxAndroidPasswd;
static char compat_g_pw_name[]="apn64",compat_g_pw_pass[]="x",compat_g_pw_dir[]="/data/local/tmp",compat_g_pw_shell[]="/system/bin/sh";
static compat_SsrnxAndroidPasswd compat_g_pw={compat_g_pw_name,compat_g_pw_pass,10000u,10000u,compat_g_pw_dir,compat_g_pw_shell};
static compat_SsrnxAndroidPasswd *compat_ssrnx_bionic_getpwuid(uint32_t uid){return uid==10000u?&compat_g_pw:NULL;}
static int compat_ssrnx_bionic_getpwuid_r(uint32_t uid,compat_SsrnxAndroidPasswd *out,char *buf,size_t buflen,compat_SsrnxAndroidPasswd **result){if(!out||!buf||!result)return EINVAL;*result=NULL;if(uid!=10000u)return 0;const char *v[]={compat_g_pw_name,compat_g_pw_pass,compat_g_pw_dir,compat_g_pw_shell};size_t need=0;for(size_t i=0;i<4;i++)need+=strlen(v[i])+1u;if(buflen<need)return ERANGE;char *p=buf;char *dst[4];for(size_t i=0;i<4;i++){size_t n=strlen(v[i])+1u;dst[i]=p;memcpy(p,v[i],n);p+=n;}*out=(compat_SsrnxAndroidPasswd){dst[0],dst[1],10000u,10000u,dst[2],dst[3]};*result=out;return 0;}

static uint32_t compat_ssrnx_bionic_umask(uint32_t mask){uint32_t old=compat_g_bionic_umask;compat_g_bionic_umask=mask&0777u;return old;}
static int compat_ssrnx_bionic_tcgetattr(int fd,void *termios){(void)fd;(void)termios;errno=ENOTTY;return -1;}
static int compat_ssrnx_bionic_tcsetattr(int fd,int action,const void *termios){(void)fd;(void)action;(void)termios;errno=ENOTTY;return -1;}
static int compat_ssrnx_bionic_mlock(const void *addr,size_t len){if(!addr&&len){errno=EINVAL;return -1;}return 0;}



/* memory */
/* Linux mmap family.
 *
 * Host builds use one page-aligned heap block per non-fixed mapping. On Switch,
 * private mappings use address-space reservations plus process-code aliases.
 * PROT_NONE anonymous mappings commit subranges lazily; accessible mappings
 * commit immediately. Both can later become executable at the same address.
 *
 * Page state is tracked per block so the Linux contract holds at page granularity:
 *  - munmap() of any page-aligned subrange succeeds (head/tail trimming of an
 *    over-sized reservation is the standard way to get aligned regions), and
 *    ranges that are not mapped are not an error;
 *  - ordinary heap-backed blocks are released only when their last page is unmapped;
 *  - sparse Switch reservations allocate physical backing only for committed pages;
 *  - a non-fixed addr is a hint and never makes mmap() fail;
 *  - MAP_FIXED / MAP_FIXED_NOREPLACE are honoured inside blocks SSRNX owns;
 *  - MADV_DONTNEED on private anonymous pages zero-fills them.
 * Writable shared file mappings fail: no coherent file writeback backend exists.
 * Protection changes on ordinary heap-backed blocks are accepted but not
 * enforced (host heap is RW); sparse Switch mappings keep PROT_NONE genuinely uncommitted and alias only committed pages through SSRNX's already-proven process-code mapping path; later protection changes remain advisory, except PROT_EXEC: executable private mappings are sparse, and their pages really become executable (see the W^X section below). */
#define SSRNX_MM_PAGE ((uintptr_t)4096u)
#define SSRNX_MM_MAP_TYPE 0x0f
#define SSRNX_MM_MAP_SHARED 0x01
#define SSRNX_MM_MAP_PRIVATE 0x02
#define SSRNX_MM_MAP_SHARED_VALIDATE 0x03
#define SSRNX_MM_MAP_FIXED 0x10
#define SSRNX_MM_MAP_ANONYMOUS 0x20
#define SSRNX_MM_MAP_FIXED_NOREPLACE 0x100000
#define SSRNX_MM_MADV_DONTNEED 4
#define SSRNX_MM_MADV_REMOVE 9
#define SSRNX_MM_PROT_WRITE 2
#define SSRNX_MM_PROT_EXEC 4

typedef struct compat_SsrnxMapBlock {
    uintptr_t base;
    size_t pages;
    size_t live;         /* pages currently mapped */
    uint8_t *mapped;     /* bit per page: page is mapped */
    uint8_t *anon;       /* bit per page: mapped page is private anonymous */
    uint8_t *committed;  /* bit per page: backing is currently mapped */
    uint8_t *prot;       /* Android PROT_* bits per page */
#ifdef __SWITCH__
    bool sparse;
    unsigned wx_slot;    /* 1 + index into g_wx_slots once a page was mapped executable */
    SsrnxVmRange *reservation;
    struct compat_SsrnxMapChunk *chunks;
#endif
} compat_SsrnxMapBlock;

#ifdef __SWITCH__
typedef struct compat_SsrnxMapBacking {
    void *ptr;
    size_t pages;
    size_t refs;
} compat_SsrnxMapBacking;

typedef struct compat_SsrnxMapChunk {
    struct compat_SsrnxMapChunk *next;
    uintptr_t guest_base;
    compat_SsrnxMapBacking *owner;
    size_t source_page;
    size_t pages;
} compat_SsrnxMapChunk;

static void compat_mm_sparse_backing_release(compat_SsrnxMapBacking *owner);
static bool compat_mm_sparse_chunk_unmap(compat_SsrnxMapChunk *chunk);
static void compat_mm_wx_slot_release_wxlocked(compat_SsrnxMapBlock *b);
static bool compat_mm_wx_lock(void) __attribute__((target("general-regs-only")));
static void compat_mm_wx_unlock(void) __attribute__((target("general-regs-only")));
#endif

static compat_SsrnxMapBlock **compat_g_map_blocks;
static size_t compat_g_map_count, compat_g_map_cap;
static pthread_mutex_t compat_g_map_lock = PTHREAD_MUTEX_INITIALIZER;

static inline bool compat_mm_bit(const uint8_t *bm, size_t i) { return (bm[i >> 3] >> (i & 7u)) & 1u; }
static inline void compat_mm_bit_set(uint8_t *bm, size_t i, bool v) {
    if (v) bm[i >> 3] |= (uint8_t)(1u << (i & 7u));
    else bm[i >> 3] &= (uint8_t)~(1u << (i & 7u));
}
static inline uintptr_t compat_mm_block_end(const compat_SsrnxMapBlock *b) { return b->base + b->pages * SSRNX_MM_PAGE; }
static inline void compat_mm_page_mark_committed_locked(compat_SsrnxMapBlock *b, size_t i) {
#ifdef __SWITCH__
    if (b->sparse) return;
#endif
    compat_mm_bit_set(b->committed, i, true);
}

/* Index of the first block whose end lies above addr (blocks sorted by base). */
static size_t compat_mm_first_above_locked(uintptr_t addr) {
    size_t lo = 0, hi = compat_g_map_count;
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2u;
        if (compat_mm_block_end(compat_g_map_blocks[mid]) <= addr) lo = mid + 1u; else hi = mid;
    }
    return lo;
}

static bool compat_mm_insert_locked(compat_SsrnxMapBlock *b) {
    if (compat_g_map_count == compat_g_map_cap) {
        const size_t cap = compat_g_map_cap ? compat_g_map_cap * 2u : 64u;
        compat_SsrnxMapBlock **grown = (compat_SsrnxMapBlock **)realloc(compat_g_map_blocks, cap * sizeof(*grown));
        if (!grown) return false;
        compat_g_map_blocks = grown;
        compat_g_map_cap = cap;
    }
    const size_t at = compat_mm_first_above_locked(b->base);
    memmove(&compat_g_map_blocks[at + 1u], &compat_g_map_blocks[at], (compat_g_map_count - at) * sizeof(*compat_g_map_blocks));
    compat_g_map_blocks[at] = b;
    ++compat_g_map_count;
    return true;
}

static void compat_mm_remove_locked(size_t index) {
    compat_SsrnxMapBlock *b = compat_g_map_blocks[index];
    memmove(&compat_g_map_blocks[index], &compat_g_map_blocks[index + 1u], (compat_g_map_count - index - 1u) * sizeof(*compat_g_map_blocks));
    --compat_g_map_count;
#ifdef __SWITCH__
    if (b->sparse) {
        const bool wx = b->wx_slot != 0;
        if (wx) (void)compat_mm_wx_lock();
        compat_mm_wx_slot_release_wxlocked(b);
        compat_SsrnxMapChunk *chunk = b->chunks;
        while (chunk) {
            compat_SsrnxMapChunk *next = chunk->next;
            if (chunk->owner) {
                if (!compat_mm_sparse_chunk_unmap(chunk)) {
                    /* Never free borrowed source pages while a kernel alias may
                     * still exist. Teardown leaks this backing rather than risk
                     * turning a failed unmap into use-after-free corruption. */
                    chunk->owner = NULL;
                }
            }
            free(chunk);
            chunk = next;
        }
        if (wx) compat_mm_wx_unlock();
        ssrnx_vm_release(b->reservation);
        b->reservation = NULL;
    } else
#endif
    {
        free((void *)b->base);
    }
    free(b);
}

#ifdef __SWITCH__
/* Sparse Android reservations stay in the general ASLR region. Physical pages
 * are supplied lazily by aliasing exact-size existing heap runs with
 * svcMapProcessCodeMemory(), the same mechanism SSRNX and nx-hbloader already
 * use for guest/NRO images. This avoids svcMapMemory's Stack-region restriction
 * without allocating additional physical memory outside the hbloader heap.
 *
 * Android allocators commonly reserve ~2x their desired aligned range and
 * immediately munmap() the head and tail. Shrink SSRNX's ownership envelope
 * and libnx reservation to the bounding live range after those trims so the
 * discarded VA can be safely reused without overlapping SSRNX VM metadata. */
static void compat_mm_sparse_compact_block_locked(compat_SsrnxMapBlock *b) {
    if (!b || !b->sparse || b->live == 0) return;

    const size_t old_pages = b->pages;
    size_t first = 0;
    while (first < old_pages && !compat_mm_bit(b->mapped, first)) ++first;
    if (first == old_pages) return;
    size_t last = old_pages;
    while (last > first && !compat_mm_bit(b->mapped, last - 1u)) --last;
    if (first == 0 && last == old_pages) return;

    const size_t new_pages = last - first;
    const uintptr_t new_base = b->base + first * SSRNX_MM_PAGE;
    if (first) {
        for (size_t i = 0; i < new_pages; ++i) {
            const size_t src = i + first;
            compat_mm_bit_set(b->mapped, i, compat_mm_bit(b->mapped, src));
            compat_mm_bit_set(b->anon, i, compat_mm_bit(b->anon, src));
            compat_mm_bit_set(b->committed, i, compat_mm_bit(b->committed, src));
            b->prot[i] = b->prot[src];
        }
    }
    for (size_t i = new_pages; i < old_pages; ++i) {
        compat_mm_bit_set(b->mapped, i, false);
        compat_mm_bit_set(b->anon, i, false);
        compat_mm_bit_set(b->committed, i, false);
        b->prot[i] = 0;
    }
    b->base = new_base;
    b->pages = new_pages;

    if (b->reservation) {
        if (!ssrnx_vm_shrink(b->reservation, (void *)b->base, b->pages * SSRNX_MM_PAGE)) {
            SSRNX_LOGCW("VM", "sparse reservation compact failed base=%p len=%zu; keeping previous reservation",
                        (void *)b->base, b->pages * SSRNX_MM_PAGE);
        }
    }
}

static void compat_mm_sparse_backing_release(compat_SsrnxMapBacking *owner) {
    if (!owner || owner->refs == 0) return;
    if (--owner->refs == 0) {
        free(owner->ptr);
        free(owner);
    }
}

static bool compat_mm_sparse_chunk_map(compat_SsrnxMapChunk *chunk, compat_SsrnxMapBacking *owner,
                                size_t source_page, uintptr_t guest, size_t pages) {
    if (!chunk || !owner || pages == 0) return false;
    const size_t size = pages * SSRNX_MM_PAGE;
    void *source = (uint8_t *)owner->ptr + source_page * SSRNX_MM_PAGE;

    /* This is the same heap -> randomized code-memory alias mechanism SSRNX
     * already uses for every mapped guest ELF image. Unlike svcMapMemory it is
     * valid in the general code/ASLR region, and unlike a fresh physical-memory
     * allocation it borrows pages from the existing hbloader heap. */
    armDCacheFlush(source, size);
    Result rc = svcMapProcessCodeMemory(envGetOwnProcessHandle(), (u64)guest,
                                        (u64)(uintptr_t)source, size);
    if (R_FAILED(rc)) {
        MemoryInfo dst_info = {0}, src_info = {0};
        u32 dst_page = 0, src_page = 0;
        (void)svcQueryMemory(&dst_info, &dst_page, guest);
        (void)svcQueryMemory(&src_info, &src_page, (u64)(uintptr_t)source);
        SSRNX_LOGCE("VM",
                    "process-code alias map failed guest=%p source=%p len=%zu rc=0x%08x dstType=%#x dstBase=%p dstSize=%zu srcType=%#x srcBase=%p srcSize=%zu",
                    (void *)guest, source, size, rc, dst_info.type,
                    (void *)(uintptr_t)dst_info.addr, (size_t)dst_info.size, src_info.type,
                    (void *)(uintptr_t)src_info.addr, (size_t)src_info.size);
        return false;
    }

    /* Guest anonymous pages are data. The established SSRNX mmap contract is
     * RW host backing with Android protection tracked separately, so keep the
     * alias writable and preserve the existing advisory mprotect semantics. */
    rc = svcSetProcessMemoryPermission(envGetOwnProcessHandle(), (u64)guest,
                                       size, Perm_Rw);
    if (R_FAILED(rc)) {
        SSRNX_LOGCE("VM", "process-code alias RW permission failed guest=%p len=%zu rc=0x%08x",
                    (void *)guest, size, rc);
        Result urc = svcUnmapProcessCodeMemory(envGetOwnProcessHandle(), (u64)guest,
                                               (u64)(uintptr_t)source, size);
        if (R_FAILED(urc))
            SSRNX_LOGCE("VM", "process-code alias rollback failed guest=%p source=%p len=%zu rc=0x%08x",
                        (void *)guest, source, size, urc);
        return false;
    }

    chunk->guest_base = guest;
    chunk->owner = owner;
    chunk->source_page = source_page;
    chunk->pages = pages;
    ++owner->refs;
    return true;
}

static bool compat_mm_sparse_chunk_unmap(compat_SsrnxMapChunk *chunk) {
    if (!chunk || !chunk->owner) return false;
    const size_t size = chunk->pages * SSRNX_MM_PAGE;
    void *source = (uint8_t *)chunk->owner->ptr + chunk->source_page * SSRNX_MM_PAGE;
    Result rc = svcUnmapProcessCodeMemory(envGetOwnProcessHandle(), (u64)chunk->guest_base,
                                          (u64)(uintptr_t)source, size);
    if (R_FAILED(rc)) {
        SSRNX_LOGCE("VM", "process-code alias unmap failed guest=%p source=%p len=%zu rc=0x%08x",
                    (void *)chunk->guest_base, source, size, rc);
        return false;
    }
    compat_mm_sparse_backing_release(chunk->owner);
    chunk->owner = NULL;
    return true;
}

/* W^X emulation of Android RWX pages.
 *
 * Android lets an app map private memory PROT_READ|PROT_WRITE|PROT_EXEC and
 * change it with mprotect() (JIT code caches: V8, LuaJIT, PCRE2-JIT ...).
 * Horizon never maps a page writable and executable at once, and a committed
 * sparse page (a process-code alias) can only change permission one way:
 *   AliasCode     (MemType_ModuleCodeStatic):  R or RX; svcSetProcessMemoryPermission
 *                 may switch it to RW, which turns it into
 *   AliasCodeData (MemType_ModuleCodeMutable): RW for good (the state has no
 *                 code flag, so svcSetProcessMemoryPermission refuses it).
 * RX -> RW is therefore one permission change, and RW -> RX unmaps the alias,
 * maps the same backing pages again (contents live in the backing) and sets RX.
 * Committed pages of a block that was ever mapped executable get:
 *   PROT_EXEC without PROT_WRITE  -> RX
 *   PROT_EXEC with PROT_WRITE     -> RW or RX, whichever the last access needed
 *   no PROT_EXEC                  -> RW (protection otherwise advisory, as above)
 * An instruction fetch from an RW page the guest mapped executable makes it,
 * and the executable pages next to it within one 64 KiB group, RX; a store to
 * an RX page the guest mapped writable makes that page RW. exception_entry.s
 * then resumes the faulting instruction. Any other fault stays a genuine fault
 * and reaches the crash report, as SIGSEGV would on Linux.
 *
 * Every host transition of these pages and every change of the per-page table
 * happens under g_wx_lock. The fault path runs inside the kernel's
 * one-thread-at-a-time user-exception window with only general registers
 * usable; it may take g_wx_lock because no holder ever touches memory that can
 * fault while holding it (a fault by the holder itself is reported, not waited
 * on). The table keeps, per page, the backing address of the alias plus the
 * guest PROT_* bits; 0 = no backing. */
#define SSRNX_WX_SLOTS 128u
#define SSRNX_WX_GROUP_PAGES 16u
#define SSRNX_WX_ZERO_STEP ((uintptr_t)1u << 20)

typedef struct compat_SsrnxWxSlot {
    uintptr_t base;
    size_t pages;        /* 0 = free slot */
    uintptr_t *page;     /* backing address | guest PROT_*, 0 without backing */
    size_t cap;          /* entries in page */
} compat_SsrnxWxSlot;

static compat_SsrnxWxSlot compat_g_wx_slots[SSRNX_WX_SLOTS];
static uintptr_t compat_g_wx_lock;          /* TLS base of the holding thread, 0 when free */
static uint32_t compat_g_wx_idle_resolves;
/* Last failed host transition, for the crash report (the fault path cannot log). */
static struct { const char *step; uintptr_t addr; Result rc; uint32_t count; } compat_g_wx_failure;

__attribute__((target("general-regs-only")))
static uintptr_t compat_mm_wx_self(void) {
    uintptr_t self;
    __asm__ volatile("mrs %0, tpidrro_el0" : "=r"(self));
    return self;
}

/* False only when the calling thread already holds the lock (it faulted inside
 * a transition); the caller then must not proceed. */
__attribute__((target("general-regs-only")))
static bool compat_mm_wx_lock(void) {
    const uintptr_t self = compat_mm_wx_self();
    for (;;) {
        uintptr_t expected = 0;
        if (__atomic_compare_exchange_n(&compat_g_wx_lock, &expected, self, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
            return true;
        if (expected == self) return false;
        /* Never spin over a holder that may be waiting for this core. */
        svcSleepThread(10000);
    }
}

__attribute__((target("general-regs-only")))
static void compat_mm_wx_unlock(void) {
    __atomic_store_n(&compat_g_wx_lock, 0, __ATOMIC_RELEASE);
}

__attribute__((target("general-regs-only")))
static Result compat_mm_wx_failed(const char *step, uintptr_t addr, Result rc) {
    compat_g_wx_failure.step = step;
    compat_g_wx_failure.addr = addr;
    compat_g_wx_failure.rc = rc;
    ++compat_g_wx_failure.count;
    return rc;
}

bool ssrnx_wx_describe_failure(char *out, size_t cap) {
    if (!out || !cap || !compat_g_wx_failure.count) return false;
    snprintf(out, cap, "W^X transitions failed=%u last step=%s addr=0x%lx rc=0x%08x",
             (unsigned)compat_g_wx_failure.count, compat_g_wx_failure.step ? compat_g_wx_failure.step : "?",
             (unsigned long)compat_g_wx_failure.addr, (unsigned)compat_g_wx_failure.rc);
    return true;
}

/* Makes the committed alias pages of [lo, hi) RW. Requires g_wx_lock. */
__attribute__((target("general-regs-only")))
static Result compat_mm_wx_host_writable(uintptr_t lo, uintptr_t hi) {
    for (uintptr_t addr = lo; addr < hi;) {
        MemoryInfo info;
        u32 page_info;
        Result rc = svcQueryMemory(&info, &page_info, addr);
        if (R_FAILED(rc)) return compat_mm_wx_failed("query", addr, rc);
        uintptr_t end = (uintptr_t)(info.addr + info.size);
        if (end > hi) end = hi;
        if (info.type == MemType_ModuleCodeStatic) {
            rc = svcSetProcessMemoryPermission(envGetOwnProcessHandle(), (u64)addr, (u64)(end - addr), Perm_Rw);
            if (R_FAILED(rc)) return compat_mm_wx_failed("set-rw", addr, rc);
        } else if (info.type != MemType_ModuleCodeMutable) {
            return compat_mm_wx_failed("rw-state", addr, MAKERESULT(Module_Kernel, KernelError_InvalidMemoryState));
        }
        addr = end;
    }
    return 0;
}

/* Makes the committed alias pages of [lo, hi), backed by the contiguous host
 * pages starting at src, RX. Requires g_wx_lock. */
__attribute__((target("general-regs-only")))
static Result compat_mm_wx_host_executable(uintptr_t lo, uintptr_t hi, uintptr_t src) {
    bool remap = false, change = false;
    for (uintptr_t addr = lo; addr < hi;) {
        MemoryInfo info;
        u32 page_info;
        Result rc = svcQueryMemory(&info, &page_info, addr);
        if (R_FAILED(rc)) return compat_mm_wx_failed("query", addr, rc);
        uintptr_t end = (uintptr_t)(info.addr + info.size);
        if (end > hi) end = hi;
        if (info.type == MemType_ModuleCodeMutable) remap = true;
        else if (info.type != MemType_ModuleCodeStatic)
            return compat_mm_wx_failed("rx-state", addr, MAKERESULT(Module_Kernel, KernelError_InvalidMemoryState));
        else if ((info.perm & (Perm_R | Perm_W | Perm_X)) != Perm_Rx) change = true;
        addr = end;
    }
    const Handle self = envGetOwnProcessHandle();
    const u64 size = (u64)(hi - lo);
    if (remap) {
        Result rc = svcUnmapProcessCodeMemory(self, (u64)lo, (u64)src, size);
        if (R_FAILED(rc)) return compat_mm_wx_failed("unmap", lo, rc);
        rc = svcMapProcessCodeMemory(self, (u64)lo, (u64)src, size);
        if (R_FAILED(rc)) return compat_mm_wx_failed("remap", lo, rc);
        rc = svcSetProcessMemoryPermission(self, (u64)lo, size, Perm_Rx);
        if (R_FAILED(rc)) return compat_mm_wx_failed("set-rx", lo, rc);
    } else if (change) {
        for (uintptr_t addr = lo; addr < hi;) {
            MemoryInfo info;
            u32 page_info;
            Result rc = svcQueryMemory(&info, &page_info, addr);
            if (R_FAILED(rc)) return compat_mm_wx_failed("query", addr, rc);
            uintptr_t end = (uintptr_t)(info.addr + info.size);
            if (end > hi) end = hi;
            if ((info.perm & (Perm_R | Perm_W | Perm_X)) != Perm_Rx) {
                rc = svcSetProcessMemoryPermission(self, (u64)addr, (u64)(end - addr), Perm_Rx);
                if (R_FAILED(rc)) return compat_mm_wx_failed("set-rx", addr, rc);
            }
            addr = end;
        }
    } else {
        return 0;
    }
    armDCacheClean((void *)lo, (size_t)size);
    armICacheInvalidate((void *)lo, (size_t)size);
    return 0;
}

static bool compat_mm_wx_slot_acquire_locked(compat_SsrnxMapBlock *b) {
    if (b->wx_slot) return true;
    const size_t pages = b->pages;
    for (unsigned i = 0; i < SSRNX_WX_SLOTS; ++i) {
        compat_SsrnxWxSlot *slot = &compat_g_wx_slots[i];
        if (slot->pages) continue;
        uintptr_t *fresh = NULL;
        if (slot->cap < pages) {
            fresh = (uintptr_t *)calloc(pages, sizeof(*fresh));
            if (!fresh) return false;
        }
        (void)compat_mm_wx_lock();
        uintptr_t *old = NULL;
        if (fresh) {
            old = slot->page;
            slot->page = fresh;
            slot->cap = pages;
        } else {
            memset(slot->page, 0, pages * sizeof(*slot->page));
        }
        slot->base = b->base;
        slot->pages = pages;
        compat_mm_wx_unlock();
        free(old);
        b->wx_slot = i + 1u;
        SSRNX_LOGCI("VM", "W^X emulation base=%p len=%zu slot=%u", (void *)b->base, pages * SSRNX_MM_PAGE, i);
        return true;
    }
    static uint32_t reports;
    if (__atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 8u)
        SSRNX_LOGCW("VM", "W^X emulation table full base=%p len=%zu", (void *)b->base, pages * SSRNX_MM_PAGE);
    return false;
}

/* Requires g_wx_lock. */
static void compat_mm_wx_slot_release_wxlocked(compat_SsrnxMapBlock *b) {
    if (!b->wx_slot) return;
    compat_g_wx_slots[b->wx_slot - 1u].pages = 0;
    b->wx_slot = 0;
}

/* Table entry of page in b's slot, or NULL. Requires g_wx_lock. */
static uintptr_t *compat_mm_wx_block_entry_wxlocked(const compat_SsrnxMapBlock *b, uintptr_t page) {
    if (!b->wx_slot) return NULL;
    const compat_SsrnxWxSlot *slot = &compat_g_wx_slots[b->wx_slot - 1u];
    if (page < slot->base) return NULL;
    const size_t index = (size_t)((page - slot->base) / SSRNX_MM_PAGE);
    return index < slot->pages ? &slot->page[index] : NULL;
}

static inline uintptr_t compat_mm_chunk_source(const compat_SsrnxMapChunk *chunk, uintptr_t guest) {
    return (uintptr_t)chunk->owner->ptr + chunk->source_page * SSRNX_MM_PAGE + (guest - chunk->guest_base);
}

/* Records guest protection prot for the pages of [start, end); pages without
 * backing get 0. Requires g_wx_lock. */
static void compat_mm_wx_note_range_wxlocked(const compat_SsrnxMapBlock *b, uintptr_t start, uintptr_t end, int prot) {
    if (!b->wx_slot) return;
    for (uintptr_t page = start; page < end; page += SSRNX_MM_PAGE) {
        uintptr_t *entry = compat_mm_wx_block_entry_wxlocked(b, page);
        if (entry) *entry = 0;
    }
    for (const compat_SsrnxMapChunk *chunk = b->chunks; chunk; chunk = chunk->next) {
        const uintptr_t cstart = chunk->guest_base;
        const uintptr_t cend = cstart + chunk->pages * SSRNX_MM_PAGE;
        const uintptr_t lo = start > cstart ? start : cstart;
        const uintptr_t hi = end < cend ? end : cend;
        for (uintptr_t page = lo; page < hi; page += SSRNX_MM_PAGE) {
            uintptr_t *entry = compat_mm_wx_block_entry_wxlocked(b, page);
            if (entry) *entry = compat_mm_chunk_source(chunk, page) | (uintptr_t)(prot & 7);
        }
    }
}

/* Gives the backed pages of [start, end) the host permission their guest
 * protection prot calls for. Requires g_wx_lock. */
static Result compat_mm_wx_host_apply_wxlocked(const compat_SsrnxMapBlock *b, uintptr_t start, uintptr_t end, int prot) {
    const bool exec = (prot & SSRNX_MM_PROT_EXEC) != 0;
    /* Writable+executable pages keep their current host permission; the
     * fault path switches them on demand. */
    if (exec && (prot & SSRNX_MM_PROT_WRITE)) return 0;
    for (const compat_SsrnxMapChunk *chunk = b->chunks; chunk; chunk = chunk->next) {
        const uintptr_t cstart = chunk->guest_base;
        const uintptr_t cend = cstart + chunk->pages * SSRNX_MM_PAGE;
        const uintptr_t lo = start > cstart ? start : cstart;
        const uintptr_t hi = end < cend ? end : cend;
        if (lo >= hi) continue;
        const Result rc = exec ? compat_mm_wx_host_executable(lo, hi, compat_mm_chunk_source(chunk, lo))
                               : compat_mm_wx_host_writable(lo, hi);
        if (R_FAILED(rc)) return rc;
    }
    return 0;
}

static void compat_mm_wx_report(const char *what, uintptr_t start, uintptr_t end, Result rc) {
    static uint32_t reports;
    if (__atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 16u)
        SSRNX_LOGCE("VM", "W^X %s failed range=%p+%zu step=%s addr=%p rc=0x%08x", what, (void *)start,
                    (size_t)(end - start), compat_g_wx_failure.step ? compat_g_wx_failure.step : "?",
                    (void *)compat_g_wx_failure.addr, rc);
}

/* Applies the host side of a guest protection change on [first, first+count). */
static bool compat_mm_wx_apply_locked(compat_SsrnxMapBlock *b, size_t first, size_t count, int prot) {
    const bool exec = (prot & SSRNX_MM_PROT_EXEC) != 0;
    if (!exec && !b->wx_slot) return true;
    if (!compat_mm_wx_slot_acquire_locked(b)) return false;
    const uintptr_t start = b->base + first * SSRNX_MM_PAGE;
    const uintptr_t end = start + count * SSRNX_MM_PAGE;
    (void)compat_mm_wx_lock();
    compat_mm_wx_note_range_wxlocked(b, start, end, prot);
    const Result rc = compat_mm_wx_host_apply_wxlocked(b, start, end, prot);
    compat_mm_wx_unlock();
    if (R_FAILED(rc)) {
        compat_mm_wx_report("mprotect", start, end, rc);
        return false;
    }
    return true;
}

/* MADV_DONTNEED on committed anonymous pages of a W^X block: zero them through
 * RW, then restore the host permission of pages the guest keeps RX. */
static void compat_mm_wx_zero_range_locked(compat_SsrnxMapBlock *b, uintptr_t start, uintptr_t end) {
    for (const compat_SsrnxMapChunk *chunk = b->chunks; chunk; chunk = chunk->next) {
        const uintptr_t cstart = chunk->guest_base;
        const uintptr_t cend = cstart + chunk->pages * SSRNX_MM_PAGE;
        const uintptr_t lo = start > cstart ? start : cstart;
        const uintptr_t hi = end < cend ? end : cend;
        for (uintptr_t p = lo; p < hi;) {
            if (!compat_mm_bit(b->anon, (size_t)((p - b->base) / SSRNX_MM_PAGE))) { p += SSRNX_MM_PAGE; continue; }
            uintptr_t q = p + SSRNX_MM_PAGE;
            while (q < hi && q - p < SSRNX_WX_ZERO_STEP && compat_mm_bit(b->anon, (size_t)((q - b->base) / SSRNX_MM_PAGE)))
                q += SSRNX_MM_PAGE;
            (void)compat_mm_wx_lock();
            Result rc = compat_mm_wx_host_writable(p, q);
            if (R_SUCCEEDED(rc)) {
                memset((void *)p, 0, (size_t)(q - p));
                for (uintptr_t r = p; r < q && R_SUCCEEDED(rc);) {
                    const int prot = b->prot[(r - b->base) / SSRNX_MM_PAGE];
                    uintptr_t s = r + SSRNX_MM_PAGE;
                    while (s < q && b->prot[(s - b->base) / SSRNX_MM_PAGE] == prot) s += SSRNX_MM_PAGE;
                    if ((prot & SSRNX_MM_PROT_EXEC) && !(prot & SSRNX_MM_PROT_WRITE))
                        rc = compat_mm_wx_host_executable(r, s, compat_mm_chunk_source(chunk, r));
                    r = s;
                }
            }
            compat_mm_wx_unlock();
            if (R_FAILED(rc)) compat_mm_wx_report("madvise", p, q, rc);
            p = q;
        }
    }
}

/* Called from exception_entry.s for every permission or translation fault,
 * inside the kernel's user-exception window: no allocation, no FP/SIMD
 * registers. True resumes the faulting instruction. */
bool ssrnx_wx_fault_resolve(const void *frame_ptr) __attribute__((target("general-regs-only")));
bool ssrnx_wx_fault_resolve(const void *frame_ptr) {
    const ThreadExceptionFrameA64 *frame = (const ThreadExceptionFrameA64 *)frame_ptr;
    const u32 ec = frame->esr >> 26;
    if (ec != 0x20u && ec != 0x24u) return false;
    if (frame->esr & (1u << 10)) return false;                 /* FnV: FAR not valid */
    const bool fetch = ec == 0x20u;
    const bool store = !fetch && (frame->esr & (1u << 6)) != 0;  /* WnR */
    const uintptr_t page = (uintptr_t)frame->far & ~(SSRNX_MM_PAGE - 1u);
    if (!compat_mm_wx_lock()) return false;

    bool ok = false;
    const compat_SsrnxWxSlot *slot = NULL;
    size_t index = 0;
    for (unsigned i = 0; i < SSRNX_WX_SLOTS && !slot; ++i) {
        const compat_SsrnxWxSlot *s = &compat_g_wx_slots[i];
        if (!s->pages || page < s->base) continue;
        const size_t at = (size_t)((page - s->base) / SSRNX_MM_PAGE);
        /* A trimmed block's old slot may still cover reused VA: its entries are 0. */
        if (at < s->pages && s->page[at]) { slot = s; index = at; }
    }
    if (slot) {
        const uintptr_t entry = slot->page[index];
        const unsigned prot = (unsigned)(entry & 7u);
        MemoryInfo info;
        u32 page_info;
        if (R_SUCCEEDED(svcQueryMemory(&info, &page_info, page))) {
            const u32 need = fetch ? Perm_X : store ? Perm_W : Perm_R;
            if (info.type != MemType_Unmapped && (info.perm & need) == need) {
                /* Already switched (by another thread, or a stale TLB entry).
                 * Bound repeats so a fault this cannot explain is reported. */
                ok = __atomic_add_fetch(&compat_g_wx_idle_resolves, 1u, __ATOMIC_RELAXED) < 64u;
            } else if (fetch && (prot & SSRNX_MM_PROT_EXEC)) {
                /* Make the executable pages of this 64 KiB group with contiguous
                 * backing RX together: one remap instead of one fault each. */
                const uintptr_t src = entry & ~(SSRNX_MM_PAGE - 1u);
                const size_t group = index & ~(size_t)(SSRNX_WX_GROUP_PAGES - 1u);
                size_t lo = index, hi = index + 1u;
                while (lo > group) {
                    const uintptr_t e = slot->page[lo - 1u];
                    if (!(e & SSRNX_MM_PROT_EXEC) || (e & ~(SSRNX_MM_PAGE - 1u)) != src - (index - lo + 1u) * SSRNX_MM_PAGE) break;
                    --lo;
                }
                while (hi < slot->pages && hi < group + SSRNX_WX_GROUP_PAGES) {
                    const uintptr_t e = slot->page[hi];
                    if (!(e & SSRNX_MM_PROT_EXEC) || (e & ~(SSRNX_MM_PAGE - 1u)) != src + (hi - index) * SSRNX_MM_PAGE) break;
                    ++hi;
                }
                ok = R_SUCCEEDED(compat_mm_wx_host_executable(slot->base + lo * SSRNX_MM_PAGE, slot->base + hi * SSRNX_MM_PAGE,
                                                       src - (index - lo) * SSRNX_MM_PAGE));
                if (ok) __atomic_store_n(&compat_g_wx_idle_resolves, 0u, __ATOMIC_RELAXED);
            } else if (store && (prot & SSRNX_MM_PROT_WRITE)) {
                ok = R_SUCCEEDED(compat_mm_wx_host_writable(page, page + SSRNX_MM_PAGE));
                if (ok) __atomic_store_n(&compat_g_wx_idle_resolves, 0u, __ATOMIC_RELAXED);
            }
        }
    }
    compat_mm_wx_unlock();
    return ok;
}

static bool compat_mm_sparse_range_mapped_locked(const compat_SsrnxMapBlock *b, size_t first, size_t count) {
    if (!b || first > b->pages || count > b->pages - first) return false;
    for (size_t i = first; i < first + count; ++i) if (!compat_mm_bit(b->mapped, i)) return false;
    return true;
}

static bool compat_mm_sparse_commit_run_locked(compat_SsrnxMapBlock *b, size_t first, size_t count, int prot) {
    if (!b || !b->sparse || count == 0) return true;
    const size_t size = count * SSRNX_MM_PAGE;
    const uintptr_t guest = b->base + first * SSRNX_MM_PAGE;

    compat_SsrnxMapBacking *owner = (compat_SsrnxMapBacking *)calloc(1, sizeof(*owner));
    compat_SsrnxMapChunk *chunk = (compat_SsrnxMapChunk *)calloc(1, sizeof(*chunk));
    if (!owner || !chunk) { free(owner); free(chunk); return false; }
    owner->ptr = memalign(SSRNX_MM_PAGE, size);
    owner->pages = count;
    if (!owner->ptr) { free(owner); free(chunk); return false; }
    memset(owner->ptr, 0, size);

    if (!compat_mm_sparse_chunk_map(chunk, owner, 0, guest, count)) {
        free(owner->ptr); free(owner); free(chunk); return false;
    }
    chunk->next = b->chunks;
    b->chunks = chunk;
    for (size_t i = first; i < first + count; ++i) {
        compat_mm_bit_set(b->committed, i, true);
        b->prot[i] = (uint8_t)(prot & 7);
    }
    return true;
}

static bool compat_mm_sparse_set_existing_prot_locked(compat_SsrnxMapBlock *b, size_t first, size_t count, int prot) {
    if (!b || !b->sparse || count == 0) return true;
    if (first > b->pages || count > b->pages - first) return false;
    /* See mm_sparse_commit_run_locked: protection changes after commitment are
     * advisory after initial commitment, matching SSRNX's established heap-backed mmap semantics. */
    for (size_t i = first; i < first + count; ++i) b->prot[i] = (uint8_t)(prot & 7);
    return true;
}

static bool compat_mm_sparse_mprotect_locked(compat_SsrnxMapBlock *b, size_t first, size_t count, int prot,
                                      size_t *out_new_bytes) {
    if (!b || !b->sparse || count == 0) return true;
    if (!compat_mm_sparse_range_mapped_locked(b, first, count)) return false;
    size_t newly = 0;
    if (prot != 0) {
        size_t page = first, limit = first + count;
        while (page < limit) {
            if (compat_mm_bit(b->committed, page)) { ++page; continue; }
            size_t end = page + 1u;
            while (end < limit && !compat_mm_bit(b->committed, end)) ++end;
            if (!compat_mm_sparse_commit_run_locked(b, page, end - page, prot)) return false;
            newly += (end - page) * SSRNX_MM_PAGE;
            page = end;
        }
    }
    if (!compat_mm_sparse_set_existing_prot_locked(b, first, count, prot)) return false;
    if (!compat_mm_wx_apply_locked(b, first, count, prot)) return false;
    if (out_new_bytes) *out_new_bytes += newly;
    return true;
}

static bool compat_mm_sparse_decommit_range_body_locked(compat_SsrnxMapBlock *b, size_t first, size_t count) {
    const uintptr_t start = b->base + first * SSRNX_MM_PAGE;
    const uintptr_t end = start + count * SSRNX_MM_PAGE;
    compat_SsrnxMapChunk **link = &b->chunks;

    while (*link) {
        compat_SsrnxMapChunk *chunk = *link;
        const uintptr_t cstart = chunk->guest_base;
        const uintptr_t cend = cstart + chunk->pages * SSRNX_MM_PAGE;
        if (cend <= start || cstart >= end) { link = &chunk->next; continue; }

        const uintptr_t lo = start > cstart ? start : cstart;
        const uintptr_t hi = end < cend ? end : cend;
        for (uintptr_t page = lo; page < hi; page += SSRNX_MM_PAGE) {
            uintptr_t *entry = compat_mm_wx_block_entry_wxlocked(b, page);
            if (entry) *entry = 0;
        }
        const size_t cut_first = (size_t)((lo - cstart) / SSRNX_MM_PAGE);
        const size_t cut_pages = (size_t)((hi - lo) / SSRNX_MM_PAGE);
        const size_t left_pages = cut_first;
        const size_t right_first = cut_first + cut_pages;
        const size_t right_pages = chunk->pages - right_first;

        if (cut_pages == chunk->pages) {
            if (!compat_mm_sparse_chunk_unmap(chunk)) return false;
            *link = chunk->next;
            free(chunk);
        } else {
            compat_SsrnxMapChunk *right = NULL;
            if (left_pages && right_pages) {
                right = (compat_SsrnxMapChunk *)calloc(1, sizeof(*right));
                if (!right) return false;
            }

            compat_SsrnxMapBacking *owner = chunk->owner;
            void *source = (uint8_t *)owner->ptr + (chunk->source_page + cut_first) * SSRNX_MM_PAGE;
            Result rc = svcUnmapProcessCodeMemory(envGetOwnProcessHandle(), (u64)lo,
                                                  (u64)(uintptr_t)source,
                                                  cut_pages * SSRNX_MM_PAGE);
            if (R_FAILED(rc)) {
                SSRNX_LOGCE("VM", "process-code alias partial unmap failed guest=%p source=%p len=%zu rc=0x%08x",
                            (void *)lo, source, cut_pages * SSRNX_MM_PAGE, rc);
                free(right);
                return false;
            }

            if (left_pages && right_pages) {
                right->guest_base = cstart + right_first * SSRNX_MM_PAGE;
                right->owner = owner;
                right->source_page = chunk->source_page + right_first;
                right->pages = right_pages;
                right->next = chunk->next;
                ++owner->refs;

                chunk->pages = left_pages;
                chunk->next = right;
                link = &right->next;
            } else if (left_pages) {
                chunk->pages = left_pages;
                link = &chunk->next;
            } else {
                chunk->guest_base = cstart + right_first * SSRNX_MM_PAGE;
                chunk->source_page += right_first;
                chunk->pages = right_pages;
                link = &chunk->next;
            }
        }

        for (size_t p = 0; p < cut_pages; ++p) {
            const size_t bi = (size_t)((lo - b->base) / SSRNX_MM_PAGE) + p;
            compat_mm_bit_set(b->committed, bi, false);
            b->prot[bi] = 0;
        }
    }
    return true;
}

/* Unmaps the backing of [first, first+count). W^X pages are unmapped under
 * g_wx_lock so the fault path never transitions a page being unmapped. */
static bool compat_mm_sparse_decommit_range_locked(compat_SsrnxMapBlock *b, size_t first, size_t count) {
    if (!b || !b->sparse || count == 0) return true;
    const bool wx = b->wx_slot != 0;
    if (wx) (void)compat_mm_wx_lock();
    const bool ok = compat_mm_sparse_decommit_range_body_locked(b, first, count);
    if (wx) compat_mm_wx_unlock();
    return ok;
}
#endif

/* True when every page of [start,end) lies inside SSRNX-owned blocks. */
static bool compat_mm_range_owned_locked(uintptr_t start, uintptr_t end) {
    size_t i = compat_mm_first_above_locked(start);
    uintptr_t cur = start;
    while (cur < end) {
        if (i >= compat_g_map_count || compat_g_map_blocks[i]->base > cur) return false;
        cur = compat_mm_block_end(compat_g_map_blocks[i++]);
    }
    return true;
}

/* Fill [dst,dst+len) from a file at off; bytes past EOF read as zero. */
static int compat_mm_fill_from_file(uint8_t *dst, size_t len, int fd, int64_t off) {
    memset(dst, 0, len);
    size_t done = 0;
    while (done < len) {
        const int64_t pos = off + (int64_t)done;
        if (pos < off) { errno = EOVERFLOW; return -1; }
        const ssize_t n = compat_ssrnx_bionic_pread(fd, dst + done, len - done, pos);
        if (n < 0) return -1;
        if (n == 0) break;
        done += (size_t)n;
    }
    return 0;
}

static int compat_ssrnx_bionic_mprotect(void *addr, size_t len, int prot) {
    const uintptr_t caller = SSRNX_MEMTRACE_CALLER();
    if (((uintptr_t)addr & (SSRNX_MM_PAGE - 1u)) != 0) {
        errno = EINVAL;
        if (compat_ssrnx_memtrace_enabled()) {
            SSRNX_LOGCI("VMTRACE", "mprotect addr=%p len=%zu prot=%#x rc=-1 errno=%d caller=%p tid=%d tp=%p",
                        addr, len, prot, errno, (void *)caller, ssrnx_bionic_gettid(), ssrnx_bionic_tls_current_base());
            compat_ssrnx_memtrace_flush();
        }
        return -1;
    }
    if (!addr && len) {
        errno = ENOMEM;
        if (compat_ssrnx_memtrace_enabled()) {
            SSRNX_LOGCI("VMTRACE", "mprotect addr=%p len=%zu prot=%#x rc=-1 errno=%d caller=%p tid=%d tp=%p",
                        addr, len, prot, errno, (void *)caller, ssrnx_bionic_gettid(), ssrnx_bionic_tls_current_base());
            compat_ssrnx_memtrace_flush();
        }
        return -1;
    }  /* page zero is never mapped */
#ifdef __SWITCH__
    if (len) {
        const uintptr_t start = (uintptr_t)addr;
        if (len > UINTPTR_MAX - start - (SSRNX_MM_PAGE - 1u)) { errno = ENOMEM; return -1; }
        const uintptr_t end = (start + len + SSRNX_MM_PAGE - 1u) & ~(SSRNX_MM_PAGE - 1u);
        bool sparse_seen = false, ok = true;
        int failure = ENOMEM;
        size_t newly_committed = 0;
        pthread_mutex_lock(&compat_g_map_lock);
        if ((prot & SSRNX_MM_PROT_EXEC) && !compat_mm_range_owned_locked(start, end)) {
            ok = false; failure = EACCES;
        }
        for (size_t i = compat_mm_first_above_locked(start); ok && i < compat_g_map_count && compat_g_map_blocks[i]->base < end; ++i) {
            compat_SsrnxMapBlock *b = compat_g_map_blocks[i];
            if (!b->sparse) {
                if (prot & SSRNX_MM_PROT_EXEC) {
                    static uint32_t reports;
                    if (__atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 8u)
                        SSRNX_LOGCW("VM", "mprotect PROT_EXEC rejected on heap-backed mapping base=%p len=%zu caller=%p errno=%d",
                                    (void *)b->base, b->pages * SSRNX_MM_PAGE, (void *)caller, EACCES);
                    ok = false;
                    failure = EACCES;
                    break;
                }
                continue;
            }
            sparse_seen = true;
            const uintptr_t lo = start > b->base ? start : b->base;
            const uintptr_t hi = end < compat_mm_block_end(b) ? end : compat_mm_block_end(b);
            const size_t first = (size_t)((lo - b->base) / SSRNX_MM_PAGE);
            const size_t count = (size_t)((hi - lo) / SSRNX_MM_PAGE);
            if (!compat_mm_sparse_mprotect_locked(b, first, count, prot, &newly_committed)) { ok = false; break; }
        }
        pthread_mutex_unlock(&compat_g_map_lock);
        if (sparse_seen || !ok || (prot & SSRNX_MM_PROT_EXEC)) {
            if (!sparse_seen && ok) { ok = false; failure = EACCES; }
            if (!ok) {
                errno = failure;
                compat_ssrnx_memory_failure(failure == EACCES ? "mprotect-exec" : "mprotect-commit", len, caller, failure);
                if (compat_ssrnx_memtrace_enabled()) {
                    SSRNX_LOGCI("VMTRACE", "mprotect addr=%p len=%zu prot=%#x rc=-1 errno=%d sparse=%d caller=%p tid=%d tp=%p",
                                addr, len, prot, errno, sparse_seen ? 1 : 0, (void *)caller,
                                ssrnx_bionic_gettid(), ssrnx_bionic_tls_current_base());
                    compat_ssrnx_memtrace_flush();
                }
                return -1;
            }
            if (compat_ssrnx_memtrace_enabled()) {
                SSRNX_LOGCI("VMTRACE", "mprotect addr=%p len=%zu prot=%#x rc=0 errno=0 sparse=1 backend=process-code-alias committed=%zu caller=%p tid=%d tp=%p",
                            addr, len, prot, newly_committed, (void *)caller,
                            ssrnx_bionic_gettid(), ssrnx_bionic_tls_current_base());
                compat_ssrnx_memtrace_flush();
            }
            return 0;
        }
    }
#else
    (void)prot;
#endif
    if (compat_ssrnx_memtrace_enabled()) {
        SSRNX_LOGCI("VMTRACE", "mprotect addr=%p len=%zu prot=%#x rc=0 errno=0 caller=%p tid=%d tp=%p",
                    addr, len, prot, (void *)caller, ssrnx_bionic_gettid(), ssrnx_bionic_tls_current_base());
        compat_ssrnx_memtrace_flush();
    }
    return 0;
}

static int compat_ssrnx_bionic_madvise(void *addr, size_t len, int advice) {
    const uintptr_t caller = SSRNX_MEMTRACE_CALLER();
    const uintptr_t start = (uintptr_t)addr;
    if ((start & (SSRNX_MM_PAGE - 1u)) != 0) { errno = EINVAL; goto madvise_fail; }
    if (len == 0) goto madvise_ok;
    if (len > UINTPTR_MAX - start - (SSRNX_MM_PAGE - 1u)) { errno = EINVAL; goto madvise_fail; }
    if (advice != SSRNX_MM_MADV_DONTNEED && advice != SSRNX_MM_MADV_REMOVE) goto madvise_ok;
    const uintptr_t end = (start + len + SSRNX_MM_PAGE - 1u) & ~(SSRNX_MM_PAGE - 1u);
    pthread_mutex_lock(&compat_g_map_lock);
    for (size_t i = compat_mm_first_above_locked(start); i < compat_g_map_count && compat_g_map_blocks[i]->base < end; ++i) {
        compat_SsrnxMapBlock *b = compat_g_map_blocks[i];
        const uintptr_t lo = start > b->base ? start : b->base;
        const uintptr_t hi = end < compat_mm_block_end(b) ? end : compat_mm_block_end(b);
#ifdef __SWITCH__
        /* Pages of a W^X block may be RX right now: zero them through RW. */
        if (b->sparse && b->wx_slot) { compat_mm_wx_zero_range_locked(b, lo, hi); continue; }
#endif
        for (uintptr_t page = lo; page < hi; page += SSRNX_MM_PAGE) {
            const size_t pi = (size_t)((page - b->base) / SSRNX_MM_PAGE);
            if (!compat_mm_bit(b->mapped, pi) || !compat_mm_bit(b->anon, pi)) continue;
#ifdef __SWITCH__
            /* Sparse committed pages are process-code aliases; discard contents
             * in place and retain the guest-visible protection metadata. */
            if (b->sparse && !compat_mm_bit(b->committed, pi)) continue;
#endif
            memset((void *)page, 0, SSRNX_MM_PAGE);
        }
    }
    pthread_mutex_unlock(&compat_g_map_lock);
 madvise_ok:
    if (compat_ssrnx_memtrace_enabled()) {
        SSRNX_LOGCI("VMTRACE", "madvise addr=%p len=%zu advice=%d rc=0 errno=0 caller=%p tid=%d tp=%p",
                    addr, len, advice, (void *)caller, ssrnx_bionic_gettid(), ssrnx_bionic_tls_current_base());
        compat_ssrnx_memtrace_flush();
    }
    return 0;
 madvise_fail:
    if (compat_ssrnx_memtrace_enabled()) {
        SSRNX_LOGCI("VMTRACE", "madvise addr=%p len=%zu advice=%d rc=-1 errno=%d caller=%p tid=%d tp=%p",
                    addr, len, advice, errno, (void *)caller, ssrnx_bionic_gettid(), ssrnx_bionic_tls_current_base());
        compat_ssrnx_memtrace_flush();
    }
    return -1;
}


/* Run body for every block page of [start,end), starting at block index first. */
#define MM_FOR_EACH_PAGE(first, start, end, b, pi, body)                                   \
    for (size_t mm_i_ = (first); mm_i_ < compat_g_map_count && compat_g_map_blocks[mm_i_]->base < (end); ++mm_i_) { \
        compat_SsrnxMapBlock *b = compat_g_map_blocks[mm_i_];                                             \
        const uintptr_t mm_lo_ = (start) > b->base ? (start) : b->base;                     \
        const uintptr_t mm_hi_ = (end) < compat_mm_block_end(b) ? (end) : compat_mm_block_end(b);         \
        for (uintptr_t mm_p_ = mm_lo_; mm_p_ < mm_hi_; mm_p_ += SSRNX_MM_PAGE) {            \
            const size_t pi = (size_t)((mm_p_ - b->base) / SSRNX_MM_PAGE);                  \
            body                                                                            \
        }                                                                                   \
    }

static void *compat_ssrnx_bionic_mmap(void *addr, size_t len, int prot, int flags, int fd, int64_t off) {
    const uintptr_t caller = SSRNX_MEMTRACE_CALLER();
    void *const failed = (void *)(intptr_t)-1;
    const int type = flags & SSRNX_MM_MAP_TYPE;
    const bool anon = (flags & SSRNX_MM_MAP_ANONYMOUS) != 0;
    const bool fixed = (flags & (SSRNX_MM_MAP_FIXED | SSRNX_MM_MAP_FIXED_NOREPLACE)) != 0;
    if (len == 0 || off < 0 || ((uint64_t)off & (SSRNX_MM_PAGE - 1u)) != 0 ||
        (type != SSRNX_MM_MAP_SHARED && type != SSRNX_MM_MAP_PRIVATE && type != SSRNX_MM_MAP_SHARED_VALIDATE)) {
        errno = EINVAL; goto mmap_fail;
    }
    if (!anon && fd < 0) { errno = EBADF; goto mmap_fail; }
    if (len > SIZE_MAX - (SSRNX_MM_PAGE - 1u)) { errno = ENOMEM; goto mmap_fail; }
    const size_t alen = (len + SSRNX_MM_PAGE - 1u) & ~(size_t)(SSRNX_MM_PAGE - 1u);
    const bool private_anon = anon && type == SSRNX_MM_MAP_PRIVATE;
    if (!anon) {
        static uint32_t reports;
        if (__atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 64u)
            SSRNX_LOGCI("VM", "file mmap request fd=%d len=%zu off=%" PRId64 " prot=%#x flags=%#x caller=%p",
                        fd, len, off, prot, flags, (void *)caller);
    }
    if (!anon && type != SSRNX_MM_MAP_PRIVATE && (prot & SSRNX_MM_PROT_WRITE)) {
        /* Heap snapshots cannot write back or stay coherent with a file. */
        static uint32_t reports;
        if (__atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 8u)
            SSRNX_LOGCW("VM", "shared writable file mmap unavailable fd=%d len=%zu off=%" PRId64 " prot=%#x flags=%#x caller=%p",
                        fd, len, off, prot, flags, (void *)caller);
        errno = EOPNOTSUPP; goto mmap_fail;
    }
#ifdef __SWITCH__
    if (type != SSRNX_MM_MAP_PRIVATE && (prot & SSRNX_MM_PROT_EXEC)) {
        errno = EACCES; goto mmap_fail;
    }
#endif

    if (fixed) {
        const uintptr_t start = (uintptr_t)addr;
        if ((start & (SSRNX_MM_PAGE - 1u)) != 0) { errno = EINVAL; goto mmap_fail; }
        if (alen > UINTPTR_MAX - start) { errno = ENOMEM; goto mmap_fail; }
        const uintptr_t end = start + alen;
        pthread_mutex_lock(&compat_g_map_lock);
        if (!compat_mm_range_owned_locked(start, end)) {
            /* Outside SSRNX-owned blocks there is nothing Horizon lets us map. */
            pthread_mutex_unlock(&compat_g_map_lock);
            errno = ENOMEM; goto mmap_fail;
        }
        const size_t first = compat_mm_first_above_locked(start);
        if (flags & SSRNX_MM_MAP_FIXED_NOREPLACE) {
            bool busy = false;
            MM_FOR_EACH_PAGE(first, start, end, b, pi, { if (compat_mm_bit(b->mapped, pi)) busy = true; })
            if (busy) { pthread_mutex_unlock(&compat_g_map_lock); errno = EEXIST; goto mmap_fail; }
        }
        int rc = 0, saved = 0;
#ifdef __SWITCH__
        bool sparse_overlap = false;
        MM_FOR_EACH_PAGE(first, start, end, b, pi, { (void)pi; if (b->sparse) sparse_overlap = true; })
        if (sparse_overlap) {
            for (size_t si = first; si < compat_g_map_count && compat_g_map_blocks[si]->base < end; ++si) {
                compat_SsrnxMapBlock *b = compat_g_map_blocks[si];
                const uintptr_t lo = start > b->base ? start : b->base;
                const uintptr_t hi = end < compat_mm_block_end(b) ? end : compat_mm_block_end(b);
                if (!b->sparse) {
                    if (anon) memset((void *)lo, 0, hi - lo);
                    else if (compat_mm_fill_from_file((uint8_t *)lo, hi - lo, fd, off + (int64_t)(lo - start)) != 0) {
                        rc = -1; saved = errno; break;
                    }
                    continue;
                }
                const size_t fp = (size_t)((lo - b->base) / SSRNX_MM_PAGE);
                const size_t pc = (size_t)((hi - lo) / SSRNX_MM_PAGE);
                if (!compat_mm_sparse_decommit_range_locked(b, fp, pc)) { rc = -1; saved = ENOMEM; break; }
                for (size_t j = fp; j < fp + pc; ++j) {
                    if (!compat_mm_bit(b->mapped, j)) { compat_mm_bit_set(b->mapped, j, true); ++b->live; }
                    compat_mm_bit_set(b->anon, j, private_anon);
                    b->prot[j] = 0;
                }
                /* Anonymous PROT_NONE (the Linux decommit idiom) stays uncommitted
                 * and reads as zero once made accessible. Everything else is
                 * committed RW, filled, then given its protection, so file
                 * contents can be written into pages that end up executable. */
                if (anon && prot == 0) continue;
                if (!compat_mm_sparse_mprotect_locked(b, fp, pc, 3, NULL)) { rc = -1; saved = ENOMEM; break; }
                if (!anon && compat_mm_fill_from_file((uint8_t *)lo, hi - lo, fd, off + (int64_t)(lo - start)) != 0) {
                    rc = -1; saved = errno; break;
                }
                if (prot != 3 && !compat_mm_sparse_mprotect_locked(b, fp, pc, prot, NULL)) { rc = -1; saved = ENOMEM; break; }
            }
        } else
#endif
        {
            if (anon) memset(addr, 0, alen);
            else if ((rc = compat_mm_fill_from_file((uint8_t *)addr, alen, fd, off)) != 0) saved = errno;
        }
        if (rc == 0) {
            MM_FOR_EACH_PAGE(first, start, end, b, pi, {
                if (!compat_mm_bit(b->mapped, pi)) { compat_mm_bit_set(b->mapped, pi, true); ++b->live; }
                compat_mm_bit_set(b->anon, pi, private_anon);
                compat_mm_page_mark_committed_locked(b, pi);
                b->prot[pi] = (uint8_t)(prot & 7);
            })
#ifdef __SWITCH__
            for (size_t ri = first; ri < compat_g_map_count && compat_g_map_blocks[ri]->base < end; ++ri)
                if (compat_g_map_blocks[ri]->sparse && compat_g_map_blocks[ri]->live)
                    compat_mm_sparse_compact_block_locked(compat_g_map_blocks[ri]);
#endif
        }
        pthread_mutex_unlock(&compat_g_map_lock);
        if (rc != 0) { errno = saved; goto mmap_fail; }
        if (compat_ssrnx_memtrace_enabled()) {
            SSRNX_LOGCI("VMTRACE",
                        "mmap addr=%p len=%zu prot=%#x flags=%#x fd=%d off=%" PRId64 " result=%p errno=0 caller=%p tid=%d pthread=%p tp=%p",
                        addr, len, prot, flags, fd, off, addr, (void *)caller,
                        ssrnx_bionic_gettid(), (void *)(uintptr_t)pthread_self(), ssrnx_bionic_tls_current_base());
            compat_ssrnx_memtrace_flush();
        }
        if (!anon && ssrnx_file_contract_mmap) ssrnx_file_contract_mmap(fd, len, off, addr, 0);
        return addr;
    }

    /* Without MAP_FIXED the address is only a hint, which Linux may ignore. */
    const size_t pages = alen / SSRNX_MM_PAGE;
    const size_t bm_bytes = (pages + 7u) / 8u;
    compat_SsrnxMapBlock *b = (compat_SsrnxMapBlock *)calloc(1, sizeof(*b) + 3u * bm_bytes + pages);
    void *p = NULL;
    bool sparse = false;
    if (!b) { errno = ENOMEM; goto mmap_fail; }
    b->mapped = (uint8_t *)(b + 1);
    b->anon = b->mapped + bm_bytes;
    b->committed = b->anon + bm_bytes;
    b->prot = b->committed + bm_bytes;
#ifdef __SWITCH__
    /* A private RW mapping can become a code cache through later mprotect.
     * Its address cannot move then, so reserve an alias-capable VA now. */
    if (type == SSRNX_MM_MAP_PRIVATE) {
        p = ssrnx_vm_reserve(alen, SSRNX_MM_PAGE, &b->reservation);
        if (!p || !b->reservation) {
            static uint32_t reports;
            if (__atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 16u)
                SSRNX_LOGCW("VMTRACE", "sparse reservation failed len=%zu caller=%p", len, (void *)caller);
            free(b);
            errno = ENOMEM;
            goto mmap_fail;
        }
        b->sparse = true;
        sparse = true;
    } else
#endif
    if (compat_ssrnx_bionic_posix_memalign(&p, SSRNX_MM_PAGE, alen) != 0) {
        free(b);
        errno = ENOMEM; goto mmap_fail;
    }
    if (!sparse && anon) memset(p, 0, alen);
    else if (!anon && !sparse && compat_mm_fill_from_file((uint8_t *)p, alen, fd, off) != 0) {
        const int saved = errno;
#ifdef __SWITCH__
        if (b->sparse && b->reservation) {
            ssrnx_vm_release(b->reservation);
        } else
#endif
        free(p);
        free(b);
        errno = saved; goto mmap_fail;
    }
    b->base = (uintptr_t)p;
    b->pages = pages;
    b->live = pages;
    for (size_t i = 0; i < pages; ++i) compat_mm_bit_set(b->mapped, i, true);
    if (private_anon) for (size_t i = 0; i < pages; ++i) compat_mm_bit_set(b->anon, i, true);
    if (!sparse) for (size_t i = 0; i < pages; ++i) compat_mm_bit_set(b->committed, i, true);
    memset(b->prot, sparse ? 0 : (prot & 7), pages);
    pthread_mutex_lock(&compat_g_map_lock);
    const bool inserted = compat_mm_insert_locked(b);
#ifdef __SWITCH__
    /* Private files must retain their bytes even when initially PROT_NONE. */
    if (inserted && sparse && (prot != 0 || !anon)) {
        bool committed = compat_mm_sparse_mprotect_locked(b, 0, pages, 3, NULL);
        int saved = committed ? 0 : ENOMEM;
        if (committed && !anon && compat_mm_fill_from_file((uint8_t *)p, alen, fd, off) != 0) {
            committed = false;
            saved = errno;
        }
        if (committed && prot != 3 && !compat_mm_sparse_mprotect_locked(b, 0, pages, prot, NULL)) {
            committed = false;
            saved = ENOMEM;
        }
        if (!committed) {
            compat_mm_remove_locked(compat_mm_first_above_locked(b->base));
            pthread_mutex_unlock(&compat_g_map_lock);
            errno = saved;
            goto mmap_fail;
        }
    }
#endif
    pthread_mutex_unlock(&compat_g_map_lock);
    if (!inserted) {
#ifdef __SWITCH__
        if (b->sparse && b->reservation) {
            ssrnx_vm_release(b->reservation);
        } else
#endif
        free(p);
        free(b); errno = ENOMEM; goto mmap_fail;
    }
    if (compat_ssrnx_memtrace_enabled()) {
        SSRNX_LOGCI("VMTRACE",
                    "mmap addr=%p len=%zu prot=%#x flags=%#x fd=%d off=%" PRId64 " result=%p errno=0 sparse=%d region=%s caller=%p tid=%d pthread=%p tp=%p",
                    addr, len, prot, flags, fd, off, p, sparse ? 1 : 0, sparse ? "code-aslr" : "heap", (void *)caller,
                    ssrnx_bionic_gettid(), (void *)(uintptr_t)pthread_self(), ssrnx_bionic_tls_current_base());
        compat_ssrnx_memtrace_flush();
    }
    if (!anon && ssrnx_file_contract_mmap) ssrnx_file_contract_mmap(fd, len, off, p, 0);
    return p;
 mmap_fail:
    compat_ssrnx_memory_failure("mmap", len, caller, errno);
    if (compat_ssrnx_memtrace_enabled()) {
        SSRNX_LOGCI("VMTRACE",
                    "mmap addr=%p len=%zu prot=%#x flags=%#x fd=%d off=%" PRId64 " result=%p errno=%d caller=%p tid=%d pthread=%p tp=%p",
                    addr, len, prot, flags, fd, off, failed, errno, (void *)caller,
                    ssrnx_bionic_gettid(), (void *)(uintptr_t)pthread_self(), ssrnx_bionic_tls_current_base());
        compat_ssrnx_memtrace_flush();
    }
    if (!anon && fd >= 0 && ssrnx_file_contract_mmap) {
        const int saved = errno;
        ssrnx_file_contract_mmap(fd, len, off, failed, saved);
        errno = saved;
    }
    return failed;
}

static int compat_ssrnx_bionic_munmap(void *addr, size_t len);

#define SSRNX_MREMAP_MAYMOVE   1
#define SSRNX_MREMAP_FIXED     2
#define SSRNX_MREMAP_DONTUNMAP 4

static void *compat_ssrnx_bionic_mremap(void *old_addr, size_t old_size, size_t new_size, int flags, ...) {
    void *const failed = (void *)(intptr_t)-1;
    const uintptr_t start = (uintptr_t)old_addr;
    if (!old_addr || (start & (SSRNX_MM_PAGE - 1u)) != 0 || old_size == 0 || new_size == 0) {
        errno = EINVAL; return failed;
    }
    if (flags & ~(SSRNX_MREMAP_MAYMOVE | SSRNX_MREMAP_FIXED | SSRNX_MREMAP_DONTUNMAP)) {
        errno = EINVAL; return failed;
    }
    if ((flags & SSRNX_MREMAP_FIXED) && !(flags & SSRNX_MREMAP_MAYMOVE)) {
        errno = EINVAL; return failed;
    }
    /* DONTUNMAP leaves two mappings aliasing the same physical pages. SSRNX's
     * ownership model does not currently represent shared anonymous backing. */
    if (flags & SSRNX_MREMAP_DONTUNMAP) { errno = ENOTSUP; return failed; }

    if (old_size > SIZE_MAX - (SSRNX_MM_PAGE - 1u) || new_size > SIZE_MAX - (SSRNX_MM_PAGE - 1u)) {
        errno = ENOMEM; return failed;
    }
    const size_t old_len = (old_size + SSRNX_MM_PAGE - 1u) & ~(size_t)(SSRNX_MM_PAGE - 1u);
    const size_t new_len = (new_size + SSRNX_MM_PAGE - 1u) & ~(size_t)(SSRNX_MM_PAGE - 1u);
    if (old_len > UINTPTR_MAX - start) { errno = EINVAL; return failed; }

    int first_prot = 3;
    bool valid = true;
    pthread_mutex_lock(&compat_g_map_lock);
    const size_t bi = compat_mm_first_above_locked(start);
    compat_SsrnxMapBlock *b = bi < compat_g_map_count ? compat_g_map_blocks[bi] : NULL;
    if (!b || start < b->base || start + old_len > compat_mm_block_end(b)) valid = false;
    if (valid) {
        const size_t first = (size_t)((start - b->base) / SSRNX_MM_PAGE);
        const size_t pages = old_len / SSRNX_MM_PAGE;
        for (size_t i = 0; i < pages; ++i) {
            if (!compat_mm_bit(b->mapped, first + i)) { valid = false; break; }
        }
        if (valid && pages) first_prot = b->prot[first] & 7;
    }
    pthread_mutex_unlock(&compat_g_map_lock);
    if (!valid) { errno = EFAULT; return failed; }

    if (new_len == old_len) return old_addr;
    if (new_len < old_len) {
        if (compat_ssrnx_bionic_munmap((uint8_t *)old_addr + new_len, old_len - new_len) != 0) return failed;
        return old_addr;
    }

    if (!(flags & SSRNX_MREMAP_MAYMOVE)) { errno = ENOMEM; return failed; }

    void *target = NULL;
    if (flags & SSRNX_MREMAP_FIXED) {
        va_list ap; va_start(ap, flags); target = va_arg(ap, void *); va_end(ap);
        if (!target || ((uintptr_t)target & (SSRNX_MM_PAGE - 1u)) != 0) { errno = EINVAL; return failed; }
    }
    const int mmap_flags = SSRNX_MM_MAP_PRIVATE | SSRNX_MM_MAP_ANONYMOUS |
                           ((flags & SSRNX_MREMAP_FIXED) ? SSRNX_MM_MAP_FIXED : 0);
    void *moved = compat_ssrnx_bionic_mmap(target, new_len, 3, mmap_flags, -1, 0);
    if (moved == failed) return failed;
    memcpy(moved, old_addr, old_len);
    if (first_prot != 3 && compat_ssrnx_bionic_mprotect(moved, old_len, first_prot) != 0) {
        const int saved = errno; (void)compat_ssrnx_bionic_munmap(moved, new_len); errno = saved; return failed;
    }
    if (compat_ssrnx_bionic_munmap(old_addr, old_len) != 0) {
        const int saved = errno; (void)compat_ssrnx_bionic_munmap(moved, new_len); errno = saved; return failed;
    }
    return moved;
}

static int compat_ssrnx_bionic_munmap(void *addr, size_t len) {
    const uintptr_t caller = SSRNX_MEMTRACE_CALLER();
    const uintptr_t start = (uintptr_t)addr;
    if ((start & (SSRNX_MM_PAGE - 1u)) != 0 || len == 0 ||
        len > UINTPTR_MAX - start - (SSRNX_MM_PAGE - 1u)) {
        errno = EINVAL;
        if (compat_ssrnx_memtrace_enabled()) {
            SSRNX_LOGCI("VMTRACE", "munmap addr=%p len=%zu rc=-1 errno=%d caller=%p tid=%d tp=%p",
                        addr, len, errno, (void *)caller, ssrnx_bionic_gettid(), ssrnx_bionic_tls_current_base());
            compat_ssrnx_memtrace_flush();
        }
        return -1;
    }
    const uintptr_t end = (start + len + SSRNX_MM_PAGE - 1u) & ~(SSRNX_MM_PAGE - 1u);
    pthread_mutex_lock(&compat_g_map_lock);
    size_t i = compat_mm_first_above_locked(start);
    while (i < compat_g_map_count && compat_g_map_blocks[i]->base < end) {
        compat_SsrnxMapBlock *b = compat_g_map_blocks[i];
        const uintptr_t lo = start > b->base ? start : b->base;
        const uintptr_t hi = end < compat_mm_block_end(b) ? end : compat_mm_block_end(b);
#ifdef __SWITCH__
        if (b->sparse) {
            const size_t first = (size_t)((lo - b->base) / SSRNX_MM_PAGE);
            const size_t count = (size_t)((hi - lo) / SSRNX_MM_PAGE);
            if (!compat_mm_sparse_decommit_range_locked(b, first, count)) {
                pthread_mutex_unlock(&compat_g_map_lock);
                errno = ENOMEM;
                if (compat_ssrnx_memtrace_enabled()) {
                    SSRNX_LOGCI("VMTRACE", "munmap addr=%p len=%zu rc=-1 errno=%d sparse=1 caller=%p tid=%d tp=%p",
                                addr, len, errno, (void *)caller,
                                ssrnx_bionic_gettid(), ssrnx_bionic_tls_current_base());
                    compat_ssrnx_memtrace_flush();
                }
                return -1;
            }
        }
#endif
        for (uintptr_t page = lo; page < hi; page += SSRNX_MM_PAGE) {
            const size_t pi = (size_t)((page - b->base) / SSRNX_MM_PAGE);
            if (compat_mm_bit(b->mapped, pi)) {
                compat_mm_bit_set(b->mapped, pi, false);
                compat_mm_bit_set(b->anon, pi, false);
                b->prot[pi] = 0;
                --b->live;
            }
        }
#ifdef __SWITCH__
        if (b->sparse && b->live) compat_mm_sparse_compact_block_locked(b);
#endif
        if (b->live == 0) compat_mm_remove_locked(i);
        else ++i;
    }
    pthread_mutex_unlock(&compat_g_map_lock);
    if (compat_ssrnx_memtrace_enabled()) {
        SSRNX_LOGCI("VMTRACE", "munmap addr=%p len=%zu rc=0 errno=0 caller=%p tid=%d tp=%p",
                    addr, len, (void *)caller, ssrnx_bionic_gettid(), ssrnx_bionic_tls_current_base());
        compat_ssrnx_memtrace_flush();
    }
    return 0;
}
#undef MM_FOR_EACH_PAGE

static int compat_ssrnx_bionic_fork(void){errno=ENOSYS;return -1;}
static int compat_ssrnx_bionic_waitpid(int pid,int *status,int options){(void)pid;(void)status;(void)options;errno=ENOSYS;return -1;}
static int compat_ssrnx_bionic_execl(const char *path,const char *arg,...){(void)path;(void)arg;errno=ENOSYS;return -1;}

static char *compat_ssrnx_bionic_basename(char *path){static char dot[]=".";if(!path||!*path)return dot;size_t n=strlen(path);while(n>1&&path[n-1]=='/')path[--n]='\0';char *s=strrchr(path,'/');return s?s+1:path;}
static int compat_ssrnx_bionic_fputs(const char *s,void *stream){FILE *f=compat_ssrnx_host_file(stream);if(f==stderr&&s)compat_ssrnx_guest_stderr_capture(s,strlen(s));return fputs(s,f);}
static void compat_ssrnx_bionic_clearerr(void *stream){clearerr(compat_ssrnx_host_file(stream));}
static char *compat_ssrnx_bionic_fgets(char *s,int n,void *stream){return fgets(s,n,compat_ssrnx_host_file(stream));}
static int compat_ssrnx_bionic_fileno(void *stream){FILE *f=compat_ssrnx_host_file(stream);int fd=compat_ssrnx_random_stdio_fileno(f);return fd>=0?fd:fileno(f);}
static void compat_ssrnx_bionic_setbuf(void *stream,char *buf){setbuf(compat_ssrnx_host_file(stream),buf);}

static void compat_ssrnx_copy_tm(compat_SsrnxAndroidTm *out,const struct tm *src,const char *zone){memset(out,0,sizeof(*out));out->tm_sec=src->tm_sec;out->tm_min=src->tm_min;out->tm_hour=src->tm_hour;out->tm_mday=src->tm_mday;out->tm_mon=src->tm_mon;out->tm_year=src->tm_year;out->tm_wday=src->tm_wday;out->tm_yday=src->tm_yday;out->tm_isdst=src->tm_isdst;out->tm_zone=zone;}
static __thread compat_SsrnxAndroidTm compat_g_android_gmtime;
static compat_SsrnxAndroidTm *compat_ssrnx_bionic_gmtime(const int64_t *gt){if(!gt)return NULL;time_t t=(time_t)*gt;struct tm *h=gmtime(&t);if(!h)return NULL;compat_ssrnx_copy_tm(&compat_g_android_gmtime,h,"UTC");return &compat_g_android_gmtime;}
static compat_SsrnxAndroidTm *compat_ssrnx_bionic_gmtime_r(const int64_t *gt,compat_SsrnxAndroidTm *out){if(!gt||!out)return NULL;time_t t=(time_t)*gt;struct tm h; if(!gmtime_r(&t,&h))return NULL;compat_ssrnx_copy_tm(out,&h,"UTC");return out;}
static int compat_ssrnx_bionic_gettimeofday(compat_SsrnxAndroidTimeval *tv,void *tz){(void)tz;if(!tv){errno=EFAULT;return -1;}struct timeval h;if(gettimeofday(&h,NULL)!=0)return -1;tv->tv_sec=h.tv_sec;tv->tv_usec=h.tv_usec;return 0;}

static const char *compat_ssrnx_bionic_inet_ntop(int af,const void *src,char *dst,uint32_t size){int h=compat_ssrnx_android_family_to_host(af);if(h<0){errno=EAFNOSUPPORT;return NULL;}const char*r=inet_ntop(h,src,dst,(socklen_t)size);if(!r)compat_ssrnx_network_fix_errno();return r;}
static int compat_ssrnx_bionic_inet_pton(int af,const char *src,void *dst){int h=compat_ssrnx_android_family_to_host(af);if(h<0){errno=EAFNOSUPPORT;return -1;}int r=inet_pton(h,src,dst);if(r<0)compat_ssrnx_network_fix_errno();return r;}
static ssize_t compat_ssrnx_bionic_sendto(int fd,const void *buf,size_t len,int flags,const compat_SsrnxAndroidSockaddr *addr,uint32_t alen){if(compat_ssrnx_network_ensure_initialized()!=0)return -1;struct sockaddr_storage h;socklen_t hl=0;if(addr&&compat_ssrnx_android_sockaddr_to_host(addr,alen,&h,&hl)!=0)return -1;return compat_ssrnx_network_result_ssize(sendto(fd,buf,len,compat_ssrnx_android_send_flags_to_host(flags),addr?(struct sockaddr*)&h:NULL,addr?hl:0));}
static ssize_t compat_ssrnx_bionic_recvfrom(int fd, void *buf, size_t len, int flags,
                                    compat_SsrnxAndroidSockaddr *addr, uint32_t *alen) {
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    struct sockaddr_storage h;
    socklen_t hl = sizeof(h);
    const ssize_t n = compat_ssrnx_safe_recvfrom(fd, buf, len, compat_ssrnx_android_send_flags_to_host(flags),
        addr ? (struct sockaddr *)&h : NULL, addr ? &hl : NULL);
    if (n < 0) compat_ssrnx_network_fix_errno();
    (void)compat_ssrnx_network_socket_io_trace("recvfrom", fd, len, n, flags, true, true,
        (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)));
    if (n < 0) return n;
    if (addr && alen && compat_ssrnx_host_sockaddr_to_android((struct sockaddr *)&h, hl, addr, alen) != 0)
        return -1;
    return n;
}




/* socket misc */
/* Android arm64 struct msghdr (Linux kernel layout: size_t lengths). */
typedef struct compat_SsrnxAndroidMsghdr {
    void *msg_name;
    uint32_t msg_namelen;
    uint32_t _pad_namelen;
    compat_SsrnxAndroidIovec *msg_iov;
    size_t msg_iovlen;
    void *msg_control;
    size_t msg_controllen;
    int32_t msg_flags;
    int32_t _pad_flags;
} compat_SsrnxAndroidMsghdr;
_Static_assert(sizeof(compat_SsrnxAndroidMsghdr) == 56, "Bionic LP64 msghdr size");
_Static_assert(offsetof(compat_SsrnxAndroidMsghdr, msg_iov) == 16, "Bionic msghdr msg_iov offset");
_Static_assert(offsetof(compat_SsrnxAndroidMsghdr, msg_control) == 32, "Bionic msghdr msg_control offset");
_Static_assert(offsetof(compat_SsrnxAndroidMsghdr, msg_flags) == 48, "Bionic msghdr msg_flags offset");
#define SSRNX_ANDROID_UIO_MAXIOV 1024u
#define SSRNX_ANDROID_MSG_OOB 0x1
#define SSRNX_ANDROID_MSG_CTRUNC 0x8
#define SSRNX_ANDROID_MSG_TRUNC 0x20
#define SSRNX_ANDROID_MSG_EOR 0x80

/* Translate the guest iovec array; *out is stack_iov or a heap copy. */
static int compat_ssrnx_msghdr_host_iov(const compat_SsrnxAndroidMsghdr *msg, struct iovec *stack_iov, size_t stack_cap,
                                 struct iovec **out) {
    if (msg->msg_iovlen > SSRNX_ANDROID_UIO_MAXIOV) { errno = EMSGSIZE; return -1; }
    if (msg->msg_iovlen && !msg->msg_iov) { errno = EFAULT; return -1; }
    struct iovec *iov = stack_iov;
    if (msg->msg_iovlen > stack_cap) {
        iov = (struct iovec *)calloc(msg->msg_iovlen, sizeof(*iov));
        if (!iov) { errno = ENOMEM; return -1; }
    }
    for (size_t i = 0; i < msg->msg_iovlen; ++i) {
        iov[i].iov_base = msg->msg_iov[i].iov_base;
        iov[i].iov_len = msg->msg_iov[i].iov_len;
    }
    *out = iov;
    return 0;
}

/* Horizon sockets carry no Linux ancillary data (SCM_RIGHTS, IP_PKTINFO,
 * ...): sendmsg rejects a control buffer and recvmsg never fills one. */
static ssize_t compat_ssrnx_bionic_sendmsg(int fd, const compat_SsrnxAndroidMsghdr *msg, int flags) {
    if (!msg) { errno = EFAULT; return -1; }
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    if (msg->msg_control && msg->msg_controllen) { errno = EOPNOTSUPP; return -1; }
    struct sockaddr_storage h;
    socklen_t hl = 0;
    if (msg->msg_name &&
        compat_ssrnx_android_sockaddr_to_host((const compat_SsrnxAndroidSockaddr *)msg->msg_name, msg->msg_namelen, &h, &hl) != 0)
        return -1;
    struct iovec stack_iov[16];
    struct iovec *iov = NULL;
    if (compat_ssrnx_msghdr_host_iov(msg, stack_iov, 16, &iov) != 0) return -1;
    struct msghdr hm;
    memset(&hm, 0, sizeof(hm));
    hm.msg_name = msg->msg_name ? (void *)&h : NULL;
    hm.msg_namelen = msg->msg_name ? hl : 0;
    hm.msg_iov = iov;
    hm.msg_iovlen = msg->msg_iovlen;
    const ssize_t rc = sendmsg(fd, &hm, compat_ssrnx_android_send_flags_to_host(flags));
    const int saved = errno;
    if (iov != stack_iov) free(iov);
    errno = saved;
    if (rc < 0) compat_ssrnx_network_fix_errno();
    return rc;
}

static ssize_t compat_ssrnx_bionic_recvmsg(int fd, compat_SsrnxAndroidMsghdr *msg, int flags) {
    if (!msg) { errno = EFAULT; return -1; }
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    struct sockaddr_storage h;
    struct iovec stack_iov[16];
    struct iovec *iov = NULL;
    if (compat_ssrnx_msghdr_host_iov(msg, stack_iov, 16, &iov) != 0) return -1;
    struct msghdr hm;
    memset(&hm, 0, sizeof(hm));
    hm.msg_name = msg->msg_name ? (void *)&h : NULL;
    hm.msg_namelen = msg->msg_name ? (socklen_t)sizeof(h) : 0;
    hm.msg_iov = iov;
    hm.msg_iovlen = msg->msg_iovlen;
    const ssize_t n = compat_ssrnx_safe_recvmsg(fd, &hm, compat_ssrnx_android_send_flags_to_host(flags));
    const int saved = errno;
    if (iov != stack_iov) free(iov);
    errno = saved;
    if (n < 0) { compat_ssrnx_network_fix_errno(); return -1; }
    if (msg->msg_name) {
        uint32_t guest_len = msg->msg_namelen;
        if (hm.msg_namelen == 0) guest_len = 0;
        else if (compat_ssrnx_host_sockaddr_to_android((const struct sockaddr *)&h, hm.msg_namelen,
                                                (compat_SsrnxAndroidSockaddr *)msg->msg_name, &guest_len) != 0)
            return -1;
        msg->msg_namelen = guest_len;
    }
    int out_flags = 0;
    if (hm.msg_flags & MSG_OOB) out_flags |= SSRNX_ANDROID_MSG_OOB;
    if (hm.msg_flags & MSG_TRUNC) out_flags |= SSRNX_ANDROID_MSG_TRUNC;
    if (hm.msg_flags & MSG_CTRUNC) out_flags |= SSRNX_ANDROID_MSG_CTRUNC;
    if (hm.msg_flags & MSG_EOR) out_flags |= SSRNX_ANDROID_MSG_EOR;
    msg->msg_controllen = 0;
    msg->msg_flags = out_flags;
    return n;
}
static int compat_ssrnx_bionic_getsockname(int fd,compat_SsrnxAndroidSockaddr *addr,uint32_t *alen){if(compat_ssrnx_network_ensure_initialized()!=0)return -1;if(!alen){errno=EFAULT;return -1;}struct sockaddr_storage h;socklen_t hl=sizeof(h);if(getsockname(fd,(struct sockaddr*)&h,&hl)!=0){compat_ssrnx_network_fix_errno();return -1;}return compat_ssrnx_host_sockaddr_to_android((struct sockaddr*)&h,hl,addr,alen);}
static int compat_ssrnx_bionic_getpeername(int fd,compat_SsrnxAndroidSockaddr *addr,uint32_t *alen){if(compat_ssrnx_network_ensure_initialized()!=0)return -1;if(!alen){errno=EFAULT;return -1;}struct sockaddr_storage h;socklen_t hl=sizeof(h);if(getpeername(fd,(struct sockaddr*)&h,&hl)!=0){compat_ssrnx_network_fix_errno();return -1;}return compat_ssrnx_host_sockaddr_to_android((struct sockaddr*)&h,hl,addr,alen);}
static int compat_ssrnx_bionic_getnameinfo(const compat_SsrnxAndroidSockaddr *addr, uint32_t alen,
                                    char *host, size_t hostlen, char *serv, size_t servlen, int flags) {
    if (flags & ~0x1f) return 3;
    if (hostlen > UINT32_MAX || servlen > UINT32_MAX) return 14;
    if (compat_ssrnx_network_ensure_initialized() != 0) return 11;
    struct sockaddr_storage h;
    socklen_t hl = 0;
    if (compat_ssrnx_android_sockaddr_to_host(addr, alen, &h, &hl) != 0) return 5;
    int host_flags = 0;
    if (flags & 1) host_flags |= NI_NOFQDN;
    if (flags & 2) host_flags |= NI_NUMERICHOST;
    if (flags & 4) host_flags |= NI_NAMEREQD;
    if (flags & 8) host_flags |= NI_NUMERICSERV;
    if (flags & 16) host_flags |= NI_DGRAM;
    int rc = getnameinfo((struct sockaddr *)&h, hl, host, (socklen_t)hostlen,
                         serv, (socklen_t)servlen, host_flags);
#ifdef EAI_SYSTEM
if(rc==EAI_SYSTEM)compat_ssrnx_network_fix_errno();
#endif
    return compat_ssrnx_host_eai_to_android(rc);
}
static const char *compat_ssrnx_bionic_gai_strerror(int code) {
    switch (code) {
    case 0: return "Success";
    case 1: return "Address family for hostname not supported";
    case 2: return "Temporary failure in name resolution";
    case 3: return "Invalid value for ai_flags";
    case 4: return "Non-recoverable failure in name resolution";
    case 5: return "ai_family not supported";
    case 6: return "Memory allocation failure";
    case 7: return "No address associated with hostname";
    case 8: return "Name or service not known";
    case 9: return "Servname not supported for ai_socktype";
    case 10: return "ai_socktype not supported";
    case 11: return "System error";
    case 12: return "Invalid value for hints";
    case 13: return "Resolved protocol is unknown";
    case 14: return "Argument buffer overflow";
    default: return "Unknown error";
    }
}
static unsigned compat_ssrnx_bionic_if_nametoindex(const char *name){
    if (!name) { errno = EFAULT; return 0; }
    if (strcmp(name,"lo")==0 || strcmp(name,"lo0")==0) return 1;
    if (compat_ssrnx_network_ensure_initialized() != 0) return 0;
    const int transport = ssrnx_compat_network_transport();
    if (transport == 1 && (strcmp(name,"wlan0")==0 || strcmp(name,"wifi0")==0)) return 2;
    if (transport == 3 && (strcmp(name,"eth0")==0 || strcmp(name,"ethernet0")==0)) return 2;
    errno = ENXIO;
    return 0;
}
static char *compat_ssrnx_bionic_if_indextoname(unsigned index, char *name) {
    if (!name) { errno = EFAULT; return NULL; }
    if (index == 1) { snprintf(name, 16, "lo"); return name; }
    if (index == 2 && compat_ssrnx_network_ensure_initialized() == 0) {
        const int transport = ssrnx_compat_network_transport();
        if (transport == 1) { snprintf(name, 16, "wlan0"); return name; }
        if (transport == 3) { snprintf(name, 16, "eth0"); return name; }
    }
    errno = ENXIO;
    return NULL;
}
static struct servent *compat_ssrnx_bionic_getservbyport(int port, const char *proto) {
    if (compat_ssrnx_network_ensure_initialized() != 0) return NULL;
    return getservbyport(port, proto);
}
static __thread struct hostent compat_g_android_hostent;
static struct hostent *compat_ssrnx_bionic_hostent_from_host(struct hostent *host) {
    if (!host) return NULL;
    const int guest_family = compat_ssrnx_host_family_to_android(host->h_addrtype);
    if (guest_family < 0) { errno = EAFNOSUPPORT; return NULL; }
    compat_g_android_hostent = *host;
    compat_g_android_hostent.h_addrtype = guest_family;
    return &compat_g_android_hostent;
}

/* Bionic's h_errno is (*__get_h_errno()), per thread. The host resolver's
 * thread-local h_errno uses the same netdb values (HOST_NOT_FOUND=1,
 * TRY_AGAIN=2, NO_RECOVERY=3, NO_DATA=4), so it is shared directly. */
static int *compat_ssrnx_bionic_get_h_errno(void) { return &h_errno; }

/* POSIX.1-2008 stpcpy: copy including NUL, return the end of dest. */
static char *compat_ssrnx_bionic_stpcpy(char *dest, const char *src) {
    const size_t n = strlen(src);
    memcpy(dest, src, n + 1u);
    return dest + n;
}

static struct hostent *compat_ssrnx_bionic_gethostbyname(const char *name){
    if(!name){errno=EFAULT;return NULL;}
    if(compat_ssrnx_network_ensure_initialized()!=0)return NULL;
    const bool trace=compat_ssrnx_network_trace_slot();
    if(trace)SSRNX_LOGCI("NET","gethostbyname begin host=%s",name);
    struct hostent *h=gethostbyname(name);
    if(!h){const int host_err=errno;compat_ssrnx_network_fix_errno();SSRNX_LOGCW("NET","gethostbyname failed host=%s errno=%d",name,host_err);return NULL;}
    struct hostent *guest=compat_ssrnx_bionic_hostent_from_host(h);
    if(trace&&guest)SSRNX_LOGCI("NET","gethostbyname complete host=%s addrtype=%d",name,guest->h_addrtype);
    return guest;
}

static struct hostent *compat_ssrnx_bionic_gethostbyaddr(const void *addr, uint32_t len, int guest_family) {
    if (!addr) { errno = EFAULT; return NULL; }
    if (compat_ssrnx_network_ensure_initialized() != 0) return NULL;
    const int host_family = compat_ssrnx_android_family_to_host(guest_family);
    if (host_family < 0) { errno = EAFNOSUPPORT; return NULL; }
    const bool trace = compat_ssrnx_network_trace_slot();
    if (trace) SSRNX_LOGCI("NET", "gethostbyaddr begin family=%d len=%u", guest_family, len);
    struct hostent *host = gethostbyaddr(addr, (socklen_t)len, host_family);
    if (!host) {
        compat_ssrnx_network_fix_errno();
        if (trace) SSRNX_LOGCW("NET", "gethostbyaddr failed family=%d len=%u errno=%d", guest_family, len, errno);
        return NULL;
    }
    struct hostent *guest = compat_ssrnx_bionic_hostent_from_host(host);
    if (trace && guest) SSRNX_LOGCI("NET", "gethostbyaddr complete family=%d name=%s", guest_family, guest->h_name ? guest->h_name : "<null>");
    return guest;
}
static char *compat_ssrnx_bionic_inet_ntoa(struct in_addr addr) {
    /* Android 11 uses one process-static buffer, overwritten by the next call. */
    static char text[INET_ADDRSTRLEN];
    const uint8_t *bytes = (const uint8_t *)&addr.s_addr;
    (void)snprintf(text, sizeof(text), "%u.%u.%u.%u",
                   bytes[0], bytes[1], bytes[2], bytes[3]);
    return text;
}

static uint32_t compat_ssrnx_bionic_inet_addr(const char *text) {
    if (!text) return UINT32_C(0xffffffff);
    struct in_addr addr;
    if (inet_pton(AF_INET, text, &addr) != 1) return UINT32_C(0xffffffff);
    return addr.s_addr;
}

/* POSIX drand48 family uses a fixed 48-bit LCG. Keep the process-global state
 * local to SSRNX so behavior is independent of the host libc implementation. */
static pthread_mutex_t compat_g_lrand48_lock = PTHREAD_MUTEX_INITIALIZER;
static uint64_t compat_g_lrand48_state = UINT64_C(0x1234abcd330e);
static void compat_ssrnx_bionic_srand48(long seed) {
    pthread_mutex_lock(&compat_g_lrand48_lock);
    compat_g_lrand48_state = (((uint64_t)(uint32_t)seed) << 16) | UINT64_C(0x330e);
    pthread_mutex_unlock(&compat_g_lrand48_lock);
}
static long compat_ssrnx_bionic_lrand48(void) {
    pthread_mutex_lock(&compat_g_lrand48_lock);
    compat_g_lrand48_state = (UINT64_C(0x5deece66d) * compat_g_lrand48_state + UINT64_C(0xb)) & UINT64_C(0xffffffffffff);
    const long out = (long)(compat_g_lrand48_state >> 17);
    pthread_mutex_unlock(&compat_g_lrand48_lock);
    return out;
}

static int compat_ssrnx_bionic_gethostname(char *name,size_t len){static const char host[]="apn64-switch";if(!name||len==0){errno=EINVAL;return -1;}size_t n=sizeof(host);if(n>len){errno=ENAMETOOLONG;return -1;}memcpy(name,host,n);return 0;}
/* Linux/Bionic struct utsname: six 65-byte fields. The node name matches
 * gethostname(); the kernel identity is Linux/aarch64, with a release that
 * names SSRNX rather than impersonating a particular device kernel. */
typedef struct compat_SsrnxAndroidUtsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
} compat_SsrnxAndroidUtsname;
_Static_assert(sizeof(compat_SsrnxAndroidUtsname) == 390, "Bionic struct utsname size");
static int compat_ssrnx_bionic_uname(compat_SsrnxAndroidUtsname *out) {
    if (!out) { errno = EFAULT; return -1; }
    memset(out, 0, sizeof(*out));
    snprintf(out->sysname, sizeof(out->sysname), "%s", "Linux");
    if (compat_ssrnx_bionic_gethostname(out->nodename, sizeof(out->nodename)) != 0)
        snprintf(out->nodename, sizeof(out->nodename), "%s", "localhost");
    snprintf(out->release, sizeof(out->release), "%s", "4.19.0-apn64");
    snprintf(out->version, sizeof(out->version), "%s", "#1 SMP PREEMPT APN64");
    snprintf(out->machine, sizeof(out->machine), "%s", "aarch64");
    snprintf(out->domainname, sizeof(out->domainname), "%s", "(none)");
    return 0;
}

static int compat_ssrnx_bionic_socketpair(int domain, int type, int protocol, int out[2]) {
    static unsigned reports;
    const bool trace = __atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 16u;
    if (trace)
        SSRNX_LOGCI("NET", "socketpair request domain=%d type=%#x protocol=%d caller=%p",
                    domain, type, protocol, __builtin_extract_return_addr(__builtin_return_address(0)));
    if (compat_ssrnx_network_ensure_initialized() != 0) return -1;
    if (!out) { errno = EFAULT; return -1; }
    if (domain != SSRNX_ANDROID_AF_UNIX) { errno = EAFNOSUPPORT; return -1; }
    bool nonblock = false;
    if (compat_ssrnx_android_socktype_to_host(type, &nonblock) != SOCK_STREAM || protocol != 0) {
        errno = EPROTONOSUPPORT; return -1;
    }
    /* Horizon has no Unix-domain backend; a loopback pair supplies
     * the existing stream transport. Datagram/seqpacket/address semantics
     * remain unsupported rather than being represented as a TCP stream. */
    int listener = socket(AF_INET, SOCK_STREAM, 0), client = -1, peer = -1;
    if (listener < 0) goto fail;
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(listener, (struct sockaddr *)&address, sizeof(address)) || listen(listener, 1)) goto fail;
    socklen_t length = sizeof(address);
    if (getsockname(listener, (struct sockaddr *)&address, &length)) goto fail;
    client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0 || connect(client, (struct sockaddr *)&address, sizeof(address))) goto fail;
    peer = accept(listener, NULL, NULL);
    if (peer < 0) goto fail;
    close(listener); listener = -1;
    int pair[2] = {client, peer};
    for (unsigned i = 0; i < 2; ++i) {
        if (nonblock) {
            const int flags = fcntl(pair[i], F_GETFL, 0);
            if (flags < 0 || fcntl(pair[i], F_SETFL, flags | O_NONBLOCK)) goto fail;
        }
        if (compat_ssrnx_socket_descriptor_open(pair[i], (type & SSRNX_ANDROID_SOCK_CLOEXEC) != 0)) goto fail;
    }
    out[0] = client; out[1] = peer;
    compat_ssrnx_network_socket_register(client);
    compat_ssrnx_network_socket_register(peer);
    if (trace) SSRNX_LOGCI("NET", "socketpair result=0 fds=%d,%d", client, peer);
    return 0;
fail: {
        const int saved = errno;
        if (trace) SSRNX_LOGCW("NET", "socketpair setup failed hostErrno=%d", saved);
        if (listener >= 0) close(listener);
        if (client >= 0) compat_ssrnx_socket_close_setup(client);
        if (peer >= 0) compat_ssrnx_socket_close_setup(peer);
        errno = saved; compat_ssrnx_network_fix_errno();
        return -1;
    }
}

typedef struct compat_SsrnxAndroidPollfd{int32_t fd;int16_t events;int16_t revents;}compat_SsrnxAndroidPollfd;
enum {
    SSRNX_ANDROID_POLLIN     = 0x0001,
    SSRNX_ANDROID_POLLPRI    = 0x0002,
    SSRNX_ANDROID_POLLOUT    = 0x0004,
    SSRNX_ANDROID_POLLERR    = 0x0008,
    SSRNX_ANDROID_POLLHUP    = 0x0010,
    SSRNX_ANDROID_POLLNVAL   = 0x0020,
    SSRNX_ANDROID_POLLRDNORM = 0x0040,
    SSRNX_ANDROID_POLLRDBAND = 0x0080,
    SSRNX_ANDROID_POLLWRNORM = 0x0100,
    SSRNX_ANDROID_POLLWRBAND = 0x0200
};

static short compat_ssrnx_android_poll_events_to_host(short events) {
    short h = 0;
    const uint16_t e = (uint16_t)events;
    if (e & (SSRNX_ANDROID_POLLIN | SSRNX_ANDROID_POLLRDNORM)) h |= POLLIN;
    if (e & (SSRNX_ANDROID_POLLOUT | SSRNX_ANDROID_POLLWRNORM | SSRNX_ANDROID_POLLWRBAND)) h |= POLLOUT;
    if (e & (SSRNX_ANDROID_POLLPRI | SSRNX_ANDROID_POLLRDBAND)) h |= POLLPRI;
    return h;
}

static short compat_ssrnx_host_poll_revents_to_android(short revents, short requested) {
    short g = 0;
    const uint16_t r = (uint16_t)revents;
    const uint16_t q = (uint16_t)requested;
    if (r & POLLIN) {
        if (q & SSRNX_ANDROID_POLLIN) g |= SSRNX_ANDROID_POLLIN;
        if (q & SSRNX_ANDROID_POLLRDNORM) g |= SSRNX_ANDROID_POLLRDNORM;
    }
    if (r & POLLOUT) {
        if (q & SSRNX_ANDROID_POLLOUT) g |= SSRNX_ANDROID_POLLOUT;
        if (q & SSRNX_ANDROID_POLLWRNORM) g |= SSRNX_ANDROID_POLLWRNORM;
        if (q & SSRNX_ANDROID_POLLWRBAND) g |= SSRNX_ANDROID_POLLWRBAND;
    }
    if (r & POLLPRI) {
        if (q & SSRNX_ANDROID_POLLPRI) g |= SSRNX_ANDROID_POLLPRI;
        if (q & SSRNX_ANDROID_POLLRDBAND) g |= SSRNX_ANDROID_POLLRDBAND;
    }
    if (r & POLLERR) g |= SSRNX_ANDROID_POLLERR;
    if (r & POLLHUP) g |= SSRNX_ANDROID_POLLHUP;
    if (r & POLLNVAL) g |= SSRNX_ANDROID_POLLNVAL;
    return g;
}

static int64_t compat_ssrnx_poll_monotonic_ms(void) {
    struct timespec ts = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (int64_t)ts.tv_sec * INT64_C(1000) + ts.tv_nsec / INT64_C(1000000);
}

/* Linux poll() never fails for an individual descriptor: a closed one reports
 * POLLNVAL and a file without a poll operation (regular files, directories)
 * reports DEFAULT_POLLMASK. libnx poll() only accepts BSD sockets and fails the
 * whole call with EBADF/ENOTSOCK, so only sockets may reach the host poll.
 * Runtimes such as Mono/IL2CPP treat a failed poll in their I/O selector as
 * fatal, which stops completion of every later asynchronous socket operation. */
typedef enum { SSRNX_POLL_FD_SOCKET, SSRNX_POLL_FD_ALWAYS_READY, SSRNX_POLL_FD_INVALID } compat_SsrnxPollFdKind;

static compat_SsrnxPollFdKind compat_ssrnx_poll_fd_kind(int fd) {
#ifdef __SWITCH__
    __handle *handle = __get_handle(fd);
    if (!handle) return SSRNX_POLL_FD_INVALID;
    const devoptab_t *device = handle->device < STD_MAX ? devoptab_list[handle->device] : NULL;
    if (device && device->name && strcmp(device->name, "soc") == 0) return SSRNX_POLL_FD_SOCKET;
    return SSRNX_POLL_FD_ALWAYS_READY;
#else
    struct stat st;
    if (fstat(fd, &st) != 0) return SSRNX_POLL_FD_INVALID;
    return (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode)) ? SSRNX_POLL_FD_ALWAYS_READY : SSRNX_POLL_FD_SOCKET;
#endif
}

static short compat_ssrnx_poll_static_revents(compat_SsrnxPollFdKind kind, short events) {
    if (kind == SSRNX_POLL_FD_INVALID) return SSRNX_ANDROID_POLLNVAL;
    const uint16_t ready = SSRNX_ANDROID_POLLIN | SSRNX_ANDROID_POLLRDNORM |
                           SSRNX_ANDROID_POLLOUT | SSRNX_ANDROID_POLLWRNORM;
    return (short)((uint16_t)events & ready);
}

static int compat_ssrnx_bionic_poll(compat_SsrnxAndroidPollfd *fds, uint32_t count, int timeout_ms) {
    extern int ssrnx_compat_epoll_poll(int,short) __attribute__((weak));
    if (count && !fds) { errno = EFAULT; compat_ssrnx_network_fix_errno(); return -1; }
    static _Thread_local compat_SsrnxNetworkWaitTrace trace_state;
    const compat_SsrnxNetworkWaitCall trace = compat_ssrnx_network_wait_begin(&trace_state, "poll", count,
        timeout_ms < 0 ? -1 : (int64_t)timeout_ms * 1000,
        (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)));
    for (uint32_t i = 0; trace.emit && i < count; ++i)
        compat_ssrnx_network_wait_fd(&trace, "poll", "begin", i, fds[i].fd,
            (unsigned)(uint16_t)fds[i].events, 0,
            (unsigned)(uint16_t)compat_ssrnx_android_poll_events_to_host(fds[i].events), 0);
    uint64_t host_calls = 0;
    int last_host_rc = 0, last_host_wait_ms = 0;
    unsigned descriptor_retries = 0;

    /* libnx's socket layer implements real socket poll() directly on bsdPoll().
     * SSRNX pipe()/urandom descriptors are virtual, however, and cannot wake a
     * host poll that is already sleeping.  Keep real descriptors in the native
     * poll set, but when at least one virtual descriptor participates, bound
     * each host sleep and re-check virtual readiness between slices.  This is
     * important for curl/OpenSSL worker wakeup pipes: an infinite host sleep on
     * an SSRNX virtual pipe otherwise strands the HTTP worker forever. */
    enum { SSRNX_VIRTUAL_POLL_SLICE_MS = 8 };
    struct pollfd stack_fds[64];
    uint32_t stack_map[64];
    struct pollfd *host_fds = stack_fds;
    uint32_t *host_map = stack_map;
    if (count > 64u) {
        host_fds = (struct pollfd *)calloc(count, sizeof(*host_fds));
        host_map = (uint32_t *)calloc(count, sizeof(*host_map));
        if (!host_fds || !host_map) {
            if (host_fds && host_fds != stack_fds) free(host_fds);
            if (host_map && host_map != stack_map) free(host_map);
            compat_ssrnx_network_wait_end(&trace_state, &trace, "poll", -1, ENOMEM, 0, 0, 0);
            errno = ENOMEM;
            compat_ssrnx_network_fix_errno();
            return -1;
        }
    }

    uint32_t host_count = 0;
    bool has_virtual = false, has_eventfd = false;
    bool classify = true;
    int static_ready = 0;

    const int64_t start_ms = timeout_ms >= 0 ? compat_ssrnx_poll_monotonic_ms() : 0;
    const int64_t deadline_ms = timeout_ms >= 0 ? start_ms + (int64_t)timeout_ms : -1;
    int ready = 0;

    for (;;) {
        if (classify) {
            classify = false;
            host_count = 0;
            has_virtual = false;
            has_eventfd = false;
            static_ready = 0;
            for (uint32_t i = 0; i < count; ++i) {
                fds[i].revents = 0;
                if (fds[i].fd < 0) continue;
                if (ssrnx_eventfd_poll && ssrnx_eventfd_poll(fds[i].fd, 0) >= 0) has_eventfd = true;
                if ((ssrnx_compat_epoll_poll && ssrnx_compat_epoll_poll(fds[i].fd,0)>=0) || ssrnx_fd_is_virtual(fds[i].fd)) {
                    has_virtual = true;
                    continue;
                }
                const compat_SsrnxPollFdKind kind = compat_ssrnx_poll_fd_kind(fds[i].fd);
                if (kind != SSRNX_POLL_FD_SOCKET) {
                    fds[i].revents = compat_ssrnx_poll_static_revents(kind, fds[i].events);
                    if (fds[i].revents) ++static_ready;
                    continue;
                }
                host_fds[host_count].fd = fds[i].fd;
                host_fds[host_count].events = compat_ssrnx_android_poll_events_to_host(fds[i].events);
                host_fds[host_count].revents = 0;
                host_map[host_count] = i;
                ++host_count;
            }
        }
        ready = static_ready;

        /* Refresh virtual descriptors every iteration.  Translate through the
         * same masks as native socket poll so aliases such as POLLRDNORM and
         * terminal bits are presented with Android/Linux values. */
        for (uint32_t i = 0; i < count; ++i) {
            if (fds[i].fd < 0) continue;
            if (ssrnx_compat_epoll_poll) {
                const int e=ssrnx_compat_epoll_poll(fds[i].fd,compat_ssrnx_android_poll_events_to_host(fds[i].events));
                if (e>=0) { fds[i].revents=compat_ssrnx_host_poll_revents_to_android((short)e,fds[i].events);if (e) ++ready;continue; }
            }
            if (!ssrnx_fd_is_virtual(fds[i].fd)) continue;
            const short host_events = compat_ssrnx_android_poll_events_to_host(fds[i].events);
            const short host_revents = ssrnx_fd_poll_revents(fds[i].fd, host_events);
            fds[i].revents = compat_ssrnx_host_poll_revents_to_android(host_revents, fds[i].events);
            if (fds[i].revents) ++ready;
        }

        int wait_ms = timeout_ms;
        if (ready || timeout_ms == 0) {
            wait_ms = 0;
        } else if (has_virtual) {
            if (timeout_ms < 0) {
                wait_ms = SSRNX_VIRTUAL_POLL_SLICE_MS;
            } else {
                const int64_t now_ms = compat_ssrnx_poll_monotonic_ms();
                int64_t remaining = deadline_ms - now_ms;
                if (remaining <= 0) wait_ms = 0;
                else if (remaining > SSRNX_VIRTUAL_POLL_SLICE_MS) wait_ms = SSRNX_VIRTUAL_POLL_SLICE_MS;
                else wait_ms = (int)remaining;
            }
        }

        for (uint32_t j = 0; j < host_count; ++j) host_fds[j].revents = 0;
        const bool host_called = host_count || wait_ms != 0;
        const int host_rc = host_called
            ? compat_ssrnx_safe_poll(host_fds, (nfds_t)host_count, wait_ms)
            : 0;
        if (host_called) ++host_calls;
        last_host_rc = host_rc;
        last_host_wait_ms = wait_ms;
        if (host_rc < 0) {
            const int host_errno = errno;
            /* A descriptor was closed or reused by another thread after
             * classification; Linux reports that per entry, never as failure. */
            if ((host_errno == EBADF || host_errno == ENOTSOCK) && descriptor_retries++ < 8u) {
                classify = true;
                continue;
            }
            SSRNX_LOGCW("NET", "poll failed fds=%u hostFds=%u timeoutMs=%d hostWaitMs=%d hostRc=%d hostErrno=%d guestErrno=%d virtual=%d",
                        count, host_count, timeout_ms, wait_ms, host_rc, host_errno,
                        compat_ssrnx_host_errno_to_android(host_errno), has_virtual);
            static unsigned failure_reports;
            if (__atomic_fetch_add(&failure_reports, 1u, __ATOMIC_RELAXED) < 4u) {
                for (uint32_t j = 0; j < host_count && j < 16u; ++j) {
                    const uint32_t i = host_map[j];
                    SSRNX_LOGCW("NET", "poll failure descriptor index=%u fd=%d guestEvents=%#x hostEvents=%#x hostRevents=%#x",
                                i, host_fds[j].fd, (unsigned)(uint16_t)fds[i].events,
                                (unsigned)(uint16_t)host_fds[j].events, (unsigned)(uint16_t)host_fds[j].revents);
                }
            }
            compat_ssrnx_network_wait_end(&trace_state, &trace, "poll", -1, host_errno,
                                  host_calls, host_rc, wait_ms);
            uint32_t host_index = 0;
            for (uint32_t i = 0; trace.emit && i < count; ++i) {
                unsigned host_events = 0, host_revents = 0;
                if (host_index < host_count && host_map[host_index] == i) {
                    host_events = (unsigned)(uint16_t)host_fds[host_index].events;
                    host_revents = (unsigned)(uint16_t)host_fds[host_index++].revents;
                }
                compat_ssrnx_network_wait_fd(&trace, "poll", "error", i, fds[i].fd,
                    (unsigned)(uint16_t)fds[i].events, (unsigned)(uint16_t)fds[i].revents,
                    host_events, host_revents);
            }
            if (host_fds != stack_fds) free(host_fds);
            if (host_map != stack_map) free(host_map);
            errno = host_errno;
            compat_ssrnx_network_fix_errno();
            return -1;
        }

        for (uint32_t j = 0; j < host_count; ++j) {
            const uint32_t i = host_map[j];
            fds[i].revents = compat_ssrnx_host_poll_revents_to_android(host_fds[j].revents, fds[i].events);
            if (fds[i].revents) ++ready;
        }

        if (ready || timeout_ms == 0 || !has_virtual) break;
        if (timeout_ms >= 0 && compat_ssrnx_poll_monotonic_ms() >= deadline_ms) break;
        if (has_eventfd) classify = true; /* Low host fd may close/reuse during a sliced wait. */
    }

    bool terminal = false;
    int first_fd = -1;
    unsigned first_events = 0, first_revents = 0;
    for (uint32_t i = 0; i < count; ++i) {
        if ((uint16_t)fds[i].revents & (SSRNX_ANDROID_POLLERR | SSRNX_ANDROID_POLLHUP | SSRNX_ANDROID_POLLNVAL))
            terminal = true;
        if (first_fd < 0 && fds[i].fd >= 0 && !ssrnx_fd_is_virtual(fds[i].fd)) {
            first_fd = fds[i].fd;
            first_events = (unsigned)(uint16_t)fds[i].events;
            first_revents = (unsigned)(uint16_t)fds[i].revents;
        }
    }
    const int saved = errno;
    if (terminal || compat_ssrnx_network_poll_trace_slot())
        SSRNX_LOGCI("NET", "poll fds=%u timeoutMs=%d result=%d firstFd=%d events=%#x revents=%#x terminal=%d virtual=%d",
                    count, timeout_ms, ready, first_fd, first_events, first_revents,
                    terminal ? 1 : 0, has_virtual ? 1 : 0);

    compat_ssrnx_network_wait_end(&trace_state, &trace, "poll", ready, 0,
                          host_calls, last_host_rc, last_host_wait_ms);
    uint32_t host_index = 0;
    for (uint32_t i = 0; trace.emit && i < count; ++i) {
        unsigned host_events = 0, host_revents = 0;
        if (host_index < host_count && host_map[host_index] == i) {
            host_events = (unsigned)(uint16_t)host_fds[host_index].events;
            host_revents = (unsigned)(uint16_t)host_fds[host_index++].revents;
        }
        compat_ssrnx_network_wait_fd(&trace, "poll", "end", i, fds[i].fd,
            (unsigned)(uint16_t)fds[i].events, (unsigned)(uint16_t)fds[i].revents,
            host_events, host_revents);
    }

    if (host_fds != stack_fds) free(host_fds);
    if (host_map != stack_map) free(host_map);
    errno = saved;
    return ready;
}

static int compat_ssrnx_bionic_select_poll(int nfds, compat_SsrnxAndroidFdSet *readfds, compat_SsrnxAndroidFdSet *writefds,
                                    compat_SsrnxAndroidFdSet *exceptfds, compat_SsrnxAndroidTimeval *timeout, uintptr_t caller) {
    compat_SsrnxAndroidPollfd stack_fds[64], *fds = stack_fds;
    uint32_t count = 0;
    for (int fd = 0; fd < nfds; ++fd)
        if (compat_ssrnx_android_fd_isset(readfds, fd) || compat_ssrnx_android_fd_isset(writefds, fd) ||
            compat_ssrnx_android_fd_isset(exceptfds, fd)) ++count;
    if (count > 64u) {
        fds = (compat_SsrnxAndroidPollfd *)calloc(count, sizeof(*fds));
        if (!fds) { errno = ENOMEM; return -1; }
    }
    const int entry_errno = errno;
    int64_t duration_us = -1;
    if (timeout)
        duration_us = timeout->tv_sec > (INT64_MAX - timeout->tv_usec) / INT64_C(1000000)
            ? INT64_MAX : timeout->tv_sec * INT64_C(1000000) + timeout->tv_usec;
    const int64_t start_ms = compat_ssrnx_poll_monotonic_ms();
    static _Thread_local compat_SsrnxNetworkWaitTrace trace_state;
    const compat_SsrnxNetworkWaitCall trace = compat_ssrnx_network_wait_begin(&trace_state, "select", (uint32_t)nfds,
        duration_us, caller);
    uint32_t index = 0;
    for (int fd = 0; fd < nfds; ++fd) {
        const short events = (compat_ssrnx_android_fd_isset(readfds, fd) ? SSRNX_ANDROID_POLLIN : 0) |
                             (compat_ssrnx_android_fd_isset(writefds, fd) ? SSRNX_ANDROID_POLLOUT : 0) |
                             (compat_ssrnx_android_fd_isset(exceptfds, fd) ? SSRNX_ANDROID_POLLPRI : 0);
        if (!events) continue;
        fds[index++] = (compat_SsrnxAndroidPollfd){fd, events, 0};
        compat_ssrnx_network_wait_fd(&trace, "select", "begin", (uint32_t)fd, fd, events, 0, events, 0);
    }
    int ready = 0, error = 0;
    uint64_t calls = 0;
    int last_rc = 0, wait_ms = -1;
    int64_t remaining_us = duration_us;
    for (;;) {
        if (timeout) {
            const int64_t elapsed_us = (compat_ssrnx_poll_monotonic_ms() - start_ms) * INT64_C(1000);
            remaining_us = elapsed_us < duration_us ? duration_us - elapsed_us : 0;
            const int64_t ms = remaining_us / 1000 + (remaining_us % 1000 != 0);
            wait_ms = ms > INT_MAX ? INT_MAX : (int)ms;
        }
        errno = entry_errno;
        last_rc = compat_ssrnx_bionic_poll(fds, count, wait_ms);
        ++calls;
        if (last_rc < 0) { error = ssrnx_errno_android_to_host(*ssrnx_guest_errno_location()); break; }
        for (uint32_t i = 0; i < count; ++i) {
            const unsigned events = (uint16_t)fds[i].events;
            const unsigned revents = (uint16_t)fds[i].revents;
            if (revents & SSRNX_ANDROID_POLLNVAL) { error = EBADF; break; }
            if ((events & SSRNX_ANDROID_POLLIN) &&
                (revents & (SSRNX_ANDROID_POLLIN | SSRNX_ANDROID_POLLHUP | SSRNX_ANDROID_POLLERR))) ++ready;
            if ((events & SSRNX_ANDROID_POLLOUT) &&
                (revents & (SSRNX_ANDROID_POLLOUT | SSRNX_ANDROID_POLLERR))) ++ready;
            if ((events & SSRNX_ANDROID_POLLPRI) && (revents & SSRNX_ANDROID_POLLPRI)) ++ready;
            /* An exception-only wait must not wake or spin on a closed peer. */
            if (events == SSRNX_ANDROID_POLLPRI && (revents & (SSRNX_ANDROID_POLLHUP | SSRNX_ANDROID_POLLERR)) &&
                !(revents & SSRNX_ANDROID_POLLPRI)) fds[i].fd = -1;
        }
        if (ready || error) break;
        if (timeout && (compat_ssrnx_poll_monotonic_ms() - start_ms) * INT64_C(1000) >= duration_us) break;
    }
    const int saved = last_rc < 0 ? errno : entry_errno;
    if (timeout) {
        const int64_t elapsed_us = (compat_ssrnx_poll_monotonic_ms() - start_ms) * INT64_C(1000);
        remaining_us = elapsed_us < duration_us ? duration_us - elapsed_us : 0;
        timeout->tv_sec = remaining_us / INT64_C(1000000);
        timeout->tv_usec = remaining_us % INT64_C(1000000);
    }
    compat_ssrnx_network_wait_end(&trace_state, &trace, "select", error ? -1 : ready, error, calls, last_rc, wait_ms);
    if (!error) {
        if (readfds) memset(readfds, 0, sizeof(*readfds));
        if (writefds) memset(writefds, 0, sizeof(*writefds));
        if (exceptfds) memset(exceptfds, 0, sizeof(*exceptfds));
        for (uint32_t i = 0; i < count; ++i) {
            const int fd = fds[i].fd;
            if (fd < 0) continue;
            const unsigned events = (uint16_t)fds[i].events;
            const unsigned revents = (uint16_t)fds[i].revents;
            unsigned result = 0;
            if ((events & SSRNX_ANDROID_POLLIN) &&
                (revents & (SSRNX_ANDROID_POLLIN | SSRNX_ANDROID_POLLHUP | SSRNX_ANDROID_POLLERR))) {
                compat_ssrnx_android_fd_set(readfds, fd); result |= SSRNX_ANDROID_POLLIN;
            }
            if ((events & SSRNX_ANDROID_POLLOUT) && (revents & (SSRNX_ANDROID_POLLOUT | SSRNX_ANDROID_POLLERR))) {
                compat_ssrnx_android_fd_set(writefds, fd); result |= SSRNX_ANDROID_POLLOUT;
            }
            if ((events & SSRNX_ANDROID_POLLPRI) && (revents & SSRNX_ANDROID_POLLPRI)) {
                compat_ssrnx_android_fd_set(exceptfds, fd); result |= SSRNX_ANDROID_POLLPRI;
            }
            compat_ssrnx_network_wait_fd(&trace, "select", "end", (uint32_t)fd, fd, events, result, events, revents);
        }
    }
    if (fds != stack_fds) free(fds);
    errno = saved;
    if (error && last_rc >= 0) { errno = error; compat_ssrnx_network_fix_errno(); }
    return error ? -1 : ready;
}





/* epoll */
_Static_assert(sizeof(SsrnxEpollEvent)==16 && offsetof(SsrnxEpollEvent,data)==8,"Android arm64 epoll ABI");
_Static_assert(sizeof(SsrnxEpollPollfd)==sizeof(compat_SsrnxAndroidPollfd),"pollfd ABI");
static int compat_ssrnx_epoll_poll_bridge(SsrnxEpollPollfd *fds,uint32_t n,int timeout) {
    return compat_ssrnx_bionic_poll((compat_SsrnxAndroidPollfd *)fds,n,timeout);
}
int ssrnx_compat_epoll_poll(int fd,short events) {
    return ssrnx_epoll_poll ? ssrnx_epoll_poll(fd,events,compat_ssrnx_epoll_poll_bridge) : -1;
}
static int compat_ssrnx_epoll_trace(const char *api,int descriptor,int op,int target,int result,uintptr_t caller) {
    const int saved=errno;
    static unsigned reports;
    if (__atomic_fetch_add(&reports,1u,__ATOMIC_RELAXED)<64u) {
        char site[192];(void)ssrnx_compat_describe_guest_address(caller,site,sizeof(site));
        SSRNX_LOGCI("EPOLL","api=%s epfd=%d op=%d target=%d result=%d errno=%d caller=%s",
            api,descriptor,op,target,result,result<0 ? ssrnx_errno_host_to_android(saved) : 0,site);
    }
    errno=saved;return result;
}
static int compat_ssrnx_bionic_epoll_create1(int flags) {
    int rc;
    if (ssrnx_epoll_create) rc=ssrnx_epoll_create(flags);
    else { errno=ENOSYS;rc=-1; }
    return compat_ssrnx_epoll_trace("create1",rc,flags,-1,rc,(uintptr_t)__builtin_return_address(0));
}
static int compat_ssrnx_bionic_epoll_create(int size) {
    if (size<=0) { errno=EINVAL;return -1; }
    return compat_ssrnx_bionic_epoll_create1(0);
}
static int compat_ssrnx_bionic_epoll_ctl(int epfd,int op,int fd,const SsrnxEpollEvent *event) {
    int rc=-1;
    if (op!=2 && !compat_ssrnx_process_vm_range_valid(event,sizeof(*event),false)) errno=EFAULT;
    else {
        struct stat st;
        if (ssrnx_fd_is_virtual(fd)) {
            if (ssrnx_fd_is_random(fd)) errno=EPERM;
            else rc=0;
        } else if (fstat(fd,&st)!=0) errno=EBADF;
        else if (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode)) errno=EPERM;
        else rc=0;
        if (rc==0) {
            if (ssrnx_epoll_ctl) rc=ssrnx_epoll_ctl(epfd,op,fd,event);
            else { errno=ENOSYS;rc=-1; }
        }
    }
    return compat_ssrnx_epoll_trace("ctl",epfd,op,fd,rc,(uintptr_t)__builtin_return_address(0));
}
static int compat_ssrnx_bionic_epoll_wait(int epfd,SsrnxEpollEvent *events,int maximum,int timeout) {
    int rc=-1;
    if (maximum<=0 || maximum>INT_MAX/(int)sizeof(*events)) errno=EINVAL;
    else if (!compat_ssrnx_process_vm_range_valid(events,(size_t)maximum*sizeof(*events),true)) errno=EFAULT;
    else if (ssrnx_epoll_wait) rc=ssrnx_epoll_wait(epfd,events,maximum,timeout,compat_ssrnx_epoll_poll_bridge);
    else errno=ENOSYS;
    return compat_ssrnx_epoll_trace("wait",epfd,maximum,timeout,rc,(uintptr_t)__builtin_return_address(0));
}
static int compat_ssrnx_bionic_epoll_pwait(int epfd,SsrnxEpollEvent *events,int maximum,int timeout,const uint64_t *mask) {
    /* No atomic guest-mask/host-wait backend yet. Do not emulate it with a
     * racy sigprocmask + wait pair. A null mask has epoll_wait semantics. */
    if (mask) { errno=EOPNOTSUPP;return -1; }
    return compat_ssrnx_bionic_epoll_wait(epfd,events,maximum,timeout);
}
long ssrnx_epoll_syscall(long number,const uintptr_t *a) {
    if (number==20) return compat_ssrnx_bionic_epoll_create1((int)a[0]);
    if (number==21) return compat_ssrnx_bionic_epoll_ctl((int)a[0],(int)a[1],(int)a[2],(const SsrnxEpollEvent *)a[3]);
    if (a[4] && a[5]!=8) { errno=EINVAL;return -1; }
    return compat_ssrnx_bionic_epoll_pwait((int)a[0],(SsrnxEpollEvent *)a[1],(int)a[2],(int)a[3],(const uint64_t *)a[4]);
}



/* libc extras */
/* Bionic libc / libz symbols required by bundled NDK libraries whose strong
 * imports must all bind before a DSO may load (Bionic linker semantics).
 *
 * Each entry keeps the Bionic ABI (LP64 layouts, FILE* via ssrnx_host_file,
 * Android struct tm) and either implements the contract on Horizon or fails
 * with the errno Android itself reports when the facility is unavailable. */

#include <getopt.h>
#include <sched.h>
#include <wchar.h>
#include <wctype.h>

/* ---------- byte strings, formatting and classification ---------- */
static char *compat_ssrnx_bionic_strndup(const char *s, size_t limit) {
    if (!s) { errno = EFAULT; return NULL; }
    const size_t n = compat_ssrnx_bionic_strnlen(s, limit);
    if (n == SIZE_MAX) { errno = ENOMEM; return NULL; }
    char *out = malloc(n + 1);
    if (!out) { errno = ENOMEM; return NULL; }
    memcpy(out, s, n);
    out[n] = '\0';
    return out;
}

static void *compat_ssrnx_bionic_memmem(const void *haystack, size_t length,
                                const void *needle, size_t needle_length) {
    if (!needle_length) return (void *)haystack;
    if (needle_length > length) return NULL;
    const unsigned char *bytes = haystack;
    const size_t last = length - needle_length;
    for (size_t i = 0; i <= last; ++i)
        if (memcmp(bytes + i, needle, needle_length) == 0) return (void *)(bytes + i);
    return NULL;
}

static int compat_ssrnx_bionic_asprintf(char **out, const char *format, ...) {
    va_list args;
    va_start(args, format);
    const int result = compat_ssrnx_bionic_vasprintf(out, format, args);
    va_end(args);
    return result;
}

static int compat_ssrnx_bionic_isdigit(int c) { return c >= '0' && c <= '9'; }
static int compat_ssrnx_bionic_isfinite(double value) { return isfinite(value) ? 1 : 0; }

/* ---------- unavailable process facilities ---------- */
static int compat_ssrnx_bionic_wait4(int pid, int *status, int options, void *usage) {
    /* No guest child-process backend: preserve waitpid's ENOSYS and leave
     * status/rusage untouched instead of exposing Horizon/host children. */
    (void)usage;
    return compat_ssrnx_bionic_waitpid(pid, status, options);
}

static int compat_ssrnx_bionic_getrusage(int who, void *usage) {
    if (who != 0 && who != -1 && who != 1) { errno = EINVAL; return -1; }
    if (!usage) { errno = EFAULT; return -1; }
    /* Horizon has no Linux rusage accounting backend; do not invent CPU/RSS. */
    SSRNX_LOGCW("BIONIC", "getrusage(%d) unavailable: no guest resource accounting", who);
    errno = ENOSYS;
    return -1;
}

static int compat_ssrnx_bionic_settimeofday(const compat_SsrnxAndroidTimeval *tv, const void *tz) {
    (void)tz;
    if (tv && (tv->tv_sec < 0 || tv->tv_usec < 0 || tv->tv_usec >= 1000000)) {
        errno = EINVAL; return -1;
    }
    /* An unprivileged Android app cannot set the system wall clock. */
    errno = EPERM;
    return -1;
}

/* The existing setitimer bridge also rejects unsupported process timers.
 * Binding the POSIX family must not claim that a notification was scheduled. */
static int compat_ssrnx_bionic_timer_create(int32_t clock, const void *event, uintptr_t *timer) {
    (void)event;
    if (!timer) { errno = EFAULT; return -1; }
    if (clock >= 0 && clock != 0 && clock != 1 && clock != 2 && clock != 3 && clock != 7 && clock != 8 && clock != 9) {
        errno = EINVAL; return -1;
    }
    if (clock == 8 || clock == 9) { errno = EPERM; return -1; }
    SSRNX_LOGCW("BIONIC", "timer_create clock=%d unavailable: no process timer backend", clock);
    errno = ENOSYS;
    return -1;
}

static int compat_ssrnx_bionic_timer_delete(uintptr_t timer) {
    (void)timer;
    errno = EINVAL; /* No timer IDs are issued by this backend. */
    return -1;
}

static int compat_ssrnx_bionic_timer_settime(uintptr_t timer, int flags, const void *value, void *old_value) {
    (void)timer; (void)flags; (void)value; (void)old_value;
    errno = EINVAL;
    return -1;
}

static int compat_ssrnx_bionic_timer_gettime(uintptr_t timer, void *value) {
    (void)timer; (void)value;
    errno = EINVAL;
    return -1;
}

static int compat_ssrnx_bionic_timer_getoverrun(uintptr_t timer) {
    (void)timer;
    errno = EINVAL;
    return -1;
}

/* ---------- FORTIFY (bionic/libc/bionic/fortify.cpp) ---------- */

static char *compat_ssrnx_strchr_chk(const char *p, int ch, size_t s_len) {
    for (;; ++p, s_len--) {
        if (s_len == 0) compat_ssrnx_fortify_fail("__strchr_chk");
        if (*p == (char)ch) return (char *)p;
        if (*p == '\0') return NULL;
    }
}

static char *compat_ssrnx_strrchr_chk(const char *p, int ch, size_t s_len) {
    const char *result = NULL;
    for (;; ++p, s_len--) {
        if (s_len == 0) compat_ssrnx_fortify_fail("__strrchr_chk");
        if (*p == (char)ch) result = p;
        if (*p == '\0') return (char *)result;
    }
}

static void *compat_ssrnx_memchr_chk(const void *s, int c, size_t n, size_t actual_size) {
    if (n > actual_size) compat_ssrnx_fortify_fail("__memchr_chk");
    return (void *)(uintptr_t)memchr(s, c, n);
}

static char *compat_ssrnx_strcpy_chk(char *dst, const char *src, size_t dst_len) {
    const size_t src_len = strlen(src) + 1;
    if (src_len > dst_len) compat_ssrnx_fortify_fail("__strcpy_chk");
    return memcpy(dst, src, src_len);
}

static char *compat_ssrnx_strcat_chk(char *dst, const char *src, size_t dst_buf_size) {
    char *save = dst;
    size_t dst_len = 0;
    while (*dst) {
        dst++;
        if (++dst_len >= dst_buf_size) compat_ssrnx_fortify_fail("__strcat_chk");
    }
    while ((*dst++ = *src++) != '\0') {
        if (++dst_len >= dst_buf_size) compat_ssrnx_fortify_fail("__strcat_chk");
    }
    return save;
}

static char *compat_ssrnx_strncat_chk(char *dst, const char *src, size_t len, size_t dst_buf_size) {
    if (len == 0) return dst;
    size_t dst_len = strlen(dst);
    if (dst_len >= dst_buf_size) compat_ssrnx_fortify_fail("__strncat_chk");
    size_t src_len = strnlen(src, len);
    if (dst_len + src_len + 1 > dst_buf_size) compat_ssrnx_fortify_fail("__strncat_chk");
    memcpy(dst + dst_len, src, src_len);
    dst[dst_len + src_len] = '\0';
    return dst;
}

static char *compat_ssrnx_strncpy_chk(char *dst, const char *src, size_t len, size_t dst_len) {
    if (len > dst_len) compat_ssrnx_fortify_fail("__strncpy_chk");
    return strncpy(dst, src, len);
}

/* clang's variant: also knows the source object size. */
static char *compat_ssrnx_strncpy_chk2(char *dst, const char *src, size_t n, size_t dst_len, size_t src_len) {
    if (n > dst_len) compat_ssrnx_fortify_fail("__strncpy_chk2");
    if (n >= src_len) {
        const size_t copied = strnlen(src, src_len);
        if (copied == src_len) compat_ssrnx_fortify_fail("__strncpy_chk2");
    }
    return strncpy(dst, src, n);
}

static size_t compat_ssrnx_strlcpy_chk(char *dst, const char *src, size_t size, size_t dst_len) {
    if (size > dst_len) compat_ssrnx_fortify_fail("__strlcpy_chk");
    const size_t src_len = strlen(src);
    if (size) {
        const size_t n = src_len >= size ? size - 1 : src_len;
        memcpy(dst, src, n);
        dst[n] = '\0';
    }
    return src_len;
}

static size_t compat_ssrnx_fwrite_chk(const void *buf, size_t size, size_t count, void *stream, size_t buf_size) {
    if (size && count > SIZE_MAX / size) compat_ssrnx_fortify_fail("__fwrite_chk");
    if (size * count > buf_size) compat_ssrnx_fortify_fail("__fwrite_chk");
    return compat_ssrnx_bionic_fwrite(buf, size, count, stream);
}

static ssize_t compat_ssrnx_pread64_chk(int fd, void *buf, size_t count, int64_t offset, size_t buf_size) {
    if (count > buf_size) compat_ssrnx_fortify_fail("__pread64_chk");
    return compat_ssrnx_bionic_pread(fd, buf, count, offset);
}

static ssize_t compat_ssrnx_readlink_chk(const char *path, char *buf, size_t size, size_t buf_size) {
    if (size > buf_size) compat_ssrnx_fortify_fail("__readlink_chk");
    return compat_ssrnx_bionic_readlink(path, buf, size);
}

static uint32_t compat_ssrnx_umask_chk(uint32_t mode) {
    if ((mode & 0777u) != mode) compat_ssrnx_fortify_fail("__umask_chk");
    return compat_ssrnx_bionic_umask(mode);
}

/* Bionic fd_set: 1024 bits in unsigned long words (LP64). */
static void compat_ssrnx_bionic_FD_CLR_chk(int fd, void *set, size_t set_size) {
    if (fd < 0 || fd >= 1024) compat_ssrnx_fortify_fail("__FD_CLR_chk");
    if (set_size < 128u) compat_ssrnx_fortify_fail("__FD_CLR_chk");
    unsigned long *bits = (unsigned long *)set;
    bits[(unsigned)fd / 64u] &= ~(1UL << ((unsigned)fd % 64u));
}

/* ---------- stdio / wide characters ---------- */

static int compat_ssrnx_bionic_putchar(int c) {
    unsigned char ch = (unsigned char)c;
    return compat_ssrnx_bionic_fwrite(&ch, 1, 1, &compat_g_bionic_sF[1]) == 1 ? (int)ch : EOF;
}

static void compat_ssrnx_bionic_perror(const char *s) {
    const int err = *compat_ssrnx_bionic_errno();
    const char *msg = compat_ssrnx_bionic_strerror(err);
    char line[512];
    const int n = snprintf(line, sizeof(line), "%s%s%s\n", s && *s ? s : "", s && *s ? ": " : "",
                           msg ? msg : "Unknown error");
    if (n > 0) (void)compat_ssrnx_bionic_fwrite(line, 1, (size_t)n < sizeof(line) ? (size_t)n : sizeof(line) - 1,
                                         &compat_g_bionic_sF[2]);
}

static wint_t compat_ssrnx_bionic_fputwc(wchar_t wc, void *stream) {
    return fputwc(wc, compat_ssrnx_host_file(stream));
}

static int compat_ssrnx_bionic_fwide(void *stream, int mode) {
    return fwide(compat_ssrnx_host_file(stream), mode);
}

/* Bionic fpos_t is a 64-bit file offset. */
static int compat_ssrnx_bionic_fsetpos(void *stream, const int64_t *pos) {
    if (!pos) { errno = EINVAL; return -1; }
    return fseeko(compat_ssrnx_host_file(stream), (off_t)*pos, SEEK_SET);
}

static int compat_ssrnx_bionic_swscanf(const wchar_t *s, const wchar_t *format, ...) {
    va_list ap;
    va_start(ap, format);
    const int rc = vswscanf(s, format, ap);
    va_end(ap);
    return rc;
}

/* ---------- strings / paths ---------- */

/* Bionic dirname(3) never modifies its argument and returns a per-thread
 * buffer: "" or NULL -> ".", "/" -> "/", "a/b/" -> "a", "a" -> ".". */
static __thread char compat_g_bionic_dirname[SSRNX_STORAGE_PATH_CAP];
static char *compat_ssrnx_bionic_dirname(const char *path) {
    if (!path || !*path) { strcpy(compat_g_bionic_dirname, "."); return compat_g_bionic_dirname; }
    size_t end = strlen(path);
    while (end > 1 && path[end - 1] == '/') end--;
    while (end > 0 && path[end - 1] != '/') end--;
    if (end == 0) { strcpy(compat_g_bionic_dirname, path[0] == '/' ? "/" : "."); return compat_g_bionic_dirname; }
    while (end > 1 && path[end - 1] == '/') end--;
    if (end >= sizeof(compat_g_bionic_dirname)) { errno = ENAMETOOLONG; return NULL; }
    memcpy(compat_g_bionic_dirname, path, end);
    compat_g_bionic_dirname[end] = '\0';
    return compat_g_bionic_dirname;
}

static char *compat_ssrnx_bionic_strsep(char **stringp, const char *delim) {
    return strsep(stringp, delim);
}

/* ---------- time ---------- */

static int64_t compat_ssrnx_days_from_civil(int64_t y, int64_t m, int64_t d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const int64_t yoe = y - era * 400;
    const int64_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

/* timegm(3): normalizes *tm like mktime but in UTC. */
static int64_t compat_ssrnx_bionic_timegm(compat_SsrnxAndroidTm *tm) {
    if (!tm) { errno = EINVAL; return -1; }
    int64_t year = (int64_t)tm->tm_year + 1900;
    int64_t mon = tm->tm_mon;
    year += mon / 12;
    mon %= 12;
    if (mon < 0) { mon += 12; year--; }
    const int64_t days = compat_ssrnx_days_from_civil(year, mon + 1, 1) + (tm->tm_mday - 1);
    const int64_t t = days * 86400 + (int64_t)tm->tm_hour * 3600 + (int64_t)tm->tm_min * 60 + tm->tm_sec;
    const time_t host_t = (time_t)t;
    struct tm h;
    if (gmtime_r(&host_t, &h)) compat_ssrnx_copy_tm(tm, &h, "UTC");
    return t;
}

/* asctime/ctime share one static buffer, as specified by C. */
static char compat_g_bionic_asctime[32];
static char *compat_ssrnx_bionic_asctime(const compat_SsrnxAndroidTm *tm) {
    if (!tm) { errno = EINVAL; return NULL; }
    static const char wday[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char mon[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    if (tm->tm_wday < 0 || tm->tm_wday > 6 || tm->tm_mon < 0 || tm->tm_mon > 11) {
        errno = EINVAL;
        return NULL;
    }
    const int n = snprintf(compat_g_bionic_asctime, sizeof(compat_g_bionic_asctime), "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",
                           wday[tm->tm_wday], mon[tm->tm_mon], tm->tm_mday, tm->tm_hour, tm->tm_min,
                           tm->tm_sec, 1900 + tm->tm_year);
    if (n < 0 || (size_t)n >= sizeof(compat_g_bionic_asctime)) { errno = EOVERFLOW; return NULL; }
    return compat_g_bionic_asctime;
}

static char *compat_ssrnx_bionic_ctime(const int64_t *t) {
    compat_SsrnxAndroidTm tm;
    if (!compat_ssrnx_bionic_localtime_r(t, &tm)) return NULL;
    return compat_ssrnx_bionic_asctime(&tm);
}

/* tzset(3) globals: seconds west of UTC, DST flag, standard/DST names. */
static long compat_g_bionic_timezone;
static int compat_g_bionic_daylight;
static char compat_g_bionic_tzname_std[16] = "GMT";
static char compat_g_bionic_tzname_dst[16] = "GMT";
static char *compat_g_bionic_tzname[2] = {compat_g_bionic_tzname_std, compat_g_bionic_tzname_dst};

static void compat_ssrnx_bionic_tzset(void) {
    tzset();
    const time_t now = time(NULL);
    struct tm local;
    if (!localtime_r(&now, &local)) return;
    compat_g_bionic_timezone = -(long)compat_ssrnx_tm_offset_seconds(&local);
    compat_g_bionic_daylight = local.tm_isdst > 0;
    char zone[16] = "";
    if (strftime(zone, sizeof(zone), "%Z", &local) > 0) {
        snprintf(compat_g_bionic_tzname_std, sizeof(compat_g_bionic_tzname_std), "%s", zone);
        snprintf(compat_g_bionic_tzname_dst, sizeof(compat_g_bionic_tzname_dst), "%s", zone);
    }
}

/* ---------- process / scheduler ---------- */

static int compat_ssrnx_bionic_system(const char *command) {
    /* Horizon has no command interpreter. system(NULL) reports that; a
     * command fails like a shell that could not be started. */
    if (!command) return 0;
    SSRNX_LOGCW("BIONIC", "system(\"%.120s\") unavailable: no command interpreter", command);
    errno = ENOSYS;
    return -1;
}

static int compat_ssrnx_bionic_execv(const char *path, char *const argv[]) {
    (void)argv;
    SSRNX_LOGCW("BIONIC", "execv(%s) unavailable: single-process runtime", path ? path : "(null)");
    errno = ENOSYS;
    return -1;
}

static int compat_ssrnx_bionic_execve(const char *path, char *const argv[], char *const envp[]) {
    (void)envp;
    return compat_ssrnx_bionic_execv(path, argv);
}

static int compat_ssrnx_bionic_execlp(const char *file, const char *arg, ...) {
    (void)arg;
    return compat_ssrnx_bionic_execv(file, NULL);
}

/* The app process is forked from zygote; its parent is not observable. */
static int compat_ssrnx_bionic_getppid(void) {
    return 1;
}

/* Linux policy ids: SCHED_OTHER 0, SCHED_FIFO 1, SCHED_RR 2, SCHED_BATCH 3,
 * SCHED_IDLE 5; real-time priorities are 1..99, others 0. */
static int compat_ssrnx_bionic_sched_get_priority_max(int policy) {
    if (policy == 1 || policy == 2) return 99;
    if (policy == 0 || policy == 3 || policy == 5) return 0;
    errno = EINVAL;
    return -1;
}

static int compat_ssrnx_bionic_sched_get_priority_min(int policy) {
    if (policy == 1 || policy == 2) return 1;
    if (policy == 0 || policy == 3 || policy == 5) return 0;
    errno = EINVAL;
    return -1;
}

static int compat_ssrnx_bionic_sched_getscheduler(int pid) {
    (void)pid;
    return 0; /* SCHED_OTHER */
}

static int compat_ssrnx_bionic_sched_getparam(int pid, int *param) {
    (void)pid;
    if (!param) { errno = EINVAL; return -1; }
    *param = 0;
    return 0;
}

typedef struct compat_SsrnxAndroidSysinfo {
    int64_t uptime;
    uint64_t loads[3];
    uint64_t totalram;
    uint64_t freeram;
    uint64_t sharedram;
    uint64_t bufferram;
    uint64_t totalswap;
    uint64_t freeswap;
    uint16_t procs;
    uint16_t pad;
    uint64_t totalhigh;
    uint64_t freehigh;
    uint32_t mem_unit;
} compat_SsrnxAndroidSysinfo;
_Static_assert(sizeof(compat_SsrnxAndroidSysinfo) == 112, "Linux LP64 struct sysinfo size");

static int compat_ssrnx_bionic_sysinfo(compat_SsrnxAndroidSysinfo *info) {
    if (!info) { errno = EFAULT; return -1; }
    memset(info, 0, sizeof(*info));
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) info->uptime = (int64_t)ts.tv_sec;
    uint64_t total = 0, available = 0;
    compat_ssrnx_process_memory(&total, &available);
    info->totalram = total;
    info->freeram = available;
    info->procs = 1;
    info->mem_unit = 1;
    return 0;
}

/* Ownership: Horizon storage has no per-file owners. Keeping the caller's own
 * uid/gid (or -1 = unchanged) succeeds; any other owner needs CAP_CHOWN. */
static int compat_ssrnx_chown_check(uint32_t owner, uint32_t group) {
    if ((owner == UINT32_MAX || owner == compat_ssrnx_bionic_getuid()) &&
        (group == UINT32_MAX || group == compat_ssrnx_bionic_getgid())) return 0;
    errno = EPERM;
    return -1;
}

static int compat_ssrnx_bionic_fchown(int fd, uint32_t owner, uint32_t group) {
    struct stat st;
    if (fstat(fd, &st) != 0) return -1;
    return compat_ssrnx_chown_check(owner, group);
}

static int compat_ssrnx_bionic_lchown(const char *path, uint32_t owner, uint32_t group) {
    if (!path) { errno = EFAULT; return -1; }
    char mapped[SSRNX_STORAGE_PATH_CAP];
    bool path_mapped = false;
    struct stat st;
    if (stat(compat_ssrnx_map_guest_path(path, mapped, sizeof(mapped), &path_mapped), &st) != 0) return -1;
    return compat_ssrnx_chown_check(owner, group);
}

/* Special files cannot be created on Horizon storage (Linux EPERM for a
 * filesystem without special-file support). */
static int compat_ssrnx_bionic_mknod(const char *path, uint32_t mode, uint64_t dev) {
    (void)path; (void)mode; (void)dev;
    errno = EPERM;
    return -1;
}

static int compat_ssrnx_bionic_mkfifo(const char *path, uint32_t mode) {
    return compat_ssrnx_bionic_mknod(path, mode, 0);
}

/* Bionic reports "no such user/group" for anything but the app's own ids;
 * SSRNX exposes no passwd/group database. */
static void *compat_ssrnx_bionic_getpwnam(const char *name) {
    if (!name) { errno = EINVAL; return NULL; }
    if (strcmp(name, compat_g_pw_name) == 0) return &compat_g_pw;
    errno = ENOENT;
    return NULL;
}

static void *compat_ssrnx_bionic_getgrnam(const char *name) {
    (void)name;
    errno = ENOENT;
    return NULL;
}

static void *compat_ssrnx_bionic_getgrgid(uint32_t gid) {
    (void)gid;
    errno = ENOENT;
    return NULL;
}

/* ---------- randomness ---------- */

static ssize_t compat_ssrnx_bionic_getrandom(void *buffer, size_t length, unsigned flags) {
    if (flags & ~3u) { errno = EINVAL; return -1; }
    /* Linux's urandom path permits short reads for very large requests. */
    const size_t limit = (flags & 2u) ? 512u : 33554431u;
    if (length > limit) length = limit;
    if (length && !compat_ssrnx_process_vm_range_valid(buffer, length, true)) { errno = EFAULT; return -1; }
    ssrnx_fd_random_fill(buffer, length);
    return (ssize_t)length;
}

static int compat_ssrnx_bionic_getentropy(void *buffer, size_t length) {
    if (length > 256u) { errno = EIO; return -1; }
    if (length && !buffer) { errno = EFAULT; return -1; }
    ssrnx_fd_random_fill(buffer, length);
    return 0;
}

static uint32_t compat_ssrnx_bionic_arc4random(void) {
    uint32_t v = 0;
    ssrnx_fd_random_fill(&v, sizeof(v));
    return v;
}

static void compat_ssrnx_bionic_arc4random_buf(void *buffer, size_t length) {
    ssrnx_fd_random_fill(buffer, length);
}

/* ---------- sockets ---------- */

typedef struct compat_SsrnxAndroidCmsghdr {
    size_t cmsg_len;
    int32_t cmsg_level;
    int32_t cmsg_type;
} compat_SsrnxAndroidCmsghdr;

static compat_SsrnxAndroidCmsghdr *compat_ssrnx_bionic_cmsg_nxthdr(compat_SsrnxAndroidMsghdr *msg, compat_SsrnxAndroidCmsghdr *cmsg) {
    if (!msg || !cmsg) return NULL;
    const size_t align = sizeof(long);
    const size_t step = (cmsg->cmsg_len + align - 1u) & ~(align - 1u);
    compat_SsrnxAndroidCmsghdr *next = (compat_SsrnxAndroidCmsghdr *)((char *)cmsg + step);
    const size_t end = (size_t)((char *)(next + 1) - (char *)msg->msg_control);
    return end > msg->msg_controllen ? NULL : next;
}

typedef struct compat_SsrnxAndroidMmsghdr {
    compat_SsrnxAndroidMsghdr msg_hdr;
    uint32_t msg_len;
    uint32_t _pad;
} compat_SsrnxAndroidMmsghdr;
_Static_assert(sizeof(compat_SsrnxAndroidMmsghdr) == 64, "Bionic LP64 mmsghdr size");

#define SSRNX_ANDROID_MSG_DONTWAIT 0x40
#define SSRNX_ANDROID_MSG_WAITFORONE 0x10000

static int compat_ssrnx_bionic_sendmmsg(int fd, compat_SsrnxAndroidMmsghdr *vec, unsigned vlen, int flags) {
    if (!vec && vlen) { errno = EFAULT; return -1; }
    unsigned sent = 0;
    for (; sent < vlen; ++sent) {
        const ssize_t n = compat_ssrnx_bionic_sendmsg(fd, &vec[sent].msg_hdr, flags);
        if (n < 0) return sent ? (int)sent : -1;
        vec[sent].msg_len = (uint32_t)n;
    }
    return (int)sent;
}

static int compat_ssrnx_bionic_recvmmsg(int fd, compat_SsrnxAndroidMmsghdr *vec, unsigned vlen, int flags, void *timeout) {
    if (!vec && vlen) { errno = EFAULT; return -1; }
    (void)timeout; /* evaluated only between datagrams; each receive blocks per flags */
    unsigned got = 0;
    int f = flags & ~SSRNX_ANDROID_MSG_WAITFORONE;
    for (; got < vlen; ++got) {
        const ssize_t n = compat_ssrnx_bionic_recvmsg(fd, &vec[got].msg_hdr, f);
        if (n < 0) {
            if (got && (errno == EAGAIN || errno == EWOULDBLOCK)) errno = 0;
            return got ? (int)got : -1;
        }
        vec[got].msg_len = (uint32_t)n;
        if (flags & SSRNX_ANDROID_MSG_WAITFORONE) f |= SSRNX_ANDROID_MSG_DONTWAIT;
    }
    return (int)got;
}

/* Linux readiness/notification APIs have no Horizon backend: fail like a
 * kernel built without them. */
static int compat_ssrnx_bionic_enosys(void) {
    errno = ENOSYS;
    return -1;
}

/* ---------- libz extras (stable z_stream ABI, as for inflate*) ---------- */

/* gzopen resolves the Android path through the storage namespace. The gzFile
 * handed back is zlib's own: struct gzFile_s is public ABI (gzgetc reads it
 * inline), so gzread/gzclose bind straight to zlib. */
static gzFile compat_ssrnx_gzopen(const char *path, const char *mode) {
    if (!path || !mode) { errno = EINVAL; return NULL; }
    int android_flags = SSRNX_ANDROID_O_RDONLY;
    if (strchr(mode, 'w')) android_flags = SSRNX_ANDROID_O_WRONLY | SSRNX_ANDROID_O_CREAT | SSRNX_ANDROID_O_TRUNC;
    else if (strchr(mode, 'a')) android_flags = SSRNX_ANDROID_O_WRONLY | SSRNX_ANDROID_O_CREAT | SSRNX_ANDROID_O_APPEND;
    const int fd = (android_flags & SSRNX_ANDROID_O_CREAT) ? compat_ssrnx_bionic_open(path, android_flags, 0666)
                                                            : compat_ssrnx_bionic_open(path, android_flags);
    if (fd < 0) return NULL;
    gzFile gz = gzdopen(fd, mode);
    if (!gz) { close(fd); errno = ENOMEM; }
    return gz;
}

/* ---------- libm ---------- */

/* AArch64 long double is IEEE binary128. newlib does not provide binary128
 * transcendental functions on every target, so expl is evaluated in double
 * precision and widened (correct to double accuracy). */
static long double compat_ssrnx_bionic_expl(long double x) {
    return (long double)exp((double)x);
}



/* posix contracts */
/* Additional Bionic LP64 contracts, including counter-backed descriptor timers. */

extern int ssrnx_timerfd_create(int,int) __attribute__((weak));
extern int ssrnx_timerfd_settime(int,int,const SsrnxTimerSpec *,SsrnxTimerSpec *) __attribute__((weak));
extern int ssrnx_timerfd_gettime(int,SsrnxTimerSpec *) __attribute__((weak));
#ifdef __SWITCH__
_Static_assert(sizeof(long double) == 16 && LDBL_MANT_DIG == 113,
               "Android AArch64 long double requires binary128 math ABI");
#endif
static int compat_ssrnx_bionic_dirfd(DIR *dir) {
    if (!dir) { errno = EINVAL; return -1; }
#ifdef __SWITCH__
    /* fsdev DIR owns an iterator, not a file descriptor; newlib declares dirfd
     * but libnx supplies no implementation. Do not expose the iterator as an fd. */
    errno = ENOTSUP;
    return -1;
#else
    return dirfd(dir);
#endif
}

static char *compat_ssrnx_bionic_mkdtemp(char *pattern) {
    if (!pattern) { errno = EINVAL; return NULL; }
    size_t n = strlen(pattern);
    if (n < 6 || strcmp(pattern + n - 6, "XXXXXX")) { errno = EINVAL; return NULL; }
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (unsigned attempt = 0; attempt < 128; ++attempt) {
        unsigned char random[6];
        ssrnx_fd_random_fill(random, sizeof(random));
        for (unsigned i = 0; i < 6; ++i) pattern[n - 6 + i] = alphabet[random[i] % 62];
        if (compat_ssrnx_bionic_mkdir(pattern, 0700) == 0) return pattern;
        if (errno != EEXIST) return NULL;
    }
    errno = EEXIST;
    return NULL;
}

static int compat_ssrnx_bionic_getpwnam_r(const char *name, compat_SsrnxAndroidPasswd *out,
                                  char *buf, size_t size, compat_SsrnxAndroidPasswd **result) {
    if (!name || !out || !buf || !result) return EINVAL;
    *result = NULL;
    if (strcmp(name, compat_g_pw_name)) return 0;
    return compat_ssrnx_bionic_getpwuid_r(compat_g_pw.pw_uid, out, buf, size, result);
}

static int compat_ssrnx_bionic_execvpe(const char *file, char *const argv[], char *const envp[]) {
    return compat_ssrnx_bionic_execve(file, argv, envp);
}

static int compat_ssrnx_bionic_setsid(void) {
    SSRNX_LOGCW("BIONIC", "setsid unavailable: single-process runtime");
    errno = ENOSYS;
    return -1;
}

static int compat_ssrnx_bionic_nftw(const char *path, void *callback, int descriptors, int flags) {
    if (!path || !callback) { errno = EFAULT; return -1; }
    if (descriptors < 1 || (flags & ~15)) { errno = EINVAL; return -1; }
    SSRNX_LOGCW("BIONIC", "nftw unavailable: no guest tree-walk backend path=%s", path);
    errno = ENOSYS;
    return -1;
}

static int compat_ssrnx_bionic_eventfd(unsigned initial, int flags) {
    int result;
    if (ssrnx_eventfd_create) result = ssrnx_eventfd_create(initial, flags);
    else { errno = ENOSYS; result = -1; }
    const int saved = errno;
    static unsigned reports;
    if (__atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 16u)
        SSRNX_LOGCW("BIONIC", "eventfd initial=%u flags=%#x result=%d hostErrno=%d caller=%p",
                    initial, flags, result, result < 0 ? saved : 0,
                    __builtin_extract_return_addr(__builtin_return_address(0)));
    errno = saved;
    return result;
}
static int compat_ssrnx_bionic_eventfd_read(int fd, uint64_t *value) { return compat_ssrnx_bionic_read(fd, value, 8) == 8 ? 0 : -1; }
static int compat_ssrnx_bionic_eventfd_write(int fd, uint64_t value) { return compat_ssrnx_bionic_write(fd, &value, 8) == 8 ? 0 : -1; }

static int compat_ssrnx_bionic_timerfd_create(int clock, int flags) {
    if (flags & ~(SSRNX_ANDROID_O_CLOEXEC | SSRNX_ANDROID_O_NONBLOCK)) { errno = EINVAL; return -1; }
    if (clock != 0 && clock != 1 && clock != 7 && clock != 8 && clock != 9) { errno = EINVAL; return -1; }
    const int rc=ssrnx_timerfd_create ? ssrnx_timerfd_create(clock,flags) : (errno=ENOSYS,-1);
    const int saved=errno;
    SSRNX_LOGCI("BIONIC","timerfd_create clock=%d flags=%#x result=%d errno=%d caller=%p",clock,flags,rc,rc<0 ? ssrnx_errno_host_to_android(saved) : 0,__builtin_return_address(0));
    errno=saved;return rc;
}

static int compat_ssrnx_bionic_timerfd_settime(int fd, int flags, const void *value, void *old_value) {
    if (!compat_ssrnx_process_vm_range_valid(value,sizeof(SsrnxTimerSpec),false) ||
        (old_value && !compat_ssrnx_process_vm_range_valid(old_value,sizeof(SsrnxTimerSpec),true))) { errno=EFAULT;return -1; }
    const int rc=ssrnx_timerfd_settime ? ssrnx_timerfd_settime(fd,flags,value,old_value) : (errno=ENOSYS,-1);
    const int saved=errno;
    static unsigned reports;
    if (__atomic_fetch_add(&reports,1u,__ATOMIC_RELAXED)<32u)
        SSRNX_LOGCI("BIONIC","timerfd_settime fd=%d flags=%#x result=%d errno=%d caller=%p",
            fd,flags,rc,rc<0 ? ssrnx_errno_host_to_android(saved) : 0,__builtin_return_address(0));
    errno=saved;return rc;
}
static int compat_ssrnx_bionic_timerfd_gettime(int fd,void *value) {
    if (!compat_ssrnx_process_vm_range_valid(value,sizeof(SsrnxTimerSpec),true)) { errno=EFAULT;return -1; }
    return ssrnx_timerfd_gettime ? ssrnx_timerfd_gettime(fd,value) : (errno=ENOSYS,-1);
}
long ssrnx_timerfd_syscall(long number,const uintptr_t *a) {
    if (number==85) return compat_ssrnx_bionic_timerfd_create((int)a[0],(int)a[1]);
    if (number==86) return compat_ssrnx_bionic_timerfd_settime((int)a[0],(int)a[1],(const void *)a[2],(void *)a[3]);
    return compat_ssrnx_bionic_timerfd_gettime((int)a[0],(void *)a[1]);
}

static int compat_ssrnx_bionic_posix_fallocate(int fd, int64_t offset, int64_t length) {
    SSRNX_IO_TIMING();
    SSRNX_PERF_SCOPE(SSRNX_PERF_FILE_IO);
    const int saved = errno;
    int error;
    if (offset < 0 || length <= 0) error = EINVAL;
    else if (offset > INT64_MAX - length) error = EFBIG;
    else {
        const bool locked = compat_ssrnx_regular_file_io_lock(fd);
        error = ssrnx_file_contract_allocate ? ssrnx_file_contract_allocate(fd, offset, length) : ENOSYS;
        compat_ssrnx_regular_file_io_unlock(locked);
    }
    const int android_error = ssrnx_errno_host_to_android(error);
    static unsigned reports;
    if (__atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 16u)
        SSRNX_LOGCI("BIONIC", "posix_fallocate fd=%d offset=%lld length=%lld androidError=%d caller=%p",
                    fd, (long long)offset, (long long)length, android_error,
                    __builtin_extract_return_addr(__builtin_return_address(0)));
    errno = saved; /* POSIX returns the error number, without setting errno. */
    return android_error;
}

static int compat_ssrnx_bionic_sched_setscheduler(int pid, int policy, const int *param) {
    if (!param || pid < 0) { errno = EINVAL; return -1; }
    const int maximum = compat_ssrnx_bionic_sched_get_priority_max(policy);
    if (maximum < 0 || *param < compat_ssrnx_bionic_sched_get_priority_min(policy) || *param > maximum) {
        errno = EINVAL; return -1;
    }
    if (pid && pid != (int)getpid()) { errno = ESRCH; return -1; }
    errno = ENOSYS;
    return -1;
}

static int compat_ssrnx_bionic_sched_getcpu(void) {
#ifdef __SWITCH__
    return svcGetCurrentProcessorNumber();
#else
    return sched_getcpu();
#endif
}

static int compat_ssrnx_bionic_msync(void *addr, size_t length, int flags) {
    (void)length;
    if (((uintptr_t)addr & 4095u) || (flags & ~7) || ((flags & 5) == 5)) {
        errno = EINVAL; return -1;
    }
    SSRNX_LOGCW("BIONIC", "msync unavailable: no shared-map writeback backend");
    errno = ENOSYS;
    return -1;
}

/* Android LP64 mallinfo uses size_t fields; glibc/newlib layouts differ. */
typedef struct compat_SsrnxAndroidMallinfo {
    size_t arena, ordblks, smblks, hblks, hblkhd, usmblks, fsmblks, uordblks, fordblks, keepcost;
} compat_SsrnxAndroidMallinfo;
_Static_assert(sizeof(compat_SsrnxAndroidMallinfo) == 80, "Bionic LP64 mallinfo ABI");
static compat_SsrnxAndroidMallinfo compat_ssrnx_bionic_mallinfo(void) {
    const struct mallinfo info = mallinfo();
    compat_SsrnxAndroidMallinfo out = {0};
#ifdef __SWITCH__
#define SSRNX_MALLINFO_COPY(field) out.field = info.field
#else
#define SSRNX_MALLINFO_COPY(field) out.field = (unsigned int)info.field
#endif
    SSRNX_MALLINFO_COPY(arena); SSRNX_MALLINFO_COPY(ordblks); SSRNX_MALLINFO_COPY(smblks);
    SSRNX_MALLINFO_COPY(hblks); SSRNX_MALLINFO_COPY(hblkhd); SSRNX_MALLINFO_COPY(usmblks);
    SSRNX_MALLINFO_COPY(fsmblks); SSRNX_MALLINFO_COPY(uordblks); SSRNX_MALLINFO_COPY(fordblks);
    SSRNX_MALLINFO_COPY(keepcost);
#undef SSRNX_MALLINFO_COPY
    return out;
}

static int compat_ssrnx_bionic_finitef(float value) { return isfinite(value) ? 1 : 0; }

static void compat_ssrnx_bionic_assert(const char *file, int line, const char *expression) {
    SSRNX_LOGCE("ASSERT", "%s:%d assertion '%s' failed", file ? file : "?", line,
                expression ? expression : "?");
    compat_ssrnx_bionic_abort();
}

static size_t compat_ssrnx_fread_chk(void *buffer, size_t size, size_t count, void *stream, size_t capacity) {
    if (size && count > SIZE_MAX / size) compat_ssrnx_fortify_fail("__fread_chk");
    if (size * count > capacity) compat_ssrnx_fortify_fail("__fread_chk");
    return compat_ssrnx_bionic_fread(buffer, size, count, stream);
}

static ssize_t compat_ssrnx_sendto_chk(int fd, const void *buffer, size_t length, size_t capacity,
                               int flags, const compat_SsrnxAndroidSockaddr *addr, uint32_t addrlen) {
    if (length > capacity) compat_ssrnx_fortify_fail("__sendto_chk");
    return compat_ssrnx_bionic_sendto(fd, buffer, length, flags, addr, addrlen);
}

/* Bionic exports the actual 16-byte in6_addr objects, not pointer variables. */
static const unsigned char g_bionic_in6addr_any[16] __attribute__((aligned(4))) = {0};
static const unsigned char g_bionic_in6addr_loopback[16] __attribute__((aligned(4))) = {[15] = 1};



/* exports */
/* SSRNX 0.0.15 strict compatibility policy:
 *
 * Never rewrite application or bundled-engine function bodies to force a
 * desired decision or parser result. Guest code is mapped unchanged except
 * for architecture-level transformations that are required to execute Android
 * AArch64 on Horizon (for example the timer system-register translation in
 * elf64.c). Missing behavior must be implemented at an SSRNX-owned ABI or
 * Android framework boundary instead. */

static const compat_SsrnxCompatExport compat_k_exports[] = {
    /* ABI-stable byte/string operations. */
    {"memcpy",   SSRNX_FUNC_ADDR(memcpy),   "libc.so"},
    {"memmove",  SSRNX_FUNC_ADDR(memmove),  "libc.so"},
    {"memset",   SSRNX_FUNC_ADDR(memset),   "libc.so"},
    {"memcmp",   SSRNX_FUNC_ADDR(memcmp),   "libc.so"},
    {"strlen",   SSRNX_FUNC_ADDR(strlen),   "libc.so"},
    {"strndup", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strndup), "libc.so"},
    {"memmem", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_memmem), "libc.so"},
    {"asprintf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_asprintf), "libc.so"},
    {"isdigit", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isdigit), "libc.so"},
    {"__isfinite", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isfinite), "libc.so"},
    {"lrintf", SSRNX_FUNC_ADDR(lrintf), "libm.so"},
    {"execlp", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_execlp), "libc.so"},
    {"getrusage", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getrusage), "libc.so"},
    {"settimeofday", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_settimeofday), "libc.so"},
    {"timer_create", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_timer_create), "libc.so"},
    {"timer_delete", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_timer_delete), "libc.so"},
    {"timer_settime", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_timer_settime), "libc.so"},
    {"timer_gettime", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_timer_gettime), "libc.so"},
    {"timer_getoverrun", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_timer_getoverrun), "libc.so"},
    {"strcmp",   SSRNX_FUNC_ADDR(strcmp),   "libc.so"},
    {"strncmp",  SSRNX_FUNC_ADDR(strncmp),  "libc.so"},
    {"strchr",   SSRNX_FUNC_ADDR(strchr),   "libc.so"},
    {"strrchr",  SSRNX_FUNC_ADDR(strrchr),  "libc.so"},
    {"strstr",   SSRNX_FUNC_ADDR(strstr),   "libc.so"},

    /* Host heap. These pointers are process-global and directly usable by
       AArch64 guest code because guest and host share one address space. */
    {"malloc",   SSRNX_FUNC_ADDR(compat_ssrnx_bionic_malloc),   "libc.so"},
    {"calloc",   SSRNX_FUNC_ADDR(compat_ssrnx_bionic_calloc),   "libc.so"},
    {"realloc",  SSRNX_FUNC_ADDR(compat_ssrnx_bionic_realloc),  "libc.so"},
    {"free",     SSRNX_FUNC_ADDR(free),     "libc.so"},

    /* Bionic errno accessor. */
    {"__errno",  SSRNX_FUNC_ADDR(compat_ssrnx_bionic_errno), "libc.so"},

    /* Itanium C++ ABI process/destructor hooks provided by Bionic libc. */
    {"__cxa_atexit",   SSRNX_FUNC_ADDR(ssrnx_bionic_cxa_atexit),   "libc.so"},
    {"__cxa_thread_atexit_impl", SSRNX_FUNC_ADDR(ssrnx_bionic_cxa_thread_atexit_impl), "libc.so"},
    {"__cxa_finalize", SSRNX_FUNC_ADDR(ssrnx_bionic_cxa_finalize), "libc.so"},
    {"__tls_get_addr", SSRNX_FUNC_ADDR(ssrnx_bionic___tls_get_addr), "libc.so"},

    /* Android's legacy system libstdc++.so supplies the compiler ABI support
       used by old/current NDK DSOs that still DT_NEEDED that platform library.
       Keep these as virtual-system exports rather than allowing an unrelated
       RTLD_LOCAL application root to satisfy them accidentally. */
    {"__cxa_guard_acquire", SSRNX_FUNC_ADDR(ssrnx_bionic_cxa_guard_acquire), "libstdc++.so"},
    {"__cxa_guard_release", SSRNX_FUNC_ADDR(ssrnx_bionic_cxa_guard_release), "libstdc++.so"},
    {"__cxa_guard_abort",   SSRNX_FUNC_ADDR(ssrnx_bionic_cxa_guard_abort),   "libstdc++.so"},
    {"__cxa_pure_virtual",  SSRNX_FUNC_ADDR(ssrnx_bionic_cxa_pure_virtual),  "libstdc++.so"},
    {"_ZdlPv",              SSRNX_FUNC_ADDR(free),                           "libstdc++.so"},
    {"_ZdaPv",              SSRNX_FUNC_ADDR(free),                           "libstdc++.so"},

    /* Scalar POSIX file-descriptor API. open/lseek are wrappers because Linux
       flag constants and off_t semantics are ABI boundaries worth translating. */
    {"open",     SSRNX_FUNC_ADDR(compat_ssrnx_bionic_open),  "libc.so"},
    {"__open_2", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_open_2), "libc.so"},
    {"fcntl",    SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fcntl), "libc.so"},
    {"read",     SSRNX_FUNC_ADDR(compat_ssrnx_bionic_read),  "libc.so"},
    {"write",    SSRNX_FUNC_ADDR(compat_ssrnx_bionic_write), "libc.so"},
    {"lseek",    SSRNX_FUNC_ADDR(compat_ssrnx_bionic_lseek), "libc.so"},
    {"lseek64",  SSRNX_FUNC_ADDR(compat_ssrnx_bionic_lseek), "libc.so"},
    {"pread",    SSRNX_FUNC_ADDR(compat_ssrnx_bionic_pread), "libc.so"},
    {"pwrite",   SSRNX_FUNC_ADDR(compat_ssrnx_bionic_pwrite), "libc.so"},
    {"isatty",   SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isatty), "libc.so"},
    {"close",    SSRNX_FUNC_ADDR(compat_ssrnx_bionic_close), "libc.so"},
    {"fsync",    SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fsync), "libc.so"},
    {"fdatasync",SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fdatasync), "libc.so"},
    {"unlink",   SSRNX_FUNC_ADDR(compat_ssrnx_bionic_unlink), "libc.so"},

    /* Explicit structure translation rather than passing host timespec through. */
    {"clock_gettime", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_clock_gettime), "libc.so"},
    {"clock_getres", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_clock_getres), "libc.so"},

    /* Stage 5: translated Bionic pthread/TLS surface. */
    {"pthread_create", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_create), "libc.so"},
    {"pthread_join", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_join), "libc.so"},
    {"pthread_detach", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_detach), "libc.so"},
    {"pthread_self", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_self), "libc.so"},
    {"pthread_equal", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_equal), "libc.so"},
    {"pthread_setname_np", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_setname_np), "libc.so"},
    {"pthread_getname_np", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_getname_np), "libc.so"},
    {"pthread_gettid_np", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_gettid_np), "libc.so"},
    {"pthread_getschedparam", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_getschedparam), "libc.so"},
    {"pthread_setschedparam", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_setschedparam), "libc.so"},
    {"pthread_setschedprio", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_setschedprio), "libc.so"},
    {"pthread_getattr_np", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_getattr_np), "libc.so"},
    {"pthread_exit", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_exit), "libc.so"},
    {"pthread_mutex_init", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_mutex_init), "libc.so"},
    {"pthread_mutex_destroy", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_mutex_destroy), "libc.so"},
    {"pthread_mutex_lock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_mutex_lock), "libc.so"},
    {"pthread_mutex_trylock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_mutex_trylock), "libc.so"},
    {"pthread_mutex_timedlock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_mutex_timedlock), "libc.so"},
    {"pthread_mutex_timedlock_monotonic_np", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_mutex_timedlock_monotonic_np), "libc.so"},
    {"pthread_mutex_clocklock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_mutex_clocklock), "libc.so"},
    {"pthread_mutex_unlock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_mutex_unlock), "libc.so"},
    {"pthread_condattr_init", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_condattr_init), "libc.so"},
    {"pthread_condattr_destroy", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_condattr_destroy), "libc.so"},
    {"pthread_condattr_getclock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_condattr_getclock), "libc.so"},
    {"pthread_condattr_setclock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_condattr_setclock), "libc.so"},
    {"pthread_condattr_getpshared", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_condattr_getpshared), "libc.so"},
    {"pthread_condattr_setpshared", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_condattr_setpshared), "libc.so"},
    {"pthread_cond_init", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_cond_init), "libc.so"},
    {"pthread_cond_destroy", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_cond_destroy), "libc.so"},
    {"pthread_cond_wait", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_cond_wait), "libc.so"},
    {"pthread_cond_timedwait_monotonic_np", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_cond_timedwait_monotonic_np), "libc.so"},
    {"pthread_cond_timedwait_relative_np", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_cond_timedwait_relative_np), "libc.so"},
    {"pthread_cond_clockwait", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_cond_clockwait), "libc.so"},
    {"pthread_cond_signal", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_cond_signal), "libc.so"},
    {"pthread_cond_broadcast", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_cond_broadcast), "libc.so"},
    {"pthread_rwlockattr_init", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlockattr_init), "libc.so"},
    {"pthread_rwlockattr_destroy", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlockattr_destroy), "libc.so"},
    {"pthread_rwlockattr_getpshared", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlockattr_getpshared), "libc.so"},
    {"pthread_rwlockattr_setpshared", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlockattr_setpshared), "libc.so"},
    {"pthread_rwlockattr_getkind_np", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlockattr_getkind_np), "libc.so"},
    {"pthread_rwlockattr_setkind_np", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlockattr_setkind_np), "libc.so"},
    {"pthread_rwlock_init", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlock_init), "libc.so"},
    {"pthread_rwlock_destroy", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlock_destroy), "libc.so"},
    {"pthread_rwlock_rdlock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlock_rdlock), "libc.so"},
    {"pthread_rwlock_timedrdlock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlock_timedrdlock), "libc.so"},
    {"pthread_rwlock_timedrdlock_monotonic_np", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlock_timedrdlock_monotonic_np), "libc.so"},
    {"pthread_rwlock_clockrdlock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlock_clockrdlock), "libc.so"},
    {"pthread_rwlock_tryrdlock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlock_tryrdlock), "libc.so"},
    {"pthread_rwlock_wrlock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlock_wrlock), "libc.so"},
    {"pthread_rwlock_timedwrlock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlock_timedwrlock), "libc.so"},
    {"pthread_rwlock_timedwrlock_monotonic_np", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlock_timedwrlock_monotonic_np), "libc.so"},
    {"pthread_rwlock_clockwrlock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlock_clockwrlock), "libc.so"},
    {"pthread_rwlock_trywrlock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlock_trywrlock), "libc.so"},
    {"pthread_rwlock_unlock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_rwlock_unlock), "libc.so"},
    {"pthread_spin_init", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_spin_init), "libc.so"},
    {"pthread_spin_destroy", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_spin_destroy), "libc.so"},
    {"pthread_spin_lock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_spin_lock), "libc.so"},
    {"pthread_spin_trylock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_spin_trylock), "libc.so"},
    {"pthread_spin_unlock", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_spin_unlock), "libc.so"},
    {"pthread_barrierattr_init", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_barrierattr_init), "libc.so"},
    {"pthread_barrierattr_destroy", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_barrierattr_destroy), "libc.so"},
    {"pthread_barrierattr_getpshared", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_barrierattr_getpshared), "libc.so"},
    {"pthread_barrierattr_setpshared", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_barrierattr_setpshared), "libc.so"},
    {"pthread_barrier_init", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_barrier_init), "libc.so"},
    {"pthread_barrier_destroy", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_barrier_destroy), "libc.so"},
    {"pthread_barrier_wait", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_barrier_wait), "libc.so"},
    {"pthread_once", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_once), "libc.so"},
    {"pthread_key_create", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_key_create), "libc.so"},
    {"pthread_key_delete", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_key_delete), "libc.so"},
    {"pthread_setspecific", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_setspecific), "libc.so"},
    {"pthread_getspecific", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_getspecific), "libc.so"},
    {"__emutls_get_address", SSRNX_FUNC_ADDR(ssrnx_bionic_emutls_get_address), "libc.so"},
    {"sem_init", SSRNX_FUNC_ADDR(ssrnx_bionic_sem_init), "libc.so"},
    {"sem_destroy", SSRNX_FUNC_ADDR(ssrnx_bionic_sem_destroy), "libc.so"},
    {"sem_wait", SSRNX_FUNC_ADDR(ssrnx_bionic_sem_wait), "libc.so"},
    {"sem_trywait", SSRNX_FUNC_ADDR(ssrnx_bionic_sem_trywait), "libc.so"},
    {"sem_timedwait", SSRNX_FUNC_ADDR(ssrnx_bionic_sem_timedwait), "libc.so"},
    {"sem_post", SSRNX_FUNC_ADDR(ssrnx_bionic_sem_post), "libc.so"},
    {"sem_getvalue", SSRNX_FUNC_ADDR(ssrnx_bionic_sem_getvalue), "libc.so"},

    /* Stage 12.2: C/POSIX surface used by Android NativeActivity glue and
       small native games. Guest FILE* values are opaque handles to newlib FILEs. */
    {"pipe", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_pipe), "libc.so"},
    {"pipe2", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_pipe2), "libc.so"},
    {"getrlimit", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getrlimit), "libc.so"},
    {"setitimer", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_setitimer), "libc.so"},
    {"popen", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_popen), "libc.so"},
    {"pclose", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_pclose), "libc.so"},
    {"mkstemp", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mkstemp), "libc.so"},
    {"strerror", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strerror), "libc.so"},
    {"usleep", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_usleep), "libc.so"},
    {"sleep", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sleep), "libc.so"},
    {"rand", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_rand), "libc.so"},
    {"srand", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_srand), "libc.so"},
    {"random", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_random), "libc.so"},
    {"srandom", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_srandom), "libc.so"},
    {"abort", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_abort), "libc.so"},
    {"exit", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_exit), "libc.so"},
    {"_exit", SSRNX_FUNC_ADDR(compat_ssrnx_bionic__exit), "libc.so"},
    {"__stack_chk_fail", SSRNX_FUNC_ADDR(compat_ssrnx_stack_chk_fail), "libc.so"},
    {"__sF", SSRNX_DATA_ADDR(compat_g_bionic_sF), "libc.so"},
    {"stdin", SSRNX_DATA_ADDR(compat_g_bionic_stdin_ptr), "libc.so"},
    {"stdout", SSRNX_DATA_ADDR(compat_g_bionic_stdout_ptr), "libc.so"},
    {"stderr", SSRNX_DATA_ADDR(compat_g_bionic_stderr_ptr), "libc.so"},
    {"sys_signame", SSRNX_DATA_ADDR(compat_g_bionic_sys_signame), "libc.so"},
    {"sys_siglist", SSRNX_DATA_ADDR(compat_g_bionic_sys_siglist), "libc.so"},
    {"sigaltstack", SSRNX_FUNC_ADDR(ssrnx_bionic_sigaltstack), "libc.so"},
    {"sigemptyset", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sigemptyset), "libc.so"},
    {"sigaction", SSRNX_FUNC_ADDR(ssrnx_bionic_sigaction), "libc.so"},
    {"raise", SSRNX_FUNC_ADDR(ssrnx_bionic_raise), "libc.so"},
    {"strsignal", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strsignal), "libc.so"},
    {"wmemcpy", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_wmemcpy), "libc.so"},
    {"wmemset", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_wmemset), "libc.so"},
    {"sched_yield", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sched_yield), "libc.so"},
    {"sched_getaffinity", SSRNX_FUNC_ADDR(ssrnx_bionic_sched_getaffinity), "libc.so"},
    {"sched_setaffinity", SSRNX_FUNC_ADDR(ssrnx_bionic_sched_setaffinity), "libc.so"},
    {"wmemmove", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_wmemmove), "libc.so"},
    {"mkdir", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mkdir), "libc.so"},
    {"memchr", SSRNX_FUNC_ADDR(memchr), "libc.so"},
    {"strncpy", SSRNX_FUNC_ADDR(strncpy), "libc.so"},
    {"qsort", SSRNX_FUNC_ADDR(qsort), "libc.so"},
    {"atoi", SSRNX_FUNC_ADDR(atoi), "libc.so"},
    {"strtol", SSRNX_FUNC_ADDR(strtol), "libc.so"},
    {"time", SSRNX_FUNC_ADDR(time), "libc.so"},
    {"stat", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_stat), "libc.so"},
    {"stat64", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_stat), "libc.so"},
    {"fstat64", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fstat), "libc.so"},
    {"lstat64", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_lstat), "libc.so"},
    {"opendir", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_opendir), "libc.so"},
    {"closedir", SSRNX_FUNC_ADDR(closedir), "libc.so"},
    {"localtime", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_localtime), "libc.so"},
    {"localtime_r", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_localtime_r), "libc.so"},
    {"mktime", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mktime), "libc.so"},
    {"getpid", SSRNX_FUNC_ADDR(getpid), "libc.so"},
    {"syscall", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_syscall), "libc.so"},
    {"fopen", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fopen), "libc.so"},
    {"freopen", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_freopen), "libc.so"},
    {"fdopen", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fdopen), "libc.so"},
    {"tmpfile", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_tmpfile), "libc.so"},
    {"fclose", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fclose), "libc.so"},
    {"fread", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fread), "libc.so"},
    {"fwrite", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fwrite), "libc.so"},
    {"fseek", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fseek), "libc.so"},
    {"ftell", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_ftell), "libc.so"},
    {"rewind", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_rewind), "libc.so"},
    {"fflush", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fflush), "libc.so"},
    {"fscanf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fscanf), "libc.so"},
    {"fprintf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fprintf), "libc.so"},
    {"fgetc", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fgetc), "libc.so"},
    {"ungetc", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_ungetc), "libc.so"},
    {"feof", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_feof), "libc.so"},
    {"ferror", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_ferror), "libc.so"},
    {"fseeko", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fseeko), "libc.so"},
    {"ftello", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_ftello), "libc.so"},
    {"fseeko64", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fseeko64), "libc.so"},
    {"ftello64", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_ftello64), "libc.so"},
    {"sprintf", SSRNX_FUNC_ADDR(sprintf), "libc.so"},
    {"snprintf", SSRNX_FUNC_ADDR(snprintf), "libc.so"},
    {"vsnprintf", SSRNX_FUNC_ADDR(vsnprintf), "libc.so"},
    {"__memcpy_chk", SSRNX_FUNC_ADDR(compat_ssrnx_memcpy_chk), "libc.so"},
    {"__memmove_chk", SSRNX_FUNC_ADDR(compat_ssrnx_memmove_chk), "libc.so"},
    {"__memset_chk", SSRNX_FUNC_ADDR(compat_ssrnx_memset_chk), "libc.so"},
    {"__strlen_chk", SSRNX_FUNC_ADDR(compat_ssrnx_strlen_chk), "libc.so"},
    {"__read_chk", SSRNX_FUNC_ADDR(compat_ssrnx_read_chk), "libc.so"},
    {"__write_chk", SSRNX_FUNC_ADDR(compat_ssrnx_write_chk), "libc.so"},
    {"__snprintf_chk", SSRNX_FUNC_ADDR(compat_ssrnx_snprintf_chk), "libc.so"},
    {"__vsnprintf_chk", SSRNX_FUNC_ADDR(compat_ssrnx_vsnprintf_chk), "libc.so"},
    {"__sprintf_chk", SSRNX_FUNC_ADDR(compat_ssrnx_sprintf_chk), "libc.so"},
    {"__vsprintf_chk", SSRNX_FUNC_ADDR(compat_ssrnx_vsprintf_chk), "libc.so"},
    {"pthread_attr_init", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_init), "libc.so"},
    {"pthread_attr_destroy", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_destroy), "libc.so"},
    {"pthread_attr_setdetachstate", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_setdetachstate), "libc.so"},
    {"pthread_attr_getdetachstate", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_getdetachstate), "libc.so"},
    {"pthread_attr_setstacksize", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_setstacksize), "libc.so"},
    {"pthread_attr_getstacksize", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_getstacksize), "libc.so"},
    {"pthread_attr_setguardsize", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_setguardsize), "libc.so"},
    {"pthread_attr_getguardsize", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_getguardsize), "libc.so"},
    {"pthread_attr_getstack", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_getstack), "libc.so"},
    {"pthread_attr_setstack", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_setstack), "libc.so"},
    {"pthread_attr_setinheritsched", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_setinheritsched), "libc.so"},
    {"pthread_attr_getinheritsched", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_getinheritsched), "libc.so"},
    {"pthread_attr_setschedpolicy", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_setschedpolicy), "libc.so"},
    {"pthread_attr_getschedpolicy", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_getschedpolicy), "libc.so"},
    {"pthread_attr_setschedparam", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_setschedparam), "libc.so"},
    {"pthread_attr_getschedparam", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_getschedparam), "libc.so"},
    {"pthread_attr_setscope", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_setscope), "libc.so"},
    {"pthread_attr_getscope", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_attr_getscope), "libc.so"},
    {"pthread_atfork", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_atfork), "libc.so"},
    {"__pthread_cleanup_push", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_cleanup_push), "libc.so"},
    {"__pthread_cleanup_pop", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_cleanup_pop), "libc.so"},
    /* Android libc++_shared/Bionic compatibility services discovered through
       generic import preflight. These preserve published libc/POSIX contracts
       and contain no application or package-specific behavior. */
    {"vfprintf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_vfprintf), "libc.so"},
    {"fputc", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fputc), "libc.so"},
    {"putc", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_putc), "libc.so"},
    {"getwc", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getwc), "libc.so"},
    {"putwc", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_putwc), "libc.so"},
    {"ungetwc", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_ungetwc), "libc.so"},
    {"setvbuf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_setvbuf), "libc.so"},
    {"vsprintf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_vsprintf), "libc.so"},
    {"vasprintf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_vasprintf), "libc.so"},
    {"android_set_abort_message", SSRNX_FUNC_ADDR(compat_ssrnx_android_set_abort_message), "libc.so"},
    {"openlog", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_openlog), "libc.so"},
    {"syslog", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_syslog), "libc.so"},
    {"closelog", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_closelog), "libc.so"},
    {"posix_memalign", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_posix_memalign), "libc.so"},
    {"memalign", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_memalign), "libc.so"},
    {"malloc_usable_size", SSRNX_FUNC_ADDR(malloc_usable_size), "libc.so"},
    {"__assert2", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_assert2), "libc.so"},
    {"pthread_cond_timedwait", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_cond_timedwait), "libc.so"},
    {"pthread_mutexattr_init", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_pthread_mutexattr_init), "libc.so"},
    {"pthread_mutexattr_gettype", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_pthread_mutexattr_gettype), "libc.so"},
    {"pthread_mutexattr_settype", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_pthread_mutexattr_settype), "libc.so"},
    {"pthread_mutexattr_getpshared", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_pthread_mutexattr_getpshared), "libc.so"},
    {"pthread_mutexattr_setpshared", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_pthread_mutexattr_setpshared), "libc.so"},
    {"pthread_mutexattr_getprotocol", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_pthread_mutexattr_getprotocol), "libc.so"},
    {"pthread_mutexattr_setprotocol", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_pthread_mutexattr_setprotocol), "libc.so"},
    {"pthread_mutexattr_destroy", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_pthread_mutexattr_destroy), "libc.so"},
    {"wcslen", SSRNX_FUNC_ADDR(wcslen), "libc.so"},
    {"wmemchr", SSRNX_FUNC_ADDR(wmemchr), "libc.so"},
    {"wmemcmp", SSRNX_FUNC_ADDR(wmemcmp), "libc.so"},
    {"strtoul", SSRNX_FUNC_ADDR(strtoul), "libc.so"},
    {"strtoll", SSRNX_FUNC_ADDR(strtoll), "libc.so"},
    {"strtoull", SSRNX_FUNC_ADDR(strtoull), "libc.so"},
    {"strtof", SSRNX_FUNC_ADDR(strtof), "libc.so"},
    {"strtod", SSRNX_FUNC_ADDR(strtod), "libc.so"},
    {"strtold", SSRNX_FUNC_ADDR(strtold), "libc.so"},
    {"wcstol", SSRNX_FUNC_ADDR(wcstol), "libc.so"},
    {"wcstoul", SSRNX_FUNC_ADDR(wcstoul), "libc.so"},
    {"wcstoll", SSRNX_FUNC_ADDR(wcstoll), "libc.so"},
    {"wcstoull", SSRNX_FUNC_ADDR(wcstoull), "libc.so"},
    {"wcstof", SSRNX_FUNC_ADDR(wcstof), "libc.so"},
    {"wcstod", SSRNX_FUNC_ADDR(wcstod), "libc.so"},
    {"wcstold", SSRNX_FUNC_ADDR(wcstold), "libc.so"},
    {"swprintf", SSRNX_FUNC_ADDR(swprintf), "libc.so"},
    {"strerror_r", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strerror_r), "libc.so"},
    {"__gnu_strerror_r", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_gnu_strerror_r), "libc.so"},
    {"sysconf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sysconf), "libc.so"},
    {"nanosleep", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_nanosleep), "libc.so"},
    {"ioctl", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_ioctl), "libc.so"},
    {"getc", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getc), "libc.so"},
    {"newlocale", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_newlocale), "libc.so"},
    {"uselocale", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_uselocale), "libc.so"},
    {"vsscanf", SSRNX_FUNC_ADDR(vsscanf), "libc.so"},
    {"isxdigit_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isxdigit_l), "libc.so"},
    {"isdigit_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isdigit_l), "libc.so"},
    {"strftime_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strftime_l), "libc.so"},
    {"strftime", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strftime), "libc.so"},
    {"wcsftime", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_wcsftime), "libc.so"},
    {"mbsrtowcs", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mbsrtowcs), "libc.so"},
    {"sscanf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sscanf), "libc.so"},
    {"setlocale", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_setlocale), "libc.so"},
    {"freelocale", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_freelocale), "libc.so"},
    {"strcoll_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strcoll_l), "libc.so"},
    {"strcoll", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strcoll), "libc.so"},
    {"strxfrm", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strxfrm), "libc.so"},
    {"wcscoll", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_wcscoll), "libc.so"},
    {"wcsxfrm", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_wcsxfrm), "libc.so"},
    {"wctype", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_wctype), "libc.so"},
    {"iswctype", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_iswctype), "libc.so"},
    {"strxfrm_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strxfrm_l), "libc.so"},
    {"wcscoll_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_wcscoll_l), "libc.so"},
    {"wcsxfrm_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_wcsxfrm_l), "libc.so"},
    {"iswlower_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_iswlower_l), "libc.so"},
    {"islower_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_islower_l), "libc.so"},
    {"isupper_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isupper_l), "libc.so"},
    {"toupper_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_toupper_l), "libc.so"},
    {"tolower_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_tolower_l), "libc.so"},
    {"iswspace_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_iswspace_l), "libc.so"},
    {"iswprint_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_iswprint_l), "libc.so"},
    {"iswcntrl_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_iswcntrl_l), "libc.so"},
    {"iswupper_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_iswupper_l), "libc.so"},
    {"iswalpha_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_iswalpha_l), "libc.so"},
    {"iswdigit_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_iswdigit_l), "libc.so"},
    {"iswpunct_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_iswpunct_l), "libc.so"},
    {"iswxdigit_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_iswxdigit_l), "libc.so"},
    {"iswblank_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_iswblank_l), "libc.so"},
    {"towupper_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_towupper_l), "libc.so"},
    {"towlower_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_towlower_l), "libc.so"},
    {"towupper", SSRNX_FUNC_ADDR(towupper), "libc.so"},
    {"towlower", SSRNX_FUNC_ADDR(towlower), "libc.so"},
    {"btowc", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_btowc), "libc.so"},
    {"wctob", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_wctob), "libc.so"},
    {"wcsnrtombs", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_wcsnrtombs), "libc.so"},
    {"wcrtomb", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_wcrtomb), "libc.so"},
    {"mbsnrtowcs", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mbsnrtowcs), "libc.so"},
    {"mbrtowc", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mbrtowc), "libc.so"},
    {"mbtowc", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mbtowc), "libc.so"},
    {"__ctype_get_mb_cur_max", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_ctype_mb_cur_max), "libc.so"},
    {"mbrlen", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mbrlen), "libc.so"},
    {"localeconv", SSRNX_FUNC_ADDR(localeconv), "libc.so"},
    {"strtoll_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strtoll_l), "libc.so"},
    {"strtoull_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strtoull_l), "libc.so"},
    {"strtold_l", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strtold_l), "libc.so"},
    {"realpath", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_realpath), "libc.so"},
    {"lstat", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_lstat), "libc.so"},
    {"symlink", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_symlink), "libc.so"},
    {"link", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_link), "libc.so"},
    {"fchmod", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fchmod), "libc.so"},
    {"ftruncate", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_ftruncate), "libc.so"},
    {"sendfile", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sendfile), "libc.so"},
    {"readlink", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_readlink), "libc.so"},
    {"pathconf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_pathconf), "libc.so"},
    {"getcwd", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getcwd), "libc.so"},
    {"chdir", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_chdir), "libc.so"},
    {"utimensat", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_utimensat), "libc.so"},
    {"utime", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_utime), "libc.so"},
    {"utimes", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_utimes), "libc.so"},
    {"futimens", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_futimens), "libc.so"},
    {"fchmodat", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fchmodat), "libc.so"},
    {"remove", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_remove), "libc.so"},
    {"openat", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_openat), "libc.so"},
    {"fdopendir", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fdopendir), "libc.so"},
    {"readdir", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_readdir), "libc.so"},
    {"readdir_r", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_readdir_r), "libc.so"},
    {"unlinkat", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_unlinkat), "libc.so"},
    {"rename", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_rename), "libc.so"},
    {"renameat", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_renameat), "libc.so"},
    {"truncate", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_truncate), "libc.so"},
    {"statvfs", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_statvfs), "libc.so"},
    {"statfs", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_statfs), "libc.so"},
    {"fstatfs", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fstatfs), "libc.so"},
    {"dup", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_dup), "libc.so"},
    {"flock", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_flock), "libc.so"},
    {"prctl", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_prctl), "libc.so"},
    {"process_vm_readv", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_process_vm_readv), "libc.so"},
    {"ptrace", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_ptrace), "libc.so"},
    {"getenv", SSRNX_FUNC_ADDR(getenv), "libc.so"},
    {"writev", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_writev), "libc.so"},
    {"regcomp", SSRNX_FUNC_ADDR(ssrnx_bionic_regcomp), "libc.so"},
    {"regexec", SSRNX_FUNC_ADDR(ssrnx_bionic_regexec), "libc.so"},
    {"regfree", SSRNX_FUNC_ADDR(ssrnx_bionic_regfree), "libc.so"},
    {"_ctype_", SSRNX_DATA_ADDR(compat_g_bionic_ctype_export), "libc.so"},
    {"fstat", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fstat), "libc.so"},
    {"__system_property_get", SSRNX_FUNC_ADDR(compat_ssrnx_system_property_get), "libc.so"},
    {"__system_property_find", SSRNX_FUNC_ADDR(compat_ssrnx_system_property_find), "libc.so"},
    {"__system_property_find_nth", SSRNX_FUNC_ADDR(compat_ssrnx_system_property_find_nth), "libc.so"},
    {"__system_property_read", SSRNX_FUNC_ADDR(compat_ssrnx_system_property_read), "libc.so"},
    {"__system_property_read_callback", SSRNX_FUNC_ADDR(compat_ssrnx_system_property_read_callback), "libc.so"},
    {"__system_property_foreach", SSRNX_FUNC_ADDR(compat_ssrnx_system_property_foreach), "libc.so"},
    {"__system_property_serial", SSRNX_FUNC_ADDR(compat_ssrnx_system_property_serial), "libc.so"},
    {"__system_property_area_serial", SSRNX_FUNC_ADDR(compat_ssrnx_system_property_area_serial), "libc.so"},
    {"android_get_device_api_level", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_android_get_device_api_level), "libc.so"},
    {"android_get_application_target_sdk_version", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_android_get_application_target_sdk_version), "libc.so"},
    {"__register_atfork", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_register_atfork), "libc.so"},
    {"__unregister_atfork", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_unregister_atfork), "libc.so"},
    {"getpagesize", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getpagesize), "libc.so"},
    {"strlcpy", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strlcpy), "libc.so"},
    {"setenv", SSRNX_FUNC_ADDR(setenv), "libc.so"},
    /* Bionic exports the process environment vector itself; getenv/setenv above
       operate on this same newlib environ, so guest reads stay coherent. */
    {"environ", SSRNX_DATA_ADDR(environ), "libc.so"},
    {"unsetenv", SSRNX_FUNC_ADDR(unsetenv), "libc.so"},
    {"rmdir", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_rmdir), "libc.so"},
    {"getauxval", SSRNX_FUNC_ADDR(ssrnx_bionic_getauxval), "libc.so"},
    {"gettid", SSRNX_FUNC_ADDR(ssrnx_bionic_gettid), "libc.so"},
    {"getpriority", SSRNX_FUNC_ADDR(ssrnx_bionic_getpriority), "libc.so"},
    {"setpriority", SSRNX_FUNC_ADDR(ssrnx_bionic_setpriority), "libc.so"},

    /* Stage 14.7: Linux/Bionic socket API translated onto the host network
       stack. Constants and public structures are converted explicitly. */
    {"socket", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_socket), "libc.so"},
    {"connect", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_connect), "libc.so"},
    {"bind", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_bind), "libc.so"},
    {"listen", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_listen), "libc.so"},
    {"accept", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_accept), "libc.so"},
    {"accept4", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_accept4), "libc.so"},
    {"shutdown", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_shutdown), "libc.so"},
    {"send", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_send), "libc.so"},
    {"recv", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_recv), "libc.so"},
    {"setsockopt", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_setsockopt), "libc.so"},
    {"getsockopt", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getsockopt), "libc.so"},
    {"getaddrinfo", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getaddrinfo), "libc.so"},
    {"freeaddrinfo", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_freeaddrinfo), "libc.so"},
    {"__FD_SET_chk", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_FD_SET_chk), "libc.so"},
    {"__FD_ISSET_chk", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_FD_ISSET_chk), "libc.so"},
    {"select", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_select), "libc.so"},

    /* Stage 14.8: Cocos/native-runtime batch observed on hardware. */
    {"__stack_chk_guard", SSRNX_DATA_ADDR(compat_g_bionic_stack_chk_guard_export), "libc.so"},
    {"setjmp", SSRNX_FUNC_ADDR(ssrnx_guest_setjmp), "libc.so"},
    {"_setjmp", SSRNX_FUNC_ADDR(ssrnx_guest__setjmp), "libc.so"},
    {"_longjmp", SSRNX_FUNC_ADDR(ssrnx_guest_longjmp), "libc.so"},
    {"longjmp", SSRNX_FUNC_ADDR(ssrnx_guest_longjmp), "libc.so"},
    {"sigsetjmp", SSRNX_FUNC_ADDR(ssrnx_guest_sigsetjmp), "libc.so"},
    {"siglongjmp", SSRNX_FUNC_ADDR(ssrnx_guest_siglongjmp), "libc.so"},
    {"strdup", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strdup), "libc.so"},
    {"atol", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_atol), "libc.so"},
    {"div", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_div), "libc.so"},
    {"lldiv", SSRNX_FUNC_ADDR(lldiv), "libc.so"},
    {"strnlen", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strnlen), "libc.so"},
    {"strncat", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strncat), "libc.so"},
    {"puts", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_puts), "libc.so"},
    {"vprintf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_vprintf), "libc.so"},
    {"strcat", SSRNX_FUNC_ADDR(strcat), "libc.so"},
    {"strcpy", SSRNX_FUNC_ADDR(strcpy), "libc.so"},
    {"strcspn", SSRNX_FUNC_ADDR(strcspn), "libc.so"},
    {"strspn", SSRNX_FUNC_ADDR(strspn), "libc.so"},
    {"strpbrk", SSRNX_FUNC_ADDR(strpbrk), "libc.so"},
    {"strcasecmp", SSRNX_FUNC_ADDR(strcasecmp), "libc.so"},
    {"strncasecmp", SSRNX_FUNC_ADDR(strncasecmp), "libc.so"},
    {"strtok", SSRNX_FUNC_ADDR(strtok), "libc.so"},
    {"strtok_r", SSRNX_FUNC_ADDR(strtok_r), "libc.so"},
    {"memrchr", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_memrchr), "libc.so"},
    {"bsearch", SSRNX_FUNC_ADDR(bsearch), "libc.so"},
    {"abs", SSRNX_FUNC_ADDR(abs), "libc.so"},
    {"atof", SSRNX_FUNC_ADDR(atof), "libc.so"},
    {"isprint", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isprint), "libc.so"},
    {"isascii", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isascii), "libc.so"},
    {"isalnum", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isalnum), "libc.so"},
    {"islower", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_islower), "libc.so"},
    {"isalpha", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isalpha), "libc.so"},
    {"isxdigit", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isxdigit), "libc.so"},
    {"isupper", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isupper), "libc.so"},
    {"isspace", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isspace), "libc.so"},
    {"isgraph", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isgraph), "libc.so"},
    {"tolower", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_tolower), "libc.so"},
    {"toupper", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_toupper), "libc.so"},
    {"getuid", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getuid), "libc.so"},
    {"geteuid", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_geteuid), "libc.so"},
    {"getgid", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getgid), "libc.so"},
    {"getegid", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getegid), "libc.so"},
    {"setuid", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_setuid), "libc.so"},
    {"setgid", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_setgid), "libc.so"},
    {"initgroups", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_initgroups), "libc.so"},
    {"getpwuid", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getpwuid), "libc.so"},
    {"getpwuid_r", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getpwuid_r), "libc.so"},
    {"umask", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_umask), "libc.so"},
    {"chmod", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_chmod), "libc.so"},
    {"access", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_access), "libc.so"},
    {"faccessat", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_faccessat), "libc.so"},
    {"dup2", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_dup2), "libc.so"},
    {"tcgetattr", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_tcgetattr), "libc.so"},
    {"tcsetattr", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_tcsetattr), "libc.so"},
    {"mlock", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mlock), "libc.so"},
    {"mmap", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mmap), "libc.so"},
    {"munmap", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_munmap), "libc.so"},
    {"mprotect", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mprotect), "libc.so"},
    {"mremap", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mremap), "libc.so"},
    {"madvise", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_madvise), "libc.so"},
    {"fork", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fork), "libc.so"},
    {"waitpid", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_waitpid), "libc.so"},
    {"wait4", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_wait4), "libc.so"},
    {"execl", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_execl), "libc.so"},
    {"basename", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_basename), "libc.so"},
    {"fnmatch", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fnmatch), "libc.so"},
    {"strcasestr", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strcasestr), "libc.so"},
    {"fputs", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fputs), "libc.so"},
    {"clearerr", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_clearerr), "libc.so"},
    {"fgets", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fgets), "libc.so"},
    {"printf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_printf), "libc.so"},
    {"fileno", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fileno), "libc.so"},
    {"setbuf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_setbuf), "libc.so"},
    {"clock", SSRNX_FUNC_ADDR(clock), "libc.so"},
    {"gmtime", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_gmtime), "libc.so"},
    {"gmtime_r", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_gmtime_r), "libc.so"},
    {"difftime", SSRNX_FUNC_ADDR(difftime), "libc.so"},
    {"gettimeofday", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_gettimeofday), "libc.so"},
    {"sigaddset", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sigaddset), "libc.so"},
    {"sigdelset", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sigdelset), "libc.so"},
    {"sigfillset", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sigfillset), "libc.so"},
    {"sigprocmask", SSRNX_FUNC_ADDR(ssrnx_bionic_sigprocmask), "libc.so"},
    {"pthread_sigmask", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_sigmask), "libc.so"},
    {"sigsuspend", SSRNX_FUNC_ADDR(ssrnx_bionic_sigsuspend), "libc.so"},
    {"sigpending", SSRNX_FUNC_ADDR(ssrnx_bionic_sigpending), "libc.so"},
    {"pthread_kill", SSRNX_FUNC_ADDR(ssrnx_bionic_pthread_kill), "libc.so"},
    {"signal", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_signal), "libc.so"},
    {"kill", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_kill), "libc.so"},
    {"alarm", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_alarm), "libc.so"},
    {"socketpair", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_socketpair), "libc.so"},
    {"sendto", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sendto), "libc.so"},
    {"recvfrom", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_recvfrom), "libc.so"},
    {"sendmsg", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sendmsg), "libc.so"},
    {"recvmsg", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_recvmsg), "libc.so"},
    {"getsockname", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getsockname), "libc.so"},
    {"getpeername", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getpeername), "libc.so"},
    {"getnameinfo", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getnameinfo), "libc.so"},
    {"inet_ntop", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_inet_ntop), "libc.so"},
    {"inet_pton", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_inet_pton), "libc.so"},
    {"gai_strerror", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_gai_strerror), "libc.so"},
    {"gethostbyname", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_gethostbyname), "libc.so"},
    {"__get_h_errno", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_get_h_errno), "libc.so"},
    {"stpcpy", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_stpcpy), "libc.so"},
    {"gethostbyaddr", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_gethostbyaddr), "libc.so"},
    {"inet_addr", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_inet_addr), "libc.so"},
    {"inet_ntoa", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_inet_ntoa), "libc.so"},
    {"srand48", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_srand48), "libc.so"},
    {"lrand48", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_lrand48), "libc.so"},
    {"if_nametoindex", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_if_nametoindex), "libc.so"},
    {"if_indextoname", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_if_indextoname), "libc.so"},
    {"getservbyport", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getservbyport), "libc.so"},
    {"gethostname", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_gethostname), "libc.so"},
    {"uname", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_uname), "libc.so"},
    {"poll", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_poll), "libc.so"},

    /* Android NDK software bitmap access. */
    {"AndroidBitmap_getInfo", SSRNX_FUNC_ADDR(AndroidBitmap_getInfo), "libjnigraphics.so"},
    {"AndroidBitmap_lockPixels", SSRNX_FUNC_ADDR(AndroidBitmap_lockPixels), "libjnigraphics.so"},
    {"AndroidBitmap_unlockPixels", SSRNX_FUNC_ADDR(AndroidBitmap_unlockPixels), "libjnigraphics.so"},
    /* libz.so: Android platform zlib uses the stable public z_stream ABI. */
    {"zlibVersion", SSRNX_FUNC_ADDR(zlibVersion), "libz.so"},
    {"inflateInit_", SSRNX_FUNC_ADDR(inflateInit_), "libz.so"},
    {"inflateInit2_", SSRNX_FUNC_ADDR(inflateInit2_), "libz.so"},
    {"inflate", SSRNX_FUNC_ADDR(inflate), "libz.so"},
    {"inflateEnd", SSRNX_FUNC_ADDR(inflateEnd), "libz.so"},
    {"inflateReset", SSRNX_FUNC_ADDR(inflateReset), "libz.so"},
    {"deflateInit_", SSRNX_FUNC_ADDR(deflateInit_), "libz.so"},
    {"deflateInit2_", SSRNX_FUNC_ADDR(deflateInit2_), "libz.so"},
    {"deflate", SSRNX_FUNC_ADDR(deflate), "libz.so"},
    {"deflateEnd", SSRNX_FUNC_ADDR(deflateEnd), "libz.so"},
    {"deflateReset", SSRNX_FUNC_ADDR(deflateReset), "libz.so"},
    {"deflateBound", SSRNX_FUNC_ADDR(deflateBound), "libz.so"},
    {"compress", SSRNX_FUNC_ADDR(compress), "libz.so"},
    {"compress2", SSRNX_FUNC_ADDR(compress2), "libz.so"},
    {"compressBound", SSRNX_FUNC_ADDR(compressBound), "libz.so"},
    {"uncompress", SSRNX_FUNC_ADDR(uncompress), "libz.so"},
    {"adler32", SSRNX_FUNC_ADDR(adler32), "libz.so"},
    {"crc32", SSRNX_FUNC_ADDR(crc32), "libz.so"},
    {"zError", SSRNX_FUNC_ADDR(zError), "libz.so"},
    {"gzopen", SSRNX_FUNC_ADDR(compat_ssrnx_gzopen), "libz.so"},
    {"gzdopen", SSRNX_FUNC_ADDR(gzdopen), "libz.so"},
    {"gzread", SSRNX_FUNC_ADDR(gzread), "libz.so"},
    {"gzwrite", SSRNX_FUNC_ADDR(gzwrite), "libz.so"},
    {"gzclose", SSRNX_FUNC_ADDR(gzclose), "libz.so"},

    /* Bionic FORTIFY, stdio, wide-char, time, process and socket extras
     * (compat/libc_extras.inc). */
    {"__strchr_chk", SSRNX_FUNC_ADDR(compat_ssrnx_strchr_chk), "libc.so"},
    {"__strrchr_chk", SSRNX_FUNC_ADDR(compat_ssrnx_strrchr_chk), "libc.so"},
    {"__memchr_chk", SSRNX_FUNC_ADDR(compat_ssrnx_memchr_chk), "libc.so"},
    {"__strcpy_chk", SSRNX_FUNC_ADDR(compat_ssrnx_strcpy_chk), "libc.so"},
    {"__strcat_chk", SSRNX_FUNC_ADDR(compat_ssrnx_strcat_chk), "libc.so"},
    {"__strncat_chk", SSRNX_FUNC_ADDR(compat_ssrnx_strncat_chk), "libc.so"},
    {"__strncpy_chk", SSRNX_FUNC_ADDR(compat_ssrnx_strncpy_chk), "libc.so"},
    {"__strncpy_chk2", SSRNX_FUNC_ADDR(compat_ssrnx_strncpy_chk2), "libc.so"},
    {"__strlcpy_chk", SSRNX_FUNC_ADDR(compat_ssrnx_strlcpy_chk), "libc.so"},
    {"__fwrite_chk", SSRNX_FUNC_ADDR(compat_ssrnx_fwrite_chk), "libc.so"},
    {"__pread64_chk", SSRNX_FUNC_ADDR(compat_ssrnx_pread64_chk), "libc.so"},
    {"__readlink_chk", SSRNX_FUNC_ADDR(compat_ssrnx_readlink_chk), "libc.so"},
    {"__umask_chk", SSRNX_FUNC_ADDR(compat_ssrnx_umask_chk), "libc.so"},
    {"__FD_CLR_chk", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_FD_CLR_chk), "libc.so"},
    {"pread64", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_pread), "libc.so"},
    {"putchar", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_putchar), "libc.so"},
    {"perror", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_perror), "libc.so"},
    {"fputwc", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fputwc), "libc.so"},
    {"fwide", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fwide), "libc.so"},
    {"fsetpos", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fsetpos), "libc.so"},
    {"swscanf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_swscanf), "libc.so"},
    {"vswprintf", SSRNX_FUNC_ADDR(vswprintf), "libc.so"},
    {"wcscmp", SSRNX_FUNC_ADDR(wcscmp), "libc.so"},
    {"wcsncpy", SSRNX_FUNC_ADDR(wcsncpy), "libc.so"},
    {"iswalnum", SSRNX_FUNC_ADDR(iswalnum), "libc.so"},
    {"iswalpha", SSRNX_FUNC_ADDR(iswalpha), "libc.so"},
    {"iswblank", SSRNX_FUNC_ADDR(iswblank), "libc.so"},
    {"iswcntrl", SSRNX_FUNC_ADDR(iswcntrl), "libc.so"},
    {"iswdigit", SSRNX_FUNC_ADDR(iswdigit), "libc.so"},
    {"iswgraph", SSRNX_FUNC_ADDR(iswgraph), "libc.so"},
    {"iswlower", SSRNX_FUNC_ADDR(iswlower), "libc.so"},
    {"iswprint", SSRNX_FUNC_ADDR(iswprint), "libc.so"},
    {"iswpunct", SSRNX_FUNC_ADDR(iswpunct), "libc.so"},
    {"iswspace", SSRNX_FUNC_ADDR(iswspace), "libc.so"},
    {"iswupper", SSRNX_FUNC_ADDR(iswupper), "libc.so"},
    {"iswxdigit", SSRNX_FUNC_ADDR(iswxdigit), "libc.so"},
    {"dirname", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_dirname), "libc.so"},
    {"strsep", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_strsep), "libc.so"},
    {"atoll", SSRNX_FUNC_ADDR(atoll), "libc.so"},
    {"timegm", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_timegm), "libc.so"},
    {"asctime", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_asctime), "libc.so"},
    {"ctime", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_ctime), "libc.so"},
    {"tzset", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_tzset), "libc.so"},
    {"timezone", SSRNX_DATA_ADDR(compat_g_bionic_timezone), "libc.so"},
    {"daylight", SSRNX_DATA_ADDR(compat_g_bionic_daylight), "libc.so"},
    {"tzname", SSRNX_DATA_ADDR(compat_g_bionic_tzname), "libc.so"},
    {"getopt_long", SSRNX_FUNC_ADDR(getopt_long), "libc.so"},
    {"optarg", SSRNX_DATA_ADDR(optarg), "libc.so"},
    {"optind", SSRNX_DATA_ADDR(optind), "libc.so"},
    {"opterr", SSRNX_DATA_ADDR(opterr), "libc.so"},
    {"optopt", SSRNX_DATA_ADDR(optopt), "libc.so"},
    {"system", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_system), "libc.so"},
    {"execv", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_execv), "libc.so"},
    {"execve", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_execve), "libc.so"},
    {"getppid", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getppid), "libc.so"},
    {"sched_get_priority_max", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sched_get_priority_max), "libc.so"},
    {"sched_get_priority_min", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sched_get_priority_min), "libc.so"},
    {"sched_getscheduler", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sched_getscheduler), "libc.so"},
    {"sched_getparam", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sched_getparam), "libc.so"},
    {"sysinfo", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sysinfo), "libc.so"},
    {"fchown", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fchown), "libc.so"},
    {"lchown", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_lchown), "libc.so"},
    {"mknod", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mknod), "libc.so"},
    {"mkfifo", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mkfifo), "libc.so"},
    {"getpwnam", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getpwnam), "libc.so"},
    {"getgrnam", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getgrnam), "libc.so"},
    {"getgrgid", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getgrgid), "libc.so"},
    {"getentropy", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getentropy), "libc.so"},
    {"getrandom", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getrandom), "libc.so"},
    {"arc4random", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_arc4random), "libc.so"},
    {"arc4random_buf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_arc4random_buf), "libc.so"},
    {"__cmsg_nxthdr", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_cmsg_nxthdr), "libc.so"},
    {"sendmmsg", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sendmmsg), "libc.so"},
    {"recvmmsg", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_recvmmsg), "libc.so"},
    {"epoll_create", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_epoll_create), "libc.so"},
    {"epoll_create1", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_epoll_create1), "libc.so"},
    {"epoll_ctl", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_epoll_ctl), "libc.so"},
    {"epoll_wait", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_epoll_wait), "libc.so"},
    {"epoll_pwait", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_epoll_pwait), "libc.so"},
    {"eventfd", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_eventfd), "libc.so"},
    {"eventfd_read", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_eventfd_read), "libc.so"},
    {"eventfd_write", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_eventfd_write), "libc.so"},
    {"inotify_init", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_enosys), "libc.so"},
    {"inotify_init1", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_enosys), "libc.so"},
    {"inotify_add_watch", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_enosys), "libc.so"},
    {"inotify_rm_watch", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_enosys), "libc.so"},

    /* Bionic LP64 native import surface. Unsupported facilities report errors. */
    {"dirfd", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_dirfd), "libc.so"},
    {"mkdtemp", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mkdtemp), "libc.so"},
    {"getpwnam_r", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_getpwnam_r), "libc.so"},
    {"execvpe", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_execvpe), "libc.so"},
    {"setsid", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_setsid), "libc.so"},
    {"vfork", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fork), "libc.so"},
    {"chown", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_lchown), "libc.so"},
    {"nftw", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_nftw), "libc.so"},
    {"__assert", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_assert), "libc.so"},
    {"__fread_chk", SSRNX_FUNC_ADDR(compat_ssrnx_fread_chk), "libc.so"},
    {"__sendto_chk", SSRNX_FUNC_ADDR(compat_ssrnx_sendto_chk), "libc.so"},
    {"timerfd_create", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_timerfd_create), "libc.so"},
    {"timerfd_settime", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_timerfd_settime), "libc.so"},
    {"timerfd_gettime", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_timerfd_gettime), "libc.so"},
    {"posix_fallocate", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_posix_fallocate), "libc.so"},
    {"posix_fallocate64", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_posix_fallocate), "libc.so"},
    {"sched_setscheduler", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sched_setscheduler), "libc.so"},
    {"sched_getcpu", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sched_getcpu), "libc.so"},
    {"mallinfo", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_mallinfo), "libc.so"},
    {"msync", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_msync), "libc.so"},
    {"ldiv", SSRNX_FUNC_ADDR(ldiv), "libc.so"},
    {"_Exit", SSRNX_FUNC_ADDR(compat_ssrnx_bionic__exit), "libc.so"},
    {"in6addr_any", SSRNX_DATA_ADDR(g_bionic_in6addr_any), "libc.so"},
    {"in6addr_loopback", SSRNX_DATA_ADDR(g_bionic_in6addr_loopback), "libc.so"},
    /* libm.so */
    {"nan", SSRNX_FUNC_ADDR(nan), "libm.so"},
    {"remquof", SSRNX_FUNC_ADDR(remquof), "libm.so"},
    {"cbrt", SSRNX_FUNC_ADDR(cbrt), "libm.so"},
    {"erfcf", SSRNX_FUNC_ADDR(erfcf), "libm.so"},
    {"erff", SSRNX_FUNC_ADDR(erff), "libm.so"},
    {"round", SSRNX_FUNC_ADDR(round), "libm.so"},
    {"roundf", SSRNX_FUNC_ADDR(roundf), "libm.so"},
    {"copysign", SSRNX_FUNC_ADDR(copysign), "libm.so"},
    {"copysignf", SSRNX_FUNC_ADDR(copysignf), "libm.so"},
    {"finitef", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_finitef), "libm.so"},
    {"ilogb", SSRNX_FUNC_ADDR(ilogb), "libm.so"},
    {"expm1", SSRNX_FUNC_ADDR(expm1), "libm.so"},
    {"fmal", SSRNX_FUNC_ADDR(fmal), "libm.so"},
    {"powl", SSRNX_FUNC_ADDR(powl), "libm.so"},
    {"fabs", SSRNX_FUNC_ADDR(fabs), "libm.so"},
    {"fabsf", SSRNX_FUNC_ADDR(fabsf), "libm.so"},
    {"sin", SSRNX_FUNC_ADDR(sin), "libm.so"},
    {"sinf", SSRNX_FUNC_ADDR(sinf), "libm.so"},
    {"cos", SSRNX_FUNC_ADDR(cos), "libm.so"},
    {"cosf", SSRNX_FUNC_ADDR(cosf), "libm.so"},
    {"sincos", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sincos), "libm.so"},
    {"sincosf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_sincosf), "libm.so"},
    {"sqrt", SSRNX_FUNC_ADDR(sqrt), "libm.so"},
    {"sqrtf", SSRNX_FUNC_ADDR(sqrtf), "libm.so"},
    {"floor", SSRNX_FUNC_ADDR(floor), "libm.so"},
    {"floorf", SSRNX_FUNC_ADDR(floorf), "libm.so"},
    {"ceil", SSRNX_FUNC_ADDR(ceil), "libm.so"},
    {"ceilf", SSRNX_FUNC_ADDR(ceilf), "libm.so"},
    {"pow", SSRNX_FUNC_ADDR(pow), "libm.so"},
    {"powf", SSRNX_FUNC_ADDR(powf), "libm.so"},
    {"fmodf", SSRNX_FUNC_ADDR(fmodf), "libm.so"},
    {"remainderf", SSRNX_FUNC_ADDR(remainderf), "libm.so"},
    {"acosf", SSRNX_FUNC_ADDR(acosf), "libm.so"},
    {"ldexpf", SSRNX_FUNC_ADDR(ldexpf), "libm.so"},
    {"tan", SSRNX_FUNC_ADDR(tan), "libm.so"},
    {"atan2", SSRNX_FUNC_ADDR(atan2), "libm.so"},
    {"hypot", SSRNX_FUNC_ADDR(hypot), "libm.so"},
    {"acos", SSRNX_FUNC_ADDR(acos), "libm.so"},
    {"atan", SSRNX_FUNC_ADDR(atan), "libm.so"},
    {"atanf", SSRNX_FUNC_ADDR(atanf), "libm.so"},
    {"log10", SSRNX_FUNC_ADDR(log10), "libm.so"},
    {"log10f", SSRNX_FUNC_ADDR(log10f), "libm.so"},
    {"logf", SSRNX_FUNC_ADDR(logf), "libm.so"},
    {"asin", SSRNX_FUNC_ADDR(asin), "libm.so"},
    {"asinf", SSRNX_FUNC_ADDR(asinf), "libm.so"},
    {"atan2f", SSRNX_FUNC_ADDR(atan2f), "libm.so"},
    {"log2", SSRNX_FUNC_ADDR(log2), "libm.so"},
    {"exp2f", SSRNX_FUNC_ADDR(exp2f), "libm.so"},
    {"expf", SSRNX_FUNC_ADDR(expf), "libm.so"},
    {"tanf", SSRNX_FUNC_ADDR(tanf), "libm.so"},
    {"log", SSRNX_FUNC_ADDR(log), "libm.so"},
    /* Android/Bionic historical ABI quirk: double-precision ldexp() is
       exported by libc.so, while ldexpf()/ldexpl() remain in libm.so.
       Preserve the provider because ELF VERNEED records name libc.so. */
    {"ldexp", SSRNX_FUNC_ADDR(ldexp), "libc.so"},
    {"modf", SSRNX_FUNC_ADDR(modf), "libm.so"},
    /* Bionic exports floating-point classification from libc, including the
       plain isnan/isinf names. DT_VERNEED provider identity must match. */
    {"__isnanf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isnanf), "libc.so"},
    {"isnan", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isnan), "libc.so"},
    {"isinf", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_isinf), "libc.so"},
    {"cbrtf", SSRNX_FUNC_ADDR(cbrtf), "libm.so"},
    {"hypotf", SSRNX_FUNC_ADDR(hypotf), "libm.so"},
    {"log2f", SSRNX_FUNC_ADDR(log2f), "libm.so"},
    {"exp2", SSRNX_FUNC_ADDR(exp2), "libm.so"},
    {"nearbyintf", SSRNX_FUNC_ADDR(nearbyintf), "libm.so"},
    {"nextafter", SSRNX_FUNC_ADDR(nextafter), "libm.so"},
    {"nextafterf", SSRNX_FUNC_ADDR(nextafterf), "libm.so"},
    {"modff", SSRNX_FUNC_ADDR(modff), "libm.so"},
    {"fmin", SSRNX_FUNC_ADDR(fmin), "libm.so"},
    {"fmax", SSRNX_FUNC_ADDR(fmax), "libm.so"},
    {"fminf", SSRNX_FUNC_ADDR(fminf), "libm.so"},
    {"fmaxf", SSRNX_FUNC_ADDR(fmaxf), "libm.so"},
    {"fmod", SSRNX_FUNC_ADDR(fmod), "libm.so"},
    {"lround", SSRNX_FUNC_ADDR(lround), "libm.so"},
    {"tanhf", SSRNX_FUNC_ADDR(tanhf), "libm.so"},
    {"tanh", SSRNX_FUNC_ADDR(tanh), "libm.so"},
    {"exp", SSRNX_FUNC_ADDR(exp), "libm.so"},
    {"frexp", SSRNX_FUNC_ADDR(frexp), "libm.so"},
    {"frexpf", SSRNX_FUNC_ADDR(frexpf), "libm.so"},
    {"expl", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_expl), "libm.so"},
    {"logb", SSRNX_FUNC_ADDR(logb), "libm.so"},
    {"scalbn", SSRNX_FUNC_ADDR(scalbn), "libm.so"},
    {"asinhf", SSRNX_FUNC_ADDR(asinhf), "libm.so"},
    {"asinh", SSRNX_FUNC_ADDR(asinh), "libm.so"},
    {"sinhf", SSRNX_FUNC_ADDR(sinhf), "libm.so"},
    {"sinh", SSRNX_FUNC_ADDR(sinh), "libm.so"},
    {"coshf", SSRNX_FUNC_ADDR(coshf), "libm.so"},
    {"cosh", SSRNX_FUNC_ADDR(cosh), "libm.so"},
    {"acoshf", SSRNX_FUNC_ADDR(acoshf), "libm.so"},
    {"acosh", SSRNX_FUNC_ADDR(acosh), "libm.so"},
    {"atanhf", SSRNX_FUNC_ADDR(atanhf), "libm.so"},
    {"atanh", SSRNX_FUNC_ADDR(atanh), "libm.so"},
    {"__fpclassifyd", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_fpclassifyd), "libc.so"},
    {"__signbit", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_signbit), "libm.so"},

    /* Android log + dynamic-loader compatibility. */
    {"__android_log_print", SSRNX_FUNC_ADDR(compat_ssrnx_android_log_print), "liblog.so"},
    {"__android_log_vprint", SSRNX_FUNC_ADDR(compat_ssrnx_android_log_vprint), "liblog.so"},
    {"__android_log_write", SSRNX_FUNC_ADDR(compat_ssrnx_android_log_write), "liblog.so"},
    {"__android_log_buf_write", SSRNX_FUNC_ADDR(compat_ssrnx_android_log_buf_write), "liblog.so"},
    {"__android_log_assert", SSRNX_FUNC_ADDR(compat_ssrnx_android_log_assert), "liblog.so"},
    {"dlopen", SSRNX_FUNC_ADDR(compat_ssrnx_dlopen), "libdl.so"},
    {"dlsym", SSRNX_FUNC_ADDR(compat_ssrnx_dlsym), "libdl.so"},
    {"dlclose", SSRNX_FUNC_ADDR(compat_ssrnx_dlclose), "libdl.so"},
    {"dlerror", SSRNX_FUNC_ADDR(compat_ssrnx_dlerror), "libdl.so"},
    {"dladdr", SSRNX_FUNC_ADDR(compat_ssrnx_bionic_dladdr), "libdl.so"},
    {"dl_iterate_phdr", SSRNX_FUNC_ADDR(compat_ssrnx_dl_iterate_phdr), "libdl.so"},

    /* NativeActivity, native window, configuration, looper and input queue. */
    {"ANativeActivity_finish", SSRNX_FUNC_ADDR(ANativeActivity_finish), "libandroid.so"},
    {"ANativeActivity_setWindowFormat", SSRNX_FUNC_ADDR(ANativeActivity_setWindowFormat), "libandroid.so"},
    {"ANativeActivity_setWindowFlags", SSRNX_FUNC_ADDR(ANativeActivity_setWindowFlags), "libandroid.so"},
    {"ANativeActivity_showSoftInput", SSRNX_FUNC_ADDR(ANativeActivity_showSoftInput), "libandroid.so"},
    {"ANativeActivity_hideSoftInput", SSRNX_FUNC_ADDR(ANativeActivity_hideSoftInput), "libandroid.so"},
    {"ANativeWindow_acquire", SSRNX_FUNC_ADDR(ANativeWindow_acquire), "libandroid.so"},
    {"ANativeWindow_release", SSRNX_FUNC_ADDR(ANativeWindow_release), "libandroid.so"},
    {"ANativeWindow_getWidth", SSRNX_FUNC_ADDR(ANativeWindow_getWidth), "libandroid.so"},
    {"ANativeWindow_getHeight", SSRNX_FUNC_ADDR(ANativeWindow_getHeight), "libandroid.so"},
    {"ANativeWindow_getFormat", SSRNX_FUNC_ADDR(ANativeWindow_getFormat), "libandroid.so"},
    {"ANativeWindow_setBuffersGeometry", SSRNX_FUNC_ADDR(ANativeWindow_setBuffersGeometry), "libandroid.so"},
    {"ANativeWindow_fromSurface", SSRNX_FUNC_ADDR(ANativeWindow_fromSurface), "libandroid.so"},
    {"ANativeWindow_toSurface", SSRNX_FUNC_ADDR(ANativeWindow_toSurface), "libandroid.so"},
    {"ANativeWindow_lock", SSRNX_FUNC_ADDR(ANativeWindow_lock), "libandroid.so"},
    {"ANativeWindow_unlockAndPost", SSRNX_FUNC_ADDR(ANativeWindow_unlockAndPost), "libandroid.so"},
    {"AChoreographer_getInstance", SSRNX_FUNC_ADDR(AChoreographer_getInstance), "libandroid.so"},
    {"AChoreographer_postFrameCallback", SSRNX_FUNC_ADDR(AChoreographer_postFrameCallback), "libandroid.so"},
    {"AChoreographer_postFrameCallback64", SSRNX_FUNC_ADDR(AChoreographer_postFrameCallback64), "libandroid.so"},
    {"AChoreographer_postFrameCallbackDelayed", SSRNX_FUNC_ADDR(AChoreographer_postFrameCallbackDelayed), "libandroid.so"},
    {"AChoreographer_postFrameCallbackDelayed64", SSRNX_FUNC_ADDR(AChoreographer_postFrameCallbackDelayed64), "libandroid.so"},
    {"AConfiguration_new", SSRNX_FUNC_ADDR(AConfiguration_new), "libandroid.so"},
    {"AConfiguration_delete", SSRNX_FUNC_ADDR(AConfiguration_delete), "libandroid.so"},
    {"AConfiguration_copy", SSRNX_FUNC_ADDR(AConfiguration_copy), "libandroid.so"},
    {"AConfiguration_fromAssetManager", SSRNX_FUNC_ADDR(AConfiguration_fromAssetManager), "libandroid.so"},
    {"AConfiguration_getMcc", SSRNX_FUNC_ADDR(AConfiguration_getMcc), "libandroid.so"},
    {"AConfiguration_setMcc", SSRNX_FUNC_ADDR(AConfiguration_setMcc), "libandroid.so"},
    {"AConfiguration_getMnc", SSRNX_FUNC_ADDR(AConfiguration_getMnc), "libandroid.so"},
    {"AConfiguration_setMnc", SSRNX_FUNC_ADDR(AConfiguration_setMnc), "libandroid.so"},
    {"AConfiguration_getLanguage", SSRNX_FUNC_ADDR(AConfiguration_getLanguage), "libandroid.so"},
    {"AConfiguration_setLanguage", SSRNX_FUNC_ADDR(AConfiguration_setLanguage), "libandroid.so"},
    {"AConfiguration_getCountry", SSRNX_FUNC_ADDR(AConfiguration_getCountry), "libandroid.so"},
    {"AConfiguration_setCountry", SSRNX_FUNC_ADDR(AConfiguration_setCountry), "libandroid.so"},
    {"AConfiguration_getOrientation", SSRNX_FUNC_ADDR(AConfiguration_getOrientation), "libandroid.so"},
    {"AConfiguration_setOrientation", SSRNX_FUNC_ADDR(AConfiguration_setOrientation), "libandroid.so"},
    {"AConfiguration_getTouchscreen", SSRNX_FUNC_ADDR(AConfiguration_getTouchscreen), "libandroid.so"},
    {"AConfiguration_setTouchscreen", SSRNX_FUNC_ADDR(AConfiguration_setTouchscreen), "libandroid.so"},
    {"AConfiguration_getDensity", SSRNX_FUNC_ADDR(AConfiguration_getDensity), "libandroid.so"},
    {"AConfiguration_setDensity", SSRNX_FUNC_ADDR(AConfiguration_setDensity), "libandroid.so"},
    {"AConfiguration_getKeyboard", SSRNX_FUNC_ADDR(AConfiguration_getKeyboard), "libandroid.so"},
    {"AConfiguration_setKeyboard", SSRNX_FUNC_ADDR(AConfiguration_setKeyboard), "libandroid.so"},
    {"AConfiguration_getNavigation", SSRNX_FUNC_ADDR(AConfiguration_getNavigation), "libandroid.so"},
    {"AConfiguration_setNavigation", SSRNX_FUNC_ADDR(AConfiguration_setNavigation), "libandroid.so"},
    {"AConfiguration_getKeysHidden", SSRNX_FUNC_ADDR(AConfiguration_getKeysHidden), "libandroid.so"},
    {"AConfiguration_setKeysHidden", SSRNX_FUNC_ADDR(AConfiguration_setKeysHidden), "libandroid.so"},
    {"AConfiguration_getNavHidden", SSRNX_FUNC_ADDR(AConfiguration_getNavHidden), "libandroid.so"},
    {"AConfiguration_setNavHidden", SSRNX_FUNC_ADDR(AConfiguration_setNavHidden), "libandroid.so"},
    {"AConfiguration_getSdkVersion", SSRNX_FUNC_ADDR(AConfiguration_getSdkVersion), "libandroid.so"},
    {"AConfiguration_setSdkVersion", SSRNX_FUNC_ADDR(AConfiguration_setSdkVersion), "libandroid.so"},
    {"AConfiguration_getScreenSize", SSRNX_FUNC_ADDR(AConfiguration_getScreenSize), "libandroid.so"},
    {"AConfiguration_setScreenSize", SSRNX_FUNC_ADDR(AConfiguration_setScreenSize), "libandroid.so"},
    {"AConfiguration_getScreenLong", SSRNX_FUNC_ADDR(AConfiguration_getScreenLong), "libandroid.so"},
    {"AConfiguration_setScreenLong", SSRNX_FUNC_ADDR(AConfiguration_setScreenLong), "libandroid.so"},
    {"AConfiguration_getUiModeType", SSRNX_FUNC_ADDR(AConfiguration_getUiModeType), "libandroid.so"},
    {"AConfiguration_setUiModeType", SSRNX_FUNC_ADDR(AConfiguration_setUiModeType), "libandroid.so"},
    {"AConfiguration_getUiModeNight", SSRNX_FUNC_ADDR(AConfiguration_getUiModeNight), "libandroid.so"},
    {"AConfiguration_setUiModeNight", SSRNX_FUNC_ADDR(AConfiguration_setUiModeNight), "libandroid.so"},
    {"AConfiguration_getScreenWidthDp", SSRNX_FUNC_ADDR(AConfiguration_getScreenWidthDp), "libandroid.so"},
    {"AConfiguration_setScreenWidthDp", SSRNX_FUNC_ADDR(AConfiguration_setScreenWidthDp), "libandroid.so"},
    {"AConfiguration_getScreenHeightDp", SSRNX_FUNC_ADDR(AConfiguration_getScreenHeightDp), "libandroid.so"},
    {"AConfiguration_setScreenHeightDp", SSRNX_FUNC_ADDR(AConfiguration_setScreenHeightDp), "libandroid.so"},
    {"AConfiguration_getSmallestScreenWidthDp", SSRNX_FUNC_ADDR(AConfiguration_getSmallestScreenWidthDp), "libandroid.so"},
    {"AConfiguration_setSmallestScreenWidthDp", SSRNX_FUNC_ADDR(AConfiguration_setSmallestScreenWidthDp), "libandroid.so"},
    {"AConfiguration_getLayoutDirection", SSRNX_FUNC_ADDR(AConfiguration_getLayoutDirection), "libandroid.so"},
    {"AConfiguration_setLayoutDirection", SSRNX_FUNC_ADDR(AConfiguration_setLayoutDirection), "libandroid.so"},
    {"AConfiguration_getScreenRound", SSRNX_FUNC_ADDR(AConfiguration_getScreenRound), "libandroid.so"},
    {"AConfiguration_setScreenRound", SSRNX_FUNC_ADDR(AConfiguration_setScreenRound), "libandroid.so"},
    {"ALooper_forThread", SSRNX_FUNC_ADDR(ALooper_forThread), "libandroid.so"},
    {"ALooper_prepare", SSRNX_FUNC_ADDR(ALooper_prepare), "libandroid.so"},
    {"ALooper_acquire", SSRNX_FUNC_ADDR(ALooper_acquire), "libandroid.so"},
    {"ALooper_release", SSRNX_FUNC_ADDR(ALooper_release), "libandroid.so"},
    {"ALooper_addFd", SSRNX_FUNC_ADDR(ALooper_addFd), "libandroid.so"},
    {"ALooper_removeFd", SSRNX_FUNC_ADDR(ALooper_removeFd), "libandroid.so"},
    {"ALooper_pollOnce", SSRNX_FUNC_ADDR(ALooper_pollOnce), "libandroid.so"},
    {"ALooper_pollAll", SSRNX_FUNC_ADDR(ALooper_pollAll), "libandroid.so"},
    {"ALooper_wake", SSRNX_FUNC_ADDR(ALooper_wake), "libandroid.so"},
    {"AInputQueue_attachLooper", SSRNX_FUNC_ADDR(AInputQueue_attachLooper), "libandroid.so"},
    {"AInputQueue_detachLooper", SSRNX_FUNC_ADDR(AInputQueue_detachLooper), "libandroid.so"},
    {"AInputQueue_getEvent", SSRNX_FUNC_ADDR(AInputQueue_getEvent), "libandroid.so"},
    {"AInputQueue_preDispatchEvent", SSRNX_FUNC_ADDR(AInputQueue_preDispatchEvent), "libandroid.so"},
    {"AInputQueue_finishEvent", SSRNX_FUNC_ADDR(AInputQueue_finishEvent), "libandroid.so"},
    {"AInputEvent_getType", SSRNX_FUNC_ADDR(AInputEvent_getType), "libandroid.so"},
    {"AKeyEvent_getAction", SSRNX_FUNC_ADDR(AKeyEvent_getAction), "libandroid.so"},
    {"AKeyEvent_getKeyCode", SSRNX_FUNC_ADDR(AKeyEvent_getKeyCode), "libandroid.so"},
    {"AMotionEvent_getAction", SSRNX_FUNC_ADDR(AMotionEvent_getAction), "libandroid.so"},
    {"AMotionEvent_getPointerCount", SSRNX_FUNC_ADDR(AMotionEvent_getPointerCount), "libandroid.so"},
    {"AMotionEvent_getPointerId", SSRNX_FUNC_ADDR(AMotionEvent_getPointerId), "libandroid.so"},
    {"AMotionEvent_getX", SSRNX_FUNC_ADDR(AMotionEvent_getX), "libandroid.so"},
    {"AMotionEvent_getY", SSRNX_FUNC_ADDR(AMotionEvent_getY), "libandroid.so"},
    {"ASensorManager_getInstance", SSRNX_FUNC_ADDR(ASensorManager_getInstance), "libandroid.so"},
    {"ASensorManager_getInstanceForPackage", SSRNX_FUNC_ADDR(ASensorManager_getInstanceForPackage), "libandroid.so"},
    {"ASensorManager_getSensorList", SSRNX_FUNC_ADDR(ASensorManager_getSensorList), "libandroid.so"},
    {"ASensorManager_getDefaultSensor", SSRNX_FUNC_ADDR(ASensorManager_getDefaultSensor), "libandroid.so"},
    {"ASensorManager_createEventQueue", SSRNX_FUNC_ADDR(ASensorManager_createEventQueue), "libandroid.so"},
    {"ASensorManager_destroyEventQueue", SSRNX_FUNC_ADDR(ASensorManager_destroyEventQueue), "libandroid.so"},
    {"ASensorEventQueue_enableSensor", SSRNX_FUNC_ADDR(ASensorEventQueue_enableSensor), "libandroid.so"},
    {"ASensorEventQueue_disableSensor", SSRNX_FUNC_ADDR(ASensorEventQueue_disableSensor), "libandroid.so"},
    {"ASensorEventQueue_setEventRate", SSRNX_FUNC_ADDR(ASensorEventQueue_setEventRate), "libandroid.so"},
    {"ASensorEventQueue_hasEvents", SSRNX_FUNC_ADDR(ASensorEventQueue_hasEvents), "libandroid.so"},
    {"ASensorEventQueue_getEvents", SSRNX_FUNC_ADDR(ASensorEventQueue_getEvents), "libandroid.so"},
    {"ASensor_getName", SSRNX_FUNC_ADDR(ASensor_getName), "libandroid.so"},
    {"ASensor_getVendor", SSRNX_FUNC_ADDR(ASensor_getVendor), "libandroid.so"},
    {"ASensor_getType", SSRNX_FUNC_ADDR(ASensor_getType), "libandroid.so"},
    {"ASensor_getResolution", SSRNX_FUNC_ADDR(ASensor_getResolution), "libandroid.so"},
    {"ASensor_getMinDelay", SSRNX_FUNC_ADDR(ASensor_getMinDelay), "libandroid.so"},

    /* Android NDK media surface. AMediaFormat is a typed container; the
       extractor/codec backend below provides real WAV/audio-raw operation and
       reports unsupported compressed codecs instead of fabricating decode. */
    {"AMediaFormat_new", SSRNX_FUNC_ADDR(ssrnx_AMediaFormat_new), "libmediandk.so"},
    {"AMediaFormat_delete", SSRNX_FUNC_ADDR(ssrnx_AMediaFormat_delete), "libmediandk.so"},
    {"AMediaFormat_getInt32", SSRNX_FUNC_ADDR(ssrnx_AMediaFormat_getInt32), "libmediandk.so"},
    {"AMediaFormat_getInt64", SSRNX_FUNC_ADDR(ssrnx_AMediaFormat_getInt64), "libmediandk.so"},
    {"AMediaFormat_getFloat", SSRNX_FUNC_ADDR(ssrnx_AMediaFormat_getFloat), "libmediandk.so"},
    {"AMediaFormat_getString", SSRNX_FUNC_ADDR(ssrnx_AMediaFormat_getString), "libmediandk.so"},
    {"AMediaFormat_getBuffer", SSRNX_FUNC_ADDR(ssrnx_AMediaFormat_getBuffer), "libmediandk.so"},
    {"AMediaFormat_setBuffer", SSRNX_FUNC_ADDR(ssrnx_AMediaFormat_setBuffer), "libmediandk.so"},
    {"AMediaFormat_toString", SSRNX_FUNC_ADDR(ssrnx_AMediaFormat_toString), "libmediandk.so"},
    {"AMediaFormat_setInt32", SSRNX_FUNC_ADDR(ssrnx_AMediaFormat_setInt32), "libmediandk.so"},
    {"AMediaFormat_setInt64", SSRNX_FUNC_ADDR(ssrnx_AMediaFormat_setInt64), "libmediandk.so"},
    {"AMediaFormat_setFloat", SSRNX_FUNC_ADDR(ssrnx_AMediaFormat_setFloat), "libmediandk.so"},
    {"AMediaFormat_setString", SSRNX_FUNC_ADDR(ssrnx_AMediaFormat_setString), "libmediandk.so"},
    {"AMediaExtractor_new", SSRNX_FUNC_ADDR(ssrnx_AMediaExtractor_new), "libmediandk.so"},
    {"AMediaExtractor_delete", SSRNX_FUNC_ADDR(ssrnx_AMediaExtractor_delete), "libmediandk.so"},
    {"AMediaExtractor_setDataSourceFd", SSRNX_FUNC_ADDR(ssrnx_AMediaExtractor_setDataSourceFd), "libmediandk.so"},
    {"AMediaExtractor_setDataSource", SSRNX_FUNC_ADDR(ssrnx_AMediaExtractor_setDataSource), "libmediandk.so"},
    {"AMediaExtractor_setDataSourceCustom", SSRNX_FUNC_ADDR(ssrnx_AMediaExtractor_setDataSourceCustom), "libmediandk.so"},
    {"AMediaDataSource_new", SSRNX_FUNC_ADDR(ssrnx_AMediaDataSource_new), "libmediandk.so"},
    {"AMediaDataSource_delete", SSRNX_FUNC_ADDR(ssrnx_AMediaDataSource_delete), "libmediandk.so"},
    {"AMediaDataSource_setUserdata", SSRNX_FUNC_ADDR(ssrnx_AMediaDataSource_setUserdata), "libmediandk.so"},
    {"AMediaDataSource_setReadAt", SSRNX_FUNC_ADDR(ssrnx_AMediaDataSource_setReadAt), "libmediandk.so"},
    {"AMediaDataSource_setGetSize", SSRNX_FUNC_ADDR(ssrnx_AMediaDataSource_setGetSize), "libmediandk.so"},
    {"AMediaDataSource_setClose", SSRNX_FUNC_ADDR(ssrnx_AMediaDataSource_setClose), "libmediandk.so"},
    {"AMediaExtractor_getTrackCount", SSRNX_FUNC_ADDR(ssrnx_AMediaExtractor_getTrackCount), "libmediandk.so"},
    {"AMediaExtractor_getTrackFormat", SSRNX_FUNC_ADDR(ssrnx_AMediaExtractor_getTrackFormat), "libmediandk.so"},
    {"AMediaExtractor_selectTrack", SSRNX_FUNC_ADDR(ssrnx_AMediaExtractor_selectTrack), "libmediandk.so"},
    {"AMediaExtractor_readSampleData", SSRNX_FUNC_ADDR(ssrnx_AMediaExtractor_readSampleData), "libmediandk.so"},
    {"AMediaExtractor_getSampleTrackIndex", SSRNX_FUNC_ADDR(ssrnx_AMediaExtractor_getSampleTrackIndex), "libmediandk.so"},
    {"AMediaExtractor_getSampleTime", SSRNX_FUNC_ADDR(ssrnx_AMediaExtractor_getSampleTime), "libmediandk.so"},
    {"AMediaExtractor_advance", SSRNX_FUNC_ADDR(ssrnx_AMediaExtractor_advance), "libmediandk.so"},
    {"AMediaExtractor_seekTo", SSRNX_FUNC_ADDR(ssrnx_AMediaExtractor_seekTo), "libmediandk.so"},
    {"AMediaCodec_createDecoderByType", SSRNX_FUNC_ADDR(ssrnx_AMediaCodec_createDecoderByType), "libmediandk.so"},
    {"AMediaCodec_createEncoderByType", SSRNX_FUNC_ADDR(ssrnx_AMediaCodec_createEncoderByType), "libmediandk.so"},
    {"AMediaCodec_delete", SSRNX_FUNC_ADDR(ssrnx_AMediaCodec_delete), "libmediandk.so"},
    {"AMediaCodec_configure", SSRNX_FUNC_ADDR(ssrnx_AMediaCodec_configure), "libmediandk.so"},
    {"AMediaCodec_start", SSRNX_FUNC_ADDR(ssrnx_AMediaCodec_start), "libmediandk.so"},
    {"AMediaCodec_stop", SSRNX_FUNC_ADDR(ssrnx_AMediaCodec_stop), "libmediandk.so"},
    {"AMediaCodec_flush", SSRNX_FUNC_ADDR(ssrnx_AMediaCodec_flush), "libmediandk.so"},
    {"AMediaCodec_dequeueInputBuffer", SSRNX_FUNC_ADDR(ssrnx_AMediaCodec_dequeueInputBuffer), "libmediandk.so"},
    {"AMediaCodec_getInputBuffer", SSRNX_FUNC_ADDR(ssrnx_AMediaCodec_getInputBuffer), "libmediandk.so"},
    {"AMediaCodec_queueInputBuffer", SSRNX_FUNC_ADDR(ssrnx_AMediaCodec_queueInputBuffer), "libmediandk.so"},
    {"AMediaCodec_dequeueOutputBuffer", SSRNX_FUNC_ADDR(ssrnx_AMediaCodec_dequeueOutputBuffer), "libmediandk.so"},
    {"AMediaCodec_getOutputFormat", SSRNX_FUNC_ADDR(ssrnx_AMediaCodec_getOutputFormat), "libmediandk.so"},
    {"AMediaCodec_getOutputBuffer", SSRNX_FUNC_ADDR(ssrnx_AMediaCodec_getOutputBuffer), "libmediandk.so"},
    {"AMediaCodec_releaseOutputBuffer", SSRNX_FUNC_ADDR(ssrnx_AMediaCodec_releaseOutputBuffer), "libmediandk.so"},
    {"AMediaCodec_setOutputSurface", SSRNX_FUNC_ADDR(ssrnx_AMediaCodec_setOutputSurface), "libmediandk.so"},
    {"AImage_getWidth", SSRNX_FUNC_ADDR(ssrnx_AImage_getWidth), "libmediandk.so"},
    {"AImage_getHeight", SSRNX_FUNC_ADDR(ssrnx_AImage_getHeight), "libmediandk.so"},
    {"AImage_getFormat", SSRNX_FUNC_ADDR(ssrnx_AImage_getFormat), "libmediandk.so"},
    {"AImage_getTimestamp", SSRNX_FUNC_ADDR(ssrnx_AImage_getTimestamp), "libmediandk.so"},
    {"AImage_getNumberOfPlanes", SSRNX_FUNC_ADDR(ssrnx_AImage_getNumberOfPlanes), "libmediandk.so"},
    {"AImage_getCropRect", SSRNX_FUNC_ADDR(ssrnx_AImage_getCropRect), "libmediandk.so"},

    /* Android NDK media public data keys. */
    {"AMEDIAFORMAT_KEY_CHANNEL_COUNT", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_CHANNEL_COUNT), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_BIT_RATE", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_BIT_RATE), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_I_FRAME_INTERVAL", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_I_FRAME_INTERVAL), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_COLOR_FORMAT", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_COLOR_FORMAT), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_COLOR_RANGE", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_COLOR_RANGE), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_COLOR_STANDARD", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_COLOR_STANDARD), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_DURATION", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_DURATION), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_ENCODER_DELAY", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_ENCODER_DELAY), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_FRAME_RATE", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_FRAME_RATE), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_HEIGHT", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_HEIGHT), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_LANGUAGE", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_LANGUAGE), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_MIME", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_MIME), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_ROTATION", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_ROTATION), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_SAMPLE_RATE", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_SAMPLE_RATE), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_SLICE_HEIGHT", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_SLICE_HEIGHT), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_STRIDE", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_STRIDE), "libmediandk.so"},
    {"AMEDIAFORMAT_KEY_WIDTH", SSRNX_DATA_ADDR(ssrnx_AMEDIAFORMAT_KEY_WIDTH), "libmediandk.so"},

    /* OpenSL ES entry point and exported IID data symbols. */
    {"AAudio_convertResultToText", SSRNX_FUNC_ADDR(AAudio_convertResultToText), "libaaudio.so"},
    {"AAudio_convertStreamStateToText", SSRNX_FUNC_ADDR(AAudio_convertStreamStateToText), "libaaudio.so"},
    {"AAudio_createStreamBuilder", SSRNX_FUNC_ADDR(AAudio_createStreamBuilder), "libaaudio.so"},
    {"AAudioStreamBuilder_setDeviceId", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setDeviceId), "libaaudio.so"},
    {"AAudioStreamBuilder_setSampleRate", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setSampleRate), "libaaudio.so"},
    {"AAudioStreamBuilder_setChannelCount", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setChannelCount), "libaaudio.so"},
    {"AAudioStreamBuilder_setSamplesPerFrame", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setSamplesPerFrame), "libaaudio.so"},
    {"AAudioStreamBuilder_setFormat", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setFormat), "libaaudio.so"},
    {"AAudioStreamBuilder_setSharingMode", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setSharingMode), "libaaudio.so"},
    {"AAudioStreamBuilder_setDirection", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setDirection), "libaaudio.so"},
    {"AAudioStreamBuilder_setBufferCapacityInFrames", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setBufferCapacityInFrames), "libaaudio.so"},
    {"AAudioStreamBuilder_setPerformanceMode", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setPerformanceMode), "libaaudio.so"},
    {"AAudioStreamBuilder_setUsage", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setUsage), "libaaudio.so"},
    {"AAudioStreamBuilder_setContentType", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setContentType), "libaaudio.so"},
    {"AAudioStreamBuilder_setInputPreset", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setInputPreset), "libaaudio.so"},
    {"AAudioStreamBuilder_setAllowedCapturePolicy", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setAllowedCapturePolicy), "libaaudio.so"},
    {"AAudioStreamBuilder_setSessionId", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setSessionId), "libaaudio.so"},
    {"AAudioStreamBuilder_setPrivacySensitive", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setPrivacySensitive), "libaaudio.so"},
    {"AAudioStreamBuilder_setChannelMask", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setChannelMask), "libaaudio.so"},
    {"AAudioStreamBuilder_setPackageName", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setPackageName), "libaaudio.so"},
    {"AAudioStreamBuilder_setAttributionTag", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setAttributionTag), "libaaudio.so"},
    {"AAudioStreamBuilder_setDataCallback", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setDataCallback), "libaaudio.so"},
    {"AAudioStreamBuilder_setFramesPerDataCallback", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setFramesPerDataCallback), "libaaudio.so"},
    {"AAudioStreamBuilder_setErrorCallback", SSRNX_FUNC_ADDR(AAudioStreamBuilder_setErrorCallback), "libaaudio.so"},
    {"AAudioStreamBuilder_openStream", SSRNX_FUNC_ADDR(AAudioStreamBuilder_openStream), "libaaudio.so"},
    {"AAudioStreamBuilder_delete", SSRNX_FUNC_ADDR(AAudioStreamBuilder_delete), "libaaudio.so"},
    {"AAudioStream_release", SSRNX_FUNC_ADDR(AAudioStream_release), "libaaudio.so"},
    {"AAudioStream_close", SSRNX_FUNC_ADDR(AAudioStream_close), "libaaudio.so"},
    {"AAudioStream_requestStart", SSRNX_FUNC_ADDR(AAudioStream_requestStart), "libaaudio.so"},
    {"AAudioStream_requestPause", SSRNX_FUNC_ADDR(AAudioStream_requestPause), "libaaudio.so"},
    {"AAudioStream_requestFlush", SSRNX_FUNC_ADDR(AAudioStream_requestFlush), "libaaudio.so"},
    {"AAudioStream_requestStop", SSRNX_FUNC_ADDR(AAudioStream_requestStop), "libaaudio.so"},
    {"AAudioStream_getState", SSRNX_FUNC_ADDR(AAudioStream_getState), "libaaudio.so"},
    {"AAudioStream_waitForStateChange", SSRNX_FUNC_ADDR(AAudioStream_waitForStateChange), "libaaudio.so"},
    {"AAudioStream_read", SSRNX_FUNC_ADDR(AAudioStream_read), "libaaudio.so"},
    {"AAudioStream_write", SSRNX_FUNC_ADDR(AAudioStream_write), "libaaudio.so"},
    {"AAudioStream_setBufferSizeInFrames", SSRNX_FUNC_ADDR(AAudioStream_setBufferSizeInFrames), "libaaudio.so"},
    {"AAudioStream_getBufferSizeInFrames", SSRNX_FUNC_ADDR(AAudioStream_getBufferSizeInFrames), "libaaudio.so"},
    {"AAudioStream_getFramesPerBurst", SSRNX_FUNC_ADDR(AAudioStream_getFramesPerBurst), "libaaudio.so"},
    {"AAudioStream_getBufferCapacityInFrames", SSRNX_FUNC_ADDR(AAudioStream_getBufferCapacityInFrames), "libaaudio.so"},
    {"AAudioStream_getFramesPerDataCallback", SSRNX_FUNC_ADDR(AAudioStream_getFramesPerDataCallback), "libaaudio.so"},
    {"AAudioStream_getXRunCount", SSRNX_FUNC_ADDR(AAudioStream_getXRunCount), "libaaudio.so"},
    {"AAudioStream_getSampleRate", SSRNX_FUNC_ADDR(AAudioStream_getSampleRate), "libaaudio.so"},
    {"AAudioStream_getChannelCount", SSRNX_FUNC_ADDR(AAudioStream_getChannelCount), "libaaudio.so"},
    {"AAudioStream_getSamplesPerFrame", SSRNX_FUNC_ADDR(AAudioStream_getSamplesPerFrame), "libaaudio.so"},
    {"AAudioStream_getDeviceId", SSRNX_FUNC_ADDR(AAudioStream_getDeviceId), "libaaudio.so"},
    {"AAudioStream_getFormat", SSRNX_FUNC_ADDR(AAudioStream_getFormat), "libaaudio.so"},
    {"AAudioStream_getSharingMode", SSRNX_FUNC_ADDR(AAudioStream_getSharingMode), "libaaudio.so"},
    {"AAudioStream_getPerformanceMode", SSRNX_FUNC_ADDR(AAudioStream_getPerformanceMode), "libaaudio.so"},
    {"AAudioStream_getDirection", SSRNX_FUNC_ADDR(AAudioStream_getDirection), "libaaudio.so"},
    {"AAudioStream_getFramesWritten", SSRNX_FUNC_ADDR(AAudioStream_getFramesWritten), "libaaudio.so"},
    {"AAudioStream_getFramesRead", SSRNX_FUNC_ADDR(AAudioStream_getFramesRead), "libaaudio.so"},
    {"AAudioStream_getSessionId", SSRNX_FUNC_ADDR(AAudioStream_getSessionId), "libaaudio.so"},
    {"AAudioStream_getTimestamp", SSRNX_FUNC_ADDR(AAudioStream_getTimestamp), "libaaudio.so"},
    {"AAudioStream_getUsage", SSRNX_FUNC_ADDR(AAudioStream_getUsage), "libaaudio.so"},
    {"AAudioStream_getContentType", SSRNX_FUNC_ADDR(AAudioStream_getContentType), "libaaudio.so"},
    {"AAudioStream_getInputPreset", SSRNX_FUNC_ADDR(AAudioStream_getInputPreset), "libaaudio.so"},
    {"AAudioStream_getAllowedCapturePolicy", SSRNX_FUNC_ADDR(AAudioStream_getAllowedCapturePolicy), "libaaudio.so"},
    {"AAudioStream_isPrivacySensitive", SSRNX_FUNC_ADDR(AAudioStream_isPrivacySensitive), "libaaudio.so"},
    {"AAudioStream_getChannelMask", SSRNX_FUNC_ADDR(AAudioStream_getChannelMask), "libaaudio.so"},
    {"AAudioStream_isMMapUsed", SSRNX_FUNC_ADDR(AAudioStream_isMMapUsed), "libaaudio.so"},
    {"slCreateEngine", SSRNX_FUNC_ADDR(slCreateEngine), "libOpenSLES.so"},
    {"SL_IID_NULL", SSRNX_DATA_ADDR(SSRNX_SL_IID_NULL), "libOpenSLES.so"},
    {"SL_IID_ENGINE", SSRNX_DATA_ADDR(SSRNX_SL_IID_ENGINE), "libOpenSLES.so"},
    {"SL_IID_ENGINECAPABILITIES", SSRNX_DATA_ADDR(SSRNX_SL_IID_ENGINECAPABILITIES), "libOpenSLES.so"},
    {"SL_IID_AUDIOIODEVICECAPABILITIES", SSRNX_DATA_ADDR(SSRNX_SL_IID_AUDIOIODEVICECAPABILITIES), "libOpenSLES.so"},
    {"SL_IID_OUTPUTMIX", SSRNX_DATA_ADDR(SSRNX_SL_IID_OUTPUTMIX), "libOpenSLES.so"},
    {"SL_IID_PLAY", SSRNX_DATA_ADDR(SSRNX_SL_IID_PLAY), "libOpenSLES.so"},
    {"SL_IID_SEEK", SSRNX_DATA_ADDR(SSRNX_SL_IID_SEEK), "libOpenSLES.so"},
    {"SL_IID_MUTESOLO", SSRNX_DATA_ADDR(SSRNX_SL_IID_MUTESOLO), "libOpenSLES.so"},
    {"SL_IID_VOLUME", SSRNX_DATA_ADDR(SSRNX_SL_IID_VOLUME), "libOpenSLES.so"},
    {"SL_IID_ENVIRONMENTALREVERB", SSRNX_DATA_ADDR(SSRNX_SL_IID_ENVIRONMENTALREVERB), "libOpenSLES.so"},
    {"SL_IID_ANDROIDSIMPLEBUFFERQUEUE", SSRNX_DATA_ADDR(SSRNX_SL_IID_ANDROIDSIMPLEBUFFERQUEUE), "libOpenSLES.so"},
    {"SL_IID_ANDROIDCONFIGURATION", SSRNX_DATA_ADDR(SSRNX_SL_IID_ANDROIDCONFIGURATION), "libOpenSLES.so"},
    {"SL_IID_RECORD", SSRNX_DATA_ADDR(SSRNX_SL_IID_RECORD), "libOpenSLES.so"},
    {"SL_IID_BUFFERQUEUE", SSRNX_DATA_ADDR(SSRNX_SL_IID_BUFFERQUEUE), "libOpenSLES.so"},
    {"SL_IID_PREFETCHSTATUS", SSRNX_DATA_ADDR(SSRNX_SL_IID_PREFETCHSTATUS), "libOpenSLES.so"},
    {"SL_IID_METADATAEXTRACTION", SSRNX_DATA_ADDR(SSRNX_SL_IID_METADATAEXTRACTION), "libOpenSLES.so"},

    /* Stage 8: Android NDK asset API, provided by virtual libandroid.so. */
    {"AAssetManager_fromJava", SSRNX_FUNC_ADDR(AAssetManager_fromJava), "libandroid.so"},
    {"AAssetManager_open", SSRNX_FUNC_ADDR(AAssetManager_open), "libandroid.so"},
    {"AAssetManager_openDir", SSRNX_FUNC_ADDR(AAssetManager_openDir), "libandroid.so"},
    {"AAssetDir_getNextFileName", SSRNX_FUNC_ADDR(AAssetDir_getNextFileName), "libandroid.so"},
    {"AAssetDir_rewind", SSRNX_FUNC_ADDR(AAssetDir_rewind), "libandroid.so"},
    {"AAssetDir_close", SSRNX_FUNC_ADDR(AAssetDir_close), "libandroid.so"},
    {"AAsset_read", SSRNX_FUNC_ADDR(AAsset_read), "libandroid.so"},
    {"AAsset_seek", SSRNX_FUNC_ADDR(AAsset_seek), "libandroid.so"},
    {"AAsset_seek64", SSRNX_FUNC_ADDR(AAsset_seek64), "libandroid.so"},
    {"AAsset_getLength", SSRNX_FUNC_ADDR(AAsset_getLength), "libandroid.so"},
    {"AAsset_getLength64", SSRNX_FUNC_ADDR(AAsset_getLength64), "libandroid.so"},
    {"AAsset_getRemainingLength", SSRNX_FUNC_ADDR(AAsset_getRemainingLength), "libandroid.so"},
    {"AAsset_getRemainingLength64", SSRNX_FUNC_ADDR(AAsset_getRemainingLength64), "libandroid.so"},
    {"AAsset_getBuffer", SSRNX_FUNC_ADDR(AAsset_getBuffer), "libandroid.so"},
    {"AAsset_openFileDescriptor", SSRNX_FUNC_ADDR(AAsset_openFileDescriptor), "libandroid.so"},
    {"AAsset_openFileDescriptor64", SSRNX_FUNC_ADDR(AAsset_openFileDescriptor64), "libandroid.so"},
    {"AAsset_isAllocated", SSRNX_FUNC_ADDR(AAsset_isAllocated), "libandroid.so"},
    {"AAsset_close", SSRNX_FUNC_ADDR(AAsset_close), "libandroid.so"},
};

bool ssrnx_compat_is_virtual_library(const char *name) {
    return name && (strcmp(name, "libc.so") == 0 ||
                    strcmp(name, "libm.so") == 0 ||
                    /* Stage 14.3: Android's legacy system libstdc++.so is a
                       platform dependency, not an APK-private DSO. Treat it
                       like libc/libm so dependency discovery does not require
                       a physical copy in the extracted app library directory. */
                    strcmp(name, "libstdc++.so") == 0 ||
                    /* Stage 14.4: zlib is another Android platform DSO. APKs
                       commonly DT_NEEDED libz.so without bundling a private
                       copy, so dependency discovery must not require one. */
                    strcmp(name, "libz.so") == 0 ||
                    strcmp(name, "liblog.so") == 0 ||
                    strcmp(name, "libdl.so") == 0 ||
                    strcmp(name, "libandroid.so") == 0 ||
                    strcmp(name, "libjnigraphics.so") == 0 ||
                    /* Android NDK media is a platform DSO. SSRNX recognizes the
                       dependency but intentionally leaves codec/extractor calls
                       unresolved until a faithful backend exists. */
                    strcmp(name, "libmediandk.so") == 0 ||
                    /* Platform dependency; unsupported OpenMAX symbols still fail lookup. */
                    strcmp(name, "libOpenMAXAL.so") == 0 ||
                    strcmp(name, "libOpenSLES.so") == 0 ||
                    strcmp(name, "libaaudio.so") == 0 ||
                    strcmp(name, "libEGL.so") == 0 ||
                    /* Android ships the GLES 1.x DSO on every device; entry
                       points resolve through the same dynamic GLES bridge. */
                    strcmp(name, "libGLESv1_CM.so") == 0 ||
                    strcmp(name, "libGLESv2.so") == 0 ||
                    strcmp(name, "libGLESv3.so") == 0);
}

SsrnxResult ssrnx_compat_resolve(const char *name,
                                 uintptr_t *out_address,
                                 const char **out_provider) {
    if (!name || !out_address) return SSRNX_ERR_ARGUMENT;

    /* Stage 9: EGL wrappers and the dynamic GLES bridge are resolved before
       the fixed compatibility table. */
    SsrnxResult graphics_rc = ssrnx_graphics_resolve(name, out_address, out_provider);
    if (graphics_rc == SSRNX_OK) return SSRNX_OK;

    compat_ssrnx_bionic_init_ctype_table();
    compat_g_bionic_stack_chk_guard_export = ssrnx_bionic_tls_stack_guard();
    for (size_t i = 0; i < sizeof(compat_k_exports) / sizeof(compat_k_exports[0]); ++i) {
        if (strcmp(name, compat_k_exports[i].name) != 0) continue;
        *out_address = compat_k_exports[i].address;
        if (out_provider) *out_provider = compat_k_exports[i].provider;
        return SSRNX_OK;
    }
    return SSRNX_ERR_SYMBOL_NOT_FOUND;
}

size_t ssrnx_compat_export_count(void) {
    return sizeof(compat_k_exports) / sizeof(compat_k_exports[0]) + ssrnx_graphics_export_count();
}



#pragma pop_macro("MM_FOR_EACH_PAGE")
#pragma pop_macro("SSRNX_WX_ZERO_STEP")
#pragma pop_macro("SSRNX_WX_SLOTS")
#pragma pop_macro("SSRNX_WX_GROUP_PAGES")
#pragma pop_macro("SSRNX_TMPFILE_CAP")
#pragma pop_macro("SSRNX_STORAGE_STREAM_MAX")
#pragma pop_macro("SSRNX_STORAGE_PATH_CAP")
#pragma pop_macro("SSRNX_NETWORK_SOCKET_TRACE_MAX")
#pragma pop_macro("SSRNX_MREMAP_MAYMOVE")
#pragma pop_macro("SSRNX_MREMAP_FIXED")
#pragma pop_macro("SSRNX_MREMAP_DONTUNMAP")
#pragma pop_macro("SSRNX_MM_PROT_WRITE")
#pragma pop_macro("SSRNX_MM_PROT_EXEC")
#pragma pop_macro("SSRNX_MM_PAGE")
#pragma pop_macro("SSRNX_MM_MAP_TYPE")
#pragma pop_macro("SSRNX_MM_MAP_SHARED_VALIDATE")
#pragma pop_macro("SSRNX_MM_MAP_SHARED")
#pragma pop_macro("SSRNX_MM_MAP_PRIVATE")
#pragma pop_macro("SSRNX_MM_MAP_FIXED_NOREPLACE")
#pragma pop_macro("SSRNX_MM_MAP_FIXED")
#pragma pop_macro("SSRNX_MM_MAP_ANONYMOUS")
#pragma pop_macro("SSRNX_MM_MADV_REMOVE")
#pragma pop_macro("SSRNX_MM_MADV_DONTNEED")
#pragma pop_macro("SSRNX_MEMTRACE_LARGE_THRESHOLD")
#pragma pop_macro("SSRNX_MEMTRACE_CALLER")
#pragma pop_macro("SSRNX_MALLINFO_COPY")
#pragma pop_macro("SSRNX_GUEST_STDERR_RING_CAP")
#pragma pop_macro("SSRNX_FUNC_ADDR")
#pragma pop_macro("SSRNX_FNV")
#pragma pop_macro("SSRNX_FLOCK_MAX")
#pragma pop_macro("SSRNX_DLOPEN_VIRTUAL_HANDLE")
#pragma pop_macro("SSRNX_DLOPEN_GLOBAL_HANDLE")
#pragma pop_macro("SSRNX_DATA_ADDR")
#pragma pop_macro("SSRNX_CPU_FREQ_KHZ")
#pragma pop_macro("SSRNX_BIONIC_RTLD_NOLOAD")
#pragma pop_macro("SSRNX_BIONIC_RTLD_GLOBAL")
#pragma pop_macro("SSRNX_BIONIC_RANDOM_SEP")
#pragma pop_macro("SSRNX_BIONIC_RANDOM_DEG")
#pragma pop_macro("SSRNX_BIONIC_MUTEXATTR_TYPE_MASK")
#pragma pop_macro("SSRNX_BIONIC_MUTEXATTR_SHARED_MASK")
#pragma pop_macro("SSRNX_BIONIC_MUTEXATTR_PROTOCOL_SHIFT")
#pragma pop_macro("SSRNX_BIONIC_MUTEXATTR_PROTOCOL_MASK")
#pragma pop_macro("SSRNX_ATFORK_MAX")
#pragma pop_macro("SSRNX_ARM64_HWCAP")
#pragma pop_macro("SSRNX_ANDROID_UIO_MAXIOV")
#pragma pop_macro("SSRNX_ANDROID_SIGABRT")
#pragma pop_macro("SSRNX_ANDROID_MSG_WAITFORONE")
#pragma pop_macro("SSRNX_ANDROID_MSG_TRUNC")
#pragma pop_macro("SSRNX_ANDROID_MSG_OOB")
#pragma pop_macro("SSRNX_ANDROID_MSG_EOR")
#pragma pop_macro("SSRNX_ANDROID_MSG_DONTWAIT")
#pragma pop_macro("SSRNX_ANDROID_MSG_CTRUNC")


/* ---- compat_cxa ---- */



#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * Bionic / Itanium C++ ABI process-destructor registry.
 *
 * Android's __cxa_atexit stores (destructor, argument, DSO handle) tuples.
 * __cxa_finalize(dso) runs matching entries in strict LIFO order; a NULL DSO
 * means process exit and drains every remaining entry. Entries are marked
 * consumed before invoking guest code so recursive finalization cannot call a
 * destructor twice. If a destructor registers a new callback, restart from the
 * newest entry to preserve Bionic's ordering semantics.
 *
 * This is intentionally independent of any one APK. Native C++ games and even
 * C binaries produced by Clang may import these symbols through compiler
 * startup/teardown glue.
 */

typedef void (*compat_cxa_SsrnxCxaDestructor)(void *);

typedef struct compat_cxa_SsrnxCxaEntry {
    compat_cxa_SsrnxCxaDestructor fn;
    void *arg;
    void *dso;
    bool active;
} compat_cxa_SsrnxCxaEntry;

typedef struct compat_cxa_SsrnxCxaThreadEntry {
    compat_cxa_SsrnxCxaDestructor fn;
    void *arg;
    void *dso;
    struct compat_cxa_SsrnxCxaThreadEntry *next;
} compat_cxa_SsrnxCxaThreadEntry;

static pthread_once_t compat_cxa_g_cxa_thread_key_once = PTHREAD_ONCE_INIT;
static pthread_key_t compat_cxa_g_cxa_thread_key;
static int compat_cxa_g_cxa_thread_key_status;

static void compat_cxa_cxa_thread_run_dtors(void *opaque) {
    /* POSIX may invoke a key destructor repeatedly when the key remains
     * non-NULL. Clear it first, then consume the Android C++ TLS destructor
     * stack in LIFO order. */
    (void)pthread_setspecific(compat_cxa_g_cxa_thread_key, NULL);
    compat_cxa_SsrnxCxaThreadEntry *entry = (compat_cxa_SsrnxCxaThreadEntry *)opaque;
    while (entry) {
        compat_cxa_SsrnxCxaThreadEntry *next = entry->next;
        compat_cxa_SsrnxCxaDestructor fn = entry->fn;
        void *arg = entry->arg;
        free(entry);
        if (fn) fn(arg);
        entry = next;
    }
}

static void compat_cxa_cxa_thread_key_init(void) {
    compat_cxa_g_cxa_thread_key_status = pthread_key_create(&compat_cxa_g_cxa_thread_key, compat_cxa_cxa_thread_run_dtors);
}

void ssrnx_bionic_cxa_thread_finalize_current(void) {
    pthread_once(&compat_cxa_g_cxa_thread_key_once, compat_cxa_cxa_thread_key_init);
    if (compat_cxa_g_cxa_thread_key_status != 0) return;
    compat_cxa_SsrnxCxaThreadEntry *head = (compat_cxa_SsrnxCxaThreadEntry *)pthread_getspecific(compat_cxa_g_cxa_thread_key);
    if (!head) return;
    /* Bionic explicitly finalizes C++ thread_local destructors before pthread
     * cleanup handlers and pthread-key destructors. Clear the host key first so
     * its fallback destructor cannot run the same list a second time. */
    (void)pthread_setspecific(compat_cxa_g_cxa_thread_key, NULL);
    compat_cxa_cxa_thread_run_dtors(head);
}

int ssrnx_bionic_cxa_thread_atexit_impl(void (*func)(void *), void *arg, void *dso) {
    if (!func) return -1;
    pthread_once(&compat_cxa_g_cxa_thread_key_once, compat_cxa_cxa_thread_key_init);
    if (compat_cxa_g_cxa_thread_key_status != 0) return -1;

    compat_cxa_SsrnxCxaThreadEntry *entry = (compat_cxa_SsrnxCxaThreadEntry *)calloc(1, sizeof(*entry));
    if (!entry) return -1;
    entry->fn = func;
    entry->arg = arg;
    entry->dso = dso;
    if (dso) ssrnx_compat_pin_guest_address(dso);
    entry->next = (compat_cxa_SsrnxCxaThreadEntry *)pthread_getspecific(compat_cxa_g_cxa_thread_key);
    if (pthread_setspecific(compat_cxa_g_cxa_thread_key, entry) != 0) {
        free(entry);
        return -1;
    }
    SSRNX_LOGCI("CXA", "__cxa_thread_atexit_impl registered fn=%p arg=%p dso=%p",
                (void *)(uintptr_t)func, arg, dso);
    return 0;
}


/*
 * Android arm64 uses the Itanium C++ ABI 64-bit guard object. Compiler-emitted
 * fast paths inspect the low byte: zero means initialization may still be
 * needed and one means complete. Keep the in-progress marker outside that byte
 * so guest code never mistakes an initializing object for a completed one.
 *
 * A single process-wide condition variable is sufficient here: the guard word
 * itself identifies the object and every waiter rechecks its own guard after a
 * wake. This preserves one-time construction semantics without depending on a
 * title, engine, or host futex implementation.
 */
enum {
    SSRNX_CXA_GUARD_UNINITIALIZED = 0u,
    SSRNX_CXA_GUARD_COMPLETE = 1u,
    SSRNX_CXA_GUARD_PENDING = 0x100u,
};

static pthread_mutex_t compat_cxa_g_cxa_guard_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t compat_cxa_g_cxa_guard_cond = PTHREAD_COND_INITIALIZER;

int ssrnx_bionic_cxa_guard_acquire(uint64_t *guard) {
    if (!guard) abort();
    uint32_t *state = (uint32_t *)(void *)guard;

    pthread_mutex_lock(&compat_cxa_g_cxa_guard_lock);
    for (;;) {
        const uint32_t current = __atomic_load_n(state, __ATOMIC_ACQUIRE);
        if (current == SSRNX_CXA_GUARD_COMPLETE) {
            pthread_mutex_unlock(&compat_cxa_g_cxa_guard_lock);
            return 0;
        }
        if (current == SSRNX_CXA_GUARD_UNINITIALIZED) {
            __atomic_store_n(state, SSRNX_CXA_GUARD_PENDING, __ATOMIC_RELAXED);
            pthread_mutex_unlock(&compat_cxa_g_cxa_guard_lock);
            return 1;
        }
        pthread_cond_wait(&compat_cxa_g_cxa_guard_cond, &compat_cxa_g_cxa_guard_lock);
    }
}

void ssrnx_bionic_cxa_guard_release(uint64_t *guard) {
    if (!guard) abort();
    uint32_t *state = (uint32_t *)(void *)guard;
    pthread_mutex_lock(&compat_cxa_g_cxa_guard_lock);
    __atomic_store_n(state, SSRNX_CXA_GUARD_COMPLETE, __ATOMIC_RELEASE);
    pthread_cond_broadcast(&compat_cxa_g_cxa_guard_cond);
    pthread_mutex_unlock(&compat_cxa_g_cxa_guard_lock);
}

void ssrnx_bionic_cxa_guard_abort(uint64_t *guard) {
    if (!guard) abort();
    uint32_t *state = (uint32_t *)(void *)guard;
    pthread_mutex_lock(&compat_cxa_g_cxa_guard_lock);
    __atomic_store_n(state, SSRNX_CXA_GUARD_UNINITIALIZED, __ATOMIC_RELEASE);
    pthread_cond_broadcast(&compat_cxa_g_cxa_guard_cond);
    pthread_mutex_unlock(&compat_cxa_g_cxa_guard_lock);
}

void ssrnx_bionic_cxa_pure_virtual(void) {
    SSRNX_LOGCE("CXA", "pure virtual function called");
    abort();
}

static pthread_mutex_t compat_cxa_g_cxa_lock = PTHREAD_MUTEX_INITIALIZER;
static compat_cxa_SsrnxCxaEntry *compat_cxa_g_cxa_entries;
static size_t compat_cxa_g_cxa_count;
static size_t compat_cxa_g_cxa_capacity;
static uint64_t compat_cxa_g_cxa_total_appends;
static unsigned compat_cxa_g_cxa_finalize_depth;

static bool compat_cxa_cxa_grow_locked(void) {
    if (compat_cxa_g_cxa_count < compat_cxa_g_cxa_capacity) return true;
    size_t next = compat_cxa_g_cxa_capacity ? compat_cxa_g_cxa_capacity * 2u : 32u;
    if (next < compat_cxa_g_cxa_capacity || next > SIZE_MAX / sizeof(*compat_cxa_g_cxa_entries)) return false;
    compat_cxa_SsrnxCxaEntry *grown = (compat_cxa_SsrnxCxaEntry *)realloc(compat_cxa_g_cxa_entries, next * sizeof(*grown));
    if (!grown) return false;
    memset(grown + compat_cxa_g_cxa_capacity, 0, (next - compat_cxa_g_cxa_capacity) * sizeof(*grown));
    compat_cxa_g_cxa_entries = grown;
    compat_cxa_g_cxa_capacity = next;
    return true;
}

static void compat_cxa_cxa_compact_locked(void) {
    size_t out = 0;
    for (size_t i = 0; i < compat_cxa_g_cxa_count; ++i) {
        if (!compat_cxa_g_cxa_entries[i].active) continue;
        if (out != i) compat_cxa_g_cxa_entries[out] = compat_cxa_g_cxa_entries[i];
        ++out;
    }
    if (out < compat_cxa_g_cxa_count) {
        memset(compat_cxa_g_cxa_entries + out, 0, (compat_cxa_g_cxa_count - out) * sizeof(*compat_cxa_g_cxa_entries));
        compat_cxa_g_cxa_count = out;
    }
}

int ssrnx_bionic_cxa_atexit(void (*func)(void *), void *arg, void *dso) {
    if (!func) return -1;

    pthread_mutex_lock(&compat_cxa_g_cxa_lock);
    if (!compat_cxa_cxa_grow_locked()) {
        pthread_mutex_unlock(&compat_cxa_g_cxa_lock);
        return -1;
    }

    compat_cxa_SsrnxCxaEntry *entry = &compat_cxa_g_cxa_entries[compat_cxa_g_cxa_count++];
    entry->fn = func;
    entry->arg = arg;
    entry->dso = dso;
    entry->active = true;
    ++compat_cxa_g_cxa_total_appends;
    const size_t pending = compat_cxa_g_cxa_count;
    pthread_mutex_unlock(&compat_cxa_g_cxa_lock);

    SSRNX_LOGCI("CXA", "__cxa_atexit registered fn=%p arg=%p dso=%p pending=%zu",
                (void *)(uintptr_t)func, arg, dso, pending);
    return 0;
}

void ssrnx_bionic_cxa_finalize(void *dso) {
    pthread_mutex_lock(&compat_cxa_g_cxa_lock);
    ++compat_cxa_g_cxa_finalize_depth;

restart:
    {
        const uint64_t observed_appends = compat_cxa_g_cxa_total_appends;
        for (size_t i = compat_cxa_g_cxa_count; i > 0; --i) {
            compat_cxa_SsrnxCxaEntry *slot = &compat_cxa_g_cxa_entries[i - 1];
            if (!slot->active || (dso && slot->dso != dso)) continue;

            /* Consume before calling guest code. This is the key recursive and
             * concurrent-finalization safety property used by Bionic too. */
            const compat_cxa_SsrnxCxaEntry entry = *slot;
            slot->active = false;
            slot->fn = NULL;

            pthread_mutex_unlock(&compat_cxa_g_cxa_lock);
            entry.fn(entry.arg);
            pthread_mutex_lock(&compat_cxa_g_cxa_lock);

            if (compat_cxa_g_cxa_total_appends != observed_appends) goto restart;
        }
    }

    if (compat_cxa_g_cxa_finalize_depth > 0) --compat_cxa_g_cxa_finalize_depth;
    if (compat_cxa_g_cxa_finalize_depth == 0 && dso != NULL) compat_cxa_cxa_compact_locked();
    pthread_mutex_unlock(&compat_cxa_g_cxa_lock);
}

size_t ssrnx_bionic_cxa_pending_count(void) {
    pthread_mutex_lock(&compat_cxa_g_cxa_lock);
    size_t pending = 0;
    for (size_t i = 0; i < compat_cxa_g_cxa_count; ++i) pending += compat_cxa_g_cxa_entries[i].active ? 1u : 0u;
    pthread_mutex_unlock(&compat_cxa_g_cxa_lock);
    return pending;
}

void ssrnx_bionic_cxa_reset(void) {
    pthread_mutex_lock(&compat_cxa_g_cxa_lock);
    free(compat_cxa_g_cxa_entries);
    compat_cxa_g_cxa_entries = NULL;
    compat_cxa_g_cxa_count = 0;
    compat_cxa_g_cxa_capacity = 0;
    compat_cxa_g_cxa_total_appends = 0;
    compat_cxa_g_cxa_finalize_depth = 0;
    pthread_mutex_unlock(&compat_cxa_g_cxa_lock);
}



/* ---- compat_threads ---- */
#pragma push_macro("SSRNX_ANDROID_APP_UID")
#pragma push_macro("SSRNX_ANDROID_MINSIGSTKSZ")
#pragma push_macro("SSRNX_ANDROID_NICE_MAX")
#pragma push_macro("SSRNX_ANDROID_NICE_MIN")
#pragma push_macro("SSRNX_ANDROID_PRIO_PGRP")
#pragma push_macro("SSRNX_ANDROID_PRIO_PROCESS")
#pragma push_macro("SSRNX_ANDROID_PRIO_USER")
#pragma push_macro("SSRNX_ANDROID_SA_NODEFER")
#pragma push_macro("SSRNX_ANDROID_SA_ONSTACK")
#pragma push_macro("SSRNX_ANDROID_SA_RESETHAND")
#pragma push_macro("SSRNX_ANDROID_SA_SIGINFO")
#pragma push_macro("SSRNX_ANDROID_SIGKILL")
#pragma push_macro("SSRNX_ANDROID_SIGSTOP")
#pragma push_macro("SSRNX_ANDROID_SIG_BLOCK")
#pragma push_macro("SSRNX_ANDROID_SIG_SETMASK")
#pragma push_macro("SSRNX_ANDROID_SIG_UNBLOCK")
#pragma push_macro("SSRNX_ANDROID_SI_TKILL")
#pragma push_macro("SSRNX_ANDROID_SS_DISABLE")
#pragma push_macro("SSRNX_ANDROID_SS_ONSTACK")
#pragma push_macro("SSRNX_BIONIC_CONDATTR_DESTROYED")
#pragma push_macro("SSRNX_BIONIC_COND_CLOCK_MASK")
#pragma push_macro("SSRNX_BIONIC_COND_SHARED_MASK")
#pragma push_macro("SSRNX_BIONIC_KEY_SEQ_IN_USE")
#pragma push_macro("SSRNX_BIONIC_KEY_VALID_FLAG")
#pragma push_macro("SSRNX_BIONIC_PTHREAD_DESTRUCTOR_ITERATIONS")
#pragma push_macro("SSRNX_BIONIC_PTHREAD_KEYS_MAX")
#pragma push_macro("SSRNX_BIONIC_RWLOCKATTR_KIND_MASK")
#pragma push_macro("SSRNX_BIONIC_RWLOCKATTR_PSHARED_MASK")
#pragma push_macro("SSRNX_BIONIC_RWLOCKATTR_RESERVED")
#pragma push_macro("SSRNX_BIONIC_SEM_VALUE_MAX")
#pragma push_macro("SSRNX_BIONIC_STATIC_TLS_CAPACITY")
#pragma push_macro("SSRNX_BIONIC_TLS_BASE_BYTES")
#pragma push_macro("SSRNX_BIONIC_TLS_SLOTS")
#pragma push_macro("SSRNX_BIONIC_TLS_SLOT_SELF")
#pragma push_macro("SSRNX_BIONIC_TLS_SLOT_STACK_GUARD")
#pragma push_macro("SSRNX_BIONIC_TLS_SLOT_THREAD_ID")
#pragma push_macro("SSRNX_ERRNO")
#pragma push_macro("SSRNX_FUTEX_BITSET_MATCH_ANY")
#pragma push_macro("SSRNX_FUTEX_BUCKETS")
#pragma push_macro("SSRNX_GUEST_SEM_MAGIC")
#pragma push_macro("SSRNX_HOST_ERRNO_IMPORTED")
#pragma push_macro("SSRNX_LINUX_FPSIMD_MAGIC")
#pragma push_macro("COMPAT_SYNC_BUCKETS")
#pragma push_macro("GUEST_WAIT_SITE")
#pragma push_macro("PTHREAD_SETUP_FAIL")
#pragma push_macro("SIGNAL_PROXY_STACK")
#pragma push_macro("SIGNAL_RED_ZONE")
#pragma push_macro("SIGNAL_SAFE_GAP")
#pragma push_macro("SIGNAL_WAIT_SLICE_NS")
#pragma push_macro("SIG_BIT")
#pragma push_macro("SIG_STATE_ASSIGNED")
#pragma push_macro("SIG_STATE_DELIVERING")
#pragma push_macro("SIG_STATE_EXITING")
#pragma push_macro("SIG_STATE_KICK")
#pragma push_macro("SIG_STATE_RETURNING")
#pragma push_macro("SIG_STATE_SAFE")
#pragma push_macro("SIG_STATE_SUSPENDED")
#pragma push_macro("SIG_STATE_TARGET_KICK")
#pragma push_macro("SIG_UNBLOCKABLE")
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif






#include <errno.h>
#include <limits.h>
#include <malloc.h>
#include <pthread.h>
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef __SWITCH__
#include <switch.h>
#else
#include <signal.h>
#include <ucontext.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#endif

/* Optional: resolved when compat.c (the guest dladdr view) and the SSRNX
 * logger are linked in; host regressions may omit either. Declared weak
 * before first use so every call site can null-check. */
bool ssrnx_compat_describe_guest_address(uintptr_t address, char *out, size_t cap)
    __attribute__((weak));
void ssrnx_log_category(const char *category, const char *level, const char *fmt, ...)
    __attribute__((weak));
bool ssrnx_diag_full_trace_enabled(void) __attribute__((weak));
void ssrnx_bionic_cxa_thread_finalize_current(void) __attribute__((weak));
size_t ssrnx_android_reap_exited_java_threads(void) __attribute__((weak));

/*
 * errno ABI boundary. Linux asm-generic/errno*.h numbering (Bionic uses the
 * kernel's) keyed by the host's symbolic constants, so on a Linux host every
 * entry is the identity and on newlib each one renumbers. 1..34 agree on all
 * hosts and need no entry. A table (not a switch) tolerates hosts where two
 * names share a value (glibc ENOTSUP == EOPNOTSUPP, EDEADLOCK == EDEADLK).
 */
typedef struct { int host; int android; } compat_threads_SsrnxErrnoMap;
static const compat_threads_SsrnxErrnoMap compat_threads_k_errno_map[] = {
#define SSRNX_ERRNO(name, value) { name, value },
#ifdef EDEADLK
    SSRNX_ERRNO(EDEADLK, 35)
#endif
#ifdef ENAMETOOLONG
    SSRNX_ERRNO(ENAMETOOLONG, 36)
#endif
#ifdef ENOLCK
    SSRNX_ERRNO(ENOLCK, 37)
#endif
#ifdef ENOSYS
    SSRNX_ERRNO(ENOSYS, 38)
#endif
#ifdef ENOTEMPTY
    SSRNX_ERRNO(ENOTEMPTY, 39)
#endif
#ifdef ELOOP
    SSRNX_ERRNO(ELOOP, 40)
#endif
#ifdef ENOMSG
    SSRNX_ERRNO(ENOMSG, 42)
#endif
#ifdef EIDRM
    SSRNX_ERRNO(EIDRM, 43)
#endif
#ifdef ECHRNG
    SSRNX_ERRNO(ECHRNG, 44)
#endif
#ifdef EL2NSYNC
    SSRNX_ERRNO(EL2NSYNC, 45)
#endif
#ifdef EL3HLT
    SSRNX_ERRNO(EL3HLT, 46)
#endif
#ifdef EL3RST
    SSRNX_ERRNO(EL3RST, 47)
#endif
#ifdef ELNRNG
    SSRNX_ERRNO(ELNRNG, 48)
#endif
#ifdef EUNATCH
    SSRNX_ERRNO(EUNATCH, 49)
#endif
#ifdef ENOCSI
    SSRNX_ERRNO(ENOCSI, 50)
#endif
#ifdef EL2HLT
    SSRNX_ERRNO(EL2HLT, 51)
#endif
#ifdef EBADE
    SSRNX_ERRNO(EBADE, 52)
#endif
#ifdef EBADR
    SSRNX_ERRNO(EBADR, 53)
#endif
#ifdef EXFULL
    SSRNX_ERRNO(EXFULL, 54)
#endif
#ifdef ENOANO
    SSRNX_ERRNO(ENOANO, 55)
#endif
#ifdef EBADRQC
    SSRNX_ERRNO(EBADRQC, 56)
#endif
#ifdef EBADSLT
    SSRNX_ERRNO(EBADSLT, 57)
#endif
#ifdef EBFONT
    SSRNX_ERRNO(EBFONT, 59)
#endif
#ifdef ENOSTR
    SSRNX_ERRNO(ENOSTR, 60)
#endif
#ifdef ENODATA
    SSRNX_ERRNO(ENODATA, 61)
#endif
#ifdef ETIME
    SSRNX_ERRNO(ETIME, 62)
#endif
#ifdef ENOSR
    SSRNX_ERRNO(ENOSR, 63)
#endif
#ifdef ENONET
    SSRNX_ERRNO(ENONET, 64)
#endif
#ifdef ENOPKG
    SSRNX_ERRNO(ENOPKG, 65)
#endif
#ifdef EREMOTE
    SSRNX_ERRNO(EREMOTE, 66)
#endif
#ifdef ENOLINK
    SSRNX_ERRNO(ENOLINK, 67)
#endif
#ifdef EADV
    SSRNX_ERRNO(EADV, 68)
#endif
#ifdef ESRMNT
    SSRNX_ERRNO(ESRMNT, 69)
#endif
#ifdef ECOMM
    SSRNX_ERRNO(ECOMM, 70)
#endif
#ifdef EPROTO
    SSRNX_ERRNO(EPROTO, 71)
#endif
#ifdef EMULTIHOP
    SSRNX_ERRNO(EMULTIHOP, 72)
#endif
#ifdef EDOTDOT
    SSRNX_ERRNO(EDOTDOT, 73)
#endif
#ifdef EBADMSG
    SSRNX_ERRNO(EBADMSG, 74)
#endif
#ifdef EOVERFLOW
    SSRNX_ERRNO(EOVERFLOW, 75)
#endif
#ifdef ENOTUNIQ
    SSRNX_ERRNO(ENOTUNIQ, 76)
#endif
#ifdef EBADFD
    SSRNX_ERRNO(EBADFD, 77)
#endif
#ifdef EREMCHG
    SSRNX_ERRNO(EREMCHG, 78)
#endif
#ifdef ELIBACC
    SSRNX_ERRNO(ELIBACC, 79)
#endif
#ifdef ELIBBAD
    SSRNX_ERRNO(ELIBBAD, 80)
#endif
#ifdef ELIBSCN
    SSRNX_ERRNO(ELIBSCN, 81)
#endif
#ifdef ELIBMAX
    SSRNX_ERRNO(ELIBMAX, 82)
#endif
#ifdef ELIBEXEC
    SSRNX_ERRNO(ELIBEXEC, 83)
#endif
#ifdef EILSEQ
    SSRNX_ERRNO(EILSEQ, 84)
#endif
#ifdef ERESTART
    SSRNX_ERRNO(ERESTART, 85)
#endif
#ifdef ESTRPIPE
    SSRNX_ERRNO(ESTRPIPE, 86)
#endif
#ifdef EUSERS
    SSRNX_ERRNO(EUSERS, 87)
#endif
#ifdef ENOTSOCK
    SSRNX_ERRNO(ENOTSOCK, 88)
#endif
#ifdef EDESTADDRREQ
    SSRNX_ERRNO(EDESTADDRREQ, 89)
#endif
#ifdef EMSGSIZE
    SSRNX_ERRNO(EMSGSIZE, 90)
#endif
#ifdef EPROTOTYPE
    SSRNX_ERRNO(EPROTOTYPE, 91)
#endif
#ifdef ENOPROTOOPT
    SSRNX_ERRNO(ENOPROTOOPT, 92)
#endif
#ifdef EPROTONOSUPPORT
    SSRNX_ERRNO(EPROTONOSUPPORT, 93)
#endif
#ifdef ESOCKTNOSUPPORT
    SSRNX_ERRNO(ESOCKTNOSUPPORT, 94)
#endif
#ifdef EOPNOTSUPP
    SSRNX_ERRNO(EOPNOTSUPP, 95)
#endif
#ifdef ENOTSUP
    SSRNX_ERRNO(ENOTSUP, 95) /* Linux: ENOTSUP is EOPNOTSUPP */
#endif
#ifdef EPFNOSUPPORT
    SSRNX_ERRNO(EPFNOSUPPORT, 96)
#endif
#ifdef EAFNOSUPPORT
    SSRNX_ERRNO(EAFNOSUPPORT, 97)
#endif
#ifdef EADDRINUSE
    SSRNX_ERRNO(EADDRINUSE, 98)
#endif
#ifdef EADDRNOTAVAIL
    SSRNX_ERRNO(EADDRNOTAVAIL, 99)
#endif
#ifdef ENETDOWN
    SSRNX_ERRNO(ENETDOWN, 100)
#endif
#ifdef ENETUNREACH
    SSRNX_ERRNO(ENETUNREACH, 101)
#endif
#ifdef ENETRESET
    SSRNX_ERRNO(ENETRESET, 102)
#endif
#ifdef ECONNABORTED
    SSRNX_ERRNO(ECONNABORTED, 103)
#endif
#ifdef ECONNRESET
    SSRNX_ERRNO(ECONNRESET, 104)
#endif
#ifdef ENOBUFS
    SSRNX_ERRNO(ENOBUFS, 105)
#endif
#ifdef EISCONN
    SSRNX_ERRNO(EISCONN, 106)
#endif
#ifdef ENOTCONN
    SSRNX_ERRNO(ENOTCONN, 107)
#endif
#ifdef ESHUTDOWN
    SSRNX_ERRNO(ESHUTDOWN, 108)
#endif
#ifdef ETOOMANYREFS
    SSRNX_ERRNO(ETOOMANYREFS, 109)
#endif
#ifdef ETIMEDOUT
    SSRNX_ERRNO(ETIMEDOUT, 110)
#endif
#ifdef ECONNREFUSED
    SSRNX_ERRNO(ECONNREFUSED, 111)
#endif
#ifdef EHOSTDOWN
    SSRNX_ERRNO(EHOSTDOWN, 112)
#endif
#ifdef EHOSTUNREACH
    SSRNX_ERRNO(EHOSTUNREACH, 113)
#endif
#ifdef EALREADY
    SSRNX_ERRNO(EALREADY, 114)
#endif
#ifdef EINPROGRESS
    SSRNX_ERRNO(EINPROGRESS, 115)
#endif
#ifdef ESTALE
    SSRNX_ERRNO(ESTALE, 116)
#endif
#ifdef EUCLEAN
    SSRNX_ERRNO(EUCLEAN, 117)
#endif
#ifdef ENOTNAM
    SSRNX_ERRNO(ENOTNAM, 118)
#endif
#ifdef ENAVAIL
    SSRNX_ERRNO(ENAVAIL, 119)
#endif
#ifdef EISNAM
    SSRNX_ERRNO(EISNAM, 120)
#endif
#ifdef EREMOTEIO
    SSRNX_ERRNO(EREMOTEIO, 121)
#endif
#ifdef EDQUOT
    SSRNX_ERRNO(EDQUOT, 122)
#endif
#ifdef ENOMEDIUM
    SSRNX_ERRNO(ENOMEDIUM, 123)
#endif
#ifdef EMEDIUMTYPE
    SSRNX_ERRNO(EMEDIUMTYPE, 124)
#endif
#ifdef ECANCELED
    SSRNX_ERRNO(ECANCELED, 125)
#endif
#ifdef ENOKEY
    SSRNX_ERRNO(ENOKEY, 126)
#endif
#ifdef EKEYEXPIRED
    SSRNX_ERRNO(EKEYEXPIRED, 127)
#endif
#ifdef EKEYREVOKED
    SSRNX_ERRNO(EKEYREVOKED, 128)
#endif
#ifdef EKEYREJECTED
    SSRNX_ERRNO(EKEYREJECTED, 129)
#endif
#ifdef EOWNERDEAD
    SSRNX_ERRNO(EOWNERDEAD, 130)
#endif
#ifdef ENOTRECOVERABLE
    SSRNX_ERRNO(ENOTRECOVERABLE, 131)
#endif
#ifdef ERFKILL
    SSRNX_ERRNO(ERFKILL, 132)
#endif
#ifdef EHWPOISON
    SSRNX_ERRNO(EHWPOISON, 133)
#endif
    /* newlib-only codes with no Linux number: nearest Linux meaning. */
#if defined(EFTYPE) && !defined(__linux__)
    SSRNX_ERRNO(EFTYPE, 22)      /* inappropriate file type -> EINVAL */
#endif
#if defined(ENMFILE) && !defined(__linux__)
    SSRNX_ERRNO(ENMFILE, 2)      /* no more files -> ENOENT */
#endif
#undef SSRNX_ERRNO
};

int ssrnx_errno_host_to_android(int host_value) {
    if (host_value <= 34) return host_value;
    for (size_t i = 0; i < sizeof(compat_threads_k_errno_map) / sizeof(compat_threads_k_errno_map[0]); i++)
        if (compat_threads_k_errno_map[i].host == host_value) return compat_threads_k_errno_map[i].android;
    return host_value;
}

int ssrnx_errno_android_to_host(int android_value) {
    if (android_value <= 34) return android_value;
    for (size_t i = 0; i < sizeof(compat_threads_k_errno_map) / sizeof(compat_threads_k_errno_map[0]); i++)
        if (compat_threads_k_errno_map[i].android == android_value) return compat_threads_k_errno_map[i].host;
    return android_value;
}

/* Host errno value meaning "already imported into the guest slot". No libc
 * reports a negative errno, so it never aliases a real error. */
#define SSRNX_HOST_ERRNO_IMPORTED (-0x41504b)
static _Thread_local int compat_threads_t_guest_errno;

int *ssrnx_guest_errno_location(void) {
    const int host = errno;
    if (host != SSRNX_HOST_ERRNO_IMPORTED) {
        /* A host errno of 0 carries no error; POSIX forbids clearing errno on
         * success, so the guest value is kept. */
        if (host != 0) compat_threads_t_guest_errno = ssrnx_errno_host_to_android(host);
        errno = SSRNX_HOST_ERRNO_IMPORTED;
    }
    return &compat_threads_t_guest_errno;
}

void ssrnx_guest_errno_publish(void) {
    (void)ssrnx_guest_errno_location();
}

/*
 * Stage 5 Bionic pthread/TLS translation.
 *
 * Never cast an Android pthread object to the host pthread type. Bionic's LP64
 * public ABI deliberately exposes opaque fixed-size storage while libnx/newlib
 * uses different layouts. SSRNX keys sidecar host objects by the guest object
 * address instead. This also handles Android static initializers (all-zero for
 * the normal mutex/cond/rwlock cases) without modifying the guest ABI.
 */

typedef struct compat_threads_CompatMutex {
    void *guest;
    pthread_mutex_t host;
    int type;
    int protocol; /* Android PTHREAD_PRIO_NONE(0) / PTHREAD_PRIO_INHERIT(1). */
    uint32_t state; /* Non-PI: 0 unlocked, 1 locked, 2 contended. */
    uint64_t acquirer; /* Ownership for checked types; diagnostic only for normal. */
    uint32_t depth;
    struct compat_threads_CompatMutex *next;
} compat_threads_CompatMutex;

typedef struct compat_threads_CompatCond {
    void *guest;
    pthread_cond_t host;
    int guest_clock_id; /* Android CLOCK_REALTIME(0) or CLOCK_MONOTONIC(1). */
    uint32_t normal_sequence;
    uint64_t wait_count, wake_count, timeout_count, signal_count, broadcast_count;
    unsigned waiters;
    int last_host_rc;
    struct compat_threads_CompatCond *next;
} compat_threads_CompatCond;

typedef struct compat_threads_CompatRwlock {
    void *guest;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    unsigned readers;
    unsigned waiting_writers;
    int writer;
    int writer_nonrecursive_preferred;
    SsrnxGuestPthread writer_owner;
    struct compat_threads_CompatRwlock *next;
} compat_threads_CompatRwlock;

typedef struct compat_threads_CompatBarrier {
    void *guest;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    unsigned init_count;
    unsigned waiting;
    unsigned generation;
    struct compat_threads_CompatBarrier *next;
} compat_threads_CompatBarrier;

#define SSRNX_BIONIC_SEM_VALUE_MAX UINT32_C(0x3fffffff)

/* Android arm64 stack_t. */
typedef struct compat_threads_SsrnxSigStack {
    uintptr_t ss_sp;
    int32_t ss_flags;
    int32_t _pad_flags;
    uint64_t ss_size;
} compat_threads_SsrnxSigStack;

enum compat_threads_HostThreadJoinState {
    HOST_THREAD_CREATING, HOST_THREAD_JOINABLE, HOST_THREAD_REAPING,
    HOST_THREAD_REAPED, HOST_THREAD_GUEST_JOINING
};

typedef struct compat_threads_CompatThread {
    SsrnxGuestPthread guest_id;
    int android_tid;
    pthread_t host;
    void *(*guest_start)(void *);
    void *guest_arg;
    void *result;
    void *stack_base;
    size_t stack_size;
    size_t guard_size;
    int stack_ready;
    int stack_capture_rc;
    int detached;
    int exited;
    enum compat_threads_HostThreadJoinState host_join_state; /* registry lock; guest result outlives host */
    int external;
    int nice_value;
    int sched_policy;
    int sched_priority;
    uint32_t attr_flags;
    char name[16];
#ifdef __SWITCH__
    int host_base_priority;
    int host_base_priority_valid;
    Handle host_handle;
#else
    /* Host-regression pause handshake (see signal_pause). */
    uint32_t host_paused;
    uintptr_t host_pause_pc;
    uintptr_t host_pause_sp;
#endif
    /* Linux signal state. Written by the thread itself or by the proxy that
     * acts for it while it is stopped; senders only set pending bits. */
    uint64_t sig_blocked;
    uint64_t sig_pending;
    uint32_t sig_wake;       /* address word: bumped on new pending/delivery */
    uint32_t sig_done;       /* address word: bumped when a proxy finishes */
    uint32_t sig_state;      /* SIG_STATE_* */
    struct compat_threads_CompatThread *sig_job_next; /* queued for a busy proxy (ASSIGNED) */
    uint32_t sig_delivered;  /* handlers run for this thread */
    uint32_t sig_on_altstack;
    compat_threads_SsrnxSigStack sig_altstack;
    uintptr_t sig_safe_sp;   /* stack pointer inside the current safe point */
    uintptr_t sig_safe_tp;   /* TPIDR_EL0 at the current safe point */
    /* This thread's per-thread guest tables, mirrored from their host keys so
     * a signal proxy can present them while acting as this thread. */
    void *key_table;
    void *emutls_array;
    SsrnxBionicPthreadCleanup *cleanup_stack;
    /* Diagnostics: the blocking primitive this thread is inside, the guest
     * call site that entered it, and when the wait began (0 = not waiting). */
    const char *wait_kind;
    const void *wait_object;
    uintptr_t wait_caller;
    uint64_t wait_since_ms;
    uintptr_t futex_frames[24]; /* includes the native/JNI caller above nested wait wrappers */
    /* Diagnostics: the runtime call this thread is running, published by the
     * thread itself (activity_since_ms 0 = none). A thread blocked on a host
     * lock never enters a Bionic wait, so this is its only location. */
    char activity[SSRNX_THREAD_ACTIVITY_TEXT];
    uint64_t activity_since_ms;
    struct compat_threads_CompatThread *next;
} compat_threads_CompatThread;

/*
 * Bionic pthread keys (libc/bionic/pthread_key.cpp ABI).
 *
 * Android guarantees PTHREAD_KEYS_MAX (128) keys per process. Horizon/libnx
 * backs host pthread keys with a few dozen fixed TLS slots shared with SSRNX
 * itself, so guest keys must not consume one host key each: an engine that
 * creates a key per thread-local subsystem exhausts the host long before
 * Android's limit. All guest key values of a thread therefore live in one
 * per-thread table reached through a single host key.
 *
 * Keys are encoded as Bionic does: KEY_VALID_FLAG | slot. Each slot carries a
 * sequence number whose bit 0 means "in use"; create and delete both bump it,
 * so a value stored under a deleted key is never returned for a key that later
 * reuses the slot.
 */
#define SSRNX_BIONIC_PTHREAD_KEYS_MAX 128
#define SSRNX_BIONIC_PTHREAD_DESTRUCTOR_ITERATIONS 4
#define SSRNX_BIONIC_KEY_VALID_FLAG (UINT32_C(1) << 31)
#define SSRNX_BIONIC_KEY_SEQ_IN_USE ((uintptr_t)1)

typedef struct compat_threads_CompatKey {
    uintptr_t seq;
    void (*guest_destructor)(void *);
} compat_threads_CompatKey;

typedef struct compat_threads_CompatKeyData {
    uintptr_t seq;
    void *value;
} compat_threads_CompatKeyData;

typedef struct compat_threads_CompatKeyTable {
    compat_threads_CompatKeyData data[SSRNX_BIONIC_PTHREAD_KEYS_MAX];
} compat_threads_CompatKeyTable;

typedef struct compat_threads_SsrnxEmutlsControl {
    uintptr_t size;
    uintptr_t align;
    union {
        uintptr_t index;
        void *address;
    } object;
    void *value;
} compat_threads_SsrnxEmutlsControl;

typedef struct compat_threads_EmutlsArray {
    size_t capacity;
    void **objects;
} compat_threads_EmutlsArray;

static pthread_mutex_t compat_threads_g_registry_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t compat_threads_g_once_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t compat_threads_g_thread_state_cond = PTHREAD_COND_INITIALIZER;
/* Guest mutex/cond objects are keyed by guest address. Every guest
 * pthread_mutex_lock/unlock and cond operation resolves its host object, so
 * the lookup is hashed: a single list grows with every object an engine ever
 * initializes and turns g_registry_lock into a process-wide hotspot. */
#define COMPAT_SYNC_BUCKETS 4096u
static compat_threads_CompatMutex *compat_threads_g_mutexes[COMPAT_SYNC_BUCKETS];
static compat_threads_CompatCond *compat_threads_g_conds[COMPAT_SYNC_BUCKETS];
static inline size_t compat_threads_sync_bucket(const void *guest) {
    uint64_t x = (uint64_t)(uintptr_t)guest;
    x ^= x >> 33;
    x *= UINT64_C(0xff51afd7ed558ccd);
    x ^= x >> 33;
    return (size_t)(x & (COMPAT_SYNC_BUCKETS - 1u));
}
/* Detached threads that exited and await host join (reap_detached_threads).
 * Updated under g_registry_lock; read lock-free so the common case of nothing
 * to reap costs no registry lock on the guest mutex/create/join paths. */
static size_t compat_threads_g_reapable_threads;
static compat_threads_CompatRwlock *compat_threads_g_rwlocks;
static compat_threads_CompatBarrier *compat_threads_g_barriers;
static compat_threads_CompatThread *compat_threads_g_threads;
static SsrnxGuestPthread compat_threads_g_next_thread_id = 1;
/* Android/Linux TIDs are process-local integer task identities. Keep them
 * independent from both guest pthread_t handles and host pthread_t/Handle
 * representation so Horizon/newlib details never leak into the guest ABI. */
static int compat_threads_g_next_android_tid = 1000;

static compat_threads_CompatKey compat_threads_g_keys[SSRNX_BIONIC_PTHREAD_KEYS_MAX];

static pthread_once_t compat_threads_g_internal_keys_once = PTHREAD_ONCE_INIT;
static bool compat_threads_g_internal_keys_initialized;
static pthread_key_t compat_threads_g_current_thread_key;
static pthread_key_t compat_threads_g_emutls_array_key;
static pthread_key_t compat_threads_g_guest_key_table_key;
static bool compat_threads_g_guest_key_table_ready;
static uintptr_t compat_threads_g_next_emutls_index = 1;

/*
 * Android arm64 Bionic TLS compatibility.
 *
 * TPIDR_EL0 is guest-owned while Android code is running. SSRNX keeps Bionic's
 * fixed slots at TP+0 and reserves a process-wide static-TLS arena after them.
 * Each PT_TLS DSO receives one module ID and one aligned TP-relative offset.
 * Per-thread arenas are initialized from the relocated PT_TLS templates before
 * guest execution. This covers AArch64 IE/LE-style TPREL and TLSDESC accesses
 * without borrowing Horizon/libnx's TPIDRRO_EL0 TLS state.
 */
#define SSRNX_BIONIC_TLS_SLOTS 64u
#define SSRNX_BIONIC_TLS_SLOT_SELF 0u
#define SSRNX_BIONIC_TLS_SLOT_THREAD_ID 1u
#define SSRNX_BIONIC_TLS_SLOT_STACK_GUARD 5u
#define SSRNX_BIONIC_STATIC_TLS_CAPACITY (256u * 1024u)
#define SSRNX_BIONIC_TLS_BASE_BYTES (SSRNX_BIONIC_TLS_SLOTS * sizeof(uintptr_t))

typedef struct compat_threads_SsrnxBionicTlsThread {
    uint8_t *arena;
    size_t initialized_modules;
} compat_threads_SsrnxBionicTlsThread;

typedef struct compat_threads_SsrnxBionicTlsRegistryEntry {
    SsrnxModule *module;
} compat_threads_SsrnxBionicTlsRegistryEntry;

static pthread_once_t compat_threads_g_android_stack_guard_once = PTHREAD_ONCE_INIT;
static uintptr_t compat_threads_g_android_stack_guard;
static pthread_once_t compat_threads_g_android_tls_key_once = PTHREAD_ONCE_INIT;
static pthread_key_t compat_threads_g_android_tls_key;
static pthread_mutex_t compat_threads_g_android_tls_registry_lock = PTHREAD_MUTEX_INITIALIZER;
static compat_threads_SsrnxBionicTlsRegistryEntry *compat_threads_g_android_tls_modules;
static size_t compat_threads_g_android_tls_module_count;
static size_t compat_threads_g_android_tls_module_capacity;
static size_t compat_threads_g_android_tls_next_offset = SSRNX_BIONIC_TLS_BASE_BYTES;
static size_t compat_threads_g_android_tls_generation = 1;

static void compat_threads_android_stack_guard_init(void) {
    struct timespec ts = {0, 0};
    (void)clock_gettime(CLOCK_MONOTONIC, &ts);

    uintptr_t x = (uintptr_t)&compat_threads_g_android_stack_guard;
    x ^= (uintptr_t)ts.tv_sec * UINT64_C(0x9e3779b97f4a7c15);
    x ^= (uintptr_t)ts.tv_nsec * UINT64_C(0xbf58476d1ce4e5b9);
    x ^= UINT64_C(0x41504b3634534755); /* "SSRNXSGU" */
    x ^= x >> 30;
    x *= UINT64_C(0xbf58476d1ce4e5b9);
    x ^= x >> 27;
    x *= UINT64_C(0x94d049bb133111eb);
    x ^= x >> 31;
    if (x == 0) x = UINT64_C(0xa64a64a64a64a64a);
    compat_threads_g_android_stack_guard = x;
}

static uintptr_t compat_threads_android_read_tpidr_el0(void) {
#if defined(__aarch64__)
    uintptr_t value;
    __asm__ volatile("mrs %0, tpidr_el0" : "=r"(value));
    return value;
#else
    compat_threads_SsrnxBionicTlsThread *tls = (compat_threads_SsrnxBionicTlsThread *)pthread_getspecific(compat_threads_g_android_tls_key);
    return tls ? (uintptr_t)tls->arena : 0;
#endif
}

static void compat_threads_android_write_tpidr_el0(uintptr_t value) {
#if defined(__aarch64__)
    __asm__ volatile("msr tpidr_el0, %0\n\tisb" : : "r"(value) : "memory");
#else
    (void)value;
#endif
}

static void compat_threads_android_tls_thread_destroy(void *ptr) {
    compat_threads_SsrnxBionicTlsThread *tls = (compat_threads_SsrnxBionicTlsThread *)ptr;
    if (!tls) return;
    free(tls->arena);
    free(tls);
}

static void compat_threads_android_tls_key_init(void) {
    (void)pthread_key_create(&compat_threads_g_android_tls_key, compat_threads_android_tls_thread_destroy);
}

static compat_threads_SsrnxBionicTlsThread *compat_threads_android_tls_thread_get(bool create) {
    (void)pthread_once(&compat_threads_g_android_tls_key_once, compat_threads_android_tls_key_init);
    compat_threads_SsrnxBionicTlsThread *tls = (compat_threads_SsrnxBionicTlsThread *)pthread_getspecific(compat_threads_g_android_tls_key);
    if (tls || !create) return tls;

    tls = (compat_threads_SsrnxBionicTlsThread *)calloc(1, sizeof(*tls));
    if (!tls) return NULL;
    /* devkitA64/newlib does not export posix_memalign().  Use the host
     * allocator primitive that is already used by SSRNX's Bionic malloc
     * bridge; memalign() storage is compatible with free() on both newlib
     * and the Linux host used by the regression suite.  This is host-side
     * TLS backing allocation, not the guest posix_memalign ABI. */
    void *arena = memalign(SSRNX_BIONIC_PAGE_SIZE, SSRNX_BIONIC_STATIC_TLS_CAPACITY);
    if (!arena) {
        free(tls);
        return NULL;
    }
    memset(arena, 0, SSRNX_BIONIC_STATIC_TLS_CAPACITY);
    tls->arena = (uint8_t *)arena;
    if (pthread_setspecific(compat_threads_g_android_tls_key, tls) != 0) {
        free(arena);
        free(tls);
        return NULL;
    }
    return tls;
}

static size_t compat_threads_tls_align_up(size_t value, size_t align) {
    if (align <= 1u) return value;
    return (value + align - 1u) & ~(align - 1u);
}

SsrnxResult ssrnx_bionic_tls_register_module(SsrnxModule *module) {
    if (!module) return SSRNX_ERR_ARGUMENT;
    if (!module->tls.present) return SSRNX_OK;

    SSRNX_PERF_LOCK(&compat_threads_g_android_tls_registry_lock);
    if (module->tls.registered) {
        pthread_mutex_unlock(&compat_threads_g_android_tls_registry_lock);
        return SSRNX_OK;
    }

    size_t align = module->tls.alignment ? module->tls.alignment : 1u;
    if ((align & (align - 1u)) != 0) {
        pthread_mutex_unlock(&compat_threads_g_android_tls_registry_lock);
        return SSRNX_ERR_DYNAMIC_INVALID;
    }
    /* Bionic caps DSO TLS alignment at a page. Keep the same practical bound. */
    if (align > SSRNX_BIONIC_PAGE_SIZE) align = SSRNX_BIONIC_PAGE_SIZE;
    const size_t offset = compat_threads_tls_align_up(compat_threads_g_android_tls_next_offset, align);
    if (offset > SSRNX_BIONIC_STATIC_TLS_CAPACITY ||
        module->tls.mem_size > SSRNX_BIONIC_STATIC_TLS_CAPACITY - offset) {
        pthread_mutex_unlock(&compat_threads_g_android_tls_registry_lock);
        SSRNX_LOGCE("TLS", "static TLS overflow module=%s memsz=%zu capacity=%u",
                    module->path, module->tls.mem_size, (unsigned)SSRNX_BIONIC_STATIC_TLS_CAPACITY);
        return SSRNX_ERR_NO_MEMORY;
    }

    if (compat_threads_g_android_tls_module_count == compat_threads_g_android_tls_module_capacity) {
        size_t next = compat_threads_g_android_tls_module_capacity ? compat_threads_g_android_tls_module_capacity * 2u : 16u;
        if (next < compat_threads_g_android_tls_module_capacity ||
            next > SIZE_MAX / sizeof(*compat_threads_g_android_tls_modules)) {
            pthread_mutex_unlock(&compat_threads_g_android_tls_registry_lock);
            return SSRNX_ERR_NO_MEMORY;
        }
        compat_threads_SsrnxBionicTlsRegistryEntry *grown = (compat_threads_SsrnxBionicTlsRegistryEntry *)realloc(
            compat_threads_g_android_tls_modules, next * sizeof(*grown));
        if (!grown) {
            pthread_mutex_unlock(&compat_threads_g_android_tls_registry_lock);
            return SSRNX_ERR_NO_MEMORY;
        }
        compat_threads_g_android_tls_modules = grown;
        compat_threads_g_android_tls_module_capacity = next;
    }

    module->tls.module_id = compat_threads_g_android_tls_module_count + 1u;
    module->tls.static_offset = (ptrdiff_t)offset;
    module->tls.registered = true;
    compat_threads_g_android_tls_modules[compat_threads_g_android_tls_module_count++].module = module;
    compat_threads_g_android_tls_next_offset = offset + module->tls.mem_size;
    ++compat_threads_g_android_tls_generation;
    SSRNX_LOGCI("TLS", "register module=%s id=%zu tprel=%td memsz=%zu align=%zu generation=%zu",
                module->path, module->tls.module_id, module->tls.static_offset,
                module->tls.mem_size, align, compat_threads_g_android_tls_generation);
    pthread_mutex_unlock(&compat_threads_g_android_tls_registry_lock);
    return SSRNX_OK;
}

static bool compat_threads_android_tls_refresh_locked(compat_threads_SsrnxBionicTlsThread *tls) {
    if (!tls || !tls->arena) return false;
    if (tls->initialized_modules >= compat_threads_g_android_tls_module_count) return true;

    /* Initialize only modules this thread has never seen. Re-copying an older
     * module on a later dlopen would incorrectly reset live thread_local data. */
    for (size_t i = tls->initialized_modules; i < compat_threads_g_android_tls_module_count; ++i) {
        SsrnxModule *module = compat_threads_g_android_tls_modules[i].module;
        if (!module || !module->tls.registered || !module->tls.present) continue;
        const size_t off = (size_t)module->tls.static_offset;
        if (off > SSRNX_BIONIC_STATIC_TLS_CAPACITY ||
            module->tls.mem_size > SSRNX_BIONIC_STATIC_TLS_CAPACITY - off) return false;
        memset(tls->arena + off, 0, module->tls.mem_size);
        if (module->tls.file_size) {
            const void *src = ssrnx_module_backing_address(module, module->tls.vaddr);
            if (!src) return false;
            memcpy(tls->arena + off, src, module->tls.file_size);
        }
    }
    tls->initialized_modules = compat_threads_g_android_tls_module_count;
    return true;
}

void ssrnx_bionic_tls_activate_current(void) {
    (void)pthread_once(&compat_threads_g_android_stack_guard_once, compat_threads_android_stack_guard_init);
    compat_threads_SsrnxBionicTlsThread *tls = compat_threads_android_tls_thread_get(true);
    if (!tls || !tls->arena) abort();

    SSRNX_PERF_LOCK(&compat_threads_g_android_tls_registry_lock);
    const bool refreshed = compat_threads_android_tls_refresh_locked(tls);
    pthread_mutex_unlock(&compat_threads_g_android_tls_registry_lock);
    if (!refreshed) abort();

    uintptr_t *slots = (uintptr_t *)(void *)tls->arena;
    slots[SSRNX_BIONIC_TLS_SLOT_SELF] = (uintptr_t)tls->arena;
    slots[SSRNX_BIONIC_TLS_SLOT_THREAD_ID] = (uintptr_t)tls;
    slots[SSRNX_BIONIC_TLS_SLOT_STACK_GUARD] = compat_threads_g_android_stack_guard;
    compat_threads_android_write_tpidr_el0((uintptr_t)tls->arena);
}

void *ssrnx_bionic_tls_current_base(void) {
    compat_threads_SsrnxBionicTlsThread *tls = compat_threads_android_tls_thread_get(false);
    return tls ? (void *)tls->arena : NULL;
}

uintptr_t ssrnx_bionic_tls_stack_guard(void) {
    (void)pthread_once(&compat_threads_g_android_stack_guard_once, compat_threads_android_stack_guard_init);
    return compat_threads_g_android_stack_guard;
}

bool ssrnx_bionic_tls_validate_current(void) {
    compat_threads_SsrnxBionicTlsThread *tls = compat_threads_android_tls_thread_get(false);
    if (!tls || !tls->arena || compat_threads_g_android_stack_guard == 0) return false;
    const uintptr_t tp = compat_threads_android_read_tpidr_el0();
    if (tp != (uintptr_t)tls->arena) return false;
    const uintptr_t *slots = (const uintptr_t *)tp;
    return slots[SSRNX_BIONIC_TLS_SLOT_SELF] == tp &&
           slots[SSRNX_BIONIC_TLS_SLOT_STACK_GUARD] == compat_threads_g_android_stack_guard;
}

void *ssrnx_bionic_tls_get_addr(size_t module_id, size_t offset) {
    ssrnx_bionic_tls_activate_current();
    compat_threads_SsrnxBionicTlsThread *tls = compat_threads_android_tls_thread_get(false);
    if (!tls || module_id == 0) return NULL;

    SSRNX_PERF_LOCK(&compat_threads_g_android_tls_registry_lock);
    SsrnxModule *module = module_id <= compat_threads_g_android_tls_module_count
        ? compat_threads_g_android_tls_modules[module_id - 1u].module : NULL;
    void *result = NULL;
    if (module && module->tls.registered && offset <= module->tls.mem_size) {
        const size_t base = (size_t)module->tls.static_offset;
        if (base <= SSRNX_BIONIC_STATIC_TLS_CAPACITY &&
            offset <= SSRNX_BIONIC_STATIC_TLS_CAPACITY - base) {
            result = tls->arena + base + offset;
        }
    }
    pthread_mutex_unlock(&compat_threads_g_android_tls_registry_lock);
    return result;
}

typedef struct compat_threads_SsrnxTlsIndex {
    size_t module;
    size_t offset;
} compat_threads_SsrnxTlsIndex;

void *ssrnx_bionic___tls_get_addr(const void *index_ptr) {
    if (!index_ptr) return NULL;
    const compat_threads_SsrnxTlsIndex *index = (const compat_threads_SsrnxTlsIndex *)index_ptr;
    return ssrnx_bionic_tls_get_addr(index->module, index->offset);
}

static void compat_threads_emutls_array_destroy(void *ptr);
static void compat_threads_guest_key_table_destroy(void *ptr);
static void compat_threads_guest_keys_thread_exit(void);

#define SSRNX_ANDROID_SS_ONSTACK 1
#define SSRNX_ANDROID_SS_DISABLE 2
static void compat_threads_signal_capture_host_thread(compat_threads_CompatThread *thread);
static void compat_threads_signal_thread_exit(compat_threads_CompatThread *thread);
static void compat_threads_signal_deliver_self(compat_threads_CompatThread *self);
static int compat_threads_signal_safe_mutex_lock(pthread_mutex_t *mutex);
static int compat_threads_signal_safe_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                                 const struct timespec *deadline);
static int compat_threads_signal_safe_cond_wait_mode(pthread_cond_t *cond, pthread_mutex_t *mutex,
                                      const struct timespec *deadline, bool target_delivery);
static int compat_threads_signal_safe_join(pthread_t host, void **result);
static void compat_threads_signal_safe_addr_wait(uint32_t *addr, uint32_t expected, int64_t timeout_ns);
static void compat_threads_addr_wake(uint32_t *addr, int count);
/* Non-zero while this host thread runs a guest signal handler on behalf of
 * another (stopped) guest thread. */
static __thread unsigned compat_threads_g_signal_proxy_depth;
static __thread unsigned compat_threads_g_signal_target_delivery_depth;

/* Blocking-wait call site, recorded by the Bionic entry point and published to
 * the thread's registry entry for the duration of the safe-point wait. */
static __thread const char *compat_threads_t_wait_kind;
static __thread const void *compat_threads_t_wait_object;
static __thread uintptr_t compat_threads_t_wait_caller;
static uint64_t compat_threads_wait_clock_ms(void) {
    struct timespec now = {0, 0};
    (void)clock_gettime(CLOCK_MONOTONIC, &now);
    const uint64_t ms = (uint64_t)now.tv_sec * UINT64_C(1000) + (uint64_t)now.tv_nsec / UINT64_C(1000000);
    return ms ? ms : 1u;
}
#define GUEST_WAIT_SITE(kind, object) do { \
        compat_threads_t_wait_kind = (kind); compat_threads_t_wait_object = (object); \
        compat_threads_t_wait_caller = (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)); \
    } while (0)

static void compat_threads_internal_keys_init(void) {
    (void)pthread_key_create(&compat_threads_g_current_thread_key, NULL);
    (void)pthread_key_create(&compat_threads_g_emutls_array_key, compat_threads_emutls_array_destroy);
    compat_threads_g_guest_key_table_ready = pthread_key_create(&compat_threads_g_guest_key_table_key, compat_threads_guest_key_table_destroy) == 0;
}

static void compat_threads_ensure_internal_keys(void) {
    /* newlib's completed pthread_once still takes a process-wide mutex. */
    if (!__atomic_load_n(&compat_threads_g_internal_keys_initialized, __ATOMIC_ACQUIRE) &&
        pthread_once(&compat_threads_g_internal_keys_once, compat_threads_internal_keys_init) == 0)
        __atomic_store_n(&compat_threads_g_internal_keys_initialized, true, __ATOMIC_RELEASE);
}

static compat_threads_CompatMutex *compat_threads_find_mutex_locked(void *guest) {
    for (compat_threads_CompatMutex *it = compat_threads_g_mutexes[compat_threads_sync_bucket(guest)]; it; it = it->next) {
        if (it->guest == guest) return it;
    }
    return NULL;
}

static int compat_threads_bionic_static_mutex_type(void *guest) {
    if (!guest) return 0;
    const uint32_t word = *(const uint32_t *)guest;
    return (int)((word >> 14) & 3U);
}

static compat_threads_CompatMutex *compat_threads_create_mutex_locked(void *guest, int type, int protocol) {
    compat_threads_CompatMutex *entry = (compat_threads_CompatMutex *)calloc(1, sizeof(*entry));
    if (!entry) return NULL;

    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0) {
        free(entry);
        return NULL;
    }
    if (type == 1) (void)pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    else if (type == 2) (void)pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    const int rc = pthread_mutex_init(&entry->host, &attr);
    (void)pthread_mutexattr_destroy(&attr);
    if (rc != 0) {
        free(entry);
        return NULL;
    }

    entry->guest = guest;
    entry->type = type;
    entry->protocol = protocol;
    entry->next = compat_threads_g_mutexes[compat_threads_sync_bucket(guest)];
    compat_threads_g_mutexes[compat_threads_sync_bucket(guest)] = entry;
    return entry;
}

static compat_threads_CompatMutex *compat_threads_get_mutex(void *guest, int create) {
    if (!guest) return NULL;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatMutex *entry = compat_threads_find_mutex_locked(guest);
    if (!entry && create) entry = compat_threads_create_mutex_locked(guest, compat_threads_bionic_static_mutex_type(guest), 0);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return entry;
}

static bool compat_threads_mutex_uses_state(const compat_threads_CompatMutex *mutex) {
    return mutex->protocol == 0;
}

static compat_threads_CompatCond *compat_threads_find_cond_locked(void *guest) {
    for (compat_threads_CompatCond *it = compat_threads_g_conds[compat_threads_sync_bucket(guest)]; it; it = it->next) {
        if (it->guest == guest) return it;
    }
    return NULL;
}

static compat_threads_CompatCond *compat_threads_create_cond_locked(void *guest) {
    compat_threads_CompatCond *entry = (compat_threads_CompatCond *)calloc(1, sizeof(*entry));
    if (!entry) return NULL;
    if (pthread_cond_init(&entry->host, NULL) != 0) {
        free(entry);
        return NULL;
    }
    entry->guest = guest;
    entry->guest_clock_id = 0; /* Bionic default: CLOCK_REALTIME. */
    entry->next = compat_threads_g_conds[compat_threads_sync_bucket(guest)];
    compat_threads_g_conds[compat_threads_sync_bucket(guest)] = entry;
    return entry;
}

static compat_threads_CompatCond *compat_threads_get_cond(void *guest, int create) {
    if (!guest) return NULL;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatCond *entry = compat_threads_find_cond_locked(guest);
    if (!entry && create) entry = compat_threads_create_cond_locked(guest);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return entry;
}

static void compat_threads_cond_diag_format(const compat_threads_CompatCond *cond, char *out, size_t cap) {
    snprintf(out, cap, " condWaits=%llu condWakes=%llu condTimeouts=%llu condSignals=%llu condBroadcasts=%llu condWaiters=%u condLastHostRc=%d",
             (unsigned long long)__atomic_load_n(&cond->wait_count, __ATOMIC_RELAXED),
             (unsigned long long)__atomic_load_n(&cond->wake_count, __ATOMIC_RELAXED),
             (unsigned long long)__atomic_load_n(&cond->timeout_count, __ATOMIC_RELAXED),
             (unsigned long long)__atomic_load_n(&cond->signal_count, __ATOMIC_RELAXED),
             (unsigned long long)__atomic_load_n(&cond->broadcast_count, __ATOMIC_RELAXED),
             __atomic_load_n(&cond->waiters, __ATOMIC_RELAXED),
             __atomic_load_n(&cond->last_host_rc, __ATOMIC_RELAXED));
}

static void compat_threads_cond_diag_log(const compat_threads_CompatCond *cond, const char *event, void *guest_mutex,
                          uintptr_t caller, uint64_t sequence, int host_rc,
                          int guest_clock_id, const struct timespec *deadline, uint64_t elapsed_ms) {
    /* Per-object limits alone flood the log when short-lived conditions churn. */
    if (!ssrnx_log_category || sequence > 4) return;
    static unsigned routine_reports, error_reports;
    unsigned *reports = host_rc ? &error_reports : &routine_reports;
    if (__atomic_fetch_add(reports, 1u, __ATOMIC_RELAXED) >= (host_rc ? 64u : 128u)) return;
    char site[192] = {0}, counters[256];
    if (ssrnx_compat_describe_guest_address)
        (void)ssrnx_compat_describe_guest_address(caller, site, sizeof(site));
    else
        snprintf(site, sizeof(site), "0x%llx", (unsigned long long)caller);
    compat_threads_cond_diag_format(cond, counters, sizeof(counters));
    ssrnx_log_category("THREAD", "INFO",
                       "cond event=%s object=%p mutex=%p caller=%s sequence=%llu clock=%d timed=%d deadline=%lld.%09ld elapsedMs=%llu hostRc=%d rc=%d%s",
                       event, cond->guest, guest_mutex, site, (unsigned long long)sequence,
                       guest_clock_id, deadline != NULL,
                       (long long)(deadline ? deadline->tv_sec : 0), deadline ? deadline->tv_nsec : 0L,
                       (unsigned long long)elapsed_ms, host_rc, ssrnx_errno_host_to_android(host_rc), counters);
}

static compat_threads_CompatRwlock *compat_threads_find_rwlock_locked(void *guest) {
    for (compat_threads_CompatRwlock *it = compat_threads_g_rwlocks; it; it = it->next) {
        if (it->guest == guest) return it;
    }
    return NULL;
}

static compat_threads_CompatRwlock *compat_threads_create_rwlock_locked(void *guest, int writer_nonrecursive_preferred) {
    compat_threads_CompatRwlock *entry = (compat_threads_CompatRwlock *)calloc(1, sizeof(*entry));
    if (!entry) return NULL;
    int rc = pthread_mutex_init(&entry->lock, NULL);
    if (rc != 0) {
        free(entry);
        return NULL;
    }
    rc = pthread_cond_init(&entry->cond, NULL);
    if (rc != 0) {
        (void)pthread_mutex_destroy(&entry->lock);
        free(entry);
        return NULL;
    }
    entry->guest = guest;
    entry->writer_nonrecursive_preferred = writer_nonrecursive_preferred ? 1 : 0;
    entry->next = compat_threads_g_rwlocks;
    compat_threads_g_rwlocks = entry;
    return entry;
}

static compat_threads_CompatRwlock *compat_threads_get_rwlock(void *guest, int create) {
    if (!guest) return NULL;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatRwlock *entry = compat_threads_find_rwlock_locked(guest);
    if (!entry && create) entry = compat_threads_create_rwlock_locked(guest, 0);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return entry;
}

static compat_threads_CompatBarrier *compat_threads_find_barrier_locked(void *guest) {
    for (compat_threads_CompatBarrier *it = compat_threads_g_barriers; it; it = it->next) {
        if (it->guest == guest) return it;
    }
    return NULL;
}

static compat_threads_CompatBarrier *compat_threads_create_barrier_locked(void *guest, unsigned count) {
    compat_threads_CompatBarrier *entry = (compat_threads_CompatBarrier *)calloc(1, sizeof(*entry));
    if (!entry) return NULL;
    if (pthread_mutex_init(&entry->lock, NULL) != 0) { free(entry); return NULL; }
    if (pthread_cond_init(&entry->cond, NULL) != 0) {
        pthread_mutex_destroy(&entry->lock);
        free(entry);
        return NULL;
    }
    entry->guest = guest;
    entry->init_count = count;
    entry->next = compat_threads_g_barriers;
    compat_threads_g_barriers = entry;
    return entry;
}

static compat_threads_CompatBarrier *compat_threads_get_barrier(void *guest) {
    if (!guest) return NULL;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatBarrier *entry = compat_threads_find_barrier_locked(guest);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return entry;
}

static compat_threads_CompatThread *compat_threads_find_thread_locked(SsrnxGuestPthread id) {
    for (compat_threads_CompatThread *it = compat_threads_g_threads; it; it = it->next) {
        if (it->guest_id == id) return it;
    }
    return NULL;
}

static void compat_threads_remove_thread_locked(compat_threads_CompatThread *target) {
    compat_threads_CompatThread **link = &compat_threads_g_threads;
    while (*link) {
        if (*link == target) {
            *link = target->next;
            return;
        }
        link = &(*link)->next;
    }
}

static size_t compat_threads_reap_detached_threads(void) {
    size_t reaped = 0;
    while (__atomic_load_n(&compat_threads_g_reapable_threads, __ATOMIC_ACQUIRE) != 0) {
        compat_threads_CompatThread *victim = NULL;
        SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
        for (compat_threads_CompatThread *it = compat_threads_g_threads; it; it = it->next) {
            if (it->detached && it->exited && !it->external &&
                it->host_join_state == HOST_THREAD_JOINABLE) {
                victim = it;
                compat_threads_remove_thread_locked(victim);
                __atomic_fetch_sub(&compat_threads_g_reapable_threads, 1u, __ATOMIC_RELEASE);
                break;
            }
        }
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        if (!victim) break;
        (void)pthread_join(victim->host, NULL);
        free(victim);
        reaped++;
    }
    return reaped;
}

/* Linux drops an exited task from the kernel thread quota even if its pthread
 * remains joinable. Horizon's closed host object must be separated from that
 * guest lifetime: keep the result/attributes until guest join or detach. */
static size_t compat_threads_reap_exited_joinable_threads(void) {
    size_t reaped = 0;
    for (;;) {
        compat_threads_CompatThread *victim = NULL;
        SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
        for (compat_threads_CompatThread *it = compat_threads_g_threads; it; it = it->next) {
            if (it->exited && !it->external && !it->detached &&
                it->host_join_state == HOST_THREAD_JOINABLE) {
                victim = it;
                victim->host_join_state = HOST_THREAD_REAPING;
                break;
            }
        }
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        if (!victim) return reaped;
        void *result = NULL;
        const int rc = pthread_join(victim->host, &result);
        SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
        bool dispose = false;
        if (rc == 0) {
            victim->result = result;
            victim->host_join_state = HOST_THREAD_REAPED;
            reaped++;
            if (victim->detached) {
                compat_threads_remove_thread_locked(victim);
                __atomic_fetch_sub(&compat_threads_g_reapable_threads, 1u, __ATOMIC_RELEASE);
                dispose = true;
            }
        } else victim->host_join_state = HOST_THREAD_JOINABLE;
        pthread_cond_broadcast(&compat_threads_g_thread_state_cond);
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        if (dispose) free(victim);
        if (rc != 0) return reaped;
    }
}

static int compat_threads_capture_current_stack(compat_threads_CompatThread *thread) {
    if (!thread) return EINVAL;
#ifdef __SWITCH__
    Thread *host_thread = threadGetSelf();
    if (!host_thread || !host_thread->stack_mirror || host_thread->stack_sz == 0) return ENOTSUP;
    /* stack_mirror is the virtual address range actually used by the running
     * thread. stack_mem is only its backing allocation and must not be exposed
     * as pthread_attr_getstack()'s address. libnx does not install a guard page. */
    thread->stack_base = host_thread->stack_mirror;
    thread->stack_size = host_thread->stack_sz;
    thread->guard_size = 0;
    return 0;
#else
    pthread_attr_t attr;
    int rc = pthread_getattr_np(pthread_self(), &attr);
    if (rc != 0) return rc;
    void *stack_base = NULL;
    size_t stack_size = 0, guard_size = 0;
    rc = pthread_attr_getstack(&attr, &stack_base, &stack_size);
    if (rc == 0) rc = pthread_attr_getguardsize(&attr, &guard_size);
    (void)pthread_attr_destroy(&attr);
    if (rc != 0) return rc;
    thread->stack_base = stack_base;
    thread->stack_size = stack_size;
    thread->guard_size = guard_size;
    return 0;
#endif
}

static compat_threads_CompatThread *compat_threads_adopt_current_thread(void) {
    compat_threads_ensure_internal_keys();
    compat_threads_CompatThread *current = (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
    if (current) return current;

    current = (compat_threads_CompatThread *)calloc(1, sizeof(*current));
    if (!current) return NULL;
    current->host = pthread_self();
    current->external = 1;
    current->sig_altstack.ss_flags = SSRNX_ANDROID_SS_DISABLE;
    /* Threads created outside Bionic (for example Java runtime workers that
     * attach to native code) are still Android guest threads once adopted.
     * Give them the same process CPU policy as pthread_create workers. */
    (void)ssrnx_thread_policy_pin_background();
    compat_threads_signal_capture_host_thread(current);
    current->stack_capture_rc = compat_threads_capture_current_stack(current);
    current->stack_ready = 1;

    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    current->guest_id = compat_threads_g_next_thread_id++;
    if (current->guest_id == 0) current->guest_id = compat_threads_g_next_thread_id++;
    current->android_tid = compat_threads_g_next_android_tid++;
    if (compat_threads_g_next_android_tid <= 0) compat_threads_g_next_android_tid = 1000;
    current->next = compat_threads_g_threads;
    compat_threads_g_threads = current;
    pthread_mutex_unlock(&compat_threads_g_registry_lock);

    if (pthread_setspecific(compat_threads_g_current_thread_key, current) != 0) {
        SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
        compat_threads_remove_thread_locked(current);
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        free(current);
        return NULL;
    }
    return current;
}

static void compat_threads_mark_current_thread_exit(void *result) {
    compat_threads_ensure_internal_keys();
    compat_threads_CompatThread *current = (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
    if (!current) return;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    current->result = result;
    current->exited = 1;
    if (current->detached && !current->external)
        __atomic_fetch_add(&compat_threads_g_reapable_threads, 1u, __ATOMIC_RELEASE);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
}


/* Generic guest CPU policy.  Do not reserve a core merely because a game
 * might have a dedicated renderer: app-managed Android render loops can keep
 * rendering on guest-owned threads and never call pin_render().  Until a
 * runtime-owned renderer explicitly elects itself, every guest pthread may
 * run on the full application-core mask.  Once a renderer is elected, reserve
 * the last application core for it and move guest workers onto the remaining
 * mask.  This mirrors Android's scheduler freedom more closely than pinning
 * each guest worker permanently to one CPU. */
#ifdef __SWITCH__
static pthread_mutex_t compat_threads_g_core_policy_lock = PTHREAD_MUTEX_INITIALIZER;
static int compat_threads_g_core_policy_list[4];
static int compat_threads_g_core_policy_count;
static unsigned compat_threads_g_core_policy_rr;
static bool compat_threads_g_core_policy_render_reserved;
static pthread_t compat_threads_g_core_policy_render_thread;
static unsigned compat_threads_g_core_policy_trace_writes;

/* Capture the writer; the sampler renews the bounded budget at each report. */
static Result compat_threads_thread_policy_set_core_mask(Handle handle, int core, uint64_t mask,
                                         const char *reason, int pid, uint64_t requested) {
    if (!ssrnx_log_category ||
        __atomic_fetch_add(&compat_threads_g_core_policy_trace_writes, 1u, __ATOMIC_RELAXED) >= 256u)
        return svcSetThreadCoreMask(handle, core, mask);
    const int saved_errno = errno;
    const uint64_t begin_us = ssrnx_perf_now_us();
    s32 before_core = -1, after_core = -1;
    u64 before_mask = 0, after_mask = 0;
    const Result before_rc = svcGetThreadCoreMask(&before_core, &before_mask, handle);
    const Result rc = svcSetThreadCoreMask(handle, core, mask);
    const Result after_rc = svcGetThreadCoreMask(&after_core, &after_mask, handle);
    SSRNX_LOGCI("THREAD", "affinity-write reason=%s caller=%p pid=%d handle=0x%x requestedMask=0x%llx appliedMask=0x%llx idealCore=%d beforeMask=0x%llx beforeIdeal=%d afterMask=0x%llx afterIdeal=%d beforeRc=0x%x setRc=0x%x afterRc=0x%x timeUs=%llu endUs=%llu",
                reason, (void *)(uintptr_t)pthread_self(), pid, (unsigned)handle,
                (unsigned long long)requested, (unsigned long long)mask, core,
                (unsigned long long)before_mask, (int)before_core,
                (unsigned long long)after_mask, (int)after_core,
                (unsigned)before_rc, (unsigned)rc, (unsigned)after_rc,
                (unsigned long long)begin_us, (unsigned long long)ssrnx_perf_now_us());
    errno = saved_errno;
    return rc;
}

static Result compat_threads_thread_policy_set_priority(Handle handle, int priority, const char *reason, int nice_value) {
    if (!ssrnx_log_category ||
        __atomic_fetch_add(&compat_threads_g_core_policy_trace_writes, 1u, __ATOMIC_RELAXED) >= 256u)
        return svcSetThreadPriority(handle, priority);
    const int saved_errno = errno;
    const uint64_t begin_us = ssrnx_perf_now_us();
    s32 before = -1, after = -1;
    const Result before_rc = svcGetThreadPriority(&before, handle);
    const Result rc = svcSetThreadPriority(handle, priority);
    const Result after_rc = svcGetThreadPriority(&after, handle);
    SSRNX_LOGCI("THREAD", "priority-write reason=%s caller=%p handle=0x%x requestedNice=%d appliedPriority=%d beforePriority=%d afterPriority=%d beforeRc=0x%x setRc=0x%x afterRc=0x%x timeUs=%llu endUs=%llu",
                reason, (void *)(uintptr_t)pthread_self(), (unsigned)handle, nice_value, priority,
                (int)before, (int)after, (unsigned)before_rc, (unsigned)rc, (unsigned)after_rc,
                (unsigned long long)begin_us, (unsigned long long)ssrnx_perf_now_us());
    errno = saved_errno;
    return rc;
}

#endif
void ssrnx_bionic_thread_diagnostics_report(void) {
#ifdef __SWITCH__
    const unsigned writes = __atomic_exchange_n(&compat_threads_g_core_policy_trace_writes, 0u, __ATOMIC_RELAXED);
    SSRNX_LOGCI("THREAD", "scheduling-coverage writes=%u recorded=%u suppressed=%u budget=256/window",
                writes, writes < 256u ? writes : 256u, writes > 256u ? writes - 256u : 0u);
#endif
}
#ifdef __SWITCH__

static void compat_threads_thread_policy_init_locked(void) {
    if (compat_threads_g_core_policy_count != 0) return;
    uint64_t mask = 0;
    if (R_FAILED(svcGetInfo(&mask, InfoType_CoreMask, CUR_PROCESS_HANDLE, 0)) || mask == 0)
        mask = UINT64_C(0x7);
    /* Core 3 is normally Horizon-reserved. If the process mask unexpectedly
     * contains it, prefer the application cores unless doing so would leave
     * no usable core. */
    const uint64_t app_mask = mask & ~UINT64_C(0x8);
    if (app_mask) mask = app_mask;
    for (int core = 0; core < 4; ++core) {
        if (mask & (UINT64_C(1) << core))
            compat_threads_g_core_policy_list[compat_threads_g_core_policy_count++] = core;
    }
    if (compat_threads_g_core_policy_count == 0)
        compat_threads_g_core_policy_list[compat_threads_g_core_policy_count++] = 0;
}

static uint64_t compat_threads_thread_policy_background_mask_locked(void) {
    uint64_t mask = 0;
    const int usable = (compat_threads_g_core_policy_render_reserved && compat_threads_g_core_policy_count > 1)
        ? compat_threads_g_core_policy_count - 1 : compat_threads_g_core_policy_count;
    for (int i = 0; i < usable; ++i) mask |= UINT64_C(1) << compat_threads_g_core_policy_list[i];
    return mask;
}

static int compat_threads_thread_policy_next_background_core_locked(void) {
    const int usable = (compat_threads_g_core_policy_render_reserved && compat_threads_g_core_policy_count > 1)
        ? compat_threads_g_core_policy_count - 1 : compat_threads_g_core_policy_count;
    return compat_threads_g_core_policy_list[compat_threads_g_core_policy_rr++ % (unsigned)usable];
}

/* sched_setaffinity(): narrow a guest-requested mask to the cores the policy
 * gives guest workers (the render core stays reserved when the request allows
 * another core), and choose the ideal core round-robin inside it so threads
 * that request the same mask are spread instead of stacked on its lowest CPU.
 * Horizon does not time-slice equal-priority threads on one core, so stacking
 * serializes a whole worker pool. */
static uint64_t compat_threads_thread_policy_affinity_mask_locked(uint64_t requested, pthread_t target) {
    if (compat_threads_g_core_policy_render_reserved && pthread_equal(compat_threads_g_core_policy_render_thread, target)) {
        const uint64_t render = UINT64_C(1) << compat_threads_g_core_policy_list[compat_threads_g_core_policy_count - 1];
        /* Preserve the reservation only when the guest still allows that core. */
        return (requested & render) ? render : requested;
    }
    const uint64_t background = requested & compat_threads_thread_policy_background_mask_locked();
    return background ? background : requested;
}

static int compat_threads_thread_policy_core_in_mask_locked(uint64_t mask) {
    const int cores = __builtin_popcountll(mask);
    if (cores <= 0) return -1;
    int nth = (int)(compat_threads_g_core_policy_rr++ % (unsigned)cores);
    for (int core = 0; core < 64; ++core) {
        if (!(mask & (UINT64_C(1) << core))) continue;
        if (nth-- == 0) return core;
    }
    return -1;
}
#endif

void ssrnx_thread_policy_reset(void) {
#ifdef __SWITCH__
    SSRNX_PERF_LOCK(&compat_threads_g_core_policy_lock);
    compat_threads_g_core_policy_render_reserved = false;
    compat_threads_g_core_policy_rr = 0;
    __atomic_store_n(&compat_threads_g_core_policy_trace_writes, 0u, __ATOMIC_RELAXED);
    pthread_mutex_unlock(&compat_threads_g_core_policy_lock);
#endif
}

int ssrnx_thread_policy_pin_render(void) {
#ifdef __SWITCH__
    SSRNX_PERF_LOCK(&compat_threads_g_core_policy_lock);
    compat_threads_thread_policy_init_locked();
    const int core = compat_threads_g_core_policy_list[compat_threads_g_core_policy_count - 1];
    compat_threads_g_core_policy_render_reserved = true;
    compat_threads_g_core_policy_render_thread = pthread_self();
    const uint64_t background_mask = compat_threads_thread_policy_background_mask_locked();
    pthread_mutex_unlock(&compat_threads_g_core_policy_lock);

    /* Guest threads may have been created before the runtime decided to use a
     * dedicated render core. Move those existing workers off that core now so
     * the reservation actually takes effect. */
    if (compat_threads_g_core_policy_count > 1) {
        SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
        unsigned rr = 0;
        for (compat_threads_CompatThread *t = compat_threads_g_threads; t; t = t->next) {
            if (t->exited || t->host_handle == INVALID_HANDLE) continue;
            const int ideal = compat_threads_g_core_policy_list[rr++ % (unsigned)(compat_threads_g_core_policy_count - 1)];
            (void)compat_threads_thread_policy_set_core_mask(t->host_handle, ideal, background_mask,
                                             "reserve-workers", t->android_tid, background_mask);
        }
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
    }

    Result rc = compat_threads_thread_policy_set_core_mask(CUR_THREAD_HANDLE, core, UINT64_C(1) << core,
                                           "elect-render", 0, UINT64_C(1) << core);

    /* Android schedules the top app's render thread ahead of its workers
     * (display priority). Here that also matters for locks: Horizon lends a
     * waiter's priority to the lock holder, so a worker holding a lock the
     * render thread needs runs at once instead of waiting for its next
     * time-slice among equal-priority threads. Raise it to the process's
     * main-thread priority when it currently runs below that. */
    s32 current = 0;
    if (R_SUCCEEDED(svcGetThreadPriority(&current, CUR_THREAD_HANDLE)) && current > 0x2C) {
        u64 allowed = 0;
        if (R_FAILED(svcGetInfo(&allowed, InfoType_PriorityMask, CUR_PROCESS_HANDLE, 0)) ||
            (allowed & (UINT64_C(1) << 0x2C)) != 0)
            (void)compat_threads_thread_policy_set_priority(CUR_THREAD_HANDLE, 0x2C, "elect-render", 0);
    }
    return R_SUCCEEDED(rc) ? core : -1;
#else
    return -1;
#endif
}

int ssrnx_thread_policy_pin_background(void) {
#ifdef __SWITCH__
    SSRNX_PERF_LOCK(&compat_threads_g_core_policy_lock);
    compat_threads_thread_policy_init_locked();
    const int core = compat_threads_thread_policy_next_background_core_locked();
    const uint64_t mask = compat_threads_thread_policy_background_mask_locked();
    pthread_mutex_unlock(&compat_threads_g_core_policy_lock);
    Result rc = compat_threads_thread_policy_set_core_mask(CUR_THREAD_HANDLE, core, mask, "background", 0, mask);
    return R_SUCCEEDED(rc) ? core : -1;
#else
    return -1;
#endif
}

static void compat_threads_guest_cleanup_run_all(compat_threads_CompatThread *thread) {
    if (!thread) return;
    while (thread->cleanup_stack) {
        SsrnxBionicPthreadCleanup *cleanup = thread->cleanup_stack;
        thread->cleanup_stack = cleanup->cleanup_prev;
        if (cleanup->cleanup_routine) cleanup->cleanup_routine(cleanup->cleanup_arg);
    }
}

void ssrnx_bionic_pthread_cleanup_push(SsrnxBionicPthreadCleanup *cleanup,
                                        void (*routine)(void *), void *arg) {
    if (!cleanup) return;
    compat_threads_CompatThread *thread = compat_threads_adopt_current_thread();
    cleanup->cleanup_routine = routine;
    cleanup->cleanup_arg = arg;
    cleanup->cleanup_prev = thread ? thread->cleanup_stack : NULL;
    if (thread) thread->cleanup_stack = cleanup;
}

void ssrnx_bionic_pthread_cleanup_pop(SsrnxBionicPthreadCleanup *cleanup, int execute) {
    if (!cleanup) return;
    compat_threads_CompatThread *thread = compat_threads_adopt_current_thread();
    if (thread) thread->cleanup_stack = cleanup->cleanup_prev;
    if (execute && cleanup->cleanup_routine) cleanup->cleanup_routine(cleanup->cleanup_arg);
}

static void *compat_threads_thread_trampoline(void *arg) {
    compat_threads_CompatThread *thread = (compat_threads_CompatThread *)arg;
    compat_threads_ensure_internal_keys();
    (void)pthread_setspecific(compat_threads_g_current_thread_key, thread);
    /* Every host thread that crosses into guest AArch64 needs Android's
     * TPIDR_EL0 view before the first guest instruction executes. Keep guest
     * workers away from the reserved render core as well. */
    (void)ssrnx_thread_policy_pin_background();
    ssrnx_bionic_tls_activate_current();
    compat_threads_signal_capture_host_thread(thread);

    const int stack_rc = compat_threads_capture_current_stack(thread);
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    thread->stack_capture_rc = stack_rc;
    thread->stack_ready = 1;
    pthread_cond_broadcast(&compat_threads_g_thread_state_cond);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);

    void *result = thread->guest_start(thread->guest_arg);
    /* Returning from the start routine is equivalent to pthread_exit. Match
     * Bionic's destructor order: C++ thread_local dtors, cleanup handlers,
     * then pthread-key destructors. */
    if (ssrnx_bionic_cxa_thread_finalize_current) ssrnx_bionic_cxa_thread_finalize_current();
    compat_threads_guest_cleanup_run_all(thread);
    compat_threads_signal_thread_exit(thread);
    compat_threads_guest_keys_thread_exit();
    if (ssrnx_heap_cache_thread_exit) ssrnx_heap_cache_thread_exit();
    compat_threads_mark_current_thread_exit(result);
    return result;
}

/* A host thread adopted into the registry (Java Thread workers and other
 * runtime threads that run guest code) has no Bionic start routine to return
 * through. Its owner calls this right before the host thread returns, giving
 * it the same exit sequence as a guest pthread: C++ thread_local destructors,
 * pthread-key destructors (e.g. a VM detaching the thread from its GC), signal
 * quiescence, then removal so pthread_kill() reports ESRCH. */
void ssrnx_bionic_thread_external_exit(void) {
    compat_threads_ensure_internal_keys();
    compat_threads_CompatThread *current = (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
    if (!current || !current->external) return;
    /* Bounded trace: each phase runs guest code (thread_local and pthread-key
     * destructors), so a fault inside shows as a begin without its end. */
    static uint32_t exit_traces;
    const bool trace = ssrnx_log_category && __atomic_fetch_add(&exit_traces, 1u, __ATOMIC_RELAXED) < 64u;
    if (trace)
        ssrnx_log_category("THREAD", "INFO", "adopted thread exit begin guest=%llu name=%s keys=%d",
                           (unsigned long long)current->guest_id, current->name[0] ? current->name : "-",
                           current->key_table ? 1 : 0);
    if (ssrnx_bionic_cxa_thread_finalize_current) ssrnx_bionic_cxa_thread_finalize_current();
    compat_threads_guest_cleanup_run_all(current);
    compat_threads_signal_thread_exit(current);
    compat_threads_guest_keys_thread_exit();
    if (trace)
        ssrnx_log_category("THREAD", "INFO", "adopted thread exit end guest=%llu",
                           (unsigned long long)current->guest_id);
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    current->exited = 1;
    compat_threads_remove_thread_locked(current);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    (void)pthread_setspecific(compat_threads_g_current_thread_key, NULL);
    free(current);
}

static int compat_threads_pthread_setup_failure(const char *operation,void *object,const void *attr,int result,uintptr_t caller) {
    const int saved=errno;
    if(ssrnx_log_category){char site[192];snprintf(site,sizeof(site),"%p",(void*)caller);
        if(ssrnx_compat_describe_guest_address)(void)ssrnx_compat_describe_guest_address(caller,site,sizeof(site));
        ssrnx_log_category("THREAD","WARN","%s setup failed androidRc=%d object=%p attr=%p caller=%s",operation,result,object,attr,site);}
    errno=saved;return result;
}
#define PTHREAD_SETUP_FAIL(op,obj,attr,rc) compat_threads_pthread_setup_failure(op,obj,attr,rc,(uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)))

int ssrnx_bionic_pthread_create(SsrnxGuestPthread *out_thread, const void *guest_attr,
                                void *(*start_routine)(void *), void *arg) {
    if (!out_thread || !start_routine) return PTHREAD_SETUP_FAIL("pthread_create",out_thread,guest_attr,EINVAL);
    const int requested_detached = guest_attr && ((((const uint32_t *)guest_attr)[0] & UINT32_C(0x1)) != 0);
    compat_threads_ensure_internal_keys();
    compat_threads_reap_detached_threads();

    compat_threads_CompatThread *thread = (compat_threads_CompatThread *)calloc(1, sizeof(*thread));
    if (!thread) return PTHREAD_SETUP_FAIL("pthread_create",out_thread,guest_attr,ENOMEM);
    thread->guest_start = start_routine;
    thread->guest_arg = arg;
    thread->detached = requested_detached;
    thread->sig_altstack.ss_flags = SSRNX_ANDROID_SS_DISABLE;
    /* POSIX: a new thread inherits the creator's signal mask and starts with
     * no pending signals. */
    {
        compat_threads_CompatThread *creator = (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
        if (creator) thread->sig_blocked = __atomic_load_n(&creator->sig_blocked, __ATOMIC_ACQUIRE);
        const SsrnxBionicPthreadAttrLayout *a =
            guest_attr ? (const SsrnxBionicPthreadAttrLayout *)guest_attr : NULL;
        thread->attr_flags = a ? a->flags : 0u;
        if (a && (a->flags & SSRNX_BIONIC_PTHREAD_ATTR_INHERIT) && creator) {
            thread->sched_policy = creator->sched_policy;
            thread->sched_priority = creator->sched_priority;
        } else if (a) {
            thread->sched_policy = a->sched_policy;
            thread->sched_priority = a->sched_priority;
        } else if (creator) {
            /* Historical Bionic default with no explicit attr follows the
             * normal inherited SCHED_OTHER/priority-0 application state. */
            thread->sched_policy = creator->sched_policy;
            thread->sched_priority = creator->sched_priority;
        }
    }

    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    thread->guest_id = compat_threads_g_next_thread_id++;
    if (thread->guest_id == 0) thread->guest_id = compat_threads_g_next_thread_id++;
    thread->android_tid = compat_threads_g_next_android_tid++;
    if (compat_threads_g_next_android_tid <= 0) compat_threads_g_next_android_tid = 1000;
    thread->next = compat_threads_g_threads;
    compat_threads_g_threads = thread;
    pthread_mutex_unlock(&compat_threads_g_registry_lock);

    pthread_attr_t host_attr;
    pthread_attr_t *host_attr_ptr = NULL;
    /* Bionic's default application pthread stack is 1 MiB. libnx/newlib's
     * host default is smaller, so always translate the Android default rather
     * than inheriting the host library's policy when guest_attr is NULL. */
    size_t requested_stack = SSRNX_BIONIC_PTHREAD_STACK_DEFAULT;
    void *requested_stack_base = NULL;
    size_t requested_guard = SSRNX_BIONIC_PAGE_SIZE;
    if (guest_attr) {
        const SsrnxBionicPthreadAttrLayout *a = (const SsrnxBionicPthreadAttrLayout *)guest_attr;
        if (a->stack_size != 0) requested_stack = a->stack_size;
        requested_stack_base = a->stack_base;
        requested_guard = a->guard_size;
    }
    /* Match Bionic pthread_create: attr setters retain the requested byte
     * count, while SSRNX-owned stack allocation rounds up to an Android page. */
    if (!requested_stack_base) requested_stack = (requested_stack + 4095u) & ~(size_t)4095u;
    int arc = pthread_attr_init(&host_attr);
    if (arc != 0) {
        SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
        compat_threads_remove_thread_locked(thread);
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        free(thread);
        if (ssrnx_log_category) SSRNX_LOGCW("THREAD",
                    "pthread_create host attr init failed hostRc=%d androidRc=%d",
                    arc, ssrnx_errno_host_to_android(arc));
        return ssrnx_errno_host_to_android(arc);
    }
    if (requested_stack_base) arc = pthread_attr_setstack(&host_attr, requested_stack_base, requested_stack);
    else arc = pthread_attr_setstacksize(&host_attr, requested_stack);
#ifndef __SWITCH__
    /* Linux pthreads can install the requested guard region directly. Horizon/
     * libnx threads do not expose a host guard-page facility here; the Android
     * guard size remains guest ABI metadata and must not make pthread_create()
     * fail merely because the host cannot mirror that implementation detail. */
    if (arc == 0 && !requested_stack_base) arc = pthread_attr_setguardsize(&host_attr, requested_guard);
#else
    (void)requested_guard;
#endif
    if (arc != 0) {
        (void)pthread_attr_destroy(&host_attr);
        SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
        compat_threads_remove_thread_locked(thread);
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        free(thread);
        if (ssrnx_log_category) SSRNX_LOGCW("THREAD",
                    "pthread_create host attr setup failed hostRc=%d androidRc=%d stack=%zu explicit=%d",
                    arc, ssrnx_errno_host_to_android(arc), requested_stack, requested_stack_base ? 1 : 0);
        return ssrnx_errno_host_to_android(arc);
    }
    host_attr_ptr = &host_attr;

    const int rc = ssrnx_host_pthread_create(&thread->host, host_attr_ptr, compat_threads_thread_trampoline, thread);
    (void)pthread_attr_destroy(&host_attr);
    if (rc != 0) {
        SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
        compat_threads_remove_thread_locked(thread);
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        free(thread);
        if (ssrnx_log_category) {
            char start_site[192], creator_site[192];
            const uintptr_t creator_pc =
                (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0));
            snprintf(start_site, sizeof(start_site), "%p", (void *)start_routine);
            snprintf(creator_site, sizeof(creator_site), "0x%lx", (unsigned long)creator_pc);
            if (ssrnx_compat_describe_guest_address) {
                (void)ssrnx_compat_describe_guest_address((uintptr_t)start_routine, start_site, sizeof(start_site));
                (void)ssrnx_compat_describe_guest_address(creator_pc, creator_site, sizeof(creator_site));
            }
            SSRNX_LOGCW("THREAD",
                    "pthread_create host create failed hostRc=%d androidRc=%d stack=%zu detached=%d start=%s arg=%p creator=%s",
                    rc, ssrnx_errno_host_to_android(rc), requested_stack, requested_detached ? 1 : 0,
                    start_site, arg, creator_site);
        }
        return ssrnx_errno_host_to_android(rc);
    }

    /* Do not return a guest pthread_t until its observable stack attributes
     * are known. Bionic pthread_getattr_np() can be called immediately after
     * pthread_create(), so removing this race is part of the ABI contract. */
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    while (!thread->stack_ready) pthread_cond_wait(&compat_threads_g_thread_state_cond, &compat_threads_g_registry_lock);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);

    *out_thread = thread->guest_id;
    {
        char start_site[192];
        if (!ssrnx_compat_describe_guest_address ||
            !ssrnx_compat_describe_guest_address((uintptr_t)start_routine,
                                                 start_site, sizeof(start_site)))
            snprintf(start_site, sizeof(start_site), "%p", (void *)start_routine);
        const uintptr_t creator_pc =
            (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0));
        char creator_site[192];
        if (!ssrnx_compat_describe_guest_address ||
            !ssrnx_compat_describe_guest_address(creator_pc, creator_site, sizeof(creator_site)))
            snprintf(creator_site, sizeof(creator_site), "0x%lx", (unsigned long)creator_pc);
        if (ssrnx_log_category) SSRNX_LOGCI("THREAD",
                    "pthread_create guest=%llu start=%s arg=%p stack=%zu detached=%d creator=%s",
                    (unsigned long long)thread->guest_id, start_site, arg,
                    requested_stack, requested_detached ? 1 : 0, creator_site);
    }
    /* No caller/reaper may free this entry before create's final use of it. */
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    thread->host_join_state = HOST_THREAD_JOINABLE;
    pthread_cond_broadcast(&compat_threads_g_thread_state_cond);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return 0;
}

int ssrnx_bionic_pthread_join(SsrnxGuestPthread guest_thread, void **retval) {
    compat_threads_reap_detached_threads();
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatThread *thread = compat_threads_find_thread_locked(guest_thread);
    while (thread && !thread->external && !thread->detached &&
                     (thread->host_join_state == HOST_THREAD_REAPING ||
                      thread->host_join_state == HOST_THREAD_CREATING)) {
        if (thread == pthread_getspecific(compat_threads_g_current_thread_key)) break;
        pthread_cond_wait(&compat_threads_g_thread_state_cond, &compat_threads_g_registry_lock);
        thread = compat_threads_find_thread_locked(guest_thread);
    }
    if (!thread || thread->external || thread->detached ||
        thread->host_join_state == HOST_THREAD_GUEST_JOINING) {
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        return EINVAL;
    }
    if (thread == pthread_getspecific(compat_threads_g_current_thread_key)) {
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        return ssrnx_errno_host_to_android(EDEADLK);
    }
    if (thread->host_join_state == HOST_THREAD_REAPED) {
        void *result = thread->result;
        compat_threads_remove_thread_locked(thread);
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        if (retval) *retval = result;
        free(thread);
        return 0;
    }
    thread->host_join_state = HOST_THREAD_GUEST_JOINING;
    const pthread_t host = thread->host;
    pthread_mutex_unlock(&compat_threads_g_registry_lock);

    void *result = NULL;
    GUEST_WAIT_SITE("pthread_join", (const void *)(uintptr_t)guest_thread);
    const int rc = compat_threads_signal_safe_join(host, &result);
    if (rc != 0) {
        SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
        thread->host_join_state = HOST_THREAD_JOINABLE;
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        return ssrnx_errno_host_to_android(rc);
    }

    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    thread = compat_threads_find_thread_locked(guest_thread);
    if (thread) compat_threads_remove_thread_locked(thread);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    if (retval) *retval = result;
    free(thread);
    return 0;
}

int ssrnx_bionic_pthread_detach(SsrnxGuestPthread guest_thread) {
    compat_threads_reap_detached_threads();
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatThread *thread = compat_threads_find_thread_locked(guest_thread);
    if (!thread || thread->external || thread->detached ||
        thread->host_join_state == HOST_THREAD_GUEST_JOINING) {
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        return EINVAL;
    }
    if (thread->host_join_state == HOST_THREAD_REAPED) {
        compat_threads_remove_thread_locked(thread);
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        free(thread);
        return 0;
    }
    /* libnx's newlib backend does not provide native pthread_detach. Keep the
       host thread joinable internally and reap it after exit, while presenting
       detached semantics to the Android guest. */
    thread->detached = 1;
    if (thread->exited) __atomic_fetch_add(&compat_threads_g_reapable_threads, 1u, __ATOMIC_RELEASE);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return 0;
}

static int compat_threads_current_android_tid_value(void) {
    /* Android task identity is SSRNX-owned state. A Java/runtime worker may be
     * created by the host rather than guest pthread_create(), but once it
     * crosses into Android it must still have a stable Linux-style TID. */
    compat_threads_CompatThread *thread = compat_threads_adopt_current_thread();
    return thread && thread->android_tid > 0 ? thread->android_tid : 1;
}

int ssrnx_bionic_gettid(void) {
    return compat_threads_current_android_tid_value();
}

#define SSRNX_ANDROID_PRIO_PROCESS 0
#define SSRNX_ANDROID_PRIO_PGRP    1
#define SSRNX_ANDROID_PRIO_USER    2
#define SSRNX_ANDROID_NICE_MIN   (-20)
#define SSRNX_ANDROID_NICE_MAX     19

#ifdef __SWITCH__
static int compat_threads_apply_current_android_nice(compat_threads_CompatThread *thread, int nice_value) {
    if (!thread) { errno = ENOMEM; return -1; }
    SSRNX_PERF_LOCK(&compat_threads_g_core_policy_lock);
    const bool render_thread = compat_threads_g_core_policy_render_reserved &&
                               pthread_equal(compat_threads_g_core_policy_render_thread, pthread_self());
    pthread_mutex_unlock(&compat_threads_g_core_policy_lock);
    /* The render thread keeps its reserved core and display priority. */
    if (render_thread) return 0;

    if (!thread->host_base_priority_valid) {
        s32 host_priority = 0;
        Result rc = svcGetThreadPriority(&host_priority, CUR_THREAD_HANDLE);
        if (R_FAILED(rc)) { errno = ENOTSUP; return -1; }
        thread->host_base_priority = (int)host_priority;
        thread->host_base_priority_valid = 1;
    }

    /* Horizon priorities use the same direction as Linux niceness after this
     * translation: a lower numeric Horizon priority is scheduled ahead of a
     * higher one. Anchor nice 0 at the thread's actual pre-guest priority so
     * SSRNX does not impose a global hard-coded scheduler priority. */
    const int base = thread->host_base_priority;
    int target = base + nice_value;
    if (target < 0) target = 0;
    if (target > 0x3f) target = 0x3f;

    u64 priority_mask = 0;
    Result rc = svcGetInfo(&priority_mask, InfoType_PriorityMask, CUR_PROCESS_HANDLE, 0);
    if (R_SUCCEEDED(rc) && (priority_mask & (UINT64_C(1) << (unsigned)target)) == 0) {
        if (target <= base) {
            /* Linux setpriority reports EACCES when an otherwise-valid request
             * would raise scheduling priority beyond the caller's allowance. */
            errno = EACCES;
            return -1;
        }
        /* Lowering is always permitted on Linux. Application threads normally
         * already sit at the lowest permitted (time-shared) Horizon priority,
         * so use the lowest permitted level at or above the base. */
        while (target > base && (priority_mask & (UINT64_C(1) << (unsigned)target)) == 0) --target;
    }

    rc = compat_threads_thread_policy_set_priority(CUR_THREAD_HANDLE, target, "nice", nice_value);
    if (R_FAILED(rc)) { errno = EPERM; return -1; }

    /* Android places threads at nice >= ANDROID_PRIORITY_BACKGROUND (10) in
     * the background cpuset (SP_BACKGROUND), which confines them to a subset
     * of CPUs so they cannot occupy the cores foreground threads use. Horizon
     * priority alone cannot express that below the time-shared level, so
     * mirror the cpuset: background threads share one application core. */
    SSRNX_PERF_LOCK(&compat_threads_g_core_policy_lock);
    compat_threads_thread_policy_init_locked();
    if (compat_threads_g_core_policy_render_reserved && pthread_equal(compat_threads_g_core_policy_render_thread, pthread_self())) {
        /* The render thread keeps its reserved core whatever its niceness. */
        pthread_mutex_unlock(&compat_threads_g_core_policy_lock);
        return 0;
    }
    const bool background = nice_value >= 10 && compat_threads_g_core_policy_count > 1;
    const int core = background ? compat_threads_g_core_policy_list[0] : compat_threads_thread_policy_next_background_core_locked();
    const uint64_t mask = background ? (UINT64_C(1) << core) : compat_threads_thread_policy_background_mask_locked();
    pthread_mutex_unlock(&compat_threads_g_core_policy_lock);
    (void)compat_threads_thread_policy_set_core_mask(CUR_THREAD_HANDLE, core, mask, "nice", thread->android_tid, mask);
    return 0;
}
#else
static int compat_threads_apply_current_android_nice(compat_threads_CompatThread *thread, int nice_value) {
    (void)thread;
    (void)nice_value;
    /* Host regressions validate the guest-visible state machine. Production
     * Switch builds use the real Horizon scheduler path above. */
    return 0;
}
#endif

static compat_threads_CompatThread *compat_threads_priority_current_thread(int which, int who) {
    if (which != SSRNX_ANDROID_PRIO_PROCESS) {
        if (which == SSRNX_ANDROID_PRIO_PGRP || which == SSRNX_ANDROID_PRIO_USER)
            errno = ENOSYS;
        else
            errno = EINVAL;
        return NULL;
    }

    const int current_tid = compat_threads_current_android_tid_value();
    if (who != 0 && who != current_tid) {
        errno = ESRCH;
        return NULL;
    }

    compat_threads_reap_detached_threads();
    compat_threads_CompatThread *thread = compat_threads_adopt_current_thread();
    if (!thread) errno = ENOMEM;
    return thread;
}

int ssrnx_bionic_getpriority(int which, int who) {
    compat_threads_CompatThread *thread = compat_threads_priority_current_thread(which, who);
    if (!thread) return -1;

    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    const int nice_value = thread->nice_value;
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return nice_value;
}

int ssrnx_bionic_setpriority(int which, int who, int prio) {
    compat_threads_CompatThread *thread = compat_threads_priority_current_thread(which, who);
    if (!thread) return -1;

    /* Linux silently clamps setpriority() input to the public nice range. */
    if (prio < SSRNX_ANDROID_NICE_MIN) prio = SSRNX_ANDROID_NICE_MIN;
    if (prio > SSRNX_ANDROID_NICE_MAX) prio = SSRNX_ANDROID_NICE_MAX;

    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    const int old_nice = thread->nice_value;
    pthread_mutex_unlock(&compat_threads_g_registry_lock);

    if (compat_threads_apply_current_android_nice(thread, prio) != 0) {
        const int backend_errno = errno;
        /* Android permits an application to lower its own scheduling priority.
         * If Horizon cannot represent that exact nice value, preserve the
         * Android-visible state rather than leaking a host scheduler policy
         * failure into java.os.Process as a bogus invalid-TID exception.
         * Raising priority remains permission-sensitive. */
        if (prio < old_nice) {
            errno = backend_errno;
            return -1;
        }
        if (ssrnx_log_category) SSRNX_LOGCW("THREAD",
                    "setpriority backend approximation tid=%d nice=%d hostErrno=%d",
                    thread->android_tid, prio, backend_errno);
    }

    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    thread->nice_value = prio;
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    errno = 0;
    return 0;
}

static uint64_t compat_threads_android_cpu_mask_load(size_t cpusetsize, const void *mask) {
    const uint8_t *bytes = (const uint8_t *)mask;
    const size_t n = cpusetsize < sizeof(uint64_t) ? cpusetsize : sizeof(uint64_t);
    uint64_t value = 0;
    for (size_t i = 0; i < n; ++i) value |= (uint64_t)bytes[i] << (i * 8u);
    return value;
}

#ifdef __SWITCH__
static Handle compat_threads_thread_handle_for_android_tid(int pid, pthread_t *host) {
    if (pid == 0 || pid == compat_threads_current_android_tid_value()) {
        if (host) *host = pthread_self();
        return CUR_THREAD_HANDLE;
    }
    Handle handle = INVALID_HANDLE;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    for (compat_threads_CompatThread *t = compat_threads_g_threads; t; t = t->next) {
        if (t->android_tid == pid && !t->exited && t->host_handle != INVALID_HANDLE) {
            handle = t->host_handle;
            if (host) *host = t->host;
            break;
        }
    }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    if (handle == INVALID_HANDLE) errno = ESRCH;
    return handle;
}
#endif

int ssrnx_bionic_sched_getaffinity(int pid, size_t cpusetsize, void *mask) {
    if (!mask) { errno = EFAULT; return -1; }
    if (cpusetsize < sizeof(uint64_t)) { errno = EINVAL; return -1; }
#ifdef __SWITCH__
    const Handle handle = compat_threads_thread_handle_for_android_tid(pid, NULL);
    if (handle == INVALID_HANDLE) return -1;
    s32 ideal = -1;
    u64 affinity = 0;
    const Result rc = svcGetThreadCoreMask(&ideal, &affinity, handle);
    if (R_FAILED(rc)) {
        if (ssrnx_log_category) SSRNX_LOGCW("THREAD", "sched_getaffinity pid=%d handle=0x%x hosRc=0x%08x",
                                           pid, (unsigned)handle, (unsigned)rc);
        errno = ESRCH;
        return -1;
    }
    memset(mask, 0, cpusetsize);
    memcpy(mask, &affinity, sizeof(affinity));
    return 0;
#else
    const int current_tid = compat_threads_current_android_tid_value();
    if (pid != 0 && pid != current_tid) { errno = ESRCH; return -1; }
    return sched_getaffinity(0, cpusetsize, mask);
#endif
}

int ssrnx_bionic_sched_setaffinity(int pid, size_t cpusetsize, const void *mask) {
    if (!mask) { errno = EFAULT; return -1; }
    if (cpusetsize == 0) { errno = EINVAL; return -1; }
    const uint64_t requested = compat_threads_android_cpu_mask_load(cpusetsize, mask);
    if (requested == 0) { errno = EINVAL; return -1; }

#ifdef __SWITCH__
    uint64_t process_mask = 0;
    if (R_FAILED(svcGetInfo(&process_mask, InfoType_CoreMask, CUR_PROCESS_HANDLE, 0)) || process_mask == 0) {
        errno = EINVAL;
        return -1;
    }
    const uint32_t allowed = (uint32_t)(requested & process_mask & UINT32_MAX);
    if (allowed == 0) { errno = EINVAL; return -1; }

    pthread_t target;
    const Handle handle = compat_threads_thread_handle_for_android_tid(pid, &target);
    if (handle == INVALID_HANDLE) return -1;

    /* Keep the thread's current ideal core when the new mask still allows it;
     * otherwise spread across the mask (see thread_policy_core_in_mask_locked). */
    SSRNX_PERF_LOCK(&compat_threads_g_core_policy_lock);
    compat_threads_thread_policy_init_locked();
    const uint64_t effective = compat_threads_thread_policy_affinity_mask_locked(allowed, target);
    s32 current_ideal = -1;
    u64 current_mask = 0;
    int ideal = R_SUCCEEDED(svcGetThreadCoreMask(&current_ideal, &current_mask, handle)) &&
                current_ideal >= 0 && current_ideal < 64 && (effective & (UINT64_C(1) << current_ideal))
        ? current_ideal : compat_threads_thread_policy_core_in_mask_locked(effective);
    pthread_mutex_unlock(&compat_threads_g_core_policy_lock);
    if (ideal < 0 || ideal > 3) { errno = EINVAL; return -1; }
    const Result rc = compat_threads_thread_policy_set_core_mask(handle, ideal, effective,
                                                 "guest-request", pid, requested);
    if (R_FAILED(rc)) { errno = EINVAL; return -1; }
    return 0;
#else
    /* Host regressions validate Linux argument/error semantics. Production
     * Switch builds apply the requested mask through Horizon above. */
    const int current_tid = compat_threads_current_android_tid_value();
    if (pid != 0 && pid != current_tid) { errno = ESRCH; return -1; }
    return sched_setaffinity(0, cpusetsize, mask);
#endif
}

SsrnxGuestPthread ssrnx_bionic_pthread_self(void) {
    /* pthread_self() is async-signal-safe: a handler running for a stopped
     * thread must not join or free threads here. */
    if (!compat_threads_g_signal_proxy_depth) compat_threads_reap_detached_threads();
    compat_threads_CompatThread *thread = compat_threads_adopt_current_thread();
    return thread ? thread->guest_id : 0;
}

int ssrnx_bionic_pthread_equal(SsrnxGuestPthread a, SsrnxGuestPthread b) {
    return a == b;
}

int ssrnx_bionic_pthread_setname_np(SsrnxGuestPthread guest_thread, const char *name) {
    if (!guest_thread || !name) return EINVAL;
    /* Linux/Bionic task names are limited to 16 bytes including NUL. */
    if (strlen(name) >= 16u) return ERANGE;
    compat_threads_reap_detached_threads();
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatThread *thread = compat_threads_find_thread_locked(guest_thread);
    if (!thread) { pthread_mutex_unlock(&compat_threads_g_registry_lock); return ESRCH; }
    const bool changed = strcmp(thread->name, name) != 0;
    memcpy(thread->name, name, strlen(name) + 1u);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    if (ssrnx_log_category && (changed || (ssrnx_diag_full_trace_enabled && ssrnx_diag_full_trace_enabled()))) SSRNX_LOGCI("THREAD", "pthread_setname guest=%llu name=%s",
                (unsigned long long)guest_thread, name);
    /* SSRNX virtualizes pthread_t, so the guest handle cannot be forwarded to
     * the host pthread API. The Android-visible task name remains real guest
     * state and is shared with prctl(PR_{SET,GET}_NAME). */
    return 0;
}

int ssrnx_bionic_pthread_getname_np(SsrnxGuestPthread guest_thread, char *name, size_t len) {
    if (!guest_thread || !name) return EINVAL;
    if (len < 16u) return ERANGE;
    compat_threads_reap_detached_threads();
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatThread *thread = compat_threads_find_thread_locked(guest_thread);
    if (!thread) { pthread_mutex_unlock(&compat_threads_g_registry_lock); return ESRCH; }
    if (thread->name[0]) memcpy(name, thread->name, 16u);
    else { memset(name, 0, 16u); memcpy(name, "apn64-guest", 11u); }
    name[15] = '\0';
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return 0;
}

int ssrnx_bionic_pthread_gettid_np(SsrnxGuestPthread guest_thread) {
    if (!guest_thread) { errno = EINVAL; return -1; }
    compat_threads_reap_detached_threads();
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatThread *thread = compat_threads_find_thread_locked(guest_thread);
    if (!thread) { pthread_mutex_unlock(&compat_threads_g_registry_lock); errno = ESRCH; return -1; }
    const int tid = thread->android_tid > 0 ? thread->android_tid : 1;
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return tid;
}

int ssrnx_bionic_pthread_getschedparam(SsrnxGuestPthread guest_thread, int *policy, struct sched_param *param) {
    if (!guest_thread || !policy || !param) return EINVAL;
    compat_threads_reap_detached_threads();
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatThread *thread = compat_threads_find_thread_locked(guest_thread);
    if (!thread) { pthread_mutex_unlock(&compat_threads_g_registry_lock); return ESRCH; }
    *policy = thread->sched_policy;
    memset(param, 0, sizeof(*param));
    param->sched_priority = thread->sched_priority;
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return 0;
}

static int compat_threads_validate_guest_schedparam(int policy, int priority) {
    /* Android application threads overwhelmingly use SCHED_OTHER(0)/priority 0.
     * Keep unsupported real-time scheduling fail-closed rather than reporting a
     * priority the Horizon backend did not actually apply. */
    if (policy == 0) return priority == 0 ? 0 : EINVAL;
    if (policy == 1 || policy == 2) return EPERM; /* FIFO / RR need privileges. */
    return EINVAL;
}

int ssrnx_bionic_pthread_setschedparam(SsrnxGuestPthread guest_thread, int policy,
                                       const struct sched_param *param) {
    if (!guest_thread || !param) return EINVAL;
    const int valid = compat_threads_validate_guest_schedparam(policy, param->sched_priority);
    if (valid != 0) return valid;
    compat_threads_reap_detached_threads();
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatThread *thread = compat_threads_find_thread_locked(guest_thread);
    if (!thread) { pthread_mutex_unlock(&compat_threads_g_registry_lock); return ESRCH; }
    thread->sched_policy = policy;
    thread->sched_priority = param->sched_priority;
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return 0;
}

int ssrnx_bionic_pthread_setschedprio(SsrnxGuestPthread guest_thread, int priority) {
    if (!guest_thread) return EINVAL;
    compat_threads_reap_detached_threads();
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatThread *thread = compat_threads_find_thread_locked(guest_thread);
    if (!thread) { pthread_mutex_unlock(&compat_threads_g_registry_lock); return ESRCH; }
    const int valid = compat_threads_validate_guest_schedparam(thread->sched_policy, priority);
    if (valid == 0) thread->sched_priority = priority;
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return valid;
}

int ssrnx_bionic_thread_set_current_name(const char *name) {
    SsrnxGuestPthread self = ssrnx_bionic_pthread_self();
    int rc = ssrnx_bionic_pthread_setname_np(self, name);
    if (rc != 0) { errno = rc; return -1; }
    return 0;
}

int ssrnx_bionic_thread_get_current_name(char out[16]) {
    if (!out) { errno = EFAULT; return -1; }
    compat_threads_CompatThread *thread = compat_threads_adopt_current_thread();
    if (!thread) { errno = ENOMEM; return -1; }
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    if (thread->name[0]) memcpy(out, thread->name, sizeof(thread->name));
    else { memset(out, 0, 16); memcpy(out, "apn64-guest", 11); }
    out[15] = '\0';
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return 0;
}

int ssrnx_bionic_pthread_getattr_np(SsrnxGuestPthread guest_thread, void *attr) {
    if (!guest_thread || !attr) return EINVAL;
    compat_threads_reap_detached_threads();

    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatThread *thread = compat_threads_find_thread_locked(guest_thread);
    if (!thread) {
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        return ESRCH;
    }
    while (!thread->stack_ready && !thread->exited)
        pthread_cond_wait(&compat_threads_g_thread_state_cond, &compat_threads_g_registry_lock);
    if (!thread->stack_ready || thread->stack_capture_rc != 0) {
        const int rc = thread->stack_capture_rc ? thread->stack_capture_rc : ENOTSUP;
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        return ssrnx_errno_host_to_android(rc);
    }

    SsrnxBionicPthreadAttrLayout out;
    memset(&out, 0, sizeof(out));
    if (thread->detached) out.flags |= SSRNX_BIONIC_PTHREAD_ATTR_DETACHED;
    out.stack_base = thread->stack_base;
    out.stack_size = thread->stack_size;
    out.guard_size = thread->guard_size;
    out.sched_policy = thread->sched_policy;
    out.sched_priority = thread->sched_priority;
    out.flags |= thread->attr_flags & (SSRNX_BIONIC_PTHREAD_ATTR_INHERIT | SSRNX_BIONIC_PTHREAD_ATTR_EXPLICIT);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);

    memcpy(attr, &out, sizeof(out));
    return 0;
}

void ssrnx_bionic_pthread_exit(void *retval) {
    compat_threads_ensure_internal_keys();
    compat_threads_CompatThread *self = (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
    if (ssrnx_bionic_cxa_thread_finalize_current) ssrnx_bionic_cxa_thread_finalize_current();
    compat_threads_guest_cleanup_run_all(self);
    if (self) compat_threads_signal_thread_exit(self);
    compat_threads_guest_keys_thread_exit();
    if (ssrnx_heap_cache_thread_exit) ssrnx_heap_cache_thread_exit();
    compat_threads_mark_current_thread_exit(retval);
    pthread_exit(retval);
}

size_t ssrnx_bionic_pthread_live_guest_count(void) {
    compat_threads_reap_detached_threads();
    size_t count = 0;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    for (compat_threads_CompatThread *it = compat_threads_g_threads; it; it = it->next) {
        if (!it->external && !it->exited) count++;
    }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return count;
}

bool ssrnx_bionic_pthread_wait_guest_idle(uint32_t timeout_ms) {
    struct timespec start = {0, 0};
    (void)clock_gettime(CLOCK_MONOTONIC, &start);
    for (;;) {
        if (ssrnx_bionic_pthread_live_guest_count() == 0) return true;
        struct timespec now = {0, 0};
        (void)clock_gettime(CLOCK_MONOTONIC, &now);
        int64_t sec = (int64_t)now.tv_sec - (int64_t)start.tv_sec;
        int64_t nsec = (int64_t)now.tv_nsec - (int64_t)start.tv_nsec;
        if (nsec < 0) { sec--; nsec += INT64_C(1000000000); }
        const uint64_t elapsed_ms = sec > 0
            ? (uint64_t)sec * UINT64_C(1000) + (uint64_t)nsec / UINT64_C(1000000)
            : (uint64_t)nsec / UINT64_C(1000000);
        if (elapsed_ms >= timeout_ms) return false;
        struct timespec nap = {0, 10000000L};
        (void)nanosleep(&nap, NULL);
    }
}


void ssrnx_bionic_thread_activity_enter(SsrnxThreadActivity *saved, const char *kind,
                                        const char *class_name, const char *method_name) {
    if (!saved) return;
    saved->since_ms = 0;
    compat_threads_ensure_internal_keys();
    compat_threads_CompatThread *self = (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
    if (!self) return;
    saved->since_ms = __atomic_load_n(&self->activity_since_ms, __ATOMIC_ACQUIRE);
    memcpy(saved->text, self->activity, sizeof(saved->text));
    /* Readers skip the text while since is 0, so it is never read half-written. */
    __atomic_store_n(&self->activity_since_ms, 0, __ATOMIC_RELEASE);
    snprintf(self->activity, sizeof(self->activity), "%s %s.%s", kind ? kind : "call",
             class_name ? class_name : "?", method_name ? method_name : "?");
    __atomic_store_n(&self->activity_since_ms, compat_threads_wait_clock_ms(), __ATOMIC_RELEASE);
}

void ssrnx_bionic_thread_activity_leave(const SsrnxThreadActivity *saved) {
    if (!saved) return;
    compat_threads_CompatThread *self = (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
    if (!self) return;
    __atomic_store_n(&self->activity_since_ms, 0, __ATOMIC_RELEASE);
    if (!saved->since_ms) return;
    memcpy(self->activity, saved->text, sizeof(self->activity));
    __atomic_store_n(&self->activity_since_ms, saved->since_ms, __ATOMIC_RELEASE);
}

bool ssrnx_bionic_current_stack_bounds(uintptr_t *out_low, uintptr_t *out_high) {
    compat_threads_ensure_internal_keys();
    const compat_threads_CompatThread *self = (const compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
    if (!self || !self->stack_ready || self->stack_capture_rc != 0 || !self->stack_base || !self->stack_size)
        return false;
    *out_low = (uintptr_t)self->stack_base;
    *out_high = (uintptr_t)self->stack_base + self->stack_size;
    return true;
}

typedef struct compat_threads_WaitSnapshot {
    uint64_t guest_id;
    int android_tid;
    uintptr_t start;
    char name[16];
    int external;
    const char *kind;
    const void *object;
    uintptr_t caller;
    uint64_t since_ms;
    char activity[SSRNX_THREAD_ACTIVITY_TEXT];
    uint64_t activity_since_ms;
    char condition[256];
    uintptr_t futex_frames[24];
} compat_threads_WaitSnapshot;

void ssrnx_bionic_pthread_diag_summary(SsrnxThreadDiagSummary *out_summary) {
    if (!out_summary) return;
    memset(out_summary, 0, sizeof(*out_summary));
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    for (compat_threads_CompatThread *it = compat_threads_g_threads; it; it = it->next) {
        if (it->exited) continue;
        out_summary->live++;
        if (__atomic_load_n(&it->wait_since_ms, __ATOMIC_ACQUIRE))
            out_summary->waiting++;
        if (__atomic_load_n(&it->activity_since_ms, __ATOMIC_ACQUIRE))
            out_summary->active++;
    }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
}

size_t ssrnx_bionic_thread_samples(SsrnxThreadSample *out, size_t cap) {
    if (!out || !cap) return 0;
    size_t n = 0;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    for (compat_threads_CompatThread *it = compat_threads_g_threads; it && n < cap; it = it->next) {
        if (it->exited) continue;
        SsrnxThreadSample *s = &out[n++];
        s->host = it->host;
        s->guest_id = it->guest_id;
        memcpy(s->name, it->name, sizeof(s->name) < sizeof(it->name) ? sizeof(s->name) : sizeof(it->name));
        s->name[sizeof(s->name) - 1] = '\0';
        s->waiting = __atomic_load_n(&it->wait_since_ms, __ATOMIC_ACQUIRE) != 0;
        s->cpu_ticks = 0;
        s->cpu_valid = false;
#ifdef __SWITCH__
        u64 ticks = 0;
        if (it->host_handle != INVALID_HANDLE &&
            R_SUCCEEDED(svcGetInfo(&ticks, InfoType_ThreadTickCount, it->host_handle, UINT64_MAX))) {
            s->cpu_ticks = ticks;
            s->cpu_valid = true;
        }
#elif defined(__linux__)
        clockid_t cpu_clock;
        struct timespec cpu_time;
        if (pthread_getcpuclockid(it->host, &cpu_clock) == 0 &&
            clock_gettime(cpu_clock, &cpu_time) == 0) {
            s->cpu_ticks = (uint64_t)cpu_time.tv_sec * UINT64_C(1000000000) + (uint64_t)cpu_time.tv_nsec;
            s->cpu_valid = true;
        }
#endif
    }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return n;
}

bool ssrnx_bionic_thread_scheduling(pthread_t host, SsrnxThreadScheduling *out) {
    if (!out) return false;
    memset(out, 0, sizeof(*out));
    out->ideal_core = out->host_priority = -1;
    out->core_result = out->priority_result = ENOSYS;
    bool found = false;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    for (compat_threads_CompatThread *it = compat_threads_g_threads; it; it = it->next) {
        if (it->exited || !pthread_equal(it->host, host)) continue;
        found = true;
        out->guest_id = it->guest_id;
        out->android_tid = it->android_tid;
#ifdef __SWITCH__
        if (it->host_handle != INVALID_HANDLE) {
            s32 ideal = -1, priority = -1;
            u64 mask = 0;
            out->core_result = svcGetThreadCoreMask(&ideal, &mask, it->host_handle);
            out->priority_result = svcGetThreadPriority(&priority, it->host_handle);
            if (R_SUCCEEDED(out->core_result)) { out->ideal_core = ideal; out->effective_mask = mask; }
            if (R_SUCCEEDED(out->priority_result)) out->host_priority = priority;
        }
#endif
        break;
    }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return found;
}

static void compat_threads_signal_log_wait_states(const char *reason);
static void compat_threads_futex_log_wait_detail(const char *reason, const compat_threads_WaitSnapshot *snapshot);
static void compat_threads_futex_capture_frames(compat_threads_CompatThread *self, const void *frame);

/* Registry lock protects sidecar/thread lifetime; observe the backend word,
 * never acquire the guest mutex or turn a Horizon handle into a guest ID. */
static void compat_threads_mutex_diag_format_locked(const void *guest, uint64_t waiter, char *out, size_t cap) {
    const compat_threads_CompatMutex *mutex = compat_threads_find_mutex_locked((void *)guest);
    if (!mutex) { snprintf(out, cap, " mutexBackend=missing"); return; }
    if (compat_threads_mutex_uses_state(mutex)) {
        const uint32_t state = __atomic_load_n(&mutex->state, __ATOMIC_ACQUIRE);
        const uint64_t acquirer = __atomic_load_n(&mutex->acquirer, __ATOMIC_RELAXED);
        const uint32_t depth = __atomic_load_n(&mutex->depth, __ATOMIC_RELAXED);
        snprintf(out, cap, " mutexBackend=bionic-state mutexType=%d mutexState=%u mutexAcquirerThread=%llu mutexSelfAcquired=%u mutexDepth=%u",
                 mutex->type, state, (unsigned long long)acquirer,
                 state != 0 && acquirer != 0 && acquirer == waiter, depth);
        return;
    }
#ifdef __SWITCH__
    const int type = mutex->host.type;
    const uint32_t word = __atomic_load_n(&mutex->host.normal, __ATOMIC_ACQUIRE);
    const Handle handle = word & ~UINT32_C(0x40000000); /* libnx HANDLE_WAIT_MASK */
    const unsigned depth = type == PTHREAD_MUTEX_RECURSIVE ?
        __atomic_load_n(&mutex->host.recursive.counter, __ATOMIC_RELAXED) : 0;
    uint64_t owner = 0;
    bool exited = false;
    if (handle != INVALID_HANDLE) {
        for (const compat_threads_CompatThread *it = compat_threads_g_threads; it; it = it->next) {
            if (it->host_handle != handle) continue;
            owner = (uint64_t)it->guest_id;
            exited = it->exited;
            break;
        }
    }
    snprintf(out, cap, " mutexHostType=%d mutexHostWord=%#x mutexOwnerHandle=%#x mutexOwnerThread=%llu mutexOwnerExited=%u mutexSelfOwned=%u mutexDepth=%u",
             type, word, handle, (unsigned long long)owner, exited,
             owner != 0 && owner == waiter, depth);
#else
    (void)guest; (void)waiter;
    snprintf(out, cap, " mutexBackend=unavailable");
#endif
}

static void compat_threads_wait_log_frames(const char *kind, const compat_threads_WaitSnapshot *s) {
    for (size_t i = 0; i < sizeof(s->futex_frames) / sizeof(s->futex_frames[0]) && s->futex_frames[i]; ++i) {
        char site[192];
        if (ssrnx_compat_describe_guest_address)
            (void)ssrnx_compat_describe_guest_address(s->futex_frames[i], site, sizeof(site));
        else snprintf(site, sizeof(site), "0x%llx", (unsigned long long)s->futex_frames[i]);
        ssrnx_log_category("THREAD", "INFO", "stall-%s-frame thread=%llu frame=%zu pc=%#llx caller=%s",
            kind, (unsigned long long)s->guest_id, i, (unsigned long long)s->futex_frames[i], site);
    }
}

size_t ssrnx_bionic_pthread_log_wait_states(const char *reason) {
    enum { MAX_SNAPSHOTS = 64 };
    compat_threads_WaitSnapshot snaps[MAX_SNAPSHOTS];
    size_t count = 0, live = 0;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    for (compat_threads_CompatThread *it = compat_threads_g_threads; it; it = it->next) {
        if (it->exited) continue;
        live++;
        /* Keep both new workers and the oldest live threads. Keeping only the
         * newest 64 hid the Activity thread during crowded startup captures. */
        size_t slot=count;
        if(count<MAX_SNAPSHOTS)count++;
        else slot=MAX_SNAPSHOTS/2+(live-MAX_SNAPSHOTS-1)%(MAX_SNAPSHOTS/2);
        compat_threads_WaitSnapshot *s = &snaps[slot];
        s->guest_id = (uint64_t)it->guest_id;
        s->android_tid = it->android_tid;
        s->start = (uintptr_t)it->guest_start;
        memcpy(s->name, it->name, sizeof(s->name));
        s->name[sizeof(s->name) - 1] = '\0';
        s->external = it->external;
        s->since_ms = __atomic_load_n(&it->wait_since_ms, __ATOMIC_ACQUIRE);
        s->kind = it->wait_kind;
        s->object = it->wait_object;
        s->caller = it->wait_caller;
        for (size_t frame = 0; frame < sizeof(s->futex_frames) / sizeof(s->futex_frames[0]); ++frame)
            s->futex_frames[frame] = __atomic_load_n(&it->futex_frames[frame], __ATOMIC_RELAXED);
        s->condition[0] = '\0';
        if (s->since_ms && s->kind &&
            (!strcmp(s->kind, "cond_wait") || !strcmp(s->kind, "cond_timedwait"))) {
            const compat_threads_CompatCond *cond = compat_threads_find_cond_locked((void *)s->object);
            if (cond) compat_threads_cond_diag_format(cond, s->condition, sizeof(s->condition));
        }
        if (s->since_ms && s->kind && !strcmp(s->kind, "mutex_lock"))
            compat_threads_mutex_diag_format_locked(s->object, s->guest_id, s->condition, sizeof(s->condition));
        /* The owner rewrites the text only while since is 0; a changed since
         * means the copy may be torn and is dropped. */
        const uint64_t activity_since = __atomic_load_n(&it->activity_since_ms, __ATOMIC_ACQUIRE);
        memcpy(s->activity, it->activity, sizeof(s->activity));
        s->activity[sizeof(s->activity) - 1] = '\0';
        s->activity_since_ms =
            activity_since == __atomic_load_n(&it->activity_since_ms, __ATOMIC_ACQUIRE) ? activity_since : 0;
    }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    if (!ssrnx_log_category) return live;

    const uint64_t now = compat_threads_wait_clock_ms();
    size_t waiting = 0;
    for (size_t i = 0; i < count; ++i) {
        const compat_threads_WaitSnapshot *s = &snaps[i];
        char start[192] = {0};
        if (s->start && ssrnx_compat_describe_guest_address)
            (void)ssrnx_compat_describe_guest_address(s->start, start, sizeof(start));
        if (!s->external)
            ssrnx_log_category("THREAD", "INFO", "stall-origin thread=%llu tid=%d startPc=%p start=%s",
                               (unsigned long long)s->guest_id, s->android_tid, (void *)s->start, start);
        char activity[SSRNX_THREAD_ACTIVITY_TEXT + 48] = "";
        if (s->activity_since_ms)
            snprintf(activity, sizeof(activity), " activity=[%s] activityMs=%llu", s->activity,
                     (unsigned long long)(now > s->activity_since_ms ? now - s->activity_since_ms : 0));
        if (!s->since_ms) {
            ssrnx_log_category("THREAD", "INFO", "stall-report reason=%s thread=%llu name=%s kind=%s state=running%s",
                               reason ? reason : "-", (unsigned long long)s->guest_id,
                               s->name[0] ? s->name : "-", s->external ? "attached" : "guest", activity);
            continue;
        }
        waiting++;
        char site[192];
        if (ssrnx_compat_describe_guest_address)
            (void)ssrnx_compat_describe_guest_address(s->caller, site, sizeof(site));
        else
            snprintf(site, sizeof(site), "0x%llx", (unsigned long long)s->caller);
        ssrnx_log_category("THREAD", "INFO",
                           "stall-report reason=%s thread=%llu name=%s kind=%s wait=%s object=%p elapsedMs=%llu caller=%s%s%s",
                           reason ? reason : "-", (unsigned long long)s->guest_id,
                           s->name[0] ? s->name : "-", s->external ? "attached" : "guest",
                           s->kind ? s->kind : "?", s->object,
                           (unsigned long long)(now > s->since_ms ? now - s->since_ms : 0), site, activity, s->condition);
        if (s->kind && !strcmp(s->kind, "futex") &&
            ((now > s->since_ms && now - s->since_ms >= 5000u) ||
             (s->activity_since_ms && now > s->activity_since_ms && now - s->activity_since_ms >= 5000u)))
            compat_threads_futex_log_wait_detail(reason, s);
        if (s->kind && !strcmp(s->kind, "mutex_lock") && now > s->since_ms && now - s->since_ms >= 5000u)
            compat_threads_wait_log_frames("mutex", s);
    }
    ssrnx_log_category("THREAD", "INFO", "stall-report reason=%s live=%zu reported=%zu waiting=%zu omitted=%zu",
                       reason ? reason : "-", live, count, waiting, live-count);
    compat_threads_signal_log_wait_states(reason);
    return live;
}

int ssrnx_bionic_pthread_mutex_init(void *guest_mutex, const long *guest_attr) {
    if (!guest_mutex) return PTHREAD_SETUP_FAIL("pthread_mutex_init",guest_mutex,guest_attr,EINVAL);
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    if (compat_threads_find_mutex_locked(guest_mutex)) {
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        return PTHREAD_SETUP_FAIL("pthread_mutex_init",guest_mutex,guest_attr,EBUSY);
    }
    int type = 0;
    int protocol = 0;
    if (guest_attr) {
        type = (int)(*guest_attr & 0x0fL);
        protocol = (int)((*guest_attr & 0x20L) >> 5);
        if (type < 0 || type > 2 || protocol < 0 || protocol > 1 || (*guest_attr & ~0x3fL) != 0) {
            pthread_mutex_unlock(&compat_threads_g_registry_lock);
            return PTHREAD_SETUP_FAIL("pthread_mutex_init",guest_mutex,guest_attr,EINVAL);
        }
    }
    /* PI retains Horizon arbitration; non-PI uses Android state below. */
    compat_threads_CompatMutex *entry = compat_threads_create_mutex_locked(guest_mutex, type, protocol);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    if (!entry) return PTHREAD_SETUP_FAIL("pthread_mutex_init",guest_mutex,guest_attr,ENOMEM);
    memset(guest_mutex, 0, 40); /* Bionic LP64 pthread_mutex_t size. */
    return 0;
}

int ssrnx_bionic_pthread_mutex_destroy(void *guest_mutex) {
    if (!guest_mutex) return EINVAL;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatMutex **link = &compat_threads_g_mutexes[compat_threads_sync_bucket(guest_mutex)];
    while (*link && (*link)->guest != guest_mutex) link = &(*link)->next;
    compat_threads_CompatMutex *entry = *link;
    if (!entry) {
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        return 0;
    }
    if (compat_threads_mutex_uses_state(entry) && __atomic_load_n(&entry->state, __ATOMIC_ACQUIRE) != 0) {
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        return EBUSY;
    }
    const int rc = pthread_mutex_destroy(&entry->host);
    if (rc == 0) {
        *link = entry->next;
        free(entry);
    }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return rc;
}

/* Normal unlock has no owner check; recursive/errorcheck explicitly do.
 * Keep those Android rules above Horizon's ownership-enforcing primitives. */
static int compat_threads_state_mutex_trylock(compat_threads_CompatMutex *mutex) {
    compat_threads_CompatThread *self = compat_threads_adopt_current_thread();
    const uint64_t id = self ? (uint64_t)self->guest_id : 0;
    if (mutex->type != 0 && id != 0 &&
        __atomic_load_n(&mutex->acquirer, __ATOMIC_ACQUIRE) == id) {
        if (mutex->type == 2) return EBUSY;
        const uint32_t depth = __atomic_load_n(&mutex->depth, __ATOMIC_RELAXED);
        if (depth == 2048) return EAGAIN; /* Bionic's 11-bit recursion counter. */
        __atomic_store_n(&mutex->depth, depth + 1, __ATOMIC_RELAXED);
        return 0;
    }
    uint32_t expected = 0;
    if (!__atomic_compare_exchange_n(&mutex->state, &expected, 1, false,
                                      __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) return EBUSY;
    __atomic_store_n(&mutex->depth, 1, __ATOMIC_RELAXED);
    __atomic_store_n(&mutex->acquirer, id, __ATOMIC_RELEASE);
    return 0;
}

static int compat_threads_state_mutex_unlock(compat_threads_CompatMutex *mutex) {
    if (mutex->type != 0) {
        compat_threads_CompatThread *self = compat_threads_adopt_current_thread();
        if (!self || __atomic_load_n(&mutex->acquirer, __ATOMIC_ACQUIRE) != (uint64_t)self->guest_id)
            return EPERM;
        const uint32_t depth = __atomic_load_n(&mutex->depth, __ATOMIC_RELAXED);
        if (mutex->type == 1 && depth > 1) {
            __atomic_store_n(&mutex->depth, depth - 1, __ATOMIC_RELAXED);
            return 0;
        }
    }
    __atomic_store_n(&mutex->depth, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&mutex->acquirer, 0, __ATOMIC_RELEASE);
    if (__atomic_exchange_n(&mutex->state, 0, __ATOMIC_RELEASE) == 2)
        compat_threads_addr_wake(&mutex->state, 1);
    return 0;
}

static int compat_threads_state_mutex_lock(compat_threads_CompatMutex *mutex) {
    const int rc = compat_threads_state_mutex_trylock(mutex);
    if (rc != EBUSY) {
        compat_threads_t_wait_kind = NULL; compat_threads_t_wait_object = NULL; compat_threads_t_wait_caller = 0;
        return rc;
    }
    const char *kind = compat_threads_t_wait_kind;
    const void *object = compat_threads_t_wait_object;
    const uintptr_t caller = compat_threads_t_wait_caller;
    compat_threads_CompatThread *self = compat_threads_adopt_current_thread();
    if (mutex->type == 2 && self &&
        __atomic_load_n(&mutex->acquirer, __ATOMIC_ACQUIRE) == (uint64_t)self->guest_id) {
        compat_threads_t_wait_kind = NULL; compat_threads_t_wait_object = NULL; compat_threads_t_wait_caller = 0;
        return EDEADLK;
    }
    compat_threads_futex_capture_frames((compat_threads_g_signal_proxy_depth || compat_threads_g_signal_target_delivery_depth) ? NULL : self,
                         __builtin_frame_address(0));
    while (__atomic_exchange_n(&mutex->state, 2, __ATOMIC_ACQUIRE) != 0) {
        compat_threads_t_wait_kind = kind; compat_threads_t_wait_object = object; compat_threads_t_wait_caller = caller;
        compat_threads_signal_safe_addr_wait(&mutex->state, 2, -1);
    }
    __atomic_store_n(&mutex->depth, 1, __ATOMIC_RELAXED);
    __atomic_store_n(&mutex->acquirer, self ? (uint64_t)self->guest_id : 0, __ATOMIC_RELEASE);
    compat_threads_t_wait_kind = NULL; compat_threads_t_wait_object = NULL; compat_threads_t_wait_caller = 0;
    return 0;
}

int ssrnx_bionic_pthread_mutex_lock(void *guest_mutex) {
    compat_threads_reap_detached_threads();
    compat_threads_CompatMutex *entry = compat_threads_get_mutex(guest_mutex, 1);
    if (!entry) return ENOMEM;
    /* Uncontended (or recursive) acquisition never blocks; a contended one is
     * a signal safe point. */
    const int rc = compat_threads_mutex_uses_state(entry) ? compat_threads_state_mutex_trylock(entry) : pthread_mutex_trylock(&entry->host);
    if (rc != EBUSY) return ssrnx_errno_host_to_android(rc);
    GUEST_WAIT_SITE("mutex_lock", guest_mutex);
    compat_threads_ensure_internal_keys();
    compat_threads_CompatThread *self = (compat_threads_g_signal_proxy_depth || compat_threads_g_signal_target_delivery_depth) ? NULL
                        : (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
    compat_threads_futex_capture_frames(self, __builtin_frame_address(0));
    return ssrnx_errno_host_to_android(compat_threads_mutex_uses_state(entry) ? compat_threads_state_mutex_lock(entry) : compat_threads_signal_safe_mutex_lock(&entry->host));
}

int ssrnx_bionic_pthread_mutex_trylock(void *guest_mutex) {
    compat_threads_CompatMutex *entry = compat_threads_get_mutex(guest_mutex, 1);
    return entry ? ssrnx_errno_host_to_android(compat_threads_mutex_uses_state(entry) ? compat_threads_state_mutex_trylock(entry) : pthread_mutex_trylock(&entry->host)) : ENOMEM;
}

int ssrnx_bionic_pthread_mutex_unlock(void *guest_mutex) {
    compat_threads_CompatMutex *entry = compat_threads_get_mutex(guest_mutex, 0);
    return entry ? ssrnx_errno_host_to_android(compat_threads_mutex_uses_state(entry) ? compat_threads_state_mutex_unlock(entry) : pthread_mutex_unlock(&entry->host)) : EINVAL;
}

/* Bionic arm64 pthread_condattr_t is a long. Bit 0 is pshared and bit 1
 * selects the clock (CLOCK_REALTIME=0, CLOCK_MONOTONIC=1). Keep that ABI
 * explicit instead of casting to the host pthread_condattr_t. */
#define SSRNX_BIONIC_COND_SHARED_MASK 0x1L
#define SSRNX_BIONIC_COND_CLOCK_MASK  0x2L
#define SSRNX_BIONIC_CONDATTR_DESTROYED ((long)0xdeada11dL)

int ssrnx_bionic_pthread_condattr_init(long *guest_attr) {
    if (!guest_attr) return EINVAL;
    *guest_attr = 0; /* PRIVATE + CLOCK_REALTIME */
    return 0;
}

int ssrnx_bionic_pthread_condattr_destroy(long *guest_attr) {
    if (!guest_attr) return EINVAL;
    *guest_attr = SSRNX_BIONIC_CONDATTR_DESTROYED;
    return 0;
}

int ssrnx_bionic_pthread_condattr_getclock(const long *guest_attr, int *clock_id) {
    if (!guest_attr || !clock_id) return EINVAL;
    *clock_id = ((*guest_attr & SSRNX_BIONIC_COND_CLOCK_MASK) != 0) ? 1 : 0;
    return 0;
}

int ssrnx_bionic_pthread_condattr_setclock(long *guest_attr, int clock_id) {
    if (!guest_attr) return EINVAL;
    if (clock_id != 0 && clock_id != 1) return EINVAL;
    *guest_attr = (*guest_attr & ~SSRNX_BIONIC_COND_CLOCK_MASK) |
                  (clock_id ? SSRNX_BIONIC_COND_CLOCK_MASK : 0L);
    return 0;
}

int ssrnx_bionic_pthread_condattr_getpshared(const long *guest_attr, int *pshared) {
    if (!guest_attr || !pshared) return EINVAL;
    *pshared = ((*guest_attr & SSRNX_BIONIC_COND_SHARED_MASK) != 0) ? 1 : 0;
    return 0;
}

int ssrnx_bionic_pthread_condattr_setpshared(long *guest_attr, int pshared) {
    if (!guest_attr) return EINVAL;
    if (pshared != 0 && pshared != 1) return EINVAL;
    if (pshared) *guest_attr |= SSRNX_BIONIC_COND_SHARED_MASK;
    else *guest_attr &= ~SSRNX_BIONIC_COND_SHARED_MASK;
    return 0;
}

int ssrnx_bionic_pthread_cond_init(void *guest_cond, const long *guest_attr) {
    if (!guest_cond) return PTHREAD_SETUP_FAIL("pthread_cond_init",guest_cond,guest_attr,EINVAL);
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    if (compat_threads_find_cond_locked(guest_cond)) {
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        return PTHREAD_SETUP_FAIL("pthread_cond_init",guest_cond,guest_attr,EBUSY);
    }
    compat_threads_CompatCond *entry = compat_threads_create_cond_locked(guest_cond);
    if (entry && guest_attr) {
        entry->guest_clock_id = ((*guest_attr & SSRNX_BIONIC_COND_CLOCK_MASK) != 0) ? 1 : 0;
    }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    if (!entry) return PTHREAD_SETUP_FAIL("pthread_cond_init",guest_cond,guest_attr,ENOMEM);
    memset(guest_cond, 0, 48); /* Bionic LP64 pthread_cond_t size. */
    return 0;
}

int ssrnx_bionic_pthread_cond_destroy(void *guest_cond) {
    if (!guest_cond) return EINVAL;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatCond **link = &compat_threads_g_conds[compat_threads_sync_bucket(guest_cond)];
    while (*link && (*link)->guest != guest_cond) link = &(*link)->next;
    compat_threads_CompatCond *entry = *link;
    if (!entry) {
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        return 0;
    }
    const int rc = pthread_cond_destroy(&entry->host);
    if (rc == 0) {
        *link = entry->next;
        free(entry);
    }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return rc;
}

typedef struct compat_threads_SsrnxGuestTimespec64 {
    int64_t tv_sec;
    int64_t tv_nsec;
} compat_threads_SsrnxGuestTimespec64;

static int compat_threads_ssrnx_timespec64_validate(const compat_threads_SsrnxGuestTimespec64 *gt) {
    return gt && gt->tv_nsec >= 0 && gt->tv_nsec < INT64_C(1000000000);
}

static int compat_threads_ssrnx_timespec_cmp(const struct timespec *a, const struct timespec *b) {
    if (a->tv_sec < b->tv_sec) return -1;
    if (a->tv_sec > b->tv_sec) return 1;
    if (a->tv_nsec < b->tv_nsec) return -1;
    if (a->tv_nsec > b->tv_nsec) return 1;
    return 0;
}

static struct timespec compat_threads_ssrnx_timespec_add_ns(struct timespec base, int64_t ns) {
    if (ns <= 0) return base;
    base.tv_sec += (time_t)(ns / INT64_C(1000000000));
    base.tv_nsec += (long)(ns % INT64_C(1000000000));
    if (base.tv_nsec >= 1000000000L) { base.tv_sec++; base.tv_nsec -= 1000000000L; }
    return base;
}


static int compat_threads_ssrnx_deadline_from_guest(const compat_threads_SsrnxGuestTimespec64 *gt, clockid_t clock_id,
                                     struct timespec *out) {
    if (!gt || !out || !compat_threads_ssrnx_timespec64_validate(gt) || gt->tv_sec < 0) return EINVAL;
    if (clock_id != CLOCK_REALTIME && clock_id != CLOCK_MONOTONIC) return EINVAL;
    out->tv_sec = (time_t)gt->tv_sec;
    out->tv_nsec = (long)gt->tv_nsec;
    return 0;
}

/* Wait on a host CLOCK_REALTIME condvar while honoring an Android absolute
 * deadline in either REALTIME or MONOTONIC. Short REALTIME slices keep a
 * monotonic deadline immune to wall-clock adjustments. */
static int compat_threads_ssrnx_cond_wait_until(pthread_cond_t *cond, pthread_mutex_t *mutex,
                                 const struct timespec *deadline, clockid_t clock_id) {
    if (!deadline) return compat_threads_signal_safe_cond_wait(cond, mutex, NULL);
    if (clock_id == CLOCK_REALTIME) return compat_threads_signal_safe_cond_wait(cond, mutex, deadline);
    if (clock_id != CLOCK_MONOTONIC) return EINVAL;

    const char *wait_kind = compat_threads_t_wait_kind;
    const void *wait_object = compat_threads_t_wait_object;
    const uintptr_t wait_caller = compat_threads_t_wait_caller;
    for (;;) {
        struct timespec mono_now = {0, 0}, real_now = {0, 0};
        if (clock_gettime(CLOCK_MONOTONIC, &mono_now) != 0 ||
            clock_gettime(CLOCK_REALTIME, &real_now) != 0) return EINVAL;
        if (compat_threads_ssrnx_timespec_cmp(&mono_now, deadline) >= 0) return ETIMEDOUT;
        int64_t remaining_ns = ((int64_t)deadline->tv_sec - (int64_t)mono_now.tv_sec) * INT64_C(1000000000) +
                               ((int64_t)deadline->tv_nsec - (int64_t)mono_now.tv_nsec);
        if (remaining_ns > INT64_C(100000000)) remaining_ns = INT64_C(100000000);
        struct timespec host_deadline = compat_threads_ssrnx_timespec_add_ns(real_now, remaining_ns);
        compat_threads_t_wait_kind = wait_kind; compat_threads_t_wait_object = wait_object; compat_threads_t_wait_caller = wait_caller;
        const int rc = compat_threads_signal_safe_cond_wait(cond, mutex, &host_deadline);
        if (rc == 0) return 0;
        if (rc != ETIMEDOUT) return rc;
    }
}

/* Sample the notification generation while still holding the guest mutex.
 * WaitIfEqual closes the release/sleep race without a host ownership handoff. */
static int compat_threads_state_cond_wait(compat_threads_CompatCond *cond, compat_threads_CompatMutex *mutex,
                             const struct timespec *deadline, clockid_t clock_id) {
    const char *kind = compat_threads_t_wait_kind;
    const void *object = compat_threads_t_wait_object;
    const uintptr_t caller = compat_threads_t_wait_caller;
    const uint32_t sequence = __atomic_load_n(&cond->normal_sequence, __ATOMIC_ACQUIRE);
    const int unlock_rc = compat_threads_state_mutex_unlock(mutex);
    if (unlock_rc != 0) return unlock_rc;
    int rc = 0;
    while (__atomic_load_n(&cond->normal_sequence, __ATOMIC_ACQUIRE) == sequence) {
        int64_t timeout_ns = -1;
        if (deadline) {
            struct timespec now;
            if (clock_gettime(clock_id, &now) != 0) { rc = errno ? errno : EINVAL; break; }
            if (compat_threads_ssrnx_timespec_cmp(&now, deadline) >= 0) { rc = ETIMEDOUT; break; }
            /* Bound before converting seconds to nanoseconds. */
            const int64_t seconds = (int64_t)deadline->tv_sec - (int64_t)now.tv_sec;
            timeout_ns = seconds > 1 ? INT64_C(1000000000) :
                seconds * INT64_C(1000000000) + deadline->tv_nsec - now.tv_nsec;
        }
        compat_threads_t_wait_kind = kind; compat_threads_t_wait_object = object; compat_threads_t_wait_caller = caller;
        compat_threads_signal_safe_addr_wait(&cond->normal_sequence, sequence, timeout_ns);
    }
    compat_threads_t_wait_kind = "mutex_lock"; compat_threads_t_wait_object = mutex->guest; compat_threads_t_wait_caller = caller;
    const int lock_rc = compat_threads_state_mutex_lock(mutex);
    if (lock_rc != 0) return lock_rc;
    return rc;
}

static int compat_threads_cond_wait_diagnostic(compat_threads_CompatCond *cond, compat_threads_CompatMutex *mutex, void *guest_mutex,
                                const struct timespec *deadline, int guest_clock_id) {
    const uintptr_t caller = compat_threads_t_wait_caller;
    const uint64_t sequence = __atomic_add_fetch(&cond->wait_count, 1, __ATOMIC_RELAXED);
    const uint64_t started = compat_threads_wait_clock_ms();
    __atomic_add_fetch(&cond->waiters, 1, __ATOMIC_RELAXED);
    compat_threads_cond_diag_log(cond, "begin", guest_mutex, caller, sequence, 0, guest_clock_id, deadline, 0);
    const clockid_t clock_id = guest_clock_id == 1 ? CLOCK_MONOTONIC : CLOCK_REALTIME;
    const int rc = compat_threads_mutex_uses_state(mutex) ? compat_threads_state_cond_wait(cond, mutex, deadline, clock_id) :
        compat_threads_ssrnx_cond_wait_until(&cond->host, &mutex->host, deadline, clock_id);
    __atomic_sub_fetch(&cond->waiters, 1, __ATOMIC_RELAXED);
    if (rc == 0) __atomic_add_fetch(&cond->wake_count, 1, __ATOMIC_RELAXED);
    if (rc == ETIMEDOUT) __atomic_add_fetch(&cond->timeout_count, 1, __ATOMIC_RELAXED);
    __atomic_store_n(&cond->last_host_rc, rc, __ATOMIC_RELAXED);
    compat_threads_cond_diag_log(cond, "wake", guest_mutex, caller, sequence, rc, guest_clock_id, deadline,
                  compat_threads_wait_clock_ms() - started);
    return rc;
}

int ssrnx_bionic_pthread_cond_wait(void *guest_cond, void *guest_mutex) {
    compat_threads_CompatCond *cond = compat_threads_get_cond(guest_cond, 1);
    compat_threads_CompatMutex *mutex = compat_threads_get_mutex(guest_mutex, 1);
    if (!cond || !mutex) return ENOMEM;
    GUEST_WAIT_SITE("cond_wait", guest_cond);
    return ssrnx_errno_host_to_android(compat_threads_cond_wait_diagnostic(cond, mutex, guest_mutex, NULL,
                                                           cond->guest_clock_id));
}

static int compat_threads_ssrnx_mutex_timedlock_clock(void *guest_mutex, const void *guest_abstime,
                                       clockid_t clock_id) {
    const compat_threads_SsrnxGuestTimespec64 *gt = (const compat_threads_SsrnxGuestTimespec64 *)guest_abstime;
    struct timespec deadline;
    const int drc = compat_threads_ssrnx_deadline_from_guest(gt, clock_id, &deadline);
    if (drc != 0) return drc;
    compat_threads_CompatMutex *entry = compat_threads_get_mutex(guest_mutex, 1);
    if (!entry) return ENOMEM;

    GUEST_WAIT_SITE("mutex_timedlock", guest_mutex);
    for (;;) {
        int rc = compat_threads_mutex_uses_state(entry) ? compat_threads_state_mutex_trylock(entry) : pthread_mutex_trylock(&entry->host);
        if (rc == 0 || rc != EBUSY) return ssrnx_errno_host_to_android(rc);
        if (compat_threads_mutex_uses_state(entry) && entry->type == 2) {
            compat_threads_CompatThread *self = compat_threads_adopt_current_thread();
            if (self && __atomic_load_n(&entry->acquirer, __ATOMIC_ACQUIRE) == (uint64_t)self->guest_id)
                return EDEADLK;
        }
        struct timespec now = {0, 0};
        if (clock_gettime(clock_id, &now) != 0) return ssrnx_errno_host_to_android(errno ? errno : EINVAL);
        if (compat_threads_ssrnx_timespec_cmp(&now, &deadline) >= 0) return ETIMEDOUT;
        int64_t remaining_ns = ((int64_t)deadline.tv_sec - (int64_t)now.tv_sec) * INT64_C(1000000000) +
                               ((int64_t)deadline.tv_nsec - (int64_t)now.tv_nsec);
        if (remaining_ns > INT64_C(1000000)) remaining_ns = INT64_C(1000000);
        struct timespec nap = { (time_t)(remaining_ns / INT64_C(1000000000)),
                                (long)(remaining_ns % INT64_C(1000000000)) };
        (void)nanosleep(&nap, NULL);
        compat_threads_signal_deliver_self(compat_threads_adopt_current_thread());
    }
}

int ssrnx_bionic_pthread_mutex_timedlock(void *guest_mutex, const void *guest_abstime) {
    return compat_threads_ssrnx_mutex_timedlock_clock(guest_mutex, guest_abstime, CLOCK_REALTIME);
}

int ssrnx_bionic_pthread_mutex_timedlock_monotonic_np(void *guest_mutex, const void *guest_abstime) {
    return compat_threads_ssrnx_mutex_timedlock_clock(guest_mutex, guest_abstime, CLOCK_MONOTONIC);
}

int ssrnx_bionic_pthread_mutex_clocklock(void *guest_mutex, int guest_clock_id,
                                         const void *guest_abstime) {
    if (guest_clock_id != CLOCK_REALTIME && guest_clock_id != CLOCK_MONOTONIC) return EINVAL;
    return compat_threads_ssrnx_mutex_timedlock_clock(guest_mutex, guest_abstime, (clockid_t)guest_clock_id);
}

/* Linux futex ABI commands used by Android/Bionic and libc++. The private
 * flag changes kernel sharing scope only; SSRNX is a single Android process,
 * so private and process-shared waits share one process-local sidecar table.
 *
 * Do not build this directly on Horizon's address arbiter: guest futex words
 * may live in heap, ELF data, or process-code-mapped anonymous pages. A
 * sidecar pthread condition variable keyed by the guest word reproduces the
 * Linux atomic check/sleep/wake contract independently of Horizon memory type
 * and is exercised by the host regression suite as well as Switch builds. */
enum {
    SSRNX_FUTEX_WAIT = 0,
    SSRNX_FUTEX_WAKE = 1,
    SSRNX_FUTEX_WAIT_BITSET = 9,
    SSRNX_FUTEX_WAKE_BITSET = 10,
    SSRNX_FUTEX_PRIVATE_FLAG = 128,
    SSRNX_FUTEX_CLOCK_REALTIME = 256,
    SSRNX_FUTEX_CMD_MASK = 127
};
#define SSRNX_FUTEX_BITSET_MATCH_ANY UINT32_C(0xffffffff)

typedef struct compat_threads_CompatFutexWaiter {
    uint32_t bitset;
    bool woken;
    struct compat_threads_CompatFutexWaiter *next;
} compat_threads_CompatFutexWaiter;

typedef struct compat_threads_CompatFutex {
    uint32_t *uaddr;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    unsigned waiters;
    compat_threads_CompatFutexWaiter *waiter_list;
    struct compat_threads_CompatFutex *next; /* immutable once published */
    uint64_t wait_count, timeout_count, mismatch_count, wake_count, selected_count;
    uintptr_t last_wake_caller;
    uint32_t last_op, last_expected, last_observed;
    int64_t last_timeout_ns;
} compat_threads_CompatFutex;

/* Android runtimes use futexes on a relatively small, repeatedly reused set
 * of synchronization words. Keep one sidecar mutex/condition per observed
 * address instead of constructing/destroying a pthread condvar on every wait.
 * Hashing also avoids an O(n) registry walk on the hottest synchronization
 * path. Objects are process-lifetime caches; guest address reuse is safe
 * because waiter state itself remains stack-local and is removed before a
 * wait returns.
 *
 * Every guest futex operation looks its sidecar up, so the registry takes no
 * lock: buckets are prepend-only lists published with release CAS and
 * entries are never unlinked or freed. A global mutex here serialized all
 * runtime synchronization behind Horizon's lock handoff, which grants
 * ownership to a waiter that may not be scheduled for a long time. */
#define SSRNX_FUTEX_BUCKETS 257u
static compat_threads_CompatFutex *compat_threads_g_futex_registry[SSRNX_FUTEX_BUCKETS];

static size_t compat_threads_futex_bucket_index(const uint32_t *uaddr) {
    uintptr_t v = (uintptr_t)uaddr >> 2;
    v ^= v >> 17;
    v ^= v >> 9;
    return (size_t)(v % SSRNX_FUTEX_BUCKETS);
}

static compat_threads_CompatFutex *compat_threads_futex_find_from(compat_threads_CompatFutex *f, const uint32_t *uaddr) {
    while (f && f->uaddr != uaddr) f = f->next;
    return f;
}

static compat_threads_CompatFutex *compat_threads_futex_get(uint32_t *uaddr, bool create) {
    compat_threads_CompatFutex **bucket = &compat_threads_g_futex_registry[compat_threads_futex_bucket_index(uaddr)];
    compat_threads_CompatFutex *head = __atomic_load_n(bucket, __ATOMIC_ACQUIRE);
    compat_threads_CompatFutex *f = compat_threads_futex_find_from(head, uaddr);
    if (f || !create) return f;

    compat_threads_CompatFutex *fresh = (compat_threads_CompatFutex *)calloc(1, sizeof(*fresh));
    if (!fresh) return NULL;
    if (pthread_mutex_init(&fresh->lock, NULL) != 0) { free(fresh); return NULL; }
    if (pthread_cond_init(&fresh->cond, NULL) != 0) {
        pthread_mutex_destroy(&fresh->lock);
        free(fresh);
        return NULL;
    }
    fresh->uaddr = uaddr;
    for (;;) {
        fresh->next = head;
        if (__atomic_compare_exchange_n(bucket, &head, fresh, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
            return fresh;
        /* Another thread published first; it may have added this address. */
        f = compat_threads_futex_find_from(head, uaddr);
        if (f) {
            pthread_cond_destroy(&fresh->cond);
            pthread_mutex_destroy(&fresh->lock);
            free(fresh);
            return f;
        }
    }
}

static void compat_threads_futex_put(compat_threads_CompatFutex *f) {
    (void)f; /* sidecars are process-lifetime */
}

static void compat_threads_futex_capture_frames(compat_threads_CompatThread *self, const void *frame) {
    if (!self) return;
    uintptr_t fp = (uintptr_t)frame;
    const uintptr_t low = (uintptr_t)self->stack_base;
    const size_t size = self->stack_size;
    for (size_t i = 0; i < sizeof(self->futex_frames) / sizeof(self->futex_frames[0]); ++i) {
        uintptr_t ret = 0, next = 0;
        if (self->stack_ready && !self->stack_capture_rc && low && size >= 2u * sizeof(uintptr_t) &&
            fp >= low && fp - low <= size - 2u * sizeof(uintptr_t) && (fp & 15u) == 0) {
            const uintptr_t *record = (const uintptr_t *)fp;
            next = record[0];
            ret = record[1];
        }
        __atomic_store_n(&self->futex_frames[i], ret, __ATOMIC_RELAXED);
        fp = ret && next > fp ? next : 0;
    }
}

/* Sidecars live for the process lifetime. Never block a stall report on the
 * lock being investigated, and never read the guest word after its wait ends. */
static void compat_threads_futex_log_wait_detail(const char *reason, const compat_threads_WaitSnapshot *s) {
    compat_threads_CompatFutex copy = {0};
    compat_threads_CompatFutex *f = compat_threads_futex_find_from(
        __atomic_load_n(&compat_threads_g_futex_registry[compat_threads_futex_bucket_index((const uint32_t *)s->object)], __ATOMIC_ACQUIRE),
        (const uint32_t *)s->object);
    bool captured = false;
    if (f && pthread_mutex_trylock(&f->lock) == 0) {
        copy.waiters = f->waiters;
        copy.wait_count = f->wait_count; copy.timeout_count = f->timeout_count;
        copy.mismatch_count = f->mismatch_count; copy.wake_count = f->wake_count;
        copy.selected_count = f->selected_count;
        copy.last_wake_caller = f->last_wake_caller;
        copy.last_op = f->last_op; copy.last_expected = f->last_expected;
        copy.last_observed = f->last_observed; copy.last_timeout_ns = f->last_timeout_ns;
        pthread_mutex_unlock(&f->lock);
        captured = true;
    }
    if (captured) {
        char wake_site[192] = "-";
        if (copy.last_wake_caller && ssrnx_compat_describe_guest_address)
            (void)ssrnx_compat_describe_guest_address(copy.last_wake_caller, wake_site, sizeof(wake_site));
        ssrnx_log_category("THREAD", "INFO",
            "stall-futex reason=%s thread=%llu object=%p lastOp=%#x expected=%#x observed=%#x timeoutNs=%lld "
            "waiters=%u waits=%llu timeouts=%llu mismatches=%llu wakeCalls=%llu selected=%llu lastWake=%s",
            reason ? reason : "-", (unsigned long long)s->guest_id, s->object,
            copy.last_op, copy.last_expected, copy.last_observed, (long long)copy.last_timeout_ns,
            copy.waiters, (unsigned long long)copy.wait_count, (unsigned long long)copy.timeout_count,
            (unsigned long long)copy.mismatch_count, (unsigned long long)copy.wake_count,
            (unsigned long long)copy.selected_count, wake_site);
    }
    compat_threads_wait_log_frames("futex", s);
}

static int compat_threads_futex_timeout_relative_ns(const compat_threads_SsrnxGuestTimespec64 *gt, int64_t *out_ns) {
    if (!gt || !out_ns || !compat_threads_ssrnx_timespec64_validate(gt) || gt->tv_sec < 0) return EINVAL;
    if (gt->tv_sec > INT64_MAX / INT64_C(1000000000)) {
        *out_ns = INT64_MAX;
        return 0;
    }
    const int64_t sec_ns = gt->tv_sec * INT64_C(1000000000);
    if (sec_ns > INT64_MAX - gt->tv_nsec) *out_ns = INT64_MAX;
    else *out_ns = sec_ns + gt->tv_nsec;
    return 0;
}

static int compat_threads_futex_timeout_absolute_ns(const compat_threads_SsrnxGuestTimespec64 *gt, bool realtime,
                                     int64_t *out_ns) {
    if (!gt || !out_ns || !compat_threads_ssrnx_timespec64_validate(gt) || gt->tv_sec < 0) return EINVAL;
    const clockid_t clock_id = realtime ? CLOCK_REALTIME : CLOCK_MONOTONIC;
    struct timespec now = {0, 0};
    if (clock_gettime(clock_id, &now) != 0) return errno ? errno : EINVAL;
    if (gt->tv_sec < (int64_t)now.tv_sec ||
        (gt->tv_sec == (int64_t)now.tv_sec && gt->tv_nsec <= (int64_t)now.tv_nsec)) {
        *out_ns = 0;
        return ETIMEDOUT;
    }
    uint64_t sec = (uint64_t)(gt->tv_sec - (int64_t)now.tv_sec);
    int64_t nsec = gt->tv_nsec - (int64_t)now.tv_nsec;
    if (nsec < 0) { sec--; nsec += INT64_C(1000000000); }
    if (sec > (uint64_t)(INT64_MAX / INT64_C(1000000000))) {
        *out_ns = INT64_MAX;
    } else {
        const int64_t base = (int64_t)sec * INT64_C(1000000000);
        *out_ns = base > INT64_MAX - nsec ? INT64_MAX : base + nsec;
    }
    return 0;
}

static struct timespec compat_threads_futex_realtime_deadline_from_ns(int64_t timeout_ns) {
    struct timespec deadline = {0, 0};
    (void)clock_gettime(CLOCK_REALTIME, &deadline);
    return compat_threads_ssrnx_timespec_add_ns(deadline, timeout_ns);
}

static void compat_threads_futex_waiter_remove_locked(compat_threads_CompatFutex *f, compat_threads_CompatFutexWaiter *waiter) {
    compat_threads_CompatFutexWaiter **pp = &f->waiter_list;
    while (*pp && *pp != waiter) pp = &(*pp)->next;
    if (*pp == waiter) *pp = waiter->next;
    if (f->waiters) f->waiters--;
}

long ssrnx_bionic_futex_at(uint32_t *uaddr, int op, uint32_t val,
                           const void *timeout, uint32_t *uaddr2, uint32_t val3,
                           uintptr_t caller, const void *frame) {
    (void)uaddr2;
    if (!uaddr || (((uintptr_t)uaddr) & 3u) != 0) { errno = EINVAL; return -1; }

    const int cmd = op & SSRNX_FUTEX_CMD_MASK;
    const bool realtime = (op & SSRNX_FUTEX_CLOCK_REALTIME) != 0;

    if (cmd == SSRNX_FUTEX_WAIT || cmd == SSRNX_FUTEX_WAIT_BITSET) {
        if (cmd == SSRNX_FUTEX_WAIT && realtime) { errno = EINVAL; return -1; }
        const uint32_t bitset = cmd == SSRNX_FUTEX_WAIT_BITSET
                ? val3 : SSRNX_FUTEX_BITSET_MATCH_ANY;
        if (bitset == 0) { errno = EINVAL; return -1; }

        int64_t timeout_ns = -1;
        if (timeout) {
            const int trc = cmd == SSRNX_FUTEX_WAIT
                    ? compat_threads_futex_timeout_relative_ns((const compat_threads_SsrnxGuestTimespec64 *)timeout, &timeout_ns)
                    : compat_threads_futex_timeout_absolute_ns((const compat_threads_SsrnxGuestTimespec64 *)timeout, realtime, &timeout_ns);
            if (trc != 0 && trc != ETIMEDOUT) { errno = trc; return -1; }
            if (trc == ETIMEDOUT) timeout_ns = 0;
        }

        compat_threads_CompatFutex *f = compat_threads_futex_get(uaddr, true);
        if (!f) { errno = ENOMEM; return -1; }
        compat_threads_CompatFutexWaiter waiter;
        memset(&waiter, 0, sizeof(waiter));
        waiter.bitset = bitset;
        pthread_mutex_lock(&f->lock);
        f->wait_count++;
        f->last_op = (uint32_t)op;
        f->last_expected = val; f->last_timeout_ns = timeout_ns;
        f->last_observed = __atomic_load_n(uaddr, __ATOMIC_ACQUIRE);
        /* Linux futex WAIT compares the user word while enqueueing the waiter;
         * a concurrent wake cannot pass this sidecar lock between the check and
         * the actual condvar sleep, so wakeups are not lost. */
        if (f->last_observed != val) {
            f->mismatch_count++;
            pthread_mutex_unlock(&f->lock);
            compat_threads_futex_put(f);
            errno = EAGAIN;
            return -1;
        }
        waiter.next = f->waiter_list;
        f->waiter_list = &waiter;
        f->waiters++;

        int rc = 0;
        struct timespec deadline = {0, 0};
        const struct timespec *deadline_ptr = NULL;
        if (timeout_ns == 0) rc = ETIMEDOUT;
        else if (timeout_ns > 0) {
            deadline = compat_threads_futex_realtime_deadline_from_ns(timeout_ns);
            deadline_ptr = &deadline;
        }

        GUEST_WAIT_SITE("futex", uaddr);
        compat_threads_t_wait_caller = caller;
        compat_threads_ensure_internal_keys();
        compat_threads_CompatThread *self = (compat_threads_g_signal_proxy_depth || compat_threads_g_signal_target_delivery_depth) ? NULL
                            : (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
        compat_threads_futex_capture_frames(self, frame);
        while (rc == 0 && !waiter.woken) {
            /* The safe point consumes its label. Republish it after signal
             * delivery or a spurious wake instead of losing the guest origin. */
            GUEST_WAIT_SITE("futex", uaddr);
            compat_threads_t_wait_caller = caller;
            /* This mutex protects only host futex bookkeeping; guest code
             * cannot be suspended while holding it. The sliced wait can
             * therefore return and deliver signals on the real target. */
            rc = compat_threads_signal_safe_cond_wait_mode(&f->cond, &f->lock, deadline_ptr, true);
            /* pthread condvars, and SSRNX's signal-safe wrapper, may wake
             * spuriously. Linux futex callers already recheck their state; do
             * not report success unless a matching FUTEX_WAKE selected us. */
            if (rc == 0 && !waiter.woken) continue;
        }

        compat_threads_futex_waiter_remove_locked(f, &waiter);
        const bool woken = waiter.woken;
        if (!woken && rc == ETIMEDOUT) f->timeout_count++;
        pthread_mutex_unlock(&f->lock);
        compat_threads_futex_put(f);
        if (woken) return 0;
        errno = rc == ETIMEDOUT ? ETIMEDOUT : EINTR;
        return -1;
    }

    if (cmd == SSRNX_FUTEX_WAKE || cmd == SSRNX_FUTEX_WAKE_BITSET) {
        if (cmd == SSRNX_FUTEX_WAKE && realtime) { errno = EINVAL; return -1; }
        const uint32_t wake_mask = cmd == SSRNX_FUTEX_WAKE_BITSET
                ? val3 : SSRNX_FUTEX_BITSET_MATCH_ANY;
        if (wake_mask == 0) { errno = EINVAL; return -1; }
        if (val == 0) return 0;

        compat_threads_CompatFutex *f = compat_threads_futex_get(uaddr, false);
        if (!f) return 0;
        pthread_mutex_lock(&f->lock);
        f->wake_count++;
        f->last_wake_caller = caller;
        unsigned count = 0;
        for (compat_threads_CompatFutexWaiter *w = f->waiter_list; w && count < val; w = w->next) {
            if (!w->woken && (w->bitset & wake_mask) != 0) {
                w->woken = true;
                count++;
            }
        }
        /* One shared condvar may wake non-selected waiters too. Their woken
         * bit remains false, so they immediately resume waiting. The return
         * count therefore retains Linux FUTEX_WAKE semantics exactly while
         * avoiding a condvar allocation per waiter. */
        if (count) (void)pthread_cond_broadcast(&f->cond);
        f->selected_count += count;
        pthread_mutex_unlock(&f->lock);
        compat_threads_futex_put(f);
        return (long)count;
    }

    /* PI, requeue and WAKE_OP futex commands require additional kernel-level
     * ownership/requeue semantics. Leave them explicit rather than silently
     * approximating them; full trace records the exact op if a guest needs one. */
    errno = ENOSYS;
    return -1;
}

long ssrnx_bionic_futex(uint32_t *uaddr, int op, uint32_t val,
                        const void *timeout, uint32_t *uaddr2, uint32_t val3) {
    return ssrnx_bionic_futex_at(uaddr, op, val, timeout, uaddr2, val3,
             (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)),
             __builtin_frame_address(0));
}

static int compat_threads_cond_timedwait_clock_host(void *guest_cond, void *guest_mutex,
                                     const void *guest_abstime, int guest_clock_id, uintptr_t caller) {
    const compat_threads_SsrnxGuestTimespec64 *gt = (const compat_threads_SsrnxGuestTimespec64 *)guest_abstime;
    struct timespec deadline;
    const clockid_t clock_id = guest_clock_id == 1 ? CLOCK_MONOTONIC : CLOCK_REALTIME;
    const int drc = compat_threads_ssrnx_deadline_from_guest(gt, clock_id, &deadline);
    if (drc != 0) return drc;

    compat_threads_CompatCond *cond = compat_threads_get_cond(guest_cond, 1);
    compat_threads_CompatMutex *mutex = compat_threads_get_mutex(guest_mutex, 1);
    if (!cond || !mutex) return ENOMEM;
    compat_threads_t_wait_kind = "cond_timedwait"; compat_threads_t_wait_object = guest_cond; compat_threads_t_wait_caller = caller;
    return compat_threads_cond_wait_diagnostic(cond, mutex, guest_mutex, &deadline, guest_clock_id);
}

static int compat_threads_cond_timedwait_host(void *guest_cond, void *guest_mutex,
                               const void *guest_abstime, uintptr_t caller) {
    compat_threads_CompatCond *cond = compat_threads_get_cond(guest_cond, 1);
    if (!cond) return ENOMEM;
    return compat_threads_cond_timedwait_clock_host(guest_cond, guest_mutex, guest_abstime,
                                     cond->guest_clock_id, caller);
}

/* pthread_* return error numbers instead of setting errno: translate at the
 * guest boundary (libc++ condition_variable compares against Linux ETIMEDOUT
 * and throws system_error on anything else). */
int ssrnx_bionic_pthread_cond_timedwait(void *guest_cond, void *guest_mutex,
                                        const void *guest_abstime) {
    return ssrnx_errno_host_to_android(compat_threads_cond_timedwait_host(guest_cond, guest_mutex, guest_abstime,
        (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0))));
}


int ssrnx_bionic_pthread_cond_timedwait_monotonic_np(void *guest_cond, void *guest_mutex,
                                                     const void *guest_abstime) {
    return ssrnx_errno_host_to_android(
        compat_threads_cond_timedwait_clock_host(guest_cond, guest_mutex, guest_abstime, 1,
            (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0))));
}

int ssrnx_bionic_pthread_cond_clockwait(void *guest_cond, void *guest_mutex, int guest_clock_id,
                                        const void *guest_abstime) {
    if (guest_clock_id != CLOCK_REALTIME && guest_clock_id != CLOCK_MONOTONIC) return EINVAL;
    return ssrnx_errno_host_to_android(
        compat_threads_cond_timedwait_clock_host(guest_cond, guest_mutex, guest_abstime, guest_clock_id,
            (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0))));
}

int ssrnx_bionic_pthread_cond_timedwait_relative_np(void *guest_cond, void *guest_mutex,
                                                    const void *guest_reltime) {
    const compat_threads_SsrnxGuestTimespec64 *rel = (const compat_threads_SsrnxGuestTimespec64 *)guest_reltime;
    if (!rel || !compat_threads_ssrnx_timespec64_validate(rel) || rel->tv_sec < 0) return EINVAL;
    struct timespec now = {0, 0};
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return ssrnx_errno_host_to_android(errno ? errno : EINVAL);
    if (rel->tv_sec > INT64_MAX / INT64_C(1000000000)) return EINVAL;
    int64_t ns = rel->tv_sec * INT64_C(1000000000) + rel->tv_nsec;
    struct timespec deadline = compat_threads_ssrnx_timespec_add_ns(now, ns);
    compat_threads_SsrnxGuestTimespec64 abs = { (int64_t)deadline.tv_sec, (int64_t)deadline.tv_nsec };
    return ssrnx_errno_host_to_android(compat_threads_cond_timedwait_clock_host(guest_cond, guest_mutex, &abs, 1,
        (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0))));
}

int ssrnx_bionic_pthread_cond_signal(void *guest_cond) {
    compat_threads_CompatCond *cond = compat_threads_get_cond(guest_cond, 1);
    if (!cond) return ENOMEM;
    const uint64_t sequence = __atomic_add_fetch(&cond->signal_count, 1, __ATOMIC_RELAXED);
    const int rc = pthread_cond_signal(&cond->host);
    if (rc == 0) {
        __atomic_add_fetch(&cond->normal_sequence, 1, __ATOMIC_RELEASE);
        compat_threads_addr_wake(&cond->normal_sequence, 1);
    }
    compat_threads_cond_diag_log(cond, "signal", NULL,
                  (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)),
                  sequence, rc, cond->guest_clock_id, NULL, 0);
    return rc;
}

int ssrnx_bionic_pthread_cond_broadcast(void *guest_cond) {
    compat_threads_CompatCond *cond = compat_threads_get_cond(guest_cond, 1);
    if (!cond) return ENOMEM;
    const uint64_t sequence = __atomic_add_fetch(&cond->broadcast_count, 1, __ATOMIC_RELAXED);
    const int rc = pthread_cond_broadcast(&cond->host);
    if (rc == 0) {
        __atomic_add_fetch(&cond->normal_sequence, 1, __ATOMIC_RELEASE);
        compat_threads_addr_wake(&cond->normal_sequence, -1);
    }
    compat_threads_cond_diag_log(cond, "broadcast", NULL,
                  (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)),
                  sequence, rc, cond->guest_clock_id, NULL, 0);
    return rc;
}

#define SSRNX_BIONIC_RWLOCKATTR_PSHARED_MASK 0x1L
#define SSRNX_BIONIC_RWLOCKATTR_KIND_MASK    0x2L
#define SSRNX_BIONIC_RWLOCKATTR_RESERVED     (~3L)

int ssrnx_bionic_pthread_rwlockattr_init(long *guest_attr) {
    if (!guest_attr) return EINVAL;
    *guest_attr = 0;
    return 0;
}

int ssrnx_bionic_pthread_rwlockattr_destroy(long *guest_attr) {
    if (!guest_attr) return EINVAL;
    *guest_attr = -1;
    return 0;
}

int ssrnx_bionic_pthread_rwlockattr_getpshared(const long *guest_attr, int *pshared) {
    if (!guest_attr || !pshared) return EINVAL;
    *pshared = (*guest_attr & SSRNX_BIONIC_RWLOCKATTR_PSHARED_MASK) ? 1 : 0;
    return 0;
}

int ssrnx_bionic_pthread_rwlockattr_setpshared(long *guest_attr, int pshared) {
    if (!guest_attr || (pshared != 0 && pshared != 1)) return EINVAL;
    if (pshared) *guest_attr |= SSRNX_BIONIC_RWLOCKATTR_PSHARED_MASK;
    else *guest_attr &= ~SSRNX_BIONIC_RWLOCKATTR_PSHARED_MASK;
    return 0;
}

int ssrnx_bionic_pthread_rwlockattr_getkind_np(const long *guest_attr, int *kind) {
    if (!guest_attr || !kind) return EINVAL;
    *kind = (*guest_attr & SSRNX_BIONIC_RWLOCKATTR_KIND_MASK) ? 1 : 0;
    return 0;
}

int ssrnx_bionic_pthread_rwlockattr_setkind_np(long *guest_attr, int kind) {
    if (!guest_attr || (kind != 0 && kind != 1)) return EINVAL;
    if (kind) *guest_attr |= SSRNX_BIONIC_RWLOCKATTR_KIND_MASK;
    else *guest_attr &= ~SSRNX_BIONIC_RWLOCKATTR_KIND_MASK;
    return 0;
}

int ssrnx_bionic_pthread_rwlock_init(void *guest_rwlock, const long *guest_attr) {
    if (!guest_rwlock) return EINVAL;
    int writer_preferred = 0;
    if (guest_attr) {
        if ((*guest_attr & SSRNX_BIONIC_RWLOCKATTR_RESERVED) != 0) return EINVAL;
        writer_preferred = ((*guest_attr & SSRNX_BIONIC_RWLOCKATTR_KIND_MASK) != 0);
    }
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    if (compat_threads_find_rwlock_locked(guest_rwlock)) {
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        return EBUSY;
    }
    compat_threads_CompatRwlock *entry = compat_threads_create_rwlock_locked(guest_rwlock, writer_preferred);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    if (!entry) return ENOMEM;
    memset(guest_rwlock, 0, 56); /* Bionic LP64 pthread_rwlock_t size. */
    return 0;
}

int ssrnx_bionic_pthread_rwlock_destroy(void *guest_rwlock) {
    if (!guest_rwlock) return EINVAL;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatRwlock **link = &compat_threads_g_rwlocks;
    while (*link && (*link)->guest != guest_rwlock) link = &(*link)->next;
    compat_threads_CompatRwlock *entry = *link;
    if (!entry) {
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        return 0;
    }

    pthread_mutex_lock(&entry->lock);
    const int busy = entry->writer || entry->readers != 0 || entry->waiting_writers != 0;
    pthread_mutex_unlock(&entry->lock);
    if (busy) {
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        return EBUSY;
    }

    const int cond_rc = pthread_cond_destroy(&entry->cond);
    const int mutex_rc = pthread_mutex_destroy(&entry->lock);
    const int rc = cond_rc != 0 ? cond_rc : mutex_rc;
    if (rc == 0) {
        *link = entry->next;
        free(entry);
    }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return ssrnx_errno_host_to_android(rc);
}

static int compat_threads_rwlock_rdlock_clock(void *guest_rwlock, const void *guest_abstime,
                               clockid_t clock_id, bool timed) {
    compat_threads_CompatRwlock *entry = compat_threads_get_rwlock(guest_rwlock, 1);
    if (!entry) return ENOMEM;
    struct timespec deadline = {0, 0};
    if (timed) {
        const int rc = compat_threads_ssrnx_deadline_from_guest((const compat_threads_SsrnxGuestTimespec64 *)guest_abstime,
                                                 clock_id, &deadline);
        if (rc != 0) return rc;
    }
    const SsrnxGuestPthread self = ssrnx_bionic_pthread_self();
    pthread_mutex_lock(&entry->lock);
    if (entry->writer && entry->writer_owner == self) {
        pthread_mutex_unlock(&entry->lock);
        return EDEADLK;
    }
    while (entry->writer || (entry->writer_nonrecursive_preferred && entry->waiting_writers != 0)) {
        GUEST_WAIT_SITE("rwlock_rdlock", guest_rwlock);
        const int rc = compat_threads_ssrnx_cond_wait_until(&entry->cond, &entry->lock,
                                             timed ? &deadline : NULL, clock_id);
        if (rc != 0) { pthread_mutex_unlock(&entry->lock); return ssrnx_errno_host_to_android(rc); }
    }
    entry->readers++;
    pthread_mutex_unlock(&entry->lock);
    return 0;
}

int ssrnx_bionic_pthread_rwlock_rdlock(void *guest_rwlock) {
    return compat_threads_rwlock_rdlock_clock(guest_rwlock, NULL, CLOCK_REALTIME, false);
}

int ssrnx_bionic_pthread_rwlock_timedrdlock(void *guest_rwlock, const void *guest_abstime) {
    return compat_threads_rwlock_rdlock_clock(guest_rwlock, guest_abstime, CLOCK_REALTIME, true);
}

int ssrnx_bionic_pthread_rwlock_timedrdlock_monotonic_np(void *guest_rwlock, const void *guest_abstime) {
    return compat_threads_rwlock_rdlock_clock(guest_rwlock, guest_abstime, CLOCK_MONOTONIC, true);
}

int ssrnx_bionic_pthread_rwlock_clockrdlock(void *guest_rwlock, int guest_clock_id,
                                            const void *guest_abstime) {
    if (guest_clock_id != CLOCK_REALTIME && guest_clock_id != CLOCK_MONOTONIC) return EINVAL;
    return compat_threads_rwlock_rdlock_clock(guest_rwlock, guest_abstime, (clockid_t)guest_clock_id, true);
}

int ssrnx_bionic_pthread_rwlock_tryrdlock(void *guest_rwlock) {
    compat_threads_CompatRwlock *entry = compat_threads_get_rwlock(guest_rwlock, 1);
    if (!entry) return ENOMEM;
    const SsrnxGuestPthread self = ssrnx_bionic_pthread_self();
    pthread_mutex_lock(&entry->lock);
    if (entry->writer && entry->writer_owner == self) {
        pthread_mutex_unlock(&entry->lock);
        return EDEADLK;
    }
    if (entry->writer || (entry->writer_nonrecursive_preferred && entry->waiting_writers != 0)) {
        pthread_mutex_unlock(&entry->lock);
        return EBUSY;
    }
    entry->readers++;
    pthread_mutex_unlock(&entry->lock);
    return 0;
}

static int compat_threads_rwlock_wrlock_clock(void *guest_rwlock, const void *guest_abstime,
                               clockid_t clock_id, bool timed) {
    compat_threads_CompatRwlock *entry = compat_threads_get_rwlock(guest_rwlock, 1);
    if (!entry) return ENOMEM;
    struct timespec deadline = {0, 0};
    if (timed) {
        const int rc = compat_threads_ssrnx_deadline_from_guest((const compat_threads_SsrnxGuestTimespec64 *)guest_abstime,
                                                 clock_id, &deadline);
        if (rc != 0) return rc;
    }
    const SsrnxGuestPthread self = ssrnx_bionic_pthread_self();
    pthread_mutex_lock(&entry->lock);
    if (entry->writer && entry->writer_owner == self) {
        pthread_mutex_unlock(&entry->lock);
        return EDEADLK;
    }
    entry->waiting_writers++;
    while (entry->writer || entry->readers != 0) {
        GUEST_WAIT_SITE("rwlock_wrlock", guest_rwlock);
        const int rc = compat_threads_ssrnx_cond_wait_until(&entry->cond, &entry->lock,
                                             timed ? &deadline : NULL, clock_id);
        if (rc != 0) {
            entry->waiting_writers--;
            pthread_cond_broadcast(&entry->cond);
            pthread_mutex_unlock(&entry->lock);
            return ssrnx_errno_host_to_android(rc);
        }
    }
    entry->waiting_writers--;
    entry->writer = 1;
    entry->writer_owner = self;
    pthread_mutex_unlock(&entry->lock);
    return 0;
}

int ssrnx_bionic_pthread_rwlock_wrlock(void *guest_rwlock) {
    return compat_threads_rwlock_wrlock_clock(guest_rwlock, NULL, CLOCK_REALTIME, false);
}

int ssrnx_bionic_pthread_rwlock_timedwrlock(void *guest_rwlock, const void *guest_abstime) {
    return compat_threads_rwlock_wrlock_clock(guest_rwlock, guest_abstime, CLOCK_REALTIME, true);
}

int ssrnx_bionic_pthread_rwlock_timedwrlock_monotonic_np(void *guest_rwlock, const void *guest_abstime) {
    return compat_threads_rwlock_wrlock_clock(guest_rwlock, guest_abstime, CLOCK_MONOTONIC, true);
}

int ssrnx_bionic_pthread_rwlock_clockwrlock(void *guest_rwlock, int guest_clock_id,
                                            const void *guest_abstime) {
    if (guest_clock_id != CLOCK_REALTIME && guest_clock_id != CLOCK_MONOTONIC) return EINVAL;
    return compat_threads_rwlock_wrlock_clock(guest_rwlock, guest_abstime, (clockid_t)guest_clock_id, true);
}

int ssrnx_bionic_pthread_rwlock_trywrlock(void *guest_rwlock) {
    compat_threads_CompatRwlock *entry = compat_threads_get_rwlock(guest_rwlock, 1);
    if (!entry) return ENOMEM;
    const SsrnxGuestPthread self = ssrnx_bionic_pthread_self();
    pthread_mutex_lock(&entry->lock);
    if (entry->writer && entry->writer_owner == self) {
        pthread_mutex_unlock(&entry->lock);
        return EDEADLK;
    }
    if (entry->writer || entry->readers != 0) {
        pthread_mutex_unlock(&entry->lock);
        return EBUSY;
    }
    entry->writer = 1;
    entry->writer_owner = self;
    pthread_mutex_unlock(&entry->lock);
    return 0;
}

int ssrnx_bionic_pthread_rwlock_unlock(void *guest_rwlock) {
    compat_threads_CompatRwlock *entry = compat_threads_get_rwlock(guest_rwlock, 0);
    if (!entry) return EINVAL;
    const SsrnxGuestPthread self = ssrnx_bionic_pthread_self();
    pthread_mutex_lock(&entry->lock);
    if (entry->writer) {
        if (entry->writer_owner != self) { pthread_mutex_unlock(&entry->lock); return EPERM; }
        entry->writer = 0;
        entry->writer_owner = 0;
    } else if (entry->readers != 0) {
        entry->readers--;
    } else {
        pthread_mutex_unlock(&entry->lock);
        return EINVAL;
    }
    pthread_cond_broadcast(&entry->cond);
    pthread_mutex_unlock(&entry->lock);
    return 0;
}

/* Android API 24 pthread_spinlock_t is a single process-visible integer. SSRNX
 * is one Android process, so PRIVATE/SHARED use the same atomic word. */
int ssrnx_bionic_pthread_spin_init(void *guest_spin, int pshared) {
    if (!guest_spin || (pshared != 0 && pshared != 1)) return EINVAL;
    __atomic_store_n((uint32_t *)guest_spin, 0u, __ATOMIC_RELEASE);
    return 0;
}

int ssrnx_bionic_pthread_spin_destroy(void *guest_spin) {
    if (!guest_spin) return EINVAL;
    if (__atomic_load_n((uint32_t *)guest_spin, __ATOMIC_ACQUIRE) != 0) return EBUSY;
    return 0;
}

int ssrnx_bionic_pthread_spin_trylock(void *guest_spin) {
    if (!guest_spin) return EINVAL;
    uint32_t expected = 0;
    return __atomic_compare_exchange_n((uint32_t *)guest_spin, &expected, 1u, false,
                                       __ATOMIC_ACQUIRE, __ATOMIC_RELAXED) ? 0 : EBUSY;
}

int ssrnx_bionic_pthread_spin_lock(void *guest_spin) {
    if (!guest_spin) return EINVAL;
    for (;;) {
        const int rc = ssrnx_bionic_pthread_spin_trylock(guest_spin);
        if (rc == 0) return 0;
        if (rc != EBUSY) return rc;
        (void)sched_yield();
        compat_threads_signal_deliver_self(compat_threads_adopt_current_thread());
    }
}

int ssrnx_bionic_pthread_spin_unlock(void *guest_spin) {
    if (!guest_spin) return EINVAL;
    if (__atomic_exchange_n((uint32_t *)guest_spin, 0u, __ATOMIC_RELEASE) == 0u) return EPERM;
    return 0;
}

int ssrnx_bionic_pthread_barrierattr_init(long *guest_attr) {
    if (!guest_attr) return EINVAL;
    *guest_attr = 0;
    return 0;
}

int ssrnx_bionic_pthread_barrierattr_destroy(long *guest_attr) {
    if (!guest_attr) return EINVAL;
    *guest_attr = -1;
    return 0;
}

int ssrnx_bionic_pthread_barrierattr_getpshared(const long *guest_attr, int *pshared) {
    if (!guest_attr || !pshared) return EINVAL;
    *pshared = (*guest_attr & 1L) ? 1 : 0;
    return 0;
}

int ssrnx_bionic_pthread_barrierattr_setpshared(long *guest_attr, int pshared) {
    if (!guest_attr || (pshared != 0 && pshared != 1)) return EINVAL;
    *guest_attr = (*guest_attr & ~1L) | (long)pshared;
    return 0;
}

int ssrnx_bionic_pthread_barrier_init(void *guest_barrier, const long *guest_attr, unsigned count) {
    if (!guest_barrier || count == 0) return EINVAL;
    if (guest_attr && ((*guest_attr & ~1L) != 0)) return EINVAL;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    if (compat_threads_find_barrier_locked(guest_barrier)) { pthread_mutex_unlock(&compat_threads_g_registry_lock); return EBUSY; }
    compat_threads_CompatBarrier *entry = compat_threads_create_barrier_locked(guest_barrier, count);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    if (!entry) return ENOMEM;
    memset(guest_barrier, 0, 32); /* Android LP64 pthread_barrier_t. */
    return 0;
}

int ssrnx_bionic_pthread_barrier_destroy(void *guest_barrier) {
    if (!guest_barrier) return EINVAL;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatBarrier **link = &compat_threads_g_barriers;
    while (*link && (*link)->guest != guest_barrier) link = &(*link)->next;
    compat_threads_CompatBarrier *entry = *link;
    if (!entry) { pthread_mutex_unlock(&compat_threads_g_registry_lock); return EINVAL; }
    pthread_mutex_lock(&entry->lock);
    if (entry->waiting != 0) { pthread_mutex_unlock(&entry->lock); pthread_mutex_unlock(&compat_threads_g_registry_lock); return EBUSY; }
    pthread_mutex_unlock(&entry->lock);
    const int crc = pthread_cond_destroy(&entry->cond);
    const int mrc = pthread_mutex_destroy(&entry->lock);
    const int rc = crc != 0 ? crc : mrc;
    if (rc == 0) { *link = entry->next; free(entry); }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return ssrnx_errno_host_to_android(rc);
}

int ssrnx_bionic_pthread_barrier_wait(void *guest_barrier) {
    compat_threads_CompatBarrier *entry = compat_threads_get_barrier(guest_barrier);
    if (!entry) return EINVAL;
    pthread_mutex_lock(&entry->lock);
    const unsigned generation = entry->generation;
    if (++entry->waiting == entry->init_count) {
        entry->waiting = 0;
        entry->generation++;
        pthread_cond_broadcast(&entry->cond);
        pthread_mutex_unlock(&entry->lock);
        return -1; /* Bionic PTHREAD_BARRIER_SERIAL_THREAD. */
    }
    while (generation == entry->generation) {
        GUEST_WAIT_SITE("barrier_wait", guest_barrier);
        const int rc = compat_threads_signal_safe_cond_wait(&entry->cond, &entry->lock, NULL);
        if (rc != 0) { pthread_mutex_unlock(&entry->lock); return ssrnx_errno_host_to_android(rc); }
    }
    pthread_mutex_unlock(&entry->lock);
    return 0;
}

int ssrnx_bionic_pthread_once(SsrnxGuestOnce *once_control, void (*init_routine)(void)) {
    if (!once_control || !init_routine) return EINVAL;
    if (__atomic_load_n(once_control, __ATOMIC_ACQUIRE) == 2) return 0;

    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    for (;;) {
        const int state = __atomic_load_n(once_control, __ATOMIC_ACQUIRE);
        if (state == 2) {
            pthread_mutex_unlock(&compat_threads_g_registry_lock);
            return 0;
        }
        if (state == 0) {
            __atomic_store_n(once_control, 1, __ATOMIC_RELEASE);
            break;
        }
        GUEST_WAIT_SITE("pthread_once", once_control);
        (void)compat_threads_signal_safe_cond_wait(&compat_threads_g_once_cond, &compat_threads_g_registry_lock, NULL);
    }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);

    init_routine();

    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    __atomic_store_n(once_control, 2, __ATOMIC_RELEASE);
    pthread_cond_broadcast(&compat_threads_g_once_cond);
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    return 0;
}

static bool compat_threads_guest_key_slot(SsrnxGuestKey key, size_t *out_slot) {
    const uint32_t raw = (uint32_t)key;
    if ((raw & SSRNX_BIONIC_KEY_VALID_FLAG) == 0) return false;
    const uint32_t slot = raw & ~SSRNX_BIONIC_KEY_VALID_FLAG;
    if (slot >= SSRNX_BIONIC_PTHREAD_KEYS_MAX) return false;
    *out_slot = slot;
    return true;
}

static compat_threads_CompatKeyTable *compat_threads_current_guest_key_table(bool create) {
    compat_threads_ensure_internal_keys();
    if (!compat_threads_g_guest_key_table_ready) return NULL;
    compat_threads_CompatKeyTable *table = (compat_threads_CompatKeyTable *)pthread_getspecific(compat_threads_g_guest_key_table_key);
    if (table) return table;
    /* A signal handler that ran on this thread's behalf may have created it. */
    compat_threads_CompatThread *self = (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
    if (self && self->key_table) {
        table = (compat_threads_CompatKeyTable *)self->key_table;
        if (pthread_setspecific(compat_threads_g_guest_key_table_key, table) == 0) return table;
        return NULL;
    }
    if (!create) return NULL;
    table = (compat_threads_CompatKeyTable *)calloc(1, sizeof(*table));
    if (!table) return NULL;
    if (pthread_setspecific(compat_threads_g_guest_key_table_key, table) != 0) {
        free(table);
        return NULL;
    }
    if (self) self->key_table = table;
    return table;
}

static void compat_threads_forget_guest_key_table(compat_threads_CompatKeyTable *table) {
    compat_threads_CompatThread *self = (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
    if (self && self->key_table == table) self->key_table = NULL;
}

/* Bionic pthread_key_clean_all(): up to PTHREAD_DESTRUCTOR_ITERATIONS rounds;
 * each live non-NULL value is cleared before its destructor runs, so a
 * destructor may store a new value and get another round. */
static void compat_threads_guest_key_table_run_destructors(compat_threads_CompatKeyTable *table) {
    for (int round = 0; round < SSRNX_BIONIC_PTHREAD_DESTRUCTOR_ITERATIONS; ++round) {
        size_t called = 0;
        for (size_t slot = 0; slot < SSRNX_BIONIC_PTHREAD_KEYS_MAX; ++slot) {
            const uintptr_t seq = __atomic_load_n(&compat_threads_g_keys[slot].seq, __ATOMIC_ACQUIRE);
            compat_threads_CompatKeyData *data = &table->data[slot];
            if ((seq & SSRNX_BIONIC_KEY_SEQ_IN_USE) == 0 || data->seq != seq || !data->value) continue;
            void (*destructor)(void *) = __atomic_load_n(&compat_threads_g_keys[slot].guest_destructor, __ATOMIC_ACQUIRE);
            if (!destructor) continue;
            if (__atomic_load_n(&compat_threads_g_keys[slot].seq, __ATOMIC_ACQUIRE) != seq) continue;
            void *value = data->value;
            data->value = NULL;
            destructor(value);
            ++called;
        }
        if (called == 0) break;
    }
}

/* Guest threads run key destructors explicitly on pthread_exit/return, as
 * Bionic does before the thread becomes joinable. The host-key destructor is
 * the fallback for host threads (JNI-attached or SSRNX-owned) that executed
 * guest code. It re-publishes the table while guest destructors run so that
 * a destructor calling pthread_getspecific/setspecific sees its own table. */
static bool compat_threads_g_process_terminating;

void ssrnx_bionic_set_process_terminating(bool terminating) {
    __atomic_store_n(&compat_threads_g_process_terminating, terminating, __ATOMIC_RELEASE);
}

static void compat_threads_guest_key_table_destroy(void *ptr) {
    compat_threads_CompatKeyTable *table = (compat_threads_CompatKeyTable *)ptr;
    if (!table) return;
    /* A host thread unwound by a process kill must not re-enter guest code:
     * Android never runs TLS destructors of a killed process, and the guest
     * runtime they belong to may already be torn down. */
    const bool killed = __atomic_load_n(&compat_threads_g_process_terminating, __ATOMIC_ACQUIRE);
    (void)pthread_setspecific(compat_threads_g_guest_key_table_key, table);
    if (!killed) compat_threads_guest_key_table_run_destructors(table);
    (void)pthread_setspecific(compat_threads_g_guest_key_table_key, NULL);
    compat_threads_forget_guest_key_table(table);
    free(table);
}

static void compat_threads_guest_keys_thread_exit(void) {
    compat_threads_CompatKeyTable *table = compat_threads_current_guest_key_table(false);
    if (!table) return;
    compat_threads_guest_key_table_run_destructors(table);
    (void)pthread_setspecific(compat_threads_g_guest_key_table_key, NULL);
    compat_threads_forget_guest_key_table(table);
    free(table);
}

int ssrnx_bionic_pthread_key_create(SsrnxGuestKey *out_key, void (*destructor)(void *)) {
    if (!out_key) return PTHREAD_SETUP_FAIL("pthread_key_create",out_key,NULL,EINVAL);
    compat_threads_ensure_internal_keys();
    if (!compat_threads_g_guest_key_table_ready) return EAGAIN;
    for (size_t slot = 0; slot < SSRNX_BIONIC_PTHREAD_KEYS_MAX; ++slot) {
        uintptr_t seq = __atomic_load_n(&compat_threads_g_keys[slot].seq, __ATOMIC_RELAXED);
        while ((seq & SSRNX_BIONIC_KEY_SEQ_IN_USE) == 0) {
            if (__atomic_compare_exchange_n(&compat_threads_g_keys[slot].seq, &seq, seq + 1u, true,
                                            __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
                __atomic_store_n(&compat_threads_g_keys[slot].guest_destructor, destructor, __ATOMIC_RELEASE);
                *out_key = (SsrnxGuestKey)(SSRNX_BIONIC_KEY_VALID_FLAG | (uint32_t)slot);
                return 0;
            }
        }
    }
    return PTHREAD_SETUP_FAIL("pthread_key_create",out_key,NULL,EAGAIN);
}

int ssrnx_bionic_pthread_key_delete(SsrnxGuestKey key) {
    size_t slot = 0;
    if (!compat_threads_guest_key_slot(key, &slot)) return EINVAL;
    /* Bumping the sequence invalidates the value in every thread; POSIX does
     * not run destructors on delete. */
    uintptr_t seq = __atomic_load_n(&compat_threads_g_keys[slot].seq, __ATOMIC_RELAXED);
    if ((seq & SSRNX_BIONIC_KEY_SEQ_IN_USE) != 0 &&
        __atomic_compare_exchange_n(&compat_threads_g_keys[slot].seq, &seq, seq + 1u, false,
                                    __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
        return 0;
    }
    return EINVAL;
}

int ssrnx_bionic_pthread_setspecific(SsrnxGuestKey key, const void *value) {
    size_t slot = 0;
    if (!compat_threads_guest_key_slot(key, &slot)) return EINVAL;
    const uintptr_t seq = __atomic_load_n(&compat_threads_g_keys[slot].seq, __ATOMIC_ACQUIRE);
    if ((seq & SSRNX_BIONIC_KEY_SEQ_IN_USE) == 0) return EINVAL;
    compat_threads_CompatKeyTable *table = compat_threads_current_guest_key_table(true);
    if (!table) return ENOMEM;
    table->data[slot].seq = seq;
    table->data[slot].value = (void *)value;
    return 0;
}

void *ssrnx_bionic_pthread_getspecific(SsrnxGuestKey key) {
    size_t slot = 0;
    if (!compat_threads_guest_key_slot(key, &slot)) return NULL;
    compat_threads_CompatKeyTable *table = compat_threads_current_guest_key_table(false);
    if (!table) return NULL;
    const uintptr_t seq = __atomic_load_n(&compat_threads_g_keys[slot].seq, __ATOMIC_ACQUIRE);
    compat_threads_CompatKeyData *data = &table->data[slot];
    if ((seq & SSRNX_BIONIC_KEY_SEQ_IN_USE) != 0 && data->seq == seq) return data->value;
    /* Deleted, or the slot now belongs to a newer key. */
    data->value = NULL;
    return NULL;
}

static void *compat_threads_aligned_tls_alloc(size_t alignment, size_t size) {
    if (alignment < sizeof(void *)) alignment = sizeof(void *);
    if ((alignment & (alignment - 1)) != 0) return NULL;
    if (size > SIZE_MAX - alignment - sizeof(void *)) return NULL;
    void *base = malloc(size + alignment - 1 + sizeof(void *));
    if (!base) return NULL;
    uintptr_t p = (uintptr_t)base + sizeof(void *);
    p = (p + alignment - 1) & ~(uintptr_t)(alignment - 1);
    ((void **)p)[-1] = base;
    return (void *)p;
}

static void compat_threads_aligned_tls_free(void *ptr) {
    if (ptr) free(((void **)ptr)[-1]);
}

static void compat_threads_emutls_array_destroy(void *ptr) {
    compat_threads_EmutlsArray *array = (compat_threads_EmutlsArray *)ptr;
    if (!array) return;
    compat_threads_CompatThread *self = (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
    if (self && self->emutls_array == array) self->emutls_array = NULL;
    for (size_t i = 0; i < array->capacity; ++i) compat_threads_aligned_tls_free(array->objects[i]);
    free(array->objects);
    free(array);
}

void *ssrnx_bionic_emutls_get_address(void *opaque_control) {
    compat_threads_SsrnxEmutlsControl *control = (compat_threads_SsrnxEmutlsControl *)opaque_control;
    if (!control || control->size == 0) return NULL;
    compat_threads_ensure_internal_keys();

    uintptr_t index = __atomic_load_n(&control->object.index, __ATOMIC_ACQUIRE);
    if (index == 0) {
        SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
        index = control->object.index;
        if (index == 0) {
            index = compat_threads_g_next_emutls_index++;
            control->object.index = index;
        }
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        __atomic_thread_fence(__ATOMIC_RELEASE);
    }

    compat_threads_EmutlsArray *array = (compat_threads_EmutlsArray *)pthread_getspecific(compat_threads_g_emutls_array_key);
    if (!array) {
        compat_threads_CompatThread *self = (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
        if (self && self->emutls_array) {
            array = (compat_threads_EmutlsArray *)self->emutls_array;
        } else {
            array = (compat_threads_EmutlsArray *)calloc(1, sizeof(*array));
            if (!array) return NULL;
        }
        if (pthread_setspecific(compat_threads_g_emutls_array_key, array) != 0) {
            if (!self || self->emutls_array != array) free(array);
            return NULL;
        }
        if (self) self->emutls_array = array;
    }

    if (index > array->capacity) {
        size_t next = array->capacity ? array->capacity * 2 : 16;
        while (next < index) {
            if (next > SIZE_MAX / 2) return NULL;
            next *= 2;
        }
        void **grown = (void **)realloc(array->objects, next * sizeof(void *));
        if (!grown) return NULL;
        memset(grown + array->capacity, 0, (next - array->capacity) * sizeof(void *));
        array->objects = grown;
        array->capacity = next;
    }

    void **slot = &array->objects[index - 1];
    if (!*slot) {
        const size_t align = control->align ? (size_t)control->align : sizeof(void *);
        void *object = compat_threads_aligned_tls_alloc(align, (size_t)control->size);
        if (!object) return NULL;
        if (control->value) memcpy(object, control->value, (size_t)control->size);
        else memset(object, 0, (size_t)control->size);
        *slot = object;
    }
    return *slot;
}

/*
 * Address-word waits: Linux futex on host builds, the Horizon address arbiter
 * on Switch. Unlike a kernel mutex, an address wait never hands ownership to
 * a sleeping thread, so a thread stopped for signal delivery cannot end up
 * owning something another thread needs. Callers re-check their condition and
 * deadline after every return.
 */
static void compat_threads_addr_wait(uint32_t *addr, uint32_t expected, int64_t timeout_ns) {
    const int saved_errno = errno;
#ifdef __SWITCH__
    (void)svcWaitForAddress(addr, ArbitrationType_WaitIfEqual, (s64)expected,
                            timeout_ns < 0 ? INT64_C(-1) : (s64)timeout_ns);
#else
    struct timespec ts;
    struct timespec *tsp = NULL;
    if (timeout_ns >= 0) {
        ts.tv_sec = (time_t)(timeout_ns / INT64_C(1000000000));
        ts.tv_nsec = (long)(timeout_ns % INT64_C(1000000000));
        tsp = &ts;
    }
    (void)syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, expected, tsp, NULL, 0);
#endif
    errno = saved_errno;
}

static void compat_threads_addr_wake(uint32_t *addr, int count) {
    const int saved_errno = errno;
#ifdef __SWITCH__
    (void)svcSignalToAddress(addr, SignalType_Signal, 0, count < 0 ? -1 : count);
#else
    (void)syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, count < 0 ? INT_MAX : count, NULL, NULL, 0);
#endif
    errno = saved_errno;
}

static inline __attribute__((always_inline)) uintptr_t signal_current_sp(void) {
    uintptr_t sp;
#if defined(__aarch64__)
    __asm__ volatile("mov %0, sp" : "=r"(sp));
#elif defined(__x86_64__)
    __asm__ volatile("mov %%rsp, %0" : "=r"(sp));
#else
    sp = (uintptr_t)__builtin_frame_address(0);
#endif
    return sp;
}

/*
 * Linux/Bionic signal runtime.
 *
 * Dispositions are process-wide; the mask, pending set and alternate stack
 * are per guest thread (CompatThread). A thread-directed signal is delivered
 * as the Linux kernel would: the handler runs as the target thread, on the
 * target's stack below its stack pointer (or on its alternate stack for
 * SA_ONSTACK), with the target stopped until the handler returns and the
 * saved mask restored. Horizon cannot redirect another thread's PC, so a
 * signal proxy host thread executes the handler while acting as the target:
 * pthread_self(), pthread keys, emutls and TPIDR_EL0 all resolve to the
 * target for the duration.
 *
 * The target is stopped either
 *  - at a safe point: it is blocked in an SSRNX wait (signal_safe_run) with
 *    its callee-saved registers spilled on its stack; it stays inside that
 *    frame until the handler has returned, and drops any lock the wait
 *    returned with first, as a futex waiter interrupted by a signal holds none;
 *  - or by pausing it (svcSetThreadActivity) while it executes guest code. A
 *    thread paused inside SSRNX/libnx code may hold host locks and is resumed
 *    and retried shortly after.
 * The register image of a paused thread is placed in the signal frame's
 * uc_mcontext, so a conservative scan from the handler's stack pointer up to
 * the thread's stack base sees every register value, as it would on Linux.
 *
 * SSRNX's async-signal-safe entry points (sem_post, pthread_self,
 * pthread_kill, raise, sigprocmask/pthread_sigmask, sigsuspend) take no lock
 * a stopped thread can hold.
 */
#define SIG_BIT(signo) (UINT64_C(1) << ((unsigned)(signo) - 1u))
#define SSRNX_ANDROID_SIGKILL 9
#define SSRNX_ANDROID_SIGSTOP 19
#define SIG_UNBLOCKABLE (SIG_BIT(SSRNX_ANDROID_SIGKILL) | SIG_BIT(SSRNX_ANDROID_SIGSTOP))
#define SSRNX_ANDROID_SIG_BLOCK 0
#define SSRNX_ANDROID_SIG_UNBLOCK 1
#define SSRNX_ANDROID_SIG_SETMASK 2
#define SSRNX_ANDROID_SA_SIGINFO UINT32_C(0x00000004)
#define SSRNX_ANDROID_SA_ONSTACK UINT32_C(0x08000000)
#define SSRNX_ANDROID_SA_NODEFER UINT32_C(0x40000000)
#define SSRNX_ANDROID_SA_RESETHAND UINT32_C(0x80000000)
#define SSRNX_ANDROID_MINSIGSTKSZ 2048u
#define SSRNX_ANDROID_SI_TKILL (-6)
#define SSRNX_ANDROID_APP_UID 10000u /* SSRNX's getuid() */

#define SIG_STATE_SAFE       UINT32_C(0x01) /* blocked at a safe point */
#define SIG_STATE_DELIVERING UINT32_C(0x02) /* a proxy runs a handler for it */
#define SIG_STATE_SUSPENDED  UINT32_C(0x04) /* in sigsuspend: delivers its own */
#define SIG_STATE_ASSIGNED   UINT32_C(0x08) /* a proxy owns its delivery */
#define SIG_STATE_KICK       UINT32_C(0x10) /* new signal while ASSIGNED */
#define SIG_STATE_EXITING    UINT32_C(0x20)
#define SIG_STATE_RETURNING  UINT32_C(0x40) /* bounded wait: target takes signals */
#define SIG_STATE_TARGET_KICK UINT32_C(0x80) /* serialize pending with safe-point exit */

/* Distance kept between a safe point's stack pointer and the signal frame:
 * the blocked wait keeps running below that stack pointer. */
#define SIGNAL_SAFE_GAP ((uintptr_t)32u * 1024u)
/* Distance kept below a paused thread's stack pointer (covers the x86-64
 * red zone used by host regressions; AArch64 Linux has none). */
#define SIGNAL_RED_ZONE ((uintptr_t)256u)
#define SIGNAL_PROXY_STACK ((size_t)64u * 1024u)
/* ponytail: poll returning waits every 100ms; replace with a combined signal/
 * object wake backend if idle wakeups become costly. */
#define SIGNAL_WAIT_SLICE_NS INT64_C(100000000)

typedef struct compat_threads_SsrnxSigAction {
    int32_t sa_flags;
    int32_t _pad_flags;
    uintptr_t handler; /* 0 = SIG_DFL, 1 = SIG_IGN */
    uint64_t sa_mask;
    uintptr_t sa_restorer;
} compat_threads_SsrnxSigAction;

typedef struct compat_threads_SsrnxSigInfo {
    int32_t si_signo;
    int32_t si_errno;
    int32_t si_code;
    int32_t _pad0;
    int32_t sender_pid; /* si_pid */
    uint32_t sender_uid; /* si_uid */
    unsigned char payload[104];
} compat_threads_SsrnxSigInfo;

/* Linux arm64 struct sigcontext and ucontext_t (Bionic <sys/ucontext.h>). */
typedef struct compat_threads_SsrnxSigContext {
    uint64_t fault_address;
    uint64_t regs[31];
    uint64_t sp;
    uint64_t pc;
    uint64_t pstate;
    uint8_t reserved[4096] __attribute__((aligned(16)));
} compat_threads_SsrnxSigContext;

typedef struct compat_threads_SsrnxUcontext {
    uint64_t uc_flags;
    uintptr_t uc_link;
    compat_threads_SsrnxSigStack uc_stack;
    uint64_t uc_sigmask;
    uint8_t _sigmask_padding[120];
    compat_threads_SsrnxSigContext uc_mcontext;
} compat_threads_SsrnxUcontext;

#define SSRNX_LINUX_FPSIMD_MAGIC UINT32_C(0x46508001)
typedef struct compat_threads_SsrnxFpsimdContext {
    uint32_t magic;
    uint32_t size;
    uint32_t fpsr;
    uint32_t fpcr;
    uint64_t vregs[32][2];
} compat_threads_SsrnxFpsimdContext;

_Static_assert(sizeof(compat_threads_SsrnxSigAction) == 32, "Android arm64 struct sigaction");
_Static_assert(sizeof(compat_threads_SsrnxSigStack) == 24, "Android arm64 stack_t");
_Static_assert(sizeof(compat_threads_SsrnxSigInfo) == 128, "Android arm64 siginfo_t");
_Static_assert(offsetof(compat_threads_SsrnxSigContext, reserved) == 288, "Linux arm64 sigcontext.__reserved");
_Static_assert(offsetof(compat_threads_SsrnxUcontext, uc_mcontext) == 176, "Linux arm64 ucontext.uc_mcontext");
_Static_assert(sizeof(compat_threads_SsrnxFpsimdContext) == 528, "Linux arm64 fpsimd_context");

/* Register image of a stopped thread. */
typedef struct compat_threads_SignalRegs {
    uint64_t x[31];
    uint64_t sp;
    uint64_t pc;
    uint64_t pstate;
    uint32_t fpsr;
    uint32_t fpcr;
    uint64_t v[32][2];
    bool has_fp;
} compat_threads_SignalRegs;

typedef struct compat_threads_SignalFrame {
    compat_threads_SsrnxUcontext uc;
    compat_threads_SsrnxSigInfo info;
} compat_threads_SignalFrame;

typedef struct compat_threads_SignalCall {
    uintptr_t handler;
    int signo;
    bool siginfo;
    compat_threads_SsrnxSigInfo *info;
    compat_threads_SsrnxUcontext *uc;
} compat_threads_SignalCall;

/* Call fn(arg) on the stack whose top is sp (16-byte aligned), then return
 * on the caller's stack. */
void ssrnx_signal_call_on_stack(uintptr_t sp, void (*fn)(void *), void *arg)
    __attribute__((visibility("hidden")));
#if defined(__aarch64__)
__asm__(
    ".pushsection .text.ssrnx_signal_call_on_stack,\"ax\",%progbits\n"
    ".p2align 2\n"
    ".global ssrnx_signal_call_on_stack\n"
    ".hidden ssrnx_signal_call_on_stack\n"
    ".type ssrnx_signal_call_on_stack, %function\n"
    "ssrnx_signal_call_on_stack:\n"
    "    stp x29, x30, [sp, #-32]!\n"
    "    str x19, [sp, #16]\n"
    "    mov x29, sp\n"
    "    mov x19, sp\n"
    "    mov sp, x0\n"
    "    mov x0, x2\n"
    "    blr x1\n"
    "    mov sp, x19\n"
    "    ldr x19, [sp, #16]\n"
    "    ldp x29, x30, [sp], #32\n"
    "    ret\n"
    ".size ssrnx_signal_call_on_stack, .-ssrnx_signal_call_on_stack\n"
    ".popsection\n");
#elif defined(__x86_64__)
__asm__(
    ".pushsection .text.ssrnx_signal_call_on_stack,\"ax\",@progbits\n"
    ".p2align 4\n"
    ".global ssrnx_signal_call_on_stack\n"
    ".hidden ssrnx_signal_call_on_stack\n"
    ".type ssrnx_signal_call_on_stack, @function\n"
    "ssrnx_signal_call_on_stack:\n"
    "    push %rbp\n"
    "    mov %rsp, %rbp\n"
    "    mov %rdi, %rsp\n"
    "    and $-16, %rsp\n"
    "    mov %rdx, %rdi\n"
    "    call *%rsi\n"
    "    mov %rbp, %rsp\n"
    "    pop %rbp\n"
    "    ret\n"
    ".size ssrnx_signal_call_on_stack, .-ssrnx_signal_call_on_stack\n"
    ".popsection\n");
#else
void ssrnx_signal_call_on_stack(uintptr_t sp, void (*fn)(void *), void *arg) {
    (void)sp;
    fn(arg);
}
#endif

/* Optional: resolved when the SSRNX logger is linked in. */
void ssrnx_log_category(const char *category, const char *level, const char *fmt, ...)
    __attribute__((weak));
void ssrnx_bionic_cxa_thread_finalize_current(void) __attribute__((weak));

static compat_threads_SsrnxSigAction compat_threads_g_sig_actions[SSRNX_BIONIC_NSIG];
static pthread_mutex_t compat_threads_g_sig_action_lock = PTHREAD_MUTEX_INITIALIZER;

static void compat_threads_signal_capture_host_thread(compat_threads_CompatThread *thread) {
#ifdef __SWITCH__
    thread->host_handle = threadGetCurHandle();
#else
    (void)thread;
#endif
}

static uintptr_t compat_threads_signal_disposition(int signo) {
    SSRNX_PERF_LOCK(&compat_threads_g_sig_action_lock);
    const uintptr_t handler = compat_threads_g_sig_actions[signo].handler;
    pthread_mutex_unlock(&compat_threads_g_sig_action_lock);
    return handler;
}

/* Snapshot the action for delivery. SA_RESETHAND resets the disposition to
 * SIG_DFL (clearing SA_SIGINFO) on entry to the handler. SIG_DFL and SIG_IGN
 * are discarded: Horizon has no Linux default action (terminate, stop, core)
 * for SSRNX to reproduce. */
static bool compat_threads_signal_action_take(int signo, compat_threads_SsrnxSigAction *out) {
    SSRNX_PERF_LOCK(&compat_threads_g_sig_action_lock);
    *out = compat_threads_g_sig_actions[signo];
    const bool caught = out->handler > 1u;
    if (caught && ((uint32_t)out->sa_flags & SSRNX_ANDROID_SA_RESETHAND) != 0) {
        compat_threads_g_sig_actions[signo].handler = 0;
        compat_threads_g_sig_actions[signo].sa_flags &= ~(int32_t)SSRNX_ANDROID_SA_SIGINFO;
    }
    pthread_mutex_unlock(&compat_threads_g_sig_action_lock);
    return caught;
}

/* POSIX discards an ignored signal when it is generated; SSRNX likewise
 * discards a default-action one. A blocked signal stays pending unless
 * ignored, since a handler may be installed before it is unblocked. */
static bool compat_threads_signal_generation_kept(const compat_threads_CompatThread *t, int signo) {
    const uintptr_t handler = compat_threads_signal_disposition(signo);
    if (handler > 1u) return true;
    return handler == 0 &&
           (__atomic_load_n(&t->sig_blocked, __ATOMIC_ACQUIRE) & SIG_BIT(signo)) != 0;
}

static bool compat_threads_signal_has_ready(compat_threads_CompatThread *t) {
    return (__atomic_load_n(&t->sig_pending, __ATOMIC_ACQUIRE) &
            ~__atomic_load_n(&t->sig_blocked, __ATOMIC_ACQUIRE)) != 0;
}

/* Take the lowest-numbered pending signal the thread does not block. */
static bool compat_threads_signal_claim(compat_threads_CompatThread *t, int *out_signo) {
    uint64_t pending = __atomic_load_n(&t->sig_pending, __ATOMIC_ACQUIRE);
    for (;;) {
        const uint64_t ready = pending & ~__atomic_load_n(&t->sig_blocked, __ATOMIC_ACQUIRE);
        if (!ready) return false;
        const uint64_t bit = ready & (~ready + 1u);
        if (__atomic_compare_exchange_n(&t->sig_pending, &pending, pending & ~bit, true,
                                        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
            *out_signo = __builtin_ctzll(bit) + 1;
            return true;
        }
    }
}

static void compat_threads_signal_notify(compat_threads_CompatThread *t) {
    __atomic_add_fetch(&t->sig_wake, 1u, __ATOMIC_ACQ_REL);
    compat_threads_addr_wake(&t->sig_wake, -1);
}

static void compat_threads_signal_call_handler(void *arg) {
    const compat_threads_SignalCall *call = (const compat_threads_SignalCall *)arg;
    if (call->siginfo)
        ((void (*)(int, void *, void *))call->handler)(call->signo, call->info, call->uc);
    else
        ((void (*)(int))call->handler)(call->signo);
}

static void compat_threads_signal_fill_mcontext(compat_threads_SsrnxSigContext *mc, const compat_threads_SignalRegs *regs) {
    memcpy(mc->regs, regs->x, sizeof(mc->regs));
    mc->sp = regs->sp;
    mc->pc = regs->pc;
    mc->pstate = regs->pstate;
    if (regs->has_fp) {
        /* One fpsimd_context record; the zeroed header after it ends the list. */
        compat_threads_SsrnxFpsimdContext *fp = (compat_threads_SsrnxFpsimdContext *)(void *)mc->reserved;
        fp->magic = SSRNX_LINUX_FPSIMD_MAGIC;
        fp->size = (uint32_t)sizeof(*fp);
        fp->fpsr = regs->fpsr;
        fp->fpcr = regs->fpcr;
        memcpy(fp->vregs, regs->v, sizeof(fp->vregs));
    }
}

/* Deliver one claimed signal to t. stack_top != 0 places the signal frame
 * below that address (the stopped thread's stack); 0 runs the handler on the
 * current stack. regs, when known, becomes uc_mcontext. */
static void compat_threads_signal_run_one(compat_threads_CompatThread *t, int signo, uintptr_t stack_top, const compat_threads_SignalRegs *regs) {
    compat_threads_SsrnxSigAction action;
    if (!compat_threads_signal_action_take(signo, &action)) return;
    const uint32_t flags = (uint32_t)action.sa_flags;
    const uint64_t saved_mask = __atomic_load_n(&t->sig_blocked, __ATOMIC_ACQUIRE);
    uint64_t handler_mask = saved_mask | action.sa_mask;
    if ((flags & SSRNX_ANDROID_SA_NODEFER) == 0) handler_mask |= SIG_BIT(signo);
    handler_mask &= ~SIG_UNBLOCKABLE;

    bool on_altstack = false;
    if ((flags & SSRNX_ANDROID_SA_ONSTACK) != 0 &&
        t->sig_altstack.ss_flags != SSRNX_ANDROID_SS_DISABLE && !t->sig_on_altstack) {
        stack_top = t->sig_altstack.ss_sp + t->sig_altstack.ss_size;
        on_altstack = true;
    }

    compat_threads_SignalFrame local_frame;
    compat_threads_SignalFrame *frame = &local_frame;
    if (stack_top) frame = (compat_threads_SignalFrame *)((stack_top - sizeof(compat_threads_SignalFrame)) & ~(uintptr_t)15);
    memset(frame, 0, sizeof(*frame));
    frame->info.si_signo = signo;
    frame->info.si_code = SSRNX_ANDROID_SI_TKILL;
    frame->info.sender_pid = (int32_t)getpid();
    frame->info.sender_uid = SSRNX_ANDROID_APP_UID;
    frame->uc.uc_stack = t->sig_altstack;
    if (t->sig_on_altstack) frame->uc.uc_stack.ss_flags |= SSRNX_ANDROID_SS_ONSTACK;
    frame->uc.uc_sigmask = saved_mask;
    if (regs) compat_threads_signal_fill_mcontext(&frame->uc.uc_mcontext, regs);

    __atomic_store_n(&t->sig_blocked, handler_mask, __ATOMIC_RELEASE);
    if (on_altstack) t->sig_on_altstack = 1;
    compat_threads_SignalCall call = {
        action.handler, signo, (flags & SSRNX_ANDROID_SA_SIGINFO) != 0, &frame->info, &frame->uc
    };
    if (stack_top)
        ssrnx_signal_call_on_stack(((uintptr_t)frame - 64u) & ~(uintptr_t)15, compat_threads_signal_call_handler, &call);
    else
        compat_threads_signal_call_handler(&call);
    if (on_altstack) t->sig_on_altstack = 0;

    /* rt_sigreturn: restore the mask saved in the frame. */
    __atomic_store_n(&t->sig_blocked, frame->uc.uc_sigmask & ~SIG_UNBLOCKABLE, __ATOMIC_RELEASE);
    __atomic_add_fetch(&t->sig_delivered, 1u, __ATOMIC_ACQ_REL);
    compat_threads_signal_notify(t);
}

static void compat_threads_signal_deliver_self(compat_threads_CompatThread *self) {
    int signo = 0;
    while (compat_threads_signal_claim(self, &signo)) compat_threads_signal_run_one(self, signo, 0, NULL);
}

/* Run t's deliverable signals on this (proxy) host thread while acting as t. */
static void compat_threads_signal_deliver_as(compat_threads_CompatThread *t, uintptr_t stack_top, uintptr_t tp, const compat_threads_SignalRegs *regs) {
    compat_threads_ensure_internal_keys();
    void *saved_self = pthread_getspecific(compat_threads_g_current_thread_key);
    void *saved_table = compat_threads_g_guest_key_table_ready ? pthread_getspecific(compat_threads_g_guest_key_table_key) : NULL;
    void *saved_emutls = pthread_getspecific(compat_threads_g_emutls_array_key);
    const uintptr_t saved_tp = compat_threads_android_read_tpidr_el0();
    ++compat_threads_g_signal_proxy_depth;
    (void)pthread_setspecific(compat_threads_g_current_thread_key, t);
    if (compat_threads_g_guest_key_table_ready) (void)pthread_setspecific(compat_threads_g_guest_key_table_key, t->key_table);
    (void)pthread_setspecific(compat_threads_g_emutls_array_key, t->emutls_array);
    if (tp) compat_threads_android_write_tpidr_el0(tp);

    int signo = 0;
    while (compat_threads_signal_claim(t, &signo)) compat_threads_signal_run_one(t, signo, stack_top, regs);

    /* Tables a handler created on the thread's behalf belong to the thread. */
    if (compat_threads_g_guest_key_table_ready) t->key_table = pthread_getspecific(compat_threads_g_guest_key_table_key);
    t->emutls_array = pthread_getspecific(compat_threads_g_emutls_array_key);
    if (tp) compat_threads_android_write_tpidr_el0(saved_tp);
    (void)pthread_setspecific(compat_threads_g_emutls_array_key, saved_emutls);
    if (compat_threads_g_guest_key_table_ready) (void)pthread_setspecific(compat_threads_g_guest_key_table_key, saved_table);
    (void)pthread_setspecific(compat_threads_g_current_thread_key, saved_self);
    --compat_threads_g_signal_proxy_depth;
}

#ifdef __SWITCH__
static uintptr_t compat_threads_g_runtime_text_start;
static uintptr_t compat_threads_g_runtime_text_end;
static pthread_once_t compat_threads_g_runtime_text_once = PTHREAD_ONCE_INIT;

/* SSRNX and libnx share one executable region. */
static void compat_threads_runtime_text_init(void) {
    MemoryInfo info;
    u32 page_info = 0;
    memset(&info, 0, sizeof(info));
    if (R_SUCCEEDED(svcQueryMemory(&info, &page_info, (u64)(uintptr_t)&compat_threads_runtime_text_init))) {
        compat_threads_g_runtime_text_start = (uintptr_t)info.addr;
        compat_threads_g_runtime_text_end = (uintptr_t)(info.addr + info.size);
    }
}

/* Stop t and capture its registers. 0: stopped in guest code. EAGAIN: it was
 * inside SSRNX/libnx, where it may hold host locks, and has been resumed.
 * ESRCH: it no longer runs. */
static int compat_threads_signal_pause(compat_threads_CompatThread *t, compat_threads_SignalRegs *regs, uintptr_t *tp) {
    (void)pthread_once(&compat_threads_g_runtime_text_once, compat_threads_runtime_text_init);
    if (R_FAILED(svcSetThreadActivity(t->host_handle, ThreadActivity_Paused))) return ESRCH;
    ThreadContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    if (R_FAILED(svcGetThreadContext3(&ctx, t->host_handle))) {
        (void)svcSetThreadActivity(t->host_handle, ThreadActivity_Runnable);
        return EAGAIN;
    }
    const uintptr_t pc = (uintptr_t)ctx.pc.x;
    if (compat_threads_g_runtime_text_end == 0 || (pc >= compat_threads_g_runtime_text_start && pc < compat_threads_g_runtime_text_end)) {
        (void)svcSetThreadActivity(t->host_handle, ThreadActivity_Runnable);
        return EAGAIN;
    }
    memset(regs, 0, sizeof(*regs));
    for (int i = 0; i < 29; ++i) regs->x[i] = ctx.cpu_gprs[i].x;
    regs->x[29] = ctx.fp;
    regs->x[30] = ctx.lr;
    regs->sp = ctx.sp;
    regs->pc = pc;
    regs->pstate = ctx.psr;
    regs->fpsr = ctx.fpsr;
    regs->fpcr = ctx.fpcr;
    for (int i = 0; i < 32; ++i) memcpy(regs->v[i], &ctx.fpu_gprs[i], sizeof(regs->v[i]));
    regs->has_fp = true;
    *tp = (uintptr_t)ctx.tpidr;
    return 0;
}

static void compat_threads_signal_resume(compat_threads_CompatThread *t) {
    (void)svcSetThreadActivity(t->host_handle, ThreadActivity_Runnable);
}
#else
/* Host regressions stop a thread with a host signal whose handler parks it;
 * the frame then goes below the parked handler. */
static void compat_threads_host_pause_handler(int sig, siginfo_t *info, void *ucontext_raw) {
    (void)sig;
    (void)info;
    const int saved_errno = errno;
    compat_threads_CompatThread *t = (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
    if (t) {
        const ucontext_t *uc = (const ucontext_t *)ucontext_raw;
#if defined(__x86_64__)
        t->host_pause_pc = (uintptr_t)uc->uc_mcontext.gregs[REG_RIP];
#elif defined(__aarch64__)
        t->host_pause_pc = (uintptr_t)uc->uc_mcontext.pc;
#else
        (void)uc;
#endif
        t->host_pause_sp = signal_current_sp() - 4096u;
        __atomic_store_n(&t->host_paused, 1u, __ATOMIC_SEQ_CST);
        compat_threads_addr_wake(&t->host_paused, -1);
        while (__atomic_load_n(&t->host_paused, __ATOMIC_ACQUIRE) == 1u) compat_threads_addr_wait(&t->host_paused, 1u, -1);
    }
    errno = saved_errno;
}

static pthread_once_t compat_threads_g_host_pause_once = PTHREAD_ONCE_INIT;
static void compat_threads_host_pause_install(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = compat_threads_host_pause_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigfillset(&sa.sa_mask);
    (void)sigaction(SIGURG, &sa, NULL);
}

static int compat_threads_signal_pause(compat_threads_CompatThread *t, compat_threads_SignalRegs *regs, uintptr_t *tp) {
    (void)pthread_once(&compat_threads_g_host_pause_once, compat_threads_host_pause_install);
    __atomic_store_n(&t->host_paused, 0u, __ATOMIC_SEQ_CST);
    if (pthread_kill(t->host, SIGURG) != 0) return ESRCH;
    while (__atomic_load_n(&t->host_paused, __ATOMIC_ACQUIRE) != 1u) compat_threads_addr_wait(&t->host_paused, 0u, -1);
    memset(regs, 0, sizeof(*regs));
    regs->sp = t->host_pause_sp;
    regs->pc = t->host_pause_pc;
    *tp = 0;
    return 0;
}

static void compat_threads_signal_resume(compat_threads_CompatThread *t) {
    __atomic_store_n(&t->host_paused, 2u, __ATOMIC_SEQ_CST);
    compat_threads_addr_wake(&t->host_paused, -1);
}
#endif

static uintptr_t compat_threads_signal_safe_stack_top(const compat_threads_CompatThread *t) {
    const uintptr_t sp = t->sig_safe_sp;
    uintptr_t gap = SIGNAL_SAFE_GAP;
    if (t->stack_base && t->stack_size) {
        const uintptr_t low = (uintptr_t)t->stack_base;
        if (sp > low && sp <= low + t->stack_size && (sp - low) / 4u < gap) gap = (sp - low) / 4u;
    }
    return (sp - gap) & ~(uintptr_t)15;
}

/* Give up ownership of t's delivery. Returns false when a signal arrived
 * meanwhile (KICK) and the caller must keep going. On true this was the last
 * access to t: an exiting thread waits for ASSIGNED to clear, then may be
 * freed. */
static bool compat_threads_signal_release_assignment(compat_threads_CompatThread *t) {
    uint32_t state = __atomic_load_n(&t->sig_state, __ATOMIC_ACQUIRE);
    for (;;) {
        if (state & SIG_STATE_KICK) {
            if (__atomic_compare_exchange_n(&t->sig_state, &state, state & ~SIG_STATE_KICK, false,
                                            __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
                return false;
            continue;
        }
        if (__atomic_compare_exchange_n(&t->sig_state, &state, state & ~SIG_STATE_ASSIGNED, false,
                                        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
            return true;
    }
}

static void compat_threads_signal_finish_delivery(compat_threads_CompatThread *t) {
    __atomic_and_fetch(&t->sig_state, ~SIG_STATE_DELIVERING, __ATOMIC_ACQ_REL);
    __atomic_add_fetch(&t->sig_done, 1u, __ATOMIC_ACQ_REL);
    compat_threads_addr_wake(&t->sig_done, -1);
}

static void compat_threads_signal_proxy_job(compat_threads_CompatThread *t) {
    unsigned attempts = 0;
    bool reported = false;
    uintptr_t last_pc = 0;
    for (;;) {
        uint32_t state = __atomic_load_n(&t->sig_state, __ATOMIC_ACQUIRE);
        if ((state & (SIG_STATE_EXITING | SIG_STATE_SUSPENDED | SIG_STATE_DELIVERING)) != 0 || !compat_threads_signal_has_ready(t)) {
            if (compat_threads_signal_release_assignment(t)) return;
            continue;
        }
        if ((state & (SIG_STATE_SAFE | SIG_STATE_RETURNING)) == (SIG_STATE_SAFE | SIG_STATE_RETURNING)) {
            /* Assignment may predate entry to the bounded wait. Let the
             * target run its handler instead of occupying a host proxy. */
            if (compat_threads_signal_release_assignment(t)) return;
            continue;
        }
        if (state & SIG_STATE_SAFE) {
            if (!__atomic_compare_exchange_n(&t->sig_state, &state, state | SIG_STATE_DELIVERING, false,
                                             __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
                continue;
            compat_threads_signal_deliver_as(t, compat_threads_signal_safe_stack_top(t), t->sig_safe_tp, NULL);
            compat_threads_signal_finish_delivery(t);
            attempts = 0;
            continue;
        }

        compat_threads_SignalRegs regs;
        uintptr_t tp = 0;
        const int rc = compat_threads_signal_pause(t, &regs, &tp);
        if (rc == ESRCH) {
            /* The host thread is gone; nothing is left to deliver to. */
            __atomic_store_n(&t->sig_pending, 0, __ATOMIC_RELEASE);
            continue;
        }
        if (rc == 0) {
            state = __atomic_load_n(&t->sig_state, __ATOMIC_ACQUIRE);
            if ((state & (SIG_STATE_SAFE | SIG_STATE_SUSPENDED | SIG_STATE_EXITING | SIG_STATE_DELIVERING)) == 0) {
                __atomic_or_fetch(&t->sig_state, SIG_STATE_DELIVERING, __ATOMIC_ACQ_REL);
                compat_threads_signal_deliver_as(t, (regs.sp - SIGNAL_RED_ZONE) & ~(uintptr_t)15, tp, &regs);
                compat_threads_signal_resume(t);
                compat_threads_signal_finish_delivery(t);
                attempts = 0;
                continue;
            }
            last_pc = regs.pc;
            compat_threads_signal_resume(t);
            if (state & SIG_STATE_SAFE) continue;
        }
        /* Inside SSRNX code: let it return to guest code or reach a safe point. */
        ++attempts;
        struct timespec nap = {0, attempts < 20u ? 50000L : 1000000L};
        (void)nanosleep(&nap, NULL);
        if (!reported && attempts == 1000u && ssrnx_log_category) {
            reported = true;
            ssrnx_log_category("SIGNAL", "WARN",
                               "signal delivery to thread=%llu delayed: target has stayed inside APN64 code "
                               "outside a safe point for ~1s (pending=%#llx pc=%#llx)",
                               (unsigned long long)t->guest_id,
                               (unsigned long long)__atomic_load_n(&t->sig_pending, __ATOMIC_ACQUIRE),
                               (unsigned long long)last_pc);
        }
    }
}

typedef struct compat_threads_SignalProxy {
    struct compat_threads_SignalProxy *next;
    compat_threads_CompatThread *job;
    uint32_t wake;
    bool stop;
    pthread_t host;
} compat_threads_SignalProxy;

static pthread_mutex_t compat_threads_g_signal_proxy_lock = PTHREAD_MUTEX_INITIALIZER;
static compat_threads_SignalProxy *compat_threads_g_signal_proxy_idle;
/* Jobs waiting for a proxy when no idle one exists and no new host thread can
 * be created (Horizon's per-process thread limit). Every queued thread holds
 * ASSIGNED, so it stays alive until a proxy finishes with it. */
static compat_threads_CompatThread *compat_threads_g_signal_job_head, *compat_threads_g_signal_job_tail;
static unsigned compat_threads_g_signal_proxy_count;

/* A queued signal may be waiting behind a handler that itself waits for the
 * guest collector. Snapshot all threads: the normal wait report caps at 64. */
static void compat_threads_signal_log_wait_states(const char *reason) {
    if (!ssrnx_log_category) return;
    struct {
        uint64_t id, pending, blocked;
        uint32_t state, delivered;
        char name[16];
        const char *wait_kind;
        const void *wait_object;
        uintptr_t wait_caller;
    } snapshots[64];
    size_t count = 0, assigned = 0, delivering = 0, suspended = 0, pending = 0, returning = 0;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    for (compat_threads_CompatThread *t = compat_threads_g_threads; t; t = t->next) {
        if (t->exited) continue;
        const uint32_t state = __atomic_load_n(&t->sig_state, __ATOMIC_ACQUIRE);
        const uint64_t bits = __atomic_load_n(&t->sig_pending, __ATOMIC_ACQUIRE);
        assigned += (state & SIG_STATE_ASSIGNED) != 0;
        delivering += (state & SIG_STATE_DELIVERING) != 0;
        suspended += (state & SIG_STATE_SUSPENDED) != 0;
        pending += bits != 0;
        returning += (state & (SIG_STATE_SAFE | SIG_STATE_RETURNING)) == (SIG_STATE_SAFE | SIG_STATE_RETURNING);
        if (!bits && !(state & (SIG_STATE_ASSIGNED | SIG_STATE_DELIVERING | SIG_STATE_SUSPENDED))) continue;
        if (count == sizeof(snapshots) / sizeof(snapshots[0])) continue;
        snapshots[count].id = t->guest_id;
        snapshots[count].pending = bits;
        snapshots[count].blocked = __atomic_load_n(&t->sig_blocked, __ATOMIC_ACQUIRE);
        snapshots[count].state = state;
        memcpy(snapshots[count].name, t->name, sizeof(t->name));
        snapshots[count].wait_kind = t->wait_kind;
        snapshots[count].wait_object = t->wait_object;
        snapshots[count].wait_caller = t->wait_caller;
        snapshots[count++].delivered = __atomic_load_n(&t->sig_delivered, __ATOMIC_ACQUIRE);
    }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    size_t queued = 0, idle = 0;
    SSRNX_PERF_LOCK(&compat_threads_g_signal_proxy_lock);
    for (compat_threads_CompatThread *t = compat_threads_g_signal_job_head; t; t = t->sig_job_next) queued++;
    for (compat_threads_SignalProxy *p = compat_threads_g_signal_proxy_idle; p; p = p->next) idle++;
    pthread_mutex_unlock(&compat_threads_g_signal_proxy_lock);
    ssrnx_log_category("SIGNAL", "INFO",
        "stall-report reason=%s proxies=%u idle=%zu queued=%zu assigned=%zu delivering=%zu suspended=%zu pendingThreads=%zu returningWaits=%zu reported=%zu",
        reason ? reason : "-", __atomic_load_n(&compat_threads_g_signal_proxy_count, __ATOMIC_ACQUIRE),
        idle, queued, assigned, delivering, suspended, pending, returning, count);
    for (size_t i = 0; i < count; ++i)
        ssrnx_log_category("SIGNAL", "INFO",
            "stall-report reason=%s thread=%llu name=%s state=%#x pending=%#llx blocked=%#llx delivered=%u wait=%s object=%p caller=%#llx",
            reason ? reason : "-", (unsigned long long)snapshots[i].id,
            snapshots[i].name[0] ? snapshots[i].name : "-", snapshots[i].state,
            (unsigned long long)snapshots[i].pending, (unsigned long long)snapshots[i].blocked,
            snapshots[i].delivered, snapshots[i].wait_kind ? snapshots[i].wait_kind : "-",
            snapshots[i].wait_object, (unsigned long long)snapshots[i].wait_caller);
}

static compat_threads_CompatThread *compat_threads_signal_job_pop_locked(void) {
    compat_threads_CompatThread *job = compat_threads_g_signal_job_head;
    if (job) {
        compat_threads_g_signal_job_head = job->sig_job_next;
        if (!compat_threads_g_signal_job_head) compat_threads_g_signal_job_tail = NULL;
        job->sig_job_next = NULL;
    }
    return job;
}

static void *compat_threads_signal_proxy_main(void *arg) {
    compat_threads_SignalProxy *proxy = (compat_threads_SignalProxy *)arg;
    for (;;) {
        const uint32_t wake = __atomic_load_n(&proxy->wake, __ATOMIC_ACQUIRE);
        if (__atomic_load_n(&proxy->stop, __ATOMIC_ACQUIRE)) return NULL;
        compat_threads_CompatThread *job = __atomic_exchange_n(&proxy->job, NULL, __ATOMIC_ACQ_REL);
        if (!job) {
            compat_threads_addr_wait(&proxy->wake, wake, -1);
            continue;
        }
        while (job) {
            compat_threads_signal_proxy_job(job);
            SSRNX_PERF_LOCK(&compat_threads_g_signal_proxy_lock);
            job = compat_threads_signal_job_pop_locked();
            if (!job) {
                proxy->next = compat_threads_g_signal_proxy_idle;
                compat_threads_g_signal_proxy_idle = proxy;
            }
            pthread_mutex_unlock(&compat_threads_g_signal_proxy_lock);
        }
    }
    return NULL;
}

/* Idle proxies are a cache of real host threads. Keep one idle reserve and
 * never retire a proxy with a queued or running signal handler. */
static bool compat_threads_signal_proxy_reclaim_idle(void) {
    SSRNX_PERF_LOCK(&compat_threads_g_signal_proxy_lock);
    compat_threads_SignalProxy *proxy = compat_threads_g_signal_proxy_idle;
    if (!proxy || !proxy->next) {
        pthread_mutex_unlock(&compat_threads_g_signal_proxy_lock);
        return false;
    }
    compat_threads_g_signal_proxy_idle = proxy->next;
    pthread_mutex_unlock(&compat_threads_g_signal_proxy_lock);
    __atomic_store_n(&proxy->stop, true, __ATOMIC_RELEASE);
    __atomic_add_fetch(&proxy->wake, 1u, __ATOMIC_ACQ_REL);
    compat_threads_addr_wake(&proxy->wake, -1);
    /* Join closes the Horizon handle and frees the stack before retrying. */
    const int rc = pthread_join(proxy->host, NULL);
    if (rc != 0) {
        if (ssrnx_log_category)
            ssrnx_log_category("SIGNAL", "ERROR", "idle signal proxy join failed hostRc=%d", rc);
        return false;
    }
    __atomic_sub_fetch(&compat_threads_g_signal_proxy_count, 1u, __ATOMIC_ACQ_REL);
    free(proxy);
    return true;
}

static void compat_threads_host_thread_pressure_report(int rc) {
    if (!ssrnx_log_category) return;
    size_t live = 0, exited_joinable = 0, exited_detached = 0, retained_results = 0;
    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    for (compat_threads_CompatThread *t = compat_threads_g_threads; t; t = t->next) {
        if (!t->exited) live++;
        else if (!t->external && t->host_join_state == HOST_THREAD_REAPED) retained_results++;
        else if (!t->external) {
            if (t->detached) exited_detached++;
            else exited_joinable++;
        }
    }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    size_t idle = 0, queued = 0;
    SSRNX_PERF_LOCK(&compat_threads_g_signal_proxy_lock);
    for (compat_threads_SignalProxy *p = compat_threads_g_signal_proxy_idle; p; p = p->next) idle++;
    for (compat_threads_CompatThread *t = compat_threads_g_signal_job_head; t; t = t->sig_job_next) queued++;
    pthread_mutex_unlock(&compat_threads_g_signal_proxy_lock);
    ssrnx_log_category("THREAD", "WARN",
        "pthread_create resource failure hostRc=%d live=%zu exitedJoinable=%zu exitedDetached=%zu retainedResults=%zu proxies=%u idle=%zu queued=%zu",
        rc, live, exited_joinable, exited_detached, retained_results,
        __atomic_load_n(&compat_threads_g_signal_proxy_count, __ATOMIC_ACQUIRE), idle, queued);
#ifdef __SWITCH__
    u64 handle_value = 0, memory_total = 0, memory_used = 0;
    s64 thread_limit = -1, thread_used = -1;
    const Result info_rc = svcGetInfo(&handle_value, InfoType_ResourceLimit, INVALID_HANDLE, 0);
    Result limit_rc = info_rc, used_rc = info_rc;
    if (R_SUCCEEDED(info_rc)) {
        limit_rc = svcGetResourceLimitLimitValue(&thread_limit, (Handle)handle_value, LimitableResource_Threads);
        used_rc = svcGetResourceLimitCurrentValue(&thread_used, (Handle)handle_value, LimitableResource_Threads);
        (void)svcCloseHandle((Handle)handle_value);
    }
    const Result total_rc = svcGetInfo(&memory_total, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
    const Result memory_rc = svcGetInfo(&memory_used, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);
    ssrnx_log_category("THREAD", "WARN",
        "pthread_create Horizon resources threads=%lld/%lld threadQueryRc=%#x/%#x/%#x memoryBytes=%llu/%llu memoryQueryRc=%#x/%#x",
        (long long)thread_used, (long long)thread_limit, info_rc, used_rc, limit_rc,
        (unsigned long long)memory_used, (unsigned long long)memory_total, memory_rc, total_rc);
#endif
    static uint64_t last_snapshot_ms;
    static pthread_mutex_t snapshot_lock = PTHREAD_MUTEX_INITIALIZER;
    const uint64_t now = compat_threads_wait_clock_ms();
    pthread_mutex_lock(&snapshot_lock);
    const bool snapshot = !last_snapshot_ms || now - last_snapshot_ms >= 5000u;
    if (snapshot) last_snapshot_ms = now;
    pthread_mutex_unlock(&snapshot_lock);
    if (snapshot) (void)ssrnx_bionic_pthread_log_wait_states("pthread-create-resource-failure");
    else compat_threads_signal_log_wait_states("pthread-create-resource-failure");
}

int ssrnx_host_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                              void *(*start)(void *), void *arg) {
    const int initial_rc = pthread_create(thread, attr, start, arg);
    if (initial_rc != ENOMEM && initial_rc != EAGAIN) return initial_rc;
    int retry_rc = initial_rc;
    const size_t native_reaped = compat_threads_reap_detached_threads() + compat_threads_reap_exited_joinable_threads();
    if (native_reaped) {
        retry_rc = pthread_create(thread, attr, start, arg);
        if (ssrnx_log_category)
            ssrnx_log_category("THREAD", "INFO",
                "pthread_create reaped exited native workers count=%zu initialHostRc=%d retryHostRc=%d",
                native_reaped, initial_rc, retry_rc);
        if (retry_rc != ENOMEM && retry_rc != EAGAIN) return retry_rc;
    }
    /* Java Thread.start's cleanup cannot release workers that finish before
     * a native pthread_create. They share the same host quota and allocator. */
    const size_t reaped = ssrnx_android_reap_exited_java_threads
        ? ssrnx_android_reap_exited_java_threads() : 0;
    if (reaped) {
        retry_rc = pthread_create(thread, attr, start, arg);
        if (ssrnx_log_category)
            ssrnx_log_category("THREAD", "INFO",
                "pthread_create reaped exited Java workers count=%zu initialHostRc=%d retryHostRc=%d",
                reaped, initial_rc, retry_rc);
    }
    /* A large requested stack may require more than one cached proxy stack.
     * Bound recovery by the starting cache size; preserve the idle reserve. */
    unsigned budget = __atomic_load_n(&compat_threads_g_signal_proxy_count, __ATOMIC_ACQUIRE);
    unsigned reclaimed = 0;
    while ((retry_rc == ENOMEM || retry_rc == EAGAIN) && budget-- && compat_threads_signal_proxy_reclaim_idle()) {
        reclaimed++;
        retry_rc = pthread_create(thread, attr, start, arg);
    }
    if (reclaimed && ssrnx_log_category)
        ssrnx_log_category("THREAD", "INFO",
            "pthread_create reclaimed idle signal proxy count=%u initialHostRc=%d retryHostRc=%d proxies=%u",
            reclaimed, initial_rc, retry_rc, __atomic_load_n(&compat_threads_g_signal_proxy_count, __ATOMIC_ACQUIRE));
    if (retry_rc == ENOMEM || retry_rc == EAGAIN) compat_threads_host_thread_pressure_report(retry_rc);
    return retry_rc;
}

/* Start a proxy thread; it is not linked anywhere yet. */
static compat_threads_SignalProxy *compat_threads_signal_proxy_spawn(void) {
    compat_threads_SignalProxy *proxy = (compat_threads_SignalProxy *)calloc(1, sizeof(*proxy));
    if (!proxy) return NULL;
    pthread_attr_t attr;
    int rc = pthread_attr_init(&attr);
    if (rc == 0) {
        (void)pthread_attr_setstacksize(&attr, SIGNAL_PROXY_STACK);
        rc = pthread_create(&proxy->host, &attr, compat_threads_signal_proxy_main, proxy);
        (void)pthread_attr_destroy(&attr);
    }
    if (rc != 0) {
        free(proxy);
        static bool reported;
        if (!reported && ssrnx_log_category) {
            reported = true;
            ssrnx_log_category("SIGNAL", "WARN", "signal proxy thread unavailable hostRc=%d proxies=%u; "
                               "deliveries queue for existing proxies", rc,
                               __atomic_load_n(&compat_threads_g_signal_proxy_count, __ATOMIC_RELAXED));
        }
        return NULL;
    }
    __atomic_add_fetch(&compat_threads_g_signal_proxy_count, 1u, __ATOMIC_RELAXED);
    return proxy;
}

/* A guest that installs a signal handler will send signals later, possibly
 * after its own threads have used up the host thread limit. Keep one proxy
 * from that point on so delivery never depends on creating a thread then. */
static void compat_threads_signal_proxy_reserve(void) {
    if (__atomic_load_n(&compat_threads_g_signal_proxy_count, __ATOMIC_ACQUIRE) != 0) return;
    SSRNX_PERF_LOCK(&compat_threads_g_signal_proxy_lock);
    const bool need = __atomic_load_n(&compat_threads_g_signal_proxy_count, __ATOMIC_RELAXED) == 0 && !compat_threads_g_signal_proxy_idle;
    pthread_mutex_unlock(&compat_threads_g_signal_proxy_lock);
    if (!need) return;
    compat_threads_SignalProxy *proxy = compat_threads_signal_proxy_spawn();
    if (!proxy) return;
    SSRNX_PERF_LOCK(&compat_threads_g_signal_proxy_lock);
    proxy->next = compat_threads_g_signal_proxy_idle;
    compat_threads_g_signal_proxy_idle = proxy;
    pthread_mutex_unlock(&compat_threads_g_signal_proxy_lock);
}

static void compat_threads_signal_proxy_submit(compat_threads_CompatThread *t) {
    SSRNX_PERF_LOCK(&compat_threads_g_signal_proxy_lock);
    compat_threads_SignalProxy *proxy = compat_threads_g_signal_proxy_idle;
    if (proxy) compat_threads_g_signal_proxy_idle = proxy->next;
    pthread_mutex_unlock(&compat_threads_g_signal_proxy_lock);
    if (!proxy) proxy = compat_threads_signal_proxy_spawn();
    if (!proxy) {
        bool queued = false;
        SSRNX_PERF_LOCK(&compat_threads_g_signal_proxy_lock);
        /* Recheck: a proxy may have gone idle since the first look. */
        proxy = compat_threads_g_signal_proxy_idle;
        if (proxy) {
            compat_threads_g_signal_proxy_idle = proxy->next;
        } else if (__atomic_load_n(&compat_threads_g_signal_proxy_count, __ATOMIC_RELAXED) != 0) {
            t->sig_job_next = NULL;
            if (compat_threads_g_signal_job_tail) compat_threads_g_signal_job_tail->sig_job_next = t;
            else compat_threads_g_signal_job_head = t;
            compat_threads_g_signal_job_tail = t;
            queued = true;
        }
        pthread_mutex_unlock(&compat_threads_g_signal_proxy_lock);
        if (!proxy) {
            if (queued) return; /* a busy proxy takes it next */
            if (ssrnx_log_category)
                ssrnx_log_category("SIGNAL", "ERROR", "no signal proxy thread for thread=%llu; signal stays pending",
                                   (unsigned long long)t->guest_id);
            /* The thread still takes it itself when it unblocks or suspends. */
            while (!compat_threads_signal_release_assignment(t)) {}
            return;
        }
    }
    __atomic_store_n(&proxy->job, t, __ATOMIC_RELEASE);
    __atomic_add_fetch(&proxy->wake, 1u, __ATOMIC_ACQ_REL);
    compat_threads_addr_wake(&proxy->wake, 1);
}

/* A safe point: op->run blocks without executing guest code. This frame
 * stays live for the whole wait, holds every callee-saved register (spilled
 * by __builtin_unwind_init) above the recorded stack pointer, and is not left
 * until a handler running for this thread has returned. op->release drops
 * whatever run() returned holding before that wait. */
typedef struct compat_threads_SignalSafeOp {
    long (*run)(void *ctx);
    void (*release)(void *ctx, long result);
    void *ctx;
    bool target_delivery; /* run returns promptly without a guest-owned lock */
} compat_threads_SignalSafeOp;

__attribute__((noinline)) static long compat_threads_signal_safe_run(const compat_threads_SignalSafeOp *op, bool *interrupted) {
    __builtin_unwind_init();
    SSRNX_PERF_SCOPE(SSRNX_PERF_GUEST_WAIT);
    *interrupted = false;
    compat_threads_ensure_internal_keys();
    compat_threads_CompatThread *self = (compat_threads_g_signal_proxy_depth || compat_threads_g_signal_target_delivery_depth) ? NULL
                                              : (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
    if (!self) return op->run(op->ctx);

    self->wait_kind = compat_threads_t_wait_kind ? compat_threads_t_wait_kind : "apn64-internal";
    self->wait_object = compat_threads_t_wait_object;
    self->wait_caller = compat_threads_t_wait_caller;
    __atomic_store_n(&self->wait_since_ms, compat_threads_wait_clock_ms(), __ATOMIC_RELEASE);
    compat_threads_t_wait_kind = NULL; compat_threads_t_wait_object = NULL; compat_threads_t_wait_caller = 0;

    self->sig_safe_sp = signal_current_sp();
    self->sig_safe_tp = compat_threads_android_read_tpidr_el0();
    __atomic_or_fetch(&self->sig_state, SIG_STATE_SAFE | (op->target_delivery ? SIG_STATE_RETURNING : 0),
                      __ATOMIC_SEQ_CST);
    const long result = op->run(op->ctx);
    const int saved_errno = errno;
    __atomic_store_n(&self->wait_since_ms, 0, __ATOMIC_RELEASE);

    bool released = false;
    for (;;) {
        uint32_t state = __atomic_load_n(&self->sig_state, __ATOMIC_ACQUIRE);
        if (state & SIG_STATE_TARGET_KICK) {
            /* Senders change this bit after publishing pending signals.
             * Clearing it before checking pending prevents a signal from
             * being stranded by the final SAFE compare/exchange. */
            __atomic_and_fetch(&self->sig_state, ~SIG_STATE_TARGET_KICK, __ATOMIC_ACQ_REL);
            continue;
        }
        if (state & SIG_STATE_DELIVERING) {
            *interrupted = true;
            if (!released && op->release) op->release(op->ctx, result);
            released = true;
            const uint32_t done = __atomic_load_n(&self->sig_done, __ATOMIC_ACQUIRE);
            if (__atomic_load_n(&self->sig_state, __ATOMIC_ACQUIRE) & SIG_STATE_DELIVERING)
                compat_threads_addr_wait(&self->sig_done, done, -1);
            continue;
        }
        /* A proxy may be queued behind suspend handlers that wait for this
         * very thread. When a host wait returns, deliver on the real target
         * instead of requiring another host thread to run its handler. */
        if (!(state & SIG_STATE_EXITING) && compat_threads_signal_has_ready(self)) {
            if (!__atomic_compare_exchange_n(&self->sig_state, &state, state | SIG_STATE_DELIVERING, false,
                                             __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) continue;
            *interrupted = true;
            if (!released && op->release) op->release(op->ctx, result);
            released = true;
            if (ssrnx_log_category && __atomic_load_n(&self->sig_delivered, __ATOMIC_ACQUIRE) < 4u)
                ssrnx_log_category("SIGNAL", "INFO", "target-thread delivery after wait thread=%llu pending=%#llx",
                    (unsigned long long)self->guest_id,
                    (unsigned long long)__atomic_load_n(&self->sig_pending, __ATOMIC_ACQUIRE));
            ++compat_threads_g_signal_target_delivery_depth;
            compat_threads_signal_deliver_self(self);
            --compat_threads_g_signal_target_delivery_depth;
            compat_threads_signal_finish_delivery(self);
            continue;
        }
        if (__atomic_compare_exchange_n(&self->sig_state, &state,
                                        state & ~(SIG_STATE_SAFE | SIG_STATE_RETURNING), false,
                                        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
            errno = saved_errno;
            return result;
        }
    }
}

long ssrnx_signal_safe_call(long (*fn)(void *), void *ctx) {
    if (!fn) return -1;
    const compat_threads_SignalSafeOp op = { fn, NULL, ctx, false };
    bool interrupted = false;
    return compat_threads_signal_safe_run(&op, &interrupted);
}

static long compat_threads_safe_mutex_lock_run(void *ctx) {
    return pthread_mutex_lock((pthread_mutex_t *)ctx);
}

static void compat_threads_safe_mutex_lock_release(void *ctx, long result) {
    if (result == 0) pthread_mutex_unlock((pthread_mutex_t *)ctx);
}

static int compat_threads_signal_safe_mutex_lock(pthread_mutex_t *mutex) {
    const char *wait_kind = compat_threads_t_wait_kind;
    const void *wait_object = compat_threads_t_wait_object;
    const uintptr_t wait_caller = compat_threads_t_wait_caller;
    for (;;) {
        const compat_threads_SignalSafeOp op = { compat_threads_safe_mutex_lock_run, compat_threads_safe_mutex_lock_release, mutex, false };
        bool interrupted = false;
        compat_threads_t_wait_kind = wait_kind; compat_threads_t_wait_object = wait_object; compat_threads_t_wait_caller = wait_caller;
        const int rc = (int)compat_threads_signal_safe_run(&op, &interrupted);
        /* After a handler ran, contend again, as a futex waiter would. */
        if (!interrupted || rc != 0) return rc;
    }
}

typedef struct compat_threads_SafeCondWait {
    pthread_cond_t *cond;
    pthread_mutex_t *mutex;
    const struct timespec *deadline;
} compat_threads_SafeCondWait;

static long compat_threads_safe_cond_wait_run(void *ctx) {
    compat_threads_SafeCondWait *wait = (compat_threads_SafeCondWait *)ctx;
    compat_threads_CompatThread *self = (compat_threads_g_signal_proxy_depth || compat_threads_g_signal_target_delivery_depth) ? NULL
                                              : pthread_getspecific(compat_threads_g_current_thread_key);
    if (!self) return wait->deadline ? pthread_cond_timedwait(wait->cond, wait->mutex, wait->deadline)
                                    : pthread_cond_wait(wait->cond, wait->mutex);
    /* A queued signal must not need a spare proxy before an indefinite or
     * long wait can return. Internal slice expiry is a spurious wake, never
     * an expiry of the caller's deadline. All host condvars use REALTIME. */
    if (wait->deadline && (wait->deadline->tv_nsec < 0 || wait->deadline->tv_nsec >= 1000000000L))
        return pthread_cond_timedwait(wait->cond, wait->mutex, wait->deadline);
    for (;;) {
        struct timespec now;
        if (clock_gettime(CLOCK_REALTIME, &now) != 0) return errno ? errno : EINVAL;
        struct timespec slice = compat_threads_ssrnx_timespec_add_ns(now, SIGNAL_WAIT_SLICE_NS);
        const struct timespec *deadline = wait->deadline;
        const bool sliced = !deadline || compat_threads_ssrnx_timespec_cmp(deadline, &slice) > 0;
        if (sliced) deadline = &slice;
        const int rc = pthread_cond_timedwait(wait->cond, wait->mutex, deadline);
        if (!sliced || rc != ETIMEDOUT) return rc;
        if (compat_threads_signal_has_ready(self) ||
            (__atomic_load_n(&self->sig_state, __ATOMIC_ACQUIRE) & SIG_STATE_DELIVERING)) return 0;
    }
}

static void compat_threads_safe_cond_wait_release(void *ctx, long result) {
    compat_threads_SafeCondWait *wait = (compat_threads_SafeCondWait *)ctx;
    if (result == 0 || result == ETIMEDOUT) pthread_mutex_unlock(wait->mutex);
}

static int compat_threads_signal_safe_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                                 const struct timespec *deadline) {
    /* A guest mutex may be owned by another suspended thread: reacquiring
     * it need not return promptly. Such waits still require a proxy. */
    return compat_threads_signal_safe_cond_wait_mode(cond, mutex, deadline, false);
}

static int compat_threads_signal_safe_cond_wait_mode(pthread_cond_t *cond, pthread_mutex_t *mutex,
                                      const struct timespec *deadline, bool target_delivery) {
    compat_threads_SafeCondWait wait = { cond, mutex, deadline };
    const compat_threads_SignalSafeOp op = { compat_threads_safe_cond_wait_run, compat_threads_safe_cond_wait_release, &wait, target_delivery };
    bool interrupted = false;
    const int rc = (int)compat_threads_signal_safe_run(&op, &interrupted);
    if (!interrupted || (rc != 0 && rc != ETIMEDOUT)) return rc;
    /* The handler ran without the mutex held; take it back and report the
     * wait as woken (a permitted spurious wakeup) or timed out. */
    const int lock_rc = compat_threads_signal_safe_mutex_lock(mutex);
    return lock_rc ? lock_rc : rc;
}

typedef struct compat_threads_SafeJoin {
    pthread_t host;
    void *result;
} compat_threads_SafeJoin;

static long compat_threads_safe_join_run(void *ctx) {
    compat_threads_SafeJoin *join = (compat_threads_SafeJoin *)ctx;
    return pthread_join(join->host, &join->result);
}

static int compat_threads_signal_safe_join(pthread_t host, void **result) {
    compat_threads_SafeJoin join = { host, NULL };
    const compat_threads_SignalSafeOp op = { compat_threads_safe_join_run, NULL, &join, false };
    bool interrupted = false;
    const int rc = (int)compat_threads_signal_safe_run(&op, &interrupted);
    if (result) *result = join.result;
    return rc;
}

typedef struct compat_threads_SafeAddrWait {
    uint32_t *addr;
    uint32_t expected;
    int64_t timeout_ns;
} compat_threads_SafeAddrWait;

static long compat_threads_safe_addr_wait_run(void *ctx) {
    compat_threads_SafeAddrWait *wait = (compat_threads_SafeAddrWait *)ctx;
    /* Semaphore callers recheck their count and original deadline. */
    const int64_t timeout = wait->timeout_ns < 0 || wait->timeout_ns > SIGNAL_WAIT_SLICE_NS
                            ? SIGNAL_WAIT_SLICE_NS : wait->timeout_ns;
    compat_threads_addr_wait(wait->addr, wait->expected, timeout);
    return 0;
}

static void compat_threads_signal_safe_addr_wait(uint32_t *addr, uint32_t expected, int64_t timeout_ns) {
    compat_threads_SafeAddrWait wait = { addr, expected, timeout_ns };
    const compat_threads_SignalSafeOp op = { compat_threads_safe_addr_wait_run, NULL, &wait, true };
    bool interrupted = false;
    (void)compat_threads_signal_safe_run(&op, &interrupted);
}

static void compat_threads_signal_thread_exit(compat_threads_CompatThread *thread) {
    /* Signals generated from now on are discarded. A proxy already working
     * for this thread may still read its state, so outlive it. */
    __atomic_or_fetch(&thread->sig_state, SIG_STATE_EXITING, __ATOMIC_ACQ_REL);
    while (__atomic_load_n(&thread->sig_state, __ATOMIC_ACQUIRE) & SIG_STATE_ASSIGNED) {
        struct timespec nap = {0, 1000000L};
        (void)nanosleep(&nap, NULL);
    }
}

/* With g_registry_lock held (t cannot be freed): true when the caller must
 * hand t to a proxy. A thread in sigsuspend takes its own signals. */
static bool compat_threads_signal_request_delivery_locked(compat_threads_CompatThread *t) {
    uint32_t state = __atomic_load_n(&t->sig_state, __ATOMIC_ACQUIRE);
    for (;;) {
        if (state & (SIG_STATE_EXITING | SIG_STATE_SUSPENDED)) return false;
        if ((state & (SIG_STATE_SAFE | SIG_STATE_RETURNING)) == (SIG_STATE_SAFE | SIG_STATE_RETURNING)) {
            if (__atomic_compare_exchange_n(&t->sig_state, &state, state | SIG_STATE_TARGET_KICK, false,
                                            __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) return false;
            continue;
        }
        const uint32_t next = (state & SIG_STATE_ASSIGNED) ? (state | SIG_STATE_KICK)
                                                           : (state | SIG_STATE_ASSIGNED);
        if (__atomic_compare_exchange_n(&t->sig_state, &state, next, false,
                                        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
            return (state & SIG_STATE_ASSIGNED) == 0;
    }
}

/* Only the pinned thread's registered stack is read, while it is paused. */
#ifdef __SWITCH__
static void compat_threads_native_stack_copy(const compat_threads_CompatThread *target, uintptr_t pc, uintptr_t lr,
                              uintptr_t sp, uintptr_t fp, SsrnxNativeContext *out) {
    out->stack[out->stack_count++] = pc;
    if (lr && lr != pc) out->stack[out->stack_count++] = lr;
    const uintptr_t base = (uintptr_t)target->stack_base;
    if (!target->stack_ready || target->stack_size < 2u * sizeof(uintptr_t) ||
        base > UINTPTR_MAX - target->stack_size) { out->stack_status = 3; return; }
    const uintptr_t end = base + target->stack_size;
    if (sp < base || sp > end) { out->stack_status = 1; return; }
    unsigned walked = 0;
    while (fp) {
        if (walked++ == SSRNX_NATIVE_STACK_CAP) { out->stack_status = 2; return; }
        if ((fp & 15u) || fp < sp || fp > end - 2u * sizeof(uintptr_t)) {
            out->stack_status = 1; return;
        }
        const uintptr_t *record = (const uintptr_t *)fp;
        const uintptr_t next = record[0], address = record[1];
        if (address && address != out->stack[out->stack_count - 1u]) {
            if (out->stack_count == SSRNX_NATIVE_STACK_CAP) { out->stack_status = 2; return; }
            out->stack[out->stack_count++] = address;
        }
        if (next && next <= fp) { out->stack_status = 1; return; }
        sp = fp + 2u * sizeof(uintptr_t);
        fp = next;
    }
}
#endif

static int compat_threads_native_context_sample(pthread_t host, SsrnxNativeContext *out, bool probe, bool stack) {
    if (!out || pthread_equal(host, pthread_self())) return EINVAL;
    memset(out, 0, sizeof(*out));
#ifdef __SWITCH__
    /* ASSIGNED pins the node/handle and prevents a signal proxy from pausing
     * the same thread. A concurrent signal sets KICK and is handed to a proxy. */
    if (pthread_mutex_trylock(&compat_threads_g_registry_lock) != 0) {
        if (stack) out->capture_status = 1;
        return EAGAIN;
    }
    compat_threads_CompatThread *target = NULL;
    for (compat_threads_CompatThread *t = compat_threads_g_threads; t; t = t->next) {
        if (t->exited || t->host_handle == INVALID_HANDLE || !pthread_equal(t->host, host)) continue;
        uint32_t expected = 0;
        if (__atomic_compare_exchange_n(&t->sig_state, &expected, SIG_STATE_ASSIGNED, false,
                                         __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) target = t;
        break;
    }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    if (!target) { if (stack) out->capture_status = 2; return EAGAIN; }
    ThreadContext context;
    memset(&context, 0, sizeof(context));
    uintptr_t probe_address = 0;
    uint64_t probe_words[24] = {0};
    int result = EAGAIN;
    const Result paused = svcSetThreadActivity(target->host_handle, ThreadActivity_Paused);
    if (stack) { out->pause_result = paused; out->capture_status = 3; }
    if (R_SUCCEEDED(paused)) {
        const Result captured = svcGetThreadContext3(&context, target->host_handle);
        if (R_SUCCEEDED(captured) && stack)
            compat_threads_native_stack_copy(target, context.pc.x, context.lr, context.sp, context.fp, out);
        if (R_SUCCEEDED(captured) && probe && ssrnx_heap_cache_read_small_block &&
            context.cpu_gprs[19].x <= UINTPTR_MAX - 960u &&
            ssrnx_heap_cache_read_small_block((void *)(uintptr_t)context.cpu_gprs[19].x,
                                             960u, probe_words, sizeof(probe_words)))
            probe_address = (uintptr_t)context.cpu_gprs[19].x + 960u;
        Result resumed = svcSetThreadActivity(target->host_handle, ThreadActivity_Runnable);
        if (R_FAILED(resumed)) resumed = svcSetThreadActivity(target->host_handle, ThreadActivity_Runnable);
        if (stack) {
            out->context_result = captured; out->resume_result = resumed;
            out->capture_status = R_FAILED(resumed) ? 5u : R_FAILED(captured) ? 4u : 0u;
        }
        /* No locks, logging or allocation while paused; optional reads are
         * restricted to permanent allocator storage that cannot be unmapped. */
        if (R_SUCCEEDED(captured) && R_SUCCEEDED(resumed)) {
            out->pc = context.pc.x; out->lr = context.lr; out->sp = context.sp; out->fp = context.fp;
            for (unsigned i = 0; i < 29; ++i) out->x[i] = context.cpu_gprs[i].x;
            if (probe_address) {
                out->probe_address = probe_address;
                memcpy(out->probe_words, probe_words, sizeof(probe_words));
            }
            result = 0;
        } else if (R_FAILED(resumed)) result = EIO;
    }
    if (!compat_threads_signal_release_assignment(target)) compat_threads_signal_proxy_submit(target);
    return result;
#else
    (void)host; (void)probe; (void)stack;
    return ENOTSUP;
#endif
}

int ssrnx_bionic_thread_sample_context(pthread_t host, SsrnxNativeContext *out) {
    return compat_threads_native_context_sample(host, out, false, false);
}

int ssrnx_bionic_thread_sample_stack(pthread_t host, SsrnxNativeContext *out) {
    return compat_threads_native_context_sample(host, out, false, true);
}

int ssrnx_bionic_thread_sample_context_probe(pthread_t host, SsrnxNativeContext *out) {
    return compat_threads_native_context_sample(host, out, true, false);
}

int ssrnx_bionic_thread_sample_pc(pthread_t host, uintptr_t *pc, uintptr_t *lr, uintptr_t *sp) {
    if (!pc || !lr || !sp || pthread_equal(host, pthread_self())) return EINVAL;
    SsrnxNativeContext context;
    const int rc = ssrnx_bionic_thread_sample_context(host, &context);
    *pc = context.pc; *lr = context.lr; *sp = context.sp;
    return rc;
}

int ssrnx_bionic_sigaction(int signo, const void *new_action, void *old_action) {
    if (signo <= 0 || signo >= SSRNX_BIONIC_NSIG ||
        (new_action && (signo == SSRNX_ANDROID_SIGKILL || signo == SSRNX_ANDROID_SIGSTOP))) {
        errno = EINVAL;
        return -1;
    }
    compat_threads_SsrnxSigAction next;
    if (new_action) memcpy(&next, new_action, sizeof(next));
    SSRNX_PERF_LOCK(&compat_threads_g_sig_action_lock);
    if (old_action) memcpy(old_action, &compat_threads_g_sig_actions[signo], sizeof(compat_threads_SsrnxSigAction));
    if (new_action) compat_threads_g_sig_actions[signo] = next;
    pthread_mutex_unlock(&compat_threads_g_sig_action_lock);
    if (new_action && next.handler > 1u) compat_threads_signal_proxy_reserve();
    return 0;
}

int ssrnx_bionic_sigaltstack(const void *new_stack, void *old_stack) {
    compat_threads_CompatThread *self = compat_threads_adopt_current_thread();
    if (!self) { errno = ENOMEM; return -1; }
    if (old_stack) {
        compat_threads_SsrnxSigStack old = self->sig_altstack;
        if (self->sig_on_altstack) old.ss_flags |= SSRNX_ANDROID_SS_ONSTACK;
        memcpy(old_stack, &old, sizeof(old));
    }
    if (!new_stack) return 0;
    compat_threads_SsrnxSigStack next;
    memcpy(&next, new_stack, sizeof(next));
    if (self->sig_on_altstack) { errno = EPERM; return -1; }
    if (next.ss_flags == SSRNX_ANDROID_SS_DISABLE) {
        memset(&self->sig_altstack, 0, sizeof(self->sig_altstack));
        self->sig_altstack.ss_flags = SSRNX_ANDROID_SS_DISABLE;
        return 0;
    }
    if (next.ss_flags != 0) { errno = EINVAL; return -1; }
    if (!next.ss_sp || next.ss_size < SSRNX_ANDROID_MINSIGSTKSZ) { errno = ENOMEM; return -1; }
    next._pad_flags = 0;
    self->sig_altstack = next;
    return 0;
}

int ssrnx_bionic_pthread_kill(SsrnxGuestPthread guest_thread, int signo) {
    if (signo < 0 || signo >= SSRNX_BIONIC_NSIG) return EINVAL;
    compat_threads_ensure_internal_keys();
    compat_threads_CompatThread *self = (compat_threads_CompatThread *)pthread_getspecific(compat_threads_g_current_thread_key);
    if (self && self->guest_id == guest_thread) {
        if (signo == 0 || !compat_threads_signal_generation_kept(self, signo)) return 0;
        __atomic_or_fetch(&self->sig_pending, SIG_BIT(signo), __ATOMIC_ACQ_REL);
        compat_threads_signal_notify(self);
        compat_threads_signal_deliver_self(self);
        return 0;
    }

    SSRNX_PERF_LOCK(&compat_threads_g_registry_lock);
    compat_threads_CompatThread *t = compat_threads_find_thread_locked(guest_thread);
    if (!t || t->exited) {
        pthread_mutex_unlock(&compat_threads_g_registry_lock);
        return ESRCH;
    }
    bool submit = false;
    if (signo != 0 && compat_threads_signal_generation_kept(t, signo)) {
        __atomic_or_fetch(&t->sig_pending, SIG_BIT(signo), __ATOMIC_ACQ_REL);
        compat_threads_signal_notify(t);
        if ((SIG_BIT(signo) & ~__atomic_load_n(&t->sig_blocked, __ATOMIC_ACQUIRE)) != 0)
            submit = compat_threads_signal_request_delivery_locked(t);
    }
    pthread_mutex_unlock(&compat_threads_g_registry_lock);
    /* ASSIGNED keeps t alive until the proxy releases it. */
    if (submit) compat_threads_signal_proxy_submit(t);
    return 0;
}

int ssrnx_bionic_raise(int signo) {
    if (signo <= 0 || signo >= SSRNX_BIONIC_NSIG) { errno = EINVAL; return -1; }
    compat_threads_CompatThread *self = compat_threads_adopt_current_thread();
    if (!self) { errno = ENOMEM; return -1; }
    const int rc = ssrnx_bionic_pthread_kill(self->guest_id, signo);
    if (rc != 0) { errno = rc; return -1; }
    return 0;
}

int ssrnx_bionic_pthread_sigmask(int how, const uint64_t *set, uint64_t *old_set) {
    compat_threads_CompatThread *self = compat_threads_adopt_current_thread();
    if (!self) return ENOMEM;
    const uint64_t current = __atomic_load_n(&self->sig_blocked, __ATOMIC_ACQUIRE);
    uint64_t next = current;
    if (set) {
        switch (how) {
            case SSRNX_ANDROID_SIG_BLOCK: next = current | *set; break;
            case SSRNX_ANDROID_SIG_UNBLOCK: next = current & ~*set; break;
            case SSRNX_ANDROID_SIG_SETMASK: next = *set; break;
            default: return EINVAL;
        }
        next &= ~SIG_UNBLOCKABLE;
    }
    if (old_set) *old_set = current;
    if (set) {
        __atomic_store_n(&self->sig_blocked, next, __ATOMIC_RELEASE);
        /* POSIX: a pending signal this unblocks is delivered before return. */
        if (current & ~next) compat_threads_signal_deliver_self(self);
    }
    return 0;
}

int ssrnx_bionic_sigprocmask(int how, const uint64_t *set, uint64_t *old_set) {
    const int rc = ssrnx_bionic_pthread_sigmask(how, set, old_set);
    if (rc != 0) { errno = rc; return -1; }
    return 0;
}

int ssrnx_bionic_sigsuspend(const uint64_t *mask) {
    if (!mask) { errno = EFAULT; return -1; }
    compat_threads_CompatThread *self = compat_threads_adopt_current_thread();
    if (!self) { errno = ENOMEM; return -1; }
    const uint64_t saved_mask = __atomic_load_n(&self->sig_blocked, __ATOMIC_ACQUIRE);
    const uint32_t delivered = __atomic_load_n(&self->sig_delivered, __ATOMIC_ACQUIRE);
    __atomic_store_n(&self->sig_blocked, *mask & ~SIG_UNBLOCKABLE, __ATOMIC_RELEASE);
    __atomic_or_fetch(&self->sig_state, SIG_STATE_SUSPENDED, __ATOMIC_ACQ_REL);
    int signo = 0;
    for (;;) {
        const uint32_t wake = __atomic_load_n(&self->sig_wake, __ATOMIC_ACQUIRE);
        if (compat_threads_signal_claim(self, &signo)) break;
        /* A proxy delivered to this thread before it was marked suspended. */
        if (__atomic_load_n(&self->sig_delivered, __ATOMIC_ACQUIRE) != delivered) break;
        compat_threads_addr_wait(&self->sig_wake, wake, -1);
    }
    __atomic_and_fetch(&self->sig_state, ~SIG_STATE_SUSPENDED, __ATOMIC_ACQ_REL);
    if (signo) {
        compat_threads_signal_run_one(self, signo, 0, NULL);
        compat_threads_signal_deliver_self(self);
    }
    __atomic_store_n(&self->sig_blocked, saved_mask, __ATOMIC_RELEASE);
    compat_threads_signal_deliver_self(self);
    errno = EINTR;
    return -1;
}

int ssrnx_bionic_sigpending(uint64_t *set) {
    if (!set) { errno = EFAULT; return -1; }
    compat_threads_CompatThread *self = compat_threads_adopt_current_thread();
    if (!self) { errno = ENOMEM; return -1; }
    *set = __atomic_load_n(&self->sig_pending, __ATOMIC_ACQUIRE);
    return 0;
}

uint32_t ssrnx_guest_signal_mask_get32(void) {
    compat_threads_CompatThread *self = compat_threads_adopt_current_thread();
    return self ? (uint32_t)__atomic_load_n(&self->sig_blocked, __ATOMIC_ACQUIRE) : 0u;
}

void ssrnx_guest_signal_mask_set32(uint32_t mask) {
    compat_threads_CompatThread *self = compat_threads_adopt_current_thread();
    if (!self) return;
    const uint64_t current = __atomic_load_n(&self->sig_blocked, __ATOMIC_ACQUIRE);
    const uint64_t next = (current & ~UINT64_C(0xffffffff)) | mask;
    (void)ssrnx_bionic_pthread_sigmask(SSRNX_ANDROID_SIG_SETMASK, &next, NULL);
}

/*
 * Bionic LP64 sem_t: 16 opaque bytes. As in Bionic, the semaphore lives in
 * the guest object itself and a post is one atomic update plus an address
 * wake, so sem_post() is async-signal-safe as POSIX requires. Signal-based
 * stop-the-world collectors depend on that: their suspend handler posts an
 * acknowledgement on behalf of a thread stopped at an arbitrary point.
 */
typedef struct compat_threads_GuestSem {
    uint32_t count;
    uint32_t waiters;
    uint32_t magic;
    uint32_t reserved;
} compat_threads_GuestSem;
_Static_assert(sizeof(compat_threads_GuestSem) == 16, "Bionic LP64 sem_t size");
#define SSRNX_GUEST_SEM_MAGIC UINT32_C(0x414d5331) /* "AMS1" */

/*
 * Bionic performs no content validation on sem_t: the object is only its
 * count word, so a zero-filled (static/bss) or bytewise-copied semaphore is
 * usable and destroy does not poison it. Match that; rejecting a sem_t
 * whose private tag is missing makes callers such as Boehm GC's
 * stop-the-world barrier abort on a spurious EINVAL.
 */
static compat_threads_GuestSem *compat_threads_guest_sem_checked(void *guest_sem) {
    compat_threads_GuestSem *sem = (compat_threads_GuestSem *)guest_sem;
    if (!sem || ((uintptr_t)sem & 3u) != 0) return NULL;
    if (__atomic_load_n(&sem->magic, __ATOMIC_ACQUIRE) != SSRNX_GUEST_SEM_MAGIC) {
        static uint32_t warned;
        if (ssrnx_log_category && !__atomic_exchange_n(&warned, 1u, __ATOMIC_ACQ_REL))
            ssrnx_log_category("COMPAT", "WARN", "sem_t %p used without apn64 sem_init tag count=%u; accepting (Bionic semantics)",
                           guest_sem, (unsigned)__atomic_load_n(&sem->count, __ATOMIC_ACQUIRE));
    }
    return sem;
}

static bool compat_threads_guest_sem_try_take(compat_threads_GuestSem *sem) {
    uint32_t value = __atomic_load_n(&sem->count, __ATOMIC_ACQUIRE);
    while (value != 0) {
        if (__atomic_compare_exchange_n(&sem->count, &value, value - 1u, true,
                                        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
            return true;
    }
    return false;
}

static void compat_threads_guest_sem_block(compat_threads_GuestSem *sem, int64_t timeout_ns) {
    __atomic_add_fetch(&sem->waiters, 1u, __ATOMIC_SEQ_CST);
    if (__atomic_load_n(&sem->count, __ATOMIC_SEQ_CST) == 0)
        compat_threads_signal_safe_addr_wait(&sem->count, 0, timeout_ns);
    __atomic_sub_fetch(&sem->waiters, 1u, __ATOMIC_SEQ_CST);
}

int ssrnx_bionic_sem_init(void *guest_sem, int pshared, unsigned value) {
    (void)pshared; /* One process: shared and private semaphores coincide. */
    if (!guest_sem || ((uintptr_t)guest_sem & 3u) != 0 || value > SSRNX_BIONIC_SEM_VALUE_MAX) {
        errno = EINVAL;
        return -1;
    }
    compat_threads_GuestSem *sem = (compat_threads_GuestSem *)guest_sem;
    sem->count = value;
    sem->waiters = 0;
    sem->reserved = 0;
    __atomic_store_n(&sem->magic, SSRNX_GUEST_SEM_MAGIC, __ATOMIC_RELEASE);
    return 0;
}

int ssrnx_bionic_sem_destroy(void *guest_sem) {
    compat_threads_GuestSem *sem = compat_threads_guest_sem_checked(guest_sem);
    if (!sem) { errno = EINVAL; return -1; }
    __atomic_store_n(&sem->magic, 0, __ATOMIC_RELEASE);
    return 0;
}

int ssrnx_bionic_sem_wait(void *guest_sem) {
    compat_threads_GuestSem *sem = compat_threads_guest_sem_checked(guest_sem);
    if (!sem) { errno = EINVAL; return -1; }
    /* Android (target SDK >= 24) does not fail sem_wait with EINTR. */
    while (!compat_threads_guest_sem_try_take(sem)) {
        GUEST_WAIT_SITE("sem_wait", guest_sem);
        compat_threads_guest_sem_block(sem, -1);
    }
    return 0;
}

int ssrnx_bionic_sem_trywait(void *guest_sem) {
    compat_threads_GuestSem *sem = compat_threads_guest_sem_checked(guest_sem);
    if (!sem) { errno = EINVAL; return -1; }
    if (compat_threads_guest_sem_try_take(sem)) return 0;
    errno = EAGAIN;
    return -1;
}

int ssrnx_bionic_sem_timedwait(void *guest_sem, const void *guest_abstime) {
    compat_threads_GuestSem *sem = compat_threads_guest_sem_checked(guest_sem);
    if (!sem) { errno = EINVAL; return -1; }
    /* POSIX/Bionic require the immediate decrement attempt before validating
     * the absolute timeout. A ready semaphore therefore succeeds even with an
     * otherwise-invalid timespec. */
    if (compat_threads_guest_sem_try_take(sem)) return 0;
    if (!guest_abstime) { errno = EINVAL; return -1; }
    const compat_threads_SsrnxGuestTimespec64 *gt = (const compat_threads_SsrnxGuestTimespec64 *)guest_abstime;
    if (gt->tv_sec < 0 || gt->tv_nsec < 0 || gt->tv_nsec >= 1000000000LL) { errno = EINVAL; return -1; }
    for (;;) {
        struct timespec now;
        if (clock_gettime(CLOCK_REALTIME, &now) != 0) { errno = EINVAL; return -1; }
        const int64_t remaining = (gt->tv_sec - (int64_t)now.tv_sec) * INT64_C(1000000000) +
                                  (gt->tv_nsec - (int64_t)now.tv_nsec);
        if (remaining <= 0) { errno = ETIMEDOUT; return -1; }
        GUEST_WAIT_SITE("sem_timedwait", guest_sem);
        compat_threads_guest_sem_block(sem, remaining);
        if (compat_threads_guest_sem_try_take(sem)) return 0;
    }
}

int ssrnx_bionic_sem_post(void *guest_sem) {
    compat_threads_GuestSem *sem = compat_threads_guest_sem_checked(guest_sem);
    if (!sem) { errno = EINVAL; return -1; }
    uint32_t value = __atomic_load_n(&sem->count, __ATOMIC_ACQUIRE);
    do {
        if (value >= SSRNX_BIONIC_SEM_VALUE_MAX) { errno = EOVERFLOW; return -1; }
    } while (!__atomic_compare_exchange_n(&sem->count, &value, value + 1u, true,
                                          __ATOMIC_SEQ_CST, __ATOMIC_ACQUIRE));
    if (__atomic_load_n(&sem->waiters, __ATOMIC_SEQ_CST) != 0) compat_threads_addr_wake(&sem->count, 1);
    return 0;
}

int ssrnx_bionic_sem_getvalue(void *guest_sem, int *out_value) {
    compat_threads_GuestSem *sem = compat_threads_guest_sem_checked(guest_sem);
    if (!sem || !out_value) { errno = EINVAL; return -1; }
    *out_value = (int)__atomic_load_n(&sem->count, __ATOMIC_ACQUIRE);
    return 0;
}

int ssrnx_signal_safe_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                                     const struct timespec *deadline) {
    if (!cond || !mutex) return EINVAL;
    return compat_threads_signal_safe_cond_wait(cond, mutex, deadline);
}

#pragma pop_macro("SIG_UNBLOCKABLE")
#pragma pop_macro("SIG_STATE_TARGET_KICK")
#pragma pop_macro("SIG_STATE_SUSPENDED")
#pragma pop_macro("SIG_STATE_SAFE")
#pragma pop_macro("SIG_STATE_RETURNING")
#pragma pop_macro("SIG_STATE_KICK")
#pragma pop_macro("SIG_STATE_EXITING")
#pragma pop_macro("SIG_STATE_DELIVERING")
#pragma pop_macro("SIG_STATE_ASSIGNED")
#pragma pop_macro("SIG_BIT")
#pragma pop_macro("SIGNAL_WAIT_SLICE_NS")
#pragma pop_macro("SIGNAL_SAFE_GAP")
#pragma pop_macro("SIGNAL_RED_ZONE")
#pragma pop_macro("SIGNAL_PROXY_STACK")
#pragma pop_macro("PTHREAD_SETUP_FAIL")
#pragma pop_macro("GUEST_WAIT_SITE")
#pragma pop_macro("COMPAT_SYNC_BUCKETS")
#pragma pop_macro("SSRNX_LINUX_FPSIMD_MAGIC")
#pragma pop_macro("SSRNX_HOST_ERRNO_IMPORTED")
#pragma pop_macro("SSRNX_GUEST_SEM_MAGIC")
#pragma pop_macro("SSRNX_FUTEX_BUCKETS")
#pragma pop_macro("SSRNX_FUTEX_BITSET_MATCH_ANY")
#pragma pop_macro("SSRNX_ERRNO")
#pragma pop_macro("SSRNX_BIONIC_TLS_SLOT_THREAD_ID")
#pragma pop_macro("SSRNX_BIONIC_TLS_SLOT_STACK_GUARD")
#pragma pop_macro("SSRNX_BIONIC_TLS_SLOT_SELF")
#pragma pop_macro("SSRNX_BIONIC_TLS_SLOTS")
#pragma pop_macro("SSRNX_BIONIC_TLS_BASE_BYTES")
#pragma pop_macro("SSRNX_BIONIC_STATIC_TLS_CAPACITY")
#pragma pop_macro("SSRNX_BIONIC_SEM_VALUE_MAX")
#pragma pop_macro("SSRNX_BIONIC_RWLOCKATTR_RESERVED")
#pragma pop_macro("SSRNX_BIONIC_RWLOCKATTR_PSHARED_MASK")
#pragma pop_macro("SSRNX_BIONIC_RWLOCKATTR_KIND_MASK")
#pragma pop_macro("SSRNX_BIONIC_PTHREAD_KEYS_MAX")
#pragma pop_macro("SSRNX_BIONIC_PTHREAD_DESTRUCTOR_ITERATIONS")
#pragma pop_macro("SSRNX_BIONIC_KEY_VALID_FLAG")
#pragma pop_macro("SSRNX_BIONIC_KEY_SEQ_IN_USE")
#pragma pop_macro("SSRNX_BIONIC_COND_SHARED_MASK")
#pragma pop_macro("SSRNX_BIONIC_COND_CLOCK_MASK")
#pragma pop_macro("SSRNX_BIONIC_CONDATTR_DESTROYED")
#pragma pop_macro("SSRNX_ANDROID_SS_ONSTACK")
#pragma pop_macro("SSRNX_ANDROID_SS_DISABLE")
#pragma pop_macro("SSRNX_ANDROID_SI_TKILL")
#pragma pop_macro("SSRNX_ANDROID_SIG_UNBLOCK")
#pragma pop_macro("SSRNX_ANDROID_SIG_SETMASK")
#pragma pop_macro("SSRNX_ANDROID_SIG_BLOCK")
#pragma pop_macro("SSRNX_ANDROID_SIGSTOP")
#pragma pop_macro("SSRNX_ANDROID_SIGKILL")
#pragma pop_macro("SSRNX_ANDROID_SA_SIGINFO")
#pragma pop_macro("SSRNX_ANDROID_SA_RESETHAND")
#pragma pop_macro("SSRNX_ANDROID_SA_ONSTACK")
#pragma pop_macro("SSRNX_ANDROID_SA_NODEFER")
#pragma pop_macro("SSRNX_ANDROID_PRIO_USER")
#pragma pop_macro("SSRNX_ANDROID_PRIO_PROCESS")
#pragma pop_macro("SSRNX_ANDROID_PRIO_PGRP")
#pragma pop_macro("SSRNX_ANDROID_NICE_MIN")
#pragma pop_macro("SSRNX_ANDROID_NICE_MAX")
#pragma pop_macro("SSRNX_ANDROID_MINSIGSTKSZ")
#pragma pop_macro("SSRNX_ANDROID_APP_UID")


/* ---- compat_regex ---- */
#pragma push_macro("SSRNX_REGEX_MAGIC")
#pragma push_macro("RX_INF")


#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SSRNX_REGEX_MAGIC 0x41525831 /* ARX1 */
#define RX_INF UINT_MAX

typedef enum compat_regex_RxAtomType {
    RX_LITERAL,
    RX_DOT,
    RX_CLASS,
    RX_BOL,
    RX_EOL,
    RX_GROUP
} compat_regex_RxAtomType;

typedef struct compat_regex_RxExpr compat_regex_RxExpr;

typedef struct compat_regex_RxClass {
    uint8_t bits[32];
    bool negate;
} compat_regex_RxClass;

typedef struct compat_regex_RxAtom {
    compat_regex_RxAtomType type;
    unsigned char literal;
    compat_regex_RxClass cls;
    compat_regex_RxExpr *group;
} compat_regex_RxAtom;

typedef struct compat_regex_RxPiece {
    compat_regex_RxAtom atom;
    unsigned min;
    unsigned max;
} compat_regex_RxPiece;

typedef struct compat_regex_RxSeq {
    compat_regex_RxPiece *pieces;
    size_t count;
    size_t cap;
} compat_regex_RxSeq;

struct compat_regex_RxExpr {
    compat_regex_RxSeq *alts;
    size_t count;
    size_t cap;
};

typedef struct compat_regex_RxCompiled {
    int cflags;
    size_t nsub;
    compat_regex_RxExpr root;
} compat_regex_RxCompiled;

typedef struct compat_regex_RxParser {
    const unsigned char *p;
    const unsigned char *end;
    int cflags;
    bool extended;
    int error;
    size_t nsub;
} compat_regex_RxParser;

typedef struct compat_regex_RxMatchCtx {
    const unsigned char *s;
    size_t len;
    size_t range_start;
    size_t range_end;
    int cflags;
    int eflags;
} compat_regex_RxMatchCtx;

typedef struct compat_regex_RxSet {
    uint8_t *bits;
    size_t n;
} compat_regex_RxSet;

static void compat_regex_rx_expr_free(compat_regex_RxExpr *expr);
static int compat_regex_rx_collect_expr(const compat_regex_RxExpr *expr, size_t pos, const compat_regex_RxMatchCtx *ctx, compat_regex_RxSet *out);

static bool compat_regex_rx_set_init(compat_regex_RxSet *set, size_t n) {
    set->bits = (uint8_t *)calloc(n ? n : 1u, 1u);
    set->n = n;
    return set->bits != NULL;
}

static void compat_regex_rx_set_clear(compat_regex_RxSet *set) {
    if (set && set->bits) memset(set->bits, 0, set->n);
}

static void compat_regex_rx_set_free(compat_regex_RxSet *set) {
    if (!set) return;
    free(set->bits);
    set->bits = NULL;
    set->n = 0;
}

static void compat_regex_rx_set_add(compat_regex_RxSet *set, size_t pos) {
    if (set && pos < set->n) set->bits[pos] = 1;
}

static bool compat_regex_rx_set_any(const compat_regex_RxSet *set) {
    if (!set) return false;
    for (size_t i = 0; i < set->n; ++i) if (set->bits[i]) return true;
    return false;
}

static void compat_regex_rx_set_or(compat_regex_RxSet *dst, const compat_regex_RxSet *src) {
    if (!dst || !src || dst->n != src->n) return;
    for (size_t i = 0; i < dst->n; ++i) dst->bits[i] |= src->bits[i];
}

static bool compat_regex_rx_seq_push(compat_regex_RxSeq *seq, const compat_regex_RxPiece *piece) {
    if (seq->count == seq->cap) {
        size_t cap = seq->cap ? seq->cap * 2u : 8u;
        compat_regex_RxPiece *next = (compat_regex_RxPiece *)realloc(seq->pieces, cap * sizeof(*next));
        if (!next) return false;
        seq->pieces = next;
        seq->cap = cap;
    }
    seq->pieces[seq->count++] = *piece;
    return true;
}

static bool compat_regex_rx_expr_push_alt(compat_regex_RxExpr *expr, compat_regex_RxSeq *seq) {
    if (expr->count == expr->cap) {
        size_t cap = expr->cap ? expr->cap * 2u : 4u;
        compat_regex_RxSeq *next = (compat_regex_RxSeq *)realloc(expr->alts, cap * sizeof(*next));
        if (!next) return false;
        expr->alts = next;
        expr->cap = cap;
    }
    expr->alts[expr->count++] = *seq;
    memset(seq, 0, sizeof(*seq));
    return true;
}

static void compat_regex_rx_atom_free(compat_regex_RxAtom *atom) {
    if (!atom) return;
    if (atom->type == RX_GROUP && atom->group) {
        compat_regex_rx_expr_free(atom->group);
        free(atom->group);
        atom->group = NULL;
    }
}

static void compat_regex_rx_seq_free(compat_regex_RxSeq *seq) {
    if (!seq) return;
    for (size_t i = 0; i < seq->count; ++i) compat_regex_rx_atom_free(&seq->pieces[i].atom);
    free(seq->pieces);
    memset(seq, 0, sizeof(*seq));
}

static void compat_regex_rx_expr_free(compat_regex_RxExpr *expr) {
    if (!expr) return;
    for (size_t i = 0; i < expr->count; ++i) compat_regex_rx_seq_free(&expr->alts[i]);
    free(expr->alts);
    memset(expr, 0, sizeof(*expr));
}

static void compat_regex_rx_class_set(compat_regex_RxClass *cls, unsigned char c) {
    cls->bits[c >> 3] |= (uint8_t)(1u << (c & 7u));
}

static bool compat_regex_rx_class_get(const compat_regex_RxClass *cls, unsigned char c) {
    return (cls->bits[c >> 3] & (uint8_t)(1u << (c & 7u))) != 0;
}

static void compat_regex_rx_class_add_named(compat_regex_RxClass *cls, const char *name, size_t n) {
    for (unsigned c = 0; c < 256u; ++c) {
        int hit = 0;
        if (n == 5 && memcmp(name, "alnum", 5) == 0) hit = isalnum((int)(unsigned char)c);
        else if (n == 5 && memcmp(name, "alpha", 5) == 0) hit = isalpha((int)(unsigned char)c);
        else if (n == 5 && memcmp(name, "blank", 5) == 0) hit = (c == ' ' || c == '\t');
        else if (n == 5 && memcmp(name, "cntrl", 5) == 0) hit = iscntrl((int)(unsigned char)c);
        else if (n == 5 && memcmp(name, "digit", 5) == 0) hit = isdigit((int)(unsigned char)c);
        else if (n == 5 && memcmp(name, "graph", 5) == 0) hit = isgraph((int)(unsigned char)c);
        else if (n == 5 && memcmp(name, "lower", 5) == 0) hit = islower((int)(unsigned char)c);
        else if (n == 5 && memcmp(name, "print", 5) == 0) hit = isprint((int)(unsigned char)c);
        else if (n == 5 && memcmp(name, "punct", 5) == 0) hit = ispunct((int)(unsigned char)c);
        else if (n == 5 && memcmp(name, "space", 5) == 0) hit = isspace((int)(unsigned char)c);
        else if (n == 5 && memcmp(name, "upper", 5) == 0) hit = isupper((int)(unsigned char)c);
        else if (n == 6 && memcmp(name, "xdigit", 6) == 0) hit = isxdigit((int)(unsigned char)c);
        if (hit) compat_regex_rx_class_set(cls, (unsigned char)c);
    }
}

static bool compat_regex_rx_is_group_end(const compat_regex_RxParser *ps) {
    if (ps->extended) return ps->p < ps->end && *ps->p == ')';
    return ps->p + 1 < ps->end && ps->p[0] == '\\' && ps->p[1] == ')';
}

static bool compat_regex_rx_is_alt(const compat_regex_RxParser *ps) {
    if (ps->extended) return ps->p < ps->end && *ps->p == '|';
    return ps->p + 1 < ps->end && ps->p[0] == '\\' && ps->p[1] == '|';
}

static void compat_regex_rx_consume_alt(compat_regex_RxParser *ps) { ps->p += ps->extended ? 1 : 2; }
static void compat_regex_rx_consume_group_end(compat_regex_RxParser *ps) { ps->p += ps->extended ? 1 : 2; }

static bool compat_regex_rx_parse_uint(compat_regex_RxParser *ps, unsigned *value) {
    if (ps->p >= ps->end || !isdigit((int)*ps->p)) return false;
    unsigned v = 0;
    while (ps->p < ps->end && isdigit((int)*ps->p)) {
        unsigned d = (unsigned)(*ps->p - '0');
        if (v > (UINT_MAX - d) / 10u) { ps->error = SSRNX_REG_BADBR; return false; }
        v = v * 10u + d;
        ++ps->p;
    }
    *value = v;
    return true;
}

static bool compat_regex_rx_parse_braces(compat_regex_RxParser *ps, unsigned *min_out, unsigned *max_out) {
    const unsigned char *save = ps->p;
    bool escaped = !ps->extended;
    if (escaped) {
        if (ps->p + 1 >= ps->end || ps->p[0] != '\\' || ps->p[1] != '{') return false;
        ps->p += 2;
    } else {
        if (ps->p >= ps->end || *ps->p != '{') return false;
        ++ps->p;
    }
    unsigned min = 0, max = 0;
    if (!compat_regex_rx_parse_uint(ps, &min)) { ps->p = save; return false; }
    if (ps->p < ps->end && *ps->p == ',') {
        ++ps->p;
        if (ps->p < ps->end && isdigit((int)*ps->p)) {
            if (!compat_regex_rx_parse_uint(ps, &max)) { ps->error = SSRNX_REG_BADBR; return true; }
        } else max = RX_INF;
    } else max = min;
    if (escaped) {
        if (ps->p + 1 >= ps->end || ps->p[0] != '\\' || ps->p[1] != '}') { ps->error = SSRNX_REG_EBRACE; return true; }
        ps->p += 2;
    } else {
        if (ps->p >= ps->end || *ps->p != '}') { ps->error = SSRNX_REG_EBRACE; return true; }
        ++ps->p;
    }
    if (max != RX_INF && min > max) { ps->error = SSRNX_REG_BADBR; return true; }
    *min_out = min; *max_out = max;
    return true;
}

static int compat_regex_rx_parse_class(compat_regex_RxParser *ps, compat_regex_RxClass *cls) {
    memset(cls, 0, sizeof(*cls));
    if (ps->p >= ps->end || *ps->p != '[') return SSRNX_REG_EBRACK;
    ++ps->p;
    if (ps->p < ps->end && *ps->p == '^') { cls->negate = true; ++ps->p; }
    bool any = false;
    if (ps->p < ps->end && *ps->p == ']') { compat_regex_rx_class_set(cls, ']'); ++ps->p; any = true; }
    while (ps->p < ps->end && *ps->p != ']') {
        if (ps->p + 1 < ps->end && ps->p[0] == '[' && ps->p[1] == ':') {
            const unsigned char *name = ps->p + 2;
            const unsigned char *q = name;
            while (q + 1 < ps->end && !(q[0] == ':' && q[1] == ']')) ++q;
            if (q + 1 >= ps->end) return SSRNX_REG_ECTYPE;
            size_t n = (size_t)(q - name);
            uint8_t before[32]; memcpy(before, cls->bits, sizeof(before));
            compat_regex_rx_class_add_named(cls, (const char *)name, n);
            if (memcmp(before, cls->bits, sizeof(before)) == 0) return SSRNX_REG_ECTYPE;
            ps->p = q + 2; any = true; continue;
        }
        unsigned char first;
        if (*ps->p == '\\' && ps->p + 1 < ps->end) { ++ps->p; first = *ps->p++; }
        else first = *ps->p++;
        if (ps->p + 1 < ps->end && *ps->p == '-' && ps->p[1] != ']') {
            ++ps->p;
            unsigned char last;
            if (*ps->p == '\\' && ps->p + 1 < ps->end) { ++ps->p; last = *ps->p++; }
            else last = *ps->p++;
            if (first > last) return SSRNX_REG_ERANGE;
            for (unsigned c = first; c <= last; ++c) compat_regex_rx_class_set(cls, (unsigned char)c);
        } else compat_regex_rx_class_set(cls, first);
        any = true;
    }
    if (ps->p >= ps->end || *ps->p != ']') return SSRNX_REG_EBRACK;
    ++ps->p;
    return any ? 0 : SSRNX_REG_EBRACK;
}

static int compat_regex_rx_parse_expr(compat_regex_RxParser *ps, compat_regex_RxExpr *expr, bool in_group);

static int compat_regex_rx_parse_atom(compat_regex_RxParser *ps, compat_regex_RxAtom *atom) {
    memset(atom, 0, sizeof(*atom));
    if (ps->p >= ps->end) return SSRNX_REG_EMPTY;
    bool group_start = ps->extended ? (*ps->p == '(') : (ps->p + 1 < ps->end && ps->p[0] == '\\' && ps->p[1] == '(');
    if (group_start) {
        ps->p += ps->extended ? 1 : 2;
        atom->type = RX_GROUP;
        atom->group = (compat_regex_RxExpr *)calloc(1, sizeof(*atom->group));
        if (!atom->group) return SSRNX_REG_ESPACE;
        ++ps->nsub;
        int rc = compat_regex_rx_parse_expr(ps, atom->group, true);
        if (rc != 0) return rc;
        if (!compat_regex_rx_is_group_end(ps)) return SSRNX_REG_EPAREN;
        compat_regex_rx_consume_group_end(ps);
        return 0;
    }
    unsigned char c = *ps->p++;
    if (c == '[') {
        --ps->p; atom->type = RX_CLASS; return compat_regex_rx_parse_class(ps, &atom->cls);
    }
    if (c == '.') { atom->type = RX_DOT; return 0; }
    if (c == '^') { atom->type = RX_BOL; return 0; }
    if (c == '$') { atom->type = RX_EOL; return 0; }
    if (c == '\\') {
        if (ps->p >= ps->end) return SSRNX_REG_EESCAPE;
        c = *ps->p++;
        /* Useful common extensions accepted by Android-facing libraries. */
        if (c == 'd' || c == 'D' || c == 's' || c == 'S' || c == 'w' || c == 'W') {
            atom->type = RX_CLASS;
            const bool neg = (c == 'D' || c == 'S' || c == 'W');
            atom->cls.negate = neg;
            const char *name = (c == 'd' || c == 'D') ? "digit" : ((c == 's' || c == 'S') ? "space" : NULL);
            if (name) compat_regex_rx_class_add_named(&atom->cls, name, 5);
            else {
                compat_regex_rx_class_add_named(&atom->cls, "alnum", 5);
                compat_regex_rx_class_set(&atom->cls, '_');
            }
            return 0;
        }
        if (c == 'n') c = '\n'; else if (c == 'r') c = '\r'; else if (c == 't') c = '\t';
    }
    atom->type = RX_LITERAL;
    atom->literal = c;
    return 0;
}

static bool compat_regex_rx_parse_quantifier(compat_regex_RxParser *ps, unsigned *min, unsigned *max) {
    if (ps->p >= ps->end) return false;
    if (*ps->p == '*') { ++ps->p; *min = 0; *max = RX_INF; return true; }
    if (ps->extended) {
        if (*ps->p == '+') { ++ps->p; *min = 1; *max = RX_INF; return true; }
        if (*ps->p == '?') { ++ps->p; *min = 0; *max = 1; return true; }
    } else if (ps->p + 1 < ps->end && ps->p[0] == '\\') {
        if (ps->p[1] == '+') { ps->p += 2; *min = 1; *max = RX_INF; return true; }
        if (ps->p[1] == '?') { ps->p += 2; *min = 0; *max = 1; return true; }
    }
    return compat_regex_rx_parse_braces(ps, min, max);
}

static int compat_regex_rx_parse_expr(compat_regex_RxParser *ps, compat_regex_RxExpr *expr, bool in_group) {
    compat_regex_RxSeq seq = {0};
    while (ps->p < ps->end) {
        if (in_group && compat_regex_rx_is_group_end(ps)) break;
        if (compat_regex_rx_is_alt(ps)) {
            if (!compat_regex_rx_expr_push_alt(expr, &seq)) { compat_regex_rx_seq_free(&seq); return SSRNX_REG_ESPACE; }
            compat_regex_rx_consume_alt(ps);
            continue;
        }
        if (ps->extended && *ps->p == ')') return SSRNX_REG_EPAREN;
        compat_regex_RxPiece piece; memset(&piece, 0, sizeof(piece)); piece.min = piece.max = 1;
        int rc = compat_regex_rx_parse_atom(ps, &piece.atom);
        if (rc != 0) { compat_regex_rx_seq_free(&seq); return rc; }
        unsigned min = 1, max = 1;
        if (compat_regex_rx_parse_quantifier(ps, &min, &max)) {
            if (ps->error) { compat_regex_rx_atom_free(&piece.atom); compat_regex_rx_seq_free(&seq); return ps->error; }
            if (piece.atom.type == RX_BOL || piece.atom.type == RX_EOL) {
                compat_regex_rx_atom_free(&piece.atom); compat_regex_rx_seq_free(&seq); return SSRNX_REG_BADRPT;
            }
            piece.min = min; piece.max = max;
        }
        if (!compat_regex_rx_seq_push(&seq, &piece)) { compat_regex_rx_atom_free(&piece.atom); compat_regex_rx_seq_free(&seq); return SSRNX_REG_ESPACE; }
    }
    if (!compat_regex_rx_expr_push_alt(expr, &seq)) { compat_regex_rx_seq_free(&seq); return SSRNX_REG_ESPACE; }
    if (in_group && ps->p >= ps->end) return SSRNX_REG_EPAREN;
    return 0;
}

static unsigned char compat_regex_rx_fold(unsigned char c) { return (unsigned char)tolower((int)c); }

static int compat_regex_rx_collect_atom(const compat_regex_RxAtom *atom, size_t pos, const compat_regex_RxMatchCtx *ctx, compat_regex_RxSet *out) {
    if (!atom || !ctx || !out || pos > ctx->range_end) return 0;
    switch (atom->type) {
        case RX_LITERAL:
            if (pos < ctx->range_end) {
                unsigned char a = ctx->s[pos], b = atom->literal;
                if ((ctx->cflags & SSRNX_REG_ICASE) != 0) { a = compat_regex_rx_fold(a); b = compat_regex_rx_fold(b); }
                if (a == b) compat_regex_rx_set_add(out, pos + 1u);
            }
            break;
        case RX_DOT:
            if (pos < ctx->range_end && !((ctx->cflags & SSRNX_REG_NEWLINE) && ctx->s[pos] == '\n')) compat_regex_rx_set_add(out, pos + 1u);
            break;
        case RX_CLASS:
            if (pos < ctx->range_end) {
                unsigned char c = ctx->s[pos];
                bool hit = compat_regex_rx_class_get(&atom->cls, c);
                if ((ctx->cflags & SSRNX_REG_ICASE) != 0) {
                    unsigned char lo = compat_regex_rx_fold(c), up = (unsigned char)toupper((int)c);
                    hit = hit || compat_regex_rx_class_get(&atom->cls, lo) || compat_regex_rx_class_get(&atom->cls, up);
                }
                if (atom->cls.negate) hit = !hit;
                if ((ctx->cflags & SSRNX_REG_NEWLINE) && atom->cls.negate && c == '\n') hit = false;
                if (hit) compat_regex_rx_set_add(out, pos + 1u);
            }
            break;
        case RX_BOL: {
            bool bol = (pos == ctx->range_start && !(ctx->eflags & SSRNX_REG_NOTBOL));
            if (!bol && (ctx->cflags & SSRNX_REG_NEWLINE) && pos > ctx->range_start && ctx->s[pos - 1u] == '\n') bol = true;
            if (bol) compat_regex_rx_set_add(out, pos);
            break;
        }
        case RX_EOL: {
            bool eol = (pos == ctx->range_end && !(ctx->eflags & SSRNX_REG_NOTEOL));
            if (!eol && (ctx->cflags & SSRNX_REG_NEWLINE) && pos < ctx->range_end && ctx->s[pos] == '\n') eol = true;
            if (eol) compat_regex_rx_set_add(out, pos);
            break;
        }
        case RX_GROUP:
            if (atom->group) return compat_regex_rx_collect_expr(atom->group, pos, ctx, out);
            break;
    }
    return 0;
}

static int compat_regex_rx_collect_piece(const compat_regex_RxPiece *piece, size_t pos, const compat_regex_RxMatchCtx *ctx, compat_regex_RxSet *out) {
    const size_t n = ctx->range_end + 1u;
    compat_regex_RxSet cur = {0}, next = {0}, atom = {0};
    if (!compat_regex_rx_set_init(&cur, n) || !compat_regex_rx_set_init(&next, n) || !compat_regex_rx_set_init(&atom, n)) {
        compat_regex_rx_set_free(&cur); compat_regex_rx_set_free(&next); compat_regex_rx_set_free(&atom); return SSRNX_REG_ESPACE;
    }
    compat_regex_rx_set_add(&cur, pos);
    if (piece->min == 0) compat_regex_rx_set_add(out, pos);
    unsigned limit = piece->max;
    if (limit == RX_INF || limit > ctx->range_end - pos + 1u) limit = (unsigned)(ctx->range_end - pos + 1u);
    for (unsigned count = 1; count <= limit; ++count) {
        compat_regex_rx_set_clear(&next);
        for (size_t p = ctx->range_start; p <= ctx->range_end; ++p) if (cur.bits[p]) {
            compat_regex_rx_set_clear(&atom);
            int rc = compat_regex_rx_collect_atom(&piece->atom, p, ctx, &atom);
            if (rc != 0) { compat_regex_rx_set_free(&cur); compat_regex_rx_set_free(&next); compat_regex_rx_set_free(&atom); return rc; }
            for (size_t q = ctx->range_start; q <= ctx->range_end; ++q) if (atom.bits[q]) compat_regex_rx_set_add(&next, q);
        }
        if (!compat_regex_rx_set_any(&next)) break;
        if (count >= piece->min) compat_regex_rx_set_or(out, &next);
        uint8_t *tmp = cur.bits; cur.bits = next.bits; next.bits = tmp;
    }
    compat_regex_rx_set_free(&cur); compat_regex_rx_set_free(&next); compat_regex_rx_set_free(&atom);
    return 0;
}

static int compat_regex_rx_collect_seq(const compat_regex_RxSeq *seq, size_t pos, const compat_regex_RxMatchCtx *ctx, compat_regex_RxSet *out) {
    const size_t n = ctx->range_end + 1u;
    compat_regex_RxSet cur = {0}, next = {0}, piece_set = {0};
    if (!compat_regex_rx_set_init(&cur, n) || !compat_regex_rx_set_init(&next, n) || !compat_regex_rx_set_init(&piece_set, n)) {
        compat_regex_rx_set_free(&cur); compat_regex_rx_set_free(&next); compat_regex_rx_set_free(&piece_set); return SSRNX_REG_ESPACE;
    }
    compat_regex_rx_set_add(&cur, pos);
    for (size_t i = 0; i < seq->count; ++i) {
        compat_regex_rx_set_clear(&next);
        for (size_t p = ctx->range_start; p <= ctx->range_end; ++p) if (cur.bits[p]) {
            compat_regex_rx_set_clear(&piece_set);
            int rc = compat_regex_rx_collect_piece(&seq->pieces[i], p, ctx, &piece_set);
            if (rc != 0) { compat_regex_rx_set_free(&cur); compat_regex_rx_set_free(&next); compat_regex_rx_set_free(&piece_set); return rc; }
            compat_regex_rx_set_or(&next, &piece_set);
        }
        if (!compat_regex_rx_set_any(&next)) { compat_regex_rx_set_free(&cur); compat_regex_rx_set_free(&next); compat_regex_rx_set_free(&piece_set); return 0; }
        uint8_t *tmp = cur.bits; cur.bits = next.bits; next.bits = tmp;
    }
    compat_regex_rx_set_or(out, &cur);
    compat_regex_rx_set_free(&cur); compat_regex_rx_set_free(&next); compat_regex_rx_set_free(&piece_set);
    return 0;
}

static int compat_regex_rx_collect_expr(const compat_regex_RxExpr *expr, size_t pos, const compat_regex_RxMatchCtx *ctx, compat_regex_RxSet *out) {
    for (size_t i = 0; i < expr->count; ++i) {
        int rc = compat_regex_rx_collect_seq(&expr->alts[i], pos, ctx, out);
        if (rc != 0) return rc;
    }
    return 0;
}

int ssrnx_bionic_regcomp(SsrnxBionicRegex *preg, const char *pattern, int cflags) {
    if (!preg || !pattern) return SSRNX_REG_INVARG;
    if (preg->re_magic == SSRNX_REGEX_MAGIC && preg->re_g) ssrnx_bionic_regfree(preg);
    preg->re_magic = 0; preg->re_nsub = 0; preg->re_g = NULL;
    const unsigned char *begin = (const unsigned char *)pattern;
    const unsigned char *end = begin + strlen(pattern);
    if ((cflags & SSRNX_REG_PEND) != 0) {
        if (!preg->re_endp || preg->re_endp < pattern) return SSRNX_REG_INVARG;
        end = (const unsigned char *)preg->re_endp;
    }
    compat_regex_RxCompiled *rx = (compat_regex_RxCompiled *)calloc(1, sizeof(*rx));
    if (!rx) return SSRNX_REG_ESPACE;
    rx->cflags = cflags;
    compat_regex_RxParser ps = { begin, end, cflags, (cflags & SSRNX_REG_EXTENDED) != 0, 0, 0 };
    int rc = compat_regex_rx_parse_expr(&ps, &rx->root, false);
    if (rc == 0 && ps.p != ps.end) rc = SSRNX_REG_BADPAT;
    if (rc != 0) { compat_regex_rx_expr_free(&rx->root); free(rx); return rc; }
    rx->nsub = ps.nsub;
    preg->re_magic = SSRNX_REGEX_MAGIC;
    preg->re_nsub = ps.nsub;
    preg->re_g = rx;
    return 0;
}

int ssrnx_bionic_regexec(const SsrnxBionicRegex *preg, const char *string,
                         size_t nmatch, SsrnxBionicRegmatch pmatch[], int eflags) {
    if (!preg || !string || preg->re_magic != SSRNX_REGEX_MAGIC || !preg->re_g) return SSRNX_REG_INVARG;
    const compat_regex_RxCompiled *rx = (const compat_regex_RxCompiled *)preg->re_g;
    size_t len = strlen(string), start = 0, end = len;
    if ((eflags & SSRNX_REG_STARTEND) != 0) {
        if (!pmatch || nmatch == 0 || pmatch[0].rm_so < 0 || pmatch[0].rm_eo < pmatch[0].rm_so) return SSRNX_REG_INVARG;
        start = (size_t)pmatch[0].rm_so; end = (size_t)pmatch[0].rm_eo;
        if (start > len || end > len) return SSRNX_REG_INVARG;
    }
    compat_regex_RxMatchCtx ctx = { (const unsigned char *)string, len, start, end, rx->cflags, eflags };
    compat_regex_RxSet ends = {0};
    if (!compat_regex_rx_set_init(&ends, end + 1u)) return SSRNX_REG_ESPACE;
    bool found = false; size_t best_start = 0, best_end = 0;
    for (size_t s = start; s <= end; ++s) {
        compat_regex_rx_set_clear(&ends);
        int rc = compat_regex_rx_collect_expr(&rx->root, s, &ctx, &ends);
        if (rc != 0) { compat_regex_rx_set_free(&ends); return rc; }
        for (size_t e = end + 1u; e-- > s;) {
            if (e < ends.n && ends.bits[e]) { found = true; best_start = s; best_end = e; break; }
        }
        if (found) break; /* POSIX: leftmost, then longest. */
    }
    compat_regex_rx_set_free(&ends);
    if (!found) return SSRNX_REG_NOMATCH;
    if (!(rx->cflags & SSRNX_REG_NOSUB) && pmatch && nmatch) {
        pmatch[0].rm_so = (SsrnxBionicRegoff)best_start;
        pmatch[0].rm_eo = (SsrnxBionicRegoff)best_end;
        for (size_t i = 1; i < nmatch; ++i) pmatch[i].rm_so = pmatch[i].rm_eo = -1;
    }
    return 0;
}

void ssrnx_bionic_regfree(SsrnxBionicRegex *preg) {
    if (!preg) return;
    if (preg->re_magic == SSRNX_REGEX_MAGIC && preg->re_g) {
        compat_regex_RxCompiled *rx = (compat_regex_RxCompiled *)preg->re_g;
        compat_regex_rx_expr_free(&rx->root);
        free(rx);
    }
    preg->re_magic = 0;
    preg->re_nsub = 0;
    preg->re_g = NULL;
}

#pragma pop_macro("RX_INF")
#pragma pop_macro("SSRNX_REGEX_MAGIC")


/* ---- heap_cache ---- */
#pragma push_macro("HC_ARENA")
#pragma push_macro("HC_ARENA_SHIFT")
#pragma push_macro("HC_CLASSES")
#pragma push_macro("HC_MAX_SIZE")
#pragma push_macro("HC_REFILL_BYTES")
#pragma push_macro("HC_STAT_SLOTS")
#pragma push_macro("HC_TABLE_SLOTS")
#pragma push_macro("HC_THREAD_BYTES")


#ifdef __SWITCH__
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <reent.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Small-block allocator in front of newlib.
 *
 * newlib's dlmalloc runs every malloc/free under one process-wide lock. With
 * many runtime and guest threads allocating, the lock holder is regularly
 * preempted and every other allocating thread queues behind it. Blocks up to
 * HC_MAX_SIZE are therefore served from dedicated arenas instead:
 *
 *  - an arena is HC_ARENA bytes, HC_ARENA-aligned, obtained once from newlib
 *    and never returned; every block in it has the arena's size class, so a
 *    pointer's class is found from its arena base (lock-free table lookup);
 *  - each thread keeps per-class free lists (no lock);
 *  - refills and overflow go to a per-class depot with its own short lock.
 *
 * The link wraps newlib's reentrant entry points (-Wl,--wrap), so every
 * allocation in the process, guest Bionic calls included, goes through here.
 * newlib's realloc/calloc/memalign call _malloc_r/_free_r internally and do
 * chunk arithmetic on the results; while they run, t_bypass routes those
 * inner calls straight to dlmalloc so they only ever see real chunks. */

void *__real__malloc_r(struct _reent *reent, size_t size);
void __real__free_r(struct _reent *reent, void *ptr);
void *__real__realloc_r(struct _reent *reent, void *ptr, size_t size);
void *__real__calloc_r(struct _reent *reent, size_t count, size_t size);
void *__real__memalign_r(struct _reent *reent, size_t align, size_t size);
size_t __real__malloc_usable_size_r(struct _reent *reent, void *ptr);

#define HC_ARENA_SHIFT 18u
#define HC_ARENA ((size_t)1 << HC_ARENA_SHIFT)            /* 256 KiB */
#define HC_TABLE_SLOTS 32768u                             /* > 4 GiB / HC_ARENA */
#define HC_MAX_SIZE 16384u
#define HC_CLASSES 60u
#define HC_REFILL_BYTES 4096u
#define HC_THREAD_BYTES (256u * 1024u)

typedef struct heap_cache_HcDepot {
    pthread_mutex_t lock;
    void *free_list;
    size_t free_count;
    uint8_t *bump;
    uint8_t *bump_end;
} heap_cache_HcDepot;

typedef struct heap_cache_HcThread {
    void *head[HC_CLASSES];
    uint32_t count[HC_CLASSES];
    size_t bytes;
    bool retired;
} heap_cache_HcThread;

static volatile bool heap_cache_g_hc_on;
static uintptr_t heap_cache_g_hc_table[HC_TABLE_SLOTS]; /* arena base | (class + 1), 0 = empty */
static pthread_mutex_t heap_cache_g_hc_table_lock = PTHREAD_MUTEX_INITIALIZER;
static heap_cache_HcDepot heap_cache_g_hc_depot[HC_CLASSES];
static _Thread_local heap_cache_HcThread heap_cache_t_hc;
static _Thread_local unsigned heap_cache_t_bypass;

/* 16-byte steps to 256, 32 to 512, 64 to 1024, 128 to 2048, 512 to 8192,
 * 1024 to 16384. The large classes cover per-call runtime scratch
 * (interpreter frames, resolved method images, interface-default search
 * levels) that would otherwise queue on dlmalloc's lock. */
static inline unsigned heap_cache_hc_class_of(size_t size) {
    if (size <= 256u) return size ? (unsigned)((size - 1u) >> 4) : 0u;          /* 0..15  */
    if (size <= 512u) return 16u + (unsigned)((size - 257u) >> 5);             /* 16..23 */
    if (size <= 1024u) return 24u + (unsigned)((size - 513u) >> 6);            /* 24..31 */
    if (size <= 2048u) return 32u + (unsigned)((size - 1025u) >> 7);           /* 32..39 */
    if (size <= 8192u) return 40u + (unsigned)((size - 2049u) >> 9);           /* 40..51 */
    return 52u + (unsigned)((size - 8193u) >> 10);                            /* 52..59 */
}

static inline size_t heap_cache_hc_class_size(unsigned cls) {
    if (cls < 16u) return (size_t)(cls + 1u) << 4;
    if (cls < 24u) return 256u + ((size_t)(cls - 15u) << 5);
    if (cls < 32u) return 512u + ((size_t)(cls - 23u) << 6);
    if (cls < 40u) return 1024u + ((size_t)(cls - 31u) << 7);
    if (cls < 52u) return 2048u + ((size_t)(cls - 39u) << 9);
    return 8192u + ((size_t)(cls - 51u) << 10);
}

/* Which requests still reach dlmalloc (and its process-wide lock): a
 * lock-free table of request sizes per kind, drained by the profiler. */
enum { HC_STAT_MALLOC, HC_STAT_CALLOC, HC_STAT_MEMALIGN, HC_STAT_REALLOC, HC_STAT_FREE, HC_STAT_KINDS };
#define HC_STAT_SLOTS 128u
typedef struct heap_cache_HcStat { size_t key; uint32_t count; } heap_cache_HcStat;
static heap_cache_HcStat heap_cache_g_hc_stats[HC_STAT_SLOTS];

static void heap_cache_hc_stat(unsigned kind, size_t size) {
    /* Sizes above the cache are bucketed to 4 KiB so the table stays small. */
    const size_t bucket = size > HC_MAX_SIZE ? (size + 4095u) & ~(size_t)4095u : size;
    const size_t key = (bucket << 3) | kind;
    size_t i = (size_t)((key * UINT64_C(0x9E3779B97F4A7C15)) >> 57) & (HC_STAT_SLOTS - 1u);
    for (unsigned probe = 0; probe < 8u; ++probe, i = (i + 1u) & (HC_STAT_SLOTS - 1u)) {
        size_t cur = __atomic_load_n(&heap_cache_g_hc_stats[i].key, __ATOMIC_RELAXED);
        if (cur == 0) {
            size_t expected = 0;
            if (!__atomic_compare_exchange_n(&heap_cache_g_hc_stats[i].key, &expected, key, false,
                                             __ATOMIC_RELAXED, __ATOMIC_RELAXED))
                cur = expected;
            else
                cur = key;
        }
        if (cur == key) { __atomic_fetch_add(&heap_cache_g_hc_stats[i].count, 1u, __ATOMIC_RELAXED); return; }
    }
}

size_t ssrnx_heap_cache_take_stats(SsrnxHeapCacheStat *out, size_t cap) {
    static const char *const kinds[HC_STAT_KINDS] = {"malloc", "calloc", "memalign", "realloc", "free"};
    size_t n = 0;
    for (size_t i = 0; i < HC_STAT_SLOTS; ++i) {
        const size_t key = __atomic_load_n(&heap_cache_g_hc_stats[i].key, __ATOMIC_RELAXED);
        const uint32_t count = __atomic_exchange_n(&heap_cache_g_hc_stats[i].count, 0u, __ATOMIC_RELAXED);
        if (!key || !count) continue;
        SsrnxHeapCacheStat s = {kinds[key & 7u], key >> 3, count};
        if (n < cap) { out[n++] = s; continue; }
        size_t min_i = 0;
        for (size_t k = 1; k < n; ++k) if (out[k].count < out[min_i].count) min_i = k;
        if (s.count > out[min_i].count) out[min_i] = s;
    }
    return n;
}

static inline size_t heap_cache_hc_hash(uintptr_t base) {
    uint64_t h = (uint64_t)(base >> HC_ARENA_SHIFT) * UINT64_C(0x9E3779B97F4A7C15);
    return (size_t)(h >> 49) & (HC_TABLE_SLOTS - 1u);
}

/* Returns class + 1 when ptr lies in an arena, else 0. */
static inline unsigned heap_cache_hc_owner(const void *ptr) {
    const uintptr_t base = (uintptr_t)ptr & ~(uintptr_t)(HC_ARENA - 1u);
    for (size_t i = heap_cache_hc_hash(base), n = 0; n < HC_TABLE_SLOTS; ++n, i = (i + 1u) & (HC_TABLE_SLOTS - 1u)) {
        const uintptr_t e = __atomic_load_n(&heap_cache_g_hc_table[i], __ATOMIC_ACQUIRE);
        if (!e) return 0;
        if ((e & ~(uintptr_t)(HC_ARENA - 1u)) == base) return (unsigned)(e & (HC_ARENA - 1u));
    }
    return 0;
}

static bool heap_cache_hc_register_arena(uintptr_t base, unsigned cls) {
    bool ok = false;
    pthread_mutex_lock(&heap_cache_g_hc_table_lock);
    for (size_t i = heap_cache_hc_hash(base), n = 0; n < HC_TABLE_SLOTS; ++n, i = (i + 1u) & (HC_TABLE_SLOTS - 1u)) {
        if (heap_cache_g_hc_table[i]) continue;
        __atomic_store_n(&heap_cache_g_hc_table[i], base | (uintptr_t)(cls + 1u), __ATOMIC_RELEASE);
        ok = true;
        break;
    }
    pthread_mutex_unlock(&heap_cache_g_hc_table_lock);
    return ok;
}

bool ssrnx_heap_cache_read_small_block(const void *block, size_t offset, void *out, size_t bytes) {
    if (!block || !out || !bytes) return false;
    const unsigned owner = heap_cache_hc_owner((void *)block);
    if (!owner || owner > HC_CLASSES) return false;
    const size_t size = heap_cache_hc_class_size(owner - 1u);
    const size_t in_arena = (uintptr_t)block & (HC_ARENA - 1u);
    if (size % 4096u == 0 || in_arena % size != 0 ||
        in_arena > HC_ARENA - size || offset > size || bytes > size - offset) return false;
    /* These arenas are never released and never serve page-aligned aliases. */
    memcpy(out, (const uint8_t *)block + offset, bytes);
    return true;
}

void ssrnx_heap_cache_enable(void) {
    for (unsigned i = 0; i < HC_CLASSES; ++i) pthread_mutex_init(&heap_cache_g_hc_depot[i].lock, NULL);
    __atomic_store_n(&heap_cache_g_hc_on, true, __ATOMIC_RELEASE);
}

/* Pops up to `want` blocks of `cls` from the depot into a linked chain. */
static unsigned heap_cache_hc_depot_take(struct _reent *reent, unsigned cls, unsigned want, void **out_chain) {
    heap_cache_HcDepot *d = &heap_cache_g_hc_depot[cls];
    const size_t size = heap_cache_hc_class_size(cls);
    void *chain = NULL;
    unsigned got = 0;
    pthread_mutex_lock(&d->lock);
    while (got < want && d->free_list) {
        void *p = d->free_list;
        d->free_list = *(void **)p;
        d->free_count--;
        *(void **)p = chain;
        chain = p;
        got++;
    }
    while (got < want) {
        if ((size_t)(d->bump_end - d->bump) < size) {
            heap_cache_t_bypass++;
            uint8_t *arena = (uint8_t *)__real__memalign_r(reent, HC_ARENA, HC_ARENA);
            heap_cache_t_bypass--;
            if (!arena) break;
            if (!heap_cache_hc_register_arena((uintptr_t)arena, cls)) {
                heap_cache_t_bypass++;
                __real__free_r(reent, arena);
                heap_cache_t_bypass--;
                break;
            }
            d->bump = arena;
            d->bump_end = arena + HC_ARENA - (HC_ARENA % size);
        }
        void *p = d->bump;
        d->bump += size;
        *(void **)p = chain;
        chain = p;
        got++;
    }
    pthread_mutex_unlock(&d->lock);
    *out_chain = chain;
    return got;
}

static void heap_cache_hc_depot_put(unsigned cls, void *first, void *last, size_t count) {
    heap_cache_HcDepot *d = &heap_cache_g_hc_depot[cls];
    pthread_mutex_lock(&d->lock);
    *(void **)last = d->free_list;
    d->free_list = first;
    d->free_count += count;
    pthread_mutex_unlock(&d->lock);
}

/* Moves all but `keep` cached blocks of `cls` back to the depot. */
static void heap_cache_hc_flush_class(heap_cache_HcThread *t, unsigned cls, uint32_t keep) {
    if (t->count[cls] <= keep) return;
    const size_t size = heap_cache_hc_class_size(cls);
    void *first = t->head[cls], *last = first;
    size_t moved = 1;
    while (t->count[cls] - moved > keep) { last = *(void **)last; moved++; }
    t->head[cls] = *(void **)last;
    t->count[cls] -= (uint32_t)moved;
    t->bytes -= moved * size;
    heap_cache_hc_depot_put(cls, first, last, moved);
}

void ssrnx_heap_cache_thread_exit(void) {
    if (!__atomic_load_n(&heap_cache_g_hc_on, __ATOMIC_ACQUIRE)) return;
    heap_cache_HcThread *t = &heap_cache_t_hc;
    t->retired = true;
    for (unsigned cls = 0; cls < HC_CLASSES; ++cls) heap_cache_hc_flush_class(t, cls, 0);
}

static void *heap_cache_hc_alloc(struct _reent *reent, unsigned cls) {
    heap_cache_HcThread *t = &heap_cache_t_hc;
    void *p = t->head[cls];
    if (p) {
        t->head[cls] = *(void **)p;
        t->count[cls]--;
        t->bytes -= heap_cache_hc_class_size(cls);
        return p;
    }
    const size_t size = heap_cache_hc_class_size(cls);
    unsigned want = t->retired ? 1u : (unsigned)(HC_REFILL_BYTES / size);
    if (want < 2u && !t->retired) want = 2u;
    void *chain = NULL;
    const unsigned got = heap_cache_hc_depot_take(reent, cls, want, &chain);
    if (!got) return NULL;
    p = chain;
    chain = *(void **)chain;
    if (got > 1u) {
        void *last = chain;
        while (*(void **)last) last = *(void **)last;
        *(void **)last = t->head[cls];
        t->head[cls] = chain;
        t->count[cls] += got - 1u;
        t->bytes += (size_t)(got - 1u) * size;
    }
    return p;
}

static void heap_cache_hc_free(unsigned cls, void *ptr) {
    heap_cache_HcThread *t = &heap_cache_t_hc;
    const size_t size = heap_cache_hc_class_size(cls);
    if (t->retired) { heap_cache_hc_depot_put(cls, ptr, ptr, 1); return; }
    *(void **)ptr = t->head[cls];
    t->head[cls] = ptr;
    t->count[cls]++;
    t->bytes += size;
    const uint32_t cap = (uint32_t)((HC_REFILL_BYTES * 4u) / size) + 4u;
    if (t->count[cls] > cap) heap_cache_hc_flush_class(t, cls, cap / 2u);
    else if (t->bytes > HC_THREAD_BYTES) heap_cache_hc_flush_class(t, cls, 0);
}

static inline bool heap_cache_hc_active(void) {
    return __atomic_load_n(&heap_cache_g_hc_on, __ATOMIC_ACQUIRE) && heap_cache_t_bypass == 0;
}

/* A dlmalloc request made directly by a caller (not newlib's own inner call
 * while t_bypass routes it there) is counted for the profiler. */
static inline void heap_cache_hc_stat_top(unsigned kind, size_t size) {
    if (__atomic_load_n(&heap_cache_g_hc_on, __ATOMIC_RELAXED) && heap_cache_t_bypass == 0) heap_cache_hc_stat(kind, size);
}

void *__wrap__malloc_r(struct _reent *reent, size_t size) {
    if (heap_cache_hc_active() && size <= HC_MAX_SIZE) {
        void *p = heap_cache_hc_alloc(reent, heap_cache_hc_class_of(size));
        if (p) return p;
    }
    heap_cache_hc_stat_top(HC_STAT_MALLOC, size);
    return __real__malloc_r(reent, size);
}

void __wrap__free_r(struct _reent *reent, void *ptr) {
    if (!ptr) return;
    const unsigned owner = __atomic_load_n(&heap_cache_g_hc_on, __ATOMIC_ACQUIRE) ? heap_cache_hc_owner(ptr) : 0u;
    if (owner) { heap_cache_hc_free(owner - 1u, ptr); return; }
    if (__atomic_load_n(&heap_cache_g_hc_on, __ATOMIC_RELAXED) && heap_cache_t_bypass == 0)
        heap_cache_hc_stat(HC_STAT_FREE, __real__malloc_usable_size_r(reent, ptr));
    __real__free_r(reent, ptr);
}

size_t __wrap__malloc_usable_size_r(struct _reent *reent, void *ptr) {
    if (!ptr) return 0;
    const unsigned owner = __atomic_load_n(&heap_cache_g_hc_on, __ATOMIC_ACQUIRE) ? heap_cache_hc_owner(ptr) : 0u;
    if (owner) return heap_cache_hc_class_size(owner - 1u);
    return __real__malloc_usable_size_r(reent, ptr);
}

void *__wrap__calloc_r(struct _reent *reent, size_t count, size_t size) {
    if (size && count > SIZE_MAX / size) { reent->_errno = ENOMEM; return NULL; }
    const size_t total = count * size;
    if (heap_cache_hc_active() && total <= HC_MAX_SIZE) {
        void *p = heap_cache_hc_alloc(reent, heap_cache_hc_class_of(total));
        if (p) { memset(p, 0, total); return p; }
    }
    heap_cache_hc_stat_top(HC_STAT_CALLOC, total);
    heap_cache_t_bypass++;
    void *p = __real__calloc_r(reent, count, size);
    heap_cache_t_bypass--;
    return p;
}

void *__wrap__memalign_r(struct _reent *reent, size_t align, size_t size) {
    /* Arenas are HC_ARENA-aligned and blocks sit at whole multiples of the
     * class size from the arena base, so a class whose size is a multiple of
     * the alignment yields aligned blocks (every class is a multiple of 16). */
    if (align <= 4096u && (align & (align - 1u)) == 0 && heap_cache_hc_active() && size <= HC_MAX_SIZE) {
        unsigned cls = heap_cache_hc_class_of(size);
        while (cls < HC_CLASSES && (heap_cache_hc_class_size(cls) & (align - 1u)) != 0) ++cls;
        if (cls < HC_CLASSES) {
            void *p = heap_cache_hc_alloc(reent, cls);
            if (p) return p;
        }
    }
    heap_cache_hc_stat_top(HC_STAT_MEMALIGN, size);
    heap_cache_t_bypass++;
    void *p = __real__memalign_r(reent, align, size);
    heap_cache_t_bypass--;
    return p;
}

void *__wrap__realloc_r(struct _reent *reent, void *ptr, size_t size) {
    if (!ptr) return __wrap__malloc_r(reent, size);
    const unsigned owner = __atomic_load_n(&heap_cache_g_hc_on, __ATOMIC_ACQUIRE) ? heap_cache_hc_owner(ptr) : 0u;
    size_t old_size = 0;
    if (owner) {
        old_size = heap_cache_hc_class_size(owner - 1u);
        if (size <= old_size) return ptr;
    } else {
        old_size = __real__malloc_usable_size_r(reent, ptr);
        /* Growth into a small block, or anything dlmalloc can resize itself. */
        if (!(heap_cache_hc_active() && size <= HC_MAX_SIZE && size > old_size)) {
            heap_cache_hc_stat_top(HC_STAT_REALLOC, size);
            heap_cache_t_bypass++;
            void *p = __real__realloc_r(reent, ptr, size);
            heap_cache_t_bypass--;
            return p;
        }
    }
    void *p = __wrap__malloc_r(reent, size);
    if (!p) return NULL;
    memcpy(p, ptr, old_size < size ? old_size : size);
    __wrap__free_r(reent, ptr);
    return p;
}
#endif

#pragma pop_macro("HC_THREAD_BYTES")
#pragma pop_macro("HC_TABLE_SLOTS")
#pragma pop_macro("HC_STAT_SLOTS")
#pragma pop_macro("HC_REFILL_BYTES")
#pragma pop_macro("HC_MAX_SIZE")
#pragma pop_macro("HC_CLASSES")
#pragma pop_macro("HC_ARENA_SHIFT")
#pragma pop_macro("HC_ARENA")
