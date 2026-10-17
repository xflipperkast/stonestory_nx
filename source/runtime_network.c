/* runtime network. */
#define _GNU_SOURCE 1
#include "runtime.h"


/* ---- java_net ---- */
#pragma push_macro("D")
#pragma push_macro("DSF")
#pragma push_macro("DSSF")
#pragma push_macro("IA")
#pragma push_macro("IA4")
#pragma push_macro("IA6")
#pragma push_macro("IN")
#pragma push_macro("ISA")
#pragma push_macro("M")
#pragma push_macro("MSG_NOSIGNAL")
#pragma push_macro("NET_MAGIC")
#pragma push_macro("OUT")
#pragma push_macro("P")
#pragma push_macro("PARAM")
#pragma push_macro("PROVIDER")
#pragma push_macro("S")
#pragma push_macro("SC")
#pragma push_macro("SESSION")
#pragma push_macro("SF")
#pragma push_macro("SOCK")
#pragma push_macro("SSF")
#pragma push_macro("SSL")
#pragma push_macro("TLS_SOCK")
#pragma push_macro("U")
#pragma push_macro("UDP_PACKET")
#pragma push_macro("UDP_SOCKET")
#pragma push_macro("V")
/* Android 11 java.net/JSSE client sockets over private host BSD descriptors.
 * TLS uses the installed Mbed TLS backend, with mandatory chain verification.
 * Guest descriptors, JNI references and host socket descriptors stay distinct.
 */





#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#include <unistd.h>
#ifdef __SWITCH__

#include <switch.h>
#endif

#define SF "javax/net/SocketFactory"
#define DSF "apn64/net/DefaultSocketFactory"
#define SC "javax/net/ssl/SSLContext"
#define SSF "javax/net/ssl/SSLSocketFactory"
#define DSSF "apn64/net/TlsSocketFactory"
#define SOCK "java/net/Socket"
#define SSL "javax/net/ssl/SSLSocket"
#define TLS_SOCK "apn64/net/TlsSocket"
#define PARAM "javax/net/ssl/SSLParameters"
#define SESSION "apn64/net/TlsSession"
#define IN "apn64/net/SocketInputStream"
#define OUT "apn64/net/SocketOutputStream"
#define ISA "java/net/InetSocketAddress"
#define IA "java/net/InetAddress"
#define IA4 "apn64/net/Inet4Address"
#define IA6 "apn64/net/Inet6Address"
#define PROVIDER "apn64/net/TlsProvider"
#define NET_MAGIC UINT64_C(0x41504b364e455431)
enum { N_ADDRESS = 1, N_CONTEXT, N_FACTORY, N_SOCKET, N_PARAMS, N_PACKET };
typedef struct java_net_NetObject {
    uint64_t magic;
    int kind;
    struct java_net_NetObject *next;
} java_net_NetObject;
typedef struct Address {
    java_net_NetObject object;
    struct sockaddr_storage address;
    socklen_t size;
    char *host;
    int port;
    bool resolved;
} Address;
typedef struct java_net_TlsMaterial {
    mbedtls_x509_crt roots;
    pthread_mutex_t verify;
    bool platform_roots;
    struct java_net_TlsMaterial *next;
} java_net_TlsMaterial;
typedef struct java_net_TlsContext {
    java_net_NetObject object;
    char protocol[24];
    java_net_TlsMaterial *material;
    bool default_context;
} java_net_TlsContext;
typedef struct java_net_Factory {
    java_net_NetObject object;
    java_net_TlsMaterial *material;
} java_net_Factory;
typedef struct java_net_Transport {
    atomic_int fd;
    atomic_bool closed;
    pthread_mutex_t io;
    pthread_mutex_t read, write; /* A blocked receive must not prevent a send. */
    struct java_net_Transport *next;
} java_net_Transport;
typedef struct java_net_Parameters {
    java_net_NetObject object;
    void *ciphers, *protocols, *alpn;
    char *endpoint;
    bool need_auth, want_auth;
} java_net_Parameters;
typedef struct java_net_SocketObject {
    java_net_NetObject object;
    java_net_Transport *transport;
    java_net_TlsMaterial *material;
    java_net_Parameters *parameters;
    struct java_net_SocketObject *parent;
    bool tls, auto_close, tls_failed;
    atomic_bool handshake;
    atomic_bool connected, bound, input_shutdown, output_shutdown;
    atomic_bool closed;
    atomic_int timeout;
    atomic_bool nodelay, keepalive, reuse;
    char *peer_host;
    int peer_port;
    struct sockaddr_storage peer_address, local_address;
    socklen_t peer_size, local_size;
    struct sockaddr_storage datagram_binding;
    socklen_t datagram_binding_size;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_session session;
    mbedtls_ssl_config config;
    mbedtls_ctr_drbg_context rng;
    int *cipher_ids;
    char **alpn_names;
} java_net_SocketObject;

