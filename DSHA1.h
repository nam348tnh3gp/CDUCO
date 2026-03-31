#ifndef SHA1_H
#define SHA1_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHA1_SIZE 20
#define SHA1_SIZE_FORMATTED 41

typedef struct {
    uint32_t h[5];
    uint64_t len;
    uint8_t buf[64];
    uint32_t buf_len;
} sha1_ctx;

// Force inline macros
#if defined(__GNUC__) || defined(__clang__)
    #define SHA1_INLINE __attribute__((always_inline)) static inline
    #define SHA1_RESTRICT __restrict
    #define SHA1_LIKELY(x) __builtin_expect(!!(x), 1)
    #define SHA1_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define SHA1_INLINE static inline
    #define SHA1_RESTRICT
    #define SHA1_LIKELY(x) (x)
    #define SHA1_UNLIKELY(x) (x)
#endif

// Byte swap for little-endian
#if defined(__ARM_NEON__) || defined(__aarch64__)
    SHA1_INLINE uint32_t sha1_bswap32(uint32_t x) {
        uint32_t y;
        __asm__("rev %0, %1" : "=r"(y) : "r"(x));
        return y;
    }
#elif defined(__i386__) || defined(__x86_64__)
    SHA1_INLINE uint32_t sha1_bswap32(uint32_t x) {
        __asm__("bswap %0" : "=r"(x) : "0"(x));
        return x;
    }
#else
    SHA1_INLINE uint32_t sha1_bswap32(uint32_t x) {
        return __builtin_bswap32(x);
    }
#endif

// Load/Store with builtins
SHA1_INLINE uint32_t sha1_load32(const uint8_t* SHA1_RESTRICT p) {
    uint32_t v;
    __builtin_memcpy(&v, p, 4);
    return sha1_bswap32(v);
}

SHA1_INLINE void sha1_store32(uint8_t* SHA1_RESTRICT p, uint32_t v) {
    v = sha1_bswap32(v);
    __builtin_memcpy(p, &v, 4);
}

// SHA-1 core macros
#define SHA1_ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define SHA1_CH(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define SHA1_PARITY(x, y, z) ((x) ^ (y) ^ (z))
#define SHA1_MAJ(x, y, z) (((x) & (y)) | ((z) & ((x) | (y))))

#define K0 0x5A827999UL
#define K1 0x6ED9EBA1UL
#define K2 0x8F1BBCDCUL
#define K3 0xCA62C1D6UL

