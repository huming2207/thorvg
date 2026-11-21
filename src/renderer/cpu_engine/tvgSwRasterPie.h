/*
 * Copyright (c) 2026 ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _TVG_SW_RASTER_PIE_H_
#define _TVG_SW_RASTER_PIE_H_

#include <cstdint>

/* ESP32-P4 and ESP32-S31 PIE (Xespv) kernels.

   - PIE accepts only x8-x15 and x24-x31 as the address and the scalar operands,
     so the operands are bound to a0-a5 explicitly.
   - The vector loops work on 16 bytes aligned addresses, the scalar loops handle the rest.
   - The multiplications only truncate (FLOOR), so the results are identical to the scalar code:
       ALPHA_BLEND(d, ia) = (d * (ia + 1)) >> 8
       MULTIPLY(d, 255 - a) = d - ((d * (a + 1)) >> 8)  (8 bits masks)
   - SAR and the rounding mode (CFG) are set per call and restored afterwards.
     ESP-IDF saves the PIE state per task, but it must not be used in ISRs or critical sections.
     On ESP32-S31, PIE is available on core 1 only: ESP-IDF migrates a task to core 1 on its first PIE instruction. */

static inline uint32_t _pieHead(const void* ptr, uint32_t size)
{
    return ((16 - (reinterpret_cast<uintptr_t>(ptr) & 15)) & 15) / size;
}

// dst[i] = val
static inline void pieFill32(uint32_t* dst, uint32_t val, uint32_t len)
{
    auto head = _pieHead(dst, sizeof(uint32_t));
    if (head > len) head = len;
    for (uint32_t i = 0; i < head; ++i) *dst++ = val;
    len -= head;

    if (auto n = len / 4) {
        register uint32_t* d asm("a0") = dst;
        register uint32_t cnt asm("a1") = n;
        register const uint32_t* v asm("a2") = &val;
        asm volatile(
            "esp.vldbc.32.ip q0, %[v], 0\n"
            "1:\n"
            "esp.vst.128.ip q0, %[d], 16\n"
            "addi %[cnt], %[cnt], -1\n"
            "bnez %[cnt], 1b\n"
            : [d] "+r"(d), [cnt] "+r"(cnt)
            : [v] "r"(v)
            : "memory");
        dst = d;
        len -= n * 4;
    }
    while (len--) *dst++ = val;
}

// dst[i] = val
static inline void pieFill16(uint16_t* dst, uint16_t val, uint32_t len)
{
    auto head = _pieHead(dst, sizeof(uint16_t));
    if (head > len) head = len;
    for (uint32_t i = 0; i < head; ++i) *dst++ = val;
    len -= head;

    if (auto n = len / 8) {
        register uint16_t* d asm("a0") = dst;
        register uint32_t cnt asm("a1") = n;
        register const uint16_t* v asm("a2") = &val;
        asm volatile(
            "esp.vldbc.16.ip q0, %[v], 0\n"
            "1:\n"
            "esp.vst.128.ip q0, %[d], 16\n"
            "addi %[cnt], %[cnt], -1\n"
            "bnez %[cnt], 1b\n"
            : [d] "+r"(d), [cnt] "+r"(cnt)
            : [v] "r"(v)
            : "memory");
        dst = d;
        len -= n * 8;
    }
    while (len--) *dst++ = val;
}

// dst[i] = src + ALPHA_BLEND(dst[i], 255 - A(src)), the premultiplied source over in 32 bits
static inline void pieBlend32(uint32_t* dst, uint32_t src, uint32_t len)
{
    auto ia = 255 - (src >> 24);
    auto scalar = [&](uint32_t d) {
        return src + ((((d >> 8) & 0x00ff00ff) * (ia + 1)) & 0xff00ff00) + ((((d & 0x00ff00ff) * (ia + 1)) >> 8) & 0x00ff00ff);
    };

    // the factor ia + 1 must fit in 8 bits
    uint32_t head = (ia == 255) ? len : _pieHead(dst, sizeof(uint32_t));
    if (head > len) head = len;
    for (uint32_t i = 0; i < head; ++i, ++dst) *dst = scalar(*dst);
    len -= head;

    if (auto n = len / 4) {
        uint8_t factor = ia + 1;
        register uint32_t* d asm("a0") = dst;
        register uint32_t cnt asm("a1") = n;
        register const uint32_t* s asm("a2") = &src;
        register const uint8_t* f asm("a3") = &factor;
        register uint32_t cfg asm("a4");
        register uint32_t sar asm("a5");
        register uint32_t tmp asm("t3");
        asm volatile(
            "esp.movx.r.cfg %[cfg]\n"
            "esp.movx.r.sar %[sar]\n"
            "andi %[tmp], %[cfg], -241\n"   // vxrm = FLOOR
            "esp.movx.w.cfg %[tmp]\n"
            "li %[tmp], 8\n"
            "esp.movx.w.sar %[tmp]\n"
            "esp.vldbc.32.ip q1, %[s], 0\n"
            "esp.vldbc.8.ip q2, %[f], 0\n"
            "1:\n"
            "esp.vld.128.ip q0, %[d], 0\n"
            "esp.vmul.u8 q0, q0, q2\n"      // (d * (ia + 1)) >> 8 per channel
            "esp.vadd.u8 q0, q0, q1\n"      // + src, the channels never exceed 255
            "esp.vst.128.ip q0, %[d], 16\n"
            "addi %[cnt], %[cnt], -1\n"
            "bnez %[cnt], 1b\n"
            "esp.movx.w.cfg %[cfg]\n"
            "esp.movx.w.sar %[sar]\n"
            : [d] "+r"(d), [cnt] "+r"(cnt), [cfg] "=&r"(cfg), [sar] "=&r"(sar), [tmp] "=&r"(tmp)
            : [s] "r"(s), [f] "r"(f)
            : "memory");
        dst = d;
        len -= n * 4;
    }
    for (; len > 0; --len, ++dst) *dst = scalar(*dst);
}