static java_net_NetObject *objects;
static java_net_Transport *java_net_transports;
static java_net_TlsMaterial *java_net_materials;
static void *java_net_default_factory, *default_context, *provider;
static pthread_mutex_t java_net_net_lock = PTHREAD_MUTEX_INITIALIZER;
static SsrnxJniValue java_net_nv(void) { return (SsrnxJniValue){0}; }
static SsrnxJniValue java_net_ni(int32_t i) { return (SsrnxJniValue){.i = i}; }
static SsrnxJniValue java_net_nb(bool b) { return (SsrnxJniValue){.z = b}; }
static SsrnxJniValue no(void *p) { return (SsrnxJniValue){.l = p}; }
static void fail(void *env, const char *kind, const char *message) {
    (void)ssrnx_jni_throw_new_by_name(env, kind, message);
}
static void java_net_oom(void *env) { fail(env, "java/lang/OutOfMemoryError", "Java socket"); }
static int java_net_network_ready(void) {
#ifdef __SWITCH__
    return ssrnx_compat_network_initialize();
#else
    return 0;
#endif
}
static int64_t java_net_millis(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}
static void *text(void *env, const char *s) { return s ? ssrnx_jni_new_local_string_utf(env, s) : NULL; }
static void *element(void *env, void *array, int32_t i) {
    void *ref = NULL;
    (void)ssrnx_jni_get_object_array_element_host(env, array, i, &ref);
    return ref;
}
static void *java_net_new_object(void *env, const char *cls, size_t size, int kind, void **out) {
    *out = NULL;
    java_net_NetObject *p = calloc(1, size);
    void *ref = p ? ssrnx_jni_new_local_object(env, cls) : NULL;
    if (!ref) {
        free(p);
        java_net_oom(env);
        return NULL;
    }
    p->magic = NET_MAGIC;
    p->kind = kind;
    pthread_mutex_lock(&java_net_net_lock);
    p->next = objects;
    objects = p;
    pthread_mutex_unlock(&java_net_net_lock);
    (void)ssrnx_jni_set_object_host_data(ref, p);
    *out = p;
    return ref;
}
static void *payload(void *token, int kind) {
    java_net_NetObject *p = ssrnx_jni_get_token_host_data(token);
    return p && p->magic == NET_MAGIC && p->kind == kind ? p : NULL;
}
static void java_net_replace_ref(void *env, void **slot, void *ref) {
    void *next = ref ? ssrnx_jni_new_global_ref_host(env, ref) : NULL;
    if (*slot)
        ssrnx_jni_delete_global_ref_host(env, *slot);
    *slot = next;
}
static void *strings(void *env, const char *const *values, size_t n) {
    if (n > INT32_MAX) {
        java_net_oom(env);
        return NULL;
    }
    void *array = ssrnx_jni_new_local_reference_array(env, "[Ljava/lang/String;", (int32_t)n);
    for (size_t i = 0; array && i < n; ++i) {
        void *s = text(env, values[i]);
        if (s) {
            (void)ssrnx_jni_set_object_array_element_host(env, array, (int32_t)i, s);
            ssrnx_jni_delete_local_ref_host(env, s);
        }
    }
    return array;
}
static void *java_net_array_copy(void *env, void *src) {
    if (!src)
        return NULL;
    int32_t n = ssrnx_jni_array_length_host(src);
    void *array = ssrnx_jni_new_local_reference_array(env, "[Ljava/lang/String;", n);
    for (int32_t i = 0; array && i < n; ++i) {
        void *v = element(env, src, i);
        (void)ssrnx_jni_set_object_array_element_host(env, array, i, v);
        if (v)
            ssrnx_jni_delete_local_ref_host(env, v);
    }
    return array;
}
static char **java_net_array_text(void *env, void *array) {
    int32_t n = ssrnx_jni_array_length_host(array);
    if (n < 0) {
        fail(env, "java/lang/IllegalArgumentException", "String array");
        return NULL;
    }
    char **values = calloc((size_t)n + 1, sizeof(*values));
    if (!values) {
        java_net_oom(env);
        return NULL;
    }
    for (int32_t i = 0; i < n; ++i) {
        void *s = element(env, array, i);
        values[i] = s ? ssrnx_jni_string_utf_dup(s) : NULL;
        if (s)
            ssrnx_jni_delete_local_ref_host(env, s);
        if (!values[i]) {
            for (int32_t j = 0; j < i; ++j)
                free(values[j]);
            free(values);
            fail(env, "java/lang/IllegalArgumentException", "Null string");
            return NULL;
        }
    }
    return values;
}
static void java_net_free_texts(char **values) {
    if (values) {
        for (size_t i = 0; values[i]; ++i)
            free(values[i]);
        free(values);
    }
}
static void java_net_cipher_name(int id, char *out, size_t cap) {
    const char *s = mbedtls_ssl_get_ciphersuite_name(id);
    size_t i = 0;
    for (; s && s[i] && i + 1 < cap; ++i)
        out[i] = s[i] == '-' ? '_' : s[i];
    out[i] = 0;
}
static bool java_net_supported_cipher(int id) {
    const mbedtls_ssl_ciphersuite_t *suite = mbedtls_ssl_ciphersuite_from_id(id);
    return suite && !mbedtls_ssl_ciphersuite_uses_psk(suite);
}
static void *java_net_all_ciphers(void *env) {
    const int *ids = mbedtls_ssl_list_ciphersuites();
    size_t n = 0;
    for (size_t i = 0; ids[i]; ++i)
        if (java_net_supported_cipher(ids[i]))
            ++n;
    void *array = ssrnx_jni_new_local_reference_array(env, "[Ljava/lang/String;", (int32_t)n);
    for (size_t i = 0, j = 0; array && ids[i]; ++i) {
        if (!java_net_supported_cipher(ids[i]))
            continue;
        char name[128];
        java_net_cipher_name(ids[i], name, sizeof(name));
        void *s = text(env, name);
        if (s) {
            (void)ssrnx_jni_set_object_array_element_host(env, array, (int32_t)j++, s);
            ssrnx_jni_delete_local_ref_host(env, s);
        }
    }
    return array;
}
static java_net_Parameters *java_net_params_new(void *env, void **out_ref) {
    java_net_Parameters *p = NULL;
    void *ref = java_net_new_object(env, PARAM, sizeof(*p), N_PARAMS, (void **)&p);
    if (p) {
        void *a = java_net_all_ciphers(env);
        java_net_replace_ref(env, &p->ciphers, a);
        if (a)
            ssrnx_jni_delete_local_ref_host(env, a);
        const char *names[] = {"TLSv1.2"};
        a = strings(env, names, 1);
        java_net_replace_ref(env, &p->protocols, a);
        if (a)
            ssrnx_jni_delete_local_ref_host(env, a);
        a = strings(env, NULL, 0);
        java_net_replace_ref(env, &p->alpn, a);
        if (a)
            ssrnx_jni_delete_local_ref_host(env, a);
    }
    if (out_ref)
        *out_ref = ref;
    else if (ref)
        ssrnx_jni_delete_local_ref_host(env, ref);
    return p;
}
static void java_net_params_copy(void *env, java_net_Parameters *dst, const java_net_Parameters *src) {
    if (!dst || !src)
        return;
    java_net_replace_ref(env, &dst->ciphers, src->ciphers);
    java_net_replace_ref(env, &dst->protocols, src->protocols);
    java_net_replace_ref(env, &dst->alpn, src->alpn);
    free(dst->endpoint);
    dst->endpoint = src->endpoint ? strdup(src->endpoint) : NULL;
    dst->need_auth = src->need_auth;
    dst->want_auth = src->want_auth;
}
static bool java_net_port_valid(void *env, int port) {
    if (port >= 0 && port <= 65535)
        return true;
    fail(env, "java/lang/IllegalArgumentException", "Port out of range");
    return false;
}
static void java_net_address_port(Address *a) {
    if (a->address.ss_family == AF_INET)
        ((struct sockaddr_in *)&a->address)->sin_port = htons((uint16_t)a->port);
    else if (a->address.ss_family == AF_INET6)
        ((struct sockaddr_in6 *)&a->address)->sin6_port = htons((uint16_t)a->port);
}
static bool java_net_resolve_address(Address *a, const char *host, bool wildcard) {
    if (java_net_network_ready() != 0)
        return false;
    struct addrinfo hints = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM}, *info = NULL;
    if (wildcard)
        hints.ai_flags = AI_PASSIVE;
    int rc = getaddrinfo(host, wildcard && !host ? "0" : NULL, &hints, &info);
    if (rc || !info)
        return false;
    a->size = (socklen_t)info->ai_addrlen;
    memcpy(&a->address, info->ai_addr, info->ai_addrlen);
    freeaddrinfo(info);
    a->resolved = true;
    java_net_address_port(a);
    return true;
}
static const char *java_net_inet_class(const Address *a) { return a->address.ss_family == AF_INET6 ? IA6 : IA4; }
static void java_net_publish_address(void *ref, const Address *a) {
    if (a->address.ss_family == AF_INET &&
        ssrnx_jni_is_instance_of_class_host(ref, "java/net/Inet4Address")) {
        bool found = false;
        (void)ssrnx_jni_set_instance_field_value_for_class(
            ref, IA, "address", "I",
            java_net_ni((int32_t)ntohl(((const struct sockaddr_in *)&a->address)->sin_addr.s_addr)), &found);
        (void)ssrnx_jni_set_instance_field_value_for_class(ref, IA, "family", "I", java_net_ni(2), &found);
    }
}
static bool java_net_decode_address(void *env, void *ref, Address *out) {
    if (!ref)
        return false;
    Address *own = payload(ssrnx_jni_object_token_host(ref), N_ADDRESS);
    if (own) {
        *out = *own;
        return true;
    }
    SsrnxJniValue bytes = {0};
    if (!ssrnx_android_call_virtual(env, ref, "getAddress", "()[B", NULL, 0, &bytes) || !bytes.l)
        return false;
    int32_t size = ssrnx_jni_byte_array_length(bytes.l);
    unsigned char raw[16] = {0};
    bool ok =
        (size == 4 || size == 16) && ssrnx_jni_byte_array_read(bytes.l, 0, size, (int8_t *)raw) == SSRNX_OK;
    ssrnx_jni_delete_local_ref_host(env, bytes.l);
    if (ok) {
        out->resolved = true;
        out->host = NULL;
        if (size == 4) {
            struct sockaddr_in *v = (void *)&out->address;
            v->sin_family = AF_INET;
            memcpy(&v->sin_addr, raw, 4);
            out->size = sizeof(*v);
        } else {
            struct sockaddr_in6 *v = (void *)&out->address;
            v->sin6_family = AF_INET6;
            memcpy(&v->sin6_addr, raw, 16);
            out->size = sizeof(*v);
        }
    }
    return ok;
}
static void *java_net_address_copy(void *env, const char *cls, const Address *value) {
    Address *a = NULL;
    void *ref =
        java_net_new_object(env, !strcmp(cls, IA) ? java_net_inet_class(value) : cls, sizeof(*a), N_ADDRESS, (void **)&a);
    if (a) {
        java_net_NetObject object = a->object;
        *a = *value;
        a->object = object;
        a->host = value->host ? strdup(value->host) : NULL;
        java_net_publish_address(ref, a);
    }
    return ref;
}
static void *java_net_address_new(void *env, const char *cls, const char *host, int port, bool wildcard,
                         bool resolve) {
    Address value = {.host = (char *)host, .port = port};
    if (resolve)
        (void)java_net_resolve_address(&value, host ? host : (wildcard ? NULL : "localhost"), wildcard);
    return java_net_address_copy(env, cls, &value);
}
static SsrnxJniValue java_net_address_unresolved(void *env, void *token, const SsrnxJniValue *args, size_t n,
                                        void *user) {
    (void)token;
    (void)n;
    (void)user;
    if (!args[0].l) {
        fail(env, "java/lang/IllegalArgumentException", "hostname is null");
        return no(NULL);
    }
    if (!java_net_port_valid(env, args[1].i))
        return no(NULL);
    char *host = ssrnx_jni_string_utf_dup(args[0].l);
    void *ref = java_net_address_new(env, ISA, host, args[1].i, false, false);
    free(host);
    return no(ref);
}
static SsrnxJniValue java_net_address_ctor(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    Address *a = NULL;
    void *ref = java_net_new_object(env, ISA, sizeof(*a), N_ADDRESS, (void **)&a);
    if (!ref)
        return java_net_nv();
    ssrnx_jni_delete_local_ref_host(env, ref);
    (void)ssrnx_jni_set_token_host_data(token, a);
    int port = n == 1 ? args[0].i : args[1].i;
    if (!java_net_port_valid(env, port))
        return java_net_nv();
    a->port = port;
    if (n == 1) {
        (void)java_net_resolve_address(a, NULL, true);
        return java_net_nv();
    }
    Address decoded = {0};
    Address *source = user && java_net_decode_address(env, args[0].l, &decoded) ? &decoded
                      : args[0].l ? payload(ssrnx_jni_object_token_host(args[0].l), N_ADDRESS)
                                  : NULL;
    if (source) {
        a->address = source->address;
        a->size = source->size;
        a->resolved = source->resolved;
        a->host = source->host ? strdup(source->host) : NULL;
        java_net_address_port(a);
    } else if (args[0].l) {
        a->host = ssrnx_jni_string_utf_dup(args[0].l);
        if (a->host)
            (void)java_net_resolve_address(a, a->host, false);
    } else if (user) {
        (void)java_net_resolve_address(a, NULL, true);
    } else {
        fail(env, "java/lang/IllegalArgumentException", "hostname is null");
    }
    return java_net_nv();
}
static SsrnxJniValue java_net_inet_get(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    (void)token;
    if (java_net_network_ready() != 0) {
        fail(env, "java/net/UnknownHostException", "Network service unavailable");
        return no(NULL);
    }
    char *host = n && args[0].l ? ssrnx_jni_string_utf_dup(args[0].l) : NULL;
    struct addrinfo hints = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM}, *info = NULL;
    int rc = getaddrinfo(host && *host ? host : "127.0.0.1", NULL, &hints, &info);
    if (rc || !info) {
        fail(env, "java/net/UnknownHostException", host ? host : "localhost");
        free(host);
        return no(NULL);
    }
    int32_t count = 0;
    for (struct addrinfo *i = info; i; i = i->ai_next)
        if (i->ai_family == AF_INET || i->ai_family == AF_INET6)
            ++count;
    void *result = user ? ssrnx_jni_new_local_reference_array(env, "[Ljava/net/InetAddress;", count) : NULL;
    int32_t index = 0;
    for (struct addrinfo *i = info; i; i = i->ai_next) {
        if (i->ai_family != AF_INET && i->ai_family != AF_INET6)
            continue;
        Address value = {.host = host, .size = (socklen_t)i->ai_addrlen, .resolved = true};
        memcpy(&value.address, i->ai_addr, i->ai_addrlen);
        void *ref = java_net_address_copy(env, IA, &value);
        if (!user) {
            result = ref;
            break;
        }
        if (result)
            (void)ssrnx_jni_set_object_array_element_host(env, result, index++, ref);
        if (ref)
            ssrnx_jni_delete_local_ref_host(env, ref);
    }
    freeaddrinfo(info);
    free(host);
    return no(result);
}
enum { A_PORT, A_UNRESOLVED, A_HOST, A_NUMERIC, A_ADDRESS, A_LOOPBACK, A_BYTES, A_ANY, A_EQUALS, A_HASH };
static SsrnxJniValue java_net_address_get(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    (void)args;
    (void)n;
    Address *a = payload(token, N_ADDRESS);
    if (!a)
        return java_net_nv();
    switch ((intptr_t)user) {
    case A_PORT:
        return java_net_ni(a->port);
    case A_UNRESOLVED:
        return java_net_nb(!a->resolved);
    case A_HOST:
        if (a->host)
            return no(text(env, a->host));
        break;
    case A_ADDRESS:
        return no(a->resolved ? java_net_address_copy(env, IA, a) : NULL);
    case A_BYTES: {
        const unsigned char *raw = a->address.ss_family == AF_INET
                                       ? (const void *)&((struct sockaddr_in *)&a->address)->sin_addr
                                       : (const void *)&((struct sockaddr_in6 *)&a->address)->sin6_addr;
        int32_t len = a->address.ss_family == AF_INET ? 4 : 16;
        void *ref = ssrnx_jni_new_local_byte_array(env, len);
        if (ref)
            (void)ssrnx_jni_byte_array_write(ref, 0, len, (const int8_t *)raw);
        return no(ref);
    }
    case A_ANY:
        return java_net_nb(a->address.ss_family == AF_INET
                      ? ((struct sockaddr_in *)&a->address)->sin_addr.s_addr == 0
                      : IN6_IS_ADDR_UNSPECIFIED(&((struct sockaddr_in6 *)&a->address)->sin6_addr));
    case A_EQUALS: {
        Address other = {0};
        if (!args[0].l || !ssrnx_jni_is_instance_of_class_host(args[0].l, IA) ||
            !java_net_decode_address(env, args[0].l, &other) || a->address.ss_family != other.address.ss_family)
            return java_net_nb(false);
        if (a->address.ss_family == AF_INET)
            return java_net_nb(((struct sockaddr_in *)&a->address)->sin_addr.s_addr ==
                      ((struct sockaddr_in *)&other.address)->sin_addr.s_addr);
        return java_net_nb(!memcmp(&((struct sockaddr_in6 *)&a->address)->sin6_addr,
                          &((struct sockaddr_in6 *)&other.address)->sin6_addr, 16));
    }
    case A_HASH:
        if (a->address.ss_family == AF_INET)
            return java_net_ni((int32_t)ntohl(((struct sockaddr_in *)&a->address)->sin_addr.s_addr));
        else {
            const int8_t *raw = (void *)&((struct sockaddr_in6 *)&a->address)->sin6_addr;
            uint32_t hash = 0;
            for (int i = 0; i < 16; i += 4) {
                uint32_t part = 0;
                for (int j = 0; j < 4; ++j)
                    part = (part << 8) + (int32_t)raw[i + j];
                hash += part;
            }
            return java_net_ni((int32_t)hash);
        }
    case A_LOOPBACK:
        if (a->address.ss_family == AF_INET)
            return java_net_nb((ntohl(((struct sockaddr_in *)&a->address)->sin_addr.s_addr) >> 24) == 127);
        return java_net_nb(a->address.ss_family == AF_INET6 &&
                  IN6_IS_ADDR_LOOPBACK(&((struct sockaddr_in6 *)&a->address)->sin6_addr));
    default:
        break;
    }
    char buf[INET6_ADDRSTRLEN] = {0};
    if (a->resolved) {
        const void *raw = a->address.ss_family == AF_INET
                              ? (void *)&((struct sockaddr_in *)&a->address)->sin_addr
                              : (void *)&((struct sockaddr_in6 *)&a->address)->sin6_addr;
        (void)inet_ntop(a->address.ss_family, raw, buf, sizeof(buf));
    }
    return no(text(env, *buf ? buf : (a->host ? a->host : "")));
}
static java_net_Transport *java_net_transport_new(void *env) {
    java_net_Transport *p = calloc(1, sizeof(*p));
    if (!p) {
        java_net_oom(env);
        return NULL;
    }
    atomic_init(&p->fd, -1);
    atomic_init(&p->closed, false);
    pthread_mutex_init(&p->io, NULL);
    pthread_mutex_init(&p->read, NULL);
    pthread_mutex_init(&p->write, NULL);
    pthread_mutex_lock(&java_net_net_lock);
    p->next = java_net_transports;
    java_net_transports = p;
    pthread_mutex_unlock(&java_net_net_lock);
    return p;
}
static void *java_net_socket_new(void *env, java_net_TlsMaterial *material) {
    java_net_SocketObject *s = NULL;
    void *ref = java_net_new_object(env, material ? TLS_SOCK : SOCK, sizeof(*s), N_SOCKET, (void **)&s);
    if (s) {
        atomic_init(&s->closed, false);
        atomic_init(&s->handshake, false);
        atomic_init(&s->timeout, 0);
        atomic_init(&s->connected, false);
        atomic_init(&s->bound, false);
        atomic_init(&s->input_shutdown, false);
        atomic_init(&s->output_shutdown, false);
        atomic_init(&s->nodelay, false);
        atomic_init(&s->keepalive, false);
        atomic_init(&s->reuse, false);
        s->transport = java_net_transport_new(env);
        s->tls = material != NULL;
        s->material = material;
        s->auto_close = true;
        mbedtls_ssl_init(&s->ssl);
        mbedtls_ssl_session_init(&s->session);
        mbedtls_ssl_config_init(&s->config);
        mbedtls_ctr_drbg_init(&s->rng);
        if (material)
            s->parameters = java_net_params_new(env, NULL);
    }
    return ref;
}
static bool java_net_socket_open(void *env, java_net_SocketObject *s) {
    if (!s || !s->transport || atomic_load(&s->closed) || atomic_load(&s->transport->closed)) {
        fail(env, "java/net/SocketException", "Socket is closed");
        return false;
    }
    return true;
}
static int java_net_await_fd(java_net_SocketObject *s, bool write, int64_t deadline) {
    while (!atomic_load(&s->closed) && !atomic_load(&s->transport->closed)) {
        if (ssrnx_jni_process_exit_requested(NULL, 0))
            return -1;
        int64_t remain = deadline ? deadline - java_net_millis() : 50;
        if (deadline && remain <= 0)
            return 0;
        if (remain > 50)
            remain = 50;
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(s->transport->fd, &fds);
        struct timeval timeout = {.tv_sec = 0, .tv_usec = (suseconds_t)(remain * 1000)};
        int rc = select(s->transport->fd + 1, write ? NULL : &fds, write ? &fds : NULL, NULL, &timeout);
        if (rc > 0)
            return 1;
        if (rc < 0 && errno != EINTR)
            return -1;
    }
    return -1;
}
static bool java_net_socket_connect(void *env, java_net_SocketObject *s, Address *a, int timeout) {
    if (java_net_network_ready() != 0) {
        fail(env, "java/net/SocketException", "Network service unavailable");
        return false;
    }
    if (!java_net_socket_open(env, s))
        return false;
    if (s->connected) {
        fail(env, "java/net/SocketException", "Already connected");
        return false;
    }
    if (timeout < 0) {
        fail(env, "java/lang/IllegalArgumentException", "Negative timeout");
        return false;
    }
    if (!a || !a->resolved) {
        fail(env, "java/net/UnknownHostException", a && a->host ? a->host : "Unresolved address");
        return false;
    }
    java_net_Transport *t = s->transport;
    pthread_mutex_lock(&t->io);
    if (!java_net_socket_open(env, s)) {
        pthread_mutex_unlock(&t->io);
        return false;
    }
    if (t->fd < 0)
        t->fd = socket(a->address.ss_family, SOCK_STREAM, 0);
    bool ok = t->fd >= 0;
    if (ok && !java_net_socket_open(env, s))
        ok = false;
    if (ok) {
        int flags = fcntl(t->fd, F_GETFL, 0);
        ok = flags >= 0 && fcntl(t->fd, F_SETFL, flags | O_NONBLOCK) == 0;
    }
    if (ok) {
        int rc = connect(t->fd, (struct sockaddr *)&a->address, a->size);
        if (rc < 0 && errno == EINPROGRESS) {
            rc = java_net_await_fd(s, true, timeout ? java_net_millis() + timeout : 0);
            if (rc == 0) {
                fail(env, "java/net/SocketTimeoutException", "Connect timed out");
                ok = false;
            } else if (rc < 0)
                ok = false;
            else {
                int error = 0;
                socklen_t size = sizeof(error);
                ok = getsockopt(t->fd, SOL_SOCKET, SO_ERROR, &error, &size) == 0 && error == 0;
                if (error)
                    errno = error;
            }
        } else
            ok = rc == 0;
    }
    if (ok && !java_net_socket_open(env, s))
        ok = false;
    if (ok) {
        s->peer_host = a->host ? strdup(a->host) : NULL;
        s->peer_port = a->port;
        s->peer_address = a->address;
        s->peer_size = a->size;
        s->local_size = sizeof(s->local_address);
        (void)getsockname(t->fd, (struct sockaddr *)&s->local_address, &s->local_size);
        int value = s->nodelay;
        (void)setsockopt(t->fd, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
        value = s->keepalive;
        (void)setsockopt(t->fd, SOL_SOCKET, SO_KEEPALIVE, &value, sizeof(value));
        s->bound = true;
        s->connected = true;
    } else {
        if (!ssrnx_jni_exception_pending_host(env))
            fail(env, "java/net/ConnectException", strerror(errno));
        if (t->fd >= 0) {
            close(t->fd);
            t->fd = -1;
        }
    }
    pthread_mutex_unlock(&t->io);
    return ok;
}
static int java_net_entropy(void *unused, unsigned char *out, size_t size) {
    (void)unused;
#ifdef __SWITCH__
    randomGet(out, size);
    return 0;
#else
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f)
        return MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED;
    bool ok = fread(out, 1, size, f) == size;
    fclose(f);
    return ok ? 0 : MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED;
#endif
}
static int java_net_tls_send(void *arg, const unsigned char *data, size_t size) {
    java_net_SocketObject *s = arg;
    ssize_t n = send(s->transport->fd, data, size, MSG_NOSIGNAL);
    if (n >= 0)
        return (int)n;
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    return MBEDTLS_ERR_NET_SEND_FAILED;
}
static int java_net_tls_recv(void *arg, unsigned char *data, size_t size) {
    java_net_SocketObject *s = arg;
    ssize_t n = recv(s->transport->fd, data, size, 0);
    if (n >= 0)
        return (int)n;
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        return MBEDTLS_ERR_SSL_WANT_READ;
    return MBEDTLS_ERR_NET_RECV_FAILED;
}
static bool java_net_tls_error(void *env, int rc) {
    char message[192];
    mbedtls_strerror(rc, message, sizeof(message));
    fail(env, "javax/net/ssl/SSLHandshakeException", message);
    return false;
}
static int java_net_verify_peer(void *arg, mbedtls_x509_crt *cert, int depth, uint32_t *flags) {
    java_net_SocketObject *s = arg;
    if (!s->parameters->endpoint || !*s->parameters->endpoint) {
        *flags &= ~MBEDTLS_X509_BADCERT_CN_MISMATCH;
    } else if (depth == 0 && s->peer_host) {
        ssrnx_tls_verify_ip_san(cert,s->peer_host,flags);
    }
    return 0;
}
static bool handshake(void *env, java_net_SocketObject *s) {
    if (s->handshake)
        return true;
    if (s->tls_failed) {
        fail(env, "javax/net/ssl/SSLHandshakeException", "Previous TLS handshake failed");
        return false;
    }
    if (s->parameters->endpoint && *s->parameters->endpoint && strcasecmp(s->parameters->endpoint, "HTTPS")) {
        fail(env, "javax/net/ssl/SSLHandshakeException", "Unsupported endpoint identification algorithm");
        return false;
    }
    if (!s->connected) {
        fail(env, "java/net/SocketException", "Socket is not connected");
        return false;
    }
    s->tls_failed = true;
    int rc = mbedtls_ctr_drbg_seed(&s->rng, java_net_entropy, NULL, NULL, 0);
    if (!rc)
        rc = mbedtls_ssl_config_defaults(&s->config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                         MBEDTLS_SSL_PRESET_DEFAULT);
    if (rc)
        return java_net_tls_error(env, rc);
    mbedtls_ssl_conf_rng(&s->config, mbedtls_ctr_drbg_random, &s->rng);
    mbedtls_ssl_conf_authmode(&s->config, MBEDTLS_SSL_VERIFY_REQUIRED);
#if defined(MBEDTLS_SSL_RENEGOTIATION)
    mbedtls_ssl_conf_renegotiation(&s->config, MBEDTLS_SSL_RENEGOTIATION_DISABLED);
#endif
    mbedtls_ssl_conf_verify(&s->config, java_net_verify_peer, s);
    mbedtls_ssl_conf_ca_chain(&s->config, &s->material->roots, NULL);
    mbedtls_ssl_conf_min_version(&s->config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_max_version(&s->config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    if (ssrnx_jni_array_length_host(s->parameters->protocols) == 0) {
        fail(env, "javax/net/ssl/SSLHandshakeException", "No enabled protocols");
        return false;
    }
    char **names = java_net_array_text(env, s->parameters->ciphers);
    if (!names)
        return false;
    size_t count = 0;
    while (names[count])
        ++count;
    s->cipher_ids = calloc(count + 1, sizeof(*s->cipher_ids));
    if (!s->cipher_ids) {
        java_net_free_texts(names);
        java_net_oom(env);
        return false;
    }
    const int *ids = mbedtls_ssl_list_ciphersuites();
    for (size_t i = 0; i < count; ++i)
        for (size_t j = 0; ids[j]; ++j) {
            char name[128];
            java_net_cipher_name(ids[j], name, sizeof(name));
            if (!strcmp(names[i], name)) {
                s->cipher_ids[i] = ids[j];
                break;
            }
        }
    java_net_free_texts(names);
    mbedtls_ssl_conf_ciphersuites(&s->config, s->cipher_ids);
    s->alpn_names = java_net_array_text(env, s->parameters->alpn);
    if (!s->alpn_names)
        return false;
#if defined(MBEDTLS_SSL_ALPN)
    if (s->alpn_names[0]) {
        rc = mbedtls_ssl_conf_alpn_protocols(&s->config, (const char **)s->alpn_names);
        if (rc)
            return java_net_tls_error(env, rc);
    }
#endif
    rc = mbedtls_ssl_setup(&s->ssl, &s->config);
    if (rc)
        return java_net_tls_error(env, rc);
    /* SNI and name verification are separate JSSE settings. OkHttp verifies
     * peer names itself unless an HTTPS endpoint algorithm was requested. */
    if (s->parameters->endpoint && *s->parameters->endpoint) {
        rc = mbedtls_ssl_set_hostname(&s->ssl, s->peer_host ? s->peer_host : "localhost");
        if (rc)
            return java_net_tls_error(env, rc);
    } else if (s->peer_host) {
        rc = mbedtls_ssl_set_hostname(&s->ssl, s->peer_host);
        if (rc)
            return java_net_tls_error(env, rc);
    }
    mbedtls_ssl_set_bio(&s->ssl, s, java_net_tls_send, java_net_tls_recv, NULL);
    int timeout = atomic_load(&s->timeout);
    int64_t deadline = timeout ? java_net_millis() + timeout : 0;
    for (;;) {
        /* The installed backend has no internal threading support. Serialize
         * each nonblocking handshake step that can use shared CA key caches. */
        pthread_mutex_lock(&s->material->verify);
        rc = mbedtls_ssl_handshake(&s->ssl);
        pthread_mutex_unlock(&s->material->verify);
        if (!rc)
            break;
        if (rc != MBEDTLS_ERR_SSL_WANT_READ && rc != MBEDTLS_ERR_SSL_WANT_WRITE)
            return java_net_tls_error(env, rc);
        int ready = java_net_await_fd(s, rc == MBEDTLS_ERR_SSL_WANT_WRITE, deadline);
        if (ready <= 0) {
            fail(env, ready == 0 ? "java/net/SocketTimeoutException" : "java/net/SocketException",
                 ready == 0 ? "TLS handshake timed out" : "Socket is closed");
            return false;
        }
    }
    rc = mbedtls_ssl_get_session(&s->ssl, &s->session);
    if (rc)
        return java_net_tls_error(env, rc);
    /* Mbed TLS's session copy retains only the leaf; JSSE exposes the entire
     * peer-supplied certificate chain, also after the socket is closed. */
    const mbedtls_x509_crt *peer = mbedtls_ssl_get_peer_cert(&s->ssl);
    for (const mbedtls_x509_crt *p = peer ? peer->next : NULL; p && p->raw.len; p = p->next) {
        rc = mbedtls_x509_crt_parse_der(s->session.peer_cert, p->raw.p, p->raw.len);
        if (rc)
            return java_net_tls_error(env, rc);
    }
    s->tls_failed = false;
    s->handshake = true;
    SSRNX_LOGCI("NET", "TLS handshake protocol=TLSv1.2 chainVerified=1");
    return true;
}
static java_net_TlsMaterial *java_net_material_new(void *env, void *managers) {
    java_net_TlsMaterial *m = calloc(1, sizeof(*m));
    if (!m) {
        java_net_oom(env);
        return NULL;
    }
    mbedtls_x509_crt_init(&m->roots);
    pthread_mutex_init(&m->verify, NULL);
    void *manager = NULL;
    if (managers) {
        int32_t n = ssrnx_jni_array_length_host(managers);
        for (int32_t i = 0; i < n && !manager; ++i) {
            void *candidate = element(env, managers, i);
            if (candidate && ssrnx_jni_is_instance_of_class_host(candidate, "javax/net/ssl/X509TrustManager"))
                manager = candidate;
            else if (candidate)
                ssrnx_jni_delete_local_ref_host(env, candidate);
        }
    }
    if (managers && !manager) {
        fail(env, "java/security/KeyManagementException", "No X509 trust manager supplied");
        goto bad;
    }
    if (manager) {
        char cls[256] = {0};
        (void)ssrnx_jni_object_class_name_host(manager, cls, sizeof(cls));
        if (strcmp(cls, "javax/net/ssl/X509TrustManager")) {
            ssrnx_jni_delete_local_ref_host(env, manager);
            fail(env, "java/security/KeyManagementException",
                 "Custom TrustManager callbacks are not supported");
            goto bad;
        }
    }
    if (!manager) {
        SsrnxJniValue algorithm = no(text(env, "PKIX")), factory = {0}, out = {0};
        bool found = false;
        (void)ssrnx_jni_invoke_method_host(env, NULL, "javax/net/ssl/TrustManagerFactory", "getInstance",
                                           "(Ljava/lang/String;)Ljavax/net/ssl/TrustManagerFactory;", true,
                                           &algorithm, &factory, &found);
        if (algorithm.l)
            ssrnx_jni_delete_local_ref_host(env, algorithm.l);
        SsrnxJniValue nil = {0};
        if (factory.l &&
            ssrnx_android_call_virtual(env, factory.l, "init", "(Ljava/security/KeyStore;)V", &nil, 1,
                                       &out) &&
            ssrnx_android_call_virtual(env, factory.l, "getTrustManagers", "()[Ljavax/net/ssl/TrustManager;",
                                       NULL, 0, &out) &&
            out.l) {
            manager = element(env, out.l, 0);
            ssrnx_jni_delete_local_ref_host(env, out.l);
        }
        if (factory.l)
            ssrnx_jni_delete_local_ref_host(env, factory.l);
    }
    if (!manager)
        goto bad;
    m->platform_roots = ssrnx_jni_get_object_host_data(manager) == (void *)(uintptr_t)1u;
    SsrnxJniValue issuers = {0};
    bool ok = ssrnx_android_call_virtual(env, manager, "getAcceptedIssuers",
                                         "()[Ljava/security/cert/X509Certificate;", NULL, 0, &issuers);
    ssrnx_jni_delete_local_ref_host(env, manager);
    if (!ok || !issuers.l)
        goto bad;
    int32_t n = ssrnx_jni_array_length_host(issuers.l);
    for (int32_t i = 0; i < n; ++i) {
        void *cert = element(env, issuers.l, i);
        SsrnxJniValue der = {0};
        if (cert && ssrnx_android_call_virtual(env, cert, "getEncoded", "()[B", NULL, 0, &der) && der.l) {
            int32_t size = ssrnx_jni_byte_array_length(der.l);
            unsigned char *bytes = size > 0 ? malloc((size_t)size) : NULL;
            if (bytes && ssrnx_jni_byte_array_read(der.l, 0, size, (int8_t *)bytes) == SSRNX_OK)
                (void)mbedtls_x509_crt_parse_der(&m->roots, bytes, (size_t)size);
            free(bytes);
            ssrnx_jni_delete_local_ref_host(env, der.l);
        }
        if (cert)
            ssrnx_jni_delete_local_ref_host(env, cert);
    }
    ssrnx_jni_delete_local_ref_host(env, issuers.l);
    if (ssrnx_jni_exception_pending_host(env))
        goto bad;
    pthread_mutex_lock(&java_net_net_lock);
    m->next = java_net_materials;
    java_net_materials = m;
    pthread_mutex_unlock(&java_net_net_lock);
    return m;
bad:
    mbedtls_x509_crt_free(&m->roots);
    pthread_mutex_destroy(&m->verify);
    free(m);
    if (!ssrnx_jni_exception_pending_host(env))
        fail(env, "java/security/KeyManagementException", "Cannot initialize trust managers");
    return NULL;
}

static void java_net_close_socket(java_net_SocketObject *s) {
    if (!s || atomic_exchange(&s->closed, true))
        return;
    java_net_Transport *t = s->transport;
    if (!t)
        return;
    bool owns_transport = s->auto_close && !atomic_exchange(&t->closed, true);
    /* Wake bounded waits first; retire their descriptor only after both users leave. */
    pthread_mutex_lock(&t->read);
    pthread_mutex_lock(&t->write);
    pthread_mutex_lock(&t->io);
    if (s->tls && s->handshake && t->fd >= 0)
        (void)mbedtls_ssl_close_notify(&s->ssl);
    if (owns_transport) {
        if (t->fd >= 0) {
            (void)shutdown(t->fd, SHUT_RDWR);
            close(t->fd);
            t->fd = -1;
        }
    }
    pthread_mutex_unlock(&t->io);
    pthread_mutex_unlock(&t->write);
    pthread_mutex_unlock(&t->read);
}
static SsrnxJniValue java_net_socket_close(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    (void)env;
    (void)args;
    (void)n;
    (void)user;
    java_net_close_socket(payload(token, N_SOCKET));
    return java_net_nv();
}
static bool java_net_bind_socket(void *env, java_net_SocketObject *s, Address *a) {
    if (java_net_network_ready() != 0) {
        fail(env, "java/net/SocketException", "Network service unavailable");
        return false;
    }
    if (!java_net_socket_open(env, s))
        return false;
    if (s->bound) {
        fail(env, "java/net/SocketException", "Already bound");
        return false;
    }
    if (!a || !a->resolved) {
        fail(env, "java/net/SocketException", "Unresolved bind address");
        return false;
    }
    java_net_Transport *t = s->transport;
    pthread_mutex_lock(&t->io);
    if (!java_net_socket_open(env, s)) {
        pthread_mutex_unlock(&t->io);
        return false;
    }
    if (t->fd < 0)
        t->fd = socket(a->address.ss_family, SOCK_STREAM, 0);
    int reuse = s->reuse;
    if (t->fd >= 0)
        (void)setsockopt(t->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (t->fd < 0 || bind(t->fd, (struct sockaddr *)&a->address, a->size) != 0) {
        fail(env, "java/net/BindException", strerror(errno));
        pthread_mutex_unlock(&t->io);
        return false;
    }
    s->local_size = sizeof(s->local_address);
    (void)getsockname(t->fd, (struct sockaddr *)&s->local_address, &s->local_size);
    s->bound = true;
    pthread_mutex_unlock(&t->io);
    return true;
}
static void java_net_connect_args(void *env, java_net_SocketObject *s, const SsrnxJniValue *args, size_t n, bool inet) {
    if (n == 0)
        return;
    if (!java_net_port_valid(env, args[1].i) || (n == 4 && !java_net_port_valid(env, args[3].i)))
        return;
    Address decoded_remote = {0};
    Address *remote = inet && java_net_decode_address(env, args[0].l, &decoded_remote) ? &decoded_remote : NULL;
    Address target = {.port = args[1].i};
    char *host = NULL;
    if (inet) {
        if (!remote) {
            fail(env, "java/lang/NullPointerException", "address");
            return;
        }
        target = *remote;
        target.port = args[1].i;
        java_net_address_port(&target);
    } else {
        host = args[0].l ? ssrnx_jni_string_utf_dup(args[0].l) : strdup("localhost");
        target.host = host;
        if (!java_net_resolve_address(&target, host, false)) {
            fail(env, "java/net/UnknownHostException", host ? host : "hostname");
            free(host);
            return;
        }
    }
    if (n == 4) {
        Address decoded_local = {0};
        Address *local = java_net_decode_address(env, args[2].l, &decoded_local) ? &decoded_local : NULL;
        Address binding = {.port = args[3].i};
        if (local) {
            binding = *local;
            binding.port = args[3].i;
            java_net_address_port(&binding);
        } else {
            binding.address.ss_family = target.address.ss_family;
            binding.size = target.size;
            binding.resolved = true;
            java_net_address_port(&binding);
        }
        if (!java_net_bind_socket(env, s, &binding)) {
            free(host);
            return;
        }
    }
    (void)java_net_socket_connect(env, s, &target, 0);
    free(host);
}
static SsrnxJniValue java_net_socket_ctor(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    void *ref = java_net_socket_new(env, NULL);
    if (!ref)
        return java_net_nv();
    java_net_SocketObject *s = payload(ssrnx_jni_object_token_host(ref), N_SOCKET);
    (void)ssrnx_jni_set_token_host_data(token, s);
    ssrnx_jni_delete_local_ref_host(env, ref);
    java_net_connect_args(env, s, args, n, user != NULL);
    return java_net_nv();
}
static SsrnxJniValue java_net_socket_connect_call(void *env, void *token, const SsrnxJniValue *args, size_t n,
                                         void *user) {
    (void)user;
    java_net_SocketObject *s = payload(token, N_SOCKET);
    Address *a = n && args[0].l ? payload(ssrnx_jni_object_token_host(args[0].l), N_ADDRESS) : NULL;
    if (!a) {
        fail(env, "java/lang/IllegalArgumentException", "Unsupported/null SocketAddress");
        return java_net_nv();
    }
    (void)java_net_socket_connect(env, s, a, n == 2 ? args[1].i : 0);
    return java_net_nv();
}
static SsrnxJniValue java_net_socket_bind(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    (void)n;
    (void)user;
    java_net_SocketObject *s = payload(token, N_SOCKET);
    Address *a = args[0].l ? payload(ssrnx_jni_object_token_host(args[0].l), N_ADDRESS) : NULL;
    Address wildcard = {.port = 0};
    if (!a) {
        (void)java_net_resolve_address(&wildcard, NULL, true);
        a = &wildcard;
    }
    (void)java_net_bind_socket(env, s, a);
    return java_net_nv();
}
enum {
    S_CONNECTED,
    S_BOUND,
    S_CLOSED,
    S_TIMEOUT,
    S_NODELAY,
    S_KEEPALIVE,
    S_REUSE,
    S_IN_SHUT,
    S_OUT_SHUT,
    S_PEER_PORT,
    S_LOCAL_PORT,
    S_LOCAL_ADDR,
    S_LOCAL_SOCKET_ADDR,
    S_REMOTE_ADDR,
    S_PEER_ADDR,
    S_CHANNEL
};
static SsrnxJniValue java_net_socket_state(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    java_net_SocketObject *s = payload(token, N_SOCKET);
    int which = (int)(intptr_t)user;
    if (!s)
        return java_net_nv();
    if ((which == S_TIMEOUT || which == S_NODELAY || which == S_KEEPALIVE || which == S_REUSE) &&
        !java_net_socket_open(env, s))
        return java_net_nv();
    if (n) {
        if (!java_net_socket_open(env, s))
            return java_net_nv();
        if (which == S_TIMEOUT) {
            if (args[0].i < 0)
                fail(env, "java/lang/IllegalArgumentException", "Negative timeout");
            else
                atomic_store(&s->timeout, args[0].i);
            return java_net_nv();
        }
        bool value = args[0].z;
        int flag = value, level = SOL_SOCKET, option = 0;
        switch (which) {
        case S_NODELAY:
            s->nodelay = value;
            level = IPPROTO_TCP;
            option = TCP_NODELAY;
            break;
        case S_KEEPALIVE:
            s->keepalive = value;
            option = SO_KEEPALIVE;
            break;
        case S_REUSE:
            s->reuse = value;
            option = SO_REUSEADDR;
            break;
        default:
            break;
        }
        if (s->transport->fd >= 0 && option &&
            setsockopt(s->transport->fd, level, option, &flag, sizeof(flag)) != 0)
            fail(env, "java/net/SocketException", strerror(errno));
        return java_net_nv();
    }
    switch (which) {
    case S_CONNECTED:
        return java_net_nb(s->connected);
    case S_BOUND:
        return java_net_nb(s->bound);
    case S_CLOSED:
        return java_net_nb(atomic_load(&s->closed) || atomic_load(&s->transport->closed));
    case S_TIMEOUT:
        if (!java_net_socket_open(env, s))
            return java_net_nv();
        return java_net_ni(atomic_load(&s->timeout));
    case S_NODELAY:
        return java_net_nb(s->nodelay);
    case S_KEEPALIVE:
        return java_net_nb(s->keepalive);
    case S_REUSE:
        return java_net_nb(s->reuse);
    case S_IN_SHUT:
        return java_net_nb(s->input_shutdown);
    case S_OUT_SHUT:
        return java_net_nb(s->output_shutdown);
    case S_PEER_PORT:
        return java_net_ni(s->connected ? s->peer_port : 0);
    case S_CHANNEL:
        return no(NULL);
    default:
        break;
    }
    Address *a = NULL;
    Address snapshot = {.address = (which == S_PEER_ADDR ? s->peer_address : s->local_address)};
    const char *cls = which == S_PEER_ADDR || which == S_LOCAL_ADDR ? java_net_inet_class(&snapshot) : ISA;
    void *ref = java_net_new_object(env, cls, sizeof(*a), N_ADDRESS, (void **)&a);
    if (!a)
        return no(NULL);
    bool peer = which == S_REMOTE_ADDR || which == S_PEER_ADDR;
    socklen_t size = peer ? s->peer_size : s->local_size;
    int rc = size ? 0 : -1;
    a->address = peer ? s->peer_address : s->local_address;
    if (rc) {
        ssrnx_jni_delete_local_ref_host(env, ref);
        return which == S_LOCAL_PORT ? java_net_ni(-1) : no(NULL);
    }
    a->size = size;
    a->resolved = true;
    a->port = a->address.ss_family == AF_INET ? ntohs(((struct sockaddr_in *)&a->address)->sin_port)
                                              : ntohs(((struct sockaddr_in6 *)&a->address)->sin6_port);
    a->host = peer && s->peer_host ? strdup(s->peer_host) : NULL;
    java_net_publish_address(ref, a);
    if (which == S_LOCAL_PORT) {
        int port = a->port;
        ssrnx_jni_delete_local_ref_host(env, ref);
        return java_net_ni(port);
    }
    return no(ref);
}
static SsrnxJniValue java_net_socket_shutdown(void *env, void *token, const SsrnxJniValue *args, size_t n,
                                     void *user) {
    (void)args;
    (void)n;
    java_net_SocketObject *s = payload(token, N_SOCKET);
    if (!java_net_socket_open(env, s))
        return java_net_nv();
    if (!s->connected) {
        fail(env, "java/net/SocketException", "Socket is not connected");
        return java_net_nv();
    }
    pthread_mutex_lock(&s->transport->io);
    if (!java_net_socket_open(env, s)) { pthread_mutex_unlock(&s->transport->io); return java_net_nv(); }
    if (user) {
        s->output_shutdown = true;
        (void)shutdown(s->transport->fd, SHUT_WR);
    } else {
        s->input_shutdown = true;
        (void)shutdown(s->transport->fd, SHUT_RD);
    }
    pthread_mutex_unlock(&s->transport->io);
    return java_net_nv();
}
static SsrnxJniValue java_net_socket_stream(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    (void)args;
    (void)n;
    java_net_SocketObject *s = payload(token, N_SOCKET);
    if (!java_net_socket_open(env, s))
        return no(NULL);
    if (!s->connected) {
        fail(env, "java/net/SocketException", "Socket is not connected");
        return no(NULL);
    }
    if (user ? s->output_shutdown : s->input_shutdown) {
        fail(env, "java/net/SocketException", "Socket stream is shut down");
        return no(NULL);
    }
    void *stream = ssrnx_jni_new_local_object(env, user ? OUT : IN);
    if (stream)
        (void)ssrnx_jni_set_object_host_data(stream, s);
    return no(stream);
}
static SsrnxJniValue java_net_stream_io(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    java_net_SocketObject *s = payload(token, N_SOCKET);
    bool writing = user != NULL;
    if (!java_net_socket_open(env, s))
        return writing ? java_net_nv() : java_net_ni(-1);
    int32_t len = 1, offset = 0;
    void *array = NULL;
    if (n && !(writing && n == 1 && !ssrnx_jni_is_instance_of_class_host(args[0].l, "[B"))) {
        array = args[0].l;
        if (!array) {
            fail(env, "java/lang/NullPointerException", "buffer");
            return java_net_nv();
        }
        int32_t total = ssrnx_jni_byte_array_length(array);
        offset = n == 3 ? args[1].i : 0;
        len = n == 3 ? args[2].i : total;
        if (total < 0 || offset < 0 || len < 0 || offset > total || len > total - offset) {
            fail(env, "java/lang/IndexOutOfBoundsException", "Socket stream range");
            return java_net_nv();
        }
    }
    if (!len)
        return writing ? java_net_nv() : java_net_ni(0);
    if (writing && s->output_shutdown) {
        fail(env, "java/net/SocketException", "Socket output is shut down");
        return java_net_nv();
    }
    if (!writing && s->input_shutdown)
        return java_net_ni(-1);
    unsigned char *bytes = malloc((size_t)len);
    if (!bytes) {
        java_net_oom(env);
        return java_net_nv();
    }
    if (writing) {
        if (array)
            (void)ssrnx_jni_byte_array_read(array, offset, len, (int8_t *)bytes);
        else
            bytes[0] = (uint8_t)args[0].i;
    }
    pthread_mutex_t *direction = writing ? &s->transport->write : &s->transport->read;
    pthread_mutex_lock(direction);
    int rc = -1;
    size_t used = 0;
    int timeout = writing ? 0 : atomic_load(&s->timeout);
    int64_t deadline = timeout ? java_net_millis() + timeout : 0;
    pthread_mutex_lock(&s->transport->io);
    const bool ready = java_net_socket_open(env, s) && (!s->tls || handshake(env, s));
    pthread_mutex_unlock(&s->transport->io);
    if (!ready)
        goto done;
    do {
        /* Mbed TLS has shared record state, but its nonblocking step is short.
         * Neither direction holds that lock while awaiting network readiness. */
        pthread_mutex_lock(&s->transport->io);
        if (!java_net_socket_open(env, s)) {
            pthread_mutex_unlock(&s->transport->io);
            break;
        }
        if (s->tls)
            rc = writing ? mbedtls_ssl_write(&s->ssl, bytes + used, (size_t)len - used)
                         : mbedtls_ssl_read(&s->ssl, bytes, (size_t)len);
        else {
            ssize_t amount = writing ? send(s->transport->fd, bytes + used, (size_t)len - used, MSG_NOSIGNAL)
                                     : recv(s->transport->fd, bytes, (size_t)len, 0);
            rc = (int)amount;
            if (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
                rc = writing ? MBEDTLS_ERR_SSL_WANT_WRITE : MBEDTLS_ERR_SSL_WANT_READ;
        }
        const int saved_errno = errno;
        pthread_mutex_unlock(&s->transport->io);
        if (rc > 0) {
            used += (size_t)rc;
            if (!writing)
                break;
            continue;
        }
        if (atomic_load(&s->closed) || atomic_load(&s->transport->closed)) {
            fail(env, "java/net/SocketException", "Socket is closed");
            break;
        }
        if (!writing && (rc == 0 || rc == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)) {
            rc = 0;
            break;
        }
        if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
            int ready = java_net_await_fd(s, rc == MBEDTLS_ERR_SSL_WANT_WRITE, deadline);
            if (ready > 0)
                continue;
            fail(env, ready == 0 ? "java/net/SocketTimeoutException" : "java/net/SocketException",
                 ready == 0 ? "Read timed out" : "Socket is closed");
            break;
        }
        fail(env, s->tls ? "javax/net/ssl/SSLException" : "java/net/SocketException",
             s->tls ? "TLS record I/O failed" : strerror(saved_errno));
        break;
    } while (used < (size_t)len);
done:
    pthread_mutex_unlock(direction);
    if (!writing && used) {
        if (array)
            (void)ssrnx_jni_byte_array_write(array, offset, (int32_t)used, (const int8_t *)bytes);
        else
            rc = bytes[0];
    }
    free(bytes);
    return writing ? java_net_nv() : java_net_ni(used ? (array ? (int32_t)used : rc) : -1);
}
static SsrnxJniValue java_net_stream_available(void *env, void *token, const SsrnxJniValue *args, size_t n,
                                      void *user) {
    (void)args;
    (void)n;
    (void)user;
    java_net_SocketObject *s = payload(token, N_SOCKET);
    if (!java_net_socket_open(env, s))
        return java_net_ni(0);
    if (s->input_shutdown)
        return java_net_ni(0);
    pthread_mutex_lock(&s->transport->io);
    if (!java_net_socket_open(env, s)) { pthread_mutex_unlock(&s->transport->io); return java_net_ni(0); }
    if (s->tls) {
        const int32_t available = (int32_t)mbedtls_ssl_get_bytes_avail(&s->ssl);
        pthread_mutex_unlock(&s->transport->io);
        return java_net_ni(available);
    }
    unsigned char b[4096];
    ssize_t got = recv(s->transport->fd, b, sizeof(b), MSG_PEEK);
    pthread_mutex_unlock(&s->transport->io);
    return java_net_ni(got > 0 ? (int32_t)got : 0);
}
static SsrnxJniValue nothing(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    (void)env;
    (void)token;
    (void)args;
    (void)n;
    (void)user;
    return java_net_nv();
}
static SsrnxJniValue java_net_context_get_instance(void *env, void *token, const SsrnxJniValue *args, size_t n,
                                          void *user) {
    (void)token;
    (void)user;
    char *protocol = n && args[0].l ? ssrnx_jni_string_utf_dup(args[0].l) : NULL;
    if (!protocol) {
        fail(env, "java/lang/NullPointerException", "protocol");
        return no(NULL);
    }
    if (n == 2) {
        char *name = NULL;
        if (!args[1].l) {
            free(protocol);
            fail(env, "java/lang/IllegalArgumentException", "Missing provider");
            return no(NULL);
        }
        const bool provider_name = ssrnx_jni_is_instance_of_class_host(args[1].l, "java/lang/String");
        if (provider_name)
            name = ssrnx_jni_string_utf_dup(args[1].l);
        else {
            SsrnxJniValue value = {0};
            if (ssrnx_android_call_virtual(env, args[1].l, "getName", "()Ljava/lang/String;", NULL, 0,
                                           &value) &&
                value.l) {
                name = ssrnx_jni_string_utf_dup(value.l);
                ssrnx_jni_delete_local_ref_host(env, value.l);
            }
        }
        if (provider_name && name && !*name) {
            free(name);
            free(protocol);
            fail(env, "java/lang/IllegalArgumentException", "Missing provider");
            return no(NULL);
        }
        bool ok = name && strcmp(name, "APN64TLS") == 0;
        free(name);
        if (!ok) {
            free(protocol);
            fail(env,
                 provider_name ? "java/security/NoSuchProviderException"
                               : "java/security/NoSuchAlgorithmException",
                 "TLS provider unavailable");
            return no(NULL);
        }
    }
    if (strcasecmp(protocol, "TLS") && strcasecmp(protocol, "TLSv1.2") && strcasecmp(protocol, "Default")) {
        fail(env, "java/security/NoSuchAlgorithmException", protocol);
        free(protocol);
        return no(NULL);
    }
    java_net_TlsContext *c = NULL;
    void *ref = java_net_new_object(env, SC, sizeof(*c), N_CONTEXT, (void **)&c);
    if (c) {
        snprintf(c->protocol, sizeof(c->protocol), "%s", protocol);
        c->default_context = !strcasecmp(protocol, "Default");
        if (c->default_context)
            c->material = java_net_material_new(env, NULL);
    }
    free(protocol);
    return no(ref);
}
static SsrnxJniValue java_net_context_init(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    (void)user;
    java_net_TlsContext *c = payload(token, N_CONTEXT);
    if (!c || n != 3)
        return java_net_nv();
    if (c->default_context) {
        fail(env, "java/security/KeyManagementException", "Default context cannot be reinitialized");
        return java_net_nv();
    }
    if (args[0].l && ssrnx_jni_array_length_host(args[0].l) > 0) {
        fail(env, "java/security/KeyManagementException", "Client key managers are not supported");
        return java_net_nv();
    }
    if (args[2].l) {
        fail(env, "java/security/KeyManagementException", "Custom SecureRandom is not supported");
        return java_net_nv();
    }
    java_net_TlsMaterial *m = java_net_material_new(env, args[1].l);
    if (m)
        c->material = m;
    return java_net_nv();
}
static SsrnxJniValue java_net_context_default(void *env, void *token, const SsrnxJniValue *args, size_t n,
                                     void *user) {
    (void)token;
    (void)user;
    if (n) {
        if (!args[0].l) {
            fail(env, "java/lang/NullPointerException", "context");
            return java_net_nv();
        }
        pthread_mutex_lock(&java_net_net_lock);
        java_net_replace_ref(env, &default_context, args[0].l);
        pthread_mutex_unlock(&java_net_net_lock);
        return java_net_nv();
    }
    pthread_mutex_lock(&java_net_net_lock);
    void *ref = default_context ? ssrnx_jni_new_local_ref_host(env, default_context) : NULL;
    pthread_mutex_unlock(&java_net_net_lock);
    if (ref)
        return no(ref);
    SsrnxJniValue arg = no(text(env, "Default")), result = java_net_context_get_instance(env, NULL, &arg, 1, NULL);
    if (arg.l)
        ssrnx_jni_delete_local_ref_host(env, arg.l);
    if (result.l && !ssrnx_jni_exception_pending_host(env)) {
        pthread_mutex_lock(&java_net_net_lock);
        if (!default_context)
            java_net_replace_ref(env, &default_context, result.l);
        else {
            ssrnx_jni_delete_local_ref_host(env, result.l);
            result.l = ssrnx_jni_new_local_ref_host(env, default_context);
        }
        pthread_mutex_unlock(&java_net_net_lock);
    }
    return result;
}
static SsrnxJniValue java_net_provider_value(void *, void *, const SsrnxJniValue *, size_t, void *);
enum { C_PROTOCOL, C_PROVIDER, C_FACTORY, C_PARAMS };
static SsrnxJniValue java_net_context_value(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    (void)args;
    (void)n;
    java_net_TlsContext *c = payload(token, N_CONTEXT);
    if (!c)
        return no(NULL);
    int which = (int)(intptr_t)user;
    if (which == C_PROTOCOL)
        return no(text(env, c->protocol));
    if (which == C_PROVIDER) {
        pthread_mutex_lock(&java_net_net_lock);
        if (!provider) {
            void *base = ssrnx_jni_find_class_ref(env, "java/security/Provider");
            if (!base) {
                pthread_mutex_unlock(&java_net_net_lock);
                return no(NULL);
            }
            ssrnx_jni_delete_local_ref_host(env, base);
            SsrnxResult rc = ssrnx_jni_register_class(PROVIDER, "java/security/Provider");
            if (rc == SSRNX_OK)
                rc = ssrnx_jni_register_method(PROVIDER, "getName", "()Ljava/lang/String;", false,
                                               java_net_provider_value, (void *)0);
            if (rc == SSRNX_OK)
                rc = ssrnx_jni_register_method(PROVIDER, "getInfo", "()Ljava/lang/String;", false,
                                               java_net_provider_value, (void *)1);
            if (rc == SSRNX_OK)
                rc = ssrnx_jni_register_method(PROVIDER, "getVersion", "()D", false, java_net_provider_value,
                                               (void *)2);
            if (rc != SSRNX_OK) {
                pthread_mutex_unlock(&java_net_net_lock);
                java_net_oom(env);
                return no(NULL);
            }
            void *ref = ssrnx_jni_new_local_object(env, PROVIDER);
            java_net_replace_ref(env, &provider, ref);
            if (ref)
                ssrnx_jni_delete_local_ref_host(env, ref);
        }
        void *ref = ssrnx_jni_new_local_ref_host(env, provider);
        pthread_mutex_unlock(&java_net_net_lock);
        return no(ref);
    }
    if (!c->material) {
        fail(env, "java/lang/IllegalStateException", "SSLContext is not initialized");
        return no(NULL);
    }
    if (which == C_PARAMS) {
        void *ref = NULL;
        (void)java_net_params_new(env, &ref);
        return no(ref);
    }
    java_net_Factory *f = NULL;
    void *ref = java_net_new_object(env, DSSF, sizeof(*f), N_FACTORY, (void **)&f);
    if (f)
        f->material = c->material;
    return no(ref);
}
bool ssrnx_java_net_factory_platform_trust(void *ref) {
    char cls[128];
    if(!ref||!ssrnx_jni_object_class_name_host(ref,cls,sizeof(cls))||strcmp(cls,DSSF))return false;
    java_net_Factory *f=payload(ssrnx_jni_object_token_host(ref),N_FACTORY);
    return f&&f->material&&f->material->platform_roots;
}
static SsrnxJniValue java_net_factory_default(void *env, void *token, const SsrnxJniValue *args, size_t n,
                                     void *user) {
    (void)token;
    (void)args;
    (void)n;
    if (user) {
        SsrnxJniValue context = java_net_context_default(env, NULL, NULL, 0, NULL);
        if (!context.l)
            return no(NULL);
        SsrnxJniValue result =
            java_net_context_value(env, ssrnx_jni_object_token_host(context.l), NULL, 0, (void *)(intptr_t)C_FACTORY);
        ssrnx_jni_delete_local_ref_host(env, context.l);
        return result;
    }
    pthread_mutex_lock(&java_net_net_lock);
    void *ref = java_net_default_factory ? ssrnx_jni_new_local_ref_host(env, java_net_default_factory) : NULL;
    pthread_mutex_unlock(&java_net_net_lock);
    if (ref)
        return no(ref);
    java_net_Factory *f = NULL;
    ref = java_net_new_object(env, DSF, sizeof(*f), N_FACTORY, (void **)&f);
    if (ref) {
        pthread_mutex_lock(&java_net_net_lock);
        if (!java_net_default_factory)
            java_net_replace_ref(env, &java_net_default_factory, ref);
        else {
            ssrnx_jni_delete_local_ref_host(env, ref);
            ref = ssrnx_jni_new_local_ref_host(env, java_net_default_factory);
        }
        pthread_mutex_unlock(&java_net_net_lock);
    }
    return no(ref);
}
static SsrnxJniValue java_net_factory_create(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    java_net_Factory *factory = payload(token, N_FACTORY);
    if (!factory) {
        fail(env, "java/net/SocketException", "Unconnected sockets not implemented");
        return no(NULL);
    }
    void *ref = java_net_socket_new(env, factory->material);
    java_net_SocketObject *s = ref ? payload(ssrnx_jni_object_token_host(ref), N_SOCKET) : NULL;
    if (!s)
        return no(NULL);
    if ((intptr_t)user == 2) {
        java_net_SocketObject *parent = args[0].l ? payload(ssrnx_jni_object_token_host(args[0].l), N_SOCKET) : NULL;
        if (!parent || !java_net_socket_open(env, parent) || !parent->connected) {
            fail(env, "java/net/SocketException", "Layered TLS requires a connected socket");
            ssrnx_jni_delete_local_ref_host(env, ref);
            return no(NULL);
        }
        if (!java_net_port_valid(env, args[2].i)) {
            ssrnx_jni_delete_local_ref_host(env, ref);
            return no(NULL);
        }
        s->transport = parent->transport;
        s->parent = parent;
        s->auto_close = args[3].z;
        s->connected = true;
        s->bound = true;
        s->peer_host = args[1].l ? ssrnx_jni_string_utf_dup(args[1].l) : NULL;
        s->peer_port = args[2].i;
        s->peer_address = parent->peer_address;
        s->peer_size = parent->peer_size;
        s->local_address = parent->local_address;
        s->local_size = parent->local_size;
        atomic_store(&s->timeout, atomic_load(&parent->timeout));
    } else
        java_net_connect_args(env, s, args, n, user != NULL);
    if (ssrnx_jni_exception_pending_host(env)) {
        java_net_close_socket(s);
        ssrnx_jni_delete_local_ref_host(env, ref);
        return no(NULL);
    }
    return no(ref);
}
static SsrnxJniValue java_net_factory_ciphers(void *env, void *token, const SsrnxJniValue *args, size_t n,
                                     void *user) {
    (void)token;
    (void)args;
    (void)n;
    (void)user;
    return no(java_net_all_ciphers(env));
}

enum { P_CIPHERS, P_PROTOCOLS, P_ALPN, P_ENDPOINT, P_NEED_AUTH, P_WANT_AUTH };
static SsrnxJniValue java_net_params_access(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    java_net_Parameters *p = payload(token, N_PARAMS);
    if (!p)
        return java_net_nv();
    int which = (int)(intptr_t)user;
    if (which == P_NEED_AUTH || which == P_WANT_AUTH) {
        bool *slot = which == P_NEED_AUTH ? &p->need_auth : &p->want_auth;
        if (n) {
            *slot = args[0].z;
            if (which == P_NEED_AUTH)
                p->want_auth = false;
            else
                p->need_auth = false;
            return java_net_nv();
        }
        return java_net_nb(*slot);
    }
    if (which == P_ENDPOINT) {
        if (n) {
            char *value = args[0].l ? ssrnx_jni_string_utf_dup(args[0].l) : NULL;
            free(p->endpoint);
            p->endpoint = value;
            return java_net_nv();
        }
        return no(text(env, p->endpoint));
    }
    void **slot = which == P_CIPHERS ? &p->ciphers : which == P_PROTOCOLS ? &p->protocols : &p->alpn;
    if (n) {
        if (which == P_ALPN && !args[0].l) {
            fail(env, "java/lang/IllegalArgumentException", "Null application protocols");
            return java_net_nv();
        }
        char **names = which == P_ALPN && args[0].l ? java_net_array_text(env, args[0].l) : NULL;
        if (which == P_ALPN && args[0].l && !names)
            return java_net_nv();
        if (which == P_ALPN)
            for (size_t i = 0; names && names[i]; ++i)
                if (!*names[i] || strlen(names[i]) > 255) {
                    java_net_free_texts(names);
                    fail(env, "java/lang/IllegalArgumentException", "Invalid application protocol");
                    return java_net_nv();
                }
        java_net_free_texts(names);
        void *copy = java_net_array_copy(env, args[0].l);
        java_net_replace_ref(env, slot, copy);
        if (copy)
            ssrnx_jni_delete_local_ref_host(env, copy);
        return java_net_nv();
    }
    return no(java_net_array_copy(env, *slot));
}
static SsrnxJniValue java_net_params_ctor(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    (void)user;
    java_net_Parameters *p = NULL;
    void *ref = java_net_new_object(env, PARAM, sizeof(*p), N_PARAMS, (void **)&p);
    if (!ref)
        return java_net_nv();
    (void)ssrnx_jni_set_token_host_data(token, p);
    ssrnx_jni_delete_local_ref_host(env, ref);
    void *empty = strings(env, NULL, 0);
    java_net_replace_ref(env, &p->alpn, empty);
    if (empty)
        ssrnx_jni_delete_local_ref_host(env, empty);
    if (n)
        (void)java_net_params_access(env, token, args, 1, (void *)(intptr_t)P_CIPHERS);
    if (n == 2)
        (void)java_net_params_access(env, token, args + 1, 1, (void *)(intptr_t)P_PROTOCOLS);
    return java_net_nv();
}
static bool java_net_check_enabled(void *env, void *array, bool protocols) {
    if (!array) {
        fail(env, "java/lang/IllegalArgumentException", "Null enabled list");
        return false;
    }
    char **names = java_net_array_text(env, array);
    if (!names)
        return false;
    bool ok = true;
    for (size_t i = 0; names[i] && ok; ++i) {
        if (protocols)
            ok = !strcmp(names[i], "TLSv1.2");
        else {
            ok = false;
            const int *ids = mbedtls_ssl_list_ciphersuites();
            for (size_t j = 0; ids[j]; ++j) {
                char name[128];
                java_net_cipher_name(ids[j], name, sizeof(name));
                if (java_net_supported_cipher(ids[j]) && !strcmp(name, names[i])) {
                    ok = true;
                    break;
                }
            }
        }
    }
    java_net_free_texts(names);
    if (!ok)
        fail(env, "java/lang/IllegalArgumentException", "Unsupported cipher suite or protocol");
    return ok;
}
enum {
    T_HANDSHAKE,
    T_PARAMS,
    T_CIPHERS,
    T_PROTOCOLS,
    T_SUPPORTED_CIPHERS,
    T_SUPPORTED_PROTOCOLS,
    T_APPLICATION,
    T_CLIENT,
    T_NEED,
    T_WANT,
    T_CREATE_SESSION,
    T_SESSION
};
static SsrnxJniValue java_net_tls_access(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    java_net_SocketObject *s = payload(token, N_SOCKET);
    if (!s || !s->tls)
        return java_net_nv();
    int which = (int)(intptr_t)user;
    if (which == T_CIPHERS || which == T_PROTOCOLS) {
        void **slot = which == T_CIPHERS ? &s->parameters->ciphers : &s->parameters->protocols;
        if (n) {
            if (!java_net_check_enabled(env, args[0].l, which == T_PROTOCOLS))
                return java_net_nv();
            void *copy = java_net_array_copy(env, args[0].l);
            java_net_replace_ref(env, slot, copy);
            if (copy)
                ssrnx_jni_delete_local_ref_host(env, copy);
            return java_net_nv();
        }
        return no(java_net_array_copy(env, *slot));
    }
    if (which == T_SUPPORTED_CIPHERS)
        return no(java_net_all_ciphers(env));
    if (which == T_SUPPORTED_PROTOCOLS) {
        const char *protocol[] = {"TLSv1.2"};
        return no(strings(env, protocol, 1));
    }
    if (which == T_PARAMS) {
        if (n) {
            java_net_Parameters *p = args[0].l ? payload(ssrnx_jni_object_token_host(args[0].l), N_PARAMS) : NULL;
            if (!p) {
                fail(env, "java/lang/NullPointerException", "parameters");
                return java_net_nv();
            }
            if (p->ciphers && !java_net_check_enabled(env, p->ciphers, false))
                return java_net_nv();
            if (p->protocols && !java_net_check_enabled(env, p->protocols, true))
                return java_net_nv();
            if (p->need_auth || p->want_auth) {
                fail(env, "java/lang/IllegalArgumentException",
                     "Client authentication mode is not supported");
                return java_net_nv();
            }
            /* Null cipher/protocol lists leave the current enabled lists intact. */
            if (p->ciphers)
                java_net_replace_ref(env, &s->parameters->ciphers, p->ciphers);
            if (p->protocols)
                java_net_replace_ref(env, &s->parameters->protocols, p->protocols);
            java_net_replace_ref(env, &s->parameters->alpn, p->alpn);
            free(s->parameters->endpoint);
            s->parameters->endpoint = p->endpoint ? strdup(p->endpoint) : NULL;
            s->parameters->need_auth = p->need_auth;
            s->parameters->want_auth = p->want_auth;
            return java_net_nv();
        }
        void *ref = NULL;
        java_net_Parameters *p = java_net_params_new(env, &ref);
        java_net_params_copy(env, p, s->parameters);
        return no(ref);
    }
    if (which == T_CLIENT) {
        if (n && !args[0].z)
            fail(env, "java/lang/IllegalArgumentException", "Server TLS mode is not supported");
        return n ? java_net_nv() : java_net_nb(true);
    }
    if (which == T_CREATE_SESSION) {
        if (n && !args[0].z)
            fail(env, "java/lang/IllegalArgumentException", "Disabling session creation is not supported");
        return n ? java_net_nv() : java_net_nb(true);
    }
    if (which == T_NEED || which == T_WANT) {
        if (n && args[0].z) {
            fail(env, "java/lang/IllegalArgumentException", "Client authentication mode is not supported");
            return java_net_nv();
        }
        return n ? java_net_nv() : java_net_nb(false);
    }
    if (which == T_APPLICATION) {
        const char *protocol = "";
#if defined(MBEDTLS_SSL_ALPN)
        if (s->handshake) {
            const char *selected = mbedtls_ssl_get_alpn_protocol(&s->ssl);
            if (selected)
                protocol = selected;
        }
#endif
        return no(text(env, protocol));
    }
    if (which == T_HANDSHAKE || which == T_SESSION) {
        if (java_net_socket_open(env, s)) {
            pthread_mutex_lock(&s->transport->io);
            (void)handshake(env, s);
            pthread_mutex_unlock(&s->transport->io);
        }
        if (which == T_HANDSHAKE)
            return java_net_nv();
        if (ssrnx_jni_exception_pending_host(env)) {
            void *error = ssrnx_jni_exception_occurred_host(env);
            if (error && ssrnx_jni_is_instance_of_class_host(error, "java/io/IOException"))
                ssrnx_jni_exception_clear_host(env);
            if (error)
                ssrnx_jni_delete_local_ref_host(env, error);
        }
        void *session = ssrnx_jni_new_local_object(env, SESSION);
        if (session)
            (void)ssrnx_jni_set_object_host_data(session, s);
        return no(session);
    }
    return java_net_nv();
}
enum {
    SE_PROTOCOL,
    SE_CIPHER,
    SE_PEER_HOST,
    SE_PEER_PORT,
    SE_PEER_CERTS,
    SE_VALID,
    SE_ID,
    SE_LOCAL_CERTS,
    SE_PACKET,
    SE_APPLICATION
};
static SsrnxJniValue java_net_session_value(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    (void)args;
    (void)n;
    java_net_SocketObject *s = payload(token, N_SOCKET);
    if (!s)
        return java_net_nv();
    switch ((intptr_t)user) {
    case SE_PROTOCOL:
        return no(text(env, s->handshake ? "TLSv1.2" : "NONE"));
    case SE_CIPHER: {
        char name[128] = "SSL_NULL_WITH_NULL_NULL";
        if (s->handshake) {
            const char *cipher = mbedtls_ssl_get_ciphersuite_name(s->session.ciphersuite);
            size_t i = 0;
            for (; cipher && cipher[i] && i + 1 < sizeof(name); ++i)
                name[i] = cipher[i] == '-' ? '_' : cipher[i];
            name[i] = 0;
        }
        return no(text(env, name));
    }
    case SE_PEER_HOST:
        return no(text(env, s->peer_host ? s->peer_host : ""));
    case SE_PEER_PORT:
        return java_net_ni(s->peer_port);
    case SE_VALID:
        return java_net_nb(s->handshake);
    case SE_LOCAL_CERTS:
        return no(NULL);
    case SE_PACKET:
        return java_net_ni(18432);
    case SE_APPLICATION:
        return java_net_ni(16384);
    case SE_ID: {
        const size_t len = s->handshake ? s->session.id_len : 0;
        void *array = ssrnx_jni_new_local_byte_array(env, (int32_t)len);
        if (array && len)
            (void)ssrnx_jni_byte_array_write(array, 0, (int32_t)len, (int8_t *)s->session.id);
        return no(array);
    }
    case SE_PEER_CERTS: {
        const mbedtls_x509_crt *peer = s->handshake ? s->session.peer_cert : NULL;
        if (!peer) {
            fail(env, "javax/net/ssl/SSLPeerUnverifiedException", "No verified peer");
            return no(NULL);
        }
        size_t count = 0;
        for (const mbedtls_x509_crt *p = peer; p && p->raw.len; p = p->next)
            ++count;
        void *array =
            ssrnx_jni_new_local_reference_array(env, "[Ljava/security/cert/Certificate;", (int32_t)count);
        size_t i = 0;
        for (const mbedtls_x509_crt *p = peer; array && p && p->raw.len; p = p->next, ++i) {
            void *cert = ssrnx_jni_new_local_object(env, "java/security/cert/X509Certificate"),
                 *der = ssrnx_jni_new_local_byte_array(env, (int32_t)p->raw.len);
            if (cert && der) {
                (void)ssrnx_jni_byte_array_write(der, 0, (int32_t)p->raw.len, (const int8_t *)p->raw.p);
                bool found = false;
                (void)ssrnx_jni_set_instance_field_value_for_class(cert, "java/security/cert/X509Certificate",
                                                                   "apn64Encoded", "[B", no(der), &found);
                (void)ssrnx_jni_set_object_array_element_host(env, array, (int32_t)i, cert);
            }
            if (der)
                ssrnx_jni_delete_local_ref_host(env, der);
            if (cert)
                ssrnx_jni_delete_local_ref_host(env, cert);
        }
        return no(array);
    }
    default:
        return java_net_nv();
    }
}
static SsrnxJniValue java_net_provider_value(void *env, void *token, const SsrnxJniValue *args, size_t n, void *user) {
    (void)token;
    (void)args;
    (void)n;
    return (intptr_t)user == 2
               ? (SsrnxJniValue){.d = 1.0}
               : no(text(env, (intptr_t)user == 0 ? "APN64TLS" : "APN64 verified Mbed TLS client provider"));
}

/* java net datagram */
/* Java UDP packets and private BSD sockets share the client transport lifetime. */
#define UDP_SOCKET "java/net/DatagramSocket"
#define UDP_PACKET "java/net/DatagramPacket"
typedef struct java_net_DatagramPacketPayload {
    java_net_NetObject object;
    void *data, *address; /* global refs; received data stays in the caller's array */
    int32_t offset, length, capacity, port;
    pthread_mutex_t lock;
} java_net_DatagramPacketPayload;

static bool java_net_datagram_range(void *env, void *data, int32_t offset, int32_t length) {
    if (!data) { fail(env, "java/lang/NullPointerException", "buffer"); return false; }
    const int32_t size = ssrnx_jni_byte_array_length(data);
    if (offset < 0 || length < 0 || offset > size || length > size - offset) {
        fail(env, "java/lang/IllegalArgumentException", "DatagramPacket range"); return false;
    }
    return true;
}

enum { DP_DATA, DP_OFFSET, DP_LENGTH, DP_ADDRESS, DP_PORT, DP_SOCKET_ADDRESS,
       DP_SET_DATA, DP_SET_RANGE, DP_SET_LENGTH, DP_SET_ADDRESS, DP_SET_PORT, DP_SET_SOCKET_ADDRESS };

static bool java_net_datagram_set_peer(void *env, java_net_DatagramPacketPayload *p, void *ref) {
    Address *a = ref ? payload(ssrnx_jni_object_token_host(ref), N_ADDRESS) : NULL;
    if (!a || !ssrnx_jni_is_instance_of_class_host(ref, ISA) || !a->resolved) {
        fail(env, "java/lang/IllegalArgumentException", "Unresolved/unsupported socket address"); return false;
    }
    void *ip = java_net_address_copy(env, IA, a);
    if (!ip) return false;
    java_net_replace_ref(env, &p->address, ip); p->port = a->port;
    ssrnx_jni_delete_local_ref_host(env, ip);
    return true;
}

/* Constructor mode: bit 0 offset overload, bit 1 InetAddress, bit 2 SocketAddress. */
static SsrnxJniValue java_net_datagram_packet_ctor(void *env, void *token, const SsrnxJniValue *a, size_t n, void *user) {
    (void)n;
    const intptr_t mode = (intptr_t)user;
    const int32_t offset = mode & 1 ? a[1].i : 0, length = a[mode & 1 ? 2 : 1].i;
    if (!java_net_datagram_range(env, a[0].l, offset, length)) return java_net_nv();
    java_net_DatagramPacketPayload *p = NULL;
    void *ref = java_net_new_object(env, UDP_PACKET, sizeof(*p), N_PACKET, (void **)&p);
    if (!ref) return java_net_nv();
    pthread_mutex_init(&p->lock, NULL);
    (void)ssrnx_jni_set_token_host_data(token, p);
    ssrnx_jni_delete_local_ref_host(env, ref);
    java_net_replace_ref(env, &p->data, a[0].l); p->offset = offset; p->length = p->capacity = length;
    const size_t peer = mode & 1 ? 3 : 2;
    if (mode & 2) {
        if (java_net_port_valid(env, a[peer + 1].i)) {
            java_net_replace_ref(env, &p->address, a[peer].l); p->port = a[peer + 1].i;
        }
    } else if (mode & 4) (void)java_net_datagram_set_peer(env, p, a[peer].l);
    return java_net_nv();
}

static SsrnxJniValue java_net_datagram_packet_op(void *env, void *token, const SsrnxJniValue *a, size_t n, void *user) {
    (void)n;
    java_net_DatagramPacketPayload *p = payload(token, N_PACKET);
    if (!p) return java_net_nv();
    pthread_mutex_lock(&p->lock);
    SsrnxJniValue out = {0};
    switch ((intptr_t)user) {
    case DP_DATA: out = no(p->data ? ssrnx_jni_new_local_ref_host(env, p->data) : NULL); break;
    case DP_OFFSET: out = java_net_ni(p->offset); break;
    case DP_LENGTH: out = java_net_ni(p->length); break;
    case DP_ADDRESS: out = no(p->address ? ssrnx_jni_new_local_ref_host(env, p->address) : NULL); break;
    case DP_PORT: out = java_net_ni(p->port); break;
    case DP_SOCKET_ADDRESS: {
        Address value = {0};
        if ((!p->address && java_net_resolve_address(&value, NULL, true)) || java_net_decode_address(env, p->address, &value)) {
            value.port = p->port; java_net_address_port(&value); out = no(java_net_address_copy(env, ISA, &value));
        }
        break;
    }
    case DP_SET_DATA: case DP_SET_RANGE: {
        const int32_t offset = (intptr_t)user == DP_SET_RANGE ? a[1].i : 0;
        const int32_t length = (intptr_t)user == DP_SET_RANGE ? a[2].i : ssrnx_jni_byte_array_length(a[0].l);
        if (java_net_datagram_range(env, a[0].l, offset, length)) {
            java_net_replace_ref(env, &p->data, a[0].l); p->offset = offset; p->length = p->capacity = length;
        }
        break;
    }
    case DP_SET_LENGTH:
        if (java_net_datagram_range(env, p->data, p->offset, a[0].i)) p->length = p->capacity = a[0].i;
        break;
    case DP_SET_ADDRESS: java_net_replace_ref(env, &p->address, a[0].l); break;
    case DP_SET_PORT: if (java_net_port_valid(env, a[0].i)) p->port = a[0].i; break;
    case DP_SET_SOCKET_ADDRESS: (void)java_net_datagram_set_peer(env, p, a[0].l); break;
    }
    pthread_mutex_unlock(&p->lock);
    return out;
}

static void java_net_datagram_local(java_net_SocketObject *s) {
    s->local_size = sizeof(s->local_address);
    if (getsockname(s->transport->fd, (struct sockaddr *)&s->local_address, &s->local_size) == 0)
        s->bound = true;
}

/* Caller holds io. Pick the address family before creating the private descriptor. */
static bool java_net_datagram_descriptor(void *env, java_net_SocketObject *s, int family) {
    java_net_Transport *t = s->transport;
    if (!java_net_socket_open(env, s)) return false;
    if (t->fd >= 0) return true;
    if (java_net_network_ready() != 0) { fail(env, "java/net/SocketException", "Network service unavailable"); return false; }
    t->fd = socket(family, SOCK_DGRAM, 0);
    if (t->fd < 0) { fail(env, "java/net/SocketException", strerror(errno)); return false; }
    if (family == AF_INET6) {
        int only = 0;
        if (setsockopt(t->fd, IPPROTO_IPV6, IPV6_V6ONLY, &only, sizeof(only)) != 0) {
            fail(env, "java/net/SocketException", strerror(errno)); close(t->fd); t->fd = -1; return false;
        }
    }
    int flags = fcntl(t->fd, F_GETFL, 0), broadcast = 1, reuse = s->reuse;
    if (flags < 0 || fcntl(t->fd, F_SETFL, flags | O_NONBLOCK) != 0) {
        fail(env, "java/net/SocketException", strerror(errno)); close(t->fd); t->fd = -1; return false;
    }
    (void)setsockopt(t->fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    (void)setsockopt(t->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    return true;
}

static bool java_net_datagram_bind_address(void *env, java_net_SocketObject *s, const Address *a) {
    if (!java_net_socket_open(env, s)) return false;
    if (s->bound) { fail(env, "java/net/SocketException", "Already bound"); return false; }
    if (!a) { fail(env, "java/lang/IllegalArgumentException", "Unsupported address"); return false; }
    if (!a->resolved) { fail(env, "java/net/SocketException", "Unresolved address"); return false; }
    pthread_mutex_lock(&s->transport->io);
    bool ok = java_net_datagram_descriptor(env, s, a->address.ss_family);
    if (ok && bind(s->transport->fd, (const struct sockaddr *)&a->address, a->size) != 0) {
        fail(env, "java/net/BindException", strerror(errno)); ok = false;
    }
    if (ok) { java_net_datagram_local(s); s->datagram_binding = s->local_address; s->datagram_binding_size = s->local_size; }
    pthread_mutex_unlock(&s->transport->io);
    return ok;
}

static bool java_net_datagram_wildcard_port(void *env, java_net_SocketObject *s, int port) {
    Address wildcard = {.port = port, .resolved = true, .size = sizeof(struct sockaddr_in6)};
    struct sockaddr_in6 *v6 = (struct sockaddr_in6 *)&wildcard.address;
    v6->sin6_family = AF_INET6; v6->sin6_port = htons(port);
    return java_net_datagram_bind_address(env, s, &wildcard);
}
static bool java_net_datagram_wildcard(void *env, java_net_SocketObject *s) { return java_net_datagram_wildcard_port(env, s, 0); }

/* A dual-stack descriptor accepts IPv4 through mapped endpoints; Java exposes Inet4Address. */
static void java_net_datagram_endpoint(java_net_SocketObject *s, Address *peer) {
    struct sockaddr_storage local;
    socklen_t size = sizeof(local);
    if (peer->address.ss_family != AF_INET || getsockname(s->transport->fd, (struct sockaddr *)&local, &size) != 0 || local.ss_family != AF_INET6) return;
    struct sockaddr_in v4 = *(struct sockaddr_in *)&peer->address;
    struct sockaddr_in6 *v6 = (struct sockaddr_in6 *)&peer->address;
    memset(&peer->address, 0, sizeof(peer->address));
    v6->sin6_family = AF_INET6; v6->sin6_port = v4.sin_port;
    ((unsigned char *)&v6->sin6_addr)[10] = 0xff; ((unsigned char *)&v6->sin6_addr)[11] = 0xff;
    memcpy((unsigned char *)&v6->sin6_addr + 12, &v4.sin_addr, 4); peer->size = sizeof(*v6);
}
static void java_net_datagram_unmap(Address *peer) {
    if (peer->address.ss_family != AF_INET6 || !IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)&peer->address)->sin6_addr)) return;
    struct sockaddr_in6 v6 = *(struct sockaddr_in6 *)&peer->address;
    struct sockaddr_in *v4 = (struct sockaddr_in *)&peer->address;
    memset(&peer->address, 0, sizeof(peer->address));
    v4->sin_family = AF_INET; v4->sin_port = v6.sin6_port;
    memcpy(&v4->sin_addr, (unsigned char *)&v6.sin6_addr + 12, 4); peer->size = sizeof(*v4);
}

static SsrnxJniValue java_net_datagram_socket_ctor(void *env, void *token, const SsrnxJniValue *a, size_t n, void *user) {
    void *ref = java_net_socket_new(env, NULL);
    if (!ref) return java_net_nv();
    java_net_SocketObject *s = payload(ssrnx_jni_object_token_host(ref), N_SOCKET);
    (void)ssrnx_jni_set_token_host_data(token, s); ssrnx_jni_delete_local_ref_host(env, ref);
    if ((intptr_t)user == 1) {
        if (a[0].l) {
            Address *binding = payload(ssrnx_jni_object_token_host(a[0].l), N_ADDRESS);
            if (!ssrnx_jni_is_instance_of_class_host(a[0].l, ISA)) binding = NULL;
            (void)java_net_datagram_bind_address(env, s, binding);
        }
    } else if (!n) (void)java_net_datagram_wildcard(env, s);
    else if (java_net_port_valid(env, a[0].i)) {
        Address binding = {.port = a[0].i};
        if (n == 2 && a[1].l) {
            if (!java_net_decode_address(env, a[1].l, &binding)) { fail(env, "java/lang/IllegalArgumentException", "address"); goto complete; }
            binding.port = a[0].i; java_net_address_port(&binding);
        } else { (void)java_net_datagram_wildcard_port(env, s, a[0].i); goto complete; }
        (void)java_net_datagram_bind_address(env, s, &binding);
    }
complete:
    if (ssrnx_jni_exception_pending_host(env)) java_net_close_socket(s);
    return java_net_nv();
}

static SsrnxJniValue java_net_datagram_bind(void *env, void *token, const SsrnxJniValue *a, size_t n, void *user) {
    (void)n; (void)user;
    java_net_SocketObject *s = payload(token, N_SOCKET);
    if (!s) return java_net_nv();
    if (!a[0].l) (void)java_net_datagram_wildcard(env, s);
    else {
        Address *binding = payload(ssrnx_jni_object_token_host(a[0].l), N_ADDRESS);
        if (!ssrnx_jni_is_instance_of_class_host(a[0].l, ISA)) binding = NULL;
        (void)java_net_datagram_bind_address(env, s, binding);
    }
    return java_net_nv();
}

static SsrnxJniValue java_net_datagram_connect(void *env, void *token, const SsrnxJniValue *a, size_t n, void *user) {
    (void)user;
    java_net_SocketObject *s = payload(token, N_SOCKET);
    Address peer = {0};
    if (!s || !java_net_socket_open(env, s)) return java_net_nv();
    if (n == 2) {
        if (!java_net_port_valid(env, a[1].i)) return java_net_nv();
        if (!java_net_decode_address(env, a[0].l, &peer)) { fail(env, "java/lang/IllegalArgumentException", "address"); return java_net_nv(); }
        peer.port = a[1].i; java_net_address_port(&peer);
    } else {
        Address *p = a[0].l ? payload(ssrnx_jni_object_token_host(a[0].l), N_ADDRESS) : NULL;
        if (!p || !p->resolved || !ssrnx_jni_is_instance_of_class_host(a[0].l, ISA)) {
            fail(env, "java/lang/IllegalArgumentException", "Unresolved/unsupported address"); return java_net_nv();
        }
        peer = *p;
    }
    if (!s->bound && !java_net_datagram_wildcard(env, s)) return java_net_nv();
    pthread_mutex_lock(&s->transport->io);
    Address endpoint = peer;
    java_net_datagram_endpoint(s, &endpoint);
    if (java_net_datagram_descriptor(env, s, endpoint.address.ss_family)) {
        if (connect(s->transport->fd, (struct sockaddr *)&endpoint.address, endpoint.size) == 0) {
            s->peer_address = peer.address; s->peer_size = peer.size; s->peer_port = peer.port; s->connected = true;
            java_net_datagram_local(s);
            Address local = {.address = s->local_address, .size = s->local_size};
            java_net_datagram_unmap(&local); s->local_address = local.address; s->local_size = local.size;
        } else fail(env, "java/net/SocketException", strerror(errno));
    }
    pthread_mutex_unlock(&s->transport->io);
    return java_net_nv();
}

static SsrnxJniValue java_net_datagram_disconnect(void *env, void *token, const SsrnxJniValue *a, size_t n, void *user) {
    (void)a; (void)n; (void)user;
    java_net_SocketObject *s = payload(token, N_SOCKET);
    if (!s || s->closed || !s->connected) return java_net_nv();
    pthread_mutex_lock(&s->transport->io);
    struct sockaddr address = {.sa_family = AF_UNSPEC};
    if (connect(s->transport->fd, &address, sizeof(address)) == 0) {
        s->connected = false; s->peer_size = 0; s->peer_port = 0;
        struct sockaddr_storage local;
        socklen_t size = sizeof(local);
        if (getsockname(s->transport->fd, (struct sockaddr *)&local, &size) == 0) {
            int port = local.ss_family == AF_INET ? ((struct sockaddr_in *)&local)->sin_port : ((struct sockaddr_in6 *)&local)->sin6_port;
            /* Linux may drop an implicit binding on AF_UNSPEC disconnect. Restore its port. */
            if (!port && bind(s->transport->fd, (struct sockaddr *)&s->datagram_binding, s->datagram_binding_size) != 0)
                fail(env, "java/net/SocketException", strerror(errno));
        }
        java_net_datagram_local(s);
    } else fail(env, "java/net/SocketException", strerror(errno));
    pthread_mutex_unlock(&s->transport->io);
    return java_net_nv();
}

static bool java_net_datagram_same_peer(const Address *a, const java_net_SocketObject *s) {
    if (a->port != s->peer_port || a->address.ss_family != s->peer_address.ss_family) return false;
    if (a->address.ss_family == AF_INET)
        return ((const struct sockaddr_in *)&a->address)->sin_addr.s_addr == ((const struct sockaddr_in *)&s->peer_address)->sin_addr.s_addr;
    return !memcmp(&((const struct sockaddr_in6 *)&a->address)->sin6_addr,
                   &((const struct sockaddr_in6 *)&s->peer_address)->sin6_addr, 16);
}

static SsrnxJniValue java_net_datagram_io(void *env, void *token, const SsrnxJniValue *a, size_t n, void *user) {
    (void)n;
    const bool sending = user != NULL;
    java_net_SocketObject *s = payload(token, N_SOCKET);
    if (!java_net_socket_open(env, s)) return java_net_nv();
    java_net_DatagramPacketPayload *p = a[0].l ? payload(ssrnx_jni_object_token_host(a[0].l), N_PACKET) : NULL;
    if (!p) { fail(env, "java/lang/NullPointerException", "packet"); return java_net_nv(); }
    pthread_mutex_lock(&p->lock);
    pthread_mutex_t *direction = sending ? &s->transport->write : &s->transport->read;
    pthread_mutex_lock(direction);
    Address peer = {0};
    unsigned char *bytes = NULL;
    if (sending) {
        if (!p->address && s->connected) {
            peer.address = s->peer_address; peer.size = s->peer_size; peer.port = s->peer_port; peer.resolved = true;
            void *ip = java_net_address_copy(env, IA, &peer);
            if (!ip) goto done;
            java_net_replace_ref(env, &p->address, ip); p->port = peer.port; ssrnx_jni_delete_local_ref_host(env, ip);
        } else if (!java_net_decode_address(env, p->address, &peer)) {
            fail(env, "java/lang/IllegalArgumentException", "Packet address missing"); goto done;
        }
        peer.port = p->port;
        if (!java_net_port_valid(env, peer.port)) goto done;
        java_net_address_port(&peer);
        if (s->connected && !java_net_datagram_same_peer(&peer, s)) {
            fail(env, "java/lang/IllegalArgumentException", "Packet address differs from connected peer"); goto done;
        }
    }
    if (!s->bound && !java_net_datagram_wildcard(env, s)) goto done;
    if (sending) java_net_datagram_endpoint(s, &peer);
    const int32_t length = sending ? p->length : p->capacity;
    if (!java_net_datagram_range(env, p->data, p->offset, length)) goto done;
    bytes = malloc(length ? (size_t)length : 1u);
    if (!bytes) { java_net_oom(env); goto done; }
    if (sending) (void)ssrnx_jni_byte_array_read(p->data, p->offset, length, (int8_t *)bytes);
    const int timeout = sending ? 0 : atomic_load(&s->timeout);
    const int64_t deadline = timeout ? java_net_millis() + timeout : 0;
    for (;;) {
        pthread_mutex_lock(&s->transport->io);
        if (!java_net_socket_open(env, s)) { pthread_mutex_unlock(&s->transport->io); break; }
        peer.size = sending ? peer.size : sizeof(peer.address);
        ssize_t count = sending ? sendto(s->transport->fd, bytes, (size_t)length, MSG_NOSIGNAL,
                                         (struct sockaddr *)&peer.address, peer.size)
                                : recvfrom(s->transport->fd, bytes, (size_t)length, 0,
                                           (struct sockaddr *)&peer.address, &peer.size);
        const int error = errno;
        pthread_mutex_unlock(&s->transport->io);
        if (count >= 0) {
            if (!sending) {
                java_net_datagram_unmap(&peer);
                peer.port = peer.address.ss_family == AF_INET ? ntohs(((struct sockaddr_in *)&peer.address)->sin_port)
                                                              : ntohs(((struct sockaddr_in6 *)&peer.address)->sin6_port);
                peer.resolved = true;
                (void)ssrnx_jni_byte_array_write(p->data, p->offset, (int32_t)count, (const int8_t *)bytes);
                void *ip = java_net_address_copy(env, IA, &peer);
                if (ip) { java_net_replace_ref(env, &p->address, ip); ssrnx_jni_delete_local_ref_host(env, ip); }
                p->port = peer.port; p->length = (int32_t)count;
            } else if (count != length) fail(env, "java/io/IOException", "Partial datagram send");
            break;
        }
        if (error != EAGAIN && error != EWOULDBLOCK && error != EINTR) {
            fail(env, error == ECONNREFUSED ? "java/net/PortUnreachableException" : "java/net/SocketException", strerror(error)); break;
        }
        int ready = java_net_await_fd(s, sending, deadline);
        if (ready > 0) continue;
        fail(env, ready == 0 ? "java/net/SocketTimeoutException" : "java/net/SocketException",
             ready == 0 ? "Receive timed out" : "Socket is closed"); break;
    }
done:
    free(bytes); pthread_mutex_unlock(direction); pthread_mutex_unlock(&p->lock);
    return java_net_nv();
}

static SsrnxJniValue java_net_datagram_state(void *env, void *token, const SsrnxJniValue *a, size_t n, void *user) {
    java_net_SocketObject *s = payload(token, N_SOCKET);
    const intptr_t op = (intptr_t)user;
    if (!s) return java_net_nv();
    if ((s->closed || s->transport->closed) && (op == S_LOCAL_ADDR || op == S_LOCAL_PORT || op == S_LOCAL_SOCKET_ADDR))
        return op == S_LOCAL_PORT ? java_net_ni(-1) : no(NULL);
    if (!s->bound && op == S_LOCAL_PORT) return java_net_ni(0);
    if (!s->bound && op == S_LOCAL_ADDR) {
        Address wildcard = {0}; (void)java_net_resolve_address(&wildcard, NULL, true);
        return no(java_net_address_copy(env, IA, &wildcard));
    }
    if (!s->connected && (op == S_PEER_ADDR || op == S_REMOTE_ADDR)) return no(NULL);
    if (!s->connected && op == S_PEER_PORT) return java_net_ni(-1);
    return java_net_socket_state(env, token, a, n, user);
}

static SsrnxResult java_net_datagram_register(void) {
    SsrnxResult rc = ssrnx_jni_register_class(UDP_SOCKET, "java/lang/Object");
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_interface(UDP_SOCKET, "java/io/Closeable");
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_class(UDP_PACKET, "java/lang/Object");
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_class("java/net/PortUnreachableException", "java/net/SocketException");
    static const struct { const char *cls, *name, *sig; SsrnxJniMethodCallback fn; intptr_t op; } entries[] = {
#define D(c,n,s,f,o) {c,n,s,f,o}
#define P(n,s,o) D(UDP_PACKET,n,s,java_net_datagram_packet_op,o)
#define V(n,s,o) D(UDP_SOCKET,n,s,java_net_datagram_state,o)
        D(UDP_PACKET,"<init>","([BI)V",java_net_datagram_packet_ctor,0),
        D(UDP_PACKET,"<init>","([BII)V",java_net_datagram_packet_ctor,1),
        D(UDP_PACKET,"<init>","([BILjava/net/InetAddress;I)V",java_net_datagram_packet_ctor,2),
        D(UDP_PACKET,"<init>","([BIILjava/net/InetAddress;I)V",java_net_datagram_packet_ctor,3),
        D(UDP_PACKET,"<init>","([BILjava/net/SocketAddress;)V",java_net_datagram_packet_ctor,4),
        D(UDP_PACKET,"<init>","([BIILjava/net/SocketAddress;)V",java_net_datagram_packet_ctor,5),
        P("getData","()[B",DP_DATA), P("getOffset","()I",DP_OFFSET), P("getLength","()I",DP_LENGTH),
        P("getAddress","()Ljava/net/InetAddress;",DP_ADDRESS), P("getPort","()I",DP_PORT),
        P("getSocketAddress","()Ljava/net/SocketAddress;",DP_SOCKET_ADDRESS),
        P("setData","([B)V",DP_SET_DATA), P("setData","([BII)V",DP_SET_RANGE), P("setLength","(I)V",DP_SET_LENGTH),
        P("setAddress","(Ljava/net/InetAddress;)V",DP_SET_ADDRESS), P("setPort","(I)V",DP_SET_PORT),
        P("setSocketAddress","(Ljava/net/SocketAddress;)V",DP_SET_SOCKET_ADDRESS),
        D(UDP_SOCKET,"<init>","()V",java_net_datagram_socket_ctor,0), D(UDP_SOCKET,"<init>","(I)V",java_net_datagram_socket_ctor,0),
        D(UDP_SOCKET,"<init>","(ILjava/net/InetAddress;)V",java_net_datagram_socket_ctor,0),
        D(UDP_SOCKET,"<init>","(Ljava/net/SocketAddress;)V",java_net_datagram_socket_ctor,1),
        D(UDP_SOCKET,"bind","(Ljava/net/SocketAddress;)V",java_net_datagram_bind,0),
        D(UDP_SOCKET,"connect","(Ljava/net/SocketAddress;)V",java_net_datagram_connect,0),
        D(UDP_SOCKET,"connect","(Ljava/net/InetAddress;I)V",java_net_datagram_connect,0),
        D(UDP_SOCKET,"disconnect","()V",java_net_datagram_disconnect,0),
        D(UDP_SOCKET,"send","(Ljava/net/DatagramPacket;)V",java_net_datagram_io,1),
        D(UDP_SOCKET,"receive","(Ljava/net/DatagramPacket;)V",java_net_datagram_io,0),
        D(UDP_SOCKET,"close","()V",java_net_socket_close,0), V("isClosed","()Z",S_CLOSED),
        V("isBound","()Z",S_BOUND), V("isConnected","()Z",S_CONNECTED),
        V("setSoTimeout","(I)V",S_TIMEOUT), V("getSoTimeout","()I",S_TIMEOUT),
        V("getLocalPort","()I",S_LOCAL_PORT), V("getPort","()I",S_PEER_PORT),
        V("getLocalAddress","()Ljava/net/InetAddress;",S_LOCAL_ADDR),
        V("getInetAddress","()Ljava/net/InetAddress;",S_PEER_ADDR),
        V("getLocalSocketAddress","()Ljava/net/SocketAddress;",S_LOCAL_SOCKET_ADDR),
        V("getRemoteSocketAddress","()Ljava/net/SocketAddress;",S_REMOTE_ADDR),
        V("setReuseAddress","(Z)V",S_REUSE), V("getReuseAddress","()Z",S_REUSE),
        V("getChannel","()Ljava/nio/channels/DatagramChannel;",S_CHANNEL),
#undef D
#undef P
#undef V
    };
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(entries)/sizeof(entries[0]); ++i)
        rc = ssrnx_jni_register_method(entries[i].cls, entries[i].name, entries[i].sig, false, entries[i].fn, (void *)entries[i].op);
    return rc;
}



typedef struct java_net_NetMethod {
    const char *cls, *name, *sig;
    bool st;
    SsrnxJniMethodCallback cb;
    intptr_t user;
} java_net_NetMethod;
#define M(c, n, s, cb) {c, n, s, false, cb, 0}
#define U(c, n, s, cb, u) {c, n, s, false, cb, u}
#define S(c, n, s, cb, u) {c, n, s, true, cb, u}
static const java_net_NetMethod methods[] = {
    M(SF, "<init>", "()V", nothing),
    S(SF, "getDefault", "()Ljavax/net/SocketFactory;", java_net_factory_default, 0),
    M(SF, "createSocket", "()Ljava/net/Socket;", java_net_factory_create),
    M(SF, "createSocket", "(Ljava/lang/String;I)Ljava/net/Socket;", java_net_factory_create),
    U(SF, "createSocket", "(Ljava/net/InetAddress;I)Ljava/net/Socket;", java_net_factory_create, 1),
    M(SF, "createSocket", "(Ljava/lang/String;ILjava/net/InetAddress;I)Ljava/net/Socket;", java_net_factory_create),
    U(SF, "createSocket", "(Ljava/net/InetAddress;ILjava/net/InetAddress;I)Ljava/net/Socket;", java_net_factory_create,
      1),
    M(DSF, "createSocket", "()Ljava/net/Socket;", java_net_factory_create),
    M(DSF, "createSocket", "(Ljava/lang/String;I)Ljava/net/Socket;", java_net_factory_create),
    U(DSF, "createSocket", "(Ljava/net/InetAddress;I)Ljava/net/Socket;", java_net_factory_create, 1),
    M(DSF, "createSocket", "(Ljava/lang/String;ILjava/net/InetAddress;I)Ljava/net/Socket;", java_net_factory_create),
    U(DSF, "createSocket", "(Ljava/net/InetAddress;ILjava/net/InetAddress;I)Ljava/net/Socket;",
      java_net_factory_create, 1),
    M(SSF, "<init>", "()V", nothing),
    S(SSF, "getDefault", "()Ljavax/net/SocketFactory;", java_net_factory_default, 1),
    U(SSF, "createSocket", "(Ljava/net/Socket;Ljava/lang/String;IZ)Ljava/net/Socket;", java_net_factory_create, 2),
    M(SSF, "getDefaultCipherSuites", "()[Ljava/lang/String;", java_net_factory_ciphers),
    M(SSF, "getSupportedCipherSuites", "()[Ljava/lang/String;", java_net_factory_ciphers),
    M(DSSF, "createSocket", "()Ljava/net/Socket;", java_net_factory_create),
    M(DSSF, "createSocket", "(Ljava/lang/String;I)Ljava/net/Socket;", java_net_factory_create),
    U(DSSF, "createSocket", "(Ljava/net/InetAddress;I)Ljava/net/Socket;", java_net_factory_create, 1),
    M(DSSF, "createSocket", "(Ljava/lang/String;ILjava/net/InetAddress;I)Ljava/net/Socket;", java_net_factory_create),
    U(DSSF, "createSocket", "(Ljava/net/InetAddress;ILjava/net/InetAddress;I)Ljava/net/Socket;",
      java_net_factory_create, 1),
    U(DSSF, "createSocket", "(Ljava/net/Socket;Ljava/lang/String;IZ)Ljava/net/Socket;", java_net_factory_create, 2),
    M(DSSF, "getDefaultCipherSuites", "()[Ljava/lang/String;", java_net_factory_ciphers),
    M(DSSF, "getSupportedCipherSuites", "()[Ljava/lang/String;", java_net_factory_ciphers),
    S(SC, "getInstance", "(Ljava/lang/String;)Ljavax/net/ssl/SSLContext;", java_net_context_get_instance, 0),
    S(SC, "getInstance", "(Ljava/lang/String;Ljava/lang/String;)Ljavax/net/ssl/SSLContext;",
      java_net_context_get_instance, 0),
    S(SC, "getInstance", "(Ljava/lang/String;Ljava/security/Provider;)Ljavax/net/ssl/SSLContext;",
      java_net_context_get_instance, 0),
    S(SC, "getDefault", "()Ljavax/net/ssl/SSLContext;", java_net_context_default, 0),
    S(SC, "setDefault", "(Ljavax/net/ssl/SSLContext;)V", java_net_context_default, 0),
    M(SC, "init",
      "([Ljavax/net/ssl/KeyManager;[Ljavax/net/ssl/TrustManager;Ljava/security/"
      "SecureRandom;)V",
      java_net_context_init),
    U(SC, "getSocketFactory", "()Ljavax/net/ssl/SSLSocketFactory;", java_net_context_value, C_FACTORY),
    U(SC, "getProtocol", "()Ljava/lang/String;", java_net_context_value, C_PROTOCOL),
    U(SC, "getProvider", "()Ljava/security/Provider;", java_net_context_value, C_PROVIDER),
    U(SC, "getDefaultSSLParameters", "()Ljavax/net/ssl/SSLParameters;", java_net_context_value, C_PARAMS),
    U(SC, "getSupportedSSLParameters", "()Ljavax/net/ssl/SSLParameters;", java_net_context_value, C_PARAMS),
    M(SOCK, "<init>", "()V", java_net_socket_ctor),
    M(SOCK, "<init>", "(Ljava/lang/String;I)V", java_net_socket_ctor),
    U(SOCK, "<init>", "(Ljava/net/InetAddress;I)V", java_net_socket_ctor, 1),
    M(SOCK, "<init>", "(Ljava/lang/String;ILjava/net/InetAddress;I)V", java_net_socket_ctor),
    U(SOCK, "<init>", "(Ljava/net/InetAddress;ILjava/net/InetAddress;I)V", java_net_socket_ctor, 1),
    M(SOCK, "connect", "(Ljava/net/SocketAddress;)V", java_net_socket_connect_call),
    M(SOCK, "connect", "(Ljava/net/SocketAddress;I)V", java_net_socket_connect_call),
    M(SOCK, "bind", "(Ljava/net/SocketAddress;)V", java_net_socket_bind),
    M(SOCK, "close", "()V", java_net_socket_close),
    M(SOCK, "getInputStream", "()Ljava/io/InputStream;", java_net_socket_stream),
    U(SOCK, "getOutputStream", "()Ljava/io/OutputStream;", java_net_socket_stream, 1),
    U(SOCK, "isConnected", "()Z", java_net_socket_state, S_CONNECTED),
    U(SOCK, "isBound", "()Z", java_net_socket_state, S_BOUND),
    U(SOCK, "isClosed", "()Z", java_net_socket_state, S_CLOSED),
    U(SOCK, "getSoTimeout", "()I", java_net_socket_state, S_TIMEOUT),
    U(SOCK, "setSoTimeout", "(I)V", java_net_socket_state, S_TIMEOUT),
    U(SOCK, "getTcpNoDelay", "()Z", java_net_socket_state, S_NODELAY),
    U(SOCK, "setTcpNoDelay", "(Z)V", java_net_socket_state, S_NODELAY),
    U(SOCK, "getKeepAlive", "()Z", java_net_socket_state, S_KEEPALIVE),
    U(SOCK, "setKeepAlive", "(Z)V", java_net_socket_state, S_KEEPALIVE),
    U(SOCK, "getReuseAddress", "()Z", java_net_socket_state, S_REUSE),
    U(SOCK, "setReuseAddress", "(Z)V", java_net_socket_state, S_REUSE),
    U(SOCK, "isInputShutdown", "()Z", java_net_socket_state, S_IN_SHUT),
    U(SOCK, "isOutputShutdown", "()Z", java_net_socket_state, S_OUT_SHUT),
    M(SOCK, "shutdownInput", "()V", java_net_socket_shutdown),
    U(SOCK, "shutdownOutput", "()V", java_net_socket_shutdown, 1),
    U(SOCK, "getPort", "()I", java_net_socket_state, S_PEER_PORT),
    U(SOCK, "getLocalPort", "()I", java_net_socket_state, S_LOCAL_PORT),
    U(SOCK, "getInetAddress", "()Ljava/net/InetAddress;", java_net_socket_state, S_PEER_ADDR),
    U(SOCK, "getLocalAddress", "()Ljava/net/InetAddress;", java_net_socket_state, S_LOCAL_ADDR),
    U(SOCK, "getRemoteSocketAddress", "()Ljava/net/SocketAddress;", java_net_socket_state, S_REMOTE_ADDR),
    U(SOCK, "getLocalSocketAddress", "()Ljava/net/SocketAddress;", java_net_socket_state, S_LOCAL_SOCKET_ADDR),
    U(SOCK, "getChannel", "()Ljava/nio/channels/SocketChannel;", java_net_socket_state, S_CHANNEL),
    M(IN, "read", "()I", java_net_stream_io),
    M(IN, "read", "([B)I", java_net_stream_io),
    M(IN, "read", "([BII)I", java_net_stream_io),
    M(IN, "available", "()I", java_net_stream_available),
    M(IN, "close", "()V", java_net_socket_close),
    U(OUT, "write", "(I)V", java_net_stream_io, 1),
    U(OUT, "write", "([B)V", java_net_stream_io, 1),
    U(OUT, "write", "([BII)V", java_net_stream_io, 1),
    M(OUT, "close", "()V", java_net_socket_close),
    M(OUT, "flush", "()V", nothing),
    M(ISA, "<init>", "(I)V", java_net_address_ctor),
    M(ISA, "<init>", "(Ljava/lang/String;I)V", java_net_address_ctor),
    U(ISA, "<init>", "(Ljava/net/InetAddress;I)V", java_net_address_ctor, 1),
    U(ISA, "getPort", "()I", java_net_address_get, A_PORT),
    U(ISA, "isUnresolved", "()Z", java_net_address_get, A_UNRESOLVED),
    U(ISA, "getHostName", "()Ljava/lang/String;", java_net_address_get, A_HOST),
    U(ISA, "getHostString", "()Ljava/lang/String;", java_net_address_get, A_HOST),
    U(ISA, "getAddress", "()Ljava/net/InetAddress;", java_net_address_get, A_ADDRESS),
    S(IA, "getByName", "(Ljava/lang/String;)Ljava/net/InetAddress;", java_net_inet_get, 0),
    U(IA4, "getHostAddress", "()Ljava/lang/String;", java_net_address_get, A_NUMERIC),
    U(IA4, "getHostName", "()Ljava/lang/String;", java_net_address_get, A_HOST),
    U(IA4, "isLoopbackAddress", "()Z", java_net_address_get, A_LOOPBACK),
    M(PARAM, "<init>", "()V", java_net_params_ctor),
    M(PARAM, "<init>", "([Ljava/lang/String;)V", java_net_params_ctor),
    M(PARAM, "<init>", "([Ljava/lang/String;[Ljava/lang/String;)V", java_net_params_ctor),
    U(PARAM, "getCipherSuites", "()[Ljava/lang/String;", java_net_params_access, P_CIPHERS),
    U(PARAM, "setCipherSuites", "([Ljava/lang/String;)V", java_net_params_access, P_CIPHERS),
    U(PARAM, "getProtocols", "()[Ljava/lang/String;", java_net_params_access, P_PROTOCOLS),
    U(PARAM, "setProtocols", "([Ljava/lang/String;)V", java_net_params_access, P_PROTOCOLS),
    U(PARAM, "getApplicationProtocols", "()[Ljava/lang/String;", java_net_params_access, P_ALPN),
    U(PARAM, "setApplicationProtocols", "([Ljava/lang/String;)V", java_net_params_access, P_ALPN),
    U(PARAM, "getEndpointIdentificationAlgorithm", "()Ljava/lang/String;", java_net_params_access, P_ENDPOINT),
    U(PARAM, "setEndpointIdentificationAlgorithm", "(Ljava/lang/String;)V", java_net_params_access, P_ENDPOINT),
    U(PARAM, "getNeedClientAuth", "()Z", java_net_params_access, P_NEED_AUTH),
    U(PARAM, "setNeedClientAuth", "(Z)V", java_net_params_access, P_NEED_AUTH),
    U(PARAM, "getWantClientAuth", "()Z", java_net_params_access, P_WANT_AUTH),
    U(PARAM, "setWantClientAuth", "(Z)V", java_net_params_access, P_WANT_AUTH),
    U(SSL, "startHandshake", "()V", java_net_tls_access, T_HANDSHAKE),
    U(SSL, "getSession", "()Ljavax/net/ssl/SSLSession;", java_net_tls_access, T_SESSION),
    U(SSL, "getSSLParameters", "()Ljavax/net/ssl/SSLParameters;", java_net_tls_access, T_PARAMS),
    U(SSL, "setSSLParameters", "(Ljavax/net/ssl/SSLParameters;)V", java_net_tls_access, T_PARAMS),
    U(SSL, "getSupportedCipherSuites", "()[Ljava/lang/String;", java_net_tls_access, T_SUPPORTED_CIPHERS),
    U(SSL, "getSupportedProtocols", "()[Ljava/lang/String;", java_net_tls_access, T_SUPPORTED_PROTOCOLS),
    U(SSL, "getEnabledCipherSuites", "()[Ljava/lang/String;", java_net_tls_access, T_CIPHERS),
    U(SSL, "setEnabledCipherSuites", "([Ljava/lang/String;)V", java_net_tls_access, T_CIPHERS),
    U(SSL, "getEnabledProtocols", "()[Ljava/lang/String;", java_net_tls_access, T_PROTOCOLS),
    U(SSL, "setEnabledProtocols", "([Ljava/lang/String;)V", java_net_tls_access, T_PROTOCOLS),
    U(SSL, "getApplicationProtocol", "()Ljava/lang/String;", java_net_tls_access, T_APPLICATION),
    U(SSL, "getUseClientMode", "()Z", java_net_tls_access, T_CLIENT),
    U(SSL, "setUseClientMode", "(Z)V", java_net_tls_access, T_CLIENT),
    U(SSL, "getNeedClientAuth", "()Z", java_net_tls_access, T_NEED),
    U(SSL, "setNeedClientAuth", "(Z)V", java_net_tls_access, T_NEED),
    U(SSL, "getWantClientAuth", "()Z", java_net_tls_access, T_WANT),
    U(SSL, "setWantClientAuth", "(Z)V", java_net_tls_access, T_WANT),
    U(SSL, "getEnableSessionCreation", "()Z", java_net_tls_access, T_CREATE_SESSION),
    U(SSL, "setEnableSessionCreation", "(Z)V", java_net_tls_access, T_CREATE_SESSION),
    U(SESSION, "getProtocol", "()Ljava/lang/String;", java_net_session_value, SE_PROTOCOL),
    U(SESSION, "getCipherSuite", "()Ljava/lang/String;", java_net_session_value, SE_CIPHER),
    U(SESSION, "getPeerHost", "()Ljava/lang/String;", java_net_session_value, SE_PEER_HOST),
    U(SESSION, "getPeerPort", "()I", java_net_session_value, SE_PEER_PORT),
    U(SESSION, "getPeerCertificates", "()[Ljava/security/cert/Certificate;", java_net_session_value, SE_PEER_CERTS),
    U(SESSION, "getLocalCertificates", "()[Ljava/security/cert/Certificate;", java_net_session_value, SE_LOCAL_CERTS),
    U(SESSION, "isValid", "()Z", java_net_session_value, SE_VALID),
    U(SESSION, "getId", "()[B", java_net_session_value, SE_ID),
    U(SESSION, "getPacketBufferSize", "()I", java_net_session_value, SE_PACKET),
    U(SESSION, "getApplicationBufferSize", "()I", java_net_session_value, SE_APPLICATION),
    U(PROVIDER, "getName", "()Ljava/lang/String;", java_net_provider_value, 0),
    U(PROVIDER, "getInfo", "()Ljava/lang/String;", java_net_provider_value, 1),
    U(PROVIDER, "getVersion", "()D", java_net_provider_value, 2),
};
#undef M
#undef U
#undef S
SsrnxResult ssrnx_java_net_register(void) {
    static const struct {
        const char *name, *super;
    } classes[] = {{SF, "java/lang/Object"},
                   {DSF, SF},
                   {SSF, SF},
                   {DSSF, SSF},
                   {SC, "java/lang/Object"},
                   {"javax/net/ssl/KeyManager", "java/lang/Object"},
                   {SOCK, "java/lang/Object"},
                   {SSL, SOCK},
                   {TLS_SOCK, SSL},
                   {PARAM, "java/lang/Object"},
                   {"javax/net/ssl/SSLSession", "java/lang/Object"},
                   {SESSION, "java/lang/Object"},
                   {IN, "java/io/InputStream"},
                   {OUT, "java/io/OutputStream"},
                   {"java/net/SocketAddress", "java/lang/Object"},
                   {ISA, "java/net/SocketAddress"},
                   {IA, "java/lang/Object"},
                   {"java/net/Inet6Address", IA},
                   {IA4, "java/net/Inet4Address"},
                   {IA6, "java/net/Inet6Address"},
                   {"java/net/SocketException", "java/io/IOException"},
                   {"java/net/ConnectException", "java/net/SocketException"},
                   {"java/net/BindException", "java/net/SocketException"},
                   {"java/io/InterruptedIOException", "java/io/IOException"},
                   {"java/net/SocketTimeoutException", "java/io/InterruptedIOException"},
                   {"javax/net/ssl/SSLException", "java/io/IOException"},
                   {"javax/net/ssl/SSLHandshakeException", "javax/net/ssl/SSLException"},
                   {"javax/net/ssl/SSLPeerUnverifiedException", "javax/net/ssl/SSLException"},
                   {"java/security/NoSuchProviderException", "java/security/GeneralSecurityException"},
                   {"java/security/KeyManagementException", "java/security/KeyException"}};
    SsrnxResult rc = SSRNX_OK;
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(classes) / sizeof(classes[0]); ++i) {
        rc = ssrnx_jni_register_class(classes[i].name, classes[i].super);
        if (rc != SSRNX_OK)
            SSRNX_LOGCE("NET", "Class registration failed class=%s rc=%d", classes[i].name, (int)rc);
    }
    if (rc == SSRNX_OK)
        rc = ssrnx_jni_register_interface(SESSION, "javax/net/ssl/SSLSession");
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(methods) / sizeof(methods[0]); ++i) {
        const java_net_NetMethod *m = &methods[i];
        if (!strcmp(m->cls, PROVIDER))
            continue;
        rc = ssrnx_jni_register_method(m->cls, m->name, m->sig, m->st, m->cb, (void *)m->user);
        if (rc == SSRNX_OK && !strcmp(m->cls, SESSION))
            rc = ssrnx_jni_register_interface_method("javax/net/ssl/SSLSession", m->name, m->sig);
        if (rc != SSRNX_OK)
            SSRNX_LOGCE("NET", "Method registration failed class=%s method=%s rc=%d", m->cls, m->name,
                        (int)rc);
    }
    static const struct {
        const char *name, *sig;
        intptr_t op;
    } address_methods[] = {{"getHostAddress", "()Ljava/lang/String;", A_NUMERIC},
                           {"getHostName", "()Ljava/lang/String;", A_HOST},
                           {"getAddress", "()[B", A_BYTES},
                           {"isLoopbackAddress", "()Z", A_LOOPBACK},
                           {"isAnyLocalAddress", "()Z", A_ANY},
                           {"equals", "(Ljava/lang/Object;)Z", A_EQUALS},
                           {"hashCode", "()I", A_HASH}};
    const char *address_classes[] = {IA4, IA6};
    for (size_t j = 0; j < 2 && rc == SSRNX_OK; ++j)
        for (size_t i = 0; i < sizeof(address_methods) / sizeof(address_methods[0]) && rc == SSRNX_OK; ++i)
            rc =
                ssrnx_jni_register_method(address_classes[j], address_methods[i].name, address_methods[i].sig,
                                          false, java_net_address_get, (void *)address_methods[i].op);
    if (rc == SSRNX_OK)
        rc = ssrnx_jni_register_method(IA, "getAllByName", "(Ljava/lang/String;)[Ljava/net/InetAddress;",
                                       true, java_net_inet_get, (void *)1);
    if (rc == SSRNX_OK)
        rc = ssrnx_jni_register_method(ISA, "createUnresolved",
                                       "(Ljava/lang/String;I)Ljava/net/InetSocketAddress;", true,
                                       java_net_address_unresolved, NULL);
    return rc == SSRNX_OK ? java_net_datagram_register() : rc;
}
void ssrnx_java_net_reset(void) {
    void *env = ssrnx_jni_current_env();
    for (java_net_NetObject *p = objects; p; p = p->next)
        if (p->kind == N_SOCKET)
            java_net_close_socket((java_net_SocketObject *)p);
    while (objects) {
        java_net_NetObject *p = objects;
        objects = p->next;
        if (p->kind == N_ADDRESS)
            free(((Address *)p)->host);
        else if (p->kind == N_SOCKET) {
            java_net_SocketObject *s = (java_net_SocketObject *)p;
            mbedtls_ssl_free(&s->ssl);
            mbedtls_ssl_session_free(&s->session);
            mbedtls_ssl_config_free(&s->config);
            mbedtls_ctr_drbg_free(&s->rng);
            free(s->peer_host);
            free(s->cipher_ids);
            java_net_free_texts(s->alpn_names);
        } else if (p->kind == N_PACKET) {
            java_net_DatagramPacketPayload *packet = (java_net_DatagramPacketPayload *)p;
            java_net_replace_ref(env, &packet->data, NULL);
            java_net_replace_ref(env, &packet->address, NULL);
            pthread_mutex_destroy(&packet->lock);
        } else if (p->kind == N_PARAMS) {
            java_net_Parameters *q = (java_net_Parameters *)p;
            java_net_replace_ref(env, &q->ciphers, NULL);
            java_net_replace_ref(env, &q->protocols, NULL);
            java_net_replace_ref(env, &q->alpn, NULL);
            free(q->endpoint);
        }
        free(p);
    }
    while (java_net_transports) {
        java_net_Transport *t = java_net_transports;
        java_net_transports = t->next;
        if (t->fd >= 0)
            close(t->fd);
        pthread_mutex_destroy(&t->io);
        pthread_mutex_destroy(&t->read);
        pthread_mutex_destroy(&t->write);
        free(t);
    }
    while (java_net_materials) {
        java_net_TlsMaterial *m = java_net_materials;
        java_net_materials = m->next;
        mbedtls_x509_crt_free(&m->roots);
        pthread_mutex_destroy(&m->verify);
        free(m);
    }
    java_net_replace_ref(env, &java_net_default_factory, NULL);
    java_net_replace_ref(env, &default_context, NULL);
    java_net_replace_ref(env, &provider, NULL);
}

