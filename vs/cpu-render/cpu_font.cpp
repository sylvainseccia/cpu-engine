#include "pch.h"

cpu_font::cpu_font()
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool cpu_font::Create(int fontPx, XMFLOAT3 color, const char* fontName, int cellWidth, int cellHeight, int firstChar, int lastChar)
{
	if ( cellHeight==-1 )
		cellHeight = fontPx + 2;
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

	HFONT hFont = CreateFontA(-fontPx, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontName);
	if ( hFont==nullptr )
	{
		SelectObject(hdc, oldBmp);
		DeleteDC(hdc);
		DeleteObject(hbmp);
		return false;
	}

	HGDIOBJ oldFont = SelectObject(hdc, hFont);
	SetTextColor(hdc, cpu::ToRGB(color) & 0xFFFFFF);
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

	bgra.resize(width*height*4);
	const int n = width * height;
	byte* src = (byte*)bits;
	for ( int i=0 ; i<n ; ++i )
	{
		int offset = i*4;
		bgra[offset+0] = src[offset+0];
		bgra[offset+1] = src[offset+1];
		bgra[offset+2] = src[offset+2];
		// antialiasing (gris -> alpha partiel)
		bgra[offset+3] = std::max({ bgra[offset+0], bgra[offset+1], bgra[offset+2] });
	}

	cpu_img32::Premultiply(bgra.data(), bgra.data(), width, height);

	SelectObject(hdc, oldFont);
	DeleteObject(hFont);
	SelectObject(hdc, oldBmp);
	DeleteDC(hdc);
	DeleteObject(hbmp);
	return true;
}
