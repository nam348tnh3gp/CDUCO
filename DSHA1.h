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

// ============== ENDIANNESS ==============
#if defined(__ARM_NEON__) || defined(__aarch64__)
    #define DSHA1_BSWAP32(x) __builtin_bswap32(x)
    #define DSHA1_BSWAP64(x) __builtin_bswap64(x)
#elif defined(__i386__) || defined(__x86_64__)
    #define DSHA1_BSWAP32(x) __builtin_bswap32(x)
    #define DSHA1_BSWAP64(x) __builtin_bswap64(x)
#else
    static inline uint32_t dsha1_bswap32(uint32_t x) {
        __asm__("rev %0, %1" : "=r"(x) : "r"(x));
        return x;
    }
    static inline uint64_t dsha1_bswap64(uint64_t x) {
        __asm__("rev %0, %1" : "=r"(x) : "r"(x));
        return x;
    }
    #define DSHA1_BSWAP32(x) dsha1_bswap32(x)
    #define DSHA1_BSWAP64(x) dsha1_bswap64(x)
#endif

// ============== ALIGNMENT-SAFE I/O ==============
static inline uint32_t LOAD32(const uint8_t *p) {
    uint32_t v;
    __builtin_memcpy(&v, p, 4);
    return DSHA1_BSWAP32(v);
}

static inline void STORE32(uint8_t *p, uint32_t v) {
    v = DSHA1_BSWAP32(v);
    __builtin_memcpy(p, &v, 4);
}

static inline void STORE64(uint8_t *p, uint64_t v) {
    v = DSHA1_BSWAP64(v);
    __builtin_memcpy(p, &v, 8);
}

// ============== SHA-1 CORE ==============
#define ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define F1(b, c, d) (d ^ (b & (c ^ d)))
#define F2(b, c, d) (b ^ c ^ d)
#define F3(b, c, d) ((b & c) | (d & (b | c)))

#define ROUND(a,b,c,d,e,f,k,w) do { \
    e += ROTL(a,5) + f(b,c,d) + k + w; \
    b = ROTL(b,30); \
} while(0)