#pragma pop_macro("V")
#pragma pop_macro("UDP_SOCKET")
#pragma pop_macro("UDP_PACKET")
#pragma pop_macro("U")
#pragma pop_macro("TLS_SOCK")
#pragma pop_macro("SSL")
#pragma pop_macro("SSF")
#pragma pop_macro("SOCK")
#pragma pop_macro("SF")
#pragma pop_macro("SESSION")
#pragma pop_macro("SC")
#pragma pop_macro("S")
#pragma pop_macro("PROVIDER")
#pragma pop_macro("PARAM")
#pragma pop_macro("P")
#pragma pop_macro("OUT")
#pragma pop_macro("NET_MAGIC")
#pragma pop_macro("MSG_NOSIGNAL")
#pragma pop_macro("M")
#pragma pop_macro("ISA")
#pragma pop_macro("IN")
#pragma pop_macro("IA6")
#pragma pop_macro("IA4")
#pragma pop_macro("IA")
#pragma pop_macro("DSSF")
#pragma pop_macro("DSF")
#pragma pop_macro("D")


/* ---- java_http ---- */
#pragma push_macro("SET")
/* Private host HTTP transfers; no host descriptors or handles enter guest fd
 * tables. libcurl handles HTTP framing and the platform TLS backend verifies
 * chain/name. */





