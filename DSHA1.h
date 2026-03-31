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
    uint32_t state[5];
    uint8_t buffer[64];
    uint64_t count;
} DSHA1_CTX;

// ============== ENDIANNESS ==============
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

// ============== ALIGNMENT-SAFE ==============
static inline uint32_t dsha1_load_be32(const uint8_t *p) {
    uint32_t v;
    memcpy(&v, p, 4);
    return DSHA1_BSWAP32(v);
}

static inline void dsha1_store_be32(uint8_t *p, uint32_t v) {
    uint32_t tmp = DSHA1_BSWAP32(v);
    memcpy(p, &tmp, 4);
}

static inline void dsha1_store_be64(uint8_t *p, uint64_t v) {
    uint64_t tmp = DSHA1_BSWAP64(v);
    memcpy(p, &tmp, 8);
}

// ============== SHA-1 CORE ==============
#define DSHA1_ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

static inline uint32_t dsha1_f1(uint32_t b, uint32_t c, uint32_t d) {
    return d ^ (b & (c ^ d));
}

static inline uint32_t dsha1_f2(uint32_t b, uint32_t c, uint32_t d) {
    return b ^ c ^ d;
}

static inline uint32_t dsha1_f3(uint32_t b, uint32_t c, uint32_t d) {
    return (b & c) | (d & (b | c));
}

