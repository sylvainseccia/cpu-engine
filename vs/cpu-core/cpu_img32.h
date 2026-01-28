#pragma once

namespace cpu_img32
{

void Free();

// Premultiplied alpha SRC-over DST blit with clipping.
// Format: RGBA8 premultiplied (R,G,B already multiplied by A/255), tightly packed (pitch = W*4).
// Blends a rectangle from src at (srcX,srcY) onto dst at (dstX,dstY), size (w,h).
// Clips automatically to both surfaces. In-place on dst.

static inline void clip_blit_rect(int srcW, int srcH, int dstW, int dstH, int& srcX, int& srcY, int& dstX, int& dstY, int& w, int& h)
{
	// clip left/top by shifting both src and dst
	if (srcX < 0) { int cut = -srcX; srcX += cut; dstX += cut; w -= cut; }
	if (srcY < 0) { int cut = -srcY; srcY += cut; dstY += cut; h -= cut; }
	if (dstX < 0) { int cut = -dstX; dstX += cut; srcX += cut; w -= cut; }
	if (dstY < 0) { int cut = -dstY; dstY += cut; srcY += cut; h -= cut; }

	if (w <= 0 || h <= 0) { w = h = 0; return; }

	// clip right/bottom against both
	w = std::min(w, srcW - srcX);
	h = std::min(h, srcH - srcY);
	w = std::min(w, dstW - dstX);
	h = std::min(h, dstH - dstY);

	if (w <= 0 || h <= 0) { w = h = 0; }
}

static inline uint32_t div255_u32(uint32_t x)
{
	// exact for 0..65025
	x += 128u;
	return (x + (x >> 8)) >> 8;
}

static inline __m128i div255_epu16_sse2(__m128i x)
{
	const __m128i bias = _mm_set1_epi16(128);
	__m128i t = _mm_add_epi16(x, bias);
	__m128i u = _mm_add_epi16(t, _mm_srli_epi16(t, 8));
	return _mm_srli_epi16(u, 8);
}

void AlphaBlend(const byte* src, int srcW, int srcH, byte* dst, int dstW, int dstH, int srcX, int srcY, int dstX, int dstY, int blitW, int blitH, XMFLOAT3* pTint = nullptr);

void Premultiply(const byte* src, byte* dst, int width, int height);

void Unpremultiply(const byte* src, byte* dst, int width, int height);

bool AlphaBlendStraightOverOpaque(const byte* src, int srcW, int srcH, byte* dst, int dstW, int dstH, int srcX, int srcY, int dstX, int dstY, int blitW, int blitH);

void Blur(byte* img, int width, int height, int radius);

void ToAmigaPalette(byte* buffer, int width, int height);

}