// ============== TRANSFORM - COMPLETE UNROLL ==============
static inline void transform(uint32_t h[5], const uint8_t *blk) {
    uint32_t a,b,c,d,e;
    uint32_t w0,w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11,w12,w13,w14,w15;
    uint32_t w16,w17,w18,w19,w20,w21,w22,w23,w24,w25,w26,w27,w28,w29,w30,w31;
    uint32_t w32,w33,w34,w35,w36,w37,w38,w39,w40,w41,w42,w43,w44,w45,w46,w47;
    uint32_t w48,w49,w50,w51,w52,w53,w54,w55,w56,w57,w58,w59,w60,w61,w62,w63;
    uint32_t w64,w65,w66,w67,w68,w69,w70,w71,w72,w73,w74,w75,w76,w77,w78,w79;
    
    a = h[0]; b = h[1]; c = h[2]; d = h[3]; e = h[4];
    
    w0  = LOAD32(blk + 0);  w1  = LOAD32(blk + 4);
    w2  = LOAD32(blk + 8);  w3  = LOAD32(blk + 12);
    w4  = LOAD32(blk + 16); w5  = LOAD32(blk + 20);
    w6  = LOAD32(blk + 24); w7  = LOAD32(blk + 28);
    w8  = LOAD32(blk + 32); w9  = LOAD32(blk + 36);
    w10 = LOAD32(blk + 40); w11 = LOAD32(blk + 44);
    w12 = LOAD32(blk + 48); w13 = LOAD32(blk + 52);
    w14 = LOAD32(blk + 56); w15 = LOAD32(blk + 60);
    
    w16 = ROTL(w13 ^ w8 ^ w2 ^ w0, 1);
    w17 = ROTL(w14 ^ w9 ^ w3 ^ w1, 1);
    w18 = ROTL(w15 ^ w10 ^ w4 ^ w2, 1);
    w19 = ROTL(w16 ^ w11 ^ w5 ^ w3, 1);
    w20 = ROTL(w17 ^ w12 ^ w6 ^ w4, 1);
    w21 = ROTL(w18 ^ w13 ^ w7 ^ w5, 1);
    w22 = ROTL(w19 ^ w14 ^ w8 ^ w6, 1);
    w23 = ROTL(w20 ^ w15 ^ w9 ^ w7, 1);
    w24 = ROTL(w21 ^ w16 ^ w10 ^ w8, 1);
    w25 = ROTL(w22 ^ w17 ^ w11 ^ w9, 1);
    w26 = ROTL(w23 ^ w18 ^ w12 ^ w10, 1);
    w27 = ROTL(w24 ^ w19 ^ w13 ^ w11, 1);
    w28 = ROTL(w25 ^ w20 ^ w14 ^ w12, 1);
    w29 = ROTL(w26 ^ w21 ^ w15 ^ w13, 1);
    w30 = ROTL(w27 ^ w22 ^ w16 ^ w14, 1);
    w31 = ROTL(w28 ^ w23 ^ w17 ^ w15, 1);
    w32 = ROTL(w29 ^ w24 ^ w18 ^ w16, 1);
    w33 = ROTL(w30 ^ w25 ^ w19 ^ w17, 1);
    w34 = ROTL(w31 ^ w26 ^ w20 ^ w18, 1);
    w35 = ROTL(w32 ^ w27 ^ w21 ^ w19, 1);
    w36 = ROTL(w33 ^ w28 ^ w22 ^ w20, 1);
    w37 = ROTL(w34 ^ w29 ^ w23 ^ w21, 1);
    w38 = ROTL(w35 ^ w30 ^ w24 ^ w22, 1);
    w39 = ROTL(w36 ^ w31 ^ w25 ^ w23, 1);
    w40 = ROTL(w37 ^ w32 ^ w26 ^ w24, 1);
    w41 = ROTL(w38 ^ w33 ^ w27 ^ w25, 1);
    w42 = ROTL(w39 ^ w34 ^ w28 ^ w26, 1);
    w43 = ROTL(w40 ^ w35 ^ w29 ^ w27, 1);
    w44 = ROTL(w41 ^ w36 ^ w30 ^ w28, 1);
    w45 = ROTL(w42 ^ w37 ^ w31 ^ w29, 1);
    w46 = ROTL(w43 ^ w38 ^ w32 ^ w30, 1);
    w47 = ROTL(w44 ^ w39 ^ w33 ^ w31, 1);
    w48 = ROTL(w45 ^ w40 ^ w34 ^ w32, 1);
    w49 = ROTL(w46 ^ w41 ^ w35 ^ w33, 1);
    w50 = ROTL(w47 ^ w42 ^ w36 ^ w34, 1);
    w51 = ROTL(w48 ^ w43 ^ w37 ^ w35, 1);
    w52 = ROTL(w49 ^ w44 ^ w38 ^ w36, 1);
    w53 = ROTL(w50 ^ w45 ^ w39 ^ w37, 1);
    w54 = ROTL(w51 ^ w46 ^ w40 ^ w38, 1);
    w55 = ROTL(w52 ^ w47 ^ w41 ^ w39, 1);
    w56 = ROTL(w53 ^ w48 ^ w42 ^ w40, 1);
    w57 = ROTL(w54 ^ w49 ^ w43 ^ w41, 1);
    w58 = ROTL(w55 ^ w50 ^ w44 ^ w42, 1);
    w59 = ROTL(w56 ^ w51 ^ w45 ^ w43, 1);
    w60 = ROTL(w57 ^ w52 ^ w46 ^ w44, 1);
    w61 = ROTL(w58 ^ w53 ^ w47 ^ w45, 1);
    w62 = ROTL(w59 ^ w54 ^ w48 ^ w46, 1);
    w63 = ROTL(w60 ^ w55 ^ w49 ^ w47, 1);
    w64 = ROTL(w61 ^ w56 ^ w50 ^ w48, 1);
    w65 = ROTL(w62 ^ w57 ^ w51 ^ w49, 1);
    w66 = ROTL(w63 ^ w58 ^ w52 ^ w50, 1);
    w67 = ROTL(w64 ^ w59 ^ w53 ^ w51, 1);
    w68 = ROTL(w65 ^ w60 ^ w54 ^ w52, 1);
    w69 = ROTL(w66 ^ w61 ^ w55 ^ w53, 1);
    w70 = ROTL(w67 ^ w62 ^ w56 ^ w54, 1);
    w71 = ROTL(w68 ^ w63 ^ w57 ^ w55, 1);
    w72 = ROTL(w69 ^ w64 ^ w58 ^ w56, 1);
    w73 = ROTL(w70 ^ w65 ^ w59 ^ w57, 1);
    w74 = ROTL(w71 ^ w66 ^ w60 ^ w58, 1);
    w75 = ROTL(w72 ^ w67 ^ w61 ^ w59, 1);
    w76 = ROTL(w73 ^ w68 ^ w62 ^ w60, 1);
    w77 = ROTL(w74 ^ w69 ^ w63 ^ w61, 1);
    w78 = ROTL(w75 ^ w70 ^ w64 ^ w62, 1);
    w79 = ROTL(w76 ^ w71 ^ w65 ^ w63, 1);
    
    ROUND(a,b,c,d,e,F1,0x5A827999UL, w0);  ROUND(e,a,b,c,d,F1,0x5A827999UL, w1);
    ROUND(d,e,a,b,c,F1,0x5A827999UL, w2);  ROUND(c,d,e,a,b,F1,0x5A827999UL, w3);
    ROUND(b,c,d,e,a,F1,0x5A827999UL, w4);  ROUND(a,b,c,d,e,F1,0x5A827999UL, w5);
    ROUND(e,a,b,c,d,F1,0x5A827999UL, w6);  ROUND(d,e,a,b,c,F1,0x5A827999UL, w7);
    ROUND(c,d,e,a,b,F1,0x5A827999UL, w8);  ROUND(b,c,d,e,a,F1,0x5A827999UL, w9);
    ROUND(a,b,c,d,e,F1,0x5A827999UL,w10); ROUND(e,a,b,c,d,F1,0x5A827999UL,w11);
    ROUND(d,e,a,b,c,F1,0x5A827999UL,w12); ROUND(c,d,e,a,b,F1,0x5A827999UL,w13);
    ROUND(b,c,d,e,a,F1,0x5A827999UL,w14); ROUND(a,b,c,d,e,F1,0x5A827999UL,w15);
    ROUND(e,a,b,c,d,F1,0x5A827999UL,w16); ROUND(d,e,a,b,c,F1,0x5A827999UL,w17);
    ROUND(c,d,e,a,b,F1,0x5A827999UL,w18); ROUND(b,c,d,e,a,F1,0x5A827999UL,w19);
    
    ROUND(a,b,c,d,e,F2,0x6ED9EBA1UL,w20); ROUND(e,a,b,c,d,F2,0x6ED9EBA1UL,w21);
    ROUND(d,e,a,b,c,F2,0x6ED9EBA1UL,w22); ROUND(c,d,e,a,b,F2,0x6ED9EBA1UL,w23);
    ROUND(b,c,d,e,a,F2,0x6ED9EBA1UL,w24); ROUND(a,b,c,d,e,F2,0x6ED9EBA1UL,w25);
    ROUND(e,a,b,c,d,F2,0x6ED9EBA1UL,w26); ROUND(d,e,a,b,c,F2,0x6ED9EBA1UL,w27);
    ROUND(c,d,e,a,b,F2,0x6ED9EBA1UL,w28); ROUND(b,c,d,e,a,F2,0x6ED9EBA1UL,w29);
    ROUND(a,b,c,d,e,F2,0x6ED9EBA1UL,w30); ROUND(e,a,b,c,d,F2,0x6ED9EBA1UL,w31);
    ROUND(d,e,a,b,c,F2,0x6ED9EBA1UL,w32); ROUND(c,d,e,a,b,F2,0x6ED9EBA1UL,w33);
    ROUND(b,c,d,e,a,F2,0x6ED9EBA1UL,w34); ROUND(a,b,c,d,e,F2,0x6ED9EBA1UL,w35);
    ROUND(e,a,b,c,d,F2,0x6ED9EBA1UL,w36); ROUND(d,e,a,b,c,F2,0x6ED9EBA1UL,w37);
    ROUND(c,d,e,a,b,F2,0x6ED9EBA1UL,w38); ROUND(b,c,d,e,a,F2,0x6ED9EBA1UL,w39);
    
    ROUND(a,b,c,d,e,F3,0x8F1BBCDCUL,w40); ROUND(e,a,b,c,d,F3,0x8F1BBCDCUL,w41);
    ROUND(d,e,a,b,c,F3,0x8F1BBCDCUL,w42); ROUND(c,d,e,a,b,F3,0x8F1BBCDCUL,w43);
    ROUND(b,c,d,e,a,F3,0x8F1BBCDCUL,w44); ROUND(a,b,c,d,e,F3,0x8F1BBCDCUL,w45);
    ROUND(e,a,b,c,d,F3,0x8F1BBCDCUL,w46); ROUND(d,e,a,b,c,F3,0x8F1BBCDCUL,w47);
    ROUND(c,d,e,a,b,F3,0x8F1BBCDCUL,w48); ROUND(b,c,d,e,a,F3,0x8F1BBCDCUL,w49);
    ROUND(a,b,c,d,e,F3,0x8F1BBCDCUL,w50); ROUND(e,a,b,c,d,F3,0x8F1BBCDCUL,w51);
    ROUND(d,e,a,b,c,F3,0x8F1BBCDCUL,w52); ROUND(c,d,e,a,b,F3,0x8F1BBCDCUL,w53);
    ROUND(b,c,d,e,a,F3,0x8F1BBCDCUL,w54); ROUND(a,b,c,d,e,F3,0x8F1BBCDCUL,w55);
    ROUND(e,a,b,c,d,F3,0x8F1BBCDCUL,w56); ROUND(d,e,a,b,c,F3,0x8F1BBCDCUL,w57);
    ROUND(c,d,e,a,b,F3,0x8F1BBCDCUL,w58); ROUND(b,c,d,e,a,F3,0x8F1BBCDCUL,w59);
    
    ROUND(a,b,c,d,e,F2,0xCA62C1D6UL,w60); ROUND(e,a,b,c,d,F2,0xCA62C1D6UL,w61);
    ROUND(d,e,a,b,c,F2,0xCA62C1D6UL,w62); ROUND(c,d,e,a,b,F2,0xCA62C1D6UL,w63);
    ROUND(b,c,d,e,a,F2,0xCA62C1D6UL,w64); ROUND(a,b,c,d,e,F2,0xCA62C1D6UL,w65);
    ROUND(e,a,b,c,d,F2,0xCA62C1D6UL,w66); ROUND(d,e,a,b,c,F2,0xCA62C1D6UL,w67);
    ROUND(c,d,e,a,b,F2,0xCA62C1D6UL,w68); ROUND(b,c,d,e,a,F2,0xCA62C1D6UL,w69);
    ROUND(a,b,c,d,e,F2,0xCA62C1D6UL,w70); ROUND(e,a,b,c,d,F2,0xCA62C1D6UL,w71);
    ROUND(d,e,a,b,c,F2,0xCA62C1D6UL,w72); ROUND(c,d,e,a,b,F2,0xCA62C1D6UL,w73);
    ROUND(b,c,d,e,a,F2,0xCA62C1D6UL,w74); ROUND(a,b,c,d,e,F2,0xCA62C1D6UL,w75);
    ROUND(e,a,b,c,d,F2,0xCA62C1D6UL,w76); ROUND(d,e,a,b,c,F2,0xCA62C1D6UL,w77);
    ROUND(c,d,e,a,b,F2,0xCA62C1D6UL,w78); ROUND(b,c,d,e,a,F2,0xCA62C1D6UL,w79);
    
    h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
}

