/* runtime audio. */
#define _GNU_SOURCE 1
#include "runtime.h"


/* ---- audio ---- */
#pragma push_macro("SSRNX_AUDIO_ALLOC_BYTES")
#pragma push_macro("SSRNX_AUDIO_BUFFER_COUNT")
#pragma push_macro("SSRNX_AUDIO_BYTES_PER_FRAME")
#pragma push_macro("SSRNX_AUDIO_CHANNELS")
#pragma push_macro("SSRNX_AUDIO_DATA_BYTES")
#pragma push_macro("SSRNX_AUDIO_DEFAULT_HZ")
#pragma push_macro("SSRNX_AUDIO_DEFAULT_VOLUME")
#pragma push_macro("SSRNX_AUDIO_FRAMES_PER_BUFFER")
#pragma push_macro("SSRNX_AUDIO_PI")
#pragma push_macro("SSRNX_AUDIO_WORKER_IDLE_NS")
#pragma push_macro("SSRNX_AUDIO_WORKER_REPORT_US")
#pragma push_macro("SSRNX_AUDIO_WORKER_WAIT_NS")
#include <malloc.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <switch.h>


#ifdef __SWITCH__

#endif

#define SSRNX_AUDIO_BUFFER_COUNT 6
#define SSRNX_AUDIO_FRAMES_PER_BUFFER 960u
#define SSRNX_AUDIO_CHANNELS 2u
#define SSRNX_AUDIO_BYTES_PER_FRAME (SSRNX_AUDIO_CHANNELS * sizeof(int16_t))
#define SSRNX_AUDIO_DATA_BYTES (SSRNX_AUDIO_FRAMES_PER_BUFFER * SSRNX_AUDIO_BYTES_PER_FRAME)
#define SSRNX_AUDIO_ALLOC_BYTES 0x1000u
#define SSRNX_AUDIO_DEFAULT_VOLUME 0.35f
#define SSRNX_AUDIO_DEFAULT_HZ 440.0f
#define SSRNX_AUDIO_PI 3.14159265358979323846f
#define SSRNX_AUDIO_WORKER_WAIT_NS UINT64_C(50000000)
#define SSRNX_AUDIO_WORKER_IDLE_NS UINT64_C(2000000)
#define SSRNX_AUDIO_WORKER_REPORT_US UINT64_C(2000000)

typedef struct audio_SsrnxAudioSlot {
    AudioOutBuffer out;
    void *memory;
} audio_SsrnxAudioSlot;

static audio_SsrnxAudioSlot audio_g_slots[SSRNX_AUDIO_BUFFER_COUNT];
static bool audio_g_initialized;
static bool audio_g_running;
static bool audio_g_paused;
static SsrnxAudioMode audio_g_mode;
static float audio_g_frequency;
static float audio_g_volume;
static float audio_g_phase;
static float audio_g_peak_left;
static float audio_g_peak_right;
static float audio_g_wave_left[SSRNX_AUDIO_WAVE_SAMPLES];
static float audio_g_wave_right[SSRNX_AUDIO_WAVE_SAMPLES];
static uint32_t audio_g_sample_rate;
static uint32_t audio_g_channels;
static uint32_t audio_g_pcm_format;
static uint32_t audio_g_queued_buffers;
static uint64_t audio_g_played_samples;
static SsrnxAudioSessionStats audio_g_stats;
static SsrnxAudioRenderFn audio_g_renderer;
static void *audio_g_renderer_user;
static pthread_mutex_t audio_g_service_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t audio_g_renderer_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t audio_g_worker_thread;
/* Static-storage _Atomic objects are zero-initialized by C.  Do not use
 * ATOMIC_VAR_INIT here: devkitA64's current libc headers do not expose that
 * legacy macro even though atomic_load/store are available. */
static atomic_bool audio_g_worker_requested;
static atomic_bool audio_g_worker_running;
static bool audio_g_worker_joinable;
static bool audio_g_queue_was_empty;

static uint64_t audio_monotonic_us(void) {
    struct timespec ts = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * UINT64_C(1000000) + (uint64_t)ts.tv_nsec / 1000u;
}

static void audio_sleep_ns(uint64_t ns) {
    struct timespec ts = {
        .tv_sec = (time_t)(ns / UINT64_C(1000000000)),
        .tv_nsec = (long)(ns % UINT64_C(1000000000)),
    };
    (void)nanosleep(&ts, NULL);
}

static float audio_clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static void audio_clear_slots(void) {
    for (unsigned i = 0; i < SSRNX_AUDIO_BUFFER_COUNT; ++i) {
        free(audio_g_slots[i].memory);
        memset(&audio_g_slots[i], 0, sizeof(audio_g_slots[i]));
    }
}

static void audio_update_mode_stats(void) {
    if (audio_g_mode == SSRNX_AUDIO_LEFT) audio_g_stats.saw_left = true;
    if (audio_g_mode == SSRNX_AUDIO_RIGHT) audio_g_stats.saw_right = true;
    if (audio_g_mode == SSRNX_AUDIO_STEREO) audio_g_stats.saw_stereo = true;
}

static void audio_fill_buffer(AudioOutBuffer *out) {
    if (!out || !out->buffer || audio_g_sample_rate == 0) return;

    int16_t *pcm = (int16_t *)out->buffer;
    float peak_l = 0.0f;
    float peak_r = 0.0f;

    SsrnxAudioRenderFn renderer = NULL;
    void *renderer_user = NULL;
    pthread_mutex_lock(&audio_g_renderer_lock);
    renderer = audio_g_renderer;
    renderer_user = audio_g_renderer_user;
    pthread_mutex_unlock(&audio_g_renderer_lock);

    if (renderer) {
        memset(pcm, 0, SSRNX_AUDIO_DATA_BYTES);
        size_t rendered = renderer(pcm, SSRNX_AUDIO_FRAMES_PER_BUFFER, renderer_user);
        if (rendered > SSRNX_AUDIO_FRAMES_PER_BUFFER) rendered = SSRNX_AUDIO_FRAMES_PER_BUFFER;
        if (rendered < SSRNX_AUDIO_FRAMES_PER_BUFFER) {
            audio_g_stats.renderer_shortfall_frames += SSRNX_AUDIO_FRAMES_PER_BUFFER - rendered;
            memset(pcm + rendered * 2u, 0,
                   (SSRNX_AUDIO_FRAMES_PER_BUFFER - rendered) * SSRNX_AUDIO_BYTES_PER_FRAME);
        }
        for (uint32_t i = 0; i < SSRNX_AUDIO_FRAMES_PER_BUFFER; ++i) {
            const float left = (float)pcm[i * 2u] / 32768.0f;
            const float right = (float)pcm[i * 2u + 1u] / 32768.0f;
            if (fabsf(left) > peak_l) peak_l = fabsf(left);
            if (fabsf(right) > peak_r) peak_r = fabsf(right);
            if (i < SSRNX_AUDIO_WAVE_SAMPLES) {
                audio_g_wave_left[i] = left;
                audio_g_wave_right[i] = right;
            }
        }
    } else {
        const float step = 2.0f * SSRNX_AUDIO_PI * audio_g_frequency / (float)audio_g_sample_rate;
        const float amplitude = 0.24f;
        for (uint32_t i = 0; i < SSRNX_AUDIO_FRAMES_PER_BUFFER; ++i) {
            const float tone = sinf(audio_g_phase) * amplitude;
            audio_g_phase += step;
            if (audio_g_phase >= 2.0f * SSRNX_AUDIO_PI) audio_g_phase -= 2.0f * SSRNX_AUDIO_PI;

            float left = 0.0f;
            float right = 0.0f;
            if (audio_g_mode == SSRNX_AUDIO_LEFT || audio_g_mode == SSRNX_AUDIO_STEREO) left = tone;
            if (audio_g_mode == SSRNX_AUDIO_RIGHT || audio_g_mode == SSRNX_AUDIO_STEREO) right = tone;

            if (fabsf(left) > peak_l) peak_l = fabsf(left);
            if (fabsf(right) > peak_r) peak_r = fabsf(right);
            pcm[i * 2u] = (int16_t)(left * 32767.0f);
            pcm[i * 2u + 1u] = (int16_t)(right * 32767.0f);
            if (i < SSRNX_AUDIO_WAVE_SAMPLES) {
                audio_g_wave_left[i] = left;
                audio_g_wave_right[i] = right;
            }
        }
    }

    audio_g_peak_left = peak_l;
    audio_g_peak_right = peak_r;
    out->data_size = SSRNX_AUDIO_DATA_BYTES;
    out->data_offset = 0;
    armDCacheFlush(out->buffer, out->buffer_size);
    audio_g_stats.frames_generated += SSRNX_AUDIO_FRAMES_PER_BUFFER;
}

void ssrnx_audio_set_renderer(SsrnxAudioRenderFn renderer, void *user) {
    pthread_mutex_lock(&audio_g_renderer_lock);
    audio_g_renderer = renderer;
    audio_g_renderer_user = user;
    pthread_mutex_unlock(&audio_g_renderer_lock);
}

static bool audio_append_buffer(AudioOutBuffer *out) {
    Result rc = audoutAppendAudioOutBuffer(out);
    if (R_FAILED(rc)) {
        SSRNX_LOGCE("AUDIO", "audoutAppendAudioOutBuffer failed rc=0x%08x", rc);
        return false;
    }
    audio_g_stats.buffers_submitted++;
    return true;
}

static bool audio_prime_all_buffers(void) {
    for (unsigned i = 0; i < SSRNX_AUDIO_BUFFER_COUNT; ++i) {
        audio_fill_buffer(&audio_g_slots[i].out);
        if (!audio_append_buffer(&audio_g_slots[i].out)) return false;
    }
    audio_g_queued_buffers = SSRNX_AUDIO_BUFFER_COUNT;
    if (audio_g_stats.min_queued > audio_g_queued_buffers) audio_g_stats.min_queued = audio_g_queued_buffers;
    return true;
}

static void audio_drain_released_without_requeue(void) {
    for (;;) {
        AudioOutBuffer *released = NULL;
        u32 count = 0;
        Result rc = audoutGetReleasedAudioOutBuffer(&released, &count);
        if (R_FAILED(rc) || count == 0 || released == NULL) break;
        audio_g_stats.buffers_released++;
    }
}

static void audio_cleanup_open_service(void) {
    (void)audoutStopAudioOut();
    if (hosversionAtLeast(4, 0, 0)) {
        bool flushed = false;
        (void)audoutFlushAudioOutBuffers(&flushed);
    }
    audio_drain_released_without_requeue();
    audio_clear_slots();
    audoutExit();
}

static bool audio_refill_one_locked(AudioOutBuffer *released, bool worker_refill) {
    if (!released) return true;
    const uint64_t begin_us = audio_monotonic_us();
    audio_g_stats.buffers_released++;
    audio_fill_buffer(released);
    if (!audio_append_buffer(released)) return false;
    if (worker_refill) audio_g_stats.worker_refills++;
    const uint64_t end_us = audio_monotonic_us();
    if (begin_us && end_us >= begin_us) {
        const uint64_t elapsed = end_us - begin_us;
        if (elapsed > audio_g_stats.max_refill_us) audio_g_stats.max_refill_us = elapsed;
    }
    return true;
}

static bool audio_drain_released_locked(AudioOutBuffer *first_released, u32 first_count,
                                  bool worker_refill) {
    if (first_count && first_released && !audio_refill_one_locked(first_released, worker_refill))
        return false;
    for (;;) {
        AudioOutBuffer *released = NULL;
        u32 released_count = 0;
        Result rc = audoutGetReleasedAudioOutBuffer(&released, &released_count);
        if (R_FAILED(rc)) {
            SSRNX_LOGCE("AUDIO", "audoutGetReleasedAudioOutBuffer failed rc=0x%08x", rc);
            return false;
        }
        if (released_count == 0 || released == NULL) break;
        if (!audio_refill_one_locked(released, worker_refill)) return false;
    }
    return true;
}

static void audio_refresh_runtime_stats_locked(void) {
    if (!hosversionAtLeast(4, 0, 0)) return;
    u32 queued = 0;
    Result qrc = audoutGetAudioOutBufferCount(&queued);
    if (R_SUCCEEDED(qrc)) {
        audio_g_queued_buffers = queued;
        if (audio_g_running && !audio_g_paused) {
            if (queued < audio_g_stats.min_queued) audio_g_stats.min_queued = queued;
            if (queued == 0 && !audio_g_queue_was_empty) {
                audio_g_stats.underruns++;
                SSRNX_LOGCW("AUDIO",
                            "hardware queue underrun count=%u played=%llu released=%llu submitted=%llu worker=%d maxGapUs=%llu maxRefillUs=%llu",
                            audio_g_stats.underruns,
                            (unsigned long long)audio_g_played_samples,
                            (unsigned long long)audio_g_stats.buffers_released,
                            (unsigned long long)audio_g_stats.buffers_submitted,
                            atomic_load_explicit(&audio_g_worker_running, memory_order_acquire) ? 1 : 0,
                            (unsigned long long)audio_g_stats.max_service_gap_us,
                            (unsigned long long)audio_g_stats.max_refill_us);
            }
            audio_g_queue_was_empty = queued == 0;
        } else {
            audio_g_queue_was_empty = false;
        }
    }

    u64 played = 0;
    if (R_SUCCEEDED(audoutGetAudioOutPlayedSampleCount(&played))) {
        audio_g_played_samples = played;
        audio_g_stats.played_samples = played;
    }
}

static void *audio_audio_worker_main(void *unused) {
    (void)unused;
#ifdef __SWITCH__
    /* OpenSL queue callbacks are guest function pointers. Keep this worker off
     * the reserved render core, then publish Bionic TLS before any callback can
     * cross back into guest AArch64. */
    const int worker_core = ssrnx_thread_policy_pin_background();
    ssrnx_bionic_tls_activate_current();
    (void)ssrnx_bionic_pthread_self();
    SSRNX_LOGCI("AUDIO", "worker CPU affinity core=%d", worker_core);
#endif
    atomic_store_explicit(&audio_g_worker_running, true, memory_order_release);
    pthread_mutex_lock(&audio_g_service_lock);
    audio_g_stats.worker_started = true;
    pthread_mutex_unlock(&audio_g_service_lock);
    SSRNX_LOGCI("AUDIO", "worker started waitMs=%u bufferMs=%u hostQueueMs=%u",
                (unsigned)(SSRNX_AUDIO_WORKER_WAIT_NS / UINT64_C(1000000)),
                (unsigned)(SSRNX_AUDIO_FRAMES_PER_BUFFER * 1000u / (audio_g_sample_rate ? audio_g_sample_rate : 48000u)),
                (unsigned)(SSRNX_AUDIO_BUFFER_COUNT * SSRNX_AUDIO_FRAMES_PER_BUFFER * 1000u /
                           (audio_g_sample_rate ? audio_g_sample_rate : 48000u)));

    uint64_t last_service_us = audio_monotonic_us();
    uint64_t last_report_us = last_service_us;
    while (atomic_load_explicit(&audio_g_worker_requested, memory_order_acquire)) {
        pthread_mutex_lock(&audio_g_service_lock);
        if (!audio_g_initialized) {
            pthread_mutex_unlock(&audio_g_service_lock);
            break;
        }
        if (!audio_g_running || audio_g_paused) {
            pthread_mutex_unlock(&audio_g_service_lock);
            audio_sleep_ns(SSRNX_AUDIO_WORKER_IDLE_NS);
            continue;
        }

        const uint64_t service_begin_us = audio_monotonic_us();
        if (last_service_us && service_begin_us >= last_service_us) {
            const uint64_t gap = service_begin_us - last_service_us;
            if (gap > audio_g_stats.max_service_gap_us) audio_g_stats.max_service_gap_us = gap;
        }
        last_service_us = service_begin_us;

        bool ok = true;
#ifdef __SWITCH__
        AudioOutBuffer *released = NULL;
        u32 released_count = 0;
        Result rc = audoutWaitPlayFinish(&released, &released_count, SSRNX_AUDIO_WORKER_WAIT_NS);
        if (R_SUCCEEDED(rc)) {
            audio_g_stats.worker_wakeups++;
            ok = audio_drain_released_locked(released, released_count, true);
        } else if (R_VALUE(rc) == KERNELRESULT(TimedOut)) {
            audio_g_stats.worker_timeouts++;
        } else {
            SSRNX_LOGCE("AUDIO", "worker audoutWaitPlayFinish failed rc=0x%08x", rc);
            ok = false;
        }
#else
        audio_g_stats.worker_wakeups++;
        ok = audio_drain_released_locked(NULL, 0, true);
#endif
        audio_refresh_runtime_stats_locked();

        const uint64_t now_us = audio_monotonic_us();
        if (last_report_us && now_us >= last_report_us + SSRNX_AUDIO_WORKER_REPORT_US) {
            SSRNX_LOGCI("AUDIO",
                        "worker heartbeat queued=%u played=%llu underruns=%u wakeups=%llu refills=%llu timeouts=%llu shortfallFrames=%llu maxGapUs=%llu maxRefillUs=%llu",
                        audio_g_queued_buffers,
                        (unsigned long long)audio_g_played_samples,
                        audio_g_stats.underruns,
                        (unsigned long long)audio_g_stats.worker_wakeups,
                        (unsigned long long)audio_g_stats.worker_refills,
                        (unsigned long long)audio_g_stats.worker_timeouts,
                        (unsigned long long)audio_g_stats.renderer_shortfall_frames,
                        (unsigned long long)audio_g_stats.max_service_gap_us,
                        (unsigned long long)audio_g_stats.max_refill_us);
            last_report_us = now_us;
        }
        pthread_mutex_unlock(&audio_g_service_lock);
        if (!ok) audio_sleep_ns(SSRNX_AUDIO_WORKER_IDLE_NS);
#ifndef __SWITCH__
        audio_sleep_ns(UINT64_C(5000000));
#endif
    }
    atomic_store_explicit(&audio_g_worker_running, false, memory_order_release);
    SSRNX_LOGCI("AUDIO", "worker stopped");
    return NULL;
}

bool ssrnx_audio_init(void) {
    if (audio_g_initialized) return true;

    memset(&audio_g_stats, 0, sizeof(audio_g_stats));
    memset(audio_g_wave_left, 0, sizeof(audio_g_wave_left));
    memset(audio_g_wave_right, 0, sizeof(audio_g_wave_right));
    audio_g_stats.min_queued = UINT32_MAX;
    audio_g_mode = SSRNX_AUDIO_STEREO;
    audio_g_frequency = SSRNX_AUDIO_DEFAULT_HZ;
    audio_g_volume = SSRNX_AUDIO_DEFAULT_VOLUME;
    audio_g_phase = 0.0f;
    audio_g_peak_left = 0.0f;
    audio_g_peak_right = 0.0f;
    audio_g_queued_buffers = 0;
    audio_g_played_samples = 0;
    audio_g_paused = false;
    audio_g_running = false;
    audio_g_queue_was_empty = false;
    /* Reset worker state explicitly for clean reinitialization. */
    atomic_store_explicit(&audio_g_worker_requested, false, memory_order_relaxed);
    atomic_store_explicit(&audio_g_worker_running, false, memory_order_relaxed);

    Result rc = audoutInitialize();
    if (R_FAILED(rc)) {
        SSRNX_LOGCE("AUDIO", "audoutInitialize failed rc=0x%08x", rc);
        return false;
    }

    audio_g_sample_rate = audoutGetSampleRate();
    audio_g_channels = audoutGetChannelCount();
    audio_g_pcm_format = (uint32_t)audoutGetPcmFormat();
    audio_g_stats.init_ok = true;
    audio_g_stats.format_ok = audio_g_sample_rate == 48000u && audio_g_channels == 2u && audio_g_pcm_format == (uint32_t)PcmFormat_Int16;

    SSRNX_LOGCI("AUDIO", "audout opened rate=%u channels=%u format=%u state=%u",
                audio_g_sample_rate, audio_g_channels, audio_g_pcm_format, (unsigned)audoutGetDeviceState());
    if (!audio_g_stats.format_ok) {
        SSRNX_LOGCE("AUDIO", "unsupported audio format rate=%u channels=%u format=%u",
                    audio_g_sample_rate, audio_g_channels, audio_g_pcm_format);
        audoutExit();
        return false;
    }

    for (unsigned i = 0; i < SSRNX_AUDIO_BUFFER_COUNT; ++i) {
        void *memory = memalign(0x1000, SSRNX_AUDIO_ALLOC_BYTES);
        if (!memory) {
            SSRNX_LOGCE("AUDIO", "audio buffer allocation failed index=%u", i);
            audio_cleanup_open_service();
            return false;
        }
        memset(memory, 0, SSRNX_AUDIO_ALLOC_BYTES);
        audio_g_slots[i].memory = memory;
        memset(&audio_g_slots[i].out, 0, sizeof(audio_g_slots[i].out));
        audio_g_slots[i].out.buffer = memory;
        audio_g_slots[i].out.buffer_size = SSRNX_AUDIO_ALLOC_BYTES;
        audio_g_slots[i].out.data_size = SSRNX_AUDIO_DATA_BYTES;
    }

    if (hosversionAtLeast(6, 0, 0)) {
        rc = audoutSetAudioOutVolume(audio_g_volume);
        if (R_FAILED(rc)) SSRNX_LOGCW("AUDIO", "initial volume set failed rc=0x%08x", rc);
    }

    if (!audio_prime_all_buffers()) {
        audio_cleanup_open_service();
        return false;
    }

    rc = audoutStartAudioOut();
    if (R_FAILED(rc)) {
        SSRNX_LOGCE("AUDIO", "audoutStartAudioOut failed rc=0x%08x", rc);
        audio_cleanup_open_service();
        return false;
    }

    audio_g_running = true;
    audio_g_initialized = true;
    audio_update_mode_stats();
    SSRNX_LOGCI("AUDIO", "Stage11 audio backend initialized buffers=%u framesPerBuffer=%u queueMs=%u",
                SSRNX_AUDIO_BUFFER_COUNT, SSRNX_AUDIO_FRAMES_PER_BUFFER,
                (unsigned)(SSRNX_AUDIO_BUFFER_COUNT * SSRNX_AUDIO_FRAMES_PER_BUFFER * 1000u / audio_g_sample_rate));
    return true;
}

bool ssrnx_audio_update(void) {
    pthread_mutex_lock(&audio_g_service_lock);
    if (!audio_g_initialized) {
        pthread_mutex_unlock(&audio_g_service_lock);
        return false;
    }
    bool ok = true;
    if (!audio_g_paused) ok = audio_drain_released_locked(NULL, 0, false);
    audio_refresh_runtime_stats_locked();
    audio_update_mode_stats();
    pthread_mutex_unlock(&audio_g_service_lock);
    return ok;
}

bool ssrnx_audio_start_worker(void) {
    if (atomic_load_explicit(&audio_g_worker_running, memory_order_acquire) || audio_g_worker_joinable)
        return true;
    pthread_mutex_lock(&audio_g_service_lock);
    const bool ready = audio_g_initialized;
    pthread_mutex_unlock(&audio_g_service_lock);
    if (!ready) return false;
    atomic_store_explicit(&audio_g_worker_requested, true, memory_order_release);
    const int rc = pthread_create(&audio_g_worker_thread, NULL, audio_audio_worker_main, NULL);
    if (rc != 0) {
        atomic_store_explicit(&audio_g_worker_requested, false, memory_order_release);
        SSRNX_LOGCE("AUDIO", "worker create failed rc=%d", rc);
        return false;
    }
    audio_g_worker_joinable = true;
    return true;
}

void ssrnx_audio_stop_worker(void) {
    if (!audio_g_worker_joinable) return;
    atomic_store_explicit(&audio_g_worker_requested, false, memory_order_release);
    (void)pthread_join(audio_g_worker_thread, NULL);
    audio_g_worker_joinable = false;
}

bool ssrnx_audio_worker_running(void) {
    return atomic_load_explicit(&audio_g_worker_running, memory_order_acquire);
}

void ssrnx_audio_set_mode(SsrnxAudioMode mode) {
    if (mode < SSRNX_AUDIO_SILENCE || mode > SSRNX_AUDIO_STEREO) return;
    pthread_mutex_lock(&audio_g_service_lock);
    if (audio_g_mode != mode) {
        audio_g_mode = mode;
        SSRNX_LOGCI("AUDIO", "mode=%d", (int)mode);
    }
    audio_update_mode_stats();
    pthread_mutex_unlock(&audio_g_service_lock);
}

void ssrnx_audio_set_frequency(float hz) {
    const float clamped = audio_clampf(hz, 110.0f, 1760.0f);
    pthread_mutex_lock(&audio_g_service_lock);
    if (fabsf(clamped - audio_g_frequency) >= 0.5f) {
        audio_g_frequency = clamped;
        SSRNX_LOGCI("AUDIO", "frequency=%.1fHz", (double)audio_g_frequency);
    }
    pthread_mutex_unlock(&audio_g_service_lock);
}

bool ssrnx_audio_set_volume(float volume) {
    const float clamped = audio_clampf(volume, 0.0f, 1.0f);
    pthread_mutex_lock(&audio_g_service_lock);
    if (fabsf(clamped - audio_g_volume) < 0.001f) {
        pthread_mutex_unlock(&audio_g_service_lock);
        return true;
    }

    if (audio_g_initialized && hosversionAtLeast(6, 0, 0)) {
        Result rc = audoutSetAudioOutVolume(clamped);
        if (R_FAILED(rc)) {
            SSRNX_LOGCE("AUDIO", "audoutSetAudioOutVolume failed rc=0x%08x", rc);
            pthread_mutex_unlock(&audio_g_service_lock);
            return false;
        }
    }

    audio_g_volume = clamped;
    audio_g_stats.saw_volume_change = true;
    SSRNX_LOGCI("AUDIO", "volume=%.2f", (double)audio_g_volume);
    pthread_mutex_unlock(&audio_g_service_lock);
    return true;
}

bool ssrnx_audio_toggle_pause(void) {
    pthread_mutex_lock(&audio_g_service_lock);
    if (!audio_g_initialized) {
        pthread_mutex_unlock(&audio_g_service_lock);
        return false;
    }
    Result rc;
    if (!audio_g_paused) {
        rc = audoutStopAudioOut();
        if (R_FAILED(rc)) {
            pthread_mutex_unlock(&audio_g_service_lock);
            return false;
        }
        audio_g_paused = true;
        audio_g_running = false;
        audio_g_queue_was_empty = false;
        SSRNX_LOGCI("AUDIO", "paused");
    } else {
        rc = audoutStartAudioOut();
        if (R_FAILED(rc)) {
            pthread_mutex_unlock(&audio_g_service_lock);
            return false;
        }
        audio_g_paused = false;
        audio_g_running = true;
        SSRNX_LOGCI("AUDIO", "resumed");
    }
    audio_g_stats.saw_pause_resume = true;
    pthread_mutex_unlock(&audio_g_service_lock);
    return true;
}

