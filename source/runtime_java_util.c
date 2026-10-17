/* runtime java util. */
#define _GNU_SOURCE 1
#include "runtime.h"


/* ---- java_text ---- */


#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- growable UTF-8 buffer ---------- */

typedef struct java_text_TextBuf {
    char *data;
    size_t len, cap;
    bool failed;
} java_text_TextBuf;

static bool java_text_buf_reserve(java_text_TextBuf *b, size_t extra) {
    if (b->failed) return false;
    if (b->len + extra + 1u <= b->cap) return true;
    size_t cap = b->cap ? b->cap : 64u;
    while (cap < b->len + extra + 1u) cap *= 2u;
    char *d = realloc(b->data, cap);
    if (!d) { b->failed = true; return false; }
    b->data = d;
    b->cap = cap;
    return true;
}
static void java_text_buf_append_n(java_text_TextBuf *b, const char *s, size_t n) {
    if (!n || !java_text_buf_reserve(b, n)) return;
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}
static void java_text_buf_append(java_text_TextBuf *b, const char *s) { java_text_buf_append_n(b, s, strlen(s)); }
static void java_text_buf_repeat(java_text_TextBuf *b, char c, size_t n) {
    if (!n || !java_text_buf_reserve(b, n)) return;
    memset(b->data + b->len, c, n);
    b->len += n;
    b->data[b->len] = '\0';
}

/* Java measures width/precision in UTF-16 code units. */
static size_t java_text_utf8_seq_len(unsigned char c) {
    if (c < 0x80u) return 1;
    if ((c & 0xe0u) == 0xc0u) return 2;
    if ((c & 0xf0u) == 0xe0u) return 3;
    if ((c & 0xf8u) == 0xf0u) return 4;
    return 1;
}
static size_t java_text_utf16_units(const char *s, size_t n) {
    size_t units = 0;
    for (size_t i = 0; i < n;) {
        size_t k = java_text_utf8_seq_len((unsigned char)s[i]);
        if (i + k > n) k = n - i;
        units += k == 4 ? 2u : 1u;
        i += k;
    }
    return units;
}
/* Byte length of the longest prefix holding at most `units` UTF-16 units. */
static size_t java_text_utf8_prefix_units(const char *s, size_t n, size_t units) {
    size_t i = 0, used = 0;
    while (i < n) {
        size_t k = java_text_utf8_seq_len((unsigned char)s[i]);
        if (i + k > n) k = n - i;
        const size_t u = k == 4 ? 2u : 1u;
        if (used + u > units) break;
        used += u;
        i += k;
    }
    return i;
}

/* ---------- specifier ---------- */

enum {
    F_LEFT = 1, F_ALT = 2, F_PLUS = 4, F_SPACE = 8, F_ZERO = 16, F_GROUP = 32, F_PAREN = 64, F_PREV = 128
};

typedef struct java_text_Spec {
    int flags;
    long width;     /* -1 when absent */
    long precision; /* -1 when absent */
    char conv;      /* lower-case conversion */
    bool upper;
    const char *start;
    size_t length;
    bool date_time;
} java_text_Spec;

static void java_text_spec_detail(const java_text_Spec *s, char *detail, size_t cap) {
    if (!detail || !cap) return;
    snprintf(detail, cap, "%.*s", (int)(s->length < 200 ? s->length : 200), s->start);
}

static void java_text_to_upper_ascii(char *s, size_t n) {
    for (size_t i = 0; i < n; ++i)
        if (s[i] >= 'a' && s[i] <= 'z') s[i] = (char)(s[i] - 'a' + 'A');
}

/* Justifies `body` (already signed/prefixed) into `out`. */
static void java_text_emit_justified(java_text_TextBuf *out, const java_text_Spec *s, const char *body, size_t n) {
    char *tmp = NULL;
    if (s->upper) {
        tmp = malloc(n + 1u);
        if (!tmp) { out->failed = true; return; }
        memcpy(tmp, body, n);
        tmp[n] = '\0';
        java_text_to_upper_ascii(tmp, n);
        body = tmp;
    }
    const size_t units = java_text_utf16_units(body, n);
    const size_t pad = s->width > 0 && (size_t)s->width > units ? (size_t)s->width - units : 0u;
    if (!(s->flags & F_LEFT)) java_text_buf_repeat(out, ' ', pad);
    java_text_buf_append_n(out, body, n);
    if (s->flags & F_LEFT) java_text_buf_repeat(out, ' ', pad);
    free(tmp);
}

static void java_text_emit_general(java_text_TextBuf *out, const java_text_Spec *s, const char *text) {
    size_t n = strlen(text);
    if (s->precision >= 0) n = java_text_utf8_prefix_units(text, n, (size_t)s->precision);
    java_text_emit_justified(out, s, text, n);
}

/* Inserts ',' every three digits of an ASCII digit run. */
static void java_text_append_grouped(java_text_TextBuf *b, const char *digits, size_t n, bool group) {
    if (!group || n <= 3u) { java_text_buf_append_n(b, digits, n); return; }
    size_t first = n % 3u;
    if (!first) first = 3u;
    java_text_buf_append_n(b, digits, first);
    for (size_t i = first; i < n; i += 3u) {
        java_text_buf_append_n(b, ",", 1);
        java_text_buf_append_n(b, digits + i, 3);
    }
}

/* Applies sign, parentheses and zero padding to a magnitude and emits it. */
static void java_text_emit_numeric(java_text_TextBuf *out, const java_text_Spec *s, bool negative, const char *prefix,
                         const char *magnitude, size_t mag_len) {
    java_text_TextBuf body = {0};
    const char *lead = negative ? ((s->flags & F_PAREN) ? "(" : "-")
                                : (s->flags & F_PLUS) ? "+" : (s->flags & F_SPACE) ? " " : "";
    const char *tail = negative && (s->flags & F_PAREN) ? ")" : "";
    java_text_buf_append(&body, lead);
    java_text_buf_append(&body, prefix);
    if (s->flags & F_ZERO) {
        const size_t used = strlen(lead) + strlen(prefix) + mag_len + strlen(tail);
        if (s->width > 0 && (size_t)s->width > used) java_text_buf_repeat(&body, '0', (size_t)s->width - used);
    }
    java_text_buf_append_n(&body, magnitude, mag_len);
    java_text_buf_append(&body, tail);
    if (body.failed) { out->failed = true; free(body.data); return; }
    java_text_emit_justified(out, s, body.data ? body.data : "", body.len);
    free(body.data);
}

/* ---------- integral conversions ---------- */

static SsrnxJavaFormatStatus java_text_format_integral(java_text_TextBuf *out, const java_text_Spec *s, const SsrnxJavaFormatArg *a) {
    char digits[80];
    if (s->conv == 'd') {
        const bool negative = a->integral < 0;
        const uint64_t mag = negative ? (uint64_t)0 - (uint64_t)a->integral : (uint64_t)a->integral;
        const int n = snprintf(digits, sizeof(digits), "%llu", (unsigned long long)mag);
        java_text_TextBuf grouped = {0};
        java_text_append_grouped(&grouped, digits, (size_t)n, (s->flags & F_GROUP) != 0);
        if (grouped.failed) { free(grouped.data); return SSRNX_JFMT_NO_MEMORY; }
        java_text_emit_numeric(out, s, negative, "", grouped.data ? grouped.data : "", grouped.len);
        free(grouped.data);
        return SSRNX_JFMT_OK;
    }
    /* o/x: two's complement of the boxed type's width; sign flags are only
     * legal for BigInteger. */
    if (s->flags & (F_PLUS | F_SPACE | F_GROUP | F_PAREN)) return SSRNX_JFMT_FLAGS_CONVERSION_MISMATCH;
    const unsigned bits = a->bits == 8u || a->bits == 16u || a->bits == 32u ? a->bits : 64u;
    uint64_t value = (uint64_t)a->integral;
    if (bits < 64u) value &= (((uint64_t)1) << bits) - 1u;
    const int n = snprintf(digits, sizeof(digits), s->conv == 'x' ? "%llx" : "%llo",
                           (unsigned long long)value);
    const char *prefix = (s->flags & F_ALT) ? (s->conv == 'x' ? "0x" : "0") : "";
    java_text_emit_numeric(out, s, false, prefix, digits, (size_t)n);
    return SSRNX_JFMT_OK;
}

/* BigInteger d/o/x: signed in every radix (no two's complement), so '+', ' '
 * and '(' are legal for o/x too; ',' only for d. */
static SsrnxJavaFormatStatus java_text_format_big_integer(java_text_TextBuf *out, const java_text_Spec *s, const SsrnxJavaFormatArg *a) {
    static const char digit_chars[] = "0123456789abcdef";
    const unsigned radix = s->conv == 'd' ? 10u : s->conv == 'o' ? 8u : 16u;
    if (s->conv != 'd' && (s->flags & F_GROUP)) return SSRNX_JFMT_FLAGS_CONVERSION_MISMATCH;
    const uint8_t *mag = a->magnitude;
    size_t size = a->magnitude_len;
    while (size && *mag == 0) { ++mag; --size; }
    /* log2(256)/log2(8) = 8/3 digits per byte bounds every radix used here. */
    char *digits = malloc(size * 3u + 2u);
    uint8_t *work = size ? malloc(size) : NULL;
    if (!digits || (size && !work)) { free(digits); free(work); return SSRNX_JFMT_NO_MEMORY; }
    size_t count = 0;
    if (!size) {
        digits[count++] = '0';
    } else {
        memcpy(work, mag, size);
        size_t first = 0;
        while (first < size) {
            unsigned rem = 0;
            for (size_t i = first; i < size; ++i) {
                const unsigned v = (rem << 8) | work[i];
                work[i] = (uint8_t)(v / radix);
                rem = v % radix;
            }
            digits[count++] = digit_chars[rem];
            while (first < size && work[first] == 0) ++first;
        }
        for (size_t i = 0; i < count / 2u; ++i) {
            const char t = digits[i]; digits[i] = digits[count - 1u - i]; digits[count - 1u - i] = t;
        }
    }
    free(work);
    const bool negative = a->integral < 0 && size;
    SsrnxJavaFormatStatus st = SSRNX_JFMT_OK;
    if (s->conv == 'd') {
        java_text_TextBuf grouped = {0};
        java_text_append_grouped(&grouped, digits, count, (s->flags & F_GROUP) != 0);
        if (grouped.failed) st = SSRNX_JFMT_NO_MEMORY;
        else java_text_emit_numeric(out, s, negative, "", grouped.data ? grouped.data : "", grouped.len);
        free(grouped.data);
    } else {
        const char *prefix = (s->flags & F_ALT) ? (s->conv == 'x' ? "0x" : "0") : "";
        java_text_emit_numeric(out, s, negative, prefix, digits, count);
    }
    free(digits);
    return st;
}

/* ---------- floating conversions ---------- */

/* Shortest decimal digits that round-trip the value (Double.toString /
 * Float.toString digit set). Value = 0.d1d2... scaled so d1 is at 10^exp. */
typedef struct Decimal {
    char digits[48];
    int ndigits;
    int exp; /* power of ten of digits[0] */
} Decimal;

static void java_text_decimal_from_value(double v, bool is_float, Decimal *d) {
    v = fabs(v);
    if (v == 0.0) {
        d->digits[0] = '0'; d->digits[1] = '\0'; d->ndigits = 1; d->exp = 0;
        return;
    }
    char buf[64];
    for (int p = 1; p <= 17; ++p) {
        snprintf(buf, sizeof(buf), "%.*e", p - 1, v);
        const bool same = is_float ? strtof(buf, NULL) == (float)v : strtod(buf, NULL) == v;
        if (same || p == 17) break;
    }
    int n = 0;
    const char *p = buf;
    for (; *p && *p != 'e'; ++p)
        if (isdigit((unsigned char)*p) && n < (int)sizeof(d->digits) - 1) d->digits[n++] = *p;
    d->digits[n] = '\0';
    d->ndigits = n;
    d->exp = *p == 'e' ? atoi(p + 1) : 0;
    while (d->ndigits > 1 && d->digits[d->ndigits - 1] == '0') d->digits[--d->ndigits] = '\0';
}

/* Keeps `keep` significant digits with RoundingMode.HALF_UP. */
static void java_text_decimal_round(Decimal *d, int keep) {
    if (keep >= d->ndigits) return;
    if (keep < 0) { d->digits[0] = '0'; d->digits[1] = '\0'; d->ndigits = 1; return; }
    const bool up = d->digits[keep] >= '5';
    d->ndigits = keep;
    d->digits[keep] = '\0';
    if (up) {
        int i = keep - 1;
        for (; i >= 0; --i) {
            if (d->digits[i] == '9') { d->digits[i] = '0'; continue; }
            d->digits[i]++;
            break;
        }
        if (i < 0) {
            memmove(d->digits + 1, d->digits, (size_t)d->ndigits + 1u);
            d->digits[0] = '1';
            d->ndigits++;
            d->exp++;
        }
    }
    if (d->ndigits == 0) {
        /* keep == 0 rounded down: zero. keep == 0 rounded up handled above. */
        d->digits[0] = '0'; d->digits[1] = '\0'; d->ndigits = 1;
    }
    while (d->ndigits > 1 && d->digits[d->ndigits - 1] == '0') d->digits[--d->ndigits] = '\0';
}

static bool java_text_decimal_is_zero(const Decimal *d) { return d->ndigits == 1 && d->digits[0] == '0'; }

static char java_text_decimal_digit(const Decimal *d, int power) {
    const int index = d->exp - power;
    return index >= 0 && index < d->ndigits ? d->digits[index] : '0';
}

static void java_text_render_fixed(java_text_TextBuf *b, Decimal d, long precision, bool group, bool alt) {
    if (!java_text_decimal_is_zero(&d)) {
        const int keep = d.exp + 1 + (int)precision;
        if (keep == 0) {
            /* Only the rounding digit remains: 0.5 -> 1 at 10^-precision. */
            if (d.digits[0] >= '5') {
                d.digits[0] = '1'; d.digits[1] = '\0'; d.ndigits = 1; d.exp = -(int)precision;
            } else {
                d.digits[0] = '0'; d.digits[1] = '\0'; d.ndigits = 1; d.exp = 0;
            }
        } else {
            java_text_decimal_round(&d, keep);
        }
    }
    if (java_text_decimal_is_zero(&d)) d.exp = 0;
    java_text_TextBuf ip = {0};
    if (d.exp < 0) java_text_buf_append(&ip, "0");
    else for (int pw = d.exp; pw >= 0; --pw) { char c = java_text_decimal_digit(&d, pw); java_text_buf_append_n(&ip, &c, 1); }
    java_text_append_grouped(b, ip.data ? ip.data : "0", ip.data ? ip.len : 1u, group);
    if (ip.failed) b->failed = true;
    free(ip.data);
    if (precision > 0 || alt) java_text_buf_append(b, ".");
    for (long i = 1; i <= precision; ++i) { char c = java_text_decimal_digit(&d, -(int)i); java_text_buf_append_n(b, &c, 1); }
}

static void java_text_render_scientific(java_text_TextBuf *b, Decimal d, long precision, bool alt) {
    if (!java_text_decimal_is_zero(&d)) java_text_decimal_round(&d, (int)precision + 1);
    const int exp = java_text_decimal_is_zero(&d) ? 0 : d.exp;
    char c = d.digits[0];
    java_text_buf_append_n(b, &c, 1);
    if (precision > 0 || alt) java_text_buf_append(b, ".");
    for (long i = 1; i <= precision; ++i) {
        c = i < d.ndigits ? d.digits[i] : '0';
        java_text_buf_append_n(b, &c, 1);
    }
    char e[16];
    snprintf(e, sizeof(e), "e%c%02d", exp < 0 ? '-' : '+', exp < 0 ? -exp : exp);
    java_text_buf_append(b, e);
}

static SsrnxJavaFormatStatus java_text_format_floating(java_text_TextBuf *out, const java_text_Spec *s, const SsrnxJavaFormatArg *a) {
    const double v = a->floating;
    const bool is_float = a->bits == 32u;
    if ((s->conv == 'e' && (s->flags & F_GROUP)) || (s->conv == 'g' && (s->flags & F_ALT)))
        return SSRNX_JFMT_FLAGS_CONVERSION_MISMATCH;
    const bool negative = signbit(v) != 0 && !isnan(v);
    if (isnan(v) || isinf(v)) {
        java_text_Spec plain = *s;
        plain.flags &= ~F_ZERO;
        const char *text = isnan(v) ? "NaN" : "Infinity";
        java_text_emit_numeric(out, &plain, negative && !isnan(v), "", text, strlen(text));
        return SSRNX_JFMT_OK;
    }
    long precision = s->precision >= 0 ? s->precision : 6;
    Decimal d;
    java_text_decimal_from_value(v, is_float, &d);
    java_text_TextBuf mag = {0};
    if (s->conv == 'f') {
        java_text_render_fixed(&mag, d, precision, (s->flags & F_GROUP) != 0, (s->flags & F_ALT) != 0);
    } else if (s->conv == 'e') {
        java_text_render_scientific(&mag, d, precision, (s->flags & F_ALT) != 0);
    } else { /* 'g' */
        if (precision == 0) precision = 1;
        Decimal r = d;
        if (!java_text_decimal_is_zero(&r)) java_text_decimal_round(&r, (int)precision);
        /* After rounding: [10^-4, 10^precision) is decimal, else scientific. */
        if (java_text_decimal_is_zero(&r) || (r.exp >= -4 && r.exp < precision))
            java_text_render_fixed(&mag, d, java_text_decimal_is_zero(&r) ? precision - 1 : precision - (r.exp + 1),
                         (s->flags & F_GROUP) != 0, false);
        else
            java_text_render_scientific(&mag, d, precision - 1, false);
    }
    if (mag.failed) { free(mag.data); return SSRNX_JFMT_NO_MEMORY; }
    java_text_emit_numeric(out, s, negative, "", mag.data ? mag.data : "", mag.len);
    free(mag.data);
    return SSRNX_JFMT_OK;
}

/* Double.toString / Float.toString: shortest round-trip digits, decimal form
 * for 10^-3 <= |v| < 10^7, otherwise computerized scientific notation. */
void ssrnx_java_floating_to_string(double v, bool is_float, char *out, size_t cap) {
    if (isnan(v)) { snprintf(out, cap, "NaN"); return; }
    if (isinf(v)) { snprintf(out, cap, v < 0 ? "-Infinity" : "Infinity"); return; }
    Decimal d;
    java_text_decimal_from_value(v, is_float, &d);
    java_text_TextBuf b = {0};
    if (signbit(v)) java_text_buf_append(&b, "-");
    if (java_text_decimal_is_zero(&d) || (d.exp >= -3 && d.exp < 7)) {
        const int frac = java_text_decimal_is_zero(&d) ? 1 : (d.ndigits - 1 - d.exp > 0 ? d.ndigits - 1 - d.exp : 1);
        java_text_render_fixed(&b, d, frac, false, false);
    } else {
        char c = d.digits[0];
        java_text_buf_append_n(&b, &c, 1);
        java_text_buf_append(&b, ".");
        if (d.ndigits > 1) java_text_buf_append(&b, d.digits + 1);
        else java_text_buf_append(&b, "0");
        char e[16];
        snprintf(e, sizeof(e), "E%d", d.exp);
        java_text_buf_append(&b, e);
    }
    snprintf(out, cap, "%s", b.data && !b.failed ? b.data : "");
    free(b.data);
}

/* ---------- driver ---------- */

static const char *java_text_arg_type_name(const SsrnxJavaFormatArg *a) {
    if (a->type_name && a->type_name[0]) return a->type_name;
    switch (a->kind) {
    case SSRNX_JFMT_ARG_STRING: return "java.lang.String";
    case SSRNX_JFMT_ARG_BOOLEAN: return "java.lang.Boolean";
    case SSRNX_JFMT_ARG_CHAR: return "java.lang.Character";
    case SSRNX_JFMT_ARG_FLOATING: return a->bits == 32u ? "java.lang.Float" : "java.lang.Double";
    case SSRNX_JFMT_ARG_INTEGRAL:
        return a->bits == 8u ? "java.lang.Byte" : a->bits == 16u ? "java.lang.Short"
             : a->bits == 32u ? "java.lang.Integer" : "java.lang.Long";
    default: return "java.lang.Object";
    }
}

static SsrnxJavaFormatStatus java_text_illegal_conversion(const java_text_Spec *s, const SsrnxJavaFormatArg *a,
                                                char *detail, size_t cap) {
    if (detail && cap) snprintf(detail, cap, "%c != %s", s->upper ? toupper((unsigned char)s->conv) : s->conv,
                                java_text_arg_type_name(a));
    return SSRNX_JFMT_ILLEGAL_CONVERSION;
}

static bool java_text_append_utf8_codepoint(java_text_TextBuf *b, uint32_t cp) {
    char u[4];
    size_t n;
    if (cp < 0x80u) { u[0] = (char)cp; n = 1; }
    else if (cp < 0x800u) { u[0] = (char)(0xc0u | (cp >> 6)); u[1] = (char)(0x80u | (cp & 0x3fu)); n = 2; }
    else if (cp < 0x10000u) {
        u[0] = (char)(0xe0u | (cp >> 12)); u[1] = (char)(0x80u | ((cp >> 6) & 0x3fu));
        u[2] = (char)(0x80u | (cp & 0x3fu)); n = 3;
    } else if (cp <= 0x10ffffu) {
        u[0] = (char)(0xf0u | (cp >> 18)); u[1] = (char)(0x80u | ((cp >> 12) & 0x3fu));
        u[2] = (char)(0x80u | ((cp >> 6) & 0x3fu)); u[3] = (char)(0x80u | (cp & 0x3fu)); n = 4;
    } else return false;
    java_text_buf_append_n(b, u, n);
    return true;
}

static SsrnxJavaFormatStatus java_text_format_one(java_text_TextBuf *out, const java_text_Spec *s, const SsrnxJavaFormatArg *a,
                                        size_t index, SsrnxJavaFormatTextFn text_fn, void *user,
                                        char *detail, size_t cap) {
    const bool is_null = a->kind == SSRNX_JFMT_ARG_NULL;
    if (s->date_time) {
        if (is_null) { java_text_emit_general(out, s, "null"); return SSRNX_JFMT_OK; }
        if (!a->date_time) return java_text_illegal_conversion(s, a, detail, cap);
        char text[256];
        if (!a->date_time(a->date_user, index, s->conv, text, sizeof(text)))
            return SSRNX_JFMT_TEXT_FAILED;
        java_text_emit_justified(out, s, text, strlen(text));
        return SSRNX_JFMT_OK;
    }
    switch (s->conv) {
    case 'b': {
        if (s->flags & ~F_LEFT & ~F_PREV) { java_text_spec_detail(s, detail, cap); return SSRNX_JFMT_FLAGS_CONVERSION_MISMATCH; }
        const bool value = a->kind == SSRNX_JFMT_ARG_BOOLEAN ? a->boolean : !is_null;
        java_text_emit_general(out, s, value ? "true" : "false");
        return SSRNX_JFMT_OK;
    }
    case 's': {
        /* '#' requires Formattable, which SSRNX does not model. */
        if (s->flags & ~F_LEFT & ~F_PREV) { java_text_spec_detail(s, detail, cap); return SSRNX_JFMT_FLAGS_CONVERSION_MISMATCH; }
        if (is_null) { java_text_emit_general(out, s, "null"); return SSRNX_JFMT_OK; }
        if (a->kind == SSRNX_JFMT_ARG_CHAR) {
            java_text_TextBuf c = {0};
            (void)java_text_append_utf8_codepoint(&c, (uint32_t)(uint16_t)a->integral);
            if (c.failed) { free(c.data); return SSRNX_JFMT_NO_MEMORY; }
            java_text_emit_general(out, s, c.data ? c.data : "");
            free(c.data);
            return SSRNX_JFMT_OK;
        }
        char tmp[64];
        const char *text = NULL;
        switch (a->kind) {
        case SSRNX_JFMT_ARG_STRING: text = a->text; break;
        case SSRNX_JFMT_ARG_INTEGRAL: snprintf(tmp, sizeof(tmp), "%lld", (long long)a->integral); text = tmp; break;
        case SSRNX_JFMT_ARG_BOOLEAN: text = a->boolean ? "true" : "false"; break;
        case SSRNX_JFMT_ARG_FLOATING: ssrnx_java_floating_to_string(a->floating, a->bits == 32u, tmp, sizeof(tmp)); text = tmp; break;
        default: text = a->text ? a->text : (text_fn ? text_fn(user, index) : NULL); break;
        }
        if (!text) return SSRNX_JFMT_TEXT_FAILED;
        java_text_emit_general(out, s, text);
        return SSRNX_JFMT_OK;
    }
    case 'c': {
        if (s->precision >= 0) { java_text_spec_detail(s, detail, cap); return SSRNX_JFMT_ILLEGAL_PRECISION; }
        if (s->flags & ~F_LEFT & ~F_PREV) { java_text_spec_detail(s, detail, cap); return SSRNX_JFMT_FLAGS_CONVERSION_MISMATCH; }
        if (is_null) { java_text_emit_general(out, s, "null"); return SSRNX_JFMT_OK; }
        if (a->kind != SSRNX_JFMT_ARG_CHAR && !(a->kind == SSRNX_JFMT_ARG_INTEGRAL && a->bits <= 32u))
            return java_text_illegal_conversion(s, a, detail, cap);
        const int64_t cp = a->bits == 8u ? (int64_t)(uint8_t)a->integral
                         : a->bits == 16u && a->kind == SSRNX_JFMT_ARG_INTEGRAL ? (int64_t)(uint16_t)a->integral
                         : a->integral;
        java_text_TextBuf c = {0};
        if (cp < 0 || !java_text_append_utf8_codepoint(&c, (uint32_t)cp)) {
            free(c.data);
            if (detail && cap) snprintf(detail, cap, "Code point = 0x%llx", (unsigned long long)cp);
            return SSRNX_JFMT_ILLEGAL_CONVERSION;
        }
        if (c.failed) { free(c.data); return SSRNX_JFMT_NO_MEMORY; }
        java_text_emit_general(out, s, c.data ? c.data : "");
        free(c.data);
        return SSRNX_JFMT_OK;
    }
    case 'd': case 'o': case 'x':
        if (s->precision >= 0) { java_text_spec_detail(s, detail, cap); return SSRNX_JFMT_ILLEGAL_PRECISION; }
        if (is_null) { java_text_emit_general(out, s, "null"); return SSRNX_JFMT_OK; }
        if (a->kind != SSRNX_JFMT_ARG_INTEGRAL && a->kind != SSRNX_JFMT_ARG_BIG_INTEGER)
            return java_text_illegal_conversion(s, a, detail, cap);
        if (s->conv == 'd' && (s->flags & F_ALT)) { java_text_spec_detail(s, detail, cap); return SSRNX_JFMT_FLAGS_CONVERSION_MISMATCH; }
        {
            SsrnxJavaFormatStatus st = a->kind == SSRNX_JFMT_ARG_BIG_INTEGER
                ? java_text_format_big_integer(out, s, a) : java_text_format_integral(out, s, a);
            if (st != SSRNX_JFMT_OK) java_text_spec_detail(s, detail, cap);
            return st;
        }
    case 'e': case 'f': case 'g':
        if (is_null) { java_text_emit_general(out, s, "null"); return SSRNX_JFMT_OK; }
        if (a->kind != SSRNX_JFMT_ARG_FLOATING) return java_text_illegal_conversion(s, a, detail, cap);
        {
            SsrnxJavaFormatStatus st = java_text_format_floating(out, s, a);
            if (st != SSRNX_JFMT_OK) java_text_spec_detail(s, detail, cap);
            return st;
        }
    default:
        java_text_spec_detail(s, detail, cap);
        return SSRNX_JFMT_UNSUPPORTED;
    }
}

