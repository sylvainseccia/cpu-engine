#pragma once

ui32 ToRGB(XMFLOAT3& color);
ui32 ToBGR(XMFLOAT3& color);
XMFLOAT3 ToColor(int r, int g, int b);

float Clamp(float v);
float Clamp(float v, float min, float max);
int Clamp(int v, int min, int max);
