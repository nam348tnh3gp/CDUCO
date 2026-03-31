#ifndef DSHA1_H
#define DSHA1_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DSHA1_OUTPUT_SIZE 20

typedef struct {
    uint32_t s[5];
    uint8_t buf[64];
    uint64_t bytes;
} DSHA1_CTX;

// Core functions
void dsha1_init(DSHA1_CTX *ctx);
void dsha1_update(DSHA1_CTX *ctx, const uint8_t *data, size_t len);
void dsha1_final(DSHA1_CTX *ctx, uint8_t hash[DSHA1_OUTPUT_SIZE]);
void dsha1_reset(DSHA1_CTX *ctx);
void dsha1_warmup(DSHA1_CTX *ctx);

// Inline implementation
static inline void dsha1_initialize(uint32_t s[5]) {
    s[0] = 0x67452301UL;
    s[1] = 0xEFCDAB89UL;
    s[2] = 0x98BADCFEUL;
    s[3] = 0x10325476UL;
    s[4] = 0xC3D2E1F0UL;
}

// Endianness macros
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define DSHA1_BSWAP32(x) __builtin_bswap32(x)
    #define DSHA1_BSWAP64(x) __builtin_bswap64(x)
#elif defined(_MSC_VER)
    #define DSHA1_BSWAP32(x) _byteswap_ulong(x)
    #define DSHA1_BSWAP64(x) _byteswap_uint64(x)
#else
    static inline uint32_t dsha1_bswap32(uint32_t x) {
        return ((x & 0xFF000000) >> 24) | ((x & 0x00FF0000) >> 8) |
               ((x & 0x0000FF00) << 8) | ((x & 0x000000FF) << 24);
    }
    static inline uint64_t dsha1_bswap64(uint64_t x) {
        return ((x & 0xFF00000000000000ULL) >> 56) |
               ((x & 0x00FF000000000000ULL) >> 40) |
               ((x & 0x0000FF0000000000ULL) >> 24) |
               ((x & 0x000000FF00000000ULL) >> 8) |
               ((x & 0x00000000FF000000ULL) << 8) |
               ((x & 0x0000000000FF0000ULL) << 24) |
               ((x & 0x000000000000FF00ULL) << 40) |
               ((x & 0x00000000000000FFULL) << 56);
    }
    #define DSHA1_BSWAP32(x) dsha1_bswap32(x)
    #define DSHA1_BSWAP64(x) dsha1_bswap64(x)
#endif

static inline uint32_t dsha1_readBE32(const uint8_t *p) {
    uint32_t v;
    memcpy(&v, p, 4);
    return DSHA1_BSWAP32(v);
}

static inline void dsha1_writeBE32(uint8_t *p, uint32_t v) {
    uint32_t tmp = DSHA1_BSWAP32(v);
    memcpy(p, &tmp, 4);
}

static inline void dsha1_writeBE64(uint8_t *p, uint64_t v) {
    uint64_t tmp = DSHA1_BSWAP64(v);
    memcpy(p, &tmp, 8);
}

static inline uint32_t dsha1_f1(uint32_t b, uint32_t c, uint32_t d) {
    return d ^ (b & (c ^ d));
}

static inline uint32_t dsha1_f2(uint32_t b, uint32_t c, uint32_t d) {
    return b ^ c ^ d;
}

static inline uint32_t dsha1_f3(uint32_t b, uint32_t c, uint32_t d) {
    return (b & c) | (d & (b | c));
}

static inline uint32_t dsha1_left_rotate(uint32_t x, uint32_t n) {
    return (x << n) | (x >> (32 - n));
}

#define DSHA1_ROUND(a, b, c, d, e, f, k, w) \
    do { \
        e += dsha1_left_rotate(a, 5) + f(b, c, d) + (k) + (w); \
        b = dsha1_left_rotate(b, 30); \
    } while(0)

