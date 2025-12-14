#include "stdafx.h"

FONT::FONT()
{
	first = 0;
	last  = 0;
	count = 0;
	cellW = 0;
	cellH = 0;
	advance = 0;
	width = 0;
	height = 0;
}

bool FONT::Create(int fontPx, XMFLOAT3 color, const char* fontName, int cellWidth, int cellHeight, int firstChar, int lastChar)
{
	if ( cellHeight==-1 )
		cellHeight = fontPx;
	if ( cellWidth==-1 )
		cellWidth = fontPx;
	if ( fontName==nullptr || fontPx<=0 || cellWidth<=0 || cellHeight<=0 )
		return false;
	if ( firstChar<0 )
		firstChar = 0;
	if ( lastChar>255 )
		lastChar = 255;
	if ( firstChar>lastChar )
		return false;

	*this = {};
	first = firstChar;
	last  = lastChar;
	count = (lastChar - firstChar + 1);
	cellW = cellWidth;
	cellH = cellHeight;
	advance = cellW;
	width  = count * cellW;
	height = cellH;

	// DIB 32-bit top-down
	BITMAPINFO bmi{};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	void* bits = nullptr;
	HBITMAP hbmp = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
	if ( hbmp==nullptr || bits==nullptr )
		return false;

	HDC hdc = CreateCompatibleDC(nullptr);
	if ( hdc==nullptr )
	{
		DeleteObject(hbmp);
		return false;
	}

	HGDIOBJ oldBmp = SelectObject(hdc, hbmp);
	std::memset(bits, 0, width*height*4);

	HFONT hFont = CreateFont(-fontPx, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontName);
	if ( hFont==nullptr )
	{
		SelectObject(hdc, oldBmp);
		DeleteDC(hdc);
		DeleteObject(hbmp);
		return false;
	}

	HGDIOBJ oldFont = SelectObject(hdc, hFont);
	SetTextColor(hdc, ToRGB(color));
	SetBkMode(hdc, TRANSPARENT);
	for ( int c=firstChar ; c<=lastChar ; ++c )
	{
		const int i = c - firstChar;
		const int cellX = i * cellW;
		wchar_t wch = (wchar_t)(unsigned char)c;
		RECT rcCell = { cellX, 0, cellX + cellW, cellH };
		SIZE sz = {};
		GetTextExtentPoint32W(hdc, &wch, 1, &sz);
		int x = cellX + (cellW - (int)sz.cx) / 2;
		int y = (cellH - (int)sz.cy) / 2;
		TextOutW(hdc, x, y, &wch, 1);
		glyph[c].x = cellX;
		glyph[c].y = 0;
		glyph[c].w = cellW;
		glyph[c].h = cellH;
		glyph[c].valid = true;
		advance = (int)sz.cx;
	}

	rgba.resize(width*height*4);
	const int n = width * height;
	byte* bgra = (byte*)bits;
	for ( int i=0 ; i<n ; ++i )
	{
		const byte b = bgra[i*4+0];
		const byte g = bgra[i*4+1];
		const byte r = bgra[i*4+2];
		const byte a = std::max({ r, g, b }); // conserve l'antialias (gris -> alpha partiel)
		rgba[i*4+0] = r;
		rgba[i*4+1] = g;
		rgba[i*4+2] = b;
		rgba[i*4+3] = a;
	}

	SelectObject(hdc, oldFont);
	DeleteObject(hFont);
	SelectObject(hdc, oldBmp);
	DeleteDC(hdc);
	DeleteObject(hbmp);
	return true;
}