bool ssrnx_audio_restart(void) {
    pthread_mutex_lock(&audio_g_service_lock);
    if (!audio_g_initialized) {
        pthread_mutex_unlock(&audio_g_service_lock);
        return false;
    }

    Result rc = audoutStopAudioOut();
    if (R_FAILED(rc)) {
        SSRNX_LOGCE("AUDIO", "restart stop failed rc=0x%08x", rc);
        pthread_mutex_unlock(&audio_g_service_lock);
        return false;
    }
    audio_g_running = false;
    audio_g_paused = false;

    if (hosversionAtLeast(4, 0, 0)) {
        bool flushed = false;
        rc = audoutFlushAudioOutBuffers(&flushed);
        if (R_FAILED(rc) || !flushed) {
            SSRNX_LOGCE("AUDIO", "restart flush failed rc=0x%08x flushed=%d", rc, flushed);
            pthread_mutex_unlock(&audio_g_service_lock);
            return false;
        }
    }
    audio_drain_released_without_requeue();

    audio_g_phase = 0.0f;
    if (!audio_prime_all_buffers()) {
        pthread_mutex_unlock(&audio_g_service_lock);
        return false;
    }
    rc = audoutStartAudioOut();
    if (R_FAILED(rc)) {
        SSRNX_LOGCE("AUDIO", "restart start failed rc=0x%08x", rc);
        pthread_mutex_unlock(&audio_g_service_lock);
        return false;
    }

    audio_g_running = true;
    audio_g_queue_was_empty = false;
    audio_g_stats.saw_restart = true;
    SSRNX_LOGCI("AUDIO", "stream restart/re-prime complete");
    pthread_mutex_unlock(&audio_g_service_lock);
    return true;
}

void ssrnx_audio_get_snapshot(SsrnxAudioSnapshot *out) {
    if (!out) return;
    pthread_mutex_lock(&audio_g_service_lock);
    memset(out, 0, sizeof(*out));
    out->initialized = audio_g_initialized;
    out->running = audio_g_running;
    out->paused = audio_g_paused;
    out->sample_rate = audio_g_sample_rate;
    out->channels = audio_g_channels;
    out->pcm_format = audio_g_pcm_format;
    out->mode = audio_g_mode;
    out->frequency_hz = audio_g_frequency;
    out->volume = audio_g_volume;
    out->peak_left = audio_g_peak_left;
    out->peak_right = audio_g_peak_right;
    out->queued_buffers = audio_g_queued_buffers;
    out->played_samples = audio_g_played_samples;
    out->underruns = audio_g_stats.underruns;
    memcpy(out->wave_left, audio_g_wave_left, sizeof(out->wave_left));
    memcpy(out->wave_right, audio_g_wave_right, sizeof(out->wave_right));
    pthread_mutex_unlock(&audio_g_service_lock);
}

void ssrnx_audio_get_stats(SsrnxAudioSessionStats *out) {
    if (!out) return;
    pthread_mutex_lock(&audio_g_service_lock);
    *out = audio_g_stats;
    if (out->min_queued == UINT32_MAX) out->min_queued = 0;
    pthread_mutex_unlock(&audio_g_service_lock);
}

void ssrnx_audio_shutdown(void) {
    ssrnx_audio_stop_worker();
    pthread_mutex_lock(&audio_g_service_lock);
    if (!audio_g_initialized) {
        pthread_mutex_unlock(&audio_g_service_lock);
        return;
    }

    if (audio_g_running || audio_g_paused) (void)audoutStopAudioOut();
    if (hosversionAtLeast(4, 0, 0)) {
        bool flushed = false;
        (void)audoutFlushAudioOutBuffers(&flushed);
    }
    audio_drain_released_without_requeue();
    audio_clear_slots();
    audoutExit();

    audio_g_initialized = false;
    audio_g_running = false;
    audio_g_paused = false;
    audio_g_queue_was_empty = false;
    SSRNX_LOGCI("AUDIO", "Stage11 audio backend shutdown");
    pthread_mutex_unlock(&audio_g_service_lock);
}

#pragma pop_macro("SSRNX_AUDIO_WORKER_WAIT_NS")
#pragma pop_macro("SSRNX_AUDIO_WORKER_REPORT_US")
#pragma pop_macro("SSRNX_AUDIO_WORKER_IDLE_NS")
#pragma pop_macro("SSRNX_AUDIO_PI")
#pragma pop_macro("SSRNX_AUDIO_FRAMES_PER_BUFFER")
#pragma pop_macro("SSRNX_AUDIO_DEFAULT_VOLUME")
#pragma pop_macro("SSRNX_AUDIO_DEFAULT_HZ")
#pragma pop_macro("SSRNX_AUDIO_DATA_BYTES")
#pragma pop_macro("SSRNX_AUDIO_CHANNELS")
#pragma pop_macro("SSRNX_AUDIO_BYTES_PER_FRAME")
#pragma pop_macro("SSRNX_AUDIO_BUFFER_COUNT")
#pragma pop_macro("SSRNX_AUDIO_ALLOC_BYTES")


/* ---- aaudio ---- */
#pragma push_macro("AA_DEVICE_ID")
#pragma push_macro("AA_DEVICE_PERIOD")
#pragma push_macro("AA_DEVICE_RATE")
#pragma push_macro("AA_GET")
#pragma push_macro("AA_MAX_CHANNELS")
#pragma push_macro("AA_SET")



#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* AAudio output streams (NDK <aaudio/AAudio.h>, API 26-30 surface).
 *
 * Every output stream is a producer for the shared SSRNX output (48 kHz
 * stereo S16, refilled in 960-frame periods by the audio worker; the same
 * values AudioManager reports for OUTPUT_SAMPLE_RATE / FRAMES_PER_BUFFER).
 * A stream converts its frames to float stereo in a ring, and the mixer
 * resamples linearly from the stream rate to the device rate.
 *
 *  - data-callback streams are pulled on the audio worker thread, in bursts
 *    of framesPerDataCallback (or the burst size);
 *  - write() streams block until the ring has room or the timeout expires;
 *    a ring that runs dry while started counts an xrun and plays silence.
 *
 * State changes are synchronous: requestStart() leaves the stream STARTED.
 * The spec allows observing the final state immediately. */

enum {
    AAUDIO_OK = 0,
    AAUDIO_ERROR_DISCONNECTED = -899,
    AAUDIO_ERROR_ILLEGAL_ARGUMENT = -898,
    AAUDIO_ERROR_INTERNAL = -896,
    AAUDIO_ERROR_INVALID_STATE = -895,
    AAUDIO_ERROR_INVALID_HANDLE = -892,
    AAUDIO_ERROR_UNIMPLEMENTED = -890,
    AAUDIO_ERROR_UNAVAILABLE = -889,
    AAUDIO_ERROR_NO_FREE_HANDLES = -888,
    AAUDIO_ERROR_NO_MEMORY = -887,
    AAUDIO_ERROR_NULL = -886,
    AAUDIO_ERROR_TIMEOUT = -885,
    AAUDIO_ERROR_WOULD_BLOCK = -884,
    AAUDIO_ERROR_INVALID_FORMAT = -883,
    AAUDIO_ERROR_OUT_OF_RANGE = -882,
    AAUDIO_ERROR_NO_SERVICE = -881,
    AAUDIO_ERROR_INVALID_RATE = -880,
};

enum {
    AAUDIO_STREAM_STATE_UNINITIALIZED = 0,
    AAUDIO_STREAM_STATE_UNKNOWN,
    AAUDIO_STREAM_STATE_OPEN,
    AAUDIO_STREAM_STATE_STARTING,
    AAUDIO_STREAM_STATE_STARTED,
    AAUDIO_STREAM_STATE_PAUSING,
    AAUDIO_STREAM_STATE_PAUSED,
    AAUDIO_STREAM_STATE_FLUSHING,
    AAUDIO_STREAM_STATE_FLUSHED,
    AAUDIO_STREAM_STATE_STOPPING,
    AAUDIO_STREAM_STATE_STOPPED,
    AAUDIO_STREAM_STATE_CLOSING,
    AAUDIO_STREAM_STATE_CLOSED,
    AAUDIO_STREAM_STATE_DISCONNECTED,
};

enum { AAUDIO_DIRECTION_OUTPUT = 0, AAUDIO_DIRECTION_INPUT = 1 };
enum {
    AAUDIO_FORMAT_INVALID = -1, AAUDIO_FORMAT_UNSPECIFIED = 0, AAUDIO_FORMAT_PCM_I16 = 1,
    AAUDIO_FORMAT_PCM_FLOAT = 2, AAUDIO_FORMAT_PCM_I24_PACKED = 3, AAUDIO_FORMAT_PCM_I32 = 4,
};
enum { AAUDIO_SHARING_MODE_EXCLUSIVE = 0, AAUDIO_SHARING_MODE_SHARED = 1 };
enum { AAUDIO_PERFORMANCE_MODE_NONE = 10 };
enum { AAUDIO_USAGE_MEDIA = 1, AAUDIO_CONTENT_TYPE_MUSIC = 2, AAUDIO_INPUT_PRESET_VOICE_RECOGNITION = 6 };
enum { AAUDIO_ALLOW_CAPTURE_BY_ALL = 1, AAUDIO_SESSION_ID_NONE = -1, AAUDIO_UNSPECIFIED = 0 };
enum { AAUDIO_CALLBACK_RESULT_CONTINUE = 0, AAUDIO_CALLBACK_RESULT_STOP = 1 };

#define AA_DEVICE_RATE 48000
#define AA_DEVICE_PERIOD 960
#define AA_DEVICE_ID 2
#define AA_MAX_CHANNELS 8

struct AAudioStreamBuilderStruct {
    int32_t device_id, sample_rate, channel_count, format, sharing_mode, direction;
    int32_t capacity, performance_mode, usage, content_type, input_preset, capture_policy;
    int32_t session_id, frames_per_callback;
    uint32_t channel_mask;
    bool privacy_sensitive;
    AAudioStream_dataCallback data_cb;
    void *data_user;
    AAudioStream_errorCallback error_cb;
    void *error_user;
};

struct AAudioStreamStruct {
    AAudioStreamBuilder cfg;     /* resolved configuration */
    int32_t state;
    int32_t burst;
    int32_t capacity;
    int32_t buffer_size;
    int32_t bytes_per_frame;
    /* float stereo ring, in stream frames */
    float *ring;
    int32_t ring_cap, ring_head, ring_fill;
    double phase;                /* position between ring[head] and ring[head+1] */
    double step;                 /* stream frames per device frame */
    uint8_t *cb_buf;
    int32_t cb_frames;
    int64_t frames_written, frames_read;
    int32_t drain_remaining;    /* AudioTrack stop; zero for native AAudio */
    int32_t xruns;
    uint64_t callbacks;
    uint64_t diagnostic_frames;
    float input_peak, output_peak;
    bool in_mix;
    bool closing;
    pthread_cond_t cond;         /* state changes, ring space, mix done */
    AAudioStream *next;
};

static pthread_mutex_t aaudio_g_aa_lock = PTHREAD_MUTEX_INITIALIZER;
static AAudioStream *aaudio_g_aa_streams;
static pthread_t aaudio_g_aa_mix_thread;
static bool aaudio_g_aa_mix_thread_valid;
static uint64_t aaudio_g_aa_opened;

const char *AAudio_convertResultToText(aaudio_result_t r) {
    switch (r) {
    case AAUDIO_OK: return "AAUDIO_OK";
    case AAUDIO_ERROR_DISCONNECTED: return "AAUDIO_ERROR_DISCONNECTED";
    case AAUDIO_ERROR_ILLEGAL_ARGUMENT: return "AAUDIO_ERROR_ILLEGAL_ARGUMENT";
    case AAUDIO_ERROR_INTERNAL: return "AAUDIO_ERROR_INTERNAL";
    case AAUDIO_ERROR_INVALID_STATE: return "AAUDIO_ERROR_INVALID_STATE";
    case AAUDIO_ERROR_INVALID_HANDLE: return "AAUDIO_ERROR_INVALID_HANDLE";
    case AAUDIO_ERROR_UNIMPLEMENTED: return "AAUDIO_ERROR_UNIMPLEMENTED";
    case AAUDIO_ERROR_UNAVAILABLE: return "AAUDIO_ERROR_UNAVAILABLE";
    case AAUDIO_ERROR_NO_FREE_HANDLES: return "AAUDIO_ERROR_NO_FREE_HANDLES";
    case AAUDIO_ERROR_NO_MEMORY: return "AAUDIO_ERROR_NO_MEMORY";
    case AAUDIO_ERROR_NULL: return "AAUDIO_ERROR_NULL";
    case AAUDIO_ERROR_TIMEOUT: return "AAUDIO_ERROR_TIMEOUT";
    case AAUDIO_ERROR_WOULD_BLOCK: return "AAUDIO_ERROR_WOULD_BLOCK";
    case AAUDIO_ERROR_INVALID_FORMAT: return "AAUDIO_ERROR_INVALID_FORMAT";
    case AAUDIO_ERROR_OUT_OF_RANGE: return "AAUDIO_ERROR_OUT_OF_RANGE";
    case AAUDIO_ERROR_NO_SERVICE: return "AAUDIO_ERROR_NO_SERVICE";
    case AAUDIO_ERROR_INVALID_RATE: return "AAUDIO_ERROR_INVALID_RATE";
    default: return "Unrecognized AAudio error.";
    }
}

const char *AAudio_convertStreamStateToText(aaudio_stream_state_t s) {
    static const char *const names[] = {
        "AAUDIO_STREAM_STATE_UNINITIALIZED", "AAUDIO_STREAM_STATE_UNKNOWN", "AAUDIO_STREAM_STATE_OPEN",
        "AAUDIO_STREAM_STATE_STARTING", "AAUDIO_STREAM_STATE_STARTED", "AAUDIO_STREAM_STATE_PAUSING",
        "AAUDIO_STREAM_STATE_PAUSED", "AAUDIO_STREAM_STATE_FLUSHING", "AAUDIO_STREAM_STATE_FLUSHED",
        "AAUDIO_STREAM_STATE_STOPPING", "AAUDIO_STREAM_STATE_STOPPED", "AAUDIO_STREAM_STATE_CLOSING",
        "AAUDIO_STREAM_STATE_CLOSED", "AAUDIO_STREAM_STATE_DISCONNECTED",
    };
    return s >= 0 && s < (int32_t)(sizeof(names) / sizeof(names[0])) ? names[s] : "Unrecognized AAudio state.";
}

/* ---- builder ------------------------------------------------------------ */

aaudio_result_t AAudio_createStreamBuilder(AAudioStreamBuilder **builder) {
    if (!builder) return AAUDIO_ERROR_NULL;
    AAudioStreamBuilder *b = calloc(1, sizeof(*b));
    if (!b) { *builder = NULL; return AAUDIO_ERROR_NO_MEMORY; }
    b->sharing_mode = AAUDIO_SHARING_MODE_SHARED;
    b->direction = AAUDIO_DIRECTION_OUTPUT;
    b->performance_mode = AAUDIO_PERFORMANCE_MODE_NONE;
    b->usage = AAUDIO_USAGE_MEDIA;
    b->content_type = AAUDIO_CONTENT_TYPE_MUSIC;
    b->input_preset = AAUDIO_INPUT_PRESET_VOICE_RECOGNITION;
    b->capture_policy = AAUDIO_ALLOW_CAPTURE_BY_ALL;
    b->session_id = AAUDIO_SESSION_ID_NONE;
    *builder = b;
    return AAUDIO_OK;
}

#define AA_SET(fn, field, type) \
    void AAudioStreamBuilder_##fn(AAudioStreamBuilder *b, type v) { if (b) b->field = v; }
AA_SET(setDeviceId, device_id, int32_t)
AA_SET(setSampleRate, sample_rate, int32_t)
AA_SET(setChannelCount, channel_count, int32_t)
AA_SET(setSamplesPerFrame, channel_count, int32_t)
AA_SET(setFormat, format, int32_t)
AA_SET(setSharingMode, sharing_mode, int32_t)
AA_SET(setDirection, direction, int32_t)
AA_SET(setBufferCapacityInFrames, capacity, int32_t)
AA_SET(setPerformanceMode, performance_mode, int32_t)
AA_SET(setUsage, usage, int32_t)
AA_SET(setContentType, content_type, int32_t)
AA_SET(setInputPreset, input_preset, int32_t)
AA_SET(setAllowedCapturePolicy, capture_policy, int32_t)
AA_SET(setSessionId, session_id, int32_t)
AA_SET(setPrivacySensitive, privacy_sensitive, _Bool)
AA_SET(setChannelMask, channel_mask, uint32_t)
AA_SET(setFramesPerDataCallback, frames_per_callback, int32_t)
#undef AA_SET

void AAudioStreamBuilder_setPackageName(AAudioStreamBuilder *b, const char *name) { (void)b; (void)name; }
void AAudioStreamBuilder_setAttributionTag(AAudioStreamBuilder *b, const char *tag) { (void)b; (void)tag; }

void AAudioStreamBuilder_setDataCallback(AAudioStreamBuilder *b, AAudioStream_dataCallback cb, void *user) {
    if (b) { b->data_cb = cb; b->data_user = user; }
}

void AAudioStreamBuilder_setErrorCallback(AAudioStreamBuilder *b, AAudioStream_errorCallback cb, void *user) {
    if (b) { b->error_cb = cb; b->error_user = user; }
}

aaudio_result_t AAudioStreamBuilder_delete(AAudioStreamBuilder *b) {
    free(b);
    return AAUDIO_OK;
}

/* AAUDIO_CHANNEL_* position masks (API 32): the channel count is the number
 * of set bits; index masks are not used by output streams here. */
static int32_t aaudio_channel_count_of_mask(uint32_t mask) {
    return mask ? (int32_t)__builtin_popcount(mask & 0x00ffffffu) : 0;
}

static int32_t aaudio_format_bytes(int32_t format) {
    switch (format) {
    case AAUDIO_FORMAT_PCM_I16: return 2;
    case AAUDIO_FORMAT_PCM_FLOAT: return 4;
    case AAUDIO_FORMAT_PCM_I24_PACKED: return 3;
    case AAUDIO_FORMAT_PCM_I32: return 4;
    default: return 0;
    }
}

aaudio_result_t AAudioStreamBuilder_openStream(AAudioStreamBuilder *b, AAudioStream **out) {
    if (!b || !out) return AAUDIO_ERROR_NULL;
    *out = NULL;
    if (b->direction == AAUDIO_DIRECTION_INPUT) {
        SSRNX_LOGCW("AAUDIO", "openStream input direction unavailable (no capture device)");
        return AAUDIO_ERROR_UNAVAILABLE;
    }
    if (b->direction != AAUDIO_DIRECTION_OUTPUT) return AAUDIO_ERROR_ILLEGAL_ARGUMENT;
    if (b->device_id != AAUDIO_UNSPECIFIED && b->device_id != AA_DEVICE_ID) {
        SSRNX_LOGCW("AAUDIO", "openStream unknown deviceId=%d", b->device_id);
        return AAUDIO_ERROR_ILLEGAL_ARGUMENT;
    }
    AAudioStream *s = calloc(1, sizeof(*s));
    if (!s) return AAUDIO_ERROR_NO_MEMORY;
    s->cfg = *b;
    AAudioStreamBuilder *c = &s->cfg;
    c->device_id = AA_DEVICE_ID;
    c->sharing_mode = AAUDIO_SHARING_MODE_SHARED; /* EXCLUSIVE may be downgraded */
    if (c->sample_rate == AAUDIO_UNSPECIFIED) c->sample_rate = AA_DEVICE_RATE;
    if (c->channel_count == AAUDIO_UNSPECIFIED) c->channel_count = aaudio_channel_count_of_mask(c->channel_mask);
    if (c->channel_count == AAUDIO_UNSPECIFIED) c->channel_count = 2;
    if (c->format == AAUDIO_FORMAT_UNSPECIFIED) c->format = AAUDIO_FORMAT_PCM_FLOAT;
    aaudio_result_t rc = AAUDIO_OK;
    if (c->sample_rate < 8000 || c->sample_rate > 192000) rc = AAUDIO_ERROR_INVALID_RATE;
    else if (c->channel_count < 1 || c->channel_count > AA_MAX_CHANNELS) rc = AAUDIO_ERROR_OUT_OF_RANGE;
    else if (!aaudio_format_bytes(c->format)) rc = AAUDIO_ERROR_INVALID_FORMAT;
    if (rc != AAUDIO_OK) {
        SSRNX_LOGCW("AAUDIO", "openStream rejected rate=%d channels=%d format=%d rc=%s",
                    c->sample_rate, c->channel_count, c->format, AAudio_convertResultToText(rc));
        free(s);
        return rc;
    }
    s->bytes_per_frame = c->channel_count * aaudio_format_bytes(c->format);
    s->step = (double)c->sample_rate / (double)AA_DEVICE_RATE;
    s->burst = (int32_t)(((int64_t)AA_DEVICE_PERIOD * c->sample_rate + AA_DEVICE_RATE / 2) / AA_DEVICE_RATE);
    if (s->burst < 16) s->burst = 16;
    s->capacity = c->capacity > 4 * s->burst ? c->capacity : 4 * s->burst;
    s->buffer_size = 2 * s->burst;
    s->cb_frames = c->frames_per_callback > 0 ? c->frames_per_callback : s->burst;
    /* Room for a full device period plus one callback beyond the capacity. */
    s->ring_cap = s->capacity + s->cb_frames + s->burst + 8;
    s->ring = calloc((size_t)s->ring_cap * 2u, sizeof(float));
    s->cb_buf = c->data_cb ? calloc((size_t)s->cb_frames, (size_t)s->bytes_per_frame) : NULL;
    if (!s->ring || (c->data_cb && !s->cb_buf)) {
        free(s->ring); free(s->cb_buf); free(s);
        return AAUDIO_ERROR_NO_MEMORY;
    }
    pthread_cond_init(&s->cond, NULL);
    s->state = AAUDIO_STREAM_STATE_OPEN;
    pthread_mutex_lock(&aaudio_g_aa_lock);
    s->next = aaudio_g_aa_streams;
    aaudio_g_aa_streams = s;
    const uint64_t n = ++aaudio_g_aa_opened;
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    SSRNX_LOGCI("AAUDIO", "openStream #%llu rate=%d channels=%d format=%d perf=%d callback=%d framesPerCallback=%d burst=%d capacity=%d",
                (unsigned long long)n, c->sample_rate, c->channel_count, c->format, c->performance_mode,
                c->data_cb ? 1 : 0, c->frames_per_callback, s->burst, s->capacity);
    *out = s;
    return AAUDIO_OK;
}

/* ---- conversion --------------------------------------------------------- */

static float aaudio_sample_to_float(const uint8_t *p, int32_t format) {
    switch (format) {
    case AAUDIO_FORMAT_PCM_I16: { int16_t v; memcpy(&v, p, 2); return (float)v / 32768.0f; }
    case AAUDIO_FORMAT_PCM_FLOAT: { float v; memcpy(&v, p, 4); return v; }
    case AAUDIO_FORMAT_PCM_I24_PACKED: {
        int32_t v = (int32_t)((uint32_t)p[0] << 8 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 24);
        return (float)(v >> 8) / 8388608.0f;
    }
    case AAUDIO_FORMAT_PCM_I32: { int32_t v; memcpy(&v, p, 4); return (float)v / 2147483648.0f; }
    default: return 0.0f;
    }
}

/* Appends up to `frames` stream frames to the ring (front-left/right; mono is
 * duplicated). Caller holds g_aa_lock. Returns frames appended. */
static int32_t aaudio_ring_push(AAudioStream *s, const uint8_t *data, int32_t frames) {
    const int32_t room = s->ring_cap - s->ring_fill;
    if (frames > room) frames = room;
    const int32_t ch = s->cfg.channel_count;
    const int32_t sb = aaudio_format_bytes(s->cfg.format);
    for (int32_t i = 0; i < frames; ++i) {
        const uint8_t *f = data + (size_t)i * (size_t)s->bytes_per_frame;
        const float l = aaudio_sample_to_float(f, s->cfg.format);
        const float r = ch > 1 ? aaudio_sample_to_float(f + sb, s->cfg.format) : l;
        const float peak = fmaxf(fabsf(l), fabsf(r));
        if (peak > s->input_peak) s->input_peak = peak;
        const int32_t at = (s->ring_head + s->ring_fill) % s->ring_cap;
        s->ring[at * 2] = l;
        s->ring[at * 2 + 1] = r;
        s->ring_fill++;
    }
    return frames;
}

static void aaudio_ring_clear(AAudioStream *s) {
    s->ring_head = 0;
    s->ring_fill = 0;
    s->phase = 0.0;
}

static void aaudio_notify_state(AAudioStream *s, int32_t state) {
    s->state = state;
    pthread_cond_broadcast(&s->cond);
}

/* ---- stream control ----------------------------------------------------- */

static bool aaudio_stream_live(const AAudioStream *s) {
    return s && s->state != AAUDIO_STREAM_STATE_CLOSED && !s->closing;
}

aaudio_result_t AAudioStream_requestStart(AAudioStream *s) {
    if (!s) return AAUDIO_ERROR_NULL;
    pthread_mutex_lock(&aaudio_g_aa_lock);
    aaudio_result_t rc = AAUDIO_OK;
    if (!aaudio_stream_live(s) || s->state == AAUDIO_STREAM_STATE_DISCONNECTED) rc = AAUDIO_ERROR_INVALID_STATE;
    else if (s->state != AAUDIO_STREAM_STATE_STARTED) {
        if (s->state == AAUDIO_STREAM_STATE_STOPPED) s->phase = 0.0;
        aaudio_notify_state(s, AAUDIO_STREAM_STATE_STARTED);
    }
    if (rc == AAUDIO_OK) s->drain_remaining = 0;
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    SSRNX_LOGCI("AAUDIO", "requestStart stream=%p rc=%s", (void *)s, AAudio_convertResultToText(rc));
    return rc;
}

aaudio_result_t AAudioStream_requestPause(AAudioStream *s) {
    if (!s) return AAUDIO_ERROR_NULL;
    pthread_mutex_lock(&aaudio_g_aa_lock);
    aaudio_result_t rc = AAUDIO_OK;
    if (!aaudio_stream_live(s)) rc = AAUDIO_ERROR_INVALID_STATE;
    else if (s->state == AAUDIO_STREAM_STATE_STARTED || s->state == AAUDIO_STREAM_STATE_OPEN ||
             s->state == AAUDIO_STREAM_STATE_FLUSHED || s->state == AAUDIO_STREAM_STATE_STOPPED)
        aaudio_notify_state(s, AAUDIO_STREAM_STATE_PAUSED);
    else if (s->state != AAUDIO_STREAM_STATE_PAUSED) rc = AAUDIO_ERROR_INVALID_STATE;
    if (rc == AAUDIO_OK) s->drain_remaining = 0;
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    return rc;
}

