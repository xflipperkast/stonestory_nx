/* runtime security. */
#define _GNU_SOURCE 1
#include "runtime.h"


/* ---- java_security ---- */
#pragma push_macro("METHOD")
#pragma push_macro("PRINCIPAL")
#pragma push_macro("X500_MAGIC")
/* Clean-room X.500 Name codec. DER Name/RDN/AttributeTypeAndValue ownership is
 * independent of the host trust store and never implies certificate trust. */


#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <limits.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PRINCIPAL "javax/security/auth/x500/X500Principal"
#define X500_MAGIC UINT64_C(0x41504b3658353030)
typedef struct { unsigned char *p; size_t n, cap; bool bad; } java_security_Bytes;
typedef struct { char *oid; java_security_Bytes value; } java_security_Ava;
typedef struct { java_security_Ava *a; size_t n; } java_security_Rdn;
typedef struct java_security_X500 { uint64_t magic; java_security_Rdn *r; size_t n; java_security_Bytes der; struct java_security_X500 *next; } java_security_X500;
static java_security_X500 *java_security_principals;
static pthread_mutex_t java_security_principal_lock = PTHREAD_MUTEX_INITIALIZER;
static void java_security_append(java_security_Bytes *b, const void *p, size_t n) {
    if (b->bad || n > SIZE_MAX - b->n - 1) { b->bad = true; return; }
    if (b->n + n + 1 > b->cap) {
        size_t cap = b->n + n + 1;
        unsigned char *q = realloc(b->p, cap);
        if (!q) { b->bad = true; return; }
        b->p = q; b->cap = cap;
    }
    if (n) memcpy(b->p + b->n, p, n);
    b->n += n; b->p[b->n] = 0;
}
static void byte(java_security_Bytes *b, unsigned char c) { java_security_append(b, &c, 1); }
static void text(java_security_Bytes *b, const char *s) { java_security_append(b, s, strlen(s)); }
static void release(java_security_X500 *p) {
    if (!p) return;
    for (size_t i=0;i<p->n;i++) {
        for (size_t j=0;j<p->r[i].n;j++) { free(p->r[i].a[j].oid); free(p->r[i].a[j].value.p); }
        free(p->r[i].a);
    }
    free(p->r); free(p->der.p); free(p);
}
static void java_security_tlv(java_security_Bytes *out, unsigned char tag, const java_security_Bytes *value) {
    if (value->bad) { out->bad = true; return; }
    byte(out,tag);
    if (value->n < 128) byte(out,(unsigned char)value->n);
    else {
        unsigned char len[sizeof(size_t)]; size_t n=0, v=value->n;
        while(v) { len[n++]=(unsigned char)v; v>>=8; }
        byte(out,(unsigned char)(0x80|n));
        while(n) byte(out,len[--n]);
    }
    java_security_append(out,value->p,value->n);
}
/* Definite lengths only; reject overflow, truncation and non-minimal DER. */
static bool java_security_read_tlv(const unsigned char **p, const unsigned char *end, unsigned char *tag,
                     const unsigned char **value, size_t *n) {
    if (end-*p < 2) return false;
    *tag=*(*p)++; unsigned char len=*(*p)++;
    if ((*tag&31)==31) return false;
    *n=len;
    if (len&128) {
        size_t count=len&127; *n=0;
        if (!count || count>sizeof(size_t) || (size_t)(end-*p)<count || !**p) return false;
        while(count--) { if (*n>SIZE_MAX/256) return false; *n=(*n<<8)|*(*p)++; }
        if (*n<128) return false;
    }
    if (*n>(size_t)(end-*p)) return false;
    *value=*p; *p+=*n; return true;
}

static bool java_security_read_tlv_span(const unsigned char **p, const unsigned char *end,
                          unsigned char *tag, const unsigned char **value, size_t *value_size,
                          const unsigned char **span, size_t *span_size) {
    if (!p || !*p || !end || !tag || !value || !value_size || !span || !span_size) return false;
    const unsigned char *start = *p;
    if (!java_security_read_tlv(p, end, tag, value, value_size)) return false;
    *span = start;
    *span_size = (size_t)(*p - start);
    return true;
}

SsrnxResult ssrnx_java_security_x509_name_der(const uint8_t *certificate_der,
                                              size_t certificate_size,
                                              bool subject,
                                              uint8_t **out_der,
                                              size_t *out_size) {
    if (!certificate_der || !certificate_size || !out_der || !out_size)
        return SSRNX_ERR_ARGUMENT;
    *out_der = NULL;
    *out_size = 0;

    const unsigned char *p = certificate_der;
    const unsigned char *end = certificate_der + certificate_size;
    const unsigned char *value = NULL, *span = NULL;
    size_t value_size = 0, span_size = 0;
    unsigned char tag = 0;

    /* Certificate ::= SEQUENCE { tbsCertificate, signatureAlgorithm, signatureValue } */
    if (!java_security_read_tlv_span(&p, end, &tag, &value, &value_size, &span, &span_size) ||
        tag != 0x30 || p != end)
        return SSRNX_ERR_APK_INVALID;

    /* TBSCertificate is the first child of the outer sequence. */
    const unsigned char *tp = value;
    const unsigned char *tend = value + value_size;
    const unsigned char *tbs_value = NULL;
    size_t tbs_size = 0;
    if (!java_security_read_tlv_span(&tp, tend, &tag, &tbs_value, &tbs_size, &span, &span_size) ||
        tag != 0x30)
        return SSRNX_ERR_APK_INVALID;

    p = tbs_value;
    end = tbs_value + tbs_size;

    /* version [0] EXPLICIT is optional; if absent the certificate is v1. */
    const unsigned char *field_value = NULL, *field_span = NULL;
    size_t field_value_size = 0, field_span_size = 0;
    if (!java_security_read_tlv_span(&p, end, &tag, &field_value, &field_value_size,
                       &field_span, &field_span_size))
        return SSRNX_ERR_APK_INVALID;
    if (tag == 0xa0) {
        if (!java_security_read_tlv_span(&p, end, &tag, &field_value, &field_value_size,
                           &field_span, &field_span_size))
            return SSRNX_ERR_APK_INVALID;
    }
    if (tag != 0x02) return SSRNX_ERR_APK_INVALID; /* serialNumber */

    /* signature AlgorithmIdentifier */
    if (!java_security_read_tlv_span(&p, end, &tag, &field_value, &field_value_size,
                       &field_span, &field_span_size) || tag != 0x30)
        return SSRNX_ERR_APK_INVALID;

    /* issuer Name */
    const unsigned char *issuer_span = NULL;
    size_t issuer_span_size = 0;
    if (!java_security_read_tlv_span(&p, end, &tag, &field_value, &field_value_size,
                       &issuer_span, &issuer_span_size) || tag != 0x30)
        return SSRNX_ERR_APK_INVALID;

    /* validity */
    if (!java_security_read_tlv_span(&p, end, &tag, &field_value, &field_value_size,
                       &field_span, &field_span_size) || tag != 0x30)
        return SSRNX_ERR_APK_INVALID;

    /* subject Name */
    const unsigned char *subject_span = NULL;
    size_t subject_span_size = 0;
    if (!java_security_read_tlv_span(&p, end, &tag, &field_value, &field_value_size,
                       &subject_span, &subject_span_size) || tag != 0x30)
        return SSRNX_ERR_APK_INVALID;

    const unsigned char *chosen = subject ? subject_span : issuer_span;
    const size_t chosen_size = subject ? subject_span_size : issuer_span_size;
    if (!chosen || !chosen_size) return SSRNX_ERR_APK_INVALID;

    uint8_t *copy = malloc(chosen_size);
    if (!copy) return SSRNX_ERR_NO_MEMORY;
    memcpy(copy, chosen, chosen_size);
    *out_der = copy;
    *out_size = chosen_size;
    return SSRNX_OK;
}
typedef struct { const char *key,*oid; } java_security_Keyword;
static const java_security_Keyword java_security_keywords[]={
 {"CN","2.5.4.3"},{"C","2.5.4.6"},{"L","2.5.4.7"},{"ST","2.5.4.8"},
 {"STREET","2.5.4.9"},{"O","2.5.4.10"},{"OU","2.5.4.11"},{"T","2.5.4.12"},
 {"DC","0.9.2342.19200300.100.1.25"},{"UID","0.9.2342.19200300.100.1.1"},
 {"DNQ","2.5.4.46"},{"DNQUALIFIER","2.5.4.46"},{"SURNAME","2.5.4.4"},
 {"GIVENNAME","2.5.4.42"},{"INITIALS","2.5.4.43"},{"GENERATION","2.5.4.44"},
 {"EMAILADDRESS","1.2.840.113549.1.9.1"},{"SERIALNUMBER","2.5.4.5"}
};
static const char *java_security_oid_keyword(const char *oid, int format) {
    size_t count=format==1?7:format==3?sizeof(java_security_keywords)/sizeof(*java_security_keywords):9; /* RFC1779 vs RFC2253/canonical */
    for(size_t i=0;i<count;i++) {
        if((format==0||format==2)&&i==7)continue; /* T is not an RFC2253 keyword. */
        if(!strcmp(oid,java_security_keywords[i].oid))return java_security_keywords[i].key;
    }
    /* UID follows DC in the table. */
    if(format!=1 && !strcmp(oid,java_security_keywords[9].oid)) return "UID";
    return NULL;
}
static int java_security_hex(unsigned char c) {
    if(c>='0'&&c<='9') return c-'0';
    if(c>='a'&&c<='f') return c-'a'+10;
    if(c>='A'&&c<='F') return c-'A'+10;
    return -1;
}
static bool java_security_oid_arcs(const char *s, uint64_t *arcs, size_t *n) {
    *n=0;
    while(*s) {
        if(*n==64 || !isdigit((unsigned char)*s)) return false;
        uint64_t v=0;
        do { unsigned d=(unsigned)(*s++-'0'); if(v>(UINT64_MAX-d)/10) return false; v=v*10+d; } while(isdigit((unsigned char)*s));
        arcs[(*n)++]=v;
        if(!*s) break;
        if(*s++!='.' || !*s) return false;
    }
    return *n>=2 && arcs[0]<=2 && (arcs[0]==2 || arcs[1]<40) && arcs[1]<=UINT64_MAX-80;
}
static void java_security_base128(java_security_Bytes *b, uint64_t v) {
    unsigned char temp[10]; size_t n=0;
    do { temp[n++]=(unsigned char)(v&127); v>>=7; } while(v);
    while(n) { unsigned char c=temp[--n]; byte(b,(unsigned char)(c|(n?128:0))); }
}
static bool java_security_encode_oid(const char *s, java_security_Bytes *out) {
    uint64_t arcs[64]; size_t n;
    if(!java_security_oid_arcs(s,arcs,&n)) return false;
    java_security_Bytes val={0};java_security_base128(&val,arcs[0]*40+arcs[1]);
    for(size_t i=2;i<n;i++) java_security_base128(&val,arcs[i]);
    java_security_tlv(out,6,&val);free(val.p);return !out->bad;
}
static char *java_security_decode_oid(const unsigned char *p, size_t n) {
    java_security_Bytes out={0}; bool first=true;
    while(n) {
        uint64_t v=0; unsigned char c;
        if(*p==128) { free(out.p); return NULL; }
        do { if(!n || v>UINT64_MAX/128) { free(out.p); return NULL; } c=*p++;n--;v=(v<<7)|(c&127); } while(c&128);
        char s[64];
        if(first) { unsigned a=v<40?0:v<80?1:2; snprintf(s,sizeof(s),"%u.%llu",a,(unsigned long long)(v-a*40));first=false; }
        else snprintf(s,sizeof(s),".%llu",(unsigned long long)v);
        text(&out,s);
    }
    if(first||out.bad) { free(out.p); return NULL; }
    return (char*)out.p;
}

static bool java_security_x509_copy_oid(const unsigned char *p, size_t n, char *out, size_t cap) {
    char *text_oid = java_security_decode_oid(p, n);
    if (!text_oid) return false;
    const size_t len = strlen(text_oid);
    const bool ok = cap > len;
    if (ok) memcpy(out, text_oid, len + 1u);
    free(text_oid);
    return ok;
}

static bool java_security_x509_algorithm_oid(const unsigned char *seq_value, size_t seq_size,
                               char *out, size_t cap) {
    const unsigned char *p = seq_value, *end = seq_value + seq_size;
    const unsigned char *value = NULL;
    size_t value_size = 0;
    unsigned char tag = 0;
    if (!java_security_read_tlv(&p, end, &tag, &value, &value_size) || tag != 0x06)
        return false;
    return java_security_x509_copy_oid(value, value_size, out, cap);
}

static bool java_security_x509_digits(const unsigned char *p, size_t n, int *out) {
    if (!p || !out || !n) return false;
    int v = 0;
    for (size_t i = 0; i < n; ++i) {
        if (p[i] < '0' || p[i] > '9') return false;
        v = v * 10 + (int)(p[i] - '0');
    }
    *out = v;
    return true;
}

