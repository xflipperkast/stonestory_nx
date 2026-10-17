/* runtime graphics. */
#define _GNU_SOURCE 1
#include "runtime.h"


/* ---- graphics ---- */
#pragma push_macro("SSRNX_CFG")
#pragma push_macro("SSRNX_EGL_GUEST_EXTENSION")
#pragma push_macro("SSRNX_MAX_EGL_DIAG_CONTEXTS")
#pragma push_macro("SSRNX_MAX_EGL_DIAG_SURFACES")
#pragma push_macro("SSRNX_MAX_TRACKED_EGL_DISPLAYS")
#pragma push_macro("SSRNX_MAX_VIRTUAL_PBUFFERS")
#pragma push_macro("EBOOL")
#pragma push_macro("EPTR")
#pragma push_macro("FADDR")
#pragma push_macro("GLADDR")
#pragma push_macro("GLDIAG_FORWARD")
#pragma push_macro("GLDIAG_MAX_FORMATS")
#pragma push_macro("GLDIAG_MAX_REPORTS")
#pragma push_macro("GLDIAG_VERTEX_LIMIT")
#pragma push_macro("STUB_TRUE")






#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static uint64_t graphics_g_ssrnx_present_count;
static SsrnxDisplayPlan graphics_g_presentation_plan;
static bool graphics_g_presentation_plan_valid;

#ifdef __SWITCH__
#include <switch.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <pthread.h>
#include <stdatomic.h>

#define EBOOL(x) ((uint32_t)((x) ? 1u : 0u))
#define EPTR(x) ((void *)(x))


static void cursor_rect(int x, int y, int w, int h) {
    glScissor(x, y, w, h);
    glClear(GL_COLOR_BUFFER_BIT);
}
static EGLBoolean cursor_window_swap(EGLDisplay display, EGLSurface surface) {
    static bool cursor_was_visible;
    float px, py;
    bool visible = nx_cursor_position(&px, &py);
    if (visible) {
        if (graphics_g_presentation_plan_valid &&
            graphics_g_presentation_plan.aspect == SSRNX_DISPLAY_LANDSCAPE &&
            graphics_g_presentation_plan.reverse) { px=1279.0f-px; py=719.0f-py; }
        EGLint width=0,height=0,version=2;
        eglQuerySurface(display,surface,EGL_WIDTH,&width);
        eglQuerySurface(display,surface,EGL_HEIGHT,&height);
        /* Switch Mesa leaves window-surface EGL dimensions at zero. */
        if (width<=0 || height<=0) {
            NWindow *window=nwindowGetDefault();
            if (window && nwindowIsValid(window)) {
                u32 native_width=0,native_height=0;
                nwindowGetDimensions(window,&native_width,&native_height);
                width=(EGLint)native_width; height=(EGLint)native_height;
            }
        }
        eglQueryContext(display,eglGetCurrentContext(),EGL_CONTEXT_CLIENT_VERSION,&version);
        if (width>0 && height>0) {
            GLint framebuffer=0,scissor[4]; GLfloat clear[4]; GLboolean mask[4];
            GLboolean enabled=glIsEnabled(GL_SCISSOR_TEST);
            GLboolean discard=version>=3 ? glIsEnabled(GL_RASTERIZER_DISCARD) : GL_FALSE;
            GLint draw_buffer=GL_BACK;
            GLenum target=version>=3 ? GL_DRAW_FRAMEBUFFER : GL_FRAMEBUFFER;
            glGetIntegerv(version>=3 ? GL_DRAW_FRAMEBUFFER_BINDING : GL_FRAMEBUFFER_BINDING,&framebuffer);
            glGetIntegerv(GL_SCISSOR_BOX,scissor);
            glGetFloatv(GL_COLOR_CLEAR_VALUE,clear);
            glGetBooleanv(GL_COLOR_WRITEMASK,mask);
            glBindFramebuffer(target,0);
            /* Unity's final pass can leave clear operations disabled. */
            if (version>=3) {
                glGetIntegerv(GL_DRAW_BUFFER0,&draw_buffer);
                const GLenum back=GL_BACK;
                glDrawBuffers(1,&back);
                glDisable(GL_RASTERIZER_DISCARD);
            }
            glEnable(GL_SCISSOR_TEST);
            glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
            int x=(int)(px*width/1280.0f),y=height-1-(int)(py*height/720.0f);
            glClearColor(0,0,0,1);
            cursor_rect(x-8,y-2,17,5); cursor_rect(x-2,y-8,5,17);
            glClearColor(1,1,1,1);
            cursor_rect(x-7,y,15,1); cursor_rect(x,y-7,1,15);
            if (version>=3) {
                const GLenum saved_buffer=(GLenum)draw_buffer;
                glDrawBuffers(1,&saved_buffer);
                if (discard) glEnable(GL_RASTERIZER_DISCARD);
            }
            glBindFramebuffer(target,(GLuint)framebuffer);
            glClearColor(clear[0],clear[1],clear[2],clear[3]);
            glColorMask(mask[0],mask[1],mask[2],mask[3]);
            glScissor(scissor[0],scissor[1],scissor[2],scissor[3]);
            if (!enabled) glDisable(GL_SCISSOR_TEST);
            if (!cursor_was_visible)
                SSRNX_LOGCI("CURSOR", "overlay surface=%p size=%dx%d GLES=%d framebuffer=%d discard=%d drawBuffer=0x%x",
                            (void *)surface,width,height,version,framebuffer,discard,(unsigned)draw_buffer);
        } else if (!cursor_was_visible) {
            SSRNX_LOGCW("CURSOR", "overlay has no drawable surface=%p size=%dx%d",(void *)surface,width,height);
        }
    }
    cursor_was_visible=visible;
    return eglSwapBuffers(display,surface);
}

static void graphics_presentation_window_reset(void) {
    NWindow *default_window = nwindowGetDefault();
    if (default_window && nwindowIsValid(default_window))
        (void)nwindowSetTransform(default_window, 0);
}

static NWindow *graphics_presentation_native_window(void) {
    return nwindowGetDefault();
}

static Framebuffer graphics_g_software_framebuffer;
static bool graphics_g_software_framebuffer_ready;
static pthread_mutex_t graphics_g_software_framebuffer_lock = PTHREAD_MUTEX_INITIALIZER;

static void graphics_software_framebuffer_reset(void) {
    pthread_mutex_lock(&graphics_g_software_framebuffer_lock);
    if (graphics_g_software_framebuffer_ready) framebufferClose(&graphics_g_software_framebuffer);
    graphics_g_software_framebuffer_ready = false;
    memset(&graphics_g_software_framebuffer, 0, sizeof(graphics_g_software_framebuffer));
    pthread_mutex_unlock(&graphics_g_software_framebuffer_lock);
}

bool ssrnx_graphics_present_rgba(const uint8_t *pixels, int32_t width, int32_t height, uint32_t stride) {
    if (!pixels || width <= 0 || height <= 0 || (uint32_t)width > UINT32_MAX / 4 || stride < (uint32_t)width * 4)
        return false;
    pthread_mutex_lock(&graphics_g_software_framebuffer_lock);
    const int32_t physical_width = graphics_g_presentation_plan_valid ? graphics_g_presentation_plan.physical_width : width;
    const int32_t physical_height = graphics_g_presentation_plan_valid ? graphics_g_presentation_plan.physical_height : height;
    if (!graphics_g_software_framebuffer_ready) {
        Result rc = framebufferCreate(&graphics_g_software_framebuffer, graphics_presentation_native_window(), physical_width, physical_height, PIXEL_FORMAT_RGBA_8888, 2);
        if (R_SUCCEEDED(rc)) {
            rc = framebufferMakeLinear(&graphics_g_software_framebuffer);
            if (R_FAILED(rc)) framebufferClose(&graphics_g_software_framebuffer);
        }
        if (R_FAILED(rc)) {
            SSRNX_LOGCE("CANVAS", "software framebuffer initialization failed rc=0x%x", (unsigned)rc);
            pthread_mutex_unlock(&graphics_g_software_framebuffer_lock);
            return false;
        }
        graphics_g_software_framebuffer_ready = true;
    }
    u32 host_stride = 0;
    uint8_t *out = framebufferBegin(&graphics_g_software_framebuffer, &host_stride);
    if (!out) { pthread_mutex_unlock(&graphics_g_software_framebuffer_lock); return false; }
    if (physical_width == width && physical_height == height &&
        (!graphics_g_presentation_plan_valid || graphics_g_presentation_plan.rotation == SSRNX_DISPLAY_ROTATION_0)) {
        for (int32_t y = 0; y < height; ++y) memcpy(out + (size_t)y * host_stride, pixels + (size_t)y * stride, (size_t)width * 4);
    } else {
        SsrnxDisplayPlan plan = graphics_g_presentation_plan;
        /* Landscape reverse is already applied by the NWindow transform. */
        if (plan.aspect == SSRNX_DISPLAY_LANDSCAPE) plan.reverse = false;
        for (int32_t y = 0; y < physical_height; ++y) for (int32_t x = 0; x < physical_width; ++x) {
            float lx = 0, ly = 0;
            uint8_t *dst = out + (size_t)y * host_stride + (size_t)x * 4;
            if (ssrnx_display_map_touch(&plan, (float)x + 0.5f, (float)y + 0.5f, &lx, &ly)) {
                const int32_t sx = (int32_t)(lx * width / plan.logical_width), sy = (int32_t)(ly * height / plan.logical_height);
                memcpy(dst, pixels + (size_t)sy * stride + (size_t)sx * 4, 4);
            } else { dst[0] = dst[1] = dst[2] = 0; dst[3] = 255; }
        }
    }
    framebufferEnd(&graphics_g_software_framebuffer);
    ++graphics_g_ssrnx_present_count;
    if (graphics_g_ssrnx_present_count == 1) SSRNX_LOGCI("CANVAS", "first software frame presented bitmap=%dx%d physical=%dx%d", width, height, physical_width, physical_height);
    pthread_mutex_unlock(&graphics_g_software_framebuffer_lock);
    return true;
}

bool ssrnx_graphics_configure_presentation(const SsrnxDisplayPlan *plan) {
    if (!plan || plan->logical_width <= 0 || plan->logical_height <= 0 ||
        plan->physical_width <= 0 || plan->physical_height <= 0)
        return false;

    graphics_software_framebuffer_reset();
    graphics_presentation_window_reset();
    graphics_g_presentation_plan = *plan;
    graphics_g_presentation_plan_valid = true;

    NWindow *window = nwindowGetDefault();
    if (!window || !nwindowIsValid(window)) return false;

    /* Landscape can still use the native compositor transform directly.  For
     * portrait, keep the one libnx-owned default NWindow completely intact.
     * SSRNX virtualizes Android's portrait default framebuffer inside EGL and
     * composites it into this physical 1280x720 surface at swap time.  This
     * avoids opening a second VI display/layer, which is not available in
     * ordinary hbmenu applet permissions on all systems. */
    const u32 transform = (plan->aspect == SSRNX_DISPLAY_LANDSCAPE && plan->reverse)
                        ? NATIVE_WINDOW_TRANSFORM_ROT_180 : 0;
    Result rc = nwindowSetTransform(window, transform);
    if (R_FAILED(rc)) {
        SSRNX_LOGCE("DISPLAY", "default NWindow transform failed rc=0x%x", (unsigned)rc);
        return false;
    }

    SSRNX_LOGCI("DISPLAY", "presentation %s logical=%dx%d physical=%dx%d content=%.1f,%.1f %.1fx%.1f reverse=%u backend=%s",
                plan->aspect == SSRNX_DISPLAY_PORTRAIT ? "portrait" : "landscape",
                plan->logical_width, plan->logical_height,
                plan->physical_width, plan->physical_height,
                plan->content_x, plan->content_y,
                plan->content_width, plan->content_height,
                plan->reverse ? 1u : 0u,
                plan->aspect == SSRNX_DISPLAY_PORTRAIT ? "egl-offscreen-compositor" : "default-nwindow");
    return true;
}

static void graphics_ssrnx_glGetIntegerv(GLenum pname, GLint *data);
static void graphics_ssrnx_glBindFramebuffer(GLenum target, GLuint framebuffer);

/* ---------- GL diagnostic shim ----------
 * Guest GL calls go straight to Mesa. A few entry points are routed through
 * these wrappers, which call Mesa unchanged and only observe results, so
 * "presents but nothing drawn" failures can be attributed from a log:
 * draw calls per frame, shader compile/link failures, incomplete
 * framebuffers, compressed texture formats and the strings the guest reads.
 * Logging is bounded so steady-state frames cost one atomic add per draw. */
#define GLDIAG_MAX_REPORTS 24u
static _Atomic uint32_t graphics_g_gldiag_draws;
static _Atomic uint32_t graphics_g_gldiag_shader_failures;
static _Atomic uint32_t graphics_g_gldiag_program_failures;
static _Atomic uint32_t graphics_g_gldiag_fbo_reports;
static _Atomic uint32_t graphics_g_gldiag_string_reports;
static _Atomic uint32_t graphics_g_gldiag_query_reports;
static _Atomic uint32_t graphics_g_gldiag_extension_reports;
static _Atomic uint32_t graphics_g_gldiag_proc_reports;
static _Atomic uint32_t graphics_g_gldiag_error_reports;
static _Atomic uint32_t graphics_g_gldiag_error_calls;
static _Atomic uint32_t graphics_g_gldiag_storage_reports;
#define GLDIAG_MAX_FORMATS 32u
static GLenum graphics_g_gldiag_formats[GLDIAG_MAX_FORMATS];
static pthread_mutex_t graphics_g_gldiag_lock = PTHREAD_MUTEX_INITIALIZER;

static void graphics_gldiag_note_format(const char *call, GLenum format, GLsizei w, GLsizei h) {
    /* "First use" is tracked per (call, format) so storage allocation and
     * data upload of the same format are reported separately. */
    const uint32_t key = (uint32_t)format ^ ((uint32_t)(unsigned char)call[2] << 24) ^ ((uint32_t)strlen(call) << 16);
    SSRNX_PERF_LOCK(&graphics_g_gldiag_lock);
    bool seen = false;
    size_t i = 0;
    for (; i < GLDIAG_MAX_FORMATS && graphics_g_gldiag_formats[i]; ++i)
        if (graphics_g_gldiag_formats[i] == key) { seen = true; break; }
    if (!seen && i < GLDIAG_MAX_FORMATS) graphics_g_gldiag_formats[i] = key;
    pthread_mutex_unlock(&graphics_g_gldiag_lock);
    if (!seen)
        SSRNX_LOGCI("GLDIAG", "%s format=0x%04x %dx%d (first use)", call, (unsigned)format,
                    (int)w, (int)h);
}

/* Error flags belong to the guest context. Diagnostics may observe the value
 * returned to the guest, but must not clear it at upload or swap boundaries. */
static GLenum graphics_ssrnx_glGetError(void) {
    const GLenum err = glGetError();
    if (atomic_fetch_add(&graphics_g_gldiag_error_calls, 1u) < 64u)
        SSRNX_LOGCI("GLPROBE", "glGetError error=0x%04x context=%p caller=%p",
                    (unsigned)err, (void *)eglGetCurrentContext(), __builtin_return_address(0));
    if (err != GL_NO_ERROR && atomic_fetch_add(&graphics_g_gldiag_error_reports, 1u) < GLDIAG_MAX_REPORTS)
        SSRNX_LOGCW("GLDIAG", "guest glGetError error=0x%04x context=%p caller=%p",
                    (unsigned)err, (void *)eglGetCurrentContext(), __builtin_return_address(0));
    return err;
}

static void graphics_ssrnx_glRenderbufferStorage(GLenum target, GLenum format, GLsizei width, GLsizei height) {
    glRenderbufferStorage(target, format, width, height);
    if (atomic_fetch_add(&graphics_g_gldiag_storage_reports, 1u) < 64u)
        SSRNX_LOGCI("GLPROBE", "glRenderbufferStorage target=0x%x format=0x%x width=%d height=%d context=%p caller=%p",
                    (unsigned)target, (unsigned)format, (int)width, (int)height,
                    (void *)eglGetCurrentContext(), __builtin_return_address(0));
}

static void graphics_ssrnx_glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum format,
                                                   GLsizei width, GLsizei height) {
    glRenderbufferStorageMultisample(target, samples, format, width, height);
    if (atomic_fetch_add(&graphics_g_gldiag_storage_reports, 1u) < 64u)
        SSRNX_LOGCI("GLPROBE", "glRenderbufferStorageMultisample target=0x%x samples=%d format=0x%x width=%d height=%d context=%p caller=%p",
                    (unsigned)target, (int)samples, (unsigned)format, (int)width, (int)height,
                    (void *)eglGetCurrentContext(), __builtin_return_address(0));
}

/* graphics shader diagnostics */
/* Mesa reports source:line(column); other GLES compilers use source:line:.
 * Respect GLSL #line so diagnostics show the expression the compiler saw. */
static void graphics_gldiag_shader_source_lines(unsigned shader, const char *source, const char *info) {
    struct { unsigned source, line; } locations[4];
    size_t count = 0;
    for (const char *p = info; *p && count < 4; ++p) {
        if (*p < '0' || *p > '9') continue;
        char *end;
        const unsigned long file = strtoul(p, &end, 10);
        if (*end != ':') { p = end - 1; continue; }
        const char *number = end + 1;
        const unsigned long line = strtoul(number, &end, 10);
        if (end == number || (*end != '(' && *end != ':') || file > INT_MAX || line > INT_MAX) continue;
        bool duplicate = false;
        for (size_t i = 0; i < count; ++i)
            if (locations[i].source == file && locations[i].line == line) duplicate = true;
        if (!duplicate) {
            locations[count].source = (unsigned)file;
            locations[count++].line = (unsigned)line;
        }
        p = end - 1;
    }
    if (!count) {
        SSRNX_LOGCW("GLDIAG", "shader=%u compiler supplied no recognized source location", shader);
        return;
    }
    unsigned file = 0, logical = 1, physical = 1, reports = 0;
    for (const char *p = source; *p; ++physical) {
        const char *end = strchr(p, '\n');
        if (!end) end = p + strlen(p);
        bool show = false;
        for (size_t i = 0; i < count; ++i) {
            const unsigned line = locations[i].line;
            if (file == locations[i].source && logical >= (line > 2 ? line - 2 : 0) &&
                (uint64_t)logical <= (uint64_t)line + 2) show = true;
        }
        if (show && reports++ == 20) {
            SSRNX_LOGCW("GLDIAG", "shader=%u source context truncated after 20 lines", shader);
            break;
        }
        if (show)
            SSRNX_LOGCW("GLDIAG", "shader=%u source=%u line=%u physical=%u: %.*s%s", shader,
                        file, logical, physical, (int)((end - p) > 768 ? 768 : end - p), p,
                        end - p > 768 ? " [line truncated]" : "");
        const char *directive = p;
        while (directive < end && (*directive == ' ' || *directive == '\t')) ++directive;
        bool reset = false;
        if (directive < end && *directive++ == '#') {
            while (directive < end && (*directive == ' ' || *directive == '\t')) ++directive;
            if (end - directive >= 5 && !strncmp(directive, "line", 4) &&
                (directive[4] == ' ' || directive[4] == '\t')) {
                char text[96];
                size_t length = (size_t)(end - directive);
                if (length < sizeof(text)) {
                    memcpy(text, directive, length); text[length] = 0;
                    unsigned next_line = 0, next_file = file;
                    const int fields = sscanf(text, "line %u %u", &next_line, &next_file);
                    if (fields >= 1 && next_line <= INT_MAX && next_file <= INT_MAX) {
                        logical = next_line; file = next_file; reset = true;
                    }
                }
            }
        }
        if (!reset && logical < UINT_MAX) ++logical;
        p = *end ? end + 1 : end;
    }
}

static void graphics_gldiag_shader_source_context(unsigned shader, const char *info) {
    GLint length = 0;
    glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &length);
    /* Diagnostic limits never change compilation or guest error results. */
    if (length <= 1 || length > 1024 * 1024) {
        SSRNX_LOGCW("GLDIAG", "shader=%u source context unavailable length=%d", shader, (int)length);
        return;
    }
    char *source = malloc((size_t)length);
    if (!source) return;
    GLsizei written = 0;
    glGetShaderSource(shader, length, &written, source);
    if (written >= 0 && written < length) {
        source[written] = 0;
        graphics_gldiag_shader_source_lines(shader, source, info);
    }
    free(source);
}



/* graphics shader compat */
/* Explicit compatibility mode: keep ESSL300, infer uint only for nonnegative
 * literal operands in integer index arithmetic, then use the real compiler. */
static bool graphics_shader_compat_retry(GLuint shader,const char *first_error) {
    const char *mode=getenv("APN64_SHADER_COMPAT");
    if ((mode && (!strcmp(mode,"0") || !strcmp(mode,"strict"))) ||
        !strstr(first_error,"implicitly convert operands")) return false;
    GLint length=0;glGetShaderiv(shader,GL_SHADER_SOURCE_LENGTH,&length);
    if (length<=1 || length>1024*1024) return false;
    char *original=malloc((size_t)length);
    if (!original) return false;
    GLsizei written=0;glGetShaderSource(shader,length,&written,original);
    size_t conversions=0;
    char *corrected=written>0 && written<length
        ? ssrnx_shader_compat_source(original,(size_t)written,&conversions) : NULL;
    if (!corrected) { free(original);return false; }
    const GLchar *text=corrected;
    glShaderSource(shader,1,&text,NULL);glCompileShader(shader);
    GLint compiled=GL_FALSE;glGetShaderiv(shader,GL_COMPILE_STATUS,&compiled);
    /* ShaderSource changes stored text, not the last compiled executable.
     * Restore source queries without retaining shader/context caches. */
    text=original;glShaderSource(shader,1,&text,NULL);
    if (compiled!=GL_TRUE) glCompileShader(shader); /* Preserve the original failure/log. */
    static unsigned reports;
    if (__atomic_fetch_add(&reports,1u,__ATOMIC_RELAXED)<64u)
        SSRNX_LOGCW("SHADERCOMPAT","shader=%u mode=uint-index-constants conversions=%zu compiled=%d sourceRestored=1",
                    shader,conversions,compiled==GL_TRUE);
    free(corrected);free(original);return compiled==GL_TRUE;
}



static void graphics_ssrnx_glCompileShader(GLuint shader) {
    glCompileShader(shader);
    GLint ok = GL_TRUE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == GL_TRUE) return;
    char info[768] = {0}, src[512] = {0};
    GLint type = 0;
    glGetShaderInfoLog(shader, (GLsizei)sizeof(info) - 1, NULL, info);
    if (graphics_shader_compat_retry(shader,info)) return;
    if (atomic_fetch_add(&graphics_g_gldiag_shader_failures,1u)>=GLDIAG_MAX_REPORTS) return;
    glGetShaderSource(shader, (GLsizei)sizeof(src) - 1, NULL, src);
    glGetShaderiv(shader, GL_SHADER_TYPE, &type);
    SSRNX_LOGCW("GLDIAG", "shader compile FAILED id=%u type=0x%x log=%s", shader, (unsigned)type, info);
    SSRNX_LOGCW("GLDIAG", "shader source head: %.400s", src);
    graphics_gldiag_shader_source_context(shader, info);
}

static void graphics_ssrnx_glLinkProgram(GLuint program) {
    glLinkProgram(program);
    GLint ok = GL_TRUE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok == GL_TRUE || atomic_fetch_add(&graphics_g_gldiag_program_failures, 1u) >= GLDIAG_MAX_REPORTS) return;
    char info[768] = {0};
    glGetProgramInfoLog(program, (GLsizei)sizeof(info) - 1, NULL, info);
    SSRNX_LOGCW("GLDIAG", "program link FAILED id=%u log=%s", program, info);
}

static GLenum graphics_ssrnx_glCheckFramebufferStatus(GLenum target) {
    const GLenum status = glCheckFramebufferStatus(target);
    if (status != GL_FRAMEBUFFER_COMPLETE && atomic_fetch_add(&graphics_g_gldiag_fbo_reports, 1u) < GLDIAG_MAX_REPORTS) {
        GLint bound = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound);
        SSRNX_LOGCW("GLDIAG", "framebuffer INCOMPLETE target=0x%x fbo=%d status=0x%x", (unsigned)target, (int)bound, (unsigned)status);
    }
    return status;
}

/* graphics texture diagnostics */
/* Read-only texture evidence. Do not inspect guest memory, consume GL errors,
 * change bindings or query GLES3-only state in a GLES2 context. */
static _Atomic uint32_t graphics_g_gldiag_texture_uploads;
static _Atomic uint32_t graphics_g_gldiag_texture_images;
static _Atomic uint32_t graphics_g_gldiag_texture_color_subimages;
static _Atomic uint32_t graphics_g_gldiag_texture_draw_reports;
static _Atomic uint64_t graphics_g_gldiag_texture_frame;

static void graphics_gldiag_texture_upload(const char *call, GLenum target, GLint level,
                                  GLsizei width, GLsizei height, GLint internalformat,
                                  GLenum format, GLenum type, const void *pixels) {
    const uint32_t sequence = atomic_fetch_add_explicit(&graphics_g_gldiag_texture_uploads, 1u, memory_order_relaxed) + 1u;
    if (!sequence) return;
    /* Glyph subimages must not consume the image-definition budget. */
    const bool image = strcmp(call, "glTexImage2D") == 0 || strcmp(call, "glCompressedTexImage2D") == 0 ||
                       strcmp(call, "glTexStorage2D") == 0;
    const bool color_subimage = !image && format != GL_ALPHA;
    const uint32_t kind_sequence = image ? atomic_fetch_add_explicit(&graphics_g_gldiag_texture_images, 1u, memory_order_relaxed) + 1u :
                                   color_subimage ? atomic_fetch_add_explicit(&graphics_g_gldiag_texture_color_subimages, 1u, memory_order_relaxed) + 1u : sequence;
    const uint32_t limit = image ? 256u : color_subimage ? 128u : 64u;
    if (!kind_sequence || (kind_sequence > limit && (kind_sequence & (kind_sequence - 1u)))) return;
    EGLContext context = eglGetCurrentContext();
    GLint texture = -1, unit = -1, alignment = -1, min_filter = -1, mag_filter = -1;
    if (context != EGL_NO_CONTEXT && target == GL_TEXTURE_2D) {
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &unit);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
        glGetTexParameteriv(target, GL_TEXTURE_MIN_FILTER, &min_filter);
        glGetTexParameteriv(target, GL_TEXTURE_MAG_FILTER, &mag_filter);
    }
    SSRNX_LOGCI("GLTEXTURE", "upload sequence=%u api=%s context=%p target=%#x texture=%d unit=%#x level=%d size=%dx%d internal=%#x format=%#x type=%#x pixels=%p unpackAlignment=%d min=%#x mag=%#x",
                sequence, call, (void *)context, (unsigned)target, texture, unit, level,
                width, height, (unsigned)internalformat, (unsigned)format, (unsigned)type,
                pixels, alignment, min_filter, mag_filter);
}

static void graphics_ssrnx_glTexImage2D(GLenum target, GLint level, GLint internalformat,
                              GLsizei width, GLsizei height, GLint border,
                              GLenum format, GLenum type, const void *pixels) {
    glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    graphics_gldiag_texture_upload("glTexImage2D", target, level, width, height, internalformat, format, type, pixels);
}

static void graphics_gldiag_texture_draw(void) {
    const uint64_t frame = atomic_load_explicit(&graphics_g_gldiag_texture_frame, memory_order_relaxed);
    if (frame % 300u || atomic_load_explicit(&graphics_g_gldiag_draws, memory_order_relaxed) > 4u ||
        atomic_load_explicit(&graphics_g_gldiag_texture_draw_reports, memory_order_relaxed) >= 128u) return;
    EGLContext context = eglGetCurrentContext();
    if (context == EGL_NO_CONTEXT) return;
    if (atomic_fetch_add_explicit(&graphics_g_gldiag_texture_draw_reports, 1u, memory_order_relaxed) >= 128u) return;
    GLint texture = 0, unit = 0, program = 0, min_filter = 0, mag_filter = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &unit);
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &min_filter);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &mag_filter);
    SSRNX_LOGCI("GLTEXTURE", "draw frame=%llu context=%p program=%d activeUnit=%#x activeTexture=%d min=%#x mag=%#x blend=%u depth=%u scissor=%u",
                (unsigned long long)frame, (void *)context, program, unit, texture,
                min_filter, mag_filter, (unsigned)glIsEnabled(GL_BLEND),
                (unsigned)glIsEnabled(GL_DEPTH_TEST), (unsigned)glIsEnabled(GL_SCISSOR_TEST));
}



static void graphics_ssrnx_glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                                         GLsizei height, GLint border, GLsizei image_size, const void *data) {
    glCompressedTexImage2D(target, level, internalformat, width, height, border, image_size, data);
    graphics_gldiag_note_format("glCompressedTexImage2D", internalformat, width, height);
    graphics_gldiag_texture_upload("glCompressedTexImage2D", target, level, width, height, internalformat, internalformat, 0, data);
}
static void graphics_ssrnx_glCompressedTexSubImage2D(GLenum target, GLint level, GLint x, GLint y, GLsizei width,
                                            GLsizei height, GLenum format, GLsizei image_size, const void *data) {
    glCompressedTexSubImage2D(target, level, x, y, width, height, format, image_size, data);
    graphics_gldiag_note_format("glCompressedTexSubImage2D", format, width, height);
}
static GLenum graphics_g_gldiag_sub_formats[GLDIAG_MAX_FORMATS];
static void graphics_ssrnx_glTexSubImage2D(GLenum target, GLint level, GLint x, GLint y, GLsizei width, GLsizei height,
                                  GLenum format, GLenum type, const void *pixels) {
    glTexSubImage2D(target, level, x, y, width, height, format, type, pixels);
    graphics_gldiag_texture_upload("glTexSubImage2D", target, level, width, height, 0, format, type, pixels);
    bool seen = false;
    size_t i = 0;
    SSRNX_PERF_LOCK(&graphics_g_gldiag_lock);
    for (; i < GLDIAG_MAX_FORMATS && graphics_g_gldiag_sub_formats[i]; ++i)
        if (graphics_g_gldiag_sub_formats[i] == format) { seen = true; break; }
    if (!seen && i < GLDIAG_MAX_FORMATS) graphics_g_gldiag_sub_formats[i] = format;
    pthread_mutex_unlock(&graphics_g_gldiag_lock);
    if (!seen)
        SSRNX_LOGCI("GLDIAG", "glTexSubImage2D format=0x%04x type=0x%04x %dx%d level=%d pbo=%d (first use)",
                    (unsigned)format, (unsigned)type, (int)width, (int)height, (int)level, pixels == NULL);
}
static void graphics_ssrnx_glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
    glTexStorage2D(target, levels, internalformat, width, height);
    graphics_gldiag_note_format("glTexStorage2D", internalformat, width, height);
    graphics_gldiag_texture_upload("glTexStorage2D", target, 0, width, height, internalformat, internalformat, 0, NULL);
}

static const GLubyte *graphics_ssrnx_glGetString(GLenum name) {
    const GLubyte *s = glGetString(name);
    if (atomic_fetch_add(&graphics_g_gldiag_string_reports, 1u) < 32u)
        SSRNX_LOGCI("GLDIAG", "glGetString(0x%x) context=%p caller=%p -> %.600s",
                    (unsigned)name, (void *)eglGetCurrentContext(), __builtin_return_address(0),
                    s ? (const char *)s : "(null)");
    if (name == GL_EXTENSIONS && s) {
        const uint32_t sequence = atomic_fetch_add(&graphics_g_gldiag_extension_reports, 1u);
        if (sequence < 4u) {
            const size_t length = strlen((const char *)s);
            for (size_t start = 0; start < length && start < 16384u; start += 512u) {
                const size_t count = length - start < 512u ? length - start : 512u;
                SSRNX_LOGCI("GLCAPS", "extensions sequence=%u context=%p caller=%p length=%zu start=%zu count=%zu text=[%.*s]",
                            sequence, (void *)eglGetCurrentContext(), __builtin_return_address(0),
                            length, start, count, (int)count, (const char *)s + start);
            }
            if (!length)
                SSRNX_LOGCI("GLCAPS", "extensions sequence=%u context=%p caller=%p length=0 start=0 count=0 text=[]",
                            sequence, (void *)eglGetCurrentContext(), __builtin_return_address(0));
        }
    }
    return s;
}

/* graphics vertex diagnostics */
/* Read-only snapshots before Mesa validates/draws. Never consume glGetError,
 * dereference client vertex/index data, or change GL bindings. */
#define GLDIAG_VERTEX_LIMIT 32u
static _Atomic uint32_t graphics_g_gldiag_vertex_draw_reports;
static _Atomic uint32_t graphics_g_gldiag_vertex_pointer_reports;
static _Atomic uint32_t graphics_g_gldiag_mesa_vertex_reports;
static _Atomic uint32_t graphics_g_gldiag_mesa_invalid_reports;