// ============== TRANSFORM - FULLY UNROLLED ==============
static inline void dsha1_transform(uint32_t state[5], const uint8_t *block) {
    uint32_t a, b, c, d, e;
    uint32_t w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13, w14, w15;
    uint32_t w16, w17, w18, w19, w20, w21, w22, w23, w24, w25, w26, w27, w28, w29, w30, w31;
    uint32_t w32, w33, w34, w35, w36, w37, w38, w39, w40, w41, w42, w43, w44, w45, w46, w47;
    uint32_t w48, w49, w50, w51, w52, w53, w54, w55, w56, w57, w58, w59, w60, w61, w62, w63;
    uint32_t w64, w65, w66, w67, w68, w69, w70, w71, w72, w73, w74, w75, w76, w77, w78, w79;
    
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    
    // Load 16 words
    w0 = dsha1_load_be32(block + 0);
    w1 = dsha1_load_be32(block + 4);
    w2 = dsha1_load_be32(block + 8);
    w3 = dsha1_load_be32(block + 12);
    w4 = dsha1_load_be32(block + 16);
    w5 = dsha1_load_be32(block + 20);
    w6 = dsha1_load_be32(block + 24);
    w7 = dsha1_load_be32(block + 28);
    w8 = dsha1_load_be32(block + 32);
    w9 = dsha1_load_be32(block + 36);
    w10 = dsha1_load_be32(block + 40);
    w11 = dsha1_load_be32(block + 44);
    w12 = dsha1_load_be32(block + 48);
    w13 = dsha1_load_be32(block + 52);
    w14 = dsha1_load_be32(block + 56);
    w15 = dsha1_load_be32(block + 60);
    
    // Expand 16 -> 80 (fully unrolled)
    w16 = DSHA1_ROTL32(w13 ^ w8 ^ w2 ^ w0, 1);
    w17 = DSHA1_ROTL32(w14 ^ w9 ^ w3 ^ w1, 1);
    w18 = DSHA1_ROTL32(w15 ^ w10 ^ w4 ^ w2, 1);
    w19 = DSHA1_ROTL32(w16 ^ w11 ^ w5 ^ w3, 1);
    w20 = DSHA1_ROTL32(w17 ^ w12 ^ w6 ^ w4, 1);
    w21 = DSHA1_ROTL32(w18 ^ w13 ^ w7 ^ w5, 1);
    w22 = DSHA1_ROTL32(w19 ^ w14 ^ w8 ^ w6, 1);
    w23 = DSHA1_ROTL32(w20 ^ w15 ^ w9 ^ w7, 1);
    w24 = DSHA1_ROTL32(w21 ^ w16 ^ w10 ^ w8, 1);
    w25 = DSHA1_ROTL32(w22 ^ w17 ^ w11 ^ w9, 1);
    w26 = DSHA1_ROTL32(w23 ^ w18 ^ w12 ^ w10, 1);
    w27 = DSHA1_ROTL32(w24 ^ w19 ^ w13 ^ w11, 1);
    w28 = DSHA1_ROTL32(w25 ^ w20 ^ w14 ^ w12, 1);
    w29 = DSHA1_ROTL32(w26 ^ w21 ^ w15 ^ w13, 1);
    w30 = DSHA1_ROTL32(w27 ^ w22 ^ w16 ^ w14, 1);
    w31 = DSHA1_ROTL32(w28 ^ w23 ^ w17 ^ w15, 1);
    w32 = DSHA1_ROTL32(w29 ^ w24 ^ w18 ^ w16, 1);
    w33 = DSHA1_ROTL32(w30 ^ w25 ^ w19 ^ w17, 1);
    w34 = DSHA1_ROTL32(w31 ^ w26 ^ w20 ^ w18, 1);
    w35 = DSHA1_ROTL32(w32 ^ w27 ^ w21 ^ w19, 1);
    w36 = DSHA1_ROTL32(w33 ^ w28 ^ w22 ^ w20, 1);
    w37 = DSHA1_ROTL32(w34 ^ w29 ^ w23 ^ w21, 1);
    w38 = DSHA1_ROTL32(w35 ^ w30 ^ w24 ^ w22, 1);
    w39 = DSHA1_ROTL32(w36 ^ w31 ^ w25 ^ w23, 1);
    w40 = DSHA1_ROTL32(w37 ^ w32 ^ w26 ^ w24, 1);
    w41 = DSHA1_ROTL32(w38 ^ w33 ^ w27 ^ w25, 1);
    w42 = DSHA1_ROTL32(w39 ^ w34 ^ w28 ^ w26, 1);
    w43 = DSHA1_ROTL32(w40 ^ w35 ^ w29 ^ w27, 1);
    w44 = DSHA1_ROTL32(w41 ^ w36 ^ w30 ^ w28, 1);
    w45 = DSHA1_ROTL32(w42 ^ w37 ^ w31 ^ w29, 1);
    w46 = DSHA1_ROTL32(w43 ^ w38 ^ w32 ^ w30, 1);
    w47 = DSHA1_ROTL32(w44 ^ w39 ^ w33 ^ w31, 1);
    w48 = DSHA1_ROTL32(w45 ^ w40 ^ w34 ^ w32, 1);
    w49 = DSHA1_ROTL32(w46 ^ w41 ^ w35 ^ w33, 1);
    w50 = DSHA1_ROTL32(w47 ^ w42 ^ w36 ^ w34, 1);
    w51 = DSHA1_ROTL32(w48 ^ w43 ^ w37 ^ w35, 1);
    w52 = DSHA1_ROTL32(w49 ^ w44 ^ w38 ^ w36, 1);
    w53 = DSHA1_ROTL32(w50 ^ w45 ^ w39 ^ w37, 1);
    w54 = DSHA1_ROTL32(w51 ^ w46 ^ w40 ^ w38, 1);
    w55 = DSHA1_ROTL32(w52 ^ w47 ^ w41 ^ w39, 1);
    w56 = DSHA1_ROTL32(w53 ^ w48 ^ w42 ^ w40, 1);
    w57 = DSHA1_ROTL32(w54 ^ w49 ^ w43 ^ w41, 1);
    w58 = DSHA1_ROTL32(w55 ^ w50 ^ w44 ^ w42, 1);
    w59 = DSHA1_ROTL32(w56 ^ w51 ^ w45 ^ w43, 1);
    w60 = DSHA1_ROTL32(w57 ^ w52 ^ w46 ^ w44, 1);
    w61 = DSHA1_ROTL32(w58 ^ w53 ^ w47 ^ w45, 1);
    w62 = DSHA1_ROTL32(w59 ^ w54 ^ w48 ^ w46, 1);
    w63 = DSHA1_ROTL32(w60 ^ w55 ^ w49 ^ w47, 1);
    w64 = DSHA1_ROTL32(w61 ^ w56 ^ w50 ^ w48, 1);
    w65 = DSHA1_ROTL32(w62 ^ w57 ^ w51 ^ w49, 1);
    w66 = DSHA1_ROTL32(w63 ^ w58 ^ w52 ^ w50, 1);
    w67 = DSHA1_ROTL32(w64 ^ w59 ^ w53 ^ w51, 1);
    w68 = DSHA1_ROTL32(w65 ^ w60 ^ w54 ^ w52, 1);
    w69 = DSHA1_ROTL32(w66 ^ w61 ^ w55 ^ w53, 1);
    w70 = DSHA1_ROTL32(w67 ^ w62 ^ w56 ^ w54, 1);
    w71 = DSHA1_ROTL32(w68 ^ w63 ^ w57 ^ w55, 1);
    w72 = DSHA1_ROTL32(w69 ^ w64 ^ w58 ^ w56, 1);
    w73 = DSHA1_ROTL32(w70 ^ w65 ^ w59 ^ w57, 1);
    w74 = DSHA1_ROTL32(w71 ^ w66 ^ w60 ^ w58, 1);
    w75 = DSHA1_ROTL32(w72 ^ w67 ^ w61 ^ w59, 1);
    w76 = DSHA1_ROTL32(w73 ^ w68 ^ w62 ^ w60, 1);
    w77 = DSHA1_ROTL32(w74 ^ w69 ^ w63 ^ w61, 1);
    w78 = DSHA1_ROTL32(w75 ^ w70 ^ w64 ^ w62, 1);
    w79 = DSHA1_ROTL32(w76 ^ w71 ^ w65 ^ w63, 1);
    
    // Round 0-19 (K1 = 0x5A827999)
    #define R1(x, y, z, k, wval) \
        do { \
            uint32_t temp = DSHA1_ROTL32(a, 5) + dsha1_f1(b, c, d) + e + (k) + (wval); \
            e = d; d = c; c = DSHA1_ROTL32(b, 30); b = a; a = temp; \
        } while(0)
    
    R1(a, b, c, d, 0x5A827999UL, w0); R1(e, a, b, c, 0x5A827999UL, w1);
    R1(d, e, a, b, 0x5A827999UL, w2); R1(c, d, e, a, 0x5A827999UL, w3);
    R1(b, c, d, e, 0x5A827999UL, w4); R1(a, b, c, d, 0x5A827999UL, w5);
    R1(e, a, b, c, 0x5A827999UL, w6); R1(d, e, a, b, 0x5A827999UL, w7);
    R1(c, d, e, a, 0x5A827999UL, w8); R1(b, c, d, e, 0x5A827999UL, w9);
    R1(a, b, c, d, 0x5A827999UL, w10); R1(e, a, b, c, 0x5A827999UL, w11);
    R1(d, e, a, b, 0x5A827999UL, w12); R1(c, d, e, a, 0x5A827999UL, w13);
    R1(b, c, d, e, 0x5A827999UL, w14); R1(a, b, c, d, 0x5A827999UL, w15);
    R1(e, a, b, c, 0x5A827999UL, w16); R1(d, e, a, b, 0x5A827999UL, w17);
    R1(c, d, e, a, 0x5A827999UL, w18); R1(b, c, d, e, 0x5A827999UL, w19);
    
    // Round 20-39 (K2 = 0x6ED9EBA1)
    #undef R1
    #define R2(x, y, z, k, wval) \
        do { \
            uint32_t temp = DSHA1_ROTL32(a, 5) + dsha1_f2(b, c, d) + e + (k) + (wval); \
            e = d; d = c; c = DSHA1_ROTL32(b, 30); b = a; a = temp; \
        } while(0)
    
    R2(a, b, c, d, 0x6ED9EBA1UL, w20); R2(e, a, b, c, 0x6ED9EBA1UL, w21);
    R2(d, e, a, b, 0x6ED9EBA1UL, w22); R2(c, d, e, a, 0x6ED9EBA1UL, w23);
    R2(b, c, d, e, 0x6ED9EBA1UL, w24); R2(a, b, c, d, 0x6ED9EBA1UL, w25);
    R2(e, a, b, c, 0x6ED9EBA1UL, w26); R2(d, e, a, b, 0x6ED9EBA1UL, w27);
    R2(c, d, e, a, 0x6ED9EBA1UL, w28); R2(b, c, d, e, 0x6ED9EBA1UL, w29);
    R2(a, b, c, d, 0x6ED9EBA1UL, w30); R2(e, a, b, c, 0x6ED9EBA1UL, w31);
    R2(d, e, a, b, 0x6ED9EBA1UL, w32); R2(c, d, e, a, 0x6ED9EBA1UL, w33);
    R2(b, c, d, e, 0x6ED9EBA1UL, w34); R2(a, b, c, d, 0x6ED9EBA1UL, w35);
    R2(e, a, b, c, 0x6ED9EBA1UL, w36); R2(d, e, a, b, 0x6ED9EBA1UL, w37);
    R2(c, d, e, a, 0x6ED9EBA1UL, w38); R2(b, c, d, e, 0x6ED9EBA1UL, w39);
    
    // Round 40-59 (K3 = 0x8F1BBCDC)
    #undef R2
    #define R3(x, y, z, k, wval) \
        do { \
            uint32_t temp = DSHA1_ROTL32(a, 5) + dsha1_f3(b, c, d) + e + (k) + (wval); \
            e = d; d = c; c = DSHA1_ROTL32(b, 30); b = a; a = temp; \
        } while(0)
    
    R3(a, b, c, d, 0x8F1BBCDCUL, w40); R3(e, a, b, c, 0x8F1BBCDCUL, w41);
    R3(d, e, a, b, 0x8F1BBCDCUL, w42); R3(c, d, e, a, 0x8F1BBCDCUL, w43);
    R3(b, c, d, e, 0x8F1BBCDCUL, w44); R3(a, b, c, d, 0x8F1BBCDCUL, w45);
    R3(e, a, b, c, 0x8F1BBCDCUL, w46); R3(d, e, a, b, 0x8F1BBCDCUL, w47);
    R3(c, d, e, a, 0x8F1BBCDCUL, w48); R3(b, c, d, e, 0x8F1BBCDCUL, w49);
    R3(a, b, c, d, 0x8F1BBCDCUL, w50); R3(e, a, b, c, 0x8F1BBCDCUL, w51);
    R3(d, e, a, b, 0x8F1BBCDCUL, w52); R3(c, d, e, a, 0x8F1BBCDCUL, w53);
    R3(b, c, d, e, 0x8F1BBCDCUL, w54); R3(a, b, c, d, 0x8F1BBCDCUL, w55);
    R3(e, a, b, c, 0x8F1BBCDCUL, w56); R3(d, e, a, b, 0x8F1BBCDCUL, w57);
    R3(c, d, e, a, 0x8F1BBCDCUL, w58); R3(b, c, d, e, 0x8F1BBCDCUL, w59);
    
    // Round 60-79 (K4 = 0xCA62C1D6)
    #undef R3
    #define R4(x, y, z, k, wval) \
        do { \
            uint32_t temp = DSHA1_ROTL32(a, 5) + dsha1_f2(b, c, d) + e + (k) + (wval); \
            e = d; d = c; c = DSHA1_ROTL32(b, 30); b = a; a = temp; \
        } while(0)
    
    R4(a, b, c, d, 0xCA62C1D6UL, w60); R4(e, a, b, c, 0xCA62C1D6UL, w61);
    R4(d, e, a, b, 0xCA62C1D6UL, w62); R4(c, d, e, a, 0xCA62C1D6UL, w63);
    R4(b, c, d, e, 0xCA62C1D6UL, w64); R4(a, b, c, d, 0xCA62C1D6UL, w65);
    R4(e, a, b, c, 0xCA62C1D6UL, w66); R4(d, e, a, b, 0xCA62C1D6UL, w67);
    R4(c, d, e, a, 0xCA62C1D6UL, w68); R4(b, c, d, e, 0xCA62C1D6UL, w69);
    R4(a, b, c, d, 0xCA62C1D6UL, w70); R4(e, a, b, c, 0xCA62C1D6UL, w71);
    R4(d, e, a, b, 0xCA62C1D6UL, w72); R4(c, d, e, a, 0xCA62C1D6UL, w73);
    R4(b, c, d, e, 0xCA62C1D6UL, w74); R4(a, b, c, d, 0xCA62C1D6UL, w75);
    R4(e, a, b, c, 0xCA62C1D6UL, w76); R4(d, e, a, b, 0xCA62C1D6UL, w77);
    R4(c, d, e, a, 0xCA62C1D6UL, w78); R4(b, c, d, e, 0xCA62C1D6UL, w79);
    
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

// ============== PUBLIC API ==============
static inline void dsha1_init(DSHA1_CTX *ctx) {
    ctx->state[0] = 0x67452301UL;
    ctx->state[1] = 0xEFCDAB89UL;
    ctx->state[2] = 0x98BADCFEUL;
    ctx->state[3] = 0x10325476UL;
    ctx->state[4] = 0xC3D2E1F0UL;
    ctx->count = 0;
}

static inline void dsha1_update(DSHA1_CTX *ctx, const uint8_t *data, size_t len) {
    size_t buffer_idx = ctx->count & 63;
    size_t remaining;
    
    ctx->count += len;
    
    if (buffer_idx) {
        remaining = 64 - buffer_idx;
        if (remaining > len) remaining = len;
        memcpy(ctx->buffer + buffer_idx, data, remaining);
        data += remaining;
        len -= remaining;
        
        if (buffer_idx + remaining == 64) {
            dsha1_transform(ctx->state, ctx->buffer);
        }
    }
    
    while (len >= 64) {
        dsha1_transform(ctx->state, data);
        data += 64;
        len -= 64;
    }
    
    if (len) {
        memcpy(ctx->buffer, data, len);
    }
}

static inline void dsha1_final(DSHA1_CTX *ctx, uint8_t hash[DSHA1_OUTPUT_SIZE]) {
    uint64_t bit_count = ctx->count << 3;
    size_t buffer_idx = ctx->count & 63;
    size_t pad_len;
    
    ctx->buffer[buffer_idx] = 0x80;
    buffer_idx++;
    
    if (buffer_idx > 56) {
        memset(ctx->buffer + buffer_idx, 0, 64 - buffer_idx);
        dsha1_transform(ctx->state, ctx->buffer);
        memset(ctx->buffer, 0, 56);
    } else {
        memset(ctx->buffer + buffer_idx, 0, 56 - buffer_idx);
    }
    
    dsha1_store_be64(ctx->buffer + 56, bit_count);
    dsha1_transform(ctx->state, ctx->buffer);
    
    dsha1_store_be32(hash, ctx->state[0]);
    dsha1_store_be32(hash + 4, ctx->state[1]);
    dsha1_store_be32(hash + 8, ctx->state[2]);
    dsha1_store_be32(hash + 12, ctx->state[3]);
    dsha1_store_be32(hash + 16, ctx->state[4]);
}

static inline void dsha1_reset(DSHA1_CTX *ctx) {
    dsha1_init(ctx);
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

#endif
