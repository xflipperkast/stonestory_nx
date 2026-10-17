/* runtime assets. */
#define _GNU_SOURCE 1
#include "runtime.h"


/* ---- apk ---- */
#pragma push_macro("APK_ARSC_CACHE_SLOTS")
#pragma push_macro("APK_MANIFEST_CACHE_SLOTS")
#pragma push_macro("APK_SIG_BLOCK_MAX")
#pragma push_macro("APK_SIG_BLOCK_V2_ID")
#pragma push_macro("APK_SIG_BLOCK_V3_ID")
#pragma push_macro("RES_NO_ENTRY")
#pragma push_macro("RES_STRING_POOL_TYPE")
#pragma push_macro("RES_STRING_POOL_UTF8_FLAG")
#pragma push_macro("RES_TABLE_ENTRY_FLAG_COMPLEX")
#pragma push_macro("RES_TABLE_PACKAGE_TYPE")
#pragma push_macro("RES_TABLE_TYPE")
#pragma push_macro("RES_TABLE_TYPE_FLAG_OFFSET16")
#pragma push_macro("RES_TABLE_TYPE_FLAG_SPARSE")
#pragma push_macro("RES_TABLE_TYPE_TYPE")
#pragma push_macro("RES_VALUE_TYPE_DYNAMIC_REFERENCE")
#pragma push_macro("RES_VALUE_TYPE_FLOAT")
#pragma push_macro("RES_VALUE_TYPE_INT_BOOLEAN")
#pragma push_macro("RES_VALUE_TYPE_INT_COLOR_ARGB4")
#pragma push_macro("RES_VALUE_TYPE_INT_COLOR_ARGB8")
#pragma push_macro("RES_VALUE_TYPE_INT_COLOR_RGB4")
#pragma push_macro("RES_VALUE_TYPE_INT_COLOR_RGB8")
#pragma push_macro("RES_VALUE_TYPE_INT_DEC")
#pragma push_macro("RES_VALUE_TYPE_INT_HEX")
#pragma push_macro("RES_VALUE_TYPE_REFERENCE")
#pragma push_macro("RES_VALUE_TYPE_STRING")
#pragma push_macro("RES_XML_END_ELEMENT_TYPE")
#pragma push_macro("RES_XML_START_ELEMENT_TYPE")
#pragma push_macro("RES_XML_TYPE")
#pragma push_macro("ZIP_CENTRAL_SIG")
#pragma push_macro("ZIP_EOCD_MIN_SIZE")
#pragma push_macro("ZIP_EOCD_SIG")
#pragma push_macro("ZIP_LOCAL_SIG")
#pragma push_macro("ZIP_MAX_COMMENT")
#pragma push_macro("ZIP_METHOD_DEFLATE")
#pragma push_macro("ZIP_METHOD_STORE")
#pragma push_macro("ZIP_STREAM_CHUNK")





#include <errno.h>
#include <pthread.h>
#include <limits.h>
#include <lzma.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define ZIP_EOCD_SIG       UINT32_C(0x06054b50)
#define ZIP_CENTRAL_SIG    UINT32_C(0x02014b50)
#define ZIP_LOCAL_SIG      UINT32_C(0x04034b50)
#define ZIP_METHOD_STORE   0u
#define ZIP_METHOD_DEFLATE 8u
#define ZIP_EOCD_MIN_SIZE  22u
#define ZIP_MAX_COMMENT    65535u
#define ZIP_STREAM_CHUNK   (1024u * 1024u)

#define RES_STRING_POOL_TYPE       0x0001u
#define RES_XML_TYPE               0x0003u
#define RES_XML_END_ELEMENT_TYPE   0x0103u
#define RES_XML_START_ELEMENT_TYPE 0x0102u
#define RES_STRING_POOL_UTF8_FLAG  0x00000100u
#define RES_VALUE_TYPE_REFERENCE   0x01u
#define RES_VALUE_TYPE_STRING      0x03u
#define RES_VALUE_TYPE_FLOAT       0x04u
#define RES_VALUE_TYPE_DYNAMIC_REFERENCE 0x07u
#define RES_VALUE_TYPE_INT_DEC     0x10u
#define RES_VALUE_TYPE_INT_HEX     0x11u
#define RES_VALUE_TYPE_INT_BOOLEAN 0x12u
#define RES_VALUE_TYPE_INT_COLOR_ARGB8 0x1cu
#define RES_VALUE_TYPE_INT_COLOR_RGB8  0x1du
#define RES_VALUE_TYPE_INT_COLOR_ARGB4 0x1eu
#define RES_VALUE_TYPE_INT_COLOR_RGB4  0x1fu
#define RES_TABLE_TYPE             0x0002u
#define RES_TABLE_PACKAGE_TYPE     0x0200u
#define RES_TABLE_TYPE_TYPE        0x0201u
#define RES_TABLE_ENTRY_FLAG_COMPLEX 0x0001u
#define RES_TABLE_TYPE_FLAG_SPARSE 0x01u
#define RES_TABLE_TYPE_FLAG_OFFSET16 0x02u
#define RES_NO_ENTRY               UINT32_C(0xffffffff)

bool ssrnx_platform_decode(const uint8_t *input, size_t input_size, void *output, size_t output_size) {
    if (!input || !output) return false;
    uint64_t memory_limit = 64u * 1024u * 1024u;
    size_t in_pos = 0, out_pos = 0;
    return lzma_stream_buffer_decode(&memory_limit, 0, NULL, input, &in_pos, input_size,
                                    output, &out_pos, output_size) == LZMA_OK &&
           in_pos == input_size && out_pos == output_size;
}

typedef struct apk_BitReader {
    const uint8_t *data;
    size_t size;
    size_t pos;
    uint32_t bits;
    unsigned count;
} apk_BitReader;

typedef struct apk_Huffman {
    uint16_t count[16];
    uint16_t symbol[288];
} apk_Huffman;

typedef struct apk_ZipEntryInfo {
    char name[SSRNX_APK_ENTRY_CAP];
    uint16_t flags;
    uint16_t method;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint32_t local_offset;
    uint32_t dos_time;
} apk_ZipEntryInfo;

/* The central directory is read once into memory: every entry lookup and
 * walk parses it from RAM instead of re-reading it from the card in small
 * stdio-buffered pieces. `file` serves entry data (local headers/payloads). */
typedef struct ZipDirectory {
    FILE *file;
    uint32_t central_offset;
    uint32_t central_size;
    uint16_t entry_count;
    long file_size;
    uint8_t *central;
    size_t central_cursor;
} ZipDirectory;

typedef struct apk_AxmlStringPool {
    const uint8_t *chunk;
    size_t chunk_size;
    const uint8_t *offsets;
    const uint8_t *strings;
    uint32_t string_count;
    uint32_t flags;
    uint32_t strings_start;
} apk_AxmlStringPool;

static uint16_t apk_rd16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t apk_rd32(const uint8_t *p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint64_t apk_rd64(const uint8_t *p) {
    return (uint64_t)apk_rd32(p) | ((uint64_t)apk_rd32(p + 4) << 32);
}

static bool apk_copy_string(char *out, size_t cap, const char *src) {
    if (!out || cap == 0 || !src) return false;
    const size_t n = strlen(src);
    if (n >= cap) {
        out[0] = '\0';
        return false;
    }
    memcpy(out, src, n + 1);
    return true;
}

static bool apk_starts_with(const char *s, const char *prefix) {
    if (!s || !prefix) return false;
    const size_t n = strlen(prefix);
    return strncmp(s, prefix, n) == 0;
}

static bool apk_ends_with(const char *s, const char *suffix) {
    if (!s || !suffix) return false;
    const size_t sn = strlen(s), en = strlen(suffix);
    return sn >= en && memcmp(s + sn - en, suffix, en) == 0;
}

/* ZIP CRC32 is used on both small metadata entries and very large nested APKs.
 * The original bit-at-a-time loop was correct but extremely expensive for
 * 100+ MiB STORE entries. Keep an incremental state so streamed extraction can
 * validate bytes as they are copied, and use the standard 256-entry table
 * algorithm to reduce each byte to one table lookup instead of eight bit
 * iterations. */
static const uint32_t apk_k_crc32_table[256] = {
    UINT32_C(0x00000000), UINT32_C(0x77073096), UINT32_C(0xee0e612c), UINT32_C(0x990951ba), UINT32_C(0x076dc419), UINT32_C(0x706af48f), UINT32_C(0xe963a535), UINT32_C(0x9e6495a3),
    UINT32_C(0x0edb8832), UINT32_C(0x79dcb8a4), UINT32_C(0xe0d5e91e), UINT32_C(0x97d2d988), UINT32_C(0x09b64c2b), UINT32_C(0x7eb17cbd), UINT32_C(0xe7b82d07), UINT32_C(0x90bf1d91),
    UINT32_C(0x1db71064), UINT32_C(0x6ab020f2), UINT32_C(0xf3b97148), UINT32_C(0x84be41de), UINT32_C(0x1adad47d), UINT32_C(0x6ddde4eb), UINT32_C(0xf4d4b551), UINT32_C(0x83d385c7),
    UINT32_C(0x136c9856), UINT32_C(0x646ba8c0), UINT32_C(0xfd62f97a), UINT32_C(0x8a65c9ec), UINT32_C(0x14015c4f), UINT32_C(0x63066cd9), UINT32_C(0xfa0f3d63), UINT32_C(0x8d080df5),
    UINT32_C(0x3b6e20c8), UINT32_C(0x4c69105e), UINT32_C(0xd56041e4), UINT32_C(0xa2677172), UINT32_C(0x3c03e4d1), UINT32_C(0x4b04d447), UINT32_C(0xd20d85fd), UINT32_C(0xa50ab56b),
    UINT32_C(0x35b5a8fa), UINT32_C(0x42b2986c), UINT32_C(0xdbbbc9d6), UINT32_C(0xacbcf940), UINT32_C(0x32d86ce3), UINT32_C(0x45df5c75), UINT32_C(0xdcd60dcf), UINT32_C(0xabd13d59),
    UINT32_C(0x26d930ac), UINT32_C(0x51de003a), UINT32_C(0xc8d75180), UINT32_C(0xbfd06116), UINT32_C(0x21b4f4b5), UINT32_C(0x56b3c423), UINT32_C(0xcfba9599), UINT32_C(0xb8bda50f),
    UINT32_C(0x2802b89e), UINT32_C(0x5f058808), UINT32_C(0xc60cd9b2), UINT32_C(0xb10be924), UINT32_C(0x2f6f7c87), UINT32_C(0x58684c11), UINT32_C(0xc1611dab), UINT32_C(0xb6662d3d),
    UINT32_C(0x76dc4190), UINT32_C(0x01db7106), UINT32_C(0x98d220bc), UINT32_C(0xefd5102a), UINT32_C(0x71b18589), UINT32_C(0x06b6b51f), UINT32_C(0x9fbfe4a5), UINT32_C(0xe8b8d433),
    UINT32_C(0x7807c9a2), UINT32_C(0x0f00f934), UINT32_C(0x9609a88e), UINT32_C(0xe10e9818), UINT32_C(0x7f6a0dbb), UINT32_C(0x086d3d2d), UINT32_C(0x91646c97), UINT32_C(0xe6635c01),
    UINT32_C(0x6b6b51f4), UINT32_C(0x1c6c6162), UINT32_C(0x856530d8), UINT32_C(0xf262004e), UINT32_C(0x6c0695ed), UINT32_C(0x1b01a57b), UINT32_C(0x8208f4c1), UINT32_C(0xf50fc457),
    UINT32_C(0x65b0d9c6), UINT32_C(0x12b7e950), UINT32_C(0x8bbeb8ea), UINT32_C(0xfcb9887c), UINT32_C(0x62dd1ddf), UINT32_C(0x15da2d49), UINT32_C(0x8cd37cf3), UINT32_C(0xfbd44c65),
    UINT32_C(0x4db26158), UINT32_C(0x3ab551ce), UINT32_C(0xa3bc0074), UINT32_C(0xd4bb30e2), UINT32_C(0x4adfa541), UINT32_C(0x3dd895d7), UINT32_C(0xa4d1c46d), UINT32_C(0xd3d6f4fb),
    UINT32_C(0x4369e96a), UINT32_C(0x346ed9fc), UINT32_C(0xad678846), UINT32_C(0xda60b8d0), UINT32_C(0x44042d73), UINT32_C(0x33031de5), UINT32_C(0xaa0a4c5f), UINT32_C(0xdd0d7cc9),
    UINT32_C(0x5005713c), UINT32_C(0x270241aa), UINT32_C(0xbe0b1010), UINT32_C(0xc90c2086), UINT32_C(0x5768b525), UINT32_C(0x206f85b3), UINT32_C(0xb966d409), UINT32_C(0xce61e49f),
    UINT32_C(0x5edef90e), UINT32_C(0x29d9c998), UINT32_C(0xb0d09822), UINT32_C(0xc7d7a8b4), UINT32_C(0x59b33d17), UINT32_C(0x2eb40d81), UINT32_C(0xb7bd5c3b), UINT32_C(0xc0ba6cad),
    UINT32_C(0xedb88320), UINT32_C(0x9abfb3b6), UINT32_C(0x03b6e20c), UINT32_C(0x74b1d29a), UINT32_C(0xead54739), UINT32_C(0x9dd277af), UINT32_C(0x04db2615), UINT32_C(0x73dc1683),
    UINT32_C(0xe3630b12), UINT32_C(0x94643b84), UINT32_C(0x0d6d6a3e), UINT32_C(0x7a6a5aa8), UINT32_C(0xe40ecf0b), UINT32_C(0x9309ff9d), UINT32_C(0x0a00ae27), UINT32_C(0x7d079eb1),
    UINT32_C(0xf00f9344), UINT32_C(0x8708a3d2), UINT32_C(0x1e01f268), UINT32_C(0x6906c2fe), UINT32_C(0xf762575d), UINT32_C(0x806567cb), UINT32_C(0x196c3671), UINT32_C(0x6e6b06e7),
    UINT32_C(0xfed41b76), UINT32_C(0x89d32be0), UINT32_C(0x10da7a5a), UINT32_C(0x67dd4acc), UINT32_C(0xf9b9df6f), UINT32_C(0x8ebeeff9), UINT32_C(0x17b7be43), UINT32_C(0x60b08ed5),
    UINT32_C(0xd6d6a3e8), UINT32_C(0xa1d1937e), UINT32_C(0x38d8c2c4), UINT32_C(0x4fdff252), UINT32_C(0xd1bb67f1), UINT32_C(0xa6bc5767), UINT32_C(0x3fb506dd), UINT32_C(0x48b2364b),
    UINT32_C(0xd80d2bda), UINT32_C(0xaf0a1b4c), UINT32_C(0x36034af6), UINT32_C(0x41047a60), UINT32_C(0xdf60efc3), UINT32_C(0xa867df55), UINT32_C(0x316e8eef), UINT32_C(0x4669be79),
    UINT32_C(0xcb61b38c), UINT32_C(0xbc66831a), UINT32_C(0x256fd2a0), UINT32_C(0x5268e236), UINT32_C(0xcc0c7795), UINT32_C(0xbb0b4703), UINT32_C(0x220216b9), UINT32_C(0x5505262f),
    UINT32_C(0xc5ba3bbe), UINT32_C(0xb2bd0b28), UINT32_C(0x2bb45a92), UINT32_C(0x5cb36a04), UINT32_C(0xc2d7ffa7), UINT32_C(0xb5d0cf31), UINT32_C(0x2cd99e8b), UINT32_C(0x5bdeae1d),
    UINT32_C(0x9b64c2b0), UINT32_C(0xec63f226), UINT32_C(0x756aa39c), UINT32_C(0x026d930a), UINT32_C(0x9c0906a9), UINT32_C(0xeb0e363f), UINT32_C(0x72076785), UINT32_C(0x05005713),
    UINT32_C(0x95bf4a82), UINT32_C(0xe2b87a14), UINT32_C(0x7bb12bae), UINT32_C(0x0cb61b38), UINT32_C(0x92d28e9b), UINT32_C(0xe5d5be0d), UINT32_C(0x7cdcefb7), UINT32_C(0x0bdbdf21),
    UINT32_C(0x86d3d2d4), UINT32_C(0xf1d4e242), UINT32_C(0x68ddb3f8), UINT32_C(0x1fda836e), UINT32_C(0x81be16cd), UINT32_C(0xf6b9265b), UINT32_C(0x6fb077e1), UINT32_C(0x18b74777),
    UINT32_C(0x88085ae6), UINT32_C(0xff0f6a70), UINT32_C(0x66063bca), UINT32_C(0x11010b5c), UINT32_C(0x8f659eff), UINT32_C(0xf862ae69), UINT32_C(0x616bffd3), UINT32_C(0x166ccf45),
    UINT32_C(0xa00ae278), UINT32_C(0xd70dd2ee), UINT32_C(0x4e048354), UINT32_C(0x3903b3c2), UINT32_C(0xa7672661), UINT32_C(0xd06016f7), UINT32_C(0x4969474d), UINT32_C(0x3e6e77db),
    UINT32_C(0xaed16a4a), UINT32_C(0xd9d65adc), UINT32_C(0x40df0b66), UINT32_C(0x37d83bf0), UINT32_C(0xa9bcae53), UINT32_C(0xdebb9ec5), UINT32_C(0x47b2cf7f), UINT32_C(0x30b5ffe9),
    UINT32_C(0xbdbdf21c), UINT32_C(0xcabac28a), UINT32_C(0x53b39330), UINT32_C(0x24b4a3a6), UINT32_C(0xbad03605), UINT32_C(0xcdd70693), UINT32_C(0x54de5729), UINT32_C(0x23d967bf),
    UINT32_C(0xb3667a2e), UINT32_C(0xc4614ab8), UINT32_C(0x5d681b02), UINT32_C(0x2a6f2b94), UINT32_C(0xb40bbe37), UINT32_C(0xc30c8ea1), UINT32_C(0x5a05df1b), UINT32_C(0x2d02ef8d),
};

static uint32_t apk_crc32_update(uint32_t crc, const uint8_t *data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        crc = apk_k_crc32_table[(crc ^ data[i]) & 0xffu] ^ (crc >> 8);
    }
    return crc;
}

static uint32_t apk_crc32_bytes(const uint8_t *data, size_t size) {
    return ~apk_crc32_update(UINT32_C(0xffffffff), data, size);
}

/* ---- raw DEFLATE --------------------------------------------------------- */

static bool apk_br_bits(apk_BitReader *br, unsigned need, uint32_t *out) {
    if (!br || !out || need > 24) return false;
    while (br->count < need) {
        if (br->pos >= br->size) return false;
        br->bits |= (uint32_t)br->data[br->pos++] << br->count;
        br->count += 8;
    }
    *out = need ? (br->bits & ((UINT32_C(1) << need) - 1u)) : 0u;
    br->bits >>= need;
    br->count -= need;
    return true;
}

static void apk_br_align(apk_BitReader *br) {
    if (!br) return;
    br->bits = 0;
    br->count = 0;
}

static bool apk_huff_construct(apk_Huffman *h, const uint8_t *lengths, size_t count) {
    if (!h || (!lengths && count)) return false;
    memset(h, 0, sizeof(*h));
    for (size_t i = 0; i < count; ++i) {
        if (lengths[i] > 15) return false;
        h->count[lengths[i]]++;
    }
    if (h->count[0] == count) return true;

    int left = 1;
    for (unsigned len = 1; len <= 15; ++len) {
        left <<= 1;
        left -= h->count[len];
        if (left < 0) return false;
    }

    uint16_t offsets[16] = {0};
    for (unsigned len = 1; len < 15; ++len) {
        offsets[len + 1] = (uint16_t)(offsets[len] + h->count[len]);
    }
    for (size_t sym = 0; sym < count; ++sym) {
        const uint8_t len = lengths[sym];
        if (len) h->symbol[offsets[len]++] = (uint16_t)sym;
    }
    return true;
}

static int apk_huff_decode(apk_BitReader *br, const apk_Huffman *h) {
    int code = 0, first = 0, index = 0;
    for (int len = 1; len <= 15; ++len) {
        uint32_t bit = 0;
        if (!apk_br_bits(br, 1, &bit)) return -1;
        code |= (int)bit;
        const int count = h->count[len];
        if (code >= first && code < first + count) {
            return h->symbol[index + (code - first)];
        }
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    return -1;
}

static bool apk_build_fixed(apk_Huffman *litlen, apk_Huffman *dist) {
    uint8_t ll[288];
    uint8_t dd[32];
    for (int i = 0; i <= 143; ++i) ll[i] = 8;
    for (int i = 144; i <= 255; ++i) ll[i] = 9;
    for (int i = 256; i <= 279; ++i) ll[i] = 7;
    for (int i = 280; i <= 287; ++i) ll[i] = 8;
    for (int i = 0; i < 32; ++i) dd[i] = 5;
    return apk_huff_construct(litlen, ll, 288) && apk_huff_construct(dist, dd, 32);
}

static bool apk_build_dynamic(apk_BitReader *br, apk_Huffman *litlen, apk_Huffman *dist) {
    static const uint8_t order[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
    uint32_t v = 0;
    if (!apk_br_bits(br, 5, &v)) return false;
    const unsigned nlit = v + 257;
    if (!apk_br_bits(br, 5, &v)) return false;
    const unsigned ndist = v + 1;
    if (!apk_br_bits(br, 4, &v)) return false;
    const unsigned ncode = v + 4;
    if (nlit > 286 || ndist > 32) return false;

    uint8_t codelen[19] = {0};
    for (unsigned i = 0; i < ncode; ++i) {
        if (!apk_br_bits(br, 3, &v)) return false;
        codelen[order[i]] = (uint8_t)v;
    }

    apk_Huffman code_tree;
    if (!apk_huff_construct(&code_tree, codelen, 19)) return false;

    uint8_t lengths[286 + 32];
    unsigned have = 0;
    const unsigned total = nlit + ndist;
    while (have < total) {
        const int sym = apk_huff_decode(br, &code_tree);
        if (sym < 0) return false;
        if (sym <= 15) {
            lengths[have++] = (uint8_t)sym;
        } else if (sym == 16) {
            if (have == 0 || !apk_br_bits(br, 2, &v)) return false;
            unsigned repeat = v + 3;
            if (have + repeat > total) return false;
            const uint8_t prev = lengths[have - 1];
            while (repeat--) lengths[have++] = prev;
        } else if (sym == 17) {
            if (!apk_br_bits(br, 3, &v)) return false;
            unsigned repeat = v + 3;
            if (have + repeat > total) return false;
            while (repeat--) lengths[have++] = 0;
        } else if (sym == 18) {
            if (!apk_br_bits(br, 7, &v)) return false;
            unsigned repeat = v + 11;
            if (have + repeat > total) return false;
            while (repeat--) lengths[have++] = 0;
        } else {
            return false;
        }
    }

    if (lengths[256] == 0) return false;
    return apk_huff_construct(litlen, lengths, nlit) &&
           apk_huff_construct(dist, lengths + nlit, ndist);
}

static bool apk_inflate_codes(apk_BitReader *br, const apk_Huffman *litlen, const apk_Huffman *dist,
                          uint8_t *out, size_t out_size, size_t *out_pos) {
    static const uint16_t len_base[29] = {
        3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258
    };
    static const uint8_t len_extra[29] = {
        0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0
    };
    static const uint16_t dist_base[30] = {
        1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,
        4097,6145,8193,12289,16385,24577
    };
    static const uint8_t dist_extra[30] = {
        0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13
    };

    for (;;) {
        const int sym = apk_huff_decode(br, litlen);
        if (sym < 0) return false;
        if (sym < 256) {
            if (*out_pos >= out_size) return false;
            out[(*out_pos)++] = (uint8_t)sym;
            continue;
        }
        if (sym == 256) return true;
        if (sym < 257 || sym > 285) return false;

        const unsigned li = (unsigned)(sym - 257);
        uint32_t extra = 0;
        if (!apk_br_bits(br, len_extra[li], &extra)) return false;
        const size_t length = (size_t)len_base[li] + extra;

        const int dsym = apk_huff_decode(br, dist);
        if (dsym < 0 || dsym > 29) return false;
        if (!apk_br_bits(br, dist_extra[dsym], &extra)) return false;
        const size_t distance = (size_t)dist_base[dsym] + extra;
        if (distance == 0 || distance > *out_pos || length > out_size - *out_pos) return false;
        for (size_t i = 0; i < length; ++i) {
            out[*out_pos] = out[*out_pos - distance];
            (*out_pos)++;
        }
    }
}

static bool apk_inflate_raw(const uint8_t *src, size_t src_size, uint8_t *dst, size_t dst_size) {
    apk_BitReader br = {src, src_size, 0, 0, 0};
    size_t out_pos = 0;
    bool final = false;
    while (!final) {
        uint32_t bit = 0, type = 0;
        if (!apk_br_bits(&br, 1, &bit) || !apk_br_bits(&br, 2, &type)) return false;
        final = bit != 0;
        if (type == 0) {
            apk_br_align(&br);
            if (br.pos + 4 > br.size) return false;
            const uint16_t len = apk_rd16(br.data + br.pos);
            const uint16_t nlen = apk_rd16(br.data + br.pos + 2);
            br.pos += 4;
            if ((uint16_t)~len != nlen || br.pos + len > br.size || len > dst_size - out_pos) return false;
            memcpy(dst + out_pos, br.data + br.pos, len);
            br.pos += len;
            out_pos += len;
        } else if (type == 1 || type == 2) {
            apk_Huffman litlen, dist;
            if (type == 1) {
                if (!apk_build_fixed(&litlen, &dist)) return false;
            } else if (!apk_build_dynamic(&br, &litlen, &dist)) {
                return false;
            }
            if (!apk_inflate_codes(&br, &litlen, &dist, dst, dst_size, &out_pos)) return false;
        } else {
            return false;
        }
    }
    return out_pos == dst_size;
}

/* ---- ZIP directory -------------------------------------------------------- */

static SsrnxResult apk_zip_open(const char *path, ZipDirectory *zip) {
    if (!path || !zip) return SSRNX_ERR_ARGUMENT;
    memset(zip, 0, sizeof(*zip));

    FILE *f = fopen(path, "rb");
    if (!f) return SSRNX_ERR_IO;
    /* Local headers and entry payloads are read through stdio; a larger
     * buffer turns header+name reads into one card request. */
    (void)setvbuf(f, NULL, _IOFBF, 64u * 1024u);
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return SSRNX_ERR_IO;
    }
    const long file_size = ftell(f);
    if (file_size < (long)ZIP_EOCD_MIN_SIZE) {
        fclose(f);
        return SSRNX_ERR_APK_INVALID;
    }

    const long search_len = file_size < (long)(ZIP_EOCD_MIN_SIZE + ZIP_MAX_COMMENT)
                          ? file_size
                          : (long)(ZIP_EOCD_MIN_SIZE + ZIP_MAX_COMMENT);
    if (fseek(f, file_size - search_len, SEEK_SET) != 0) {
        fclose(f);
        return SSRNX_ERR_IO;
    }

    uint8_t *tail = malloc((size_t)search_len);
    if (!tail) {
        fclose(f);
        return SSRNX_ERR_NO_MEMORY;
    }
    if (fread(tail, 1, (size_t)search_len, f) != (size_t)search_len) {
        free(tail);
        fclose(f);
        return SSRNX_ERR_IO;
    }

    long eocd = -1;
    for (long i = search_len - (long)ZIP_EOCD_MIN_SIZE; i >= 0; --i) {
        if (apk_rd32(tail + i) == ZIP_EOCD_SIG) {
            const uint16_t comment_len = apk_rd16(tail + i + 20);
            if (i + (long)ZIP_EOCD_MIN_SIZE + comment_len == search_len) {
                eocd = i;
                break;
            }
        }
    }
    if (eocd < 0) {
        free(tail);
        fclose(f);
        return SSRNX_ERR_APK_INVALID;
    }

    const uint16_t disk = apk_rd16(tail + eocd + 4);
    const uint16_t cd_disk = apk_rd16(tail + eocd + 6);
    const uint16_t disk_entries = apk_rd16(tail + eocd + 8);
    const uint16_t total_entries = apk_rd16(tail + eocd + 10);
    const uint32_t cd_size = apk_rd32(tail + eocd + 12);
    const uint32_t cd_offset = apk_rd32(tail + eocd + 16);
    free(tail);

    if (disk != 0 || cd_disk != 0 || disk_entries != total_entries ||
        total_entries == UINT16_MAX || cd_size == UINT32_MAX || cd_offset == UINT32_MAX) {
        fclose(f);
        return SSRNX_ERR_APK_UNSUPPORTED;
    }
    if ((uint64_t)cd_offset + cd_size > (uint64_t)file_size) {
        fclose(f);
        return SSRNX_ERR_APK_INVALID;
    }

    uint8_t *central = malloc(cd_size ? cd_size : 1u);
    if (!central) {
        fclose(f);
        return SSRNX_ERR_NO_MEMORY;
    }
    if (cd_size && (fseek(f, (long)cd_offset, SEEK_SET) != 0 || fread(central, 1, cd_size, f) != cd_size)) {
        free(central);
        fclose(f);
        return SSRNX_ERR_IO;
    }

    zip->file = f;
    zip->central_offset = cd_offset;
    zip->central_size = cd_size;
    zip->entry_count = total_entries;
    zip->file_size = file_size;
    zip->central = central;
    zip->central_cursor = 0;
    return SSRNX_OK;
}

static void apk_zip_close(ZipDirectory *zip) {
    if (zip && zip->file) fclose(zip->file);
    if (zip) free(zip->central);
    if (zip) memset(zip, 0, sizeof(*zip));
}

/* Restarts a central-directory walk (always succeeds; returns 0 so callers
 * keep the shape of the former fseek()). */
static int apk_zip_rewind_central(ZipDirectory *zip) {
    zip->central_cursor = 0;
    return 0;
}

/* ---- APK Signing Block metadata ------------------------------------------ */

#define APK_SIG_BLOCK_V2_ID  UINT32_C(0x7109871a)
#define APK_SIG_BLOCK_V3_ID  UINT32_C(0xf05368c0)
#define APK_SIG_BLOCK_MAX    (16u * 1024u * 1024u)

static const uint8_t apk_k_apk_sig_block_magic[16] = {
    'A','P','K',' ','S','i','g',' ','B','l','o','c','k',' ','4','2'
};

static bool apk_signing_take_lp32(const uint8_t **cursor, const uint8_t *end,
                              const uint8_t **out_data, size_t *out_size) {
    if (!cursor || !*cursor || !end || !out_data || !out_size ||
        (size_t)(end - *cursor) < 4u) return false;
    const uint32_t n = apk_rd32(*cursor);
    *cursor += 4;
    if ((size_t)(end - *cursor) < (size_t)n) return false;
    *out_data = *cursor;
    *out_size = (size_t)n;
    *cursor += n;
    return true;
}

void ssrnx_apk_signing_info_free(SsrnxApkSigningInfo *info) {
    if (!info) return;
    for (size_t i = 0; i < info->signer_count && i < SSRNX_APK_SIGNER_MAX; ++i) {
        free(info->signers[i].data);
        info->signers[i].data = NULL;
        info->signers[i].size = 0;
    }
    memset(info, 0, sizeof(*info));
}

static SsrnxResult apk_signing_parse_active_signers(const uint8_t *value, size_t value_size,
                                                int32_t scheme_version,
                                                SsrnxApkSigningInfo *out_info) {
    const uint8_t *cursor = value, *end = value + value_size;
    const uint8_t *signers = NULL;
    size_t signers_size = 0;
    if (!apk_signing_take_lp32(&cursor, end, &signers, &signers_size))
        return SSRNX_ERR_APK_INVALID;

    const uint8_t *sp = signers, *send = signers + signers_size;
    while (sp < send) {
        if (out_info->signer_count >= SSRNX_APK_SIGNER_MAX) {
            ssrnx_apk_signing_info_free(out_info);
            return SSRNX_ERR_APK_UNSUPPORTED;
        }
        const uint8_t *signer = NULL;
        size_t signer_size = 0;
        if (!apk_signing_take_lp32(&sp, send, &signer, &signer_size)) {
            ssrnx_apk_signing_info_free(out_info);
            return SSRNX_ERR_APK_INVALID;
        }

        const uint8_t *sc = signer, *se = signer + signer_size;
        const uint8_t *signed_data = NULL;
        size_t signed_data_size = 0;
        if (!apk_signing_take_lp32(&sc, se, &signed_data, &signed_data_size)) {
            ssrnx_apk_signing_info_free(out_info);
            return SSRNX_ERR_APK_INVALID;
        }

        /* v2 and v3 signed-data both begin with length-prefixed digests followed
         * by length-prefixed X.509 certificates. We only need the first
         * certificate in each signer chain: Android's PackageInfo signatures
         * array represents active signers, not their CA chains. */
        const uint8_t *dc = signed_data, *de = signed_data + signed_data_size;
        const uint8_t *digests = NULL, *certificates = NULL;
        size_t digests_size = 0, certificates_size = 0;
        if (!apk_signing_take_lp32(&dc, de, &digests, &digests_size) ||
            !apk_signing_take_lp32(&dc, de, &certificates, &certificates_size)) {
            ssrnx_apk_signing_info_free(out_info);
            return SSRNX_ERR_APK_INVALID;
        }
        (void)digests;
        (void)digests_size;

        const uint8_t *cc = certificates, *ce = certificates + certificates_size;
        const uint8_t *cert = NULL;
        size_t cert_size = 0;
        if (!apk_signing_take_lp32(&cc, ce, &cert, &cert_size) || cert_size == 0) {
            ssrnx_apk_signing_info_free(out_info);
            return SSRNX_ERR_APK_INVALID;
        }

        uint8_t *copy = malloc(cert_size);
        if (!copy) {
            ssrnx_apk_signing_info_free(out_info);
            return SSRNX_ERR_NO_MEMORY;
        }
        memcpy(copy, cert, cert_size);
        SsrnxApkSigningCertificate *dst = &out_info->signers[out_info->signer_count++];
        dst->data = copy;
        dst->size = cert_size;
    }

    if (out_info->signer_count == 0) return SSRNX_ERR_APK_INVALID;
    out_info->scheme_version = scheme_version;
    return SSRNX_OK;
}

SsrnxResult ssrnx_apk_read_signing_info(const char *apk_path,
                                        SsrnxApkSigningInfo *out_info) {
    if (!apk_path || !out_info) return SSRNX_ERR_ARGUMENT;
    memset(out_info, 0, sizeof(*out_info));

    ZipDirectory zip;
    SsrnxResult rc = apk_zip_open(apk_path, &zip);
    if (rc != SSRNX_OK) return rc;
    if (zip.central_offset < 32u) {
        apk_zip_close(&zip);
        return SSRNX_ERR_APK_UNSUPPORTED;
    }

    uint8_t footer[24];
    if (fseek(zip.file, (long)zip.central_offset - (long)sizeof(footer), SEEK_SET) != 0 ||
        fread(footer, 1, sizeof(footer), zip.file) != sizeof(footer)) {
        apk_zip_close(&zip);
        return SSRNX_ERR_IO;
    }
    if (memcmp(footer + 8, apk_k_apk_sig_block_magic, sizeof(apk_k_apk_sig_block_magic)) != 0) {
        apk_zip_close(&zip);
        return SSRNX_ERR_APK_UNSUPPORTED; /* v1-only/unsigned APK */
    }

    const uint64_t encoded_size = apk_rd64(footer);
    if (encoded_size < 24u || encoded_size > APK_SIG_BLOCK_MAX ||
        encoded_size + 8u > zip.central_offset) {
        apk_zip_close(&zip);
        return SSRNX_ERR_APK_INVALID;
    }
    const size_t block_size = (size_t)encoded_size + 8u;
    const long block_offset = (long)zip.central_offset - (long)block_size;
    uint8_t *block = malloc(block_size);
    if (!block) {
        apk_zip_close(&zip);
        return SSRNX_ERR_NO_MEMORY;
    }
    if (fseek(zip.file, block_offset, SEEK_SET) != 0 ||
        fread(block, 1, block_size, zip.file) != block_size) {
        free(block);
        apk_zip_close(&zip);
        return SSRNX_ERR_IO;
    }
    apk_zip_close(&zip);

    if (apk_rd64(block) != encoded_size ||
        memcmp(block + block_size - 16u, apk_k_apk_sig_block_magic,
               sizeof(apk_k_apk_sig_block_magic)) != 0 ||
        apk_rd64(block + block_size - 24u) != encoded_size) {
        free(block);
        return SSRNX_ERR_APK_INVALID;
    }

    const uint8_t *p = block + 8u;
    const uint8_t *pairs_end = block + block_size - 24u;
    const uint8_t *best = NULL;
    size_t best_size = 0;
    int best_priority = 0;
    int32_t scheme_version = 0;
    while (p < pairs_end) {
        if ((size_t)(pairs_end - p) < 8u) {
            free(block);
            return SSRNX_ERR_APK_INVALID;
        }
        const uint64_t pair_size64 = apk_rd64(p);
        p += 8u;
        if (pair_size64 < 4u || pair_size64 > (uint64_t)(pairs_end - p)) {
            free(block);
            return SSRNX_ERR_APK_INVALID;
        }
        const size_t pair_size = (size_t)pair_size64;
        const uint32_t id = apk_rd32(p);
        int priority = 0;
        int32_t version = 0;
        if (id == APK_SIG_BLOCK_V3_ID) { priority = 3; version = 3; }
        else if (id == APK_SIG_BLOCK_V2_ID) { priority = 2; version = 2; }
        if (priority > best_priority) {
            best_priority = priority;
            scheme_version = version;
            best = p + 4u;
            best_size = pair_size - 4u;
        }
        p += pair_size;
    }

    if (!best) {
        free(block);
        return SSRNX_ERR_APK_UNSUPPORTED;
    }
    rc = apk_signing_parse_active_signers(best, best_size, scheme_version, out_info);
    free(block);
    return rc;
}

static SsrnxResult apk_zip_read_central_entry(ZipDirectory *zip, apk_ZipEntryInfo *entry) {
    if (!zip->central || zip->central_cursor > zip->central_size ||
        zip->central_size - zip->central_cursor < 46u) return SSRNX_ERR_APK_INVALID;
    const uint8_t *hdr = zip->central + zip->central_cursor;
    if (apk_rd32(hdr) != ZIP_CENTRAL_SIG) return SSRNX_ERR_APK_INVALID;

    const uint16_t name_len = apk_rd16(hdr + 28);
    const uint16_t extra_len = apk_rd16(hdr + 30);
    const uint16_t comment_len = apk_rd16(hdr + 32);
    if (name_len == 0 || name_len >= sizeof(entry->name)) return SSRNX_ERR_APK_UNSUPPORTED;

    memset(entry, 0, sizeof(*entry));
    entry->flags = apk_rd16(hdr + 8);
    entry->method = apk_rd16(hdr + 10);
    entry->dos_time = apk_rd32(hdr + 12);
    entry->crc32 = apk_rd32(hdr + 16);
    entry->compressed_size = apk_rd32(hdr + 20);
    entry->uncompressed_size = apk_rd32(hdr + 24);
    entry->local_offset = apk_rd32(hdr + 42);

    if (entry->compressed_size == UINT32_MAX || entry->uncompressed_size == UINT32_MAX ||
        entry->local_offset == UINT32_MAX) {
        return SSRNX_ERR_APK_UNSUPPORTED;
    }
    const size_t record = 46u + (size_t)name_len + extra_len + comment_len;
    if (zip->central_size - zip->central_cursor < record) return SSRNX_ERR_APK_INVALID;
    memcpy(entry->name, hdr + 46, name_len);
    entry->name[name_len] = '\0';
    zip->central_cursor += record;
    return SSRNX_OK;
}

static SsrnxResult apk_zip_find_entry(ZipDirectory *zip, const char *name, apk_ZipEntryInfo *out_entry) {
    if (!zip || !zip->file || !name || !out_entry) return SSRNX_ERR_ARGUMENT;
    (void)apk_zip_rewind_central(zip);

    for (uint16_t i = 0; i < zip->entry_count; ++i) {
        apk_ZipEntryInfo e;
        SsrnxResult rc = apk_zip_read_central_entry(zip, &e);
        if (rc != SSRNX_OK) return rc;
        if (strcmp(e.name, name) == 0) {
            *out_entry = e;
            return SSRNX_OK;
        }
    }
    return SSRNX_ERR_DEPENDENCY_NOT_FOUND;
}

static void apk_zip_entry_public(const apk_ZipEntryInfo *entry, SsrnxZipEntryInfo *out) {
    memset(out, 0, sizeof(*out));
    memcpy(out->name, entry->name, sizeof(out->name));
    out->flags = entry->flags;
    out->method = entry->method;
    out->crc32 = entry->crc32;
    out->compressed_size = entry->compressed_size;
    out->uncompressed_size = entry->uncompressed_size;
    out->dos_time = entry->dos_time;
}

SsrnxResult ssrnx_zip_visit(const char *zip_path, SsrnxZipEntryVisitor visitor, void *userdata) {
    if (!zip_path || !visitor) return SSRNX_ERR_ARGUMENT;
    ZipDirectory zip;
    SsrnxResult rc = apk_zip_open(zip_path, &zip);
    if (rc != SSRNX_OK) return rc;
    if (apk_zip_rewind_central(&zip) != 0) {
        apk_zip_close(&zip);
        return SSRNX_ERR_IO;
    }
    for (uint16_t i = 0; i < zip.entry_count; ++i) {
        apk_ZipEntryInfo entry;
        rc = apk_zip_read_central_entry(&zip, &entry);
        if (rc != SSRNX_OK) {
            apk_zip_close(&zip);
            return rc;
        }
        SsrnxZipEntryInfo pub;
        apk_zip_entry_public(&entry, &pub);
        if (!visitor(&pub, userdata)) break;
    }
    apk_zip_close(&zip);
    return SSRNX_OK;
}

SsrnxResult ssrnx_zip_has_entry(const char *zip_path, const char *entry_name, bool *out_present) {
    if (!zip_path || !entry_name || !out_present) return SSRNX_ERR_ARGUMENT;
    *out_present = false;
    ZipDirectory zip;
    SsrnxResult rc = apk_zip_open(zip_path, &zip);
    if (rc != SSRNX_OK) return rc;
    apk_ZipEntryInfo entry;
    rc = apk_zip_find_entry(&zip, entry_name, &entry);
    apk_zip_close(&zip);
    if (rc == SSRNX_OK) {
        *out_present = true;
        return SSRNX_OK;
    }
    if (rc == SSRNX_ERR_DEPENDENCY_NOT_FOUND || rc == SSRNX_ERR_SYMBOL_NOT_FOUND) return SSRNX_OK;
    return rc;
}

static SsrnxResult apk_zip_seek_entry_data(ZipDirectory *zip, const apk_ZipEntryInfo *entry,
                                      uint64_t *out_data_offset) {
    if (!zip || !zip->file || !entry) return SSRNX_ERR_ARGUMENT;
    if (entry->flags & 1u) return SSRNX_ERR_APK_UNSUPPORTED; /* encryption */
    if (entry->method != ZIP_METHOD_STORE && entry->method != ZIP_METHOD_DEFLATE && entry->method != 14u) {
        return SSRNX_ERR_APK_UNSUPPORTED;
    }
    if ((uint64_t)entry->local_offset + 30u > (uint64_t)zip->file_size) return SSRNX_ERR_APK_INVALID;
    if (fseek(zip->file, (long)entry->local_offset, SEEK_SET) != 0) return SSRNX_ERR_IO;

    uint8_t local[30];
    if (fread(local, 1, sizeof(local), zip->file) != sizeof(local) || apk_rd32(local) != ZIP_LOCAL_SIG) {
        return SSRNX_ERR_APK_INVALID;
    }
    const uint16_t name_len = apk_rd16(local + 26);
    const uint16_t extra_len = apk_rd16(local + 28);
    const uint64_t data_offset = (uint64_t)entry->local_offset + 30u + name_len + extra_len;
    if (data_offset + entry->compressed_size > (uint64_t)zip->file_size) return SSRNX_ERR_APK_INVALID;
    if (data_offset > LONG_MAX) return SSRNX_ERR_APK_UNSUPPORTED;
    if (fseek(zip->file, (long)data_offset, SEEK_SET) != 0) return SSRNX_ERR_IO;
    if (out_data_offset) *out_data_offset = data_offset;
    return SSRNX_OK;
}

/* ZIP method 14 carries LZMA1 properties before its raw stream (APPNOTE 5.8). */
static bool apk_zip_lzma_decode(const uint8_t *input, size_t input_size, uint8_t *output, size_t output_size) {
    if (!input || input_size < 9u || apk_rd16(input + 2) != 5u) return false;
    lzma_filter filters[] = {{LZMA_FILTER_LZMA1, NULL}, {LZMA_VLI_UNKNOWN, NULL}};
    if (lzma_properties_decode(&filters[0], NULL, input + 4, 5) != LZMA_OK) return false;
    const lzma_options_lzma *options = filters[0].options;
    size_t in_pos = 9, out_pos = 0;
    bool valid = options->dict_size <= 16u * 1024u * 1024u &&
        lzma_raw_buffer_decode(filters, NULL, input, &in_pos, input_size, output, &out_pos, output_size) == LZMA_OK &&
        in_pos == input_size && out_pos == output_size;
    lzma_filters_free(filters, NULL);
    return valid;
}

static SsrnxResult apk_zip_read_entry_data(ZipDirectory *zip, const apk_ZipEntryInfo *entry,
                                       uint8_t **out_data, size_t *out_size) {
    if (!zip || !zip->file || !entry || !out_data || !out_size) return SSRNX_ERR_ARGUMENT;
    *out_data = NULL;
    *out_size = 0;

    SsrnxResult rc = apk_zip_seek_entry_data(zip, entry, NULL);
    if (rc != SSRNX_OK) return rc;

    const size_t allocation = entry->uncompressed_size ? entry->uncompressed_size : 1u;
    uint8_t *data = malloc(allocation);
    if (!data) return SSRNX_ERR_NO_MEMORY;

    bool ok = false;
    if (entry->method == ZIP_METHOD_STORE) {
        ok = entry->compressed_size == entry->uncompressed_size;
        if (ok && entry->uncompressed_size &&
            fread(data, 1, entry->uncompressed_size, zip->file) != entry->uncompressed_size) {
            free(data);
            return SSRNX_ERR_IO;
        }
    } else {
        uint8_t *compressed = NULL;
        if (entry->compressed_size) {
            compressed = malloc(entry->compressed_size);
            if (!compressed) { free(data); return SSRNX_ERR_NO_MEMORY; }
            if (fread(compressed, 1, entry->compressed_size, zip->file) != entry->compressed_size) {
                free(compressed);
                free(data);
                return SSRNX_ERR_IO;
            }
        }
        ok = entry->method == 14u ?
            apk_zip_lzma_decode(compressed, entry->compressed_size, data, entry->uncompressed_size) :
            apk_inflate_raw(compressed, entry->compressed_size, data, entry->uncompressed_size);
        free(compressed);
    }

    if (!ok || apk_crc32_bytes(data, entry->uncompressed_size) != entry->crc32) {
        free(data);
        return SSRNX_ERR_APK_INVALID;
    }

    *out_data = data;
    *out_size = entry->uncompressed_size;
    return SSRNX_OK;
}

static SsrnxResult apk_zip_extract_store_stream(ZipDirectory *zip, const apk_ZipEntryInfo *entry,
                                            const char *output_path) {
    if (!zip || !entry || !output_path) return SSRNX_ERR_ARGUMENT;
    if (entry->method != ZIP_METHOD_STORE || entry->compressed_size != entry->uncompressed_size) {
        return SSRNX_ERR_APK_INVALID;
    }
    SsrnxResult rc = apk_zip_seek_entry_data(zip, entry, NULL);
    if (rc != SSRNX_OK) return rc;

    FILE *out = fopen(output_path, "wb");
    if (!out) return SSRNX_ERR_IO;
    uint8_t *buffer = malloc(ZIP_STREAM_CHUNK);
    if (!buffer) {
        fclose(out);
        (void)unlink(output_path);
        return SSRNX_ERR_NO_MEMORY;
    }

    uint32_t crc = UINT32_C(0xffffffff);
    uint32_t remaining = entry->uncompressed_size;
    bool ok = true;
    while (remaining) {
        const size_t chunk = remaining < ZIP_STREAM_CHUNK ? remaining : ZIP_STREAM_CHUNK;
        if (fread(buffer, 1, chunk, zip->file) != chunk) { ok = false; break; }
        crc = apk_crc32_update(crc, buffer, chunk);
        if (fwrite(buffer, 1, chunk, out) != chunk) { ok = false; break; }
        remaining -= (uint32_t)chunk;
    }
    free(buffer);
    if (fclose(out) != 0) ok = false;
    if (!ok) {
        (void)unlink(output_path);
        return SSRNX_ERR_IO;
    }
    if (~crc != entry->crc32) {
        (void)unlink(output_path);
        return SSRNX_ERR_APK_INVALID;
    }
    return SSRNX_OK;
}

/* ---- Android binary XML --------------------------------------------------- */

static bool apk_axml_decode_len8(const uint8_t **pp, const uint8_t *end, uint32_t *out) {
    if (!pp || !*pp || !out || *pp >= end) return false;
    const uint8_t first = *(*pp)++;
    if (first & 0x80u) {
        if (*pp >= end) return false;
        *out = ((uint32_t)(first & 0x7fu) << 8) | *(*pp)++;
    } else {
        *out = first;
    }
    return true;
}

static bool apk_axml_decode_len16(const uint8_t **pp, const uint8_t *end, uint32_t *out) {
    if (!pp || !*pp || !out || (size_t)(end - *pp) < 2) return false;
    uint16_t first = apk_rd16(*pp);
    *pp += 2;
    if (first & 0x8000u) {
        if ((size_t)(end - *pp) < 2) return false;
        const uint16_t second = apk_rd16(*pp);
        *pp += 2;
        *out = ((uint32_t)(first & 0x7fffu) << 16) | second;
    } else {
        *out = first;
    }
    return true;
}

static bool apk_utf8_append_codepoint(char *out, size_t cap, size_t *pos, uint32_t cp) {
    if (!out || !pos || *pos >= cap) return false;
    if (cp <= 0x7f) {
        if (*pos + 1 >= cap) return false;
        out[(*pos)++] = (char)cp;
    } else if (cp <= 0x7ff) {
        if (*pos + 2 >= cap) return false;
        out[(*pos)++] = (char)(0xc0u | (cp >> 6));
        out[(*pos)++] = (char)(0x80u | (cp & 0x3fu));
    } else if (cp <= 0xffff) {
        if (*pos + 3 >= cap) return false;
        out[(*pos)++] = (char)(0xe0u | (cp >> 12));
        out[(*pos)++] = (char)(0x80u | ((cp >> 6) & 0x3fu));
        out[(*pos)++] = (char)(0x80u | (cp & 0x3fu));
    } else if (cp <= 0x10ffff) {
        if (*pos + 4 >= cap) return false;
        out[(*pos)++] = (char)(0xf0u | (cp >> 18));
        out[(*pos)++] = (char)(0x80u | ((cp >> 12) & 0x3fu));
        out[(*pos)++] = (char)(0x80u | ((cp >> 6) & 0x3fu));
        out[(*pos)++] = (char)(0x80u | (cp & 0x3fu));
    } else {
        return false;
    }
    return true;
}

static bool apk_axml_pool_get(const apk_AxmlStringPool *pool, uint32_t index, char *out, size_t cap) {
    if (!pool || !out || cap == 0 || index >= pool->string_count) return false;
    out[0] = '\0';
    const uint32_t rel = apk_rd32(pool->offsets + (size_t)index * 4u);
    if (rel >= pool->chunk_size - pool->strings_start) return false;

    const uint8_t *p = pool->strings + rel;
    const uint8_t *end = pool->chunk + pool->chunk_size;
    size_t w = 0;

    if (pool->flags & RES_STRING_POOL_UTF8_FLAG) {
        uint32_t utf16_len = 0, byte_len = 0;
        if (!apk_axml_decode_len8(&p, end, &utf16_len) || !apk_axml_decode_len8(&p, end, &byte_len)) return false;
        (void)utf16_len;
        if ((size_t)(end - p) < (size_t)byte_len + 1u || byte_len >= cap) return false;
        memcpy(out, p, byte_len);
        out[byte_len] = '\0';
        return p[byte_len] == 0;
    }

    uint32_t units = 0;
    if (!apk_axml_decode_len16(&p, end, &units)) return false;
    for (uint32_t i = 0; i < units; ++i) {
        if ((size_t)(end - p) < 2) return false;
        uint32_t cp = apk_rd16(p);
        p += 2;
        if (cp >= 0xd800u && cp <= 0xdbffu) {
            if (++i >= units || (size_t)(end - p) < 2) return false;
            const uint32_t low = apk_rd16(p);
            p += 2;
            if (low < 0xdc00u || low > 0xdfffu) return false;
            cp = UINT32_C(0x10000) + ((cp - 0xd800u) << 10) + (low - 0xdc00u);
        }
        if (!apk_utf8_append_codepoint(out, cap, &w, cp)) return false;
    }
    if ((size_t)(end - p) < 2 || apk_rd16(p) != 0 || w >= cap) return false;
    out[w] = '\0';
    return true;
}

static bool apk_axml_pool_init(apk_AxmlStringPool *pool, const uint8_t *chunk, size_t available) {
    if (!pool || !chunk || available < 28 || apk_rd16(chunk) != RES_STRING_POOL_TYPE) return false;
    const uint16_t header_size = apk_rd16(chunk + 2);
    const uint32_t chunk_size = apk_rd32(chunk + 4);
    if (header_size < 28 || chunk_size < header_size || chunk_size > available) return false;

    const uint32_t string_count = apk_rd32(chunk + 8);
    const uint32_t style_count = apk_rd32(chunk + 12);
    const uint32_t flags = apk_rd32(chunk + 16);
    const uint32_t strings_start = apk_rd32(chunk + 20);
    const uint32_t styles_start = apk_rd32(chunk + 24);
    (void)styles_start;

    const uint64_t offset_bytes = (uint64_t)(string_count + style_count) * 4u;
    if ((uint64_t)header_size + offset_bytes > chunk_size || strings_start >= chunk_size) return false;

    memset(pool, 0, sizeof(*pool));
    pool->chunk = chunk;
    pool->chunk_size = chunk_size;
    pool->offsets = chunk + header_size;
    pool->strings = chunk + strings_start;
    pool->string_count = string_count;
    pool->flags = flags;
    pool->strings_start = strings_start;
    return true;
}

static bool apk_axml_attr_name(const apk_AxmlStringPool *pool, const uint8_t *attr, char *out, size_t cap) {
    return apk_axml_pool_get(pool, apk_rd32(attr + 4), out, cap);
}

static bool apk_axml_attr_value(const apk_AxmlStringPool *pool, const uint8_t *attr, char *out, size_t cap) {
    if (!pool || !attr || !out || cap == 0) return false;
    const uint32_t raw = apk_rd32(attr + 8);
    if (raw != RES_NO_ENTRY) return apk_axml_pool_get(pool, raw, out, cap);

    const uint16_t value_size = apk_rd16(attr + 12);
    const uint8_t data_type = attr[15];
    const uint32_t data = apk_rd32(attr + 16);
    if (value_size < 8) return false;
    if (data_type == RES_VALUE_TYPE_STRING) return apk_axml_pool_get(pool, data, out, cap);
    if (data_type == RES_VALUE_TYPE_INT_DEC || data_type == RES_VALUE_TYPE_INT_HEX ||
        data_type == RES_VALUE_TYPE_INT_BOOLEAN) {
        const int n = snprintf(out, cap, "%u", data);
        return n > 0 && (size_t)n < cap;
    }
    out[0] = '\0';
    return false;
}

static const uint8_t *apk_axml_find_attr(const apk_AxmlStringPool *pool, const uint8_t *attrs,
                                     uint16_t attr_size, uint16_t attr_count,
                                     const char *wanted) {
    if (!pool || !attrs || attr_size < 20 || !wanted) return NULL;
    for (uint16_t i = 0; i < attr_count; ++i) {
        const uint8_t *attr = attrs + (size_t)i * attr_size;
        char name[128];
        if (apk_axml_attr_name(pool, attr, name, sizeof(name)) && strcmp(name, wanted) == 0) {
            return attr;
        }
    }
    return NULL;
}

static bool apk_axml_copy_attr_string(const apk_AxmlStringPool *pool, const uint8_t *attrs,
                                  uint16_t attr_size, uint16_t attr_count,
                                  const char *name, char *out, size_t cap) {
    const uint8_t *attr = apk_axml_find_attr(pool, attrs, attr_size, attr_count, name);
    return attr && apk_axml_attr_value(pool, attr, out, cap);
}

static bool apk_axml_attr_u32(const apk_AxmlStringPool *pool, const uint8_t *attrs,
                          uint16_t attr_size, uint16_t attr_count,
                          const char *name, uint32_t *out) {
    const uint8_t *attr = apk_axml_find_attr(pool, attrs, attr_size, attr_count, name);
    if (!attr || !out) return false;
    const uint8_t data_type = attr[15];
    if (data_type == RES_VALUE_TYPE_INT_DEC || data_type == RES_VALUE_TYPE_INT_HEX ||
        data_type == RES_VALUE_TYPE_INT_BOOLEAN) {
        *out = apk_rd32(attr + 16);
        return true;
    }
    char value[64];
    if (!apk_axml_attr_value(pool, attr, value, sizeof(value))) return false;
    char *end = NULL;
    errno = 0;
    const unsigned long v = strtoul(value, &end, 0);
    if (errno || !end || *end || v > UINT32_MAX) return false;
    *out = (uint32_t)v;
    return true;
}


static bool apk_axml_attr_bool(const apk_AxmlStringPool *pool, const uint8_t *attrs,
                           uint16_t attr_size, uint16_t attr_count,
                           const char *name, bool *out) {
    if (!out) return false;
    const uint8_t *attr = apk_axml_find_attr(pool, attrs, attr_size, attr_count, name);
    if (!attr) return false;
    const uint8_t data_type = attr[15];
    if (data_type == RES_VALUE_TYPE_INT_BOOLEAN ||
        data_type == RES_VALUE_TYPE_INT_DEC ||
        data_type == RES_VALUE_TYPE_INT_HEX) {
        *out = apk_rd32(attr + 16) != 0;
        return true;
    }
    char value[32];
    if (!apk_axml_attr_value(pool, attr, value, sizeof(value))) return false;
    if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) { *out = true; return true; }
    if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) { *out = false; return true; }
    return false;
}

static bool apk_axml_attr_resource_id(const apk_AxmlStringPool *pool, const uint8_t *attrs,
                                  uint16_t attr_size, uint16_t attr_count,
                                  const char *name, uint32_t *out) {
    const uint8_t *attr = apk_axml_find_attr(pool, attrs, attr_size, attr_count, name);
    if (!attr || !out || apk_rd16(attr + 12) < 8) return false;
    const uint8_t data_type = attr[15];
    if (data_type != RES_VALUE_TYPE_REFERENCE && data_type != RES_VALUE_TYPE_DYNAMIC_REFERENCE) return false;
    const uint32_t value = apk_rd32(attr + 16);
    if (value == 0) return false;
    *out = value;
    return true;
}

static bool apk_is_native_activity_name(const char *name) {
    if (!name) return false;
    return strcmp(name, "android.app.NativeActivity") == 0 ||
           strcmp(name, "android/app/NativeActivity") == 0;
}

static bool apk_screen_orientation_from_string(const char *value, int32_t *out) {
    if (!value || !out) return false;
    struct apk_Pair { const char *name; int32_t value; };
    static const struct apk_Pair map[] = {
        {"unspecified", SSRNX_SCREEN_ORIENTATION_UNSPECIFIED},
        {"landscape", SSRNX_SCREEN_ORIENTATION_LANDSCAPE},
        {"portrait", SSRNX_SCREEN_ORIENTATION_PORTRAIT},
        {"user", SSRNX_SCREEN_ORIENTATION_USER},
        {"behind", SSRNX_SCREEN_ORIENTATION_BEHIND},
        {"sensor", SSRNX_SCREEN_ORIENTATION_SENSOR},
        {"nosensor", SSRNX_SCREEN_ORIENTATION_NOSENSOR},
        {"sensorLandscape", SSRNX_SCREEN_ORIENTATION_SENSOR_LANDSCAPE},
        {"sensorPortrait", SSRNX_SCREEN_ORIENTATION_SENSOR_PORTRAIT},
        {"reverseLandscape", SSRNX_SCREEN_ORIENTATION_REVERSE_LANDSCAPE},
        {"reversePortrait", SSRNX_SCREEN_ORIENTATION_REVERSE_PORTRAIT},
        {"fullSensor", SSRNX_SCREEN_ORIENTATION_FULL_SENSOR},
        {"userLandscape", SSRNX_SCREEN_ORIENTATION_USER_LANDSCAPE},
        {"userPortrait", SSRNX_SCREEN_ORIENTATION_USER_PORTRAIT},
        {"fullUser", SSRNX_SCREEN_ORIENTATION_FULL_USER},
        {"locked", SSRNX_SCREEN_ORIENTATION_LOCKED},
    };
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); ++i) {
        if (strcmp(value, map[i].name) == 0) {
            *out = map[i].value;
            return true;
        }
    }
    char *end = NULL;
    errno = 0;
    long parsed = strtol(value, &end, 0);
    if (!errno && end && *end == '\0' && parsed >= INT32_MIN && parsed <= INT32_MAX) {
        *out = (int32_t)parsed;
        return true;
    }
    return false;
}

static bool apk_axml_attr_screen_orientation(const apk_AxmlStringPool *pool, const uint8_t *attrs,
                                         uint16_t attr_size, uint16_t attr_count,
                                         int32_t *out) {
    const uint8_t *attr = apk_axml_find_attr(pool, attrs, attr_size, attr_count, "screenOrientation");
    if (!attr || !out) return false;
    const uint8_t data_type = attr[15];
    if (data_type == RES_VALUE_TYPE_INT_DEC || data_type == RES_VALUE_TYPE_INT_HEX) {
        *out = (int32_t)apk_rd32(attr + 16);
        return true;
    }
    char value[64];
    if (!apk_axml_attr_value(pool, attr, value, sizeof(value))) return false;
    return apk_screen_orientation_from_string(value, out);
}

static SsrnxResult apk_parse_binary_manifest(const uint8_t *data, size_t size, SsrnxApkInfo *info) {
    if (!data || !info || size < 8 || apk_rd16(data) != RES_XML_TYPE) return SSRNX_ERR_MANIFEST_INVALID;
    const uint16_t root_header = apk_rd16(data + 2);
    const uint32_t root_size = apk_rd32(data + 4);
    if (root_header < 8 || root_size > size || root_size < root_header) return SSRNX_ERR_MANIFEST_INVALID;

    apk_AxmlStringPool pool;
    memset(&pool, 0, sizeof(pool));
    bool have_pool = false;
    bool selected_activity_open = false;
    int depth = 0;
    int selected_activity_depth = -1;

    /* Track launcher intent filters for ordinary Java activities as well as
     * NativeActivity. Android launches the component carrying MAIN+LAUNCHER;
     * its ActivityInfo.screenOrientation is therefore the orientation contract
     * the guest framework must expose. */
    bool activity_open = false;
    int activity_depth = -1;
    char activity_class[SSRNX_APK_ACTIVITY_CAP] = {0};
    char activity_target[SSRNX_APK_ACTIVITY_CAP] = {0};
    int32_t activity_orientation = SSRNX_SCREEN_ORIENTATION_UNSPECIFIED;
    bool activity_orientation_declared = false;
    bool activity_is_alias = false;
    bool activity_enabled = true;
    bool activity_is_launcher = false;
    bool launcher_selected = false;
    bool intent_filter_open = false;
    int intent_filter_depth = -1;
    bool intent_filter_main = false;
    bool intent_filter_launcher = false;

    size_t off = root_header;
    while (off + 8 <= root_size) {
        const uint8_t *chunk = data + off;
        const uint16_t type = apk_rd16(chunk);
        const uint16_t header_size = apk_rd16(chunk + 2);
        const uint32_t chunk_size = apk_rd32(chunk + 4);
        if (header_size < 8 || chunk_size < header_size || off + chunk_size > root_size) {
            return SSRNX_ERR_MANIFEST_INVALID;
        }

        if (type == RES_STRING_POOL_TYPE) {
            if (!apk_axml_pool_init(&pool, chunk, root_size - off)) return SSRNX_ERR_MANIFEST_INVALID;
            have_pool = true;
        } else if (type == RES_XML_START_ELEMENT_TYPE) {
            if (!have_pool || chunk_size < 36) return SSRNX_ERR_MANIFEST_INVALID;
            const uint8_t *ext = chunk + 16;
            const uint32_t name_index = apk_rd32(ext + 4);
            const uint16_t attr_start = apk_rd16(ext + 8);
            const uint16_t attr_size = apk_rd16(ext + 10);
            const uint16_t attr_count = apk_rd16(ext + 12);
            const size_t attrs_off = 16u + attr_start;
            if (attr_size < 20 || attrs_off > chunk_size ||
                (uint64_t)attrs_off + (uint64_t)attr_size * attr_count > chunk_size) {
                return SSRNX_ERR_MANIFEST_INVALID;
            }

            char tag[128];
            if (!apk_axml_pool_get(&pool, name_index, tag, sizeof(tag))) return SSRNX_ERR_MANIFEST_INVALID;
            const uint8_t *attrs = chunk + attrs_off;
            ++depth;

            if (strcmp(tag, "manifest") == 0) {
                (void)apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count,
                                            "package", info->package_name, sizeof(info->package_name));
                (void)apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count,
                                            "versionName", info->version_name, sizeof(info->version_name));
                (void)apk_axml_attr_u32(&pool, attrs, attr_size, attr_count,
                                    "versionCode", &info->version_code);
                if (apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count,
                                          "split", info->split_name, sizeof(info->split_name))) {
                    info->split_apk = info->split_name[0] != '\0';
                }
            } else if (strcmp(tag, "application") == 0) {
                /* Preserve the manifest Application class declaration. Android
                 * expands relative/unqualified names against the package when
                 * creating LoadedApk.makeApplication(); SSRNX performs the same
                 * expansion in the launcher once the package is known. */
                (void)apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count,
                                            "name", info->application_name,
                                            sizeof(info->application_name));
                /* Literal labels are immediately useful to the launcher. Resource
                 * labels still fall back to filename/package metadata, but icon
                 * references are retained so launcher decoding can resolve the
                 * exact resource through resources.arsc. */
                (void)apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count,
                                            "label", info->application_label,
                                            sizeof(info->application_label));
                (void)apk_axml_attr_resource_id(&pool, attrs, attr_size, attr_count,
                                            "label", &info->application_label_resource);
                (void)apk_axml_attr_resource_id(&pool, attrs, attr_size, attr_count,
                                            "icon", &info->application_icon_resource);
                (void)apk_axml_attr_resource_id(&pool, attrs, attr_size, attr_count,
                                            "roundIcon", &info->application_round_icon_resource);
                (void)apk_axml_attr_resource_id(&pool, attrs, attr_size, attr_count,
                                            "theme", &info->application_theme_resource);
            } else if (strcmp(tag, "uses-sdk") == 0) {
                (void)apk_axml_attr_u32(&pool, attrs, attr_size, attr_count,
                                    "minSdkVersion", &info->min_sdk);
                (void)apk_axml_attr_u32(&pool, attrs, attr_size, attr_count,
                                    "targetSdkVersion", &info->target_sdk);
            } else if (strcmp(tag, "provider") == 0) {
                char provider_name[SSRNX_APK_ACTIVITY_CAP] = {0};
                if (apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count,
                                          "name", provider_name, sizeof(provider_name)) &&
                    info->provider_count < SSRNX_APK_PROVIDER_MAX) {
                    SsrnxManifestProvider *provider = &info->providers[info->provider_count];
                    memset(provider, 0, sizeof(*provider));
                    provider->enabled = true;
                    (void)apk_copy_string(provider->class_name, sizeof(provider->class_name), provider_name);
                    (void)apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count,
                                                "process", provider->process_name,
                                                sizeof(provider->process_name));
                    (void)apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count,
                                                "authorities", provider->authorities,
                                                sizeof(provider->authorities));
                    uint32_t init_order = 0;
                    if (apk_axml_attr_u32(&pool, attrs, attr_size, attr_count,
                                      "initOrder", &init_order))
                        provider->init_order = (int32_t)init_order;
                    (void)apk_axml_attr_bool(&pool, attrs, attr_size, attr_count,
                                         "enabled", &provider->enabled);
                    provider->exported_declared = apk_axml_attr_bool(&pool, attrs, attr_size, attr_count,
                                                                  "exported", &provider->exported);
                    ++info->provider_count;
                }
            } else if (strcmp(tag, "activity") == 0 || strcmp(tag, "activity-alias") == 0) {
                char activity[SSRNX_APK_ACTIVITY_CAP] = {0};
                if (apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count,
                                          "name", activity, sizeof(activity))) {
                    /* Preserve a fallback for malformed/unusual manifests, then
                     * override it once a real MAIN+LAUNCHER component is found. */
                    if (!info->activity_name[0]) {
                        (void)apk_copy_string(info->activity_name, sizeof(info->activity_name), activity);
                    }

                    activity_open = true;
                    activity_depth = depth;
                    activity_is_alias = strcmp(tag, "activity-alias") == 0;
                    activity_enabled = true;
                    (void)apk_axml_attr_bool(&pool, attrs, attr_size, attr_count,
                                         "enabled", &activity_enabled);
                    activity_is_launcher = false;
                    intent_filter_open = false;
                    intent_filter_depth = -1;
                    intent_filter_main = false;
                    intent_filter_launcher = false;
                    activity_target[0] = '\0';
                    (void)apk_copy_string(activity_class, sizeof(activity_class), activity);
                    if (activity_is_alias) {
                        (void)apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count,
                                                    "targetActivity", activity_target,
                                                    sizeof(activity_target));
                    }
                    activity_orientation = SSRNX_SCREEN_ORIENTATION_UNSPECIFIED;
                    activity_orientation_declared =
                        apk_axml_attr_screen_orientation(&pool, attrs, attr_size, attr_count,
                                                     &activity_orientation);

                    if (apk_is_native_activity_name(activity)) {
                        if (!launcher_selected) {
                            (void)apk_copy_string(info->activity_name, sizeof(info->activity_name), activity);
                            info->native_activity = true;
                            if (activity_orientation_declared) {
                                info->screen_orientation = activity_orientation;
                                info->screen_orientation_declared = true;
                            }
                        }
                        selected_activity_open = true;
                        selected_activity_depth = depth;
                    }
                }
            } else if (strcmp(tag, "intent-filter") == 0 && activity_open) {
                intent_filter_open = true;
                intent_filter_depth = depth;
                intent_filter_main = false;
                intent_filter_launcher = false;
            } else if (strcmp(tag, "action") == 0 && activity_open && intent_filter_open) {
                char action_name[128] = {0};
                if (apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count,
                                          "name", action_name, sizeof(action_name)) &&
                    strcmp(action_name, "android.intent.action.MAIN") == 0) {
                    intent_filter_main = true;
                }
            } else if (strcmp(tag, "category") == 0 && activity_open && intent_filter_open) {
                char category_name[128] = {0};
                if (apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count,
                                          "name", category_name, sizeof(category_name)) &&
                    strcmp(category_name, "android.intent.category.LAUNCHER") == 0) {
                    intent_filter_launcher = true;
                }
            } else if (strcmp(tag, "meta-data") == 0 && selected_activity_open) {
                char meta_name[192];
                if (apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count,
                                          "name", meta_name, sizeof(meta_name)) &&
                    strcmp(meta_name, "android.app.lib_name") == 0) {
                    (void)apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count,
                                                "value", info->native_lib_name,
                                                sizeof(info->native_lib_name));
                }
            }
        } else if (type == RES_XML_END_ELEMENT_TYPE) {
            if (intent_filter_open && depth == intent_filter_depth) {
                if (activity_enabled && intent_filter_main && intent_filter_launcher)
                    activity_is_launcher = true;
                intent_filter_open = false;
                intent_filter_depth = -1;
                intent_filter_main = false;
                intent_filter_launcher = false;
            }
            if (activity_open && depth == activity_depth) {
                if (activity_is_launcher && !launcher_selected) {
                    const char *launch_class =
                        (activity_is_alias && activity_target[0]) ? activity_target : activity_class;
                    (void)apk_copy_string(info->activity_name, sizeof(info->activity_name), launch_class);
                    launcher_selected = true;
                    info->native_activity = apk_is_native_activity_name(launch_class);
                    if (activity_orientation_declared) {
                        info->screen_orientation = activity_orientation;
                        info->screen_orientation_declared = true;
                    } else if (!info->native_activity) {
                        /* Do not leak an orientation captured from an unrelated
                         * NativeActivity/fallback component into this launcher. */
                        info->screen_orientation = SSRNX_SCREEN_ORIENTATION_UNSPECIFIED;
                        info->screen_orientation_declared = false;
                    }
                }
                activity_open = false;
                activity_depth = -1;
                activity_class[0] = '\0';
                activity_target[0] = '\0';
                activity_orientation = SSRNX_SCREEN_ORIENTATION_UNSPECIFIED;
                activity_orientation_declared = false;
                activity_is_alias = false;
                activity_enabled = true;
                activity_is_launcher = false;
            }
            if (selected_activity_open && depth == selected_activity_depth) {
                selected_activity_open = false;
                selected_activity_depth = -1;
            }
            if (depth > 0) --depth;
        }

        off += chunk_size;
    }

    info->manifest_binary_xml = true;
    /* A valid Android APK does not have to use NativeActivity.  The launcher
     * must be able to inventory ordinary Java/engine APKs even though the
     * current runtime can only execute the NativeActivity subset.  Treat only
     * a missing package name as a malformed manifest here; launchability is
     * classified later by ssrnx_apk_probe(). */
    if (!info->package_name[0]) return SSRNX_ERR_MANIFEST_INVALID;
    return SSRNX_OK;
}

static void apk_make_native_entry_name(SsrnxApkInfo *info) {
    if (!info || !info->native_lib_name[0]) return;
    char filename[SSRNX_APK_LIBNAME_CAP + 16];
    if (apk_starts_with(info->native_lib_name, "lib") && apk_ends_with(info->native_lib_name, ".so")) {
        snprintf(filename, sizeof(filename), "%s", info->native_lib_name);
    } else {
        snprintf(filename, sizeof(filename), "lib%s.so", info->native_lib_name);
    }
    snprintf(info->native_entry_path, sizeof(info->native_entry_path),
             "lib/arm64-v8a/%s", filename);
}

SsrnxResult ssrnx_apk_inspect(const char *apk_path, SsrnxApkInfo *out_info) {
    if (!apk_path || !out_info) return SSRNX_ERR_ARGUMENT;
    memset(out_info, 0, sizeof(*out_info));
    out_info->screen_orientation = SSRNX_SCREEN_ORIENTATION_UNSPECIFIED;
    if (!apk_copy_string(out_info->apk_path, sizeof(out_info->apk_path), apk_path) ||
        !apk_copy_string(out_info->base_apk_path, sizeof(out_info->base_apk_path), apk_path)) {
        return SSRNX_ERR_ARGUMENT;
    }
    out_info->package_apk_count = 1;
    out_info->package_selected_apk_count = 1;

    ZipDirectory zip;
    SsrnxResult rc = apk_zip_open(apk_path, &zip);
    if (rc != SSRNX_OK) return rc;
    out_info->zip_entry_count = zip.entry_count;

    apk_ZipEntryInfo manifest_entry;
    rc = apk_zip_find_entry(&zip, "AndroidManifest.xml", &manifest_entry);
    if (rc != SSRNX_OK) {
        apk_zip_close(&zip);
        return SSRNX_ERR_MANIFEST_INVALID;
    }

    uint8_t *manifest = NULL;
    size_t manifest_size = 0;
    rc = apk_zip_read_entry_data(&zip, &manifest_entry, &manifest, &manifest_size);
    if (rc != SSRNX_OK) {
        apk_zip_close(&zip);
        return rc;
    }

    rc = apk_parse_binary_manifest(manifest, manifest_size, out_info);
    free(manifest);
    if (rc != SSRNX_OK) {
        apk_zip_close(&zip);
        return rc;
    }
    out_info->game_activity = strcmp(out_info->activity_name,
                                     "com.google.androidgamesdk.GameActivity") == 0;
    rc = ssrnx_apk_activity_theme(apk_path, out_info->package_name,
                                  out_info->activity_name, &out_info->activity_theme_resource);
    if (rc != SSRNX_OK && rc != SSRNX_ERR_SYMBOL_NOT_FOUND) {
        apk_zip_close(&zip);
        return rc;
    }
    apk_make_native_entry_name(out_info);

    if (apk_zip_rewind_central(&zip) != 0) {
        apk_zip_close(&zip);
        return SSRNX_ERR_IO;
    }
    for (uint16_t i = 0; i < zip.entry_count; ++i) {
        apk_ZipEntryInfo entry;
        rc = apk_zip_read_central_entry(&zip, &entry);
        if (rc != SSRNX_OK) {
            apk_zip_close(&zip);
            return rc;
        }
        if (apk_starts_with(entry.name, "lib/") && apk_ends_with(entry.name, ".so")) {
            out_info->native_lib_count++;
        }
        if (apk_starts_with(entry.name, "lib/arm64-v8a/") && apk_ends_with(entry.name, ".so")) {
            out_info->arm64_lib_count++;
            const char *soname = strrchr(entry.name, '/');
            soname = soname ? soname + 1 : entry.name;
            if (strcmp(soname, "libunity.so") == 0) {
                out_info->unity_player_present = true;
                out_info->engine_kind = SSRNX_ENGINE_UNITY;
            } else if (strcmp(soname, "libmain.so") == 0) {
                out_info->unity_main_present = true;
            } else if (strcmp(soname, "libil2cpp.so") == 0) {
                out_info->unity_il2cpp_present = true;
            } else if (strcmp(soname, "libmonobdwgc-2.0.so") == 0 ||
                       strcmp(soname, "libmono.so") == 0) {
                out_info->unity_mono_present = true;
            } else if (strcmp(soname, "libSDL2.so") == 0 ||
                       strcmp(soname, "libSDL3.so") == 0) {
                out_info->sdl_present = true;
            } else if (strcmp(soname, "libgodot_android.so") == 0) {
                out_info->godot_present = true;
            } else if (strcmp(soname, "libyoyo.so") == 0) {
                out_info->gamemaker_present = true;
            }
            if (out_info->native_entry_path[0] &&
                strcmp(entry.name, out_info->native_entry_path) == 0) {
                out_info->native_library_present = true;
            }
        }
    }

    if (out_info->unity_player_present) {
        if (out_info->unity_il2cpp_present && out_info->unity_mono_present)
            out_info->unity_backend = SSRNX_UNITY_BACKEND_MIXED;
        else if (out_info->unity_il2cpp_present)
            out_info->unity_backend = SSRNX_UNITY_BACKEND_IL2CPP;
        else if (out_info->unity_mono_present)
            out_info->unity_backend = SSRNX_UNITY_BACKEND_MONO;
    } else {
        out_info->engine_kind = SSRNX_ENGINE_UNKNOWN;
        out_info->unity_backend = SSRNX_UNITY_BACKEND_NONE;
    }

    apk_zip_close(&zip);
    return SSRNX_OK;
}

SsrnxResult ssrnx_apk_probe(const char *apk_path, SsrnxApkInfo *out_info) {
    SsrnxResult rc = ssrnx_apk_inspect(apk_path, out_info);
    if (rc != SSRNX_OK) return rc;
    if (!out_info->native_activity || !out_info->native_lib_name[0]) {
        return SSRNX_ERR_APK_UNSUPPORTED;
    }
    return out_info->native_library_present ? SSRNX_OK : SSRNX_ERR_ABI_NOT_FOUND;
}


static uint32_t apk_rd32be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           (uint32_t)p[3];
}

static uint8_t apk_png_paeth(uint8_t a, uint8_t b, uint8_t c) {
    const int pa = (int)b - (int)c;
    const int pb = (int)a - (int)c;
    const int pc = pa + pb;
    const int apa = pa < 0 ? -pa : pa;
    const int apb = pb < 0 ? -pb : pb;
    const int apc = pc < 0 ? -pc : pc;
    if (apa <= apb && apa <= apc) return a;
    if (apb <= apc) return b;
    return c;
}

bool ssrnx_png_decode_rgba(const uint8_t *png, size_t png_size,
                            uint32_t **out_pixels, int *out_width, int *out_height) {
    static const uint8_t sig[8] = {137,80,78,71,13,10,26,10};
    if (!png || png_size < 33 || !out_pixels || !out_width || !out_height ||
        memcmp(png, sig, sizeof(sig)) != 0) return false;

    uint32_t width = 0, height = 0;
    uint8_t bit_depth = 0, color_type = 0, interlace = 0;
    uint8_t palette[256][4];
    size_t palette_count = 0;
    for (size_t i = 0; i < 256; ++i) {
        palette[i][0] = palette[i][1] = palette[i][2] = 0;
        palette[i][3] = 255;
    }

    uint8_t *idat = NULL;
    size_t idat_size = 0;
    bool have_ihdr = false;
    size_t pos = 8;
    while (pos + 12 <= png_size) {
        const uint32_t len = apk_rd32be(png + pos);
        const uint8_t *type = png + pos + 4;
        const size_t data_pos = pos + 8;
        if ((uint64_t)data_pos + len + 4u > png_size) { free(idat); return false; }
        const uint8_t *data = png + data_pos;
        if (memcmp(type, "IHDR", 4) == 0) {
            if (len != 13) { free(idat); return false; }
            width = apk_rd32be(data);
            height = apk_rd32be(data + 4);
            bit_depth = data[8];
            color_type = data[9];
            interlace = data[12];
            if (width == 0 || height == 0 || width > 2048 || height > 2048 ||
                bit_depth != 8 || data[10] != 0 || data[11] != 0 || interlace != 0) {
                free(idat); return false;
            }
            have_ihdr = true;
        } else if (memcmp(type, "PLTE", 4) == 0) {
            if (len == 0 || len > 768 || (len % 3) != 0) { free(idat); return false; }
            palette_count = len / 3;
            for (size_t i = 0; i < palette_count; ++i) {
                palette[i][0] = data[i * 3 + 0];
                palette[i][1] = data[i * 3 + 1];
                palette[i][2] = data[i * 3 + 2];
            }
        } else if (memcmp(type, "tRNS", 4) == 0 && color_type == 3) {
            const size_t n = len < palette_count ? len : palette_count;
            for (size_t i = 0; i < n; ++i) palette[i][3] = data[i];
        } else if (memcmp(type, "IDAT", 4) == 0) {
            if (len > SIZE_MAX - idat_size) { free(idat); return false; }
            uint8_t *grown = realloc(idat, idat_size + len);
            if (!grown) { free(idat); return false; }
            idat = grown;
            if (len) memcpy(idat + idat_size, data, len);
            idat_size += len;
        } else if (memcmp(type, "IEND", 4) == 0) {
            break;
        }
        pos = data_pos + len + 4u;
    }
    if (!have_ihdr || !idat || idat_size < 6) { free(idat); return false; }

    size_t channels = 0;
    switch (color_type) {
        case 0: channels = 1; break;
        case 2: channels = 3; break;
        case 3: channels = 1; break;
        case 4: channels = 2; break;
        case 6: channels = 4; break;
        default: free(idat); return false;
    }
    if (color_type == 3 && palette_count == 0) { free(idat); return false; }

    if (width > SIZE_MAX / channels) { free(idat); return false; }
    const size_t row_bytes = (size_t)width * channels;
    if (row_bytes + 1u > SIZE_MAX / height) { free(idat); return false; }
    const size_t raw_size = (row_bytes + 1u) * (size_t)height;
    uint8_t *raw = malloc(raw_size ? raw_size : 1u);
    const size_t recon_size = row_bytes * (size_t)height;
    uint8_t *recon = malloc(recon_size ? recon_size : 1u);
    if (!raw || !recon) { free(raw); free(recon); free(idat); return false; }

    const uint8_t cmf = idat[0], flg = idat[1];
    const bool zlib_ok = (cmf & 0x0fu) == 8u && (((unsigned)cmf << 8) + flg) % 31u == 0u &&
                         (flg & 0x20u) == 0u;
    const bool inflated = zlib_ok && apk_inflate_raw(idat + 2, idat_size - 6, raw, raw_size);
    free(idat);
    if (!inflated) { free(raw); free(recon); return false; }

    for (uint32_t y = 0; y < height; ++y) {
        const uint8_t filter = raw[(row_bytes + 1u) * y];
        const uint8_t *src = raw + (row_bytes + 1u) * y + 1u;
        uint8_t *dst = recon + row_bytes * y;
        const uint8_t *prev = y ? recon + row_bytes * (y - 1u) : NULL;
        if (filter > 4) { free(raw); free(recon); return false; }
        for (size_t x = 0; x < row_bytes; ++x) {
            const uint8_t left = x >= channels ? dst[x - channels] : 0;
            const uint8_t up = prev ? prev[x] : 0;
            const uint8_t upper_left = (prev && x >= channels) ? prev[x - channels] : 0;
            uint8_t value = src[x];
            if (filter == 1) value = (uint8_t)(value + left);
            else if (filter == 2) value = (uint8_t)(value + up);
            else if (filter == 3) value = (uint8_t)(value + (uint8_t)(((unsigned)left + up) / 2u));
            else if (filter == 4) value = (uint8_t)(value + apk_png_paeth(left, up, upper_left));
            dst[x] = value;
        }
    }
    free(raw);

    if ((size_t)width > SIZE_MAX / (size_t)height ||
        (size_t)width * (size_t)height > SIZE_MAX / sizeof(uint32_t)) {
        free(recon); return false;
    }
    uint32_t *pixels = malloc((size_t)width * (size_t)height * sizeof(uint32_t));
    if (!pixels) { free(recon); return false; }
    for (uint32_t y = 0; y < height; ++y) {
        const uint8_t *row = recon + row_bytes * y;
        for (uint32_t x = 0; x < width; ++x) {
            uint8_t r = 0, g = 0, b = 0, a = 255;
            const uint8_t *px = row + (size_t)x * channels;
            if (color_type == 0) r = g = b = px[0];
            else if (color_type == 2) { r = px[0]; g = px[1]; b = px[2]; }
            else if (color_type == 3) {
                const size_t pi = px[0];
                if (pi >= palette_count) { free(recon); free(pixels); return false; }
                r = palette[pi][0]; g = palette[pi][1]; b = palette[pi][2]; a = palette[pi][3];
            } else if (color_type == 4) { r = g = b = px[0]; a = px[1]; }
            else { r = px[0]; g = px[1]; b = px[2]; a = px[3]; }
            pixels[(size_t)y * width + x] = (uint32_t)r | ((uint32_t)g << 8) |
                                                ((uint32_t)b << 16) | ((uint32_t)a << 24);
        }
    }
    free(recon);
    *out_pixels = pixels;
    *out_width = (int)width;
    *out_height = (int)height;
    return true;
}


typedef struct apk_ApkResourceRank {
    int score;
    uint16_t smallest_width, width, height;
    uint8_t orientation;
} apk_ApkResourceRank;

typedef struct apk_ApkResourceCandidate {
    uint8_t data_type;
    uint32_t data;
    uint16_t density;
    uint16_t sdk_version;
    apk_ApkResourceRank rank;
    char path[SSRNX_APK_ENTRY_CAP];
} apk_ApkResourceCandidate;

typedef enum apk_ApkResourceScoreMode {
    APK_RES_SCORE_DEVICE = 0, /* the active Android resource configuration */
    APK_RES_SCORE_ICON,       /* launcher art: sharpest density available */
    APK_RES_SCORE_LABEL,      /* launcher text: default locale, then English */
} apk_ApkResourceScoreMode;

typedef struct apk_ApkArscView {
    const uint8_t *data;
    size_t size;
    uint32_t root_size;
    apk_AxmlStringPool global_pool;
    bool have_global_pool;
    apk_ApkResourceScoreMode score_mode;
    SsrnxAssetConfiguration configuration;
} apk_ApkArscView;

typedef struct apk_ApkDrawableXmlInfo {
    bool adaptive;
    uint32_t foreground_ref;
    uint32_t background_ref;
    uint32_t generic_ref;
    bool background_literal_color;
    uint32_t background_color_rgba;
} apk_ApkDrawableXmlInfo;

static bool apk_resource_color_type(uint8_t type) {
    return type >= RES_VALUE_TYPE_INT_COLOR_ARGB8 && type <= RES_VALUE_TYPE_INT_COLOR_RGB4;
}

static uint32_t apk_android_color_to_rgba(uint8_t type, uint32_t value) {
    uint8_t a = 255, r = 0, g = 0, b = 0;
    if (type == RES_VALUE_TYPE_INT_COLOR_ARGB8) {
        a = (uint8_t)(value >> 24); r = (uint8_t)(value >> 16);
        g = (uint8_t)(value >> 8); b = (uint8_t)value;
    } else if (type == RES_VALUE_TYPE_INT_COLOR_RGB8) {
        r = (uint8_t)(value >> 16); g = (uint8_t)(value >> 8); b = (uint8_t)value;
    } else if (type == RES_VALUE_TYPE_INT_COLOR_ARGB4) {
        a = (uint8_t)(((value >> 12) & 0xfu) * 17u);
        r = (uint8_t)(((value >> 8) & 0xfu) * 17u);
        g = (uint8_t)(((value >> 4) & 0xfu) * 17u);
        b = (uint8_t)((value & 0xfu) * 17u);
    } else if (type == RES_VALUE_TYPE_INT_COLOR_RGB4) {
        r = (uint8_t)(((value >> 8) & 0xfu) * 17u);
        g = (uint8_t)(((value >> 4) & 0xfu) * 17u);
        b = (uint8_t)((value & 0xfu) * 17u);
    }
    return (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
}

static uint32_t apk_android_color_to_argb(uint8_t type, uint32_t value) {
    uint8_t a = 255, r = 0, g = 0, b = 0;
    if (type == RES_VALUE_TYPE_INT_COLOR_ARGB8) {
        a = (uint8_t)(value >> 24); r = (uint8_t)(value >> 16);
        g = (uint8_t)(value >> 8); b = (uint8_t)value;
    } else if (type == RES_VALUE_TYPE_INT_COLOR_RGB8) {
        r = (uint8_t)(value >> 16); g = (uint8_t)(value >> 8); b = (uint8_t)value;
    } else if (type == RES_VALUE_TYPE_INT_COLOR_ARGB4) {
        a = (uint8_t)(((value >> 12) & 0xfu) * 17u);
        r = (uint8_t)(((value >> 8) & 0xfu) * 17u);
        g = (uint8_t)(((value >> 4) & 0xfu) * 17u);
        b = (uint8_t)((value & 0xfu) * 17u);
    } else if (type == RES_VALUE_TYPE_INT_COLOR_RGB4) {
        r = (uint8_t)(((value >> 8) & 0xfu) * 17u);
        g = (uint8_t)(((value >> 4) & 0xfu) * 17u);
        b = (uint8_t)((value & 0xfu) * 17u);
    }
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static int apk_resource_config_score(uint16_t density, uint16_t sdk_version) {
    /* SSRNX currently advertises mdpi/SDK 35.  Prefer the newest compatible
     * qualifier, then Android's exact/default density before nearby densities.
     * If the best qualified drawable is unsupported (for example a vector),
     * the decoder continues to older/alternate configurations. */
    if (sdk_version > 35u) return INT_MIN;
    int density_score;
    if (density == 160u) density_score = 5000;
    else if (density == 0u) density_score = 4500;
    else {
        int delta = (int)density - 160;
        if (delta < 0) delta = -delta;
        if (delta > 3500) delta = 3500;
        density_score = 4000 - delta;
    }
    return (int)sdk_version * 10000 + density_score;
}

static int apk_resource_view_score(const apk_ApkArscView *view, uint16_t density, uint16_t sdk_version,
                               char lang0, char lang1) {
    if (sdk_version > 35u) return INT_MIN;
    const apk_ApkResourceScoreMode mode = view ? view->score_mode : APK_RES_SCORE_DEVICE;
    if (mode == APK_RES_SCORE_ICON) {
        /* The launcher draws icons larger than an mdpi phone does, so prefer
         * anydpi XML (adaptive layers), then the densest bitmap up to xxxhdpi. */
        int density_score;
        if (density == 0xfffeu) density_score = 9000;
        else if (density == 0xffffu) density_score = 4480;
        else if (density == 0u) density_score = 4160;
        else density_score = 4000 + (density > 640u ? 640 - (int)(density - 640u) / 4 : (int)density);
        return (int)sdk_version * 10000 + density_score;
    }
    if (mode == APK_RES_SCORE_LABEL) {
        int locale_score = 100000;
        if (!lang0 && !lang1) locale_score = 300000;
        else if (lang0 == 'e' && lang1 == 'n') locale_score = 200000;
        return locale_score + (int)sdk_version;
    }
    return apk_resource_config_score(density, sdk_version);
}

/* Runtime configuration is optional for standalone package/launcher tools. */
extern SsrnxAssetManager *ssrnx_assets_manager(void) __attribute__((weak));
extern bool ssrnx_assets_get_configuration(const SsrnxAssetManager *, SsrnxAssetConfiguration *) __attribute__((weak));

/* apk resource config */
/* Android 11 size/orientation qualifiers precede density and version ranking.
 * Launcher icon/label policy deliberately remains independent of a running app. */
static apk_ApkResourceRank apk_resource_config_rank(const apk_ApkArscView *view, const uint8_t *config,
                                            size_t size) {
    apk_ApkResourceRank rank = {.score = INT_MIN};
    if (!config || size < 4 || apk_rd32(config) < 4 || apk_rd32(config) > size) return rank;
    size = apk_rd32(config);
    const uint16_t density = size >= 16 ? apk_rd16(config + 14) : 0;
    const uint16_t sdk = size >= 28 ? apk_rd16(config + 24) : 0;
    rank.score = apk_resource_view_score(view, density, sdk, size >= 12 ? config[8] : 0,
                                    size >= 12 ? config[9] : 0);
    if (view->score_mode != APK_RES_SCORE_DEVICE) return rank;
    rank.orientation = size >= 16 ? config[12] : 0;
    rank.smallest_width = size >= 32 ? apk_rd16(config + 30) : 0;
    rank.width = size >= 36 ? apk_rd16(config + 32) : 0;
    rank.height = size >= 36 ? apk_rd16(config + 34) : 0;
    const SsrnxAssetConfiguration *requested = &view->configuration;
    if (rank.smallest_width > requested->smallest_screen_width_dp ||
        rank.width > requested->screen_width_dp || rank.height > requested->screen_height_dp ||
        (rank.orientation && rank.orientation != requested->orientation) ||
        (sdk && sdk > requested->sdk_version)) rank.score = INT_MIN;
    return rank;
}

static bool apk_resource_rank_better(const apk_ApkResourceRank *a, const apk_ApkResourceRank *b) {
    if (a->score == INT_MIN) return false;
    if (b->score == INT_MIN) return true;
    if (a->smallest_width != b->smallest_width) return a->smallest_width > b->smallest_width;
    /* After rejecting oversized configs, the nearest w/h pair has the largest sum. */
    const unsigned sum_a = (unsigned)a->width + a->height, sum_b = (unsigned)b->width + b->height;
    if (sum_a != sum_b) return sum_a > sum_b;
    if (a->orientation != b->orientation) return a->orientation != 0;
    return a->score > b->score;
}



static void apk_resource_candidate_insert(apk_ApkResourceCandidate *items, size_t *count, size_t cap,
                                      const apk_ApkResourceCandidate *candidate) {
    if (!items || !count || !candidate || cap == 0 || candidate->rank.score == INT_MIN) return;
    for (size_t i = 0; i < *count; ++i) {
        if (items[i].data_type == candidate->data_type && items[i].data == candidate->data &&
            strcmp(items[i].path, candidate->path) == 0) {
            if (!apk_resource_rank_better(&candidate->rank, &items[i].rank)) return;
            memmove(items + i, items + i + 1, (*count - i - 1) * sizeof(*items));
            --*count;
            break;
        }
    }
    size_t pos = 0;
    while (pos < *count && !apk_resource_rank_better(&candidate->rank, &items[pos].rank)) ++pos;
    if (pos >= cap) return;
    size_t n = *count < cap ? *count : cap - 1u;
    while (n > pos) { items[n] = items[n - 1u]; --n; }
    items[pos] = *candidate;
    if (*count < cap) ++(*count);
}

static bool apk_arsc_view_init(apk_ApkArscView *view, const uint8_t *data, size_t size) {
    if (!view || !data || size < 12 || apk_rd16(data) != RES_TABLE_TYPE) return false;
    const uint16_t header_size = apk_rd16(data + 2);
    const uint32_t root_size = apk_rd32(data + 4);
    if (header_size < 12 || root_size < header_size || root_size > size) return false;
    memset(view, 0, sizeof(*view));
    view->configuration.density = 160;
    view->configuration.sdk_version = 35;
    if (ssrnx_assets_manager && ssrnx_assets_get_configuration)
        (void)ssrnx_assets_get_configuration(ssrnx_assets_manager(), &view->configuration);
    view->data = data; view->size = size; view->root_size = root_size;
    size_t off = header_size;
    while (off + 8u <= root_size) {
        const uint8_t *chunk = data + off;
        const uint16_t type = apk_rd16(chunk);
        const uint16_t hs = apk_rd16(chunk + 2);
        const uint32_t cs = apk_rd32(chunk + 4);
        if (hs < 8 || cs < hs || off + cs > root_size) return false;
        if (type == RES_STRING_POOL_TYPE && !view->have_global_pool) {
            if (!apk_axml_pool_init(&view->global_pool, chunk, root_size - off)) return false;
            view->have_global_pool = true;
            break;
        }
        off += cs;
    }
    return view->have_global_pool;
}

static bool apk_arsc_type_entry_offset(const uint8_t *type_chunk, uint16_t header_size,
                                   uint32_t chunk_size, uint8_t flags,
                                   uint32_t entry_count, uint32_t wanted_entry,
                                   uint32_t *out_offset) {
    if (!type_chunk || !out_offset || header_size > chunk_size) return false;
    const uint8_t *index = type_chunk + header_size;
    const size_t index_bytes = (size_t)(type_chunk + chunk_size - index);
    if (flags & RES_TABLE_TYPE_FLAG_SPARSE) {
        if ((uint64_t)entry_count * 4u > index_bytes) return false;
        for (uint32_t i = 0; i < entry_count; ++i) {
            const uint16_t idx = apk_rd16(index + (size_t)i * 4u);
            if (idx == wanted_entry) {
                *out_offset = (uint32_t)apk_rd16(index + (size_t)i * 4u + 2u) * 4u;
                return true;
            }
            if (idx > wanted_entry) break;
        }
        return false;
    }
    if (wanted_entry >= entry_count) return false;
    if (flags & RES_TABLE_TYPE_FLAG_OFFSET16) {
        if ((uint64_t)entry_count * 2u > index_bytes) return false;
        const uint16_t raw = apk_rd16(index + (size_t)wanted_entry * 2u);
        if (raw == 0xffffu) return false;
        *out_offset = (uint32_t)raw * 4u;
        return true;
    }
    if ((uint64_t)entry_count * 4u > index_bytes) return false;
    const uint32_t raw = apk_rd32(index + (size_t)wanted_entry * 4u);
    if (raw == RES_NO_ENTRY) return false;
    *out_offset = raw;
    return true;
}

static void apk_arsc_collect_values_depth(const apk_ApkArscView *view, uint32_t resource_id,
                                      apk_ApkResourceCandidate *items, size_t *count, size_t cap,
                                      unsigned depth) {
    if (!view || !resource_id || !items || !count || depth > 8u) return;
    const uint8_t wanted_package = (uint8_t)(resource_id >> 24);
    const uint8_t wanted_type = (uint8_t)(resource_id >> 16);
    const uint32_t wanted_entry = resource_id & 0xffffu;
    size_t off = apk_rd16(view->data + 2);
    while (off + 8u <= view->root_size) {
        const uint8_t *chunk = view->data + off;
        const uint16_t type = apk_rd16(chunk);
        const uint16_t header_size = apk_rd16(chunk + 2);
        const uint32_t chunk_size = apk_rd32(chunk + 4);
        if (header_size < 8 || chunk_size < header_size || off + chunk_size > view->root_size) return;
        if (type == RES_TABLE_PACKAGE_TYPE && header_size >= 284u) {
            const uint32_t package_id = apk_rd32(chunk + 8);
            uint32_t type_id_offset = 0;
            if (header_size >= 288u) type_id_offset = apk_rd32(chunk + 284);
            if ((uint8_t)package_id == wanted_package) {
                size_t child_off = header_size;
                while (child_off + 8u <= chunk_size) {
                    const uint8_t *child = chunk + child_off;
                    const uint16_t child_type = apk_rd16(child);
                    const uint16_t child_header = apk_rd16(child + 2);
                    const uint32_t child_size = apk_rd32(child + 4);
                    if (child_header < 8 || child_size < child_header || child_off + child_size > chunk_size) break;
                    if (child_type == RES_TABLE_TYPE_TYPE && child_header >= 24u) {
                        const uint8_t raw_type_id = child[8];
                        const uint8_t type_flags = child[9];
                        const uint32_t actual_type_id = (uint32_t)raw_type_id + type_id_offset;
                        if (actual_type_id == wanted_type) {
                            const uint32_t entry_count = apk_rd32(child + 12);
                            const uint32_t entries_start = apk_rd32(child + 16);
                            if (entries_start < child_size) {
                                uint32_t entry_offset = 0;
                                if (apk_arsc_type_entry_offset(child, child_header, child_size, type_flags,
                                                           entry_count, wanted_entry, &entry_offset) &&
                                    entry_offset <= child_size - entries_start) {
                                    const size_t absolute = (size_t)entries_start + entry_offset;
                                    if (absolute + 8u <= child_size) {
                                        const uint8_t *entry = child + absolute;
                                        const uint16_t entry_size = apk_rd16(entry);
                                        const uint16_t entry_flags = apk_rd16(entry + 2);
                                        if (entry_size >= 8u && !(entry_flags & RES_TABLE_ENTRY_FLAG_COMPLEX) &&
                                            absolute + entry_size + 8u <= child_size) {
                                            const uint8_t *value = entry + entry_size;
                                            if (apk_rd16(value) >= 8u) {
                                                const uint8_t data_type = value[3];
                                                const uint32_t data = apk_rd32(value + 4);
                                                uint16_t density = 0, sdk = 0;
                                                if (child_header >= 36u) {
                                                    const uint8_t *config = child + 20;
                                                    const uint32_t config_size = apk_rd32(config);
                                                    if (config_size >= 16u && 20u + config_size <= child_header)
                                                        density = apk_rd16(config + 14);
                                                    if (config_size >= 28u && 20u + config_size <= child_header)
                                                        sdk = apk_rd16(config + 24);
                                                }
                                                const apk_ApkResourceRank rank = apk_resource_config_rank(view, child + 20, child_header - 20u);
                                                if (rank.score != INT_MIN && (data_type == RES_VALUE_TYPE_REFERENCE ||
                                                    data_type == RES_VALUE_TYPE_DYNAMIC_REFERENCE)) {
                                                    if (data != resource_id)
                                                        apk_arsc_collect_values_depth(view, data, items, count, cap, depth + 1u);
                                                } else if (rank.score != INT_MIN && (data_type == RES_VALUE_TYPE_STRING ||
                                                           data_type == RES_VALUE_TYPE_INT_BOOLEAN ||
                                                           (data_type >= RES_VALUE_TYPE_INT_DEC &&
                                                            data_type <= RES_VALUE_TYPE_INT_HEX) ||
                                                           data_type == RES_VALUE_TYPE_FLOAT ||
                                                           apk_resource_color_type(data_type))) {
                                                    apk_ApkResourceCandidate c;
                                                    memset(&c, 0, sizeof(c));
                                                    c.data_type = data_type; c.data = data; c.density = density; c.sdk_version = sdk;
                                                    c.rank = rank;
                                                    if (data_type == RES_VALUE_TYPE_STRING &&
                                                        !apk_axml_pool_get(&view->global_pool, data, c.path, sizeof(c.path))) {
                                                        c.rank.score = INT_MIN;
                                                    }
                                                    apk_resource_candidate_insert(items, count, cap, &c);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    child_off += child_size;
                }
            }
        }
        off += chunk_size;
    }
}

static size_t apk_arsc_collect_values(const apk_ApkArscView *view, uint32_t resource_id,
                                  apk_ApkResourceCandidate *items, size_t cap) {
    if (!items || cap == 0) return 0;
    size_t count = 0;
    apk_arsc_collect_values_depth(view, resource_id, items, &count, cap, 0);
    return count;
}

static bool apk_axml_attr_typed(const apk_AxmlStringPool *pool, const uint8_t *attrs,
                            uint16_t attr_size, uint16_t attr_count, const char *name,
                            uint8_t *out_type, uint32_t *out_data) {
    const uint8_t *attr = apk_axml_find_attr(pool, attrs, attr_size, attr_count, name);
    if (!attr || apk_rd16(attr + 12) < 8u || !out_type || !out_data) return false;
    *out_type = attr[15]; *out_data = apk_rd32(attr + 16);
    return true;
}

static bool apk_parse_drawable_xml(const uint8_t *data, size_t size, apk_ApkDrawableXmlInfo *out) {
    if (!data || !out || size < 8u || apk_rd16(data) != RES_XML_TYPE) return false;
    memset(out, 0, sizeof(*out));
    const uint16_t root_header = apk_rd16(data + 2);
    const uint32_t root_size = apk_rd32(data + 4);
    if (root_header < 8u || root_size < root_header || root_size > size) return false;
    apk_AxmlStringPool pool; memset(&pool, 0, sizeof(pool)); bool have_pool = false;
    size_t off = root_header;
    while (off + 8u <= root_size) {
        const uint8_t *chunk = data + off;
        const uint16_t type = apk_rd16(chunk), header_size = apk_rd16(chunk + 2);
        const uint32_t chunk_size = apk_rd32(chunk + 4);
        if (header_size < 8u || chunk_size < header_size || off + chunk_size > root_size) return false;
        if (type == RES_STRING_POOL_TYPE) {
            if (!apk_axml_pool_init(&pool, chunk, root_size - off)) return false;
            have_pool = true;
        } else if (type == RES_XML_START_ELEMENT_TYPE) {
            if (!have_pool || chunk_size < 36u) return false;
            const uint8_t *ext = chunk + 16;
            const uint32_t name_index = apk_rd32(ext + 4);
            const uint16_t attr_start = apk_rd16(ext + 8), attr_size = apk_rd16(ext + 10), attr_count = apk_rd16(ext + 12);
            if (attr_size < 20u) return false;
            const size_t attrs_off = 16u + attr_start;
            if (attrs_off > chunk_size || (uint64_t)attrs_off + (uint64_t)attr_size * attr_count > chunk_size) return false;
            char tag[64];
            if (!apk_axml_pool_get(&pool, name_index, tag, sizeof(tag))) return false;
            const uint8_t *attrs = chunk + attrs_off;
            if (strcmp(tag, "adaptive-icon") == 0) out->adaptive = true;
            uint8_t value_type = 0; uint32_t value_data = 0;
            const char *attr_name = strcmp(tag, "bitmap") == 0 ? "src" : "drawable";
            if (apk_axml_attr_typed(&pool, attrs, attr_size, attr_count, attr_name, &value_type, &value_data)) {
                if (value_type == RES_VALUE_TYPE_REFERENCE || value_type == RES_VALUE_TYPE_DYNAMIC_REFERENCE) {
                    if (strcmp(tag, "foreground") == 0 || strcmp(tag, "monochrome") == 0) out->foreground_ref = value_data;
                    else if (strcmp(tag, "background") == 0) out->background_ref = value_data;
                    else if (!out->generic_ref) out->generic_ref = value_data;
                } else if (strcmp(tag, "background") == 0 && apk_resource_color_type(value_type)) {
                    out->background_literal_color = true;
                    out->background_color_rgba = apk_android_color_to_rgba(value_type, value_data);
                }
            }
        }
        off += chunk_size;
    }
    return out->adaptive || out->foreground_ref || out->background_ref || out->generic_ref;
}

static uint32_t apk_rgba_over(uint32_t src, uint32_t dst) {
    const unsigned a = (src >> 24) & 255u;
    if (a == 255u) return src;
    if (a == 0u) return dst;
    const unsigned sr = src & 255u, sg = (src >> 8) & 255u, sb = (src >> 16) & 255u;
    const unsigned dr = dst & 255u, dg = (dst >> 8) & 255u, db = (dst >> 16) & 255u;
    const unsigned r = (sr * a + dr * (255u - a) + 127u) / 255u;
    const unsigned g = (sg * a + dg * (255u - a) + 127u) / 255u;
    const unsigned b = (sb * a + db * (255u - a) + 127u) / 255u;
    return (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | UINT32_C(0xff000000);
}

static bool apk_arsc_best_color(const apk_ApkArscView *view, uint32_t resource_id, uint32_t *out_rgba) {
    apk_ApkResourceCandidate candidates[16];
    const size_t count = apk_arsc_collect_values(view, resource_id, candidates, 16);
    for (size_t i = 0; i < count; ++i) {
        if (apk_resource_color_type(candidates[i].data_type)) {
            if (out_rgba) *out_rgba = apk_android_color_to_rgba(candidates[i].data_type, candidates[i].data);
            return true;
        }
    }
    return false;
}

static SsrnxResult apk_decode_arsc_icon_resource(ZipDirectory *zip, const apk_ApkArscView *view,
                                             uint32_t resource_id, unsigned depth,
                                             uint32_t **out_pixels, int *out_width, int *out_height,
                                             char *out_source, size_t out_source_cap);

/* Adaptive icon layers are 108dp with only the centre 72dp visible; the outer
 * 18dp per side is reserved for launcher masks/parallax. Crop to that viewport
 * so the artwork fills the launcher tile the way it does on Android. */
static bool apk_crop_adaptive_viewport(uint32_t **pixels, int *width, int *height) {
    if (!pixels || !*pixels || !width || !height || *width < 6 || *height < 6) return false;
    const int inset_x = *width / 6, inset_y = *height / 6;
    const int w = *width - 2 * inset_x, h = *height - 2 * inset_y;
    uint32_t *dst = malloc((size_t)w * (size_t)h * sizeof(uint32_t));
    if (!dst) return false;
    for (int y = 0; y < h; ++y)
        memcpy(dst + (size_t)y * w, *pixels + (size_t)(y + inset_y) * *width + inset_x,
               (size_t)w * sizeof(uint32_t));
    free(*pixels);
    *pixels = dst; *width = w; *height = h;
    return true;
}

static SsrnxResult apk_decode_drawable_xml_icon(ZipDirectory *zip, const apk_ApkArscView *view,
                                             const uint8_t *xml, size_t xml_size, unsigned depth,
                                             uint32_t **out_pixels, int *out_width, int *out_height) {
    if (!zip || !view || !xml || !out_pixels || !out_width || !out_height || depth > 8u)
        return SSRNX_ERR_ARGUMENT;
    apk_ApkDrawableXmlInfo info;
    if (!apk_parse_drawable_xml(xml, xml_size, &info)) return SSRNX_ERR_SYMBOL_NOT_FOUND;
    if (!info.adaptive) {
        const uint32_t ref = info.generic_ref ? info.generic_ref : (info.foreground_ref ? info.foreground_ref : info.background_ref);
        if (!ref) return SSRNX_ERR_SYMBOL_NOT_FOUND;
        return apk_decode_arsc_icon_resource(zip, view, ref, depth + 1u,
                                         out_pixels, out_width, out_height, NULL, 0);
    }

    uint32_t *fg = NULL, *bg = NULL; int fg_w = 0, fg_h = 0, bg_w = 0, bg_h = 0;
    SsrnxResult fg_rc = SSRNX_ERR_SYMBOL_NOT_FOUND;
    if (info.foreground_ref) {
        fg_rc = apk_decode_arsc_icon_resource(zip, view, info.foreground_ref, depth + 1u,
                                          &fg, &fg_w, &fg_h, NULL, 0);
    }
    uint32_t bg_color = 0; bool have_bg_color = info.background_literal_color;
    if (have_bg_color) bg_color = info.background_color_rgba;
    if (info.background_ref) {
        if (!apk_arsc_best_color(view, info.background_ref, &bg_color)) {
            (void)apk_decode_arsc_icon_resource(zip, view, info.background_ref, depth + 1u,
                                            &bg, &bg_w, &bg_h, NULL, 0);
        } else have_bg_color = true;
    }
    if (fg_rc != SSRNX_OK || !fg || fg_w <= 0 || fg_h <= 0) {
        if (bg && bg_w > 0 && bg_h > 0) {
            free(fg);
            (void)apk_crop_adaptive_viewport(&bg, &bg_w, &bg_h);
            *out_pixels = bg; *out_width = bg_w; *out_height = bg_h;
            return SSRNX_OK;
        }
        free(bg); free(fg);
        return SSRNX_ERR_SYMBOL_NOT_FOUND;
    }

    uint32_t *canvas = malloc((size_t)fg_w * (size_t)fg_h * sizeof(uint32_t));
    if (!canvas) { free(fg); free(bg); return SSRNX_ERR_NO_MEMORY; }
    for (int y = 0; y < fg_h; ++y) {
        for (int x = 0; x < fg_w; ++x) {
            uint32_t base = have_bg_color ? bg_color : 0u;
            if (bg && bg_w > 0 && bg_h > 0) {
                const int bx = (int)((int64_t)x * bg_w / fg_w);
                const int by = (int)((int64_t)y * bg_h / fg_h);
                base = bg[(size_t)by * bg_w + bx];
            }
            canvas[(size_t)y * fg_w + x] = apk_rgba_over(fg[(size_t)y * fg_w + x], base);
        }
    }
    free(fg); free(bg);
    int out_w = fg_w, out_h = fg_h;
    (void)apk_crop_adaptive_viewport(&canvas, &out_w, &out_h);
    *out_pixels = canvas; *out_width = out_w; *out_height = out_h;
    return SSRNX_OK;
}

static SsrnxResult apk_decode_arsc_icon_resource(ZipDirectory *zip, const apk_ApkArscView *view,
                                             uint32_t resource_id, unsigned depth,
                                             uint32_t **out_pixels, int *out_width, int *out_height,
                                             char *out_source, size_t out_source_cap) {
    if (!zip || !view || !resource_id || !out_pixels || !out_width || !out_height || depth > 8u)
        return SSRNX_ERR_ARGUMENT;
    apk_ApkResourceCandidate candidates[24];
    const size_t count = apk_arsc_collect_values(view, resource_id, candidates, 24);
    SsrnxResult last = SSRNX_ERR_SYMBOL_NOT_FOUND;
    for (size_t i = 0; i < count; ++i) {
        const apk_ApkResourceCandidate *c = &candidates[i];
        if (c->data_type != RES_VALUE_TYPE_STRING || !c->path[0]) continue;
        apk_ZipEntryInfo entry;
        if (apk_zip_find_entry(zip, c->path, &entry) != SSRNX_OK || entry.uncompressed_size == 0 ||
            entry.uncompressed_size > 8u * 1024u * 1024u) continue;
        uint8_t *data = NULL; size_t size = 0;
        SsrnxResult rc = apk_zip_read_entry_data(zip, &entry, &data, &size);
        if (rc != SSRNX_OK) { last = rc; continue; }
        uint32_t *pixels = NULL; int width = 0, height = 0;
        bool decoded = false;
        if (apk_ends_with(c->path, ".png")) {
            decoded = ssrnx_png_decode_rgba(data, size, &pixels, &width, &height);
            rc = decoded ? SSRNX_OK : SSRNX_ERR_APK_INVALID;
        } else if (apk_ends_with(c->path, ".xml")) {
            rc = apk_decode_drawable_xml_icon(zip, view, data, size, depth + 1u, &pixels, &width, &height);
            decoded = rc == SSRNX_OK;
        } else {
            /* WebP/JPEG/vector-only drawables remain unsupported by the built-in
             * dependency-free decoder. Continue to lower-priority resource
             * configurations (typically the legacy PNG launcher icon). */
            rc = SSRNX_ERR_SYMBOL_NOT_FOUND;
        }
        free(data);
        if (!decoded) { free(pixels); last = rc; continue; }
        *out_pixels = pixels; *out_width = width; *out_height = height;
        if (out_source && out_source_cap) (void)apk_copy_string(out_source, out_source_cap, c->path);
        return SSRNX_OK;
    }
    return last;
}

static SsrnxResult apk_load_manifest_resource_icon(ZipDirectory *zip,
                                                uint32_t **out_pixels, int *out_width, int *out_height,
                                                char *out_entry_name, size_t out_entry_name_cap) {
    if (!zip || !out_pixels || !out_width || !out_height) return SSRNX_ERR_ARGUMENT;
    apk_ZipEntryInfo manifest_entry, table_entry;
    if (apk_zip_find_entry(zip, "AndroidManifest.xml", &manifest_entry) != SSRNX_OK ||
        apk_zip_find_entry(zip, "resources.arsc", &table_entry) != SSRNX_OK ||
        table_entry.uncompressed_size == 0 || table_entry.uncompressed_size > 32u * 1024u * 1024u)
        return SSRNX_ERR_SYMBOL_NOT_FOUND;
    uint8_t *manifest = NULL, *arsc = NULL; size_t manifest_size = 0, arsc_size = 0;
    SsrnxResult rc = apk_zip_read_entry_data(zip, &manifest_entry, &manifest, &manifest_size);
    if (rc != SSRNX_OK) return rc;
    SsrnxApkInfo info; memset(&info, 0, sizeof(info));
    rc = apk_parse_binary_manifest(manifest, manifest_size, &info);
    free(manifest);
    if (rc != SSRNX_OK) return rc;
    if (!info.application_icon_resource && !info.application_round_icon_resource)
        return SSRNX_ERR_SYMBOL_NOT_FOUND;
    rc = apk_zip_read_entry_data(zip, &table_entry, &arsc, &arsc_size);
    if (rc != SSRNX_OK) return rc;
    apk_ApkArscView view;
    if (!apk_arsc_view_init(&view, arsc, arsc_size)) { free(arsc); return SSRNX_ERR_APK_INVALID; }
    view.score_mode = APK_RES_SCORE_ICON;
    const uint32_t ids[2] = { info.application_icon_resource, info.application_round_icon_resource };
    rc = SSRNX_ERR_SYMBOL_NOT_FOUND;
    for (size_t i = 0; i < 2; ++i) {
        if (!ids[i] || (i == 1 && ids[i] == ids[0])) continue;
        rc = apk_decode_arsc_icon_resource(zip, &view, ids[i], 0, out_pixels, out_width, out_height,
                                       out_entry_name, out_entry_name_cap);
        if (rc == SSRNX_OK) break;
    }
    free(arsc);
    return rc;
}

static int apk_apk_icon_score(const char *name) {
    if (!name || !apk_starts_with(name, "res/") || !apk_ends_with(name, ".png")) return -1;
    int score = 0;
    if (strstr(name, "/mipmap")) score += 1200;
    else if (strstr(name, "/drawable")) score += 500;
    else return -1;
    if (strstr(name, "ic_launcher")) score += 1200;
    else if (strstr(name, "launcher")) score += 900;
    else if (strstr(name, "icon")) score += 700;
    if (strstr(name, "xxxhdpi")) score += 500;
    else if (strstr(name, "xxhdpi")) score += 400;
    else if (strstr(name, "xhdpi")) score += 300;
    else if (strstr(name, "hdpi")) score += 200;
    else if (strstr(name, "nodpi")) score += 180;
    else if (strstr(name, "mdpi")) score += 100;
    if (strstr(name, "round")) score += 40;
    if (strstr(name, "foreground")) score -= 120;
    if (strstr(name, "background")) score -= 500;
    return score;
}

typedef struct apk_ApkIconCandidate {
    apk_ZipEntryInfo entry;
    int score;
} apk_ApkIconCandidate;

static void apk_icon_candidate_insert(apk_ApkIconCandidate *items, size_t *count, size_t cap,
                                  const apk_ZipEntryInfo *entry, int score) {
    size_t pos = 0;
    while (pos < *count) {
        if (score > items[pos].score) break;
        if (score == items[pos].score && entry->uncompressed_size > items[pos].entry.uncompressed_size) break;
        ++pos;
    }
    if (pos >= cap) return;
    size_t n = *count < cap ? *count : cap - 1u;
    while (n > pos) { items[n] = items[n - 1u]; --n; }
    items[pos].entry = *entry;
    items[pos].score = score;
    if (*count < cap) ++(*count);
}

static bool apk_downsample_icon(uint32_t **pixels, int *width, int *height) {
    const int max_dim = 256;
    if (!pixels || !*pixels || !width || !height || *width <= 0 || *height <= 0) return false;
    if (*width <= max_dim && *height <= max_dim) return true;
    const int src_w = *width, src_h = *height;
    int dst_w = src_w, dst_h = src_h;
    if (src_w >= src_h) {
        dst_w = max_dim;
        dst_h = (int)(((int64_t)src_h * max_dim + src_w / 2) / src_w);
    } else {
        dst_h = max_dim;
        dst_w = (int)(((int64_t)src_w * max_dim + src_h / 2) / src_h);
    }
    if (dst_w < 1) dst_w = 1;
    if (dst_h < 1) dst_h = 1;
    uint32_t *dst = malloc((size_t)dst_w * (size_t)dst_h * sizeof(uint32_t));
    if (!dst) return false;
    /* Box filter with premultiplied alpha: every source pixel contributes to
     * its destination cell, so shrinking a 432px xxxhdpi icon stays smooth
     * instead of aliasing the way point sampling does. */
    for (int y = 0; y < dst_h; ++y) {
        const int sy0 = (int)((int64_t)y * src_h / dst_h);
        int sy1 = (int)((int64_t)(y + 1) * src_h / dst_h);
        if (sy1 <= sy0) sy1 = sy0 + 1;
        for (int x = 0; x < dst_w; ++x) {
            const int sx0 = (int)((int64_t)x * src_w / dst_w);
            int sx1 = (int)((int64_t)(x + 1) * src_w / dst_w);
            if (sx1 <= sx0) sx1 = sx0 + 1;
            uint64_t r = 0, g = 0, b = 0, a = 0, n = 0;
            for (int sy = sy0; sy < sy1; ++sy) {
                const uint32_t *row = *pixels + (size_t)sy * src_w;
                for (int sx = sx0; sx < sx1; ++sx) {
                    const uint32_t p = row[sx];
                    const uint32_t pa = p >> 24;
                    r += (p & 255u) * pa; g += ((p >> 8) & 255u) * pa;
                    b += ((p >> 16) & 255u) * pa; a += pa; ++n;
                }
            }
            uint32_t out = 0;
            if (a) {
                out = (uint32_t)((r + a / 2) / a) | ((uint32_t)((g + a / 2) / a) << 8) |
                      ((uint32_t)((b + a / 2) / a) << 16) | ((uint32_t)((a + n / 2) / n) << 24);
            }
            dst[(size_t)y * dst_w + x] = out;
        }
    }
    free(*pixels);
    *pixels = dst;
    *width = dst_w;
    *height = dst_h;
    return true;
}

SsrnxResult ssrnx_apk_load_launcher_icon(const char *apk_path,
                                         uint32_t **out_pixels,
                                         int *out_width,
                                         int *out_height,
                                         char *out_entry_name,
                                         size_t out_entry_name_cap) {
    if (!apk_path || !out_pixels || !out_width || !out_height) return SSRNX_ERR_ARGUMENT;
    *out_pixels = NULL;
    *out_width = 0;
    *out_height = 0;
    if (out_entry_name && out_entry_name_cap) out_entry_name[0] = '\0';

    ZipDirectory zip;
    SsrnxResult rc = apk_zip_open(apk_path, &zip);
    if (rc != SSRNX_OK) return rc;

    /* First honor the exact resource declared by the manifest.  Modern APKs
     * commonly point android:icon at a resources.arsc entry whose selected
     * value is density-qualified or adaptive-icon XML, so filename guessing
     * alone is not Android-compatible. */
    rc = apk_load_manifest_resource_icon(&zip, out_pixels, out_width, out_height,
                                     out_entry_name, out_entry_name_cap);
    if (rc == SSRNX_OK) {
        if (!apk_downsample_icon(out_pixels, out_width, out_height)) {
            free(*out_pixels); *out_pixels = NULL; *out_width = 0; *out_height = 0;
            apk_zip_close(&zip); return SSRNX_ERR_NO_MEMORY;
        }
        apk_zip_close(&zip);
        return SSRNX_OK;
    }

    /* Retain the legacy scan as a compatibility fallback for malformed/old
     * packages with no resolvable icon resource table. */
    if (apk_zip_rewind_central(&zip) != 0) { apk_zip_close(&zip); return SSRNX_ERR_IO; }

    apk_ApkIconCandidate candidates[12];
    size_t candidate_count = 0;
    for (uint16_t i = 0; i < zip.entry_count; ++i) {
        apk_ZipEntryInfo entry;
        rc = apk_zip_read_central_entry(&zip, &entry);
        if (rc != SSRNX_OK) { apk_zip_close(&zip); return rc; }
        const int score = apk_apk_icon_score(entry.name);
        if (score >= 0 && entry.uncompressed_size > 0 && entry.uncompressed_size <= 8u * 1024u * 1024u) {
            apk_icon_candidate_insert(candidates, &candidate_count, 12, &entry, score);
        }
    }

    rc = SSRNX_ERR_SYMBOL_NOT_FOUND;
    for (size_t i = 0; i < candidate_count; ++i) {
        uint8_t *data = NULL;
        size_t size = 0;
        SsrnxResult read_rc = apk_zip_read_entry_data(&zip, &candidates[i].entry, &data, &size);
        if (read_rc != SSRNX_OK) continue;
        uint32_t *pixels = NULL;
        int width = 0, height = 0;
        const bool decoded = ssrnx_png_decode_rgba(data, size, &pixels, &width, &height);
        free(data);
        if (!decoded) continue;
        if (!apk_downsample_icon(&pixels, &width, &height)) { free(pixels); rc = SSRNX_ERR_NO_MEMORY; break; }
        *out_pixels = pixels;
        *out_width = width;
        *out_height = height;
        if (out_entry_name && out_entry_name_cap) apk_copy_string(out_entry_name, out_entry_name_cap, candidates[i].entry.name);
        rc = SSRNX_OK;
        break;
    }
    apk_zip_close(&zip);
    return rc;
}


static bool apk_arsc_package_name_utf8(const uint8_t *package_chunk, uint16_t header_size,
                                   char *out, size_t cap) {
    if (!package_chunk || header_size < 268u || !out || cap == 0) return false;
    size_t w = 0;
    for (uint32_t i = 0; i < 128u; ++i) {
        uint32_t cp = apk_rd16(package_chunk + 12u + (size_t)i * 2u);
        if (cp == 0) { out[w] = '\0'; return true; }
        if (cp >= 0xd800u && cp <= 0xdbffu) {
            if (++i >= 128u) return false;
            const uint32_t low = apk_rd16(package_chunk + 12u + (size_t)i * 2u);
            if (low < 0xdc00u || low > 0xdfffu) return false;
            cp = UINT32_C(0x10000) + ((cp - 0xd800u) << 10) + (low - 0xdc00u);
        }
        if (!apk_utf8_append_codepoint(out, cap, &w, cp)) return false;
    }
    if (w >= cap) return false;
    out[w] = '\0';
    return true;
}

static bool apk_arsc_entry_at(const uint8_t *type_chunk, uint16_t header_size, uint32_t chunk_size,
                          uint8_t flags, uint32_t entry_count, uint32_t ordinal,
                          uint32_t *out_entry_index, uint32_t *out_entry_offset) {
    if (!type_chunk || !out_entry_index || !out_entry_offset || ordinal >= entry_count ||
        header_size > chunk_size) return false;
    const uint8_t *index = type_chunk + header_size;
    const size_t index_bytes = (size_t)(type_chunk + chunk_size - index);
    if (flags & RES_TABLE_TYPE_FLAG_SPARSE) {
        if ((uint64_t)entry_count * 4u > index_bytes) return false;
        *out_entry_index = apk_rd16(index + (size_t)ordinal * 4u);
        *out_entry_offset = (uint32_t)apk_rd16(index + (size_t)ordinal * 4u + 2u) * 4u;
        return true;
    }
    *out_entry_index = ordinal;
    if (flags & RES_TABLE_TYPE_FLAG_OFFSET16) {
        if ((uint64_t)entry_count * 2u > index_bytes) return false;
        const uint16_t off16 = apk_rd16(index + (size_t)ordinal * 2u);
        if (off16 == 0xffffu) return false;
        *out_entry_offset = (uint32_t)off16 * 4u;
        return true;
    }
    if ((uint64_t)entry_count * 4u > index_bytes) return false;
    const uint32_t off32 = apk_rd32(index + (size_t)ordinal * 4u);
    if (off32 == UINT32_C(0xffffffff)) return false;
    *out_entry_offset = off32;
    return true;
}

static bool apk_resource_name_split(const char *name, const char *def_type, const char *def_package,
                                char *out_entry, size_t entry_cap,
                                char *out_type, size_t type_cap,
                                char *out_package, size_t package_cap) {
    if (!name || !name[0] || !out_entry || !entry_cap || !out_type || !type_cap ||
        !out_package || !package_cap) return false;
    out_entry[0] = out_type[0] = out_package[0] = '\0';
    const char *entry = name;
    const char *slash = strchr(name, '/');
    const char *colon = strchr(name, ':');
    if (colon && (!slash || colon < slash)) {
        const size_t n = (size_t)(colon - name);
        if (n >= package_cap) return false;
        memcpy(out_package, name, n); out_package[n] = '\0';
        entry = colon + 1;
    } else if (def_package && def_package[0]) {
        if (!apk_copy_string(out_package, package_cap, def_package)) return false;
    }
    slash = strchr(entry, '/');
    if (slash) {
        const size_t n = (size_t)(slash - entry);
        if (n >= type_cap) return false;
        memcpy(out_type, entry, n); out_type[n] = '\0';
        entry = slash + 1;
    } else if (def_type && def_type[0]) {
        if (!apk_copy_string(out_type, type_cap, def_type)) return false;
    }
    return entry[0] && apk_copy_string(out_entry, entry_cap, entry);
}

/* resources.arsc of an installed package is immutable while it runs, and the
 * runtime asks it for strings, booleans and identifiers repeatedly (some
 * libraries on every call of a hot path). Each table is read from the APK
 * once and kept for the process lifetime; entries are never evicted, so a
 * returned pointer stays valid. Beyond the cache size a private copy is
 * returned (*out_owned) for the caller to free. */
#define APK_ARSC_CACHE_SLOTS 4u
typedef struct apk_ApkArscCacheEntry {
    char *path;
    uint8_t *data;
    size_t size;
} apk_ApkArscCacheEntry;
static apk_ApkArscCacheEntry apk_g_arsc_cache[APK_ARSC_CACHE_SLOTS];
static pthread_mutex_t apk_g_arsc_cache_lock = PTHREAD_MUTEX_INITIALIZER;

static SsrnxResult apk_arsc_table_cached(const char *apk_path, const uint8_t **out_data,
                                     size_t *out_size, bool *out_owned) {
    *out_data = NULL; *out_size = 0; *out_owned = false;
    SSRNX_PERF_LOCK(&apk_g_arsc_cache_lock);
    for (size_t i = 0; i < APK_ARSC_CACHE_SLOTS; ++i) {
        if (apk_g_arsc_cache[i].path && strcmp(apk_g_arsc_cache[i].path, apk_path) == 0) {
            *out_data = apk_g_arsc_cache[i].data;
            *out_size = apk_g_arsc_cache[i].size;
            pthread_mutex_unlock(&apk_g_arsc_cache_lock);
            return SSRNX_OK;
        }
    }
    pthread_mutex_unlock(&apk_g_arsc_cache_lock);

    ZipDirectory zip;
    SsrnxResult rc = apk_zip_open(apk_path, &zip);
    if (rc != SSRNX_OK) return rc;
    apk_ZipEntryInfo table_entry;
    if (apk_zip_find_entry(&zip, "resources.arsc", &table_entry) != SSRNX_OK ||
        table_entry.uncompressed_size == 0 || table_entry.uncompressed_size > 32u * 1024u * 1024u) {
        apk_zip_close(&zip);
        return SSRNX_ERR_SYMBOL_NOT_FOUND;
    }
    uint8_t *arsc = NULL; size_t arsc_size = 0;
    rc = apk_zip_read_entry_data(&zip, &table_entry, &arsc, &arsc_size);
    apk_zip_close(&zip);
    if (rc != SSRNX_OK) return rc;

    SSRNX_PERF_LOCK(&apk_g_arsc_cache_lock);
    for (size_t i = 0; i < APK_ARSC_CACHE_SLOTS; ++i) {
        if (apk_g_arsc_cache[i].path && strcmp(apk_g_arsc_cache[i].path, apk_path) == 0) {
            /* Another thread cached it meanwhile. */
            *out_data = apk_g_arsc_cache[i].data;
            *out_size = apk_g_arsc_cache[i].size;
            pthread_mutex_unlock(&apk_g_arsc_cache_lock);
            free(arsc);
            return SSRNX_OK;
        }
    }
    for (size_t i = 0; i < APK_ARSC_CACHE_SLOTS; ++i) {
        if (apk_g_arsc_cache[i].path) continue;
        const size_t n = strlen(apk_path) + 1u;
        char *path = malloc(n);
        if (!path) break;
        memcpy(path, apk_path, n);
        apk_g_arsc_cache[i].data = arsc;
        apk_g_arsc_cache[i].size = arsc_size;
        apk_g_arsc_cache[i].path = path;
        *out_data = arsc;
        *out_size = arsc_size;
        pthread_mutex_unlock(&apk_g_arsc_cache_lock);
        return SSRNX_OK;
    }
    pthread_mutex_unlock(&apk_g_arsc_cache_lock);
    *out_data = arsc;
    *out_size = arsc_size;
    *out_owned = true;
    return SSRNX_OK;
}

/* The binary AndroidManifest.xml is cached the same way: PackageManager
 * component queries (getServiceInfo/getReceiverInfo/getProviderInfo,
 * setComponentEnabledSetting's declaration check, meta-data) read it on
 * every call, and inflating it plus resources.arsc per call dominated
 * startup profiles of apps that query their own components in loops. */
#define APK_MANIFEST_CACHE_SLOTS 4u
static apk_ApkArscCacheEntry apk_g_manifest_cache[APK_MANIFEST_CACHE_SLOTS];

static SsrnxResult apk_manifest_xml_cached(const char *apk_path, const uint8_t **out_data,
                                       size_t *out_size, bool *out_owned) {
    *out_data = NULL; *out_size = 0; *out_owned = false;
    SSRNX_PERF_LOCK(&apk_g_arsc_cache_lock);
    for (size_t i = 0; i < APK_MANIFEST_CACHE_SLOTS; ++i) {
        if (apk_g_manifest_cache[i].path && strcmp(apk_g_manifest_cache[i].path, apk_path) == 0) {
            *out_data = apk_g_manifest_cache[i].data;
            *out_size = apk_g_manifest_cache[i].size;
            pthread_mutex_unlock(&apk_g_arsc_cache_lock);
            return SSRNX_OK;
        }
    }
    pthread_mutex_unlock(&apk_g_arsc_cache_lock);

    ZipDirectory zip;
    SsrnxResult rc = apk_zip_open(apk_path, &zip);
    if (rc != SSRNX_OK) return rc;
    apk_ZipEntryInfo manifest_entry;
    uint8_t *xml = NULL; size_t xml_size = 0;
    rc = apk_zip_find_entry(&zip, "AndroidManifest.xml", &manifest_entry);
    if (rc == SSRNX_OK) rc = apk_zip_read_entry_data(&zip, &manifest_entry, &xml, &xml_size);
    apk_zip_close(&zip);
    if (rc != SSRNX_OK) return rc;

    SSRNX_PERF_LOCK(&apk_g_arsc_cache_lock);
    for (size_t i = 0; i < APK_MANIFEST_CACHE_SLOTS; ++i) {
        if (apk_g_manifest_cache[i].path && strcmp(apk_g_manifest_cache[i].path, apk_path) == 0) {
            *out_data = apk_g_manifest_cache[i].data;
            *out_size = apk_g_manifest_cache[i].size;
            pthread_mutex_unlock(&apk_g_arsc_cache_lock);
            free(xml);
            return SSRNX_OK;
        }
    }
    for (size_t i = 0; i < APK_MANIFEST_CACHE_SLOTS; ++i) {
        if (apk_g_manifest_cache[i].path) continue;
        const size_t n = strlen(apk_path) + 1u;
        char *path = malloc(n);
        if (!path) break;
        memcpy(path, apk_path, n);
        apk_g_manifest_cache[i].data = xml;
        apk_g_manifest_cache[i].size = xml_size;
        apk_g_manifest_cache[i].path = path;
        *out_data = xml;
        *out_size = xml_size;
        pthread_mutex_unlock(&apk_g_arsc_cache_lock);
        return SSRNX_OK;
    }
    pthread_mutex_unlock(&apk_g_arsc_cache_lock);
    *out_data = xml;
    *out_size = xml_size;
    *out_owned = true;
    return SSRNX_OK;
}

/* apk style */
/* Style bags share the checked table view/configuration ranking. Package 0x01
 * is served by the pinned Android resource table; unknown providers still fail. */
extern SsrnxResult ssrnx_framework_resource_table(const uint8_t**,size_t*) __attribute__((weak));
static SsrnxResult apk_arsc_resource_view(const apk_ApkArscView *package,uint32_t id,apk_ApkArscView *out) {
    if((id>>24)!=1){*out=*package;return SSRNX_OK;}
    const uint8_t *data=NULL;size_t size=0;
    SsrnxResult rc=ssrnx_framework_resource_table?ssrnx_framework_resource_table(&data,&size):SSRNX_ERR_APK_UNSUPPORTED;
    if(rc==SSRNX_OK&&!apk_arsc_view_init(out,data,size))rc=SSRNX_ERR_APK_INVALID;
    return rc;
}
static uint32_t apk_arsc_java_config(uint32_t native) {
    /* ActivityInfo.CONFIG_NATIVE_BITS, Android 11. */
    const uint32_t bits[]={2,1,4,8,16,32,64,128,0x800,0x1000,0x200,0x2000,0x100,0x4000,0x10000};
    uint32_t java=0;for(size_t i=0;i<sizeof(bits)/sizeof(bits[0]);++i)if(native&bits[i])java|=1u<<i;
    return java;
}
static SsrnxResult apk_arsc_style_entry_value(const apk_ApkArscView *view,uint32_t id,const uint8_t **out,size_t *remaining,SsrnxStyleValue *metadata) {
    apk_ApkResourceRank best={.score=INT_MIN};bool package_found=false;*out=NULL;
    for(size_t off=apk_rd16(view->data+2);off+8<=view->root_size;){
        const uint8_t *package=view->data+off;uint16_t hs=apk_rd16(package+2);uint32_t size=apk_rd32(package+4);
        if(hs<8||size<hs||size>view->root_size-off)return SSRNX_ERR_APK_INVALID;
        if(apk_rd16(package)==RES_TABLE_PACKAGE_TYPE&&hs>=284&&apk_rd32(package+8)==(id>>24)){
            package_found=true;uint32_t type_offset=hs>=288?apk_rd32(package+284):0;
            for(size_t pos=hs;pos+8<=size;){
                const uint8_t *type=package+pos;uint16_t th=apk_rd16(type+2);uint32_t ts=apk_rd32(type+4);
                if(th<8||ts<th||ts>size-pos)return SSRNX_ERR_APK_INVALID;
                if(apk_rd16(type)==0x0202&&th>=16&&type[8]+type_offset==((id>>16)&255)){
                    uint32_t count=apk_rd32(type+12);if((uint64_t)count*4>ts-th)return SSRNX_ERR_APK_INVALID;
                    if(metadata&&(id&65535)<count)metadata->changing_configurations|=apk_arsc_java_config(apk_rd32(type+th+(size_t)(id&65535)*4));
                }
                if(apk_rd16(type)==RES_TABLE_TYPE_TYPE&&th>=24&&type[8]+type_offset==((id>>16)&255)){
                    uint32_t entry_offset=0,entries=apk_rd32(type+16);
                    if(entries<th||entries>ts)return SSRNX_ERR_APK_INVALID;
                    if(apk_arsc_type_entry_offset(type,th,ts,type[9],apk_rd32(type+12),id&65535,&entry_offset)){
                        if(entry_offset>ts-entries||ts-entries-entry_offset<8)return SSRNX_ERR_APK_INVALID;
                        const uint8_t *config=type+20;uint32_t cs=apk_rd32(config);
                        if(cs<4||cs>th-20u)return SSRNX_ERR_APK_INVALID;
                        uint16_t density=cs>=16?apk_rd16(config+14):0;
                        apk_ApkResourceRank rank=apk_resource_config_rank(view,config,cs);
                        if(apk_resource_rank_better(&rank,&best)){best=rank;*out=type+entries+entry_offset;*remaining=ts-entries-entry_offset;if(metadata){metadata->density=density;metadata->asset_cookie=(id>>24)==1?1:2;}}
                    }
                }
                pos+=ts;
            }
        }
        off+=size;
    }
    return *out?SSRNX_OK:package_found?SSRNX_ERR_SYMBOL_NOT_FOUND:SSRNX_ERR_APK_UNSUPPORTED;
}
static SsrnxResult apk_arsc_style_attr(const apk_ApkArscView *view,uint32_t style,uint32_t attr,
                                  SsrnxStyleValue *out,apk_ApkArscView *value_view,unsigned depth) {
    if(depth>=32)return SSRNX_ERR_APK_INVALID;
    apk_ApkArscView selected;SsrnxResult rc=apk_arsc_resource_view(view,style,&selected);
    if(rc!=SSRNX_OK)return rc;
    const uint8_t *entry=NULL;size_t size=0;rc=apk_arsc_style_entry_value(&selected,style,&entry,&size,out);
    if(rc!=SSRNX_OK)return rc;
    uint16_t hs=apk_rd16(entry),flags=apk_rd16(entry+2);
    if(!(flags&RES_TABLE_ENTRY_FLAG_COMPLEX))return SSRNX_ERR_SYMBOL_NOT_FOUND;
    if(hs<16||hs>size)return SSRNX_ERR_APK_INVALID;
    uint32_t count=apk_rd32(entry+12);if((uint64_t)count*12>size-hs)return SSRNX_ERR_APK_INVALID;
    for(uint32_t i=0;i<count;++i){const uint8_t *map=entry+hs+(size_t)i*12;
        if(apk_rd16(map+4)<8)return SSRNX_ERR_APK_INVALID;
        if(apk_rd32(map)==attr){out->type=map[7];out->data=apk_rd32(map+8);*value_view=selected;return SSRNX_OK;}}
    uint32_t parent=apk_rd32(entry+8);
    return parent?apk_arsc_style_attr(view,parent,attr,out,value_view,depth+1):SSRNX_ERR_SYMBOL_NOT_FOUND;
}
static SsrnxResult apk_arsc_style_resolve(const apk_ApkArscView *view,uint32_t style,bool refs,SsrnxStyleValue *out) {
    (void)style;
    apk_ApkArscView current=*view;
    uint32_t seen[32];size_t count=0;
    for(;;){
        if(refs&&out->type==RES_VALUE_TYPE_REFERENCE&&out->data){
            if(count>=32)return SSRNX_ERR_APK_INVALID;
            for(size_t i=0;i<count;++i)if(seen[i]==out->data)return SSRNX_ERR_APK_INVALID;
            seen[count++]=out->data;out->resource_id=out->data;
            const uint8_t *entry=NULL;size_t size=0;
            apk_ApkArscView selected;SsrnxResult rc=apk_arsc_resource_view(view,out->data,&selected);if(rc!=SSRNX_OK)return rc;
            rc=apk_arsc_style_entry_value(&selected,out->data,&entry,&size,out);if(rc!=SSRNX_OK)return rc;
            current=selected;
            uint16_t hs=apk_rd16(entry);if(hs<8||hs>size||size-hs<8)return SSRNX_ERR_APK_INVALID;
            if(apk_rd16(entry+2)&RES_TABLE_ENTRY_FLAG_COMPLEX)return SSRNX_OK;
            if(apk_rd16(entry+hs)<8)return SSRNX_ERR_APK_INVALID;
            out->type=entry[hs+3];out->data=apk_rd32(entry+hs+4);
        }else break;
    }
    if(out->type==7||out->type==8)return SSRNX_ERR_APK_UNSUPPORTED;
    if(out->type==RES_VALUE_TYPE_STRING){
        const apk_AxmlStringPool *pool=&current.global_pool;
        if(out->data>=pool->string_count)return SSRNX_ERR_APK_INVALID;
        uint32_t offset=apk_rd32(pool->offsets+(size_t)out->data*4);
        if(offset>=pool->chunk_size-pool->strings_start)return SSRNX_ERR_APK_INVALID;
        const uint8_t *text=pool->strings+offset,*end=pool->chunk+pool->chunk_size;
        uint32_t units=0,bytes=0;size_t capacity=0;
        if(pool->flags&RES_STRING_POOL_UTF8_FLAG){
            if(!apk_axml_decode_len8(&text,end,&units)||!apk_axml_decode_len8(&text,end,&bytes)||(size_t)(end-text)<(size_t)bytes+1)return SSRNX_ERR_APK_INVALID;
            capacity=(size_t)bytes+1;
        }else{
            if(!apk_axml_decode_len16(&text,end,&units)||(uint64_t)units*2+2>(uint64_t)(end-text))return SSRNX_ERR_APK_INVALID;
            capacity=(size_t)units*3+1;
        }
        out->text=malloc(capacity);if(!out->text)return SSRNX_ERR_NO_MEMORY;
        if(!apk_axml_pool_get(pool,out->data,out->text,capacity)){free(out->text);out->text=NULL;return SSRNX_ERR_APK_INVALID;}
    }
    return SSRNX_OK;
}
SsrnxResult ssrnx_apk_resolve_style_attribute(const char *path,uint32_t style,uint32_t attr,bool refs,SsrnxStyleValue *out) {
    if(!path||!style||!attr||!out)return SSRNX_ERR_ARGUMENT;
    memset(out,0,sizeof(*out));const uint8_t *data=NULL;size_t size=0;bool owned=false;
    SsrnxResult rc=apk_arsc_table_cached(path,&data,&size,&owned);if(rc!=SSRNX_OK)return rc;
    apk_ApkArscView view;
    if(!apk_arsc_view_init(&view,data,size))rc=SSRNX_ERR_APK_INVALID;
    else{apk_ApkArscView value_view;rc=apk_arsc_style_attr(&view,style,attr,out,&value_view,0);if(rc==SSRNX_OK)rc=apk_arsc_style_resolve(&value_view,style,refs,out);}
    if(owned)free((void*)data);
    return rc;
}



/* apk layout */
/* Layout parsing shares the manifest's checked Android string-pool decoder. */
void ssrnx_apk_free_xml(SsrnxXmlDocument *document) {
    if(!document)return;
    for(size_t i=0;i<document->count;++i){SsrnxXmlNode *node=&document->nodes[i];free(node->name);
        for(size_t j=0;j<node->attribute_count;++j){free(node->attributes[j].name);free(node->attributes[j].namespace_uri);free(node->attributes[j].text);}
        free(node->attributes);
    }
    free(document->nodes);memset(document,0,sizeof(*document));
}
static char *apk_layout_pool_string(const apk_AxmlStringPool *pool,uint32_t index) {
    if(index==RES_NO_ENTRY)return strdup("");
    if(index>=pool->string_count)return NULL;
    /* Pool bytes bound the UTF-8 result; UTF-16 can expand by at most 3/2. */
    size_t cap=(size_t)pool->chunk_size*2+1;char *text=malloc(cap);
    if(text&&!apk_axml_pool_get(pool,index,text,cap)){free(text);text=NULL;}
    if(text){char *small=realloc(text,strlen(text)+1);if(small)text=small;}
    return text;
}
SsrnxResult ssrnx_apk_parse_binary_xml(const uint8_t *data,size_t size,SsrnxXmlDocument *out) {
    if(!out)return SSRNX_ERR_ARGUMENT;
    memset(out,0,sizeof(*out));
    if(!data||size<8||apk_rd16(data)!=RES_XML_TYPE)return SSRNX_ERR_APK_INVALID;
    size_t total=apk_rd32(data+4),off=apk_rd16(data+2);
    if(off<8||total>size||total<off)return SSRNX_ERR_APK_INVALID;
    apk_AxmlStringPool pool={0};bool have_pool=false;const uint8_t *map=NULL;size_t map_count=0;
    int32_t stack[128];size_t depth=0;SsrnxResult rc=SSRNX_ERR_APK_INVALID;
    while(off<total){
        if(total-off<8)goto fail;
        const uint8_t *chunk=data+off;uint16_t type=apk_rd16(chunk),header=apk_rd16(chunk+2);size_t bytes=apk_rd32(chunk+4);
        if(header<8||bytes<header||bytes>total-off)goto fail;
        if(type==RES_STRING_POOL_TYPE){if(have_pool||!apk_axml_pool_init(&pool,chunk,bytes))goto fail;have_pool=true;}
        else if(type==0x0180){if((bytes-header)%4)goto fail;map=chunk+header;map_count=(bytes-header)/4;}
        else if(type==RES_XML_START_ELEMENT_TYPE){
            if(!have_pool||header!=16||bytes<36||depth==128||out->count>=8192)goto fail;
            const uint8_t *ext=chunk+16;size_t start=16u+apk_rd16(ext+8),stride=apk_rd16(ext+10),count=apk_rd16(ext+12);
            if(start<36||stride<20||start>bytes||count>(bytes-start)/stride)goto fail;
            SsrnxXmlNode *nodes=realloc(out->nodes,(out->count+1)*sizeof(*nodes));
            if(!nodes){rc=SSRNX_ERR_NO_MEMORY;goto fail;}out->nodes=nodes;
            size_t index=out->count++;SsrnxXmlNode *node=&nodes[index];memset(node,0,sizeof(*node));
            node->parent=depth?stack[depth-1]:-1;node->line=apk_rd32(chunk+8);
            if(index&&node->parent<0)goto fail;
            node->name=apk_layout_pool_string(&pool,apk_rd32(ext+4));if(!node->name)goto fail;
            node->attributes=calloc(count?count:1,sizeof(*node->attributes));
            if(!node->attributes){rc=SSRNX_ERR_NO_MEMORY;goto fail;}node->attribute_count=count;
            for(size_t i=0;i<count;++i){const uint8_t *raw=chunk+start+i*stride;SsrnxXmlAttribute *a=&node->attributes[i];
                if(apk_rd16(raw+12)!=8)goto fail;
                uint32_t name=apk_rd32(raw+4);a->name=apk_layout_pool_string(&pool,name);a->namespace_uri=apk_layout_pool_string(&pool,apk_rd32(raw));
                a->name_resource=name<map_count?apk_rd32(map+(size_t)name*4):0;a->type=raw[15];a->data=apk_rd32(raw+16);
                uint32_t text=apk_rd32(raw+8);if(text==RES_NO_ENTRY&&a->type==RES_VALUE_TYPE_STRING)text=a->data;
                if(text!=RES_NO_ENTRY)a->text=apk_layout_pool_string(&pool,text);
                if(!a->name||!a->namespace_uri||(text!=RES_NO_ENTRY&&!a->text))goto fail;
            }
            stack[depth++]=(int32_t)index;
        }else if(type==RES_XML_END_ELEMENT_TYPE){
            if(!have_pool||header!=16||bytes<24||!depth)goto fail;
            char *name=apk_layout_pool_string(&pool,apk_rd32(chunk+20));bool same=name&&!strcmp(name,out->nodes[stack[depth-1]].name);free(name);
            if(!same)goto fail;
            --depth;
        }else if(type!=0x0100&&type!=0x0101&&type!=0x0104)goto fail;
        off+=bytes;
    }
    if(depth||!out->count)goto fail;
    return SSRNX_OK;
fail:ssrnx_apk_free_xml(out);return rc;
}
static SsrnxResult apk_arsc_get_resource_value(const char *path,uint32_t id,bool refs,bool bags,SsrnxStyleValue *out) {
    if(!out)return SSRNX_ERR_ARGUMENT;
    memset(out,0,sizeof(*out));
    if(!id||!((id>>16)&255))return SSRNX_ERR_SYMBOL_NOT_FOUND;
    const uint8_t *data=NULL;size_t size=0;bool owned=false;apk_ApkArscView view;
    SsrnxResult rc=(id>>24)==1?ssrnx_framework_resource_table?ssrnx_framework_resource_table(&data,&size):SSRNX_ERR_APK_UNSUPPORTED:apk_arsc_table_cached(path,&data,&size,&owned);if(rc!=SSRNX_OK)return rc;
    if(!apk_arsc_view_init(&view,data,size))rc=SSRNX_ERR_APK_INVALID;
    else{
        const uint8_t *entry=NULL;size_t remaining=0;
        rc=apk_arsc_style_entry_value(&view,id,&entry,&remaining,out);
        if(rc==SSRNX_OK){uint16_t hs=apk_rd16(entry);out->resource_id=id;
            if(hs<8||hs>remaining||remaining-hs<8)rc=SSRNX_ERR_APK_INVALID;
            else if(apk_rd16(entry+2)&RES_TABLE_ENTRY_FLAG_COMPLEX){
                if(bags){out->type=RES_VALUE_TYPE_REFERENCE;out->data=id;}
                else rc=SSRNX_ERR_SYMBOL_NOT_FOUND;
            }
            else if(apk_rd16(entry+hs)<8)rc=SSRNX_ERR_APK_INVALID;
            else{out->type=entry[hs+3];out->data=apk_rd32(entry+hs+4);rc=apk_arsc_style_resolve(&view,0,refs,out);}
        }
    }
    if(owned)free((void*)data);
    return rc;
}
SsrnxResult ssrnx_apk_get_resource_value(const char *path,uint32_t id,bool refs,SsrnxStyleValue *out) {
    return apk_arsc_get_resource_value(path,id,refs,false,out);
}
SsrnxResult ssrnx_apk_resolve_value(const char *path,uint32_t id,SsrnxStyleValue *out) {
    return apk_arsc_get_resource_value(path,id,true,true,out);
}



SsrnxResult ssrnx_apk_find_resource_id(const char *apk_path, const char *name,
                                       const char *def_type, const char *def_package,
                                       uint32_t *out_resource_id) {
    if (!apk_path || !name || !out_resource_id) return SSRNX_ERR_ARGUMENT;
    *out_resource_id = 0;
    char wanted_entry[256], wanted_type[128], wanted_package[256];
    if (!apk_resource_name_split(name, def_type, def_package,
                             wanted_entry, sizeof(wanted_entry),
                             wanted_type, sizeof(wanted_type),
                             wanted_package, sizeof(wanted_package)))
        return SSRNX_ERR_ARGUMENT;

    const uint8_t *arsc_table = NULL; size_t arsc_size = 0; bool arsc_owned = false;
    SsrnxResult rc = apk_arsc_table_cached(apk_path, &arsc_table, &arsc_size, &arsc_owned);
    if (rc != SSRNX_OK) return rc;
    uint8_t *arsc = arsc_owned ? (uint8_t *)arsc_table : NULL; /* freed below when owned */
    apk_ApkArscView view;
    if (!apk_arsc_view_init(&view, arsc_table, arsc_size)) { free(arsc); return SSRNX_ERR_APK_INVALID; }

    const uint16_t root_header = apk_rd16(view.data + 2);
    size_t off = root_header;
    rc = SSRNX_ERR_SYMBOL_NOT_FOUND;
    while (off + 8u <= view.root_size) {
        const uint8_t *package = view.data + off;
        const uint16_t chunk_type = apk_rd16(package);
        const uint16_t package_header = apk_rd16(package + 2);
        const uint32_t package_size = apk_rd32(package + 4);
        if (package_header < 8u || package_size < package_header || off + package_size > view.root_size) {
            rc = SSRNX_ERR_APK_INVALID; break;
        }
        if (chunk_type != RES_TABLE_PACKAGE_TYPE || package_header < 284u) { off += package_size; continue; }
        const uint32_t package_id = apk_rd32(package + 8);
        char package_name[256];
        if (!apk_arsc_package_name_utf8(package, package_header, package_name, sizeof(package_name))) {
            off += package_size; continue;
        }
        if (wanted_package[0] && strcmp(wanted_package, package_name) != 0) { off += package_size; continue; }
        const uint32_t type_strings_off = apk_rd32(package + 268u);
        const uint32_t key_strings_off = apk_rd32(package + 276u);
        const uint32_t type_id_offset = package_header >= 288u ? apk_rd32(package + 284u) : 0u;
        if (type_strings_off >= package_size || key_strings_off >= package_size) { off += package_size; continue; }
        apk_AxmlStringPool type_pool, key_pool;
        if (!apk_axml_pool_init(&type_pool, package + type_strings_off, package_size - type_strings_off) ||
            !apk_axml_pool_init(&key_pool, package + key_strings_off, package_size - key_strings_off)) {
            off += package_size; continue;
        }

        size_t child_off = package_header;
        while (child_off + 8u <= package_size) {
            const uint8_t *child = package + child_off;
            const uint16_t child_type = apk_rd16(child);
            const uint16_t child_header = apk_rd16(child + 2);
            const uint32_t child_size = apk_rd32(child + 4);
            if (child_header < 8u || child_size < child_header || child_off + child_size > package_size) break;
            if (child_type == RES_TABLE_TYPE_TYPE && child_header >= 24u) {
                const uint8_t raw_type_id = child[8];
                const uint8_t flags = child[9];
                if (raw_type_id != 0) {
                    char type_name[128];
                    if (apk_axml_pool_get(&type_pool, (uint32_t)raw_type_id - 1u, type_name, sizeof(type_name)) &&
                        (!wanted_type[0] || strcmp(type_name, wanted_type) == 0)) {
                        const uint32_t entry_count = apk_rd32(child + 12u);
                        const uint32_t entries_start = apk_rd32(child + 16u);
                        if (entries_start < child_size) {
                            for (uint32_t ordinal = 0; ordinal < entry_count; ++ordinal) {
                                uint32_t entry_index = 0, entry_offset = 0;
                                if (!apk_arsc_entry_at(child, child_header, child_size, flags, entry_count,
                                                   ordinal, &entry_index, &entry_offset)) continue;
                                if (entry_offset > child_size - entries_start) continue;
                                const size_t absolute = (size_t)entries_start + entry_offset;
                                if (absolute + 8u > child_size) continue;
                                const uint8_t *entry = child + absolute;
                                const uint16_t entry_size = apk_rd16(entry);
                                if (entry_size < 8u || absolute + entry_size > child_size) continue;
                                const uint32_t key_index = apk_rd32(entry + 4u);
                                char key_name[256];
                                if (!apk_axml_pool_get(&key_pool, key_index, key_name, sizeof(key_name)) ||
                                    strcmp(key_name, wanted_entry) != 0) continue;
                                const uint32_t actual_type_id = (uint32_t)raw_type_id + type_id_offset;
                                if (package_id > 0xffu || actual_type_id > 0xffu || entry_index > 0xffffu) continue;
                                *out_resource_id = (package_id << 24) | (actual_type_id << 16) | entry_index;
                                rc = SSRNX_OK;
                                goto done_find_resource;
                            }
                        }
                    }
                }
            }
            child_off += child_size;
        }
        off += package_size;
    }

done_find_resource:
    free(arsc);
    return rc;
}

SsrnxResult ssrnx_apk_resolve_string_resource(const char *apk_path, uint32_t resource_id,
                                              char *out, size_t out_cap) {
    if (!apk_path || !resource_id || !out || out_cap == 0) return SSRNX_ERR_ARGUMENT;
    out[0] = '\0';
    const uint8_t *arsc_table = NULL; size_t arsc_size = 0; bool arsc_owned = false;
    SsrnxResult rc = apk_arsc_table_cached(apk_path, &arsc_table, &arsc_size, &arsc_owned);
    if (rc != SSRNX_OK) return rc;
    uint8_t *arsc = arsc_owned ? (uint8_t *)arsc_table : NULL; /* freed below when owned */
    apk_ApkArscView view;
    if (!apk_arsc_view_init(&view, arsc_table, arsc_size)) { free(arsc); return SSRNX_ERR_APK_INVALID; }
    view.score_mode = APK_RES_SCORE_LABEL;
    apk_ApkResourceCandidate candidates[8];
    const size_t count = apk_arsc_collect_values(&view, resource_id, candidates, 8);
    rc = SSRNX_ERR_SYMBOL_NOT_FOUND;
    for (size_t i = 0; i < count; ++i) {
        if (candidates[i].data_type != RES_VALUE_TYPE_STRING || !candidates[i].path[0]) continue;
        if (apk_copy_string(out, out_cap, candidates[i].path)) { rc = SSRNX_OK; break; }
    }
    free(arsc);
    return rc;
}

SsrnxResult ssrnx_apk_resolve_file_resource(const char *apk_path, uint32_t resource_id,
                                            char *out, size_t out_cap) {
    if (!apk_path || !resource_id || !out || out_cap == 0) return SSRNX_ERR_ARGUMENT;
    out[0] = '\0';
    const uint8_t *arsc_table = NULL; size_t arsc_size = 0; bool arsc_owned = false;
    SsrnxResult rc = apk_arsc_table_cached(apk_path, &arsc_table, &arsc_size, &arsc_owned);
    if (rc != SSRNX_OK) return rc;
    uint8_t *arsc = arsc_owned ? (uint8_t *)arsc_table : NULL; /* freed below when owned */
    apk_ApkArscView view;
    if (!apk_arsc_view_init(&view, arsc_table, arsc_size)) { free(arsc); return SSRNX_ERR_APK_INVALID; }
    view.score_mode = APK_RES_SCORE_DEVICE;
    apk_ApkResourceCandidate candidates[8];
    const size_t count = apk_arsc_collect_values(&view, resource_id, candidates, 8);
    rc = SSRNX_ERR_SYMBOL_NOT_FOUND;
    for (size_t i = 0; i < count; ++i) {
        if (candidates[i].data_type != RES_VALUE_TYPE_STRING || strncmp(candidates[i].path, "res/", 4) != 0)
            continue;
        if (apk_copy_string(out, out_cap, candidates[i].path)) { rc = SSRNX_OK; break; }
    }
    free(arsc);
    return rc;
}

SsrnxResult ssrnx_apk_resolve_boolean_resource(const char *apk_path, uint32_t resource_id,
                                               bool *out_value) {
    if (!apk_path || !resource_id || !out_value) return SSRNX_ERR_ARGUMENT;
    *out_value = false;
    SsrnxStyleValue value = {0};
    SsrnxResult rc = ssrnx_apk_get_resource_value(apk_path, resource_id, true, &value);
    if (rc == SSRNX_OK) {
        if (value.type >= RES_VALUE_TYPE_INT_DEC && value.type <= RES_VALUE_TYPE_INT_COLOR_RGB4)
            *out_value = value.data != 0;
        else rc = SSRNX_ERR_SYMBOL_NOT_FOUND;
    }
    free(value.text);
    return rc;
}

SsrnxResult ssrnx_apk_resolve_color_resource(const char *apk_path, uint32_t resource_id,
                                             uint32_t *out_argb) {
    if (!apk_path || !resource_id || !out_argb) return SSRNX_ERR_ARGUMENT;
    *out_argb = 0;
    const uint8_t *arsc_table = NULL; size_t arsc_size = 0; bool arsc_owned = false;
    SsrnxResult rc = apk_arsc_table_cached(apk_path, &arsc_table, &arsc_size, &arsc_owned);
    if (rc != SSRNX_OK) return rc;
    uint8_t *arsc = arsc_owned ? (uint8_t *)arsc_table : NULL;
    apk_ApkArscView view;
    if (!apk_arsc_view_init(&view, arsc_table, arsc_size)) {
        free(arsc);
        return SSRNX_ERR_APK_INVALID;
    }
    view.score_mode = APK_RES_SCORE_DEVICE;
    apk_ApkResourceCandidate candidates[8];
    const size_t count = apk_arsc_collect_values(&view, resource_id, candidates,
                                              sizeof(candidates) / sizeof(candidates[0]));
    rc = SSRNX_ERR_SYMBOL_NOT_FOUND;
    for (size_t i = 0; i < count; ++i) {
        if (!apk_resource_color_type(candidates[i].data_type)) continue;
        *out_argb = apk_android_color_to_argb(candidates[i].data_type, candidates[i].data);
        rc = SSRNX_OK;
        break;
    }
    free(arsc);
    return rc;
}


/* Store one typed value in a meta-data entry following PackageParser's
 * parseMetaData: string, boolean, any TYPE_*INT* (including colors) and float. */
static bool apk_meta_data_store_typed(SsrnxMetaData *md, uint8_t data_type, uint32_t data,
                                  const char *string_value) {
    if (data_type == RES_VALUE_TYPE_STRING) {
        md->type = SSRNX_META_DATA_STRING;
        return string_value && apk_copy_string(md->string_value, sizeof(md->string_value), string_value);
    }
    if (data_type == RES_VALUE_TYPE_INT_BOOLEAN) {
        md->type = SSRNX_META_DATA_BOOLEAN;
        md->int_value = data != 0;
        return true;
    }
    if (data_type >= RES_VALUE_TYPE_INT_DEC && data_type <= RES_VALUE_TYPE_INT_COLOR_RGB4) {
        md->type = SSRNX_META_DATA_INT;
        md->int_value = (int32_t)data;
        return true;
    }
    if (data_type == RES_VALUE_TYPE_FLOAT) {
        md->type = SSRNX_META_DATA_FLOAT;
        memcpy(&md->float_value, &data, sizeof(md->float_value));
        return true;
    }
    return false;
}

SsrnxResult ssrnx_apk_read_application_meta_data(const char *apk_path, SsrnxMetaData *out,
                                                 size_t cap, size_t *out_count) {
    if (!apk_path || !out_count || (!out && cap)) return SSRNX_ERR_ARGUMENT;
    *out_count = 0;
    const uint8_t *xml = NULL, *arsc = NULL;
    size_t xml_size = 0, arsc_size = 0;
    bool xml_owned = false, arsc_owned = false;
    SsrnxResult rc = apk_manifest_xml_cached(apk_path, &xml, &xml_size, &xml_owned);
    if (rc != SSRNX_OK) return rc;
    /* resources.arsc is optional: only @reference values need it. */
    if (apk_arsc_table_cached(apk_path, &arsc, &arsc_size, &arsc_owned) != SSRNX_OK) { arsc = NULL; arsc_size = 0; }

    apk_ApkArscView view;
    const bool have_view = arsc && apk_arsc_view_init(&view, arsc, arsc_size);
    if (have_view) view.score_mode = APK_RES_SCORE_LABEL;

    rc = SSRNX_ERR_MANIFEST_INVALID;
    if (xml_size < 8 || apk_rd16(xml) != RES_XML_TYPE) goto done;
    const uint16_t root_header = apk_rd16(xml + 2);
    const uint32_t root_size = apk_rd32(xml + 4);
    if (root_header < 8 || root_size > xml_size || root_size < root_header) goto done;

    apk_AxmlStringPool pool;
    memset(&pool, 0, sizeof(pool));
    bool have_pool = false;
    int depth = 0, application_depth = -1;
    for (size_t off = root_header; off + 8 <= root_size;) {
        const uint8_t *chunk = xml + off;
        const uint16_t type = apk_rd16(chunk);
        const uint16_t header_size = apk_rd16(chunk + 2);
        const uint32_t chunk_size = apk_rd32(chunk + 4);
        if (header_size < 8 || chunk_size < header_size || off + chunk_size > root_size) goto done;
        if (type == RES_STRING_POOL_TYPE) {
            if (!apk_axml_pool_init(&pool, chunk, root_size - off)) goto done;
            have_pool = true;
        } else if (type == RES_XML_START_ELEMENT_TYPE) {
            if (!have_pool || chunk_size < 36) goto done;
            const uint8_t *ext = chunk + 16;
            const uint16_t attr_start = apk_rd16(ext + 8), attr_size = apk_rd16(ext + 10), attr_count = apk_rd16(ext + 12);
            const size_t attrs_off = 16u + attr_start;
            if (attr_size < 20 || attrs_off > chunk_size ||
                (uint64_t)attrs_off + (uint64_t)attr_size * attr_count > chunk_size) goto done;
            char tag[128];
            if (!apk_axml_pool_get(&pool, apk_rd32(ext + 4), tag, sizeof(tag))) goto done;
            const uint8_t *attrs = chunk + attrs_off;
            ++depth;
            if (strcmp(tag, "application") == 0 && application_depth < 0) {
                application_depth = depth;
            } else if (strcmp(tag, "meta-data") == 0 && application_depth >= 0 &&
                       depth == application_depth + 1 && *out_count < cap) {
                SsrnxMetaData *md = &out[*out_count];
                memset(md, 0, sizeof(*md));
                if (!apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count, "name",
                                           md->name, sizeof(md->name)) || !md->name[0]) {
                    off += chunk_size;
                    continue;
                }
                bool stored = false;
                uint32_t resource_id = 0;
                const uint8_t *value_attr = apk_axml_find_attr(&pool, attrs, attr_size, attr_count, "value");
                if (apk_axml_attr_resource_id(&pool, attrs, attr_size, attr_count, "resource", &resource_id)) {
                    md->type = SSRNX_META_DATA_INT;
                    md->int_value = (int32_t)resource_id;
                    stored = true;
                } else if (value_attr && apk_rd32(value_attr + 8) != RES_NO_ENTRY) {
                    char text[sizeof(md->string_value)];
                    stored = apk_axml_pool_get(&pool, apk_rd32(value_attr + 8), text, sizeof(text)) &&
                             apk_meta_data_store_typed(md, RES_VALUE_TYPE_STRING, 0, text);
                } else if (value_attr && apk_rd16(value_attr + 12) >= 8) {
                    uint8_t data_type = value_attr[15];
                    uint32_t data = apk_rd32(value_attr + 16);
                    char text[sizeof(md->string_value)] = "";
                    if (data_type == RES_VALUE_TYPE_STRING) {
                        (void)apk_axml_pool_get(&pool, data, text, sizeof(text));
                    } else if ((data_type == RES_VALUE_TYPE_REFERENCE ||
                                data_type == RES_VALUE_TYPE_DYNAMIC_REFERENCE) && have_view) {
                        /* TypedArray.peekValue resolves android:value references. */
                        apk_ApkResourceCandidate candidates[8];
                        const size_t n = apk_arsc_collect_values(&view, data, candidates, 8);
                        data_type = 0;
                        for (size_t i = 0; i < n; ++i) {
                            if (candidates[i].data_type == RES_VALUE_TYPE_REFERENCE) continue;
                            data_type = candidates[i].data_type;
                            data = candidates[i].data;
                            if (data_type == RES_VALUE_TYPE_STRING)
                                (void)apk_copy_string(text, sizeof(text), candidates[i].path);
                            break;
                        }
                    }
                    stored = apk_meta_data_store_typed(md, data_type, data, text);
                }
                /* PackageParser also drops unsupported value types. */
                if (stored) ++*out_count;
            }
        } else if (type == RES_XML_END_ELEMENT_TYPE) {
            if (depth == application_depth) application_depth = -2; /* only the first <application> */
            if (depth > 0) --depth;
        }
        off += chunk_size;
    }
    rc = SSRNX_OK;
done:
    if (xml_owned) free((void *)xml);
    if (arsc_owned) free((void *)arsc);
    return rc;
}


static bool apk_manifest_provider_name_matches(const char *package_name, const char *declared,
                                           const char *wanted) {
    if (!package_name || !declared || !wanted || !declared[0] || !wanted[0]) return false;
    char expanded[SSRNX_APK_ACTIVITY_CAP + SSRNX_APK_PACKAGE_CAP + 4u];
    if (declared[0] == '.') {
        if (snprintf(expanded, sizeof(expanded), "%s%s", package_name, declared) >= (int)sizeof(expanded))
            return false;
    } else if (!strchr(declared, '.')) {
        if (snprintf(expanded, sizeof(expanded), "%s.%s", package_name, declared) >= (int)sizeof(expanded))
            return false;
    } else if (!apk_copy_string(expanded, sizeof(expanded), declared)) {
        return false;
    }
    char normalized_wanted[sizeof(expanded)];
    if (!apk_copy_string(normalized_wanted, sizeof(normalized_wanted), wanted)) return false;
    for (char *c = expanded; *c; ++c) if (*c == '/') *c = '.';
    for (char *c = normalized_wanted; *c; ++c) if (*c == '/') *c = '.';
    return strcmp(expanded, normalized_wanted) == 0;
}

SsrnxResult ssrnx_apk_read_component_meta_data(const char *apk_path, const char *package_name,
                                               const char *component_tag, const char *provider_class_name,
                                               SsrnxMetaData *out, size_t cap, size_t *out_count) {
    if (!apk_path || !package_name || !component_tag || !provider_class_name || !out_count || (!out && cap))
        return SSRNX_ERR_ARGUMENT;
    *out_count = 0;
    const uint8_t *xml = NULL, *arsc = NULL;
    size_t xml_size = 0, arsc_size = 0;
    bool xml_owned = false, arsc_owned = false;
    SsrnxResult rc = apk_manifest_xml_cached(apk_path, &xml, &xml_size, &xml_owned);
    if (rc != SSRNX_OK) return rc;
    /* resources.arsc is optional: only @reference values need it. */
    if (apk_arsc_table_cached(apk_path, &arsc, &arsc_size, &arsc_owned) != SSRNX_OK) { arsc = NULL; arsc_size = 0; }

    apk_ApkArscView view;
    const bool have_view = arsc && apk_arsc_view_init(&view, arsc, arsc_size);
    if (have_view) view.score_mode = APK_RES_SCORE_LABEL;

    rc = SSRNX_ERR_MANIFEST_INVALID;
    if (xml_size < 8 || apk_rd16(xml) != RES_XML_TYPE) goto done;
    const uint16_t root_header = apk_rd16(xml + 2);
    const uint32_t root_size = apk_rd32(xml + 4);
    if (root_header < 8 || root_size > xml_size || root_size < root_header) goto done;

    apk_AxmlStringPool pool;
    memset(&pool, 0, sizeof(pool));
    bool have_pool = false;
    bool found_provider = false;
    int depth = 0, provider_depth = -1;
    for (size_t off = root_header; off + 8 <= root_size;) {
        const uint8_t *chunk = xml + off;
        const uint16_t type = apk_rd16(chunk);
        const uint16_t header_size = apk_rd16(chunk + 2);
        const uint32_t chunk_size = apk_rd32(chunk + 4);
        if (header_size < 8 || chunk_size < header_size || off + chunk_size > root_size) goto done;
        if (type == RES_STRING_POOL_TYPE) {
            if (!apk_axml_pool_init(&pool, chunk, root_size - off)) goto done;
            have_pool = true;
        } else if (type == RES_XML_START_ELEMENT_TYPE) {
            if (!have_pool || chunk_size < 36) goto done;
            const uint8_t *ext = chunk + 16;
            const uint16_t attr_start = apk_rd16(ext + 8), attr_size = apk_rd16(ext + 10), attr_count = apk_rd16(ext + 12);
            const size_t attrs_off = 16u + attr_start;
            if (attr_size < 20 || attrs_off > chunk_size ||
                (uint64_t)attrs_off + (uint64_t)attr_size * attr_count > chunk_size) goto done;
            char tag[128];
            if (!apk_axml_pool_get(&pool, apk_rd32(ext + 4), tag, sizeof(tag))) goto done;
            const uint8_t *attrs = chunk + attrs_off;
            ++depth;
            if (!found_provider && strcmp(tag, component_tag) == 0) {
                char declared[SSRNX_APK_ACTIVITY_CAP] = {0};
                if (apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count, "name",
                                          declared, sizeof(declared)) &&
                    apk_manifest_provider_name_matches(package_name, declared, provider_class_name)) {
                    found_provider = true;
                    provider_depth = depth;
                }
            } else if (found_provider && provider_depth >= 0 && strcmp(tag, "meta-data") == 0 &&
                       depth == provider_depth + 1 && *out_count < cap) {
                SsrnxMetaData *md = &out[*out_count];
                memset(md, 0, sizeof(*md));
                if (!apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count, "name",
                                           md->name, sizeof(md->name)) || !md->name[0]) {
                    off += chunk_size;
                    continue;
                }
                bool stored = false;
                uint32_t resource_id = 0;
                const uint8_t *value_attr = apk_axml_find_attr(&pool, attrs, attr_size, attr_count, "value");
                if (apk_axml_attr_resource_id(&pool, attrs, attr_size, attr_count, "resource", &resource_id)) {
                    md->type = SSRNX_META_DATA_INT;
                    md->int_value = (int32_t)resource_id;
                    stored = true;
                } else if (value_attr && apk_rd32(value_attr + 8) != RES_NO_ENTRY) {
                    char text[sizeof(md->string_value)];
                    stored = apk_axml_pool_get(&pool, apk_rd32(value_attr + 8), text, sizeof(text)) &&
                             apk_meta_data_store_typed(md, RES_VALUE_TYPE_STRING, 0, text);
                } else if (value_attr && apk_rd16(value_attr + 12) >= 8) {
                    uint8_t data_type = value_attr[15];
                    uint32_t data = apk_rd32(value_attr + 16);
                    char text[sizeof(md->string_value)] = "";
                    if (data_type == RES_VALUE_TYPE_STRING) {
                        (void)apk_axml_pool_get(&pool, data, text, sizeof(text));
                    } else if ((data_type == RES_VALUE_TYPE_REFERENCE ||
                                data_type == RES_VALUE_TYPE_DYNAMIC_REFERENCE) && have_view) {
                        apk_ApkResourceCandidate candidates[8];
                        const size_t value_count = apk_arsc_collect_values(&view, data, candidates, 8);
                        data_type = 0;
                        for (size_t i = 0; i < value_count; ++i) {
                            if (candidates[i].data_type == RES_VALUE_TYPE_REFERENCE) continue;
                            data_type = candidates[i].data_type;
                            data = candidates[i].data;
                            if (data_type == RES_VALUE_TYPE_STRING)
                                (void)apk_copy_string(text, sizeof(text), candidates[i].path);
                            break;
                        }
                    }
                    stored = apk_meta_data_store_typed(md, data_type, data, text);
                }
                if (stored) ++*out_count;
            }
        } else if (type == RES_XML_END_ELEMENT_TYPE) {
            if (found_provider && depth == provider_depth) provider_depth = -2;
            if (depth > 0) --depth;
        }
        off += chunk_size;
    }
    rc = found_provider ? SSRNX_OK : SSRNX_ERR_SYMBOL_NOT_FOUND;
done:
    if (xml_owned) free((void *)xml);
    if (arsc_owned) free((void *)arsc);
    return rc;
}

SsrnxResult ssrnx_apk_read_provider_meta_data(const char *apk_path, const char *package_name,
                                              const char *provider_class_name, SsrnxMetaData *out,
                                              size_t cap, size_t *out_count) {
    return ssrnx_apk_read_component_meta_data(apk_path, package_name, "provider", provider_class_name,
                                              out, cap, out_count);
}

SsrnxResult ssrnx_apk_list_components(const char *apk_path, const char *component_tag,
                                      SsrnxManifestComponent *out, size_t cap, size_t *out_count) {
    if (!apk_path || !component_tag || !out_count || (!out && cap)) return SSRNX_ERR_ARGUMENT;
    *out_count = 0;
    const uint8_t *xml = NULL;
    size_t xml_size = 0;
    bool xml_owned = false;
    SsrnxResult rc = apk_manifest_xml_cached(apk_path, &xml, &xml_size, &xml_owned);
    if (rc != SSRNX_OK) return rc;

    rc = SSRNX_ERR_MANIFEST_INVALID;
    if (xml_size < 8 || apk_rd16(xml) != RES_XML_TYPE) goto done;
    const uint16_t root_header = apk_rd16(xml + 2);
    const uint32_t root_size = apk_rd32(xml + 4);
    if (root_header < 8 || root_size > xml_size || root_size < root_header) goto done;

    apk_AxmlStringPool pool;
    memset(&pool, 0, sizeof(pool));
    bool have_pool = false;
    int depth = 0, application_depth = -1;
    for (size_t off = root_header; off + 8 <= root_size;) {
        const uint8_t *chunk = xml + off;
        const uint16_t type = apk_rd16(chunk);
        const uint16_t header_size = apk_rd16(chunk + 2);
        const uint32_t chunk_size = apk_rd32(chunk + 4);
        if (header_size < 8 || chunk_size < header_size || off + chunk_size > root_size) goto done;
        if (type == RES_STRING_POOL_TYPE) {
            if (!apk_axml_pool_init(&pool, chunk, root_size - off)) goto done;
            have_pool = true;
        } else if (type == RES_XML_START_ELEMENT_TYPE) {
            if (!have_pool || chunk_size < 36) goto done;
            const uint8_t *ext = chunk + 16;
            const uint16_t attr_start = apk_rd16(ext + 8), attr_size = apk_rd16(ext + 10), attr_count = apk_rd16(ext + 12);
            const size_t attrs_off = 16u + attr_start;
            if (attr_size < 20 || attrs_off > chunk_size ||
                (uint64_t)attrs_off + (uint64_t)attr_size * attr_count > chunk_size) goto done;
            char tag[128];
            if (!apk_axml_pool_get(&pool, apk_rd32(ext + 4), tag, sizeof(tag))) goto done;
            const uint8_t *attrs = chunk + attrs_off;
            ++depth;
            if (application_depth < 0 && strcmp(tag, "application") == 0) {
                application_depth = depth;
            } else if (application_depth >= 0 && depth == application_depth + 1 &&
                       strcmp(tag, component_tag) == 0 && *out_count < cap) {
                SsrnxManifestComponent *c = &out[*out_count];
                memset(c, 0, sizeof(*c));
                c->enabled = true;
                c->exported_declared = apk_axml_attr_bool(&pool, attrs, attr_size, attr_count, "exported",
                                                      &c->exported);
                (void)apk_axml_attr_bool(&pool, attrs, attr_size, attr_count, "enabled", &c->enabled);
                (void)apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count, "targetActivity",
                                            c->target_activity, sizeof(c->target_activity));
                if (apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count, "name",
                                          c->class_name, sizeof(c->class_name)) && c->class_name[0])
                    ++*out_count;
            }
        } else if (type == RES_XML_END_ELEMENT_TYPE) {
            if (depth == application_depth) application_depth = -2;
            if (depth > 0) --depth;
        }
        off += chunk_size;
    }
    rc = SSRNX_OK;
done:
    if (xml_owned) free((void *)xml);
    return rc;
}

static const SsrnxXmlAttribute *apk_manifest_node_attr(const SsrnxXmlNode *node,const char *name) {
    for(size_t i=0;i<node->attribute_count;++i) {
        const SsrnxXmlAttribute *a=&node->attributes[i];
        if(strcmp(a->name,name)==0&&a->namespace_uri&&
           strcmp(a->namespace_uri,"http://schemas.android.com/apk/res/android")==0)return a;
    }
    return NULL;
}
static bool apk_manifest_class_equal(const char *pkg,const char *declared,const char *requested) {
    if(!declared||!requested)return false;
    char full[SSRNX_APK_PACKAGE_CAP+SSRNX_APK_ACTIVITY_CAP+2];
    int n=snprintf(full,sizeof(full),declared[0]=='.'?"%s%s":!strchr(declared,'.')?"%s.%s":"%s%s",
                    declared[0]=='.'||!strchr(declared,'.')?pkg:"",declared);
    if(n<0||(size_t)n>=sizeof(full))return false;
    for(char *p=full;*p;++p)if(*p=='/')*p='.';
    char other[sizeof(full)];
    n=snprintf(other,sizeof(other),requested[0]=='.'?"%s%s":!strchr(requested,'.')&&!strchr(requested,'/')?"%s.%s":"%s%s",
                 requested[0]=='.'||(!strchr(requested,'.')&&!strchr(requested,'/'))?pkg:"",requested);
    if(n<0||(size_t)n>=sizeof(other))return false;
    for(char *p=other;*p;++p)if(*p=='/')*p='.';
    return strcmp(full,other)==0;
}
SsrnxResult ssrnx_apk_activity_theme(const char *path,const char *pkg,const char *name,uint32_t *out) {
    if(!path||!pkg||!name||!out)return SSRNX_ERR_ARGUMENT;
    *out=0;const uint8_t *xml=NULL;size_t size=0;bool owned=false;
    SsrnxResult rc=apk_manifest_xml_cached(path,&xml,&size,&owned);
    if(rc!=SSRNX_OK)return rc;
    SsrnxXmlDocument doc={0};rc=ssrnx_apk_parse_binary_xml(xml,size,&doc);
    if(owned)free((void*)xml);
    if(rc!=SSRNX_OK)return rc;
    const char *target=name;
    /* PackageParser aliases inherit the already declared target Activity theme. */
    for(unsigned pass=0;pass<2;++pass) {
        rc=SSRNX_ERR_SYMBOL_NOT_FOUND;bool alias=false;
        for(size_t i=0;i<doc.count;++i) {
            SsrnxXmlNode *node=&doc.nodes[i];
            if(node->parent<0||strcmp(doc.nodes[node->parent].name,"application")!=0)continue;
            if(strcmp(node->name,"activity")!=0&&(pass||strcmp(node->name,"activity-alias")!=0))continue;
            const SsrnxXmlAttribute *a=apk_manifest_node_attr(node,"name");
            if(!a||!apk_manifest_class_equal(pkg,a->text,target))continue;
            if(strcmp(node->name,"activity-alias")==0) {
                a=apk_manifest_node_attr(node,"targetActivity");target=a?a->text:NULL;alias=target!=NULL;
                break;
            }
            a=apk_manifest_node_attr(node,"theme");
            if(a&&a->type==RES_VALUE_TYPE_REFERENCE)*out=a->data;
            rc=SSRNX_OK;break;
        }
        if(!alias)break;
    }
    ssrnx_apk_free_xml(&doc);return rc;
}

SsrnxResult ssrnx_apk_requested_permissions(const char *apk_path, uint32_t platform_api,
                                            char ***out_names, size_t *out_count) {
    if (!apk_path || !out_names || !out_count) return SSRNX_ERR_ARGUMENT;
    *out_names = NULL; *out_count = 0;
    const uint8_t *xml = NULL;
    size_t xml_size = 0, count = 0;
    bool owned = false;
    char **names = NULL;
    SsrnxResult rc = apk_manifest_xml_cached(apk_path, &xml, &xml_size, &owned);
    if (rc != SSRNX_OK) return rc;
    rc = SSRNX_ERR_MANIFEST_INVALID;
    if (xml_size < 8 || apk_rd16(xml) != RES_XML_TYPE) goto done;
    const uint16_t root_header = apk_rd16(xml + 2);
    const uint32_t root_size = apk_rd32(xml + 4);
    if (root_header < 8 || root_size > xml_size || root_size < root_header) goto done;
    apk_AxmlStringPool pool = {0};
    bool have_pool = false;
    int depth = 0;
    for (size_t off = root_header; off + 8 <= root_size;) {
        const uint8_t *chunk = xml + off;
        const uint16_t type = apk_rd16(chunk), header_size = apk_rd16(chunk + 2);
        const uint32_t chunk_size = apk_rd32(chunk + 4);
        if (header_size < 8 || chunk_size < header_size || chunk_size > root_size - off) goto done;
        if (type == RES_STRING_POOL_TYPE) {
            if (!apk_axml_pool_init(&pool, chunk, root_size - off)) goto done;
            have_pool = true;
        } else if (type == RES_XML_START_ELEMENT_TYPE) {
            if (!have_pool || chunk_size < 36) goto done;
            const uint8_t *ext = chunk + 16;
            const uint16_t attr_start = apk_rd16(ext + 8), attr_size = apk_rd16(ext + 10), attr_count = apk_rd16(ext + 12);
            const size_t attrs_off = 16u + attr_start;
            if (attr_size < 20 || attrs_off > chunk_size ||
                (uint64_t)attrs_off + (uint64_t)attr_size * attr_count > chunk_size) goto done;
            char tag[128];
            if (!apk_axml_pool_get(&pool, apk_rd32(ext + 4), tag, sizeof(tag))) goto done;
            ++depth;
            const bool sdk23 = strcmp(tag, "uses-permission-sdk-23") == 0 || strcmp(tag, "uses-permission-sdk-m") == 0;
            if (depth == 2 && (strcmp(tag, "uses-permission") == 0 || sdk23) && (!sdk23 || platform_api >= 23)) {
                const uint8_t *attrs = chunk + attrs_off;
                uint32_t max_sdk = UINT32_MAX;
                (void)apk_axml_attr_u32(&pool, attrs, attr_size, attr_count, "maxSdkVersion", &max_sdk);
                char name[256];
                if (max_sdk >= platform_api && apk_axml_copy_attr_string(&pool, attrs, attr_size, attr_count, "name", name, sizeof(name)) && name[0]) {
                    bool duplicate = false;
                    for (size_t i = 0; i < count; ++i) if (strcmp(names[i], name) == 0) { duplicate = true; break; }
                    if (!duplicate) {
                        char **grown = realloc(names, (count + 1) * sizeof(*names));
                        if (!grown) { rc = SSRNX_ERR_NO_MEMORY; goto done; }
                        names = grown;
                        names[count] = strdup(name);
                        if (!names[count]) { rc = SSRNX_ERR_NO_MEMORY; goto done; }
                        ++count;
                    }
                }
            }
        } else if (type == RES_XML_END_ELEMENT_TYPE) {
            if (depth <= 0) goto done;
            --depth;
        }
        off += chunk_size;
    }
    if (depth != 0) goto done;
    rc = SSRNX_OK;
done:
    if (owned) free((void *)xml);
    if (rc == SSRNX_OK) { *out_names = names; *out_count = count; }
    else { for (size_t i = 0; i < count; ++i) free(names[i]); free(names); }
    return rc;
}

SsrnxResult ssrnx_zip_read_entry_alloc(const char *zip_path,
                                       const char *entry_name,
                                       uint8_t **out_data,
                                       size_t *out_size) {
    if (!zip_path || !entry_name || !out_data || !out_size) return SSRNX_ERR_ARGUMENT;
    *out_data = NULL;
    *out_size = 0;
    ZipDirectory zip;
    SsrnxResult rc = apk_zip_open(zip_path, &zip);
    if (rc != SSRNX_OK) return rc;
    apk_ZipEntryInfo entry;
    rc = apk_zip_find_entry(&zip, entry_name, &entry);
    if (rc == SSRNX_OK) rc = apk_zip_read_entry_data(&zip, &entry, out_data, out_size);
    apk_zip_close(&zip);
    return rc;
}

SsrnxResult ssrnx_zip_archive_open(const char *path, SsrnxZipArchive **out) {
    if (!path || !out) return SSRNX_ERR_ARGUMENT;
    *out = NULL;
    ZipDirectory *zip = calloc(1, sizeof(*zip));
    if (!zip) return SSRNX_ERR_NO_MEMORY;
    SsrnxResult rc = apk_zip_open(path, zip);
    /* Validate the directory now, rather than accepting a malformed archive
     * until its first getEntry(). */
    for (uint16_t i = 0; rc == SSRNX_OK && i < zip->entry_count; ++i) {
        apk_ZipEntryInfo entry;
        rc = apk_zip_read_central_entry(zip, &entry);
    }
    if (rc != SSRNX_OK) { apk_zip_close(zip); free(zip); return rc; }
    *out = zip;
    return SSRNX_OK;
}

void ssrnx_zip_archive_close(SsrnxZipArchive *zip) {
    if (zip) { apk_zip_close(zip); free(zip); }
}

SsrnxResult ssrnx_zip_archive_entry(SsrnxZipArchive *zip, const char *name, SsrnxZipEntryInfo *out) {
    if (!out) return SSRNX_ERR_ARGUMENT;
    apk_ZipEntryInfo entry;
    SsrnxResult rc = apk_zip_find_entry(zip, name, &entry);
    if (rc != SSRNX_OK) return rc;
    apk_zip_entry_public(&entry, out);
    return SSRNX_OK;
}

SsrnxResult ssrnx_zip_archive_count(SsrnxZipArchive *zip, size_t *out) {
    if (!zip || !zip->file || !out) return SSRNX_ERR_ARGUMENT;
    *out = zip->entry_count;
    return SSRNX_OK;
}

SsrnxResult ssrnx_zip_archive_next(SsrnxZipArchive *zip, size_t *cursor, SsrnxZipEntryInfo *out) {
    if (!zip || !zip->file || !cursor || !out) return SSRNX_ERR_ARGUMENT;
    ZipDirectory walk = *zip;
    walk.central_cursor = *cursor;
    apk_ZipEntryInfo entry;
    SsrnxResult rc = apk_zip_read_central_entry(&walk, &entry);
    if (rc != SSRNX_OK) return rc;
    apk_zip_entry_public(&entry, out);
    *cursor = walk.central_cursor;
    return SSRNX_OK;
}

SsrnxResult ssrnx_zip_archive_read(SsrnxZipArchive *zip, const char *name, uint8_t **out, size_t *size) {
    if (!out || !size) return SSRNX_ERR_ARGUMENT;
    *out = NULL; *size = 0;
    apk_ZipEntryInfo entry;
    SsrnxResult rc = apk_zip_find_entry(zip, name, &entry);
    return rc == SSRNX_OK ? apk_zip_read_entry_data(zip, &entry, out, size) : rc;
}

SsrnxResult ssrnx_zip_extract_entry(const char *zip_path,
                                    const char *entry_name,
                                    const char *output_path) {
    if (!zip_path || !entry_name || !output_path) return SSRNX_ERR_ARGUMENT;
    ZipDirectory zip;
    SsrnxResult rc = apk_zip_open(zip_path, &zip);
    if (rc != SSRNX_OK) return rc;

    apk_ZipEntryInfo entry;
    rc = apk_zip_find_entry(&zip, entry_name, &entry);
    if (rc != SSRNX_OK) {
        apk_zip_close(&zip);
        return rc;
    }

    /* Stored nested APKs are common in XAPK bundles. Do not materialize the
     * entire entry twice in RAM just to copy it to a cache file. Copy in a
     * bounded 1 MiB window and validate CRC32 incrementally. */
    if (entry.method == ZIP_METHOD_STORE) {
        rc = apk_zip_extract_store_stream(&zip, &entry, output_path);
        apk_zip_close(&zip);
        return rc;
    }

    /* Raw DEFLATE still uses the existing whole-entry inflater because that
     * decoder currently requires contiguous input/output buffers. This keeps
     * behavior unchanged for compressed APK assets while the hot XAPK STORE
     * path becomes bounded-memory. */
    uint8_t *data = NULL;
    size_t size = 0;
    rc = apk_zip_read_entry_data(&zip, &entry, &data, &size);
    apk_zip_close(&zip);
    if (rc != SSRNX_OK) return rc;

    FILE *out = fopen(output_path, "wb");
    if (!out) {
        free(data);
        return SSRNX_ERR_IO;
    }
    const bool ok = size == 0 || fwrite(data, 1, size, out) == size;
    const bool close_ok = fclose(out) == 0;
    free(data);
    if (!ok || !close_ok) {
        (void)unlink(output_path);
        return SSRNX_ERR_IO;
    }
    return SSRNX_OK;
}

SsrnxResult ssrnx_apk_extract_entry(const char *apk_path,
                                    const char *entry_name,
                                    const char *output_path) {
    return ssrnx_zip_extract_entry(apk_path, entry_name, output_path);
}

SsrnxResult ssrnx_apk_extract_arm64(const char *apk_path,
                                    const char *destination_dir,
                                    size_t *out_count) {
    if (!apk_path || !destination_dir) return SSRNX_ERR_ARGUMENT;
    if (out_count) *out_count = 0;

    if (mkdir(destination_dir, 0777) != 0 && errno != EEXIST) return SSRNX_ERR_IO;

    ZipDirectory zip;
    SsrnxResult rc = apk_zip_open(apk_path, &zip);
    if (rc != SSRNX_OK) return rc;
    if (apk_zip_rewind_central(&zip) != 0) {
        apk_zip_close(&zip);
        return SSRNX_ERR_IO;
    }

    size_t extracted = 0;
    for (uint16_t i = 0; i < zip.entry_count; ++i) {
        apk_ZipEntryInfo entry;
        rc = apk_zip_read_central_entry(&zip, &entry);
        if (rc != SSRNX_OK) break;

        if (apk_starts_with(entry.name, "lib/arm64-v8a/") && apk_ends_with(entry.name, ".so")) {
            const char *base = entry.name + strlen("lib/arm64-v8a/");
            if (!base[0] || strchr(base, '/') || strchr(base, '\\') || strstr(base, "..")) {
                rc = SSRNX_ERR_APK_INVALID;
                break;
            }
            char output[SSRNX_APK_PATH_CAP];
            const int n = snprintf(output, sizeof(output), "%s/%s", destination_dir, base);
            if (n <= 0 || (size_t)n >= sizeof(output)) {
                rc = SSRNX_ERR_ARGUMENT;
                break;
            }

            uint8_t *data = NULL;
            size_t data_size = 0;
            rc = apk_zip_read_entry_data(&zip, &entry, &data, &data_size);
            if (rc != SSRNX_OK) break;
            FILE *out = fopen(output, "wb");
            if (!out) {
                free(data);
                rc = SSRNX_ERR_IO;
                break;
            }
            const bool wrote = data_size == 0 || fwrite(data, 1, data_size, out) == data_size;
            const bool closed = fclose(out) == 0;
            free(data);
            if (!wrote || !closed) {
                rc = SSRNX_ERR_IO;
                break;
            }
            ++extracted;
        }
    }

    apk_zip_close(&zip);
    if (rc == SSRNX_OK && extracted == 0) rc = SSRNX_ERR_ABI_NOT_FOUND;
    if (out_count) *out_count = extracted;
    return rc;
}

#pragma pop_macro("ZIP_STREAM_CHUNK")
#pragma pop_macro("ZIP_METHOD_STORE")
#pragma pop_macro("ZIP_METHOD_DEFLATE")
#pragma pop_macro("ZIP_MAX_COMMENT")
#pragma pop_macro("ZIP_LOCAL_SIG")
#pragma pop_macro("ZIP_EOCD_SIG")
#pragma pop_macro("ZIP_EOCD_MIN_SIZE")
#pragma pop_macro("ZIP_CENTRAL_SIG")
#pragma pop_macro("RES_XML_TYPE")
#pragma pop_macro("RES_XML_START_ELEMENT_TYPE")
#pragma pop_macro("RES_XML_END_ELEMENT_TYPE")
#pragma pop_macro("RES_VALUE_TYPE_STRING")
#pragma pop_macro("RES_VALUE_TYPE_REFERENCE")
#pragma pop_macro("RES_VALUE_TYPE_INT_HEX")
#pragma pop_macro("RES_VALUE_TYPE_INT_DEC")
#pragma pop_macro("RES_VALUE_TYPE_INT_COLOR_RGB8")
#pragma pop_macro("RES_VALUE_TYPE_INT_COLOR_RGB4")
#pragma pop_macro("RES_VALUE_TYPE_INT_COLOR_ARGB8")
#pragma pop_macro("RES_VALUE_TYPE_INT_COLOR_ARGB4")
#pragma pop_macro("RES_VALUE_TYPE_INT_BOOLEAN")
#pragma pop_macro("RES_VALUE_TYPE_FLOAT")
#pragma pop_macro("RES_VALUE_TYPE_DYNAMIC_REFERENCE")
#pragma pop_macro("RES_TABLE_TYPE_TYPE")
#pragma pop_macro("RES_TABLE_TYPE_FLAG_SPARSE")
#pragma pop_macro("RES_TABLE_TYPE_FLAG_OFFSET16")
#pragma pop_macro("RES_TABLE_TYPE")
#pragma pop_macro("RES_TABLE_PACKAGE_TYPE")
#pragma pop_macro("RES_TABLE_ENTRY_FLAG_COMPLEX")
#pragma pop_macro("RES_STRING_POOL_UTF8_FLAG")
#pragma pop_macro("RES_STRING_POOL_TYPE")
#pragma pop_macro("RES_NO_ENTRY")
#pragma pop_macro("APK_SIG_BLOCK_V3_ID")
#pragma pop_macro("APK_SIG_BLOCK_V2_ID")
#pragma pop_macro("APK_SIG_BLOCK_MAX")
#pragma pop_macro("APK_MANIFEST_CACHE_SLOTS")
#pragma pop_macro("APK_ARSC_CACHE_SLOTS")


/* ---- package ---- */
#pragma push_macro("SSRNX_DEX_ACTIVE_SLOTS")
#pragma push_macro("SSRNX_DEX_INDEX_MAGIC")
#pragma push_macro("SSRNX_DEX_INDEX_PARKED")
#pragma push_macro("SSRNX_DEX_INDEX_VERSION")
#pragma push_macro("SSRNX_DEX_LOADING_SLOTS")
#pragma push_macro("SSRNX_DEX_RESIDENT_BUDGET")
#pragma push_macro("SSRNX_ICON_CACHE_MAGIC")
#pragma push_macro("SSRNX_ICON_CACHE_MAX_DIM")
#pragma push_macro("SSRNX_ICON_CACHE_VERSION")
#pragma push_macro("SSRNX_LAUNCHER_CACHE_MAGIC")
#pragma push_macro("SSRNX_LAUNCHER_CACHE_VERSION")
#pragma push_macro("SSRNX_PACKAGE_CACHE_MAGIC")
#pragma push_macro("SSRNX_PACKAGE_CACHE_ROOT")
#pragma push_macro("SSRNX_PACKAGE_CACHE_VERSION")
#pragma push_macro("SSRNX_PACKAGE_MAX_APKS")
#pragma push_macro("SSRNX_PACKAGE_MAX_ARM64_NAMES")
#pragma push_macro("SSRNX_PACKAGE_MAX_OBBS")
#pragma push_macro("SSRNX_PLAN_CACHE_PARKED")
#pragma push_macro("SSRNX_SECONDARY_DEX_MAX")
#pragma push_macro("SSRNX_XAPK_CACHE_ROOT")
#pragma push_macro("APK_INFO_FIELD")
#pragma push_macro("DEX_CLASS_MEMO_BUCKETS")
#pragma push_macro("DEX_CLASS_MEMO_MAX")
#pragma push_macro("DEX_CLASS_MEMO_TEXT_CAP")
#pragma push_macro("DEX_INDEX_CANDIDATES")
#pragma push_macro("DEX_METHOD_MEMO_BUCKETS")
#pragma push_macro("DEX_METHOD_MEMO_MAX")
#pragma push_macro("DEX_VALUE_ANNOTATION")
#pragma push_macro("DEX_VALUE_ARRAY")
#pragma push_macro("DEX_VALUE_BOOLEAN")
#pragma push_macro("DEX_VALUE_BYTE")
#pragma push_macro("DEX_VALUE_CHAR")
#pragma push_macro("DEX_VALUE_DOUBLE")
#pragma push_macro("DEX_VALUE_ENUM")
#pragma push_macro("DEX_VALUE_FIELD")
#pragma push_macro("DEX_VALUE_FLOAT")
#pragma push_macro("DEX_VALUE_INT")
#pragma push_macro("DEX_VALUE_LONG")
#pragma push_macro("DEX_VALUE_METHOD")
#pragma push_macro("DEX_VALUE_METHOD_HANDLE")
#pragma push_macro("DEX_VALUE_METHOD_TYPE")
#pragma push_macro("DEX_VALUE_NULL")
#pragma push_macro("DEX_VALUE_SHORT")
#pragma push_macro("DEX_VALUE_STRING")
#pragma push_macro("DEX_VALUE_TYPE")





#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define SSRNX_PACKAGE_MAX_APKS 48u
#define SSRNX_PACKAGE_MAX_OBBS 32u

#ifdef __SWITCH__
#define SSRNX_XAPK_CACHE_ROOT "sdmc:/switch/stonestory_nx/cache/xapk-v2"
#define SSRNX_PACKAGE_CACHE_ROOT "sdmc:/switch/stonestory_nx/cache/package-v1"
#else
#define SSRNX_XAPK_CACHE_ROOT "/tmp/apn64-xapk-cache"
#define SSRNX_PACKAGE_CACHE_ROOT "/tmp/apn64-package-cache-v1"
#endif

#define SSRNX_PACKAGE_CACHE_MAGIC UINT64_C(0x41504b3634504b47) /* SSRNXPKG */
#define SSRNX_PACKAGE_CACHE_VERSION 3u
#define SSRNX_ICON_CACHE_MAGIC UINT64_C(0x41504b363449434e) /* SSRNXICN */
#define SSRNX_ICON_CACHE_VERSION 4u
#define SSRNX_LAUNCHER_CACHE_MAGIC UINT64_C(0x41504b364c4e4348) /* SSRNXLNCH */
#define SSRNX_LAUNCHER_CACHE_VERSION 3u
#define SSRNX_DEX_INDEX_MAGIC UINT64_C(0x41504b3644455849) /* SSRNXDEXI */
#define SSRNX_DEX_INDEX_VERSION 1u
/* Must match the apk.c icon downsample bound. */
#define SSRNX_ICON_CACHE_MAX_DIM 256u

/* The package and launcher caches store raw SsrnxApkInfo images. sizeof()
 * alone misses field moves absorbed by tail padding, so the headers also
 * carry a fingerprint of every field offset. */
#define APK_INFO_FIELD(f) (uint32_t)offsetof(SsrnxApkInfo, f)
static uint32_t package_apk_info_layout_key(void) {
    const uint32_t offs[] = {
        (uint32_t)sizeof(SsrnxApkInfo),
        APK_INFO_FIELD(apk_path), APK_INFO_FIELD(package_name), APK_INFO_FIELD(version_name),
        APK_INFO_FIELD(application_label), APK_INFO_FIELD(version_code), APK_INFO_FIELD(min_sdk),
        APK_INFO_FIELD(target_sdk), APK_INFO_FIELD(application_icon_resource),
        APK_INFO_FIELD(application_round_icon_resource), APK_INFO_FIELD(application_label_resource),
        APK_INFO_FIELD(screen_orientation), APK_INFO_FIELD(screen_orientation_declared),
        APK_INFO_FIELD(activity_name), APK_INFO_FIELD(native_lib_name),
        APK_INFO_FIELD(native_entry_path), APK_INFO_FIELD(split_name),
        APK_INFO_FIELD(zip_entry_count), APK_INFO_FIELD(native_lib_count),
        APK_INFO_FIELD(arm64_lib_count), APK_INFO_FIELD(native_activity),
        APK_INFO_FIELD(native_library_present), APK_INFO_FIELD(manifest_binary_xml),
        APK_INFO_FIELD(split_apk), APK_INFO_FIELD(engine_kind), APK_INFO_FIELD(unity_backend),
        APK_INFO_FIELD(unity_player_present), APK_INFO_FIELD(unity_main_present),
        APK_INFO_FIELD(unity_il2cpp_present), APK_INFO_FIELD(unity_mono_present),
        APK_INFO_FIELD(sdl_present), APK_INFO_FIELD(godot_present),
        APK_INFO_FIELD(gamemaker_present), APK_INFO_FIELD(game_activity),
        APK_INFO_FIELD(java_activity_bootstrap), APK_INFO_FIELD(java_dex_count),
        APK_INFO_FIELD(java_bootstrap_lib_count), APK_INFO_FIELD(java_bootstrap_libs),
        0x504b4431u, /* java_bootstrap_libs encoding: packed NUL-separated names */
        APK_INFO_FIELD(source_xapk), APK_INFO_FIELD(package_apk_count),
        APK_INFO_FIELD(package_selected_apk_count), APK_INFO_FIELD(package_obb_count),
        APK_INFO_FIELD(base_apk_path),
    };
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < sizeof(offs) / sizeof(offs[0]); ++i) {
        h ^= offs[i];
        h *= 16777619u;
    }
    return h;
}
#undef APK_INFO_FIELD


typedef struct package_PackageOuterIndex {
    char apk_entries[SSRNX_PACKAGE_MAX_APKS][SSRNX_APK_ENTRY_CAP];
    uint32_t apk_crc32[SSRNX_PACKAGE_MAX_APKS];
    uint32_t apk_compressed_size[SSRNX_PACKAGE_MAX_APKS];
    uint32_t apk_uncompressed_size[SSRNX_PACKAGE_MAX_APKS];
    uint16_t apk_method[SSRNX_PACKAGE_MAX_APKS];
    size_t apk_count;
    size_t apk_seen;
    char obb_entries[SSRNX_PACKAGE_MAX_OBBS][SSRNX_APK_ENTRY_CAP];
    size_t obb_count;
    size_t obb_seen;
    bool overflow;
} package_PackageOuterIndex;

typedef struct package_PackageComponent {
    char outer_entry[SSRNX_APK_ENTRY_CAP];
    char apk_path[SSRNX_APK_PATH_CAP];
    SsrnxApkInfo info;
    bool selected;
    bool base;
} package_PackageComponent;

typedef struct package_PackagePlan {
    char source_path[SSRNX_APK_PATH_CAP];
    bool xapk;
    package_PackageOuterIndex outer;
    package_PackageComponent components[SSRNX_PACKAGE_MAX_APKS];
    size_t component_count;
    size_t selected_count;
    size_t base_index;
    char cache_dir[SSRNX_APK_PATH_CAP];
} package_PackagePlan;

typedef struct package_PackageDiskHeader {
    uint64_t magic;
    uint32_t version;
    uint32_t plan_size;
    uint32_t info_size;
    int32_t probe_result;
    uint64_t source_key;
    uint32_t info_layout;
    uint32_t reserved;
} package_PackageDiskHeader;

typedef struct package_PackageIconDiskHeader {
    uint64_t magic;
    uint32_t version;
    int32_t result;
    uint64_t source_key;
    int32_t width;
    int32_t height;
    uint32_t pixel_count;
    char entry_name[SSRNX_APK_ENTRY_CAP];
} package_PackageIconDiskHeader;

typedef struct package_PackageLauncherDiskHeader {
    uint64_t magic;
    uint32_t version;
    uint32_t info_size;
    uint64_t source_key;
    uint32_t info_layout;
    uint32_t reserved;
} package_PackageLauncherDiskHeader;

typedef struct package_DexIndexDiskHeader {
    uint64_t magic;
    uint32_t version;
    uint32_t entry_size;
    uint64_t source_key;
    uint32_t entry_count;
    uint32_t reserved;
} package_DexIndexDiskHeader;

typedef struct package_DexIndexEntry {
    uint64_t class_hash;
    uint64_t dex_name_hash;
    uint32_t component_index;
    uint32_t reserved;
} package_DexIndexEntry;

typedef struct package_DexIndexState {
    bool valid;
    uint64_t source_key;
    char source_path[SSRNX_APK_PATH_CAP];
    package_DexIndexEntry *entries;
    size_t count;
    size_t capacity;
    /* entries[] positions ordered by (class_hash, position): binary search
     * replaces a linear scan per class lookup while preserving the
     * base-before-split order of hash collisions. */
    uint32_t *order;
} package_DexIndexState;

static package_DexIndexState package_g_dex_index;
/* Package/DEX metadata is shared by the Activity thread and guest Java worker
 * threads. Android permits those threads to resolve classes concurrently, so
 * the cache must never expose a half-mutated index or free a DEX blob while a
 * second resolver is parsing it. */
static pthread_mutex_t package_g_dex_index_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t package_g_dex_blob_lock = PTHREAD_MUTEX_INITIALIZER;
/* DEX images are read from the card without g_dex_blob_lock held (a load
 * took the lock for the whole multi-megabyte read, stalling every thread
 * resolving a class). Paths being loaded are listed here; a second request
 * for one waits on g_dex_blob_loaded instead of reading it again. */
#define SSRNX_DEX_LOADING_SLOTS 8u
static pthread_cond_t package_g_dex_blob_loaded = PTHREAD_COND_INITIALIZER;
static char package_g_dex_loading[SSRNX_DEX_LOADING_SLOTS][SSRNX_APK_PATH_CAP];
static bool package_g_dex_loading_used[SSRNX_DEX_LOADING_SLOTS];

static bool package_copy_text(char *dst, size_t cap, const char *src);
static SsrnxResult package_read_file_alloc(const char *path, uint8_t **out_data, size_t *out_size);
static void package_dex_index_reset(void);
static void package_dex_index_reset_all(void);

/* DEX resident set. ART keeps every DEX file of the boot/app class path
 * mapped for the life of the process, so a class-path image must never be
 * re-read from storage just because another DEX file was touched. Hardware
 * logs of a 7-DEX multidex package showed the previous 8-slot / 4-warm policy
 * degrade into a per-invoke 7 MiB SD read (22 GB for one startup, ~100 DEX
 * instructions per second). Images stay refcounted while in use; released
 * images stay warm until the resident byte budget is exceeded, at which point
 * the least recently used warm image is dropped. The slot table is sized well
 * above any real multidex count; the owned-allocation fallback only covers
 * pathological recursion that pins every slot. */
#define SSRNX_DEX_ACTIVE_SLOTS 64u
#define SSRNX_DEX_RESIDENT_BUDGET (256ull * 1024ull * 1024ull)
typedef struct package_DexActiveBlob {
    bool used;
    char path[SSRNX_APK_PATH_CAP];
    uint8_t *data;
    size_t size;
    size_t refs;
    uint64_t last_use;
    /* Lazily built descriptor -> class_def map (index + 1, 0 = empty). The
     * blob is immutable while resident, so the table is built once. */
    uint32_t *class_table;
    uint32_t class_table_mask;
    uint64_t load_id; /* distinguishes successive images in one slot */
} package_DexActiveBlob;
static package_DexActiveBlob package_g_dex_active[SSRNX_DEX_ACTIVE_SLOTS];
/* SSRNX core library (boot class path): an immutable DEX linked into the
 * runtime (ssrnx_core_library_dex, generated by tools/libcore/build.sh). It
 * lives outside g_dex_active, so releasing a reference to it is a no-op. */
static package_DexActiveBlob package_g_aosp_core_blob;
static package_DexActiveBlob package_g_core_blob;
/* Secondary DEX tier (DexClassLoader dexPath elements), see core_library_blob. */
#define SSRNX_SECONDARY_DEX_MAX 32u
static package_DexActiveBlob package_g_secondary_dex[SSRNX_SECONDARY_DEX_MAX];
static size_t package_g_secondary_dex_count;
static uint64_t package_g_dex_blob_hits;
static uint64_t package_g_dex_blob_loads;
static uint64_t package_g_dex_blob_bytes;
static uint64_t package_g_dex_blob_evictions;
static uint64_t package_g_dex_blob_unshared;
static uint64_t package_g_dex_use_clock;
static uint64_t package_g_dex_resident_bytes;

static size_t package_dex_blob_warm_count(void) {
    size_t count = 0;
    for (size_t i = 0; i < SSRNX_DEX_ACTIVE_SLOTS; ++i)
        if (package_g_dex_active[i].used && package_g_dex_active[i].refs == 0) count++;
    return count;
}

static size_t package_dex_blob_oldest_warm(void) {
    size_t found = SSRNX_DEX_ACTIVE_SLOTS;
    uint64_t oldest = UINT64_MAX;
    for (size_t i = 0; i < SSRNX_DEX_ACTIVE_SLOTS; ++i) {
        package_DexActiveBlob *slot = &package_g_dex_active[i];
        if (!slot->used || slot->refs != 0) continue;
        if (found == SSRNX_DEX_ACTIVE_SLOTS || slot->last_use < oldest) {
            found = i;
            oldest = slot->last_use;
        }
    }
    return found;
}

static void package_dex_blob_clear_slot(size_t index) {
    if (index >= SSRNX_DEX_ACTIVE_SLOTS) return;
    package_DexActiveBlob *slot = &package_g_dex_active[index];
    if (slot->used) package_g_dex_resident_bytes -= slot->size;
    free(slot->data);
    free(slot->class_table);
    memset(slot, 0, sizeof(*slot));
}

/* Drops least recently used warm images until `incoming` more bytes fit in
 * the resident budget. In-use images are never dropped. */
static void package_dex_blob_trim_warm_locked(uint64_t incoming) {
    while (package_g_dex_resident_bytes + incoming > SSRNX_DEX_RESIDENT_BUDGET) {
        const size_t oldest = package_dex_blob_oldest_warm();
        if (oldest == SSRNX_DEX_ACTIVE_SLOTS) break;
        package_dex_blob_clear_slot(oldest);
        package_g_dex_blob_evictions++;
    }
}

/* Hot-path acquire: succeeds only when the DEX image is already resident, so
 * repeated class/method resolution performs no file system access. */
static bool package_dex_blob_acquire_resident(const char *path, uint8_t **out_data,
                                      size_t *out_size, void **out_owner) {
    bool found = false;
    SSRNX_PERF_LOCK(&package_g_dex_blob_lock);
    for (size_t i = 0; i < SSRNX_DEX_ACTIVE_SLOTS; ++i) {
        package_DexActiveBlob *slot = &package_g_dex_active[i];
        if (!slot->used || strcmp(slot->path, path) != 0) continue;
        slot->refs++;
        slot->last_use = ++package_g_dex_use_clock;
        *out_data = slot->data; *out_size = slot->size; *out_owner = slot;
        package_g_dex_blob_hits++;
        found = true;
        break;
    }
    pthread_mutex_unlock(&package_g_dex_blob_lock);
    return found;
}

static bool package_dex_blob_take_resident_locked(const char *path, uint8_t **out_data,
                                          size_t *out_size, void **out_owner) {
    for (size_t i = 0; i < SSRNX_DEX_ACTIVE_SLOTS; ++i) {
        package_DexActiveBlob *slot = &package_g_dex_active[i];
        if (!slot->used || strcmp(slot->path, path) != 0) continue;
        slot->refs++;
        slot->last_use = ++package_g_dex_use_clock;
        *out_data = slot->data; *out_size = slot->size; *out_owner = slot;
        package_g_dex_blob_hits++;
        return true;
    }
    return false;
}

static size_t package_dex_blob_loading_find_locked(const char *path) {
    for (size_t i = 0; i < SSRNX_DEX_LOADING_SLOTS; ++i)
        if (package_g_dex_loading_used[i] && strcmp(package_g_dex_loading[i], path) == 0) return i;
    return SSRNX_DEX_LOADING_SLOTS;
}

/* Installs an image read by the caller (lock held). A copy loaded meanwhile
 * by an untracked concurrent load wins and the caller's data is freed. */
static SsrnxResult package_dex_blob_install_locked(const char *path, uint8_t *data, size_t size,
                                           uint8_t **out_data, size_t *out_size, void **out_owner) {
    if (package_dex_blob_take_resident_locked(path, out_data, out_size, out_owner)) {
        free(data);
        return SSRNX_OK;
    }
    package_g_dex_blob_loads++;
    package_g_dex_blob_bytes += size;

    package_dex_blob_trim_warm_locked(size);
    size_t free_index = SSRNX_DEX_ACTIVE_SLOTS;
    for (size_t i = 0; i < SSRNX_DEX_ACTIVE_SLOTS; ++i) {
        if (!package_g_dex_active[i].used) { free_index = i; break; }
    }
    if (free_index == SSRNX_DEX_ACTIVE_SLOTS) {
        free_index = package_dex_blob_oldest_warm();
        if (free_index != SSRNX_DEX_ACTIVE_SLOTS) {
            package_dex_blob_clear_slot(free_index);
            package_g_dex_blob_evictions++;
        }
    }

    if (free_index == SSRNX_DEX_ACTIVE_SLOTS) {
        /* Pathological recursion can occupy every slot. Preserve correctness
         * with a normal owned allocation rather than failing the launch. */
        if (package_g_dex_blob_unshared++ == 0)
            SSRNX_LOGCW("PERF", "DEX resident set exhausted slots=%u path=%s; serving unshared image",
                        (unsigned)SSRNX_DEX_ACTIVE_SLOTS, path);
        *out_data = data; *out_size = size; *out_owner = NULL;
        return SSRNX_OK;
    }

    package_DexActiveBlob *slot = &package_g_dex_active[free_index];
    memset(slot, 0, sizeof(*slot));
    if (!package_copy_text(slot->path, sizeof(slot->path), path)) {
        free(data);
        return SSRNX_ERR_ARGUMENT;
    }
    slot->used = true; slot->data = data; slot->size = size; slot->refs = 1;
    slot->last_use = ++package_g_dex_use_clock;
    static uint64_t load_seq;
    slot->load_id = ++load_seq;
    package_g_dex_resident_bytes += size;
    *out_data = data; *out_size = size; *out_owner = slot;
    return SSRNX_OK;
}

static SsrnxResult package_dex_blob_acquire(const char *path, uint8_t **out_data,
                                    size_t *out_size, void **out_owner) {
    if (!path || !out_data || !out_size || !out_owner) return SSRNX_ERR_ARGUMENT;
    *out_data = NULL; *out_size = 0; *out_owner = NULL;
    SSRNX_PERF_LOCK(&package_g_dex_blob_lock);
    for (;;) {
        if (package_dex_blob_take_resident_locked(path, out_data, out_size, out_owner)) {
            pthread_mutex_unlock(&package_g_dex_blob_lock);
            return SSRNX_OK;
        }
        if (package_dex_blob_loading_find_locked(path) == SSRNX_DEX_LOADING_SLOTS) break;
        pthread_cond_wait(&package_g_dex_blob_loaded, &package_g_dex_blob_lock);
    }
    size_t loading = SSRNX_DEX_LOADING_SLOTS;
    for (size_t i = 0; i < SSRNX_DEX_LOADING_SLOTS && loading == SSRNX_DEX_LOADING_SLOTS; ++i)
        if (!package_g_dex_loading_used[i] && package_copy_text(package_g_dex_loading[i], sizeof(package_g_dex_loading[i]), path)) loading = i;
    if (loading != SSRNX_DEX_LOADING_SLOTS) package_g_dex_loading_used[loading] = true;
    pthread_mutex_unlock(&package_g_dex_blob_lock);

    uint8_t *data = NULL; size_t size = 0;
    const SsrnxResult read_rc = package_read_file_alloc(path, &data, &size);

    SSRNX_PERF_LOCK(&package_g_dex_blob_lock);
    if (loading != SSRNX_DEX_LOADING_SLOTS) {
        package_g_dex_loading_used[loading] = false;
        pthread_cond_broadcast(&package_g_dex_blob_loaded);
    }
    const SsrnxResult rc = read_rc != SSRNX_OK ? read_rc
                         : package_dex_blob_install_locked(path, data, size, out_data, out_size, out_owner);
    pthread_mutex_unlock(&package_g_dex_blob_lock);
    return rc;
}

/* Takes another reference on a resident image without a path lookup, provided
 * the slot still holds the same load of it. */
static bool package_dex_blob_reacquire(void *owner, uint64_t load_id, uint8_t **out_data, size_t *out_size) {
    package_DexActiveBlob *slot = (package_DexActiveBlob *)owner;
    if (!slot || slot < package_g_dex_active || slot >= package_g_dex_active + SSRNX_DEX_ACTIVE_SLOTS) return false;
    SSRNX_PERF_LOCK(&package_g_dex_blob_lock);
    const bool ok = slot->used && slot->load_id == load_id && slot->data;
    if (ok) {
        slot->refs++;
        slot->last_use = ++package_g_dex_use_clock;
        package_g_dex_blob_hits++;
        *out_data = slot->data;
        *out_size = slot->size;
    }
    pthread_mutex_unlock(&package_g_dex_blob_lock);
    return ok;
}

static uint64_t package_dex_blob_load_id(void *owner) {
    package_DexActiveBlob *slot = (package_DexActiveBlob *)owner;
    if (!slot || slot < package_g_dex_active || slot >= package_g_dex_active + SSRNX_DEX_ACTIVE_SLOTS) return 0;
    SSRNX_PERF_LOCK(&package_g_dex_blob_lock);
    const uint64_t id = slot->used ? slot->load_id : 0;
    pthread_mutex_unlock(&package_g_dex_blob_lock);
    return id;
}

static void package_dex_blob_release(void *owner, uint8_t *data) {
    SSRNX_PERF_LOCK(&package_g_dex_blob_lock);
    if (!owner) { free(data); pthread_mutex_unlock(&package_g_dex_blob_lock); return; }
    package_DexActiveBlob *slot = (package_DexActiveBlob *)owner;
    if (slot < package_g_dex_active || slot >= package_g_dex_active + SSRNX_DEX_ACTIVE_SLOTS || !slot->used || !slot->refs) {
        pthread_mutex_unlock(&package_g_dex_blob_lock);
        return;
    }
    slot->refs--;
    if (slot->refs != 0) { pthread_mutex_unlock(&package_g_dex_blob_lock); return; }
    slot->last_use = ++package_g_dex_use_clock;
    package_dex_blob_trim_warm_locked(0);
    pthread_mutex_unlock(&package_g_dex_blob_lock);
}

void ssrnx_package_get_dex_runtime_cache_stats(uint64_t *out_hits,
                                               uint64_t *out_loads,
                                               uint64_t *out_bytes) {
    SSRNX_PERF_LOCK(&package_g_dex_blob_lock);
    if (out_hits) *out_hits = package_g_dex_blob_hits;
    if (out_loads) *out_loads = package_g_dex_blob_loads;
    if (out_bytes) *out_bytes = package_g_dex_blob_bytes;
    pthread_mutex_unlock(&package_g_dex_blob_lock);
}

void ssrnx_package_get_dex_runtime_cache_detail(uint32_t *out_warm_entries,
                                                uint64_t *out_evictions) {
    SSRNX_PERF_LOCK(&package_g_dex_blob_lock);
    if (out_warm_entries) *out_warm_entries = (uint32_t)package_dex_blob_warm_count();
    if (out_evictions) *out_evictions = package_g_dex_blob_evictions;
    pthread_mutex_unlock(&package_g_dex_blob_lock);
}

void ssrnx_package_reset_dex_runtime_cache(void) {
    SSRNX_PERF_LOCK(&package_g_dex_blob_lock);
    for (size_t i = 0; i < SSRNX_DEX_ACTIVE_SLOTS; ++i) {
        if (package_g_dex_active[i].used && package_g_dex_active[i].refs == 0) package_dex_blob_clear_slot(i);
    }
    package_g_dex_blob_hits = 0;
    package_g_dex_blob_loads = 0;
    package_g_dex_blob_bytes = 0;
    package_g_dex_blob_evictions = 0;
    package_g_dex_blob_unshared = 0;
    package_g_dex_use_clock = 0;
    pthread_mutex_unlock(&package_g_dex_blob_lock);
}

static bool package_copy_text(char *dst, size_t cap, const char *src) {
    if (!dst || cap == 0 || !src) return false;
    size_t n = strlen(src);
    if (n >= cap) {
        dst[0] = '\0';
        return false;
    }
    memcpy(dst, src, n + 1);
    return true;
}

static bool package_ends_with_ci(const char *s, const char *suffix) {
    if (!s || !suffix) return false;
    size_t n = strlen(s), m = strlen(suffix);
    if (n < m) return false;
    s += n - m;
    for (size_t i = 0; i < m; ++i) {
        if (tolower((unsigned char)s[i]) != tolower((unsigned char)suffix[i])) return false;
    }
    return true;
}

bool ssrnx_package_is_xapk_name(const char *name) {
    return package_ends_with_ci(name, ".xapk");
}

bool ssrnx_package_is_source_name(const char *name) {
    return package_ends_with_ci(name, ".apk") || ssrnx_package_is_xapk_name(name);
}

static bool package_path_join(char *dst, size_t cap, const char *a, const char *b) {
    if (!dst || cap == 0 || !a || !b) return false;
    size_t an = strlen(a), bn = strlen(b);
    bool slash = an > 0 && a[an - 1] != '/';
    if (an + (slash ? 1u : 0u) + bn + 1u > cap) {
        dst[0] = '\0';
        return false;
    }
    memcpy(dst, a, an);
    size_t p = an;
    if (slash) dst[p++] = '/';
    memcpy(dst + p, b, bn);
    dst[p + bn] = '\0';
    return true;
}

static bool package_mkdir_one(const char *path) {
    return mkdir(path, 0777) == 0 || errno == EEXIST;
}

static bool package_mkdirs(const char *path) {
    if (!path || !path[0]) return false;
    char tmp[SSRNX_APK_PATH_CAP];
    if (!package_copy_text(tmp, sizeof(tmp), path)) return false;
    size_t start = 1;
    const char *colon = strchr(tmp, ':');
    if (colon && colon[1] == '/') start = (size_t)(colon - tmp) + 2u;
    for (size_t i = start; tmp[i]; ++i) {
        if (tmp[i] != '/') continue;
        char saved = tmp[i];
        tmp[i] = '\0';
        if (tmp[0] && !package_mkdir_one(tmp)) return false;
        tmp[i] = saved;
    }
    return package_mkdir_one(tmp);
}

static uint64_t package_package_monotonic_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * UINT64_C(1000) +
           (uint64_t)ts.tv_nsec / UINT64_C(1000000);
}

static uint64_t package_fnv1a64_bytes(uint64_t h, const void *data, size_t n) {
    const unsigned char *p = (const unsigned char *)data;
    for (size_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= UINT64_C(1099511628211);
    }
    return h;
}

static uint64_t package_source_cache_key(const char *path) {
    uint64_t h = UINT64_C(1469598103934665603);
    if (path) h = package_fnv1a64_bytes(h, path, strlen(path));
    struct stat st;
    if (path && stat(path, &st) == 0) {
        h = package_fnv1a64_bytes(h, &st.st_size, sizeof(st.st_size));
        h = package_fnv1a64_bytes(h, &st.st_mtime, sizeof(st.st_mtime));
    }
    return h;
}

static uint64_t package_source_path_hash(const char *path) {
    uint64_t h = UINT64_C(1469598103934665603);
    if (path) h = package_fnv1a64_bytes(h, path, strlen(path));
    return h;
}

/* The package cache root is created once per process. Every DEX class and
 * method lookup derives a cache path, and on Horizon each mkdir is a
 * file-system IPC round trip; re-creating the directory chain per lookup
 * dominated Java execution time. */
static bool package_g_package_cache_root_ready;

static bool package_ensure_package_cache_root(void) {
    if (__atomic_load_n(&package_g_package_cache_root_ready, __ATOMIC_ACQUIRE)) return true;
#ifdef __SWITCH__
    (void)package_mkdir_one("sdmc:/switch/stonestory_nx");
    (void)package_mkdir_one("sdmc:/switch/stonestory_nx/cache");
#endif
    if (!package_mkdirs(SSRNX_PACKAGE_CACHE_ROOT)) return false;
    __atomic_store_n(&package_g_package_cache_root_ready, true, __ATOMIC_RELEASE);
    return true;
}

static bool package_package_cache_path(const char *source_path, const char *suffix,
                               char *out, size_t out_cap) {
    if (!source_path || !suffix || !out || out_cap == 0) return false;
    if (!package_ensure_package_cache_root()) return false;
    char name[64];
    int n = snprintf(name, sizeof(name), "%016" PRIx64 "%s", package_source_path_hash(source_path), suffix);
    return n > 0 && (size_t)n < sizeof(name) &&
           package_path_join(out, out_cap, SSRNX_PACKAGE_CACHE_ROOT, name);
}

static bool package_read_package_disk_cache(const char *source_path,
                                    package_PackagePlan *out_plan,
                                    SsrnxApkInfo *out_info,
                                    SsrnxResult *out_probe_result) {
    if (!source_path || !out_plan) return false;
    char path[SSRNX_APK_PATH_CAP];
    if (!package_package_cache_path(source_path, ".pkg", path, sizeof(path))) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    package_PackageDiskHeader hdr;
    SsrnxApkInfo info;
    bool ok = fread(&hdr, 1, sizeof(hdr), f) == sizeof(hdr) &&
              hdr.magic == SSRNX_PACKAGE_CACHE_MAGIC &&
              hdr.version == SSRNX_PACKAGE_CACHE_VERSION &&
              hdr.plan_size == sizeof(*out_plan) && hdr.info_size == sizeof(info) &&
              hdr.info_layout == package_apk_info_layout_key() &&
              hdr.source_key == package_source_cache_key(source_path) &&
              fread(out_plan, 1, sizeof(*out_plan), f) == sizeof(*out_plan) &&
              fread(&info, 1, sizeof(info), f) == sizeof(info) &&
              strcmp(out_plan->source_path, source_path) == 0 &&
              strcmp(info.apk_path, source_path) == 0;
    fclose(f);
    if (!ok) return false;
    if (out_info) *out_info = info;
    if (out_probe_result) *out_probe_result = (SsrnxResult)hdr.probe_result;
    return true;
}

static void package_write_package_disk_cache(const char *source_path,
                                     const package_PackagePlan *plan,
                                     const SsrnxApkInfo *info,
                                     SsrnxResult probe_result) {
    if (!source_path || !plan || !info) return;
    char path[SSRNX_APK_PATH_CAP], tmp[SSRNX_APK_PATH_CAP];
    if (!package_package_cache_path(source_path, ".pkg", path, sizeof(path))) return;
    size_t n = strlen(path);
    if (n + 5u > sizeof(tmp)) return;
    memcpy(tmp, path, n); memcpy(tmp + n, ".tmp", 5u);
    FILE *f = fopen(tmp, "wb");
    if (!f) return;
    package_PackageDiskHeader hdr = {
        .magic = SSRNX_PACKAGE_CACHE_MAGIC,
        .version = SSRNX_PACKAGE_CACHE_VERSION,
        .plan_size = (uint32_t)sizeof(*plan),
        .info_size = (uint32_t)sizeof(*info),
        .probe_result = (int32_t)probe_result,
        .source_key = package_source_cache_key(source_path),
        .info_layout = package_apk_info_layout_key(),
    };
    bool ok = fwrite(&hdr, 1, sizeof(hdr), f) == sizeof(hdr) &&
              fwrite(plan, 1, sizeof(*plan), f) == sizeof(*plan) &&
              fwrite(info, 1, sizeof(*info), f) == sizeof(*info) &&
              fflush(f) == 0;
    fclose(f);
    if (!ok) { (void)unlink(tmp); return; }
    (void)unlink(path);
    if (rename(tmp, path) != 0) (void)unlink(tmp);
}







/* Most manifests declare android:label="@string/app_name"; resolve it so the
 * home screen shows the real app name instead of a filename fallback. */






static uint64_t package_package_cache_key(const package_PackagePlan *plan) {
    uint64_t h = package_source_cache_key(plan ? plan->source_path : NULL);
    if (!plan) return h;
    for (size_t i = 0; i < plan->outer.apk_count; ++i) {
        h = package_fnv1a64_bytes(h, plan->outer.apk_entries[i], strlen(plan->outer.apk_entries[i]));
        h = package_fnv1a64_bytes(h, &plan->outer.apk_crc32[i], sizeof(plan->outer.apk_crc32[i]));
        h = package_fnv1a64_bytes(h, &plan->outer.apk_compressed_size[i], sizeof(plan->outer.apk_compressed_size[i]));
        h = package_fnv1a64_bytes(h, &plan->outer.apk_uncompressed_size[i], sizeof(plan->outer.apk_uncompressed_size[i]));
        h = package_fnv1a64_bytes(h, &plan->outer.apk_method[i], sizeof(plan->outer.apk_method[i]));
    }
    return h;
}

static bool package_outer_index_visitor(const SsrnxZipEntryInfo *entry, void *userdata) {
    package_PackageOuterIndex *idx = (package_PackageOuterIndex *)userdata;
    if (!entry || !idx) return false;
    if (package_ends_with_ci(entry->name, ".apk")) {
        idx->apk_seen++;
        if (idx->apk_count < SSRNX_PACKAGE_MAX_APKS) {
            if (!package_copy_text(idx->apk_entries[idx->apk_count], sizeof(idx->apk_entries[0]), entry->name)) {
                idx->overflow = true;
                return false;
            }
            idx->apk_crc32[idx->apk_count] = entry->crc32;
            idx->apk_compressed_size[idx->apk_count] = entry->compressed_size;
            idx->apk_uncompressed_size[idx->apk_count] = entry->uncompressed_size;
            idx->apk_method[idx->apk_count] = entry->method;
            idx->apk_count++;
        } else {
            idx->overflow = true;
            return false;
        }
    } else if (package_ends_with_ci(entry->name, ".obb")) {
        idx->obb_seen++;
        if (idx->obb_count < SSRNX_PACKAGE_MAX_OBBS) {
            if (!package_copy_text(idx->obb_entries[idx->obb_count], sizeof(idx->obb_entries[0]), entry->name)) {
                idx->overflow = true;
                return false;
            }
            idx->obb_count++;
        } else {
            idx->overflow = true;
            return false;
        }
    }
    return true;
}

static bool package_cached_component_valid(const char *path, uint32_t expected_size) {
    struct stat st;
    if (!path || stat(path, &st) != 0 || !S_ISREG(st.st_mode)) return false;
    return (uint64_t)st.st_size == (uint64_t)expected_size;
}

static SsrnxResult package_extract_component_atomic(const char *source_path,
                                            const char *entry_name,
                                            const char *output_path,
                                            uint32_t expected_size,
                                            uint16_t method) {
    char tmp[SSRNX_APK_PATH_CAP];
    size_t n = strlen(output_path);
    if (n + 5u > sizeof(tmp)) return SSRNX_ERR_ARGUMENT;
    memcpy(tmp, output_path, n);
    memcpy(tmp + n, ".tmp", 5u);
    (void)unlink(tmp);
    const uint64_t begin_ms = package_package_monotonic_ms();
    SsrnxResult rc = ssrnx_zip_extract_entry(source_path, entry_name, tmp);
    const uint64_t elapsed_ms = package_package_monotonic_ms() - begin_ms;
    if (rc != SSRNX_OK) {
        SSRNX_LOGCW("XAPK", "component materialize failed entry=%s mode=%s bytes=%u elapsedMs=%llu rc=%s",
                    entry_name, method == 0u ? "stream-store" : "buffered-deflate",
                    expected_size, (unsigned long long)elapsed_ms, ssrnx_result_string(rc));
        (void)unlink(tmp);
        return rc;
    }
    SSRNX_LOGCI("XAPK", "component materialize entry=%s mode=%s bytes=%u elapsedMs=%llu",
                entry_name, method == 0u ? "stream-store" : "buffered-deflate",
                expected_size, (unsigned long long)elapsed_ms);
    if (!package_cached_component_valid(tmp, expected_size)) {
        (void)unlink(tmp);
        return SSRNX_ERR_APK_INVALID;
    }
    (void)unlink(output_path);
    if (rename(tmp, output_path) != 0) {
        (void)unlink(tmp);
        return SSRNX_ERR_IO;
    }
    return SSRNX_OK;
}

static SsrnxResult package_materialize_xapk_components(package_PackagePlan *plan) {
    if (!plan) return SSRNX_ERR_ARGUMENT;
    SsrnxResult rc = ssrnx_zip_visit(plan->source_path, package_outer_index_visitor, &plan->outer);
    if (rc != SSRNX_OK) return rc;
    if (plan->outer.overflow || plan->outer.apk_seen > SSRNX_PACKAGE_MAX_APKS ||
        plan->outer.obb_seen > SSRNX_PACKAGE_MAX_OBBS) {
        return SSRNX_ERR_APK_UNSUPPORTED;
    }
    if (plan->outer.apk_count == 0) return SSRNX_ERR_APK_INVALID;

#ifdef __SWITCH__
    (void)package_mkdir_one("sdmc:/switch/stonestory_nx");
    (void)package_mkdir_one("sdmc:/switch/stonestory_nx/cache/runtime");
#endif
    if (!package_mkdirs(SSRNX_XAPK_CACHE_ROOT)) return SSRNX_ERR_IO;
    char leaf[48];
    snprintf(leaf, sizeof(leaf), "%016" PRIx64, package_package_cache_key(plan));
    if (!package_path_join(plan->cache_dir, sizeof(plan->cache_dir), SSRNX_XAPK_CACHE_ROOT, leaf) ||
        !package_mkdirs(plan->cache_dir)) {
        return SSRNX_ERR_IO;
    }

    size_t reused = 0, extracted = 0;
    for (size_t i = 0; i < plan->outer.apk_count; ++i) {
        package_PackageComponent *c = &plan->components[i];
        memset(c, 0, sizeof(*c));
        if (!package_copy_text(c->outer_entry, sizeof(c->outer_entry), plan->outer.apk_entries[i])) {
            return SSRNX_ERR_APK_UNSUPPORTED;
        }
        char name[40];
        snprintf(name, sizeof(name), "component-%02zu.apk", i);
        if (!package_path_join(c->apk_path, sizeof(c->apk_path), plan->cache_dir, name)) return SSRNX_ERR_ARGUMENT;

        const uint32_t expected_size = plan->outer.apk_uncompressed_size[i];
        bool from_cache = package_cached_component_valid(c->apk_path, expected_size);
        if (!from_cache) {
            rc = package_extract_component_atomic(plan->source_path, c->outer_entry, c->apk_path, expected_size,
                                          plan->outer.apk_method[i]);
            if (rc != SSRNX_OK) return rc;
            extracted++;
        } else {
            reused++;
        }

        rc = ssrnx_apk_inspect(c->apk_path, &c->info);
        if (rc != SSRNX_OK && from_cache) {
            /* A same-sized cache file can still be corrupted. Recreate it once
             * from the immutable XAPK before treating the package as invalid. */
            rc = package_extract_component_atomic(plan->source_path, c->outer_entry, c->apk_path, expected_size,
                                          plan->outer.apk_method[i]);
            if (rc == SSRNX_OK) rc = ssrnx_apk_inspect(c->apk_path, &c->info);
            if (rc == SSRNX_OK) { reused--; extracted++; from_cache = false; }
        }
        if (rc != SSRNX_OK) {
            SSRNX_LOGCW("XAPK", "component inspect failed entry=%s rc=%s", c->outer_entry, ssrnx_result_string(rc));
            return rc;
        }
        if (!c->info.split_apk || c->info.arm64_lib_count > 0 || c->info.native_lib_count > 0) {
            SSRNX_LOGCI("XAPK", "component entry=%s split=%s package=%s arm64=%zu native=%zu cache=%s",
                        c->outer_entry, c->info.split_name[0] ? c->info.split_name : "<base>",
                        c->info.package_name, c->info.arm64_lib_count, c->info.native_lib_count,
                        from_cache ? "hit" : "extract");
        }
        plan->component_count++;
    }
    SSRNX_LOGCI("XAPK", "component cache source=%s hit=%zu extracted=%zu", plan->source_path, reused, extracted);
    return SSRNX_OK;
}

static SsrnxResult package_ensure_component_materialized(package_PackagePlan *plan, size_t index) {
    if (!plan || index >= plan->component_count) return SSRNX_ERR_ARGUMENT;
    if (!plan->xapk) return SSRNX_OK;
    package_PackageComponent *c = &plan->components[index];
    if (package_cached_component_valid(c->apk_path, plan->outer.apk_uncompressed_size[index])) return SSRNX_OK;
    if (!plan->cache_dir[0] || !package_mkdirs(plan->cache_dir)) return SSRNX_ERR_IO;
    SsrnxResult rc = package_extract_component_atomic(plan->source_path, c->outer_entry, c->apk_path,
                                              plan->outer.apk_uncompressed_size[index],
                                              plan->outer.apk_method[index]);
    if (rc == SSRNX_OK) {
        SSRNX_LOGCI("XAPK", "deferred component materialized entry=%s index=%zu", c->outer_entry, index);
    }
    return rc;
}

static SsrnxResult package_select_xapk_package(package_PackagePlan *plan) {
    if (!plan || plan->component_count == 0) return SSRNX_ERR_APK_INVALID;
    size_t base = SIZE_MAX;
    for (size_t i = 0; i < plan->component_count; ++i) {
        if (!plan->components[i].info.split_apk) {
            if (base == SIZE_MAX) {
                base = i;
            } else {
                SSRNX_LOGCW("XAPK", "multiple base APKs are unsupported first=%s other=%s",
                            plan->components[base].outer_entry, plan->components[i].outer_entry);
                return SSRNX_ERR_APK_UNSUPPORTED;
            }
        }
    }
    if (base == SIZE_MAX) return SSRNX_ERR_MANIFEST_INVALID;
    plan->base_index = base;
    package_PackageComponent *base_component = &plan->components[base];
    base_component->base = true;

    for (size_t i = 0; i < plan->component_count; ++i) {
        package_PackageComponent *c = &plan->components[i];
        if (strcmp(c->info.package_name, base_component->info.package_name) != 0) continue;
        /* A config split that carries native libraries but none for arm64-v8a
         * is an ABI split for another architecture. Classify by contents, not
         * by filename/split-name conventions. Resource-only splits stay in the
         * logical package set. The base APK is always retained. */
        if (!c->base && c->info.native_lib_count > 0 && c->info.arm64_lib_count == 0) {
            SSRNX_LOGCI("XAPK", "component entry=%s split=%s SKIP incompatible-native-abi libs=%zu",
                        c->outer_entry, c->info.split_name[0] ? c->info.split_name : "<unnamed>",
                        c->info.native_lib_count);
            continue;
        }
        c->selected = true;
        plan->selected_count++;
    }
    if (plan->selected_count == 0) return SSRNX_ERR_APK_INVALID;
    return SSRNX_OK;
}

static package_PackagePlan package_g_plan_cache;
static bool package_g_plan_cache_valid;
static uint64_t package_g_plan_cache_source_key;
static pthread_mutex_t package_g_plan_cache_lock = PTHREAD_MUTEX_INITIALIZER;
#define SSRNX_PLAN_CACHE_PARKED 3u
static package_PackagePlan package_g_plan_cache_parked[SSRNX_PLAN_CACHE_PARKED];
static uint64_t package_g_plan_cache_parked_key[SSRNX_PLAN_CACHE_PARKED];
static bool package_g_plan_cache_parked_valid[SSRNX_PLAN_CACHE_PARKED];
static package_PackagePlan package_g_plan_cache_scratch;

static SsrnxResult package_discover_plan_uncached(const char *source_path, package_PackagePlan *plan) {
    if (!source_path || !plan) return SSRNX_ERR_ARGUMENT;
    memset(plan, 0, sizeof(*plan));
    plan->base_index = SIZE_MAX;
    if (!package_copy_text(plan->source_path, sizeof(plan->source_path), source_path)) return SSRNX_ERR_ARGUMENT;
    plan->xapk = ssrnx_package_is_xapk_name(source_path);
    if (!plan->xapk) {
        package_PackageComponent *c = &plan->components[0];
        if (!package_copy_text(c->apk_path, sizeof(c->apk_path), source_path)) return SSRNX_ERR_ARGUMENT;
        SsrnxResult rc = ssrnx_apk_inspect(source_path, &c->info);
        if (rc != SSRNX_OK) return rc;
        c->selected = true;
        c->base = true;
        plan->component_count = plan->selected_count = 1;
        plan->base_index = 0;
        return SSRNX_OK;
    }

    SsrnxResult rc = package_materialize_xapk_components(plan);
    if (rc != SSRNX_OK) return rc;
    return package_select_xapk_package(plan);
}

/* Parked plans (MRU first) for the other sources a lookup chain alternates
 * with; see the DEX index parking above. Guarded by g_plan_cache_lock. */
static void package_plan_cache_park_locked(void) {
    if (!package_g_plan_cache_valid) return;
    const size_t last = SSRNX_PLAN_CACHE_PARKED - 1u;
    memmove(&package_g_plan_cache_parked[1], &package_g_plan_cache_parked[0], last * sizeof(package_g_plan_cache_parked[0]));
    memmove(&package_g_plan_cache_parked_key[1], &package_g_plan_cache_parked_key[0], last * sizeof(package_g_plan_cache_parked_key[0]));
    memmove(&package_g_plan_cache_parked_valid[1], &package_g_plan_cache_parked_valid[0], last * sizeof(package_g_plan_cache_parked_valid[0]));
    package_g_plan_cache_parked[0] = package_g_plan_cache;
    package_g_plan_cache_parked_key[0] = package_g_plan_cache_source_key;
    package_g_plan_cache_parked_valid[0] = true;
    package_g_plan_cache_valid = false;
}

static bool package_plan_cache_unpark_locked(const char *source_path) {
    for (size_t i = 0; i < SSRNX_PLAN_CACHE_PARKED; ++i) {
        if (!package_g_plan_cache_parked_valid[i] || strcmp(package_g_plan_cache_parked[i].source_path, source_path) != 0) continue;
        package_g_plan_cache_scratch = package_g_plan_cache_parked[i]; /* static: plans are large */
        const uint64_t hit_key = package_g_plan_cache_parked_key[i];
        memmove(&package_g_plan_cache_parked[1], &package_g_plan_cache_parked[0], i * sizeof(package_g_plan_cache_parked[0]));
        memmove(&package_g_plan_cache_parked_key[1], &package_g_plan_cache_parked_key[0], i * sizeof(package_g_plan_cache_parked_key[0]));
        memmove(&package_g_plan_cache_parked_valid[1], &package_g_plan_cache_parked_valid[0], i * sizeof(package_g_plan_cache_parked_valid[0]));
        package_g_plan_cache_parked_valid[0] = false;
        if (package_g_plan_cache_valid) {
            package_g_plan_cache_parked[0] = package_g_plan_cache;
            package_g_plan_cache_parked_key[0] = package_g_plan_cache_source_key;
            package_g_plan_cache_parked_valid[0] = true;
        }
        package_g_plan_cache = package_g_plan_cache_scratch;
        package_g_plan_cache_source_key = hit_key;
        package_g_plan_cache_valid = true;
        return true;
    }
    return false;
}

static void package_plan_cache_store(const package_PackagePlan *plan, uint64_t key) {
    SSRNX_PERF_LOCK(&package_g_plan_cache_lock);
    if (package_g_plan_cache_valid && strcmp(package_g_plan_cache.source_path, plan->source_path) != 0) package_plan_cache_park_locked();
    package_g_plan_cache = *plan;
    package_g_plan_cache_source_key = key;
    package_g_plan_cache_valid = true;
    pthread_mutex_unlock(&package_g_plan_cache_lock);
}

static SsrnxResult package_discover_plan(const char *source_path, package_PackagePlan *plan) {
    if (!source_path || !plan) return SSRNX_ERR_ARGUMENT;
    SSRNX_PERF_LOCK(&package_g_plan_cache_lock);
    if ((package_g_plan_cache_valid && strcmp(package_g_plan_cache.source_path, source_path) == 0) ||
        package_plan_cache_unpark_locked(source_path)) {
        *plan = package_g_plan_cache;
        pthread_mutex_unlock(&package_g_plan_cache_lock);
        if (ssrnx_diag_full_trace_enabled())
            SSRNX_LOGCI("PACKAGE", "plan cache hit source=%s components=%zu selected=%zu",
                        source_path, plan->component_count, plan->selected_count);
        return SSRNX_OK;
    }
    pthread_mutex_unlock(&package_g_plan_cache_lock);
    const uint64_t key = package_source_cache_key(source_path);

    if (package_read_package_disk_cache(source_path, plan, NULL, NULL)) {
        package_plan_cache_store(plan, key);
        SSRNX_LOGCI("PACKAGE", "persistent plan cache hit source=%s components=%zu selected=%zu",
                    source_path, plan->component_count, plan->selected_count);
        return SSRNX_OK;
    }
    SsrnxResult rc = package_discover_plan_uncached(source_path, plan);
    if (rc == SSRNX_OK) package_plan_cache_store(plan, key);
    return rc;
}


SsrnxResult ssrnx_package_read_selected_entry_concat(const char *source_path,
                                                       const char *entry_name,
                                                       uint8_t **out_data,
                                                       size_t *out_size) {
    if (!source_path || !entry_name || !out_data || !out_size) return SSRNX_ERR_ARGUMENT;
    *out_data = NULL;
    *out_size = 0;

    /* PackagePlan is ~1.5 MB: never on the stack (Java worker threads run
     * with 1 MB stacks). */
    package_PackagePlan *plan = malloc(sizeof(*plan));
    if (!plan) return SSRNX_ERR_NO_MEMORY;
    SsrnxResult rc = package_discover_plan(source_path, plan);
    if (rc != SSRNX_OK) { free(plan); return rc; }

    uint8_t *merged = NULL;
    size_t merged_size = 0;
    for (size_t i = 0; i < plan->component_count; ++i) {
        package_PackageComponent *component = &plan->components[i];
        if (!component->selected) continue;
        rc = package_ensure_component_materialized(plan, i);
        if (rc != SSRNX_OK) { free(merged); free(plan); return rc; }
        bool present = false;
        rc = ssrnx_zip_has_entry(component->apk_path, entry_name, &present);
        if (rc != SSRNX_OK) { free(merged); free(plan); return rc; }
        if (!present) continue;

        uint8_t *part = NULL;
        size_t part_size = 0;
        rc = ssrnx_zip_read_entry_alloc(component->apk_path, entry_name, &part, &part_size);
        if (rc != SSRNX_OK) { free(merged); free(plan); return rc; }
        const bool separator = merged_size > 0 && part_size > 0 && merged[merged_size - 1u] != '\n';
        if (part_size > SIZE_MAX - merged_size - (separator ? 1u : 0u)) {
            free(part); free(merged); free(plan); return SSRNX_ERR_NO_MEMORY;
        }
        const size_t next_size = merged_size + (separator ? 1u : 0u) + part_size;
        uint8_t *next = realloc(merged, next_size ? next_size : 1u);
        if (!next) { free(part); free(merged); free(plan); return SSRNX_ERR_NO_MEMORY; }
        merged = next;
        if (separator) merged[merged_size++] = '\n';
        if (part_size) memcpy(merged + merged_size, part, part_size);
        merged_size += part_size;
        free(part);
    }
    free(plan);
    *out_data = merged;
    *out_size = merged_size;
    return SSRNX_OK;
}

SsrnxResult ssrnx_package_find_selected_entry_path(const char *source_path, const char *entry_name,
                                                   char *out_path, size_t capacity) {
    if (!source_path || !entry_name || !out_path || !capacity) return SSRNX_ERR_ARGUMENT;
    out_path[0] = 0;
    package_PackagePlan *plan = malloc(sizeof(*plan)); /* ~1.5 MB: never on the stack */
    if (!plan) return SSRNX_ERR_NO_MEMORY;
    SsrnxResult rc = package_discover_plan(source_path, plan);
    for (size_t i = 0; rc == SSRNX_OK && i < plan->component_count; ++i) {
        package_PackageComponent *component = &plan->components[i];
        if (!component->selected) continue;
        rc = package_ensure_component_materialized(plan, i);
        bool present = false;
        if (rc == SSRNX_OK) rc = ssrnx_zip_has_entry(component->apk_path, entry_name, &present);
        if (rc != SSRNX_OK || !present) continue;
        if (!package_copy_text(out_path, capacity, component->apk_path)) rc = SSRNX_ERR_ARGUMENT;
        break;
    }
    free(plan);
    return rc;
}

SsrnxResult ssrnx_package_read_selected_entry_first(const char *source_path, const char *entry_name,
                                                   uint8_t **out_data, size_t *out_size, bool *out_found) {
    if (!out_data || !out_size || !out_found) return SSRNX_ERR_ARGUMENT;
    *out_data = NULL; *out_size = 0; *out_found = false;
    char path[SSRNX_APK_PATH_CAP];
    SsrnxResult rc = ssrnx_package_find_selected_entry_path(source_path, entry_name, path, sizeof(path));
    if (rc == SSRNX_OK && path[0]) {
        rc = ssrnx_zip_read_entry_alloc(path, entry_name, out_data, out_size);
        *out_found = rc == SSRNX_OK;
    }
    return rc;
}

/* Diagnostic component path for a DEX image served from the resident cache,
 * without copying the whole plan. */
static void package_plan_cached_component_path(const char *source_path, uint32_t index,
                                       char *out, size_t out_cap) {
    if (!out || !out_cap) return;
    out[0] = '\0';
    SSRNX_PERF_LOCK(&package_g_plan_cache_lock);
    if (package_g_plan_cache_valid && index < package_g_plan_cache.component_count &&
        strcmp(package_g_plan_cache.source_path, source_path) == 0)
        (void)package_copy_text(out, out_cap, package_g_plan_cache.components[index].apk_path);
    pthread_mutex_unlock(&package_g_plan_cache_lock);
}

void ssrnx_package_clear_method_memo(void);
void ssrnx_package_clear_class_memo(void);

void ssrnx_package_invalidate_source_caches(void) {
    ssrnx_package_clear_method_memo();
    ssrnx_package_clear_class_memo();
    SSRNX_PERF_LOCK(&package_g_dex_index_lock);
    package_dex_index_reset_all();
    pthread_mutex_unlock(&package_g_dex_index_lock);
    SSRNX_PERF_LOCK(&package_g_plan_cache_lock);
    package_g_plan_cache_valid = false;
    for (size_t i = 0; i < SSRNX_PLAN_CACHE_PARKED; ++i) package_g_plan_cache_parked_valid[i] = false;
    pthread_mutex_unlock(&package_g_plan_cache_lock);
    ssrnx_package_reset_dex_runtime_cache();
}

#define SSRNX_PACKAGE_MAX_ARM64_NAMES 64u

typedef struct package_JavaNativeNames {
    char short_names[SSRNX_PACKAGE_MAX_ARM64_NAMES][SSRNX_APK_LIBNAME_CAP];
    size_t count;
    bool overflow;
} package_JavaNativeNames;

typedef struct package_JavaNativeNameCtx {
    package_JavaNativeNames *names;
} package_JavaNativeNameCtx;

static bool package_java_native_name_exists(const package_JavaNativeNames *names, const char *short_name) {
    if (!names || !short_name) return false;
    for (size_t i = 0; i < names->count; ++i) {
        if (strcmp(names->short_names[i], short_name) == 0) return true;
    }
    return false;
}

static bool package_java_native_name_visitor(const SsrnxZipEntryInfo *entry, void *userdata) {
    package_JavaNativeNameCtx *ctx = (package_JavaNativeNameCtx *)userdata;
    if (!entry || !ctx || !ctx->names) return false;
    static const char prefix[] = "lib/arm64-v8a/";
    if (strncmp(entry->name, prefix, sizeof(prefix) - 1u) != 0 || !package_ends_with_ci(entry->name, ".so")) {
        return true;
    }
    const char *base = entry->name + sizeof(prefix) - 1u;
    if (strncmp(base, "lib", 3) != 0) return true;
    const size_t n = strlen(base);
    if (n <= 6u || n - 6u >= SSRNX_APK_LIBNAME_CAP) return true;
    char short_name[SSRNX_APK_LIBNAME_CAP];
    memcpy(short_name, base + 3, n - 6u);
    short_name[n - 6u] = '\0';
    if (package_java_native_name_exists(ctx->names, short_name)) return true;
    if (ctx->names->count >= SSRNX_PACKAGE_MAX_ARM64_NAMES) {
        ctx->names->overflow = true;
        return false;
    }
    if (!package_copy_text(ctx->names->short_names[ctx->names->count],
                   sizeof(ctx->names->short_names[0]), short_name)) {
        ctx->names->overflow = true;
        return false;
    }
    ctx->names->count++;
    return true;
}

static uint32_t package_rd32_mem(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool package_dex_skip_uleb128(const uint8_t *data, size_t size, size_t *io_off) {
    if (!data || !io_off) return false;
    size_t off = *io_off;
    for (unsigned i = 0; i < 5; ++i) {
        if (off >= size) return false;
        uint8_t b = data[off++];
        if ((b & 0x80u) == 0) { *io_off = off; return true; }
    }
    return false;
}

static bool package_dex_string_equals(const uint8_t *data, size_t size, uint32_t data_off, const char *want) {
    if (!data || !want || data_off >= size) return false;
    size_t off = data_off;
    if (!package_dex_skip_uleb128(data, size, &off)) return false;
    size_t n = strlen(want);
    if (n > size - off) return false;
    if (memcmp(data + off, want, n) != 0) return false;
    return off + n < size && data[off + n] == 0;
}

static const char *package_dex_string_by_index(const uint8_t *data, size_t size, uint32_t string_index) {
    if (!data || size < 0x70u) return NULL;
    const uint32_t strings_size = package_rd32_mem(data + 0x38);
    const uint32_t strings_off = package_rd32_mem(data + 0x3c);
    if (string_index >= strings_size ||
        (uint64_t)strings_off + (uint64_t)strings_size * 4u > size) return NULL;
    const uint32_t data_off = package_rd32_mem(data + strings_off + (size_t)string_index * 4u);
    if (data_off >= size) return NULL;
    size_t off = data_off;
    if (!package_dex_skip_uleb128(data, size, &off)) return NULL;
    const void *nul = memchr(data + off, 0, size - off);
    return nul ? (const char *)(data + off) : NULL;
}

static const char *package_dex_type_descriptor(const uint8_t *data, size_t size, uint32_t type_index) {
    if (!data || size < 0x70u) return NULL;
    const uint32_t type_size = package_rd32_mem(data + 0x40);
    const uint32_t type_off = package_rd32_mem(data + 0x44);
    if (type_index >= type_size || (uint64_t)type_off + (uint64_t)type_size * 4u > size) return NULL;
    const uint32_t descriptor_index = package_rd32_mem(data + type_off + (size_t)type_index * 4u);
    return package_dex_string_by_index(data, size, descriptor_index);
}

static bool package_dex_descriptor_to_jni_name(const char *descriptor, char *out, size_t out_cap) {
    if (!descriptor || !out || out_cap == 0) return false;
    const size_t n = strlen(descriptor);
    if (n < 3u || descriptor[0] != 'L' || descriptor[n - 1u] != ';' || n - 1u > out_cap) return false;
    const size_t name_len = n - 2u;
    if (name_len + 1u > out_cap) return false;
    memcpy(out, descriptor + 1, name_len);
    out[name_len] = '\0';
    return true;
}

static uint64_t package_dex_descriptor_hash(const char *descriptor, size_t length) {
    return package_fnv1a64_bytes(UINT64_C(1469598103934665603), descriptor, length);
}

/* Builds the descriptor -> class_def table of a resident blob. Called with
 * g_dex_blob_lock held. Allocation failure leaves the linear scan in use. */
static void package_dex_blob_build_class_table(package_DexActiveBlob *slot) {
    const uint8_t *data = slot->data;
    const size_t size = slot->size;
    if (!data || size < 0x70u || memcmp(data, "dex\n", 4) != 0) return;
    const uint32_t class_count = package_rd32_mem(data + 0x60);
    const uint32_t class_off = package_rd32_mem(data + 0x64);
    if (!class_count || class_count > (UINT32_C(1) << 24) ||
        (uint64_t)class_off + (uint64_t)class_count * 32u > size) return;
    uint32_t capacity = 16u;
    while (capacity < class_count * 2u) capacity <<= 1;
    uint32_t *table = calloc(capacity, sizeof(*table));
    if (!table) return;
    const uint32_t mask = capacity - 1u;
    for (uint32_t i = 0; i < class_count; ++i) {
        const char *desc = package_dex_type_descriptor(data, size, package_rd32_mem(data + class_off + (size_t)i * 32u));
        if (!desc) continue;
        uint32_t pos = (uint32_t)package_dex_descriptor_hash(desc, strlen(desc)) & mask;
        while (table[pos]) pos = (pos + 1u) & mask;
        table[pos] = i + 1u;
    }
    slot->class_table = table;
    slot->class_table_mask = mask;
}

/* Narrows a class_def scan for descriptor `wanted` to the single candidate a
 * resident blob's table reports (or to nothing when the DEX does not define
 * it). Blobs outside the resident cache keep the full linear range. */
static void package_dex_class_scan_range(const uint8_t *data, size_t size, const char *wanted,
                                 uint32_t class_off, uint32_t class_count,
                                 uint32_t *out_begin, uint32_t *out_end) {
    *out_begin = 0;
    *out_end = class_count;
    const uint32_t *table = NULL;
    uint32_t mask = 0;
    SSRNX_PERF_LOCK(&package_g_dex_blob_lock);
    if (data == package_g_aosp_core_blob.data && package_g_aosp_core_blob.class_table) {
        table = package_g_aosp_core_blob.class_table;
        mask = package_g_aosp_core_blob.class_table_mask;
    } else if (data == package_g_core_blob.data && package_g_core_blob.class_table) {
        table = package_g_core_blob.class_table;
        mask = package_g_core_blob.class_table_mask;
    }
    for (size_t i = 0; !table && i < package_g_secondary_dex_count; ++i) {
        if (package_g_secondary_dex[i].data == data && package_g_secondary_dex[i].class_table) {
            table = package_g_secondary_dex[i].class_table;
            mask = package_g_secondary_dex[i].class_table_mask;
        }
    }
    for (size_t i = 0; !table && i < SSRNX_DEX_ACTIVE_SLOTS; ++i) {
        package_DexActiveBlob *slot = &package_g_dex_active[i];
        if (!slot->used || slot->data != data || slot->size != size) continue;
        if (!slot->class_table) package_dex_blob_build_class_table(slot);
        table = slot->class_table;
        mask = slot->class_table_mask;
        break;
    }
    pthread_mutex_unlock(&package_g_dex_blob_lock);
    if (!table) return;
    /* The caller holds a reference to the blob, so the table stays valid. */
    for (uint32_t pos = (uint32_t)package_dex_descriptor_hash(wanted, strlen(wanted)) & mask;;
         pos = (pos + 1u) & mask) {
        const uint32_t entry = table[pos];
        if (!entry) { *out_begin = *out_end = 0; return; }
        const uint32_t index = entry - 1u;
        if (index >= class_count) continue;
        const char *desc = package_dex_type_descriptor(data, size,
                                               package_rd32_mem(data + class_off + (size_t)index * 32u));
        if (desc && strcmp(desc, wanted) == 0) { *out_begin = index; *out_end = index + 1u; return; }
    }
}

static SsrnxResult package_dex_find_defined_class(const uint8_t *data, size_t size, const char *class_name,
                                           char *out_super_name, size_t out_super_name_cap,
                                           char *out_interfaces, size_t out_interfaces_cap,
                                           bool *out_found) {
    if (!data || !class_name || !out_found || size < 0x70u || memcmp(data, "dex\n", 4) != 0)
        return SSRNX_ERR_APK_INVALID;
    *out_found = false;
    if (out_super_name && out_super_name_cap) out_super_name[0] = '\0';
    if (out_interfaces && out_interfaces_cap) out_interfaces[0] = '\0';

    char wanted[512];
    const int wn = snprintf(wanted, sizeof(wanted), "L%s;", class_name);
    if (wn <= 0 || (size_t)wn >= sizeof(wanted)) return SSRNX_ERR_ARGUMENT;

    const uint32_t class_count = package_rd32_mem(data + 0x60);
    const uint32_t class_off = package_rd32_mem(data + 0x64);
    if ((uint64_t)class_off + (uint64_t)class_count * 32u > size) return SSRNX_ERR_APK_INVALID;

    uint32_t scan_begin = 0, scan_end = class_count;
    package_dex_class_scan_range(data, size, wanted, class_off, class_count, &scan_begin, &scan_end);
    for (uint32_t i = scan_begin; i < scan_end; ++i) {
        const uint8_t *def = data + class_off + (size_t)i * 32u;
        const uint32_t class_idx = package_rd32_mem(def);
        const char *descriptor = package_dex_type_descriptor(data, size, class_idx);
        if (!descriptor) return SSRNX_ERR_APK_INVALID;
        if (strcmp(descriptor, wanted) != 0) continue;

        const uint32_t super_idx = package_rd32_mem(def + 8u);
        if (super_idx != UINT32_MAX && out_super_name && out_super_name_cap) {
            const char *super_desc = package_dex_type_descriptor(data, size, super_idx);
            if (!super_desc) return SSRNX_ERR_APK_INVALID;
            (void)package_dex_descriptor_to_jni_name(super_desc, out_super_name, out_super_name_cap);
        }
        /* class_def.interfaces_off -> type_list (uint size; ushort type_idx[size]). */
        const uint32_t interfaces_off = package_rd32_mem(def + 12u);
        if (interfaces_off && out_interfaces && out_interfaces_cap) {
            if ((uint64_t)interfaces_off + 4u > size) return SSRNX_ERR_APK_INVALID;
            const uint32_t n = package_rd32_mem(data + interfaces_off);
            if ((uint64_t)interfaces_off + 4u + (uint64_t)n * 2u > size) return SSRNX_ERR_APK_INVALID;
            size_t used = 0;
            for (uint32_t k = 0; k < n; ++k) {
                const uint8_t *entry = data + interfaces_off + 4u + (size_t)k * 2u;
                const uint32_t type_idx = (uint32_t)entry[0] | ((uint32_t)entry[1] << 8);
                char name[512];
                if (!package_dex_descriptor_to_jni_name(package_dex_type_descriptor(data, size, type_idx), name, sizeof(name)))
                    return SSRNX_ERR_APK_INVALID;
                const size_t len = strlen(name);
                if (used + (used ? 1u : 0u) + len + 1u > out_interfaces_cap) break;
                if (used) out_interfaces[used++] = '\n';
                memcpy(out_interfaces + used, name, len + 1u);
                used += len;
            }
        }
        *out_found = true;
        return SSRNX_OK;
    }
    return SSRNX_OK;
}

static uint64_t package_dex_name_hash64(const char *text) {
    uint64_t h = UINT64_C(1469598103934665603);
    return text ? package_fnv1a64_bytes(h, text, strlen(text)) : h;
}

static uint64_t package_dex_class_hash64(const char *class_name) {
    return package_dex_name_hash64(class_name);
}

static void package_dex_index_reset(void) {
    free(package_g_dex_index.entries);
    free(package_g_dex_index.order);
    memset(&package_g_dex_index, 0, sizeof(package_g_dex_index));
}

/* Lookups name their package by the path they were given: an XAPK and each
 * of its materialized split APKs are distinct sources with distinct indexes,
 * and one Java call chain can alternate between them. A single resident index
 * then re-read (and re-sorted) its whole on-disk cache on every switch, per
 * class/method lookup. Keep the most recently used indexes parked in memory
 * (MRU first) and swap them in instead. g_dex_index_lock guards all slots. */
#define SSRNX_DEX_INDEX_PARKED 3u
static package_DexIndexState package_g_dex_index_parked[SSRNX_DEX_INDEX_PARKED];

static void package_dex_index_state_free(package_DexIndexState *state) {
    free(state->entries);
    free(state->order);
    memset(state, 0, sizeof(*state));
}

static void package_dex_index_park_current(void) {
    if (!package_g_dex_index.valid) { package_dex_index_reset(); return; }
    package_dex_index_state_free(&package_g_dex_index_parked[SSRNX_DEX_INDEX_PARKED - 1u]);
    memmove(&package_g_dex_index_parked[1], &package_g_dex_index_parked[0],
            (SSRNX_DEX_INDEX_PARKED - 1u) * sizeof(package_g_dex_index_parked[0]));
    package_g_dex_index_parked[0] = package_g_dex_index;
    memset(&package_g_dex_index, 0, sizeof(package_g_dex_index));
}

static bool package_dex_index_unpark(const char *source_path) {
    for (size_t i = 0; i < SSRNX_DEX_INDEX_PARKED; ++i) {
        if (!package_g_dex_index_parked[i].valid || strcmp(package_g_dex_index_parked[i].source_path, source_path) != 0) continue;
        const package_DexIndexState active = package_g_dex_index;
        package_g_dex_index = package_g_dex_index_parked[i];
        memmove(&package_g_dex_index_parked[1], &package_g_dex_index_parked[0], i * sizeof(package_g_dex_index_parked[0]));
        package_g_dex_index_parked[0] = active;
        return true;
    }
    return false;
}

static void package_dex_index_reset_all(void) {
    package_dex_index_reset();
    for (size_t i = 0; i < SSRNX_DEX_INDEX_PARKED; ++i) package_dex_index_state_free(&package_g_dex_index_parked[i]);
}

static int package_dex_index_order_cmp(const void *a, const void *b) {
    const uint32_t ia = *(const uint32_t *)a, ib = *(const uint32_t *)b;
    const uint64_t ha = package_g_dex_index.entries[ia].class_hash, hb = package_g_dex_index.entries[ib].class_hash;
    if (ha != hb) return ha < hb ? -1 : 1;
    return ia < ib ? -1 : (ia > ib ? 1 : 0);
}

static void package_dex_index_build_order(void) {
    free(package_g_dex_index.order);
    package_g_dex_index.order = NULL;
    if (!package_g_dex_index.count || package_g_dex_index.count > UINT32_MAX) return;
    uint32_t *order = malloc(package_g_dex_index.count * sizeof(*order));
    if (!order) return; /* linear lookup remains correct */
    for (size_t i = 0; i < package_g_dex_index.count; ++i) order[i] = (uint32_t)i;
    qsort(order, package_g_dex_index.count, sizeof(*order), package_dex_index_order_cmp);
    package_g_dex_index.order = order;
}

static bool package_dex_index_file_path(const char *source_path, char *out, size_t out_cap) {
    if (!source_path || !out || out_cap == 0) return false;
    if (!package_ensure_package_cache_root()) return false;
    char name[96];
    const int n = snprintf(name, sizeof(name), "%016" PRIx64 "-%016" PRIx64 ".dexi",
                           package_source_path_hash(source_path), package_source_cache_key(source_path));
    return n > 0 && (size_t)n < sizeof(name) && package_path_join(out, out_cap, SSRNX_PACKAGE_CACHE_ROOT, name);
}

/* Raw-DEX cache file of one index entry for a known package identity key. */
static bool package_dex_cache_file_path_keyed(const char *source_path, uint64_t key, uint32_t component_index,
                                      uint64_t dex_name_hash, char *out, size_t out_cap) {
    if (!source_path || !out || out_cap == 0) return false;
    if (!package_ensure_package_cache_root()) return false;
    char name[128];
    const int n = snprintf(name, sizeof(name), "%016" PRIx64 "-%016" PRIx64 "-c%u-d%016" PRIx64 ".dex",
                           package_source_path_hash(source_path), key, component_index, dex_name_hash);
    return n > 0 && (size_t)n < sizeof(name) && package_path_join(out, out_cap, SSRNX_PACKAGE_CACHE_ROOT, name);
}

/* Identity key of source_path. The active DEX index already carries the
 * identity it was validated against (caller holds g_dex_index_lock); reuse it
 * instead of stat()ing the package again. */
static uint64_t package_dex_source_key_locked(const char *source_path) {
    return package_g_dex_index.valid && strcmp(package_g_dex_index.source_path, source_path) == 0
        ? package_g_dex_index.source_key : package_source_cache_key(source_path);
}

static SsrnxResult package_read_file_alloc(const char *path, uint8_t **out_data, size_t *out_size) {
    if (!path || !out_data || !out_size) return SSRNX_ERR_ARGUMENT;
    *out_data = NULL; *out_size = 0;
    FILE *f = fopen(path, "rb");
    if (!f) return SSRNX_ERR_IO;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return SSRNX_ERR_IO; }
    long end = ftell(f);
    if (end < 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return SSRNX_ERR_IO; }
    size_t size = (size_t)end;
    uint8_t *data = malloc(size ? size : 1u);
    if (!data) { fclose(f); return SSRNX_ERR_NO_MEMORY; }
    if (size && fread(data, 1, size, f) != size) { free(data); fclose(f); return SSRNX_ERR_IO; }
    fclose(f);
    *out_data = data; *out_size = size;
    return SSRNX_OK;
}

static bool package_dex_index_add(uint64_t class_hash, uint32_t component_index, uint64_t dex_name_hash) {
    for (size_t i = 0; i < package_g_dex_index.count; ++i) {
        package_DexIndexEntry *e = &package_g_dex_index.entries[i];
        if (e->class_hash == class_hash && e->component_index == component_index &&
            e->dex_name_hash == dex_name_hash) return true;
    }
    if (package_g_dex_index.count == package_g_dex_index.capacity) {
        size_t next = package_g_dex_index.capacity ? package_g_dex_index.capacity * 2u : 1024u;
        if (next < package_g_dex_index.capacity || next > SIZE_MAX / sizeof(*package_g_dex_index.entries)) return false;
        package_DexIndexEntry *grown = realloc(package_g_dex_index.entries, next * sizeof(*grown));
        if (!grown) return false;
        package_g_dex_index.entries = grown;
        package_g_dex_index.capacity = next;
    }
    package_g_dex_index.entries[package_g_dex_index.count++] = (package_DexIndexEntry){
        .class_hash = class_hash, .dex_name_hash = dex_name_hash,
        .component_index = component_index, .reserved = 0
    };
    return true;
}

static SsrnxResult package_dex_index_add_classes(const uint8_t *data, size_t size,
                                         uint32_t component_index, uint64_t dex_name_hash) {
    if (!data || size < 0x70u || memcmp(data, "dex\n", 4) != 0) return SSRNX_ERR_APK_INVALID;
    const uint32_t class_count = package_rd32_mem(data + 0x60);
    const uint32_t class_off = package_rd32_mem(data + 0x64);
    if ((uint64_t)class_off + (uint64_t)class_count * 32u > size) return SSRNX_ERR_APK_INVALID;
    for (uint32_t i = 0; i < class_count; ++i) {
        const uint8_t *def = data + class_off + (size_t)i * 32u;
        const char *desc = package_dex_type_descriptor(data, size, package_rd32_mem(def));
        if (!desc) return SSRNX_ERR_APK_INVALID;
        char class_name[512];
        if (!package_dex_descriptor_to_jni_name(desc, class_name, sizeof(class_name))) continue;
        if (!package_dex_index_add(package_dex_class_hash64(class_name), component_index, dex_name_hash))
            return SSRNX_ERR_NO_MEMORY;
    }
    return SSRNX_OK;
}

static SsrnxResult package_ensure_cached_dex_file(const char *source_path, const char *apk_path,
                                          uint32_t component_index, uint64_t source_key,
                                          const SsrnxZipEntryInfo *entry,
                                          char *out_path, size_t out_path_cap,
                                          bool *out_reused) {
    if (!source_path || !apk_path || !entry || !out_path) return SSRNX_ERR_ARGUMENT;
    const uint64_t name_hash = package_dex_name_hash64(entry->name);
    if (!package_dex_cache_file_path_keyed(source_path, source_key, component_index, name_hash, out_path, out_path_cap))
        return SSRNX_ERR_IO;
    struct stat st;
    if (stat(out_path, &st) == 0 && S_ISREG(st.st_mode) && (uint64_t)st.st_size == entry->uncompressed_size) {
        if (out_reused) *out_reused = true;
        return SSRNX_OK;
    }
    if (out_reused) *out_reused = false;
    char tmp[SSRNX_APK_PATH_CAP];
    const int n = snprintf(tmp, sizeof(tmp), "%s.tmp", out_path);
    if (n <= 0 || (size_t)n >= sizeof(tmp)) return SSRNX_ERR_ARGUMENT;
    (void)unlink(tmp);
    SsrnxResult rc = ssrnx_zip_extract_entry(apk_path, entry->name, tmp);
    if (rc != SSRNX_OK) { (void)unlink(tmp); return rc; }
    if (rename(tmp, out_path) != 0) { (void)unlink(tmp); return SSRNX_ERR_IO; }
    return SSRNX_OK;
}

typedef struct package_DexIndexBuildCtx {
    const char *source_path;
    const char *apk_path;
    uint32_t component_index;
    SsrnxResult rc;
    size_t dex_count;
    size_t extracted;
    size_t reused;
} package_DexIndexBuildCtx;

static bool package_dex_index_build_visitor(const SsrnxZipEntryInfo *entry, void *userdata) {
    package_DexIndexBuildCtx *ctx = (package_DexIndexBuildCtx *)userdata;
    if (!entry || !ctx) return false;
    const char *base = strrchr(entry->name, '/');
    base = base ? base + 1 : entry->name;
    if (strncmp(base, "classes", 7) != 0 || !package_ends_with_ci(base, ".dex")) return true;
    char cache_path[SSRNX_APK_PATH_CAP];
    bool reused = false;
    SsrnxResult rc = package_ensure_cached_dex_file(ctx->source_path, ctx->apk_path, ctx->component_index,
                                            package_g_dex_index.source_key, entry, cache_path, sizeof(cache_path),
                                            &reused);
    if (rc != SSRNX_OK) { ctx->rc = rc; return false; }
    uint8_t *data = NULL; size_t size = 0;
    rc = package_read_file_alloc(cache_path, &data, &size);
    if (rc == SSRNX_OK)
        rc = package_dex_index_add_classes(data, size, ctx->component_index, package_dex_name_hash64(entry->name));
    free(data);
    if (rc != SSRNX_OK) { ctx->rc = rc; return false; }
    ctx->dex_count++;
    if (reused) ctx->reused++; else ctx->extracted++;
    return true;
}

static bool package_dex_index_write(const char *source_path) {
    char path[SSRNX_APK_PATH_CAP];
    if (!package_dex_index_file_path(source_path, path, sizeof(path))) return false;
    char tmp[SSRNX_APK_PATH_CAP];
    const int n = snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    if (n <= 0 || (size_t)n >= sizeof(tmp)) return false;
    FILE *f = fopen(tmp, "wb");
    if (!f) return false;
    package_DexIndexDiskHeader hdr = {
        .magic = SSRNX_DEX_INDEX_MAGIC, .version = SSRNX_DEX_INDEX_VERSION,
        .entry_size = (uint32_t)sizeof(package_DexIndexEntry), .source_key = package_source_cache_key(source_path),
        .entry_count = (uint32_t)package_g_dex_index.count, .reserved = 0
    };
    bool ok = fwrite(&hdr, 1, sizeof(hdr), f) == sizeof(hdr) &&
              (!package_g_dex_index.count || fwrite(package_g_dex_index.entries, sizeof(package_DexIndexEntry), package_g_dex_index.count, f) == package_g_dex_index.count);
    if (fclose(f) != 0) ok = false;
    if (!ok) { (void)unlink(tmp); return false; }
    if (rename(tmp, path) != 0) { (void)unlink(tmp); return false; }
    return true;
}

static bool package_dex_index_load(const char *source_path) {
    char path[SSRNX_APK_PATH_CAP];
    if (!package_dex_index_file_path(source_path, path, sizeof(path))) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    package_DexIndexDiskHeader hdr;
    if (fread(&hdr, 1, sizeof(hdr), f) != sizeof(hdr) || hdr.magic != SSRNX_DEX_INDEX_MAGIC ||
        hdr.version != SSRNX_DEX_INDEX_VERSION || hdr.entry_size != sizeof(package_DexIndexEntry) ||
        hdr.source_key != package_source_cache_key(source_path)) { fclose(f); return false; }
    package_DexIndexEntry *entries = NULL;
    if (hdr.entry_count) {
        entries = malloc((size_t)hdr.entry_count * sizeof(*entries));
        if (!entries || fread(entries, sizeof(*entries), hdr.entry_count, f) != hdr.entry_count) {
            free(entries); fclose(f); return false;
        }
    }
    fclose(f);
    package_dex_index_reset();
    package_g_dex_index.valid = true;
    package_g_dex_index.source_key = hdr.source_key;
    (void)package_copy_text(package_g_dex_index.source_path, sizeof(package_g_dex_index.source_path), source_path);
    package_g_dex_index.entries = entries;
    package_g_dex_index.count = package_g_dex_index.capacity = hdr.entry_count;
    package_dex_index_build_order();
    return true;
}

static SsrnxResult package_prepare_dex_cache_locked(const char *source_path) {
    if (!source_path || !source_path[0]) return SSRNX_ERR_ARGUMENT;
    /* The index was validated against the package identity when it was
     * loaded or built. Installed packages only change through the installer,
     * which calls ssrnx_package_invalidate_source_caches(), so every later
     * lookup skips the stat() of the package. */
    if (package_g_dex_index.valid && strcmp(package_g_dex_index.source_path, source_path) == 0) return SSRNX_OK;
    if (package_dex_index_unpark(source_path)) return SSRNX_OK;
    const uint64_t key = package_source_cache_key(source_path);
    package_dex_index_park_current();
    if (package_dex_index_load(source_path)) {
        SSRNX_LOGCI("PERF", "DEX index cache hit classes=%zu", package_g_dex_index.count);
        return SSRNX_OK;
    }

    package_PackagePlan *plan = malloc(sizeof(*plan));
    if (!plan) return SSRNX_ERR_NO_MEMORY;
    SsrnxResult rc = package_discover_plan(source_path, plan);
    if (rc != SSRNX_OK) { free(plan); return rc; }
    package_dex_index_reset();
    package_g_dex_index.valid = true;
    package_g_dex_index.source_key = key;
    (void)package_copy_text(package_g_dex_index.source_path, sizeof(package_g_dex_index.source_path), source_path);
    size_t dex_count = 0, extracted = 0, reused = 0;
    for (size_t pass = 0; pass < 2; ++pass) {
        for (size_t i = 0; i < plan->component_count; ++i) {
            if (!plan->components[i].selected) continue;
            if ((pass == 0 && i != plan->base_index) || (pass == 1 && i == plan->base_index)) continue;
            rc = package_ensure_component_materialized(plan, i);
            if (rc != SSRNX_OK) break;
            package_DexIndexBuildCtx ctx = {
                .source_path = source_path, .apk_path = plan->components[i].apk_path,
                .component_index = (uint32_t)i, .rc = SSRNX_OK
            };
            rc = ssrnx_zip_visit(plan->components[i].apk_path, package_dex_index_build_visitor, &ctx);
            if (rc == SSRNX_OK) rc = ctx.rc;
            dex_count += ctx.dex_count; extracted += ctx.extracted; reused += ctx.reused;
            if (rc != SSRNX_OK) break;
        }
        if (rc != SSRNX_OK) break;
    }
    if (rc == SSRNX_OK && !package_dex_index_write(source_path)) rc = SSRNX_ERR_IO;
    if (rc == SSRNX_OK) package_dex_index_build_order();
    SSRNX_LOGCI("PERF", "DEX index cache %s classes=%zu dex=%zu extracted=%zu reused=%zu",
                rc == SSRNX_OK ? "build" : "fail", package_g_dex_index.count, dex_count, extracted, reused);
    free(plan);
    if (rc != SSRNX_OK) package_dex_index_reset();
    return rc;
}

SsrnxResult ssrnx_package_prepare_dex_cache(const char *source_path) {
    SSRNX_PERF_LOCK(&package_g_dex_index_lock);
    SsrnxResult rc = package_prepare_dex_cache_locked(source_path);
    pthread_mutex_unlock(&package_g_dex_index_lock);
    return rc;
}

/* Iterates index entries for class_hash. Pass start=0 for the first match and
 * the returned *out_next for each following one. */
static bool package_dex_index_find(uint64_t class_hash, size_t start, package_DexIndexEntry *out, size_t *out_next) {
    if (!package_g_dex_index.valid) return false;
    if (package_g_dex_index.order) {
        size_t pos = start;
        if (start == 0) {
            size_t lo = 0, hi = package_g_dex_index.count;
            while (lo < hi) {
                const size_t mid = lo + (hi - lo) / 2u;
                if (package_g_dex_index.entries[package_g_dex_index.order[mid]].class_hash < class_hash) lo = mid + 1u;
                else hi = mid;
            }
            pos = lo;
        }
        if (pos >= package_g_dex_index.count) return false;
        const package_DexIndexEntry *e = &package_g_dex_index.entries[package_g_dex_index.order[pos]];
        if (e->class_hash != class_hash) return false;
        if (out) *out = *e;
        if (out_next) *out_next = pos + 1u;
        return true;
    }
    for (size_t i = start; i < package_g_dex_index.count; ++i) {
        if (package_g_dex_index.entries[i].class_hash != class_hash) continue;
        if (out) *out = package_g_dex_index.entries[i];
        if (out_next) *out_next = i + 1u;
        return true;
    }
    return false;
}

typedef struct package_DexEntryHashCtx {
    uint64_t want_hash;
    SsrnxZipEntryInfo found;
    bool present;
} package_DexEntryHashCtx;

static bool package_dex_entry_hash_visitor(const SsrnxZipEntryInfo *entry, void *userdata) {
    package_DexEntryHashCtx *ctx = (package_DexEntryHashCtx *)userdata;
    if (!entry || !ctx) return false;
    const char *base = strrchr(entry->name, '/');
    base = base ? base + 1 : entry->name;
    if (strncmp(base, "classes", 7) == 0 && package_ends_with_ci(base, ".dex") &&
        package_dex_name_hash64(entry->name) == ctx->want_hash) {
        ctx->found = *entry; ctx->present = true; return false;
    }
    return true;
}

/* Serializes re-extraction of a missing raw-DEX cache file; loads run
 * without g_dex_index_lock, so two threads may miss the same file. */
static pthread_mutex_t package_g_dex_extract_lock = PTHREAD_MUTEX_INITIALIZER;

static SsrnxResult package_load_indexed_dex(package_PackagePlan *plan, const char *source_path,
                                    const package_DexIndexEntry *loc, uint64_t source_key,
                                    const char *cache_path_in, uint8_t **out_data, size_t *out_size,
                                    void **out_owner,
                                    char *out_dex_name, size_t out_dex_name_cap) {
    if (!plan || !source_path || !loc || !cache_path_in || !out_data || !out_size || !out_owner ||
        loc->component_index >= plan->component_count) return SSRNX_ERR_ARGUMENT;
    package_PackageComponent *component = &plan->components[loc->component_index];
    SsrnxResult rc = package_ensure_component_materialized(plan, loc->component_index);
    if (rc != SSRNX_OK) return rc;
    char cache_path[SSRNX_APK_PATH_CAP];
    if (!package_copy_text(cache_path, sizeof(cache_path), cache_path_in)) return SSRNX_ERR_IO;
    struct stat st;
    package_DexEntryHashCtx find = {.want_hash = loc->dex_name_hash};
    pthread_mutex_lock(&package_g_dex_extract_lock);
    if (stat(cache_path, &st) != 0 || !S_ISREG(st.st_mode)) {
        /* Only scan the APK ZIP when the persistent raw-DEX cache is actually
         * missing and we must locate/extract the original entry. On cache hits
         * the entry name is diagnostic-only; rescanning a 6k-entry APK for
         * every Java method used to dominate hardware startup time. */
        rc = ssrnx_zip_visit(component->apk_path, package_dex_entry_hash_visitor, &find);
        if (rc != SSRNX_OK || !find.present) {
            pthread_mutex_unlock(&package_g_dex_extract_lock);
            return rc != SSRNX_OK ? rc : SSRNX_ERR_APK_INVALID;
        }
        bool reused = false;
        rc = package_ensure_cached_dex_file(source_path, component->apk_path, loc->component_index, source_key,
                                    &find.found, cache_path, sizeof(cache_path), &reused);
        if (rc != SSRNX_OK) { pthread_mutex_unlock(&package_g_dex_extract_lock); return rc; }
    }
    pthread_mutex_unlock(&package_g_dex_extract_lock);
    if (out_dex_name && out_dex_name_cap) {
        if (find.present) (void)package_copy_text(out_dex_name, out_dex_name_cap, find.found.name);
        else snprintf(out_dex_name, out_dex_name_cap, "dex-%016" PRIx64, loc->dex_name_hash);
    }
    return package_dex_blob_acquire(cache_path, out_data, out_size, out_owner);
}

/* Acquires the DEX image for an index entry; the caller holds
 * g_dex_index_lock. A resident image is served with no file system access and
 * without the package plan. Loading a non-resident image (megabytes from the
 * SD card) runs with g_dex_index_lock released, so lookups that hit resident
 * images are not serialized behind it; the lock is re-taken and this source's
 * index made active again before returning. Callers therefore keep only index
 * positions (never g_dex_index pointers) across this call. */
static SsrnxResult package_dex_index_acquire(const char *source_path, const package_DexIndexEntry *loc,
                                     package_PackagePlan **io_plan, uint8_t **out_data, size_t *out_size,
                                     void **out_owner, char *out_dex_name, size_t out_dex_name_cap) {
    const uint64_t source_key = package_dex_source_key_locked(source_path);
    char cache_path[SSRNX_APK_PATH_CAP];
    if (!package_dex_cache_file_path_keyed(source_path, source_key, loc->component_index, loc->dex_name_hash,
                                   cache_path, sizeof(cache_path))) return SSRNX_ERR_IO;
    if (package_dex_blob_acquire_resident(cache_path, out_data, out_size, out_owner)) {
        if (out_dex_name && out_dex_name_cap)
            snprintf(out_dex_name, out_dex_name_cap, "dex-%016" PRIx64, loc->dex_name_hash);
        return SSRNX_OK;
    }
    const package_DexIndexEntry entry = *loc;
    pthread_mutex_unlock(&package_g_dex_index_lock);
    SsrnxResult rc = SSRNX_OK;
    if (!*io_plan) {
        package_PackagePlan *plan = malloc(sizeof(*plan));
        if (!plan) rc = SSRNX_ERR_NO_MEMORY;
        else if ((rc = package_discover_plan(source_path, plan)) != SSRNX_OK) free(plan);
        else *io_plan = plan;
    }
    if (rc == SSRNX_OK)
        rc = package_load_indexed_dex(*io_plan, source_path, &entry, source_key, cache_path, out_data, out_size,
                              out_owner, out_dex_name, out_dex_name_cap);
    SSRNX_PERF_LOCK(&package_g_dex_index_lock);
    const SsrnxResult index_rc = package_prepare_dex_cache_locked(source_path);
    if (rc == SSRNX_OK && index_rc != SSRNX_OK) {
        package_dex_blob_release(*out_owner, *out_data);
        *out_data = NULL; *out_size = 0; *out_owner = NULL;
        rc = index_rc;
    }
    return rc;
}

static void package_dex_index_component_path(const char *source_path, const package_PackagePlan *plan,
                                     uint32_t component_index, char *out, size_t out_cap) {
    if (plan && component_index < plan->component_count)
        (void)package_copy_text(out, out_cap, plan->components[component_index].apk_path);
    else
        package_plan_cached_component_path(source_path, component_index, out, out_cap);
}

/* Index lookups hold g_dex_index_lock only to find a class's index entries and
 * pin their (refcounted) DEX images. Parsing the class data then runs without
 * the lock, so threads resolving different methods proceed in parallel. */
#define DEX_INDEX_CANDIDATES 8u
typedef struct package_DexIndexCandidate {
    uint8_t *data;
    size_t size;
    void *owner;
    package_DexIndexEntry loc;
    char dex_name[96];
} package_DexIndexCandidate;

static void package_dex_index_release_candidates(package_DexIndexCandidate *cands, size_t from, size_t count) {
    for (size_t i = from; i < count; ++i) package_dex_blob_release(cands[i].owner, cands[i].data);
}

/* *out_ready is false when the index is unavailable; callers then use the
 * uncached ZIP scan. A non-OK result with *out_ready set is a real error. */
static SsrnxResult package_dex_index_collect(const char *source_path, const char *class_name,
                                     package_DexIndexCandidate *out, size_t *out_count,
                                     package_PackagePlan **io_plan, bool *out_ready) {
    *out_count = 0;
    *out_ready = false;
    SSRNX_PERF_LOCK(&package_g_dex_index_lock);
    SsrnxResult rc = package_prepare_dex_cache_locked(source_path);
    if (rc != SSRNX_OK) {
        pthread_mutex_unlock(&package_g_dex_index_lock);
        return SSRNX_OK;
    }
    *out_ready = true;
    const uint64_t hash = package_dex_class_hash64(class_name);
    size_t pos = 0, next = 0;
    package_DexIndexEntry loc;
    while (*out_count < DEX_INDEX_CANDIDATES && package_dex_index_find(hash, pos, &loc, &next)) {
        pos = next;
        package_DexIndexCandidate *c = &out[*out_count];
        memset(c, 0, sizeof(*c));
        c->loc = loc;
        rc = package_dex_index_acquire(source_path, &loc, io_plan, &c->data, &c->size, &c->owner,
                               c->dex_name, sizeof(c->dex_name));
        if (rc != SSRNX_OK) break;
        (*out_count)++;
    }
    pthread_mutex_unlock(&package_g_dex_index_lock);
    if (rc != SSRNX_OK) {
        package_dex_index_release_candidates(out, 0, *out_count);
        *out_count = 0;
    }
    return rc;
}

static bool package_dex_read_uleb128_value(const uint8_t *data, size_t size, size_t *inout_off,
                                   uint32_t *out_value) {
    if (!data || !inout_off || !out_value) return false;
    size_t off = *inout_off;
    uint32_t value = 0;
    unsigned shift = 0;
    for (unsigned i = 0; i < 5; ++i) {
        if (off >= size) return false;
        const uint8_t b = data[off++];
        value |= (uint32_t)(b & 0x7fu) << shift;
        if ((b & 0x80u) == 0) {
            *inout_off = off;
            *out_value = value;
            return true;
        }
        shift += 7;
    }
    return false;
}

static bool package_dex_proto_descriptor(const uint8_t *data, size_t size, uint32_t proto_index,
                                 char *out, size_t out_cap) {
    if (!data || !out || out_cap < 4u || size < 0x70u) return false;
    const uint32_t proto_count = package_rd32_mem(data + 0x48);
    const uint32_t proto_off = package_rd32_mem(data + 0x4c);
    if (proto_index >= proto_count || (uint64_t)proto_off + (uint64_t)proto_count * 12u > size)
        return false;
    const uint8_t *proto = data + proto_off + (size_t)proto_index * 12u;
    const uint32_t return_type_idx = package_rd32_mem(proto + 4u);
    const uint32_t params_off = package_rd32_mem(proto + 8u);
    const char *ret = package_dex_type_descriptor(data, size, return_type_idx);
    if (!ret) return false;

    size_t used = 0;
    out[used++] = '(';
    if (params_off) {
        if ((uint64_t)params_off + 4u > size) return false;
        const uint32_t count = package_rd32_mem(data + params_off);
        if ((uint64_t)params_off + 4u + (uint64_t)count * 2u > size) return false;
        for (uint32_t i = 0; i < count; ++i) {
            const uint16_t type_idx = (uint16_t)(data[params_off + 4u + i * 2u] |
                                                 ((uint16_t)data[params_off + 5u + i * 2u] << 8));
            const char *desc = package_dex_type_descriptor(data, size, type_idx);
            if (!desc) return false;
            const size_t n = strlen(desc);
            if (used + n + 2u >= out_cap) return false;
            memcpy(out + used, desc, n);
            used += n;
        }
    }
    if (used + 2u + strlen(ret) >= out_cap) return false;
    out[used++] = ')';
    memcpy(out + used, ret, strlen(ret) + 1u);
    return true;
}

static bool package_dex_method_identity(const uint8_t *data, size_t size, uint32_t method_index,
                                char *out_class, size_t out_class_cap,
                                char *out_name, size_t out_name_cap,
                                char *out_sig, size_t out_sig_cap) {
    if (!data || size < 0x70u) return false;
    const uint32_t method_count = package_rd32_mem(data + 0x58);
    const uint32_t method_off = package_rd32_mem(data + 0x5c);
    if (method_index >= method_count ||
        (uint64_t)method_off + (uint64_t)method_count * 8u > size) return false;
    const uint8_t *item = data + method_off + (size_t)method_index * 8u;
    const uint16_t class_idx = (uint16_t)(item[0] | ((uint16_t)item[1] << 8));
    const uint16_t proto_idx = (uint16_t)(item[2] | ((uint16_t)item[3] << 8));
    const uint32_t name_idx = package_rd32_mem(item + 4u);
    const char *class_desc = package_dex_type_descriptor(data, size, class_idx);
    const char *name = package_dex_string_by_index(data, size, name_idx);
    char class_name[512];
    if (!class_desc || !name || !package_dex_descriptor_to_jni_name(class_desc, class_name, sizeof(class_name)))
        return false;
    if (out_class && out_class_cap && !package_copy_text(out_class, out_class_cap, class_name)) return false;
    if (out_name && out_name_cap && !package_copy_text(out_name, out_name_cap, name)) return false;
    if (out_sig && out_sig_cap && !package_dex_proto_descriptor(data, size, proto_idx, out_sig, out_sig_cap))
        return false;
    return true;
}


static const char *package_dex_method_name_ref(const uint8_t *data, size_t size, uint32_t method_index) {
    if (!data || size < 0x70u) return NULL;
    const uint32_t method_count = package_rd32_mem(data + 0x58);
    const uint32_t method_off = package_rd32_mem(data + 0x5c);
    if (method_index >= method_count ||
        (uint64_t)method_off + (uint64_t)method_count * 8u > size) return NULL;
    return package_dex_string_by_index(data, size, package_rd32_mem(data + method_off + (size_t)method_index * 8u + 4u));
}

#define DEX_VALUE_BYTE       0x00u
#define DEX_VALUE_SHORT      0x02u
#define DEX_VALUE_CHAR       0x03u
#define DEX_VALUE_INT        0x04u
#define DEX_VALUE_LONG       0x06u
#define DEX_VALUE_FLOAT      0x10u
#define DEX_VALUE_DOUBLE     0x11u
#define DEX_VALUE_METHOD_TYPE 0x15u
#define DEX_VALUE_METHOD_HANDLE 0x16u
#define DEX_VALUE_STRING     0x17u
#define DEX_VALUE_TYPE       0x18u
#define DEX_VALUE_FIELD      0x19u
#define DEX_VALUE_METHOD     0x1au
#define DEX_VALUE_ENUM       0x1bu
#define DEX_VALUE_ARRAY      0x1cu
#define DEX_VALUE_ANNOTATION 0x1du
#define DEX_VALUE_NULL       0x1eu
#define DEX_VALUE_BOOLEAN    0x1fu

static bool package_dex_read_compact_unsigned(const uint8_t *data, size_t size, size_t *io_off,
                                      unsigned byte_count, uint64_t *out_value) {
    if (!data || !io_off || !out_value || byte_count == 0 || byte_count > 8u ||
        *io_off > size || byte_count > size - *io_off) return false;
    uint64_t value = 0;
    for (unsigned i = 0; i < byte_count; ++i)
        value |= (uint64_t)data[*io_off + i] << (i * 8u);
    *io_off += byte_count;
    *out_value = value;
    return true;
}

static uint64_t package_dex_sign_extend_compact(uint64_t value, unsigned byte_count) {
    if (byte_count >= 8u) return value;
    const unsigned bits = byte_count * 8u;
    const uint64_t sign = UINT64_C(1) << (bits - 1u);
    if (value & sign) value |= UINT64_MAX << bits;
    return value;
}

static bool package_dex_skip_encoded_value_depth(const uint8_t *data, size_t size, size_t *io_off,
                                         unsigned depth) {
    if (!data || !io_off || *io_off >= size || depth > 16u) return false;
    const uint8_t head = data[(*io_off)++];
    const uint8_t type = head & 0x1fu;
    const uint8_t arg = head >> 5;
    switch (type) {
        case DEX_VALUE_BYTE:
            if (arg != 0u || *io_off >= size) return false;
            (*io_off)++;
            return true;
        case DEX_VALUE_SHORT: if (arg > 1u) return false; break;
        case DEX_VALUE_CHAR: if (arg > 1u) return false; break;
        case DEX_VALUE_INT: case DEX_VALUE_FLOAT:
        case DEX_VALUE_METHOD_TYPE: case DEX_VALUE_METHOD_HANDLE:
        case DEX_VALUE_STRING: case DEX_VALUE_TYPE: case DEX_VALUE_FIELD:
        case DEX_VALUE_METHOD: case DEX_VALUE_ENUM:
            if (arg > 3u) return false;
            break;
        case DEX_VALUE_LONG: case DEX_VALUE_DOUBLE:
            if (arg > 7u) return false;
            break;
        case DEX_VALUE_NULL:
            return arg == 0u;
        case DEX_VALUE_BOOLEAN:
            return arg <= 1u;
        case DEX_VALUE_ARRAY: {
            if (arg != 0u) return false;
            uint32_t count = 0;
            if (!package_dex_read_uleb128_value(data, size, io_off, &count)) return false;
            for (uint32_t i = 0; i < count; ++i)
                if (!package_dex_skip_encoded_value_depth(data, size, io_off, depth + 1u)) return false;
            return true;
        }
        case DEX_VALUE_ANNOTATION: {
            if (arg != 0u) return false;
            uint32_t ignored_type = 0, count = 0;
            if (!package_dex_read_uleb128_value(data, size, io_off, &ignored_type) ||
                !package_dex_read_uleb128_value(data, size, io_off, &count)) return false;
            for (uint32_t i = 0; i < count; ++i) {
                uint32_t ignored_name = 0;
                if (!package_dex_read_uleb128_value(data, size, io_off, &ignored_name) ||
                    !package_dex_skip_encoded_value_depth(data, size, io_off, depth + 1u)) return false;
            }
            return true;
        }
        default:
            return false;
    }
    const size_t byte_count = (size_t)arg + 1u;
    if (*io_off > size || byte_count > size - *io_off) return false;
    *io_off += byte_count;
    return true;
}

static bool package_dex_decode_static_value(const uint8_t *data, size_t size, size_t *io_off,
                                    SsrnxDexStaticValue *out) {
    if (!data || !io_off || !out || *io_off >= size) return false;
    memset(out, 0, sizeof(*out));
    const uint8_t head = data[(*io_off)++];
    const uint8_t type = head & 0x1fu;
    const uint8_t arg = head >> 5;
    uint64_t raw = 0;
    unsigned bytes = 0;
    switch (type) {
        case DEX_VALUE_BYTE:
            if (arg != 0u) return false;
            bytes = 1u;
            if (!package_dex_read_compact_unsigned(data, size, io_off, bytes, &raw)) return false;
            out->type = SSRNX_DEX_STATIC_BYTE;
            out->bits = package_dex_sign_extend_compact(raw, bytes);
            return true;
        case DEX_VALUE_SHORT:
            if (arg > 1u) return false;
            bytes = (unsigned)arg + 1u;
            if (!package_dex_read_compact_unsigned(data, size, io_off, bytes, &raw)) return false;
            out->type = SSRNX_DEX_STATIC_SHORT;
            out->bits = package_dex_sign_extend_compact(raw, bytes);
            return true;
        case DEX_VALUE_CHAR:
            if (arg > 1u) return false;
            bytes = (unsigned)arg + 1u;
            if (!package_dex_read_compact_unsigned(data, size, io_off, bytes, &raw)) return false;
            out->type = SSRNX_DEX_STATIC_CHAR;
            out->bits = raw;
            return true;
        case DEX_VALUE_INT:
            if (arg > 3u) return false;
            bytes = (unsigned)arg + 1u;
            if (!package_dex_read_compact_unsigned(data, size, io_off, bytes, &raw)) return false;
            out->type = SSRNX_DEX_STATIC_INT;
            out->bits = package_dex_sign_extend_compact(raw, bytes);
            return true;
        case DEX_VALUE_LONG:
            if (arg > 7u) return false;
            bytes = (unsigned)arg + 1u;
            if (!package_dex_read_compact_unsigned(data, size, io_off, bytes, &raw)) return false;
            out->type = SSRNX_DEX_STATIC_LONG;
            out->bits = package_dex_sign_extend_compact(raw, bytes);
            return true;
        case DEX_VALUE_FLOAT:
            if (arg > 3u) return false;
            bytes = (unsigned)arg + 1u;
            if (!package_dex_read_compact_unsigned(data, size, io_off, bytes, &raw)) return false;
            out->type = SSRNX_DEX_STATIC_FLOAT;
            out->bits = raw << ((4u - bytes) * 8u);
            return true;
        case DEX_VALUE_DOUBLE:
            if (arg > 7u) return false;
            bytes = (unsigned)arg + 1u;
            if (!package_dex_read_compact_unsigned(data, size, io_off, bytes, &raw)) return false;
            out->type = SSRNX_DEX_STATIC_DOUBLE;
            out->bits = raw << ((8u - bytes) * 8u);
            return true;
        case DEX_VALUE_STRING: {
            if (arg > 3u) return false;
            bytes = (unsigned)arg + 1u;
            if (!package_dex_read_compact_unsigned(data, size, io_off, bytes, &raw) || raw > UINT32_MAX)
                return false;
            const char *text = package_dex_string_by_index(data, size, (uint32_t)raw);
            if (!text || !package_copy_text(out->string_value, sizeof(out->string_value), text)) return false;
            out->type = SSRNX_DEX_STATIC_STRING;
            return true;
        }
        case DEX_VALUE_NULL:
            if (arg != 0u) return false;
            out->type = SSRNX_DEX_STATIC_NULL;
            return true;
        case DEX_VALUE_BOOLEAN:
            if (arg > 1u) return false;
            out->type = SSRNX_DEX_STATIC_BOOLEAN;
            out->bits = arg;
            return true;
        default:
            /* Reflective constants (Class/Field/Method/enum), arrays and
             * annotations require object materialization beyond the primitive
             * lazy-field bridge. Preserve correctness by reporting them as an
             * explicit unsupported initializer instead of silently using zero. */
            out->type = SSRNX_DEX_STATIC_UNSUPPORTED;
            /* This is the target field's value, so callers stop here and
             * report unsupported rather than parsing any following element.
             * The header above has already been consumed; calling the generic
             * skip helper here would incorrectly treat payload bytes as a new
             * encoded-value header. */
            return true;
    }
}

static bool package_dex_field_identity(const uint8_t *data, size_t size, uint32_t field_idx,
                               char *out_class, size_t out_class_cap,
                               char *out_name, size_t out_name_cap,
                               char *out_sig, size_t out_sig_cap) {
    if (!data || size < 0x70u) return false;
    const uint32_t field_count = package_rd32_mem(data + 0x50);
    const uint32_t field_off = package_rd32_mem(data + 0x54);
    if (field_idx >= field_count || (uint64_t)field_off + (uint64_t)field_count * 8u > size)
        return false;
    const uint8_t *item = data + field_off + (size_t)field_idx * 8u;
    const uint16_t class_idx = (uint16_t)(item[0] | ((uint16_t)item[1] << 8));
    const uint16_t type_idx = (uint16_t)(item[2] | ((uint16_t)item[3] << 8));
    const uint32_t name_idx = package_rd32_mem(item + 4u);
    const char *class_desc = package_dex_type_descriptor(data, size, class_idx);
    const char *type_desc = package_dex_type_descriptor(data, size, type_idx);
    const char *name = package_dex_string_by_index(data, size, name_idx);
    char class_name[512];
    if (!class_desc || !type_desc || !name ||
        !package_dex_descriptor_to_jni_name(class_desc, class_name, sizeof(class_name))) return false;
    return package_copy_text(out_class, out_class_cap, class_name) &&
           package_copy_text(out_name, out_name_cap, name) && package_copy_text(out_sig, out_sig_cap, type_desc);
}

static SsrnxResult package_dex_find_static_field_initial_value(const uint8_t *data, size_t size,
                                                        const char *class_name,
                                                        const char *field_name,
                                                        const char *signature,
                                                        SsrnxDexStaticValue *out_value,
                                                        bool *out_class_found,
                                                        bool *out_field_found) {
    if (!data || !class_name || !field_name || !signature || !out_value ||
        !out_class_found || !out_field_found || size < 0x70u || memcmp(data, "dex\n", 4) != 0)
        return SSRNX_ERR_APK_INVALID;
    memset(out_value, 0, sizeof(*out_value));
    out_value->type = SSRNX_DEX_STATIC_DEFAULT;
    *out_class_found = false;
    *out_field_found = false;

    char wanted[512];
    const int wn = snprintf(wanted, sizeof(wanted), "L%s;", class_name);
    if (wn <= 0 || (size_t)wn >= sizeof(wanted)) return SSRNX_ERR_ARGUMENT;
    const uint32_t class_count = package_rd32_mem(data + 0x60);
    const uint32_t class_off = package_rd32_mem(data + 0x64);
    if ((uint64_t)class_off + (uint64_t)class_count * 32u > size) return SSRNX_ERR_APK_INVALID;

    uint32_t scan_begin = 0, scan_end = class_count;
    package_dex_class_scan_range(data, size, wanted, class_off, class_count, &scan_begin, &scan_end);
    for (uint32_t i = scan_begin; i < scan_end; ++i) {
        const uint8_t *def = data + class_off + (size_t)i * 32u;
        const char *descriptor = package_dex_type_descriptor(data, size, package_rd32_mem(def));
        if (!descriptor) return SSRNX_ERR_APK_INVALID;
        if (strcmp(descriptor, wanted) != 0) continue;
        *out_class_found = true;
        const uint32_t class_data_off = package_rd32_mem(def + 24u);
        if (!class_data_off) return SSRNX_OK;
        if (class_data_off >= size) return SSRNX_ERR_APK_INVALID;
        size_t off = class_data_off;
        uint32_t static_fields = 0, instance_fields = 0, direct_methods = 0, virtual_methods = 0;
        if (!package_dex_read_uleb128_value(data, size, &off, &static_fields) ||
            !package_dex_read_uleb128_value(data, size, &off, &instance_fields) ||
            !package_dex_read_uleb128_value(data, size, &off, &direct_methods) ||
            !package_dex_read_uleb128_value(data, size, &off, &virtual_methods)) return SSRNX_ERR_APK_INVALID;
        (void)direct_methods;
        (void)virtual_methods;

        uint32_t field_idx = 0;
        uint32_t target_ordinal = UINT32_MAX;
        for (uint32_t n = 0; n < static_fields; ++n) {
            uint32_t diff = 0, access = 0;
            if (!package_dex_read_uleb128_value(data, size, &off, &diff) ||
                !package_dex_read_uleb128_value(data, size, &off, &access)) return SSRNX_ERR_APK_INVALID;
            (void)access;
            if (UINT32_MAX - field_idx < diff) return SSRNX_ERR_APK_INVALID;
            field_idx += diff;
            char cls[512], name[256], sig[SSRNX_DEX_SIGNATURE_CAP];
            if (!package_dex_field_identity(data, size, field_idx, cls, sizeof(cls), name, sizeof(name),
                                    sig, sizeof(sig))) return SSRNX_ERR_APK_INVALID;
            if (strcmp(cls, class_name) == 0 && strcmp(name, field_name) == 0 &&
                strcmp(sig, signature) == 0) target_ordinal = n;
        }
        /* Validate/skip the instance-field entries so malformed class_data is
         * still rejected consistently even though they are not part of the
         * static-value ordering. */
        uint32_t instance_idx = 0;
        for (uint32_t n = 0; n < instance_fields; ++n) {
            uint32_t diff = 0, access = 0;
            if (!package_dex_read_uleb128_value(data, size, &off, &diff) ||
                !package_dex_read_uleb128_value(data, size, &off, &access)) return SSRNX_ERR_APK_INVALID;
            (void)access;
            if (UINT32_MAX - instance_idx < diff) return SSRNX_ERR_APK_INVALID;
            instance_idx += diff;
        }
        if (target_ordinal == UINT32_MAX) return SSRNX_OK;
        *out_field_found = true;

        const uint32_t values_off = package_rd32_mem(def + 28u);
        if (!values_off) return SSRNX_OK;
        if (values_off >= size) return SSRNX_ERR_APK_INVALID;
        size_t value_off = values_off;
        uint32_t value_count = 0;
        if (!package_dex_read_uleb128_value(data, size, &value_off, &value_count) ||
            value_count > static_fields) return SSRNX_ERR_APK_INVALID;
        if (target_ordinal >= value_count) return SSRNX_OK;
        for (uint32_t n = 0; n < target_ordinal; ++n)
            if (!package_dex_skip_encoded_value_depth(data, size, &value_off, 0u)) return SSRNX_ERR_APK_INVALID;
        if (!package_dex_decode_static_value(data, size, &value_off, out_value)) return SSRNX_ERR_APK_INVALID;
        return SSRNX_OK;
    }
    return SSRNX_OK;
}

static SsrnxResult package_dex_load_method_from_data(uint8_t *data, size_t size,
                                             const char *class_name,
                                             const char *method_name,
                                             const char *signature,
                                             SsrnxDexMethodImage *out,
                                             bool *out_class_found,
                                             bool *out_method_found) {
    if (!data || !class_name || !method_name || !signature || !out ||
        !out_class_found || !out_method_found || size < 0x70u || memcmp(data, "dex\n", 4) != 0)
        return SSRNX_ERR_APK_INVALID;
    *out_class_found = false;
    *out_method_found = false;

    char wanted[512];
    const int wn = snprintf(wanted, sizeof(wanted), "L%s;", class_name);
    if (wn <= 0 || (size_t)wn >= sizeof(wanted)) return SSRNX_ERR_ARGUMENT;
    const uint32_t class_count = package_rd32_mem(data + 0x60);
    const uint32_t class_off = package_rd32_mem(data + 0x64);
    if ((uint64_t)class_off + (uint64_t)class_count * 32u > size) return SSRNX_ERR_APK_INVALID;

    uint32_t scan_begin = 0, scan_end = class_count;
    package_dex_class_scan_range(data, size, wanted, class_off, class_count, &scan_begin, &scan_end);
    for (uint32_t i = scan_begin; i < scan_end; ++i) {
        const uint8_t *def = data + class_off + (size_t)i * 32u;
        const uint32_t class_idx = package_rd32_mem(def);
        const char *descriptor = package_dex_type_descriptor(data, size, class_idx);
        if (!descriptor) return SSRNX_ERR_APK_INVALID;
        if (strcmp(descriptor, wanted) != 0) continue;
        *out_class_found = true;

        (void)package_copy_text(out->declaring_class, sizeof(out->declaring_class), class_name);
        out->class_access_flags = package_rd32_mem(def + 4u);
        const uint32_t super_idx = package_rd32_mem(def + 8u);
        if (super_idx != UINT32_MAX) {
            const char *super_desc = package_dex_type_descriptor(data, size, super_idx);
            if (!super_desc) return SSRNX_ERR_APK_INVALID;
            (void)package_dex_descriptor_to_jni_name(super_desc, out->super_class, sizeof(out->super_class));
        }

        const uint32_t class_data_off = package_rd32_mem(def + 24u);
        if (!class_data_off) return SSRNX_OK;
        if (class_data_off >= size) return SSRNX_ERR_APK_INVALID;
        size_t off = class_data_off;
        uint32_t static_fields = 0, instance_fields = 0, direct_methods = 0, virtual_methods = 0;
        if (!package_dex_read_uleb128_value(data, size, &off, &static_fields) ||
            !package_dex_read_uleb128_value(data, size, &off, &instance_fields) ||
            !package_dex_read_uleb128_value(data, size, &off, &direct_methods) ||
            !package_dex_read_uleb128_value(data, size, &off, &virtual_methods)) return SSRNX_ERR_APK_INVALID;
        uint32_t ignored = 0;
        for (uint64_t n = 0; n < (uint64_t)static_fields + instance_fields; ++n) {
            if (!package_dex_read_uleb128_value(data, size, &off, &ignored) ||
                !package_dex_read_uleb128_value(data, size, &off, &ignored)) return SSRNX_ERR_APK_INVALID;
        }

        uint32_t method_idx = 0;
        for (uint64_t group = 0; group < 2; ++group) {
            const uint32_t count = group == 0 ? direct_methods : virtual_methods;
            method_idx = 0;
            for (uint32_t n = 0; n < count; ++n) {
                uint32_t diff = 0, access = 0, code_off = 0;
                if (!package_dex_read_uleb128_value(data, size, &off, &diff) ||
                    !package_dex_read_uleb128_value(data, size, &off, &access) ||
                    !package_dex_read_uleb128_value(data, size, &off, &code_off)) return SSRNX_ERR_APK_INVALID;
                if (UINT32_MAX - method_idx < diff) return SSRNX_ERR_APK_INVALID;
                method_idx += diff;
                /* Compare the name in place first; only a name match pays for
                 * building the proto descriptor. */
                const char *ref_name = package_dex_method_name_ref(data, size, method_idx);
                if (!ref_name) return SSRNX_ERR_APK_INVALID;
                if (strcmp(ref_name, method_name) != 0) continue;
                char cls[512], name[256], sig[SSRNX_DEX_SIGNATURE_CAP];
                if (!package_dex_method_identity(data, size, method_idx, cls, sizeof(cls), name, sizeof(name),
                                         sig, sizeof(sig))) return SSRNX_ERR_APK_INVALID;
                if (strcmp(sig, signature) != 0) continue;
                out->method_index = method_idx;
                out->access_flags = access;
                out->code_offset = code_off;
                (void)package_copy_text(out->method_name, sizeof(out->method_name), name);
                (void)package_copy_text(out->signature, sizeof(out->signature), sig);
                if (code_off) {
                    if ((uint64_t)code_off + 16u > size) return SSRNX_ERR_APK_INVALID;
                    const uint8_t *code = data + code_off;
                    out->registers_size = (uint16_t)(code[0] | ((uint16_t)code[1] << 8));
                    out->ins_size = (uint16_t)(code[2] | ((uint16_t)code[3] << 8));
                    out->outs_size = (uint16_t)(code[4] | ((uint16_t)code[5] << 8));
                    out->insns_size = package_rd32_mem(code + 12u);
                    if ((uint64_t)code_off + 16u + (uint64_t)out->insns_size * 2u > size)
                        return SSRNX_ERR_APK_INVALID;
                    out->insns = (const uint16_t *)(const void *)(code + 16u);
                }
                *out_method_found = true;
                return SSRNX_OK;
            }
        }
        return SSRNX_OK;
    }
    return SSRNX_OK;
}

_Static_assert(SSRNX_APK_JAVA_BOOTSTRAP_MAX >= SSRNX_PACKAGE_MAX_ARM64_NAMES,
               "every packaged arm64 library can be a Java bootstrap candidate");

static bool package_append_java_bootstrap_lib(SsrnxApkInfo *out, const char *short_name) {
    if (!out || !short_name || !short_name[0]) return false;
    size_t used = 0;
    for (size_t i = 0; i < out->java_bootstrap_lib_count; ++i) {
        const char *name = ssrnx_apk_java_bootstrap_lib(out, i);
        if (!name) return false;
        if (strcmp(name, short_name) == 0) return true;
        used = (size_t)(name - out->java_bootstrap_libs) + strlen(name) + 1u;
    }
    const size_t len = strlen(short_name) + 1u;
    if (out->java_bootstrap_lib_count >= SSRNX_APK_JAVA_BOOTSTRAP_MAX ||
        len > sizeof(out->java_bootstrap_libs) - used) return false;
    memcpy(out->java_bootstrap_libs + used, short_name, len);
    out->java_bootstrap_lib_count++;
    return true;
}

static SsrnxResult package_scan_dex_load_library_strings(const uint8_t *data, size_t size,
                                                  const package_JavaNativeNames *names,
                                                  SsrnxApkInfo *out,
                                                  const char *apk_path,
                                                  const char *dex_name) {
    if (!data || !names || !out || size < 0x70u || memcmp(data, "dex\n", 4) != 0) {
        return SSRNX_ERR_APK_INVALID;
    }
    const uint32_t strings_size = package_rd32_mem(data + 0x38);
    const uint32_t strings_off = package_rd32_mem(data + 0x3c);
    if ((uint64_t)strings_off + (uint64_t)strings_size * 4u > size) return SSRNX_ERR_APK_INVALID;

    bool has_loader_class = false;
    bool has_load_library = false;
    bool hits[SSRNX_PACKAGE_MAX_ARM64_NAMES] = {0};
    for (uint32_t i = 0; i < strings_size; ++i) {
        uint32_t data_off = package_rd32_mem(data + strings_off + (size_t)i * 4u);
        if (package_dex_string_equals(data, size, data_off, "Ljava/lang/System;") ||
            package_dex_string_equals(data, size, data_off, "java/lang/System") ||
            package_dex_string_equals(data, size, data_off, "Ljava/lang/Runtime;") ||
            package_dex_string_equals(data, size, data_off, "java/lang/Runtime")) {
            has_loader_class = true;
        }
        if (package_dex_string_equals(data, size, data_off, "loadLibrary")) has_load_library = true;
        for (size_t k = 0; k < names->count; ++k) {
            if (hits[k]) continue;
            const char *short_name = names->short_names[k];
            char full_name[SSRNX_APK_LIBNAME_CAP + 8u];
            int full_n = snprintf(full_name, sizeof(full_name), "lib%s.so", short_name);
            if (package_dex_string_equals(data, size, data_off, short_name) ||
                (full_n > 0 && (size_t)full_n < sizeof(full_name) &&
                 package_dex_string_equals(data, size, data_off, full_name))) {
                hits[k] = true;
            }
        }
    }

    out->java_dex_count++;
    if (!has_loader_class || !has_load_library) return SSRNX_OK;
    for (size_t k = 0; k < names->count; ++k) {
        if (!hits[k]) continue;
        if (!package_append_java_bootstrap_lib(out, names->short_names[k])) {
            SSRNX_LOGCW("JAVA", "bootstrap library list overflow apk=%s dex=%s", apk_path, dex_name);
            return SSRNX_ERR_APK_UNSUPPORTED;
        }
        SSRNX_LOGCI("JAVA", "DEX loadLibrary candidate apk=%s dex=%s library=%s",
                    apk_path, dex_name, names->short_names[k]);
    }
    return SSRNX_OK;
}

typedef struct package_JavaDexScanCtx {
    const char *apk_path;
    const package_JavaNativeNames *names;
    SsrnxApkInfo *out;
    SsrnxResult rc;
} package_JavaDexScanCtx;

static bool package_java_dex_visitor(const SsrnxZipEntryInfo *entry, void *userdata) {
    package_JavaDexScanCtx *ctx = (package_JavaDexScanCtx *)userdata;
    if (!entry || !ctx) return false;
    const char *base = strrchr(entry->name, '/');
    base = base ? base + 1 : entry->name;
    if (strncmp(base, "classes", 7) != 0 || !package_ends_with_ci(base, ".dex")) return true;

    uint8_t *data = NULL;
    size_t size = 0;
    SsrnxResult rc = ssrnx_zip_read_entry_alloc(ctx->apk_path, entry->name, &data, &size);
    if (rc == SSRNX_OK) {
        rc = package_scan_dex_load_library_strings(data, size, ctx->names, ctx->out, ctx->apk_path, entry->name);
    }
    free(data);
    if (rc != SSRNX_OK) {
        ctx->rc = rc;
        return false;
    }
    return true;
}

static SsrnxResult package_analyze_java_bootstrap(const package_PackagePlan *plan, SsrnxApkInfo *out) {
    if (!plan || !out) return SSRNX_ERR_ARGUMENT;
    out->java_activity_bootstrap = false;
    out->java_dex_count = 0;
    out->java_bootstrap_lib_count = 0;
    memset(out->java_bootstrap_libs, 0, sizeof(out->java_bootstrap_libs));
    if (out->native_activity || out->arm64_lib_count == 0) return SSRNX_OK;

    package_JavaNativeNames names = {0};
    package_JavaNativeNameCtx name_ctx = { .names = &names };
    for (size_t i = 0; i < plan->component_count; ++i) {
        const package_PackageComponent *c = &plan->components[i];
        if (!c->selected || c->info.arm64_lib_count == 0) continue;
        SsrnxResult rc = ssrnx_zip_visit(c->apk_path, package_java_native_name_visitor, &name_ctx);
        if (rc != SSRNX_OK) return rc;
        if (names.overflow) return SSRNX_ERR_APK_UNSUPPORTED;
    }

    /* Java loaders may obtain their library name from ApplicationInfo.metaData
     * instead of a DEX string literal (the android.app.lib_name contract).
     * Keep candidates limited to libraries actually packaged for this ABI;
     * preparation does not run constructors or replace System.loadLibrary. */
    SsrnxMetaData *metadata = malloc(SSRNX_APK_META_DATA_MAX * sizeof(*metadata));
    if (!metadata) return SSRNX_ERR_NO_MEMORY;
    size_t metadata_count = 0;
    SsrnxResult metadata_rc = ssrnx_apk_read_application_meta_data(
        plan->components[plan->base_index].apk_path, metadata,
        SSRNX_APK_META_DATA_MAX, &metadata_count);
    for (size_t i = 0; metadata_rc == SSRNX_OK && i < metadata_count; ++i) {
        const SsrnxMetaData *md = &metadata[i];
        if (md->type != SSRNX_META_DATA_STRING || strcmp(md->name, "android.app.lib_name") != 0 ||
            !package_java_native_name_exists(&names, md->string_value)) continue;
        if (!package_append_java_bootstrap_lib(out, md->string_value)) {
            metadata_rc = SSRNX_ERR_APK_UNSUPPORTED;
            break;
        }
        SSRNX_LOGCI("JAVA", "manifest loadLibrary candidate apk=%s library=%s",
                    plan->components[plan->base_index].apk_path, md->string_value);
    }
    free(metadata);
    if (metadata_rc != SSRNX_OK) return metadata_rc;

    for (size_t i = 0; i < plan->component_count; ++i) {
        const package_PackageComponent *c = &plan->components[i];
        if (!c->selected) continue;
        package_JavaDexScanCtx dex_ctx = { .apk_path = c->apk_path, .names = &names, .out = out, .rc = SSRNX_OK };
        SsrnxResult rc = ssrnx_zip_visit(c->apk_path, package_java_dex_visitor, &dex_ctx);
        if (rc != SSRNX_OK) return rc;
        if (dex_ctx.rc != SSRNX_OK) return dex_ctx.rc;
    }
    out->java_activity_bootstrap = out->java_dex_count > 0 && out->java_bootstrap_lib_count > 0;
    return SSRNX_OK;
}

typedef struct package_DexClassLookupCtx {
    const char *apk_path;
    const char *class_name;
    char *out_super_name;
    size_t out_super_name_cap;
    char *out_interfaces;
    size_t out_interfaces_cap;
    bool found;
    SsrnxResult rc;
    char dex_name[SSRNX_APK_ENTRY_CAP];
} package_DexClassLookupCtx;

static bool package_dex_class_lookup_visitor(const SsrnxZipEntryInfo *entry, void *userdata) {
    package_DexClassLookupCtx *ctx = (package_DexClassLookupCtx *)userdata;
    if (!entry || !ctx) return false;
    const char *base = strrchr(entry->name, '/');
    base = base ? base + 1 : entry->name;
    if (strncmp(base, "classes", 7) != 0 || !package_ends_with_ci(base, ".dex")) return true;

    uint8_t *data = NULL;
    size_t size = 0;
    SsrnxResult rc = ssrnx_zip_read_entry_alloc(ctx->apk_path, entry->name, &data, &size);
    if (rc == SSRNX_OK) {
        rc = package_dex_find_defined_class(data, size, ctx->class_name, ctx->out_super_name,
                                    ctx->out_super_name_cap, ctx->out_interfaces,
                                    ctx->out_interfaces_cap, &ctx->found);
    }
    free(data);
    if (rc != SSRNX_OK) { ctx->rc = rc; return false; }
    if (ctx->found) {
        (void)package_copy_text(ctx->dex_name, sizeof(ctx->dex_name), entry->name);
        return false;
    }
    return true;
}

static SsrnxResult package_find_dex_class_in_component(package_PackageComponent *component, const char *class_name,
                                                char *out_super_name, size_t out_super_name_cap,
                                                char *out_interfaces, size_t out_interfaces_cap,
                                                bool *out_found, char *out_dex_name,
                                                size_t out_dex_name_cap) {
    if (!component || !class_name || !out_found) return SSRNX_ERR_ARGUMENT;
    *out_found = false;
    package_DexClassLookupCtx ctx = {
        .apk_path = component->apk_path, .class_name = class_name,
        .out_super_name = out_super_name, .out_super_name_cap = out_super_name_cap,
        .out_interfaces = out_interfaces, .out_interfaces_cap = out_interfaces_cap,
        .found = false, .rc = SSRNX_OK,
    };
    SsrnxResult rc = ssrnx_zip_visit(component->apk_path, package_dex_class_lookup_visitor, &ctx);
    if (rc != SSRNX_OK) return rc;
    if (ctx.rc != SSRNX_OK) return ctx.rc;
    *out_found = ctx.found;
    if (ctx.found && out_dex_name && out_dex_name_cap)
        (void)package_copy_text(out_dex_name, out_dex_name_cap, ctx.dex_name);
    return SSRNX_OK;
}

SsrnxResult ssrnx_package_find_dex_class(const char *source_path, const char *class_name,
                                         char *out_super_name, size_t out_super_name_cap,
                                         bool *out_found) {
    return ssrnx_package_find_dex_class_ex(source_path, class_name, out_super_name,
                                           out_super_name_cap, NULL, 0, out_found);
}

static SsrnxResult package_package_find_dex_class_app(const char *source_path, const char *class_name,
                                              char *out_super_name, size_t out_super_name_cap,
                                              char *out_interfaces, size_t out_interfaces_cap,
                                              bool *out_found) {
    if (!source_path || !class_name || !class_name[0] || !out_found) return SSRNX_ERR_ARGUMENT;
    *out_found = false;
    if (out_super_name && out_super_name_cap) out_super_name[0] = '\0';
    if (out_interfaces && out_interfaces_cap) out_interfaces[0] = '\0';

    package_DexIndexCandidate cands[DEX_INDEX_CANDIDATES];
    size_t cand_count = 0;
    bool index_ready = false;
    package_PackagePlan *index_plan = NULL;
    SsrnxResult rc = package_dex_index_collect(source_path, class_name, cands, &cand_count,
                                       &index_plan, &index_ready);
    if (index_ready) {
        if (rc != SSRNX_OK) { free(index_plan); return rc; }
        for (size_t i = 0; i < cand_count; ++i) {
            package_DexIndexCandidate *c = &cands[i];
            bool found = false;
            rc = package_dex_find_defined_class(c->data, c->size, class_name, out_super_name, out_super_name_cap,
                                        out_interfaces, out_interfaces_cap, &found);
            if (rc != SSRNX_OK || found) {
                if (rc == SSRNX_OK) {
                    *out_found = true;
                    if (ssrnx_diag_full_trace_enabled()) {
                        char apk_path[SSRNX_APK_PATH_CAP];
                        package_dex_index_component_path(source_path, index_plan, c->loc.component_index,
                                                 apk_path, sizeof(apk_path));
                        SSRNX_LOGCI("JAVA", "DEX class resolved(index) class=%s super=%s apk=%s dex=%s",
                                    class_name,
                                    out_super_name && out_super_name[0] ? out_super_name : "<unknown>",
                                    apk_path[0] ? apk_path : "<cached>",
                                    c->dex_name[0] ? c->dex_name : "<cached>");
                    }
                }
                package_dex_index_release_candidates(cands, i, cand_count);
                free(index_plan);
                return rc;
            }
            package_dex_blob_release(c->owner, c->data);
        }
        free(index_plan);
        return SSRNX_OK;
    }
    free(index_plan);

    /* Cache preparation is an optimization. Fall back to the original ZIP
       scan so a cache I/O failure cannot make a valid APK unlaunchable. */
    package_PackagePlan *plan = (package_PackagePlan *)malloc(sizeof(*plan));
    if (!plan) return SSRNX_ERR_NO_MEMORY;
    rc = package_discover_plan(source_path, plan);
    if (rc != SSRNX_OK) { free(plan); return rc; }
    for (size_t pass = 0; pass < 2 && !*out_found; ++pass) {
        for (size_t i = 0; i < plan->component_count; ++i) {
            if (!plan->components[i].selected) continue;
            if ((pass == 0 && i != plan->base_index) || (pass == 1 && i == plan->base_index)) continue;
            rc = package_ensure_component_materialized(plan, i);
            if (rc != SSRNX_OK) { free(plan); return rc; }
            char dex_name[SSRNX_APK_ENTRY_CAP] = {0};
            rc = package_find_dex_class_in_component(&plan->components[i], class_name, out_super_name,
                                             out_super_name_cap, out_interfaces, out_interfaces_cap,
                                             out_found, dex_name, sizeof(dex_name));
            if (rc != SSRNX_OK) { free(plan); return rc; }
            if (*out_found) {
                SSRNX_LOGCI("JAVA", "DEX class resolved(fallback) class=%s super=%s apk=%s dex=%s",
                            class_name,
                            out_super_name && out_super_name[0] ? out_super_name : "<unknown>",
                            plan->components[i].apk_path, dex_name[0] ? dex_name : "<unknown>");
                break;
            }
        }
    }
    free(plan);
    return SSRNX_OK;
}


typedef struct package_DexStaticFieldLookupCtx {
    const char *apk_path;
    const char *class_name;
    const char *field_name;
    const char *signature;
    SsrnxDexStaticValue *out_value;
    bool class_found;
    bool field_found;
    SsrnxResult rc;
    char dex_name[SSRNX_APK_ENTRY_CAP];
} package_DexStaticFieldLookupCtx;

static bool package_dex_static_field_lookup_visitor(const SsrnxZipEntryInfo *entry, void *userdata) {
    package_DexStaticFieldLookupCtx *ctx = (package_DexStaticFieldLookupCtx *)userdata;
    if (!entry || !ctx) return false;
    const char *base = strrchr(entry->name, '/');
    base = base ? base + 1 : entry->name;
    if (strncmp(base, "classes", 7) != 0 || !package_ends_with_ci(base, ".dex")) return true;

    uint8_t *data = NULL;
    size_t size = 0;
    SsrnxResult rc = ssrnx_zip_read_entry_alloc(ctx->apk_path, entry->name, &data, &size);
    if (rc == SSRNX_OK) {
        rc = package_dex_find_static_field_initial_value(data, size, ctx->class_name, ctx->field_name,
                                                 ctx->signature, ctx->out_value,
                                                 &ctx->class_found, &ctx->field_found);
    }
    free(data);
    if (rc != SSRNX_OK) { ctx->rc = rc; return false; }
    if (ctx->class_found) {
        (void)package_copy_text(ctx->dex_name, sizeof(ctx->dex_name), entry->name);
        return false;
    }
    return true;
}

static SsrnxResult package_package_static_field_app(const char *source_path,
                                            const char *class_name,
                                            const char *field_name,
                                            const char *signature,
                                            SsrnxDexStaticValue *out_value,
                                            bool *out_class_found,
                                            bool *out_field_found) {
    if (!source_path || !class_name || !field_name || !signature || !out_value ||
        !out_class_found || !out_field_found) return SSRNX_ERR_ARGUMENT;
    memset(out_value, 0, sizeof(*out_value));
    out_value->type = SSRNX_DEX_STATIC_DEFAULT;
    *out_class_found = false;
    *out_field_found = false;

    SSRNX_PERF_LOCK(&package_g_dex_index_lock);
    SsrnxResult rc = package_prepare_dex_cache_locked(source_path);
    if (rc == SSRNX_OK) {
        package_PackagePlan *plan = NULL;
        const uint64_t hash = package_dex_class_hash64(class_name);
        size_t pos = 0, next = 0;
        package_DexIndexEntry loc;
        while (package_dex_index_find(hash, pos, &loc, &next)) {
            pos = next;
            uint8_t *data = NULL;
            size_t size = 0;
            void *owner = NULL;
            char dex_name[SSRNX_APK_ENTRY_CAP] = {0};
            rc = package_dex_index_acquire(source_path, &loc, &plan, &data, &size, &owner,
                                   dex_name, sizeof(dex_name));
            if (rc != SSRNX_OK) { free(plan); pthread_mutex_unlock(&package_g_dex_index_lock); return rc; }
            bool class_found = false, field_found = false;
            SsrnxDexStaticValue value = {0};
            rc = package_dex_find_static_field_initial_value(data, size, class_name, field_name, signature,
                                                     &value, &class_found, &field_found);
            package_dex_blob_release(owner, data);
            if (rc != SSRNX_OK) { free(plan); pthread_mutex_unlock(&package_g_dex_index_lock); return rc; }
            if (!class_found) continue; /* class-hash collision */
            *out_class_found = true;
            *out_field_found = field_found;
            if (field_found) *out_value = value;
            SSRNX_LOGCI("JAVA", "DEX static field resolved(index) class=%s name=%s sig=%s type=%d dex=%s",
                        class_name, field_name, signature, (int)out_value->type,
                        dex_name[0] ? dex_name : "<cached>");
            free(plan);
            pthread_mutex_unlock(&package_g_dex_index_lock);
            return SSRNX_OK;
        }
        free(plan);
        pthread_mutex_unlock(&package_g_dex_index_lock);
        return SSRNX_OK;
    }
    pthread_mutex_unlock(&package_g_dex_index_lock);

    /* Cache setup is only an optimization. Mirror the class/method lookup
     * fallback so encoded static constants remain correct on read-only or
     * otherwise cache-constrained environments. */
    package_PackagePlan *plan = malloc(sizeof(*plan));
    if (!plan) return SSRNX_ERR_NO_MEMORY;
    rc = package_discover_plan(source_path, plan);
    if (rc != SSRNX_OK) { free(plan); return rc; }
    for (size_t pass = 0; pass < 2 && !*out_class_found; ++pass) {
        for (size_t i = 0; i < plan->component_count; ++i) {
            if (!plan->components[i].selected) continue;
            if ((pass == 0 && i != plan->base_index) || (pass == 1 && i == plan->base_index)) continue;
            rc = package_ensure_component_materialized(plan, i);
            if (rc != SSRNX_OK) { free(plan); return rc; }
            SsrnxDexStaticValue value = {0};
            package_DexStaticFieldLookupCtx ctx = {
                .apk_path = plan->components[i].apk_path,
                .class_name = class_name,
                .field_name = field_name,
                .signature = signature,
                .out_value = &value,
                .rc = SSRNX_OK,
            };
            rc = ssrnx_zip_visit(plan->components[i].apk_path, package_dex_static_field_lookup_visitor, &ctx);
            if (rc == SSRNX_OK) rc = ctx.rc;
            if (rc != SSRNX_OK) { free(plan); return rc; }
            if (ctx.class_found) {
                *out_class_found = true;
                *out_field_found = ctx.field_found;
                if (ctx.field_found) *out_value = value;
                SSRNX_LOGCI("JAVA", "DEX static field resolved(fallback) class=%s name=%s sig=%s type=%d dex=%s",
                            class_name, field_name, signature, (int)out_value->type,
                            ctx.dex_name[0] ? ctx.dex_name : "<unknown>");
                break;
            }
        }
    }
    free(plan);
    return SSRNX_OK;
}

typedef struct package_DexMethodLookupCtx {
    const char *apk_path;
    const char *class_name;
    const char *method_name;
    const char *signature;
    SsrnxDexMethodImage *out;
    bool class_found;
    bool method_found;
    SsrnxResult rc;
    char dex_name[SSRNX_APK_ENTRY_CAP];
} package_DexMethodLookupCtx;

static bool package_dex_method_lookup_visitor(const SsrnxZipEntryInfo *entry, void *userdata) {
    package_DexMethodLookupCtx *ctx = (package_DexMethodLookupCtx *)userdata;
    if (!entry || !ctx) return false;
    const char *base = strrchr(entry->name, '/');
    base = base ? base + 1 : entry->name;
    if (strncmp(base, "classes", 7) != 0 || !package_ends_with_ci(base, ".dex")) return true;

    uint8_t *data = NULL;
    size_t size = 0;
    SsrnxResult rc = ssrnx_zip_read_entry_alloc(ctx->apk_path, entry->name, &data, &size);
    if (rc != SSRNX_OK) { ctx->rc = rc; return false; }

    SsrnxDexMethodImage candidate = {0};
    bool class_found = false, method_found = false;
    rc = package_dex_load_method_from_data(data, size, ctx->class_name, ctx->method_name,
                                   ctx->signature, &candidate, &class_found, &method_found);
    if (rc != SSRNX_OK) {
        free(data);
        ctx->rc = rc;
        return false;
    }
    if (!class_found) {
        free(data);
        return true;
    }
    ctx->class_found = true;
    if (!method_found) {
        if (candidate.super_class[0] && !ctx->out->super_class[0])
            (void)package_copy_text(ctx->out->super_class, sizeof(ctx->out->super_class), candidate.super_class);
        free(data);
        return false;
    }

    candidate.dex_data = data;
    candidate.dex_size = size;
    (void)package_copy_text(candidate.apk_path, sizeof(candidate.apk_path), ctx->apk_path);
    (void)package_copy_text(candidate.dex_name, sizeof(candidate.dex_name), entry->name);
    *ctx->out = candidate;
    ctx->method_found = true;
    (void)package_copy_text(ctx->dex_name, sizeof(ctx->dex_name), entry->name);
    return false;
}

static SsrnxResult package_package_load_method_app(const char *source_path,
                                          const char *class_name,
                                          const char *method_name,
                                          const char *signature,
                                          SsrnxDexMethodImage *out_method,
                                          bool *out_class_found,
                                          bool *out_method_found,
                                          package_DexIndexEntry *out_loc) {
    if (!source_path || !class_name || !method_name || !signature || !out_method ||
        !out_class_found || !out_method_found) return SSRNX_ERR_ARGUMENT;
    memset(out_method, 0, sizeof(*out_method));
    *out_class_found = false;
    *out_method_found = false;

    package_DexIndexCandidate cands[DEX_INDEX_CANDIDATES];
    size_t cand_count = 0;
    bool index_ready = false;
    package_PackagePlan *index_plan = NULL;
    SsrnxResult rc = package_dex_index_collect(source_path, class_name, cands, &cand_count, &index_plan, &index_ready);
    if (index_ready) {
        if (rc != SSRNX_OK) { free(index_plan); return rc; }
        for (size_t i = 0; i < cand_count; ++i) {
            uint8_t *data = cands[i].data; size_t size = cands[i].size; void *owner = cands[i].owner;
            const package_DexIndexEntry loc = cands[i].loc;
            const char *dex_name = cands[i].dex_name;
            SsrnxDexMethodImage candidate = {0};
            bool class_found = false, method_found = false;
            rc = package_dex_load_method_from_data(data, size, class_name, method_name, signature,
                                           &candidate, &class_found, &method_found);
            if (rc != SSRNX_OK) { package_dex_index_release_candidates(cands, i, cand_count); free(index_plan); return rc; }
            if (!class_found) { package_dex_blob_release(owner, data); continue; } /* hash collision */
            package_dex_index_release_candidates(cands, i + 1, cand_count);
            *out_class_found = true;
            if (out_loc) *out_loc = loc;
            *out_method_found = method_found;
            if (method_found) {
                candidate.dex_data = data;
                candidate.dex_size = size;
                candidate.dex_owner = owner;
                package_dex_index_component_path(source_path, index_plan, loc.component_index,
                                         candidate.apk_path, sizeof(candidate.apk_path));
                (void)package_copy_text(candidate.dex_name, sizeof(candidate.dex_name),
                                dex_name[0] ? dex_name : "<cached>");
                *out_method = candidate;
                if (ssrnx_diag_full_trace_enabled())
                    SSRNX_LOGCI("JAVA", "DEX method resolved(index) class=%s name=%s sig=%s apk=%s dex=%s code=%#x",
                            class_name, method_name, signature, out_method->apk_path,
                            out_method->dex_name, out_method->code_offset);
            } else {
                if (candidate.super_class[0])
                    (void)package_copy_text(out_method->super_class, sizeof(out_method->super_class), candidate.super_class);
                package_dex_blob_release(owner, data);
            }
            free(index_plan);
            return SSRNX_OK;
        }
        free(index_plan);
        return SSRNX_OK;
    }
    free(index_plan);

    /* Correctness fallback when cache storage is unavailable. */
    package_PackagePlan *plan = (package_PackagePlan *)malloc(sizeof(*plan));
    if (!plan) return SSRNX_ERR_NO_MEMORY;
    rc = package_discover_plan(source_path, plan);
    if (rc != SSRNX_OK) { free(plan); return rc; }
    for (size_t pass = 0; pass < 2 && !*out_class_found; ++pass) {
        for (size_t i = 0; i < plan->component_count; ++i) {
            if (!plan->components[i].selected) continue;
            if ((pass == 0 && i != plan->base_index) || (pass == 1 && i == plan->base_index)) continue;
            rc = package_ensure_component_materialized(plan, i);
            if (rc != SSRNX_OK) { free(plan); return rc; }
            package_DexMethodLookupCtx ctx = {
                .apk_path = plan->components[i].apk_path,
                .class_name = class_name,
                .method_name = method_name,
                .signature = signature,
                .out = out_method,
                .rc = SSRNX_OK,
            };
            rc = ssrnx_zip_visit(plan->components[i].apk_path, package_dex_method_lookup_visitor, &ctx);
            if (rc == SSRNX_OK) rc = ctx.rc;
            if (rc != SSRNX_OK) { free(plan); return rc; }
            if (ctx.class_found) {
                *out_class_found = true;
                *out_method_found = ctx.method_found;
                if (ctx.method_found) {
                    SSRNX_LOGCI("JAVA", "DEX method resolved(fallback) class=%s name=%s sig=%s apk=%s dex=%s code=%#x",
                                class_name, method_name, signature, out_method->apk_path,
                                out_method->dex_name, out_method->code_offset);
                }
                break;
            }
        }
    }
    free(plan);
    return SSRNX_OK;
}

void ssrnx_package_release_dex_method(SsrnxDexMethodImage *method) {
    if (!method) return;
    package_dex_blob_release(method->dex_owner, method->dex_data);
    method->dex_owner = NULL;
    method->dex_data = NULL;
    method->dex_size = 0;
    method->insns = NULL;
    method->code_offset = 0;
}

void ssrnx_package_free_dex_members(SsrnxDexMember *members, size_t count) {
    if (!members) return;
    for (size_t i = 0; i < count; ++i) {
        free(members[i].name);
        free(members[i].signature);
    }
    free(members);
}

typedef struct package_DexMemberList {
    SsrnxDexMember *items;
    size_t count;
    size_t capacity;
} package_DexMemberList;

static bool package_dex_member_list_add(package_DexMemberList *list, bool is_method, uint32_t access,
                                const char *name, const char *signature) {
    if (list->count == list->capacity) {
        const size_t next = list->capacity ? list->capacity * 2u : 16u;
        SsrnxDexMember *grown = realloc(list->items, next * sizeof(*grown));
        if (!grown) return false;
        list->items = grown;
        list->capacity = next;
    }
    SsrnxDexMember *m = &list->items[list->count];
    memset(m, 0, sizeof(*m));
    m->is_method = is_method;
    m->is_static = (access & 0x0008u) != 0;
    m->access_flags = access;
    m->name = strdup(name);
    m->signature = strdup(signature);
    if (!m->name || !m->signature) { free(m->name); free(m->signature); return false; }
    list->count++;
    return true;
}

/* Reads class_data_item (DEX format: static fields, instance fields, direct
 * methods, virtual methods; member indices are delta-encoded per group). */
static SsrnxResult package_dex_list_members_from_data(const uint8_t *data, size_t size, const char *class_name,
                                              package_DexMemberList *list, bool *out_class_found) {
    if (!data || size < 0x70u || memcmp(data, "dex\n", 4) != 0) return SSRNX_ERR_APK_INVALID;
    char wanted[512];
    const int wn = snprintf(wanted, sizeof(wanted), "L%s;", class_name);
    if (wn <= 0 || (size_t)wn >= sizeof(wanted)) return SSRNX_ERR_ARGUMENT;
    const uint32_t class_count = package_rd32_mem(data + 0x60);
    const uint32_t class_off = package_rd32_mem(data + 0x64);
    if ((uint64_t)class_off + (uint64_t)class_count * 32u > size) return SSRNX_ERR_APK_INVALID;
    uint32_t scan_begin = 0, scan_end = class_count;
    package_dex_class_scan_range(data, size, wanted, class_off, class_count, &scan_begin, &scan_end);
    for (uint32_t i = scan_begin; i < scan_end; ++i) {
        const uint8_t *def = data + class_off + (size_t)i * 32u;
        const char *descriptor = package_dex_type_descriptor(data, size, package_rd32_mem(def));
        if (!descriptor) return SSRNX_ERR_APK_INVALID;
        if (strcmp(descriptor, wanted) != 0) continue;
        *out_class_found = true;
        const uint32_t class_data_off = package_rd32_mem(def + 24u);
        if (!class_data_off) return SSRNX_OK;
        if (class_data_off >= size) return SSRNX_ERR_APK_INVALID;
        size_t off = class_data_off;
        uint32_t counts[4] = {0};
        for (unsigned g = 0; g < 4u; ++g)
            if (!package_dex_read_uleb128_value(data, size, &off, &counts[g])) return SSRNX_ERR_APK_INVALID;
        for (unsigned g = 0; g < 4u; ++g) {
            const bool methods = g >= 2u;
            uint32_t index = 0;
            for (uint32_t n = 0; n < counts[g]; ++n) {
                uint32_t diff = 0, access = 0, code_off = 0;
                if (!package_dex_read_uleb128_value(data, size, &off, &diff) ||
                    !package_dex_read_uleb128_value(data, size, &off, &access) ||
                    (methods && !package_dex_read_uleb128_value(data, size, &off, &code_off)))
                    return SSRNX_ERR_APK_INVALID;
                if (UINT32_MAX - index < diff) return SSRNX_ERR_APK_INVALID;
                index += diff;
                char cls[512], name[256], sig[SSRNX_DEX_SIGNATURE_CAP];
                const bool ok = methods
                    ? package_dex_method_identity(data, size, index, cls, sizeof(cls), name, sizeof(name), sig, sizeof(sig))
                    : package_dex_field_identity(data, size, index, cls, sizeof(cls), name, sizeof(name), sig, sizeof(sig));
                if (!ok) return SSRNX_ERR_APK_INVALID;
                if (!package_dex_member_list_add(list, methods, access, name, sig)) return SSRNX_ERR_NO_MEMORY;
            }
        }
        return SSRNX_OK;
    }
    return SSRNX_OK;
}

static SsrnxResult package_package_list_members_app(const char *source_path, const char *class_name,
                                            SsrnxDexMember **out_members, size_t *out_count,
                                            bool *out_class_found) {
    if (!source_path || !class_name || !out_members || !out_count || !out_class_found)
        return SSRNX_ERR_ARGUMENT;
    *out_members = NULL;
    *out_count = 0;
    *out_class_found = false;
    package_DexMemberList list = {0};
    SSRNX_PERF_LOCK(&package_g_dex_index_lock);
    SsrnxResult rc = package_prepare_dex_cache_locked(source_path);
    package_PackagePlan *plan = NULL;
    if (rc == SSRNX_OK) {
        const uint64_t hash = package_dex_class_hash64(class_name);
        size_t pos = 0, next = 0;
        package_DexIndexEntry loc;
        while (!*out_class_found && package_dex_index_find(hash, pos, &loc, &next)) {
            pos = next;
            uint8_t *data = NULL; size_t size = 0; void *owner = NULL;
            rc = package_dex_index_acquire(source_path, &loc, &plan, &data, &size, &owner, NULL, 0);
            if (rc != SSRNX_OK) break;
            rc = package_dex_list_members_from_data(data, size, class_name, &list, out_class_found);
            package_dex_blob_release(owner, data);
            if (rc != SSRNX_OK) break;
        }
    }
    pthread_mutex_unlock(&package_g_dex_index_lock);
    free(plan);
    if (rc != SSRNX_OK) {
        ssrnx_package_free_dex_members(list.items, list.count);
        *out_class_found = false;
        return rc;
    }
    *out_members = list.items;
    *out_count = list.count;
    return SSRNX_OK;
}

/* Offset of class_name's class_def_item in one DEX image, or 0 when the
 * image does not define it (offset 0 is always the DEX header). */
static uint32_t package_dex_class_def_offset(const uint8_t *data, size_t size, const char *class_name) {
    if (!data || size < 0x70u || memcmp(data, "dex\n", 4) != 0) return 0;
    char wanted[512];
    const int wn = snprintf(wanted, sizeof(wanted), "L%s;", class_name);
    if (wn <= 0 || (size_t)wn >= sizeof(wanted)) return 0;
    const uint32_t class_count = package_rd32_mem(data + 0x60);
    const uint32_t class_off = package_rd32_mem(data + 0x64);
    if ((uint64_t)class_off + (uint64_t)class_count * 32u > size) return 0;
    uint32_t scan_begin = 0, scan_end = class_count;
    package_dex_class_scan_range(data, size, wanted, class_off, class_count, &scan_begin, &scan_end);
    for (uint32_t i = scan_begin; i < scan_end; ++i) {
        const uint32_t def = class_off + i * 32u;
        const char *descriptor = package_dex_type_descriptor(data, size, package_rd32_mem(data + def));
        if (descriptor && strcmp(descriptor, wanted) == 0) return def;
    }
    return 0;
}

static SsrnxResult package_package_acquire_class_app(const char *source_path, const char *class_name,
                                             SsrnxDexClassView *out_view, bool *out_class_found) {
    if (!source_path || !class_name || !out_view || !out_class_found) return SSRNX_ERR_ARGUMENT;
    memset(out_view, 0, sizeof(*out_view));
    *out_class_found = false;
    SSRNX_PERF_LOCK(&package_g_dex_index_lock);
    SsrnxResult rc = package_prepare_dex_cache_locked(source_path);
    if (rc == SSRNX_OK) {
        package_PackagePlan *plan = NULL;
        const uint64_t hash = package_dex_class_hash64(class_name);
        size_t pos = 0, next = 0;
        package_DexIndexEntry loc;
        while (package_dex_index_find(hash, pos, &loc, &next)) {
            pos = next;
            uint8_t *data = NULL; size_t size = 0; void *owner = NULL;
            rc = package_dex_index_acquire(source_path, &loc, &plan, &data, &size, &owner, NULL, 0);
            if (rc != SSRNX_OK) break;
            const uint32_t def = package_dex_class_def_offset(data, size, class_name);
            if (!def) { package_dex_blob_release(owner, data); continue; } /* hash collision */
            out_view->dex_data = data;
            out_view->dex_size = size;
            out_view->dex_owner = owner;
            out_view->class_def_offset = def;
            *out_class_found = true;
            break;
        }
        free(plan);
        pthread_mutex_unlock(&package_g_dex_index_lock);
        return rc;
    }
    pthread_mutex_unlock(&package_g_dex_index_lock);

    /* Cache-less fallback mirrors the method lookup: base APK first, then splits. */
    package_PackagePlan *plan = malloc(sizeof(*plan));
    if (!plan) return SSRNX_ERR_NO_MEMORY;
    rc = package_discover_plan(source_path, plan);
    if (rc != SSRNX_OK) { free(plan); return rc; }
    for (size_t pass = 0; pass < 2 && !*out_class_found && rc == SSRNX_OK; ++pass) {
        for (size_t i = 0; i < plan->component_count && !*out_class_found; ++i) {
            if (!plan->components[i].selected) continue;
            if ((pass == 0 && i != plan->base_index) || (pass == 1 && i == plan->base_index)) continue;
            rc = package_ensure_component_materialized(plan, i);
            if (rc != SSRNX_OK) break;
            /* Multidex order: classes.dex, classes2.dex, ... (first gap ends). */
            for (unsigned n = 1; n < 100u && !*out_class_found; ++n) {
                char entry[32];
                if (n == 1) snprintf(entry, sizeof(entry), "classes.dex");
                else snprintf(entry, sizeof(entry), "classes%u.dex", n);
                uint8_t *data = NULL; size_t size = 0;
                if (ssrnx_zip_read_entry_alloc(plan->components[i].apk_path, entry, &data, &size) != SSRNX_OK)
                    break;
                const uint32_t def = package_dex_class_def_offset(data, size, class_name);
                if (!def) { free(data); continue; }
                out_view->dex_data = data;
                out_view->dex_size = size;
                out_view->class_def_offset = def;
                *out_class_found = true;
            }
        }
    }
    free(plan);
    return rc;
}

void ssrnx_package_release_dex_class(SsrnxDexClassView *view) {
    if (!view || !view->dex_data) return;
    package_dex_blob_release(view->dex_owner, view->dex_data);
    memset(view, 0, sizeof(*view));
}

static SsrnxResult package_aggregate_info(const package_PackagePlan *plan, SsrnxApkInfo *out) {
    if (!plan || !out || plan->base_index >= plan->component_count) return SSRNX_ERR_ARGUMENT;
    const package_PackageComponent *base = &plan->components[plan->base_index];
    *out = base->info;
    if (!package_copy_text(out->apk_path, sizeof(out->apk_path), plan->source_path) ||
        !package_copy_text(out->base_apk_path, sizeof(out->base_apk_path), base->apk_path)) {
        return SSRNX_ERR_ARGUMENT;
    }
    out->source_xapk = plan->xapk;
    out->package_apk_count = plan->component_count;
    out->package_selected_apk_count = plan->selected_count;
    out->package_obb_count = plan->xapk ? plan->outer.obb_seen : 0;
    out->zip_entry_count = 0;
    out->native_lib_count = 0;
    out->arm64_lib_count = 0;
    out->native_library_present = false;
    out->engine_kind = SSRNX_ENGINE_UNKNOWN;
    out->unity_backend = SSRNX_UNITY_BACKEND_NONE;
    out->unity_player_present = false;
    out->unity_main_present = false;
    out->unity_il2cpp_present = false;
    out->unity_mono_present = false;
    out->sdl_present = false;
    out->godot_present = false;
    out->gamemaker_present = false;
    out->game_activity = base->info.game_activity;

    for (size_t i = 0; i < plan->component_count; ++i) {
        const package_PackageComponent *c = &plan->components[i];
        if (!c->selected) continue;
        out->zip_entry_count += c->info.zip_entry_count;
        out->native_lib_count += c->info.native_lib_count;
        out->arm64_lib_count += c->info.arm64_lib_count;
        if (c->info.unity_player_present) {
            out->unity_player_present = true;
            out->engine_kind = SSRNX_ENGINE_UNITY;
        }
        if (c->info.unity_main_present) out->unity_main_present = true;
        if (c->info.unity_il2cpp_present) out->unity_il2cpp_present = true;
        if (c->info.unity_mono_present) out->unity_mono_present = true;
        if (c->info.sdl_present) out->sdl_present = true;
        if (c->info.godot_present) out->godot_present = true;
        if (c->info.gamemaker_present) out->gamemaker_present = true;
        if (out->native_entry_path[0]) {
            bool present = false;
            SsrnxResult rc = ssrnx_zip_has_entry(c->apk_path, out->native_entry_path, &present);
            if (rc != SSRNX_OK) return rc;
            if (present) out->native_library_present = true;
        }
    }
    if (out->unity_player_present) {
        if (out->unity_il2cpp_present && out->unity_mono_present)
            out->unity_backend = SSRNX_UNITY_BACKEND_MIXED;
        else if (out->unity_il2cpp_present)
            out->unity_backend = SSRNX_UNITY_BACKEND_IL2CPP;
        else if (out->unity_mono_present)
            out->unity_backend = SSRNX_UNITY_BACKEND_MONO;
    } else {
        out->engine_kind = SSRNX_ENGINE_UNKNOWN;
        out->unity_backend = SSRNX_UNITY_BACKEND_NONE;
    }
    return package_analyze_java_bootstrap(plan, out);
}

static bool package_json_string_field(const uint8_t *data, size_t size, const char *key, char *out, size_t cap) {
    if (!data || !key || !out || cap == 0) return false;
    char pat[96]; int pn = snprintf(pat, sizeof(pat), "\"%s\"", key);
    if (pn <= 0 || (size_t)pn >= sizeof(pat)) return false;
    const char *p = (const char *)data, *end = p + size, *hit = NULL;
    for (const char *q = p; q + pn <= end; ++q) if (memcmp(q, pat, (size_t)pn) == 0) { hit = q + pn; break; }
    if (!hit) return false;
    while (hit < end && isspace((unsigned char)*hit)) ++hit;
    if (hit >= end || *hit++ != ':') return false;
    while (hit < end && isspace((unsigned char)*hit)) ++hit;
    if (hit >= end || *hit++ != '"') return false;
    size_t n = 0;
    while (hit < end && *hit != '"') {
        unsigned char c = (unsigned char)*hit++;
        if (c == '\\' && hit < end) {
            unsigned char e = (unsigned char)*hit++;
            if (e == '"' || e == '\\' || e == '/') c = e;
            else if (e == 'n') c = '\n'; else if (e == 'r') c = '\r'; else if (e == 't') c = '\t';
            else return false;
        }
        if (n + 1u >= cap) return false;
        out[n++] = (char)c;
    }
    if (hit >= end || *hit != '"') return false;
    out[n] = '\0'; return true;
}

static bool package_json_u32_field(const uint8_t *data, size_t size, const char *key, uint32_t *out) {
    char tmp[32];
    if (package_json_string_field(data, size, key, tmp, sizeof(tmp))) {
        char *e = NULL; unsigned long v = strtoul(tmp, &e, 10); if (e && *e == '\0' && v <= UINT32_MAX) { *out = (uint32_t)v; return true; }
    }
    char pat[96]; int pn = snprintf(pat, sizeof(pat), "\"%s\"", key); if (pn <= 0 || (size_t)pn >= sizeof(pat)) return false;
    const char *p=(const char *)data,*end=p+size,*hit=NULL; for(const char*q=p;q+pn<=end;++q)if(memcmp(q,pat,(size_t)pn)==0){hit=q+pn;break;} if(!hit)return false;
    while (hit < end && isspace((unsigned char)*hit)) ++hit;
    if (hit >= end || *hit++ != ':') return false;
    while (hit < end && isspace((unsigned char)*hit)) ++hit;
    unsigned long v = 0; bool any = false;
    while (hit < end && isdigit((unsigned char)*hit)) {
        any = true; v = v * 10u + (unsigned)(*hit - '0');
        if (v > UINT32_MAX) return false;
        ++hit;
    }
    if (!any) return false;
    *out = (uint32_t)v;
    return true;
}

static bool package_xapk_outer_display_metadata(const char *source_path, SsrnxApkInfo *out_info, char *out_icon, size_t icon_cap) {
    uint8_t *json = NULL; size_t json_size = 0;
    if (ssrnx_zip_read_entry_alloc(source_path, "manifest.json", &json, &json_size) != SSRNX_OK) return false;
    char package_name[SSRNX_APK_PACKAGE_CAP]={0}, name[128]={0}, version[128]={0}, icon[SSRNX_APK_ENTRY_CAP]={0};
    bool ok = package_json_string_field(json,json_size,"package_name",package_name,sizeof(package_name)) &&
              package_json_string_field(json,json_size,"name",name,sizeof(name)) &&
              package_json_string_field(json,json_size,"version_name",version,sizeof(version));
    uint32_t vc=0,min_sdk=0,target_sdk=0; (void)package_json_u32_field(json,json_size,"version_code",&vc); (void)package_json_u32_field(json,json_size,"min_sdk_version",&min_sdk); (void)package_json_u32_field(json,json_size,"target_sdk_version",&target_sdk);
    if (!package_json_string_field(json,json_size,"icon",icon,sizeof(icon))) (void)package_copy_text(icon,sizeof(icon),"icon.png");
    free(json);
    if (!ok) return false;
    memset(out_info,0,sizeof(*out_info));
    if (!package_copy_text(out_info->apk_path,sizeof(out_info->apk_path),source_path) || !package_copy_text(out_info->package_name,sizeof(out_info->package_name),package_name) ||
        !package_copy_text(out_info->application_label,sizeof(out_info->application_label),name) || !package_copy_text(out_info->version_name,sizeof(out_info->version_name),version)) return false;
    out_info->version_code=vc; out_info->min_sdk=min_sdk; out_info->target_sdk=target_sdk; out_info->source_xapk=true;
    if (out_icon && icon_cap) (void)package_copy_text(out_icon,icon_cap,icon);
    return true;
}

static SsrnxResult package_xapk_outer_icon(const char *source_path, const char *entry,
                                   uint32_t **out_pixels, int *out_width, int *out_height) {
    uint8_t *png=NULL; size_t size=0;
    SsrnxResult rc=ssrnx_zip_read_entry_alloc(source_path,entry&&entry[0]?entry:"icon.png",&png,&size);
    if(rc!=SSRNX_OK)return rc;
    bool ok=ssrnx_png_decode_rgba(png,size,out_pixels,out_width,out_height); free(png);
    return ok?SSRNX_OK:SSRNX_ERR_APK_INVALID;
}



SsrnxResult ssrnx_package_probe(const char *source_path, SsrnxApkInfo *out_info) {
    if (!source_path || !out_info) return SSRNX_ERR_ARGUMENT;
    package_PackagePlan *plan = (package_PackagePlan *)malloc(sizeof(*plan));
    if (!plan) return SSRNX_ERR_NO_MEMORY;

    SsrnxResult cached_result = SSRNX_ERR_APK_INVALID;
    if (package_read_package_disk_cache(source_path, plan, out_info, &cached_result)) {
        SSRNX_PERF_LOCK(&package_g_plan_cache_lock);
        package_g_plan_cache = *plan;
        package_g_plan_cache_source_key = package_source_cache_key(source_path);
        package_g_plan_cache_valid = true;
        pthread_mutex_unlock(&package_g_plan_cache_lock);
        SSRNX_LOGCI("PACKAGE", "metadata cache hit source=%s package=%s result=%d",
                    source_path, out_info->package_name, (int)cached_result);
        free(plan);
        return cached_result;
    }

    SsrnxResult rc = package_discover_plan(source_path, plan);
    if (rc != SSRNX_OK) {
        memset(out_info, 0, sizeof(*out_info));
        (void)package_copy_text(out_info->apk_path, sizeof(out_info->apk_path), source_path);
        out_info->source_xapk = ssrnx_package_is_xapk_name(source_path);
        free(plan);
        return rc;
    }
    rc = package_aggregate_info(plan, out_info);
    if (rc != SSRNX_OK) {
        free(plan);
        return rc;
    }

    const char *engine_name = out_info->engine_kind == SSRNX_ENGINE_UNITY ? "unity" : "unknown";
    const char *unity_backend = out_info->unity_backend == SSRNX_UNITY_BACKEND_IL2CPP ? "il2cpp" :
                                out_info->unity_backend == SSRNX_UNITY_BACKEND_MONO ? "mono" :
                                out_info->unity_backend == SSRNX_UNITY_BACKEND_MIXED ? "mixed" : "none";
    SSRNX_LOGCI("PACKAGE", "source=%s type=%s package=%s apks=%zu selected=%zu obb=%zu arm64=%zu nativeActivity=%u gameActivity=%u javaBootstrap=%u javaLibs=%zu dex=%zu engine=%s unityBackend=%s sdl=%u godot=%u gamemaker=%u",
                source_path, out_info->source_xapk ? "xapk" : "apk", out_info->package_name,
                out_info->package_apk_count, out_info->package_selected_apk_count,
                out_info->package_obb_count, out_info->arm64_lib_count,
                out_info->native_activity ? 1u : 0u,
                out_info->game_activity ? 1u : 0u,
                out_info->java_activity_bootstrap ? 1u : 0u,
                out_info->java_bootstrap_lib_count, out_info->java_dex_count,
                engine_name, unity_backend,
                out_info->sdl_present ? 1u : 0u,
                out_info->godot_present ? 1u : 0u,
                out_info->gamemaker_present ? 1u : 0u);

    SsrnxResult result = SSRNX_ERR_APK_UNSUPPORTED;
    if (out_info->native_activity && out_info->native_lib_name[0]) {
        result = out_info->native_library_present ? SSRNX_OK : SSRNX_ERR_ABI_NOT_FOUND;
    } else if (out_info->arm64_lib_count == 0) {
        result = SSRNX_ERR_ABI_NOT_FOUND;
    } else if (out_info->activity_name[0] && out_info->java_activity_bootstrap &&
               out_info->java_bootstrap_lib_count > 0) {
        result = SSRNX_OK;
    }
    package_write_package_disk_cache(source_path, plan, out_info, result);
    SSRNX_LOGCI("PACKAGE", "metadata cache fill source=%s package=%s result=%d",
                source_path, out_info->package_name, (int)result);
    free(plan);
    return result;
}

SsrnxResult ssrnx_package_extract_arm64(const char *source_path,
                                        const char *destination_dir,
                                        size_t *out_count) {
    if (!source_path || !destination_dir) return SSRNX_ERR_ARGUMENT;
    if (out_count) *out_count = 0;
    package_PackagePlan *plan = (package_PackagePlan *)malloc(sizeof(*plan));
    if (!plan) return SSRNX_ERR_NO_MEMORY;
    SsrnxResult rc = package_discover_plan(source_path, plan);
    if (rc != SSRNX_OK) { free(plan); return rc; }
    if (!package_mkdirs(destination_dir)) { free(plan); return SSRNX_ERR_IO; }

    size_t total = 0;
    for (size_t i = 0; i < plan->component_count; ++i) {
        package_PackageComponent *c = &plan->components[i];
        if (!c->selected || c->info.arm64_lib_count == 0) continue;
        rc = package_ensure_component_materialized(plan, i);
        if (rc != SSRNX_OK) { free(plan); return rc; }
        size_t count = 0;
        rc = ssrnx_apk_extract_arm64(c->apk_path, destination_dir, &count);
        if (rc != SSRNX_OK && rc != SSRNX_ERR_ABI_NOT_FOUND) { free(plan); return rc; }
        total += count;
    }
    if (out_count) *out_count = total;
    free(plan);
    return total ? SSRNX_OK : SSRNX_ERR_ABI_NOT_FOUND;
}

static bool package_safe_relative_path(const char *path) {
    if (!path || !path[0] || path[0] == '/' || path[0] == '\\') return false;
    const char *p = path;
    while (*p) {
        const char *end = strchr(p, '/');
        size_t n = end ? (size_t)(end - p) : strlen(p);
        if (n == 0 || (n == 1 && p[0] == '.') || (n == 2 && p[0] == '.' && p[1] == '.')) return false;
        if (memchr(p, '\\', n)) return false;
        if (!end) break;
        p = end + 1;
    }
    return true;
}

static bool package_make_parent_dirs(const char *path) {
    char tmp[SSRNX_APK_PATH_CAP];
    if (!package_copy_text(tmp, sizeof(tmp), path)) return false;
    char *slash = strrchr(tmp, '/');
    if (!slash) return true;
    *slash = '\0';
    return tmp[0] ? package_mkdirs(tmp) : true;
}

typedef struct package_AssetExtractCtx {
    const char *apk_path;
    const char *destination_dir;
    size_t count;
    SsrnxResult rc;
} package_AssetExtractCtx;

static bool package_split_asset_visitor(const SsrnxZipEntryInfo *entry, void *userdata) {
    package_AssetExtractCtx *ctx = (package_AssetExtractCtx *)userdata;
    if (!entry || !ctx) return false;
    static const char prefix[] = "assets/";
    if (strncmp(entry->name, prefix, sizeof(prefix) - 1u) != 0) return true;
    const char *relative = entry->name + sizeof(prefix) - 1u;
    size_t n = strlen(relative);
    if (!n || relative[n - 1] == '/') return true;
    if (!package_safe_relative_path(relative)) {
        ctx->rc = SSRNX_ERR_APK_INVALID;
        return false;
    }
    char output[SSRNX_APK_PATH_CAP];
    if (!package_path_join(output, sizeof(output), ctx->destination_dir, relative) || !package_make_parent_dirs(output)) {
        ctx->rc = SSRNX_ERR_IO;
        return false;
    }
    ctx->rc = ssrnx_zip_extract_entry(ctx->apk_path, entry->name, output);
    if (ctx->rc != SSRNX_OK) return false;
    ctx->count++;
    return true;
}

SsrnxResult ssrnx_package_extract_split_assets(const char *source_path,
                                               const char *destination_dir,
                                               size_t *out_count) {
    if (!source_path || !destination_dir) return SSRNX_ERR_ARGUMENT;
    if (out_count) *out_count = 0;
    package_PackagePlan *plan = (package_PackagePlan *)malloc(sizeof(*plan));
    if (!plan) return SSRNX_ERR_NO_MEMORY;
    SsrnxResult rc = package_discover_plan(source_path, plan);
    if (rc != SSRNX_OK) { free(plan); return rc; }
    if (!package_mkdirs(destination_dir)) { free(plan); return SSRNX_ERR_IO; }

    size_t total = 0;
    for (size_t i = 0; i < plan->component_count; ++i) {
        package_PackageComponent *c = &plan->components[i];
        if (!c->selected || c->base) continue;
        rc = package_ensure_component_materialized(plan, i);
        if (rc != SSRNX_OK) { free(plan); return rc; }
        package_AssetExtractCtx ctx = {c->apk_path, destination_dir, 0, SSRNX_OK};
        rc = ssrnx_zip_visit(c->apk_path, package_split_asset_visitor, &ctx);
        if (rc != SSRNX_OK) { free(plan); return rc; }
        if (ctx.rc != SSRNX_OK) { rc = ctx.rc; free(plan); return rc; }
        total += ctx.count;
    }
    if (out_count) *out_count = total;
    free(plan);
    return SSRNX_OK;
}

SsrnxResult ssrnx_package_extract_obb(const char *source_path,
                                      const char *destination_dir,
                                      size_t *out_count) {
    if (!source_path || !destination_dir) return SSRNX_ERR_ARGUMENT;
    if (out_count) *out_count = 0;
    if (!ssrnx_package_is_xapk_name(source_path)) return SSRNX_OK;
    package_PackagePlan *plan = (package_PackagePlan *)malloc(sizeof(*plan));
    if (!plan) return SSRNX_ERR_NO_MEMORY;
    SsrnxResult rc = package_discover_plan(source_path, plan);
    if (rc != SSRNX_OK) { free(plan); return rc; }
    if (!package_mkdirs(destination_dir)) { free(plan); return SSRNX_ERR_IO; }

    size_t total = 0;
    for (size_t i = 0; i < plan->outer.obb_count; ++i) {
        const char *entry = plan->outer.obb_entries[i];
        const char *base = strrchr(entry, '/');
        base = base ? base + 1 : entry;
        if (!base[0] || strchr(base, '\\') || strstr(base, "..")) { free(plan); return SSRNX_ERR_APK_INVALID; }
        char output[SSRNX_APK_PATH_CAP];
        if (!package_path_join(output, sizeof(output), destination_dir, base)) { free(plan); return SSRNX_ERR_ARGUMENT; }
        rc = ssrnx_zip_extract_entry(source_path, entry, output);
        if (rc != SSRNX_OK) { free(plan); return rc; }
        total++;
    }
    if (out_count) *out_count = total;
    free(plan);
    return SSRNX_OK;
}







SsrnxResult ssrnx_package_prepare_launch_source(const char *source_path,
                                                SsrnxApkInfo *inout_info) {
    if (!source_path || !inout_info) return SSRNX_ERR_ARGUMENT;
    if (!inout_info->source_xapk) return SSRNX_OK;

    package_PackagePlan *plan = (package_PackagePlan *)malloc(sizeof(*plan));
    if (!plan) return SSRNX_ERR_NO_MEMORY;
    SsrnxResult rc = package_discover_plan(source_path, plan);
    if (rc != SSRNX_OK) { free(plan); return rc; }
    if (plan->base_index >= plan->component_count) { free(plan); return SSRNX_ERR_APK_INVALID; }
    rc = package_ensure_component_materialized(plan, plan->base_index);
    if (rc != SSRNX_OK) { free(plan); return rc; }
    if (!package_copy_text(inout_info->base_apk_path, sizeof(inout_info->base_apk_path),
                   plan->components[plan->base_index].apk_path)) {
        free(plan);
        return SSRNX_ERR_ARGUMENT;
    }
    SSRNX_LOGCI("PACKAGE", "launch source ready source=%s base=%s",
                source_path, inout_info->base_apk_path);
    free(plan);
    return SSRNX_OK;
}


/* ---------- SSRNX core library (boot class path) ----------
 *
 * Boot classes delegate to AOSP overlay/legacy core before package DEX.
 * The overlay is optional and linked as generated DEX bytes by CI/build tooling.
 * A class found in an earlier layer shadows the same class in later layers,
 * matching boot-class-path class ownership rather than mixing members across
 * class definitions. Host/JNI models remain outside this lookup and are never
 * silently replaced by an overlay class. */
#include <zlib.h>
extern const uint8_t ssrnx_aosp_libcore_dex[] __attribute__((weak));
extern const size_t ssrnx_aosp_libcore_dex_size __attribute__((weak));
extern const size_t ssrnx_aosp_libcore_dex_compressed_size __attribute__((weak));
extern const uint8_t ssrnx_core_library_dex[] __attribute__((weak));
extern const size_t ssrnx_core_library_dex_size __attribute__((weak));
extern const size_t ssrnx_core_library_dex_compressed_size __attribute__((weak));
static pthread_once_t package_g_core_once = PTHREAD_ONCE_INIT;

static void package_core_library_init_blob(package_DexActiveBlob *slot,
                                   const uint8_t *data,
                                   const size_t *size_ptr,
                                   const size_t *compressed_size_ptr,
                                   const char *path,
                                   const char *label) {
    if (!slot || !data || !size_ptr || !compressed_size_ptr) return;
    const size_t size = *size_ptr;
    uint8_t *image = malloc(size);
    if (!image) return;
    if (!ssrnx_platform_decode(data, *compressed_size_ptr, image, size) ||
        size < 0x70u || memcmp(image, "dex\n", 4) != 0) {
        free(image);
        SSRNX_LOGCE("JAVA", "%s DEX decompression failed", label);
        return;
    }
    SSRNX_PERF_LOCK(&package_g_dex_blob_lock);
    slot->used = true;
    slot->data = image; /* Boot classes retain pointers for the process lifetime. */
    slot->size = size;
    (void)package_copy_text(slot->path, sizeof(slot->path), path);
    package_dex_blob_build_class_table(slot);
    pthread_mutex_unlock(&package_g_dex_blob_lock);
    SSRNX_LOGCI("JAVA", "%s DEX ready size=%zu classes=%u", label, size,
                package_rd32_mem(slot->data + 0x60));
}

static void package_core_library_init(void) {
    package_core_library_init_blob(&package_g_aosp_core_blob,
                           ssrnx_aosp_libcore_dex,
                           &ssrnx_aosp_libcore_dex_size,
                           &ssrnx_aosp_libcore_dex_compressed_size,
                           "apn64-aosp-libcore.dex",
                           "AOSP libcore overlay");
    package_core_library_init_blob(&package_g_core_blob,
                           ssrnx_core_library_dex,
                           &ssrnx_core_library_dex_size,
                           &ssrnx_core_library_dex_compressed_size,
                           "apn64-core.dex",
                           "core library");
}

/* Secondary DEX tier (dalvik.system.DexClassLoader / BaseDexClassLoader
 * dexPath elements added at runtime). Searched after the package and core
 * tiers, i.e. after the loader's parents, as parent-first delegation does.
 * Loaded images are immutable and stay resident for the process lifetime;
 * they are not core-library classes, so they keep the application loader. */
/* Secondary slots are filled before the count is published (release) and are
 * never removed, so lookups read the count without g_dex_blob_lock; every
 * memoized class/method lookup asks it. */
static size_t package_dex_lookup_tier_count(void) {
    return 2u + __atomic_load_n(&package_g_secondary_dex_count, __ATOMIC_ACQUIRE);
}

static package_DexActiveBlob *package_core_library_blob(size_t index) {
    pthread_once(&package_g_core_once, package_core_library_init);
    if (index == 0) return package_g_aosp_core_blob.data ? &package_g_aosp_core_blob : NULL;
    if (index == 1) return package_g_core_blob.data ? &package_g_core_blob : NULL;
    package_DexActiveBlob *slot = NULL;
    if (index - 2u < __atomic_load_n(&package_g_secondary_dex_count, __ATOMIC_ACQUIRE)) slot = &package_g_secondary_dex[index - 2u];
    return slot && slot->data ? slot : NULL;
}

static SsrnxResult package_secondary_dex_add_image(const char *label, uint8_t *data, size_t size, bool *out_added) {
    if (size < 0x70u || memcmp(data, "dex\n", 4) != 0) { free(data); return SSRNX_ERR_APK_INVALID; }
    SSRNX_PERF_LOCK(&package_g_dex_blob_lock);
    for (size_t i = 0; i < package_g_secondary_dex_count; ++i) {
        if (!strcmp(package_g_secondary_dex[i].path, label)) {
            pthread_mutex_unlock(&package_g_dex_blob_lock);
            free(data);
            return SSRNX_OK; /* the same element is loaded once per process */
        }
    }
    if (package_g_secondary_dex_count >= SSRNX_SECONDARY_DEX_MAX) {
        pthread_mutex_unlock(&package_g_dex_blob_lock);
        free(data);
        return SSRNX_ERR_NO_MEMORY;
    }
    package_DexActiveBlob *slot = &package_g_secondary_dex[package_g_secondary_dex_count];
    memset(slot, 0, sizeof(*slot));
    slot->used = true;
    slot->data = data;
    slot->size = size;
    (void)package_copy_text(slot->path, sizeof(slot->path), label);
    package_dex_blob_build_class_table(slot);
    __atomic_store_n(&package_g_secondary_dex_count, package_g_secondary_dex_count + 1u, __ATOMIC_RELEASE);
    if (ssrnx_jni_class_resolver_invalidate) ssrnx_jni_class_resolver_invalidate();
    pthread_mutex_unlock(&package_g_dex_blob_lock);
    if (out_added) *out_added = true;
    SSRNX_LOGCI("JAVA", "secondary DEX loaded element=%s size=%zu classes=%u", label, size, package_rd32_mem(data + 0x60));
    return SSRNX_OK;
}

SsrnxResult ssrnx_package_add_secondary_dex(const char *host_path, size_t *out_images) {
    if (out_images) *out_images = 0;
    if (!host_path || !host_path[0]) return SSRNX_ERR_ARGUMENT;
    FILE *fp = fopen(host_path, "rb");
    if (!fp) return SSRNX_ERR_IO;
    uint8_t magic[4] = {0};
    const size_t got = fread(magic, 1, sizeof(magic), fp);
    SsrnxResult rc = SSRNX_OK;
    if (got == 4 && memcmp(magic, "dex\n", 4) == 0) {
        /* A bare .dex element. */
        uint8_t *data = NULL;
        long len = 0;
        if (fseek(fp, 0, SEEK_END) != 0 || (len = ftell(fp)) <= 0 || fseek(fp, 0, SEEK_SET) != 0 ||
            !(data = malloc((size_t)len)) || fread(data, 1, (size_t)len, fp) != (size_t)len) {
            free(data);
            fclose(fp);
            return SSRNX_ERR_IO;
        }
        fclose(fp);
        bool added = false;
        rc = package_secondary_dex_add_image(host_path, data, (size_t)len, &added);
        if (rc == SSRNX_OK && out_images) *out_images = 1;
        return rc;
    }
    fclose(fp);
    /* A .jar/.apk/.zip element: classes.dex, classes2.dex, ... (multidex). */
    size_t images = 0;
    for (unsigned n = 1; n < 100u; ++n) {
        char entry[32], label[SSRNX_APK_PATH_CAP];
        if (n == 1) snprintf(entry, sizeof(entry), "classes.dex");
        else snprintf(entry, sizeof(entry), "classes%u.dex", n);
        bool present = false;
        rc = ssrnx_zip_has_entry(host_path, entry, &present);
        if (rc != SSRNX_OK || !present) break;
        uint8_t *data = NULL;
        size_t size = 0;
        rc = ssrnx_zip_read_entry_alloc(host_path, entry, &data, &size);
        if (rc != SSRNX_OK) break;
        snprintf(label, sizeof(label), "%s!/%s", host_path, entry);
        bool added = false;
        rc = package_secondary_dex_add_image(label, data, size, &added);
        if (rc != SSRNX_OK) break;
        images++;
    }
    if (out_images) *out_images = images;
    if (rc == SSRNX_OK && images == 0) return SSRNX_ERR_SYMBOL_NOT_FOUND;
    return rc;
}

struct SsrnxDexFile {
    package_DexActiveBlob images[SSRNX_SECONDARY_DEX_MAX];
    size_t image_count, class_count, byte_count;
    char **names; /* dotted class names, as returned by ART's getClassNameList */
    bool published;
};

/* File input is a new trust boundary: reject truncated ID/definition tables
 * before any existing DEX decoder sees the image. Code items are still checked
 * by the interpreter when loaded. Compact/reverse-endian DEX is unsupported. */
static bool package_dex_file_header_valid(const uint8_t *data, size_t size) {
    if (!data || size < 0x70u || size > SSRNX_DEX_RESIDENT_BUDGET ||
        memcmp(data, "dex\n", 4) || data[7] ||
        (memcmp(data + 4, "035", 3) && memcmp(data + 4, "037", 3) &&
         memcmp(data + 4, "038", 3) && memcmp(data + 4, "039", 3)) ||
        package_rd32_mem(data + 0x20) != size || package_rd32_mem(data + 0x24) != 0x70u ||
        package_rd32_mem(data + 0x28) != 0x12345678u) return false;
    static const unsigned widths[] = {4, 4, 12, 8, 8, 32};
    for (size_t i = 0; i < 6; ++i) {
        const uint32_t count = package_rd32_mem(data + 0x38u + i * 8u);
        const uint32_t off = package_rd32_mem(data + 0x3cu + i * 8u);
        if (count && (off < 0x70u || (off & 3u) || (uint64_t)off + (uint64_t)count * widths[i] > size))
            return false;
    }
    const uint32_t off = package_rd32_mem(data + 0x6c), count = package_rd32_mem(data + 0x68);
    return (uint64_t)off + count <= size;
}

static SsrnxResult package_dex_file_append(SsrnxDexFile *file, uint8_t *data, size_t size) {
    if (!package_dex_file_header_valid(data, size) || file->image_count == SSRNX_SECONDARY_DEX_MAX ||
        size > SSRNX_DEX_RESIDENT_BUDGET - file->byte_count) {
        free(data); return SSRNX_ERR_APK_INVALID;
    }
    const size_t count = package_rd32_mem(data + 0x60), off = package_rd32_mem(data + 0x64);
    char **names = realloc(file->names, (file->class_count + count + 1u) * sizeof(*names));
    if (!names) { free(data); return SSRNX_ERR_NO_MEMORY; }
    file->names = names;
    for (size_t i = 0; i < count; ++i) {
        const char *desc = package_dex_type_descriptor(data, size, package_rd32_mem(data + off + i * 32u));
        const size_t len = desc ? strlen(desc) : 0;
        if (len < 3 || desc[0] != 'L' || desc[len - 1u] != ';') {
            free(data); return SSRNX_ERR_APK_INVALID;
        }
        char *name = malloc(len - 1u);
        if (!name) { free(data); return SSRNX_ERR_NO_MEMORY; }
        memcpy(name, desc + 1, len - 2u); name[len - 2u] = 0;
        for (char *p = name; *p; ++p) if (*p == '/') *p = '.';
        file->names[file->class_count++] = name;
    }
    package_DexActiveBlob *image = &file->images[file->image_count++];
    image->data = data; image->size = size;
    file->byte_count += size;
    return SSRNX_OK;
}

void ssrnx_package_dex_file_close(SsrnxDexFile *file) {
    if (!file) return;
    if (!file->published)
        for (size_t i = 0; i < file->image_count; ++i) free(file->images[i].data);
    for (size_t i = 0; i < file->class_count; ++i) free(file->names[i]);
    free(file->names); free(file);
}

static int package_dex_file_name_compare(const void *a, const void *b) {
    return strcmp(*(char *const *)a, *(char *const *)b);
}
SsrnxResult ssrnx_package_dex_file_open(const char *path, SsrnxDexFile **out_file) {
    if (!path || !out_file) return SSRNX_ERR_ARGUMENT;
    *out_file = NULL;
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0) return SSRNX_ERR_IO;
    if ((uint64_t)st.st_size > SSRNX_DEX_RESIDENT_BUDGET) return SSRNX_ERR_NO_MEMORY;
    SsrnxDexFile *file = calloc(1, sizeof(*file));
    if (!file) return SSRNX_ERR_NO_MEMORY;
    uint8_t *data = NULL; size_t size = 0;
    SsrnxResult rc = package_read_file_alloc(path, &data, &size);
    if (rc == SSRNX_OK && size >= 4u && !memcmp(data, "dex\n", 4)) {
        rc = package_dex_file_append(file, data, size); data = NULL;
    } else if (rc == SSRNX_OK) {
        free(data); data = NULL;
        for (unsigned i = 1; i <= SSRNX_SECONDARY_DEX_MAX + 1u; ++i) {
            char entry[32]; bool present = false;
            if (i == 1) snprintf(entry, sizeof(entry), "classes.dex");
            else snprintf(entry, sizeof(entry), "classes%u.dex", i);
            rc = ssrnx_zip_has_entry(path, entry, &present);
            if (rc != SSRNX_OK || !present) break;
            rc = ssrnx_zip_read_entry_alloc(path, entry, &data, &size);
            if (rc != SSRNX_OK) break;
            rc = package_dex_file_append(file, data, size); data = NULL;
            if (rc != SSRNX_OK) break;
        }
        if (rc == SSRNX_OK && !file->image_count) rc = SSRNX_ERR_APK_INVALID;
    }
    free(data);
    if (rc != SSRNX_OK) { ssrnx_package_dex_file_close(file); return rc; }
    if (file->class_count > 1) {
        qsort(file->names, file->class_count, sizeof(*file->names), package_dex_file_name_compare);
        size_t kept = 0;
        for (size_t i = 0; i < file->class_count; ++i) {
            if (kept && !strcmp(file->names[kept - 1u], file->names[i])) free(file->names[i]);
            else file->names[kept++] = file->names[i];
        }
        file->class_count = kept;
    }
    *out_file = file; return SSRNX_OK;
}

size_t ssrnx_package_dex_file_count(const SsrnxDexFile *file) { return file ? file->class_count : 0; }
const char *ssrnx_package_dex_file_name(const SsrnxDexFile *file, size_t index) {
    return file && index < file->class_count ? file->names[index] : NULL;
}
bool ssrnx_package_dex_file_contains(const SsrnxDexFile *file, const char *name) {
    if (!file || !name) return false;
    for (size_t i = 0; i < file->image_count; ++i)
        if (package_dex_class_def_offset(file->images[i].data, file->images[i].size, name)) return true;
    return false;
}
SsrnxResult ssrnx_package_dex_file_publish(SsrnxDexFile *file) {
    if (!file) return SSRNX_ERR_ARGUMENT;
    SSRNX_PERF_LOCK(&package_g_dex_blob_lock);
    if (!file->published) {
        if (file->image_count > SSRNX_SECONDARY_DEX_MAX - package_g_secondary_dex_count) {
            pthread_mutex_unlock(&package_g_dex_blob_lock); return SSRNX_ERR_NO_MEMORY;
        }
        for (size_t i = 0; i < file->image_count; ++i) {
            package_DexActiveBlob *slot = &package_g_secondary_dex[package_g_secondary_dex_count + i];
            *slot = file->images[i]; slot->used = true;
            /* Opened files are snapshots, never deduplicated by a mutable path. */
            snprintf(slot->path, sizeof(slot->path), "DexFile:%p:%zu", (void *)file, i);
            package_dex_blob_build_class_table(slot);
        }
        file->published = true;
        __atomic_store_n(&package_g_secondary_dex_count, package_g_secondary_dex_count + file->image_count, __ATOMIC_RELEASE);
        if (ssrnx_jni_class_resolver_invalidate) ssrnx_jni_class_resolver_invalidate();
    }
    pthread_mutex_unlock(&package_g_dex_blob_lock);
    return SSRNX_OK;
}

const char *ssrnx_package_core_library_origin(const char *class_name) {
    if (!class_name) return NULL;
    for (size_t i = 0; i < 2; ++i) {
        package_DexActiveBlob *slot = package_core_library_blob(i);
        if (slot && package_dex_class_def_offset(slot->data, slot->size, class_name) != 0)
            return i == 0 ? "aosp-overlay" : "legacy";
    }
    return NULL;
}

bool ssrnx_package_core_library_defines(const char *class_name) {
    return ssrnx_package_core_library_origin(class_name) != NULL;
}

/* Class metadata memo. Whether a class is defined, its superclass and its
 * interfaces never change for a package, yet every field access, class
 * initialization and hierarchy walk asks, which made g_dex_index_lock the
 * hottest lock of startup. Same validity rule as the method memo: entries are
 * keyed by source path and cleared when a lookup tier (DexClassLoader path)
 * is added. */
typedef struct package_DexClassMemo {
    struct package_DexClassMemo *next;
    uint64_t hash;
    char *key;            /* source NUL class NUL */
    size_t key_len;
    bool found;
    char *super_name;
    char *interfaces;
} package_DexClassMemo;

#define DEX_CLASS_MEMO_BUCKETS 8192u
#define DEX_CLASS_MEMO_MAX 65536u
#define DEX_CLASS_MEMO_TEXT_CAP 4096u
static pthread_rwlock_t package_g_class_memo_lock = PTHREAD_RWLOCK_INITIALIZER;
static package_DexClassMemo *package_g_class_memo[DEX_CLASS_MEMO_BUCKETS];
static size_t package_g_class_memo_count;
static size_t package_g_class_memo_tiers;

static void package_class_memo_free(package_DexClassMemo *e) {
    if (!e) return;
    free(e->key); free(e->super_name); free(e->interfaces); free(e);
}

static void package_class_memo_clear_locked(void) {
    for (size_t i = 0; i < DEX_CLASS_MEMO_BUCKETS; ++i) {
        package_DexClassMemo *e = package_g_class_memo[i];
        while (e) {
            package_DexClassMemo *next = e->next;
            package_class_memo_free(e);
            e = next;
        }
        package_g_class_memo[i] = NULL;
    }
    package_g_class_memo_count = 0;
    package_g_class_memo_tiers = 0;
}

void ssrnx_package_clear_class_memo(void) {
    pthread_rwlock_wrlock(&package_g_class_memo_lock);
    package_class_memo_clear_locked();
    pthread_rwlock_unlock(&package_g_class_memo_lock);
}

static size_t package_class_memo_key(const char *source_path, const char *class_name, char *out, size_t cap,
                             uint64_t *out_hash) {
    const size_t s = strlen(source_path), c = strlen(class_name);
    const size_t n = s + c + 2u;
    if (n > cap) return 0;
    memcpy(out, source_path, s + 1u);
    memcpy(out + s + 1u, class_name, c + 1u);
    uint64_t h = UINT64_C(1469598103934665603);
    for (size_t i = 0; i < n; ++i) { h ^= (uint8_t)out[i]; h *= UINT64_C(1099511628211); }
    *out_hash = h;
    return n;
}

static SsrnxResult package_package_find_dex_class_uncached(const char *source_path, const char *class_name,
                                                   char *out_super_name, size_t out_super_name_cap,
                                                   char *out_interfaces, size_t out_interfaces_cap,
                                                   bool *out_found);
static char *package_memo_strdup(const char *text);

SsrnxResult ssrnx_package_find_dex_class_ex(const char *source_path, const char *class_name,
                                            char *out_super_name, size_t out_super_name_cap,
                                            char *out_interfaces, size_t out_interfaces_cap,
                                            bool *out_found) {
    if (!source_path || !class_name || !class_name[0] || !out_found)
        return package_package_find_dex_class_uncached(source_path, class_name, out_super_name, out_super_name_cap,
                                               out_interfaces, out_interfaces_cap, out_found);
    char key[1536];
    uint64_t hash = 0;
    const size_t key_len = package_class_memo_key(source_path, class_name, key, sizeof(key), &hash);
    const size_t tiers = package_dex_lookup_tier_count();
    if (!key_len)
        return package_package_find_dex_class_uncached(source_path, class_name, out_super_name, out_super_name_cap,
                                               out_interfaces, out_interfaces_cap, out_found);
    pthread_rwlock_rdlock(&package_g_class_memo_lock);
    if (package_g_class_memo_tiers != tiers) {
        pthread_rwlock_unlock(&package_g_class_memo_lock);
        pthread_rwlock_wrlock(&package_g_class_memo_lock);
        if (package_g_class_memo_tiers != tiers) {
            package_class_memo_clear_locked();
            package_g_class_memo_tiers = tiers;
        }
    }
    bool hit = false, hit_fits = true, hit_found = false;
    for (package_DexClassMemo *e = package_g_class_memo[hash % DEX_CLASS_MEMO_BUCKETS]; e; e = e->next) {
        if (e->hash != hash || e->key_len != key_len || memcmp(e->key, key, key_len) != 0) continue;
        hit = true;
        hit_found = e->found;
        if (out_super_name && out_super_name_cap)
            hit_fits = package_copy_text(out_super_name, out_super_name_cap, e->super_name ? e->super_name : "");
        if (hit_fits && out_interfaces && out_interfaces_cap)
            hit_fits = package_copy_text(out_interfaces, out_interfaces_cap, e->interfaces ? e->interfaces : "");
        break;
    }
    pthread_rwlock_unlock(&package_g_class_memo_lock);
    if (hit && hit_fits) {
        *out_found = hit_found;
        return SSRNX_OK;
    }
    if (hit) /* caller buffer too small for the cached text: answer it the long way */
        return package_package_find_dex_class_uncached(source_path, class_name, out_super_name, out_super_name_cap,
                                               out_interfaces, out_interfaces_cap, out_found);

    /* Resolve once with full-size buffers so the entry serves every caller. */
    char *super_name = malloc(512u), *interfaces = malloc(DEX_CLASS_MEMO_TEXT_CAP);
    if (!super_name || !interfaces) {
        free(super_name); free(interfaces);
        return package_package_find_dex_class_uncached(source_path, class_name, out_super_name, out_super_name_cap,
                                               out_interfaces, out_interfaces_cap, out_found);
    }
    bool found = false;
    SsrnxResult rc = package_package_find_dex_class_uncached(source_path, class_name, super_name, 512u,
                                                     interfaces, DEX_CLASS_MEMO_TEXT_CAP, &found);
    if (rc == SSRNX_OK) {
        *out_found = found;
        bool fits = true;
        if (out_super_name && out_super_name_cap) fits = package_copy_text(out_super_name, out_super_name_cap, super_name);
        if (fits && out_interfaces && out_interfaces_cap) fits = package_copy_text(out_interfaces, out_interfaces_cap, interfaces);
        if (!fits) {
            free(super_name); free(interfaces);
            return package_package_find_dex_class_uncached(source_path, class_name, out_super_name, out_super_name_cap,
                                                   out_interfaces, out_interfaces_cap, out_found);
        }
        package_DexClassMemo *e = calloc(1, sizeof(*e));
        if (e) {
            e->hash = hash;
            e->key_len = key_len;
            e->key = malloc(key_len);
            e->found = found;
            e->super_name = package_memo_strdup(super_name);
            e->interfaces = package_memo_strdup(interfaces);
            if (e->key) memcpy(e->key, key, key_len);
        }
        if (e && e->key) {
            pthread_rwlock_wrlock(&package_g_class_memo_lock);
            bool skip = package_g_class_memo_count >= DEX_CLASS_MEMO_MAX || package_g_class_memo_tiers != tiers;
            for (package_DexClassMemo *it = package_g_class_memo[hash % DEX_CLASS_MEMO_BUCKETS]; it && !skip; it = it->next)
                skip = it->hash == hash && it->key_len == key_len && memcmp(it->key, key, key_len) == 0;
            if (!skip) {
                e->next = package_g_class_memo[hash % DEX_CLASS_MEMO_BUCKETS];
                package_g_class_memo[hash % DEX_CLASS_MEMO_BUCKETS] = e;
                package_g_class_memo_count++;
                e = NULL;
            }
            pthread_rwlock_unlock(&package_g_class_memo_lock);
        }
        package_class_memo_free(e);
    }
    free(super_name);
    free(interfaces);
    return rc;
}

static SsrnxResult package_package_find_dex_class_uncached(const char *source_path, const char *class_name,
                                                   char *out_super_name, size_t out_super_name_cap,
                                                   char *out_interfaces, size_t out_interfaces_cap,
                                                   bool *out_found) {
    SsrnxResult rc = SSRNX_OK;
    if (out_found && ssrnx_package_core_library_defines(class_name)) *out_found = false;
    else rc = package_package_find_dex_class_app(source_path, class_name, out_super_name,
                                                out_super_name_cap, out_interfaces,
                                                out_interfaces_cap, out_found);
    if (rc != SSRNX_OK || *out_found) return rc;
    for (size_t i = 0, tiers = package_dex_lookup_tier_count(); i < tiers && !*out_found; ++i) {
        package_DexActiveBlob *slot = package_core_library_blob(i);
        if (!slot) continue;
        rc = package_dex_find_defined_class(slot->data, slot->size, class_name,
                                    out_super_name, out_super_name_cap,
                                    out_interfaces, out_interfaces_cap, out_found);
        if (rc != SSRNX_OK) return rc;
    }
    return rc;
}

SsrnxResult ssrnx_package_get_dex_static_field_initial_value(const char *source_path,
                                                              const char *class_name,
                                                              const char *field_name,
                                                              const char *signature,
                                                              SsrnxDexStaticValue *out_value,
                                                              bool *out_class_found,
                                                              bool *out_field_found) {
    SsrnxResult rc = SSRNX_OK;
    if (out_class_found && out_field_found && ssrnx_package_core_library_defines(class_name)) {
        *out_class_found = false; *out_field_found = false;
    } else rc = package_package_static_field_app(source_path, class_name, field_name, signature,
                                              out_value, out_class_found, out_field_found);
    if (rc != SSRNX_OK || *out_class_found) return rc;
    for (size_t i = 0, tiers = package_dex_lookup_tier_count(); i < tiers && !*out_class_found; ++i) {
        package_DexActiveBlob *slot = package_core_library_blob(i);
        if (!slot) continue;
        rc = package_dex_find_static_field_initial_value(slot->data, slot->size, class_name,
                                                 field_name, signature, out_value,
                                                 out_class_found, out_field_found);
        if (rc != SSRNX_OK) return rc;
    }
    return rc;
}

/* Method-resolution memo. DEX images are immutable for the life of a package
 * index, so (class, name, signature) always resolves to the same outcome.
 * Without it every invoke re-hashes the class, scans its class_data and
 * rebuilds proto descriptors under the index lock; with it a hit is a hash
 * probe plus re-acquiring the (normally resident) image. Cleared when the
 * package index or the secondary DEX tier changes. */
typedef enum package_DexMethodMemoOutcome {
    DEX_MEMO_NO_CLASS = 0,     /* no package/core/secondary class */
    DEX_MEMO_NO_METHOD,        /* class defined, method absent: super_class */
    DEX_MEMO_APP,              /* package DEX: re-acquire loc */
    DEX_MEMO_TIER,             /* resident core/secondary tier image */
} package_DexMethodMemoOutcome;

typedef struct package_DexMethodMemo {
    struct package_DexMethodMemo *next;
    uint64_t hash;
    char *key;                 /* class NUL name NUL signature NUL */
    size_t key_len;
    package_DexMethodMemoOutcome outcome;
    package_DexIndexEntry loc;
    uint8_t *tier_data;
    size_t tier_size;
    void *tier_owner;
    uint32_t method_index, access_flags, class_access_flags, code_offset;
    void *app_owner;          /* last resident image of an app method */
    uint64_t app_load_id;
    char *super_class;
    char *apk_path;
    char *dex_name;
} package_DexMethodMemo;

#define DEX_METHOD_MEMO_BUCKETS 16384u
#define DEX_METHOD_MEMO_MAX 131072u
/* Read-mostly: every interpreted call looks a method up, inserts are rare. */
static pthread_rwlock_t package_g_method_memo_lock = PTHREAD_RWLOCK_INITIALIZER;
static package_DexMethodMemo *package_g_method_memo[DEX_METHOD_MEMO_BUCKETS];
static size_t package_g_method_memo_count;
static size_t package_g_method_memo_tiers;

static void package_method_memo_free(package_DexMethodMemo *e) {
    if (!e) return;
    free(e->key); free(e->super_class); free(e->apk_path); free(e->dex_name); free(e);
}

static void package_method_memo_clear_locked(void) {
    for (size_t i = 0; i < DEX_METHOD_MEMO_BUCKETS; ++i) {
        package_DexMethodMemo *e = package_g_method_memo[i];
        while (e) {
            package_DexMethodMemo *next = e->next;
            package_method_memo_free(e);
            e = next;
        }
        package_g_method_memo[i] = NULL;
    }
    package_g_method_memo_count = 0;
    package_g_method_memo_tiers = 0;
}

void ssrnx_package_clear_method_memo(void) {
    pthread_rwlock_wrlock(&package_g_method_memo_lock);
    package_method_memo_clear_locked();
    pthread_rwlock_unlock(&package_g_method_memo_lock);
}

/* The key names the package source too: one package is resolved through
 * more than one source path (the container and its extracted split), and
 * entries of both stay valid side by side. */
static size_t package_method_memo_key(const char *source_path, const char *class_name, const char *method_name,
                              const char *signature, char *out, size_t cap, uint64_t *out_hash) {
    const size_t s = strlen(source_path), a = strlen(class_name), b = strlen(method_name), c = strlen(signature);
    const size_t n = s + a + b + c + 4u;
    if (n > cap) return 0;
    memcpy(out, source_path, s + 1u);
    memcpy(out + s + 1u, class_name, a + 1u);
    memcpy(out + s + a + 2u, method_name, b + 1u);
    memcpy(out + s + a + b + 3u, signature, c + 1u);
    uint64_t h = UINT64_C(1469598103934665603);
    for (size_t i = 0; i < n; ++i) { h ^= (uint8_t)out[i]; h *= UINT64_C(1099511628211); }
    *out_hash = h;
    return n;
}

static char *package_memo_strdup(const char *text) {
    if (!text || !text[0]) return NULL;
    const size_t n = strlen(text) + 1u;
    char *copy = malloc(n);
    if (copy) memcpy(copy, text, n);
    return copy;
}

/* Fills out_method from a memo hit. Returns false when the image could not be
 * re-acquired (the caller then resolves normally). */
static bool package_method_memo_apply(const char *source_path, const package_DexMethodMemo *e,
                              const char *class_name, const char *method_name,
                              const char *signature, SsrnxDexMethodImage *out_method,
                              bool *out_class_found, bool *out_method_found) {
    /* super_class, apk_path and dex_name were copied under the memo lock. */
    out_method->dex_data = NULL; out_method->dex_size = 0; out_method->dex_owner = NULL;
    out_method->insns = NULL;
    const bool method_found = e->outcome == DEX_MEMO_APP || e->outcome == DEX_MEMO_TIER;
    if (!method_found) {
        *out_class_found = e->outcome != DEX_MEMO_NO_CLASS;
        *out_method_found = false;
        return true;
    }
    uint8_t *data = e->tier_data; size_t size = e->tier_size; void *owner = e->tier_owner;
    if (e->outcome == DEX_MEMO_APP && e->app_owner &&
        package_dex_blob_reacquire(e->app_owner, e->app_load_id, &data, &size)) {
        owner = e->app_owner;
    } else if (e->outcome == DEX_MEMO_APP) {
        package_PackagePlan *plan = NULL;
        SSRNX_PERF_LOCK(&package_g_dex_index_lock);
        SsrnxResult rc = package_prepare_dex_cache_locked(source_path);
        if (rc == SSRNX_OK)
            rc = package_dex_index_acquire(source_path, &e->loc, &plan, &data, &size, &owner, NULL, 0);
        pthread_mutex_unlock(&package_g_dex_index_lock);
        free(plan);
        if (rc != SSRNX_OK) return false;
    }
    if (e->code_offset && ((uint64_t)e->code_offset + 16u > size)) {
        if (e->outcome == DEX_MEMO_APP) package_dex_blob_release(owner, data);
        return false;
    }
    *out_class_found = true;
    *out_method_found = true;
    out_method->dex_data = data;
    out_method->dex_size = size;
    out_method->dex_owner = owner;
    out_method->method_index = e->method_index;
    out_method->access_flags = e->access_flags;
    out_method->class_access_flags = e->class_access_flags;
    out_method->code_offset = e->code_offset;
    out_method->registers_size = out_method->ins_size = out_method->outs_size = 0;
    out_method->insns_size = 0;
    if (e->code_offset) {
        const uint8_t *code = data + e->code_offset;
        out_method->registers_size = (uint16_t)(code[0] | ((uint16_t)code[1] << 8));
        out_method->ins_size = (uint16_t)(code[2] | ((uint16_t)code[3] << 8));
        out_method->outs_size = (uint16_t)(code[4] | ((uint16_t)code[5] << 8));
        out_method->insns_size = package_rd32_mem(code + 12u);
        out_method->insns = (const uint16_t *)(const void *)(code + 16u);
    }
    (void)package_copy_text(out_method->declaring_class, sizeof(out_method->declaring_class), class_name);
    (void)package_copy_text(out_method->method_name, sizeof(out_method->method_name), method_name);
    (void)package_copy_text(out_method->signature, sizeof(out_method->signature), signature);
    return true;
}

static SsrnxResult package_package_load_dex_method_uncached(const char *source_path,
                                                    const char *class_name,
                                                    const char *method_name,
                                                    const char *signature,
                                                    SsrnxDexMethodImage *out_method,
                                                    bool *out_class_found,
                                                    bool *out_method_found,
                                                    package_DexMethodMemo *memo) {
    memo->outcome = DEX_MEMO_NO_CLASS;
    SsrnxResult rc = SSRNX_OK;
    if (out_class_found && out_method_found && ssrnx_package_core_library_defines(class_name)) {
        *out_class_found = false; *out_method_found = false;
    } else rc = package_package_load_method_app(source_path, class_name, method_name, signature,
                                             out_method, out_class_found, out_method_found,
                                             &memo->loc);
    if (rc != SSRNX_OK) return rc;
    if (*out_class_found) {
        memo->outcome = *out_method_found ? DEX_MEMO_APP : DEX_MEMO_NO_METHOD;
        return rc;
    }

    for (size_t i = 0, tiers = package_dex_lookup_tier_count(); i < tiers && !*out_class_found; ++i) {
        package_DexActiveBlob *slot = package_core_library_blob(i);
        if (!slot) continue;
        SsrnxDexMethodImage candidate = {0};
        rc = package_dex_load_method_from_data(slot->data, slot->size, class_name, method_name,
                                       signature, &candidate,
                                       out_class_found, out_method_found);
        if (rc != SSRNX_OK) return rc;
        if (!*out_class_found) continue;
        if (*out_method_found) {
            candidate.dex_data = slot->data;
            candidate.dex_size = slot->size;
            candidate.dex_owner = slot;
            (void)package_copy_text(candidate.apk_path, sizeof(candidate.apk_path),
                            slot == &package_g_aosp_core_blob ? "apn64-aosp-libcore" : slot == &package_g_core_blob ? "apn64-core" : "apn64-secondary-dex");
            (void)package_copy_text(candidate.dex_name, sizeof(candidate.dex_name), slot->path);
            *out_method = candidate;
            memo->outcome = DEX_MEMO_TIER;
            memo->tier_data = slot->data;
            memo->tier_size = slot->size;
            memo->tier_owner = slot;
        } else {
            memo->outcome = DEX_MEMO_NO_METHOD;
            if (candidate.super_class[0])
                (void)package_copy_text(out_method->super_class, sizeof(out_method->super_class),
                                candidate.super_class);
        }
    }
    return rc;
}

SsrnxResult ssrnx_package_load_dex_method(const char *source_path,
                                          const char *class_name,
                                          const char *method_name,
                                          const char *signature,
                                          SsrnxDexMethodImage *out_method,
                                          bool *out_class_found,
                                          bool *out_method_found) {
    if (!source_path || !class_name || !method_name || !signature || !out_method ||
        !out_class_found || !out_method_found)
        return package_package_load_method_app(source_path, class_name, method_name, signature,
                                       out_method, out_class_found, out_method_found, NULL);
    char key[2304];
    uint64_t hash = 0;
    const size_t key_len = package_method_memo_key(source_path, class_name, method_name, signature, key, sizeof(key), &hash);
    const size_t tiers = package_dex_lookup_tier_count();
    if (key_len) {
        package_DexMethodMemo hit;
        bool have = false;
        pthread_rwlock_rdlock(&package_g_method_memo_lock);
        if (package_g_method_memo_tiers != tiers) {
            /* A new lookup tier (DexClassLoader path) can shadow resolved
             * outcomes; resolve everything again against the new tiers. */
            pthread_rwlock_unlock(&package_g_method_memo_lock);
            pthread_rwlock_wrlock(&package_g_method_memo_lock);
            if (package_g_method_memo_tiers != tiers) {
                static uint32_t clear_reports;
                if (package_g_method_memo_count && __atomic_fetch_add(&clear_reports, 1u, __ATOMIC_RELAXED) < 16u)
                    SSRNX_LOGCI("JAVA", "method memo cleared entries=%zu oldTiers=%zu newTiers=%zu",
                                (size_t)package_g_method_memo_count, (size_t)package_g_method_memo_tiers, tiers);
                package_method_memo_clear_locked();
                package_g_method_memo_tiers = tiers;
            }
        }
        for (package_DexMethodMemo *e = package_g_method_memo[hash % DEX_METHOD_MEMO_BUCKETS]; e; e = e->next) {
            if (e->hash == hash && e->key_len == key_len && memcmp(e->key, key, key_len) == 0) {
                /* The strings are copied into out_method now, while the entry
                 * cannot be freed; method_memo_apply then only reads ints. */
                hit = *e;
                hit.key = NULL;
                out_method->super_class[0] = '\0';
                if (e->super_class)
                    (void)package_copy_text(out_method->super_class, sizeof(out_method->super_class), e->super_class);
                (void)package_copy_text(out_method->apk_path, sizeof(out_method->apk_path), e->apk_path ? e->apk_path : "");
                (void)package_copy_text(out_method->dex_name, sizeof(out_method->dex_name), e->dex_name ? e->dex_name : "");
                hit.super_class = hit.apk_path = hit.dex_name = NULL;
                have = true;
                break;
            }
        }
        pthread_rwlock_unlock(&package_g_method_memo_lock);
        if (have) {
            const bool ok = package_method_memo_apply(source_path, &hit, class_name, method_name, signature,
                                              out_method, out_class_found, out_method_found);
            if (ok) return SSRNX_OK;
        }
    }

    package_DexMethodMemo fresh;
    memset(&fresh, 0, sizeof(fresh));
    const SsrnxResult rc = package_package_load_dex_method_uncached(source_path, class_name, method_name,
                                                            signature, out_method, out_class_found,
                                                            out_method_found, &fresh);
    if (rc != SSRNX_OK || !key_len) return rc;
    package_DexMethodMemo *e = calloc(1, sizeof(*e));
    if (!e) return rc;
    *e = fresh;
    e->next = NULL;
    e->hash = hash;
    e->key_len = key_len;
    e->key = malloc(key_len);
    e->super_class = package_memo_strdup(out_method->super_class);
    if (*out_method_found) {
        e->method_index = out_method->method_index;
        e->access_flags = out_method->access_flags;
        e->class_access_flags = out_method->class_access_flags;
        e->code_offset = out_method->code_offset;
        e->apk_path = package_memo_strdup(out_method->apk_path);
        e->dex_name = package_memo_strdup(out_method->dex_name);
        if (e->outcome == DEX_MEMO_APP) {
            e->app_owner = out_method->dex_owner;
            e->app_load_id = package_dex_blob_load_id(out_method->dex_owner);
            if (!e->app_load_id) e->app_owner = NULL;
        }
    }
    if (!e->key) { package_method_memo_free(e); return rc; }
    memcpy(e->key, key, key_len);
    pthread_rwlock_wrlock(&package_g_method_memo_lock);
    bool skip = package_g_method_memo_count >= DEX_METHOD_MEMO_MAX || package_g_method_memo_tiers != tiers;
    for (package_DexMethodMemo *it = package_g_method_memo[hash % DEX_METHOD_MEMO_BUCKETS]; it && !skip; it = it->next)
        skip = it->hash == hash && it->key_len == key_len && memcmp(it->key, key, key_len) == 0;
    if (!skip) {
        e->next = package_g_method_memo[hash % DEX_METHOD_MEMO_BUCKETS];
        package_g_method_memo[hash % DEX_METHOD_MEMO_BUCKETS] = e;
        package_g_method_memo_count++;
        e = NULL;
    }
    pthread_rwlock_unlock(&package_g_method_memo_lock);
    package_method_memo_free(e);
    return rc;
}

SsrnxResult ssrnx_package_list_dex_class_members(const char *source_path, const char *class_name,
                                                 SsrnxDexMember **out_members, size_t *out_count,
                                                 bool *out_class_found) {
    SsrnxResult rc = SSRNX_OK;
    if (out_members && out_count && out_class_found && ssrnx_package_core_library_defines(class_name)) {
        *out_members = NULL; *out_count = 0; *out_class_found = false;
    } else rc = package_package_list_members_app(source_path, class_name, out_members, out_count,
                                              out_class_found);
    if (rc != SSRNX_OK || *out_class_found) return rc;
    for (size_t i = 0, tiers = package_dex_lookup_tier_count(); i < tiers && !*out_class_found; ++i) {
        package_DexActiveBlob *slot = package_core_library_blob(i);
        if (!slot) continue;
        package_DexMemberList list = {0};
        rc = package_dex_list_members_from_data(slot->data, slot->size, class_name, &list,
                                        out_class_found);
        if (rc != SSRNX_OK) {
            ssrnx_package_free_dex_members(list.items, list.count);
            *out_class_found = false;
            return rc;
        }
        if (*out_class_found) {
            *out_members = list.items;
            *out_count = list.count;
            return SSRNX_OK;
        }
        ssrnx_package_free_dex_members(list.items, list.count);
    }
    return rc;
}

SsrnxResult ssrnx_package_acquire_dex_class(const char *source_path, const char *class_name,
                                            SsrnxDexClassView *out_view, bool *out_class_found) {
    SsrnxResult rc = SSRNX_OK;
    if (out_view && out_class_found && ssrnx_package_core_library_defines(class_name)) {
        memset(out_view, 0, sizeof(*out_view)); *out_class_found = false;
    } else rc = package_package_acquire_class_app(source_path, class_name, out_view, out_class_found);
    if (rc != SSRNX_OK || *out_class_found) return rc;
    for (size_t i = 0, tiers = package_dex_lookup_tier_count(); i < tiers; ++i) {
        package_DexActiveBlob *slot = package_core_library_blob(i);
        if (!slot) continue;
        const uint32_t def = package_dex_class_def_offset(slot->data, slot->size, class_name);
        if (!def) continue;
        out_view->dex_data = slot->data;
        out_view->dex_size = slot->size;
        out_view->dex_owner = slot;
        out_view->class_def_offset = def;
        *out_class_found = true;
        return SSRNX_OK;
    }
    return rc;
}

#pragma pop_macro("DEX_VALUE_TYPE")
#pragma pop_macro("DEX_VALUE_STRING")
#pragma pop_macro("DEX_VALUE_SHORT")
#pragma pop_macro("DEX_VALUE_NULL")
#pragma pop_macro("DEX_VALUE_METHOD_TYPE")
#pragma pop_macro("DEX_VALUE_METHOD_HANDLE")
#pragma pop_macro("DEX_VALUE_METHOD")
#pragma pop_macro("DEX_VALUE_LONG")
#pragma pop_macro("DEX_VALUE_INT")
#pragma pop_macro("DEX_VALUE_FLOAT")
#pragma pop_macro("DEX_VALUE_FIELD")
#pragma pop_macro("DEX_VALUE_ENUM")
#pragma pop_macro("DEX_VALUE_DOUBLE")
#pragma pop_macro("DEX_VALUE_CHAR")
#pragma pop_macro("DEX_VALUE_BYTE")
#pragma pop_macro("DEX_VALUE_BOOLEAN")
#pragma pop_macro("DEX_VALUE_ARRAY")
#pragma pop_macro("DEX_VALUE_ANNOTATION")
#pragma pop_macro("DEX_METHOD_MEMO_MAX")
#pragma pop_macro("DEX_METHOD_MEMO_BUCKETS")
#pragma pop_macro("DEX_INDEX_CANDIDATES")
#pragma pop_macro("DEX_CLASS_MEMO_TEXT_CAP")
#pragma pop_macro("DEX_CLASS_MEMO_MAX")
#pragma pop_macro("DEX_CLASS_MEMO_BUCKETS")
#pragma pop_macro("APK_INFO_FIELD")
#pragma pop_macro("SSRNX_XAPK_CACHE_ROOT")
#pragma pop_macro("SSRNX_SECONDARY_DEX_MAX")
#pragma pop_macro("SSRNX_PLAN_CACHE_PARKED")
#pragma pop_macro("SSRNX_PACKAGE_MAX_OBBS")
#pragma pop_macro("SSRNX_PACKAGE_MAX_ARM64_NAMES")
#pragma pop_macro("SSRNX_PACKAGE_MAX_APKS")
#pragma pop_macro("SSRNX_PACKAGE_CACHE_VERSION")
#pragma pop_macro("SSRNX_PACKAGE_CACHE_ROOT")
#pragma pop_macro("SSRNX_PACKAGE_CACHE_MAGIC")
#pragma pop_macro("SSRNX_LAUNCHER_CACHE_VERSION")
#pragma pop_macro("SSRNX_LAUNCHER_CACHE_MAGIC")
#pragma pop_macro("SSRNX_ICON_CACHE_VERSION")
#pragma pop_macro("SSRNX_ICON_CACHE_MAX_DIM")
#pragma pop_macro("SSRNX_ICON_CACHE_MAGIC")
#pragma pop_macro("SSRNX_DEX_RESIDENT_BUDGET")
#pragma pop_macro("SSRNX_DEX_LOADING_SLOTS")
#pragma pop_macro("SSRNX_DEX_INDEX_VERSION")
#pragma pop_macro("SSRNX_DEX_INDEX_PARKED")
#pragma pop_macro("SSRNX_DEX_INDEX_MAGIC")
#pragma pop_macro("SSRNX_DEX_ACTIVE_SLOTS")


/* ---- assets ---- */
#pragma push_macro("SSRNX_ASSET_PATH_CAP")
#pragma push_macro("O_CLOEXEC")
#pragma push_macro("ZIP_CENTRAL_SIG")
#pragma push_macro("ZIP_EOCD_SIG")
#pragma push_macro("ZIP_LOCAL_SIG")
#pragma push_macro("ZIP_METHOD_DEFLATE")
#pragma push_macro("ZIP_METHOD_STORE")


#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>



/* Production links diagnostics/log.c; focused host tests that link the asset
 * manager without it get the non-tracing path. */
extern bool ssrnx_diag_full_trace_enabled(void) __attribute__((weak));

static bool assets_asset_full_trace(void) {
    return ssrnx_diag_full_trace_enabled && ssrnx_diag_full_trace_enabled();
}
static _Atomic unsigned assets_g_asset_open_reports, assets_g_asset_missing_reports;

#define SSRNX_ASSET_PATH_CAP 768u
#define ZIP_EOCD_SIG 0x06054b50u
#define ZIP_CENTRAL_SIG 0x02014b50u
#define ZIP_LOCAL_SIG 0x04034b50u
#define ZIP_METHOD_STORE 0u
#define ZIP_METHOD_DEFLATE 8u

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

/* ---- Tiny raw-DEFLATE decoder ------------------------------------------------
 * APK entries using ZIP compression method 8 contain a raw RFC 1951 stream.
 * This decoder intentionally implements only the DEFLATE format, not zlib or
 * gzip wrappers, so the runtime has no external zlib/libzip dependency. */

typedef struct assets_BitReader {
    const uint8_t *data;
    size_t size;
    size_t pos;
    uint32_t bits;
    unsigned count;
} assets_BitReader;

typedef struct assets_Huffman {
    uint16_t count[16];
    uint16_t symbol[288];
} assets_Huffman;

static bool assets_br_bits(assets_BitReader *br, unsigned need, uint32_t *out) {
    if (!br || !out || need > 24) return false;
    while (br->count < need) {
        if (br->pos >= br->size) return false;
        br->bits |= (uint32_t)br->data[br->pos++] << br->count;
        br->count += 8;
    }
    *out = need ? (br->bits & ((UINT32_C(1) << need) - 1u)) : 0u;
    br->bits >>= need;
    br->count -= need;
    return true;
}

static void assets_br_align(assets_BitReader *br) {
    if (!br) return;
    br->bits = 0;
    br->count = 0;
}

static bool assets_huff_construct(assets_Huffman *h, const uint8_t *lengths, size_t count) {
    if (!h || (!lengths && count)) return false;
    memset(h, 0, sizeof(*h));
    for (size_t i = 0; i < count; ++i) {
        if (lengths[i] > 15) return false;
        h->count[lengths[i]]++;
    }
    if (h->count[0] == count) return true;

    int left = 1;
    for (unsigned len = 1; len <= 15; ++len) {
        left <<= 1;
        left -= h->count[len];
        if (left < 0) return false;
    }

    uint16_t offsets[16] = {0};
    offsets[1] = 0;
    for (unsigned len = 1; len < 15; ++len) offsets[len + 1] = (uint16_t)(offsets[len] + h->count[len]);
    for (size_t sym = 0; sym < count; ++sym) {
        const uint8_t len = lengths[sym];
        if (len) h->symbol[offsets[len]++] = (uint16_t)sym;
    }
    return true;
}

static int assets_huff_decode(assets_BitReader *br, const assets_Huffman *h) {
    int code = 0;
    int first = 0;
    int index = 0;
    for (int len = 1; len <= 15; ++len) {
        uint32_t bit = 0;
        if (!assets_br_bits(br, 1, &bit)) return -1;
        code |= (int)bit;
        const int count = h->count[len];
        if (code >= first && code < first + count) return h->symbol[index + (code - first)];
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    return -1;
}

static bool assets_build_fixed(assets_Huffman *litlen, assets_Huffman *dist) {
    uint8_t ll[288];
    uint8_t dd[32];
    for (int i = 0; i <= 143; ++i) ll[i] = 8;
    for (int i = 144; i <= 255; ++i) ll[i] = 9;
    for (int i = 256; i <= 279; ++i) ll[i] = 7;
    for (int i = 280; i <= 287; ++i) ll[i] = 8;
    for (int i = 0; i < 32; ++i) dd[i] = 5;
    return assets_huff_construct(litlen, ll, 288) && assets_huff_construct(dist, dd, 32);
}

static bool assets_build_dynamic(assets_BitReader *br, assets_Huffman *litlen, assets_Huffman *dist) {
    static const uint8_t order[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
    uint32_t v = 0;
    if (!assets_br_bits(br, 5, &v)) return false;
    const unsigned nlit = v + 257;
    if (!assets_br_bits(br, 5, &v)) return false;
    const unsigned ndist = v + 1;
    if (!assets_br_bits(br, 4, &v)) return false;
    const unsigned ncode = v + 4;
    if (nlit > 286 || ndist > 32) return false;

    uint8_t codelen[19] = {0};
    for (unsigned i = 0; i < ncode; ++i) {
        if (!assets_br_bits(br, 3, &v)) return false;
        codelen[order[i]] = (uint8_t)v;
    }
    assets_Huffman code_tree;
    if (!assets_huff_construct(&code_tree, codelen, 19)) return false;

    uint8_t lengths[286 + 32];
    unsigned have = 0;
    const unsigned total = nlit + ndist;
    while (have < total) {
        const int sym = assets_huff_decode(br, &code_tree);
        if (sym < 0) return false;
        if (sym <= 15) {
            lengths[have++] = (uint8_t)sym;
        } else if (sym == 16) {
            if (have == 0 || !assets_br_bits(br, 2, &v)) return false;
            unsigned repeat = v + 3;
            if (have + repeat > total) return false;
            const uint8_t prev = lengths[have - 1];
            while (repeat--) lengths[have++] = prev;
        } else if (sym == 17) {
            if (!assets_br_bits(br, 3, &v)) return false;
            unsigned repeat = v + 3;
            if (have + repeat > total) return false;
            while (repeat--) lengths[have++] = 0;
        } else if (sym == 18) {
            if (!assets_br_bits(br, 7, &v)) return false;
            unsigned repeat = v + 11;
            if (have + repeat > total) return false;
            while (repeat--) lengths[have++] = 0;
        } else {
            return false;
        }
    }
    if (lengths[256] == 0) return false;
    return assets_huff_construct(litlen, lengths, nlit) && assets_huff_construct(dist, lengths + nlit, ndist);
}

static bool assets_inflate_codes(assets_BitReader *br, const assets_Huffman *litlen, const assets_Huffman *dist,
                          uint8_t *out, size_t out_size, size_t *out_pos) {
    static const uint16_t len_base[29] = {
        3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258
    };
    static const uint8_t len_extra[29] = {
        0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0
    };
    static const uint16_t dist_base[30] = {
        1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,
        4097,6145,8193,12289,16385,24577
    };
    static const uint8_t dist_extra[30] = {
        0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13
    };

    for (;;) {
        const int sym = assets_huff_decode(br, litlen);
        if (sym < 0) return false;
        if (sym < 256) {
            if (*out_pos >= out_size) return false;
            out[(*out_pos)++] = (uint8_t)sym;
            continue;
        }
        if (sym == 256) return true;
        if (sym < 257 || sym > 285) return false;
        const unsigned li = (unsigned)(sym - 257);
        uint32_t extra = 0;
        if (!assets_br_bits(br, len_extra[li], &extra)) return false;
        const size_t length = (size_t)len_base[li] + extra;

        const int dsym = assets_huff_decode(br, dist);
        if (dsym < 0 || dsym > 29) return false;
        if (!assets_br_bits(br, dist_extra[dsym], &extra)) return false;
        const size_t distance = (size_t)dist_base[dsym] + extra;
        if (distance == 0 || distance > *out_pos || length > out_size - *out_pos) return false;
        for (size_t i = 0; i < length; ++i) {
            out[*out_pos] = out[*out_pos - distance];
            (*out_pos)++;
        }
    }
}

static bool assets_inflate_raw(const uint8_t *src, size_t src_size, uint8_t *dst, size_t dst_size) {
    assets_BitReader br = {src, src_size, 0, 0, 0};
    size_t out_pos = 0;
    bool final = false;
    while (!final) {
        uint32_t bit = 0, type = 0;
        if (!assets_br_bits(&br, 1, &bit) || !assets_br_bits(&br, 2, &type)) return false;
        final = bit != 0;
        if (type == 0) {
            assets_br_align(&br);
            if (br.pos + 4 > br.size) return false;
            const uint16_t len = (uint16_t)(br.data[br.pos] | ((uint16_t)br.data[br.pos + 1] << 8));
            const uint16_t nlen = (uint16_t)(br.data[br.pos + 2] | ((uint16_t)br.data[br.pos + 3] << 8));
            br.pos += 4;
            if ((uint16_t)~len != nlen || br.pos + len > br.size || len > dst_size - out_pos) return false;
            memcpy(dst + out_pos, br.data + br.pos, len);
            br.pos += len;
            out_pos += len;
        } else if (type == 1 || type == 2) {
            assets_Huffman litlen, dist;
            if (type == 1) {
                if (!assets_build_fixed(&litlen, &dist)) return false;
            } else if (!assets_build_dynamic(&br, &litlen, &dist)) {
                return false;
            }
            if (!assets_inflate_codes(&br, &litlen, &dist, dst, dst_size, &out_pos)) return false;
        } else {
            return false;
        }
    }
    return out_pos == dst_size;
}

/* ---- ZIP/APK index ---------------------------------------------------------- */

typedef struct assets_ZipEntry {
    char *name; /* assets/-relative path */
    uint16_t method;
    uint16_t flags;
    uint32_t crc32;
    uint64_t compressed_size;
    uint64_t uncompressed_size;
    uint64_t local_offset;
    struct assets_ZipEntry *next;
} assets_ZipEntry;

typedef struct assets_ZipArchive {
    char path[SSRNX_ASSET_PATH_CAP];
    assets_ZipEntry *entries;
    size_t entry_count;
    /* Open-addressed name index over `entries` (immutable after indexing). */
    assets_ZipEntry **index;
    size_t index_mask;
} assets_ZipArchive;

typedef enum assets_AssetSourceType {
    ASSET_SOURCE_DIRECTORY = 1,
    ASSET_SOURCE_ZIP = 2,
} assets_AssetSourceType;

typedef struct assets_AssetSource {
    assets_AssetSourceType type;
    char path[SSRNX_ASSET_PATH_CAP];
    assets_ZipArchive zip;
} assets_AssetSource;

struct SsrnxAsset {
    SsrnxAssetManager *manager;
    uint8_t *data;
    size_t length;
    size_t offset;
    bool allocated;
    char fd_path[SSRNX_ASSET_PATH_CAP];
    int64_t fd_start;
    int64_t fd_length;
    bool fd_supported;
    struct SsrnxAsset *next;
};

struct SsrnxAssetDir {
    SsrnxAssetManager *manager;
    char **names;
    size_t count;
    size_t index;
    struct SsrnxAssetDir *next;
};

struct SsrnxAssetManager {
    bool initialized;
    assets_AssetSource sources[2];
    size_t source_count;
    SsrnxAsset *assets;
    SsrnxAssetDir *dirs;
    bool configuration_valid;
    SsrnxAssetConfiguration configuration;
};

static SsrnxAssetManager assets_g_assets;
static pthread_mutex_t assets_g_assets_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t assets_g_font_trace_lock = PTHREAD_MUTEX_INITIALIZER;

static uint64_t assets_g_font_asset_trace_hashes[64];
static size_t assets_g_font_asset_trace_count;

static bool assets_asset_name_fontish(const char *name) {
    if (!name || !name[0]) return false;
    for (const char *p = name; *p; ++p) {
        if ((p[0] == 'f' || p[0] == 'F') &&
            (p[1] == 'o' || p[1] == 'O') &&
            (p[2] == 'n' || p[2] == 'N') &&
            (p[3] == 't' || p[3] == 'T')) return true;
    }
    const size_t n = strlen(name);
    return n >= 4u && name[n - 4u] == '.' &&
           (name[n - 3u] == 'f' || name[n - 3u] == 'F') &&
           (name[n - 2u] == 'n' || name[n - 2u] == 'N') &&
           (name[n - 1u] == 't' || name[n - 1u] == 'T');
}

static bool assets_font_asset_trace_once(const char *name) {
    if (!assets_asset_name_fontish(name)) return false;
    uint64_t h = UINT64_C(1469598103934665603);
    for (const unsigned char *p = (const unsigned char *)name; *p; ++p) {
        h ^= (uint64_t)*p;
        h *= UINT64_C(1099511628211);
    }
    pthread_mutex_lock(&assets_g_font_trace_lock);
    for (size_t i = 0; i < assets_g_font_asset_trace_count; ++i) {
        if (assets_g_font_asset_trace_hashes[i] == h) {
            pthread_mutex_unlock(&assets_g_font_trace_lock);
            return false;
        }
    }
    if (assets_g_font_asset_trace_count < sizeof(assets_g_font_asset_trace_hashes) / sizeof(assets_g_font_asset_trace_hashes[0]))
        assets_g_font_asset_trace_hashes[assets_g_font_asset_trace_count++] = h;
    pthread_mutex_unlock(&assets_g_font_trace_lock);
    return true;
}

static uint16_t assets_rd16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t assets_rd32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint64_t assets_rd64(const uint8_t *p) { return (uint64_t)assets_rd32(p) | ((uint64_t)assets_rd32(p + 4) << 32); }

static uint32_t assets_crc32_bytes(const uint8_t *data, size_t size) {
    uint32_t crc = UINT32_C(0xffffffff);
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b) crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & (uint32_t)-(int32_t)(crc & 1u));
    }
    return ~crc;
}

static bool assets_copy_path(char *out, size_t cap, const char *path) {
    if (!out || !cap || !path) return false;
    const size_t n = strlen(path);
    if (n >= cap) { out[0] = '\0'; return false; }
    memcpy(out, path, n + 1);
    return true;
}

static bool assets_join_path(char *out, size_t cap, const char *a, const char *b) {
    if (!out || !cap || !a || !b) return false;
    const size_t an = strlen(a), bn = strlen(b);
    const bool sep = an && a[an - 1] != '/';
    if (an + (sep ? 1u : 0u) + bn >= cap) { out[0] = '\0'; return false; }
    memcpy(out, a, an);
    size_t pos = an;
    if (sep) out[pos++] = '/';
    memcpy(out + pos, b, bn + 1);
    return true;
}

static bool assets_normalize_asset_path(const char *in, char *out, size_t cap, bool allow_empty) {
    if (!in || !out || !cap) return false;
    if (!in[0]) { out[0] = '\0'; return allow_empty; }
    if (in[0] == '/' || strchr(in, ':')) return false;
    size_t w = 0, seg_start = 0;
    for (size_t r = 0;; ++r) {
        char c = in[r];
        if (c == '\\') c = '/';
        if (c == '/' || c == '\0') {
            const size_t seg_len = w - seg_start;
            /* AssetManager directory APIs commonly accept a trailing slash. */
            if (c == '\0' && allow_empty && seg_len == 0 && w > 0 && out[w - 1] == '/') {
                --w;
                break;
            }
            if (seg_len == 0 || (seg_len == 1 && out[seg_start] == '.') ||
                (seg_len == 2 && out[seg_start] == '.' && out[seg_start + 1] == '.')) return false;
            if (c == '\0') break;
            if (w + 1 >= cap) return false;
            out[w++] = '/';
            seg_start = w;
        } else {
            if ((unsigned char)c < 0x20 || w + 1 >= cap) return false;
            out[w++] = c;
        }
    }
    out[w] = '\0';
    return true;
}

static bool assets_parse_zip64_extra(const uint8_t *extra, size_t extra_len,
                              uint64_t *usize, uint64_t *csize, uint64_t *offset,
                              bool need_usize, bool need_csize, bool need_offset) {
    size_t pos = 0;
    while (pos + 4 <= extra_len) {
        const uint16_t id = assets_rd16(extra + pos);
        const uint16_t len = assets_rd16(extra + pos + 2);
        pos += 4;
        if (pos + len > extra_len) return false;
        if (id == 0x0001) {
            size_t q = pos;
            if (need_usize) { if (q + 8 > pos + len) return false; *usize = assets_rd64(extra + q); q += 8; }
            if (need_csize) { if (q + 8 > pos + len) return false; *csize = assets_rd64(extra + q); q += 8; }
            if (need_offset) { if (q + 8 > pos + len) return false; *offset = assets_rd64(extra + q); }
            return true;
        }
        pos += len;
    }
    return !(need_usize || need_csize || need_offset);
}

static void assets_zip_free(assets_ZipArchive *zip) {
    if (!zip) return;
    assets_ZipEntry *entry = zip->entries;
    while (entry) { assets_ZipEntry *next = entry->next; free(entry->name); free(entry); entry = next; }
    free(zip->index);
    memset(zip, 0, sizeof(*zip));
}

static size_t assets_zip_name_hash(const char *name) {
    uint64_t h = UINT64_C(1469598103934665603);
    for (const unsigned char *p = (const unsigned char *)name; *p; ++p) h = (h ^ *p) * UINT64_C(1099511628211);
    return (size_t)(h ^ (h >> 29));
}

/* Lookups were a linear strcmp walk over every asset per open. Without an
 * index (allocation failure) zip_find keeps that walk as a fallback. The
 * list is newest-first, so the first inserted duplicate name wins, as the
 * walk found it. */
static void assets_zip_build_index(assets_ZipArchive *zip) {
    size_t cap = 16;
    while (cap < zip->entry_count * 2u) cap <<= 1;
    assets_ZipEntry **index = calloc(cap, sizeof(*index));
    if (!index) return;
    for (assets_ZipEntry *e = zip->entries; e; e = e->next) {
        size_t slot = assets_zip_name_hash(e->name) & (cap - 1u);
        while (index[slot] && strcmp(index[slot]->name, e->name) != 0) slot = (slot + 1u) & (cap - 1u);
        index[slot] = e;
    }
    zip->index = index;
    zip->index_mask = cap - 1u;
}

static SsrnxResult assets_zip_open_index(assets_ZipArchive *zip, const char *path) {
    if (!zip || !path || !path[0] || !assets_copy_path(zip->path, sizeof(zip->path), path)) return SSRNX_ERR_ARGUMENT;
    FILE *fp = fopen(path, "rb");
    if (!fp) return SSRNX_ERR_IO;
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return SSRNX_ERR_IO; }
    const long file_end = ftell(fp);
    if (file_end < 22) { fclose(fp); return SSRNX_ERR_IO; }
    const size_t tail_size = (size_t)(file_end < 65557 ? file_end : 65557);
    if (fseek(fp, file_end - (long)tail_size, SEEK_SET) != 0) { fclose(fp); return SSRNX_ERR_IO; }
    uint8_t *tail = malloc(tail_size);
    if (!tail) { fclose(fp); return SSRNX_ERR_NO_MEMORY; }
    if (fread(tail, 1, tail_size, fp) != tail_size) { free(tail); fclose(fp); return SSRNX_ERR_IO; }
    size_t eocd = SIZE_MAX;
    for (size_t i = tail_size - 22 + 1; i-- > 0;) {
        if (assets_rd32(tail + i) == ZIP_EOCD_SIG && i + 22u + assets_rd16(tail + i + 20) <= tail_size) { eocd = i; break; }
    }
    if (eocd == SIZE_MAX) { free(tail); fclose(fp); return SSRNX_ERR_IO; }
    const uint16_t disk = assets_rd16(tail + eocd + 4), cd_disk = assets_rd16(tail + eocd + 6);
    const uint16_t entries_disk = assets_rd16(tail + eocd + 8), entries = assets_rd16(tail + eocd + 10);
    const uint32_t cd_size32 = assets_rd32(tail + eocd + 12), cd_off32 = assets_rd32(tail + eocd + 16);
    free(tail);
    if (disk != 0 || cd_disk != 0 || entries_disk != entries || entries == 0xffffu ||
        cd_size32 == UINT32_MAX || cd_off32 == UINT32_MAX) {
        fclose(fp);
        SSRNX_LOGCW("ASSET", "ZIP64/multi-disk EOCD not supported path=%s", path);
        return SSRNX_ERR_DYNAMIC_INVALID;
    }
    if ((uint64_t)cd_off32 + cd_size32 > (uint64_t)file_end || fseek(fp, (long)cd_off32, SEEK_SET) != 0) {
        fclose(fp); return SSRNX_ERR_IO;
    }
    /* One read for the whole central directory; entries parse from memory. */
    uint8_t *central = malloc(cd_size32 ? cd_size32 : 1u);
    if (!central) { fclose(fp); return SSRNX_ERR_NO_MEMORY; }
    if (cd_size32 && fread(central, 1, cd_size32, fp) != cd_size32) { free(central); fclose(fp); return SSRNX_ERR_IO; }
    fclose(fp);

    size_t pos = 0;
    for (uint32_t i = 0; i < entries; ++i) {
        if (cd_size32 - pos < 46u || assets_rd32(central + pos) != ZIP_CENTRAL_SIG) { free(central); assets_zip_free(zip); return SSRNX_ERR_IO; }
        const uint8_t *hdr = central + pos;
        const uint16_t flags = assets_rd16(hdr + 8), method = assets_rd16(hdr + 10);
        uint64_t csize = assets_rd32(hdr + 20), usize = assets_rd32(hdr + 24), local = assets_rd32(hdr + 42);
        const uint16_t name_len = assets_rd16(hdr + 28), extra_len = assets_rd16(hdr + 30), comment_len = assets_rd16(hdr + 32);
        const size_t record = 46u + (size_t)name_len + extra_len + comment_len;
        if (cd_size32 - pos < record) { free(central); assets_zip_free(zip); return SSRNX_ERR_IO; }
        const uint8_t *raw_name = hdr + 46, *extra = raw_name + name_len;
        pos += record;
        const bool need_u = usize == UINT32_MAX, need_c = csize == UINT32_MAX, need_o = local == UINT32_MAX;
        if ((need_u || need_c || need_o) && !assets_parse_zip64_extra(extra, extra_len, &usize, &csize, &local, need_u, need_c, need_o)) {
            free(central); assets_zip_free(zip); return SSRNX_ERR_IO;
        }
        /* Only assets/ files are indexed; longer names cannot normalize. */
        char name[SSRNX_ASSET_PATH_CAP + 8];
        if (name_len <= 7 || name_len >= sizeof(name) || memcmp(raw_name, "assets/", 7) != 0 || raw_name[name_len - 1] == '/')
            continue;
        memcpy(name, raw_name, name_len);
        name[name_len] = '\0';
        char normalized[SSRNX_ASSET_PATH_CAP];
        if (!assets_normalize_asset_path(name + 7, normalized, sizeof(normalized), false)) continue;
        assets_ZipEntry *entry = calloc(1, sizeof(*entry));
        if (!entry) { free(central); assets_zip_free(zip); return SSRNX_ERR_NO_MEMORY; }
        entry->name = strdup(normalized);
        if (!entry->name) { free(entry); free(central); assets_zip_free(zip); return SSRNX_ERR_NO_MEMORY; }
        entry->flags = flags; entry->method = method; entry->crc32 = assets_rd32(hdr + 16);
        entry->compressed_size = csize; entry->uncompressed_size = usize; entry->local_offset = local;
        entry->next = zip->entries; zip->entries = entry; zip->entry_count++;
    }
    free(central);
    assets_zip_build_index(zip);
    SSRNX_LOGCI("ASSET", "indexed APK path=%s assets=%zu", path, zip->entry_count);
    return SSRNX_OK;
}

static assets_ZipEntry *assets_zip_find(assets_ZipArchive *zip, const char *name) {
    if (!zip) return NULL;
    if (zip->index) {
        for (size_t slot = assets_zip_name_hash(name) & zip->index_mask; zip->index[slot]; slot = (slot + 1u) & zip->index_mask)
            if (strcmp(zip->index[slot]->name, name) == 0) return zip->index[slot];
        return NULL;
    }
    for (assets_ZipEntry *e = zip->entries; e; e = e->next) if (strcmp(e->name, name) == 0) return e;
    return NULL;
}

static bool assets_zip_data_offset(FILE *fp, const assets_ZipEntry *entry, uint64_t *out) {
    if (!fp || !entry || !out || entry->local_offset > LONG_MAX || fseek(fp, (long)entry->local_offset, SEEK_SET) != 0) return false;
    uint8_t hdr[30];
    if (fread(hdr, 1, sizeof(hdr), fp) != sizeof(hdr) || assets_rd32(hdr) != ZIP_LOCAL_SIG) return false;
    *out = entry->local_offset + 30u + assets_rd16(hdr + 26) + assets_rd16(hdr + 28);
    return true;
}

/* Reads one entry from a value snapshot (no manager lock held). Stored data
 * is read straight into the asset buffer and is not CRC-checked, matching
 * Android's AssetManager, which maps stored assets without verification;
 * deflated data keeps the inflate + CRC check. */
static bool assets_load_zip_entry(const char *zip_path, const assets_ZipEntry *entry, uint8_t **out_data, size_t *out_len,
                           bool *fd_supported, int64_t *fd_start) {
    if ((entry->flags & 1u) != 0 || entry->uncompressed_size > SIZE_MAX || entry->compressed_size > SIZE_MAX) return false;
    if (entry->method != ZIP_METHOD_STORE && entry->method != ZIP_METHOD_DEFLATE) return false;
    const size_t clen = (size_t)entry->compressed_size, ulen = (size_t)entry->uncompressed_size;
    if (entry->method == ZIP_METHOD_STORE && clen != ulen) return false;
    FILE *fp = fopen(zip_path, "rb");
    if (!fp) return false;
    uint64_t data_off = 0;
    if (!assets_zip_data_offset(fp, entry, &data_off) || data_off > LONG_MAX || fseek(fp, (long)data_off, SEEK_SET) != 0) { fclose(fp); return false; }
    uint8_t *data = malloc(ulen ? ulen : 1u);
    if (!data) { fclose(fp); return false; }
    bool ok;
    if (entry->method == ZIP_METHOD_STORE) {
        ok = !ulen || fread(data, 1, ulen, fp) == ulen;
        fclose(fp);
    } else {
        uint8_t *compressed = clen ? malloc(clen) : NULL;
        ok = (!clen || compressed) && (!clen || fread(compressed, 1, clen, fp) == clen);
        fclose(fp);
        ok = ok && assets_inflate_raw(compressed, clen, data, ulen) && assets_crc32_bytes(data, ulen) == entry->crc32;
        free(compressed);
    }
    if (!ok) { free(data); return false; }
    *out_data = data; *out_len = ulen;
    *fd_supported = entry->method == ZIP_METHOD_STORE;
    *fd_start = (int64_t)data_off;
    return true;
}

/* ---- Asset manager ---------------------------------------------------------- */

static bool assets_regular_file(const char *path, size_t *out_size) {
    struct stat st;
    if (!path || stat(path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0) return false;
    if (out_size) *out_size = (size_t)st.st_size;
    return true;
}

static SsrnxAsset *assets_asset_alloc(SsrnxAssetManager *manager) {
    SsrnxAsset *asset = calloc(1, sizeof(*asset));
    if (!asset) return NULL;
    asset->manager = manager;
    asset->allocated = true;
    return asset;
}

static bool assets_load_directory_asset(assets_AssetSource *source, const char *name, SsrnxAsset *asset) {
    char path[SSRNX_ASSET_PATH_CAP];
    if (!assets_join_path(path, sizeof(path), source->path, name)) return false;
    size_t size = 0;
    if (!assets_regular_file(path, &size)) return false;
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;
    uint8_t *data = malloc(size ? size : 1u);
    if (!data) { fclose(fp); return false; }
    if (size && fread(data, 1, size, fp) != size) { free(data); fclose(fp); return false; }
    fclose(fp);
    asset->data = data; asset->length = size; asset->fd_supported = true; asset->fd_start = 0; asset->fd_length = (int64_t)size;
    return assets_copy_path(asset->fd_path, sizeof(asset->fd_path), path);
}

SsrnxResult ssrnx_assets_init(const char *directory_root, const char *apk_path) {
    ssrnx_assets_reset();
    atomic_store(&assets_g_asset_open_reports, 0u);
    atomic_store(&assets_g_asset_missing_reports, 0u);
    assets_g_assets.initialized = true;
    if (directory_root && directory_root[0]) {
        assets_AssetSource *s = &assets_g_assets.sources[assets_g_assets.source_count];
        if (!assets_copy_path(s->path, sizeof(s->path), directory_root)) { ssrnx_assets_reset(); return SSRNX_ERR_ARGUMENT; }
        struct stat st;
        if (stat(directory_root, &st) != 0 || !S_ISDIR(st.st_mode)) { ssrnx_assets_reset(); return SSRNX_ERR_IO; }
        s->type = ASSET_SOURCE_DIRECTORY; assets_g_assets.source_count++;
        SSRNX_LOGCI("ASSET", "mounted directory root=%s", directory_root);
    }
    if (apk_path && apk_path[0]) {
        assets_AssetSource *s = &assets_g_assets.sources[assets_g_assets.source_count];
        s->type = ASSET_SOURCE_ZIP;
        const SsrnxResult rc = assets_zip_open_index(&s->zip, apk_path);
        if (rc != SSRNX_OK) { ssrnx_assets_reset(); return rc; }
        (void)assets_copy_path(s->path, sizeof(s->path), apk_path);
        assets_g_assets.source_count++;
    }
    return SSRNX_OK;
}

SsrnxAssetManager *ssrnx_assets_manager(void) { return assets_g_assets.initialized ? &assets_g_assets : NULL; }
size_t ssrnx_assets_backend_count(void) {
    pthread_mutex_lock(&assets_g_assets_lock);
    const size_t count = assets_g_assets.source_count;
    pthread_mutex_unlock(&assets_g_assets_lock);
    return count;
}
size_t ssrnx_assets_zip_entry_count(void) {
    size_t count = 0;
    pthread_mutex_lock(&assets_g_assets_lock);
    for (size_t i = 0; i < assets_g_assets.source_count; ++i) {
        if (assets_g_assets.sources[i].type == ASSET_SOURCE_ZIP) {
            count = assets_g_assets.sources[i].zip.entry_count;
            break;
        }
    }
    pthread_mutex_unlock(&assets_g_assets_lock);
    return count;
}

bool ssrnx_assets_set_configuration(SsrnxAssetManager *manager,
                                    const SsrnxAssetConfiguration *configuration) {
    if (!manager || !configuration) return false;
    pthread_mutex_lock(&assets_g_assets_lock);
    const bool valid = manager == &assets_g_assets && manager->initialized;
    if (valid) {
        manager->configuration = *configuration;
        manager->configuration_valid = true;
    }
    pthread_mutex_unlock(&assets_g_assets_lock);
    return valid;
}

bool ssrnx_assets_get_configuration(const SsrnxAssetManager *manager,
                                    SsrnxAssetConfiguration *out_configuration) {
    if (!manager || !out_configuration) return false;
    pthread_mutex_lock(&assets_g_assets_lock);
    const bool valid = manager == &assets_g_assets && manager->initialized && manager->configuration_valid;
    if (valid) *out_configuration = manager->configuration;
    pthread_mutex_unlock(&assets_g_assets_lock);
    return valid;
}

typedef struct assets_AssetOpenCandidate {
    assets_AssetSourceType type;
    char path[SSRNX_ASSET_PATH_CAP];
    assets_ZipEntry entry; /* value snapshot; name is not used */
} assets_AssetOpenCandidate;

/* Sources are resolved under g_assets_lock, then files are read without it:
 * an open used to hold the manager lock across card I/O and decompression,
 * serializing every asset open, read-dir and close in the process. */
SsrnxAsset *ssrnx_asset_open(SsrnxAssetManager *manager, const char *filename, int mode) {
    if (!manager || mode < 0 || mode > 3) return NULL;
    char name[SSRNX_ASSET_PATH_CAP];
    if (!assets_normalize_asset_path(filename, name, sizeof(name), false)) return NULL;
    SsrnxAsset *asset = assets_asset_alloc(manager);
    if (!asset) return NULL;
    assets_AssetOpenCandidate candidates[sizeof(manager->sources) / sizeof(manager->sources[0])];
    size_t candidate_count = 0;
    pthread_mutex_lock(&assets_g_assets_lock);
    if (manager != &assets_g_assets || !manager->initialized) {
        pthread_mutex_unlock(&assets_g_assets_lock);
        free(asset);
        return NULL;
    }
    for (size_t i = 0; i < manager->source_count; ++i) {
        assets_AssetSource *source = &manager->sources[i];
        assets_AssetOpenCandidate *c = &candidates[candidate_count];
        c->type = source->type;
        if (source->type == ASSET_SOURCE_DIRECTORY) {
            memcpy(c->path, source->path, sizeof(c->path));
            candidate_count++;
        } else {
            const assets_ZipEntry *entry = assets_zip_find(&source->zip, name);
            if (!entry) continue;
            memcpy(c->path, source->zip.path, sizeof(c->path));
            c->entry = *entry;
            c->entry.name = NULL;
            c->entry.next = NULL;
            candidate_count++;
        }
    }
    pthread_mutex_unlock(&assets_g_assets_lock);

    const char *found_in = NULL;
    for (size_t i = 0; i < candidate_count && !found_in; ++i) {
        assets_AssetOpenCandidate *c = &candidates[i];
        if (c->type == ASSET_SOURCE_DIRECTORY) {
            assets_AssetSource directory = { .type = ASSET_SOURCE_DIRECTORY };
            memcpy(directory.path, c->path, sizeof(directory.path));
            if (assets_load_directory_asset(&directory, name, asset)) found_in = "directory";
        } else {
            uint8_t *data = NULL; size_t len = 0; bool fd_ok = false; int64_t start = 0;
            if (assets_load_zip_entry(c->path, &c->entry, &data, &len, &fd_ok, &start)) {
                asset->data = data; asset->length = len; asset->fd_supported = fd_ok; asset->fd_start = start; asset->fd_length = (int64_t)len;
                if (fd_ok) (void)assets_copy_path(asset->fd_path, sizeof(asset->fd_path), c->path);
                found_in = "apk";
            }
        }
    }
    if (found_in) {
        pthread_mutex_lock(&assets_g_assets_lock);
        const bool live = manager == &assets_g_assets && manager->initialized;
        if (live) {
            asset->next = manager->assets;
            manager->assets = asset;
        }
        pthread_mutex_unlock(&assets_g_assets_lock);
        if (!live) { free(asset->data); free(asset); return NULL; }
        if (assets_asset_full_trace() || atomic_fetch_add(&assets_g_asset_open_reports, 1u) < 64u)
            SSRNX_LOGCI("ASSET", "open name=%s mode=%d source=%s bytes=%zu", name, mode, found_in, asset->length);
        else if (assets_font_asset_trace_once(name))
            SSRNX_LOGCI("ASSET", "font asset open name=%s source=%s bytes=%zu", name, found_in, asset->length);
        return asset;
    }
    if (assets_asset_full_trace() || atomic_fetch_add(&assets_g_asset_missing_reports, 1u) < 64u)
        SSRNX_LOGCW("ASSET", "open failed name=%s mode=%d candidates=%zu", name, mode, candidate_count);
    else if (assets_font_asset_trace_once(name)) SSRNX_LOGCW("ASSET", "font asset missing name=%s", name);
    free(asset->data);
    free(asset);
    return NULL;
}

int ssrnx_asset_read(SsrnxAsset *asset, void *buffer, size_t count) {
    if (!asset || (!buffer && count)) return -1;
    const size_t remain = asset->length - asset->offset;
    if (count > remain) count = remain;
    if (count > (size_t)INT_MAX) count = (size_t)INT_MAX;
    if (count) memcpy(buffer, asset->data + asset->offset, count);
    asset->offset += count;
    return count > INT32_MAX ? INT32_MAX : (int)count;
}

int64_t ssrnx_asset_seek64(SsrnxAsset *asset, int64_t offset, int whence) {
    if (!asset) return -1;
    int64_t base = 0;
    if (whence == SEEK_SET) base = 0;
    else if (whence == SEEK_CUR) base = (int64_t)asset->offset;
    else if (whence == SEEK_END) base = (int64_t)asset->length;
    else return -1;
    if (offset > 0 && base > INT64_MAX - offset) return -1;
    if (offset < 0 && offset < -base) return -1;
    const int64_t next = base + offset;
    if (next < 0 || (uint64_t)next > asset->length) return -1;
    asset->offset = (size_t)next;
    return next;
}

int64_t ssrnx_asset_length64(const SsrnxAsset *asset) { return asset ? (int64_t)asset->length : -1; }
int64_t ssrnx_asset_remaining64(const SsrnxAsset *asset) { return asset ? (int64_t)(asset->length - asset->offset) : -1; }
const void *ssrnx_asset_buffer(SsrnxAsset *asset) { return asset ? asset->data : NULL; }
int ssrnx_asset_is_allocated(const SsrnxAsset *asset) { return asset && asset->allocated ? 1 : 0; }

const char *ssrnx_asset_fd_host_path(const SsrnxAsset *asset) {
    return asset && asset->fd_supported && asset->fd_path[0] ? asset->fd_path : NULL;
}

int ssrnx_asset_open_fd64(SsrnxAsset *asset, int64_t *start, int64_t *length) {
    if (!asset || !asset->fd_supported || !asset->fd_path[0] || !start || !length) return -1;
    const int fd = open(asset->fd_path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return -1;
    *start = asset->fd_start; *length = asset->fd_length;
    return fd;
}

void ssrnx_asset_close(SsrnxAsset *asset) {
    if (!asset || !asset->manager) return;
    pthread_mutex_lock(&assets_g_assets_lock);
    SsrnxAsset **link = &asset->manager->assets;
    while (*link && *link != asset) link = &(*link)->next;
    if (*link == asset) *link = asset->next;
    pthread_mutex_unlock(&assets_g_assets_lock);
    free(asset->data); asset->data = NULL; free(asset);
}

static bool assets_add_name(char ***names, size_t *count, const char *name, size_t len) {
    if (!names || !count || !name || !len) return false;
    for (size_t i = 0; i < *count; ++i) if (strlen((*names)[i]) == len && strncmp((*names)[i], name, len) == 0) return true;
    char *copy = malloc(len + 1);
    char **grown = realloc(*names, (*count + 1) * sizeof(char *));
    if (!copy || !grown) { free(copy); return false; }
    memcpy(copy, name, len); copy[len] = '\0';
    *names = grown; (*names)[(*count)++] = copy; return true;
}

static bool assets_collect_dir_source(assets_AssetSource *source, const char *dir, char ***names, size_t *count) {
    char path[SSRNX_ASSET_PATH_CAP];
    if (dir[0]) { if (!assets_join_path(path, sizeof(path), source->path, dir)) return false; }
    else if (!assets_copy_path(path, sizeof(path), source->path)) return false;
    DIR *dp = opendir(path);
    if (!dp) return true;
    struct dirent *ent;
    while ((ent = readdir(dp)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        if (!assets_add_name(names, count, ent->d_name, strlen(ent->d_name))) { closedir(dp); return false; }
    }
    closedir(dp); return true;
}

static bool assets_collect_zip_source(assets_AssetSource *source, const char *dir, char ***names, size_t *count) {
    char prefix[SSRNX_ASSET_PATH_CAP];
    size_t plen = 0;
    if (dir[0]) {
        const size_t n = strlen(dir); if (n + 1 >= sizeof(prefix)) return false;
        memcpy(prefix, dir, n); prefix[n] = '/'; prefix[n + 1] = '\0'; plen = n + 1;
    } else prefix[0] = '\0';
    for (assets_ZipEntry *e = source->zip.entries; e; e = e->next) {
        if (strncmp(e->name, prefix, plen) != 0) continue;
        const char *rest = e->name + plen;
        if (!rest[0]) continue;
        const char *slash = strchr(rest, '/');
        const size_t len = slash ? (size_t)(slash - rest) : strlen(rest);
        if (!assets_add_name(names, count, rest, len)) return false;
    }
    return true;
}

static int assets_cmp_names(const void *a, const void *b) { return strcmp(*(const char *const *)a, *(const char *const *)b); }

SsrnxAssetDir *ssrnx_asset_open_dir(SsrnxAssetManager *manager, const char *dirname) {
    if (!manager) return NULL;
    char dir[SSRNX_ASSET_PATH_CAP];
    if (!assets_normalize_asset_path(dirname ? dirname : "", dir, sizeof(dir), true)) return NULL;
    SsrnxAssetDir *out = calloc(1, sizeof(*out));
    if (!out) return NULL;
    out->manager = manager;
    pthread_mutex_lock(&assets_g_assets_lock);
    if (manager != &assets_g_assets || !manager->initialized) {
        pthread_mutex_unlock(&assets_g_assets_lock);
        free(out);
        return NULL;
    }
    for (size_t i = 0; i < manager->source_count; ++i) {
        bool ok = manager->sources[i].type == ASSET_SOURCE_DIRECTORY
            ? assets_collect_dir_source(&manager->sources[i], dir, &out->names, &out->count)
            : assets_collect_zip_source(&manager->sources[i], dir, &out->names, &out->count);
        if (!ok) {
            pthread_mutex_unlock(&assets_g_assets_lock);
            for (size_t j = 0; j < out->count; ++j) free(out->names[j]);
            free(out->names);
            free(out);
            return NULL;
        }
    }
    if (out->count > 1) qsort(out->names, out->count, sizeof(char *), assets_cmp_names);
    out->next = manager->dirs; manager->dirs = out;
    pthread_mutex_unlock(&assets_g_assets_lock);
    return out;
}

const char *ssrnx_asset_dir_next(SsrnxAssetDir *dir) { return dir && dir->index < dir->count ? dir->names[dir->index++] : NULL; }
void ssrnx_asset_dir_rewind(SsrnxAssetDir *dir) { if (dir) dir->index = 0; }
void ssrnx_asset_dir_close(SsrnxAssetDir *dir) {
    if (!dir) return;
    if (dir->manager) {
        pthread_mutex_lock(&assets_g_assets_lock);
        SsrnxAssetDir **link=&dir->manager->dirs; while(*link&&*link!=dir)link=&(*link)->next; if(*link==dir)*link=dir->next;
        pthread_mutex_unlock(&assets_g_assets_lock);
    }
    for (size_t i = 0; i < dir->count; ++i) free(dir->names[i]);
    free(dir->names);
    free(dir);
}

void ssrnx_assets_reset(void) {
    while (assets_g_assets.assets) ssrnx_asset_close(assets_g_assets.assets);
    while (assets_g_assets.dirs) ssrnx_asset_dir_close(assets_g_assets.dirs);
    for (size_t i=0;i<assets_g_assets.source_count;++i) if(assets_g_assets.sources[i].type==ASSET_SOURCE_ZIP) assets_zip_free(&assets_g_assets.sources[i].zip);
    memset(&assets_g_assets,0,sizeof(assets_g_assets));
    pthread_mutex_lock(&assets_g_font_trace_lock);
    memset(assets_g_font_asset_trace_hashes, 0, sizeof(assets_g_font_asset_trace_hashes));
    assets_g_font_asset_trace_count = 0;
    pthread_mutex_unlock(&assets_g_font_trace_lock);
}

/* ---- Android NDK ABI shims -------------------------------------------------- */
void *AAssetManager_fromJava(void *env, void *asset_manager_object) {
    (void)env;
    void *manager = ssrnx_jni_get_object_host_data(asset_manager_object);
    return manager ? manager : NULL;
}
SsrnxAsset *AAssetManager_open(SsrnxAssetManager *m,const char*n,int mode){return ssrnx_asset_open(m,n,mode);}
SsrnxAssetDir *AAssetManager_openDir(SsrnxAssetManager*m,const char*n){return ssrnx_asset_open_dir(m,n);}
const char *AAssetDir_getNextFileName(SsrnxAssetDir*d){return ssrnx_asset_dir_next(d);}
void AAssetDir_rewind(SsrnxAssetDir*d){ssrnx_asset_dir_rewind(d);}
void AAssetDir_close(SsrnxAssetDir*d){ssrnx_asset_dir_close(d);}
int AAsset_read(SsrnxAsset*a,void*b,size_t n){return ssrnx_asset_read(a,b,n);}
int64_t AAsset_seek(SsrnxAsset*a,int64_t o,int w){return ssrnx_asset_seek64(a,o,w);}
int64_t AAsset_seek64(SsrnxAsset*a,int64_t o,int w){return ssrnx_asset_seek64(a,o,w);}
int64_t AAsset_getLength(SsrnxAsset*a){return ssrnx_asset_length64(a);}
int64_t AAsset_getLength64(SsrnxAsset*a){return ssrnx_asset_length64(a);}
int64_t AAsset_getRemainingLength(SsrnxAsset*a){return ssrnx_asset_remaining64(a);}
int64_t AAsset_getRemainingLength64(SsrnxAsset*a){return ssrnx_asset_remaining64(a);}
const void *AAsset_getBuffer(SsrnxAsset*a){return ssrnx_asset_buffer(a);}
int AAsset_openFileDescriptor(SsrnxAsset*a,int64_t*s,int64_t*l){return ssrnx_asset_open_fd64(a,s,l);}
int AAsset_openFileDescriptor64(SsrnxAsset*a,int64_t*s,int64_t*l){return ssrnx_asset_open_fd64(a,s,l);}
int AAsset_isAllocated(SsrnxAsset*a){return ssrnx_asset_is_allocated(a);}
void AAsset_close(SsrnxAsset*a){ssrnx_asset_close(a);}

/* Stream over bytes not owned by an asset source (class-path resources read
 * from APK ZIP entries). Takes ownership of data; ssrnx_asset_close frees it. */
SsrnxAsset *ssrnx_asset_from_owned_buffer(uint8_t *data, size_t length) {
    SsrnxAsset *asset = assets_asset_alloc(&assets_g_assets);
    if (!asset) { free(data); return NULL; }
    asset->data = data;
    asset->length = length;
    pthread_mutex_lock(&assets_g_assets_lock);
    asset->next = assets_g_assets.assets;
    assets_g_assets.assets = asset;
    pthread_mutex_unlock(&assets_g_assets_lock);
    return asset;
}

#pragma pop_macro("ZIP_METHOD_STORE")
#pragma pop_macro("ZIP_METHOD_DEFLATE")
#pragma pop_macro("ZIP_LOCAL_SIG")
#pragma pop_macro("ZIP_EOCD_SIG")
#pragma pop_macro("ZIP_CENTRAL_SIG")
#pragma pop_macro("O_CLOEXEC")
#pragma pop_macro("SSRNX_ASSET_PATH_CAP")


/* ---- framework_resources ---- */

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <zlib.h>

#if __has_include("framework_resources_data.inc")
#include "framework_resources_data.inc"
static pthread_mutex_t framework_resources_table_lock=PTHREAD_MUTEX_INITIALIZER;
static uint8_t *framework_resources_framework_table;
#endif

/* Immutable process-wide provider; package tables retain their own string pools. */
SsrnxResult ssrnx_framework_resource_table(const uint8_t **data,size_t *size) {
    if(!data||!size)return SSRNX_ERR_ARGUMENT;
    *data=NULL;*size=0;
#if __has_include("framework_resources_data.inc")
    pthread_mutex_lock(&framework_resources_table_lock);
    SsrnxResult rc=SSRNX_OK;
    if(!framework_resources_framework_table){
        uint8_t *buffer=malloc(framework_table_size);
        if(!buffer)rc=SSRNX_ERR_NO_MEMORY;
        else if(!ssrnx_platform_decode(framework_table_xz,sizeof(framework_table_xz),buffer,framework_table_size)){free(buffer);rc=SSRNX_ERR_APK_INVALID;}
        else framework_resources_framework_table=buffer;
    }
    if(rc==SSRNX_OK){*data=framework_resources_framework_table;*size=framework_table_size;}
    pthread_mutex_unlock(&framework_resources_table_lock);
    return rc;
#else
    return SSRNX_ERR_APK_UNSUPPORTED;
#endif
}