static bool java_security_x509_leap_year(int year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

static int java_security_x509_days_in_month(int year, int month) {
    static const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (month < 1 || month > 12) return 0;
    return month == 2 && java_security_x509_leap_year(year) ? 29 : days[month - 1];
}

/* Days since 1970-01-01, adapted from the proleptic Gregorian civil-date
 * identity. It avoids timezone/libc dependencies on Horizon. */
static int64_t java_security_x509_days_from_civil(int year, unsigned month, unsigned day) {
    year -= month <= 2u;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = (unsigned)(year - era * 400);
    const unsigned mp = month > 2u ? month - 3u : month + 9u;
    const unsigned doy = (153u * mp + 2u) / 5u + day - 1u;
    const unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

static bool java_security_x509_parse_time(unsigned char tag, const unsigned char *p, size_t n,
                            int64_t *out_ms) {
    if (!p || !out_ms) return false;
    int year=0, month=0, day=0, hour=0, minute=0, second=0;
    size_t off = 0;
    if (tag == 0x17) {
        int yy = 0;
        if (n != 13 || p[12] != 'Z' ||
            !java_security_x509_digits(p,2,&yy)) return false;
        year = yy >= 50 ? 1900 + yy : 2000 + yy;
        off = 2;
    } else if (tag == 0x18) {
        if (n != 15 || p[14] != 'Z' ||
            !java_security_x509_digits(p,4,&year)) return false;
        off = 4;
    } else {
        return false;
    }
    if (!java_security_x509_digits(p+off,2,&month) ||
        !java_security_x509_digits(p+off+2,2,&day) ||
        !java_security_x509_digits(p+off+4,2,&hour) ||
        !java_security_x509_digits(p+off+6,2,&minute) ||
        !java_security_x509_digits(p+off+8,2,&second))
        return false;
    if (month < 1 || month > 12 ||
        day < 1 || day > java_security_x509_days_in_month(year, month) ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
        second < 0 || second > 59)
        return false;
    const int64_t days = java_security_x509_days_from_civil(year, (unsigned)month, (unsigned)day);
    const int64_t seconds = days * INT64_C(86400) +
                            (int64_t)hour * 3600 +
                            (int64_t)minute * 60 +
                            second;
    if (seconds > INT64_MAX / 1000 || seconds < INT64_MIN / 1000) return false;
    *out_ms = seconds * 1000;
    return true;
}

SsrnxResult ssrnx_java_security_x509_metadata(const uint8_t *certificate_der,
                                              size_t certificate_size,
                                              SsrnxJavaX509Metadata *out_metadata) {
    if (!certificate_der || !certificate_size || !out_metadata)
        return SSRNX_ERR_ARGUMENT;
    memset(out_metadata, 0, sizeof(*out_metadata));

    const unsigned char *p = certificate_der;
    const unsigned char *end = certificate_der + certificate_size;
    const unsigned char *outer = NULL;
    size_t outer_size = 0;
    unsigned char tag = 0;
    if (!java_security_read_tlv(&p, end, &tag, &outer, &outer_size) || tag != 0x30 || p != end)
        return SSRNX_ERR_APK_INVALID;

    const unsigned char *outer_p = outer;
    const unsigned char *outer_end = outer + outer_size;
    const unsigned char *tbs = NULL, *tbs_span = NULL;
    size_t tbs_size = 0, tbs_span_size = 0;
    if (!java_security_read_tlv_span(&outer_p, outer_end, &tag, &tbs, &tbs_size,
                       &tbs_span, &tbs_span_size) || tag != 0x30)
        return SSRNX_ERR_APK_INVALID;

    out_metadata->tbs_certificate = tbs_span;
    out_metadata->tbs_certificate_size = tbs_span_size;

    const unsigned char *tp = tbs;
    const unsigned char *tend = tbs + tbs_size;
    const unsigned char *value = NULL, *span = NULL;
    size_t value_size = 0, span_size = 0;

    /* version [0] EXPLICIT Version DEFAULT v1 */
    out_metadata->version = 1;
    const unsigned char *field_start = tp;
    if (!java_security_read_tlv_span(&tp, tend, &tag, &value, &value_size, &span, &span_size))
        return SSRNX_ERR_APK_INVALID;
    if (tag == 0xa0) {
        const unsigned char *vp = value, *vend = value + value_size;
        const unsigned char *iv = NULL;
        size_t iv_size = 0;
        unsigned char itag = 0;
        if (!java_security_read_tlv(&vp, vend, &itag, &iv, &iv_size) || vp != vend ||
            itag != 0x02 || iv_size != 1 || iv[0] > 2)
            return SSRNX_ERR_APK_INVALID;
        out_metadata->version = (int32_t)iv[0] + 1;
        if (!java_security_read_tlv_span(&tp, tend, &tag, &value, &value_size, &span, &span_size))
            return SSRNX_ERR_APK_INVALID;
    } else {
        tp = field_start;
        if (!java_security_read_tlv_span(&tp, tend, &tag, &value, &value_size, &span, &span_size))
            return SSRNX_ERR_APK_INVALID;
    }
    if (tag != 0x02 || value_size == 0) return SSRNX_ERR_APK_INVALID;
    out_metadata->serial_value = value;
    out_metadata->serial_value_size = value_size;

    /* TBSCertificate.signature */
    if (!java_security_read_tlv(&tp, tend, &tag, &value, &value_size) || tag != 0x30)
        return SSRNX_ERR_APK_INVALID;

    /* issuer */
    if (!java_security_read_tlv(&tp, tend, &tag, &value, &value_size) || tag != 0x30)
        return SSRNX_ERR_APK_INVALID;

    /* validity ::= SEQUENCE { notBefore Time, notAfter Time } */
    const unsigned char *validity = NULL;
    size_t validity_size = 0;
    if (!java_security_read_tlv(&tp, tend, &tag, &validity, &validity_size) || tag != 0x30)
        return SSRNX_ERR_APK_INVALID;
    const unsigned char *vp = validity, *vend = validity + validity_size;
    if (!java_security_read_tlv(&vp, vend, &tag, &value, &value_size) ||
        !java_security_x509_parse_time(tag, value, value_size, &out_metadata->not_before_ms) ||
        !java_security_read_tlv(&vp, vend, &tag, &value, &value_size) ||
        !java_security_x509_parse_time(tag, value, value_size, &out_metadata->not_after_ms) ||
        vp != vend)
        return SSRNX_ERR_APK_INVALID;

    /* subject */
    if (!java_security_read_tlv(&tp, tend, &tag, &value, &value_size) || tag != 0x30)
        return SSRNX_ERR_APK_INVALID;

    /* subjectPublicKeyInfo: preserve exact DER and parse AlgorithmIdentifier OID. */
    const unsigned char *spki = NULL, *spki_span = NULL;
    size_t spki_size = 0, spki_span_size = 0;
    if (!java_security_read_tlv_span(&tp, tend, &tag, &spki, &spki_size,
                       &spki_span, &spki_span_size) || tag != 0x30)
        return SSRNX_ERR_APK_INVALID;
    const unsigned char *sp = spki, *spend = spki + spki_size;
    const unsigned char *alg = NULL;
    size_t alg_size = 0;
    if (!java_security_read_tlv(&sp, spend, &tag, &alg, &alg_size) || tag != 0x30 ||
        !java_security_x509_algorithm_oid(alg, alg_size, out_metadata->public_key_oid,
                            sizeof(out_metadata->public_key_oid)))
        return SSRNX_ERR_APK_INVALID;
    out_metadata->subject_public_key_info = spki_span;
    out_metadata->subject_public_key_info_size = spki_span_size;

    /* Remaining optional TBSCertificate fields are allowed; the outer
     * signature algorithm/value follow outside tbsCertificate. */
    const unsigned char *outer_alg = NULL;
    size_t outer_alg_size = 0;
    if (!java_security_read_tlv(&outer_p, outer_end, &tag, &outer_alg, &outer_alg_size) ||
        tag != 0x30 ||
        !java_security_x509_algorithm_oid(outer_alg, outer_alg_size, out_metadata->signature_oid,
                            sizeof(out_metadata->signature_oid)))
        return SSRNX_ERR_APK_INVALID;

    /* Preserve the exact DER encoding of AlgorithmIdentifier parameters.
     * getSigAlgParams() returns this encoded ANY value, or null when absent. */
    {
        const unsigned char *ap = outer_alg, *aend = outer_alg + outer_alg_size;
        const unsigned char *av = NULL, *aspan = NULL;
        size_t av_size = 0, aspan_size = 0;
        unsigned char atag = 0;
        if (!java_security_read_tlv(&ap, aend, &atag, &av, &av_size) || atag != 0x06)
            return SSRNX_ERR_APK_INVALID;
        if (ap < aend) {
            if (!java_security_read_tlv_span(&ap, aend, &atag, &av, &av_size,
                               &aspan, &aspan_size) || ap != aend)
                return SSRNX_ERR_APK_INVALID;
            out_metadata->signature_algorithm_params = aspan;
            out_metadata->signature_algorithm_params_size = aspan_size;
        }
    }

    const unsigned char *sig_bits = NULL;
    size_t sig_bits_size = 0;
    if (!java_security_read_tlv(&outer_p, outer_end, &tag, &sig_bits, &sig_bits_size) ||
        tag != 0x03 || sig_bits_size < 1 || sig_bits[0] != 0 || outer_p != outer_end)
        return SSRNX_ERR_APK_INVALID;
    out_metadata->signature_value = sig_bits + 1;
    out_metadata->signature_value_size = sig_bits_size - 1u;
    return SSRNX_OK;
}

static SsrnxResult java_security_x509_extensions_content(const uint8_t *certificate_der,
                                                size_t certificate_size,
                                                const uint8_t **out_extensions,
                                                size_t *out_extensions_size,
                                                bool *out_present) {
    if (!certificate_der || !certificate_size || !out_extensions ||
        !out_extensions_size || !out_present)
        return SSRNX_ERR_ARGUMENT;
    *out_extensions = NULL;
    *out_extensions_size = 0;
    *out_present = false;

    const unsigned char *p = certificate_der;
    const unsigned char *end = certificate_der + certificate_size;
    const unsigned char *outer = NULL;
    size_t outer_size = 0;
    unsigned char tag = 0;
    if (!java_security_read_tlv(&p, end, &tag, &outer, &outer_size) || tag != 0x30 || p != end)
        return SSRNX_ERR_APK_INVALID;

    const unsigned char *op = outer, *oend = outer + outer_size;
    const unsigned char *tbs = NULL;
    size_t tbs_size = 0;
    if (!java_security_read_tlv(&op, oend, &tag, &tbs, &tbs_size) || tag != 0x30)
        return SSRNX_ERR_APK_INVALID;

    const unsigned char *tp = tbs, *tend = tbs + tbs_size;
    const unsigned char *value = NULL;
    size_t value_size = 0;

    /* Optional version, then six required fields through subjectPublicKeyInfo. */
    if (!java_security_read_tlv(&tp, tend, &tag, &value, &value_size))
        return SSRNX_ERR_APK_INVALID;
    if (tag == 0xa0) {
        if (!java_security_read_tlv(&tp, tend, &tag, &value, &value_size))
            return SSRNX_ERR_APK_INVALID;
    }
    if (tag != 0x02) return SSRNX_ERR_APK_INVALID; /* serial */
    for (unsigned i = 0; i < 5u; ++i) {
        if (!java_security_read_tlv(&tp, tend, &tag, &value, &value_size) || tag != 0x30)
            return SSRNX_ERR_APK_INVALID;
    }

    /* issuerUniqueID [1] and subjectUniqueID [2] may precede extensions [3]. */
    while (tp < tend) {
        if (!java_security_read_tlv(&tp, tend, &tag, &value, &value_size))
            return SSRNX_ERR_APK_INVALID;
        if (tag == 0x81 || tag == 0x82)
            continue;
        if (tag != 0xa3)
            return SSRNX_ERR_APK_INVALID;

        const unsigned char *ep = value, *eend = value + value_size;
        const unsigned char *seq = NULL;
        size_t seq_size = 0;
        if (!java_security_read_tlv(&ep, eend, &tag, &seq, &seq_size) ||
            tag != 0x30 || ep != eend)
            return SSRNX_ERR_APK_INVALID;
        *out_extensions = seq;
        *out_extensions_size = seq_size;
        *out_present = true;
        if (tp != tend) return SSRNX_ERR_APK_INVALID;
        return SSRNX_OK;
    }
    return SSRNX_OK;
}

typedef struct java_security_X509ExtensionView {
    char oid[96];
    bool critical;
    const uint8_t *encoded_value;
    size_t encoded_value_size;
    const uint8_t *value;
    size_t value_size;
} java_security_X509ExtensionView;

static bool java_security_x509_extension_next(const uint8_t **cursor, const uint8_t *end,
                                java_security_X509ExtensionView *out) {
    if (!cursor || !*cursor || !end || !out || *cursor >= end) return false;
    memset(out, 0, sizeof(*out));

    const unsigned char *seq = NULL;
    size_t seq_size = 0;
    unsigned char tag = 0;
    if (!java_security_read_tlv(cursor, end, &tag, &seq, &seq_size) || tag != 0x30)
        return false;

    const unsigned char *p = seq, *send = seq + seq_size;
    const unsigned char *value = NULL, *span = NULL;
    size_t value_size = 0, span_size = 0;
    if (!java_security_read_tlv(&p, send, &tag, &value, &value_size) || tag != 0x06 ||
        !java_security_x509_copy_oid(value, value_size, out->oid, sizeof(out->oid)))
        return false;

    if (!java_security_read_tlv_span(&p, send, &tag, &value, &value_size, &span, &span_size))
        return false;
    if (tag == 0x01) {
        if (value_size != 1) return false;
        out->critical = value[0] != 0;
        if (!java_security_read_tlv_span(&p, send, &tag, &value, &value_size, &span, &span_size))
            return false;
    }
    if (tag != 0x04 || p != send) return false;
    out->encoded_value = span;
    out->encoded_value_size = span_size;
    out->value = value;
    out->value_size = value_size;
    return true;
}

SsrnxResult ssrnx_java_security_x509_extension(const uint8_t *certificate_der,
                                               size_t certificate_size,
                                               const char *oid,
                                               SsrnxJavaX509Extension *out_extension,
                                               bool *out_found) {
    if (!oid || !out_extension || !out_found) return SSRNX_ERR_ARGUMENT;
    memset(out_extension, 0, sizeof(*out_extension));
    *out_found = false;

    const uint8_t *extensions = NULL;
    size_t extensions_size = 0;
    bool present = false;
    SsrnxResult rc = java_security_x509_extensions_content(
        certificate_der, certificate_size, &extensions, &extensions_size, &present);
    if (rc != SSRNX_OK || !present) return rc;

    const uint8_t *p = extensions, *end = extensions + extensions_size;
    while (p < end) {
        java_security_X509ExtensionView ext;
        if (!java_security_x509_extension_next(&p, end, &ext)) return SSRNX_ERR_APK_INVALID;
        if (strcmp(ext.oid, oid) == 0) {
            out_extension->critical = ext.critical;
            out_extension->encoded_value = ext.encoded_value;
            out_extension->encoded_value_size = ext.encoded_value_size;
            out_extension->value = ext.value;
            out_extension->value_size = ext.value_size;
            *out_found = true;
            return SSRNX_OK;
        }
    }
    return SSRNX_OK;
}

SsrnxResult ssrnx_java_security_x509_extension_oids(const uint8_t *certificate_der,
                                                    size_t certificate_size,
                                                    bool critical,
                                                    char ***out_oids,
                                                    size_t *out_count) {
    if (!out_oids || !out_count) return SSRNX_ERR_ARGUMENT;
    *out_oids = NULL;
    *out_count = 0;

    const uint8_t *extensions = NULL;
    size_t extensions_size = 0;
    bool present = false;
    SsrnxResult rc = java_security_x509_extensions_content(
        certificate_der, certificate_size, &extensions, &extensions_size, &present);
    if (rc != SSRNX_OK || !present) return rc;

    char **items = NULL;
    size_t count = 0;
    const uint8_t *p = extensions, *end = extensions + extensions_size;
    while (p < end) {
        java_security_X509ExtensionView ext;
        if (!java_security_x509_extension_next(&p, end, &ext)) {
            ssrnx_java_security_x509_free_oids(items, count);
            return SSRNX_ERR_APK_INVALID;
        }
        if (ext.critical != critical) continue;
        char **grown = realloc(items, (count + 1u) * sizeof(*items));
        if (!grown) {
            ssrnx_java_security_x509_free_oids(items, count);
            return SSRNX_ERR_NO_MEMORY;
        }
        items = grown;
        items[count] = strdup(ext.oid);
        if (!items[count]) {
            ssrnx_java_security_x509_free_oids(items, count);
            return SSRNX_ERR_NO_MEMORY;
        }
        ++count;
    }
    *out_oids = items;
    *out_count = count;
    return SSRNX_OK;
}

void ssrnx_java_security_x509_free_oids(char **oids, size_t count) {
    if (!oids) return;
    for (size_t i = 0; i < count; ++i) free(oids[i]);
    free(oids);
}

SsrnxResult ssrnx_java_security_x509_key_usage(const uint8_t *certificate_der,
                                               size_t certificate_size,
                                               bool **out_usage,
                                               size_t *out_count,
                                               bool *out_present) {
    if (!out_usage || !out_count || !out_present) return SSRNX_ERR_ARGUMENT;
    *out_usage = NULL;
    *out_count = 0;
    *out_present = false;

    SsrnxJavaX509Extension ext = {0};
    bool found = false;
    SsrnxResult rc = ssrnx_java_security_x509_extension(
        certificate_der, certificate_size, "2.5.29.15", &ext, &found);
    if (rc != SSRNX_OK || !found) return rc;

    const unsigned char *p = ext.value, *end = ext.value + ext.value_size;
    const unsigned char *bits = NULL;
    size_t bits_size = 0;
    unsigned char tag = 0;
    if (!java_security_read_tlv(&p, end, &tag, &bits, &bits_size) ||
        tag != 0x03 || p != end || bits_size < 1u || bits[0] > 7u)
        return SSRNX_ERR_APK_INVALID;
    const size_t bit_count = (bits_size - 1u) * 8u - bits[0];
    const size_t out_n = bit_count < 9u ? 9u : bit_count;
    bool *usage = calloc(out_n ? out_n : 1u, sizeof(*usage));
    if (!usage) return SSRNX_ERR_NO_MEMORY;
    for (size_t i = 0; i < bit_count; ++i)
        usage[i] = (bits[1u + i / 8u] & (uint8_t)(0x80u >> (i % 8u))) != 0;
    *out_usage = usage;
    *out_count = out_n;
    *out_present = true;
    return SSRNX_OK;
}

SsrnxResult ssrnx_java_security_x509_basic_constraints(const uint8_t *certificate_der,
                                                       size_t certificate_size,
                                                       bool *out_ca,
                                                       int32_t *out_path_len,
                                                       bool *out_present) {
    if (!out_ca || !out_path_len || !out_present) return SSRNX_ERR_ARGUMENT;
    *out_ca = false;
    *out_path_len = -1;
    *out_present = false;

    SsrnxJavaX509Extension ext = {0};
    bool found = false;
    SsrnxResult rc = ssrnx_java_security_x509_extension(
        certificate_der, certificate_size, "2.5.29.19", &ext, &found);
    if (rc != SSRNX_OK || !found) return rc;

    const unsigned char *p = ext.value, *end = ext.value + ext.value_size;
    const unsigned char *seq = NULL;
    size_t seq_size = 0;
    unsigned char tag = 0;
    if (!java_security_read_tlv(&p, end, &tag, &seq, &seq_size) || tag != 0x30 || p != end)
        return SSRNX_ERR_APK_INVALID;

    const unsigned char *sp = seq, *send = seq + seq_size;
    bool ca = false;
    bool have_path = false;
    int32_t path_len = INT32_MAX;
    if (sp < send) {
        const unsigned char *value = NULL;
        size_t value_size = 0;
        const unsigned char *saved = sp;
        if (!java_security_read_tlv(&sp, send, &tag, &value, &value_size))
            return SSRNX_ERR_APK_INVALID;
        if (tag == 0x01) {
            if (value_size != 1u) return SSRNX_ERR_APK_INVALID;
            ca = value[0] != 0;
        } else {
            sp = saved;
        }
    }
    if (sp < send) {
        const unsigned char *value = NULL;
        size_t value_size = 0;
        if (!java_security_read_tlv(&sp, send, &tag, &value, &value_size) ||
            tag != 0x02 || value_size == 0u || (value[0] & 0x80u))
            return SSRNX_ERR_APK_INVALID;
        while (value_size > 1u && value[0] == 0u) {
            ++value;
            --value_size;
        }
        uint64_t parsed = 0;
        for (size_t i = 0; i < value_size; ++i) {
            if (parsed > (uint64_t)INT32_MAX >> 8u)
                return SSRNX_ERR_APK_UNSUPPORTED;
            parsed = (parsed << 8u) | value[i];
        }
        if (parsed > INT32_MAX) return SSRNX_ERR_APK_UNSUPPORTED;
        path_len = (int32_t)parsed;
        have_path = true;
    }
    if (sp != send || (have_path && !ca))
        return SSRNX_ERR_APK_INVALID;

    *out_ca = ca;
    *out_path_len = ca ? path_len : -1;
    *out_present = true;
    return SSRNX_OK;
}

SsrnxResult ssrnx_java_security_x509_has_unsupported_critical_extension(
    const uint8_t *certificate_der, size_t certificate_size, bool *out_unsupported) {
    if (!out_unsupported) return SSRNX_ERR_ARGUMENT;
    *out_unsupported = false;

    const uint8_t *extensions = NULL;
    size_t extensions_size = 0;
    bool present = false;
    SsrnxResult rc = java_security_x509_extensions_content(
        certificate_der, certificate_size, &extensions, &extensions_size, &present);
    if (rc != SSRNX_OK || !present) return rc;

    const uint8_t *p = extensions, *end = extensions + extensions_size;
    while (p < end) {
        java_security_X509ExtensionView ext;
        if (!java_security_x509_extension_next(&p, end, &ext)) return SSRNX_ERR_APK_INVALID;
        if (!ext.critical) continue;
        if (strcmp(ext.oid, "2.5.29.15") != 0 &&
            strcmp(ext.oid, "2.5.29.19") != 0) {
            *out_unsupported = true;
            return SSRNX_OK;
        }
    }
    return SSRNX_OK;
}

static java_security_Rdn *java_security_add_rdn(java_security_X500 *p) {
    java_security_Rdn *r=realloc(p->r,(p->n+1)*sizeof(*r));if(!r)return NULL;
    p->r=r;memset(&r[p->n],0,sizeof(*r));return &r[p->n++];
}
static java_security_Ava *java_security_add_ava(java_security_Rdn *r) {
    java_security_Ava *a=realloc(r->a,(r->n+1)*sizeof(*a));if(!a)return NULL;
    r->a=a;memset(&a[r->n],0,sizeof(*a));return &a[r->n++];
}
static bool java_security_printable(const java_security_Bytes *v) {
    for(size_t i=0;i<v->n;i++) {
        unsigned char c=v->p[i];
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||(c&&strchr(" '()+,-./:=?",c)))) return false;
    }
    return true;
}
static bool java_security_valid_utf8(const unsigned char *p,size_t n) {
    while(n) {
        uint32_t cp;unsigned need;unsigned char c=*p++;n--;
        if(c<128) continue;
        if(c>=0xc2&&c<=0xdf) {cp=c&31;need=1;}
        else if(c>=0xe0&&c<=0xef) {cp=c&15;need=2;}
        else if(c>=0xf0&&c<=0xf4) {cp=c&7;need=3;}
        else return false;
        if(n<need)return false;
        unsigned total=need;
        while(need--) {c=*p++;n--;if((c&0xc0)!=0x80)return false;cp=(cp<<6)|(c&63);}
        if((total==2&&cp<0x800)||(total==3&&cp<0x10000)||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return false;
    }
    return true;
}
static bool java_security_parse_text(java_security_X500 *out, const char *s) {
    if(!*s)return true;
    java_security_Rdn *r=java_security_add_rdn(out);if(!r)return false;
    while(*s) {
        while(*s==' ')s++;
        const char *begin=s;while(*s && *s!='=')s++;
        if(!*s)return false;
        const char *end=s++;while(end>begin&&end[-1]==' ')end--;
        char *key=strndup(begin,(size_t)(end-begin));if(!key)return false;
        for(char *k=key;*k;k++) *k=(char)toupper((unsigned char)*k);
        const char *oid=NULL;
        for(size_t i=0;i<sizeof(java_security_keywords)/sizeof(*java_security_keywords);i++)if(!strcmp(key,java_security_keywords[i].key))oid=java_security_keywords[i].oid;
        if(!oid) { oid=key; if(!strncmp(oid,"OID.",4))oid+=4; }
        uint64_t arcs[64];size_t num;
        if(!java_security_oid_arcs(oid,arcs,&num)) {free(key);return false;}
        java_security_Ava *a=java_security_add_ava(r);if(!a){free(key);return false;}
        a->oid=strdup(oid);free(key);if(!a->oid)return false;
        while(*s==' ')s++;
        if(*s=='#') {
            s++;java_security_Bytes raw={0};
            while(java_security_hex((unsigned char)s[0])>=0) {
                int h=java_security_hex((unsigned char)s[0]),l=java_security_hex((unsigned char)s[1]);
                if(l<0){free(raw.p);return false;} byte(&raw,(unsigned char)(h*16+l));s+=2;
            }
            const unsigned char *p=raw.p,*value=NULL;size_t len=0;unsigned char tag=0;
            bool ok=raw.n && java_security_read_tlv(&p,raw.p+raw.n,&tag,&value,&len) && p==raw.p+raw.n;
            if(!ok||raw.bad){free(raw.p);return false;}a->value=raw;
            while(*s==' ')s++;
        } else {
            java_security_Bytes value={0};bool quoted=*s=='"',closed=!quoted;size_t keep=0;
            if(quoted)s++;
            while(*s) {
                unsigned char c=(unsigned char)*s;
                if(quoted&&c=='"'){s++;closed=true;break;}
                if(!quoted&&(c==','||c==';'||c=='+'))break;
                if(c=='\\') {
                    s++;if(!*s){free(value.p);return false;}
                    int h=java_security_hex((unsigned char)s[0]),l=java_security_hex((unsigned char)s[1]);
                    if(h>=0&&l>=0){c=(unsigned char)(h*16+l);s+=2;}
                    else {
                        c=(unsigned char)*s++;
                        if(!strchr(" ,=+<>#;\"\\",c)){free(value.p);return false;}
                    }
                    byte(&value,c);keep=value.n;
                } else {
                    if(!quoted&&(c=='<'||c=='>'||c=='\"')){free(value.p);return false;}
                    s++;byte(&value,c);if(c!=' '||quoted)keep=value.n;
                }
            }
            if(!closed||value.bad){free(value.p);return false;}
            value.n=keep;
            if(!java_security_valid_utf8(value.p,value.n)){free(value.p);return false;}
            unsigned char tag=java_security_printable(&value)?0x13:0x0c;
            if(!strcmp(a->oid,"0.9.2342.19200300.100.1.25")||!strcmp(a->oid,"1.2.840.113549.1.9.1")){
                tag=0x16;for(size_t i=0;i<value.n;i++)if(value.p[i]>=128){free(value.p);return false;}
            }
            java_security_tlv(&a->value,tag,&value);free(value.p);if(a->value.bad)return false;
            while(*s==' ')s++;
        }
        if(!*s)break;
        char sep=*s++;if(sep!=','&&sep!=';'&&sep!='+')return false;
        if(!*s)return false;
        if(sep!='+') {r=java_security_add_rdn(out);if(!r)return false;}
    }
    return true;
}
static int java_security_bytes_cmp(const void *left,const void *right) {
    const java_security_Bytes *a=left,*b=right;size_t n=a->n<b->n?a->n:b->n;
    int c=memcmp(a->p,b->p,n);return c?c:a->n<b->n?-1:a->n>b->n?1:0;
}
static bool java_security_encode_name(java_security_X500 *p) {
    java_security_Bytes seq={0};
    for(size_t i=p->n;i>0;i--) {
        java_security_Rdn *r=&p->r[i-1];java_security_Bytes *ava=calloc(r->n,sizeof(*ava));java_security_Bytes set={0};
        if(!ava){free(seq.p);return false;}
        for(size_t j=0;j<r->n;j++) {
            java_security_Bytes body={0};if(!java_security_encode_oid(r->a[j].oid,&body))body.bad=true;
            java_security_append(&body,r->a[j].value.p,r->a[j].value.n);java_security_tlv(&ava[j],0x30,&body);free(body.p);
        }
        qsort(ava,r->n,sizeof(*ava),java_security_bytes_cmp);
        for(size_t j=0;j<r->n;j++){if(ava[j].bad)set.bad=true;java_security_append(&set,ava[j].p,ava[j].n);free(ava[j].p);}
        free(ava);java_security_tlv(&seq,0x31,&set);free(set.p);
    }
    java_security_tlv(&p->der,0x30,&seq);free(seq.p);return !p->der.bad;
}
static bool java_security_parse_der(java_security_X500 *out,const unsigned char *data,size_t size) {
    if(!size)return false;
    const unsigned char *p=data,*end=data+size,*v=NULL;size_t n=0;unsigned char tag;
    if(!java_security_read_tlv(&p,end,&tag,&v,&n)||tag!=0x30||p!=end)return false;
    p=v;end=v+n;
    while(p<end) {
        if(!java_security_read_tlv(&p,end,&tag,&v,&n)||tag!=0x31||!n)return false;
        java_security_Rdn *r=java_security_add_rdn(out);if(!r)return false;
        const unsigned char *q=v,*rend=v+n;
        while(q<rend) {
            if(!java_security_read_tlv(&q,rend,&tag,&v,&n)||tag!=0x30)return false;
            const unsigned char *a=v,*aend=v+n;
            if(!java_security_read_tlv(&a,aend,&tag,&v,&n)||tag!=6||!n)return false;
            java_security_Ava *ava=java_security_add_ava(r);if(!ava)return false;
            ava->oid=java_security_decode_oid(v,n);if(!ava->oid)return false;
            const unsigned char *start=a;
            if(!java_security_read_tlv(&a,aend,&tag,&v,&n)||a!=aend)return false;
            if(tag==0x0c&&!java_security_valid_utf8(v,n))return false;
            java_security_append(&ava->value,start,(size_t)(a-start));if(ava->value.bad)return false;
        }
    }
    /* DER order is root-first; Java string order is most-specific-first. */
    for(size_t i=0;i<out->n/2;i++){java_security_Rdn t=out->r[i];out->r[i]=out->r[out->n-1-i];out->r[out->n-1-i]=t;}
    java_security_append(&out->der,data,size);return !out->der.bad;
}
static void java_security_utf8(java_security_Bytes *out,uint32_t cp) {
    if(cp<128)byte(out,(unsigned char)cp);
    else if(cp<0x800){byte(out,(unsigned char)(0xc0|(cp>>6)));byte(out,(unsigned char)(0x80|(cp&63)));}
    else if(cp<0x10000){byte(out,(unsigned char)(0xe0|(cp>>12)));byte(out,(unsigned char)(0x80|((cp>>6)&63)));byte(out,(unsigned char)(0x80|(cp&63)));}
    else{byte(out,(unsigned char)(0xf0|(cp>>18)));byte(out,(unsigned char)(0x80|((cp>>12)&63)));byte(out,(unsigned char)(0x80|((cp>>6)&63)));byte(out,(unsigned char)(0x80|(cp&63)));}
}
static uint32_t java_security_next_cp(const unsigned char **p) {
    uint32_t c=*(*p)++;if(c<128)return c;
    unsigned n=c<0xe0?1:c<0xf0?2:3;c&=n==1?31:n==2?15:7;
    while(n--)c=(c<<6)|(*(*p)++&63);
    return c;
}
/* Deterministic Unicode case mapping/NFKD, independent of host locale. */
#include "../assets/platform/java_security_unicode.inc"
static unsigned java_security_combining(uint32_t cp) {
    size_t lo=0,hi=sizeof(x500_ccc)/sizeof(*x500_ccc);
    while(lo<hi){size_t m=lo+(hi-lo)/2;if(x500_ccc[m].cp<cp)lo=m+1;else hi=m;}
    return lo<sizeof(x500_ccc)/sizeof(*x500_ccc)&&x500_ccc[lo].cp==cp?x500_ccc[lo].ccc:0;
}
static unsigned java_security_case_flags(uint32_t cp) {
    size_t lo=0,hi=sizeof(x500_case)/sizeof(*x500_case);
    while(lo<hi){size_t m=lo+(hi-lo)/2;if(x500_case[m].cp<cp)lo=m+1;else hi=m;}
    return lo<sizeof(x500_case)/sizeof(*x500_case)&&x500_case[lo].cp==cp?x500_case[lo].flags:0;
}
static void java_security_normalize(java_security_Bytes *out,const java_security_Bytes *in) {
    java_security_Bytes mapped={0};const unsigned char *p=in->p,*end=p+in->n;
    bool preceded=false;
    while(p<end) {
        uint32_t cp=java_security_next_cp(&p);unsigned flags=java_security_case_flags(cp);
        if(cp==0x3a3||cp==0x3c3||cp==0x3c2) {
            const unsigned char *next=p;bool followed=false;
            while(next<end){unsigned f=java_security_case_flags(java_security_next_cp(&next));if(f&2)continue;followed=(f&1)!=0;break;}
            java_security_utf8(&mapped,preceded&&!followed?0x3c2:0x3c3);preceded=true;continue;
        }
        if(!(flags&2))preceded=(flags&1)!=0;
        if(cp>=0xac00&&cp<=0xd7a3){uint32_t s=cp-0xac00;java_security_utf8(&mapped,0x1100+s/588);java_security_utf8(&mapped,0x1161+s%588/28);if(s%28)java_security_utf8(&mapped,0x11a7+s%28);continue;}
        size_t lo=0,hi=sizeof(x500_norm)/sizeof(*x500_norm);
        while(lo<hi){size_t m=lo+(hi-lo)/2;if(x500_norm[m].cp<cp)lo=m+1;else hi=m;}
        if(lo<sizeof(x500_norm)/sizeof(*x500_norm)&&x500_norm[lo].cp==cp)text(&mapped,x500_norm[lo].s);
        else java_security_utf8(&mapped,cp);
    }
    if(mapped.bad){out->bad=true;free(mapped.p);return;}
    uint32_t *codes=calloc(mapped.n+1,sizeof(*codes));size_t n=0;
    if(!codes){out->bad=true;free(mapped.p);return;}
    p=mapped.p;end=p+mapped.n;
    while(p<end){uint32_t cp=java_security_next_cp(&p);unsigned cls=java_security_combining(cp);size_t i=n++;
        while(i&&cls&&java_security_combining(codes[i-1])>cls){codes[i]=codes[i-1];i--;}codes[i]=cp;}
    for(size_t i=0;i<n;i++)java_security_utf8(out,codes[i]);
    free(codes);free(mapped.p);
}
static void java_security_hex_bytes(java_security_Bytes *out,const java_security_Bytes *b) {
    static const char digits[]="0123456789abcdef";byte(out,'#');
    for(size_t i=0;i<b->n;i++){byte(out,digits[b->p[i]>>4]);byte(out,digits[b->p[i]&15]);}
}
static bool string_value(const java_security_Ava *a,java_security_Bytes *out,unsigned char *tag) {
    const unsigned char *p=a->value.p,*end=p+a->value.n,*v=NULL;size_t n;
    if(!java_security_read_tlv(&p,end,tag,&v,&n))return false;
    if(*tag==0x0c||*tag==0x13||*tag==0x16||*tag==0x14||*tag==0x1b){
        if(*tag==0x14){for(size_t i=0;i<n;i++)java_security_utf8(out,v[i]);}
        else java_security_append(out,v,n);
        return !out->bad&&java_security_valid_utf8(out->p,out->n);
    }
    if(*tag==0x1e){if(n%2)return false;for(size_t i=0;i<n;i+=2){uint32_t cp=(v[i]<<8)|v[i+1];if(cp>=0xd800&&cp<=0xdfff)return false;java_security_utf8(out,cp);}return !out->bad;}
    return false;
}
static java_security_Bytes java_security_format_ava(const java_security_Ava *a,int format) {
    java_security_Bytes out={0},value={0};const char *key=java_security_oid_keyword(a->oid,format);unsigned char tag=0;
    if(key)text(&out,key);else {if(format==1||format==3)text(&out,"OID.");text(&out,a->oid);}
    byte(&out,'=');bool is_string=string_value(a,&value,&tag);
    if(!is_string || ((format==0||format==2)&&!key) || (format==2&&tag!=0x0c&&tag!=0x13))java_security_hex_bytes(&out,&a->value);
    else if(format==1||format==3) {
        bool quote=false;
        for(size_t i=0;i<value.n;i++)if(value.p[i]&&strchr(",=+<>#;\"\\\t\n\r",value.p[i]))quote=true;
        if(value.n&&(value.p[0]==' '||value.p[value.n-1]==' '))quote=true;
        for(size_t i=1;i<value.n;i++)if(value.p[i]==' '&&value.p[i-1]==' ')quote=true;
        if(quote)byte(&out,'"');
        for(size_t i=0;i<value.n;i++){if(value.p[i]=='"'||value.p[i]=='\\')byte(&out,'\\');byte(&out,value.p[i]);}
        if(quote)byte(&out,'"');
    } else {
        size_t begin=0,end=value.n;
        if(format==2) {
            while(begin<end&&value.p[begin]<=32)begin++;
            while(end>begin&&value.p[end-1]<=32)end--;
            java_security_Bytes folded={0};bool previous_space=false;
            const unsigned char *v=value.p+begin,*vend=value.p+end;
            while(v<vend){uint32_t cp=java_security_next_cp(&v);
                bool whitespace=(cp>=9&&cp<=13)||(cp>=0x1c&&cp<=0x20)||cp==0x1680||
                    (cp>=0x2000&&cp<=0x200a&&cp!=0x2007)||cp==0x2028||cp==0x2029||cp==0x205f||cp==0x3000;
                if(whitespace){if(!previous_space)byte(&folded,' ');}else java_security_utf8(&folded,cp);
                previous_space=whitespace;
            }
            free(value.p);value=folded;begin=0;end=value.n;
        }
        for(size_t i=begin;i<end;i++) {
            unsigned char c=value.p[i];
            if(format==2&&c==' '&&i>begin&&value.p[i-1]==' ')continue;
            if(!c&&format!=2)text(&out,"\\00");
            else {
                bool escape=c&&strchr(format==2?",+<>;\"\\":",=+<>;\"\\",c)!=NULL;
                if(c=='#'&&(format!=2||i==begin))escape=true;
                if(format!=2&&c==' '&&(i==begin||i==end-1))escape=true;
                if(escape)byte(&out,'\\');
                byte(&out,c);
            }
        }
    }
    free(value.p);
    if(format==2){java_security_Bytes norm={0};java_security_normalize(&norm,&out);free(out.p);out=norm;}
    return out;
}
static int java_security_text_cmp(const void *a,const void *b){
    const java_security_Bytes *x=a,*y=b;
    bool xkey=x->n&&isalpha(x->p[0]),ykey=y->n&&isalpha(y->p[0]);
    if(xkey!=ykey)return xkey?-1:1;
    return java_security_bytes_cmp(a,b);
}
static java_security_Bytes java_security_format_name(const java_security_X500 *p,int format) {
    java_security_Bytes out={0};
    for(size_t i=0;i<p->n;i++) {
        if(i)text(&out,(format==1||format==3)?", ":",");
        java_security_Rdn *r=&p->r[i];java_security_Bytes *values=calloc(r->n,sizeof(*values));
        if(!values){out.bad=true;break;}
        for(size_t j=0;j<r->n;j++){values[j]=java_security_format_ava(&r->a[j],format);if(values[j].bad)out.bad=true;}
        if(format==2&&!out.bad)qsort(values,r->n,sizeof(*values),java_security_text_cmp);
        for(size_t j=0;j<r->n;j++){if(j)text(&out,(format==1||format==3)?" + ":"+");java_security_append(&out,values[j].p,values[j].n);free(values[j].p);}
        free(values);
    }
    if(!out.p)text(&out,"");
    return out;
}
void ssrnx_java_security_x509_free_names(SsrnxJavaX509GeneralName *names, size_t count) {
    for (size_t i = 0; i < count; ++i) free(names[i].text);
    free(names);
}

SsrnxResult ssrnx_java_security_x509_general_names(const uint8_t *der, size_t size,
    bool subject, SsrnxJavaX509GeneralName **out_names, size_t *out_count) {
    if (!out_names || !out_count) return SSRNX_ERR_ARGUMENT;
    *out_names = NULL; *out_count = 0;
    SsrnxJavaX509Extension ext; bool found = false;
    SsrnxResult rc = ssrnx_java_security_x509_extension(der, size,
        subject ? "2.5.29.17" : "2.5.29.18", &ext, &found);
    if (rc != SSRNX_OK || !found) return rc;
    const unsigned char *p = ext.value, *end = p + ext.value_size, *seq = NULL;
    size_t seq_size = 0; unsigned char tag = 0;
    if (!java_security_read_tlv(&p, end, &tag, &seq, &seq_size) || tag != 0x30 || p != end)
        return SSRNX_ERR_APK_INVALID;
    p = seq; end = seq + seq_size;
    SsrnxJavaX509GeneralName *names = NULL; size_t count = 0;
    while (p < end) {
        const unsigned char *value = NULL, *span = NULL;
        size_t length = 0, span_size = 0;
        if (!java_security_read_tlv_span(&p, end, &tag, &value, &length, &span, &span_size)) goto invalid;
        const unsigned type = tag & 31u;
        const bool constructed = type == 0 || type == 3 || type == 4 || type == 5;
        if (type > 8 || tag != (unsigned char)(0x80u | (constructed ? 0x20u : 0u) | type)) goto invalid;
        char *name = NULL, text_buffer[96];
        if (type == 1 || type == 2 || type == 6) {
            /* Conscrypt prunes IA5 names containing NUL or non-ASCII bytes. */
            bool valid = true;
            for (size_t i = 0; i < length; ++i) if (!value[i] || value[i] > 127) valid = false;
            if (!valid) continue;
            name = malloc(length + 1u);
            if (name) { memcpy(name, value, length); name[length] = '\0'; }
        } else if (type == 7) {
            if (length != 4 && length != 16) continue;
            if (!inet_ntop(length == 4 ? AF_INET : AF_INET6, value, text_buffer, sizeof(text_buffer))) goto invalid;
            name = strdup(text_buffer);
        } else if (type == 8) {
            if (!java_security_x509_copy_oid(value, length, text_buffer, sizeof(text_buffer))) goto invalid;
            name = strdup(text_buffer);
        } else if (type == 4) {
            java_security_X500 *principal = calloc(1, sizeof(*principal));
            if (!principal) goto oom;
            if (!java_security_parse_der(principal, value, length)) { release(principal); goto invalid; }
            java_security_Bytes formatted = java_security_format_name(principal, 0); release(principal);
            if (formatted.bad) { free(formatted.p); goto oom; }
            name = (char *)formatted.p;
        }
        if ((type != 0 && type != 3 && type != 5) && !name) goto oom;
        if (count >= SIZE_MAX / sizeof(*names) - 1u) { free(name); goto oom; }
        SsrnxJavaX509GeneralName *next = realloc(names, (count + 1u) * sizeof(*names));
        if (!next) { free(name); goto oom; }
        names = next;
        names[count++] = (SsrnxJavaX509GeneralName){(int32_t)type, name, span, span_size};
    }
    *out_names = names; *out_count = count;
    return SSRNX_OK;
invalid:
    rc = SSRNX_ERR_APK_INVALID; goto failed;
oom:
    rc = SSRNX_ERR_NO_MEMORY;
failed:
    ssrnx_java_security_x509_free_names(names, count);
    return rc;
}

static void *java_security_java_string(void *env,const java_security_Bytes *b) {
    uint16_t *chars=calloc(b->n+1,sizeof(*chars));if(!chars)return NULL;
    const unsigned char *p=b->p,*end=p+b->n;size_t n=0;
    while(p<end){uint32_t cp=java_security_next_cp(&p);if(cp>0xffff){cp-=0x10000;chars[n++]=(uint16_t)(0xd800+(cp>>10));cp=0xdc00+(cp&1023);}chars[n++]=(uint16_t)cp;}
    void *r=n<=INT32_MAX?ssrnx_jni_new_local_string_utf16(env,chars,(int32_t)n):NULL;free(chars);return r;
}
static java_security_X500 *payload(void *token){java_security_X500 *p=ssrnx_jni_get_token_host_data(token);return p&&p->magic==X500_MAGIC?p:NULL;}
static void java_security_throw(void *env,const char *cls,const char *msg){(void)ssrnx_jni_throw_new_by_name(env,cls,msg);}
static SsrnxJniValue java_security_ctor(void *env,void *token,const SsrnxJniValue *a,size_t n,void *u) {
    SsrnxJniValue out={0};if(n!=1)return out;
    if(!a[0].l){java_security_throw(env,"java/lang/NullPointerException","name == null");return out;}
    java_security_X500 *p=calloc(1,sizeof(*p));if(!p){java_security_throw(env,"java/lang/OutOfMemoryError","X500Principal");return out;}
    p->magic=X500_MAGIC;bool ok=false;
    if(u) {
        int32_t len=ssrnx_jni_byte_array_length(a[0].l);unsigned char *bytes=len>0?malloc((size_t)len):NULL;
        if(bytes&&ssrnx_jni_byte_array_read(a[0].l,0,len,(int8_t*)bytes)==SSRNX_OK)ok=java_security_parse_der(p,bytes,(size_t)len);
        free(bytes);
    } else {
        int32_t len=0;uint16_t *chars=ssrnx_jni_string_utf16_dup_host(a[0].l,&len);java_security_Bytes input={0};
        bool valid=chars!=NULL;
        for(int32_t i=0;valid&&i<len;i++) {
            uint32_t cp=chars[i];
            if(cp>=0xd800&&cp<=0xdbff){if(i+1>=len||chars[i+1]<0xdc00||chars[i+1]>0xdfff){valid=false;break;}cp=0x10000+((cp-0xd800)<<10)+(chars[++i]-0xdc00);}
            else if(cp>=0xdc00&&cp<=0xdfff){valid=false;break;}
            /* A literal NUL in a Java DN value is legal. Encode as an escaped byte. */
            if(!cp)text(&input,"\\00");else java_security_utf8(&input,cp);
        }
        free(chars);if(!input.p)text(&input,"");
        if(valid&&!input.bad)ok=java_security_parse_text(p,(char*)input.p)&&java_security_encode_name(p);
        free(input.p);
    }
    if(!ok){release(p);java_security_throw(env,"java/lang/IllegalArgumentException","Invalid X.500 distinguished name");return out;}
    pthread_mutex_lock(&java_security_principal_lock);p->next=java_security_principals;java_security_principals=p;pthread_mutex_unlock(&java_security_principal_lock);
    (void)ssrnx_jni_set_token_host_data(token,p);return out;
}
static SsrnxJniValue name(void *env,void *token,const SsrnxJniValue *a,size_t n,void *u) {
    SsrnxJniValue out={0};java_security_X500 *p=payload(token);int format=u?(int)(intptr_t)u:0;
    if(n){char f[32];if(!a[0].l){java_security_throw(env,"java/lang/NullPointerException","format == null");return out;}
        if(!ssrnx_jni_string_utf_copy(a[0].l,f,sizeof(f))){java_security_throw(env,"java/lang/IllegalArgumentException","Invalid format");return out;}
        if(!strcasecmp(f,"RFC1779"))format=1;else if(!strcasecmp(f,"CANONICAL"))format=2;
        else if(strcasecmp(f,"RFC2253")){java_security_throw(env,"java/lang/IllegalArgumentException","Invalid format");return out;}}
    if(p){java_security_Bytes b=java_security_format_name(p,format);if(b.bad)java_security_throw(env,"java/lang/OutOfMemoryError","X500Principal name");else out.l=java_security_java_string(env,&b);free(b.p);}return out;
}
static SsrnxJniValue encoded(void *env,void *token,const SsrnxJniValue *a,size_t n,void *u) {
    (void)a;(void)n;(void)u;SsrnxJniValue out={0};java_security_X500 *p=payload(token);
    if(p&&p->der.n<=INT32_MAX){out.l=ssrnx_jni_new_local_byte_array(env,(int32_t)p->der.n);if(out.l)(void)ssrnx_jni_byte_array_write(out.l,0,(int32_t)p->der.n,(int8_t*)p->der.p);}return out;
}
static SsrnxJniValue equals(void *env,void *token,const SsrnxJniValue *a,size_t n,void *u) {
    (void)u;SsrnxJniValue out={0};java_security_X500 *p=payload(token),*q=n==1&&a[0].l?payload(ssrnx_jni_object_token_host(a[0].l)):NULL;
    if(p&&q){java_security_Bytes x=java_security_format_name(p,2),y=java_security_format_name(q,2);if(x.bad||y.bad)java_security_throw(env,"java/lang/OutOfMemoryError","X500Principal equality");else out.z=x.n==y.n&&!memcmp(x.p,y.p,x.n);free(x.p);free(y.p);}return out;
}
static SsrnxJniValue hash(void *env,void *token,const SsrnxJniValue *a,size_t n,void *u) {
    (void)a;(void)n;(void)u;SsrnxJniValue out={0};java_security_X500 *p=payload(token);
    if(p){java_security_Bytes x=java_security_format_name(p,2);if(x.bad)java_security_throw(env,"java/lang/OutOfMemoryError","X500Principal hash");else{
        const unsigned char *b=x.p,*end=b+x.n;uint32_t h=0;while(b<end){uint32_t cp=java_security_next_cp(&b);if(cp>0xffff){cp-=0x10000;h=31*h+(0xd800+(cp>>10));cp=0xdc00+(cp&1023);}h=31*h+cp;}out.i=(int32_t)h;}free(x.p);}return out;
}
void ssrnx_java_security_reset(void){pthread_mutex_lock(&java_security_principal_lock);java_security_X500 *p=java_security_principals;java_security_principals=NULL;pthread_mutex_unlock(&java_security_principal_lock);while(p){java_security_X500 *next=p->next;release(p);p=next;}}
SsrnxResult ssrnx_java_security_register(void) {
    SsrnxResult rc=ssrnx_jni_register_class("java/security/Principal","java/lang/Object");
    if(rc==SSRNX_OK)rc=ssrnx_jni_register_interface_method("java/security/Principal","getName","()Ljava/lang/String;");
    if(rc==SSRNX_OK)rc=ssrnx_jni_register_class(PRINCIPAL,"java/lang/Object");
    if(rc==SSRNX_OK)rc=ssrnx_jni_register_interface(PRINCIPAL,"java/security/Principal");
    if(rc==SSRNX_OK)rc=ssrnx_jni_register_class("java/io/Serializable","java/lang/Object");
    if(rc==SSRNX_OK)rc=ssrnx_jni_register_interface(PRINCIPAL,"java/io/Serializable");
#define METHOD(m,s,cb,u) do{if(rc==SSRNX_OK)rc=ssrnx_jni_register_method(PRINCIPAL,m,s,false,cb,u);}while(0)
    METHOD("<init>","(Ljava/lang/String;)V",java_security_ctor,NULL);METHOD("<init>","([B)V",java_security_ctor,(void*)1);
    METHOD("getName","()Ljava/lang/String;",name,NULL);METHOD("getName","(Ljava/lang/String;)Ljava/lang/String;",name,NULL);
    METHOD("toString","()Ljava/lang/String;",name,(void*)3);METHOD("getEncoded","()[B",encoded,NULL);
    METHOD("equals","(Ljava/lang/Object;)Z",equals,NULL);METHOD("hashCode","()I",hash,NULL);
#undef METHOD
    const char *formats[]={"RFC1779","RFC2253","CANONICAL"};
    for(size_t i=0;rc==SSRNX_OK&&i<3;i++) {
        rc=ssrnx_jni_register_field(PRINCIPAL,formats[i],"Ljava/lang/String;",true,(SsrnxJniValue){0});
        void *s=rc==SSRNX_OK?ssrnx_jni_new_local_string_utf(ssrnx_jni_current_env(),formats[i]):NULL;
        SsrnxJniValue value={.l=s};if(s)rc=ssrnx_jni_set_static_field_value(PRINCIPAL,formats[i],"Ljava/lang/String;",value);
        if(s)ssrnx_jni_delete_local_ref_host(ssrnx_jni_current_env(),s);
    }
    return rc;
}

#pragma pop_macro("X500_MAGIC")
#pragma pop_macro("PRINCIPAL")
#pragma pop_macro("METHOD")


/* ---- java_crypto ---- */
#pragma push_macro("AES_BLOCK")
#pragma push_macro("CIPHER")
#pragma push_macro("CIPHER_MAGIC")
#pragma push_macro("HMAC_MAGIC")
#pragma push_macro("HMAC_SPI")
#pragma push_macro("IV_MAGIC")
#pragma push_macro("IV_SPEC")
#pragma push_macro("KEY_MAGIC")
#pragma push_macro("SECRET_KEY_SPEC")
/* javax.crypto Cipher/SecretKeySpec/IvParameterSpec and the streaming mbedTLS
 * HMAC SPI used by the AOSP Mac engine. Cipher uses a clean-room AES
 * (FIPS-197) for the transformations Android's default provider exposes for
 * AES: ECB and CBC with PKCS5Padding (alias PKCS7Padding) or NoPadding, and
 * CTR with NoPadding. "AES" alone means AES/ECB/PKCS5Padding (JCA default). */
#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/platform_util.h>

#ifdef __SWITCH__
#include <switch.h>
#endif



#define CIPHER "javax/crypto/Cipher"
#define SECRET_KEY_SPEC "javax/crypto/spec/SecretKeySpec"
#define IV_SPEC "javax/crypto/spec/IvParameterSpec"
#define KEY_MAGIC UINT64_C(0x41504b364145534b)    /* APK6AESK */
#define IV_MAGIC UINT64_C(0x41504b3641455349)     /* APK6AESI */
#define CIPHER_MAGIC UINT64_C(0x41504b3641455343) /* APK6AESC */
#define AES_BLOCK 16
#define HMAC_SPI "com/apn64/security/Apn64Hmac"
#define HMAC_MAGIC UINT64_C(0x41504b36484d4143) /* APK6HMAC */

/* ---------------- AES (FIPS-197) ---------------- */

static const uint8_t java_crypto_k_sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16,
};
static uint8_t java_crypto_k_inv_sbox[256];
static pthread_once_t java_crypto_k_inv_once = PTHREAD_ONCE_INIT;
static void java_crypto_inv_sbox_init(void) { for (int i = 0; i < 256; ++i) java_crypto_k_inv_sbox[java_crypto_k_sbox[i]] = (uint8_t)i; }

