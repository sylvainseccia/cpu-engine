#include "stdafx.h"

ui32 ToRGB(XMFLOAT3& color)
{
	int r = Clamp((int)(color.x*255.0f), 0, 255);
	int g = Clamp((int)(color.y*255.0f), 0, 255);
	int b = Clamp((int)(color.z*255.0f), 0, 255);
	return RGB(r, g, b);
}

ui32 ToBGR(XMFLOAT3& color)
{
	int r = Clamp((int)(color.z*255.0f), 0, 255);
	int g = Clamp((int)(color.y*255.0f), 0, 255);
	int b = Clamp((int)(color.x*255.0f), 0, 255);
	return RGB(r, g, b);
}

XMFLOAT3 ToColor(int r, int g, int b)
{
	XMFLOAT3 color;
	color.x = Clamp(r, 0, 255)/255.0f;
	color.y = Clamp(g, 0, 255)/255.0f;
	color.z = Clamp(b, 0, 255)/255.0f;
	return color;
}

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