static void graphics_gldiag_vertex_draw(const char *call, GLenum mode, GLint first, GLsizei count,
                                GLenum type, const void *indices, GLsizei instances, void *caller) {
    const uint32_t sequence = atomic_fetch_add(&graphics_g_gldiag_vertex_draw_reports, 1u);
    if (sequence >= GLDIAG_MAX_REPORTS) return;
    EGLContext context = eglGetCurrentContext();
    SSRNX_LOGCI("GLVERTEX", "draw BEGIN sequence=%u api=%s context=%p caller=%p mode=%#x first=%d count=%d indexType=%#x indices=%p instances=%d",
                sequence, call, (void *)context, caller, (unsigned)mode, first, count,
                (unsigned)type, indices, instances);
    if (context == EGL_NO_CONTEXT) return;
    GLint program = 0, array = 0, element = 0, maximum = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &array);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &element);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maximum);
    SSRNX_LOGCI("GLVERTEX", "state sequence=%u program=%d arrayBuffer=%d elementBuffer=%d maxAttribs=%d",
                sequence, program, array, element, maximum);
    for (GLuint i = 0; i < (GLuint)(maximum > 0 ? maximum : 0) && i < GLDIAG_VERTEX_LIMIT; ++i) {
        GLint enabled = 0, size = 0, format = 0, normalized = 0, stride = 0, buffer = 0;
        void *pointer = NULL;
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, &format);
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &normalized);
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &buffer);
        glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &pointer);
        SSRNX_LOGCI("GLVERTEX", "attribute sequence=%u index=%u enabled=%d size=%d type=%#x normalized=%d stride=%d buffer=%d pointer=%p",
                    sequence, i, enabled, size, (unsigned)format, normalized, stride, buffer, pointer);
    }
    if (program) {
        GLint active = 0;
        glGetProgramiv((GLuint)program, GL_ACTIVE_ATTRIBUTES, &active);
        SSRNX_LOGCI("GLVERTEX", "program sequence=%u id=%d activeAttribs=%d", sequence, program, active);
        for (GLuint i = 0; i < (GLuint)(active > 0 ? active : 0) && i < GLDIAG_VERTEX_LIMIT; ++i) {
            char name[128] = {0}; GLsizei length = 0; GLint size = 0; GLenum format = 0;
            glGetActiveAttrib((GLuint)program, i, sizeof(name), &length, &size, &format, name);
            const bool truncated = length >= (GLsizei)sizeof(name) - 1;
            GLint location = truncated ? -1 : glGetAttribLocation((GLuint)program, name);
            SSRNX_LOGCI("GLVERTEX", "input sequence=%u index=%u location=%d size=%d type=%#x name=%.127s truncated=%d",
                        sequence, i, location, size, (unsigned)format, name, truncated ? 1 : 0);
        }
    }
}

static void graphics_ssrnx_glVertexAttribPointer(GLuint index, GLint size, GLenum type,
                                         GLboolean normalized, GLsizei stride, const void *pointer) {
    if (atomic_fetch_add(&graphics_g_gldiag_vertex_pointer_reports, 1u) < 64u)
        SSRNX_LOGCI("GLVERTEX", "pointer api=glVertexAttribPointer context=%p caller=%p index=%u size=%d type=%#x normalized=%u stride=%d pointer=%p",
                    (void *)eglGetCurrentContext(), __builtin_return_address(0), index, size,
                    (unsigned)type, (unsigned)normalized, stride, pointer);
    /* Mesa indexes its vertex-format table with this byte: canonicalize GL truth. */
    glVertexAttribPointer(index, size, type, normalized != GL_FALSE, stride, pointer);
}

static void graphics_ssrnx_glVertexAttribIPointer(GLuint index, GLint size, GLenum type,
                                          GLsizei stride, const void *pointer) {
    if (atomic_fetch_add(&graphics_g_gldiag_vertex_pointer_reports, 1u) < 64u)
        SSRNX_LOGCI("GLVERTEX", "pointer api=glVertexAttribIPointer context=%p caller=%p index=%u size=%d type=%#x stride=%d pointer=%p",
                    (void *)eglGetCurrentContext(), __builtin_return_address(0), index, size,
                    (unsigned)type, stride, pointer);
    glVertexAttribIPointer(index, size, type, stride, pointer);
}

/* Opaque pointer ABI only: the pinned Mesa archive's pipe_vertex_element is
 * eight bytes (offset:16, buffer:5, format:11, then divisor:32). Keep private
 * Mesa/newlib structure layouts out of SSRNX's compilation environment. */
#ifdef SSRNX_MESA_VERTEX_DIAGNOSTICS
struct util_format_description;
extern const struct util_format_description *util_format_description(unsigned format);
extern void *__real_nvc0_vertex_state_create(void *pipe, unsigned count, const void *elements);

void *__wrap_nvc0_vertex_state_create(void *pipe, unsigned count, const void *elements) {
    uint32_t records[GLDIAG_VERTEX_LIMIT][2] = {{0}};
    const unsigned captured = elements ? (count < GLDIAG_VERTEX_LIMIT ? count : GLDIAG_VERTEX_LIMIT) : 0;
    bool invalid = false;
    for (unsigned i = 0; i < captured; ++i) {
        memcpy(records[i], (const uint8_t *)elements + i * 8u, 8u);
        if (!util_format_description(records[i][0] >> 21)) invalid = true;
    }
    const uint32_t sequence = atomic_fetch_add(&graphics_g_gldiag_mesa_vertex_reports, 1u);
    if (sequence < GLDIAG_MAX_REPORTS ||
        (invalid && atomic_fetch_add(&graphics_g_gldiag_mesa_invalid_reports, 1u) < GLDIAG_MAX_REPORTS)) {
        SSRNX_LOGCI("GLVERTEX", "Mesa vertex BEGIN sequence=%u pipe=%p elements=%p count=%u captured=%u invalidFormat=%d",
                    sequence, pipe, elements, count, captured, invalid ? 1 : 0);
        for (unsigned i = 0; i < captured; ++i) {
            const uint32_t word = records[i][0], format = word >> 21;
            SSRNX_LOGCI("GLVERTEX", "Mesa element sequence=%u index=%u raw=%#x,%#x offset=%u buffer=%u format=%u divisor=%u described=%d",
                        sequence, i, word, records[i][1], word & 0xffffu, (word >> 16) & 31u,
                        format, records[i][1], util_format_description(format) ? 1 : 0);
        }
    }
    /* Forward even malformed input: diagnostics must not hide or repair it. */
    return __real_nvc0_vertex_state_create(pipe, count, elements);
}
#endif

static void graphics_gldiag_vertex_reset(void) {
    atomic_store(&graphics_g_gldiag_vertex_draw_reports, 0u);
    atomic_store(&graphics_g_gldiag_vertex_pointer_reports, 0u);
    atomic_store(&graphics_g_gldiag_mesa_vertex_reports, 0u);
    atomic_store(&graphics_g_gldiag_mesa_invalid_reports, 0u);
}



static void graphics_ssrnx_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    atomic_fetch_add_explicit(&graphics_g_gldiag_draws, 1u, memory_order_relaxed);
    graphics_gldiag_vertex_draw("glDrawArrays", mode, first, count, 0, NULL, 1, __builtin_return_address(0));
    graphics_gldiag_texture_draw();
    glDrawArrays(mode, first, count);
}
static void graphics_ssrnx_glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) {
    atomic_fetch_add_explicit(&graphics_g_gldiag_draws, 1u, memory_order_relaxed);
    graphics_gldiag_vertex_draw("glDrawElements", mode, 0, count, type, indices, 1, __builtin_return_address(0));
    graphics_gldiag_texture_draw();
    glDrawElements(mode, count, type, indices);
}
static void graphics_ssrnx_glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices) {
    atomic_fetch_add_explicit(&graphics_g_gldiag_draws, 1u, memory_order_relaxed);
    graphics_gldiag_vertex_draw("glDrawRangeElements", mode, 0, count, type, indices, 1, __builtin_return_address(0));
    graphics_gldiag_texture_draw();
    glDrawRangeElements(mode, start, end, count, type, indices);
}
static void graphics_ssrnx_glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instances) {
    atomic_fetch_add_explicit(&graphics_g_gldiag_draws, 1u, memory_order_relaxed);
    graphics_gldiag_vertex_draw("glDrawArraysInstanced", mode, first, count, 0, NULL, instances, __builtin_return_address(0));
    graphics_gldiag_texture_draw();
    glDrawArraysInstanced(mode, first, count, instances);
}
static void graphics_ssrnx_glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instances) {
    atomic_fetch_add_explicit(&graphics_g_gldiag_draws, 1u, memory_order_relaxed);
    graphics_gldiag_vertex_draw("glDrawElementsInstanced", mode, 0, count, type, indices, instances, __builtin_return_address(0));
    graphics_gldiag_texture_draw();
    glDrawElementsInstanced(mode, count, type, indices, instances);
}

/* GLES 3.1/3.2 draw entry points: resolved from Mesa on first use so the
 * count covers every draw path a guest can take. */
typedef void (*graphics_Pfn_DrawElementsBaseVertex)(GLenum, GLsizei, GLenum, const void *, GLint);
typedef void (*graphics_Pfn_DrawRangeElementsBaseVertex)(GLenum, GLuint, GLuint, GLsizei, GLenum, const void *, GLint);
typedef void (*graphics_Pfn_DrawElementsInstancedBaseVertex)(GLenum, GLsizei, GLenum, const void *, GLsizei, GLint);
typedef void (*graphics_Pfn_DrawArraysIndirect)(GLenum, const void *);
typedef void (*graphics_Pfn_DrawElementsIndirect)(GLenum, GLenum, const void *);
#define GLDIAG_FORWARD(pfn, name) \
    static pfn real; \
    if (!real) real = (pfn)eglGetProcAddress(name); \
    atomic_fetch_add_explicit(&graphics_g_gldiag_draws, 1u, memory_order_relaxed)
static void graphics_ssrnx_glDrawElementsBaseVertex(GLenum m, GLsizei c, GLenum t, const void *i, GLint b) {
    GLDIAG_FORWARD(graphics_Pfn_DrawElementsBaseVertex, "glDrawElementsBaseVertex");
    graphics_gldiag_vertex_draw("glDrawElementsBaseVertex", m, 0, c, t, i, 1, __builtin_return_address(0));
    if (real) real(m, c, t, i, b);
}
static void graphics_ssrnx_glDrawRangeElementsBaseVertex(GLenum m, GLuint s, GLuint e, GLsizei c, GLenum t, const void *i, GLint b) {
    GLDIAG_FORWARD(graphics_Pfn_DrawRangeElementsBaseVertex, "glDrawRangeElementsBaseVertex");
    graphics_gldiag_vertex_draw("glDrawRangeElementsBaseVertex", m, 0, c, t, i, 1, __builtin_return_address(0));
    if (real) real(m, s, e, c, t, i, b);
}
static void graphics_ssrnx_glDrawElementsInstancedBaseVertex(GLenum m, GLsizei c, GLenum t, const void *i, GLsizei n, GLint b) {
    GLDIAG_FORWARD(graphics_Pfn_DrawElementsInstancedBaseVertex, "glDrawElementsInstancedBaseVertex");
    graphics_gldiag_vertex_draw("glDrawElementsInstancedBaseVertex", m, 0, c, t, i, n, __builtin_return_address(0));
    if (real) real(m, c, t, i, n, b);
}
static void graphics_ssrnx_glDrawArraysIndirect(GLenum m, const void *ind) {
    GLDIAG_FORWARD(graphics_Pfn_DrawArraysIndirect, "glDrawArraysIndirect");
    /* Counts reside in the indirect buffer; diagnostics never read that data. */
    graphics_gldiag_vertex_draw("glDrawArraysIndirect", m, 0, -1, 0, ind, -1, __builtin_return_address(0));
    if (real) real(m, ind);
}
static void graphics_ssrnx_glDrawElementsIndirect(GLenum m, GLenum t, const void *ind) {
    GLDIAG_FORWARD(graphics_Pfn_DrawElementsIndirect, "glDrawElementsIndirect");
    graphics_gldiag_vertex_draw("glDrawElementsIndirect", m, 0, -1, t, ind, -1, __builtin_return_address(0));
    if (real) real(m, t, ind);
}

/* Per-frame summary at the existing swap log cadence. The GL error flag
 * is read only when the guest itself calls glGetError. */
static void graphics_gldiag_frame_report(uint64_t present) {
    const uint32_t draws = atomic_exchange_explicit(&graphics_g_gldiag_draws, 0u, memory_order_relaxed);
    atomic_store_explicit(&graphics_g_gldiag_texture_frame, present, memory_order_relaxed);
    if (present > 5u && (present % 120u) != 0u) return;
    GLint fbo = 0, viewport[4] = {0};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    glGetIntegerv(GL_VIEWPORT, viewport);
    SSRNX_LOGCI("GLDIAG", "frame=%llu draws=%u fbo=%d viewport=%d,%d %dx%d guestErrors=%u shaderFail=%u linkFail=%u",
                (unsigned long long)present, draws, (int)fbo, viewport[0], viewport[1], viewport[2], viewport[3],
                (unsigned)atomic_load(&graphics_g_gldiag_error_reports), (unsigned)atomic_load(&graphics_g_gldiag_shader_failures),
                (unsigned)atomic_load(&graphics_g_gldiag_program_failures));
}

typedef struct graphics_SsrnxGlDirectExport { const char *name; uintptr_t addr; const char *provider; } graphics_SsrnxGlDirectExport;
#define GLADDR(fn) ((uintptr_t)(void (*)(void))(fn))
static const graphics_SsrnxGlDirectExport graphics_k_gl_direct[] = {
 {"glRenderbufferStorage",GLADDR(graphics_ssrnx_glRenderbufferStorage),"libGLESv2.so"},
 {"glRenderbufferStorageMultisample",GLADDR(graphics_ssrnx_glRenderbufferStorageMultisample),"libGLESv3.so"},
 {"glGetString",GLADDR(graphics_ssrnx_glGetString),"libGLESv2.so"},{"glGetIntegerv",GLADDR(graphics_ssrnx_glGetIntegerv),"libGLESv2.so"},{"glGetError",GLADDR(graphics_ssrnx_glGetError),"libGLESv2.so"},
 {"glViewport",GLADDR(glViewport),"libGLESv2.so"},{"glClearColor",GLADDR(glClearColor),"libGLESv2.so"},{"glClear",GLADDR(glClear),"libGLESv2.so"},
 {"glCreateShader",GLADDR(glCreateShader),"libGLESv2.so"},{"glShaderSource",GLADDR(glShaderSource),"libGLESv2.so"},{"glCompileShader",GLADDR(graphics_ssrnx_glCompileShader),"libGLESv2.so"},{"glGetShaderiv",GLADDR(glGetShaderiv),"libGLESv2.so"},{"glGetShaderInfoLog",GLADDR(glGetShaderInfoLog),"libGLESv2.so"},
 {"glCreateProgram",GLADDR(glCreateProgram),"libGLESv2.so"},{"glAttachShader",GLADDR(glAttachShader),"libGLESv2.so"},{"glBindAttribLocation",GLADDR(glBindAttribLocation),"libGLESv2.so"},{"glLinkProgram",GLADDR(graphics_ssrnx_glLinkProgram),"libGLESv2.so"},{"glGetProgramiv",GLADDR(glGetProgramiv),"libGLESv2.so"},{"glGetProgramInfoLog",GLADDR(glGetProgramInfoLog),"libGLESv2.so"},{"glUseProgram",GLADDR(glUseProgram),"libGLESv2.so"},{"glGetAttribLocation",GLADDR(glGetAttribLocation),"libGLESv2.so"},{"glGetUniformLocation",GLADDR(glGetUniformLocation),"libGLESv2.so"},{"glUniform4f",GLADDR(glUniform4f),"libGLESv2.so"},
 {"glGenBuffers",GLADDR(glGenBuffers),"libGLESv2.so"},{"glBindBuffer",GLADDR(glBindBuffer),"libGLESv2.so"},{"glBufferData",GLADDR(glBufferData),"libGLESv2.so"},{"glEnableVertexAttribArray",GLADDR(glEnableVertexAttribArray),"libGLESv2.so"},{"glDisableVertexAttribArray",GLADDR(glDisableVertexAttribArray),"libGLESv2.so"},{"glVertexAttribPointer",GLADDR(graphics_ssrnx_glVertexAttribPointer),"libGLESv2.so"},{"glVertexAttribIPointer",GLADDR(graphics_ssrnx_glVertexAttribIPointer),"libGLESv3.so"},{"glDrawArrays",GLADDR(graphics_ssrnx_glDrawArrays),"libGLESv2.so"},{"glReadPixels",GLADDR(glReadPixels),"libGLESv2.so"},
 {"glDeleteBuffers",GLADDR(glDeleteBuffers),"libGLESv2.so"},{"glDeleteShader",GLADDR(glDeleteShader),"libGLESv2.so"},{"glDeleteProgram",GLADDR(glDeleteProgram),"libGLESv2.so"},{"glActiveTexture",GLADDR(glActiveTexture),"libGLESv2.so"},{"glGenTextures",GLADDR(glGenTextures),"libGLESv2.so"},{"glBindTexture",GLADDR(glBindTexture),"libGLESv2.so"},{"glTexParameteri",GLADDR(glTexParameteri),"libGLESv2.so"},{"glTexImage2D",GLADDR(graphics_ssrnx_glTexImage2D),"libGLESv2.so"},{"glDeleteTextures",GLADDR(glDeleteTextures),"libGLESv2.so"},
 {"glGenFramebuffers",GLADDR(glGenFramebuffers),"libGLESv2.so"},{"glBindFramebuffer",GLADDR(graphics_ssrnx_glBindFramebuffer),"libGLESv2.so"},{"glFramebufferTexture2D",GLADDR(glFramebufferTexture2D),"libGLESv2.so"},{"glCheckFramebufferStatus",GLADDR(graphics_ssrnx_glCheckFramebufferStatus),"libGLESv2.so"},{"glDeleteFramebuffers",GLADDR(glDeleteFramebuffers),"libGLESv2.so"},{"glFinish",GLADDR(glFinish),"libGLESv2.so"},{"glFlush",GLADDR(glFlush),"libGLESv2.so"},
 {"glGenVertexArrays",GLADDR(glGenVertexArrays),"libGLESv3.so"},{"glBindVertexArray",GLADDR(glBindVertexArray),"libGLESv3.so"},{"glDeleteVertexArrays",GLADDR(glDeleteVertexArrays),"libGLESv3.so"},
 {"glDrawElements",GLADDR(graphics_ssrnx_glDrawElements),"libGLESv2.so"},{"glDrawRangeElements",GLADDR(graphics_ssrnx_glDrawRangeElements),"libGLESv3.so"},{"glDrawArraysInstanced",GLADDR(graphics_ssrnx_glDrawArraysInstanced),"libGLESv3.so"},{"glDrawElementsInstanced",GLADDR(graphics_ssrnx_glDrawElementsInstanced),"libGLESv3.so"},
 {"glCompressedTexImage2D",GLADDR(graphics_ssrnx_glCompressedTexImage2D),"libGLESv2.so"},{"glCompressedTexSubImage2D",GLADDR(graphics_ssrnx_glCompressedTexSubImage2D),"libGLESv2.so"},{"glTexStorage2D",GLADDR(graphics_ssrnx_glTexStorage2D),"libGLESv3.so"},{"glTexSubImage2D",GLADDR(graphics_ssrnx_glTexSubImage2D),"libGLESv2.so"},
 {"glDrawElementsBaseVertex",GLADDR(graphics_ssrnx_glDrawElementsBaseVertex),"libGLESv3.so"},{"glDrawRangeElementsBaseVertex",GLADDR(graphics_ssrnx_glDrawRangeElementsBaseVertex),"libGLESv3.so"},{"glDrawElementsInstancedBaseVertex",GLADDR(graphics_ssrnx_glDrawElementsInstancedBaseVertex),"libGLESv3.so"},{"glDrawArraysIndirect",GLADDR(graphics_ssrnx_glDrawArraysIndirect),"libGLESv3.so"},{"glDrawElementsIndirect",GLADDR(graphics_ssrnx_glDrawElementsIndirect),"libGLESv3.so"},
 {"glDrawElementsBaseVertexEXT",GLADDR(graphics_ssrnx_glDrawElementsBaseVertex),"libGLESv3.so"},{"glDrawElementsBaseVertexOES",GLADDR(graphics_ssrnx_glDrawElementsBaseVertex),"libGLESv3.so"},{"glDrawElementsInstancedBaseVertexEXT",GLADDR(graphics_ssrnx_glDrawElementsInstancedBaseVertex),"libGLESv3.so"},{"glDrawElementsInstancedBaseVertexOES",GLADDR(graphics_ssrnx_glDrawElementsInstancedBaseVertex),"libGLESv3.so"}
};
static uintptr_t graphics_lookup_gl_direct(const char *name, const char **provider) {
    for (size_t i = 0; i < sizeof(graphics_k_gl_direct) / sizeof(graphics_k_gl_direct[0]); ++i) {
        if (strcmp(name, graphics_k_gl_direct[i].name) == 0) {
            if (provider) *provider = graphics_k_gl_direct[i].provider;
            return graphics_k_gl_direct[i].addr;
        }
    }
    return 0;
}

#define SSRNX_MAX_TRACKED_EGL_DISPLAYS 4
static EGLDisplay graphics_g_initialized_displays[SSRNX_MAX_TRACKED_EGL_DISPLAYS];

/*
 * The devkitPro Switch EGL driver intentionally exposes window-only configs
 * (EGL_WINDOW_BIT), but it also exposes EGL_KHR_surfaceless_context. Android
 * software commonly asks for EGL_PBUFFER_BIT configs for worker/offscreen
 * contexts.  Preserve that guest contract without inventing a second NWindow:
 * a virtual pbuffer is an SSRNX-owned opaque EGLSurface handle whose context is
 * made current surfaceless on the host.  The guest-facing surface handle and
 * dimensions remain observable through the EGL wrappers.
 *
 * This is deliberately a compatibility translation, not a fixture special
 * case.  It applies to any guest pbuffer request and keeps native window
 * ownership unchanged.
 */
#define SSRNX_MAX_VIRTUAL_PBUFFERS 32
typedef struct graphics_SsrnxVirtualPbuffer {
    bool in_use;
    EGLDisplay display;
    EGLConfig config;
    EGLint width;
    EGLint height;
} graphics_SsrnxVirtualPbuffer;

static graphics_SsrnxVirtualPbuffer graphics_g_virtual_pbuffers[SSRNX_MAX_VIRTUAL_PBUFFERS];
static pthread_mutex_t graphics_g_virtual_pbuffer_lock = PTHREAD_MUTEX_INITIALIZER;

static _Thread_local EGLDisplay graphics_g_guest_current_display;
static _Thread_local EGLContext graphics_g_guest_current_context;
static _Thread_local void *graphics_g_guest_current_draw;
static _Thread_local void *graphics_g_guest_current_read;
/* Guest-visible EGL error state. SSRNX virtualizes some EGL objects and also
 * consumes host errors for diagnostics. Keep eglGetError() semantics owned by
 * the Android compatibility layer rather than by those host details. */
static _Thread_local EGLint graphics_g_guest_egl_error = EGL_SUCCESS;

static void graphics_guest_egl_set_error(EGLint error) {
    if (error == EGL_SUCCESS) return;
    (void)eglGetError(); /* discard host residue for this failed guest call */
    graphics_g_guest_egl_error = error;
}

static EGLint graphics_guest_egl_capture_host_error(void) {
    EGLint error = eglGetError();
    if (error == EGL_SUCCESS) error = EGL_BAD_PARAMETER;
    graphics_g_guest_egl_error = error;
    return error;
}

static uint32_t graphics_guest_egl_bool(EGLBoolean ok) {
    if (ok != EGL_TRUE) (void)graphics_guest_egl_capture_host_error();
    return EBOOL(ok);
}


typedef struct graphics_SsrnxPortraitBridge {
    bool surface_active;
    bool resources_ready;
    bool source_probe_logged;
    bool physical_probe_logged;
    EGLDisplay display;
    EGLConfig config;
    EGLSurface surface;
    EGLContext guest_context;
    GLuint color_texture;
    GLuint depth_buffer;
    GLuint stencil_buffer;
    GLuint guest_fbo;
    GLuint compositor_program;
    GLuint compositor_vbo;
    GLint compositor_sampler;
} graphics_SsrnxPortraitBridge;

static graphics_SsrnxPortraitBridge graphics_g_portrait_bridge;

static bool graphics_portrait_plan_enabled(void) {
    return graphics_g_presentation_plan_valid && graphics_g_presentation_plan.aspect == SSRNX_DISPLAY_PORTRAIT;
}

static bool graphics_portrait_surface_is(const void *surface) {
    return graphics_portrait_plan_enabled() && graphics_g_portrait_bridge.surface_active &&
           surface && (EGLSurface)surface == graphics_g_portrait_bridge.surface;
}

static bool graphics_portrait_guest_context_current(void) {
    return graphics_g_portrait_bridge.resources_ready &&
           graphics_g_portrait_bridge.guest_context != EGL_NO_CONTEXT &&
           eglGetCurrentContext() == graphics_g_portrait_bridge.guest_context;
}

static void graphics_ssrnx_glBindFramebuffer(GLenum target, GLuint framebuffer) {
    if (graphics_portrait_guest_context_current() && framebuffer == 0) {
#ifdef GL_DRAW_FRAMEBUFFER
        if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
#else
        if (target == GL_FRAMEBUFFER) {
#endif
            glBindFramebuffer(target, graphics_g_portrait_bridge.guest_fbo);
            return;
        }
    }
    glBindFramebuffer(target, framebuffer);
}

static void graphics_ssrnx_glGetIntegerv(GLenum pname, GLint *data) {
    glGetIntegerv(pname, data);
    /* Only scalar capability queries; never drain the guest's GL error state. */
    switch (pname) {
        case 0x0d33: /* GL_MAX_TEXTURE_SIZE */
        case 0x84e8: /* GL_MAX_RENDERBUFFER_SIZE */
        case 0x8872: /* GL_MAX_TEXTURE_IMAGE_UNITS */
        case 0x8d57: /* GL_MAX_SAMPLES, forwarded only when the guest asks */
        case 0x8869: /* GL_MAX_VERTEX_ATTRIBS */
            if (data && eglGetCurrentContext() != EGL_NO_CONTEXT &&
                atomic_fetch_add(&graphics_g_gldiag_query_reports, 1u) < 64u)
                SSRNX_LOGCI("GLDIAG", "glGetIntegerv pname=0x%04x value=%d context=%p caller=%p",
                            (unsigned)pname, (int)*data, (void *)eglGetCurrentContext(), __builtin_return_address(0));
            break;
        default: break;
    }
    if (!data || !graphics_portrait_guest_context_current()) return;
#ifdef GL_DRAW_FRAMEBUFFER_BINDING
    if ((pname == GL_FRAMEBUFFER_BINDING || pname == GL_DRAW_FRAMEBUFFER_BINDING ||
         pname == GL_READ_FRAMEBUFFER_BINDING) && (GLuint)*data == graphics_g_portrait_bridge.guest_fbo) {
#else
    if (pname == GL_FRAMEBUFFER_BINDING && (GLuint)*data == graphics_g_portrait_bridge.guest_fbo) {
#endif
        *data = 0;
    }
}

static GLuint portrait_compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    if (!shader) return 0;
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        GLchar info[256]; GLsizei got = 0;
        glGetShaderInfoLog(shader, (GLsizei)sizeof(info), &got, info);
        SSRNX_LOGCE("DISPLAY", "portrait compositor shader compile failed type=0x%x log=%s",
                    (unsigned)type, got > 0 ? info : "(none)");
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint portrait_create_program(GLint *sampler_out) {
    static const char vs_source[] =
        "attribute vec2 aPos;\n"
        "attribute vec2 aUV;\n"
        "varying vec2 vUV;\n"
        "void main(){gl_Position=vec4(aPos,0.0,1.0);vUV=aUV;}\n";
    static const char fs_source[] =
        "precision mediump float;\n"
        "varying vec2 vUV;\n"
        "uniform sampler2D uTex;\n"
        "void main(){gl_FragColor=texture2D(uTex,vUV);}\n";
    GLuint vs = portrait_compile_shader(GL_VERTEX_SHADER, vs_source);
    if (!vs) return 0;
    GLuint fs = portrait_compile_shader(GL_FRAGMENT_SHADER, fs_source);
    if (!fs) { glDeleteShader(vs); return 0; }
    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glBindAttribLocation(program, 0, "aPos");
        glBindAttribLocation(program, 1, "aUV");
        glLinkProgram(program);
        GLint ok = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (ok != GL_TRUE) {
            GLchar info[256]; GLsizei got = 0;
            glGetProgramInfoLog(program, (GLsizei)sizeof(info), &got, info);
            SSRNX_LOGCE("DISPLAY", "portrait compositor program link failed log=%s",
                        got > 0 ? info : "(none)");
            glDeleteProgram(program);
            program = 0;
        } else if (sampler_out) {
            *sampler_out = glGetUniformLocation(program, "uTex");
        }
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

static void portrait_bridge_clear_metadata(void) {
    memset(&graphics_g_portrait_bridge, 0, sizeof(graphics_g_portrait_bridge));
}

/* Portrait resources live in the guest context itself.  This deliberately
 * avoids a second EGL context/share-group synchronization path: Switch Mesa
 * accepted the shared-context setup used by orientation-v2, but hardware
 * showed a permanently black sampled texture despite successful swaps. */
static void portrait_bridge_destroy_resources(void) {
    if (graphics_g_portrait_bridge.resources_ready &&
        graphics_g_portrait_bridge.guest_context != EGL_NO_CONTEXT &&
        eglGetCurrentContext() == graphics_g_portrait_bridge.guest_context) {
        if (graphics_g_portrait_bridge.compositor_vbo)
            glDeleteBuffers(1, &graphics_g_portrait_bridge.compositor_vbo);
        if (graphics_g_portrait_bridge.compositor_program)
            glDeleteProgram(graphics_g_portrait_bridge.compositor_program);
        if (graphics_g_portrait_bridge.guest_fbo)
            glDeleteFramebuffers(1, &graphics_g_portrait_bridge.guest_fbo);
        if (graphics_g_portrait_bridge.color_texture)
            glDeleteTextures(1, &graphics_g_portrait_bridge.color_texture);
        if (graphics_g_portrait_bridge.depth_buffer)
            glDeleteRenderbuffers(1, &graphics_g_portrait_bridge.depth_buffer);
        if (graphics_g_portrait_bridge.stencil_buffer)
            glDeleteRenderbuffers(1, &graphics_g_portrait_bridge.stencil_buffer);
    }
    graphics_g_portrait_bridge.resources_ready = false;
    graphics_g_portrait_bridge.color_texture = 0;
    graphics_g_portrait_bridge.depth_buffer = 0;
    graphics_g_portrait_bridge.stencil_buffer = 0;
    graphics_g_portrait_bridge.guest_fbo = 0;
    graphics_g_portrait_bridge.compositor_program = 0;
    graphics_g_portrait_bridge.compositor_vbo = 0;
    graphics_g_portrait_bridge.compositor_sampler = -1;
    graphics_g_portrait_bridge.guest_context = EGL_NO_CONTEXT;
    graphics_g_portrait_bridge.source_probe_logged = false;
    graphics_g_portrait_bridge.physical_probe_logged = false;
}

static bool portrait_bridge_adopt_window_surface(EGLDisplay display, EGLSurface surface,
                                                EGLConfig config) {
    if (!graphics_portrait_plan_enabled() || display == EGL_NO_DISPLAY ||
        surface == EGL_NO_SURFACE || !config) return false;
    if (graphics_g_portrait_bridge.surface_active && graphics_g_portrait_bridge.surface == surface) {
        graphics_g_portrait_bridge.display = display;
        graphics_g_portrait_bridge.config = config;
        return true;
    }

    /* Runtime orientation changes can switch the presentation plan after the
     * EGL window surface already exists. Adopt that existing Android window
     * lazily on the render thread instead of requiring a title-specific surface
     * recreation path. GL resources are then built in the current guest context. */
    portrait_bridge_destroy_resources();
    portrait_bridge_clear_metadata();
    graphics_g_portrait_bridge.surface_active = true;
    graphics_g_portrait_bridge.display = display;
    graphics_g_portrait_bridge.config = config;
    graphics_g_portrait_bridge.surface = surface;
    SSRNX_LOGCI("DISPLAY",
                "portrait bridge adopted existing window surface=%p logical=%dx%d",
                (void *)surface, graphics_g_presentation_plan.logical_width,
                graphics_g_presentation_plan.logical_height);
    return true;
}

static bool portrait_bridge_ensure(EGLDisplay display, EGLConfig config,
                                   EGLSurface surface, EGLContext guest_context) {
    if (!graphics_portrait_plan_enabled() || display == EGL_NO_DISPLAY || !config ||
        surface == EGL_NO_SURFACE || guest_context == EGL_NO_CONTEXT)
        return true;

    if (!graphics_g_portrait_bridge.surface_active || graphics_g_portrait_bridge.surface != surface) {
        SSRNX_LOGCE("DISPLAY", "portrait bridge surface metadata missing surface=%p", (void *)surface);
        return false;
    }
    if (graphics_g_portrait_bridge.resources_ready && graphics_g_portrait_bridge.guest_context == guest_context) {
        glBindFramebuffer(GL_FRAMEBUFFER, graphics_g_portrait_bridge.guest_fbo);
        return true;
    }
    if (graphics_g_portrait_bridge.resources_ready && graphics_g_portrait_bridge.guest_context != guest_context) {
        /* Old resources belong to another context.  Context destruction owns
         * their GL lifetime; forget the names rather than deleting them from
         * the replacement context. */
        portrait_bridge_destroy_resources();
    }

    GLint previous_fbo = 0;
    GLint previous_program = 0;
    GLint previous_array_buffer = 0;
    GLint previous_active_texture = GL_TEXTURE0;
    GLint previous_texture0 = 0;
    GLint previous_renderbuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_fbo);
    glGetIntegerv(GL_CURRENT_PROGRAM, &previous_program);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previous_array_buffer);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previous_active_texture);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &previous_renderbuffer);
    if (previous_active_texture != GL_TEXTURE0) glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture0);

    GLuint texture = 0, fbo = 0, vbo = 0, depth_buffer = 0, stencil_buffer = 0;
    GLint sampler = -1;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 (GLsizei)graphics_g_presentation_plan.logical_width,
                 (GLsizei)graphics_g_presentation_plan.logical_height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    /* The redirected default framebuffer must retain the window config's
     * depth/stencil storage. A complete color-only FBO silently disables
     * depth testing, letting later background geometry overwrite the scene. */
    EGLint depth = 0, stencil = 0;
    const bool config_ok = eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth) &&
                           eglGetConfigAttrib(display, config, EGL_STENCIL_SIZE, &stencil);
    if (config_ok && depth > 0) {
        glGenRenderbuffers(1, &depth_buffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
        const GLenum format = stencil > 0 ? (depth <= 24 ? GL_DEPTH24_STENCIL8 : GL_DEPTH32F_STENCIL8)
                              : depth <= 16 ? GL_DEPTH_COMPONENT16 :
                                depth <= 24 ? GL_DEPTH_COMPONENT24 : GL_DEPTH_COMPONENT32F;
        glRenderbufferStorage(GL_RENDERBUFFER, format,
                              graphics_g_presentation_plan.logical_width, graphics_g_presentation_plan.logical_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
        if (stencil > 0)
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
    }
    if (config_ok && stencil > 0 && depth == 0) {
        glGenRenderbuffers(1, &stencil_buffer);
        glBindRenderbuffer(GL_RENDERBUFFER, stencil_buffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8,
                              graphics_g_presentation_plan.logical_width, graphics_g_presentation_plan.logical_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencil_buffer);
    }
    glBindRenderbuffer(GL_RENDERBUFFER, (GLuint)previous_renderbuffer);
    if (!config_ok || !texture || !fbo || (depth > 0 && !depth_buffer) ||
        (stencil > 0 && !depth_buffer && !stencil_buffer) ||
        glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SSRNX_LOGCE("DISPLAY", "portrait offscreen framebuffer creation failed texture=%u fbo=%u",
                    (unsigned)texture, (unsigned)fbo);
        if (fbo) glDeleteFramebuffers(1, &fbo);
        if (texture) glDeleteTextures(1, &texture);
        if (depth_buffer) glDeleteRenderbuffers(1, &depth_buffer);
        if (stencil_buffer) glDeleteRenderbuffers(1, &stencil_buffer);
        glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)previous_fbo);
        glBindTexture(GL_TEXTURE_2D, (GLuint)previous_texture0);
        if (previous_active_texture != GL_TEXTURE0) glActiveTexture((GLenum)previous_active_texture);
        return false;
    }

    GLuint program = portrait_create_program(&sampler);
    if (!program) {
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &texture);
        if (depth_buffer) glDeleteRenderbuffers(1, &depth_buffer);
        if (stencil_buffer) glDeleteRenderbuffers(1, &stencil_buffer);
        glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)previous_fbo);
        glBindTexture(GL_TEXTURE_2D, (GLuint)previous_texture0);
        if (previous_active_texture != GL_TEXTURE0) glActiveTexture((GLenum)previous_active_texture);
        return false;
    }

    /* Interleaved x,y,u,v quad.  Keeping compositor geometry in a VBO avoids
     * driver-dependent client-array behavior on GLES2. */
    GLfloat quad[16];
    const GLfloat normal[16] = {
        -1.0f,-1.0f, 0.0f,0.0f,
         1.0f,-1.0f, 1.0f,0.0f,
        -1.0f, 1.0f, 0.0f,1.0f,
         1.0f, 1.0f, 1.0f,1.0f
    };
    /* Source texture quarter-turns. With OpenGL's bottom-left texture origin,
     * these UVs rotate the 720x1280 portrait texture across the complete
     * 1280x720 physical surface. */
    const GLfloat ccw90[16] = {
        -1.0f,-1.0f, 0.0f,1.0f,
         1.0f,-1.0f, 0.0f,0.0f,
        -1.0f, 1.0f, 1.0f,1.0f,
         1.0f, 1.0f, 1.0f,0.0f
    };
    const GLfloat cw90[16] = {
        -1.0f,-1.0f, 1.0f,0.0f,
         1.0f,-1.0f, 1.0f,1.0f,
        -1.0f, 1.0f, 0.0f,0.0f,
         1.0f, 1.0f, 0.0f,1.0f
    };
    const GLfloat *selected = normal;
    if (graphics_g_presentation_plan.rotation == SSRNX_DISPLAY_ROTATION_CCW_90) selected = ccw90;
    else if (graphics_g_presentation_plan.rotation == SSRNX_DISPLAY_ROTATION_CW_90) selected = cw90;
    memcpy(quad, selected, sizeof(quad));
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(quad), quad, GL_STATIC_DRAW);

    graphics_g_portrait_bridge.display = display;
    graphics_g_portrait_bridge.config = config;
    graphics_g_portrait_bridge.surface = surface;
    graphics_g_portrait_bridge.guest_context = guest_context;
    graphics_g_portrait_bridge.color_texture = texture;
    graphics_g_portrait_bridge.depth_buffer = depth_buffer;
    graphics_g_portrait_bridge.stencil_buffer = stencil_buffer;
    graphics_g_portrait_bridge.guest_fbo = fbo;
    graphics_g_portrait_bridge.compositor_program = program;
    graphics_g_portrait_bridge.compositor_vbo = vbo;
    graphics_g_portrait_bridge.compositor_sampler = sampler;
    graphics_g_portrait_bridge.resources_ready = true;

    /* Restore all setup state except the Android-visible default framebuffer,
     * which must now be redirected to the portrait offscreen FBO. */
    glUseProgram((GLuint)previous_program);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)previous_array_buffer);
    glBindTexture(GL_TEXTURE_2D, (GLuint)previous_texture0);
    if (previous_active_texture != GL_TEXTURE0) glActiveTexture((GLenum)previous_active_texture);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    SSRNX_LOGCI("DISPLAY", "portrait bridge ready logical=%dx%d physical=%dx%d texture=%u fbo=%u depth=%d stencil=%d backend=single-context-egl-rotated",
                graphics_g_presentation_plan.logical_width, graphics_g_presentation_plan.logical_height,
                graphics_g_presentation_plan.physical_width, graphics_g_presentation_plan.physical_height,
                (unsigned)texture, (unsigned)fbo, (int)depth, (int)stencil);
    return true;
}