typedef struct java_crypto_Aes {
    int rounds;
    uint8_t rk[16 * 15];
} java_crypto_Aes;

static uint8_t java_crypto_xtime(uint8_t x) { return (uint8_t)((x << 1) ^ ((x & 0x80) ? 0x1b : 0)); }
static uint8_t java_crypto_gmul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    while (b) { if (b & 1) p ^= a; a = java_crypto_xtime(a); b >>= 1; }
    return p;
}

static bool java_crypto_aes_init(java_crypto_Aes *aes, const uint8_t *key, size_t key_len) {
    if (key_len != 16 && key_len != 24 && key_len != 32) return false;
    pthread_once(&java_crypto_k_inv_once, java_crypto_inv_sbox_init);
    const int nk = (int)key_len / 4;
    aes->rounds = nk + 6;
    const int total = 4 * (aes->rounds + 1);
    memcpy(aes->rk, key, key_len);
    uint8_t rcon = 1;
    for (int i = nk; i < total; ++i) {
        uint8_t t[4];
        memcpy(t, aes->rk + 4 * (i - 1), 4);
        if (i % nk == 0) {
            const uint8_t first = t[0];
            t[0] = (uint8_t)(java_crypto_k_sbox[t[1]] ^ rcon); t[1] = java_crypto_k_sbox[t[2]];
            t[2] = java_crypto_k_sbox[t[3]]; t[3] = java_crypto_k_sbox[first];
            rcon = java_crypto_xtime(rcon);
        } else if (nk > 6 && i % nk == 4) {
            for (int j = 0; j < 4; ++j) t[j] = java_crypto_k_sbox[t[j]];
        }
        for (int j = 0; j < 4; ++j) aes->rk[4 * i + j] = (uint8_t)(aes->rk[4 * (i - nk) + j] ^ t[j]);
    }
    return true;
}

