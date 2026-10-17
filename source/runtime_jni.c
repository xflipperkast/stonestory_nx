/* runtime jni. */
#define _GNU_SOURCE 1
#include "runtime.h"


/* ---- jni ---- */
#pragma push_macro("SSRNX_ACC_ABSTRACT")
#pragma push_macro("SSRNX_ACC_INTERFACE")
#pragma push_macro("SSRNX_ACC_NATIVE")
#pragma push_macro("SSRNX_ACC_PUBLIC")
#pragma push_macro("SSRNX_ACC_STATIC")
#pragma push_macro("SSRNX_DIRECT_BUFFER_CLASS")
#pragma push_macro("SSRNX_INVOCATION_HANDLER_INVOKE_SIG")
#pragma push_macro("SSRNX_PROXY_CLASS")
#pragma push_macro("SSRNX_PROXY_HANDLER_FIELD")
#pragma push_macro("SSRNX_PROXY_HANDLER_SIG")
#pragma push_macro("DEFINE_CALL_SET")
#pragma push_macro("DEFINE_CALL_TYPE")
#pragma push_macro("DEFINE_CALL_VOID")
#pragma push_macro("DEFINE_PRIMITIVE_ARRAY_JNI")
#pragma push_macro("DEFINE_SMALL_FIELD")
#pragma push_macro("JNI_FALSE")
#pragma push_macro("JNI_SLOT")
#pragma push_macro("JNI_TABLE_SLOTS")
#pragma push_macro("JNI_TRUE")
#pragma push_macro("JVM_TABLE_SLOTS")
#pragma push_macro("NONVIRTUAL_CLASS")
#pragma push_macro("NO_EXTRA")
#pragma push_macro("UNSAFE_FIELD_CAP")
#pragma push_macro("UNSAFE_FIELD_OFFSET_BASE")



#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef __SWITCH__
#include <switch.h>
#endif

/* Full SSRNX builds provide the Bionic TLS bridge from compat_threads.c.
 * Keep the reference weak so the standalone host JNI tests that intentionally
 * link only jni.c remain self-contained. A successful VM attach is the generic
 * boundary where an ART/Java-created host thread becomes eligible to execute
 * guest Android AArch64, so publish TPIDR_EL0 before returning JNIEnv*. */
extern void ssrnx_bionic_tls_activate_current(void) __attribute__((weak));
extern bool ssrnx_bionic_tls_validate_current(void) __attribute__((weak));
extern bool ssrnx_diag_full_trace_enabled(void) __attribute__((weak));
extern void ssrnx_bionic_set_process_terminating(bool terminating) __attribute__((weak));
extern bool ssrnx_bionic_current_stack_bounds(uintptr_t *low, uintptr_t *high) __attribute__((weak));
extern bool ssrnx_compat_describe_guest_address(uintptr_t address, char *out, size_t cap) __attribute__((weak));

static void jni_jni_activate_android_tls(void) {
    if (ssrnx_bionic_tls_activate_current)
        ssrnx_bionic_tls_activate_current();
}

#define JNI_TABLE_SLOTS 233u
#define JVM_TABLE_SLOTS 8u
#define JNI_TRUE  ((uint8_t)1)
#define JNI_FALSE ((uint8_t)0)
#define SSRNX_ACC_NATIVE 0x0100u

#define JNI_SLOT(fn) ((void *)(uintptr_t)(void (*)(void))(fn))

static bool jni_jni_export_append_literal(char *out, size_t cap, size_t *used,
                                      const char *text) {
    if (!out || !used || !text) return false;
    const size_t n = strlen(text);
    if (*used + n + 1u > cap) return false;
    memcpy(out + *used, text, n);
    *used += n;
    out[*used] = '\0';
    return true;
}

static bool jni_jni_export_append_hex16(char *out, size_t cap, size_t *used, uint16_t value) {
    char encoded[7];
    snprintf(encoded, sizeof(encoded), "_0%04x", (unsigned)value);
    return jni_jni_export_append_literal(out, cap, used, encoded);
}

static bool jni_jni_export_decode_utf8(const unsigned char **cursor, uint32_t *out_cp) {
    const unsigned char *p = *cursor;
    if (!p || !*p || !out_cp) return false;
    uint32_t cp;
    size_t count;
    if (p[0] < 0x80u) { cp = p[0]; count = 1; }
    else if ((p[0] & 0xe0u) == 0xc0u) {
        if (!p[1] || (p[1] & 0xc0u) != 0x80u) return false;
        cp = ((uint32_t)(p[0] & 0x1fu) << 6) | (uint32_t)(p[1] & 0x3fu); count = 2;
        if (cp < 0x80u) return false;
    } else if ((p[0] & 0xf0u) == 0xe0u) {
        if (!p[1] || !p[2] || (p[1] & 0xc0u) != 0x80u || (p[2] & 0xc0u) != 0x80u)
            return false;
        cp = ((uint32_t)(p[0] & 0x0fu) << 12) | ((uint32_t)(p[1] & 0x3fu) << 6) |
             (uint32_t)(p[2] & 0x3fu); count = 3;
        if (cp < 0x800u) return false;
    } else if ((p[0] & 0xf8u) == 0xf0u) {
        if (!p[1] || !p[2] || !p[3] || (p[1] & 0xc0u) != 0x80u ||
            (p[2] & 0xc0u) != 0x80u || (p[3] & 0xc0u) != 0x80u) return false;
        cp = ((uint32_t)(p[0] & 0x07u) << 18) | ((uint32_t)(p[1] & 0x3fu) << 12) |
             ((uint32_t)(p[2] & 0x3fu) << 6) | (uint32_t)(p[3] & 0x3fu); count = 4;
        if (cp < 0x10000u || cp > 0x10ffffu) return false;
    } else return false;
    if (cp >= 0xd800u && cp <= 0xdfffu) return false;
    *cursor = p + count;
    *out_cp = cp;
    return true;
}

static bool jni_jni_export_append_mangled(char *out, size_t cap, size_t *used,
                                      const char *text) {
    if (!out || !used || !text) return false;
    const unsigned char *p = (const unsigned char *)text;
    while (*p) {
        if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
            (*p >= '0' && *p <= '9')) {
            char one[2] = {(char)*p++, '\0'};
            if (!jni_jni_export_append_literal(out, cap, used, one)) return false;
            continue;
        }
        if (*p == '/') { p++; if (!jni_jni_export_append_literal(out, cap, used, "_")) return false; continue; }
        if (*p == '_') { p++; if (!jni_jni_export_append_literal(out, cap, used, "_1")) return false; continue; }
        if (*p == ';') { p++; if (!jni_jni_export_append_literal(out, cap, used, "_2")) return false; continue; }
        if (*p == '[') { p++; if (!jni_jni_export_append_literal(out, cap, used, "_3")) return false; continue; }
        uint32_t cp = 0;
        if (!jni_jni_export_decode_utf8(&p, &cp)) return false;
        if (cp <= 0xffffu) {
            if (!jni_jni_export_append_hex16(out, cap, used, (uint16_t)cp)) return false;
        } else {
            cp -= 0x10000u;
            const uint16_t high = (uint16_t)(0xd800u + (cp >> 10));
            const uint16_t low = (uint16_t)(0xdc00u + (cp & 0x3ffu));
            if (!jni_jni_export_append_hex16(out, cap, used, high) ||
                !jni_jni_export_append_hex16(out, cap, used, low)) return false;
        }
    }
    return true;
}

bool ssrnx_jni_build_export_name(const char *class_name, const char *method_name,
                                  const char *signature, bool long_name,
                                  char *out, size_t out_cap) {
    if (!class_name || !method_name || !signature || !out || out_cap < 8u) return false;
    size_t used = 0;
    out[0] = '\0';
    if (!jni_jni_export_append_literal(out, out_cap, &used, "Java_") ||
        !jni_jni_export_append_mangled(out, out_cap, &used, class_name) ||
        !jni_jni_export_append_literal(out, out_cap, &used, "_") ||
        !jni_jni_export_append_mangled(out, out_cap, &used, method_name)) return false;
    if (!long_name) return true;
    const char *open = strchr(signature, '(');
    const char *close = open ? strchr(open + 1, ')') : NULL;
    if (!open || !close) return false;
    if (!jni_jni_export_append_literal(out, out_cap, &used, "__")) return false;
    const size_t params_len = (size_t)(close - (open + 1));
    if (params_len >= 512u) return false;
    char params[512];
    memcpy(params, open + 1, params_len);
    params[params_len] = '\0';
    return jni_jni_export_append_mangled(out, out_cap, &used, params);
}

typedef const void *const *jni_SsrnxJNIEnv;
typedef const void *const *jni_SsrnxJavaVM;
typedef long long jni_SsrnxJLong;

typedef struct jni_SsrnxJniClass jni_SsrnxJniClass;
typedef struct jni_SsrnxJniMethod jni_SsrnxJniMethod;
typedef struct jni_SsrnxJniField jni_SsrnxJniField;
typedef struct jni_SsrnxJniObject jni_SsrnxJniObject;
typedef struct jni_SsrnxJniRef jni_SsrnxJniRef;
typedef struct jni_SsrnxJniEnvState jni_SsrnxJniEnvState;
typedef struct jni_SsrnxJniNativeBinding jni_SsrnxJniNativeBinding;
typedef struct jni_SsrnxJniInterfaceLink jni_SsrnxJniInterfaceLink;

static jni_SsrnxJniClass *jni_resolve_class_lazy(const char *name, unsigned depth);
extern const char *ssrnx_package_core_library_origin(const char *class_name)
    __attribute__((weak));
/* Java-level blocking (Object.wait, contended monitors) is a tracked signal
 * safe point when the compat runtime is linked. */
extern int ssrnx_signal_safe_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                                            const struct timespec *deadline) __attribute__((weak));
static int jni_jni_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *deadline) {
    if (ssrnx_signal_safe_cond_timedwait) return ssrnx_signal_safe_cond_timedwait(cond, mutex, deadline);
    return deadline ? pthread_cond_timedwait(cond, mutex, deadline) : pthread_cond_wait(cond, mutex);
}
/* Declared API-30 platform surface (source/android/api_stubs.c). */
extern bool ssrnx_api_class_info(const char *class_name, char *out_super, size_t super_cap,
                                 char *out_interfaces, size_t interfaces_cap,
                                 bool *out_is_interface) __attribute__((weak));
extern bool ssrnx_api_declared_supertypes(const char *class_name, char *out_super, size_t super_cap,
                                          char *out_interfaces, size_t interfaces_cap,
                                          bool *out_is_interface) __attribute__((weak));
extern bool ssrnx_api_stub_bind_method(void *env, const char *class_name, const char *name,
                                       const char *signature, bool is_static,
                                       char *out_owner, size_t owner_cap) __attribute__((weak));
extern bool ssrnx_api_stub_bind_field(void *env, const char *class_name, const char *name,
                                      const char *signature, bool is_static,
                                      char *out_owner, size_t owner_cap) __attribute__((weak));
extern size_t ssrnx_api_stub_declare_members(void *env, const char *class_name) __attribute__((weak));


typedef enum jni_SsrnxJniObjectKind {
    SSRNX_JNI_OBJECT_INSTANCE = 1,
    SSRNX_JNI_OBJECT_CLASS,
    SSRNX_JNI_OBJECT_STRING,
    SSRNX_JNI_OBJECT_BOOLEAN_ARRAY,
    SSRNX_JNI_OBJECT_BYTE_ARRAY,
    SSRNX_JNI_OBJECT_CHAR_ARRAY,
    SSRNX_JNI_OBJECT_SHORT_ARRAY,
    SSRNX_JNI_OBJECT_INT_ARRAY,
    SSRNX_JNI_OBJECT_LONG_ARRAY,
    SSRNX_JNI_OBJECT_FLOAT_ARRAY,
    SSRNX_JNI_OBJECT_DOUBLE_ARRAY,
    SSRNX_JNI_OBJECT_OBJECT_ARRAY,
    SSRNX_JNI_OBJECT_DIRECT_BUFFER,
    SSRNX_JNI_OBJECT_EXCEPTION,
} jni_SsrnxJniObjectKind;

typedef enum jni_SsrnxJniRefKind {
    SSRNX_JNI_REF_LOCAL = 1,
    SSRNX_JNI_REF_GLOBAL = 2,
    SSRNX_JNI_REF_WEAK = 3,
} jni_SsrnxJniRefKind;

typedef enum jni_SsrnxJniClassLoaderKind {
    SSRNX_JNI_LOADER_BOOT = 0,
    SSRNX_JNI_LOADER_APPLICATION = 1,
} jni_SsrnxJniClassLoaderKind;

struct jni_SsrnxJniMethod {
    char *name;
    char *signature;
    bool is_static;
    SsrnxJniMethodCallback callback;
    void *user_data;
    jni_SsrnxJniClass *owner;
    /* Canonical java.lang.reflect.Method/Constructor for ToReflectedMethod and
     * proxy dispatch; FromReflectedMethod maps it back to this method ID. */
    jni_SsrnxJniObject *reflected;
    /* Virtual-dispatch cache for Call<Type>Method on a receiver of another
     * class (guarded by g_lock, valid while dispatch_generation matches). */
    jni_SsrnxJniClass *dispatch_class;
    jni_SsrnxJniMethod *dispatch_target;
    size_t dispatch_generation;
    jni_SsrnxJniMethod *next;
};

struct jni_SsrnxJniNativeBinding {
    char *name;
    char *signature;
    void *fn_ptr;
    jni_SsrnxJniNativeBinding *next;
};

struct jni_SsrnxJniField {
    char *name;
    char *signature;
    bool is_static;
    size_t instance_index;
    SsrnxJniValue static_value;
    jni_SsrnxJniClass *owner;
    /* Canonical java.lang.reflect.Field for ToReflectedField and the Java
     * reflection model.  As with methods, Java does not guarantee object
     * identity; caching avoids allocating on every lookup. */
    jni_SsrnxJniObject *reflected;
    jni_SsrnxJniField *next;
};

struct jni_SsrnxJniInterfaceLink {
    jni_SsrnxJniClass *clazz;
    jni_SsrnxJniInterfaceLink *next;
};

typedef struct jni_SsrnxJniInitiatingLoader {
    jni_SsrnxJniObject *loader;
    struct jni_SsrnxJniInitiatingLoader *next;
} jni_SsrnxJniInitiatingLoader;

struct jni_SsrnxJniClass {
    char *name;
    jni_SsrnxJniClass *super;
    jni_SsrnxJniInterfaceLink *interfaces;
    bool hierarchy_provisional;
    /* Resolver epoch in which the package resolver last reported no DEX
     * definition for this provisional class (0 = never asked). */
    uint32_t resolver_miss_epoch;
    bool is_interface;
    bool primitive; /* Class object created for a primitive type descriptor. */
    bool platform_interfaces_checked; /* on-demand API interface link attempted */
    jni_SsrnxJniClassLoaderKind loader_kind;
    jni_SsrnxJniObject *defining_loader; /* model objects live until free_model */
    jni_SsrnxJniInitiatingLoader *initiating_loaders;
    /* java.lang.reflect.Proxy class: ordered interface list as passed to
     * Proxy.newProxyInstance (the order selects the Method for duplicates). */
    bool is_proxy;
    jni_SsrnxJniClass **proxy_interfaces;
    size_t proxy_interface_count;
    jni_SsrnxJniObject *class_object;
    jni_SsrnxJniMethod *methods;
    jni_SsrnxJniField *fields;
    jni_SsrnxJniNativeBinding *native_bindings;
    size_t instance_field_count;
    /* Every member the package class_def declares has been registered (see
     * ssrnx_jni_set_class_member_materializer). */
    bool members_materialized;
    /* The API-30 members of a platform class were declared for reflection. */
    bool platform_members_declared;
    /* Defined from the API-30 declaration table only: no framework model,
     * so its members are inert (see ssrnx/api_stubs.h). */
    bool platform_inert;
    jni_SsrnxJniClass *next;
};

typedef struct jni_SsrnxJniFieldCell {
    const struct jni_SsrnxJniField *field;
    SsrnxJniValue value;
    struct jni_SsrnxJniFieldCell *next;
} jni_SsrnxJniFieldCell;

struct jni_SsrnxJniObject {
    jni_SsrnxJniObjectKind kind;
    jni_SsrnxJniClass *clazz;
    void *host_data;
    pthread_mutex_t monitor_lock;
    pthread_cond_t monitor_cond;
    pthread_t monitor_owner;
    unsigned monitor_depth;
    /* Object.wait set: threads waiting and notifications not yet taken.
     * monitor_cond is shared with entry contention, so it is broadcast
     * whenever waiters exist. */
    unsigned monitor_waiters;
    unsigned monitor_notifies;
    bool monitor_owner_valid;
    bool monitor_initialized;
    /* Non-NULL for java.lang.reflect.Method/Constructor objects created by the
     * JNI layer. Framework-owned host_data stays available to other models. */
    jni_SsrnxJniMethod *reflected_method;
    /* Non-NULL for java.lang.reflect.Field objects created by the JNI layer. */
    jni_SsrnxJniField *reflected_field;
    jni_SsrnxJniObject *next;
    union {
        struct {
            /* Instance field values, one cell per field the object has
             * observed (absent = Java default value). Cells never move, so
             * slot pointers stay valid; new cells are published at the head
             * with release ordering under g_field_cell_locks. */
            struct jni_SsrnxJniFieldCell *cells;
        } instance;
        struct {
            uint16_t *chars;
            int32_t length;
        } string;
        struct {
            uint8_t *values;
            int32_t length;
        } boolean_array;
        struct {
            int8_t *values;
            int32_t length;
        } byte_array;
        struct {
            uint16_t *values;
            int32_t length;
        } char_array;
        struct {
            int16_t *values;
            int32_t length;
        } short_array;
        struct {
            int32_t *values;
            int32_t length;
        } int_array;
        struct {
            int64_t *values;
            int32_t length;
        } long_array;
        struct {
            float *values;
            int32_t length;
        } float_array;
        struct {
            double *values;
            int32_t length;
        } double_array;
        struct {
            jni_SsrnxJniObject **values;
            int32_t length;
            jni_SsrnxJniClass *component_class;
        } object_array;
        struct {
            void *address;
            int64_t capacity;
        } direct_buffer;
        struct {
            char *message;
        } exception;
        struct {
            jni_SsrnxJniClass *descriptor;
        } class_data;
    } u;
};

struct jni_SsrnxJniRef {
    jni_SsrnxJniObject *object;
    jni_SsrnxJniRefKind kind;
    jni_SsrnxJniEnvState *owner;
    unsigned frame_depth;
    bool alive;
    jni_SsrnxJniRef *next_all;
    /* Live local references of the owning thread, newest first. Only the
     * owning thread links or unlinks them, so no lock is needed. */
    jni_SsrnxJniRef *next_env;
    jni_SsrnxJniRef *prev_env;
    bool in_env_list;
};

typedef struct jni_SsrnxJniEnvHandle {
    const void *const *functions;
    jni_SsrnxJniEnvState *state;
} jni_SsrnxJniEnvHandle;

struct jni_SsrnxJniEnvState {
    jni_SsrnxJniEnvHandle handle;
    pthread_t thread;
    jni_SsrnxJniObject *pending_exception;
    jni_SsrnxJniRef *locals;
    unsigned frame_depth;
    unsigned application_loader_depth;
    bool daemon;
    char *attach_name;
};

typedef struct jni_SsrnxJniVmHandle {
    const void *const *functions;
} jni_SsrnxJniVmHandle;

static pthread_mutex_t jni_g_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t jni_g_env_key;
static bool jni_g_env_key_valid;
static bool jni_g_initialized;
static void *jni_g_jni_table[JNI_TABLE_SLOTS];
static void *jni_g_vm_table[JVM_TABLE_SLOTS];
static jni_SsrnxJniVmHandle jni_g_vm_handle;
static jni_SsrnxJniClass *jni_g_classes;
static jni_SsrnxJniObject *jni_g_objects;
static jni_SsrnxJniRef *jni_g_refs;
static size_t jni_g_class_count;
static size_t jni_g_method_count;
/* Bumped whenever a method is declared or a class hierarchy changes, which
 * invalidates every cached virtual-dispatch target. Guarded by g_lock. */
static size_t jni_g_dispatch_generation = 1;
static size_t jni_g_field_count;
static size_t jni_g_native_binding_count;
static size_t jni_g_instance_field_slot_count;
static SsrnxJniClassResolver jni_g_class_resolver;
static void *jni_g_class_resolver_user;
static SsrnxJniClassResolverEx jni_g_class_resolver_ex;
static void *jni_g_class_resolver_ex_user;
static SsrnxJniMethodResolver jni_g_method_resolver;
static SsrnxJniClassMemberMaterializer jni_g_member_materializer;
static void *jni_g_member_materializer_user;
static SsrnxJniClassInitializer jni_g_class_initializer;
static void *jni_g_class_initializer_user;
static void *jni_g_method_resolver_user;
/* Serializes proxy class definition so one ordered interface list maps to one
 * proxy class, as Proxy.getProxyClass requires within a class loader. */
static pthread_mutex_t jni_g_proxy_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned jni_g_proxy_class_serial;

static char *jni_dup_string(const char *s) {
    if (!s) return NULL;
    const size_t n = strlen(s) + 1;
    char *copy = malloc(n);
    if (copy) memcpy(copy, s, n);
    return copy;
}

static jni_SsrnxJniEnvState *jni_env_state_unchecked(void *env_handle) {
    if (!env_handle) return NULL;
    jni_SsrnxJniEnvHandle *handle = (jni_SsrnxJniEnvHandle *)env_handle;
    return handle->state;
}

static jni_SsrnxJniEnvState *jni_env_state(void *env_handle) {
    jni_SsrnxJniEnvState *env = jni_env_state_unchecked(env_handle);
    if (!env) return NULL;
    /* JNIEnv is thread-local by contract. Android's CheckJNI treats using an
     * environment from another thread as a fatal JNI programming error. SSRNX
     * rejects the call rather than letting foreign-thread state leak through. */
    if (!pthread_equal(env->thread, pthread_self())) {
        SSRNX_LOGCW("JNI", "JNIEnv used from non-owner thread env=%p", env_handle);
        return NULL;
    }
    return env;
}

/* Address sets of every SsrnxJniRef and SsrnxJniObject ever handed out. Both
 * stay allocated until free_model(), so membership lets callers that hold an
 * untyped 64-bit value (the DEX interpreter's registers) tell a reference from
 * a primitive without dereferencing it, and lets as_ref() reject a value that
 * is not a live reference to a real object (CheckJNI's "invalid jobject")
 * instead of faulting on it. Open addressing; writers hold g_lock, readers
 * probe without it. A grown table is retired, not freed, until free_model(),
 * so a reader still holding the previous table probes valid memory. */
typedef struct jni_PtrSetTable {
    size_t cap;
    struct jni_PtrSetTable *retired;
    uintptr_t slots[];
} jni_PtrSetTable;

typedef struct jni_PtrSet {
    jni_PtrSetTable *table;
    size_t count;
} jni_PtrSet;

static jni_PtrSet jni_g_ref_set;
static jni_PtrSet jni_g_object_set;

static size_t jni_ptr_set_slot(uintptr_t key, size_t cap) {
    uint64_t h = (uint64_t)key;
    h ^= h >> 33; h *= 0xff51afd7ed558ccdULL; h ^= h >> 33;
    return (size_t)h & (cap - 1u);
}

static bool jni_ptr_set_insert_locked(jni_PtrSet *set, uintptr_t key) {
    if (!key) return false;
    jni_PtrSetTable *table = set->table;
    if (!table || (set->count + 1u) * 2u > table->cap) {
        const size_t cap = table ? table->cap * 2u : 4096u;
        jni_PtrSetTable *grown = calloc(1, sizeof(*grown) + cap * sizeof(grown->slots[0]));
        if (!grown) return false;
        grown->cap = cap;
        grown->retired = table;
        for (size_t i = 0; table && i < table->cap; ++i) {
            if (!table->slots[i]) continue;
            size_t s = jni_ptr_set_slot(table->slots[i], cap);
            while (grown->slots[s]) s = (s + 1u) & (cap - 1u);
            grown->slots[s] = table->slots[i];
        }
        __atomic_store_n(&set->table, grown, __ATOMIC_RELEASE);
        table = grown;
    }
    size_t s = jni_ptr_set_slot(key, table->cap);
    while (table->slots[s]) {
        if (table->slots[s] == key) return true;
        s = (s + 1u) & (table->cap - 1u);
    }
    __atomic_store_n(&table->slots[s], key, __ATOMIC_RELEASE);
    set->count++;
    return true;
}

static bool jni_ptr_set_contains(const jni_PtrSet *set, uintptr_t key) {
    const jni_PtrSetTable *table = __atomic_load_n(&set->table, __ATOMIC_ACQUIRE);
    if (!key || !table) return false;
    size_t s = jni_ptr_set_slot(key, table->cap);
    for (size_t probes = 0; probes < table->cap; ++probes) {
        const uintptr_t slot = __atomic_load_n(&table->slots[s], __ATOMIC_ACQUIRE);
        if (slot == key) return true;
        if (!slot) return false;
        s = (s + 1u) & (table->cap - 1u);
    }
    return false;
}

static void jni_ptr_set_reset_locked(jni_PtrSet *set) {
    jni_PtrSetTable *table = set->table;
    __atomic_store_n(&set->table, NULL, __ATOMIC_RELEASE);
    while (table) {
        jni_PtrSetTable *older = table->retired;
        free(table);
        table = older;
    }
    set->count = 0;
}

static bool jni_ref_set_insert_locked(const jni_SsrnxJniRef *ref) {
    return jni_ptr_set_insert_locked(&jni_g_ref_set, (uintptr_t)ref);
}

static bool jni_ref_set_contains_locked(uintptr_t key) {
    return jni_ptr_set_contains(&jni_g_ref_set, key);
}

#ifdef __SWITCH__
static bool jni_invalid_ref_readable(uintptr_t address, size_t bytes) {
    MemoryInfo info;
    u32 page = 0;
    if (R_FAILED(svcQueryMemory(&info, &page, address))) return false;
    if (info.type == MemType_Unmapped || !(info.perm & Perm_R)) return false;
    return address >= info.addr && address + bytes <= info.addr + info.size;
}
#else
static bool jni_invalid_ref_readable(uintptr_t address, size_t bytes) {
    (void)address; (void)bytes;
    return false;
}
#endif

extern size_t ssrnx_dex_current_stack(SsrnxDexStackFrame *out_frames, size_t capacity)
    __attribute__((weak));

/* Logs (bounded) a value used as a JNI reference that is not one, with the
 * memory around it and the executing Java frames; the caller then treats it
 * as null. `overwritten` means the reference cell is known but its referent
 * is not an object SSRNX allocated, i.e. something wrote over the cell. */
static __attribute__((noinline, cold)) void report_invalid_ref(const void *value, bool overwritten,
                                                               const void *caller) {
    static uint32_t reports;
    if (__atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) >= 16u) return;
    SSRNX_LOGCE("JNI", "invalid JNI reference value=%p kind=%s caller=%p", value,
                overwritten ? "overwritten-cell" : "not-a-reference", caller);
    const uintptr_t at = (uintptr_t)value;
    for (int row = -4; row < 4; ++row) {
        const uintptr_t line = at + (uintptr_t)(intptr_t)(row * 16);
        if (!jni_invalid_ref_readable(line, 16u)) continue;
        const unsigned char *b = (const unsigned char *)line;
        char hex[16 * 3 + 1], text[17];
        for (int i = 0; i < 16; ++i) {
            snprintf(hex + i * 3, 4, "%02x ", b[i]);
            text[i] = b[i] >= 0x20 && b[i] < 0x7f ? (char)b[i] : '.';
        }
        text[16] = '\0';
        SSRNX_LOGCE("JNI", "invalid JNI reference memory %+04d: %s|%s|", row * 16, hex, text);
    }
    if (ssrnx_dex_current_stack) {
        SsrnxDexStackFrame frames[8];
        const size_t count = ssrnx_dex_current_stack(frames, 8u);
        for (size_t i = 0; i < count; ++i)
            SSRNX_LOGCE("JNI", "invalid JNI reference java frame %zu %s.%s", i,
                        frames[i].class_name, frames[i].method_name);
    }
}

static jni_SsrnxJniRef *jni_as_ref(void *value) {
    jni_SsrnxJniRef *ref = (jni_SsrnxJniRef *)value;
    if (!ref) return NULL;
    if (!jni_ptr_set_contains(&jni_g_ref_set, (uintptr_t)ref)) {
        report_invalid_ref(value, false, __builtin_return_address(0));
        return NULL;
    }
    if (!__atomic_load_n(&ref->alive, __ATOMIC_ACQUIRE)) return NULL;
    if (!jni_ptr_set_contains(&jni_g_object_set, (uintptr_t)ref->object)) {
        report_invalid_ref(value, true, __builtin_return_address(0));
        return NULL;
    }
    /* Local references belong to the creating JNI thread. Global and weak
     * globals are process-wide and may cross threads. */
    if (ref->kind == SSRNX_JNI_REF_LOCAL &&
        (!ref->owner || !pthread_equal(ref->owner->thread, pthread_self())))
        return NULL;
    return ref;
}

static jni_SsrnxJniObject *jni_object_from_ref(void *value) {
    jni_SsrnxJniRef *ref = jni_as_ref(value);
    return ref ? ref->object : NULL;
}

static jni_SsrnxJniClass *jni_class_from_ref(void *value) {
    jni_SsrnxJniObject *object = jni_object_from_ref(value);
    if (!object || object->kind != SSRNX_JNI_OBJECT_CLASS) return NULL;
    return object->u.class_data.descriptor;
}

/* Bumped when a new DEX lookup tier appears, so provisional classes that
 * the package resolver could not define are asked again. */
static uint32_t jni_g_class_resolver_epoch = 1u;

void ssrnx_jni_class_resolver_invalidate(void) {
    __atomic_add_fetch(&jni_g_class_resolver_epoch, 1u, __ATOMIC_ACQ_REL);
}

/* Name index over g_classes (open addressing). Lookups by name are on the
 * hot path of almost every JNI and interpreter operation, so they also run
 * without g_lock: writers hold it, a class is published only once fully
 * built, and grown tables are retired (not freed) until the model resets. */
typedef struct jni_ClassIndexTable {
    size_t cap;
    struct jni_ClassIndexTable *retired;
    jni_SsrnxJniClass *slots[];
} jni_ClassIndexTable;

static jni_ClassIndexTable *jni_g_class_index;
static size_t jni_g_class_index_count;
static bool jni_g_class_index_disabled;

static size_t jni_class_name_hash(const char *name) {
    uint64_t h = UINT64_C(1469598103934665603);
    for (const unsigned char *p = (const unsigned char *)name; *p; ++p) { h ^= *p; h *= UINT64_C(1099511628211); }
    return (size_t)(h ^ (h >> 29));
}

static bool jni_class_index_insert_locked(jni_SsrnxJniClass *clazz) {
    jni_ClassIndexTable *table = jni_g_class_index;
    if (!table || (jni_g_class_index_count + 1u) * 2u > table->cap) {
        const size_t cap = table ? table->cap * 2u : 4096u;
        jni_ClassIndexTable *grown = calloc(1, sizeof(*grown) + cap * sizeof(grown->slots[0]));
        if (!grown) return false;
        grown->cap = cap;
        grown->retired = table;
        for (size_t i = 0; table && i < table->cap; ++i) {
            if (!table->slots[i]) continue;
            size_t s = jni_class_name_hash(table->slots[i]->name) & (cap - 1u);
            while (grown->slots[s]) s = (s + 1u) & (cap - 1u);
            grown->slots[s] = table->slots[i];
        }
        __atomic_store_n(&jni_g_class_index, grown, __ATOMIC_RELEASE);
        table = grown;
    }
    size_t s = jni_class_name_hash(clazz->name) & (table->cap - 1u);
    while (table->slots[s]) s = (s + 1u) & (table->cap - 1u);
    __atomic_store_n(&table->slots[s], clazz, __ATOMIC_RELEASE);
    jni_g_class_index_count++;
    return true;
}

static void jni_class_index_reset_locked(void) {
    jni_ClassIndexTable *table = jni_g_class_index;
    __atomic_store_n(&jni_g_class_index, NULL, __ATOMIC_RELEASE);
    while (table) {
        jni_ClassIndexTable *older = table->retired;
        free(table);
        table = older;
    }
    jni_g_class_index_count = 0;
}

/* Index probe; *out_indexed is false when no index exists (linear lookup). */
static jni_SsrnxJniClass *jni_class_index_find(const char *name, bool *out_indexed) {
    const jni_ClassIndexTable *table = __atomic_load_n(&jni_g_class_index, __ATOMIC_ACQUIRE);
    *out_indexed = table != NULL;
    if (!table) return NULL;
    size_t s = jni_class_name_hash(name) & (table->cap - 1u);
    for (size_t probes = 0; probes < table->cap; ++probes, s = (s + 1u) & (table->cap - 1u)) {
        jni_SsrnxJniClass *c = __atomic_load_n(&table->slots[s], __ATOMIC_ACQUIRE);
        if (!c) return NULL;
        if (strcmp(c->name, name) == 0) return c;
    }
    return NULL;
}

static jni_SsrnxJniClass *jni_find_class_locked(const char *name) {
    if (!name) return NULL;
    bool indexed = false;
    jni_SsrnxJniClass *found = jni_class_index_find(name, &indexed);
    if (indexed) return found;
    for (jni_SsrnxJniClass *clazz = jni_g_classes; clazz; clazz = clazz->next) {
        if (strcmp(clazz->name, name) == 0) return clazz;
    }
    return NULL;
}

static jni_SsrnxJniObject *jni_alloc_object_locked(jni_SsrnxJniObjectKind kind, jni_SsrnxJniClass *clazz) {
    jni_SsrnxJniObject *object = calloc(1, sizeof(*object));
    if (!object) return NULL;
    if (pthread_mutex_init(&object->monitor_lock, NULL) != 0) {
        free(object);
        return NULL;
    }
    if (pthread_cond_init(&object->monitor_cond, NULL) != 0) {
        pthread_mutex_destroy(&object->monitor_lock);
        free(object);
        return NULL;
    }
    if (!jni_ptr_set_insert_locked(&jni_g_object_set, (uintptr_t)object)) {
        pthread_cond_destroy(&object->monitor_cond);
        pthread_mutex_destroy(&object->monitor_lock);
        free(object);
        return NULL;
    }
    object->monitor_initialized = true;
    object->kind = kind;
    object->clazz = clazz;
    object->next = jni_g_objects;
    jni_g_objects = object;
    return object;
}

static jni_SsrnxJniRef *jni_new_ref_locked(jni_SsrnxJniEnvState *env, jni_SsrnxJniObject *object,
                                    jni_SsrnxJniRefKind kind) {
    if (!object) return NULL;
    jni_SsrnxJniRef *ref = calloc(1, sizeof(*ref));
    if (!ref) return NULL;
    if (!jni_ref_set_insert_locked(ref)) { free(ref); return NULL; }
    ref->object = object;
    ref->kind = kind;
    ref->owner = kind == SSRNX_JNI_REF_LOCAL ? env : NULL;
    ref->frame_depth = env ? env->frame_depth : 0;
    ref->alive = true;
    ref->next_all = jni_g_refs;
    jni_g_refs = ref;
    if (kind == SSRNX_JNI_REF_LOCAL && env) {
        ref->prev_env = NULL;
        ref->next_env = env->locals;
        if (env->locals) env->locals->prev_env = ref;
        env->locals = ref;
        ref->in_env_list = true;
    }
    return ref;
}

static void jni_env_locals_unlink(jni_SsrnxJniEnvState *env, jni_SsrnxJniRef *ref) {
    if (!env || !ref || !ref->in_env_list) return;
    if (ref->prev_env) ref->prev_env->next_env = ref->next_env;
    else env->locals = ref->next_env;
    if (ref->next_env) ref->next_env->prev_env = ref->prev_env;
    ref->next_env = ref->prev_env = NULL;
    ref->in_env_list = false;
}

/* Local references are created at the current frame depth and the list is
 * newest first, so every reference of the frames being popped sits at the
 * head: the walk stops at the first older reference instead of visiting the
 * thread's whole reference history. Dead references leave the list. */
static void jni_invalidate_env_locals_locked(jni_SsrnxJniEnvState *env, unsigned min_depth) {
    if (!env) return;
    jni_SsrnxJniRef *ref = env->locals;
    while (ref && ref->frame_depth >= min_depth) {
        jni_SsrnxJniRef *next = ref->next_env;
        __atomic_store_n(&ref->alive, false, __ATOMIC_RELEASE);
        jni_env_locals_unlink(env, ref);
        ref = next;
    }
}

static jni_SsrnxJniObject *jni_new_exception_locked(const char *class_name, const char *message) {
    jni_SsrnxJniClass *clazz = jni_find_class_locked(class_name);
    if (!clazz) clazz = jni_find_class_locked("java/lang/RuntimeException");
    jni_SsrnxJniObject *object = jni_alloc_object_locked(SSRNX_JNI_OBJECT_EXCEPTION, clazz);
    if (!object) return NULL;
    object->u.exception.message = jni_dup_string(message ? message : "");
    return object;
}

static void jni_set_exception(jni_SsrnxJniEnvState *env, const char *class_name, const char *message) {
    if (!env) return;
    SSRNX_PERF_LOCK(&jni_g_lock);
    if (!env->pending_exception) {
        env->pending_exception = jni_new_exception_locked(class_name, message);
    }
    pthread_mutex_unlock(&jni_g_lock);
}

static bool jni_class_is_assignable_to_depth(jni_SsrnxJniClass *actual, jni_SsrnxJniClass *wanted,
                                         unsigned depth) {
    if (!actual || !wanted || depth > 64u) return false;
    for (jni_SsrnxJniClass *c = actual; c; c = c->super) {
        if (c == wanted) return true;
        for (jni_SsrnxJniInterfaceLink *link = c->interfaces; link; link = link->next) {
            if (link->clazz == wanted ||
                jni_class_is_assignable_to_depth(link->clazz, wanted, depth + 1u)) return true;
        }
    }
    return false;
}

static bool jni_class_is_assignable_to(jni_SsrnxJniClass *actual, jni_SsrnxJniClass *wanted) {
    return jni_class_is_assignable_to_depth(actual, wanted, 0u);
}

/* Some application classes are first observed through reflection/member
 * materialization before their full DEX superclass chain is reflected in the
 * JNI class graph. Field IDs still obey the declaring-class rule: an instance
 * of a DEX subclass may access a field declared by a DEX superclass. When the
 * in-memory graph cannot prove that relation, consult the package class
 * resolver rather than rejecting a valid inherited field access. This is a
 * read-only fallback; it never makes unrelated classes compatible. */
static bool jni_class_assignable_name_via_resolver_walk(
    const char *actual_name, const char *wanted_name,
    SsrnxJniClassResolver resolver, void *resolver_user,
    SsrnxJniClassResolverEx resolver_ex, void *resolver_ex_user,
    unsigned depth) {
    if (!actual_name || !wanted_name || !actual_name[0] || !wanted_name[0] || depth > 64u)
        return false;
    if (strcmp(actual_name, wanted_name) == 0) return true;

    char super_name[512] = {0};
    char interfaces[1024] = {0};
    bool found = false;
    SsrnxResult rc = resolver_ex
        ? resolver_ex(actual_name, super_name, sizeof(super_name), interfaces,
                      sizeof(interfaces), &found, resolver_ex_user)
        : resolver(actual_name, super_name, sizeof(super_name), &found, resolver_user);
    if (rc != SSRNX_OK || !found) return false;

    if (super_name[0] && strcmp(super_name, actual_name) != 0 &&
        jni_class_assignable_name_via_resolver_walk(super_name, wanted_name,
                                                 resolver, resolver_user,
                                                 resolver_ex, resolver_ex_user,
                                                 depth + 1u))
        return true;

    /* The extended DEX resolver reports class_def.interfaces[] as newline-
     * separated JNI names. Traverse those edges as part of Java assignment
     * compatibility so a lazily/incompletely materialized JNI graph does not
     * make valid interface casts fail. */
    for (const char *cur = interfaces; cur && *cur;) {
        const char *end = strchr(cur, '\n');
        const size_t len = end ? (size_t)(end - cur) : strlen(cur);
        if (len > 0 && len < 512u) {
            char iface[512];
            memcpy(iface, cur, len);
            iface[len] = '\0';
            if (jni_class_assignable_name_via_resolver_walk(iface, wanted_name,
                                                         resolver, resolver_user,
                                                         resolver_ex, resolver_ex_user,
                                                         depth + 1u))
                return true;
        }
        if (!end) break;
        cur = end + 1;
    }
    return false;
}

static bool jni_class_assignable_name_via_resolver(const char *actual_name,
                                                const char *wanted_name) {
    if (!actual_name || !wanted_name || !actual_name[0] || !wanted_name[0]) return false;
    if (strcmp(actual_name, wanted_name) == 0) return true;

    SSRNX_PERF_LOCK(&jni_g_lock);
    SsrnxJniClassResolver resolver = jni_g_class_resolver;
    void *resolver_user = jni_g_class_resolver_user;
    SsrnxJniClassResolverEx resolver_ex = jni_g_class_resolver_ex;
    void *resolver_ex_user = jni_g_class_resolver_ex_user;
    pthread_mutex_unlock(&jni_g_lock);
    if (!resolver && !resolver_ex) return false;

    return jni_class_assignable_name_via_resolver_walk(actual_name, wanted_name,
                                                    resolver, resolver_user,
                                                    resolver_ex, resolver_ex_user, 0u);
}

static bool jni_class_inherits_name_via_resolver(const char *actual_name,
                                              const char *wanted_name) {
    if (!actual_name || !wanted_name || !actual_name[0] || !wanted_name[0]) return false;
    if (strcmp(actual_name, wanted_name) == 0) return true;

    SSRNX_PERF_LOCK(&jni_g_lock);
    SsrnxJniClassResolver resolver = jni_g_class_resolver;
    void *resolver_user = jni_g_class_resolver_user;
    SsrnxJniClassResolverEx resolver_ex = jni_g_class_resolver_ex;
    void *resolver_ex_user = jni_g_class_resolver_ex_user;
    pthread_mutex_unlock(&jni_g_lock);
    if (!resolver && !resolver_ex) return false;

    char current[512];
    if (snprintf(current, sizeof(current), "%s", actual_name) <= 0) return false;
    for (unsigned depth = 0; depth < 64u; ++depth) {
        char super_name[512] = {0};
        char interfaces[1024] = {0};
        bool found = false;
        SsrnxResult rc = resolver_ex
            ? resolver_ex(current, super_name, sizeof(super_name), interfaces,
                          sizeof(interfaces), &found, resolver_ex_user)
            : resolver(current, super_name, sizeof(super_name), &found, resolver_user);
        if (rc != SSRNX_OK || !found || !super_name[0]) return false;
        if (strcmp(super_name, wanted_name) == 0) return true;
        if (strcmp(super_name, current) == 0) return false;
        if (snprintf(current, sizeof(current), "%s", super_name) <= 0) return false;
    }
    return false;
}

static jni_SsrnxJniMethod *jni_find_interface_method(jni_SsrnxJniClass *clazz, const char *name,
                                             const char *signature, bool is_static,
                                             unsigned depth) {
    if (!clazz || depth > 64u) return NULL;
    for (jni_SsrnxJniInterfaceLink *link = clazz->interfaces; link; link = link->next) {
        for (jni_SsrnxJniMethod *m = link->clazz->methods; m; m = m->next) {
            if (m->is_static == is_static && strcmp(m->name, name) == 0 &&
                strcmp(m->signature, signature) == 0) return m;
        }
        jni_SsrnxJniMethod *nested = jni_find_interface_method(link->clazz, name, signature,
                                                       is_static, depth + 1u);
        if (nested) return nested;
    }
    return NULL;
}

static jni_SsrnxJniMethod *jni_find_declared_method(jni_SsrnxJniClass *clazz, const char *name,
                                           const char *signature, bool is_static) {
    if (!clazz) return NULL;
    for (jni_SsrnxJniMethod *m = clazz->methods; m; m = m->next) {
        if (m->is_static == is_static && strcmp(m->name, name) == 0 &&
            strcmp(m->signature, signature) == 0) return m;
    }
    return NULL;
}

/* JVMS 5.4.3.3: search the class and all of its superclasses first, then the
 * superinterfaces. An abstract interface method must not shadow a concrete
 * implementation inherited from a superclass (FutureTask.get() for a
 * ScheduledFutureTask reached through Future.get()). */
static jni_SsrnxJniMethod *jni_find_method(jni_SsrnxJniClass *clazz, const char *name,
                                  const char *signature, bool is_static) {
    for (jni_SsrnxJniClass *c = clazz; c; c = c->super) {
        for (jni_SsrnxJniMethod *m = c->methods; m; m = m->next) {
            if (m->is_static == is_static && strcmp(m->name, name) == 0 &&
                strcmp(m->signature, signature) == 0) return m;
        }
    }
    for (jni_SsrnxJniClass *c = clazz; c; c = c->super) {
        jni_SsrnxJniMethod *interface_method = jni_find_interface_method(c, name, signature, is_static, 0u);
        if (interface_method) return interface_method;
    }
    return NULL;
}

static jni_SsrnxJniField *jni_find_field(jni_SsrnxJniClass *clazz, const char *name,
                                const char *signature, bool is_static) {
    for (jni_SsrnxJniClass *c = clazz; c; c = c->super) {
        for (jni_SsrnxJniField *f = c->fields; f; f = f->next) {
            if (f->is_static == is_static && strcmp(f->name, name) == 0 &&
                strcmp(f->signature, signature) == 0) return f;
        }
    }
    return NULL;
}

static int jni_descriptor_args(const char *signature, char *types, size_t cap) {
    if (!signature || signature[0] != '(') return -1;
    size_t count = 0, slots = 0;
    const char *p = signature + 1;
    while (*p && *p != ')') {
        char type = *p++;
        if (type == 'L') {
            while (*p && *p != ';') p++;
            if (*p != ';') return -1;
            p++;
            type = 'L';
        } else if (type == '[') {
            while (*p == '[') p++;
            if (*p == 'L') {
                p++;
                while (*p && *p != ';') p++;
                if (*p != ';') return -1;
                p++;
            } else if (*p) {
                p++;
            } else {
                return -1;
            }
            type = 'L';
        } else if (!strchr("ZBCSIJFD", type)) {
            return -1;
        }
        slots += type == 'J' || type == 'D' ? 2u : 1u;
        if (count >= cap || slots > SSRNX_JNI_MAX_PARAMETER_SLOTS) return -1;
        types[count++] = type;
    }
    return *p == ')' ? (int)count : -1;
}

static int jni_collect_va_args(jni_SsrnxJniMethod *method, va_list ap, SsrnxJniValue *values,
                           size_t cap, size_t *out_count) {
    char types[SSRNX_JNI_MAX_PARAMETER_SLOTS];
    const int n = jni_descriptor_args(method ? method->signature : NULL, types, sizeof(types));
    if (n < 0 || (size_t)n > cap) return -1;
    for (int i = 0; i < n; ++i) {
        switch (types[i]) {
            case 'Z': values[i].z = (uint8_t)va_arg(ap, int); break;
            case 'B': values[i].b = (int8_t)va_arg(ap, int); break;
            case 'C': values[i].c = (uint16_t)va_arg(ap, int); break;
            case 'S': values[i].s = (int16_t)va_arg(ap, int); break;
            case 'I': values[i].i = (int32_t)va_arg(ap, int); break;
            case 'J': values[i].j = (int64_t)va_arg(ap, int64_t); break;
            case 'F': values[i].f = (float)va_arg(ap, double); break;
            case 'D': values[i].d = va_arg(ap, double); break;
            case 'L': values[i].l = va_arg(ap, void *); break;
            default: return -1;
        }
    }
    *out_count = (size_t)n;
    return 0;
}

static bool jni_proxy_dispatches_method(jni_SsrnxJniClass *proxy, const jni_SsrnxJniMethod *method);
static SsrnxJniValue jni_proxy_invoke(void *env_handle, void *proxy_ref, jni_SsrnxJniObject *proxy,
                                  jni_SsrnxJniMethod *method, const SsrnxJniValue *args);

static SsrnxJniValue jni_invoke_method(void *env_handle, void *receiver_ref,
                                  jni_SsrnxJniMethod *method, const SsrnxJniValue *args) {
    SsrnxJniValue zero = {0};
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || !method) return zero;

    jni_SsrnxJniObject *receiver = NULL;
    if (!method->is_static) {
        receiver = jni_object_from_ref(receiver_ref);
        if (!receiver || !jni_class_is_assignable_to(receiver->clazz, method->owner)) {
            jni_set_exception(env, "java/lang/RuntimeException", "invalid JNI receiver");
            return zero;
        }
        /* java.lang.reflect.Proxy: interface methods and Object's hashCode,
         * equals and toString are routed to InvocationHandler.invoke. */
        if (jni_proxy_dispatches_method(receiver->clazz, method))
            return jni_proxy_invoke(env_handle, receiver_ref, receiver, method, args);
    }

    char types[SSRNX_JNI_MAX_PARAMETER_SLOTS];
    const int n = jni_descriptor_args(method->signature, types, sizeof(types));
    if (n < 0) {
        jni_set_exception(env, "java/lang/RuntimeException", "invalid JNI method descriptor");
        return zero;
    }
    if (!method->callback) return zero;
    return method->callback(env_handle, receiver, args, (size_t)n, method->user_data);
}

static void jni_env_destroy(jni_SsrnxJniEnvState *env) {
    if (!env) return;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_invalidate_env_locals_locked(env, 0);
    pthread_mutex_unlock(&jni_g_lock);
    free(env->attach_name);
    free(env);
}

static void jni_env_key_destructor(void *value) {
    jni_env_destroy((jni_SsrnxJniEnvState *)value);
}

/* ---- JavaVM invocation interface (real JNI slot positions) ---- */
static bool jni_supported_version(int32_t version);

static int32_t jni_vm_destroy(jni_SsrnxJavaVM *vm) {
    (void)vm;
    return SSRNX_JNI_ERR;
}

typedef struct jni_SsrnxJavaVMAttachArgs {
    int32_t version;
    const char *name;
    void *group;
} jni_SsrnxJavaVMAttachArgs;

static int32_t jni_vm_attach_common(jni_SsrnxJavaVM *vm, void **out_env, void *args, bool daemon) {
    (void)vm;
    if (!out_env || !jni_g_initialized) return SSRNX_JNI_EINVAL;
    *out_env = NULL;

    const jni_SsrnxJavaVMAttachArgs *attach = (const jni_SsrnxJavaVMAttachArgs *)args;
    if (attach && !jni_supported_version(attach->version))
        return SSRNX_JNI_EVERSION;

    /* Android's VM may create a Java thread itself and later let that thread
     * cross into JNI. Such a thread did not pass through SSRNX's translated
     * pthread_create trampoline, but direct NDK code still expects Bionic's
     * TPIDR_EL0 slots to be live. */
    jni_jni_activate_android_tls();

    jni_SsrnxJniEnvState *existing = jni_g_env_key_valid ? pthread_getspecific(jni_g_env_key) : NULL;
    if (existing) {
        *out_env = &existing->handle;
        return SSRNX_JNI_OK;
    }

    jni_SsrnxJniEnvState *env = calloc(1, sizeof(*env));
    if (!env) return SSRNX_JNI_ENOMEM;
    env->handle.functions = (const void *const *)jni_g_jni_table;
    env->handle.state = env;
    env->thread = pthread_self();
    env->daemon = daemon;
    if (attach && attach->name) {
        env->attach_name = jni_dup_string(attach->name);
        if (!env->attach_name) {
            free(env);
            return SSRNX_JNI_ENOMEM;
        }
    }
    if (pthread_setspecific(jni_g_env_key, env) != 0) {
        free(env->attach_name);
        free(env);
        return SSRNX_JNI_ERR;
    }
    *out_env = &env->handle;
    SSRNX_LOGCI("JNI", daemon ? "AttachCurrentThreadAsDaemon env=%p name=%s"
                             : "AttachCurrentThread env=%p name=%s",
                (void *)&env->handle,
                env->attach_name ? env->attach_name : "<unnamed>");
    return SSRNX_JNI_OK;
}

static int32_t jni_vm_attach(jni_SsrnxJavaVM *vm, void **out_env, void *args) {
    return jni_vm_attach_common(vm, out_env, args, false);
}

static int32_t jni_vm_attach_daemon(jni_SsrnxJavaVM *vm, void **out_env, void *args) {
    return jni_vm_attach_common(vm, out_env, args, true);
}

static int32_t jni_vm_detach(jni_SsrnxJavaVM *vm) {
    (void)vm;
    if (!jni_g_initialized || !jni_g_env_key_valid) return SSRNX_JNI_ERR;
    jni_SsrnxJniEnvState *env = pthread_getspecific(jni_g_env_key);
    if (!env) return SSRNX_JNI_EDETACHED;
    /* Android does not allow DetachCurrentThread while Java/native VM frames
     * are active on the thread. SSRNX uses the application-loader context as
     * the explicit boundary around JNI_OnLoad and Java->native invocation. */
    if (env->application_loader_depth != 0) return SSRNX_JNI_ERR;
    (void)pthread_setspecific(jni_g_env_key, NULL);
    SSRNX_LOGCI("JNI", "DetachCurrentThread env=%p", (void *)&env->handle);
    jni_env_destroy(env);
    return SSRNX_JNI_OK;
}

static bool jni_supported_version(int32_t version) {
    return version == SSRNX_JNI_VERSION_1_1 || version == SSRNX_JNI_VERSION_1_2 ||
           version == SSRNX_JNI_VERSION_1_4 || version == SSRNX_JNI_VERSION_1_6;
}

static int32_t jni_vm_get_env(jni_SsrnxJavaVM *vm, void **out_env, int32_t version) {
    (void)vm;
    if (!out_env) return SSRNX_JNI_EINVAL;
    *out_env = NULL;
    if (!jni_supported_version(version)) return SSRNX_JNI_EVERSION;
    if (!jni_g_initialized || !jni_g_env_key_valid) return SSRNX_JNI_EDETACHED;
    jni_SsrnxJniEnvState *env = pthread_getspecific(jni_g_env_key);
    if (!env) return SSRNX_JNI_EDETACHED;
    *out_env = &env->handle;
    return SSRNX_JNI_OK;
}

/* ---- JNIEnv interface ---- */
static SsrnxResult jni_register_class_internal_ex_loader(const char *name, const char *super_name,
                                                       bool hierarchy_provisional,
                                                       jni_SsrnxJniClassLoaderKind loader_kind);
static SsrnxResult jni_register_class_internal_ex(const char *name, const char *super_name,
                                               bool hierarchy_provisional);
static SsrnxResult jni_register_class_internal(const char *name, const char *super_name);
static int32_t jni_jni_get_version(jni_SsrnxJNIEnv *env) {
    return jni_env_state(env) ? SSRNX_JNI_VERSION_1_6 : 0;
}

static void jni_link_dex_interfaces(const char *name, const char *interfaces, unsigned depth);
static size_t jni_link_declared_platform_interfaces(const char *class_name);

/* Array types exist for every component type (JLS 10.8). SSRNX records the
 * exact runtime array class and, for reference arrays, the component class so
 * JNI IsInstanceOf/IsAssignableFrom and ArrayStoreException follow Java array
 * covariance instead of treating every reference-array type as equivalent. */
static bool jni_array_descriptor_valid(const char *name) {
    if (!name || name[0] != '[') return false;
    const char *p = name;
    while (*p == '[') ++p;
    if (p - name > 255) return false;
    if (*p == 'L') {
        const char *end = strchr(p, ';');
        return end && end > p + 1 && end[1] == '\0';
    }
    return p[0] && strchr("ZBCSIJFD", p[0]) && p[1] == '\0';
}

static bool jni_array_descriptor_assignable(const char *actual, const char *wanted) {
    if (!actual || !wanted || actual[0] != '[' || wanted[0] != '[') return false;
    if (strcmp(actual, wanted) == 0) return true;

    const char *a = actual + 1;
    const char *w = wanted + 1;
    if (*w == 'L' && strcmp(w, "Ljava/lang/Object;") == 0)
        return *a == 'L' || *a == '[';
    if (*a == '[' && *w == '[')
        return jni_array_descriptor_assignable(a, w);
    if (*a != 'L' || *w != 'L') return false;

    const char *ae = strchr(a + 1, ';');
    const char *we = strchr(w + 1, ';');
    if (!ae || !we || ae[1] != '\0' || we[1] != '\0') return false;
    char an[512], wn[512];
    const size_t al = (size_t)(ae - (a + 1));
    const size_t wl = (size_t)(we - (w + 1));
    if (!al || !wl || al >= sizeof(an) || wl >= sizeof(wn)) return false;
    memcpy(an, a + 1, al); an[al] = '\0';
    memcpy(wn, w + 1, wl); wn[wl] = '\0';

    jni_SsrnxJniClass *ac = jni_resolve_class_lazy(an, 0u);
    jni_SsrnxJniClass *wc = jni_resolve_class_lazy(wn, 0u);
    return ac && wc && jni_class_is_assignable_to(ac, wc);
}


static bool jni_object_is_assignable_to_class(const jni_SsrnxJniObject *object, jni_SsrnxJniClass *wanted) {
    if (!object || !wanted || !object->clazz) return false;
    if (object->clazz->name[0] == '[') {
        if (wanted->name[0] == '[')
            return jni_array_descriptor_assignable(object->clazz->name, wanted->name);
        return strcmp(wanted->name, "java/lang/Object") == 0 ||
               strcmp(wanted->name, "java/lang/Cloneable") == 0 ||
               strcmp(wanted->name, "java/io/Serializable") == 0;
    }
    if (jni_class_is_assignable_to(object->clazz, wanted)) return true;
    return jni_class_assignable_name_via_resolver(object->clazz->name, wanted->name);
}

static bool jni_array_object_is_instance(const jni_SsrnxJniObject *object, const char *descriptor) {
    if (!object || !jni_array_descriptor_valid(descriptor)) return false;
    switch (object->kind) {
        case SSRNX_JNI_OBJECT_OBJECT_ARRAY:
            return object->clazz && jni_array_descriptor_assignable(object->clazz->name, descriptor);
        case SSRNX_JNI_OBJECT_BOOLEAN_ARRAY: return strcmp(descriptor, "[Z") == 0;
        case SSRNX_JNI_OBJECT_BYTE_ARRAY: return strcmp(descriptor, "[B") == 0;
        case SSRNX_JNI_OBJECT_CHAR_ARRAY: return strcmp(descriptor, "[C") == 0;
        case SSRNX_JNI_OBJECT_SHORT_ARRAY: return strcmp(descriptor, "[S") == 0;
        case SSRNX_JNI_OBJECT_INT_ARRAY: return strcmp(descriptor, "[I") == 0;
        case SSRNX_JNI_OBJECT_LONG_ARRAY: return strcmp(descriptor, "[J") == 0;
        case SSRNX_JNI_OBJECT_FLOAT_ARRAY: return strcmp(descriptor, "[F") == 0;
        case SSRNX_JNI_OBJECT_DOUBLE_ARRAY: return strcmp(descriptor, "[D") == 0;
        default: return false;
    }
}

static jni_SsrnxJniClassLoaderKind jni_array_loader_kind_locked(const char *descriptor) {
    if (!descriptor || descriptor[0] != '[') return SSRNX_JNI_LOADER_BOOT;
    const char *p = descriptor;
    while (*p == '[') ++p;
    if (*p != 'L') return SSRNX_JNI_LOADER_BOOT;
    const char *end = strchr(p + 1, ';');
    if (!end || end[1] != '\0') return SSRNX_JNI_LOADER_BOOT;
    char component[512];
    const size_t n = (size_t)(end - (p + 1));
    if (n == 0 || n >= sizeof(component)) return SSRNX_JNI_LOADER_BOOT;
    memcpy(component, p + 1, n);
    component[n] = '\0';
    jni_SsrnxJniClass *clazz = jni_find_class_locked(component);
    return clazz ? clazz->loader_kind : SSRNX_JNI_LOADER_BOOT;
}

static jni_SsrnxJniClassLoaderKind jni_resolved_dex_loader_kind(const char *name) {
    return ssrnx_package_core_library_origin &&
           ssrnx_package_core_library_origin(name) != NULL
        ? SSRNX_JNI_LOADER_BOOT
        : SSRNX_JNI_LOADER_APPLICATION;
}

static void jni_link_dex_interfaces(const char *name, const char *interfaces, unsigned depth);

/* Last resort after the registry and package/core DEX: a class declared by the
 * API-30 platform surface exists on every device the app targets. It is
 * defined by the boot loader with its declared superclass and interfaces and
 * flagged platform_inert. */
static jni_SsrnxJniClass *jni_resolve_platform_api_class(const char *name, unsigned depth) {
    if (!ssrnx_api_class_info || !name || depth > 64u) return NULL;
    char super_name[512] = {0};
    char interfaces[1024] = {0};
    bool is_interface = false;
    if (!ssrnx_api_class_info(name, super_name, sizeof(super_name), interfaces,
                              sizeof(interfaces), &is_interface))
        return NULL;
    const char *parent_name = "java/lang/Object";
    if (super_name[0] && strcmp(super_name, name) != 0 && jni_resolve_class_lazy(super_name, depth + 1u))
        parent_name = super_name;
    if (jni_register_class_internal_ex_loader(name, parent_name, false, SSRNX_JNI_LOADER_BOOT) != SSRNX_OK)
        return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *clazz = jni_find_class_locked(name);
    if (clazz) {
        clazz->platform_inert = true;
        if (is_interface) clazz->is_interface = true;
    }
    pthread_mutex_unlock(&jni_g_lock);
    if (!clazz) return NULL;
    SSRNX_LOGCI("JNI", "platform API class defined class=%s super=%s interface=%d inert=1",
                name, parent_name, is_interface ? 1 : 0);
    jni_link_dex_interfaces(name, interfaces, depth);
    return clazz;
}

static jni_SsrnxJniClass *jni_resolve_class_lazy(const char *name, unsigned depth) {
    if (!name || !name[0] || depth > 64u) return NULL;
    /* Fast path: a published class whose hierarchy is final needs no resolver
     * (hierarchy_provisional only ever changes from true to false). */
    bool indexed = false;
    jni_SsrnxJniClass *known = jni_class_index_find(name, &indexed);
    if (known && (!__atomic_load_n(&known->hierarchy_provisional, __ATOMIC_ACQUIRE) ||
                  __atomic_load_n(&known->resolver_miss_epoch, __ATOMIC_ACQUIRE) ==
                      __atomic_load_n(&jni_g_class_resolver_epoch, __ATOMIC_ACQUIRE)))
        return known;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *clazz = jni_find_class_locked(name);
    const bool provisional = clazz && clazz->hierarchy_provisional;
    SsrnxJniClassResolver resolver = jni_g_class_resolver;
    void *resolver_user = jni_g_class_resolver_user;
    SsrnxJniClassResolverEx resolver_ex = jni_g_class_resolver_ex;
    void *resolver_ex_user = jni_g_class_resolver_ex_user;
    pthread_mutex_unlock(&jni_g_lock);
    const bool have_resolver = resolver || resolver_ex;
    if (clazz && (!provisional || !have_resolver)) return clazz;
    if (!clazz && jni_array_descriptor_valid(name)) {
        /* An array class is defined by the same loader as its reference
         * component type; primitive arrays are bootstrap-defined. Resolve a
         * reference component first so the loader provenance is known. */
        const char *p = name;
        while (*p == '[') ++p;
        if (*p == 'L') {
            const char *end = strchr(p + 1, ';');
            if (end && end[1] == '\0') {
                char component[512];
                const size_t n = (size_t)(end - (p + 1));
                if (n > 0 && n < sizeof(component)) {
                    memcpy(component, p + 1, n);
                    component[n] = '\0';
                    (void)jni_resolve_class_lazy(component, depth + 1u);
                }
            }
        }
        SSRNX_PERF_LOCK(&jni_g_lock);
        const jni_SsrnxJniClassLoaderKind loader_kind = jni_array_loader_kind_locked(name);
        pthread_mutex_unlock(&jni_g_lock);
        if (jni_register_class_internal_ex_loader(name, "java/lang/Object", false, loader_kind) != SSRNX_OK)
            return NULL;
        SSRNX_PERF_LOCK(&jni_g_lock);
        clazz = jni_find_class_locked(name);
        pthread_mutex_unlock(&jni_g_lock);
        return clazz;
    }
    if (!clazz && !have_resolver) return jni_resolve_platform_api_class(name, depth);

    char super_name[512] = {0};
    char interfaces[1024] = {0};
    bool found = false;
    SsrnxResult rc = resolver_ex
        ? resolver_ex(name, super_name, sizeof(super_name), interfaces, sizeof(interfaces),
                      &found, resolver_ex_user)
        : resolver(name, super_name, sizeof(super_name), &found, resolver_user);
    if (rc != SSRNX_OK) {
        SSRNX_LOGCW("JNI", "DEX class resolver error class=%s rc=%d", name, (int)rc);
        return clazz;
    }
    /* A provisional framework class remains a valid fallback if the package
     * has no DEX definition for it. Only package-backed metadata may replace
     * the temporary superclass. */
    if (!found) {
        if (clazz)
            __atomic_store_n(&clazz->resolver_miss_epoch,
                             __atomic_load_n(&jni_g_class_resolver_epoch, __ATOMIC_ACQUIRE), __ATOMIC_RELEASE);
        return clazz ? clazz : jni_resolve_platform_api_class(name, depth);
    }

    jni_SsrnxJniClass *parent = NULL;
    const char *parent_name = "java/lang/Object";
    bool parent_authoritative = true;
    if (super_name[0] && strcmp(super_name, name) != 0) {
        parent = jni_resolve_class_lazy(super_name, depth + 1u);
        if (parent) parent_name = super_name;
        else {
            SSRNX_PERF_LOCK(&jni_g_lock);
            parent = jni_find_class_locked(super_name);
            pthread_mutex_unlock(&jni_g_lock);
            if (parent) parent_name = super_name;
            else {
                parent_authoritative = false;
                SSRNX_LOGCW("JNI", "DEX superclass unavailable class=%s super=%s fallback=java/lang/Object provisional=1",
                             name, super_name);
            }
        }
    } else {
        SSRNX_PERF_LOCK(&jni_g_lock);
        parent = jni_find_class_locked(parent_name);
        pthread_mutex_unlock(&jni_g_lock);
    }

    if (clazz) {
        SSRNX_PERF_LOCK(&jni_g_lock);
        clazz = jni_find_class_locked(name);
        jni_SsrnxJniClass *resolved_parent = jni_find_class_locked(parent_name);
        if (clazz) clazz->loader_kind = jni_resolved_dex_loader_kind(name);
        if (clazz && clazz->hierarchy_provisional && parent_authoritative && resolved_parent &&
            resolved_parent != clazz && !jni_class_is_assignable_to(resolved_parent, clazz)) {
            const char *old_super = clazz->super ? clazz->super->name : "<none>";
            clazz->super = resolved_parent;
            __atomic_store_n(&clazz->hierarchy_provisional, false, __ATOMIC_RELEASE);
            jni_g_dispatch_generation++;
            SSRNX_LOGCI("JNI", "DEX class hierarchy reconciled class=%s super=%s previous=%s",
                        name, parent_name, old_super);
        }
        pthread_mutex_unlock(&jni_g_lock);
        if (clazz) {
            jni_link_dex_interfaces(name, interfaces, depth);
            if (jni_resolved_dex_loader_kind(name) == SSRNX_JNI_LOADER_BOOT)
                (void)jni_link_declared_platform_interfaces(name);
        }
        return clazz;
    }

    if (jni_register_class_internal_ex_loader(name, parent_name, !parent_authoritative,
                                          jni_resolved_dex_loader_kind(name)) != SSRNX_OK) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    clazz = jni_find_class_locked(name);
    pthread_mutex_unlock(&jni_g_lock);
    if (clazz) {
        SSRNX_LOGCI("JNI", "lazy DEX class registered class=%s super=%s", name, parent_name);
        jni_link_dex_interfaces(name, interfaces, depth);
        if (jni_resolved_dex_loader_kind(name) == SSRNX_JNI_LOADER_BOOT)
            (void)jni_link_declared_platform_interfaces(name);
    }
    return clazz;
}

/* Link a package class to the interfaces its class_def declares. An interface
 * the package does not define and the framework model does not know is still
 * a real type on Android (the class could not load otherwise), so it is
 * registered as a provisional Object-rooted type rather than dropped: type
 * tests against it must succeed for implementors. */
static void jni_link_dex_interfaces(const char *name, const char *interfaces, unsigned depth) {
    for (const char *p = interfaces; p && *p;) {
        const char *end = strchr(p, '\n');
        const size_t len = end ? (size_t)(end - p) : strlen(p);
        char iface[512];
        if (len > 0 && len < sizeof(iface)) {
            memcpy(iface, p, len);
            iface[len] = '\0';
            jni_SsrnxJniClass *iface_class = jni_resolve_class_lazy(iface, depth + 1u);
            if (!iface_class && jni_register_class_internal_ex_loader(iface, "java/lang/Object", true,
                                                                  SSRNX_JNI_LOADER_APPLICATION) == SSRNX_OK)
                SSRNX_LOGCI("JNI", "DEX interface registered provisionally interface=%s implementor=%s",
                            iface, name);
            const SsrnxResult rc = ssrnx_jni_register_interface(name, iface);
            if (rc != SSRNX_OK)
                SSRNX_LOGCW("JNI", "DEX interface link failed class=%s interface=%s rc=%d",
                            name, iface, (int)rc);
        }
        if (!end) break;
        p = end + 1;
    }
}

static bool jni_descriptor_boot_visible_locked(const char *name) {
    if (!name || !jni_array_descriptor_valid(name)) return false;
    const char *p = name;
    while (*p == '[') ++p;
    if (*p != 'L') return true;
    const char *end = strchr(p + 1, ';');
    if (!end || end[1] != '\0') return false;
    char component[512];
    const size_t n = (size_t)(end - (p + 1));
    if (n == 0 || n >= sizeof(component)) return false;
    memcpy(component, p + 1, n);
    component[n] = '\0';
    jni_SsrnxJniClass *clazz = jni_find_class_locked(component);
    return clazz && clazz->loader_kind == SSRNX_JNI_LOADER_BOOT;
}

static jni_SsrnxJniClass *jni_jni_find_class_for_context(jni_SsrnxJniEnvState *env, const char *name) {
    if (!env || !name || !name[0]) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *clazz = jni_find_class_locked(name);
    const bool application_context = env->application_loader_depth != 0;
    if (clazz && (application_context || clazz->loader_kind == SSRNX_JNI_LOADER_BOOT)) {
        pthread_mutex_unlock(&jni_g_lock);
        return clazz;
    }
    const bool boot_array = !application_context && jni_descriptor_boot_visible_locked(name);
    pthread_mutex_unlock(&jni_g_lock);

    /* JNI_OnLoad and native methods invoked from application classes use the
     * application's ClassLoader. A freshly attached native thread has no Java
     * caller and therefore sees only the system/bootstrap namespace. Core DEX
     * classes are part of that bootstrap namespace even before their first
     * lazy materialization. */
    const bool boot_core = !application_context &&
                           ssrnx_package_core_library_origin &&
                           ssrnx_package_core_library_origin(name) != NULL;
    if (application_context || boot_array || boot_core)
        return jni_resolve_class_lazy(name, 0u);
    return NULL;
}

static void *jni_jni_find_class(jni_SsrnxJNIEnv *env_handle, const char *name) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || !name) return NULL;
    jni_SsrnxJniClass *clazz = jni_jni_find_class_for_context(env, name);
    /* JNI FindClass does not resolve primitive names such as "int". Primitive
     * Class objects are nevertheless needed by java.lang.reflect.Field and
     * Method metadata, so SSRNX keeps them in the model but hides them from
     * FindClass just like ART does. */
    if (clazz && clazz->primitive) clazz = NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = clazz ? jni_new_ref_locked(env, clazz->class_object, SSRNX_JNI_REF_LOCAL) : NULL;
    pthread_mutex_unlock(&jni_g_lock);
    SSRNX_LOGCI("JNI", "FindClass name=%s loader=%s -> %p", name,
                env->application_loader_depth ? "application" : "system", (void *)ref);
    if (!clazz) {
        SSRNX_LOGCW("JNI", "FindClass failed name=%s loader=%s -> ClassNotFoundException",
                    name, env->application_loader_depth ? "application" : "system");
        jni_set_exception(env, "java/lang/ClassNotFoundException", name);
    }
    return ref;
}

static int32_t jni_jni_throw(jni_SsrnxJNIEnv *env_handle, void *throwable) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *object = jni_object_from_ref(throwable);
    if (!env || !object) return SSRNX_JNI_ERR;
    env->pending_exception = object;
    return SSRNX_JNI_OK;
}

static int32_t jni_jni_throw_new(jni_SsrnxJNIEnv *env_handle, void *clazz_ref, const char *message) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    if (!env || !clazz) return SSRNX_JNI_ERR;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniObject *object = jni_alloc_object_locked(SSRNX_JNI_OBJECT_EXCEPTION, clazz);
    if (object) object->u.exception.message = jni_dup_string(message ? message : "");
    pthread_mutex_unlock(&jni_g_lock);
    if (!object) return SSRNX_JNI_ENOMEM;
    env->pending_exception = object;
    SSRNX_LOGCI("JNI", "ThrowNew class=%s message=%s", clazz->name, message ? message : "");
    return SSRNX_JNI_OK;
}

static void *jni_jni_exception_occurred(jni_SsrnxJNIEnv *env_handle) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || !env->pending_exception) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, env->pending_exception, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

static void jni_jni_exception_describe(jni_SsrnxJNIEnv *env_handle) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || !env->pending_exception) return;
    jni_SsrnxJniObject *ex = env->pending_exception;
    SSRNX_LOGCW("JNI", "pending exception class=%s message=%s",
                ex->clazz ? ex->clazz->name : "<unknown>",
                ex->u.exception.message ? ex->u.exception.message : "");
}

static void jni_jni_exception_clear(jni_SsrnxJNIEnv *env_handle) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (env) env->pending_exception = NULL;
}

static int32_t jni_jni_push_local_frame(jni_SsrnxJNIEnv *env_handle, int32_t capacity) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || capacity < 0) return SSRNX_JNI_EINVAL;
    env->frame_depth++;
    return SSRNX_JNI_OK;
}

static void *jni_jni_pop_local_frame(jni_SsrnxJNIEnv *env_handle, void *result_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || env->frame_depth == 0) return NULL;
    jni_SsrnxJniObject *result = jni_object_from_ref(result_ref);
    const unsigned depth = env->frame_depth;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_invalidate_env_locals_locked(env, depth);
    env->frame_depth--;
    jni_SsrnxJniRef *out = result ? jni_new_ref_locked(env, result, SSRNX_JNI_REF_LOCAL) : NULL;
    pthread_mutex_unlock(&jni_g_lock);
    return out;
}

static void *jni_jni_new_global_ref(jni_SsrnxJNIEnv *env_handle, void *object_ref) {
    (void)env_handle;
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!object) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(NULL, object, SSRNX_JNI_REF_GLOBAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

static void jni_jni_delete_global_ref(jni_SsrnxJNIEnv *env_handle, void *ref_value) {
    (void)env_handle;
    jni_SsrnxJniRef *ref = jni_as_ref(ref_value);
    if (ref && ref->kind == SSRNX_JNI_REF_GLOBAL)
        __atomic_store_n(&ref->alive, false, __ATOMIC_RELEASE);
}

static void *jni_jni_new_weak_global_ref(jni_SsrnxJNIEnv *env_handle, void *object_ref) {
    (void)env_handle;
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!object) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(NULL, object, SSRNX_JNI_REF_WEAK);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

static void jni_jni_delete_weak_global_ref(jni_SsrnxJNIEnv *env_handle, void *ref_value) {
    (void)env_handle;
    jni_SsrnxJniRef *ref = jni_as_ref(ref_value);
    if (ref && ref->kind == SSRNX_JNI_REF_WEAK)
        __atomic_store_n(&ref->alive, false, __ATOMIC_RELEASE);
}

static void jni_jni_delete_local_ref(jni_SsrnxJNIEnv *env_handle, void *ref_value) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniRef *ref = jni_as_ref(ref_value);
    if (env && ref && ref->kind == SSRNX_JNI_REF_LOCAL && ref->owner == env) {
        __atomic_store_n(&ref->alive, false, __ATOMIC_RELEASE);
        jni_env_locals_unlink(env, ref);
    }
}

static uint8_t jni_jni_is_same_object(jni_SsrnxJNIEnv *env_handle, void *a, void *b) {
    (void)env_handle;
    if (!a || !b) return a == b ? JNI_TRUE : JNI_FALSE;
    jni_SsrnxJniObject *oa = jni_object_from_ref(a);
    jni_SsrnxJniObject *ob = jni_object_from_ref(b);
    return oa && ob && oa == ob ? JNI_TRUE : JNI_FALSE;
}

static void *jni_jni_new_local_ref(jni_SsrnxJNIEnv *env_handle, void *object_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!env || !object) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, object, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

static int32_t jni_jni_ensure_local_capacity(jni_SsrnxJNIEnv *env_handle, int32_t capacity) {
    return jni_env_state(env_handle) && capacity >= 0 ? SSRNX_JNI_OK : SSRNX_JNI_EINVAL;
}

static jni_SsrnxJniObject *jni_new_instance_object(jni_SsrnxJniClass *clazz) {
    if (!clazz) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniObject *object = jni_alloc_object_locked(SSRNX_JNI_OBJECT_INSTANCE, clazz);
    pthread_mutex_unlock(&jni_g_lock);
    return object;
}

static void jni_free_field_cells(jni_SsrnxJniObject *object) {
    for (jni_SsrnxJniFieldCell *cell = object->u.instance.cells; cell;) {
        jni_SsrnxJniFieldCell *next = cell->next;
        free(cell);
        cell = next;
    }
    object->u.instance.cells = NULL;
}

static void *jni_jni_alloc_object(jni_SsrnxJNIEnv *env_handle, void *clazz_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    jni_SsrnxJniObject *object = jni_new_instance_object(clazz);
    if (!env || !object) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, object, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

static void *jni_jni_new_object_a(jni_SsrnxJNIEnv *env_handle, void *clazz_ref, void *method_id,
                              const SsrnxJniValue *args) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    jni_SsrnxJniMethod *method = (jni_SsrnxJniMethod *)method_id;
    if (!env || !clazz || !method || method->is_static || strcmp(method->name, "<init>") != 0) {
        jni_set_exception(env, "java/lang/RuntimeException", "invalid constructor");
        return NULL;
    }
    void *ref = jni_jni_alloc_object(env_handle, clazz_ref);
    if (!ref) return NULL;
    (void)jni_invoke_method(env_handle, ref, method, args);
    if (env->pending_exception) {
        jni_jni_delete_local_ref(env_handle, ref);
        return NULL;
    }
    return ref;
}

static void *jni_jni_new_object_v(jni_SsrnxJNIEnv *env_handle, void *clazz_ref, void *method_id, va_list ap) {
    SsrnxJniValue values[SSRNX_JNI_MAX_PARAMETER_SLOTS];
    size_t count = 0;
    jni_SsrnxJniMethod *method = (jni_SsrnxJniMethod *)method_id;
    va_list copy;
    va_copy(copy, ap);
    const int rc = jni_collect_va_args(method, copy, values, SSRNX_JNI_MAX_PARAMETER_SLOTS, &count);
    va_end(copy);
    (void)count;
    return rc == 0 ? jni_jni_new_object_a(env_handle, clazz_ref, method_id, values) : NULL;
}

static void *jni_jni_new_object(jni_SsrnxJNIEnv *env_handle, void *clazz_ref, void *method_id, ...) {
    va_list ap;
    va_start(ap, method_id);
    void *result = jni_jni_new_object_v(env_handle, clazz_ref, method_id, ap);
    va_end(ap);
    return result;
}

static void *jni_jni_get_object_class(jni_SsrnxJNIEnv *env_handle, void *object_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!env || !object || !object->clazz) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, object->clazz->class_object, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

static uint8_t jni_jni_is_instance_of(jni_SsrnxJNIEnv *env_handle, void *object_ref, void *clazz_ref) {
    (void)env_handle;
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    if (!object_ref) return JNI_TRUE;
    return object && clazz && jni_object_is_assignable_to_class(object, clazz) ? JNI_TRUE : JNI_FALSE;
}

/* JNI GetSuperclass: NULL for java.lang.Object and for interfaces, otherwise
 * a local reference to the direct superclass. */
static void *jni_jni_get_superclass(jni_SsrnxJNIEnv *env_handle, void *clazz_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    if (!env || !clazz) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *super = clazz->is_interface ? NULL : clazz->super;
    jni_SsrnxJniRef *ref = super && super->class_object
        ? jni_new_ref_locked(env, super->class_object, SSRNX_JNI_REF_LOCAL) : NULL;
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

/* JNI IsAssignableFrom: whether an object of clazz1 can be cast to clazz2. */
static uint8_t jni_jni_is_assignable_from(jni_SsrnxJNIEnv *env_handle, void *clazz1_ref, void *clazz2_ref) {
    (void)env_handle;
    jni_SsrnxJniClass *from = jni_class_from_ref(clazz1_ref);
    jni_SsrnxJniClass *to = jni_class_from_ref(clazz2_ref);
    if (!from || !to) return JNI_FALSE;
    bool ok = false;
    if (from->name[0] == '[' || to->name[0] == '[') {
        if (from->name[0] == '[' && to->name[0] == '[')
            ok = jni_array_descriptor_assignable(from->name, to->name);
        else if (from->name[0] == '[')
            ok = strcmp(to->name, "java/lang/Object") == 0 ||
                 strcmp(to->name, "java/lang/Cloneable") == 0 ||
                 strcmp(to->name, "java/io/Serializable") == 0;
    } else {
        SSRNX_PERF_LOCK(&jni_g_lock);
        ok = jni_class_is_assignable_to(from, to) ||
             (from->is_interface && strcmp(to->name, "java/lang/Object") == 0);
        pthread_mutex_unlock(&jni_g_lock);
        if (!ok) ok = jni_class_assignable_name_via_resolver(from->name, to->name);
    }
    return ok ? JNI_TRUE : JNI_FALSE;
}

static void jni_materialize_reflected_classes(void *env_handle, jni_SsrnxJniClass *clazz,
                                          bool declared_only);
static bool jni_jni_initialize_class_for_member_id(jni_SsrnxJniEnvState *env, jni_SsrnxJniClass *clazz);

/* JNI method lookup shared by Get(Static)MethodID and host dispatch. A package
 * DEX declaration in clazz or a DEX superclass overrides a host model method
 * inherited from a framework ancestor, so the lazy resolver is consulted
 * unless clazz itself already declares the method. */
static jni_SsrnxJniMethod *jni_lookup_method(jni_SsrnxJniClass *clazz, const char *name,
                                     const char *signature, bool is_static) {
    if (!clazz || !name || !signature) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniMethod *method = jni_find_method(clazz, name, signature, is_static);
    SsrnxJniMethodResolver resolver = jni_g_method_resolver;
    void *resolver_user = jni_g_method_resolver_user;
    pthread_mutex_unlock(&jni_g_lock);
    if ((!method || method->owner != clazz) && resolver) {
        SsrnxJniMethodCallback callback = NULL;
        void *callback_user = NULL;
        bool found = false;
        SsrnxResult rc = resolver(clazz->name, name, signature, is_static,
                                  &callback, &callback_user, &found, resolver_user);
        if (rc == SSRNX_OK && found && callback) {
            rc = ssrnx_jni_register_method(clazz->name, name, signature, is_static,
                                            callback, callback_user);
            if (rc == SSRNX_OK) {
                SSRNX_PERF_LOCK(&jni_g_lock);
                method = jni_find_method(clazz, name, signature, is_static);
                pthread_mutex_unlock(&jni_g_lock);
            }
        } else if (rc != SSRNX_OK) {
            SSRNX_LOGCW("JNI", "Java method resolver error class=%s name=%s sig=%s rc=%d",
                        clazz->name, name, signature, (int)rc);
        }
    }
    return method;
}

static void *jni_jni_get_method_id_common(jni_SsrnxJNIEnv *env_handle, void *clazz_ref, const char *name,
                                      const char *signature, bool is_static) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    if (!env || !clazz || !name || !signature) return NULL;

    /* JNI specifies class initialization for both GetMethodID and
     * GetStaticMethodID, not only for field-ID lookup. Materialize package
     * members before initialization so <clinit> and inherited lookup see the
     * same class model. */
    jni_materialize_reflected_classes(env_handle, clazz, false);
    if (!jni_jni_initialize_class_for_member_id(env, clazz)) return NULL;

    jni_SsrnxJniMethod *method = jni_lookup_method(clazz, name, signature, is_static);
    char api_owner[512];
    if (!method && ssrnx_api_stub_bind_method &&
        ssrnx_api_stub_bind_method(env_handle, clazz->name, name, signature, is_static,
                                   api_owner, sizeof(api_owner)))
        method = jni_lookup_method(clazz, name, signature, is_static);
    SSRNX_LOGCI("JNI", "%s class=%s name=%s sig=%s -> %p",
                is_static ? "GetStaticMethodID" : "GetMethodID",
                clazz->name, name, signature, (void *)method);
    if (!method) {
        SSRNX_LOGCW("JNI", "%s failed class=%s name=%s sig=%s -> NoSuchMethodError",
                    is_static ? "GetStaticMethodID" : "GetMethodID", clazz->name, name, signature);
        jni_set_exception(env, "java/lang/NoSuchMethodError", name);
    }
    return method;
}

static void *jni_jni_get_method_id(jni_SsrnxJNIEnv *env, void *clazz, const char *name, const char *signature) {
    return jni_jni_get_method_id_common(env, clazz, name, signature, false);
}

static void *jni_jni_get_static_method_id(jni_SsrnxJNIEnv *env, void *clazz, const char *name, const char *signature) {
    return jni_jni_get_method_id_common(env, clazz, name, signature, true);
}

static bool jni_jni_initialize_class_for_member_id(jni_SsrnxJniEnvState *env, jni_SsrnxJniClass *clazz) {
    if (!env || !clazz) return false;
    SsrnxJniClassInitializer initializer = NULL;
    void *initializer_user = NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    initializer = jni_g_class_initializer;
    initializer_user = jni_g_class_initializer_user;
    pthread_mutex_unlock(&jni_g_lock);
    if (!initializer) return true;

    const SsrnxResult rc = initializer(&env->handle, clazz->name, initializer_user);
    if (rc == SSRNX_OK) return env->pending_exception == NULL;
    if (!env->pending_exception)
        jni_set_exception(env, "java/lang/ExceptionInInitializerError", clazz->name);
    SSRNX_LOGCW("JNI", "class initialization failed during member-ID lookup class=%s rc=%d",
                clazz->name, (int)rc);
    return false;
}

SsrnxResult ssrnx_jni_initialize_class_host(void *env_handle,
                                            const char *class_name) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || !class_name || !class_name[0]) return SSRNX_ERR_ARGUMENT;
    jni_SsrnxJniClass *clazz = jni_resolve_class_lazy(class_name, 0u);
    if (!clazz) return SSRNX_ERR_APK_UNSUPPORTED;
    return jni_jni_initialize_class_for_member_id(env, clazz)
        ? SSRNX_OK : SSRNX_ERR_APK_UNSUPPORTED;
}

static SsrnxJniValue jni_interface_method_dispatch(void *env_handle, void *object_token,
                                               const SsrnxJniValue *args, size_t arg_count,
                                               void *user_data);

/* Call<Type>Method[A|V] dispatch virtually: the implementation run is the one
 * the receiver's runtime class selects for the method ID's name and
 * descriptor, so a package override of a framework method runs even when the
 * ID came from the framework class (JNI spec "Call<type>Method Routines"; ART
 * FindVirtualMethodForVirtualOrInterface). Constructors and static methods are
 * never re-dispatched; interface IDs and proxies dispatch in invoke_method. */
static jni_SsrnxJniMethod *jni_virtual_dispatch_target(void *receiver_ref, jni_SsrnxJniMethod *method) {
    if (!method || method->is_static || method->name[0] == '<' ||
        method->callback == jni_interface_method_dispatch) return method;
    jni_SsrnxJniObject *receiver = jni_object_from_ref(receiver_ref);
    jni_SsrnxJniClass *clazz = receiver ? receiver->clazz : NULL;
    if (!clazz || clazz == method->owner || clazz->is_proxy) return method;
    SSRNX_PERF_LOCK(&jni_g_lock);
    if (method->dispatch_class == clazz && method->dispatch_generation == jni_g_dispatch_generation) {
        jni_SsrnxJniMethod *cached = method->dispatch_target;
        pthread_mutex_unlock(&jni_g_lock);
        return cached;
    }
    const bool assignable = jni_class_is_assignable_to(clazz, method->owner);
    pthread_mutex_unlock(&jni_g_lock);
    /* invoke_method reports the invalid receiver. */
    if (!assignable) return method;
    jni_SsrnxJniMethod *target = jni_lookup_method(clazz, method->name, method->signature, false);
    if (!target || !target->callback) target = method;
    SSRNX_PERF_LOCK(&jni_g_lock);
    method->dispatch_class = clazz;
    method->dispatch_target = target;
    method->dispatch_generation = jni_g_dispatch_generation;
    pthread_mutex_unlock(&jni_g_lock);
    return target;
}

/* Trace the exported native->Java boundary, not internal DEX dispatch. Each
 * thread samples methods independently so a hot rendering call cannot hide an
 * asynchronous task registration/result. Object identities survive JNI handle
 * copies, allowing task receivers and returned tasks to be correlated. */
static uint64_t jni_jni_call_diagnostic_sequence(jni_SsrnxJniMethod *method) {
    static _Thread_local struct { jni_SsrnxJniMethod *method; uint64_t calls; } targets[256];
    if (!method) return 0;
    for (size_t i = 0; i < sizeof(targets) / sizeof(targets[0]); ++i) {
        if (!targets[i].method) targets[i].method = method;
        if (targets[i].method == method) return ++targets[i].calls;
    }
    return 0;
}

static uint64_t jni_jni_call_diagnostic_us(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * UINT64_C(1000000) + (uint64_t)ts.tv_nsec / 1000u;
}

/* Observe native callers before Java changes their state; only registered
 * stack records are readable, and optimized-away frames remain unknown. */
static void jni_jni_call_diagnostic_stack(jni_SsrnxJniMethod *method, uint64_t sequence, uintptr_t fp) {
    uintptr_t low = 0, high = 0;
    if (!ssrnx_bionic_current_stack_bounds || !ssrnx_compat_describe_guest_address ||
        !ssrnx_bionic_current_stack_bounds(&low, &high) || high < low ||
        high - low < 2u * sizeof(uintptr_t)) return;
    unsigned shown = 0;
    for (unsigned depth = 0; depth < 16u; ++depth) {
        if (fp < low || fp > high - 2u * sizeof(uintptr_t) || (fp & 15u)) break;
        const uintptr_t *record = (const uintptr_t *)fp;
        const uintptr_t next = record[0], ret = record[1];
        char site[192];
        if (!ret) break;
        if (ssrnx_compat_describe_guest_address(ret, site, sizeof(site))) {
            SSRNX_LOGCI("JNICALLER", "class=%s method=%s pthread=%p sequence=%llu frame=%u pc=%p site=%s",
                method->owner->name, method->name, (void *)(uintptr_t)pthread_self(),
                (unsigned long long)sequence, shown, (void *)ret, site);
            if (++shown == 8u) break;
        }
        if (next <= fp) break;
        fp = next;
    }
}

static _Thread_local unsigned jni_g_jni_array_state_calls;

/* Validate the entire bounded range before copying diagnostic bytes. */
static bool jni_jni_diagnostic_copy(uintptr_t address, void *out, size_t size) {
#ifdef __SWITCH__
    if (!address || !out || !size || size > 512u || address > UINTPTR_MAX - size)
        return false;
    const uintptr_t end = address + size;
    for (uintptr_t cursor = address; cursor < end;) {
        MemoryInfo info;
        u32 page_info;
        if (R_FAILED(svcQueryMemory(&info, &page_info, cursor)) || !(info.perm & Perm_R) ||
            info.addr > cursor || !info.size || info.addr > UINTPTR_MAX - info.size)
            return false;
        const uintptr_t region_end = info.addr + info.size;
        if (region_end <= cursor) return false;
        cursor = region_end < end ? region_end : end;
    }
    memcpy(out, (const void *)address, size);
    return true;
#else
    (void)address; (void)out; (void)size;
    return false;
#endif
}

/* Saved stack words are candidates, not reconstructed registers or object types. */
static void jni_jni_call_diagnostic_objects(jni_SsrnxJniMethod *method, uint64_t sequence, uintptr_t fp) {
    uintptr_t low = 0, high = 0, seen[6];
    if (!ssrnx_bionic_current_stack_bounds || !ssrnx_compat_describe_guest_address ||
        !ssrnx_bionic_current_stack_bounds(&low, &high) || high < low ||
        high - low < 2u * sizeof(uintptr_t)) return;
    unsigned shown = 0, objects = 0;
    for (unsigned depth = 0; depth < 16u; ++depth) {
        if (fp < low || fp > high - 2u * sizeof(uintptr_t) || (fp & 15u)) break;
        const uintptr_t *record = (const uintptr_t *)fp;
        const uintptr_t next = record[0], ret = record[1];
        char site[192];
        if (!ret) break;
        if (ssrnx_compat_describe_guest_address(ret, site, sizeof(site))) {
            const uintptr_t window = 8u * sizeof(uintptr_t);
            uintptr_t begin = fp - low < window ? low : fp - window;
            if (begin & (sizeof(uintptr_t) - 1u))
                begin += sizeof(uintptr_t) - (begin & (sizeof(uintptr_t) - 1u));
            for (uintptr_t slot = begin; slot < fp; slot += sizeof(uintptr_t)) {
                const uintptr_t address = *(const uintptr_t *)slot;
                if (!address || (address & (sizeof(uintptr_t) - 1u))) continue;
                unsigned i;
                for (i = 0; i < objects && seen[i] != address; ++i) {}
                if (i < objects) continue;
                uintptr_t table;
                unsigned char bytes[512];
                if (!jni_jni_diagnostic_copy(address, &table, sizeof(table)) ||
                    !ssrnx_compat_describe_guest_address(table, site, sizeof(site)) ||
                    !jni_jni_diagnostic_copy(address, bytes, sizeof(bytes))) continue;
                seen[objects++] = address;
                for (size_t offset = 0; offset < sizeof(bytes); offset += 64u) {
                    static const char digits[] = "0123456789abcdef";
                    char hex[129];
                    for (size_t j = 0; j < 64u; ++j) {
                        hex[2u*j] = digits[bytes[offset+j] >> 4];
                        hex[2u*j+1u] = digits[bytes[offset+j] & 15u];
                    }
                    hex[128] = '\0';
                    SSRNX_LOGCI("JNIOBJECT", "candidate class=%.160s method=%.96s pthread=%p sequence=%llu frame=%u slot=%p address=%p table=%p tableSite=%.160s length=512 offset=%zu hex=%s",
                        method->owner->name, method->name, (void *)(uintptr_t)pthread_self(),
                        (unsigned long long)sequence, shown, (void *)slot, (void *)address,
                        (void *)table, site, offset, hex);
                }
                if (objects == 6u) return;
            }
            if (++shown == 8u) break;
        }
        if (next <= fp) break;
        fp = next;
    }
}

static SsrnxJniValue jni_call_a(void *env, void *receiver, void *method_id, const SsrnxJniValue *args,
                            bool dispatch) {
    jni_SsrnxJniMethod *method = (jni_SsrnxJniMethod *)method_id;
    if (dispatch) method = jni_virtual_dispatch_target(receiver, method);
    const uint64_t sequence = jni_jni_call_diagnostic_sequence(method);
    const bool trace = method && ((ssrnx_diag_full_trace_enabled && ssrnx_diag_full_trace_enabled()) ||
        (sequence && (sequence <= 4 || sequence % 300 == 0)));
    if (jni_g_jni_array_state_calls) {
        --jni_g_jni_array_state_calls;
        if (method) jni_jni_call_diagnostic_objects(method, sequence, (uintptr_t)__builtin_frame_address(0));
    }
    uint64_t begin_us = 0;
    if (trace) {
        if (sequence && sequence <= 4u)
            jni_jni_call_diagnostic_stack(method, sequence, (uintptr_t)__builtin_frame_address(0));
        char types[SSRNX_JNI_MAX_PARAMETER_SLOTS];
        const int count = jni_descriptor_args(method->signature, types, sizeof(types));
        uint64_t bits[8] = {0};
        for (int i = 0; args && i < count && i < 8; ++i) {
            /* JNI callers need only initialize the active jvalue member. */
            size_t width = sizeof(args[i].l);
            switch (types[i]) {
                case 'Z': width = sizeof(args[i].z); break;
                case 'B': width = sizeof(args[i].b); break;
                case 'C': width = sizeof(args[i].c); break;
                case 'S': width = sizeof(args[i].s); break;
                case 'I': width = sizeof(args[i].i); break;
                case 'J': width = sizeof(args[i].j); break;
                case 'F': width = sizeof(args[i].f); break;
                case 'D': width = sizeof(args[i].d); break;
            }
            memcpy(&bits[i], &args[i], width);
        }
        SSRNX_LOGCI("JNI", "Java call BEGIN class=%s method=%s sig=%s static=%d receiver=%p object=%p pthread=%p sequence=%llu pending=%d args=%d bits=(%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx)",
                    method->owner->name, method->name, method->signature, method->is_static ? 1 : 0,
                    receiver, (void *)jni_object_from_ref(receiver), (void *)(uintptr_t)pthread_self(),
                    (unsigned long long)sequence, ssrnx_jni_exception_pending_host(env) ? 1 : 0, count,
                    (unsigned long long)bits[0], (unsigned long long)bits[1],
                    (unsigned long long)bits[2], (unsigned long long)bits[3],
                    (unsigned long long)bits[4], (unsigned long long)bits[5],
                    (unsigned long long)bits[6], (unsigned long long)bits[7]);
        begin_us = jni_jni_call_diagnostic_us();
    }
    SsrnxJniValue result = jni_invoke_method(env, receiver, method, args);
    if (trace) {
        uint64_t result_bits = 0;
        memcpy(&result_bits, &result, sizeof(result_bits));
        const char *ret = strchr(method->signature, ')');
        jni_SsrnxJniObject *object = ret && (ret[1] == 'L' || ret[1] == '[') ? jni_object_from_ref(result.l) : NULL;
        const uint64_t end_us = jni_jni_call_diagnostic_us();
        SSRNX_LOGCI("JNI", "Java call END class=%s method=%s sig=%s pthread=%p sequence=%llu resultBits=0x%llx object=%p resultClass=%s pending=%d elapsedUs=%llu",
                    method->owner->name, method->name, method->signature, (void *)(uintptr_t)pthread_self(),
                    (unsigned long long)sequence, (unsigned long long)result_bits,
                    (void *)object, object && object->clazz ? object->clazz->name : "-",
                    ssrnx_jni_exception_pending_host(env) ? 1 : 0,
                    (unsigned long long)(end_us >= begin_us ? end_us - begin_us : 0));
        /* Observe native/Java capability arrays without changing their contents. */
        if (object && object->kind == SSRNX_JNI_OBJECT_INT_ARRAY) {
            static unsigned array_reports;
            if (__atomic_fetch_add(&array_reports, 1u, __ATOMIC_RELAXED) < 16u) {
                const int32_t length = object->u.int_array.length;
                const int32_t shown = length < 96 ? length : 96;
                for (int32_t start = 0; start < shown; start += 12) {
                    char values[160] = {0};
                    size_t used = 0;
                    int32_t count = shown - start;
                    if (count > 12) count = 12;
                    for (int32_t i = 0; i < count; ++i) {
                        const int written = snprintf(values + used, sizeof(values) - used, "%s%d",
                            i ? "," : "", object->u.int_array.values[start + i]);
                        if (written < 0 || (size_t)written >= sizeof(values) - used) break;
                        used += (size_t)written;
                    }
                    SSRNX_LOGCI("JNIARRAY", "result class=%s method=%s pthread=%p sequence=%llu length=%d start=%d count=%d values=[%s]",
                        method->owner->name, method->name, (void *)(uintptr_t)pthread_self(), (unsigned long long)sequence,
                        (int)length, (int)start, (int)count, values);
                }
                if (!length)
                    SSRNX_LOGCI("JNIARRAY", "result class=%s method=%s pthread=%p sequence=%llu length=0",
                        method->owner->name, method->name, (void *)(uintptr_t)pthread_self(), (unsigned long long)sequence);
            }
        }
    }
    return result;
}

static SsrnxJniValue jni_call_v(void *env, void *receiver, void *method_id, va_list ap, bool dispatch) {
    SsrnxJniValue zero = {0};
    SsrnxJniValue values[SSRNX_JNI_MAX_PARAMETER_SLOTS];
    size_t count = 0;
    jni_SsrnxJniMethod *method = (jni_SsrnxJniMethod *)method_id;
    va_list copy;
    va_copy(copy, ap);
    const int rc = jni_collect_va_args(method, copy, values, SSRNX_JNI_MAX_PARAMETER_SLOTS, &count);
    va_end(copy);
    (void)count;
    return rc == 0 ? jni_call_a(env, receiver, method, values, dispatch) : zero;
}

/* One Call*Method family per result type: plain varargs, V and A forms.
 * EXTRA is the jclass parameter of the CallNonvirtual forms (JNI passes it but
 * the method ID alone selects the implementation). */
#define DEFINE_CALL_TYPE(prefix, suffix, ctype, member, receiver_expr, dispatch, EXTRA) \
static ctype prefix##_##suffix##_a(jni_SsrnxJNIEnv *env, void *receiver, EXTRA() void *method, const SsrnxJniValue *args) { \
    SsrnxJniValue v = jni_call_a(env, (receiver_expr), method, args, (dispatch)); return (ctype)v.member; } \
static ctype prefix##_##suffix##_v(jni_SsrnxJNIEnv *env, void *receiver, EXTRA() void *method, va_list ap) { \
    SsrnxJniValue v = jni_call_v(env, (receiver_expr), method, ap, (dispatch)); return (ctype)v.member; } \
static ctype prefix##_##suffix(jni_SsrnxJNIEnv *env, void *receiver, EXTRA() void *method, ...) { \
    va_list ap; va_start(ap, method); SsrnxJniValue v = jni_call_v(env, (receiver_expr), method, ap, (dispatch)); \
    va_end(ap); return (ctype)v.member; }
#define DEFINE_CALL_VOID(prefix, receiver_expr, dispatch, EXTRA) \
static void prefix##_void_a(jni_SsrnxJNIEnv *env, void *receiver, EXTRA() void *method, const SsrnxJniValue *args) { \
    (void)jni_call_a(env, (receiver_expr), method, args, (dispatch)); } \
static void prefix##_void_v(jni_SsrnxJNIEnv *env, void *receiver, EXTRA() void *method, va_list ap) { \
    (void)jni_call_v(env, (receiver_expr), method, ap, (dispatch)); } \
static void prefix##_void(jni_SsrnxJNIEnv *env, void *receiver, EXTRA() void *method, ...) { \
    va_list ap; va_start(ap, method); (void)jni_call_v(env, (receiver_expr), method, ap, (dispatch)); va_end(ap); }
#define DEFINE_CALL_SET(prefix, receiver_expr, dispatch, EXTRA) \
    DEFINE_CALL_TYPE(prefix, object, void *, l, receiver_expr, dispatch, EXTRA) \
    DEFINE_CALL_TYPE(prefix, boolean, uint8_t, z, receiver_expr, dispatch, EXTRA) \
    DEFINE_CALL_TYPE(prefix, byte, int8_t, b, receiver_expr, dispatch, EXTRA) \
    DEFINE_CALL_TYPE(prefix, char, uint16_t, c, receiver_expr, dispatch, EXTRA) \
    DEFINE_CALL_TYPE(prefix, short, int16_t, s, receiver_expr, dispatch, EXTRA) \
    DEFINE_CALL_TYPE(prefix, int, int32_t, i, receiver_expr, dispatch, EXTRA) \
    DEFINE_CALL_TYPE(prefix, long, jni_SsrnxJLong, j, receiver_expr, dispatch, EXTRA) \
    DEFINE_CALL_TYPE(prefix, float, float, f, receiver_expr, dispatch, EXTRA) \
    DEFINE_CALL_TYPE(prefix, double, double, d, receiver_expr, dispatch, EXTRA) \
    DEFINE_CALL_VOID(prefix, receiver_expr, dispatch, EXTRA)
#define NO_EXTRA()
#define NONVIRTUAL_CLASS() void *clazz,

DEFINE_CALL_SET(jni_call, receiver, true, NO_EXTRA)
DEFINE_CALL_SET(jni_call_static, ((void)receiver, NULL), false, NO_EXTRA)
DEFINE_CALL_SET(jni_call_nonvirtual, ((void)clazz, receiver), false, NONVIRTUAL_CLASS)
#undef NO_EXTRA
#undef NONVIRTUAL_CLASS

static void *jni_jni_get_field_id_common(jni_SsrnxJNIEnv *env_handle, void *clazz_ref, const char *name,
                                     const char *signature, bool is_static) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    if (!env || !clazz || !name || !signature) return NULL;

    /* Package DEX classes are registered lazily. JNI field lookup is not a
     * reflection-only operation, so make the class' declared fields (and its
     * superclasses' fields) visible before resolution. */
    jni_materialize_reflected_classes(env_handle, clazz, false);

    /* JNI specifies that GetFieldID/GetStaticFieldID initialize an
     * uninitialized class before returning the ID. This is essential for enum
     * constants and other object-valued static fields populated by <clinit>. */
    if (!jni_jni_initialize_class_for_member_id(env, clazz)) return NULL;

    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniField *field = jni_find_field(clazz, name, signature, is_static);
    pthread_mutex_unlock(&jni_g_lock);
    char api_owner[512];
    if (!field && ssrnx_api_stub_bind_field &&
        ssrnx_api_stub_bind_field(env_handle, clazz->name, name, signature, is_static,
                                  api_owner, sizeof(api_owner))) {
        SSRNX_PERF_LOCK(&jni_g_lock);
        field = jni_find_field(clazz, name, signature, is_static);
        pthread_mutex_unlock(&jni_g_lock);
    }
    SSRNX_LOGCI("JNI", "%s class=%s name=%s sig=%s -> %p",
                is_static ? "GetStaticFieldID" : "GetFieldID",
                clazz->name, name, signature, (void *)field);
    if (!field) {
        SSRNX_LOGCW("JNI", "%s failed class=%s name=%s sig=%s -> NoSuchFieldError",
                    is_static ? "GetStaticFieldID" : "GetFieldID", clazz->name, name, signature);
        jni_set_exception(env, "java/lang/NoSuchFieldError", name);
    }
    return field;
}

static void *jni_jni_get_field_id(jni_SsrnxJNIEnv *env, void *clazz, const char *name, const char *signature) {
    return jni_jni_get_field_id_common(env, clazz, name, signature, false);
}

static void *jni_jni_get_static_field_id(jni_SsrnxJNIEnv *env, void *clazz, const char *name, const char *signature) {
    return jni_jni_get_field_id_common(env, clazz, name, signature, true);
}

/* Leaf locks for field-cell insertion, striped by object address. */
static pthread_mutex_t jni_g_field_cell_locks[32] = {
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
};

static SsrnxJniValue *jni_field_cell_find(jni_SsrnxJniObject *object, const jni_SsrnxJniField *field) {
    for (jni_SsrnxJniFieldCell *cell = __atomic_load_n(&object->u.instance.cells, __ATOMIC_ACQUIRE); cell;
         cell = cell->next)
        if (cell->field == field) return &cell->value;
    return NULL;
}

/* The storage slot of an instance field of object, created (holding the
 * field's default value) on first access. NULL for a static field, a field of
 * an unrelated class, or allocation failure. Field registration therefore
 * never has to revisit existing objects. */
static SsrnxJniValue *jni_instance_field_value_compatible(jni_SsrnxJniObject *object,
                                                        jni_SsrnxJniField *field,
                                                        bool owner_verified) {
    if (!object || !field || field->is_static || object->kind != SSRNX_JNI_OBJECT_INSTANCE)
        return NULL;
    if (!owner_verified && !jni_class_is_assignable_to(object->clazz, field->owner)) return NULL;
    SsrnxJniValue *slot = jni_field_cell_find(object, field);
    if (slot) return slot;
    pthread_mutex_t *lock = &jni_g_field_cell_locks[((uintptr_t)object >> 4) & 31u];
    pthread_mutex_lock(lock);
    slot = jni_field_cell_find(object, field);
    if (!slot) {
        jni_SsrnxJniFieldCell *cell = calloc(1, sizeof(*cell));
        if (cell) {
            cell->field = field;
            cell->next = object->u.instance.cells;
            __atomic_store_n(&object->u.instance.cells, cell, __ATOMIC_RELEASE);
            slot = &cell->value;
        }
    }
    pthread_mutex_unlock(lock);
    return slot;
}

static SsrnxJniValue *jni_instance_field_value(jni_SsrnxJniObject *object, jni_SsrnxJniField *field) {
    return jni_instance_field_value_compatible(object, field, false);
}

static void *jni_jni_get_object_field(jni_SsrnxJNIEnv *env_handle, void *object_ref, void *field_id) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    SsrnxJniValue *value = jni_instance_field_value(jni_object_from_ref(object_ref), (jni_SsrnxJniField *)field_id);
    jni_SsrnxJniObject *object = value ? (jni_SsrnxJniObject *)value->l : NULL;
    if (!env || !object) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, object, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

static uint8_t jni_jni_get_boolean_field(jni_SsrnxJNIEnv *env, void *object, void *field) {
    (void)env;
    SsrnxJniValue *value = jni_instance_field_value(jni_object_from_ref(object), (jni_SsrnxJniField *)field);
    return value ? value->z : 0;
}

static int32_t jni_jni_get_int_field(jni_SsrnxJNIEnv *env, void *object, void *field) {
    (void)env;
    SsrnxJniValue *value = jni_instance_field_value(jni_object_from_ref(object), (jni_SsrnxJniField *)field);
    return value ? value->i : 0;
}

static jni_SsrnxJLong jni_jni_get_long_field(jni_SsrnxJNIEnv *env, void *object, void *field) {
    (void)env;
    SsrnxJniValue *value = jni_instance_field_value(jni_object_from_ref(object), (jni_SsrnxJniField *)field);
    return value ? value->j : 0;
}

static float jni_jni_get_float_field(jni_SsrnxJNIEnv *env, void *object, void *field) {
    (void)env;
    SsrnxJniValue *value = jni_instance_field_value(jni_object_from_ref(object), (jni_SsrnxJniField *)field);
    return value ? value->f : 0.0f;
}

static double jni_jni_get_double_field(jni_SsrnxJNIEnv *env, void *object, void *field) {
    (void)env;
    SsrnxJniValue *value = jni_instance_field_value(jni_object_from_ref(object), (jni_SsrnxJniField *)field);
    return value ? value->d : 0.0;
}

static void jni_jni_set_object_field(jni_SsrnxJNIEnv *env, void *object, void *field, void *new_value) {
    (void)env;
    SsrnxJniValue *value = jni_instance_field_value(jni_object_from_ref(object), (jni_SsrnxJniField *)field);
    if (value) value->l = jni_object_from_ref(new_value);
}

static void jni_jni_set_boolean_field(jni_SsrnxJNIEnv *env, void *object, void *field, uint8_t new_value) {
    (void)env;
    SsrnxJniValue *value = jni_instance_field_value(jni_object_from_ref(object), (jni_SsrnxJniField *)field);
    if (value) value->z = new_value ? JNI_TRUE : JNI_FALSE;
}

static void jni_jni_set_int_field(jni_SsrnxJNIEnv *env, void *object, void *field, int32_t new_value) {
    (void)env;
    SsrnxJniValue *value = jni_instance_field_value(jni_object_from_ref(object), (jni_SsrnxJniField *)field);
    if (value) value->i = new_value;
}

static void jni_jni_set_long_field(jni_SsrnxJNIEnv *env, void *object, void *field, jni_SsrnxJLong new_value) {
    (void)env;
    SsrnxJniValue *value = jni_instance_field_value(jni_object_from_ref(object), (jni_SsrnxJniField *)field);
    if (value) value->j = new_value;
}

static void jni_jni_set_float_field(jni_SsrnxJNIEnv *env, void *object, void *field, float new_value) {
    (void)env;
    SsrnxJniValue *value = jni_instance_field_value(jni_object_from_ref(object), (jni_SsrnxJniField *)field);
    if (value) value->f = new_value;
}

static void jni_jni_set_double_field(jni_SsrnxJNIEnv *env, void *object, void *field, double new_value) {
    (void)env;
    SsrnxJniValue *value = jni_instance_field_value(jni_object_from_ref(object), (jni_SsrnxJniField *)field);
    if (value) value->d = new_value;
}

/* Get/Set<Byte|Char|Short>Field and their static forms. */
#define DEFINE_SMALL_FIELD(suffix, ctype, member) \
static ctype jni_get_##suffix##_field(jni_SsrnxJNIEnv *env, void *object, void *field) { \
    (void)env; \
    SsrnxJniValue *value = jni_instance_field_value(jni_object_from_ref(object), (jni_SsrnxJniField *)field); \
    return value ? value->member : 0; } \
static void jni_set_##suffix##_field(jni_SsrnxJNIEnv *env, void *object, void *field, ctype new_value) { \
    (void)env; \
    SsrnxJniValue *value = jni_instance_field_value(jni_object_from_ref(object), (jni_SsrnxJniField *)field); \
    if (value) value->member = new_value; } \
static ctype jni_get_static_##suffix##_field(jni_SsrnxJNIEnv *env, void *clazz_ref, void *field_id) { \
    (void)env; \
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref); \
    jni_SsrnxJniField *field = (jni_SsrnxJniField *)field_id; \
    return clazz && field && field->is_static && jni_class_is_assignable_to(clazz, field->owner) \
        ? field->static_value.member : 0; } \
static void jni_set_static_##suffix##_field(jni_SsrnxJNIEnv *env, void *clazz_ref, void *field_id, ctype value) { \
    (void)env; \
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref); \
    jni_SsrnxJniField *field = (jni_SsrnxJniField *)field_id; \
    if (clazz && field && field->is_static && jni_class_is_assignable_to(clazz, field->owner)) \
        field->static_value.member = value; }
DEFINE_SMALL_FIELD(byte, int8_t, b)
DEFINE_SMALL_FIELD(char, uint16_t, c)
DEFINE_SMALL_FIELD(short, int16_t, s)
#undef DEFINE_SMALL_FIELD

static void *jni_jni_get_static_object_field(jni_SsrnxJNIEnv *env_handle, void *clazz_ref, void *field_id) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    jni_SsrnxJniField *field = (jni_SsrnxJniField *)field_id;
    if (!env || !clazz || !field || !field->is_static || !jni_class_is_assignable_to(clazz, field->owner)) return NULL;
    jni_SsrnxJniObject *object = (jni_SsrnxJniObject *)field->static_value.l;
    if (!object) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, object, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

static uint8_t jni_jni_get_static_boolean_field(jni_SsrnxJNIEnv *env, void *clazz_ref, void *field_id) {
    (void)env;
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    jni_SsrnxJniField *field = (jni_SsrnxJniField *)field_id;
    return clazz && field && field->is_static && jni_class_is_assignable_to(clazz, field->owner) ? field->static_value.z : 0;
}

static int32_t jni_jni_get_static_int_field(jni_SsrnxJNIEnv *env, void *clazz_ref, void *field_id) {
    (void)env;
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    jni_SsrnxJniField *field = (jni_SsrnxJniField *)field_id;
    return clazz && field && field->is_static && jni_class_is_assignable_to(clazz, field->owner) ? field->static_value.i : 0;
}

static jni_SsrnxJLong jni_jni_get_static_long_field(jni_SsrnxJNIEnv *env, void *clazz_ref, void *field_id) {
    (void)env;
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    jni_SsrnxJniField *field = (jni_SsrnxJniField *)field_id;
    return clazz && field && field->is_static && jni_class_is_assignable_to(clazz, field->owner) ? field->static_value.j : 0;
}

static float jni_jni_get_static_float_field(jni_SsrnxJNIEnv *env, void *clazz_ref, void *field_id) {
    (void)env;
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    jni_SsrnxJniField *field = (jni_SsrnxJniField *)field_id;
    return clazz && field && field->is_static && jni_class_is_assignable_to(clazz, field->owner) ? field->static_value.f : 0.0f;
}

static double jni_jni_get_static_double_field(jni_SsrnxJNIEnv *env, void *clazz_ref, void *field_id) {
    (void)env;
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    jni_SsrnxJniField *field = (jni_SsrnxJniField *)field_id;
    return clazz && field && field->is_static && jni_class_is_assignable_to(clazz, field->owner) ? field->static_value.d : 0.0;
}

static void jni_jni_set_static_object_field(jni_SsrnxJNIEnv *env, void *clazz_ref, void *field_id, void *value_ref) {
    (void)env;
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    jni_SsrnxJniField *field = (jni_SsrnxJniField *)field_id;
    if (clazz && field && field->is_static && jni_class_is_assignable_to(clazz, field->owner)) {
        field->static_value.l = jni_object_from_ref(value_ref);
    }
}

static void jni_jni_set_static_boolean_field(jni_SsrnxJNIEnv *env, void *clazz_ref, void *field_id, uint8_t value) {
    (void)env;
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    jni_SsrnxJniField *field = (jni_SsrnxJniField *)field_id;
    if (clazz && field && field->is_static && jni_class_is_assignable_to(clazz, field->owner)) field->static_value.z = value ? JNI_TRUE : JNI_FALSE;
}

static void jni_jni_set_static_int_field(jni_SsrnxJNIEnv *env, void *clazz_ref, void *field_id, int32_t value) {
    (void)env;
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    jni_SsrnxJniField *field = (jni_SsrnxJniField *)field_id;
    if (clazz && field && field->is_static && jni_class_is_assignable_to(clazz, field->owner)) field->static_value.i = value;
}

static void jni_jni_set_static_long_field(jni_SsrnxJNIEnv *env, void *clazz_ref, void *field_id, jni_SsrnxJLong value) {
    (void)env;
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    jni_SsrnxJniField *field = (jni_SsrnxJniField *)field_id;
    if (clazz && field && field->is_static && jni_class_is_assignable_to(clazz, field->owner)) field->static_value.j = value;
}

static void jni_jni_set_static_float_field(jni_SsrnxJNIEnv *env, void *clazz_ref, void *field_id, float value) {
    (void)env;
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    jni_SsrnxJniField *field = (jni_SsrnxJniField *)field_id;
    if (clazz && field && field->is_static && jni_class_is_assignable_to(clazz, field->owner)) field->static_value.f = value;
}

static void jni_jni_set_static_double_field(jni_SsrnxJNIEnv *env, void *clazz_ref, void *field_id, double value) {
    (void)env;
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    jni_SsrnxJniField *field = (jni_SsrnxJniField *)field_id;
    if (clazz && field && field->is_static && jni_class_is_assignable_to(clazz, field->owner)) field->static_value.d = value;
}

static size_t jni_mutf8_length(const uint16_t *chars, int32_t length) {
    size_t bytes = 0;
    for (int32_t i = 0; i < length; ++i) {
        const uint16_t c = chars[i];
        if (c == 0) bytes += 2;
        else if (c <= 0x7f) bytes += 1;
        else if (c <= 0x7ff) bytes += 2;
        else bytes += 3;
    }
    return bytes;
}

static uint16_t *jni_mutf8_decode(const char *text, int32_t *out_length) {
    if (!text || !out_length) return NULL;
    const size_t bytes = strlen(text);
    uint16_t *chars = malloc((bytes + 1) * sizeof(uint16_t));
    if (!chars) return NULL;
    int32_t count = 0;
    for (size_t i = 0; i < bytes;) {
        const unsigned char a = (unsigned char)text[i++];
        if ((a & 0x80u) == 0) {
            chars[count++] = a;
        } else if ((a & 0xe0u) == 0xc0u && i < bytes) {
            const unsigned char b = (unsigned char)text[i++];
            if ((b & 0xc0u) != 0x80u) { free(chars); return NULL; }
            const uint16_t value = (uint16_t)(((a & 0x1fu) << 6) | (b & 0x3fu));
            /* Modified UTF-8 permits exactly one overlong encoding: C0 80 for
             * U+0000. Other overlong two-byte forms are malformed. */
            if (value < 0x80u && !(a == 0xc0u && b == 0x80u)) { free(chars); return NULL; }
            chars[count++] = value;
        } else if ((a & 0xf0u) == 0xe0u && i + 1 < bytes) {
            const unsigned char b = (unsigned char)text[i++];
            const unsigned char c = (unsigned char)text[i++];
            if ((b & 0xc0u) != 0x80u || (c & 0xc0u) != 0x80u) { free(chars); return NULL; }
            const uint16_t value = (uint16_t)(((a & 0x0fu) << 12) | ((b & 0x3fu) << 6) | (c & 0x3fu));
            if (value < 0x800u) { free(chars); return NULL; }
            /* Surrogate code units are intentionally accepted: JNI Modified
             * UTF-8 encodes supplementary characters as surrogate pairs. */
            chars[count++] = value;
        } else {
            free(chars);
            return NULL;
        }
    }
    *out_length = count;
    return chars;
}

static char *jni_mutf8_encode(const uint16_t *chars, int32_t length) {
    const size_t bytes = jni_mutf8_length(chars, length);
    char *text = malloc(bytes + 1);
    if (!text) return NULL;
    size_t out = 0;
    for (int32_t i = 0; i < length; ++i) {
        const uint16_t c = chars[i];
        if (c == 0) {
            text[out++] = (char)0xc0; text[out++] = (char)0x80;
        } else if (c <= 0x7f) {
            text[out++] = (char)c;
        } else if (c <= 0x7ff) {
            text[out++] = (char)(0xc0 | (c >> 6));
            text[out++] = (char)(0x80 | (c & 0x3f));
        } else {
            text[out++] = (char)(0xe0 | (c >> 12));
            text[out++] = (char)(0x80 | ((c >> 6) & 0x3f));
            text[out++] = (char)(0x80 | (c & 0x3f));
        }
    }
    text[out] = '\0';
    return text;
}

static void *jni_new_string_from_utf16(void *env_handle, const uint16_t *chars, int32_t length) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || length < 0 || (length && !chars)) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *clazz = jni_find_class_locked("java/lang/String");
    jni_SsrnxJniObject *object = jni_alloc_object_locked(SSRNX_JNI_OBJECT_STRING, clazz);
    if (object && length) {
        object->u.string.chars = malloc((size_t)length * sizeof(uint16_t));
        if (object->u.string.chars) memcpy(object->u.string.chars, chars, (size_t)length * sizeof(uint16_t));
        else object = NULL;
    }
    if (object) object->u.string.length = length;
    jni_SsrnxJniRef *ref = object ? jni_new_ref_locked(env, object, SSRNX_JNI_REF_LOCAL) : NULL;
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

static void *jni_jni_new_string(jni_SsrnxJNIEnv *env, const uint16_t *chars, int32_t length) {
    return jni_new_string_from_utf16(env, chars, length);
}

static int32_t jni_jni_get_string_length(jni_SsrnxJNIEnv *env, void *string_ref) {
    (void)env;
    jni_SsrnxJniObject *object = jni_object_from_ref(string_ref);
    return object && object->kind == SSRNX_JNI_OBJECT_STRING ? object->u.string.length : 0;
}

static const uint16_t *jni_jni_get_string_chars(jni_SsrnxJNIEnv *env_handle, void *string_ref, uint8_t *is_copy) {
    (void)env_handle;
    jni_SsrnxJniObject *object = jni_object_from_ref(string_ref);
    if (!object || object->kind != SSRNX_JNI_OBJECT_STRING) return NULL;
    const size_t bytes = (size_t)object->u.string.length * sizeof(uint16_t);
    uint16_t *copy = malloc(bytes ? bytes : sizeof(uint16_t));
    if (!copy) return NULL;
    if (bytes) memcpy(copy, object->u.string.chars, bytes);
    if (is_copy) *is_copy = JNI_TRUE;
    return copy;
}

static void jni_jni_release_string_chars(jni_SsrnxJNIEnv *env, void *string_ref, const uint16_t *chars) {
    (void)env; (void)string_ref; free((void *)chars);
}

static void *jni_jni_new_string_utf(jni_SsrnxJNIEnv *env_handle, const char *text) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env) return NULL;
    /* ART accepts a null native UTF pointer as a null jstring. It does not
     * manufacture an exception (or replace an already pending exception). */
    if (!text) return NULL;
    int32_t length = 0;
    uint16_t *chars = jni_mutf8_decode(text, &length);
    if (!chars && text[0]) {
        jni_set_exception(env, "java/lang/RuntimeException", "invalid modified UTF-8");
        return NULL;
    }
    void *string = jni_new_string_from_utf16(env_handle, chars, length);
    free(chars);
    SSRNX_LOGCI("JNI", "NewStringUTF text=%s -> %p", text ? text : "<null>", string);
    return string;
}

static int32_t jni_jni_get_string_utf_length(jni_SsrnxJNIEnv *env, void *string_ref) {
    (void)env;
    jni_SsrnxJniObject *object = jni_object_from_ref(string_ref);
    if (!object || object->kind != SSRNX_JNI_OBJECT_STRING) return 0;
    const size_t n = jni_mutf8_length(object->u.string.chars, object->u.string.length);
    return n > INT32_MAX ? INT32_MAX : (int32_t)n;
}

static const char *jni_jni_get_string_utf_chars(jni_SsrnxJNIEnv *env, void *string_ref, uint8_t *is_copy) {
    (void)env;
    jni_SsrnxJniObject *object = jni_object_from_ref(string_ref);
    if (!object || object->kind != SSRNX_JNI_OBJECT_STRING) return NULL;
    char *text = jni_mutf8_encode(object->u.string.chars, object->u.string.length);
    if (text && is_copy) *is_copy = JNI_TRUE;
    return text;
}

static void jni_jni_release_string_utf_chars(jni_SsrnxJNIEnv *env, void *string_ref, const char *chars) {
    (void)env; (void)string_ref; free((void *)chars);
}

static int32_t jni_jni_get_array_length(jni_SsrnxJNIEnv *env, void *array_ref) {
    (void)env;
    jni_SsrnxJniObject *object = jni_object_from_ref(array_ref);
    if (!object) return 0;
    if (object->kind == SSRNX_JNI_OBJECT_BOOLEAN_ARRAY) return object->u.boolean_array.length;
    if (object->kind == SSRNX_JNI_OBJECT_BYTE_ARRAY) return object->u.byte_array.length;
    if (object->kind == SSRNX_JNI_OBJECT_CHAR_ARRAY) return object->u.char_array.length;
    if (object->kind == SSRNX_JNI_OBJECT_SHORT_ARRAY) return object->u.short_array.length;
    if (object->kind == SSRNX_JNI_OBJECT_INT_ARRAY) return object->u.int_array.length;
    if (object->kind == SSRNX_JNI_OBJECT_LONG_ARRAY) return object->u.long_array.length;
    if (object->kind == SSRNX_JNI_OBJECT_FLOAT_ARRAY) return object->u.float_array.length;
    if (object->kind == SSRNX_JNI_OBJECT_DOUBLE_ARRAY) return object->u.double_array.length;
    if (object->kind == SSRNX_JNI_OBJECT_OBJECT_ARRAY) return object->u.object_array.length;
    return 0;
}

static void *jni_jni_new_object_array(jni_SsrnxJNIEnv *env_handle, int32_t length, void *clazz_ref, void *initial_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env) return NULL;
    if (length < 0) {
        jni_set_exception(env, "java/lang/NegativeArraySizeException", "NewObjectArray");
        return NULL;
    }

    jni_SsrnxJniClass *component = clazz_ref ? jni_class_from_ref(clazz_ref) : jni_resolve_class_lazy("java/lang/Object", 0u);
    if (!component) {
        jni_set_exception(env, "java/lang/NullPointerException", "NewObjectArray element class");
        return NULL;
    }
    jni_SsrnxJniObject *initial = jni_object_from_ref(initial_ref);
    if (initial && !jni_object_is_assignable_to_class(initial, component)) {
        jni_set_exception(env, "java/lang/ArrayStoreException", component->name);
        return NULL;
    }

    char descriptor[1024];
    const int dn = component->name[0] == '['
        ? snprintf(descriptor, sizeof(descriptor), "[%s", component->name)
        : snprintf(descriptor, sizeof(descriptor), "[L%s;", component->name);
    if (dn <= 0 || (size_t)dn >= sizeof(descriptor)) {
        jni_set_exception(env, "java/lang/OutOfMemoryError", "array descriptor too long");
        return NULL;
    }
    jni_SsrnxJniClass *array_class = jni_resolve_class_lazy(descriptor, 0u);
    if (!array_class) {
        jni_set_exception(env, "java/lang/OutOfMemoryError", "array class");
        return NULL;
    }

    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniObject *array = jni_alloc_object_locked(SSRNX_JNI_OBJECT_OBJECT_ARRAY, array_class);
    if (array && length) {
        array->u.object_array.values = calloc((size_t)length, sizeof(jni_SsrnxJniObject *));
        if (!array->u.object_array.values) {
            jni_g_objects = array->next;
            pthread_cond_destroy(&array->monitor_cond);
            pthread_mutex_destroy(&array->monitor_lock);
            free(array);
            array = NULL;
        }
    }
    if (array) {
        array->u.object_array.length = length;
        array->u.object_array.component_class = component;
        for (int32_t i = 0; i < length; ++i) array->u.object_array.values[i] = initial;
    }
    jni_SsrnxJniRef *ref = array ? jni_new_ref_locked(env, array, SSRNX_JNI_REF_LOCAL) : NULL;
    pthread_mutex_unlock(&jni_g_lock);
    if (!ref && !env->pending_exception)
        jni_set_exception(env, "java/lang/OutOfMemoryError", "NewObjectArray");
    return ref;
}

static bool jni_array_index_ok(jni_SsrnxJniEnvState *env, int32_t length, int32_t index) {
    if (index >= 0 && index < length) return true;
    jni_set_exception(env, "java/lang/ArrayIndexOutOfBoundsException", "array index");
    return false;
}

static void *jni_jni_get_object_array_element(jni_SsrnxJNIEnv *env_handle, void *array_ref, int32_t index) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (!env || !array || array->kind != SSRNX_JNI_OBJECT_OBJECT_ARRAY ||
        !jni_array_index_ok(env, array->u.object_array.length, index)) return NULL;
    jni_SsrnxJniObject *object = array->u.object_array.values[index];
    if (!object) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, object, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

static void jni_jni_set_object_array_element(jni_SsrnxJNIEnv *env_handle, void *array_ref, int32_t index, void *value_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (!env || !array || array->kind != SSRNX_JNI_OBJECT_OBJECT_ARRAY ||
        !jni_array_index_ok(env, array->u.object_array.length, index)) return;
    jni_SsrnxJniObject *value = jni_object_from_ref(value_ref);
    if (value && array->u.object_array.component_class &&
        !jni_object_is_assignable_to_class(value, array->u.object_array.component_class)) {
        jni_set_exception(env, "java/lang/ArrayStoreException",
                      array->u.object_array.component_class->name);
        return;
    }
    array->u.object_array.values[index] = value;
}

/* ---------- java.lang.reflect.Method / java.lang.reflect.Proxy ---------- */

#define SSRNX_PROXY_CLASS "java/lang/reflect/Proxy"
#define SSRNX_PROXY_HANDLER_FIELD "h"
#define SSRNX_PROXY_HANDLER_SIG "Ljava/lang/reflect/InvocationHandler;"
#define SSRNX_INVOCATION_HANDLER_INVOKE_SIG \
    "(Ljava/lang/Object;Ljava/lang/reflect/Method;[Ljava/lang/Object;)Ljava/lang/Object;"

static jni_SsrnxJniClass *jni_ensure_class(const char *name, const char *super_name) {
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *clazz = jni_find_class_locked(name);
    pthread_mutex_unlock(&jni_g_lock);
    if (clazz) return clazz;
    if (jni_register_class_internal(name, super_name) != SSRNX_OK) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    clazz = jni_find_class_locked(name);
    pthread_mutex_unlock(&jni_g_lock);
    return clazz;
}

/* One canonical reflection object per method ID. Java does not promise
 * reflection object identity, and caching keeps repeated proxy calls from
 * allocating a new Method every time. */
static jni_SsrnxJniObject *jni_reflected_method_object(jni_SsrnxJniMethod *method) {
    if (!method) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniObject *existing = method->reflected;
    pthread_mutex_unlock(&jni_g_lock);
    if (existing) return existing;
    const bool ctor = strcmp(method->name, "<init>") == 0;
    jni_SsrnxJniClass *clazz = jni_ensure_class(ctor ? "java/lang/reflect/Constructor"
                                             : "java/lang/reflect/Method",
                                        "java/lang/Object");
    jni_SsrnxJniObject *object = jni_new_instance_object(clazz);
    if (!object) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    if (method->reflected) {
        object = method->reflected;
    } else {
        object->reflected_method = method;
        method->reflected = object;
    }
    pthread_mutex_unlock(&jni_g_lock);
    return object;
}

static void *jni_jni_to_reflected_method(jni_SsrnxJNIEnv *env_handle, void *clazz_ref, void *method_id,
                                     uint8_t is_static) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniMethod *method = (jni_SsrnxJniMethod *)method_id;
    (void)clazz_ref;
    (void)is_static;
    if (!env || !method) return NULL;
    jni_SsrnxJniObject *object = jni_reflected_method_object(method);
    if (!object) {
        jni_set_exception(env, "java/lang/OutOfMemoryError", "ToReflectedMethod");
        return NULL;
    }
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, object, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

static void *jni_jni_from_reflected_method(jni_SsrnxJNIEnv *env_handle, void *method_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *object = jni_object_from_ref(method_ref);
    if (!env || !object) return NULL;
    if (object->reflected_method) return object->reflected_method;
    jni_set_exception(env, "java/lang/NoSuchMethodError", "reflected method has no JNI method ID");
    return NULL;
}

/* One canonical reflection object per field ID, mirroring the Method path
 * above.  Keeping this in the JNI object model is important: Java reflection
 * helpers and native JNI To/FromReflectedField must describe the same field. */
static jni_SsrnxJniObject *jni_reflected_field_object(jni_SsrnxJniField *field) {
    if (!field) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniObject *existing = field->reflected;
    pthread_mutex_unlock(&jni_g_lock);
    if (existing) return existing;
    jni_SsrnxJniClass *clazz = jni_ensure_class("java/lang/reflect/Field", "java/lang/Object");
    jni_SsrnxJniObject *object = jni_new_instance_object(clazz);
    if (!object) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    if (field->reflected) {
        object = field->reflected;
    } else {
        object->reflected_field = field;
        field->reflected = object;
    }
    pthread_mutex_unlock(&jni_g_lock);
    return object;
}

static void *jni_jni_to_reflected_field(jni_SsrnxJNIEnv *env_handle, void *clazz_ref, void *field_id,
                                    uint8_t is_static) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniField *field = (jni_SsrnxJniField *)field_id;
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    if (!env || !field) return NULL;
    if (clazz && field->owner && !jni_class_is_assignable_to(clazz, field->owner) &&
        !jni_class_is_assignable_to(field->owner, clazz)) {
        jni_set_exception(env, "java/lang/IllegalArgumentException", "field does not belong to class");
        return NULL;
    }
    if ((is_static != 0) != field->is_static) {
        jni_set_exception(env, "java/lang/IllegalArgumentException", "field staticness mismatch");
        return NULL;
    }
    jni_SsrnxJniObject *object = jni_reflected_field_object(field);
    if (!object) {
        jni_set_exception(env, "java/lang/OutOfMemoryError", "ToReflectedField");
        return NULL;
    }
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, object, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

static void *jni_jni_from_reflected_field(jni_SsrnxJNIEnv *env_handle, void *field_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *object = jni_object_from_ref(field_ref);
    if (!env || !object) return NULL;
    if (object->reflected_field) return object->reflected_field;
    jni_set_exception(env, "java/lang/NoSuchFieldError", "reflected field has no JNI field ID");
    return NULL;
}

/* Abstract interface method: dispatch on the receiver's runtime class, i.e.
 * the implementation a JVM would select for invokeinterface (JVMS 6.5). A
 * proxy receiver never reaches this callback; invoke_method routes it first. */
static SsrnxJniValue jni_interface_method_dispatch(void *env_handle, void *object_token,
                                               const SsrnxJniValue *args, size_t arg_count,
                                               void *user_data) {
    (void)arg_count;
    SsrnxJniValue zero = {0};
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniMethod *abstract_method = (jni_SsrnxJniMethod *)user_data;
    jni_SsrnxJniObject *receiver = (jni_SsrnxJniObject *)object_token;
    if (!env || !abstract_method || !receiver || !receiver->clazz) return zero;
    jni_SsrnxJniMethod *impl = jni_lookup_method(receiver->clazz, abstract_method->name,
                                         abstract_method->signature, false);
    if (!impl || impl == abstract_method || !impl->callback) {
        char detail[512];
        snprintf(detail, sizeof(detail), "%s.%s%s", receiver->clazz->name,
                 abstract_method->name, abstract_method->signature);
        jni_set_exception(env, "java/lang/AbstractMethodError", detail);
        return zero;
    }
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *self = jni_new_ref_locked(env, receiver, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    if (!self) return zero;
    SsrnxJniValue result = jni_invoke_method(env_handle, self, impl, args);
    jni_jni_delete_local_ref(env_handle, self);
    return result;
}

SsrnxResult ssrnx_jni_register_interface_method(const char *interface_name, const char *name,
                                                const char *signature) {
    if (!jni_g_initialized || !interface_name || !name || !signature) return SSRNX_ERR_ARGUMENT;
    SsrnxResult rc = ssrnx_jni_register_method(interface_name, name, signature, false,
                                               jni_interface_method_dispatch, NULL);
    if (rc != SSRNX_OK) return rc;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *clazz = jni_find_class_locked(interface_name);
    jni_SsrnxJniMethod *method = clazz ? jni_find_declared_method(clazz, name, signature, false) : NULL;
    if (clazz) clazz->is_interface = true;
    if (method && method->callback == jni_interface_method_dispatch) method->user_data = method;
    pthread_mutex_unlock(&jni_g_lock);
    return method ? SSRNX_OK : SSRNX_ERR_ARGUMENT;
}

typedef struct jni_SsrnxJniBoxSpec {
    char type;
    const char *class_name;
    const char *value_of_sig;
    const char *unbox_name;
    const char *unbox_sig;
} jni_SsrnxJniBoxSpec;

static const jni_SsrnxJniBoxSpec jni_k_box_specs[] = {
    {'Z', "java/lang/Boolean", "(Z)Ljava/lang/Boolean;", "booleanValue", "()Z"},
    {'B', "java/lang/Byte", "(B)Ljava/lang/Byte;", "byteValue", "()B"},
    {'C', "java/lang/Character", "(C)Ljava/lang/Character;", "charValue", "()C"},
    {'S', "java/lang/Short", "(S)Ljava/lang/Short;", "shortValue", "()S"},
    {'I', "java/lang/Integer", "(I)Ljava/lang/Integer;", "intValue", "()I"},
    {'J', "java/lang/Long", "(J)Ljava/lang/Long;", "longValue", "()J"},
    {'F', "java/lang/Float", "(F)Ljava/lang/Float;", "floatValue", "()F"},
    {'D', "java/lang/Double", "(D)Ljava/lang/Double;", "doubleValue", "()D"},
};

static const jni_SsrnxJniBoxSpec *jni_box_spec(char type) {
    for (size_t i = 0; i < sizeof(jni_k_box_specs) / sizeof(jni_k_box_specs[0]); ++i)
        if (jni_k_box_specs[i].type == type) return &jni_k_box_specs[i];
    return NULL;
}

/* Boxing conversion (JLS 5.1.7) through the modeled wrapper valueOf. */
static void *jni_box_primitive(void *env_handle, jni_SsrnxJniEnvState *env, char type, SsrnxJniValue value) {
    const jni_SsrnxJniBoxSpec *spec = jni_box_spec(type);
    jni_SsrnxJniClass *wrapper = spec ? jni_resolve_class_lazy(spec->class_name, 0u) : NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniMethod *value_of = wrapper ? jni_find_declared_method(wrapper, "valueOf", spec->value_of_sig, true)
                                       : NULL;
    pthread_mutex_unlock(&jni_g_lock);
    if (!value_of || !value_of->callback) {
        jni_set_exception(env, "java/lang/NoSuchMethodError", spec ? spec->value_of_sig : "valueOf");
        return NULL;
    }
    return value_of->callback(env_handle, NULL, &value, 1, value_of->user_data).l;
}

/* Unboxing conversion (JLS 5.1.8) of a proxy handler result. */
static bool jni_unbox_primitive(void *env_handle, jni_SsrnxJniEnvState *env, char type, void *boxed_ref,
                            SsrnxJniValue *out) {
    const jni_SsrnxJniBoxSpec *spec = jni_box_spec(type);
    jni_SsrnxJniObject *boxed = jni_object_from_ref(boxed_ref);
    if (!boxed) {
        jni_set_exception(env, "java/lang/NullPointerException",
                      "InvocationHandler returned null for a primitive return type");
        return false;
    }
    jni_SsrnxJniClass *wrapper = spec ? jni_resolve_class_lazy(spec->class_name, 0u) : NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    const bool matches = wrapper && boxed->clazz == wrapper;
    jni_SsrnxJniMethod *unbox = matches ? jni_find_declared_method(wrapper, spec->unbox_name, spec->unbox_sig, false)
                                    : NULL;
    pthread_mutex_unlock(&jni_g_lock);
    if (!matches) {
        jni_set_exception(env, "java/lang/ClassCastException",
                      spec ? spec->class_name : "InvocationHandler result");
        return false;
    }
    if (!unbox || !unbox->callback) {
        jni_set_exception(env, "java/lang/NoSuchMethodError", spec->unbox_name);
        return false;
    }
    *out = unbox->callback(env_handle, boxed, NULL, 0, unbox->user_data);
    return true;
}

static jni_SsrnxJniObject *jni_proxy_handler_object(jni_SsrnxJniObject *proxy) {
    if (!proxy || !proxy->clazz) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniField *field = jni_find_field(proxy->clazz, SSRNX_PROXY_HANDLER_FIELD,
                                      SSRNX_PROXY_HANDLER_SIG, false);
    SsrnxJniValue *slot = jni_instance_field_value(proxy, field);
    jni_SsrnxJniObject *handler = slot ? (jni_SsrnxJniObject *)slot->l : NULL;
    pthread_mutex_unlock(&jni_g_lock);
    return handler;
}

static bool jni_proxy_dispatches_method(jni_SsrnxJniClass *proxy, const jni_SsrnxJniMethod *method) {
    if (!proxy || !proxy->is_proxy || !method || method->is_static || !method->owner ||
        method->name[0] == '<') return false;
    if (strcmp(method->owner->name, "java/lang/Object") == 0) {
        return (strcmp(method->name, "hashCode") == 0 && strcmp(method->signature, "()I") == 0) ||
               (strcmp(method->name, "equals") == 0 &&
                strcmp(method->signature, "(Ljava/lang/Object;)Z") == 0) ||
               (strcmp(method->name, "toString") == 0 &&
                strcmp(method->signature, "()Ljava/lang/String;") == 0);
    }
    return method->owner->is_interface && jni_class_is_assignable_to(proxy, method->owner);
}

/* Proxy dispatch (java.lang.reflect.Proxy): the handler receives the proxy,
 * the interface Method, and the boxed arguments (null for a no-arg method);
 * its result is unboxed/returned according to the interface method type. */
static SsrnxJniValue jni_proxy_invoke(void *env_handle, void *proxy_ref, jni_SsrnxJniObject *proxy,
                                  jni_SsrnxJniMethod *method, const SsrnxJniValue *args) {
    SsrnxJniValue result = {0};
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env) return result;
    jni_SsrnxJniObject *handler = jni_proxy_handler_object(proxy);
    if (!handler) {
        jni_set_exception(env, "java/lang/NullPointerException", "proxy has no InvocationHandler");
        return result;
    }
    jni_SsrnxJniMethod *invoke = jni_lookup_method(handler->clazz, "invoke",
                                           SSRNX_INVOCATION_HANDLER_INVOKE_SIG, false);
    if (!invoke || !invoke->callback || invoke->callback == jni_interface_method_dispatch) {
        jni_set_exception(env, "java/lang/AbstractMethodError", "InvocationHandler.invoke");
        return result;
    }
    jni_SsrnxJniObject *reflected = jni_reflected_method_object(method);
    char types[SSRNX_JNI_MAX_PARAMETER_SLOTS];
    const int argc = jni_descriptor_args(method->signature, types, sizeof(types));
    if (!reflected || argc < 0) {
        jni_set_exception(env, "java/lang/OutOfMemoryError", "proxy method");
        return result;
    }

    void *array_ref = NULL;
    if (argc > 0) {
        array_ref = jni_jni_new_object_array((jni_SsrnxJNIEnv *)env_handle, argc, NULL, NULL);
        if (!array_ref) {
            jni_set_exception(env, "java/lang/OutOfMemoryError", "proxy arguments");
            return result;
        }
        for (int i = 0; i < argc; ++i) {
            if (types[i] == 'L') {
                jni_jni_set_object_array_element((jni_SsrnxJNIEnv *)env_handle, array_ref, i, args[i].l);
                continue;
            }
            void *boxed = jni_box_primitive(env_handle, env, types[i], args[i]);
            if (!boxed) {
                jni_jni_delete_local_ref((jni_SsrnxJNIEnv *)env_handle, array_ref);
                return result;
            }
            jni_jni_set_object_array_element((jni_SsrnxJNIEnv *)env_handle, array_ref, i, boxed);
            jni_jni_delete_local_ref((jni_SsrnxJNIEnv *)env_handle, boxed);
        }
    }

    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *method_ref = jni_new_ref_locked(env, reflected, SSRNX_JNI_REF_LOCAL);
    jni_SsrnxJniRef *handler_ref = jni_new_ref_locked(env, handler, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    if (method_ref && handler_ref) {
        if (ssrnx_diag_full_trace_enabled && ssrnx_diag_full_trace_enabled())
            SSRNX_LOGCI("JNI", "proxy invoke class=%s method=%s.%s%s handler=%s",
                        proxy->clazz->name, method->owner->name, method->name,
                        method->signature, handler->clazz ? handler->clazz->name : "?");
        const SsrnxJniValue call_args[3] = {{.l = proxy_ref}, {.l = method_ref}, {.l = array_ref}};
        SsrnxJniValue out = jni_invoke_method(env_handle, handler_ref, invoke, call_args);
        const char *ret = strchr(method->signature, ')');
        const char rt = ret && ret[1] ? ret[1] : 'V';
        if (env->pending_exception || rt == 'V') {
            if (out.l) jni_jni_delete_local_ref((jni_SsrnxJNIEnv *)env_handle, out.l);
        } else if (rt == 'L' || rt == '[') {
            result.l = out.l;
        } else {
            (void)jni_unbox_primitive(env_handle, env, rt, out.l, &result);
            if (out.l) jni_jni_delete_local_ref((jni_SsrnxJNIEnv *)env_handle, out.l);
        }
    } else {
        jni_set_exception(env, "java/lang/OutOfMemoryError", "proxy references");
    }
    if (method_ref) jni_jni_delete_local_ref((jni_SsrnxJNIEnv *)env_handle, method_ref);
    if (handler_ref) jni_jni_delete_local_ref((jni_SsrnxJNIEnv *)env_handle, handler_ref);
    if (array_ref) jni_jni_delete_local_ref((jni_SsrnxJNIEnv *)env_handle, array_ref);
    return result;
}

/* Proxy.getProxyClass semantics: one class per ordered interface list. The
 * class extends java.lang.reflect.Proxy and implements exactly the given
 * interfaces, so instance-of/check-cast/GetMethodID see the real hierarchy. */
static jni_SsrnxJniClass *jni_proxy_class_for(jni_SsrnxJniClass *const *interfaces, size_t count) {
    SSRNX_PERF_LOCK(&jni_g_proxy_lock);
    SSRNX_PERF_LOCK(&jni_g_lock);
    for (jni_SsrnxJniClass *c = jni_g_classes; c; c = c->next) {
        if (!c->is_proxy || c->proxy_interface_count != count) continue;
        bool same = true;
        for (size_t i = 0; i < count && same; ++i) same = c->proxy_interfaces[i] == interfaces[i];
        if (same) {
            pthread_mutex_unlock(&jni_g_lock);
            pthread_mutex_unlock(&jni_g_proxy_lock);
            return c;
        }
    }
    const unsigned serial = jni_g_proxy_class_serial++;
    pthread_mutex_unlock(&jni_g_lock);

    char name[32];
    snprintf(name, sizeof(name), "$Proxy%u", serial);
    jni_SsrnxJniClass **copy = calloc(count ? count : 1u, sizeof(*copy));
    jni_SsrnxJniClass *clazz = NULL;
    if (copy && jni_ensure_class(SSRNX_PROXY_CLASS, "java/lang/Object") &&
        ssrnx_jni_register_field(SSRNX_PROXY_CLASS, SSRNX_PROXY_HANDLER_FIELD,
                                 SSRNX_PROXY_HANDLER_SIG, false, (SsrnxJniValue){0}) == SSRNX_OK &&
        jni_register_class_internal(name, SSRNX_PROXY_CLASS) == SSRNX_OK) {
        bool linked = true;
        for (size_t i = 0; i < count && linked; ++i)
            linked = ssrnx_jni_register_interface(name, interfaces[i]->name) == SSRNX_OK;
        if (linked) {
            memcpy(copy, interfaces, count * sizeof(*copy));
            SSRNX_PERF_LOCK(&jni_g_lock);
            clazz = jni_find_class_locked(name);
            if (clazz) {
                clazz->proxy_interfaces = copy;
                clazz->proxy_interface_count = count;
                clazz->is_proxy = true;
                copy = NULL;
            }
            pthread_mutex_unlock(&jni_g_lock);
        }
    }
    free(copy);
    pthread_mutex_unlock(&jni_g_proxy_lock);
    if (clazz)
        SSRNX_LOGCI("JNI", "proxy class defined name=%s interfaces=%zu first=%s", name, count,
                    count ? interfaces[0]->name : "-");
    return clazz;
}

SsrnxResult ssrnx_jni_proxy_new_instance_host(void *env_handle, void *interfaces_ref,
                                              void *handler_ref, void **out_proxy_ref) {
    if (out_proxy_ref) *out_proxy_ref = NULL;
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || !out_proxy_ref) return SSRNX_ERR_ARGUMENT;
    jni_SsrnxJniObject *handler = jni_object_from_ref(handler_ref);
    jni_SsrnxJniObject *array = jni_object_from_ref(interfaces_ref);
    if (!handler || !array) {
        jni_set_exception(env, "java/lang/NullPointerException",
                      handler ? "interfaces" : "InvocationHandler");
        return SSRNX_OK;
    }
    if (array->kind != SSRNX_JNI_OBJECT_OBJECT_ARRAY || array->u.object_array.length > 65535) {
        jni_set_exception(env, "java/lang/IllegalArgumentException", "interface limit exceeded");
        return SSRNX_OK;
    }
    const size_t count = (size_t)array->u.object_array.length;
    jni_SsrnxJniClass **interfaces = calloc(count ? count : 1u, sizeof(*interfaces));
    if (!interfaces) {
        jni_set_exception(env, "java/lang/OutOfMemoryError", "Proxy.newProxyInstance");
        return SSRNX_OK;
    }
    const char *error_class = NULL;
    char error[320] = {0};
    for (size_t i = 0; i < count && !error_class; ++i) {
        jni_SsrnxJniObject *element = array->u.object_array.values[i];
        jni_SsrnxJniClass *iface = element && element->kind == SSRNX_JNI_OBJECT_CLASS
                               ? element->u.class_data.descriptor : NULL;
        if (!element) {
            error_class = "java/lang/NullPointerException";
            snprintf(error, sizeof(error), "interfaces[%zu]", i);
        } else if (!iface || iface->is_proxy || iface->name[0] == '[' ||
                   strcmp(iface->name, "java/lang/Object") == 0) {
            error_class = "java/lang/IllegalArgumentException";
            snprintf(error, sizeof(error), "%s is not an interface", iface ? iface->name : "?");
        } else {
            for (size_t j = 0; j < i; ++j) {
                if (interfaces[j] == iface) {
                    error_class = "java/lang/IllegalArgumentException";
                    snprintf(error, sizeof(error), "repeated interface: %s", iface->name);
                }
            }
        }
        interfaces[i] = iface;
    }
    jni_SsrnxJniClass *clazz = error_class ? NULL : jni_proxy_class_for(interfaces, count);
    free(interfaces);
    if (error_class) {
        jni_set_exception(env, error_class, error);
        return SSRNX_OK;
    }
    jni_SsrnxJniObject *object = clazz ? jni_new_instance_object(clazz) : NULL;
    if (!object) {
        jni_set_exception(env, "java/lang/OutOfMemoryError", "Proxy.newProxyInstance");
        return SSRNX_OK;
    }
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniField *field = jni_find_field(clazz, SSRNX_PROXY_HANDLER_FIELD, SSRNX_PROXY_HANDLER_SIG, false);
    SsrnxJniValue *slot = jni_instance_field_value(object, field);
    if (slot) slot->l = handler;
    jni_SsrnxJniRef *ref = slot ? jni_new_ref_locked(env, object, SSRNX_JNI_REF_LOCAL) : NULL;
    pthread_mutex_unlock(&jni_g_lock);
    if (!ref) {
        jni_set_exception(env, "java/lang/OutOfMemoryError", "Proxy.newProxyInstance");
        return SSRNX_OK;
    }
    *out_proxy_ref = ref;
    return SSRNX_OK;
}

void *ssrnx_jni_proxy_get_handler_host(void *env_handle, void *proxy_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *proxy = jni_object_from_ref(proxy_ref);
    if (!env || !proxy || !proxy->clazz || !proxy->clazz->is_proxy) return NULL;
    jni_SsrnxJniObject *handler = jni_proxy_handler_object(proxy);
    if (!handler) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, handler, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

bool ssrnx_jni_is_proxy_class_host(void *class_ref) {
    jni_SsrnxJniClass *clazz = jni_class_from_ref(class_ref);
    return clazz && clazz->is_proxy;
}

/* A DEX invoke on a proxy reaches the host with the proxy's runtime class.
 * Resolve the interface method from the proxy's interfaces in declaration
 * order; a method the model has not declared yet is registered on the first
 * interface that the lazy resolver cannot place elsewhere. */
static jni_SsrnxJniMethod *jni_proxy_interface_method(void *receiver_ref, const char *name,
                                              const char *signature) {
    jni_SsrnxJniObject *receiver = jni_object_from_ref(receiver_ref);
    jni_SsrnxJniClass *proxy = receiver ? receiver->clazz : NULL;
    if (!proxy || !proxy->is_proxy || !proxy->proxy_interface_count || name[0] == '<') return NULL;
    for (size_t i = 0; i < proxy->proxy_interface_count; ++i) {
        jni_SsrnxJniMethod *method = jni_lookup_method(proxy->proxy_interfaces[i], name, signature, false);
        if (method && jni_proxy_dispatches_method(proxy, method)) return method;
    }
    jni_SsrnxJniClass *first = proxy->proxy_interfaces[0];
    if (ssrnx_jni_register_interface_method(first->name, name, signature) != SSRNX_OK) return NULL;
    SSRNX_LOGCI("JNI", "proxy interface method declared class=%s method=%s%s proxy=%s",
                first->name, name, signature, proxy->name);
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniMethod *method = jni_find_declared_method(first, name, signature, false);
    pthread_mutex_unlock(&jni_g_lock);
    return method;
}

bool ssrnx_jni_reflected_method_info_host(void *method_ref, char *out_owner, size_t owner_cap,
                                          char *out_name, size_t name_cap,
                                          char *out_signature, size_t signature_cap,
                                          bool *out_is_static, bool *out_owner_is_interface) {
    jni_SsrnxJniObject *object = jni_object_from_ref(method_ref);
    jni_SsrnxJniMethod *method = object ? object->reflected_method : NULL;
    if (!method || !method->owner) return false;
    if (out_owner && owner_cap && snprintf(out_owner, owner_cap, "%s", method->owner->name) >= (int)owner_cap)
        return false;
    if (out_name && name_cap && snprintf(out_name, name_cap, "%s", method->name) >= (int)name_cap)
        return false;
    if (out_signature && signature_cap &&
        snprintf(out_signature, signature_cap, "%s", method->signature) >= (int)signature_cap)
        return false;
    if (out_is_static) *out_is_static = method->is_static;
    if (out_owner_is_interface) *out_owner_is_interface = method->owner->is_interface;
    return true;
}

extern bool ssrnx_dex_method_access_flags(const char *class_name, const char *method_name,
                                          const char *signature, uint32_t *out_method_flags,
                                          uint32_t *out_class_flags) __attribute__((weak));
extern bool ssrnx_dex_class_access_flags(const char *, uint32_t *) __attribute__((weak));

#define SSRNX_ACC_PUBLIC    0x0001u
#define SSRNX_ACC_STATIC    0x0008u
#define SSRNX_ACC_INTERFACE 0x0200u
#define SSRNX_ACC_ABSTRACT  0x0400u

/* Declared Java access flags of a reflected method (JVMS 4.6 / DEX
 * access_flags; the low 16 bits share the same encoding) and whether its
 * declaring type is an interface. Package DEX methods report their real
 * metadata. Framework models are public; their interface instance methods are
 * the platform's abstract interface methods. */
bool ssrnx_jni_reflected_method_modifiers_host(void *method_ref, uint32_t *out_access_flags,
                                               bool *out_declared_in_interface) {
    jni_SsrnxJniObject *object = jni_object_from_ref(method_ref);
    jni_SsrnxJniMethod *method = object ? object->reflected_method : NULL;
    if (!method || !method->owner) return false;
    uint32_t method_flags = 0, class_flags = 0;
    if (strcmp(method->name, "<clinit>") != 0 && ssrnx_dex_method_access_flags &&
        ssrnx_dex_method_access_flags(method->owner->name, method->name, method->signature,
                                      &method_flags, &class_flags)) {
        if (out_access_flags) *out_access_flags = method_flags & 0xffffu;
        if (out_declared_in_interface)
            *out_declared_in_interface = (class_flags & SSRNX_ACC_INTERFACE) != 0;
        return true;
    }
    uint32_t flags = SSRNX_ACC_PUBLIC;
    if (method->is_static) flags |= SSRNX_ACC_STATIC;
    else if (method->owner->is_interface && method->name[0] != '<') flags |= SSRNX_ACC_ABSTRACT;
    if (out_access_flags) *out_access_flags = flags;
    if (out_declared_in_interface) *out_declared_in_interface = method->owner->is_interface;
    return true;
}

/* Registers every member the class and (unless declared_only) its
 * superclasses declare before a reflection enumeration. Each class is
 * materialized once; a failure is logged and retried on the next query.
 * Without a package materializer every class is a framework model. */
static void jni_materialize_reflected_classes(void *env_handle, jni_SsrnxJniClass *clazz,
                                          bool declared_only) {
    jni_SsrnxJniClass *pending[64];
    size_t count = 0;
    SSRNX_PERF_LOCK(&jni_g_lock);
    SsrnxJniClassMemberMaterializer materializer = jni_g_member_materializer;
    void *user = jni_g_member_materializer_user;
    for (jni_SsrnxJniClass *c = clazz; c && count < 64u; c = declared_only ? NULL : c->super) {
        if (c->members_materialized || c->primitive || c->is_proxy) continue;
        c->members_materialized = true;
        pending[count++] = c;
    }
    pthread_mutex_unlock(&jni_g_lock);
    for (size_t i = 0; i < count; ++i) {
        const SsrnxResult rc = materializer ? materializer(env_handle, pending[i]->name, user)
                                            : ssrnx_jni_declare_modelled_interface_methods_host(pending[i]->name);
        if (rc == SSRNX_OK) continue;
        SSRNX_LOGCW("JNI", "declared member materialization failed class=%s rc=%d",
                    pending[i]->name, (int)rc);
        SSRNX_PERF_LOCK(&jni_g_lock);
        pending[i]->members_materialized = false;
        pthread_mutex_unlock(&jni_g_lock);
    }
}

/* Declares the API-30 platform members of the class and (unless
 * declared_only) its superclasses before a reflection query, once per class.
 * Members a model or package class already provides are left alone. */
static void jni_materialize_platform_members(void *env_handle, jni_SsrnxJniClass *clazz, bool declared_only) {
    if (!ssrnx_api_stub_declare_members) return;
    jni_SsrnxJniClass *pending[64];
    size_t count = 0;
    SSRNX_PERF_LOCK(&jni_g_lock);
    for (jni_SsrnxJniClass *c = clazz; c && count < 64u; c = declared_only ? NULL : c->super) {
        if (c->platform_members_declared || c->primitive || c->is_proxy || c->name[0] == '[') continue;
        c->platform_members_declared = true;
        pending[count++] = c;
    }
    pthread_mutex_unlock(&jni_g_lock);
    for (size_t i = 0; i < count; ++i) (void)ssrnx_api_stub_declare_members(env_handle, pending[i]->name);
}

void *ssrnx_jni_class_get_superclass_host(void *env_handle, void *class_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(class_ref);
    if (!env || !clazz || clazz->primitive || !clazz->super ||
        ssrnx_jni_class_is_interface_host(class_ref)) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, clazz->super->class_object, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

void *ssrnx_jni_class_get_reflected_methods_host(void *env_handle, void *class_ref,
                                                  bool declared_only, bool constructors_only,
                                                  bool public_only) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(class_ref);
    if (!env || !clazz) return NULL;
    jni_materialize_reflected_classes(env_handle, clazz, declared_only);
    jni_materialize_platform_members(env_handle, clazz, declared_only);
    jni_SsrnxJniMethod **methods = NULL;
    size_t count = 0, capacity = 0;
    SSRNX_PERF_LOCK(&jni_g_lock);
    for (jni_SsrnxJniClass *c = clazz; c; c = declared_only ? NULL : c->super) {
        for (jni_SsrnxJniMethod *m = c->methods; m; m = m->next) {
            const bool constructor = strcmp(m->name, "<init>") == 0;
            if (constructor != constructors_only || strcmp(m->name, "<clinit>") == 0) continue;
            /* getMethods(): a subclass declaration hides the one it overrides. */
            bool hidden = false;
            for (size_t i = 0; i < count && !hidden && c != clazz; ++i)
                hidden = methods[i]->is_static == m->is_static && strcmp(methods[i]->name, m->name) == 0 &&
                         strcmp(methods[i]->signature, m->signature) == 0;
            if (hidden) continue;
            if (public_only) {
                uint32_t flags = SSRNX_ACC_PUBLIC;
                if (ssrnx_dex_method_access_flags)
                    (void)ssrnx_dex_method_access_flags(m->owner->name, m->name, m->signature,
                                                        &flags, NULL);
                if ((flags & SSRNX_ACC_PUBLIC) == 0) continue;
            }
            if (count == capacity) {
                const size_t next = capacity ? capacity * 2u : 8u;
                jni_SsrnxJniMethod **grown = realloc(methods, next * sizeof(*grown));
                if (!grown) { free(methods); pthread_mutex_unlock(&jni_g_lock); return NULL; }
                methods = grown; capacity = next;
            }
            methods[count++] = m;
        }
    }
    pthread_mutex_unlock(&jni_g_lock);

    void *array = ssrnx_jni_new_local_object_array(env_handle, (int32_t)count);
    if (!array) { free(methods); return NULL; }
    for (size_t i = 0; i < count; ++i) {
        jni_SsrnxJniObject *object = jni_reflected_method_object(methods[i]);
        if (!object) continue;
        SSRNX_PERF_LOCK(&jni_g_lock);
        jni_SsrnxJniRef *ref = jni_new_ref_locked(env, object, SSRNX_JNI_REF_LOCAL);
        pthread_mutex_unlock(&jni_g_lock);
        if (ref) {
            (void)ssrnx_jni_set_object_array_element_host(env_handle, array, (int32_t)i, ref);
            jni_jni_delete_local_ref((jni_SsrnxJNIEnv *)env_handle, ref);
        }
    }
    free(methods);
    return array;
}

/* One reflected Method/Constructor by name and parameter descriptor "(...)"
 * (Class.getDeclaredMethod/getMethod/getDeclaredConstructor/getConstructor).
 * The most derived declaration wins, as for Class.getMethod's superclass walk.
 * NULL when no such member exists. */
void *ssrnx_jni_class_get_reflected_method_host(void *env_handle, void *class_ref,
                                                 const char *name, const char *params,
                                                 bool declared_only, bool public_only) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(class_ref);
    if (!env || !clazz || !name || !params || params[0] != '(' || strcmp(name, "<clinit>") == 0) return NULL;
    const size_t params_len = strlen(params);
    if (params[params_len - 1] != ')') return NULL;
    jni_materialize_reflected_classes(env_handle, clazz, declared_only);
    jni_materialize_platform_members(env_handle, clazz, declared_only);
    jni_SsrnxJniMethod *found = NULL;
    const bool constructor = strcmp(name, "<init>") == 0;
    SSRNX_PERF_LOCK(&jni_g_lock);
    for (jni_SsrnxJniClass *c = clazz; c && !found; c = declared_only || constructor ? NULL : c->super) {
        for (jni_SsrnxJniMethod *m = c->methods; m; m = m->next) {
            if (strcmp(m->name, name) != 0 || strncmp(m->signature, params, params_len) != 0) continue;
            if (public_only) {
                uint32_t flags = SSRNX_ACC_PUBLIC;
                if (ssrnx_dex_method_access_flags)
                    (void)ssrnx_dex_method_access_flags(m->owner->name, m->name, m->signature, &flags, NULL);
                if ((flags & SSRNX_ACC_PUBLIC) == 0) continue;
            }
            found = m;
            break;
        }
    }
    pthread_mutex_unlock(&jni_g_lock);
    jni_SsrnxJniObject *object = found ? jni_reflected_method_object(found) : NULL;
    if (!object) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, object, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

/* Whether the class declaring a reflected constructor can be instantiated:
 * not an interface and, for package classes, not ACC_ABSTRACT
 * (Constructor.newInstance/Class.newInstance throw InstantiationException). */
bool ssrnx_jni_reflected_constructor_instantiable_host(void *method_ref) {
    jni_SsrnxJniObject *object = jni_object_from_ref(method_ref);
    jni_SsrnxJniMethod *method = object ? object->reflected_method : NULL;
    if (!method || !method->owner || method->owner->is_interface || method->owner->primitive ||
        method->owner->name[0] == '[') return false;
    uint32_t method_flags = 0, class_flags = 0;
    if (ssrnx_dex_method_access_flags &&
        ssrnx_dex_method_access_flags(method->owner->name, method->name, method->signature,
                                      &method_flags, &class_flags))
        return (class_flags & (SSRNX_ACC_ABSTRACT | SSRNX_ACC_INTERFACE)) == 0;
    return true;
}

/* Return a reflected Field by Java name.  getDeclaredField stops at the
 * supplied class; getField walks the superclass chain.  SSRNX's field model
 * is populated from framework declarations and lazily materialized DEX
 * fields, so both JNI and Java reflection share the same identities. */
void *ssrnx_jni_class_get_reflected_field_host(void *env_handle, void *class_ref,
                                                const char *name, bool declared_only) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(class_ref);
    if (!env || !clazz || !name) return NULL;
    jni_materialize_reflected_classes(env_handle, clazz, declared_only);
    jni_materialize_platform_members(env_handle, clazz, declared_only);
    jni_SsrnxJniField *found = NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    for (jni_SsrnxJniClass *c = clazz; c && !found; c = declared_only ? NULL : c->super) {
        for (jni_SsrnxJniField *f = c->fields; f; f = f->next) {
            if (strcmp(f->name, name) == 0) { found = f; break; }
        }
    }
    pthread_mutex_unlock(&jni_g_lock);
    if (!found) return NULL;
    jni_SsrnxJniObject *object = jni_reflected_field_object(found);
    if (!object) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, object, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

void *ssrnx_jni_class_get_reflected_fields_host(void *env_handle, void *class_ref,
                                                 bool declared_only) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(class_ref);
    if (!env || !clazz) return NULL;
    jni_materialize_reflected_classes(env_handle, clazz, declared_only);
    jni_materialize_platform_members(env_handle, clazz, declared_only);
    jni_SsrnxJniField **fields = NULL;
    size_t count = 0, capacity = 0;
    SSRNX_PERF_LOCK(&jni_g_lock);
    for (jni_SsrnxJniClass *c = clazz; c; c = declared_only ? NULL : c->super) {
        for (jni_SsrnxJniField *f = c->fields; f; f = f->next) {
            if (count == capacity) {
                size_t next = capacity ? capacity * 2u : 8u;
                jni_SsrnxJniField **grown = realloc(fields, next * sizeof(*grown));
                if (!grown) { free(fields); pthread_mutex_unlock(&jni_g_lock); return NULL; }
                fields = grown; capacity = next;
            }
            fields[count++] = f;
        }
    }
    pthread_mutex_unlock(&jni_g_lock);

    void *array = ssrnx_jni_new_local_object_array(env_handle, (int32_t)count);
    if (!array) { free(fields); return NULL; }
    for (size_t i = 0; i < count; ++i) {
        jni_SsrnxJniObject *object = jni_reflected_field_object(fields[i]);
        if (!object) continue;
        SSRNX_PERF_LOCK(&jni_g_lock);
        jni_SsrnxJniRef *ref = jni_new_ref_locked(env, object, SSRNX_JNI_REF_LOCAL);
        pthread_mutex_unlock(&jni_g_lock);
        if (ref) {
            (void)ssrnx_jni_set_object_array_element_host(env_handle, array, (int32_t)i, ref);
            jni_jni_delete_local_ref((jni_SsrnxJNIEnv *)env_handle, ref);
        }
    }
    free(fields);
    return array;
}

bool ssrnx_jni_reflected_field_info_host(void *field_ref, char *out_owner, size_t owner_cap,
                                         char *out_name, size_t name_cap,
                                         char *out_signature, size_t signature_cap,
                                         bool *out_is_static) {
    jni_SsrnxJniObject *object = jni_object_from_ref(field_ref);
    jni_SsrnxJniField *field = object ? object->reflected_field : NULL;
    if (!field || !field->owner) return false;
    if (out_owner && owner_cap && snprintf(out_owner, owner_cap, "%s", field->owner->name) >= (int)owner_cap)
        return false;
    if (out_name && name_cap && snprintf(out_name, name_cap, "%s", field->name) >= (int)name_cap)
        return false;
    if (out_signature && signature_cap &&
        snprintf(out_signature, signature_cap, "%s", field->signature) >= (int)signature_cap)
        return false;
    if (out_is_static) *out_is_static = field->is_static;
    return true;
}

/* ---- sun.misc.Unsafe storage -------------------------------------------- */

/* Field offsets are opaque handles (field index * 8) but stay in ART's range:
 * past the 8-byte object header and well below 2^20. protobuf-lite packs
 * objectFieldOffset() results into 20 bits of its schema table
 * (MessageSchema OFFSET_MASK), so larger values are silently truncated. */
#define UNSAFE_FIELD_OFFSET_BASE INT64_C(8)
#define UNSAFE_FIELD_CAP 8192u
static jni_SsrnxJniField *jni_g_unsafe_fields[UNSAFE_FIELD_CAP];
static size_t jni_g_unsafe_field_count;
static pthread_mutex_t jni_g_unsafe_lock = PTHREAD_MUTEX_INITIALIZER;

bool ssrnx_jni_unsafe_field_offset_host(void *field_ref, bool want_static, int64_t *out_offset) {
    jni_SsrnxJniObject *object = jni_object_from_ref(field_ref);
    jni_SsrnxJniField *field = object ? object->reflected_field : NULL;
    if (!field || field->is_static != want_static || !out_offset) return false;
    pthread_mutex_lock(&jni_g_unsafe_lock);
    size_t index = 0;
    const size_t count = jni_g_unsafe_field_count;
    while (index < count && jni_g_unsafe_fields[index] != field) ++index;
    bool ok = index < count;
    if (!ok && count < UNSAFE_FIELD_CAP) {
        jni_g_unsafe_fields[count] = field;
        __atomic_store_n(&jni_g_unsafe_field_count, count + 1u, __ATOMIC_RELEASE);
        ok = true;
    }
    pthread_mutex_unlock(&jni_g_unsafe_lock);
    if (ok) *out_offset = UNSAFE_FIELD_OFFSET_BASE + (int64_t)index * 8;
    return ok;
}

static jni_SsrnxJniField *jni_unsafe_field_at(int64_t offset) {
    if (offset < UNSAFE_FIELD_OFFSET_BASE || ((offset - UNSAFE_FIELD_OFFSET_BASE) & 7) != 0) return NULL;
    const uint64_t index = (uint64_t)(offset - UNSAFE_FIELD_OFFSET_BASE) / 8u;
    return index < __atomic_load_n(&jni_g_unsafe_field_count, __ATOMIC_ACQUIRE) ? jni_g_unsafe_fields[index] : NULL;
}

/* ART mirror::Array: 8-byte object header, 4-byte length, data aligned to the
 * component size; references are 32-bit (compressed) slots. */
static bool jni_unsafe_array_kind_layout(char kind, int32_t *base, int32_t *scale) {
    switch (kind) {
    case 'Z': case 'B': *scale = 1; break;
    case 'C': case 'S': *scale = 2; break;
    case 'I': case 'F': case 'L': case '[': *scale = 4; break;
    case 'J': case 'D': *scale = 8; break;
    default: return false;
    }
    *base = *scale == 8 ? 16 : 12;
    return true;
}

bool ssrnx_jni_unsafe_array_layout_host(void *class_ref, int32_t *out_base, int32_t *out_scale) {
    jni_SsrnxJniObject *object = jni_object_from_ref(class_ref);
    jni_SsrnxJniClass *clazz = object && object->kind == SSRNX_JNI_OBJECT_CLASS ? object->u.class_data.descriptor : NULL;
    if (!clazz || !clazz->name || clazz->name[0] != '[' || !out_base || !out_scale) return false;
    return jni_unsafe_array_kind_layout(clazz->name[1], out_base, out_scale);
}

static bool jni_unsafe_array_slot(void *values, int32_t length, char kind, int64_t offset, SsrnxUnsafeSlot *out) {
    int32_t base = 0, scale = 0;
    if (!values || !jni_unsafe_array_kind_layout(kind, &base, &scale)) return false;
    if (offset < base || ((offset - base) % scale) != 0) return false;
    const int64_t index = (offset - base) / scale;
    if (index >= length) return false;
    /* Reference arrays store host object pointers, not 32-bit slots. */
    const size_t stride = kind == 'L' ? sizeof(void *) : (size_t)scale;
    out->address = (uint8_t *)values + (size_t)index * stride;
    out->kind = kind;
    return true;
}

bool ssrnx_jni_unsafe_slot_host(void *object_ref, int64_t offset, SsrnxUnsafeSlot *out) {
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!object || !out) return false;
    switch (object->kind) {
    case SSRNX_JNI_OBJECT_BOOLEAN_ARRAY:
        return jni_unsafe_array_slot(object->u.boolean_array.values, object->u.boolean_array.length, 'Z', offset, out);
    case SSRNX_JNI_OBJECT_BYTE_ARRAY:
        return jni_unsafe_array_slot(object->u.byte_array.values, object->u.byte_array.length, 'B', offset, out);
    case SSRNX_JNI_OBJECT_CHAR_ARRAY:
        return jni_unsafe_array_slot(object->u.char_array.values, object->u.char_array.length, 'C', offset, out);
    case SSRNX_JNI_OBJECT_SHORT_ARRAY:
        return jni_unsafe_array_slot(object->u.short_array.values, object->u.short_array.length, 'S', offset, out);
    case SSRNX_JNI_OBJECT_INT_ARRAY:
        return jni_unsafe_array_slot(object->u.int_array.values, object->u.int_array.length, 'I', offset, out);
    case SSRNX_JNI_OBJECT_LONG_ARRAY:
        return jni_unsafe_array_slot(object->u.long_array.values, object->u.long_array.length, 'J', offset, out);
    case SSRNX_JNI_OBJECT_FLOAT_ARRAY:
        return jni_unsafe_array_slot(object->u.float_array.values, object->u.float_array.length, 'F', offset, out);
    case SSRNX_JNI_OBJECT_DOUBLE_ARRAY:
        return jni_unsafe_array_slot(object->u.double_array.values, object->u.double_array.length, 'D', offset, out);
    case SSRNX_JNI_OBJECT_OBJECT_ARRAY:
        return jni_unsafe_array_slot(object->u.object_array.values, object->u.object_array.length, 'L', offset, out);
    case SSRNX_JNI_OBJECT_CLASS: {
        /* staticFieldBase() returns the declaring Class. */
        jni_SsrnxJniField *field = jni_unsafe_field_at(offset);
        if (!field || !field->is_static || field->owner != object->u.class_data.descriptor) return false;
        out->address = &field->static_value;
        out->kind = field->signature[0] == '[' ? 'L' : field->signature[0];
        return true;
    }
    case SSRNX_JNI_OBJECT_INSTANCE: {
        jni_SsrnxJniField *field = jni_unsafe_field_at(offset);
        SsrnxJniValue *value = field && !field->is_static ? jni_instance_field_value(object, field) : NULL;
        if (!value) return false;
        out->address = value;
        out->kind = field->signature[0] == '[' ? 'L' : field->signature[0];
        return true;
    }
    default:
        return false;
    }
}

void *ssrnx_jni_class_ref_for_descriptor_host(void *env_handle, const char *signature) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || !signature || !signature[0]) return NULL;
    char name[512] = {0};
    bool primitive = false;
    if (signature[0] == 'L') {
        const char *end = strchr(signature, ';');
        if (!end || end == signature + 1 || (size_t)(end - signature - 1) >= sizeof(name)) return NULL;
        memcpy(name, signature + 1, (size_t)(end - signature - 1));
        name[end - signature - 1] = '\0';
    } else if (signature[0] == '[') {
        if (snprintf(name, sizeof(name), "%s", signature) >= (int)sizeof(name)) return NULL;
    } else {
        const char *primitive_name = NULL;
        switch (signature[0]) {
            case 'Z': primitive_name = "boolean"; break;
            case 'B': primitive_name = "byte"; break;
            case 'C': primitive_name = "char"; break;
            case 'S': primitive_name = "short"; break;
            case 'I': primitive_name = "int"; break;
            case 'J': primitive_name = "long"; break;
            case 'F': primitive_name = "float"; break;
            case 'D': primitive_name = "double"; break;
            case 'V': primitive_name = "void"; break;
            default: return NULL;
        }
        snprintf(name, sizeof(name), "%s", primitive_name);
        primitive = true;
    }

    /* A reference type names a real class: resolve it (package DEX, framework
     * model or array) so its superclass chain is the declared one. Registering
     * it here as an Object subclass would permanently cut the hierarchy, and
     * inherited fields/methods would no longer be found through it. Only a
     * type nothing defines gets a provisional Object-rooted placeholder, which
     * later package metadata may still reconcile. */
    jni_SsrnxJniClass *clazz = primitive ? jni_ensure_class(name, "java/lang/Object") : jni_resolve_class_lazy(name, 0u);
    if (!clazz && !primitive &&
        jni_register_class_internal_ex_loader(name, "java/lang/Object", true, SSRNX_JNI_LOADER_APPLICATION) == SSRNX_OK) {
        SSRNX_PERF_LOCK(&jni_g_lock);
        clazz = jni_find_class_locked(name);
        pthread_mutex_unlock(&jni_g_lock);
    }
    if (!clazz) return NULL;
    if (primitive) {
        SSRNX_PERF_LOCK(&jni_g_lock);
        clazz->primitive = true;
        pthread_mutex_unlock(&jni_g_lock);
    }
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, clazz->class_object, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

static bool jni_array_region_ok(jni_SsrnxJniEnvState *env, int32_t length, int32_t start, int32_t count) {
    if (start >= 0 && count >= 0 && start <= length && count <= length - start) return true;
    jni_set_exception(env, "java/lang/ArrayIndexOutOfBoundsException", "array region");
    return false;
}

/* Observe the values actually copied to native code, after region validation. */
static void jni_jni_int_array_read_diagnostic(void *array, int32_t length, int32_t start,
                                          int32_t count, const int32_t *values, void *caller) {
    static _Thread_local unsigned calls;
    if (calls >= 32u) return;
    const unsigned sequence = ++calls;
    if (count > 0 && sequence <= 8u) jni_g_jni_array_state_calls = 2u;
    const int32_t captured = count < 96 ? count : 96;
    char site[192] = "unmapped";
    if (ssrnx_compat_describe_guest_address)
        (void)ssrnx_compat_describe_guest_address((uintptr_t)caller, site, sizeof(site));
    for (int32_t offset = 0; offset < captured || (count == 0 && offset == 0); offset += 12) {
        const int32_t chunk = captured - offset < 12 ? captured - offset : 12;
        char text[160];
        size_t used = 0;
        text[0] = '\0';
        for (int32_t i = 0; i < chunk; ++i)
            used += (size_t)snprintf(text + used, sizeof(text) - used, "%s%d",
                                    i ? "," : "", (int)values[offset + i]);
        SSRNX_LOGCI("JNIARRAYREAD", "array=%p pthread=%p sequence=%u length=%d start=%d count=%d captured=%d offset=%d values=[%s] caller=%p site=%s",
            array, (void *)(uintptr_t)pthread_self(), sequence, (int)length, (int)start,
            (int)count, (int)captured, (int)offset, text, caller, site);
    }
}

#define DEFINE_PRIMITIVE_ARRAY_JNI(NAME, CTYPE, KIND, MEMBER, DESCRIPTOR) \
static void *jni_new_##NAME##_array(jni_SsrnxJNIEnv *env_handle, int32_t length) { \
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle); \
    if (!env) return NULL; \
    if (length < 0) { \
        jni_set_exception(env, "java/lang/NegativeArraySizeException", #NAME " array"); \
        return NULL; \
    } \
    jni_SsrnxJniClass *clazz = jni_resolve_class_lazy(DESCRIPTOR, 0u); \
    if (!clazz) { \
        jni_set_exception(env, "java/lang/OutOfMemoryError", #NAME " array class"); \
        return NULL; \
    } \
    pthread_mutex_lock(&jni_g_lock); \
    jni_SsrnxJniObject *array = jni_alloc_object_locked(KIND, clazz); \
    if (array) { \
        const size_t count = length > 0 ? (size_t)length : 1u; \
        array->u.MEMBER.values = calloc(count, sizeof(CTYPE)); \
        if (!array->u.MEMBER.values) { \
            jni_g_objects = array->next; \
            pthread_cond_destroy(&array->monitor_cond); \
            pthread_mutex_destroy(&array->monitor_lock); \
            free(array); \
            array = NULL; \
        } \
    } \
    if (array) array->u.MEMBER.length = length; \
    jni_SsrnxJniRef *ref = array ? jni_new_ref_locked(env, array, SSRNX_JNI_REF_LOCAL) : NULL; \
    pthread_mutex_unlock(&jni_g_lock); \
    if (!ref && !env->pending_exception) \
        jni_set_exception(env, "java/lang/OutOfMemoryError", #NAME " array"); \
    return ref; \
} \
static CTYPE *jni_get_##NAME##_array_elements(jni_SsrnxJNIEnv *env_handle, void *array_ref, uint8_t *is_copy) { \
    (void)env_handle; \
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref); \
    if (!array || array->kind != KIND) return NULL; \
    const size_t bytes = (size_t)array->u.MEMBER.length * sizeof(CTYPE); \
    CTYPE *copy = malloc(bytes ? bytes : 1u); \
    if (!copy) { \
        jni_SsrnxJniEnvState *env = jni_env_state(env_handle); \
        if (env && !env->pending_exception) \
            jni_set_exception(env, "java/lang/OutOfMemoryError", #NAME " array elements"); \
        return NULL; \
    } \
    if (bytes) memcpy(copy, array->u.MEMBER.values, bytes); \
    if (is_copy) *is_copy = JNI_TRUE; \
    return copy; \
} \
static void jni_release_##NAME##_array_elements(jni_SsrnxJNIEnv *env_handle, void *array_ref, \
                                                 CTYPE *elements, int32_t mode) { \
    (void)env_handle; \
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref); \
    if (!array || array->kind != KIND || !elements) return; \
    if (mode != 2 && array->u.MEMBER.length > 0) \
        memcpy(array->u.MEMBER.values, elements, (size_t)array->u.MEMBER.length * sizeof(CTYPE)); \
    if (mode != 1) free(elements); \
} \
static void jni_get_##NAME##_array_region(jni_SsrnxJNIEnv *env_handle, void *array_ref, int32_t start, \
                                           int32_t count, CTYPE *out) { \
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle); \
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref); \
    if (!env || !array || array->kind != KIND || !out || \
        !jni_array_region_ok(env, array->u.MEMBER.length, start, count)) return; \
    memcpy(out, array->u.MEMBER.values + start, (size_t)count * sizeof(CTYPE)); \
    if (KIND == SSRNX_JNI_OBJECT_INT_ARRAY) \
        jni_jni_int_array_read_diagnostic(array, array->u.MEMBER.length, start, count, \
                                      (const int32_t *)out, __builtin_return_address(0)); \
} \
static void jni_set_##NAME##_array_region(jni_SsrnxJNIEnv *env_handle, void *array_ref, int32_t start, \
                                           int32_t count, const CTYPE *in) { \
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle); \
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref); \
    if (!env || !array || array->kind != KIND || !in || \
        !jni_array_region_ok(env, array->u.MEMBER.length, start, count)) return; \
    memcpy(array->u.MEMBER.values + start, in, (size_t)count * sizeof(CTYPE)); \
}

DEFINE_PRIMITIVE_ARRAY_JNI(boolean, uint8_t, SSRNX_JNI_OBJECT_BOOLEAN_ARRAY, boolean_array, "[Z")
DEFINE_PRIMITIVE_ARRAY_JNI(byte, int8_t, SSRNX_JNI_OBJECT_BYTE_ARRAY, byte_array, "[B")
DEFINE_PRIMITIVE_ARRAY_JNI(char, uint16_t, SSRNX_JNI_OBJECT_CHAR_ARRAY, char_array, "[C")
DEFINE_PRIMITIVE_ARRAY_JNI(short, int16_t, SSRNX_JNI_OBJECT_SHORT_ARRAY, short_array, "[S")
DEFINE_PRIMITIVE_ARRAY_JNI(int, int32_t, SSRNX_JNI_OBJECT_INT_ARRAY, int_array, "[I")
DEFINE_PRIMITIVE_ARRAY_JNI(long, int64_t, SSRNX_JNI_OBJECT_LONG_ARRAY, long_array, "[J")
DEFINE_PRIMITIVE_ARRAY_JNI(float, float, SSRNX_JNI_OBJECT_FLOAT_ARRAY, float_array, "[F")
DEFINE_PRIMITIVE_ARRAY_JNI(double, double, SSRNX_JNI_OBJECT_DOUBLE_ARRAY, double_array, "[D")

#undef DEFINE_PRIMITIVE_ARRAY_JNI

static void jni_jni_get_string_region(jni_SsrnxJNIEnv *env_handle, void *string_ref,
                                  int32_t start, int32_t count, uint16_t *out) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *string = jni_object_from_ref(string_ref);
    if (!env || !string || string->kind != SSRNX_JNI_OBJECT_STRING || !out ||
        !jni_array_region_ok(env, string->u.string.length, start, count)) return;
    if (count > 0)
        memcpy(out, string->u.string.chars + start, (size_t)count * sizeof(uint16_t));
}

static void jni_jni_get_string_utf_region(jni_SsrnxJNIEnv *env_handle, void *string_ref,
                                      int32_t start, int32_t count, char *out) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *string = jni_object_from_ref(string_ref);
    if (!env || !string || string->kind != SSRNX_JNI_OBJECT_STRING || !out ||
        !jni_array_region_ok(env, string->u.string.length, start, count)) return;
    char *encoded = jni_mutf8_encode(string->u.string.chars + start, count);
    if (!encoded) {
        jni_set_exception(env, "java/lang/RuntimeException", "GetStringUTFRegion allocation failed");
        return;
    }
    memcpy(out, encoded, strlen(encoded));
    free(encoded);
}

static void *jni_jni_get_primitive_array_critical(jni_SsrnxJNIEnv *env_handle, void *array_ref,
                                               uint8_t *is_copy) {
    (void)env_handle;
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (!array) return NULL;
    if (is_copy) *is_copy = JNI_FALSE;
    switch (array->kind) {
        case SSRNX_JNI_OBJECT_BOOLEAN_ARRAY: return array->u.boolean_array.values;
        case SSRNX_JNI_OBJECT_BYTE_ARRAY: return array->u.byte_array.values;
        case SSRNX_JNI_OBJECT_CHAR_ARRAY: return array->u.char_array.values;
        case SSRNX_JNI_OBJECT_SHORT_ARRAY: return array->u.short_array.values;
        case SSRNX_JNI_OBJECT_INT_ARRAY: return array->u.int_array.values;
        case SSRNX_JNI_OBJECT_LONG_ARRAY: return array->u.long_array.values;
        case SSRNX_JNI_OBJECT_FLOAT_ARRAY: return array->u.float_array.values;
        case SSRNX_JNI_OBJECT_DOUBLE_ARRAY: return array->u.double_array.values;
        default: return NULL;
    }
}

void *ssrnx_jni_primitive_array_data_host(void *array_ref) {
    return jni_jni_get_primitive_array_critical(NULL, array_ref, NULL);
}

static void jni_jni_release_primitive_array_critical(jni_SsrnxJNIEnv *env_handle, void *array_ref,
                                                  void *elements, int32_t mode) {
    (void)env_handle; (void)array_ref; (void)elements; (void)mode;
    /* Primitive arrays are backed by stable SSRNX-owned storage and no moving GC
     * exists, so critical access is a direct pin and release needs no copy-back. */
}

static const uint16_t *jni_jni_get_string_critical(jni_SsrnxJNIEnv *env_handle, void *string_ref,
                                                uint8_t *is_copy) {
    (void)env_handle;
    jni_SsrnxJniObject *string = jni_object_from_ref(string_ref);
    if (!string || string->kind != SSRNX_JNI_OBJECT_STRING) return NULL;
    if (is_copy) *is_copy = JNI_FALSE;
    return string->u.string.chars;
}

static void jni_jni_release_string_critical(jni_SsrnxJNIEnv *env_handle, void *string_ref,
                                        const uint16_t *chars) {
    (void)env_handle; (void)string_ref; (void)chars;
}

static int32_t jni_jni_monitor_enter_timed(jni_SsrnxJNIEnv *env_handle, void *object_ref,
                                       uint32_t timeout_ms) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!env || !object || !object->monitor_initialized) return SSRNX_JNI_ERR;
    const pthread_t self = pthread_self();
    if (pthread_mutex_lock(&object->monitor_lock) != 0) return SSRNX_JNI_ERR;

    struct timespec deadline = {0};
    const bool timed = timeout_ms != UINT32_MAX;
    if (timed) {
        if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
            pthread_mutex_unlock(&object->monitor_lock);
            return SSRNX_JNI_ERR;
        }
        deadline.tv_sec += (time_t)(timeout_ms / 1000u);
        deadline.tv_nsec += (long)(timeout_ms % 1000u) * 1000000L;
        if (deadline.tv_nsec >= 1000000000L) {
            deadline.tv_sec++;
            deadline.tv_nsec -= 1000000000L;
        }
    }

    while (object->monitor_owner_valid && !pthread_equal(object->monitor_owner, self)) {
        const int rc = jni_jni_cond_wait(&object->monitor_cond, &object->monitor_lock, timed ? &deadline : NULL);
        if (rc == ETIMEDOUT) {
            pthread_mutex_unlock(&object->monitor_lock);
            jni_set_exception(env, "java/lang/RuntimeException", "DEX monitor wait watchdog exceeded");
            return SSRNX_JNI_ERR;
        }
        if (rc != 0) {
            pthread_mutex_unlock(&object->monitor_lock);
            return SSRNX_JNI_ERR;
        }
    }
    if (!object->monitor_owner_valid) {
        object->monitor_owner = self;
        object->monitor_owner_valid = true;
        object->monitor_depth = 1u;
    } else {
        object->monitor_depth++;
    }
    pthread_mutex_unlock(&object->monitor_lock);
    return SSRNX_JNI_OK;
}

static int32_t jni_jni_monitor_enter(jni_SsrnxJNIEnv *env_handle, void *object_ref) {
    return jni_jni_monitor_enter_timed(env_handle, object_ref, UINT32_MAX);
}

static int32_t jni_jni_monitor_exit(jni_SsrnxJNIEnv *env_handle, void *object_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!env || !object || !object->monitor_initialized) return SSRNX_JNI_ERR;
    const pthread_t self = pthread_self();
    if (pthread_mutex_lock(&object->monitor_lock) != 0) return SSRNX_JNI_ERR;
    if (!object->monitor_owner_valid || !pthread_equal(object->monitor_owner, self) ||
        object->monitor_depth == 0u) {
        pthread_mutex_unlock(&object->monitor_lock);
        jni_set_exception(env, "java/lang/IllegalMonitorStateException", "MonitorExit without ownership");
        return SSRNX_JNI_ERR;
    }
    object->monitor_depth--;
    if (object->monitor_depth == 0u) {
        object->monitor_owner_valid = false;
        if (object->monitor_waiters) pthread_cond_broadcast(&object->monitor_cond);
        else pthread_cond_signal(&object->monitor_cond);
    }
    pthread_mutex_unlock(&object->monitor_lock);
    return SSRNX_JNI_OK;
}

static int32_t jni_jni_monitor_notify(jni_SsrnxJNIEnv *env_handle, void *object_ref, bool notify_all) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!env || !object || !object->monitor_initialized) return SSRNX_JNI_ERR;
    const pthread_t self = pthread_self();
    if (pthread_mutex_lock(&object->monitor_lock) != 0) return SSRNX_JNI_ERR;
    if (!object->monitor_owner_valid || !pthread_equal(object->monitor_owner, self) ||
        object->monitor_depth == 0u) {
        pthread_mutex_unlock(&object->monitor_lock);
        jni_set_exception(env, "java/lang/IllegalMonitorStateException", "notify without ownership");
        return SSRNX_JNI_ERR;
    }
    /* notify moves one waiter out of the wait set, notifyAll all of them;
     * with none waiting it has no effect (JLS 17.2.2). */
    if (object->monitor_waiters > object->monitor_notifies)
        object->monitor_notifies = notify_all ? object->monitor_waiters : object->monitor_notifies + 1u;
    const int rc = object->monitor_notifies ? pthread_cond_broadcast(&object->monitor_cond) : 0;
    pthread_mutex_unlock(&object->monitor_lock);
    return rc == 0 ? SSRNX_JNI_OK : SSRNX_JNI_ERR;
}

/* Object.wait(timeout_ms, nanos) (JLS 17.2.1): the owner releases the monitor
 * completely, waits for a notification or the timeout (0 = none), then
 * reacquires it at its previous recursion depth. */
static int32_t jni_jni_monitor_wait(jni_SsrnxJNIEnv *env_handle, void *object_ref, int64_t timeout_ms, int32_t nanos) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!env || !object || !object->monitor_initialized) return SSRNX_JNI_ERR;
    if (timeout_ms < 0 || nanos < 0 || nanos > 999999) {
        jni_set_exception(env, "java/lang/IllegalArgumentException",
                      timeout_ms < 0 ? "timeout value is negative" : "nanosecond timeout value out of range");
        return SSRNX_JNI_ERR;
    }
    const pthread_t self = pthread_self();
    if (pthread_mutex_lock(&object->monitor_lock) != 0) return SSRNX_JNI_ERR;
    if (!object->monitor_owner_valid || !pthread_equal(object->monitor_owner, self) ||
        object->monitor_depth == 0u) {
        pthread_mutex_unlock(&object->monitor_lock);
        jni_set_exception(env, "java/lang/IllegalMonitorStateException", "object not locked by thread before wait()");
        return SSRNX_JNI_ERR;
    }
    struct timespec deadline = {0};
    const bool timed = timeout_ms > 0 || nanos > 0;
    if (timed) {
        (void)clock_gettime(CLOCK_REALTIME, &deadline);
        const int64_t add_ns = (timeout_ms % 1000) * 1000000LL + nanos;
        deadline.tv_sec += (time_t)(timeout_ms / 1000) + (time_t)(add_ns / 1000000000LL);
        deadline.tv_nsec += (long)(add_ns % 1000000000LL);
        if (deadline.tv_nsec >= 1000000000L) { deadline.tv_sec++; deadline.tv_nsec -= 1000000000L; }
    }
    const unsigned depth = object->monitor_depth;
    object->monitor_owner_valid = false;
    object->monitor_depth = 0u;
    object->monitor_waiters++;
    pthread_cond_broadcast(&object->monitor_cond);
    while (object->monitor_notifies == 0u && !ssrnx_jni_process_exit_requested(NULL, 0)) {
        struct timespec now = {0}, slice;
        (void)clock_gettime(CLOCK_REALTIME, &now);
        if (timed && (now.tv_sec > deadline.tv_sec ||
            (now.tv_sec == deadline.tv_sec && now.tv_nsec >= deadline.tv_nsec))) break;
        slice = now; slice.tv_nsec += 50000000L;
        if (slice.tv_nsec >= 1000000000L) { ++slice.tv_sec; slice.tv_nsec -= 1000000000L; }
        if (timed && (slice.tv_sec > deadline.tv_sec ||
            (slice.tv_sec == deadline.tv_sec && slice.tv_nsec > deadline.tv_nsec))) slice = deadline;
        const int rc = jni_jni_cond_wait(&object->monitor_cond, &object->monitor_lock, &slice);
        if (rc != 0 && rc != ETIMEDOUT && object->monitor_notifies == 0u) break;
    }
    if (object->monitor_notifies) object->monitor_notifies--;
    object->monitor_waiters--;
    if (object->monitor_notifies > object->monitor_waiters) object->monitor_notifies = object->monitor_waiters;
    while (object->monitor_owner_valid) (void)jni_jni_cond_wait(&object->monitor_cond, &object->monitor_lock, NULL);
    object->monitor_owner = self;
    object->monitor_owner_valid = true;
    object->monitor_depth = depth;
    pthread_mutex_unlock(&object->monitor_lock);
    return SSRNX_JNI_OK;
}

static void *jni_jni_get_method_id_common(jni_SsrnxJNIEnv *env_handle, void *clazz_ref, const char *name,
                                      const char *signature, bool is_static);
static void *jni_jni_new_object_a(jni_SsrnxJNIEnv *env_handle, void *clazz_ref, void *method_id,
                              const SsrnxJniValue *args);

/* java.nio direct buffers are the SSRNX core library's java.nio.DirectByteBuffer
 * (boot class path DEX). Like ART, NewDirectByteBuffer runs its
 * (long address, int capacity) constructor, and the Get* accessors read the
 * java.nio.Buffer address/capacity fields. Without the core library (host
 * tools linking only the JNI model) a bare ByteBuffer object carries them. */
#define SSRNX_DIRECT_BUFFER_CLASS "java/nio/DirectByteBuffer"

static bool jni_java_nio_buffer_field(jni_SsrnxJniObject *buffer, const char *name, const char *sig,
                                  SsrnxJniValue *out) {
    if (!buffer || !buffer->clazz) return false;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *nio_buffer = jni_find_class_locked("java/nio/Buffer");
    bool ok = false;
    if (nio_buffer && jni_class_is_assignable_to(buffer->clazz, nio_buffer)) {
        jni_SsrnxJniField *field = jni_find_field(buffer->clazz, name, sig, false);
        SsrnxJniValue *slot = field ? jni_instance_field_value(buffer, field) : NULL;
        if (slot) { *out = *slot; ok = true; }
    }
    pthread_mutex_unlock(&jni_g_lock);
    return ok;
}

static void *jni_jni_new_direct_byte_buffer(jni_SsrnxJNIEnv *env_handle, void *address, jni_SsrnxJLong capacity) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || !address || capacity < 0) return NULL;
    if (capacity > INT32_MAX) {
        jni_set_exception(env, "java/lang/IllegalArgumentException", "buffer capacity greater than maximum jint");
        return NULL;
    }
    jni_SsrnxJniClass *java_class = jni_resolve_class_lazy(SSRNX_DIRECT_BUFFER_CLASS, 0u);
    if (java_class) {
        SSRNX_PERF_LOCK(&jni_g_lock);
        jni_SsrnxJniRef *class_ref = jni_new_ref_locked(env, java_class->class_object, SSRNX_JNI_REF_LOCAL);
        pthread_mutex_unlock(&jni_g_lock);
        void *ctor = class_ref ? jni_jni_get_method_id_common(env_handle, class_ref, "<init>", "(JI)V", false) : NULL;
        void *result = NULL;
        if (ctor) {
            SsrnxJniValue args[2];
            memset(args, 0, sizeof(args));
            args[0].j = (int64_t)(uintptr_t)address;
            args[1].i = (int32_t)capacity;
            result = jni_jni_new_object_a(env_handle, class_ref, ctor, args);
        }
        if (class_ref) jni_jni_delete_local_ref(env_handle, class_ref);
        if (result || env->pending_exception) return result;
    }
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *clazz = jni_find_class_locked("java/nio/ByteBuffer");
    pthread_mutex_unlock(&jni_g_lock);
    if (!clazz && jni_register_class_internal("java/nio/ByteBuffer", "java/lang/Object") == SSRNX_OK) {
        SSRNX_PERF_LOCK(&jni_g_lock);
        clazz = jni_find_class_locked("java/nio/ByteBuffer");
        pthread_mutex_unlock(&jni_g_lock);
    }
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniObject *buffer = clazz ? jni_alloc_object_locked(SSRNX_JNI_OBJECT_DIRECT_BUFFER, clazz) : NULL;
    if (buffer) {
        buffer->u.direct_buffer.address = address;
        buffer->u.direct_buffer.capacity = capacity;
    }
    jni_SsrnxJniRef *ref = buffer ? jni_new_ref_locked(env, buffer, SSRNX_JNI_REF_LOCAL) : NULL;
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

static void *jni_jni_get_direct_buffer_address(jni_SsrnxJNIEnv *env_handle, void *buffer_ref) {
    (void)env_handle;
    jni_SsrnxJniObject *buffer = jni_object_from_ref(buffer_ref);
    if (!buffer) return NULL;
    if (buffer->kind == SSRNX_JNI_OBJECT_DIRECT_BUFFER) return buffer->u.direct_buffer.address;
    SsrnxJniValue address = {0};
    return jni_java_nio_buffer_field(buffer, "address", "J", &address) && address.j
        ? (void *)(uintptr_t)address.j : NULL;
}

static jni_SsrnxJLong jni_jni_get_direct_buffer_capacity(jni_SsrnxJNIEnv *env_handle, void *buffer_ref) {
    (void)env_handle;
    jni_SsrnxJniObject *buffer = jni_object_from_ref(buffer_ref);
    if (!buffer) return -1;
    if (buffer->kind == SSRNX_JNI_OBJECT_DIRECT_BUFFER) return buffer->u.direct_buffer.capacity;
    SsrnxJniValue address = {0}, capacity = {0};
    if (!jni_java_nio_buffer_field(buffer, "address", "J", &address) || !address.j) return -1;
    return jni_java_nio_buffer_field(buffer, "capacity", "I", &capacity) ? (jni_SsrnxJLong)capacity.i : -1;
}

static int32_t jni_jni_get_java_vm(jni_SsrnxJNIEnv *env_handle, void **out_vm) {
    if (!jni_env_state(env_handle) || !out_vm) return SSRNX_JNI_ERR;
    *out_vm = &jni_g_vm_handle;
    return SSRNX_JNI_OK;
}

static uint8_t jni_jni_exception_check(jni_SsrnxJNIEnv *env_handle) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    return env && env->pending_exception ? JNI_TRUE : JNI_FALSE;
}

static int32_t jni_jni_get_object_ref_type(jni_SsrnxJNIEnv *env_handle, void *ref_value) {
    (void)env_handle;
    jni_SsrnxJniRef *ref = jni_as_ref(ref_value);
    if (!ref) return 0;
    if (ref->kind == SSRNX_JNI_REF_LOCAL) return 1;
    if (ref->kind == SSRNX_JNI_REF_GLOBAL) return 2;
    if (ref->kind == SSRNX_JNI_REF_WEAK) return 3;
    return 0;
}

typedef struct jni_SsrnxJniNativeMethodAbi {
    const char *name;
    const char *signature;
    void *fn_ptr;
} jni_SsrnxJniNativeMethodAbi;

static int32_t jni_jni_register_natives(jni_SsrnxJNIEnv *env_handle, void *clazz_ref,
                                    const jni_SsrnxJniNativeMethodAbi *methods, int32_t count) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    if (!env || !clazz || count < 0 || (count > 0 && !methods)) return SSRNX_JNI_ERR;

    for (int32_t i = 0; i < count; ++i) {
        if (!methods[i].name || !methods[i].signature || !methods[i].fn_ptr)
            return SSRNX_JNI_ERR;
        char types[SSRNX_JNI_MAX_PARAMETER_SLOTS];
        if (jni_descriptor_args(methods[i].signature, types, sizeof(types)) < 0)
            return SSRNX_JNI_ERR;

        /* ART rejects RegisterNatives entries that do not identify a native
         * method declared by the target class. SSRNX can verify this exactly
         * for package DEX classes while leaving synthetic/framework model
         * classes available to host-side tests. */
        if (clazz->loader_kind == SSRNX_JNI_LOADER_APPLICATION &&
            ssrnx_dex_method_access_flags) {
            uint32_t method_flags = 0;
            if (!ssrnx_dex_method_access_flags(clazz->name, methods[i].name,
                                               methods[i].signature,
                                               &method_flags, NULL) ||
                (method_flags & SSRNX_ACC_NATIVE) == 0) {
                jni_set_exception(env, "java/lang/NoSuchMethodError", methods[i].name);
                SSRNX_LOGCW("JNI", "RegisterNatives rejected non-native/missing method class=%s name=%s sig=%s",
                            clazz->name, methods[i].name, methods[i].signature);
                return SSRNX_JNI_ERR;
            }
        }
    }

    SSRNX_PERF_LOCK(&jni_g_lock);
    for (int32_t i = 0; i < count; ++i) {
        jni_SsrnxJniNativeBinding *binding = clazz->native_bindings;
        while (binding && (strcmp(binding->name, methods[i].name) != 0 ||
                           strcmp(binding->signature, methods[i].signature) != 0)) binding = binding->next;
        if (binding) {
            binding->fn_ptr = methods[i].fn_ptr;
            continue;
        }
        binding = calloc(1, sizeof(*binding));
        if (!binding) { pthread_mutex_unlock(&jni_g_lock); return SSRNX_JNI_ENOMEM; }
        binding->name = jni_dup_string(methods[i].name);
        binding->signature = jni_dup_string(methods[i].signature);
        if (!binding->name || !binding->signature) {
            free(binding->name); free(binding->signature); free(binding);
            pthread_mutex_unlock(&jni_g_lock);
            return SSRNX_JNI_ENOMEM;
        }
        binding->fn_ptr = methods[i].fn_ptr;
        binding->next = clazz->native_bindings;
        clazz->native_bindings = binding;
        jni_g_native_binding_count++;
    }
    pthread_mutex_unlock(&jni_g_lock);
    SSRNX_LOGCI("JNI", "RegisterNatives class=%s count=%" PRId32, clazz->name, count);
    return SSRNX_JNI_OK;
}

static int32_t jni_jni_unregister_natives(jni_SsrnxJNIEnv *env_handle, void *clazz_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(clazz_ref);
    if (!env || !clazz) return SSRNX_JNI_ERR;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniNativeBinding *binding = clazz->native_bindings;
    clazz->native_bindings = NULL;
    while (binding) {
        jni_SsrnxJniNativeBinding *next = binding->next;
        free(binding->name); free(binding->signature); free(binding);
        if (jni_g_native_binding_count) jni_g_native_binding_count--;
        binding = next;
    }
    pthread_mutex_unlock(&jni_g_lock);
    SSRNX_LOGCI("JNI", "UnregisterNatives class=%s", clazz->name);
    return SSRNX_JNI_OK;
}

/* JNI FatalError: "Raises a fatal error and does not expect the VM to
 * recover. This function does not return." Android logs the message and
 * aborts the process. */
static void jni_jni_fatal_error(jni_SsrnxJNIEnv *env_handle, const char *message) {
    (void)env_handle;
    SSRNX_LOGCE("JNI", "FatalError called: %s", message ? message : "(null)");
    abort();
}

/* DefineClass: Android cannot define classes from class-file bytes; ART
 * logs "JNI DefineClass is not supported" and returns NULL. */
static void *jni_jni_define_class(jni_SsrnxJNIEnv *env, const char *name, void *loader,
                              const int8_t *buf, int32_t len) {
    (void)env; (void)loader; (void)buf;
    SSRNX_LOGCW("JNI", "DefineClass is not supported name=%s len=%d", name ? name : "(null)", (int)len);
    return NULL;
}

static void jni_setup_tables(void) {
    memset(jni_g_jni_table, 0, sizeof(jni_g_jni_table));
    memset(jni_g_vm_table, 0, sizeof(jni_g_vm_table));

    jni_g_vm_table[3] = JNI_SLOT(jni_vm_destroy);
    jni_g_vm_table[4] = JNI_SLOT(jni_vm_attach);
    jni_g_vm_table[5] = JNI_SLOT(jni_vm_detach);
    jni_g_vm_table[6] = JNI_SLOT(jni_vm_get_env);
    jni_g_vm_table[7] = JNI_SLOT(jni_vm_attach_daemon);
    jni_g_vm_handle.functions = (const void *const *)jni_g_vm_table;

    jni_g_jni_table[4] = JNI_SLOT(jni_jni_get_version);
    jni_g_jni_table[5] = JNI_SLOT(jni_jni_define_class);
    jni_g_jni_table[6] = JNI_SLOT(jni_jni_find_class);
    jni_g_jni_table[7] = JNI_SLOT(jni_jni_from_reflected_method);
    jni_g_jni_table[8] = JNI_SLOT(jni_jni_from_reflected_field);
    jni_g_jni_table[9] = JNI_SLOT(jni_jni_to_reflected_method);
    jni_g_jni_table[10] = JNI_SLOT(jni_jni_get_superclass);
    jni_g_jni_table[11] = JNI_SLOT(jni_jni_is_assignable_from);
    jni_g_jni_table[12] = JNI_SLOT(jni_jni_to_reflected_field);
    jni_g_jni_table[13] = JNI_SLOT(jni_jni_throw);
    jni_g_jni_table[14] = JNI_SLOT(jni_jni_throw_new);
    jni_g_jni_table[15] = JNI_SLOT(jni_jni_exception_occurred);
    jni_g_jni_table[16] = JNI_SLOT(jni_jni_exception_describe);
    jni_g_jni_table[17] = JNI_SLOT(jni_jni_exception_clear);
    jni_g_jni_table[18] = JNI_SLOT(jni_jni_fatal_error);
    jni_g_jni_table[19] = JNI_SLOT(jni_jni_push_local_frame);
    jni_g_jni_table[20] = JNI_SLOT(jni_jni_pop_local_frame);
    jni_g_jni_table[21] = JNI_SLOT(jni_jni_new_global_ref);
    jni_g_jni_table[22] = JNI_SLOT(jni_jni_delete_global_ref);
    jni_g_jni_table[23] = JNI_SLOT(jni_jni_delete_local_ref);
    jni_g_jni_table[24] = JNI_SLOT(jni_jni_is_same_object);
    jni_g_jni_table[25] = JNI_SLOT(jni_jni_new_local_ref);
    jni_g_jni_table[26] = JNI_SLOT(jni_jni_ensure_local_capacity);
    jni_g_jni_table[27] = JNI_SLOT(jni_jni_alloc_object);
    jni_g_jni_table[28] = JNI_SLOT(jni_jni_new_object);
    jni_g_jni_table[29] = JNI_SLOT(jni_jni_new_object_v);
    jni_g_jni_table[30] = JNI_SLOT(jni_jni_new_object_a);
    jni_g_jni_table[31] = JNI_SLOT(jni_jni_get_object_class);
    jni_g_jni_table[32] = JNI_SLOT(jni_jni_is_instance_of);
    jni_g_jni_table[33] = JNI_SLOT(jni_jni_get_method_id);
    jni_g_jni_table[34] = JNI_SLOT(jni_call_object);
    jni_g_jni_table[35] = JNI_SLOT(jni_call_object_v);
    jni_g_jni_table[36] = JNI_SLOT(jni_call_object_a);
    jni_g_jni_table[37] = JNI_SLOT(jni_call_boolean);
    jni_g_jni_table[38] = JNI_SLOT(jni_call_boolean_v);
    jni_g_jni_table[39] = JNI_SLOT(jni_call_boolean_a);
    jni_g_jni_table[40] = JNI_SLOT(jni_call_byte);
    jni_g_jni_table[41] = JNI_SLOT(jni_call_byte_v);
    jni_g_jni_table[42] = JNI_SLOT(jni_call_byte_a);
    jni_g_jni_table[43] = JNI_SLOT(jni_call_char);
    jni_g_jni_table[44] = JNI_SLOT(jni_call_char_v);
    jni_g_jni_table[45] = JNI_SLOT(jni_call_char_a);
    jni_g_jni_table[46] = JNI_SLOT(jni_call_short);
    jni_g_jni_table[47] = JNI_SLOT(jni_call_short_v);
    jni_g_jni_table[48] = JNI_SLOT(jni_call_short_a);
    jni_g_jni_table[49] = JNI_SLOT(jni_call_int);
    jni_g_jni_table[50] = JNI_SLOT(jni_call_int_v);
    jni_g_jni_table[51] = JNI_SLOT(jni_call_int_a);
    jni_g_jni_table[52] = JNI_SLOT(jni_call_long);
    jni_g_jni_table[53] = JNI_SLOT(jni_call_long_v);
    jni_g_jni_table[54] = JNI_SLOT(jni_call_long_a);
    jni_g_jni_table[55] = JNI_SLOT(jni_call_float);
    jni_g_jni_table[56] = JNI_SLOT(jni_call_float_v);
    jni_g_jni_table[57] = JNI_SLOT(jni_call_float_a);
    jni_g_jni_table[58] = JNI_SLOT(jni_call_double);
    jni_g_jni_table[59] = JNI_SLOT(jni_call_double_v);
    jni_g_jni_table[60] = JNI_SLOT(jni_call_double_a);
    jni_g_jni_table[61] = JNI_SLOT(jni_call_void);
    jni_g_jni_table[62] = JNI_SLOT(jni_call_void_v);
    jni_g_jni_table[63] = JNI_SLOT(jni_call_void_a);
    jni_g_jni_table[64] = JNI_SLOT(jni_call_nonvirtual_object);
    jni_g_jni_table[65] = JNI_SLOT(jni_call_nonvirtual_object_v);
    jni_g_jni_table[66] = JNI_SLOT(jni_call_nonvirtual_object_a);
    jni_g_jni_table[67] = JNI_SLOT(jni_call_nonvirtual_boolean);
    jni_g_jni_table[68] = JNI_SLOT(jni_call_nonvirtual_boolean_v);
    jni_g_jni_table[69] = JNI_SLOT(jni_call_nonvirtual_boolean_a);
    jni_g_jni_table[70] = JNI_SLOT(jni_call_nonvirtual_byte);
    jni_g_jni_table[71] = JNI_SLOT(jni_call_nonvirtual_byte_v);
    jni_g_jni_table[72] = JNI_SLOT(jni_call_nonvirtual_byte_a);
    jni_g_jni_table[73] = JNI_SLOT(jni_call_nonvirtual_char);
    jni_g_jni_table[74] = JNI_SLOT(jni_call_nonvirtual_char_v);
    jni_g_jni_table[75] = JNI_SLOT(jni_call_nonvirtual_char_a);
    jni_g_jni_table[76] = JNI_SLOT(jni_call_nonvirtual_short);
    jni_g_jni_table[77] = JNI_SLOT(jni_call_nonvirtual_short_v);
    jni_g_jni_table[78] = JNI_SLOT(jni_call_nonvirtual_short_a);
    jni_g_jni_table[79] = JNI_SLOT(jni_call_nonvirtual_int);
    jni_g_jni_table[80] = JNI_SLOT(jni_call_nonvirtual_int_v);
    jni_g_jni_table[81] = JNI_SLOT(jni_call_nonvirtual_int_a);
    jni_g_jni_table[82] = JNI_SLOT(jni_call_nonvirtual_long);
    jni_g_jni_table[83] = JNI_SLOT(jni_call_nonvirtual_long_v);
    jni_g_jni_table[84] = JNI_SLOT(jni_call_nonvirtual_long_a);
    jni_g_jni_table[85] = JNI_SLOT(jni_call_nonvirtual_float);
    jni_g_jni_table[86] = JNI_SLOT(jni_call_nonvirtual_float_v);
    jni_g_jni_table[87] = JNI_SLOT(jni_call_nonvirtual_float_a);
    jni_g_jni_table[88] = JNI_SLOT(jni_call_nonvirtual_double);
    jni_g_jni_table[89] = JNI_SLOT(jni_call_nonvirtual_double_v);
    jni_g_jni_table[90] = JNI_SLOT(jni_call_nonvirtual_double_a);
    jni_g_jni_table[91] = JNI_SLOT(jni_call_nonvirtual_void);
    jni_g_jni_table[92] = JNI_SLOT(jni_call_nonvirtual_void_v);
    jni_g_jni_table[93] = JNI_SLOT(jni_call_nonvirtual_void_a);
    jni_g_jni_table[94] = JNI_SLOT(jni_jni_get_field_id);
    jni_g_jni_table[95] = JNI_SLOT(jni_jni_get_object_field);
    jni_g_jni_table[96] = JNI_SLOT(jni_jni_get_boolean_field);
    jni_g_jni_table[97] = JNI_SLOT(jni_get_byte_field);
    jni_g_jni_table[98] = JNI_SLOT(jni_get_char_field);
    jni_g_jni_table[99] = JNI_SLOT(jni_get_short_field);
    jni_g_jni_table[100] = JNI_SLOT(jni_jni_get_int_field);
    jni_g_jni_table[101] = JNI_SLOT(jni_jni_get_long_field);
    jni_g_jni_table[102] = JNI_SLOT(jni_jni_get_float_field);
    jni_g_jni_table[103] = JNI_SLOT(jni_jni_get_double_field);
    jni_g_jni_table[104] = JNI_SLOT(jni_jni_set_object_field);
    jni_g_jni_table[105] = JNI_SLOT(jni_jni_set_boolean_field);
    jni_g_jni_table[106] = JNI_SLOT(jni_set_byte_field);
    jni_g_jni_table[107] = JNI_SLOT(jni_set_char_field);
    jni_g_jni_table[108] = JNI_SLOT(jni_set_short_field);
    jni_g_jni_table[109] = JNI_SLOT(jni_jni_set_int_field);
    jni_g_jni_table[110] = JNI_SLOT(jni_jni_set_long_field);
    jni_g_jni_table[111] = JNI_SLOT(jni_jni_set_float_field);
    jni_g_jni_table[112] = JNI_SLOT(jni_jni_set_double_field);
    jni_g_jni_table[113] = JNI_SLOT(jni_jni_get_static_method_id);
    jni_g_jni_table[114] = JNI_SLOT(jni_call_static_object);
    jni_g_jni_table[115] = JNI_SLOT(jni_call_static_object_v);
    jni_g_jni_table[116] = JNI_SLOT(jni_call_static_object_a);
    jni_g_jni_table[117] = JNI_SLOT(jni_call_static_boolean);
    jni_g_jni_table[118] = JNI_SLOT(jni_call_static_boolean_v);
    jni_g_jni_table[119] = JNI_SLOT(jni_call_static_boolean_a);
    jni_g_jni_table[120] = JNI_SLOT(jni_call_static_byte);
    jni_g_jni_table[121] = JNI_SLOT(jni_call_static_byte_v);
    jni_g_jni_table[122] = JNI_SLOT(jni_call_static_byte_a);
    jni_g_jni_table[123] = JNI_SLOT(jni_call_static_char);
    jni_g_jni_table[124] = JNI_SLOT(jni_call_static_char_v);
    jni_g_jni_table[125] = JNI_SLOT(jni_call_static_char_a);
    jni_g_jni_table[126] = JNI_SLOT(jni_call_static_short);
    jni_g_jni_table[127] = JNI_SLOT(jni_call_static_short_v);
    jni_g_jni_table[128] = JNI_SLOT(jni_call_static_short_a);
    jni_g_jni_table[129] = JNI_SLOT(jni_call_static_int);
    jni_g_jni_table[130] = JNI_SLOT(jni_call_static_int_v);
    jni_g_jni_table[131] = JNI_SLOT(jni_call_static_int_a);
    jni_g_jni_table[132] = JNI_SLOT(jni_call_static_long);
    jni_g_jni_table[133] = JNI_SLOT(jni_call_static_long_v);
    jni_g_jni_table[134] = JNI_SLOT(jni_call_static_long_a);
    jni_g_jni_table[135] = JNI_SLOT(jni_call_static_float);
    jni_g_jni_table[136] = JNI_SLOT(jni_call_static_float_v);
    jni_g_jni_table[137] = JNI_SLOT(jni_call_static_float_a);
    jni_g_jni_table[138] = JNI_SLOT(jni_call_static_double);
    jni_g_jni_table[139] = JNI_SLOT(jni_call_static_double_v);
    jni_g_jni_table[140] = JNI_SLOT(jni_call_static_double_a);
    jni_g_jni_table[141] = JNI_SLOT(jni_call_static_void);
    jni_g_jni_table[142] = JNI_SLOT(jni_call_static_void_v);
    jni_g_jni_table[143] = JNI_SLOT(jni_call_static_void_a);
    jni_g_jni_table[144] = JNI_SLOT(jni_jni_get_static_field_id);
    jni_g_jni_table[145] = JNI_SLOT(jni_jni_get_static_object_field);
    jni_g_jni_table[146] = JNI_SLOT(jni_jni_get_static_boolean_field);
    jni_g_jni_table[147] = JNI_SLOT(jni_get_static_byte_field);
    jni_g_jni_table[148] = JNI_SLOT(jni_get_static_char_field);
    jni_g_jni_table[149] = JNI_SLOT(jni_get_static_short_field);
    jni_g_jni_table[150] = JNI_SLOT(jni_jni_get_static_int_field);
    jni_g_jni_table[151] = JNI_SLOT(jni_jni_get_static_long_field);
    jni_g_jni_table[152] = JNI_SLOT(jni_jni_get_static_float_field);
    jni_g_jni_table[153] = JNI_SLOT(jni_jni_get_static_double_field);
    jni_g_jni_table[154] = JNI_SLOT(jni_jni_set_static_object_field);
    jni_g_jni_table[155] = JNI_SLOT(jni_jni_set_static_boolean_field);
    jni_g_jni_table[156] = JNI_SLOT(jni_set_static_byte_field);
    jni_g_jni_table[157] = JNI_SLOT(jni_set_static_char_field);
    jni_g_jni_table[158] = JNI_SLOT(jni_set_static_short_field);
    jni_g_jni_table[159] = JNI_SLOT(jni_jni_set_static_int_field);
    jni_g_jni_table[160] = JNI_SLOT(jni_jni_set_static_long_field);
    jni_g_jni_table[161] = JNI_SLOT(jni_jni_set_static_float_field);
    jni_g_jni_table[162] = JNI_SLOT(jni_jni_set_static_double_field);
    jni_g_jni_table[163] = JNI_SLOT(jni_jni_new_string);
    jni_g_jni_table[164] = JNI_SLOT(jni_jni_get_string_length);
    jni_g_jni_table[165] = JNI_SLOT(jni_jni_get_string_chars);
    jni_g_jni_table[166] = JNI_SLOT(jni_jni_release_string_chars);
    jni_g_jni_table[167] = JNI_SLOT(jni_jni_new_string_utf);
    jni_g_jni_table[168] = JNI_SLOT(jni_jni_get_string_utf_length);
    jni_g_jni_table[169] = JNI_SLOT(jni_jni_get_string_utf_chars);
    jni_g_jni_table[170] = JNI_SLOT(jni_jni_release_string_utf_chars);
    jni_g_jni_table[171] = JNI_SLOT(jni_jni_get_array_length);
    jni_g_jni_table[172] = JNI_SLOT(jni_jni_new_object_array);
    jni_g_jni_table[173] = JNI_SLOT(jni_jni_get_object_array_element);
    jni_g_jni_table[174] = JNI_SLOT(jni_jni_set_object_array_element);
    jni_g_jni_table[175] = JNI_SLOT(jni_new_boolean_array);
    jni_g_jni_table[176] = JNI_SLOT(jni_new_byte_array);
    jni_g_jni_table[177] = JNI_SLOT(jni_new_char_array);
    jni_g_jni_table[178] = JNI_SLOT(jni_new_short_array);
    jni_g_jni_table[179] = JNI_SLOT(jni_new_int_array);
    jni_g_jni_table[180] = JNI_SLOT(jni_new_long_array);
    jni_g_jni_table[181] = JNI_SLOT(jni_new_float_array);
    jni_g_jni_table[182] = JNI_SLOT(jni_new_double_array);
    jni_g_jni_table[183] = JNI_SLOT(jni_get_boolean_array_elements);
    jni_g_jni_table[184] = JNI_SLOT(jni_get_byte_array_elements);
    jni_g_jni_table[185] = JNI_SLOT(jni_get_char_array_elements);
    jni_g_jni_table[186] = JNI_SLOT(jni_get_short_array_elements);
    jni_g_jni_table[187] = JNI_SLOT(jni_get_int_array_elements);
    jni_g_jni_table[188] = JNI_SLOT(jni_get_long_array_elements);
    jni_g_jni_table[189] = JNI_SLOT(jni_get_float_array_elements);
    jni_g_jni_table[190] = JNI_SLOT(jni_get_double_array_elements);
    jni_g_jni_table[191] = JNI_SLOT(jni_release_boolean_array_elements);
    jni_g_jni_table[192] = JNI_SLOT(jni_release_byte_array_elements);
    jni_g_jni_table[193] = JNI_SLOT(jni_release_char_array_elements);
    jni_g_jni_table[194] = JNI_SLOT(jni_release_short_array_elements);
    jni_g_jni_table[195] = JNI_SLOT(jni_release_int_array_elements);
    jni_g_jni_table[196] = JNI_SLOT(jni_release_long_array_elements);
    jni_g_jni_table[197] = JNI_SLOT(jni_release_float_array_elements);
    jni_g_jni_table[198] = JNI_SLOT(jni_release_double_array_elements);
    jni_g_jni_table[199] = JNI_SLOT(jni_get_boolean_array_region);
    jni_g_jni_table[200] = JNI_SLOT(jni_get_byte_array_region);
    jni_g_jni_table[201] = JNI_SLOT(jni_get_char_array_region);
    jni_g_jni_table[202] = JNI_SLOT(jni_get_short_array_region);
    jni_g_jni_table[203] = JNI_SLOT(jni_get_int_array_region);
    jni_g_jni_table[204] = JNI_SLOT(jni_get_long_array_region);
    jni_g_jni_table[205] = JNI_SLOT(jni_get_float_array_region);
    jni_g_jni_table[206] = JNI_SLOT(jni_get_double_array_region);
    jni_g_jni_table[207] = JNI_SLOT(jni_set_boolean_array_region);
    jni_g_jni_table[208] = JNI_SLOT(jni_set_byte_array_region);
    jni_g_jni_table[209] = JNI_SLOT(jni_set_char_array_region);
    jni_g_jni_table[210] = JNI_SLOT(jni_set_short_array_region);
    jni_g_jni_table[211] = JNI_SLOT(jni_set_int_array_region);
    jni_g_jni_table[212] = JNI_SLOT(jni_set_long_array_region);
    jni_g_jni_table[213] = JNI_SLOT(jni_set_float_array_region);
    jni_g_jni_table[214] = JNI_SLOT(jni_set_double_array_region);
    jni_g_jni_table[215] = JNI_SLOT(jni_jni_register_natives);
    jni_g_jni_table[216] = JNI_SLOT(jni_jni_unregister_natives);
    jni_g_jni_table[217] = JNI_SLOT(jni_jni_monitor_enter);
    jni_g_jni_table[218] = JNI_SLOT(jni_jni_monitor_exit);
    jni_g_jni_table[219] = JNI_SLOT(jni_jni_get_java_vm);
    jni_g_jni_table[220] = JNI_SLOT(jni_jni_get_string_region);
    jni_g_jni_table[221] = JNI_SLOT(jni_jni_get_string_utf_region);
    jni_g_jni_table[222] = JNI_SLOT(jni_jni_get_primitive_array_critical);
    jni_g_jni_table[223] = JNI_SLOT(jni_jni_release_primitive_array_critical);
    jni_g_jni_table[224] = JNI_SLOT(jni_jni_get_string_critical);
    jni_g_jni_table[225] = JNI_SLOT(jni_jni_release_string_critical);
    jni_g_jni_table[226] = JNI_SLOT(jni_jni_new_weak_global_ref);
    jni_g_jni_table[227] = JNI_SLOT(jni_jni_delete_weak_global_ref);
    jni_g_jni_table[228] = JNI_SLOT(jni_jni_exception_check);
    jni_g_jni_table[229] = JNI_SLOT(jni_jni_new_direct_byte_buffer);
    jni_g_jni_table[230] = JNI_SLOT(jni_jni_get_direct_buffer_address);
    jni_g_jni_table[231] = JNI_SLOT(jni_jni_get_direct_buffer_capacity);
    jni_g_jni_table[232] = JNI_SLOT(jni_jni_get_object_ref_type);
}

static SsrnxResult jni_register_class_internal_ex_loader(const char *name, const char *super_name,
                                                       bool hierarchy_provisional,
                                                       jni_SsrnxJniClassLoaderKind loader_kind) {
    if (!name || !name[0]) return SSRNX_ERR_ARGUMENT;
    SSRNX_PERF_LOCK(&jni_g_lock);
    if (jni_find_class_locked(name)) {
        pthread_mutex_unlock(&jni_g_lock);
        return SSRNX_OK;
    }
    jni_SsrnxJniClass *super = NULL;
    if (super_name) {
        super = jni_find_class_locked(super_name);
        if (!super) {
            pthread_mutex_unlock(&jni_g_lock);
            return SSRNX_ERR_ARGUMENT;
        }
    }
    jni_SsrnxJniClass *clazz = calloc(1, sizeof(*clazz));
    jni_SsrnxJniObject *class_object = calloc(1, sizeof(*class_object));
    char *copy = jni_dup_string(name);
    if (!clazz || !class_object || !copy) {
        free(clazz); free(class_object); free(copy);
        pthread_mutex_unlock(&jni_g_lock);
        return SSRNX_ERR_NO_MEMORY;
    }
    clazz->name = copy;
    clazz->super = super;
    clazz->hierarchy_provisional = hierarchy_provisional;
    clazz->loader_kind = loader_kind;
    clazz->class_object = class_object;
    if (pthread_mutex_init(&class_object->monitor_lock, NULL) != 0) {
        free(clazz->name); free(clazz); free(class_object);
        pthread_mutex_unlock(&jni_g_lock);
        return SSRNX_ERR_NO_MEMORY;
    }
    if (pthread_cond_init(&class_object->monitor_cond, NULL) != 0) {
        pthread_mutex_destroy(&class_object->monitor_lock);
        free(clazz->name); free(clazz); free(class_object);
        pthread_mutex_unlock(&jni_g_lock);
        return SSRNX_ERR_NO_MEMORY;
    }
    if (!jni_ptr_set_insert_locked(&jni_g_object_set, (uintptr_t)class_object)) {
        pthread_cond_destroy(&class_object->monitor_cond);
        pthread_mutex_destroy(&class_object->monitor_lock);
        free(clazz->name); free(clazz); free(class_object);
        pthread_mutex_unlock(&jni_g_lock);
        return SSRNX_ERR_NO_MEMORY;
    }
    class_object->monitor_initialized = true;
    class_object->kind = SSRNX_JNI_OBJECT_CLASS;
    class_object->clazz = strcmp(name, "java/lang/Class") == 0 ? clazz : jni_find_class_locked("java/lang/Class");
    class_object->u.class_data.descriptor = clazz;
    class_object->next = jni_g_objects;
    jni_g_objects = class_object;
    clazz->next = jni_g_classes;
    jni_g_classes = clazz;
    /* Published last: lock-free readers may use the class at once. */
    if (!jni_g_class_index_disabled && !jni_class_index_insert_locked(clazz)) {
        jni_class_index_reset_locked();
        jni_g_class_index_disabled = true; /* out of memory: linear lookup from now on */
    }
    jni_g_class_count++;
    pthread_mutex_unlock(&jni_g_lock);
    return SSRNX_OK;
}

static SsrnxResult jni_register_class_internal_ex(const char *name, const char *super_name,
                                                bool hierarchy_provisional) {
    return jni_register_class_internal_ex_loader(name, super_name, hierarchy_provisional,
                                             SSRNX_JNI_LOADER_BOOT);
}

static SsrnxResult jni_register_class_internal(const char *name, const char *super_name) {
    return jni_register_class_internal_ex_loader(name, super_name, false, SSRNX_JNI_LOADER_BOOT);
}

SsrnxResult ssrnx_jni_register_class(const char *name, const char *super_name) {
    if (!jni_g_initialized) return SSRNX_ERR_ARGUMENT;
    return jni_register_class_internal_ex(name, super_name, false);
}

SsrnxResult ssrnx_jni_register_interface(const char *class_name, const char *interface_name) {
    if (!jni_g_initialized || !class_name || !interface_name ||
        !class_name[0] || !interface_name[0]) return SSRNX_ERR_ARGUMENT;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *clazz = jni_find_class_locked(class_name);
    jni_SsrnxJniClass *interface_class = jni_find_class_locked(interface_name);
    if (!clazz || !interface_class || clazz == interface_class ||
        jni_class_is_assignable_to(interface_class, clazz)) {
        pthread_mutex_unlock(&jni_g_lock);
        return SSRNX_ERR_ARGUMENT;
    }
    for (jni_SsrnxJniInterfaceLink *link = clazz->interfaces; link; link = link->next) {
        if (link->clazz == interface_class) {
            pthread_mutex_unlock(&jni_g_lock);
            return SSRNX_OK;
        }
    }
    jni_SsrnxJniInterfaceLink *link = calloc(1, sizeof(*link));
    if (!link) {
        pthread_mutex_unlock(&jni_g_lock);
        return SSRNX_ERR_NO_MEMORY;
    }
    link->clazz = interface_class;
    link->next = clazz->interfaces;
    clazz->interfaces = link;
    jni_g_dispatch_generation++;
    /* class_def/ClassFile interfaces[] entries must name interface types. */
    interface_class->is_interface = true;
    pthread_mutex_unlock(&jni_g_lock);
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_register_class_provisional(const char *name, const char *super_name) {
    if (!jni_g_initialized) return SSRNX_ERR_ARGUMENT;
    /* Provisional classes are package/SDK classes discovered before their DEX
     * hierarchy has been materialized; they therefore belong to the
     * application loader rather than the bootstrap framework namespace. */
    return jni_register_class_internal_ex_loader(name, super_name, true,
                                             SSRNX_JNI_LOADER_APPLICATION);
}

SsrnxResult ssrnx_jni_register_method(const char *class_name, const char *name,
                                      const char *signature, bool is_static,
                                      SsrnxJniMethodCallback callback, void *user_data) {
    if (!jni_g_initialized || !class_name || !name || !signature) return SSRNX_ERR_ARGUMENT;
    char types[SSRNX_JNI_MAX_PARAMETER_SLOTS];
    if (jni_descriptor_args(signature, types, sizeof(types)) < 0) return SSRNX_ERR_ARGUMENT;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *clazz = jni_find_class_locked(class_name);
    if (!clazz || jni_find_declared_method(clazz, name, signature, is_static)) {
        pthread_mutex_unlock(&jni_g_lock);
        return clazz ? SSRNX_OK : SSRNX_ERR_ARGUMENT;
    }
    jni_SsrnxJniMethod *method = calloc(1, sizeof(*method));
    if (!method) { pthread_mutex_unlock(&jni_g_lock); return SSRNX_ERR_NO_MEMORY; }
    method->name = jni_dup_string(name);
    method->signature = jni_dup_string(signature);
    if (!method->name || !method->signature) {
        free(method->name); free(method->signature); free(method);
        pthread_mutex_unlock(&jni_g_lock);
        return SSRNX_ERR_NO_MEMORY;
    }
    method->is_static = is_static;
    method->callback = callback;
    method->user_data = user_data;
    method->owner = clazz;
    method->next = clazz->methods;
    clazz->methods = method;
    jni_g_method_count++;
    jni_g_dispatch_generation++;
    pthread_mutex_unlock(&jni_g_lock);
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_register_field(const char *class_name, const char *name,
                                     const char *signature, bool is_static,
                                     SsrnxJniValue initial_value) {
    if (!jni_g_initialized || !class_name || !name || !signature || !signature[0]) return SSRNX_ERR_ARGUMENT;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *clazz = jni_find_class_locked(class_name);
    bool declared = false;
    if (clazz) {
        for (jni_SsrnxJniField *existing = clazz->fields; existing; existing = existing->next) {
            if (existing->is_static == is_static && strcmp(existing->name, name) == 0 &&
                strcmp(existing->signature, signature) == 0) { declared = true; break; }
        }
    }
    if (!clazz || declared) {
        pthread_mutex_unlock(&jni_g_lock);
        return clazz ? SSRNX_OK : SSRNX_ERR_ARGUMENT;
    }
    jni_SsrnxJniField *field = calloc(1, sizeof(*field));
    if (!field) { pthread_mutex_unlock(&jni_g_lock); return SSRNX_ERR_NO_MEMORY; }
    field->name = jni_dup_string(name);
    field->signature = jni_dup_string(signature);
    if (!field->name || !field->signature) {
        free(field->name); free(field->signature); free(field);
        pthread_mutex_unlock(&jni_g_lock);
        return SSRNX_ERR_NO_MEMORY;
    }
    field->is_static = is_static;
    field->owner = clazz;
    if (is_static) {
        field->static_value = initial_value;
    } else {
        /* Objects keep per-field cells created on first access, so a field
         * discovered after instances exist needs no change to them. */
        const size_t new_slot_count = jni_g_instance_field_slot_count + 1u;
        field->instance_index = jni_g_instance_field_slot_count;
        jni_g_instance_field_slot_count = new_slot_count;
        clazz->instance_field_count++;
    }
    field->next = clazz->fields;
    clazz->fields = field;
    jni_g_field_count++;
    pthread_mutex_unlock(&jni_g_lock);
    return SSRNX_OK;
}

static void jni_free_model(void) {
    jni_SsrnxJniRef *ref = jni_g_refs;
    while (ref) { jni_SsrnxJniRef *next = ref->next_all; free(ref); ref = next; }
    jni_g_refs = NULL;
    jni_ptr_set_reset_locked(&jni_g_ref_set);
    jni_ptr_set_reset_locked(&jni_g_object_set);

    jni_SsrnxJniObject *object = jni_g_objects;
    while (object) {
        jni_SsrnxJniObject *next = object->next;
        if (object->kind == SSRNX_JNI_OBJECT_INSTANCE) jni_free_field_cells(object);
        else if (object->kind == SSRNX_JNI_OBJECT_STRING) free(object->u.string.chars);
        else if (object->kind == SSRNX_JNI_OBJECT_BOOLEAN_ARRAY) free(object->u.boolean_array.values);
        else if (object->kind == SSRNX_JNI_OBJECT_BYTE_ARRAY) free(object->u.byte_array.values);
        else if (object->kind == SSRNX_JNI_OBJECT_CHAR_ARRAY) free(object->u.char_array.values);
        else if (object->kind == SSRNX_JNI_OBJECT_SHORT_ARRAY) free(object->u.short_array.values);
        else if (object->kind == SSRNX_JNI_OBJECT_INT_ARRAY) free(object->u.int_array.values);
        else if (object->kind == SSRNX_JNI_OBJECT_LONG_ARRAY) free(object->u.long_array.values);
        else if (object->kind == SSRNX_JNI_OBJECT_FLOAT_ARRAY) free(object->u.float_array.values);
        else if (object->kind == SSRNX_JNI_OBJECT_DOUBLE_ARRAY) free(object->u.double_array.values);
        else if (object->kind == SSRNX_JNI_OBJECT_OBJECT_ARRAY) free(object->u.object_array.values);
        else if (object->kind == SSRNX_JNI_OBJECT_EXCEPTION) free(object->u.exception.message);
        if (object->monitor_initialized) {
            pthread_cond_destroy(&object->monitor_cond);
            pthread_mutex_destroy(&object->monitor_lock);
        }
        free(object);
        object = next;
    }
    jni_g_objects = NULL;

    jni_SsrnxJniClass *clazz = jni_g_classes;
    while (clazz) {
        jni_SsrnxJniClass *next_class = clazz->next;
        jni_SsrnxJniMethod *method = clazz->methods;
        while (method) {
            jni_SsrnxJniMethod *next = method->next;
            free(method->name); free(method->signature); free(method); method = next;
        }
        jni_SsrnxJniField *field = clazz->fields;
        while (field) {
            jni_SsrnxJniField *next = field->next;
            free(field->name); free(field->signature); free(field); field = next;
        }
        jni_SsrnxJniNativeBinding *binding = clazz->native_bindings;
        while (binding) {
            jni_SsrnxJniNativeBinding *next = binding->next;
            free(binding->name); free(binding->signature); free(binding); binding = next;
        }
        jni_SsrnxJniInterfaceLink *interface_link = clazz->interfaces;
        while (interface_link) {
            jni_SsrnxJniInterfaceLink *next = interface_link->next;
            free(interface_link);
            interface_link = next;
        }
        while (clazz->initiating_loaders) {
            jni_SsrnxJniInitiatingLoader *link = clazz->initiating_loaders;
            clazz->initiating_loaders = link->next;
            free(link);
        }
        free(clazz->proxy_interfaces);
        free(clazz->name);
        free(clazz);
        clazz = next_class;
    }
    jni_g_classes = NULL;
    jni_class_index_reset_locked();
    jni_g_class_index_disabled = false;
    jni_g_class_count = jni_g_method_count = jni_g_field_count = jni_g_native_binding_count = 0;
    jni_g_instance_field_slot_count = 0;
    jni_g_proxy_class_serial = 0;
}


void *ssrnx_jni_current_env(void) {
    if (!jni_g_initialized || !jni_g_env_key_valid) return NULL;
    jni_SsrnxJniEnvState *env = pthread_getspecific(jni_g_env_key);
    return env ? (void *)&env->handle : NULL;
}

SsrnxResult ssrnx_jni_attach_current_thread_host(void **out_env) {
    if (!out_env || !jni_g_initialized) return SSRNX_ERR_ARGUMENT;
    *out_env = NULL;

    /* Host framework threads use this helper rather than the guest JavaVM
     * table. Keep its already-attached fast path equivalent to vm_attach(). */
    jni_jni_activate_android_tls();

    void *existing = ssrnx_jni_current_env();
    if (existing) { *out_env = existing; return SSRNX_OK; }
    const int32_t rc = jni_vm_attach((jni_SsrnxJavaVM *)&jni_g_vm_handle, out_env, NULL);
    return rc == SSRNX_JNI_OK ? SSRNX_OK : SSRNX_ERR_NO_MEMORY;
}

void ssrnx_jni_detach_current_thread_host(void) {
    if (!jni_g_initialized) return;
    (void)jni_vm_detach((jni_SsrnxJavaVM *)&jni_g_vm_handle);
}

SsrnxResult ssrnx_jni_invoke_method_host(void *env_handle,
                                         void *receiver_ref,
                                         const char *class_name,
                                         const char *method_name,
                                         const char *signature,
                                         bool is_static,
                                         const SsrnxJniValue *args,
                                         SsrnxJniValue *out_result,
                                         bool *out_found) {
    if (!env_handle || !class_name || !method_name || !signature || !out_result || !out_found)
        return SSRNX_ERR_ARGUMENT;
    *out_found = false;
    memset(out_result, 0, sizeof(*out_result));
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env) return SSRNX_ERR_ARGUMENT;

    jni_SsrnxJniClass *clazz = jni_resolve_class_lazy(class_name, 0);
    if (!clazz) return SSRNX_OK;
    /* Host/native callbacks may target real package/core DEX methods. Use the
     * same lazy resolver as JNI GetMethodID so an imported libcore class can
     * call back into Java without requiring a duplicate C registration. */
    jni_SsrnxJniMethod *method = jni_lookup_method(clazz, method_name, signature, is_static);
    if (!method && !is_static && clazz->is_proxy)
        method = jni_proxy_interface_method(receiver_ref, method_name, signature);
    if (!method) return SSRNX_OK;

    *out_found = true;
    *out_result = jni_invoke_method(env_handle, receiver_ref, method, args);
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_has_method_host(const char *class_name,
                                      const char *method_name,
                                      const char *signature,
                                      bool is_static,
                                      bool *out_found) {
    if (!class_name || !method_name || !signature || !out_found)
        return SSRNX_ERR_ARGUMENT;
    *out_found = false;
    jni_SsrnxJniClass *clazz = jni_resolve_class_lazy(class_name, 0);
    if (!clazz) return SSRNX_OK;
    SSRNX_PERF_LOCK(&jni_g_lock);
    *out_found = jni_find_method(clazz, method_name, signature, is_static) != NULL;
    pthread_mutex_unlock(&jni_g_lock);
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_materialize_class_members_host(void *env_handle,
                                                      const char *class_name) {
    if (!env_handle || !class_name || !class_name[0]) return SSRNX_ERR_ARGUMENT;
    jni_SsrnxJniClass *clazz = jni_resolve_class_lazy(class_name, 0u);
    if (!clazz) return SSRNX_ERR_APK_UNSUPPORTED;
    jni_materialize_reflected_classes(env_handle, clazz, true);
    return SSRNX_OK;
}



void *ssrnx_jni_find_class_ref(void *env_handle, const char *name) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || !name) return NULL;
    /* Internal framework/DEX code is already executing in the application
     * runtime and is not subject to guest FindClass caller-loader inference. */
    jni_SsrnxJniClass *clazz = jni_resolve_class_lazy(name, 0u);
    if (clazz && clazz->primitive) clazz = NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = clazz ? jni_new_ref_locked(env, clazz->class_object, SSRNX_JNI_REF_LOCAL) : NULL;
    pthread_mutex_unlock(&jni_g_lock);
    if (!clazz) jni_set_exception(env, "java/lang/ClassNotFoundException", name);
    return ref;
}

SsrnxResult ssrnx_jni_push_application_class_loader_host(void *env_handle) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env) return SSRNX_ERR_ARGUMENT;
    if (env->application_loader_depth == UINT32_MAX) return SSRNX_ERR_ARGUMENT;
    env->application_loader_depth++;
    return SSRNX_OK;
}

void ssrnx_jni_pop_application_class_loader_host(void *env_handle) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (env && env->application_loader_depth) env->application_loader_depth--;
}

SsrnxResult ssrnx_jni_push_local_frame_host(void *env_handle, int32_t capacity) {
    return jni_jni_push_local_frame((jni_SsrnxJNIEnv *)env_handle, capacity) == SSRNX_JNI_OK
               ? SSRNX_OK : SSRNX_ERR_ARGUMENT;
}

void *ssrnx_jni_pop_local_frame_host(void *env_handle, void *result_ref) {
    return jni_jni_pop_local_frame((jni_SsrnxJNIEnv *)env_handle, result_ref);
}

void *ssrnx_jni_get_object_class_host(void *env_handle, void *object_ref) {
    return jni_jni_get_object_class((jni_SsrnxJNIEnv *)env_handle, object_ref);
}

void *ssrnx_jni_new_local_object(void *env_handle, const char *class_name) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || !class_name) return NULL;
    /* Object construction is a hierarchy-sensitive operation. Resolve any
     * provisional manifest/framework class before the instance captures its
     * class descriptor. Reconciliation updates descriptors in place, so older
     * instances also observe the corrected superclass chain. */
    jni_SsrnxJniClass *clazz = jni_resolve_class_lazy(class_name, 0u);
    if (!clazz) {
        jni_set_exception(env, "java/lang/ClassNotFoundException", class_name);
        return NULL;
    }
    jni_SsrnxJniObject *object = jni_new_instance_object(clazz);
    if (!object) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, object, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

void *ssrnx_jni_new_local_string_utf(void *env_handle, const char *text) {
    return jni_jni_new_string_utf((jni_SsrnxJNIEnv *)env_handle, text ? text : "");
}

void *ssrnx_jni_new_local_string_utf16(void *env_handle, const uint16_t *chars, int32_t length) {
    return jni_new_string_from_utf16(env_handle, chars, length);
}

int32_t ssrnx_jni_string_length_host(void *string_ref) {
    jni_SsrnxJniObject *object = jni_object_from_ref(string_ref);
    return object && object->kind == SSRNX_JNI_OBJECT_STRING ? object->u.string.length : -1;
}

uint16_t *ssrnx_jni_string_utf16_dup_host(void *string_ref, int32_t *out_length) {
    jni_SsrnxJniObject *object = jni_object_from_ref(string_ref);
    if (!object || object->kind != SSRNX_JNI_OBJECT_STRING || !out_length) return NULL;
    const int32_t length = object->u.string.length;
    uint16_t *copy = malloc((size_t)(length > 0 ? length : 1) * sizeof(*copy));
    if (!copy) return NULL;
    if (length > 0) memcpy(copy, object->u.string.chars, (size_t)length * sizeof(*copy));
    *out_length = length;
    return copy;
}

bool ssrnx_jni_string_char_at_host(void *string_ref, int32_t index, uint16_t *out_char) {
    if (!out_char || index < 0) return false;
    jni_SsrnxJniObject *object = jni_object_from_ref(string_ref);
    if (!object || object->kind != SSRNX_JNI_OBJECT_STRING || index >= object->u.string.length)
        return false;
    *out_char = object->u.string.chars[index];
    return true;
}

bool ssrnx_jni_class_is_assignable_from_host(void *target_class_ref, void *source_class_ref) {
    jni_SsrnxJniClass *target = jni_class_from_ref(target_class_ref);
    jni_SsrnxJniClass *source = jni_class_from_ref(source_class_ref);
    if (!target || !source) return false;
    if (source->name[0] == '[' || target->name[0] == '[') {
        if (source->name[0] == '[' && target->name[0] == '[')
            return jni_array_descriptor_assignable(source->name, target->name);
        if (source->name[0] == '[')
            return strcmp(target->name, "java/lang/Object") == 0 ||
                   strcmp(target->name, "java/lang/Cloneable") == 0 ||
                   strcmp(target->name, "java/io/Serializable") == 0;
        return false;
    }
    SSRNX_PERF_LOCK(&jni_g_lock);
    bool ok = jni_class_is_assignable_to(source, target) ||
              (source->is_interface && strcmp(target->name, "java/lang/Object") == 0);
    pthread_mutex_unlock(&jni_g_lock);
    if (!ok) ok = jni_class_assignable_name_via_resolver(source->name, target->name);
    return ok;
}

bool ssrnx_jni_class_is_interface_host(void *class_ref) {
    jni_SsrnxJniClass *clazz = jni_class_from_ref(class_ref);
    if (!clazz) return false;
    SSRNX_PERF_LOCK(&jni_g_lock);
    bool result = clazz->is_interface;
    pthread_mutex_unlock(&jni_g_lock);
    /* An interface need not have been linked to an implementing class yet. */
    uint32_t flags = 0;
    if (!result && ssrnx_dex_class_access_flags && ssrnx_dex_class_access_flags(clazz->name, &flags)) {
        result = (flags & UINT32_C(0x0200)) != 0;
        if (result) {
            SSRNX_PERF_LOCK(&jni_g_lock);
            clazz->is_interface = true;
            pthread_mutex_unlock(&jni_g_lock);
        }
    }
    return result;
}

/* Model-owned classes have their direct interface declarations in registration
 * order. DEX-backed Class.getInterfaces uses the class_def order instead. */
void *ssrnx_jni_class_get_interfaces_host(void *env_handle, void *class_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(class_ref);
    if (!env || !clazz) return NULL;
    if (clazz->name[0] == '[') {
        static const char *const names[] = {"java/lang/Cloneable", "java/io/Serializable"};
        void *array = ssrnx_jni_new_local_reference_array(env_handle, "[Ljava/lang/Class;", 2);
        for (int32_t i = 0; array && i < 2; ++i) {
            void *iface = ssrnx_jni_find_class_ref(env_handle, names[i]);
            (void)ssrnx_jni_set_object_array_element_host(env_handle, array, i, iface);
            if (iface) ssrnx_jni_delete_local_ref_host(env_handle, iface);
        }
        return array;
    }
    SSRNX_PERF_LOCK(&jni_g_lock);
    int32_t count = 0;
    for (jni_SsrnxJniInterfaceLink *p = clazz->interfaces; p; p = p->next) ++count;
    void **refs = calloc(count ? (size_t)count : 1u, sizeof(*refs));
    int32_t index = count;
    for (jni_SsrnxJniInterfaceLink *p = clazz->interfaces; refs && p; p = p->next)
        refs[--index] = jni_new_ref_locked(env, p->clazz->class_object, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    if (!refs) { jni_set_exception(env, "java/lang/OutOfMemoryError", "class interfaces"); return NULL; }
    void *array = ssrnx_jni_new_local_reference_array(env_handle, "[Ljava/lang/Class;", count);
    for (int32_t i = 0; i < count; ++i) {
        if (array) (void)ssrnx_jni_set_object_array_element_host(env_handle, array, i, refs[i]);
        if (refs[i]) ssrnx_jni_delete_local_ref_host(env_handle, refs[i]);
    }
    free(refs);
    return array;
}

bool ssrnx_jni_class_uses_application_loader_host(void *class_ref) {
    jni_SsrnxJniClass *clazz = jni_class_from_ref(class_ref);
    if (!clazz) return false;
    SSRNX_PERF_LOCK(&jni_g_lock);
    const bool result = clazz->loader_kind == SSRNX_JNI_LOADER_APPLICATION;
    pthread_mutex_unlock(&jni_g_lock);
    return result;
}

bool ssrnx_jni_class_initiated_by_host(void *class_ref, void *loader_ref) {
    jni_SsrnxJniClass *clazz = jni_class_from_ref(class_ref);
    jni_SsrnxJniObject *loader = jni_object_from_ref(loader_ref);
    if (!clazz || !loader) return false;
    SSRNX_PERF_LOCK(&jni_g_lock);
    bool found = false;
    for (jni_SsrnxJniInitiatingLoader *p = clazz->initiating_loaders; p; p = p->next)
        if (p->loader == loader) { found = true; break; }
    pthread_mutex_unlock(&jni_g_lock);
    return found;
}
SsrnxResult ssrnx_jni_class_record_initiator_host(void *class_ref, void *loader_ref) {
    jni_SsrnxJniClass *clazz = jni_class_from_ref(class_ref);
    jni_SsrnxJniObject *loader = jni_object_from_ref(loader_ref);
    if (!clazz || !loader) return SSRNX_ERR_ARGUMENT;
    SSRNX_PERF_LOCK(&jni_g_lock);
    for (jni_SsrnxJniInitiatingLoader *p = clazz->initiating_loaders; p; p = p->next) {
        if (p->loader == loader) { pthread_mutex_unlock(&jni_g_lock); return SSRNX_OK; }
    }
    jni_SsrnxJniInitiatingLoader *p = calloc(1, sizeof(*p));
    if (p) { p->loader = loader; p->next = clazz->initiating_loaders; clazz->initiating_loaders = p; }
    pthread_mutex_unlock(&jni_g_lock);
    return p ? SSRNX_OK : SSRNX_ERR_NO_MEMORY;
}
SsrnxResult ssrnx_jni_class_set_defining_loader_host(void *class_ref, void *loader_ref) {
    jni_SsrnxJniClass *clazz = jni_class_from_ref(class_ref);
    jni_SsrnxJniObject *loader = jni_object_from_ref(loader_ref);
    if (!clazz || !loader) return SSRNX_ERR_ARGUMENT;
    SSRNX_PERF_LOCK(&jni_g_lock);
    const bool valid = clazz->loader_kind == SSRNX_JNI_LOADER_APPLICATION &&
                        (!clazz->defining_loader || clazz->defining_loader == loader);
    if (valid) clazz->defining_loader = loader;
    pthread_mutex_unlock(&jni_g_lock);
    return valid ? SSRNX_OK : SSRNX_ERR_ARGUMENT;
}
void *ssrnx_jni_class_defining_loader_host(void *env_handle, void *class_ref, void *application_loader) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniClass *clazz = jni_class_from_ref(class_ref);
    jni_SsrnxJniObject *fallback = jni_object_from_ref(application_loader);
    if (!env || !clazz) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniObject *loader = clazz->defining_loader ? clazz->defining_loader :
        clazz->loader_kind == SSRNX_JNI_LOADER_APPLICATION ? fallback : NULL;
    void *ref = loader ? jni_new_ref_locked(env, loader, SSRNX_JNI_REF_LOCAL) : NULL;
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}
void *ssrnx_jni_find_registered_class_host(void *env_handle, const char *name) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || !name) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *clazz = jni_find_class_locked(name);
    void *ref = clazz && !clazz->primitive ? jni_new_ref_locked(env, clazz->class_object, SSRNX_JNI_REF_LOCAL) : NULL;
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}
void *ssrnx_jni_find_boot_class_ref_host(void *env_handle, const char *name) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || !name) return NULL;
    const uint32_t saved = env->application_loader_depth;
    env->application_loader_depth = 0;
    jni_SsrnxJniClass *clazz = jni_jni_find_class_for_context(env, name);
    env->application_loader_depth = saved;
    SSRNX_PERF_LOCK(&jni_g_lock);
    void *ref = clazz && !clazz->primitive ? jni_new_ref_locked(env, clazz->class_object, SSRNX_JNI_REF_LOCAL) : NULL;
    pthread_mutex_unlock(&jni_g_lock);
    if (!clazz) jni_set_exception(env, "java/lang/ClassNotFoundException", name);
    return ref;
}

SsrnxResult ssrnx_jni_string_token_assign_utf16(void *object_token,
                                                 const uint16_t *chars,
                                                 int32_t length) {
    if (!object_token || length < 0 || (length > 0 && !chars)) return SSRNX_ERR_ARGUMENT;
    uint16_t *copy = NULL;
    if (length > 0) {
        copy = malloc((size_t)length * sizeof(*copy));
        if (!copy) return SSRNX_ERR_NO_MEMORY;
        memcpy(copy, chars, (size_t)length * sizeof(*copy));
    }

    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniObject *object = (jni_SsrnxJniObject *)object_token;
    if (!object->clazz || !object->clazz->name ||
        strcmp(object->clazz->name, "java/lang/String") != 0 ||
        (object->kind != SSRNX_JNI_OBJECT_INSTANCE && object->kind != SSRNX_JNI_OBJECT_STRING)) {
        pthread_mutex_unlock(&jni_g_lock);
        free(copy);
        return SSRNX_ERR_ARGUMENT;
    }
    if (object->kind == SSRNX_JNI_OBJECT_INSTANCE) {
        jni_free_field_cells(object);
    } else {
        free(object->u.string.chars);
    }
    memset(&object->u, 0, sizeof(object->u));
    object->kind = SSRNX_JNI_OBJECT_STRING;
    object->u.string.chars = copy;
    object->u.string.length = length;
    pthread_mutex_unlock(&jni_g_lock);
    return SSRNX_OK;
}

void *ssrnx_jni_new_local_ref_host(void *env_handle, void *ref_value) {
    return jni_jni_new_local_ref((jni_SsrnxJNIEnv *)env_handle, ref_value);
}

void *ssrnx_jni_new_local_from_token(void *env_handle, void *object_token) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *object = (jni_SsrnxJniObject *)object_token;
    if (!env || !object) return NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniRef *ref = jni_new_ref_locked(env, object, SSRNX_JNI_REF_LOCAL);
    pthread_mutex_unlock(&jni_g_lock);
    return ref;
}

void *ssrnx_jni_new_global_ref_host(void *env_handle, void *ref_value) {
    return jni_jni_new_global_ref((jni_SsrnxJNIEnv *)env_handle, ref_value);
}

void ssrnx_jni_delete_local_ref_host(void *env_handle, void *ref_value) {
    jni_jni_delete_local_ref((jni_SsrnxJNIEnv *)env_handle, ref_value);
}

void ssrnx_jni_delete_global_ref_host(void *env_handle, void *ref_value) {
    jni_jni_delete_global_ref((jni_SsrnxJNIEnv *)env_handle, ref_value);
}

bool ssrnx_jni_string_utf_copy(void *string_ref, char *out, size_t out_size) {
    if (!out || out_size == 0) return false;
    out[0] = '\0';
    jni_SsrnxJniObject *object = jni_object_from_ref(string_ref);
    if (!object || object->kind != SSRNX_JNI_OBJECT_STRING) return false;
    char *text = jni_mutf8_encode(object->u.string.chars, object->u.string.length);
    if (!text) return false;
    const size_t n = strlen(text);
    if (n >= out_size) { free(text); return false; }
    memcpy(out, text, n + 1);
    free(text);
    return true;
}

char *ssrnx_jni_string_utf_dup(void *string_ref) {
    jni_SsrnxJniObject *object = jni_object_from_ref(string_ref);
    if (!object || object->kind != SSRNX_JNI_OBJECT_STRING) return NULL;
    return jni_mutf8_encode(object->u.string.chars, object->u.string.length);
}

SsrnxResult ssrnx_jni_set_object_host_data(void *object_ref, void *host_data) {
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!object) return SSRNX_ERR_ARGUMENT;
    object->host_data = host_data;
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_set_token_host_data(void *object_token, void *host_data) {
    jni_SsrnxJniObject *object = (jni_SsrnxJniObject *)object_token;
    if (!object) return SSRNX_ERR_ARGUMENT;
    object->host_data = host_data;
    return SSRNX_OK;
}

void *ssrnx_jni_get_object_host_data(void *object_ref) {
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    return object ? object->host_data : NULL;
}

void *ssrnx_jni_get_token_host_data(void *object_token) {
    jni_SsrnxJniObject *object = (jni_SsrnxJniObject *)object_token;
    return object ? object->host_data : NULL;
}

bool ssrnx_jni_same_object_host(void *a_ref, void *b_ref) {
    if (!a_ref || !b_ref) return a_ref == b_ref;
    jni_SsrnxJniObject *a = jni_object_from_ref(a_ref);
    jni_SsrnxJniObject *b = jni_object_from_ref(b_ref);
    return a && b && a == b;
}

int32_t ssrnx_jni_monitor_enter_host(void *env_handle, void *object_ref) {
    return jni_jni_monitor_enter((jni_SsrnxJNIEnv *)env_handle, object_ref);
}

int32_t ssrnx_jni_monitor_enter_timed_host(void *env_handle, void *object_ref, uint32_t timeout_ms) {
    return jni_jni_monitor_enter_timed((jni_SsrnxJNIEnv *)env_handle, object_ref, timeout_ms);
}

int32_t ssrnx_jni_monitor_exit_host(void *env_handle, void *object_ref) {
    return jni_jni_monitor_exit((jni_SsrnxJNIEnv *)env_handle, object_ref);
}

/* Object.clone: arrays copy their elements; an instance of a Cloneable class
 * gets a shallow field-by-field copy. Anything else, and framework model
 * objects whose state lives outside the field model, leaves
 * CloneNotSupportedException pending. */
void *ssrnx_jni_clone_object_host(void *env_handle, void *object_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!env || !object) return NULL;
    void *copy = NULL;
    const int32_t length = ssrnx_jni_array_length_host(object_ref);
    switch (object->kind) {
        case SSRNX_JNI_OBJECT_BOOLEAN_ARRAY: copy = jni_new_boolean_array((jni_SsrnxJNIEnv *)env_handle, length); break;
        case SSRNX_JNI_OBJECT_BYTE_ARRAY: copy = jni_new_byte_array((jni_SsrnxJNIEnv *)env_handle, length); break;
        case SSRNX_JNI_OBJECT_CHAR_ARRAY: copy = jni_new_char_array((jni_SsrnxJNIEnv *)env_handle, length); break;
        case SSRNX_JNI_OBJECT_SHORT_ARRAY: copy = jni_new_short_array((jni_SsrnxJNIEnv *)env_handle, length); break;
        case SSRNX_JNI_OBJECT_INT_ARRAY: copy = jni_new_int_array((jni_SsrnxJNIEnv *)env_handle, length); break;
        case SSRNX_JNI_OBJECT_LONG_ARRAY: copy = jni_new_long_array((jni_SsrnxJNIEnv *)env_handle, length); break;
        case SSRNX_JNI_OBJECT_FLOAT_ARRAY: copy = jni_new_float_array((jni_SsrnxJNIEnv *)env_handle, length); break;
        case SSRNX_JNI_OBJECT_DOUBLE_ARRAY: copy = jni_new_double_array((jni_SsrnxJNIEnv *)env_handle, length); break;
        case SSRNX_JNI_OBJECT_OBJECT_ARRAY: {
            void *component_ref = NULL;
            if (object->u.object_array.component_class && object->u.object_array.component_class->class_object) {
                SSRNX_PERF_LOCK(&jni_g_lock);
                component_ref = jni_new_ref_locked(env, object->u.object_array.component_class->class_object,
                                               SSRNX_JNI_REF_LOCAL);
                pthread_mutex_unlock(&jni_g_lock);
            }
            copy = jni_jni_new_object_array((jni_SsrnxJNIEnv *)env_handle, length, component_ref, NULL);
            jni_jni_delete_local_ref((jni_SsrnxJNIEnv *)env_handle, component_ref);
            break;
        }
        case SSRNX_JNI_OBJECT_INSTANCE: {
            SSRNX_PERF_LOCK(&jni_g_lock);
            jni_SsrnxJniClass *cloneable = jni_find_class_locked("java/lang/Cloneable");
            const bool allowed = !object->host_data && object->clazz && cloneable &&
                                 jni_class_is_assignable_to(object->clazz, cloneable);
            pthread_mutex_unlock(&jni_g_lock);
            if (!allowed) break;
            jni_SsrnxJniObject *clone = jni_new_instance_object(object->clazz);
            if (!clone) break;
            pthread_mutex_t *lock = &jni_g_field_cell_locks[((uintptr_t)object >> 4) & 31u];
            pthread_mutex_lock(lock);
            for (jni_SsrnxJniFieldCell *cell = object->u.instance.cells; cell; cell = cell->next) {
                jni_SsrnxJniFieldCell *dup = calloc(1, sizeof(*dup));
                if (!dup) break;
                *dup = *cell;
                dup->next = clone->u.instance.cells;
                clone->u.instance.cells = dup;
            }
            pthread_mutex_unlock(lock);
            SSRNX_PERF_LOCK(&jni_g_lock);
            copy = jni_new_ref_locked(env, clone, SSRNX_JNI_REF_LOCAL);
            pthread_mutex_unlock(&jni_g_lock);
            break;
        }
        default: break;
    }
    if (copy && object->kind != SSRNX_JNI_OBJECT_INSTANCE && length > 0 &&
        ssrnx_jni_arraycopy_host(object_ref, 0, copy, 0, length) != SSRNX_OK) {
        jni_jni_delete_local_ref((jni_SsrnxJNIEnv *)env_handle, copy);
        copy = NULL;
    }
    if (!copy && !env->pending_exception)
        jni_set_exception(env, "java/lang/CloneNotSupportedException", object->clazz ? object->clazz->name : "");
    return copy;
}

int32_t ssrnx_jni_monitor_wait_host(void *env_handle, void *object_ref, int64_t timeout_ms, int32_t nanos) {
    return jni_jni_monitor_wait((jni_SsrnxJNIEnv *)env_handle, object_ref, timeout_ms, nanos);
}

int32_t ssrnx_jni_monitor_notify_host(void *env_handle, void *object_ref, bool notify_all) {
    return jni_jni_monitor_notify((jni_SsrnxJNIEnv *)env_handle, object_ref, notify_all);
}

bool ssrnx_jni_monitor_held_by_current_host(void *object_ref) {
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!object || !object->monitor_initialized) return false;
    if (pthread_mutex_lock(&object->monitor_lock) != 0) return false;
    const bool held = object->monitor_owner_valid && object->monitor_depth != 0u &&
                      pthread_equal(object->monitor_owner, pthread_self());
    pthread_mutex_unlock(&object->monitor_lock);
    return held;
}

bool ssrnx_jni_ref_values_identity(uint64_t a, uint64_t b, bool *out_same) {
    if (out_same) *out_same = false;
    SSRNX_PERF_LOCK(&jni_g_lock);
    const bool refs = jni_ref_set_contains_locked((uintptr_t)a) && jni_ref_set_contains_locked((uintptr_t)b);
    if (refs && out_same) {
        const jni_SsrnxJniRef *ra = (const jni_SsrnxJniRef *)(uintptr_t)a;
        const jni_SsrnxJniRef *rb = (const jni_SsrnxJniRef *)(uintptr_t)b;
        /* Reference equality is identity of the referent (JLS 15.21.3). A
         * deleted/out-of-scope ref no longer names an object, so it is only
         * equal to itself. */
        *out_same = a == b || (ra->alive && rb->alive && ra->object == rb->object);
    }
    pthread_mutex_unlock(&jni_g_lock);
    return refs;
}

void *ssrnx_jni_object_token_host(void *object_ref) {
    return jni_object_from_ref(object_ref);
}

bool ssrnx_jni_object_class_name_host(void *object_ref, char *out, size_t out_size) {
    if (!out || out_size == 0) return false;
    out[0] = '\0';
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!object || !object->clazz || !object->clazz->name) return false;
    const size_t n = strlen(object->clazz->name);
    if (n + 1u > out_size) return false;
    memcpy(out, object->clazz->name, n + 1u);
    return true;
}

bool ssrnx_jni_class_ref_name_host(void *class_ref, char *out, size_t out_size) {
    if (!out || out_size == 0) return false;
    out[0] = '\0';
    jni_SsrnxJniClass *clazz = jni_class_from_ref(class_ref);
    if (!clazz || !clazz->name) return false;
    const size_t n = strlen(clazz->name);
    if (n + 1u > out_size) return false;
    memcpy(out, clazz->name, n + 1u);
    return true;
}

bool ssrnx_jni_exception_message_host(void *throwable_ref, char *out, size_t out_size) {
    if (!out || out_size == 0) return false;
    out[0] = '\0';
    jni_SsrnxJniObject *object = jni_object_from_ref(throwable_ref);
    if (!object || object->kind != SSRNX_JNI_OBJECT_EXCEPTION) return false;
    const char *message = object->u.exception.message ? object->u.exception.message : "";
    const size_t n = strlen(message);
    if (n + 1u > out_size) return false;
    memcpy(out, message, n + 1u);
    return true;
}

static SsrnxResult jni_set_field_common(jni_SsrnxJniObject *object, jni_SsrnxJniClass *clazz,
                                    const char *name, const char *signature,
                                    SsrnxJniValue value, bool is_static) {
    if (!clazz || !name || !signature) return SSRNX_ERR_ARGUMENT;
    jni_SsrnxJniField *field = jni_find_field(clazz, name, signature, is_static);
    if (!field) return SSRNX_ERR_ARGUMENT;
    if (signature[0] == 'L' || signature[0] == '[') {
        value.l = jni_object_from_ref(value.l);
    }
    if (is_static) {
        field->static_value = value;
        return SSRNX_OK;
    }
    SsrnxJniValue *slot = jni_instance_field_value(object, field);
    if (!slot) return SSRNX_ERR_ARGUMENT;
    *slot = value;
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_set_instance_field_value(void *object_ref, const char *name,
                                                const char *signature, SsrnxJniValue value) {
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    return object ? jni_set_field_common(object, object->clazz, name, signature, value, false)
                  : SSRNX_ERR_ARGUMENT;
}

SsrnxResult ssrnx_jni_get_instance_field_value(void *env_handle, void *object_ref,
                                                const char *class_name, const char *name,
                                                const char *signature, SsrnxJniValue *out_value,
                                                bool *out_found) {
    if (!env_handle || !object_ref || !class_name || !name || !signature ||
        !out_value || !out_found) return SSRNX_ERR_ARGUMENT;
    *out_found = false;
    memset(out_value, 0, sizeof(*out_value));
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!env || !object) return SSRNX_ERR_ARGUMENT;

    jni_SsrnxJniClass *owner = jni_resolve_class_lazy(class_name, 0);
    if (!owner) return SSRNX_OK;

    /* Resolve declarations before accepting an inherited field. Otherwise a
     * lazily loaded subclass's hidden field aliases its superclass's slot.
     * GetFieldID already uses this same member materializer. */
    SSRNX_PERF_LOCK(&jni_g_lock);
    if (!owner->members_materialized && jni_g_member_materializer) {
        pthread_mutex_unlock(&jni_g_lock);
        jni_materialize_reflected_classes(env_handle, owner, false);
        SSRNX_PERF_LOCK(&jni_g_lock);
    }
    if (!jni_class_is_assignable_to(object->clazz, owner)) {
        char actual_name[512] = {0};
        if (object->clazz && object->clazz->name)
            (void)snprintf(actual_name, sizeof(actual_name), "%s", object->clazz->name);
        pthread_mutex_unlock(&jni_g_lock);
        if (!jni_class_inherits_name_via_resolver(actual_name, class_name)) {
            SSRNX_LOGCW("JNI", "instance field owner mismatch actual=%s owner=%s field=%s sig=%s",
                        actual_name[0] ? actual_name : "<none>", class_name, name, signature);
            return SSRNX_OK;
        }
        SSRNX_PERF_LOCK(&jni_g_lock);
    }
    jni_SsrnxJniField *field = jni_find_field(owner, name, signature, false);
    if (!field) {
        pthread_mutex_unlock(&jni_g_lock);
        return SSRNX_OK;
    }
    SsrnxJniValue *slot = jni_instance_field_value_compatible(object, field, true);
    if (!slot) {
        pthread_mutex_unlock(&jni_g_lock);
        return SSRNX_ERR_ARGUMENT;
    }
    *out_found = true;
    *out_value = *slot;
    if ((signature[0] == 'L' || signature[0] == '[') && slot->l) {
        jni_SsrnxJniRef *ref = jni_new_ref_locked(env, (jni_SsrnxJniObject *)slot->l, SSRNX_JNI_REF_LOCAL);
        if (!ref) {
            pthread_mutex_unlock(&jni_g_lock);
            return SSRNX_ERR_NO_MEMORY;
        }
        out_value->l = ref;
    }
    pthread_mutex_unlock(&jni_g_lock);
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_set_instance_field_value_for_class(void *object_ref,
                                                          const char *class_name,
                                                          const char *name,
                                                          const char *signature,
                                                          SsrnxJniValue value,
                                                          bool *out_found) {
    if (!object_ref || !class_name || !name || !signature || !out_found)
        return SSRNX_ERR_ARGUMENT;
    *out_found = false;
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!object) return SSRNX_ERR_ARGUMENT;
    jni_SsrnxJniClass *owner = jni_resolve_class_lazy(class_name, 0);
    if (!owner) return SSRNX_OK;

    /* Same declaration-before-inheritance rule as the qualified getter. */
    SSRNX_PERF_LOCK(&jni_g_lock);
    if (!owner->members_materialized && jni_g_member_materializer) {
        pthread_mutex_unlock(&jni_g_lock);
        jni_materialize_reflected_classes(ssrnx_jni_current_env(), owner, false);
        SSRNX_PERF_LOCK(&jni_g_lock);
    }
    if (!jni_class_is_assignable_to(object->clazz, owner)) {
        char actual_name[512] = {0};
        if (object->clazz && object->clazz->name)
            (void)snprintf(actual_name, sizeof(actual_name), "%s", object->clazz->name);
        pthread_mutex_unlock(&jni_g_lock);
        if (!jni_class_inherits_name_via_resolver(actual_name, class_name)) {
            SSRNX_LOGCW("JNI", "instance field owner mismatch actual=%s owner=%s field=%s sig=%s",
                        actual_name[0] ? actual_name : "<none>", class_name, name, signature);
            return SSRNX_OK;
        }
        SSRNX_PERF_LOCK(&jni_g_lock);
    }
    jni_SsrnxJniField *field = jni_find_field(owner, name, signature, false);
    if (!field) {
        pthread_mutex_unlock(&jni_g_lock);
        return SSRNX_OK;
    }
    if (signature[0] == 'L' || signature[0] == '[') value.l = jni_object_from_ref(value.l);
    SsrnxJniValue *slot = jni_instance_field_value_compatible(object, field, true);
    if (!slot) {
        pthread_mutex_unlock(&jni_g_lock);
        return SSRNX_ERR_ARGUMENT;
    }
    *slot = value;
    *out_found = true;
    pthread_mutex_unlock(&jni_g_lock);
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_set_static_field_value(const char *class_name, const char *name,
                                              const char *signature, SsrnxJniValue value) {
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *clazz = jni_find_class_locked(class_name ? class_name : "");
    SsrnxResult rc = clazz ? jni_set_field_common(NULL, clazz, name, signature, value, true)
                           : SSRNX_ERR_ARGUMENT;
    pthread_mutex_unlock(&jni_g_lock);
    return rc;
}

SsrnxResult ssrnx_jni_get_static_field_value(void *env_handle, const char *class_name,
                                              const char *name, const char *signature,
                                              SsrnxJniValue *out_value, bool *out_found) {
    if (!env_handle || !class_name || !name || !signature || !out_value || !out_found)
        return SSRNX_ERR_ARGUMENT;
    *out_found = false;
    memset(out_value, 0, sizeof(*out_value));
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env) return SSRNX_ERR_ARGUMENT;

    jni_SsrnxJniClass *clazz = jni_resolve_class_lazy(class_name, 0);
    if (!clazz) return SSRNX_OK;

    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniField *field = jni_find_field(clazz, name, signature, true);
    if (!field) {
        pthread_mutex_unlock(&jni_g_lock);
        return SSRNX_OK;
    }
    *out_found = true;
    *out_value = field->static_value;
    if ((signature[0] == 'L' || signature[0] == '[') && field->static_value.l) {
        jni_SsrnxJniRef *ref = jni_new_ref_locked(env, (jni_SsrnxJniObject *)field->static_value.l, SSRNX_JNI_REF_LOCAL);
        if (!ref) {
            pthread_mutex_unlock(&jni_g_lock);
            return SSRNX_ERR_NO_MEMORY;
        }
        out_value->l = ref;
    }
    pthread_mutex_unlock(&jni_g_lock);
    return SSRNX_OK;
}

int32_t ssrnx_jni_throw_new_by_name(void *env_handle, const char *class_name,
                                    const char *message) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    if (!env || !class_name) return SSRNX_JNI_ERR;
    /* FindClass semantics: throwables defined by DEX code (app or the core
     * library) are resolved through the class resolver, not only models. */
    jni_SsrnxJniClass *clazz = jni_resolve_class_lazy(class_name, 0);
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniObject *object = clazz ? jni_alloc_object_locked(SSRNX_JNI_OBJECT_EXCEPTION, clazz) : NULL;
    if (object) object->u.exception.message = jni_dup_string(message ? message : "");
    pthread_mutex_unlock(&jni_g_lock);
    if (!object) return SSRNX_JNI_ERR;
    env->pending_exception = object;
    return SSRNX_JNI_OK;
}


bool ssrnx_jni_exception_pending_host(void *env_handle) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    return env && env->pending_exception;
}

void *ssrnx_jni_exception_occurred_host(void *env_handle) {
    return jni_jni_exception_occurred((jni_SsrnxJNIEnv *)env_handle);
}

void ssrnx_jni_exception_clear_host(void *env_handle) {
    jni_jni_exception_clear((jni_SsrnxJNIEnv *)env_handle);
}

SsrnxResult ssrnx_jni_throw_host(void *env_handle, void *throwable_ref) {
    return jni_jni_throw((jni_SsrnxJNIEnv *)env_handle, throwable_ref) == SSRNX_JNI_OK
               ? SSRNX_OK : SSRNX_ERR_ARGUMENT;
}

static size_t jni_link_declared_platform_interfaces(const char *class_name);

bool ssrnx_jni_is_instance_of_class_host(void *object_ref, const char *class_name) {
    if (!object_ref || !class_name || !class_name[0]) return false;
    jni_SsrnxJniObject *object = jni_object_from_ref(object_ref);
    if (!object || !object->clazz) return false;
    if (class_name[0] == '[') return jni_array_object_is_instance(object, class_name);
    jni_SsrnxJniClass *wanted = jni_resolve_class_lazy(class_name, 0u);
    if (!wanted) return false;
    /* Use the same Java assignment rules as JNI IsInstanceOf. In particular,
     * every array type is assignable to Object, Cloneable and Serializable. */
    if (jni_object_is_assignable_to_class(object, wanted)) return true;
    if (!wanted->is_interface || object->clazz->name[0] == '[') return false;
    /* The startup link pass only sees boot classes that exist and are final at
     * that point. Link the declared platform interfaces of every boot class in
     * this object's superclass chain on first use, then re-check. */
    char chain[8][512];
    size_t chain_count = 0;
    SSRNX_PERF_LOCK(&jni_g_lock);
    for (jni_SsrnxJniClass *c = object->clazz; c && chain_count < 8u; c = c->super) {
        if (c->loader_kind != SSRNX_JNI_LOADER_BOOT || c->is_interface ||
            c->platform_interfaces_checked) continue;
        c->platform_interfaces_checked = true;
        snprintf(chain[chain_count++], sizeof(chain[0]), "%s", c->name);
    }
    pthread_mutex_unlock(&jni_g_lock);
    size_t linked = 0;
    for (size_t i = 0; i < chain_count; ++i) linked += jni_link_declared_platform_interfaces(chain[i]);
    const bool ok = linked && jni_object_is_assignable_to_class(object, wanted);
    static uint32_t reports;
    if (chain_count && __atomic_fetch_add(&reports, 1u, __ATOMIC_RELAXED) < 16u)
        SSRNX_LOGCI("JNI", "platform interface on-demand link class=%s wanted=%s wantedLoader=%d linked=%zu result=%d",
                    object->clazz->name, class_name, (int)wanted->loader_kind, linked, ok ? 1 : 0);
    return ok;
}

void ssrnx_jni_visit_registry(SsrnxJniRegistryVisitor visitor, void *user_data) {
    if (!visitor || !jni_g_initialized) return;
    SSRNX_PERF_LOCK(&jni_g_lock);
    for (jni_SsrnxJniClass *c = jni_g_classes; c; c = c->next) {
        if (c->primitive) continue;
        visitor(c->name, c->super ? c->super->name : NULL, NULL, NULL, c->is_interface, user_data);
        for (jni_SsrnxJniInterfaceLink *link = c->interfaces; link; link = link->next)
            visitor(c->name, link->clazz ? link->clazz->name : NULL, "<implements>", "", false, user_data);
        for (jni_SsrnxJniMethod *m = c->methods; m; m = m->next)
            visitor(c->name, NULL, m->name, m->signature, m->is_static, user_data);
    }
    pthread_mutex_unlock(&jni_g_lock);
}

/* Platform classes implement every interface their API declaration lists
 * (Messenger is Parcelable, Thread is Runnable, File is Comparable), but host
 * models and trimmed core DEX images name only the ones they needed. Link the
 * declared interfaces of class_name and of its declared API superclasses so
 * check-cast/instanceof/IsAssignableFrom follow the platform type graph. An
 * interface SSRNX models as a plain, directly instantiated host class is left
 * alone so its object model is unchanged. Returns links added. */
static size_t jni_link_declared_platform_interfaces(const char *class_name) {
    if (!ssrnx_api_declared_supertypes || !class_name) return 0;
    size_t linked = 0;
    char cur[512];
    snprintf(cur, sizeof(cur), "%s", class_name);
    for (unsigned walk = 0; walk < 32u && cur[0]; ++walk) {
        char super_name[512] = {0}, interfaces[1024] = {0};
        bool is_interface = false;
        if (!ssrnx_api_declared_supertypes(cur, super_name, sizeof(super_name), interfaces,
                                           sizeof(interfaces), &is_interface) || is_interface)
            break;
        for (char *p = interfaces; *p;) {
            char *end = strchr(p, '\n');
            if (end) *end = '\0';
            jni_SsrnxJniClass *iface = jni_resolve_class_lazy(p, 1u);
            SSRNX_PERF_LOCK(&jni_g_lock);
            jni_SsrnxJniClass *clazz = jni_find_class_locked(class_name);
            const bool eligible = iface && clazz && clazz != iface &&
                                  (iface->is_interface || iface->platform_inert) &&
                                  !jni_class_is_assignable_to(clazz, iface);
            pthread_mutex_unlock(&jni_g_lock);
            if (eligible && ssrnx_jni_register_interface(class_name, p) == SSRNX_OK) ++linked;
            if (!end) break;
            p = end + 1;
        }
        if (!super_name[0] || strcmp(super_name, cur) == 0) break;
        snprintf(cur, sizeof(cur), "%s", super_name);
    }
    return linked;
}

static bool jni_platform_type_graph_candidate_locked(const jni_SsrnxJniClass *c) {
    return c && !c->primitive && !c->platform_inert && !c->hierarchy_provisional && !c->is_interface &&
           c->loader_kind == SSRNX_JNI_LOADER_BOOT && c->name[0] != '[';
}

size_t ssrnx_jni_link_platform_api_interfaces(void) {
    if (!jni_g_initialized || !ssrnx_api_declared_supertypes) return 0;
    size_t count = 0, cap = 0;
    char **names = NULL;
    SSRNX_PERF_LOCK(&jni_g_lock);
    for (jni_SsrnxJniClass *c = jni_g_classes; c; c = c->next) {
        if (!jni_platform_type_graph_candidate_locked(c)) continue;
        if (count == cap) {
            const size_t next_cap = cap ? cap * 2u : 256u;
            char **grown = realloc(names, next_cap * sizeof(*names));
            if (!grown) break;
            names = grown;
            cap = next_cap;
        }
        if (!(names[count] = jni_dup_string(c->name))) break;
        ++count;
    }
    pthread_mutex_unlock(&jni_g_lock);
    size_t linked = 0;
    for (size_t i = 0; i < count; ++i) {
        linked += jni_link_declared_platform_interfaces(names[i]);
        free(names[i]);
    }
    free(names);
    if (linked) SSRNX_LOGCI("JNI", "platform API interfaces linked onto boot classes count=%zu", linked);
    return linked;
}

void ssrnx_jni_visit_registry_fields(SsrnxJniFieldVisitor visitor, void *user_data) {
    if (!visitor || !jni_g_initialized) return;
    SSRNX_PERF_LOCK(&jni_g_lock);
    for (jni_SsrnxJniClass *c = jni_g_classes; c; c = c->next) {
        if (c->primitive) continue;
        for (jni_SsrnxJniField *f = c->fields; f; f = f->next)
            visitor(c->name, f->name, f->signature, f->is_static, user_data);
    }
    pthread_mutex_unlock(&jni_g_lock);
}

SsrnxResult ssrnx_jni_init(void) {
    if (jni_g_initialized) return SSRNX_OK;
    ssrnx_jni_clear_process_exit();
    jni_setup_tables();
    if (pthread_key_create(&jni_g_env_key, jni_env_key_destructor) != 0) return SSRNX_ERR_NO_MEMORY;
    jni_g_env_key_valid = true;
    jni_g_initialized = true;

    const char *builtins[][2] = {
        {"java/lang/Object", NULL},
        {"java/lang/Class", "java/lang/Object"},
        {"java/lang/String", "java/lang/Object"},
        {"java/lang/Cloneable", "java/lang/Object"},
        {"java/io/Serializable", "java/lang/Object"},
        {"java/lang/Comparable", "java/lang/Object"},
        {"java/lang/Enum", "java/lang/Object"},
        {"java/lang/Throwable", "java/lang/Object"},
        {"java/lang/Exception", "java/lang/Throwable"},
        {"java/lang/RuntimeException", "java/lang/Exception"},
        /* Superclasses follow java.lang so DEX catch clauses match as on Android. */
        {"java/lang/Error", "java/lang/Throwable"},
        {"java/lang/VirtualMachineError", "java/lang/Error"},
        {"java/lang/OutOfMemoryError", "java/lang/VirtualMachineError"},
        {"java/lang/LinkageError", "java/lang/Error"},
        {"java/lang/ClassFormatError", "java/lang/LinkageError"},
        {"java/lang/ExceptionInInitializerError", "java/lang/LinkageError"},
        {"java/lang/IncompatibleClassChangeError", "java/lang/LinkageError"},
        {"java/lang/ClassCastException", "java/lang/RuntimeException"},
        {"java/lang/ReflectiveOperationException", "java/lang/Exception"},
        {"java/lang/ClassNotFoundException", "java/lang/ReflectiveOperationException"},
        {"java/lang/NoSuchMethodError", "java/lang/IncompatibleClassChangeError"},
        {"java/lang/NoSuchFieldError", "java/lang/IncompatibleClassChangeError"},
        {"java/lang/NoClassDefFoundError", "java/lang/LinkageError"},
        {"java/lang/UnsatisfiedLinkError", "java/lang/LinkageError"},
        {"java/lang/IndexOutOfBoundsException", "java/lang/RuntimeException"},
        {"java/lang/ArrayIndexOutOfBoundsException", "java/lang/IndexOutOfBoundsException"},
        {"java/lang/StringIndexOutOfBoundsException", "java/lang/IndexOutOfBoundsException"},
        {"java/lang/NullPointerException", "java/lang/RuntimeException"},
        {"java/lang/NegativeArraySizeException", "java/lang/RuntimeException"},
        {"java/lang/ArrayStoreException", "java/lang/RuntimeException"},
        {"java/lang/ArithmeticException", "java/lang/RuntimeException"},
        {"java/lang/IllegalMonitorStateException", "java/lang/RuntimeException"},
    };
    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); ++i) {
        SsrnxResult rc = jni_register_class_internal(builtins[i][0], builtins[i][1]);
        if (rc != SSRNX_OK) { ssrnx_jni_reset(); return rc; }
    }

    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *cloneable_class = jni_find_class_locked("java/lang/Cloneable");
    jni_SsrnxJniClass *serializable_class = jni_find_class_locked("java/io/Serializable");
    jni_SsrnxJniClass *comparable_class = jni_find_class_locked("java/lang/Comparable");
    if (cloneable_class) cloneable_class->is_interface = true;
    if (serializable_class) serializable_class->is_interface = true;
    if (comparable_class) comparable_class->is_interface = true;
    pthread_mutex_unlock(&jni_g_lock);

    /* java.lang.Enum implements Comparable<E> and Serializable.  Keep these
     * core java.lang type relationships in the Phase-5 VM model so DEX
     * check-cast/instance-of work before the higher framework registration
     * layer has run. */
    SsrnxResult core_link_rc = ssrnx_jni_register_interface("java/lang/Enum", "java/lang/Comparable");
    if (core_link_rc == SSRNX_OK)
        core_link_rc = ssrnx_jni_register_interface("java/lang/Enum", "java/io/Serializable");
    /* Android libcore java.lang.Class implements java.io.Serializable. Class
     * literals are ordinary Class objects and must pass Serializable casts. */
    if (core_link_rc == SSRNX_OK)
        core_link_rc = ssrnx_jni_register_interface("java/lang/Class", "java/io/Serializable");
    if (core_link_rc != SSRNX_OK) {
        ssrnx_jni_reset();
        return core_link_rc;
    }

    /* Class objects themselves are instances of java/lang/Class. */
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *class_class = jni_find_class_locked("java/lang/Class");
    for (jni_SsrnxJniClass *clazz = jni_g_classes; clazz; clazz = clazz->next) {
        clazz->class_object->clazz = class_class;
    }
    pthread_mutex_unlock(&jni_g_lock);

    void *env = NULL;
    const int32_t attach_rc = jni_vm_attach((jni_SsrnxJavaVM *)&jni_g_vm_handle, &env, NULL);
    if (attach_rc != SSRNX_JNI_OK) { ssrnx_jni_reset(); return SSRNX_ERR_NO_MEMORY; }
    SSRNX_LOGCI("JNI", "JavaVM initialized version=0x%08x vm=%p env=%p",
                SSRNX_JNI_VERSION_1_6, (void *)&jni_g_vm_handle, env);
    return SSRNX_OK;
}

void ssrnx_jni_reset(void) {
    if (!jni_g_initialized) { jni_g_class_resolver = NULL; jni_g_class_resolver_user = NULL; jni_g_class_resolver_ex = NULL; jni_g_class_resolver_ex_user = NULL; jni_g_method_resolver = NULL; jni_g_method_resolver_user = NULL; jni_g_member_materializer = NULL; jni_g_member_materializer_user = NULL; jni_g_class_initializer = NULL; jni_g_class_initializer_user = NULL; return; }
    if (jni_g_env_key_valid) {
        jni_SsrnxJniEnvState *env = pthread_getspecific(jni_g_env_key);
        if (env) {
            (void)pthread_setspecific(jni_g_env_key, NULL);
            jni_env_destroy(env);
        }
        (void)pthread_key_delete(jni_g_env_key);
        jni_g_env_key_valid = false;
    }
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_free_model();
    pthread_mutex_unlock(&jni_g_lock);
    jni_g_class_resolver = NULL;
    jni_g_class_resolver_user = NULL;
    jni_g_class_resolver_ex = NULL;
    jni_g_class_resolver_ex_user = NULL;
    jni_g_method_resolver = NULL;
    jni_g_member_materializer = NULL;
    jni_g_member_materializer_user = NULL;
    jni_g_class_initializer = NULL;
    jni_g_class_initializer_user = NULL;
    jni_g_method_resolver_user = NULL;
    jni_g_initialized = false;
}

void *ssrnx_jni_vm(void) {
    return jni_g_initialized ? (void *)&jni_g_vm_handle : NULL;
}

void ssrnx_jni_set_class_resolver(SsrnxJniClassResolver resolver, void *user_data) {
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_g_class_resolver = resolver;
    jni_g_class_resolver_user = user_data;
    pthread_mutex_unlock(&jni_g_lock);
}

void ssrnx_jni_set_class_resolver_ex(SsrnxJniClassResolverEx resolver, void *user_data) {
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_g_class_resolver_ex = resolver;
    jni_g_class_resolver_ex_user = user_data;
    pthread_mutex_unlock(&jni_g_lock);
}

void ssrnx_jni_set_method_resolver(SsrnxJniMethodResolver resolver, void *user_data) {
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_g_method_resolver = resolver;
    jni_g_method_resolver_user = user_data;
    pthread_mutex_unlock(&jni_g_lock);
}

void ssrnx_jni_set_class_member_materializer(SsrnxJniClassMemberMaterializer materializer,
                                             void *user_data) {
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_g_member_materializer = materializer;
    jni_g_member_materializer_user = user_data;
    pthread_mutex_unlock(&jni_g_lock);
}

void ssrnx_jni_set_class_initializer(SsrnxJniClassInitializer initializer,
                                     void *user_data) {
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_g_class_initializer = initializer;
    jni_g_class_initializer_user = user_data;
    pthread_mutex_unlock(&jni_g_lock);
}

bool ssrnx_jni_class_is_platform_inert_host(const char *class_name) {
    jni_SsrnxJniClass *clazz = jni_resolve_class_lazy(class_name, 0u);
    if (!clazz) return false;
    SSRNX_PERF_LOCK(&jni_g_lock);
    const bool inert = clazz->platform_inert;
    pthread_mutex_unlock(&jni_g_lock);
    return inert;
}

bool ssrnx_jni_class_resolves_member_host(const char *class_name, const char *name,
                                          const char *signature, bool is_static, bool is_method) {
    if (!class_name || !name || !signature) return false;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *clazz = jni_find_class_locked(class_name);
    const bool resolved = clazz && (is_method ? jni_find_method(clazz, name, signature, is_static) != NULL
                                              : jni_find_field(clazz, name, signature, is_static) != NULL);
    pthread_mutex_unlock(&jni_g_lock);
    return resolved;
}

bool ssrnx_jni_class_declares_member_host(const char *class_name, const char *name,
                                          const char *signature, bool is_static, bool is_method) {
    if (!class_name || !name || !signature) return false;
    bool declared = false;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *clazz = jni_find_class_locked(class_name);
    if (clazz && is_method) {
        declared = jni_find_declared_method(clazz, name, signature, is_static) != NULL;
    } else if (clazz) {
        for (jni_SsrnxJniField *f = clazz->fields; f && !declared; f = f->next)
            declared = f->is_static == is_static && strcmp(f->name, name) == 0 &&
                       strcmp(f->signature, signature) == 0;
    }
    pthread_mutex_unlock(&jni_g_lock);
    return declared;
}

typedef struct jni_SsrnxJniModelledMethod {
    const char *name;
    const char *signature;
    SsrnxJniMethodCallback callback;
    void *user_data;
} jni_SsrnxJniModelledMethod;

/* Collects the implemented (non-abstract) instance methods of the interfaces
 * reachable from iface that class resolution (find_method) binds to an
 * interface method. Requires g_lock. */
static bool jni_collect_modelled_interface_methods(jni_SsrnxJniClass *clazz, jni_SsrnxJniClass *iface, unsigned depth,
                                               jni_SsrnxJniModelledMethod **items, size_t *count, size_t *cap) {
    if (!iface || depth > 64u) return true;
    for (jni_SsrnxJniMethod *m = iface->methods; m; m = m->next) {
        if (m->is_static || m->name[0] == '<' || !m->callback || m->callback == jni_interface_method_dispatch) continue;
        jni_SsrnxJniMethod *bound = jni_find_method(clazz, m->name, m->signature, false);
        if (!bound || !bound->owner || !bound->owner->is_interface || !bound->callback ||
            bound->callback == jni_interface_method_dispatch) continue;
        bool seen = false;
        for (size_t i = 0; i < *count && !seen; ++i)
            seen = strcmp((*items)[i].name, m->name) == 0 && strcmp((*items)[i].signature, m->signature) == 0;
        if (seen) continue;
        if (*count == *cap) {
            const size_t next = *cap ? *cap * 2u : 16u;
            jni_SsrnxJniModelledMethod *grown = realloc(*items, next * sizeof(*grown));
            if (!grown) return false;
            *items = grown;
            *cap = next;
        }
        (*items)[(*count)++] = (jni_SsrnxJniModelledMethod){bound->name, bound->signature, bound->callback, bound->user_data};
    }
    for (jni_SsrnxJniInterfaceLink *link = iface->interfaces; link; link = link->next)
        if (!jni_collect_modelled_interface_methods(clazz, link->clazz, depth + 1u, items, count, cap)) return false;
    return true;
}

/* A framework-modelled class (one without a DEX class_def) gets most of its
 * behaviour from interface registrations: java/util/Collection.add serves
 * every modelled collection. The platform class declares those methods
 * itself, and reflection callers such as Unity's ReflectionHelper look for
 * them only in getDeclaredMethods() of the class and its superclasses. Declare
 * on the class every interface method of its implemented interfaces that
 * method resolution currently binds to an interface implementation, with that
 * same callback, so dispatch is unchanged. Methods the class chain declares,
 * abstract interface methods and interface static methods are left alone. */
SsrnxResult ssrnx_jni_declare_modelled_interface_methods_host(const char *class_name) {
    if (!class_name || class_name[0] == '[') return SSRNX_OK;
    jni_SsrnxJniModelledMethod *items = NULL;
    size_t count = 0, cap = 0;
    bool ok = true;
    SSRNX_PERF_LOCK(&jni_g_lock);
    jni_SsrnxJniClass *clazz = jni_find_class_locked(class_name);
    if (clazz && !clazz->is_interface && !clazz->primitive && !clazz->is_proxy && !clazz->platform_inert) {
        for (jni_SsrnxJniClass *c = clazz; c && ok; c = c->super)
            for (jni_SsrnxJniInterfaceLink *link = c->interfaces; link && ok; link = link->next)
                ok = jni_collect_modelled_interface_methods(clazz, link->clazz, 0u, &items, &count, &cap);
    }
    pthread_mutex_unlock(&jni_g_lock);
    SsrnxResult rc = ok ? SSRNX_OK : SSRNX_ERR_NO_MEMORY;
    for (size_t i = 0; i < count && rc == SSRNX_OK; ++i)
        rc = ssrnx_jni_register_method(class_name, items[i].name, items[i].signature, false,
                                       items[i].callback, items[i].user_data);
    free(items);
    return rc;
}

extern bool ssrnx_core_native_lookup(const char *class_name, const char *name,
                                     const char *signature, void **out_fn) __attribute__((weak));

SsrnxResult ssrnx_jni_find_native_binding(const char *class_name, const char *name,
                                           const char *signature, void **out_fn,
                                           bool *out_found) {
    if (!class_name || !name || !signature || !out_fn || !out_found)
        return SSRNX_ERR_ARGUMENT;
    *out_fn = NULL;
    *out_found = false;
    /* Natives of the SSRNX core library are bound by the runtime itself, as
     * ART's libjavacore registers libcore natives at boot. */
    if (ssrnx_core_native_lookup && ssrnx_core_native_lookup(class_name, name, signature, out_fn)) {
        *out_found = *out_fn != NULL;
        return SSRNX_OK;
    }
    jni_SsrnxJniClass *clazz = jni_resolve_class_lazy(class_name, 0);
    if (!clazz) return SSRNX_OK;
    SSRNX_PERF_LOCK(&jni_g_lock);
    for (jni_SsrnxJniNativeBinding *binding = clazz->native_bindings; binding; binding = binding->next) {
        if (strcmp(binding->name, name) == 0 && strcmp(binding->signature, signature) == 0) {
            *out_fn = binding->fn_ptr;
            *out_found = binding->fn_ptr != NULL;
            break;
        }
    }
    pthread_mutex_unlock(&jni_g_lock);
    return SSRNX_OK;
}

size_t ssrnx_jni_native_binding_count(void) { return jni_g_native_binding_count; }
size_t ssrnx_jni_class_count(void) { return jni_g_class_count; }
size_t ssrnx_jni_method_count(void) { return jni_g_method_count; }
size_t ssrnx_jni_field_count(void) { return jni_g_field_count; }

void *ssrnx_jni_new_local_object_array(void *env_handle, int32_t length) {
    return jni_jni_new_object_array((jni_SsrnxJNIEnv *)env_handle, length, NULL, NULL);
}

void *ssrnx_jni_new_local_reference_array(void *env_handle,
                                          const char *array_descriptor,
                                          int32_t length) {
    jni_SsrnxJniEnvState *env = jni_env_state((jni_SsrnxJNIEnv *)env_handle);
    if (!env || !array_descriptor || !jni_array_descriptor_valid(array_descriptor) ||
        array_descriptor[0] != '[' ||
        (array_descriptor[1] != 'L' && array_descriptor[1] != '['))
        return NULL;

    const char *component_desc = array_descriptor + 1;
    char component_name[1024];
    if (component_desc[0] == '[') {
        const size_t n = strlen(component_desc);
        if (n >= sizeof(component_name)) return NULL;
        memcpy(component_name, component_desc, n + 1u);
    } else {
        const size_t n = strlen(component_desc);
        if (n < 3u || component_desc[0] != 'L' || component_desc[n - 1u] != ';' ||
            n - 2u >= sizeof(component_name))
            return NULL;
        memcpy(component_name, component_desc + 1, n - 2u);
        component_name[n - 2u] = '\0';
    }

    void *component_ref = ssrnx_jni_find_class_ref(env_handle, component_name);
    if (!component_ref) return NULL;
    void *array_ref = jni_jni_new_object_array((jni_SsrnxJNIEnv *)env_handle, length,
                                           component_ref, NULL);
    ssrnx_jni_delete_local_ref_host(env_handle, component_ref);
    return array_ref;
}

SsrnxResult ssrnx_jni_set_object_array_element_host(void *env_handle, void *array_ref,
                                                     int32_t index, void *value_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (!env || !array || array->kind != SSRNX_JNI_OBJECT_OBJECT_ARRAY ||
        index < 0 || index >= array->u.object_array.length) return SSRNX_ERR_ARGUMENT;
    jni_jni_set_object_array_element((jni_SsrnxJNIEnv *)env_handle, array_ref, index, value_ref);
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_get_object_array_element_host(void *env_handle, void *array_ref,
                                                     int32_t index, void **out_value_ref) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (out_value_ref) *out_value_ref = NULL;
    if (!env || !out_value_ref || !array || array->kind != SSRNX_JNI_OBJECT_OBJECT_ARRAY ||
        index < 0 || index >= array->u.object_array.length) return SSRNX_ERR_ARGUMENT;
    *out_value_ref = jni_jni_get_object_array_element((jni_SsrnxJNIEnv *)env_handle, array_ref, index);
    /* NULL is a valid Object[] element. Bounds/type were validated above. */
    return SSRNX_OK;
}


void *ssrnx_jni_new_local_array_like_host(void *env_handle,
                                           void *source_array_ref,
                                           int32_t length) {
    jni_SsrnxJniEnvState *env = jni_env_state(env_handle);
    jni_SsrnxJniObject *source = jni_object_from_ref(source_array_ref);
    if (!env || !source || length < 0) return NULL;
    switch (source->kind) {
        case SSRNX_JNI_OBJECT_BOOLEAN_ARRAY: return jni_new_boolean_array((jni_SsrnxJNIEnv *)env_handle, length);
        case SSRNX_JNI_OBJECT_BYTE_ARRAY: return jni_new_byte_array((jni_SsrnxJNIEnv *)env_handle, length);
        case SSRNX_JNI_OBJECT_CHAR_ARRAY: return jni_new_char_array((jni_SsrnxJNIEnv *)env_handle, length);
        case SSRNX_JNI_OBJECT_SHORT_ARRAY: return jni_new_short_array((jni_SsrnxJNIEnv *)env_handle, length);
        case SSRNX_JNI_OBJECT_INT_ARRAY: return jni_new_int_array((jni_SsrnxJNIEnv *)env_handle, length);
        case SSRNX_JNI_OBJECT_LONG_ARRAY: return jni_new_long_array((jni_SsrnxJNIEnv *)env_handle, length);
        case SSRNX_JNI_OBJECT_FLOAT_ARRAY: return jni_new_float_array((jni_SsrnxJNIEnv *)env_handle, length);
        case SSRNX_JNI_OBJECT_DOUBLE_ARRAY: return jni_new_double_array((jni_SsrnxJNIEnv *)env_handle, length);
        case SSRNX_JNI_OBJECT_OBJECT_ARRAY: {
            void *component_ref = NULL;
            if (source->u.object_array.component_class && source->u.object_array.component_class->class_object) {
                SSRNX_PERF_LOCK(&jni_g_lock);
                component_ref = jni_new_ref_locked(env, source->u.object_array.component_class->class_object,
                                               SSRNX_JNI_REF_LOCAL);
                pthread_mutex_unlock(&jni_g_lock);
            }
            void *copy = jni_jni_new_object_array((jni_SsrnxJNIEnv *)env_handle, length, component_ref, NULL);
            jni_jni_delete_local_ref((jni_SsrnxJNIEnv *)env_handle, component_ref);
            return copy;
        }
        default: return NULL;
    }
}

void *ssrnx_jni_new_local_boolean_array(void *env_handle, int32_t length) {
    return jni_new_boolean_array((jni_SsrnxJNIEnv *)env_handle, length);
}

void *ssrnx_jni_new_local_byte_array(void *env_handle, int32_t length) {
    return jni_new_byte_array((jni_SsrnxJNIEnv *)env_handle, length);
}

void *ssrnx_jni_new_local_char_array(void *env_handle, int32_t length) {
    return jni_new_char_array((jni_SsrnxJNIEnv *)env_handle, length);
}

void *ssrnx_jni_new_local_short_array(void *env_handle, int32_t length) {
    return jni_new_short_array((jni_SsrnxJNIEnv *)env_handle, length);
}

void *ssrnx_jni_new_local_int_array(void *env_handle, int32_t length) {
    return jni_new_int_array((jni_SsrnxJNIEnv *)env_handle, length);
}

void *ssrnx_jni_new_local_float_array(void *env_handle, int32_t length) {
    return jni_new_float_array((jni_SsrnxJNIEnv *)env_handle, length);
}

void *ssrnx_jni_new_local_long_array(void *env_handle, int32_t length) {
    return jni_new_long_array((jni_SsrnxJNIEnv *)env_handle, length);
}

void *ssrnx_jni_new_local_double_array(void *env_handle, int32_t length) {
    return jni_new_double_array((jni_SsrnxJNIEnv *)env_handle, length);
}

static jni_SsrnxJniObjectKind jni_primitive_array_kind(char type) {
    switch (type) {
        case 'Z': return SSRNX_JNI_OBJECT_BOOLEAN_ARRAY;
        case 'B': return SSRNX_JNI_OBJECT_BYTE_ARRAY;
        case 'C': return SSRNX_JNI_OBJECT_CHAR_ARRAY;
        case 'S': return SSRNX_JNI_OBJECT_SHORT_ARRAY;
        case 'I': return SSRNX_JNI_OBJECT_INT_ARRAY;
        case 'J': return SSRNX_JNI_OBJECT_LONG_ARRAY;
        case 'F': return SSRNX_JNI_OBJECT_FLOAT_ARRAY;
        case 'D': return SSRNX_JNI_OBJECT_DOUBLE_ARRAY;
        default: return SSRNX_JNI_OBJECT_INSTANCE;
    }
}

SsrnxResult ssrnx_jni_primitive_array_get_host(void *array_ref, char type, int32_t index,
                                               uint64_t *out_bits) {
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (!out_bits) return SSRNX_ERR_ARGUMENT;
    *out_bits = 0;
    if (!array || array->kind != jni_primitive_array_kind(type)) return SSRNX_ERR_ARGUMENT;
    if (index < 0 || index >= ssrnx_jni_array_length_host(array_ref)) return SSRNX_ERR_ARGUMENT;
    switch (type) {
        case 'Z': *out_bits = array->u.boolean_array.values[index]; break;
        case 'B': *out_bits = (uint64_t)(int64_t)array->u.byte_array.values[index]; break;
        case 'C': *out_bits = array->u.char_array.values[index]; break;
        case 'S': *out_bits = (uint64_t)(int64_t)array->u.short_array.values[index]; break;
        case 'I': *out_bits = (uint64_t)(int64_t)array->u.int_array.values[index]; break;
        case 'J': *out_bits = (uint64_t)array->u.long_array.values[index]; break;
        case 'F': { uint32_t b; memcpy(&b, &array->u.float_array.values[index], sizeof(b)); *out_bits = b; break; }
        default: memcpy(out_bits, &array->u.double_array.values[index], sizeof(*out_bits)); break;
    }
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_primitive_array_set_host(void *array_ref, char type, int32_t index,
                                               uint64_t bits) {
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (!array || array->kind != jni_primitive_array_kind(type)) return SSRNX_ERR_ARGUMENT;
    if (index < 0 || index >= ssrnx_jni_array_length_host(array_ref)) return SSRNX_ERR_ARGUMENT;
    switch (type) {
        case 'Z': array->u.boolean_array.values[index] = (uint8_t)bits; break;
        case 'B': array->u.byte_array.values[index] = (int8_t)bits; break;
        case 'C': array->u.char_array.values[index] = (uint16_t)bits; break;
        case 'S': array->u.short_array.values[index] = (int16_t)bits; break;
        case 'I': array->u.int_array.values[index] = (int32_t)bits; break;
        case 'J': array->u.long_array.values[index] = (int64_t)bits; break;
        case 'F': { uint32_t b = (uint32_t)bits; memcpy(&array->u.float_array.values[index], &b, sizeof(b)); break; }
        default: memcpy(&array->u.double_array.values[index], &bits, sizeof(bits)); break;
    }
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_boolean_array_write(void *array_ref, int32_t start, int32_t count,
                                          const uint8_t *in) {
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (!array || array->kind != SSRNX_JNI_OBJECT_BOOLEAN_ARRAY || !in || start < 0 || count < 0 ||
        start > array->u.boolean_array.length || count > array->u.boolean_array.length - start)
        return SSRNX_ERR_ARGUMENT;
    memcpy(array->u.boolean_array.values + start, in, (size_t)count * sizeof(uint8_t));
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_char_array_write(void *array_ref, int32_t start, int32_t count,
                                       const uint16_t *in) {
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (!array || array->kind != SSRNX_JNI_OBJECT_CHAR_ARRAY || !in || start < 0 || count < 0 ||
        start > array->u.char_array.length || count > array->u.char_array.length - start)
        return SSRNX_ERR_ARGUMENT;
    memcpy(array->u.char_array.values + start, in, (size_t)count * sizeof(uint16_t));
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_short_array_write(void *array_ref, int32_t start, int32_t count,
                                        const int16_t *in) {
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (!array || array->kind != SSRNX_JNI_OBJECT_SHORT_ARRAY || !in || start < 0 || count < 0 ||
        start > array->u.short_array.length || count > array->u.short_array.length - start)
        return SSRNX_ERR_ARGUMENT;
    memcpy(array->u.short_array.values + start, in, (size_t)count * sizeof(int16_t));
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_int_array_write(void *array_ref, int32_t start, int32_t count,
                                      const int32_t *in) {
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (!array || array->kind != SSRNX_JNI_OBJECT_INT_ARRAY || !in || start < 0 || count < 0 ||
        start > array->u.int_array.length || count > array->u.int_array.length - start)
        return SSRNX_ERR_ARGUMENT;
    memcpy(array->u.int_array.values + start, in, (size_t)count * sizeof(int32_t));
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_float_array_write(void *array_ref, int32_t start, int32_t count,
                                        const float *in) {
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (!array || array->kind != SSRNX_JNI_OBJECT_FLOAT_ARRAY || !in || start < 0 || count < 0 ||
        start > array->u.float_array.length || count > array->u.float_array.length - start)
        return SSRNX_ERR_ARGUMENT;
    memcpy(array->u.float_array.values + start, in, (size_t)count * sizeof(float));
    return SSRNX_OK;
}

int32_t ssrnx_jni_array_length_host(void *array_ref) {
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (!array) return -1;
    switch (array->kind) {
        case SSRNX_JNI_OBJECT_BOOLEAN_ARRAY: return array->u.boolean_array.length;
        case SSRNX_JNI_OBJECT_BYTE_ARRAY: return array->u.byte_array.length;
        case SSRNX_JNI_OBJECT_CHAR_ARRAY: return array->u.char_array.length;
        case SSRNX_JNI_OBJECT_SHORT_ARRAY: return array->u.short_array.length;
        case SSRNX_JNI_OBJECT_INT_ARRAY: return array->u.int_array.length;
        case SSRNX_JNI_OBJECT_LONG_ARRAY: return array->u.long_array.length;
        case SSRNX_JNI_OBJECT_FLOAT_ARRAY: return array->u.float_array.length;
        case SSRNX_JNI_OBJECT_DOUBLE_ARRAY: return array->u.double_array.length;
        case SSRNX_JNI_OBJECT_OBJECT_ARRAY: return array->u.object_array.length;
        default: return -1;
    }
}

SsrnxResult ssrnx_jni_arraycopy_host(void *src_ref, int32_t src_pos, void *dst_ref,
                                         int32_t dst_pos, int32_t length) {
    jni_SsrnxJniObject *src = jni_object_from_ref(src_ref);
    jni_SsrnxJniObject *dst = jni_object_from_ref(dst_ref);
    if (!src || !dst || src_pos < 0 || dst_pos < 0 || length < 0 || src->kind != dst->kind)
        return SSRNX_ERR_ARGUMENT;
    const int32_t slen = ssrnx_jni_array_length_host(src_ref);
    const int32_t dlen = ssrnx_jni_array_length_host(dst_ref);
    if (slen < 0 || dlen < 0 || src_pos > slen || length > slen - src_pos ||
        dst_pos > dlen || length > dlen - dst_pos) return SSRNX_ERR_ARGUMENT;
    size_t elem = 0; void *sp = NULL; void *dp = NULL;
    switch (src->kind) {
        case SSRNX_JNI_OBJECT_BOOLEAN_ARRAY: elem=sizeof(uint8_t); sp=src->u.boolean_array.values; dp=dst->u.boolean_array.values; break;
        case SSRNX_JNI_OBJECT_BYTE_ARRAY: elem=sizeof(int8_t); sp=src->u.byte_array.values; dp=dst->u.byte_array.values; break;
        case SSRNX_JNI_OBJECT_CHAR_ARRAY: elem=sizeof(uint16_t); sp=src->u.char_array.values; dp=dst->u.char_array.values; break;
        case SSRNX_JNI_OBJECT_SHORT_ARRAY: elem=sizeof(int16_t); sp=src->u.short_array.values; dp=dst->u.short_array.values; break;
        case SSRNX_JNI_OBJECT_INT_ARRAY: elem=sizeof(int32_t); sp=src->u.int_array.values; dp=dst->u.int_array.values; break;
        case SSRNX_JNI_OBJECT_LONG_ARRAY: elem=sizeof(int64_t); sp=src->u.long_array.values; dp=dst->u.long_array.values; break;
        case SSRNX_JNI_OBJECT_FLOAT_ARRAY: elem=sizeof(float); sp=src->u.float_array.values; dp=dst->u.float_array.values; break;
        case SSRNX_JNI_OBJECT_DOUBLE_ARRAY: elem=sizeof(double); sp=src->u.double_array.values; dp=dst->u.double_array.values; break;
        case SSRNX_JNI_OBJECT_OBJECT_ARRAY: elem=sizeof(void*); sp=src->u.object_array.values; dp=dst->u.object_array.values; break;
        default: return SSRNX_ERR_ARGUMENT;
    }
    if (length) memmove((char*)dp + (size_t)dst_pos*elem, (char*)sp + (size_t)src_pos*elem, (size_t)length*elem);
    return SSRNX_OK;
}

int32_t ssrnx_jni_byte_array_length(void *array_ref) {
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    return array && array->kind == SSRNX_JNI_OBJECT_BYTE_ARRAY ? array->u.byte_array.length : -1;
}

SsrnxResult ssrnx_jni_char_array_read(void *array_ref, int32_t start, int32_t count, uint16_t *out) {
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (!array || array->kind != SSRNX_JNI_OBJECT_CHAR_ARRAY || !out || start < 0 || count < 0 ||
        start > array->u.char_array.length || count > array->u.char_array.length - start) return SSRNX_ERR_ARGUMENT;
    memcpy(out, array->u.char_array.values + start, (size_t)count * sizeof(uint16_t));
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_byte_array_read(void *array_ref, int32_t start, int32_t count, int8_t *out) {
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (!array || array->kind != SSRNX_JNI_OBJECT_BYTE_ARRAY || !out || start < 0 || count < 0 ||
        start > array->u.byte_array.length || count > array->u.byte_array.length - start) return SSRNX_ERR_ARGUMENT;
    memcpy(out, array->u.byte_array.values + start, (size_t)count);
    return SSRNX_OK;
}

SsrnxResult ssrnx_jni_byte_array_write(void *array_ref, int32_t start, int32_t count, const int8_t *in) {
    jni_SsrnxJniObject *array = jni_object_from_ref(array_ref);
    if (!array || array->kind != SSRNX_JNI_OBJECT_BYTE_ARRAY || !in || start < 0 || count < 0 ||
        start > array->u.byte_array.length || count > array->u.byte_array.length - start) return SSRNX_ERR_ARGUMENT;
    memcpy(array->u.byte_array.values + start, in, (size_t)count);
    return SSRNX_OK;
}

bool ssrnx_jni_field_exists_host(const char *class_name, const char *name,
                                 const char *signature, bool is_static) {
    if (!class_name || !name || !signature) return false;
    jni_SsrnxJniClass *clazz = jni_resolve_class_lazy(class_name, 0);
    if (!clazz) return false;
    SSRNX_PERF_LOCK(&jni_g_lock);
    const bool found = jni_find_field(clazz, name, signature, is_static) != NULL;
    pthread_mutex_unlock(&jni_g_lock);
    return found;
}

/* Android kills the app process when, for example, a Java thread dies with an
 * uncaught failure (the default uncaught-exception handler). SSRNX records that
 * request here so blocking framework waits and the DEX interpreter unwind to
 * the launcher instead of waiting forever for a thread that no longer runs. */
static pthread_mutex_t jni_g_process_exit_lock = PTHREAD_MUTEX_INITIALIZER;
static bool jni_g_process_exit_requested;
static char jni_g_process_exit_reason[256];

void ssrnx_jni_request_process_exit(const char *reason) {
    SSRNX_PERF_LOCK(&jni_g_process_exit_lock);
    if (!jni_g_process_exit_requested) {
        snprintf(jni_g_process_exit_reason, sizeof(jni_g_process_exit_reason), "%s", reason ? reason : "");
        __atomic_store_n(&jni_g_process_exit_requested, true, __ATOMIC_RELEASE);
    }
    pthread_mutex_unlock(&jni_g_process_exit_lock);
    if (ssrnx_bionic_set_process_terminating) ssrnx_bionic_set_process_terminating(true);
}

bool ssrnx_jni_process_exit_requested(char *out_reason, size_t out_reason_cap) {
    if (!__atomic_load_n(&jni_g_process_exit_requested, __ATOMIC_ACQUIRE)) return false;
    if (out_reason && out_reason_cap) {
        SSRNX_PERF_LOCK(&jni_g_process_exit_lock);
        snprintf(out_reason, out_reason_cap, "%s", jni_g_process_exit_reason);
        pthread_mutex_unlock(&jni_g_process_exit_lock);
    }
    return true;
}

void ssrnx_jni_clear_process_exit(void) {
    SSRNX_PERF_LOCK(&jni_g_process_exit_lock);
    __atomic_store_n(&jni_g_process_exit_requested, false, __ATOMIC_RELEASE);
    jni_g_process_exit_reason[0] = '\0';
    pthread_mutex_unlock(&jni_g_process_exit_lock);
    if (ssrnx_bionic_set_process_terminating) ssrnx_bionic_set_process_terminating(false);
}

#pragma pop_macro("UNSAFE_FIELD_OFFSET_BASE")
#pragma pop_macro("UNSAFE_FIELD_CAP")
#pragma pop_macro("NO_EXTRA")
#pragma pop_macro("NONVIRTUAL_CLASS")
#pragma pop_macro("JVM_TABLE_SLOTS")
#pragma pop_macro("JNI_TRUE")
#pragma pop_macro("JNI_TABLE_SLOTS")
#pragma pop_macro("JNI_SLOT")
#pragma pop_macro("JNI_FALSE")
#pragma pop_macro("DEFINE_SMALL_FIELD")
#pragma pop_macro("DEFINE_PRIMITIVE_ARRAY_JNI")
#pragma pop_macro("DEFINE_CALL_VOID")
#pragma pop_macro("DEFINE_CALL_TYPE")
#pragma pop_macro("DEFINE_CALL_SET")
#pragma pop_macro("SSRNX_PROXY_HANDLER_SIG")
#pragma pop_macro("SSRNX_PROXY_HANDLER_FIELD")
#pragma pop_macro("SSRNX_PROXY_CLASS")
#pragma pop_macro("SSRNX_INVOCATION_HANDLER_INVOKE_SIG")
#pragma pop_macro("SSRNX_DIRECT_BUFFER_CLASS")
#pragma pop_macro("SSRNX_ACC_STATIC")
#pragma pop_macro("SSRNX_ACC_PUBLIC")
#pragma pop_macro("SSRNX_ACC_NATIVE")
#pragma pop_macro("SSRNX_ACC_INTERFACE")
#pragma pop_macro("SSRNX_ACC_ABSTRACT")


/* ---- java_lang ---- */
#pragma push_macro("INTERN_BUCKETS")
#pragma push_macro("NPE")
#pragma push_macro("SBUF_CLASS")
#pragma push_macro("SB_CLASS")
#pragma push_macro("SB_MAGIC")
#pragma push_macro("SIOOBE")
#pragma push_macro("STRING_CLASS")
#pragma push_macro("STRING_COMPARATOR")
/* Core java.lang text model on exact UTF-16 data: String operations,
 * StringBuilder/StringBuffer, the CharSequence and Appendable interfaces,
 * String.intern, Object.wait/clone.
 *
 * Every operation works on the complete UTF-16 contents (JLS 3.1, String and
 * AbstractStringBuilder contracts); no fixed-size text buffer is involved,
 * so long strings (JSON, URLs, protocol payloads) are never truncated. */


#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




#define SB_MAGIC UINT64_C(0x41504b3653423136) /* APK6SB16 */
#define STRING_CLASS "java/lang/String"
#define STRING_COMPARATOR "java/lang/String$CaseInsensitiveComparator"
#define SB_CLASS "java/lang/StringBuilder"
#define SBUF_CLASS "java/lang/StringBuffer"
#define SIOOBE "java/lang/StringIndexOutOfBoundsException"
#define NPE "java/lang/NullPointerException"

typedef struct java_lang_SbPayload {
    uint64_t magic;
    uint16_t *buf;
    int32_t len, cap;
    pthread_mutex_t lock; /* StringBuffer methods are synchronized */
    struct java_lang_SbPayload *next;
} java_lang_SbPayload;

static pthread_mutex_t java_lang_g_lang_lock = PTHREAD_MUTEX_INITIALIZER;
static java_lang_SbPayload *java_lang_g_builders;

static SsrnxJniValue java_lang_v_obj(void *ref) { SsrnxJniValue v = {0}; v.l = ref; return v; }
static SsrnxJniValue java_lang_v_i(int32_t i) { SsrnxJniValue v = {0}; v.i = i; return v; }
static SsrnxJniValue java_lang_v_z(bool z) { SsrnxJniValue v = {0}; v.z = z ? 1 : 0; return v; }
static SsrnxJniValue java_lang_v_c(uint16_t c) { SsrnxJniValue v = {0}; v.c = c; return v; }
static SsrnxJniValue java_lang_v_void(void) { SsrnxJniValue v = {0}; return v; }

static void java_lang_throw_msg(void *env, const char *cls, const char *msg) { (void)ssrnx_jni_throw_new_by_name(env, cls, msg); }

static void java_lang_throw_index(void *env, const char *cls, int32_t index, int32_t length) {
    char msg[96];
    snprintf(msg, sizeof(msg), "index=%d length=%d", (int)index, (int)length);
    java_lang_throw_msg(env, cls, msg);
}

static void *java_lang_new_str(void *env, const uint16_t *s, int32_t n) {
    static const uint16_t empty = 0;
    return ssrnx_jni_new_local_string_utf16(env, n > 0 ? s : &empty, n > 0 ? n : 0);
}

/* ------------------------------------------------------------ builders */

static java_lang_SbPayload *java_lang_sb_from_token(void *token) {
    java_lang_SbPayload *p = token ? ssrnx_jni_get_token_host_data(token) : NULL;
    return p && p->magic == SB_MAGIC ? p : NULL;
}

static java_lang_SbPayload *java_lang_sb_from_ref(void *ref) {
    java_lang_SbPayload *p = ref ? ssrnx_jni_get_object_host_data(ref) : NULL;
    return p && p->magic == SB_MAGIC ? p : NULL;
}

static java_lang_SbPayload *java_lang_sb_ensure(void *token) {
    java_lang_SbPayload *p = java_lang_sb_from_token(token);
    if (p || !token) return p;
    p = calloc(1, sizeof(*p));
    if (!p) return NULL;
    p->magic = SB_MAGIC;
    p->cap = 16;
    p->buf = malloc((size_t)p->cap * sizeof(uint16_t));
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&p->lock, &attr);
    pthread_mutexattr_destroy(&attr);
    if (!p->buf || ssrnx_jni_set_token_host_data(token, p) != SSRNX_OK) {
        pthread_mutex_destroy(&p->lock);
        free(p->buf);
        free(p);
        return NULL;
    }
    pthread_mutex_lock(&java_lang_g_lang_lock);
    p->next = java_lang_g_builders;
    java_lang_g_builders = p;
    pthread_mutex_unlock(&java_lang_g_lang_lock);
    return p;
}

static bool java_lang_sb_reserve(java_lang_SbPayload *p, int64_t extra) {
    const int64_t need = (int64_t)p->len + extra;
    if (need > INT32_MAX - 8) return false;
    if (need <= p->cap) return true;
    int64_t cap = (int64_t)p->cap * 2 + 2;
    if (cap < need) cap = need;
    if (cap > INT32_MAX - 8) cap = INT32_MAX - 8;
    uint16_t *grown = realloc(p->buf, (size_t)cap * sizeof(uint16_t));
    if (!grown) return false;
    p->buf = grown;
    p->cap = (int32_t)cap;
    return true;
}

/* Insert n code units at index (index <= len already checked). */
static bool java_lang_sb_insert(void *env, java_lang_SbPayload *p, int32_t index, const uint16_t *s, int32_t n) {
    if (n <= 0) return true;
    if (!java_lang_sb_reserve(p, n)) { java_lang_throw_msg(env, "java/lang/OutOfMemoryError", "StringBuilder capacity"); return false; }
    memmove(p->buf + index + n, p->buf + index, (size_t)(p->len - index) * sizeof(uint16_t));
    memcpy(p->buf + index, s, (size_t)n * sizeof(uint16_t));
    p->len += n;
    return true;
}

static bool java_lang_sb_append(void *env, java_lang_SbPayload *p, const uint16_t *s, int32_t n) {
    return java_lang_sb_insert(env, p, p->len, s, n);
}

/* ------------------------------------------------------------ text access */

uint16_t *ssrnx_java_charseq_utf16(void *env, void *ref, int32_t *len) {
    *len = 0;
    if (!ref) { java_lang_throw_msg(env, NPE, "CharSequence"); return NULL; }
    uint16_t *chars = ssrnx_jni_string_utf16_dup_host(ref, len);
    if (chars) return chars;
    java_lang_SbPayload *p = java_lang_sb_from_ref(ref);
    if (p) {
        pthread_mutex_lock(&p->lock);
        chars = malloc((size_t)(p->len > 0 ? p->len : 1) * sizeof(uint16_t));
        if (chars) { memcpy(chars, p->buf, (size_t)p->len * sizeof(uint16_t)); *len = p->len; }
        pthread_mutex_unlock(&p->lock);
        return chars;
    }
    SsrnxJniValue out = {0};
    if (!ssrnx_android_call_virtual(env, ref, "toString", "()Ljava/lang/String;", NULL, 0, &out)) return NULL;
    if (!out.l) {
        static const uint16_t null_text[] = {'n', 'u', 'l', 'l'};
        chars = malloc(sizeof(null_text));
        if (chars) { memcpy(chars, null_text, sizeof(null_text)); *len = 4; }
        return chars;
    }
    chars = ssrnx_jni_string_utf16_dup_host(out.l, len);
    ssrnx_jni_delete_local_ref_host(env, out.l);
    return chars;
}

/* String.valueOf(Object) text: "null" or the object's toString(). */
static uint16_t *java_lang_value_of_utf16(void *env, void *ref, int32_t *len) {
    if (!ref) {
        static const uint16_t null_text[] = {'n', 'u', 'l', 'l'};
        uint16_t *chars = malloc(sizeof(null_text));
        if (chars) { memcpy(chars, null_text, sizeof(null_text)); *len = 4; }
        return chars;
    }
    return ssrnx_java_charseq_utf16(env, ref, len);
}

static uint16_t *java_lang_self_utf16(void *env, void *token, int32_t *len) {
    void *self = ssrnx_jni_new_local_from_token(env, token);
    uint16_t *chars = self ? ssrnx_jni_string_utf16_dup_host(self, len) : NULL;
    if (self) ssrnx_jni_delete_local_ref_host(env, self);
    return chars;
}

static uint16_t *java_lang_string_arg(void *env, void *ref, int32_t *len, const char *what) {
    if (!ref) { java_lang_throw_msg(env, NPE, what); return NULL; }
    return ssrnx_jni_string_utf16_dup_host(ref, len);
}

/* ------------------------------------------------------------ search helpers */

static int32_t java_lang_find_units(const uint16_t *s, int32_t n, const uint16_t *t, int32_t m, int32_t from) {
    if (from < 0) from = 0;
    if (from > n) return m == 0 ? n : -1;
    if (m == 0) return from;
    for (int32_t i = from; i + m <= n; ++i)
        if (s[i] == t[0] && memcmp(s + i, t, (size_t)m * sizeof(uint16_t)) == 0) return i;
    return -1;
}

static int32_t java_lang_rfind_units(const uint16_t *s, int32_t n, const uint16_t *t, int32_t m, int32_t from) {
    if (from < 0) return -1;
    if (from > n - m) from = n - m;
    for (int32_t i = from; i >= 0; --i)
        if (memcmp(s + i, t, (size_t)m * sizeof(uint16_t)) == 0) return i;
    return -1;
}

/* Code point ch as 1 or 2 code units. */
static int32_t java_lang_code_point_units(int32_t ch, uint16_t out[2]) {
    if (ch < 0 || ch > 0x10FFFF) return 0;
    if (ch < 0x10000) { out[0] = (uint16_t)ch; return 1; }
    ch -= 0x10000;
    out[0] = (uint16_t)(0xD800 + (ch >> 10));
    out[1] = (uint16_t)(0xDC00 + (ch & 0x3FF));
    return 2;
}

static int32_t java_lang_code_point_at(const uint16_t *s, int32_t n, int32_t i) {
    const uint16_t c = s[i];
    if (c >= 0xD800 && c <= 0xDBFF && i + 1 < n && s[i + 1] >= 0xDC00 && s[i + 1] <= 0xDFFF)
        return 0x10000 + ((c - 0xD800) << 10) + (s[i + 1] - 0xDC00);
    return c;
}

/* Case mapping per code point (1:1 simple mapping). */
static uint16_t *java_lang_map_case(const uint16_t *s, int32_t n, bool upper) {
    uint16_t *out = calloc((size_t)(n > 0 ? n : 1), sizeof(uint16_t));
    if (!out) return NULL;
    for (int32_t i = 0; i < n;) {
        const int32_t cp = java_lang_code_point_at(s, n, i);
        const uint32_t mapped = upper ? ssrnx_java_to_upper((uint32_t)cp) : ssrnx_java_to_lower((uint32_t)cp);
        uint16_t units[2];
        const int32_t w = cp >= 0x10000 ? 2 : 1;
        if (java_lang_code_point_units((int32_t)mapped, units) == w) memcpy(out + i, units, (size_t)w * sizeof(uint16_t));
        else memcpy(out + i, s + i, (size_t)w * sizeof(uint16_t));
        i += w;
    }
    return out;
}

static bool java_lang_chars_equal_ignore_case(uint16_t a, uint16_t b) {
    if (a == b) return true;
    const uint32_t ua = ssrnx_java_to_upper(a), ub = ssrnx_java_to_upper(b);
    return ua == ub || ssrnx_java_to_lower(ua) == ssrnx_java_to_lower(ub);
}

/* ------------------------------------------------------------ String */

enum java_lang_StrOp {
    S_EQUALS, S_EQUALS_IC, S_CONTAINS, S_STARTS, S_STARTS_AT, S_ENDS, S_INDEX_CH, S_INDEX_CH_FROM,
    S_INDEX_STR, S_INDEX_STR_FROM, S_LAST_CH, S_LAST_CH_FROM, S_LAST_STR, S_LAST_STR_FROM, S_LOWER, S_UPPER,
    S_TRIM, S_TO_CHARS, S_COMPARE_IC, S_REGION, S_REGION_IC, S_REPLACE_SEQ, S_CONTENT_EQUALS, S_SUBSEQ,
    S_TO_STRING, S_CODE_POINT_AT, S_CODE_POINT_BEFORE, S_CODE_POINT_COUNT, S_INTERN, S_GET_CHARS, S_IS_BLANK,
    S_STRIP, S_REPEAT,
};

/* String.intern(): one canonical instance per content, held by a global ref. */
typedef struct java_lang_InternEntry { uint32_t hash; int32_t len; uint16_t *chars; void *global; struct java_lang_InternEntry *next; } java_lang_InternEntry;
#define INTERN_BUCKETS 1024u
static java_lang_InternEntry *java_lang_g_intern[INTERN_BUCKETS];

/* Canonical String for these UTF-16 units. `candidate` (a String with the
 * same content, may be NULL) becomes canonical when none exists yet; with no
 * candidate a new String is created. Returns a local reference. */
static void *java_lang_string_intern_chars(void *env, void *candidate, const uint16_t *s, int32_t n) {
    uint32_t h = 0;
    for (int32_t i = 0; i < n; ++i) h = 31u * h + s[i];
    pthread_mutex_lock(&java_lang_g_lang_lock);
    for (java_lang_InternEntry *e = java_lang_g_intern[h % INTERN_BUCKETS]; e; e = e->next) {
        if (e->hash == h && e->len == n && memcmp(e->chars, s, (size_t)n * sizeof(uint16_t)) == 0) {
            void *local = ssrnx_jni_new_local_ref_host(env, e->global);
            pthread_mutex_unlock(&java_lang_g_lang_lock);
            return local;
        }
    }
    java_lang_InternEntry *e = calloc(1, sizeof(*e));
    void *self = candidate ? ssrnx_jni_new_local_ref_host(env, candidate)
                           : ssrnx_jni_new_local_string_utf16(env, s, n);
    if (e && self) {
        e->chars = malloc((size_t)(n > 0 ? n : 1) * sizeof(uint16_t));
        e->global = e->chars ? ssrnx_jni_new_global_ref_host(env, self) : NULL;
    }
    if (!e || !self || !e->global) {
        pthread_mutex_unlock(&java_lang_g_lang_lock);
        if (e) free(e->chars);
        free(e);
        return self;
    }
    memcpy(e->chars, s, (size_t)n * sizeof(uint16_t));
    e->hash = h;
    e->len = n;
    e->next = java_lang_g_intern[h % INTERN_BUCKETS];
    java_lang_g_intern[h % INTERN_BUCKETS] = e;
    pthread_mutex_unlock(&java_lang_g_lang_lock);
    return self;
}

static void *java_lang_string_intern(void *env, void *token, const uint16_t *s, int32_t n) {
    void *self = ssrnx_jni_new_local_from_token(env, token);
    void *canonical = java_lang_string_intern_chars(env, self, s, n);
    if (self && canonical != self) ssrnx_jni_delete_local_ref_host(env, self);
    return canonical;
}

/* JLS 3.10.5: string literals of equal content are the same String instance
 * (in every class), and String.intern() of equal content returns it too.
 * `mutf8` is the DEX/class-file modified UTF-8 form. */
void *ssrnx_java_lang_intern_literal(void *env, const char *mutf8) {
    if (!mutf8) return NULL;
    uint16_t stack_units[256] = {0};
    const size_t bytes = strlen(mutf8);
    uint16_t *units = bytes <= 256u ? stack_units : malloc((bytes + 1u) * sizeof(uint16_t));
    if (!units) return NULL;
    units[0] = 0;
    int32_t n = 0;
    for (const unsigned char *p = (const unsigned char *)mutf8; *p;) {
        uint32_t c = *p++;
        if (c >= 0xF0 && (p[0] & 0xC0) == 0x80 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
            /* Standard 4-byte UTF-8 (not MUTF-8, but tolerated): surrogate pair. */
            c = ((c & 0x07u) << 18) | ((uint32_t)(p[0] & 0x3F) << 12) | ((uint32_t)(p[1] & 0x3F) << 6) | (p[2] & 0x3Fu);
            p += 3;
            c -= 0x10000u;
            units[n++] = (uint16_t)(0xD800u + (c >> 10));
            units[n++] = (uint16_t)(0xDC00u + (c & 0x3FFu));
            continue;
        }
        if (c >= 0xE0 && (p[0] & 0xC0) == 0x80 && (p[1] & 0xC0) == 0x80) {
            c = ((c & 0x0Fu) << 12) | ((uint32_t)(p[0] & 0x3F) << 6) | (p[1] & 0x3Fu);
            p += 2;
        } else if (c >= 0xC0 && (p[0] & 0xC0) == 0x80) {
            c = ((c & 0x1Fu) << 6) | (p[0] & 0x3Fu); /* includes MUTF-8 NUL (C0 80) */
            p += 1;
        }
        units[n++] = (uint16_t)c;
    }
    void *ref = java_lang_string_intern_chars(env, NULL, units, n);
    if (units != stack_units) free(units);
    return ref;
}

static SsrnxJniValue java_lang_string_op(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    const enum java_lang_StrOp op = (enum java_lang_StrOp)(intptr_t)u;
    int32_t len = 0, olen = 0;
    uint16_t *s = java_lang_self_utf16(env, token, &len);
    if (!s) return java_lang_v_obj(NULL);
    uint16_t *o = NULL;
    SsrnxJniValue r = java_lang_v_obj(NULL);
    switch (op) {
        case S_EQUALS:
            o = a[0].l ? ssrnx_jni_string_utf16_dup_host(a[0].l, &olen) : NULL;
            r = java_lang_v_z(o && olen == len && memcmp(s, o, (size_t)len * sizeof(uint16_t)) == 0);
            break;
        case S_EQUALS_IC: {
            o = a[0].l ? ssrnx_jni_string_utf16_dup_host(a[0].l, &olen) : NULL;
            bool eq = o && olen == len;
            for (int32_t i = 0; eq && i < len; ++i) eq = java_lang_chars_equal_ignore_case(s[i], o[i]);
            r = java_lang_v_z(eq);
            break;
        }
        case S_CONTAINS:
            if ((o = ssrnx_java_charseq_utf16(env, a[0].l, &olen))) r = java_lang_v_z(java_lang_find_units(s, len, o, olen, 0) >= 0);
            else r = java_lang_v_z(false);
            break;
        case S_STARTS: case S_STARTS_AT: {
            const int32_t at = op == S_STARTS_AT ? a[1].i : 0;
            if (!(o = java_lang_string_arg(env, a[0].l, &olen, "prefix"))) { r = java_lang_v_z(false); break; }
            r = java_lang_v_z(at >= 0 && at <= len - olen && memcmp(s + at, o, (size_t)olen * sizeof(uint16_t)) == 0);
            break;
        }
        case S_ENDS:
            if (!(o = java_lang_string_arg(env, a[0].l, &olen, "suffix"))) { r = java_lang_v_z(false); break; }
            r = java_lang_v_z(olen <= len && memcmp(s + len - olen, o, (size_t)olen * sizeof(uint16_t)) == 0);
            break;
        case S_INDEX_CH: case S_INDEX_CH_FROM: case S_LAST_CH: case S_LAST_CH_FROM: {
            uint16_t units[2];
            const int32_t m = java_lang_code_point_units(a[0].i, units);
            if (!m) { r = java_lang_v_i(-1); break; }
            if (op == S_INDEX_CH || op == S_INDEX_CH_FROM) {
                const int32_t from = op == S_INDEX_CH_FROM ? a[1].i : 0;
                r = java_lang_v_i(from >= len ? -1 : java_lang_find_units(s, len, units, m, from));
            } else {
                int32_t from = op == S_LAST_CH_FROM ? a[1].i : len - 1;
                if (from >= len) from = len - 1;
                r = java_lang_v_i(java_lang_rfind_units(s, len, units, m, from));
            }
            break;
        }
        case S_INDEX_STR: case S_INDEX_STR_FROM: case S_LAST_STR: case S_LAST_STR_FROM:
            if (!(o = java_lang_string_arg(env, a[0].l, &olen, "str"))) { r = java_lang_v_i(-1); break; }
            if (op == S_INDEX_STR || op == S_INDEX_STR_FROM)
                r = java_lang_v_i(java_lang_find_units(s, len, o, olen, op == S_INDEX_STR_FROM ? a[1].i : 0));
            else
                r = java_lang_v_i(java_lang_rfind_units(s, len, o, olen, op == S_LAST_STR_FROM ? a[1].i : len));
            break;
        case S_LOWER: case S_UPPER: {
            uint16_t *mapped = java_lang_map_case(s, len, op == S_UPPER);
            if (mapped) {
                if (memcmp(mapped, s, (size_t)len * sizeof(uint16_t)) == 0) r = java_lang_v_obj(ssrnx_jni_new_local_from_token(env, token));
                else r = java_lang_v_obj(java_lang_new_str(env, mapped, len));
                free(mapped);
            }
            break;
        }
        case S_TRIM: case S_STRIP: {
            int32_t b = 0, e = len;
            if (op == S_TRIM) { while (b < e && s[b] <= ' ') b++; while (e > b && s[e - 1] <= ' ') e--; }
            else { while (b < e && ssrnx_java_character_is_whitespace(s[b])) b++; while (e > b && ssrnx_java_character_is_whitespace(s[e - 1])) e--; }
            r = java_lang_v_obj(b == 0 && e == len ? ssrnx_jni_new_local_from_token(env, token) : java_lang_new_str(env, s + b, e - b));
            break;
        }
        case S_IS_BLANK: {
            bool blank = true;
            for (int32_t i = 0; blank && i < len; ++i) blank = ssrnx_java_character_is_whitespace(s[i]);
            r = java_lang_v_z(blank);
            break;
        }
        case S_REPEAT: {
            const int32_t count = a[0].i;
            if (count < 0) { java_lang_throw_msg(env, "java/lang/IllegalArgumentException", "count is negative"); break; }
            if (len && (int64_t)len * count > INT32_MAX - 8) { java_lang_throw_msg(env, "java/lang/OutOfMemoryError", "String.repeat"); break; }
            uint16_t *out = malloc((size_t)(len * count > 0 ? len * count : 1) * sizeof(uint16_t));
            if (!out) break;
            for (int32_t i = 0; i < count; ++i) memcpy(out + (size_t)i * (size_t)len, s, (size_t)len * sizeof(uint16_t));
            r = java_lang_v_obj(java_lang_new_str(env, out, len * count));
            free(out);
            break;
        }
        case S_TO_CHARS: {
            void *array = ssrnx_jni_new_local_char_array(env, len);
            if (array && len) (void)ssrnx_jni_char_array_write(array, 0, len, s);
            r = java_lang_v_obj(array);
            break;
        }
        case S_COMPARE_IC: {
            if (!(o = java_lang_string_arg(env, a[0].l, &olen, "str"))) { r = java_lang_v_i(0); break; }
            const int32_t m = len < olen ? len : olen;
            int32_t diff = len - olen;
            for (int32_t i = 0; i < m; ++i) {
                uint32_t c1 = s[i], c2 = o[i];
                if (c1 == c2) continue;
                c1 = ssrnx_java_to_lower(ssrnx_java_to_upper(c1));
                c2 = ssrnx_java_to_lower(ssrnx_java_to_upper(c2));
                if (c1 != c2) { diff = (int32_t)c1 - (int32_t)c2; break; }
            }
            r = java_lang_v_i(diff);
            break;
        }
        case S_REGION: case S_REGION_IC: {
            const bool ic = op == S_REGION_IC ? a[0].z != 0 : false;
            const size_t base = op == S_REGION_IC ? 1u : 0u;
            const int32_t toffset = a[base].i, ooffset = a[base + 2].i, count = a[base + 3].i;
            if (!(o = java_lang_string_arg(env, a[base + 1].l, &olen, "other"))) { r = java_lang_v_z(false); break; }
            /* String.regionMatches: offsets and length checked as Java does. */
            bool ok = toffset >= 0 && ooffset >= 0 && (int64_t)toffset <= (int64_t)len - count &&
                      (int64_t)ooffset <= (int64_t)olen - count;
            for (int32_t i = 0; ok && i < count; ++i)
                ok = s[toffset + i] == o[ooffset + i] || (ic && java_lang_chars_equal_ignore_case(s[toffset + i], o[ooffset + i]));
            r = java_lang_v_z(ok);
            break;
        }
        case S_REPLACE_SEQ: {
            int32_t tlen = 0, rlen = 0;
            uint16_t *t = ssrnx_java_charseq_utf16(env, a[0].l, &tlen);
            uint16_t *rep = t ? ssrnx_java_charseq_utf16(env, a[1].l, &rlen) : NULL;
            if (!t || !rep) { free(t); free(rep); break; }
            size_t cap = (size_t)len + 16u, out_n = 0;
            uint16_t *out = malloc(cap * sizeof(uint16_t));
            int32_t i = 0;
            bool ok = out != NULL;
            while (ok && i <= len) {
                const int32_t hit = tlen == 0 ? i : java_lang_find_units(s, len, t, tlen, i);
                const int32_t stop = hit < 0 ? len : hit;
                const size_t need = out_n + (size_t)(stop - i) + (hit < 0 ? 0u : (size_t)rlen) + 1u;
                if (need > cap) {
                    while (cap < need) cap *= 2u;
                    uint16_t *grown = realloc(out, cap * sizeof(uint16_t));
                    if (!grown) { ok = false; break; }
                    out = grown;
                }
                memcpy(out + out_n, s + i, (size_t)(stop - i) * sizeof(uint16_t));
                out_n += (size_t)(stop - i);
                if (hit < 0) break;
                memcpy(out + out_n, rep, (size_t)rlen * sizeof(uint16_t));
                out_n += (size_t)rlen;
                if (tlen == 0) {
                    if (hit >= len) break;
                    out[out_n++] = s[hit];
                    i = hit + 1;
                } else i = hit + tlen;
            }
            if (ok && out_n <= INT32_MAX) r = java_lang_v_obj(java_lang_new_str(env, out, (int32_t)out_n));
            free(out); free(t); free(rep);
            break;
        }
        case S_CONTENT_EQUALS:
            if ((o = ssrnx_java_charseq_utf16(env, a[0].l, &olen)))
                r = java_lang_v_z(olen == len && memcmp(s, o, (size_t)len * sizeof(uint16_t)) == 0);
            else r = java_lang_v_z(false);
            break;
        case S_SUBSEQ: {
            const int32_t b = a[0].i, e = a[1].i;
            if (b < 0 || e > len || b > e) { java_lang_throw_index(env, SIOOBE, b < 0 ? b : e, len); break; }
            r = java_lang_v_obj(java_lang_new_str(env, s + b, e - b));
            break;
        }
        case S_TO_STRING: r = java_lang_v_obj(ssrnx_jni_new_local_from_token(env, token)); break;
        case S_CODE_POINT_AT:
            if (a[0].i < 0 || a[0].i >= len) { java_lang_throw_index(env, SIOOBE, a[0].i, len); r = java_lang_v_i(0); break; }
            r = java_lang_v_i(java_lang_code_point_at(s, len, a[0].i));
            break;
        case S_CODE_POINT_BEFORE: {
            const int32_t i = a[0].i;
            if (i < 1 || i > len) { java_lang_throw_index(env, SIOOBE, i, len); r = java_lang_v_i(0); break; }
            int32_t cp = s[i - 1];
            if (cp >= 0xDC00 && cp <= 0xDFFF && i >= 2 && s[i - 2] >= 0xD800 && s[i - 2] <= 0xDBFF)
                cp = 0x10000 + ((s[i - 2] - 0xD800) << 10) + (cp - 0xDC00);
            r = java_lang_v_i(cp);
            break;
        }
        case S_CODE_POINT_COUNT: {
            const int32_t b = a[0].i, e = a[1].i;
            if (b < 0 || e > len || b > e) { java_lang_throw_msg(env, "java/lang/IndexOutOfBoundsException", "codePointCount"); r = java_lang_v_i(0); break; }
            int32_t count = 0;
            for (int32_t i = b; i < e; ++i, ++count)
                if (s[i] >= 0xD800 && s[i] <= 0xDBFF && i + 1 < e && s[i + 1] >= 0xDC00 && s[i + 1] <= 0xDFFF) i++;
            r = java_lang_v_i(count);
            break;
        }
        case S_INTERN: r = java_lang_v_obj(java_lang_string_intern(env, token, s, len)); break;
        case S_GET_CHARS: {
            const int32_t b = a[0].i, e = a[1].i, db = a[3].i;
            if (!a[2].l) { java_lang_throw_msg(env, NPE, "dst"); break; }
            const int32_t dlen = ssrnx_jni_array_length_host(a[2].l);
            if (b < 0 || e > len || b > e) { java_lang_throw_index(env, SIOOBE, b < 0 ? b : e, len); break; }
            if (db < 0 || db > dlen - (e - b)) { java_lang_throw_msg(env, "java/lang/ArrayIndexOutOfBoundsException", "dstBegin"); break; }
            if (e > b) (void)ssrnx_jni_char_array_write(a[2].l, db, e - b, s + b);
            r = java_lang_v_void();
            break;
        }
    }
    (void)n;
    free(s);
    free(o);
    return r;
}

/* Comparator's erased bridge casts both arguments before comparing Strings. */
static SsrnxJniValue java_lang_string_case_compare(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)token; (void)u;
    if (n != 2) return java_lang_v_i(0);
    for (size_t i = 0; i < 2; ++i) {
        if (a[i].l && !ssrnx_jni_is_instance_of_class_host(a[i].l, STRING_CLASS)) {
            java_lang_throw_msg(env, "java/lang/ClassCastException", "expected String");
            return java_lang_v_i(0);
        }
    }
    if (!a[0].l || !a[1].l) {
        java_lang_throw_msg(env, NPE, "string");
        return java_lang_v_i(0);
    }
    return java_lang_string_op(env, ssrnx_jni_object_token_host(a[0].l), a + 1, 1, (void *)(intptr_t)S_COMPARE_IC);
}

/* new String(char[] [, offset, count]) / String(String) / String(StringBuilder|StringBuffer). */
static SsrnxJniValue java_lang_string_ctor_chars(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    if (!a[0].l) { java_lang_throw_msg(env, NPE, "value"); return java_lang_v_void(); }
    const int32_t total = ssrnx_jni_array_length_host(a[0].l);
    const int32_t off = n == 3 ? a[1].i : 0, count = n == 3 ? a[2].i : total;
    if (total < 0 || off < 0 || count < 0 || off > total - count) { java_lang_throw_index(env, SIOOBE, off, total); return java_lang_v_void(); }
    uint16_t *chars = malloc((size_t)(count > 0 ? count : 1) * sizeof(uint16_t));
    if (!chars) return java_lang_v_void();
    if (count && ssrnx_jni_char_array_read(a[0].l, off, count, chars) != SSRNX_OK) { free(chars); return java_lang_v_void(); }
    (void)ssrnx_jni_string_token_assign_utf16(token, chars, count);
    free(chars);
    return java_lang_v_void();
}

/* ART StringFactory: the deprecated high byte is masked, not a charset. */
static SsrnxJniValue java_lang_string_ctor_high_bytes(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    if (!a[0].l) { java_lang_throw_msg(env, NPE, "data"); return java_lang_v_void(); }
    const int32_t total = ssrnx_jni_byte_array_length(a[0].l);
    const int32_t off = n == 4 ? a[2].i : 0, count = n == 4 ? a[3].i : total;
    if (off < 0 || count < 0 || off > total - count) { java_lang_throw_index(env, SIOOBE, off, total); return java_lang_v_void(); }
    uint16_t *chars = malloc((size_t)(count > 0 ? count : 1) * sizeof(*chars));
    if (!chars) { java_lang_throw_msg(env, "java/lang/OutOfMemoryError", "String"); return java_lang_v_void(); }
    const uint8_t *bytes = ssrnx_jni_primitive_array_data_host(a[0].l);
    const uint16_t high = (uint16_t)((a[1].i & 0xff) << 8);
    for (int32_t i = 0; i < count; ++i) chars[i] = (uint16_t)(high | bytes[off + i]);
    (void)ssrnx_jni_string_token_assign_utf16(token, chars, count);
    free(chars);
    return java_lang_v_void();
}

static SsrnxJniValue java_lang_string_ctor_codepoints(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)n; (void)u;
    if (!a[0].l) { java_lang_throw_msg(env, NPE, "codePoints"); return java_lang_v_void(); }
    const int32_t total = ssrnx_jni_array_length_host(a[0].l), off = a[1].i, count = a[2].i;
    if (off < 0 || count < 0 || off > total - count) { java_lang_throw_index(env, SIOOBE, off, total); return java_lang_v_void(); }
    if (count > INT32_MAX / 2) { java_lang_throw_msg(env, "java/lang/OutOfMemoryError", "String"); return java_lang_v_void(); }
    uint16_t *chars = malloc((size_t)(count > 0 ? count * 2 : 1) * sizeof(*chars));
    if (!chars) { java_lang_throw_msg(env, "java/lang/OutOfMemoryError", "String"); return java_lang_v_void(); }
    int32_t len = 0;
    for (int32_t i = 0; i < count; ++i) {
        uint64_t bits = 0;
        (void)ssrnx_jni_primitive_array_get_host(a[0].l, 'I', off + i, &bits);
        const uint32_t cp = (uint32_t)bits;
        if (cp > 0x10ffff) {
            free(chars); java_lang_throw_msg(env, "java/lang/IllegalArgumentException", "invalid code point"); return java_lang_v_void();
        }
        if (cp < 0x10000) chars[len++] = (uint16_t)cp;
        else {
            chars[len++] = (uint16_t)(0xd800 + ((cp - 0x10000) >> 10));
            chars[len++] = (uint16_t)(0xdc00 + ((cp - 0x10000) & 0x3ff));
        }
    }
    (void)ssrnx_jni_string_token_assign_utf16(token, chars, len);
    free(chars);
    return java_lang_v_void();
}

/* AOSP String.getBytes' legacy low-byte copy checks each destination store;
 * a short array can retain a written prefix, and an empty range never touches dst. */
static SsrnxJniValue java_lang_string_get_bytes_legacy(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)n; (void)u;
    int32_t len = 0;
    uint16_t *chars = java_lang_self_utf16(env, token, &len);
    if (!chars) return java_lang_v_void();
    const int32_t begin = a[0].i, end = a[1].i;
    if (begin < 0 || end > len || begin > end) {
        free(chars); java_lang_throw_index(env, SIOOBE, begin, len); return java_lang_v_void();
    }
    const int32_t dest_len = ssrnx_jni_byte_array_length(a[2].l);
    for (int32_t i = begin; i < end; ++i) {
        const int64_t j = (int64_t)a[3].i + i - begin;
        if (!a[2].l) { java_lang_throw_msg(env, NPE, "dst"); break; }
        if (j < 0 || j >= dest_len) { java_lang_throw_index(env, "java/lang/ArrayIndexOutOfBoundsException", a[3].i, dest_len); break; }
        (void)ssrnx_jni_primitive_array_set_host(a[2].l, 'B', (int32_t)j, (uint8_t)chars[i]);
    }
    free(chars);
    return java_lang_v_void();
}

static SsrnxJniValue java_lang_string_ctor_charseq(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)n; (void)u;
    int32_t len = 0;
    uint16_t *chars = ssrnx_java_charseq_utf16(env, a[0].l, &len);
    if (!chars) return java_lang_v_void();
    (void)ssrnx_jni_string_token_assign_utf16(token, chars, len);
    free(chars);
    return java_lang_v_void();
}

/* String.valueOf(char[] [, offset, count]) / copyValueOf. */
static SsrnxJniValue java_lang_string_value_of_chars(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)token; (void)u;
    if (!a[0].l) { java_lang_throw_msg(env, NPE, "data"); return java_lang_v_obj(NULL); }
    const int32_t total = ssrnx_jni_array_length_host(a[0].l);
    const int32_t off = n == 3 ? a[1].i : 0, count = n == 3 ? a[2].i : total;
    if (total < 0 || off < 0 || count < 0 || off > total - count) { java_lang_throw_index(env, SIOOBE, off, total); return java_lang_v_obj(NULL); }
    uint16_t *chars = malloc((size_t)(count > 0 ? count : 1) * sizeof(uint16_t));
    if (!chars) return java_lang_v_obj(NULL);
    void *s = NULL;
    if (!count || ssrnx_jni_char_array_read(a[0].l, off, count, chars) == SSRNX_OK) s = java_lang_new_str(env, chars, count);
    free(chars);
    return java_lang_v_obj(s);
}

/* String.valueOf(Object): "null" or obj.toString() (virtual). */
static SsrnxJniValue java_lang_string_value_of_object(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)token; (void)n; (void)u;
    if (!a[0].l) {
        static const uint16_t null_text[] = {'n', 'u', 'l', 'l'};
        return java_lang_v_obj(java_lang_new_str(env, null_text, 4));
    }
    if (ssrnx_jni_string_length_host(a[0].l) >= 0) return java_lang_v_obj(ssrnx_jni_new_local_ref_host(env, a[0].l));
    SsrnxJniValue out = {0};
    if (!ssrnx_android_call_virtual(env, a[0].l, "toString", "()Ljava/lang/String;", NULL, 0, &out)) return java_lang_v_obj(NULL);
    return out;
}

/* String.join(delimiter, CharSequence...) and join(delimiter, Iterable). */
static bool java_lang_join_piece(void *env, java_lang_SbPayload *acc, void *element, bool first, const uint16_t *d, int32_t dlen) {
    if (!first && !java_lang_sb_append(env, acc, d, dlen)) return false;
    int32_t n = 0;
    uint16_t *text = java_lang_value_of_utf16(env, element, &n);
    if (!text) return false;
    const bool ok = java_lang_sb_append(env, acc, text, n);
    free(text);
    return ok;
}

static SsrnxJniValue java_lang_string_join(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)token; (void)n;
    const bool iterable = u != NULL;
    int32_t dlen = 0;
    uint16_t *d = ssrnx_java_charseq_utf16(env, a[0].l, &dlen);
    if (!d) return java_lang_v_obj(NULL);
    if (!a[1].l) { free(d); java_lang_throw_msg(env, NPE, "elements"); return java_lang_v_obj(NULL); }
    java_lang_SbPayload acc = {0};
    acc.magic = SB_MAGIC;
    bool ok = true;
    if (!iterable) {
        const int32_t count = ssrnx_jni_array_length_host(a[1].l);
        for (int32_t i = 0; ok && i < count; ++i) {
            void *e = NULL;
            (void)ssrnx_jni_get_object_array_element_host(env, a[1].l, i, &e);
            ok = java_lang_join_piece(env, &acc, e, i == 0, d, dlen);
            if (e) ssrnx_jni_delete_local_ref_host(env, e);
        }
    } else {
        SsrnxJniValue it = {0}, has = {0}, next = {0};
        ok = ssrnx_android_call_virtual(env, a[1].l, "iterator", "()Ljava/util/Iterator;", NULL, 0, &it) && it.l;
        for (bool first = true; ok; first = false) {
            ok = ssrnx_android_call_virtual(env, it.l, "hasNext", "()Z", NULL, 0, &has);
            if (!ok || !has.z) break;
            ok = ssrnx_android_call_virtual(env, it.l, "next", "()Ljava/lang/Object;", NULL, 0, &next) &&
                 java_lang_join_piece(env, &acc, next.l, first, d, dlen);
            if (next.l) ssrnx_jni_delete_local_ref_host(env, next.l);
            next.l = NULL;
        }
        if (it.l) ssrnx_jni_delete_local_ref_host(env, it.l);
    }
    void *result = ok && !ssrnx_jni_exception_pending_host(env) ? java_lang_new_str(env, acc.buf, acc.len) : NULL;
    free(acc.buf);
    free(d);
    return java_lang_v_obj(result);
}

/* ------------------------------------------------------------ StringBuilder / StringBuffer */

enum java_lang_SbOp {
    B_CTOR, B_CTOR_CAP, B_CTOR_SEQ, B_APPEND_OBJ, B_APPEND_STR, B_APPEND_SEQ_RANGE, B_APPEND_CHARS,
    B_APPEND_CHARS_RANGE, B_APPEND_Z, B_APPEND_C, B_APPEND_I, B_APPEND_J, B_APPEND_F, B_APPEND_D, B_APPEND_CP,
    B_LENGTH, B_CAPACITY, B_ENSURE, B_TRIM, B_SET_LENGTH, B_CHAR_AT, B_SET_CHAR_AT, B_CODE_POINT_AT,
    B_DELETE_CHAR_AT, B_DELETE, B_INSERT_OBJ, B_INSERT_STR, B_INSERT_CHARS, B_INSERT_Z, B_INSERT_C, B_INSERT_I,
    B_INSERT_J, B_INSERT_F, B_INSERT_D, B_REVERSE, B_INDEX, B_INDEX_FROM, B_LAST, B_LAST_FROM, B_REPLACE,
    B_SUBSTRING, B_SUBSTRING_RANGE, B_SUBSEQ, B_TO_STRING, B_GET_CHARS, B_COMPARE_TO,
};

/* Text of a primitive/object argument for append/insert. */
static uint16_t *java_lang_arg_text(void *env, enum java_lang_SbOp op, const SsrnxJniValue *v, int32_t *n) {
    char text[64];
    *n = 0;
    switch (op) {
        case B_APPEND_Z: case B_INSERT_Z: snprintf(text, sizeof(text), "%s", v->z ? "true" : "false"); break;
        case B_APPEND_I: case B_INSERT_I: snprintf(text, sizeof(text), "%d", (int)v->i); break;
        case B_APPEND_J: case B_INSERT_J: snprintf(text, sizeof(text), "%lld", (long long)v->j); break;
        case B_APPEND_F: case B_INSERT_F: ssrnx_java_floating_to_string(v->f, true, text, sizeof(text)); break;
        case B_APPEND_D: case B_INSERT_D: ssrnx_java_floating_to_string(v->d, false, text, sizeof(text)); break;
        case B_APPEND_C: case B_INSERT_C: {
            uint16_t *c = malloc(sizeof(uint16_t));
            if (c) { *c = v->c; *n = 1; }
            return c;
        }
        case B_APPEND_CP: {
            uint16_t units[2];
            const int32_t w = java_lang_code_point_units(v->i, units);
            if (!w) { java_lang_throw_msg(env, "java/lang/IllegalArgumentException", "Not a valid Unicode code point"); return NULL; }
            uint16_t *c = malloc(2 * sizeof(uint16_t));
            if (c) { memcpy(c, units, sizeof(units)); *n = w; }
            return c;
        }
        default: return java_lang_value_of_utf16(env, v->l, n);
    }
    const int32_t len = (int32_t)strlen(text);
    uint16_t *out = malloc((size_t)(len > 0 ? len : 1) * sizeof(uint16_t));
    if (out) { for (int32_t i = 0; i < len; ++i) out[i] = (uint8_t)text[i]; *n = len; }
    return out;
}

static uint16_t *java_lang_char_array_range(void *env, void *array, bool ranged, int32_t off, int32_t count, int32_t *n) {
    if (!array) { java_lang_throw_msg(env, NPE, "str"); return NULL; }
    const int32_t total = ssrnx_jni_array_length_host(array);
    if (!ranged) { off = 0; count = total; }
    if (total < 0 || off < 0 || count < 0 || off > total - count) { java_lang_throw_index(env, "java/lang/IndexOutOfBoundsException", off, total); return NULL; }
    uint16_t *out = malloc((size_t)(count > 0 ? count : 1) * sizeof(uint16_t));
    if (!out) return NULL;
    if (count && ssrnx_jni_char_array_read(array, off, count, out) != SSRNX_OK) { free(out); return NULL; }
    *n = count;
    return out;
}

static SsrnxJniValue java_lang_builder_op(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    const enum java_lang_SbOp op = (enum java_lang_SbOp)(intptr_t)u;
    java_lang_SbPayload *p = java_lang_sb_ensure(token);
    if (!p) { java_lang_throw_msg(env, "java/lang/OutOfMemoryError", "StringBuilder"); return java_lang_v_obj(NULL); }
    (void)n;
    SsrnxJniValue self = java_lang_v_obj(NULL);
    bool returns_self = false;
    SsrnxJniValue r = java_lang_v_obj(NULL);
    uint16_t *text = NULL;
    int32_t tn = 0;
    /* Argument text is produced before taking the (recursive) lock: it may
     * run toString() on arbitrary objects, including this builder. */
    switch (op) {
        case B_CTOR_SEQ: case B_APPEND_STR: case B_APPEND_OBJ:
            text = java_lang_value_of_utf16(env, a[0].l, &tn);
            break;
        case B_APPEND_SEQ_RANGE: {
            int32_t full = 0;
            uint16_t *seq = java_lang_value_of_utf16(env, a[0].l, &full);
            if (!seq) break;
            const int32_t s = a[1].i, e = a[2].i;
            if (s < 0 || s > e || e > full) { free(seq); java_lang_throw_msg(env, "java/lang/IndexOutOfBoundsException", "start/end"); break; }
            text = seq; memmove(text, seq + s, (size_t)(e - s) * sizeof(uint16_t)); tn = e - s;
            break;
        }
        case B_APPEND_CHARS: text = java_lang_char_array_range(env, a[0].l, false, 0, 0, &tn); break;
        case B_APPEND_CHARS_RANGE: text = java_lang_char_array_range(env, a[0].l, true, a[1].i, a[2].i, &tn); break;
        case B_APPEND_Z: case B_APPEND_C: case B_APPEND_I: case B_APPEND_J: case B_APPEND_F: case B_APPEND_D:
        case B_APPEND_CP:
            text = java_lang_arg_text(env, op, &a[0], &tn);
            break;
        case B_INSERT_OBJ: case B_INSERT_STR: text = java_lang_value_of_utf16(env, a[1].l, &tn); break;
        case B_INSERT_CHARS: text = java_lang_char_array_range(env, a[1].l, false, 0, 0, &tn); break;
        case B_INSERT_Z: case B_INSERT_C: case B_INSERT_I: case B_INSERT_J: case B_INSERT_F: case B_INSERT_D:
            text = java_lang_arg_text(env, op, &a[1], &tn);
            break;
        case B_INDEX: case B_INDEX_FROM: case B_LAST: case B_LAST_FROM: case B_REPLACE: case B_COMPARE_TO:
            if (op == B_COMPARE_TO) text = ssrnx_java_charseq_utf16(env, a[0].l, &tn);
            else text = java_lang_string_arg(env, op == B_REPLACE ? a[2].l : a[0].l, &tn, "str");
            break;
        default: break;
    }
    if (ssrnx_jni_exception_pending_host(env)) { free(text); return java_lang_v_obj(NULL); }
    pthread_mutex_lock(&p->lock);
    switch (op) {
        case B_CTOR: break;
        case B_CTOR_CAP:
            if (a[0].i < 0) java_lang_throw_msg(env, "java/lang/NegativeArraySizeException", "capacity");
            else (void)java_lang_sb_reserve(p, a[0].i);
            break;
        case B_CTOR_SEQ:
            if (!a[0].l) { java_lang_throw_msg(env, NPE, "str"); break; }
            (void)java_lang_sb_append(env, p, text, tn);
            break;
        case B_APPEND_OBJ: case B_APPEND_STR: case B_APPEND_SEQ_RANGE: case B_APPEND_CHARS: case B_APPEND_CHARS_RANGE:
        case B_APPEND_Z: case B_APPEND_C: case B_APPEND_I: case B_APPEND_J: case B_APPEND_F: case B_APPEND_D:
        case B_APPEND_CP:
            if (text) returns_self = java_lang_sb_append(env, p, text, tn);
            break;
        case B_LENGTH: r = java_lang_v_i(p->len); break;
        case B_CAPACITY: r = java_lang_v_i(p->cap); break;
        case B_ENSURE: if (a[0].i > p->cap) (void)java_lang_sb_reserve(p, a[0].i - p->len); r = java_lang_v_void(); break;
        case B_TRIM: r = java_lang_v_void(); break;
        case B_SET_LENGTH:
            if (a[0].i < 0) { java_lang_throw_index(env, SIOOBE, a[0].i, p->len); break; }
            if (a[0].i > p->len) {
                if (!java_lang_sb_reserve(p, a[0].i - p->len)) { java_lang_throw_msg(env, "java/lang/OutOfMemoryError", "setLength"); break; }
                memset(p->buf + p->len, 0, (size_t)(a[0].i - p->len) * sizeof(uint16_t));
            }
            p->len = a[0].i;
            r = java_lang_v_void();
            break;
        case B_CHAR_AT:
            if (a[0].i < 0 || a[0].i >= p->len) { java_lang_throw_index(env, SIOOBE, a[0].i, p->len); r = java_lang_v_c(0); break; }
            r = java_lang_v_c(p->buf[a[0].i]);
            break;
        case B_SET_CHAR_AT:
            if (a[0].i < 0 || a[0].i >= p->len) { java_lang_throw_index(env, SIOOBE, a[0].i, p->len); break; }
            p->buf[a[0].i] = a[1].c;
            r = java_lang_v_void();
            break;
        case B_CODE_POINT_AT:
            if (a[0].i < 0 || a[0].i >= p->len) { java_lang_throw_index(env, SIOOBE, a[0].i, p->len); r = java_lang_v_i(0); break; }
            r = java_lang_v_i(java_lang_code_point_at(p->buf, p->len, a[0].i));
            break;
        case B_DELETE_CHAR_AT:
            if (a[0].i < 0 || a[0].i >= p->len) { java_lang_throw_index(env, SIOOBE, a[0].i, p->len); break; }
            memmove(p->buf + a[0].i, p->buf + a[0].i + 1, (size_t)(p->len - a[0].i - 1) * sizeof(uint16_t));
            p->len--;
            returns_self = true;
            break;
        case B_DELETE: {
            int32_t s = a[0].i, e = a[1].i;
            if (e > p->len) e = p->len;
            if (s < 0 || s > e) { java_lang_throw_index(env, SIOOBE, s, p->len); break; }
            memmove(p->buf + s, p->buf + e, (size_t)(p->len - e) * sizeof(uint16_t));
            p->len -= e - s;
            returns_self = true;
            break;
        }
        case B_INSERT_OBJ: case B_INSERT_STR: case B_INSERT_CHARS: case B_INSERT_Z: case B_INSERT_C: case B_INSERT_I:
        case B_INSERT_J: case B_INSERT_F: case B_INSERT_D:
            if (a[0].i < 0 || a[0].i > p->len) { java_lang_throw_index(env, SIOOBE, a[0].i, p->len); break; }
            if (text) returns_self = java_lang_sb_insert(env, p, a[0].i, text, tn);
            break;
        case B_REVERSE: {
            for (int32_t i = 0, j = p->len - 1; i < j; ++i, --j) { const uint16_t t = p->buf[i]; p->buf[i] = p->buf[j]; p->buf[j] = t; }
            /* Surrogate pairs keep their order (AbstractStringBuilder.reverse). */
            for (int32_t i = 0; i + 1 < p->len; ++i)
                if (p->buf[i] >= 0xDC00 && p->buf[i] <= 0xDFFF && p->buf[i + 1] >= 0xD800 && p->buf[i + 1] <= 0xDBFF) {
                    const uint16_t t = p->buf[i]; p->buf[i] = p->buf[i + 1]; p->buf[i + 1] = t; i++;
                }
            returns_self = true;
            break;
        }
        case B_INDEX: case B_INDEX_FROM:
            if (text) r = java_lang_v_i(java_lang_find_units(p->buf, p->len, text, tn, op == B_INDEX_FROM ? a[1].i : 0));
            break;
        case B_LAST: case B_LAST_FROM:
            if (text) r = java_lang_v_i(java_lang_rfind_units(p->buf, p->len, text, tn, op == B_LAST_FROM ? a[1].i : p->len));
            break;
        case B_REPLACE: {
            int32_t s = a[0].i, e = a[1].i;
            if (s < 0 || s > p->len || s > e) { java_lang_throw_index(env, SIOOBE, s, p->len); break; }
            if (e > p->len) e = p->len;
            if (!text) break;
            const int32_t removed = e - s;
            if (tn > removed && !java_lang_sb_reserve(p, tn - removed)) { java_lang_throw_msg(env, "java/lang/OutOfMemoryError", "replace"); break; }
            memmove(p->buf + s + tn, p->buf + e, (size_t)(p->len - e) * sizeof(uint16_t));
            memcpy(p->buf + s, text, (size_t)tn * sizeof(uint16_t));
            p->len += tn - removed;
            returns_self = true;
            break;
        }
        case B_SUBSTRING: case B_SUBSTRING_RANGE: case B_SUBSEQ: {
            const int32_t s = a[0].i, e = op == B_SUBSTRING ? p->len : a[1].i;
            if (s < 0 || e > p->len || s > e) { java_lang_throw_index(env, SIOOBE, s < 0 ? s : e, p->len); break; }
            r = java_lang_v_obj(java_lang_new_str(env, p->buf + s, e - s));
            break;
        }
        case B_TO_STRING: r = java_lang_v_obj(java_lang_new_str(env, p->buf, p->len)); break;
        case B_GET_CHARS: {
            const int32_t s = a[0].i, e = a[1].i, db = a[3].i;
            if (!a[2].l) { java_lang_throw_msg(env, NPE, "dst"); break; }
            if (s < 0 || e > p->len || s > e) { java_lang_throw_index(env, SIOOBE, s < 0 ? s : e, p->len); break; }
            const int32_t dlen = ssrnx_jni_array_length_host(a[2].l);
            if (db < 0 || db > dlen - (e - s)) { java_lang_throw_msg(env, "java/lang/ArrayIndexOutOfBoundsException", "dstBegin"); break; }
            if (e > s) (void)ssrnx_jni_char_array_write(a[2].l, db, e - s, p->buf + s);
            r = java_lang_v_void();
            break;
        }
        case B_COMPARE_TO: {
            if (!text) break;
            const int32_t m = p->len < tn ? p->len : tn;
            int32_t diff = p->len - tn;
            for (int32_t i = 0; i < m; ++i) if (p->buf[i] != text[i]) { diff = (int32_t)p->buf[i] - (int32_t)text[i]; break; }
            r = java_lang_v_i(diff);
            break;
        }
    }
    pthread_mutex_unlock(&p->lock);
    free(text);
    if (returns_self && !ssrnx_jni_exception_pending_host(env)) self = java_lang_v_obj(ssrnx_jni_new_local_from_token(env, token));
    return returns_self ? self : r;
}

/* ------------------------------------------------------------ Object */

static SsrnxJniValue java_lang_object_wait(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    void *self = ssrnx_jni_new_local_from_token(env, token);
    if (!self) return java_lang_v_void();
    const int64_t ms = n >= 1 ? a[0].j : 0;
    const int32_t ns = n == 2 ? a[1].i : 0;
    (void)ssrnx_jni_monitor_wait_host(env, self, ms, ns);
    ssrnx_jni_delete_local_ref_host(env, self);
    return java_lang_v_void();
}

static SsrnxJniValue java_lang_object_clone(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)n; (void)u;
    void *self = ssrnx_jni_new_local_from_token(env, token);
    void *copy = self ? ssrnx_jni_clone_object_host(env, self) : NULL;
    if (self) ssrnx_jni_delete_local_ref_host(env, self);
    return java_lang_v_obj(copy);
}

/* ------------------------------------------------------------ registration */

typedef struct java_lang_LangMethod {
    const char *cls, *name, *sig;
    bool is_static;
    SsrnxJniMethodCallback cb;
    intptr_t op;
} java_lang_LangMethod;

static const java_lang_LangMethod java_lang_k_string_methods[] = {
    {STRING_CLASS, "equals", "(Ljava/lang/Object;)Z", false, java_lang_string_op, S_EQUALS},
    {STRING_CLASS, "equalsIgnoreCase", "(Ljava/lang/String;)Z", false, java_lang_string_op, S_EQUALS_IC},
    {STRING_CLASS, "contains", "(Ljava/lang/CharSequence;)Z", false, java_lang_string_op, S_CONTAINS},
    {STRING_CLASS, "startsWith", "(Ljava/lang/String;)Z", false, java_lang_string_op, S_STARTS},
    {STRING_CLASS, "startsWith", "(Ljava/lang/String;I)Z", false, java_lang_string_op, S_STARTS_AT},
    {STRING_CLASS, "endsWith", "(Ljava/lang/String;)Z", false, java_lang_string_op, S_ENDS},
    {STRING_CLASS, "indexOf", "(I)I", false, java_lang_string_op, S_INDEX_CH},
    {STRING_CLASS, "indexOf", "(II)I", false, java_lang_string_op, S_INDEX_CH_FROM},
    {STRING_CLASS, "indexOf", "(Ljava/lang/String;)I", false, java_lang_string_op, S_INDEX_STR},
    {STRING_CLASS, "indexOf", "(Ljava/lang/String;I)I", false, java_lang_string_op, S_INDEX_STR_FROM},
    {STRING_CLASS, "lastIndexOf", "(I)I", false, java_lang_string_op, S_LAST_CH},
    {STRING_CLASS, "lastIndexOf", "(II)I", false, java_lang_string_op, S_LAST_CH_FROM},
    {STRING_CLASS, "lastIndexOf", "(Ljava/lang/String;)I", false, java_lang_string_op, S_LAST_STR},
    {STRING_CLASS, "lastIndexOf", "(Ljava/lang/String;I)I", false, java_lang_string_op, S_LAST_STR_FROM},
    {STRING_CLASS, "toLowerCase", "()Ljava/lang/String;", false, java_lang_string_op, S_LOWER},
    {STRING_CLASS, "toLowerCase", "(Ljava/util/Locale;)Ljava/lang/String;", false, java_lang_string_op, S_LOWER},
    {STRING_CLASS, "toUpperCase", "()Ljava/lang/String;", false, java_lang_string_op, S_UPPER},
    {STRING_CLASS, "toUpperCase", "(Ljava/util/Locale;)Ljava/lang/String;", false, java_lang_string_op, S_UPPER},
    {STRING_CLASS, "trim", "()Ljava/lang/String;", false, java_lang_string_op, S_TRIM},
    {STRING_CLASS, "strip", "()Ljava/lang/String;", false, java_lang_string_op, S_STRIP},
    {STRING_CLASS, "isBlank", "()Z", false, java_lang_string_op, S_IS_BLANK},
    {STRING_CLASS, "repeat", "(I)Ljava/lang/String;", false, java_lang_string_op, S_REPEAT},
    {STRING_CLASS, "toCharArray", "()[C", false, java_lang_string_op, S_TO_CHARS},
    {STRING_CLASS, "compareToIgnoreCase", "(Ljava/lang/String;)I", false, java_lang_string_op, S_COMPARE_IC},
    {STRING_CLASS, "regionMatches", "(ILjava/lang/String;II)Z", false, java_lang_string_op, S_REGION},
    {STRING_CLASS, "regionMatches", "(ZILjava/lang/String;II)Z", false, java_lang_string_op, S_REGION_IC},
    {STRING_CLASS, "replace", "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Ljava/lang/String;", false, java_lang_string_op, S_REPLACE_SEQ},
    {STRING_CLASS, "contentEquals", "(Ljava/lang/CharSequence;)Z", false, java_lang_string_op, S_CONTENT_EQUALS},
    {STRING_CLASS, "contentEquals", "(Ljava/lang/StringBuffer;)Z", false, java_lang_string_op, S_CONTENT_EQUALS},
    {STRING_CLASS, "subSequence", "(II)Ljava/lang/CharSequence;", false, java_lang_string_op, S_SUBSEQ},
    {STRING_CLASS, "toString", "()Ljava/lang/String;", false, java_lang_string_op, S_TO_STRING},
    {STRING_CLASS, "codePointAt", "(I)I", false, java_lang_string_op, S_CODE_POINT_AT},
    {STRING_CLASS, "codePointBefore", "(I)I", false, java_lang_string_op, S_CODE_POINT_BEFORE},
    {STRING_CLASS, "codePointCount", "(II)I", false, java_lang_string_op, S_CODE_POINT_COUNT},
    {STRING_CLASS, "intern", "()Ljava/lang/String;", false, java_lang_string_op, S_INTERN},
    {STRING_CLASS, "getChars", "(II[CI)V", false, java_lang_string_op, S_GET_CHARS},
    {STRING_CLASS, "<init>", "([C)V", false, java_lang_string_ctor_chars, 0},
    {STRING_CLASS, "<init>", "([CII)V", false, java_lang_string_ctor_chars, 0},
    {STRING_CLASS, "<init>", "([BI)V", false, java_lang_string_ctor_high_bytes, 0},
    {STRING_CLASS, "<init>", "([BIII)V", false, java_lang_string_ctor_high_bytes, 0},
    {STRING_CLASS, "<init>", "([III)V", false, java_lang_string_ctor_codepoints, 0},
    {STRING_CLASS, "getBytes", "(II[BI)V", false, java_lang_string_get_bytes_legacy, 0},
    {STRING_CLASS, "<init>", "(Ljava/lang/String;)V", false, java_lang_string_ctor_charseq, 0},
    {STRING_CLASS, "<init>", "(Ljava/lang/StringBuilder;)V", false, java_lang_string_ctor_charseq, 0},
    {STRING_CLASS, "<init>", "(Ljava/lang/StringBuffer;)V", false, java_lang_string_ctor_charseq, 0},
    {STRING_CLASS, "valueOf", "([C)Ljava/lang/String;", true, java_lang_string_value_of_chars, 0},
    {STRING_CLASS, "valueOf", "([CII)Ljava/lang/String;", true, java_lang_string_value_of_chars, 0},
    {STRING_CLASS, "copyValueOf", "([C)Ljava/lang/String;", true, java_lang_string_value_of_chars, 0},
    {STRING_CLASS, "copyValueOf", "([CII)Ljava/lang/String;", true, java_lang_string_value_of_chars, 0},
    {STRING_CLASS, "valueOf", "(Ljava/lang/Object;)Ljava/lang/String;", true, java_lang_string_value_of_object, 0},
    {STRING_CLASS, "join", "(Ljava/lang/CharSequence;[Ljava/lang/CharSequence;)Ljava/lang/String;", true, java_lang_string_join, 0},
    {STRING_CLASS, "join", "(Ljava/lang/CharSequence;Ljava/lang/Iterable;)Ljava/lang/String;", true, java_lang_string_join, 1},
    {"java/lang/Object", "wait", "()V", false, java_lang_object_wait, 0},
    {"java/lang/Object", "wait", "(J)V", false, java_lang_object_wait, 0},
    {"java/lang/Object", "wait", "(JI)V", false, java_lang_object_wait, 0},
    {"java/lang/Object", "clone", "()Ljava/lang/Object;", false, java_lang_object_clone, 0},
};

/* Builder methods; "@" in a descriptor stands for the builder class itself. */
static const struct { const char *name, *sig; intptr_t op; } java_lang_k_builder_methods[] = {
    {"<init>", "()V", B_CTOR}, {"<init>", "(I)V", B_CTOR_CAP}, {"<init>", "(Ljava/lang/String;)V", B_CTOR_SEQ},
    {"<init>", "(Ljava/lang/CharSequence;)V", B_CTOR_SEQ},
    {"append", "(Ljava/lang/Object;)@", B_APPEND_OBJ}, {"append", "(Ljava/lang/String;)@", B_APPEND_STR},
    {"append", "(Ljava/lang/StringBuffer;)@", B_APPEND_STR}, {"append", "(Ljava/lang/CharSequence;)@", B_APPEND_STR},
    {"append", "(Ljava/lang/CharSequence;II)@", B_APPEND_SEQ_RANGE}, {"append", "([C)@", B_APPEND_CHARS},
    {"append", "([CII)@", B_APPEND_CHARS_RANGE}, {"append", "(Z)@", B_APPEND_Z}, {"append", "(C)@", B_APPEND_C},
    {"append", "(I)@", B_APPEND_I}, {"append", "(J)@", B_APPEND_J}, {"append", "(F)@", B_APPEND_F},
    {"append", "(D)@", B_APPEND_D}, {"appendCodePoint", "(I)@", B_APPEND_CP},
    /* Appendable bridges. */
    {"append", "(C)Ljava/lang/Appendable;", B_APPEND_C},
    {"append", "(Ljava/lang/CharSequence;)Ljava/lang/Appendable;", B_APPEND_STR},
    {"append", "(Ljava/lang/CharSequence;II)Ljava/lang/Appendable;", B_APPEND_SEQ_RANGE},
    {"length", "()I", B_LENGTH}, {"capacity", "()I", B_CAPACITY}, {"ensureCapacity", "(I)V", B_ENSURE},
    {"trimToSize", "()V", B_TRIM}, {"setLength", "(I)V", B_SET_LENGTH}, {"charAt", "(I)C", B_CHAR_AT},
    {"setCharAt", "(IC)V", B_SET_CHAR_AT}, {"codePointAt", "(I)I", B_CODE_POINT_AT},
    {"deleteCharAt", "(I)@", B_DELETE_CHAR_AT}, {"delete", "(II)@", B_DELETE},
    {"insert", "(ILjava/lang/Object;)@", B_INSERT_OBJ}, {"insert", "(ILjava/lang/String;)@", B_INSERT_STR},
    {"insert", "(ILjava/lang/CharSequence;)@", B_INSERT_STR}, {"insert", "(I[C)@", B_INSERT_CHARS},
    {"insert", "(IZ)@", B_INSERT_Z}, {"insert", "(IC)@", B_INSERT_C}, {"insert", "(II)@", B_INSERT_I},
    {"insert", "(IJ)@", B_INSERT_J}, {"insert", "(IF)@", B_INSERT_F}, {"insert", "(ID)@", B_INSERT_D},
    {"reverse", "()@", B_REVERSE}, {"indexOf", "(Ljava/lang/String;)I", B_INDEX},
    {"indexOf", "(Ljava/lang/String;I)I", B_INDEX_FROM}, {"lastIndexOf", "(Ljava/lang/String;)I", B_LAST},
    {"lastIndexOf", "(Ljava/lang/String;I)I", B_LAST_FROM}, {"replace", "(IILjava/lang/String;)@", B_REPLACE},
    {"substring", "(I)Ljava/lang/String;", B_SUBSTRING}, {"substring", "(II)Ljava/lang/String;", B_SUBSTRING_RANGE},
    {"subSequence", "(II)Ljava/lang/CharSequence;", B_SUBSEQ}, {"toString", "()Ljava/lang/String;", B_TO_STRING},
    {"getChars", "(II[CI)V", B_GET_CHARS}, {"compareTo", "(@)I", B_COMPARE_TO},
};

SsrnxResult ssrnx_java_lang_register_classes(void) {
    static const char *const k_classes[][2] = {
        {"java/lang/CharSequence", "java/lang/Object"},
        {"java/lang/Appendable", "java/lang/Object"},
        {"java/lang/Cloneable", "java/lang/Object"},
        {"java/io/Serializable", "java/lang/Object"},
        {STRING_COMPARATOR, "java/lang/Object"},
        {SB_CLASS, "java/lang/Object"},
        {SBUF_CLASS, "java/lang/Object"},
        {"java/lang/CloneNotSupportedException", "java/lang/Exception"},
        {"java/lang/NegativeArraySizeException", "java/lang/RuntimeException"},
        {"java/lang/InterruptedException", "java/lang/Exception"},
        {"java/io/File", "java/lang/Object"},
    };
    for (size_t i = 0; i < sizeof(k_classes) / sizeof(k_classes[0]); ++i) {
        const SsrnxResult rc = ssrnx_jni_register_class(k_classes[i][0], k_classes[i][1]);
        if (rc != SSRNX_OK) return rc;
    }
    static const char *const k_links[][2] = {
        {STRING_CLASS, "java/lang/CharSequence"}, {SB_CLASS, "java/lang/CharSequence"},
        {SBUF_CLASS, "java/lang/CharSequence"}, {SB_CLASS, "java/lang/Appendable"},
        {SBUF_CLASS, "java/lang/Appendable"}, {STRING_CLASS, "java/io/Serializable"},
        {SB_CLASS, "java/io/Serializable"}, {SBUF_CLASS, "java/io/Serializable"},
        {STRING_COMPARATOR, "java/util/Comparator"}, {STRING_COMPARATOR, "java/io/Serializable"},
    };
    for (size_t i = 0; i < sizeof(k_links) / sizeof(k_links[0]); ++i) {
        const SsrnxResult rc = ssrnx_jni_register_interface(k_links[i][0], k_links[i][1]);
        if (rc != SSRNX_OK) return rc;
    }
    return SSRNX_OK;
}

SsrnxResult ssrnx_java_lang_register_members(void) {
    for (size_t i = 0; i < sizeof(java_lang_k_string_methods) / sizeof(java_lang_k_string_methods[0]); ++i) {
        const java_lang_LangMethod *m = &java_lang_k_string_methods[i];
        const SsrnxResult rc = ssrnx_jni_register_method(m->cls, m->name, m->sig, m->is_static, m->cb, (void *)m->op);
        if (rc != SSRNX_OK) return rc;
    }
    SsrnxResult comparator_rc = ssrnx_jni_register_method(STRING_COMPARATOR, "compare",
        "(Ljava/lang/Object;Ljava/lang/Object;)I", false, java_lang_string_case_compare, NULL);
    if (comparator_rc == SSRNX_OK) comparator_rc = ssrnx_jni_register_method(STRING_COMPARATOR, "compare",
        "(Ljava/lang/String;Ljava/lang/String;)I", false, java_lang_string_case_compare, NULL);
    if (comparator_rc != SSRNX_OK) return comparator_rc;
    comparator_rc = ssrnx_jni_register_field(STRING_CLASS, "CASE_INSENSITIVE_ORDER", "Ljava/util/Comparator;",
                                            true, java_lang_v_obj(NULL));
    if (comparator_rc != SSRNX_OK) return comparator_rc;
    void *comparator_env = ssrnx_jni_current_env();
    if (!comparator_env) return SSRNX_ERR_ARGUMENT;
    void *comparator = ssrnx_jni_new_local_object(comparator_env, STRING_COMPARATOR);
    if (!comparator) return SSRNX_ERR_NO_MEMORY;
    /* Static object fields retain the token after the initializer's local ref is deleted. */
    comparator_rc = ssrnx_jni_set_static_field_value(STRING_CLASS, "CASE_INSENSITIVE_ORDER",
                                                    "Ljava/util/Comparator;", java_lang_v_obj(comparator));
    ssrnx_jni_delete_local_ref_host(comparator_env, comparator);
    if (comparator_rc != SSRNX_OK) return comparator_rc;
    static const char *const k_builders[] = {SB_CLASS, SBUF_CLASS};
    for (size_t b = 0; b < 2; ++b) {
        for (size_t i = 0; i < sizeof(java_lang_k_builder_methods) / sizeof(java_lang_k_builder_methods[0]); ++i) {
            char sig[160];
            size_t o = 0;
            for (const char *c = java_lang_k_builder_methods[i].sig; *c && o + 40 < sizeof(sig); ++c) {
                if (*c == '@') o += (size_t)snprintf(sig + o, sizeof(sig) - o, "L%s;", k_builders[b]);
                else sig[o++] = *c;
            }
            sig[o] = '\0';
            const SsrnxResult rc = ssrnx_jni_register_method(k_builders[b], java_lang_k_builder_methods[i].name, sig, false,
                                                             java_lang_builder_op, (void *)java_lang_k_builder_methods[i].op);
            if (rc != SSRNX_OK) return rc;
        }
    }
    static const char *const k_charseq[][2] = {
        {"length", "()I"}, {"charAt", "(I)C"}, {"subSequence", "(II)Ljava/lang/CharSequence;"},
        {"toString", "()Ljava/lang/String;"},
    };
    for (size_t i = 0; i < sizeof(k_charseq) / sizeof(k_charseq[0]); ++i) {
        const SsrnxResult rc = ssrnx_jni_register_interface_method("java/lang/CharSequence", k_charseq[i][0], k_charseq[i][1]);
        if (rc != SSRNX_OK) return rc;
    }
    static const char *const k_appendable[] = {
        "(C)Ljava/lang/Appendable;", "(Ljava/lang/CharSequence;)Ljava/lang/Appendable;",
        "(Ljava/lang/CharSequence;II)Ljava/lang/Appendable;",
    };
    for (size_t i = 0; i < sizeof(k_appendable) / sizeof(k_appendable[0]); ++i) {
        const SsrnxResult rc = ssrnx_jni_register_interface_method("java/lang/Appendable", "append", k_appendable[i]);
        if (rc != SSRNX_OK) return rc;
    }

    /* java.io.File separator constants are core Java state on Android/Linux. */
    SsrnxJniValue zero = {0};
    SsrnxResult rc = ssrnx_jni_register_field("java/io/File", "separator", "Ljava/lang/String;", true, zero);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_field("java/io/File", "separatorChar", "C", true, zero);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_field("java/io/File", "pathSeparator", "Ljava/lang/String;", true, zero);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_field("java/io/File", "pathSeparatorChar", "C", true, zero);
    if (rc != SSRNX_OK) return rc;
    void *env = ssrnx_jni_current_env();
    if (env) {
        void *slash = ssrnx_jni_new_local_string_utf(env, "/");
        void *colon = ssrnx_jni_new_local_string_utf(env, ":");
        SsrnxJniValue v = {0};
        if (slash) { v.l = slash; (void)ssrnx_jni_set_static_field_value("java/io/File", "separator", "Ljava/lang/String;", v); }
        if (colon) { v.l = colon; (void)ssrnx_jni_set_static_field_value("java/io/File", "pathSeparator", "Ljava/lang/String;", v); }
        memset(&v, 0, sizeof(v)); v.c = (uint16_t)'/';
        (void)ssrnx_jni_set_static_field_value("java/io/File", "separatorChar", "C", v);
        memset(&v, 0, sizeof(v)); v.c = (uint16_t)':';
        (void)ssrnx_jni_set_static_field_value("java/io/File", "pathSeparatorChar", "C", v);
        if (slash) ssrnx_jni_delete_local_ref_host(env, slash);
        if (colon) ssrnx_jni_delete_local_ref_host(env, colon);
    }
    return ssrnx_java_lang_numbers_register_members();
}

void ssrnx_java_lang_reset(void) {
    pthread_mutex_lock(&java_lang_g_lang_lock);
    for (java_lang_SbPayload *p = java_lang_g_builders; p;) {
        java_lang_SbPayload *next = p->next;
        pthread_mutex_destroy(&p->lock);
        free(p->buf);
        free(p);
        p = next;
    }
    java_lang_g_builders = NULL;
    for (size_t i = 0; i < INTERN_BUCKETS; ++i) {
        for (java_lang_InternEntry *e = java_lang_g_intern[i]; e;) {
            java_lang_InternEntry *next = e->next;
            free(e->chars);
            free(e);
            e = next;
        }
        java_lang_g_intern[i] = NULL;
    }
    pthread_mutex_unlock(&java_lang_g_lang_lock);
}

#pragma pop_macro("STRING_COMPARATOR")
#pragma pop_macro("STRING_CLASS")
#pragma pop_macro("SIOOBE")
#pragma pop_macro("SB_MAGIC")
#pragma pop_macro("SB_CLASS")
#pragma pop_macro("SBUF_CLASS")
#pragma pop_macro("NPE")
#pragma pop_macro("INTERN_BUCKETS")


/* ---- java_lang_numbers ---- */
#pragma push_macro("AIFU_CLASS")
#pragma push_macro("ALFU_CLASS")
#pragma push_macro("ARFU_CLASS")
#pragma push_macro("CH")
#pragma push_macro("DBL")
#pragma push_macro("FLT")
#pragma push_macro("INT")
#pragma push_macro("LNG")
#pragma push_macro("MTH")
#pragma push_macro("NFE")
#pragma push_macro("PFU_USER")
#pragma push_macro("RET_INT")
/* java.lang numeric and character statics: Character classification and
 * conversion, Integer/Long parsing, radix formatting and bit operations,
 * Float/Double bit views, comparison and parsing, Number conversions and the
 * java.lang.Math functions beyond the basic set. Semantics follow the Java SE
 * API specifications of these classes (NaN canonicalisation, -0.0 ordering,
 * NumberFormatException/ArithmeticException conditions). */


#include <math.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define NFE "java/lang/NumberFormatException"

static SsrnxJniValue java_lang_numbers_v_obj(void *ref) { SsrnxJniValue v = {0}; v.l = ref; return v; }
static SsrnxJniValue java_lang_numbers_v_i(int32_t i) { SsrnxJniValue v = {0}; v.i = i; return v; }
static SsrnxJniValue java_lang_numbers_v_j(int64_t j) { SsrnxJniValue v = {0}; v.j = j; return v; }
static SsrnxJniValue java_lang_numbers_v_f(float f) { SsrnxJniValue v = {0}; v.f = f; return v; }
static SsrnxJniValue java_lang_numbers_v_d(double d) { SsrnxJniValue v = {0}; v.d = d; return v; }
static SsrnxJniValue java_lang_numbers_v_z(bool z) { SsrnxJniValue v = {0}; v.z = z ? 1 : 0; return v; }
static SsrnxJniValue java_lang_numbers_v_c(uint16_t c) { SsrnxJniValue v = {0}; v.c = c; return v; }
static SsrnxJniValue java_lang_numbers_v_b(int8_t b) { SsrnxJniValue v = {0}; v.b = b; return v; }
static SsrnxJniValue java_lang_numbers_v_s(int16_t s) { SsrnxJniValue v = {0}; v.s = s; return v; }

static void *java_lang_numbers_new_ascii(void *env, const char *text) {
    uint16_t buf[80];
    int32_t n = 0;
    for (; text[n] && n < 80; ++n) buf[n] = (uint8_t)text[n];
    return ssrnx_jni_new_local_string_utf16(env, buf, n);
}

static void *java_lang_numbers_box(void *env, const char *cls, const char *sig, SsrnxJniValue value) {
    SsrnxJniValue out = {0};
    bool found = false;
    return ssrnx_jni_invoke_method_host(env, NULL, cls, "valueOf", sig, true, &value, &out, &found) == SSRNX_OK && found
               ? out.l : NULL;
}

/* ------------------------------------------------------------ Character */

static bool java_lang_numbers_cp_whitespace(uint32_t c) {
    return c == ' ' || (c >= 0x09 && c <= 0x0D) || (c >= 0x1C && c <= 0x1F) || c == 0x1680 ||
           (c >= 0x2000 && c <= 0x2006) || (c >= 0x2008 && c <= 0x200A) || c == 0x2028 || c == 0x2029 ||
           c == 0x205F || c == 0x3000;
}

static bool java_lang_numbers_cp_space_char(uint32_t c) {
    return c == ' ' || c == 0xA0 || c == 0x1680 || (c >= 0x2000 && c <= 0x200A) || c == 0x2028 ||
           c == 0x2029 || c == 0x202F || c == 0x205F || c == 0x3000;
}

static bool java_lang_numbers_cp_upper(uint32_t c) { return ssrnx_java_to_lower(c) != c; }
static bool java_lang_numbers_cp_lower(uint32_t c) { return ssrnx_java_to_upper(c) != c || c == 0xDF || c == 0x138; }

/* Character.digit(cp, radix): value of cp in radix, or -1. */
static int32_t java_lang_numbers_cp_digit(uint32_t c, int32_t radix) {
    if (radix < 2 || radix > 36) return -1;
    int v = ssrnx_java_digit_value(c);
    if (v < 0) {
        if (c >= 'a' && c <= 'z') v = (int)(c - 'a') + 10;
        else if (c >= 'A' && c <= 'Z') v = (int)(c - 'A') + 10;
        else if (c >= 0xFF21 && c <= 0xFF3A) v = (int)(c - 0xFF21) + 10;
        else if (c >= 0xFF41 && c <= 0xFF5A) v = (int)(c - 0xFF41) + 10;
    }
    return v >= 0 && v < radix ? v : -1;
}

enum java_lang_numbers_CharOp {
    C_DIGIT_P, C_LETTER, C_LETTER_OR_DIGIT, C_WHITESPACE, C_SPACE, C_UPPER_P, C_LOWER_P, C_ALPHA, C_TO_UPPER,
    C_TO_LOWER, C_HIGH_SURR, C_LOW_SURR, C_SURR, C_SURR_PAIR, C_TO_CODE_POINT, C_CHAR_COUNT, C_DIGIT, C_FOR_DIGIT,
    C_NUMERIC, C_ISO_CONTROL, C_TO_STRING, C_HASH, C_COMPARE, C_VALID, C_BMP, C_SUPPLEMENTARY, C_TO_CHARS,
    C_CP_AT_SEQ, C_CP_AT_CHARS,
};

static SsrnxJniValue java_lang_numbers_char_op(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)token; (void)n;
    const enum java_lang_numbers_CharOp op = (enum java_lang_numbers_CharOp)((intptr_t)u >> 1);
    const bool wide = ((intptr_t)u & 1) != 0; /* int code point argument */
    const uint32_t c = wide ? (uint32_t)a[0].i : a[0].c;
    switch (op) {
        case C_DIGIT_P: return java_lang_numbers_v_z(ssrnx_java_is_digit(c));
        case C_LETTER: return java_lang_numbers_v_z(ssrnx_java_is_letter(c));
        case C_LETTER_OR_DIGIT: return java_lang_numbers_v_z(ssrnx_java_is_letter(c) || ssrnx_java_is_digit(c));
        case C_WHITESPACE: return java_lang_numbers_v_z(java_lang_numbers_cp_whitespace(c));
        case C_SPACE: return java_lang_numbers_v_z(java_lang_numbers_cp_space_char(c));
        case C_UPPER_P: return java_lang_numbers_v_z(java_lang_numbers_cp_upper(c));
        case C_LOWER_P: return java_lang_numbers_v_z(java_lang_numbers_cp_lower(c));
        case C_ALPHA: return java_lang_numbers_v_z(ssrnx_java_is_letter(c));
        case C_TO_UPPER: return wide ? java_lang_numbers_v_i((int32_t)ssrnx_java_to_upper(c)) : java_lang_numbers_v_c((uint16_t)ssrnx_java_to_upper(c));
        case C_TO_LOWER: return wide ? java_lang_numbers_v_i((int32_t)ssrnx_java_to_lower(c)) : java_lang_numbers_v_c((uint16_t)ssrnx_java_to_lower(c));
        case C_HIGH_SURR: return java_lang_numbers_v_z(c >= 0xD800 && c <= 0xDBFF);
        case C_LOW_SURR: return java_lang_numbers_v_z(c >= 0xDC00 && c <= 0xDFFF);
        case C_SURR: return java_lang_numbers_v_z(c >= 0xD800 && c <= 0xDFFF);
        case C_SURR_PAIR: return java_lang_numbers_v_z(a[0].c >= 0xD800 && a[0].c <= 0xDBFF && a[1].c >= 0xDC00 && a[1].c <= 0xDFFF);
        case C_TO_CODE_POINT: return java_lang_numbers_v_i(0x10000 + ((a[0].c - 0xD800) << 10) + (a[1].c - 0xDC00));
        case C_CHAR_COUNT: return java_lang_numbers_v_i(a[0].i >= 0x10000 ? 2 : 1);
        case C_DIGIT: return java_lang_numbers_v_i(java_lang_numbers_cp_digit(c, a[1].i));
        case C_FOR_DIGIT: {
            const int32_t d = a[0].i, radix = a[1].i;
            if (radix < 2 || radix > 36 || d < 0 || d >= radix) return java_lang_numbers_v_c(0);
            return java_lang_numbers_v_c((uint16_t)(d < 10 ? '0' + d : 'a' + d - 10));
        }
        case C_NUMERIC: {
            const int32_t d = java_lang_numbers_cp_digit(c, 36);
            return java_lang_numbers_v_i(d);
        }
        case C_ISO_CONTROL: return java_lang_numbers_v_z(c <= 0x1F || (c >= 0x7F && c <= 0x9F));
        case C_TO_STRING: { const uint16_t ch = a[0].c; return java_lang_numbers_v_obj(ssrnx_jni_new_local_string_utf16(env, &ch, 1)); }
        case C_HASH: return java_lang_numbers_v_i(a[0].c);
        case C_COMPARE: return java_lang_numbers_v_i((int32_t)a[0].c - (int32_t)a[1].c);
        case C_VALID: return java_lang_numbers_v_z(a[0].i >= 0 && a[0].i <= 0x10FFFF);
        case C_BMP: return java_lang_numbers_v_z((uint32_t)a[0].i < 0x10000u);
        case C_SUPPLEMENTARY: return java_lang_numbers_v_z(a[0].i >= 0x10000 && a[0].i <= 0x10FFFF);
        case C_TO_CHARS: {
            const int32_t cp = a[0].i;
            if (cp < 0 || cp > 0x10FFFF) { (void)ssrnx_jni_throw_new_by_name(env, "java/lang/IllegalArgumentException", "Not a valid Unicode code point"); return java_lang_numbers_v_obj(NULL); }
            uint16_t units[2];
            int32_t count = 1;
            if (cp >= 0x10000) { units[0] = (uint16_t)(0xD800 + ((cp - 0x10000) >> 10)); units[1] = (uint16_t)(0xDC00 + ((cp - 0x10000) & 0x3FF)); count = 2; }
            else units[0] = (uint16_t)cp;
            void *array = ssrnx_jni_new_local_char_array(env, count);
            if (array) (void)ssrnx_jni_char_array_write(array, 0, count, units);
            return java_lang_numbers_v_obj(array);
        }
        case C_CP_AT_SEQ: case C_CP_AT_CHARS: {
            int32_t len = 0;
            uint16_t *s = NULL;
            if (op == C_CP_AT_SEQ) s = ssrnx_java_charseq_utf16(env, a[0].l, &len);
            else if (a[0].l) {
                len = ssrnx_jni_array_length_host(a[0].l);
                s = malloc((size_t)(len > 0 ? len : 1) * sizeof(uint16_t));
                if (s && len > 0 && ssrnx_jni_char_array_read(a[0].l, 0, len, s) != SSRNX_OK) { free(s); s = NULL; }
            } else (void)ssrnx_jni_throw_new_by_name(env, "java/lang/NullPointerException", "a");
            if (!s) return java_lang_numbers_v_i(0);
            const int32_t i = a[1].i;
            int32_t cp = 0;
            if (i < 0 || i >= len) (void)ssrnx_jni_throw_new_by_name(env, "java/lang/IndexOutOfBoundsException", "index");
            else {
                cp = s[i];
                if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < len && s[i + 1] >= 0xDC00 && s[i + 1] <= 0xDFFF)
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (s[i + 1] - 0xDC00);
            }
            free(s);
            return java_lang_numbers_v_i(cp);
        }
    }
    return java_lang_numbers_v_i(0);
}

/* ------------------------------------------------------------ integer parsing/formatting */

static void java_lang_numbers_throw_nfe(void *env, const uint16_t *s, int32_t len, int32_t radix) {
    char msg[200];
    size_t n = (size_t)snprintf(msg, sizeof(msg), "For input string: \"");
    for (int32_t i = 0; i < len && n + 24 < sizeof(msg); ++i) msg[n++] = s[i] < 0x80 ? (char)s[i] : '?';
    n += (size_t)snprintf(msg + n, sizeof(msg) - n, "\"");
    if (radix != 10) snprintf(msg + n, sizeof(msg) - n, " under radix %d", (int)radix);
    (void)ssrnx_jni_throw_new_by_name(env, NFE, msg);
}

/* Integer/Long.parseXxx(s, radix) into [min, max]. */
static bool java_lang_numbers_parse_integer(void *env, void *ref, int32_t radix, int64_t min, int64_t max, int64_t *out) {
    if (!ref) { (void)ssrnx_jni_throw_new_by_name(env, NFE, "Cannot parse null string: null"); return false; }
    if (radix < 2 || radix > 36) {
        char msg[64];
        snprintf(msg, sizeof(msg), "radix %d %s", (int)radix, radix < 2 ? "less than Character.MIN_RADIX" : "greater than Character.MAX_RADIX");
        (void)ssrnx_jni_throw_new_by_name(env, NFE, msg);
        return false;
    }
    int32_t len = 0;
    uint16_t *s = ssrnx_jni_string_utf16_dup_host(ref, &len);
    if (!s) return false;
    int32_t i = 0;
    bool negative = false;
    if (len > 0 && (s[0] == '-' || s[0] == '+')) { negative = s[0] == '-'; i = 1; }
    bool ok = i < len;
    /* Accumulate negatively so the most negative value parses. */
    int64_t acc = 0;
    const int64_t limit = negative ? min : -max;
    for (; ok && i < len; ++i) {
        const int32_t d = java_lang_numbers_cp_digit(s[i], radix);
        if (d < 0 || acc < limit / radix) { ok = false; break; }
        acc *= radix;
        if (acc < limit + d) { ok = false; break; }
        acc -= d;
    }
    if (!ok) { java_lang_numbers_throw_nfe(env, s, len, radix); free(s); return false; }
    free(s);
    *out = negative ? acc : -acc;
    return true;
}

/* Byte/Short parsing uses the same Java integral parser as Integer/Long,
 * but constrains the accepted result to the wrapper type's signed range. */
enum java_lang_numbers_NarrowIntOp {
    NARROW_PARSE_BYTE = 1,
    NARROW_PARSE_SHORT,
    NARROW_BYTE_HASH,
    NARROW_SHORT_HASH,
    NARROW_BYTE_TO_UNSIGNED_INT,
    NARROW_SHORT_REVERSE_BYTES,
};

static SsrnxJniValue java_lang_numbers_narrow_int_op(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)token;
    const enum java_lang_numbers_NarrowIntOp op = (enum java_lang_numbers_NarrowIntOp)(intptr_t)u;
    switch (op) {
        case NARROW_PARSE_BYTE: {
            int64_t out = 0;
            const int32_t radix = n == 2 ? a[1].i : 10;
            if (!java_lang_numbers_parse_integer(env, a[0].l, radix, INT8_MIN, INT8_MAX, &out)) return java_lang_numbers_v_b(0);
            return java_lang_numbers_v_b((int8_t)out);
        }
        case NARROW_PARSE_SHORT: {
            int64_t out = 0;
            const int32_t radix = n == 2 ? a[1].i : 10;
            if (!java_lang_numbers_parse_integer(env, a[0].l, radix, INT16_MIN, INT16_MAX, &out)) return java_lang_numbers_v_s(0);
            return java_lang_numbers_v_s((int16_t)out);
        }
        case NARROW_BYTE_HASH: return java_lang_numbers_v_i((int32_t)a[0].b);
        case NARROW_SHORT_HASH: return java_lang_numbers_v_i((int32_t)a[0].s);
        case NARROW_BYTE_TO_UNSIGNED_INT: return java_lang_numbers_v_i((int32_t)(uint8_t)a[0].b);
        case NARROW_SHORT_REVERSE_BYTES: {
            const uint16_t x = (uint16_t)a[0].s;
            return java_lang_numbers_v_s((int16_t)((x << 8) | (x >> 8)));
        }
    }
    return java_lang_numbers_v_i(0);
}

static void *java_lang_numbers_format_radix(void *env, int64_t value, int32_t radix, bool is_unsigned, int bits) {
    if (radix < 2 || radix > 36) radix = 10;
    char buf[72];
    int pos = 71;
    buf[pos] = '\0';
    uint64_t v;
    bool negative = false;
    if (is_unsigned) v = bits == 32 ? (uint64_t)(uint32_t)value : (uint64_t)value;
    else { negative = value < 0; v = negative ? (uint64_t)0 - (uint64_t)value : (uint64_t)value; }
    do { const unsigned d = (unsigned)(v % (unsigned)radix); buf[--pos] = (char)(d < 10 ? '0' + d : 'a' + d - 10); v /= (unsigned)radix; } while (v);
    if (negative) buf[--pos] = '-';
    return java_lang_numbers_new_ascii(env, buf + pos);
}

enum java_lang_numbers_IntOp {
    I_PARSE_RADIX, I_VALUE_OF_RADIX, I_TO_STRING_RADIX, I_BINARY, I_OCTAL, I_HEX, I_BIT_COUNT, I_NLZ, I_NTZ, I_HIGHEST,
    I_LOWEST, I_REVERSE, I_REVERSE_BYTES, I_ROTL, I_ROTR, I_MAX, I_MIN, I_SUM, I_CMP_UNSIGNED, I_TO_UNSIGNED_LONG,
    I_UNSIGNED_STRING, I_PARSE_UNSIGNED, I_DECODE, I_SIGNUM, I_PARSE, I_COMPARE, I_HASH, I_DIVIDE_UNSIGNED,
    I_REMAINDER_UNSIGNED,
};

static SsrnxJniValue java_lang_numbers_int_op(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)token; (void)n;
    const bool is_long = ((intptr_t)u & 1) != 0;
    const enum java_lang_numbers_IntOp op = (enum java_lang_numbers_IntOp)((intptr_t)u >> 1);
    const int bits = is_long ? 64 : 32;
    const int64_t x = is_long ? a[0].j : a[0].i;
    const uint64_t ux = is_long ? (uint64_t)a[0].j : (uint64_t)(uint32_t)a[0].i;
    const char *box_cls = is_long ? "java/lang/Long" : "java/lang/Integer";
    const char *box_sig = is_long ? "(J)Ljava/lang/Long;" : "(I)Ljava/lang/Integer;";
#define RET_INT(v) (is_long ? java_lang_numbers_v_j((int64_t)(v)) : java_lang_numbers_v_i((int32_t)(v)))
    switch (op) {
        case I_PARSE: case I_PARSE_RADIX: case I_VALUE_OF_RADIX: {
            int64_t out = 0;
            const int32_t radix = op == I_PARSE ? 10 : a[1].i;
            if (!java_lang_numbers_parse_integer(env, a[0].l, radix, is_long ? INT64_MIN : INT32_MIN, is_long ? INT64_MAX : INT32_MAX, &out))
                return java_lang_numbers_v_obj(NULL);
            if (op != I_VALUE_OF_RADIX) return RET_INT(out);
            SsrnxJniValue b = RET_INT(out);
            return java_lang_numbers_v_obj(java_lang_numbers_box(env, box_cls, box_sig, b));
        }
        case I_PARSE_UNSIGNED: {
            int64_t out = 0;
            if (!java_lang_numbers_parse_integer(env, a[0].l, n == 2 ? a[1].i : 10, 0, is_long ? INT64_MAX : (int64_t)UINT32_MAX, &out)) return java_lang_numbers_v_obj(NULL);
            return RET_INT(out);
        }
        case I_DECODE: {
            if (!a[0].l) { (void)ssrnx_jni_throw_new_by_name(env, "java/lang/NullPointerException", "nm"); return java_lang_numbers_v_obj(NULL); }
            int32_t len = 0;
            uint16_t *s = ssrnx_jni_string_utf16_dup_host(a[0].l, &len);
            if (!s) return java_lang_numbers_v_obj(NULL);
            int32_t i = 0, radix = 10;
            bool negative = false;
            if (i < len && (s[i] == '-' || s[i] == '+')) negative = s[i++] == '-';
            if (i + 1 < len && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) { radix = 16; i += 2; }
            else if (i < len && s[i] == '#') { radix = 16; i++; }
            else if (i + 1 < len && s[i] == '0') { radix = 8; i++; }
            void *digits = ssrnx_jni_new_local_string_utf16(env, s + i, len - i);
            int64_t out = 0;
            const bool ok = digits && i < len && s[i] != '-' && s[i] != '+' &&
                            java_lang_numbers_parse_integer(env, digits, radix, is_long ? INT64_MIN : INT32_MIN,
                                          is_long ? INT64_MAX : (int64_t)INT32_MAX + 1, &out);
            if (digits) ssrnx_jni_delete_local_ref_host(env, digits);
            if (!ok) {
                if (ssrnx_jni_exception_pending_host(env)) ssrnx_jni_exception_clear_host(env);
                java_lang_numbers_throw_nfe(env, s, len, 10);
                free(s);
                return java_lang_numbers_v_obj(NULL);
            }
            free(s);
            if (negative) out = -out;
            if (!is_long && (out > INT32_MAX || out < INT32_MIN)) { java_lang_numbers_throw_nfe(env, NULL, 0, 10); return java_lang_numbers_v_obj(NULL); }
            SsrnxJniValue b = RET_INT(out);
            return java_lang_numbers_v_obj(java_lang_numbers_box(env, box_cls, box_sig, b));
        }
        case I_TO_STRING_RADIX: return java_lang_numbers_v_obj(java_lang_numbers_format_radix(env, x, a[1].i, false, bits));
        case I_BINARY: return java_lang_numbers_v_obj(java_lang_numbers_format_radix(env, x, 2, true, bits));
        case I_OCTAL: return java_lang_numbers_v_obj(java_lang_numbers_format_radix(env, x, 8, true, bits));
        case I_HEX: return java_lang_numbers_v_obj(java_lang_numbers_format_radix(env, x, 16, true, bits));
        case I_UNSIGNED_STRING: return java_lang_numbers_v_obj(java_lang_numbers_format_radix(env, x, n == 2 ? a[1].i : 10, true, bits));
        case I_BIT_COUNT: return java_lang_numbers_v_i(__builtin_popcountll(ux));
        case I_NLZ: return java_lang_numbers_v_i(ux ? (is_long ? __builtin_clzll(ux) : __builtin_clz((uint32_t)ux)) : bits);
        case I_NTZ: return java_lang_numbers_v_i(ux ? __builtin_ctzll(ux) : bits);
        case I_HIGHEST: return RET_INT(ux ? (uint64_t)1 << (63 - __builtin_clzll(ux)) : 0);
        case I_LOWEST: return RET_INT(ux & (~ux + 1u));
        case I_REVERSE: {
            uint64_t r = 0;
            for (int i = 0; i < bits; ++i) if (ux & ((uint64_t)1 << i)) r |= (uint64_t)1 << (bits - 1 - i);
            return RET_INT(r);
        }
        case I_REVERSE_BYTES: return is_long ? java_lang_numbers_v_j((int64_t)__builtin_bswap64(ux)) : java_lang_numbers_v_i((int32_t)__builtin_bswap32((uint32_t)ux));
        case I_ROTL: case I_ROTR: {
            const unsigned d = (unsigned)a[is_long ? 1 : 1].i & (unsigned)(bits - 1);
            const unsigned k = op == I_ROTL ? d : (unsigned)(bits - (int)d) & (unsigned)(bits - 1);
            if (!k) return RET_INT(ux);
            if (is_long) return java_lang_numbers_v_j((int64_t)((ux << k) | (ux >> (64 - k))));
            const uint32_t w = (uint32_t)ux;
            return java_lang_numbers_v_i((int32_t)((w << k) | (w >> (32 - k))));
        }
        case I_MAX: return is_long ? java_lang_numbers_v_j(a[0].j > a[1].j ? a[0].j : a[1].j) : java_lang_numbers_v_i(a[0].i > a[1].i ? a[0].i : a[1].i);
        case I_MIN: return is_long ? java_lang_numbers_v_j(a[0].j < a[1].j ? a[0].j : a[1].j) : java_lang_numbers_v_i(a[0].i < a[1].i ? a[0].i : a[1].i);
        case I_SUM: return is_long ? java_lang_numbers_v_j((int64_t)((uint64_t)a[0].j + (uint64_t)a[1].j)) : java_lang_numbers_v_i((int32_t)((uint32_t)a[0].i + (uint32_t)a[1].i));
        case I_CMP_UNSIGNED: {
            const uint64_t y = is_long ? (uint64_t)a[1].j : (uint64_t)(uint32_t)a[1].i;
            return java_lang_numbers_v_i(ux < y ? -1 : ux > y);
        }
        case I_COMPARE: {
            const int64_t y = is_long ? a[1].j : a[1].i;
            return java_lang_numbers_v_i(x < y ? -1 : x > y);
        }
        case I_DIVIDE_UNSIGNED: case I_REMAINDER_UNSIGNED: {
            const uint64_t y = is_long ? (uint64_t)a[1].j : (uint64_t)(uint32_t)a[1].i;
            if (!y) { (void)ssrnx_jni_throw_new_by_name(env, "java/lang/ArithmeticException", "/ by zero"); return RET_INT(0); }
            return RET_INT(op == I_DIVIDE_UNSIGNED ? ux / y : ux % y);
        }
        case I_TO_UNSIGNED_LONG: return java_lang_numbers_v_j((int64_t)(uint32_t)a[0].i);
        case I_SIGNUM: return java_lang_numbers_v_i(x > 0 ? 1 : x < 0 ? -1 : 0);
        case I_HASH: return java_lang_numbers_v_i(is_long ? (int32_t)(uint32_t)(ux ^ (ux >> 32)) : a[0].i);
    }
#undef RET_INT
    return java_lang_numbers_v_i(0);
}

/* ------------------------------------------------------------ Float / Double */

static int32_t java_lang_numbers_float_bits(float f) { int32_t b; memcpy(&b, &f, 4); return b; }
static int64_t java_lang_numbers_double_bits(double d) { int64_t b; memcpy(&b, &d, 8); return b; }

/* Float/Double.compare: -0.0 < 0.0, NaN above everything, NaN == NaN. */
static int32_t java_lang_numbers_fp_compare(double a, double b) {
    if (a < b) return -1;
    if (a > b) return 1;
    const int64_t x = isnan(a) ? INT64_C(0x7ff8000000000000) : java_lang_numbers_double_bits(a);
    const int64_t y = isnan(b) ? INT64_C(0x7ff8000000000000) : java_lang_numbers_double_bits(b);
    return x == y ? 0 : x < y ? -1 : 1;
}

/* Double.parseDouble / Float.parseFloat grammar (FloatingDecimal): trimmed
 * text, optional sign, NaN/Infinity, decimal or hex significand with
 * exponent, optional f/F/d/D suffix. */
static bool java_lang_numbers_parse_fp(void *env, void *ref, double *out) {
    if (!ref) { (void)ssrnx_jni_throw_new_by_name(env, "java/lang/NullPointerException", "s"); return false; }
    int32_t len = 0;
    uint16_t *s = ssrnx_jni_string_utf16_dup_host(ref, &len);
    if (!s) return false;
    int32_t b = 0, e = len;
    while (b < e && s[b] <= ' ') b++;
    while (e > b && s[e - 1] <= ' ') e--;
    char text[128];
    bool ok = e - b > 0 && e - b < (int32_t)sizeof(text);
    for (int32_t i = b; ok && i < e; ++i) { if (s[i] >= 0x80) ok = false; else text[i - b] = (char)s[i]; }
    if (ok) {
        text[e - b] = '\0';
        size_t tl = strlen(text);
        const char *body = text + (text[0] == '+' || text[0] == '-');
        const bool neg = text[0] == '-';
        if (!strcmp(body, "NaN")) *out = NAN;
        else if (!strcmp(body, "Infinity")) *out = neg ? -INFINITY : INFINITY;
        else {
            if (tl > 1 && strchr("fFdD", text[tl - 1]) && !(tl > 2 && (text[1] == 'x' || text[1] == 'X' ||
                                                                   text[2] == 'x' || text[2] == 'X') && !strchr(text, 'p') && !strchr(text, 'P')))
                text[--tl] = '\0';
            /* strtod also accepts forms Java rejects (inf, nan, leading spaces handled above). */
            for (size_t i = 0; ok && i < tl; ++i)
                ok = strchr("0123456789+-.eExXpPaAbBcCdDfF", text[i]) != NULL;
            char *end = NULL;
            if (ok) *out = strtod(text, &end);
            ok = ok && end && *end == '\0' && end != text;
        }
    }
    if (!ok) java_lang_numbers_throw_nfe(env, s + b, e - b, 10);
    free(s);
    return ok;
}

enum java_lang_numbers_FpOp {
    F_TO_BITS, F_TO_RAW_BITS, F_FROM_BITS, F_IS_NAN, F_IS_INF, F_IS_FINITE, F_COMPARE, F_PARSE, F_VALUE_OF_STR,
    F_TO_STRING, F_HASH, F_MAX, F_MIN, F_SUM,
};

static SsrnxJniValue java_lang_numbers_fp_op(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)token; (void)n;
    const bool is_double = ((intptr_t)u & 1) != 0;
    const enum java_lang_numbers_FpOp op = (enum java_lang_numbers_FpOp)((intptr_t)u >> 1);
    const double x = is_double ? a[0].d : a[0].f;
    switch (op) {
        case F_TO_BITS:
            if (is_double) return java_lang_numbers_v_j(isnan(x) ? INT64_C(0x7ff8000000000000) : java_lang_numbers_double_bits(a[0].d));
            return java_lang_numbers_v_i(isnan(x) ? 0x7fc00000 : java_lang_numbers_float_bits(a[0].f));
        case F_TO_RAW_BITS: return is_double ? java_lang_numbers_v_j(java_lang_numbers_double_bits(a[0].d)) : java_lang_numbers_v_i(java_lang_numbers_float_bits(a[0].f));
        case F_FROM_BITS: {
            if (is_double) { double d; memcpy(&d, &a[0].j, 8); return java_lang_numbers_v_d(d); }
            float f; memcpy(&f, &a[0].i, 4); return java_lang_numbers_v_f(f);
        }
        case F_IS_NAN: return java_lang_numbers_v_z(isnan(x));
        case F_IS_INF: return java_lang_numbers_v_z(isinf(x));
        case F_IS_FINITE: return java_lang_numbers_v_z(isfinite(x));
        case F_COMPARE: return java_lang_numbers_v_i(java_lang_numbers_fp_compare(x, is_double ? a[1].d : a[1].f));
        case F_PARSE: case F_VALUE_OF_STR: {
            double d = 0;
            if (!java_lang_numbers_parse_fp(env, a[0].l, &d)) return java_lang_numbers_v_obj(NULL);
            if (op == F_PARSE) return is_double ? java_lang_numbers_v_d(d) : java_lang_numbers_v_f((float)d);
            return java_lang_numbers_v_obj(is_double ? java_lang_numbers_box(env, "java/lang/Double", "(D)Ljava/lang/Double;", java_lang_numbers_v_d(d))
                                   : java_lang_numbers_box(env, "java/lang/Float", "(F)Ljava/lang/Float;", java_lang_numbers_v_f((float)d)));
        }
        case F_TO_STRING: {
            char text[64];
            ssrnx_java_floating_to_string(x, !is_double, text, sizeof(text));
            return java_lang_numbers_v_obj(java_lang_numbers_new_ascii(env, text));
        }
        case F_HASH: {
            if (!is_double) return java_lang_numbers_v_i(isnan(x) ? 0x7fc00000 : java_lang_numbers_float_bits(a[0].f));
            const uint64_t bits = (uint64_t)(isnan(x) ? INT64_C(0x7ff8000000000000) : java_lang_numbers_double_bits(a[0].d));
            return java_lang_numbers_v_i((int32_t)(uint32_t)(bits ^ (bits >> 32)));
        }
        case F_MAX: case F_MIN: {
            const double y = is_double ? a[1].d : a[1].f;
            double r;
            if (isnan(x) || isnan(y)) r = NAN;
            else if (x == y && x == 0) r = op == F_MAX ? (signbit(x) ? y : x) : (signbit(x) ? x : y);
            else r = op == F_MAX ? (x > y ? x : y) : (x < y ? x : y);
            return is_double ? java_lang_numbers_v_d(r) : java_lang_numbers_v_f((float)r);
        }
        case F_SUM: return is_double ? java_lang_numbers_v_d(a[0].d + a[1].d) : java_lang_numbers_v_f(a[0].f + a[1].f);
    }
    return java_lang_numbers_v_i(0);
}

/* ------------------------------------------------------------ Number */

static __thread int java_lang_numbers_t_number_depth;

/* Numeric value of a boxed number through its own canonical accessor. */
static bool java_lang_numbers_number_value(void *env, void *obj, int64_t *l, double *d, bool *fp) {
    static const struct { const char *cls, *name, *sig; char kind; } k_accessors[] = {
        {"java/lang/Integer", "intValue", "()I", 'I'}, {"java/lang/Long", "longValue", "()J", 'J'},
        {"java/lang/Float", "floatValue", "()F", 'F'}, {"java/lang/Double", "doubleValue", "()D", 'D'},
        {"java/lang/Short", "shortValue", "()S", 'S'}, {"java/lang/Byte", "byteValue", "()B", 'B'},
        {"java/util/concurrent/atomic/AtomicInteger", "get", "()I", 'I'},
        {"java/util/concurrent/atomic/AtomicLong", "get", "()J", 'J'},
    };
    if (!obj || java_lang_numbers_t_number_depth > 0) return false;
    for (size_t i = 0; i < sizeof(k_accessors) / sizeof(k_accessors[0]); ++i) {
        if (!ssrnx_jni_is_instance_of_class_host(obj, k_accessors[i].cls)) continue;
        SsrnxJniValue out = {0};
        bool found = false;
        java_lang_numbers_t_number_depth++;
        const SsrnxResult rc = ssrnx_jni_invoke_method_host(env, obj, k_accessors[i].cls, k_accessors[i].name,
                                                            k_accessors[i].sig, false, NULL, &out, &found);
        java_lang_numbers_t_number_depth--;
        if (rc != SSRNX_OK || !found) return false;
        switch (k_accessors[i].kind) {
            case 'I': *l = out.i; *d = out.i; *fp = false; break;
            case 'J': *l = out.j; *d = (double)out.j; *fp = false; break;
            case 'S': *l = out.s; *d = out.s; *fp = false; break;
            case 'B': *l = out.b; *d = out.b; *fp = false; break;
            case 'F': *d = out.f; *fp = true; break;
            default: *d = out.d; *fp = true; break;
        }
        return true;
    }
    return false;
}

/* Java narrowing of a double to an integral type (JLS 5.1.3). */
static int64_t java_lang_numbers_d2l(double d) {
    if (isnan(d)) return 0;
    if (d >= 9223372036854775807.0) return INT64_MAX;
    if (d <= -9223372036854775808.0) return INT64_MIN;
    return (int64_t)d;
}
static int32_t java_lang_numbers_d2i(double d) {
    if (isnan(d)) return 0;
    if (d >= 2147483647.0) return INT32_MAX;
    if (d <= -2147483648.0) return INT32_MIN;
    return (int32_t)d;
}

static SsrnxJniValue java_lang_numbers_number_op(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)n;
    const char kind = (char)(intptr_t)u;
    void *self = ssrnx_jni_new_local_from_token(env, token);
    int64_t l = 0;
    double d = 0;
    bool fp = false;
    const bool ok = java_lang_numbers_number_value(env, self, &l, &d, &fp);
    if (self) ssrnx_jni_delete_local_ref_host(env, self);
    if (!ok) { (void)ssrnx_jni_throw_new_by_name(env, "java/lang/AbstractMethodError", "java.lang.Number"); return java_lang_numbers_v_i(0); }
    switch (kind) {
        case 'I': return java_lang_numbers_v_i(fp ? java_lang_numbers_d2i(d) : (int32_t)l);
        case 'J': return java_lang_numbers_v_j(fp ? java_lang_numbers_d2l(d) : l);
        case 'F': return java_lang_numbers_v_f(fp ? (float)d : (float)l);
        case 'D': return java_lang_numbers_v_d(fp ? d : (double)l);
        case 'S': return java_lang_numbers_v_s((int16_t)(fp ? java_lang_numbers_d2i(d) : (int32_t)l));
        case 'B': return java_lang_numbers_v_b((int8_t)(fp ? java_lang_numbers_d2i(d) : (int32_t)l));
        case 'N': return java_lang_numbers_v_z(fp && isnan(d));
        case 'X': return java_lang_numbers_v_z(fp && isinf(d));
        default: return java_lang_numbers_v_i(0);
    }
}


/* ------------------------------------------------ AtomicReferenceFieldUpdater */

#define ARFU_CLASS "java/util/concurrent/atomic/AtomicReferenceFieldUpdater"
static pthread_mutex_t java_lang_numbers_g_arfu_lock = PTHREAD_MUTEX_INITIALIZER;

static bool java_lang_numbers_ascii_from_jstring(void *ref, char *out, size_t cap) {
    if (!ref || !out || cap == 0) return false;
    int32_t len = 0;
    uint16_t *chars = ssrnx_jni_string_utf16_dup_host(ref, &len);
    if (!chars || len < 0 || (size_t)len + 1u > cap) { free(chars); return false; }
    for (int32_t i = 0; i < len; ++i) {
        if (chars[i] > 0x7f) { free(chars); return false; }
        out[i] = (char)chars[i];
    }
    out[len] = '\0';
    free(chars);
    return true;
}

static SsrnxResult java_lang_numbers_arfu_meta(void *env, void *token, char *owner, size_t owner_cap,
                             char *field, size_t field_cap, char *sig, size_t sig_cap) {
    void *self = ssrnx_jni_new_local_from_token(env, token);
    if (!self) return SSRNX_ERR_ARGUMENT;
    SsrnxJniValue cv = {0}, vv = {0}, nv = {0}; bool found = false;
    SsrnxResult rc = ssrnx_jni_get_instance_field_value(env, self, ARFU_CLASS, "targetClass",
                                                         "Ljava/lang/Class;", &cv, &found);
    if (rc != SSRNX_OK || !found || !cv.l) goto fail;
    rc = ssrnx_jni_get_instance_field_value(env, self, ARFU_CLASS, "valueClass",
                                             "Ljava/lang/Class;", &vv, &found);
    if (rc != SSRNX_OK || !found || !vv.l) goto fail;
    rc = ssrnx_jni_get_instance_field_value(env, self, ARFU_CLASS, "fieldName",
                                             "Ljava/lang/String;", &nv, &found);
    if (rc != SSRNX_OK || !found || !nv.l) goto fail;
    if (!ssrnx_jni_class_ref_name_host(cv.l, owner, owner_cap) ||
        !java_lang_numbers_ascii_from_jstring(nv.l, field, field_cap)) goto fail;
    char value_name[512];
    if (!ssrnx_jni_class_ref_name_host(vv.l, value_name, sizeof(value_name))) goto fail;
    if (value_name[0] == '[') {
        if (strlen(value_name) + 1u > sig_cap) goto fail;
        memcpy(sig, value_name, strlen(value_name) + 1u);
    } else {
        const int n = snprintf(sig, sig_cap, "L%s;", value_name);
        if (n <= 0 || (size_t)n >= sig_cap) goto fail;
    }
    ssrnx_jni_delete_local_ref_host(env, cv.l);
    ssrnx_jni_delete_local_ref_host(env, vv.l);
    ssrnx_jni_delete_local_ref_host(env, nv.l);
    ssrnx_jni_delete_local_ref_host(env, self);
    return SSRNX_OK;
fail:
    if (cv.l) ssrnx_jni_delete_local_ref_host(env, cv.l);
    if (vv.l) ssrnx_jni_delete_local_ref_host(env, vv.l);
    if (nv.l) ssrnx_jni_delete_local_ref_host(env, nv.l);
    ssrnx_jni_delete_local_ref_host(env, self);
    return rc == SSRNX_OK ? SSRNX_ERR_APK_UNSUPPORTED : rc;
}

static SsrnxJniValue java_lang_numbers_arfu_new_updater(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)token; (void)u;
    if (n != 3 || !a[0].l || !a[1].l || !a[2].l) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/NullPointerException", "AtomicReferenceFieldUpdater.newUpdater");
        return java_lang_numbers_v_obj(NULL);
    }
    char owner[512] = {0};
    if (!ssrnx_jni_class_ref_name_host(a[0].l, owner, sizeof(owner))) return java_lang_numbers_v_obj(NULL);
    (void)ssrnx_jni_materialize_class_members_host(env, owner);
    void *obj = ssrnx_jni_new_local_object(env, ARFU_CLASS);
    if (!obj) return java_lang_numbers_v_obj(NULL);
    SsrnxJniValue v = {0};
    v.l = a[0].l; (void)ssrnx_jni_set_instance_field_value(obj, "targetClass", "Ljava/lang/Class;", v);
    v.l = a[1].l; (void)ssrnx_jni_set_instance_field_value(obj, "valueClass", "Ljava/lang/Class;", v);
    v.l = a[2].l; (void)ssrnx_jni_set_instance_field_value(obj, "fieldName", "Ljava/lang/String;", v);
    return java_lang_numbers_v_obj(obj);
}

enum java_lang_numbers_ArfuOp { ARFU_GET, ARFU_SET, ARFU_GET_AND_SET, ARFU_CAS };
static SsrnxJniValue java_lang_numbers_arfu_op(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    const enum java_lang_numbers_ArfuOp op = (enum java_lang_numbers_ArfuOp)(intptr_t)u;
    if ((op == ARFU_GET && n != 1) || (op == ARFU_SET && n != 2) ||
        (op == ARFU_GET_AND_SET && n != 2) || (op == ARFU_CAS && n != 3) || !a[0].l)
        return op == ARFU_CAS ? java_lang_numbers_v_z(false) : java_lang_numbers_v_obj(NULL);
    char owner[512], field[256], sig[600];
    if (java_lang_numbers_arfu_meta(env, token, owner, sizeof(owner), field, sizeof(field), sig, sizeof(sig)) != SSRNX_OK)
        return op == ARFU_CAS ? java_lang_numbers_v_z(false) : java_lang_numbers_v_obj(NULL);
    (void)ssrnx_jni_materialize_class_members_host(env, owner);
    pthread_mutex_lock(&java_lang_numbers_g_arfu_lock);
    SsrnxJniValue cur = {0}; bool found = false;
    SsrnxResult rc = ssrnx_jni_get_instance_field_value(env, a[0].l, owner, field, sig, &cur, &found);
    if (rc != SSRNX_OK || !found) {
        pthread_mutex_unlock(&java_lang_numbers_g_arfu_lock);
        return op == ARFU_CAS ? java_lang_numbers_v_z(false) : java_lang_numbers_v_obj(NULL);
    }
    if (op == ARFU_GET) { pthread_mutex_unlock(&java_lang_numbers_g_arfu_lock); return java_lang_numbers_v_obj(cur.l); }
    if (op == ARFU_CAS) {
        const bool same = ssrnx_jni_same_object_host(cur.l, a[1].l);
        if (same) {
            SsrnxJniValue nv = {0}; nv.l = a[2].l; bool set_found = false;
            rc = ssrnx_jni_set_instance_field_value_for_class(a[0].l, owner, field, sig, nv, &set_found);
            found = set_found && rc == SSRNX_OK;
        } else found = false;
        if (cur.l) ssrnx_jni_delete_local_ref_host(env, cur.l);
        pthread_mutex_unlock(&java_lang_numbers_g_arfu_lock);
        return java_lang_numbers_v_z(same && found);
    }
    SsrnxJniValue nv = {0}; nv.l = a[1].l; bool set_found = false;
    rc = ssrnx_jni_set_instance_field_value_for_class(a[0].l, owner, field, sig, nv, &set_found);
    pthread_mutex_unlock(&java_lang_numbers_g_arfu_lock);
    if (rc != SSRNX_OK || !set_found) {
        if (cur.l) ssrnx_jni_delete_local_ref_host(env, cur.l);
        return java_lang_numbers_v_obj(NULL);
    }
    if (op == ARFU_SET) {
        if (cur.l) ssrnx_jni_delete_local_ref_host(env, cur.l);
        SsrnxJniValue z = {0}; return z;
    }
    return java_lang_numbers_v_obj(cur.l);
}


/* ---------------------------------------- AtomicInteger/LongFieldUpdater */

#define AIFU_CLASS "java/util/concurrent/atomic/AtomicIntegerFieldUpdater"
#define ALFU_CLASS "java/util/concurrent/atomic/AtomicLongFieldUpdater"

enum java_lang_numbers_PrimitiveFieldUpdaterKind { PFU_INT = 1, PFU_LONG = 2 };
enum java_lang_numbers_PrimitiveFieldUpdaterOp {
    PFU_GET = 1,
    PFU_SET,
    PFU_GET_AND_SET,
    PFU_CAS,
    PFU_GET_AND_INCREMENT,
    PFU_GET_AND_DECREMENT,
    PFU_GET_AND_ADD,
    PFU_INCREMENT_AND_GET,
    PFU_DECREMENT_AND_GET,
    PFU_ADD_AND_GET,
};

#define PFU_USER(kind, op) ((void *)(intptr_t)(((kind) << 8) | (op)))

static SsrnxResult java_lang_numbers_pfu_meta(void *env, void *token, const char *updater_class,
                            char *owner, size_t owner_cap,
                            char *field, size_t field_cap) {
    void *self = ssrnx_jni_new_local_from_token(env, token);
    if (!self) return SSRNX_ERR_ARGUMENT;
    SsrnxJniValue cv = {0}, nv = {0};
    bool found = false;
    SsrnxResult rc = ssrnx_jni_get_instance_field_value(env, self, updater_class,
                                                         "targetClass", "Ljava/lang/Class;",
                                                         &cv, &found);
    if (rc != SSRNX_OK || !found || !cv.l) goto fail;
    rc = ssrnx_jni_get_instance_field_value(env, self, updater_class,
                                             "fieldName", "Ljava/lang/String;",
                                             &nv, &found);
    if (rc != SSRNX_OK || !found || !nv.l) goto fail;
    if (!ssrnx_jni_class_ref_name_host(cv.l, owner, owner_cap) ||
        !java_lang_numbers_ascii_from_jstring(nv.l, field, field_cap)) goto fail;
    ssrnx_jni_delete_local_ref_host(env, cv.l);
    ssrnx_jni_delete_local_ref_host(env, nv.l);
    ssrnx_jni_delete_local_ref_host(env, self);
    return SSRNX_OK;
fail:
    if (cv.l) ssrnx_jni_delete_local_ref_host(env, cv.l);
    if (nv.l) ssrnx_jni_delete_local_ref_host(env, nv.l);
    ssrnx_jni_delete_local_ref_host(env, self);
    return rc == SSRNX_OK ? SSRNX_ERR_APK_UNSUPPORTED : rc;
}

static SsrnxJniValue java_lang_numbers_pfu_new_updater(void *env, void *token,
                                     const SsrnxJniValue *a, size_t n, void *u) {
    (void)token;
    const char *updater_class = (const char *)u;
    if (!updater_class || n != 2 || !a[0].l || !a[1].l) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/NullPointerException",
                                          "AtomicFieldUpdater.newUpdater");
        return java_lang_numbers_v_obj(NULL);
    }
    char owner[512] = {0};
    if (!ssrnx_jni_class_ref_name_host(a[0].l, owner, sizeof(owner))) return java_lang_numbers_v_obj(NULL);
    (void)ssrnx_jni_materialize_class_members_host(env, owner);
    void *obj = ssrnx_jni_new_local_object(env, updater_class);
    if (!obj) return java_lang_numbers_v_obj(NULL);
    SsrnxJniValue v = {0};
    v.l = a[0].l;
    (void)ssrnx_jni_set_instance_field_value(obj, "targetClass", "Ljava/lang/Class;", v);
    v.l = a[1].l;
    (void)ssrnx_jni_set_instance_field_value(obj, "fieldName", "Ljava/lang/String;", v);
    return java_lang_numbers_v_obj(obj);
}

static SsrnxJniValue java_lang_numbers_pfu_op(void *env, void *token,
                            const SsrnxJniValue *a, size_t n, void *u) {
    const intptr_t packed = (intptr_t)u;
    const enum java_lang_numbers_PrimitiveFieldUpdaterKind kind = (enum java_lang_numbers_PrimitiveFieldUpdaterKind)((packed >> 8) & 0xff);
    const enum java_lang_numbers_PrimitiveFieldUpdaterOp op = (enum java_lang_numbers_PrimitiveFieldUpdaterOp)(packed & 0xff);
    const bool is_long = kind == PFU_LONG;
    const bool is_cas = op == PFU_CAS;
    const bool one_arg = op == PFU_GET || op == PFU_GET_AND_INCREMENT ||
                         op == PFU_GET_AND_DECREMENT || op == PFU_INCREMENT_AND_GET ||
                         op == PFU_DECREMENT_AND_GET;
    const bool two_arg = op == PFU_SET || op == PFU_GET_AND_SET ||
                         op == PFU_GET_AND_ADD || op == PFU_ADD_AND_GET;
    if ((!one_arg && !two_arg && !is_cas) ||
        (one_arg && n != 1) || (two_arg && n != 2) || (is_cas && n != 3) ||
        !a[0].l) {
        return is_cas ? java_lang_numbers_v_z(false) : (is_long ? java_lang_numbers_v_j(0) : java_lang_numbers_v_i(0));
    }

    const char *updater_class = is_long ? ALFU_CLASS : AIFU_CLASS;
    const char *sig = is_long ? "J" : "I";
    char owner[512] = {0}, field[256] = {0};
    if (java_lang_numbers_pfu_meta(env, token, updater_class, owner, sizeof(owner), field, sizeof(field)) != SSRNX_OK)
        return is_cas ? java_lang_numbers_v_z(false) : (is_long ? java_lang_numbers_v_j(0) : java_lang_numbers_v_i(0));
    (void)ssrnx_jni_materialize_class_members_host(env, owner);

    pthread_mutex_lock(&java_lang_numbers_g_arfu_lock);
    SsrnxJniValue cur = {0};
    bool found = false;
    SsrnxResult rc = ssrnx_jni_get_instance_field_value(env, a[0].l, owner, field, sig,
                                                         &cur, &found);
    if (rc != SSRNX_OK || !found) {
        pthread_mutex_unlock(&java_lang_numbers_g_arfu_lock);
        return is_cas ? java_lang_numbers_v_z(false) : (is_long ? java_lang_numbers_v_j(0) : java_lang_numbers_v_i(0));
    }

    if (op == PFU_GET) {
        pthread_mutex_unlock(&java_lang_numbers_g_arfu_lock);
        return is_long ? java_lang_numbers_v_j(cur.j) : java_lang_numbers_v_i(cur.i);
    }

    int64_t current = is_long ? cur.j : (int64_t)cur.i;
    int64_t replacement = current;
    bool return_new = false;
    if (op == PFU_SET || op == PFU_GET_AND_SET) {
        replacement = is_long ? a[1].j : (int64_t)a[1].i;
    } else if (op == PFU_CAS) {
        const int64_t expect = is_long ? a[1].j : (int64_t)a[1].i;
        if (current != expect) {
            pthread_mutex_unlock(&java_lang_numbers_g_arfu_lock);
            return java_lang_numbers_v_z(false);
        }
        replacement = is_long ? a[2].j : (int64_t)a[2].i;
    } else {
        int64_t delta = 0;
        if (op == PFU_GET_AND_INCREMENT || op == PFU_INCREMENT_AND_GET) delta = 1;
        else if (op == PFU_GET_AND_DECREMENT || op == PFU_DECREMENT_AND_GET) delta = -1;
        else delta = is_long ? a[1].j : (int64_t)a[1].i;
        if (is_long)
            replacement = (int64_t)((uint64_t)current + (uint64_t)delta);
        else
            replacement = (int32_t)((uint32_t)(int32_t)current + (uint32_t)(int32_t)delta);
        return_new = op == PFU_INCREMENT_AND_GET || op == PFU_DECREMENT_AND_GET ||
                     op == PFU_ADD_AND_GET;
    }

    SsrnxJniValue nv = {0};
    if (is_long) nv.j = replacement;
    else nv.i = (int32_t)replacement;
    bool set_found = false;
    rc = ssrnx_jni_set_instance_field_value_for_class(a[0].l, owner, field, sig, nv, &set_found);
    pthread_mutex_unlock(&java_lang_numbers_g_arfu_lock);
    if (rc != SSRNX_OK || !set_found)
        return is_cas ? java_lang_numbers_v_z(false) : (is_long ? java_lang_numbers_v_j(0) : java_lang_numbers_v_i(0));

    if (is_cas) return java_lang_numbers_v_z(true);
    if (op == PFU_SET) { SsrnxJniValue z = {0}; return z; }
    const int64_t result = return_new ? replacement : current;
    return is_long ? java_lang_numbers_v_j(result) : java_lang_numbers_v_i((int32_t)result);
}

static SsrnxResult java_lang_numbers_register_primitive_field_updater(const char *clazz,
                                                     enum java_lang_numbers_PrimitiveFieldUpdaterKind kind) {
    SsrnxResult rc = ssrnx_jni_register_class(clazz, "java/lang/Object");
    SsrnxJniValue z = {0};
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_field(clazz, "targetClass", "Ljava/lang/Class;", false, z);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_field(clazz, "fieldName", "Ljava/lang/String;", false, z);
    if (rc != SSRNX_OK) return rc;

    const bool is_long = kind == PFU_LONG;
    const char *new_sig = is_long
        ? "(Ljava/lang/Class;Ljava/lang/String;)Ljava/util/concurrent/atomic/AtomicLongFieldUpdater;"
        : "(Ljava/lang/Class;Ljava/lang/String;)Ljava/util/concurrent/atomic/AtomicIntegerFieldUpdater;";
    const char *get_sig = is_long ? "(Ljava/lang/Object;)J" : "(Ljava/lang/Object;)I";
    const char *set_sig = is_long ? "(Ljava/lang/Object;J)V" : "(Ljava/lang/Object;I)V";
    const char *getset_sig = is_long ? "(Ljava/lang/Object;J)J" : "(Ljava/lang/Object;I)I";
    const char *cas_sig = is_long ? "(Ljava/lang/Object;JJ)Z" : "(Ljava/lang/Object;II)Z";
    const char *one_ret_sig = get_sig;
    const char *add_sig = getset_sig;

    rc = ssrnx_jni_register_method(clazz, "newUpdater", new_sig, true, java_lang_numbers_pfu_new_updater, (void *)clazz);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(clazz, "get", get_sig, false, java_lang_numbers_pfu_op, PFU_USER(kind, PFU_GET));
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(clazz, "set", set_sig, false, java_lang_numbers_pfu_op, PFU_USER(kind, PFU_SET));
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(clazz, "lazySet", set_sig, false, java_lang_numbers_pfu_op, PFU_USER(kind, PFU_SET));
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(clazz, "getAndSet", getset_sig, false, java_lang_numbers_pfu_op, PFU_USER(kind, PFU_GET_AND_SET));
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(clazz, "compareAndSet", cas_sig, false, java_lang_numbers_pfu_op, PFU_USER(kind, PFU_CAS));
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(clazz, "weakCompareAndSet", cas_sig, false, java_lang_numbers_pfu_op, PFU_USER(kind, PFU_CAS));
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(clazz, "getAndIncrement", one_ret_sig, false, java_lang_numbers_pfu_op, PFU_USER(kind, PFU_GET_AND_INCREMENT));
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(clazz, "getAndDecrement", one_ret_sig, false, java_lang_numbers_pfu_op, PFU_USER(kind, PFU_GET_AND_DECREMENT));
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(clazz, "getAndAdd", add_sig, false, java_lang_numbers_pfu_op, PFU_USER(kind, PFU_GET_AND_ADD));
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(clazz, "incrementAndGet", one_ret_sig, false, java_lang_numbers_pfu_op, PFU_USER(kind, PFU_INCREMENT_AND_GET));
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(clazz, "decrementAndGet", one_ret_sig, false, java_lang_numbers_pfu_op, PFU_USER(kind, PFU_DECREMENT_AND_GET));
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(clazz, "addAndGet", add_sig, false, java_lang_numbers_pfu_op, PFU_USER(kind, PFU_ADD_AND_GET));
    return rc;
}

/* ------------------------------------------------------------ Math */

enum java_lang_numbers_MathOp {
    M_TAN, M_ASIN, M_ACOS, M_ATAN, M_ATAN2, M_SINH, M_COSH, M_TANH, M_LOG10, M_LOG1P, M_EXPM1, M_CBRT, M_HYPOT,
    M_TO_RAD, M_TO_DEG, M_SIGNUM_D, M_SIGNUM_F, M_RANDOM, M_RINT, M_COPYSIGN_D, M_COPYSIGN_F, M_FLOOR_DIV_I,
    M_FLOOR_DIV_J, M_FLOOR_MOD_I, M_FLOOR_MOD_J, M_ADD_EXACT_I, M_ADD_EXACT_J, M_SUB_EXACT_I, M_SUB_EXACT_J,
    M_MUL_EXACT_I, M_MUL_EXACT_J, M_TO_INT_EXACT, M_NEGATE_EXACT_I, M_NEGATE_EXACT_J, M_ULP_D, M_ULP_F, M_LOG,
    M_EXP, M_SQRT, M_POW, M_CEIL, M_FLOOR, M_SIN, M_COS, M_ABS_D, M_ABS_F,
};

static uint64_t java_lang_numbers_g_random_state = 0x9E3779B97F4A7C15ull;
static pthread_mutex_t java_lang_numbers_g_random_lock = PTHREAD_MUTEX_INITIALIZER;

static double java_lang_numbers_math_random(void) {
    pthread_mutex_lock(&java_lang_numbers_g_random_lock);
    if (java_lang_numbers_g_random_state == 0x9E3779B97F4A7C15ull) java_lang_numbers_g_random_state ^= (uint64_t)(uintptr_t)&java_lang_numbers_g_random_state ^ (uint64_t)time(NULL);
    /* splitmix64 */
    uint64_t z = (java_lang_numbers_g_random_state += 0x9E3779B97F4A7C15ull);
    pthread_mutex_unlock(&java_lang_numbers_g_random_lock);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    z ^= z >> 31;
    return (double)(z >> 11) * (1.0 / 9007199254740992.0);
}

static SsrnxJniValue java_lang_numbers_arith_overflow(void *env, const char *what, bool is_long) {
    (void)ssrnx_jni_throw_new_by_name(env, "java/lang/ArithmeticException", what);
    return is_long ? java_lang_numbers_v_j(0) : java_lang_numbers_v_i(0);
}

static SsrnxJniValue java_lang_numbers_math_op(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)token; (void)n;
    const enum java_lang_numbers_MathOp op = (enum java_lang_numbers_MathOp)(intptr_t)u;
    switch (op) {
        case M_TAN: return java_lang_numbers_v_d(tan(a[0].d));
        case M_ASIN: return java_lang_numbers_v_d(asin(a[0].d));
        case M_ACOS: return java_lang_numbers_v_d(acos(a[0].d));
        case M_ATAN: return java_lang_numbers_v_d(atan(a[0].d));
        case M_ATAN2: return java_lang_numbers_v_d(atan2(a[0].d, a[1].d));
        case M_SINH: return java_lang_numbers_v_d(sinh(a[0].d));
        case M_COSH: return java_lang_numbers_v_d(cosh(a[0].d));
        case M_TANH: return java_lang_numbers_v_d(tanh(a[0].d));
        case M_LOG10: return java_lang_numbers_v_d(log10(a[0].d));
        case M_LOG1P: return java_lang_numbers_v_d(log1p(a[0].d));
        case M_EXPM1: return java_lang_numbers_v_d(expm1(a[0].d));
        case M_CBRT: return java_lang_numbers_v_d(cbrt(a[0].d));
        case M_HYPOT: return java_lang_numbers_v_d(hypot(a[0].d, a[1].d));
        case M_TO_RAD: return java_lang_numbers_v_d(a[0].d / 180.0 * 3.14159265358979323846);
        case M_TO_DEG: return java_lang_numbers_v_d(a[0].d * 180.0 / 3.14159265358979323846);
        case M_SIGNUM_D: return java_lang_numbers_v_d(a[0].d == 0 || isnan(a[0].d) ? a[0].d : copysign(1.0, a[0].d));
        case M_SIGNUM_F: return java_lang_numbers_v_f(a[0].f == 0 || isnan(a[0].f) ? a[0].f : copysignf(1.0f, a[0].f));
        case M_RANDOM: return java_lang_numbers_v_d(java_lang_numbers_math_random());
        case M_RINT: return java_lang_numbers_v_d(nearbyint(a[0].d));
        case M_COPYSIGN_D: return java_lang_numbers_v_d(copysign(a[0].d, a[1].d));
        case M_COPYSIGN_F: return java_lang_numbers_v_f(copysignf(a[0].f, a[1].f));
        case M_FLOOR_DIV_I: case M_FLOOR_MOD_I: {
            const int32_t x = a[0].i, y = a[1].i;
            if (!y) return java_lang_numbers_arith_overflow(env, "/ by zero", false);
            if (x == INT32_MIN && y == -1) return java_lang_numbers_v_i(op == M_FLOOR_DIV_I ? INT32_MIN : 0);
            int32_t q = x / y;
            if ((x % y != 0) && ((x ^ y) < 0)) q--;
            return java_lang_numbers_v_i(op == M_FLOOR_DIV_I ? q : x - q * y);
        }
        case M_FLOOR_DIV_J: case M_FLOOR_MOD_J: {
            const int64_t x = a[0].j, y = a[1].j;
            if (!y) return java_lang_numbers_arith_overflow(env, "/ by zero", true);
            if (x == INT64_MIN && y == -1) return java_lang_numbers_v_j(op == M_FLOOR_DIV_J ? INT64_MIN : 0);
            int64_t q = x / y;
            if ((x % y != 0) && ((x ^ y) < 0)) q--;
            return java_lang_numbers_v_j(op == M_FLOOR_DIV_J ? q : x - q * y);
        }
        case M_ADD_EXACT_I: { int32_t r; if (__builtin_add_overflow(a[0].i, a[1].i, &r)) return java_lang_numbers_arith_overflow(env, "integer overflow", false); return java_lang_numbers_v_i(r); }
        case M_ADD_EXACT_J: { int64_t r; if (__builtin_add_overflow(a[0].j, a[1].j, &r)) return java_lang_numbers_arith_overflow(env, "long overflow", true); return java_lang_numbers_v_j(r); }
        case M_SUB_EXACT_I: { int32_t r; if (__builtin_sub_overflow(a[0].i, a[1].i, &r)) return java_lang_numbers_arith_overflow(env, "integer overflow", false); return java_lang_numbers_v_i(r); }
        case M_SUB_EXACT_J: { int64_t r; if (__builtin_sub_overflow(a[0].j, a[1].j, &r)) return java_lang_numbers_arith_overflow(env, "long overflow", true); return java_lang_numbers_v_j(r); }
        case M_MUL_EXACT_I: { int32_t r; if (__builtin_mul_overflow(a[0].i, a[1].i, &r)) return java_lang_numbers_arith_overflow(env, "integer overflow", false); return java_lang_numbers_v_i(r); }
        case M_MUL_EXACT_J: { int64_t r; if (__builtin_mul_overflow(a[0].j, a[1].j, &r)) return java_lang_numbers_arith_overflow(env, "long overflow", true); return java_lang_numbers_v_j(r); }
        case M_NEGATE_EXACT_I: if (a[0].i == INT32_MIN) return java_lang_numbers_arith_overflow(env, "integer overflow", false); return java_lang_numbers_v_i(-a[0].i);
        case M_NEGATE_EXACT_J: if (a[0].j == INT64_MIN) return java_lang_numbers_arith_overflow(env, "long overflow", true); return java_lang_numbers_v_j(-a[0].j);
        case M_TO_INT_EXACT: if (a[0].j > INT32_MAX || a[0].j < INT32_MIN) return java_lang_numbers_arith_overflow(env, "integer overflow", false); return java_lang_numbers_v_i((int32_t)a[0].j);
        case M_ULP_D: return java_lang_numbers_v_d(isnan(a[0].d) ? a[0].d : isinf(a[0].d) ? INFINITY : nextafter(fabs(a[0].d), INFINITY) - fabs(a[0].d));
        case M_ULP_F: return java_lang_numbers_v_f(isnan(a[0].f) ? a[0].f : isinf(a[0].f) ? INFINITY : nextafterf(fabsf(a[0].f), INFINITY) - fabsf(a[0].f));
        case M_LOG: return java_lang_numbers_v_d(log(a[0].d));
        case M_EXP: return java_lang_numbers_v_d(exp(a[0].d));
        case M_SQRT: return java_lang_numbers_v_d(sqrt(a[0].d));
        case M_POW: return java_lang_numbers_v_d(pow(a[0].d, a[1].d));
        case M_CEIL: return java_lang_numbers_v_d(ceil(a[0].d));
        case M_FLOOR: return java_lang_numbers_v_d(floor(a[0].d));
        case M_SIN: return java_lang_numbers_v_d(sin(a[0].d));
        case M_COS: return java_lang_numbers_v_d(cos(a[0].d));
        case M_ABS_D: return java_lang_numbers_v_d(fabs(a[0].d));
        case M_ABS_F: return java_lang_numbers_v_f(fabsf(a[0].f));
    }
    return java_lang_numbers_v_i(0);
}

/* ------------------------------------------------------------ registration */

typedef struct java_lang_numbers_NumMethod { const char *cls, *name, *sig; bool is_static; SsrnxJniMethodCallback cb; intptr_t op; } java_lang_numbers_NumMethod;
#define CH(name, sig, op, wide) {"java/lang/Character", name, sig, true, java_lang_numbers_char_op, ((intptr_t)(op) << 1) | (wide)}
#define INT(name, sig, op) {"java/lang/Integer", name, sig, true, java_lang_numbers_int_op, (intptr_t)(op) << 1}
#define LNG(name, sig, op) {"java/lang/Long", name, sig, true, java_lang_numbers_int_op, ((intptr_t)(op) << 1) | 1}
#define FLT(name, sig, op) {"java/lang/Float", name, sig, true, java_lang_numbers_fp_op, (intptr_t)(op) << 1}
#define DBL(name, sig, op) {"java/lang/Double", name, sig, true, java_lang_numbers_fp_op, ((intptr_t)(op) << 1) | 1}
#define MTH(name, sig, op) {"java/lang/Math", name, sig, true, java_lang_numbers_math_op, op}, {"java/lang/StrictMath", name, sig, true, java_lang_numbers_math_op, op}

static const java_lang_numbers_NumMethod java_lang_numbers_k_number_methods[] = {
    CH("isDigit", "(C)Z", C_DIGIT_P, 0), CH("isDigit", "(I)Z", C_DIGIT_P, 1),
    CH("isLetter", "(C)Z", C_LETTER, 0), CH("isLetter", "(I)Z", C_LETTER, 1),
    CH("isLetterOrDigit", "(C)Z", C_LETTER_OR_DIGIT, 0), CH("isLetterOrDigit", "(I)Z", C_LETTER_OR_DIGIT, 1),
    CH("isWhitespace", "(C)Z", C_WHITESPACE, 0), CH("isWhitespace", "(I)Z", C_WHITESPACE, 1),
    CH("isSpaceChar", "(C)Z", C_SPACE, 0), CH("isSpaceChar", "(I)Z", C_SPACE, 1),
    CH("isUpperCase", "(C)Z", C_UPPER_P, 0), CH("isUpperCase", "(I)Z", C_UPPER_P, 1),
    CH("isLowerCase", "(C)Z", C_LOWER_P, 0), CH("isLowerCase", "(I)Z", C_LOWER_P, 1),
    CH("isAlphabetic", "(I)Z", C_ALPHA, 1),
    CH("toUpperCase", "(C)C", C_TO_UPPER, 0), CH("toUpperCase", "(I)I", C_TO_UPPER, 1),
    CH("toLowerCase", "(C)C", C_TO_LOWER, 0), CH("toLowerCase", "(I)I", C_TO_LOWER, 1),
    CH("isHighSurrogate", "(C)Z", C_HIGH_SURR, 0), CH("isLowSurrogate", "(C)Z", C_LOW_SURR, 0),
    CH("isSurrogate", "(C)Z", C_SURR, 0), CH("isSurrogatePair", "(CC)Z", C_SURR_PAIR, 0),
    CH("toCodePoint", "(CC)I", C_TO_CODE_POINT, 0), CH("charCount", "(I)I", C_CHAR_COUNT, 1),
    CH("digit", "(CI)I", C_DIGIT, 0), CH("digit", "(II)I", C_DIGIT, 1), CH("forDigit", "(II)C", C_FOR_DIGIT, 1),
    CH("getNumericValue", "(C)I", C_NUMERIC, 0), CH("getNumericValue", "(I)I", C_NUMERIC, 1),
    CH("isISOControl", "(C)Z", C_ISO_CONTROL, 0), CH("toString", "(C)Ljava/lang/String;", C_TO_STRING, 0),
    CH("hashCode", "(C)I", C_HASH, 0), CH("compare", "(CC)I", C_COMPARE, 0),
    CH("isValidCodePoint", "(I)Z", C_VALID, 1), CH("isBmpCodePoint", "(I)Z", C_BMP, 1),
    CH("isSupplementaryCodePoint", "(I)Z", C_SUPPLEMENTARY, 1), CH("toChars", "(I)[C", C_TO_CHARS, 1),
    CH("codePointAt", "(Ljava/lang/CharSequence;I)I", C_CP_AT_SEQ, 0), CH("codePointAt", "([CI)I", C_CP_AT_CHARS, 0),

    {"java/lang/Byte", "parseByte", "(Ljava/lang/String;)B", true, java_lang_numbers_narrow_int_op, NARROW_PARSE_BYTE},
    {"java/lang/Byte", "parseByte", "(Ljava/lang/String;I)B", true, java_lang_numbers_narrow_int_op, NARROW_PARSE_BYTE},
    {"java/lang/Byte", "hashCode", "(B)I", true, java_lang_numbers_narrow_int_op, NARROW_BYTE_HASH},
    {"java/lang/Byte", "toUnsignedInt", "(B)I", true, java_lang_numbers_narrow_int_op, NARROW_BYTE_TO_UNSIGNED_INT},
    {"java/lang/Short", "parseShort", "(Ljava/lang/String;)S", true, java_lang_numbers_narrow_int_op, NARROW_PARSE_SHORT},
    {"java/lang/Short", "parseShort", "(Ljava/lang/String;I)S", true, java_lang_numbers_narrow_int_op, NARROW_PARSE_SHORT},
    {"java/lang/Short", "hashCode", "(S)I", true, java_lang_numbers_narrow_int_op, NARROW_SHORT_HASH},
    {"java/lang/Short", "reverseBytes", "(S)S", true, java_lang_numbers_narrow_int_op, NARROW_SHORT_REVERSE_BYTES},

    INT("parseInt", "(Ljava/lang/String;)I", I_PARSE), INT("parseInt", "(Ljava/lang/String;I)I", I_PARSE_RADIX),
    INT("valueOf", "(Ljava/lang/String;I)Ljava/lang/Integer;", I_VALUE_OF_RADIX),
    INT("toString", "(II)Ljava/lang/String;", I_TO_STRING_RADIX), INT("toBinaryString", "(I)Ljava/lang/String;", I_BINARY),
    INT("toOctalString", "(I)Ljava/lang/String;", I_OCTAL), INT("toHexString", "(I)Ljava/lang/String;", I_HEX),
    INT("toUnsignedString", "(I)Ljava/lang/String;", I_UNSIGNED_STRING), INT("toUnsignedString", "(II)Ljava/lang/String;", I_UNSIGNED_STRING),
    INT("parseUnsignedInt", "(Ljava/lang/String;)I", I_PARSE_UNSIGNED), INT("parseUnsignedInt", "(Ljava/lang/String;I)I", I_PARSE_UNSIGNED),
    INT("bitCount", "(I)I", I_BIT_COUNT), INT("numberOfLeadingZeros", "(I)I", I_NLZ),
    INT("numberOfTrailingZeros", "(I)I", I_NTZ), INT("highestOneBit", "(I)I", I_HIGHEST), INT("lowestOneBit", "(I)I", I_LOWEST),
    INT("reverse", "(I)I", I_REVERSE), INT("reverseBytes", "(I)I", I_REVERSE_BYTES),
    INT("rotateLeft", "(II)I", I_ROTL), INT("rotateRight", "(II)I", I_ROTR), INT("max", "(II)I", I_MAX),
    INT("min", "(II)I", I_MIN), INT("sum", "(II)I", I_SUM), INT("compareUnsigned", "(II)I", I_CMP_UNSIGNED),
    INT("compare", "(II)I", I_COMPARE), INT("toUnsignedLong", "(I)J", I_TO_UNSIGNED_LONG),
    INT("divideUnsigned", "(II)I", I_DIVIDE_UNSIGNED), INT("remainderUnsigned", "(II)I", I_REMAINDER_UNSIGNED),
    INT("decode", "(Ljava/lang/String;)Ljava/lang/Integer;", I_DECODE), INT("signum", "(I)I", I_SIGNUM),
    INT("hashCode", "(I)I", I_HASH),

    LNG("parseLong", "(Ljava/lang/String;)J", I_PARSE), LNG("parseLong", "(Ljava/lang/String;I)J", I_PARSE_RADIX),
    LNG("valueOf", "(Ljava/lang/String;I)Ljava/lang/Long;", I_VALUE_OF_RADIX),
    LNG("toString", "(JI)Ljava/lang/String;", I_TO_STRING_RADIX), LNG("toBinaryString", "(J)Ljava/lang/String;", I_BINARY),
    LNG("toOctalString", "(J)Ljava/lang/String;", I_OCTAL), LNG("toHexString", "(J)Ljava/lang/String;", I_HEX),
    LNG("toUnsignedString", "(J)Ljava/lang/String;", I_UNSIGNED_STRING), LNG("toUnsignedString", "(JI)Ljava/lang/String;", I_UNSIGNED_STRING),
    LNG("parseUnsignedLong", "(Ljava/lang/String;)J", I_PARSE_UNSIGNED),
    LNG("bitCount", "(J)I", I_BIT_COUNT), LNG("numberOfLeadingZeros", "(J)I", I_NLZ),
    LNG("numberOfTrailingZeros", "(J)I", I_NTZ), LNG("highestOneBit", "(J)J", I_HIGHEST), LNG("lowestOneBit", "(J)J", I_LOWEST),
    LNG("reverse", "(J)J", I_REVERSE), LNG("reverseBytes", "(J)J", I_REVERSE_BYTES),
    LNG("rotateLeft", "(JI)J", I_ROTL), LNG("rotateRight", "(JI)J", I_ROTR), LNG("max", "(JJ)J", I_MAX),
    LNG("min", "(JJ)J", I_MIN), LNG("sum", "(JJ)J", I_SUM), LNG("compareUnsigned", "(JJ)I", I_CMP_UNSIGNED),
    LNG("compare", "(JJ)I", I_COMPARE), LNG("divideUnsigned", "(JJ)J", I_DIVIDE_UNSIGNED),
    LNG("remainderUnsigned", "(JJ)J", I_REMAINDER_UNSIGNED),
    LNG("decode", "(Ljava/lang/String;)Ljava/lang/Long;", I_DECODE), LNG("signum", "(J)I", I_SIGNUM),
    LNG("hashCode", "(J)I", I_HASH),

    FLT("floatToIntBits", "(F)I", F_TO_BITS), FLT("floatToRawIntBits", "(F)I", F_TO_RAW_BITS),
    FLT("intBitsToFloat", "(I)F", F_FROM_BITS), FLT("isNaN", "(F)Z", F_IS_NAN), FLT("isInfinite", "(F)Z", F_IS_INF),
    FLT("isFinite", "(F)Z", F_IS_FINITE), FLT("compare", "(FF)I", F_COMPARE), FLT("parseFloat", "(Ljava/lang/String;)F", F_PARSE),
    FLT("valueOf", "(Ljava/lang/String;)Ljava/lang/Float;", F_VALUE_OF_STR), FLT("toString", "(F)Ljava/lang/String;", F_TO_STRING),
    FLT("hashCode", "(F)I", F_HASH), FLT("max", "(FF)F", F_MAX), FLT("min", "(FF)F", F_MIN), FLT("sum", "(FF)F", F_SUM),

    DBL("doubleToLongBits", "(D)J", F_TO_BITS), DBL("doubleToRawLongBits", "(D)J", F_TO_RAW_BITS),
    DBL("longBitsToDouble", "(J)D", F_FROM_BITS), DBL("isNaN", "(D)Z", F_IS_NAN), DBL("isInfinite", "(D)Z", F_IS_INF),
    DBL("isFinite", "(D)Z", F_IS_FINITE), DBL("compare", "(DD)I", F_COMPARE), DBL("parseDouble", "(Ljava/lang/String;)D", F_PARSE),
    DBL("valueOf", "(Ljava/lang/String;)Ljava/lang/Double;", F_VALUE_OF_STR), DBL("toString", "(D)Ljava/lang/String;", F_TO_STRING),
    DBL("hashCode", "(D)I", F_HASH), DBL("max", "(DD)D", F_MAX), DBL("min", "(DD)D", F_MIN), DBL("sum", "(DD)D", F_SUM),

    {"java/lang/Number", "intValue", "()I", false, java_lang_numbers_number_op, 'I'},
    {"java/lang/Number", "longValue", "()J", false, java_lang_numbers_number_op, 'J'},
    {"java/lang/Number", "floatValue", "()F", false, java_lang_numbers_number_op, 'F'},
    {"java/lang/Number", "doubleValue", "()D", false, java_lang_numbers_number_op, 'D'},
    {"java/lang/Number", "shortValue", "()S", false, java_lang_numbers_number_op, 'S'},
    {"java/lang/Number", "byteValue", "()B", false, java_lang_numbers_number_op, 'B'},
    {"java/lang/Float", "isNaN", "()Z", false, java_lang_numbers_number_op, 'N'}, {"java/lang/Double", "isNaN", "()Z", false, java_lang_numbers_number_op, 'N'},
    {"java/lang/Float", "isInfinite", "()Z", false, java_lang_numbers_number_op, 'X'}, {"java/lang/Double", "isInfinite", "()Z", false, java_lang_numbers_number_op, 'X'},

    MTH("tan", "(D)D", M_TAN), MTH("asin", "(D)D", M_ASIN), MTH("acos", "(D)D", M_ACOS), MTH("atan", "(D)D", M_ATAN),
    MTH("atan2", "(DD)D", M_ATAN2), MTH("sinh", "(D)D", M_SINH), MTH("cosh", "(D)D", M_COSH), MTH("tanh", "(D)D", M_TANH),
    MTH("log10", "(D)D", M_LOG10), MTH("log1p", "(D)D", M_LOG1P), MTH("expm1", "(D)D", M_EXPM1), MTH("cbrt", "(D)D", M_CBRT),
    MTH("hypot", "(DD)D", M_HYPOT), MTH("toRadians", "(D)D", M_TO_RAD), MTH("toDegrees", "(D)D", M_TO_DEG),
    MTH("signum", "(D)D", M_SIGNUM_D), MTH("signum", "(F)F", M_SIGNUM_F), MTH("random", "()D", M_RANDOM),
    MTH("rint", "(D)D", M_RINT), MTH("copySign", "(DD)D", M_COPYSIGN_D), MTH("copySign", "(FF)F", M_COPYSIGN_F),
    MTH("floorDiv", "(II)I", M_FLOOR_DIV_I), MTH("floorDiv", "(JJ)J", M_FLOOR_DIV_J),
    MTH("floorMod", "(II)I", M_FLOOR_MOD_I), MTH("floorMod", "(JJ)J", M_FLOOR_MOD_J),
    MTH("addExact", "(II)I", M_ADD_EXACT_I), MTH("addExact", "(JJ)J", M_ADD_EXACT_J),
    MTH("subtractExact", "(II)I", M_SUB_EXACT_I), MTH("subtractExact", "(JJ)J", M_SUB_EXACT_J),
    MTH("multiplyExact", "(II)I", M_MUL_EXACT_I), MTH("multiplyExact", "(JJ)J", M_MUL_EXACT_J),
    MTH("negateExact", "(I)I", M_NEGATE_EXACT_I), MTH("negateExact", "(J)J", M_NEGATE_EXACT_J),
    MTH("toIntExact", "(J)I", M_TO_INT_EXACT), MTH("ulp", "(D)D", M_ULP_D), MTH("ulp", "(F)F", M_ULP_F),
    /* StrictMath mirrors the Math functions already modeled. */
    {"java/lang/StrictMath", "log", "(D)D", true, java_lang_numbers_math_op, M_LOG}, {"java/lang/StrictMath", "exp", "(D)D", true, java_lang_numbers_math_op, M_EXP},
    {"java/lang/StrictMath", "sqrt", "(D)D", true, java_lang_numbers_math_op, M_SQRT}, {"java/lang/StrictMath", "pow", "(DD)D", true, java_lang_numbers_math_op, M_POW},
    {"java/lang/StrictMath", "ceil", "(D)D", true, java_lang_numbers_math_op, M_CEIL}, {"java/lang/StrictMath", "floor", "(D)D", true, java_lang_numbers_math_op, M_FLOOR},
    {"java/lang/StrictMath", "sin", "(D)D", true, java_lang_numbers_math_op, M_SIN}, {"java/lang/StrictMath", "cos", "(D)D", true, java_lang_numbers_math_op, M_COS},
    {"java/lang/StrictMath", "abs", "(D)D", true, java_lang_numbers_math_op, M_ABS_D}, {"java/lang/StrictMath", "abs", "(F)F", true, java_lang_numbers_math_op, M_ABS_F},
};

SsrnxResult ssrnx_java_lang_numbers_register_members(void) {
    SsrnxResult rc = ssrnx_jni_register_class("java/lang/StrictMath", "java/lang/Object");
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_class("java/lang/Math", "java/lang/Object");
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_class("java/lang/ArithmeticException", "java/lang/RuntimeException");
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_class(NFE, "java/lang/IllegalArgumentException");
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_class(ARFU_CLASS, "java/lang/Object");
    if (rc == SSRNX_OK) { SsrnxJniValue z = {0}; rc = ssrnx_jni_register_field(ARFU_CLASS, "targetClass", "Ljava/lang/Class;", false, z); }
    if (rc == SSRNX_OK) { SsrnxJniValue z = {0}; rc = ssrnx_jni_register_field(ARFU_CLASS, "valueClass", "Ljava/lang/Class;", false, z); }
    if (rc == SSRNX_OK) { SsrnxJniValue z = {0}; rc = ssrnx_jni_register_field(ARFU_CLASS, "fieldName", "Ljava/lang/String;", false, z); }
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(ARFU_CLASS, "newUpdater", "(Ljava/lang/Class;Ljava/lang/Class;Ljava/lang/String;)Ljava/util/concurrent/atomic/AtomicReferenceFieldUpdater;", true, java_lang_numbers_arfu_new_updater, NULL);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(ARFU_CLASS, "get", "(Ljava/lang/Object;)Ljava/lang/Object;", false, java_lang_numbers_arfu_op, (void *)(intptr_t)ARFU_GET);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(ARFU_CLASS, "set", "(Ljava/lang/Object;Ljava/lang/Object;)V", false, java_lang_numbers_arfu_op, (void *)(intptr_t)ARFU_SET);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(ARFU_CLASS, "lazySet", "(Ljava/lang/Object;Ljava/lang/Object;)V", false, java_lang_numbers_arfu_op, (void *)(intptr_t)ARFU_SET);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(ARFU_CLASS, "getAndSet", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;", false, java_lang_numbers_arfu_op, (void *)(intptr_t)ARFU_GET_AND_SET);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(ARFU_CLASS, "compareAndSet", "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)Z", false, java_lang_numbers_arfu_op, (void *)(intptr_t)ARFU_CAS);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method(ARFU_CLASS, "weakCompareAndSet", "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)Z", false, java_lang_numbers_arfu_op, (void *)(intptr_t)ARFU_CAS);
    if (rc == SSRNX_OK) rc = java_lang_numbers_register_primitive_field_updater(AIFU_CLASS, PFU_INT);
    if (rc == SSRNX_OK) rc = java_lang_numbers_register_primitive_field_updater(ALFU_CLASS, PFU_LONG);
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(java_lang_numbers_k_number_methods) / sizeof(java_lang_numbers_k_number_methods[0]); ++i) {
        const java_lang_numbers_NumMethod *m = &java_lang_numbers_k_number_methods[i];
        rc = ssrnx_jni_register_method(m->cls, m->name, m->sig, m->is_static, m->cb, (void *)m->op);
    }
    return rc;
}

#pragma pop_macro("RET_INT")
#pragma pop_macro("PFU_USER")
#pragma pop_macro("NFE")
#pragma pop_macro("MTH")
#pragma pop_macro("LNG")
#pragma pop_macro("INT")
#pragma pop_macro("FLT")
#pragma pop_macro("DBL")
#pragma pop_macro("CH")
#pragma pop_macro("ARFU_CLASS")
#pragma pop_macro("ALFU_CLASS")
#pragma pop_macro("AIFU_CLASS")


/* ---- java_annotations ---- */
#pragma push_macro("ACC_ANNOTATION")
#pragma push_macro("ANNOTATION_ARRAY")
#pragma push_macro("ANNOTATION_IFACE")
#pragma push_macro("FACTORY_CLASS")
#pragma push_macro("INCOMPLETE_CLASS")
#pragma push_macro("MAX_NESTING")
#pragma push_macro("PAYLOAD_MAGIC")
#pragma push_macro("TARGET_ENTRY")
#pragma push_macro("VIS_RUNTIME")
#pragma push_macro("VIS_SYSTEM")
/* Java annotation reflection over DEX annotation metadata.
 *
 * DEX layout (dex-format "annotations_directory_item"): class_def.annotations_off
 * points at { class_annotations_off, fields_size, annotated_methods_size,
 * annotated_parameters_size, field_annotation[], method_annotation[],
 * parameter_annotation[] }. An annotation_set_item lists annotation_item
 * offsets; each item is a visibility byte followed by an encoded_annotation.
 *
 * Only VISIBILITY_RUNTIME annotations are reflected. Annotation instances are
 * java.lang.reflect.Proxy objects implementing the annotation interface whose
 * InvocationHandler (libcore.reflect.AnnotationFactory, as on Android) answers
 * element methods from decoded values, falling back to the annotation type's
 * dalvik.annotation.AnnotationDefault. A missing element with no default
 * raises IncompleteAnnotationException on access (JLS 9.6.2 / Annotation
 * javadoc), and array values are returned as fresh copies. */


#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>





/* Package/DEX services are optional so host tools that link only the JNI and
 * framework models (e.g. the analyzer capability oracle) still link. */
extern const char *ssrnx_dex_java_method_source(void) __attribute__((weak));
extern SsrnxResult ssrnx_dex_initialize_class(const char *source_path, const char *class_name,
                                              void *env_handle,
                                              SsrnxDexExecReport *out_report) __attribute__((weak));
extern SsrnxResult ssrnx_package_acquire_dex_class(const char *source_path, const char *class_name,
                                                   SsrnxDexClassView *out_view,
                                                   bool *out_class_found) __attribute__((weak));
extern void ssrnx_package_release_dex_class(SsrnxDexClassView *view) __attribute__((weak));
extern SsrnxResult ssrnx_package_list_dex_class_members(const char *source_path,
                                                        const char *class_name,
                                                        SsrnxDexMember **out_members,
                                                        size_t *out_count,
                                                        bool *out_class_found) __attribute__((weak));
extern void ssrnx_package_free_dex_members(SsrnxDexMember *members, size_t count) __attribute__((weak));

#define FACTORY_CLASS "libcore/reflect/AnnotationFactory"
#define ANNOTATION_IFACE "java/lang/annotation/Annotation"
#define ANNOTATION_ARRAY "[Ljava/lang/annotation/Annotation;"
#define INCOMPLETE_CLASS "java/lang/annotation/IncompleteAnnotationException"
#define PAYLOAD_MAGIC 0x414E4E4Fu /* ANNO */
#define VIS_RUNTIME 1u
#define VIS_SYSTEM 2u
#define MAX_NESTING 8u
#define ACC_ANNOTATION 0x2000u

enum {
    EV_BYTE = 0x00, EV_SHORT = 0x02, EV_CHAR = 0x03, EV_INT = 0x04, EV_LONG = 0x06,
    EV_FLOAT = 0x10, EV_DOUBLE = 0x11, EV_METHOD_TYPE = 0x15, EV_METHOD_HANDLE = 0x16,
    EV_STRING = 0x17, EV_TYPE = 0x18, EV_FIELD = 0x19, EV_METHOD = 0x1a, EV_ENUM = 0x1b,
    EV_ARRAY = 0x1c, EV_ANNOTATION = 0x1d, EV_NULL = 0x1e, EV_BOOLEAN = 0x1f,
};

typedef struct java_annotations_AnnMember {
    char *name;
    char *type;      /* element return descriptor; NULL when the type is not in DEX */
    void *value;     /* global ref (boxed primitive, String, Class, enum, array, annotation) */
    bool present;
} java_annotations_AnnMember;

typedef struct java_annotations_AnnPayload {
    uint32_t magic;
    char *type_name;
    void *type_class; /* global ref */
    java_annotations_AnnMember *members;
    size_t count;
    struct java_annotations_AnnPayload *next;
} java_annotations_AnnPayload;

typedef struct java_annotations_AnnCacheEntry {
    char *key;
    void *array; /* global ref: declared RUNTIME annotations */
    struct java_annotations_AnnCacheEntry *next;
} java_annotations_AnnCacheEntry;

typedef struct java_annotations_DexView {
    const uint8_t *d;
    size_t size;
} java_annotations_DexView;

static pthread_mutex_t java_annotations_g_ann_lock = PTHREAD_MUTEX_INITIALIZER;
static java_annotations_AnnPayload *java_annotations_g_payloads;
static java_annotations_AnnCacheEntry *java_annotations_g_cache;

/* ---------- raw DEX readers (bounds-checked) ---------- */

static uint32_t java_annotations_rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint16_t java_annotations_rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

static bool java_annotations_in_bounds(const java_annotations_DexView *v, uint64_t off, uint64_t len) {
    return off <= v->size && len <= v->size - off;
}

static bool java_annotations_read_u32(const java_annotations_DexView *v, uint64_t off, uint32_t *out) {
    if (!java_annotations_in_bounds(v, off, 4)) return false;
    *out = java_annotations_rd32(v->d + off);
    return true;
}

static bool java_annotations_read_uleb(const java_annotations_DexView *v, size_t *off, uint32_t *out) {
    uint32_t result = 0;
    for (unsigned i = 0; i < 5u; ++i) {
        if (*off >= v->size) return false;
        const uint8_t b = v->d[(*off)++];
        if (i == 4u && (b & 0xf0u)) return false;
        result |= (uint32_t)(b & 0x7fu) << (i * 7u);
        if (!(b & 0x80u)) { *out = result; return true; }
    }
    return false;
}

static const char *java_annotations_dex_string(const java_annotations_DexView *v, uint32_t idx) {
    uint32_t count = 0, table = 0, data_off = 0;
    if (!java_annotations_read_u32(v, 0x38, &count) || !java_annotations_read_u32(v, 0x3c, &table) || idx >= count ||
        !java_annotations_read_u32(v, (uint64_t)table + (uint64_t)idx * 4u, &data_off)) return NULL;
    size_t off = data_off;
    uint32_t utf16_len = 0;
    if (!java_annotations_read_uleb(v, &off, &utf16_len) || off >= v->size) return NULL;
    const char *s = (const char *)v->d + off;
    return memchr(s, 0, v->size - off) ? s : NULL;
}

static const char *java_annotations_dex_type(const java_annotations_DexView *v, uint32_t idx) {
    uint32_t count = 0, table = 0, str = 0;
    if (!java_annotations_read_u32(v, 0x40, &count) || !java_annotations_read_u32(v, 0x44, &table) || idx >= count ||
        !java_annotations_read_u32(v, (uint64_t)table + (uint64_t)idx * 4u, &str)) return NULL;
    return java_annotations_dex_string(v, str);
}

/* "Lfoo/Bar;" -> "foo/Bar"; array and primitive descriptors are kept. */
static bool java_annotations_desc_to_name(const char *desc, char *out, size_t cap) {
    if (!desc || !out || !cap) return false;
    const size_t n = strlen(desc);
    if (desc[0] == 'L') {
        if (n < 3 || desc[n - 1] != ';' || n - 2 >= cap) return false;
        memcpy(out, desc + 1, n - 2);
        out[n - 2] = '\0';
        return true;
    }
    if (n >= cap) return false;
    memcpy(out, desc, n + 1);
    return true;
}

static bool java_annotations_append_text(char *out, size_t cap, size_t *used, const char *text) {
    const size_t n = strlen(text);
    if (*used + n >= cap) return false;
    memcpy(out + *used, text, n + 1);
    *used += n;
    return true;
}

static bool java_annotations_proto_sig(const java_annotations_DexView *v, uint32_t proto_idx, char *out, size_t cap) {
    uint32_t count = 0, table = 0;
    if (!java_annotations_read_u32(v, 0x48, &count) || !java_annotations_read_u32(v, 0x4c, &table) || proto_idx >= count) return false;
    const uint64_t item = (uint64_t)table + (uint64_t)proto_idx * 12u;
    uint32_t ret_idx = 0, params_off = 0;
    if (!java_annotations_read_u32(v, item + 4u, &ret_idx) || !java_annotations_read_u32(v, item + 8u, &params_off)) return false;
    size_t used = 0;
    out[0] = '\0';
    if (!java_annotations_append_text(out, cap, &used, "(")) return false;
    if (params_off) {
        uint32_t n = 0;
        if (!java_annotations_read_u32(v, params_off, &n) || !java_annotations_in_bounds(v, (uint64_t)params_off + 4u, (uint64_t)n * 2u))
            return false;
        for (uint32_t i = 0; i < n; ++i) {
            const char *t = java_annotations_dex_type(v, java_annotations_rd16(v->d + params_off + 4u + i * 2u));
            if (!t || !java_annotations_append_text(out, cap, &used, t)) return false;
        }
    }
    const char *ret = java_annotations_dex_type(v, ret_idx);
    return ret && java_annotations_append_text(out, cap, &used, ")") && java_annotations_append_text(out, cap, &used, ret);
}

static bool java_annotations_method_identity(const java_annotations_DexView *v, uint32_t idx, const char **out_class,
                            const char **out_name, char *sig, size_t sig_cap) {
    uint32_t count = 0, table = 0, name_idx = 0;
    if (!java_annotations_read_u32(v, 0x58, &count) || !java_annotations_read_u32(v, 0x5c, &table) || idx >= count) return false;
    const uint64_t item = (uint64_t)table + (uint64_t)idx * 8u;
    if (!java_annotations_in_bounds(v, item, 8u)) return false;
    name_idx = java_annotations_rd32(v->d + item + 4u);
    *out_class = java_annotations_dex_type(v, java_annotations_rd16(v->d + item));
    *out_name = java_annotations_dex_string(v, name_idx);
    return *out_class && *out_name && java_annotations_proto_sig(v, java_annotations_rd16(v->d + item + 2u), sig, sig_cap);
}

static bool java_annotations_field_identity(const java_annotations_DexView *v, uint32_t idx, const char **out_class,
                           const char **out_type, const char **out_name) {
    uint32_t count = 0, table = 0;
    if (!java_annotations_read_u32(v, 0x50, &count) || !java_annotations_read_u32(v, 0x54, &table) || idx >= count) return false;
    const uint64_t item = (uint64_t)table + (uint64_t)idx * 8u;
    if (!java_annotations_in_bounds(v, item, 8u)) return false;
    *out_class = java_annotations_dex_type(v, java_annotations_rd16(v->d + item));
    *out_type = java_annotations_dex_type(v, java_annotations_rd16(v->d + item + 2u));
    *out_name = java_annotations_dex_string(v, java_annotations_rd32(v->d + item + 4u));
    return *out_class && *out_type && *out_name;
}

static bool java_annotations_skip_value(const java_annotations_DexView *v, size_t *off, unsigned depth) {
    if (*off >= v->size || depth > 16u) return false;
    const uint8_t head = v->d[(*off)++];
    const uint8_t type = head & 0x1fu, arg = head >> 5;
    switch (type) {
        case EV_NULL: case EV_BOOLEAN: return true;
        case EV_ARRAY: {
            uint32_t n = 0;
            if (!java_annotations_read_uleb(v, off, &n)) return false;
            for (uint32_t i = 0; i < n; ++i) if (!java_annotations_skip_value(v, off, depth + 1u)) return false;
            return true;
        }
        case EV_ANNOTATION: {
            uint32_t type_idx = 0, n = 0;
            if (!java_annotations_read_uleb(v, off, &type_idx) || !java_annotations_read_uleb(v, off, &n)) return false;
            for (uint32_t i = 0; i < n; ++i) {
                uint32_t name = 0;
                if (!java_annotations_read_uleb(v, off, &name) || !java_annotations_skip_value(v, off, depth + 1u)) return false;
            }
            return true;
        }
        default:
            if (!java_annotations_in_bounds(v, *off, (uint64_t)arg + 1u)) return false;
            *off += (size_t)arg + 1u;
            return true;
    }
}

static uint64_t java_annotations_read_compact(const java_annotations_DexView *v, size_t *off, unsigned n, bool *ok) {
    uint64_t value = 0;
    if (!n || n > 8u || !java_annotations_in_bounds(v, *off, n)) { *ok = false; return 0; }
    for (unsigned i = 0; i < n; ++i) value |= (uint64_t)v->d[*off + i] << (i * 8u);
    *off += n;
    return value;
}

static uint64_t java_annotations_sign_extend(uint64_t value, unsigned bytes) {
    if (bytes >= 8u) return value;
    const unsigned bits = bytes * 8u;
    if (value & (UINT64_C(1) << (bits - 1u))) value |= UINT64_MAX << bits;
    return value;
}

/* Primitive encoded value -> (descriptor char, canonical DEX register bits). */
static bool java_annotations_decode_primitive(uint8_t type, uint8_t arg, const java_annotations_DexView *v, size_t *off,
                             char *out_kind, uint64_t *out_bits) {
    bool ok = true;
    const unsigned n = (unsigned)arg + 1u;
    switch (type) {
        case EV_BYTE: *out_kind = 'B'; *out_bits = java_annotations_sign_extend(java_annotations_read_compact(v, off, 1u, &ok), 1u); break;
        case EV_SHORT: *out_kind = 'S'; *out_bits = java_annotations_sign_extend(java_annotations_read_compact(v, off, n, &ok), n); break;
        case EV_CHAR: *out_kind = 'C'; *out_bits = java_annotations_read_compact(v, off, n, &ok); break;
        case EV_INT: *out_kind = 'I'; *out_bits = java_annotations_sign_extend(java_annotations_read_compact(v, off, n, &ok), n); break;
        case EV_LONG: *out_kind = 'J'; *out_bits = java_annotations_sign_extend(java_annotations_read_compact(v, off, n, &ok), n); break;
        case EV_FLOAT: *out_kind = 'F'; *out_bits = java_annotations_read_compact(v, off, n, &ok) << ((4u - n) * 8u); break;
        case EV_DOUBLE: *out_kind = 'D'; *out_bits = java_annotations_read_compact(v, off, n, &ok) << ((8u - n) * 8u); break;
        case EV_BOOLEAN: *out_kind = 'Z'; *out_bits = arg ? 1u : 0u; break;
        default: return false;
    }
    return ok;
}

/* ---------- Java object helpers ---------- */

static const char *source_path(void) {
    return ssrnx_dex_java_method_source ? ssrnx_dex_java_method_source() : NULL;
}

static void java_annotations_clear_exception(void *env) {
    if (ssrnx_jni_exception_pending_host(env)) ssrnx_jni_exception_clear_host(env);
}

static void *java_annotations_box_primitive(void *env, char kind, uint64_t bits) {
    static const struct { char kind; const char *cls, *sig; } k_box[] = {
        {'Z', "java/lang/Boolean", "(Z)Ljava/lang/Boolean;"},
        {'B', "java/lang/Byte", "(B)Ljava/lang/Byte;"},
        {'C', "java/lang/Character", "(C)Ljava/lang/Character;"},
        {'S', "java/lang/Short", "(S)Ljava/lang/Short;"},
        {'I', "java/lang/Integer", "(I)Ljava/lang/Integer;"},
        {'J', "java/lang/Long", "(J)Ljava/lang/Long;"},
        {'F', "java/lang/Float", "(F)Ljava/lang/Float;"},
        {'D', "java/lang/Double", "(D)Ljava/lang/Double;"},
    };
    for (size_t i = 0; i < sizeof(k_box) / sizeof(k_box[0]); ++i) {
        if (k_box[i].kind != kind) continue;
        SsrnxJniValue arg = {0}, out = {0};
        switch (kind) {
            case 'Z': arg.z = bits ? 1u : 0u; break;
            case 'B': arg.b = (int8_t)bits; break;
            case 'C': arg.c = (uint16_t)bits; break;
            case 'S': arg.s = (int16_t)bits; break;
            case 'I': arg.i = (int32_t)bits; break;
            case 'F': { uint32_t b32 = (uint32_t)bits; memcpy(&arg.f, &b32, sizeof(arg.f)); break; }
            case 'D': memcpy(&arg.d, &bits, sizeof(arg.d)); break;
            default: arg.j = (int64_t)bits; break;
        }
        bool found = false;
        if (ssrnx_jni_invoke_method_host(env, NULL, k_box[i].cls, "valueOf", k_box[i].sig, true,
                                         &arg, &out, &found) == SSRNX_OK && found)
            return out.l;
        return NULL;
    }
    return NULL;
}

static void *java_annotations_new_primitive_array(void *env, char kind, int32_t n) {
    switch (kind) {
        case 'Z': return ssrnx_jni_new_local_boolean_array(env, n);
        case 'B': return ssrnx_jni_new_local_byte_array(env, n);
        case 'C': return ssrnx_jni_new_local_char_array(env, n);
        case 'S': return ssrnx_jni_new_local_short_array(env, n);
        case 'I': return ssrnx_jni_new_local_int_array(env, n);
        case 'J': return ssrnx_jni_new_local_long_array(env, n);
        case 'F': return ssrnx_jni_new_local_float_array(env, n);
        case 'D': return ssrnx_jni_new_local_double_array(env, n);
        default: return NULL;
    }
}

static bool java_annotations_is_primitive_desc(const char *t) {
    return t && t[0] && t[1] == '\0' && strchr("ZBCSIJFD", t[0]);
}

static void *java_annotations_copy_array(void *env, void *array) {
    const int32_t n = ssrnx_jni_array_length_host(array);
    if (n < 0) return NULL;
    void *copy = ssrnx_jni_new_local_array_like_host(env, array, n);
    if (copy && n > 0 && ssrnx_jni_arraycopy_host(array, 0, copy, 0, n) != SSRNX_OK) {
        ssrnx_jni_delete_local_ref_host(env, copy);
        return NULL;
    }
    return copy;
}

static void *java_annotations_annotation_array(void *env, void *const *items, size_t n) {
    void *array = ssrnx_jni_new_local_reference_array(env, ANNOTATION_ARRAY, (int32_t)n);
    for (size_t i = 0; array && i < n; ++i)
        (void)ssrnx_jni_set_object_array_element_host(env, array, (int32_t)i, items[i]);
    return array;
}

/* ---------- annotation type metadata ---------- */

typedef struct java_annotations_AnnBuild {
    void *env;
    const char *source;
    unsigned depth;
} java_annotations_AnnBuild;

static bool java_annotations_acquire_view(const char *source, const char *class_name, SsrnxDexClassView *out) {
    bool found = false;
    memset(out, 0, sizeof(*out));
    if (!source || !class_name || !ssrnx_package_acquire_dex_class) return false;
    if (ssrnx_package_acquire_dex_class(source, class_name, out, &found) != SSRNX_OK || !found)
        return false;
    return true;
}

static void java_annotations_release_view(SsrnxDexClassView *view) {
    if (view->dex_data && ssrnx_package_release_dex_class) ssrnx_package_release_dex_class(view);
}

/* annotation_set_item offset for the class itself (0 when none). */
static uint32_t java_annotations_class_annotation_set(const java_annotations_DexView *v, uint32_t class_def) {
    uint32_t dir = 0, set = 0;
    if (!java_annotations_read_u32(v, (uint64_t)class_def + 20u, &dir) || !dir || !java_annotations_read_u32(v, dir, &set)) return 0;
    return set;
}

/* Finds the annotation_item in a set whose type and visibility match; returns
 * the encoded_annotation element list offset (after type_idx and size). */
static bool java_annotations_find_in_set(const java_annotations_DexView *v, uint32_t set_off, const char *type_desc,
                        uint8_t visibility, size_t *out_elements, uint32_t *out_count) {
    uint32_t n = 0;
    if (!set_off || !java_annotations_read_u32(v, set_off, &n) || !java_annotations_in_bounds(v, (uint64_t)set_off + 4u, (uint64_t)n * 4u))
        return false;
    for (uint32_t i = 0; i < n; ++i) {
        const uint32_t item = java_annotations_rd32(v->d + set_off + 4u + i * 4u);
        if (!java_annotations_in_bounds(v, item, 1u) || v->d[item] != visibility) continue;
        size_t off = (size_t)item + 1u;
        uint32_t type_idx = 0, count = 0;
        if (!java_annotations_read_uleb(v, &off, &type_idx) || !java_annotations_read_uleb(v, &off, &count)) continue;
        const char *desc = java_annotations_dex_type(v, type_idx);
        if (!desc || strcmp(desc, type_desc) != 0) continue;
        *out_elements = off;
        *out_count = count;
        return true;
    }
    return false;
}

static bool java_annotations_annotation_type_has(const char *source, const char *annotation_class,
                                const char *meta_desc) {
    SsrnxDexClassView view;
    if (!java_annotations_acquire_view(source, annotation_class, &view)) return false;
    const java_annotations_DexView v = {view.dex_data, view.dex_size};
    size_t elements = 0;
    uint32_t count = 0;
    const bool has = java_annotations_find_in_set(&v, java_annotations_class_annotation_set(&v, view.class_def_offset), meta_desc,
                                 VIS_RUNTIME, &elements, &count);
    java_annotations_release_view(&view);
    return has;
}

static void java_annotations_free_members(void *env, java_annotations_AnnMember *m, size_t n) {
    for (size_t i = 0; m && i < n; ++i) {
        if (m[i].value) ssrnx_jni_delete_global_ref_host(env, m[i].value);
        free(m[i].name);
        free(m[i].type);
    }
    free(m);
}

/* Element methods declared by the annotation interface (name + return type). */
static java_annotations_AnnMember *java_annotations_annotation_members(const char *source, const char *annotation_class,
                                     size_t *out_count) {
    *out_count = 0;
    if (!source || !ssrnx_package_list_dex_class_members) return NULL;
    SsrnxDexMember *members = NULL;
    size_t count = 0;
    bool found = false;
    if (ssrnx_package_list_dex_class_members(source, annotation_class, &members, &count,
                                             &found) != SSRNX_OK || !found) return NULL;
    java_annotations_AnnMember *out = calloc(count ? count : 1u, sizeof(*out));
    size_t used = 0;
    for (size_t i = 0; out && i < count; ++i) {
        const SsrnxDexMember *m = &members[i];
        if (!m->is_method || m->is_static || m->name[0] == '<' ||
            strncmp(m->signature, "()", 2) != 0) continue;
        out[used].name = strdup(m->name);
        out[used].type = strdup(m->signature + 2);
        if (!out[used].name || !out[used].type) {
            free(out[used].name); free(out[used].type);
            continue;
        }
        used++;
    }
    if (ssrnx_package_free_dex_members) ssrnx_package_free_dex_members(members, count);
    *out_count = used;
    return out;
}

static java_annotations_AnnMember *java_annotations_member_named(java_annotations_AnnMember **members, size_t *count, const char *name) {
    for (size_t i = 0; i < *count; ++i)
        if (strcmp((*members)[i].name, name) == 0) return &(*members)[i];
    java_annotations_AnnMember *grown = realloc(*members, (*count + 1u) * sizeof(**members));
    if (!grown) return NULL;
    *members = grown;
    java_annotations_AnnMember *m = &grown[(*count)++];
    memset(m, 0, sizeof(*m));
    m->name = strdup(name);
    return m->name ? m : NULL;
}

static void *java_annotations_build_annotation(java_annotations_AnnBuild *b, const java_annotations_DexView *v, size_t *off);

static char java_annotations_inferred_kind(uint8_t type) {
    switch (type) {
        case EV_BYTE: return 'B';
        case EV_SHORT: return 'S';
        case EV_CHAR: return 'C';
        case EV_INT: return 'I';
        case EV_LONG: return 'J';
        case EV_FLOAT: return 'F';
        case EV_DOUBLE: return 'D';
        case EV_BOOLEAN: return 'Z';
        default: return 0;
    }
}

/* Decodes one encoded_value as the Java object an element method returns.
 * `expected` is the element's declared return descriptor ("" if unknown). */
static void *java_annotations_decode_value(java_annotations_AnnBuild *b, const java_annotations_DexView *v, size_t *off, const char *expected,
                          bool *ok) {
    if (*off >= v->size || b->depth > MAX_NESTING) { *ok = false; return NULL; }
    const size_t start = *off;
    const uint8_t head = v->d[(*off)++];
    const uint8_t type = head & 0x1fu, arg = head >> 5;
    void *env = b->env;
    char kind = 0;
    uint64_t bits = 0;
    if (java_annotations_decode_primitive(type, arg, v, off, &kind, &bits)) {
        if (java_annotations_is_primitive_desc(expected)) kind = expected[0];
        return java_annotations_box_primitive(env, kind, bits);
    }
    bool compact_ok = true;
    switch (type) {
        case EV_STRING: {
            const uint64_t idx = java_annotations_read_compact(v, off, (unsigned)arg + 1u, &compact_ok);
            const char *s = compact_ok ? java_annotations_dex_string(v, (uint32_t)idx) : NULL;
            if (!s) { *ok = false; return NULL; }
            return ssrnx_jni_new_local_string_utf(env, s);
        }
        case EV_TYPE: {
            const uint64_t idx = java_annotations_read_compact(v, off, (unsigned)arg + 1u, &compact_ok);
            const char *desc = compact_ok ? java_annotations_dex_type(v, (uint32_t)idx) : NULL;
            if (!desc) { *ok = false; return NULL; }
            void *cls = ssrnx_jni_class_ref_for_descriptor_host(env, desc);
            if (!cls) {
                java_annotations_clear_exception(env);
                SSRNX_LOGCW("JAVA", "annotation Class element unresolved type=%s", desc);
            }
            return cls;
        }
        case EV_ENUM: {
            const uint64_t idx = java_annotations_read_compact(v, off, (unsigned)arg + 1u, &compact_ok);
            const char *cls_desc = NULL, *type_desc = NULL, *name = NULL;
            char cls[512];
            if (!compact_ok || !java_annotations_field_identity(v, (uint32_t)idx, &cls_desc, &type_desc, &name) ||
                !java_annotations_desc_to_name(cls_desc, cls, sizeof(cls))) { *ok = false; return NULL; }
            /* Package static fields exist in the JNI model only once declared
             * members are materialized; <clinit> then assigns the constants. */
            (void)ssrnx_jni_materialize_class_members_host(env, cls);
            if (ssrnx_dex_initialize_class && b->source) {
                SsrnxDexExecReport report = {0};
                const SsrnxResult init_rc = ssrnx_dex_initialize_class(b->source, cls, env, &report);
                if (init_rc != SSRNX_OK)
                    SSRNX_LOGCW("JAVA", "annotation enum class init failed class=%s rc=%d detail=%s",
                                cls, (int)init_rc, report.detail);
            }
            java_annotations_clear_exception(env);
            SsrnxJniValue value = {0};
            bool found = false;
            if (ssrnx_jni_get_static_field_value(env, cls, name, type_desc, &value, &found) != SSRNX_OK ||
                !found || !value.l) {
                SSRNX_LOGCW("JAVA", "annotation enum constant unresolved %s.%s", cls, name);
                return NULL;
            }
            return value.l;
        }
        case EV_ARRAY: {
            uint32_t n = 0;
            if (!java_annotations_read_uleb(v, off, &n) || n > 0x7fffffffu) { *ok = false; return NULL; }
            char component[512] = "";
            if (expected && expected[0] == '[') {
                snprintf(component, sizeof(component), "%s", expected + 1);
            } else if (n > 0 && *off < v->size) {
                const uint8_t first = v->d[*off] & 0x1fu;
                const char k = java_annotations_inferred_kind(first);
                if (k) { component[0] = k; component[1] = '\0'; }
                else if (first == EV_STRING) snprintf(component, sizeof(component), "Ljava/lang/String;");
                else if (first == EV_TYPE) snprintf(component, sizeof(component), "Ljava/lang/Class;");
                else snprintf(component, sizeof(component), "Ljava/lang/Object;");
            } else {
                snprintf(component, sizeof(component), "Ljava/lang/Object;");
            }
            if (java_annotations_is_primitive_desc(component)) {
                void *array = java_annotations_new_primitive_array(env, component[0], (int32_t)n);
                for (uint32_t i = 0; i < n && *ok; ++i) {
                    if (*off >= v->size) { *ok = false; break; }
                    const uint8_t eh = v->d[(*off)++];
                    char ek = 0;
                    uint64_t eb = 0;
                    if (!java_annotations_decode_primitive(eh & 0x1fu, eh >> 5, v, off, &ek, &eb)) { *ok = false; break; }
                    if (array) (void)ssrnx_jni_primitive_array_set_host(array, component[0], (int32_t)i, eb);
                }
                return array;
            }
            char array_desc[520];
            snprintf(array_desc, sizeof(array_desc), "[%s", component);
            void *array = ssrnx_jni_new_local_reference_array(env, array_desc, (int32_t)n);
            if (!array) java_annotations_clear_exception(env);
            for (uint32_t i = 0; i < n && *ok; ++i) {
                void *element = java_annotations_decode_value(b, v, off, component, ok);
                if (array && element)
                    (void)ssrnx_jni_set_object_array_element_host(env, array, (int32_t)i, element);
                if (element) ssrnx_jni_delete_local_ref_host(env, element);
            }
            return array;
        }
        case EV_ANNOTATION: {
            b->depth++;
            void *nested = java_annotations_build_annotation(b, v, off);
            b->depth--;
            return nested;
        }
        case EV_NULL:
            return NULL;
        default:
            /* Method/field/method-type/handle constants are not legal element
             * values (JLS 9.6.1); skip them without materializing. */
            *off = start;
            if (!java_annotations_skip_value(v, off, 0)) *ok = false;
            return NULL;
    }
}

/* Decodes `count` annotation_elements at *off into members. */
static bool java_annotations_decode_elements(java_annotations_AnnBuild *b, const java_annotations_DexView *v, size_t *off, uint32_t count,
                            java_annotations_AnnMember **members, size_t *member_count, bool only_absent) {
    bool ok = true;
    for (uint32_t i = 0; i < count && ok; ++i) {
        uint32_t name_idx = 0;
        if (!java_annotations_read_uleb(v, off, &name_idx)) return false;
        const char *name = java_annotations_dex_string(v, name_idx);
        if (!name) return false;
        java_annotations_AnnMember *m = java_annotations_member_named(members, member_count, name);
        if (!m) return false;
        if (only_absent && m->present) {
            if (!java_annotations_skip_value(v, off, 0)) return false;
            continue;
        }
        void *value = java_annotations_decode_value(b, v, off, m->type ? m->type : "", &ok);
        if (m->value) ssrnx_jni_delete_global_ref_host(b->env, m->value);
        m->value = value ? ssrnx_jni_new_global_ref_host(b->env, value) : NULL;
        if (value) ssrnx_jni_delete_local_ref_host(b->env, value);
        m->present = true;
    }
    return ok;
}

/* dalvik.annotation.AnnotationDefault on the annotation type holds a nested
 * annotation of that type whose elements are the declared defaults. */
static void java_annotations_apply_defaults(java_annotations_AnnBuild *b, const char *annotation_class, java_annotations_AnnMember **members,
                           size_t *count) {
    SsrnxDexClassView view;
    if (!java_annotations_acquire_view(b->source, annotation_class, &view)) return;
    const java_annotations_DexView v = {view.dex_data, view.dex_size};
    size_t off = 0;
    uint32_t n = 0;
    if (java_annotations_find_in_set(&v, java_annotations_class_annotation_set(&v, view.class_def_offset),
                    "Ldalvik/annotation/AnnotationDefault;", VIS_SYSTEM, &off, &n)) {
        for (uint32_t i = 0; i < n; ++i) {
            uint32_t name_idx = 0;
            if (!java_annotations_read_uleb(&v, &off, &name_idx)) break;
            const char *name = java_annotations_dex_string(&v, name_idx);
            if (!name || strcmp(name, "value") != 0 || off >= v.size ||
                (v.d[off] & 0x1fu) != EV_ANNOTATION) {
                if (!java_annotations_skip_value(&v, &off, 0)) break;
                continue;
            }
            off++;
            uint32_t type_idx = 0, defaults = 0;
            if (!java_annotations_read_uleb(&v, &off, &type_idx) || !java_annotations_read_uleb(&v, &off, &defaults)) break;
            (void)java_annotations_decode_elements(b, &v, &off, defaults, members, count, true);
            break;
        }
    }
    java_annotations_release_view(&view);
}

static SsrnxJniValue java_annotations_factory_invoke(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u);

/* Builds an annotation from an encoded_annotation at *off (type_idx first).
 * Returns NULL (after consuming the bytes) when the type cannot be loaded:
 * Android omits annotations whose type is not present. */
static void *java_annotations_build_annotation(java_annotations_AnnBuild *b, const java_annotations_DexView *v, size_t *off) {
    void *env = b->env;
    uint32_t type_idx = 0, count = 0;
    if (!java_annotations_read_uleb(v, off, &type_idx) || !java_annotations_read_uleb(v, off, &count)) return NULL;
    const char *desc = java_annotations_dex_type(v, type_idx);
    char type_name[512];
    if (!desc || !java_annotations_desc_to_name(desc, type_name, sizeof(type_name))) return NULL;
    void *type_class = ssrnx_jni_find_class_ref(env, type_name);
    if (!type_class) {
        java_annotations_clear_exception(env);
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t name = 0;
            if (!java_annotations_read_uleb(v, off, &name) || !java_annotations_skip_value(v, off, 0)) break;
        }
        SSRNX_LOGCW("JAVA", "annotation type unavailable type=%s (annotation omitted)", type_name);
        return NULL;
    }
    size_t member_count = 0;
    java_annotations_AnnMember *members = java_annotations_annotation_members(b->source, type_name, &member_count);
    const bool elements_ok = java_annotations_decode_elements(b, v, off, count, &members, &member_count, false);
    java_annotations_apply_defaults(b, type_name, &members, &member_count);
    if (!elements_ok) {
        java_annotations_free_members(env, members, member_count);
        ssrnx_jni_delete_local_ref_host(env, type_class);
        return NULL;
    }

    /* Annotation element methods are the interface's abstract methods; this
     * also marks the annotation type as an interface for proxy dispatch. */
    (void)ssrnx_jni_register_interface(type_name, ANNOTATION_IFACE);
    for (size_t i = 0; i < member_count; ++i) {
        if (!members[i].type) continue;
        char sig[600];
        snprintf(sig, sizeof(sig), "()%s", members[i].type);
        (void)ssrnx_jni_register_interface_method(type_name, members[i].name, sig);
    }

    java_annotations_AnnPayload *payload = calloc(1, sizeof(*payload));
    void *handler = payload ? ssrnx_jni_new_local_object(env, FACTORY_CLASS) : NULL;
    void *ifaces = handler ? ssrnx_jni_new_local_reference_array(env, "[Ljava/lang/Class;", 1) : NULL;
    void *proxy = NULL;
    if (ifaces && ssrnx_jni_set_object_array_element_host(env, ifaces, 0, type_class) == SSRNX_OK) {
        payload->magic = PAYLOAD_MAGIC;
        payload->type_name = strdup(type_name);
        payload->type_class = ssrnx_jni_new_global_ref_host(env, type_class);
        payload->members = members;
        payload->count = member_count;
        members = NULL;
        if (payload->type_name && ssrnx_jni_set_object_host_data(handler, payload) == SSRNX_OK &&
            ssrnx_jni_proxy_new_instance_host(env, ifaces, handler, &proxy) == SSRNX_OK && proxy) {
            pthread_mutex_lock(&java_annotations_g_ann_lock);
            payload->next = java_annotations_g_payloads;
            java_annotations_g_payloads = payload;
            pthread_mutex_unlock(&java_annotations_g_ann_lock);
            payload = NULL;
        }
        java_annotations_clear_exception(env);
    }
    if (payload) {
        java_annotations_free_members(env, payload->members, payload->count);
        if (payload->type_class) ssrnx_jni_delete_global_ref_host(env, payload->type_class);
        free(payload->type_name);
        free(payload);
        if (handler) (void)ssrnx_jni_set_object_host_data(handler, NULL);
    }
    java_annotations_free_members(env, members, member_count);
    if (ifaces) ssrnx_jni_delete_local_ref_host(env, ifaces);
    if (handler) ssrnx_jni_delete_local_ref_host(env, handler);
    ssrnx_jni_delete_local_ref_host(env, type_class);
    return proxy;
}

/* All RUNTIME annotations of an annotation_set_item as an Annotation[]. */
static void *java_annotations_annotations_of_set(java_annotations_AnnBuild *b, const java_annotations_DexView *v, uint32_t set_off) {
    void *items[64] = {0};
    size_t used = 0;
    uint32_t n = 0;
    if (set_off && java_annotations_read_u32(v, set_off, &n) && java_annotations_in_bounds(v, (uint64_t)set_off + 4u, (uint64_t)n * 4u)) {
        for (uint32_t i = 0; i < n && used < 64u; ++i) {
            const uint32_t item = java_annotations_rd32(v->d + set_off + 4u + i * 4u);
            if (!java_annotations_in_bounds(v, item, 1u) || v->d[item] != VIS_RUNTIME) continue;
            size_t off = (size_t)item + 1u;
            void *a = java_annotations_build_annotation(b, v, &off);
            if (a) items[used++] = a;
        }
    }
    void *array = java_annotations_annotation_array(b->env, items, used);
    for (size_t i = 0; i < used; ++i) ssrnx_jni_delete_local_ref_host(b->env, items[i]);
    return array;
}

typedef enum java_annotations_AnnTarget { TARGET_CLASS, TARGET_FIELD, TARGET_METHOD, TARGET_PARAMETERS } java_annotations_AnnTarget;

/* annotation_set_item (or, for parameters, annotation_set_ref_list) offset of
 * one member of the class in `view`. */
static uint32_t java_annotations_member_annotation_offset(const java_annotations_DexView *v, uint32_t class_def, java_annotations_AnnTarget target,
                                         const char *owner_desc, const char *name,
                                         const char *sig) {
    uint32_t dir = 0, class_set = 0, fields = 0, methods = 0, params = 0;
    if (!java_annotations_read_u32(v, (uint64_t)class_def + 20u, &dir) || !dir || !java_annotations_read_u32(v, dir, &class_set) ||
        !java_annotations_read_u32(v, (uint64_t)dir + 4u, &fields) || !java_annotations_read_u32(v, (uint64_t)dir + 8u, &methods) ||
        !java_annotations_read_u32(v, (uint64_t)dir + 12u, &params)) return 0;
    if (target == TARGET_CLASS) return class_set;
    uint64_t table = (uint64_t)dir + 16u;
    uint32_t entries = fields;
    if (target != TARGET_FIELD) table += (uint64_t)fields * 8u;
    if (target == TARGET_METHOD) entries = methods;
    if (target == TARGET_PARAMETERS) { table += (uint64_t)methods * 8u; entries = params; }
    if (!java_annotations_in_bounds(v, table, (uint64_t)entries * 8u)) return 0;
    for (uint32_t i = 0; i < entries; ++i) {
        const uint32_t idx = java_annotations_rd32(v->d + table + i * 8u);
        const uint32_t set = java_annotations_rd32(v->d + table + i * 8u + 4u);
        const char *cls = NULL, *member = NULL;
        if (target == TARGET_FIELD) {
            const char *type = NULL;
            if (!java_annotations_field_identity(v, idx, &cls, &type, &member)) continue;
            if (strcmp(cls, owner_desc) == 0 && strcmp(member, name) == 0 && strcmp(type, sig) == 0)
                return set;
        } else {
            char msig[1024];
            if (!java_annotations_method_identity(v, idx, &cls, &member, msig, sizeof(msig))) continue;
            if (strcmp(cls, owner_desc) == 0 && strcmp(member, name) == 0 && strcmp(msig, sig) == 0)
                return set;
        }
    }
    return 0;
}

static void *java_annotations_cache_lookup(void *env, const char *key) {
    void *copy = NULL;
    pthread_mutex_lock(&java_annotations_g_ann_lock);
    for (java_annotations_AnnCacheEntry *e = java_annotations_g_cache; e; e = e->next) {
        if (strcmp(e->key, key) == 0) { copy = ssrnx_jni_new_local_ref_host(env, e->array); break; }
    }
    pthread_mutex_unlock(&java_annotations_g_ann_lock);
    return copy;
}

static void java_annotations_cache_store(void *env, const char *key, void *array) {
    java_annotations_AnnCacheEntry *entry = calloc(1, sizeof(*entry));
    if (!entry) return;
    entry->key = strdup(key);
    entry->array = ssrnx_jni_new_global_ref_host(env, array);
    if (!entry->key || !entry->array) {
        free(entry->key);
        if (entry->array) ssrnx_jni_delete_global_ref_host(env, entry->array);
        free(entry);
        return;
    }
    pthread_mutex_lock(&java_annotations_g_ann_lock);
    entry->next = java_annotations_g_cache;
    java_annotations_g_cache = entry;
    pthread_mutex_unlock(&java_annotations_g_ann_lock);
}

/* Declared RUNTIME annotations of a class/field/method as a fresh local
 * Annotation[] (never NULL except on allocation failure). Decoding happens
 * outside the cache lock: enum constants may run <clinit>. */
static void *java_annotations_declared_annotations(void *env, const char *owner, java_annotations_AnnTarget target,
                                  const char *name, const char *sig) {
    char key[1400];
    snprintf(key, sizeof(key), "%d|%s|%s|%s", (int)target, owner, name ? name : "",
             sig ? sig : "");
    void *cached = java_annotations_cache_lookup(env, key);
    if (!cached) {
        java_annotations_AnnBuild b = {env, source_path(), 0};
        SsrnxDexClassView view;
        void *array = NULL;
        if (java_annotations_acquire_view(b.source, owner, &view)) {
            const java_annotations_DexView v = {view.dex_data, view.dex_size};
            char owner_desc[520];
            snprintf(owner_desc, sizeof(owner_desc), "L%s;", owner);
            const uint32_t set = java_annotations_member_annotation_offset(&v, view.class_def_offset, target,
                                                          owner_desc, name, sig);
            array = java_annotations_annotations_of_set(&b, &v, set);
            java_annotations_release_view(&view);
        } else {
            array = java_annotations_annotation_array(env, NULL, 0);
        }
        if (!array) return NULL;
        java_annotations_cache_store(env, key, array);
        cached = array;
    }
    void *copy = java_annotations_copy_array(env, cached);
    ssrnx_jni_delete_local_ref_host(env, cached);
    return copy;
}

static java_annotations_AnnPayload *java_annotations_payload_of(void *env, void *annotation_ref) {
    if (!annotation_ref) return NULL;
    void *handler = ssrnx_jni_proxy_get_handler_host(env, annotation_ref);
    if (!handler) return NULL;
    java_annotations_AnnPayload *p = (java_annotations_AnnPayload *)ssrnx_jni_get_object_host_data(handler);
    ssrnx_jni_delete_local_ref_host(env, handler);
    return p && p->magic == PAYLOAD_MAGIC ? p : NULL;
}

static bool java_annotations_annotation_type_name(void *env, void *annotation, char *out, size_t cap) {
    java_annotations_AnnPayload *p = java_annotations_payload_of(env, annotation);
    if (p) return snprintf(out, cap, "%s", p->type_name) < (int)cap;
    return ssrnx_jni_object_class_name_host(annotation, out, cap);
}

/* Class.getAnnotations(): declared annotations plus @Inherited annotations of
 * superclasses whose type is not already present (JLS 9.6.4.3). */
static void *java_annotations_class_annotations(void *env, const char *class_name, bool declared_only) {
    void *declared = java_annotations_declared_annotations(env, class_name, TARGET_CLASS, NULL, NULL);
    if (declared_only || !declared) return declared;
    const char *source = source_path();
    void *items[64] = {0};
    char types[64][256];
    size_t used = 0;
    const int32_t own = ssrnx_jni_array_length_host(declared);
    for (int32_t i = 0; i < own && used < 64u; ++i) {
        void *a = NULL;
        if (ssrnx_jni_get_object_array_element_host(env, declared, i, &a) != SSRNX_OK || !a) continue;
        (void)java_annotations_annotation_type_name(env, a, types[used], sizeof(types[used]));
        items[used++] = a;
    }
    char current[512];
    snprintf(current, sizeof(current), "%s", class_name);
    for (unsigned walk = 0; walk < 32u; ++walk) {
        SsrnxDexClassView view;
        if (!java_annotations_acquire_view(source, current, &view)) break;
        const java_annotations_DexView v = {view.dex_data, view.dex_size};
        uint32_t super_idx = 0;
        const char *super_desc = NULL;
        if (java_annotations_read_u32(&v, (uint64_t)view.class_def_offset + 8u, &super_idx) && super_idx != UINT32_MAX)
            super_desc = java_annotations_dex_type(&v, super_idx);
        char next[512] = "";
        const bool more = super_desc && java_annotations_desc_to_name(super_desc, next, sizeof(next));
        java_annotations_release_view(&view);
        if (!more) break;
        snprintf(current, sizeof(current), "%s", next);
        void *inherited = java_annotations_declared_annotations(env, current, TARGET_CLASS, NULL, NULL);
        const int32_t count = inherited ? ssrnx_jni_array_length_host(inherited) : 0;
        for (int32_t i = 0; i < count && used < 64u; ++i) {
            void *a = NULL;
            char type[256] = "";
            if (ssrnx_jni_get_object_array_element_host(env, inherited, i, &a) != SSRNX_OK || !a) continue;
            bool keep = java_annotations_annotation_type_name(env, a, type, sizeof(type)) &&
                        java_annotations_annotation_type_has(source, type, "Ljava/lang/annotation/Inherited;");
            for (size_t k = 0; keep && k < used; ++k) keep = strcmp(types[k], type) != 0;
            if (!keep) { ssrnx_jni_delete_local_ref_host(env, a); continue; }
            snprintf(types[used], sizeof(types[used]), "%s", type);
            items[used++] = a;
        }
        if (inherited) ssrnx_jni_delete_local_ref_host(env, inherited);
    }
    void *result = java_annotations_annotation_array(env, items, used);
    for (size_t i = 0; i < used; ++i) ssrnx_jni_delete_local_ref_host(env, items[i]);
    ssrnx_jni_delete_local_ref_host(env, declared);
    return result;
}

/* ---------- reflective receivers ---------- */

typedef struct java_annotations_AnnTargetRef {
    java_annotations_AnnTarget target;
    char owner[512];
    char name[256];
    char sig[1024];
    bool valid;
} java_annotations_AnnTargetRef;

static java_annotations_AnnTargetRef java_annotations_target_of(void *env, void *token, java_annotations_AnnTarget kind) {
    java_annotations_AnnTargetRef t;
    memset(&t, 0, sizeof(t));
    t.target = kind;
    void *self = ssrnx_jni_new_local_from_token(env, token);
    if (!self) return t;
    if (kind == TARGET_CLASS) {
        t.valid = ssrnx_jni_class_ref_name_host(self, t.owner, sizeof(t.owner));
    } else if (kind == TARGET_FIELD) {
        bool is_static = false;
        t.valid = ssrnx_jni_reflected_field_info_host(self, t.owner, sizeof(t.owner), t.name,
                                                      sizeof(t.name), t.sig, sizeof(t.sig), &is_static);
    } else {
        bool is_static = false, iface = false;
        t.valid = ssrnx_jni_reflected_method_info_host(self, t.owner, sizeof(t.owner), t.name,
                                                       sizeof(t.name), t.sig, sizeof(t.sig),
                                                       &is_static, &iface);
    }
    ssrnx_jni_delete_local_ref_host(env, self);
    return t;
}

static void *java_annotations_annotations_for(void *env, const java_annotations_AnnTargetRef *t, bool declared_only) {
    if (!t->valid) return java_annotations_annotation_array(env, NULL, 0);
    if (t->target == TARGET_CLASS) return java_annotations_class_annotations(env, t->owner, declared_only);
    return java_annotations_declared_annotations(env, t->owner, t->target, t->name, t->sig);
}

static void *java_annotations_find_annotation(void *env, void *array, const char *type) {
    const int32_t n = array ? ssrnx_jni_array_length_host(array) : 0;
    for (int32_t i = 0; i < n; ++i) {
        void *a = NULL;
        char name[512] = "";
        if (ssrnx_jni_get_object_array_element_host(env, array, i, &a) != SSRNX_OK || !a) continue;
        if (java_annotations_annotation_type_name(env, a, name, sizeof(name)) && strcmp(name, type) == 0) return a;
        ssrnx_jni_delete_local_ref_host(env, a);
    }
    return NULL;
}

static bool java_annotations_requested_type(void *env, const SsrnxJniValue *a, size_t n, char *out, size_t cap) {
    if (n != 1 || !a[0].l) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/NullPointerException", "annotationClass");
        return false;
    }
    return ssrnx_jni_class_ref_name_host(a[0].l, out, cap);
}

static SsrnxJniValue java_annotations_v_obj(void *ref) { SsrnxJniValue v = {0}; v.l = ref; return v; }

static SsrnxJniValue java_annotations_get_annotation_common(void *env, void *token, const SsrnxJniValue *a,
                                           size_t n, java_annotations_AnnTarget kind, bool declared_only) {
    char type[512];
    if (!java_annotations_requested_type(env, a, n, type, sizeof(type))) return java_annotations_v_obj(NULL);
    const java_annotations_AnnTargetRef t = java_annotations_target_of(env, token, kind);
    void *all = java_annotations_annotations_for(env, &t, declared_only);
    void *found = java_annotations_find_annotation(env, all, type);
    if (all) ssrnx_jni_delete_local_ref_host(env, all);
    return java_annotations_v_obj(found);
}

/* getAnnotationsByType: directly present annotations of the type, else the
 * contents of a present container annotation (JLS 9.6.3: an annotation whose
 * value() element returns T[]). */
static SsrnxJniValue java_annotations_by_type_common(void *env, void *token, const SsrnxJniValue *a, size_t n,
                                    java_annotations_AnnTarget kind, bool declared_only) {
    char type[512];
    if (!java_annotations_requested_type(env, a, n, type, sizeof(type))) return java_annotations_v_obj(NULL);
    const java_annotations_AnnTargetRef t = java_annotations_target_of(env, token, kind);
    void *all = java_annotations_annotations_for(env, &t, declared_only);
    char array_desc[520];
    snprintf(array_desc, sizeof(array_desc), "[L%s;", type);
    void *direct = java_annotations_find_annotation(env, all, type);
    void *result = NULL;
    if (direct) {
        result = ssrnx_jni_new_local_reference_array(env, array_desc, 1);
        if (result) (void)ssrnx_jni_set_object_array_element_host(env, result, 0, direct);
        ssrnx_jni_delete_local_ref_host(env, direct);
    } else {
        const int32_t count = all ? ssrnx_jni_array_length_host(all) : 0;
        for (int32_t i = 0; i < count && !result; ++i) {
            void *container = NULL;
            if (ssrnx_jni_get_object_array_element_host(env, all, i, &container) != SSRNX_OK || !container)
                continue;
            java_annotations_AnnPayload *p = java_annotations_payload_of(env, container);
            for (size_t k = 0; p && k < p->count && !result; ++k) {
                const java_annotations_AnnMember *m = &p->members[k];
                if (strcmp(m->name, "value") == 0 && m->present && m->value && m->type &&
                    strcmp(m->type, array_desc) == 0)
                    result = java_annotations_copy_array(env, m->value);
            }
            ssrnx_jni_delete_local_ref_host(env, container);
        }
        if (!result) result = ssrnx_jni_new_local_reference_array(env, array_desc, 0);
    }
    if (all) ssrnx_jni_delete_local_ref_host(env, all);
    return java_annotations_v_obj(result);
}

#define TARGET_ENTRY(suffix, kind)                                                                 \
    static SsrnxJniValue get_annotation_##suffix(void *e, void *t, const SsrnxJniValue *a, size_t n, \
                                                 void *u) {                                          \
        (void)u; return java_annotations_get_annotation_common(e, t, a, n, kind, false);                             \
    }                                                                                                \
    static SsrnxJniValue get_declared_annotation_##suffix(void *e, void *t, const SsrnxJniValue *a,  \
                                                          size_t n, void *u) {                       \
        (void)u; return java_annotations_get_annotation_common(e, t, a, n, kind, true);                              \
    }                                                                                                \
    static SsrnxJniValue is_present_##suffix(void *e, void *t, const SsrnxJniValue *a, size_t n,     \
                                             void *u) {                                              \
        (void)u;                                                                                     \
        SsrnxJniValue r = java_annotations_get_annotation_common(e, t, a, n, kind, false), out = {0};                \
        out.z = r.l != NULL;                                                                         \
        if (r.l) ssrnx_jni_delete_local_ref_host(e, r.l);                                            \
        return out;                                                                                  \
    }                                                                                                \
    static SsrnxJniValue get_annotations_##suffix(void *e, void *t, const SsrnxJniValue *a,          \
                                                  size_t n, void *u) {                               \
        (void)a; (void)u;                                                                            \
        if (n != 0) return java_annotations_v_obj(NULL);                                                              \
        const java_annotations_AnnTargetRef ref = java_annotations_target_of(e, t, kind);                                             \
        return java_annotations_v_obj(java_annotations_annotations_for(e, &ref, false));                                              \
    }                                                                                                \
    static SsrnxJniValue get_declared_annotations_##suffix(void *e, void *t, const SsrnxJniValue *a, \
                                                           size_t n, void *u) {                      \
        (void)a; (void)u;                                                                            \
        if (n != 0) return java_annotations_v_obj(NULL);                                                              \
        const java_annotations_AnnTargetRef ref = java_annotations_target_of(e, t, kind);                                             \
        return java_annotations_v_obj(java_annotations_annotations_for(e, &ref, true));                                               \
    }                                                                                                \
    static SsrnxJniValue by_type_##suffix(void *e, void *t, const SsrnxJniValue *a, size_t n,        \
                                          void *u) {                                                 \
        (void)u; return java_annotations_by_type_common(e, t, a, n, kind, false);                                    \
    }                                                                                                \
    static SsrnxJniValue declared_by_type_##suffix(void *e, void *t, const SsrnxJniValue *a,         \
                                                   size_t n, void *u) {                              \
        (void)u; return java_annotations_by_type_common(e, t, a, n, kind, true);                                     \
    }

TARGET_ENTRY(class, TARGET_CLASS)
TARGET_ENTRY(field, TARGET_FIELD)
TARGET_ENTRY(method, TARGET_METHOD)

static int32_t java_annotations_descriptor_param_count(const char *sig) {
    if (!sig || sig[0] != '(') return -1;
    int32_t count = 0;
    for (const char *p = sig + 1; *p && *p != ')'; ++count) {
        while (*p == '[') ++p;
        if (*p == 'L') {
            p = strchr(p, ';');
            if (!p) return -1;
        }
        ++p;
    }
    return count;
}

/* Method/Constructor.getParameterAnnotations(): one Annotation[] per formal
 * parameter. The DEX list may be shorter than the parameter count (for
 * synthetic leading parameters javac omits); missing entries are empty. */
static SsrnxJniValue java_annotations_parameter_annotations(void *env, void *token, const SsrnxJniValue *a,
                                           size_t n, void *u) {
    (void)a; (void)u;
    if (n != 0) return java_annotations_v_obj(NULL);
    const java_annotations_AnnTargetRef t = java_annotations_target_of(env, token, TARGET_METHOD);
    const int32_t params = t.valid ? java_annotations_descriptor_param_count(t.sig) : 0;
    void *outer = ssrnx_jni_new_local_reference_array(env, "[[Ljava/lang/annotation/Annotation;",
                                                      params > 0 ? params : 0);
    if (!outer || params <= 0) return java_annotations_v_obj(outer);
    java_annotations_AnnBuild b = {env, source_path(), 0};
    SsrnxDexClassView view;
    uint32_t list = 0, list_size = 0;
    java_annotations_DexView v = {NULL, 0};
    const bool have_view = java_annotations_acquire_view(b.source, t.owner, &view);
    if (have_view) {
        v.d = view.dex_data;
        v.size = view.dex_size;
        char owner_desc[520];
        snprintf(owner_desc, sizeof(owner_desc), "L%s;", t.owner);
        list = java_annotations_member_annotation_offset(&v, view.class_def_offset, TARGET_PARAMETERS, owner_desc,
                                        t.name, t.sig);
        if (!list || !java_annotations_read_u32(&v, list, &list_size) ||
            !java_annotations_in_bounds(&v, (uint64_t)list + 4u, (uint64_t)list_size * 4u)) list_size = 0;
    }
    const int32_t offset = list_size < (uint32_t)params ? params - (int32_t)list_size : 0;
    for (int32_t i = 0; i < params; ++i) {
        void *inner = NULL;
        if (i >= offset && (uint32_t)(i - offset) < list_size)
            inner = java_annotations_annotations_of_set(&b, &v, java_annotations_rd32(v.d + list + 4u + (uint32_t)(i - offset) * 4u));
        else
            inner = java_annotations_annotation_array(env, NULL, 0);
        if (inner) {
            (void)ssrnx_jni_set_object_array_element_host(env, outer, i, inner);
            ssrnx_jni_delete_local_ref_host(env, inner);
        }
    }
    if (have_view) java_annotations_release_view(&view);
    return java_annotations_v_obj(outer);
}

static SsrnxJniValue java_annotations_class_is_annotation(void *env, void *token, const SsrnxJniValue *a, size_t n,
                                         void *u) {
    (void)a; (void)u;
    SsrnxJniValue out = {0};
    if (n != 0) return out;
    const java_annotations_AnnTargetRef t = java_annotations_target_of(env, token, TARGET_CLASS);
    SsrnxDexClassView view;
    if (t.valid && java_annotations_acquire_view(source_path(), t.owner, &view)) {
        const java_annotations_DexView v = {view.dex_data, view.dex_size};
        uint32_t flags = 0;
        out.z = java_annotations_read_u32(&v, (uint64_t)view.class_def_offset + 4u, &flags) &&
                (flags & ACC_ANNOTATION) != 0;
        java_annotations_release_view(&view);
    }
    return out;
}

/* ---------- class nesting ----------
 *
 * D8/dx record JVMS InnerClasses/EnclosingMethod as SYSTEM annotations:
 * dalvik.annotation.InnerClass{accessFlags, name} on every member, local and
 * anonymous class (name null for anonymous), EnclosingClass{value} for member
 * classes, EnclosingMethod{value} for classes declared inside a method or
 * constructor. Class.isAnonymousClass/isLocalClass/isMemberClass and
 * getEnclosingClass/getDeclaringClass/getEnclosingMethod/Constructor follow
 * the JLS 14.3/15.9.5 definitions over them, as ART does. Classes without DEX
 * metadata (SSRNX platform classes) are top level. */

typedef struct java_annotations_ClassNesting {
    bool anonymous;          /* InnerClass present with a null name */
    bool member;             /* EnclosingClass present, not anonymous and not in a method */
    bool in_method;          /* EnclosingMethod present */
    char enclosing[512];     /* immediately enclosing class, "" when top level */
    char method_name[256];   /* EnclosingMethod name ("<init>" for constructors) */
    char method_params[512]; /* EnclosingMethod parameter descriptor "(...)" */
} java_annotations_ClassNesting;

/* Positions *off at the encoded_value of element `name` of a SYSTEM annotation. */
static bool java_annotations_system_element(const java_annotations_DexView *v, uint32_t set, const char *type_desc, const char *name,
                           size_t *off) {
    size_t cur = 0;
    uint32_t count = 0;
    if (!java_annotations_find_in_set(v, set, type_desc, VIS_SYSTEM, &cur, &count)) return false;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t name_idx = 0;
        if (!java_annotations_read_uleb(v, &cur, &name_idx)) return false;
        const char *element = java_annotations_dex_string(v, name_idx);
        if (element && strcmp(element, name) == 0) {
            *off = cur;
            return cur < v->size;
        }
        if (!java_annotations_skip_value(v, &cur, 0)) return false;
    }
    return false;
}

/* Index payload of an encoded_value of the given type (type/method index). */
static bool java_annotations_system_index(const java_annotations_DexView *v, size_t off, uint8_t type, uint32_t *out) {
    if ((v->d[off] & 0x1fu) != type) return false;
    size_t pos = off + 1u;
    bool ok = true;
    *out = (uint32_t)java_annotations_read_compact(v, &pos, (unsigned)(v->d[off] >> 5) + 1u, &ok);
    return ok;
}

static bool java_annotations_class_nesting(const char *class_name, java_annotations_ClassNesting *out) {
    memset(out, 0, sizeof(*out));
    SsrnxDexClassView view;
    if (!class_name || class_name[0] == '[' || !java_annotations_acquire_view(source_path(), class_name, &view)) return false;
    const java_annotations_DexView v = {view.dex_data, view.dex_size};
    const uint32_t set = java_annotations_class_annotation_set(&v, view.class_def_offset);
    size_t off = 0;
    uint32_t idx = 0;
    bool inner = false, enclosing_class = false;
    if (set && java_annotations_system_element(&v, set, "Ldalvik/annotation/InnerClass;", "name", &off)) {
        inner = true;
        out->anonymous = (v.d[off] & 0x1fu) == EV_NULL;
    }
    if (set && java_annotations_system_element(&v, set, "Ldalvik/annotation/EnclosingClass;", "value", &off) &&
        java_annotations_system_index(&v, off, EV_TYPE, &idx))
        enclosing_class = java_annotations_desc_to_name(java_annotations_dex_type(&v, idx), out->enclosing, sizeof(out->enclosing));
    if (set && java_annotations_system_element(&v, set, "Ldalvik/annotation/EnclosingMethod;", "value", &off) &&
        java_annotations_system_index(&v, off, EV_METHOD, &idx)) {
        const char *owner = NULL, *name = NULL;
        char sig[512];
        if (java_annotations_method_identity(&v, idx, &owner, &name, sig, sizeof(sig))) {
            char *close = strchr(sig, ')');
            if (close) close[1] = '\0';
            out->in_method = close && snprintf(out->method_name, sizeof(out->method_name), "%s", name) <
                                          (int)sizeof(out->method_name) &&
                             snprintf(out->method_params, sizeof(out->method_params), "%s", sig) <
                                 (int)sizeof(out->method_params);
            if (!enclosing_class) (void)java_annotations_desc_to_name(owner, out->enclosing, sizeof(out->enclosing));
        }
    }
    out->anonymous = inner && out->anonymous;
    out->member = enclosing_class && inner && !out->anonymous && !out->in_method;
    java_annotations_release_view(&view);
    return true;
}

/* ART uses InnerClass.accessFlags for nested classes, not class_def flags. */
static bool java_annotations_reflected_class_flags(void *env, void *cls, uint32_t *out) {
    char name[512];
    if (!ssrnx_jni_class_ref_name_host(cls, name, sizeof(name))) return false;
    const char *base = name;
    while (*base == '[') ++base;
    const bool array = base != name;
    void *component = array ? ssrnx_jni_class_ref_for_descriptor_host(env, base) : NULL;
    if (array && (!component || !ssrnx_jni_class_ref_name_host(component, name, sizeof(name)))) {
        if (component) ssrnx_jni_delete_local_ref_host(env, component);
        return false;
    }
    uint32_t flags = 0;
    SsrnxDexClassView view;
    if (strcmp(name, "boolean") == 0 || strcmp(name, "byte") == 0 ||
        strcmp(name, "char") == 0 || strcmp(name, "short") == 0 ||
        strcmp(name, "int") == 0 || strcmp(name, "long") == 0 ||
        strcmp(name, "float") == 0 || strcmp(name, "double") == 0 ||
        strcmp(name, "void") == 0) {
        flags = 0x411u; /* public abstract final */
    } else if (java_annotations_acquire_view(source_path(), name, &view)) {
        const java_annotations_DexView v = {view.dex_data, view.dex_size};
        size_t off = 0;
        bool valid = java_annotations_read_u32(&v, (uint64_t)view.class_def_offset + 4u, &flags);
        if (valid && java_annotations_system_element(&v, java_annotations_class_annotation_set(&v, view.class_def_offset),
                                     "Ldalvik/annotation/InnerClass;", "accessFlags", &off)) {
            const uint8_t head = v.d[off++];
            valid = (head & 0x1fu) == EV_INT && (head >> 5) <= 3u;
            if (valid) flags = (uint32_t)java_annotations_read_compact(&v, &off, (head >> 5) + 1u, &valid);
        }
        java_annotations_release_view(&view);
        if (!valid) {
            if (component) ssrnx_jni_delete_local_ref_host(env, component);
            (void)ssrnx_jni_throw_new_by_name(env, "java/lang/ClassFormatError", "invalid DEX class modifiers");
            return false;
        }
    } else if (ssrnx_jni_class_is_interface_host(component ? component : cls)) {
        flags = 0x601u; /* public interface abstract */
    }
    if (component) ssrnx_jni_delete_local_ref_host(env, component);
    flags &= 0xffffu;
    if (array) {
        if (flags & 0x200u) flags &= ~0x208u; /* Android removes interface/static */
        flags |= 0x410u;
    }
    *out = flags;
    return true;
}

static SsrnxJniValue java_annotations_class_modifiers(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)u;
    void *self = n == 0 ? ssrnx_jni_new_local_from_token(env, token) : NULL;
    uint32_t flags = 0;
    if (self) (void)java_annotations_reflected_class_flags(env, self, &flags);
    if (self) ssrnx_jni_delete_local_ref_host(env, self);
    return (SsrnxJniValue){.i = (int32_t)flags};
}

static SsrnxJniValue java_annotations_class_is_synthetic(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    SsrnxJniValue flags = java_annotations_class_modifiers(env, token, a, n, u);
    return (SsrnxJniValue){.z = (flags.i & 0x1000u) != 0};
}

static SsrnxJniValue java_annotations_class_is_enum(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    SsrnxJniValue flags = java_annotations_class_modifiers(env, token, a, n, u);
    bool is_enum = false;
    void *self = n == 0 && (flags.i & 0x4000u) ? ssrnx_jni_new_local_from_token(env, token) : NULL;
    void *super = self ? ssrnx_jni_class_get_superclass_host(env, self) : NULL;
    char name[512];
    is_enum = super && ssrnx_jni_class_ref_name_host(super, name, sizeof(name)) && strcmp(name, "java/lang/Enum") == 0;
    if (super) ssrnx_jni_delete_local_ref_host(env, super);
    if (self) ssrnx_jni_delete_local_ref_host(env, self);
    return (SsrnxJniValue){.z = is_enum};
}

/* MemberClasses lists declared members; '$' names alone imply no membership. */
static void *java_annotations_declared_class_array(void *env, void *cls) {
    char name[512];
    if (!ssrnx_jni_class_ref_name_host(cls, name, sizeof(name))) return NULL;
    SsrnxDexClassView view;
    if (name[0] == '[' || !java_annotations_acquire_view(source_path(), name, &view))
        return ssrnx_jni_new_local_reference_array(env, "[Ljava/lang/Class;", 0);
    const java_annotations_DexView v = {view.dex_data, view.dex_size};
    size_t off = 0;
    uint32_t count = 0;
    void *array = NULL;
    if (!java_annotations_system_element(&v, java_annotations_class_annotation_set(&v, view.class_def_offset),
                         "Ldalvik/annotation/MemberClasses;", "value", &off)) {
        array = ssrnx_jni_new_local_reference_array(env, "[Ljava/lang/Class;", 0);
        goto done;
    }
    if (v.d[off++] != EV_ARRAY || !java_annotations_read_uleb(&v, &off, &count) ||
        count > INT32_MAX || count > (v.size - off) / 2u) goto malformed;
    array = ssrnx_jni_new_local_reference_array(env, "[Ljava/lang/Class;", (int32_t)count);
    if (!array) goto done;
    for (uint32_t i = 0; i < count; ++i) {
        if (off >= v.size) goto malformed;
        uint8_t head = v.d[off++];
        bool valid = (head & 0x1fu) == EV_TYPE && (head >> 5) <= 3u;
        uint32_t idx = valid ? (uint32_t)java_annotations_read_compact(&v, &off, (head >> 5) + 1u, &valid) : 0;
        const char *desc = valid ? java_annotations_dex_type(&v, idx) : NULL;
        if (!desc || strlen(desc) < 3u || desc[0] != 'L' || desc[strlen(desc) - 1u] != ';') goto malformed;
        void *member = ssrnx_jni_class_ref_for_descriptor_host(env, desc);
        if (!member || ssrnx_jni_set_object_array_element_host(env, array, (int32_t)i, member) != SSRNX_OK) {
            if (member) ssrnx_jni_delete_local_ref_host(env, member);
            ssrnx_jni_delete_local_ref_host(env, array);
            array = NULL;
            if (!ssrnx_jni_exception_pending_host(env))
                (void)ssrnx_jni_throw_new_by_name(env, "java/lang/NoClassDefFoundError", desc);
            goto done;
        }
        ssrnx_jni_delete_local_ref_host(env, member);
    }
    goto done;
malformed:
    if (array) ssrnx_jni_delete_local_ref_host(env, array);
    array = NULL;
    (void)ssrnx_jni_throw_new_by_name(env, "java/lang/ClassFormatError", "invalid DEX MemberClasses annotation");
done:
    java_annotations_release_view(&view);
    return array;
}

static SsrnxJniValue java_annotations_class_get_declared_classes(void *env, void *token, const SsrnxJniValue *a,
                                              size_t n, void *u) {
    (void)a; (void)u;
    void *self = n == 0 ? ssrnx_jni_new_local_from_token(env, token) : NULL;
    void *array = self ? java_annotations_declared_class_array(env, self) : NULL;
    if (self) ssrnx_jni_delete_local_ref_host(env, self);
    return java_annotations_v_obj(array);
}

static SsrnxJniValue java_annotations_class_get_classes(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)u;
    void *current = n == 0 ? ssrnx_jni_new_local_from_token(env, token) : NULL;
    void **members = NULL;
    size_t count = 0;
    void *result = NULL;
    while (current && !ssrnx_jni_exception_pending_host(env)) {
        void *declared = java_annotations_declared_class_array(env, current);
        if (!declared) break;
        int32_t length = ssrnx_jni_array_length_host(declared);
        for (int32_t i = 0; i < length && !ssrnx_jni_exception_pending_host(env); ++i) {
            void *member = NULL;
            uint32_t flags = 0;
            (void)ssrnx_jni_get_object_array_element_host(env, declared, i, &member);
            if (member && java_annotations_reflected_class_flags(env, member, &flags) && (flags & 1u)) {
                void **grown = count < INT32_MAX && count < SIZE_MAX / sizeof(*members) - 1u
                    ? realloc(members, (count + 1u) * sizeof(*members)) : NULL;
                if (!grown) {
                    ssrnx_jni_delete_local_ref_host(env, member);
                    (void)ssrnx_jni_throw_new_by_name(env, "java/lang/OutOfMemoryError", "public member classes");
                    break;
                }
                members = grown;
                members[count++] = member;
            } else if (member) ssrnx_jni_delete_local_ref_host(env, member);
        }
        ssrnx_jni_delete_local_ref_host(env, declared);
        void *super = ssrnx_jni_class_get_superclass_host(env, current);
        ssrnx_jni_delete_local_ref_host(env, current);
        current = super;
    }
    if (current) ssrnx_jni_delete_local_ref_host(env, current);
    if (!ssrnx_jni_exception_pending_host(env)) {
        result = ssrnx_jni_new_local_reference_array(env, "[Ljava/lang/Class;", (int32_t)count);
        for (size_t i = 0; result && i < count; ++i)
            (void)ssrnx_jni_set_object_array_element_host(env, result, (int32_t)i, members[i]);
    }
    for (size_t i = 0; i < count; ++i) ssrnx_jni_delete_local_ref_host(env, members[i]);
    free(members);
    return java_annotations_v_obj(result);
}

/* ART joins the String[] in the SYSTEM dalvik.annotation.Signature annotation.
 * Keep it separate from user-visible RUNTIME annotations. The same pinned
 * libcore parser supplies parameterized types, bounds, wildcards and arrays. */
static void *java_annotations_generic_signature(void *env, const SsrnxDexClassView *view, uint32_t set) {
    const java_annotations_DexView v = {view->dex_data, view->dex_size};
    size_t off = 0;
    if (!java_annotations_system_element(&v, set,
                        "Ldalvik/annotation/Signature;", "value", &off)) return NULL;
    if (v.d[off++] != EV_ARRAY) goto malformed;
    uint32_t count = 0;
    if (!java_annotations_read_uleb(&v, &off, &count)) goto malformed;
    char *text = NULL;
    size_t length = 0;
    for (uint32_t i = 0; i < count; ++i) {
        if (!java_annotations_in_bounds(&v, off, 1) || (v.d[off] & 0x1fu) != EV_STRING) {
            free(text); goto malformed;
        }
        const unsigned bytes = (unsigned)(v.d[off++] >> 5) + 1u;
        bool ok = true;
        uint64_t idx = java_annotations_read_compact(&v, &off, bytes, &ok);
        const char *part = ok && idx <= UINT32_MAX ? java_annotations_dex_string(&v, (uint32_t)idx) : NULL;
        if (!part) { free(text); goto malformed; }
        size_t size = strlen(part);
        if (size > SIZE_MAX - length - 1u) { free(text); goto oom; }
        char *grown = realloc(text, length + size + 1u);
        if (!grown) { free(text); goto oom; }
        text = grown;
        memcpy(text + length, part, size + 1u);
        length += size;
    }
    void *result = ssrnx_jni_new_local_string_utf(env, text ? text : "");
    free(text);
    return result;
malformed:
    (void)ssrnx_jni_throw_new_by_name(env, "java/lang/reflect/GenericSignatureFormatError", "invalid DEX Signature annotation");
    return NULL;
oom:
    (void)ssrnx_jni_throw_new_by_name(env, "java/lang/OutOfMemoryError", "generic signature");
    return NULL;
}

static SsrnxJniValue java_annotations_class_get_interfaces(void *env, void *token, const SsrnxJniValue *a,
                                         size_t n, void *u) {
    (void)a; (void)u;
    const java_annotations_AnnTargetRef target = java_annotations_target_of(env, token, TARGET_CLASS);
    if (n != 0 || !target.valid) return java_annotations_v_obj(NULL);
    SsrnxDexClassView view;
    if (!java_annotations_acquire_view(source_path(), target.owner, &view)) {
        void *self = ssrnx_jni_new_local_from_token(env, token);
        void *array = ssrnx_jni_class_get_interfaces_host(env, self);
        if (self) ssrnx_jni_delete_local_ref_host(env, self);
        return java_annotations_v_obj(array);
    }
    const java_annotations_DexView v = {view.dex_data, view.dex_size};
    uint32_t off = 0, count = 0;
    bool valid = java_annotations_read_u32(&v, (uint64_t)view.class_def_offset + 12u, &off) &&
                 (!off || (java_annotations_read_u32(&v, off, &count) && count <= INT32_MAX &&
                            java_annotations_in_bounds(&v, (uint64_t)off + 4u, (uint64_t)count * 2u)));
    void *array = valid ? ssrnx_jni_new_local_reference_array(env, "[Ljava/lang/Class;", (int32_t)count) : NULL;
    for (uint32_t i = 0; array && i < count; ++i) {
        const uint8_t *p = v.d + off + 4u + i * 2u;
        const char *desc = java_annotations_dex_type(&v, (uint32_t)p[0] | ((uint32_t)p[1] << 8));
        void *iface = ssrnx_jni_class_ref_for_descriptor_host(env, desc);
        if (!iface || ssrnx_jni_set_object_array_element_host(env, array, (int32_t)i, iface) != SSRNX_OK) {
            if (iface) ssrnx_jni_delete_local_ref_host(env, iface);
            break;
        }
        ssrnx_jni_delete_local_ref_host(env, iface);
    }
    java_annotations_release_view(&view);
    if (!valid) (void)ssrnx_jni_throw_new_by_name(env, "java/lang/ClassFormatError", "invalid DEX interfaces");
    return java_annotations_v_obj(array);
}

static SsrnxJniValue java_annotations_class_generic_types(void *env, void *token, const SsrnxJniValue *a,
                                        size_t n, void *u) {
    (void)a;
    const java_annotations_AnnTargetRef target = java_annotations_target_of(env, token, TARGET_CLASS);
    if (n != 0 || !target.valid) return java_annotations_v_obj(NULL);
    void *signature = NULL;
    SsrnxDexClassView view;
    if (java_annotations_acquire_view(source_path(), target.owner, &view)) {
        const java_annotations_DexView v = {view.dex_data, view.dex_size};
        signature = java_annotations_generic_signature(env, &view, java_annotations_class_annotation_set(&v, view.class_def_offset));
        java_annotations_release_view(&view);
    }
    SsrnxJniValue args[3] = {{0}}, out = {0};
    args[0].l = ssrnx_jni_new_local_from_token(env, token);
    args[1].l = signature;
    args[2].i = (int32_t)(intptr_t)u;
    bool found = false;
    if (!ssrnx_jni_exception_pending_host(env))
        (void)ssrnx_jni_invoke_method_host(env, NULL, "com/apn64/reflect/ClassTypes", "forClass",
            "(Ljava/lang/Class;Ljava/lang/String;I)Ljava/lang/Object;", true, args, &out, &found);
    if (args[0].l) ssrnx_jni_delete_local_ref_host(env, args[0].l);
    if (signature) ssrnx_jni_delete_local_ref_host(env, signature);
    return out;
}

static SsrnxJniValue java_annotations_member_generic_types(void *env, void *token, const SsrnxJniValue *a,
                                         size_t n, void *u) {
    (void)a;
    const java_annotations_AnnTargetRef target = java_annotations_target_of(env, token, TARGET_METHOD);
    if (n != 0 || !target.valid) return java_annotations_v_obj(NULL);
    void *signature = NULL;
    SsrnxDexClassView view;
    if (java_annotations_acquire_view(source_path(), target.owner, &view)) {
        const java_annotations_DexView v = {view.dex_data, view.dex_size};
        char owner_desc[sizeof(target.owner) + 3];
        snprintf(owner_desc, sizeof(owner_desc), "L%s;", target.owner);
        signature = java_annotations_generic_signature(env, &view, java_annotations_member_annotation_offset(&v, view.class_def_offset,
            TARGET_METHOD, owner_desc, target.name, target.sig));
        java_annotations_release_view(&view);
    }
    SsrnxJniValue args[4] = {{0}}, out = {0};
    args[0].l = ssrnx_jni_new_local_from_token(env, token);
    args[1].l = ssrnx_jni_find_class_ref(env, target.owner);
    args[2].l = signature;
    args[3].i = (int32_t)(intptr_t)u;
    bool found = false;
    if (!ssrnx_jni_exception_pending_host(env))
        (void)ssrnx_jni_invoke_method_host(env, NULL, "com/apn64/reflect/ClassTypes", "forMember",
            "(Ljava/lang/reflect/GenericDeclaration;Ljava/lang/Class;Ljava/lang/String;I)Ljava/lang/Object;",
            true, args, &out, &found);
    for (size_t i = 0; i < 3; ++i)
        if (args[i].l) ssrnx_jni_delete_local_ref_host(env, args[i].l);
    return out;
}

static bool java_annotations_receiver_nesting(void *env, void *token, java_annotations_ClassNesting *out) {
    const java_annotations_AnnTargetRef t = java_annotations_target_of(env, token, TARGET_CLASS);
    return t.valid && java_annotations_class_nesting(t.owner, out);
}

static SsrnxJniValue java_annotations_class_is_anonymous(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)u;
    SsrnxJniValue out = {0};
    java_annotations_ClassNesting nest;
    out.z = n == 0 && java_annotations_receiver_nesting(env, token, &nest) && nest.anonymous;
    return out;
}

static SsrnxJniValue java_annotations_class_is_local(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)u;
    SsrnxJniValue out = {0};
    java_annotations_ClassNesting nest;
    out.z = n == 0 && java_annotations_receiver_nesting(env, token, &nest) && nest.in_method && !nest.anonymous;
    return out;
}

static SsrnxJniValue java_annotations_class_is_member(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)u;
    SsrnxJniValue out = {0};
    java_annotations_ClassNesting nest;
    out.z = n == 0 && java_annotations_receiver_nesting(env, token, &nest) && nest.member;
    return out;
}

static SsrnxJniValue java_annotations_class_nesting_class(void *env, void *token, size_t n, bool declaring) {
    java_annotations_ClassNesting nest;
    if (n != 0 || !java_annotations_receiver_nesting(env, token, &nest) || !nest.enclosing[0] || (declaring && !nest.member))
        return java_annotations_v_obj(NULL);
    void *found = ssrnx_jni_find_class_ref(env, nest.enclosing);
    if (!found) java_annotations_clear_exception(env);
    return java_annotations_v_obj(found);
}

static SsrnxJniValue java_annotations_class_get_enclosing_class(void *env, void *token, const SsrnxJniValue *a, size_t n,
                                               void *u) {
    (void)a; (void)u;
    return java_annotations_class_nesting_class(env, token, n, false);
}

static SsrnxJniValue java_annotations_class_get_declaring_class(void *env, void *token, const SsrnxJniValue *a, size_t n,
                                               void *u) {
    (void)a; (void)u;
    return java_annotations_class_nesting_class(env, token, n, true);
}

static SsrnxJniValue java_annotations_class_enclosing_executable(void *env, void *token, size_t n, bool constructor) {
    java_annotations_ClassNesting nest;
    if (n != 0 || !java_annotations_receiver_nesting(env, token, &nest) || !nest.in_method ||
        (strcmp(nest.method_name, "<init>") == 0) != constructor || strcmp(nest.method_name, "<clinit>") == 0)
        return java_annotations_v_obj(NULL);
    void *owner = ssrnx_jni_find_class_ref(env, nest.enclosing);
    if (!owner) { java_annotations_clear_exception(env); return java_annotations_v_obj(NULL); }
    void *member = ssrnx_jni_class_get_reflected_method_host(env, owner, nest.method_name, nest.method_params,
                                                             true, false);
    ssrnx_jni_delete_local_ref_host(env, owner);
    return java_annotations_v_obj(member);
}

static SsrnxJniValue java_annotations_class_get_enclosing_method(void *env, void *token, const SsrnxJniValue *a, size_t n,
                                                void *u) {
    (void)a; (void)u;
    return java_annotations_class_enclosing_executable(env, token, n, false);
}

static SsrnxJniValue java_annotations_class_get_enclosing_constructor(void *env, void *token, const SsrnxJniValue *a,
                                                     size_t n, void *u) {
    (void)a; (void)u;
    return java_annotations_class_enclosing_executable(env, token, n, true);
}

/* Class.getTypeName(): the binary name, with arrays as "component[]" per
 * dimension ("java.lang.String[]", "int[][]") as java.lang.reflect.Type wants. */
static SsrnxJniValue java_annotations_class_get_type_name(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)u;
    const java_annotations_AnnTargetRef t = java_annotations_target_of(env, token, TARGET_CLASS);
    if (n != 0 || !t.valid) return java_annotations_v_obj(NULL);
    const char *p = t.owner;
    size_t dims = 0;
    while (*p == '[') { ++dims; ++p; }
    char name[600] = {0};
    if (!dims) {
        snprintf(name, sizeof(name), "%s", t.owner);
    } else if (*p == 'L') {
        const char *end = strchr(p, ';');
        snprintf(name, sizeof(name), "%.*s", end ? (int)(end - p - 1) : (int)strlen(p + 1), p + 1);
    } else {
        static const char *const prim[][2] = {{"Z", "boolean"}, {"B", "byte"}, {"C", "char"}, {"S", "short"},
                                              {"I", "int"}, {"J", "long"}, {"F", "float"}, {"D", "double"}};
        for (size_t i = 0; i < sizeof(prim) / sizeof(prim[0]); ++i)
            if (*p == prim[i][0][0]) snprintf(name, sizeof(name), "%s", prim[i][1]);
    }
    for (char *c = name; *c; ++c) if (*c == '/') *c = '.';
    for (size_t i = 0; i < dims && strlen(name) + 2u < sizeof(name); ++i) strcat(name, "[]");
    return java_annotations_v_obj(ssrnx_jni_new_local_string_utf(env, name));
}

/* ---------- Annotation equals / hashCode / toString ---------- */

static int32_t java_annotations_java_hash_text(const char *s) {
    uint32_t h = 0;
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) h = h * 31u + *p;
    return (int32_t)h;
}

static int32_t java_annotations_object_hash(void *env, void *obj) {
    if (!obj) return 0;
    char cls[512] = "";
    if (!ssrnx_jni_object_class_name_host(obj, cls, sizeof(cls))) return 0;
    if (cls[0] == '[') {
        const int32_t n = ssrnx_jni_array_length_host(obj);
        uint32_t h = 1;
        for (int32_t i = 0; i < n; ++i) {
            uint32_t eh = 0;
            if (cls[1] == 'L' || cls[1] == '[') {
                void *e = NULL;
                if (ssrnx_jni_get_object_array_element_host(env, obj, i, &e) == SSRNX_OK && e) {
                    eh = (uint32_t)java_annotations_object_hash(env, e);
                    ssrnx_jni_delete_local_ref_host(env, e);
                }
            } else {
                uint64_t bits = 0;
                (void)ssrnx_jni_primitive_array_get_host(obj, cls[1], i, &bits);
                switch (cls[1]) {
                    case 'Z': eh = bits ? 1231u : 1237u; break;
                    case 'J': case 'D': eh = (uint32_t)(bits ^ (bits >> 32)); break;
                    default: eh = (uint32_t)bits; break;
                }
            }
            h = 31u * h + eh;
        }
        return (int32_t)h;
    }
    SsrnxJniValue out = {0};
    bool found = false;
    if (ssrnx_jni_invoke_method_host(env, obj, cls, "hashCode", "()I", false, NULL, &out, &found) !=
            SSRNX_OK || !found) return 0;
    return out.i;
}

static bool java_annotations_values_equal(void *env, void *x, void *y) {
    if (!x || !y) return x == y;
    if (ssrnx_jni_same_object_host(x, y)) return true;
    char cx[512] = "", cy[512] = "";
    if (!ssrnx_jni_object_class_name_host(x, cx, sizeof(cx)) ||
        !ssrnx_jni_object_class_name_host(y, cy, sizeof(cy))) return false;
    if (cx[0] == '[') {
        if (strcmp(cx, cy) != 0) return false;
        const int32_t n = ssrnx_jni_array_length_host(x);
        if (n != ssrnx_jni_array_length_host(y)) return false;
        for (int32_t i = 0; i < n; ++i) {
            if (cx[1] == 'L' || cx[1] == '[') {
                void *ex = NULL, *ey = NULL;
                (void)ssrnx_jni_get_object_array_element_host(env, x, i, &ex);
                (void)ssrnx_jni_get_object_array_element_host(env, y, i, &ey);
                const bool same = java_annotations_values_equal(env, ex, ey);
                if (ex) ssrnx_jni_delete_local_ref_host(env, ex);
                if (ey) ssrnx_jni_delete_local_ref_host(env, ey);
                if (!same) return false;
            } else {
                uint64_t bx = 0, by = 0;
                (void)ssrnx_jni_primitive_array_get_host(x, cx[1], i, &bx);
                (void)ssrnx_jni_primitive_array_get_host(y, cx[1], i, &by);
                if (bx != by) return false;
            }
        }
        return true;
    }
    SsrnxJniValue arg = {0}, out = {0};
    arg.l = y;
    bool found = false;
    if (ssrnx_jni_invoke_method_host(env, x, cx, "equals", "(Ljava/lang/Object;)Z", false, &arg,
                                     &out, &found) != SSRNX_OK || !found) return false;
    return out.z != 0;
}

static bool java_annotations_append_value(void *env, void *value, char *out, size_t cap, size_t *used) {
    if (!value) return java_annotations_append_text(out, cap, used, "null");
    char cls[512] = "";
    (void)ssrnx_jni_object_class_name_host(value, cls, sizeof(cls));
    if (cls[0] == '[') {
        const int32_t n = ssrnx_jni_array_length_host(value);
        if (!java_annotations_append_text(out, cap, used, "[")) return false;
        for (int32_t i = 0; i < n; ++i) {
            if (i && !java_annotations_append_text(out, cap, used, ", ")) return false;
            if (cls[1] == 'L' || cls[1] == '[') {
                void *e = NULL;
                (void)ssrnx_jni_get_object_array_element_host(env, value, i, &e);
                const bool ok = java_annotations_append_value(env, e, out, cap, used);
                if (e) ssrnx_jni_delete_local_ref_host(env, e);
                if (!ok) return false;
                continue;
            }
            uint64_t bits = 0;
            (void)ssrnx_jni_primitive_array_get_host(value, cls[1], i, &bits);
            void *boxed = java_annotations_box_primitive(env, cls[1], bits);
            const bool ok = java_annotations_append_value(env, boxed, out, cap, used);
            if (boxed) ssrnx_jni_delete_local_ref_host(env, boxed);
            if (!ok) return false;
        }
        return java_annotations_append_text(out, cap, used, "]");
    }
    if (strcmp(cls, "java/lang/Class") == 0) {
        char name[512] = "";
        if (!ssrnx_jni_class_ref_name_host(value, name, sizeof(name))) return false;
        for (char *p = name; *p; ++p) if (*p == '/') *p = '.';
        return java_annotations_append_text(out, cap, used, name) && java_annotations_append_text(out, cap, used, ".class");
    }
    SsrnxJniValue str = {0};
    bool found = false;
    if (ssrnx_jni_invoke_method_host(env, value, cls, "toString", "()Ljava/lang/String;", false, NULL,
                                     &str, &found) != SSRNX_OK || !found || !str.l)
        return java_annotations_append_text(out, cap, used, "?");
    char *text = ssrnx_jni_string_utf_dup(str.l);
    ssrnx_jni_delete_local_ref_host(env, str.l);
    const bool quoted = strcmp(cls, "java/lang/String") == 0;
    const bool ok = text && (!quoted || java_annotations_append_text(out, cap, used, "\"")) &&
                    java_annotations_append_text(out, cap, used, text) &&
                    (!quoted || java_annotations_append_text(out, cap, used, "\""));
    free(text);
    return ok;
}

static void *java_annotations_annotation_to_string(void *env, const java_annotations_AnnPayload *p) {
    char text[4096];
    size_t used = 0;
    text[0] = '\0';
    char type[512];
    snprintf(type, sizeof(type), "%s", p->type_name);
    for (char *c = type; *c; ++c) if (*c == '/') *c = '.';
    bool ok = java_annotations_append_text(text, sizeof(text), &used, "@") && java_annotations_append_text(text, sizeof(text), &used, type) &&
              java_annotations_append_text(text, sizeof(text), &used, "(");
    for (size_t i = 0; ok && i < p->count; ++i) {
        if (!p->members[i].present) continue;
        ok = (used && text[used - 1] == '(' ? true : java_annotations_append_text(text, sizeof(text), &used, ", ")) &&
             java_annotations_append_text(text, sizeof(text), &used, p->members[i].name) &&
             java_annotations_append_text(text, sizeof(text), &used, "=") &&
             java_annotations_append_value(env, p->members[i].value, text, sizeof(text), &used);
    }
    if (ok) (void)java_annotations_append_text(text, sizeof(text), &used, ")");
    return ssrnx_jni_new_local_string_utf(env, text);
}

/* InvocationHandler.invoke(proxy, Method, Object[]) for annotation proxies. */
static SsrnxJniValue java_annotations_factory_invoke(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    java_annotations_AnnPayload *p = (java_annotations_AnnPayload *)ssrnx_jni_get_token_host_data(token);
    char owner[512] = "", name[256] = "", sig[512] = "";
    bool is_static = false, iface = false;
    if (!p || p->magic != PAYLOAD_MAGIC || n != 3 || !a[1].l ||
        !ssrnx_jni_reflected_method_info_host(a[1].l, owner, sizeof(owner), name, sizeof(name), sig,
                                              sizeof(sig), &is_static, &iface)) {
        (void)ssrnx_jni_throw_new_by_name(env, "java/lang/IllegalStateException",
                                          "annotation handler has no annotation state");
        return java_annotations_v_obj(NULL);
    }
    if (strcmp(name, "annotationType") == 0 && strcmp(sig, "()Ljava/lang/Class;") == 0)
        return java_annotations_v_obj(ssrnx_jni_new_local_ref_host(env, p->type_class));
    if (strcmp(name, "toString") == 0 && strcmp(sig, "()Ljava/lang/String;") == 0)
        return java_annotations_v_obj(java_annotations_annotation_to_string(env, p));
    if (strcmp(name, "hashCode") == 0 && strcmp(sig, "()I") == 0) {
        uint32_t h = 0;
        for (size_t i = 0; i < p->count; ++i)
            if (p->members[i].present)
                h += (127u * (uint32_t)java_annotations_java_hash_text(p->members[i].name)) ^
                     (uint32_t)java_annotations_object_hash(env, p->members[i].value);
        return java_annotations_v_obj(java_annotations_box_primitive(env, 'I', (uint64_t)(int64_t)(int32_t)h));
    }
    if (strcmp(name, "equals") == 0 && strcmp(sig, "(Ljava/lang/Object;)Z") == 0) {
        void *other = NULL;
        if (a[2].l) (void)ssrnx_jni_get_object_array_element_host(env, a[2].l, 0, &other);
        bool equal = other && ssrnx_jni_same_object_host(other, a[0].l);
        java_annotations_AnnPayload *q = equal ? NULL : java_annotations_payload_of(env, other);
        if (q && strcmp(q->type_name, p->type_name) == 0) {
            equal = true;
            for (size_t i = 0; equal && i < p->count; ++i) {
                const java_annotations_AnnMember *mine = &p->members[i];
                const java_annotations_AnnMember *theirs = NULL;
                for (size_t k = 0; k < q->count; ++k)
                    if (strcmp(q->members[k].name, mine->name) == 0) theirs = &q->members[k];
                equal = theirs && theirs->present == mine->present &&
                        java_annotations_values_equal(env, mine->value, theirs->value);
            }
        }
        if (other) ssrnx_jni_delete_local_ref_host(env, other);
        return java_annotations_v_obj(java_annotations_box_primitive(env, 'Z', equal ? 1u : 0u));
    }
    for (size_t i = 0; i < p->count; ++i) {
        const java_annotations_AnnMember *m = &p->members[i];
        if (strcmp(m->name, name) != 0 || strncmp(sig, "()", 2) != 0) continue;
        if (!m->present) break;
        if (!m->value) return java_annotations_v_obj(NULL);
        if (ssrnx_jni_array_length_host(m->value) >= 0) return java_annotations_v_obj(java_annotations_copy_array(env, m->value));
        return java_annotations_v_obj(ssrnx_jni_new_local_ref_host(env, m->value));
    }
    char msg[800];
    snprintf(msg, sizeof(msg), "%s missing element %s", p->type_name, name);
    for (char *c = msg; *c && *c != ' '; ++c) if (*c == '/') *c = '.';
    (void)ssrnx_jni_throw_new_by_name(env, INCOMPLETE_CLASS, msg);
    return java_annotations_v_obj(NULL);
}

/* ---------- registration ---------- */

/* Type's API-30 default implementation calls the receiver's toString. A
 * modeled interface declaration needs a concrete default callback as well as
 * the type graph; otherwise JNI dispatch mistakes it for an abstract member. */
static SsrnxJniValue java_annotations_type_get_type_name(void *env, void *token, const SsrnxJniValue *a,
                                       size_t n, void *u) {
    (void)a; (void)u;
    void *self = n == 0 ? ssrnx_jni_new_local_from_token(env, token) : NULL;
    SsrnxJniValue out = {0};
    char cls[512];
    bool found = false;
    if (self && ssrnx_jni_object_class_name_host(self, cls, sizeof(cls)))
        (void)ssrnx_jni_invoke_method_host(env, self, cls, "toString", "()Ljava/lang/String;",
                                          false, NULL, &out, &found);
    if (self) ssrnx_jni_delete_local_ref_host(env, self);
    return out;
}

SsrnxResult ssrnx_java_annotations_register_members(void) {
    static const char *const k_classes[][2] = {
        {"java/lang/reflect/InvocationHandler", "java/lang/Object"},
        {ANNOTATION_IFACE, "java/lang/Object"},
        {FACTORY_CLASS, "java/lang/Object"},
        {"java/lang/reflect/AccessibleObject", "java/lang/Object"},
        {"java/lang/reflect/Executable", "java/lang/reflect/AccessibleObject"},
        {"java/lang/reflect/Method", "java/lang/reflect/Executable"},
        {"java/lang/reflect/Constructor", "java/lang/reflect/Executable"},
        {"java/lang/reflect/Field", "java/lang/reflect/AccessibleObject"},
        {INCOMPLETE_CLASS, "java/lang/RuntimeException"},
        {"java/lang/annotation/AnnotationTypeMismatchException", "java/lang/RuntimeException"},
        {"java/lang/annotation/AnnotationFormatError", "java/lang/Error"},
        {"java/lang/reflect/GenericSignatureFormatError", "java/lang/ClassFormatError"},
    };
    SsrnxResult rc = SSRNX_OK;
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(k_classes) / sizeof(k_classes[0]); ++i)
        rc = ssrnx_jni_register_class(k_classes[i][0], k_classes[i][1]);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_interface(FACTORY_CLASS, "java/lang/reflect/InvocationHandler");

    /* java.lang.reflect type graph (java.* platform types are always real
     * models, never inert API rows). Libraries name these types directly —
     * protobuf's FieldType.<clinit> allocates a Type[] — so they must exist
     * even where SSRNX offers no generic-signature reflection. */
    static const struct { const char *iface, *super_iface, *method, *sig; } k_reflect_types[] = {
        {"java/lang/reflect/AnnotatedElement", NULL, "isAnnotationPresent", "(Ljava/lang/Class;)Z"},
        {"java/lang/reflect/Type", NULL, "getTypeName", "()Ljava/lang/String;"},
        {"java/lang/reflect/Member", NULL, "getName", "()Ljava/lang/String;"},
        {"java/lang/reflect/GenericDeclaration", "java/lang/reflect/AnnotatedElement", "getTypeParameters",
         "()[Ljava/lang/reflect/TypeVariable;"},
        {"java/lang/reflect/ParameterizedType", "java/lang/reflect/Type", "getActualTypeArguments",
         "()[Ljava/lang/reflect/Type;"},
        {"java/lang/reflect/GenericArrayType", "java/lang/reflect/Type", "getGenericComponentType",
         "()Ljava/lang/reflect/Type;"},
        {"java/lang/reflect/WildcardType", "java/lang/reflect/Type", "getUpperBounds",
         "()[Ljava/lang/reflect/Type;"},
        {"java/lang/reflect/TypeVariable", "java/lang/reflect/Type", "getBounds",
         "()[Ljava/lang/reflect/Type;"},
    };
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(k_reflect_types) / sizeof(k_reflect_types[0]); ++i) {
        rc = ssrnx_jni_register_class(k_reflect_types[i].iface, "java/lang/Object");
        if (rc == SSRNX_OK) {
            if (strcmp(k_reflect_types[i].iface, "java/lang/reflect/Type") == 0)
                rc = ssrnx_jni_register_method(k_reflect_types[i].iface, k_reflect_types[i].method,
                    k_reflect_types[i].sig, false, java_annotations_type_get_type_name, NULL);
            else
                rc = ssrnx_jni_register_interface_method(k_reflect_types[i].iface, k_reflect_types[i].method,
                                                         k_reflect_types[i].sig);
        }
        if (rc == SSRNX_OK && k_reflect_types[i].super_iface)
            rc = ssrnx_jni_register_interface(k_reflect_types[i].iface, k_reflect_types[i].super_iface);
    }
    static const char *const k_reflect_links[][2] = {
        {"java/lang/Class", "java/lang/reflect/GenericDeclaration"},
        {"java/lang/Class", "java/lang/reflect/Type"},
        {"java/lang/Class", "java/lang/reflect/AnnotatedElement"},
        {"java/lang/reflect/AccessibleObject", "java/lang/reflect/AnnotatedElement"},
        {"java/lang/reflect/Executable", "java/lang/reflect/GenericDeclaration"},
        {"java/lang/reflect/Executable", "java/lang/reflect/Member"},
        {"java/lang/reflect/Field", "java/lang/reflect/Member"},
    };
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(k_reflect_links) / sizeof(k_reflect_links[0]); ++i)
        rc = ssrnx_jni_register_interface(k_reflect_links[i][0], k_reflect_links[i][1]);
    static const char *const type_methods[][3] = {
        {"java/lang/reflect/ParameterizedType", "getRawType", "()Ljava/lang/reflect/Type;"},
        {"java/lang/reflect/ParameterizedType", "getOwnerType", "()Ljava/lang/reflect/Type;"},
        {"java/lang/reflect/WildcardType", "getLowerBounds", "()[Ljava/lang/reflect/Type;"},
        {"java/lang/reflect/TypeVariable", "getGenericDeclaration", "()Ljava/lang/reflect/GenericDeclaration;"},
        {"java/lang/reflect/TypeVariable", "getName", "()Ljava/lang/String;"},
    };
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(type_methods) / sizeof(type_methods[0]); ++i)
        rc = ssrnx_jni_register_interface_method(type_methods[i][0], type_methods[i][1], type_methods[i][2]);
    static const char *const generic_methods[][2] = {
        {"getGenericSuperclass", "()Ljava/lang/reflect/Type;"},
        {"getGenericInterfaces", "()[Ljava/lang/reflect/Type;"},
        {"getTypeParameters", "()[Ljava/lang/reflect/TypeVariable;"},
    };
    for (size_t i = 0; rc == SSRNX_OK && i < 3u; ++i)
        rc = ssrnx_jni_register_method("java/lang/Class", generic_methods[i][0], generic_methods[i][1],
                                       false, java_annotations_class_generic_types, (void *)(intptr_t)i);
    static const char *const member_methods[][2] = {
        {"getGenericParameterTypes", "()[Ljava/lang/reflect/Type;"},
        {"getTypeParameters", "()[Ljava/lang/reflect/TypeVariable;"},
        {"getGenericReturnType", "()Ljava/lang/reflect/Type;"},
    };
    const char *const owners[] = {"java/lang/reflect/Method", "java/lang/reflect/Constructor"};
    for (size_t j = 0; rc == SSRNX_OK && j < 2u; ++j)
        for (size_t i = 0; rc == SSRNX_OK && i < (j ? 2u : 3u); ++i)
            rc = ssrnx_jni_register_method(owners[j], member_methods[i][0], member_methods[i][1],
                                           false, java_annotations_member_generic_types, (void *)(intptr_t)i);
    if (rc == SSRNX_OK)
        rc = ssrnx_jni_register_method("java/lang/Class", "getInterfaces", "()[Ljava/lang/Class;",
                                       false, java_annotations_class_get_interfaces, NULL);
    if (rc == SSRNX_OK)
        rc = ssrnx_jni_register_method("java/lang/Class", "getTypeName", "()Ljava/lang/String;", false,
                                       java_annotations_class_get_type_name, NULL);
    if (rc == SSRNX_OK)
        rc = ssrnx_jni_register_interface_method(ANNOTATION_IFACE, "annotationType", "()Ljava/lang/Class;");
    if (rc == SSRNX_OK)
        rc = ssrnx_jni_register_method(FACTORY_CLASS, "invoke",
                                       "(Ljava/lang/Object;Ljava/lang/reflect/Method;[Ljava/lang/Object;)Ljava/lang/Object;",
                                       false, java_annotations_factory_invoke, NULL);

    typedef struct java_annotations_Entry { const char *name, *sig; SsrnxJniMethodCallback cb; } java_annotations_Entry;
    const java_annotations_Entry class_entries[] = {
        {"getClasses", "()[Ljava/lang/Class;", java_annotations_class_get_classes},
        {"getDeclaredClasses", "()[Ljava/lang/Class;", java_annotations_class_get_declared_classes},
        {"getModifiers", "()I", java_annotations_class_modifiers},
        {"isEnum", "()Z", java_annotations_class_is_enum},
        {"isSynthetic", "()Z", java_annotations_class_is_synthetic},
        {"getAnnotation", "(Ljava/lang/Class;)Ljava/lang/annotation/Annotation;", get_annotation_class},
        {"getDeclaredAnnotation", "(Ljava/lang/Class;)Ljava/lang/annotation/Annotation;", get_declared_annotation_class},
        {"isAnnotationPresent", "(Ljava/lang/Class;)Z", is_present_class},
        {"getAnnotations", "()[Ljava/lang/annotation/Annotation;", get_annotations_class},
        {"getDeclaredAnnotations", "()[Ljava/lang/annotation/Annotation;", get_declared_annotations_class},
        {"getAnnotationsByType", "(Ljava/lang/Class;)[Ljava/lang/annotation/Annotation;", by_type_class},
        {"getDeclaredAnnotationsByType", "(Ljava/lang/Class;)[Ljava/lang/annotation/Annotation;", declared_by_type_class},
        {"isAnnotation", "()Z", java_annotations_class_is_annotation},
        {"isAnonymousClass", "()Z", java_annotations_class_is_anonymous},
        {"isLocalClass", "()Z", java_annotations_class_is_local},
        {"isMemberClass", "()Z", java_annotations_class_is_member},
        {"getEnclosingClass", "()Ljava/lang/Class;", java_annotations_class_get_enclosing_class},
        {"getDeclaringClass", "()Ljava/lang/Class;", java_annotations_class_get_declaring_class},
        {"getEnclosingMethod", "()Ljava/lang/reflect/Method;", java_annotations_class_get_enclosing_method},
        {"getEnclosingConstructor", "()Ljava/lang/reflect/Constructor;", java_annotations_class_get_enclosing_constructor},
    };
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(class_entries) / sizeof(class_entries[0]); ++i)
        rc = ssrnx_jni_register_method("java/lang/Class", class_entries[i].name, class_entries[i].sig,
                                       false, class_entries[i].cb, NULL);

    /* Field and Method/Constructor: member annotations are never inherited. */
    const java_annotations_Entry field_entries[] = {
        {"getAnnotation", "(Ljava/lang/Class;)Ljava/lang/annotation/Annotation;", get_annotation_field},
        {"getDeclaredAnnotation", "(Ljava/lang/Class;)Ljava/lang/annotation/Annotation;", get_declared_annotation_field},
        {"isAnnotationPresent", "(Ljava/lang/Class;)Z", is_present_field},
        {"getAnnotations", "()[Ljava/lang/annotation/Annotation;", get_annotations_field},
        {"getDeclaredAnnotations", "()[Ljava/lang/annotation/Annotation;", get_declared_annotations_field},
        {"getAnnotationsByType", "(Ljava/lang/Class;)[Ljava/lang/annotation/Annotation;", by_type_field},
        {"getDeclaredAnnotationsByType", "(Ljava/lang/Class;)[Ljava/lang/annotation/Annotation;", declared_by_type_field},
    };
    const java_annotations_Entry method_entries[] = {
        {"getAnnotation", "(Ljava/lang/Class;)Ljava/lang/annotation/Annotation;", get_annotation_method},
        {"getDeclaredAnnotation", "(Ljava/lang/Class;)Ljava/lang/annotation/Annotation;", get_declared_annotation_method},
        {"isAnnotationPresent", "(Ljava/lang/Class;)Z", is_present_method},
        {"getAnnotations", "()[Ljava/lang/annotation/Annotation;", get_annotations_method},
        {"getDeclaredAnnotations", "()[Ljava/lang/annotation/Annotation;", get_declared_annotations_method},
        {"getAnnotationsByType", "(Ljava/lang/Class;)[Ljava/lang/annotation/Annotation;", by_type_method},
        {"getDeclaredAnnotationsByType", "(Ljava/lang/Class;)[Ljava/lang/annotation/Annotation;", declared_by_type_method},
        {"getParameterAnnotations", "()[[Ljava/lang/annotation/Annotation;", java_annotations_parameter_annotations},
    };
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(field_entries) / sizeof(field_entries[0]); ++i)
        rc = ssrnx_jni_register_method("java/lang/reflect/Field", field_entries[i].name,
                                       field_entries[i].sig, false, field_entries[i].cb, NULL);
    static const char *const k_executables[] = {"java/lang/reflect/Method", "java/lang/reflect/Constructor"};
    for (size_t c = 0; rc == SSRNX_OK && c < 2u; ++c)
        for (size_t i = 0; rc == SSRNX_OK && i < sizeof(method_entries) / sizeof(method_entries[0]); ++i)
            rc = ssrnx_jni_register_method(k_executables[c], method_entries[i].name,
                                           method_entries[i].sig, false, method_entries[i].cb, NULL);
    return rc;
}

void ssrnx_java_annotations_reset(void) {
    void *env = ssrnx_jni_current_env();
    pthread_mutex_lock(&java_annotations_g_ann_lock);
    java_annotations_AnnCacheEntry *cache = java_annotations_g_cache;
    java_annotations_AnnPayload *payloads = java_annotations_g_payloads;
    java_annotations_g_cache = NULL;
    java_annotations_g_payloads = NULL;
    pthread_mutex_unlock(&java_annotations_g_ann_lock);
    while (cache) {
        java_annotations_AnnCacheEntry *next = cache->next;
        if (env && cache->array) ssrnx_jni_delete_global_ref_host(env, cache->array);
        free(cache->key);
        free(cache);
        cache = next;
    }
    /* Like other framework payload resets: handlers that survive the reset see
     * a cleared magic and report an invalid annotation state. */
    while (payloads) {
        java_annotations_AnnPayload *next = payloads->next;
        payloads->magic = 0;
        if (env) {
            java_annotations_free_members(env, payloads->members, payloads->count);
            if (payloads->type_class) ssrnx_jni_delete_global_ref_host(env, payloads->type_class);
        }
        free(payloads->type_name);
        free(payloads);
        payloads = next;
    }
}

#pragma pop_macro("VIS_SYSTEM")
#pragma pop_macro("VIS_RUNTIME")
#pragma pop_macro("TARGET_ENTRY")
#pragma pop_macro("PAYLOAD_MAGIC")
#pragma pop_macro("MAX_NESTING")
#pragma pop_macro("INCOMPLETE_CLASS")
#pragma pop_macro("FACTORY_CLASS")
#pragma pop_macro("ANNOTATION_IFACE")
#pragma pop_macro("ANNOTATION_ARRAY")
#pragma pop_macro("ACC_ANNOTATION")
