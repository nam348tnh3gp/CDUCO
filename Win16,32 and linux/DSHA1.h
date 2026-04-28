#ifndef DSHA1_H
#define DSHA1_H

#include <stdint.h>
#include <string.h>

#define DSHA1_OUTPUT_SIZE 20

typedef struct {
    uint32_t s[5];
    unsigned char buf[64];
    uint64_t bytes;
} DSHA1_CTX;

// Các hằng số
static const uint32_t DSHA1_K1 = 0x5A827999ul;
static const uint32_t DSHA1_K2 = 0x6ED9EBA1ul;
static const uint32_t DSHA1_K3 = 0x8F1BBCDCul;
static const uint32_t DSHA1_K4 = 0xCA62C1D6ul;

// Hàm nội tuyến
static inline uint32_t dsha1_f1(uint32_t b, uint32_t c, uint32_t d) { 
    return d ^ (b & (c ^ d)); 
}

static inline uint32_t dsha1_f2(uint32_t b, uint32_t c, uint32_t d) { 
    return b ^ c ^ d; 
}

static inline uint32_t dsha1_f3(uint32_t b, uint32_t c, uint32_t d) { 
    return (b & c) | (d & (b | c)); 
}

static inline uint32_t dsha1_left(uint32_t x) { 
    return (x << 1) | (x >> 31); 
}

static inline void dsha1_round(uint32_t a, uint32_t *b, uint32_t c, uint32_t d, uint32_t *e,
                                uint32_t f, uint32_t k, uint32_t w) {
    *e += ((a << 5) | (a >> 27)) + f + k + w;
    *b = (*b << 30) | (*b >> 2);
}

// Hàm đọc/ghi big-endian
static inline uint32_t dsha1_readBE32(const unsigned char *ptr) {
    return ((uint32_t)ptr[0] << 24) |
           ((uint32_t)ptr[1] << 16) |
           ((uint32_t)ptr[2] << 8)  |
           ((uint32_t)ptr[3]);
}

static inline void dsha1_writeBE32(unsigned char *ptr, uint32_t x) {
    ptr[0] = (x >> 24) & 0xFF;
    ptr[1] = (x >> 16) & 0xFF;
    ptr[2] = (x >> 8) & 0xFF;
    ptr[3] = x & 0xFF;
}

static inline void dsha1_writeBE64(unsigned char *ptr, uint64_t x) {
    ptr[0] = (x >> 56) & 0xFF;
    ptr[1] = (x >> 48) & 0xFF;
    ptr[2] = (x >> 40) & 0xFF;
    ptr[3] = (x >> 32) & 0xFF;
    ptr[4] = (x >> 24) & 0xFF;
    ptr[5] = (x >> 16) & 0xFF;
    ptr[6] = (x >> 8) & 0xFF;
    ptr[7] = x & 0xFF;
}

// Khởi tạo context
static inline void dsha1_init(DSHA1_CTX *ctx) {
    ctx->s[0] = 0x67452301ul;
    ctx->s[1] = 0xEFCDAB89ul;
    ctx->s[2] = 0x98BADCFEul;
    ctx->s[3] = 0x10325476ul;
    ctx->s[4] = 0xC3D2E1F0ul;
    ctx->bytes = 0;
    memset(ctx->buf, 0, sizeof(ctx->buf));
}