aaudio_result_t AAudioStream_requestFlush(AAudioStream *s) {
    if (!s) return AAUDIO_ERROR_NULL;
    pthread_mutex_lock(&aaudio_g_aa_lock);
    aaudio_result_t rc = AAUDIO_OK;
    if (!aaudio_stream_live(s)) rc = AAUDIO_ERROR_INVALID_STATE;
    else if (s->state == AAUDIO_STREAM_STATE_PAUSED || s->state == AAUDIO_STREAM_STATE_OPEN ||
             s->state == AAUDIO_STREAM_STATE_FLUSHED || s->state == AAUDIO_STREAM_STATE_STOPPED) {
        /* Discarded frames still count as read, keeping written - read the
         * number of frames pending in the buffer. */
        s->frames_read += s->ring_fill;
        aaudio_ring_clear(s);
        aaudio_notify_state(s, AAUDIO_STREAM_STATE_FLUSHED);
    } else rc = AAUDIO_ERROR_INVALID_STATE;
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    return rc;
}

aaudio_result_t AAudioStream_requestStop(AAudioStream *s) {
    if (!s) return AAUDIO_ERROR_NULL;
    pthread_mutex_lock(&aaudio_g_aa_lock);
    aaudio_result_t rc = AAUDIO_OK;
    if (!aaudio_stream_live(s)) rc = AAUDIO_ERROR_INVALID_STATE;
    else if (s->state != AAUDIO_STREAM_STATE_STOPPED) {
        /* A stopped output stream discards what was not yet played. */
        s->frames_read += s->ring_fill;
        aaudio_ring_clear(s);
        aaudio_notify_state(s, AAUDIO_STREAM_STATE_STOPPED);
    }
    if (rc == AAUDIO_OK) s->drain_remaining = 0;
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    return rc;
}

aaudio_result_t ssrnx_aaudio_stop_after_queued_frames(AAudioStream *s) {
    if (!s) return AAUDIO_ERROR_NULL;
    pthread_mutex_lock(&aaudio_g_aa_lock);
    const aaudio_result_t rc = aaudio_stream_live(s) && s->state != AAUDIO_STREAM_STATE_DISCONNECTED
        ? AAUDIO_OK : AAUDIO_ERROR_INVALID_STATE;
    if (rc == AAUDIO_OK) {
        s->drain_remaining = s->ring_fill;
        aaudio_notify_state(s, s->drain_remaining ? AAUDIO_STREAM_STATE_STARTED : AAUDIO_STREAM_STATE_STOPPED);
    }
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    return rc;
}

aaudio_stream_state_t AAudioStream_getState(AAudioStream *s) {
    if (!s) return AAUDIO_STREAM_STATE_UNINITIALIZED;
    pthread_mutex_lock(&aaudio_g_aa_lock);
    const int32_t st = s->state;
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    return st;
}

static void aaudio_deadline_after(struct timespec *ts, int64_t nanos) {
    clock_gettime(CLOCK_REALTIME, ts);
    if (nanos < 0) nanos = 0;
    ts->tv_sec += (time_t)(nanos / 1000000000);
    ts->tv_nsec += (long)(nanos % 1000000000);
    if (ts->tv_nsec >= 1000000000L) { ts->tv_sec++; ts->tv_nsec -= 1000000000L; }
}

aaudio_result_t AAudioStream_waitForStateChange(AAudioStream *s, aaudio_stream_state_t input,
                                                aaudio_stream_state_t *next, int64_t timeout_nanos) {
    if (!s) return AAUDIO_ERROR_NULL;
    struct timespec deadline;
    aaudio_deadline_after(&deadline, timeout_nanos);
    pthread_mutex_lock(&aaudio_g_aa_lock);
    aaudio_result_t rc = AAUDIO_OK;
    while (s->state == input && rc == AAUDIO_OK) {
        if (timeout_nanos <= 0 || pthread_cond_timedwait(&s->cond, &aaudio_g_aa_lock, &deadline) == ETIMEDOUT)
            rc = s->state == input ? AAUDIO_ERROR_TIMEOUT : AAUDIO_OK;
    }
    if (next) *next = s->state;
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    return rc;
}

aaudio_result_t AAudioStream_close(AAudioStream *s) {
    if (!s) return AAUDIO_ERROR_NULL;
    pthread_mutex_lock(&aaudio_g_aa_lock);
    if (s->closing) { pthread_mutex_unlock(&aaudio_g_aa_lock); return AAUDIO_ERROR_INVALID_STATE; }
    s->closing = true;
    aaudio_notify_state(s, AAUDIO_STREAM_STATE_CLOSING);
    for (AAudioStream **it = &aaudio_g_aa_streams; *it; it = &(*it)->next)
        if (*it == s) { *it = s->next; break; }
    /* Never free under a running data callback. A callback closing its own
     * stream is a spec violation; avoid deadlocking on it all the same. */
    const bool on_mixer = aaudio_g_aa_mix_thread_valid && pthread_equal(aaudio_g_aa_mix_thread, pthread_self());
    while (s->in_mix && !on_mixer) pthread_cond_wait(&s->cond, &aaudio_g_aa_lock);
    const bool defer = s->in_mix;
    s->state = AAUDIO_STREAM_STATE_CLOSED;
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    SSRNX_LOGCI("AAUDIO", "close stream=%p written=%lld read=%lld xruns=%d callbacks=%llu",
                (void *)s, (long long)s->frames_written, (long long)s->frames_read, s->xruns,
                (unsigned long long)s->callbacks);
    if (defer) return AAUDIO_OK; /* freed by the mixer once the callback returns */
    pthread_cond_destroy(&s->cond);
    free(s->ring);
    free(s->cb_buf);
    free(s);
    return AAUDIO_OK;
}

aaudio_result_t AAudioStream_release(AAudioStream *s) {
    if (!s) return AAUDIO_ERROR_NULL;
    pthread_mutex_lock(&aaudio_g_aa_lock);
    aaudio_result_t rc = AAUDIO_OK;
    if (!aaudio_stream_live(s)) rc = AAUDIO_ERROR_INVALID_STATE;
    else if (s->state != AAUDIO_STREAM_STATE_STOPPED && s->state != AAUDIO_STREAM_STATE_OPEN) {
        s->frames_read += s->ring_fill;
        aaudio_ring_clear(s);
        aaudio_notify_state(s, AAUDIO_STREAM_STATE_STOPPED);
    }
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    return rc;
}

/* ---- blocking I/O ------------------------------------------------------- */

int32_t AAudioStream_write(AAudioStream *s, const void *buffer, int32_t num_frames, int64_t timeout_nanos) {
    if (!s || !buffer) return AAUDIO_ERROR_NULL;
    if (num_frames < 0) return AAUDIO_ERROR_ILLEGAL_ARGUMENT;
    if (s->cfg.data_cb) return AAUDIO_ERROR_INVALID_STATE;
    struct timespec deadline;
    aaudio_deadline_after(&deadline, timeout_nanos);
    const uint8_t *src = buffer;
    int32_t done = 0;
    pthread_mutex_lock(&aaudio_g_aa_lock);
    while (done < num_frames) {
        if (!aaudio_stream_live(s) || s->state == AAUDIO_STREAM_STATE_DISCONNECTED) {
            if (!done) done = AAUDIO_ERROR_INVALID_STATE;
            break;
        }
        int32_t room = s->buffer_size - s->ring_fill;
        if (room > 0) {
            const int32_t want = num_frames - done < room ? num_frames - done : room;
            const int32_t n = aaudio_ring_push(s, src + (size_t)done * (size_t)s->bytes_per_frame, want);
            done += n;
            s->frames_written += n;
            if (n > 0) continue;
        }
        if (timeout_nanos <= 0) break;
        if (pthread_cond_timedwait(&s->cond, &aaudio_g_aa_lock, &deadline) == ETIMEDOUT) break;
    }
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    return done;
}

int32_t AAudioStream_read(AAudioStream *s, void *buffer, int32_t num_frames, int64_t timeout_nanos) {
    (void)buffer; (void)num_frames; (void)timeout_nanos;
    if (!s) return AAUDIO_ERROR_NULL;
    return AAUDIO_ERROR_UNIMPLEMENTED; /* output streams only */
}

/* ---- queries ------------------------------------------------------------ */

aaudio_result_t AAudioStream_setBufferSizeInFrames(AAudioStream *s, int32_t frames) {
    if (!s) return AAUDIO_ERROR_NULL;
    pthread_mutex_lock(&aaudio_g_aa_lock);
    if (frames < s->burst) frames = s->burst;
    if (frames > s->capacity) frames = s->capacity;
    s->buffer_size = frames;
    pthread_cond_broadcast(&s->cond);
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    return frames;
}

#define AA_GET(fn, type, expr) \
    type AAudioStream_##fn(AAudioStream *s) { return s ? (type)(expr) : (type)0; }
AA_GET(getBufferSizeInFrames, int32_t, s->buffer_size)
AA_GET(getFramesPerBurst, int32_t, s->burst)
AA_GET(getBufferCapacityInFrames, int32_t, s->capacity)
AA_GET(getFramesPerDataCallback, int32_t, s->cfg.frames_per_callback)
AA_GET(getXRunCount, int32_t, s->xruns)
AA_GET(getSampleRate, int32_t, s->cfg.sample_rate)
AA_GET(getChannelCount, int32_t, s->cfg.channel_count)
AA_GET(getSamplesPerFrame, int32_t, s->cfg.channel_count)
AA_GET(getDeviceId, int32_t, s->cfg.device_id)
AA_GET(getFormat, int32_t, s->cfg.format)
AA_GET(getSharingMode, int32_t, s->cfg.sharing_mode)
AA_GET(getPerformanceMode, int32_t, s->cfg.performance_mode)
AA_GET(getDirection, int32_t, s->cfg.direction)
AA_GET(getSessionId, int32_t, s->cfg.session_id)
AA_GET(getUsage, int32_t, s->cfg.usage)
AA_GET(getContentType, int32_t, s->cfg.content_type)
AA_GET(getInputPreset, int32_t, s->cfg.input_preset)
AA_GET(getAllowedCapturePolicy, int32_t, s->cfg.capture_policy)
AA_GET(isPrivacySensitive, _Bool, s->cfg.privacy_sensitive)
AA_GET(getChannelMask, uint32_t, s->cfg.channel_mask)
AA_GET(isMMapUsed, _Bool, 0)
#undef AA_GET

int64_t AAudioStream_getFramesWritten(AAudioStream *s) {
    if (!s) return 0;
    pthread_mutex_lock(&aaudio_g_aa_lock);
    const int64_t v = s->frames_written;
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    return v;
}

int64_t AAudioStream_getFramesRead(AAudioStream *s) {
    if (!s) return 0;
    pthread_mutex_lock(&aaudio_g_aa_lock);
    const int64_t v = s->frames_read;
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    return v;
}

aaudio_result_t AAudioStream_getTimestamp(AAudioStream *s, int32_t clockid, int64_t *frame_position,
                                          int64_t *time_nanoseconds) {
    if (!s || !frame_position || !time_nanoseconds) return AAUDIO_ERROR_NULL;
    struct timespec now;
    if (clock_gettime((clockid_t)clockid, &now) != 0) return AAUDIO_ERROR_ILLEGAL_ARGUMENT;
    pthread_mutex_lock(&aaudio_g_aa_lock);
    const bool started = s->state == AAUDIO_STREAM_STATE_STARTED;
    const int64_t read = s->frames_read;
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    if (!started || read <= 0) return AAUDIO_ERROR_INVALID_STATE;
    *frame_position = read;
    *time_nanoseconds = (int64_t)now.tv_sec * 1000000000LL + now.tv_nsec;
    return AAUDIO_OK;
}

/* ---- mixer (audio worker thread) ---------------------------------------- */

/* Pulls data callbacks until the ring holds `need` frames. Called with
 * g_aa_lock held and s->in_mix set; the lock is dropped around each callback
 * so the app may call back into the stream API. */
static void aaudio_fill_from_callback(AAudioStream *s, int32_t need) {
    int guard = 64;
    while (s->ring_fill < need && s->state == AAUDIO_STREAM_STATE_STARTED && !s->closing && guard-- > 0) {
        if (s->ring_cap - s->ring_fill < s->cb_frames) break;
        AAudioStream_dataCallback cb = s->cfg.data_cb;
        void *user = s->cfg.data_user;
        const int32_t frames = s->cb_frames;
        pthread_mutex_unlock(&aaudio_g_aa_lock);
        memset(s->cb_buf, 0, (size_t)frames * (size_t)s->bytes_per_frame);
        const aaudio_data_callback_result_t r = cb(s, user, s->cb_buf, frames);
        pthread_mutex_lock(&aaudio_g_aa_lock);
        s->callbacks++;
        if (s->callbacks == 1)
            SSRNX_LOGCI("AAUDIO", "first data callback stream=%p frames=%d result=%d", (void *)s, frames, r);
        if (s->closing) break;
        s->frames_written += aaudio_ring_push(s, s->cb_buf, frames);
        if (r != AAUDIO_CALLBACK_RESULT_CONTINUE) {
            aaudio_notify_state(s, AAUDIO_STREAM_STATE_STOPPED);
            break;
        }
    }
}

static int16_t aaudio_clamp_s16(float v) {
    if (v > 1.0f) v = 1.0f;
    if (v < -1.0f) v = -1.0f;
    return (int16_t)(v * 32767.0f);
}

static void aaudio_mix_stream(AAudioStream *s, int16_t *out, size_t frames) {
    if (s->cfg.data_cb) {
        const int32_t need = (int32_t)(s->phase + (double)frames * s->step) + 2;
        aaudio_fill_from_callback(s, need);
    }
    if (s->state != AAUDIO_STREAM_STATE_STARTED || s->closing) return;
    bool starved = false;
    for (size_t i = 0; i < frames; ++i) {
        if (s->ring_fill < 1) { starved = true; break; }
        const int32_t a = s->ring_head;
        const int32_t b = s->ring_fill > 1 ? (a + 1) % s->ring_cap : a;
        const float t = (float)s->phase;
        const float l = s->ring[a * 2] + (s->ring[b * 2] - s->ring[a * 2]) * t;
        const float r = s->ring[a * 2 + 1] + (s->ring[b * 2 + 1] - s->ring[a * 2 + 1]) * t;
        const float peak = fmaxf(fabsf(l), fabsf(r));
        if (peak > s->output_peak) s->output_peak = peak;
        out[i * 2] = aaudio_clamp_s16((float)out[i * 2] / 32768.0f + l);
        out[i * 2 + 1] = aaudio_clamp_s16((float)out[i * 2 + 1] / 32768.0f + r);
        s->phase += s->step;
        while (s->phase >= 1.0 && s->ring_fill > 0) {
            s->phase -= 1.0;
            s->ring_head = (s->ring_head + 1) % s->ring_cap;
            s->ring_fill--;
            s->frames_read++;
            if (s->drain_remaining > 0 && --s->drain_remaining == 0) {
                s->phase = 0.0;
                aaudio_notify_state(s, AAUDIO_STREAM_STATE_STOPPED);
                break;
            }
        }
        if (s->state != AAUDIO_STREAM_STATE_STARTED) break;
    }
    if (starved && s->frames_written > 0) s->xruns++;
    s->diagnostic_frames += frames;
    if (s->diagnostic_frames >= AA_DEVICE_RATE * 2u) {
        s->diagnostic_frames = 0;
        SSRNX_LOGCI("AAUDIO", "stream heartbeat stream=%p state=%d rate=%d queued=%d written=%lld read=%lld xruns=%d inputPeak=%.5f outputPeak=%.5f",
                    (void *)s, s->state, s->cfg.sample_rate, s->ring_fill,
                    (long long)s->frames_written, (long long)s->frames_read, s->xruns,
                    (double)s->input_peak, (double)s->output_peak);
        s->input_peak = s->output_peak = 0.0f;
    }
    pthread_cond_broadcast(&s->cond); /* ring space for blocked writers */
}

size_t ssrnx_aaudio_mix_s16(int16_t *io_interleaved, size_t frames) {
    if (!io_interleaved || !frames) return 0;
    pthread_mutex_lock(&aaudio_g_aa_lock);
    aaudio_g_aa_mix_thread = pthread_self();
    aaudio_g_aa_mix_thread_valid = true;
    size_t mixed = 0;
    for (AAudioStream *s = aaudio_g_aa_streams; s; ) {
        if (s->state != AAUDIO_STREAM_STATE_STARTED || s->closing) { s = s->next; continue; }
        s->in_mix = true;
        aaudio_mix_stream(s, io_interleaved, frames);
        s->in_mix = false;
        mixed++;
        AAudioStream *next = s->next;
        if (s->closing) {
            /* close() returned early (called from this thread); finish it. */
            pthread_cond_broadcast(&s->cond);
            if (s->state == AAUDIO_STREAM_STATE_CLOSED) {
                pthread_cond_destroy(&s->cond);
                free(s->ring);
                free(s->cb_buf);
                free(s);
            }
        } else {
            pthread_cond_broadcast(&s->cond);
        }
        s = next;
    }
    pthread_mutex_unlock(&aaudio_g_aa_lock);
    return mixed;
}

void ssrnx_aaudio_reset(void) {
    /* Streams belong to the previous guest runtime; never call its callbacks. */
    pthread_mutex_lock(&aaudio_g_aa_lock);
    aaudio_g_aa_streams = NULL;
    aaudio_g_aa_opened = 0;
    pthread_mutex_unlock(&aaudio_g_aa_lock);
}

#pragma pop_macro("AA_SET")
#pragma pop_macro("AA_MAX_CHANNELS")
#pragma pop_macro("AA_GET")
#pragma pop_macro("AA_DEVICE_RATE")
#pragma pop_macro("AA_DEVICE_PERIOD")
#pragma pop_macro("AA_DEVICE_ID")


/* ---- opensles ---- */
#pragma push_macro("SSRNX_SL_PCM_OUTPUT_RATE")
#pragma push_macro("SSRNX_SL_PCM_QUEUE_MAX")




#include <limits.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* OpenSL ES C interfaces are pointer-to-pointer vtables. */
typedef const struct opensles_SsrnxSLObjectVTable * const * opensles_SsrnxSLObjectItf;
typedef const struct opensles_SsrnxSLEngineVTable * const * opensles_SsrnxSLEngineItf;
typedef const struct opensles_SsrnxSLPlayVTable * const * opensles_SsrnxSLPlayItf;
typedef const struct opensles_SsrnxSLSeekVTable * const * opensles_SsrnxSLSeekItf;
typedef const struct opensles_SsrnxSLAndroidSimpleBufferQueueVTable * const * opensles_SsrnxSLAndroidSimpleBufferQueueItf;
typedef const struct opensles_SsrnxSLAndroidConfigurationVTable * const * opensles_SsrnxSLAndroidConfigurationItf;

typedef struct opensles_SsrnxSLObject opensles_SsrnxSLObject;
typedef struct opensles_SsrnxSLHandle {
    const void *vtable;
    opensles_SsrnxSLObject *owner;
} opensles_SsrnxSLHandle;

struct opensles_SsrnxSLObjectVTable {
    SsrnxSLresult (*Realize)(opensles_SsrnxSLObjectItf self, SsrnxSLboolean async);
    SsrnxSLresult (*Resume)(opensles_SsrnxSLObjectItf self, SsrnxSLboolean async);
    SsrnxSLresult (*GetState)(opensles_SsrnxSLObjectItf self, SsrnxSLuint32 *state);
    SsrnxSLresult (*GetInterface)(opensles_SsrnxSLObjectItf self, const void *iid, void *out_interface);
    SsrnxSLresult (*RegisterCallback)(opensles_SsrnxSLObjectItf self, void *callback, void *context);
    void (*AbortAsyncOperation)(opensles_SsrnxSLObjectItf self);
    void (*Destroy)(opensles_SsrnxSLObjectItf self);
    SsrnxSLresult (*SetPriority)(opensles_SsrnxSLObjectItf self, SsrnxSLint32 priority, SsrnxSLboolean preemptable);
    SsrnxSLresult (*GetPriority)(opensles_SsrnxSLObjectItf self, SsrnxSLint32 *priority, SsrnxSLboolean *preemptable);
    SsrnxSLresult (*SetLossOfControlInterfaces)(opensles_SsrnxSLObjectItf self, int16_t num, void *ids, SsrnxSLboolean enabled);
};

/* Slots 0..6 are CreateLEDDevice, CreateVibraDevice, CreateAudioPlayer,
 * CreateAudioRecorder, CreateMidiPlayer, CreateListener, Create3DGroup;
 * CreateOutputMix is slot 7 in the Khronos/Android ABI. */
struct opensles_SsrnxSLEngineVTable {
    SsrnxSLresult (*CreateLEDDevice)(opensles_SsrnxSLEngineItf, void*, SsrnxSLuint32, SsrnxSLuint32, const void*, const void*);
    SsrnxSLresult (*CreateVibraDevice)(opensles_SsrnxSLEngineItf, void*, SsrnxSLuint32, SsrnxSLuint32, const void*, const void*);
    SsrnxSLresult (*CreateAudioPlayer)(opensles_SsrnxSLEngineItf, void*, void*, void*, SsrnxSLuint32, const void*, const void*);
    SsrnxSLresult (*CreateAudioRecorder)(opensles_SsrnxSLEngineItf, void*, void*, void*, SsrnxSLuint32, const void*, const void*);
    SsrnxSLresult (*CreateMidiPlayer)(opensles_SsrnxSLEngineItf, void*, void*, void*, void*, void*, void*, SsrnxSLuint32, const void*, const void*);
    SsrnxSLresult (*CreateListener)(opensles_SsrnxSLEngineItf, void*, SsrnxSLuint32, const void*, const void*);
    SsrnxSLresult (*Create3DGroup)(opensles_SsrnxSLEngineItf, void*, SsrnxSLuint32, const void*, const void*);
    SsrnxSLresult (*CreateOutputMix)(opensles_SsrnxSLEngineItf, void*, SsrnxSLuint32, const void*, const void*);
    void *rest[8];
};

typedef void (*opensles_SsrnxSLPlayCallback)(opensles_SsrnxSLPlayItf, void *, SsrnxSLuint32);
struct opensles_SsrnxSLPlayVTable {
    SsrnxSLresult (*SetPlayState)(opensles_SsrnxSLPlayItf, SsrnxSLuint32);
    SsrnxSLresult (*GetPlayState)(opensles_SsrnxSLPlayItf, SsrnxSLuint32*);
    SsrnxSLresult (*GetDuration)(opensles_SsrnxSLPlayItf, SsrnxSLmillisecond*);
    SsrnxSLresult (*GetPosition)(opensles_SsrnxSLPlayItf, SsrnxSLmillisecond*);
    SsrnxSLresult (*RegisterCallback)(opensles_SsrnxSLPlayItf, opensles_SsrnxSLPlayCallback, void *);
    SsrnxSLresult (*SetCallbackEventsMask)(opensles_SsrnxSLPlayItf, SsrnxSLuint32);
    SsrnxSLresult (*GetCallbackEventsMask)(opensles_SsrnxSLPlayItf, SsrnxSLuint32 *);
    SsrnxSLresult (*SetMarkerPosition)(opensles_SsrnxSLPlayItf, SsrnxSLmillisecond);
    SsrnxSLresult (*ClearMarkerPosition)(opensles_SsrnxSLPlayItf);
    SsrnxSLresult (*GetMarkerPosition)(opensles_SsrnxSLPlayItf, SsrnxSLmillisecond *);
    SsrnxSLresult (*SetPositionUpdatePeriod)(opensles_SsrnxSLPlayItf, SsrnxSLmillisecond);
    SsrnxSLresult (*GetPositionUpdatePeriod)(opensles_SsrnxSLPlayItf, SsrnxSLmillisecond *);
};

struct opensles_SsrnxSLSeekVTable {
    SsrnxSLresult (*SetPosition)(opensles_SsrnxSLSeekItf, SsrnxSLmillisecond, SsrnxSLuint32);
    SsrnxSLresult (*SetLoop)(opensles_SsrnxSLSeekItf, SsrnxSLboolean, SsrnxSLmillisecond, SsrnxSLmillisecond);
    SsrnxSLresult (*GetLoop)(opensles_SsrnxSLSeekItf, SsrnxSLboolean*, SsrnxSLmillisecond*, SsrnxSLmillisecond*);
};

typedef struct opensles_SsrnxSLAndroidSimpleBufferQueueState {
    SsrnxSLuint32 count;
    SsrnxSLuint32 index;
} opensles_SsrnxSLAndroidSimpleBufferQueueState;

typedef void (*opensles_SsrnxSLAndroidSimpleBufferQueueCallback)(opensles_SsrnxSLAndroidSimpleBufferQueueItf caller,
                                                         void *context);

struct opensles_SsrnxSLAndroidSimpleBufferQueueVTable {
    SsrnxSLresult (*Enqueue)(opensles_SsrnxSLAndroidSimpleBufferQueueItf, const void *, SsrnxSLuint32);
    SsrnxSLresult (*Clear)(opensles_SsrnxSLAndroidSimpleBufferQueueItf);
    SsrnxSLresult (*GetState)(opensles_SsrnxSLAndroidSimpleBufferQueueItf, opensles_SsrnxSLAndroidSimpleBufferQueueState *);
    SsrnxSLresult (*RegisterCallback)(opensles_SsrnxSLAndroidSimpleBufferQueueItf,
                                      opensles_SsrnxSLAndroidSimpleBufferQueueCallback, void *);
};

/* SLPrefetchStatusItf (OpenSL ES 1.0.1 section 8.23). */
typedef void (*opensles_SsrnxSLPrefetchCallback)(const void *caller, void *context, SsrnxSLuint32 event);
struct opensles_SsrnxSLPrefetchStatusVTable {
    SsrnxSLresult (*GetPrefetchStatus)(const void *self, SsrnxSLuint32 *status);
    SsrnxSLresult (*GetFillLevel)(const void *self, int16_t *level);
    SsrnxSLresult (*RegisterCallback)(const void *self, opensles_SsrnxSLPrefetchCallback callback, void *context);
    SsrnxSLresult (*SetCallbackEventsMask)(const void *self, SsrnxSLuint32 flags);
    SsrnxSLresult (*GetCallbackEventsMask)(const void *self, SsrnxSLuint32 *flags);
    SsrnxSLresult (*SetFillUpdatePeriod)(const void *self, int16_t period);
    SsrnxSLresult (*GetFillUpdatePeriod)(const void *self, int16_t *period);
};

