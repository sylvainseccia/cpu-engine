#include "pch.h"

cpu_light::cpu_light()
{
	dir = { 0.5f, -1.0f, 0.5f };
	XMStoreFloat3(&dir, XMVector3Normalize(XMLoadFloat3(&dir)));
	ambient = 0.2f;
}
