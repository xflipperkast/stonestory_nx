/* runtime java. */
#define _GNU_SOURCE 1
#include "runtime.h"


/* ---- dex_vm ---- */
#pragma push_macro("A64_FP")
#pragma push_macro("A64_GP")
#pragma push_macro("DEX_ACC_ABSTRACT")
#pragma push_macro("DEX_ACC_INTERFACE")
#pragma push_macro("DEX_ACC_NATIVE")
#pragma push_macro("DEX_ACC_PRIVATE")
#pragma push_macro("DEX_ACC_STATIC")
#pragma push_macro("DEX_ACC_SYNCHRONIZED")
#pragma push_macro("DEX_FIELD_TYPE_CAP")
#pragma push_macro("DEX_JNI_PROXY_CHUNK")
#pragma push_macro("DEX_LEDGER_CAP")
#pragma push_macro("DEX_MAX_DEPTH")
#pragma push_macro("DEX_MAX_REGS")
#pragma push_macro("DEX_NATIVE_RESOLVE_CACHE_MAX")
#pragma push_macro("DEX_NATIVE_STACK_RESERVE")
#pragma push_macro("DEX_PREFLIGHT_IFACE_MAX")
#pragma push_macro("DEX_PREFLIGHT_MAX_METHODS")
#pragma push_macro("DEX_PREFLIGHT_MAX_UNRESOLVED")
#pragma push_macro("DEX_PROF_METHOD_BUCKETS")
#pragma push_macro("DEX_PROF_REPORT_MS")
#pragma push_macro("DEX_PROF_SAMPLE_US")
#pragma push_macro("DEX_PROF_SLOTS")
#pragma push_macro("DEX_SOFT_STEP_DIAG")
#pragma push_macro("DEX_STACK_SNAPSHOT_MAX")
#pragma push_macro("DEX_STATIC_OWNER_KEY_CAP")
#pragma push_macro("DEX_STATIC_OWNER_SLOTS")
#pragma push_macro("DEX_WATCHDOG_POLL_MASK")
#pragma push_macro("NATIVE_A64_STACK_CAP")
#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE /* pthread_getattr_np for the native stack guard */
#endif


#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __SWITCH__
#include <switch.h>
#endif













/* Engine adapters are optional in standalone interpreter fixtures. */


#define DEX_ACC_STATIC 0x0008u
#define DEX_ACC_SYNCHRONIZED 0x0020u
#define DEX_ACC_PRIVATE 0x0002u
#define DEX_ACC_NATIVE 0x0100u
#define DEX_ACC_INTERFACE 0x0200u
#define DEX_ACC_ABSTRACT 0x0400u
#define DEX_MAX_REGS 512u
/* A field type descriptor names one type: at most a class name plus 'L'/';'
 * and array dimensions. */
#define DEX_FIELD_TYPE_CAP 640u
/* Java call depth backstop. The native stack guard (dex_native_stack_exhausted)
 * is the real limit and raises StackOverflowError as ART does when its stack
 * runs out; this bound only keeps runaway recursion finite. */
#define DEX_MAX_DEPTH 1024u
#define DEX_SOFT_STEP_DIAG 200000u
#define DEX_WATCHDOG_POLL_MASK 0x0fffu
#define DEX_STACK_SNAPSHOT_MAX 68u /* outermost frames of a call chain kept for diagnostics */

/* Shadow stack of the methods executing on this thread. Each entry is the
 * resolved method image, which stays alive until its frame returns, so a push
 * is one pointer store; names are only copied when a snapshot is taken. */
static _Thread_local const SsrnxDexMethodImage *dex_vm_g_dex_stack[DEX_STACK_SNAPSHOT_MAX];
static _Thread_local size_t dex_vm_g_dex_stack_depth;

size_t ssrnx_dex_current_stack(SsrnxDexStackFrame *out_frames, size_t capacity) {
    const size_t available = dex_vm_g_dex_stack_depth < DEX_STACK_SNAPSHOT_MAX
                                 ? dex_vm_g_dex_stack_depth : DEX_STACK_SNAPSHOT_MAX;
    const size_t count = out_frames && capacity < available ? capacity : available;
    if (out_frames) {
        for (size_t i = 0; i < count; ++i) {
            const SsrnxDexMethodImage *m = dex_vm_g_dex_stack[available - 1u - i];
            SsrnxDexStackFrame *f = &out_frames[i];
            f->class_name[0] = f->method_name[0] = '\0';
            if (!m) continue;
            snprintf(f->class_name, sizeof(f->class_name), "%s", m->declaring_class);
            snprintf(f->method_name, sizeof(f->method_name), "%s", m->method_name);
        }
    }
    return available;
}

/* ---------- sampling profiler ----------
 * Every thread that runs Java publishes its innermost method image in a
 * process-global slot (one store per call/return). A sampler thread reads the
 * slots and the thread registry's wait state every few milliseconds and logs,
 * every DEX_PROF_REPORT_MS, how busy each thread was and which Java methods it
 * was executing. DEEP publication and name snapshots share g_prof_lock so
 * a returned frame cannot release an image while the sampler reads it.
 * LIGHT only observes the atomic boundary marker, never frame metadata. */
#define DEX_PROF_SLOTS 160u
#define DEX_PROF_SAMPLE_US 10000u
#define DEX_PROF_REPORT_MS 10000u
#define DEX_PROF_METHOD_BUCKETS 1024u

typedef struct dex_vm_DexProfSlot {
    pthread_t host;
    _Atomic(const SsrnxDexMethodImage *) innermost;
    /* Framework (host-modeled) call in progress from the innermost frame. */
    _Atomic(const char *) host_class;
    _Atomic(const char *) host_method;
    _Atomic bool used;
} dex_vm_DexProfSlot;

static dex_vm_DexProfSlot dex_vm_g_prof_slots[DEX_PROF_SLOTS];
static pthread_mutex_t dex_vm_g_prof_lock = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local dex_vm_DexProfSlot *dex_vm_g_prof_self;
static _Thread_local bool dex_vm_g_prof_self_full;
static pthread_once_t dex_vm_g_prof_once = PTHREAD_ONCE_INIT;
static pthread_t dex_vm_g_prof_thread;
static atomic_bool dex_vm_g_prof_thread_running;
static atomic_bool dex_vm_g_prof_stop;
static pthread_key_t dex_vm_g_prof_key;
static bool dex_vm_g_prof_key_ready;

static void dex_vm_dex_prof_start(void);
static dex_vm_DexProfSlot *dex_vm_dex_prof_slot(void);

typedef struct dex_vm_DexProfHostMark { dex_vm_DexProfSlot *slot; const char *cls, *name; } dex_vm_DexProfHostMark;

static const SsrnxDexMethodImage *dex_vm_dex_prof_method_exchange(dex_vm_DexProfSlot *slot, const SsrnxDexMethodImage *m) {
    if (!slot) return NULL;
    const bool deep = ssrnx_perf_mode() == SSRNX_PERF_DEEP;
    if (deep) pthread_mutex_lock(&dex_vm_g_prof_lock);
    const SsrnxDexMethodImage *saved = atomic_exchange_explicit(&slot->innermost, m, memory_order_acq_rel);
    if (deep) pthread_mutex_unlock(&dex_vm_g_prof_lock);
    return saved;
}

static inline dex_vm_DexProfHostMark dex_vm_dex_prof_host_enter(const char *cls, const char *name) {
    dex_vm_DexProfHostMark mark = {dex_vm_dex_prof_slot(), NULL, NULL};
    if (mark.slot) {
        const bool deep = ssrnx_perf_mode() == SSRNX_PERF_DEEP;
        if (deep) pthread_mutex_lock(&dex_vm_g_prof_lock);
        mark.cls = atomic_exchange_explicit(&mark.slot->host_class, cls, memory_order_acq_rel);
        mark.name = atomic_exchange_explicit(&mark.slot->host_method, name, memory_order_acq_rel);
        if (deep) pthread_mutex_unlock(&dex_vm_g_prof_lock);
    }
    return mark;
}

static inline void dex_vm_dex_prof_host_leave(dex_vm_DexProfHostMark mark) {
    if (!mark.slot) return;
    const bool deep = ssrnx_perf_mode() == SSRNX_PERF_DEEP;
    if (deep) pthread_mutex_lock(&dex_vm_g_prof_lock);
    atomic_store_explicit(&mark.slot->host_class, mark.cls, memory_order_release);
    atomic_store_explicit(&mark.slot->host_method, mark.name, memory_order_release);
    if (deep) pthread_mutex_unlock(&dex_vm_g_prof_lock);
}

static void dex_vm_dex_prof_thread_exit(void *value) {
    dex_vm_DexProfSlot *slot = value;
    pthread_mutex_lock(&dex_vm_g_prof_lock);
    atomic_store(&slot->innermost, NULL);
    atomic_store(&slot->host_class, NULL);
    atomic_store(&slot->host_method, NULL);
    atomic_store(&slot->used, false);
    pthread_mutex_unlock(&dex_vm_g_prof_lock);
}

static dex_vm_DexProfSlot *dex_vm_dex_prof_slot(void) {
    if (ssrnx_perf_mode() == SSRNX_PERF_OFF) return NULL;
    if (dex_vm_g_prof_self) return dex_vm_g_prof_self;
    if (dex_vm_g_prof_self_full) return NULL;
    pthread_once(&dex_vm_g_prof_once, dex_vm_dex_prof_start);
    if (!dex_vm_g_prof_key_ready) { dex_vm_g_prof_self_full = true; return NULL; }
    const pthread_t self = pthread_self();
    SSRNX_PERF_LOCK(&dex_vm_g_prof_lock);
    dex_vm_DexProfSlot *free_slot = NULL;
    for (size_t i = 0; i < DEX_PROF_SLOTS; ++i) {
        dex_vm_DexProfSlot *slot = &dex_vm_g_prof_slots[i];
        if (atomic_load(&slot->used) && pthread_equal(slot->host, self)) { free_slot = slot; break; }
        if (!free_slot && !atomic_load(&slot->used)) free_slot = slot;
    }
    if (free_slot && !atomic_load(&free_slot->used)) {
        free_slot->host = self;
        atomic_store(&free_slot->innermost, NULL);
        atomic_store(&free_slot->host_class, NULL);
        atomic_store(&free_slot->host_method, NULL);
        atomic_store(&free_slot->used, true);
    }
    pthread_mutex_unlock(&dex_vm_g_prof_lock);
    dex_vm_g_prof_self = free_slot;
    dex_vm_g_prof_self_full = !free_slot;
    if (free_slot && pthread_setspecific(dex_vm_g_prof_key, free_slot) != 0) {
        dex_vm_dex_prof_thread_exit(free_slot);
        dex_vm_g_prof_self = NULL;
        dex_vm_g_prof_self_full = true;
        SSRNX_LOGCW("PROF", "DEX sampler TLS registration failed; this thread is unobserved");
        return NULL;
    }
    if (!free_slot) SSRNX_LOGCW("PROF", "DEX sampler slots exhausted; this thread is unobserved");
    return free_slot;
}

typedef struct dex_vm_DexProfThreadStat {
    pthread_t host;
    uint64_t guest_id;
    char name[16];
    uint32_t samples, busy, java;
    uint64_t first_ticks, last_ticks; /* kernel CPU time over the window */
    bool cpu_valid;
} dex_vm_DexProfThreadStat;

typedef struct dex_vm_DexProfMethodStat {
    char key[274]; /* thread | class.method [> framework class.method] */
    uint32_t busy;
    bool used;
} dex_vm_DexProfMethodStat;

static void dex_vm_dex_prof_copy(char *out, size_t cap, const char *in) {
    size_t n = 0;
    while (n + 1u < cap && in[n] && in[n] >= 0x20 && in[n] < 0x7f) { out[n] = in[n]; ++n; }
    out[n] = '\0';
}

static int dex_vm_dex_prof_thread_cmp(const void *a, const void *b) {
    const dex_vm_DexProfThreadStat *x = a, *y = b;
    const uint64_t cx = x->last_ticks - x->first_ticks, cy = y->last_ticks - y->first_ticks;
    if (cx != cy) return cx < cy ? 1 : -1;
    return x->busy < y->busy ? 1 : x->busy > y->busy ? -1 : 0;
}

static int dex_vm_dex_prof_method_cmp(const void *a, const void *b) {
    const dex_vm_DexProfMethodStat *x = a, *y = b;
    return x->busy < y->busy ? 1 : x->busy > y->busy ? -1 : 0;
}

static int dex_vm_dex_prof_alloc_cmp(const void *a, const void *b) {
    const SsrnxHeapCacheStat *x = a, *y = b;
    return x->count < y->count ? 1 : x->count > y->count ? -1 : 0;
}

typedef struct dex_vm_DexProfNativeState {
    struct { pthread_t host; uint64_t guest_id, ticks; bool valid; } previous[128];
    size_t previous_count, cursor;
    uint64_t last_ms, attempts, successful, capture_us, capture_max_us;
    unsigned turn;
} dex_vm_DexProfNativeState;

/* Half the budget follows recent CPU use; half preserves broad coverage.
 * Jitter avoids repeatedly observing one fixed point in a periodic frame. */
static void dex_vm_dex_prof_native_progress(const SsrnxThreadSample *samples, size_t count,
                                     uint64_t elapsed_ms, dex_vm_DexProfNativeState *state) {
#ifdef __SWITCH__
    if (elapsed_ms < state->last_ms ||
        elapsed_ms - state->last_ms < 125u + state->turn * 17u % 23u || !count) return;
    const uint64_t span_ms = elapsed_ms - state->last_ms;
    state->last_ms = elapsed_ms;
    size_t hot[2] = { SIZE_MAX, SIZE_MAX };
    uint64_t deltas[128] = {0};
    const size_t available = count < 128u ? count : 128u;
    for (size_t i = 0; i < available; ++i) {
        const SsrnxThreadSample *s = &samples[i];
        if (!s->cpu_valid || pthread_equal(s->host, pthread_self())) continue;
        for (size_t j = 0; j < state->previous_count; ++j) {
            if (state->previous[j].valid && state->previous[j].guest_id == s->guest_id &&
                pthread_equal(state->previous[j].host, s->host) && s->cpu_ticks >= state->previous[j].ticks) {
                deltas[i] = s->cpu_ticks - state->previous[j].ticks; break;
            }
        }
        if (!deltas[i]) continue;
        if (hot[0] == SIZE_MAX || deltas[i] > deltas[hot[0]]) { hot[1] = hot[0]; hot[0] = i; }
        else if (hot[1] == SIZE_MAX || deltas[i] > deltas[hot[1]]) hot[1] = i;
    }
    for (size_t i = 0; i < available; ++i) {
        state->previous[i].host = samples[i].host;
        state->previous[i].guest_id = samples[i].guest_id;
        state->previous[i].ticks = samples[i].cpu_ticks;
        state->previous[i].valid = samples[i].cpu_valid;
    }
    state->previous_count = available;
    size_t selected = SIZE_MAX;
    bool cpu_selected = false;
    if ((state->turn & 1u) == 0u && hot[0] != SIZE_MAX) {
        selected = hot[(state->turn & 2u) && hot[1] != SIZE_MAX ? 1 : 0];
        cpu_selected = true;
    } else for (size_t k = 0; k < available; ++k) {
        const size_t i = state->cursor++ % available;
        if (!pthread_equal(samples[i].host, pthread_self())) { selected = i; break; }
    }
    ++state->turn;
    if (selected != SIZE_MAX) {
        const SsrnxThreadSample *s = &samples[selected];
        SsrnxNativeContext context = {0};
        const uint64_t begin_us = ssrnx_perf_now_us();
        const int rc = ssrnx_bionic_thread_sample_stack(s->host, &context);
        const uint64_t end_us = ssrnx_perf_now_us();
        const uint64_t cost_us = end_us >= begin_us ? end_us - begin_us : 0;
        ++state->attempts;
        state->successful += !rc;
        state->capture_us += cost_us;
        if (cost_us > state->capture_max_us) state->capture_max_us = cost_us;
        char pc_name[192] = "host/unmapped", lr_name[192] = "host/unmapped";
        if (!rc) {
            (void)ssrnx_compat_describe_guest_address(context.pc, pc_name, sizeof(pc_name));
            (void)ssrnx_compat_describe_guest_address(context.lr, lr_name, sizeof(lr_name));
            ssrnx_perf_native_sample(s->host, context.pc, s->cpu_ticks, s->cpu_valid);
        }
        SSRNX_LOGCI("PROF", "native sample ms=%llu name=%s pthread=%p waiting=%d rc=%d pc=%p (hint=%s) lr=%p (hint=%s) sp=%p cpuTicks=%llu cpuAvailable=%u captureBudget=8/s selection=%s cpuDeltaTicks=%llu deltaSpanMs=%llu captureUs=%llu captureStatus=%u pauseRc=0x%x contextRc=0x%x resumeRc=0x%x timeUs=%llu",
                    (unsigned long long)elapsed_ms, s->name[0] ? s->name : "-",
                    (void *)(uintptr_t)s->host, s->waiting ? 1 : 0, rc,
                    (void *)context.pc, pc_name, (void *)context.lr, lr_name, (void *)context.sp,
                    (unsigned long long)s->cpu_ticks, s->cpu_valid ? 1u : 0u,
                    cpu_selected ? "cpu" : "rotation", (unsigned long long)deltas[selected],
                    (unsigned long long)span_ms, (unsigned long long)cost_us,
                    context.capture_status, (unsigned)context.pause_result,
                    (unsigned)context.context_result, (unsigned)context.resume_result,
                    (unsigned long long)begin_us);
        if (!rc) {
            char pcs[192] = "";
            size_t used = 0;
            for (unsigned i = 0; i < context.stack_count; ++i) {
                const int written = snprintf(pcs + used, sizeof(pcs) - used, "%s%p",
                                             i ? "," : "", (void *)context.stack[i]);
                if (written <= 0 || (size_t)written >= sizeof(pcs) - used) break;
                used += (size_t)written;
            }
            SSRNX_LOGCI("PROF", "native stack ms=%llu pthread=%p depth=%u status=%u pcs=%s",
                        (unsigned long long)elapsed_ms, (void *)(uintptr_t)s->host,
                        context.stack_count, context.stack_status, pcs);
        }
    }
#else
    (void)samples; (void)count; (void)elapsed_ms; (void)state;
#endif
}

static bool dex_vm_dex_prof_self_ticks(uint64_t *out) {
#ifdef __SWITCH__
    return R_SUCCEEDED(svcGetInfo(out, InfoType_ThreadTickCount, CUR_THREAD_HANDLE, UINT64_MAX));
#elif defined(__linux__)
    struct timespec now;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now) != 0) return false;
    *out = (uint64_t)now.tv_sec * UINT64_C(1000000000) + (uint64_t)now.tv_nsec;
    return true;
#else
    (void)out; return false;
#endif
}

static void *dex_vm_dex_prof_main(void *arg) {
    (void)arg;
    /* Never on the reserved render core: sampling must not delay frames. */
    (void)ssrnx_thread_policy_pin_background();
    enum { MAX_THREADS = 128 };
    SsrnxThreadSample *samples = calloc(MAX_THREADS, sizeof(*samples));
    dex_vm_DexProfThreadStat *threads = calloc(MAX_THREADS, sizeof(*threads));
    const bool deep = ssrnx_perf_mode() == SSRNX_PERF_DEEP;
    dex_vm_DexProfMethodStat *methods = deep ? calloc(DEX_PROF_METHOD_BUCKETS, sizeof(*methods)) : NULL;
    if (!samples || !threads || (deep && !methods)) {
        SSRNX_LOGCW("PROF", "sampler unavailable: allocation failed; gameplay continues without samples");
        free(samples); free(threads); free(methods); return NULL;
    }
    size_t thread_count = 0;
    uint32_t ticks = 0;
    struct timespec window_begin;
    clock_gettime(CLOCK_MONOTONIC, &window_begin);
    const struct timespec startup_begin = window_begin;
    dex_vm_DexProfNativeState native = {0};
    uint64_t sweep_us = 0, snapshot_full = 0, thread_overflow = 0, method_overflow = 0;
    uint64_t self_ticks = 0;
    bool self_valid = dex_vm_dex_prof_self_ticks(&self_ticks);
    SSRNX_LOGCI("PROF", "capture-schema version=2 nativeEnabled=%u nativeBudget=8/s stackCap=%u gapTriggerUs=33000 gpuTiming=unavailable schedulerLatency=unavailable cpuAttribution=residency",
                deep ? 1u : 0u, SSRNX_NATIVE_STACK_CAP);
    SSRNX_LOGCI("PROF", "symbol-anchor name=apn64_dex_profiler_start address=%p", (void *)(uintptr_t)&ssrnx_dex_profiler_start);
    SSRNX_LOGCI("PROF", "symbol-anchor name=apn64_bionic_thread_samples address=%p", (void *)(uintptr_t)&ssrnx_bionic_thread_samples);
    SsrnxDexRuntimeProgress previous_progress = {0};
    while (!atomic_load_explicit(&dex_vm_g_prof_stop, memory_order_acquire)) {
        const unsigned interval_us = ssrnx_perf_mode() == SSRNX_PERF_DEEP ? DEX_PROF_SAMPLE_US : 100000u;
        const struct timespec pause = {0, (long)interval_us * 1000L};
        nanosleep(&pause, NULL);
        if (atomic_load_explicit(&dex_vm_g_prof_stop, memory_order_acquire)) break;
        const uint64_t sweep_begin = ssrnx_perf_now_us();
        const size_t n = ssrnx_bionic_thread_samples(samples, MAX_THREADS);
        snapshot_full += n == MAX_THREADS;
        ticks++;
        for (size_t i = 0; i < n; ++i) {
            dex_vm_DexProfThreadStat *t = NULL;
            for (size_t k = 0; k < thread_count; ++k)
                if (threads[k].guest_id == samples[i].guest_id &&
                    pthread_equal(threads[k].host, samples[i].host)) { t = &threads[k]; break; }
            if (!t && thread_count < MAX_THREADS) {
                t = &threads[thread_count++];
                memset(t, 0, sizeof(*t));
                t->host = samples[i].host;
                t->guest_id = samples[i].guest_id;
            }
            if (!t) { ++thread_overflow; continue; }
            memcpy(t->name, samples[i].name, sizeof(t->name));
            /* A recycled host thread handle restarts its CPU counter. */
            if (samples[i].cpu_valid) {
                if (!t->cpu_valid || samples[i].cpu_ticks < t->last_ticks) t->first_ticks = samples[i].cpu_ticks;
                t->last_ticks = samples[i].cpu_ticks;
                t->cpu_valid = true;
            }
            t->samples++;
            if (samples[i].waiting) continue;
            t->busy++;
            const SsrnxDexMethodImage *m = NULL;
            const char *hcls = NULL, *hname = NULL;
            char cls[112] = "", name[48] = "", key[274], host[99] = "";
            pthread_mutex_lock(&dex_vm_g_prof_lock);
            for (size_t k = 0; k < DEX_PROF_SLOTS; ++k) {
                if (atomic_load_explicit(&dex_vm_g_prof_slots[k].used, memory_order_acquire) &&
                    pthread_equal(dex_vm_g_prof_slots[k].host, samples[i].host)) {
                    m = atomic_load_explicit(&dex_vm_g_prof_slots[k].innermost, memory_order_acquire);
                    hcls = atomic_load_explicit(&dex_vm_g_prof_slots[k].host_class, memory_order_acquire);
                    hname = atomic_load_explicit(&dex_vm_g_prof_slots[k].host_method, memory_order_acquire);
                    if (deep && m) {
                        dex_vm_dex_prof_copy(cls, sizeof(cls), m->declaring_class);
                        dex_vm_dex_prof_copy(name, sizeof(name), m->method_name);
                        if (hcls && hname) {
                            char hc[64], hn[32];
                            dex_vm_dex_prof_copy(hc, sizeof(hc), hcls);
                            dex_vm_dex_prof_copy(hn, sizeof(hn), hname);
                            snprintf(host, sizeof(host), " > %s.%s", hc, hn);
                        }
                    }
                    break;
                }
            }
            pthread_mutex_unlock(&dex_vm_g_prof_lock);
            if (!m) continue;
            t->java++;
            if (!deep) continue;
            snprintf(key, sizeof(key), "%.15s|%s.%s%s", t->name[0] ? t->name : "-", cls, name, host);
            uint64_t h = UINT64_C(1469598103934665603);
            for (const char *p = key; *p; ++p) { h ^= (uint8_t)*p; h *= UINT64_C(1099511628211); }
            bool stored = false;
            for (size_t probe = 0; probe < 16; ++probe) {
                dex_vm_DexProfMethodStat *e = &methods[(h + probe) % DEX_PROF_METHOD_BUCKETS];
                if (!e->used) { e->used = true; memcpy(e->key, key, sizeof(e->key)); e->busy = 1; stored = true; break; }
                if (strcmp(e->key, key) == 0) { e->busy++; stored = true; break; }
            }
            method_overflow += !stored;
        }
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        const int64_t startup_ns = (int64_t)(now.tv_sec - startup_begin.tv_sec) * INT64_C(1000000000) +
                                    (int64_t)now.tv_nsec - startup_begin.tv_nsec;
        const uint64_t startup_ms = startup_ns > 0 ? (uint64_t)(startup_ns / 1000000) : 0;
        if (ssrnx_perf_mode() == SSRNX_PERF_DEEP)
            dex_vm_dex_prof_native_progress(samples, n, startup_ms, &native);
        const uint64_t sweep_end = ssrnx_perf_now_us();
        if (sweep_end >= sweep_begin) sweep_us += sweep_end - sweep_begin;
        const uint64_t elapsed_ms = (uint64_t)(now.tv_sec - window_begin.tv_sec) * 1000u +
                                    (uint64_t)((now.tv_nsec - window_begin.tv_nsec) / 1000000L);
        if (elapsed_ms < DEX_PROF_REPORT_MS || !ticks) continue;
        ssrnx_perf_report();
        if (ssrnx_bionic_thread_diagnostics_report) ssrnx_bionic_thread_diagnostics_report();
        ssrnx_compat_log_memory_state("profiler-window");
        uint64_t current_self = 0;
        const bool current_self_valid = dex_vm_dex_prof_self_ticks(&current_self);
        SSRNX_LOGCI("PROF", "coverage windowMs=%llu snapshotAtCapacity=%llu threadOverflow=%llu methodOverflow=%llu sweepUs=%llu nativeAttempts=%llu nativeSuccess=%llu captureTotalUs=%llu captureMaxUs=%llu selfCpuTicks=%llu selfCpuAvailable=%u",
                    (unsigned long long)elapsed_ms, (unsigned long long)snapshot_full,
                    (unsigned long long)thread_overflow, (unsigned long long)method_overflow,
                    (unsigned long long)sweep_us, (unsigned long long)native.attempts,
                    (unsigned long long)native.successful, (unsigned long long)native.capture_us,
                    (unsigned long long)native.capture_max_us,
                    (unsigned long long)(current_self_valid && self_valid && current_self >= self_ticks ? current_self - self_ticks : 0),
                    current_self_valid && self_valid && current_self >= self_ticks ? 1u : 0u);
        self_ticks = current_self; self_valid = current_self_valid;
        sweep_us = snapshot_full = thread_overflow = method_overflow = 0;
        native.attempts = native.successful = native.capture_us = native.capture_max_us = 0;
        SsrnxDexRuntimeProgress progress;
        ssrnx_dex_get_runtime_progress(&progress);
        SSRNX_LOGCI("PROF", "dex-progress windowMs=%llu instructions=%llu calls=%llu nativeCalls=%llu (wall-window counts, not interpreter CPU throughput)",
                    (unsigned long long)elapsed_ms,
                    (unsigned long long)(progress.instructions - previous_progress.instructions),
                    (unsigned long long)(progress.calls - previous_progress.calls),
                    (unsigned long long)(progress.native_calls - previous_progress.native_calls));
        previous_progress = progress;
        qsort(threads, thread_count, sizeof(*threads), dex_vm_dex_prof_thread_cmp);
        SSRNX_LOGCI("PROF", "window ms=%llu ticks=%u threads=%zu (cpu = kernel CPU time of one core; busy = not in a tracked wait)",
                    (unsigned long long)elapsed_ms, ticks, thread_count);
#ifdef __SWITCH__
        if (deep) (void)ssrnx_bionic_pthread_log_wait_states("runtime-native-progress");
#endif
#ifdef __SWITCH__
        const uint64_t window_ticks = elapsed_ms * (armGetSystemTickFreq() / 1000u);
#else
        const uint64_t window_ticks = elapsed_ms * 1000000u; /* host samples report nanoseconds */
#endif
        SSRNX_LOGCI("PROF", "cpu-clock frequency=%llu", (unsigned long long)(window_ticks * 1000u / elapsed_ms));
        for (size_t k = 0; k < thread_count && k < 16; ++k) {
            const dex_vm_DexProfThreadStat *t = &threads[k];
            const uint64_t cpu = t->last_ticks - t->first_ticks;
            if (!t->busy && !cpu) break;
            SSRNX_LOGCI("PROF", "thread name=%s cpu=%llu%% busy=%u%% java=%u%% samples=%u cpuAvailable=%u pthread=%p",
                        t->name[0] ? t->name : "-",
                        (unsigned long long)(window_ticks ? cpu * 100u / window_ticks : 0),
                        t->busy * 100u / ticks, t->java * 100u / ticks, t->samples,
                        t->cpu_valid ? 1u : 0u, (void *)(uintptr_t)t->host);
            SsrnxThreadScheduling scheduling;
            if (ssrnx_bionic_thread_scheduling && ssrnx_bionic_thread_scheduling(t->host, &scheduling))
                SSRNX_LOGCI("PROF", "placement pthread=%p thread=%llu tid=%d effectiveMask=0x%llx idealCore=%d hostPriority=%d coreResult=0x%x priorityResult=0x%x",
                            (void *)(uintptr_t)t->host, (unsigned long long)scheduling.guest_id,
                            scheduling.android_tid, (unsigned long long)scheduling.effective_mask,
                            scheduling.ideal_core, scheduling.host_priority,
                            (unsigned)scheduling.core_result, (unsigned)scheduling.priority_result);
        }
        if (methods) qsort(methods, DEX_PROF_METHOD_BUCKETS, sizeof(*methods), dex_vm_dex_prof_method_cmp);
        for (size_t k = 0; methods && k < 20 && methods[k].used; ++k)
            SSRNX_LOGCI("PROF", "java %u%% %s", methods[k].busy * 100u / ticks, methods[k].key);
        if (ssrnx_perf_lock_take_deltas) {
            SsrnxPerfLockDelta locks[12];
            const size_t lock_count = ssrnx_perf_lock_take_deltas(locks, 12);
            for (size_t k = 0; k < lock_count; ++k) {
                const char *file = locks[k].site->file ? locks[k].site->file : "?";
                const char *base = strrchr(file, '/');
                SSRNX_LOGCI("PROF", "lock wait=%llums (%llu%% of window) contended=%llu site=%s:%d %s",
                            (unsigned long long)(locks[k].wait_us / 1000u),
                            (unsigned long long)(elapsed_ms ? locks[k].wait_us / 10u / elapsed_ms : 0),
                            (unsigned long long)locks[k].contended, base ? base + 1 : file,
                            locks[k].site->line, locks[k].site->name ? locks[k].site->name : "");
            }
        }
        if (ssrnx_heap_cache_take_stats) {
            SsrnxHeapCacheStat allocs[10];
            const size_t alloc_count = ssrnx_heap_cache_take_stats(allocs, 10);
            qsort(allocs, alloc_count, sizeof(*allocs), dex_vm_dex_prof_alloc_cmp);
            for (size_t k = 0; k < alloc_count; ++k)
                SSRNX_LOGCI("PROF", "host-alloc %s size=%zu count=%u", allocs[k].kind,
                            allocs[k].size, allocs[k].count);
        }
        /* Only TLS teardown retires a slot. A bounded registry snapshot can
         * omit a live thread, so absence there is not proof of thread exit. */
        memset(threads, 0, MAX_THREADS * sizeof(*threads));
        if (methods) memset(methods, 0, DEX_PROF_METHOD_BUCKETS * sizeof(*methods));
        thread_count = 0;
        ticks = 0;
        window_begin = now;
    }
    free(samples);
    free(threads);
    free(methods);
    return NULL;
}

static void dex_vm_dex_prof_start(void) {
    const int key_rc = pthread_key_create(&dex_vm_g_prof_key, dex_vm_dex_prof_thread_exit);
    dex_vm_g_prof_key_ready = key_rc == 0;
    if (!dex_vm_g_prof_key_ready) SSRNX_LOGCW("PROF", "DEX method sampling disabled: TLS key hostError=%d", key_rc);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 64u * 1024u);
    const int rc = pthread_create(&dex_vm_g_prof_thread, &attr, dex_vm_dex_prof_main, NULL);
    if (rc == 0) {
        atomic_store_explicit(&dex_vm_g_prof_thread_running, true, memory_order_release);
        SSRNX_LOGCI("PROF", "sampling profiler started mode=%s intervalUs=%u reportMs=%u (method samples are wall residency, not CPU attribution)",
                    ssrnx_perf_mode() == SSRNX_PERF_DEEP ? "DEEP" : "LIGHT",
                    ssrnx_perf_mode() == SSRNX_PERF_DEEP ? DEX_PROF_SAMPLE_US : 100000u, DEX_PROF_REPORT_MS);
#ifdef __SWITCH__
        SSRNX_LOGCI("PROF", "native CPU counter frequencyHz=%llu; address hints may be nearest exports, not exact functions",
                    (unsigned long long)armGetSystemTickFreq());
#endif
    } else {
        SSRNX_LOGCW("PROF", "sampler unavailable: pthread_create hostError=%d", rc);
    }
    pthread_attr_destroy(&attr);
}

void ssrnx_dex_profiler_start(void) {
    if (ssrnx_perf_mode() != SSRNX_PERF_OFF) pthread_once(&dex_vm_g_prof_once, dex_vm_dex_prof_start);
}

/* The sampler is joined before the process returns to the loader: a host
 * thread still running when hbloader unmaps the NRO faults on its next
 * instruction fetch. */
void ssrnx_dex_profiler_shutdown(void) {
    if (!atomic_exchange_explicit(&dex_vm_g_prof_thread_running, false, memory_order_acq_rel)) return;
    atomic_store_explicit(&dex_vm_g_prof_stop, true, memory_order_release);
    (void)pthread_join(dex_vm_g_prof_thread, NULL);
    ssrnx_perf_report();
    if (ssrnx_bionic_thread_diagnostics_report) ssrnx_bionic_thread_diagnostics_report();
}


typedef struct dex_vm_DexExecCtx {
    const char *source_path;
    void *env;
    SsrnxDexExecReport *report;
    size_t steps;
    size_t calls;
    size_t published_steps;
    size_t published_calls;
    uint64_t start_ms;
    size_t next_progress_steps;
    const struct SsrnxDexMethodImage *current; /* executing frame, for diagnostics */
} dex_vm_DexExecCtx;

static void dex_vm_fail_report(dex_vm_DexExecCtx *ctx, const char *cls, const char *name, const char *sig,
                        uint32_t pc, uint8_t opcode, const char *detail);
static bool dex_vm_copy_text(char *dst, size_t cap, const char *src);
static SsrnxResult dex_vm_ensure_dex_class_initialized(dex_vm_DexExecCtx *ctx, const char *class_name,
                                                       unsigned depth);

static uint64_t dex_vm_dex_monotonic_ms(void) {
    struct timespec ts = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * UINT64_C(1000) + (uint64_t)ts.tv_nsec / UINT64_C(1000000);
}

static uint64_t dex_vm_dex_monotonic_us(void) {
    struct timespec ts = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * UINT64_C(1000000) + (uint64_t)ts.tv_nsec / UINT64_C(1000);
}

static uint64_t dex_vm_g_last_slow_native_log_us;

/* Process-wide execution evidence for startup-stall diagnostics.  Publishing
 * every DEX opcode would distort interpreter performance, so each execution
 * context batches its instruction/call deltas and flushes them at invoke sites
 * and at the existing 4096-op watchdog poll boundary. */
static uint64_t dex_vm_g_runtime_progress_instructions;
static uint64_t dex_vm_g_runtime_progress_calls;
static uint64_t dex_vm_g_runtime_progress_native_calls;
static pthread_mutex_t dex_vm_g_runtime_progress_lock = PTHREAD_MUTEX_INITIALIZER;
static SsrnxDexRuntimeProgress dex_vm_g_runtime_progress_location;

static void dex_vm_dex_progress_copy(char *out, size_t cap, const char *in) {
    const size_t n = strnlen(in, cap - 1u);
    memcpy(out, in, n);
    out[n] = '\0';
}

static void dex_vm_dex_runtime_progress_location(const char *kind,
                                          const char *class_name,
                                          const char *method_name,
                                          const char *signature,
                                          uint32_t pc, uint8_t opcode) {
    /* Own the strings before publishing: frame metadata may be released when
     * the call returns. Keep parsing/copying outside the shared lock. */
    SsrnxDexRuntimeProgress next = {0};
    dex_vm_dex_progress_copy(next.kind, sizeof(next.kind), kind ? kind : "dex");
    dex_vm_dex_progress_copy(next.class_name, sizeof(next.class_name), class_name ? class_name : "?");
    dex_vm_dex_progress_copy(next.method_name, sizeof(next.method_name), method_name ? method_name : "?");
    dex_vm_dex_progress_copy(next.signature, sizeof(next.signature), signature ? signature : "?");
    next.pc = pc;
    next.opcode = opcode;
    SSRNX_PERF_LOCK(&dex_vm_g_runtime_progress_lock);
    next.last_update_ms = dex_vm_dex_monotonic_ms();
    dex_vm_g_runtime_progress_location = next;
    pthread_mutex_unlock(&dex_vm_g_runtime_progress_lock);
}

static void dex_vm_dex_runtime_progress_publish(dex_vm_DexExecCtx *ctx,
                                         const struct SsrnxDexMethodImage *m,
                                         uint32_t pc, uint8_t opcode) {
    if (!ctx) return;
    const size_t step_delta = ctx->steps - ctx->published_steps;
    const size_t call_delta = ctx->calls - ctx->published_calls;
    if (step_delta)
        __atomic_add_fetch(&dex_vm_g_runtime_progress_instructions, (uint64_t)step_delta, __ATOMIC_RELAXED);
    if (call_delta)
        __atomic_add_fetch(&dex_vm_g_runtime_progress_calls, (uint64_t)call_delta, __ATOMIC_RELAXED);
    ctx->published_steps = ctx->steps;
    ctx->published_calls = ctx->calls;
    if ((step_delta || call_delta) && m)
        dex_vm_dex_runtime_progress_location("dex", m->declaring_class, m->method_name, m->signature, pc, opcode);
}

static void dex_vm_dex_runtime_progress_native(const char *class_name,
                                        const char *method_name,
                                        const char *signature) {
    __atomic_add_fetch(&dex_vm_g_runtime_progress_native_calls, UINT64_C(1), __ATOMIC_RELAXED);
    dex_vm_dex_runtime_progress_location("native-jni", class_name, method_name, signature, 0, 0);
}

void ssrnx_dex_get_runtime_progress(SsrnxDexRuntimeProgress *out_progress) {
    if (!out_progress) return;
    memset(out_progress, 0, sizeof(*out_progress));
    out_progress->instructions = __atomic_load_n(&dex_vm_g_runtime_progress_instructions, __ATOMIC_RELAXED);
    out_progress->calls = __atomic_load_n(&dex_vm_g_runtime_progress_calls, __ATOMIC_RELAXED);
    out_progress->native_calls = __atomic_load_n(&dex_vm_g_runtime_progress_native_calls, __ATOMIC_RELAXED);
    SSRNX_PERF_LOCK(&dex_vm_g_runtime_progress_lock);
    out_progress->last_update_ms = dex_vm_g_runtime_progress_location.last_update_ms;
    out_progress->pc = dex_vm_g_runtime_progress_location.pc;
    out_progress->opcode = dex_vm_g_runtime_progress_location.opcode;
    memcpy(out_progress->kind, dex_vm_g_runtime_progress_location.kind, sizeof(out_progress->kind));
    memcpy(out_progress->class_name, dex_vm_g_runtime_progress_location.class_name, sizeof(out_progress->class_name));
    memcpy(out_progress->method_name, dex_vm_g_runtime_progress_location.method_name, sizeof(out_progress->method_name));
    memcpy(out_progress->signature, dex_vm_g_runtime_progress_location.signature, sizeof(out_progress->signature));
    pthread_mutex_unlock(&dex_vm_g_runtime_progress_lock);
}

static void dex_vm_dex_log_long_run(dex_vm_DexExecCtx *ctx, const SsrnxDexMethodImage *m, uint32_t pc, uint8_t op) {
    if (!ctx || !m) return;
    const uint64_t now_ms = dex_vm_dex_monotonic_ms();
    const uint64_t elapsed_ms = (ctx->start_ms && now_ms >= ctx->start_ms) ? now_ms - ctx->start_ms : 0;
    SSRNX_LOGCI("DEX", "long-run progress class=%s method=%s sig=%s instructions=%zu calls=%zu elapsedMs=%" PRIu64 " pc=%#x opcode=0x%02x",
                m->declaring_class, m->method_name, m->signature, ctx->steps, ctx->calls,
                elapsed_ms, pc, op);
}

/* Detect a tiny but provably non-terminating DEX loop. The supported shape is
 * intentionally narrow: one pure
 * integer literal operation (22s/22b) immediately followed by goto/8 back to
 * that same operation.  No framework/JNI/native call or memory operation can
 * occur inside the cycle, and Dalvik registers are frame-local, so once control
 * enters this pair there is no observable mechanism that can make it exit.
 *
 * This is diagnostic correctness, not a compatibility bypass.  We stop at the
 * first impossible-to-exit cycle and log the predecessor/code/register state so
 * the *earlier* control-flow semantic that admitted the path can be fixed. */
static bool dex_vm_dex_literal_goto_cycle(const SsrnxDexMethodImage *m, uint32_t pc,
                                   int8_t *out_rel, uint32_t *out_goto_pc) {
    if (!m || !m->insns || pc >= m->insns_size) return false;
    const uint8_t op = (uint8_t)(m->insns[pc] & 0xffu);
    if (!((op >= 0xd0u && op <= 0xd7u) || (op >= 0xd8u && op <= 0xe2u))) return false;
    if ((uint64_t)pc + 2u >= m->insns_size) return false;
    const uint32_t goto_pc = pc + 2u;
    const uint16_t goto_word = m->insns[goto_pc];
    if ((uint8_t)(goto_word & 0xffu) != 0x28u) return false;
    const int8_t rel = (int8_t)(goto_word >> 8);
    const int64_t target = (int64_t)goto_pc + rel;
    if (target != (int64_t)pc) return false;
    if (out_rel) *out_rel = rel;
    if (out_goto_pc) *out_goto_pc = goto_pc;
    return true;
}

static void dex_vm_dex_log_state_window(const SsrnxDexMethodImage *m, const uint64_t *regs,
                                 uint32_t pc, const char *label) {
    if (!m || !regs || !label) return;
    const uint32_t first = pc > 8u ? pc - 8u : 0u;
    uint32_t last = pc + 8u;
    if (last >= m->insns_size) last = m->insns_size ? m->insns_size - 1u : 0u;
    char code[768];
    size_t used = 0;
    code[0] = '\0';
    for (uint32_t i = first; i <= last && used + 24u < sizeof(code); ++i) {
        int n = snprintf(code + used, sizeof(code) - used, "%s%#x:%04x",
                         used ? " " : "", i, (unsigned)m->insns[i]);
        if (n <= 0 || (size_t)n >= sizeof(code) - used) break;
        used += (size_t)n;
    }
    SSRNX_LOGCI("DEX", "%s code pc=%#x regs=%u ins=%u words=%s",
                label, pc, m->registers_size, m->ins_size, code);

    const uint32_t reg_count = m->registers_size < 24u ? m->registers_size : 24u;
    for (uint32_t base = 0; base < reg_count; base += 4u) {
        char line[384];
        size_t pos = 0;
        line[0] = '\0';
        const uint32_t end = (base + 4u < reg_count) ? base + 4u : reg_count;
        for (uint32_t r = base; r < end && pos + 64u < sizeof(line); ++r) {
            int n = snprintf(line + pos, sizeof(line) - pos, "%sv%u=%#018" PRIx64,
                             pos ? " " : "", r, regs[r]);
            if (n <= 0 || (size_t)n >= sizeof(line) - pos) break;
            pos += (size_t)n;
        }
        SSRNX_LOGCI("DEX", "%s regs %s", label, line);
    }
    if (m->registers_size > reg_count)
        SSRNX_LOGCI("DEX", "%s regs truncated shown=%u total=%u", label, reg_count, m->registers_size);
}

static void dex_vm_dex_log_cycle_state(const SsrnxDexMethodImage *m, const uint64_t *regs,
                                uint32_t pc, bool have_previous, uint32_t previous_pc,
                                uint8_t previous_op, int8_t rel, uint32_t goto_pc) {
    if (!m || !regs) return;
    SSRNX_LOGCW("DEX",
                "non-terminating local loop class=%s method=%s sig=%s pc=%#x gotoPc=%#x gotoRel=%d",
                m->declaring_class, m->method_name, m->signature, pc, goto_pc, (int)rel);
    if (have_previous) {
        SSRNX_LOGCI("DEX", "loop-entry predecessor pc=%#x opcode=0x%02x -> loopPc=%#x",
                    previous_pc, previous_op, pc);
    } else {
        SSRNX_LOGCI("DEX", "loop-entry predecessor unavailable method entered at loopPc=%#x", pc);
    }
    dex_vm_dex_log_state_window(m, regs, pc, "loop-entry");
}

static SsrnxDexNativeResolver dex_vm_g_native_resolver;
static void *dex_vm_g_native_resolver_user;

#define DEX_NATIVE_RESOLVE_CACHE_MAX 128u
typedef struct dex_vm_DexNativeResolveCacheEntry {
    bool used;
    bool found;
    char class_name[256];
    char method_name[128];
    char signature[1024];
    void *fn;
} dex_vm_DexNativeResolveCacheEntry;
static dex_vm_DexNativeResolveCacheEntry dex_vm_g_native_resolve_cache[DEX_NATIVE_RESOLVE_CACHE_MAX];
static size_t dex_vm_g_native_resolve_cache_next;
static uint64_t dex_vm_g_native_resolve_cache_hits;
static uint64_t dex_vm_g_native_resolve_cache_misses;
static pthread_mutex_t dex_vm_g_native_resolve_cache_lock = PTHREAD_MUTEX_INITIALIZER;

/* JNI method IDs bound to package DEX methods. Registered JNI methods keep a
 * pointer to their binding for the life of the JNI model, so bindings live in
 * chunks that are recycled (marked unused) on reset but never freed. */
#define DEX_JNI_PROXY_CHUNK 64u
typedef struct dex_vm_DexJniProxyBinding {
    bool used;
    bool virtual_dispatch;
    char class_name[512];
    char method_name[256];
    char signature[SSRNX_DEX_SIGNATURE_CAP];
} dex_vm_DexJniProxyBinding;
typedef struct dex_vm_DexJniProxyChunk {
    struct dex_vm_DexJniProxyChunk *next;
    size_t used; /* slots handed out from this chunk */
    dex_vm_DexJniProxyBinding slots[DEX_JNI_PROXY_CHUNK];
} dex_vm_DexJniProxyChunk;

static char dex_vm_g_java_method_source[SSRNX_APK_PATH_CAP];
static dex_vm_DexJniProxyChunk *dex_vm_g_java_method_bindings;
static pthread_mutex_t dex_vm_g_java_method_bindings_lock = PTHREAD_MUTEX_INITIALIZER;

typedef enum dex_vm_DexClassInitState {
    DEX_CLASS_INIT_NEW = 0,
    DEX_CLASS_INIT_RUNNING,
    DEX_CLASS_INIT_DONE,
    DEX_CLASS_INIT_FAILED,
} dex_vm_DexClassInitState;

typedef struct dex_vm_DexClassInitEntry {
    bool used;
    char class_name[512];
    dex_vm_DexClassInitState state;
    pthread_t owner;
    SsrnxResult failure_rc;
} dex_vm_DexClassInitEntry;

/* Per-class initialization state (JLS 12.4.2). Real apps touch thousands of
 * classes, so the table is an open-addressed hash that grows on demand.
 * Entries are allocated individually and never freed while the process runs:
 * threads blocked in pthread_cond_wait() hold entry pointers across unlock,
 * so a rehash moves only the pointer slots, never the entries. */
static dex_vm_DexClassInitEntry **dex_vm_g_class_init_slots;
static size_t dex_vm_g_class_init_cap;   /* power of two */
static size_t dex_vm_g_class_init_count;
static pthread_mutex_t dex_vm_g_class_init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t dex_vm_g_class_init_cond = PTHREAD_COND_INITIALIZER;

static uint64_t dex_vm_dex_class_init_hash(const char *s) {
    uint64_t h = UINT64_C(1469598103934665603);
    while (*s) { h ^= (unsigned char)*s++; h *= UINT64_C(1099511628211); }
    return h;
}

static void dex_vm_dex_class_init_reset(void) {
    SSRNX_PERF_LOCK(&dex_vm_g_class_init_lock);
    /* Keep entry storage alive (a waiter may still reference it); mark every
     * entry unused so the next process run starts from a clean table. */
    for (size_t i = 0; i < dex_vm_g_class_init_cap; ++i) {
        dex_vm_DexClassInitEntry *e = dex_vm_g_class_init_slots[i];
        if (!e) continue;
        e->used = false;
        e->state = DEX_CLASS_INIT_NEW;
        e->failure_rc = SSRNX_OK;
    }
    pthread_cond_broadcast(&dex_vm_g_class_init_cond);
    pthread_mutex_unlock(&dex_vm_g_class_init_lock);
}

static bool dex_vm_dex_class_init_grow_locked(void) {
    const size_t next_cap = dex_vm_g_class_init_cap ? dex_vm_g_class_init_cap * 2u : 1024u;
    dex_vm_DexClassInitEntry **next = calloc(next_cap, sizeof(*next));
    if (!next) return false;
    for (size_t i = 0; i < dex_vm_g_class_init_cap; ++i) {
        dex_vm_DexClassInitEntry *e = dex_vm_g_class_init_slots[i];
        if (!e) continue;
        size_t j = (size_t)dex_vm_dex_class_init_hash(e->class_name) & (next_cap - 1u);
        while (next[j]) j = (j + 1u) & (next_cap - 1u);
        next[j] = e;
    }
    free(dex_vm_g_class_init_slots);
    dex_vm_g_class_init_slots = next;
    dex_vm_g_class_init_cap = next_cap;
    return true;
}

static dex_vm_DexClassInitEntry *dex_vm_dex_class_init_entry_locked(const char *class_name) {
    if (!class_name || strlen(class_name) >= sizeof(((dex_vm_DexClassInitEntry *)0)->class_name)) return NULL;
    if ((dex_vm_g_class_init_count + 1u) * 10u >= dex_vm_g_class_init_cap * 7u && !dex_vm_dex_class_init_grow_locked())
        return NULL;
    size_t i = (size_t)dex_vm_dex_class_init_hash(class_name) & (dex_vm_g_class_init_cap - 1u);
    while (dex_vm_g_class_init_slots[i]) {
        dex_vm_DexClassInitEntry *e = dex_vm_g_class_init_slots[i];
        if (strcmp(e->class_name, class_name) == 0) {
            e->used = true; /* revive an entry left unused by reset */
            return e;
        }
        i = (i + 1u) & (dex_vm_g_class_init_cap - 1u);
    }
    dex_vm_DexClassInitEntry *e = calloc(1, sizeof(*e));
    if (!e) return NULL;
    e->used = true;
    (void)dex_vm_copy_text(e->class_name, sizeof(e->class_name), class_name);
    dex_vm_g_class_init_slots[i] = e;
    dex_vm_g_class_init_count++;
    return e;
}


void ssrnx_dex_set_native_resolver(SsrnxDexNativeResolver resolver, void *user_data) {
    /* Called before guest Java starts and after all guest workers are idle. */
    pthread_mutex_lock(&dex_vm_g_native_resolve_cache_lock);
    const bool changed = resolver != dex_vm_g_native_resolver || user_data != dex_vm_g_native_resolver_user;
    dex_vm_g_native_resolver = resolver;
    dex_vm_g_native_resolver_user = user_data;
    if (changed) {
        memset(dex_vm_g_native_resolve_cache, 0, sizeof(dex_vm_g_native_resolve_cache));
        dex_vm_g_native_resolve_cache_next = 0;
        dex_vm_g_native_resolve_cache_hits = 0;
        dex_vm_g_native_resolve_cache_misses = 0;
    }
    pthread_mutex_unlock(&dex_vm_g_native_resolve_cache_lock);
}

void ssrnx_dex_get_native_resolver_cache_stats(uint64_t *out_hits, uint64_t *out_misses) {
    pthread_mutex_lock(&dex_vm_g_native_resolve_cache_lock);
    if (out_hits) *out_hits = dex_vm_g_native_resolve_cache_hits;
    if (out_misses) *out_misses = dex_vm_g_native_resolve_cache_misses;
    pthread_mutex_unlock(&dex_vm_g_native_resolve_cache_lock);
}

static uint16_t dex_vm_rd16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t dex_vm_rd32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

static bool dex_vm_copy_text(char *dst, size_t cap, const char *src) {
    if (!dst || cap == 0 || !src) return false;
    const size_t n = strlen(src);
    if (n + 1u > cap) return false;
    memcpy(dst, src, n + 1u);
    return true;
}

static bool dex_vm_native_resolve_cache_lookup(const char *class_name, const char *method_name,
                                        const char *signature, void **out_fn, bool *out_found) {
    if (!class_name || !method_name || !signature || !out_fn || !out_found) return false;
    pthread_mutex_lock(&dex_vm_g_native_resolve_cache_lock);
    for (size_t i = 0; i < DEX_NATIVE_RESOLVE_CACHE_MAX; ++i) {
        const dex_vm_DexNativeResolveCacheEntry *e = &dex_vm_g_native_resolve_cache[i];
        if (!e->used || strcmp(e->class_name, class_name) != 0 ||
            strcmp(e->method_name, method_name) != 0 || strcmp(e->signature, signature) != 0)
            continue;
        *out_fn = e->fn;
        *out_found = e->found;
        dex_vm_g_native_resolve_cache_hits++;
        pthread_mutex_unlock(&dex_vm_g_native_resolve_cache_lock);
        return true;
    }
    dex_vm_g_native_resolve_cache_misses++;
    pthread_mutex_unlock(&dex_vm_g_native_resolve_cache_lock);
    return false;
}

static void dex_vm_native_resolve_cache_store(const char *class_name, const char *method_name,
                                       const char *signature, void *fn, bool found) {
    pthread_mutex_lock(&dex_vm_g_native_resolve_cache_lock);
    dex_vm_DexNativeResolveCacheEntry *e =
        &dex_vm_g_native_resolve_cache[dex_vm_g_native_resolve_cache_next++ % DEX_NATIVE_RESOLVE_CACHE_MAX];
    memset(e, 0, sizeof(*e));
    if (!dex_vm_copy_text(e->class_name, sizeof(e->class_name), class_name) ||
        !dex_vm_copy_text(e->method_name, sizeof(e->method_name), method_name) ||
        !dex_vm_copy_text(e->signature, sizeof(e->signature), signature)) {
        pthread_mutex_unlock(&dex_vm_g_native_resolve_cache_lock);
        return;
    }
    e->fn = fn;
    e->found = found;
    e->used = true;
    pthread_mutex_unlock(&dex_vm_g_native_resolve_cache_lock);
}

SsrnxResult ssrnx_dex_prewarm_native_method(const char *class_name,
                                            const char *method_name,
                                            const char *signature,
                                            bool *out_found) {
    if (!class_name || !method_name || !signature || !out_found)
        return SSRNX_ERR_ARGUMENT;
    *out_found = false;
    void *fn = NULL;
    bool found = false;
    SsrnxResult rc = ssrnx_jni_find_native_binding(class_name, method_name, signature, &fn, &found);
    if (rc != SSRNX_OK) {
        char detail[512];
        snprintf(detail, sizeof(detail),
                 "native JNI binding lookup failed class=%.140s method=%.80s sig=%.140s rc=%d (%s)",
                 class_name, method_name, signature, (int)rc, ssrnx_result_string(rc));
        /* Prewarm runs outside a DexExecCtx, so there is no execution report to update here. */
        SSRNX_LOGCW("DEX", "%s", detail);
        return rc;
    }
    if (!found) {
        if (!dex_vm_native_resolve_cache_lookup(class_name, method_name, signature, &fn, &found) &&
            dex_vm_g_native_resolver) {
            rc = dex_vm_g_native_resolver(class_name, method_name, signature, &fn, &found, dex_vm_g_native_resolver_user);
            if (rc != SSRNX_OK) {
                char detail[512];
                snprintf(detail, sizeof(detail),
                         "native JNI resolver failed class=%.140s method=%.80s sig=%.140s rc=%d (%s)",
                         class_name, method_name, signature, (int)rc, ssrnx_result_string(rc));
                /* Prewarm runs outside a DexExecCtx, so there is no execution report to update here. */
                SSRNX_LOGCW("DEX", "%s", detail);
                return rc;
            }
            if (found && fn) dex_vm_native_resolve_cache_store(class_name, method_name, signature, fn, true);
        }
    }
    *out_found = found && fn;
    return SSRNX_OK;
}

static bool dex_vm_dex_skip_uleb(const uint8_t *data, size_t size, size_t *off) {
    if (!data || !off) return false;
    for (unsigned i = 0; i < 5; ++i) {
        if (*off >= size) return false;
        if ((data[(*off)++] & 0x80u) == 0) return true;
    }
    return false;
}

static bool dex_vm_dex_read_uleb_value(const uint8_t *data, size_t size, size_t *off, uint32_t *out) {
    if (!data || !off || !out) return false;
    uint32_t value = 0;
    unsigned shift = 0;
    for (unsigned i = 0; i < 5; ++i, shift += 7u) {
        if (*off >= size) return false;
        const uint8_t b = data[(*off)++];
        value |= (uint32_t)(b & 0x7fu) << shift;
        if ((b & 0x80u) == 0) { *out = value; return true; }
    }
    return false;
}

static bool dex_vm_dex_read_sleb_value(const uint8_t *data, size_t size, size_t *off, int32_t *out) {
    if (!data || !off || !out) return false;
    int32_t value = 0;
    unsigned shift = 0;
    uint8_t b = 0;
    for (unsigned i = 0; i < 5; ++i) {
        if (*off >= size) return false;
        b = data[(*off)++];
        value |= (int32_t)(b & 0x7fu) << shift;
        shift += 7u;
        if ((b & 0x80u) == 0) {
            if (shift < 32u && (b & 0x40u)) value |= -(INT32_C(1) << shift);
            *out = value;
            return true;
        }
    }
    return false;
}

static const char *dex_vm_dex_string(const uint8_t *data, size_t size, uint32_t index) {
    if (!data || size < 0x70u) return NULL;
    const uint32_t count = dex_vm_rd32(data + 0x38);
    const uint32_t off = dex_vm_rd32(data + 0x3c);
    if (index >= count || (uint64_t)off + (uint64_t)count * 4u > size) return NULL;
    const uint32_t str_off = dex_vm_rd32(data + off + (size_t)index * 4u);
    if (str_off >= size) return NULL;
    size_t p = str_off;
    if (!dex_vm_dex_skip_uleb(data, size, &p)) return NULL;
    return memchr(data + p, 0, size - p) ? (const char *)(data + p) : NULL;
}

static const char *dex_vm_dex_type_desc(const uint8_t *data, size_t size, uint32_t type_idx) {
    if (!data || size < 0x70u) return NULL;
    const uint32_t count = dex_vm_rd32(data + 0x40);
    const uint32_t off = dex_vm_rd32(data + 0x44);
    if (type_idx >= count || (uint64_t)off + (uint64_t)count * 4u > size) return NULL;
    return dex_vm_dex_string(data, size, dex_vm_rd32(data + off + (size_t)type_idx * 4u));
}

static bool dex_vm_desc_to_class(const char *desc, char *out, size_t cap) {
    if (!desc || !out || cap == 0) return false;
    const size_t n = strlen(desc);
    if (n < 3 || desc[0] != 'L' || desc[n - 1] != ';' || n - 1u > cap) return false;
    if (n - 1u > cap) return false;
    memcpy(out, desc + 1, n - 2u);
    out[n - 2u] = '\0';
    return true;
}

static bool dex_vm_proto_signature(const uint8_t *data, size_t size, uint32_t proto_idx,
                            char *out, size_t cap, size_t *out_needed,
                            const char **out_reason) {
    if (out_needed) *out_needed = 0;
    if (out_reason) *out_reason = "invalid proto";
    if (!data || !out || cap < 4u || size < 0x70u) {
        if (out_reason) *out_reason = "invalid DEX/proto buffer";
        return false;
    }
    const uint32_t count = dex_vm_rd32(data + 0x48);
    const uint32_t off = dex_vm_rd32(data + 0x4c);
    if (proto_idx >= count) {
        if (out_reason) *out_reason = "proto index out of range";
        return false;
    }
    if ((uint64_t)off + (uint64_t)count * 12u > size) {
        if (out_reason) *out_reason = "proto_ids table out of bounds";
        return false;
    }
    const uint8_t *proto = data + off + (size_t)proto_idx * 12u;
    const char *ret = dex_vm_dex_type_desc(data, size, dex_vm_rd32(proto + 4u));
    const uint32_t params_off = dex_vm_rd32(proto + 8u);
    if (!ret) {
        if (out_reason) *out_reason = "invalid return type";
        return false;
    }

    /* First validate every descriptor and compute the exact descriptor length.
     * This avoids conflating a structurally-invalid proto with the historical
     * fixed scratch-buffer limit used by the interpreter. */
    size_t needed = 1u /* '(' */ + 1u /* ')' */ + strlen(ret) + 1u /* NUL */;
    uint32_t param_count = 0;
    if (params_off) {
        if ((uint64_t)params_off + 4u > size) {
            if (out_reason) *out_reason = "parameter type_list header out of bounds";
            return false;
        }
        param_count = dex_vm_rd32(data + params_off);
        if ((uint64_t)params_off + 4u + (uint64_t)param_count * 2u > size) {
            if (out_reason) *out_reason = "parameter type_list out of bounds";
            return false;
        }
        for (uint32_t i = 0; i < param_count; ++i) {
            const uint16_t tid = dex_vm_rd16(data + params_off + 4u + (size_t)i * 2u);
            const char *d = dex_vm_dex_type_desc(data, size, tid);
            if (!d) {
                if (out_reason) *out_reason = "invalid parameter type";
                return false;
            }
            const size_t dn = strlen(d);
            if (SIZE_MAX - needed < dn) {
                if (out_reason) *out_reason = "descriptor length overflow";
                return false;
            }
            needed += dn;
        }
    }
    if (out_needed) *out_needed = needed;
    if (needed > cap) {
        if (out_reason) *out_reason = "method descriptor exceeds APN64 buffer";
        return false;
    }

    size_t used = 0;
    out[used++] = '(';
    if (params_off) {
        for (uint32_t i = 0; i < param_count; ++i) {
            const uint16_t tid = dex_vm_rd16(data + params_off + 4u + (size_t)i * 2u);
            const char *d = dex_vm_dex_type_desc(data, size, tid);
            const size_t dn = strlen(d);
            memcpy(out + used, d, dn);
            used += dn;
        }
    }
    out[used++] = ')';
    const size_t rn = strlen(ret);
    memcpy(out + used, ret, rn + 1u);
    if (out_reason) *out_reason = "ok";
    return true;
}

static bool method_ref(const SsrnxDexMethodImage *m, uint32_t method_idx,
                       char *cls, size_t cls_cap, char *name, size_t name_cap,
                       char *sig, size_t sig_cap, char *detail, size_t detail_cap) {
    if (detail && detail_cap) detail[0] = '\0';
    if (!m || !m->dex_data || m->dex_size < 0x70u) {
        if (detail && detail_cap) snprintf(detail, detail_cap, "invalid/missing DEX image");
        return false;
    }
    const uint8_t *data = m->dex_data;
    const size_t size = m->dex_size;
    const uint32_t count = dex_vm_rd32(data + 0x58);
    const uint32_t off = dex_vm_rd32(data + 0x5c);
    if (method_idx >= count) {
        if (detail && detail_cap)
            snprintf(detail, detail_cap, "method_idx=%u method_ids_size=%u out of range dex=%.160s",
                     method_idx, count, m->dex_name[0] ? m->dex_name : "<unknown>");
        return false;
    }
    if ((uint64_t)off + (uint64_t)count * 8u > size) {
        if (detail && detail_cap)
            snprintf(detail, detail_cap,
                     "method_ids table out of bounds off=%#x count=%u dexSize=%zu dex=%.160s",
                     off, count, size, m->dex_name[0] ? m->dex_name : "<unknown>");
        return false;
    }
    const uint8_t *item = data + off + (size_t)method_idx * 8u;
    const uint16_t class_idx = dex_vm_rd16(item);
    const uint16_t proto_idx = dex_vm_rd16(item + 2u);
    const uint32_t name_idx = dex_vm_rd32(item + 4u);
    char tmp[512];
    const char *owner_desc = dex_vm_dex_type_desc(data, size, class_idx);
    if (!owner_desc) {
        if (detail && detail_cap)
            snprintf(detail, detail_cap,
                     "invalid owner type method_idx=%u class_idx=%u proto_idx=%u name_idx=%u dex=%.160s",
                     method_idx, class_idx, proto_idx, name_idx,
                     m->dex_name[0] ? m->dex_name : "<unknown>");
        return false;
    }
    /* method_id.class_idx is a type reference. Ordinary classes use the JNI
     * slash form, while array owners preserve their descriptor verbatim. */
    if (owner_desc[0] == '[') {
        if (!dex_vm_copy_text(tmp, sizeof(tmp), owner_desc)) {
            if (detail && detail_cap)
                snprintf(detail, detail_cap, "owner descriptor too long method_idx=%u", method_idx);
            return false;
        }
    } else if (!dex_vm_desc_to_class(owner_desc, tmp, sizeof(tmp))) {
        if (detail && detail_cap)
            snprintf(detail, detail_cap, "invalid/oversized owner descriptor method_idx=%u owner=%s",
                     method_idx, owner_desc);
        return false;
    }
    const char *n = dex_vm_dex_string(data, size, name_idx);
    if (!n) {
        if (detail && detail_cap)
            snprintf(detail, detail_cap,
                     "invalid method name method_idx=%u name_idx=%u class_idx=%u proto_idx=%u dex=%.160s",
                     method_idx, name_idx, class_idx, proto_idx,
                     m->dex_name[0] ? m->dex_name : "<unknown>");
        return false;
    }

    size_t sig_needed = 0;
    const char *proto_reason = NULL;
    if (!dex_vm_proto_signature(data, size, proto_idx, sig, sig_cap, &sig_needed, &proto_reason)) {
        if (detail && detail_cap)
            snprintf(detail, detail_cap,
                     "proto decode failed method_idx=%u class_idx=%u proto_idx=%u name_idx=%u "
                     "target=%.140s.%.80s reason=%s needed=%zu cap=%zu dex=%.96s",
                     method_idx, class_idx, proto_idx, name_idx, tmp, n,
                     proto_reason ? proto_reason : "unknown", sig_needed, sig_cap,
                     m->dex_name[0] ? m->dex_name : "<unknown>");
        return false;
    }
    if (!dex_vm_copy_text(cls, cls_cap, tmp)) {
        if (detail && detail_cap) snprintf(detail, detail_cap, "target class buffer too small method_idx=%u", method_idx);
        return false;
    }
    if (!dex_vm_copy_text(name, name_cap, n)) {
        if (detail && detail_cap) snprintf(detail, detail_cap, "target method-name buffer too small method_idx=%u", method_idx);
        return false;
    }
    return true;
}

/* const-string yields the interned literal (JLS 3.10.5): equal literals are
 * one String instance across all classes, so reference comparisons and
 * identity-keyed maps behave as on ART. */
extern void *ssrnx_java_lang_intern_literal(void *env, const char *mutf8) __attribute__((weak));
static void *dex_vm_dex_literal_string(void *env, const char *mutf8) {
    void *ref = ssrnx_java_lang_intern_literal ? ssrnx_java_lang_intern_literal(env, mutf8) : NULL;
    return ref ? ref : ssrnx_jni_new_local_string_utf(env, mutf8);
}

static bool dex_vm_type_ref(const SsrnxDexMethodImage *m, uint32_t type_idx, char *out, size_t cap) {
    return dex_vm_desc_to_class(dex_vm_dex_type_desc(m->dex_data, m->dex_size, type_idx), out, cap);
}

/* DEX check-cast/instance-of accept any reference type, including arrays.
 * JNI class names use slash names for ordinary classes but preserve array
 * descriptors verbatim (for example [Ljava/lang/String; and [I). */
static bool dex_vm_reference_type_ref(const SsrnxDexMethodImage *m, uint32_t type_idx,
                               char *out, size_t cap) {
    if (!m || !out || cap == 0) return false;
    const char *desc = dex_vm_dex_type_desc(m->dex_data, m->dex_size, type_idx);
    if (!desc) return false;
    if (desc[0] == '[') return dex_vm_copy_text(out, cap, desc);
    return dex_vm_desc_to_class(desc, out, cap);
}

/* Resolve a pending Java exception through one DEX code_item's try/catch table.
 * handler_off is byte-relative to the encoded_catch_handler_list start, exactly
 * as specified by the DEX format. The pending JNI exception is only cleared
 * after a typed/catch-all handler has been selected; otherwise it remains
 * pending so an interpreted caller can catch it. */
static bool dex_vm_dex_find_exception_handler(const SsrnxDexMethodImage *m, uint32_t throw_pc,
                                       void *exception_ref, uint32_t *out_handler_pc,
                                       char *out_catch_class, size_t catch_cap) {
    if (!m || !m->dex_data || !m->code_offset || !exception_ref || !out_handler_pc) return false;
    if ((uint64_t)m->code_offset + 16u > m->dex_size) return false;
    const uint8_t *code = m->dex_data + m->code_offset;
    const uint16_t tries_size = dex_vm_rd16(code + 6u);
    if (!tries_size) return false;
    const uint32_t insns_size = dex_vm_rd32(code + 12u);
    size_t tries_off = (size_t)m->code_offset + 16u + (size_t)insns_size * 2u;
    if (insns_size & 1u) tries_off += 2u;
    if (tries_off > m->dex_size || (uint64_t)tries_off + (uint64_t)tries_size * 8u > m->dex_size)
        return false;
    const size_t handlers_base = tries_off + (size_t)tries_size * 8u;
    if (handlers_base >= m->dex_size) return false;

    for (uint16_t i = 0; i < tries_size; ++i) {
        const uint8_t *item = m->dex_data + tries_off + (size_t)i * 8u;
        const uint32_t start = dex_vm_rd32(item);
        const uint16_t count = dex_vm_rd16(item + 4u);
        if (throw_pc < start || throw_pc >= start + (uint32_t)count) continue;
        const uint16_t handler_off = dex_vm_rd16(item + 6u);
        size_t hp = handlers_base + (size_t)handler_off;
        if (hp >= m->dex_size) return false;
        int32_t encoded_count = 0;
        if (!dex_vm_dex_read_sleb_value(m->dex_data, m->dex_size, &hp, &encoded_count)) return false;
        const uint32_t typed_count = encoded_count < 0 ? (uint32_t)(-encoded_count) : (uint32_t)encoded_count;
        for (uint32_t j = 0; j < typed_count; ++j) {
            uint32_t type_idx = 0, addr = 0;
            if (!dex_vm_dex_read_uleb_value(m->dex_data, m->dex_size, &hp, &type_idx) ||
                !dex_vm_dex_read_uleb_value(m->dex_data, m->dex_size, &hp, &addr)) return false;
            char catch_class[512];
            if (!dex_vm_type_ref(m, type_idx, catch_class, sizeof(catch_class))) return false;
            if (ssrnx_jni_is_instance_of_class_host(exception_ref, catch_class)) {
                if (addr >= m->insns_size) return false;
                *out_handler_pc = addr;
                if (out_catch_class && catch_cap) (void)dex_vm_copy_text(out_catch_class, catch_cap, catch_class);
                return true;
            }
        }
        if (encoded_count <= 0) {
            uint32_t addr = 0;
            if (!dex_vm_dex_read_uleb_value(m->dex_data, m->dex_size, &hp, &addr) || addr >= m->insns_size)
                return false;
            *out_handler_pc = addr;
            if (out_catch_class && catch_cap) (void)dex_vm_copy_text(out_catch_class, catch_cap, "<catch-all>");
            return true;
        }
        return false;
    }
    return false;
}

/* Exceptions are ordinary Java control flow (coroutines cancel flows with
 * AbortFlowException, retry loops rethrow ...). Log the first few throws and
 * propagations per exception class, then every 256th. */
static bool dex_vm_dex_exception_log_slot(const char *exception_class) {
    static struct { uint64_t hash; uint32_t count; } slots[128];
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    uint64_t hash = UINT64_C(1469598103934665603);
    for (const unsigned char *p = (const unsigned char *)(exception_class ? exception_class : ""); *p; ++p)
        hash = (hash ^ *p) * UINT64_C(1099511628211);
    if (!hash) hash = 1;
    uint32_t count = 0;
    pthread_mutex_lock(&lock);
    for (size_t i = 0; i < sizeof(slots) / sizeof(slots[0]); ++i) {
        const size_t at = (size_t)((hash + i) % (sizeof(slots) / sizeof(slots[0])));
        if (slots[at].hash == hash || !slots[at].hash) {
            slots[at].hash = hash;
            count = ++slots[at].count;
            break;
        }
    }
    pthread_mutex_unlock(&lock);
    return count == 0 || count <= 8u || (count & 255u) == 0;
}

static bool dex_vm_dex_route_pending_exception(dex_vm_DexExecCtx *ctx, const SsrnxDexMethodImage *m,
                                        uint32_t throw_pc, void **out_exception_ref,
                                        uint32_t *out_handler_pc) {
    if (!ctx || !m || !out_exception_ref || !out_handler_pc ||
        !ssrnx_jni_exception_pending_host(ctx->env)) return false;
    void *exception_ref = ssrnx_jni_exception_occurred_host(ctx->env);
    if (!exception_ref) return false;
    char catch_class[512] = {0};
    uint32_t handler_pc = 0;
    if (!dex_vm_dex_find_exception_handler(m, throw_pc, exception_ref, &handler_pc,
                                    catch_class, sizeof(catch_class))) {
        ssrnx_jni_delete_local_ref_host(ctx->env, exception_ref);
        return false;
    }
    char actual_class[512] = {0};
    (void)ssrnx_jni_object_class_name_host(exception_ref, actual_class, sizeof(actual_class));
    ssrnx_jni_exception_clear_host(ctx->env);
    char origin[1024] = {0};
    uint32_t origin_pc = throw_pc;
    if (ctx->report && ctx->report->class_name[0]) {
        snprintf(origin, sizeof(origin), "%.400s.%.200s%.400s", ctx->report->class_name,
                 ctx->report->method_name, ctx->report->signature);
        origin_pc = ctx->report->pc;
    } else {
        snprintf(origin, sizeof(origin), "%.400s.%.200s%.400s", m->declaring_class,
                 m->method_name, m->signature);
    }
    /* Inner frames recorded this throwable as propagating; a handler here ends
     * that propagation, so the report must not keep describing it. Otherwise a
     * later genuinely uncaught throwable is reported under a stale cause. */
    if (ctx->report) {
        ctx->report->pc = 0;
        ctx->report->opcode = 0;
        ctx->report->class_name[0] = '\0';
        ctx->report->method_name[0] = '\0';
        ctx->report->signature[0] = '\0';
        ctx->report->detail[0] = '\0';
    }
    *out_exception_ref = exception_ref;
    *out_handler_pc = handler_pc;
    if (dex_vm_dex_exception_log_slot(actual_class))
        SSRNX_LOGCI("DEX", "exception caught class=%s catch=%s throwPc=%#x handlerPc=%#x method=%s%s caller=%s.%s%s origin=%s originPc=%#x",
                    actual_class[0] ? actual_class : "<unknown>", catch_class[0] ? catch_class : "<unknown>",
                    throw_pc, handler_pc, m->method_name, m->signature,
                    m->declaring_class, m->method_name, m->signature, origin, origin_pc);
    return true;
}

/* A callee left a Java exception pending and this frame has no handler, so it
 * propagates. The innermost propagating frame records which throwable escaped
 * from which call (outer frames keep that report), so an uncaught exception is
 * never reported as an anonymous failure. The exception stays pending. */
static void dex_vm_dex_report_uncaught_propagation(dex_vm_DexExecCtx *ctx, const SsrnxDexMethodImage *m,
                                            uint32_t pc, uint8_t op, const char *origin) {
    if (!ctx || !m || !ctx->report || ctx->report->detail[0]) return;
    char ex_class[256] = {0}, ex_msg[256] = {0};
    void *ex = ssrnx_jni_exception_occurred_host(ctx->env);
    if (ex) {
        (void)ssrnx_jni_object_class_name_host(ex, ex_class, sizeof(ex_class));
        (void)ssrnx_jni_exception_message_host(ex, ex_msg, sizeof(ex_msg));
        ssrnx_jni_delete_local_ref_host(ctx->env, ex);
    }
    char detail[768];
    snprintf(detail, sizeof(detail), "uncaught %.200s: %.200s from %.300s",
             ex_class[0] ? ex_class : "<unknown>", ex_msg, origin ? origin : "?");
    dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, pc, op, detail);
    /* Leaving one interpreted call is not yet a failure: an outer frame may
     * catch it. Escapes that end a callback are reported as warnings there. */
    if (dex_vm_dex_exception_log_slot(ex_class))
        SSRNX_LOGCI("DEX", "exception propagating out of %s.%s%s pc=%#x: %s",
                    m->declaring_class, m->method_name, m->signature, pc, detail);
}

static bool field_ref(const SsrnxDexMethodImage *m, uint32_t field_idx,
                      char *cls, size_t cls_cap, char *name, size_t name_cap,
                      char *sig, size_t sig_cap) {
    const uint8_t *data = m->dex_data;
    const size_t size = m->dex_size;
    if (!data || size < 0x70u) return false;
    const uint32_t count = dex_vm_rd32(data + 0x50);
    const uint32_t off = dex_vm_rd32(data + 0x54);
    if (field_idx >= count || (uint64_t)off + (uint64_t)count * 8u > size) return false;
    const uint8_t *item = data + off + (size_t)field_idx * 8u;
    char tmp[512];
    if (!dex_vm_desc_to_class(dex_vm_dex_type_desc(data, size, dex_vm_rd16(item)), tmp, sizeof(tmp))) return false;
    const char *type = dex_vm_dex_type_desc(data, size, dex_vm_rd16(item + 2u));
    const char *field_name = dex_vm_dex_string(data, size, dex_vm_rd32(item + 4u));
    if (!type || !field_name) return false;
    return dex_vm_copy_text(cls, cls_cap, tmp) && dex_vm_copy_text(name, name_cap, field_name) &&
           dex_vm_copy_text(sig, sig_cap, type);
}

#define DEX_PREFLIGHT_MAX_METHODS 256u
#define DEX_PREFLIGHT_MAX_UNRESOLVED 256u

typedef struct dex_vm_DexPreflightMethodKey {
    char class_name[512];
    char method_name[256];
    char signature[SSRNX_DEX_SIGNATURE_CAP];
    bool is_static;
} dex_vm_DexPreflightMethodKey;

typedef struct dex_vm_DexPreflightMissingKey {
    char class_name[512];
    char method_name[256];
    char signature[SSRNX_DEX_SIGNATURE_CAP];
} dex_vm_DexPreflightMissingKey;

static bool dex_vm_dex_preflight_same_method(const dex_vm_DexPreflightMethodKey *a,
                                      const char *cls, const char *name,
                                      const char *sig, bool is_static) {
    return a && a->is_static == is_static &&
           strcmp(a->class_name, cls) == 0 && strcmp(a->method_name, name) == 0 &&
           strcmp(a->signature, sig) == 0;
}

static bool dex_vm_dex_preflight_queue_add(dex_vm_DexPreflightMethodKey *queue, size_t *queue_count,
                                    const char *cls, const char *name,
                                    const char *sig, bool is_static,
                                    SsrnxDexPreflightReport *report) {
    if (!queue || !queue_count || !cls || !name || !sig || !report) return false;
    for (size_t i = 0; i < *queue_count; ++i)
        if (dex_vm_dex_preflight_same_method(&queue[i], cls, name, sig, is_static)) return true;
    if (*queue_count >= DEX_PREFLIGHT_MAX_METHODS) { report->truncated = true; return false; }
    dex_vm_DexPreflightMethodKey *dst = &queue[(*queue_count)++];
    memset(dst, 0, sizeof(*dst));
    if (!dex_vm_copy_text(dst->class_name, sizeof(dst->class_name), cls) ||
        !dex_vm_copy_text(dst->method_name, sizeof(dst->method_name), name) ||
        !dex_vm_copy_text(dst->signature, sizeof(dst->signature), sig)) {
        (*queue_count)--;
        report->truncated = true;
        return false;
    }
    dst->is_static = is_static;
    return true;
}

static bool dex_vm_dex_preflight_missing_add(dex_vm_DexPreflightMissingKey *missing, size_t *missing_count,
                                      const char *cls, const char *name, const char *sig,
                                      SsrnxDexPreflightReport *report) {
    if (!missing || !missing_count || !cls || !name || !sig || !report) return false;
    for (size_t i = 0; i < *missing_count; ++i) {
        if (strcmp(missing[i].class_name, cls) == 0 &&
            strcmp(missing[i].method_name, name) == 0 &&
            strcmp(missing[i].signature, sig) == 0) return false;
    }
    if (*missing_count >= DEX_PREFLIGHT_MAX_UNRESOLVED) { report->truncated = true; return false; }
    dex_vm_DexPreflightMissingKey *dst = &missing[(*missing_count)++];
    memset(dst, 0, sizeof(*dst));
    if (!dex_vm_copy_text(dst->class_name, sizeof(dst->class_name), cls) ||
        !dex_vm_copy_text(dst->method_name, sizeof(dst->method_name), name) ||
        !dex_vm_copy_text(dst->signature, sizeof(dst->signature), sig)) {
        (*missing_count)--;
        report->truncated = true;
        return false;
    }
    return true;
}

static uint32_t dex_vm_dex_preflight_width(const SsrnxDexMethodImage *m, uint32_t pc, bool *out_valid) {
    if (out_valid) *out_valid = false;
    if (!m || !m->insns || pc >= m->insns_size) return 0;
    const uint16_t word = m->insns[pc];
    const uint8_t op = (uint8_t)(word & 0xffu);
    uint64_t width = 1;
    if (op == 0x00u) {
        const uint8_t ident = (uint8_t)(word >> 8);
        if (ident == 0x01u) { /* packed-switch-payload */
            if ((uint64_t)pc + 1u >= m->insns_size) return 0;
            width = 4u + (uint64_t)m->insns[pc + 1u] * 2u;
        } else if (ident == 0x02u) { /* sparse-switch-payload */
            if ((uint64_t)pc + 1u >= m->insns_size) return 0;
            width = 2u + (uint64_t)m->insns[pc + 1u] * 4u;
        } else if (ident == 0x03u) { /* fill-array-data-payload */
            if ((uint64_t)pc + 3u >= m->insns_size) return 0;
            const uint64_t elem = m->insns[pc + 1u];
            const uint64_t count = (uint64_t)m->insns[pc + 2u] |
                                   ((uint64_t)m->insns[pc + 3u] << 16);
            if (elem && count > (UINT64_MAX - 1u) / elem) return 0;
            width = 4u + ((elem * count + 1u) / 2u);
        }
    } else if (op == 0x02u || op == 0x05u || op == 0x08u || op == 0x13u ||
               op == 0x15u || op == 0x16u || op == 0x19u || op == 0x1au ||
               op == 0x1cu || op == 0x1fu || op == 0x20u || op == 0x22u ||
               op == 0x23u || op == 0x29u || (op >= 0x2du && op <= 0x3du) ||
               (op >= 0x44u && op <= 0x6du) || (op >= 0x90u && op <= 0xafu) ||
               (op >= 0xd0u && op <= 0xe2u) || op == 0xfeu || op == 0xffu) {
        width = 2u;
    } else if (op == 0x03u || op == 0x06u || op == 0x09u || op == 0x14u ||
               op == 0x17u || op == 0x1bu || op == 0x24u || op == 0x25u ||
               op == 0x26u || op == 0x2au || op == 0x2bu || op == 0x2cu ||
               (op >= 0x6eu && op <= 0x72u) || (op >= 0x74u && op <= 0x78u) ||
               op == 0xfcu || op == 0xfdu) {
        width = 3u;
    } else if (op == 0xfau || op == 0xfbu) {
        width = 4u;
    } else if (op == 0x18u) {
        width = 5u;
    }
    if (width == 0 || (uint64_t)pc + width > m->insns_size) return 0;
    if (out_valid) *out_valid = true;
    return (uint32_t)width;
}

/* Keep the static preflight aligned with the actual interpreter switch below.
 * This is diagnostic only: an unsupported opcode is reported, never skipped or
 * emulated during real execution. */
static bool dex_vm_dex_opcode_runtime_supported(uint8_t op) {
    return op <= 0x3du ||
           (op >= 0x44u && op <= 0x72u) ||
           (op >= 0x74u && op <= 0x78u) ||
           (op >= 0x7bu && op <= 0xe2u);
}


static SsrnxResult dex_vm_dex_preflight_queue_declaring_clinit(const char *source_path,
                                                        const char *class_name,
                                                        dex_vm_DexPreflightMethodKey *queue,
                                                        size_t *queue_count,
                                                        SsrnxDexPreflightReport *report) {
    if (!source_path || !class_name || !queue || !queue_count || !report)
        return SSRNX_ERR_ARGUMENT;
    SsrnxDexMethodImage clinit = {0};
    bool class_found = false, method_found = false;
    SsrnxResult rc = ssrnx_package_load_dex_method(source_path, class_name, "<clinit>", "()V",
                                                    &clinit, &class_found, &method_found);
    if (rc != SSRNX_OK) return rc;
    if (method_found && !(clinit.access_flags & DEX_ACC_NATIVE) && clinit.insns && clinit.insns_size)
        (void)dex_vm_dex_preflight_queue_add(queue, queue_count, clinit.declaring_class,
                                      "<clinit>", "()V", true, report);
    ssrnx_package_release_dex_method(&clinit);
    return SSRNX_OK;
}

static SsrnxResult dex_vm_dex_preflight_resolve_chain(const char *source_path,
                                               const char *class_name,
                                               const char *method_name,
                                               const char *signature,
                                               bool is_static,
                                               bool *out_resolved,
                                               bool *out_package_body,
                                               bool *out_native_missing,
                                               char *out_declaring,
                                               size_t out_declaring_cap) {
    if (!source_path || !class_name || !method_name || !signature || !out_resolved ||
        !out_package_body || !out_native_missing || !out_declaring || !out_declaring_cap)
        return SSRNX_ERR_ARGUMENT;
    *out_resolved = false;
    *out_package_body = false;
    *out_native_missing = false;
    out_declaring[0] = '\0';
    char current[512];
    if (!dex_vm_copy_text(current, sizeof(current), class_name)) return SSRNX_ERR_ARGUMENT;
    for (unsigned walk = 0; walk < DEX_MAX_DEPTH; ++walk) {
        SsrnxDexMethodImage method = {0};
        bool class_found = false, method_found = false;
        SsrnxResult rc = ssrnx_package_load_dex_method(source_path, current, method_name, signature,
                                                        &method, &class_found, &method_found);
        if (rc != SSRNX_OK) return rc;
        if (method_found) {
            *out_resolved = true;
            (void)dex_vm_copy_text(out_declaring, out_declaring_cap, method.declaring_class);
            if (method.access_flags & DEX_ACC_NATIVE) {
                bool native_found = false;
                rc = ssrnx_dex_prewarm_native_method(method.declaring_class, method_name,
                                                      signature, &native_found);
                if (rc != SSRNX_OK) { ssrnx_package_release_dex_method(&method); return rc; }
                if (!native_found) { *out_resolved = false; *out_native_missing = true; }
            } else if (method.insns && method.insns_size) {
                *out_package_body = true;
            }
            ssrnx_package_release_dex_method(&method);
            return SSRNX_OK;
        }
        if (!class_found) {
            if (!is_static && strcmp(current, "java/lang/Object") == 0 &&
                strcmp(method_name, "<init>") == 0 && strcmp(signature, "()V") == 0) {
                *out_resolved = true;
                (void)dex_vm_copy_text(out_declaring, out_declaring_cap, current);
                return SSRNX_OK;
            }
            bool host_found = false;
            rc = ssrnx_jni_has_method_host(current, method_name, signature, is_static, &host_found);
            if (rc != SSRNX_OK) return rc;
            *out_resolved = host_found;
            if (host_found) (void)dex_vm_copy_text(out_declaring, out_declaring_cap, current);
            return SSRNX_OK;
        }
        if (!method.super_class[0]) { ssrnx_package_release_dex_method(&method); return SSRNX_OK; }
        char next[512];
        (void)dex_vm_copy_text(next, sizeof(next), method.super_class);
        ssrnx_package_release_dex_method(&method);
        (void)dex_vm_copy_text(current, sizeof(current), next);
    }
    return SSRNX_OK;
}

/* JVMS 5.4.3.3 step 3: when no class in the superclass chain declares the
 * method, it resolves to a superinterface method; a default (Java 8) method
 * supplies the body. Breadth-first over the superclasses' interfaces, bounded. */
#define DEX_PREFLIGHT_IFACE_MAX 48u
static SsrnxResult dex_vm_dex_preflight_resolve_interfaces(const char *source_path,
                                                    const char *class_name,
                                                    const char *method_name,
                                                    const char *signature,
                                                    bool *out_resolved,
                                                    bool *out_package_body,
                                                    char *out_declaring,
                                                    size_t out_declaring_cap) {
    char (*queue)[512] = calloc(DEX_PREFLIGHT_IFACE_MAX, sizeof(*queue));
    char *interfaces = malloc(4096);
    if (!queue || !interfaces) { free(queue); free(interfaces); return SSRNX_ERR_NO_MEMORY; }
    size_t count = 0;
    (void)dex_vm_copy_text(queue[count++], sizeof(queue[0]), class_name);
    SsrnxResult rc = SSRNX_OK;
    for (size_t at = 0; at < count && !*out_resolved; ++at) {
        const char *current = queue[at];
        char super_name[512] = {0};
        bool found = false;
        interfaces[0] = '\0';
        rc = ssrnx_package_find_dex_class_ex(source_path, current, super_name, sizeof(super_name),
                                             interfaces, 4096, &found);
        if (rc != SSRNX_OK) break;
        if (!found) {
            /* Platform interface: its model answers for (default) members. */
            if (at > 0) {
                bool host_found = false;
                rc = ssrnx_jni_has_method_host(current, method_name, signature, false, &host_found);
                if (rc != SSRNX_OK) break;
                if (host_found) {
                    *out_resolved = true;
                    (void)dex_vm_copy_text(out_declaring, out_declaring_cap, current);
                }
            }
            continue;
        }
        if (at > 0) {
            SsrnxDexMethodImage method = {0};
            bool class_found = false, method_found = false;
            rc = ssrnx_package_load_dex_method(source_path, current, method_name, signature,
                                               &method, &class_found, &method_found);
            if (rc != SSRNX_OK) break;
            if (method_found && strcmp(method.declaring_class, current) == 0) {
                *out_resolved = true;
                *out_package_body = method.insns && method.insns_size;
                (void)dex_vm_copy_text(out_declaring, out_declaring_cap, current);
            }
            ssrnx_package_release_dex_method(&method);
            if (*out_resolved) break;
        }
        if (super_name[0] && count < DEX_PREFLIGHT_IFACE_MAX)
            (void)dex_vm_copy_text(queue[count++], sizeof(queue[0]), super_name);
        for (char *name = interfaces; *name && count < DEX_PREFLIGHT_IFACE_MAX;) {
            char *end = strchr(name, '\n');
            if (end) *end = '\0';
            bool seen = false;
            for (size_t i = 0; i < count && !seen; ++i) seen = strcmp(queue[i], name) == 0;
            if (!seen && name[0]) (void)dex_vm_copy_text(queue[count++], sizeof(queue[0]), name);
            if (!end) break;
            name = end + 1;
        }
    }
    free(queue);
    free(interfaces);
    return rc;
}

static SsrnxResult dex_vm_dex_preflight_resolve_target(const char *source_path,
                                                const char *class_name,
                                                const char *method_name,
                                                const char *signature,
                                                bool is_static,
                                                bool *out_resolved,
                                                bool *out_package_body,
                                                bool *out_native_missing,
                                                char *out_declaring,
                                                size_t out_declaring_cap) {
    SsrnxResult rc = dex_vm_dex_preflight_resolve_chain(source_path, class_name, method_name, signature,
                                                 is_static, out_resolved, out_package_body,
                                                 out_native_missing, out_declaring, out_declaring_cap);
    if (rc != SSRNX_OK || *out_resolved || *out_native_missing || is_static ||
        strcmp(method_name, "<init>") == 0)
        return rc;
    return dex_vm_dex_preflight_resolve_interfaces(source_path, class_name, method_name, signature,
                                            out_resolved, out_package_body, out_declaring,
                                            out_declaring_cap);
}

SsrnxResult ssrnx_dex_preflight_method_tree(const char *source_path,
                                             const char *class_name,
                                             const char *method_name,
                                             const char *signature,
                                             bool is_static,
                                             SsrnxDexPreflightReport *out_report) {
    if (!source_path || !class_name || !method_name || !signature || !out_report)
        return SSRNX_ERR_ARGUMENT;
    memset(out_report, 0, sizeof(*out_report));
    dex_vm_DexPreflightMethodKey *queue = calloc(DEX_PREFLIGHT_MAX_METHODS, sizeof(*queue));
    dex_vm_DexPreflightMissingKey *missing = calloc(DEX_PREFLIGHT_MAX_UNRESOLVED, sizeof(*missing));
    if (!queue || !missing) { free(queue); free(missing); return SSRNX_ERR_NO_MEMORY; }
    size_t queue_count = 0, cursor = 0, missing_count = 0;
    (void)dex_vm_dex_preflight_queue_add(queue, &queue_count, class_name, method_name, signature,
                                  is_static, out_report);

    while (cursor < queue_count) {
        const dex_vm_DexPreflightMethodKey key = queue[cursor++];
        bool resolved = false, package_body = false, native_missing = false;
        char declaring[512] = {0};
        SsrnxResult rc = dex_vm_dex_preflight_resolve_target(source_path, key.class_name, key.method_name,
                                                       key.signature, key.is_static, &resolved,
                                                       &package_body, &native_missing, declaring,
                                                       sizeof(declaring));
        if (rc != SSRNX_OK) { free(queue); free(missing); return rc; }
        if (!resolved || !package_body) {
            if (!resolved && dex_vm_dex_preflight_missing_add(missing, &missing_count, key.class_name,
                                                       key.method_name, key.signature, out_report)) {
                out_report->unresolved_methods++;
                if (native_missing) {
                    out_report->unresolved_native_methods++;
                    SSRNX_LOGCI("DEX-PREFLIGHT", "native method not bound yet class=%s method=%s sig=%s (System.loadLibrary/RegisterNatives may bind it before the call)",
                                key.class_name, key.method_name, key.signature);
                } else {
                    SSRNX_LOGCW("DEX-PREFLIGHT", "unresolved method class=%s method=%s sig=%s",
                                key.class_name, key.method_name, key.signature);
                }
            }
            continue;
        }

        SsrnxDexMethodImage method = {0};
        bool class_found = false, method_found = false;
        rc = ssrnx_package_load_dex_method(source_path, declaring, key.method_name, key.signature,
                                           &method, &class_found, &method_found);
        if (rc != SSRNX_OK) { free(queue); free(missing); return rc; }
        if (!method_found || !method.insns) { ssrnx_package_release_dex_method(&method); continue; }
        if (strcmp(method.method_name, "<clinit>") != 0) {
            rc = dex_vm_dex_preflight_queue_declaring_clinit(source_path, method.declaring_class, queue,
                                                       &queue_count, out_report);
            if (rc != SSRNX_OK) { ssrnx_package_release_dex_method(&method); free(queue); free(missing); return rc; }
        }
        out_report->methods_scanned++;
        for (uint32_t pc = 0; pc < method.insns_size; ) {
            bool valid = false;
            const uint32_t width = dex_vm_dex_preflight_width(&method, pc, &valid);
            if (!valid || !width) {
                out_report->truncated = true;
                SSRNX_LOGCW("DEX-PREFLIGHT", "decode stopped class=%s method=%s sig=%s pc=%#x",
                            method.declaring_class, method.method_name, method.signature, pc);
                break;
            }
            const uint8_t op = (uint8_t)(method.insns[pc] & 0xffu);
            if (!dex_vm_dex_opcode_runtime_supported(op)) {
                out_report->unsupported_opcode_sites++;
                SSRNX_LOGCW("DEX-PREFLIGHT",
                            "unsupported opcode class=%s method=%s sig=%s pc=%#x opcode=0x%02x",
                            method.declaring_class, method.method_name, method.signature, pc, op);
            }
            if (op >= 0x52u && op <= 0x6du) {
                /* Package-class fields materialize lazily at runtime; framework
                 * fields must already be modeled, so report missing ones here. */
                char field_class[512], field_name[256], field_sig[512], field_super[512];
                bool field_package = false;
                if (field_ref(&method, method.insns[pc + 1u], field_class, sizeof(field_class),
                              field_name, sizeof(field_name), field_sig, sizeof(field_sig)) &&
                    ssrnx_package_find_dex_class(source_path, field_class, field_super,
                                                 sizeof(field_super), &field_package) == SSRNX_OK &&
                    !field_package &&
                    !ssrnx_jni_field_exists_host(field_class, field_name, field_sig, op >= 0x60u) &&
                    dex_vm_dex_preflight_missing_add(missing, &missing_count, field_class, field_name,
                                              field_sig, out_report)) {
                    out_report->unresolved_fields++;
                    SSRNX_LOGCW("DEX-PREFLIGHT",
                                "unresolved %s field class=%s name=%s sig=%s referencedBy=%s.%s%s pc=%#x",
                                op >= 0x60u ? "static" : "instance", field_class, field_name, field_sig,
                                method.declaring_class, method.method_name, method.signature, pc);
                }
            }
            if ((op >= 0x6eu && op <= 0x72u) || (op >= 0x74u && op <= 0x78u)) {
                const uint32_t method_idx = method.insns[pc + 1u];
                char target_class[512], target_name[256], target_sig[SSRNX_DEX_SIGNATURE_CAP];
                if (method_ref(&method, method_idx, target_class, sizeof(target_class),
                               target_name, sizeof(target_name), target_sig, sizeof(target_sig),
                               NULL, 0)) {
                    const bool target_static = (op == 0x71u || op == 0x77u);
                    out_report->invoke_refs++;
                    bool target_resolved = false, target_package = false, target_native_missing = false;
                    char target_declaring[512] = {0};
                    rc = dex_vm_dex_preflight_resolve_target(source_path, target_class, target_name, target_sig,
                                                       target_static, &target_resolved, &target_package,
                                                       &target_native_missing, target_declaring,
                                                       sizeof(target_declaring));
                    if (rc != SSRNX_OK) { ssrnx_package_release_dex_method(&method); free(queue); free(missing); return rc; }
                    if (target_resolved && target_package) {
                        out_report->package_targets++;
                        (void)dex_vm_dex_preflight_queue_add(queue, &queue_count, target_declaring,
                                                      target_name, target_sig, target_static, out_report);
                    } else if (target_resolved) {
                        out_report->framework_targets++;
                    } else if (dex_vm_dex_preflight_missing_add(missing, &missing_count, target_class,
                                                         target_name, target_sig, out_report)) {
                        out_report->unresolved_methods++;
                        if (target_native_missing) {
                            out_report->unresolved_native_methods++;
                            SSRNX_LOGCI("DEX-PREFLIGHT",
                                        "native method not bound yet class=%s method=%s sig=%s referencedBy=%s.%s%s pc=%#x (System.loadLibrary/RegisterNatives may bind it before the call)",
                                        target_class, target_name, target_sig, method.declaring_class,
                                        method.method_name, method.signature, pc);
                        } else {
                            SSRNX_LOGCW("DEX-PREFLIGHT",
                                        "unresolved method class=%s method=%s sig=%s referencedBy=%s.%s%s pc=%#x opcode=0x%02x",
                                        target_class, target_name, target_sig, method.declaring_class,
                                        method.method_name, method.signature, pc, op);
                        }
                    }
                }
            }
            pc += width;
        }
        ssrnx_package_release_dex_method(&method);
    }
    SSRNX_LOGCI("DEX-PREFLIGHT",
                "summary root=%s.%s%s methods=%zu invokes=%zu package=%zu framework=%zu unresolved=%zu nativeMissing=%zu unsupportedOpcodes=%zu unresolvedFields=%zu truncated=%d",
                class_name, method_name, signature, out_report->methods_scanned,
                out_report->invoke_refs, out_report->package_targets,
                out_report->framework_targets, out_report->unresolved_methods,
                out_report->unresolved_native_methods, out_report->unsupported_opcode_sites,
                out_report->unresolved_fields,
                out_report->truncated ? 1 : 0);
    free(queue);
    free(missing);
    return SSRNX_OK;
}

/* Converts a class_def static_values constant to the JNI slot value of a
 * field with `signature`. A String constant is returned as a new local
 * reference in *out_temp_string (the slot holds its token). *out_mismatch is
 * set when the encoded type does not match the field descriptor. */
static SsrnxResult dex_vm_dex_static_initial_jni_value(void *env, const char *signature,
                                                const SsrnxDexStaticValue *initial,
                                                SsrnxJniValue *out_value, void **out_temp_string,
                                                bool *out_mismatch) {
    SsrnxJniValue value = {0};
    *out_temp_string = NULL;
    *out_mismatch = false;
    switch (initial->type) {
        case SSRNX_DEX_STATIC_DEFAULT:
        case SSRNX_DEX_STATIC_NULL:
            break;
        case SSRNX_DEX_STATIC_BYTE:
            if (signature[0] != 'B') { *out_mismatch = true; return SSRNX_OK; }
            value.b = (int8_t)initial->bits;
            break;
        case SSRNX_DEX_STATIC_SHORT:
            if (signature[0] != 'S') { *out_mismatch = true; return SSRNX_OK; }
            value.s = (int16_t)initial->bits;
            break;
        case SSRNX_DEX_STATIC_CHAR:
            if (signature[0] != 'C') { *out_mismatch = true; return SSRNX_OK; }
            value.c = (uint16_t)initial->bits;
            break;
        case SSRNX_DEX_STATIC_INT:
            if (signature[0] != 'I') { *out_mismatch = true; return SSRNX_OK; }
            value.i = (int32_t)initial->bits;
            break;
        case SSRNX_DEX_STATIC_LONG:
            if (signature[0] != 'J') { *out_mismatch = true; return SSRNX_OK; }
            value.j = (int64_t)initial->bits;
            break;
        case SSRNX_DEX_STATIC_FLOAT: {
            if (signature[0] != 'F') { *out_mismatch = true; return SSRNX_OK; }
            const uint32_t bits = (uint32_t)initial->bits;
            memcpy(&value.f, &bits, sizeof(bits));
            break;
        }
        case SSRNX_DEX_STATIC_DOUBLE: {
            if (signature[0] != 'D') { *out_mismatch = true; return SSRNX_OK; }
            const uint64_t bits = initial->bits;
            memcpy(&value.d, &bits, sizeof(bits));
            break;
        }
        case SSRNX_DEX_STATIC_BOOLEAN:
            if (signature[0] != 'Z') { *out_mismatch = true; return SSRNX_OK; }
            value.z = initial->bits ? 1u : 0u;
            break;
        case SSRNX_DEX_STATIC_STRING: {
            if (strcmp(signature, "Ljava/lang/String;") != 0) { *out_mismatch = true; return SSRNX_OK; }
            void *temp = ssrnx_jni_new_local_string_utf(env, initial->string_value);
            if (!temp) return SSRNX_ERR_NO_MEMORY;
            value.l = ssrnx_jni_object_token_host(temp);
            if (!value.l) {
                ssrnx_jni_delete_local_ref_host(env, temp);
                return SSRNX_ERR_APK_UNSUPPORTED;
            }
            *out_temp_string = temp;
            break;
        }
        case SSRNX_DEX_STATIC_UNSUPPORTED:
        default:
            *out_mismatch = true;
            return SSRNX_OK;
    }
    *out_value = value;
    return SSRNX_OK;
}

/* JVMS 5.4.3.2 field resolution over package DEX class_defs: the named class
 * declares the field, else (static fields) a superinterface does, recursively,
 * else the superclass chain. out receives the declaring class. False when the
 * chain leaves the package DEX (framework-declared fields) or has no match. */
static bool dex_vm_dex_resolve_field_owner(dex_vm_DexExecCtx *ctx, const char *class_name, const char *field_name,
                                    const char *signature, bool is_static, char *out, size_t cap,
                                    unsigned depth) {
    if (depth > 32u || !class_name || !class_name[0]) return false;
    SsrnxDexMember *members = NULL;
    size_t count = 0;
    bool class_found = false;
    if (ssrnx_package_list_dex_class_members(ctx->source_path, class_name, &members, &count,
                                             &class_found) != SSRNX_OK || !class_found)
        return false;
    bool declared = false;
    for (size_t i = 0; i < count && !declared; ++i) {
        declared = !members[i].is_method && members[i].is_static == is_static &&
                   strcmp(members[i].name, field_name) == 0 &&
                   strcmp(members[i].signature, signature) == 0;
    }
    ssrnx_package_free_dex_members(members, count);
    if (declared) return dex_vm_copy_text(out, cap, class_name);

    char super_name[512] = {0};
    char interfaces[2048] = {0};
    bool found = false;
    if (ssrnx_package_find_dex_class_ex(ctx->source_path, class_name, super_name, sizeof(super_name),
                                        interfaces, sizeof(interfaces), &found) != SSRNX_OK || !found)
        return false;
    if (is_static) {
        for (char *iface = interfaces; *iface;) {
            char *end = strchr(iface, '\n');
            if (end) *end = '\0';
            if (dex_vm_dex_resolve_field_owner(ctx, iface, field_name, signature, true, out, cap, depth + 1u))
                return true;
            if (!end) break;
            iface = end + 1;
        }
    }
    return super_name[0] &&
           dex_vm_dex_resolve_field_owner(ctx, super_name, field_name, signature, is_static, out, cap, depth + 1u);
}

/* Successful field resolution is immutable for a loaded class. Keep the full
 * source/member key so hash collisions and package switches cannot alias it. */
#define DEX_STATIC_OWNER_SLOTS 256u
#define DEX_STATIC_OWNER_KEY_CAP 2048u
typedef struct dex_vm_DexStaticOwner {
    uint64_t hash;
    char *key;
    size_t key_len;
    bool dex_declared;
    char owner[512];
} dex_vm_DexStaticOwner;
static dex_vm_DexStaticOwner dex_vm_g_static_owner[DEX_STATIC_OWNER_SLOTS];
static pthread_mutex_t dex_vm_g_static_owner_lock = PTHREAD_MUTEX_INITIALIZER;

static size_t dex_vm_dex_static_owner_key(const char *source, const char *cls, const char *name,
                                    const char *sig, char *out, size_t cap, uint64_t *hash) {
    const char *parts[] = {source, cls, name, sig};
    size_t used = 0;
    uint64_t h = UINT64_C(1469598103934665603);
    for (unsigned k = 0; k < 4u; ++k) {
        size_t len = strlen(parts[k]) + 1u;
        if (len > cap - used) return 0;
        memcpy(out + used, parts[k], len);
        for (size_t i = 0; i < len; ++i) h = (h ^ (uint8_t)parts[k][i]) * UINT64_C(1099511628211);
        used += len;
    }
    *hash = h;
    return used;
}

static bool dex_vm_dex_static_owner_lookup(uint64_t hash, const char *key, size_t key_len,
                                     char *out, size_t cap, bool *dex_declared) {
    if (!key_len) return false;
    SSRNX_PERF_LOCK(&dex_vm_g_static_owner_lock);
    const dex_vm_DexStaticOwner *slot = &dex_vm_g_static_owner[hash % DEX_STATIC_OWNER_SLOTS];
    const bool hit = slot->key && slot->hash == hash && slot->key_len == key_len &&
                     memcmp(slot->key, key, key_len) == 0 && dex_vm_copy_text(out, cap, slot->owner);
    if (hit) *dex_declared = slot->dex_declared;
    pthread_mutex_unlock(&dex_vm_g_static_owner_lock);
    return hit;
}

static void dex_vm_dex_static_owner_store(uint64_t hash, const char *key, size_t key_len,
                                    const char *owner, bool dex_declared) {
    if (!key_len || strlen(owner) >= sizeof(dex_vm_g_static_owner[0].owner)) return;
    char *copy = malloc(key_len);
    if (!copy) return;
    memcpy(copy, key, key_len);
    SSRNX_PERF_LOCK(&dex_vm_g_static_owner_lock);
    dex_vm_DexStaticOwner *slot = &dex_vm_g_static_owner[hash % DEX_STATIC_OWNER_SLOTS];
    free(slot->key);
    slot->key = copy; slot->key_len = key_len; slot->hash = hash;
    slot->dex_declared = dex_declared;
    (void)dex_vm_copy_text(slot->owner, sizeof(slot->owner), owner);
    pthread_mutex_unlock(&dex_vm_g_static_owner_lock);
}

static void dex_vm_dex_static_owner_reset(void) {
    SSRNX_PERF_LOCK(&dex_vm_g_static_owner_lock);
    for (size_t i = 0; i < DEX_STATIC_OWNER_SLOTS; ++i) free(dex_vm_g_static_owner[i].key);
    memset(dex_vm_g_static_owner, 0, sizeof(dex_vm_g_static_owner));
    pthread_mutex_unlock(&dex_vm_g_static_owner_lock);
}

/* class_name is updated in place to the field's declaring class when it is
 * reached through another type, so the caller's get/put use the real slot. */
static SsrnxResult dex_vm_ensure_dex_static_field(dex_vm_DexExecCtx *ctx, char *class_name, size_t class_cap,
                                           const char *field_name, const char *signature,
                                           unsigned depth, bool *out_available) {
    if (!ctx || !class_name || !field_name || !signature || !out_available)
        return SSRNX_ERR_ARGUMENT;
    *out_available = false;

    uint64_t owner_hash = 0;
    char key[DEX_STATIC_OWNER_KEY_CAP], owner[512];
    const size_t key_len = dex_vm_dex_static_owner_key(ctx->source_path, class_name, field_name,
                                                signature, key, sizeof(key), &owner_hash);
    bool dex_field_declared = false;
    const bool owner_known = dex_vm_dex_static_owner_lookup(owner_hash, key, key_len, owner,
                                                      sizeof(owner), &dex_field_declared);
    if (owner_known && !dex_vm_copy_text(class_name, class_cap, owner)) return SSRNX_ERR_ARGUMENT;

    SsrnxResult rc = SSRNX_OK;
    bool dex_class_found = false;
    if (!owner_known) {
        char super_name[512] = {0};
        rc = ssrnx_package_find_dex_class(ctx->source_path, class_name, super_name, sizeof(super_name),
                                          &dex_class_found);
        if (rc != SSRNX_OK) return rc;
    }

    /* Resolve the declaring DEX class before initialization. Active use of
     * Sub.FIELD must initialize the class/interface that actually declares
     * FIELD, not Sub merely because the bytecode's field_id names it (JLS
     * 12.4.1 / JVMS 5.4.3.2). If resolution leaves the DEX hierarchy, the
     * field is framework/host-owned and no DEX class is initialized here. */
    if (!owner_known && dex_class_found) {
        dex_field_declared = dex_vm_dex_resolve_field_owner(
            ctx, class_name, field_name, signature, true, owner, sizeof(owner), 0u);
        if (dex_field_declared) {
            dex_vm_dex_static_owner_store(owner_hash, key, key_len, owner, true);
            if (!dex_vm_copy_text(class_name, class_cap, owner)) return SSRNX_ERR_ARGUMENT;
        }
    }

    if (dex_field_declared) {
        rc = dex_vm_ensure_dex_class_initialized(ctx, class_name, depth + 1u);
        if (rc != SSRNX_OK) return rc;
    }

    /* Existing framework/JNI fields win. This keeps DEX accesses coherent with
     * native JNI Get/SetStatic*Field calls instead of maintaining a second
     * interpreter-only store. */
    SsrnxJniValue existing = {0};
    bool found = false;
    rc = ssrnx_jni_get_static_field_value(ctx->env, class_name, field_name, signature,
                                          &existing, &found);
    if (rc != SSRNX_OK) return rc;
    if (found) {
        if ((signature[0] == 'L' || signature[0] == '[') && existing.l)
            ssrnx_jni_delete_local_ref_host(ctx->env, existing.l);
        /* An inherited JNI slot cannot stand in for an unregistered hiding
         * declaration in this DEX class (JVMS 5.4.3.2). */
        if (!dex_field_declared || ssrnx_jni_class_declares_member_host(
                class_name, field_name, signature, true, false)) {
            *out_available = true;
            return SSRNX_OK;
        }
    }

    if (!dex_field_declared) {
        char api_owner[512];
        if (ssrnx_api_stub_bind_field(ctx->env, class_name, field_name, signature, true,
                                      api_owner, sizeof(api_owner))) {
            if (strcmp(api_owner, class_name) != 0) {
                dex_vm_dex_static_owner_store(owner_hash, key, key_len, api_owner, false);
                if (!dex_vm_copy_text(class_name, class_cap, api_owner)) return SSRNX_ERR_ARGUMENT;
            }
            *out_available = true;
        }
        return SSRNX_OK;
    }

    /* Force lazy class materialization, then initialize the real Java field slot
     * from class_def.static_values_off. Android applies these encoded constants
     * before any bytecode runs; silently using zero here breaks ordinary static
     * guards/counters and can change control flow long before <clinit> is needed. */
    void *clazz = ssrnx_jni_find_class_ref(ctx->env, class_name);
    if (!clazz) return SSRNX_ERR_APK_UNSUPPORTED;
    ssrnx_jni_delete_local_ref_host(ctx->env, clazz);

    SsrnxDexStaticValue initial = {0};
    bool init_class_found = false, init_field_found = false;
    rc = ssrnx_package_get_dex_static_field_initial_value(ctx->source_path, class_name,
                                                           field_name, signature, &initial,
                                                           &init_class_found, &init_field_found);
    if (rc != SSRNX_OK) return rc;
    SsrnxJniValue value = {0};
    void *temp_string = NULL;
    bool mismatch = false;
    if (init_field_found) {
        rc = dex_vm_dex_static_initial_jni_value(ctx->env, signature, &initial, &value, &temp_string,
                                          &mismatch);
        if (rc != SSRNX_OK) return rc;
        if (mismatch) goto bad_static_init;
    }
    rc = ssrnx_jni_register_field(class_name, field_name, signature, true, value);
    if (temp_string) ssrnx_jni_delete_local_ref_host(ctx->env, temp_string);
    if (rc != SSRNX_OK) return rc;
    SSRNX_LOGCI("DEX", "static field materialized class=%s name=%s sig=%s initType=%d initBits=%#" PRIx64,
                class_name, field_name, signature, (int)initial.type, initial.bits);
    *out_available = true;
    return SSRNX_OK;

bad_static_init:
    if (temp_string) ssrnx_jni_delete_local_ref_host(ctx->env, temp_string);
    {
        char detail[768];
        snprintf(detail, sizeof(detail),
                 "unsupported/mismatched DEX static initializer class=%.220s name=%.160s sig=%.220s type=%d",
                 class_name, field_name, signature, (int)initial.type);
        dex_vm_fail_report(ctx, class_name, field_name, signature, 0, 0, detail);
        SSRNX_LOGCW("DEX", "%s", detail);
    }
    return SSRNX_ERR_APK_UNSUPPORTED;
}

static SsrnxResult dex_vm_ensure_dex_instance_field(dex_vm_DexExecCtx *ctx, void *object_ref,
                                             const char *class_name,
                                             const char *field_name, const char *signature,
                                             bool *out_available) {
    if (!ctx || !object_ref || !class_name || !field_name || !signature || !out_available)
        return SSRNX_ERR_ARGUMENT;
    *out_available = false;

    SsrnxJniValue existing = {0};
    bool found = false;
    SsrnxResult rc = ssrnx_jni_get_instance_field_value(ctx->env, object_ref, class_name,
                                                         field_name, signature, &existing, &found);
    if (rc != SSRNX_OK) return rc;
    if (found) {
        if ((signature[0] == 'L' || signature[0] == '[') && existing.l)
            ssrnx_jni_delete_local_ref_host(ctx->env, existing.l);
        *out_available = true;
        return SSRNX_OK;
    }

    char super_name[512] = {0};
    bool dex_class_found = false;
    rc = ssrnx_package_find_dex_class(ctx->source_path, class_name, super_name, sizeof(super_name),
                                      &dex_class_found);
    if (rc != SSRNX_OK) return rc;
    if (!dex_class_found) {
        char api_owner[512];
        *out_available = ssrnx_api_stub_bind_field(ctx->env, class_name, field_name, signature,
                                                   false, api_owner, sizeof(api_owner));
        return SSRNX_OK;
    }

    /* The slot belongs to the declaring class: iput Sub.f and iget Super.f on
     * one object must share it even when Sub is the first to touch f. */
    char owner[512];
    const char *declaring = class_name;
    if (!dex_vm_dex_resolve_field_owner(ctx, class_name, field_name, signature, false, owner, sizeof(owner), 0u)) {
        *out_available = ssrnx_api_stub_bind_field(ctx->env, class_name, field_name, signature,
                                                   false, owner, sizeof(owner));
        return SSRNX_OK;
    }
    declaring = owner;
    void *clazz = ssrnx_jni_find_class_ref(ctx->env, declaring);
    if (!clazz) return SSRNX_ERR_APK_UNSUPPORTED;
    ssrnx_jni_delete_local_ref_host(ctx->env, clazz);
    SsrnxJniValue zero = {0};
    rc = ssrnx_jni_register_field(declaring, field_name, signature, false, zero);
    if (rc != SSRNX_OK) return rc;
    SSRNX_LOGCI("DEX", "instance field materialized class=%s name=%s sig=%s",
                declaring, field_name, signature);
    *out_available = true;
    return SSRNX_OK;
}

static bool dex_vm_dex_int_div(int32_t lhs, int32_t rhs, int32_t *out) {
    if (!out || rhs == 0) return false;
    if (lhs == INT32_MIN && rhs == -1) *out = INT32_MIN;
    else *out = lhs / rhs;
    return true;
}

static bool dex_vm_dex_int_rem(int32_t lhs, int32_t rhs, int32_t *out) {
    if (!out || rhs == 0) return false;
    if (lhs == INT32_MIN && rhs == -1) *out = 0;
    else *out = lhs % rhs;
    return true;
}

static bool dex_vm_dex_long_div(int64_t lhs, int64_t rhs, int64_t *out) {
    if (!out || rhs == 0) return false;
    if (lhs == INT64_MIN && rhs == -1) *out = INT64_MIN;
    else *out = lhs / rhs;
    return true;
}

static bool dex_vm_dex_long_rem(int64_t lhs, int64_t rhs, int64_t *out) {
    if (!out || rhs == 0) return false;
    if (lhs == INT64_MIN && rhs == -1) *out = 0;
    else *out = lhs % rhs;
    return true;
}

static void dex_vm_dex_write_wide(uint64_t *regs, uint16_t reg, uint64_t bits) {
    regs[reg] = bits;
    regs[reg + 1u] = bits >> 32;
}

static float dex_vm_dex_read_float(const uint64_t *regs, uint16_t reg) {
    uint32_t bits = (uint32_t)regs[reg];
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static double dex_vm_dex_read_double(const uint64_t *regs, uint16_t reg) {
    uint64_t bits = regs[reg];
    double value = 0.0;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void dex_vm_dex_write_float(uint64_t *regs, uint16_t reg, float value) {
    uint32_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    regs[reg] = bits;
}

static void dex_vm_dex_write_double(uint64_t *regs, uint16_t reg, double value) {
    uint64_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    dex_vm_dex_write_wide(regs, reg, bits);
}

/* Dalvik registers are 32 bits wide; SSRNX keeps them in 64-bit slots, so a
 * narrow value must stay canonical (sign/zero-extended per its Java type) for
 * 64-bit register compares to be exact. JNI value unions and AArch64 native
 * returns leave the upper bits of narrow results unspecified. */
static uint64_t dex_vm_dex_canonical_result(char type, SsrnxJniValue v) {
    switch (type) {
        case 'Z': return v.z;
        case 'B': return (uint64_t)(int64_t)v.b;
        case 'C': return v.c;
        case 'S': return (uint64_t)(int64_t)v.s;
        case 'I': return (uint64_t)(int64_t)v.i;
        case 'F': { uint32_t bits = 0; memcpy(&bits, &v.f, sizeof(bits)); return bits; }
        default: return (uint64_t)v.j;
    }
}

/* JNI handles may occupy the same address range as Java ints. Track the DEX
 * operand kind instead of guessing from the register's bits. */
static const char *dex_vm_skip_type(const char *p);
static bool dex_vm_dex_init_reference_regs(const SsrnxDexMethodImage *m, bool *refs) {
    size_t reg = (size_t)m->registers_size - m->ins_size;
    if (!(m->access_flags & DEX_ACC_STATIC)) {
        if (reg >= m->registers_size) return false;
        refs[reg++] = true;
    }
    const char *p = m->signature;
    if (*p++ != '(') return false;
    while (*p && *p != ')') {
        if (reg >= m->registers_size) return false;
        refs[reg] = *p == 'L' || *p == '[';
        reg += *p == 'J' || *p == 'D' ? 2u : 1u;
        p = dex_vm_skip_type(p);
        if (!p) return false;
    }
    return *p == ')' && reg == m->registers_size;
}

/* Called only after a successful instruction, whose operands were validated
 * by execute_code. Stores, branches and invokes leave the register kinds alone. */
static void dex_vm_dex_update_reference_regs(bool *refs, const uint16_t *insn) {
    const uint8_t op = (uint8_t)insn[0], hi = (uint8_t)(insn[0] >> 8);
    uint16_t a;
    bool object = false, wide = false;
    if (op >= 0x01 && op <= 0x09) {
        a = op == 0x03 || op == 0x06 || op == 0x09 ? insn[1] :
            op == 0x01 || op == 0x04 || op == 0x07 ? hi & 0x0f : hi;
        object = op >= 0x07;
        wide = op >= 0x04 && op <= 0x06;
    } else if (op >= 0x0a && op <= 0x0d) {
        a = hi; object = op == 0x0c || op == 0x0d; wide = op == 0x0b;
    } else if (op >= 0x12 && op <= 0x1c) {
        a = op == 0x12 ? hi & 0x0f : hi;
        object = op >= 0x1a; wide = op >= 0x16 && op <= 0x19;
    } else if (op >= 0x20 && op <= 0x23) {
        a = op == 0x22 ? hi : hi & 0x0f; object = op >= 0x22;
    } else if (op >= 0x2d && op <= 0x31) {
        a = hi;
    } else if (op >= 0x44 && op <= 0x4a) {
        a = hi; object = op == 0x46; wide = op == 0x45;
    } else if (op >= 0x52 && op <= 0x58) {
        a = hi & 0x0f; object = op == 0x54; wide = op == 0x53;
    } else if (op >= 0x60 && op <= 0x66) {
        a = hi; object = op == 0x62; wide = op == 0x61;
    } else if (op >= 0x7b && op <= 0x8f) {
        a = hi & 0x0f;
        wide = op == 0x7d || op == 0x7e || op == 0x80 || op == 0x81 ||
               op == 0x83 || op == 0x86 || op == 0x88 || op == 0x89 || op == 0x8b;
    } else if (op >= 0x90 && op <= 0xaf) {
        a = hi; wide = (op >= 0x9b && op <= 0xa5) || op >= 0xab;
    } else if (op >= 0xb0 && op <= 0xcf) {
        a = hi & 0x0f; wide = (op >= 0xbb && op <= 0xc5) || op >= 0xcb;
    } else if (op >= 0xd0 && op <= 0xe2) {
        a = op <= 0xd7 ? hi & 0x0f : hi;
    } else {
        return;
    }
    refs[a] = object;
    if (wide) refs[a + 1u] = false;
}

static char dex_vm_dex_return_type(const char *signature) {
    const char *p = signature ? strrchr(signature, ')') : NULL;
    return p && p[1] ? p[1] : 'V';
}

/* Java narrowing of floating-point to integral types (JLS 5.1.3): NaN -> 0,
 * out-of-range values saturate. A plain C cast is undefined there. */
static int32_t dex_vm_dex_java_d2i(double v) {
    if (v != v) return 0;
    if (v >= 2147483647.0) return INT32_MAX;
    if (v <= -2147483648.0) return INT32_MIN;
    return (int32_t)v;
}

static int64_t dex_vm_dex_java_d2l(double v) {
    if (v != v) return 0;
    if (v >= 9223372036854775807.0) return INT64_MAX;
    if (v <= -9223372036854775808.0) return INT64_MIN;
    return (int64_t)v;
}

/* A pending process exit (see ssrnx_jni_request_process_exit) ends every
 * running frame, as the process would be killed on Android. */
static bool dex_vm_dex_process_exiting(dex_vm_DexExecCtx *ctx, const SsrnxDexMethodImage *m, uint32_t pc,
                                uint8_t op) {
    char reason[256];
    if (!ssrnx_jni_process_exit_requested(reason, sizeof(reason))) return false;
    char detail[320];
    snprintf(detail, sizeof(detail), "process terminating: %s", reason);
    dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, pc, op, detail);
    return true;
}

static int32_t dex_vm_dex_payload_i32(const uint16_t *p) {
    return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 16));
}

static void dex_vm_fail_report(dex_vm_DexExecCtx *ctx, const char *cls, const char *name, const char *sig,
                        uint32_t pc, uint8_t opcode, const char *detail) {
    if (!ctx || !ctx->report) return;
    ctx->report->completed = false;
    ctx->report->pc = pc;
    ctx->report->opcode = opcode;
    ctx->report->instructions = ctx->steps;
    ctx->report->calls = ctx->calls;
    if (cls) (void)dex_vm_copy_text(ctx->report->class_name, sizeof(ctx->report->class_name), cls);
    if (name) (void)dex_vm_copy_text(ctx->report->method_name, sizeof(ctx->report->method_name), name);
    if (sig) (void)dex_vm_copy_text(ctx->report->signature, sizeof(ctx->report->signature), sig);
    if (detail) (void)dex_vm_copy_text(ctx->report->detail, sizeof(ctx->report->detail), detail);
}

/* ---------- linkage failures: ART semantics + unresolved-member ledger ----------
 * ART resolves invoke targets lazily and throws NoSuchMethodError (or
 * NoClassDefFoundError when the owning class is absent) at the call site; the
 * guest may catch it. SSRNX follows that contract by default so one run keeps
 * executing and exposes every missing member it reaches, while the ledger
 * preserves first-fault order. strict-first-fault.flag restores stop-at-first. */
#define DEX_LEDGER_CAP 512u
typedef struct dex_vm_DexLedgerEntry {
    char cls[192];
    char name[96];
    char sig[224];
    char caller[320];
    char kind[24];
    uint32_t hits;
} dex_vm_DexLedgerEntry;

static pthread_mutex_t dex_vm_g_ledger_lock = PTHREAD_MUTEX_INITIALIZER;
static dex_vm_DexLedgerEntry dex_vm_g_ledger[DEX_LEDGER_CAP];
static size_t dex_vm_g_ledger_count;
static size_t dex_vm_g_ledger_dropped;
static bool dex_vm_g_linkage_strict;

void ssrnx_dex_set_strict_linkage(bool strict) { dex_vm_g_linkage_strict = strict; }

size_t ssrnx_dex_unresolved_count(void) {
    SSRNX_PERF_LOCK(&dex_vm_g_ledger_lock);
    const size_t n = dex_vm_g_ledger_count;
    pthread_mutex_unlock(&dex_vm_g_ledger_lock);
    return n;
}

/* Returns true the first time a member is recorded. */
static bool dex_vm_dex_ledger_record(const char *kind, const char *cls, const char *name,
                              const char *sig, const char *caller) {
    bool first = false;
    SSRNX_PERF_LOCK(&dex_vm_g_ledger_lock);
    for (size_t i = 0; i < dex_vm_g_ledger_count; ++i) {
        dex_vm_DexLedgerEntry *e = &dex_vm_g_ledger[i];
        if (strcmp(e->cls, cls ? cls : "") == 0 && strcmp(e->name, name ? name : "") == 0 &&
            strcmp(e->sig, sig ? sig : "") == 0) {
            e->hits++;
            pthread_mutex_unlock(&dex_vm_g_ledger_lock);
            return false;
        }
    }
    if (dex_vm_g_ledger_count < DEX_LEDGER_CAP) {
        dex_vm_DexLedgerEntry *e = &dex_vm_g_ledger[dex_vm_g_ledger_count++];
        memset(e, 0, sizeof(*e));
        (void)dex_vm_copy_text(e->cls, sizeof(e->cls), cls ? cls : "");
        (void)dex_vm_copy_text(e->name, sizeof(e->name), name ? name : "");
        (void)dex_vm_copy_text(e->sig, sizeof(e->sig), sig ? sig : "");
        (void)dex_vm_copy_text(e->caller, sizeof(e->caller), caller ? caller : "");
        (void)dex_vm_copy_text(e->kind, sizeof(e->kind), kind ? kind : "");
        e->hits = 1;
        first = true;
    } else {
        dex_vm_g_ledger_dropped++;
    }
    pthread_mutex_unlock(&dex_vm_g_ledger_lock);
    return first;
}

void ssrnx_dex_log_unresolved_summary(void) {
    SSRNX_PERF_LOCK(&dex_vm_g_ledger_lock);
    SSRNX_LOGCW("LEDGER", "unresolved Java members distinct=%zu dropped=%zu policy=%s",
                dex_vm_g_ledger_count, dex_vm_g_ledger_dropped, dex_vm_g_linkage_strict ? "strict-first-fault" : "art-linkage-error");
    for (size_t i = 0; i < dex_vm_g_ledger_count; ++i) {
        const dex_vm_DexLedgerEntry *e = &dex_vm_g_ledger[i];
        SSRNX_LOGCW("LEDGER", "#%zu %s %s->%s%s hits=%u caller=%s", i + 1u, e->kind, e->cls,
                    e->name, e->sig, e->hits, e->caller[0] ? e->caller : "<top>");
    }
    pthread_mutex_unlock(&dex_vm_g_ledger_lock);
}

/* Record, report and (unless strict) raise the ART linkage error for an
 * unresolvable method or field. Always returns failure; the pending throwable
 * lets the invoking frame route it to a guest handler. */
static SsrnxResult dex_vm_dex_linkage_failure(dex_vm_DexExecCtx *ctx, const char *kind, const char *cls,
                                       const char *name, const char *sig, const char *detail) {
    char caller[320] = {0};
    if (ctx && ctx->current)
        snprintf(caller, sizeof(caller), "%.150s.%.60s%.100s", ctx->current->declaring_class,
                 ctx->current->method_name, ctx->current->signature);
    const bool first = dex_vm_dex_ledger_record(kind, cls, name, sig, caller);
    if (first) {
        SSRNX_LOGCW("DEX", "%s caller=%s", detail, caller[0] ? caller : "<top>");
        if (ssrnx_dex_unresolved_count() == 1u)
            SSRNX_LOGCW("LEDGER", "FIRST unresolved member %s->%s%s caller=%s", cls, name, sig,
                        caller[0] ? caller : "<top>");
    }
    /* The first linkage failure stays the reported causal detail. */
    if (ctx && ctx->report && !ctx->report->detail[0])
        dex_vm_fail_report(ctx, cls, name, sig, 0, 0, detail);
    if (!dex_vm_g_linkage_strict && ctx && ctx->env && !ssrnx_jni_exception_pending_host(ctx->env)) {
        const bool field = kind && (strcmp(kind, "static-field") == 0 ||
                                    strcmp(kind, "instance-field") == 0);
        char message[640];
        snprintf(message, sizeof(message), "No %s %.60s%.200s in class L%.300s;",
                 field ? "field" : "method", name, sig, cls);
        void *clazz = NULL;
        if (kind && strcmp(kind, "native") == 0) {
            /* ART: a native method with no registered or exported body. */
            snprintf(message, sizeof(message), "No implementation found for %.300s.%.60s%.200s",
                     cls, name, sig);
            (void)ssrnx_jni_throw_new_by_name(ctx->env, "java/lang/UnsatisfiedLinkError", message);
        } else if ((clazz = ssrnx_jni_find_class_ref(ctx->env, cls)) != NULL) {
            ssrnx_jni_delete_local_ref_host(ctx->env, clazz);
            (void)ssrnx_jni_throw_new_by_name(ctx->env,
                                            field ? "java/lang/NoSuchFieldError" : "java/lang/NoSuchMethodError",
                                            message);
        } else {
            ssrnx_jni_exception_clear_host(ctx->env);
            (void)ssrnx_jni_throw_new_by_name(ctx->env, "java/lang/NoClassDefFoundError", cls);
        }
    }
    return SSRNX_ERR_APK_UNSUPPORTED;
}

static const char *dex_vm_skip_type(const char *p) {
    if (!p || !*p) return NULL;
    while (*p == '[') ++p;
    if (*p == 'L') {
        const char *semi = strchr(p, ';');
        return semi ? semi + 1 : NULL;
    }
    return p + 1;
}

static bool dex_vm_host_args_from_words(const char *sig, const uint64_t *words, size_t word_count,
                                 SsrnxJniValue *out, size_t out_cap, size_t *out_count) {
    if (!sig || sig[0] != '(' || !out_count) return false;
    size_t wi = 0, ai = 0;
    const char *p = sig + 1;
    while (*p && *p != ')') {
        if (ai >= out_cap || wi >= word_count) return false;
        const char *start = p;
        while (*p == '[') ++p;
        const char t = *p;
        memset(&out[ai], 0, sizeof(out[ai]));
        const bool wide = start[0] != '[' && (t == 'J' || t == 'D');
        if (start[0] == '[' || t == 'L') out[ai].l = (void *)(uintptr_t)words[wi];
        else if (wide) out[ai].j = (int64_t)words[wi];
        else if (t == 'F') {
            uint32_t bits = (uint32_t)words[wi];
            memcpy(&out[ai].f, &bits, sizeof(bits));
        } else out[ai].i = (int32_t)words[wi];
        p = dex_vm_skip_type(start);
        if (!p) return false;
        wi += wide ? 2u : 1u;
        ++ai;
    }
    if (*p != ')' || wi > word_count) return false;
    *out_count = ai;
    return true;
}

#if !defined(__aarch64__)
static bool dex_vm_native_gpr_args(const char *signature, const SsrnxJniValue *args, size_t arg_count,
                            uint64_t *out_words, size_t out_cap, size_t *out_count,
                            char *out_return_type) {
    if (!signature || signature[0] != '(' || !out_count || !out_return_type) return false;
    size_t ai = 0, wi = 0;
    const char *p = signature + 1;
    while (*p && *p != ')') {
        if (ai >= arg_count || wi >= out_cap) return false;
        while (*p == '[') {
            while (*p == '[') ++p;
            if (*p == 'L') { while (*p && *p != ';') ++p; if (*p != ';') return false; ++p; }
            else if (*p) ++p;
            out_words[wi++] = (uint64_t)(uintptr_t)args[ai++].l;
            goto next_arg;
        }
        if (*p == 'L') {
            while (*p && *p != ';') ++p;
            if (*p != ';') return false;
            ++p;
            out_words[wi++] = (uint64_t)(uintptr_t)args[ai++].l;
        } else if (*p == 'J') {
            out_words[wi++] = (uint64_t)args[ai++].j;
            ++p;
        } else if (*p == 'F' || *p == 'D') {
            return false; /* A generic FP-register bridge is a separate ABI slice. */
        } else if (*p == 'Z') {
            out_words[wi++] = args[ai++].z ? 1u : 0u; ++p;
        } else if (*p == 'B') {
            out_words[wi++] = (uint64_t)(int64_t)(int8_t)args[ai++].b; ++p;
        } else if (*p == 'C') {
            out_words[wi++] = (uint64_t)args[ai++].c; ++p;
        } else if (*p == 'S') {
            out_words[wi++] = (uint64_t)(int64_t)(int16_t)args[ai++].s; ++p;
        } else if (*p == 'I') {
            out_words[wi++] = (uint64_t)(int64_t)args[ai++].i; ++p;
        } else {
            return false;
        }
next_arg:
        ;
    }
    if (*p != ')' || ai != arg_count) return false;
    ++p;
    if (!*p) return false;
    *out_return_type = *p;
    if (*p == '[' || *p == 'L') {
        /* Object/array returns use the ordinary integer return register. */
    } else if (*p == 'F' || *p == 'D') {
        return false;
    } else if (!strchr("VZBCSIJ", *p)) {
        return false;
    }
    *out_count = wi;
    return true;
}
#endif

#if defined(__aarch64__)
extern uint64_t ssrnx_dex_call_native_a64(void *fn, const uint64_t *gpr,
                                           const uint64_t *fpr, uint64_t *out_fp_bits,
                                           const uint64_t *stack_words, size_t stack_count);

/* JNI functions take JNIEnv*, jobject/jclass and at most 255 Java argument
 * slots, so this many stack words always suffice. */
#define NATIVE_A64_STACK_CAP 256u

/* Classify Java arguments per AAPCS64 (Linux/Android): integer-class values
 * fill x0-x7, float/double fill v0-v7, and once a register class is exhausted
 * its remaining arguments take the next 8-byte stack slot in argument order
 * (a float occupies the low 4 bytes of its slot). */
static bool dex_vm_native_a64_args(const char *signature, const SsrnxJniValue *args, size_t arg_count,
                            uint64_t gpr[8], size_t gpr_start, uint64_t fpr[8],
                            uint64_t *stack, size_t stack_cap, size_t *out_stack_count,
                            char *out_return_type) {
    if (!signature || signature[0] != '(' || !gpr || !fpr || !stack || !out_stack_count ||
        !out_return_type || gpr_start > 8u)
        return false;
    size_t ai = 0, gi = gpr_start, fi = 0, si = 0;
    const char *p = signature + 1;
#define A64_GP(value) do { const uint64_t v_ = (value); \
        if (gi < 8u) gpr[gi++] = v_; else if (si < stack_cap) stack[si++] = v_; else return false; } while (0)
#define A64_FP(value) do { const uint64_t v_ = (value); \
        if (fi < 8u) fpr[fi++] = v_; else if (si < stack_cap) stack[si++] = v_; else return false; } while (0)
    while (*p && *p != ')') {
        if (ai >= arg_count) return false;
        const char *start = p;
        while (*p == '[') ++p;
        const char t = *p;
        if (start[0] == '[') {
            if (t == 'L') { while (*p && *p != ';') ++p; if (*p != ';') return false; ++p; }
            else if (*p) ++p;
            A64_GP((uint64_t)(uintptr_t)args[ai++].l);
            continue;
        }
        if (t == 'L') {
            while (*p && *p != ';') ++p;
            if (*p != ';') return false;
            ++p;
            A64_GP((uint64_t)(uintptr_t)args[ai++].l);
        } else if (t == 'F') {
            uint32_t bits = 0;
            memcpy(&bits, &args[ai++].f, sizeof(bits));
            A64_FP(bits);
            ++p;
        } else if (t == 'D') {
            uint64_t bits = 0;
            memcpy(&bits, &args[ai++].d, sizeof(bits));
            A64_FP(bits);
            ++p;
        } else if (t == 'J') {
            A64_GP((uint64_t)args[ai++].j); ++p;
        } else if (t == 'Z') {
            A64_GP(args[ai++].z ? 1u : 0u); ++p;
        } else if (t == 'B') {
            A64_GP((uint64_t)(int64_t)(int8_t)args[ai++].b); ++p;
        } else if (t == 'C') {
            A64_GP((uint64_t)args[ai++].c); ++p;
        } else if (t == 'S') {
            A64_GP((uint64_t)(int64_t)(int16_t)args[ai++].s); ++p;
        } else if (t == 'I') {
            A64_GP((uint64_t)(int64_t)args[ai++].i); ++p;
        } else {
            return false;
        }
    }
#undef A64_GP
#undef A64_FP
    if (*p != ')' || ai != arg_count) return false;
    ++p;
    if (!*p || (!strchr("VZBCSIJFDL[", *p))) return false;
    *out_return_type = *p;
    *out_stack_count = si;
    return true;
}
#endif

static bool dex_vm_dex_words_from_args(const char *signature, const SsrnxJniValue *args,
                                size_t arg_count, uint64_t *out_words, size_t out_cap,
                                size_t *out_count, char *out_return_type) {
    if (!signature || signature[0] != '(' || !out_count || !out_return_type) return false;
    size_t ai = 0, wi = 0;
    const char *p = signature + 1;
    while (*p && *p != ')') {
        if (ai >= arg_count || wi >= out_cap) return false;
        const char *start = p;
        while (*p == '[') ++p;
        const char t = *p;
        if (start[0] == '[') {
            if (t == 'L') { while (*p && *p != ';') ++p; if (*p != ';') return false; ++p; }
            else if (*p) ++p;
            out_words[wi++] = (uint64_t)(uintptr_t)args[ai++].l;
            continue;
        }
        if (t == 'L') {
            while (*p && *p != ';') ++p;
            if (*p != ';') return false;
            ++p;
            out_words[wi++] = (uint64_t)(uintptr_t)args[ai++].l;
        } else if (t == 'J' || t == 'D') {
            if (wi + 1u >= out_cap) return false;
            uint64_t bits = 0;
            if (t == 'J') bits = (uint64_t)args[ai].j;
            else memcpy(&bits, &args[ai].d, sizeof(bits));
            out_words[wi++] = bits;
            out_words[wi++] = bits >> 32;
            ++ai; ++p;
        } else if (t == 'F') {
            uint32_t bits = 0;
            memcpy(&bits, &args[ai].f, sizeof(bits));
            out_words[wi++] = bits;
            ++ai; ++p;
        } else if (t == 'Z') { out_words[wi++] = args[ai++].z ? 1u : 0u; ++p; }
        else if (t == 'B') { out_words[wi++] = (uint64_t)(int64_t)(int8_t)args[ai++].b; ++p; }
        else if (t == 'C') { out_words[wi++] = (uint64_t)args[ai++].c; ++p; }
        else if (t == 'S') { out_words[wi++] = (uint64_t)(int64_t)(int16_t)args[ai++].s; ++p; }
        else if (t == 'I') { out_words[wi++] = (uint64_t)(int64_t)args[ai++].i; ++p; }
        else return false;
    }
    if (*p != ')' || ai != arg_count) return false;
    ++p;
    if (!*p) return false;
    *out_return_type = *p;
    *out_count = wi;
    return true;
}

#if !defined(__aarch64__)
static uint64_t dex_vm_call_native_gpr_value(void *fn, const uint64_t *a, size_t n) {
    switch (n) {
        case 0: return ((uint64_t (*)(void))fn)();
        case 1: return ((uint64_t (*)(uint64_t))fn)(a[0]);
        case 2: return ((uint64_t (*)(uint64_t,uint64_t))fn)(a[0],a[1]);
        case 3: return ((uint64_t (*)(uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2]);
        case 4: return ((uint64_t (*)(uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3]);
        case 5: return ((uint64_t (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4]);
        case 6: return ((uint64_t (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4],a[5]);
        case 7: return ((uint64_t (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4],a[5],a[6]);
        case 8: return ((uint64_t (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]);
        case 9: return ((uint64_t (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8]);
        case 10:return ((uint64_t (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9]);
        case 11:return ((uint64_t (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],a[10]);
        case 12:return ((uint64_t (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],a[10],a[11]);
        default:return 0;
    }
}

static void dex_vm_call_native_gpr_void(void *fn, const uint64_t *a, size_t n) {
    switch (n) {
        case 0: ((void (*)(void))fn)(); break;
        case 1: ((void (*)(uint64_t))fn)(a[0]); break;
        case 2: ((void (*)(uint64_t,uint64_t))fn)(a[0],a[1]); break;
        case 3: ((void (*)(uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2]); break;
        case 4: ((void (*)(uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3]); break;
        case 5: ((void (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4]); break;
        case 6: ((void (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4],a[5]); break;
        case 7: ((void (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4],a[5],a[6]); break;
        case 8: ((void (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]); break;
        case 9: ((void (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8]); break;
        case 10:((void (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9]); break;
        case 11:((void (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],a[10]); break;
        case 12:((void (*)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t))fn)(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],a[10],a[11]); break;
        default: break;
    }
}

#endif

/* Stall reports name the native method a thread is inside. Weak so DEX host
 * tests that link without the Bionic thread registry stay self-contained. */
extern void ssrnx_bionic_thread_activity_enter(SsrnxThreadActivity *saved, const char *kind,
                                               const char *class_name, const char *method_name)
    __attribute__((weak));
extern void ssrnx_bionic_thread_activity_leave(const SsrnxThreadActivity *saved) __attribute__((weak));
static void dex_vm_thread_activity_enter(SsrnxThreadActivity *saved, const char *kind,
                                  const char *class_name, const char *method_name) {
    saved->since_ms = 0;
    if (ssrnx_bionic_thread_activity_enter)
        ssrnx_bionic_thread_activity_enter(saved, kind, class_name, method_name);
}
static void dex_vm_thread_activity_leave(const SsrnxThreadActivity *saved) {
    if (ssrnx_bionic_thread_activity_leave) ssrnx_bionic_thread_activity_leave(saved);
}

static bool dex_vm_native_jni_scope_enter(void *env) {
    if (ssrnx_jni_push_local_frame_host(env, 32) != SSRNX_OK) return false;
    if (ssrnx_jni_push_application_class_loader_host(env) != SSRNX_OK) {
        (void)ssrnx_jni_pop_local_frame_host(env, NULL);
        return false;
    }
    return true;
}

static void *dex_vm_native_jni_scope_leave(void *env, void *result_ref) {
    ssrnx_jni_pop_application_class_loader_host(env);
    return ssrnx_jni_pop_local_frame_host(env, result_ref);
}


/* Callee for ssrnx_dex_native_bridge_selftest: records what arrived. Nine
 * integer-class and twelve FP arguments overflow both register classes so
 * stack slots interleave in argument order. */
static double dex_vm_g_a64_probe_seen[21];
static void *dex_vm_g_a64_probe_object;
static int64_t dex_vm_a64_bridge_probe(void *env, void *self, int32_t i0, int32_t i1, int32_t i2, int32_t i3,
                                int32_t i4, int32_t i5, float f0, double d1, float f2, double d3,
                                float f4, double d5, float f6, double d7, int64_t j6, float f8,
                                uint8_t z7, double d9, int16_t s8, float f10, void *obj, double d11) {
    const double seen[] = {(double)(intptr_t)env, (double)(intptr_t)self, i0, i1, i2, i3, i4, i5,
                           f0, d1, f2, d3, f4, d5, f6, d7, (double)j6, f8, z7, d9, s8};
    memcpy(dex_vm_g_a64_probe_seen, seen, sizeof(seen));
    dex_vm_g_a64_probe_object = obj;
    return (int64_t)(f10 * 4.0f + d11 * 2.0);
}

bool ssrnx_dex_native_bridge_selftest(char *detail, size_t detail_cap) {
    if (detail && detail_cap) detail[0] = '\0';
    static const char sig[] = "(IIIIIIFDFDFDFDJFZDSFLjava/lang/Object;D)J";
    SsrnxJniValue a[22];
    memset(a, 0, sizeof(a));
    const int32_t ints[6] = {-1, 2, -3, 4, -5, 6};
    for (int i = 0; i < 6; ++i) a[i].i = ints[i];
    a[6].f = 1.5f; a[7].d = -2.25; a[8].f = 3.5f; a[9].d = 4.75; a[10].f = -5.5f; a[11].d = 6.125;
    a[12].f = 7.25f; a[13].d = -8.5; a[14].j = INT64_C(-9000000001); a[15].f = 10.5f; a[16].z = 1;
    a[17].d = 11.75; a[18].s = -12; a[19].f = 13.25f; a[20].l = (void *)&dex_vm_g_a64_probe_object; a[21].d = 14.5;
    const double want[21] = {(double)(intptr_t)0x1000, (double)(intptr_t)0x2000, -1, 2, -3, 4, -5, 6,
                             1.5, -2.25, 3.5, 4.75, -5.5, 6.125, 7.25, -8.5, -9000000001.0, 10.5, 1, 11.75, -12};
#if defined(__aarch64__)
    uint64_t gpr[8] = {0}, fpr[8] = {0}, fp_ret = 0, stack_words[NATIVE_A64_STACK_CAP];
    size_t stack_count = 0;
    char ret = 0;
    gpr[0] = 0x1000; gpr[1] = 0x2000;
    if (!dex_vm_native_a64_args(sig, a, 22, gpr, 2u, fpr, stack_words, NATIVE_A64_STACK_CAP, &stack_count, &ret)) {
        if (detail) snprintf(detail, detail_cap, "classification failed");
        return false;
    }
    memset(dex_vm_g_a64_probe_seen, 0, sizeof(dex_vm_g_a64_probe_seen));
    dex_vm_g_a64_probe_object = NULL;
    const int64_t r = (int64_t)ssrnx_dex_call_native_a64((void *)dex_vm_a64_bridge_probe, gpr, fpr, &fp_ret,
                                                         stack_words, stack_count);
    for (int i = 0; i < 21; ++i) {
        if (dex_vm_g_a64_probe_seen[i] != want[i]) {
            if (detail) snprintf(detail, detail_cap, "arg%d got=%g want=%g stackWords=%zu", i,
                                 dex_vm_g_a64_probe_seen[i], want[i], stack_count);
            return false;
        }
    }
    if (dex_vm_g_a64_probe_object != (void *)&dex_vm_g_a64_probe_object || r != (int64_t)(13.25f * 4.0f + 14.5 * 2.0)) {
        if (detail) snprintf(detail, detail_cap, "object/last-args mismatch result=%lld stackWords=%zu",
                             (long long)r, stack_count);
        return false;
    }
    if (detail) snprintf(detail, detail_cap, "stackWords=%zu", stack_count);
    return true;
#else
    (void)sig; (void)want; (void)dex_vm_a64_bridge_probe;
    if (detail) snprintf(detail, detail_cap, "host-without-a64-bridge");
    return true;
#endif
}

/* Diagnostics only: sample each native target independently on each thread so
 * hot input/render calls cannot hide the first initialization/lifecycle call. */
static uint64_t dex_vm_native_diagnostic_sequence(void *fn) {
    static _Thread_local struct { void *fn; uint64_t calls; } targets[128];
    for (size_t i = 0; i < sizeof(targets) / sizeof(targets[0]); ++i) {
        if (!targets[i].fn) targets[i].fn = fn;
        if (targets[i].fn == fn) return ++targets[i].calls;
    }
    return 0;
}

static SsrnxResult dex_vm_execute_native_method(dex_vm_DexExecCtx *ctx, const char *class_name,
                                         const char *method_name, const char *signature,
                                         bool is_static, void *receiver,
                                         const SsrnxJniValue *args, size_t arg_count,
                                         SsrnxJniValue *out_result) {
    if (!ctx || !class_name || !method_name || !signature || !out_result) return SSRNX_ERR_ARGUMENT;
    SSRNX_PERF_SCOPE(SSRNX_PERF_NATIVE);
    void *fn = NULL;
    bool found = false;
    SsrnxResult rc = ssrnx_jni_find_native_binding(class_name, method_name, signature, &fn, &found);
    if (rc != SSRNX_OK) return rc;
    if (!found) {
        if (!dex_vm_native_resolve_cache_lookup(class_name, method_name, signature, &fn, &found) &&
            dex_vm_g_native_resolver) {
            rc = dex_vm_g_native_resolver(class_name, method_name, signature, &fn, &found, dex_vm_g_native_resolver_user);
            if (rc != SSRNX_OK) return rc;
            /* Cache successful symbol resolutions only. Guest dlopen can make a
             * previously absent JNI export appear later without replacing the
             * resolver callback, so negative caching would make that dynamic
             * transition incorrectly permanent. */
            if (found && fn) dex_vm_native_resolve_cache_store(class_name, method_name, signature, fn, true);
        }
    }
    if (!found || !fn) {
        char detail[512];
        snprintf(detail, sizeof(detail), "unresolved native Java method class=%.140s method=%.80s sig=%.140s",
                 class_name, method_name, signature);
        return dex_vm_dex_linkage_failure(ctx, "native", class_name, method_name, signature, detail);
    }

    void *second = receiver;
    if (is_static) second = ssrnx_jni_find_class_ref(ctx->env, class_name);
    if (!second) return SSRNX_ERR_APK_UNSUPPORTED;
    memset(out_result, 0, sizeof(*out_result));
    dex_vm_dex_runtime_progress_native(class_name, method_name, signature);
    uint64_t native_begin_us = 0;
    uint64_t perf_before[SSRNX_PERF_KIND_COUNT];
    const uint64_t native_sequence = dex_vm_native_diagnostic_sequence(fn);
    const bool trace_native = ssrnx_diag_full_trace_enabled() ||
        (native_sequence && (native_sequence <= 4 || native_sequence % 300 == 0));
    if (trace_native) {
        uint64_t argument_bits[8] = {0};
        for (size_t i = 0; args && i < arg_count && i < 8; ++i)
            memcpy(&argument_bits[i], &args[i], sizeof(argument_bits[i]));
        SSRNX_LOGCI("DEX", "native JNI BEGIN class=%s method=%s sig=%s entry=%p pthread=%p sequence=%llu pending=%d args=%zu bits=(%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx)",
                    class_name, method_name, signature, fn, (void *)(uintptr_t)pthread_self(),
                    (unsigned long long)native_sequence, ssrnx_jni_exception_pending_host(ctx->env) ? 1 : 0,
                    arg_count, (unsigned long long)argument_bits[0], (unsigned long long)argument_bits[1],
                    (unsigned long long)argument_bits[2], (unsigned long long)argument_bits[3],
                    (unsigned long long)argument_bits[4], (unsigned long long)argument_bits[5],
                    (unsigned long long)argument_bits[6], (unsigned long long)argument_bits[7]);
    }
#if defined(__aarch64__)
    uint64_t gpr[8] = {0}, fpr[8] = {0}, fp_ret = 0;
    uint64_t stack_words[NATIVE_A64_STACK_CAP];
    size_t stack_count = 0;
    gpr[0] = (uint64_t)(uintptr_t)ctx->env;
    gpr[1] = (uint64_t)(uintptr_t)second;
    char ret = 0;
    if (!dex_vm_native_a64_args(signature, args, arg_count, gpr, 2u, fpr, stack_words, NATIVE_A64_STACK_CAP,
                         &stack_count, &ret)) {
        if (is_static) ssrnx_jni_delete_local_ref_host(ctx->env, second);
        dex_vm_fail_report(ctx, class_name, method_name, signature, 0, 0,
                    "native JNI signature does not match its arguments");
        return SSRNX_ERR_APK_UNSUPPORTED;
    }
    if (ssrnx_diag_full_trace_enabled() || (native_sequence && native_sequence <= 4)) {
        /* A guest abort never returns through execute_native_method().  Make
         * the call-site checkpoint durable before entering guest code so the
         * startup transition survives a CPU exception. Periodic render traces
         * use the asynchronous writer; SD flushes must not block every 300 frames. */
        ssrnx_diag_flush();
    }
    if (!dex_vm_native_jni_scope_enter(ctx->env)) {
        if (is_static) ssrnx_jni_delete_local_ref_host(ctx->env, second);
        return SSRNX_ERR_NO_MEMORY;
    }
    SsrnxThreadActivity saved_activity;
    dex_vm_thread_activity_enter(&saved_activity, "native", class_name, method_name);
    ssrnx_perf_account_read(perf_before);
    native_begin_us = dex_vm_dex_monotonic_us();
    const uint64_t raw = ssrnx_dex_call_native_a64(fn, gpr, fpr, &fp_ret, stack_words, stack_count);
    if (ret == 'L' || ret == '[') {
        out_result->l = dex_vm_native_jni_scope_leave(ctx->env, (void *)(uintptr_t)raw);
    } else {
        (void)dex_vm_native_jni_scope_leave(ctx->env, NULL);
        if (ret == 'J') out_result->j = (int64_t)raw;
        else if (ret == 'Z') out_result->z = raw ? 1u : 0u;
        else if (ret == 'B') out_result->b = (int8_t)raw;
        else if (ret == 'C') out_result->c = (uint16_t)raw;
        else if (ret == 'S') out_result->s = (int16_t)raw;
        else if (ret == 'F') {
            const uint32_t bits = (uint32_t)fp_ret;
            memcpy(&out_result->f, &bits, sizeof(bits));
        } else if (ret == 'D') {
            memcpy(&out_result->d, &fp_ret, sizeof(fp_ret));
        } else if (ret != 'V') {
            out_result->i = (int32_t)raw;
        }
    }
#else
    uint64_t java_words[10] = {0};
    size_t java_count = 0;
    char ret = 0;
    if (!dex_vm_native_gpr_args(signature, args, arg_count, java_words, 10, &java_count, &ret)) {
        if (is_static) ssrnx_jni_delete_local_ref_host(ctx->env, second);
        dex_vm_fail_report(ctx, class_name, method_name, signature, 0, 0,
                    "native JNI signature requires unsupported host FP/argument ABI");
        return SSRNX_ERR_APK_UNSUPPORTED;
    }
    if (java_count + 2u > 12u) {
        if (is_static) ssrnx_jni_delete_local_ref_host(ctx->env, second);
        return SSRNX_ERR_APK_UNSUPPORTED;
    }
    uint64_t call_words[12] = {0};
    call_words[0] = (uint64_t)(uintptr_t)ctx->env;
    call_words[1] = (uint64_t)(uintptr_t)second;
    for (size_t i = 0; i < java_count; ++i) call_words[i + 2u] = java_words[i];
    if (!dex_vm_native_jni_scope_enter(ctx->env)) {
        if (is_static) ssrnx_jni_delete_local_ref_host(ctx->env, second);
        return SSRNX_ERR_NO_MEMORY;
    }
    SsrnxThreadActivity saved_activity;
    dex_vm_thread_activity_enter(&saved_activity, "native", class_name, method_name);
    ssrnx_perf_account_read(perf_before);
    native_begin_us = dex_vm_dex_monotonic_us();
    if (ret == 'V') {
        dex_vm_call_native_gpr_void(fn, call_words, java_count + 2u);
        (void)dex_vm_native_jni_scope_leave(ctx->env, NULL);
    } else {
        const uint64_t raw = dex_vm_call_native_gpr_value(fn, call_words, java_count + 2u);
        if (ret == 'L' || ret == '[')
            out_result->l = dex_vm_native_jni_scope_leave(ctx->env, (void *)(uintptr_t)raw);
        else {
            (void)dex_vm_native_jni_scope_leave(ctx->env, NULL);
            if (ret == 'J') out_result->j = (int64_t)raw;
            else if (ret == 'Z') out_result->z = raw ? 1u : 0u;
            else if (ret == 'B') out_result->b = (int8_t)raw;
            else if (ret == 'C') out_result->c = (uint16_t)raw;
            else if (ret == 'S') out_result->s = (int16_t)raw;
            else out_result->i = (int32_t)raw;
        }
    }
#endif
    dex_vm_thread_activity_leave(&saved_activity);
    const uint64_t native_end_us = dex_vm_dex_monotonic_us();
    const uint64_t native_elapsed_us = native_end_us >= native_begin_us
                                     ? native_end_us - native_begin_us : 0;
    if (native_elapsed_us >= UINT64_C(50000)) {
        uint64_t previous = __atomic_load_n(&dex_vm_g_last_slow_native_log_us, __ATOMIC_RELAXED);
        if (native_end_us >= previous + UINT64_C(1000000) &&
            __atomic_compare_exchange_n(&dex_vm_g_last_slow_native_log_us, &previous,
                                        native_end_us, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
            uint64_t perf_after[SSRNX_PERF_KIND_COUNT];
            ssrnx_perf_account_read(perf_after);
            SSRNX_LOGCI("PERF", "slow native JNI class=%s method=%s sig=%s elapsedUs=%" PRIu64
                        " fileIoUs=%" PRIu64 " javaUs=%" PRIu64 " waitUs=%" PRIu64 " presentUs=%" PRIu64
                        " lockUs=%" PRIu64,
                        class_name, method_name, signature, native_elapsed_us,
                        perf_after[SSRNX_PERF_FILE_IO] - perf_before[SSRNX_PERF_FILE_IO],
                        perf_after[SSRNX_PERF_JAVA] - perf_before[SSRNX_PERF_JAVA],
                        perf_after[SSRNX_PERF_GUEST_WAIT] - perf_before[SSRNX_PERF_GUEST_WAIT],
                        perf_after[SSRNX_PERF_PRESENT] - perf_before[SSRNX_PERF_PRESENT],
                        perf_after[SSRNX_PERF_HOST_LOCK] - perf_before[SSRNX_PERF_HOST_LOCK]);
            if (native_elapsed_us >= UINT64_C(250000))
                ssrnx_compat_log_memory_state("slow-native-return");
        }
    }
    if (is_static) ssrnx_jni_delete_local_ref_host(ctx->env, second);
    if (trace_native) {
        uint64_t result_bits = 0;
        memcpy(&result_bits, out_result, sizeof(result_bits));
        SSRNX_LOGCI("DEX", "native JNI END class=%s method=%s sig=%s entry=%p pthread=%p sequence=%llu resultBits=0x%llx pending=%d elapsedUs=%llu",
                    class_name, method_name, signature, fn, (void *)(uintptr_t)pthread_self(),
                    (unsigned long long)native_sequence, (unsigned long long)result_bits,
                    ssrnx_jni_exception_pending_host(ctx->env) ? 1 : 0, (unsigned long long)native_elapsed_us);
    }
    return SSRNX_OK;
}

SsrnxResult ssrnx_dex_call_static_native_method(const char *source_path,
                                                 const char *class_name,
                                                 const char *method_name,
                                                 const char *signature,
                                                 void *env_handle,
                                                 const SsrnxJniValue *args,
                                                 size_t arg_count,
                                                 SsrnxJniValue *out_result) {
    if (!class_name || !method_name || !signature || !env_handle || !out_result)
        return SSRNX_ERR_ARGUMENT;
    SsrnxDexExecReport report = {0};
    dex_vm_DexExecCtx ctx = {
        .source_path = source_path ? source_path : dex_vm_g_java_method_source,
        .env = env_handle,
        .report = &report,
    };
    SsrnxResult init_rc = dex_vm_ensure_dex_class_initialized(&ctx, class_name, 0);
    if (init_rc != SSRNX_OK) return init_rc;
    return dex_vm_execute_native_method(&ctx, class_name, method_name, signature, true, NULL,
                                 args, arg_count, out_result);
}

static SsrnxResult dex_vm_execute_target(dex_vm_DexExecCtx *ctx, const char *class_name, const char *method_name,
                                  const char *signature, bool is_static, bool virtual_dispatch,
                                  bool interface_dispatch, const uint64_t *words,
                                  size_t word_count, unsigned depth,
                                  SsrnxJniValue *out_result);

static SsrnxResult dex_vm_execute_image(dex_vm_DexExecCtx *ctx, SsrnxDexMethodImage *m,
                                 const uint64_t *incoming_words, size_t incoming_count,
                                 unsigned depth, SsrnxJniValue *out_result);

/* Native stack per Java call bounds how deep guest Java can recurse (ART
 * frames are a few hundred bytes). Data that a frame must keep while a nested
 * call runs therefore lives on the heap, and cold diagnostics are formatted in
 * noinline helpers so their buffers are not reserved in recursive frames. */
typedef struct dex_vm_DexInvokeNames {
    char cls[512];
    char name[256];
    char sig[SSRNX_DEX_SIGNATURE_CAP];
    char ref_detail[512];
} dex_vm_DexInvokeNames;

static atomic_uint dex_vm_g_null_stringbuilder_arg_reports;

static __attribute__((noinline, cold)) void dex_log_null_stringbuilder_arg(
        const SsrnxDexMethodImage *caller, uint32_t pc, uint8_t opcode,
        const dex_vm_DexInvokeNames *target, const uint64_t *words, size_t word_count,
        uint32_t receiver_reg, uint32_t string_reg) {
    if (!caller || !target || !words || word_count != 2 || words[1] != 0 ||
        strcmp(target->cls, "java/lang/StringBuilder") != 0 ||
        strcmp(target->name, "<init>") != 0 ||
        strcmp(target->sig, "(Ljava/lang/String;)V") != 0)
        return;

    if (atomic_fetch_add_explicit(&dex_vm_g_null_stringbuilder_arg_reports, 1u,
                                  memory_order_relaxed) >= 8u)
        return;
    SSRNX_LOGCW("DEX", "null StringBuilder(String) argument caller=%s.%s%s pc=%#x opcode=0x%02x receiverReg=v%u receiver=%p stringReg=v%u wordCount=%zu",
                caller->declaring_class, caller->method_name, caller->signature,
                pc, opcode, receiver_reg, (void *)(uintptr_t)words[0],
                string_reg, word_count);
    SsrnxDexStackFrame frames[8];
    const size_t count = ssrnx_dex_current_stack(frames, 8u);
    for (size_t i = 0; i < count && i < 8u; ++i)
        SSRNX_LOGCW("DEX", "null StringBuilder(String) stack[%zu]=%s.%s",
                    i, frames[i].class_name, frames[i].method_name);
}

static __attribute__((noinline)) void dex_invoke_decode_failure(dex_vm_DexExecCtx *ctx, const SsrnxDexMethodImage *m,
                                                                uint32_t pc, uint8_t op, uint32_t mid,
                                                                bool range, uint32_t first, uint32_t count,
                                                                bool have_previous, uint32_t previous_pc,
                                                                uint8_t previous_op, const char *ref_detail) {
    char detail[768];
    if (range)
        snprintf(detail, sizeof(detail),
                 "DEX invoke-range method-ref decode failed pc=%#x opcode=0x%02x methodIdx=%u "
                 "firstReg=%u wordCount=%u regs=%u u0=%#06x u1=%#06x u2=%#06x "
                 "previousPc=%#x previousOp=0x%02x: %.360s",
                 pc, op, (unsigned)mid, (unsigned)first, (unsigned)count,
                 (unsigned)m->registers_size, (unsigned)m->insns[pc],
                 (unsigned)m->insns[pc + 1], (unsigned)m->insns[pc + 2],
                 have_previous ? previous_pc : UINT32_MAX, have_previous ? previous_op : 0u, ref_detail);
    else
        snprintf(detail, sizeof(detail),
                 "DEX invoke method-ref decode failed pc=%#x opcode=0x%02x methodIdx=%u "
                 "u0=%#06x u1=%#06x u2=%#06x previousPc=%#x previousOp=0x%02x: %.430s",
                 pc, op, (unsigned)mid, (unsigned)m->insns[pc], (unsigned)m->insns[pc + 1],
                 (unsigned)m->insns[pc + 2], have_previous ? previous_pc : UINT32_MAX,
                 have_previous ? previous_op : 0u, ref_detail);
    dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, pc, op, detail);
    SSRNX_LOGCW("DEX", "%s", detail);
}

static __attribute__((noinline)) void dex_invoke_uncaught(dex_vm_DexExecCtx *ctx, const SsrnxDexMethodImage *m,
                                                          uint32_t pc, uint8_t op, const dex_vm_DexInvokeNames *nm) {
    char origin[320];
    snprintf(origin, sizeof(origin), "%.160s.%.60s%.90s", nm->cls, nm->name, nm->sig);
    dex_vm_dex_report_uncaught_propagation(ctx, m, pc, op, origin);
}

static __attribute__((noinline)) void dex_report_unresolved_field(dex_vm_DexExecCtx *ctx, const SsrnxDexMethodImage *m,
                                                                  uint32_t pc, uint8_t op, bool is_static,
                                                                  const char *cls, const char *name, const char *sig) {
    char detail[768];
    snprintf(detail, sizeof(detail), "unresolved %s field class=%.220s name=%.160s sig=%.220s",
             is_static ? "static" : "instance", cls, name, sig);
    (void)dex_vm_dex_linkage_failure(ctx, is_static ? "static-field" : "instance-field", cls, name, sig, detail);
    dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, pc, op, detail);
}

/* Java 8 class-method resolution includes maximally-specific superinterface
 * defaults when no class/superclass implementation exists. DEX invoke-virtual
 * can therefore legitimately reference a concrete class while the executable
 * body lives in one of its interfaces. Walk the package class/interface graph
 * after the normal class hierarchy has missed. The DEX verifier prevents
 * cyclic inheritance; the depth bound also keeps malformed input finite. */
/* Per-level state of the default-method search, heap-held because each level
 * recurses (and may run the default body) while it is alive. */
typedef struct dex_vm_DexDefaultTreeLevel {
    char super_name[512];
    char interfaces[4096];
    SsrnxDexMethodImage method;
} dex_vm_DexDefaultTreeLevel;

static SsrnxResult dex_vm_execute_interface_default_tree(dex_vm_DexExecCtx *ctx,
                                                   const char *class_name,
                                                   const char *method_name,
                                                   const char *signature,
                                                   const uint64_t *words,
                                                   size_t word_count,
                                                   unsigned depth,
                                                   SsrnxJniValue *out_result,
                                                   bool *out_found) {
    if (out_found) *out_found = false;
    if (!ctx || !class_name || !method_name || !signature || !out_result || !out_found ||
        depth > DEX_MAX_DEPTH) return SSRNX_ERR_ARGUMENT;

    dex_vm_DexDefaultTreeLevel *level = calloc(1, sizeof(*level));
    if (!level) return SSRNX_ERR_NO_MEMORY;
    bool class_found = false;
    SsrnxResult rc = ssrnx_package_find_dex_class_ex(ctx->source_path, class_name,
                                                      level->super_name, sizeof(level->super_name),
                                                      level->interfaces, sizeof(level->interfaces),
                                                      &class_found);
    if (rc != SSRNX_OK || !class_found) goto done;

    SsrnxDexMethodImage *method = &level->method;
    bool method_class_found = false, method_found = false;
    rc = ssrnx_package_load_dex_method(ctx->source_path, class_name, method_name, signature,
                                       method, &method_class_found, &method_found);
    if (rc != SSRNX_OK) goto done;
    const bool concrete_default = method_found &&
        (method->class_access_flags & DEX_ACC_INTERFACE) != 0 &&
        (method->access_flags & (DEX_ACC_STATIC | DEX_ACC_ABSTRACT)) == 0 &&
        method->code_offset && method->insns;
    if (concrete_default) {
        if (ssrnx_diag_full_trace_enabled())
            SSRNX_LOGCI("DEX", "interface default dispatch class=%s method=%s sig=%s",
                        class_name, method_name, signature);
        *out_found = true;
        rc = dex_vm_execute_image(ctx, method, words, word_count, depth + 1u, out_result);
        ssrnx_package_release_dex_method(method);
        goto done;
    }
    if (method_class_found || method_found) ssrnx_package_release_dex_method(method);

    /* Direct interfaces are more specific than inherited superclass
     * interfaces, so inspect them first. */
    char *save = NULL;
    for (char *iface = strtok_r(level->interfaces, "\n", &save); iface;
         iface = strtok_r(NULL, "\n", &save)) {
        bool found = false;
        rc = dex_vm_execute_interface_default_tree(ctx, iface, method_name, signature,
                                            words, word_count, depth + 1u,
                                            out_result, &found);
        if (rc != SSRNX_OK || found) {
            *out_found = found;
            goto done;
        }
    }

    if (level->super_name[0] && strcmp(level->super_name, class_name) != 0) {
        bool found = false;
        rc = dex_vm_execute_interface_default_tree(ctx, level->super_name, method_name, signature,
                                            words, word_count, depth + 1u,
                                            out_result, &found);
        if (rc != SSRNX_OK || found) {
            *out_found = found;
            goto done;
        }
    }
    rc = SSRNX_OK;
done:
    free(level);
    return rc;
}

static SsrnxResult dex_vm_ensure_dex_class_initialized(dex_vm_DexExecCtx *ctx, const char *class_name,
                                                unsigned depth) {
    if (!ctx || !ctx->source_path || !class_name || !class_name[0] || depth > DEX_MAX_DEPTH)
        return SSRNX_ERR_ARGUMENT;

    const char *optional_reason = ssrnx_optional_sdk_unavailable_reason(class_name);
    if (optional_reason && ssrnx_optional_sdk_skip_class_initialization(class_name)) {
        SSRNX_LOGCI("DEX", "optional SDK class init skipped class=%s reason=%s",
                    class_name, optional_reason);
        return SSRNX_OK;
    }

    char super_name[512] = {0};
    bool class_found = false;
    SsrnxResult rc = ssrnx_package_find_dex_class(ctx->source_path, class_name,
                                                   super_name, sizeof(super_name),
                                                   &class_found);
    if (rc != SSRNX_OK) {
        char detail[512];
        snprintf(detail, sizeof(detail), "DEX class lookup failed class=%s rc=%d (%s)",
                 class_name, (int)rc, ssrnx_result_string(rc));
        dex_vm_fail_report(ctx, class_name, "<class-init>", "()V", 0, 0, detail);
        return rc;
    }
    if (!class_found) return SSRNX_OK; /* Framework/host class: initialized by its model. */

    const pthread_t self = pthread_self();
    SSRNX_PERF_LOCK(&dex_vm_g_class_init_lock);
    dex_vm_DexClassInitEntry *entry = dex_vm_dex_class_init_entry_locked(class_name);
    if (!entry) {
        const size_t tracked = dex_vm_g_class_init_count;
        pthread_mutex_unlock(&dex_vm_g_class_init_lock);
        char detail[640];
        snprintf(detail, sizeof(detail), "class init state allocation failed class=%.400s tracked=%zu",
                 class_name, tracked);
        dex_vm_fail_report(ctx, class_name, "<clinit>", "()V", 0, 0, detail);
        SSRNX_LOGCE("DEX", "%s", detail);
        return SSRNX_ERR_NO_MEMORY;
    }
    for (;;) {
        if (entry->state == DEX_CLASS_INIT_DONE) {
            pthread_mutex_unlock(&dex_vm_g_class_init_lock);
            return SSRNX_OK;
        }
        if (entry->state == DEX_CLASS_INIT_FAILED) {
            pthread_mutex_unlock(&dex_vm_g_class_init_lock);
            /* JLS 12.4.2 step 5: a class whose initialization failed is in an
             * erroneous state; every later active use throws
             * NoClassDefFoundError, which the caller can catch. */
            char message[600];
            snprintf(message, sizeof(message), "Could not initialize class %s", class_name);
            for (char *c = message; *c; ++c) if (*c == '/') *c = '.';
            if (!ssrnx_jni_exception_pending_host(ctx->env))
                (void)ssrnx_jni_throw_new_by_name(ctx->env, "java/lang/NoClassDefFoundError", message);
            dex_vm_fail_report(ctx, class_name, "<clinit>", "()V", 0, 0, message);
            return SSRNX_ERR_APK_UNSUPPORTED;
        }
        if (entry->state == DEX_CLASS_INIT_RUNNING) {
            if (pthread_equal(entry->owner, self)) {
                /* Recursive active use from this class's own <clinit>. Java class
                 * initialization permits the initializing thread to continue. */
                pthread_mutex_unlock(&dex_vm_g_class_init_lock);
                return SSRNX_OK;
            }
            pthread_cond_wait(&dex_vm_g_class_init_cond, &dex_vm_g_class_init_lock);
            continue;
        }
        entry->state = DEX_CLASS_INIT_RUNNING;
        entry->owner = self;
        break;
    }
    pthread_mutex_unlock(&dex_vm_g_class_init_lock);

    /* Java initializes the direct superclass before the class itself. */
    if (super_name[0]) rc = dex_vm_ensure_dex_class_initialized(ctx, super_name, depth + 1u);

    /* Heap-held: <clinit> runs (and may recurse) while the image is alive. */
    SsrnxDexMethodImage *clinit = calloc(1, sizeof(*clinit));
    bool clinit_class_found = false, clinit_found = false;
    if (rc == SSRNX_OK && !clinit) rc = SSRNX_ERR_NO_MEMORY;
    if (rc == SSRNX_OK) {
        rc = ssrnx_package_load_dex_method(ctx->source_path, class_name, "<clinit>", "()V",
                                           clinit, &clinit_class_found, &clinit_found);
    }
    if (rc == SSRNX_OK && clinit_found) {
        if (clinit->access_flags & DEX_ACC_NATIVE) {
            dex_vm_fail_report(ctx, class_name, "<clinit>", "()V", 0, 0,
                        "native DEX <clinit> is invalid/unsupported");
            rc = SSRNX_ERR_APK_UNSUPPORTED;
        } else {
            SsrnxJniValue ignored = {0};
            SSRNX_LOGCI("DEX", "class init begin class=%s super=%s", class_name,
                        super_name[0] ? super_name : "<none>");
            rc = dex_vm_execute_image(ctx, clinit, NULL, 0, depth + 1u, &ignored);
            if (rc == SSRNX_OK)
                SSRNX_LOGCI("DEX", "class init complete class=%s", class_name);
        }
    }
    if (clinit && (clinit_found || clinit_class_found)) ssrnx_package_release_dex_method(clinit);
    free(clinit);

    SSRNX_PERF_LOCK(&dex_vm_g_class_init_lock);
    entry = dex_vm_dex_class_init_entry_locked(class_name);
    if (entry) {
        if (rc == SSRNX_OK) {
            entry->state = DEX_CLASS_INIT_DONE;
            entry->failure_rc = SSRNX_OK;
        } else {
            entry->state = DEX_CLASS_INIT_FAILED;
            entry->failure_rc = rc;
        }
    }
    pthread_cond_broadcast(&dex_vm_g_class_init_cond);
    pthread_mutex_unlock(&dex_vm_g_class_init_lock);
    return rc;
}

static SsrnxResult dex_vm_dex_make_filled_array(dex_vm_DexExecCtx *ctx, const char *descriptor,
                                              const uint64_t *values, uint32_t count,
                                              void **out_array) {
    if (out_array) *out_array = NULL;
    if (!ctx || !descriptor || descriptor[0] != '[' || !descriptor[1] ||
        (!values && count) || !out_array || count > INT32_MAX)
        return SSRNX_ERR_ARGUMENT;

    void *array_ref = NULL;
    const char element = descriptor[1];
    if (element == 'L' || element == '[') {
        array_ref = ssrnx_jni_new_local_reference_array(ctx->env, descriptor,
                                                        (int32_t)count);
        if (!array_ref) return SSRNX_ERR_NO_MEMORY;
        for (uint32_t i = 0; i < count; ++i) {
            SsrnxResult rc = ssrnx_jni_set_object_array_element_host(
                ctx->env, array_ref, (int32_t)i, (void *)(uintptr_t)values[i]);
            if (rc != SSRNX_OK) {
                ssrnx_jni_delete_local_ref_host(ctx->env, array_ref);
                return rc;
            }
        }
    } else if (element == 'Z') {
        array_ref = ssrnx_jni_new_local_boolean_array(ctx->env, (int32_t)count);
        if (!array_ref) return SSRNX_ERR_NO_MEMORY;
        for (uint32_t i = 0; i < count; ++i) {
            const uint8_t value = values[i] ? 1u : 0u;
            if (ssrnx_jni_boolean_array_write(array_ref, (int32_t)i, 1, &value) != SSRNX_OK)
                goto fill_failed;
        }
    } else if (element == 'B') {
        array_ref = ssrnx_jni_new_local_byte_array(ctx->env, (int32_t)count);
        if (!array_ref) return SSRNX_ERR_NO_MEMORY;
        for (uint32_t i = 0; i < count; ++i) {
            const int8_t value = (int8_t)(int32_t)values[i];
            if (ssrnx_jni_byte_array_write(array_ref, (int32_t)i, 1, &value) != SSRNX_OK)
                goto fill_failed;
        }
    } else if (element == 'C') {
        array_ref = ssrnx_jni_new_local_char_array(ctx->env, (int32_t)count);
        if (!array_ref) return SSRNX_ERR_NO_MEMORY;
        for (uint32_t i = 0; i < count; ++i) {
            const uint16_t value = (uint16_t)values[i];
            if (ssrnx_jni_char_array_write(array_ref, (int32_t)i, 1, &value) != SSRNX_OK)
                goto fill_failed;
        }
    } else if (element == 'S') {
        array_ref = ssrnx_jni_new_local_short_array(ctx->env, (int32_t)count);
        if (!array_ref) return SSRNX_ERR_NO_MEMORY;
        for (uint32_t i = 0; i < count; ++i) {
            const int16_t value = (int16_t)(int32_t)values[i];
            if (ssrnx_jni_short_array_write(array_ref, (int32_t)i, 1, &value) != SSRNX_OK)
                goto fill_failed;
        }
    } else if (element == 'I') {
        array_ref = ssrnx_jni_new_local_int_array(ctx->env, (int32_t)count);
        if (!array_ref) return SSRNX_ERR_NO_MEMORY;
        for (uint32_t i = 0; i < count; ++i) {
            const int32_t value = (int32_t)values[i];
            if (ssrnx_jni_int_array_write(array_ref, (int32_t)i, 1, &value) != SSRNX_OK)
                goto fill_failed;
        }
    } else if (element == 'F') {
        array_ref = ssrnx_jni_new_local_float_array(ctx->env, (int32_t)count);
        if (!array_ref) return SSRNX_ERR_NO_MEMORY;
        for (uint32_t i = 0; i < count; ++i) {
            const uint32_t bits = (uint32_t)values[i];
            float value = 0.0f;
            memcpy(&value, &bits, sizeof(value));
            if (ssrnx_jni_float_array_write(array_ref, (int32_t)i, 1, &value) != SSRNX_OK)
                goto fill_failed;
        }
    } else {
        /* DEX filled-new-array may only use element widths of 32 bits or less;
         * long[] and double[] are verifier-invalid for this opcode. */
        return SSRNX_ERR_APK_UNSUPPORTED;
    }

    *out_array = array_ref;
    return SSRNX_OK;

fill_failed:
    ssrnx_jni_delete_local_ref_host(ctx->env, array_ref);
    return SSRNX_ERR_APK_UNSUPPORTED;
}

/* dex buffered read */
/* Android 11 BufferedInputStream.read(): accelerate only the already-buffered
 * branch. Refill/EOF/closed paths continue through the original Java, including
 * virtual fill-time callbacks, mark/reset state and exception handling.
 * The selected declaring method is checked after virtual dispatch, so a
 * subclass's read() override is never replaced. */
static bool dex_vm_dex_buffered_read_cached(dex_vm_DexExecCtx *ctx, const SsrnxDexMethodImage *m,
                                      const uint64_t *words, size_t count,
                                      SsrnxJniValue *out) {
    static const char owner[] = "java/io/BufferedInputStream";
    if (count != 1 || !words[0] || strcmp(m->method_name, "read") != 0 ||
        strcmp(m->signature, "()I") != 0 || strcmp(m->declaring_class, owner) != 0)
        return false;
    void *self = (void *)(uintptr_t)words[0];
    if (ssrnx_jni_monitor_enter_host(ctx->env, self) != SSRNX_JNI_OK) return false;
    SsrnxJniValue pos = {0}, end = {0}, buf = {0};
    bool found = false, handled = false;
    if (ssrnx_jni_get_instance_field_value(ctx->env, self, owner, "pos", "I", &pos, &found) != SSRNX_OK || !found)
        goto done;
    if (ssrnx_jni_get_instance_field_value(ctx->env, self, owner, "count", "I", &end, &found) != SSRNX_OK || !found ||
        pos.i < 0 || pos.i >= end.i) goto done;
    if (ssrnx_jni_get_instance_field_value(ctx->env, self, owner, "buf", "[B", &buf, &found) != SSRNX_OK || !found || !buf.l)
        goto done;
    /* Array access is checked through the same JNI object model as aget-byte. */
    uint64_t byte = 0;
    if (ssrnx_jni_primitive_array_get_host(buf.l, 'B', pos.i, &byte) != SSRNX_OK) goto done;
    ++pos.i;
    if (ssrnx_jni_set_instance_field_value_for_class(self, owner, "pos", "I", pos, &found) != SSRNX_OK || !found)
        goto done;
    out->i = (int32_t)(byte & 255u);
    handled = true;
done:
    if (buf.l) ssrnx_jni_delete_local_ref_host(ctx->env, buf.l);
    (void)ssrnx_jni_monitor_exit_host(ctx->env, self);
    return handled;
}

/* BufferedReader uses Reader.lock (usually the underlying Reader), not this.
 * Only consume a cached character without skipLF; refill, CRLF handling,
 * close, mark/reset and overridden read methods stay in the Java body. */
static bool dex_vm_dex_buffered_char_read_cached(dex_vm_DexExecCtx *ctx, const SsrnxDexMethodImage *m,
                                           const uint64_t *words, size_t count,
                                           SsrnxJniValue *out) {
    static const char owner[] = "java/io/BufferedReader";
    if (count != 1 || !words[0] || strcmp(m->method_name, "read") != 0 ||
        strcmp(m->signature, "()I") != 0 || strcmp(m->declaring_class, owner) != 0)
        return false;
    void *self = (void *)(uintptr_t)words[0];
    SsrnxJniValue lock = {0}, pos = {0}, end = {0}, buf = {0}, skip = {0}, in = {0};
    bool found = false, handled = false;
    if (ssrnx_jni_get_instance_field_value(ctx->env, self, owner, "lock", "Ljava/lang/Object;", &lock, &found) != SSRNX_OK ||
        !found || !lock.l) return false;
    if (ssrnx_jni_monitor_enter_host(ctx->env, lock.l) != SSRNX_JNI_OK) goto release;
    if (ssrnx_jni_get_instance_field_value(ctx->env, self, owner, "in", "Ljava/io/Reader;", &in, &found) != SSRNX_OK ||
        !found || !in.l) goto done;
    if (ssrnx_jni_get_instance_field_value(ctx->env, self, owner, "skipLF", "Z", &skip, &found) != SSRNX_OK ||
        !found || skip.z) goto done;
    if (ssrnx_jni_get_instance_field_value(ctx->env, self, owner, "nextChar", "I", &pos, &found) != SSRNX_OK || !found)
        goto done;
    if (ssrnx_jni_get_instance_field_value(ctx->env, self, owner, "nChars", "I", &end, &found) != SSRNX_OK || !found ||
        pos.i < 0 || pos.i >= end.i) goto done;
    if (ssrnx_jni_get_instance_field_value(ctx->env, self, owner, "cb", "[C", &buf, &found) != SSRNX_OK || !found || !buf.l)
        goto done;
    uint64_t ch = 0;
    if (ssrnx_jni_primitive_array_get_host(buf.l, 'C', pos.i, &ch) != SSRNX_OK) goto done;
    ++pos.i;
    if (ssrnx_jni_set_instance_field_value_for_class(self, owner, "nextChar", "I", pos, &found) != SSRNX_OK || !found)
        goto done;
    out->i = (int32_t)(ch & 65535u);
    handled = true;
done:
    if (in.l) ssrnx_jni_delete_local_ref_host(ctx->env, in.l);
    if (buf.l) ssrnx_jni_delete_local_ref_host(ctx->env, buf.l);
    (void)ssrnx_jni_monitor_exit_host(ctx->env, lock.l);
release:
    ssrnx_jni_delete_local_ref_host(ctx->env, lock.l);
    return handled;
}



static SsrnxResult dex_vm_execute_image_body(dex_vm_DexExecCtx *ctx, SsrnxDexMethodImage *m,
                                 const uint64_t *incoming_words, size_t incoming_count,
                                 unsigned depth, SsrnxJniValue *out_result) {
    if (!ctx || !m || !out_result || depth > DEX_MAX_DEPTH) return SSRNX_ERR_APK_UNSUPPORTED;
    if (!m->code_offset || !m->insns) {
        char detail[512];
        snprintf(detail, sizeof(detail), "method has no code access=%#x%s", m->access_flags,
                 (m->access_flags & DEX_ACC_NATIVE) ? " native" : "");
        dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, 0, 0, detail);
        return SSRNX_ERR_APK_UNSUPPORTED;
    }
    if (m->registers_size > DEX_MAX_REGS || incoming_count != m->ins_size || m->ins_size > m->registers_size) {
        dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, 0, 0, "invalid DEX register/ins layout");
        return SSRNX_ERR_APK_INVALID;
    }

    /* Resolve and initialize the real package method before adapting its
     * platform capability; JNI, reflection and direct DEX share this path. */


    if (dex_vm_dex_buffered_read_cached(ctx, m, incoming_words, incoming_count, out_result) ||
        dex_vm_dex_buffered_char_read_cached(ctx, m, incoming_words, incoming_count, out_result))
        return SSRNX_OK;

    /* Exactly the method's register file: every register operand is checked
     * against registers_size before use. */
    const size_t reg_slots = m->registers_size ? m->registers_size : 1u;
    uint64_t regs[reg_slots];
    memset(regs, 0, reg_slots * sizeof(regs[0]));
    bool ref_regs[reg_slots];
    memset(ref_regs, 0, sizeof(ref_regs));
    if (!dex_vm_dex_init_reference_regs(m, ref_regs)) {
        dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, 0, 0,
                    "invalid DEX parameter register layout");
        return SSRNX_ERR_APK_INVALID;
    }
    const size_t in_base = (size_t)m->registers_size - m->ins_size;
    for (size_t i = 0; i < incoming_count; ++i) regs[in_base + i] = incoming_words[i];
    SsrnxJniValue pending = {0};
    bool have_pending = false;
    char pending_type = 'V';
    void *caught_exception = NULL;
    bool have_caught_exception = false;
    uint32_t pc = 0;
    bool have_previous = false;
    uint32_t previous_pc = 0;
    uint8_t previous_op = 0;
    if (!ctx->start_ms) ctx->start_ms = dex_vm_dex_monotonic_ms();
    if (!ctx->next_progress_steps) ctx->next_progress_steps = DEX_SOFT_STEP_DIAG;

    while (pc < m->insns_size) {
        ++ctx->steps;
        const uint8_t watchdog_op = (uint8_t)(m->insns[pc] & 0xffu);
        if ((ctx->steps & DEX_WATCHDOG_POLL_MASK) == 0u)
            dex_vm_dex_runtime_progress_publish(ctx, m, pc, watchdog_op);
        if (ctx->steps >= ctx->next_progress_steps) {
            dex_vm_dex_log_long_run(ctx, m, pc, watchdog_op);
            if (ctx->steps == DEX_SOFT_STEP_DIAG)
                dex_vm_dex_log_state_window(m, regs, pc, "long-run-entry");
            /* ART permits arbitrarily long Java work (including byte-at-a-time
             * downloads and worker loops). Instruction counts are diagnostics,
             * not a process lifetime limit. Keep shutdown polling below. */
            if (ctx->next_progress_steps <= SIZE_MAX / 2u) ctx->next_progress_steps *= 2u;
            else ctx->next_progress_steps = SIZE_MAX;
        }
        if ((ctx->steps & DEX_WATCHDOG_POLL_MASK) == 0u && dex_vm_dex_process_exiting(ctx, m, pc, watchdog_op))
            return SSRNX_ERR_APK_UNSUPPORTED;
        const uint16_t u0 = m->insns[pc];
        const uint8_t op = (uint8_t)(u0 & 0xffu);
        const uint8_t hi = (uint8_t)(u0 >> 8);
        const uint32_t executing_pc = pc;
        const char *throw_class = NULL;
        char throw_msg[96];
        throw_msg[0] = 0;
        int8_t loop_rel = 0;
        uint32_t loop_goto_pc = 0;
        if (dex_vm_dex_literal_goto_cycle(m, pc, &loop_rel, &loop_goto_pc)) {
            dex_vm_dex_log_cycle_state(m, regs, pc, have_previous, previous_pc, previous_op,
                                loop_rel, loop_goto_pc);
            char detail[256];
            snprintf(detail, sizeof(detail),
                     "non-terminating DEX literal/goto cycle pc=%#x goto=%#x rel=%d predecessor=%s",
                     pc, loop_goto_pc, (int)loop_rel, have_previous ? "logged" : "entry");
            dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, pc, op, detail);
            return SSRNX_ERR_APK_UNSUPPORTED;
        }
        switch (op) {
            case 0x00: pc += 1; break; /* nop */
            case 0x01: case 0x07: { /* move/move-object vA,vB */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if (a >= m->registers_size || b >= m->registers_size) goto bad_reg;
                regs[a] = regs[b]; pc += 1; break;
            }
            case 0x04: { /* move-wide vA,vB */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if ((uint32_t)a + 1u >= m->registers_size || (uint32_t)b + 1u >= m->registers_size) goto bad_reg;
                const uint64_t bits = regs[b];
                dex_vm_dex_write_wide(regs, a, bits); pc += 1; break;
            }
            case 0x02: case 0x08: { /* move/from16 */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi; const uint16_t b = m->insns[pc + 1];
                if (a >= m->registers_size || b >= m->registers_size) goto bad_reg;
                regs[a] = regs[b]; pc += 2; break;
            }
            case 0x05: { /* move-wide/from16 */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi; const uint16_t b = m->insns[pc + 1];
                if ((uint32_t)a + 1u >= m->registers_size || (uint32_t)b + 1u >= m->registers_size) goto bad_reg;
                const uint64_t bits = regs[b];
                dex_vm_dex_write_wide(regs, a, bits); pc += 2; break;
            }
            case 0x03: case 0x09: { /* move/16 */
                if (pc + 2 >= m->insns_size) goto truncated;
                const uint16_t a = m->insns[pc + 1], b = m->insns[pc + 2];
                if (a >= m->registers_size || b >= m->registers_size) goto bad_reg;
                regs[a] = regs[b]; pc += 3; break;
            }
            case 0x06: { /* move-wide/16 */
                if (pc + 2 >= m->insns_size) goto truncated;
                const uint16_t a = m->insns[pc + 1], b = m->insns[pc + 2];
                if ((uint32_t)a + 1u >= m->registers_size || (uint32_t)b + 1u >= m->registers_size) goto bad_reg;
                const uint64_t bits = regs[b];
                dex_vm_dex_write_wide(regs, a, bits); pc += 3; break;
            }
            case 0x0a: case 0x0c: { /* move-result/move-result-object */
                const uint8_t a = hi;
                if (a >= m->registers_size || !have_pending) goto bad_reg;
                regs[a] = op == 0x0a ? dex_vm_dex_canonical_result(pending_type, pending) : (uint64_t)pending.j;
                have_pending = false; pc += 1; break;
            }
            case 0x0b: { /* move-result-wide */
                const uint8_t a = hi;
                if ((uint32_t)a + 1u >= m->registers_size || !have_pending) goto bad_reg;
                dex_vm_dex_write_wide(regs, a, (uint64_t)pending.j); have_pending = false; pc += 1; break;
            }
            case 0x0d: { /* move-exception */
                const uint8_t a = hi;
                if (a >= m->registers_size || !have_caught_exception || !caught_exception) goto bad_reg;
                regs[a] = (uint64_t)(uintptr_t)caught_exception;
                caught_exception = NULL;
                have_caught_exception = false;
                pc += 1;
                break;
            }
            case 0x0e: /* return-void */
                memset(out_result, 0, sizeof(*out_result)); return SSRNX_OK;
            case 0x0f: case 0x11: { /* return/return-object */
                const uint8_t a = hi; if (a >= m->registers_size) goto bad_reg;
                memset(out_result, 0, sizeof(*out_result)); out_result->j = (int64_t)regs[a]; return SSRNX_OK;
            }
            case 0x10: { /* return-wide */
                const uint8_t a = hi; if ((uint32_t)a + 1u >= m->registers_size) goto bad_reg;
                memset(out_result, 0, sizeof(*out_result)); out_result->j = (int64_t)regs[a]; return SSRNX_OK;
            }
            case 0x12: { /* const/4 */
                const uint8_t a = hi & 0x0f; int8_t lit = (int8_t)hi >> 4;
                if (a >= m->registers_size) goto bad_reg;
                regs[a] = (uint64_t)(int64_t)lit;
                pc += 1;
                break;
            }
            case 0x13: { /* const/16 */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if (a >= m->registers_size) goto bad_reg;
                regs[a] = (uint64_t)(int64_t)(int16_t)m->insns[pc + 1];
                pc += 2;
                break;
            }
            case 0x14: { /* const */
                if (pc + 2 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if (a >= m->registers_size) goto bad_reg;
                const uint32_t v = (uint32_t)m->insns[pc + 1] | ((uint32_t)m->insns[pc + 2] << 16);
                regs[a] = (uint64_t)(int64_t)(int32_t)v; pc += 3; break;
            }
            case 0x15: { /* const/high16 */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if (a >= m->registers_size) goto bad_reg;
                regs[a] = (uint64_t)(int64_t)(int32_t)((uint32_t)m->insns[pc + 1] << 16); pc += 2; break;
            }
            case 0x16: { /* const-wide/16 */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if ((uint32_t)a + 1u >= m->registers_size) goto bad_reg;
                dex_vm_dex_write_wide(regs, a, (uint64_t)(int64_t)(int16_t)m->insns[pc + 1]);
                pc += 2; break;
            }
            case 0x17: { /* const-wide/32 */
                if (pc + 2 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if ((uint32_t)a + 1u >= m->registers_size) goto bad_reg;
                const uint32_t lo = (uint32_t)m->insns[pc + 1] | ((uint32_t)m->insns[pc + 2] << 16);
                dex_vm_dex_write_wide(regs, a, (uint64_t)(int64_t)(int32_t)lo);
                pc += 3; break;
            }
            case 0x18: { /* const-wide */
                if (pc + 4 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if ((uint32_t)a + 1u >= m->registers_size) goto bad_reg;
                const uint64_t bits = (uint64_t)m->insns[pc + 1] |
                                      ((uint64_t)m->insns[pc + 2] << 16) |
                                      ((uint64_t)m->insns[pc + 3] << 32) |
                                      ((uint64_t)m->insns[pc + 4] << 48);
                dex_vm_dex_write_wide(regs, a, bits);
                pc += 5; break;
            }
            case 0x19: { /* const-wide/high16 */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if ((uint32_t)a + 1u >= m->registers_size) goto bad_reg;
                dex_vm_dex_write_wide(regs, a, (uint64_t)m->insns[pc + 1] << 48);
                pc += 2; break;
            }
            case 0x1a: { /* const-string */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if (a >= m->registers_size) goto bad_reg;
                const char *s = dex_vm_dex_string(m->dex_data, m->dex_size, m->insns[pc + 1]);
                if (!s) goto malformed;
                void *ref = dex_vm_dex_literal_string(ctx->env, s);
                if (!ref) goto runtime_fail;
                regs[a] = (uint64_t)(uintptr_t)ref; pc += 2; break;
            }
            case 0x1c: { /* const-class */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if (a >= m->registers_size) goto bad_reg;
                const char *descriptor = dex_vm_dex_type_desc(m->dex_data, m->dex_size, m->insns[pc + 1]);
                if (!descriptor) goto malformed;
                /* const-class addresses a DEX type descriptor, not a JNI
                 * FindClass name. Preserve arrays and primitive Class objects. */
                void *ref = ssrnx_jni_class_ref_for_descriptor_host(ctx->env, descriptor);
                if (!ref) goto runtime_fail;
                regs[a] = (uint64_t)(uintptr_t)ref; pc += 2; break;
            }
            case 0x1d: case 0x1e: { /* monitor-enter / monitor-exit vAA */
                const uint8_t a = hi;
                if (a >= m->registers_size) goto bad_reg;
                void *object_ref = (void *)(uintptr_t)regs[a];
                int32_t mrc = SSRNX_JNI_ERR;
                if (!object_ref) {
                    (void)ssrnx_jni_throw_new_by_name(ctx->env, "java/lang/NullPointerException",
                                                      op == 0x1d ? "monitor-enter" : "monitor-exit");
                } else if (op == 0x1d) {
                    /* Dalvik monitor-enter has no timeout. A wall-clock cutoff here
                     * changes synchronized semantics and rejects valid cold-start
                     * work on a slow interpreter, so use the JNI monitor contract. */
                    mrc = ssrnx_jni_monitor_enter_host(ctx->env, object_ref);
                } else {
                    mrc = ssrnx_jni_monitor_exit_host(ctx->env, object_ref);
                }
                if (mrc == SSRNX_JNI_OK && !ssrnx_jni_exception_pending_host(ctx->env)) {
                    pc += 1;
                    break;
                }
                void *ex = NULL; uint32_t handler_pc = 0;
                if (dex_vm_dex_route_pending_exception(ctx, m, executing_pc, &ex, &handler_pc)) {
                    if (have_caught_exception && caught_exception)
                        ssrnx_jni_delete_local_ref_host(ctx->env, caught_exception);
                    caught_exception = ex;
                    have_caught_exception = true;
                    have_pending = false;
                    pc = handler_pc;
                    break;
                }
                dex_vm_dex_report_uncaught_propagation(ctx, m, executing_pc, op,
                                                op == 0x1d ? "monitor-enter" : "monitor-exit");
                return SSRNX_ERR_APK_UNSUPPORTED;
            }
            case 0x1f: { /* check-cast vAA, type@BBBB (format 21c) */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if (a >= m->registers_size) goto bad_reg;
                char cls[512];
                if (!dex_vm_reference_type_ref(m, m->insns[pc + 1], cls, sizeof(cls))) goto malformed;
                void *object_ref = (void *)(uintptr_t)regs[a];
                /* null passes every check-cast. A non-null reference to an
                 * unresolvable type is an ART linkage error (catchable
                 * NoClassDefFoundError), never an interpreter abort. */
                if (!object_ref) { pc += 2; break; }
                void *clazz_ref = ssrnx_jni_find_class_ref(ctx->env, cls);
                if (!clazz_ref) {
                    ssrnx_jni_exception_clear_host(ctx->env);
                    throw_class = "java/lang/NoClassDefFoundError";
                    snprintf(throw_msg, sizeof(throw_msg), "%.95s", cls);
                    goto java_throw;
                }
                ssrnx_jni_delete_local_ref_host(ctx->env, clazz_ref);
                if (!ssrnx_jni_is_instance_of_class_host(object_ref, cls)) {
                    (void)ssrnx_jni_throw_new_by_name(ctx->env, "java/lang/ClassCastException", cls);
                    void *ex = NULL; uint32_t handler_pc = 0;
                    if (dex_vm_dex_route_pending_exception(ctx, m, executing_pc, &ex, &handler_pc)) {
                        if (have_caught_exception && caught_exception)
                            ssrnx_jni_delete_local_ref_host(ctx->env, caught_exception);
                        caught_exception = ex;
                        have_caught_exception = true;
                        have_pending = false;
                        pc = handler_pc;
                        break;
                    }
                    { char origin[560]; snprintf(origin, sizeof(origin), "check-cast %.540s", cls);
                      dex_vm_dex_report_uncaught_propagation(ctx, m, executing_pc, op, origin); }
                    return SSRNX_ERR_APK_UNSUPPORTED;
                }
                pc += 2;
                break;
            }
            case 0x20: { /* instance-of vA, vB, type@CCCC (format 22c) */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi & 0x0f;
                const uint8_t b = hi >> 4;
                if (a >= m->registers_size || b >= m->registers_size) goto bad_reg;
                char cls[512];
                if (!dex_vm_reference_type_ref(m, m->insns[pc + 1], cls, sizeof(cls))) goto malformed;
                void *object_ref = (void *)(uintptr_t)regs[b];
                if (object_ref) {
                    void *clazz_ref = ssrnx_jni_find_class_ref(ctx->env, cls);
                    if (!clazz_ref) {
                        ssrnx_jni_exception_clear_host(ctx->env);
                        throw_class = "java/lang/NoClassDefFoundError";
                        snprintf(throw_msg, sizeof(throw_msg), "%.95s", cls);
                        goto java_throw;
                    }
                    ssrnx_jni_delete_local_ref_host(ctx->env, clazz_ref);
                }
                regs[a] = object_ref && ssrnx_jni_is_instance_of_class_host(object_ref, cls) ? 1u : 0u;
                pc += 2;
                break;
            }
            case 0x27: { /* throw vAA */
                const uint8_t a = hi;
                if (a >= m->registers_size) goto bad_reg;
                void *throwable = (void *)(uintptr_t)regs[a];
                if (!throwable) {
                    (void)ssrnx_jni_throw_new_by_name(ctx->env, "java/lang/NullPointerException", "throw null");
                } else if (ssrnx_jni_throw_host(ctx->env, throwable) != SSRNX_OK) {
                    goto runtime_fail;
                }
                void *ex = NULL; uint32_t handler_pc = 0;
                if (dex_vm_dex_route_pending_exception(ctx, m, executing_pc, &ex, &handler_pc)) {
                    if (have_caught_exception && caught_exception)
                        ssrnx_jni_delete_local_ref_host(ctx->env, caught_exception);
                    caught_exception = ex;
                    have_caught_exception = true;
                    have_pending = false;
                    pc = handler_pc;
                    break;
                }
                dex_vm_dex_report_uncaught_propagation(ctx, m, executing_pc, op, "throw");
                return SSRNX_ERR_APK_UNSUPPORTED;
            }
            case 0x21: { /* array-length vA, vB (format 12x) */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if (a >= m->registers_size || b >= m->registers_size) goto bad_reg;
                void *array_ref = (void *)(uintptr_t)regs[b];
                if (!array_ref) {
                    throw_class = "java/lang/NullPointerException";
                    snprintf(throw_msg, sizeof(throw_msg), "array-length on a null array");
                    goto java_throw;
                }
                const int32_t length = ssrnx_jni_array_length_host(array_ref);
                if (length < 0) {
                    dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature,
                                pc, op, "array-length operand is not a modeled array");
                    return SSRNX_ERR_APK_UNSUPPORTED;
                }
                regs[a] = (uint64_t)(int64_t)length;
                pc += 1;
                break;
            }
            case 0x44: case 0x45: case 0x46: case 0x47: case 0x48: case 0x49: case 0x4a:
            case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f: case 0x50: case 0x51: {
                /* aget/aput family vAA, vBB, vCC (format 23x). Variant order within
                 * each group: plain (int/float), wide (long/double), object,
                 * boolean, byte, char, short. */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                const uint16_t packed = m->insns[pc + 1];
                const uint8_t b = (uint8_t)(packed & 0xffu);
                const uint8_t c = (uint8_t)(packed >> 8);
                const bool is_get = op <= 0x4a;
                const unsigned variant = (unsigned)op - (is_get ? 0x44u : 0x4bu);
                const bool wide = variant == 1u;
                if (a >= m->registers_size || (wide && (uint32_t)a + 1u >= m->registers_size) ||
                    b >= m->registers_size || c >= m->registers_size)
                    goto bad_reg;
                void *array_ref = (void *)(uintptr_t)regs[b];
                const int32_t index = (int32_t)regs[c];
                if (!array_ref) {
                    throw_class = "java/lang/NullPointerException";
                    snprintf(throw_msg, sizeof(throw_msg), "%s on a null array", is_get ? "aget" : "aput");
                    goto java_throw;
                }
                const int32_t length = ssrnx_jni_array_length_host(array_ref);
                if (length < 0) {
                    dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature,
                                pc, op, "aget/aput operand is not a modeled array");
                    return SSRNX_ERR_APK_UNSUPPORTED;
                }
                if (index < 0 || index >= length) {
                    throw_class = "java/lang/ArrayIndexOutOfBoundsException";
                    snprintf(throw_msg, sizeof(throw_msg), "length=%d; index=%d", length, index);
                    goto java_throw;
                }
                SsrnxResult arc;
                if (variant == 2u) {
                    if (is_get) {
                        void *element_ref = NULL;
                        arc = ssrnx_jni_get_object_array_element_host(ctx->env, array_ref, index, &element_ref);
                        if (arc == SSRNX_OK) regs[a] = (uint64_t)(uintptr_t)element_ref;
                    } else {
                        arc = ssrnx_jni_set_object_array_element_host(ctx->env, array_ref, index,
                                                                      (void *)(uintptr_t)regs[a]);
                    }
                } else {
                    /* The plain/wide variants serve int[]/float[] and long[]/double[]
                     * alike; the array's own element kind decides. */
                    static const char kVariantType[7] = { 'I', 'J', 'L', 'Z', 'B', 'C', 'S' };
                    const char type = kVariantType[variant];
                    const char alt = variant == 0u ? 'F' : variant == 1u ? 'D' : 0;
                    if (is_get) {
                        uint64_t bits = 0;
                        arc = ssrnx_jni_primitive_array_get_host(array_ref, type, index, &bits);
                        if (arc != SSRNX_OK && alt)
                            arc = ssrnx_jni_primitive_array_get_host(array_ref, alt, index, &bits);
                        if (arc == SSRNX_OK) {
                            if (wide) dex_vm_dex_write_wide(regs, a, bits);
                            else regs[a] = bits;
                        }
                    } else {
                        arc = ssrnx_jni_primitive_array_set_host(array_ref, type, index, regs[a]);
                        if (arc != SSRNX_OK && alt)
                            arc = ssrnx_jni_primitive_array_set_host(array_ref, alt, index, regs[a]);
                    }
                }
                if (arc != SSRNX_OK) {
                    dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, pc, op,
                                "aget/aput element type does not match the modeled array");
                    return SSRNX_ERR_APK_UNSUPPORTED;
                }
                pc += 2;
                break;
            }
            case 0x22: { /* new-instance */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if (a >= m->registers_size) goto bad_reg;
                char cls[512]; if (!dex_vm_type_ref(m, m->insns[pc + 1], cls, sizeof(cls))) goto malformed;
                SsrnxResult init_rc = dex_vm_ensure_dex_class_initialized(ctx, cls, depth + 1u);
                if (init_rc != SSRNX_OK) return init_rc;
                /* new-instance needs an object reference, not a temporary Class
                 * local. Resolve/allocation is already performed by the JNI host
                 * helper; avoiding the redundant FindClass prevents one leaked
                 * local reference per DEX allocation and removes a spurious
                 * allocation-failure point. */
                void *obj = ssrnx_jni_new_local_object(ctx->env, cls);
                if (!obj) {
                    char detail[640];
                    snprintf(detail, sizeof(detail), "new-instance allocation failed class=%.500s", cls);
                    dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, pc, op, detail);
                    return SSRNX_ERR_APK_UNSUPPORTED;
                }
                regs[a] = (uint64_t)(uintptr_t)obj; pc += 2; break;
            }
            case 0x24: { /* filled-new-array {vC..vG}, type@BBBB (format 35c) */
                if (pc + 2 >= m->insns_size) goto truncated;
                const uint8_t count = hi >> 4;
                const uint8_t g = hi & 0x0f;
                if (count > 5) goto malformed;
                const uint16_t type_idx = m->insns[pc + 1];
                const uint16_t packed = m->insns[pc + 2];
                const uint8_t r[5] = {
                    (uint8_t)(packed & 0x0fu),
                    (uint8_t)((packed >> 4) & 0x0fu),
                    (uint8_t)((packed >> 8) & 0x0fu),
                    (uint8_t)((packed >> 12) & 0x0fu),
                    g
                };
                uint64_t values[5] = {0};
                for (uint8_t i = 0; i < count; ++i) {
                    if (r[i] >= m->registers_size) goto bad_reg;
                    values[i] = regs[r[i]];
                }
                const char *descriptor = dex_vm_dex_type_desc(m->dex_data, m->dex_size, type_idx);
                if (!descriptor || descriptor[0] != '[') goto malformed;
                void *array_ref = NULL;
                const SsrnxResult arc = dex_vm_dex_make_filled_array(ctx, descriptor, values, count, &array_ref);
                if (arc != SSRNX_OK || !array_ref) {
                    char detail[640];
                    snprintf(detail, sizeof(detail), "filled-new-array type not modeled descriptor=%.500s",
                             descriptor ? descriptor : "<invalid>");
                    dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, pc, op, detail);
                    return arc != SSRNX_OK ? arc : SSRNX_ERR_APK_UNSUPPORTED;
                }
                memset(&pending, 0, sizeof(pending));
                pending.l = array_ref;
                pending_type = '[';
                have_pending = true;
                pc += 3;
                break;
            }
            case 0x25: { /* filled-new-array/range {vCCCC ..}, type@BBBB (format 3rc) */
                if (pc + 2 >= m->insns_size) goto truncated;
                const uint8_t count = hi;
                const uint16_t type_idx = m->insns[pc + 1];
                const uint16_t first = m->insns[pc + 2];
                if ((uint32_t)first + count > m->registers_size) goto bad_reg;
                const char *descriptor = dex_vm_dex_type_desc(m->dex_data, m->dex_size, type_idx);
                if (!descriptor || descriptor[0] != '[') goto malformed;
                void *array_ref = NULL;
                const SsrnxResult arc = dex_vm_dex_make_filled_array(ctx, descriptor, regs + first, count, &array_ref);
                if (arc != SSRNX_OK || !array_ref) {
                    char detail[640];
                    snprintf(detail, sizeof(detail), "filled-new-array/range type not modeled descriptor=%.500s",
                             descriptor ? descriptor : "<invalid>");
                    dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, pc, op, detail);
                    return arc != SSRNX_OK ? arc : SSRNX_ERR_APK_UNSUPPORTED;
                }
                memset(&pending, 0, sizeof(pending));
                pending.l = array_ref;
                pending_type = '[';
                have_pending = true;
                pc += 3;
                break;
            }
            case 0x23: { /* new-array vA, vB, type@CCCC (format 22c) */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if (a >= m->registers_size || b >= m->registers_size) goto bad_reg;
                const int32_t length = (int32_t)regs[b];
                const char *array_descriptor = dex_vm_dex_type_desc(
                    m->dex_data, m->dex_size, m->insns[pc + 1]);
                if (!array_descriptor || array_descriptor[0] != '[' || array_descriptor[1] == '\0')
                    goto malformed;
                char descriptor[512];
                if (!dex_vm_copy_text(descriptor, sizeof(descriptor), array_descriptor)) goto malformed;
                if (length < 0) {
                    throw_class = "java/lang/NegativeArraySizeException";
                    snprintf(throw_msg, sizeof(throw_msg), "%d", length);
                    goto java_throw;
                }
                void *array_ref = NULL;
                if (descriptor[1] == 'L' || descriptor[1] == '[')
                    array_ref = ssrnx_jni_new_local_reference_array(ctx->env, descriptor,
                                                                    length);
                else if (strcmp(descriptor, "[Z") == 0)
                    array_ref = ssrnx_jni_new_local_boolean_array(ctx->env, length);
                else if (strcmp(descriptor, "[B") == 0)
                    array_ref = ssrnx_jni_new_local_byte_array(ctx->env, length);
                else if (strcmp(descriptor, "[C") == 0)
                    array_ref = ssrnx_jni_new_local_char_array(ctx->env, length);
                else if (strcmp(descriptor, "[S") == 0)
                    array_ref = ssrnx_jni_new_local_short_array(ctx->env, length);
                else if (strcmp(descriptor, "[I") == 0)
                    array_ref = ssrnx_jni_new_local_int_array(ctx->env, length);
                else if (strcmp(descriptor, "[J") == 0)
                    array_ref = ssrnx_jni_new_local_long_array(ctx->env, length);
                else if (strcmp(descriptor, "[F") == 0)
                    array_ref = ssrnx_jni_new_local_float_array(ctx->env, length);
                else if (strcmp(descriptor, "[D") == 0)
                    array_ref = ssrnx_jni_new_local_double_array(ctx->env, length);
                else {
                    char detail[640];
                    snprintf(detail, sizeof(detail), "new-array type not modeled descriptor=%.500s", descriptor);
                    dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, pc, op, detail);
                    return SSRNX_ERR_APK_UNSUPPORTED;
                }
                if (!array_ref) goto runtime_fail;
                regs[a] = (uint64_t)(uintptr_t)array_ref;
                pc += 2;
                break;
            }
            case 0x28: { /* goto */
                const int8_t rel = (int8_t)hi; const int64_t np = (int64_t)pc + rel;
                if (np < 0 || (uint64_t)np >= m->insns_size) goto malformed;
                pc = (uint32_t)np;
                break;
            }
            case 0x29: { /* goto/16 */
                if (pc + 1 >= m->insns_size) goto truncated;
                const int16_t rel = (int16_t)m->insns[pc + 1];
                const int64_t np = (int64_t)pc + rel; if (np < 0 || (uint64_t)np >= m->insns_size) goto malformed;
                pc = (uint32_t)np;
                break;
            }
            case 0x2a: { /* goto/32 */
                if (pc + 2 >= m->insns_size) goto truncated;
                const int32_t rel = (int32_t)((uint32_t)m->insns[pc + 1] | ((uint32_t)m->insns[pc + 2] << 16));
                const int64_t np = (int64_t)pc + rel; if (np < 0 || (uint64_t)np >= m->insns_size) goto malformed;
                pc = (uint32_t)np;
                break;
            }
            case 0x31: { /* cmp-long vAA, vBB, vCC */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                const uint16_t packed = m->insns[pc + 1];
                const uint8_t b = (uint8_t)(packed & 0xffu), c = (uint8_t)(packed >> 8);
                if (a >= m->registers_size || (uint32_t)b + 1u >= m->registers_size ||
                    (uint32_t)c + 1u >= m->registers_size) goto bad_reg;
                const int64_t lhs = (int64_t)regs[b], rhs = (int64_t)regs[c];
                regs[a] = (uint64_t)(int64_t)(lhs < rhs ? -1 : (lhs > rhs ? 1 : 0));
                pc += 2; break;
            }
            case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37: { /* if-test */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi & 0x0f, b = hi >> 4; if (a >= m->registers_size || b >= m->registers_size) goto bad_reg;
                const int16_t rel = (int16_t)m->insns[pc + 1]; const int64_t av=(int64_t)regs[a], bv=(int64_t)regs[b]; bool take=false;
                if(op==0x32||op==0x33){
                    /* Fresh JNI handles compare by referent; primitive
                     * registers compare by value even at live ref addresses. */
                    bool same=av==bv;
                    if(!same&&ref_regs[a]&&ref_regs[b]){bool id=false;if(ssrnx_jni_ref_values_identity((uint64_t)av,(uint64_t)bv,&id))same=id;}
                    take=op==0x32?same:!same;
                } else if(op==0x34)take=av<bv; else if(op==0x35)take=av>=bv; else if(op==0x36)take=av>bv; else take=av<=bv;
                if(take){const int64_t np=(int64_t)pc+rel;if(np<0||(uint64_t)np>=m->insns_size)goto malformed;pc=(uint32_t)np;}else pc+=2; break;
            }
            case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: { /* if-testz */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if (a >= m->registers_size) goto bad_reg;
                const int16_t rel=(int16_t)m->insns[pc+1];const int64_t av=(int64_t)regs[a];bool take=false;
                if(op==0x38)take=av==0;else if(op==0x39)take=av!=0;else if(op==0x3a)take=av<0;else if(op==0x3b)take=av>=0;else if(op==0x3c)take=av>0;else take=av<=0;
                if(take){const int64_t np=(int64_t)pc+rel;if(np<0||(uint64_t)np>=m->insns_size)goto malformed;pc=(uint32_t)np;}else pc+=2;break;
            }
            case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: case 0x58:
            case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f: { /* iget/iput 22c */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if (a >= m->registers_size || b >= m->registers_size) goto bad_reg;
                const bool is_get = op <= 0x58;
                const bool is_wide = op == 0x53 || op == 0x5a;
                if (is_wide && (uint32_t)a + 1u >= m->registers_size) goto bad_reg;
                void *object_ref = (void *)(uintptr_t)regs[b];
                if (!object_ref) {
                    /* JLS 15.11.1 / 15.26.1: field access on null throws NPE. */
                    throw_class = "java/lang/NullPointerException";
                    snprintf(throw_msg, sizeof(throw_msg), "Attempt to %s field on a null object reference",
                             is_get ? "read from" : "write to");
                    goto java_throw;
                }
                char cls[512], name[256], sig[DEX_FIELD_TYPE_CAP];
                if (!field_ref(m, m->insns[pc + 1], cls, sizeof(cls), name, sizeof(name),
                               sig, sizeof(sig))) goto malformed;
                bool available = false;
                SsrnxResult frc = dex_vm_ensure_dex_instance_field(ctx, object_ref, cls, name, sig,
                                                             &available);
                if (frc != SSRNX_OK) { if (ssrnx_jni_exception_pending_host(ctx->env)) { throw_class = NULL; goto java_pending; } return frc; }
                if (!available) {
                    dex_report_unresolved_field(ctx, m, pc, op, false, cls, name, sig);
                    if (ssrnx_jni_exception_pending_host(ctx->env)) { throw_class = NULL; goto java_pending; }
                    return SSRNX_ERR_APK_UNSUPPORTED;
                }
                if (is_get) {
                    SsrnxJniValue value = {0}; bool found = false;
                    frc = ssrnx_jni_get_instance_field_value(ctx->env, object_ref, cls, name, sig,
                                                              &value, &found);
                    if (frc != SSRNX_OK) { if (ssrnx_jni_exception_pending_host(ctx->env)) { throw_class = NULL; goto java_pending; } return frc; }
                    if (!found) goto runtime_fail;
                    if (op == 0x52) {
                        if (sig[0] == 'F') {
                            uint32_t bits = 0; memcpy(&bits, &value.f, sizeof(bits)); regs[a] = bits;
                        } else regs[a] = (uint64_t)(int64_t)value.i;
                    } else if (op == 0x53) {
                        uint64_t bits = 0;
                        if (sig[0] == 'D') memcpy(&bits, &value.d, sizeof(bits));
                        else bits = (uint64_t)value.j;
                        regs[a] = bits; regs[a + 1u] = bits >> 32;
                    } else if (op == 0x54) {
                        regs[a] = (uint64_t)(uintptr_t)value.l;
                    } else if (op == 0x55) regs[a] = value.z ? 1u : 0u;
                    else if (op == 0x56) regs[a] = (uint64_t)(int64_t)(int8_t)value.b;
                    else if (op == 0x57) regs[a] = (uint64_t)value.c;
                    else regs[a] = (uint64_t)(int64_t)(int16_t)value.s;
                } else {
                    SsrnxJniValue value = {0};
                    if (op == 0x59) {
                        if (sig[0] == 'F') { uint32_t bits = (uint32_t)regs[a]; memcpy(&value.f, &bits, sizeof(bits)); }
                        else value.i = (int32_t)regs[a];
                    } else if (op == 0x5a) {
                        if (sig[0] == 'D') { uint64_t bits = regs[a]; memcpy(&value.d, &bits, sizeof(bits)); }
                        else value.j = (int64_t)regs[a];
                    } else if (op == 0x5b) value.l = (void *)(uintptr_t)regs[a];
                    else if (op == 0x5c) value.z = regs[a] ? 1u : 0u;
                    else if (op == 0x5d) value.b = (int8_t)regs[a];
                    else if (op == 0x5e) value.c = (uint16_t)regs[a];
                    else value.s = (int16_t)regs[a];
                    bool found = false;
                    frc = ssrnx_jni_set_instance_field_value_for_class(object_ref, cls, name, sig,
                                                                         value, &found);
                    if (frc != SSRNX_OK) { if (ssrnx_jni_exception_pending_host(ctx->env)) { throw_class = NULL; goto java_pending; } return frc; }
                    if (!found) goto runtime_fail;
                }
                pc += 2;
                break;
            }
            case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66:
            case 0x67: case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: { /* sget/sput 21c */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if (a >= m->registers_size) goto bad_reg;
                const bool is_get = op <= 0x66;
                const bool is_wide = op == 0x61 || op == 0x68;
                if (is_wide && (uint32_t)a + 1u >= m->registers_size) goto bad_reg;
                char cls[512], name[256], sig[DEX_FIELD_TYPE_CAP];
                if (!field_ref(m, m->insns[pc + 1], cls, sizeof(cls), name, sizeof(name), sig, sizeof(sig)))
                    goto malformed;
                bool available = false;
                SsrnxResult frc = dex_vm_ensure_dex_static_field(ctx, cls, sizeof(cls), name, sig, depth, &available);
                if (frc != SSRNX_OK) { if (ssrnx_jni_exception_pending_host(ctx->env)) { throw_class = NULL; goto java_pending; } return frc; }
                if (!available) {
                    dex_report_unresolved_field(ctx, m, pc, op, true, cls, name, sig);
                    if (ssrnx_jni_exception_pending_host(ctx->env)) { throw_class = NULL; goto java_pending; }
                    return SSRNX_ERR_APK_UNSUPPORTED;
                }

                if (is_get) {
                    SsrnxJniValue value = {0}; bool found = false;
                    frc = ssrnx_jni_get_static_field_value(ctx->env, cls, name, sig, &value, &found);
                    if (frc != SSRNX_OK) { if (ssrnx_jni_exception_pending_host(ctx->env)) { throw_class = NULL; goto java_pending; } return frc; }
                    if (!found) goto runtime_fail;
                    if (op == 0x60) regs[a] = (uint64_t)(int64_t)value.i;
                    else if (op == 0x61) { regs[a] = (uint64_t)value.j; regs[a + 1u] = (uint64_t)value.j >> 32; }
                    else if (op == 0x62) regs[a] = (uint64_t)(uintptr_t)value.l;
                    else if (op == 0x63) regs[a] = value.z ? 1u : 0u;
                    else if (op == 0x64) regs[a] = (uint64_t)(int64_t)(int8_t)value.b;
                    else if (op == 0x65) regs[a] = (uint64_t)value.c;
                    else regs[a] = (uint64_t)(int64_t)(int16_t)value.s;
                } else {
                    SsrnxJniValue value = {0};
                    if (op == 0x67) value.i = (int32_t)regs[a];
                    else if (op == 0x68) value.j = (int64_t)regs[a];
                    else if (op == 0x69) value.l = (void *)(uintptr_t)regs[a];
                    else if (op == 0x6a) value.z = regs[a] ? 1u : 0u;
                    else if (op == 0x6b) value.b = (int8_t)regs[a];
                    else if (op == 0x6c) value.c = (uint16_t)regs[a];
                    else value.s = (int16_t)regs[a];
                    frc = ssrnx_jni_set_static_field_value(cls, name, sig, value);
                    if (frc != SSRNX_OK) goto runtime_fail;
                }
                pc += 2;
                break;
            }
            case 0x7b: case 0x7c: case 0x7f: case 0x82: case 0x83:
            case 0x8d: case 0x8e: case 0x8f: { /* unary/conversion from a 32-bit source, 12x */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                const bool wide_dst = op == 0x83;
                if (b >= m->registers_size || a >= m->registers_size ||
                    (wide_dst && (uint32_t)a + 1u >= m->registers_size)) goto bad_reg;
                const int32_t v = (int32_t)regs[b];
                if (op == 0x7b) regs[a] = (uint64_t)(int64_t)(int32_t)(0u - (uint32_t)v);   /* neg-int */
                else if (op == 0x7c) regs[a] = (uint64_t)(int64_t)~v;                      /* not-int */
                else if (op == 0x7f) dex_vm_dex_write_float(regs, a, -dex_vm_dex_read_float(regs, b));  /* neg-float */
                else if (op == 0x82) dex_vm_dex_write_float(regs, a, (float)v);                   /* int-to-float */
                else if (op == 0x83) dex_vm_dex_write_double(regs, a, (double)v);                 /* int-to-double */
                else if (op == 0x8d) regs[a] = (uint64_t)(int64_t)(int8_t)v;               /* int-to-byte */
                else if (op == 0x8e) regs[a] = (uint64_t)(uint16_t)v;                      /* int-to-char */
                else regs[a] = (uint64_t)(int64_t)(int16_t)v;                              /* int-to-short */
                pc += 1; break;
            }
            case 0x81: { /* int-to-long */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if ((uint32_t)a + 1u >= m->registers_size || b >= m->registers_size) goto bad_reg;
                dex_vm_dex_write_wide(regs, a, (uint64_t)(int64_t)(int32_t)regs[b]);
                pc += 1; break;
            }
            case 0x7d: case 0x7e: case 0x80: case 0x84: case 0x85: case 0x86: { /* unary/conversion from a wide source, 12x */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                const bool wide_dst = op == 0x7d || op == 0x7e || op == 0x80 || op == 0x86;
                if ((uint32_t)b + 1u >= m->registers_size || a >= m->registers_size ||
                    (wide_dst && (uint32_t)a + 1u >= m->registers_size)) goto bad_reg;
                const int64_t v = (int64_t)regs[b];
                if (op == 0x7d) dex_vm_dex_write_wide(regs, a, 0u - (uint64_t)v);                 /* neg-long */
                else if (op == 0x7e) dex_vm_dex_write_wide(regs, a, ~(uint64_t)v);                /* not-long */
                else if (op == 0x80) dex_vm_dex_write_double(regs, a, -dex_vm_dex_read_double(regs, b)); /* neg-double */
                else if (op == 0x84) regs[a] = (uint64_t)(int64_t)(int32_t)(uint32_t)v;    /* long-to-int */
                else if (op == 0x85) dex_vm_dex_write_float(regs, a, (float)v);                   /* long-to-float */
                else dex_vm_dex_write_double(regs, a, (double)v);                                 /* long-to-double */
                pc += 1; break;
            }
            case 0x2d: case 0x2e: case 0x2f: case 0x30: { /* cmpl/cmpg-float/double 23x */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                const uint16_t packed = m->insns[pc + 1];
                const uint8_t b = (uint8_t)(packed & 0xffu), c = (uint8_t)(packed >> 8);
                const bool dbl = op >= 0x2f;
                if (a >= m->registers_size || b >= m->registers_size || c >= m->registers_size ||
                    (dbl && ((uint32_t)b + 1u >= m->registers_size || (uint32_t)c + 1u >= m->registers_size)))
                    goto bad_reg;
                const double lhs = dbl ? dex_vm_dex_read_double(regs, b) : (double)dex_vm_dex_read_float(regs, b);
                const double rhs = dbl ? dex_vm_dex_read_double(regs, c) : (double)dex_vm_dex_read_float(regs, c);
                int32_t result;
                if (lhs > rhs) result = 1;
                else if (lhs == rhs) result = 0;
                else if (lhs < rhs) result = -1;
                else result = (op == 0x2d || op == 0x2f) ? -1 : 1; /* NaN: cmpl biases -1, cmpg +1 */
                regs[a] = (uint64_t)(int64_t)result;
                pc += 2; break;
            }
            case 0x2b: case 0x2c: { /* packed-switch / sparse-switch vAA, +BBBBBBBB (31t) */
                if (pc + 2 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if (a >= m->registers_size) goto bad_reg;
                const int64_t tp = (int64_t)pc + dex_vm_dex_payload_i32(m->insns + pc + 1);
                if (tp < 0 || (uint64_t)tp + 2u > m->insns_size) goto malformed;
                const uint16_t *t = m->insns + tp;
                const uint32_t size = t[1];
                const int32_t key = (int32_t)regs[a];
                int64_t target = -1;
                if (op == 0x2b) {
                    if (t[0] != 0x0100u || (uint64_t)tp + 4u + 2u * size > m->insns_size) goto malformed;
                    const int64_t slot = (int64_t)key - dex_vm_dex_payload_i32(t + 2);
                    if (slot >= 0 && slot < (int64_t)size)
                        target = (int64_t)pc + dex_vm_dex_payload_i32(t + 4 + 2 * slot);
                } else {
                    if (t[0] != 0x0200u || (uint64_t)tp + 2u + 4u * size > m->insns_size) goto malformed;
                    for (uint32_t i = 0; i < size; ++i) {
                        if (dex_vm_dex_payload_i32(t + 2 + 2 * i) == key) {
                            target = (int64_t)pc + dex_vm_dex_payload_i32(t + 2 + 2 * size + 2 * i);
                            break;
                        }
                    }
                }
                if (target < 0) { pc += 3; break; }
                if ((uint64_t)target >= m->insns_size) goto malformed;
                pc = (uint32_t)target;
                break;
            }
            case 0x26: { /* fill-array-data vAA, +BBBBBBBB (31t) */
                if (pc + 2 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if (a >= m->registers_size) goto bad_reg;
                const int64_t tp = (int64_t)pc + dex_vm_dex_payload_i32(m->insns + pc + 1);
                if (tp < 0 || (uint64_t)tp + 4u > m->insns_size) goto malformed;
                const uint16_t *t = m->insns + tp;
                const uint32_t width = t[1];
                const uint32_t count = (uint32_t)dex_vm_dex_payload_i32(t + 2);
                if (t[0] != 0x0300u || (width != 1u && width != 2u && width != 4u && width != 8u) ||
                    (uint64_t)tp + 4u + ((uint64_t)width * count + 1u) / 2u > m->insns_size) goto malformed;
                void *array_ref = (void *)(uintptr_t)regs[a];
                if (!array_ref) {
                    throw_class = "java/lang/NullPointerException";
                    snprintf(throw_msg, sizeof(throw_msg), "fill-array-data on a null array");
                    goto java_throw;
                }
                const int32_t length = ssrnx_jni_array_length_host(array_ref);
                if (length < 0 || count > (uint32_t)length) {
                    throw_class = "java/lang/ArrayIndexOutOfBoundsException";
                    snprintf(throw_msg, sizeof(throw_msg), "length=%d; fill=%u", length, count);
                    goto java_throw;
                }
                /* Payload width fixes the element size; the array kind picks
                 * between the two Java types of that size. */
                static const char kWidthTypes[9][2] = {
                    {0, 0}, {'B', 'Z'}, {'C', 'S'}, {0, 0}, {'I', 'F'}, {0, 0}, {0, 0}, {0, 0}, {'J', 'D'}
                };
                const uint8_t *data = (const uint8_t *)(t + 4);
                char type = 0;
                for (uint32_t i = 0; i < count; ++i) {
                    uint64_t bits = 0;
                    for (uint32_t k = 0; k < width; ++k) bits |= (uint64_t)data[i * width + k] << (8u * k);
                    if (!type) {
                        type = kWidthTypes[width][0];
                        if (ssrnx_jni_primitive_array_set_host(array_ref, type, (int32_t)i, bits) == SSRNX_OK) continue;
                        type = kWidthTypes[width][1];
                    }
                    if (ssrnx_jni_primitive_array_set_host(array_ref, type, (int32_t)i, bits) != SSRNX_OK) {
                        dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, pc, op,
                                    "fill-array-data width does not match the modeled array");
                        return SSRNX_ERR_APK_UNSUPPORTED;
                    }
                }
                pc += 3; break;
            }
            case 0x1b: { /* const-string/jumbo vAA, string@BBBBBBBB (31c) */
                if (pc + 2 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                if (a >= m->registers_size) goto bad_reg;
                const char *s = dex_vm_dex_string(m->dex_data, m->dex_size, (uint32_t)dex_vm_dex_payload_i32(m->insns + pc + 1));
                if (!s) goto malformed;
                void *ref = dex_vm_dex_literal_string(ctx->env, s);
                if (!ref) goto runtime_fail;
                regs[a] = (uint64_t)(uintptr_t)ref; pc += 3; break;
            }
            case 0x87: { /* float-to-int */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if (a >= m->registers_size || b >= m->registers_size) goto bad_reg;
                regs[a] = (uint64_t)(int64_t)dex_vm_dex_java_d2i((double)dex_vm_dex_read_float(regs, b));
                pc += 1; break;
            }
            case 0x88: { /* float-to-long */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if ((uint32_t)a + 1u >= m->registers_size || b >= m->registers_size) goto bad_reg;
                dex_vm_dex_write_wide(regs, a, (uint64_t)dex_vm_dex_java_d2l((double)dex_vm_dex_read_float(regs, b)));
                pc += 1; break;
            }
            case 0x89: { /* float-to-double */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if ((uint32_t)a + 1u >= m->registers_size || b >= m->registers_size) goto bad_reg;
                dex_vm_dex_write_double(regs, a, (double)dex_vm_dex_read_float(regs, b));
                pc += 1; break;
            }
            case 0x8a: { /* double-to-int */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if (a >= m->registers_size || (uint32_t)b + 1u >= m->registers_size) goto bad_reg;
                regs[a] = (uint64_t)(int64_t)dex_vm_dex_java_d2i(dex_vm_dex_read_double(regs, b));
                pc += 1; break;
            }
            case 0x8b: { /* double-to-long */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if ((uint32_t)a + 1u >= m->registers_size || (uint32_t)b + 1u >= m->registers_size) goto bad_reg;
                dex_vm_dex_write_wide(regs, a, (uint64_t)dex_vm_dex_java_d2l(dex_vm_dex_read_double(regs, b)));
                pc += 1; break;
            }
            case 0x8c: { /* double-to-float */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if (a >= m->registers_size || (uint32_t)b + 1u >= m->registers_size) goto bad_reg;
                dex_vm_dex_write_float(regs, a, (float)dex_vm_dex_read_double(regs, b));
                pc += 1; break;
            }
            case 0xa6: case 0xa7: case 0xa8: case 0xa9: case 0xaa: { /* float binop 23x */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                const uint16_t packed = m->insns[pc + 1];
                const uint8_t b = (uint8_t)(packed & 0xffu), c = (uint8_t)(packed >> 8);
                if (a >= m->registers_size || b >= m->registers_size || c >= m->registers_size) goto bad_reg;
                const float lhs = dex_vm_dex_read_float(regs, b), rhs = dex_vm_dex_read_float(regs, c);
                float result = 0.0f;
                if (op == 0xa6) result = lhs + rhs;
                else if (op == 0xa7) result = lhs - rhs;
                else if (op == 0xa8) result = lhs * rhs;
                else if (op == 0xa9) result = lhs / rhs;
                else result = fmodf(lhs, rhs);
                dex_vm_dex_write_float(regs, a, result);
                pc += 2; break;
            }
            case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf: { /* double binop 23x */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                const uint16_t packed = m->insns[pc + 1];
                const uint8_t b = (uint8_t)(packed & 0xffu), c = (uint8_t)(packed >> 8);
                if ((uint32_t)a + 1u >= m->registers_size || (uint32_t)b + 1u >= m->registers_size ||
                    (uint32_t)c + 1u >= m->registers_size) goto bad_reg;
                const double lhs = dex_vm_dex_read_double(regs, b), rhs = dex_vm_dex_read_double(regs, c);
                double result = 0.0;
                if (op == 0xab) result = lhs + rhs;
                else if (op == 0xac) result = lhs - rhs;
                else if (op == 0xad) result = lhs * rhs;
                else if (op == 0xae) result = lhs / rhs;
                else result = fmod(lhs, rhs);
                dex_vm_dex_write_double(regs, a, result);
                pc += 2; break;
            }
            case 0xc6: case 0xc7: case 0xc8: case 0xc9: case 0xca: { /* float binop/2addr */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if (a >= m->registers_size || b >= m->registers_size) goto bad_reg;
                const float lhs = dex_vm_dex_read_float(regs, a), rhs = dex_vm_dex_read_float(regs, b);
                float result = 0.0f;
                if (op == 0xc6) result = lhs + rhs;
                else if (op == 0xc7) result = lhs - rhs;
                else if (op == 0xc8) result = lhs * rhs;
                else if (op == 0xc9) result = lhs / rhs;
                else result = fmodf(lhs, rhs);
                dex_vm_dex_write_float(regs, a, result);
                pc += 1; break;
            }
            case 0xcb: case 0xcc: case 0xcd: case 0xce: case 0xcf: { /* double binop/2addr */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if ((uint32_t)a + 1u >= m->registers_size || (uint32_t)b + 1u >= m->registers_size) goto bad_reg;
                const double lhs = dex_vm_dex_read_double(regs, a), rhs = dex_vm_dex_read_double(regs, b);
                double result = 0.0;
                if (op == 0xcb) result = lhs + rhs;
                else if (op == 0xcc) result = lhs - rhs;
                else if (op == 0xcd) result = lhs * rhs;
                else if (op == 0xce) result = lhs / rhs;
                else result = fmod(lhs, rhs);
                dex_vm_dex_write_double(regs, a, result);
                pc += 1; break;
            }
            case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95:
            case 0x96: case 0x97: case 0x98: case 0x99: case 0x9a: { /* int binop 23x */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                const uint16_t packed = m->insns[pc + 1];
                const uint8_t b = (uint8_t)(packed & 0xffu), c = (uint8_t)(packed >> 8);
                if (a >= m->registers_size || b >= m->registers_size || c >= m->registers_size) goto bad_reg;
                const int32_t lhs = (int32_t)regs[b], rhs = (int32_t)regs[c];
                int32_t result = 0;
                if (op == 0x90) result = (int32_t)((uint32_t)lhs + (uint32_t)rhs);
                else if (op == 0x91) result = (int32_t)((uint32_t)lhs - (uint32_t)rhs);
                else if (op == 0x92) result = (int32_t)((uint32_t)lhs * (uint32_t)rhs);
                else if (op == 0x93) {
                    if (!dex_vm_dex_int_div(lhs, rhs, &result)) { throw_class = "java/lang/ArithmeticException"; snprintf(throw_msg, sizeof(throw_msg), "/ by zero"); goto java_throw; }
                } else if (op == 0x94) {
                    if (!dex_vm_dex_int_rem(lhs, rhs, &result)) { throw_class = "java/lang/ArithmeticException"; snprintf(throw_msg, sizeof(throw_msg), "/ by zero"); goto java_throw; }
                } else if (op == 0x95) result = lhs & rhs;
                else if (op == 0x96) result = lhs | rhs;
                else if (op == 0x97) result = lhs ^ rhs;
                else if (op == 0x98) result = (int32_t)((uint32_t)lhs << ((uint32_t)rhs & 31u));
                else if (op == 0x99) result = lhs >> ((uint32_t)rhs & 31u);
                else result = (int32_t)((uint32_t)lhs >> ((uint32_t)rhs & 31u));
                regs[a] = (uint64_t)(int64_t)result; pc += 2; break;
            }
            case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f: case 0xa0:
            case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: { /* long binop 23x */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                const uint16_t packed = m->insns[pc + 1];
                const uint8_t b = (uint8_t)(packed & 0xffu), c = (uint8_t)(packed >> 8);
                if ((uint32_t)a + 1u >= m->registers_size || (uint32_t)b + 1u >= m->registers_size || c >= m->registers_size) goto bad_reg;
                if (op <= 0xa2 && (uint32_t)c + 1u >= m->registers_size) goto bad_reg;
                const int64_t lhs = (int64_t)regs[b];
                const int64_t rhs = (int64_t)regs[c];
                int64_t result = 0;
                if (op == 0x9b) result = (int64_t)((uint64_t)lhs + (uint64_t)rhs);
                else if (op == 0x9c) result = (int64_t)((uint64_t)lhs - (uint64_t)rhs);
                else if (op == 0x9d) result = (int64_t)((uint64_t)lhs * (uint64_t)rhs);
                else if (op == 0x9e) {
                    if (!dex_vm_dex_long_div(lhs, rhs, &result)) { throw_class = "java/lang/ArithmeticException"; snprintf(throw_msg, sizeof(throw_msg), "/ by zero"); goto java_throw; }
                } else if (op == 0x9f) {
                    if (!dex_vm_dex_long_rem(lhs, rhs, &result)) { throw_class = "java/lang/ArithmeticException"; snprintf(throw_msg, sizeof(throw_msg), "/ by zero"); goto java_throw; }
                } else if (op == 0xa0) result = lhs & rhs;
                else if (op == 0xa1) result = lhs | rhs;
                else if (op == 0xa2) result = lhs ^ rhs;
                else if (op == 0xa3) result = (int64_t)((uint64_t)lhs << ((uint32_t)regs[c] & 63u));
                else if (op == 0xa4) result = lhs >> ((uint32_t)regs[c] & 63u);
                else result = (int64_t)((uint64_t)lhs >> ((uint32_t)regs[c] & 63u));
                dex_vm_dex_write_wide(regs, a, (uint64_t)result); pc += 2; break;
            }
            case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5:
            case 0xb6: case 0xb7: case 0xb8: case 0xb9: case 0xba: { /* int binop/2addr 12x */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if (a >= m->registers_size || b >= m->registers_size) goto bad_reg;
                const int32_t lhs = (int32_t)regs[a], rhs = (int32_t)regs[b];
                int32_t result = 0;
                if (op == 0xb0) result = (int32_t)((uint32_t)lhs + (uint32_t)rhs);
                else if (op == 0xb1) result = (int32_t)((uint32_t)lhs - (uint32_t)rhs);
                else if (op == 0xb2) result = (int32_t)((uint32_t)lhs * (uint32_t)rhs);
                else if (op == 0xb3) { if (!dex_vm_dex_int_div(lhs, rhs, &result)) { throw_class = "java/lang/ArithmeticException"; snprintf(throw_msg, sizeof(throw_msg), "/ by zero"); goto java_throw; } }
                else if (op == 0xb4) { if (!dex_vm_dex_int_rem(lhs, rhs, &result)) { throw_class = "java/lang/ArithmeticException"; snprintf(throw_msg, sizeof(throw_msg), "/ by zero"); goto java_throw; } }
                else if (op == 0xb5) result = lhs & rhs;
                else if (op == 0xb6) result = lhs | rhs;
                else if (op == 0xb7) result = lhs ^ rhs;
                else if (op == 0xb8) result = (int32_t)((uint32_t)lhs << ((uint32_t)rhs & 31u));
                else if (op == 0xb9) result = lhs >> ((uint32_t)rhs & 31u);
                else result = (int32_t)((uint32_t)lhs >> ((uint32_t)rhs & 31u));
                regs[a] = (uint64_t)(int64_t)result; pc += 1; break;
            }
            case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf: case 0xc0:
            case 0xc1: case 0xc2: case 0xc3: case 0xc4: case 0xc5: { /* long binop/2addr 12x */
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if ((uint32_t)a + 1u >= m->registers_size || b >= m->registers_size) goto bad_reg;
                if (op <= 0xc2 && (uint32_t)b + 1u >= m->registers_size) goto bad_reg;
                const int64_t lhs = (int64_t)regs[a], rhs = (int64_t)regs[b];
                int64_t result = 0;
                if (op == 0xbb) result = (int64_t)((uint64_t)lhs + (uint64_t)rhs);
                else if (op == 0xbc) result = (int64_t)((uint64_t)lhs - (uint64_t)rhs);
                else if (op == 0xbd) result = (int64_t)((uint64_t)lhs * (uint64_t)rhs);
                else if (op == 0xbe) { if (!dex_vm_dex_long_div(lhs, rhs, &result)) { throw_class = "java/lang/ArithmeticException"; snprintf(throw_msg, sizeof(throw_msg), "/ by zero"); goto java_throw; } }
                else if (op == 0xbf) { if (!dex_vm_dex_long_rem(lhs, rhs, &result)) { throw_class = "java/lang/ArithmeticException"; snprintf(throw_msg, sizeof(throw_msg), "/ by zero"); goto java_throw; } }
                else if (op == 0xc0) result = lhs & rhs;
                else if (op == 0xc1) result = lhs | rhs;
                else if (op == 0xc2) result = lhs ^ rhs;
                else if (op == 0xc3) result = (int64_t)((uint64_t)lhs << ((uint32_t)regs[b] & 63u));
                else if (op == 0xc4) result = lhs >> ((uint32_t)regs[b] & 63u);
                else result = (int64_t)((uint64_t)lhs >> ((uint32_t)regs[b] & 63u));
                dex_vm_dex_write_wide(regs, a, (uint64_t)result); pc += 1; break;
            }
            case 0xd0: case 0xd1: case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd6: case 0xd7: { /* int/lit16 22s */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi & 0x0f, b = hi >> 4;
                if (a >= m->registers_size || b >= m->registers_size) goto bad_reg;
                const int32_t lhs = (int32_t)regs[b];
                const int32_t lit = (int16_t)m->insns[pc + 1];
                int32_t result = 0;
                if (op == 0xd0) result = (int32_t)((uint32_t)lhs + (uint32_t)lit);
                else if (op == 0xd1) result = (int32_t)((uint32_t)lit - (uint32_t)lhs);
                else if (op == 0xd2) result = (int32_t)((uint32_t)lhs * (uint32_t)lit);
                else if (op == 0xd3) {
                    if (!dex_vm_dex_int_div(lhs, lit, &result)) {
                        throw_class = "java/lang/ArithmeticException";
                        snprintf(throw_msg, sizeof(throw_msg), "/ by zero");
                        goto java_throw;
                    }
                } else if (op == 0xd4) {
                    if (!dex_vm_dex_int_rem(lhs, lit, &result)) {
                        throw_class = "java/lang/ArithmeticException";
                        snprintf(throw_msg, sizeof(throw_msg), "/ by zero");
                        goto java_throw;
                    }
                } else if (op == 0xd5) result = lhs & lit;
                else if (op == 0xd6) result = lhs | lit;
                else result = lhs ^ lit;
                regs[a] = (uint64_t)(int64_t)result;
                pc += 2;
                break;
            }
            case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:
            case 0xe0: case 0xe1: case 0xe2: { /* int/lit8 22b */
                if (pc + 1 >= m->insns_size) goto truncated;
                const uint8_t a = hi;
                const uint16_t packed = m->insns[pc + 1];
                const uint8_t b = (uint8_t)(packed & 0xffu);
                const int32_t lit = (int8_t)(packed >> 8);
                if (a >= m->registers_size || b >= m->registers_size) goto bad_reg;
                const int32_t lhs = (int32_t)regs[b];
                int32_t result = 0;
                if (op == 0xd8) result = (int32_t)((uint32_t)lhs + (uint32_t)lit);
                else if (op == 0xd9) result = (int32_t)((uint32_t)lit - (uint32_t)lhs);
                else if (op == 0xda) result = (int32_t)((uint32_t)lhs * (uint32_t)lit);
                else if (op == 0xdb) {
                    if (!dex_vm_dex_int_div(lhs, lit, &result)) {
                        throw_class = "java/lang/ArithmeticException";
                        snprintf(throw_msg, sizeof(throw_msg), "/ by zero");
                        goto java_throw;
                    }
                } else if (op == 0xdc) {
                    if (!dex_vm_dex_int_rem(lhs, lit, &result)) {
                        throw_class = "java/lang/ArithmeticException";
                        snprintf(throw_msg, sizeof(throw_msg), "/ by zero");
                        goto java_throw;
                    }
                } else if (op == 0xdd) result = lhs & lit;
                else if (op == 0xde) result = lhs | lit;
                else if (op == 0xdf) result = lhs ^ lit;
                else if (op == 0xe0) result = (int32_t)((uint32_t)lhs << ((uint32_t)lit & 31u));
                else if (op == 0xe1) result = lhs >> ((uint32_t)lit & 31u);
                else result = (int32_t)((uint32_t)lhs >> ((uint32_t)lit & 31u));
                regs[a] = (uint64_t)(int64_t)result;
                pc += 2;
                break;
            }
            case 0x6e: case 0x6f: case 0x70: case 0x71: case 0x72: { /* invoke-kind 35c */
                if (pc + 2 >= m->insns_size) goto truncated;
                /* DEX format 35c packs A (argument word count) in bits 12..15
                 * and G (the fifth register) in bits 8..11 of the first code
                 * unit: A|G|op. Swapping these nibbles makes a valid one-word
                 * call look like it has zero arguments and breaks vG when the
                 * fifth argument register is present. */
                const uint8_t count = hi >> 4, g = hi & 0x0f; if (count > 5) goto malformed;
                const uint16_t mid = m->insns[pc + 1], packed = m->insns[pc + 2];
                const uint8_t r[5] = {(uint8_t)(packed & 0xf),(uint8_t)((packed>>4)&0xf),(uint8_t)((packed>>8)&0xf),(uint8_t)((packed>>12)&0xf),g};
                uint64_t words[5] = {0}; for(uint8_t i=0;i<count;++i){if(r[i]>=m->registers_size)goto bad_reg;words[i]=regs[r[i]];}
                dex_vm_DexInvokeNames *nm = malloc(sizeof(*nm));
                if (!nm) goto runtime_fail;
                if (!method_ref(m, mid, nm->cls, sizeof(nm->cls), nm->name, sizeof(nm->name), nm->sig,
                                sizeof(nm->sig), nm->ref_detail, sizeof(nm->ref_detail))) {
                    dex_invoke_decode_failure(ctx, m, executing_pc, op, mid, false, 0, count,
                                              have_previous, previous_pc, previous_op, nm->ref_detail);
                    free(nm);
                    return SSRNX_ERR_APK_INVALID;
                }
                const bool is_static = op == 0x71; ctx->calls++;
                dex_vm_dex_runtime_progress_publish(ctx, m, executing_pc, op);
                const bool virtual_dispatch = op == 0x6e || op == 0x72;
                const bool interface_dispatch = op == 0x72;
                dex_log_null_stringbuilder_arg(m, executing_pc, op, nm, words, count,
                                               count > 0 ? r[0] : UINT32_MAX,
                                               count > 1 ? r[1] : UINT32_MAX);
                SsrnxJniValue rv={0}; SsrnxResult rc=dex_vm_execute_target(ctx,nm->cls,nm->name,nm->sig,is_static,virtual_dispatch,interface_dispatch,words,count,depth+1,&rv);
                if (ssrnx_jni_exception_pending_host(ctx->env)) {
                    void *ex = NULL; uint32_t handler_pc = 0;
                    if (dex_vm_dex_route_pending_exception(ctx, m, executing_pc, &ex, &handler_pc)) {
                        free(nm);
                        if (have_caught_exception && caught_exception)
                            ssrnx_jni_delete_local_ref_host(ctx->env, caught_exception);
                        caught_exception = ex;
                        have_caught_exception = true;
                        have_pending = false;
                        pc = handler_pc;
                        break;
                    }
                    dex_invoke_uncaught(ctx, m, executing_pc, op, nm);
                    free(nm);
                    return SSRNX_ERR_APK_UNSUPPORTED;
                }
                pending_type = dex_vm_dex_return_type(nm->sig);
                free(nm);
                if (rc != SSRNX_OK) return rc;
                if (dex_vm_dex_process_exiting(ctx, m, executing_pc, op)) return SSRNX_ERR_APK_UNSUPPORTED;
                pending = rv;
                have_pending = true;
                pc += 3;
                break;
            }
            case 0x74: case 0x75: case 0x76: case 0x77: case 0x78: { /* invoke-kind/range */
                if (pc + 2 >= m->insns_size) goto truncated;
                const uint8_t count = hi;
                const uint16_t mid = m->insns[pc + 1], first = m->insns[pc + 2];
                if ((uint32_t)first + count > m->registers_size) goto bad_reg;
                const uint64_t *words = regs + first;
                dex_vm_DexInvokeNames *nm = malloc(sizeof(*nm));
                if (!nm) goto runtime_fail;
                if (!method_ref(m, mid, nm->cls, sizeof(nm->cls), nm->name, sizeof(nm->name), nm->sig,
                                sizeof(nm->sig), nm->ref_detail, sizeof(nm->ref_detail))) {
                    dex_invoke_decode_failure(ctx, m, executing_pc, op, mid, true, first, count,
                                              have_previous, previous_pc, previous_op, nm->ref_detail);
                    free(nm);
                    return SSRNX_ERR_APK_INVALID;
                }
                const bool is_static=op==0x77;const bool virtual_dispatch=op==0x74||op==0x78;const bool interface_dispatch=op==0x78;ctx->calls++;dex_vm_dex_runtime_progress_publish(ctx,m,executing_pc,op);dex_log_null_stringbuilder_arg(m,executing_pc,op,nm,words,count,first,count>1?(uint32_t)first+1u:UINT32_MAX);SsrnxJniValue rv={0};SsrnxResult rc=dex_vm_execute_target(ctx,nm->cls,nm->name,nm->sig,is_static,virtual_dispatch,interface_dispatch,words,count,depth+1,&rv);
                if (ssrnx_jni_exception_pending_host(ctx->env)) {
                    void *ex = NULL; uint32_t handler_pc = 0;
                    if (dex_vm_dex_route_pending_exception(ctx, m, executing_pc, &ex, &handler_pc)) {
                        free(nm);
                        if (have_caught_exception && caught_exception)
                            ssrnx_jni_delete_local_ref_host(ctx->env, caught_exception);
                        caught_exception = ex;
                        have_caught_exception = true;
                        have_pending = false;
                        pc = handler_pc;
                        break;
                    }
                    dex_invoke_uncaught(ctx, m, executing_pc, op, nm);
                    free(nm);
                    return SSRNX_ERR_APK_UNSUPPORTED;
                }
                pending_type = dex_vm_dex_return_type(nm->sig);
                free(nm);
                if (rc != SSRNX_OK) return rc;
                if (dex_vm_dex_process_exiting(ctx, m, executing_pc, op)) return SSRNX_ERR_APK_UNSUPPORTED;
                pending = rv;
                have_pending = true;
                pc += 3;
                break;
            }
            default: {
                char detail[128]; snprintf(detail,sizeof(detail),"unsupported DEX opcode 0x%02x at code-unit %#x",op,pc);
                dex_vm_fail_report(ctx,m->declaring_class,m->method_name,m->signature,pc,op,detail);
                SSRNX_LOGCW("DEX", "class=%s method=%s%s pc=%#x opcode=0x%02x unsupported",
                            m->declaring_class,m->method_name,m->signature,pc,op);
                return SSRNX_ERR_APK_UNSUPPORTED;
            }
        }
        dex_vm_dex_update_reference_regs(ref_regs, m->insns + executing_pc);
        previous_pc = executing_pc;
        previous_op = op;
        have_previous = true;
        continue;
bad_reg:
        dex_vm_fail_report(ctx,m->declaring_class,m->method_name,m->signature,pc,op,"invalid DEX register index"); return SSRNX_ERR_APK_INVALID;
truncated:
        dex_vm_fail_report(ctx,m->declaring_class,m->method_name,m->signature,pc,op,"truncated DEX instruction"); return SSRNX_ERR_APK_INVALID;
malformed:
        dex_vm_fail_report(ctx,m->declaring_class,m->method_name,m->signature,pc,op,"malformed DEX constant/control reference"); return SSRNX_ERR_APK_INVALID;
runtime_fail:
        /* A framework operation that failed by throwing (for example a class
         * that cannot be resolved for new-array) is an ordinary Java throw at
         * this instruction: route it through the catch table. */
        if (ssrnx_jni_exception_pending_host(ctx->env)) {
            throw_class = NULL;
            goto java_pending;
        }
        dex_vm_fail_report(ctx,m->declaring_class,m->method_name,m->signature,pc,op,"JNI/framework operation failed during DEX execution"); return SSRNX_ERR_APK_UNSUPPORTED;
java_throw:
        /* Opcode-raised Java exception (NPE, bounds, arithmetic): route it through
         * this method's try/catch table, else leave it pending for the caller. */
        (void)ssrnx_jni_throw_new_by_name(ctx->env, throw_class, throw_msg);
java_pending:
        {
            void *ex = NULL; uint32_t handler_pc = 0;
            if (!dex_vm_dex_route_pending_exception(ctx, m, executing_pc, &ex, &handler_pc)) {
                if (throw_class) {
                    char detail[192];
                    snprintf(detail, sizeof(detail), "uncaught %s: %s", throw_class, throw_msg);
                    dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, executing_pc, op, detail);
                } else {
                    dex_vm_dex_report_uncaught_propagation(ctx, m, executing_pc, op, "framework operation");
                }
                return SSRNX_ERR_APK_UNSUPPORTED;
            }
            if (have_caught_exception && caught_exception)
                ssrnx_jni_delete_local_ref_host(ctx->env, caught_exception);
            caught_exception = ex;
            have_caught_exception = true;
            have_pending = false;
            pc = handler_pc;
        }
        continue;
    }
    dex_vm_fail_report(ctx,m->declaring_class,m->method_name,m->signature,pc,0,"DEX method fell off end without return");
    return SSRNX_ERR_APK_INVALID;
}

/* Native stack guard. Each interpreted Java call recurses on the native stack,
 * so the call-depth limit alone does not protect threads with small stacks.
 * Like ART's stack guard, refuse a new frame (StackOverflowError) once less
 * than DEX_NATIVE_STACK_RESERVE bytes remain, leaving room for the exception
 * path and for JNI natives the caller may still run. Threads whose bounds are
 * unknown are not guarded. */
#define DEX_NATIVE_STACK_RESERVE (64u * 1024u)
static __thread uintptr_t dex_vm_g_dex_stack_lo;
static __thread bool dex_vm_g_dex_stack_known;
static __thread bool dex_vm_g_dex_stack_probed;

static void dex_vm_dex_native_stack_probe(void) {
    dex_vm_g_dex_stack_probed = true;
    const uintptr_t sp = (uintptr_t)__builtin_frame_address(0);
#ifdef __SWITCH__
    Thread *self = threadGetSelf();
    if (!self || !self->stack_sz) return;
    const uintptr_t bases[2] = {(uintptr_t)self->stack_mirror, (uintptr_t)self->stack_mem};
    for (size_t i = 0; i < 2; ++i) {
        if (bases[i] && sp > bases[i] && sp <= bases[i] + self->stack_sz) {
            dex_vm_g_dex_stack_lo = bases[i];
            dex_vm_g_dex_stack_known = true;
            return;
        }
    }
#elif defined(__linux__)
    pthread_attr_t attr;
    void *addr = NULL;
    size_t size = 0;
    if (pthread_getattr_np(pthread_self(), &attr) != 0) return;
    if (pthread_attr_getstack(&attr, &addr, &size) == 0 && addr && size &&
        sp > (uintptr_t)addr && sp <= (uintptr_t)addr + size) {
        dex_vm_g_dex_stack_lo = (uintptr_t)addr;
        dex_vm_g_dex_stack_known = true;
    }
    (void)pthread_attr_destroy(&attr);
#else
    (void)sp;
#endif
}

static bool dex_vm_dex_native_stack_exhausted(size_t *out_remaining) {
    if (!dex_vm_g_dex_stack_probed) dex_vm_dex_native_stack_probe();
    if (!dex_vm_g_dex_stack_known) return false;
    const uintptr_t sp = (uintptr_t)__builtin_frame_address(0);
    const size_t remaining = sp > dex_vm_g_dex_stack_lo ? (size_t)(sp - dex_vm_g_dex_stack_lo) : 0u;
    if (out_remaining) *out_remaining = remaining;
    return remaining < DEX_NATIVE_STACK_RESERVE;
}

/* Track the executing frame so linkage failures can name their caller. */
static SsrnxResult dex_vm_execute_image(dex_vm_DexExecCtx *ctx, SsrnxDexMethodImage *m,
                                 const uint64_t *incoming_words, size_t incoming_count,
                                 unsigned depth, SsrnxJniValue *out_result) {
    size_t stack_remaining = 0;
    if (ctx && m && dex_vm_dex_native_stack_exhausted(&stack_remaining)) {
        char detail[512];
        snprintf(detail, sizeof(detail),
                 "native stack exhausted (%zu bytes left, depth %u) entering %.200s.%.80s%.160s",
                 stack_remaining, depth, m->declaring_class, m->method_name, m->signature);
        if (ctx->report && !ctx->report->detail[0])
            dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, 0, 0, detail);
        SSRNX_LOGCW("DEX", "%s", detail);
        if (ctx->env && !ssrnx_jni_exception_pending_host(ctx->env))
            (void)ssrnx_jni_throw_new_by_name(ctx->env, "java/lang/StackOverflowError", detail);
        return SSRNX_ERR_APK_UNSUPPORTED;
    }
    if (ctx && m && depth > DEX_MAX_DEPTH) {
        /* ART raises StackOverflowError; keep it catchable by the guest. */
        char detail[512];
        snprintf(detail, sizeof(detail), "Java call depth limit %u exceeded entering %.200s.%.80s%.160s",
                 DEX_MAX_DEPTH, m->declaring_class, m->method_name, m->signature);
        if (ctx->report && !ctx->report->detail[0])
            dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, 0, 0, detail);
        SSRNX_LOGCW("DEX", "%s", detail);
        if (ctx->env && !ssrnx_jni_exception_pending_host(ctx->env))
            (void)ssrnx_jni_throw_new_by_name(ctx->env, "java/lang/StackOverflowError", detail);
        return SSRNX_ERR_APK_UNSUPPORTED;
    }
    const SsrnxDexMethodImage *saved = ctx ? ctx->current : NULL;
    const bool stack_pushed = m && dex_vm_g_dex_stack_depth < DEX_STACK_SNAPSHOT_MAX;
    if (stack_pushed) dex_vm_g_dex_stack[dex_vm_g_dex_stack_depth++] = m;
    dex_vm_DexProfSlot *prof = dex_vm_dex_prof_slot();
    const SsrnxDexMethodImage *prof_saved = NULL;
    /* A framework call that re-entered Java is not this frame's callee. */
    dex_vm_DexProfHostMark prof_host = {NULL, NULL, NULL};
    if (prof) {
        prof_saved = dex_vm_dex_prof_method_exchange(prof, m);
        prof_host = dex_vm_dex_prof_host_enter(NULL, NULL);
    }
    if (ctx) ctx->current = m;
    const SsrnxResult rc = dex_vm_execute_image_body(ctx, m, incoming_words, incoming_count, depth, out_result);
    if (ctx) ctx->current = saved;
    if (prof) {
        (void)dex_vm_dex_prof_method_exchange(prof, prof_saved);
        dex_vm_dex_prof_host_leave(prof_host);
    }
    if (stack_pushed && dex_vm_g_dex_stack_depth) --dex_vm_g_dex_stack_depth;
    /* Every failure leaves a diagnosis: an interpreter-internal error (for
     * example resource exhaustion) that did not report itself is attributed
     * to the innermost frame that observed it. */
    if (rc != SSRNX_OK && ctx && ctx->report && !ctx->report->detail[0] && m &&
        !(ctx->env && ssrnx_jni_exception_pending_host(ctx->env))) {
        char detail[640];
        snprintf(detail, sizeof(detail), "internal failure rc=%d (%s) in %.200s.%.80s%.160s",
                 (int)rc, ssrnx_result_string(rc), m->declaring_class, m->method_name, m->signature);
        dex_vm_fail_report(ctx, m->declaring_class, m->method_name, m->signature, 0, 0, detail);
        SSRNX_LOGCW("DEX", "%s", detail);
    }
    return rc;
}

static __attribute__((noinline)) SsrnxResult dex_throw_null_receiver(dex_vm_DexExecCtx *ctx, const char *class_name,
                                                                     const char *method_name, const char *signature) {
    /* JLS 15.12.4.4: an instance invocation on null throws NPE, which the
     * invoking frame routes through its catch table. */
    char msg[640];
    snprintf(msg, sizeof(msg), "Attempt to invoke method '%s.%s%s' on a null object reference",
             class_name, method_name, signature);
    (void)ssrnx_jni_throw_new_by_name(ctx->env, "java/lang/NullPointerException", msg);
    dex_vm_fail_report(ctx, class_name, method_name, signature, 0, 0, msg);
    return SSRNX_ERR_APK_UNSUPPORTED;
}

static __attribute__((noinline)) void dex_report_method_lookup_failure(dex_vm_DexExecCtx *ctx, const char *current,
                                                                       const char *method_name, const char *signature,
                                                                       SsrnxResult rc) {
    char detail[640];
    snprintf(detail, sizeof(detail), "DEX method lookup failed class=%s method=%s sig=%s rc=%d (%s)",
             current, method_name, signature, (int)rc, ssrnx_result_string(rc));
    dex_vm_fail_report(ctx, current, method_name, signature, 0, 0, detail);
}

/* A framework method (including a Proxy's InvocationHandler) completed
 * abruptly: record which throwable escaped from which call. */
static __attribute__((noinline)) void dex_report_framework_uncaught(dex_vm_DexExecCtx *ctx, const char *current,
                                                                    const char *method_name, const char *signature) {
    char ex_class[256] = {0}, ex_msg[256] = {0}, detail[768];
    void *ex = ssrnx_jni_exception_occurred_host(ctx->env);
    if (ex) {
        (void)ssrnx_jni_object_class_name_host(ex, ex_class, sizeof(ex_class));
        (void)ssrnx_jni_exception_message_host(ex, ex_msg, sizeof(ex_msg));
        ssrnx_jni_delete_local_ref_host(ctx->env, ex);
    }
    snprintf(detail, sizeof(detail), "uncaught %.200s: %.200s from %.160s.%.60s%.90s",
             ex_class[0] ? ex_class : "<unknown>", ex_msg, current, method_name, signature);
    if (ctx->report && !ctx->report->detail[0])
        dex_vm_fail_report(ctx, current, method_name, signature, 0, 0, detail);
}

static __attribute__((noinline)) SsrnxResult dex_unresolved_method(dex_vm_DexExecCtx *ctx, const char *current,
                                                                   const char *method_name, const char *signature) {
    char detail[512];
    snprintf(detail, sizeof(detail), "unresolved Java method class=%s method=%s sig=%s",
             current, method_name, signature);
    return dex_vm_dex_linkage_failure(ctx, "method", current, method_name, signature, detail);
}

static __attribute__((noinline)) SsrnxResult dex_dispatch_exhausted(dex_vm_DexExecCtx *ctx, const char *class_name,
                                                                    const char *method_name, const char *signature,
                                                                    bool virtual_dispatch, bool interface_dispatch,
                                                                    bool runtime_dispatch_selected) {
    char detail[768];
    snprintf(detail, sizeof(detail),
             "method resolution exhausted class=%s method=%s sig=%s dispatch=%s runtimeSelected=%d",
             class_name, method_name, signature,
             interface_dispatch ? "interface" : (virtual_dispatch ? "virtual" : "direct"),
             runtime_dispatch_selected ? 1 : 0);
    return dex_vm_dex_linkage_failure(ctx, "dispatch", class_name, method_name, signature, detail);
}

static SsrnxResult dex_vm_execute_target_with(dex_vm_DexExecCtx *ctx, const char *class_name, const char *method_name,
                                       const char *signature, bool is_static, bool virtual_dispatch,
                                       bool interface_dispatch, const uint64_t *words,
                                       size_t word_count, unsigned depth,
                                       SsrnxJniValue *out_result, SsrnxDexMethodImage *method_slot,
                                       SsrnxJniValue *args);

static __attribute__((noinline)) void dex_descriptor_args_failure(dex_vm_DexExecCtx *ctx, const char *class_name,
                                                                  const char *method_name,
                                                                  const char *signature,
                                                                  size_t arg_words) {
    char detail[160];
    snprintf(detail, sizeof(detail),
             "unsupported/invalid method descriptor arguments argWords=%zu maxSlots=%u",
             arg_words, (unsigned)SSRNX_JNI_MAX_PARAMETER_SLOTS);
    dex_vm_fail_report(ctx, class_name, method_name, signature, 0, 0, detail);
}

/* Every model has missed: a member the API-30 platform declares binds its inert
 * implementation (ssrnx/api_stubs.h) instead of raising a linkage error.
 * Returns true when the call was dispatched; *out_rc then holds its result. */
static __attribute__((noinline)) bool dex_platform_stub_call(dex_vm_DexExecCtx *ctx, const char *class_name,
                                                              const char *method_name,
                                                              const char *signature, bool is_static,
                                                              void *receiver, const SsrnxJniValue *args,
                                                              SsrnxJniValue *out_result,
                                                              SsrnxResult *out_rc) {
    char owner[512];
    if (!ssrnx_api_stub_bind_method(ctx->env, class_name, method_name, signature, is_static,
                                    owner, sizeof(owner)))
        return false;
    bool found = false;
    const SsrnxResult rc = ssrnx_jni_invoke_method_host(ctx->env, is_static ? NULL : receiver, owner,
                                                        method_name, signature, is_static, args,
                                                        out_result, &found);
    if (rc != SSRNX_OK) { *out_rc = rc; return true; }
    if (!found) return false;
    *out_rc = ssrnx_jni_exception_pending_host(ctx->env) ? SSRNX_ERR_APK_UNSUPPORTED : SSRNX_OK;
    return true;
}

static SsrnxResult dex_vm_execute_target(dex_vm_DexExecCtx *ctx, const char *class_name, const char *method_name,
                                  const char *signature, bool is_static, bool virtual_dispatch,
                                  bool interface_dispatch, const uint64_t *words,
                                  size_t word_count, unsigned depth,
                                  SsrnxJniValue *out_result) {
    if (!ctx || !class_name || !method_name || !signature || !out_result || depth > DEX_MAX_DEPTH)
        return SSRNX_ERR_ARGUMENT;
    /* The resolved method image (~5 KB of names) stays alive while the callee
     * runs, so it is heap-held rather than part of the recursive frame. */
    struct { SsrnxDexMethodImage method; SsrnxJniValue args[SSRNX_JNI_MAX_PARAMETER_SLOTS]; } *frame = malloc(sizeof(*frame));
    if (!frame) {
        dex_vm_fail_report(ctx, class_name, method_name, signature, 0, 0, "out of memory resolving DEX method");
        return SSRNX_ERR_NO_MEMORY;
    }
    const SsrnxResult rc = dex_vm_execute_target_with(ctx, class_name, method_name, signature, is_static,
                                               virtual_dispatch, interface_dispatch, words, word_count,
                                               depth, out_result, &frame->method, frame->args);
    free(frame);
    return rc;
}

static SsrnxResult dex_vm_execute_target_with(dex_vm_DexExecCtx *ctx, const char *class_name, const char *method_name,
                                       const char *signature, bool is_static, bool virtual_dispatch,
                                       bool interface_dispatch, const uint64_t *words,
                                       size_t word_count, unsigned depth,
                                       SsrnxJniValue *out_result, SsrnxDexMethodImage *method_slot,
                                       SsrnxJniValue *args) {
    void *receiver = NULL;
    const uint64_t *arg_words = words;
    size_t arg_word_count = word_count;
    if (!is_static) {
        if (!word_count) return SSRNX_ERR_APK_INVALID;
        receiver = (void *)(uintptr_t)words[0];
        arg_words = words + 1;
        arg_word_count = word_count - 1u;
        if (!receiver) return dex_throw_null_receiver(ctx, class_name, method_name, signature);
    }
    size_t arg_count = 0;
    if (!dex_vm_host_args_from_words(signature,arg_words,arg_word_count,args,SSRNX_JNI_MAX_PARAMETER_SLOTS,&arg_count)) {
        dex_descriptor_args_failure(ctx, class_name, method_name, signature, arg_word_count);
        return SSRNX_ERR_APK_UNSUPPORTED;
    }

    /* Optional third-party SDK policy runs before package DEX resolution so a
     * known ad SDK entry point can report "unavailable" without executing a
     * large provider implementation that SSRNX cannot present on Switch.  The
     * policy is deliberately narrow: unknown/non-ad methods fall through to
     * ordinary DEX/framework resolution unchanged. */
    bool optional_handled = false;
    SsrnxResult optional_rc = ssrnx_optional_sdk_try_call(ctx->env, receiver,
                                                          class_name, method_name,
                                                          signature, is_static,
                                                          args, arg_count,
                                                          out_result, &optional_handled);
    if (optional_rc != SSRNX_OK) return optional_rc;
    if (optional_handled) {
        SSRNX_LOGCI("DEX", "optional SDK policy class=%s method=%s sig=%s",
                    class_name, method_name, signature);
        return SSRNX_OK;
    }

    /* Reusable middleware bridges are checked before package DEX execution only
     * when SSRNX is implementing a platform-facing engine contract (for example
     * Cocos keyboard/bitmap integration). Application-defined Java methods must
     * execute their original DEX and are never intercepted here. */


    bool host_found = false;
    SsrnxResult rc = SSRNX_OK;
    char current[512];
    bool runtime_dispatch_selected = false;
    if (!dex_vm_copy_text(current, sizeof(current), class_name)) return SSRNX_ERR_ARGUMENT;
    if (virtual_dispatch && receiver) {
        /* invoke-virtual/interface select the implementation from the
         * receiver's runtime class (JVMS 5.4.6), not the referenced class.
         * For invoke-interface the verifier has already established receiver
         * compatibility.  Do not require SSRNX's lazily-built interface graph
         * to be complete before selecting the concrete receiver class: doing so
         * turns valid interface calls into Object.<method> or an exhausted
         * superclass walk during early application bootstrap. */
        char runtime[512];
        if (ssrnx_jni_object_class_name_host(receiver, runtime, sizeof(runtime)) &&
            strcmp(runtime, class_name) != 0 &&
            (interface_dispatch || ssrnx_jni_is_instance_of_class_host(receiver, class_name))) {
            (void)dex_vm_copy_text(current, sizeof(current), runtime);
            runtime_dispatch_selected = true;
        }
    }
    for (unsigned walk=0; walk<DEX_MAX_DEPTH; ++walk) {
        /* The lookup initializes every field it reports. */
        SsrnxDexMethodImage *const method = method_slot; method->dex_owner = NULL; method->dex_data = NULL;
        method->super_class[0] = '\0'; bool class_found=false, method_found=false;
        rc=ssrnx_package_load_dex_method(ctx->source_path,current,method_name,signature,method,&class_found,&method_found);
        if(rc!=SSRNX_OK){
            dex_report_method_lookup_failure(ctx,current,method_name,signature,rc);
            return rc;
        }
        if(method_found){
            if (is_static && strcmp(method_name, "<clinit>") != 0) {
                SsrnxResult init_rc = dex_vm_ensure_dex_class_initialized(ctx, method->declaring_class, depth + 1u);
                if (init_rc != SSRNX_OK) { ssrnx_package_release_dex_method(method); return init_rc; }
            }
            void *sync_ref = NULL;
            bool sync_ref_local = false;
            const bool synchronized_method =
                (method->access_flags & DEX_ACC_SYNCHRONIZED) != 0;
            if (synchronized_method) {
                if (is_static) {
                    sync_ref = ssrnx_jni_find_class_ref(ctx->env, method->declaring_class);
                    sync_ref_local = sync_ref != NULL;
                } else {
                    sync_ref = receiver;
                }
                if (!sync_ref ||
                    ssrnx_jni_monitor_enter_host(ctx->env, sync_ref) != SSRNX_JNI_OK) {
                    if (sync_ref_local)
                        ssrnx_jni_delete_local_ref_host(ctx->env, sync_ref);
                    ssrnx_package_release_dex_method(method);
                    if (!ssrnx_jni_exception_pending_host(ctx->env))
                        (void)ssrnx_jni_throw_new_by_name(
                            ctx->env, "java/lang/IllegalMonitorStateException",
                            method->declaring_class);
                    return SSRNX_ERR_APK_UNSUPPORTED;
                }
            }

            if(method->access_flags&DEX_ACC_NATIVE){
                /* Native time is not Java time for the profiler. */
                dex_vm_DexProfSlot *prof = dex_vm_dex_prof_slot();
                const SsrnxDexMethodImage *prof_saved =
                    dex_vm_dex_prof_method_exchange(prof, NULL);
                rc = dex_vm_execute_native_method(ctx, current, method_name, signature, is_static,
                                           receiver, args, arg_count, out_result);
                (void)dex_vm_dex_prof_method_exchange(prof, prof_saved);
            } else {
                const uint64_t *incoming=words;size_t incoming_count=word_count;
                if (ssrnx_diag_full_trace_enabled())
                    SSRNX_LOGCI("DEX", "enter class=%s method=%s sig=%s regs=%u ins=%u code=%#x",
                                method->declaring_class,method->method_name,method->signature,method->registers_size,method->ins_size,method->code_offset);
                rc=dex_vm_execute_image(ctx,method,incoming,incoming_count,depth,out_result);
                if (ssrnx_diag_full_trace_enabled())
                    SSRNX_LOGCI("DEX", "leave class=%s method=%s sig=%s rc=%d",
                                method->declaring_class,method->method_name,method->signature,(int)rc);
            }

            if (synchronized_method) {
                const int32_t exit_rc = ssrnx_jni_monitor_exit_host(ctx->env, sync_ref);
                if (exit_rc != SSRNX_JNI_OK && rc == SSRNX_OK)
                    rc = SSRNX_ERR_APK_UNSUPPORTED;
                if (sync_ref_local)
                    ssrnx_jni_delete_local_ref_host(ctx->env, sync_ref);
            }
            ssrnx_package_release_dex_method(method);
            return rc;
        }
        if (!class_found) {
            if (!is_static && strcmp(current, "java/lang/Object") == 0 &&
                strcmp(method_name, "<init>") == 0 && strcmp(signature, "()V") == 0) {
                memset(out_result, 0, sizeof(*out_result));
                SSRNX_LOGCI("DEX", "framework intrinsic class=java/lang/Object method=<init> sig=()V");
                return SSRNX_OK;
            }
            const dex_vm_DexProfHostMark host_mark = dex_vm_dex_prof_host_enter(current, method_name);
            rc = ssrnx_jni_invoke_method_host(ctx->env, receiver, current, method_name, signature,
                                              is_static, args, out_result, &host_found);
            dex_vm_dex_prof_host_leave(host_mark);
            if (rc != SSRNX_OK) return rc;
            if (host_found) {
                SSRNX_LOGCI("DEX", "framework call class=%s method=%s sig=%s",
                            current, method_name, signature);
                /* A framework method (including a Proxy's InvocationHandler)
                 * completed abruptly: the throwable stays pending and the call
                 * fails, so a top-level callback is never reported complete
                 * while an exception is still in flight. Nested invoke opcodes
                 * check the pending exception before rc and route it. */
                if (ssrnx_jni_exception_pending_host(ctx->env)) {
                    dex_report_framework_uncaught(ctx, current, method_name, signature);
                    return SSRNX_ERR_APK_UNSUPPORTED;
                }
                return SSRNX_OK;
            }
            /* A receiver-class lookup for invoke-interface may legitimately
             * exhaust the concrete class/superclass implementation.  Do not
             * turn that into Object.<interface-method>; the interface default
             * resolution below still has to run. */
            /* Do not collapse an unresolved virtual/interface call to
             * Object.<method>. After the package DEX class walk, the receiver's
             * JNI model still carries implemented interfaces and their host
             * methods. Defer the miss until that model is queried. */
            if (virtual_dispatch && receiver) break;
            if (dex_platform_stub_call(ctx, current, method_name, signature, is_static, receiver,
                                       args, out_result, &rc))
                return rc;
            return dex_unresolved_method(ctx, current, method_name, signature);
        }
        if(!method->super_class[0]){ssrnx_package_release_dex_method(method);break;}
        /* DEX methods must take precedence over inheritance-aware host/JNI
         * shims.  Do not ask the host immediately after moving to a DEX
         * superclass: a framework method inherited by that class can otherwise
         * mask a real implementation farther up the package DEX hierarchy
         * (e.g. GameActivity -> LiteActivity -> Cocos2dxActivity.onResume()).
         * The next loop iteration first probes the superclass DEX definition;
         * host dispatch is only attempted once the walk reaches a class that is
         * not defined in the package. */
        char next[512];
        (void)dex_vm_copy_text(next, sizeof(next), method->super_class);
        ssrnx_package_release_dex_method(method);
        (void)dex_vm_copy_text(current, sizeof(current), next);
    }

    /* DEX lookup has now had first priority. Resolve inherited host/interface
     * methods from the receiver's real runtime class before declaring a miss. */
    if (virtual_dispatch && receiver) {
        char runtime_class[512] = {0};
        if (ssrnx_jni_object_class_name_host(receiver, runtime_class, sizeof(runtime_class))) {
            host_found = false;
            const dex_vm_DexProfHostMark host_mark = dex_vm_dex_prof_host_enter(runtime_class, method_name);
            rc = ssrnx_jni_invoke_method_host(ctx->env, receiver, runtime_class, method_name,
                                              signature, false, args, out_result, &host_found);
            dex_vm_dex_prof_host_leave(host_mark);
            if (rc != SSRNX_OK) return rc;
            if (host_found) {
                SSRNX_LOGCI("DEX", "framework receiver dispatch class=%s method=%s sig=%s",
                            runtime_class, method_name, signature);
                return ssrnx_jni_exception_pending_host(ctx->env)
                    ? SSRNX_ERR_APK_UNSUPPORTED : SSRNX_OK;
            }
        }
    }

    /* A symbolic class reference may inherit its implementation from a Java 8
     * default interface method. Search the runtime receiver's package
     * interface graph after both DEX and host class dispatch have missed. */
    if (virtual_dispatch && receiver) {
        char runtime_root[512] = {0};
        if (!ssrnx_jni_object_class_name_host(receiver, runtime_root, sizeof(runtime_root)))
            (void)dex_vm_copy_text(runtime_root, sizeof(runtime_root), class_name);
        bool default_found = false;
        rc = dex_vm_execute_interface_default_tree(ctx, runtime_root, method_name, signature,
                                            words, word_count, depth + 1u,
                                            out_result, &default_found);
        if (rc != SSRNX_OK || default_found) return rc;
    }

    /* invoke-interface first searches the receiver class hierarchy.  If no
     * class implementation exists, Android then resolves a maximally-specific
     * interface default method.  Preserve the original method_id owner so a
     * runtime-class walk cannot degrade an interface call to Object.<method>.
     * This also lets modeled Java/JNI interface methods handle abstract
     * interface declarations when the concrete behavior lives in the host
     * compatibility runtime. */
    if (interface_dispatch && receiver) {
        SsrnxDexMethodImage *const iface_method = method_slot; /* the class walk released it */
        memset(iface_method, 0, sizeof(*iface_method));
        bool iface_class_found = false, iface_method_found = false;
        rc = ssrnx_package_load_dex_method(ctx->source_path, class_name, method_name, signature,
                                           iface_method, &iface_class_found,
                                           &iface_method_found);
        if (rc != SSRNX_OK) return rc;
        const bool interface_owner = iface_class_found &&
            (iface_method->class_access_flags & DEX_ACC_INTERFACE) != 0;
        if (interface_owner && iface_method_found &&
            (iface_method->access_flags & (DEX_ACC_STATIC | DEX_ACC_ABSTRACT)) == 0 &&
            iface_method->code_offset && iface_method->insns) {
            if (ssrnx_diag_full_trace_enabled())
                SSRNX_LOGCI("DEX", "interface default dispatch class=%s method=%s sig=%s",
                            class_name, method_name, signature);
            rc = dex_vm_execute_image(ctx, iface_method, words, word_count, depth, out_result);
            ssrnx_package_release_dex_method(iface_method);
            return rc;
        }
        ssrnx_package_release_dex_method(iface_method);

        if (interface_owner) {
            host_found = false;
            rc = ssrnx_jni_invoke_method_host(ctx->env, receiver, class_name, method_name,
                                              signature, false, args, out_result, &host_found);
            if (rc != SSRNX_OK) return rc;
            if (host_found) {
                SSRNX_LOGCI("DEX", "framework interface call class=%s method=%s sig=%s",
                            class_name, method_name, signature);
                return ssrnx_jni_exception_pending_host(ctx->env)
                    ? SSRNX_ERR_APK_UNSUPPORTED : SSRNX_OK;
            }
        }
    }
    if (dex_platform_stub_call(ctx, class_name, method_name, signature, is_static, receiver,
                               args, out_result, &rc))
        return rc;
    return dex_dispatch_exhausted(ctx, class_name, method_name, signature, virtual_dispatch,
                                  interface_dispatch, runtime_dispatch_selected);
}

SsrnxResult ssrnx_dex_execute_activity_on_create(const char *source_path,
                                                 const char *activity_class,
                                                 void *env_handle,
                                                 void *activity_ref,
                                                 SsrnxDexExecReport *out_report) {
    if (!source_path || !activity_class || !env_handle || !activity_ref || !out_report)
        return SSRNX_ERR_ARGUMENT;
    memset(out_report,0,sizeof(*out_report));
    dex_vm_DexExecCtx ctx={.source_path=source_path,.env=env_handle,.report=out_report};
    uint64_t words[2]={(uint64_t)(uintptr_t)activity_ref,0};
    SsrnxJniValue result={0};
    const char *sig="(Landroid/os/Bundle;)V";
    SSRNX_LOGCI("DEX", "Activity lifecycle enter class=%s method=onCreate sig=%s",activity_class,sig);
    SsrnxDexPreflightReport preflight = {0};
    SsrnxResult preflight_rc = ssrnx_dex_preflight_method_tree(source_path, activity_class,
                                                                "onCreate", sig, false, &preflight);
    if (preflight_rc != SSRNX_OK)
        SSRNX_LOGCW("DEX-PREFLIGHT", "scan failed class=%s rc=%d; strict execution continues",
                    activity_class, (int)preflight_rc);
    SsrnxResult rc = dex_vm_ensure_dex_class_initialized(&ctx, activity_class, 0);
    if (rc == SSRNX_OK)
        rc=dex_vm_execute_target(&ctx,activity_class,"onCreate",sig,false,false,false,words,2,0,&result);
    out_report->instructions=ctx.steps;out_report->calls=ctx.calls;
    if(rc==SSRNX_OK){out_report->completed=true;(void)dex_vm_copy_text(out_report->class_name,sizeof(out_report->class_name),activity_class);(void)dex_vm_copy_text(out_report->method_name,sizeof(out_report->method_name),"onCreate");(void)dex_vm_copy_text(out_report->signature,sizeof(out_report->signature),sig);(void)dex_vm_copy_text(out_report->detail,sizeof(out_report->detail),"Activity onCreate completed");SSRNX_LOGCI("DEX","Activity lifecycle onCreate complete class=%s instructions=%zu calls=%zu",activity_class,ctx.steps,ctx.calls);}
    else SSRNX_LOGCW("DEX","Activity lifecycle stopped class=%s rc=%d pc=%#x opcode=0x%02x detail=%s",activity_class,(int)rc,out_report->pc,out_report->opcode,out_report->detail);
    return rc;
}

static SsrnxResult dex_vm_dex_execute_instance_common(const char *source_path,
                                              const char *class_name,
                                              const char *method_name,
                                              const char *signature,
                                              void *env_handle,
                                              void *receiver_ref,
                                              const SsrnxJniValue *args,
                                              size_t arg_count,
                                              bool virtual_dispatch,
                                              SsrnxJniValue *out_result,
                                              SsrnxDexExecReport *out_report) {
    if (!source_path || !class_name || !method_name || !signature || !env_handle ||
        !receiver_ref || !out_report)
        return SSRNX_ERR_ARGUMENT;
    memset(out_report, 0, sizeof(*out_report));
    SsrnxJniValue discard_result = {0};
    if (!out_result) out_result = &discard_result;
    else memset(out_result, 0, sizeof(*out_result));

    /* Instance callbacks enter the DEX interpreter, so their arguments must be
     * encoded as Dalvik register words rather than host/native ABI registers.
     * In particular, float occupies one 32-bit DEX register while double/long
     * occupy two consecutive registers.  Using native_gpr_args() here rejected
     * every instance callback containing F/D before the method could execute
     * (for example Cocos handleActionDown(IFFD)V). */
    uint64_t words[SSRNX_JNI_MAX_PARAMETER_SLOTS] = {0};
    words[0] = (uint64_t)(uintptr_t)receiver_ref;
    size_t arg_word_count = 0;
    char return_type = 0;
    if (!dex_vm_dex_words_from_args(signature, args, arg_count, words + 1, SSRNX_JNI_MAX_PARAMETER_SLOTS - 1u,
                             &arg_word_count, &return_type)) {
        (void)return_type;
        (void)dex_vm_copy_text(out_report->class_name, sizeof(out_report->class_name), class_name);
        (void)dex_vm_copy_text(out_report->method_name, sizeof(out_report->method_name), method_name);
        (void)dex_vm_copy_text(out_report->signature, sizeof(out_report->signature), signature);
        (void)dex_vm_copy_text(out_report->detail, sizeof(out_report->detail),
                        "unsupported/invalid instance DEX callback arguments");
        return SSRNX_ERR_APK_UNSUPPORTED;
    }

    dex_vm_DexExecCtx ctx = {.source_path=source_path, .env=env_handle, .report=out_report};
    if (ssrnx_diag_full_trace_enabled())
        SSRNX_LOGCI("DEX", "instance callback enter class=%s method=%s sig=%s",
                    class_name, method_name, signature);
    SsrnxThreadActivity saved_activity;
    dex_vm_thread_activity_enter(&saved_activity, "java", class_name, method_name);
    SsrnxPerfScope java_scope __attribute__((cleanup(ssrnx_perf_scope_end))) =
        ssrnx_perf_scope_begin(SSRNX_PERF_JAVA);
    SsrnxResult rc = dex_vm_ensure_dex_class_initialized(&ctx, class_name, 0);
    if (rc == SSRNX_OK)
        rc = dex_vm_execute_target(&ctx, class_name, method_name, signature, false, virtual_dispatch, false,
                            words, arg_word_count + 1u, 0, out_result);
    dex_vm_thread_activity_leave(&saved_activity);
    out_report->instructions = ctx.steps;
    out_report->calls = ctx.calls;
    if (rc == SSRNX_OK) {
        out_report->completed = true;
        (void)dex_vm_copy_text(out_report->class_name, sizeof(out_report->class_name), class_name);
        (void)dex_vm_copy_text(out_report->method_name, sizeof(out_report->method_name), method_name);
        (void)dex_vm_copy_text(out_report->signature, sizeof(out_report->signature), signature);
        (void)dex_vm_copy_text(out_report->detail, sizeof(out_report->detail), "instance callback completed");
        if (ssrnx_diag_full_trace_enabled())
            SSRNX_LOGCI("DEX", "instance callback complete class=%s method=%s instructions=%zu calls=%zu",
                        class_name, method_name, ctx.steps, ctx.calls);
    } else {
        SSRNX_LOGCW("DEX", "instance callback stopped class=%s method=%s rc=%d at=%s.%s%s pc=%#x opcode=0x%02x detail=%s",
                    class_name, method_name, (int)rc, out_report->class_name,
                    out_report->method_name, out_report->signature, out_report->pc,
                    out_report->opcode, out_report->detail);
    }
    return rc;
}

SsrnxResult ssrnx_dex_execute_instance_method(const char *source_path,
                                              const char *class_name,
                                              const char *method_name,
                                              const char *signature,
                                              void *env_handle,
                                              void *receiver_ref,
                                              const SsrnxJniValue *args,
                                              size_t arg_count,
                                              SsrnxJniValue *out_result,
                                              SsrnxDexExecReport *out_report) {
    return dex_vm_dex_execute_instance_common(source_path, class_name, method_name, signature,
                                       env_handle, receiver_ref, args, arg_count, false,
                                       out_result, out_report);
}


SsrnxResult ssrnx_dex_initialize_class(const char *source_path,
                                       const char *class_name,
                                       void *env_handle,
                                       SsrnxDexExecReport *out_report) {
    if (!source_path || !class_name || !env_handle || !out_report) return SSRNX_ERR_ARGUMENT;
    memset(out_report, 0, sizeof(*out_report));
    dex_vm_DexExecCtx ctx = {.source_path = source_path, .env = env_handle, .report = out_report};
    const SsrnxResult rc = dex_vm_ensure_dex_class_initialized(&ctx, class_name, 0);
    out_report->instructions = ctx.steps;
    out_report->calls = ctx.calls;
    out_report->completed = rc == SSRNX_OK;
    return rc;
}

SsrnxResult ssrnx_dex_execute_static_method(const char *source_path,
                                            const char *class_name,
                                            const char *method_name,
                                            const char *signature,
                                            void *env_handle,
                                            const SsrnxJniValue *args,
                                            size_t arg_count,
                                            SsrnxJniValue *out_result,
                                            SsrnxDexExecReport *out_report) {
    if (!source_path || !class_name || !method_name || !signature || !env_handle || !out_report)
        return SSRNX_ERR_ARGUMENT;
    memset(out_report, 0, sizeof(*out_report));
    SsrnxJniValue discard_result = {0};
    if (!out_result) out_result = &discard_result;
    else memset(out_result, 0, sizeof(*out_result));

    uint64_t words[SSRNX_JNI_MAX_PARAMETER_SLOTS] = {0};
    size_t word_count = 0;
    char return_type = 0;
    if (!dex_vm_dex_words_from_args(signature, args, arg_count, words, SSRNX_JNI_MAX_PARAMETER_SLOTS,
                             &word_count, &return_type)) {
        (void)dex_vm_copy_text(out_report->class_name, sizeof(out_report->class_name), class_name);
        (void)dex_vm_copy_text(out_report->method_name, sizeof(out_report->method_name), method_name);
        (void)dex_vm_copy_text(out_report->signature, sizeof(out_report->signature), signature);
        (void)dex_vm_copy_text(out_report->detail, sizeof(out_report->detail),
                        "unsupported/invalid static DEX callback arguments");
        return SSRNX_ERR_APK_UNSUPPORTED;
    }

    dex_vm_DexExecCtx ctx = {.source_path=source_path, .env=env_handle, .report=out_report};
    /* Native code may call into Java many times per frame: trace the first
     * callbacks (all of them under full tracing); failures always log. */
    static uint32_t callback_traces;
    const bool trace = ssrnx_diag_full_trace_enabled() ||
                       __atomic_fetch_add(&callback_traces, 1u, __ATOMIC_RELAXED) < 256u;
    if (trace)
        SSRNX_LOGCI("DEX", "static JNI callback enter class=%s method=%s sig=%s",
                    class_name, method_name, signature);
    SsrnxThreadActivity saved_activity;
    dex_vm_thread_activity_enter(&saved_activity, "java", class_name, method_name);
    SsrnxPerfScope java_scope __attribute__((cleanup(ssrnx_perf_scope_end))) =
        ssrnx_perf_scope_begin(SSRNX_PERF_JAVA);
    SsrnxResult rc = dex_vm_execute_target(&ctx, class_name, method_name, signature, true, false, false,
                                    words, word_count, 0, out_result);
    dex_vm_thread_activity_leave(&saved_activity);
    out_report->instructions = ctx.steps;
    out_report->calls = ctx.calls;
    if (rc == SSRNX_OK) {
        if (return_type == 'F') {
            uint32_t bits = (uint32_t)out_result->j;
            float value = 0.0f;
            memcpy(&value, &bits, sizeof(value));
            out_result->f = value;
        } else if (return_type == 'D') {
            uint64_t bits = (uint64_t)out_result->j;
            double value = 0.0;
            memcpy(&value, &bits, sizeof(value));
            out_result->d = value;
        }
        out_report->completed = true;
        (void)dex_vm_copy_text(out_report->class_name, sizeof(out_report->class_name), class_name);
        (void)dex_vm_copy_text(out_report->method_name, sizeof(out_report->method_name), method_name);
        (void)dex_vm_copy_text(out_report->signature, sizeof(out_report->signature), signature);
        (void)dex_vm_copy_text(out_report->detail, sizeof(out_report->detail), "static JNI callback completed");
        if (!trace) {
        } else if (return_type == 'Z') {
            SSRNX_LOGCI("DEX",
                        "static JNI callback complete class=%s method=%s resultZ=%d instructions=%zu calls=%zu",
                        class_name, method_name, out_result->z ? 1 : 0, ctx.steps, ctx.calls);
        } else if (return_type == 'L' || return_type == '[') {
            SSRNX_LOGCI("DEX", "static JNI callback complete class=%s method=%s resultObject=%p pending=%d instructions=%zu calls=%zu",
                        class_name, method_name, out_result->l,
                        ssrnx_jni_exception_pending_host(env_handle) ? 1 : 0, ctx.steps, ctx.calls);
        } else {
            SSRNX_LOGCI("DEX", "static JNI callback complete class=%s method=%s instructions=%zu calls=%zu",
                        class_name, method_name, ctx.steps, ctx.calls);
        }
    } else {
        SSRNX_LOGCW("DEX", "static JNI callback stopped class=%s method=%s rc=%d pc=%#x opcode=0x%02x detail=%s",
                    class_name, method_name, (int)rc, out_report->pc,
                    out_report->opcode, out_report->detail);
    }
    return rc;
}

static void dex_vm_dex_jni_bridge_failed(void *env_handle, const dex_vm_DexJniProxyBinding *binding,
                                  const SsrnxDexExecReport *report, const char *kind) {
    SSRNX_LOGCW("DEX", "JNI->DEX %s bridge failed class=%s method=%s sig=%s detail=%s",
                kind, binding->class_name, binding->method_name, binding->signature,
                report->detail[0] ? report->detail : "execution failed");
    if (!ssrnx_jni_exception_pending_host(env_handle))
        (void)ssrnx_jni_throw_new_by_name(env_handle, "java/lang/RuntimeException",
                                          "APN64 JNI-to-DEX method failed");
}

static SsrnxJniValue dex_vm_dex_jni_proxy_call(void *env_handle, void *object_token,
                                        const SsrnxJniValue *args, size_t arg_count,
                                        void *user_data) {
    (void)object_token;
    SsrnxJniValue result = {0};
    dex_vm_DexJniProxyBinding *binding = (dex_vm_DexJniProxyBinding *)user_data;
    if (!binding || !binding->used || !dex_vm_g_java_method_source[0]) return result;
    SsrnxDexExecReport report = {0};
    SsrnxResult rc = ssrnx_dex_execute_static_method(dex_vm_g_java_method_source,
                                                      binding->class_name,
                                                      binding->method_name,
                                                      binding->signature,
                                                      env_handle, args, arg_count,
                                                      &result, &report);
    if (rc != SSRNX_OK || !report.completed) {
        dex_vm_dex_jni_bridge_failed(env_handle, binding, &report, "static");
        memset(&result, 0, sizeof(result));
    }
    return result;
}

/* Call<Type>Method on a package DEX method: the receiver arrives as a JNI
 * object token and dispatch follows its runtime class (JNI Call<type>Method is
 * virtual), except for constructors and private methods. */
static SsrnxJniValue dex_vm_dex_jni_instance_proxy_call(void *env_handle, void *object_token,
                                                 const SsrnxJniValue *args, size_t arg_count,
                                                 void *user_data) {
    SsrnxJniValue result = {0};
    dex_vm_DexJniProxyBinding *binding = (dex_vm_DexJniProxyBinding *)user_data;
    if (!binding || !binding->used || !dex_vm_g_java_method_source[0] || !object_token) return result;
    void *self = ssrnx_jni_new_local_from_token(env_handle, object_token);
    if (!self) return result;
    SsrnxDexExecReport report = {0};
    SsrnxResult rc = dex_vm_dex_execute_instance_common(dex_vm_g_java_method_source, binding->class_name,
                                                 binding->method_name, binding->signature,
                                                 env_handle, self, args, arg_count,
                                                 binding->virtual_dispatch, &result, &report);
    /* A DEX return-object may return the receiver's exact local handle (for
     * example a fluent builder's "return this"). That handle now belongs to
     * the native caller; deleting it here invalidates the returned jobject. */
    const char *return_type = strrchr(binding->signature, ')');
    const bool returns_self = rc == SSRNX_OK && report.completed && return_type &&
                              (return_type[1] == 'L' || return_type[1] == '[') &&
                              result.l == self;
    if (!returns_self) ssrnx_jni_delete_local_ref_host(env_handle, self);
    if (rc != SSRNX_OK || !report.completed) {
        dex_vm_dex_jni_bridge_failed(env_handle, binding, &report, "instance");
        memset(&result, 0, sizeof(result));
    }
    return result;
}

/* Bindings are never released individually (only by a full reset), so every
 * chunk before the head is full and allocation is a bump of the head chunk.
 * Reflection materializes whole classes; a scan of every chunk per binding
 * made that quadratic. */
static dex_vm_DexJniProxyBinding *dex_vm_dex_jni_binding_alloc(void) {
    SSRNX_PERF_LOCK(&dex_vm_g_java_method_bindings_lock);
    dex_vm_DexJniProxyBinding *slot = NULL;
    dex_vm_DexJniProxyChunk *head = dex_vm_g_java_method_bindings;
    if (head && head->used < DEX_JNI_PROXY_CHUNK) slot = &head->slots[head->used++];
    if (!slot) {
        dex_vm_DexJniProxyChunk *chunk = calloc(1, sizeof(*chunk));
        if (chunk) {
            chunk->next = dex_vm_g_java_method_bindings;
            dex_vm_g_java_method_bindings = chunk;
            chunk->used = 1;
            slot = &chunk->slots[0];
        }
    }
    if (slot) { memset(slot, 0, sizeof(*slot)); slot->used = true; }
    pthread_mutex_unlock(&dex_vm_g_java_method_bindings_lock);
    return slot;
}

static void dex_vm_dex_jni_bindings_reset(void) {
    SSRNX_PERF_LOCK(&dex_vm_g_java_method_bindings_lock);
    for (dex_vm_DexJniProxyChunk *chunk = dex_vm_g_java_method_bindings; chunk; chunk = chunk->next) {
        memset(chunk->slots, 0, sizeof(chunk->slots));
        chunk->used = 0;
    }
    pthread_mutex_unlock(&dex_vm_g_java_method_bindings_lock);
}

static SsrnxResult dex_vm_dex_jni_method_resolver(const char *class_name,
                                           const char *method_name,
                                           const char *signature,
                                           bool is_static,
                                           SsrnxJniMethodCallback *out_callback,
                                           void **out_callback_user_data,
                                           bool *out_found,
                                           void *user_data) {
    (void)user_data;
    if (!class_name || !method_name || !signature || !out_callback ||
        !out_callback_user_data || !out_found) return SSRNX_ERR_ARGUMENT;
    *out_callback = NULL;
    *out_callback_user_data = NULL;
    *out_found = false;
    if (!dex_vm_g_java_method_source[0]) return SSRNX_OK;
    /* Constructors are not inherited; every other method is looked up through
     * the superclass chain (JNI GetMethodID/GetStaticMethodID). */
    const bool constructor = strcmp(method_name, "<init>") == 0;
    if (constructor && is_static) return SSRNX_OK;





    char current[512];
    if (!dex_vm_copy_text(current, sizeof(current), class_name)) return SSRNX_ERR_ARGUMENT;
    for (unsigned walk = 0; walk < DEX_MAX_DEPTH; ++walk) {
        SsrnxDexMethodImage method = {0};
        bool class_found = false, method_found = false;
        SsrnxResult rc = ssrnx_package_load_dex_method(dex_vm_g_java_method_source, current,
                                                        method_name, signature, &method,
                                                        &class_found, &method_found);
        if (rc != SSRNX_OK) return rc;
        if (method_found) {
            if (((method.access_flags & DEX_ACC_STATIC) != 0) != is_static) {
                ssrnx_package_release_dex_method(&method);
                return SSRNX_OK;
            }
            /* JNI method lookup must reflect the class metadata, not predict whether
             * the eventual call graph will succeed. If a real static DEX method exists,
             * publish a method ID now and let the bounded interpreter report the first
             * concrete unsupported operation when CallStatic* actually executes it.
             * This is both closer to JNI semantics and avoids false "method missing"
             * results for helpers that legitimately call other package DEX methods. */
            SSRNX_LOGCI("DEX", "JNI->DEX method dynamically bridgeable class=%s method=%s sig=%s",
                        class_name, method_name, signature);
            dex_vm_DexJniProxyBinding *slot = dex_vm_dex_jni_binding_alloc();
            if (!slot) { ssrnx_package_release_dex_method(&method); return SSRNX_ERR_NO_MEMORY; }
            slot->virtual_dispatch = !is_static && !constructor &&
                                     !(method.access_flags & DEX_ACC_PRIVATE);
            if (!dex_vm_copy_text(slot->class_name, sizeof(slot->class_name), class_name) ||
                !dex_vm_copy_text(slot->method_name, sizeof(slot->method_name), method_name) ||
                !dex_vm_copy_text(slot->signature, sizeof(slot->signature), signature)) {
                memset(slot, 0, sizeof(*slot));
                ssrnx_package_release_dex_method(&method);
                return SSRNX_ERR_ARGUMENT;
            }
            ssrnx_package_release_dex_method(&method);
            *out_callback = is_static ? dex_vm_dex_jni_proxy_call : dex_vm_dex_jni_instance_proxy_call;
            *out_callback_user_data = slot;
            *out_found = true;
            SSRNX_LOGCI("DEX", "JNI->DEX method bound class=%s method=%s sig=%s",
                        class_name, method_name, signature);
            return SSRNX_OK;
        }
        if (!class_found || !method.super_class[0] || constructor) {
            ssrnx_package_release_dex_method(&method);
            return SSRNX_OK;
        }
        char next[512];
        (void)dex_vm_copy_text(next, sizeof(next), method.super_class);
        ssrnx_package_release_dex_method(&method);
        (void)dex_vm_copy_text(current, sizeof(current), next);
    }
    return SSRNX_OK;
}

/* Declared access flags of a package DEX method (JVMS/DEX access_flags) and of
 * its declaring class, resolved like JNI GetMethodID through the superclass
 * chain. Reflection uses this for Method.getModifiers()/isDefault(). */
bool ssrnx_dex_class_access_flags(const char *class_name, uint32_t *out_class_flags) {
    const char *source = ssrnx_dex_java_method_source();
    if (!source || !class_name || !out_class_flags) return false;
    SsrnxDexClassView view = {0};
    bool found = false;
    if (ssrnx_package_acquire_dex_class(source, class_name, &view, &found) != SSRNX_OK) return false;
    const bool valid = found && view.dex_data &&
                       (uint64_t)view.class_def_offset + 8u <= view.dex_size;
    if (valid) {
        *out_class_flags = dex_vm_rd32(view.dex_data + view.class_def_offset + 4u);
    }
    ssrnx_package_release_dex_class(&view);
    return valid;
}

bool ssrnx_dex_method_access_flags(const char *class_name, const char *method_name,
                                   const char *signature, uint32_t *out_method_flags,
                                   uint32_t *out_class_flags) {
    if (!class_name || !method_name || !signature || !dex_vm_g_java_method_source[0]) return false;
    char current[512];
    if (!dex_vm_copy_text(current, sizeof(current), class_name)) return false;
    for (unsigned walk = 0; walk < DEX_MAX_DEPTH; ++walk) {
        SsrnxDexMethodImage method = {0};
        bool class_found = false, method_found = false;
        if (ssrnx_package_load_dex_method(dex_vm_g_java_method_source, current, method_name, signature,
                                          &method, &class_found, &method_found) != SSRNX_OK)
            return false;
        if (method_found) {
            if (out_method_flags) *out_method_flags = method.access_flags;
            if (out_class_flags) *out_class_flags = method.class_access_flags;
            ssrnx_package_release_dex_method(&method);
            return true;
        }
        char next[512] = {0};
        const bool more = class_found && method.super_class[0] &&
                          dex_vm_copy_text(next, sizeof(next), method.super_class);
        ssrnx_package_release_dex_method(&method);
        if (!more) return false;
        (void)dex_vm_copy_text(current, sizeof(current), next);
    }
    return false;
}

/* Registers every member a package class declares in the JNI model, so
 * reflection (Class.getDeclaredMethods/Fields/Constructors and the public
 * variants) sees the class_def's complete member set. Methods bind exactly as
 * a JNI GetMethodID would; fields get their class_def static_values constant
 * (JLS 12.4.2 step 6 applies these before <clinit>) or the Java default.
 * No bytecode runs here. */
static SsrnxResult dex_vm_dex_class_member_materializer(void *env, const char *class_name, void *user_data) {
    (void)user_data;
    if (!class_name || !dex_vm_g_java_method_source[0]) return SSRNX_OK;
    SsrnxDexMember *members = NULL;
    size_t count = 0;
    bool class_found = false;
    SsrnxResult rc = ssrnx_package_list_dex_class_members(dex_vm_g_java_method_source, class_name,
                                                          &members, &count, &class_found);
    if (rc != SSRNX_OK) return rc;
    if (!class_found) return ssrnx_jni_declare_modelled_interface_methods_host(class_name);
    size_t added = 0;
    for (size_t i = 0; i < count && rc == SSRNX_OK; ++i) {
        const SsrnxDexMember *m = &members[i];
        if (m->is_method && strcmp(m->name, "<clinit>") == 0) continue;
        if (ssrnx_jni_class_declares_member_host(class_name, m->name, m->signature,
                                                 m->is_static, m->is_method)) continue;
        if (m->is_method) {
            SsrnxJniMethodCallback callback = NULL;
            void *callback_user = NULL;
            bool found = false;
            rc = dex_vm_dex_jni_method_resolver(class_name, m->name, m->signature, m->is_static,
                                         &callback, &callback_user, &found, NULL);
            if (rc == SSRNX_OK && found && callback)
                rc = ssrnx_jni_register_method(class_name, m->name, m->signature, m->is_static,
                                               callback, callback_user);
        } else if (m->is_static) {
            SsrnxDexStaticValue initial = {0};
            bool init_class_found = false, init_field_found = false;
            rc = ssrnx_package_get_dex_static_field_initial_value(dex_vm_g_java_method_source, class_name,
                                                                   m->name, m->signature, &initial,
                                                                   &init_class_found, &init_field_found);
            SsrnxJniValue value = {0};
            void *temp_string = NULL;
            bool mismatch = false;
            if (rc == SSRNX_OK && init_field_found)
                rc = dex_vm_dex_static_initial_jni_value(env, m->signature, &initial, &value, &temp_string,
                                                  &mismatch);
            if (rc == SSRNX_OK && mismatch) {
                SSRNX_LOGCW("DEX", "unsupported/mismatched DEX static initializer class=%s name=%s sig=%s type=%d",
                            class_name, m->name, m->signature, (int)initial.type);
                rc = SSRNX_ERR_APK_UNSUPPORTED;
            }
            if (rc == SSRNX_OK)
                rc = ssrnx_jni_register_field(class_name, m->name, m->signature, true, value);
            if (temp_string) ssrnx_jni_delete_local_ref_host(env, temp_string);
        } else {
            const SsrnxJniValue zero = {0};
            rc = ssrnx_jni_register_field(class_name, m->name, m->signature, false, zero);
        }
        if (rc == SSRNX_OK) added++;
    }
    SSRNX_LOGCI("DEX", "declared members materialized class=%s members=%zu added=%zu rc=%d",
                class_name, count, added, (int)rc);
    ssrnx_package_free_dex_members(members, count);
    return rc;
}

const char *ssrnx_dex_java_method_source(void) {
    return dex_vm_g_java_method_source[0] ? dex_vm_g_java_method_source : NULL;
}

void ssrnx_dex_set_java_method_source(const char *source_path) {
    dex_vm_dex_class_init_reset();
    dex_vm_dex_static_owner_reset();
    memset(dex_vm_g_java_method_source, 0, sizeof(dex_vm_g_java_method_source));
    dex_vm_dex_jni_bindings_reset();
    if (source_path && source_path[0]) {
        if (!dex_vm_copy_text(dex_vm_g_java_method_source, sizeof(dex_vm_g_java_method_source), source_path)) {
            ssrnx_jni_set_method_resolver(NULL, NULL);
            ssrnx_jni_set_class_member_materializer(NULL, NULL);
            return;
        }
        ssrnx_jni_set_method_resolver(dex_vm_dex_jni_method_resolver, NULL);
        ssrnx_jni_set_class_member_materializer(dex_vm_dex_class_member_materializer, NULL);
    } else {
        ssrnx_jni_set_method_resolver(NULL, NULL);
        ssrnx_jni_set_class_member_materializer(NULL, NULL);
    }
}

#pragma pop_macro("NATIVE_A64_STACK_CAP")
#pragma pop_macro("DEX_WATCHDOG_POLL_MASK")
#pragma pop_macro("DEX_STATIC_OWNER_SLOTS")
#pragma pop_macro("DEX_STATIC_OWNER_KEY_CAP")
#pragma pop_macro("DEX_STACK_SNAPSHOT_MAX")
#pragma pop_macro("DEX_SOFT_STEP_DIAG")
#pragma pop_macro("DEX_PROF_SLOTS")
#pragma pop_macro("DEX_PROF_SAMPLE_US")
#pragma pop_macro("DEX_PROF_REPORT_MS")
#pragma pop_macro("DEX_PROF_METHOD_BUCKETS")
#pragma pop_macro("DEX_PREFLIGHT_MAX_UNRESOLVED")
#pragma pop_macro("DEX_PREFLIGHT_MAX_METHODS")
#pragma pop_macro("DEX_PREFLIGHT_IFACE_MAX")
#pragma pop_macro("DEX_NATIVE_STACK_RESERVE")
#pragma pop_macro("DEX_NATIVE_RESOLVE_CACHE_MAX")
#pragma pop_macro("DEX_MAX_REGS")
#pragma pop_macro("DEX_MAX_DEPTH")
#pragma pop_macro("DEX_LEDGER_CAP")
#pragma pop_macro("DEX_JNI_PROXY_CHUNK")
#pragma pop_macro("DEX_FIELD_TYPE_CAP")
#pragma pop_macro("DEX_ACC_SYNCHRONIZED")
#pragma pop_macro("DEX_ACC_STATIC")
#pragma pop_macro("DEX_ACC_PRIVATE")
#pragma pop_macro("DEX_ACC_NATIVE")
#pragma pop_macro("DEX_ACC_INTERFACE")
#pragma pop_macro("DEX_ACC_ABSTRACT")
#pragma pop_macro("A64_GP")
#pragma pop_macro("A64_FP")


/* ---- core_natives ---- */
#pragma push_macro("SSRNX_X509_MAX_DER")
#pragma push_macro("CORE_FN")
/* SSRNX boot natives: JNI implementations of native methods declared by the
 * SSRNX core library DEX (tools/libcore), bound by the runtime the way ART's
 * libjavacore registers libcore natives at boot. Each function uses the
 * ordinary JNI calling convention (JNIEnv*, jclass, args...). */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>




/* ---- com.apn64.core.Memory (raw memory behind JNI direct buffers) ---- */

static int8_t core_natives_memory_peek_byte(void *env, void *clazz, int64_t address) {
    (void)env; (void)clazz;
    return *(const volatile int8_t *)(uintptr_t)address;
}

static void core_natives_memory_poke_byte(void *env, void *clazz, int64_t address, int8_t value) {
    (void)env; (void)clazz;
    *(volatile int8_t *)(uintptr_t)address = value;
}

static void core_natives_memory_peek_byte_array(void *env, void *clazz, int64_t address, void *dst,
                                   int32_t offset, int32_t count) {
    (void)clazz;
    if (ssrnx_jni_byte_array_write(dst, offset, count, (const int8_t *)(uintptr_t)address) != SSRNX_OK)
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/ArrayIndexOutOfBoundsException", "peekByteArray");
}

static void core_natives_memory_poke_byte_array(void *env, void *clazz, int64_t address, void *src,
                                   int32_t offset, int32_t count) {
    (void)clazz;
    if (ssrnx_jni_byte_array_read(src, offset, count, (int8_t *)(uintptr_t)address) != SSRNX_OK)
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/ArrayIndexOutOfBoundsException", "pokeByteArray");
}

static void core_natives_memory_memmove(void *env, void *clazz, int64_t dst, int64_t src, int64_t count) {
    (void)env; (void)clazz;
    if (count > 0) memmove((void *)(uintptr_t)dst, (const void *)(uintptr_t)src, (size_t)count);
}

static int64_t core_natives_memory_array_base_address(void *env, void *clazz, void *array) {
    (void)clazz;
    void *data = ssrnx_jni_primitive_array_data_host(array);
    if (!data && ssrnx_jni_array_length_host(array) != 0) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/IllegalArgumentException", "not a primitive array");
        return 0;
    }
    return (int64_t)(uintptr_t)data;
}

/* ---- com.apn64.security.SsrnxX509Bridge ------------------------------- */

#define SSRNX_X509_MAX_DER (4u * 1024u * 1024u)

static bool core_natives_x509_input_read_all(void *env, void *input, uint8_t **out_data, size_t *out_size) {
    if (!env || !input || !out_data || !out_size) return false;
    *out_data = NULL;
    *out_size = 0;

    char class_name[256] = {0};
    if (!ssrnx_jni_object_class_name_host(input, class_name, sizeof(class_name)))
        return false;

    uint8_t *data = NULL;
    size_t size = 0, capacity = 0;
    for (;;) {
        SsrnxJniValue result = {0};
        bool found = false;
        SsrnxResult rc = ssrnx_jni_invoke_method_host(
            env, input, class_name, "read", "()I", false, NULL, &result, &found);
        if (rc != SSRNX_OK || !found) {
            free(data);
            return false;
        }
        if (ssrnx_jni_exception_pending_host(env)) {
            free(data);
            return false;
        }
        const int32_t value = result.i;
        if (value < 0) break;
        if (value > 255 || size >= SSRNX_X509_MAX_DER) {
            free(data);
            return false;
        }
        if (size == capacity) {
            size_t next = capacity ? capacity * 2u : 2048u;
            if (next > SSRNX_X509_MAX_DER) next = SSRNX_X509_MAX_DER;
            uint8_t *grown = realloc(data, next);
            if (!grown) {
                free(data);
                return false;
            }
            data = grown;
            capacity = next;
        }
        data[size++] = (uint8_t)value;
    }

    if (!size) {
        free(data);
        return false;
    }
    *out_data = data;
    *out_size = size;
    return true;
}

/* Android's X.509 CertificateFactory (Conscrypt) accepts PEM as well as DER:
 * the base64 body between "-----BEGIN CERTIFICATE-----" (or the X509 /
 * TRUSTED variants) and the matching END line. Decodes in place; false when
 * the data is not PEM. */
static bool core_natives_x509_pem_to_der(uint8_t *data, size_t *inout_size) {
    size_t size = *inout_size, pos = 0;
    while (pos < size && (data[pos] == ' ' || data[pos] == '\t' || data[pos] == '\r' || data[pos] == '\n')) ++pos;
    static const char begin[] = "-----BEGIN ";
    if (size - pos < sizeof(begin) - 1u || memcmp(data + pos, begin, sizeof(begin) - 1u) != 0) return false;
    while (pos < size && data[pos] != '\n') ++pos; /* rest of the BEGIN line */
    size_t out = 0;
    uint32_t acc = 0;
    unsigned bits = 0;
    for (; pos < size; ++pos) {
        const uint8_t c = data[pos];
        if (c == '-') break; /* END line */
        int v;
        if (c >= 'A' && c <= 'Z') v = c - 'A';
        else if (c >= 'a' && c <= 'z') v = c - 'a' + 26;
        else if (c >= '0' && c <= '9') v = c - '0' + 52;
        else if (c == '+') v = 62;
        else if (c == '/') v = 63;
        else continue; /* line breaks, '=' padding, whitespace */
        acc = (acc << 6) | (uint32_t)v;
        bits += 6u;
        if (bits >= 8u) {
            bits -= 8u;
            data[out++] = (uint8_t)(acc >> bits);
        }
    }
    if (!out) return false;
    *inout_size = out;
    return true;
}

static void *core_natives_x509_bridge_generate_certificate(void *env, void *clazz, void *input) {
    (void)clazz;
    if (!input) {
        (void)ssrnx_jni_throw_new_by_name(
            env, "java/security/cert/CertificateException", "input == null");
        return NULL;
    }

    uint8_t *der = NULL;
    size_t der_size = 0;
    if (!core_natives_x509_input_read_all(env, input, &der, &der_size)) {
        if (!ssrnx_jni_exception_pending_host(env))
            (void)ssrnx_jni_throw_new_by_name(
                env, "java/security/cert/CertificateException",
                "could not read X.509 certificate");
        return NULL;
    }
    (void)core_natives_x509_pem_to_der(der, &der_size);

    /* Validate the certificate shape and prove that the subject Name can be
     * located before exposing a certificate object. This does not establish
     * cryptographic trust. */
    uint8_t *subject = NULL;
    size_t subject_size = 0;
    SsrnxResult parse_rc = ssrnx_java_security_x509_name_der(
        der, der_size, true, &subject, &subject_size);
    free(subject);
    if (parse_rc != SSRNX_OK) {
        free(der);
        (void)ssrnx_jni_throw_new_by_name(
            env, "java/security/cert/CertificateException",
            "invalid X.509 DER");
        return NULL;
    }

    if (der_size > INT32_MAX) {
        free(der);
        (void)ssrnx_jni_throw_new_by_name(
            env, "java/security/cert/CertificateException",
            "X.509 certificate too large");
        return NULL;
    }

    void *encoded = ssrnx_jni_new_local_byte_array(env, (int32_t)der_size);
    if (!encoded || (der_size &&
        ssrnx_jni_byte_array_write(encoded, 0, (int32_t)der_size,
                                   (const int8_t *)der) != SSRNX_OK)) {
        free(der);
        if (encoded) ssrnx_jni_delete_local_ref_host(env, encoded);
        if (!ssrnx_jni_exception_pending_host(env))
            (void)ssrnx_jni_throw_new_by_name(env, "java/lang/OutOfMemoryError", "X.509 DER");
        return NULL;
    }
    free(der);

    void *certificate = ssrnx_jni_new_local_object(env, "java/security/cert/X509Certificate");
    if (!certificate) {
        ssrnx_jni_delete_local_ref_host(env, encoded);
        return NULL;
    }
    SsrnxJniValue value = {.l = encoded};
    if (ssrnx_jni_set_instance_field_value(
            certificate, "apn64Encoded", "[B", value) != SSRNX_OK) {
        ssrnx_jni_delete_local_ref_host(env, encoded);
        ssrnx_jni_delete_local_ref_host(env, certificate);
        (void)ssrnx_jni_throw_new_by_name(
            env, "java/security/cert/CertificateException",
            "X.509 runtime field unavailable");
        return NULL;
    }
    ssrnx_jni_delete_local_ref_host(env, encoded);
    return certificate;
}

typedef struct core_natives_CoreNative {
    const char *class_name;
    const char *name;
    const char *signature;
    void *fn;
} core_natives_CoreNative;

/* AOSP CRC32 uses zlib's finalized CRC convention. Keep JNI array access
 * bounded even for a large update; Java validates the public input range. */
static int32_t core_natives_zip_crc_byte(void *env, void *clazz, int32_t crc, int32_t byte) {
    (void)env; (void)clazz;
    const unsigned char value=(unsigned char)byte;
    return (int32_t)crc32((uint32_t)crc, &value, 1);
}
static int32_t core_natives_zip_crc_bytes(void *env, void *clazz, int32_t crc, void *array,
                             int32_t offset, int32_t count) {
    (void)clazz;
    unsigned char bytes[4096];
    while (count>0) {
        const int32_t size=count>(int32_t)sizeof(bytes) ? (int32_t)sizeof(bytes) : count;
        if (ssrnx_jni_byte_array_read(array,offset,size,(int8_t *)bytes)!=SSRNX_OK) {
            (void)ssrnx_jni_throw_new_by_name(env,"java/lang/ArrayIndexOutOfBoundsException","CRC32 input");
            break;
        }
        crc=(int32_t)crc32((uint32_t)crc,bytes,(uInt)size);
        offset+=size; count-=size;
    }
    return crc;
}

#define CORE_FN(f) ((void *)(uintptr_t)(f))

static const core_natives_CoreNative core_natives_k_core_natives[] = {
    {"com/apn64/security/Apn64Pbkdf2", "derive", "([B[BIILjava/lang/String;)[B", CORE_FN(ssrnx_java_pbkdf2_derive)},
    {"java/util/zip/CRC32", "update", "(II)I", CORE_FN(core_natives_zip_crc_byte)},
    {"java/util/zip/CRC32", "updateBytes", "(I[BII)I", CORE_FN(core_natives_zip_crc_bytes)},
    {"com/apn64/security/Apn64Hmac", "create", "(Ljava/lang/String;)V", CORE_FN(ssrnx_java_hmac_create)},
    {"com/apn64/security/Apn64Hmac", "engineGetMacLength", "()I", CORE_FN(ssrnx_java_hmac_length)},
    {"com/apn64/security/Apn64Hmac", "engineInit", "(Ljava/security/Key;Ljava/security/spec/AlgorithmParameterSpec;)V", CORE_FN(ssrnx_java_hmac_init)},
    {"com/apn64/security/Apn64Hmac", "engineUpdate", "(B)V", CORE_FN(ssrnx_java_hmac_update_byte)},
    {"com/apn64/security/Apn64Hmac", "engineUpdate", "([BII)V", CORE_FN(ssrnx_java_hmac_update_array)},
    {"com/apn64/security/Apn64Hmac", "engineDoFinal", "()[B", CORE_FN(ssrnx_java_hmac_final)},
    {"com/apn64/security/Apn64Hmac", "engineReset", "()V", CORE_FN(ssrnx_java_hmac_reset)},
    {"com/apn64/security/Apn64Hmac", "clone", "()Ljava/lang/Object;", CORE_FN(ssrnx_java_hmac_clone)},
    {"com/apn64/security/Apn64X509Bridge", "generateCertificate",
     "(Ljava/io/InputStream;)Ljava/security/cert/Certificate;",
     CORE_FN(core_natives_x509_bridge_generate_certificate)},
    {"com/apn64/core/Memory", "peekByte", "(J)B", CORE_FN(core_natives_memory_peek_byte)},
    {"com/apn64/core/Memory", "pokeByte", "(JB)V", CORE_FN(core_natives_memory_poke_byte)},
    {"com/apn64/core/Memory", "peekByteArray", "(J[BII)V", CORE_FN(core_natives_memory_peek_byte_array)},
    {"com/apn64/core/Memory", "pokeByteArray", "(J[BII)V", CORE_FN(core_natives_memory_poke_byte_array)},
    {"com/apn64/core/Memory", "memmove", "(JJJ)V", CORE_FN(core_natives_memory_memmove)},
    {"com/apn64/core/Memory", "arrayBaseAddress", "(Ljava/lang/Object;)J", CORE_FN(core_natives_memory_array_base_address)},
};

/* com/apn64/core/SQLite lives with the SQLite build (sqlite_host.c). */
extern bool ssrnx_sqlite_native_lookup(const char *name, const char *signature, void **out_fn)
    __attribute__((weak));

bool ssrnx_core_native_lookup(const char *class_name, const char *name, const char *signature,
                              void **out_fn) {
    if (!class_name || !name || !signature || !out_fn) return false;
    const bool core_class = strncmp(class_name, "com/apn64/core/", 15) == 0;
    const bool security_class = strncmp(class_name, "com/apn64/security/", 19) == 0;
    if (!core_class && !security_class && strcmp(class_name,"java/util/zip/CRC32")!=0) return false;
    if (strcmp(class_name, "com/apn64/core/SQLite") == 0)
        return ssrnx_sqlite_native_lookup && ssrnx_sqlite_native_lookup(name, signature, out_fn);
    for (size_t i = 0; i < sizeof(core_natives_k_core_natives) / sizeof(core_natives_k_core_natives[0]); ++i) {
        const core_natives_CoreNative *n = &core_natives_k_core_natives[i];
        if (strcmp(n->class_name, class_name) == 0 && strcmp(n->name, name) == 0 &&
            strcmp(n->signature, signature) == 0) {
            *out_fn = n->fn;
            return true;
        }
    }
    return false;
}

#pragma pop_macro("CORE_FN")
#pragma pop_macro("SSRNX_X509_MAX_DER")


/* ---- api_stubs ---- */
#pragma push_macro("API_NAME_CAP")
#pragma push_macro("API_WALK_MAX")


#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Generated: source/android/api30_table.c (tools/api_stubs/gen_api_table.py).
 * Rows, sorted by class name:
 *   C <class> <flags:i=interface a=abstract e=enum|-> <super> <iface,iface|->
 *   M<s|i|a> <name> <descriptor>         s=static i=instance a=abstract
 *   F<s|i|e> <name> <descriptor> [value] e=enum constant, value=primitive constant
 *                                        or a String constant's Java literal */
#include "android_api.inc"

#define API_NAME_CAP 512u
#define API_WALK_MAX 48u

typedef struct api_stubs_ApiClassRow {
    const char *members; /* first member row, or the next class row */
    char name[API_NAME_CAP];
    char flags[8];
    char super_name[API_NAME_CAP];
    char interfaces[1024]; /* comma-separated */
} api_stubs_ApiClassRow;

typedef enum api_stubs_ApiStubKind {
    API_STUB_METHOD = 0,
    API_STUB_CONSTRUCTOR,
    API_STUB_ENUM_VALUES,
    API_STUB_ENUM_VALUE_OF,
} api_stubs_ApiStubKind;

typedef struct api_stubs_ApiStubEntry {
    struct api_stubs_ApiStubEntry *next;
    atomic_bool reported;
    api_stubs_ApiStubKind kind;
    char owner[API_NAME_CAP];
    char name[160];
    char signature[768];
} api_stubs_ApiStubEntry;

static pthread_once_t api_stubs_g_index_once = PTHREAD_ONCE_INIT;
static const char **api_stubs_g_class_rows;
static size_t api_stubs_g_class_count;
static char *api_stubs_g_table;
static pthread_mutex_t api_stubs_g_bind_lock = PTHREAD_MUTEX_INITIALIZER;
static api_stubs_ApiStubEntry *api_stubs_g_entries;
static atomic_size_t api_stubs_g_bound;

static void api_stubs_build_index(void) {
    char *table = malloc(api_stubs_table_size + 1u);
    if (!table) return;
    const size_t size = api_stubs_table_size;
    if (!ssrnx_platform_decode(api_stubs_table_xz, sizeof(api_stubs_table_xz), table, size)) { free(table); return; }
    table[size] = '\0';
    const char *end = table + size;
    size_t count = 0;
    for (const char *p = table; p < end;) {
        if (p[0] == 'C' && p[1] == ' ') ++count;
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        p = nl ? nl + 1 : end;
    }
    api_stubs_g_class_rows = count ? calloc(count, sizeof(*api_stubs_g_class_rows)) : NULL;
    if (!api_stubs_g_class_rows) { free(table); return; }
    api_stubs_g_table = table;
    for (const char *p = table; p < end && api_stubs_g_class_count < count;) {
        if (p[0] == 'C' && p[1] == ' ') api_stubs_g_class_rows[api_stubs_g_class_count++] = p;
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        p = nl ? nl + 1 : end;
    }
}

/* Compares the class-name token of a "C " row with name (byte order, the
 * order the generator sorts in). */
static int api_stubs_row_name_cmp(const char *row, const char *name) {
    const unsigned char *a = (const unsigned char *)row + 2;
    const unsigned char *b = (const unsigned char *)name;
    while (*a != ' ' && *a == *b && *b) { ++a; ++b; }
    const int ca = *a == ' ' ? 0 : *a;
    return ca - (int)*b;
}

static const char *api_stubs_next_token(const char *p, char *out, size_t cap) {
    size_t n = 0;
    while (*p && *p != ' ' && *p != '\n') {
        if (n + 1u < cap) out[n++] = *p;
        ++p;
    }
    if (cap) out[n] = '\0';
    return *p == ' ' ? p + 1 : p;
}

static bool api_stubs_find_any_class_row(const char *name, api_stubs_ApiClassRow *out) {
    if (!name || !name[0]) return false;
    pthread_once(&api_stubs_g_index_once, api_stubs_build_index);
    size_t lo = 0, hi = api_stubs_g_class_count;
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2u;
        const int c = api_stubs_row_name_cmp(api_stubs_g_class_rows[mid], name);
        if (c == 0) {
            const char *p = api_stubs_g_class_rows[mid] + 2;
            p = api_stubs_next_token(p, out->name, sizeof(out->name));
            p = api_stubs_next_token(p, out->flags, sizeof(out->flags));
            p = api_stubs_next_token(p, out->super_name, sizeof(out->super_name));
            p = api_stubs_next_token(p, out->interfaces, sizeof(out->interfaces));
            if (strcmp(out->interfaces, "-") == 0) out->interfaces[0] = '\0';
            const char *nl = strchr(p, '\n');
            out->members = nl ? nl + 1 : p;
            return true;
        }
        if (c < 0) lo = mid + 1u;
        else hi = mid;
    }
    return false;
}

static bool api_stubs_row_flag(const api_stubs_ApiClassRow *row, char flag) {
    return strchr(row->flags, flag) != NULL;
}

/* Stub-able (android.*) rows only. 'h' rows (java.*, javax.*) carry the
 * declared type graph and never back inert classes or members. */
static bool api_stubs_find_class_row(const char *name, api_stubs_ApiClassRow *out) {
    return api_stubs_find_any_class_row(name, out) && !api_stubs_row_flag(out, 'h');
}

/* Finds one member row of a class. Returns the row start or NULL. */
static const char *api_stubs_find_member_row(const api_stubs_ApiClassRow *row, char kind, const char *name,
                                   const char *signature, bool is_static) {
    char token_name[160], token_sig[768];
    for (const char *p = row->members; *p && !(p[0] == 'C' && p[1] == ' ');) {
        const char *nl = strchr(p, '\n');
        if (p[0] == kind) {
            const char flag = p[1];
            const bool row_static = flag == 's' || flag == 'e';
            const char *q = api_stubs_next_token(p + 3, token_name, sizeof(token_name));
            (void)api_stubs_next_token(q, token_sig, sizeof(token_sig));
            if (row_static == is_static && strcmp(token_name, name) == 0 &&
                strcmp(token_sig, signature) == 0)
                return p;
        }
        if (!nl) break;
        p = nl + 1;
    }
    return NULL;
}

static void *api_stubs_ensure_env(void *env) { return env ? env : ssrnx_jni_current_env(); }

static bool api_stubs_registry_superclass(void *env, const char *class_name, char *out, size_t cap) {
    env = api_stubs_ensure_env(env);
    if (!env) return false;
    const bool pending = ssrnx_jni_exception_pending_host(env);
    void *clazz = ssrnx_jni_find_class_ref(env, class_name);
    if (!clazz) {
        if (!pending) ssrnx_jni_exception_clear_host(env);
        return false;
    }
    void *super_ref = ssrnx_jni_class_get_superclass_host(env, clazz);
    ssrnx_jni_delete_local_ref_host(env, clazz);
    if (!super_ref) return false;
    const bool ok = ssrnx_jni_class_ref_name_host(super_ref, out, cap);
    ssrnx_jni_delete_local_ref_host(env, super_ref);
    return ok;
}

/* JVMS 5.4.3.2/5.4.3.3 resolution over the declared platform surface: the
 * class, its superinterfaces, then its superclass. Classes the table does not
 * know (package or modeled classes) continue through the registry superclass. */
static const char *api_stubs_resolve_member(void *env, const char *start, char kind, const char *name,
                                  const char *signature, bool is_static,
                                  char *out_owner, size_t owner_cap, unsigned depth) {
    char cur[API_NAME_CAP];
    if (!start || depth > 8u || strlen(start) >= sizeof(cur)) return NULL;
    strcpy(cur, start);
    api_stubs_ApiClassRow *row = malloc(sizeof(*row));
    if (!row) return NULL;
    const char *found = NULL;
    for (unsigned walk = 0; walk < API_WALK_MAX && !found; ++walk) {
        char next[API_NAME_CAP] = {0};
        if (api_stubs_find_class_row(cur, row)) {
            found = api_stubs_find_member_row(row, kind, name, signature, is_static);
            if (found) {
                if (strlen(cur) >= owner_cap) found = NULL;
                else strcpy(out_owner, cur);
                break;
            }
            /* Static methods are not inherited from interfaces (JLS 8.4.8). */
            if (!(kind == 'M' && is_static)) {
                char ifaces[1024];
                strcpy(ifaces, row->interfaces);
                char *save = NULL;
                for (char *it = strtok_r(ifaces, ",", &save); it && !found;
                     it = strtok_r(NULL, ",", &save))
                    found = api_stubs_resolve_member(env, it, kind, name, signature, is_static,
                                           out_owner, owner_cap, depth + 1u);
                if (found) break;
            }
            if (api_stubs_row_flag(row, 'i')) break;
            strcpy(next, row->super_name);
        } else if (!api_stubs_registry_superclass(env, cur, next, sizeof(next))) {
            break;
        }
        if (!next[0] || strcmp(next, cur) == 0) break;
        strcpy(cur, next);
    }
    free(row);
    return found;
}

static api_stubs_ApiStubEntry *api_stubs_stub_entry(api_stubs_ApiStubKind kind, const char *owner, const char *name,
                                const char *signature) {
    for (api_stubs_ApiStubEntry *e = api_stubs_g_entries; e; e = e->next)
        if (e->kind == kind && strcmp(e->owner, owner) == 0 && strcmp(e->name, name) == 0 &&
            strcmp(e->signature, signature) == 0)
            return e;
    if (strlen(owner) >= sizeof(((api_stubs_ApiStubEntry *)0)->owner) ||
        strlen(name) >= sizeof(((api_stubs_ApiStubEntry *)0)->name) ||
        strlen(signature) >= sizeof(((api_stubs_ApiStubEntry *)0)->signature))
        return NULL;
    api_stubs_ApiStubEntry *e = calloc(1, sizeof(*e));
    if (!e) return NULL;
    e->kind = kind;
    strcpy(e->owner, owner);
    strcpy(e->name, name);
    strcpy(e->signature, signature);
    e->next = api_stubs_g_entries;
    api_stubs_g_entries = e;
    return e;
}

static bool api_stubs_ensure_class(void *env, const char *class_name) {
    env = api_stubs_ensure_env(env);
    if (!env) return false;
    const bool pending = ssrnx_jni_exception_pending_host(env);
    void *clazz = ssrnx_jni_find_class_ref(env, class_name);
    if (!clazz) {
        if (!pending) ssrnx_jni_exception_clear_host(env);
        return false;
    }
    ssrnx_jni_delete_local_ref_host(env, clazz);
    return true;
}

static SsrnxJniValue api_stubs_parse_constant(const char *signature, const char *text) {
    SsrnxJniValue v;
    memset(&v, 0, sizeof(v));
    if (!text || !*text) return v;
    switch (signature[0]) {
    case 'Z': v.z = strncmp(text, "true", 4) == 0; break;
    case 'B': v.b = (int8_t)strtol(text, NULL, 0); break;
    case 'C': v.c = (uint16_t)strtol(text, NULL, 0); break;
    case 'S': v.s = (int16_t)strtol(text, NULL, 0); break;
    case 'I': v.i = (int32_t)strtoll(text, NULL, 0); break;
    case 'J': v.j = strtoll(text, NULL, 0); break;
    case 'F': v.f = strtof(text, NULL); break;
    case 'D': v.d = strtod(text, NULL); break;
    default: break;
    }
    return v;
}

/* Decodes the Java string literal at p ("...", escapes as javac reads them)
 * into UTF-16. NULL on a malformed literal. */
static uint16_t *api_stubs_decode_java_literal(const char *p, int32_t *out_length) {
    if (*p != '"') return NULL;
    const char *end = strchr(p, '\n');
    const size_t cap = end ? (size_t)(end - p) : strlen(p);
    uint16_t *out = malloc((cap + 1u) * sizeof(*out));
    if (!out) return NULL;
    int32_t n = 0;
    for (++p; *p && *p != '\n'; ++p) {
        unsigned char c = (unsigned char)*p;
        if (c == '"') { *out_length = n; return out; }
        if (c == '\\') {
            const char e = *++p;
            if (e == 'u') {
                unsigned v = 0;
                for (int i = 0; i < 4; ++i) {
                    const char h = *++p;
                    const int digit = h >= '0' && h <= '9' ? h - '0' : h >= 'a' && h <= 'f' ? h - 'a' + 10
                                    : h >= 'A' && h <= 'F' ? h - 'A' + 10 : -1;
                    if (digit < 0) { free(out); return NULL; }
                    v = v * 16u + (unsigned)digit;
                }
                out[n++] = (uint16_t)v;
            } else {
                const char *esc = strchr("btnfr\"'\\", e);
                if (!e || !esc) { free(out); return NULL; }
                out[n++] = (uint16_t)("\b\t\n\f\r\"'\\"[esc - "btnfr\"'\\"]);
            }
            continue;
        }
        /* UTF-8 text to UTF-16 (current.txt is UTF-8). */
        uint32_t cp = c;
        int more = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : 0;
        if (more) cp = c & (0x3Fu >> more);
        while (more-- > 0 && (p[1] & 0xC0) == 0x80) cp = (cp << 6) | ((unsigned char)*++p & 0x3Fu);
        if (cp >= 0x10000u) {
            cp -= 0x10000u;
            out[n++] = (uint16_t)(0xD800u + (cp >> 10));
            out[n++] = (uint16_t)(0xDC00u + (cp & 0x3FFu));
        } else {
            out[n++] = (uint16_t)cp;
        }
    }
    free(out);
    return NULL;
}

static int32_t api_stubs_enum_ordinal(const api_stubs_ApiClassRow *row, const char *name) {
    char token[160];
    int32_t ordinal = 0;
    for (const char *p = row->members; *p && !(p[0] == 'C' && p[1] == ' ');) {
        const char *nl = strchr(p, '\n');
        if (p[0] == 'F' && p[1] == 'e') {
            (void)api_stubs_next_token(p + 3, token, sizeof(token));
            if (strcmp(token, name) == 0) return ordinal;
            ++ordinal;
        }
        if (!nl) break;
        p = nl + 1;
    }
    return -1;
}

/* Registers one declared field on owner (caller holds g_bind_lock). */
static bool api_stubs_bind_field_row(void *env, const char *owner, const char *row_text,
                           const char *name, const char *signature, bool is_static, bool quiet) {
    if (ssrnx_jni_class_declares_member_host(owner, name, signature, is_static, false)) return true;
    if (!api_stubs_ensure_class(env, owner)) return false;
    env = api_stubs_ensure_env(env);
    SsrnxJniValue value;
    memset(&value, 0, sizeof(value));
    void *local = NULL;
    if (row_text[1] == 'e') {
        /* Enum constants are distinct instances of the enum class carrying
         * name() and ordinal(). API text lists constants alphabetically, not
         * in declaration order, so ordinals are consistent but not AOSP's. */
        api_stubs_ApiClassRow row;
        if (!api_stubs_find_class_row(owner, &row)) return false;
        local = ssrnx_jni_new_local_object(env, owner);
        void *name_ref = local ? ssrnx_jni_new_local_string_utf(env, name) : NULL;
        if (!name_ref) {
            if (local) ssrnx_jni_delete_local_ref_host(env, local);
            return false;
        }
        SsrnxJniValue args[2];
        memset(args, 0, sizeof(args));
        args[0].l = name_ref;
        args[1].i = api_stubs_enum_ordinal(&row, name);
        SsrnxJniValue ignored;
        bool found = false;
        (void)ssrnx_jni_invoke_method_host(env, local, "java/lang/Enum", "<init>",
                                           "(Ljava/lang/String;I)V", false, args, &ignored, &found);
        ssrnx_jni_delete_local_ref_host(env, name_ref);
        value.l = local;
    } else {
        char token[160], sig[768];
        const char *p = api_stubs_next_token(row_text + 3, token, sizeof(token));
        p = api_stubs_next_token(p, sig, sizeof(sig));
        if (*p == '"' && strcmp(signature, "Ljava/lang/String;") == 0) {
            int32_t length = 0;
            uint16_t *chars = api_stubs_decode_java_literal(p, &length);
            local = chars ? ssrnx_jni_new_local_string_utf16(env, chars, length) : NULL;
            free(chars);
            if (!local) return false;
            value.l = local;
        } else if (*p && *p != '\n') {
            char constant[96];
            (void)api_stubs_next_token(p, constant, sizeof(constant));
            value = api_stubs_parse_constant(signature, constant);
        }
    }
    /* Field storage holds objects, not references: register the slot, then
     * store the object through the reference-converting setter. */
    SsrnxJniValue initial = value;
    if (local) memset(&initial, 0, sizeof(initial));
    SsrnxResult rc = ssrnx_jni_register_field(owner, name, signature, is_static, initial);
    if (rc == SSRNX_OK && local) rc = ssrnx_jni_set_static_field_value(owner, name, signature, value);
    if (local) ssrnx_jni_delete_local_ref_host(env, local);
    if (rc != SSRNX_OK) return false;
    atomic_fetch_add(&api_stubs_g_bound, 1u);
    if (!quiet)
        SSRNX_LOGCW("APISTUB", "platform field bound class=%s name=%s sig=%s kind=%s",
                    owner, name, signature, row_text[1] == 'e' ? "enum-constant" : "declared");
    return true;
}

/* Local reference to an enum's constant (bound on demand), by ordinal. */
static void *api_stubs_enum_constant_at(void *env, const char *owner, int32_t index, int32_t *out_count) {
    api_stubs_ApiClassRow row;
    if (!api_stubs_find_class_row(owner, &row)) return NULL;
    char token[160];
    char sig[API_NAME_CAP + 4];
    snprintf(sig, sizeof(sig), "L%s;", owner);
    void *result = NULL;
    int32_t ordinal = 0;
    for (const char *p = row.members; *p && !(p[0] == 'C' && p[1] == ' ');) {
        const char *nl = strchr(p, '\n');
        if (p[0] == 'F' && p[1] == 'e') {
            if (ordinal == index) {
                (void)api_stubs_next_token(p + 3, token, sizeof(token));
                pthread_mutex_lock(&api_stubs_g_bind_lock);
                const bool bound = api_stubs_bind_field_row(env, owner, p, token, sig, true, false);
                pthread_mutex_unlock(&api_stubs_g_bind_lock);
                SsrnxJniValue v;
                bool found = false;
                if (bound && ssrnx_jni_get_static_field_value(env, owner, token, sig, &v, &found) == SSRNX_OK &&
                    found)
                    result = v.l;
            }
            ++ordinal;
        }
        if (!nl) break;
        p = nl + 1;
    }
    if (out_count) *out_count = ordinal;
    return result;
}

static void *api_stubs_new_collection(void *env, const char *impl) {
    void *object = ssrnx_jni_new_local_object(env, impl);
    if (!object) return NULL;
    SsrnxJniValue ignored;
    bool found = false;
    if (ssrnx_jni_invoke_method_host(env, object, impl, "<init>", "()V", false, NULL,
                                     &ignored, &found) != SSRNX_OK || !found ||
        ssrnx_jni_exception_pending_host(env)) {
        ssrnx_jni_delete_local_ref_host(env, object);
        return NULL;
    }
    return object;
}

static void *api_stubs_empty_array(void *env, const char *descriptor) {
    switch (descriptor[1]) {
    case 'Z': return ssrnx_jni_new_local_boolean_array(env, 0);
    case 'B': return ssrnx_jni_new_local_byte_array(env, 0);
    case 'C': return ssrnx_jni_new_local_char_array(env, 0);
    case 'S': return ssrnx_jni_new_local_short_array(env, 0);
    case 'I': return ssrnx_jni_new_local_int_array(env, 0);
    case 'J': return ssrnx_jni_new_local_long_array(env, 0);
    case 'F': return ssrnx_jni_new_local_float_array(env, 0);
    case 'D': return ssrnx_jni_new_local_double_array(env, 0);
    default: return ssrnx_jni_new_local_reference_array(env, descriptor, 0);
    }
}

/* Neutral value of a declared return type. */
static SsrnxJniValue api_stubs_neutral_value(void *env, const char *descriptor) {
    SsrnxJniValue v;
    memset(&v, 0, sizeof(v));
    if (descriptor[0] == '[') {
        v.l = api_stubs_empty_array(env, descriptor);
        return v;
    }
    if (descriptor[0] != 'L') return v;
    char name[API_NAME_CAP];
    const size_t n = strlen(descriptor);
    if (n < 3 || n - 2u >= sizeof(name)) return v;
    memcpy(name, descriptor + 1, n - 2u);
    name[n - 2u] = '\0';
    static const struct { const char *type, *impl; } collections[] = {
        {"java/util/List", "java/util/ArrayList"},
        {"java/util/Collection", "java/util/ArrayList"},
        {"java/lang/Iterable", "java/util/ArrayList"},
        {"java/util/ArrayList", "java/util/ArrayList"},
        {"java/util/Set", "java/util/HashSet"},
        {"java/util/HashSet", "java/util/HashSet"},
        {"java/util/Map", "java/util/HashMap"},
        {"java/util/HashMap", "java/util/HashMap"},
    };
    for (size_t i = 0; i < sizeof(collections) / sizeof(collections[0]); ++i)
        if (strcmp(name, collections[i].type) == 0) {
            v.l = api_stubs_new_collection(env, collections[i].impl);
            return v;
        }
    api_stubs_ApiClassRow row;
    if (!api_stubs_find_class_row(name, &row) || api_stubs_row_flag(&row, 'i')) return v;
    if (api_stubs_row_flag(&row, 'e')) {
        v.l = api_stubs_enum_constant_at(env, name, 0, NULL);
        return v;
    }
    /* Only a table-only class can be instantiated without its constructor:
     * a modeled class expects the payload its constructor creates. */
    if (ssrnx_jni_class_is_platform_inert_host(name)) v.l = ssrnx_jni_new_local_object(env, name);
    return v;
}

static SsrnxJniValue api_stubs_enum_values(void *env, const api_stubs_ApiStubEntry *e) {
    SsrnxJniValue v;
    memset(&v, 0, sizeof(v));
    int32_t count = 0;
    void *first = api_stubs_enum_constant_at(env, e->owner, 0, &count);
    if (first) ssrnx_jni_delete_local_ref_host(env, first);
    char descriptor[API_NAME_CAP + 4];
    snprintf(descriptor, sizeof(descriptor), "[L%s;", e->owner);
    void *array = ssrnx_jni_new_local_reference_array(env, descriptor, count);
    if (!array) return v;
    for (int32_t i = 0; i < count; ++i) {
        void *item = api_stubs_enum_constant_at(env, e->owner, i, NULL);
        (void)ssrnx_jni_set_object_array_element_host(env, array, i, item);
        if (item) ssrnx_jni_delete_local_ref_host(env, item);
    }
    v.l = array;
    return v;
}

static SsrnxJniValue api_stubs_enum_value_of(void *env, const api_stubs_ApiStubEntry *e, const SsrnxJniValue *args,
                                   size_t arg_count) {
    SsrnxJniValue v;
    memset(&v, 0, sizeof(v));
    if (arg_count < 1 || !args[0].l) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/NullPointerException", "Name is null");
        return v;
    }
    char wanted[160];
    if (!ssrnx_jni_string_utf_copy(args[0].l, wanted, sizeof(wanted))) wanted[0] = '\0';
    api_stubs_ApiClassRow row;
    if (api_stubs_find_class_row(e->owner, &row)) {
        const int32_t ordinal = api_stubs_enum_ordinal(&row, wanted);
        if (ordinal >= 0) {
            v.l = api_stubs_enum_constant_at(env, e->owner, ordinal, NULL);
            return v;
        }
    }
    char message[400];
    snprintf(message, sizeof(message), "No enum constant %.200s.%.150s", e->owner, wanted);
    for (char *p = message; *p; ++p)
        if (*p == '/' || *p == '$') *p = '.';
    (void)ssrnx_jni_throw_new_by_name(env, "java/lang/IllegalArgumentException", message);
    return v;
}

/* A constructor runs the nearest modeled superclass constructor with the same
 * descriptor, so state a framework model keeps (e.g. LayoutParams width and
 * height) is still initialized; otherwise it only initializes Object. */
static void api_stubs_chain_constructor(void *env, void *token, const api_stubs_ApiStubEntry *e,
                              const SsrnxJniValue *args) {
    char cur[API_NAME_CAP], next[API_NAME_CAP];
    strcpy(cur, e->owner);
    for (unsigned walk = 0; walk < API_WALK_MAX; ++walk) {
        api_stubs_ApiClassRow row;
        if (api_stubs_find_class_row(cur, &row)) strcpy(next, row.super_name);
        else if (!api_stubs_registry_superclass(env, cur, next, sizeof(next))) return;
        if (!next[0] || strcmp(next, "java/lang/Object") == 0 || strcmp(next, cur) == 0) return;
        if (ssrnx_jni_class_declares_member_host(next, "<init>", e->signature, false, true)) {
            void *self = ssrnx_jni_new_local_from_token(env, token);
            if (!self) return;
            SsrnxJniValue ignored;
            bool found = false;
            (void)ssrnx_jni_invoke_method_host(env, self, next, "<init>", e->signature, false,
                                               args, &ignored, &found);
            ssrnx_jni_delete_local_ref_host(env, self);
            return;
        }
        strcpy(cur, next);
    }
}

static SsrnxJniValue api_stubs_api_stub_invoke(void *env, void *token, const SsrnxJniValue *args,
                                     size_t arg_count, void *user_data) {
    api_stubs_ApiStubEntry *e = (api_stubs_ApiStubEntry *)user_data;
    SsrnxJniValue v;
    memset(&v, 0, sizeof(v));
    if (!e) return v;
    if (!atomic_exchange(&e->reported, true))
        SSRNX_LOGCW("APISTUB", "inert platform member called class=%s method=%s sig=%s",
                    e->owner, e->name, e->signature);
    switch (e->kind) {
    case API_STUB_CONSTRUCTOR:
        api_stubs_chain_constructor(env, token, e, args);
        return v;
    case API_STUB_ENUM_VALUES:
        return api_stubs_enum_values(env, e);
    case API_STUB_ENUM_VALUE_OF:
        return api_stubs_enum_value_of(env, e, args, arg_count);
    case API_STUB_METHOD:
    default: {
        const char *ret = strrchr(e->signature, ')');
        if (!ret) return v;
        /* An instance method declared to return its own class is a fluent
         * setter/builder step (Intent.setType, Builder.setView): the platform
         * returns this, and callers chain on it. */
        const size_t owner_len = strlen(e->owner);
        if (token && ret[1] == 'L' && strncmp(ret + 2, e->owner, owner_len) == 0 &&
            ret[2 + owner_len] == ';' && ret[3 + owner_len] == '\0') {
            v.l = ssrnx_jni_new_local_from_token(env, token);
            return v;
        }
        return api_stubs_neutral_value(env, ret + 1);
    }
    }
}

static bool api_stubs_is_enum_synthetic(const char *class_name, const char *name, const char *signature,
                              bool is_static, api_stubs_ApiStubKind *out_kind) {
    if (!is_static) return false;
    api_stubs_ApiClassRow row;
    if (!api_stubs_find_class_row(class_name, &row) || !api_stubs_row_flag(&row, 'e')) return false;
    char expected[API_NAME_CAP + 32];
    snprintf(expected, sizeof(expected), "()[L%s;", class_name);
    if (strcmp(name, "values") == 0 && strcmp(signature, expected) == 0) {
        *out_kind = API_STUB_ENUM_VALUES;
        return true;
    }
    snprintf(expected, sizeof(expected), "(Ljava/lang/String;)L%s;", class_name);
    if (strcmp(name, "valueOf") == 0 && strcmp(signature, expected) == 0) {
        *out_kind = API_STUB_ENUM_VALUE_OF;
        return true;
    }
    return false;
}

bool ssrnx_api_class_info(const char *class_name, char *out_super, size_t super_cap,
                          char *out_interfaces, size_t interfaces_cap, bool *out_is_interface) {
    api_stubs_ApiClassRow row;
    if (!api_stubs_find_class_row(class_name, &row)) return false;
    if (out_super && super_cap) {
        if (strlen(row.super_name) >= super_cap) return false;
        strcpy(out_super, row.super_name);
    }
    if (out_interfaces && interfaces_cap) {
        if (strlen(row.interfaces) >= interfaces_cap) return false;
        strcpy(out_interfaces, row.interfaces);
        for (char *p = out_interfaces; *p; ++p)
            if (*p == ',') *p = '\n';
    }
    if (out_is_interface) *out_is_interface = api_stubs_row_flag(&row, 'i');
    return true;
}

static bool api_stubs_bind_method(void *env, const char *class_name, const char *name,
                        const char *signature, bool is_static,
                        char *out_owner, size_t owner_cap, bool quiet) {
    if (!class_name || !name || !signature || !out_owner || !owner_cap) return false;
    env = api_stubs_ensure_env(env);
    char owner[API_NAME_CAP];
    api_stubs_ApiStubKind kind = API_STUB_METHOD;
    if (api_stubs_is_enum_synthetic(class_name, name, signature, is_static, &kind)) {
        strcpy(owner, class_name);
    } else {
        if (!api_stubs_resolve_member(env, class_name, 'M', name, signature, is_static, owner,
                            sizeof(owner), 0u))
            return false;
        if (strcmp(name, "<init>") == 0) {
            /* Constructors are not inherited (JLS 8.8). */
            if (strcmp(owner, class_name) != 0) return false;
            kind = API_STUB_CONSTRUCTOR;
        }
    }
    if (strlen(owner) >= owner_cap) return false;
    strcpy(out_owner, owner);
    if (ssrnx_jni_class_declares_member_host(owner, name, signature, is_static, true)) return true;
    if (!api_stubs_ensure_class(env, owner)) return false;
    pthread_mutex_lock(&api_stubs_g_bind_lock);
    api_stubs_ApiStubEntry *e = api_stubs_stub_entry(kind, owner, name, signature);
    SsrnxResult rc = e ? ssrnx_jni_register_method(owner, name, signature, is_static,
                                                   api_stubs_api_stub_invoke, e)
                       : SSRNX_ERR_NO_MEMORY;
    pthread_mutex_unlock(&api_stubs_g_bind_lock);
    if (rc != SSRNX_OK) return false;
    atomic_fetch_add(&api_stubs_g_bound, 1u);
    if (!quiet)
        SSRNX_LOGCW("APISTUB", "platform method bound class=%s method=%s sig=%s static=%d requested=%s",
                    owner, name, signature, is_static ? 1 : 0, class_name);
    return true;
}

bool ssrnx_api_stub_bind_method(void *env, const char *class_name, const char *name,
                                const char *signature, bool is_static,
                                char *out_owner, size_t owner_cap) {
    return api_stubs_bind_method(env, class_name, name, signature, is_static, out_owner, owner_cap, false);
}

bool ssrnx_api_stub_bind_field(void *env, const char *class_name, const char *name,
                               const char *signature, bool is_static,
                               char *out_owner, size_t owner_cap) {
    if (!class_name || !name || !signature || !out_owner || !owner_cap) return false;
    env = api_stubs_ensure_env(env);
    char owner[API_NAME_CAP];
    const char *row_text = api_stubs_resolve_member(env, class_name, 'F', name, signature, is_static,
                                          owner, sizeof(owner), 0u);
    if (!row_text || strlen(owner) >= owner_cap) return false;
    strcpy(out_owner, owner);
    pthread_mutex_lock(&api_stubs_g_bind_lock);
    const bool ok = api_stubs_bind_field_row(env, owner, row_text, name, signature, is_static, false);
    pthread_mutex_unlock(&api_stubs_g_bind_lock);
    return ok;
}

/* Binds every member the table declares on class_name that JNI resolution
 * does not reach yet (a framework model keeps precedence), so reflection over
 * a platform class lists its declared surface: Unity's ReflectionHelper looks
 * up Notification.EXTRA_TITLE through getDeclaredFields(). Methods of platform
 * interfaces stay unbound (proxies and implementors dispatch them). */
size_t ssrnx_api_stub_declare_members(void *env, const char *class_name) {
    api_stubs_ApiClassRow *row = malloc(sizeof(*row));
    if (!row) return 0;
    if (!api_stubs_find_class_row(class_name, row)) { free(row); return 0; }
    env = api_stubs_ensure_env(env);
    const bool interface = api_stubs_row_flag(row, 'i');
    size_t methods = 0, fields = 0;
    char name[160], sig[768], owner[API_NAME_CAP];
    for (const char *p = row->members; *p && !(p[0] == 'C' && p[1] == ' ');) {
        const char *nl = strchr(p, '\n');
        const char kind = p[0], flag = p[1];
        const char *q = api_stubs_next_token(p + 3, name, sizeof(name));
        (void)api_stubs_next_token(q, sig, sizeof(sig));
        if (kind == 'M' && !interface) {
            const bool is_static = flag == 's';
            const bool constructor = strcmp(name, "<init>") == 0;
            const bool present = constructor
                ? ssrnx_jni_class_declares_member_host(class_name, name, sig, false, true)
                : ssrnx_jni_class_resolves_member_host(class_name, name, sig, is_static, true);
            if (!present && api_stubs_bind_method(env, class_name, name, sig, is_static, owner, sizeof(owner), true))
                ++methods;
        } else if (kind == 'F') {
            const bool is_static = flag == 's' || flag == 'e';
            if (!ssrnx_jni_class_resolves_member_host(class_name, name, sig, is_static, false)) {
                pthread_mutex_lock(&api_stubs_g_bind_lock);
                if (api_stubs_bind_field_row(env, class_name, p, name, sig, is_static, true)) ++fields;
                pthread_mutex_unlock(&api_stubs_g_bind_lock);
            }
        }
        if (!nl) break;
        p = nl + 1;
    }
    free(row);
    if (methods || fields)
        SSRNX_LOGCI("APISTUB", "platform members declared for reflection class=%s methods=%zu fields=%zu",
                    class_name, methods, fields);
    return methods + fields;
}

size_t ssrnx_api_stub_bound_count(void) { return atomic_load(&api_stubs_g_bound); }

bool ssrnx_api_declared_supertypes(const char *class_name, char *out_super, size_t super_cap,
                                   char *out_interfaces, size_t interfaces_cap,
                                   bool *out_is_interface) {
    api_stubs_ApiClassRow row;
    if (!api_stubs_find_any_class_row(class_name, &row)) return false;
    if (strlen(row.super_name) >= super_cap || strlen(row.interfaces) >= interfaces_cap) return false;
    strcpy(out_super, row.super_name);
    strcpy(out_interfaces, row.interfaces);
    for (char *p = out_interfaces; *p; ++p)
        if (*p == ',') *p = '\n';
    if (out_is_interface) *out_is_interface = api_stubs_row_flag(&row, 'i');
    return true;
}

#pragma pop_macro("API_WALK_MAX")
#pragma pop_macro("API_NAME_CAP")

/* ---- android_compat ---- */


#include <string.h>

static const SsrnxAndroidCompatProfile android_compat_k_default_profile = {
    .platform_api = SSRNX_ANDROID_PLATFORM_API_DEFAULT,
    .min_sdk = SSRNX_ANDROID_MIN_SDK_DEFAULT,
    .target_sdk = SSRNX_ANDROID_TARGET_SDK_DEFAULT,
    .abi = SSRNX_ANDROID_ABI_ARM64_V8A,
};

static SsrnxAndroidCompatProfile android_compat_g_profile = {
    .platform_api = SSRNX_ANDROID_PLATFORM_API_DEFAULT,
    .min_sdk = SSRNX_ANDROID_MIN_SDK_DEFAULT,
    .target_sdk = SSRNX_ANDROID_TARGET_SDK_DEFAULT,
    .abi = SSRNX_ANDROID_ABI_ARM64_V8A,
};
static bool android_compat_g_profile_configured;

SsrnxAndroidCompatProfile ssrnx_android_compat_profile_make(int32_t platform_api,
                                                            int32_t min_sdk,
                                                            int32_t target_sdk,
                                                            SsrnxAndroidAbi abi) {
    SsrnxAndroidCompatProfile profile = android_compat_k_default_profile;
    if (platform_api > 0) profile.platform_api = platform_api;
    if (min_sdk > 0) profile.min_sdk = min_sdk;
    if (target_sdk > 0) profile.target_sdk = target_sdk;
    if (abi != SSRNX_ANDROID_ABI_UNKNOWN) profile.abi = abi;
    return profile;
}

void ssrnx_android_compat_set_profile(const SsrnxAndroidCompatProfile *profile) {
    android_compat_g_profile = profile
        ? ssrnx_android_compat_profile_make(profile->platform_api,
                                            profile->min_sdk,
                                            profile->target_sdk,
                                            profile->abi)
        : android_compat_k_default_profile;
    android_compat_g_profile_configured = profile != NULL;
}

void ssrnx_android_compat_reset_profile(void) {
    android_compat_g_profile = android_compat_k_default_profile;
    android_compat_g_profile_configured = false;
}

void ssrnx_android_compat_get_profile(SsrnxAndroidCompatProfile *out_profile) {
    if (out_profile) *out_profile = android_compat_g_profile;
}

bool ssrnx_android_compat_profile_is_configured(void) {
    return android_compat_g_profile_configured;
}

int32_t ssrnx_android_compat_platform_api(void) { return android_compat_g_profile.platform_api; }
int32_t ssrnx_android_compat_min_sdk(void) { return android_compat_g_profile.min_sdk; }
int32_t ssrnx_android_compat_target_sdk(void) { return android_compat_g_profile.target_sdk; }
SsrnxAndroidAbi ssrnx_android_compat_abi(void) { return android_compat_g_profile.abi; }

const char *ssrnx_android_compat_abi_name(SsrnxAndroidAbi abi) {
    switch (abi) {
        case SSRNX_ANDROID_ABI_ARM64_V8A: return "arm64-v8a";
        default: return "unknown";
    }
}

bool ssrnx_android_compat_platform_at_least(int32_t api_level) {
    return api_level > 0 && android_compat_g_profile.platform_api >= api_level;
}

bool ssrnx_android_compat_target_at_least(int32_t api_level) {
    return api_level > 0 && android_compat_g_profile.target_sdk >= api_level;
}

bool ssrnx_android_compat_target_before(int32_t api_level) {
    return api_level > 0 && android_compat_g_profile.target_sdk < api_level;
}

bool ssrnx_android_compat_min_sdk_supported(void) {
    return android_compat_g_profile.min_sdk <= android_compat_g_profile.platform_api;
}

bool ssrnx_android_compat_provider_default_exported(void) {
    return ssrnx_android_compat_target_before(17);
}


const size_t ssrnx_core_library_dex_size = 215740;
__attribute__((aligned(8))) const uint8_t ssrnx_core_library_dex[] = {
#include "core_library_dex.inc"
};
const size_t ssrnx_core_library_dex_compressed_size = sizeof(ssrnx_core_library_dex);

const size_t ssrnx_aosp_libcore_dex_size = 467508;
__attribute__((aligned(8))) const uint8_t ssrnx_aosp_libcore_dex[] = {
#include "aosp_libcore_dex.inc"
};
const size_t ssrnx_aosp_libcore_dex_compressed_size = sizeof(ssrnx_aosp_libcore_dex);


/* ---- optional_sdk ---- */
#pragma push_macro("FAILED_TASK_CALLBACK")
#pragma push_macro("UMP_FORM_ERROR")
#pragma push_macro("UMP_FORM_ERROR_INTERNAL")


#include <string.h>



static bool optional_sdk_starts_with(const char *text, const char *prefix) {
    if (!text || !prefix) return false;
    const size_t n = strlen(prefix);
    return strncmp(text, prefix, n) == 0;
}

static bool optional_sdk_is_known_ad_sdk(const char *class_name) {
    /* Relocated dependencies implement ordinary library contracts and can be
     * shared by non-ad callers. Their enclosing SDK namespace is not evidence
     * that a call requests an unavailable advertising service. R8 outlines
     * also hold shared API forwarding code after optimization; execute their
     * bodies so policy applies to the actual service endpoint they invoke. */
    if (!class_name || strstr(class_name, "/shadow/") || strstr(class_name, "/shaded/") ||
        strstr(class_name, "$$ExternalSyntheticApiModelOutline") ||
        strstr(class_name, "$$ExternalSyntheticOutline"))
        return false;
    return optional_sdk_starts_with(class_name, "com/facebook/ads/") ||
           optional_sdk_starts_with(class_name, "com/applovin/") ||
           optional_sdk_starts_with(class_name, "com/google/android/gms/ads/") ||
           optional_sdk_starts_with(class_name, "com/google/android/libraries/ads/mobile/sdk/") ||
           optional_sdk_starts_with(class_name, "com/google/android/ump/") ||
           optional_sdk_starts_with(class_name, "com/unity3d/ads/") ||
           optional_sdk_starts_with(class_name, "com/unity3d/ironsourceads/") ||
           optional_sdk_starts_with(class_name, "com/vungle/") ||
           optional_sdk_starts_with(class_name, "com/ironsource/") ||
           optional_sdk_starts_with(class_name, "com/fyber/") ||
           optional_sdk_starts_with(class_name, "com/inmobi/") ||
           optional_sdk_starts_with(class_name, "com/amazon/device/ads/") ||
           optional_sdk_starts_with(class_name, "com/bytedance/sdk/openadsdk/") ||
           optional_sdk_starts_with(class_name, "com/qq/e/ads/") ||
           optional_sdk_starts_with(class_name, "com/kpadplayer/sdk/ads/") ||
           optional_sdk_starts_with(class_name, "com/mbridge/msdk/") ||
           optional_sdk_starts_with(class_name, "com/chartboost/") ||
           optional_sdk_starts_with(class_name, "com/startapp/") ||
           optional_sdk_starts_with(class_name, "com/tapjoy/") ||
           optional_sdk_starts_with(class_name, "com/adcolony/") ||
           optional_sdk_starts_with(class_name, "com/yandex/mobile/ads/") ||
           optional_sdk_starts_with(class_name, "com/huawei/hms/ads/") ||
           optional_sdk_starts_with(class_name, "com/smaato/sdk/") ||
           optional_sdk_starts_with(class_name, "com/moloco/sdk/") ||
           optional_sdk_starts_with(class_name, "com/ogury/") ||
           optional_sdk_starts_with(class_name, "net/pubnative/lite/sdk/");
}

static bool optional_sdk_is_known_external_platform_sdk(const char *class_name) {
    /* These SDKs are client libraries for external Android platform services
     * that SSRNX does not provide. Treat the service as unavailable rather
     * than executing packaged implementation internals that ultimately expect
     * Play Store / Play Services Binder endpoints. */
    return (class_name &&
            (strcmp(class_name, "com/google/android/gms/auth/api/signin/GoogleSignIn") == 0 ||
             strcmp(class_name, "com/google/android/gms/auth/api/signin/GoogleSignInClient") == 0)) ||
           optional_sdk_starts_with(class_name, "com/google/android/gms/games/") ||
           optional_sdk_starts_with(class_name, "com/android/billingclient/api/") ||
           optional_sdk_starts_with(class_name, "com/google/android/gms/internal/play_billing/");
}

static bool optional_sdk_is_known_unavailable_sdk(const char *class_name) {
    return optional_sdk_is_known_ad_sdk(class_name) || optional_sdk_is_known_external_platform_sdk(class_name);
}

const char *ssrnx_optional_sdk_unavailable_reason(const char *class_name) {
    if (optional_sdk_is_known_ad_sdk(class_name)) return "unsupported-ad-monetization-sdk";
    if (optional_sdk_is_known_external_platform_sdk(class_name)) return "unsupported-platform-service-sdk";
    return NULL;
}

bool ssrnx_optional_sdk_is_unavailable_class(const char *class_name) {
    return ssrnx_optional_sdk_unavailable_reason(class_name) != NULL;
}

bool ssrnx_optional_sdk_skip_class_initialization(const char *class_name) {
    /* External services may be unavailable while their packaged API metadata
     * (Scope, Api, response constants) is still needed by ordinary Java code.
     * Only the advertising black hole suppresses all initialization. */
    return optional_sdk_is_known_ad_sdk(class_name);
}

static void *optional_sdk_stub_object(void *env, const char *class_name) {
    if (!env || !class_name) return NULL;
    /* Package metadata may later reconcile the real superclass.  Registering
     * provisionally keeps these compatibility objects generic and avoids
     * claiming ownership of an app/provider class hierarchy. */
    if (ssrnx_jni_register_class_provisional(class_name, "java/lang/Object") != SSRNX_OK)
        return NULL;
    return ssrnx_jni_new_local_object(env, class_name);
}

static SsrnxJniValue optional_sdk_object_result(void *ref) {
    SsrnxJniValue value = {0};
    value.l = ref;
    return value;
}

static SsrnxJniValue optional_sdk_bool_result(bool value) {
    SsrnxJniValue out = {0};
    out.z = value ? 1 : 0;
    return out;
}

/* Google Task objects produced by an unavailable external service are modeled
 * as already-completed failures.  The marker lives on the JNI object itself so
 * ordinary com.google.android.gms.tasks.Task instances remain untouched. */
static const char optional_sdk_g_unavailable_task_marker;

static void *optional_sdk_unavailable_api_status(void *env, int code) {
    (void)ssrnx_jni_register_class_provisional("com/google/android/gms/common/api/Status", "java/lang/Object");
    void *status=ssrnx_jni_new_local_object(env,"com/google/android/gms/common/api/Status");
    SsrnxJniValue arg={.i=code},ignored={0};bool found=false;
    if(status)(void)ssrnx_jni_invoke_method_host(env,status,"com/google/android/gms/common/api/Status","<init>","(I)V",false,&arg,&ignored,&found);
    if(!found||ssrnx_jni_exception_pending_host(env)) {
        if(status)ssrnx_jni_delete_local_ref_host(env,status);
        return NULL;
    }
    return status;
}

static void *optional_sdk_unavailable_task_object(void *env, int status_code) {
    void *task = optional_sdk_stub_object(env, "com/google/android/gms/tasks/Task");
    if (!task) return NULL;
    (void)ssrnx_jni_set_object_host_data(task, (void *)&optional_sdk_g_unavailable_task_marker);
    void *error = NULL;
    /* Use the packaged API's real value constructors when available. Host
     * fixtures without the SDK retain a generic failure, never a success. */
    (void)ssrnx_jni_register_class_provisional("com/google/android/gms/common/api/ApiException", "java/lang/Exception");
    void *status = optional_sdk_unavailable_api_status(env,status_code);
    SsrnxJniValue arg = {0}, ignored = {0}; bool found = false;
    if (status && !ssrnx_jni_exception_pending_host(env)) {
        error = ssrnx_jni_new_local_object(env, "com/google/android/gms/common/api/ApiException");
        arg.l = status; found = false;
        if (error) (void)ssrnx_jni_invoke_method_host(env, error, "com/google/android/gms/common/api/ApiException", "<init>", "(Lcom/google/android/gms/common/api/Status;)V", false, &arg, &ignored, &found);
        if (!found && error) {ssrnx_jni_delete_local_ref_host(env,error);error=NULL;}
    }
    if (status) ssrnx_jni_delete_local_ref_host(env,status);
    if (ssrnx_jni_exception_pending_host(env)) {
        if(error)ssrnx_jni_delete_local_ref_host(env,error);
        ssrnx_jni_delete_local_ref_host(env,task);return NULL;
    }
    if (!error) error = ssrnx_jni_new_local_object(env,"java/lang/Exception");
    bool retained = error && ssrnx_jni_register_field("com/google/android/gms/tasks/Task", "apn64Failure", "Ljava/lang/Exception;", false, (SsrnxJniValue){0}) == SSRNX_OK &&
        ssrnx_jni_set_instance_field_value(task,"apn64Failure","Ljava/lang/Exception;",(SsrnxJniValue){.l=error}) == SSRNX_OK;
    if(error)ssrnx_jni_delete_local_ref_host(env,error);
    if(!retained){ssrnx_jni_delete_local_ref_host(env,task);return NULL;}
    return task;
}

static void *optional_sdk_unavailable_task_error(void *env, void *task) {
    SsrnxJniValue error={0};bool found=false;
    (void)ssrnx_jni_get_instance_field_value(env,task,"com/google/android/gms/tasks/Task","apn64Failure","Ljava/lang/Exception;",&error,&found);
    return found ? error.l : NULL;
}

static bool optional_sdk_is_unavailable_task(void *receiver_ref) {
    return receiver_ref &&
           ssrnx_jni_get_object_host_data(receiver_ref) == (void *)&optional_sdk_g_unavailable_task_marker;
}

#define FAILED_TASK_CALLBACK "apn64/platform/FailedTaskCallback"
static SsrnxJniValue optional_sdk_failed_task_callback(void *env, void *token,
    const SsrnxJniValue *args, size_t count, void *user) {
    (void)args; (void)count; (void)user;
    void *self = ssrnx_jni_new_local_from_token(env, token);
    SsrnxJniValue listener = {0}, argument = {0}, failure = {0}, scoped = {0}, ignored = {0};
    bool found = false;
    (void)ssrnx_jni_get_instance_field_value(env, self, FAILED_TASK_CALLBACK, "listener", "Ljava/lang/Object;", &listener, &found);
    (void)ssrnx_jni_get_instance_field_value(env, self, FAILED_TASK_CALLBACK, "argument", "Ljava/lang/Object;", &argument, &found);
    (void)ssrnx_jni_get_instance_field_value(env, self, FAILED_TASK_CALLBACK, "failure", "Z", &failure, &found);
    (void)ssrnx_jni_get_instance_field_value(env, self, FAILED_TASK_CALLBACK, "scoped", "Z", &scoped, &found);
    const SsrnxAndroidActivityState state = ssrnx_android_activity_state();
    char class_name[512];
    if (listener.l && (!scoped.z || state < SSRNX_ANDROID_ACTIVITY_STOPPED) &&
        ssrnx_jni_object_class_name_host(listener.l, class_name, sizeof(class_name))) {
        (void)ssrnx_jni_invoke_method_host(env, listener.l, class_name,
            failure.z ? "onFailure" : "onComplete",
            failure.z ? "(Ljava/lang/Exception;)V" : "(Lcom/google/android/gms/tasks/Task;)V",
            false, &argument, &ignored, &found);
    }
    /* Managed object fields retain callback references while queued. */
    SsrnxJniValue empty = {0};
    (void)ssrnx_jni_set_instance_field_value(self, "listener", "Ljava/lang/Object;", empty);
    (void)ssrnx_jni_set_instance_field_value(self, "argument", "Ljava/lang/Object;", empty);
    if (listener.l) ssrnx_jni_delete_local_ref_host(env, listener.l);
    if (argument.l) ssrnx_jni_delete_local_ref_host(env, argument.l);
    if (self) ssrnx_jni_delete_local_ref_host(env, self);
    return empty;
}

static bool optional_sdk_schedule_failed_task_listener(void *env, void *task, const char *signature,
    const SsrnxJniValue *args, size_t count, bool failure) {
    void *listener = count ? args[count - 1].l : NULL;
    if (!listener) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/NullPointerException", "listener");
        return false;
    }
    const bool executor = strncmp(signature, "(Ljava/util/concurrent/Executor;", strlen("(Ljava/util/concurrent/Executor;")) == 0;
    const bool scoped = strncmp(signature, "(Landroid/app/Activity;", strlen("(Landroid/app/Activity;")) == 0;
    if (executor && !args[0].l) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/NullPointerException", "executor");
        return false;
    }
    if (ssrnx_jni_register_class(FAILED_TASK_CALLBACK, "java/lang/Object") != SSRNX_OK ||
        ssrnx_jni_register_method(FAILED_TASK_CALLBACK, "run", "()V", false, optional_sdk_failed_task_callback, NULL) != SSRNX_OK)
        return false;
    static const struct {const char *name, *signature;} fields[] = {
        {"listener", "Ljava/lang/Object;"}, {"argument", "Ljava/lang/Object;"},
        {"failure", "Z"}, {"scoped", "Z"}};
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
        if (ssrnx_jni_register_field(FAILED_TASK_CALLBACK, fields[i].name, fields[i].signature, false, (SsrnxJniValue){0}) != SSRNX_OK)
            return false;
    void *runnable = ssrnx_jni_new_local_object(env, FAILED_TASK_CALLBACK);
    if (!runnable) return false;
    SsrnxJniValue value = {.l=listener}, ignored = {0};
    (void)ssrnx_jni_set_instance_field_value(runnable, "listener", "Ljava/lang/Object;", value);
    value.l = failure ? optional_sdk_unavailable_task_error(env, task) : task;
    (void)ssrnx_jni_set_instance_field_value(runnable, "argument", "Ljava/lang/Object;", value);
    if (failure && value.l) ssrnx_jni_delete_local_ref_host(env, value.l);
    value.z = failure;
    (void)ssrnx_jni_set_instance_field_value(runnable, "failure", "Z", value);
    value.z = scoped;
    (void)ssrnx_jni_set_instance_field_value(runnable, "scoped", "Z", value);
    bool found = false, ok = false;
    SsrnxJniValue callback = {.l=runnable};
    if (executor) {
        char class_name[512];
        if (ssrnx_jni_object_class_name_host(args[0].l, class_name, sizeof(class_name)))
            ok = ssrnx_jni_invoke_method_host(env, args[0].l, class_name, "execute",
                "(Ljava/lang/Runnable;)V", false, &callback, &ignored, &found) == SSRNX_OK && found;
    } else {
        SsrnxJniValue looper = {0};
        ok = ssrnx_jni_invoke_method_host(env, NULL, "android/os/Looper", "getMainLooper",
            "()Landroid/os/Looper;", true, NULL, &looper, &found) == SSRNX_OK && found && looper.l;
        void *handler = ok ? ssrnx_jni_new_local_object(env, "android/os/Handler") : NULL;
        if (handler) {
            ok = ssrnx_jni_invoke_method_host(env, handler, "android/os/Handler", "<init>",
                "(Landroid/os/Looper;)V", false, &looper, &ignored, &found) == SSRNX_OK && found;
            if (ok) ok = ssrnx_jni_invoke_method_host(env, handler, "android/os/Handler", "post",
                "(Ljava/lang/Runnable;)Z", false, &callback, &ignored, &found) == SSRNX_OK && found && ignored.z;
            ssrnx_jni_delete_local_ref_host(env, handler);
        } else ok = false;
        if (looper.l) ssrnx_jni_delete_local_ref_host(env, looper.l);
    }
    ssrnx_jni_delete_local_ref_host(env, runnable);
    return ok;
}

static const char *optional_sdk_signature_return_descriptor(const char *signature) {
    const char *close = signature ? strrchr(signature, ')') : NULL;
    return close && close[1] ? close + 1 : NULL;
}

static bool optional_sdk_descriptor_class_name(const char *descriptor, char *out, size_t cap) {
    if (!descriptor || !out || cap < 2 || descriptor[0] != 'L') return false;
    const char *semi = strchr(descriptor, ';');
    if (!semi || semi == descriptor + 1) return false;
    size_t n = (size_t)(semi - (descriptor + 1));
    if (n + 1 > cap) return false;
    memcpy(out, descriptor + 1, n);
    out[n] = '\0';
    return true;
}

static bool optional_sdk_text_contains_ci(const char *text, const char *needle) {
    if (!text || !needle || !*needle) return false;
    const size_t nn = strlen(needle);
    for (const char *p = text; *p; ++p) {
        size_t i = 0;
        while (i < nn && p[i]) {
            unsigned char a = (unsigned char)p[i];
            unsigned char b = (unsigned char)needle[i];
            if (a >= 'A' && a <= 'Z') a = (unsigned char)(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = (unsigned char)(b - 'A' + 'a');
            if (a != b) break;
            ++i;
        }
        if (i == nn) return true;
    }
    return false;
}

/* Universal unavailable-ad policy. This deliberately models an environment in
 * which advertising services are absent. It never fabricates completion,
 * impressions, clicks, purchases, or rewarded-video rewards. */
static const int optional_sdk_billing_service_disconnected = -1;
static void *optional_sdk_billing_failure_result(void *env, const int *code);

static bool optional_sdk_blackhole_default_result(void *env_handle,
                                     const char *method_name,
                                     const char *signature,
                                     SsrnxJniValue *out_result) {
    if (!env_handle || !method_name || !signature || !out_result) return false;
    memset(out_result, 0, sizeof(*out_result));
    const char *ret = optional_sdk_signature_return_descriptor(signature);
    if (!ret) return false;

    switch (ret[0]) {
        case 'V':
            return true;
        case 'Z':
            out_result->z = 0;
            return true;
        case 'B':
            out_result->b = 0;
            return true;
        case 'C':
            out_result->c = 0;
            return true;
        case 'S':
            out_result->s = 0;
            return true;
        case 'I':
            out_result->i = 0;
            return true;
        case 'J':
            out_result->j = 0;
            return true;
        case 'F':
            out_result->f = 0.0f;
            return true;
        case 'D':
            out_result->d = 0.0;
            return true;
        case '[':
            /* No inventory / no SDK-owned result collection. */
            out_result->l = NULL;
            return true;
        case 'L': {
            char return_class[512];
            if (optional_sdk_descriptor_class_name(ret, return_class, sizeof(return_class)) &&
                strcmp(return_class, "com/google/android/gms/tasks/Task") == 0) {
                /* External Play services use Task for asynchronous completion.
                 * Return an already-failed Task so listeners observe completion
                 * and can take their normal unavailable/offline path. */
                out_result->l = optional_sdk_unavailable_task_object(env_handle, strcmp(method_name,"getSignedInAccountFromIntent")==0 ? 12500 : 17);
            } else if (optional_sdk_descriptor_class_name(ret, return_class, sizeof(return_class)) &&
                       strcmp(return_class,"com/android/billingclient/api/BillingResult")==0) {
                /* Every result produced by the unavailable facade is a failure;
                 * a zero-initialized packaged result would falsely report OK. */
                out_result->l=optional_sdk_billing_failure_result(env_handle,&optional_sdk_billing_service_disconnected);
            } else if (optional_sdk_descriptor_class_name(ret, return_class, sizeof(return_class)) &&
                       optional_sdk_is_known_unavailable_sdk(return_class)) {
                /* Keep fluent/factory APIs alive without entering provider code. */
                out_result->l = optional_sdk_stub_object(env_handle, return_class);
            } else {
                out_result->l = NULL;
            }
            return true;
        }
        default:
            return false;
    }
}

static bool optional_sdk_reward_surface(const char *class_name, const char *method_name) {
    return optional_sdk_text_contains_ci(class_name, "reward") || optional_sdk_text_contains_ci(method_name, "reward");
}

static bool optional_sdk_signature_returns_void(const char *signature) {
    const char *close = signature ? strrchr(signature, ')') : NULL;
    return close && close[1] == 'V' && close[2] == '\0';
}

static bool optional_sdk_signature_returns_boolean(const char *signature) {
    const char *close = signature ? strrchr(signature, ')') : NULL;
    return close && close[1] == 'Z' && close[2] == '\0';
}

static bool optional_sdk_method_is(const char *name, const char *expected) {
    return name && expected && strcmp(name, expected) == 0;
}

/* Google User Messaging Platform (consent) talks to a Google endpoint SSRNX
 * does not provide. Its asynchronous requests complete as failures carrying a
 * FormError marked here, so apps leave their consent-pending state instead of
 * waiting for a callback that never arrives. */
static const char optional_sdk_g_unavailable_form_error_marker;
#define UMP_FORM_ERROR "com/google/android/ump/FormError"
#define UMP_FORM_ERROR_INTERNAL 1 /* FormError.ErrorCode.INTERNAL_ERROR */

static void *optional_sdk_unavailable_form_error(void *env) {
    void *error = optional_sdk_stub_object(env, UMP_FORM_ERROR);
    if (error)
        (void)ssrnx_jni_set_object_host_data(error, (void *)&optional_sdk_g_unavailable_form_error_marker);
    return error;
}

static void optional_sdk_deliver_listener(void *env, void *listener, const char *method, const char *signature,
                             void *arg, const char *action) {
    char listener_class[512] = {0};
    if (!listener || !ssrnx_jni_object_class_name_host(listener, listener_class, sizeof(listener_class)))
        return;
    SsrnxJniValue callback_arg = {0}, ignored = {0};
    bool found = false;
    callback_arg.l = arg;
    (void)ssrnx_jni_invoke_method_host(env, listener, listener_class, method, signature, false,
                                       &callback_arg, &ignored, &found);
    SSRNX_LOGCI("ADS", "provider=ump action=%s listener=%s.%s delivered=%d", action, listener_class,
                method, found ? 1 : 0);
}

static bool optional_sdk_ump_try_call(void *env, void *receiver_ref, const char *class_name, const char *method_name,
                         const char *signature, const SsrnxJniValue *args, size_t arg_count,
                         SsrnxJniValue *out_result) {
    if (strcmp(class_name, UMP_FORM_ERROR) == 0 &&
        ssrnx_jni_get_object_host_data(receiver_ref) == (void *)&optional_sdk_g_unavailable_form_error_marker) {
        if (optional_sdk_method_is(method_name, "getErrorCode") && strcmp(signature, "()I") == 0) {
            out_result->i = UMP_FORM_ERROR_INTERNAL;
            return true;
        }
        if (optional_sdk_method_is(method_name, "getMessage") && strcmp(signature, "()Ljava/lang/String;") == 0) {
            out_result->l = ssrnx_jni_new_local_string_utf(env, "Consent service unavailable");
            return true;
        }
        return false;
    }
    if (strcmp(class_name, "com/google/android/ump/ConsentInformation") == 0 &&
        optional_sdk_method_is(method_name, "requestConsentInfoUpdate") && arg_count == 4 &&
        strcmp(signature,
               "(Landroid/app/Activity;Lcom/google/android/ump/ConsentRequestParameters;"
               "Lcom/google/android/ump/ConsentInformation$OnConsentInfoUpdateSuccessListener;"
               "Lcom/google/android/ump/ConsentInformation$OnConsentInfoUpdateFailureListener;)V") == 0) {
        optional_sdk_deliver_listener(env, args[3].l, "onConsentInfoUpdateFailure",
                         "(Lcom/google/android/ump/FormError;)V", optional_sdk_unavailable_form_error(env),
                         "requestConsentInfoUpdate");
        return true;
    }
    if (strcmp(class_name, "com/google/android/ump/UserMessagingPlatform") == 0 &&
        optional_sdk_method_is(method_name, "loadConsentForm") && arg_count == 3 &&
        strcmp(signature,
               "(Landroid/content/Context;"
               "Lcom/google/android/ump/UserMessagingPlatform$OnConsentFormLoadSuccessListener;"
               "Lcom/google/android/ump/UserMessagingPlatform$OnConsentFormLoadFailureListener;)V") == 0) {
        optional_sdk_deliver_listener(env, args[2].l, "onConsentFormLoadFailure",
                         "(Lcom/google/android/ump/FormError;)V", optional_sdk_unavailable_form_error(env),
                         "loadConsentForm");
        return true;
    }
    if (strcmp(class_name, "com/google/android/ump/UserMessagingPlatform") == 0 && arg_count == 2 &&
        strcmp(signature,
               "(Landroid/app/Activity;Lcom/google/android/ump/ConsentForm$OnConsentFormDismissedListener;)V") == 0) {
        /* loadAndShowConsentFormIfRequired: consent status is never REQUIRED,
         * which the SDK reports as an immediate dismissal with a null error.
         * showPrivacyOptionsForm: no privacy options form exists -> error. */
        if (optional_sdk_method_is(method_name, "loadAndShowConsentFormIfRequired")) {
            optional_sdk_deliver_listener(env, args[1].l, "onConsentFormDismissed",
                             "(Lcom/google/android/ump/FormError;)V", NULL, method_name);
            return true;
        }
        if (optional_sdk_method_is(method_name, "showPrivacyOptionsForm")) {
            optional_sdk_deliver_listener(env, args[1].l, "onConsentFormDismissed",
                             "(Lcom/google/android/ump/FormError;)V", optional_sdk_unavailable_form_error(env),
                             method_name);
            return true;
        }
    }
    return false;
}

static bool optional_sdk_common_unavailable_endpoint(const char *method_name, const char *signature,
                                        SsrnxJniValue *out_result) {
    if ((optional_sdk_method_is(method_name, "isReady") || optional_sdk_method_is(method_name, "isLoaded") ||
         optional_sdk_method_is(method_name, "isAdReady") || optional_sdk_method_is(method_name, "canShow")) &&
        optional_sdk_signature_returns_boolean(signature)) {
        *out_result = optional_sdk_bool_result(false);
        return true;
    }
    if ((optional_sdk_method_is(method_name, "loadAd") || optional_sdk_method_is(method_name, "showAd") ||
         optional_sdk_method_is(method_name, "show") || optional_sdk_method_is(method_name, "display")) &&
        optional_sdk_signature_returns_void(signature)) {
        memset(out_result, 0, sizeof(*out_result));
        return true;
    }
    return false;
}

/* billing unavailable */
/* Disconnected Play Billing completion contracts. No store/account/purchase
 * service exists here. Packaged clients also complete their disconnected path
 * immediately, before validating parameters or submitting service work. */
static const int optional_sdk_billing_setup_unavailable = 3;

static void *optional_sdk_billing_failure_result(void *env, const int *code) {
    void *result = optional_sdk_stub_object(env, "com/android/billingclient/api/BillingResult");
    if (result) (void)ssrnx_jni_set_object_host_data(result, (void *)code);
    else (void)ssrnx_jni_throw_new_by_name(env,"java/lang/OutOfMemoryError","Billing failure result");
    return result;
}

static bool optional_sdk_billing_unavailable_call(void *env, void *receiver, const char *cls,
                                     const char *method, const char *signature,
                                     const SsrnxJniValue *args, size_t n,
                                     SsrnxJniValue *out) {
    if (strcmp(cls,"com/android/billingclient/api/BillingResult")==0) {
        const void *marker=ssrnx_jni_get_object_host_data(receiver);
        if (marker!=&optional_sdk_billing_setup_unavailable && marker!=&optional_sdk_billing_service_disconnected) return false;
        if (optional_sdk_method_is(method,"getResponseCode") && strcmp(signature,"()I")==0) {
            out->i=*(const int *)marker;return true;
        }
        if (optional_sdk_method_is(method,"getDebugMessage") && strcmp(signature,"()Ljava/lang/String;")==0) {
            out->l=ssrnx_jni_new_local_string_utf(env,"Play Store billing service unavailable");return true;
        }
        return false;
    }
    if (strcmp(cls,"com/android/billingclient/api/BillingClient")!=0) return false;
    const char *callback=NULL;
    bool list=false, empty=false, setup=false;
    if (optional_sdk_method_is(method,"startConnection") && strcmp(signature,"(Lcom/android/billingclient/api/BillingClientStateListener;)V")==0) {
        callback="onBillingSetupFinished";setup=true;
    } else if (optional_sdk_method_is(method,"queryPurchasesAsync") &&
        (strcmp(signature,"(Ljava/lang/String;Lcom/android/billingclient/api/PurchasesResponseListener;)V")==0 ||
         strcmp(signature,"(Lcom/android/billingclient/api/QueryPurchasesParams;Lcom/android/billingclient/api/PurchasesResponseListener;)V")==0)) {
        callback="onQueryPurchasesResponse";list=true;empty=true;
    } else if (optional_sdk_method_is(method,"queryPurchaseHistoryAsync") &&
        (strcmp(signature,"(Ljava/lang/String;Lcom/android/billingclient/api/PurchaseHistoryResponseListener;)V")==0 ||
         strcmp(signature,"(Lcom/android/billingclient/api/QueryPurchaseHistoryParams;Lcom/android/billingclient/api/PurchaseHistoryResponseListener;)V")==0)) {
        callback="onPurchaseHistoryResponse";list=true;
    } else if (optional_sdk_method_is(method,"querySkuDetailsAsync") && strcmp(signature,"(Lcom/android/billingclient/api/SkuDetailsParams;Lcom/android/billingclient/api/SkuDetailsResponseListener;)V")==0) {
        callback="onSkuDetailsResponse";list=true;
    } else if (optional_sdk_method_is(method,"acknowledgePurchase") && strcmp(signature,"(Lcom/android/billingclient/api/AcknowledgePurchaseParams;Lcom/android/billingclient/api/AcknowledgePurchaseResponseListener;)V")==0) {
        callback="onAcknowledgePurchaseResponse";
    }
    if (!callback) return false;
    void *listener=args && n==(setup?1u:2u) ? args[n-1].l : NULL;
    if (!listener) {
        (void)ssrnx_jni_throw_new_by_name(env,"java/lang/NullPointerException","Billing response listener");return true;
    }
    SsrnxJniValue values[2]={{.l=optional_sdk_billing_failure_result(env,setup?&optional_sdk_billing_setup_unavailable:&optional_sdk_billing_service_disconnected)},{0}}, ignored={0};
    bool found=false;
    if (values[0].l && empty) {
        (void)ssrnx_jni_invoke_method_host(env,NULL,"java/util/Collections","emptyList","()Ljava/util/List;",true,NULL,&values[1],&found);
        if (!found && !ssrnx_jni_exception_pending_host(env))
            (void)ssrnx_jni_throw_new_by_name(env,"java/lang/NoSuchMethodError","Collections.emptyList");
    }
    char listener_class[512];
    if (values[0].l && !ssrnx_jni_exception_pending_host(env) && ssrnx_jni_object_class_name_host(listener,listener_class,sizeof(listener_class))) {
        found=false;
        (void)ssrnx_jni_invoke_method_host(env,listener,listener_class,callback,
            list?"(Lcom/android/billingclient/api/BillingResult;Ljava/util/List;)V":"(Lcom/android/billingclient/api/BillingResult;)V",false,values,&ignored,&found);
        SSRNX_LOGCI("PLATFORM","billing action=%s callback=%s code=%d delivered=%d",method,callback,setup?3:-1,found?1:0);
        if (!found && !ssrnx_jni_exception_pending_host(env))
            (void)ssrnx_jni_throw_new_by_name(env,"java/lang/NoSuchMethodError",callback);
    }
    if(values[1].l)ssrnx_jni_delete_local_ref_host(env,values[1].l);
    if(values[0].l)ssrnx_jni_delete_local_ref_host(env,values[0].l);
    return true;
}



bool ssrnx_optional_sdk_can_handle(const char *class_name,
                                     const char *method_name,
                                     const char *signature) {
    if (!class_name || !method_name || !signature) return false;
    if (optional_sdk_is_known_external_platform_sdk(class_name) && optional_sdk_method_is(method_name, "<clinit>"))
        return false;
    if (strcmp(class_name, "com/google/android/gms/tasks/Task") == 0) {
        return optional_sdk_method_is(method_name, "isComplete") ||
               optional_sdk_method_is(method_name, "isSuccessful") ||
               optional_sdk_method_is(method_name, "isCanceled") ||
               optional_sdk_method_is(method_name, "getException") ||
               optional_sdk_method_is(method_name, "getResult") ||
               optional_sdk_method_is(method_name, "addOnCompleteListener") ||
               optional_sdk_method_is(method_name, "addOnFailureListener") ||
               optional_sdk_method_is(method_name, "addOnSuccessListener") ||
               optional_sdk_method_is(method_name, "addOnCanceledListener");
    }
    if (!optional_sdk_is_known_unavailable_sdk(class_name)) return false;
    if (strcmp(class_name, "com/facebook/ads/AdSettings") == 0 &&
        optional_sdk_method_is(method_name, "setDataProcessingOptions") &&
        optional_sdk_signature_returns_void(signature)) return true;

    if (strcmp(class_name, "com/applovin/sdk/AppLovinSdk") == 0 &&
        optional_sdk_method_is(method_name, "getInstance") &&
        strcmp(signature, "(Landroid/content/Context;)Lcom/applovin/sdk/AppLovinSdk;") == 0) return true;
    if (strcmp(class_name, "com/applovin/sdk/AppLovinSdk") == 0 &&
        optional_sdk_method_is(method_name, "getSettings") &&
        strcmp(signature, "()Lcom/applovin/sdk/AppLovinSdkSettings;") == 0) return true;
    if (strcmp(class_name, "com/applovin/sdk/AppLovinSdkSettings") == 0 &&
        optional_sdk_method_is(method_name, "getTermsAndPrivacyPolicyFlowSettings") &&
        strcmp(signature, "()Lcom/applovin/sdk/AppLovinTermsAndPrivacyPolicyFlowSettings;") == 0) return true;
    if (strcmp(class_name, "com/applovin/sdk/AppLovinTermsAndPrivacyPolicyFlowSettings") == 0 &&
        (optional_sdk_method_is(method_name, "setEnabled") || optional_sdk_method_is(method_name, "setPrivacyPolicyUri") ||
         optional_sdk_method_is(method_name, "setTermsOfServiceUri")) && optional_sdk_signature_returns_void(signature)) return true;
    if (strcmp(class_name, "com/applovin/sdk/AppLovinSdkInitializationConfiguration") == 0 &&
        optional_sdk_method_is(method_name, "builder") &&
        strcmp(signature, "(Ljava/lang/String;)Lcom/applovin/sdk/AppLovinSdkInitializationConfiguration$Builder;") == 0) return true;
    if (strcmp(class_name, "com/applovin/sdk/AppLovinSdkInitializationConfiguration$Builder") == 0 &&
        optional_sdk_method_is(method_name, "setMediationProvider") &&
        strcmp(signature, "(Ljava/lang/String;)Lcom/applovin/sdk/AppLovinSdkInitializationConfiguration$Builder;") == 0) return true;
    if (strcmp(class_name, "com/applovin/sdk/AppLovinSdkInitializationConfiguration$Builder") == 0 &&
        optional_sdk_method_is(method_name, "build") &&
        strcmp(signature, "()Lcom/applovin/sdk/AppLovinSdkInitializationConfiguration;") == 0) return true;
    if (strcmp(class_name, "com/applovin/sdk/AppLovinSdk") == 0 &&
        optional_sdk_method_is(method_name, "initialize") && optional_sdk_signature_returns_void(signature)) return true;

    /* The ad black-hole is intentionally namespace-wide. Once a class belongs
     * to a recognized monetization SDK, SSRNX does not execute its DEX/native
     * implementation because there is no ad presentation backend. */
    return true;
}

SsrnxResult ssrnx_optional_sdk_try_call(void *env_handle,
                                        void *receiver_ref,
                                        const char *class_name,
                                        const char *method_name,
                                        const char *signature,
                                        bool is_static,
                                        const SsrnxJniValue *args,
                                        size_t arg_count,
                                        SsrnxJniValue *out_result,
                                        bool *out_handled) {
    (void)is_static;
    (void)args;
    (void)arg_count;
    if (!env_handle || !class_name || !method_name || !signature || !out_result || !out_handled)
        return SSRNX_ERR_ARGUMENT;
    *out_handled = false;
    memset(out_result, 0, sizeof(*out_result));

    /* Only Task instances created by SSRNX's unavailable-service model are
     * intercepted. Genuine Google Task objects keep their packaged behavior. */
    if (strcmp(class_name, "com/google/android/gms/tasks/Task") == 0 &&
        optional_sdk_is_unavailable_task(receiver_ref)) {
        if ((optional_sdk_method_is(method_name, "isComplete") || optional_sdk_method_is(method_name, "isSuccessful") ||
             optional_sdk_method_is(method_name, "isCanceled")) &&
            optional_sdk_signature_returns_boolean(signature)) {
            out_result->z = optional_sdk_method_is(method_name, "isComplete") ? 1 : 0;
            *out_handled = true;
            return SSRNX_OK;
        }
        if (optional_sdk_method_is(method_name, "getException") &&
            strcmp(signature, "()Ljava/lang/Exception;") == 0) {
            out_result->l = optional_sdk_unavailable_task_error(env_handle,receiver_ref);
            *out_handled = true;
            return SSRNX_OK;
        }
        if (optional_sdk_method_is(method_name, "getResult")) {
            if (strcmp(signature,"(Ljava/lang/Class;)Ljava/lang/Object;")==0) {
                if (!arg_count || !args[0].l) {
                    (void)ssrnx_jni_throw_new_by_name(env_handle,"java/lang/NullPointerException","exceptionType");
                    *out_handled=true;return SSRNX_OK;
                }
                char requested[512];void *error=optional_sdk_unavailable_task_error(env_handle,receiver_ref);
                bool match=error&&ssrnx_jni_class_ref_name_host(args[0].l,requested,sizeof(requested))&&ssrnx_jni_is_instance_of_class_host(error,requested);
                if(match)(void)ssrnx_jni_throw_host(env_handle,error);
                if(error)ssrnx_jni_delete_local_ref_host(env_handle,error);
                if(match){*out_handled=true;return SSRNX_OK;}
            }
            (void)ssrnx_jni_register_class_provisional("com/google/android/gms/tasks/RuntimeExecutionException", "java/lang/RuntimeException");
            void *cause = optional_sdk_unavailable_task_error(env_handle,receiver_ref);
            void *wrapper=ssrnx_jni_new_local_object(env_handle,"com/google/android/gms/tasks/RuntimeExecutionException");
            if (wrapper && cause) {
                SsrnxJniValue argument={.l=cause}, ignored={0};bool found=false;
                (void)ssrnx_jni_invoke_method_host(env_handle,wrapper,"com/google/android/gms/tasks/RuntimeExecutionException","<init>","(Ljava/lang/Throwable;)V",false,&argument,&ignored,&found);
                if(!found && !ssrnx_jni_exception_pending_host(env_handle))
                    (void)ssrnx_jni_throw_new_by_name(env_handle,"java/lang/NoSuchMethodError","RuntimeExecutionException(Throwable)");
                if(!ssrnx_jni_exception_pending_host(env_handle))(void)ssrnx_jni_throw_host(env_handle,wrapper);
            } else if(!ssrnx_jni_exception_pending_host(env_handle)) {
                (void)ssrnx_jni_throw_new_by_name(env_handle,"java/lang/OutOfMemoryError","Task failure exception");
            }
            if(wrapper)ssrnx_jni_delete_local_ref_host(env_handle,wrapper);
            if(cause)ssrnx_jni_delete_local_ref_host(env_handle,cause);
            *out_handled = true;
            return SSRNX_OK;
        }
        if (optional_sdk_method_is(method_name, "addOnCompleteListener") || optional_sdk_method_is(method_name, "addOnFailureListener")) {
            bool scheduled = optional_sdk_schedule_failed_task_listener(env_handle, receiver_ref, signature,
                args, arg_count, optional_sdk_method_is(method_name, "addOnFailureListener"));
            if (!scheduled && !ssrnx_jni_exception_pending_host(env_handle))
                (void)ssrnx_jni_throw_new_by_name(env_handle, "java/lang/IllegalStateException", "Unable to schedule Task listener");
            out_result->l = ssrnx_jni_new_local_ref_host(env_handle, receiver_ref);
            *out_handled = true;
            return SSRNX_OK;
        }
        if ((optional_sdk_method_is(method_name, "addOnSuccessListener") ||
             optional_sdk_method_is(method_name, "addOnCanceledListener"))) {
            /* Failed tasks never invoke success/cancel callbacks. */
            out_result->l = ssrnx_jni_new_local_ref_host(env_handle, receiver_ref);
            *out_handled = true;
            return SSRNX_OK;
        }
    }

    const bool ad_sdk = optional_sdk_is_known_ad_sdk(class_name);
    const bool external_sdk = optional_sdk_is_known_external_platform_sdk(class_name);
    if (!ad_sdk && !external_sdk) return SSRNX_OK;
    if (external_sdk && optional_sdk_method_is(method_name, "<clinit>")) return SSRNX_OK;
    if (strcmp(class_name, "com/google/android/gms/auth/api/signin/GoogleSignInClient") == 0 &&
        optional_sdk_method_is(method_name, "getSignInIntent") && strcmp(signature, "()Landroid/content/Intent;") == 0) {
        /* NO_IMPL completes a failed local flow, rather than starting auth UI
         * or returning null and leaving the activity-result caller waiting. */
        void *status=optional_sdk_unavailable_api_status(env_handle,12500);
        if(!status) {
            if(!ssrnx_jni_exception_pending_host(env_handle))(void)ssrnx_jni_throw_new_by_name(env_handle,"java/lang/UnsupportedOperationException","Sign-in failure status unavailable");
            *out_handled=true;return SSRNX_OK;
        }
        void *data=ssrnx_jni_new_local_object(env_handle,"android/content/Intent");
        SsrnxJniValue extra[2]={{.l=ssrnx_jni_new_local_string_utf(env_handle,"googleSignInStatus")},{.l=status}},ignored={0};bool found=false;
        if(data&&extra[0].l)(void)ssrnx_jni_invoke_method_host(env_handle,data,"android/content/Intent","putExtra","(Ljava/lang/String;Landroid/os/Parcelable;)Landroid/content/Intent;",false,extra,&ignored,&found);
        if(found&&!ssrnx_jni_exception_pending_host(env_handle))out_result->l=ssrnx_android_unavailable_activity_intent(env_handle,"com.google.android.gms.auth.NO_IMPL",data);
        else if(!ssrnx_jni_exception_pending_host(env_handle))(void)ssrnx_jni_throw_new_by_name(env_handle,"java/lang/OutOfMemoryError","Sign-in failure result");
        if(ignored.l)ssrnx_jni_delete_local_ref_host(env_handle,ignored.l);
        if(extra[0].l)ssrnx_jni_delete_local_ref_host(env_handle,extra[0].l);
        if(data)ssrnx_jni_delete_local_ref_host(env_handle,data);
        ssrnx_jni_delete_local_ref_host(env_handle,status);
        *out_handled = true;
        return SSRNX_OK;
    }

    /* Google Play Billing is a client for a Play Store Binder service that is
     * absent on Horizon. Keep its Java facade inert and explicitly unavailable:
     * never synthesize purchases or a successful billing connection. */
    if (external_sdk) {
        if (optional_sdk_billing_unavailable_call(env_handle,receiver_ref,class_name,method_name,signature,args,arg_count,out_result)) {
            *out_handled=true;return SSRNX_OK;
        }
        /* Real SDK value objects keep their packaged fields and getters. */
        if (strcmp(class_name,"com/android/billingclient/api/BillingResult")==0)
            return SSRNX_OK;
        if (optional_sdk_common_unavailable_endpoint(method_name, signature, out_result) ||
            optional_sdk_blackhole_default_result(env_handle, method_name, signature, out_result)) {
            SSRNX_LOGCI("PLATFORM",
                        "external-sdk-class=%s action=%s result=unavailable",
                        class_name, method_name);
            *out_handled = true;
            return SSRNX_OK;
        }
        return SSRNX_OK;
    }

    if (optional_sdk_ump_try_call(env_handle, receiver_ref, class_name, method_name, signature, args, arg_count,
                     out_result)) {
        *out_handled = true;
        return SSRNX_OK;
    }

    /* Facebook Audience Network privacy/configuration.  There is no useful ad
     * presentation work for SSRNX to perform, so accept the setting without
     * descending into provider internals. */
    if (strcmp(class_name, "com/facebook/ads/AdSettings") == 0 &&
        optional_sdk_method_is(method_name, "setDataProcessingOptions") &&
        optional_sdk_signature_returns_void(signature)) {
        SSRNX_LOGCI("ADS", "provider=facebook action=data-processing-options result=accepted-unavailable");
        *out_handled = true;
        return SSRNX_OK;
    }

    /* AppLovin setup objects used by the public SDK configuration surface. */
    if (strcmp(class_name, "com/applovin/sdk/AppLovinSdk") == 0 &&
        optional_sdk_method_is(method_name, "getInstance") &&
        strcmp(signature, "(Landroid/content/Context;)Lcom/applovin/sdk/AppLovinSdk;") == 0) {
        *out_result = optional_sdk_object_result(optional_sdk_stub_object(env_handle, "com/applovin/sdk/AppLovinSdk"));
        *out_handled = true;
        SSRNX_LOGCI("ADS", "provider=applovin action=getInstance result=stub-unavailable");
        return SSRNX_OK;
    }
    if (strcmp(class_name, "com/applovin/sdk/AppLovinSdk") == 0 &&
        optional_sdk_method_is(method_name, "getSettings") &&
        strcmp(signature, "()Lcom/applovin/sdk/AppLovinSdkSettings;") == 0) {
        *out_result = optional_sdk_object_result(optional_sdk_stub_object(env_handle, "com/applovin/sdk/AppLovinSdkSettings"));
        *out_handled = true;
        return SSRNX_OK;
    }
    if (strcmp(class_name, "com/applovin/sdk/AppLovinSdkSettings") == 0 &&
        optional_sdk_method_is(method_name, "getTermsAndPrivacyPolicyFlowSettings") &&
        strcmp(signature, "()Lcom/applovin/sdk/AppLovinTermsAndPrivacyPolicyFlowSettings;") == 0) {
        *out_result = optional_sdk_object_result(optional_sdk_stub_object(env_handle, "com/applovin/sdk/AppLovinTermsAndPrivacyPolicyFlowSettings"));
        *out_handled = true;
        return SSRNX_OK;
    }
    if (strcmp(class_name, "com/applovin/sdk/AppLovinTermsAndPrivacyPolicyFlowSettings") == 0 &&
        (optional_sdk_method_is(method_name, "setEnabled") || optional_sdk_method_is(method_name, "setPrivacyPolicyUri") ||
         optional_sdk_method_is(method_name, "setTermsOfServiceUri")) && optional_sdk_signature_returns_void(signature)) {
        *out_handled = true;
        return SSRNX_OK;
    }
    if (strcmp(class_name, "com/applovin/sdk/AppLovinSdkInitializationConfiguration") == 0 &&
        optional_sdk_method_is(method_name, "builder") &&
        strcmp(signature, "(Ljava/lang/String;)Lcom/applovin/sdk/AppLovinSdkInitializationConfiguration$Builder;") == 0) {
        *out_result = optional_sdk_object_result(optional_sdk_stub_object(env_handle,
                                                "com/applovin/sdk/AppLovinSdkInitializationConfiguration$Builder"));
        *out_handled = true;
        return SSRNX_OK;
    }
    if (strcmp(class_name, "com/applovin/sdk/AppLovinSdkInitializationConfiguration$Builder") == 0 &&
        optional_sdk_method_is(method_name, "setMediationProvider") &&
        strcmp(signature, "(Ljava/lang/String;)Lcom/applovin/sdk/AppLovinSdkInitializationConfiguration$Builder;") == 0) {
        *out_result = optional_sdk_object_result(receiver_ref ? ssrnx_jni_new_local_ref_host(env_handle, receiver_ref) : NULL);
        *out_handled = true;
        return SSRNX_OK;
    }
    if (strcmp(class_name, "com/applovin/sdk/AppLovinSdkInitializationConfiguration$Builder") == 0 &&
        optional_sdk_method_is(method_name, "build") &&
        strcmp(signature, "()Lcom/applovin/sdk/AppLovinSdkInitializationConfiguration;") == 0) {
        *out_result = optional_sdk_object_result(optional_sdk_stub_object(env_handle, "com/applovin/sdk/AppLovinSdkInitializationConfiguration"));
        *out_handled = true;
        return SSRNX_OK;
    }
    if (strcmp(class_name, "com/applovin/sdk/AppLovinSdk") == 0 &&
        optional_sdk_method_is(method_name, "initialize") && optional_sdk_signature_returns_void(signature)) {
        /* Deliberately do not invoke the success listener.  That callback is
         * where many SDKs create/load banners/interstitials.  An unavailable
         * ad service is neither initialized nor ready. */
        SSRNX_LOGCI("ADS", "provider=applovin action=initialize result=unavailable callback=deferred-none");
        *out_handled = true;
        return SSRNX_OK;
    }

    if (optional_sdk_common_unavailable_endpoint(method_name, signature, out_result)) {
        SSRNX_LOGCI("ADS", "provider-class=%s action=%s result=unavailable reward=never-synthesized",
                    class_name, method_name);
        *out_handled = true;
        return SSRNX_OK;
    }

    /* Final namespace-wide sink. Constructors, configuration, callbacks and
     * provider internals are not allowed to descend into the real ad SDK.
     * Neutral return values represent an unavailable service. Object-valued
     * fluent/factory APIs receive inert ad-SDK objects only when their return
     * type is itself a recognized ad namespace. */
    if (optional_sdk_blackhole_default_result(env_handle, method_name, signature, out_result)) {
        SSRNX_LOGCI("ADS",
                    "provider-class=%s action=%s result=blackholed-unavailable reward=%s",
                    class_name, method_name,
                    optional_sdk_reward_surface(class_name, method_name) ? "never-synthesized" : "not-applicable");
        *out_handled = true;
        return SSRNX_OK;
    }
    return SSRNX_OK;
}

#pragma pop_macro("UMP_FORM_ERROR_INTERNAL")
#pragma pop_macro("UMP_FORM_ERROR")
#pragma pop_macro("FAILED_TASK_CALLBACK")
