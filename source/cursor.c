#include "runtime_base.h"
#include <math.h>
#include <pthread.h>

/* Input is sampled on the UI thread; Unity presents from another thread. */
static pthread_mutex_t cursor_lock = PTHREAD_MUTEX_INITIALIZER;
static bool cursor_enabled, cursor_wait_release;
static float cursor_x = 640.0f, cursor_y = 360.0f;

void nx_cursor_reset(void) {
    pthread_mutex_lock(&cursor_lock);
    cursor_enabled = cursor_wait_release = false;
    cursor_x = 640.0f; cursor_y = 360.0f;
    pthread_mutex_unlock(&cursor_lock);
}

static float cursor_axis(float value) {
    if (!isfinite(value)) return 0.0f;
    float magnitude = fminf(fabsf(value), 1.0f);
    return magnitude <= 0.18f ? 0.0f : copysignf((magnitude - 0.18f) / 0.82f, value);
}

void nx_cursor_update(SsrnxInputSnapshot *snapshot, double seconds) {
    if (!snapshot) return;
    bool raw_a = (snapshot->buttons & SSRNX_BTN_A) != 0;
    pthread_mutex_lock(&cursor_lock);
    bool toggled = (snapshot->buttons_down & SSRNX_BTN_MINUS) != 0;
    if (toggled) {
        cursor_enabled = !cursor_enabled;
        cursor_wait_release = raw_a;
    }
    snapshot->buttons &= ~SSRNX_BTN_MINUS;
    snapshot->buttons_down &= ~SSRNX_BTN_MINUS;
    snapshot->buttons_up &= ~SSRNX_BTN_MINUS;
    if (cursor_enabled) {
        float dt = isfinite(seconds) && seconds > 0.0 ? (float)fmin(seconds, 0.05) : 0.0f;
        cursor_x = fmaxf(0.0f, fminf(1279.0f, cursor_x + cursor_axis(snapshot->left_x) * 600.0f * dt));
        cursor_y = fmaxf(0.0f, fminf(719.0f, cursor_y - cursor_axis(snapshot->left_y) * 600.0f * dt));
        if (!raw_a) cursor_wait_release = false;
        /* A real finger owns the gesture until it lifts. */
        if (!cursor_wait_release && snapshot->controller_connected &&
            !snapshot->touch_count && raw_a) {
            snapshot->touch_count = 1;
            snapshot->touches[0] = (SsrnxTouchPoint){
                .id = 31, .x = cursor_x, .y = cursor_y,
                .diameter_x = 1.0f, .diameter_y = 1.0f
            };
        }
        snapshot->left_x = snapshot->left_y = 0.0f;
    }
    /* Release an Android A key on entry; suppress A until it lifts on exit. */
    if (cursor_enabled || toggled || cursor_wait_release) {
        snapshot->buttons &= ~SSRNX_BTN_A;
        snapshot->buttons_down &= ~SSRNX_BTN_A;
        snapshot->buttons_up = (snapshot->buttons_up & ~SSRNX_BTN_A) |
            (toggled && cursor_enabled && raw_a ? SSRNX_BTN_A : 0);
    }
    if (!raw_a) cursor_wait_release = false;
    pthread_mutex_unlock(&cursor_lock);
}

bool nx_cursor_position(float *x, float *y) {
    pthread_mutex_lock(&cursor_lock);
    bool enabled = cursor_enabled;
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
    pthread_mutex_unlock(&cursor_lock);
    return enabled;
}