static bool java_text_parse_number(const char **p, long *out) {
    if (!isdigit((unsigned char)**p)) return false;
    long v = 0;
    while (isdigit((unsigned char)**p)) {
        if (v < 100000000L) v = v * 10 + (**p - '0');
        ++*p;
    }
    *out = v;
    return true;
}

SsrnxJavaFormatStatus ssrnx_java_format(const char *format,
                                        const SsrnxJavaFormatArg *args, size_t argc,
                                        SsrnxJavaFormatTextFn text_fn, void *text_user,
                                        char **out, char *detail, size_t detail_cap) {
    if (detail && detail_cap) detail[0] = '\0';
    if (!out) return SSRNX_JFMT_NO_MEMORY;
    *out = NULL;
    if (!format) format = "";
    java_text_TextBuf b = {0};
    java_text_buf_append(&b, "");
    size_t ordinary = 0;
    long last = -1;
    const char *p = format;
    while (*p) {
        const char *pct = strchr(p, '%');
        if (!pct) { java_text_buf_append(&b, p); break; }
        java_text_buf_append_n(&b, p, (size_t)(pct - p));
        java_text_Spec s = { 0, -1, -1, 0, false, pct, 0, false };
        const char *q = pct + 1;
        long explicit_index = -1;
        /* argument_index$ : digits followed by '$'. */
        {
            const char *r = q;
            long v = 0;
            if (java_text_parse_number(&r, &v) && *r == '$') { explicit_index = v; q = r + 1; }
        }
        for (;; ++q) {
            int f = 0;
            switch (*q) {
            case '-': f = F_LEFT; break;
            case '#': f = F_ALT; break;
            case '+': f = F_PLUS; break;
            case ' ': f = F_SPACE; break;
            case '0': f = F_ZERO; break;
            case ',': f = F_GROUP; break;
            case '(': f = F_PAREN; break;
            case '<': f = F_PREV; break;
            default: break;
            }
            if (!f) break;
            if (s.flags & f) {
                s.length = (size_t)(q - pct + 1);
                java_text_spec_detail(&s, detail, detail_cap);
                free(b.data);
                return SSRNX_JFMT_ILLEGAL_FLAGS; /* DuplicateFormatFlagsException */
            }
            s.flags |= f;
        }
        (void)java_text_parse_number(&q, &s.width);
        if (*q == '.') {
            ++q;
            if (!java_text_parse_number(&q, &s.precision)) {
                s.length = (size_t)(q - pct);
                if (detail && detail_cap) snprintf(detail, detail_cap, "Conversion = '.'");
                free(b.data);
                return SSRNX_JFMT_UNKNOWN_CONVERSION;
            }
        }
        if (!*q) {
            if (detail && detail_cap) snprintf(detail, detail_cap, "Conversion = '%%'");
            free(b.data);
            return SSRNX_JFMT_UNKNOWN_CONVERSION;
        }
        char conv = *q;
        s.length = (size_t)(q - pct + 1);
        p = q + 1;
        if (conv == 'h' || conv == 'H' || conv == 'a' || conv == 'A') {
            java_text_spec_detail(&s, detail, detail_cap);
            free(b.data);
            return SSRNX_JFMT_UNSUPPORTED;
        }
        s.date_time = conv == 't' || conv == 'T';
        if (s.date_time) {
            s.upper = conv == 'T';
            conv = *p;
            if (conv) { ++p; ++s.length; }
            s.conv = conv;
        } else {
            s.upper = conv == 'S' || conv == 'B' || conv == 'C' || conv == 'X' || conv == 'E' || conv == 'G';
            s.conv = (char)tolower((unsigned char)conv);
        }
        if (s.date_time ? !conv || !strchr("HIklMSLNpzZsQBbhAaCYyjmdeRTrDFc", conv)
                        : !strchr("bscdoxefg%n", s.conv) || (conv == 'D' || conv == 'O' || conv == 'F' || conv == 'N')) {
            if (detail && detail_cap) snprintf(detail, detail_cap, "Conversion = '%c'", conv);
            free(b.data);
            return SSRNX_JFMT_UNKNOWN_CONVERSION;
        }
        if (s.date_time && s.precision >= 0) {
            java_text_spec_detail(&s, detail, detail_cap); free(b.data); return SSRNX_JFMT_ILLEGAL_PRECISION;
        }
        if (s.date_time && (s.flags & ~(F_LEFT | F_PREV))) {
            java_text_spec_detail(&s, detail, detail_cap); free(b.data); return SSRNX_JFMT_FLAGS_CONVERSION_MISMATCH;
        }
        if ((s.flags & F_LEFT) && (s.flags & F_ZERO)) {
            java_text_spec_detail(&s, detail, detail_cap); free(b.data); return SSRNX_JFMT_ILLEGAL_FLAGS;
        }
        if ((s.flags & F_PLUS) && (s.flags & F_SPACE)) {
            java_text_spec_detail(&s, detail, detail_cap); free(b.data); return SSRNX_JFMT_ILLEGAL_FLAGS;
        }
        if ((s.flags & (F_LEFT | F_ZERO)) && s.width < 0) {
            java_text_spec_detail(&s, detail, detail_cap); free(b.data); return SSRNX_JFMT_MISSING_WIDTH;
        }
        if (!s.date_time && s.conv == 'n') {
            if (s.flags || s.width >= 0 || s.precision >= 0) {
                java_text_spec_detail(&s, detail, detail_cap); free(b.data); return SSRNX_JFMT_ILLEGAL_FLAGS;
            }
            java_text_buf_append(&b, "\n");
            continue;
        }
        if (!s.date_time && s.conv == '%') {
            if (s.precision >= 0) { java_text_spec_detail(&s, detail, detail_cap); free(b.data); return SSRNX_JFMT_ILLEGAL_PRECISION; }
            if (s.flags & ~F_LEFT) { java_text_spec_detail(&s, detail, detail_cap); free(b.data); return SSRNX_JFMT_FLAGS_CONVERSION_MISMATCH; }
            java_text_emit_justified(&b, &s, "%", 1);
            continue;
        }
        if ((s.flags & F_ZERO) && strchr("bsc", s.conv)) {
            java_text_spec_detail(&s, detail, detail_cap); free(b.data); return SSRNX_JFMT_FLAGS_CONVERSION_MISMATCH;
        }
        long index;
        if (s.flags & F_PREV) index = last;
        else if (explicit_index >= 0) index = explicit_index - 1;
        else index = (long)ordinary++;
        if (index < 0 || (size_t)index >= argc) {
            java_text_spec_detail(&s, detail, detail_cap);
            free(b.data);
            return SSRNX_JFMT_MISSING_ARGUMENT;
        }
        last = index;
        SsrnxJavaFormatStatus st = java_text_format_one(&b, &s, &args[index], (size_t)index, text_fn, text_user,
                                              detail, detail_cap);
        if (st != SSRNX_JFMT_OK) { free(b.data); return st; }
        if (b.failed) break;
    }
    if (b.failed) { free(b.data); return SSRNX_JFMT_NO_MEMORY; }
    *out = b.data;
    return SSRNX_JFMT_OK;
}

const char *ssrnx_java_format_exception_class(SsrnxJavaFormatStatus status) {
    switch (status) {
    case SSRNX_JFMT_NO_MEMORY: return "java/lang/OutOfMemoryError";
    case SSRNX_JFMT_UNKNOWN_CONVERSION: return "java/util/UnknownFormatConversionException";
    case SSRNX_JFMT_MISSING_ARGUMENT: return "java/util/MissingFormatArgumentException";
    case SSRNX_JFMT_ILLEGAL_CONVERSION: return "java/util/IllegalFormatConversionException";
    case SSRNX_JFMT_MISSING_WIDTH: return "java/util/MissingFormatWidthException";
    case SSRNX_JFMT_ILLEGAL_PRECISION: return "java/util/IllegalFormatPrecisionException";
    case SSRNX_JFMT_FLAGS_CONVERSION_MISMATCH: return "java/util/FormatFlagsConversionMismatchException";
    case SSRNX_JFMT_ILLEGAL_FLAGS: return "java/util/IllegalFormatFlagsException";
    case SSRNX_JFMT_UNSUPPORTED: return "java/lang/UnsupportedOperationException";
    default: return NULL;
    }
}

/* ---------- String.getBytes(charsetName) ---------- */

static void java_text_normalize_charset(const char *name, char *out, size_t cap) {
    size_t used = 0;
    for (const unsigned char *p = (const unsigned char *)(name ? name : ""); *p && used + 1u < cap; ++p) {
        unsigned char c = (unsigned char)toupper(*p);
        if (isalnum(c)) out[used++] = (char)c;
    }
    out[used] = '\0';
}

bool ssrnx_java_encode_utf16(const uint16_t *chars, size_t length, const char *charset,
                             uint8_t **out_bytes, size_t *out_length, bool *out_supported) {
    if (out_supported) *out_supported = true;
    if (!out_bytes || !out_length || (length && !chars)) return false;
    *out_bytes = NULL;
    *out_length = 0;
    char cs[48];
    java_text_normalize_charset(charset ? charset : "UTF-8", cs, sizeof(cs));
    enum { ENC_UTF8, ENC_ASCII, ENC_LATIN1, ENC_UTF16, ENC_UTF16BE, ENC_UTF16LE } enc;
    if (strcmp(cs, "UTF8") == 0) enc = ENC_UTF8;
    else if (strcmp(cs, "USASCII") == 0 || strcmp(cs, "ASCII") == 0) enc = ENC_ASCII;
    else if (strcmp(cs, "ISO88591") == 0 || strcmp(cs, "LATIN1") == 0) enc = ENC_LATIN1;
    else if (strcmp(cs, "UTF16") == 0) enc = ENC_UTF16;
    else if (strcmp(cs, "UTF16BE") == 0) enc = ENC_UTF16BE;
    else if (strcmp(cs, "UTF16LE") == 0) enc = ENC_UTF16LE;
    else { if (out_supported) *out_supported = false; return false; }

    const size_t cap = enc == ENC_UTF8 ? length * 3u + 1u : length * 2u + 3u;
    uint8_t *o = malloc(cap);
    if (!o) return false;
    size_t n = 0;
    if (enc == ENC_UTF16) { o[n++] = 0xfeu; o[n++] = 0xffu; }
    for (size_t i = 0; i < length; ++i) {
        uint32_t cp = chars[i];
        const bool high = cp >= 0xd800u && cp <= 0xdbffu;
        const bool low = cp >= 0xdc00u && cp <= 0xdfffu;
        bool paired = false;
        if (high && i + 1u < length && chars[i + 1u] >= 0xdc00u && chars[i + 1u] <= 0xdfffu) paired = true;
        switch (enc) {
        case ENC_UTF8:
            if (paired) {
                cp = 0x10000u + ((cp - 0xd800u) << 10) + (uint32_t)(chars[i + 1u] - 0xdc00u);
                ++i;
                o[n++] = (uint8_t)(0xf0u | (cp >> 18)); o[n++] = (uint8_t)(0x80u | ((cp >> 12) & 0x3fu));
                o[n++] = (uint8_t)(0x80u | ((cp >> 6) & 0x3fu)); o[n++] = (uint8_t)(0x80u | (cp & 0x3fu));
            } else if (high || low) {
                o[n++] = '?';
            } else if (cp < 0x80u) {
                o[n++] = (uint8_t)cp;
            } else if (cp < 0x800u) {
                o[n++] = (uint8_t)(0xc0u | (cp >> 6)); o[n++] = (uint8_t)(0x80u | (cp & 0x3fu));
            } else {
                o[n++] = (uint8_t)(0xe0u | (cp >> 12)); o[n++] = (uint8_t)(0x80u | ((cp >> 6) & 0x3fu));
                o[n++] = (uint8_t)(0x80u | (cp & 0x3fu));
            }
            break;
        case ENC_ASCII:
        case ENC_LATIN1:
            /* A surrogate pair is one unmappable character: one '?'. */
            if (paired) ++i;
            o[n++] = (!high && !low && cp <= (enc == ENC_ASCII ? 0x7fu : 0xffu)) ? (uint8_t)cp : (uint8_t)'?';
            break;
        default: {
            uint16_t units[2] = { (uint16_t)cp, 0 };
            size_t count = 1;
            if (paired) { units[1] = chars[++i]; count = 2; }
            else if (high || low) units[0] = 0xfffdu;
            for (size_t k = 0; k < count; ++k) {
                if (enc == ENC_UTF16LE) { o[n++] = (uint8_t)(units[k] & 0xffu); o[n++] = (uint8_t)(units[k] >> 8); }
                else { o[n++] = (uint8_t)(units[k] >> 8); o[n++] = (uint8_t)(units[k] & 0xffu); }
            }
            break;
        }
        }
    }
    *out_bytes = o;
    *out_length = n;
    return true;
}

/* Paired case blocks: [lo, hi] maps to lo + delta (upper -> lower); "alt"
 * blocks alternate upper (even offset) / lower (odd offset). */
typedef struct java_text_JavaCaseRange { uint32_t lo, hi; int32_t delta; bool alt; } java_text_JavaCaseRange;
static const java_text_JavaCaseRange java_text_k_java_case[] = {
    {0x0041, 0x005A, 32, false}, {0x00C0, 0x00D6, 32, false}, {0x00D8, 0x00DE, 32, false},
    {0x0100, 0x012F, 1, true}, {0x0132, 0x0137, 1, true}, {0x0139, 0x0148, 1, true},
    {0x014A, 0x0177, 1, true}, {0x0179, 0x017E, 1, true}, {0x0200, 0x021F, 1, true},
    {0x0391, 0x03A1, 32, false}, {0x03A3, 0x03AB, 32, false}, {0x03D8, 0x03EF, 1, true},
    {0x0400, 0x040F, 80, false}, {0x0410, 0x042F, 32, false}, {0x0460, 0x0481, 1, true},
    {0x048A, 0x04BF, 1, true}, {0x04C1, 0x04CE, 1, true}, {0x04D0, 0x052F, 1, true},
    {0x0531, 0x0556, 48, false}, {0x1E00, 0x1E95, 1, true}, {0x1EA0, 0x1EFF, 1, true},
    {0xFF21, 0xFF3A, 32, false},
};

uint32_t ssrnx_java_to_lower(uint32_t c) {
    if (c < 0x80) return c >= 'A' && c <= 'Z' ? c + 32u : c;
    if (c == 0x0130) return 'i';
    if (c == 0x0178) return 0x00FF;
    if (c == 0x212A) return 'k'; /* Kelvin sign's Unicode simple lowercase mapping. */
    if (c == 0x0386) return 0x03AC;
    if (c >= 0x0388 && c <= 0x038A) return c + 37u;
    if (c == 0x038C) return 0x03CC;
    if (c == 0x038E || c == 0x038F) return c + 63u;
    for (size_t i = 0; i < sizeof(java_text_k_java_case) / sizeof(java_text_k_java_case[0]); ++i) {
        const java_text_JavaCaseRange *r = &java_text_k_java_case[i];
        if (c < r->lo || c > r->hi) continue;
        if (r->alt) return ((c - r->lo) & 1u) ? c : c + 1u;
        return c + (uint32_t)r->delta;
    }
    return c;
}

uint32_t ssrnx_java_to_upper(uint32_t c) {
    if (c < 0x80) return c >= 'a' && c <= 'z' ? c - 32u : c;
    if (c == 0x00B5) return 0x039C;
    if (c == 0x00FF) return 0x0178;
    if (c == 0x0131) return 'I';
    if (c == 0x017F) return 'S';
    if (c == 0x03C2) return 0x03A3; /* final sigma */
    if (c == 0x03AC) return 0x0386;
    if (c >= 0x03AD && c <= 0x03AF) return c - 37u;
    if (c == 0x03CC) return 0x038C;
    if (c == 0x03CD || c == 0x03CE) return c - 63u;
    for (size_t i = 0; i < sizeof(java_text_k_java_case) / sizeof(java_text_k_java_case[0]); ++i) {
        const java_text_JavaCaseRange *r = &java_text_k_java_case[i];
        if (r->alt) {
            if (c > r->lo && c <= r->hi + 1u && ((c - r->lo) & 1u)) return c - 1u;
            continue;
        }
        const uint32_t lo = r->lo + (uint32_t)r->delta, hi = r->hi + (uint32_t)r->delta;
        if (c >= lo && c <= hi) return c - (uint32_t)r->delta;
    }
    return c;
}

typedef struct java_text_JavaCpRange { uint32_t lo, hi; } java_text_JavaCpRange;
static const java_text_JavaCpRange java_text_k_java_letters[] = {
    {'A','Z'},{'a','z'},{0xAA,0xAA},{0xB5,0xB5},{0xBA,0xBA},{0xC0,0xD6},{0xD8,0xF6},{0xF8,0x2C1},
    {0x2C6,0x2D1},{0x2E0,0x2E4},{0x370,0x374},{0x376,0x37D},{0x386,0x386},{0x388,0x3FF},{0x400,0x481},
    {0x48A,0x52F},{0x531,0x556},{0x561,0x587},{0x5D0,0x5EA},{0x620,0x64A},{0x671,0x6D3},{0x904,0x939},
    {0x993,0x9B9},{0xE01,0xE30},{0x10A0,0x10FF},{0x1100,0x11FF},{0x1E00,0x1FBC},{0x1FC2,0x1FFC},
    {0x2C00,0x2CE4},{0x3041,0x3096},{0x30A1,0x30FA},{0x3105,0x312F},{0x3131,0x318E},{0x3400,0x4DBF},
    {0x4E00,0x9FFF},{0xA000,0xA48C},{0xAC00,0xD7A3},{0xF900,0xFAFF},{0xFF21,0xFF3A},{0xFF41,0xFF5A},
    {0xFF66,0xFFDC},{0x20000,0x2FA1F},
};
/* Zero of each decimal digit block. */
static const uint32_t java_text_k_java_digit_zeros[] = {'0', 0x660, 0x6F0, 0x966, 0x9E6, 0xE50, 0xFF10};

bool ssrnx_java_is_letter(uint32_t c) {
    for (size_t i = 0; i < sizeof(java_text_k_java_letters) / sizeof(java_text_k_java_letters[0]); ++i)
        if (c >= java_text_k_java_letters[i].lo && c <= java_text_k_java_letters[i].hi) return true;
    return false;
}

int ssrnx_java_digit_value(uint32_t c) {
    for (size_t i = 0; i < sizeof(java_text_k_java_digit_zeros) / sizeof(java_text_k_java_digit_zeros[0]); ++i)
        if (c >= java_text_k_java_digit_zeros[i] && c <= java_text_k_java_digit_zeros[i] + 9u) return (int)(c - java_text_k_java_digit_zeros[i]);
    return -1;
}

bool ssrnx_java_is_digit(uint32_t c) { return ssrnx_java_digit_value(c) >= 0; }

/* ---------- java.util.StringTokenizer core ---------- */

static size_t java_text_tokenizer_code_point_at(const uint16_t *s, size_t length, size_t index, uint32_t *out) {
    const uint16_t hi = s[index];
    if (hi >= 0xd800u && hi <= 0xdbffu && index + 1u < length) {
        const uint16_t lo = s[index + 1u];
        if (lo >= 0xdc00u && lo <= 0xdfffu) {
            *out = UINT32_C(0x10000) + (((uint32_t)hi - 0xd800u) << 10) + ((uint32_t)lo - 0xdc00u);
            return 2u;
        }
    }
    *out = hi;
    return 1u;
}

static bool java_text_tokenizer_is_delimiter(const SsrnxJavaStringTokenizerState *state, size_t index,
                                   size_t *out_width) {
    uint32_t cp = 0;
    const size_t width = java_text_tokenizer_code_point_at(state->text, state->text_length, index, &cp);
    if (out_width) *out_width = width;
    for (size_t i = 0; i < state->delimiter_length;) {
        uint32_t delimiter = 0;
        const size_t delimiter_width = java_text_tokenizer_code_point_at(state->delimiters, state->delimiter_length,
                                                                i, &delimiter);
        if (delimiter == cp) return true;
        i += delimiter_width;
    }
    return false;
}

static SsrnxJavaStringTokenizerStatus java_text_tokenizer_skip_delimiters(
    const SsrnxJavaStringTokenizerState *state, size_t *position) {
    if (!state->delimiters) return SSRNX_JST_NULL_DELIMITERS;
    while (*position < state->text_length) {
        size_t width = 0;
        if (!java_text_tokenizer_is_delimiter(state, *position, &width)) break;
        *position += width;
    }
    return SSRNX_JST_OK;
}

SsrnxJavaStringTokenizerStatus ssrnx_java_string_tokenizer_has_more(
    const SsrnxJavaStringTokenizerState *state, bool *out_has_more) {
    if (!state || !out_has_more || (!state->text && state->text_length)) return SSRNX_JST_NO_SUCH_ELEMENT;
    size_t position = state->position;
    if (position >= state->text_length) {
        *out_has_more = false;
        return SSRNX_JST_OK;
    }
    if (state->return_delimiters) {
        *out_has_more = true;
        return SSRNX_JST_OK;
    }
    SsrnxJavaStringTokenizerStatus status = java_text_tokenizer_skip_delimiters(state, &position);
    if (status != SSRNX_JST_OK) return status;
    *out_has_more = position < state->text_length;
    return SSRNX_JST_OK;
}

SsrnxJavaStringTokenizerStatus ssrnx_java_string_tokenizer_next(
    SsrnxJavaStringTokenizerState *state, size_t *out_start, size_t *out_length) {
    if (!state || !out_start || !out_length || (!state->text && state->text_length))
        return SSRNX_JST_NO_SUCH_ELEMENT;
    size_t position = state->position;
    if (position >= state->text_length) return SSRNX_JST_NO_SUCH_ELEMENT;
    if (!state->delimiters) return SSRNX_JST_NULL_DELIMITERS;

    size_t width = 0;
    if (state->return_delimiters && java_text_tokenizer_is_delimiter(state, position, &width)) {
        *out_start = position;
        *out_length = width;
        state->position = position + width;
        return SSRNX_JST_OK;
    }
    if (!state->return_delimiters) {
        const SsrnxJavaStringTokenizerStatus status = java_text_tokenizer_skip_delimiters(state, &position);
        if (status != SSRNX_JST_OK) return status;
        if (position >= state->text_length) {
            state->position = position;
            return SSRNX_JST_NO_SUCH_ELEMENT;
        }
    }

    const size_t start = position;
    while (position < state->text_length) {
        if (java_text_tokenizer_is_delimiter(state, position, &width)) break;
        position += width;
    }
    *out_start = start;
    *out_length = position - start;
    state->position = position;
    return SSRNX_JST_OK;
}

SsrnxJavaStringTokenizerStatus ssrnx_java_string_tokenizer_count(
    const SsrnxJavaStringTokenizerState *state, size_t *out_count) {
    if (!state || !out_count || (!state->text && state->text_length)) return SSRNX_JST_NO_SUCH_ELEMENT;
    SsrnxJavaStringTokenizerState copy = *state;
    size_t count = 0, start = 0, length = 0;
    for (;;) {
        const SsrnxJavaStringTokenizerStatus status = ssrnx_java_string_tokenizer_next(&copy, &start, &length);
        if (status == SSRNX_JST_NO_SUCH_ELEMENT) break;
        if (status != SSRNX_JST_OK) return status;
        count++;
    }
    *out_count = count;
    return SSRNX_JST_OK;
}



/* ---- java_regex ---- */
#pragma push_macro("JR_INF")
#pragma push_macro("JR_MAX_DEPTH")
#pragma push_macro("JR_MAX_GROUPS")
#pragma push_macro("JR_STACK_FALLBACK")
#pragma push_macro("JR_STACK_RESERVE")
/* java.util.regex engine: Java pattern syntax, leftmost-first backtracking.
 *
 * The compiled form is a node graph linked through `next`, in the style of
 * the reference implementation's Node chain: alternation branches join on a
 * shared successor, a general quantifier is a Loop node whose body ends in a
 * LoopTail, and single-code-point atoms under a quantifier use a counted
 * repeat that backtracks without recursion per iteration. Matching recurses
 * along the chain with a depth limit. */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Guest thread stack bounds (compat_threads.c); absent in host tests. */
extern bool ssrnx_bionic_current_stack_bounds(uintptr_t *out_low, uintptr_t *out_high) __attribute__((weak));

#define JR_INF UINT32_MAX
#define JR_MAX_DEPTH 20000u
/* Stack the backtracker may use: what the guest thread has left above this
 * reserve, else a fixed budget when the thread's bounds are unknown. */
#define JR_STACK_RESERVE (96u * 1024u)
#define JR_STACK_FALLBACK (256u * 1024u)
#define JR_MAX_GROUPS 512u

/* ------------------------------------------------------------ code point sets */

typedef struct java_regex_JrRange { uint32_t lo, hi; } java_regex_JrRange;
typedef struct java_regex_JrSet { java_regex_JrRange *r; size_t n, cap; } java_regex_JrSet;

static bool java_regex_set_add(java_regex_JrSet *s, uint32_t lo, uint32_t hi) {
    if (lo > hi) return true;
    if (s->n == s->cap) {
        const size_t next = s->cap ? s->cap * 2u : 8u;
        java_regex_JrRange *grown = realloc(s->r, next * sizeof(*grown));
        if (!grown) return false;
        s->r = grown; s->cap = next;
    }
    s->r[s->n].lo = lo; s->r[s->n].hi = hi; s->n++;
    return true;
}

static int java_regex_range_cmp(const void *a, const void *b) {
    const java_regex_JrRange *x = a, *y = b;
    return x->lo < y->lo ? -1 : x->lo > y->lo;
}

static void java_regex_set_normalize(java_regex_JrSet *s) {
    if (s->n < 2) return;
    qsort(s->r, s->n, sizeof(*s->r), java_regex_range_cmp);
    size_t out = 0;
    for (size_t i = 1; i < s->n; ++i) {
        if (s->r[i].lo <= s->r[out].hi + 1u && s->r[out].hi != UINT32_MAX) {
            if (s->r[i].hi > s->r[out].hi) s->r[out].hi = s->r[i].hi;
        } else s->r[++out] = s->r[i];
    }
    s->n = out + 1;
}

static bool java_regex_set_union(java_regex_JrSet *dst, const java_regex_JrSet *src) {
    for (size_t i = 0; i < src->n; ++i) if (!java_regex_set_add(dst, src->r[i].lo, src->r[i].hi)) return false;
    java_regex_set_normalize(dst);
    return true;
}

static bool java_regex_set_complement(java_regex_JrSet *s) {
    java_regex_set_normalize(s);
    java_regex_JrSet out = {0};
    uint32_t next = 0;
    for (size_t i = 0; i < s->n; ++i) {
        if (s->r[i].lo > next && !java_regex_set_add(&out, next, s->r[i].lo - 1u)) { free(out.r); return false; }
        next = s->r[i].hi + 1u;
    }
    if (next <= 0x10FFFFu && !java_regex_set_add(&out, next, 0x10FFFFu)) { free(out.r); return false; }
    free(s->r);
    *s = out;
    return true;
}