// Transform function
static void dsha1_transform(uint32_t *s, const unsigned char *chunk) {
    uint32_t a = s[0], b = s[1], c = s[2], d = s[3], e = s[4];
    uint32_t w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13, w14, w15;

    // Vòng 1
    dsha1_round(a, &b, c, d, &e, dsha1_f1(b, c, d), DSHA1_K1, w0 = dsha1_readBE32(chunk + 0));
    dsha1_round(e, &a, b, c, &d, dsha1_f1(a, b, c), DSHA1_K1, w1 = dsha1_readBE32(chunk + 4));
    dsha1_round(d, &e, a, b, &c, dsha1_f1(e, a, b), DSHA1_K1, w2 = dsha1_readBE32(chunk + 8));
    dsha1_round(c, &d, e, a, &b, dsha1_f1(d, e, a), DSHA1_K1, w3 = dsha1_readBE32(chunk + 12));
    dsha1_round(b, &c, d, e, &a, dsha1_f1(c, d, e), DSHA1_K1, w4 = dsha1_readBE32(chunk + 16));
    dsha1_round(a, &b, c, d, &e, dsha1_f1(b, c, d), DSHA1_K1, w5 = dsha1_readBE32(chunk + 20));
    dsha1_round(e, &a, b, c, &d, dsha1_f1(a, b, c), DSHA1_K1, w6 = dsha1_readBE32(chunk + 24));
    dsha1_round(d, &e, a, b, &c, dsha1_f1(e, a, b), DSHA1_K1, w7 = dsha1_readBE32(chunk + 28));
    dsha1_round(c, &d, e, a, &b, dsha1_f1(d, e, a), DSHA1_K1, w8 = dsha1_readBE32(chunk + 32));
    dsha1_round(b, &c, d, e, &a, dsha1_f1(c, d, e), DSHA1_K1, w9 = dsha1_readBE32(chunk + 36));
    dsha1_round(a, &b, c, d, &e, dsha1_f1(b, c, d), DSHA1_K1, w10 = dsha1_readBE32(chunk + 40));
    dsha1_round(e, &a, b, c, &d, dsha1_f1(a, b, c), DSHA1_K1, w11 = dsha1_readBE32(chunk + 44));
    dsha1_round(d, &e, a, b, &c, dsha1_f1(e, a, b), DSHA1_K1, w12 = dsha1_readBE32(chunk + 48));
    dsha1_round(c, &d, e, a, &b, dsha1_f1(d, e, a), DSHA1_K1, w13 = dsha1_readBE32(chunk + 52));
    dsha1_round(b, &c, d, e, &a, dsha1_f1(c, d, e), DSHA1_K1, w14 = dsha1_readBE32(chunk + 56));
    dsha1_round(a, &b, c, d, &e, dsha1_f1(b, c, d), DSHA1_K1, w15 = dsha1_readBE32(chunk + 60));

    // Vòng 2
    dsha1_round(e, &a, b, c, &d, dsha1_f1(a, b, c), DSHA1_K1, w0 = dsha1_left(w0 ^ w13 ^ w8 ^ w2));
    dsha1_round(d, &e, a, b, &c, dsha1_f1(e, a, b), DSHA1_K1, w1 = dsha1_left(w1 ^ w14 ^ w9 ^ w3));
    dsha1_round(c, &d, e, a, &b, dsha1_f1(d, e, a), DSHA1_K1, w2 = dsha1_left(w2 ^ w15 ^ w10 ^ w4));
    dsha1_round(b, &c, d, e, &a, dsha1_f1(c, d, e), DSHA1_K1, w3 = dsha1_left(w3 ^ w0 ^ w11 ^ w5));
    dsha1_round(a, &b, c, d, &e, dsha1_f2(b, c, d), DSHA1_K2, w4 = dsha1_left(w4 ^ w1 ^ w12 ^ w6));
    dsha1_round(e, &a, b, c, &d, dsha1_f2(a, b, c), DSHA1_K2, w5 = dsha1_left(w5 ^ w2 ^ w13 ^ w7));
    dsha1_round(d, &e, a, b, &c, dsha1_f2(e, a, b), DSHA1_K2, w6 = dsha1_left(w6 ^ w3 ^ w14 ^ w8));
    dsha1_round(c, &d, e, a, &b, dsha1_f2(d, e, a), DSHA1_K2, w7 = dsha1_left(w7 ^ w4 ^ w15 ^ w9));
    dsha1_round(b, &c, d, e, &a, dsha1_f2(c, d, e), DSHA1_K2, w8 = dsha1_left(w8 ^ w5 ^ w0 ^ w10));
    dsha1_round(a, &b, c, d, &e, dsha1_f2(b, c, d), DSHA1_K2, w9 = dsha1_left(w9 ^ w6 ^ w1 ^ w11));
    dsha1_round(e, &a, b, c, &d, dsha1_f2(a, b, c), DSHA1_K2, w10 = dsha1_left(w10 ^ w7 ^ w2 ^ w12));
    dsha1_round(d, &e, a, b, &c, dsha1_f2(e, a, b), DSHA1_K2, w11 = dsha1_left(w11 ^ w8 ^ w3 ^ w13));
    dsha1_round(c, &d, e, a, &b, dsha1_f2(d, e, a), DSHA1_K2, w12 = dsha1_left(w12 ^ w9 ^ w4 ^ w14));
    dsha1_round(b, &c, d, e, &a, dsha1_f2(c, d, e), DSHA1_K2, w13 = dsha1_left(w13 ^ w10 ^ w5 ^ w15));
    dsha1_round(a, &b, c, d, &e, dsha1_f2(b, c, d), DSHA1_K2, w14 = dsha1_left(w14 ^ w11 ^ w6 ^ w0));
    dsha1_round(e, &a, b, c, &d, dsha1_f2(a, b, c), DSHA1_K2, w15 = dsha1_left(w15 ^ w12 ^ w7 ^ w1));

    // Vòng 3
    dsha1_round(d, &e, a, b, &c, dsha1_f2(e, a, b), DSHA1_K2, w0 = dsha1_left(w0 ^ w13 ^ w8 ^ w2));
    dsha1_round(c, &d, e, a, &b, dsha1_f2(d, e, a), DSHA1_K2, w1 = dsha1_left(w1 ^ w14 ^ w9 ^ w3));
    dsha1_round(b, &c, d, e, &a, dsha1_f2(c, d, e), DSHA1_K2, w2 = dsha1_left(w2 ^ w15 ^ w10 ^ w4));
    dsha1_round(a, &b, c, d, &e, dsha1_f2(b, c, d), DSHA1_K2, w3 = dsha1_left(w3 ^ w0 ^ w11 ^ w5));
    dsha1_round(e, &a, b, c, &d, dsha1_f2(a, b, c), DSHA1_K2, w4 = dsha1_left(w4 ^ w1 ^ w12 ^ w6));
    dsha1_round(d, &e, a, b, &c, dsha1_f2(e, a, b), DSHA1_K2, w5 = dsha1_left(w5 ^ w2 ^ w13 ^ w7));
    dsha1_round(c, &d, e, a, &b, dsha1_f2(d, e, a), DSHA1_K2, w6 = dsha1_left(w6 ^ w3 ^ w14 ^ w8));
    dsha1_round(b, &c, d, e, &a, dsha1_f2(c, d, e), DSHA1_K2, w7 = dsha1_left(w7 ^ w4 ^ w15 ^ w9));
    dsha1_round(a, &b, c, d, &e, dsha1_f3(b, c, d), DSHA1_K3, w8 = dsha1_left(w8 ^ w5 ^ w0 ^ w10));
    dsha1_round(e, &a, b, c, &d, dsha1_f3(a, b, c), DSHA1_K3, w9 = dsha1_left(w9 ^ w6 ^ w1 ^ w11));
    dsha1_round(d, &e, a, b, &c, dsha1_f3(e, a, b), DSHA1_K3, w10 = dsha1_left(w10 ^ w7 ^ w2 ^ w12));
    dsha1_round(c, &d, e, a, &b, dsha1_f3(d, e, a), DSHA1_K3, w11 = dsha1_left(w11 ^ w8 ^ w3 ^ w13));
    dsha1_round(b, &c, d, e, &a, dsha1_f3(c, d, e), DSHA1_K3, w12 = dsha1_left(w12 ^ w9 ^ w4 ^ w14));
    dsha1_round(a, &b, c, d, &e, dsha1_f3(b, c, d), DSHA1_K3, w13 = dsha1_left(w13 ^ w10 ^ w5 ^ w15));
    dsha1_round(e, &a, b, c, &d, dsha1_f3(a, b, c), DSHA1_K3, w14 = dsha1_left(w14 ^ w11 ^ w6 ^ w0));
    dsha1_round(d, &e, a, b, &c, dsha1_f3(e, a, b), DSHA1_K3, w15 = dsha1_left(w15 ^ w12 ^ w7 ^ w1));

    // Vòng 4
    dsha1_round(c, &d, e, a, &b, dsha1_f3(d, e, a), DSHA1_K3, w0 = dsha1_left(w0 ^ w13 ^ w8 ^ w2));
    dsha1_round(b, &c, d, e, &a, dsha1_f3(c, d, e), DSHA1_K3, w1 = dsha1_left(w1 ^ w14 ^ w9 ^ w3));
    dsha1_round(a, &b, c, d, &e, dsha1_f3(b, c, d), DSHA1_K3, w2 = dsha1_left(w2 ^ w15 ^ w10 ^ w4));
    dsha1_round(e, &a, b, c, &d, dsha1_f3(a, b, c), DSHA1_K3, w3 = dsha1_left(w3 ^ w0 ^ w11 ^ w5));
    dsha1_round(d, &e, a, b, &c, dsha1_f3(e, a, b), DSHA1_K3, w4 = dsha1_left(w4 ^ w1 ^ w12 ^ w6));
    dsha1_round(c, &d, e, a, &b, dsha1_f3(d, e, a), DSHA1_K3, w5 = dsha1_left(w5 ^ w2 ^ w13 ^ w7));
    dsha1_round(b, &c, d, e, &a, dsha1_f3(c, d, e), DSHA1_K3, w6 = dsha1_left(w6 ^ w3 ^ w14 ^ w8));
    dsha1_round(a, &b, c, d, &e, dsha1_f3(b, c, d), DSHA1_K3, w7 = dsha1_left(w7 ^ w4 ^ w15 ^ w9));
    dsha1_round(e, &a, b, c, &d, dsha1_f3(a, b, c), DSHA1_K3, w8 = dsha1_left(w8 ^ w5 ^ w0 ^ w10));
    dsha1_round(d, &e, a, b, &c, dsha1_f3(e, a, b), DSHA1_K3, w9 = dsha1_left(w9 ^ w6 ^ w1 ^ w11));
    dsha1_round(c, &d, e, a, &b, dsha1_f3(d, e, a), DSHA1_K3, w10 = dsha1_left(w10 ^ w7 ^ w2 ^ w12));
    dsha1_round(b, &c, d, e, &a, dsha1_f3(c, d, e), DSHA1_K3, w11 = dsha1_left(w11 ^ w8 ^ w3 ^ w13));
    dsha1_round(a, &b, c, d, &e, dsha1_f2(b, c, d), DSHA1_K4, w12 = dsha1_left(w12 ^ w9 ^ w4 ^ w14));
    dsha1_round(e, &a, b, c, &d, dsha1_f2(a, b, c), DSHA1_K4, w13 = dsha1_left(w13 ^ w10 ^ w5 ^ w15));
    dsha1_round(d, &e, a, b, &c, dsha1_f2(e, a, b), DSHA1_K4, w14 = dsha1_left(w14 ^ w11 ^ w6 ^ w0));
    dsha1_round(c, &d, e, a, &b, dsha1_f2(d, e, a), DSHA1_K4, w15 = dsha1_left(w15 ^ w12 ^ w7 ^ w1));

    // Vòng 5
    dsha1_round(b, &c, d, e, &a, dsha1_f2(c, d, e), DSHA1_K4, w0 = dsha1_left(w0 ^ w13 ^ w8 ^ w2));
    dsha1_round(a, &b, c, d, &e, dsha1_f2(b, c, d), DSHA1_K4, w1 = dsha1_left(w1 ^ w14 ^ w9 ^ w3));
    dsha1_round(e, &a, b, c, &d, dsha1_f2(a, b, c), DSHA1_K4, w2 = dsha1_left(w2 ^ w15 ^ w10 ^ w4));
    dsha1_round(d, &e, a, b, &c, dsha1_f2(e, a, b), DSHA1_K4, w3 = dsha1_left(w3 ^ w0 ^ w11 ^ w5));
    dsha1_round(c, &d, e, a, &b, dsha1_f2(d, e, a), DSHA1_K4, w4 = dsha1_left(w4 ^ w1 ^ w12 ^ w6));
    dsha1_round(b, &c, d, e, &a, dsha1_f2(c, d, e), DSHA1_K4, w5 = dsha1_left(w5 ^ w2 ^ w13 ^ w7));
    dsha1_round(a, &b, c, d, &e, dsha1_f2(b, c, d), DSHA1_K4, w6 = dsha1_left(w6 ^ w3 ^ w14 ^ w8));
    dsha1_round(e, &a, b, c, &d, dsha1_f2(a, b, c), DSHA1_K4, w7 = dsha1_left(w7 ^ w4 ^ w15 ^ w9));
    dsha1_round(d, &e, a, b, &c, dsha1_f2(e, a, b), DSHA1_K4, w8 = dsha1_left(w8 ^ w5 ^ w0 ^ w10));
    dsha1_round(c, &d, e, a, &b, dsha1_f2(d, e, a), DSHA1_K4, w9 = dsha1_left(w9 ^ w6 ^ w1 ^ w11));
    dsha1_round(b, &c, d, e, &a, dsha1_f2(c, d, e), DSHA1_K4, w10 = dsha1_left(w10 ^ w7 ^ w2 ^ w12));
    dsha1_round(a, &b, c, d, &e, dsha1_f2(b, c, d), DSHA1_K4, w11 = dsha1_left(w11 ^ w8 ^ w3 ^ w13));
    dsha1_round(e, &a, b, c, &d, dsha1_f2(a, b, c), DSHA1_K4, w12 = dsha1_left(w12 ^ w9 ^ w4 ^ w14));
    dsha1_round(d, &e, a, b, &c, dsha1_f2(e, a, b), DSHA1_K4, dsha1_left(w13 ^ w10 ^ w5 ^ w15));
    dsha1_round(c, &d, e, a, &b, dsha1_f2(d, e, a), DSHA1_K4, dsha1_left(w14 ^ w11 ^ w6 ^ w0));
    dsha1_round(b, &c, d, e, &a, dsha1_f2(c, d, e), DSHA1_K4, dsha1_left(w15 ^ w12 ^ w7 ^ w1));

    s[0] += a;
    s[1] += b;
    s[2] += c;
    s[3] += d;
    s[4] += e;
}

