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
    uint32_t h[5];
    uint8_t buf[64];
    uint64_t len;
} DSHA1_CTX;

// ============== FORCE INLINE ==============
#if defined(__GNUC__) || defined(__clang__)
    #define ALWAYS_INLINE __attribute__((always_inline)) inline
    #define RESTRICT __restrict
    #define LIKELY(x)   __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define ALWAYS_INLINE inline
    #define RESTRICT
    #define LIKELY(x)   (x)
    #define UNLIKELY(x) (x)
#endif

// ============== ENDIANNESS (FASTEST POSSIBLE) ==============
#if defined(__ARM_NEON__) || defined(__aarch64__)
    static ALWAYS_INLINE uint32_t bswap32(uint32_t x) {
        uint32_t y;
        __asm__("rev %0, %1" : "=r"(y) : "r"(x));
        return y;
    }
    static ALWAYS_INLINE uint64_t bswap64(uint64_t x) {
        uint64_t y;
        __asm__("rev %0, %1" : "=r"(y) : "r"(x));
        return y;
    }
#elif defined(__i386__) || defined(__x86_64__)
    static ALWAYS_INLINE uint32_t bswap32(uint32_t x) {
        __asm__("bswap %0" : "=r"(x) : "0"(x));
        return x;
    }
    static ALWAYS_INLINE uint64_t bswap64(uint64_t x) {
        __asm__("bswap %0" : "=r"(x) : "0"(x));
        return x;
    }
#else
    static ALWAYS_INLINE uint32_t bswap32(uint32_t x) {
        return __builtin_bswap32(x);
    }
    static ALWAYS_INLINE uint64_t bswap64(uint64_t x) {
        return __builtin_bswap64(x);
    }
#endif

// ============== LOAD/STORE WITH PREFETCH ==============
static ALWAYS_INLINE uint32_t LOAD32(const uint8_t* RESTRICT p) {
    uint32_t v;
    __builtin_memcpy(&v, p, 4);
    return bswap32(v);
}

static ALWAYS_INLINE void STORE32(uint8_t* RESTRICT p, uint32_t v) {
    v = bswap32(v);
    __builtin_memcpy(p, &v, 4);
}

static ALWAYS_INLINE void STORE64(uint8_t* RESTRICT p, uint64_t v) {
    v = bswap64(v);
    __builtin_memcpy(p, &v, 8);
}

// ============== SHA-1 CORE MACROS (RUST-STYLE) ==============
#define ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define CH(x, y, z)  ((z) ^ ((x) & ((y) ^ (z))))
#define PARITY(x, y, z) ((x) ^ (y) ^ (z))
#define MAJ(x, y, z) (((x) & (y)) | ((z) & ((x) | (y))))

#define K0 0x5A827999UL
#define K1 0x6ED9EBA1UL
#define K2 0x8F1BBCDCUL
#define K3 0xCA62C1D6UL

// ============== TRANSFORM - RUST-LEVEL OPTIMIZATION ==============
static ALWAYS_INLINE void transform(uint32_t h[5], const uint8_t* RESTRICT blk) {
    uint32_t a, b, c, d, e;
    uint32_t w[80];
    
    // Load with vector-friendly pattern
    for (int i = 0; i < 16; i++) {
        w[i] = LOAD32(blk + (i << 2));
    }
    
    // Schedule expansion - unrolled for speed
    #pragma GCC unroll 4
    for (int i = 16; i < 80; i++) {
        w[i] = ROTL(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    }
    
    a = h[0]; b = h[1]; c = h[2]; d = h[3]; e = h[4];
    
    // Round 0-19
    #pragma GCC unroll 20
    for (int i = 0; i < 20; i++) {
        uint32_t temp = ROTL(a, 5) + CH(b, c, d) + e + K0 + w[i];
        e = d; d = c; c = ROTL(b, 30); b = a; a = temp;
    }
    
    // Round 20-39
    #pragma GCC unroll 20
    for (int i = 20; i < 40; i++) {
        uint32_t temp = ROTL(a, 5) + PARITY(b, c, d) + e + K1 + w[i];
        e = d; d = c; c = ROTL(b, 30); b = a; a = temp;
    }
    
    // Round 40-59
    #pragma GCC unroll 20
    for (int i = 40; i < 60; i++) {
        uint32_t temp = ROTL(a, 5) + MAJ(b, c, d) + e + K2 + w[i];
        e = d; d = c; c = ROTL(b, 30); b = a; a = temp;
    }
    
    // Round 60-79
    #pragma GCC unroll 20
    for (int i = 60; i < 80; i++) {
        uint32_t temp = ROTL(a, 5) + PARITY(b, c, d) + e + K3 + w[i];
        e = d; d = c; c = ROTL(b, 30); b = a; a = temp;
    }
    
    h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
}

// ============== PUBLIC API ==============
static ALWAYS_INLINE void dsha1_init(DSHA1_CTX* RESTRICT ctx) {
    ctx->h[0] = 0x67452301UL;
    ctx->h[1] = 0xEFCDAB89UL;
    ctx->h[2] = 0x98BADCFEUL;
    ctx->h[3] = 0x10325476UL;
    ctx->h[4] = 0xC3D2E1F0UL;
    ctx->len = 0;
}

static ALWAYS_INLINE void dsha1_update(DSHA1_CTX* RESTRICT ctx, 
                                        const uint8_t* RESTRICT data, 
                                        size_t len) {
    size_t i = ctx->len & 63;
    ctx->len += len;
    
    if (LIKELY(i)) {
        size_t n = 64 - i;
        if (n > len) n = len;
        __builtin_memcpy(ctx->buf + i, data, n);
        data += n;
        len -= n;
        if (UNLIKELY(i + n == 64)) {
            transform(ctx->h, ctx->buf);
        }
    }
    
    while (LIKELY(len >= 64)) {
        transform(ctx->h, data);
        data += 64;
        len -= 64;
    }
    
    if (UNLIKELY(len)) {
        __builtin_memcpy(ctx->buf, data, len);
    }
}

static ALWAYS_INLINE void dsha1_final(DSHA1_CTX* RESTRICT ctx, 
                                       uint8_t hash[RESTRICT 20]) {
    uint64_t bits = ctx->len << 3;
    size_t i = ctx->len & 63;
    
    ctx->buf[i++] = 0x80;
    
    if (UNLIKELY(i > 56)) {
        __builtin_memset(ctx->buf + i, 0, 64 - i);
        transform(ctx->h, ctx->buf);
        __builtin_memset(ctx->buf, 0, 56);
    } else {
        __builtin_memset(ctx->buf + i, 0, 56 - i);
    }
    
    STORE64(ctx->buf + 56, bits);
    transform(ctx->h, ctx->buf);
    
    STORE32(hash,      ctx->h[0]);
    STORE32(hash + 4,  ctx->h[1]);
    STORE32(hash + 8,  ctx->h[2]);
    STORE32(hash + 12, ctx->h[3]);
    STORE32(hash + 16, ctx->h[4]);
}

static ALWAYS_INLINE void dsha1_reset(DSHA1_CTX* RESTRICT ctx) {
    dsha1_init(ctx);
}

static ALWAYS_INLINE void dsha1_warmup(DSHA1_CTX* RESTRICT ctx) {
    uint8_t tmp[20];
    dsha1_update(ctx, (const uint8_t*)"warmupwarmupwa", 20);
    dsha1_final(ctx, tmp);
}

#ifdef __cplusplus
}
#endif

#endif