// ============== PUBLIC API ==============
static inline void dsha1_init(DSHA1_CTX *ctx) {
    ctx->h[0] = 0x67452301UL;
    ctx->h[1] = 0xEFCDAB89UL;
    ctx->h[2] = 0x98BADCFEUL;
    ctx->h[3] = 0x10325476UL;
    ctx->h[4] = 0xC3D2E1F0UL;
    ctx->len = 0;
}

static inline void dsha1_update(DSHA1_CTX *ctx, const uint8_t *data, size_t len) {
    size_t i = ctx->len & 63;
    ctx->len += len;
    
    if (i) {
        size_t n = 64 - i;
        if (n > len) n = len;
        __builtin_memcpy(ctx->buf + i, data, n);
        data += n;
        len -= n;
        if (i + n == 64) transform(ctx->h, ctx->buf);
    }
    
    while (len >= 64) {
        transform(ctx->h, data);
        data += 64;
        len -= 64;
    }
    
    if (len) __builtin_memcpy(ctx->buf, data, len);
}

static inline void dsha1_final(DSHA1_CTX *ctx, uint8_t hash[20]) {
    uint64_t bits = ctx->len << 3;
    size_t i = ctx->len & 63;
    
    ctx->buf[i++] = 0x80;
    
    if (i > 56) {
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

static inline void dsha1_reset(DSHA1_CTX *ctx) {
    dsha1_init(ctx);
}

static inline void dsha1_warmup(DSHA1_CTX *ctx) {
    uint8_t tmp[20];
    dsha1_update(ctx, (const uint8_t*)"warmupwarmupwa", 20);
    dsha1_final(ctx, tmp);
}

#ifdef __cplusplus
}
#endif

#endif