// Ghi dữ liệu vào context
static inline void dsha1_write(DSHA1_CTX *ctx, const unsigned char *data, size_t len) {
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

// Finalize và lấy hash
static inline void dsha1_finalize(DSHA1_CTX *ctx, unsigned char hash[DSHA1_OUTPUT_SIZE]) {
    const unsigned char pad[64] = {0x80};
    unsigned char sizedesc[8];
    dsha1_writeBE64(sizedesc, ctx->bytes << 3);
    dsha1_write(ctx, pad, 1 + ((119 - (ctx->bytes % 64)) % 64));
    dsha1_write(ctx, sizedesc, 8);
    dsha1_writeBE32(hash, ctx->s[0]);
    dsha1_writeBE32(hash + 4, ctx->s[1]);
    dsha1_writeBE32(hash + 8, ctx->s[2]);
    dsha1_writeBE32(hash + 12, ctx->s[3]);
    dsha1_writeBE32(hash + 16, ctx->s[4]);
}

// Reset context
static inline void dsha1_reset(DSHA1_CTX *ctx) {
    ctx->bytes = 0;
    dsha1_init(ctx);
}

// Warmup function
static inline void dsha1_warmup(DSHA1_CTX *ctx) {
    unsigned char warmup[20];
    dsha1_write(ctx, (const unsigned char *)"warmupwarmupwa", 20);
    dsha1_finalize(ctx, warmup);
    dsha1_reset(ctx);
}

#endif // DSHA1_H