struct opensles_SsrnxSLAndroidConfigurationVTable {
    SsrnxSLresult (*SetConfiguration)(opensles_SsrnxSLAndroidConfigurationItf, const char *, const void *, SsrnxSLuint32);
    SsrnxSLresult (*GetConfiguration)(opensles_SsrnxSLAndroidConfigurationItf, const char *, SsrnxSLuint32 *, void *);
};

typedef enum opensles_SsrnxSLObjectKind {
    SSRNX_SL_ENGINE = 1,
    SSRNX_SL_OUTPUT_MIX = 2,
    SSRNX_SL_PLAYER = 3,
} opensles_SsrnxSLObjectKind;

#define SSRNX_SL_PCM_QUEUE_MAX 32u
#define SSRNX_SL_PCM_OUTPUT_RATE SSRNX_MEDIA_OUTPUT_RATE

typedef struct opensles_SsrnxSLPcmBuffer {
    const uint8_t *data;
    size_t size;
    size_t offset;
} opensles_SsrnxSLPcmBuffer;

struct opensles_SsrnxSLObject {
    opensles_SsrnxSLHandle object_if;
    opensles_SsrnxSLHandle engine_if;
    opensles_SsrnxSLHandle play_if;
    opensles_SsrnxSLHandle seek_if;
    opensles_SsrnxSLHandle simple_queue_if;
    opensles_SsrnxSLHandle android_config_if;
    opensles_SsrnxSLHandle prefetch_if;
    opensles_SsrnxSLObjectKind kind;
    SsrnxSLuint32 state;
    opensles_SsrnxSLPlayCallback play_callback;
    void *play_context;
    SsrnxSLuint32 play_mask;
    SsrnxSLmillisecond play_period;
    bool realized;
    SsrnxMediaPlayer *media_player;

    bool pcm_queue_player;
    uint32_t pcm_channels;
    uint32_t pcm_sample_rate;
    uint32_t pcm_bytes_per_frame;
    uint32_t pcm_bits_per_sample;
    uint32_t pcm_phase; /* fractional source frame, denominator = output rate */
    uint32_t pcm_queue_capacity;
    opensles_SsrnxSLPcmBuffer pcm_queue[SSRNX_SL_PCM_QUEUE_MAX];
    uint32_t pcm_head;
    uint32_t pcm_count;
    uint32_t pcm_index;
    uint64_t pcm_frames_mixed;
    uint64_t pcm_position_frames;
    uint64_t pcm_buffers_enqueued;
    uint64_t pcm_buffers_consumed;
    opensles_SsrnxSLAndroidSimpleBufferQueueCallback pcm_callback;
    void *pcm_callback_context;
    int32_t android_stream_type;
    int32_t android_performance_mode;
    bool android_stream_type_set;
    bool android_performance_mode_set;
    opensles_SsrnxSLObject *pcm_next;
    opensles_SsrnxSLPrefetchCallback prefetch_callback;
    void *prefetch_context;
    SsrnxSLuint32 prefetch_mask;
    int16_t prefetch_period;
    bool prefetch_reported;
};

typedef struct opensles_SsrnxSLIidMarker { uint32_t tag; } opensles_SsrnxSLIidMarker;
/* Standard exported identifier data exists even for interfaces this backend
 * cannot acquire. dlsym returns the address of an SLInterfaceID variable, not
 * an interface vtable. Keep the Khronos UUID layout for these capability IDs. */
typedef struct opensles_SsrnxSLInterfaceId {
    uint32_t time_low;
    uint16_t time_mid, time_hi_and_version, clock_seq;
    uint8_t node[6];
} opensles_SsrnxSLInterfaceId;
static const opensles_SsrnxSLInterfaceId opensles_k_iid_null =
    {0xec7178ec, 0xe5e1, 0x4432, 0xa3f4, {0x46, 0x57, 0xe6, 0x79, 0x52, 0x10}};
static const opensles_SsrnxSLInterfaceId opensles_k_iid_engine_capabilities =
    {0x8320d0a0, 0xddd5, 0x11db, 0xa1b1, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}};
static const opensles_SsrnxSLInterfaceId opensles_k_iid_audio_io_device_capabilities =
    {0xb2564dc0, 0xddd3, 0x11db, 0xbd62, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}};
static const opensles_SsrnxSLInterfaceId opensles_k_iid_output_mix =
    {0x97750f60, 0xddd7, 0x11db, 0x92b1, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}};
static const opensles_SsrnxSLIidMarker opensles_k_iid_engine = {0x454e4701};
static const opensles_SsrnxSLIidMarker opensles_k_iid_play = {0x504c5901};
static const opensles_SsrnxSLIidMarker opensles_k_iid_seek = {0x53454501};
static const opensles_SsrnxSLIidMarker opensles_k_iid_mutesolo = {0x4d555401};
static const opensles_SsrnxSLIidMarker opensles_k_iid_volume = {0x564f4c01};
static const opensles_SsrnxSLIidMarker opensles_k_iid_reverb = {0x52455601};
static const opensles_SsrnxSLIidMarker opensles_k_iid_android_simple_buffer_queue = {0x42514601};
static const opensles_SsrnxSLIidMarker opensles_k_iid_android_configuration = {0x41434601};
static const opensles_SsrnxSLIidMarker opensles_k_iid_record = {0x52454301};
static const opensles_SsrnxSLIidMarker opensles_k_iid_buffer_queue = {0x42554601};
static const opensles_SsrnxSLIidMarker opensles_k_iid_prefetch_status = {0x50524601};
static const opensles_SsrnxSLIidMarker opensles_k_iid_metadata_extraction = {0x4d455401};
const void *const SSRNX_SL_IID_NULL = &opensles_k_iid_null;
const void *SSRNX_SL_IID_ENGINE = &opensles_k_iid_engine;
const void *SSRNX_SL_IID_ENGINECAPABILITIES = &opensles_k_iid_engine_capabilities;
const void *SSRNX_SL_IID_AUDIOIODEVICECAPABILITIES = &opensles_k_iid_audio_io_device_capabilities;
const void *SSRNX_SL_IID_OUTPUTMIX = &opensles_k_iid_output_mix;
const void *SSRNX_SL_IID_PLAY = &opensles_k_iid_play;
const void *SSRNX_SL_IID_SEEK = &opensles_k_iid_seek;
const void *SSRNX_SL_IID_MUTESOLO = &opensles_k_iid_mutesolo;
const void *SSRNX_SL_IID_VOLUME = &opensles_k_iid_volume;
const void *SSRNX_SL_IID_ENVIRONMENTALREVERB = &opensles_k_iid_reverb;
const void *SSRNX_SL_IID_ANDROIDSIMPLEBUFFERQUEUE = &opensles_k_iid_android_simple_buffer_queue;
const void *SSRNX_SL_IID_ANDROIDCONFIGURATION = &opensles_k_iid_android_configuration;
const void *SSRNX_SL_IID_RECORD = &opensles_k_iid_record;
const void *SSRNX_SL_IID_BUFFERQUEUE = &opensles_k_iid_buffer_queue;
const void *SSRNX_SL_IID_PREFETCHSTATUS = &opensles_k_iid_prefetch_status;
const void *SSRNX_SL_IID_METADATAEXTRACTION = &opensles_k_iid_metadata_extraction;

typedef struct opensles_SsrnxSLDataSource {
    void *pLocator;
    void *pFormat;
} opensles_SsrnxSLDataSource;

typedef struct opensles_SsrnxSLDataLocatorAndroidFD {
    uint32_t locatorType;
    int32_t fd;
    int64_t offset;
    int64_t length;
} opensles_SsrnxSLDataLocatorAndroidFD;

typedef struct opensles_SsrnxSLDataFormatMime {
    uint32_t formatType;
    char *mimeType;
    uint32_t containerType;
} opensles_SsrnxSLDataFormatMime;

typedef struct opensles_SsrnxSLDataLocatorAndroidSimpleBufferQueue {
    uint32_t locatorType;
    uint32_t numBuffers;
} opensles_SsrnxSLDataLocatorAndroidSimpleBufferQueue;

typedef struct opensles_SsrnxSLDataFormatPcm {
    uint32_t formatType;
    uint32_t numChannels;
    uint32_t samplesPerSec;
    uint32_t bitsPerSample;
    uint32_t containerSize;
    uint32_t channelMask;
    uint32_t endianness;
} opensles_SsrnxSLDataFormatPcm;

enum {
    SSRNX_SL_DATALOCATOR_ANDROIDFD = 0x800007BCu,
    SSRNX_SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE = 0x800007BDu,
    /* Android accepts the standard buffer-queue locator for audio players and
     * serves it with the Android simple buffer queue (same ABI). */
    SSRNX_SL_DATALOCATOR_BUFFERQUEUE = 0x00000006u,
    SSRNX_SL_DATAFORMAT_MIME = 0x00000001u,
    SSRNX_SL_DATAFORMAT_PCM = 0x00000002u,
};

static uint64_t opensles_g_player_create_count;
static uint64_t opensles_g_pcm_player_create_count;
static opensles_SsrnxSLObject *opensles_g_pcm_players;
static pthread_mutex_t opensles_g_pcm_lock = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local unsigned opensles_g_pcm_lock_depth;

/* OpenSL buffer callbacks commonly enqueue the next buffer synchronously from
 * inside the mixer. Make the registry lock re-entrant per host thread without
 * depending on non-portable recursive-mutex attributes. */
static void opensles_pcm_lock(void) {
    if (opensles_g_pcm_lock_depth++ == 0)
        (void)pthread_mutex_lock(&opensles_g_pcm_lock);
}

static void opensles_pcm_unlock(void) {
    if (!opensles_g_pcm_lock_depth) return;
    if (--opensles_g_pcm_lock_depth == 0)
        (void)pthread_mutex_unlock(&opensles_g_pcm_lock);
}

static opensles_SsrnxSLObject *opensles_owner_from_handle(const void *self) {
    return self ? ((const opensles_SsrnxSLHandle *)self)->owner : NULL;
}

static SsrnxSLresult opensles_object_realize(opensles_SsrnxSLObjectItf self, SsrnxSLboolean async) {
    (void)async;
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (o->kind == SSRNX_SL_PLAYER && o->media_player &&
        !ssrnx_media_player_realize(o->media_player)) {
        return SSRNX_SL_RESULT_CONTENT_UNSUPPORTED;
    }
    o->realized = true;
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_object_resume(opensles_SsrnxSLObjectItf self, SsrnxSLboolean async) { (void)self; (void)async; return SSRNX_SL_RESULT_SUCCESS; }
static SsrnxSLresult opensles_object_get_state(opensles_SsrnxSLObjectItf self, SsrnxSLuint32 *state) {
    opensles_SsrnxSLObject *o=opensles_owner_from_handle(self); if(!o||!state)return SSRNX_SL_RESULT_PARAMETER_INVALID; *state=o->realized?2u:1u; return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_object_get_interface(opensles_SsrnxSLObjectItf self, const void *iid, void *out_interface) {
    if (out_interface) *(void **)out_interface = NULL;
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !iid || !out_interface) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    void *result = NULL;
    if (o->kind == SSRNX_SL_ENGINE && iid == SSRNX_SL_IID_ENGINE) result = &o->engine_if.vtable;
    else if (o->kind == SSRNX_SL_PLAYER && iid == SSRNX_SL_IID_PLAY) result = &o->play_if.vtable;
    else if (o->kind == SSRNX_SL_PLAYER && iid == SSRNX_SL_IID_SEEK && !o->pcm_queue_player) result = &o->seek_if.vtable;
    else if (o->kind == SSRNX_SL_PLAYER && o->pcm_queue_player &&
             (iid == SSRNX_SL_IID_ANDROIDSIMPLEBUFFERQUEUE || iid == SSRNX_SL_IID_BUFFERQUEUE))
        result = &o->simple_queue_if.vtable;
    else if (o->kind == SSRNX_SL_PLAYER && !o->pcm_queue_player && o->media_player &&
             iid == SSRNX_SL_IID_PREFETCHSTATUS)
        result = &o->prefetch_if.vtable;
    else if (o->kind == SSRNX_SL_PLAYER && o->pcm_queue_player && iid == SSRNX_SL_IID_ANDROIDCONFIGURATION)
        result = &o->android_config_if.vtable;
    else if (o->kind == SSRNX_SL_PLAYER && (iid == SSRNX_SL_IID_MUTESOLO || iid == SSRNX_SL_IID_VOLUME)) {
        /* These optional interfaces may be requested at player creation even when the app never acquires them directly. */
        return SSRNX_SL_RESULT_FEATURE_UNSUPPORTED;
    } else if (o->kind == SSRNX_SL_OUTPUT_MIX && iid == SSRNX_SL_IID_ENVIRONMENTALREVERB) {
        return SSRNX_SL_RESULT_FEATURE_UNSUPPORTED;
    }
    if (!result) return SSRNX_SL_RESULT_FEATURE_UNSUPPORTED;
    *(void **)out_interface = result;
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_object_register_callback(opensles_SsrnxSLObjectItf self, void *cb, void *ctx) { (void)self;(void)cb;(void)ctx;return SSRNX_SL_RESULT_FEATURE_UNSUPPORTED; }
static void opensles_object_abort(opensles_SsrnxSLObjectItf self) { (void)self; }
static void opensles_object_destroy(opensles_SsrnxSLObjectItf self) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o) return;
    if (o->pcm_queue_player) {
        opensles_pcm_lock();
        opensles_SsrnxSLObject **link = &opensles_g_pcm_players;
        while (*link) {
            if (*link == o) { *link = o->pcm_next; break; }
            link = &(*link)->pcm_next;
        }
        SSRNX_LOGCI("OPENSL", "PCM queue player destroy frames=%llu enqueued=%llu consumed=%llu",
                    (unsigned long long)o->pcm_frames_mixed,
                    (unsigned long long)o->pcm_buffers_enqueued,
                    (unsigned long long)o->pcm_buffers_consumed);
        opensles_pcm_unlock();
    }
    if (o->media_player) {
        ssrnx_media_player_destroy(o->media_player);
        o->media_player = NULL;
    }
    free(o);
}
static SsrnxSLresult opensles_object_set_priority(opensles_SsrnxSLObjectItf s, SsrnxSLint32 p, SsrnxSLboolean b){(void)s;(void)p;(void)b;return SSRNX_SL_RESULT_FEATURE_UNSUPPORTED;}
static SsrnxSLresult opensles_object_get_priority(opensles_SsrnxSLObjectItf s, SsrnxSLint32*p, SsrnxSLboolean*b){(void)s;(void)p;(void)b;return SSRNX_SL_RESULT_FEATURE_UNSUPPORTED;}
static SsrnxSLresult opensles_object_loss(opensles_SsrnxSLObjectItf s,int16_t n,void*i,SsrnxSLboolean e){(void)s;(void)n;(void)i;(void)e;return SSRNX_SL_RESULT_FEATURE_UNSUPPORTED;}

static const struct opensles_SsrnxSLObjectVTable opensles_k_object_vtable = {
    opensles_object_realize, opensles_object_resume, opensles_object_get_state, opensles_object_get_interface,
    opensles_object_register_callback, opensles_object_abort, opensles_object_destroy,
    opensles_object_set_priority, opensles_object_get_priority, opensles_object_loss
};

static SsrnxSLresult opensles_unsupported_device(void *self, ...) { (void)self; return SSRNX_SL_RESULT_FEATURE_UNSUPPORTED; }

/* URI/FD players decode their whole source during Realize, so prefetch is
 * complete from then on: SUFFICIENTDATA at fill level 1000 permille. The
 * status/fill events Android raises while prefetching are delivered once, when
 * playback first leaves STOPPED, to a registered callback whose mask
 * selects them. */
enum {
    SSRNX_SL_PREFETCHSTATUS_UNDERFLOW = 1u,
    SSRNX_SL_PREFETCHSTATUS_SUFFICIENTDATA = 2u,
    SSRNX_SL_PREFETCHEVENT_STATUSCHANGE = 1u,
    SSRNX_SL_PREFETCHEVENT_FILLLEVELCHANGE = 2u,
};
static SsrnxSLresult opensles_prefetch_get_status(const void *self, SsrnxSLuint32 *status) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !status) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    *status = o->realized ? SSRNX_SL_PREFETCHSTATUS_SUFFICIENTDATA : SSRNX_SL_PREFETCHSTATUS_UNDERFLOW;
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_prefetch_get_fill_level(const void *self, int16_t *level) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !level) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    *level = o->realized ? 1000 : 0;
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_prefetch_register_callback(const void *self, opensles_SsrnxSLPrefetchCallback cb, void *ctx) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    o->prefetch_callback = cb;
    o->prefetch_context = ctx;
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_prefetch_set_mask(const void *self, SsrnxSLuint32 flags) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    o->prefetch_mask = flags;
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_prefetch_get_mask(const void *self, SsrnxSLuint32 *flags) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !flags) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    *flags = o->prefetch_mask;
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_prefetch_set_period(const void *self, int16_t period) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || period <= 0 || period > 1000) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    o->prefetch_period = period;
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_prefetch_get_period(const void *self, int16_t *period) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !period) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    *period = o->prefetch_period ? o->prefetch_period : 100;
    return SSRNX_SL_RESULT_SUCCESS;
}
static const struct opensles_SsrnxSLPrefetchStatusVTable opensles_k_prefetch_vtable = {
    opensles_prefetch_get_status, opensles_prefetch_get_fill_level, opensles_prefetch_register_callback,
    opensles_prefetch_set_mask, opensles_prefetch_get_mask, opensles_prefetch_set_period, opensles_prefetch_get_period
};

static void opensles_prefetch_report(opensles_SsrnxSLObject *o) {
    if (o->prefetch_reported || !o->prefetch_callback) return;
    const SsrnxSLuint32 events = o->prefetch_mask & (SSRNX_SL_PREFETCHEVENT_STATUSCHANGE |
                                                     SSRNX_SL_PREFETCHEVENT_FILLLEVELCHANGE);
    if (!events) return;
    o->prefetch_reported = true;
    o->prefetch_callback(&o->prefetch_if.vtable, o->prefetch_context, events);
}

static SsrnxSLresult opensles_play_set_state(opensles_SsrnxSLPlayItf self, SsrnxSLuint32 state) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (state < SSRNX_SL_PLAYSTATE_STOPPED || state > SSRNX_SL_PLAYSTATE_PLAYING)
        return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (o->pcm_queue_player) {
        opensles_pcm_lock();
        o->state = state;
        if (state == SSRNX_SL_PLAYSTATE_STOPPED) {
            o->pcm_position_frames = 0;
            o->pcm_phase = 0;
        }
        SSRNX_LOGCI("OPENSL", "PCM queue play state=%u queued=%u", state, o->pcm_count);
        opensles_pcm_unlock();
        return SSRNX_SL_RESULT_SUCCESS;
    }
    if (!o->media_player) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (!ssrnx_media_player_set_state(o->media_player, (SsrnxMediaState)state))
        return SSRNX_SL_RESULT_CONTENT_UNSUPPORTED;
    o->state = state;
    if (state != SSRNX_SL_PLAYSTATE_STOPPED && o->realized) opensles_prefetch_report(o);
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_play_get_state(opensles_SsrnxSLPlayItf self, SsrnxSLuint32 *state) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !state) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (o->pcm_queue_player) {
        opensles_pcm_lock();
        *state = o->state;
        opensles_pcm_unlock();
        return SSRNX_SL_RESULT_SUCCESS;
    }
    if (!o->media_player) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    o->state = (SsrnxSLuint32)ssrnx_media_player_get_state(o->media_player);
    *state = o->state;
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_play_get_duration(opensles_SsrnxSLPlayItf self, SsrnxSLmillisecond *ms) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !ms) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (o->pcm_queue_player) { *ms = UINT32_MAX; return SSRNX_SL_RESULT_SUCCESS; }
    if (!o->media_player) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    *ms = ssrnx_media_player_get_duration_ms(o->media_player);
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_play_get_position(opensles_SsrnxSLPlayItf self, SsrnxSLmillisecond *ms) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !ms) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (o->pcm_queue_player) {
        opensles_pcm_lock();
        *ms = o->pcm_sample_rate ? (SsrnxSLmillisecond)(o->pcm_position_frames * 1000u / o->pcm_sample_rate) : 0;
        opensles_pcm_unlock();
        return SSRNX_SL_RESULT_SUCCESS;
    }
    if (!o->media_player) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    *ms = ssrnx_media_player_get_position_ms(o->media_player);
    return SSRNX_SL_RESULT_SUCCESS;
}
enum { SSRNX_SL_PLAYEVENT_HEADATEND = 1u, SSRNX_SL_PLAYEVENT_HEADATNEWPOS = 4u };

static SsrnxSLresult opensles_play_register_callback(opensles_SsrnxSLPlayItf self, opensles_SsrnxSLPlayCallback cb, void *context) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (!o->pcm_queue_player) return SSRNX_SL_RESULT_FEATURE_UNSUPPORTED;
    opensles_pcm_lock();
    o->play_callback = cb;
    o->play_context = context;
    opensles_pcm_unlock();
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_play_set_mask(opensles_SsrnxSLPlayItf self, SsrnxSLuint32 mask) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (mask & ~31u) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (!o->pcm_queue_player || (mask & ~(SSRNX_SL_PLAYEVENT_HEADATEND | SSRNX_SL_PLAYEVENT_HEADATNEWPOS)))
        return SSRNX_SL_RESULT_FEATURE_UNSUPPORTED;
    opensles_pcm_lock(); o->play_mask = mask; opensles_pcm_unlock();
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_play_get_mask(opensles_SsrnxSLPlayItf self, SsrnxSLuint32 *mask) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !mask) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    opensles_pcm_lock(); *mask = o->play_mask; opensles_pcm_unlock();
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_play_set_period(opensles_SsrnxSLPlayItf self, SsrnxSLmillisecond period) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !period) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (!o->pcm_queue_player) return SSRNX_SL_RESULT_FEATURE_UNSUPPORTED;
    opensles_pcm_lock(); o->play_period = period; opensles_pcm_unlock();
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_play_get_period(opensles_SsrnxSLPlayItf self, SsrnxSLmillisecond *period) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !period) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    opensles_pcm_lock(); *period = o->play_period; opensles_pcm_unlock();
    return SSRNX_SL_RESULT_SUCCESS;
}
static SsrnxSLresult opensles_play_set_marker(opensles_SsrnxSLPlayItf self, SsrnxSLmillisecond marker) {
    (void)marker;
    return self ? SSRNX_SL_RESULT_FEATURE_UNSUPPORTED : SSRNX_SL_RESULT_PARAMETER_INVALID;
}
static SsrnxSLresult opensles_play_clear_marker(opensles_SsrnxSLPlayItf self) {
    return self ? SSRNX_SL_RESULT_FEATURE_UNSUPPORTED : SSRNX_SL_RESULT_PARAMETER_INVALID;
}
static SsrnxSLresult opensles_play_get_marker(opensles_SsrnxSLPlayItf self, SsrnxSLmillisecond *marker) {
    return self && marker ? SSRNX_SL_RESULT_FEATURE_UNSUPPORTED : SSRNX_SL_RESULT_PARAMETER_INVALID;
}
static const struct opensles_SsrnxSLPlayVTable opensles_k_play_vtable = {
    opensles_play_set_state, opensles_play_get_state, opensles_play_get_duration, opensles_play_get_position,
    opensles_play_register_callback, opensles_play_set_mask, opensles_play_get_mask,
    opensles_play_set_marker, opensles_play_clear_marker, opensles_play_get_marker,
    opensles_play_set_period, opensles_play_get_period
};

static SsrnxSLresult opensles_seek_set_position(opensles_SsrnxSLSeekItf self, SsrnxSLmillisecond pos, SsrnxSLuint32 mode) {
    (void)mode;
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !o->media_player) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    return ssrnx_media_player_seek_ms(o->media_player, pos) ? SSRNX_SL_RESULT_SUCCESS : SSRNX_SL_RESULT_FEATURE_UNSUPPORTED;
}
static SsrnxSLresult opensles_seek_set_loop(opensles_SsrnxSLSeekItf self, SsrnxSLboolean enable, SsrnxSLmillisecond start, SsrnxSLmillisecond end) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !o->media_player) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    return ssrnx_media_player_set_loop(o->media_player, enable != 0, start, end) ? SSRNX_SL_RESULT_SUCCESS : SSRNX_SL_RESULT_FEATURE_UNSUPPORTED;
}
static SsrnxSLresult opensles_seek_get_loop(opensles_SsrnxSLSeekItf self, SsrnxSLboolean *enable, SsrnxSLmillisecond *start, SsrnxSLmillisecond *end) {
    (void)self;
    if (enable) *enable = 0;
    if (start) *start = 0;
    if (end) *end = 0;
    return SSRNX_SL_RESULT_SUCCESS;
}
static const struct opensles_SsrnxSLSeekVTable opensles_k_seek_vtable = {opensles_seek_set_position,opensles_seek_set_loop,opensles_seek_get_loop};

static SsrnxSLresult opensles_simple_queue_enqueue(opensles_SsrnxSLAndroidSimpleBufferQueueItf self,
                                           const void *buffer, SsrnxSLuint32 size) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !o->pcm_queue_player || !buffer || size == 0)
        return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (!o->pcm_bytes_per_frame || (size % o->pcm_bytes_per_frame) != 0)
        return SSRNX_SL_RESULT_PARAMETER_INVALID;
    opensles_pcm_lock();
    if (o->pcm_count >= o->pcm_queue_capacity || o->pcm_count >= SSRNX_SL_PCM_QUEUE_MAX) {
        opensles_pcm_unlock();
        return SSRNX_SL_RESULT_BUFFER_INSUFFICIENT;
    }
    const uint32_t tail = (o->pcm_head + o->pcm_count) % SSRNX_SL_PCM_QUEUE_MAX;
    o->pcm_queue[tail].data = (const uint8_t *)buffer;
    o->pcm_queue[tail].size = size;
    o->pcm_queue[tail].offset = 0;
    o->pcm_count++;
    o->pcm_buffers_enqueued++;
    if (o->pcm_buffers_enqueued <= 4u || (o->pcm_buffers_enqueued % 128u) == 0u) {
        SSRNX_LOGCI("OPENSL", "PCM queue enqueue buffers=%u bytes=%u total=%llu",
                    o->pcm_count, size, (unsigned long long)o->pcm_buffers_enqueued);
    }
    opensles_pcm_unlock();
    return SSRNX_SL_RESULT_SUCCESS;
}