static void java_crypto_add_round_key(uint8_t *s, const uint8_t *rk) { for (int i = 0; i < 16; ++i) s[i] ^= rk[i]; }

/* State is column-major: s[4*c + r]. */
static void java_crypto_shift_rows(uint8_t *s, bool inverse) {
    uint8_t t[16];
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            t[4 * c + r] = inverse ? s[4 * ((c - r + 4) % 4) + r] : s[4 * ((c + r) % 4) + r];
    memcpy(s, t, 16);
}

static void java_crypto_mix_columns(uint8_t *s, bool inverse) {
    for (int c = 0; c < 4; ++c) {
        uint8_t *col = s + 4 * c;
        uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
        if (inverse) {
            /* Inverse mixing is forward mixing after multiplying by M^2. */
            const uint8_t u = java_crypto_xtime(java_crypto_xtime(a0 ^ a2)), v = java_crypto_xtime(java_crypto_xtime(a1 ^ a3));
            a0 ^= u; a2 ^= u; a1 ^= v; a3 ^= v;
        }
        col[0] = (uint8_t)(java_crypto_gmul(a0, 2) ^ java_crypto_gmul(a1, 3) ^ a2 ^ a3);
        col[1] = (uint8_t)(a0 ^ java_crypto_gmul(a1, 2) ^ java_crypto_gmul(a2, 3) ^ a3);
        col[2] = (uint8_t)(a0 ^ a1 ^ java_crypto_gmul(a2, 2) ^ java_crypto_gmul(a3, 3));
        col[3] = (uint8_t)(java_crypto_gmul(a0, 3) ^ a1 ^ a2 ^ java_crypto_gmul(a3, 2));
    }
}

