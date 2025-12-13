#pragma once

struct GLYPH
{
	int x = 0, y = 0, w = 0, h = 0;
	int advance = 0;
	int bearingX = 0;
	int bearingY = 0;
	bool valid = false;
};

struct FONT
{
	int first = 32;
	int last  = 255;
	int count = 0;

	int cellW = 0;
	int cellH = 0;

	int width = 0;
	int height = 0;

	std::vector<byte> rgba;

	GLYPH glyph[256] = {};

	bool Create(const char* fontName, int fontPx, int cellW = -1, int cellH = -1, int firstChar = 32, int lastChar = 255);
};