// dst[i] = val + MULTIPLY(dst[i], 255 - alpha), the premultiplied source over in 8 bits (val <= alpha)
static inline void pieBlend8(uint8_t* dst, uint8_t val, uint8_t alpha, uint32_t len)
{
    auto scalar = [&](uint8_t d) {
        return static_cast<uint8_t>(val + ((d * (255 - alpha) + 0xff) >> 8));
    };

    // the factor alpha + 1 must fit in 8 bits
    uint32_t head = (alpha == 255) ? len : _pieHead(dst, sizeof(uint8_t));
    if (head > len) head = len;
    for (uint32_t i = 0; i < head; ++i, ++dst) *dst = scalar(*dst);
    len -= head;

    if (auto n = len / 16) {
        uint8_t factor = alpha + 1;
        register uint8_t* d asm("a0") = dst;
        register uint32_t cnt asm("a1") = n;
        register const uint8_t* v asm("a2") = &val;
        register const uint8_t* f asm("a3") = &factor;
        register uint32_t cfg asm("a4");
        register uint32_t sar asm("a5");
        register uint32_t tmp asm("t3");
        asm volatile(
            "esp.movx.r.cfg %[cfg]\n"
            "esp.movx.r.sar %[sar]\n"
            "andi %[tmp], %[cfg], -241\n"   // vxrm = FLOOR
            "esp.movx.w.cfg %[tmp]\n"
            "li %[tmp], 8\n"
            "esp.movx.w.sar %[tmp]\n"
            "esp.vldbc.8.ip q1, %[v], 0\n"
            "esp.vldbc.8.ip q2, %[f], 0\n"
            "1:\n"
            "esp.vld.128.ip q0, %[d], 0\n"
            "esp.vmul.u8 q3, q0, q2\n"      // (d * (alpha + 1)) >> 8
            "esp.vsub.u8 q0, q0, q3\n"      // d - that = MULTIPLY(d, 255 - alpha)
            "esp.vadd.u8 q0, q0, q1\n"      // + val, never exceeds 255
            "esp.vst.128.ip q0, %[d], 16\n"
            "addi %[cnt], %[cnt], -1\n"
            "bnez %[cnt], 1b\n"
            "esp.movx.w.cfg %[cfg]\n"
            "esp.movx.w.sar %[sar]\n"
            : [d] "+r"(d), [cnt] "+r"(cnt), [cfg] "=&r"(cfg), [sar] "=&r"(sar), [tmp] "=&r"(tmp)
            : [v] "r"(v), [f] "r"(f)
            : "memory");
        dst = d;
        len -= n * 16;
    }
    for (; len > 0; --len, ++dst) *dst = scalar(*dst);
}

// dst[i] = val + ALPHA_BLEND(dst[i], 255 - alpha), the premultiplied source over of the gray target (val <= alpha)
static inline void pieAlphaBlend8(uint8_t* dst, uint8_t val, uint8_t alpha, uint32_t len)
{
    if (alpha == 0) return;  // val is 0 as well, dst stays

    auto scalar = [&](uint8_t d) {
        return static_cast<uint8_t>(val + ((d * (256 - alpha)) >> 8));
    };

    uint32_t head = _pieHead(dst, sizeof(uint8_t));
    if (head > len) head = len;
    for (uint32_t i = 0; i < head; ++i, ++dst) *dst = scalar(*dst);
    len -= head;

    if (auto n = len / 16) {
        uint8_t factor = 256 - alpha;
        register uint8_t* d asm("a0") = dst;
        register uint32_t cnt asm("a1") = n;
        register const uint8_t* v asm("a2") = &val;
        register const uint8_t* f asm("a3") = &factor;
        register uint32_t cfg asm("a4");
        register uint32_t sar asm("a5");
        register uint32_t tmp asm("t3");
        asm volatile(
            "esp.movx.r.cfg %[cfg]\n"
            "esp.movx.r.sar %[sar]\n"
            "andi %[tmp], %[cfg], -241\n"   // vxrm = FLOOR
            "esp.movx.w.cfg %[tmp]\n"
            "li %[tmp], 8\n"
            "esp.movx.w.sar %[tmp]\n"
            "esp.vldbc.8.ip q1, %[v], 0\n"
            "esp.vldbc.8.ip q2, %[f], 0\n"
            "1:\n"
            "esp.vld.128.ip q0, %[d], 0\n"
            "esp.vmul.u8 q0, q0, q2\n"      // (d * (256 - alpha)) >> 8
            "esp.vadd.u8 q0, q0, q1\n"      // + val, never exceeds 255
            "esp.vst.128.ip q0, %[d], 16\n"
            "addi %[cnt], %[cnt], -1\n"
            "bnez %[cnt], 1b\n"
            "esp.movx.w.cfg %[cfg]\n"
            "esp.movx.w.sar %[sar]\n"
            : [d] "+r"(d), [cnt] "+r"(cnt), [cfg] "=&r"(cfg), [sar] "=&r"(sar), [tmp] "=&r"(tmp)
            : [v] "r"(v), [f] "r"(f)
            : "memory");
        dst = d;
        len -= n * 16;
    }
    for (; len > 0; --len, ++dst) *dst = scalar(*dst);
}

#endif // _TVG_SW_RASTER_PIE_H_
