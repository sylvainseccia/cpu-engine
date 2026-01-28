#include "pch.h"

namespace cpu_img32
{

std::vector<byte>* g_pBlurScratch = nullptr;
std::vector<int>* g_pBlurDv = nullptr;

void Free()
{
	CPU_DELPTR(g_pBlurScratch);
	CPU_DELPTR(g_pBlurDv);
}

void AlphaBlend(const byte* src, int srcW, int srcH, byte* dst, int dstW, int dstH, int srcX, int srcY, int dstX, int dstY, int blitW, int blitH, XMFLOAT3* pTint)
{
	if ( src==nullptr || dst==nullptr || srcW<=0 || srcH<=0 || dstW<=0 || dstH<=0 )
		return;
	if ( blitW<=0 || blitH<=0 )
		return;

	// Clip
	int w = blitW, h = blitH;
	clip_blit_rect(srcW, srcH, dstW, dstH, srcX, srcY, dstX, dstY, w, h);
	if ( w<=0 || h<=0 )
		return;

	const int srcPitch = srcW * 4;
	const int dstPitch = dstW * 4;

	// If src and dst pointers are identical (same surface) and regions overlap, behavior is undefined for blending.
	// If you need self-blend with overlap, you must handle it with a temp buffer (not done here for speed).

	const __m128i zero = _mm_setzero_si128();
	const __m128i v255 = _mm_set1_epi16(255);

	__m128i tintLo;
	__m128i tintHi;
	ui16 tr = 0;
	ui16 tg = 0;
	ui16 tb = 0;
	if ( pTint )
	{
		auto f2u8 = [](float v) -> ui16
		{
			if ( v<0.0f ) v = 0.0f;
			if ( v>1.0f ) v = 1.0f;
			return (ui16)(v*255.f+0.5f);
		};
		tr = f2u8(pTint->z);
		tg = f2u8(pTint->y);
		tb = f2u8(pTint->x);
		tintLo = _mm_setr_epi16((short)tr,(short)tg,(short)tb,(short)255, (short)tr,(short)tg,(short)tb,(short)255);
		tintHi = _mm_setr_epi16((short)tr,(short)tg,(short)tb,(short)255, (short)tr,(short)tg,(short)tb,(short)255);
	}

	// SSE2 path: process 4 pixels (16 bytes) per iter. We use SSSE3 shuffle if available would help,
	// but to keep SSE2-only we do scalar alpha checks and SSE2 math on unpacked bytes.
	// For speed, we keep it simple: SSE2 for math, scalar for alpha replication.
	for ( int y=0 ; y<h ; ++y )
	{
		const byte* sRow = src + (srcY + y) * srcPitch + srcX * 4;
		byte* dRow = dst + (dstY + y) * dstPitch + dstX * 4;

		const int rowBytes = w * 4;
		int xBytes = 0;

		for ( ; xBytes+16<=rowBytes ; xBytes+=16 )
		{
			// Load 4 pixels
			__m128i s8 = _mm_loadu_si128((const __m128i*)(sRow + xBytes));
			__m128i d8 = _mm_loadu_si128((const __m128i*)(dRow + xBytes));

			// Quick alpha=255 fast path (scalar check 4 bytes)
			const byte* sa = sRow + xBytes + 3;
			if ( sa[0]==255 && sa[4]==255 && sa[8]==255 && sa[12]==255 && pTint==nullptr )
			{
				_mm_storeu_si128((__m128i*)(dRow + xBytes), s8);
				continue;
			}

			// Unpack to u16
			__m128i sLo = _mm_unpacklo_epi8(s8, zero); // 8 lanes u16: R0 G0 B0 A0 R1 G1 B1 A1
			__m128i sHi = _mm_unpackhi_epi8(s8, zero); // R2..A3
			__m128i dLo = _mm_unpacklo_epi8(d8, zero);
			__m128i dHi = _mm_unpackhi_epi8(d8, zero);

			if ( pTint )
			{
				__m128i sMulLo = _mm_mullo_epi16(sLo, tintLo);
				__m128i sMulHi = _mm_mullo_epi16(sHi, tintHi);
				sLo = div255_epu16_sse2(sMulLo);
				sHi = div255_epu16_sse2(sMulHi);
			}

			// Build invA vectors (u16 lanes) for 2 pixels per half (SSE2, so do scalar replication)
			// Layout sLo lanes: [R0,G0,B0,A0,R1,G1,B1,A1]
			// Need invA replicated to 4 lanes per pixel.
			ui16 a0 = (ui16)(sRow[xBytes + 3]);
			ui16 a1 = (ui16)(sRow[xBytes + 7]);
			ui16 a2 = (ui16)(sRow[xBytes + 11]);
			ui16 a3 = (ui16)(sRow[xBytes + 15]);

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
		for ( ; xBytes<rowBytes ; xBytes+=4 )
		{
			const byte* sp = sRow + xBytes;
			byte* dp = dRow + xBytes;

			ui32 sr = sp[0], sg = sp[1], sb = sp[2], sa = sp[3];
			ui32 dr = dp[0], dg = dp[1], db = dp[2], da = dp[3];

			if ( pTint )
			{
				sr = div255_u32(sr * tr);
				sg = div255_u32(sg * tg);
				sb = div255_u32(sb * tb);
			}

			ui32 invA = 255u - sa;

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
void Premultiply(const byte* src, byte* dst, int width, int height)
{
	if ( src==nullptr || dst==nullptr || width<=0 || height<=0 )
		return;

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
		premLo = _mm256_or_si256(_mm256_and_si256(alphaMask16, lo), _mm256_andnot_si256(alphaMask16, premLo));
		premHi = _mm256_or_si256(_mm256_and_si256(alphaMask16, hi), _mm256_andnot_si256(alphaMask16, premHi));

		__m256i out8 = _mm256_packus_epi16(premLo, premHi);
		_mm256_storeu_si256((__m256i*)(dst + i), out8);
	}

	// Tail scalar
	for ( ; i<bytes ; i+=4 )
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
	for ( ; i+16<=bytes ; i+=16 ) // 4 pixels
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
	for ( ; i<bytes ; i+=4 )
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
	for ( int i=0 ; i<bytes ; i+=4 )
	{
		uint32_t r = src[i+0], g = src[i+1], b = src[i+2], a = src[i+3];
		dst[i+3] = (byte)a;

		if ( a==255 )
		{
			dst[i+0] = (byte)r; dst[i+1] = (byte)g; dst[i+2] = (byte)b;
		}
		else if ( a==0 )
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

void Unpremultiply(const byte* src, byte* dst, int width, int height)
{
	if ( src==nullptr || dst==nullptr || width<=0 || height<=0 )
		return;

	const int pixels = width * height;
	const int bytes  = pixels * 4;

	// LUT: scale16[a] = round((255<<16)/a). scale16[0]=0.
	// rgb_out = (rgb_premul * scale16[a] + 0x8000) >> 16
	static uint32_t scale16[256];
	static bool inited = false;
	if ( inited==false )
	{
		scale16[0] = 0;
		for ( int a=1 ; a<256 ; ++a )
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
		if ( a[0]==0 && a[4]==0 && a[8]==0 && a[12]==0 && a[16]==0 && a[20]==0 && a[24]==0 && a[28]==0 )
		{
			// Fully transparent => RGB must be 0, alpha remains 0
			// We can just zero 32 bytes.
			std::memset(dst + i, 0, 32);
			continue;
		}

		// Mixed alpha: do 8 scalar pixels (still decent; LUT avoids division)
		for ( int k=0 ; k<32 ; k+=4 )
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
	for ( ; i<bytes ; i+=4 )
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
	for ( ; i+16<=bytes ; i+=16 ) // 4 pixels
	{
		const byte* a = src + i + 3;

		if ( a[0]==255 && a[4]==255 && a[8]==255 && a[12]==255 )
		{
			std::memcpy(dst + i, src + i, 16);
			continue;
		}
		if ( a[0]==0 && a[4]==0 && a[8]==0 && a[12]==0 )
		{
			std::memset(dst + i, 0, 16);
			continue;
		}

		for ( int k=0 ; k<16 ; k+=4 )
		{
			const byte* sp = src + i + k;
			byte* dp = dst + i + k;

			uint32_t r = sp[0], g = sp[1], b = sp[2], A = sp[3];
			dp[3] = (byte)A;

			if ( A==0 )
			{
				dp[0] = dp[1] = dp[2] = 0;
			}
			else if ( A==255 )
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

	for ( ; i<bytes ; i+=4 )
	{
		uint32_t r = src[i+0], g = src[i+1], b = src[i+2], A = src[i+3];
		dst[i+3] = (byte)A;

		if ( A==0 ) { dst[i+0]=dst[i+1]=dst[i+2]=0; }
		else if ( A==255 ) { dst[i+0]=(byte)r; dst[i+1]=(byte)g; dst[i+2]=(byte)b; }
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
	for ( int i=0 ; i<bytes ; i+=4 )
	{
		uint32_t r = src[i+0], g = src[i+1], b = src[i+2], A = src[i+3];
		dst[i+3] = (byte)A;

		if ( A==0 )
		{
			dst[i+0] = dst[i+1] = dst[i+2] = 0;
		}
		else if ( A==255 )
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

// IMPORTANT: width/height are in PIXELS.
// Buffer layout: BGRA, premultiplied, tightly packed => stride = width*4.
void Blur(byte* img, int width, int height, int radius)
{
	if ( img==nullptr || width <= 0 || height <= 0 || radius <= 0 )
		return;
	radius = std::min(radius, 254);

	const int stride = width * 4;
	const int div = radius * 2 + 1;

	// Scratch for horizontal pass (same size)
	if ( g_pBlurScratch==nullptr )
		g_pBlurScratch = new std::vector<byte>();
	g_pBlurScratch->resize((size_t)height * (size_t)stride);
	byte* tmp = g_pBlurScratch->data();

	// Stack entries (B,G,R,A)
	struct P { int b,g,r,a; };
	std::vector<P> stack((size_t)div);

	// Precompute division table for this radius (avoids / in the hot loop)
	const int divSum = ((div + 1) >> 1) * ((div + 1) >> 1);
	if ( g_pBlurDv==nullptr )
		g_pBlurDv = new std::vector<int>();
	if ( divSum!=g_pBlurDv->size() )
	{
		g_pBlurDv->resize((size_t)256 * (size_t)divSum);
		for (int i = 0; i < 256 * divSum; ++i)
			(*g_pBlurDv)[i] = i / divSum;
	}

	// -------------------------
	// Horizontal pass: img -> tmp
	// -------------------------
	for (int y = 0; y < height; ++y)
	{
		const byte* srcRow = img + y * stride;
		byte* dstRow = tmp + y * stride;

		int sumB=0,sumG=0,sumR=0,sumA=0;
		int inB=0,inG=0,inR=0,inA=0;
		int outB=0,outG=0,outR=0,outA=0;

		// init stack with left edge clamped
		for (int i = -radius; i <= radius; ++i)
		{
			const int x = cpu::Clamp(i, 0, width - 1);
			const byte* p = srcRow + x * 4;

			P& s = stack[(size_t)(i + radius)];
			s.b = p[0]; s.g = p[1]; s.r = p[2]; s.a = p[3];

			const int wgt = radius + 1 - std::abs(i);
			sumB += s.b * wgt; sumG += s.g * wgt; sumR += s.r * wgt; sumA += s.a * wgt;

			if (i > 0) { inB += s.b; inG += s.g; inR += s.r; inA += s.a; }
			else       { outB+= s.b; outG+= s.g; outR+= s.r; outA+= s.a; }
		}

		int stackPtr = radius;

		for (int x = 0; x < width; ++x)
		{
			byte* d = dstRow + x * 4;
			d[0] = (byte)(*g_pBlurDv)[sumB];
			d[1] = (byte)(*g_pBlurDv)[sumG];
			d[2] = (byte)(*g_pBlurDv)[sumR];
			d[3] = (byte)(*g_pBlurDv)[sumA];

			// remove outgoing
			sumB -= outB; sumG -= outG; sumR -= outR; sumA -= outA;

			int stackStart = stackPtr - radius;
			if (stackStart < 0) stackStart += div;
			P& out = stack[(size_t)stackStart];

			outB -= out.b; outG -= out.g; outR -= out.r; outA -= out.a;

			// add incoming (write into outgoing slot)
			const int xIn = cpu::Clamp(x + radius + 1, 0, width - 1);
			const byte* pIn = srcRow + xIn * 4;

			out.b = pIn[0]; out.g = pIn[1]; out.r = pIn[2]; out.a = pIn[3];

			inB += out.b; inG += out.g; inR += out.r; inA += out.a;
			sumB += inB;  sumG += inG;  sumR += inR;  sumA += inA;

			// advance
			stackPtr = (stackPtr + 1) % div;
			P& inPix = stack[(size_t)stackPtr];

			outB += inPix.b; outG += inPix.g; outR += inPix.r; outA += inPix.a;
			inB  -= inPix.b; inG  -= inPix.g; inR  -= inPix.r; inA  -= inPix.a;
		}
	}

	// -------------------------
	// Vertical pass: tmp -> img
	// -------------------------
	for (int x = 0; x < width; ++x)
	{
		int sumB=0,sumG=0,sumR=0,sumA=0;
		int inB=0,inG=0,inR=0,inA=0;
		int outB=0,outG=0,outR=0,outA=0;

		// init stack with top edge clamped
		for (int i = -radius; i <= radius; ++i)
		{
			const int y = cpu::Clamp(i, 0, height - 1);
			const byte* p = tmp + y * stride + x * 4;

			P& s = stack[(size_t)(i + radius)];
			s.b = p[0]; s.g = p[1]; s.r = p[2]; s.a = p[3];

			const int wgt = radius + 1 - std::abs(i);
			sumB += s.b * wgt; sumG += s.g * wgt; sumR += s.r * wgt; sumA += s.a * wgt;

			if (i > 0) { inB += s.b; inG += s.g; inR += s.r; inA += s.a; }
			else       { outB+= s.b; outG+= s.g; outR+= s.r; outA+= s.a; }
		}

		int stackPtr = radius;

		for (int y = 0; y < height; ++y)
		{
			byte* d = img + y * stride + x * 4;
			d[0] = (byte)(*g_pBlurDv)[sumB];
			d[1] = (byte)(*g_pBlurDv)[sumG];
			d[2] = (byte)(*g_pBlurDv)[sumR];
			d[3] = (byte)(*g_pBlurDv)[sumA];

			sumB -= outB; sumG -= outG; sumR -= outR; sumA -= outA;

			int stackStart = stackPtr - radius;
			if (stackStart < 0) stackStart += div;
			P& out = stack[(size_t)stackStart];

			outB -= out.b; outG -= out.g; outR -= out.r; outA -= out.a;

			const int yIn = cpu::Clamp(y + radius + 1, 0, height - 1);
			const byte* pIn = tmp + yIn * stride + x * 4;

			out.b = pIn[0]; out.g = pIn[1]; out.r = pIn[2]; out.a = pIn[3];

			inB += out.b; inG += out.g; inR += out.r; inA += out.a;
			sumB += inB;  sumG += inG;  sumR += inR;  sumA += inA;

			stackPtr = (stackPtr + 1) % div;
			P& inPix = stack[(size_t)stackPtr];

			outB += inPix.b; outG += inPix.g; outR += inPix.r; outA += inPix.a;
			inB  -= inPix.b; inG  -= inPix.g; inR  -= inPix.r; inA  -= inPix.a;
		}
	}
}

void ToAmigaPalette(byte* buffer, int width, int height)
{
	if (!buffer || width <= 0 || height <= 0) return;

	// Dithers préfabriqués pour 4 pixels (16 bytes BGRA BGRA BGRA BGRA)
	// dd appliqué à B,G,R ; A reste 0.
	alignas(16) static const unsigned char D_EVENY_XEVEN[16] = {
		0,0,0,0,   8,8,8,0,   0,0,0,0,   8,8,8,0
	};
	alignas(16) static const unsigned char D_EVENY_XODD[16] = {
		8,8,8,0,   0,0,0,0,   8,8,8,0,   0,0,0,0
	};
	alignas(16) static const unsigned char D_ODDY_XEVEN[16] = {
		12,12,12,0,  4,4,4,0,  12,12,12,0,  4,4,4,0
	};
	alignas(16) static const unsigned char D_ODDY_XODD[16] = {
		4,4,4,0,   12,12,12,0,  4,4,4,0,   12,12,12,0
	};

	const __m128i mask_alpha = _mm_set1_epi32(0xFF000000);
	const __m128i mask_rgb   = _mm_set1_epi32(0x00FFFFFF);
	const __m128i zero       = _mm_setzero_si128();

	const __m128i d_even_xeven = _mm_load_si128(reinterpret_cast<const __m128i*>(D_EVENY_XEVEN));
	const __m128i d_even_xodd  = _mm_load_si128(reinterpret_cast<const __m128i*>(D_EVENY_XODD));
	const __m128i d_odd_xeven  = _mm_load_si128(reinterpret_cast<const __m128i*>(D_ODDY_XEVEN));
	const __m128i d_odd_xodd   = _mm_load_si128(reinterpret_cast<const __m128i*>(D_ODDY_XODD));

	const int strideBytes = width * 4;

	for (int y = 0; y < height; ++y)
	{
		unsigned char* row = buffer + y * strideBytes;

		// Choix des deux motifs selon la parité de y
		const bool yOdd = (y & 1) != 0;
		const __m128i d_xeven = yOdd ? d_odd_xeven : d_even_xeven;
		const __m128i d_xodd  = yOdd ? d_odd_xodd  : d_even_xodd;

		int x = 0;

		// SIMD par blocs de 4 pixels
		const int simdWidth = width & ~3; // multiple de 4
		for (; x < simdWidth; x += 4)
		{
			unsigned char* p = row + x * 4;

			__m128i px = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));

			// Extraire alpha
			__m128i alpha = _mm_and_si128(px, mask_alpha);

			// Dither correct selon parité de x (début de bloc)
			const __m128i d = (x & 1) ? d_xodd : d_xeven;

			// Ajouter dithering uniquement sur RGB
			__m128i rgb = _mm_and_si128(px, mask_rgb);
			rgb = _mm_adds_epu8(rgb, d);

			// Quantification 8->4->8 SANS mélange de canaux :
			// unpack bytes->u16, shift, replicate, pack
			__m128i lo = _mm_unpacklo_epi8(rgb, zero);
			__m128i hi = _mm_unpackhi_epi8(rgb, zero);

			lo = _mm_srli_epi16(lo, 4);
			hi = _mm_srli_epi16(hi, 4);

			lo = _mm_or_si128(_mm_slli_epi16(lo, 4), lo);
			hi = _mm_or_si128(_mm_slli_epi16(hi, 4), hi);

			__m128i rgb8 = _mm_packus_epi16(lo, hi);

			// Recombiner avec alpha original
			__m128i out = _mm_or_si128(_mm_and_si128(rgb8, mask_rgb), alpha);

			_mm_storeu_si128(reinterpret_cast<__m128i*>(p), out);
		}

		// Reste scalaire (0..3 pixels) en fin de ligne
		for (; x < width; ++x)
		{
			unsigned char* q = row + x * 4;

			unsigned char b = q[0];
			unsigned char g = q[1];
			unsigned char r = q[2];

			// vrai 2x2 : index = (x&1) + 2*(y&1)
			static const unsigned char d2x2[4] = { 0, 8, 12, 4 };
			unsigned char dd = d2x2[(x & 1) | ((y & 1) << 1)];

			// (optionnel) saturer à 255 avant >>4 pour coller à _mm_adds_epu8
			int bi = b + dd; if (bi > 255) bi = 255;
			int gi = g + dd; if (gi > 255) gi = 255;
			int ri = r + dd; if (ri > 255) ri = 255;

			unsigned char b4 = (unsigned char)(bi >> 4);
			unsigned char g4 = (unsigned char)(gi >> 4);
			unsigned char r4 = (unsigned char)(ri >> 4);

			q[0] = (b4 << 4) | b4;
			q[1] = (g4 << 4) | g4;
			q[2] = (r4 << 4) | r4;
			// same alpha
		}
	}
}

}
