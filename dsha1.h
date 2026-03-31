#ifndef DSHA1_H
#define DSHA1_H

#include <stdint.h>
#include <string.h>

// Cấu trúc DSHA1
typedef struct {
    uint32_t s[5];
    unsigned char buf[64];
    uint64_t bytes;
} DSHA1;

// Hằng số
#define K1 0x5A827999ul
#define K2 0x6ED9EBA1ul
#define K3 0x8F1BBCDCul
#define K4 0xCA62C1D6ul
#define OUTPUT_SIZE 20

// Hàm inline
static inline uint32_t f1(uint32_t b, uint32_t c, uint32_t d) {
    return d ^ (b & (c ^ d));
}

static inline uint32_t f2(uint32_t b, uint32_t c, uint32_t d) {
    return b ^ c ^ d;
}

static inline uint32_t f3(uint32_t b, uint32_t c, uint32_t d) {
    return (b & c) | (d & (b | c));
}

static inline uint32_t left(uint32_t x) {
    return (x << 1) | (x >> 31);
}

static inline void Round(uint32_t a, uint32_t *b, uint32_t c, uint32_t d, uint32_t *e,
                         uint32_t f, uint32_t k, uint32_t w) {
    *e += ((a << 5) | (a >> 27)) + f + k + w;
    *b = (*b << 30) | (*b >> 2);
}

static inline uint32_t readBE32(const unsigned char *ptr) {
    return __builtin_bswap32(*(uint32_t *)ptr);
}

static inline void writeBE32(unsigned char *ptr, uint32_t x) {
    *(uint32_t *)ptr = __builtin_bswap32(x);
}

static inline void writeBE64(unsigned char *ptr, uint64_t x) {
    *(uint64_t *)ptr = __builtin_bswap64(x);
}

// Khởi tạo
static inline void dsha1_init(DSHA1 *ctx) {
    ctx->s[0] = 0x67452301ul;
    ctx->s[1] = 0xEFCDAB89ul;
    ctx->s[2] = 0x98BADCFEul;
    ctx->s[3] = 0x10325476ul;
    ctx->s[4] = 0xC3D2E1F0ul;
    ctx->bytes = 0;
}