static bool java_regex_set_intersect(java_regex_JrSet *a, const java_regex_JrSet *b) {
    java_regex_set_normalize(a);
    java_regex_JrSet out = {0};
    for (size_t i = 0; i < a->n; ++i)
        for (size_t j = 0; j < b->n; ++j) {
            const uint32_t lo = a->r[i].lo > b->r[j].lo ? a->r[i].lo : b->r[j].lo;
            const uint32_t hi = a->r[i].hi < b->r[j].hi ? a->r[i].hi : b->r[j].hi;
            if (lo <= hi && !java_regex_set_add(&out, lo, hi)) { free(out.r); return false; }
        }
    java_regex_set_normalize(&out);
    free(a->r);
    *a = out;
    return true;
}

static bool java_regex_set_has(const java_regex_JrSet *s, uint32_t c) {
    size_t lo = 0, hi = s->n;
    while (lo < hi) {
        const size_t mid = (lo + hi) / 2u;
        if (c < s->r[mid].lo) hi = mid;
        else if (c > s->r[mid].hi) lo = mid + 1u;
        else return true;
    }
    return false;
}

/* Approximate Unicode letter blocks (\p{L}); scripts outside these report as
 * non-letters. */
static const java_regex_JrRange java_regex_k_letters[] = {
    {'A','Z'},{'a','z'},{0xAA,0xAA},{0xB5,0xB5},{0xBA,0xBA},{0xC0,0xD6},{0xD8,0xF6},{0xF8,0x2C1},
    {0x2C6,0x2D1},{0x2E0,0x2E4},{0x370,0x374},{0x376,0x37D},{0x386,0x386},{0x388,0x3FF},{0x400,0x481},
    {0x48A,0x52F},{0x531,0x556},{0x561,0x587},{0x5D0,0x5EA},{0x620,0x64A},{0x671,0x6D3},{0x904,0x939},
    {0x993,0x9B9},{0xE01,0xE30},{0x10A0,0x10FF},{0x1100,0x11FF},{0x1E00,0x1FBC},{0x1FC2,0x1FFC},
    {0x2C00,0x2CE4},{0x3041,0x3096},{0x30A1,0x30FA},{0x3105,0x312F},{0x3131,0x318E},{0x3400,0x4DBF},
    {0x4E00,0x9FFF},{0xA000,0xA48C},{0xAC00,0xD7A3},{0xF900,0xFAFF},{0xFF21,0xFF3A},{0xFF41,0xFF5A},
    {0xFF66,0xFFDC},{0x20000,0x2FA1F},
};
static const java_regex_JrRange java_regex_k_digits[] = {
    {'0','9'},{0x660,0x669},{0x6F0,0x6F9},{0x966,0x96F},{0xFF10,0xFF19},
};
static const java_regex_JrRange java_regex_k_punct[] = {
    {'!','/'},{':','@'},{'[','`'},{'{','~'},{0xA1,0xA1},{0xA7,0xA7},{0xAB,0xAB},{0xB6,0xB7},{0xBB,0xBB},
    {0xBF,0xBF},{0x2010,0x2027},{0x2030,0x205E},{0x3001,0x3003},{0x3008,0x3011},{0xFF01,0xFF0F},
};
static const java_regex_JrRange java_regex_k_space_separators[] = {
    {' ',' '},{0xA0,0xA0},{0x1680,0x1680},{0x2000,0x200A},{0x202F,0x202F},{0x205F,0x205F},{0x3000,0x3000},
};

static bool java_regex_set_add_table(java_regex_JrSet *s, const java_regex_JrRange *t, size_t n) {
    for (size_t i = 0; i < n; ++i) if (!java_regex_set_add(s, t[i].lo, t[i].hi)) return false;
    return true;
}

static bool java_regex_in_table(const java_regex_JrRange *t, size_t n, uint32_t c) {
    for (size_t i = 0; i < n; ++i) if (c >= t[i].lo && c <= t[i].hi) return true;
    return false;
}

static bool java_regex_cp_is_letter(uint32_t c) { return java_regex_in_table(java_regex_k_letters, sizeof(java_regex_k_letters) / sizeof(java_regex_k_letters[0]), c); }
static bool java_regex_cp_is_digit(uint32_t c) { return java_regex_in_table(java_regex_k_digits, sizeof(java_regex_k_digits) / sizeof(java_regex_k_digits[0]), c); }

/* ------------------------------------------------------------ case folding */

static uint32_t java_regex_fold_lower(uint32_t c, bool unicode) {
    if (c >= 'A' && c <= 'Z') return c + 32u;
    if (!unicode || c < 0x80) return c;
    return ssrnx_java_to_lower(c);
}

static uint32_t java_regex_fold_upper(uint32_t c, bool unicode) {
    if (c >= 'a' && c <= 'z') return c - 32u;
    if (!unicode || c < 0x80) return c;
    return ssrnx_java_to_upper(c);
}

static bool java_regex_cp_equal(uint32_t a, uint32_t b, bool icase, bool unicode) {
    if (a == b) return true;
    if (!icase) return false;
    return java_regex_fold_lower(a, unicode) == java_regex_fold_lower(b, unicode) || java_regex_fold_upper(a, unicode) == java_regex_fold_upper(b, unicode);
}

/* ------------------------------------------------------------ nodes */

typedef enum java_regex_JrKind {
    J_ACCEPT, J_NOP, J_CHAR, J_STRING, J_ANY, J_SET, J_BOL, J_EOL, J_WORDB, J_NWORDB,
    J_BEGIN, J_END, J_ENDZ, J_LASTEND, J_LINEBREAK, J_GROUP_OPEN, J_GROUP_CLOSE, J_ALT,
    J_LOOK, J_ATOMIC, J_SUB_ACCEPT, J_REPEAT, J_LOOP, J_LOOP_TAIL, J_BACKREF,
} java_regex_JrKind;

enum { JR_GREEDY, JR_LAZY, JR_POSSESSIVE };

typedef struct java_regex_JrNode java_regex_JrNode;
struct java_regex_JrNode {
    java_regex_JrKind kind;
    java_regex_JrNode *next;
    uint32_t c;
    uint16_t *str;
    size_t str_len;
    java_regex_JrSet set;
    int flags;
    int group;
    java_regex_JrNode **alts;
    size_t nalts;
    java_regex_JrNode *body;
    bool behind, negative;
    uint32_t min, max;
    int mode;
    int loop_id;
    java_regex_JrNode *loop;
    java_regex_JrNode *arena_next;
};

struct SsrnxJavaRegex {
    java_regex_JrNode *start;
    java_regex_JrNode *nodes;
    size_t groups;
    size_t loops;
    char **names;
    int32_t *name_groups;
    size_t name_count;
    int flags;
};

typedef struct java_regex_JrFrag { java_regex_JrNode *head, *tail; } java_regex_JrFrag;

typedef struct java_regex_JrParser {
    SsrnxJavaRegex *re;
    const uint16_t *p;
    size_t len, pos;
    int flags;
    char *error;
    size_t error_cap;
    int32_t error_index;
    bool failed;
    size_t depth;
} java_regex_JrParser;

static java_regex_JrNode *java_regex_node_new(java_regex_JrParser *ps, java_regex_JrKind kind) {
    java_regex_JrNode *n = calloc(1, sizeof(*n));
    if (!n) { ps->failed = true; snprintf(ps->error, ps->error_cap, "out of memory"); return NULL; }
    n->kind = kind;
    n->flags = ps->flags;
    n->arena_next = ps->re->nodes;
    ps->re->nodes = n;
    return n;
}

static void java_regex_fail_at(java_regex_JrParser *ps, const char *message, size_t index) {
    if (ps->failed) return;
    ps->failed = true;
    ps->error_index = (int32_t)index;
    snprintf(ps->error, ps->error_cap, "%s", message);
}

static bool java_regex_at_end(const java_regex_JrParser *ps) { return ps->pos >= ps->len; }
static uint32_t java_regex_peek(const java_regex_JrParser *ps) { return java_regex_at_end(ps) ? 0 : ps->p[ps->pos]; }

static uint32_t java_regex_take_cp(java_regex_JrParser *ps) {
    uint32_t c = ps->p[ps->pos++];
    if (c >= 0xD800 && c <= 0xDBFF && ps->pos < ps->len && ps->p[ps->pos] >= 0xDC00 && ps->p[ps->pos] <= 0xDFFF)
        c = 0x10000u + ((c - 0xD800u) << 10) + (ps->p[ps->pos++] - 0xDC00u);
    return c;
}

static void java_regex_skip_comments(java_regex_JrParser *ps) {
    if (!(ps->flags & SSRNX_JREGEX_COMMENTS)) return;
    while (!java_regex_at_end(ps)) {
        const uint32_t c = java_regex_peek(ps);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == 0x0B) ps->pos++;
        else if (c == '#') { while (!java_regex_at_end(ps) && java_regex_peek(ps) != '\n') ps->pos++; }
        else break;
    }
}

static java_regex_JrFrag java_regex_frag_single(java_regex_JrNode *n) { java_regex_JrFrag f = {n, n}; return f; }

static bool java_regex_frag_ok(java_regex_JrFrag f) { return f.head && f.tail; }

/* ------------------------------------------------------------ class parsing */

static bool java_regex_add_predefined(java_regex_JrParser *ps, java_regex_JrSet *s, uint32_t esc) {
    java_regex_JrSet tmp = {0};
    bool ok = true, negate = false;
    switch (esc) {
        case 'D': negate = true; /* fallthrough */
        case 'd': ok = java_regex_set_add(&tmp, '0', '9'); break;
        case 'S': negate = true; /* fallthrough */
        case 's': ok = java_regex_set_add(&tmp, '\t', '\r') && java_regex_set_add(&tmp, ' ', ' '); break;
        case 'W': negate = true; /* fallthrough */
        case 'w': ok = java_regex_set_add(&tmp, 'a', 'z') && java_regex_set_add(&tmp, 'A', 'Z') && java_regex_set_add(&tmp, '0', '9') &&
                       java_regex_set_add(&tmp, '_', '_'); break;
        case 'H': negate = true; /* fallthrough */
        case 'h': ok = java_regex_set_add(&tmp, ' ', ' ') && java_regex_set_add(&tmp, '\t', '\t') && java_regex_set_add(&tmp, 0xA0, 0xA0) &&
                       java_regex_set_add(&tmp, 0x1680, 0x1680) && java_regex_set_add(&tmp, 0x180E, 0x180E) &&
                       java_regex_set_add(&tmp, 0x2000, 0x200A) && java_regex_set_add(&tmp, 0x202F, 0x202F) &&
                       java_regex_set_add(&tmp, 0x205F, 0x205F) && java_regex_set_add(&tmp, 0x3000, 0x3000); break;
        case 'V': negate = true; /* fallthrough */
        case 'v': ok = java_regex_set_add(&tmp, '\n', '\r') && java_regex_set_add(&tmp, 0x85, 0x85) && java_regex_set_add(&tmp, 0x2028, 0x2029); break;
        default: free(tmp.r); return false;
    }
    if (ok && (ps->flags & SSRNX_JREGEX_UNICODE_CHARACTER_CLASS) && (esc == 'w' || esc == 'W')) {
        ok = java_regex_set_add_table(&tmp, java_regex_k_letters, sizeof(java_regex_k_letters) / sizeof(java_regex_k_letters[0])) &&
             java_regex_set_add_table(&tmp, java_regex_k_digits, sizeof(java_regex_k_digits) / sizeof(java_regex_k_digits[0]));
    }
    if (ok && (ps->flags & SSRNX_JREGEX_UNICODE_CHARACTER_CLASS) && (esc == 'd' || esc == 'D'))
        ok = java_regex_set_add_table(&tmp, java_regex_k_digits, sizeof(java_regex_k_digits) / sizeof(java_regex_k_digits[0]));
    java_regex_set_normalize(&tmp);
    if (ok && negate) ok = java_regex_set_complement(&tmp);
    if (ok) ok = java_regex_set_union(s, &tmp);
    free(tmp.r);
    if (!ok) java_regex_fail_at(ps, "out of memory", ps->pos);
    return ok;
}

/* \p{Name} / \P{Name}: POSIX character classes (US-ASCII), java.lang.Character
 * classes and the general categories/binary properties apps use. */
static bool java_regex_property_set(const char *name, java_regex_JrSet *s) {
    const char *n = name;
    if (strncmp(n, "Is", 2) == 0) n += 2;
    else if (strncmp(n, "general_category=", 17) == 0) n += 17;
    else if (strncmp(n, "gc=", 3) == 0) n += 3;
    bool ok = true;
    if (!strcmp(n, "Lower")) ok = java_regex_set_add(s, 'a', 'z');
    else if (!strcmp(n, "Upper")) ok = java_regex_set_add(s, 'A', 'Z');
    else if (!strcmp(n, "ASCII")) ok = java_regex_set_add(s, 0, 0x7F);
    else if (!strcmp(n, "Alpha")) ok = java_regex_set_add(s, 'a', 'z') && java_regex_set_add(s, 'A', 'Z');
    else if (!strcmp(n, "Digit")) ok = java_regex_set_add(s, '0', '9');
    else if (!strcmp(n, "Alnum")) ok = java_regex_set_add(s, 'a', 'z') && java_regex_set_add(s, 'A', 'Z') && java_regex_set_add(s, '0', '9');
    else if (!strcmp(n, "Punct")) ok = java_regex_set_add(s, '!', '/') && java_regex_set_add(s, ':', '@') && java_regex_set_add(s, '[', '`') && java_regex_set_add(s, '{', '~');
    else if (!strcmp(n, "Graph")) ok = java_regex_set_add(s, '!', '~');
    else if (!strcmp(n, "Print")) ok = java_regex_set_add(s, ' ', '~');
    else if (!strcmp(n, "Blank")) ok = java_regex_set_add(s, ' ', ' ') && java_regex_set_add(s, '\t', '\t');
    else if (!strcmp(n, "Cntrl")) ok = java_regex_set_add(s, 0, 0x1F) && java_regex_set_add(s, 0x7F, 0x7F);
    else if (!strcmp(n, "XDigit")) ok = java_regex_set_add(s, '0', '9') && java_regex_set_add(s, 'a', 'f') && java_regex_set_add(s, 'A', 'F');
    else if (!strcmp(n, "Space") || !strcmp(n, "javaWhitespace"))
        ok = java_regex_set_add(s, '\t', '\r') && java_regex_set_add(s, ' ', ' ') && java_regex_set_add(s, 0x1C, 0x1F);
    else if (!strcmp(n, "L") || !strcmp(n, "Letter") || !strcmp(n, "Alphabetic") || !strcmp(n, "javaLetter") ||
             !strcmp(n, "IsAlphabetic"))
        ok = java_regex_set_add_table(s, java_regex_k_letters, sizeof(java_regex_k_letters) / sizeof(java_regex_k_letters[0]));
    else if (!strcmp(n, "N") || !strcmp(n, "Nd") || !strcmp(n, "javaDigit"))
        ok = java_regex_set_add_table(s, java_regex_k_digits, sizeof(java_regex_k_digits) / sizeof(java_regex_k_digits[0]));
    else if (!strcmp(n, "javaLetterOrDigit"))
        ok = java_regex_set_add_table(s, java_regex_k_letters, sizeof(java_regex_k_letters) / sizeof(java_regex_k_letters[0])) &&
             java_regex_set_add_table(s, java_regex_k_digits, sizeof(java_regex_k_digits) / sizeof(java_regex_k_digits[0]));
    else if (!strcmp(n, "P") || !strcmp(n, "Punctuation"))
        ok = java_regex_set_add_table(s, java_regex_k_punct, sizeof(java_regex_k_punct) / sizeof(java_regex_k_punct[0]));
    else if (!strcmp(n, "Z") || !strcmp(n, "Zs"))
        ok = java_regex_set_add_table(s, java_regex_k_space_separators, sizeof(java_regex_k_space_separators) / sizeof(java_regex_k_space_separators[0]));
    else if (!strcmp(n, "White_Space") || !strcmp(n, "WhiteSpace"))
        ok = java_regex_set_add(s, '\t', '\r') && java_regex_set_add(s, 0x85, 0x85) &&
             java_regex_set_add_table(s, java_regex_k_space_separators, sizeof(java_regex_k_space_separators) / sizeof(java_regex_k_space_separators[0])) &&
             java_regex_set_add(s, 0x2028, 0x2029);
    else if (!strcmp(n, "Lu") || !strcmp(n, "Uppercase") || !strcmp(n, "javaUpperCase") ||
             !strcmp(n, "Ll") || !strcmp(n, "Lowercase") || !strcmp(n, "javaLowerCase")) {
        const bool upper = n[1] == 'u' || n[0] == 'U' || !strcmp(n, "javaUpperCase");
        for (size_t i = 0; ok && i < sizeof(java_regex_k_letters) / sizeof(java_regex_k_letters[0]); ++i)
            for (uint32_t c = java_regex_k_letters[i].lo; ok && c <= java_regex_k_letters[i].hi && c < 0x2000u; ++c)
                if ((upper ? java_regex_fold_lower(c, true) != c : java_regex_fold_upper(c, true) != c)) ok = java_regex_set_add(s, c, c);
    } else if (!strcmp(n, "InBasicLatin") || !strcmp(n, "BasicLatin")) ok = java_regex_set_add(s, 0, 0x7F);
    else if (!strcmp(n, "InLatin-1Supplement")) ok = java_regex_set_add(s, 0x80, 0xFF);
    else if (!strcmp(n, "Latin")) ok = java_regex_set_add(s, 'A', 'Z') && java_regex_set_add(s, 'a', 'z') && java_regex_set_add(s, 0xC0, 0x24F);
    else if (!strcmp(n, "Han")) ok = java_regex_set_add(s, 0x3400, 0x4DBF) && java_regex_set_add(s, 0x4E00, 0x9FFF) && java_regex_set_add(s, 0xF900, 0xFAFF);
    else if (!strcmp(n, "Hiragana")) ok = java_regex_set_add(s, 0x3041, 0x309F);
    else if (!strcmp(n, "Katakana")) ok = java_regex_set_add(s, 0x30A0, 0x30FF);
    else if (!strcmp(n, "Hangul")) ok = java_regex_set_add(s, 0x1100, 0x11FF) && java_regex_set_add(s, 0xAC00, 0xD7A3);
    else if (!strcmp(n, "Cyrillic")) ok = java_regex_set_add(s, 0x400, 0x52F);
    else if (!strcmp(n, "Greek")) ok = java_regex_set_add(s, 0x370, 0x3FF);
    else if (!strcmp(n, "Arabic")) ok = java_regex_set_add(s, 0x600, 0x6FF);
    else if (!strcmp(n, "Hebrew")) ok = java_regex_set_add(s, 0x590, 0x5FF);
    else if (!strcmp(n, "Thai")) ok = java_regex_set_add(s, 0xE00, 0xE7F);
    else if (!strcmp(n, "Cc")) ok = java_regex_set_add(s, 0, 0x1F) && java_regex_set_add(s, 0x7F, 0x9F);
    else return false;
    java_regex_set_normalize(s);
    return ok;
}

static bool java_regex_parse_property(java_regex_JrParser *ps, java_regex_JrSet *s, bool negate) {
    const size_t start = ps->pos;
    char name[64];
    size_t n = 0;
    if (java_regex_peek(ps) == '{') {
        ps->pos++;
        while (!java_regex_at_end(ps) && java_regex_peek(ps) != '}' && n + 1 < sizeof(name)) name[n++] = (char)java_regex_take_cp(ps);
        if (java_regex_peek(ps) != '}') { java_regex_fail_at(ps, "Unclosed character family", start); return false; }
        ps->pos++;
    } else if (!java_regex_at_end(ps)) name[n++] = (char)java_regex_take_cp(ps);
    name[n] = '\0';
    java_regex_JrSet tmp = {0};
    if (!java_regex_property_set(name, &tmp)) {
        free(tmp.r);
        char msg[160];
        snprintf(msg, sizeof(msg), "Unknown character property name {%s}", name);
        java_regex_fail_at(ps, msg, start);
        return false;
    }
    bool ok = true;
    if (negate) ok = java_regex_set_complement(&tmp);
    if (ok) ok = java_regex_set_union(s, &tmp);
    free(tmp.r);
    return ok;
}

static uint32_t java_regex_hex_value(uint32_t c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10u;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10u;
    return 16u;
}

/* Escape that denotes one code point; false when esc is not such an escape. */
static bool java_regex_escape_literal(java_regex_JrParser *ps, uint32_t esc, uint32_t *out) {
    switch (esc) {
        case 't': *out = '\t'; return true;
        case 'n': *out = '\n'; return true;
        case 'r': *out = '\r'; return true;
        case 'f': *out = '\f'; return true;
        case 'a': *out = 0x07; return true;
        case 'e': *out = 0x1B; return true;
        case 'c':
            if (java_regex_at_end(ps)) { java_regex_fail_at(ps, "Illegal control escape sequence", ps->pos); return false; }
            *out = java_regex_take_cp(ps) ^ 64u;
            return true;
        case '0': {
            uint32_t v = 0;
            size_t digits = 0;
            while (digits < 3 && !java_regex_at_end(ps) && java_regex_peek(ps) >= '0' && java_regex_peek(ps) <= '7') {
                const uint32_t next = v * 8u + (java_regex_peek(ps) - '0');
                if (next > 0377u) break;
                v = next; ps->pos++; digits++;
            }
            if (!digits) { java_regex_fail_at(ps, "Illegal octal escape sequence", ps->pos); return false; }
            *out = v;
            return true;
        }
        case 'x': {
            uint32_t v = 0;
            if (java_regex_peek(ps) == '{') {
                ps->pos++;
                size_t digits = 0;
                while (!java_regex_at_end(ps) && java_regex_hex_value(java_regex_peek(ps)) < 16u && v <= 0x10FFFFu) { v = v * 16u + java_regex_hex_value(java_regex_take_cp(ps)); digits++; }
                if (!digits || java_regex_peek(ps) != '}' || v > 0x10FFFFu) { java_regex_fail_at(ps, "Illegal hexadecimal escape sequence", ps->pos); return false; }
                ps->pos++;
            } else {
                for (int i = 0; i < 2; ++i) {
                    if (java_regex_at_end(ps) || java_regex_hex_value(java_regex_peek(ps)) >= 16u) { java_regex_fail_at(ps, "Illegal hexadecimal escape sequence", ps->pos); return false; }
                    v = v * 16u + java_regex_hex_value(java_regex_take_cp(ps));
                }
            }
            *out = v;
            return true;
        }
        case 'u': {
            uint32_t v = 0;
            for (int i = 0; i < 4; ++i) {
                if (java_regex_at_end(ps) || java_regex_hex_value(java_regex_peek(ps)) >= 16u) { java_regex_fail_at(ps, "Illegal Unicode escape sequence", ps->pos); return false; }
                v = v * 16u + java_regex_hex_value(java_regex_take_cp(ps));
            }
            /* 😀: a surrogate pair written as two escapes. */
            if (v >= 0xD800 && v <= 0xDBFF && ps->pos + 6 <= ps->len && ps->p[ps->pos] == '\\' && ps->p[ps->pos + 1] == 'u') {
                uint32_t low = 0;
                bool hex = true;
                for (int i = 0; i < 4; ++i) {
                    const uint32_t h = java_regex_hex_value(ps->p[ps->pos + 2 + (size_t)i]);
                    if (h >= 16u) { hex = false; break; }
                    low = low * 16u + h;
                }
                if (hex && low >= 0xDC00 && low <= 0xDFFF) {
                    ps->pos += 6;
                    v = 0x10000u + ((v - 0xD800u) << 10) + (low - 0xDC00u);
                }
            }
            *out = v;
            return true;
        }
        default:
            if ((esc >= 'a' && esc <= 'z') || (esc >= 'A' && esc <= 'Z') || (esc >= '0' && esc <= '9')) return false;
            *out = esc;
            return true;
    }
}

static bool java_regex_parse_class_body(java_regex_JrParser *ps, java_regex_JrSet *out);

/* One operand of a class: nested [..], an escape, or a literal/range. */
static bool java_regex_parse_class_item(java_regex_JrParser *ps, java_regex_JrSet *s) {
    const size_t start = ps->pos;
    if (java_regex_peek(ps) == '[') {
        ps->pos++;
        java_regex_JrSet nested = {0};
        const bool ok = java_regex_parse_class_body(ps, &nested) && java_regex_set_union(s, &nested);
        free(nested.r);
        return ok;
    }
    uint32_t lo;
    uint32_t c = java_regex_take_cp(ps);
    if (c == '\\') {
        if (java_regex_at_end(ps)) { java_regex_fail_at(ps, "Unexpected internal error", start); return false; }
        const uint32_t esc = java_regex_take_cp(ps);
        if (strchr("dDsSwWhHvV", (int)esc) && esc < 128) return java_regex_add_predefined(ps, s, esc);
        if (esc == 'p' || esc == 'P') return java_regex_parse_property(ps, s, esc == 'P');
        if (esc == 'Q') {
            while (!java_regex_at_end(ps)) {
                if (java_regex_peek(ps) == '\\' && ps->pos + 1 < ps->len && ps->p[ps->pos + 1] == 'E') { ps->pos += 2; break; }
                const uint32_t q = java_regex_take_cp(ps);
                if (!java_regex_set_add(s, q, q)) return false;
            }
            java_regex_set_normalize(s);
            return true;
        }
        if (!java_regex_escape_literal(ps, esc, &lo)) {
            if (!ps->failed) java_regex_fail_at(ps, "Illegal/unsupported escape sequence", start + 1);
            return false;
        }
    } else lo = c;
    uint32_t hi = lo;
    if (java_regex_peek(ps) == '-' && ps->pos + 1 < ps->len && ps->p[ps->pos + 1] != ']' && ps->p[ps->pos + 1] != '[') {
        ps->pos++;
        const uint32_t d = java_regex_take_cp(ps);
        if (d == '\\') {
            if (java_regex_at_end(ps)) { java_regex_fail_at(ps, "Illegal character range", ps->pos); return false; }
            const uint32_t esc = java_regex_take_cp(ps);
            if (!java_regex_escape_literal(ps, esc, &hi)) { if (!ps->failed) java_regex_fail_at(ps, "Illegal character range", ps->pos); return false; }
        } else hi = d;
        if (hi < lo) { java_regex_fail_at(ps, "Illegal character range", ps->pos - 1); return false; }
    }
    if (!java_regex_set_add(s, lo, hi)) return false;
    if (ps->flags & SSRNX_JREGEX_CASE_INSENSITIVE) {
        const bool unicode = (ps->flags & SSRNX_JREGEX_UNICODE_CASE) != 0;
        const uint32_t top = hi - lo > 0x3000u ? lo + 0x3000u : hi;
        for (uint32_t x = lo; x <= top; ++x) {
            const uint32_t l = java_regex_fold_lower(x, unicode), u = java_regex_fold_upper(x, unicode);
            if ((l != x && !java_regex_set_add(s, l, l)) || (u != x && !java_regex_set_add(s, u, u))) return false;
        }
    }
    java_regex_set_normalize(s);
    return true;
}