static void java_crypto_aes_encrypt_block(const java_crypto_Aes *aes, uint8_t *s) {
    java_crypto_add_round_key(s, aes->rk);
    for (int round = 1; round <= aes->rounds; ++round) {
        for (int i = 0; i < 16; ++i) s[i] = java_crypto_k_sbox[s[i]];
        java_crypto_shift_rows(s, false);
        if (round != aes->rounds) java_crypto_mix_columns(s, false);
        java_crypto_add_round_key(s, aes->rk + 16 * round);
    }
}

static void java_crypto_aes_decrypt_block(const java_crypto_Aes *aes, uint8_t *s) {
    java_crypto_add_round_key(s, aes->rk + 16 * aes->rounds);
    for (int round = aes->rounds - 1; round >= 0; --round) {
        java_crypto_shift_rows(s, true);
        for (int i = 0; i < 16; ++i) s[i] = java_crypto_k_inv_sbox[s[i]];
        java_crypto_add_round_key(s, aes->rk + 16 * round);
        if (round) java_crypto_mix_columns(s, true);
    }
}

/* ---------------- Java object payloads ---------------- */

typedef struct java_crypto_KeyPayload {
    uint64_t magic;
    uint8_t *bytes;
    int32_t length;
    char algorithm[32];
} java_crypto_KeyPayload;

typedef struct java_crypto_IvPayload {
    uint64_t magic;
    uint8_t *bytes;
    int32_t length;
} java_crypto_IvPayload;

typedef enum { MODE_ECB, MODE_CBC, MODE_CTR } java_crypto_BlockMode;

typedef struct java_crypto_CipherPayload {
    uint64_t magic;
    char transformation[64];
    java_crypto_BlockMode mode;
    bool padding;
    int opmode; /* 0 until init(): 1 = ENCRYPT_MODE, 2 = DECRYPT_MODE */
    java_crypto_Aes aes;
    uint8_t iv[AES_BLOCK];
    bool has_iv;
    uint8_t *pending;
    size_t pending_len, pending_cap;
} java_crypto_CipherPayload;

static SsrnxJniValue java_crypto_v_void(void) { SsrnxJniValue v = {0}; return v; }
static SsrnxJniValue java_crypto_v_int(int32_t i) { SsrnxJniValue v = {0}; v.i = i; return v; }
static SsrnxJniValue java_crypto_v_object(void *l) { SsrnxJniValue v = {0}; v.l = l; return v; }

static void *java_crypto_payload_of(void *token, uint64_t magic) {
    const uint64_t *p = token ? ssrnx_jni_get_token_host_data(token) : NULL;
    return p && *p == magic ? (void *)p : NULL;
}

static void *java_crypto_payload_of_ref(void *ref, uint64_t magic) {
    const uint64_t *p = ref ? ssrnx_jni_get_object_host_data(ref) : NULL;
    return p && *p == magic ? (void *)p : NULL;
}

static void java_crypto_throw_name(void *env, const char *cls, const char *message) {
    (void)ssrnx_jni_throw_new_by_name(env, cls, message);
}