// Optimized transform - fully unrolled loops
static void sha1_transform(sha1_ctx* SHA1_RESTRICT ctx, const uint8_t* SHA1_RESTRICT block) {
    uint32_t a, b, c, d, e;
    uint32_t w[80];
    
    // Load with prefetch
    for (int i = 0; i < 16; i++) {
        w[i] = sha1_load32(block + (i << 2));
    }
    
    // Schedule expansion - unrolled with pragma
    #pragma GCC unroll 4
    for (int i = 16; i < 80; i++) {
        w[i] = SHA1_ROTL(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    }
    
    a = ctx->h[0]; b = ctx->h[1]; c = ctx->h[2]; d = ctx->h[3]; e = ctx->h[4];
    
    // Round 0-19
    #pragma GCC unroll 20
    for (int i = 0; i < 20; i++) {
        uint32_t temp = SHA1_ROTL(a, 5) + SHA1_CH(b, c, d) + e + K0 + w[i];
        e = d; d = c; c = SHA1_ROTL(b, 30); b = a; a = temp;
    }
    
    // Round 20-39
    #pragma GCC unroll 20
    for (int i = 20; i < 40; i++) {
        uint32_t temp = SHA1_ROTL(a, 5) + SHA1_PARITY(b, c, d) + e + K1 + w[i];
        e = d; d = c; c = SHA1_ROTL(b, 30); b = a; a = temp;
    }
    
    // Round 40-59
    #pragma GCC unroll 20
    for (int i = 40; i < 60; i++) {
        uint32_t temp = SHA1_ROTL(a, 5) + SHA1_MAJ(b, c, d) + e + K2 + w[i];
        e = d; d = c; c = SHA1_ROTL(b, 30); b = a; a = temp;
    }
    
    // Round 60-79
    #pragma GCC unroll 20
    for (int i = 60; i < 80; i++) {
        uint32_t temp = SHA1_ROTL(a, 5) + SHA1_PARITY(b, c, d) + e + K3 + w[i];
        e = d; d = c; c = SHA1_ROTL(b, 30); b = a; a = temp;
    }
    
    ctx->h[0] += a; ctx->h[1] += b;
    ctx->h[2] += c; ctx->h[3] += d;
    ctx->h[4] += e;
}

// Public API
SHA1_INLINE void sha1_init(sha1_ctx* ctx) {
    if (SHA1_UNLIKELY(!ctx)) return;
    ctx->h[0] = 0x67452301UL;
    ctx->h[1] = 0xEFCDAB89UL;
    ctx->h[2] = 0x98BADCFEUL;
    ctx->h[3] = 0x10325476UL;
    ctx->h[4] = 0xC3D2E1F0UL;
    ctx->len = 0;
    ctx->buf_len = 0;
}

SHA1_INLINE void sha1_update(sha1_ctx* SHA1_RESTRICT ctx, 
                              const void* SHA1_RESTRICT data, 
                              size_t len) {
    if (SHA1_UNLIKELY(!ctx || (!data && len))) return;
    
    const uint8_t* bytes = (const uint8_t*)data;
    size_t processed = 0;
    
    while (processed < len) {
        size_t remaining = len - processed;
        
        if (ctx->buf_len == 0 && remaining >= 64) {
            // Fast path: process directly
            sha1_transform(ctx, bytes + processed);
            processed += 64;
        } else {
            // Slow path: use buffer
            size_t space = 64 - ctx->buf_len;
            size_t take = (remaining < space) ? remaining : space;
            
            __builtin_memcpy(ctx->buf + ctx->buf_len, bytes + processed, take);
            ctx->buf_len += (uint32_t)take;
            processed += take;
            
            if (ctx->buf_len == 64) {
                sha1_transform(ctx, ctx->buf);
                ctx->buf_len = 0;
            }
        }
    }
    
    ctx->len += len;
}

SHA1_INLINE void sha1_final(sha1_ctx* SHA1_RESTRICT ctx, 
                             uint8_t SHA1_RESTRICT hash[SHA1_SIZE]) {
    if (SHA1_UNLIKELY(!hash)) return;
    if (SHA1_UNLIKELY(!ctx)) {
        __builtin_memset(hash, 0, SHA1_SIZE);
        return;
    }
    
    uint64_t bit_len = ctx->len << 3;
    uint32_t buf_len = ctx->buf_len;
    
    // Add padding
    ctx->buf[buf_len++] = 0x80;
    
    if (buf_len > 56) {
        __builtin_memset(ctx->buf + buf_len, 0, 64 - buf_len);
        sha1_transform(ctx, ctx->buf);
        buf_len = 0;
    }
    
    __builtin_memset(ctx->buf + buf_len, 0, 56 - buf_len);
    
    // Add length in bits (big-endian)
    uint32_t len_hi = (uint32_t)(bit_len >> 32);
    uint32_t len_lo = (uint32_t)(bit_len & 0xFFFFFFFF);
    
    ctx->buf[56] = (uint8_t)(len_hi >> 24);
    ctx->buf[57] = (uint8_t)(len_hi >> 16);
    ctx->buf[58] = (uint8_t)(len_hi >> 8);
    ctx->buf[59] = (uint8_t)(len_hi);
    ctx->buf[60] = (uint8_t)(len_lo >> 24);
    ctx->buf[61] = (uint8_t)(len_lo >> 16);
    ctx->buf[62] = (uint8_t)(len_lo >> 8);
    ctx->buf[63] = (uint8_t)(len_lo);
    
    sha1_transform(ctx, ctx->buf);
    
    // Store hash
    sha1_store32(hash,      ctx->h[0]);
    sha1_store32(hash + 4,  ctx->h[1]);
    sha1_store32(hash + 8,  ctx->h[2]);
    sha1_store32(hash + 12, ctx->h[3]);
    sha1_store32(hash + 16, ctx->h[4]);
}

SHA1_INLINE void sha1(uint8_t hash[SHA1_SIZE], const void* data, size_t len) {
    sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, data, len);
    sha1_final(&ctx, hash);
}

SHA1_INLINE void sha1_format(char* dst, size_t dst_cap, const uint8_t* hash) {
    if (!dst || dst_cap < SHA1_SIZE_FORMATTED) {
        if (dst && dst_cap) *dst = 0;
        return;
    }
    
    static const char hex[] = "0123456789abcdef";
    for (int i = 0; i < SHA1_SIZE; i++) {
        dst[i*2]     = hex[(hash[i] >> 4) & 0xF];
        dst[i*2 + 1] = hex[hash[i] & 0xF];
    }
    dst[SHA1_SIZE_FORMATTED - 1] = '\0';
}

#undef SHA1_INLINE
#undef SHA1_RESTRICT
#undef SHA1_LIKELY
#undef SHA1_UNLIKELY

#ifdef __cplusplus
}
#endif

#endif // SHA1_H