typedef struct graphics_SsrnxSavedAttribState {
    GLint enabled;
    GLint size;
    GLint type;
    GLint normalized;
    GLint stride;
    GLint buffer_binding;
    void *pointer;
} graphics_SsrnxSavedAttribState;

typedef struct graphics_SsrnxPortraitGlState {
    GLint framebuffer;
    GLint viewport[4];
    GLint program;
    GLint array_buffer;
    GLint active_texture;
    GLint texture0_2d;
    GLfloat clear_color[4];
    GLboolean color_mask[4];
    GLboolean blend;
    GLboolean cull;
    GLboolean depth;
    GLboolean scissor;
    GLboolean stencil;
    graphics_SsrnxSavedAttribState attrib[2];
} graphics_SsrnxPortraitGlState;

static void portrait_save_attrib(GLuint index, graphics_SsrnxSavedAttribState *out) {
    memset(out, 0, sizeof(*out));
    glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &out->enabled);
    glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &out->size);
    glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &out->type);
    glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &out->normalized);
    glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &out->stride);
    glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &out->buffer_binding);
    glGetVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &out->pointer);
}

static void portrait_restore_attrib(GLuint index, const graphics_SsrnxSavedAttribState *s) {
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)s->buffer_binding);
    glVertexAttribPointer(index, s->size, (GLenum)s->type,
                          s->normalized ? GL_TRUE : GL_FALSE,
                          (GLsizei)s->stride, s->pointer);
    if (s->enabled) glEnableVertexAttribArray(index);
    else glDisableVertexAttribArray(index);
}

static void portrait_save_gl_state(graphics_SsrnxPortraitGlState *s) {
    memset(s, 0, sizeof(*s));
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &s->framebuffer);
    glGetIntegerv(GL_VIEWPORT, s->viewport);
    glGetIntegerv(GL_CURRENT_PROGRAM, &s->program);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &s->array_buffer);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &s->active_texture);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, s->clear_color);
    glGetBooleanv(GL_COLOR_WRITEMASK, s->color_mask);
    s->blend = glIsEnabled(GL_BLEND);
    s->cull = glIsEnabled(GL_CULL_FACE);
    s->depth = glIsEnabled(GL_DEPTH_TEST);
    s->scissor = glIsEnabled(GL_SCISSOR_TEST);
    s->stencil = glIsEnabled(GL_STENCIL_TEST);
    portrait_save_attrib(0, &s->attrib[0]);
    portrait_save_attrib(1, &s->attrib[1]);

    if (s->active_texture != GL_TEXTURE0) glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &s->texture0_2d);
    if (s->active_texture != GL_TEXTURE0) glActiveTexture((GLenum)s->active_texture);
}

static void portrait_restore_enable(GLenum cap, GLboolean enabled) {
    if (enabled) glEnable(cap); else glDisable(cap);
}

static void portrait_restore_gl_state(const graphics_SsrnxPortraitGlState *s) {
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)s->framebuffer);
    glViewport(s->viewport[0], s->viewport[1], s->viewport[2], s->viewport[3]);
    glClearColor(s->clear_color[0], s->clear_color[1], s->clear_color[2], s->clear_color[3]);
    glColorMask(s->color_mask[0], s->color_mask[1], s->color_mask[2], s->color_mask[3]);
    portrait_restore_enable(GL_BLEND, s->blend);
    portrait_restore_enable(GL_CULL_FACE, s->cull);
    portrait_restore_enable(GL_DEPTH_TEST, s->depth);
    portrait_restore_enable(GL_SCISSOR_TEST, s->scissor);
    portrait_restore_enable(GL_STENCIL_TEST, s->stencil);
    glUseProgram((GLuint)s->program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (GLuint)s->texture0_2d);
    if (s->active_texture != GL_TEXTURE0) glActiveTexture((GLenum)s->active_texture);

    portrait_restore_attrib(0, &s->attrib[0]);
    portrait_restore_attrib(1, &s->attrib[1]);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)s->array_buffer);
}

static EGLBoolean portrait_bridge_present(EGLDisplay display, EGLSurface surface) {
    if (!graphics_portrait_surface_is(surface) || !graphics_g_portrait_bridge.resources_ready)
        return cursor_window_swap(display, surface);
    if (graphics_g_guest_current_context != graphics_g_portrait_bridge.guest_context ||
        graphics_g_guest_current_draw != (void *)surface ||
        eglGetCurrentContext() != graphics_g_portrait_bridge.guest_context) {
        SSRNX_LOGCE("DISPLAY", "portrait swap without expected guest current context");
        return EGL_FALSE;
    }

    graphics_SsrnxPortraitGlState saved;
    portrait_save_gl_state(&saved);

    /* A one-shot hardware probe makes future black-frame failures actionable:
     * it tells us whether the guest actually produced pixels before SSRNX
     * composites them, without changing the steady-state rendering path. */
    if (!graphics_g_portrait_bridge.source_probe_logged) {
        GLubyte px[4] = {0,0,0,0};
        glBindFramebuffer(GL_FRAMEBUFFER, graphics_g_portrait_bridge.guest_fbo);
        glReadPixels(graphics_g_presentation_plan.logical_width / 2,
                     graphics_g_presentation_plan.logical_height / 2,
                     1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
        SSRNX_LOGCI("DISPLAY", "portrait source probe center rgba=%u,%u,%u,%u",
                    (unsigned)px[0], (unsigned)px[1], (unsigned)px[2], (unsigned)px[3]);
        graphics_g_portrait_bridge.source_probe_logged = true;
    }

    /* Same-context composition: no EGL context switch and no cross-context
     * shared-texture synchronization.  The guest's virtual default framebuffer
     * texture is sampled directly by the context that just rendered it. */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, (GLsizei)graphics_g_presentation_plan.physical_width,
               (GLsizei)graphics_g_presentation_plan.physical_height);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    GLint content_w = (GLint)(graphics_g_presentation_plan.content_width + 0.5f);
    GLint content_h = (GLint)(graphics_g_presentation_plan.content_height + 0.5f);
    GLint content_x = (GLint)(graphics_g_presentation_plan.content_x + 0.5f);
    GLint content_y = (GLint)(graphics_g_presentation_plan.content_y + 0.5f);
    glViewport(content_x, content_y, content_w, content_h);

    glUseProgram(graphics_g_portrait_bridge.compositor_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, graphics_g_portrait_bridge.color_texture);
    if (graphics_g_portrait_bridge.compositor_sampler >= 0)
        glUniform1i(graphics_g_portrait_bridge.compositor_sampler, 0);
    glBindBuffer(GL_ARRAY_BUFFER, graphics_g_portrait_bridge.compositor_vbo);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * (GLsizei)sizeof(GLfloat), (const void *)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * (GLsizei)sizeof(GLfloat), (const void *)(2 * sizeof(GLfloat)));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (!graphics_g_portrait_bridge.physical_probe_logged) {
        GLubyte px[4] = {0,0,0,0};
        glReadPixels(graphics_g_presentation_plan.physical_width / 2,
                     graphics_g_presentation_plan.physical_height / 2,
                     1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
        SSRNX_LOGCI("DISPLAY", "portrait physical probe center rgba=%u,%u,%u,%u",
                    (unsigned)px[0], (unsigned)px[1], (unsigned)px[2], (unsigned)px[3]);
        graphics_g_portrait_bridge.physical_probe_logged = true;
    }

    EGLBoolean ok = cursor_window_swap(display, surface);
    portrait_restore_gl_state(&saved);
    return ok;
}

/* HWFix6 EGL object metadata. HWFix6 keeps the diagnostic snapshots from the
 * previous probe build and also uses the tracked window->config relationship to
 * preserve config compatibility when a guest re-queries a window config (for
 * example when upgrading from an ES2 to an ES3 context). */
#define SSRNX_MAX_EGL_DIAG_SURFACES 8
#define SSRNX_MAX_EGL_DIAG_CONTEXTS 32
typedef struct graphics_SsrnxEglDiagSurface {
    bool in_use;
    EGLDisplay display;
    EGLSurface surface;
    EGLConfig config;
    EGLint width;
    EGLint height;
    bool is_window;
} graphics_SsrnxEglDiagSurface;
typedef struct graphics_SsrnxEglDiagContext {
    bool in_use;
    EGLDisplay display;
    EGLContext context;
    EGLConfig config;
    EGLint requested_client_version;
} graphics_SsrnxEglDiagContext;

typedef struct graphics_SsrnxEglConfigDiag {
    EGLint config_id;
    EGLint surface_type;
    EGLint renderable_type;
    EGLint conformant;
    EGLint red, green, blue, alpha;
    EGLint depth, stencil;
    EGLint sample_buffers, samples;
    EGLint color_buffer_type;
    EGLint caveat;
} graphics_SsrnxEglConfigDiag;

static graphics_SsrnxEglDiagSurface g_egl_diag_surfaces[SSRNX_MAX_EGL_DIAG_SURFACES];
static graphics_SsrnxEglDiagContext g_egl_diag_contexts[SSRNX_MAX_EGL_DIAG_CONTEXTS];
static pthread_mutex_t g_egl_diag_lock = PTHREAD_MUTEX_INITIALIZER;

static EGLint attrib_list_get(const EGLint *attribs, EGLint key, EGLint fallback) {
    if (!attribs) return fallback;
    for (size_t pairs = 0; pairs < 64; ++pairs) {
        EGLint k = attribs[pairs * 2];
        if (k == EGL_NONE) break;
        if (k == key) return attribs[pairs * 2 + 1];
    }
    return fallback;
}

static const char *egl_error_name(EGLint error) {
    switch (error) {
        case EGL_SUCCESS: return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
        case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
        case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
        case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
        case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
#ifdef EGL_CONTEXT_LOST
        case EGL_CONTEXT_LOST: return "EGL_CONTEXT_LOST";
#endif
        default: return "EGL_UNKNOWN_ERROR";
    }
}

static bool read_config_diag(EGLDisplay display, EGLConfig config, graphics_SsrnxEglConfigDiag *out) {
    if (!out || display == EGL_NO_DISPLAY || !config) return false;
    memset(out, 0, sizeof(*out));
#define SSRNX_CFG(attr, field) do { if (eglGetConfigAttrib(display, config, (attr), &out->field) != EGL_TRUE) return false; } while (0)
    SSRNX_CFG(EGL_CONFIG_ID, config_id);
    SSRNX_CFG(EGL_SURFACE_TYPE, surface_type);
    SSRNX_CFG(EGL_RENDERABLE_TYPE, renderable_type);
    SSRNX_CFG(EGL_CONFORMANT, conformant);
    SSRNX_CFG(EGL_RED_SIZE, red);
    SSRNX_CFG(EGL_GREEN_SIZE, green);
    SSRNX_CFG(EGL_BLUE_SIZE, blue);
    SSRNX_CFG(EGL_ALPHA_SIZE, alpha);
    SSRNX_CFG(EGL_DEPTH_SIZE, depth);
    SSRNX_CFG(EGL_STENCIL_SIZE, stencil);
    SSRNX_CFG(EGL_SAMPLE_BUFFERS, sample_buffers);
    SSRNX_CFG(EGL_SAMPLES, samples);
    SSRNX_CFG(EGL_COLOR_BUFFER_TYPE, color_buffer_type);
    SSRNX_CFG(EGL_CONFIG_CAVEAT, caveat);
#undef SSRNX_CFG
    return true;
}

static void log_config_diag(const char *label, EGLDisplay display, EGLConfig config) {
    graphics_SsrnxEglConfigDiag x;
    if (!read_config_diag(display, config, &x)) {
        SSRNX_LOGCW("EGL-DIAG", "%s config=%p: attribute snapshot failed", label, (void *)config);
        return;
    }
    SSRNX_LOGCI("EGL-DIAG", "%s config=%p id=%d surface=0x%x renderable=0x%x conformant=0x%x caveat=0x%x",
                label, (void *)config, (int)x.config_id, (unsigned)x.surface_type,
                (unsigned)x.renderable_type, (unsigned)x.conformant, (unsigned)x.caveat);
    SSRNX_LOGCI("EGL-DIAG", "%s rgba=%d/%d/%d/%d depth=%d stencil=%d samples=%d buffers=%d colorType=0x%x",
                label, (int)x.red, (int)x.green, (int)x.blue, (int)x.alpha,
                (int)x.depth, (int)x.stencil, (int)x.samples, (int)x.sample_buffers,
                (unsigned)x.color_buffer_type);
}

static void log_config_compatibility(EGLDisplay display, EGLConfig surface_config, EGLConfig context_config) {
    graphics_SsrnxEglConfigDiag surf, ctx;
    if (!read_config_diag(display, surface_config, &surf) ||
        !read_config_diag(display, context_config, &ctx)) {
        SSRNX_LOGCW("EGL-DIAG", "GLES3 config compatibility snapshot failed surfaceConfig=%p contextConfig=%p",
                    (void *)surface_config, (void *)context_config);
        return;
    }
    bool same_rgba = surf.red == ctx.red && surf.green == ctx.green && surf.blue == ctx.blue && surf.alpha == ctx.alpha;
    bool same_depth_stencil = surf.depth == ctx.depth && surf.stencil == ctx.stencil;
    bool same_samples = surf.sample_buffers == ctx.sample_buffers && surf.samples == ctx.samples;
    SSRNX_LOGCI("EGL-DIAG", "GLES3 config compare surfaceId=%d contextId=%d sameId=%u sameRGBA=%u sameDepthStencil=%u sameSamples=%u",
                (int)surf.config_id, (int)ctx.config_id, surf.config_id == ctx.config_id ? 1u : 0u,
                same_rgba ? 1u : 0u, same_depth_stencil ? 1u : 0u, same_samples ? 1u : 0u);
    SSRNX_LOGCI("EGL-DIAG", "GLES3 capability compare surfaceWindow=%u surfaceES3=%u contextES3=%u surfaceRenderable=0x%x contextRenderable=0x%x",
                (surf.surface_type & EGL_WINDOW_BIT) ? 1u : 0u,
                (surf.renderable_type & EGL_OPENGL_ES3_BIT_KHR) ? 1u : 0u,
                (ctx.renderable_type & EGL_OPENGL_ES3_BIT_KHR) ? 1u : 0u,
                (unsigned)surf.renderable_type, (unsigned)ctx.renderable_type);
}

static void remember_diag_surface(EGLDisplay display, EGLSurface surface, EGLConfig config,
                                  EGLint width, EGLint height, bool is_window) {
    if (display == EGL_NO_DISPLAY || surface == EGL_NO_SURFACE || !config) return;
    SSRNX_PERF_LOCK(&g_egl_diag_lock);
    for (size_t i = 0; i < SSRNX_MAX_EGL_DIAG_SURFACES; ++i) {
        if (g_egl_diag_surfaces[i].in_use && g_egl_diag_surfaces[i].surface == surface) {
            g_egl_diag_surfaces[i].display = display;
            g_egl_diag_surfaces[i].config = config;
            g_egl_diag_surfaces[i].width = width;
            g_egl_diag_surfaces[i].height = height;
            g_egl_diag_surfaces[i].is_window = is_window;
            pthread_mutex_unlock(&g_egl_diag_lock);
            return;
        }
    }
    for (size_t i = 0; i < SSRNX_MAX_EGL_DIAG_SURFACES; ++i) {
        if (!g_egl_diag_surfaces[i].in_use) {
            g_egl_diag_surfaces[i].in_use = true;
            g_egl_diag_surfaces[i].display = display;
            g_egl_diag_surfaces[i].surface = surface;
            g_egl_diag_surfaces[i].config = config;
            g_egl_diag_surfaces[i].width = width;
            g_egl_diag_surfaces[i].height = height;
            g_egl_diag_surfaces[i].is_window = is_window;
            break;
        }
    }
    pthread_mutex_unlock(&g_egl_diag_lock);
}

static bool snapshot_diag_surface(EGLSurface surface, graphics_SsrnxEglDiagSurface *out) {
    bool found = false;
    SSRNX_PERF_LOCK(&g_egl_diag_lock);
    for (size_t i = 0; i < SSRNX_MAX_EGL_DIAG_SURFACES; ++i) {
        if (g_egl_diag_surfaces[i].in_use && g_egl_diag_surfaces[i].surface == surface) {
            if (out) *out = g_egl_diag_surfaces[i];
            found = true;
            break;
        }
    }
    pthread_mutex_unlock(&g_egl_diag_lock);
    return found;
}

/* Determine whether a specific native EGLConfig is genuinely one of the
 * configs selected by the guest's ORIGINAL eglChooseConfig attribute list.
 *
 * Do not append EGL_CONFIG_ID to the request: EGL specifies that once
 * EGL_CONFIG_ID is present, every other selection attribute is ignored.  The
 * HWFix6 probe did exactly that, so its host fixture could claim a config was a
 * valid match without testing the guest's actual requirements.
 *
 * Instead, delegate all matching/default/mask/minimum rules to host EGL twice:
 * first obtain the total match count, then enumerate that exact result set and
 * look for the active window's config handle.  This mirrors the host's own
 * matching semantics and avoids reimplementing Table 3.4 in SSRNX. */
static bool config_is_in_guest_choose_results(EGLDisplay display, EGLConfig target,
                                               const EGLint *attribs,
                                               EGLint *out_total, EGLint *out_index) {
    if (out_total) *out_total = 0;
    if (out_index) *out_index = -1;
    if (display == EGL_NO_DISPLAY || !target || !attribs) return false;

    EGLint total = 0;
    if (eglChooseConfig(display, attribs, NULL, 0, &total) != EGL_TRUE || total <= 0) {
        /* This is an internal compatibility probe. Do not leak a probe-only EGL
         * error after the guest's original eglChooseConfig already succeeded. */
        (void)eglGetError();
        return false;
    }
    if (out_total) *out_total = total;

    enum { SSRNX_STACK_CONFIGS = 16 };
    EGLConfig stack_configs[SSRNX_STACK_CONFIGS];
    EGLConfig *matches = stack_configs;
    if (total > SSRNX_STACK_CONFIGS) {
        matches = (EGLConfig *)malloc(sizeof(*matches) * (size_t)total);
        if (!matches) return false;
    }

    EGLint got = 0;
    EGLBoolean ok = eglChooseConfig(display, attribs, matches, total, &got);
    if (ok != EGL_TRUE) {
        (void)eglGetError();
        if (matches != stack_configs) free(matches);
        return false;
    }

    bool found = false;
    for (EGLint i = 0; i < got; ++i) {
        if (matches[i] == target) {
            found = true;
            if (out_index) *out_index = i;
            break;
        }
    }

    if (matches != stack_configs) free(matches);
    return found;
}

static bool promote_active_window_config(EGLDisplay display, const EGLint *attribs,
                                         EGLConfig *configs, EGLint config_size, EGLint *num_config) {
    if (display == EGL_NO_DISPLAY || !attribs || !configs || config_size <= 0 ||
        !num_config || *num_config <= 0) return false;

    EGLint requested_surface = attrib_list_get(attribs, EGL_SURFACE_TYPE, 0);
    if (!(requested_surface & EGL_WINDOW_BIT) || (requested_surface & EGL_PBUFFER_BIT)) return false;

    EGLSurface active = EGL_NO_SURFACE;
    if (graphics_g_guest_current_display == display && graphics_g_guest_current_context != EGL_NO_CONTEXT && graphics_g_guest_current_draw)
        active = (EGLSurface)graphics_g_guest_current_draw;
    else if (eglGetCurrentDisplay() == display)
        active = eglGetCurrentSurface(EGL_DRAW);

    graphics_SsrnxEglDiagSurface surface_meta;
    if (active == EGL_NO_SURFACE || !snapshot_diag_surface(active, &surface_meta) ||
        surface_meta.display != display || !surface_meta.config) return false;

    EGLint match_total = 0, match_index = -1;
    bool active_matches = config_is_in_guest_choose_results(display, surface_meta.config, attribs,
                                                            &match_total, &match_index);
    EGLint active_id = -1;
    (void)eglGetConfigAttrib(display, surface_meta.config, EGL_CONFIG_ID, &active_id);
    SSRNX_LOGCI("EGL", "eglChooseConfig: active-window candidate surface=%p config=%p id=%d matches=%u matchIndex=%d totalMatches=%d",
                (void *)active, (void *)surface_meta.config, (int)active_id,
                active_matches ? 1u : 0u, (int)match_index, (int)match_total);
    if (!active_matches) return false;

    if (configs[0] == surface_meta.config) return false;

    EGLConfig displaced = configs[0];
    EGLint returned = *num_config < config_size ? *num_config : config_size;
    EGLint existing = -1;
    for (EGLint i = 1; i < returned; ++i) {
        if (configs[i] == surface_meta.config) { existing = i; break; }
    }
    if (existing >= 0) configs[existing] = displaced;
    configs[0] = surface_meta.config;

    EGLint config_id = -1;
    (void)eglGetConfigAttrib(display, surface_meta.config, EGL_CONFIG_ID, &config_id);
    SSRNX_LOGCI("EGL", "eglChooseConfig: active-window compatibility promotion surface=%p config=%p id=%d displaced=%p",
                (void *)active, (void *)surface_meta.config, (int)config_id, (void *)displaced);
    return true;
}

static void forget_diag_surface(EGLSurface surface) {
    ssrnx_perf_forget_surface((uintptr_t)surface);
    SSRNX_PERF_LOCK(&g_egl_diag_lock);
    for (size_t i = 0; i < SSRNX_MAX_EGL_DIAG_SURFACES; ++i)
        if (g_egl_diag_surfaces[i].in_use && g_egl_diag_surfaces[i].surface == surface)
            memset(&g_egl_diag_surfaces[i], 0, sizeof(g_egl_diag_surfaces[i]));
    pthread_mutex_unlock(&g_egl_diag_lock);
}

static void remember_diag_context(EGLDisplay display, EGLContext context, EGLConfig config, EGLint version) {
    if (display == EGL_NO_DISPLAY || context == EGL_NO_CONTEXT || !config) return;
    SSRNX_PERF_LOCK(&g_egl_diag_lock);
    for (size_t i = 0; i < SSRNX_MAX_EGL_DIAG_CONTEXTS; ++i) {
        if (g_egl_diag_contexts[i].in_use && g_egl_diag_contexts[i].context == context) {
            g_egl_diag_contexts[i].display = display;
            g_egl_diag_contexts[i].config = config;
            g_egl_diag_contexts[i].requested_client_version = version;
            pthread_mutex_unlock(&g_egl_diag_lock);
            return;
        }
    }
    for (size_t i = 0; i < SSRNX_MAX_EGL_DIAG_CONTEXTS; ++i) {
        if (!g_egl_diag_contexts[i].in_use) {
            g_egl_diag_contexts[i].in_use = true;
            g_egl_diag_contexts[i].display = display;
            g_egl_diag_contexts[i].context = context;
            g_egl_diag_contexts[i].config = config;
            g_egl_diag_contexts[i].requested_client_version = version;
            break;
        }
    }
    pthread_mutex_unlock(&g_egl_diag_lock);
}

static bool snapshot_diag_context(EGLContext context, graphics_SsrnxEglDiagContext *out) {
    bool found = false;
    SSRNX_PERF_LOCK(&g_egl_diag_lock);
    for (size_t i = 0; i < SSRNX_MAX_EGL_DIAG_CONTEXTS; ++i) {
        if (g_egl_diag_contexts[i].in_use && g_egl_diag_contexts[i].context == context) {
            if (out) *out = g_egl_diag_contexts[i];
            found = true;
            break;
        }
    }
    pthread_mutex_unlock(&g_egl_diag_lock);
    return found;
}

static void forget_diag_context(EGLContext context) {
    SSRNX_PERF_LOCK(&g_egl_diag_lock);
    for (size_t i = 0; i < SSRNX_MAX_EGL_DIAG_CONTEXTS; ++i)
        if (g_egl_diag_contexts[i].in_use && g_egl_diag_contexts[i].context == context)
            memset(&g_egl_diag_contexts[i], 0, sizeof(g_egl_diag_contexts[i]));
    pthread_mutex_unlock(&g_egl_diag_lock);
}

static void clear_egl_diag_objects(void) {
    SSRNX_PERF_LOCK(&g_egl_diag_lock);
    for (size_t i = 0; i < SSRNX_MAX_EGL_DIAG_SURFACES; ++i)
        if (g_egl_diag_surfaces[i].in_use)
            ssrnx_perf_forget_surface((uintptr_t)g_egl_diag_surfaces[i].surface);
    memset(g_egl_diag_surfaces, 0, sizeof(g_egl_diag_surfaces));
    memset(g_egl_diag_contexts, 0, sizeof(g_egl_diag_contexts));
    pthread_mutex_unlock(&g_egl_diag_lock);
}

static graphics_SsrnxVirtualPbuffer *find_virtual_pbuffer_locked(const void *surface) {
    if (!surface) return NULL;
    for (size_t i = 0; i < SSRNX_MAX_VIRTUAL_PBUFFERS; ++i) {
        graphics_SsrnxVirtualPbuffer *p = &graphics_g_virtual_pbuffers[i];
        if (p->in_use && surface == (const void *)p) return p;
    }
    return NULL;
}

static bool virtual_pbuffer_snapshot(const void *surface, graphics_SsrnxVirtualPbuffer *out) {
    bool found = false;
    SSRNX_PERF_LOCK(&graphics_g_virtual_pbuffer_lock);
    graphics_SsrnxVirtualPbuffer *p = find_virtual_pbuffer_locked(surface);
    if (p) {
        if (out) *out = *p;
        found = true;
    }
    pthread_mutex_unlock(&graphics_g_virtual_pbuffer_lock);
    return found;
}