/* After '[': items up to the matching ']', with ^ negation and && intersection. */
static bool java_regex_parse_class_body(java_regex_JrParser *ps, java_regex_JrSet *out) {
    const size_t start = ps->pos - 1;
    if (++ps->depth > 200u) { java_regex_fail_at(ps, "Pattern too complex", start); return false; }
    bool negate = false;
    if (java_regex_peek(ps) == '^') { negate = true; ps->pos++; }
    java_regex_JrSet acc = {0};
    bool first = true, ok = true, closed = false;
    while (ok && !java_regex_at_end(ps)) {
        const uint32_t c = java_regex_peek(ps);
        if (c == ']' && !first) { ps->pos++; closed = true; break; }
        if (c == '&' && ps->pos + 1 < ps->len && ps->p[ps->pos + 1] == '&') {
            ps->pos += 2;
            java_regex_JrSet rhs = {0};
            /* The right operand runs to this class's closing bracket. */
            while (ok && !java_regex_at_end(ps) && java_regex_peek(ps) != ']') {
                if (java_regex_peek(ps) == '&' && ps->pos + 1 < ps->len && ps->p[ps->pos + 1] == '&') break;
                ok = java_regex_parse_class_item(ps, &rhs);
            }
            if (ok) ok = java_regex_set_intersect(&acc, &rhs);
            free(rhs.r);
            first = false;
            continue;
        }
        ok = java_regex_parse_class_item(ps, &acc);
        first = false;
    }
    ps->depth--;
    if (ok && !closed) { java_regex_fail_at(ps, "Unclosed character class", start); ok = false; }
    if (ok && negate) ok = java_regex_set_complement(&acc);
    if (ok) { java_regex_set_normalize(&acc); *out = acc; } else free(acc.r);
    if (!ok && !ps->failed) java_regex_fail_at(ps, "out of memory", start);
    return ok;
}

/* ------------------------------------------------------------ pattern parsing */

static java_regex_JrFrag java_regex_parse_alternation(java_regex_JrParser *ps);

static java_regex_JrFrag java_regex_set_atom(java_regex_JrParser *ps, java_regex_JrSet *s) {
    java_regex_JrNode *n = java_regex_node_new(ps, J_SET);
    if (!n) { free(s->r); java_regex_JrFrag none = {0}; return none; }
    n->set = *s;
    n->flags &= ~SSRNX_JREGEX_CASE_INSENSITIVE; /* folded into the set at parse time */
    return java_regex_frag_single(n);
}

static java_regex_JrFrag java_regex_char_atom(java_regex_JrParser *ps, uint32_t c) {
    java_regex_JrNode *n = java_regex_node_new(ps, J_CHAR);
    if (n) n->c = c;
    return java_regex_frag_single(n);
}

static int java_regex_group_by_name(const SsrnxJavaRegex *re, const char *name) {
    for (size_t i = 0; i < re->name_count; ++i) if (!strcmp(re->names[i], name)) return re->name_groups[i];
    return -1;
}

static bool java_regex_read_group_name(java_regex_JrParser *ps, char *name, size_t cap) {
    size_t n = 0;
    const size_t start = ps->pos;
    while (!java_regex_at_end(ps) && java_regex_peek(ps) != '>') {
        const uint32_t c = java_regex_take_cp(ps);
        const bool valid = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (n > 0 && c >= '0' && c <= '9');
        if (!valid || n + 1 >= cap) { java_regex_fail_at(ps, "named capturing group is missing trailing '>'", start); return false; }
        name[n++] = (char)c;
    }
    if (java_regex_at_end(ps) || n == 0) { java_regex_fail_at(ps, "named capturing group is missing trailing '>'", start); return false; }
    ps->pos++;
    name[n] = '\0';
    return true;
}

static java_regex_JrFrag java_regex_capture_group(java_regex_JrParser *ps, int group, java_regex_JrFrag inner) {
    java_regex_JrFrag none = {0};
    if (!java_regex_frag_ok(inner)) return none;
    java_regex_JrNode *open = java_regex_node_new(ps, J_GROUP_OPEN), *close = java_regex_node_new(ps, J_GROUP_CLOSE);
    if (!open || !close) return none;
    open->group = close->group = group;
    open->next = inner.head;
    inner.tail->next = close;
    java_regex_JrFrag f = {open, close};
    return f;
}

/* Wrap body in a sub-match node (lookaround / atomic group). */
static java_regex_JrFrag java_regex_sub_match(java_regex_JrParser *ps, java_regex_JrKind kind, java_regex_JrFrag inner, bool behind, bool negative) {
    java_regex_JrFrag none = {0};
    if (!java_regex_frag_ok(inner)) return none;
    java_regex_JrNode *n = java_regex_node_new(ps, kind), *accept = java_regex_node_new(ps, J_SUB_ACCEPT);
    if (!n || !accept) return none;
    inner.tail->next = accept;
    n->body = inner.head;
    n->behind = behind;
    n->negative = negative;
    return java_regex_frag_single(n);
}

static bool java_regex_parse_inline_flags(java_regex_JrParser *ps, int *flags) {
    bool on = true;
    while (!java_regex_at_end(ps)) {
        const uint32_t c = java_regex_peek(ps);
        int bit = 0;
        switch (c) {
            case 'i': bit = SSRNX_JREGEX_CASE_INSENSITIVE; break;
            case 'd': bit = SSRNX_JREGEX_UNIX_LINES; break;
            case 'm': bit = SSRNX_JREGEX_MULTILINE; break;
            case 's': bit = SSRNX_JREGEX_DOTALL; break;
            case 'u': bit = SSRNX_JREGEX_UNICODE_CASE; break;
            case 'x': bit = SSRNX_JREGEX_COMMENTS; break;
            case 'U': bit = SSRNX_JREGEX_UNICODE_CHARACTER_CLASS; break;
            case '-': on = false; ps->pos++; continue;
            default: return true;
        }
        ps->pos++;
        if (on) *flags |= bit; else *flags &= ~bit;
    }
    return true;
}

static java_regex_JrFrag java_regex_parse_group(java_regex_JrParser *ps) {
    java_regex_JrFrag none = {0};
    const size_t start = ps->pos - 1;
    if (java_regex_peek(ps) != '?') {
        const int group = (int)++ps->re->groups;
        if (ps->re->groups > JR_MAX_GROUPS) { java_regex_fail_at(ps, "Too many groups", start); return none; }
        const int saved = ps->flags;
        java_regex_JrFrag inner = java_regex_parse_alternation(ps);
        ps->flags = saved;
        if (ps->failed) return none;
        if (java_regex_peek(ps) != ')') { java_regex_fail_at(ps, "Unclosed group", ps->len); return none; }
        ps->pos++;
        return java_regex_capture_group(ps, group, inner);
    }
    ps->pos++;
    const uint32_t kind = java_regex_at_end(ps) ? 0 : java_regex_take_cp(ps);
    const int saved = ps->flags;
    java_regex_JrFrag result = none;
    if (kind == ':' || kind == '=' || kind == '!' || kind == '>') {
        java_regex_JrFrag inner = java_regex_parse_alternation(ps);
        ps->flags = saved;
        if (ps->failed) return none;
        if (kind == ':') result = inner;
        else if (kind == '>') result = java_regex_sub_match(ps, J_ATOMIC, inner, false, false);
        else result = java_regex_sub_match(ps, J_LOOK, inner, false, kind == '!');
    } else if (kind == '<') {
        const uint32_t next = java_regex_peek(ps);
        if (next == '=' || next == '!') {
            ps->pos++;
            java_regex_JrFrag inner = java_regex_parse_alternation(ps);
            ps->flags = saved;
            if (ps->failed) return none;
            result = java_regex_sub_match(ps, J_LOOK, inner, true, next == '!');
        } else {
            char name[64];
            if (!java_regex_read_group_name(ps, name, sizeof(name))) return none;
            if (java_regex_group_by_name(ps->re, name) >= 0) {
                char msg[160];
                snprintf(msg, sizeof(msg), "Named capturing group <%s> is already defined", name);
                java_regex_fail_at(ps, msg, ps->pos - 1);
                return none;
            }
            const int group = (int)++ps->re->groups;
            char **names = realloc(ps->re->names, (ps->re->name_count + 1) * sizeof(*names));
            int32_t *groups = names ? realloc(ps->re->name_groups, (ps->re->name_count + 1) * sizeof(*groups)) : NULL;
            if (names) ps->re->names = names;
            if (groups) ps->re->name_groups = groups;
            char *copy = groups ? strdup(name) : NULL;
            if (!copy) { java_regex_fail_at(ps, "out of memory", start); return none; }
            ps->re->names[ps->re->name_count] = copy;
            ps->re->name_groups[ps->re->name_count++] = group;
            java_regex_JrFrag inner = java_regex_parse_alternation(ps);
            ps->flags = saved;
            if (ps->failed) return none;
            if (java_regex_peek(ps) != ')') { java_regex_fail_at(ps, "Unclosed group", ps->len); return none; }
            ps->pos++;
            return java_regex_capture_group(ps, group, inner);
        }
    } else {
        /* (?flags) or (?flags:X) */
        ps->pos--;
        int flags = ps->flags;
        java_regex_parse_inline_flags(ps, &flags);
        if (java_regex_peek(ps) == ')') {
            ps->pos++;
            ps->flags = flags; /* applies to the rest of the enclosing group */
            java_regex_JrNode *nop = java_regex_node_new(ps, J_NOP);
            return java_regex_frag_single(nop);
        }
        if (java_regex_peek(ps) != ':') { java_regex_fail_at(ps, "Unknown inline modifier", ps->pos); return none; }
        ps->pos++;
        ps->flags = flags;
        result = java_regex_parse_alternation(ps);
        ps->flags = saved;
        if (ps->failed) return none;
    }
    if (java_regex_peek(ps) != ')') { java_regex_fail_at(ps, "Unclosed group", ps->len); return none; }
    ps->pos++;
    return result;
}

static java_regex_JrFrag java_regex_parse_escape(java_regex_JrParser *ps) {
    java_regex_JrFrag none = {0};
    const size_t start = ps->pos - 1;
    if (java_regex_at_end(ps)) { java_regex_fail_at(ps, "Unexpected internal error", start); return none; }
    const uint32_t esc = java_regex_take_cp(ps);
    java_regex_JrKind anchor = J_NOP;
    switch (esc) {
        case 'b': anchor = J_WORDB; break;
        case 'B': anchor = J_NWORDB; break;
        case 'A': anchor = J_BEGIN; break;
        case 'z': anchor = J_END; break;
        case 'Z': anchor = J_ENDZ; break;
        case 'G': anchor = J_LASTEND; break;
        case 'R': anchor = J_LINEBREAK; break;
        default: break;
    }
    if (anchor != J_NOP) return java_regex_frag_single(java_regex_node_new(ps, anchor));
    if (esc < 128 && strchr("dDsSwWhHvV", (int)esc)) {
        java_regex_JrSet s = {0};
        if (!java_regex_add_predefined(ps, &s, esc)) { free(s.r); return none; }
        return java_regex_set_atom(ps, &s);
    }
    if (esc == 'p' || esc == 'P') {
        java_regex_JrSet s = {0};
        if (!java_regex_parse_property(ps, &s, esc == 'P')) { free(s.r); return none; }
        return java_regex_set_atom(ps, &s);
    }
    if (esc == 'Q') {
        const size_t begin = ps->pos;
        size_t end = ps->len;
        for (size_t i = begin; i + 1 < ps->len; ++i)
            if (ps->p[i] == '\\' && ps->p[i + 1] == 'E') { end = i; break; }
        java_regex_JrNode *n = java_regex_node_new(ps, J_STRING);
        if (!n) return none;
        n->str_len = end - begin;
        n->str = malloc((n->str_len ? n->str_len : 1u) * sizeof(uint16_t));
        if (!n->str) { java_regex_fail_at(ps, "out of memory", start); return none; }
        memcpy(n->str, ps->p + begin, n->str_len * sizeof(uint16_t));
        ps->pos = end < ps->len ? end + 2 : ps->len;
        return java_regex_frag_single(n);
    }
    if (esc == 'k') {
        if (java_regex_peek(ps) != '<') { java_regex_fail_at(ps, "\\k is not followed by '<' for named capturing group", ps->pos); return none; }
        ps->pos++;
        char name[64];
        if (!java_regex_read_group_name(ps, name, sizeof(name))) return none;
        const int group = java_regex_group_by_name(ps->re, name);
        if (group < 0) {
            char msg[160];
            snprintf(msg, sizeof(msg), "named capturing group <%s> does not exist", name);
            java_regex_fail_at(ps, msg, ps->pos - 1);
            return none;
        }
        java_regex_JrNode *n = java_regex_node_new(ps, J_BACKREF);
        if (n) n->group = group;
        return java_regex_frag_single(n);
    }
    if (esc >= '1' && esc <= '9') {
        /* Take further digits while the result names an existing group. */
        int group = (int)(esc - '0');
        while (!java_regex_at_end(ps) && java_regex_peek(ps) >= '0' && java_regex_peek(ps) <= '9') {
            const int next = group * 10 + (int)(java_regex_peek(ps) - '0');
            if ((size_t)next > ps->re->groups) break;
            group = next;
            ps->pos++;
        }
        java_regex_JrNode *n = java_regex_node_new(ps, J_BACKREF);
        if (n) n->group = group;
        return java_regex_frag_single(n);
    }
    uint32_t c;
    if (!java_regex_escape_literal(ps, esc, &c)) {
        if (!ps->failed) java_regex_fail_at(ps, "Illegal/unsupported escape sequence", start + 1);
        return none;
    }
    return java_regex_char_atom(ps, c);
}

static bool java_regex_parse_number(java_regex_JrParser *ps, uint32_t *out) {
    size_t digits = 0;
    uint64_t v = 0;
    while (!java_regex_at_end(ps) && java_regex_peek(ps) >= '0' && java_regex_peek(ps) <= '9') {
        v = v * 10u + (java_regex_peek(ps) - '0');
        if (v > 0x7FFFFFFFu) v = 0x7FFFFFFFu;
        ps->pos++; digits++;
    }
    *out = (uint32_t)v;
    return digits > 0;
}

static bool java_regex_is_simple_atom(const java_regex_JrFrag *f) {
    return f->head == f->tail && (f->head->kind == J_CHAR || f->head->kind == J_ANY || f->head->kind == J_SET);
}

static java_regex_JrFrag java_regex_apply_quantifier(java_regex_JrParser *ps, java_regex_JrFrag atom, uint32_t min, uint32_t max, int mode) {
    java_regex_JrFrag none = {0};
    if (!java_regex_frag_ok(atom)) return none;
    if (java_regex_is_simple_atom(&atom)) {
        java_regex_JrNode *r = java_regex_node_new(ps, J_REPEAT);
        if (!r) return none;
        r->body = atom.head;
        r->min = min; r->max = max; r->mode = mode;
        return java_regex_frag_single(r);
    }
    java_regex_JrNode *loop = java_regex_node_new(ps, J_LOOP), *tail = java_regex_node_new(ps, J_LOOP_TAIL);
    if (!loop || !tail) return none;
    loop->body = atom.head;
    loop->min = min; loop->max = max;
    loop->mode = mode == JR_LAZY ? JR_LAZY : JR_GREEDY;
    loop->loop_id = (int)ps->re->loops++;
    atom.tail->next = tail;
    tail->loop = loop;
    java_regex_JrFrag f = {loop, loop};
    if (mode == JR_POSSESSIVE) return java_regex_sub_match(ps, J_ATOMIC, f, false, false);
    return f;
}

static java_regex_JrFrag java_regex_parse_quantified(java_regex_JrParser *ps, java_regex_JrFrag atom) {
    for (;;) {
        java_regex_skip_comments(ps);
        if (java_regex_at_end(ps) || ps->failed) return atom;
        const uint32_t c = java_regex_peek(ps);
        uint32_t min, max;
        const size_t start = ps->pos;
        if (c == '*') { min = 0; max = JR_INF; ps->pos++; }
        else if (c == '+') { min = 1; max = JR_INF; ps->pos++; }
        else if (c == '?') { min = 0; max = 1; ps->pos++; }
        else if (c == '{') {
            ps->pos++;
            if (!java_regex_parse_number(ps, &min)) { java_regex_fail_at(ps, "Illegal repetition", start); java_regex_JrFrag none = {0}; return none; }
            max = min;
            if (java_regex_peek(ps) == ',') {
                ps->pos++;
                if (!java_regex_parse_number(ps, &max)) max = JR_INF;
            }
            if (java_regex_peek(ps) != '}') { java_regex_fail_at(ps, "Unclosed counted closure", ps->pos); java_regex_JrFrag none = {0}; return none; }
            ps->pos++;
            if (max < min) { java_regex_fail_at(ps, "Illegal repetition range", start); java_regex_JrFrag none = {0}; return none; }
        } else return atom;
        int mode = JR_GREEDY;
        if (java_regex_peek(ps) == '?') { mode = JR_LAZY; ps->pos++; }
        else if (java_regex_peek(ps) == '+') { mode = JR_POSSESSIVE; ps->pos++; }
        atom = java_regex_apply_quantifier(ps, atom, min, max, mode);
        if (!java_regex_frag_ok(atom)) return atom;
    }
}

static java_regex_JrFrag java_regex_parse_atom(java_regex_JrParser *ps) {
    java_regex_JrFrag none = {0};
    const size_t start = ps->pos;
    const uint32_t c = java_regex_take_cp(ps);
    switch (c) {
        case '(': return java_regex_parse_group(ps);
        case '[': {
            java_regex_JrSet s = {0};
            if (!java_regex_parse_class_body(ps, &s)) return none;
            return java_regex_set_atom(ps, &s);
        }
        case '.': return java_regex_frag_single(java_regex_node_new(ps, J_ANY));
        case '^': return java_regex_frag_single(java_regex_node_new(ps, J_BOL));
        case '$': return java_regex_frag_single(java_regex_node_new(ps, J_EOL));
        case '\\': return java_regex_parse_escape(ps);
        case '*': case '+': case '?':
            java_regex_fail_at(ps, "Dangling meta character", start);
            return none;
        case '{':
            java_regex_fail_at(ps, "Illegal repetition", start);
            return none;
        default:
            return java_regex_char_atom(ps, c);
    }
}

static java_regex_JrFrag java_regex_parse_sequence(java_regex_JrParser *ps) {
    java_regex_JrFrag seq = {0};
    for (;;) {
        java_regex_skip_comments(ps);
        if (java_regex_at_end(ps) || ps->failed) break;
        const uint32_t c = java_regex_peek(ps);
        if (c == '|' || c == ')') break;
        java_regex_JrFrag atom = java_regex_parse_atom(ps);
        if (ps->failed || !java_regex_frag_ok(atom)) { java_regex_JrFrag none = {0}; return none; }
        atom = java_regex_parse_quantified(ps, atom);
        if (ps->failed || !java_regex_frag_ok(atom)) { java_regex_JrFrag none = {0}; return none; }
        if (!seq.head) seq = atom;
        else { seq.tail->next = atom.head; seq.tail = atom.tail; }
    }
    if (!seq.head) seq = java_regex_frag_single(java_regex_node_new(ps, J_NOP));
    return seq;
}

static java_regex_JrFrag java_regex_parse_alternation(java_regex_JrParser *ps) {
    java_regex_JrFrag none = {0};
    if (++ps->depth > 400u) { java_regex_fail_at(ps, "Pattern too complex", ps->pos); return none; }
    java_regex_JrFrag first = java_regex_parse_sequence(ps);
    if (ps->failed || !java_regex_frag_ok(first)) { ps->depth--; return none; }
    if (java_regex_peek(ps) != '|' || java_regex_at_end(ps)) { ps->depth--; return first; }
    java_regex_JrNode *alt = java_regex_node_new(ps, J_ALT), *join = java_regex_node_new(ps, J_NOP);
    if (!alt || !join) return none;
    java_regex_JrNode **branches = NULL;
    size_t count = 0;
    java_regex_JrFrag branch = first;
    for (;;) {
        java_regex_JrNode **grown = realloc(branches, (count + 1) * sizeof(*grown));
        if (!grown) { free(branches); java_regex_fail_at(ps, "out of memory", ps->pos); return none; }
        branches = grown;
        branches[count++] = branch.head;
        branch.tail->next = join;
        if (java_regex_at_end(ps) || java_regex_peek(ps) != '|') break;
        ps->pos++;
        branch = java_regex_parse_sequence(ps);
        if (ps->failed || !java_regex_frag_ok(branch)) { free(branches); return none; }
    }
    alt->alts = branches;
    alt->nalts = count;
    ps->depth--;
    java_regex_JrFrag f = {alt, join};
    return f;
}

void ssrnx_java_regex_free(SsrnxJavaRegex *re) {
    if (!re) return;
    for (java_regex_JrNode *n = re->nodes; n;) {
        java_regex_JrNode *next = n->arena_next;
        free(n->str); free(n->set.r); free(n->alts); free(n);
        n = next;
    }
    for (size_t i = 0; i < re->name_count; ++i) free(re->names[i]);
    free(re->names); free(re->name_groups); free(re);
}

SsrnxJavaRegex *ssrnx_java_regex_compile(const uint16_t *pattern, size_t length, int flags,
                                         char *error, size_t error_cap, int32_t *error_index) {
    char local_error[160];
    if (!error || !error_cap) { error = local_error; error_cap = sizeof(local_error); }
    error[0] = '\0';
    if (error_index) *error_index = -1;
    SsrnxJavaRegex *re = calloc(1, sizeof(*re));
    if (!re) { snprintf(error, error_cap, "out of memory"); return NULL; }
    re->flags = flags;
    java_regex_JrParser ps = {re, pattern, length, 0, flags, error, error_cap, -1, false, 0};
    java_regex_JrFrag body;
    if (flags & SSRNX_JREGEX_LITERAL) {
        java_regex_JrNode *n = java_regex_node_new(&ps, J_STRING);
        if (n) {
            n->str_len = length;
            n->str = malloc((length ? length : 1u) * sizeof(uint16_t));
            if (n->str) memcpy(n->str, pattern, length * sizeof(uint16_t));
            else java_regex_fail_at(&ps, "out of memory", 0);
        }
        body = java_regex_frag_single(n);
    } else {
        body = java_regex_parse_alternation(&ps);
        if (!ps.failed && !java_regex_at_end(&ps)) java_regex_fail_at(&ps, java_regex_peek(&ps) == ')' ? "Unmatched closing ')'" : "Unexpected character", ps.pos);
    }
    java_regex_JrNode *accept = ps.failed ? NULL : java_regex_node_new(&ps, J_ACCEPT);
    if (ps.failed || !java_regex_frag_ok(body) || !accept) {
        if (error_index) *error_index = ps.error_index;
        ssrnx_java_regex_free(re);
        return NULL;
    }
    body.tail->next = accept;
    re->start = body.head;
    return re;
}

size_t ssrnx_java_regex_group_count(const SsrnxJavaRegex *re) { return re ? re->groups : 0; }

int32_t ssrnx_java_regex_group_index(const SsrnxJavaRegex *re, const char *name) {
    return re && name ? java_regex_group_by_name(re, name) : -1;
}

/* ------------------------------------------------------------ matching */

typedef struct java_regex_JrState {
    const SsrnxJavaRegex *re;
    const uint16_t *s;
    size_t rstart, rend, last_end;
    size_t input_length, anchor_start, anchor_end;
    bool anchoring, transparent;
    SsrnxJavaRegexMode mode;
    int32_t *groups;
    int32_t *open;
    int32_t *loop_count;
    int32_t *loop_pos;
    size_t depth;
    uintptr_t stack_limit;
    bool overflow;
    size_t sub_end;
    size_t behind_target;
    bool behind_active;
} java_regex_JrState;

static bool java_regex_is_line_terminator(uint32_t c, int flags) {
    if (flags & SSRNX_JREGEX_UNIX_LINES) return c == '\n';
    return c == '\n' || c == '\r' || c == 0x85 || c == 0x2028 || c == 0x2029;
}

static uint32_t java_regex_cp_at(const java_regex_JrState *st, size_t pos, size_t *width) {
    uint32_t c = st->s[pos];
    *width = 1;
    if (c >= 0xD800 && c <= 0xDBFF && pos + 1 < st->rend && st->s[pos + 1] >= 0xDC00 && st->s[pos + 1] <= 0xDFFF) {
        c = 0x10000u + ((c - 0xD800u) << 10) + (st->s[pos + 1] - 0xDC00u);
        *width = 2;
    }
    return c;
}

static uint32_t java_regex_cp_before(const java_regex_JrState *st, size_t pos, size_t *width) {
    uint32_t c = st->s[pos - 1];
    *width = 1;
    if (c >= 0xDC00 && c <= 0xDFFF && pos >= st->rstart + 2 && st->s[pos - 2] >= 0xD800 && st->s[pos - 2] <= 0xDBFF) {
        c = 0x10000u + ((st->s[pos - 2] - 0xD800u) << 10) + (c - 0xDC00u);
        *width = 2;
    }
    return c;
}

static bool java_regex_is_word_cp(uint32_t c) {
    return c == '_' || (c < 128 ? ((c | 32u) >= 'a' && (c | 32u) <= 'z') || (c >= '0' && c <= '9')
                                : java_regex_cp_is_letter(c) || java_regex_cp_is_digit(c));
}

static bool java_regex_word_boundary(const java_regex_JrState *st, size_t pos) {
    java_regex_JrState visible = *st;
    if (st->transparent) { visible.rstart = 0; visible.rend = st->input_length; }
    st = &visible;
    size_t w;
    const bool before = pos > st->rstart && java_regex_is_word_cp(java_regex_cp_before(st, pos, &w));
    const bool after = pos < st->rend && java_regex_is_word_cp(java_regex_cp_at(st, pos, &w));
    return before != after;
}

/* One code-point atom (CHAR/ANY/SET) at pos; width out. */
static bool java_regex_atom_at(const java_regex_JrState *st, const java_regex_JrNode *n, size_t pos, size_t *width) {
    if (pos >= st->rend) return false;
    const uint32_t c = java_regex_cp_at(st, pos, width);
    const bool icase = (n->flags & SSRNX_JREGEX_CASE_INSENSITIVE) != 0;
    const bool unicode = (n->flags & SSRNX_JREGEX_UNICODE_CASE) != 0;
    switch (n->kind) {
        case J_CHAR: return java_regex_cp_equal(c, n->c, icase, unicode);
        case J_ANY: return (n->flags & SSRNX_JREGEX_DOTALL) || !java_regex_is_line_terminator(c, n->flags);
        case J_SET: return java_regex_set_has(&n->set, c);
        default: return false;
    }
}

static bool java_regex_match_node(java_regex_JrState *st, const java_regex_JrNode *n, size_t pos);
static bool java_regex_match_sub(java_regex_JrState *st, const java_regex_JrNode *n, size_t pos);

static bool java_regex_loop_iterate(java_regex_JrState *st, const java_regex_JrNode *loop, size_t pos);

/* Lookaround and atomic groups: run the body as a sub-match ending in
 * SUB_ACCEPT, then continue (or not) without backtracking into it. Kept out of
 * match_node so the recursive frame stays small. */
static __attribute__((noinline)) bool java_regex_match_sub(java_regex_JrState *st, const java_regex_JrNode *n, size_t pos) {
    const size_t ngroups = 2u * (st->re->groups + 1u);
    int32_t stack_saved[32];
    int32_t *saved = ngroups <= 32u ? stack_saved : malloc(ngroups * sizeof(*saved));
    if (!saved) { st->overflow = true; return false; }
    memcpy(saved, st->groups, ngroups * sizeof(*saved));
    const size_t saved_target = st->behind_target;
    const bool saved_behind = st->behind_active;
    const size_t saved_start = st->rstart, saved_end = st->rend;
    const size_t saved_anchor_start = st->anchor_start, saved_anchor_end = st->anchor_end;
    if (n->kind == J_LOOK && st->transparent) {
        if (n->behind) {
            st->rstart = 0;
            if (st->anchoring) st->anchor_start = 0;
        } else {
            st->rend = st->input_length;
            if (st->anchoring) st->anchor_end = st->input_length;
        }
    }
    bool hit = false, r = false;
    size_t end = pos;
    if (n->kind == J_LOOK && n->behind) {
        st->behind_active = true;
        st->behind_target = pos;
        for (size_t start = pos + 1; start-- > st->rstart && !hit && !st->overflow;) hit = java_regex_match_node(st, n->body, start);
    } else {
        st->behind_active = false;
        hit = java_regex_match_node(st, n->body, pos);
        end = st->sub_end;
    }
    st->behind_target = saved_target;
    st->behind_active = saved_behind;
    st->rstart = saved_start;
    st->rend = saved_end;
    st->anchor_start = saved_anchor_start;
    st->anchor_end = saved_anchor_end;
    if (n->kind == J_LOOK && n->negative) {
        memcpy(st->groups, saved, ngroups * sizeof(*saved));
        r = !hit && !st->overflow && java_regex_match_node(st, n->next, pos);
    } else if (hit) {
        r = java_regex_match_node(st, n->next, n->kind == J_ATOMIC ? end : pos);
        if (!r) memcpy(st->groups, saved, ngroups * sizeof(*saved));
    } else memcpy(st->groups, saved, ngroups * sizeof(*saved));
    if (saved != stack_saved) free(saved);
    return r;
}

