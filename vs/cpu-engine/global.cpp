#include "stdafx.h"

#ifdef _DEBUG
	_CrtMemState g_memState;
#endif

bool InitializeEngine()
{
#ifdef _DEBUG
	_CrtMemCheckpoint(&g_memState);
#endif

	return true;
}

void UninitializeEngine()
{
#ifdef _DEBUG
	_CrtMemState memState, memStateDiff;
	_CrtMemCheckpoint(&memState);
	if ( _CrtMemDifference(&memStateDiff, &g_memState, &memState) )
		MessageBox(nullptr, "Memory leaks", "ALERT", 0);
	_CrtDumpMemoryLeaks();
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ui32 SwapRB(ui32 color)
{
	return (color & 0xFF00FF00u) | ((color & 0x000000FFu) << 16) | ((color & 0x00FF0000u) >> 16);
}

ui32 ToRGB(XMFLOAT3& color)
{
	int red = Clamp((int)(color.x*255.0f), 0, 255);
	int green = Clamp((int)(color.y*255.0f), 0, 255);
	int blue = Clamp((int)(color.z*255.0f), 0, 255);
	return RGB(red, green, blue);
}

ui32 ToRGB(float r, float g, float b)
{
	int red = Clamp((int)(r*255.0f), 0, 255);
	int green = Clamp((int)(g*255.0f), 0, 255);
	int blue = Clamp((int)(b*255.0f), 0, 255);
	return RGB(red, green, blue);
}

ui32 ToBGR(XMFLOAT3& color)
{
	int red = Clamp((int)(color.z*255.0f), 0, 255);
	int green = Clamp((int)(color.y*255.0f), 0, 255);
	int blue = Clamp((int)(color.x*255.0f), 0, 255);
	return RGB(red, green, blue);
}

ui32 ToBGR(float r, float g, float b)
{
	int red = Clamp((int)(b*255.0f), 0, 255);
	int green = Clamp((int)(g*255.0f), 0, 255);
	int blue = Clamp((int)(r*255.0f), 0, 255);
	return RGB(red, green, blue);
}

XMFLOAT3 ToColor(int r, int g, int b)
{
	XMFLOAT3 color;
	color.x = Clamp(r, 0, 255)/255.0f;
	color.y = Clamp(g, 0, 255)/255.0f;
	color.z = Clamp(b, 0, 255)/255.0f;
	return color;
}

XMFLOAT3 ToColorFromRGB(ui32 rgb)
{
	XMFLOAT3 color;
	color.x = (rgb & 0xFF)/255.0f;
	color.y = ((rgb>>16) & 0xFF)/255.0f;
	color.z = ((rgb>>24) & 0xFF)/255.0f;
	return color;
}

XMFLOAT3 ToColorFromBGR(ui32 bgr)
{
	XMFLOAT3 color;
	color.x = ((bgr>>24) & 0xFF)/255.0f;
	color.y = ((bgr>>16) & 0xFF)/255.0f;
	color.z = (bgr & 0xFF)/255.0f;
	return color;
}

XMFLOAT4 ToColor(int r, int g, int b, int a)
{
	XMFLOAT4 color;
	color.x = Clamp(r, 0, 255)/255.0f;
	color.y = Clamp(g, 0, 255)/255.0f;
	color.z = Clamp(b, 0, 255)/255.0f;
	color.w = Clamp(a, 0, 255)/255.0f;
	return color;
}

ui32 LerpBGR(ui32 c0, ui32 c1, float t)
{
	int b0 = (int)( c0        & 255);
	int g0 = (int)((c0 >>  8) & 255);
	int r0 = (int)((c0 >> 16) & 255);
	int b1 = (int)( c1        & 255);
	int g1 = (int)((c1 >>  8) & 255);
	int r1 = (int)((c1 >> 16) & 255);
	float it = 1.0f - t;
	int b = (int)(b0 * it + b1 * t);
	int g = (int)(g0 * it + g1 * t);
	int r = (int)(r0 * it + r1 * t);
	return (ui32)b | ((ui32)g << 8) | ((ui32)r << 16);
}

ui32 AddSaturateRGBA(ui32 dst, ui32 src)
{
	// Format: R(31..24) G(23..16) B(15..8) A(7..0)
	const ui32 dr = (dst >> 24) & 255;
	const ui32 dg = (dst >> 16) & 255;
	const ui32 db = (dst >>  8) & 255;
	const ui32 da = (dst      ) & 255;

	const ui32 sr = (src >> 24) & 255;
	const ui32 sg = (src >> 16) & 255;
	const ui32 sb = (src >>  8) & 255;
	const ui32 sa = (src      ) & 255;

	const ui32 r = (dr + sr > 255) ? 255 : (dr + sr);
	const ui32 g = (dg + sg > 255) ? 255 : (dg + sg);
	const ui32 b = (db + sb > 255) ? 255 : (db + sb);
	const ui32 a = (da + sa > 255) ? 255 : (da + sa);

	return (r << 24) | (g << 16) | (b << 8) | a;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float Clamp(float v)
{
	if ( v<0.0f )
		return 0.0f;
	if ( v>1.0f )
		return 1.0f;
	return v;
}

float Clamp(float v, float min, float max)
{
	if ( v<min )
		return min;
	if ( v>max )
		return max;
	return v;
}

int Clamp(int v, int min, int max)
{
	if ( v<min )
		return min;
	if ( v>max )
		return max;
	return v;
}

int FloorToInt(float v)
{
	int i = (int)v;
	return (float)i>v ? i-1 : i;
}

int CeilToInt(float v)
{
	int i = (int)v;
	return (float)i<v ? i+1 : i;
}

int RoundToInt(float v)
{
	int i = (int)v;
	return v>=0.0f ? ((float)i+0.5f<=v ? i+1 : i) : ((float)i-0.5f>=v ? i-1 : i);
}

ui32 WangHash(ui32 x)
{
	x = (x ^ 61u) ^ (x >> 16);
	x *= 9u;
	x = x ^ (x >> 4);
	x *= 0x27d4eb2du;
	x = x ^ (x >> 15);
	return x;
}

float Rand01(ui32& seed)
{
	seed = WangHash(seed);
	return (seed & 0x00FFFFFF) / 16777216.0f;
}

float RandSigned(ui32& seed)
{
	return Rand01(seed)*2.0f - 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

XMFLOAT3 SphericalPoint(float r, float theta, float phi)
{
	float st = sinf(theta);
	float ct = cosf(theta);
	float sp = sinf(phi);
	float cp = cosf(phi);
	return XMFLOAT3(r * st * cp, r * ct, r * st * sp);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

byte* LoadFile(const char* path, int& size)
{
	byte* data = nullptr;
	FILE* file;
	if ( fopen_s(&file, path, "rb") )
		return data;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if ( size>0 )
	{
		data = new byte[size];
		fread(data, 1, size, file);
	}
	fclose(file);
	return data;
}