/* Copy byte[] [offset, offset+len) into a malloc buffer (len 0 allowed). */
static bool java_crypto_read_bytes(void *env, void *array, int32_t offset, int32_t len, uint8_t **out) {
    *out = NULL;
    if (!array) { java_crypto_throw_name(env, "java/lang/IllegalArgumentException", "Null input buffer"); return false; }
    const int32_t total = ssrnx_jni_byte_array_length(array);
    if (offset < 0 || len < 0 || offset > total || len > total - offset) {
        java_crypto_throw_name(env, "java/lang/IllegalArgumentException", "Bad arguments");
        return false;
    }
    *out = malloc((size_t)len + 1u);
    if (!*out) { java_crypto_throw_name(env, "java/lang/OutOfMemoryError", "byte[]"); return false; }
    if (len && ssrnx_jni_byte_array_read(array, offset, len, (int8_t *)*out) != SSRNX_OK) {
        free(*out); *out = NULL;
        java_crypto_throw_name(env, "java/lang/IllegalArgumentException", "Bad arguments");
        return false;
    }
    return true;
}

static void *java_crypto_new_byte_array(void *env, const uint8_t *bytes, size_t len) {
    if (len > INT32_MAX) { java_crypto_throw_name(env, "java/lang/OutOfMemoryError", "byte[]"); return NULL; }
    void *array = ssrnx_jni_new_local_byte_array(env, (int32_t)len);
    if (array && len) (void)ssrnx_jni_byte_array_write(array, 0, (int32_t)len, (const int8_t *)bytes);
    return array;
}

/* ---------------- PBKDF2 provider native boundary ---------------- */
static SsrnxJniValue java_crypto_pbkdf2_derive(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)t; (void)u;
    if (n != 5 || !a[0].l || !a[1].l || !a[4].l || a[2].i <= 0 || a[3].i <= 0) {
        java_crypto_throw_name(env, "java/security/spec/InvalidKeySpecException", "Invalid PBKDF2 parameters");
        return java_crypto_v_object(NULL);
    }
    char digest[16] = {0};
    (void)ssrnx_jni_string_utf_copy(a[4].l, digest, sizeof(digest));
    const mbedtls_md_info_t *info = mbedtls_md_info_from_string(digest);
    if (!info) {
        java_crypto_throw_name(env, "java/security/NoSuchAlgorithmException", digest);
        return java_crypto_v_object(NULL);
    }
    const int32_t pw_len = ssrnx_jni_byte_array_length(a[0].l);
    const int32_t salt_len = ssrnx_jni_byte_array_length(a[1].l);
    uint8_t *password = NULL, *salt = NULL;
    if (!java_crypto_read_bytes(env, a[0].l, 0, pw_len, &password)) return java_crypto_v_object(NULL);
    if (!java_crypto_read_bytes(env, a[1].l, 0, salt_len, &salt)) {
        mbedtls_platform_zeroize(password, (size_t)pw_len); free(password);
        return java_crypto_v_object(NULL);
    }
    /* Android's PKCS5S2 generator truncates bits to whole bytes. */
    const size_t length = (size_t)a[3].i / 8u;
    uint8_t *bytes = malloc(length ? length : 1u);
    mbedtls_md_context_t md;
    mbedtls_md_init(&md);
    int rc = bytes ? mbedtls_md_setup(&md, info, 1) : 0;
    if (bytes && rc == 0)
        rc = mbedtls_pkcs5_pbkdf2_hmac(&md, password, (size_t)pw_len, salt, (size_t)salt_len,
                                      (unsigned int)a[2].i, (uint32_t)length, bytes);
    void *result = NULL;
    if (!bytes) java_crypto_throw_name(env, "java/lang/OutOfMemoryError", "PBKDF2 key");
    else if (rc) java_crypto_throw_name(env, "java/security/ProviderException", "PBKDF2 derivation failed");
    else result = java_crypto_new_byte_array(env, bytes, length);
    mbedtls_md_free(&md);
    mbedtls_platform_zeroize(password, (size_t)pw_len); free(password);
    mbedtls_platform_zeroize(salt, (size_t)salt_len); free(salt);
    if (bytes) { mbedtls_platform_zeroize(bytes, length); free(bytes); }
    return java_crypto_v_object(result);
}

void *ssrnx_java_pbkdf2_derive(void *env, void *self, void *password, void *salt,
                              int32_t iterations, int32_t bits, void *digest) {
    const SsrnxJniValue a[] = {{.l = password}, {.l = salt}, {.i = iterations}, {.i = bits}, {.l = digest}};
    return java_crypto_pbkdf2_derive(env, self, a, 5, NULL).l;
}

/* ---------------- SecretKeySpec / IvParameterSpec ---------------- */

static SsrnxJniValue java_crypto_key_ctor_common(void *env, void *t, void *array, int32_t offset, int32_t len,
                                     void *algorithm, bool ranged) {
    if (!array || !algorithm) { java_crypto_throw_name(env, "java/lang/IllegalArgumentException", "Missing argument"); return java_crypto_v_void(); }
    const int32_t total = ssrnx_jni_byte_array_length(array);
    if (!ranged) { offset = 0; len = total; }
    if (total == 0 || (ranged && len == 0)) {
        java_crypto_throw_name(env, "java/lang/IllegalArgumentException", "Empty key");
        return java_crypto_v_void();
    }
    if (ranged && (offset < 0 || len < 0 || offset > total || len > total - offset)) {
        java_crypto_throw_name(env, "java/lang/ArrayIndexOutOfBoundsException", "Invalid offset/length combination");
        return java_crypto_v_void();
    }
    java_crypto_KeyPayload *p = calloc(1, sizeof(*p));
    if (!p || !java_crypto_read_bytes(env, array, offset, len, &p->bytes)) { free(p); return java_crypto_v_void(); }
    p->magic = KEY_MAGIC;
    p->length = len;
    (void)ssrnx_jni_string_utf_copy(algorithm, p->algorithm, sizeof(p->algorithm));
    (void)ssrnx_jni_set_token_host_data(t, p);
    return java_crypto_v_void();
}

static SsrnxJniValue java_crypto_key_ctor(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    return n == 2 ? java_crypto_key_ctor_common(env, t, a[0].l, 0, 0, a[1].l, false) : java_crypto_v_void();
}
static SsrnxJniValue java_crypto_key_ctor_range(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    return n == 4 ? java_crypto_key_ctor_common(env, t, a[0].l, a[1].i, a[2].i, a[3].l, true) : java_crypto_v_void();
}
static SsrnxJniValue java_crypto_key_get_algorithm(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)n; (void)u;
    java_crypto_KeyPayload *p = java_crypto_payload_of(t, KEY_MAGIC);
    return java_crypto_v_object(p ? ssrnx_jni_new_local_string_utf(env, p->algorithm) : NULL);
}
static SsrnxJniValue java_crypto_key_get_format(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)t; (void)a; (void)n; (void)u;
    return java_crypto_v_object(ssrnx_jni_new_local_string_utf(env, "RAW"));
}
static SsrnxJniValue java_crypto_key_get_encoded(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)n; (void)u;
    java_crypto_KeyPayload *p = java_crypto_payload_of(t, KEY_MAGIC);
    return java_crypto_v_object(p ? java_crypto_new_byte_array(env, p->bytes, (size_t)p->length) : NULL);
}
static SsrnxJniValue java_crypto_key_hash(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)env; (void)a; (void)n; (void)u;
    java_crypto_KeyPayload *p = java_crypto_payload_of(t, KEY_MAGIC);
    int32_t h = 0;
    for (int32_t i = 0; p && i < p->length; ++i) h += (int8_t)p->bytes[i] * (i + 1);
    if (p) for (const char *s = p->algorithm; *s; ++s) h ^= tolower((unsigned char)*s);
    return java_crypto_v_int(h);
}
static SsrnxJniValue java_crypto_key_equals(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)env; (void)u;
    java_crypto_KeyPayload *p = java_crypto_payload_of(t, KEY_MAGIC);
    java_crypto_KeyPayload *q = n == 1 ? java_crypto_payload_of_ref(a[0].l, KEY_MAGIC) : NULL;
    SsrnxJniValue v = {0};
    v.z = p && q && p->length == q->length && strcasecmp(p->algorithm, q->algorithm) == 0 &&
          memcmp(p->bytes, q->bytes, (size_t)p->length) == 0;
    return v;
}

static SsrnxJniValue java_crypto_iv_ctor_common(void *env, void *t, void *array, int32_t offset, int32_t len, bool ranged) {
    if (!array) { java_crypto_throw_name(env, "java/lang/NullPointerException", "IV missing"); return java_crypto_v_void(); }
    const int32_t total = ssrnx_jni_byte_array_length(array);
    if (!ranged) { offset = 0; len = total; }
    if (ranged && (offset < 0 || len < 0 || offset > total || len > total - offset)) {
        java_crypto_throw_name(env, "java/lang/ArrayIndexOutOfBoundsException", "Invalid offset/length combination");
        return java_crypto_v_void();
    }
    java_crypto_IvPayload *p = calloc(1, sizeof(*p));
    if (!p || !java_crypto_read_bytes(env, array, offset, len, &p->bytes)) { free(p); return java_crypto_v_void(); }
    p->magic = IV_MAGIC;
    p->length = len;
    (void)ssrnx_jni_set_token_host_data(t, p);
    return java_crypto_v_void();
}
static SsrnxJniValue java_crypto_iv_ctor(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    return n == 1 ? java_crypto_iv_ctor_common(env, t, a[0].l, 0, 0, false) : java_crypto_v_void();
}
static SsrnxJniValue java_crypto_iv_ctor_range(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    return n == 3 ? java_crypto_iv_ctor_common(env, t, a[0].l, a[1].i, a[2].i, true) : java_crypto_v_void();
}
static SsrnxJniValue java_crypto_iv_get(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)n; (void)u;
    java_crypto_IvPayload *p = java_crypto_payload_of(t, IV_MAGIC);
    return java_crypto_v_object(p ? java_crypto_new_byte_array(env, p->bytes, (size_t)p->length) : NULL);
}

/* ---------------- Cipher ---------------- */

static bool java_crypto_parse_transformation(const char *text, java_crypto_BlockMode *mode, bool *padding, bool *bad_padding) {
    char parts[3][32] = {{0}};
    int count = 0;
    const char *p = text;
    for (;;) {
        const char *slash = strchr(p, '/');
        const size_t len = slash ? (size_t)(slash - p) : strlen(p);
        if (count == 3 || len >= sizeof(parts[0])) return false;
        memcpy(parts[count++], p, len);
        if (!slash) break;
        p = slash + 1;
    }
    if (strcasecmp(parts[0], "AES") != 0) return false;
    *bad_padding = false;
    if (count == 1) { *mode = MODE_ECB; *padding = true; return true; }
    if (count != 3) return false;
    if (strcasecmp(parts[1], "ECB") == 0) *mode = MODE_ECB;
    else if (strcasecmp(parts[1], "CBC") == 0) *mode = MODE_CBC;
    else if (strcasecmp(parts[1], "CTR") == 0) *mode = MODE_CTR;
    else return false;
    if (strcasecmp(parts[2], "PKCS5Padding") == 0 || strcasecmp(parts[2], "PKCS7Padding") == 0) *padding = true;
    else if (strcasecmp(parts[2], "NoPadding") == 0) *padding = false;
    else { *bad_padding = true; return false; }
    if (*mode == MODE_CTR && *padding) { *bad_padding = true; return false; }
    return true;
}

static SsrnxJniValue java_crypto_cipher_get_instance(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)t; (void)u;
    if (n < 1) return java_crypto_v_object(NULL);
    char transformation[64] = {0};
    if (!a[0].l || !ssrnx_jni_string_utf_copy(a[0].l, transformation, sizeof(transformation)) ||
        !transformation[0]) {
        java_crypto_throw_name(env, "java/security/NoSuchAlgorithmException", "Invalid transformation format");
        return java_crypto_v_object(NULL);
    }
    java_crypto_BlockMode mode = MODE_ECB;
    bool padding = true, bad_padding = false;
    if (!java_crypto_parse_transformation(transformation, &mode, &padding, &bad_padding)) {
        char message[128];
        snprintf(message, sizeof(message), "No provider found for %s", transformation);
        java_crypto_throw_name(env, bad_padding ? "javax/crypto/NoSuchPaddingException"
                                    : "java/security/NoSuchAlgorithmException", message);
        return java_crypto_v_object(NULL);
    }
    void *obj = ssrnx_jni_new_local_object(env, CIPHER);
    java_crypto_CipherPayload *p = obj ? calloc(1, sizeof(*p)) : NULL;
    if (!p) {
        if (obj) ssrnx_jni_delete_local_ref_host(env, obj);
        java_crypto_throw_name(env, "java/lang/OutOfMemoryError", "Cipher");
        return java_crypto_v_object(NULL);
    }
    p->magic = CIPHER_MAGIC;
    memcpy(p->transformation, transformation, sizeof(p->transformation));
    p->mode = mode;
    p->padding = padding;
    (void)ssrnx_jni_set_object_host_data(obj, p);
    return java_crypto_v_object(obj);
}

static void java_crypto_random_bytes(uint8_t *out, size_t len) {
#ifdef __SWITCH__
    randomGet(out, len);
#else
    FILE *f = fopen("/dev/urandom", "rb");
    const size_t got = f ? fread(out, 1, len, f) : 0;
    if (f) fclose(f);
    for (size_t i = got; i < len; ++i) out[i] = (uint8_t)rand();
#endif
}

/* Raw key bytes: SecretKeySpec payload, else Key.getEncoded() on the object. */
static bool java_crypto_key_bytes(void *env, void *key, uint8_t **out, int32_t *out_len) {
    *out = NULL; *out_len = 0;
    java_crypto_KeyPayload *p = java_crypto_payload_of_ref(key, KEY_MAGIC);
    if (p) {
        if (!(*out = malloc((size_t)p->length))) return false;
        memcpy(*out, p->bytes, (size_t)p->length);
        *out_len = p->length;
        return true;
    }
    char cls[256] = {0};
    SsrnxJniValue encoded = {0};
    bool found = false;
    if (!ssrnx_jni_object_class_name_host(key, cls, sizeof(cls)) ||
        ssrnx_jni_invoke_method_host(env, key, cls, "getEncoded", "()[B", false, NULL, &encoded, &found) != SSRNX_OK ||
        !found || !encoded.l || ssrnx_jni_exception_pending_host(env))
        return false;
    *out_len = ssrnx_jni_byte_array_length(encoded.l);
    const bool ok = java_crypto_read_bytes(env, encoded.l, 0, *out_len, out);
    ssrnx_jni_delete_local_ref_host(env, encoded.l);
    return ok;
}

/* Only the provider's primitive SPI is native. The pinned AOSP Mac/MacSpi
 * overlay owns JCA provider dispatch, public validation and buffer overloads.
 * Contexts stream without retaining the message and live until runtime reset. */
typedef struct java_crypto_HmacPayload {
    uint64_t magic;
    mbedtls_md_context_t md;
    uint8_t *key;
    int32_t key_len;
    struct java_crypto_HmacPayload *next;
} java_crypto_HmacPayload;
static java_crypto_HmacPayload *java_crypto_k_hmac_states;
static pthread_mutex_t java_crypto_k_hmac_lock = PTHREAD_MUTEX_INITIALIZER;