static SsrnxSLresult opensles_simple_queue_clear(opensles_SsrnxSLAndroidSimpleBufferQueueItf self) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !o->pcm_queue_player) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    opensles_pcm_lock();
    memset(o->pcm_queue, 0, sizeof(o->pcm_queue));
    o->pcm_head = 0;
    o->pcm_count = 0;
    o->pcm_index = 0;
    o->pcm_phase = 0;
    opensles_pcm_unlock();
    return SSRNX_SL_RESULT_SUCCESS;
}

static SsrnxSLresult opensles_simple_queue_get_state(opensles_SsrnxSLAndroidSimpleBufferQueueItf self,
                                             opensles_SsrnxSLAndroidSimpleBufferQueueState *state) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !o->pcm_queue_player || !state) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    opensles_pcm_lock();
    state->count = o->pcm_count;
    state->index = o->pcm_index;
    opensles_pcm_unlock();
    return SSRNX_SL_RESULT_SUCCESS;
}

static SsrnxSLresult opensles_simple_queue_register_callback(opensles_SsrnxSLAndroidSimpleBufferQueueItf self,
                                                     opensles_SsrnxSLAndroidSimpleBufferQueueCallback callback,
                                                     void *context) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !o->pcm_queue_player) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    opensles_pcm_lock();
    if (o->state != SSRNX_SL_PLAYSTATE_STOPPED) {
        opensles_pcm_unlock();
        return SSRNX_SL_RESULT_PRECONDITIONS_VIOLATED;
    }
    o->pcm_callback = callback;
    o->pcm_callback_context = context;
    SSRNX_LOGCI("OPENSL", "PCM queue callback=%s", callback ? "registered" : "cleared");
    opensles_pcm_unlock();
    return SSRNX_SL_RESULT_SUCCESS;
}

static const struct opensles_SsrnxSLAndroidSimpleBufferQueueVTable opensles_k_simple_queue_vtable = {
    opensles_simple_queue_enqueue, opensles_simple_queue_clear, opensles_simple_queue_get_state, opensles_simple_queue_register_callback
};

static SsrnxSLresult opensles_android_config_set(opensles_SsrnxSLAndroidConfigurationItf self, const char *key,
                                         const void *value, SsrnxSLuint32 value_size) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !o->pcm_queue_player || !key || !value || value_size < sizeof(int32_t))
        return SSRNX_SL_RESULT_PARAMETER_INVALID;
    int32_t v = 0;
    memcpy(&v, value, sizeof(v));
    if (strcmp(key, "androidPlaybackStreamType") == 0) {
        o->android_stream_type = v;
        o->android_stream_type_set = true;
    } else if (strcmp(key, "androidPerformanceMode") == 0) {
        o->android_performance_mode = v;
        o->android_performance_mode_set = true;
    } else {
        SSRNX_LOGCI("OPENSL", "AndroidConfiguration ignored key=%s bytes=%u", key, value_size);
        return SSRNX_SL_RESULT_SUCCESS;
    }
    SSRNX_LOGCI("OPENSL", "AndroidConfiguration key=%s value=%d", key, v);
    return SSRNX_SL_RESULT_SUCCESS;
}

static SsrnxSLresult opensles_android_config_get(opensles_SsrnxSLAndroidConfigurationItf self, const char *key,
                                         SsrnxSLuint32 *value_size, void *value) {
    opensles_SsrnxSLObject *o = opensles_owner_from_handle(self);
    if (!o || !o->pcm_queue_player || !key || !value_size) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    int32_t v = 0;
    bool found = false;
    if (strcmp(key, "androidPlaybackStreamType") == 0 && o->android_stream_type_set) {
        v = o->android_stream_type; found = true;
    } else if (strcmp(key, "androidPerformanceMode") == 0 && o->android_performance_mode_set) {
        v = o->android_performance_mode; found = true;
    }
    if (!found) return SSRNX_SL_RESULT_FEATURE_UNSUPPORTED;
    const SsrnxSLuint32 need = (SsrnxSLuint32)sizeof(v);
    if (!value) { *value_size = need; return SSRNX_SL_RESULT_SUCCESS; }
    if (*value_size < need) { *value_size = need; return SSRNX_SL_RESULT_BUFFER_INSUFFICIENT; }
    memcpy(value, &v, need);
    *value_size = need;
    return SSRNX_SL_RESULT_SUCCESS;
}

static const struct opensles_SsrnxSLAndroidConfigurationVTable opensles_k_android_config_vtable = {
    opensles_android_config_set, opensles_android_config_get
};

static bool opensles_player_interface_supported(const void *iid) {
    return iid == SSRNX_SL_IID_ANDROIDSIMPLEBUFFERQUEUE ||
           iid == SSRNX_SL_IID_BUFFERQUEUE ||
           iid == SSRNX_SL_IID_ANDROIDCONFIGURATION ||
           iid == SSRNX_SL_IID_PLAY;
}

static bool opensles_required_interfaces_supported(SsrnxSLuint32 count, const void *ids, const void *required) {
    if (count == 0) return true;
    if (!ids) return false;
    const void * const *iid_list = (const void * const *)ids;
    const SsrnxSLboolean *required_list = (const SsrnxSLboolean *)required;
    for (SsrnxSLuint32 i = 0; i < count; ++i) {
        const bool is_required = required_list ? required_list[i] != 0 : false;
        if (is_required && !opensles_player_interface_supported(iid_list[i])) return false;
    }
    return true;
}

static opensles_SsrnxSLObject *opensles_object_alloc(opensles_SsrnxSLObjectKind kind);

/* Android's documented buffer-queue PCM family, independent of the host
 * output rate. Higher rates and extended/float PCM remain unsupported. */
static SsrnxSLresult opensles_pcm_format_check(const opensles_SsrnxSLDataFormatPcm *fmt) {
    if (!fmt->numChannels || !fmt->samplesPerSec)
        return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (fmt->numChannels > 2u) return SSRNX_SL_RESULT_CONTENT_UNSUPPORTED;
    switch (fmt->samplesPerSec) {
        case 8000000u: case 11025000u: case 12000000u: case 16000000u:
        case 22050000u: case 24000000u: case 32000000u: case 44100000u:
        case 48000000u: break;
        default: return SSRNX_SL_RESULT_CONTENT_UNSUPPORTED;
    }
    switch (fmt->bitsPerSample) {
        case 8u: case 16u: break;
        case 20u: case 24u: case 28u: case 32u:
            return SSRNX_SL_RESULT_CONTENT_UNSUPPORTED;
        default: return SSRNX_SL_RESULT_PARAMETER_INVALID;
    }
    if (fmt->containerSize < fmt->bitsPerSample)
        return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (fmt->containerSize != fmt->bitsPerSample)
        return SSRNX_SL_RESULT_CONTENT_UNSUPPORTED;
    const uint32_t mask = fmt->channelMask;
    if (mask && (fmt->numChannels == 2u ? mask != 3u :
                 (mask != 1u && mask != 2u && mask != 4u)))
        return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (fmt->endianness != 1u && fmt->endianness != 2u)
        return SSRNX_SL_RESULT_PARAMETER_INVALID;
    if (fmt->endianness != 2u) return SSRNX_SL_RESULT_CONTENT_UNSUPPORTED;
    return SSRNX_SL_RESULT_SUCCESS;
}

static SsrnxSLresult opensles_engine_create_player(opensles_SsrnxSLEngineItf self, void *out_player,
                                           void *src, void *sink, SsrnxSLuint32 n,
                                           const void *ids, const void *required) {
    (void)self; (void)sink;
    if (!out_player) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    *(void **)out_player = NULL;
    if (!src || (n && (!ids || !required))) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    const opensles_SsrnxSLDataSource *source = (const opensles_SsrnxSLDataSource *)src;
    if (!source->pLocator || !source->pFormat) return SSRNX_SL_RESULT_PARAMETER_INVALID;
    const uint32_t locator_type = *(const uint32_t *)source->pLocator;
    const uint32_t format_type = *(const uint32_t *)source->pFormat;

    if (locator_type == SSRNX_SL_DATALOCATOR_ANDROIDFD && format_type == SSRNX_SL_DATAFORMAT_MIME) {
        const opensles_SsrnxSLDataLocatorAndroidFD *loc = (const opensles_SsrnxSLDataLocatorAndroidFD *)source->pLocator;
        const opensles_SsrnxSLDataFormatMime *fmt = (const opensles_SsrnxSLDataFormatMime *)source->pFormat;
        opensles_SsrnxSLObject *o = opensles_object_alloc(SSRNX_SL_PLAYER);
        if (!o) return SSRNX_SL_RESULT_MEMORY_FAILURE;
        o->media_player = ssrnx_media_player_create_android_fd(loc->fd, loc->offset, loc->length,
                                                                fmt->containerType, fmt->mimeType);
        if (!o->media_player) {
            free(o);
            return SSRNX_SL_RESULT_CONTENT_UNSUPPORTED;
        }
        o->state = SSRNX_SL_PLAYSTATE_STOPPED;
        *(void **)out_player = &o->object_if.vtable;
        opensles_g_player_create_count++;
        SSRNX_LOGCI("OPENSL", "CreateAudioPlayer #%llu AndroidFD fd=%d offset=%lld length=%lld container=0x%x",
                    (unsigned long long)opensles_g_player_create_count, loc->fd,
                    (long long)loc->offset, (long long)loc->length, fmt->containerType);
        return SSRNX_SL_RESULT_SUCCESS;
    }

    if ((locator_type == SSRNX_SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE ||
         locator_type == SSRNX_SL_DATALOCATOR_BUFFERQUEUE) &&
        format_type == SSRNX_SL_DATAFORMAT_PCM) {
        const opensles_SsrnxSLDataLocatorAndroidSimpleBufferQueue *loc =
            (const opensles_SsrnxSLDataLocatorAndroidSimpleBufferQueue *)source->pLocator;
        const opensles_SsrnxSLDataFormatPcm *fmt = (const opensles_SsrnxSLDataFormatPcm *)source->pFormat;
        const uint32_t rate = fmt->samplesPerSec / 1000u;
        if (loc->numBuffers == 0 || loc->numBuffers > 255u)
            return SSRNX_SL_RESULT_PARAMETER_INVALID;
        if (loc->numBuffers > SSRNX_SL_PCM_QUEUE_MAX)
            return SSRNX_SL_RESULT_CONTENT_UNSUPPORTED;
        const SsrnxSLresult format_result = opensles_pcm_format_check(fmt);
        if (format_result != SSRNX_SL_RESULT_SUCCESS) {
            SSRNX_LOGCW("OPENSL", "PCM queue unsupported buffers=%u channels=%u rateMilliHz=%u bits=%u container=%u",
                        loc->numBuffers, fmt->numChannels, fmt->samplesPerSec,
                        fmt->bitsPerSample, fmt->containerSize);
            return format_result;
        }
        if (!opensles_required_interfaces_supported(n, ids, required))
            return SSRNX_SL_RESULT_FEATURE_UNSUPPORTED;

        opensles_SsrnxSLObject *o = opensles_object_alloc(SSRNX_SL_PLAYER);
        if (!o) return SSRNX_SL_RESULT_MEMORY_FAILURE;
        o->pcm_queue_player = true;
        o->pcm_channels = fmt->numChannels;
        o->pcm_sample_rate = rate;
        o->pcm_bits_per_sample = fmt->bitsPerSample;
        o->pcm_bytes_per_frame = fmt->numChannels * (fmt->bitsPerSample / 8u);
        o->pcm_queue_capacity = loc->numBuffers;
        o->state = SSRNX_SL_PLAYSTATE_STOPPED;
        opensles_pcm_lock();
        o->pcm_next = opensles_g_pcm_players;
        opensles_g_pcm_players = o;
        opensles_pcm_unlock();
        *(void **)out_player = &o->object_if.vtable;
        opensles_g_player_create_count++;
        opensles_g_pcm_player_create_count++;
        SSRNX_LOGCI("OPENSL", "CreateAudioPlayer #%llu AndroidSimpleBufferQueue PCM channels=%u rate=%u buffers=%u",
                    (unsigned long long)opensles_g_player_create_count, o->pcm_channels,
                    o->pcm_sample_rate, o->pcm_queue_capacity);
        return SSRNX_SL_RESULT_SUCCESS;
    }

    SSRNX_LOGCW("OPENSL", "CreateAudioPlayer unsupported locator=0x%x format=0x%x",
                locator_type, format_type);
    return SSRNX_SL_RESULT_CONTENT_UNSUPPORTED;
}
static SsrnxSLresult opensles_engine_create_mix(opensles_SsrnxSLEngineItf self, void *out_mix,
                                        SsrnxSLuint32 n, const void *ids, const void *required) {
    (void)self;(void)n;(void)ids;(void)required;
    if(!out_mix)return SSRNX_SL_RESULT_PARAMETER_INVALID;
    opensles_SsrnxSLObject *o=opensles_object_alloc(SSRNX_SL_OUTPUT_MIX); if(!o)return SSRNX_SL_RESULT_MEMORY_FAILURE;
    *(void **)out_mix=&o->object_if.vtable;
    return SSRNX_SL_RESULT_SUCCESS;
}

static const struct opensles_SsrnxSLEngineVTable opensles_k_engine_vtable = {
    (void *)opensles_unsupported_device,
    (void *)opensles_unsupported_device,
    opensles_engine_create_player,
    (void *)opensles_unsupported_device,
    (void *)opensles_unsupported_device,
    (void *)opensles_unsupported_device,
    (void *)opensles_unsupported_device,
    opensles_engine_create_mix,
    {0}
};

static opensles_SsrnxSLObject *opensles_object_alloc(opensles_SsrnxSLObjectKind kind) {
    opensles_SsrnxSLObject *o=(opensles_SsrnxSLObject*)calloc(1,sizeof(*o)); if(!o)return NULL;
    o->kind=kind;
    o->object_if.vtable=&opensles_k_object_vtable; o->object_if.owner=o;
    o->engine_if.vtable=&opensles_k_engine_vtable; o->engine_if.owner=o;
    o->play_if.vtable=&opensles_k_play_vtable; o->play_if.owner=o;
    o->seek_if.vtable=&opensles_k_seek_vtable; o->seek_if.owner=o;
    o->simple_queue_if.vtable=&opensles_k_simple_queue_vtable; o->simple_queue_if.owner=o;
    o->android_config_if.vtable=&opensles_k_android_config_vtable; o->android_config_if.owner=o;
    o->prefetch_if.vtable=&opensles_k_prefetch_vtable; o->prefetch_if.owner=o;
    o->state=SSRNX_SL_PLAYSTATE_STOPPED;
    o->play_period = 1000;
    return o;
}

SsrnxSLresult slCreateEngine(void *out_engine_object,
                             SsrnxSLuint32 num_options,
                             const void *options,
                             SsrnxSLuint32 num_interfaces,
                             const void *interface_ids,
                             const void *interface_required) {
    (void)num_options;(void)options;(void)num_interfaces;(void)interface_ids;(void)interface_required;
    if(!out_engine_object)return SSRNX_SL_RESULT_PARAMETER_INVALID;
    opensles_SsrnxSLObject *o=opensles_object_alloc(SSRNX_SL_ENGINE); if(!o)return SSRNX_SL_RESULT_MEMORY_FAILURE;
    *(void **)out_engine_object=&o->object_if.vtable;
    SSRNX_LOGCI("OPENSL", "slCreateEngine -> %p", *(void **)out_engine_object);
    return SSRNX_SL_RESULT_SUCCESS;
}

static int16_t opensles_mix_s16_sample(int16_t base, int16_t add) {
    int32_t v = (int32_t)base + (int32_t)add;
    if (v > INT16_MAX) v = INT16_MAX;
    if (v < INT16_MIN) v = INT16_MIN;
    return (int16_t)v;
}

static int16_t opensles_pcm_sample(const uint8_t *data, uint32_t bits) {
    if (bits == 8u) return (int16_t)(((int32_t)data[0] - 128) * 256);
    return (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

static bool opensles_pcm_player_mix_frame(opensles_SsrnxSLObject *o, int16_t *left, int16_t *right) {
    if (!o || !o->pcm_queue_player || o->state != SSRNX_SL_PLAYSTATE_PLAYING ||
        !left || !right || o->pcm_count == 0) return false;

    opensles_SsrnxSLPcmBuffer *b = &o->pcm_queue[o->pcm_head];
    if (!b->data || b->offset + o->pcm_bytes_per_frame > b->size) return false;
    const uint8_t *a = b->data + b->offset;
    const uint8_t *next = a;
    if (b->offset + 2u * o->pcm_bytes_per_frame <= b->size)
        next += o->pcm_bytes_per_frame;
    else if (o->pcm_count > 1u)
        next = o->pcm_queue[(o->pcm_head + 1u) % SSRNX_SL_PCM_QUEUE_MAX].data;
    const uint32_t stride = o->pcm_bits_per_sample / 8u;
    const int32_t l = opensles_pcm_sample(a, o->pcm_bits_per_sample);
    const int32_t r = o->pcm_channels == 2u ? opensles_pcm_sample(a + stride, o->pcm_bits_per_sample) : l;
    const int32_t nl = opensles_pcm_sample(next, o->pcm_bits_per_sample);
    const int32_t nr = o->pcm_channels == 2u ? opensles_pcm_sample(next + stride, o->pcm_bits_per_sample) : nl;
    /* Linear interpolation, as in the existing AAudio mixer. Integer phase
     * preserves exact source duration across host calls and queue boundaries.
     * No guest buffer is retained after its completion callback. */
    *left = (int16_t)(l + (int64_t)(nl - l) * o->pcm_phase / SSRNX_SL_PCM_OUTPUT_RATE);
    *right = (int16_t)(r + (int64_t)(nr - r) * o->pcm_phase / SSRNX_SL_PCM_OUTPUT_RATE);
    o->pcm_frames_mixed++;
    o->pcm_phase += o->pcm_sample_rate;
    if (o->pcm_phase < SSRNX_SL_PCM_OUTPUT_RATE) return true;
    o->pcm_phase -= SSRNX_SL_PCM_OUTPUT_RATE;
    b->offset += o->pcm_bytes_per_frame;
    const uint64_t previous_ms = o->pcm_position_frames * 1000u / o->pcm_sample_rate;
    o->pcm_position_frames++;
    const uint64_t current_ms = o->pcm_position_frames * 1000u / o->pcm_sample_rate;
    SsrnxSLuint32 play_events = o->play_period && previous_ms / o->play_period != current_ms / o->play_period
        ? SSRNX_SL_PLAYEVENT_HEADATNEWPOS : 0;

    if (b->offset >= b->size) {
        memset(b, 0, sizeof(*b));
        o->pcm_head = (o->pcm_head + 1u) % SSRNX_SL_PCM_QUEUE_MAX;
        o->pcm_count--;
        o->pcm_index++;
        o->pcm_buffers_consumed++;
        opensles_SsrnxSLAndroidSimpleBufferQueueCallback cb = o->pcm_callback;
        void *ctx = o->pcm_callback_context;
        if (o->pcm_buffers_consumed <= 4u || (o->pcm_buffers_consumed % 128u) == 0u) {
            SSRNX_LOGCI("OPENSL", "PCM queue consumed queued=%u index=%u total=%llu",
                        o->pcm_count, o->pcm_index,
                        (unsigned long long)o->pcm_buffers_consumed);
        }
        if (cb) cb((opensles_SsrnxSLAndroidSimpleBufferQueueItf)&o->simple_queue_if.vtable, ctx);
        if (!o->pcm_count) play_events |= SSRNX_SL_PLAYEVENT_HEADATEND;
    }
    play_events &= o->play_mask;
    if (play_events && o->play_callback)
        o->play_callback((opensles_SsrnxSLPlayItf)&o->play_if.vtable, o->play_context, play_events);
    return true;
}

size_t ssrnx_opensles_mix_s16(int16_t *io_interleaved, size_t frames) {
    if (!io_interleaved || frames == 0) return 0;
    opensles_pcm_lock();
    size_t active_frames = 0;
    for (size_t i = 0; i < frames; ++i) {
        bool active = false;
        for (opensles_SsrnxSLObject *o = opensles_g_pcm_players; o; ) {
            opensles_SsrnxSLObject *next = o->pcm_next;
            int16_t left = 0, right = 0;
            if (opensles_pcm_player_mix_frame(o, &left, &right)) {
                io_interleaved[i * 2u] = opensles_mix_s16_sample(io_interleaved[i * 2u], left);
                io_interleaved[i * 2u + 1u] = opensles_mix_s16_sample(io_interleaved[i * 2u + 1u], right);
                active = true;
            }
            o = next;
        }
        if (active) active_frames++;
    }
    opensles_pcm_unlock();
    return active_frames;
}

void ssrnx_opensles_reset(void) {
    /* Guest OpenSL objects live inside the current Android runtime. Never carry
     * their host registry or guest callback pointers across a package reset. */
    opensles_pcm_lock();
    opensles_g_pcm_players = NULL;
    opensles_g_player_create_count = 0;
    opensles_g_pcm_player_create_count = 0;
    opensles_pcm_unlock();
}
uint64_t ssrnx_opensles_player_create_count(void) { return opensles_g_player_create_count; }

void ssrnx_opensles_get_pcm_runtime_stats(SsrnxSLPcmRuntimeStats *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    opensles_pcm_lock();
    for (opensles_SsrnxSLObject *o = opensles_g_pcm_players; o; o = o->pcm_next) {
        out->players++;
        out->queued_buffers += o->pcm_count;
        out->frames_mixed += o->pcm_frames_mixed;
        out->buffers_enqueued += o->pcm_buffers_enqueued;
        out->buffers_consumed += o->pcm_buffers_consumed;
    }
    opensles_pcm_unlock();
}

#pragma pop_macro("SSRNX_SL_PCM_QUEUE_MAX")
#pragma pop_macro("SSRNX_SL_PCM_OUTPUT_RATE")


/* ---- media_ndk ---- */
#pragma push_macro("SSRNX_MEDIA_IO_BUFFER")


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>


typedef enum media_ndk_SsrnxMediaValueType {
    SSRNX_MEDIA_VALUE_INT32 = 1,
    SSRNX_MEDIA_VALUE_INT64 = 2,
    SSRNX_MEDIA_VALUE_FLOAT = 3,
    SSRNX_MEDIA_VALUE_STRING = 4,
    SSRNX_MEDIA_VALUE_BUFFER = 5,
} media_ndk_SsrnxMediaValueType;

typedef struct media_ndk_SsrnxMediaEntry {
    char *name;
    media_ndk_SsrnxMediaValueType type;
    union {
        int32_t i32;
        int64_t i64;
        float f;
        char *s;
        struct { void *data; size_t size; } buffer;
    } value;
    struct media_ndk_SsrnxMediaEntry *next;
} media_ndk_SsrnxMediaEntry;

struct SsrnxAMediaFormat {
    media_ndk_SsrnxMediaEntry *entries;
    char *debug;
};

const char *ssrnx_AMEDIAFORMAT_KEY_BIT_RATE = "bitrate";
const char *ssrnx_AMEDIAFORMAT_KEY_I_FRAME_INTERVAL = "i-frame-interval";
const char *ssrnx_AMEDIAFORMAT_KEY_CHANNEL_COUNT = "channel-count";
const char *ssrnx_AMEDIAFORMAT_KEY_COLOR_FORMAT = "color-format";
const char *ssrnx_AMEDIAFORMAT_KEY_COLOR_RANGE = "color-range";
const char *ssrnx_AMEDIAFORMAT_KEY_COLOR_STANDARD = "color-standard";
const char *ssrnx_AMEDIAFORMAT_KEY_DURATION = "durationUs";
const char *ssrnx_AMEDIAFORMAT_KEY_ENCODER_DELAY = "encoder-delay";
const char *ssrnx_AMEDIAFORMAT_KEY_FRAME_RATE = "frame-rate";
const char *ssrnx_AMEDIAFORMAT_KEY_HEIGHT = "height";
const char *ssrnx_AMEDIAFORMAT_KEY_LANGUAGE = "language";
const char *ssrnx_AMEDIAFORMAT_KEY_MIME = "mime";
const char *ssrnx_AMEDIAFORMAT_KEY_ROTATION = "rotation-degrees";
const char *ssrnx_AMEDIAFORMAT_KEY_SAMPLE_RATE = "sample-rate";
const char *ssrnx_AMEDIAFORMAT_KEY_SLICE_HEIGHT = "slice-height";
const char *ssrnx_AMEDIAFORMAT_KEY_STRIDE = "stride";
const char *ssrnx_AMEDIAFORMAT_KEY_WIDTH = "width";

static media_ndk_SsrnxMediaEntry *media_ndk_find_entry(SsrnxAMediaFormat *format, const char *name) {
    if (!format || !name) return NULL;
    for (media_ndk_SsrnxMediaEntry *it = format->entries; it; it = it->next)
        if (strcmp(it->name, name) == 0) return it;
    return NULL;
}

static void media_ndk_clear_value(media_ndk_SsrnxMediaEntry *entry) {
    if (entry && entry->type == SSRNX_MEDIA_VALUE_STRING) free(entry->value.s);
    if (entry && entry->type == SSRNX_MEDIA_VALUE_BUFFER) free(entry->value.buffer.data);
}

static media_ndk_SsrnxMediaEntry *media_ndk_ensure_entry(SsrnxAMediaFormat *format, const char *name,
                                     media_ndk_SsrnxMediaValueType type) {
    if (!format || !name) return NULL;
    media_ndk_SsrnxMediaEntry *entry = media_ndk_find_entry(format, name);
    if (!entry) {
        entry = (media_ndk_SsrnxMediaEntry *)calloc(1, sizeof(*entry));
        if (!entry) return NULL;
        entry->name = strdup(name);
        if (!entry->name) { free(entry); return NULL; }
        entry->next = format->entries;
        format->entries = entry;
    } else {
        media_ndk_clear_value(entry);
        memset(&entry->value, 0, sizeof(entry->value));
    }
    entry->type = type;
    return entry;
}

SsrnxAMediaFormat *ssrnx_AMediaFormat_new(void) {
    return (SsrnxAMediaFormat *)calloc(1, sizeof(SsrnxAMediaFormat));
}

int ssrnx_AMediaFormat_delete(SsrnxAMediaFormat *format) {
    if (!format) return 0;
    media_ndk_SsrnxMediaEntry *entry = format->entries;
    while (entry) {
        media_ndk_SsrnxMediaEntry *next = entry->next;
        media_ndk_clear_value(entry);
        free(entry->name);
        free(entry);
        entry = next;
    }
    free(format->debug);
    free(format);
    return 0; /* AMEDIA_OK */
}

bool ssrnx_AMediaFormat_getInt32(SsrnxAMediaFormat *format, const char *name, int32_t *out) {
    media_ndk_SsrnxMediaEntry *entry = media_ndk_find_entry(format, name);
    if (!entry || entry->type != SSRNX_MEDIA_VALUE_INT32 || !out) return false;
    *out = entry->value.i32;
    return true;
}

bool ssrnx_AMediaFormat_getInt64(SsrnxAMediaFormat *format, const char *name, int64_t *out) {
    media_ndk_SsrnxMediaEntry *entry = media_ndk_find_entry(format, name);
    if (!entry || entry->type != SSRNX_MEDIA_VALUE_INT64 || !out) return false;
    *out = entry->value.i64;
    return true;
}

bool ssrnx_AMediaFormat_getFloat(SsrnxAMediaFormat *format, const char *name, float *out) {
    media_ndk_SsrnxMediaEntry *entry = media_ndk_find_entry(format, name);
    if (!entry || entry->type != SSRNX_MEDIA_VALUE_FLOAT || !out) return false;
    *out = entry->value.f;
    return true;
}

bool ssrnx_AMediaFormat_getString(SsrnxAMediaFormat *format, const char *name, const char **out) {
    if (!format || !out) return false;
    media_ndk_SsrnxMediaEntry *entry = media_ndk_find_entry(format, name);
    if (!entry || entry->type != SSRNX_MEDIA_VALUE_STRING) return false;
    *out = entry->value.s ? entry->value.s : "";
    return true;
}

void ssrnx_AMediaFormat_setInt32(SsrnxAMediaFormat *format, const char *name, int32_t value) {
    media_ndk_SsrnxMediaEntry *entry = media_ndk_ensure_entry(format, name, SSRNX_MEDIA_VALUE_INT32);
    if (entry) entry->value.i32 = value;
}

void ssrnx_AMediaFormat_setInt64(SsrnxAMediaFormat *format, const char *name, int64_t value) {
    media_ndk_SsrnxMediaEntry *entry = media_ndk_ensure_entry(format, name, SSRNX_MEDIA_VALUE_INT64);
    if (entry) entry->value.i64 = value;
}

void ssrnx_AMediaFormat_setFloat(SsrnxAMediaFormat *format, const char *name, float value) {
    media_ndk_SsrnxMediaEntry *entry = media_ndk_ensure_entry(format, name, SSRNX_MEDIA_VALUE_FLOAT);
    if (entry) entry->value.f = value;
}

void ssrnx_AMediaFormat_setString(SsrnxAMediaFormat *format, const char *name, const char *value) {
    if (!format || !name || !value) return;
    /* getString may supply the old value: copy before replacing its entry. */
    char *copy = strdup(value);
    if (!copy) return;
    media_ndk_SsrnxMediaEntry *entry = media_ndk_ensure_entry(format, name, SSRNX_MEDIA_VALUE_STRING);
    if (!entry) { free(copy); return; }
    entry->value.s = copy;
}

bool ssrnx_AMediaFormat_getBuffer(SsrnxAMediaFormat *format, const char *name, void **data, size_t *size) {
    media_ndk_SsrnxMediaEntry *entry = media_ndk_find_entry(format, name);
    if (!entry || entry->type != SSRNX_MEDIA_VALUE_BUFFER || !data || !size) return false;
    *data = entry->value.buffer.data;
    *size = entry->value.buffer.size;
    return true;
}

void ssrnx_AMediaFormat_setBuffer(SsrnxAMediaFormat *format, const char *name, const void *data, size_t size) {
    if (!format || !name || (!data && size)) return;
    void *copy = malloc(size ? size : 1);
    if (!copy) return;
    if (size) memcpy(copy, data, size);
    media_ndk_SsrnxMediaEntry *entry = media_ndk_ensure_entry(format, name, SSRNX_MEDIA_VALUE_BUFFER);
    if (!entry) { free(copy); return; }
    entry->value.buffer.data = copy;
    entry->value.buffer.size = size;
}

static int media_ndk_media_entry_text(char *out, size_t cap, const media_ndk_SsrnxMediaEntry *entry) {
    switch (entry->type) {
    case SSRNX_MEDIA_VALUE_INT32: return snprintf(out, cap, "%s: int32(%" PRId32 ")", entry->name, entry->value.i32);
    case SSRNX_MEDIA_VALUE_INT64: return snprintf(out, cap, "%s: int64(%" PRId64 ")", entry->name, entry->value.i64);
    case SSRNX_MEDIA_VALUE_FLOAT: return snprintf(out, cap, "%s: float(%f)", entry->name, entry->value.f);
    case SSRNX_MEDIA_VALUE_STRING: return snprintf(out, cap, "%s: string(%s)", entry->name, entry->value.s);
    case SSRNX_MEDIA_VALUE_BUFFER: return snprintf(out, cap, "%s: data", entry->name);
    default: return snprintf(out, cap, "%s: unknown", entry->name);
    }
}

const char *ssrnx_AMediaFormat_toString(SsrnxAMediaFormat *format) {
    if (!format) return NULL;
    size_t size = 3;
    for (media_ndk_SsrnxMediaEntry *e = format->entries; e; e = e->next) {
        int n = media_ndk_media_entry_text(NULL, 0, e);
        if (n < 0 || size > SIZE_MAX - 2 || (size_t)n > SIZE_MAX - size - 2) return NULL;
        size += (size_t)n + 2;
    }
    char *text = malloc(size);
    if (!text) return NULL;
    size_t pos = 0;
    text[pos++] = '{';
    for (media_ndk_SsrnxMediaEntry *e = format->entries; e; e = e->next) {
        if (pos > 1) { text[pos++] = ','; text[pos++] = ' '; }
        pos += (size_t)media_ndk_media_entry_text(text + pos, size - pos, e);
    }
    text[pos++] = '}'; text[pos] = 0;
    free(format->debug);
    format->debug = text;
    return text;
}

SsrnxAMediaCodec *ssrnx_AMediaCodec_createEncoderByType(const char *mime_type) {
    SSRNX_LOGCW("MEDIA", "encoder unavailable mime=%s", mime_type ? mime_type : "<null>");
    return NULL;
}

/* -------------------------------------------------------------------------
 * Minimal real NDK media backend.
 *
 * SSRNX currently has a real PCM/WAV path but no H.264/AAC/VPx decoder. The
 * NDK surface below therefore implements WAV extraction and an audio/raw
 * pass-through decoder exactly enough to be useful, while unsupported codecs
 * fail with normal NDK error/null results. This intentionally avoids fake
 * success for video or compressed codecs SSRNX cannot decode yet.
 * ------------------------------------------------------------------------- */


#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#define SSRNX_MEDIA_IO_BUFFER (64u * 1024u)

typedef struct media_ndk_SsrnxWavInfo {
    uint16_t format_tag;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    int64_t data_offset;
    int64_t data_length;
} media_ndk_SsrnxWavInfo;

struct SsrnxAMediaDataSource {
    void *userdata;
    SsrnxAMediaDataSourceReadAt read_at;
    SsrnxAMediaDataSourceGetSize get_size;
    SsrnxAMediaDataSourceClose close_cb;
};

struct SsrnxAMediaExtractor {
    int fd;
    bool owns_fd;
    int64_t source_offset;
    int64_t source_length;
    SsrnxAMediaDataSource *custom_source;
    media_ndk_SsrnxWavInfo wav;
    bool source_ready;
    bool track_selected;
    int64_t sample_pos;
};

struct SsrnxAMediaCodec {
    bool configured;
    bool started;
    bool input_dequeued;
    bool output_ready;
    bool format_change_pending;
    int32_t sample_rate;
    int32_t channel_count;
    uint8_t input[SSRNX_MEDIA_IO_BUFFER];
    uint8_t output[SSRNX_MEDIA_IO_BUFFER];
    size_t output_size;
    int64_t output_pts;
    uint32_t output_flags;
};

static uint16_t media_ndk_rd16le(const uint8_t *p) {
    return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8);
}
static uint32_t media_ndk_rd32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static ssize_t media_ndk_read_at_fd(int fd, int64_t offset, void *dst, size_t bytes) {
    if (fd < 0 || offset < 0 || (!dst && bytes)) { errno = EINVAL; return -1; }
    off_t saved = lseek(fd, 0, SEEK_CUR);
    if (saved < 0) return -1;
    if (lseek(fd, (off_t)offset, SEEK_SET) < 0) return -1;
    ssize_t rc = read(fd, dst, bytes);
    int saved_errno = errno;
    (void)lseek(fd, saved, SEEK_SET);
    errno = saved_errno;
    return rc;
}

typedef ssize_t (*media_ndk_SsrnxMediaReadAt)(void *ctx, int64_t offset, void *dst, size_t bytes);

static ssize_t media_ndk_fd_reader(void *ctx, int64_t offset, void *dst, size_t bytes) {
    return media_ndk_read_at_fd((int)(intptr_t)ctx, offset, dst, bytes);
}

static ssize_t media_ndk_custom_reader(void *ctx, int64_t offset, void *dst, size_t bytes) {
    SsrnxAMediaDataSource *source = (SsrnxAMediaDataSource *)ctx;
    if (!source || !source->read_at || !source->userdata || offset < 0) return -1;
    return source->read_at(source->userdata, offset, dst, bytes);
}

static bool media_ndk_parse_wav_reader(media_ndk_SsrnxMediaReadAt reader, void *ctx, int64_t base, int64_t length, media_ndk_SsrnxWavInfo *out) {
    if (!out || !reader || base < 0 || length < 12) return false;
    uint8_t header[12];
    if (reader(ctx, base, header, sizeof(header)) != (ssize_t)sizeof(header)) return false;
    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) return false;

    bool have_fmt = false, have_data = false;
    media_ndk_SsrnxWavInfo info;
    memset(&info, 0, sizeof(info));
    int64_t pos = 12;
    while (pos + 8 <= length) {
        uint8_t chunk[8];
        if (reader(ctx, base + pos, chunk, sizeof(chunk)) != (ssize_t)sizeof(chunk)) return false;
        const uint32_t size = media_ndk_rd32le(chunk + 4);
        const int64_t payload = pos + 8;
        if ((uint64_t)size > (uint64_t)(length - payload)) return false;
        if (memcmp(chunk, "fmt ", 4) == 0) {
            uint8_t fmt[40];
            const size_t want = size < sizeof(fmt) ? size : sizeof(fmt);
            if (want < 16 || reader(ctx, base + payload, fmt, want) != (ssize_t)want) return false;
            info.format_tag = media_ndk_rd16le(fmt + 0);
            info.channels = media_ndk_rd16le(fmt + 2);
            info.sample_rate = media_ndk_rd32le(fmt + 4);
            info.byte_rate = media_ndk_rd32le(fmt + 8);
            info.block_align = media_ndk_rd16le(fmt + 12);
            info.bits_per_sample = media_ndk_rd16le(fmt + 14);
            have_fmt = true;
        } else if (memcmp(chunk, "data", 4) == 0) {
            info.data_offset = payload;
            info.data_length = size;
            have_data = true;
        }
        pos = payload + (int64_t)size + (size & 1u);
        if (have_fmt && have_data) break;
    }
    if (!have_fmt || !have_data || info.format_tag != 1 || info.channels == 0 ||
        info.sample_rate == 0 || info.block_align == 0 || info.bits_per_sample == 0) return false;
    if (info.data_offset < 0 || info.data_length < 0 || info.data_offset + info.data_length > length) return false;
    *out = info;
    return true;
}

