#include "stdafx.h"

cpu_texture::cpu_texture()
{
	bgra = nullptr;
	Close();
}

cpu_texture::~cpu_texture()
{
	Close();
}

bool cpu_texture::Load(const char* path)
{
	Close();

	int filesize;
	byte* data = cpu::LoadFile(path, filesize);
	if ( data==nullptr )
		return false;

	int w, h;
	byte* buf = cpu_png32::parse_png_rgba(data, filesize, &w, &h);
	delete [] data;
	if ( buf==nullptr )
		return false;

	bgra = buf;
	width = w;
	height = h;
	count = width * height;
	size = count * 4;

	for ( int i=0 ; i<size ; i+=4 )
	{
		byte r = bgra[i];
		bgra[i] = bgra[i+2];
		bgra[i+2] = r;
	}
	cpu_img32::Premultiply(bgra, bgra, width, height);
	return true;
}

void cpu_texture::Close()
{
	CPU_DELPTRS(bgra);
	width = 0;
	height = 0;
	count = 0;
	size = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_sprite::cpu_sprite()
{
	pTexture = nullptr;
	x = 0;
	y = 0;
	z = 0;
	anchorX = 0;
	anchorY = 0;
	index = -1;
	sortedIndex = -1;
	dead = false;
	visible = true;
}

void cpu_sprite::CenterAnchor()
{
	if ( pTexture )
	{
		anchorX = pTexture->width/2;
		anchorY = pTexture->height/2;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

bool cpu_font::Create(float size, XMFLOAT3 color, const char* fontName, int cellWidth, int cellHeight, int firstChar, int lastChar)
{
	int fontPx = cpu::CeilToInt(size*CPU.GetMainRT()->height);
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

	HFONT hFont = CreateFontA(-fontPx, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontName);
	if ( hFont==nullptr )
	{
		SelectObject(hdc, oldBmp);
		DeleteDC(hdc);
		DeleteObject(hbmp);
		return false;
	}

	HGDIOBJ oldFont = SelectObject(hdc, hFont);
	SetTextColor(hdc, cpu::ToRGB(color)&0xFFFFFF);
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
		// conserve l'antialias (gris -> alpha partiel);
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
