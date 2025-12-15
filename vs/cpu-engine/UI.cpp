#include "stdafx.h"

cpu_texture::cpu_texture()
{
	rgba = nullptr;
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
	byte* data = LoadFile(path, filesize);
	if ( data==nullptr )
		return false;

	lodepng::State state;
	ui32 pngWidth, pngHeight;
	if ( lodepng_inspect(&pngWidth, &pngHeight, &state, data, filesize) )
	{
		delete [] data;
		return false;
	}

	bool alpha = false;
	if ( state.info_png.color.colortype==LCT_RGBA )
		alpha = true;
	else if ( state.info_png.color.colortype!=LCT_RGB )
	{
		delete [] data;
		return false;
	}

	ui32 error = lodepng_decode32(&rgba, (ui32*)&width, (ui32*)&height, data, filesize);
	delete [] data;
	if ( error || width==0 || height==0 )
	{
		Close();
		return false;
	}
	count = width * height;
	size = count * 4;
	return true;
}

void cpu_texture::Close()
{
	delete [] rgba;
	rgba = nullptr;
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