static inline void dsha1_transform(uint32_t *s, const uint8_t *chunk) {
    uint32_t a = s[0], b = s[1], c = s[2], d = s[3], e = s[4];
    uint32_t w[16];
    int i;
    
    for (i = 0; i < 16; i++) {
        w[i] = dsha1_readBE32(chunk + (i << 2));
    }
    
    // Round 1
    DSHA1_ROUND(a, b, c, d, e, dsha1_f1, 0x5A827999UL, w[0]);
    DSHA1_ROUND(e, a, b, c, d, dsha1_f1, 0x5A827999UL, w[1]);
    DSHA1_ROUND(d, e, a, b, c, dsha1_f1, 0x5A827999UL, w[2]);
    DSHA1_ROUND(c, d, e, a, b, dsha1_f1, 0x5A827999UL, w[3]);
    DSHA1_ROUND(b, c, d, e, a, dsha1_f1, 0x5A827999UL, w[4]);
    DSHA1_ROUND(a, b, c, d, e, dsha1_f1, 0x5A827999UL, w[5]);
    DSHA1_ROUND(e, a, b, c, d, dsha1_f1, 0x5A827999UL, w[6]);
    DSHA1_ROUND(d, e, a, b, c, dsha1_f1, 0x5A827999UL, w[7]);
    DSHA1_ROUND(c, d, e, a, b, dsha1_f1, 0x5A827999UL, w[8]);
    DSHA1_ROUND(b, c, d, e, a, dsha1_f1, 0x5A827999UL, w[9]);
    DSHA1_ROUND(a, b, c, d, e, dsha1_f1, 0x5A827999UL, w[10]);
    DSHA1_ROUND(e, a, b, c, d, dsha1_f1, 0x5A827999UL, w[11]);
    DSHA1_ROUND(d, e, a, b, c, dsha1_f1, 0x5A827999UL, w[12]);
    DSHA1_ROUND(c, d, e, a, b, dsha1_f1, 0x5A827999UL, w[13]);
    DSHA1_ROUND(b, c, d, e, a, dsha1_f1, 0x5A827999UL, w[14]);
    DSHA1_ROUND(a, b, c, d, e, dsha1_f1, 0x5A827999UL, w[15]);
    
    // Round 2-4 compressed
    for (i = 16; i < 80; i++) {
        uint32_t tmp = w[(i-3) & 15] ^ w[(i-8) & 15] ^ w[(i-14) & 15] ^ w[(i-16) & 15];
        w[i & 15] = dsha1_left_rotate(tmp, 1);
        
        uint32_t f, k;
        if (i < 20) {
            f = dsha1_f1(b, c, d);
            k = 0x5A827999UL;
            DSHA1_ROUND(a, b, c, d, e, dsha1_f1, k, w[i & 15]);
        } else if (i < 40) {
            f = dsha1_f2(b, c, d);
            k = 0x6ED9EBA1UL;
            DSHA1_ROUND(a, b, c, d, e, dsha1_f2, k, w[i & 15]);
        } else if (i < 60) {
            f = dsha1_f3(b, c, d);
            k = 0x8F1BBCDCUL;
            DSHA1_ROUND(a, b, c, d, e, dsha1_f3, k, w[i & 15]);
        } else {
            f = dsha1_f2(b, c, d);
            k = 0xCA62C1D6UL;
            DSHA1_ROUND(a, b, c, d, e, dsha1_f2, k, w[i & 15]);
        }
        
        // Rotate registers
        uint32_t temp = e;
        e = d;
        d = c;
        c = dsha1_left_rotate(b, 30);
        b = a;
        a = temp;
    }
    
    s[0] += a;
    s[1] += b;
    s[2] += c;
    s[3] += d;
    s[4] += e;
}

static inline void dsha1_init(DSHA1_CTX *ctx) {
    dsha1_initialize(ctx->s);
    ctx->bytes = 0;
    memset(ctx->buf, 0, sizeof(ctx->buf));
}

static inline void dsha1_update(DSHA1_CTX *ctx, const uint8_t *data, size_t len) {
    size_t bufsize = ctx->bytes % 64;
    
    if (bufsize && bufsize + len >= 64) {
        memcpy(ctx->buf + bufsize, data, 64 - bufsize);
        ctx->bytes += 64 - bufsize;
        data += 64 - bufsize;
        dsha1_transform(ctx->s, ctx->buf);
        bufsize = 0;
    }
    
    while (len >= 64) {
        dsha1_transform(ctx->s, data);
        ctx->bytes += 64;
        data += 64;
        len -= 64;
    }
    
    if (len > 0) {
        memcpy(ctx->buf + bufsize, data, len);
        ctx->bytes += len;
    }
}

static inline void dsha1_final(DSHA1_CTX *ctx, uint8_t hash[DSHA1_OUTPUT_SIZE]) {
    const uint8_t pad = 0x80;
    uint8_t sizedesc[8];
    size_t padlen;
    
    dsha1_writeBE64(sizedesc, ctx->bytes << 3);
    padlen = 1 + ((119 - (ctx->bytes % 64)) % 64);
    
    dsha1_update(ctx, &pad, 1);
    if (padlen > 1) {
        uint8_t zeros[64] = {0};
        dsha1_update(ctx, zeros, padlen - 1);
    }
    dsha1_update(ctx, sizedesc, 8);
    
    dsha1_writeBE32(hash, ctx->s[0]);
    dsha1_writeBE32(hash + 4, ctx->s[1]);
    dsha1_writeBE32(hash + 8, ctx->s[2]);
    dsha1_writeBE32(hash + 12, ctx->s[3]);
    dsha1_writeBE32(hash + 16, ctx->s[4]);
}

static inline void dsha1_reset(DSHA1_CTX *ctx) {
    ctx->bytes = 0;
    dsha1_initialize(ctx->s);
    memset(ctx->buf, 0, sizeof(ctx->buf));
}

static inline void dsha1_warmup(DSHA1_CTX *ctx) {
    uint8_t warmup_hash[DSHA1_OUTPUT_SIZE];
    dsha1_update(ctx, (const uint8_t *)"warmupwarmupwa", 20);
    dsha1_final(ctx, warmup_hash);
    (void)warmup_hash;
}

#ifdef __cplusplus
}
#endif

#endif // DSHA1_H