static void *alloc_virtual_pbuffer(EGLDisplay display, EGLConfig config, EGLint width, EGLint height) {
    void *handle = NULL;
    SSRNX_PERF_LOCK(&graphics_g_virtual_pbuffer_lock);
    for (size_t i = 0; i < SSRNX_MAX_VIRTUAL_PBUFFERS; ++i) {
        graphics_SsrnxVirtualPbuffer *p = &graphics_g_virtual_pbuffers[i];
        if (p->in_use) continue;
        p->in_use = true;
        p->display = display;
        p->config = config;
        p->width = width;
        p->height = height;
        handle = (void *)p;
        break;
    }
    pthread_mutex_unlock(&graphics_g_virtual_pbuffer_lock);
    return handle;
}

static bool free_virtual_pbuffer(void *surface, EGLDisplay display) {
    bool found = false;
    SSRNX_PERF_LOCK(&graphics_g_virtual_pbuffer_lock);
    graphics_SsrnxVirtualPbuffer *p = find_virtual_pbuffer_locked(surface);
    if (p && (display == EGL_NO_DISPLAY || p->display == display)) {
        memset(p, 0, sizeof(*p));
        found = true;
    }
    pthread_mutex_unlock(&graphics_g_virtual_pbuffer_lock);
    return found;
}

static void free_virtual_pbuffers_for_display(EGLDisplay display) {
    SSRNX_PERF_LOCK(&graphics_g_virtual_pbuffer_lock);
    for (size_t i = 0; i < SSRNX_MAX_VIRTUAL_PBUFFERS; ++i) {
        graphics_SsrnxVirtualPbuffer *p = &graphics_g_virtual_pbuffers[i];
        if (p->in_use && (display == EGL_NO_DISPLAY || p->display == display))
            memset(p, 0, sizeof(*p));
    }
    pthread_mutex_unlock(&graphics_g_virtual_pbuffer_lock);
}

static bool attrib_list_requests_pbuffer(const EGLint *attribs) {
    if (!attribs) return false;
    /* EGL config lists are key/value pairs terminated by EGL_NONE.  Bound the
       scan so a malformed guest list cannot make SSRNX walk forever. */
    for (size_t pairs = 0; pairs < 64; ++pairs) {
        EGLint key = attribs[pairs * 2];
        if (key == EGL_NONE) return false;
        EGLint value = attribs[pairs * 2 + 1];
        if (key == EGL_SURFACE_TYPE && (value & EGL_PBUFFER_BIT)) return true;
    }
    return false;
}

static bool translate_pbuffer_config_attribs(const EGLint *src, EGLint *dst, size_t dst_count) {
    if (!src || !dst || dst_count < 3) return false;
    size_t out = 0;
    bool translated = false;
    for (size_t pairs = 0; pairs < 64; ++pairs) {
        EGLint key = src[pairs * 2];
        if (key == EGL_NONE) {
            if (out >= dst_count) return false;
            dst[out] = EGL_NONE;
            return translated;
        }
        if (out + 2 >= dst_count) return false;
        EGLint value = src[pairs * 2 + 1];
        if (key == EGL_SURFACE_TYPE && (value & EGL_PBUFFER_BIT)) {
            /* Host configs are window-only. SSRNX supplies the requested
               pbuffer behavior itself, so select the equivalent native config. */
            value = (value & ~EGL_PBUFFER_BIT) | EGL_WINDOW_BIT;
            translated = true;
        }
        dst[out++] = key;
        dst[out++] = value;
    }
    return false;
}

static void parse_pbuffer_size(const EGLint *attribs, EGLint *width, EGLint *height) {
    EGLint w = 0, h = 0;
    if (attribs) {
        for (size_t pairs = 0; pairs < 64; ++pairs) {
            EGLint key = attribs[pairs * 2];
            if (key == EGL_NONE) break;
            EGLint value = attribs[pairs * 2 + 1];
            if (key == EGL_WIDTH) w = value;
            else if (key == EGL_HEIGHT) h = value;
        }
    }
    if (width) *width = w;
    if (height) *height = h;
}

static void remember_initialized_display(EGLDisplay display) {
    if (display == EGL_NO_DISPLAY) return;
    for (size_t i = 0; i < SSRNX_MAX_TRACKED_EGL_DISPLAYS; ++i) {
        if (graphics_g_initialized_displays[i] == display) return;
        if (graphics_g_initialized_displays[i] == EGL_NO_DISPLAY) {
            graphics_g_initialized_displays[i] = display;
            return;
        }
    }
    SSRNX_LOGCW("EGL", "initialized-display tracker full; newest=%p", (void *)display);
}

static void forget_initialized_display(EGLDisplay display) {
    if (display == EGL_NO_DISPLAY) return;
    for (size_t i = 0; i < SSRNX_MAX_TRACKED_EGL_DISPLAYS; ++i) {
        if (graphics_g_initialized_displays[i] == display) {
            graphics_g_initialized_displays[i] = EGL_NO_DISPLAY;
        }
    }
}

void *ssrnx_eglGetDisplay(void *display_id) {
    SSRNX_LOGCI("EGL", "eglGetDisplay: BEGIN id=%p", display_id);
    void *out = EPTR(eglGetDisplay((EGLNativeDisplayType)display_id));
    if ((EGLDisplay)out == EGL_NO_DISPLAY) (void)graphics_guest_egl_capture_host_error();
    SSRNX_LOGCI("EGL", "eglGetDisplay: END display=%p", out);
    return out;
}
uint32_t ssrnx_eglInitialize(void *d, int32_t *a, int32_t *b) {
    SSRNX_LOGCI("EGL", "eglInitialize: BEGIN display=%p", d);
    EGLBoolean ok = eglInitialize((EGLDisplay)d,(EGLint*)a,(EGLint*)b);
    if (ok) remember_initialized_display((EGLDisplay)d);
    else (void)graphics_guest_egl_capture_host_error();
    SSRNX_LOGCI("EGL", "eglInitialize: END ok=%u version=%d.%d", (unsigned)ok, a ? *a : -1, b ? *b : -1);
    return EBOOL(ok);
}
uint32_t ssrnx_eglTerminate(void *d) {
    SSRNX_LOGCI("EGL", "eglTerminate: BEGIN display=%p", d);
    EGLBoolean ok = eglTerminate((EGLDisplay)d);
    if (!ok) (void)graphics_guest_egl_capture_host_error();
    if (ok) {
        forget_initialized_display((EGLDisplay)d);
        free_virtual_pbuffers_for_display((EGLDisplay)d);
        if (graphics_g_guest_current_display == (EGLDisplay)d) {
            graphics_g_guest_current_display = EGL_NO_DISPLAY;
            graphics_g_guest_current_context = EGL_NO_CONTEXT;
            graphics_g_guest_current_draw = NULL;
            graphics_g_guest_current_read = NULL;
        }
    }
    SSRNX_LOGCI("EGL", "eglTerminate: END ok=%u", (unsigned)ok);
    return EBOOL(ok);
}
#define SSRNX_EGL_GUEST_EXTENSION "EGL_ANDROID_presentation_time"
typedef struct graphics_SsrnxEglExtensionString {
    EGLDisplay display;
    const char *host;
    char *guest;
} graphics_SsrnxEglExtensionString;
static graphics_SsrnxEglExtensionString g_egl_extension_strings[SSRNX_MAX_TRACKED_EGL_DISPLAYS];
static pthread_mutex_t g_egl_extension_lock = PTHREAD_MUTEX_INITIALIZER;

static bool egl_extension_listed(const char *list, const char *name) {
    const size_t len = strlen(name);
    for (const char *it = list; it && (it = strstr(it, name)) != NULL; it += len) {
        if ((it == list || it[-1] == ' ') && (it[len] == '\0' || it[len] == ' ')) return true;
    }
    return false;
}

/* Display extension strings advertise the EGL extensions SSRNX implements on
 * top of the host driver. Strings are cached per display so returned pointers
 * stay valid for the display's lifetime, as EGL requires. */
static const char *guest_egl_extensions(EGLDisplay d, const char *host) {
    if (egl_extension_listed(host, SSRNX_EGL_GUEST_EXTENSION)) return host;
    const char *out = host;
    pthread_mutex_lock(&g_egl_extension_lock);
    graphics_SsrnxEglExtensionString *slot = NULL;
    for (size_t i = 0; i < SSRNX_MAX_TRACKED_EGL_DISPLAYS; ++i) {
        graphics_SsrnxEglExtensionString *e = &g_egl_extension_strings[i];
        if (e->display == d) { slot = e; break; }
        if (!slot && e->display == EGL_NO_DISPLAY) slot = e;
    }
    if (slot && slot->display == d && slot->host == host && slot->guest) {
        out = slot->guest;
    } else if (slot) {
        const size_t host_len = strlen(host);
        char *guest = malloc(host_len + sizeof(SSRNX_EGL_GUEST_EXTENSION) + 1u);
        if (guest) {
            memcpy(guest, host, host_len);
            size_t at = host_len;
            if (at && guest[at - 1] != ' ') guest[at++] = ' ';
            memcpy(guest + at, SSRNX_EGL_GUEST_EXTENSION, sizeof(SSRNX_EGL_GUEST_EXTENSION));
            free(slot->guest);
            *slot = (graphics_SsrnxEglExtensionString){ .display = d, .host = host, .guest = guest };
            out = guest;
        }
    }
    pthread_mutex_unlock(&g_egl_extension_lock);
    return out;
}

const char *ssrnx_eglQueryString(void *d, int32_t n) {
    const char *out = eglQueryString((EGLDisplay)d,(EGLint)n);
    if (!out) (void)graphics_guest_egl_capture_host_error();
    else if ((EGLint)n == EGL_EXTENSIONS && (EGLDisplay)d != EGL_NO_DISPLAY)
        out = guest_egl_extensions((EGLDisplay)d, out);
    return out;
}
uint32_t ssrnx_eglGetConfigs(void *d, void **c, int32_t s, int32_t *n) { return graphics_guest_egl_bool(eglGetConfigs((EGLDisplay)d,(EGLConfig*)c,(EGLint)s,(EGLint*)n)); }
uint32_t ssrnx_eglChooseConfig(void *d, const int32_t *a, void **c, int32_t s, int32_t *n) {
    const EGLint *attrs = (const EGLint *)a;
    EGLint requested_surface = attrib_list_get(attrs, EGL_SURFACE_TYPE, 0);
    EGLint requested_renderable = attrib_list_get(attrs, EGL_RENDERABLE_TYPE, 0);
    SSRNX_LOGCI("EGL", "eglChooseConfig: BEGIN display=%p max=%d surface=0x%x renderable=0x%x rgba=%d/%d/%d/%d depth=%d stencil=%d sampleBuffers=%d samples=%d",
                d, (int)s, (unsigned)requested_surface, (unsigned)requested_renderable,
                (int)attrib_list_get(attrs, EGL_RED_SIZE, -1), (int)attrib_list_get(attrs, EGL_GREEN_SIZE, -1),
                (int)attrib_list_get(attrs, EGL_BLUE_SIZE, -1), (int)attrib_list_get(attrs, EGL_ALPHA_SIZE, -1),
                (int)attrib_list_get(attrs, EGL_DEPTH_SIZE, -1), (int)attrib_list_get(attrs, EGL_STENCIL_SIZE, -1),
                (int)attrib_list_get(attrs, EGL_SAMPLE_BUFFERS, -1), (int)attrib_list_get(attrs, EGL_SAMPLES, -1));
    bool gles3_request = (requested_renderable & EGL_OPENGL_ES3_BIT_KHR) != 0;
    if (gles3_request) {
        SSRNX_LOGCI("EGL-DIAG", "GLES3 choose request display=%p surface=0x%x renderable=0x%x rgb=%d/%d/%d",
                    d, (unsigned)requested_surface, (unsigned)requested_renderable,
                    (int)attrib_list_get(attrs, EGL_RED_SIZE, -1),
                    (int)attrib_list_get(attrs, EGL_GREEN_SIZE, -1),
                    (int)attrib_list_get(attrs, EGL_BLUE_SIZE, -1));
    }
    EGLBoolean ok = eglChooseConfig((EGLDisplay)d,attrs,(EGLConfig*)c,(EGLint)s,(EGLint*)n);

    /* Switch Mesa's EGL driver exposes window-only configs.  If a valid
       pbuffer request finds no native match, re-run selection with only the
       surface-type requirement translated; all color/depth/renderable
       requirements remain untouched. */
    if (ok == EGL_TRUE && n && *n == 0 && attrib_list_requests_pbuffer(attrs)) {
        EGLint translated[129];
        if (translate_pbuffer_config_attribs(attrs, translated, sizeof(translated) / sizeof(translated[0]))) {
            SSRNX_LOGCI("EGL", "eglChooseConfig: virtual-pbuffer fallback -> native window config");
            ok = eglChooseConfig((EGLDisplay)d,translated,(EGLConfig*)c,(EGLint)s,(EGLint*)n);
        }
    }

    /* Switch Mesa can return multiple window configs that all satisfy a later
       ES3 query, while requiring eglMakeCurrent's context and window surface to
       originate from the same config. If the currently bound SSRNX window's
       exact config also satisfies the guest query, it is a fully valid EGL
       result and is safer to return first than an incompatible sibling config. */
    if (ok == EGL_TRUE && n && *n > 0 && c && s > 0)
        (void)promote_active_window_config((EGLDisplay)d, attrs, (EGLConfig *)c, (EGLint)s, (EGLint *)n);

    SSRNX_LOGCI("EGL", "eglChooseConfig: END ok=%u count=%d first=%p",
                (unsigned)ok, n ? (int)*n : -1, (c && s > 0) ? c[0] : NULL);
    if (gles3_request && ok == EGL_TRUE && n && *n > 0 && c && s > 0 && c[0])
        log_config_diag("GLES3 selected", (EGLDisplay)d, (EGLConfig)c[0]);
    if (ok != EGL_TRUE) (void)graphics_guest_egl_capture_host_error();
    return EBOOL(ok);
}
uint32_t ssrnx_eglGetConfigAttrib(void *d, void *c, int32_t a, int32_t *v) {
    EGLBoolean ok = eglGetConfigAttrib((EGLDisplay)d,(EGLConfig)c,(EGLint)a,(EGLint*)v);
    if (ok == EGL_TRUE && v && a == EGL_SURFACE_TYPE) {
        /* Report the capability SSRNX provides, not only the host driver's
           native surface factory. */
        *v |= EGL_PBUFFER_BIT;
    }
    SSRNX_LOGCI("EGL", "eglGetConfigAttrib display=%p config=%p attr=0x%04x ok=%u value=%d",
                d, c, (unsigned)a, (unsigned)ok, (ok && v) ? (int)*v : -1);
    if (ok != EGL_TRUE) (void)graphics_guest_egl_capture_host_error();
    return EBOOL(ok);
}
void *ssrnx_eglCreateWindowSurface(void *d, void *c, void *guest_window, const int32_t *a) {
    graphics_software_framebuffer_reset();
    (void)guest_window;
    /* Android's EGLNativeWindowType is an ANativeWindow*.  It is not ABI
       compatible with libnx NWindow, so never alias it.  SSRNX currently has
       one process window; map any non-null Android window token to it. */
    NWindow *window = graphics_presentation_native_window();
    SSRNX_LOGCI("EGL", "eglCreateWindowSurface: BEGIN display=%p config=%p guest=%p nwindow=%p presentation=%s", d, c, guest_window, (void *)window,
                graphics_portrait_plan_enabled() ? "portrait-virtual" : "default");
    if ((EGLDisplay)d == EGL_NO_DISPLAY) { graphics_guest_egl_set_error(EGL_BAD_DISPLAY); return NULL; }
    if (!c) { graphics_guest_egl_set_error(EGL_BAD_CONFIG); return NULL; }
    if (!guest_window || !window) { graphics_guest_egl_set_error(EGL_BAD_NATIVE_WINDOW); return NULL; }
    void *surface = EPTR(eglCreateWindowSurface((EGLDisplay)d,(EGLConfig)c,(EGLNativeWindowType)window,(const EGLint*)a));
    if (!surface) (void)graphics_guest_egl_capture_host_error();
    if (surface) {
        int32_t guest_width = 0, guest_height = 0;
        if (!ssrnx_native_window_dimensions((const SsrnxANativeWindow *)guest_window, &guest_width, &guest_height)) {
            if (graphics_g_presentation_plan_valid) {
                guest_width = graphics_g_presentation_plan.logical_width;
                guest_height = graphics_g_presentation_plan.logical_height;
            }
        }
        remember_diag_surface((EGLDisplay)d, (EGLSurface)surface, (EGLConfig)c,
                              (EGLint)guest_width, (EGLint)guest_height, true);
        SSRNX_LOGCI("EGL", "window surface guest dimensions=%dx%d", guest_width, guest_height);
        log_config_diag("window-surface", (EGLDisplay)d, (EGLConfig)c);
        if (graphics_portrait_plan_enabled()) {
            portrait_bridge_destroy_resources();
            portrait_bridge_clear_metadata();
            graphics_g_portrait_bridge.surface_active = true;
            graphics_g_portrait_bridge.display = (EGLDisplay)d;
            graphics_g_portrait_bridge.config = (EGLConfig)c;
            graphics_g_portrait_bridge.surface = (EGLSurface)surface;
            SSRNX_LOGCI("DISPLAY", "portrait physical window surface=%p logical=%dx%d physical=%dx%d",
                        surface, graphics_g_presentation_plan.logical_width, graphics_g_presentation_plan.logical_height,
                        graphics_g_presentation_plan.physical_width, graphics_g_presentation_plan.physical_height);
        }
    }
    SSRNX_LOGCI("EGL", "eglCreateWindowSurface: END surface=%p", surface);
    return surface;
}
void *ssrnx_eglCreatePbufferSurface(void *d, void *c, const int32_t *a) {
    SSRNX_LOGCI("EGL", "eglCreatePbufferSurface: BEGIN display=%p config=%p", d, c);

    /* Validate that the host knows this config without requiring native
       PBUFFER_BIT support. Context creation with the config remains native. */
    EGLint renderable = 0;
    if ((EGLDisplay)d == EGL_NO_DISPLAY) {
        SSRNX_LOGCW("EGL", "eglCreatePbufferSurface: invalid display");
        graphics_guest_egl_set_error(EGL_BAD_DISPLAY);
        return NULL;
    }
    if (!c) {
        SSRNX_LOGCW("EGL", "eglCreatePbufferSurface: invalid config");
        graphics_guest_egl_set_error(EGL_BAD_CONFIG);
        return NULL;
    }
    EGLint native_surface_type = 0;
    if (eglGetConfigAttrib((EGLDisplay)d, (EGLConfig)c, EGL_SURFACE_TYPE, &native_surface_type) != EGL_TRUE) {
        (void)graphics_guest_egl_capture_host_error();
        return NULL;
    }
    if (native_surface_type & EGL_PBUFFER_BIT) {
        EGLSurface native = eglCreatePbufferSurface((EGLDisplay)d, (EGLConfig)c, (const EGLint *)a);
        if (native == EGL_NO_SURFACE) (void)graphics_guest_egl_capture_host_error();
        else {
            remember_diag_surface((EGLDisplay)d, native, (EGLConfig)c, 0, 0, false);
            SSRNX_LOGCI("EGL", "eglCreatePbufferSurface: native surface=%p backend=render-target", (void *)native);
        }
        return EPTR(native);
    }
    if (eglGetConfigAttrib((EGLDisplay)d,(EGLConfig)c,EGL_RENDERABLE_TYPE,&renderable) != EGL_TRUE) {
        SSRNX_LOGCW("EGL", "eglCreatePbufferSurface: config rejected by host");
        (void)graphics_guest_egl_capture_host_error();
        return NULL;
    }

    EGLint width = 0, height = 0;
    parse_pbuffer_size((const EGLint *)a, &width, &height);
    if (width < 0 || height < 0) {
        SSRNX_LOGCW("EGL", "eglCreatePbufferSurface: invalid size %dx%d", (int)width, (int)height);
        graphics_guest_egl_set_error(EGL_BAD_ATTRIBUTE);
        return NULL;
    }

    void *surface = alloc_virtual_pbuffer((EGLDisplay)d, (EGLConfig)c, width, height);
    if (!surface) {
        SSRNX_LOGCW("EGL", "eglCreatePbufferSurface: virtual surface table full");
        graphics_guest_egl_set_error(EGL_BAD_ALLOC);
        return NULL;
    }
    SSRNX_LOGCI("EGL", "eglCreatePbufferSurface: virtual surface=%p size=%dx%d backend=surfaceless",
                surface, (int)width, (int)height);
    return surface;
}
void *ssrnx_eglCreatePixmapSurface(void *d, void *c, void *p, const int32_t *a) {
    EGLSurface out = eglCreatePixmapSurface((EGLDisplay)d,(EGLConfig)c,(EGLNativePixmapType)p,(const EGLint*)a);
    if (out == EGL_NO_SURFACE) (void)graphics_guest_egl_capture_host_error();
    return EPTR(out);
}
uint32_t ssrnx_eglDestroySurface(void *d, void *s) {
    graphics_SsrnxVirtualPbuffer virtual_meta;
    if (virtual_pbuffer_snapshot(s, &virtual_meta)) {
        if (virtual_meta.display != (EGLDisplay)d) {
            graphics_guest_egl_set_error(EGL_BAD_MATCH);
            return 0u;
        }
        (void)free_virtual_pbuffer(s, (EGLDisplay)d);
        if (graphics_g_guest_current_draw == s) graphics_g_guest_current_draw = NULL;
        if (graphics_g_guest_current_read == s) graphics_g_guest_current_read = NULL;
        SSRNX_LOGCI("EGL", "eglDestroySurface: virtual pbuffer=%p", s);
        return 1u;
    }
    bool portrait = graphics_portrait_surface_is(s);
    if (portrait) portrait_bridge_destroy_resources();
    EGLBoolean ok = eglDestroySurface((EGLDisplay)d,(EGLSurface)s);
    if (ok != EGL_TRUE) (void)graphics_guest_egl_capture_host_error();
    if (ok == EGL_TRUE) {
        forget_diag_surface((EGLSurface)s);
        if (portrait) portrait_bridge_clear_metadata();
    }
    return EBOOL(ok);
}
uint32_t ssrnx_eglQuerySurface(void *d, void *s, int32_t a, int32_t *v) {
    graphics_SsrnxVirtualPbuffer pbuf;
    if (virtual_pbuffer_snapshot(s, &pbuf)) {
        if (pbuf.display != (EGLDisplay)d) { graphics_guest_egl_set_error(EGL_BAD_MATCH); return 0u; }
        if (!v) { graphics_guest_egl_set_error(EGL_BAD_PARAMETER); return 0u; }
        EGLint value = 0;
        switch ((EGLint)a) {
            case EGL_WIDTH: value = pbuf.width; break;
            case EGL_HEIGHT: value = pbuf.height; break;
            case EGL_CONFIG_ID:
                if (eglGetConfigAttrib((EGLDisplay)d,pbuf.config,EGL_CONFIG_ID,&value) != EGL_TRUE) {
                    (void)graphics_guest_egl_capture_host_error();
                    return 0u;
                }
                break;
            case EGL_RENDER_BUFFER: value = EGL_BACK_BUFFER; break;
            case EGL_TEXTURE_FORMAT: value = EGL_NO_TEXTURE; break;
            case EGL_TEXTURE_TARGET: value = EGL_NO_TEXTURE; break;
            case EGL_MIPMAP_TEXTURE: value = EGL_FALSE; break;
            case EGL_MIPMAP_LEVEL: value = 0; break;
            default:
                SSRNX_LOGCW("EGL", "eglQuerySurface: unsupported virtual-pbuffer attr=0x%x", (unsigned)a);
                graphics_guest_egl_set_error(EGL_BAD_ATTRIBUTE);
                return 0u;
        }
        *v = (int32_t)value;
        SSRNX_LOGCI("EGL", "eglQuerySurface virtual-pbuffer=%p attr=0x%x value=%d", s, (unsigned)a, (int)value);
        return 1u;
    }

    graphics_SsrnxEglDiagSurface portrait_window_meta;
    if (graphics_portrait_plan_enabled() && v && ((EGLint)a == EGL_WIDTH || (EGLint)a == EGL_HEIGHT) &&
        snapshot_diag_surface((EGLSurface)s, &portrait_window_meta) &&
        portrait_window_meta.is_window && portrait_window_meta.display == (EGLDisplay)d) {
        *v = ((EGLint)a == EGL_WIDTH) ? graphics_g_presentation_plan.logical_width : graphics_g_presentation_plan.logical_height;
        SSRNX_LOGCI("EGL", "eglQuerySurface portrait surface=%p attr=0x%x value=%d",
                    s, (unsigned)a, (int)*v);
        return 1u;
    }

    /* Switch Mesa can report 0x0 for a perfectly usable NWindow surface.
       Android apps are allowed to trust EGL_WIDTH/EGL_HEIGHT after creating a
       window surface, so virtualize those two attributes from the Android
       ANativeWindow dimensions captured at surface creation.  This also
       preserves guest buffer scaling requested through
       ANativeWindow_setBuffersGeometry(). */
    graphics_SsrnxEglDiagSurface window_meta;
    if (v && ((EGLint)a == EGL_WIDTH || (EGLint)a == EGL_HEIGHT) &&
        snapshot_diag_surface((EGLSurface)s, &window_meta) && window_meta.is_window &&
        window_meta.display == (EGLDisplay)d && window_meta.width > 0 && window_meta.height > 0) {
        *v = ((EGLint)a == EGL_WIDTH) ? (int32_t)window_meta.width : (int32_t)window_meta.height;
        SSRNX_LOGCI("EGL", "eglQuerySurface virtual-window surface=%p attr=0x%x value=%d",
                    s, (unsigned)a, (int)*v);
        return 1u;
    }

    EGLint value = 0;
    EGLBoolean ok = eglQuerySurface((EGLDisplay)d,(EGLSurface)s,(EGLint)a,&value);
    if (ok != EGL_TRUE) (void)graphics_guest_egl_capture_host_error();
    if (v) *v = (int32_t)value;
    SSRNX_LOGCI("EGL", "eglQuerySurface display=%p surface=%p attr=0x%x ok=%d value=%d",
                d, s, (unsigned)a, ok == EGL_TRUE ? 1 : 0, (int)value);
    return EBOOL(ok);
}
uint32_t ssrnx_eglBindAPI(uint32_t api) { return graphics_guest_egl_bool(eglBindAPI((EGLenum)api)); }
uint32_t ssrnx_eglQueryAPI(void) { return (uint32_t)eglQueryAPI(); }
uint32_t ssrnx_eglWaitClient(void) { return graphics_guest_egl_bool(eglWaitClient()); }
uint32_t ssrnx_eglReleaseThread(void) {
    EGLBoolean ok = eglReleaseThread();
    if (ok != EGL_TRUE) (void)graphics_guest_egl_capture_host_error();
    if (ok == EGL_TRUE) {
        graphics_g_guest_current_display = EGL_NO_DISPLAY;
        graphics_g_guest_current_context = EGL_NO_CONTEXT;
        graphics_g_guest_current_draw = NULL;
        graphics_g_guest_current_read = NULL;
    }
    return EBOOL(ok);
}
uint32_t ssrnx_eglSurfaceAttrib(void *d, void *s, int32_t a, int32_t v) {
    graphics_SsrnxVirtualPbuffer p;
    if (virtual_pbuffer_snapshot(s, &p)) {
        (void)d;
        /* No mutable pbuffer attributes are currently materialized by the
           surfaceless backend. Reject rather than forwarding an opaque handle. */
        SSRNX_LOGCW("EGL", "eglSurfaceAttrib: unsupported virtual-pbuffer attr=0x%x value=%d", (unsigned)a, (int)v);
        graphics_guest_egl_set_error(EGL_BAD_ATTRIBUTE);
        return 0u;
    }
    return graphics_guest_egl_bool(eglSurfaceAttrib((EGLDisplay)d,(EGLSurface)s,(EGLint)a,(EGLint)v));
}
/* EGL_ANDROID_presentation_time has been part of Android's EGL since 4.3, and
 * frame-pacing libraries (Swappy/AGDK) refuse to initialize without it. Android
 * forwards the timestamp to the BufferQueue so the compositor may hold the
 * buffer; SSRNX presents directly to the host window at eglSwapBuffers' next
 * vsync, so the timestamp is accepted as the extension's scheduling hint after
 * the display/surface validation Android performs. */