static bool media_ndk_parse_wav(int fd, int64_t base, int64_t length, media_ndk_SsrnxWavInfo *out) {
    return media_ndk_parse_wav_reader(media_ndk_fd_reader, (void *)(intptr_t)fd, base, length, out);
}

static void media_ndk_extractor_clear(SsrnxAMediaExtractor *ex) {
    if (!ex) return;
    if (ex->owns_fd && ex->fd >= 0) close(ex->fd);
    ex->fd = -1;
    ex->owns_fd = false;
    ex->source_offset = ex->source_length = 0;
    ex->custom_source = NULL;
    memset(&ex->wav, 0, sizeof(ex->wav));
    ex->source_ready = false;
    ex->track_selected = false;
    ex->sample_pos = 0;
}

SsrnxAMediaDataSource *ssrnx_AMediaDataSource_new(void) {
    return (SsrnxAMediaDataSource *)calloc(1, sizeof(SsrnxAMediaDataSource));
}

void ssrnx_AMediaDataSource_delete(SsrnxAMediaDataSource *source) {
    free(source);
}

void ssrnx_AMediaDataSource_setUserdata(SsrnxAMediaDataSource *source, void *userdata) {
    if (source) source->userdata = userdata;
}

void ssrnx_AMediaDataSource_setReadAt(SsrnxAMediaDataSource *source, SsrnxAMediaDataSourceReadAt read_at) {
    if (source) source->read_at = read_at;
}

void ssrnx_AMediaDataSource_setGetSize(SsrnxAMediaDataSource *source, SsrnxAMediaDataSourceGetSize get_size) {
    if (source) source->get_size = get_size;
}

void ssrnx_AMediaDataSource_setClose(SsrnxAMediaDataSource *source, SsrnxAMediaDataSourceClose close_cb) {
    if (source) source->close_cb = close_cb;
}

SsrnxAMediaExtractor *ssrnx_AMediaExtractor_new(void) {
    SsrnxAMediaExtractor *ex = (SsrnxAMediaExtractor *)calloc(1, sizeof(*ex));
    if (ex) ex->fd = -1;
    return ex;
}

int ssrnx_AMediaExtractor_delete(SsrnxAMediaExtractor *ex) {
    if (!ex) return SSRNX_AMEDIA_OK;
    media_ndk_extractor_clear(ex);
    free(ex);
    return SSRNX_AMEDIA_OK;
}

int ssrnx_AMediaExtractor_setDataSourceFd(SsrnxAMediaExtractor *ex, int fd, int64_t offset, int64_t length) {
    if (!ex || fd < 0 || offset < 0 || length < 0) return SSRNX_AMEDIA_ERROR_INVALID_PARAMETER;
    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size < 0 || offset > (int64_t)st.st_size)
        return SSRNX_AMEDIA_ERROR_IO;
    int64_t available = (int64_t)st.st_size - offset;
    if (length > available) length = available;
#ifdef F_DUPFD
    int owned = fcntl(fd, F_DUPFD, 0);
#else
    int owned = -1;
    errno = ENOTSUP;
#endif
    if (owned < 0) return SSRNX_AMEDIA_ERROR_IO;
    media_ndk_SsrnxWavInfo wav;
    if (!media_ndk_parse_wav(owned, offset, length, &wav)) {
        close(owned);
        return SSRNX_AMEDIA_ERROR_UNSUPPORTED;
    }
    media_ndk_extractor_clear(ex);
    ex->fd = owned;
    ex->owns_fd = true;
    ex->source_offset = offset;
    ex->source_length = length;
    ex->wav = wav;
    ex->source_ready = true;
    return SSRNX_AMEDIA_OK;
}

int ssrnx_AMediaExtractor_setDataSource(SsrnxAMediaExtractor *ex, const char *location) {
    if (!ex || !location) return SSRNX_AMEDIA_ERROR_INVALID_PARAMETER;
    const char *path = strncmp(location, "file://", 7) == 0 ? location + 7 : location;
    int fd = ssrnx_compat_open_guest_readonly(path);
    if (fd < 0) return SSRNX_AMEDIA_ERROR_IO;
    struct stat st;
    int status = SSRNX_AMEDIA_ERROR_IO;
    if (fstat(fd, &st) == 0 && st.st_size >= 0)
        status = ssrnx_AMediaExtractor_setDataSourceFd(ex, fd, 0, (int64_t)st.st_size);
    close(fd);
    return status;
}

int ssrnx_AMediaExtractor_setDataSourceCustom(SsrnxAMediaExtractor *ex, SsrnxAMediaDataSource *source) {
    if (!ex || !source || !source->userdata || !source->read_at || !source->get_size)
        return SSRNX_AMEDIA_ERROR_INVALID_PARAMETER;
    const ssize_t size = source->get_size(source->userdata);
    if (size < 12) return size < 0 ? SSRNX_AMEDIA_ERROR_UNSUPPORTED : SSRNX_AMEDIA_ERROR_MALFORMED;
    media_ndk_SsrnxWavInfo wav;
    if (!media_ndk_parse_wav_reader(media_ndk_custom_reader, source, 0, (int64_t)size, &wav))
        return SSRNX_AMEDIA_ERROR_UNSUPPORTED;
    media_ndk_extractor_clear(ex);
    ex->custom_source = source;
    ex->source_offset = 0;
    ex->source_length = (int64_t)size;
    ex->wav = wav;
    ex->source_ready = true;
    return SSRNX_AMEDIA_OK;
}

size_t ssrnx_AMediaExtractor_getTrackCount(SsrnxAMediaExtractor *ex) {
    return ex && ex->source_ready ? 1u : 0u;
}

SsrnxAMediaFormat *ssrnx_AMediaExtractor_getTrackFormat(SsrnxAMediaExtractor *ex, size_t idx) {
    if (!ex || !ex->source_ready || idx != 0) return NULL;
    SsrnxAMediaFormat *fmt = ssrnx_AMediaFormat_new();
    if (!fmt) return NULL;
    ssrnx_AMediaFormat_setString(fmt, "mime", "audio/raw");
    ssrnx_AMediaFormat_setInt32(fmt, "sample-rate", (int32_t)ex->wav.sample_rate);
    ssrnx_AMediaFormat_setInt32(fmt, "channel-count", (int32_t)ex->wav.channels);
    if (ex->wav.byte_rate) {
        int64_t duration = (ex->wav.data_length * INT64_C(1000000)) / ex->wav.byte_rate;
        ssrnx_AMediaFormat_setInt64(fmt, "durationUs", duration);
    }
    return fmt;
}

int ssrnx_AMediaExtractor_selectTrack(SsrnxAMediaExtractor *ex, size_t idx) {
    if (!ex || !ex->source_ready) return SSRNX_AMEDIA_ERROR_INVALID_OBJECT;
    if (idx != 0) return SSRNX_AMEDIA_ERROR_INVALID_PARAMETER;
    ex->track_selected = true;
    return SSRNX_AMEDIA_OK;
}

static size_t media_ndk_extractor_current_size(const SsrnxAMediaExtractor *ex) {
    if (!ex || !ex->source_ready || !ex->track_selected || ex->sample_pos >= ex->wav.data_length) return 0;
    int64_t remain = ex->wav.data_length - ex->sample_pos;
    size_t amount = remain > 32768 ? 32768u : (size_t)remain;
    if (ex->wav.block_align > 1 && amount > ex->wav.block_align) {
        amount -= amount % ex->wav.block_align;
    }
    return amount;
}

ssize_t ssrnx_AMediaExtractor_readSampleData(SsrnxAMediaExtractor *ex, uint8_t *buffer, size_t capacity) {
    if (!ex || !buffer || !capacity) return -1;
    size_t sample = media_ndk_extractor_current_size(ex);
    if (!sample) return -1;
    if (sample > capacity) sample = capacity - (capacity % ex->wav.block_align);
    if (!sample) return 0;
    const int64_t offset = ex->source_offset + ex->wav.data_offset + ex->sample_pos;
    ssize_t rc = ex->custom_source
        ? media_ndk_custom_reader(ex->custom_source, offset, buffer, sample)
        : media_ndk_read_at_fd(ex->fd, offset, buffer, sample);
    return rc == (ssize_t)sample ? rc : -1;
}

int ssrnx_AMediaExtractor_getSampleTrackIndex(SsrnxAMediaExtractor *ex) {
    return media_ndk_extractor_current_size(ex) ? 0 : -1;
}

int64_t ssrnx_AMediaExtractor_getSampleTime(SsrnxAMediaExtractor *ex) {
    if (!media_ndk_extractor_current_size(ex) || !ex->wav.byte_rate) return -1;
    return (ex->sample_pos * INT64_C(1000000)) / ex->wav.byte_rate;
}

bool ssrnx_AMediaExtractor_advance(SsrnxAMediaExtractor *ex) {
    size_t amount = media_ndk_extractor_current_size(ex);
    if (!amount) return false;
    ex->sample_pos += (int64_t)amount;
    return media_ndk_extractor_current_size(ex) != 0;
}

int ssrnx_AMediaExtractor_seekTo(SsrnxAMediaExtractor *ex, int64_t seek_pos_us, int mode) {
    if (!ex || !ex->source_ready) return SSRNX_AMEDIA_ERROR_INVALID_OBJECT;
    if (mode < 0 || mode > 2) return SSRNX_AMEDIA_ERROR_INVALID_PARAMETER;
    if (seek_pos_us < 0) seek_pos_us = 0;
    uint64_t frame = ((uint64_t)seek_pos_us * ex->wav.sample_rate) / UINT64_C(1000000);
    uint64_t byte = frame * ex->wav.block_align;
    if (byte > (uint64_t)ex->wav.data_length) byte = (uint64_t)ex->wav.data_length;
    ex->sample_pos = (int64_t)byte;
    return SSRNX_AMEDIA_OK;
}

SsrnxAMediaCodec *ssrnx_AMediaCodec_createDecoderByType(const char *mime_type) {
    if (!mime_type || strcmp(mime_type, "audio/raw") != 0) return NULL;
    return (SsrnxAMediaCodec *)calloc(1, sizeof(SsrnxAMediaCodec));
}

int ssrnx_AMediaCodec_delete(SsrnxAMediaCodec *codec) {
    free(codec);
    return SSRNX_AMEDIA_OK;
}

int ssrnx_AMediaCodec_configure(SsrnxAMediaCodec *codec, const SsrnxAMediaFormat *format,
                                void *surface, void *crypto, uint32_t flags) {
    if (!codec || !format) return SSRNX_AMEDIA_ERROR_INVALID_PARAMETER;
    if (surface || crypto || (flags & SSRNX_AMEDIACODEC_CONFIGURE_FLAG_ENCODE))
        return SSRNX_AMEDIA_ERROR_UNSUPPORTED;
    int32_t rate = 0, channels = 0;
    const char *mime = NULL;
    if (!ssrnx_AMediaFormat_getString((SsrnxAMediaFormat *)format, "mime", &mime) ||
        !mime || strcmp(mime, "audio/raw") != 0 ||
        !ssrnx_AMediaFormat_getInt32((SsrnxAMediaFormat *)format, "sample-rate", &rate) ||
        !ssrnx_AMediaFormat_getInt32((SsrnxAMediaFormat *)format, "channel-count", &channels) ||
        rate <= 0 || channels <= 0)
        return SSRNX_AMEDIA_ERROR_UNSUPPORTED;
    codec->sample_rate = rate;
    codec->channel_count = channels;
    codec->configured = true;
    return SSRNX_AMEDIA_OK;
}

int ssrnx_AMediaCodec_start(SsrnxAMediaCodec *codec) {
    if (!codec || !codec->configured) return SSRNX_AMEDIA_ERROR_INVALID_OPERATION;
    codec->started = true;
    codec->input_dequeued = false;
    codec->output_ready = false;
    codec->format_change_pending = true;
    return SSRNX_AMEDIA_OK;
}

int ssrnx_AMediaCodec_stop(SsrnxAMediaCodec *codec) {
    if (!codec || !codec->started) return SSRNX_AMEDIA_ERROR_INVALID_OPERATION;
    codec->started = false;
    codec->input_dequeued = false;
    codec->output_ready = false;
    return SSRNX_AMEDIA_OK;
}

int ssrnx_AMediaCodec_flush(SsrnxAMediaCodec *codec) {
    if (!codec || !codec->started) return SSRNX_AMEDIA_ERROR_INVALID_OPERATION;
    codec->input_dequeued = false;
    codec->output_ready = false;
    codec->format_change_pending = true;
    codec->output_size = 0;
    return SSRNX_AMEDIA_OK;
}

ssize_t ssrnx_AMediaCodec_dequeueInputBuffer(SsrnxAMediaCodec *codec, int64_t timeout_us) {
    (void)timeout_us;
    if (!codec || !codec->started) return SSRNX_AMEDIACODEC_INFO_TRY_AGAIN_LATER;
    if (codec->input_dequeued || codec->output_ready) return SSRNX_AMEDIACODEC_INFO_TRY_AGAIN_LATER;
    codec->input_dequeued = true;
    return 0;
}

uint8_t *ssrnx_AMediaCodec_getInputBuffer(SsrnxAMediaCodec *codec, size_t idx, size_t *out_size) {
    if (!codec || !codec->started || !codec->input_dequeued || idx != 0) return NULL;
    if (out_size) *out_size = sizeof(codec->input);
    return codec->input;
}

int ssrnx_AMediaCodec_queueInputBuffer(SsrnxAMediaCodec *codec, size_t idx, off_t offset,
                                       size_t size, uint64_t time_us, uint32_t flags) {
    if (!codec || !codec->started || !codec->input_dequeued || idx != 0 || offset < 0 ||
        (uint64_t)offset + size > sizeof(codec->input))
        return SSRNX_AMEDIA_ERROR_INVALID_PARAMETER;
    memcpy(codec->output, codec->input + offset, size);
    codec->output_size = size;
    codec->output_pts = (int64_t)time_us;
    codec->output_flags = flags;
    codec->output_ready = true;
    codec->input_dequeued = false;
    return SSRNX_AMEDIA_OK;
}

ssize_t ssrnx_AMediaCodec_dequeueOutputBuffer(SsrnxAMediaCodec *codec,
                                               SsrnxAMediaCodecBufferInfo *info,
                                               int64_t timeout_us) {
    (void)timeout_us;
    if (!codec || !codec->started) return SSRNX_AMEDIACODEC_INFO_TRY_AGAIN_LATER;
    if (codec->format_change_pending) {
        codec->format_change_pending = false;
        return SSRNX_AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED;
    }
    if (!codec->output_ready) return SSRNX_AMEDIACODEC_INFO_TRY_AGAIN_LATER;
    if (info) {
        info->offset = 0;
        info->size = (int32_t)codec->output_size;
        info->presentationTimeUs = codec->output_pts;
        info->flags = codec->output_flags;
    }
    return 0;
}

SsrnxAMediaFormat *ssrnx_AMediaCodec_getOutputFormat(SsrnxAMediaCodec *codec) {
    if (!codec || !codec->configured) return NULL;
    SsrnxAMediaFormat *fmt = ssrnx_AMediaFormat_new();
    if (!fmt) return NULL;
    ssrnx_AMediaFormat_setString(fmt, "mime", "audio/raw");
    ssrnx_AMediaFormat_setInt32(fmt, "sample-rate", codec->sample_rate);
    ssrnx_AMediaFormat_setInt32(fmt, "channel-count", codec->channel_count);
    return fmt;
}

uint8_t *ssrnx_AMediaCodec_getOutputBuffer(SsrnxAMediaCodec *codec, size_t idx, size_t *out_size) {
    if (!codec || !codec->started || !codec->output_ready || idx != 0) return NULL;
    if (out_size) *out_size = codec->output_size;
    return codec->output;
}