// Hàm transform
static void dsha1_transform(uint32_t *s, const unsigned char *chunk) {
    uint32_t a = s[0], b = s[1], c = s[2], d = s[3], e = s[4];
    uint32_t w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13, w14, w15;

    Round(a, &b, c, d, &e, f1(b, c, d), K1, w0 = readBE32(chunk + 0));
    Round(e, &a, b, c, &d, f1(a, b, c), K1, w1 = readBE32(chunk + 4));
    Round(d, &e, a, b, &c, f1(e, a, b), K1, w2 = readBE32(chunk + 8));
    Round(c, &d, e, a, &b, f1(d, e, a), K1, w3 = readBE32(chunk + 12));
    Round(b, &c, d, e, &a, f1(c, d, e), K1, w4 = readBE32(chunk + 16));
    Round(a, &b, c, d, &e, f1(b, c, d), K1, w5 = readBE32(chunk + 20));
    Round(e, &a, b, c, &d, f1(a, b, c), K1, w6 = readBE32(chunk + 24));
    Round(d, &e, a, b, &c, f1(e, a, b), K1, w7 = readBE32(chunk + 28));
    Round(c, &d, e, a, &b, f1(d, e, a), K1, w8 = readBE32(chunk + 32));
    Round(b, &c, d, e, &a, f1(c, d, e), K1, w9 = readBE32(chunk + 36));
    Round(a, &b, c, d, &e, f1(b, c, d), K1, w10 = readBE32(chunk + 40));
    Round(e, &a, b, c, &d, f1(a, b, c), K1, w11 = readBE32(chunk + 44));
    Round(d, &e, a, b, &c, f1(e, a, b), K1, w12 = readBE32(chunk + 48));
    Round(c, &d, e, a, &b, f1(d, e, a), K1, w13 = readBE32(chunk + 52));
    Round(b, &c, d, e, &a, f1(c, d, e), K1, w14 = readBE32(chunk + 56));
    Round(a, &b, c, d, &e, f1(b, c, d), K1, w15 = readBE32(chunk + 60));

    Round(e, &a, b, c, &d, f1(a, b, c), K1, w0 = left(w0 ^ w13 ^ w8 ^ w2));
    Round(d, &e, a, b, &c, f1(e, a, b), K1, w1 = left(w1 ^ w14 ^ w9 ^ w3));
    Round(c, &d, e, a, &b, f1(d, e, a), K1, w2 = left(w2 ^ w15 ^ w10 ^ w4));
    Round(b, &c, d, e, &a, f1(c, d, e), K1, w3 = left(w3 ^ w0 ^ w11 ^ w5));
    Round(a, &b, c, d, &e, f2(b, c, d), K2, w4 = left(w4 ^ w1 ^ w12 ^ w6));
    Round(e, &a, b, c, &d, f2(a, b, c), K2, w5 = left(w5 ^ w2 ^ w13 ^ w7));
    Round(d, &e, a, b, &c, f2(e, a, b), K2, w6 = left(w6 ^ w3 ^ w14 ^ w8));
    Round(c, &d, e, a, &b, f2(d, e, a), K2, w7 = left(w7 ^ w4 ^ w15 ^ w9));
    Round(b, &c, d, e, &a, f2(c, d, e), K2, w8 = left(w8 ^ w5 ^ w0 ^ w10));
    Round(a, &b, c, d, &e, f2(b, c, d), K2, w9 = left(w9 ^ w6 ^ w1 ^ w11));
    Round(e, &a, b, c, &d, f2(a, b, c), K2, w10 = left(w10 ^ w7 ^ w2 ^ w12));
    Round(d, &e, a, b, &c, f2(e, a, b), K2, w11 = left(w11 ^ w8 ^ w3 ^ w13));
    Round(c, &d, e, a, &b, f2(d, e, a), K2, w12 = left(w12 ^ w9 ^ w4 ^ w14));
    Round(b, &c, d, e, &a, f2(c, d, e), K2, w13 = left(w13 ^ w10 ^ w5 ^ w15));
    Round(a, &b, c, d, &e, f2(b, c, d), K2, w14 = left(w14 ^ w11 ^ w6 ^ w0));
    Round(e, &a, b, c, &d, f2(a, b, c), K2, w15 = left(w15 ^ w12 ^ w7 ^ w1));

    Round(d, &e, a, b, &c, f2(e, a, b), K2, w0 = left(w0 ^ w13 ^ w8 ^ w2));
    Round(c, &d, e, a, &b, f2(d, e, a), K2, w1 = left(w1 ^ w14 ^ w9 ^ w3));
    Round(b, &c, d, e, &a, f2(c, d, e), K2, w2 = left(w2 ^ w15 ^ w10 ^ w4));
    Round(a, &b, c, d, &e, f2(b, c, d), K2, w3 = left(w3 ^ w0 ^ w11 ^ w5));
    Round(e, &a, b, c, &d, f2(a, b, c), K2, w4 = left(w4 ^ w1 ^ w12 ^ w6));
    Round(d, &e, a, b, &c, f2(e, a, b), K2, w5 = left(w5 ^ w2 ^ w13 ^ w7));
    Round(c, &d, e, a, &b, f2(d, e, a), K2, w6 = left(w6 ^ w3 ^ w14 ^ w8));
    Round(b, &c, d, e, &a, f2(c, d, e), K2, w7 = left(w7 ^ w4 ^ w15 ^ w9));
    Round(a, &b, c, d, &e, f3(b, c, d), K3, w8 = left(w8 ^ w5 ^ w0 ^ w10));
    Round(e, &a, b, c, &d, f3(a, b, c), K3, w9 = left(w9 ^ w6 ^ w1 ^ w11));
    Round(d, &e, a, b, &c, f3(e, a, b), K3, w10 = left(w10 ^ w7 ^ w2 ^ w12));
    Round(c, &d, e, a, &b, f3(d, e, a), K3, w11 = left(w11 ^ w8 ^ w3 ^ w13));
    Round(b, &c, d, e, &a, f3(c, d, e), K3, w12 = left(w12 ^ w9 ^ w4 ^ w14));
    Round(a, &b, c, d, &e, f3(b, c, d), K3, w13 = left(w13 ^ w10 ^ w5 ^ w15));
    Round(e, &a, b, c, &d, f3(a, b, c), K3, w14 = left(w14 ^ w11 ^ w6 ^ w0));
    Round(d, &e, a, b, &c, f3(e, a, b), K3, w15 = left(w15 ^ w12 ^ w7 ^ w1));

    Round(c, &d, e, a, &b, f3(d, e, a), K3, w0 = left(w0 ^ w13 ^ w8 ^ w2));
    Round(b, &c, d, e, &a, f3(c, d, e), K3, w1 = left(w1 ^ w14 ^ w9 ^ w3));
    Round(a, &b, c, d, &e, f3(b, c, d), K3, w2 = left(w2 ^ w15 ^ w10 ^ w4));
    Round(e, &a, b, c, &d, f3(a, b, c), K3, w3 = left(w3 ^ w0 ^ w11 ^ w5));
    Round(d, &e, a, b, &c, f3(e, a, b), K3, w4 = left(w4 ^ w1 ^ w12 ^ w6));
    Round(c, &d, e, a, &b, f3(d, e, a), K3, w5 = left(w5 ^ w2 ^ w13 ^ w7));
    Round(b, &c, d, e, &a, f3(c, d, e), K3, w6 = left(w6 ^ w3 ^ w14 ^ w8));
    Round(a, &b, c, d, &e, f3(b, c, d), K3, w7 = left(w7 ^ w4 ^ w15 ^ w9));
    Round(e, &a, b, c, &d, f3(a, b, c), K3, w8 = left(w8 ^ w5 ^ w0 ^ w10));
    Round(d, &e, a, b, &c, f3(e, a, b), K3, w9 = left(w9 ^ w6 ^ w1 ^ w11));
    Round(c, &d, e, a, &b, f3(d, e, a), K3, w10 = left(w10 ^ w7 ^ w2 ^ w12));
    Round(b, &c, d, e, &a, f3(c, d, e), K3, w11 = left(w11 ^ w8 ^ w3 ^ w13));
    Round(a, &b, c, d, &e, f2(b, c, d), K4, w12 = left(w12 ^ w9 ^ w4 ^ w14));
    Round(e, &a, b, c, &d, f2(a, b, c), K4, w13 = left(w13 ^ w10 ^ w5 ^ w15));
    Round(d, &e, a, b, &c, f2(e, a, b), K4, w14 = left(w14 ^ w11 ^ w6 ^ w0));
    Round(c, &d, e, a, &b, f2(d, e, a), K4, w15 = left(w15 ^ w12 ^ w7 ^ w1));

    Round(b, &c, d, e, &a, f2(c, d, e), K4, w0 = left(w0 ^ w13 ^ w8 ^ w2));
    Round(a, &b, c, d, &e, f2(b, c, d), K4, w1 = left(w1 ^ w14 ^ w9 ^ w3));
    Round(e, &a, b, c, &d, f2(a, b, c), K4, w2 = left(w2 ^ w15 ^ w10 ^ w4));
    Round(d, &e, a, b, &c, f2(e, a, b), K4, w3 = left(w3 ^ w0 ^ w11 ^ w5));
    Round(c, &d, e, a, &b, f2(d, e, a), K4, w4 = left(w4 ^ w1 ^ w12 ^ w6));
    Round(b, &c, d, e, &a, f2(c, d, e), K4, w5 = left(w5 ^ w2 ^ w13 ^ w7));
    Round(a, &b, c, d, &e, f2(b, c, d), K4, w6 = left(w6 ^ w3 ^ w14 ^ w8));
    Round(e, &a, b, c, &d, f2(a, b, c), K4, w7 = left(w7 ^ w4 ^ w15 ^ w9));
    Round(d, &e, a, b, &c, f2(e, a, b), K4, w8 = left(w8 ^ w5 ^ w0 ^ w10));
    Round(c, &d, e, a, &b, f2(d, e, a), K4, w9 = left(w9 ^ w6 ^ w1 ^ w11));
    Round(b, &c, d, e, &a, f2(c, d, e), K4, w10 = left(w10 ^ w7 ^ w2 ^ w12));
    Round(a, &b, c, d, &e, f2(b, c, d), K4, w11 = left(w11 ^ w8 ^ w3 ^ w13));
    Round(e, &a, b, c, &d, f2(a, b, c), K4, w12 = left(w12 ^ w9 ^ w4 ^ w14));
    Round(d, &e, a, b, &c, f2(e, a, b), K4, left(w13 ^ w10 ^ w5 ^ w15));
    Round(c, &d, e, a, &b, f2(d, e, a), K4, left(w14 ^ w11 ^ w6 ^ w0));
    Round(b, &c, d, e, &a, f2(c, d, e), K4, left(w15 ^ w12 ^ w7 ^ w1));

    s[0] += a;
    s[1] += b;
    s[2] += c;
    s[3] += d;
    s[4] += e;
}

// Hàm write
static inline void dsha1_write(DSHA1 *ctx, const unsigned char *data, size_t len) {
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

// Hàm finalize
static inline void dsha1_finalize(DSHA1 *ctx, unsigned char hash[OUTPUT_SIZE]) {
    const unsigned char pad[64] = {0x80};
    unsigned char sizedesc[8];
    writeBE64(sizedesc, ctx->bytes << 3);
    dsha1_write(ctx, pad, 1 + ((119 - (ctx->bytes % 64)) % 64));
    dsha1_write(ctx, sizedesc, 8);
    writeBE32(hash, ctx->s[0]);
    writeBE32(hash + 4, ctx->s[1]);
    writeBE32(hash + 8, ctx->s[2]);
    writeBE32(hash + 12, ctx->s[3]);
    writeBE32(hash + 16, ctx->s[4]);
}

// Hàm reset
static inline void dsha1_reset(DSHA1 *ctx) {
    dsha1_init(ctx);
}

#endif