uint32_t ssrnx_eglPresentationTimeANDROID(void *d, void *s, int64_t when_ns) {
    graphics_SsrnxVirtualPbuffer p;
    if (virtual_pbuffer_snapshot(s, &p)) {
        if (p.display != (EGLDisplay)d) { graphics_guest_egl_set_error(EGL_BAD_MATCH); return 0u; }
    } else {
        EGLint width = 0;
        if (eglQuerySurface((EGLDisplay)d,(EGLSurface)s,EGL_WIDTH,&width) != EGL_TRUE) {
            (void)graphics_guest_egl_capture_host_error();
            return 0u;
        }
    }
    static atomic_uint reports;
    if (atomic_fetch_add(&reports, 1u) < 4u)
        SSRNX_LOGCI("EGL", "eglPresentationTimeANDROID display=%p surface=%p whenNs=%lld",
                    d, s, (long long)when_ns);
    return 1u;
}
uint32_t ssrnx_eglBindTexImage(void *d, void *s, int32_t b) {
    graphics_SsrnxVirtualPbuffer p;
    if (virtual_pbuffer_snapshot(s, &p)) { (void)d; (void)b; graphics_guest_egl_set_error(EGL_BAD_MATCH); return 0u; }
    return graphics_guest_egl_bool(eglBindTexImage((EGLDisplay)d,(EGLSurface)s,(EGLint)b));
}
uint32_t ssrnx_eglReleaseTexImage(void *d, void *s, int32_t b) {
    graphics_SsrnxVirtualPbuffer p;
    if (virtual_pbuffer_snapshot(s, &p)) { (void)d; (void)b; graphics_guest_egl_set_error(EGL_BAD_MATCH); return 0u; }
    return graphics_guest_egl_bool(eglReleaseTexImage((EGLDisplay)d,(EGLSurface)s,(EGLint)b));
}
uint32_t ssrnx_eglSwapInterval(void *d, int32_t i) {
    EGLBoolean ok = eglSwapInterval((EGLDisplay)d,(EGLint)i);
    if (ok != EGL_TRUE) (void)graphics_guest_egl_capture_host_error();
    SSRNX_LOGCI("EGL", "eglSwapInterval display=%p interval=%d ok=%d", d, (int)i, ok == EGL_TRUE ? 1 : 0);
    return EBOOL(ok);
}
void *ssrnx_eglCreateContext(void *d, void *c, void *s, const int32_t *a) {
    EGLint requested_version = attrib_list_get((const EGLint *)a, EGL_CONTEXT_CLIENT_VERSION, 1);
    SSRNX_LOGCI("EGL", "eglCreateContext: BEGIN display=%p config=%p share=%p requestedClient=%d", d, c, s, (int)requested_version);
    if (requested_version >= 3) log_config_diag("GLES3 context-config", (EGLDisplay)d, (EGLConfig)c);
    void *ctx = EPTR(eglCreateContext((EGLDisplay)d,(EGLConfig)c,(EGLContext)s,(const EGLint*)a));
    if (ctx) {
        remember_diag_context((EGLDisplay)d, (EGLContext)ctx, (EGLConfig)c, requested_version);
        if (requested_version >= 3) {
            EGLint actual_version = -1;
            EGLBoolean qok = eglQueryContext((EGLDisplay)d, (EGLContext)ctx, EGL_CONTEXT_CLIENT_VERSION, &actual_version);
            SSRNX_LOGCI("EGL-DIAG", "GLES3 context created=%p requested=%d queryOk=%u actualClient=%d",
                        ctx, (int)requested_version, (unsigned)qok, (int)actual_version);
        }
    } else {
        EGLint error = graphics_guest_egl_capture_host_error();
        if (requested_version >= 3)
            SSRNX_LOGCW("EGL-DIAG", "GLES3 eglCreateContext failed error=0x%04x %s", (unsigned)error, egl_error_name(error));
    }
    SSRNX_LOGCI("EGL", "eglCreateContext: END context=%p", ctx);
    return ctx;
}
uint32_t ssrnx_eglDestroyContext(void *d, void *c) {
    if ((EGLContext)c != EGL_NO_CONTEXT && (EGLContext)c == graphics_g_portrait_bridge.guest_context) {
        portrait_bridge_destroy_resources();
        graphics_g_portrait_bridge.guest_context = EGL_NO_CONTEXT;
        graphics_g_portrait_bridge.color_texture = 0;
        graphics_g_portrait_bridge.guest_fbo = 0;
    }
    EGLBoolean ok = eglDestroyContext((EGLDisplay)d,(EGLContext)c);
    if (ok != EGL_TRUE) (void)graphics_guest_egl_capture_host_error();
    if (ok == EGL_TRUE) forget_diag_context((EGLContext)c);
    return EBOOL(ok);
}
uint32_t ssrnx_eglMakeCurrent(void *d, void *draw, void *read, void *c) {
    static _Thread_local uint64_t sequence;
    const bool trace = ssrnx_diag_full_trace_enabled() || ++sequence <= 8 ||
        d != (void *)graphics_g_guest_current_display || c != (void *)graphics_g_guest_current_context ||
        draw != graphics_g_guest_current_draw || read != graphics_g_guest_current_read;
    if (trace) SSRNX_LOGCI("EGL", "eglMakeCurrent: BEGIN display=%p draw=%p read=%p context=%p", d, draw, read, c);

    graphics_SsrnxVirtualPbuffer draw_pbuf, read_pbuf;
    bool draw_virtual = virtual_pbuffer_snapshot(draw, &draw_pbuf);
    bool read_virtual = virtual_pbuffer_snapshot(read, &read_pbuf);
    EGLBoolean ok;

    graphics_SsrnxEglDiagContext context_diag;
    bool have_context_diag = snapshot_diag_context((EGLContext)c, &context_diag);
    bool gles3_context = have_context_diag && context_diag.requested_client_version >= 3;
    graphics_SsrnxEglDiagSurface draw_diag, read_diag;
    bool have_draw_diag = snapshot_diag_surface((EGLSurface)draw, &draw_diag);
    bool have_read_diag = snapshot_diag_surface((EGLSurface)read, &read_diag);
    if (gles3_context && trace) {
        SSRNX_LOGCI("EGL-DIAG", "GLES3 make-current pre hostCurrent display=%p context=%p draw=%p read=%p",
                    (void *)eglGetCurrentDisplay(), (void *)eglGetCurrentContext(),
                    (void *)eglGetCurrentSurface(EGL_DRAW), (void *)eglGetCurrentSurface(EGL_READ));
        SSRNX_LOGCI("EGL-DIAG", "GLES3 context=%p config=%p requestedClient=%d drawTracked=%u readTracked=%u",
                    c, (void *)context_diag.config, (int)context_diag.requested_client_version,
                    have_draw_diag ? 1u : 0u, have_read_diag ? 1u : 0u);
        log_config_diag("GLES3 context-config@make-current", (EGLDisplay)d, context_diag.config);
        if (have_draw_diag) {
            log_config_diag("GLES3 draw-surface-config", (EGLDisplay)d, draw_diag.config);
            SSRNX_LOGCI("EGL-DIAG", "GLES3 draw compatibility surface=%p surfaceConfig=%p contextConfig=%p sameHandle=%u sameDisplay=%u",
                        draw, (void *)draw_diag.config, (void *)context_diag.config,
                        draw_diag.config == context_diag.config ? 1u : 0u,
                        draw_diag.display == (EGLDisplay)d && context_diag.display == (EGLDisplay)d ? 1u : 0u);
            log_config_compatibility((EGLDisplay)d, draw_diag.config, context_diag.config);
        }
        if (have_read_diag && (!have_draw_diag || read_diag.config != draw_diag.config))
            log_config_diag("GLES3 read-surface-config", (EGLDisplay)d, read_diag.config);
    }

    if (draw_virtual || read_virtual) {
        /* EGL_KHR_surfaceless_context requires both native host surfaces to be
           EGL_NO_SURFACE. Keep the virtual handles only in SSRNX's guest-facing
           TLS so eglGetCurrentSurface still observes Android semantics. */
        if (!draw_virtual || !read_virtual ||
            draw_pbuf.display != (EGLDisplay)d || read_pbuf.display != (EGLDisplay)d) {
            SSRNX_LOGCW("EGL", "eglMakeCurrent: mixed/foreign virtual pbuffer surfaces unsupported");
            graphics_guest_egl_set_error(EGL_BAD_MATCH);
            return 0u;
        }
        ok = eglMakeCurrent((EGLDisplay)d,EGL_NO_SURFACE,EGL_NO_SURFACE,(EGLContext)c);
        if (trace || ok != EGL_TRUE) SSRNX_LOGCI("EGL", "eglMakeCurrent: virtual-pbuffer host binding uses EGL_KHR_surfaceless_context ok=%u", (unsigned)ok);
    } else {
        ok = eglMakeCurrent((EGLDisplay)d,(EGLSurface)draw,(EGLSurface)read,(EGLContext)c);
    }

    if (ok == EGL_TRUE && (EGLContext)c != EGL_NO_CONTEXT && graphics_portrait_plan_enabled() &&
        have_draw_diag && draw_diag.is_window) {
        (void)portrait_bridge_adopt_window_surface((EGLDisplay)d, (EGLSurface)draw, draw_diag.config);
    }
    if (ok == EGL_TRUE && (EGLContext)c != EGL_NO_CONTEXT && graphics_portrait_surface_is(draw)) {
        EGLConfig bridge_config = have_draw_diag ? draw_diag.config : (have_context_diag ? context_diag.config : graphics_g_portrait_bridge.config);
        if (!portrait_bridge_ensure((EGLDisplay)d, bridge_config, (EGLSurface)draw, (EGLContext)c)) {
            SSRNX_LOGCE("DISPLAY", "portrait bridge initialization failed");
            (void)eglMakeCurrent((EGLDisplay)d, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            graphics_guest_egl_set_error(EGL_BAD_ALLOC);
            ok = EGL_FALSE;
        }
    }

    if (gles3_context) {
        if (ok == EGL_TRUE && trace) {
            const GLubyte *version = glGetString(GL_VERSION);
            const GLubyte *renderer = glGetString(GL_RENDERER);
            SSRNX_LOGCI("EGL-DIAG", "GLES3 make-current SUCCESS GL_VERSION=%s GL_RENDERER=%s",
                        version ? (const char *)version : "(null)", renderer ? (const char *)renderer : "(null)");
        } else if (ok != EGL_TRUE) {
            EGLint error = graphics_g_guest_egl_error != EGL_SUCCESS
                         ? graphics_g_guest_egl_error : graphics_guest_egl_capture_host_error();
            SSRNX_LOGCW("EGL-DIAG", "GLES3 make-current FAIL error=0x%04x %s",
                        (unsigned)error, egl_error_name(error));
        }
    }

    if (ok != EGL_TRUE && graphics_g_guest_egl_error == EGL_SUCCESS)
        (void)graphics_guest_egl_capture_host_error();

    if (ok == EGL_TRUE) {
        if ((EGLContext)c == EGL_NO_CONTEXT) {
            graphics_g_guest_current_display = EGL_NO_DISPLAY;
            graphics_g_guest_current_context = EGL_NO_CONTEXT;
            graphics_g_guest_current_draw = NULL;
            graphics_g_guest_current_read = NULL;
        } else {
            graphics_g_guest_current_display = (EGLDisplay)d;
            graphics_g_guest_current_context = (EGLContext)c;
            graphics_g_guest_current_draw = draw;
            graphics_g_guest_current_read = read;
        }
    }
    if (trace || ok != EGL_TRUE) SSRNX_LOGCI("EGL", "eglMakeCurrent: END ok=%u", (unsigned)ok);
    return EBOOL(ok);
}
void *ssrnx_eglGetCurrentContext(void) {
    EGLContext host = eglGetCurrentContext();
    if (graphics_g_guest_current_context != EGL_NO_CONTEXT && host == graphics_g_guest_current_context)
        return EPTR(graphics_g_guest_current_context);
    return EPTR(host);
}
void *ssrnx_eglGetCurrentSurface(int32_t r) {
    if (graphics_g_guest_current_context != EGL_NO_CONTEXT) {
        if ((EGLint)r == EGL_READ) return graphics_g_guest_current_read;
        if ((EGLint)r == EGL_DRAW) return graphics_g_guest_current_draw;
    }
    return EPTR(eglGetCurrentSurface((EGLint)r));
}
void *ssrnx_eglGetCurrentDisplay(void) {
    if (graphics_g_guest_current_context != EGL_NO_CONTEXT && graphics_g_guest_current_display != EGL_NO_DISPLAY)
        return EPTR(graphics_g_guest_current_display);
    return EPTR(eglGetCurrentDisplay());
}
uint32_t ssrnx_eglQueryContext(void *d, void *c, int32_t a, int32_t *v) { return graphics_guest_egl_bool(eglQueryContext((EGLDisplay)d,(EGLContext)c,(EGLint)a,(EGLint*)v)); }
uint32_t ssrnx_eglWaitGL(void) { return graphics_guest_egl_bool(eglWaitGL()); }
uint32_t ssrnx_eglWaitNative(int32_t e) { return graphics_guest_egl_bool(eglWaitNative((EGLint)e)); }
extern int ssrnx_thread_policy_pin_render(void) __attribute__((weak));

/* The thread that presents a window surface is the app's render thread,
 * whether SSRNX drives the loop or the engine does (Unity, Unreal, ...).
 * Android runs it in the top-app group with display priority; SSRNX's
 * equivalent is the reserved render core, which keeps guest workers and
 * background Java threads from time-slicing with it. Elected once, on the
 * first window present of a thread other than the current elected one. */
static void present_thread_elect_render(EGLSurface s) {
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    static pthread_t elected;
    static bool have_elected;
    if (!ssrnx_thread_policy_pin_render) return;
    graphics_SsrnxEglDiagSurface meta;
    if (!snapshot_diag_surface(s, &meta) || !meta.is_window) return;
    const pthread_t self = pthread_self();
    pthread_mutex_lock(&lock);
    const bool elect = !have_elected || !pthread_equal(elected, self);
    if (elect) { elected = self; have_elected = true; }
    pthread_mutex_unlock(&lock);
    if (!elect) return;
    const int core = ssrnx_thread_policy_pin_render();
    SSRNX_LOGCI("PERF", "render thread elected by window present core=%d", core);
}

uint32_t ssrnx_eglSwapBuffers(void *d, void *s) {
    SSRNX_PERF_SCOPE(SSRNX_PERF_PRESENT);
    struct timespec swap_begin_ts = {0};
    (void)clock_gettime(CLOCK_MONOTONIC, &swap_begin_ts);
    const uint64_t next_present = graphics_g_ssrnx_present_count + 1u;
    const bool log_swap = next_present <= 5u || (next_present % 120u) == 0u;
    if (log_swap) SSRNX_LOGCI("EGL", "eglSwapBuffers: BEGIN display=%p surface=%p", d, s);
    graphics_SsrnxVirtualPbuffer p;
    if (virtual_pbuffer_snapshot(s, &p)) {
        (void)d;
        SSRNX_LOGCI("EGL", "eglSwapBuffers: virtual pbuffer no-op");
        return 1u;
    }
    graphics_SsrnxEglDiagSurface offscreen;
    if (snapshot_diag_surface((EGLSurface)s, &offscreen) && !offscreen.is_window)
        return graphics_guest_egl_bool(eglSwapBuffers((EGLDisplay)d, (EGLSurface)s));
    const bool perf_enabled = ssrnx_perf_mode() != SSRNX_PERF_OFF;
    const uint64_t perf_begin_us = perf_enabled ? ssrnx_perf_now_us() : 0;
    if (graphics_portrait_plan_enabled() && !graphics_portrait_surface_is(s)) {
        graphics_SsrnxEglDiagSurface meta;
        if (snapshot_diag_surface((EGLSurface)s, &meta) && meta.is_window &&
            meta.display == (EGLDisplay)d &&
            portrait_bridge_adopt_window_surface((EGLDisplay)d, (EGLSurface)s, meta.config)) {
            EGLContext current = eglGetCurrentContext();
            if (current != EGL_NO_CONTEXT)
                (void)portrait_bridge_ensure((EGLDisplay)d, meta.config,
                                             (EGLSurface)s, current);
        }
    }
    graphics_gldiag_frame_report(next_present);
    present_thread_elect_render((EGLSurface)s);
    EGLBoolean ok = graphics_portrait_surface_is(s)
                  ? portrait_bridge_present((EGLDisplay)d, (EGLSurface)s)
                  : cursor_window_swap((EGLDisplay)d,(EGLSurface)s);
    const uint64_t perf_end_us = perf_enabled ? ssrnx_perf_now_us() : 0;
    if (ok != EGL_TRUE && graphics_g_guest_egl_error == EGL_SUCCESS)
        (void)graphics_guest_egl_capture_host_error();
    if (ok == EGL_TRUE) graphics_g_ssrnx_present_count++;
    struct timespec swap_end_ts = {0};
    (void)clock_gettime(CLOCK_MONOTONIC, &swap_end_ts);
    uint64_t swap_elapsed_us = 0;
    if (swap_end_ts.tv_sec > swap_begin_ts.tv_sec ||
        (swap_end_ts.tv_sec == swap_begin_ts.tv_sec && swap_end_ts.tv_nsec >= swap_begin_ts.tv_nsec)) {
        const int64_t sec = (int64_t)swap_end_ts.tv_sec - (int64_t)swap_begin_ts.tv_sec;
        const int64_t nsec = (int64_t)swap_end_ts.tv_nsec - (int64_t)swap_begin_ts.tv_nsec;
        swap_elapsed_us = (uint64_t)(sec * INT64_C(1000000) + nsec / INT64_C(1000));
    }
    if (log_swap || ok != EGL_TRUE)
        SSRNX_LOGCI("EGL", "eglSwapBuffers: END ok=%u presents=%llu elapsedUs=%llu",
                    (unsigned)ok, (unsigned long long)graphics_g_ssrnx_present_count,
                    (unsigned long long)swap_elapsed_us);
    /* Engine-independent frame rate: presents per wall-clock window, so
     * app-managed render loops (Unity, Unreal, ...) report FPS as well. */
    if (ok == EGL_TRUE) {
        static uint64_t window_begin_us, window_presents, window_max_gap_us, last_present_us;
        const uint64_t now_us = (uint64_t)swap_end_ts.tv_sec * UINT64_C(1000000) +
                                (uint64_t)swap_end_ts.tv_nsec / UINT64_C(1000);
        ssrnx_perf_present((uintptr_t)s, perf_begin_us, perf_end_us);
        if (!window_begin_us) window_begin_us = now_us;
        if (last_present_us && now_us - last_present_us > window_max_gap_us)
            window_max_gap_us = now_us - last_present_us;
        last_present_us = now_us;
        window_presents++;
        if (now_us - window_begin_us >= UINT64_C(5000000)) {
            const uint64_t span = now_us - window_begin_us;
            const uint64_t fps_x10 = window_presents * UINT64_C(10000000) / span;
            SSRNX_LOGCI("PERF", "present rate fps=%llu.%llu presents=%llu windowMs=%llu maxFrameGapMs=%llu",
                        (unsigned long long)(fps_x10 / 10u), (unsigned long long)(fps_x10 % 10u),
                        (unsigned long long)graphics_g_ssrnx_present_count,
                        (unsigned long long)(span / 1000u),
                        (unsigned long long)(window_max_gap_us / 1000u));
            window_begin_us = now_us;
            window_presents = 0;
            window_max_gap_us = 0;
        }
    }
    if (swap_elapsed_us >= UINT64_C(50000))
        SSRNX_LOGCI("PERF", "slow eglSwapBuffers elapsedUs=%llu presents=%llu",
                    (unsigned long long)swap_elapsed_us,
                    (unsigned long long)graphics_g_ssrnx_present_count);
    /* full-trace.flag captures startup/link/JNI divergence. Once a real guest
     * frame is presented, sustained per-futex/per-JNI tracing can dominate
     * frame time through svcOutputDebugString + SD I/O. runtime-trace.flag is
     * the explicit opt-in for deliberately tracing the hot path. */
    if (ok == EGL_TRUE && graphics_g_ssrnx_present_count == 1u && ssrnx_diag_full_trace_enabled()) {
        if (access("sdmc:/switch/stonestory_nx/cache/runtime-trace.flag", F_OK) != 0) {
            SSRNX_LOGCI("PERF", "first guest present reached; switching diagnostics FULL TRACE -> FAST (create runtime-trace.flag to retain hot-path trace)");
            ssrnx_diag_set_full_trace(false);
        } else {
            SSRNX_LOGCW("PERF", "runtime-trace.flag keeps FULL TRACE active after first present; remove it for representative FPS testing");
        }
    }
    return EBOOL(ok);
}
uint32_t ssrnx_eglCopyBuffers(void *d, void *s, void *t) {
    graphics_SsrnxVirtualPbuffer p;
    if (virtual_pbuffer_snapshot(s, &p)) { (void)d; (void)t; graphics_guest_egl_set_error(EGL_BAD_SURFACE); return 0u; }
    return graphics_guest_egl_bool(eglCopyBuffers((EGLDisplay)d,(EGLSurface)s,(EGLNativePixmapType)t));
}
int32_t ssrnx_eglGetError(void) {
    if (graphics_g_guest_egl_error != EGL_SUCCESS) {
        EGLint error = graphics_g_guest_egl_error;
        graphics_g_guest_egl_error = EGL_SUCCESS;
        return (int32_t)error;
    }
    return (int32_t)eglGetError();
}

#else
bool ssrnx_graphics_configure_presentation(const SsrnxDisplayPlan *plan) {
    if (!plan) return false;
    graphics_g_presentation_plan = *plan;
    graphics_g_presentation_plan_valid = true;
    return true;
}
bool ssrnx_graphics_present_rgba(const uint8_t *pixels, int32_t width, int32_t height, uint32_t stride) {
    (void)pixels; (void)width; (void)height; (void)stride;
    return false; /* Host tests inject a framebuffer backend explicitly. */
}
/* Host compilation stubs.  Real behavioral Stage-9 host tests provide a
 * separate fake EGL/GLES implementation to the fixture. */
#define STUB_TRUE 1u
void *ssrnx_eglGetDisplay(void *x){(void)x;return (void*)1;}
uint32_t ssrnx_eglInitialize(void*d,int32_t*a,int32_t*b){(void)d;if(a)*a=1;if(b)*b=5;return STUB_TRUE;}
uint32_t ssrnx_eglTerminate(void*d){(void)d;return STUB_TRUE;}
const char *ssrnx_eglQueryString(void*d,int32_t n){(void)d;(void)n;return "APN64 host EGL";}
uint32_t ssrnx_eglGetConfigs(void*d,void**c,int32_t s,int32_t*n){(void)d;(void)s;if(c)*c=(void*)2;if(n)*n=1;return STUB_TRUE;}
uint32_t ssrnx_eglChooseConfig(void*d,const int32_t*a,void**c,int32_t s,int32_t*n){(void)d;(void)a;(void)s;if(c)*c=(void*)2;if(n)*n=1;return STUB_TRUE;}
uint32_t ssrnx_eglGetConfigAttrib(void*d,void*c,int32_t a,int32_t*v){(void)d;(void)c;(void)a;if(v)*v=8;return STUB_TRUE;}
void *ssrnx_eglCreateWindowSurface(void*d,void*c,void*w,const int32_t*a){(void)d;(void)c;(void)w;(void)a;return (void*)3;}
void *ssrnx_eglCreatePbufferSurface(void*d,void*c,const int32_t*a){(void)d;(void)c;(void)a;return (void*)4;}
void *ssrnx_eglCreatePixmapSurface(void*d,void*c,void*p,const int32_t*a){(void)d;(void)c;(void)p;(void)a;return NULL;}
uint32_t ssrnx_eglDestroySurface(void*d,void*s){(void)d;(void)s;return STUB_TRUE;}
uint32_t ssrnx_eglQuerySurface(void*d,void*s,int32_t a,int32_t*v){(void)d;(void)s;(void)a;if(v)*v=64;return STUB_TRUE;}
uint32_t ssrnx_eglBindAPI(uint32_t a){(void)a;return STUB_TRUE;}
uint32_t ssrnx_eglQueryAPI(void){return 0x30A0u;}
uint32_t ssrnx_eglWaitClient(void){return STUB_TRUE;}
uint32_t ssrnx_eglReleaseThread(void){return STUB_TRUE;}
uint32_t ssrnx_eglSurfaceAttrib(void*d,void*s,int32_t a,int32_t v){(void)d;(void)s;(void)a;(void)v;return STUB_TRUE;}
uint32_t ssrnx_eglPresentationTimeANDROID(void*d,void*s,int64_t t){(void)d;(void)s;(void)t;return STUB_TRUE;}
uint32_t ssrnx_eglBindTexImage(void*d,void*s,int32_t b){(void)d;(void)s;(void)b;return STUB_TRUE;}
uint32_t ssrnx_eglReleaseTexImage(void*d,void*s,int32_t b){(void)d;(void)s;(void)b;return STUB_TRUE;}
uint32_t ssrnx_eglSwapInterval(void*d,int32_t i){(void)d;(void)i;return STUB_TRUE;}
void *ssrnx_eglCreateContext(void*d,void*c,void*s,const int32_t*a){(void)d;(void)c;(void)s;(void)a;return (void*)5;}
uint32_t ssrnx_eglDestroyContext(void*d,void*c){(void)d;(void)c;return STUB_TRUE;}
uint32_t ssrnx_eglMakeCurrent(void*d,void*a,void*b,void*c){(void)d;(void)a;(void)b;(void)c;return STUB_TRUE;}
void *ssrnx_eglGetCurrentContext(void){return (void*)5;}
void *ssrnx_eglGetCurrentSurface(int32_t r){(void)r;return (void*)3;}
void *ssrnx_eglGetCurrentDisplay(void){return (void*)1;}
uint32_t ssrnx_eglQueryContext(void*d,void*c,int32_t a,int32_t*v){(void)d;(void)c;(void)a;if(v)*v=2;return STUB_TRUE;}
uint32_t ssrnx_eglWaitGL(void){return STUB_TRUE;}
uint32_t ssrnx_eglWaitNative(int32_t e){(void)e;return STUB_TRUE;}
uint32_t ssrnx_eglSwapBuffers(void*d,void*s){(void)d;(void)s;return STUB_TRUE;}
uint32_t ssrnx_eglCopyBuffers(void*d,void*s,void*t){(void)d;(void)s;(void)t;return 0;}
int32_t ssrnx_eglGetError(void){return 0x3000;}
#endif

typedef struct graphics_SsrnxEglExport { const char *name; uintptr_t addr; } graphics_SsrnxEglExport;
#define FADDR(fn) ((uintptr_t)(void (*)(void))(fn))
static const graphics_SsrnxEglExport k_egl_exports[] = {
 {"eglGetDisplay",FADDR(ssrnx_eglGetDisplay)}, {"eglInitialize",FADDR(ssrnx_eglInitialize)},
 {"eglTerminate",FADDR(ssrnx_eglTerminate)}, {"eglQueryString",FADDR(ssrnx_eglQueryString)},
 {"eglGetConfigs",FADDR(ssrnx_eglGetConfigs)}, {"eglChooseConfig",FADDR(ssrnx_eglChooseConfig)},
 {"eglGetConfigAttrib",FADDR(ssrnx_eglGetConfigAttrib)}, {"eglCreateWindowSurface",FADDR(ssrnx_eglCreateWindowSurface)},
 {"eglCreatePbufferSurface",FADDR(ssrnx_eglCreatePbufferSurface)}, {"eglCreatePixmapSurface",FADDR(ssrnx_eglCreatePixmapSurface)},
 {"eglDestroySurface",FADDR(ssrnx_eglDestroySurface)}, {"eglQuerySurface",FADDR(ssrnx_eglQuerySurface)},
 {"eglBindAPI",FADDR(ssrnx_eglBindAPI)}, {"eglQueryAPI",FADDR(ssrnx_eglQueryAPI)},
 {"eglWaitClient",FADDR(ssrnx_eglWaitClient)}, {"eglReleaseThread",FADDR(ssrnx_eglReleaseThread)},
 {"eglSurfaceAttrib",FADDR(ssrnx_eglSurfaceAttrib)}, {"eglBindTexImage",FADDR(ssrnx_eglBindTexImage)},
 {"eglReleaseTexImage",FADDR(ssrnx_eglReleaseTexImage)}, {"eglSwapInterval",FADDR(ssrnx_eglSwapInterval)},
 {"eglCreateContext",FADDR(ssrnx_eglCreateContext)}, {"eglDestroyContext",FADDR(ssrnx_eglDestroyContext)},
 {"eglMakeCurrent",FADDR(ssrnx_eglMakeCurrent)}, {"eglGetCurrentContext",FADDR(ssrnx_eglGetCurrentContext)},
 {"eglGetCurrentSurface",FADDR(ssrnx_eglGetCurrentSurface)}, {"eglGetCurrentDisplay",FADDR(ssrnx_eglGetCurrentDisplay)},
 {"eglQueryContext",FADDR(ssrnx_eglQueryContext)}, {"eglWaitGL",FADDR(ssrnx_eglWaitGL)},
 {"eglWaitNative",FADDR(ssrnx_eglWaitNative)}, {"eglSwapBuffers",FADDR(ssrnx_eglSwapBuffers)},
 {"eglCopyBuffers",FADDR(ssrnx_eglCopyBuffers)}, {"eglGetError",FADDR(ssrnx_eglGetError)},
 {"eglGetProcAddress",FADDR(ssrnx_eglGetProcAddress)},
 {"eglPresentationTimeANDROID",FADDR(ssrnx_eglPresentationTimeANDROID)}
};

/* Diagnostic count for the GLES bridge. Resolution is intentionally not
 * restricted to this list: any gl* entry exposed by Switch Mesa is eligible.
 * These names represent the common GLES2/3 core surface SSRNX expects to see. */
static const char *const k_common_gl_names[] = {
 "glActiveTexture","glAttachShader","glBindAttribLocation","glBindBuffer","glBindFramebuffer","glBindRenderbuffer","glBindTexture","glBlendColor","glBlendEquation","glBlendEquationSeparate","glBlendFunc","glBlendFuncSeparate","glBufferData","glBufferSubData","glCheckFramebufferStatus","glClear","glClearColor","glClearDepthf","glClearStencil","glColorMask","glCompileShader","glCompressedTexImage2D","glCompressedTexSubImage2D","glCopyTexImage2D","glCopyTexSubImage2D","glCreateProgram","glCreateShader","glCullFace","glDeleteBuffers","glDeleteFramebuffers","glDeleteProgram","glDeleteRenderbuffers","glDeleteShader","glDeleteTextures","glDepthFunc","glDepthMask","glDepthRangef","glDetachShader","glDisable","glDisableVertexAttribArray","glDrawArrays","glDrawElements","glEnable","glEnableVertexAttribArray","glFinish","glFlush","glFramebufferRenderbuffer","glFramebufferTexture2D","glFrontFace","glGenBuffers","glGenerateMipmap","glGenFramebuffers","glGenRenderbuffers","glGenTextures","glGetActiveAttrib","glGetActiveUniform","glGetAttachedShaders","glGetAttribLocation","glGetBooleanv","glGetBufferParameteriv","glGetError","glGetFloatv","glGetFramebufferAttachmentParameteriv","glGetIntegerv","glGetProgramInfoLog","glGetProgramiv","glGetRenderbufferParameteriv","glGetShaderInfoLog","glGetShaderiv","glGetShaderPrecisionFormat","glGetShaderSource","glGetString","glGetTexParameterfv","glGetTexParameteriv","glGetUniformfv","glGetUniformiv","glGetUniformLocation","glGetVertexAttribfv","glGetVertexAttribiv","glGetVertexAttribPointerv","glHint","glIsBuffer","glIsEnabled","glIsFramebuffer","glIsProgram","glIsRenderbuffer","glIsShader","glIsTexture","glLineWidth","glLinkProgram","glPixelStorei","glPolygonOffset","glReadPixels","glReleaseShaderCompiler","glRenderbufferStorage","glSampleCoverage","glScissor","glShaderBinary","glShaderSource","glStencilFunc","glStencilFuncSeparate","glStencilMask","glStencilMaskSeparate","glStencilOp","glStencilOpSeparate","glTexImage2D","glTexParameterf","glTexParameterfv","glTexParameteri","glTexParameteriv","glTexSubImage2D","glUniform1f","glUniform1fv","glUniform1i","glUniform1iv","glUniform2f","glUniform2fv","glUniform2i","glUniform2iv","glUniform3f","glUniform3fv","glUniform3i","glUniform3iv","glUniform4f","glUniform4fv","glUniform4i","glUniform4iv","glUniformMatrix2fv","glUniformMatrix3fv","glUniformMatrix4fv","glUseProgram","glValidateProgram","glVertexAttrib1f","glVertexAttrib1fv","glVertexAttrib2f","glVertexAttrib2fv","glVertexAttrib3f","glVertexAttrib3fv","glVertexAttrib4f","glVertexAttrib4fv","glVertexAttribPointer","glViewport",
 "glReadBuffer","glDrawRangeElements","glTexImage3D","glTexSubImage3D","glCopyTexSubImage3D","glCompressedTexImage3D","glCompressedTexSubImage3D","glGenQueries","glDeleteQueries","glIsQuery","glBeginQuery","glEndQuery","glGetQueryiv","glGetQueryObjectuiv","glUnmapBuffer","glGetBufferPointerv","glDrawBuffers","glUniformMatrix2x3fv","glUniformMatrix3x2fv","glUniformMatrix2x4fv","glUniformMatrix4x2fv","glUniformMatrix3x4fv","glUniformMatrix4x3fv","glBlitFramebuffer","glRenderbufferStorageMultisample","glFramebufferTextureLayer","glMapBufferRange","glFlushMappedBufferRange","glBindVertexArray","glDeleteVertexArrays","glGenVertexArrays","glIsVertexArray","glGetIntegeri_v","glBeginTransformFeedback","glEndTransformFeedback","glBindBufferRange","glBindBufferBase","glTransformFeedbackVaryings","glGetTransformFeedbackVarying","glVertexAttribIPointer","glGetVertexAttribIiv","glGetVertexAttribIuiv","glVertexAttribI4i","glVertexAttribI4ui","glVertexAttribI4iv","glVertexAttribI4uiv","glGetUniformuiv","glGetFragDataLocation","glUniform1ui","glUniform2ui","glUniform3ui","glUniform4ui","glUniform1uiv","glUniform2uiv","glUniform3uiv","glUniform4uiv","glClearBufferiv","glClearBufferuiv","glClearBufferfv","glClearBufferfi","glGetStringi","glCopyBufferSubData","glGetUniformIndices","glGetActiveUniformsiv","glGetUniformBlockIndex","glGetActiveUniformBlockiv","glGetActiveUniformBlockName","glUniformBlockBinding","glDrawArraysInstanced","glDrawElementsInstanced","glFenceSync","glIsSync","glDeleteSync","glClientWaitSync","glWaitSync","glGetInteger64v","glGetSynciv","glGetInteger64i_v","glGetBufferParameteri64v","glGenSamplers","glDeleteSamplers","glIsSampler","glBindSampler","glSamplerParameteri","glSamplerParameteriv","glSamplerParameterf","glSamplerParameterfv","glGetSamplerParameteriv","glGetSamplerParameterfv","glVertexAttribDivisor","glBindTransformFeedback","glDeleteTransformFeedbacks","glGenTransformFeedbacks","glIsTransformFeedback","glPauseTransformFeedback","glResumeTransformFeedback","glProgramParameteri","glInvalidateFramebuffer","glInvalidateSubFramebuffer","glTexStorage2D","glTexStorage3D","glGetInternalformativ"
};

static uintptr_t lookup_egl_wrapper(const char *name) {
    for (size_t i=0;i<sizeof(k_egl_exports)/sizeof(k_egl_exports[0]);++i)
        if (strcmp(name,k_egl_exports[i].name)==0) return k_egl_exports[i].addr;
    return 0;
}

void *ssrnx_eglGetProcAddress(const char *name) {
    if (!name) return NULL;
    uintptr_t wrapped = lookup_egl_wrapper(name);
#ifdef __SWITCH__
    if (!wrapped) wrapped = graphics_lookup_gl_direct(name, NULL);
    void *result = wrapped ? (void *)wrapped : (void *)eglGetProcAddress(name);
    if (eglGetCurrentContext() != EGL_NO_CONTEXT &&
        atomic_fetch_add(&graphics_g_gldiag_proc_reports, 1u) < 64u)
        SSRNX_LOGCI("GLDIAG", "eglGetProcAddress name=%.128s result=%p context=%p caller=%p",
                    name, result, (void *)eglGetCurrentContext(), __builtin_return_address(0));
    return result;
#else
    return (void *)wrapped;
#endif
}

SsrnxResult ssrnx_graphics_resolve(const char *name, uintptr_t *out_address, const char **out_provider) {
    if (!name || !out_address) return SSRNX_ERR_ARGUMENT;
    uintptr_t wrapped = lookup_egl_wrapper(name);
    if (wrapped) {
        *out_address = wrapped;
        if (out_provider) *out_provider = "libEGL.so";
        return SSRNX_OK;
    }
    if (name[0]=='g' && name[1]=='l') {
#ifdef __SWITCH__
        const char *provider = NULL;
        uintptr_t direct = graphics_lookup_gl_direct(name, &provider);
        if (direct) {
            *out_address = direct;
            if (out_provider) *out_provider = provider;
            return SSRNX_OK;
        }
        void *p=(void *)eglGetProcAddress(name);
        if (!p) return SSRNX_ERR_SYMBOL_NOT_FOUND;
        *out_address=(uintptr_t)p;
        if (out_provider) *out_provider = "libGLESv2.so";
        return SSRNX_OK;
#else
        return SSRNX_ERR_SYMBOL_NOT_FOUND;
#endif
    }
    return SSRNX_ERR_SYMBOL_NOT_FOUND;
}

size_t ssrnx_graphics_export_count(void) {
    return sizeof(k_egl_exports)/sizeof(k_egl_exports[0]) + sizeof(k_common_gl_names)/sizeof(k_common_gl_names[0]);
}
uint64_t ssrnx_graphics_present_count(void) { return graphics_g_ssrnx_present_count; }

void ssrnx_graphics_reset(void) {
    graphics_g_ssrnx_present_count = 0;
#ifdef __SWITCH__
    graphics_gldiag_vertex_reset();
    graphics_software_framebuffer_reset();
    /*
     * Recovery path as well as normal teardown.  eglGetCurrentDisplay() is
     * EGL_NO_DISPLAY until a context has been made current, so it is not
     * sufficient after a failure between eglInitialize() and
     * eglMakeCurrent().  Track every display SSRNX successfully initialized
     * and terminate it here even if setup aborted early.
     */
    EGLDisplay current = eglGetCurrentDisplay();
    if (current != EGL_NO_DISPLAY) {
        SSRNX_LOGCI("EGL", "graphics-reset: unbind current display=%p", (void *)current);
        (void)eglMakeCurrent(current, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    portrait_bridge_destroy_resources();
    portrait_bridge_clear_metadata();

    for (size_t i = 0; i < SSRNX_MAX_TRACKED_EGL_DISPLAYS; ++i) {
        EGLDisplay display = graphics_g_initialized_displays[i];
        if (display == EGL_NO_DISPLAY) continue;
        SSRNX_LOGCI("EGL", "graphics-reset: terminate tracked display=%p", (void *)display);
        (void)eglTerminate(display);
        graphics_g_initialized_displays[i] = EGL_NO_DISPLAY;
    }

    free_virtual_pbuffers_for_display(EGL_NO_DISPLAY);
    clear_egl_diag_objects();
    graphics_g_guest_current_display = EGL_NO_DISPLAY;
    graphics_g_guest_current_context = EGL_NO_CONTEXT;
    graphics_g_guest_current_draw = NULL;
    graphics_g_guest_current_read = NULL;
    graphics_g_guest_egl_error = EGL_SUCCESS;

    (void)eglReleaseThread();
    graphics_presentation_window_reset();
    SSRNX_LOGCI("EGL", "graphics-reset: complete");
#endif
    graphics_g_presentation_plan_valid = false;
    memset(&graphics_g_presentation_plan, 0, sizeof(graphics_g_presentation_plan));
}

#pragma pop_macro("STUB_TRUE")
#pragma pop_macro("GLDIAG_VERTEX_LIMIT")
#pragma pop_macro("GLDIAG_MAX_REPORTS")
#pragma pop_macro("GLDIAG_MAX_FORMATS")
#pragma pop_macro("GLDIAG_FORWARD")
#pragma pop_macro("GLADDR")
#pragma pop_macro("FADDR")
#pragma pop_macro("EPTR")
#pragma pop_macro("EBOOL")
#pragma pop_macro("SSRNX_MAX_VIRTUAL_PBUFFERS")
#pragma pop_macro("SSRNX_MAX_TRACKED_EGL_DISPLAYS")
#pragma pop_macro("SSRNX_MAX_EGL_DIAG_SURFACES")
#pragma pop_macro("SSRNX_MAX_EGL_DIAG_CONTEXTS")
#pragma pop_macro("SSRNX_EGL_GUEST_EXTENSION")
#pragma pop_macro("SSRNX_CFG")


/* ---- display ---- */


#include <math.h>
#include <string.h>

static bool display_orientation_is_portrait(int32_t value) {
    return value == SSRNX_SCREEN_ORIENTATION_PORTRAIT ||
           value == SSRNX_SCREEN_ORIENTATION_SENSOR_PORTRAIT ||
           value == SSRNX_SCREEN_ORIENTATION_REVERSE_PORTRAIT ||
           value == SSRNX_SCREEN_ORIENTATION_USER_PORTRAIT;
}

static bool display_orientation_is_landscape(int32_t value) {
    return value == SSRNX_SCREEN_ORIENTATION_LANDSCAPE ||
           value == SSRNX_SCREEN_ORIENTATION_SENSOR_LANDSCAPE ||
           value == SSRNX_SCREEN_ORIENTATION_REVERSE_LANDSCAPE ||
           value == SSRNX_SCREEN_ORIENTATION_USER_LANDSCAPE;
}

static bool display_orientation_is_reverse(int32_t value) {
    return value == SSRNX_SCREEN_ORIENTATION_REVERSE_LANDSCAPE ||
           value == SSRNX_SCREEN_ORIENTATION_REVERSE_PORTRAIT;
}

bool ssrnx_display_make_plan(int32_t screen_orientation,
                             bool orientation_declared,
                             int32_t physical_width,
                             int32_t physical_height,
                             SsrnxDisplayPlan *out) {
    if (!out || physical_width <= 0 || physical_height <= 0) return false;
    memset(out, 0, sizeof(*out));
    out->screen_orientation = screen_orientation;
    out->orientation_declared = orientation_declared;
    out->physical_width = physical_width;
    out->physical_height = physical_height;

    bool portrait = orientation_declared && display_orientation_is_portrait(screen_orientation);
    bool landscape = orientation_declared && display_orientation_is_landscape(screen_orientation);
    if (!portrait && !landscape) {
        /* Unspecified/user/sensor/locked modes follow the current Switch
         * display orientation until SSRNX gains runtime rotation sensors. */
        landscape = physical_width >= physical_height;
        portrait = !landscape;
    }

    out->aspect = portrait ? SSRNX_DISPLAY_PORTRAIT : SSRNX_DISPLAY_LANDSCAPE;
    out->reverse = orientation_declared && display_orientation_is_reverse(screen_orientation);
    out->logical_width = portrait ? physical_height : physical_width;
    out->logical_height = portrait ? physical_width : physical_height;

    /* A portrait Android surface is 720x1280, but the Switch LCD is physically
     * 1280x720. Present the portrait surface rotated over the complete panel so
     * holding the console vertically gives a true full-screen portrait view.
     * Normal portrait is emitted CCW on a landscape-held Switch (turn the
     * console clockwise to view it upright); reversePortrait uses the opposite
     * quarter-turn. */
    if (portrait) {
        out->rotation = out->reverse ? SSRNX_DISPLAY_ROTATION_CW_90
                                     : SSRNX_DISPLAY_ROTATION_CCW_90;
    } else {
        out->rotation = SSRNX_DISPLAY_ROTATION_0;
    }
    out->content_x = 0.0f;
    out->content_y = 0.0f;
    out->content_width = (float)physical_width;
    out->content_height = (float)physical_height;
    return true;
}

int32_t ssrnx_display_android_rotation(const SsrnxDisplayPlan *plan) {
    if (!plan) return 0;
    if (plan->aspect == SSRNX_DISPLAY_PORTRAIT) {
        /* Android reports the rotation of the drawn graphics, opposite the
         * physical device turn. Match the compositor's quarter-turn. */
        return (int32_t)plan->rotation;
    }
    return plan->reverse ? 2 : 0;
}

const char *ssrnx_display_orientation_name(int32_t value, bool declared) {
    if (!declared) return "unspecified";
    switch (value) {
        case SSRNX_SCREEN_ORIENTATION_UNSPECIFIED: return "unspecified";
        case SSRNX_SCREEN_ORIENTATION_LANDSCAPE: return "landscape";
        case SSRNX_SCREEN_ORIENTATION_PORTRAIT: return "portrait";
        case SSRNX_SCREEN_ORIENTATION_USER: return "user";
        case SSRNX_SCREEN_ORIENTATION_BEHIND: return "behind";
        case SSRNX_SCREEN_ORIENTATION_SENSOR: return "sensor";
        case SSRNX_SCREEN_ORIENTATION_NOSENSOR: return "nosensor";
        case SSRNX_SCREEN_ORIENTATION_SENSOR_LANDSCAPE: return "sensorLandscape";
        case SSRNX_SCREEN_ORIENTATION_SENSOR_PORTRAIT: return "sensorPortrait";
        case SSRNX_SCREEN_ORIENTATION_REVERSE_LANDSCAPE: return "reverseLandscape";
        case SSRNX_SCREEN_ORIENTATION_REVERSE_PORTRAIT: return "reversePortrait";
        case SSRNX_SCREEN_ORIENTATION_FULL_SENSOR: return "fullSensor";
        case SSRNX_SCREEN_ORIENTATION_USER_LANDSCAPE: return "userLandscape";
        case SSRNX_SCREEN_ORIENTATION_USER_PORTRAIT: return "userPortrait";
        case SSRNX_SCREEN_ORIENTATION_FULL_USER: return "fullUser";
        case SSRNX_SCREEN_ORIENTATION_LOCKED: return "locked";
        default: return "unknown";
    }
}

const char *ssrnx_display_resolved_orientation_name(int32_t value,
                                                    bool declared,
                                                    int32_t physical_width,
                                                    int32_t physical_height) {
    if (declared && (display_orientation_is_portrait(value) || display_orientation_is_landscape(value))) {
        return ssrnx_display_orientation_name(value, true);
    }
    SsrnxDisplayPlan plan;
    if (!ssrnx_display_make_plan(value, declared, physical_width, physical_height, &plan)) {
        return "unknown";
    }
    return plan.aspect == SSRNX_DISPLAY_PORTRAIT ? "auto portrait" : "auto landscape";
}

bool ssrnx_display_map_touch(const SsrnxDisplayPlan *plan,
                             float physical_x,
                             float physical_y,
                             float *out_x,
                             float *out_y) {
    if (!plan || !out_x || !out_y || plan->content_width <= 0.0f ||
        plan->content_height <= 0.0f || plan->logical_width <= 0 ||
        plan->logical_height <= 0) return false;

    if (physical_x < plan->content_x || physical_y < plan->content_y ||
        physical_x >= plan->content_x + plan->content_width ||
        physical_y >= plan->content_y + plan->content_height) {
        return false;
    }

    const float nx = (physical_x - plan->content_x) / plan->content_width;
    const float ny = (physical_y - plan->content_y) / plan->content_height;
    float x = 0.0f, y = 0.0f;

    switch (plan->rotation) {
        case SSRNX_DISPLAY_ROTATION_CCW_90:
            /* Inverse of a counter-clockwise presentation rotation. Physical
             * top-left coordinates map into Android's top-left coordinate space. */
            x = (1.0f - ny) * (float)plan->logical_width;
            y = nx * (float)plan->logical_height;
            break;
        case SSRNX_DISPLAY_ROTATION_CW_90:
            x = ny * (float)plan->logical_width;
            y = (1.0f - nx) * (float)plan->logical_height;
            break;
        case SSRNX_DISPLAY_ROTATION_0:
        default:
            x = nx * (float)plan->logical_width;
            y = ny * (float)plan->logical_height;
            if (plan->reverse) {
                x = (float)plan->logical_width - x;
                y = (float)plan->logical_height - y;
            }
            break;
    }

    if (x < 0.0f) x = 0.0f;
    if (y < 0.0f) y = 0.0f;
    if (x > (float)plan->logical_width - 1.0f) x = (float)plan->logical_width - 1.0f;
    if (y > (float)plan->logical_height - 1.0f) y = (float)plan->logical_height - 1.0f;
    *out_x = x;
    *out_y = y;
    return true;
}



/* ---- shader_compat ---- */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { UNKNOWN, SIGNED, UNSIGNED };
typedef struct shader_compat_Token { const char *text; size_t length; } shader_compat_Token;
typedef struct shader_compat_Expr { unsigned type; size_t literal_end; } shader_compat_Expr;
typedef struct shader_compat_Parser {
    const char *source, *cursor;
    shader_compat_Token token;
    size_t edits[128], count;
    bool failed;
} shader_compat_Parser;
static bool space(char c) { return c==' ' || c=='\t' || c=='\r' || c=='\n' || c=='\f' || c=='\v'; }
static bool digit(char c) { return c>='0' && c<='9'; }
static bool letter(char c) { return (c>='a' && c<='z') || (c>='A' && c<='Z') || c=='_'; }
static bool shader_compat_equal(shader_compat_Token t,const char *word) { return strlen(word)==t.length && !memcmp(t.text,word,t.length); }
static void advance(shader_compat_Parser *p) {
    const char *s=p->cursor;
    for (;;) {
        while (space(*s)) ++s;
        if (s[0]=='/' && s[1]=='/') { while (*s && *s!='\n') ++s; }
        else if (s[0]=='/' && s[1]=='*') {
            s+=2;while (*s && !(s[0]=='*' && s[1]=='/')) ++s;
            if (*s) s+=2;else p->failed=true;
        } else break;
    }
    p->token.text=s;
    if (letter(*s)) { do { ++s; } while (letter(*s) || digit(*s)); }
    else if (digit(*s)) {
        /* Keep float/hex/exponent tokens whole; only integer literals qualify. */
        const bool hex=s[0]=='0' && (s[1]=='x' || s[1]=='X');
        do {
            const char c=*s++;
            if (!hex && (c=='e' || c=='E') && (*s=='+' || *s=='-')) ++s;
        } while (letter(*s) || digit(*s) || *s=='.');
    } else if (*s) {
        const char c=*s++;
        if (*s && ((c==*s && strchr("<>=!&|+-",c)) || (*s=='=' && strchr("<>!=",c)))) ++s;
    }
    p->token.length=(size_t)(s-p->token.text);p->cursor=s;
}
static shader_compat_Expr integer(shader_compat_Token t,const char *source) {
    shader_compat_Expr result={0};
    if (!t.length || !digit(*t.text)) return result;
    size_t length=t.length;
    const bool suffix=t.text[length-1]=='u' || t.text[length-1]=='U';
    if (suffix) --length;
    unsigned base=10;size_t i=0;
    if (length>1 && t.text[0]=='0') {
        base=8;
        if (length>2 && (t.text[1]=='x' || t.text[1]=='X')) { base=16;i=2; }
    }
    uint64_t value=0;
    for (;i<length;++i) {
        const unsigned char c=(unsigned char)t.text[i];
        const unsigned n=digit((char)c) ? (unsigned)(c-'0') : c>='a' && c<='f' ? c-'a'+10u : c>='A' && c<='F' ? c-'A'+10u : 99u;
        if (n>=base || value>(UINT32_MAX-n)/base) return result;
        value=value*base+n;
    }
    if (suffix) result.type=UNSIGNED;
    else if (value<=INT32_MAX) { result.type=SIGNED;result.literal_end=(size_t)(t.text-source)+t.length; }
    return result;
}
static int precedence(shader_compat_Token t) {
    if (shader_compat_equal(t,"|")) return 1;
    if (shader_compat_equal(t,"^")) return 2;
    if (shader_compat_equal(t,"&")) return 3;
    if (shader_compat_equal(t,"<<") || shader_compat_equal(t,">>")) return 4;
    if (shader_compat_equal(t,"+") || shader_compat_equal(t,"-")) return 5;
    if (shader_compat_equal(t,"*") || shader_compat_equal(t,"/") || shader_compat_equal(t,"%")) return 6;
    return 0;
}
static shader_compat_Expr shader_compat_expression(shader_compat_Parser *,int,unsigned);
static shader_compat_Expr shader_compat_atom(shader_compat_Parser *p,unsigned depth) {
    shader_compat_Expr result={0};shader_compat_Token t=p->token;
    if (depth>32 || !t.length) { p->failed=true;return result; }
    if (shader_compat_equal(t,"+") || shader_compat_equal(t,"-") || shader_compat_equal(t,"~")) {
        advance(p);result=shader_compat_atom(p,depth+1);
        if (!shader_compat_equal(t,"+")) result.literal_end=0;
        return result;
    }
    if (shader_compat_equal(t,"(")) {
        advance(p);result=shader_compat_expression(p,1,depth+1);
        if (!shader_compat_equal(p->token,")")) p->failed=true;else advance(p);
    } else if (digit(*t.text)) { result=integer(t,p->source);advance(p); }
    else if (letter(*t.text)) {
        advance(p);
        if (shader_compat_equal(p->token,"(")) {
            /* Function arguments keep their own types and overload selection. */
            unsigned nesting=1;
            while (nesting && !p->failed) {
                advance(p);
                if (!p->token.length) { p->failed=true;break; }
                if (shader_compat_equal(p->token,"(") && ++nesting>32) p->failed=true;
                if (shader_compat_equal(p->token,")")) --nesting;
            }
            if (!p->failed) advance(p);
            if (shader_compat_equal(t,"uint")) result.type=UNSIGNED;
            if (shader_compat_equal(t,"int")) result.type=SIGNED;
        }
    } else { p->failed=true;return result; }
    while (shader_compat_equal(p->token,".") || shader_compat_equal(p->token,"[")) {
        result=(shader_compat_Expr){0};
        if (shader_compat_equal(p->token,".")) {
            advance(p);if (!p->token.length || !letter(*p->token.text)) { p->failed=true;break; }
            advance(p);
        } else {
            unsigned nesting=1;
            while (nesting && !p->failed) {
                advance(p);
                if (!p->token.length) { p->failed=true;break; }
                if (shader_compat_equal(p->token,"[") && ++nesting>32) p->failed=true;
                if (shader_compat_equal(p->token,"]")) --nesting;
            }
            if (!p->failed) advance(p);
        }
    }
    return result;
}
static void shader_compat_convert_literal(shader_compat_Parser *p,shader_compat_Expr e) {
    if (p->count==sizeof(p->edits)/sizeof(*p->edits)) { p->failed=true;return; }
    p->edits[p->count++]=e.literal_end;
}
static shader_compat_Expr shader_compat_expression(shader_compat_Parser *p,int minimum,unsigned depth) {
    shader_compat_Expr left=shader_compat_atom(p,depth);
    for (int priority;!p->failed && (priority=precedence(p->token))>=minimum;) {
        const bool shift=shader_compat_equal(p->token,"<<") || shader_compat_equal(p->token,">>");
        advance(p);shader_compat_Expr right=shader_compat_expression(p,priority+1,depth+1);
        if (shift) { left.literal_end=0;continue; } /* Shift counts need not match. */
        if (left.type==UNSIGNED && right.type==SIGNED && right.literal_end) shader_compat_convert_literal(p,right);
        if (right.type==UNSIGNED && left.type==SIGNED && left.literal_end) shader_compat_convert_literal(p,left);
        /* Unknown variables must already match their unsigned operand or the
         * real compiler will still reject the retry. No variable casts. */
        left.type=left.type==UNSIGNED || right.type==UNSIGNED ? UNSIGNED : UNKNOWN;
        left.literal_end=0;
    }
    return left;
}
char *ssrnx_shader_compat_source(const char *source,size_t length,size_t *conversions) {
    if (conversions) *conversions=0;
    if (!source || !length || length>1024u*1024u || memchr(source,0,length)) return NULL;
    char *copy=malloc(length+1);
    if (!copy) return NULL;
    memcpy(copy,source,length);copy[length]=0;
    shader_compat_Parser scanner={.source=copy,.cursor=copy};advance(&scanner);
    if (!shader_compat_equal(scanner.token,"#")) { free(copy);return NULL; }
    advance(&scanner);if (!shader_compat_equal(scanner.token,"version")) { free(copy);return NULL; }
    advance(&scanner);if (!shader_compat_equal(scanner.token,"300")) { free(copy);return NULL; }
    advance(&scanner);if (!shader_compat_equal(scanner.token,"es")) { free(copy);return NULL; }
    unsigned char *insert=calloc(length+1,1);
    if (!insert) { free(copy);return NULL; }
    while (scanner.token.length && !scanner.failed) {
        if (shader_compat_equal(scanner.token,"#")) {
            /* Macro bodies and continued preprocessor lines stay byte-exact. */
            const char *s=scanner.cursor;
            while (*s) {
                if (*s++=='\n' && (s-copy<2 || s[-2]!='\\') &&
                    (s-copy<3 || s[-2]!='\r' || s[-3]!='\\')) break;
            }
            scanner.cursor=s;
        } else if (shader_compat_equal(scanner.token,"[")) {
            shader_compat_Parser index=scanner;index.count=0;advance(&index);
            (void)shader_compat_expression(&index,1,0);
            if (!index.failed && shader_compat_equal(index.token,"]"))
                for (size_t i=0;i<index.count;++i) insert[index.edits[i]]=1;
        }
        advance(&scanner);
    }
    size_t count=0;
    for (size_t i=0;i<=length;++i) count+=insert[i];
    char *output=count && !scanner.failed ? malloc(length+count+1) : NULL;
    if (output) {
        size_t n=0;
        for (size_t i=0;i<=length;++i) { if (insert[i]) output[n++]='u';output[n++]=copy[i]; }
        if (conversions) *conversions=count;
    }
    free(insert);free(copy);return output;
}



/* ---- java_egl ---- */
#pragma push_macro("D_CONFIG")
#pragma push_macro("D_CONTEXT")
#pragma push_macro("D_DISPLAY")
#pragma push_macro("D_SURFACE")
#pragma push_macro("EGL10_IFACE")
#pragma push_macro("EGL11_IFACE")
#pragma push_macro("EGL_CONFIG_CLASS")
#pragma push_macro("EGL_CONTEXT_CLASS")
#pragma push_macro("EGL_DISPLAY_CLASS")
#pragma push_macro("EGL_FN")
#pragma push_macro("EGL_HANDLE_MAGIC")
#pragma push_macro("EGL_IFACE")
#pragma push_macro("EGL_IMPL_CLASS")
#pragma push_macro("EGL_NONE_")
#pragma push_macro("EGL_PKG")
#pragma push_macro("EGL_SURFACE_CLASS")
#pragma push_macro("GL10_CLASS")
#pragma push_macro("GL_IFACE")
/* javax.microedition.khronos.egl (JSR-239) binding over SSRNX's guest EGL.
 *
 * Android exposes EGL to Java through EGL10/EGL11, implemented in AOSP by
 * com.google.android.gles_jni.EGLImpl. Apps and engines that ship their own
 * GLSurfaceView fork (GL thread + EglHelper) drive the display, config,
 * surface and context lifecycle through this API instead of
 * android.opengl.GLSurfaceView. Handles are opaque Java objects wrapping the
 * native EGL handle; the EGL10 "no object" constants are null (AOSP EGL10).
 */
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>






#define EGL_PKG "javax/microedition/khronos/egl/"
#define EGL_IFACE EGL_PKG "EGL"
#define EGL10_IFACE EGL_PKG "EGL10"
#define EGL11_IFACE EGL_PKG "EGL11"
#define EGL_DISPLAY_CLASS EGL_PKG "EGLDisplay"
#define EGL_CONFIG_CLASS EGL_PKG "EGLConfig"
#define EGL_SURFACE_CLASS EGL_PKG "EGLSurface"
#define EGL_CONTEXT_CLASS EGL_PKG "EGLContext"
#define EGL_IMPL_CLASS "com/google/android/gles_jni/EGLImpl"
#define GL_IFACE "javax/microedition/khronos/opengles/GL"
#define GL10_CLASS "javax/microedition/khronos/opengles/GL10"

#define D_DISPLAY "L" EGL_DISPLAY_CLASS ";"
#define D_CONFIG "L" EGL_CONFIG_CLASS ";"
#define D_SURFACE "L" EGL_SURFACE_CLASS ";"
#define D_CONTEXT "L" EGL_CONTEXT_CLASS ";"

#define EGL_HANDLE_MAGIC UINT64_C(0x41504b3645474c48) /* APK6EGLH */

/* EGL 1.4 EGL_NONE (Khronos egl.h): attribute-list terminator. */
#define EGL_NONE_ 0x3038

typedef struct java_egl_EglHandle {
    uint64_t magic;
    void *handle;
} java_egl_EglHandle;

static pthread_mutex_t java_egl_g_egl_lock = PTHREAD_MUTEX_INITIALIZER;
static void *java_egl_g_egl_instance;

static SsrnxJniValue java_egl_v_void(void) { SsrnxJniValue v = {0}; return v; }
static SsrnxJniValue java_egl_v_bool(bool b) { SsrnxJniValue v = {0}; v.z = b ? 1u : 0u; return v; }
static SsrnxJniValue java_egl_v_int(int32_t i) { SsrnxJniValue v = {0}; v.i = i; return v; }
static SsrnxJniValue java_egl_v_object(void *l) { SsrnxJniValue v = {0}; v.l = l; return v; }

static void java_egl_throw_iae(void *env, const char *message) {
    (void)ssrnx_jni_throw_new_by_name(env, "java/lang/IllegalArgumentException", message);
}

/* Wraps a native handle; a null handle maps to the EGL10 EGL_NO_* constant. */
static void *java_egl_wrap_handle(void *env, const char *cls, void *handle) {
    if (!handle) return NULL;
    void *obj = ssrnx_jni_new_local_object(env, cls);
    java_egl_EglHandle *h = obj ? calloc(1, sizeof(*h)) : NULL;
    if (!h) {
        if (obj) ssrnx_jni_delete_local_ref_host(env, obj);
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/OutOfMemoryError", cls);
        return NULL;
    }
    h->magic = EGL_HANDLE_MAGIC;
    h->handle = handle;
    (void)ssrnx_jni_set_object_host_data(obj, h);
    return obj;
}

static void *java_egl_handle_of(void *ref) {
    const java_egl_EglHandle *h = ref ? ssrnx_jni_get_object_host_data(ref) : NULL;
    return h && h->magic == EGL_HANDLE_MAGIC ? h->handle : NULL;
}

static void *java_egl_handle_of_token(void *token) {
    const java_egl_EglHandle *h = token ? ssrnx_jni_get_token_host_data(token) : NULL;
    return h && h->magic == EGL_HANDLE_MAGIC ? h->handle : NULL;
}

/* int[] storage (SSRNX primitive arrays never move) with a minimum length. */
static int32_t *java_egl_int_array(void *env, void *array, int32_t min_length, const char *what) {
    if (!array || ssrnx_jni_array_length_host(array) < min_length) {
        char message[128];
        snprintf(message, sizeof(message), "%s == null or %s.length < %d", what, what, (int)min_length);
        java_egl_throw_iae(env, message);
        return NULL;
    }
    return ssrnx_jni_primitive_array_data_host(array);
}

/* AOSP EGLImpl rejects attribute lists that are not EGL_NONE terminated. */
static bool java_egl_attrib_list_valid(void *env, void *array, const int32_t **out) {
    *out = NULL;
    if (!array) return true;
    const int32_t n = ssrnx_jni_array_length_host(array);
    const int32_t *attrs = ssrnx_jni_primitive_array_data_host(array);
    for (int32_t i = 0; attrs && i < n; i += 2) {
        if (attrs[i] == EGL_NONE_) { *out = attrs; return true; }
    }
    java_egl_throw_iae(env, "attrib_list must contain EGL_NONE!");
    return false;
}

#define EGL_FN(name) static SsrnxJniValue name(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u)

EGL_FN(egl_get_display) {
    (void)t; (void)u; (void)a;
    if (n != 1) return java_egl_v_object(NULL);
    /* EGLImpl ignores native_display: Android has one default display. */
    return java_egl_v_object(java_egl_wrap_handle(env, EGL_DISPLAY_CLASS, ssrnx_eglGetDisplay(NULL)));
}

EGL_FN(egl_initialize) {
    (void)t; (void)u;
    if (n != 2) return java_egl_v_bool(false);
    int32_t *version = NULL;
    if (a[1].l && !(version = java_egl_int_array(env, a[1].l, 2, "major_minor"))) return java_egl_v_bool(false);
    int32_t major = 0, minor = 0;
    const bool ok = ssrnx_eglInitialize(java_egl_handle_of(a[0].l), &major, &minor) != 0;
    if (ok && version) { version[0] = major; version[1] = minor; }
    return java_egl_v_bool(ok);
}

EGL_FN(egl_terminate) {
    (void)env; (void)t; (void)u;
    return java_egl_v_bool(n == 1 && ssrnx_eglTerminate(java_egl_handle_of(a[0].l)) != 0);
}

EGL_FN(egl_query_string) {
    (void)t; (void)u;
    if (n != 2) return java_egl_v_object(NULL);
    const char *s = ssrnx_eglQueryString(java_egl_handle_of(a[0].l), a[1].i);
    return java_egl_v_object(s ? ssrnx_jni_new_local_string_utf(env, s) : NULL);
}

/* Shared tail of eglGetConfigs/eglChooseConfig: wrap the native configs into
 * the caller's EGLConfig[] (when non-null) and report the count. */
static bool java_egl_configs_out(void *env, void **configs, int32_t count, int32_t capacity, void *array, int32_t *num) {
    /* Broken EGL drivers must not write beyond the Java caller's array. */
    if (count < 0 || (array && count > capacity)) {
        java_egl_throw_iae(env, "EGL returned invalid config count");
        return false;
    }
    if (num) *num = count;
    for (int32_t i = 0; array && i < count; ++i) {
        void *cfg = java_egl_wrap_handle(env, EGL_CONFIG_CLASS, configs[i]);
        if (!cfg && configs[i]) return false;
        (void)ssrnx_jni_set_object_array_element_host(env, array, i, cfg);
        if (cfg) ssrnx_jni_delete_local_ref_host(env, cfg);
    }
    return true;
}

static bool java_egl_config_args(void *env, void *array, int32_t config_size, void *num_array,
                        int32_t **out_num, void ***out_configs, int32_t *out_cap) {
    /* AOSP EGLImpl allows num_config == null; EGL still needs a native count. */
    *out_num = num_array ? java_egl_int_array(env, num_array, 1, "num_config") : NULL;
    if (num_array && !*out_num) return false;
    if (config_size < 0) { java_egl_throw_iae(env, "config_size < 0"); return false; }
    if (array && ssrnx_jni_array_length_host(array) < config_size) {
        java_egl_throw_iae(env, "configs.length < config_size");
        return false;
    }
    *out_cap = array ? config_size : 0;
    *out_configs = NULL;
    if (*out_cap > 0 && !(*out_configs = calloc((size_t)*out_cap, sizeof(void *)))) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/OutOfMemoryError", "EGLConfig[]");
        return false;
    }
    return true;
}

EGL_FN(egl_get_configs) {
    (void)t; (void)u;
    if (n != 4) return java_egl_v_bool(false);
    int32_t *num = NULL, cap = 0, count = 0;
    void **configs = NULL;
    if (!java_egl_config_args(env, a[1].l, a[2].i, a[3].l, &num, &configs, &cap)) return java_egl_v_bool(false);
    bool ok = ssrnx_eglGetConfigs(java_egl_handle_of(a[0].l), configs, cap, &count) != 0;
    if (ok) ok = java_egl_configs_out(env, configs, configs ? count : 0, cap, a[1].l, num);
    if (ok && !configs && num) *num = count;
    free(configs);
    return java_egl_v_bool(ok);
}

EGL_FN(egl_choose_config) {
    (void)t; (void)u;
    if (n != 5) return java_egl_v_bool(false);
    const int32_t *attrs = NULL;
    if (!java_egl_attrib_list_valid(env, a[1].l, &attrs)) return java_egl_v_bool(false);
    int32_t *num = NULL, cap = 0, count = 0;
    void **configs = NULL;
    if (!java_egl_config_args(env, a[2].l, a[3].i, a[4].l, &num, &configs, &cap)) return java_egl_v_bool(false);
    bool ok = ssrnx_eglChooseConfig(java_egl_handle_of(a[0].l), attrs, configs, cap, &count) != 0;
    if (ok) ok = java_egl_configs_out(env, configs, configs ? count : 0, cap, a[2].l, num);
    if (ok && !configs && num) *num = count;
    free(configs);
    return java_egl_v_bool(ok);
}

EGL_FN(egl_get_config_attrib) {
    (void)t; (void)u;
    if (n != 4) return java_egl_v_bool(false);
    int32_t *value = java_egl_int_array(env, a[3].l, 1, "value");
    if (!value) return java_egl_v_bool(false);
    return java_egl_v_bool(ssrnx_eglGetConfigAttrib(java_egl_handle_of(a[0].l), java_egl_handle_of(a[1].l), a[2].i, value) != 0);
}

/* EGLImpl accepts a Surface, SurfaceView, SurfaceHolder or SurfaceTexture.
 * All of them name the single process window on Horizon. */
static bool java_egl_native_window_object(void *obj) {
    static const char *const kinds[] = {
        "android/view/Surface", "android/view/SurfaceView",
        "android/view/SurfaceHolder", "android/graphics/SurfaceTexture",
    };
    for (size_t i = 0; obj && i < sizeof(kinds) / sizeof(kinds[0]); ++i)
        if (ssrnx_jni_is_instance_of_class_host(obj, kinds[i])) return true;
    return false;
}

EGL_FN(egl_create_window_surface) {
    (void)t; (void)u;
    if (n != 4) return java_egl_v_object(NULL);
    if (!java_egl_native_window_object(a[2].l)) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/UnsupportedOperationException",
            "eglCreateWindowSurface() can only be called with an instance of Surface, "
            "SurfaceView, SurfaceTexture or SurfaceHolder at the moment, this will be fixed later.");
        return java_egl_v_object(NULL);
    }
    const int32_t *attrs = NULL;
    if (!java_egl_attrib_list_valid(env, a[3].l, &attrs)) return java_egl_v_object(NULL);
    void *surface = ssrnx_eglCreateWindowSurface(java_egl_handle_of(a[0].l), java_egl_handle_of(a[1].l),
                                                 ssrnx_native_window(), attrs);
    SSRNX_LOGCI("JAVA-EGL", "eglCreateWindowSurface surface=%p", surface);
    return java_egl_v_object(java_egl_wrap_handle(env, EGL_SURFACE_CLASS, surface));
}