static bool java_regex_match_node(java_regex_JrState *st, const java_regex_JrNode *n, size_t pos) {
    if (++st->depth > JR_MAX_DEPTH || (uintptr_t)__builtin_frame_address(0) < st->stack_limit) {
        st->overflow = true;
        st->depth--;
        return false;
    }
    bool r = false;
    size_t w;
    for (;;) {
        switch (n->kind) {
            case J_ACCEPT:
                r = st->mode != SSRNX_JREGEX_MATCHES || pos == st->rend;
                if (r) st->groups[1] = (int32_t)pos;
                goto done;
            case J_SUB_ACCEPT:
                r = !st->behind_active || pos == st->behind_target;
                if (r) st->sub_end = pos;
                goto done;
            case J_NOP:
                n = n->next;
                continue;
            case J_CHAR: case J_ANY: case J_SET:
                if (!java_regex_atom_at(st, n, pos, &w)) goto done;
                pos += w; n = n->next;
                continue;
            case J_STRING: {
                if (st->rend - pos < n->str_len) goto done;
                const bool icase = (n->flags & SSRNX_JREGEX_CASE_INSENSITIVE) != 0;
                const bool unicode = (n->flags & SSRNX_JREGEX_UNICODE_CASE) != 0;
                for (size_t i = 0; i < n->str_len; ++i)
                    if (!java_regex_cp_equal(st->s[pos + i], n->str[i], icase, unicode)) goto done;
                pos += n->str_len; n = n->next;
                continue;
            }
            case J_BOL:
                if (n->flags & SSRNX_JREGEX_MULTILINE) {
                    if (pos < st->anchor_start || pos >= st->anchor_end) goto done;
                    if (pos != st->anchor_start) {
                        const uint32_t prev = st->s[pos - 1];
                        if (!java_regex_is_line_terminator(prev, n->flags)) goto done;
                        /* not between \r and \n */
                        if (prev == '\r' && st->s[pos] == '\n' && !(n->flags & SSRNX_JREGEX_UNIX_LINES)) goto done;
                    }
                } else if (pos != st->anchor_start) goto done;
                n = n->next;
                continue;
            case J_EOL: {
                bool at = pos == st->anchor_end;
                if (!at && pos < st->anchor_end && java_regex_is_line_terminator(st->s[pos], n->flags)) {
                    size_t term_end = pos + 1;
                    if (st->s[pos] == '\r' && pos + 1 < st->anchor_end && st->s[pos + 1] == '\n' &&
                        !(n->flags & SSRNX_JREGEX_UNIX_LINES)) term_end++;
                    const bool between_crlf = pos > st->anchor_start && st->s[pos] == '\n' && st->s[pos - 1] == '\r' &&
                                              !(n->flags & SSRNX_JREGEX_UNIX_LINES);
                    if (!between_crlf) at = (n->flags & SSRNX_JREGEX_MULTILINE) || term_end == st->anchor_end;
                }
                if (!at) goto done;
                n = n->next;
                continue;
            }
            case J_WORDB: if (!java_regex_word_boundary(st, pos)) goto done; n = n->next; continue;
            case J_NWORDB: if (java_regex_word_boundary(st, pos)) goto done; n = n->next; continue;
            case J_BEGIN: if (pos != st->anchor_start) goto done; n = n->next; continue;
            case J_END: if (pos != st->anchor_end) goto done; n = n->next; continue;
            case J_ENDZ: {
                bool at = pos == st->anchor_end;
                if (!at && pos < st->anchor_end && java_regex_is_line_terminator(st->s[pos], n->flags)) {
                    size_t end = pos + 1;
                    if (st->s[pos] == '\r' && end < st->anchor_end && st->s[end] == '\n') end++;
                    at = end == st->anchor_end && !(pos > st->anchor_start && st->s[pos] == '\n' &&
                                                   st->s[pos - 1] == '\r' && !(n->flags & SSRNX_JREGEX_UNIX_LINES));
                }
                if (!at) goto done;
                n = n->next;
                continue;
            }
            case J_LASTEND: if (pos != st->last_end) goto done; n = n->next; continue;
            case J_LINEBREAK:
                if (pos + 1 < st->rend && st->s[pos] == '\r' && st->s[pos + 1] == '\n') { pos += 2; n = n->next; continue; }
                if (pos < st->rend && (st->s[pos] == '\n' || st->s[pos] == 0x0B || st->s[pos] == '\f' || st->s[pos] == '\r' ||
                                       st->s[pos] == 0x85 || st->s[pos] == 0x2028 || st->s[pos] == 0x2029)) {
                    pos++; n = n->next; continue;
                }
                goto done;
            case J_GROUP_OPEN: {
                const int32_t saved = st->open[n->group];
                st->open[n->group] = (int32_t)pos;
                r = java_regex_match_node(st, n->next, pos);
                if (!r) st->open[n->group] = saved;
                goto done;
            }
            case J_GROUP_CLOSE: {
                const int32_t s0 = st->groups[2 * n->group], s1 = st->groups[2 * n->group + 1];
                st->groups[2 * n->group] = st->open[n->group];
                st->groups[2 * n->group + 1] = (int32_t)pos;
                r = java_regex_match_node(st, n->next, pos);
                if (!r) { st->groups[2 * n->group] = s0; st->groups[2 * n->group + 1] = s1; }
                goto done;
            }
            case J_BACKREF: {
                /* A reference to a group the pattern does not define never matches. */
                if ((size_t)n->group > st->re->groups) goto done;
                const int32_t gs = st->groups[2 * n->group], ge = st->groups[2 * n->group + 1];
                if (gs < 0 || ge < 0) goto done;
                const size_t glen = (size_t)(ge - gs);
                if (st->rend - pos < glen) goto done;
                const bool icase = (n->flags & SSRNX_JREGEX_CASE_INSENSITIVE) != 0;
                const bool unicode = (n->flags & SSRNX_JREGEX_UNICODE_CASE) != 0;
                for (size_t i = 0; i < glen; ++i)
                    if (!java_regex_cp_equal(st->s[pos + i], st->s[(size_t)gs + i], icase, unicode)) goto done;
                pos += glen; n = n->next;
                continue;
            }
            case J_ALT:
                for (size_t i = 0; i < n->nalts && !r; ++i) r = java_regex_match_node(st, n->alts[i], pos);
                goto done;
            case J_REPEAT: {
                const java_regex_JrNode *atom = n->body;
                size_t p = pos;
                uint32_t count = 0;
                if (n->mode == JR_LAZY) {
                    while (count < n->min) {
                        if (!java_regex_atom_at(st, atom, p, &w)) goto done;
                        p += w; count++;
                    }
                    for (;;) {
                        if (java_regex_match_node(st, n->next, p)) { r = true; goto done; }
                        if (st->overflow || count >= n->max || !java_regex_atom_at(st, atom, p, &w)) goto done;
                        p += w; count++;
                    }
                }
                while (count < n->max && java_regex_atom_at(st, atom, p, &w)) { p += w; count++; }
                if (count < n->min) goto done;
                if (n->mode == JR_POSSESSIVE) { pos = p; n = n->next; continue; }
                for (;;) {
                    if (java_regex_match_node(st, n->next, p)) { r = true; goto done; }
                    if (st->overflow || count == n->min) goto done;
                    java_regex_cp_before(st, p, &w);
                    p -= w; count--;
                }
            }
            case J_LOOP: {
                const int id = n->loop_id;
                const int32_t saved_count = st->loop_count[id], saved_pos = st->loop_pos[id];
                st->loop_count[id] = 0;
                st->loop_pos[id] = -1;
                r = java_regex_loop_iterate(st, n, pos);
                st->loop_count[id] = saved_count;
                st->loop_pos[id] = saved_pos;
                goto done;
            }
            case J_LOOP_TAIL:
                r = java_regex_loop_iterate(st, n->loop, pos);
                goto done;
            case J_LOOK: case J_ATOMIC:
                r = java_regex_match_sub(st, n, pos);
                goto done;
        }
    }
done:
    st->depth--;
    return r;
}

/* Loop control after (or before) each iteration of a general quantifier. An
 * iteration that consumed nothing ends the loop once min is met, as Java's
 * Loop does, so X* over an empty-matching X terminates. */
static bool java_regex_loop_iterate(java_regex_JrState *st, const java_regex_JrNode *loop, size_t pos) {
    const int id = loop->loop_id;
    const int32_t count = st->loop_count[id], last = st->loop_pos[id];
    if ((uint32_t)count < loop->min) {
        st->loop_count[id] = count + 1;
        st->loop_pos[id] = (int32_t)pos;
        if (java_regex_match_node(st, loop->body, pos)) return true;
        st->loop_count[id] = count;
        st->loop_pos[id] = last;
        return false;
    }
    const bool may_repeat = (uint32_t)count < loop->max && !(count > 0 && last == (int32_t)pos);
    if (loop->mode == JR_LAZY) {
        if (java_regex_match_node(st, loop->next, pos)) return true;
        if (!may_repeat || st->overflow) return false;
        st->loop_count[id] = count + 1;
        st->loop_pos[id] = (int32_t)pos;
        if (java_regex_match_node(st, loop->body, pos)) return true;
        st->loop_count[id] = count;
        st->loop_pos[id] = last;
        return false;
    }
    if (may_repeat) {
        st->loop_count[id] = count + 1;
        st->loop_pos[id] = (int32_t)pos;
        if (java_regex_match_node(st, loop->body, pos)) return true;
        st->loop_count[id] = count;
        st->loop_pos[id] = last;
        if (st->overflow) return false;
    }
    return java_regex_match_node(st, loop->next, pos);
}

int ssrnx_java_regex_match(const SsrnxJavaRegex *re, const uint16_t *input, size_t region_start,
                           size_t region_end, size_t from, size_t last_match_end,
                           SsrnxJavaRegexMode mode, int32_t *groups) {
    return ssrnx_java_regex_match_bounds(re, input, region_end, region_start, region_end, from,
                                         last_match_end, true, false, mode, groups);
}

int ssrnx_java_regex_match_bounds(const SsrnxJavaRegex *re, const uint16_t *input, size_t input_length,
                                  size_t region_start, size_t region_end, size_t from, size_t last_match_end,
                                  bool anchoring, bool transparent, SsrnxJavaRegexMode mode, int32_t *groups) {
    if (!re || !groups || region_start > region_end || region_end > input_length ||
        from < region_start || from > region_end) return 0;
    const size_t ngroups = 2u * (re->groups + 1u);
    int32_t *open = malloc((re->groups + 1u) * sizeof(*open));
    int32_t *loops = calloc(re->loops ? 2u * re->loops : 1u, sizeof(*loops));
    if (!open || !loops) { free(open); free(loops); return -1; }
    java_regex_JrState st = {0};
    st.re = re;
    st.s = input;
    st.rstart = region_start;
    st.rend = region_end;
    st.input_length = input_length;
    st.anchor_start = anchoring ? region_start : 0;
    st.anchor_end = anchoring ? region_end : input_length;
    st.anchoring = anchoring;
    st.transparent = transparent;
    st.last_end = last_match_end;
    st.mode = mode;
    st.groups = groups;
    st.open = open;
    st.loop_count = loops;
    st.loop_pos = loops + re->loops;
    const uintptr_t here = (uintptr_t)__builtin_frame_address(0);
    uintptr_t low = 0, high = 0;
    if (ssrnx_bionic_current_stack_bounds && ssrnx_bionic_current_stack_bounds(&low, &high) &&
        here > low && here <= high)
        st.stack_limit = here - low > JR_STACK_RESERVE ? low + JR_STACK_RESERVE : here;
    else
        st.stack_limit = here > JR_STACK_FALLBACK ? here - JR_STACK_FALLBACK : 0;
    int result = 0;
    for (size_t start = from; start <= region_end; ++start) {
        for (size_t i = 0; i < ngroups; ++i) groups[i] = -1;
        for (size_t i = 0; i <= re->groups; ++i) open[i] = -1;
        groups[0] = (int32_t)start;
        if (java_regex_match_node(&st, re->start, start)) { result = 1; break; }
        if (st.overflow) { result = -1; break; }
        if (mode != SSRNX_JREGEX_FIND) break;
        /* Never start inside a surrogate pair. */
        if (start < region_end && input[start] >= 0xD800 && input[start] <= 0xDBFF && start + 1 < region_end &&
            input[start + 1] >= 0xDC00 && input[start + 1] <= 0xDFFF) start++;
    }
    if (result != 1) for (size_t i = 0; i < ngroups; ++i) groups[i] = -1;
    free(open);
    free(loops);
    return result;
}

#pragma pop_macro("JR_STACK_RESERVE")
#pragma pop_macro("JR_STACK_FALLBACK")
#pragma pop_macro("JR_MAX_GROUPS")
#pragma pop_macro("JR_MAX_DEPTH")
#pragma pop_macro("JR_INF")


/* ---- java_regex_framework ---- */
#pragma push_macro("MATCHER_CLASS")
#pragma push_macro("MATCHER_MAGIC")
#pragma push_macro("PATTERN_CACHE_SIZE")
#pragma push_macro("PATTERN_CLASS")
#pragma push_macro("PATTERN_MAGIC")
#pragma push_macro("PSE_CLASS")
/* java.util.regex.Pattern / Matcher and the regex methods of java.lang.String
 * over the Java regex engine (java_regex.c).
 *
 * A Pattern owns an immutable compiled program (thread-safe, as in Java); a
 * Matcher owns a UTF-16 snapshot of its input, its region, the last match's
 * groups and the append position, following the Matcher state machine
 * (first/last/from/to). String.matches/replaceAll/replaceFirst/split reuse a
 * small cache of compiled patterns, since apps call them in loops. */


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define PATTERN_CLASS "java/util/regex/Pattern"
#define MATCHER_CLASS "java/util/regex/Matcher"
#define PSE_CLASS "java/util/regex/PatternSyntaxException"
#define PATTERN_MAGIC 0x4A525850u /* JRXP */
#define MATCHER_MAGIC 0x4A52584Du /* JRXM */
#define PATTERN_CACHE_SIZE 16u

typedef struct java_regex_framework_PatternPayload {
    uint32_t magic;
    SsrnxJavaRegex *re;
    uint16_t *source;
    int32_t source_len;
    int flags;
    struct java_regex_framework_PatternPayload *next;
} java_regex_framework_PatternPayload;

typedef struct java_regex_framework_MatcherPayload {
    uint32_t magic;
    java_regex_framework_PatternPayload *pattern;
    uint16_t *text;
    int32_t len;
    int32_t from, to;         /* region */
    int32_t first, last;      /* last match bounds; first == -1 when none */
    int32_t append_pos;
    int32_t *groups;
    size_t group_slots;
    bool non_anchoring, transparent; /* zero-initialized defaults: anchoring, opaque */
    struct java_regex_framework_MatcherPayload *next;
} java_regex_framework_MatcherPayload;

static pthread_mutex_t java_regex_framework_g_regex_lock = PTHREAD_MUTEX_INITIALIZER;
static java_regex_framework_PatternPayload *java_regex_framework_g_patterns;
static java_regex_framework_MatcherPayload *java_regex_framework_g_matchers;
/* String.* regex cache: patterns compiled with flags 0, most recent first. */
static java_regex_framework_PatternPayload *java_regex_framework_g_cache[PATTERN_CACHE_SIZE];

static SsrnxJniValue java_regex_framework_v_obj(void *ref) { SsrnxJniValue v = {0}; v.l = ref; return v; }
static SsrnxJniValue java_regex_framework_v_i(int32_t i) { SsrnxJniValue v = {0}; v.i = i; return v; }
static SsrnxJniValue java_regex_framework_v_z(bool z) { SsrnxJniValue v = {0}; v.z = z ? 1 : 0; return v; }

static void java_regex_framework_throw_msg(void *env, const char *cls, const char *message) {
    (void)ssrnx_jni_throw_new_by_name(env, cls, message);
}

/* ------------------------------------------------------------ UTF-16 buffers */

typedef struct java_regex_framework_U16Buf { uint16_t *p; size_t n, cap; bool oom; } java_regex_framework_U16Buf;

static void java_regex_framework_buf_append(java_regex_framework_U16Buf *b, const uint16_t *s, size_t n) {
    if (b->oom || !n) return;
    if (b->n + n > b->cap) {
        size_t next = b->cap ? b->cap * 2u : 64u;
        while (next < b->n + n) next *= 2u;
        uint16_t *grown = realloc(b->p, next * sizeof(*grown));
        if (!grown) { b->oom = true; return; }
        b->p = grown; b->cap = next;
    }
    memcpy(b->p + b->n, s, n * sizeof(*s));
    b->n += n;
}

static void *java_regex_framework_new_string16(void *env, const uint16_t *s, size_t n) {
    return ssrnx_jni_new_local_string_utf16(env, s, (int32_t)n);
}

/* CharSequence argument as UTF-16: a String directly, anything else through
 * its (virtual) toString(). NULL with NullPointerException for null. */
static uint16_t *java_regex_framework_charseq_utf16(void *env, void *ref, int32_t *len) {
    if (!ref) { java_regex_framework_throw_msg(env, "java/lang/NullPointerException", "input == null"); return NULL; }
    uint16_t *chars = ssrnx_jni_string_utf16_dup_host(ref, len);
    if (chars) return chars;
    SsrnxJniValue out = {0};
    if (!ssrnx_android_call_virtual(env, ref, "toString", "()Ljava/lang/String;", NULL, 0, &out) || !out.l) {
        if (!ssrnx_jni_exception_pending_host(env))
            java_regex_framework_throw_msg(env, "java/lang/NullPointerException", "CharSequence.toString() returned null");
        return NULL;
    }
    chars = ssrnx_jni_string_utf16_dup_host(out.l, len);
    ssrnx_jni_delete_local_ref_host(env, out.l);
    return chars;
}

/* ------------------------------------------------------------ Pattern */

static java_regex_framework_PatternPayload *java_regex_framework_pattern_payload(void *token) {
    java_regex_framework_PatternPayload *p = token ? ssrnx_jni_get_token_host_data(token) : NULL;
    return p && p->magic == PATTERN_MAGIC ? p : NULL;
}

static java_regex_framework_PatternPayload *java_regex_framework_pattern_ref_payload(void *ref) {
    java_regex_framework_PatternPayload *p = ref ? ssrnx_jni_get_object_host_data(ref) : NULL;
    return p && p->magic == PATTERN_MAGIC ? p : NULL;
}

/* PatternSyntaxException.getMessage(): description, " near index N", the
 * pattern and a caret under the index. */
static void java_regex_framework_throw_syntax(void *env, const char *description, const uint16_t *source, int32_t len, int32_t index) {
    char pattern[512];
    size_t n = 0;
    for (int32_t i = 0; i < len && n + 4 < sizeof(pattern); ++i) {
        const uint16_t c = source[i];
        if (c < 0x80) pattern[n++] = (char)c;
        else if (c < 0x800) { pattern[n++] = (char)(0xC0 | (c >> 6)); pattern[n++] = (char)(0x80 | (c & 63)); }
        else { pattern[n++] = (char)(0xE0 | (c >> 12)); pattern[n++] = (char)(0x80 | ((c >> 6) & 63)); pattern[n++] = (char)(0x80 | (c & 63)); }
    }
    pattern[n] = '\0';
    char message[1400];
    int pos = index >= 0 ? snprintf(message, sizeof(message), "%s near index %d\n%s\n", description, (int)index, pattern)
                         : snprintf(message, sizeof(message), "%s\n%s", description, pattern);
    for (int32_t i = 0; index >= 0 && i < index && pos > 0 && (size_t)pos + 2 < sizeof(message); ++i) message[pos++] = ' ';
    if (index >= 0 && pos > 0 && (size_t)pos + 2 < sizeof(message)) { message[pos++] = '^'; message[pos] = '\0'; }
    java_regex_framework_throw_msg(env, PSE_CLASS, message);
}

static java_regex_framework_PatternPayload *java_regex_framework_pattern_compile(void *env, const uint16_t *source, int32_t len, int flags) {
    char error[160];
    int32_t index = -1;
    SsrnxJavaRegex *re = ssrnx_java_regex_compile(source, (size_t)len, flags, error, sizeof(error), &index);
    if (!re) { java_regex_framework_throw_syntax(env, error, source, len, index); return NULL; }
    java_regex_framework_PatternPayload *p = calloc(1, sizeof(*p));
    uint16_t *copy = malloc((size_t)(len > 0 ? len : 1) * sizeof(*copy));
    if (!p || !copy) {
        free(p); free(copy); ssrnx_java_regex_free(re);
        java_regex_framework_throw_msg(env, "java/lang/OutOfMemoryError", "Pattern.compile");
        return NULL;
    }
    if (len > 0) memcpy(copy, source, (size_t)len * sizeof(*copy));
    p->magic = PATTERN_MAGIC;
    p->re = re;
    p->source = copy;
    p->source_len = len;
    p->flags = flags;
    pthread_mutex_lock(&java_regex_framework_g_regex_lock);
    p->next = java_regex_framework_g_patterns;
    java_regex_framework_g_patterns = p;
    pthread_mutex_unlock(&java_regex_framework_g_regex_lock);
    return p;
}

/* Compiled program for a String.* regex argument (flags 0), cached. */
static java_regex_framework_PatternPayload *java_regex_framework_pattern_cached(void *env, void *regex_ref) {
    int32_t len = 0;
    uint16_t *source = java_regex_framework_charseq_utf16(env, regex_ref, &len);
    if (!source) return NULL;
    pthread_mutex_lock(&java_regex_framework_g_regex_lock);
    for (size_t i = 0; i < PATTERN_CACHE_SIZE; ++i) {
        java_regex_framework_PatternPayload *p = java_regex_framework_g_cache[i];
        if (p && p->source_len == len && memcmp(p->source, source, (size_t)len * sizeof(*source)) == 0) {
            memmove(&java_regex_framework_g_cache[1], &java_regex_framework_g_cache[0], i * sizeof(java_regex_framework_g_cache[0]));
            java_regex_framework_g_cache[0] = p;
            pthread_mutex_unlock(&java_regex_framework_g_regex_lock);
            free(source);
            return p;
        }
    }
    pthread_mutex_unlock(&java_regex_framework_g_regex_lock);
    java_regex_framework_PatternPayload *p = java_regex_framework_pattern_compile(env, source, len, 0);
    free(source);
    if (!p) return NULL;
    pthread_mutex_lock(&java_regex_framework_g_regex_lock);
    memmove(&java_regex_framework_g_cache[1], &java_regex_framework_g_cache[0], (PATTERN_CACHE_SIZE - 1u) * sizeof(java_regex_framework_g_cache[0]));
    java_regex_framework_g_cache[0] = p; /* evicted entries stay on g_patterns until reset */
    pthread_mutex_unlock(&java_regex_framework_g_regex_lock);
    return p;
}

static void *java_regex_framework_pattern_new_object(void *env, java_regex_framework_PatternPayload *p) {
    void *obj = ssrnx_jni_new_local_object(env, PATTERN_CLASS);
    if (obj && ssrnx_jni_set_object_host_data(obj, p) != SSRNX_OK) {
        ssrnx_jni_delete_local_ref_host(env, obj);
        obj = NULL;
    }
    return obj;
}

/* Pattern.compile(String[, int]) */
static SsrnxJniValue java_regex_framework_pattern_compile_java(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)token; (void)u;
    if (n < 1 || !a[0].l) { java_regex_framework_throw_msg(env, "java/lang/NullPointerException", "regex == null"); return java_regex_framework_v_obj(NULL); }
    const int flags = n == 2 ? a[1].i : 0;
    if (flags & ~0x1FF) { java_regex_framework_throw_msg(env, "java/lang/IllegalArgumentException", "Unknown flag"); return java_regex_framework_v_obj(NULL); }
    int32_t len = 0;
    uint16_t *source = ssrnx_jni_string_utf16_dup_host(a[0].l, &len);
    if (!source) return java_regex_framework_v_obj(NULL);
    java_regex_framework_PatternPayload *p = java_regex_framework_pattern_compile(env, source, len, flags);
    free(source);
    return java_regex_framework_v_obj(p ? java_regex_framework_pattern_new_object(env, p) : NULL);
}

static SsrnxJniValue java_regex_framework_pattern_source(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)n; (void)u;
    java_regex_framework_PatternPayload *p = java_regex_framework_pattern_payload(token);
    return java_regex_framework_v_obj(p ? java_regex_framework_new_string16(env, p->source, (size_t)p->source_len) : NULL);
}

static SsrnxJniValue java_regex_framework_pattern_flags(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)env; (void)a; (void)n; (void)u;
    java_regex_framework_PatternPayload *p = java_regex_framework_pattern_payload(token);
    return java_regex_framework_v_i(p ? p->flags : 0);
}

/* Pattern.quote: \Q...\E, splitting any \E inside the text. */
static SsrnxJniValue java_regex_framework_pattern_quote(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)token; (void)u;
    int32_t len = 0;
    uint16_t *s = n == 1 && a[0].l ? ssrnx_jni_string_utf16_dup_host(a[0].l, &len) : NULL;
    if (!s) { java_regex_framework_throw_msg(env, "java/lang/NullPointerException", "s == null"); return java_regex_framework_v_obj(NULL); }
    static const uint16_t kQ[] = {'\\', 'Q'}, kE[] = {'\\', 'E'}, kEsc[] = {'\\', 'E', '\\', '\\', 'E', '\\', 'Q'};
    java_regex_framework_U16Buf b = {0};
    java_regex_framework_buf_append(&b, kQ, 2);
    int32_t start = 0;
    for (int32_t i = 0; i + 1 < len; ++i) {
        if (s[i] == '\\' && s[i + 1] == 'E') {
            java_regex_framework_buf_append(&b, s + start, (size_t)(i - start));
            java_regex_framework_buf_append(&b, kEsc, 7);
            start = i + 2;
            i++;
        }
    }
    java_regex_framework_buf_append(&b, s + start, (size_t)(len - start));
    java_regex_framework_buf_append(&b, kE, 2);
    free(s);
    void *out = b.oom ? NULL : java_regex_framework_new_string16(env, b.p, b.n);
    free(b.p);
    return java_regex_framework_v_obj(out);
}

/* ------------------------------------------------------------ Matcher core */

static java_regex_framework_MatcherPayload *java_regex_framework_matcher_payload(void *token) {
    java_regex_framework_MatcherPayload *m = token ? ssrnx_jni_get_token_host_data(token) : NULL;
    return m && m->magic == MATCHER_MAGIC ? m : NULL;
}

