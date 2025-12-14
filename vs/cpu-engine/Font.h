#pragma once

struct GLYPH
{
	int x = 0, y = 0, w = 0, h = 0;
	int advance = 0;
	bool valid = false;
};

struct FONT
{
	int first;
	int last;
	int count;

	int cellW;
	int cellH;

	int width;
	int height;

	std::vector<byte> rgba;

	GLYPH glyph[256];

	FONT();
	bool Create(int fontPx = 24, XMFLOAT3 color = {1.0f,1.0f,1.0f}, const char* fontName = "Consolas", int cellW = -1, int cellH = -1, int firstChar = 32, int lastChar = 255);
};