EGL_FN(egl_create_pbuffer_surface) {
    (void)t; (void)u;
    if (n != 3) return java_egl_v_object(NULL);
    const int32_t *attrs = NULL;
    if (!java_egl_attrib_list_valid(env, a[2].l, &attrs)) return java_egl_v_object(NULL);
    return java_egl_v_object(java_egl_wrap_handle(env, EGL_SURFACE_CLASS,
                                ssrnx_eglCreatePbufferSurface(java_egl_handle_of(a[0].l), java_egl_handle_of(a[1].l), attrs)));
}

EGL_FN(egl_create_pixmap_surface) {
    (void)env; (void)t; (void)u; (void)a; (void)n;
    /* No native pixmaps exist on Android either (EGLImpl returns EGL_NO_SURFACE). */
    return java_egl_v_object(NULL);
}

EGL_FN(egl_destroy_surface) {
    (void)env; (void)t; (void)u;
    return java_egl_v_bool(n == 2 && ssrnx_eglDestroySurface(java_egl_handle_of(a[0].l), java_egl_handle_of(a[1].l)) != 0);
}

EGL_FN(egl_query_surface) {
    (void)t; (void)u;
    if (n != 4) return java_egl_v_bool(false);
    int32_t *value = java_egl_int_array(env, a[3].l, 1, "value");
    return java_egl_v_bool(value && ssrnx_eglQuerySurface(java_egl_handle_of(a[0].l), java_egl_handle_of(a[1].l), a[2].i, value) != 0);
}