static bool java_regex_framework_matcher_set_pattern(java_regex_framework_MatcherPayload *m, java_regex_framework_PatternPayload *p) {
    const size_t slots = 2u * (ssrnx_java_regex_group_count(p->re) + 1u);
    int32_t *groups = malloc(slots * sizeof(*groups));
    if (!groups) return false;
    for (size_t i = 0; i < slots; ++i) groups[i] = -1;
    free(m->groups);
    m->groups = groups;
    m->group_slots = slots;
    m->pattern = p;
    return true;
}

static void java_regex_framework_matcher_reset(java_regex_framework_MatcherPayload *m) {
    m->first = -1;
    m->last = 0;
    m->append_pos = 0;
    m->from = 0;
    m->to = m->len;
    for (size_t i = 0; i < m->group_slots; ++i) m->groups[i] = -1;
}

static java_regex_framework_MatcherPayload *java_regex_framework_matcher_create(java_regex_framework_PatternPayload *p, uint16_t *text, int32_t len) {
    java_regex_framework_MatcherPayload *m = calloc(1, sizeof(*m));
    if (!m || !java_regex_framework_matcher_set_pattern(m, p)) { free(m); return NULL; }
    m->magic = MATCHER_MAGIC;
    m->text = text;
    m->len = len;
    java_regex_framework_matcher_reset(m);
    pthread_mutex_lock(&java_regex_framework_g_regex_lock);
    m->next = java_regex_framework_g_matchers;
    java_regex_framework_g_matchers = m;
    pthread_mutex_unlock(&java_regex_framework_g_regex_lock);
    return m;
}

/* One match attempt; updates first/last and groups. */
static bool java_regex_framework_matcher_run(void *env, java_regex_framework_MatcherPayload *m, int32_t from, SsrnxJavaRegexMode mode) {
    const size_t last_end = m->first >= 0 ? (size_t)m->last : (size_t)m->from;
    const int rc = ssrnx_java_regex_match_bounds(m->pattern->re, m->text, (size_t)m->len,
                                                 (size_t)m->from, (size_t)m->to, (size_t)from,
                                                 last_end, !m->non_anchoring, m->transparent, mode, m->groups);
    if (rc < 0) {
        java_regex_framework_throw_msg(env, "java/lang/StackOverflowError", "regular expression backtracking too deep");
        m->first = -1;
        return false;
    }
    if (rc == 0) { m->first = -1; return false; }
    m->first = m->groups[0];
    m->last = m->groups[1];
    return true;
}

/* Matcher.find(): continue after the previous match (one past an empty one). */
static bool java_regex_framework_matcher_find_next(void *env, java_regex_framework_MatcherPayload *m) {
    int32_t next = m->last;
    if (next == m->first) next++;
    if (next < m->from) next = m->from;
    if (next > m->to) {
        for (size_t i = 0; i < m->group_slots; ++i) m->groups[i] = -1;
        m->first = -1;
        return false;
    }
    return java_regex_framework_matcher_run(env, m, next, SSRNX_JREGEX_FIND);
}

static bool java_regex_framework_matcher_require_match(void *env, const java_regex_framework_MatcherPayload *m) {
    if (m->first >= 0) return true;
    java_regex_framework_throw_msg(env, "java/lang/IllegalStateException", "No match found");
    return false;
}

static bool java_regex_framework_matcher_group_valid(void *env, const java_regex_framework_MatcherPayload *m, int32_t group) {
    if (group >= 0 && (size_t)group <= ssrnx_java_regex_group_count(m->pattern->re)) return true;
    char message[48];
    snprintf(message, sizeof(message), "No group %d", (int)group);
    java_regex_framework_throw_msg(env, "java/lang/IndexOutOfBoundsException", message);
    return false;
}

static int32_t java_regex_framework_matcher_named_group(void *env, const java_regex_framework_MatcherPayload *m, void *name_ref) {
    char name[128] = {0};
    if (!name_ref || !ssrnx_jni_string_utf_copy(name_ref, name, sizeof(name))) {
        java_regex_framework_throw_msg(env, "java/lang/NullPointerException", "Null group name");
        return -1;
    }
    const int32_t group = ssrnx_java_regex_group_index(m->pattern->re, name);
    if (group < 0) {
        char message[180];
        snprintf(message, sizeof(message), "No group with name <%s>", name);
        java_regex_framework_throw_msg(env, "java/lang/IllegalArgumentException", message);
    }
    return group;
}

/* Replacement string expansion (Matcher.appendReplacement): \x is x, $n and
 * ${name} are group references, the longest $n that names a group wins. */
static bool java_regex_framework_expand_replacement(void *env, const java_regex_framework_MatcherPayload *m, const uint16_t *r, int32_t rlen, java_regex_framework_U16Buf *out) {
    const size_t groups = ssrnx_java_regex_group_count(m->pattern->re);
    for (int32_t i = 0; i < rlen; ++i) {
        uint16_t c = r[i];
        if (c == '\\') {
            if (++i >= rlen) { java_regex_framework_throw_msg(env, "java/lang/IllegalArgumentException", "character to be escaped is missing"); return false; }
            java_regex_framework_buf_append(out, &r[i], 1);
            continue;
        }
        if (c != '$') { java_regex_framework_buf_append(out, &c, 1); continue; }
        if (++i >= rlen) {
            java_regex_framework_throw_msg(env, "java/lang/IllegalArgumentException", "Illegal group reference: group index is missing");
            return false;
        }
        int32_t group;
        if (r[i] == '{') {
            char name[128];
            size_t n = 0;
            while (++i < rlen && r[i] != '}' && n + 1 < sizeof(name)) {
                const uint16_t ch = r[i];
                const bool valid = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9');
                if (!valid) { java_regex_framework_throw_msg(env, "java/lang/IllegalArgumentException", "named capturing group is missing trailing '}'"); return false; }
                name[n++] = (char)ch;
            }
            name[n] = '\0';
            if (i >= rlen || r[i] != '}') { java_regex_framework_throw_msg(env, "java/lang/IllegalArgumentException", "named capturing group is missing trailing '}'"); return false; }
            if (!n) { java_regex_framework_throw_msg(env, "java/lang/IllegalArgumentException", "named capturing group has 0 length name"); return false; }
            group = ssrnx_java_regex_group_index(m->pattern->re, name);
            if (group < 0) {
                char message[180];
                snprintf(message, sizeof(message), "No group with name {%s}", name);
                java_regex_framework_throw_msg(env, "java/lang/IllegalArgumentException", message);
                return false;
            }
        } else {
            if (r[i] < '0' || r[i] > '9') { java_regex_framework_throw_msg(env, "java/lang/IllegalArgumentException", "Illegal group reference"); return false; }
            group = r[i] - '0';
            while (i + 1 < rlen && r[i + 1] >= '0' && r[i + 1] <= '9') {
                const int32_t next = group * 10 + (r[i + 1] - '0');
                if ((size_t)next > groups) break;
                group = next;
                i++;
            }
            if ((size_t)group > groups) {
                char message[48];
                snprintf(message, sizeof(message), "No group %d", (int)group);
                java_regex_framework_throw_msg(env, "java/lang/IndexOutOfBoundsException", message);
                return false;
            }
        }
        const int32_t gs = m->groups[2 * group], ge = m->groups[2 * group + 1];
        if (gs >= 0 && ge >= gs) java_regex_framework_buf_append(out, m->text + gs, (size_t)(ge - gs));
    }
    return !out->oom;
}

/* Text before the current match (from the append position) plus the expanded
 * replacement; advances the append position. */
static bool java_regex_framework_append_replacement(void *env, java_regex_framework_MatcherPayload *m, const uint16_t *r, int32_t rlen, java_regex_framework_U16Buf *out) {
    if (!java_regex_framework_matcher_require_match(env, m)) return false;
    java_regex_framework_U16Buf expanded = {0};
    if (!java_regex_framework_expand_replacement(env, m, r, rlen, &expanded)) { free(expanded.p); return false; }
    java_regex_framework_buf_append(out, m->text + m->append_pos, (size_t)(m->first - m->append_pos));
    java_regex_framework_buf_append(out, expanded.p, expanded.n);
    free(expanded.p);
    m->append_pos = m->last;
    return !out->oom;
}

/* replaceAll / replaceFirst over the whole input. */
static void *java_regex_framework_matcher_replace(void *env, java_regex_framework_MatcherPayload *m, void *replacement_ref, bool all) {
    if (!replacement_ref) { java_regex_framework_throw_msg(env, "java/lang/NullPointerException", "replacement"); return NULL; }
    int32_t rlen = 0;
    uint16_t *r = ssrnx_jni_string_utf16_dup_host(replacement_ref, &rlen);
    if (!r) return NULL;
    java_regex_framework_matcher_reset(m);
    java_regex_framework_U16Buf out = {0};
    bool ok = true, found = java_regex_framework_matcher_find_next(env, m);
    while (found && ok) {
        ok = java_regex_framework_append_replacement(env, m, r, rlen, &out);
        found = ok && all && java_regex_framework_matcher_find_next(env, m);
    }
    free(r);
    if (!ok || ssrnx_jni_exception_pending_host(env)) { free(out.p); return NULL; }
    java_regex_framework_buf_append(&out, m->text + m->append_pos, (size_t)(m->len - m->append_pos));
    void *result = out.oom ? NULL : java_regex_framework_new_string16(env, out.p ? out.p : (const uint16_t *)u"", out.n);
    free(out.p);
    return result;
}

/* Pattern.split semantics (also String.split). */
static void *java_regex_framework_split_to_array(void *env, java_regex_framework_PatternPayload *p, const uint16_t *text, int32_t len, int32_t limit) {
    uint16_t *copy = malloc((size_t)(len > 0 ? len : 1) * sizeof(*copy));
    if (!copy) return NULL;
    if (len > 0) memcpy(copy, text, (size_t)len * sizeof(*copy));
    java_regex_framework_MatcherPayload m = {0};
    m.magic = MATCHER_MAGIC;
    m.text = copy;
    m.len = len;
    if (!java_regex_framework_matcher_set_pattern(&m, p)) { free(copy); return NULL; }
    java_regex_framework_matcher_reset(&m);
    int32_t *starts = NULL, *ends = NULL;
    size_t count = 0, cap = 0;
    int32_t index = 0;
    const bool limited = limit > 0;
    bool ok = true;
    while (ok && java_regex_framework_matcher_find_next(env, &m)) {
        int32_t s = -1, e = -1;
        if (!limited || count < (size_t)limit - 1u) {
            if (index == 0 && m.first == 0 && m.first == m.last) continue;
            s = index; e = m.first;
            index = m.last;
        } else if (count == (size_t)limit - 1u) {
            s = index; e = len;
            index = m.last;
        } else continue;
        if (count == cap) {
            const size_t next = cap ? cap * 2u : 16u;
            int32_t *ns = realloc(starts, next * sizeof(*ns));
            if (ns) starts = ns;
            int32_t *ne = ns ? realloc(ends, next * sizeof(*ne)) : NULL;
            if (ne) ends = ne;
            if (!ns || !ne) { ok = false; break; }
            cap = next;
        }
        starts[count] = s; ends[count] = e; count++;
    }
    void *array = NULL;
    if (ok && !ssrnx_jni_exception_pending_host(env)) {
        if (index == 0) {
            /* No match: the whole input. */
            count = 0;
            array = ssrnx_jni_new_local_object_array(env, 1);
            void *s = array ? java_regex_framework_new_string16(env, copy, (size_t)len) : NULL;
            if (s) { (void)ssrnx_jni_set_object_array_element_host(env, array, 0, s); ssrnx_jni_delete_local_ref_host(env, s); }
        } else {
            if (!limited || count < (size_t)limit) {
                if (count == cap) {
                    int32_t *ns = realloc(starts, (cap + 1) * sizeof(*ns));
                    if (ns) starts = ns;
                    int32_t *ne = ns ? realloc(ends, (cap + 1) * sizeof(*ne)) : NULL;
                    if (ne) ends = ne;
                    ok = ns && ne;
                }
                if (ok) { starts[count] = index; ends[count] = len; count++; }
            }
            size_t size = count;
            if (limit == 0) while (size > 0 && starts[size - 1] == ends[size - 1]) size--;
            array = ok ? ssrnx_jni_new_local_object_array(env, (int32_t)size) : NULL;
            for (size_t i = 0; array && i < size; ++i) {
                void *s = java_regex_framework_new_string16(env, copy + starts[i], (size_t)(ends[i] - starts[i]));
                if (!s) { ssrnx_jni_delete_local_ref_host(env, array); array = NULL; break; }
                (void)ssrnx_jni_set_object_array_element_host(env, array, (int32_t)i, s);
                ssrnx_jni_delete_local_ref_host(env, s);
            }
        }
    }
    free(starts); free(ends); free(m.groups); free(copy);
    if (!array && !ssrnx_jni_exception_pending_host(env)) java_regex_framework_throw_msg(env, "java/lang/OutOfMemoryError", "split");
    return array;
}

/* ------------------------------------------------------------ Pattern methods */

static void *java_regex_framework_matcher_new_object(void *env, java_regex_framework_PatternPayload *p, void *input_ref) {
    int32_t len = 0;
    uint16_t *text = java_regex_framework_charseq_utf16(env, input_ref, &len);
    if (!text) return NULL;
    java_regex_framework_MatcherPayload *m = java_regex_framework_matcher_create(p, text, len);
    if (!m) { free(text); java_regex_framework_throw_msg(env, "java/lang/OutOfMemoryError", "Pattern.matcher"); return NULL; }
    void *obj = ssrnx_jni_new_local_object(env, MATCHER_CLASS);
    if (obj && ssrnx_jni_set_object_host_data(obj, m) != SSRNX_OK) { ssrnx_jni_delete_local_ref_host(env, obj); obj = NULL; }
    return obj;
}

static SsrnxJniValue java_regex_framework_pattern_matcher(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    java_regex_framework_PatternPayload *p = java_regex_framework_pattern_payload(token);
    return java_regex_framework_v_obj(p && n == 1 ? java_regex_framework_matcher_new_object(env, p, a[0].l) : NULL);
}

static SsrnxJniValue java_regex_framework_pattern_split(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    java_regex_framework_PatternPayload *p = java_regex_framework_pattern_payload(token);
    if (!p || n < 1) return java_regex_framework_v_obj(NULL);
    int32_t len = 0;
    uint16_t *text = java_regex_framework_charseq_utf16(env, a[0].l, &len);
    if (!text) return java_regex_framework_v_obj(NULL);
    void *array = java_regex_framework_split_to_array(env, p, text, len, n == 2 ? a[1].i : 0);
    free(text);
    return java_regex_framework_v_obj(array);
}

/* Pattern.matches(regex, input) */
static SsrnxJniValue java_regex_framework_pattern_matches_static(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)token; (void)u;
    if (n != 2 || !a[0].l) { java_regex_framework_throw_msg(env, "java/lang/NullPointerException", "regex == null"); return java_regex_framework_v_z(false); }
    java_regex_framework_PatternPayload *p = java_regex_framework_pattern_cached(env, a[0].l);
    int32_t len = 0;
    uint16_t *text = p ? java_regex_framework_charseq_utf16(env, a[1].l, &len) : NULL;
    if (!text) return java_regex_framework_v_z(false);
    int32_t *groups = malloc(2u * (ssrnx_java_regex_group_count(p->re) + 1u) * sizeof(*groups));
    const int rc = groups ? ssrnx_java_regex_match(p->re, text, 0, (size_t)len, 0, 0, SSRNX_JREGEX_MATCHES, groups) : -1;
    free(groups); free(text);
    if (rc < 0) java_regex_framework_throw_msg(env, "java/lang/StackOverflowError", "regular expression backtracking too deep");
    return java_regex_framework_v_z(rc == 1);
}

/* ------------------------------------------------------------ Matcher methods */

enum java_regex_framework_MatcherOp {
    M_MATCHES, M_LOOKING_AT, M_FIND, M_FIND_FROM, M_GROUP_COUNT, M_GROUP, M_GROUP_INT, M_GROUP_NAME,
    M_START, M_START_INT, M_START_NAME, M_END, M_END_INT, M_END_NAME, M_RESET, M_RESET_INPUT,
    M_REPLACE_ALL, M_REPLACE_FIRST, M_REGION, M_REGION_START, M_REGION_END, M_PATTERN, M_USE_PATTERN,
    M_HIT_END, M_TO_STRING, M_ANCHORING, M_TRANSPARENT, M_HAS_ANCHORING, M_HAS_TRANSPARENT,
};

static SsrnxJniValue java_regex_framework_matcher_op(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    const enum java_regex_framework_MatcherOp op = (enum java_regex_framework_MatcherOp)(intptr_t)u;
    java_regex_framework_MatcherPayload *m = java_regex_framework_matcher_payload(token);
    if (!m) return java_regex_framework_v_obj(NULL);
    (void)n;
    switch (op) {
        case M_MATCHES: case M_LOOKING_AT:
            return java_regex_framework_v_z(java_regex_framework_matcher_run(env, m, m->from, op == M_MATCHES ? SSRNX_JREGEX_MATCHES : SSRNX_JREGEX_LOOKING_AT));
        case M_FIND: return java_regex_framework_v_z(java_regex_framework_matcher_find_next(env, m));
        case M_FIND_FROM:
            if (a[0].i < 0 || a[0].i > m->len) { java_regex_framework_throw_msg(env, "java/lang/IndexOutOfBoundsException", "Illegal start index"); return java_regex_framework_v_z(false); }
            java_regex_framework_matcher_reset(m);
            return java_regex_framework_v_z(java_regex_framework_matcher_run(env, m, a[0].i, SSRNX_JREGEX_FIND));
        case M_GROUP_COUNT: return java_regex_framework_v_i((int32_t)ssrnx_java_regex_group_count(m->pattern->re));
        case M_GROUP: case M_GROUP_INT: case M_GROUP_NAME: case M_START: case M_START_INT: case M_START_NAME:
        case M_END: case M_END_INT: case M_END_NAME: {
            if (!java_regex_framework_matcher_require_match(env, m)) return java_regex_framework_v_obj(NULL);
            int32_t group = 0;
            if (op == M_GROUP_INT || op == M_START_INT || op == M_END_INT) {
                group = a[0].i;
                if (!java_regex_framework_matcher_group_valid(env, m, group)) return java_regex_framework_v_obj(NULL);
            } else if (op == M_GROUP_NAME || op == M_START_NAME || op == M_END_NAME) {
                group = java_regex_framework_matcher_named_group(env, m, a[0].l);
                if (group < 0) return java_regex_framework_v_obj(NULL);
            }
            const int32_t gs = m->groups[2 * group], ge = m->groups[2 * group + 1];
            if (op == M_START || op == M_START_INT || op == M_START_NAME) return java_regex_framework_v_i(gs);
            if (op == M_END || op == M_END_INT || op == M_END_NAME) return java_regex_framework_v_i(ge);
            return java_regex_framework_v_obj(gs >= 0 && ge >= gs ? java_regex_framework_new_string16(env, m->text + gs, (size_t)(ge - gs)) : NULL);
        }
        case M_RESET: case M_RESET_INPUT: {
            if (op == M_RESET_INPUT) {
                int32_t len = 0;
                uint16_t *text = java_regex_framework_charseq_utf16(env, a[0].l, &len);
                if (!text) return java_regex_framework_v_obj(NULL);
                free(m->text);
                m->text = text;
                m->len = len;
            }
            java_regex_framework_matcher_reset(m);
            return java_regex_framework_v_obj(ssrnx_jni_new_local_from_token(env, token));
        }
        case M_REPLACE_ALL: case M_REPLACE_FIRST:
            return java_regex_framework_v_obj(java_regex_framework_matcher_replace(env, m, a[0].l, op == M_REPLACE_ALL));
        case M_REGION:
            if (a[0].i < 0 || a[0].i > m->len || a[1].i < a[0].i || a[1].i > m->len) {
                java_regex_framework_throw_msg(env, "java/lang/IndexOutOfBoundsException", a[0].i < 0 || a[0].i > m->len ? "start" : "end");
                return java_regex_framework_v_obj(NULL);
            }
            java_regex_framework_matcher_reset(m);
            m->from = a[0].i;
            m->to = a[1].i;
            return java_regex_framework_v_obj(ssrnx_jni_new_local_from_token(env, token));
        case M_REGION_START: return java_regex_framework_v_i(m->from);
        case M_REGION_END: return java_regex_framework_v_i(m->to);
        case M_ANCHORING: case M_TRANSPARENT:
            if (op == M_ANCHORING) m->non_anchoring = !a[0].z;
            else m->transparent = a[0].z != 0;
            return java_regex_framework_v_obj(ssrnx_jni_new_local_from_token(env, token));
        case M_HAS_ANCHORING: return java_regex_framework_v_z(!m->non_anchoring);
        case M_HAS_TRANSPARENT: return java_regex_framework_v_z(m->transparent);
        case M_PATTERN: {
            void *obj = ssrnx_jni_new_local_object(env, PATTERN_CLASS);
            if (obj) (void)ssrnx_jni_set_object_host_data(obj, m->pattern);
            return java_regex_framework_v_obj(obj);
        }
        case M_USE_PATTERN: {
            java_regex_framework_PatternPayload *p = java_regex_framework_pattern_ref_payload(a[0].l);
            if (!p) { java_regex_framework_throw_msg(env, "java/lang/IllegalArgumentException", "Pattern cannot be null"); return java_regex_framework_v_obj(NULL); }
            if (!java_regex_framework_matcher_set_pattern(m, p)) { java_regex_framework_throw_msg(env, "java/lang/OutOfMemoryError", "usePattern"); return java_regex_framework_v_obj(NULL); }
            return java_regex_framework_v_obj(ssrnx_jni_new_local_from_token(env, token));
        }
        case M_HIT_END:
            /* The last search examined the end of the region: a failed search
             * always did; a match did when it ends at the region end. */
            return java_regex_framework_v_z(m->first < 0 || m->last == m->to);
        case M_TO_STRING: {
            char head[64];
            java_regex_framework_U16Buf b = {0};
            static const uint16_t kHead[] = u"java.util.regex.Matcher[pattern=";
            java_regex_framework_buf_append(&b, kHead, sizeof(kHead) / sizeof(kHead[0]) - 1u);
            java_regex_framework_buf_append(&b, m->pattern->source, (size_t)m->pattern->source_len);
            int len = snprintf(head, sizeof(head), " region=%d,%d lastmatch=", (int)m->from, (int)m->to);
            for (int i = 0; i < len; ++i) { const uint16_t c = (uint16_t)head[i]; java_regex_framework_buf_append(&b, &c, 1); }
            if (m->first >= 0) java_regex_framework_buf_append(&b, m->text + m->first, (size_t)(m->last - m->first));
            const uint16_t close = ']';
            java_regex_framework_buf_append(&b, &close, 1);
            void *s = b.oom ? NULL : java_regex_framework_new_string16(env, b.p, b.n);
            free(b.p);
            return java_regex_framework_v_obj(s);
        }
    }
    return java_regex_framework_v_obj(NULL);
}

/* appendReplacement(StringBuffer|StringBuilder, String) / appendTail(...):
 * appended through the builder's own append(String). */
static bool java_regex_framework_builder_append(void *env, void *builder, const uint16_t *s, size_t n) {
    if (!builder) { java_regex_framework_throw_msg(env, "java/lang/NullPointerException", "sb"); return false; }
    char cls[128] = {0};
    (void)ssrnx_jni_object_class_name_host(builder, cls, sizeof(cls));
    const char *sig = strcmp(cls, "java/lang/StringBuffer") == 0 ? "(Ljava/lang/String;)Ljava/lang/StringBuffer;"
                                                                 : "(Ljava/lang/String;)Ljava/lang/StringBuilder;";
    void *str = java_regex_framework_new_string16(env, s ? s : (const uint16_t *)u"", n);
    if (!str) return false;
    SsrnxJniValue arg = {0}, out = {0};
    arg.l = str;
    const bool ok = ssrnx_android_call_virtual(env, builder, "append", sig, &arg, 1, &out);
    if (out.l) ssrnx_jni_delete_local_ref_host(env, out.l);
    ssrnx_jni_delete_local_ref_host(env, str);
    return ok;
}

static SsrnxJniValue java_regex_framework_matcher_append_replacement(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    java_regex_framework_MatcherPayload *m = java_regex_framework_matcher_payload(token);
    if (!m || n != 2) return java_regex_framework_v_obj(NULL);
    if (!a[1].l) { java_regex_framework_throw_msg(env, "java/lang/NullPointerException", "replacement"); return java_regex_framework_v_obj(NULL); }
    int32_t rlen = 0;
    uint16_t *r = ssrnx_jni_string_utf16_dup_host(a[1].l, &rlen);
    java_regex_framework_U16Buf out = {0};
    const bool ok = r && java_regex_framework_append_replacement(env, m, r, rlen, &out) && java_regex_framework_builder_append(env, a[0].l, out.p, out.n);
    free(r); free(out.p);
    return java_regex_framework_v_obj(ok ? ssrnx_jni_new_local_from_token(env, token) : NULL);
}

static SsrnxJniValue java_regex_framework_matcher_append_tail(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    java_regex_framework_MatcherPayload *m = java_regex_framework_matcher_payload(token);
    if (!m || n != 1) return java_regex_framework_v_obj(NULL);
    if (!java_regex_framework_builder_append(env, a[0].l, m->text + m->append_pos, (size_t)(m->len - m->append_pos))) return java_regex_framework_v_obj(NULL);
    return java_regex_framework_v_obj(ssrnx_jni_new_local_ref_host(env, a[0].l));
}

/* Matcher.quoteReplacement: escape '\' and '$'. */
static SsrnxJniValue java_regex_framework_matcher_quote_replacement(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)token; (void)u;
    int32_t len = 0;
    uint16_t *s = n == 1 && a[0].l ? ssrnx_jni_string_utf16_dup_host(a[0].l, &len) : NULL;
    if (!s) { java_regex_framework_throw_msg(env, "java/lang/NullPointerException", "s == null"); return java_regex_framework_v_obj(NULL); }
    java_regex_framework_U16Buf b = {0};
    for (int32_t i = 0; i < len; ++i) {
        if (s[i] == '\\' || s[i] == '$') { const uint16_t esc = '\\'; java_regex_framework_buf_append(&b, &esc, 1); }
        java_regex_framework_buf_append(&b, &s[i], 1);
    }
    free(s);
    void *out = b.oom ? NULL : java_regex_framework_new_string16(env, b.p ? b.p : (const uint16_t *)u"", b.n);
    free(b.p);
    return java_regex_framework_v_obj(out);
}

/* ------------------------------------------------------------ String methods */

static uint16_t *java_regex_framework_string_self(void *env, void *token, int32_t *len) {
    void *self = ssrnx_jni_new_local_from_token(env, token);
    uint16_t *text = self ? ssrnx_jni_string_utf16_dup_host(self, len) : NULL;
    if (self) ssrnx_jni_delete_local_ref_host(env, self);
    return text;
}

static SsrnxJniValue java_regex_framework_string_matches(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    if (n != 1) return java_regex_framework_v_z(false);
    java_regex_framework_PatternPayload *p = a[0].l ? java_regex_framework_pattern_cached(env, a[0].l) : NULL;
    if (!a[0].l) java_regex_framework_throw_msg(env, "java/lang/NullPointerException", "regex == null");
    int32_t len = 0;
    uint16_t *text = p ? java_regex_framework_string_self(env, token, &len) : NULL;
    if (!text) return java_regex_framework_v_z(false);
    int32_t *groups = malloc(2u * (ssrnx_java_regex_group_count(p->re) + 1u) * sizeof(*groups));
    const int rc = groups ? ssrnx_java_regex_match(p->re, text, 0, (size_t)len, 0, 0, SSRNX_JREGEX_MATCHES, groups) : -1;
    free(groups); free(text);
    if (rc < 0) java_regex_framework_throw_msg(env, "java/lang/StackOverflowError", "regular expression backtracking too deep");
    return java_regex_framework_v_z(rc == 1);
}

