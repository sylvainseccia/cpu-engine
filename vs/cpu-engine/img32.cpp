#include "stdafx.h"

namespace cpu_ns_img32
{

void AlphaBlend(
    const byte* src, int srcW, int srcH,
    byte* dst, int dstW, int dstH,
    int srcX, int srcY,
    int dstX, int dstY,
    int blitW, int blitH)
{
    if (!src || !dst || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) return;
    if (blitW <= 0 || blitH <= 0) return;

    // Clip
    int w = blitW, h = blitH;
    clip_blit_rect(srcW, srcH, dstW, dstH, srcX, srcY, dstX, dstY, w, h);
    if (w <= 0 || h <= 0) return;

    const int srcPitch = srcW * 4;
    const int dstPitch = dstW * 4;

    // If src and dst pointers are identical (same surface) and regions overlap, behavior is undefined for blending.
    // If you need self-blend with overlap, you must handle it with a temp buffer (not done here for speed).

#if BLIT_AVX2
    const __m256i zero = _mm256_setzero_si256();
    const __m256i v255 = _mm256_set1_epi16(255);

    // Shuffle mask to replicate A into RGBA lanes within each 128-bit half after unpack:
    // After unpacklo/hi_epi8: [R0 G0 B0 A0 R1 G1 B1 A1 ...] as u16
    const __m256i shufA = _mm256_setr_epi8(
        6,7, 6,7, 6,7, 6,7,   14,15, 14,15, 14,15, 14,15,
        6,7, 6,7, 6,7, 6,7,   14,15, 14,15, 14,15, 14,15
    );

    for (int y = 0; y < h; ++y)
    {
        const byte* sRow = src + (srcY + y) * srcPitch + srcX * 4;
        byte* dRow = dst + (dstY + y) * dstPitch + dstX * 4;

        const int rowBytes = w * 4;
        int xBytes = 0;

        // 8 pixels / 32 bytes per iter
        for (; xBytes + 32 <= rowBytes; xBytes += 32)
        {
            __m256i s8 = _mm256_loadu_si256((const __m256i*)(sRow + xBytes));
            __m256i d8 = _mm256_loadu_si256((const __m256i*)(dRow + xBytes));

            __m256i sLo = _mm256_unpacklo_epi8(s8, zero);
            __m256i sHi = _mm256_unpackhi_epi8(s8, zero);
            __m256i dLo = _mm256_unpacklo_epi8(d8, zero);
            __m256i dHi = _mm256_unpackhi_epi8(d8, zero);

            __m256i aLo = _mm256_shuffle_epi8(sLo, shufA);
            __m256i aHi = _mm256_shuffle_epi8(sHi, shufA);

            __m256i invALo = _mm256_sub_epi16(v255, aLo);
            __m256i invAHi = _mm256_sub_epi16(v255, aHi);

            // If all invA == 0 => all alpha == 255 for this block: out = src (fast)
            if (_mm256_testz_si256(invALo, invALo) && _mm256_testz_si256(invAHi, invAHi))
            {
                _mm256_storeu_si256((__m256i*)(dRow + xBytes), s8);
                continue;
            }

            // out = src + dst * invA / 255  (premul SRC-over)
            __m256i dMulLo = _mm256_mullo_epi16(dLo, invALo);
            __m256i dMulHi = _mm256_mullo_epi16(dHi, invAHi);

            __m256i dScaledLo = div255_epu16(dMulLo);
            __m256i dScaledHi = div255_epu16(dMulHi);

            __m256i outLo = _mm256_add_epi16(sLo, dScaledLo);
            __m256i outHi = _mm256_add_epi16(sHi, dScaledHi);

            __m256i out8 = _mm256_packus_epi16(outLo, outHi);
            _mm256_storeu_si256((__m256i*)(dRow + xBytes), out8);
        }

        // Tail scalar pixels
        for (; xBytes < rowBytes; xBytes += 4)
        {
            const byte* sp = sRow + xBytes;
            byte* dp = dRow + xBytes;

            uint32_t sr = sp[0], sg = sp[1], sb = sp[2], sa = sp[3];
            uint32_t dr = dp[0], dg = dp[1], db = dp[2], da = dp[3];

            uint32_t invA = 255u - sa;

            dp[0] = (byte)std::min(255u, sr + div255_u32(dr * invA));
            dp[1] = (byte)std::min(255u, sg + div255_u32(dg * invA));
            dp[2] = (byte)std::min(255u, sb + div255_u32(db * invA));
            dp[3] = (byte)std::min(255u, sa + div255_u32(da * invA));
        }
    }
    return;
#endif

#if BLIT_SSE2
    const __m128i zero = _mm_setzero_si128();
    const __m128i v255 = _mm_set1_epi16(255);

    // SSE2 path: process 4 pixels (16 bytes) per iter. We use SSSE3 shuffle if available would help,
    // but to keep SSE2-only we do scalar alpha checks and SSE2 math on unpacked bytes.
    // For speed, we keep it simple: SSE2 for math, scalar for alpha replication.
    for (int y = 0; y < h; ++y)
    {
        const byte* sRow = src + (srcY + y) * srcPitch + srcX * 4;
        byte* dRow = dst + (dstY + y) * dstPitch + dstX * 4;

        const int rowBytes = w * 4;
        int xBytes = 0;

        for (; xBytes + 16 <= rowBytes; xBytes += 16)
        {
            // Load 4 pixels
            __m128i s8 = _mm_loadu_si128((const __m128i*)(sRow + xBytes));
            __m128i d8 = _mm_loadu_si128((const __m128i*)(dRow + xBytes));

            // Quick alpha=255 fast path (scalar check 4 bytes)
            const byte* sa = sRow + xBytes + 3;
            if (sa[0] == 255 && sa[4] == 255 && sa[8] == 255 && sa[12] == 255)
            {
                _mm_storeu_si128((__m128i*)(dRow + xBytes), s8);
                continue;
            }

            // Unpack to u16
            __m128i sLo = _mm_unpacklo_epi8(s8, zero); // 8 lanes u16: R0 G0 B0 A0 R1 G1 B1 A1
            __m128i sHi = _mm_unpackhi_epi8(s8, zero); // R2..A3
            __m128i dLo = _mm_unpacklo_epi8(d8, zero);
            __m128i dHi = _mm_unpackhi_epi8(d8, zero);

            // Build invA vectors (u16 lanes) for 2 pixels per half (SSE2, so do scalar replication)
            // Layout sLo lanes: [R0,G0,B0,A0,R1,G1,B1,A1]
            // Need invA replicated to 4 lanes per pixel.
            uint16_t a0 = (uint16_t)(sRow[xBytes + 3]);
            uint16_t a1 = (uint16_t)(sRow[xBytes + 7]);
            uint16_t a2 = (uint16_t)(sRow[xBytes + 11]);
            uint16_t a3 = (uint16_t)(sRow[xBytes + 15]);

            __m128i invLo = _mm_setr_epi16(
                (short)(255 - a0),(short)(255 - a0),(short)(255 - a0),(short)(255 - a0),
                (short)(255 - a1),(short)(255 - a1),(short)(255 - a1),(short)(255 - a1)
            );
            __m128i invHi = _mm_setr_epi16(
                (short)(255 - a2),(short)(255 - a2),(short)(255 - a2),(short)(255 - a2),
                (short)(255 - a3),(short)(255 - a3),(short)(255 - a3),(short)(255 - a3)
            );

            __m128i dMulLo = _mm_mullo_epi16(dLo, invLo);
            __m128i dMulHi = _mm_mullo_epi16(dHi, invHi);

            __m128i dScaledLo = div255_epu16_sse2(dMulLo);
            __m128i dScaledHi = div255_epu16_sse2(dMulHi);

            __m128i outLo = _mm_add_epi16(sLo, dScaledLo);
            __m128i outHi = _mm_add_epi16(sHi, dScaledHi);

            __m128i out8 = _mm_packus_epi16(outLo, outHi);
            _mm_storeu_si128((__m128i*)(dRow + xBytes), out8);
        }

        // Tail scalar
        for (; xBytes < rowBytes; xBytes += 4)
        {
            const byte* sp = sRow + xBytes;
            byte* dp = dRow + xBytes;

            uint32_t sr = sp[0], sg = sp[1], sb = sp[2], sa = sp[3];
            uint32_t dr = dp[0], dg = dp[1], db = dp[2], da = dp[3];

            uint32_t invA = 255u - sa;

            dp[0] = (byte)std::min(255u, sr + div255_u32(dr * invA));
            dp[1] = (byte)std::min(255u, sg + div255_u32(dg * invA));
            dp[2] = (byte)std::min(255u, sb + div255_u32(db * invA));
            dp[3] = (byte)std::min(255u, sa + div255_u32(da * invA));
        }
    }
    return;
#endif

    // Scalar fallback
    for (int y = 0; y < h; ++y)
    {
        const byte* sRow = src + (srcY + y) * srcPitch + srcX * 4;
        byte* dRow = dst + (dstY + y) * dstPitch + dstX * 4;

        for (int x = 0; x < w; ++x)
        {
            const byte* sp = sRow + x * 4;
            byte* dp = dRow + x * 4;

            uint32_t sr = sp[0], sg = sp[1], sb = sp[2], sa = sp[3];
            uint32_t dr = dp[0], dg = dp[1], db = dp[2], da = dp[3];

            uint32_t invA = 255u - sa;

            dp[0] = (byte)std::min(255u, sr + div255_u32(dr * invA));
            dp[1] = (byte)std::min(255u, sg + div255_u32(dg * invA));
            dp[2] = (byte)std::min(255u, sb + div255_u32(db * invA));
            dp[3] = (byte)std::min(255u, sa + div255_u32(da * invA));
        }
    }
}

// Premultiply RGBA8 in-place (or src->dst) : RGB = RGB * A / 255, A unchanged.
// Reuses helper(s): div255_epu16 / div255_epu16_sse2 / div255_u32 from previous code.
// Format: RGBA8 tightly packed (pitch = width*4). Processes full surface (no clipping here).

void Premultiply(
    const byte* src, byte* dst,
    int width, int height)
{
    if (!src || !dst || width <= 0 || height <= 0) return;

    const int bytes = width * height * 4;

#if BLIT_AVX2
    const __m256i zero = _mm256_setzero_si256();
    const __m256i v255 = _mm256_set1_epi16(255);

    // Replicate alpha (u16) to RGBA lanes within each 128-bit half after unpack.
    // u16 layout per 128-bit: [R0 G0 B0 A0 R1 G1 B1 A1 ...]
    const __m256i shufA = _mm256_setr_epi8(
        6,7, 6,7, 6,7, 6,7,   14,15, 14,15, 14,15, 14,15,
        6,7, 6,7, 6,7, 6,7,   14,15, 14,15, 14,15, 14,15
    );

    // Select alpha lanes (u16 lanes 3,7,11,15): -1 else 0
    const __m256i alphaMask16 = _mm256_setr_epi16(
        0,0,0,(short)-1,  0,0,0,(short)-1,
        0,0,0,(short)-1,  0,0,0,(short)-1
    );

    int i = 0;
    for (; i + 32 <= bytes; i += 32) // 8 pixels
    {
        __m256i s8 = _mm256_loadu_si256((const __m256i*)(src + i));

        __m256i lo = _mm256_unpacklo_epi8(s8, zero);
        __m256i hi = _mm256_unpackhi_epi8(s8, zero);

        __m256i aLo = _mm256_shuffle_epi8(lo, shufA);
        __m256i aHi = _mm256_shuffle_epi8(hi, shufA);

        // Fast path: if all alpha == 255 => unchanged
        __m256i invALo = _mm256_sub_epi16(v255, aLo);
        __m256i invAHi = _mm256_sub_epi16(v255, aHi);
        if (_mm256_testz_si256(invALo, invALo) && _mm256_testz_si256(invAHi, invAHi))
        {
            _mm256_storeu_si256((__m256i*)(dst + i), s8);
            continue;
        }

        // Multiply all channels by alpha and divide by 255
        __m256i mulLo = _mm256_mullo_epi16(lo, aLo);
        __m256i mulHi = _mm256_mullo_epi16(hi, aHi);

        __m256i premLo = div255_epu16(mulLo);
        __m256i premHi = div255_epu16(mulHi);

        // Restore alpha lanes from original (alpha must remain unchanged)
        premLo = _mm256_or_si256(_mm256_and_si256(alphaMask16, lo),
                                 _mm256_andnot_si256(alphaMask16, premLo));
        premHi = _mm256_or_si256(_mm256_and_si256(alphaMask16, hi),
                                 _mm256_andnot_si256(alphaMask16, premHi));

        __m256i out8 = _mm256_packus_epi16(premLo, premHi);
        _mm256_storeu_si256((__m256i*)(dst + i), out8);
    }

    // Tail scalar
    for (; i < bytes; i += 4)
    {
        uint32_t r = src[i+0], g = src[i+1], b = src[i+2], a = src[i+3];
        dst[i+3] = (byte)a;

        if (a == 255)
        {
            dst[i+0] = (byte)r; dst[i+1] = (byte)g; dst[i+2] = (byte)b;
        }
        else if (a == 0)
        {
            dst[i+0] = dst[i+1] = dst[i+2] = 0;
        }
        else
        {
            dst[i+0] = (byte)div255_u32(r * a);
            dst[i+1] = (byte)div255_u32(g * a);
            dst[i+2] = (byte)div255_u32(b * a);
        }
    }
    return;
#endif

#if BLIT_SSE2
    const __m128i zero = _mm_setzero_si128();

    int i = 0;
    for (; i + 16 <= bytes; i += 16) // 4 pixels
    {
        __m128i s8 = _mm_loadu_si128((const __m128i*)(src + i));

        // Quick alpha==255 check for 4 pixels (scalar)
        const byte* a = src + i + 3;
        if (a[0] == 255 && a[4] == 255 && a[8] == 255 && a[12] == 255)
        {
            _mm_storeu_si128((__m128i*)(dst + i), s8);
            continue;
        }

        __m128i lo = _mm_unpacklo_epi8(s8, zero); // R0 G0 B0 A0 R1 G1 B1 A1
        __m128i hi = _mm_unpackhi_epi8(s8, zero); // R2..A3

        uint16_t a0 = (uint16_t)a[0];
        uint16_t a1 = (uint16_t)a[4];
        uint16_t a2 = (uint16_t)a[8];
        uint16_t a3 = (uint16_t)a[12];

        __m128i aLo = _mm_setr_epi16((short)a0,(short)a0,(short)a0,(short)a0, (short)a1,(short)a1,(short)a1,(short)a1);
        __m128i aHi = _mm_setr_epi16((short)a2,(short)a2,(short)a2,(short)a2, (short)a3,(short)a3,(short)a3,(short)a3);

        __m128i mulLo = _mm_mullo_epi16(lo, aLo);
        __m128i mulHi = _mm_mullo_epi16(hi, aHi);

        __m128i premLo = div255_epu16_sse2(mulLo);
        __m128i premHi = div255_epu16_sse2(mulHi);

        // Restore alpha lanes (lane 3 and 7 in each 8-lane vector)
        const __m128i alphaMask = _mm_setr_epi16(0,0,0,(short)0xFFFF, 0,0,0,(short)0xFFFF);
        premLo = _mm_or_si128(_mm_and_si128(alphaMask, lo), _mm_andnot_si128(alphaMask, premLo));
        premHi = _mm_or_si128(_mm_and_si128(alphaMask, hi), _mm_andnot_si128(alphaMask, premHi));

        __m128i out8 = _mm_packus_epi16(premLo, premHi);
        _mm_storeu_si128((__m128i*)(dst + i), out8);
    }

    // Tail scalar
    for (; i < bytes; i += 4)
    {
        uint32_t r = src[i+0], g = src[i+1], b = src[i+2], a = src[i+3];
        dst[i+3] = (byte)a;

        if (a == 255)
        {
            dst[i+0] = (byte)r; dst[i+1] = (byte)g; dst[i+2] = (byte)b;
        }
        else if (a == 0)
        {
            dst[i+0] = dst[i+1] = dst[i+2] = 0;
        }
        else
        {
            dst[i+0] = (byte)div255_u32(r * a);
            dst[i+1] = (byte)div255_u32(g * a);
            dst[i+2] = (byte)div255_u32(b * a);
        }
    }
    return;
#endif

    // Scalar fallback
    for (int i = 0; i < bytes; i += 4)
    {
        uint32_t r = src[i+0], g = src[i+1], b = src[i+2], a = src[i+3];
        dst[i+3] = (byte)a;

        if (a == 255)
        {
            dst[i+0] = (byte)r; dst[i+1] = (byte)g; dst[i+2] = (byte)b;
        }
        else if (a == 0)
        {
            dst[i+0] = dst[i+1] = dst[i+2] = 0;
        }
        else
        {
            dst[i+0] = (byte)div255_u32(r * a);
            dst[i+1] = (byte)div255_u32(g * a);
            dst[i+2] = (byte)div255_u32(b * a);
        }
    }
}


// Unpremultiply RGBA8: RGB = RGB * 255 / A, A unchanged.
// Input must be RGBA8 *premultiplied* (RGB already multiplied by A/255).
// Output is RGBA8 *straight alpha*.
// Tightly packed (pitch = width*4).
//
// Note on performance:
// True SIMD unpremul is hard without gather (scale depends on per-pixel alpha).
// This implementation is nevertheless fast in practice thanks to block fast-paths
// (all A==255 => memcpy; all A==0 => zero) and a LUT for the division.

void Unpremultiply(
    const byte* src, byte* dst,
    int width, int height)
{
    if (!src || !dst || width <= 0 || height <= 0) return;

    const int pixels = width * height;
    const int bytes  = pixels * 4;

    // LUT: scale16[a] = round((255<<16)/a). scale16[0]=0.
    // rgb_out = (rgb_premul * scale16[a] + 0x8000) >> 16
    static uint32_t scale16[256];
    static bool inited = false;
    if (!inited)
    {
        scale16[0] = 0;
        for (int a = 1; a < 256; ++a)
            scale16[a] = (uint32_t)(((255u << 16) + (uint32_t)a / 2u) / (uint32_t)a);
        inited = true;
    }

#if BLIT_AVX2
    int i = 0;
    for (; i + 32 <= bytes; i += 32) // 8 pixels
    {
        // Fast-path checks on alpha bytes for this block
        // Read alpha bytes directly (every 4th byte).
        const byte* a = src + i + 3;

        // Check all-255 (opaque) quickly
        if (a[0]==255 && a[4]==255 && a[8]==255 && a[12]==255 && a[16]==255 && a[20]==255 && a[24]==255 && a[28]==255)
        {
            // Straight == premul when A=255
            std::memcpy(dst + i, src + i, 32);
            continue;
        }

        // Check all-0 quickly
        if (a[0]==0 && a[4]==0 && a[8]==0 && a[12]==0 && a[16]==0 && a[20]==0 && a[24]==0 && a[28]==0)
        {
            // Fully transparent => RGB must be 0, alpha remains 0
            // We can just zero 32 bytes.
            std::memset(dst + i, 0, 32);
            continue;
        }

        // Mixed alpha: do 8 scalar pixels (still decent; LUT avoids division)
        for (int k = 0; k < 32; k += 4)
        {
            const byte* sp = src + i + k;
            byte* dp = dst + i + k;

            uint32_t r = sp[0], g = sp[1], b = sp[2], A = sp[3];
            dp[3] = (byte)A;

            if (A == 0)
            {
                dp[0] = dp[1] = dp[2] = 0;
            }
            else if (A == 255)
            {
                dp[0] = (byte)r; dp[1] = (byte)g; dp[2] = (byte)b;
            }
            else
            {
                const uint32_t s = scale16[A]; // 16.16
                uint32_t ro = (r * s + 0x8000u) >> 16;
                uint32_t go = (g * s + 0x8000u) >> 16;
                uint32_t bo = (b * s + 0x8000u) >> 16;

                dp[0] = (byte)std::min(ro, 255u);
                dp[1] = (byte)std::min(go, 255u);
                dp[2] = (byte)std::min(bo, 255u);
            }
        }
    }

    // Tail
    for (; i < bytes; i += 4)
    {
        uint32_t r = src[i+0], g = src[i+1], b = src[i+2], A = src[i+3];
        dst[i+3] = (byte)A;

        if (A == 0) { dst[i+0]=dst[i+1]=dst[i+2]=0; }
        else if (A == 255) { dst[i+0]=(byte)r; dst[i+1]=(byte)g; dst[i+2]=(byte)b; }
        else
        {
            const uint32_t s = scale16[A];
            dst[i+0] = (byte)std::min((r * s + 0x8000u) >> 16, 255u);
            dst[i+1] = (byte)std::min((g * s + 0x8000u) >> 16, 255u);
            dst[i+2] = (byte)std::min((b * s + 0x8000u) >> 16, 255u);
        }
    }
    return;
#endif

#if BLIT_SSE2
    int i = 0;
    for (; i + 16 <= bytes; i += 16) // 4 pixels
    {
        const byte* a = src + i + 3;

        if (a[0]==255 && a[4]==255 && a[8]==255 && a[12]==255)
        {
            std::memcpy(dst + i, src + i, 16);
            continue;
        }
        if (a[0]==0 && a[4]==0 && a[8]==0 && a[12]==0)
        {
            std::memset(dst + i, 0, 16);
            continue;
        }

        for (int k = 0; k < 16; k += 4)
        {
            const byte* sp = src + i + k;
            byte* dp = dst + i + k;

            uint32_t r = sp[0], g = sp[1], b = sp[2], A = sp[3];
            dp[3] = (byte)A;

            if (A == 0)
            {
                dp[0] = dp[1] = dp[2] = 0;
            }
            else if (A == 255)
            {
                dp[0] = (byte)r; dp[1] = (byte)g; dp[2] = (byte)b;
            }
            else
            {
                const uint32_t s = scale16[A];
                dp[0] = (byte)std::min((r * s + 0x8000u) >> 16, 255u);
                dp[1] = (byte)std::min((g * s + 0x8000u) >> 16, 255u);
                dp[2] = (byte)std::min((b * s + 0x8000u) >> 16, 255u);
            }
        }
    }

    for (; i < bytes; i += 4)
    {
        uint32_t r = src[i+0], g = src[i+1], b = src[i+2], A = src[i+3];
        dst[i+3] = (byte)A;

        if (A == 0) { dst[i+0]=dst[i+1]=dst[i+2]=0; }
        else if (A == 255) { dst[i+0]=(byte)r; dst[i+1]=(byte)g; dst[i+2]=(byte)b; }
        else
        {
            const uint32_t s = scale16[A];
            dst[i+0] = (byte)std::min((r * s + 0x8000u) >> 16, 255u);
            dst[i+1] = (byte)std::min((g * s + 0x8000u) >> 16, 255u);
            dst[i+2] = (byte)std::min((b * s + 0x8000u) >> 16, 255u);
        }
    }
    return;
#endif

    // Scalar fallback
    for (int i = 0; i < bytes; i += 4)
    {
        uint32_t r = src[i+0], g = src[i+1], b = src[i+2], A = src[i+3];
        dst[i+3] = (byte)A;

        if (A == 0)
        {
            dst[i+0] = dst[i+1] = dst[i+2] = 0;
        }
        else if (A == 255)
        {
            dst[i+0] = (byte)r; dst[i+1] = (byte)g; dst[i+2] = (byte)b;
        }
        else
        {
            const uint32_t s = scale16[A];
            dst[i+0] = (byte)std::min((r * s + 0x8000u) >> 16, 255u);
            dst[i+1] = (byte)std::min((g * s + 0x8000u) >> 16, 255u);
            dst[i+2] = (byte)std::min((b * s + 0x8000u) >> 16, 255u);
        }
    }
}

bool AlphaBlendStraightOverOpaque(const byte* src, int srcW, int srcH, byte* dst, int dstW, int dstH, int srcX, int srcY, int dstX, int dstY, int blitW, int blitH)
{
	if ( blitW<=0 || blitH<=0 )
		return false;

	if ( dstX<0 ) { int d = -dstX; dstX = 0; srcX += d; blitW -= d; }
	if ( dstY<0 ) { int d = -dstY; dstY = 0; srcY += d; blitH -= d; }
	if ( dstX+blitW>dstW ) blitW = dstW - dstX;
	if ( dstY+blitH>dstH ) blitH = dstH - dstY;
	if ( srcX<0 ) { int d = -srcX; srcX = 0; dstX += d; blitW -= d; }
	if ( srcY<0 ) { int d = -srcY; srcY = 0; dstY += d; blitH -= d; }
	if ( srcX+blitW>srcW ) blitW = srcW - srcX;
	if ( srcY+blitH>srcH ) blitH = srcH - srcY;

	if ( blitW<=0 || blitH<=0 )
		return false;
	
	const int dstStride = dstW * 4;
	const int srcStride = srcW * 4;
	byte* drow = dst + dstY * dstStride + dstX * 4;
	const byte* srow = src + srcY * srcStride + srcX * 4;
	for ( int y=0 ; y<blitH ; ++y )
	{
		byte* d = drow;
		const byte* s = srow;
		for ( int x=0 ; x<blitW ; ++x )
		{
			const byte sa = s[3];
			if ( sa==255 )
			{
				d[0] = s[2];
				d[1] = s[1];
				d[2] = s[0];
			}
			else if ( sa!=0 )
			{
				const byte a = sa;
				const byte ia = 255 - a;
				d[0] = (byte)((s[2] * a + d[0] * ia + 127) / 255);
				d[1] = (byte)((s[1] * a + d[1] * ia + 127) / 255);
				d[2] = (byte)((s[0] * a + d[2] * ia + 127) / 255);
			}
			d += 4;
			s += 4;
		}
		drow += dstStride;
		srow += srcStride;
	}

	return true;
}

}