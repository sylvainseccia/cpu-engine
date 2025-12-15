#pragma once

namespace png_lib
{
	uint8_t* parse_png_rgba(const uint8_t* data, size_t len, int* w, int* h);
}