static SsrnxJniValue java_regex_framework_string_replace_regex(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    const bool all = u != NULL;
    if (n != 2) return java_regex_framework_v_obj(NULL);
    if (!a[0].l) { java_regex_framework_throw_msg(env, "java/lang/NullPointerException", "regex == null"); return java_regex_framework_v_obj(NULL); }
    java_regex_framework_PatternPayload *p = java_regex_framework_pattern_cached(env, a[0].l);
    int32_t len = 0;
    uint16_t *text = p ? java_regex_framework_string_self(env, token, &len) : NULL;
    if (!text) return java_regex_framework_v_obj(NULL);
    java_regex_framework_MatcherPayload m = {0};
    m.magic = MATCHER_MAGIC;
    m.text = text;
    m.len = len;
    void *result = NULL;
    if (java_regex_framework_matcher_set_pattern(&m, p)) result = java_regex_framework_matcher_replace(env, &m, a[1].l, all);
    free(m.groups);
    free(text);
    return java_regex_framework_v_obj(result);
}

/* String.split: a one-character literal (or an escaped non-alphanumeric one)
 * splits without the regex engine, as Android's fast path does; the result is
 * identical either way. */
static SsrnxJniValue java_regex_framework_string_split(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    if ((n != 1 && n != 2) || !a[0].l) {
        java_regex_framework_throw_msg(env, "java/lang/NullPointerException", "regex == null");
        return java_regex_framework_v_obj(NULL);
    }
    java_regex_framework_PatternPayload *p = java_regex_framework_pattern_cached(env, a[0].l);
    int32_t len = 0;
    uint16_t *text = p ? java_regex_framework_string_self(env, token, &len) : NULL;
    if (!text) return java_regex_framework_v_obj(NULL);
    void *array = java_regex_framework_split_to_array(env, p, text, len, n == 2 ? a[1].i : 0);
    free(text);
    return java_regex_framework_v_obj(array);
}

/* ------------------------------------------------------------ registration */

SsrnxResult ssrnx_java_regex_register_classes(void) {
    static const char *const k_classes[][2] = {
        {"java/util/regex/MatchResult", "java/lang/Object"},
        {PATTERN_CLASS, "java/lang/Object"},
        {MATCHER_CLASS, "java/lang/Object"},
        {PSE_CLASS, "java/lang/IllegalArgumentException"},
        {"java/lang/StackOverflowError", "java/lang/VirtualMachineError"},
    };
    SsrnxResult rc = ssrnx_jni_register_class("java/lang/VirtualMachineError", "java/lang/Error");
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(k_classes) / sizeof(k_classes[0]); ++i)
        rc = ssrnx_jni_register_class(k_classes[i][0], k_classes[i][1]);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_interface(MATCHER_CLASS, "java/util/regex/MatchResult");
    return rc;
}