static void java_crypto_hmac_free(java_crypto_HmacPayload *p) {
    mbedtls_md_free(&p->md);
    if (p->key) { mbedtls_platform_zeroize(p->key, (size_t)p->key_len); free(p->key); }
    free(p);
}

static java_crypto_HmacPayload *java_crypto_hmac_new(void *env, const mbedtls_md_info_t *info) {
    java_crypto_HmacPayload *p = calloc(1, sizeof(*p));
    if (p) {
        p->magic = HMAC_MAGIC;
        mbedtls_md_init(&p->md);
        if (mbedtls_md_setup(&p->md, info, 1) == 0) return p;
        java_crypto_hmac_free(p);
    }
    java_crypto_throw_name(env, "java/lang/OutOfMemoryError", "HMAC context");
    return NULL;
}

static void java_crypto_hmac_attach(void *ref, java_crypto_HmacPayload *p) {
    (void)ssrnx_jni_set_object_host_data(ref, p);
    pthread_mutex_lock(&java_crypto_k_hmac_lock);
    p->next = java_crypto_k_hmac_states;
    java_crypto_k_hmac_states = p;
    pthread_mutex_unlock(&java_crypto_k_hmac_lock);
}

static SsrnxJniValue java_crypto_hmac_create(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)n; (void)u;
    char digest[24] = {0};
    (void)ssrnx_jni_string_utf_copy(a[0].l, digest, sizeof(digest));
    const mbedtls_md_info_t *info = mbedtls_md_info_from_string(digest);
    if (!info) { java_crypto_throw_name(env, "java/security/NoSuchAlgorithmException", digest); return java_crypto_v_void(); }
    java_crypto_HmacPayload *p = java_crypto_hmac_new(env, info);
    if (p) {
        void *self = ssrnx_jni_new_local_from_token(env, token);
        java_crypto_hmac_attach(self, p);
        ssrnx_jni_delete_local_ref_host(env, self);
    }
    return java_crypto_v_void();
}

static SsrnxJniValue java_crypto_hmac_length(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)env; (void)a; (void)n; (void)u;
    java_crypto_HmacPayload *p = java_crypto_payload_of(token, HMAC_MAGIC);
    return java_crypto_v_int(p ? mbedtls_md_get_size(p->md.md_info) : 0);
}

static SsrnxJniValue java_crypto_hmac_init(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)n; (void)u;
    java_crypto_HmacPayload *p = java_crypto_payload_of(token, HMAC_MAGIC);
    if (!p) return java_crypto_v_void();
    /* Conscrypt accepts encoded SecretKeys (including empty custom keys),
     * rejects every non-null AlgorithmParameterSpec, and ignores key algorithm. */
    if (!a[0].l || !ssrnx_jni_is_instance_of_class_host(a[0].l, "javax/crypto/SecretKey")) {
        java_crypto_throw_name(env, "java/security/InvalidKeyException", "key must be a SecretKey"); return java_crypto_v_void();
    }
    if (a[1].l) {
        java_crypto_throw_name(env, "java/security/InvalidAlgorithmParameterException", "unknown parameter type"); return java_crypto_v_void();
    }
    uint8_t *key = NULL;
    int32_t len = 0;
    if (!java_crypto_key_bytes(env, a[0].l, &key, &len)) {
        if (!ssrnx_jni_exception_pending_host(env)) java_crypto_throw_name(env, "java/security/InvalidKeyException", "key cannot be encoded");
        return java_crypto_v_void();
    }
    mbedtls_md_context_t next;
    mbedtls_md_init(&next);
    const int setup = mbedtls_md_setup(&next, p->md.md_info, 1);
    const int start = setup ? setup : mbedtls_md_hmac_starts(&next, key, (size_t)len);
    if (start) {
        mbedtls_md_free(&next); mbedtls_platform_zeroize(key, (size_t)len); free(key);
        java_crypto_throw_name(env, "java/lang/OutOfMemoryError", "HMAC context"); return java_crypto_v_void();
    }
    mbedtls_md_free(&p->md);
    if (p->key) { mbedtls_platform_zeroize(p->key, (size_t)p->key_len); free(p->key); }
    p->md = next; p->key = key; p->key_len = len;
    return java_crypto_v_void();
}

static SsrnxJniValue java_crypto_hmac_update(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    java_crypto_HmacPayload *p = java_crypto_payload_of(token, HMAC_MAGIC);
    if (!p || !p->key) { java_crypto_throw_name(env, "java/lang/IllegalStateException", "MAC not initialized"); return java_crypto_v_void(); }
    int result;
    if (n == 1) {
        const uint8_t byte = (uint8_t)a[0].b;
        result = mbedtls_md_hmac_update(&p->md, &byte, 1);
    } else {
        const int32_t total = ssrnx_jni_byte_array_length(a[0].l), off = a[1].i, len = a[2].i;
        if (!a[0].l || off < 0 || len < 0 || off > total - len) {
            java_crypto_throw_name(env, "java/lang/IllegalArgumentException", "Bad arguments"); return java_crypto_v_void();
        }
        const uint8_t *bytes = ssrnx_jni_primitive_array_data_host(a[0].l);
        result = mbedtls_md_hmac_update(&p->md, bytes + off, (size_t)len);
    }
    if (result) java_crypto_throw_name(env, "java/security/ProviderException", "HMAC update failed");
    return java_crypto_v_void();
}

static SsrnxJniValue java_crypto_hmac_final(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)n; (void)u;
    java_crypto_HmacPayload *p = java_crypto_payload_of(token, HMAC_MAGIC);
    if (!p || !p->key) { java_crypto_throw_name(env, "java/lang/IllegalStateException", "MAC not initialized"); return java_crypto_v_object(NULL); }
    uint8_t result[MBEDTLS_MD_MAX_SIZE];
    if (mbedtls_md_hmac_finish(&p->md, result)) {
        java_crypto_throw_name(env, "java/security/ProviderException", "HMAC final failed"); return java_crypto_v_object(NULL);
    }
    return java_crypto_v_object(java_crypto_new_byte_array(env, result, mbedtls_md_get_size(p->md.md_info)));
}

static SsrnxJniValue java_crypto_hmac_reset(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)n; (void)u;
    java_crypto_HmacPayload *p = java_crypto_payload_of(token, HMAC_MAGIC);
    if (p && p->key && mbedtls_md_hmac_reset(&p->md))
        java_crypto_throw_name(env, "java/security/ProviderException", "HMAC reset failed");
    return java_crypto_v_void();
}

static SsrnxJniValue java_crypto_hmac_clone(void *env, void *token, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)n; (void)u;
    java_crypto_HmacPayload *p = java_crypto_payload_of(token, HMAC_MAGIC);
    if (!p) return java_crypto_v_object(NULL);
    java_crypto_HmacPayload *copy = java_crypto_hmac_new(env, p->md.md_info);
    if (!copy) return java_crypto_v_object(NULL);
    if (p->key) {
        copy->key = malloc((size_t)p->key_len + 1);
        if (!copy->key) { java_crypto_hmac_free(copy); java_crypto_throw_name(env, "java/lang/OutOfMemoryError", "HMAC key"); return java_crypto_v_object(NULL); }
        copy->key_len = p->key_len;
        memcpy(copy->key, p->key, (size_t)p->key_len);
        /* md_clone copies the streaming digest only. Initialize separate HMAC
         * pads first so clone final/reset never aliases the original context. */
        if (mbedtls_md_hmac_starts(&copy->md, copy->key, (size_t)copy->key_len) ||
            mbedtls_md_clone(&copy->md, &p->md)) {
            java_crypto_hmac_free(copy); java_crypto_throw_name(env, "java/security/ProviderException", "HMAC clone failed"); return java_crypto_v_object(NULL);
        }
    }
    char cls[256] = {0};
    void *self = ssrnx_jni_new_local_from_token(env, token);
    (void)ssrnx_jni_object_class_name_host(self, cls, sizeof(cls));
    ssrnx_jni_delete_local_ref_host(env, self);
    void *obj = ssrnx_jni_new_local_object(env, cls);
    if (!obj) { java_crypto_hmac_free(copy); return java_crypto_v_object(NULL); }
    java_crypto_hmac_attach(obj, copy);
    return java_crypto_v_object(obj);
}

/* Actual JNI boot-native entry points for the overlay's native declarations. */
void ssrnx_java_hmac_create(void *env, void *self, void *digest) {
    const SsrnxJniValue a[] = {{.l = digest}};
    (void)java_crypto_hmac_create(env, ssrnx_jni_object_token_host(self), a, 1, NULL);
}
int32_t ssrnx_java_hmac_length(void *env, void *self) {
    return java_crypto_hmac_length(env, ssrnx_jni_object_token_host(self), NULL, 0, NULL).i;
}
void ssrnx_java_hmac_init(void *env, void *self, void *key, void *params) {
    const SsrnxJniValue a[] = {{.l = key}, {.l = params}};
    (void)java_crypto_hmac_init(env, ssrnx_jni_object_token_host(self), a, 2, NULL);
}
void ssrnx_java_hmac_update_byte(void *env, void *self, int8_t byte) {
    const SsrnxJniValue a[] = {{.b = byte}};
    (void)java_crypto_hmac_update(env, ssrnx_jni_object_token_host(self), a, 1, NULL);
}
void ssrnx_java_hmac_update_array(void *env, void *self, void *array, int32_t offset, int32_t length) {
    const SsrnxJniValue a[] = {{.l = array}, {.i = offset}, {.i = length}};
    (void)java_crypto_hmac_update(env, ssrnx_jni_object_token_host(self), a, 3, NULL);
}
void *ssrnx_java_hmac_final(void *env, void *self) {
    return java_crypto_hmac_final(env, ssrnx_jni_object_token_host(self), NULL, 0, NULL).l;
}
void ssrnx_java_hmac_reset(void *env, void *self) {
    (void)java_crypto_hmac_reset(env, ssrnx_jni_object_token_host(self), NULL, 0, NULL);
}
void *ssrnx_java_hmac_clone(void *env, void *self) {
    return java_crypto_hmac_clone(env, ssrnx_jni_object_token_host(self), NULL, 0, NULL).l;
}

static SsrnxJniValue java_crypto_cipher_init(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    java_crypto_CipherPayload *p = java_crypto_payload_of(t, CIPHER_MAGIC);
    if (!p || n < 2) return java_crypto_v_void();
    const int opmode = a[0].i;
    if (opmode != 1 && opmode != 2) {
        java_crypto_throw_name(env, "java/security/InvalidParameterException", "Unsupported opmode");
        return java_crypto_v_void();
    }
    if (!a[1].l) { java_crypto_throw_name(env, "java/security/InvalidKeyException", "key == null"); return java_crypto_v_void(); }
    uint8_t *key = NULL;
    int32_t key_len = 0;
    if (!java_crypto_key_bytes(env, a[1].l, &key, &key_len)) {
        if (!ssrnx_jni_exception_pending_host(env))
            java_crypto_throw_name(env, "java/security/InvalidKeyException", "Unsupported key");
        return java_crypto_v_void();
    }
    java_crypto_Aes aes;
    const bool key_ok = java_crypto_aes_init(&aes, key, (size_t)key_len);
    free(key);
    if (!key_ok) {
        char message[64];
        snprintf(message, sizeof(message), "Unsupported key size: %d bytes", (int)key_len);
        java_crypto_throw_name(env, "java/security/InvalidKeyException", message);
        return java_crypto_v_void();
    }
    /* Third argument: AlgorithmParameterSpec (IvParameterSpec) or SecureRandom. */
    const java_crypto_IvPayload *iv = n >= 3 ? java_crypto_payload_of_ref(a[2].l, IV_MAGIC) : NULL;
    const bool spec_arg = n >= 3 && a[2].l && !iv &&
                          ssrnx_jni_is_instance_of_class_host(a[2].l, "java/security/spec/AlgorithmParameterSpec");
    if (spec_arg) {
        java_crypto_throw_name(env, "java/security/InvalidAlgorithmParameterException", "Unsupported parameter spec");
        return java_crypto_v_void();
    }
    if (iv && p->mode == MODE_ECB) {
        java_crypto_throw_name(env, "java/security/InvalidAlgorithmParameterException", "ECB mode cannot use IV");
        return java_crypto_v_void();
    }
    if (iv && iv->length != AES_BLOCK) {
        java_crypto_throw_name(env, "java/security/InvalidAlgorithmParameterException", "IV must be 16 bytes long.");
        return java_crypto_v_void();
    }
    uint8_t iv_bytes[AES_BLOCK] = {0};
    bool has_iv = false;
    if (iv) { memcpy(iv_bytes, iv->bytes, AES_BLOCK); has_iv = true; }
    else if (p->mode != MODE_ECB) {
        if (opmode == 2) {
            java_crypto_throw_name(env, "java/security/InvalidKeyException", "IV must be specified in CBC/CTR mode");
            return java_crypto_v_void();
        }
        java_crypto_random_bytes(iv_bytes, AES_BLOCK);
        has_iv = true;
    }
    p->aes = aes;
    memcpy(p->iv, iv_bytes, AES_BLOCK);
    p->has_iv = has_iv;
    p->opmode = opmode;
    p->pending_len = 0;
    return java_crypto_v_void();
}

static bool java_crypto_pending_append(void *env, java_crypto_CipherPayload *p, const uint8_t *in, size_t len) {
    if (!len) return true;
    if (p->pending_len + len > p->pending_cap) {
        size_t cap = p->pending_cap ? p->pending_cap : 64;
        while (cap < p->pending_len + len) cap *= 2;
        uint8_t *q = realloc(p->pending, cap);
        if (!q) { java_crypto_throw_name(env, "java/lang/OutOfMemoryError", "Cipher"); return false; }
        p->pending = q; p->pending_cap = cap;
    }
    memcpy(p->pending + p->pending_len, in, len);
    p->pending_len += len;
    return true;
}

static bool java_crypto_cipher_ready(void *env, java_crypto_CipherPayload *p) {
    if (p && p->opmode) return true;
    java_crypto_throw_name(env, "java/lang/IllegalStateException", "Cipher not initialized");
    return false;
}