int ssrnx_AMediaCodec_releaseOutputBuffer(SsrnxAMediaCodec *codec, size_t idx, bool render) {
    if (!codec || !codec->started || !codec->output_ready || idx != 0)
        return SSRNX_AMEDIA_ERROR_INVALID_PARAMETER;
    if (render) return SSRNX_AMEDIA_ERROR_UNSUPPORTED;
    codec->output_ready = false;
    codec->output_size = 0;
    return SSRNX_AMEDIA_OK;
}

int ssrnx_AMediaCodec_setOutputSurface(SsrnxAMediaCodec *codec, void *surface) {
    if (!codec) return SSRNX_AMEDIA_ERROR_INVALID_PARAMETER;
    if (!codec->configured) return SSRNX_AMEDIA_ERROR_INVALID_OPERATION;
    /* SSRNX's current codec backend is audio/raw only and is never configured
       with a rendering surface. Advertising success here would falsely expose
       a video decoder path that does not exist. */
    (void)surface;
    return SSRNX_AMEDIA_ERROR_UNSUPPORTED;
}

/* AImage objects are produced only by AImageReader_acquire*Image(). SSRNX's
   codec backend has no surface/image output, so it does not export
   AImageReader and no guest AImage handle can be valid. The accessors keep the
   NDK contract: missing arguments are INVALID_PARAMETER, and any image handle
   is an invalid object. */
static int media_ndk_aimage_accessor(const SsrnxAImage *image, const void *out) {
    if (!image || !out) return SSRNX_AMEDIA_ERROR_INVALID_PARAMETER;
    return SSRNX_AMEDIA_ERROR_INVALID_OBJECT;
}

int ssrnx_AImage_getWidth(const SsrnxAImage *image, int32_t *width) {
    return media_ndk_aimage_accessor(image, width);
}

int ssrnx_AImage_getHeight(const SsrnxAImage *image, int32_t *height) {
    return media_ndk_aimage_accessor(image, height);
}

int ssrnx_AImage_getFormat(const SsrnxAImage *image, int32_t *format) {
    return media_ndk_aimage_accessor(image, format);
}

int ssrnx_AImage_getTimestamp(const SsrnxAImage *image, int64_t *timestamp_ns) {
    return media_ndk_aimage_accessor(image, timestamp_ns);
}

int ssrnx_AImage_getNumberOfPlanes(const SsrnxAImage *image, int32_t *num_planes) {
    return media_ndk_aimage_accessor(image, num_planes);
}

int ssrnx_AImage_getCropRect(const SsrnxAImage *image, SsrnxAImageCropRect *rect) {
    return media_ndk_aimage_accessor(image, rect);
}

#pragma pop_macro("SSRNX_MEDIA_IO_BUFFER")


/* ---- media ---- */
#pragma push_macro("SSRNX_MEDIA_FD_CLIP_MAX_BYTES")
#pragma push_macro("SSRNX_SL_CONTAINERTYPE_MP3")
#pragma push_macro("SSRNX_SL_CONTAINERTYPE_UNSPECIFIED")





#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SSRNX_SL_CONTAINERTYPE_UNSPECIFIED 0x00000001u
#define SSRNX_SL_CONTAINERTYPE_MP3 0x00000009u

#if defined(__GNUC__)
extern size_t ssrnx_opensles_mix_s16(int16_t *io_interleaved, size_t frames) __attribute__((weak));
extern size_t ssrnx_aaudio_mix_s16(int16_t *io_interleaved, size_t frames) __attribute__((weak));
#else
extern size_t ssrnx_opensles_mix_s16(int16_t *io_interleaved, size_t frames);
#endif

struct SsrnxMediaPlayer {
    SsrnxMediaPlayer *next;
    SsrnxMediaSource source;
    SsrnxMp3Decoder *mp3;
    SsrnxStereoRing ring;
    SsrnxMediaClip *clip;
    SsrnxMediaCodec codec;
    SsrnxMediaState state;
    uint32_t container_type;
    char mime_type[64];
    uint32_t source_rate;
    double phase;
    double clip_pos;
    float gain_left;
    float gain_right;
    float playback_rate;
    bool eof;
    bool realized;
    bool failed;
    bool loop;
    uint32_t loop_start_ms;
    uint32_t loop_end_ms;
    uint64_t output_frames_played;
};

static pthread_mutex_t media_g_media_lock = PTHREAD_MUTEX_INITIALIZER;
static SsrnxMediaPlayer *media_g_players;
static SsrnxMediaStats media_g_stats;
static bool media_g_initialized;

static float media_clamp_unit(float x) {
    if (x > 1.0f) return 1.0f;
    if (x < -1.0f) return -1.0f;
    return x;
}

static bool media_codec_is_mp3(uint32_t container_type, const char *mime_type) {
    if (container_type == SSRNX_SL_CONTAINERTYPE_MP3) return true;
    if (mime_type && mime_type[0]) {
        return strcmp(mime_type, "audio/mpeg") == 0 ||
               strcmp(mime_type, "audio/mp3") == 0 ||
               strcmp(mime_type, "audio/mpeg3") == 0;
    }
    return container_type == SSRNX_SL_CONTAINERTYPE_UNSPECIFIED;
}

static void media_player_mark_finished(SsrnxMediaPlayer *p) {
    if (!p || p->state != SSRNX_MEDIA_PLAYING) return;
    p->state = SSRNX_MEDIA_STOPPED;
    media_g_stats.players_finished++;
    if (media_g_stats.active_players) media_g_stats.active_players--;
    SSRNX_LOGCI("MEDIA", "player=%p EOF -> STOPPED", (void *)p);
}

static bool media_player_reset_decoder(SsrnxMediaPlayer *p) {
    if (!p) return false;
    p->phase = 0.0;
    p->clip_pos = 0.0;
    p->eof = false;
    p->output_frames_played = 0;
    p->failed = false;
    if (p->clip) return true;
    if (!p->mp3 || !ssrnx_mp3_decoder_reset(p->mp3)) return false;
    ssrnx_stereo_ring_reset(&p->ring);
    return true;
}

static bool media_player_decode_more(SsrnxMediaPlayer *p) {
    if (!p || !p->realized || p->failed || p->clip) return p && p->clip;
    if (p->eof || ssrnx_stereo_ring_space(&p->ring) < SSRNX_MEDIA_DECODE_FRAME_CAP) return true;
    bool eof = false;
    if (!ssrnx_mp3_decoder_decode_into(p->mp3, &p->ring, &p->source_rate, &eof,
                                       &media_g_stats.mp3_frames_decoded,
                                       &media_g_stats.source_bytes_read)) {
        p->failed = true;
        p->state = SSRNX_MEDIA_STOPPED;
        SSRNX_LOGCE("MEDIA", "decoder failure player=%p", (void *)p);
        return false;
    }
    p->eof = eof;
    return true;
}

static bool media_player_ensure_source_frames(SsrnxMediaPlayer *p, size_t need) {
    unsigned guard = 0;
    while (p->ring.count < need && !p->eof && !p->failed && guard++ < 16) {
        const size_t before = p->ring.count;
        if (!media_player_decode_more(p)) return false;
        if (p->ring.count == before && !p->eof) {
            media_g_stats.decode_starvations++;
            break;
        }
    }
    return p->ring.count > 0;
}

static bool media_player_rewind_for_loop(SsrnxMediaPlayer *p) {
    if (!p || !p->loop) return false;
    if (p->loop_start_ms != 0 || (p->loop_end_ms != 0 && p->loop_end_ms != UINT32_MAX)) return false;
    return media_player_reset_decoder(p);
}

static bool media_clip_next_sample(SsrnxMediaPlayer *p, float *left, float *right) {
    if (!p || !p->clip || !p->clip->samples || p->clip->frames == 0 || !p->clip->source_rate) return false;

again:
    if (p->clip_pos >= (double)p->clip->frames) {
        const double overflow = p->clip_pos - (double)p->clip->frames;
        if (media_player_rewind_for_loop(p)) {
            p->clip_pos = fmod(overflow, (double)p->clip->frames);
            goto again;
        }
        media_player_mark_finished(p);
        return false;
    }

    size_t i0 = (size_t)p->clip_pos;
    size_t i1 = i0 + 1u < p->clip->frames ? i0 + 1u : i0;
    const float frac = (float)(p->clip_pos - (double)i0);
    const float l0 = (float)p->clip->samples[i0 * 2u] / 32768.0f;
    const float r0 = (float)p->clip->samples[i0 * 2u + 1u] / 32768.0f;
    const float l1 = (float)p->clip->samples[i1 * 2u] / 32768.0f;
    const float r1 = (float)p->clip->samples[i1 * 2u + 1u] / 32768.0f;
    *left = (l0 + (l1 - l0) * frac) * p->gain_left;
    *right = (r0 + (r1 - r0) * frac) * p->gain_right;

    p->clip_pos += (double)p->clip->source_rate * p->playback_rate / (double)SSRNX_MEDIA_OUTPUT_RATE;
    p->output_frames_played++;
    return true;
}

static bool media_player_next_sample(SsrnxMediaPlayer *p, float *left, float *right) {
    if (!p || !left || !right || p->state != SSRNX_MEDIA_PLAYING || !p->realized || p->failed) return false;
    if (p->clip) return media_clip_next_sample(p, left, right);

again:
    if (!media_player_ensure_source_frames(p, 2)) {
        if (p->eof && p->ring.count == 0 && media_player_rewind_for_loop(p)) goto again;
        if (p->eof && p->ring.count == 0) media_player_mark_finished(p);
        return false;
    }

    float l0 = 0.0f, r0 = 0.0f, l1 = 0.0f, r1 = 0.0f;
    if (!ssrnx_stereo_ring_peek(&p->ring, 0, &l0, &r0)) return false;
    if (!ssrnx_stereo_ring_peek(&p->ring, 1, &l1, &r1)) { l1 = l0; r1 = r0; }

    const float frac = (float)p->phase;
    *left = (l0 + (l1 - l0) * frac) * p->gain_left;
    *right = (r0 + (r1 - r0) * frac) * p->gain_right;

    const double step = p->source_rate ? (double)p->source_rate * p->playback_rate / (double)SSRNX_MEDIA_OUTPUT_RATE : 1.0;
    p->phase += step;
    const size_t advance = (size_t)p->phase;
    if (advance) {
        ssrnx_stereo_ring_discard(&p->ring, advance);
        p->phase -= (double)advance;
    }
    p->output_frames_played++;
    return true;
}

bool ssrnx_media_init(void) {
    pthread_mutex_lock(&media_g_media_lock);
    if (!media_g_initialized) {
        memset(&media_g_stats, 0, sizeof(media_g_stats));
        media_g_initialized = true;
        ssrnx_audio_set_renderer(ssrnx_media_render_s16, NULL);
        SSRNX_LOGCI("MEDIA", "media engine initialized output=%uHz stereo s16",
                    SSRNX_MEDIA_OUTPUT_RATE);
    }
    pthread_mutex_unlock(&media_g_media_lock);
    return true;
}

static SsrnxMediaPlayer *media_player_alloc(void) {
    SsrnxMediaPlayer *p = calloc(1, sizeof(*p));
    if (!p) return NULL;
    p->state = SSRNX_MEDIA_STOPPED;
    p->gain_left = 1.0f;
    p->gain_right = 1.0f;
    p->playback_rate = 1.0f;
    return p;
}

static void media_player_link(SsrnxMediaPlayer *p, bool realized) {
    pthread_mutex_lock(&media_g_media_lock);
    p->next = media_g_players;
    media_g_players = p;
    media_g_stats.players_created++;
    if (realized) media_g_stats.players_realized++;
    pthread_mutex_unlock(&media_g_media_lock);
}

static void media_player_free(SsrnxMediaPlayer *p) {
    if (!p) return;
    ssrnx_mp3_decoder_destroy(p->mp3);
    p->mp3 = NULL;
    ssrnx_stereo_ring_free(&p->ring);
    ssrnx_media_source_close(&p->source);
    ssrnx_media_clip_release(p->clip);
    p->clip = NULL;
    free(p);
}

void ssrnx_media_reset(void) {
    pthread_mutex_lock(&media_g_media_lock);
    SsrnxMediaPlayer *p = media_g_players;
    media_g_players = NULL;
    while (p) {
        SsrnxMediaPlayer *next = p->next;
        media_player_free(p);
        p = next;
    }
    memset(&media_g_stats, 0, sizeof(media_g_stats));
    media_g_initialized = false;
    ssrnx_audio_set_renderer(NULL, NULL);
    pthread_mutex_unlock(&media_g_media_lock);
}

SsrnxMediaClip *ssrnx_media_clip_load_path(const char *path) {
    if (!media_g_initialized && !ssrnx_media_init()) return NULL;
    SsrnxMediaClip *clip = ssrnx_media_clip_decode_path_internal(path);
    if (!clip) return NULL;
    pthread_mutex_lock(&media_g_media_lock);
    if (clip->codec == SSRNX_MEDIA_CODEC_WAV) media_g_stats.wav_clips_decoded++;
    else if (clip->codec == SSRNX_MEDIA_CODEC_VORBIS) media_g_stats.vorbis_clips_decoded++;
    pthread_mutex_unlock(&media_g_media_lock);
    return clip;
}

SsrnxMediaPlayer *ssrnx_media_player_create_clip(SsrnxMediaClip *clip) {
    if (!clip || !clip->samples || !clip->frames || !clip->source_rate) return NULL;
    if (!media_g_initialized && !ssrnx_media_init()) return NULL;
    SsrnxMediaPlayer *p = media_player_alloc();
    if (!p) return NULL;
    ssrnx_media_clip_retain(clip);
    p->clip = clip;
    p->codec = (SsrnxMediaCodec)clip->codec;
    p->source_rate = clip->source_rate;
    p->realized = true;
    media_player_link(p, true);
    SSRNX_LOGCI("MEDIA", "clip player=%p codec=%d rate=%u frames=%zu",
                (void *)p, (int)p->codec, p->source_rate, clip->frames);
    return p;
}

SsrnxMediaPlayer *ssrnx_media_player_create_path(const char *path) {
    SsrnxMediaClip *clip = ssrnx_media_clip_load_path(path);
    if (!clip) return NULL;
    SsrnxMediaPlayer *p = ssrnx_media_player_create_clip(clip);
    ssrnx_media_clip_release(clip);
    return p;
}

/* Upper bound for a compressed effect decoded in one piece (SoundPool). */
#define SSRNX_MEDIA_FD_CLIP_MAX_BYTES (64u << 20)

static ssize_t media_media_fd_read_head(int fd, int64_t offset, int64_t length, void *dst, size_t bytes,
                                  int64_t *resolved_length) {
    SsrnxMediaSource src;
    if (!ssrnx_media_source_init(&src, fd, offset, length)) return -1;
    if (resolved_length) *resolved_length = src.length;
    size_t got = 0;
    while (got < bytes) {
        const ssize_t rc = ssrnx_media_source_read(&src, (unsigned char *)dst + got, bytes - got);
        if (rc <= 0) break;
        got += (size_t)rc;
    }
    ssrnx_media_source_close(&src);
    return (ssize_t)got;
}

SsrnxMediaClip *ssrnx_media_clip_load_fd(int fd, int64_t offset, int64_t length) {
    if (!media_g_initialized && !ssrnx_media_init()) return NULL;
    int64_t size = 0;
    unsigned char probe;
    if (media_media_fd_read_head(fd, offset, length, &probe, 0, &size) < 0 || size < 12 ||
        size > (int64_t)SSRNX_MEDIA_FD_CLIP_MAX_BYTES) {
        SSRNX_LOGCW("MEDIA", "fd clip rejected fd=%d offset=%lld length=%lld", fd,
                    (long long)offset, (long long)size);
        return NULL;
    }
    unsigned char *data = malloc((size_t)size);
    if (!data) return NULL;
    SsrnxMediaClip *clip = NULL;
    char label[64];
    snprintf(label, sizeof(label), "fd:%d@%lld+%lld", fd, (long long)offset, (long long)size);
    if (media_media_fd_read_head(fd, offset, length, data, (size_t)size, NULL) == (ssize_t)size)
        clip = ssrnx_media_clip_decode_memory_internal(data, (size_t)size, label);
    free(data);
    if (!clip) return NULL;
    pthread_mutex_lock(&media_g_media_lock);
    if (clip->codec == SSRNX_MEDIA_CODEC_WAV) media_g_stats.wav_clips_decoded++;
    else if (clip->codec == SSRNX_MEDIA_CODEC_VORBIS) media_g_stats.vorbis_clips_decoded++;
    pthread_mutex_unlock(&media_g_media_lock);
    return clip;
}

SsrnxMediaPlayer *ssrnx_media_player_create_fd(int fd, int64_t offset, int64_t length) {
    unsigned char head[12];
    if (media_media_fd_read_head(fd, offset, length, head, sizeof(head), NULL) != (ssize_t)sizeof(head)) return NULL;
    const bool riff_wave = memcmp(head, "RIFF", 4) == 0 && memcmp(head + 8, "WAVE", 4) == 0;
    if (riff_wave || memcmp(head, "OggS", 4) == 0) {
        SsrnxMediaClip *clip = ssrnx_media_clip_load_fd(fd, offset, length);
        if (!clip) return NULL;
        SsrnxMediaPlayer *p = ssrnx_media_player_create_clip(clip);
        ssrnx_media_clip_release(clip);
        return p;
    }
    /* MPEG audio: an ID3v2 tag or an MPEG frame sync (11 set bits). */
    const bool mpeg = memcmp(head, "ID3", 3) == 0 || (head[0] == 0xff && (head[1] & 0xe0) == 0xe0);
    if (!mpeg) {
        SSRNX_LOGCW("MEDIA", "unsupported fd audio container fd=%d magic=%02x%02x%02x%02x",
                    fd, head[0], head[1], head[2], head[3]);
        return NULL;
    }
    return ssrnx_media_player_create_android_fd(fd, offset, length, 0, "audio/mpeg");
}

SsrnxMediaPlayer *ssrnx_media_player_create_android_fd(int fd,
                                                        int64_t offset,
                                                        int64_t length,
                                                        uint32_t container_type,
                                                        const char *mime_type) {
    if (!media_g_initialized && !ssrnx_media_init()) return NULL;
    SsrnxMediaPlayer *p = media_player_alloc();
    if (!p) return NULL;
    if (!ssrnx_media_source_init(&p->source, fd, offset, length) ||
        !ssrnx_stereo_ring_init(&p->ring, SSRNX_MEDIA_SOURCE_RING_FRAMES)) {
        ssrnx_media_source_close(&p->source);
        ssrnx_stereo_ring_free(&p->ring);
        free(p);
        return NULL;
    }
    p->container_type = container_type;
    if (mime_type) {
        strncpy(p->mime_type, mime_type, sizeof(p->mime_type) - 1u);
        p->mime_type[sizeof(p->mime_type) - 1u] = '\0';
    }
    p->codec = media_codec_is_mp3(container_type, p->mime_type) ? SSRNX_MEDIA_CODEC_MP3 : SSRNX_MEDIA_CODEC_UNKNOWN;
    media_player_link(p, false);

    SSRNX_LOGCI("MEDIA", "AndroidFD player=%p fd=%d offset=%lld length=%lld container=0x%x mime=%s codec=%s",
                (void *)p, fd, (long long)offset, (long long)p->source.length,
                container_type, p->mime_type[0] ? p->mime_type : "(null)",
                p->codec == SSRNX_MEDIA_CODEC_MP3 ? "MP3" : "unknown");
    return p;
}

bool ssrnx_media_player_realize(SsrnxMediaPlayer *p) {
    if (!p) return false;
    pthread_mutex_lock(&media_g_media_lock);
    if (p->realized) { pthread_mutex_unlock(&media_g_media_lock); return true; }
    if (p->codec != SSRNX_MEDIA_CODEC_MP3) {
        pthread_mutex_unlock(&media_g_media_lock);
        SSRNX_LOGCE("MEDIA", "unsupported compressed codec player=%p container=0x%x", (void *)p, p->container_type);
        return false;
    }
    p->mp3 = ssrnx_mp3_decoder_create(&p->source);
    if (!p->mp3) { pthread_mutex_unlock(&media_g_media_lock); return false; }
    p->realized = true;
    if (!media_player_decode_more(p) || p->source_rate == 0 || p->ring.count == 0) {
        p->failed = true;
        pthread_mutex_unlock(&media_g_media_lock);
        SSRNX_LOGCE("MEDIA", "MP3 realize failed player=%p", (void *)p);
        return false;
    }
    media_g_stats.players_realized++;
    const uint32_t rate = p->source_rate;
    const size_t buffered = p->ring.count;
    pthread_mutex_unlock(&media_g_media_lock);
    SSRNX_LOGCI("MEDIA", "MP3 realized player=%p rate=%u channels=2 normalized buffered=%zu resample=%u->%u",
                (void *)p, rate, buffered, rate, SSRNX_MEDIA_OUTPUT_RATE);
    return true;
}

void ssrnx_media_player_destroy(SsrnxMediaPlayer *p) {
    if (!p) return;
    pthread_mutex_lock(&media_g_media_lock);
    SsrnxMediaPlayer **link = &media_g_players;
    while (*link && *link != p) link = &(*link)->next;
    if (*link == p) *link = p->next;
    if (p->state == SSRNX_MEDIA_PLAYING && media_g_stats.active_players) media_g_stats.active_players--;
    pthread_mutex_unlock(&media_g_media_lock);
    SSRNX_LOGCI("MEDIA", "player destroy=%p", (void *)p);
    media_player_free(p);
}

bool ssrnx_media_player_set_state(SsrnxMediaPlayer *p, SsrnxMediaState state) {
    if (!p || state < SSRNX_MEDIA_STOPPED || state > SSRNX_MEDIA_PLAYING) return false;
    pthread_mutex_lock(&media_g_media_lock);
    if (state == SSRNX_MEDIA_PLAYING && !p->realized) {
        pthread_mutex_unlock(&media_g_media_lock);
        return false;
    }
    const SsrnxMediaState old = p->state;
    if (state == SSRNX_MEDIA_STOPPED) {
        if (p->realized && !media_player_reset_decoder(p)) p->failed = true;
    } else if (state == SSRNX_MEDIA_PLAYING) {
        bool at_end = p->clip ? p->clip_pos >= (double)p->clip->frames : (p->eof && p->ring.count == 0);
        if (at_end && !media_player_reset_decoder(p)) p->failed = true;
    }
    if (p->failed) { p->state = SSRNX_MEDIA_STOPPED; pthread_mutex_unlock(&media_g_media_lock); return false; }
    p->state = state;
    if (old != SSRNX_MEDIA_PLAYING && state == SSRNX_MEDIA_PLAYING) {
        media_g_stats.active_players++;
        media_g_stats.players_started++;
    } else if (old == SSRNX_MEDIA_PLAYING && state != SSRNX_MEDIA_PLAYING && media_g_stats.active_players) {
        media_g_stats.active_players--;
    }
    pthread_mutex_unlock(&media_g_media_lock);
    SSRNX_LOGCI("MEDIA", "player=%p state=%u", (void *)p, (unsigned)state);
    return true;
}

SsrnxMediaState ssrnx_media_player_get_state(SsrnxMediaPlayer *p) {
    if (!p) return SSRNX_MEDIA_STOPPED;
    pthread_mutex_lock(&media_g_media_lock);
    SsrnxMediaState state = p->state;
    pthread_mutex_unlock(&media_g_media_lock);
    return state;
}

uint32_t ssrnx_media_player_get_duration_ms(SsrnxMediaPlayer *p) {
    if (!p) return 0;
    pthread_mutex_lock(&media_g_media_lock);
    uint32_t duration = p->clip ? ssrnx_media_clip_duration_ms(p->clip) : UINT32_MAX;
    pthread_mutex_unlock(&media_g_media_lock);
    return duration;
}

uint32_t ssrnx_media_player_get_position_ms(SsrnxMediaPlayer *p) {
    if (!p) return 0;
    pthread_mutex_lock(&media_g_media_lock);
    uint64_t ms;
    if (p->clip && p->clip->source_rate) {
        ms = (uint64_t)(p->clip_pos * 1000.0 / (double)p->clip->source_rate);
    } else {
        ms = p->output_frames_played * 1000u / SSRNX_MEDIA_OUTPUT_RATE;
    }
    pthread_mutex_unlock(&media_g_media_lock);
    return ms > UINT32_MAX ? UINT32_MAX : (uint32_t)ms;
}

bool ssrnx_media_player_seek_ms(SsrnxMediaPlayer *p, uint32_t position_ms) {
    if (!p) return false;
    pthread_mutex_lock(&media_g_media_lock);
    bool ok = false;
    if (p->clip && p->clip->source_rate) {
        double pos = (double)position_ms * (double)p->clip->source_rate / 1000.0;
        if (pos <= (double)p->clip->frames) {
            p->clip_pos = pos;
            p->output_frames_played = (uint64_t)position_ms * SSRNX_MEDIA_OUTPUT_RATE / 1000u;
            ok = true;
        }
    } else if (position_ms == 0 && p->realized) {
        ok = media_player_reset_decoder(p);
    }
    pthread_mutex_unlock(&media_g_media_lock);
    return ok;
}

bool ssrnx_media_player_set_loop(SsrnxMediaPlayer *p, bool enabled,
                                  uint32_t start_ms, uint32_t end_ms) {
    if (!p) return false;
    if (enabled && (start_ms != 0 || (end_ms != 0 && end_ms != UINT32_MAX))) return false;
    pthread_mutex_lock(&media_g_media_lock);
    p->loop = enabled;
    p->loop_start_ms = start_ms;
    p->loop_end_ms = end_ms;
    pthread_mutex_unlock(&media_g_media_lock);
    return true;
}

bool ssrnx_media_player_set_volume(SsrnxMediaPlayer *p, float left, float right) {
    if (!p) return false;
    if (left < 0.0f) left = 0.0f;
    if (right < 0.0f) right = 0.0f;
    if (left > 1.0f) left = 1.0f;
    if (right > 1.0f) right = 1.0f;
    pthread_mutex_lock(&media_g_media_lock);
    p->gain_left = left;
    p->gain_right = right;
    pthread_mutex_unlock(&media_g_media_lock);
    return true;
}

bool ssrnx_media_player_set_rate(SsrnxMediaPlayer *p, float rate) {
    if (!p || !isfinite(rate) || rate < 0.5f || rate > 2.0f) return false;
    pthread_mutex_lock(&media_g_media_lock);
    p->playback_rate = rate;
    pthread_mutex_unlock(&media_g_media_lock);
    return true;
}