#include <curl/curl.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
extern size_t ssrnx_compat_network_ca_count(void) __attribute__((weak));
extern bool ssrnx_compat_network_ca_copy(size_t, uint8_t *, size_t, size_t *)
    __attribute__((weak));

struct SsrnxHttpTransfer {
  CURL *easy;
  CURLM *multi;
  struct curl_slist *request_headers;
  SsrnxHttpHeader *headers;
  uint8_t *body;
  size_t body_size, position, capacity;
  int status, read_timeout;
  bool headers_ready, done, established, automatic_gzip, memory_error,
      timed_out;
  _Atomic bool canceled;
  pthread_mutex_t io;
  int64_t last_activity;
  CURLcode result;
  char error[CURL_ERROR_SIZE];
  mbedtls_x509_crt roots;
  char *peer_host, *url;
};
static pthread_once_t java_http_http_once = PTHREAD_ONCE_INIT;
static CURLcode java_http_http_init_result;
static void java_http_http_init(void) {
  java_http_http_init_result = curl_global_init(CURL_GLOBAL_DEFAULT);
}
static int64_t java_http_now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
static void java_http_free_headers(SsrnxHttpHeader *head) {
  while (head) {
    SsrnxHttpHeader *next = head->next;
    free(head->key);
    free(head->value);
    free(head);
    head = next;
  }
}
static size_t java_http_receive_body(char *data, size_t size, size_t count,
                           void *opaque) {
  SsrnxHttpTransfer *t = opaque;
  if (size && count > SIZE_MAX / size)
    return 0;
  size_t n = size * count;
  if (n > INT32_MAX || t->body_size > INT32_MAX - n) {
    t->memory_error = true;
    return 0;
  }
  /* Compact consumed bytes; response code returns at headers and reads pump
   * the transfer, so an entire response need not fit in memory. */
  if (t->position) {
    memmove(t->body, t->body + t->position, t->body_size - t->position);
    t->body_size -= t->position;
    t->position = 0;
  }
  size_t needed = t->body_size + n;
  if (needed > t->capacity) {
    size_t capacity = needed > t->capacity * 2 ? needed : t->capacity * 2;
    uint8_t *grown = realloc(t->body, capacity);
    if (!grown) {
      t->memory_error = true;
      return 0;
    }
    t->body = grown;
    t->capacity = capacity;
  }
  if (n)
    memcpy(t->body + t->body_size, data, n);
  t->body_size += n;
  t->last_activity = java_http_now_ms();
  return n;
}
static size_t java_http_receive_header(char *data, size_t size, size_t count,
                             void *opaque) {
  SsrnxHttpTransfer *t = opaque;
  if (size && count > SIZE_MAX / size)
    return 0;
  size_t n = size * count, length = n;
  t->last_activity = java_http_now_ms();
  while (length && (data[length - 1] == '\r' || data[length - 1] == '\n'))
    --length;
  if (!length) {
    if (t->status >= 200) {
      /* The lookup hides transparently decoded gzip. Check the original
       * headers and remove transport metadata from the Java view. */
      bool gzip = false;
      for (const SsrnxHttpHeader *h = t->headers; h; h = h->next)
        if (h->key && !strcasecmp(h->key, "Content-Encoding") &&
            !strcasecmp(h->value, "gzip"))
          gzip = true;
      if (t->automatic_gzip && gzip) {
        for (SsrnxHttpHeader **h = &t->headers; *h;) {
          if ((*h)->key && (!strcasecmp((*h)->key, "Content-Encoding") ||
                            !strcasecmp((*h)->key, "Content-Length"))) {
            SsrnxHttpHeader *dead = *h;
            *h = dead->next;
            dead->next = NULL;
            java_http_free_headers(dead);
          } else
            h = &(*h)->next;
        }
      }
      t->headers_ready = true;
    }
    return n;
  }
  bool status = length >= 5 && !memcmp(data, "HTTP/", 5);
  if (status) {
    java_http_free_headers(t->headers);
    t->headers = NULL;
    t->headers_ready = false;
  }
  const char *colon = status ? NULL : memchr(data, ':', length);
  if (!status && !colon)
    return n;
  SsrnxHttpHeader *h = calloc(1, sizeof(*h));
  if (!h) {
    t->memory_error = true;
    return 0;
  }
  if (status) {
    h->value = strndup(data, length);
    if (h->value) {
      const char *space = strchr(h->value, ' ');
      t->status = space ? atoi(space + 1) : 0;
    }
  } else {
    h->key = strndup(data, (size_t)(colon - data));
    const char *value = colon + 1, *end = data + length;
    while (value < end && (*value == ' ' || *value == '\t'))
      ++value;
    while (end > value && (end[-1] == ' ' || end[-1] == '\t'))
      --end;
    h->value = strndup(value, (size_t)(end - value));
  }
  if (!h->value || (!status && !h->key)) {
    java_http_free_headers(h);
    t->memory_error = true;
    return 0;
  }
  SsrnxHttpHeader **tail = &t->headers;
  while (*tail)
    tail = &(*tail)->next;
  *tail = h;
  return n;
}
/* Host and Switch HTTP use the same Mbed TLS backend and platform-root bridge. */
static int java_http_verify_http_peer(void *opaque,mbedtls_x509_crt *cert,int depth,uint32_t *flags) {
  SsrnxHttpTransfer *t=opaque;
  if(depth==0)ssrnx_tls_verify_ip_san(cert,t->peer_host,flags);
  return 0;
}
static CURLcode java_http_host_ssl_context(CURL *easy, void *context, void *opaque) {
  (void)easy;
  SsrnxHttpTransfer *t = opaque;
  mbedtls_ssl_conf_ca_chain(context, &t->roots, NULL);
  mbedtls_ssl_conf_verify(context,java_http_verify_http_peer,t);
  return CURLE_OK;
}
SsrnxHttpTransfer *ssrnx_http_create(const SsrnxHttpRequest *r) {
  SsrnxHttpTransfer *t = calloc(1, sizeof(*t));
  if (!t) return NULL;
  atomic_init(&t->canceled, false);
  pthread_mutex_init(&t->io, NULL);
  mbedtls_x509_crt_init(&t->roots);
  t->url = strdup(r->url);
  if (!t->url) { ssrnx_http_destroy(t); return NULL; }
#ifdef __SWITCH__
  if (ssrnx_compat_network_initialize() != 0) {
    t->done=true;t->result=CURLE_FAILED_INIT;
    snprintf(t->error,sizeof(t->error),"Platform network initialization failed");
    return t;
  }
#endif
  pthread_once(&java_http_http_once, java_http_http_init);
  if (java_http_http_init_result != CURLE_OK) {t->done=true;t->result=java_http_http_init_result;return t;}
  size_t roots =
      ssrnx_compat_network_ca_count ? ssrnx_compat_network_ca_count() : 0;
  for (size_t i = 0; i < roots; ++i) {
    size_t size = 0;
    if (!ssrnx_compat_network_ca_copy ||
        !ssrnx_compat_network_ca_copy(i, NULL, 0, &size))
      continue;
    uint8_t *der = malloc(size);
    if (!der) {
      ssrnx_http_destroy(t);
      return NULL;
    }
    if (ssrnx_compat_network_ca_copy(i, der, size, &size))
      (void)mbedtls_x509_crt_parse_der(&t->roots, der, size);
    free(der);
  }
  t->easy = curl_easy_init();
  t->multi = curl_multi_init();
  if (!t->easy || !t->multi) {
    ssrnx_http_destroy(t);
    return NULL;
  }
  t->read_timeout = r->read_timeout;
  CURLU *url=curl_url();
  if(url&&curl_url_set(url,CURLUPART_URL,r->url,0)==CURLUE_OK)
    (void)curl_url_get(url,CURLUPART_HOST,&t->peer_host,0);
  curl_url_cleanup(url);
  t->last_activity = java_http_now_ms();
  bool explicit_encoding = false;
  for (const SsrnxHttpHeader *h = r->headers; h; h = h->next) {
    if (!h->key || !h->value)
      continue;
    if (r->strip_sensitive && (!strcasecmp(h->key, "Authorization") ||
                               !strcasecmp(h->key, "Cookie") ||
                               !strcasecmp(h->key, "Proxy-Authorization")))
      continue;
    if (!strcasecmp(h->key, "Accept-Encoding"))
      explicit_encoding = true;
    size_t size = strlen(h->key) + strlen(h->value) + 3;
    char *line = malloc(size);
    if (!line) {
      ssrnx_http_destroy(t);
      return NULL;
    }
    snprintf(line, size, "%s: %s", h->key, h->value);
    struct curl_slist *headers = curl_slist_append(t->request_headers, line);
    free(line);
    if (!headers) {
      ssrnx_http_destroy(t);
      return NULL;
    }
    t->request_headers = headers;
  }
  if (r->chunked && r->has_body) {
    struct curl_slist *headers =
        curl_slist_append(t->request_headers, "Transfer-Encoding: chunked");
    if (!headers) {
      ssrnx_http_destroy(t);
      return NULL;
    }
    t->request_headers = headers;
  }
#define SET(option, value)                                                     \
  do {                                                                         \
    if (curl_easy_setopt(t->easy, option, value) != CURLE_OK) {                \
      ssrnx_http_destroy(t);                                                   \
      return NULL;                                                             \
    }                                                                          \
  } while (0)
  SET(CURLOPT_URL, r->url);
  SET(CURLOPT_NOSIGNAL, 1L);
  SET(CURLOPT_PROXY,
      ""); /* Host environment proxy settings are not Android policy. */
#if LIBCURL_VERSION_NUM >= 0x075500
  SET(CURLOPT_PROTOCOLS_STR, "http,https");
#else
  SET(CURLOPT_PROTOCOLS, (long)(CURLPROTO_HTTP | CURLPROTO_HTTPS));
#endif
  SET(CURLOPT_SSL_VERIFYPEER, 1L);
  SET(CURLOPT_SSL_VERIFYHOST, 2L);
  SET(CURLOPT_CONNECTTIMEOUT_MS,
      r->connect_timeout ? (long)r->connect_timeout : LONG_MAX / 1000);
  SET(CURLOPT_WRITEFUNCTION, java_http_receive_body);
  SET(CURLOPT_WRITEDATA, t);
  SET(CURLOPT_HEADERFUNCTION, java_http_receive_header);
  SET(CURLOPT_HEADERDATA, t);
  SET(CURLOPT_ERRORBUFFER, t->error);
  SET(CURLOPT_HTTPHEADER, t->request_headers);
  SET(CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_1_1);
  if (!explicit_encoding) {
    SET(CURLOPT_ACCEPT_ENCODING, "gzip");
    t->automatic_gzip = true;
  } else
    SET(CURLOPT_HTTP_CONTENT_DECODING, 0L);
  if (r->has_body) {
    SET(CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)r->body_size);
    SET(CURLOPT_COPYPOSTFIELDS, r->body_size ? (const char *)r->body : "");
  }
  if (!strcmp(r->method, "HEAD"))
    SET(CURLOPT_NOBODY, 1L);
  SET(CURLOPT_CUSTOMREQUEST, r->method);
  SET(CURLOPT_CAINFO, NULL);
  SET(CURLOPT_CAPATH, NULL);
  SET(CURLOPT_SSL_CTX_FUNCTION, java_http_host_ssl_context);
  SET(CURLOPT_SSL_CTX_DATA, t);