EGL_FN(egl_create_context) {
    (void)t; (void)u;
    if (n != 4) return java_egl_v_object(NULL);
    const int32_t *attrs = NULL;
    if (!java_egl_attrib_list_valid(env, a[3].l, &attrs)) return java_egl_v_object(NULL);
    void *context = ssrnx_eglCreateContext(java_egl_handle_of(a[0].l), java_egl_handle_of(a[1].l), java_egl_handle_of(a[2].l), attrs);
    SSRNX_LOGCI("JAVA-EGL", "eglCreateContext context=%p", context);
    return java_egl_v_object(java_egl_wrap_handle(env, EGL_CONTEXT_CLASS, context));
}

EGL_FN(egl_destroy_context) {
    (void)env; (void)t; (void)u;
    return java_egl_v_bool(n == 2 && ssrnx_eglDestroyContext(java_egl_handle_of(a[0].l), java_egl_handle_of(a[1].l)) != 0);
}

EGL_FN(egl_make_current) {
    (void)env; (void)t; (void)u;
    if (n != 4) return java_egl_v_bool(false);
    return java_egl_v_bool(ssrnx_eglMakeCurrent(java_egl_handle_of(a[0].l), java_egl_handle_of(a[1].l),
                                       java_egl_handle_of(a[2].l), java_egl_handle_of(a[3].l)) != 0);
}

EGL_FN(egl_query_context) {
    (void)t; (void)u;
    if (n != 4) return java_egl_v_bool(false);
    int32_t *value = java_egl_int_array(env, a[3].l, 1, "value");
    return java_egl_v_bool(value && ssrnx_eglQueryContext(java_egl_handle_of(a[0].l), java_egl_handle_of(a[1].l), a[2].i, value) != 0);
}

EGL_FN(egl_get_current_context) {
    (void)t; (void)u; (void)a; (void)n;
    return java_egl_v_object(java_egl_wrap_handle(env, EGL_CONTEXT_CLASS, ssrnx_eglGetCurrentContext()));
}

EGL_FN(egl_get_current_display) {
    (void)t; (void)u; (void)a; (void)n;
    return java_egl_v_object(java_egl_wrap_handle(env, EGL_DISPLAY_CLASS, ssrnx_eglGetCurrentDisplay()));
}

EGL_FN(egl_get_current_surface) {
    (void)t; (void)u;
    return java_egl_v_object(n == 1 ? java_egl_wrap_handle(env, EGL_SURFACE_CLASS, ssrnx_eglGetCurrentSurface(a[0].i)) : NULL);
}

EGL_FN(egl_swap_buffers) {
    (void)env; (void)t; (void)u;
    return java_egl_v_bool(n == 2 && ssrnx_eglSwapBuffers(java_egl_handle_of(a[0].l), java_egl_handle_of(a[1].l)) != 0);
}

EGL_FN(egl_copy_buffers) {
    (void)env; (void)t; (void)u; (void)a; (void)n;
    return java_egl_v_bool(false); /* EGL_BAD_NATIVE_PIXMAP: no native pixmap type. */
}

EGL_FN(egl_wait_gl) { (void)env; (void)t; (void)u; (void)a; (void)n; return java_egl_v_bool(ssrnx_eglWaitGL() != 0); }
EGL_FN(egl_wait_native) {
    (void)env; (void)t; (void)u;
    return java_egl_v_bool(n == 2 && ssrnx_eglWaitNative(a[0].i) != 0);
}
EGL_FN(egl_get_error) { (void)env; (void)t; (void)u; (void)a; (void)n; return java_egl_v_int(ssrnx_eglGetError()); }

/* EGLContext.getEGL(): the process-wide EGL instance. */
EGL_FN(egl_context_get_egl) {
    (void)t; (void)u; (void)a; (void)n;
    pthread_mutex_lock(&java_egl_g_egl_lock);
    if (!java_egl_g_egl_instance) {
        void *obj = ssrnx_jni_new_local_object(env, EGL_IMPL_CLASS);
        if (obj) {
            java_egl_g_egl_instance = ssrnx_jni_new_global_ref_host(env, obj);
            ssrnx_jni_delete_local_ref_host(env, obj);
        }
    }
    void *out = java_egl_g_egl_instance ? ssrnx_jni_new_local_ref_host(env, java_egl_g_egl_instance) : NULL;
    pthread_mutex_unlock(&java_egl_g_egl_lock);
    return java_egl_v_object(out);
}

/* EGLContext.getGL(): a GL10 facade dispatching to the current GLES context. */
EGL_FN(egl_context_get_gl) {
    (void)t; (void)u; (void)a; (void)n;
    return java_egl_v_object(ssrnx_jni_new_local_object(env, GL10_CLASS));
}

/* Handle objects compare by native handle (EGLObjectHandle.equals). */
EGL_FN(egl_handle_equals) {
    (void)env; (void)u;
    if (n != 1 || !a[0].l) return java_egl_v_bool(false);
    char want[128] = {0}, have[128] = {0};
    void *self = ssrnx_jni_new_local_from_token(env, t);
    const bool same_class = self && ssrnx_jni_object_class_name_host(self, want, sizeof(want)) &&
                            ssrnx_jni_object_class_name_host(a[0].l, have, sizeof(have)) &&
                            strcmp(want, have) == 0;
    if (self) ssrnx_jni_delete_local_ref_host(env, self);
    return java_egl_v_bool(same_class && java_egl_handle_of_token(t) == java_egl_handle_of(a[0].l));
}

EGL_FN(egl_handle_hash) {
    (void)env; (void)u; (void)a; (void)n;
    const uintptr_t h = (uintptr_t)java_egl_handle_of_token(t);
    return java_egl_v_int((int32_t)(h ^ (h >> 32)));
}

/* ---- GL10 facade: entry points resolved from the guest GLES table ---- */

static void *java_egl_gl_proc(const char *name) { return ssrnx_eglGetProcAddress(name); }

EGL_FN(gl_get_string) {
    (void)t; (void)u;
    if (n != 1) return java_egl_v_object(NULL);
    const unsigned char *(*fn)(uint32_t) = (const unsigned char *(*)(uint32_t))java_egl_gl_proc("glGetString");
    const unsigned char *s = fn ? fn((uint32_t)a[0].i) : NULL;
    return java_egl_v_object(s ? ssrnx_jni_new_local_string_utf(env, (const char *)s) : NULL);
}

EGL_FN(gl_get_error) {
    (void)env; (void)t; (void)u; (void)a; (void)n;
    uint32_t (*fn)(void) = (uint32_t (*)(void))java_egl_gl_proc("glGetError");
    return java_egl_v_int(fn ? (int32_t)fn() : 0);
}

/* Android glgen sizes the result before passing a Java array to GL. */
static int32_t java_egl_gl_integer_count(uint32_t pname, void (*query)(uint32_t, int32_t *)) {
    switch (pname) {
        case 0x0b12: /* GL_SMOOTH_POINT_SIZE_RANGE */
        case 0x0b22: /* GL_SMOOTH_LINE_WIDTH_RANGE */
        case 0x0b70: /* GL_DEPTH_RANGE */
        case 0x0d3a: /* GL_MAX_VIEWPORT_DIMS */
        case 0x846d: /* GL_ALIASED_POINT_SIZE_RANGE */
        case 0x846e: /* GL_ALIASED_LINE_WIDTH_RANGE */
            return 2;
        case 0x0b02: /* GL_CURRENT_NORMAL */
        case 0x8129: /* GL_POINT_DISTANCE_ATTENUATION */
            return 3;
        case 0x0b00: /* GL_CURRENT_COLOR */
        case 0x0b03: /* GL_CURRENT_TEXTURE_COORDS */
        case 0x0b53: /* GL_LIGHT_MODEL_AMBIENT */
        case 0x0b66: /* GL_FOG_COLOR */
        case 0x0ba2: /* GL_VIEWPORT */
        case 0x0c10: /* GL_SCISSOR_BOX */
        case 0x0c22: /* GL_COLOR_CLEAR_VALUE */
        case 0x0c23: /* GL_COLOR_WRITEMASK */
        case 0x8005: /* GL_BLEND_COLOR */
            return 4;
        case 0x0ba6: /* GL_MODELVIEW_MATRIX */
        case 0x0ba7: /* GL_PROJECTION_MATRIX */
        case 0x0ba8: /* GL_TEXTURE_MATRIX */
        case 0x898d: /* GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES */
        case 0x898e: /* GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES */
        case 0x898f: /* GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES */
            return 16;
        case 0x86a3: /* GL_COMPRESSED_TEXTURE_FORMATS */
        case 0x8df8: /* GL_SHADER_BINARY_FORMATS */
        case 0x87ff: { /* GL_PROGRAM_BINARY_FORMATS */
            const uint32_t count_name = pname == 0x86a3 ? 0x86a2 :
                                        pname == 0x8df8 ? 0x8df9 : 0x87fe;
            int32_t count = 0;
            if (query) query(count_name, &count);
            return count;
        }
        default: return 1;
    }
}

EGL_FN(gl_get_integerv) {
    (void)t; (void)u;
    if (n != 3) return java_egl_v_void();
    if (!a[1].l) { java_egl_throw_iae(env, "params == null"); return java_egl_v_void(); }
    if (a[2].i < 0) { java_egl_throw_iae(env, "offset < 0"); return java_egl_v_void(); }
    const int32_t len = ssrnx_jni_array_length_host(a[1].l);
    void (*fn)(uint32_t, int32_t *) = (void (*)(uint32_t, int32_t *))java_egl_gl_proc("glGetIntegerv");
    const int32_t needed = java_egl_gl_integer_count((uint32_t)a[0].i, fn);
    if (needed < 0 || a[2].i > len || needed > len - a[2].i) {
        java_egl_throw_iae(env, "length - offset < needed"); return java_egl_v_void();
    }
    int32_t *params = ssrnx_jni_primitive_array_data_host(a[1].l);
    int32_t empty = 0;
    if (fn) fn((uint32_t)a[0].i, params ? params + a[2].i : &empty);
    return java_egl_v_void();
}

EGL_FN(gl_void0) {
    (void)env; (void)t; (void)a; (void)n;
    void (*fn)(void) = (void (*)(void))java_egl_gl_proc((const char *)u);
    if (fn) fn();
    return java_egl_v_void();
}

EGL_FN(gl_clear) {
    (void)env; (void)t; (void)u;
    void (*fn)(uint32_t) = (void (*)(uint32_t))java_egl_gl_proc("glClear");
    if (fn && n == 1) fn((uint32_t)a[0].i);
    return java_egl_v_void();
}

/* GLES scalar entry points share the same JNI shape as their native GL calls.
 * Keep the name in the registration table so the facade can forward the full
 * GL10/GLES2 state setup without adding one callback per enum function. */
EGL_FN(gl_enum1) {
    (void)env; (void)t;
    void (*fn)(uint32_t) = (void (*)(uint32_t))java_egl_gl_proc((const char *)u);
    if (fn && n == 1) fn((uint32_t)a[0].i);
    return java_egl_v_void();
}

EGL_FN(gl_enum2) {
    (void)env; (void)t;
    void (*fn)(uint32_t, uint32_t) = (void (*)(uint32_t, uint32_t))java_egl_gl_proc((const char *)u);
    if (fn && n == 2) fn((uint32_t)a[0].i, (uint32_t)a[1].i);
    return java_egl_v_void();
}

EGL_FN(gl_int4) {
    (void)env; (void)t;
    void (*fn)(int32_t, int32_t, int32_t, int32_t) =
        (void (*)(int32_t, int32_t, int32_t, int32_t))java_egl_gl_proc((const char *)u);
    if (fn && n == 4) fn(a[0].i, a[1].i, a[2].i, a[3].i);
    return java_egl_v_void();
}

EGL_FN(gl_float1) {
    (void)env; (void)t;
    void (*fn)(float) = (void (*)(float))java_egl_gl_proc((const char *)u);
    if (fn && n == 1) fn(a[0].f);
    return java_egl_v_void();
}

EGL_FN(gl_float2) {
    (void)env; (void)t;
    void (*fn)(float, float) = (void (*)(float, float))java_egl_gl_proc((const char *)u);
    if (fn && n == 2) fn(a[0].f, a[1].f);
    return java_egl_v_void();
}

EGL_FN(gl_clear_color) {
    (void)env; (void)t; (void)u;
    void (*fn)(float, float, float, float) = (void (*)(float, float, float, float))java_egl_gl_proc("glClearColor");
    if (fn && n == 4) fn(a[0].f, a[1].f, a[2].f, a[3].f);
    return java_egl_v_void();
}

EGL_FN(gl_viewport) {
    (void)env; (void)t; (void)u;
    void (*fn)(int32_t, int32_t, int32_t, int32_t) =
        (void (*)(int32_t, int32_t, int32_t, int32_t))java_egl_gl_proc("glViewport");
    if (fn && n == 4) fn(a[0].i, a[1].i, a[2].i, a[3].i);
    return java_egl_v_void();
}

typedef struct java_egl_EglMethod {
    const char *name, *sig;
    SsrnxJniMethodCallback cb;
} java_egl_EglMethod;

static const java_egl_EglMethod java_egl_k_egl10_methods[] = {
    {"eglGetDisplay", "(Ljava/lang/Object;)" D_DISPLAY, egl_get_display},
    {"eglInitialize", "(" D_DISPLAY "[I)Z", egl_initialize},
    {"eglTerminate", "(" D_DISPLAY ")Z", egl_terminate},
    {"eglQueryString", "(" D_DISPLAY "I)Ljava/lang/String;", egl_query_string},
    {"eglGetConfigs", "(" D_DISPLAY "[" D_CONFIG "I[I)Z", egl_get_configs},
    {"eglChooseConfig", "(" D_DISPLAY "[I[" D_CONFIG "I[I)Z", egl_choose_config},
    {"eglGetConfigAttrib", "(" D_DISPLAY D_CONFIG "I[I)Z", egl_get_config_attrib},
    {"eglCreateWindowSurface", "(" D_DISPLAY D_CONFIG "Ljava/lang/Object;[I)" D_SURFACE, egl_create_window_surface},
    {"eglCreatePbufferSurface", "(" D_DISPLAY D_CONFIG "[I)" D_SURFACE, egl_create_pbuffer_surface},
    {"eglCreatePixmapSurface", "(" D_DISPLAY D_CONFIG "Ljava/lang/Object;[I)" D_SURFACE, egl_create_pixmap_surface},
    {"eglDestroySurface", "(" D_DISPLAY D_SURFACE ")Z", egl_destroy_surface},
    {"eglQuerySurface", "(" D_DISPLAY D_SURFACE "I[I)Z", egl_query_surface},
    {"eglCreateContext", "(" D_DISPLAY D_CONFIG D_CONTEXT "[I)" D_CONTEXT, egl_create_context},
    {"eglDestroyContext", "(" D_DISPLAY D_CONTEXT ")Z", egl_destroy_context},
    {"eglMakeCurrent", "(" D_DISPLAY D_SURFACE D_SURFACE D_CONTEXT ")Z", egl_make_current},
    {"eglQueryContext", "(" D_DISPLAY D_CONTEXT "I[I)Z", egl_query_context},
    {"eglGetCurrentContext", "()" D_CONTEXT, egl_get_current_context},
    {"eglGetCurrentDisplay", "()" D_DISPLAY, egl_get_current_display},
    {"eglGetCurrentSurface", "(I)" D_SURFACE, egl_get_current_surface},
    {"eglSwapBuffers", "(" D_DISPLAY D_SURFACE ")Z", egl_swap_buffers},
    {"eglCopyBuffers", "(" D_DISPLAY D_SURFACE "Ljava/lang/Object;)Z", egl_copy_buffers},
    {"eglWaitGL", "()Z", egl_wait_gl},
    {"eglWaitNative", "(ILjava/lang/Object;)Z", egl_wait_native},
    {"eglGetError", "()I", egl_get_error},
};

SsrnxResult ssrnx_java_egl_register(void) {
    static const struct { const char *name, *super; } classes[] = {
        {EGL_IFACE, "java/lang/Object"},
        {EGL10_IFACE, "java/lang/Object"},
        {EGL11_IFACE, "java/lang/Object"},
        {EGL_DISPLAY_CLASS, "java/lang/Object"},
        {EGL_CONFIG_CLASS, "java/lang/Object"},
        {EGL_SURFACE_CLASS, "java/lang/Object"},
        {EGL_CONTEXT_CLASS, "java/lang/Object"},
        {EGL_IMPL_CLASS, "java/lang/Object"},
        {GL_IFACE, "java/lang/Object"},
        {GL10_CLASS, "java/lang/Object"},
        {"android/opengl/GLES10", "java/lang/Object"},
        {"android/opengl/GLES20", "java/lang/Object"},
    };
    SsrnxResult rc = SSRNX_OK;
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(classes) / sizeof(classes[0]); ++i)
        rc = ssrnx_jni_register_class(classes[i].name, classes[i].super);
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(java_egl_k_egl10_methods) / sizeof(java_egl_k_egl10_methods[0]); ++i) {
        rc = ssrnx_jni_register_interface_method(EGL10_IFACE, java_egl_k_egl10_methods[i].name, java_egl_k_egl10_methods[i].sig);
        if (rc == SSRNX_OK)
            rc = ssrnx_jni_register_method(EGL_IMPL_CLASS, java_egl_k_egl10_methods[i].name, java_egl_k_egl10_methods[i].sig,
                                           false, java_egl_k_egl10_methods[i].cb, NULL);
    }
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_interface(EGL10_IFACE, EGL_IFACE);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_interface(EGL11_IFACE, EGL10_IFACE);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_interface(EGL_IMPL_CLASS, EGL10_IFACE);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_interface(EGL_IMPL_CLASS, EGL11_IFACE);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_interface(GL10_CLASS, GL_IFACE);
    /* AOSP EGL10: EGL_DEFAULT_DISPLAY and the EGL_NO_* handles are null. */
    static const struct { const char *name, *sig; } constants[] = {
        {"EGL_DEFAULT_DISPLAY", "Ljava/lang/Object;"},
        {"EGL_NO_CONTEXT", D_CONTEXT},
        {"EGL_NO_DISPLAY", D_DISPLAY},
        {"EGL_NO_SURFACE", D_SURFACE},
    };
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(constants) / sizeof(constants[0]); ++i)
        rc = ssrnx_jni_register_field(EGL10_IFACE, constants[i].name, constants[i].sig, true, (SsrnxJniValue){0});
    if (rc == SSRNX_OK)
        rc = ssrnx_jni_register_method(EGL_CONTEXT_CLASS, "getEGL", "()L" EGL_IFACE ";", true, egl_context_get_egl, NULL);
    if (rc == SSRNX_OK)
        rc = ssrnx_jni_register_method(EGL_CONTEXT_CLASS, "getGL", "()L" GL_IFACE ";", false, egl_context_get_gl, NULL);
    static const char *const handle_classes[] = {EGL_DISPLAY_CLASS, EGL_CONFIG_CLASS, EGL_SURFACE_CLASS, EGL_CONTEXT_CLASS};
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(handle_classes) / sizeof(handle_classes[0]); ++i) {
        rc = ssrnx_jni_register_method(handle_classes[i], "equals", "(Ljava/lang/Object;)Z", false, egl_handle_equals, NULL);
        if (rc == SSRNX_OK)
            rc = ssrnx_jni_register_method(handle_classes[i], "hashCode", "()I", false, egl_handle_hash, NULL);
    }
    static const struct { const char *name, *sig; SsrnxJniMethodCallback cb; void *u; } gl[] = {
        {"glActiveTexture", "(I)V", gl_enum1, (void *)"glActiveTexture"},
        {"glBindTexture", "(II)V", gl_enum2, (void *)"glBindTexture"},
        {"glBlendFunc", "(II)V", gl_enum2, (void *)"glBlendFunc"},
        {"glCullFace", "(I)V", gl_enum1, (void *)"glCullFace"},
        {"glDepthFunc", "(I)V", gl_enum1, (void *)"glDepthFunc"},
        {"glDisable", "(I)V", gl_enum1, (void *)"glDisable"},
        {"glEnable", "(I)V", gl_enum1, (void *)"glEnable"},
        {"glFrontFace", "(I)V", gl_enum1, (void *)"glFrontFace"},
        {"glHint", "(II)V", gl_enum2, (void *)"glHint"},
        {"glLineWidth", "(F)V", gl_float1, (void *)"glLineWidth"},
        {"glPolygonOffset", "(FF)V", gl_float2, (void *)"glPolygonOffset"},
        {"glScissor", "(IIII)V", gl_int4, (void *)"glScissor"},
        {"glGetString", "(I)Ljava/lang/String;", gl_get_string, NULL},
        {"glGetError", "()I", gl_get_error, NULL},
        {"glGetIntegerv", "(I[II)V", gl_get_integerv, NULL},
        {"glFlush", "()V", gl_void0, (void *)"glFlush"},
        {"glFinish", "()V", gl_void0, (void *)"glFinish"},
        {"glClear", "(I)V", gl_clear, NULL},
        {"glClearColor", "(FFFF)V", gl_clear_color, NULL},
        {"glViewport", "(IIII)V", gl_viewport, NULL},
    };
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(gl) / sizeof(gl[0]); ++i)
        rc = ssrnx_jni_register_method(GL10_CLASS, gl[i].name, gl[i].sig, false, gl[i].cb, gl[i].u);
    /* These queries are identical across GL10 and GLES20 and use the current
     * native context, including its real error state and capability strings. */
    static const char *const query_classes[] = {"android/opengl/GLES10", "android/opengl/GLES20"};
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(query_classes) / sizeof(query_classes[0]); ++i) {
        rc = ssrnx_jni_register_method(query_classes[i], "glGetString", "(I)Ljava/lang/String;", true, gl_get_string, NULL);
        if (rc == SSRNX_OK)
            rc = ssrnx_jni_register_method(query_classes[i], "glGetError", "()I", true, gl_get_error, NULL);
        if (rc == SSRNX_OK)
            rc = ssrnx_jni_register_method(query_classes[i], "glGetIntegerv", "(I[II)V", true, gl_get_integerv, NULL);
    }
    return rc;
}

void ssrnx_java_egl_reset(void) {
    pthread_mutex_lock(&java_egl_g_egl_lock);
    java_egl_g_egl_instance = NULL; /* The JNI runtime reset drops every global ref. */
    pthread_mutex_unlock(&java_egl_g_egl_lock);
}

#pragma pop_macro("GL_IFACE")
#pragma pop_macro("GL10_CLASS")
#pragma pop_macro("EGL_SURFACE_CLASS")
#pragma pop_macro("EGL_PKG")
#pragma pop_macro("EGL_NONE_")
#pragma pop_macro("EGL_IMPL_CLASS")
#pragma pop_macro("EGL_IFACE")
#pragma pop_macro("EGL_HANDLE_MAGIC")
#pragma pop_macro("EGL_FN")
#pragma pop_macro("EGL_DISPLAY_CLASS")
#pragma pop_macro("EGL_CONTEXT_CLASS")
#pragma pop_macro("EGL_CONFIG_CLASS")
#pragma pop_macro("EGL11_IFACE")
#pragma pop_macro("EGL10_IFACE")
#pragma pop_macro("D_SURFACE")
#pragma pop_macro("D_DISPLAY")
#pragma pop_macro("D_CONTEXT")
#pragma pop_macro("D_CONFIG")


/* ---- bitmap ---- */



#include <stddef.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(SsrnxAndroidBitmapInfo) == 20, "AndroidBitmapInfo ABI");
_Static_assert(offsetof(SsrnxAndroidBitmapInfo, flags) == 16, "AndroidBitmapInfo flags ABI");

BitmapPayload *ssrnx_bitmap_alloc(int32_t width, int32_t height, int32_t config) {
    /* Android 11 promotes deprecated ARGB_4444 to RGBA_8888. */
    const uint32_t bpp = config == 1 ? 1 : config == 3 ? 2 :
                         (config == 4 || config == 5) ? 4 : 0;
    if (!bpp || width <= 0 || height <= 0 || (uint32_t)width > UINT32_MAX / bpp)
        return NULL;
    const uint32_t stride = (uint32_t)width * bpp;
    if ((uint32_t)height > SIZE_MAX / stride) return NULL;
    BitmapPayload *p = calloc(1, sizeof(*p));
    if (!p) return NULL;
    p->pixels = calloc((size_t)height, stride);
    if (!p->pixels) { free(p); return NULL; }
    p->magic = BITMAP_PAYLOAD_MAGIC;
    p->width = width;
    p->height = height;
    p->stride = stride;
    p->format = config == 1 ? 8 : config == 3 ? 4 : 1;
    p->flags = config == 3 ? 1 : 0; /* opaque RGB565; otherwise premultiplied */
    p->density = 160;
    p->is_mutable = true;
    atomic_init(&p->locks, 0);
    return p;
}

void ssrnx_bitmap_store_color(BitmapPayload *p, int32_t x, int32_t y, uint32_t argb) {
    const uint32_t a = argb >> 24, r = (argb >> 16) & 255,
                   g = (argb >> 8) & 255, b = argb & 255;
    uint8_t *row = p->pixels + (size_t)y * p->stride;
    if (p->format == 8) {
        row[x] = (uint8_t)a;
    } else if (p->format == 4) {
        const uint16_t rgb = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
        memcpy(row + (size_t)x * 2, &rgb, sizeof(rgb));
    } else {
        uint8_t *pixel = row + (size_t)x * 4;
        pixel[0] = (uint8_t)((r * a + 127) / 255);
        pixel[1] = (uint8_t)((g * a + 127) / 255);
        pixel[2] = (uint8_t)((b * a + 127) / 255);
        pixel[3] = (uint8_t)a;
    }
}

static BitmapPayload *bitmap_ndk_bitmap(void *env, void *bitmap, bool unlocking) {
    if (ssrnx_jni_exception_pending_host(env)) return NULL;
    if (!ssrnx_jni_is_instance_of_class_host(bitmap, "android/graphics/Bitmap")) return NULL;
    BitmapPayload *p = ssrnx_jni_get_object_host_data(bitmap);
    if (!p || p->magic != BITMAP_PAYLOAD_MAGIC || !p->pixels) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/IllegalStateException",
                                        "Bitmap has no native pixels");
        return NULL;
    }
    if (p->recycled && (!unlocking || atomic_load_explicit(&p->locks,memory_order_relaxed)==0)) {
        (void)ssrnx_jni_throw_new_by_name(env,"java/lang/IllegalStateException","Bitmap is recycled");
        return NULL;
    }
    return p;
}

int AndroidBitmap_getInfo(void *env, void *bitmap, SsrnxAndroidBitmapInfo *info) {
    if (!env || !bitmap) return -1;
    BitmapPayload *p = bitmap_ndk_bitmap(env, bitmap, false);
    if (!p) return -2;
    /* Android 11 permits a null output pointer for both getInfo and lockPixels. */
    if (info) *info = (SsrnxAndroidBitmapInfo){(uint32_t)p->width, (uint32_t)p->height,
                                            p->stride, p->format, p->flags};
    return 0;
}

int AndroidBitmap_lockPixels(void *env, void *bitmap, void **pixels) {
    if (!env || !bitmap) return -1;
    BitmapPayload *p = bitmap_ndk_bitmap(env, bitmap, false);
    if (!p) return -2;
    /* ponytail: process-owned pixels stay pinned in the non-collecting VM;
       add reference-based reclamation when Bitmap.recycle/GC is implemented. */
    atomic_fetch_add_explicit(&p->locks, 1, memory_order_relaxed);
    if (pixels) *pixels = p->pixels;
    return 0;
}

int AndroidBitmap_unlockPixels(void *env, void *bitmap) {
    if (!env || !bitmap) return -1;
    BitmapPayload *p = bitmap_ndk_bitmap(env, bitmap, true);
    if (!p) return -2;
    uint32_t locks = atomic_load_explicit(&p->locks, memory_order_relaxed);
    while (locks && !atomic_compare_exchange_weak_explicit(&p->locks, &locks, locks - 1,
                                                          memory_order_relaxed, memory_order_relaxed)) {}
    return locks ? 0 : -1;
}



/* ---- cocos_text ---- */
#pragma push_macro("COCOS_TEXT_MAX_DIM")
#pragma push_macro("COCOS_TEXT_MAX_LINES")


#include <ctype.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __SWITCH__
#include <switch.h>
#include <ft2build.h>
#include FT_FREETYPE_H


#endif

#define COCOS_TEXT_MAX_DIM 4096
#define COCOS_TEXT_MAX_LINES 512u

typedef struct cocos_text_CocosTextLine {
    size_t begin;
    size_t end;
    int width;
} cocos_text_CocosTextLine;

static float cocos_text_clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static uint8_t cocos_text_color8(float v) {
    const float c = cocos_text_clamp01(v) * 255.0f;
    return (uint8_t)(c + 0.5f);
}

static bool cocos_text_text_request_valid(const SsrnxCocosTextRequest *r) {
    if (!r || !r->text || r->font_size <= 0 || r->font_size > COCOS_TEXT_MAX_DIM) return false;
    if (r->width < 0 || r->height < 0) return false;
    if (r->width > COCOS_TEXT_MAX_DIM || r->height > COCOS_TEXT_MAX_DIM) return false;
    if (!isfinite(r->tint_r) || !isfinite(r->tint_g) || !isfinite(r->tint_b) || !isfinite(r->tint_a) ||
        !isfinite(r->line_spacing) || fabsf(r->line_spacing) > COCOS_TEXT_MAX_DIM ||
        !isfinite(r->shadow_dx) || !isfinite(r->shadow_dy) || !isfinite(r->shadow_blur) ||
        fabsf(r->shadow_dx) > COCOS_TEXT_MAX_DIM || fabsf(r->shadow_dy) > COCOS_TEXT_MAX_DIM ||
        !isfinite(r->shadow_opacity) ||
        !isfinite(r->stroke_r) || !isfinite(r->stroke_g) || !isfinite(r->stroke_b) ||
        !isfinite(r->stroke_a) || !isfinite(r->stroke_size)) return false;
    return true;
}

typedef bool (*cocos_text_CocosShrinkMeasure)(void *, const SsrnxCocosTextRequest *, int, int, int *, int *);
/* Cocos 3.17/4.0 measure with the previous Paint size, then decrement the
 * size by one. No-wrap height uses the candidate size; wrapped height uses
 * the previous size. A zero bound skips fitting; exhausting sizes restores
 * the original. Line spacing is applied only to the final layout. */
static int cocos_text_shrink_font_size(const SsrnxCocosTextRequest *r, cocos_text_CocosShrinkMeasure measure, void *ctx) {
    if (!r->shrink || !r->width || !r->height) return r->font_size;
    int paint_size = r->font_size;
    for (int candidate = r->font_size; candidate > 0; --candidate) {
        int width = 0, height = 0;
        if (!measure(ctx, r, paint_size, candidate, &width, &height)) return r->font_size;
        paint_size = candidate;
        if (width <= r->width && height <= r->height) return candidate;
    }
    return r->font_size;
}