size_t ssrnx_media_render_s16(int16_t *out, size_t frames, void *unused) {
    (void)unused;
    if (!out || frames == 0) return 0;
    pthread_mutex_lock(&media_g_media_lock);
    for (size_t frame = 0; frame < frames; ++frame) {
        float mix_l = 0.0f, mix_r = 0.0f;
        for (SsrnxMediaPlayer *p = media_g_players; p; p = p->next) {
            float l = 0.0f, r = 0.0f;
            if (media_player_next_sample(p, &l, &r)) {
                mix_l += l;
                mix_r += r;
            }
        }
        mix_l = media_clamp_unit(mix_l);
        mix_r = media_clamp_unit(mix_r);
        out[frame * 2u] = (int16_t)lrintf(mix_l * 32767.0f);
        out[frame * 2u + 1u] = (int16_t)lrintf(mix_r * 32767.0f);
    }
    media_g_stats.output_frames_mixed += frames;
    pthread_mutex_unlock(&media_g_media_lock);

    /* OpenSL Android Simple Buffer Queue is an independent PCM producer. Mix it
     * after releasing the media lock because consuming a queue buffer may invoke
     * the guest refill callback, which can immediately enqueue another buffer. */
    if (ssrnx_opensles_mix_s16) (void)ssrnx_opensles_mix_s16(out, frames);
    if (ssrnx_aaudio_mix_s16) (void)ssrnx_aaudio_mix_s16(out, frames);
    return frames;
}

void ssrnx_media_get_stats(SsrnxMediaStats *out) {
    if (!out) return;
    pthread_mutex_lock(&media_g_media_lock);
    *out = media_g_stats;
    pthread_mutex_unlock(&media_g_media_lock);
}

#pragma pop_macro("SSRNX_SL_CONTAINERTYPE_UNSPECIFIED")
#pragma pop_macro("SSRNX_SL_CONTAINERTYPE_MP3")
#pragma pop_macro("SSRNX_MEDIA_FD_CLIP_MAX_BYTES")


/* ---- media_clip ---- */




#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef SSRNX_HAVE_VORBIS
/* libvorbisfile publishes four header-local callback tables. They are useful to
 * consumers that select those presets, but SSRNX supplies its own callbacks, so
 * GCC otherwise reports the header's static variables as unused in this TU. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <vorbis/vorbisfile.h>
#pragma GCC diagnostic pop
#endif

static uint16_t media_clip_rd16(const unsigned char *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t media_clip_rd32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int16_t media_clip_clamp_s16(int32_t v) {
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (int16_t)v;
}

static int16_t media_clip_float_to_s16(float v) {
    if (v > 1.0f) v = 1.0f;
    if (v < -1.0f) v = -1.0f;
    return media_clip_clamp_s16((int32_t)lrintf(v * 32767.0f));
}

static SsrnxMediaClip *media_clip_clip_alloc(size_t frames, uint32_t rate, SsrnxMediaCodec codec) {
    if (!frames || !rate || frames > SIZE_MAX / (2u * sizeof(int16_t))) return NULL;
    SsrnxMediaClip *clip = calloc(1, sizeof(*clip));
    if (!clip) return NULL;
    clip->samples = malloc(frames * 2u * sizeof(int16_t));
    if (!clip->samples) {
        free(clip);
        return NULL;
    }
    clip->frames = frames;
    clip->source_rate = rate;
    atomic_init(&clip->refcount, 1u);
    clip->codec = (int)codec;
    return clip;
}

static bool media_clip_ext_is(const char *path, const char *ext) {
    if (!path || !ext) return false;
    size_t pn = strlen(path), en = strlen(ext);
    if (pn < en) return false;
    const char *a = path + pn - en;
    for (size_t i = 0; i < en; ++i) {
        char ca = a[i], cb = ext[i];
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return false;
    }
    return true;
}

/* Decode a RIFF/WAVE stream; takes ownership of f. */
static SsrnxMediaClip *media_clip_load_wav_stream(FILE *f, const char *path) {
    if (!f) return NULL;

    unsigned char hdr[12];
    if (fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr) ||
        memcmp(hdr, "RIFF", 4) != 0 || memcmp(hdr + 8, "WAVE", 4) != 0) {
        fclose(f);
        return NULL;
    }

    uint16_t format = 0, channels = 0, bits = 0;
    uint32_t rate = 0, block_align = 0;
    long data_pos = -1;
    uint32_t data_size = 0;

    for (;;) {
        unsigned char ch[8];
        if (fread(ch, 1, sizeof(ch), f) != sizeof(ch)) break;
        uint32_t size = media_clip_rd32(ch + 4);
        long payload = ftell(f);
        if (payload < 0) break;
        if (memcmp(ch, "fmt ", 4) == 0 && size >= 16) {
            unsigned char fmt[40] = {0};
            size_t take = size < sizeof(fmt) ? size : sizeof(fmt);
            if (fread(fmt, 1, take, f) != take) break;
            format = media_clip_rd16(fmt + 0);
            channels = media_clip_rd16(fmt + 2);
            rate = media_clip_rd32(fmt + 4);
            block_align = media_clip_rd16(fmt + 12);
            bits = media_clip_rd16(fmt + 14);
        } else if (memcmp(ch, "data", 4) == 0) {
            data_pos = payload;
            data_size = size;
        }
        long next = payload + (long)size + (long)(size & 1u);
        if (fseek(f, next, SEEK_SET) != 0) break;
        if (data_pos >= 0 && format && channels && rate && bits) break;
    }

    if (data_pos < 0 || !channels || !rate || !bits || !block_align ||
        (format != 1 && format != 3) || data_size < block_align) {
        fclose(f);
        return NULL;
    }

    const size_t frames = data_size / block_align;
    SsrnxMediaClip *clip = media_clip_clip_alloc(frames, rate, SSRNX_MEDIA_CODEC_WAV);
    if (!clip) { fclose(f); return NULL; }
    if (fseek(f, data_pos, SEEK_SET) != 0) {
        ssrnx_media_clip_release(clip);
        fclose(f);
        return NULL;
    }

    unsigned char *frame = malloc(block_align);
    if (!frame) {
        ssrnx_media_clip_release(clip);
        fclose(f);
        return NULL;
    }

    const unsigned bytes_per_sample = (bits + 7u) / 8u;
    if (bytes_per_sample == 0 || (uint32_t)channels * bytes_per_sample > block_align) {
        free(frame); ssrnx_media_clip_release(clip); fclose(f); return NULL;
    }

    size_t written = 0;
    for (; written < frames; ++written) {
        if (fread(frame, 1, block_align, f) != block_align) break;
        int16_t out[2] = {0, 0};
        for (unsigned dst = 0; dst < 2; ++dst) {
            unsigned src_ch = channels == 1 ? 0u : dst;
            if (src_ch >= channels) src_ch = channels - 1u;
            const unsigned char *s = frame + src_ch * bytes_per_sample;
            int16_t v = 0;
            if (format == 3 && bits == 32) {
                float fv = 0.0f;
                memcpy(&fv, s, sizeof(fv));
                v = media_clip_float_to_s16(fv);
            } else if (format == 1) {
                switch (bits) {
                    case 8:  v = (int16_t)(((int)s[0] - 128) << 8); break;
                    case 16: v = (int16_t)media_clip_rd16(s); break;
                    case 24: {
                        int32_t x = (int32_t)((uint32_t)s[0] | ((uint32_t)s[1] << 8) | ((uint32_t)s[2] << 16));
                        if (x & 0x00800000) x |= (int32_t)0xff000000;
                        v = media_clip_clamp_s16(x >> 8);
                        break;
                    }
                    case 32: {
                        int32_t x = (int32_t)media_clip_rd32(s);
                        v = media_clip_clamp_s16(x >> 16);
                        break;
                    }
                    default: free(frame); ssrnx_media_clip_release(clip); fclose(f); return NULL;
                }
            } else {
                free(frame); ssrnx_media_clip_release(clip); fclose(f); return NULL;
            }
            out[dst] = v;
        }
        clip->samples[written * 2u] = out[0];
        clip->samples[written * 2u + 1u] = out[1];
    }
    free(frame);
    fclose(f);

    if (written == 0) { ssrnx_media_clip_release(clip); return NULL; }
    clip->frames = written;
    SSRNX_LOGCI("MEDIA", "WAV decoded path=%s rate=%u frames=%zu", path, rate, written);
    return clip;
}

static SsrnxMediaClip *media_clip_load_wav(const char *path) {
    return media_clip_load_wav_stream(fopen(path, "rb"), path);
}

#ifdef SSRNX_HAVE_VORBIS
/* libvorbisfile callbacks over a borrowed in-memory Ogg file. */
typedef struct media_clip_MemoryOgg {
    const unsigned char *data;
    size_t length;
    size_t pos;
} media_clip_MemoryOgg;

static size_t media_clip_memory_ogg_read(void *dst, size_t size, size_t count, void *src) {
    media_clip_MemoryOgg *m = src;
    if (!size || m->pos >= m->length) return 0;
    size_t items = (m->length - m->pos) / size;
    if (items > count) items = count;
    memcpy(dst, m->data + m->pos, items * size);
    m->pos += items * size;
    return items;
}

static int media_clip_memory_ogg_seek(void *src, ogg_int64_t offset, int whence) {
    media_clip_MemoryOgg *m = src;
    ogg_int64_t base = whence == SEEK_SET ? 0 : whence == SEEK_CUR ? (ogg_int64_t)m->pos
                     : whence == SEEK_END ? (ogg_int64_t)m->length : -1;
    if (base < 0 || base + offset < 0 || base + offset > (ogg_int64_t)m->length) return -1;
    m->pos = (size_t)(base + offset);
    return 0;
}

static long media_clip_memory_ogg_tell(void *src) {
    return (long)((media_clip_MemoryOgg *)src)->pos;
}

static SsrnxMediaClip *media_clip_decode_vorbis(OggVorbis_File *opened, const char *path);

static SsrnxMediaClip *media_clip_load_vorbis(const char *path) {
    OggVorbis_File vf;
    memset(&vf, 0, sizeof(vf));
    if (ov_fopen(path, &vf) != 0) return NULL;
    return media_clip_decode_vorbis(&vf, path);
}

static SsrnxMediaClip *media_clip_load_vorbis_memory(const unsigned char *data, size_t length, const char *label) {
    media_clip_MemoryOgg m = {data, length, 0};
    const ov_callbacks callbacks = {media_clip_memory_ogg_read, media_clip_memory_ogg_seek, NULL, media_clip_memory_ogg_tell};
    OggVorbis_File vf;
    memset(&vf, 0, sizeof(vf));
    if (ov_open_callbacks(&m, &vf, NULL, 0, callbacks) != 0) return NULL;
    return media_clip_decode_vorbis(&vf, label);
}

/* Decode an opened Vorbis stream to a stereo clip; always clears vf. */
static SsrnxMediaClip *media_clip_decode_vorbis(OggVorbis_File *vf, const char *path) {
    vorbis_info *info = ov_info(vf, -1);
    if (!info || info->rate <= 0 || info->channels <= 0) {
        ov_clear(vf);
        return NULL;
    }

    ogg_int64_t total = ov_pcm_total(vf, -1);
    size_t capacity = total > 0 && (uint64_t)total < SIZE_MAX / 2u ? (size_t)total : (size_t)info->rate * 30u;
    if (capacity < 4096u) capacity = 4096u;
    SsrnxMediaClip *clip = media_clip_clip_alloc(capacity, (uint32_t)info->rate, SSRNX_MEDIA_CODEC_VORBIS);
    if (!clip) { ov_clear(vf); return NULL; }
    clip->frames = 0;

    unsigned char buf[16384];
    int bitstream = 0;
    for (;;) {
        long bytes = ov_read(vf, (char *)buf, (int)sizeof(buf), 0, 2, 1, &bitstream);
        if (bytes == 0) break;
        if (bytes < 0) continue; /* recoverable hole */
        const size_t samples = (size_t)bytes / sizeof(int16_t);
        const size_t frames = samples / (size_t)info->channels;
        if (!frames) continue;
        if (clip->frames > SIZE_MAX - frames) { ssrnx_media_clip_release(clip); ov_clear(vf); return NULL; }
        const size_t need = clip->frames + frames;
        if (need > capacity) {
            size_t newcap = capacity;
            while (newcap < need) {
                if (newcap > SIZE_MAX / 2u) { newcap = need; break; }
                newcap *= 2u;
            }
            if (newcap > SIZE_MAX / (2u * sizeof(int16_t))) { ssrnx_media_clip_release(clip); ov_clear(vf); return NULL; }
            int16_t *grown = realloc(clip->samples, newcap * 2u * sizeof(int16_t));
            if (!grown) { ssrnx_media_clip_release(clip); ov_clear(vf); return NULL; }
            clip->samples = grown;
            capacity = newcap;
        }
        const int16_t *src = (const int16_t *)buf;
        for (size_t i = 0; i < frames; ++i) {
            int16_t l = src[i * (size_t)info->channels];
            int16_t r = info->channels > 1 ? src[i * (size_t)info->channels + 1u] : l;
            clip->samples[(clip->frames + i) * 2u] = l;
            clip->samples[(clip->frames + i) * 2u + 1u] = r;
        }
        clip->frames += frames;
    }
    ov_clear(vf);
    if (!clip->frames) { ssrnx_media_clip_release(clip); return NULL; }
    SSRNX_LOGCI("MEDIA", "Vorbis decoded path=%s rate=%u frames=%zu", path, clip->source_rate, clip->frames);
    return clip;
}
#endif

SsrnxMediaClip *ssrnx_media_clip_decode_path_internal(const char *path) {
    if (!path || !path[0]) return NULL;
    if (media_clip_ext_is(path, ".wav")) return media_clip_load_wav(path);
#ifdef SSRNX_HAVE_VORBIS
    if (media_clip_ext_is(path, ".ogg") || media_clip_ext_is(path, ".oga")) return media_clip_load_vorbis(path);
#else
    if (media_clip_ext_is(path, ".ogg") || media_clip_ext_is(path, ".oga")) {
        SSRNX_LOGCW("MEDIA", "Vorbis path requested but APN64 was built without Vorbis support path=%s", path);
        return NULL;
    }
#endif
    SSRNX_LOGCW("MEDIA", "unsupported path audio codec path=%s", path);
    return NULL;
}

SsrnxMediaClip *ssrnx_media_clip_decode_memory_internal(const unsigned char *data, size_t length,
                                                       const char *label) {
    if (!data || length < 12) return NULL;
    if (memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WAVE", 4) == 0)
        return media_clip_load_wav_stream(fmemopen((void *)data, length, "rb"), label);
    if (memcmp(data, "OggS", 4) == 0) {
#ifdef SSRNX_HAVE_VORBIS
        return media_clip_load_vorbis_memory(data, length, label);
#else
        SSRNX_LOGCW("MEDIA", "Vorbis data requested but APN64 was built without Vorbis support source=%s", label);
        return NULL;
#endif
    }
    SSRNX_LOGCW("MEDIA", "unsupported in-memory audio container source=%s magic=%02x%02x%02x%02x",
                label, data[0], data[1], data[2], data[3]);
    return NULL;
}

void ssrnx_media_clip_retain(SsrnxMediaClip *clip) {
    if (clip) (void)atomic_fetch_add_explicit(&clip->refcount, 1u, memory_order_relaxed);
}

void ssrnx_media_clip_release(SsrnxMediaClip *clip) {
    if (!clip) return;
    if (atomic_fetch_sub_explicit(&clip->refcount, 1u, memory_order_acq_rel) == 1u) {
        free(clip->samples);
        free(clip);
    }
}

uint32_t ssrnx_media_clip_duration_ms(const SsrnxMediaClip *clip) {
    if (!clip || !clip->source_rate) return 0;
    uint64_t ms = (uint64_t)clip->frames * 1000u / clip->source_rate;
    return ms > UINT32_MAX ? UINT32_MAX : (uint32_t)ms;
}



/* ---- media_source ---- */



#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

bool ssrnx_media_source_range_valid(int fd, int64_t offset, int64_t length,
                                    int64_t *resolved_length) {
    if (fd < 0 || offset < 0) return false;
    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size < 0) return false;
    const int64_t size = (int64_t)st.st_size;
    if (offset > size) return false;
    int64_t use_length = length;
    if (length == SSRNX_MEDIA_ANDROIDFD_USE_FILE_SIZE) use_length = size - offset;
    if (use_length < 0 || use_length > size - offset) return false;
    if (resolved_length) *resolved_length = use_length;
    return true;
}

bool ssrnx_media_source_init(SsrnxMediaSource *src, int fd, int64_t offset, int64_t length) {
    if (!src) return false;
    memset(src, 0, sizeof(*src));
    src->fd = -1;
    int64_t resolved = 0;
    if (!ssrnx_media_source_range_valid(fd, offset, length, &resolved)) return false;

    /* AndroidFD applications keep the source descriptor alive until the
     * player object is destroyed. Borrow it instead of depending on dup(),
     * which is not uniformly provided by Switch/newlib environments. Each
     * player uses explicit lseek(base+position), so descriptor position is
     * never assumed to be stable between calls. */
    src->fd = fd;
    src->owns_fd = false;
    src->base = offset;
    src->length = resolved;
    src->pos = 0;
    return true;
}

void ssrnx_media_source_close(SsrnxMediaSource *src) {
    if (!src) return;
    src->fd = -1;
    src->owns_fd = false;
    src->base = src->length = src->pos = 0;
}

ssize_t ssrnx_media_source_read(SsrnxMediaSource *src, void *dst, size_t bytes) {
    if (!src || src->fd < 0 || (!dst && bytes)) { errno = EINVAL; return -1; }
    if (src->pos >= src->length || bytes == 0) return 0;
    int64_t remain = src->length - src->pos;
    if ((uint64_t)bytes > (uint64_t)remain) bytes = (size_t)remain;
    if (lseek(src->fd, (off_t)(src->base + src->pos), SEEK_SET) < 0) return -1;
    ssize_t rc = read(src->fd, dst, bytes);
    if (rc > 0) src->pos += rc;
    return rc;
}

bool ssrnx_media_source_seek(SsrnxMediaSource *src, int64_t relative) {
    if (!src || src->fd < 0 || relative < 0 || relative > src->length) return false;
    src->pos = relative;
    return true;
}

bool ssrnx_stereo_ring_init(SsrnxStereoRing *ring, size_t capacity_frames) {
    if (!ring || capacity_frames < 2) return false;
    memset(ring, 0, sizeof(*ring));
    if (capacity_frames > SIZE_MAX / (sizeof(float) * 2u)) return false;
    ring->samples = (float *)calloc(capacity_frames * 2u, sizeof(float));
    if (!ring->samples) return false;
    ring->capacity_frames = capacity_frames;
    return true;
}

void ssrnx_stereo_ring_reset(SsrnxStereoRing *ring) {
    if (!ring) return;
    ring->head = 0;
    ring->count = 0;
}

void ssrnx_stereo_ring_free(SsrnxStereoRing *ring) {
    if (!ring) return;
    free(ring->samples);
    memset(ring, 0, sizeof(*ring));
}

size_t ssrnx_stereo_ring_space(const SsrnxStereoRing *ring) {
    return ring && ring->capacity_frames >= ring->count ? ring->capacity_frames - ring->count : 0;
}

bool ssrnx_stereo_ring_push(SsrnxStereoRing *ring, float left, float right) {
    if (!ring || !ring->samples || ring->count >= ring->capacity_frames) return false;
    const size_t tail = (ring->head + ring->count) % ring->capacity_frames;
    ring->samples[tail * 2u] = left;
    ring->samples[tail * 2u + 1u] = right;
    ring->count++;
    return true;
}

bool ssrnx_stereo_ring_peek(const SsrnxStereoRing *ring, size_t index, float *left, float *right) {
    if (!ring || !ring->samples || index >= ring->count || !left || !right) return false;
    const size_t pos = (ring->head + index) % ring->capacity_frames;
    *left = ring->samples[pos * 2u];
    *right = ring->samples[pos * 2u + 1u];
    return true;
}

void ssrnx_stereo_ring_discard(SsrnxStereoRing *ring, size_t frames) {
    if (!ring || !ring->capacity_frames) return;
    if (frames > ring->count) frames = ring->count;
    ring->head = (ring->head + frames) % ring->capacity_frames;
    ring->count -= frames;
}



/* ---- mp3_decoder ---- */
#pragma push_macro("MINIMP3_FLOAT_OUTPUT")
#pragma push_macro("MINIMP3_IMPLEMENTATION")
#pragma push_macro("MINIMP3_NO_SIMD")
#pragma push_macro("MINIMP3_ONLY_MP3")



#include <stdlib.h>
#include <string.h>

#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_SIMD
#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include "../third_party/minimp3/minimp3.h"

struct SsrnxMp3Decoder {
    SsrnxMediaSource *source;
    mp3dec_t decoder;
    uint8_t compressed[SSRNX_MEDIA_COMPRESSED_BUFFER_BYTES];
    size_t compressed_size;
    bool input_eof;
    bool decoder_eof;
    bool tag_checked;
};


static bool mp3_decoder_skip_id3v2_if_present(SsrnxMp3Decoder *d) {
    if (!d || d->tag_checked) return true;
    if (d->compressed_size < 10u) return true;
    d->tag_checked = true;
    if (memcmp(d->compressed, "ID3", 3) != 0) return true;
    const uint8_t *h = d->compressed;
    if ((h[6] | h[7] | h[8] | h[9]) & 0x80u) return false;
    uint64_t payload = ((uint64_t)h[6] << 21) | ((uint64_t)h[7] << 14) |
                       ((uint64_t)h[8] << 7) | (uint64_t)h[9];
    uint64_t total = payload + 10u;
    if ((h[5] & 0x10u) != 0) total += 10u; /* ID3v2.4 footer */
    if (total > (uint64_t)d->source->length) return false;
    if (!ssrnx_media_source_seek(d->source, (int64_t)total)) return false;
    d->compressed_size = 0;
    d->input_eof = false;
    SSRNX_LOGCI("MEDIA", "MP3 skipped ID3v2 tag bytes=%llu", (unsigned long long)total);
    return true;
}

static bool mp3_decoder_refill(SsrnxMp3Decoder *d, uint64_t *bytes_read) {
    if (!d || !d->source || d->input_eof) return true;
    if (d->compressed_size >= sizeof(d->compressed)) return true;
    const size_t space = sizeof(d->compressed) - d->compressed_size;
    ssize_t got = ssrnx_media_source_read(d->source, d->compressed + d->compressed_size, space);
    if (got < 0) return false;
    if (got == 0) {
        d->input_eof = true;
        return true;
    }
    d->compressed_size += (size_t)got;
    if (bytes_read) *bytes_read += (uint64_t)got;
    return true;
}

static void consume(SsrnxMp3Decoder *d, size_t bytes) {
    if (!d || bytes == 0) return;
    if (bytes >= d->compressed_size) {
        d->compressed_size = 0;
        return;
    }
    memmove(d->compressed, d->compressed + bytes, d->compressed_size - bytes);
    d->compressed_size -= bytes;
}

SsrnxMp3Decoder *ssrnx_mp3_decoder_create(SsrnxMediaSource *source) {
    if (!source) return NULL;
    SsrnxMp3Decoder *d = (SsrnxMp3Decoder *)calloc(1, sizeof(*d));
    if (!d) return NULL;
    d->source = source;
    mp3dec_init(&d->decoder);
    return d;
}

void ssrnx_mp3_decoder_destroy(SsrnxMp3Decoder *decoder) {
    free(decoder);
}

bool ssrnx_mp3_decoder_reset(SsrnxMp3Decoder *d) {
    if (!d || !d->source || !ssrnx_media_source_seek(d->source, 0)) return false;
    mp3dec_init(&d->decoder);
    d->compressed_size = 0;
    d->input_eof = false;
    d->decoder_eof = false;
    d->tag_checked = false;
    return true;
}

bool ssrnx_mp3_decoder_decode_into(SsrnxMp3Decoder *d, SsrnxStereoRing *ring,
                                   uint32_t *sample_rate, bool *eof,
                                   uint64_t *decoded_mp3_frames,
                                   uint64_t *source_bytes_read) {
    if (!d || !ring || !sample_rate || !eof) return false;
    *eof = d->decoder_eof;
    if (d->decoder_eof) return true;

    for (unsigned attempts = 0; attempts < 8; ++attempts) {
        if (d->compressed_size < 4096u && !d->input_eof) {
            if (!mp3_decoder_refill(d, source_bytes_read)) return false;
        }
        if (!mp3_decoder_skip_id3v2_if_present(d)) return false;
        if (d->compressed_size == 0 && !d->input_eof) {
            if (!mp3_decoder_refill(d, source_bytes_read)) return false;
        }
        if (d->compressed_size == 0 && d->input_eof) {
            d->decoder_eof = true;
            *eof = true;
            return true;
        }

        float pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        mp3dec_frame_info_t info;
        memset(&info, 0, sizeof(info));
        const int samples_per_channel = mp3dec_decode_frame(&d->decoder,
                                                             d->compressed,
                                                             (int)d->compressed_size,
                                                             pcm, &info);
        if (info.frame_bytes > 0) consume(d, (size_t)info.frame_bytes);

        if (samples_per_channel > 0) {
            if (info.channels != 1 && info.channels != 2) {
                SSRNX_LOGCE("MEDIA", "MP3 unsupported channel count=%d", info.channels);
                return false;
            }
            if (info.hz <= 0) return false;
            *sample_rate = (uint32_t)info.hz;
            if (ssrnx_stereo_ring_space(ring) < (size_t)samples_per_channel) {
                /* Caller should drain before asking for more. */
                return true;
            }
            for (int i = 0; i < samples_per_channel; ++i) {
                float l, r;
                if (info.channels == 1) {
                    l = r = pcm[i];
                } else {
                    l = pcm[i * 2];
                    r = pcm[i * 2 + 1];
                }
                if (!ssrnx_stereo_ring_push(ring, l, r)) return false;
            }
            if (decoded_mp3_frames) (*decoded_mp3_frames)++;
            return true;
        }

        if (info.frame_bytes == 0) {
            if (d->input_eof) {
                d->decoder_eof = true;
                *eof = true;
                return true;
            }
            if (d->compressed_size == sizeof(d->compressed)) {
                SSRNX_LOGCE("MEDIA", "MP3 decoder stalled with %zu compressed bytes", d->compressed_size);
                return false;
            }
            if (!mp3_decoder_refill(d, source_bytes_read)) return false;
        }
    }
    return true;
}

#pragma pop_macro("MINIMP3_ONLY_MP3")
#pragma pop_macro("MINIMP3_NO_SIMD")
#pragma pop_macro("MINIMP3_IMPLEMENTATION")
#pragma pop_macro("MINIMP3_FLOAT_OUTPUT")