#undef SET
  if (curl_multi_add_handle(t->multi, t->easy) != CURLM_OK) {
    ssrnx_http_destroy(t);
    return NULL;
  }
  return t;
}
/* Each transfer owns its curl connection cache, so completion must retire it.
 * Response headers/body remain Java-visible after the network handles close. */
static void java_http_retire_transport(SsrnxHttpTransfer *t) {
  if (t->multi && t->easy) curl_multi_remove_handle(t->multi, t->easy);
  if (t->easy) curl_easy_cleanup(t->easy);
  if (t->multi) curl_multi_cleanup(t->multi);
  t->easy = NULL; t->multi = NULL;
}
/* Caller holds io; cancellation is published before waiting for this lock. */
static bool java_http_pump_locked(SsrnxHttpTransfer *t, bool headers) {
  /* Java read timeout starts when a caller blocks, not while the caller is
   * processing bytes returned by a previous read. */
  t->last_activity = java_http_now_ms();
  while (!t->done &&
         !(headers ? t->headers_ready : t->body_size > t->position)) {
    if (atomic_load(&t->canceled) ||
        ssrnx_jni_process_exit_requested(NULL, 0)) {
      t->result = CURLE_ABORTED_BY_CALLBACK;
      t->done = true;
      break;
    }
    int running = 0;
    if (curl_multi_perform(t->multi, &running) != CURLM_OK) {
      t->result = CURLE_RECV_ERROR;
      t->done = true;
      break;
    }
    int messages = 0;
    CURLMsg *message;
    while ((message = curl_multi_info_read(t->multi, &messages))) {
      if (message->msg == CURLMSG_DONE) {
        t->result = message->data.result;
        t->done = true;
      }
    }
    if (t->done || (headers ? t->headers_ready : t->body_size > t->position))
      break;
    double connected = 0;
    curl_easy_getinfo(t->easy, CURLINFO_CONNECT_TIME, &connected);
    if (!t->established && connected > 0) {
      t->established = true;
      t->last_activity = java_http_now_ms();
    }
    if (t->established && t->read_timeout &&
        java_http_now_ms() - t->last_activity >= t->read_timeout) {
      t->timed_out = true;
      t->result = CURLE_OPERATION_TIMEDOUT;
      t->done = true;
      break;
    }
    int fds = 0;
    if (curl_multi_wait(t->multi, NULL, 0, 20, &fds) != CURLM_OK) {
      t->result = CURLE_RECV_ERROR;
      t->done = true;
      break;
    }
    if (!fds) {
      struct timespec pause = {0, 1000000};
      nanosleep(&pause, NULL);
    }
  }
  if (t->done) {
    if (t->easy && t->result != CURLE_OK) {
      long native_errno = 0;
      (void)curl_easy_getinfo(t->easy, CURLINFO_OS_ERRNO, &native_errno);
      SSRNX_LOGCW("NET", "HTTP transport failure host=%s curl=%d errno=%ld reason=%s",
                  t->peer_host ? t->peer_host : "-", (int)t->result, native_errno,
                  t->error[0] ? t->error : curl_easy_strerror(t->result));
    }
    java_http_retire_transport(t);
  }
  return headers ? t->headers_ready
                 : t->body_size > t->position || t->result == CURLE_OK;
}
bool ssrnx_http_response(SsrnxHttpTransfer *t) {
  if (!t) return false;
  pthread_mutex_lock(&t->io);
  bool ready = !atomic_load(&t->canceled) && java_http_pump_locked(t, true);
  pthread_mutex_unlock(&t->io);
  return ready;
}
bool ssrnx_http_connect_probe(SsrnxHttpTransfer *t) {
  if (!t) return false;
  pthread_mutex_lock(&t->io);
  bool ok = t->easy && !atomic_load(&t->canceled) &&
            curl_easy_setopt(t->easy, CURLOPT_CONNECT_ONLY, 1L) == CURLE_OK;
  if (ok) { (void)java_http_pump_locked(t, false); ok = t->done && t->result == CURLE_OK; }
  pthread_mutex_unlock(&t->io);
  return ok;
}
int32_t ssrnx_http_read(SsrnxHttpTransfer *t, uint8_t *out, size_t size) {
  if (!size) return 0;
  if (!t) return -2;
  pthread_mutex_lock(&t->io);
  int32_t count = -2;
  if (!atomic_load(&t->canceled) && java_http_pump_locked(t, false)) {
    size_t available = t->body_size - t->position;
    if (!available) count = -1;
    else {
      if (size > available) size = available;
      memcpy(out, t->body + t->position, size);
      t->position += size; count = (int32_t)size;
    }
  }
  pthread_mutex_unlock(&t->io);
  return count;
}
void ssrnx_http_cancel(SsrnxHttpTransfer *t) {
  if (!t) return;
  atomic_store(&t->canceled, true);
  pthread_mutex_lock(&t->io);
  if (!t->done) { t->done = true; t->result = CURLE_ABORTED_BY_CALLBACK; }
  java_http_retire_transport(t);
  pthread_mutex_unlock(&t->io);
}
void ssrnx_http_destroy(SsrnxHttpTransfer *t) {
  if (!t) return;
  ssrnx_http_cancel(t);
  curl_slist_free_all(t->request_headers);
  java_http_free_headers(t->headers);
  free(t->body); free(t->url);
  mbedtls_x509_crt_free(&t->roots);
  curl_free(t->peer_host);
  pthread_mutex_destroy(&t->io);
  free(t);
}
int ssrnx_http_status(const SsrnxHttpTransfer *t) { return t ? t->status : -1; }
const SsrnxHttpHeader *ssrnx_http_headers(const SsrnxHttpTransfer *t) {
  return t ? t->headers : NULL;
}
const char *ssrnx_http_header(const SsrnxHttpTransfer *t, const char *key) {
  const char *value = NULL;
  for (const SsrnxHttpHeader *h = t ? t->headers : NULL; h; h = h->next) {
    if ((!key && !h->key) || (key && h->key && !strcasecmp(key, h->key)))
      value = h->value;
  }
  if (t && t->automatic_gzip && key &&
      (!strcasecmp(key, "Content-Encoding") ||
       !strcasecmp(key, "Content-Length"))) {
    for (const SsrnxHttpHeader *h = t->headers; h; h = h->next)
      if (h->key && !strcasecmp(h->key, "Content-Encoding") &&
          !strcasecmp(h->value, "gzip"))
        return NULL;
  }
  return value;
}
const char *ssrnx_http_exception(const SsrnxHttpTransfer *t) {
  if (!t || t->memory_error)
    return "java/lang/OutOfMemoryError";
  if (t->result == CURLE_COULDNT_RESOLVE_HOST)
    return "java/net/UnknownHostException";
  if (t->result == CURLE_COULDNT_CONNECT)
    return "java/net/ConnectException";
  if (t->result == CURLE_OPERATION_TIMEDOUT)
    return "java/net/SocketTimeoutException";
  if (t->result == CURLE_PEER_FAILED_VERIFICATION ||
      t->result == CURLE_SSL_CONNECT_ERROR ||
      t->result == CURLE_SSL_CACERT_BADFILE)
    return "javax/net/ssl/SSLHandshakeException";
  return "java/io/IOException";
}
const char *ssrnx_http_error(const SsrnxHttpTransfer *t) {
  return !t             ? "HTTP transfer allocation failed"
         : t->timed_out ? "Read timed out"
         : t->error[0]  ? t->error
                        : curl_easy_strerror(t->result);
}
char *ssrnx_http_redirect_url(const SsrnxHttpTransfer *t) {
  const char *location = ssrnx_http_header(t, "Location");
  char *redirect = NULL, *out = NULL;
  if (!location || !t->url) return NULL;
  /* Redirects are followed by URLConnection, never inside this transfer. */
  const char *base = t->url;
  CURLU *url = curl_url();
  if (url && curl_url_set(url, CURLUPART_URL, base, 0) == CURLUE_OK &&
      curl_url_set(url, CURLUPART_URL, location, 0) == CURLUE_OK &&
      curl_url_get(url, CURLUPART_URL, &redirect, 0) == CURLUE_OK)
    out = strdup(redirect);
  curl_free(redirect);
  curl_url_cleanup(url);
  return out;
}
bool ssrnx_http_same_origin(const char *left, const char *right) {
  CURLU *a = curl_url(), *b = curl_url();
  bool same = a && b;
  if (same)
    same = curl_url_set(a, CURLUPART_URL, left, 0) == CURLUE_OK &&
           curl_url_set(b, CURLUPART_URL, right, 0) == CURLUE_OK;
  const CURLUPart parts[] = {CURLUPART_SCHEME, CURLUPART_HOST, CURLUPART_PORT};
  for (size_t i = 0; same && i < 3; i++) {
    char *x = NULL, *y = NULL;
    same = curl_url_get(a, parts[i], &x, CURLU_DEFAULT_PORT) == CURLUE_OK &&
           curl_url_get(b, parts[i], &y, CURLU_DEFAULT_PORT) == CURLUE_OK &&
           !strcasecmp(x, y);
    curl_free(x);
    curl_free(y);
  }
  curl_url_cleanup(a);
  curl_url_cleanup(b);
  return same;
}

#pragma pop_macro("SET")