#ifdef __SWITCH__
static size_t cocos_text_utf8_decode(const char *s, size_t remaining, uint32_t *out_cp) {
    if (!s || !remaining || !out_cp) return 0;
    const uint8_t c0 = (uint8_t)s[0];
    if (c0 < 0x80u) { *out_cp = c0; return 1; }
    if ((c0 & 0xe0u) == 0xc0u && remaining >= 2u) {
        const uint8_t c1 = (uint8_t)s[1];
        if ((c1 & 0xc0u) != 0x80u) return 0;
        const uint32_t cp = ((uint32_t)(c0 & 0x1fu) << 6) | (uint32_t)(c1 & 0x3fu);
        if (cp < 0x80u) return 0;
        *out_cp = cp; return 2;
    }
    if ((c0 & 0xf0u) == 0xe0u && remaining >= 3u) {
        const uint8_t c1 = (uint8_t)s[1], c2 = (uint8_t)s[2];
        if ((c1 & 0xc0u) != 0x80u || (c2 & 0xc0u) != 0x80u) return 0;
        const uint32_t cp = ((uint32_t)(c0 & 0x0fu) << 12) |
                            ((uint32_t)(c1 & 0x3fu) << 6) | (uint32_t)(c2 & 0x3fu);
        if (cp < 0x800u || (cp >= 0xd800u && cp <= 0xdfffu)) return 0;
        *out_cp = cp; return 3;
    }
    if ((c0 & 0xf8u) == 0xf0u && remaining >= 4u) {
        const uint8_t c1 = (uint8_t)s[1], c2 = (uint8_t)s[2], c3 = (uint8_t)s[3];
        if ((c1 & 0xc0u) != 0x80u || (c2 & 0xc0u) != 0x80u || (c3 & 0xc0u) != 0x80u) return 0;
        const uint32_t cp = ((uint32_t)(c0 & 0x07u) << 18) |
                            ((uint32_t)(c1 & 0x3fu) << 12) |
                            ((uint32_t)(c2 & 0x3fu) << 6) | (uint32_t)(c3 & 0x3fu);
        if (cp < 0x10000u || cp > 0x10ffffu) return 0;
        *out_cp = cp; return 4;
    }
    /* Match Java's forgiving String conversion closely enough for malformed
     * guest bytes: consume one byte and emit U+FFFD instead of aborting. */
    *out_cp = 0xfffdu;
    return 1;
}

#endif

static void cocos_text_bitmap_reset(SsrnxCocosTextBitmap *b) {
    if (b) memset(b, 0, sizeof(*b));
}

void ssrnx_cocos_text_bitmap_free(SsrnxCocosTextBitmap *bitmap) {
    if (!bitmap) return;
    free(bitmap->pixels);
    cocos_text_bitmap_reset(bitmap);
}

static void cocos_text_blend_pixel_rgba(uint8_t *dst, uint8_t r, uint8_t g, uint8_t b,
                             uint8_t base_alpha, uint8_t coverage) {
    if (!dst || coverage == 0 || base_alpha == 0) return;
    const unsigned sa = ((unsigned)base_alpha * coverage + 127u) / 255u;
    if (!sa) return;
    const unsigned inv = 255u - sa;
    /* Android text bitmaps are premultiplied. Keep the compatibility bitmap
     * premultiplied so Cocos' existing Android native conversion sees the same
     * kind of source data it gets from Bitmap.copyPixelsToBuffer(). */
    const unsigned sr = ((unsigned)r * sa + 127u) / 255u;
    const unsigned sg = ((unsigned)g * sa + 127u) / 255u;
    const unsigned sb = ((unsigned)b * sa + 127u) / 255u;
    dst[0] = (uint8_t)(sr + ((unsigned)dst[0] * inv + 127u) / 255u);
    dst[1] = (uint8_t)(sg + ((unsigned)dst[1] * inv + 127u) / 255u);
    dst[2] = (uint8_t)(sb + ((unsigned)dst[2] * inv + 127u) / 255u);
    dst[3] = (uint8_t)(sa + ((unsigned)dst[3] * inv + 127u) / 255u);
}

#ifdef __SWITCH__

static FT_Library cocos_text_g_ft_library;
static bool cocos_text_g_ft_ready;
static bool cocos_text_g_pl_ready;
static PlFontData cocos_text_g_shared_font;
static bool cocos_text_g_shared_font_ready;

static SsrnxResult cocos_text_ensure_freetype(void) {
    if (cocos_text_g_ft_ready) return SSRNX_OK;
    if (FT_Init_FreeType(&cocos_text_g_ft_library) != 0) return SSRNX_ERR_APK_UNSUPPORTED;
    cocos_text_g_ft_ready = true;
    return SSRNX_OK;
}

static SsrnxResult cocos_text_ensure_shared_font(void) {
    SsrnxResult rc = cocos_text_ensure_freetype();
    if (rc != SSRNX_OK) return rc;
    if (!cocos_text_g_pl_ready) {
        const Result prc = plInitialize(PlServiceType_User);
        if (R_FAILED(prc)) {
            SSRNX_LOGCW("COCOS-TEXT", "plInitialize failed rc=0x%x", prc);
            return SSRNX_ERR_APK_UNSUPPORTED;
        }
        cocos_text_g_pl_ready = true;
    }
    if (!cocos_text_g_shared_font_ready) {
        const Result prc = plGetSharedFontByType(&cocos_text_g_shared_font, PlSharedFontType_Standard);
        if (R_FAILED(prc) || !cocos_text_g_shared_font.address || !cocos_text_g_shared_font.size) {
            SSRNX_LOGCW("COCOS-TEXT", "plGetSharedFontByType failed rc=0x%x", prc);
            return SSRNX_ERR_APK_UNSUPPORTED;
        }
        cocos_text_g_shared_font_ready = true;
    }
    return SSRNX_OK;
}

static bool cocos_text_suffix_case_equal(const char *s, const char *suffix) {
    if (!s || !suffix) return false;
    const size_t sn = strlen(s), nn = strlen(suffix);
    if (sn < nn) return false;
    s += sn - nn;
    for (size_t i = 0; i < nn; ++i)
        if (tolower((unsigned char)s[i]) != tolower((unsigned char)suffix[i])) return false;
    return true;
}

typedef struct cocos_text_FontFaceOwner {
    FT_Face face;
    SsrnxAsset *asset;
} cocos_text_FontFaceOwner;

static void cocos_text_font_face_close(cocos_text_FontFaceOwner *owner) {
    if (!owner) return;
    if (owner->face) FT_Done_Face(owner->face);
    if (owner->asset) ssrnx_asset_close(owner->asset);
    memset(owner, 0, sizeof(*owner));
}

static bool cocos_text_try_asset_face_name(const char *asset_name, cocos_text_FontFaceOwner *owner) {
    if (!asset_name || !asset_name[0] || !owner || !ssrnx_assets_manager()) return false;
    SsrnxAsset *asset = ssrnx_asset_open(ssrnx_assets_manager(), asset_name, SSRNX_AASSET_MODE_BUFFER);
    if (!asset) return false;
    const int64_t len64 = ssrnx_asset_length64(asset);
    const void *data = ssrnx_asset_buffer(asset);
    if (!data || len64 <= 0 || (uint64_t)len64 > (uint64_t)0x7fffffff) {
        ssrnx_asset_close(asset);
        return false;
    }
    FT_Face face = NULL;
    if (FT_New_Memory_Face(cocos_text_g_ft_library, (const FT_Byte *)data, (FT_Long)len64, 0, &face) != 0) {
        ssrnx_asset_close(asset);
        return false;
    }
    owner->face = face;
    owner->asset = asset;
    return true;
}

static SsrnxResult cocos_text_open_font_face(const char *font_name, int font_size,
                                  cocos_text_FontFaceOwner *owner, char *source, size_t source_cap) {
    if (!owner || !source || source_cap == 0 || font_size <= 0) return SSRNX_ERR_ARGUMENT;
    memset(owner, 0, sizeof(*owner));
    source[0] = '\0';
    SsrnxResult rc = cocos_text_ensure_freetype();
    if (rc != SSRNX_OK) return rc;

    if (font_name && font_name[0] &&
        (cocos_text_suffix_case_equal(font_name, ".ttf") || cocos_text_suffix_case_equal(font_name, ".otf"))) {
        const char *asset_name = font_name;
        if (strncmp(asset_name, "assets/", 7) == 0) asset_name += 7;
        if (cocos_text_try_asset_face_name(asset_name, owner)) {
            snprintf(source, source_cap, "asset");
        } else if (!strchr(asset_name, '/')) {
            char candidate[512];
            const int candidate_len = snprintf(candidate, sizeof(candidate), "fonts/%s", asset_name);
            if (candidate_len > 0 && (size_t)candidate_len < sizeof(candidate) &&
                cocos_text_try_asset_face_name(candidate, owner)) {
                snprintf(source, source_cap, "asset-fonts");
            }
        }
    }

    if (!owner->face) {
        rc = cocos_text_ensure_shared_font();
        if (rc != SSRNX_OK) return rc;
        if (FT_New_Memory_Face(cocos_text_g_ft_library, (const FT_Byte *)cocos_text_g_shared_font.address,
                               (FT_Long)cocos_text_g_shared_font.size, 0, &owner->face) != 0)
            return SSRNX_ERR_APK_UNSUPPORTED;
        snprintf(source, source_cap, "switch-shared");
    }
    if (FT_Set_Pixel_Sizes(owner->face, 0, (FT_UInt)font_size) != 0) {
        cocos_text_font_face_close(owner);
        return SSRNX_ERR_APK_UNSUPPORTED;
    }
    return SSRNX_OK;
}

/* FreeType library-level calls (face creation/destruction) are not thread-safe;
 * Paint queries can run on any Java thread next to Cocos rasterization. */
static pthread_mutex_t cocos_text_g_ft_lock = PTHREAD_MUTEX_INITIALIZER;

static SsrnxResult cocos_text_measure_switch(const char *text, float font_size, float *out_width) {
    cocos_text_FontFaceOwner owner = {0};
    char source[32];
    const int pixels = (int)ceilf(font_size);
    SsrnxResult rc = cocos_text_open_font_face(NULL, pixels > 0 ? pixels : 1, &owner, source, sizeof(source));
    if (rc != SSRNX_OK) return rc;
    (void)FT_Set_Char_Size(owner.face, 0, (FT_F26Dot6)(font_size * 64.0f), 72, 72);
    double width = 0.0;
    const size_t len = strlen(text);
    for (size_t p = 0; p < len;) {
        uint32_t cp = 0;
        const size_t n = cocos_text_utf8_decode(text + p, len - p, &cp);
        if (!n) break;
        p += n;
        const FT_UInt glyph = FT_Get_Char_Index(owner.face, (FT_ULong)cp);
        if (FT_Load_Glyph(owner.face, glyph, FT_LOAD_NO_HINTING) == 0)
            width += (double)owner.face->glyph->linearHoriAdvance / 65536.0;
    }
    cocos_text_font_face_close(&owner);
    *out_width = (float)width;
    return SSRNX_OK;
}

static SsrnxResult cocos_text_has_glyphs_switch(const char *text, bool *out_has) {
    cocos_text_FontFaceOwner owner = {0};
    char source[32];
    SsrnxResult rc = cocos_text_open_font_face(NULL, 16, &owner, source, sizeof(source));
    if (rc != SSRNX_OK) return rc;
    bool has = true;
    const size_t len = strlen(text);
    for (size_t p = 0; p < len && has;) {
        uint32_t cp = 0;
        const size_t n = cocos_text_utf8_decode(text + p, len - p, &cp);
        if (!n) { has = false; break; }
        p += n;
        /* Variation selectors and joiners combine with a base glyph. */
        if (cp == 0x200Du || (cp >= 0xFE00u && cp <= 0xFE0Fu) || (cp >= 0xE0100u && cp <= 0xE01EFu)) continue;
        has = FT_Get_Char_Index(owner.face, (FT_ULong)cp) != 0;
    }
    cocos_text_font_face_close(&owner);
    *out_has = has;
    return SSRNX_OK;
}

static int cocos_text_glyph_advance(FT_Face face, uint32_t cp, FT_UInt *io_previous) {
    const FT_UInt glyph = FT_Get_Char_Index(face, (FT_ULong)cp);
    int advance = 0;
    if (FT_HAS_KERNING(face) && io_previous && *io_previous && glyph) {
        FT_Vector kern = {0, 0};
        if (FT_Get_Kerning(face, *io_previous, glyph, FT_KERNING_DEFAULT, &kern) == 0)
            advance += (int)(kern.x >> 6);
    }
    if (FT_Load_Glyph(face, glyph, FT_LOAD_DEFAULT) == 0)
        advance += (int)(face->glyph->advance.x >> 6);
    if (io_previous) *io_previous = glyph;
    return advance > 0 ? advance : 1;
}

static int cocos_text_measure_span(FT_Face face, const char *text, size_t begin, size_t end) {
    int width = 0;
    FT_UInt previous = 0;
    for (size_t p = begin; p < end;) {
        uint32_t cp = 0;
        const size_t n = cocos_text_utf8_decode(text + p, end - p, &cp);
        if (!n) break;
        p += n;
        if (cp == '\r') continue;
        width += cocos_text_glyph_advance(face, cp, &previous);
    }
    return width;
}

static bool cocos_text_push_line(cocos_text_CocosTextLine *lines, size_t *count, size_t begin, size_t end, int width,
                      size_t max_lines) {
    if (!lines || !count || *count >= COCOS_TEXT_MAX_LINES || *count >= max_lines) return false;
    lines[*count] = (cocos_text_CocosTextLine){.begin = begin, .end = end, .width = width};
    (*count)++;
    return true;
}

static size_t cocos_text_build_lines(FT_Face face, const char *text, int max_width, int max_height,
                          int line_height, cocos_text_CocosTextLine *lines, int *out_max_width) {
    const size_t text_len = strlen(text);
    size_t count = 0;
    int widest = 0;
    size_t max_lines = COCOS_TEXT_MAX_LINES;
    if (max_height > 0 && line_height > 0) {
        const int bounded = max_height / line_height;
        if (bounded > 0 && (size_t)bounded < max_lines) max_lines = (size_t)bounded;
    }

    size_t segment_begin = 0;
    while (segment_begin <= text_len && count < max_lines) {
        size_t segment_end = segment_begin;
        while (segment_end < text_len && text[segment_end] != '\n') segment_end++;

        if (max_width <= 0) {
            const int w = cocos_text_measure_span(face, text, segment_begin, segment_end);
            if (!cocos_text_push_line(lines, &count, segment_begin, segment_end, w, max_lines)) break;
            if (w > widest) widest = w;
        } else if (segment_begin == segment_end) {
            if (!cocos_text_push_line(lines, &count, segment_begin, segment_end, 0, max_lines)) break;
        } else {
            size_t line_begin = segment_begin;
            while (line_begin < segment_end && count < max_lines) {
                size_t p = line_begin, best_end = line_begin, last_space_end = 0;
                int width = 0, width_at_space = 0;
                FT_UInt previous = 0;
                while (p < segment_end) {
                    uint32_t cp = 0;
                    const size_t n = cocos_text_utf8_decode(text + p, segment_end - p, &cp);
                    if (!n) break;
                    const int adv = cocos_text_glyph_advance(face, cp, &previous);
                    if (width + adv > max_width && best_end > line_begin) break;
                    width += adv;
                    p += n;
                    best_end = p;
                    if (cp == ' ' || cp == '\t') {
                        last_space_end = p;
                        width_at_space = width;
                    }
                    if (width >= max_width) break;
                }
                size_t line_end = best_end;
                int line_width = width;
                if (best_end < segment_end && last_space_end > line_begin) {
                    line_end = last_space_end;
                    line_width = width_at_space;
                }
                if (line_end <= line_begin) {
                    uint32_t cp = 0;
                    const size_t n = cocos_text_utf8_decode(text + line_begin, segment_end - line_begin, &cp);
                    line_end = line_begin + (n ? n : 1u);
                    line_width = cocos_text_measure_span(face, text, line_begin, line_end);
                }
                size_t trim_end = line_end;
                while (trim_end > line_begin && (text[trim_end - 1u] == ' ' || text[trim_end - 1u] == '\t'))
                    trim_end--;
                line_width = cocos_text_measure_span(face, text, line_begin, trim_end);
                if (!cocos_text_push_line(lines, &count, line_begin, trim_end, line_width, max_lines)) break;
                if (line_width > widest) widest = line_width;
                line_begin = line_end;
                while (line_begin < segment_end && (text[line_begin] == ' ' || text[line_begin] == '\t')) line_begin++;
            }
        }

        if (segment_end == text_len) break;
        segment_begin = segment_end + 1u;
    }
    if (count == 0) (void)cocos_text_push_line(lines, &count, 0, 0, 0, max_lines);
    if (max_width > 0) widest = max_width;
    if (out_max_width) *out_max_width = widest;
    return count;
}

static void cocos_text_draw_ft_glyph_bitmap(SsrnxCocosTextBitmap *bitmap, const FT_Bitmap *glyph,
                                 int dst_x, int dst_y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!bitmap || !bitmap->pixels || !glyph || glyph->pixel_mode != FT_PIXEL_MODE_GRAY) return;
    for (unsigned gy = 0; gy < glyph->rows; ++gy) {
        const int y = dst_y + (int)gy;
        if (y < 0 || y >= bitmap->height) continue;
        const uint8_t *row = glyph->pitch >= 0
            ? glyph->buffer + (ptrdiff_t)gy * glyph->pitch
            : glyph->buffer + (ptrdiff_t)(glyph->rows - 1u - gy) * (-(ptrdiff_t)glyph->pitch);
        for (unsigned gx = 0; gx < glyph->width; ++gx) {
            const int x = dst_x + (int)gx;
            if (x < 0 || x >= bitmap->width) continue;
            const uint8_t coverage = row[gx];
            cocos_text_blend_pixel_rgba(bitmap->pixels + ((size_t)y * (size_t)bitmap->width + (size_t)x) * 4u,
                             r, g, b, a, coverage);
        }
    }
}

static void cocos_text_draw_ft_span(FT_Face face, SsrnxCocosTextBitmap *bitmap, const char *text,
                         size_t begin, size_t end, int start_x, int baseline_y,
                         uint8_t r, uint8_t g, uint8_t b, uint8_t a, int offset_x, int offset_y) {
    int pen_x = start_x;
    FT_UInt previous = 0;
    for (size_t p = begin; p < end;) {
        uint32_t cp = 0;
        const size_t n = cocos_text_utf8_decode(text + p, end - p, &cp);
        if (!n) break;
        p += n;
        if (cp == '\r') continue;
        const FT_UInt glyph_index = FT_Get_Char_Index(face, (FT_ULong)cp);
        if (FT_HAS_KERNING(face) && previous && glyph_index) {
            FT_Vector kern = {0, 0};
            if (FT_Get_Kerning(face, previous, glyph_index, FT_KERNING_DEFAULT, &kern) == 0)
                pen_x += (int)(kern.x >> 6);
        }
        if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT) != 0) { previous = glyph_index; continue; }
        const int advance = (int)(face->glyph->advance.x >> 6);
        if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) == 0) {
            const int gx = pen_x + face->glyph->bitmap_left + offset_x;
            const int gy = baseline_y - face->glyph->bitmap_top + offset_y;
            cocos_text_draw_ft_glyph_bitmap(bitmap, &face->glyph->bitmap, gx, gy, r, g, b, a);
        }
        pen_x += advance > 0 ? advance : 1;
        previous = glyph_index;
    }
}

static int cocos_text_line_start_x(const cocos_text_CocosTextLine *line, int max_width, int horizontal) {
    if (!line) return 0;
    if (horizontal == 3) return max_width / 2 - line->width / 2;
    if (horizontal == 2) return max_width - line->width;
    return 0;
}

static bool cocos_text_measure_shrink_switch(void *ctx, const SsrnxCocosTextRequest *r, int paint_size,
                                  int candidate, int *width, int *height) {
    FT_Face face = ctx;
    if (FT_Set_Pixel_Sizes(face, 0, (FT_UInt)paint_size)) return false;
    int line_height = (int)((face->size->metrics.height + 63) >> 6);
    if (line_height <= 0) line_height = paint_size;
    cocos_text_CocosTextLine lines[COCOS_TEXT_MAX_LINES];
    const size_t count = cocos_text_build_lines(face, r->text, r->shrink_wrap ? r->width : 0,
                                      0, line_height, lines, width);
    if (!r->shrink_wrap) {
        if (FT_Set_Pixel_Sizes(face, 0, (FT_UInt)candidate)) return false;
        line_height = (int)((face->size->metrics.ascender - face->size->metrics.descender + 63) >> 6);
        if (line_height <= 0) line_height = candidate;
    }
    *height = count >= COCOS_TEXT_MAX_LINES ? COCOS_TEXT_MAX_DIM + 1 : (int)count * line_height;
    return true;
}

static SsrnxResult cocos_text_rasterize_switch(const SsrnxCocosTextRequest *r, SsrnxCocosTextBitmap *out) {
    cocos_text_FontFaceOwner owner = {0};
    char font_source[32] = {0};
    SsrnxResult rc = cocos_text_open_font_face(r->font_name, r->font_size, &owner, font_source, sizeof(font_source));
    if (rc != SSRNX_OK) return rc;
    FT_Face face = owner.face;
    const int fitted_size = cocos_text_shrink_font_size(r, cocos_text_measure_shrink_switch, face);
    if (FT_Set_Pixel_Sizes(face, 0, (FT_UInt)fitted_size)) {
        cocos_text_font_face_close(&owner); return SSRNX_ERR_APK_UNSUPPORTED;
    }

    int line_height = (int)((face->size->metrics.height + 63) >> 6);
    int ascender = (int)((face->size->metrics.ascender + 63) >> 6);
    if (line_height <= 0) line_height = fitted_size;
    if (ascender <= 0) ascender = fitted_size;
    const int line_step = (int)fmaxf(1.0f, line_height + r->line_spacing);

    cocos_text_CocosTextLine lines[COCOS_TEXT_MAX_LINES];
    int max_content_width = 0;
    const size_t line_count = cocos_text_build_lines(face, r->text, r->no_wrap ? 0 : r->width, r->height, line_step,
                                          lines, &max_content_width);
    if (r->no_wrap && r->width > 0) max_content_width = r->width;
    const int total_height = line_height + (int)(line_count - 1u) * line_step;
    const int bitmap_text_height = r->height == 0 ? total_height : r->height;
    const int pad_x = r->shadow ? (int)fabsf(r->shadow_dx) : 0;
    const int pad_y = r->shadow ? (int)fabsf(r->shadow_dy) : 0;
    const int width = max_content_width + pad_x;
    const int height = bitmap_text_height + pad_y;
    if (width <= 0 || height <= 0 || width > COCOS_TEXT_MAX_DIM || height > COCOS_TEXT_MAX_DIM) {
        cocos_text_font_face_close(&owner);
        return SSRNX_ERR_APK_UNSUPPORTED;
    }
    const size_t pixel_count = (size_t)width * (size_t)height;
    if (pixel_count > SIZE_MAX / 4u) { cocos_text_font_face_close(&owner); return SSRNX_ERR_NO_MEMORY; }
    uint8_t *pixels = (uint8_t *)calloc(pixel_count, 4u);
    if (!pixels) { cocos_text_font_face_close(&owner); return SSRNX_ERR_NO_MEMORY; }

    out->width = width;
    out->height = height;
    out->pixels = pixels;
    out->byte_count = pixel_count * 4u;
    out->line_count = line_count;
    out->font_size = fitted_size;
    snprintf(out->font_source, sizeof(out->font_source), "%s", font_source);

    const int horizontal = r->alignment & 0x0f;
    const int vertical = (r->alignment >> 4) & 0x0f;
    int baseline = ascender;
    if (r->height > total_height) {
        if (vertical == 3) baseline += (r->height - total_height) / 2;
        else if (vertical == 2) baseline += r->height - total_height;
    }
    const int render_dx = r->shadow && r->shadow_dx < 0.0f ? pad_x : 0;
    const int render_dy = r->shadow && r->shadow_dy < 0.0f ? pad_y : 0;

    const uint8_t tr = cocos_text_color8(r->tint_r), tg = cocos_text_color8(r->tint_g), tb = cocos_text_color8(r->tint_b);
    const uint8_t sr = cocos_text_color8(r->stroke_r), sg = cocos_text_color8(r->stroke_g), sb = cocos_text_color8(r->stroke_b);
    const uint8_t ta = cocos_text_color8(r->tint_a), sa = cocos_text_color8(r->stroke_a);
    const uint8_t shadow_alpha = cocos_text_color8(r->shadow_opacity);
    const int shadow_x = (int)lroundf(r->shadow_dx);
    const int shadow_y = (int)lroundf(r->shadow_dy);
    int stroke_radius = r->stroke ? (int)ceilf(fabsf(r->stroke_size) * 0.5f) : 0;
    if (stroke_radius > 8) stroke_radius = 8;

    for (size_t i = 0; i < line_count; ++i) {
        const int x = cocos_text_line_start_x(&lines[i], max_content_width, horizontal) + render_dx;
        const int y = baseline + (int)i * line_step + render_dy;
        if (r->shadow) {
            /* Android Paint applies blur; a direct shadow offset keeps the core
             * compatibility behavior without turning text creation into a hot
             * convolution path. */
            cocos_text_draw_ft_span(face, out, r->text, lines[i].begin, lines[i].end,
                         x, y, 125u, 125u, 125u, shadow_alpha, shadow_x, shadow_y);
        }
        if (r->stroke && stroke_radius > 0) {
            for (int oy = -stroke_radius; oy <= stroke_radius; ++oy) {
                for (int ox = -stroke_radius; ox <= stroke_radius; ++ox) {
                    if (ox == 0 && oy == 0) continue;
                    if (ox * ox + oy * oy > stroke_radius * stroke_radius + stroke_radius) continue;
                    cocos_text_draw_ft_span(face, out, r->text, lines[i].begin, lines[i].end,
                                 x, y, sr, sg, sb, sa, ox, oy);
                }
            }
        }
        cocos_text_draw_ft_span(face, out, r->text, lines[i].begin, lines[i].end,
                     x, y, tr, tg, tb, ta, 0, 0);
    }

    SSRNX_LOGCI("COCOS-TEXT",
                "bitmap text bytes=%zu font=%s size=%d fitted=%d shrink=%u canvas=%dx%d lines=%zu source=%s shadow=%u blur=%.2f stroke=%u",
                strlen(r->text), r->font_name && r->font_name[0] ? r->font_name : "<default>",
                r->font_size, fitted_size, r->shrink ? 1u : 0u, out->width, out->height, out->line_count, out->font_source,
                r->shadow ? 1u : 0u, (double)r->shadow_blur, r->stroke ? 1u : 0u);
    cocos_text_font_face_close(&owner);
    return SSRNX_OK;
}

#else

/* Host-only deterministic fallback. It is deliberately not a visual substitute
 * for FreeType; it validates the generic Cocos bridge, bounds, layout and pixel
 * transport in retained regressions. */
static bool cocos_text_measure_shrink_host(void *ctx, const SsrnxCocosTextRequest *r, int paint_size,
                                int candidate, int *width, int *height) {
    (void)ctx;
    const int cell_h = paint_size > 2 ? paint_size : 3;
    const int cell_w = cell_h > 3 ? cell_h * 3 / 5 : 2;
    size_t count = 1, columns = 0, widest = 0;
    const size_t limit = r->shrink_wrap ? (size_t)(r->width / cell_w > 0 ? r->width / cell_w : 1) : SIZE_MAX;
    for (const unsigned char *p = (const unsigned char *)r->text; *p; ++p) {
        if (*p == '\n') { if (columns > widest) widest = columns; columns = 0; count++; }
        else if ((*p & 0xc0u) != 0x80u && *p != '\r') {
            if (columns == limit) { if (columns > widest) widest = columns; columns = 0; count++; }
            columns++;
        }
    }
    if (columns > widest) widest = columns;
    *width = r->shrink_wrap ? r->width : (int)widest * cell_w;
    *height = (int)count * (r->shrink_wrap ? cell_h : (candidate > 2 ? candidate : 3));
    return true;
}
static SsrnxResult cocos_text_rasterize_host(const SsrnxCocosTextRequest *r, SsrnxCocosTextBitmap *out) {
    const int fitted_size = cocos_text_shrink_font_size(r, cocos_text_measure_shrink_host, NULL);
    const int cell_h = fitted_size > 2 ? fitted_size : 3;
    const int cell_w = cell_h > 3 ? (cell_h * 3) / 5 : 2;
    const int line_step = (int)fmaxf(1.0f, cell_h + r->line_spacing);
    size_t lines = 1, columns = 0, max_columns = 0;
    for (const unsigned char *p = (const unsigned char *)r->text; *p; ++p) {
        if (*p == '\n') { if (columns > max_columns) max_columns = columns; columns = 0; lines++; }
        else if ((*p & 0xc0u) != 0x80u) columns++;
    }
    if (columns > max_columns) max_columns = columns;
    int width = r->width > 0 ? r->width : (int)(max_columns ? max_columns : 1u) * cell_w;
    int height = r->height > 0 ? r->height : cell_h + (int)(lines - 1u) * line_step;
    if (r->shadow) { width += (int)fabsf(r->shadow_dx); height += (int)fabsf(r->shadow_dy); }
    if (width <= 0 || height <= 0 || width > COCOS_TEXT_MAX_DIM || height > COCOS_TEXT_MAX_DIM)
        return SSRNX_ERR_APK_UNSUPPORTED;
    const size_t bytes = (size_t)width * (size_t)height * 4u;
    uint8_t *pixels = (uint8_t *)calloc(1, bytes);
    if (!pixels) return SSRNX_ERR_NO_MEMORY;
    const uint8_t tr = cocos_text_color8(r->tint_r), tg = cocos_text_color8(r->tint_g), tb = cocos_text_color8(r->tint_b);
    const uint8_t ta = cocos_text_color8(r->tint_a);
    int x = 0, y = 0;
    for (const unsigned char *p = (const unsigned char *)r->text; *p && y < height; ++p) {
        if (*p == '\n') { x = 0; y += line_step; continue; }
        if ((*p & 0xc0u) == 0x80u) continue;
        if (*p != ' ' && *p != '\t') {
            for (int yy = 1; yy < cell_h - 1 && y + yy < height; ++yy)
                for (int xx = 1; xx < cell_w - 1 && x + xx < width; ++xx)
                    cocos_text_blend_pixel_rgba(pixels + ((size_t)(y + yy) * (size_t)width + (size_t)(x + xx)) * 4u,
                                     tr, tg, tb, ta, 220u);
        }
        x += cell_w;
        if (!r->no_wrap && r->width > 0 && x + cell_w > r->width) { x = 0; y += line_step; }
    }
    out->width = width; out->height = height; out->pixels = pixels; out->byte_count = bytes;
    out->line_count = lines;
    out->font_size = fitted_size;
    snprintf(out->font_source, sizeof(out->font_source), "host-fixture");
    return SSRNX_OK;
}

#endif

SsrnxResult ssrnx_cocos_text_rasterize(const SsrnxCocosTextRequest *request,
                                       SsrnxCocosTextBitmap *out_bitmap) {
    if (!out_bitmap) return SSRNX_ERR_ARGUMENT;
    cocos_text_bitmap_reset(out_bitmap);
    if (!cocos_text_text_request_valid(request)) return SSRNX_ERR_ARGUMENT;
#ifdef __SWITCH__
    pthread_mutex_lock(&cocos_text_g_ft_lock);
    const SsrnxResult rc = cocos_text_rasterize_switch(request, out_bitmap);
    pthread_mutex_unlock(&cocos_text_g_ft_lock);
    return rc;
#else
    return cocos_text_rasterize_host(request, out_bitmap);
#endif
}

SsrnxResult ssrnx_cocos_text_measure(const char *text, float font_size, float *out_width) {
    if (!text || !out_width || !(font_size > 0.0f) || font_size > (float)COCOS_TEXT_MAX_DIM) return SSRNX_ERR_ARGUMENT;
    *out_width = 0.0f;
    if (!text[0]) return SSRNX_OK;
#ifdef __SWITCH__
    pthread_mutex_lock(&cocos_text_g_ft_lock);
    const SsrnxResult rc = cocos_text_measure_switch(text, font_size, out_width);
    pthread_mutex_unlock(&cocos_text_g_ft_lock);
    return rc;
#else
    /* Host fixture metrics match rasterize_host: 3/5 em per code point. */
    size_t points = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) if ((*p & 0xc0u) != 0x80u) ++points;
    *out_width = (float)points * font_size * 0.6f;
    return SSRNX_OK;
#endif
}

SsrnxResult ssrnx_cocos_text_has_glyphs(const char *text, bool *out_has) {
    if (!text || !out_has) return SSRNX_ERR_ARGUMENT;
    *out_has = false;
    if (!text[0]) return SSRNX_OK;
#ifdef __SWITCH__
    pthread_mutex_lock(&cocos_text_g_ft_lock);
    const SsrnxResult rc = cocos_text_has_glyphs_switch(text, out_has);
    pthread_mutex_unlock(&cocos_text_g_ft_lock);
    return rc;
#else
    *out_has = true;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) if (*p < 0x20u) *out_has = false;
    return SSRNX_OK;
#endif
}

#pragma pop_macro("COCOS_TEXT_MAX_LINES")
#pragma pop_macro("COCOS_TEXT_MAX_DIM")