SsrnxResult ssrnx_java_regex_register_members(void) {
    static const struct {
        const char *cls, *name, *sig;
        bool is_static;
        SsrnxJniMethodCallback cb;
        intptr_t op;
    } k_methods[] = {
        {PATTERN_CLASS, "compile", "(Ljava/lang/String;)Ljava/util/regex/Pattern;", true, java_regex_framework_pattern_compile_java, 0},
        {PATTERN_CLASS, "compile", "(Ljava/lang/String;I)Ljava/util/regex/Pattern;", true, java_regex_framework_pattern_compile_java, 0},
        {PATTERN_CLASS, "matches", "(Ljava/lang/String;Ljava/lang/CharSequence;)Z", true, java_regex_framework_pattern_matches_static, 0},
        {PATTERN_CLASS, "quote", "(Ljava/lang/String;)Ljava/lang/String;", true, java_regex_framework_pattern_quote, 0},
        {PATTERN_CLASS, "matcher", "(Ljava/lang/CharSequence;)Ljava/util/regex/Matcher;", false, java_regex_framework_pattern_matcher, 0},
        {PATTERN_CLASS, "pattern", "()Ljava/lang/String;", false, java_regex_framework_pattern_source, 0},
        {PATTERN_CLASS, "toString", "()Ljava/lang/String;", false, java_regex_framework_pattern_source, 0},
        {PATTERN_CLASS, "flags", "()I", false, java_regex_framework_pattern_flags, 0},
        {PATTERN_CLASS, "split", "(Ljava/lang/CharSequence;)[Ljava/lang/String;", false, java_regex_framework_pattern_split, 0},
        {PATTERN_CLASS, "split", "(Ljava/lang/CharSequence;I)[Ljava/lang/String;", false, java_regex_framework_pattern_split, 0},
        {MATCHER_CLASS, "matches", "()Z", false, java_regex_framework_matcher_op, M_MATCHES},
        {MATCHER_CLASS, "lookingAt", "()Z", false, java_regex_framework_matcher_op, M_LOOKING_AT},
        {MATCHER_CLASS, "find", "()Z", false, java_regex_framework_matcher_op, M_FIND},
        {MATCHER_CLASS, "find", "(I)Z", false, java_regex_framework_matcher_op, M_FIND_FROM},
        {MATCHER_CLASS, "groupCount", "()I", false, java_regex_framework_matcher_op, M_GROUP_COUNT},
        {MATCHER_CLASS, "group", "()Ljava/lang/String;", false, java_regex_framework_matcher_op, M_GROUP},
        {MATCHER_CLASS, "group", "(I)Ljava/lang/String;", false, java_regex_framework_matcher_op, M_GROUP_INT},
        {MATCHER_CLASS, "group", "(Ljava/lang/String;)Ljava/lang/String;", false, java_regex_framework_matcher_op, M_GROUP_NAME},
        {MATCHER_CLASS, "start", "()I", false, java_regex_framework_matcher_op, M_START},
        {MATCHER_CLASS, "start", "(I)I", false, java_regex_framework_matcher_op, M_START_INT},
        {MATCHER_CLASS, "start", "(Ljava/lang/String;)I", false, java_regex_framework_matcher_op, M_START_NAME},
        {MATCHER_CLASS, "end", "()I", false, java_regex_framework_matcher_op, M_END},
        {MATCHER_CLASS, "end", "(I)I", false, java_regex_framework_matcher_op, M_END_INT},
        {MATCHER_CLASS, "end", "(Ljava/lang/String;)I", false, java_regex_framework_matcher_op, M_END_NAME},
        {MATCHER_CLASS, "reset", "()Ljava/util/regex/Matcher;", false, java_regex_framework_matcher_op, M_RESET},
        {MATCHER_CLASS, "reset", "(Ljava/lang/CharSequence;)Ljava/util/regex/Matcher;", false, java_regex_framework_matcher_op, M_RESET_INPUT},
        {MATCHER_CLASS, "replaceAll", "(Ljava/lang/String;)Ljava/lang/String;", false, java_regex_framework_matcher_op, M_REPLACE_ALL},
        {MATCHER_CLASS, "replaceFirst", "(Ljava/lang/String;)Ljava/lang/String;", false, java_regex_framework_matcher_op, M_REPLACE_FIRST},
        {MATCHER_CLASS, "region", "(II)Ljava/util/regex/Matcher;", false, java_regex_framework_matcher_op, M_REGION},
        {MATCHER_CLASS, "regionStart", "()I", false, java_regex_framework_matcher_op, M_REGION_START},
        {MATCHER_CLASS, "regionEnd", "()I", false, java_regex_framework_matcher_op, M_REGION_END},
        {MATCHER_CLASS, "useAnchoringBounds", "(Z)Ljava/util/regex/Matcher;", false, java_regex_framework_matcher_op, M_ANCHORING},
        {MATCHER_CLASS, "useTransparentBounds", "(Z)Ljava/util/regex/Matcher;", false, java_regex_framework_matcher_op, M_TRANSPARENT},
        {MATCHER_CLASS, "hasAnchoringBounds", "()Z", false, java_regex_framework_matcher_op, M_HAS_ANCHORING},
        {MATCHER_CLASS, "hasTransparentBounds", "()Z", false, java_regex_framework_matcher_op, M_HAS_TRANSPARENT},
        {MATCHER_CLASS, "pattern", "()Ljava/util/regex/Pattern;", false, java_regex_framework_matcher_op, M_PATTERN},
        {MATCHER_CLASS, "usePattern", "(Ljava/util/regex/Pattern;)Ljava/util/regex/Matcher;", false, java_regex_framework_matcher_op, M_USE_PATTERN},
        {MATCHER_CLASS, "hitEnd", "()Z", false, java_regex_framework_matcher_op, M_HIT_END},
        {MATCHER_CLASS, "toString", "()Ljava/lang/String;", false, java_regex_framework_matcher_op, M_TO_STRING},
        {MATCHER_CLASS, "appendReplacement", "(Ljava/lang/StringBuffer;Ljava/lang/String;)Ljava/util/regex/Matcher;", false, java_regex_framework_matcher_append_replacement, 0},
        {MATCHER_CLASS, "appendReplacement", "(Ljava/lang/StringBuilder;Ljava/lang/String;)Ljava/util/regex/Matcher;", false, java_regex_framework_matcher_append_replacement, 0},
        {MATCHER_CLASS, "appendTail", "(Ljava/lang/StringBuffer;)Ljava/lang/StringBuffer;", false, java_regex_framework_matcher_append_tail, 0},
        {MATCHER_CLASS, "appendTail", "(Ljava/lang/StringBuilder;)Ljava/lang/StringBuilder;", false, java_regex_framework_matcher_append_tail, 0},
        {MATCHER_CLASS, "quoteReplacement", "(Ljava/lang/String;)Ljava/lang/String;", true, java_regex_framework_matcher_quote_replacement, 0},
        {"java/lang/String", "matches", "(Ljava/lang/String;)Z", false, java_regex_framework_string_matches, 0},
        {"java/lang/String", "replaceAll", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", false, java_regex_framework_string_replace_regex, 1},
        {"java/lang/String", "replaceFirst", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", false, java_regex_framework_string_replace_regex, 0},
        {"java/lang/String", "split", "(Ljava/lang/String;)[Ljava/lang/String;", false, java_regex_framework_string_split, 0},
        {"java/lang/String", "split", "(Ljava/lang/String;I)[Ljava/lang/String;", false, java_regex_framework_string_split, 0},
    };
    static const char *const k_match_result[][2] = {
        {"groupCount", "()I"}, {"group", "()Ljava/lang/String;"}, {"group", "(I)Ljava/lang/String;"},
        {"start", "()I"}, {"start", "(I)I"}, {"end", "()I"}, {"end", "(I)I"},
    };
    for (size_t i = 0; i < sizeof(k_methods) / sizeof(k_methods[0]); ++i) {
        const SsrnxResult rc = ssrnx_jni_register_method(k_methods[i].cls, k_methods[i].name, k_methods[i].sig,
                                                         k_methods[i].is_static, k_methods[i].cb,
                                                         (void *)k_methods[i].op);
        if (rc != SSRNX_OK) return rc;
    }
    for (size_t i = 0; i < sizeof(k_match_result) / sizeof(k_match_result[0]); ++i) {
        const SsrnxResult rc = ssrnx_jni_register_interface_method("java/util/regex/MatchResult",
                                                                   k_match_result[i][0], k_match_result[i][1]);
        if (rc != SSRNX_OK) return rc;
    }
    static const struct { const char *name; int32_t value; } k_flags[] = {
        {"UNIX_LINES", SSRNX_JREGEX_UNIX_LINES}, {"CASE_INSENSITIVE", SSRNX_JREGEX_CASE_INSENSITIVE},
        {"COMMENTS", SSRNX_JREGEX_COMMENTS}, {"MULTILINE", SSRNX_JREGEX_MULTILINE},
        {"LITERAL", SSRNX_JREGEX_LITERAL}, {"DOTALL", SSRNX_JREGEX_DOTALL},
        {"UNICODE_CASE", SSRNX_JREGEX_UNICODE_CASE}, {"CANON_EQ", SSRNX_JREGEX_CANON_EQ},
        {"UNICODE_CHARACTER_CLASS", SSRNX_JREGEX_UNICODE_CHARACTER_CLASS},
    };
    for (size_t i = 0; i < sizeof(k_flags) / sizeof(k_flags[0]); ++i) {
        SsrnxJniValue v = {0};
        v.i = k_flags[i].value;
        const SsrnxResult rc = ssrnx_jni_register_field(PATTERN_CLASS, k_flags[i].name, "I", true, v);
        if (rc != SSRNX_OK) return rc;
    }
    return SSRNX_OK;
}

void ssrnx_java_regex_reset(void) {
    pthread_mutex_lock(&java_regex_framework_g_regex_lock);
    for (java_regex_framework_PatternPayload *p = java_regex_framework_g_patterns; p;) {
        java_regex_framework_PatternPayload *next = p->next;
        ssrnx_java_regex_free(p->re);
        free(p->source);
        free(p);
        p = next;
    }
    for (java_regex_framework_MatcherPayload *m = java_regex_framework_g_matchers; m;) {
        java_regex_framework_MatcherPayload *next = m->next;
        free(m->text);
        free(m->groups);
        free(m);
        m = next;
    }
    java_regex_framework_g_patterns = NULL;
    java_regex_framework_g_matchers = NULL;
    memset(java_regex_framework_g_cache, 0, sizeof(java_regex_framework_g_cache));
    pthread_mutex_unlock(&java_regex_framework_g_regex_lock);
}

#pragma pop_macro("PSE_CLASS")
#pragma pop_macro("PATTERN_MAGIC")
#pragma pop_macro("PATTERN_CLASS")
#pragma pop_macro("PATTERN_CACHE_SIZE")
#pragma pop_macro("MATCHER_MAGIC")
#pragma pop_macro("MATCHER_CLASS")


/* ---- java_idn ---- */
#pragma push_macro("COUNT")
/* Android java.net.IDN: IDNA2003 (RFC3490), Nameprep (RFC3491 /
 * Unicode 3.2), and Punycode (RFC3492). No DNS or host-locale dependency. */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "packed_idn.inc"
#define COUNT(a) (sizeof(a)/sizeof(*(a)))
typedef struct { uint32_t *p; size_t n,cap; int error; } java_idn_Codes;
static void add(java_idn_Codes *v,uint32_t cp) {
    if(v->error)return;
    if(v->n==v->cap){size_t cap=v->cap?v->cap*2:32;if(cap<v->cap||cap>SIZE_MAX/sizeof(*v->p)){v->error=2;return;}void*p=realloc(v->p,cap*sizeof(*v->p));if(!p){v->error=2;return;}v->p=p;v->cap=cap;}
    v->p[v->n++]=cp;
}
static void copy(java_idn_Codes *v,const uint32_t *p,size_t n){for(size_t i=0;i<n;i++)add(v,p[i]);}
static unsigned java_idn_ccc(uint32_t cp){size_t l=0,h=COUNT(idn_ccc);while(l<h){size_t m=l+(h-l)/2;if(idn_ccc[m].cp<cp)l=m+1;else h=m;}return l<COUNT(idn_ccc)&&idn_ccc[l].cp==cp?idn_ccc[l].cls:0;}
static unsigned flags(uint32_t cp){size_t l=0,h=COUNT(idn_ranges);while(l<h){size_t m=l+(h-l)/2;if(idn_ranges[m].last<cp)l=m+1;else h=m;}return l<COUNT(idn_ranges)&&idn_ranges[l].first<=cp?idn_ranges[l].flags:0;}
static uint32_t java_idn_composition(uint32_t a,uint32_t b) {
    if(a>=0x1100&&a<0x1113&&b>=0x1161&&b<0x1176)return 0xac00+(a-0x1100)*588+(b-0x1161)*28;
    if(a>=0xac00&&a<=0xd7a3&&(a-0xac00)%28==0&&b>0x11a7&&b<0x11c3)return a+b-0x11a7;
    uint64_t key=((uint64_t)a<<21)|b;size_t l=0,h=COUNT(idn_compositions);
    while(l<h){size_t m=l+(h-l)/2;if(idn_compositions[m].pair<key)l=m+1;else h=m;}
    return l<COUNT(idn_compositions)&&idn_compositions[l].pair==key?idn_compositions[l].cp:0;
}
static java_idn_Codes java_idn_prep(const uint32_t *p,size_t n,int option) {
    java_idn_Codes v={0};
    for(size_t i=0;i<n;i++){size_t l=0,h=COUNT(idn_maps);while(l<h){size_t m=l+(h-l)/2;if(idn_maps[m].cp<p[i])l=m+1;else h=m;}
        if(l<COUNT(idn_maps)&&idn_maps[l].cp==p[i])copy(&v,idn_map_data+idn_maps[l].offset,idn_maps[l].length);else add(&v,p[i]);}
    if(v.error)return v;
    for(size_t i=1;i<v.n;i++){uint32_t cp=v.p[i];unsigned cls=java_idn_ccc(cp);size_t j=i;while(j&&cls&&java_idn_ccc(v.p[j-1])>cls){v.p[j]=v.p[j-1];j--;}v.p[j]=cp;}
    size_t out=0,starter=SIZE_MAX;unsigned previous=0;
    for(size_t i=0;i<v.n;i++){uint32_t cp=v.p[i];unsigned cls=java_idn_ccc(cp);uint32_t joined=starter!=SIZE_MAX&&(previous<cls||previous==0)?java_idn_composition(v.p[starter],cp):0;
        if(joined)v.p[starter]=joined;else{if(!cls)starter=out;v.p[out++]=cp;previous=cls;}}
    v.n=out;bool rtl=false,left=false;
    for(size_t i=0;i<v.n;i++){unsigned f=flags(v.p[i]);if((f&2)||((f&1)&&!(option&1)))v.error=1;rtl|=(f&4)!=0;left|=(f&8)!=0;}
    if(rtl&&(left||!v.n||!(flags(v.p[0])&4)||!(flags(v.p[v.n-1])&4)))v.error=1;
    return v;
}
static uint32_t java_idn_lower(uint32_t c){return c>='A'&&c<='Z'?c+32:c;}
static bool java_idn_ace(const uint32_t *p,size_t n){return n>=4&&java_idn_lower(p[0])=='x'&&java_idn_lower(p[1])=='n'&&p[2]=='-'&&p[3]=='-';}
static unsigned java_idn_adapt(uint64_t delta,uint64_t points,bool first){delta=first?delta/700:delta/2;delta+=delta/points;unsigned k=0;while(delta>455){delta/=35;k+=36;}return k+(unsigned)(36*delta/(delta+38));}
static unsigned java_idn_threshold(unsigned k,unsigned bias){return k<=bias?1:k>=bias+26?26:k-bias;}
static uint32_t digit(unsigned n){return n<26?'a'+n:'0'+n-26;}
static int java_idn_undigit(uint32_t c){c=java_idn_lower(c);return c>='a'&&c<='z'?(int)(c-'a'):c>='0'&&c<='9'?(int)(c-'0'+26):-1;}
static void java_idn_encode(java_idn_Codes *out,const uint32_t *p,size_t len) {
    uint64_t n=128,delta=0;unsigned bias=72;size_t basic=0;
    for(size_t i=0;i<len;i++)if(p[i]<128){add(out,p[i]);basic++;}
    size_t done=basic;if(basic)add(out,'-');
    while(done<len&&!out->error){uint32_t next=UINT32_MAX;for(size_t i=0;i<len;i++)if(p[i]>=n&&p[i]<next)next=p[i];
        delta+=(next-n)*(done+1);n=next;if(delta>UINT32_MAX){out->error=1;break;}
        for(size_t i=0;i<len;i++){if(p[i]<n)delta++;if(p[i]==n){uint64_t q=delta;
            for(unsigned k=36;;k+=36){unsigned t=java_idn_threshold(k,bias);if(q<t)break;add(out,digit(t+(unsigned)((q-t)%(36-t))));q=(q-t)/(36-t);}
            add(out,digit((unsigned)q));bias=java_idn_adapt(delta,done+1,done==basic);delta=0;done++;}}
        delta++;n++;}
}
static java_idn_Codes decode(const uint32_t *p,size_t len) {
    java_idn_Codes out={0};size_t pos=0;for(size_t j=0;j<len;j++)if(p[j]=='-')pos=j+1;
    if(pos)copy(&out,p,pos-1);
    for(size_t j=0;j<out.n;j++)if(out.p[j]>=128)out.error=1;
    uint64_t n=128,i=0;unsigned bias=72;
    while(pos<len&&!out.error){uint64_t old=i,w=1;
        for(unsigned k=36;;k+=36){if(pos==len){out.error=1;break;}int d=java_idn_undigit(p[pos++]);if(d<0||(uint64_t)d>(UINT32_MAX-i)/w){out.error=1;break;}i+=(unsigned)d*w;unsigned t=java_idn_threshold(k,bias);if((unsigned)d<t)break;if(w>UINT32_MAX/(36-t)){out.error=1;break;}w*=36-t;}
        if(out.error)break;
        bias=java_idn_adapt(i-old,out.n+1,old==0);n+=i/(out.n+1);i%=out.n+1;
        if(n>0x10ffff||(n>=0xd800&&n<=0xdfff)){out.error=1;break;}
        size_t at=(size_t)i;add(&out,(uint32_t)n);if(out.error)break;memmove(out.p+at+1,out.p+at,(out.n-at-1)*sizeof(*out.p));out.p[at]=(uint32_t)n;i++;
    }
    return out;
}
static java_idn_Codes java_idn_ascii_label(const uint32_t *p,size_t n,int option) {
    java_idn_Codes v={0};bool nonascii=false;for(size_t i=0;i<n;i++)nonascii|=p[i]>=128;
    if(nonascii)v=java_idn_prep(p,n,option);else copy(&v,p,n);
    if(v.error)return v;
    if(!v.n){v.error=1;return v;}
    if(option&2){for(size_t i=0;i<v.n;i++)if(v.p[i]<128&&!((java_idn_lower(v.p[i])>='a'&&java_idn_lower(v.p[i])<='z')||(v.p[i]>='0'&&v.p[i]<='9')||v.p[i]=='-'))v.error=1;if(v.p[0]=='-'||v.p[v.n-1]=='-')v.error=1;}
    nonascii=false;for(size_t i=0;i<v.n;i++)nonascii|=v.p[i]>=128;
    if(nonascii&&!v.error){java_idn_Codes out={0};if(java_idn_ace(v.p,v.n))out.error=1;else{add(&out,'x');add(&out,'n');add(&out,'-');add(&out,'-');java_idn_encode(&out,v.p,v.n);}free(v.p);v=out;}
    if(v.n>63)v.error=1;
    return v;
}
static java_idn_Codes java_idn_unicode_label(const uint32_t *p,size_t n,int option) {
    java_idn_Codes candidate={0};bool nonascii=false;for(size_t j=0;j<n;j++)nonascii|=p[j]>=128;
    if(nonascii)candidate=java_idn_prep(p,n,option);else copy(&candidate,p,n);
    if(!candidate.error&&java_idn_ace(candidate.p,candidate.n)) {
        java_idn_Codes decoded=decode(candidate.p+4,candidate.n-4);
        if(!decoded.error){java_idn_Codes check=java_idn_ascii_label(decoded.p,decoded.n,option);bool match=!check.error&&check.n==candidate.n;
            for(size_t j=0;match&&j<check.n;j++)match=java_idn_lower(check.p[j])==java_idn_lower(candidate.p[j]);
            free(check.p);
            /* Android's ICU propagates a failed ToASCII verification to the
             * outer name conversion, whose wrapper returns the whole input. */
            if(check.error){free(candidate.p);decoded.error=check.error;return decoded;}
            if(match){free(candidate.p);return decoded;}}
        if(decoded.error==2)candidate.error=2;
        free(decoded.p);
    }
    int memory_error=candidate.error==2;free(candidate.p);candidate=(java_idn_Codes){0};copy(&candidate,p,n);if(memory_error)candidate.error=2;return candidate;
}
static bool dot(uint32_t cp){return cp=='.'||cp==0x3002||cp==0xff0e||cp==0xff61;}
static SsrnxJniValue java_idn_convert(void *env,void *token,const SsrnxJniValue *args,size_t count,void *unicode) {
    (void)token;SsrnxJniValue out={0};
    if(!count||!args[0].l){(void)ssrnx_jni_throw_new_by_name(env,"java/lang/NullPointerException","input");return out;}
    int length=0;uint16_t *input=ssrnx_jni_string_utf16_dup_host(args[0].l,&length);java_idn_Codes in={0},result={0};
    if(!input){(void)ssrnx_jni_throw_new_by_name(env,"java/lang/OutOfMemoryError","IDN input");return out;}
    for(int j=0;j<length;j++){uint32_t cp=input[j];if(cp>=0xd800&&cp<=0xdbff&&j+1<length&&input[j+1]>=0xdc00&&input[j+1]<=0xdfff){cp=0x10000+((cp-0xd800)<<10)+(input[++j]-0xdc00);}add(&in,cp);}free(input);
    int option=count==2?args[1].i:0;
    for(size_t start=0;start<in.n;){size_t end=start;while(end<in.n&&!dot(in.p[end]))end++;
        if(unicode&&end==start&&end<in.n){result.error=1;break;}
        java_idn_Codes label={0};if(!unicode&&in.n==1&&in.p[0]=='.'){}else label=unicode?java_idn_unicode_label(in.p+start,end-start,option):java_idn_ascii_label(in.p+start,end-start,option);
        copy(&result,label.p,label.n);if(label.error&&result.error!=2)result.error=label.error;free(label.p);if(end<in.n)add(&result,'.');start=end+1;if(result.error)break;}
    size_t units=result.n;for(size_t j=0;j<result.n;j++)if(result.p[j]>0xffff)units++;
    if(units>255&&result.error!=2)result.error=1;
    if(unicode&&result.error==1){free(result.p);result=(java_idn_Codes){0};copy(&result,in.p,in.n);}
    if(in.error)result.error=in.error;
    free(in.p);
    if(result.error){(void)ssrnx_jni_throw_new_by_name(env,result.error==2?"java/lang/OutOfMemoryError":"java/lang/IllegalArgumentException","IDN conversion failed");free(result.p);return out;}
    if(result.n>INT32_MAX/2){(void)ssrnx_jni_throw_new_by_name(env,"java/lang/OutOfMemoryError","IDN output");free(result.p);return out;}
    uint16_t *chars=malloc((result.n*2+1)*sizeof(*chars));size_t used=0;
    if(chars){for(size_t j=0;j<result.n;j++){uint32_t cp=result.p[j];if(cp>0xffff){cp-=0x10000;chars[used++]=0xd800+(cp>>10);chars[used++]=0xdc00+(cp&1023);}else chars[used++]=(uint16_t)cp;}out.l=ssrnx_jni_new_local_string_utf16(env,chars,(int32_t)used);free(chars);}
    else (void)ssrnx_jni_throw_new_by_name(env,"java/lang/OutOfMemoryError","IDN output");
    free(result.p);return out;
}
SsrnxResult ssrnx_java_idn_register(void) {
    if (!packed_idn_ready()) return SSRNX_ERR_APK_INVALID;
    SsrnxResult rc=ssrnx_jni_register_class("java/net/IDN","java/lang/Object");if(rc!=SSRNX_OK)return rc;
    const char *sigs[]={"(Ljava/lang/String;)Ljava/lang/String;","(Ljava/lang/String;I)Ljava/lang/String;"};
    for(size_t j=0;j<2;j++)for(size_t k=0;k<2;k++){rc=ssrnx_jni_register_method("java/net/IDN",k?"toUnicode":"toASCII",sigs[j],true,java_idn_convert,k?(void*)1:NULL);if(rc!=SSRNX_OK)return rc;}
    rc=ssrnx_jni_register_field("java/net/IDN","ALLOW_UNASSIGNED","I",true,(SsrnxJniValue){.i=1});if(rc!=SSRNX_OK)return rc;
    return ssrnx_jni_register_field("java/net/IDN","USE_STD3_ASCII_RULES","I",true,(SsrnxJniValue){.i=2});
}

#pragma pop_macro("COUNT")


/* ---- unicode_normalization ---- */
#pragma push_macro("UN_COUNT")
/* Unicode 13 UAX #15 normalization, matching Android 11's Unicode profile. */

#include <stdbool.h>
#include <stdlib.h>
#include "packed_norm.inc"
#define UN_COUNT(a) (sizeof(a)/sizeof(*(a)))

typedef struct unicode_normalization_UnBuffer { uint32_t *p; size_t n,cap; bool failed; } unicode_normalization_UnBuffer;

static unsigned unicode_normalization_un_class(uint32_t cp) {
    size_t lo=0,hi=UN_COUNT(un_classes);
    while(lo<hi){size_t mid=lo+(hi-lo)/2;if(un_classes[mid].cp<cp)lo=mid+1;else hi=mid;}
    return lo<UN_COUNT(un_classes)&&un_classes[lo].cp==cp?un_classes[lo].value:0;
}

static void unicode_normalization_un_append(unicode_normalization_UnBuffer *b, uint32_t cp) {
    if(b->failed)return;
    if(b->n==b->cap){
        size_t cap=b->cap?b->cap*2:32;
        if(cap<b->cap||cap>SIZE_MAX/sizeof(*b->p)){b->failed=true;return;}
        void *p=realloc(b->p,cap*sizeof(*b->p));
        if(!p){b->failed=true;return;}b->p=p;b->cap=cap;
    }
    b->p[b->n++]=cp;
}

static bool unicode_normalization_un_reorder(unicode_normalization_UnBuffer *b) {
    uint32_t *scratch=NULL;size_t capacity=0;
    for(size_t first=0;first<b->n;){
        if(!unicode_normalization_un_class(b->p[first])){++first;continue;}
        size_t end=first+1;
        while(end<b->n&&unicode_normalization_un_class(b->p[end]))++end;
        size_t length=end-first;
        if(length<=32){
            for(size_t i=first+1;i<end;i++){
                uint32_t cp=b->p[i];unsigned cls=unicode_normalization_un_class(cp);size_t pos=i;
                while(pos>first&&unicode_normalization_un_class(b->p[pos-1])>cls){b->p[pos]=b->p[pos-1];--pos;}
                b->p[pos]=cp;
            }
        }else{
            /* Stable counting sort bounds work for hostile long combining runs. */
            if(length>capacity){
                void *p=realloc(scratch,length*sizeof(*scratch));
                if(!p){free(scratch);return false;}scratch=p;capacity=length;
            }
            size_t positions[256]={0},total=0;
            for(size_t i=first;i<end;i++)++positions[unicode_normalization_un_class(b->p[i])];
            for(size_t cls=0;cls<256;cls++){size_t count=positions[cls];positions[cls]=total;total+=count;}
            for(size_t i=first;i<end;i++)scratch[positions[unicode_normalization_un_class(b->p[i])]++]=b->p[i];
            for(size_t i=0;i<length;i++)b->p[first+i]=scratch[i];
        }
        first=end;
    }
    free(scratch);return true;
}

static void unicode_normalization_un_decompose(unicode_normalization_UnBuffer *b, uint32_t cp, bool compatibility) {
    if(cp>=0xac00&&cp<=0xd7a3){
        uint32_t s=cp-0xac00;
        unicode_normalization_un_append(b,0x1100+s/588);unicode_normalization_un_append(b,0x1161+(s%588)/28);
        if(s%28)unicode_normalization_un_append(b,0x11a7+s%28);
        return;
    }
    size_t lo=0,hi=UN_COUNT(un_decompositions);
    while(lo<hi){size_t mid=lo+(hi-lo)/2;if(un_decompositions[mid].cp<cp)lo=mid+1;else hi=mid;}
    if(lo<UN_COUNT(un_decompositions)&&un_decompositions[lo].cp==cp&&
       (compatibility||!un_decompositions[lo].compatibility)){
        for(unsigned i=0;i<un_decompositions[lo].length;i++)
            unicode_normalization_un_decompose(b,un_decomposition_data[un_decompositions[lo].offset+i],compatibility);
    }else unicode_normalization_un_append(b,cp);
}

static uint32_t unicode_normalization_un_compose_pair(uint32_t a, uint32_t b) {
    if(a>=0x1100&&a<0x1113&&b>=0x1161&&b<0x1176)
        return 0xac00+(a-0x1100)*588+(b-0x1161)*28;
    if(a>=0xac00&&a<=0xd7a3&&(a-0xac00)%28==0&&b>0x11a7&&b<0x11c3)
        return a+b-0x11a7;
    uint64_t pair=((uint64_t)a<<21)|b;
    size_t lo=0,hi=UN_COUNT(un_compositions);
    while(lo<hi){size_t mid=lo+(hi-lo)/2;if(un_compositions[mid].pair<pair)lo=mid+1;else hi=mid;}
    return lo<UN_COUNT(un_compositions)&&un_compositions[lo].pair==pair?un_compositions[lo].cp:0;
}

int ssrnx_unicode_normalize(const uint16_t *input, size_t length, int form,
                            uint16_t **output, size_t *output_length) {
    if((!input&&length)||!output||!output_length||form<0||form>3)return -1;
    if(!packed_norm_ready())return -2;
    unicode_normalization_UnBuffer b={0};
    for(size_t i=0;i<length;i++){
        uint32_t cp=input[i];
        if(cp>=0xd800&&cp<=0xdbff&&i+1<length&&input[i+1]>=0xdc00&&input[i+1]<=0xdfff)
            cp=0x10000+((cp-0xd800)<<10)+(input[++i]-0xdc00);
        unicode_normalization_un_decompose(&b,cp,form>=2);
        if(b.failed){free(b.p);return -2;}
    }
    if(!unicode_normalization_un_reorder(&b)){free(b.p);return -2;}
    if(form&1){
        size_t out=0,starter=SIZE_MAX;unsigned previous=0;
        for(size_t i=0;i<b.n;i++){
            uint32_t cp=b.p[i];unsigned cls=unicode_normalization_un_class(cp);
            uint32_t composed=starter!=SIZE_MAX&&(previous==0||previous<cls)?unicode_normalization_un_compose_pair(b.p[starter],cp):0;
            if(composed)b.p[starter]=composed;
            else{if(!cls)starter=out;b.p[out++]=cp;previous=cls;}
        }
        b.n=out;
    }
    size_t units=b.n;
    for(size_t i=0;i<b.n;i++)if(b.p[i]>0xffff){if(units==SIZE_MAX){free(b.p);return -2;}++units;}
    if(units>SIZE_MAX/sizeof(uint16_t)-1){free(b.p);return -2;}
    uint16_t *text=malloc((units+1)*sizeof(*text));
    if(!text){free(b.p);return -2;}
    size_t pos=0;
    for(size_t i=0;i<b.n;i++){
        uint32_t cp=b.p[i];
        if(cp>0xffff){cp-=0x10000;text[pos++]=(uint16_t)(0xd800+(cp>>10));text[pos++]=(uint16_t)(0xdc00+(cp&1023));}
        else text[pos++]=(uint16_t)cp;
    }
    text[pos]=0;free(b.p);*output=text;*output_length=pos;return 0;
}

#pragma pop_macro("UN_COUNT")


/* ---- java_zip ---- */
#pragma push_macro("ZIP_ITERATOR")
#pragma push_macro("ZIP_STREAM")
/* Java ZipFile bridge over the shared APK/ZIP reader. No package policy here. */




#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define ZIP_STREAM "java/util/zip/ZipFile$ZipFileInputStream"
#define ZIP_ITERATOR "java/util/zip/ZipFile$ZipEntryIterator"
typedef struct java_zip_JavaZip {
    SsrnxZipArchive *archive;
    char *name;
    struct java_zip_JavaZip *next;
} java_zip_JavaZip;
typedef struct java_zip_JavaZipStream {
    /* ponytail: reuse the reader's whole-entry inflation; incremental streams
     * when large-entry memory use becomes a blocker. Allocation failures throw. */
    java_zip_JavaZip *owner;
    uint8_t *data;
    size_t size, position;
    bool closed;
    struct java_zip_JavaZipStream *next;
} java_zip_JavaZipStream;
typedef struct java_zip_JavaZipIterator {
    java_zip_JavaZip *owner;
    size_t cursor, index;
    struct java_zip_JavaZipIterator *next;
} java_zip_JavaZipIterator;
static java_zip_JavaZip *java_zip_g_zips;
static java_zip_JavaZipStream *java_zip_g_streams;
static java_zip_JavaZipIterator *java_zip_g_iterators;
/* ponytail: serialize archive/stream state; per-archive locks if contended. */
static pthread_mutex_t java_zip_g_zip_lock = PTHREAD_MUTEX_INITIALIZER;
extern bool ssrnx_compat_map_guest_path(const char *, char *, size_t) __attribute__((weak));

static SsrnxJniValue object(void *p) { return (SsrnxJniValue){.l=p}; }
static SsrnxJniValue integer(int32_t i) { return (SsrnxJniValue){.i=i}; }
static SsrnxJniValue number(int64_t j) { return (SsrnxJniValue){.j=j}; }
static void error(void *env, const char *cls, const char *message) {
    (void)ssrnx_jni_throw_new_by_name(env, cls, message);
}
static void java_zip_zip_error(void *env, SsrnxResult rc, const char *name) {
    error(env, rc == SSRNX_ERR_NO_MEMORY ? "java/lang/OutOfMemoryError" :
          rc == SSRNX_ERR_IO ? "java/io/IOException" : "java/util/zip/ZipException", name);
}
static java_zip_JavaZip *java_zip_open_zip(void *env, void *token) {
    java_zip_JavaZip *z = ssrnx_jni_get_token_host_data(token);
    if (!z || !z->archive) { error(env,"java/lang/IllegalStateException","zip file closed"); return NULL; }
    return z;
}
static void java_zip_close_stream(java_zip_JavaZipStream *s) {
    if (s) { free(s->data); s->data=NULL; s->closed=true; }
}
static void java_zip_close_zip(java_zip_JavaZip *z) {
    if (!z) return;
    for (java_zip_JavaZipStream *s=java_zip_g_streams; s; s=s->next) if (s->owner==z) java_zip_close_stream(s);
    ssrnx_zip_archive_close(z->archive); z->archive=NULL;
}

static SsrnxJniValue java_zip_zip_ctor(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    void *name = n==1 && a ? a[0].l : NULL;
    SsrnxJniValue path={0}; bool found=false;
    if (!name) { error(env,"java/lang/NullPointerException","name"); return object(NULL); }
    if (u) {
        if (ssrnx_jni_invoke_method_host(env,name,"java/io/File","getPath","()Ljava/lang/String;",
                                        false,NULL,&path,&found)!=SSRNX_OK || !found || !path.l) return object(NULL);
        name=path.l;
    }
    char *guest=ssrnx_jni_string_utf_dup(name);
    if (path.l) ssrnx_jni_delete_local_ref_host(env,path.l);
    if (!guest) { error(env,"java/lang/OutOfMemoryError","ZipFile name"); return object(NULL); }
    char mapped[SSRNX_APK_PATH_CAP];
    const char *host=ssrnx_compat_map_guest_path && ssrnx_compat_map_guest_path(guest,mapped,sizeof(mapped)) ? mapped : guest;
    java_zip_JavaZip *z=calloc(1,sizeof(*z));
    SsrnxResult rc=z ? ssrnx_zip_archive_open(host,&z->archive) : SSRNX_ERR_NO_MEMORY;
    SSRNX_LOGCI("JAVA-ZIP","open guest=%s host=%s rc=%d",guest,host,(int)rc);
    if (rc!=SSRNX_OK) { java_zip_zip_error(env,rc,guest); free(z); free(guest); return object(NULL); }
    z->name=guest;
    pthread_mutex_lock(&java_zip_g_zip_lock);
    z->next=java_zip_g_zips; java_zip_g_zips=z;
    (void)ssrnx_jni_set_token_host_data(token,z);
    pthread_mutex_unlock(&java_zip_g_zip_lock);
    return object(NULL);
}
/* Android falls back from name to name/ for directory entries. */
static SsrnxResult java_zip_find_entry(java_zip_JavaZip *z, const char *name, SsrnxZipEntryInfo *info) {
    SsrnxResult rc=ssrnx_zip_archive_entry(z->archive,name,info);
    const size_t len=strlen(name);
    if (rc==SSRNX_ERR_DEPENDENCY_NOT_FOUND && len && name[len-1]!='/' && len+1<sizeof(info->name)) {
        char directory[SSRNX_APK_ENTRY_CAP];
        memcpy(directory,name,len); directory[len]='/'; directory[len+1]=0;
        rc=ssrnx_zip_archive_entry(z->archive,directory,info);
    }
    return rc;
}
static void *java_zip_entry_object(void *env, const SsrnxZipEntryInfo *info) {
    void *ref=ssrnx_jni_new_local_object(env,"java/util/zip/ZipEntry");
    if (!ref) { error(env,"java/lang/OutOfMemoryError","ZipEntry"); return NULL; }
    void *name=ssrnx_jni_new_local_string_utf(env,info->name);
    SsrnxJniValue arg={.l=name}, out={0}; bool found=false;
    if (name) (void)ssrnx_jni_invoke_method_host(env,ref,"java/util/zip/ZipEntry","<init>",
        "(Ljava/lang/String;)V",false,&arg,&out,&found);
    if (name) ssrnx_jni_delete_local_ref_host(env,name);
    if (!found || ssrnx_jni_exception_pending_host(env)) {
        if (!ssrnx_jni_exception_pending_host(env)) error(env,"java/lang/OutOfMemoryError","ZipEntry name");
        ssrnx_jni_delete_local_ref_host(env,ref); return NULL;
    }
    (void)ssrnx_jni_set_instance_field_value(ref,"size","J",number(info->uncompressed_size));
    (void)ssrnx_jni_set_instance_field_value(ref,"csize","J",number(info->compressed_size));
    (void)ssrnx_jni_set_instance_field_value(ref,"crc","J",number(info->crc32));
    (void)ssrnx_jni_set_instance_field_value(ref,"method","I",integer(info->method));
    (void)ssrnx_jni_set_instance_field_value(ref,"flag","I",integer(info->flags));
    (void)ssrnx_jni_set_instance_field_value(ref,"xdostime","J",number(info->dos_time));
    return ref;
}
static SsrnxJniValue java_zip_zip_entry(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    if (n!=1 || !a || !a[0].l) { error(env,"java/lang/NullPointerException","name"); return object(NULL); }
    char *name=ssrnx_jni_string_utf_dup(a[0].l);
    if (!name) { error(env,"java/lang/OutOfMemoryError","ZipEntry name"); return object(NULL); }
    pthread_mutex_lock(&java_zip_g_zip_lock);
    java_zip_JavaZip *z=java_zip_open_zip(env,token);
    SsrnxZipEntryInfo info;
    void *ref=NULL;
    const SsrnxResult rc=z ? java_zip_find_entry(z,name,&info) : SSRNX_ERR_DEPENDENCY_NOT_FOUND;
    if (rc==SSRNX_OK) ref=java_zip_entry_object(env,&info);
    else if (rc!=SSRNX_ERR_DEPENDENCY_NOT_FOUND) java_zip_zip_error(env,rc,name);
    pthread_mutex_unlock(&java_zip_g_zip_lock); free(name);
    return object(ref);
}
static SsrnxJniValue java_zip_zip_stream(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    if (n!=1 || !a || !a[0].l) { error(env,"java/lang/NullPointerException","entry"); return object(NULL); }
    SsrnxJniValue entry_name={0}; bool found=false;
    if (ssrnx_jni_invoke_method_host(env,a[0].l,"java/util/zip/ZipEntry","getName",
            "()Ljava/lang/String;",false,NULL,&entry_name,&found)!=SSRNX_OK || !found || !entry_name.l)
        return object(NULL);
    char *name=ssrnx_jni_string_utf_dup(entry_name.l);
    ssrnx_jni_delete_local_ref_host(env,entry_name.l);
    if (!name) { error(env,"java/lang/OutOfMemoryError","ZipEntry name"); return object(NULL); }
    pthread_mutex_lock(&java_zip_g_zip_lock);
    java_zip_JavaZip *z=java_zip_open_zip(env,token);
    SsrnxZipEntryInfo info;
    SsrnxResult rc=z ? java_zip_find_entry(z,name,&info) : SSRNX_ERR_DEPENDENCY_NOT_FOUND;
    void *ref=NULL;
    if (rc==SSRNX_OK) {
        java_zip_JavaZipStream *s=calloc(1,sizeof(*s));
        rc=s ? ssrnx_zip_archive_read(z->archive,info.name,&s->data,&s->size) : SSRNX_ERR_NO_MEMORY;
        if (rc==SSRNX_OK) {
            ref=ssrnx_jni_new_local_object(env,ZIP_STREAM);
            if (ref) { s->owner=z; s->next=java_zip_g_streams; java_zip_g_streams=s; (void)ssrnx_jni_set_object_host_data(ref,s); }
        }
        if (!ref && s) { java_zip_close_stream(s); free(s); }
    }
    if (rc!=SSRNX_OK && rc!=SSRNX_ERR_DEPENDENCY_NOT_FOUND) java_zip_zip_error(env,rc,name);
    pthread_mutex_unlock(&java_zip_g_zip_lock); free(name); return object(ref);
}
static SsrnxJniValue java_zip_stream_read(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    pthread_mutex_lock(&java_zip_g_zip_lock);
    java_zip_JavaZipStream *s=ssrnx_jni_get_token_host_data(token);
    int32_t result=-1;
    if (!s || s->closed) error(env,"java/io/IOException","Stream closed");
    else if (n==0) { if (s->position<s->size) result=s->data[s->position++]; }
    else if (!a || !a[0].l) error(env,"java/lang/NullPointerException","buffer");
    else {
        const int32_t total=ssrnx_jni_byte_array_length(a[0].l);
        const int32_t off=n==3 ? a[1].i : 0, len=n==3 ? a[2].i : total;
        if (off<0 || len<0 || total<0 || off>total || len>total-off)
            error(env,"java/lang/IndexOutOfBoundsException","InputStream.read");
        else if (!len) result=0;
        else if (s->position<s->size) {
            size_t count=s->size-s->position;
            if (count>(size_t)len) count=(size_t)len;
            if (ssrnx_jni_byte_array_write(a[0].l,off,(int32_t)count,(int8_t *)s->data+s->position)==SSRNX_OK) {
                s->position+=count; result=(int32_t)count;
            }
        }
    }
    pthread_mutex_unlock(&java_zip_g_zip_lock); return integer(result);
}
static SsrnxJniValue java_zip_stream_state(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)n;
    pthread_mutex_lock(&java_zip_g_zip_lock);
    java_zip_JavaZipStream *s=ssrnx_jni_get_token_host_data(token);
    SsrnxJniValue result={0};
    if ((intptr_t)u==0) java_zip_close_stream(s);
    else if (!s || s->closed) error(env,"java/io/IOException","Stream closed");
    else if ((intptr_t)u==1) { size_t left=s->size-s->position; result.i=left>INT32_MAX ? INT32_MAX : (int32_t)left; }
    else { int64_t count=a[0].j; if (count<0) count=0; if ((uint64_t)count>s->size-s->position) count=(int64_t)(s->size-s->position); s->position+=(size_t)count; result.j=count; }
    pthread_mutex_unlock(&java_zip_g_zip_lock); return result;
}
static SsrnxJniValue java_zip_zip_close(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)env; (void)a; (void)n; (void)u;
    pthread_mutex_lock(&java_zip_g_zip_lock); java_zip_close_zip(ssrnx_jni_get_token_host_data(token)); pthread_mutex_unlock(&java_zip_g_zip_lock);
    return object(NULL);
}
static SsrnxJniValue java_zip_zip_name(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)n; (void)u;
    java_zip_JavaZip *z=ssrnx_jni_get_token_host_data(token);
    return object(z ? ssrnx_jni_new_local_string_utf(env,z->name) : NULL);
}
static SsrnxJniValue java_zip_zip_size(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)n; (void)u;
    pthread_mutex_lock(&java_zip_g_zip_lock);
    java_zip_JavaZip *z=java_zip_open_zip(env,token); size_t count=0;
    if (z) {
        const SsrnxResult rc=ssrnx_zip_archive_count(z->archive,&count);
        if (rc!=SSRNX_OK) java_zip_zip_error(env,rc,z->name);
    }
    pthread_mutex_unlock(&java_zip_g_zip_lock); return integer((int32_t)count);
}
static SsrnxJniValue java_zip_zip_entries(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)n; (void)u;
    pthread_mutex_lock(&java_zip_g_zip_lock);
    java_zip_JavaZip *z=java_zip_open_zip(env,token); void *ref=NULL;
    if (z) {
        java_zip_JavaZipIterator *it=calloc(1,sizeof(*it));
        if (it) ref=ssrnx_jni_new_local_object(env,ZIP_ITERATOR);
        if (!ref) { free(it); error(env,"java/lang/OutOfMemoryError","ZipFile entries"); }
        else { it->owner=z; it->next=java_zip_g_iterators; java_zip_g_iterators=it; (void)ssrnx_jni_set_object_host_data(ref,it); }
    }
    pthread_mutex_unlock(&java_zip_g_zip_lock); return object(ref);
}
static SsrnxJniValue java_zip_zip_iterator(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)n;
    pthread_mutex_lock(&java_zip_g_zip_lock);
    java_zip_JavaZipIterator *it=ssrnx_jni_get_token_host_data(token);
    SsrnxJniValue result={0}; size_t count=0;
    if (!it || !it->owner->archive) error(env,"java/lang/IllegalStateException","zip file closed");
    else {
        SsrnxResult rc=ssrnx_zip_archive_count(it->owner->archive,&count);
        if (rc!=SSRNX_OK) java_zip_zip_error(env,rc,it->owner->name);
        else if (!u) result.z=it->index<count;
        else if (it->index>=count) error(env,"java/util/NoSuchElementException","ZipFile entries exhausted");
        else {
            SsrnxZipEntryInfo info; size_t cursor=it->cursor;
            rc=ssrnx_zip_archive_next(it->owner->archive,&cursor,&info);
            if (rc!=SSRNX_OK) java_zip_zip_error(env,rc,it->owner->name);
            else {
                result.l=java_zip_entry_object(env,&info);
                if (result.l) { it->cursor=cursor; ++it->index; }
            }
        }
    }
    pthread_mutex_unlock(&java_zip_g_zip_lock); return result;
}
static SsrnxJniValue java_zip_iterator_remove(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)token; (void)a; (void)n; (void)u;
    error(env,"java/lang/UnsupportedOperationException","ZipFile entries are immutable"); return object(NULL);
}
void ssrnx_java_zip_reset(void) {
    while (java_zip_g_iterators) { java_zip_JavaZipIterator *it=java_zip_g_iterators; java_zip_g_iterators=it->next; free(it); }
    while (java_zip_g_zips) { java_zip_JavaZip *z=java_zip_g_zips; java_zip_g_zips=z->next; java_zip_close_zip(z); free(z->name); free(z); }
    while (java_zip_g_streams) { java_zip_JavaZipStream *s=java_zip_g_streams; java_zip_g_streams=s->next; java_zip_close_stream(s); free(s); }
}
SsrnxResult ssrnx_java_zip_register(void) {
    const char *classes[][2]={{"java/util/zip/ZipFile","java/lang/Object"},
        
        {ZIP_STREAM,"java/io/InputStream"},{ZIP_ITERATOR,"java/lang/Object"}};
    for (size_t i=0;i<sizeof(classes)/sizeof(*classes);++i) {
        SsrnxResult rc=ssrnx_jni_register_class(classes[i][0],classes[i][1]); if (rc!=SSRNX_OK) return rc;
    }
    SsrnxResult rc=ssrnx_jni_register_interface("java/util/zip/ZipFile","java/io/Closeable"); if (rc!=SSRNX_OK) return rc;
    rc=ssrnx_jni_register_interface(ZIP_ITERATOR,"java/util/Enumeration"); if (rc!=SSRNX_OK) return rc;
    rc=ssrnx_jni_register_interface(ZIP_ITERATOR,"java/util/Iterator"); if (rc!=SSRNX_OK) return rc;
    const struct { const char *c,*name,*sig; SsrnxJniMethodCallback cb; intptr_t arg; } methods[]={
        {"java/util/zip/ZipFile","<init>","(Ljava/lang/String;)V",java_zip_zip_ctor,0},
        {"java/util/zip/ZipFile","<init>","(Ljava/io/File;)V",java_zip_zip_ctor,1},
        {"java/util/zip/ZipFile","getEntry","(Ljava/lang/String;)Ljava/util/zip/ZipEntry;",java_zip_zip_entry,0},
        {"java/util/zip/ZipFile","getInputStream","(Ljava/util/zip/ZipEntry;)Ljava/io/InputStream;",java_zip_zip_stream,0},
        {"java/util/zip/ZipFile","close","()V",java_zip_zip_close,0},
        {"java/util/zip/ZipFile","getName","()Ljava/lang/String;",java_zip_zip_name,0},
        {"java/util/zip/ZipFile","size","()I",java_zip_zip_size,0},
        {"java/util/zip/ZipFile","entries","()Ljava/util/Enumeration;",java_zip_zip_entries,0},
        {ZIP_ITERATOR,"hasMoreElements","()Z",java_zip_zip_iterator,0},
        {ZIP_ITERATOR,"hasNext","()Z",java_zip_zip_iterator,0},
        {ZIP_ITERATOR,"nextElement","()Ljava/lang/Object;",java_zip_zip_iterator,1},
        {ZIP_ITERATOR,"nextElement","()Ljava/util/zip/ZipEntry;",java_zip_zip_iterator,1},
        {ZIP_ITERATOR,"next","()Ljava/lang/Object;",java_zip_zip_iterator,1},
        {ZIP_ITERATOR,"next","()Ljava/util/zip/ZipEntry;",java_zip_zip_iterator,1},
        {ZIP_ITERATOR,"remove","()V",java_zip_iterator_remove,0},
        {ZIP_STREAM,"read","()I",java_zip_stream_read,0},{ZIP_STREAM,"read","([B)I",java_zip_stream_read,0},
        {ZIP_STREAM,"read","([BII)I",java_zip_stream_read,0},{ZIP_STREAM,"close","()V",java_zip_stream_state,0},
        {ZIP_STREAM,"available","()I",java_zip_stream_state,1},{ZIP_STREAM,"skip","(J)J",java_zip_stream_state,2},
    };
    for (size_t i=0;i<sizeof(methods)/sizeof(*methods);++i) {
        rc=ssrnx_jni_register_method(methods[i].c,methods[i].name,methods[i].sig,false,methods[i].cb,(void *)methods[i].arg);
        if (rc!=SSRNX_OK) return rc;
    }
    return SSRNX_OK;
}

#pragma pop_macro("ZIP_STREAM")
#pragma pop_macro("ZIP_ITERATOR")