/* Runs the whole pending message; *out is malloc-owned. */
static bool java_crypto_cipher_run(void *env, java_crypto_CipherPayload *p, uint8_t **out, size_t *out_len) {
    const bool encrypt = p->opmode == 1;
    const size_t len = p->pending_len;
    *out = NULL; *out_len = 0;
    size_t total = len;
    if (p->mode != MODE_CTR) {
        if (encrypt && p->padding) total = (len / AES_BLOCK + 1) * AES_BLOCK;
        else if (len % AES_BLOCK) {
            java_crypto_throw_name(env, "javax/crypto/IllegalBlockSizeException",
                       encrypt ? "data not block size aligned"
                               : "last block incomplete in decryption");
            return false;
        }
    }
    uint8_t *buf = malloc(total ? total : 1);
    if (!buf) { java_crypto_throw_name(env, "java/lang/OutOfMemoryError", "Cipher"); return false; }
    if (len) memcpy(buf, p->pending, len);
    if (total > len) memset(buf + len, (int)(total - len), total - len);
    uint8_t chain[AES_BLOCK];
    memcpy(chain, p->iv, AES_BLOCK);
    for (size_t off = 0; off < total; off += AES_BLOCK) {
        uint8_t *block = buf + off;
        if (p->mode == MODE_CTR) {
            uint8_t ks[AES_BLOCK];
            memcpy(ks, chain, AES_BLOCK);
            java_crypto_aes_encrypt_block(&p->aes, ks);
            const size_t m = total - off < AES_BLOCK ? total - off : AES_BLOCK;
            for (size_t i = 0; i < m; ++i) block[i] ^= ks[i];
            for (int i = AES_BLOCK - 1; i >= 0 && ++chain[i] == 0; --i) {}
        } else if (p->mode == MODE_CBC && encrypt) {
            for (int i = 0; i < AES_BLOCK; ++i) block[i] ^= chain[i];
            java_crypto_aes_encrypt_block(&p->aes, block);
            memcpy(chain, block, AES_BLOCK);
        } else if (p->mode == MODE_CBC) {
            uint8_t saved[AES_BLOCK];
            memcpy(saved, block, AES_BLOCK);
            java_crypto_aes_decrypt_block(&p->aes, block);
            for (int i = 0; i < AES_BLOCK; ++i) block[i] ^= chain[i];
            memcpy(chain, saved, AES_BLOCK);
        } else if (encrypt) {
            java_crypto_aes_encrypt_block(&p->aes, block);
        } else {
            java_crypto_aes_decrypt_block(&p->aes, block);
        }
    }
    if (!encrypt && p->padding && p->mode != MODE_CTR) {
        const uint8_t pad = total ? buf[total - 1] : 0;
        bool ok = pad >= 1 && pad <= AES_BLOCK && pad <= total;
        for (size_t i = 0; ok && i < pad; ++i) ok = buf[total - 1 - i] == pad;
        if (!ok) {
            free(buf);
            java_crypto_throw_name(env, "javax/crypto/BadPaddingException", "pad block corrupted");
            return false;
        }
        total -= pad;
    }
    *out = buf;
    *out_len = total;
    return true;
}

static SsrnxJniValue java_crypto_cipher_update(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    java_crypto_CipherPayload *p = java_crypto_payload_of(t, CIPHER_MAGIC);
    if (!java_crypto_cipher_ready(env, p) || (n != 1 && n != 3)) return java_crypto_v_object(NULL);
    uint8_t *in = NULL;
    const int32_t offset = n == 3 ? a[1].i : 0;
    const int32_t len = n == 3 ? a[2].i : (a[0].l ? ssrnx_jni_byte_array_length(a[0].l) : 0);
    if (!java_crypto_read_bytes(env, a[0].l, offset, len, &in)) return java_crypto_v_object(NULL);
    const bool ok = java_crypto_pending_append(env, p, in, (size_t)len);
    free(in);
    /* Output is produced at doFinal(); the concatenation is identical. */
    return java_crypto_v_object(ok ? ssrnx_jni_new_local_byte_array(env, 0) : NULL);
}

static SsrnxJniValue java_crypto_cipher_do_final(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    java_crypto_CipherPayload *p = java_crypto_payload_of(t, CIPHER_MAGIC);
    if (!java_crypto_cipher_ready(env, p) || (n != 0 && n != 1 && n != 3)) return java_crypto_v_object(NULL);
    if (n) {
        uint8_t *in = NULL;
        const int32_t offset = n == 3 ? a[1].i : 0;
        const int32_t len = n == 3 ? a[2].i : (a[0].l ? ssrnx_jni_byte_array_length(a[0].l) : 0);
        if (!java_crypto_read_bytes(env, a[0].l, offset, len, &in)) return java_crypto_v_object(NULL);
        const bool ok = java_crypto_pending_append(env, p, in, (size_t)len);
        free(in);
        if (!ok) return java_crypto_v_object(NULL);
    }
    uint8_t *out = NULL;
    size_t out_len = 0;
    const bool ok = java_crypto_cipher_run(env, p, &out, &out_len);
    /* doFinal resets the Cipher to its initialized state (same key and IV). */
    p->pending_len = 0;
    void *result = ok ? java_crypto_new_byte_array(env, out, out_len) : NULL;
    free(out);
    return java_crypto_v_object(result);
}

static SsrnxJniValue java_crypto_cipher_get_iv(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)n; (void)u;
    java_crypto_CipherPayload *p = java_crypto_payload_of(t, CIPHER_MAGIC);
    return java_crypto_v_object(p && p->has_iv ? java_crypto_new_byte_array(env, p->iv, AES_BLOCK) : NULL);
}
static SsrnxJniValue java_crypto_cipher_get_block_size(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)env; (void)t; (void)a; (void)n; (void)u;
    return java_crypto_v_int(AES_BLOCK);
}
static SsrnxJniValue java_crypto_cipher_get_algorithm(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)a; (void)n; (void)u;
    java_crypto_CipherPayload *p = java_crypto_payload_of(t, CIPHER_MAGIC);
    return java_crypto_v_object(p ? ssrnx_jni_new_local_string_utf(env, p->transformation) : NULL);
}
static SsrnxJniValue java_crypto_cipher_get_output_size(void *env, void *t, const SsrnxJniValue *a, size_t n, void *u) {
    (void)u;
    java_crypto_CipherPayload *p = java_crypto_payload_of(t, CIPHER_MAGIC);
    if (!java_crypto_cipher_ready(env, p) || n != 1) return java_crypto_v_int(0);
    const size_t len = p->pending_len + (size_t)(a[0].i > 0 ? a[0].i : 0);
    const size_t out = p->opmode == 1 && p->padding && p->mode != MODE_CTR
                       ? (len / AES_BLOCK + 1) * AES_BLOCK : len;
    return java_crypto_v_int(out > INT32_MAX ? INT32_MAX : (int32_t)out);
}

typedef struct java_crypto_CryptoMethod {
    const char *cls, *name, *sig;
    bool is_static;
    SsrnxJniMethodCallback cb;
} java_crypto_CryptoMethod;

static const java_crypto_CryptoMethod java_crypto_k_methods[] = {
    {HMAC_SPI, "create", "(Ljava/lang/String;)V", false, java_crypto_hmac_create},
    {HMAC_SPI, "engineGetMacLength", "()I", false, java_crypto_hmac_length},
    {HMAC_SPI, "engineInit", "(Ljava/security/Key;Ljava/security/spec/AlgorithmParameterSpec;)V", false, java_crypto_hmac_init},
    {HMAC_SPI, "engineUpdate", "(B)V", false, java_crypto_hmac_update},
    {HMAC_SPI, "engineUpdate", "([BII)V", false, java_crypto_hmac_update},
    {HMAC_SPI, "engineDoFinal", "()[B", false, java_crypto_hmac_final},
    {HMAC_SPI, "engineReset", "()V", false, java_crypto_hmac_reset},
    {HMAC_SPI, "clone", "()Ljava/lang/Object;", false, java_crypto_hmac_clone},
    {SECRET_KEY_SPEC, "<init>", "([BLjava/lang/String;)V", false, java_crypto_key_ctor},
    {SECRET_KEY_SPEC, "<init>", "([BIILjava/lang/String;)V", false, java_crypto_key_ctor_range},
    {SECRET_KEY_SPEC, "getAlgorithm", "()Ljava/lang/String;", false, java_crypto_key_get_algorithm},
    {SECRET_KEY_SPEC, "getFormat", "()Ljava/lang/String;", false, java_crypto_key_get_format},
    {SECRET_KEY_SPEC, "getEncoded", "()[B", false, java_crypto_key_get_encoded},
    {SECRET_KEY_SPEC, "hashCode", "()I", false, java_crypto_key_hash},
    {SECRET_KEY_SPEC, "equals", "(Ljava/lang/Object;)Z", false, java_crypto_key_equals},
    {IV_SPEC, "<init>", "([B)V", false, java_crypto_iv_ctor},
    {IV_SPEC, "<init>", "([BII)V", false, java_crypto_iv_ctor_range},
    {IV_SPEC, "getIV", "()[B", false, java_crypto_iv_get},
    {CIPHER, "getInstance", "(Ljava/lang/String;)Ljavax/crypto/Cipher;", true, java_crypto_cipher_get_instance},
    {CIPHER, "getInstance", "(Ljava/lang/String;Ljava/lang/String;)Ljavax/crypto/Cipher;", true, java_crypto_cipher_get_instance},
    {CIPHER, "getInstance", "(Ljava/lang/String;Ljava/security/Provider;)Ljavax/crypto/Cipher;", true, java_crypto_cipher_get_instance},
    {CIPHER, "init", "(ILjava/security/Key;)V", false, java_crypto_cipher_init},
    {CIPHER, "init", "(ILjava/security/Key;Ljava/security/spec/AlgorithmParameterSpec;)V", false, java_crypto_cipher_init},
    {CIPHER, "init", "(ILjava/security/Key;Ljava/security/SecureRandom;)V", false, java_crypto_cipher_init},
    {CIPHER, "init", "(ILjava/security/Key;Ljava/security/spec/AlgorithmParameterSpec;Ljava/security/SecureRandom;)V", false, java_crypto_cipher_init},
    {CIPHER, "update", "([B)[B", false, java_crypto_cipher_update},
    {CIPHER, "update", "([BII)[B", false, java_crypto_cipher_update},
    {CIPHER, "doFinal", "()[B", false, java_crypto_cipher_do_final},
    {CIPHER, "doFinal", "([B)[B", false, java_crypto_cipher_do_final},
    {CIPHER, "doFinal", "([BII)[B", false, java_crypto_cipher_do_final},
    {CIPHER, "getIV", "()[B", false, java_crypto_cipher_get_iv},
    {CIPHER, "getBlockSize", "()I", false, java_crypto_cipher_get_block_size},
    {CIPHER, "getAlgorithm", "()Ljava/lang/String;", false, java_crypto_cipher_get_algorithm},
    {CIPHER, "getOutputSize", "(I)I", false, java_crypto_cipher_get_output_size},
};

SsrnxResult ssrnx_java_crypto_register(void) {
    static const struct { const char *name, *super; } classes[] = {
        {"java/security/GeneralSecurityException", "java/lang/Exception"},
        {"java/security/cert/CertificateException", "java/security/GeneralSecurityException"},
        {"java/security/cert/CertificateParsingException", "java/security/cert/CertificateException"},
        {"java/security/cert/CertificateExpiredException", "java/security/cert/CertificateException"},
        {"java/security/cert/CertificateNotYetValidException", "java/security/cert/CertificateException"},
        {"java/security/NoSuchAlgorithmException", "java/security/GeneralSecurityException"},
        {"java/security/KeyException", "java/security/GeneralSecurityException"},
        {"java/security/InvalidKeyException", "java/security/KeyException"},
        {"java/security/SignatureException", "java/security/GeneralSecurityException"},
        {"java/security/InvalidAlgorithmParameterException", "java/security/GeneralSecurityException"},
        {"java/security/InvalidParameterException", "java/lang/IllegalArgumentException"},
        {"java/security/ProviderException", "java/lang/RuntimeException"},
        {"javax/crypto/NoSuchPaddingException", "java/security/GeneralSecurityException"},
        {"javax/crypto/BadPaddingException", "java/security/GeneralSecurityException"},
        {"javax/crypto/AEADBadTagException", "javax/crypto/BadPaddingException"},
        {"javax/crypto/IllegalBlockSizeException", "java/security/GeneralSecurityException"},
        {"javax/crypto/ShortBufferException", "java/security/GeneralSecurityException"},
        {"java/security/Key", "java/lang/Object"},
        {"javax/crypto/SecretKey", "java/lang/Object"},
        {"java/security/spec/KeySpec", "java/lang/Object"},
        {"java/security/spec/AlgorithmParameterSpec", "java/lang/Object"},
        {SECRET_KEY_SPEC, "java/lang/Object"},
        {IV_SPEC, "java/lang/Object"},
        {CIPHER, "java/lang/Object"},
        {"javax/crypto/MacSpi", "java/lang/Object"},
        {HMAC_SPI, "javax/crypto/MacSpi"},
        {"javax/crypto/SecretKeyFactorySpi", "java/lang/Object"},
        {"com/apn64/security/Apn64Pbkdf2", "javax/crypto/SecretKeyFactorySpi"},
    };
    SsrnxResult rc = SSRNX_OK;
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(classes) / sizeof(classes[0]); ++i)
        rc = ssrnx_jni_register_class(classes[i].name, classes[i].super);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_method("com/apn64/security/Apn64Pbkdf2", "derive",
        "([B[BIILjava/lang/String;)[B", false, java_crypto_pbkdf2_derive, NULL);
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_interface_method("javax/crypto/SecretKey", "getAlgorithm", "()Ljava/lang/String;");
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_interface("javax/crypto/SecretKey", "java/security/Key");
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_interface(SECRET_KEY_SPEC, "javax/crypto/SecretKey");
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_interface(SECRET_KEY_SPEC, "java/security/spec/KeySpec");
    if (rc == SSRNX_OK) rc = ssrnx_jni_register_interface(IV_SPEC, "java/security/spec/AlgorithmParameterSpec");
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(java_crypto_k_methods) / sizeof(java_crypto_k_methods[0]); ++i)
        rc = ssrnx_jni_register_method(java_crypto_k_methods[i].cls, java_crypto_k_methods[i].name, java_crypto_k_methods[i].sig,
                                       java_crypto_k_methods[i].is_static, java_crypto_k_methods[i].cb, NULL);
    static const struct { const char *name; int32_t value; } modes[] = {
        {"ENCRYPT_MODE", 1}, {"DECRYPT_MODE", 2}, {"WRAP_MODE", 3}, {"UNWRAP_MODE", 4},
        {"PUBLIC_KEY", 1}, {"PRIVATE_KEY", 2}, {"SECRET_KEY", 3},
    };
    for (size_t i = 0; rc == SSRNX_OK && i < sizeof(modes) / sizeof(modes[0]); ++i) {
        SsrnxJniValue value = {0};
        value.i = modes[i].value;
        rc = ssrnx_jni_register_field(CIPHER, modes[i].name, "I", true, value);
    }
    return rc;
}

void ssrnx_java_crypto_reset(void) {
    pthread_mutex_lock(&java_crypto_k_hmac_lock);
    java_crypto_HmacPayload *p = java_crypto_k_hmac_states;
    java_crypto_k_hmac_states = NULL;
    pthread_mutex_unlock(&java_crypto_k_hmac_lock);
    while (p) { java_crypto_HmacPayload *next = p->next; java_crypto_hmac_free(p); p = next; }
}

#pragma pop_macro("SECRET_KEY_SPEC")
#pragma pop_macro("KEY_MAGIC")
#pragma pop_macro("IV_SPEC")
#pragma pop_macro("IV_MAGIC")
#pragma pop_macro("HMAC_SPI")
#pragma pop_macro("HMAC_MAGIC")
#pragma pop_macro("CIPHER_MAGIC")
#pragma pop_macro("CIPHER")
#pragma pop_macro("AES_BLOCK")
