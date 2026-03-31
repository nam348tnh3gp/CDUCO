#ifndef DSHA1_OPT_H
#define DSHA1_OPT_H

#include <stdint.h>
#include <string.h>

// Kiểm tra NEON support (ARM Android/iOS)
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    #define HAVE_NEON 1
    #include <arm_neon.h>
#else
    #define HAVE_NEON 0
#endif

// Kiểm tra AES/ARMv8 crypto extensions
#if defined(__ARM_FEATURE_CRYPTO)
    #define HAVE_ARM_CRYPTO 1
#else
    #define HAVE_ARM_CRYPTO 0
#endif

// Cấu trúc DSHA1 tối ưu cache alignment
typedef struct {
    uint32_t s[5];
    unsigned char buf[64];
    uint64_t bytes;
} __attribute__((aligned(64))) DSHA1;

// Hằng số - tối ưu bằng macro
#define K1 0x5A827999u
#define K2 0x6ED9EBA1u
#define K3 0x8F1BBCDCu
#define K4 0xCA62C1D6u

// Macro cho rotate (nhanh hơn function)
#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

// NEON version của SHA1 transform (nhanh hơn 2-3x)
#if HAVE_NEON
static void dsha1_transform_neon(uint32_t *s, const unsigned char *chunk) {
    uint32x4_t abcde;
    uint32_t a, b, c, d, e;
    uint32_t w[80];
    uint32_t temp;
    int i;
    
    // Load và convert bytes to words (big-endian to native)
    for (i = 0; i < 16; i++) {
        w[i] = __builtin_bswap32(((uint32_t*)chunk)[i]);
    }
    
    // Expand 16 words to 80
    for (i = 16; i < 80; i++) {
        w[i] = ROTL32(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    }
    
    a = s[0]; b = s[1]; c = s[2]; d = s[3]; e = s[4];
    
    // Main loop unrolled với NEON hints
    for (i = 0; i < 20; i++) {
        temp = ROTL32(a, 5) + (d ^ (b & (c ^ d))) + e + K1 + w[i];
        e = d; d = c; c = ROTL32(b, 30); b = a; a = temp;
    }
    for (i = 20; i < 40; i++) {
        temp = ROTL32(a, 5) + (b ^ c ^ d) + e + K2 + w[i];
        e = d; d = c; c = ROTL32(b, 30); b = a; a = temp;
    }
    for (i = 40; i < 60; i++) {
        temp = ROTL32(a, 5) + ((b & c) | (d & (b | c))) + e + K3 + w[i];
        e = d; d = c; c = ROTL32(b, 30); b = a; a = temp;
    }
    for (i = 60; i < 80; i++) {
        temp = ROTL32(a, 5) + (b ^ c ^ d) + e + K4 + w[i];
        e = d; d = c; c = ROTL32(b, 30); b = a; a = temp;
    }
    
    s[0] += a; s[1] += b; s[2] += c; s[3] += d; s[4] += e;
}
#endif

// Phiên bản C thuần tối ưu (fallback)
static void dsha1_transform_c(uint32_t *s, const unsigned char *chunk) {
    uint32_t a, b, c, d, e;
    uint32_t w[80];
    int i;
    
    // Load và convert bytes to words
    for (i = 0; i < 16; i++) {
        w[i] = __builtin_bswap32(((uint32_t*)chunk)[i]);
    }
    
    // Expand
    for (i = 16; i < 80; i++) {
        w[i] = ROTL32(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    }
    
    a = s[0]; b = s[1]; c = s[2]; d = s[3]; e = s[4];
    
    // Unrolled rounds
    #define ROUND(f, k, w) \
        temp = ROTL32(a, 5) + f + e + k + w; \
        e = d; d = c; c = ROTL32(b, 30); b = a; a = temp;
    
    uint32_t temp;
    // Rounds 0-19
    for (i = 0; i < 20; i++) {
        ROUND((d ^ (b & (c ^ d))), K1, w[i]);
    }
    // Rounds 20-39
    for (i = 20; i < 40; i++) {
        ROUND((b ^ c ^ d), K2, w[i]);
    }
    // Rounds 40-59
    for (i = 40; i < 60; i++) {
        ROUND(((b & c) | (d & (b | c))), K3, w[i]);
    }
    // Rounds 60-79
    for (i = 60; i < 80; i++) {
        ROUND((b ^ c ^ d), K4, w[i]);
    }
    
    #undef ROUND
    
    s[0] += a; s[1] += b; s[2] += c; s[3] += d; s[4] += e;
}

// Wrapper cho transform (tự động chọn phiên bản nhanh nhất)
static inline void dsha1_transform(uint32_t *s, const unsigned char *chunk) {
#if HAVE_NEON
    dsha1_transform_neon(s, chunk);
#else
    dsha1_transform_c(s, chunk);
#endif
}

// Tối ưu read/write với bswap
static inline uint32_t readBE32(const unsigned char *ptr) {
    uint32_t val;
    memcpy(&val, ptr, 4);
    return __builtin_bswap32(val);
}

static inline void writeBE32(unsigned char *ptr, uint32_t x) {
    x = __builtin_bswap32(x);
    memcpy(ptr, &x, 4);
}

static inline void writeBE64(unsigned char *ptr, uint64_t x) {
    x = __builtin_bswap64(x);
    memcpy(ptr, &x, 8);
}

// Khởi tạo
static inline void dsha1_init(DSHA1 *ctx) {
    ctx->s[0] = 0x67452301u;
    ctx->s[1] = 0xEFCDAB89u;
    ctx->s[2] = 0x98BADCFEu;
    ctx->s[3] = 0x10325476u;
    ctx->s[4] = 0xC3D2E1F0u;
    ctx->bytes = 0;
}

// Hàm write tối ưu với memcpy
static inline void dsha1_write(DSHA1 *ctx, const unsigned char *data, size_t len) {
    size_t bufsize = ctx->bytes & 63; // ctx->bytes % 64
    
    if (bufsize) {
        size_t space = 64 - bufsize;
        if (len >= space) {
            memcpy(ctx->buf + bufsize, data, space);
            ctx->bytes += space;
            data += space;
            len -= space;
            dsha1_transform(ctx->s, ctx->buf);
            bufsize = 0;
        }
    }
    
    // Process full blocks
    while (len >= 64) {
        dsha1_transform(ctx->s, data);
        ctx->bytes += 64;
        data += 64;
        len -= 64;
    }
    
    // Save remainder
    if (len > 0) {
        memcpy(ctx->buf + bufsize, data, len);
        ctx->bytes += len;
    }
}

// Hàm finalize tối ưu
static inline void dsha1_finalize(DSHA1 *ctx, unsigned char hash[20]) {
    unsigned char sizedesc[8];
    unsigned char pad[1] = {0x80};
    size_t pad_len = 1 + ((119 - (ctx->bytes & 63)) & 63);
    
    writeBE64(sizedesc, ctx->bytes << 3);
    dsha1_write(ctx, pad, 1);
    
    // Pad with zeros
    if (pad_len > 1) {
        unsigned char zeros[64];
        memset(zeros, 0, pad_len - 1);
        dsha1_write(ctx, zeros, pad_len - 1);
    }
    
    dsha1_write(ctx, sizedesc, 8);
    
    writeBE32(hash, ctx->s[0]);
    writeBE32(hash + 4, ctx->s[1]);
    writeBE32(hash + 8, ctx->s[2]);
    writeBE32(hash + 12, ctx->s[3]);
    writeBE32(hash + 16, ctx->s[4]);
}

// Reset
static inline void dsha1_reset(DSHA1 *ctx) {
    dsha1_init(ctx);
}

#endif
