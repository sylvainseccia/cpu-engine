#include "pch.h"

#include <cstdlib>
#include <cstring>
#include <climits>

namespace cpu_png32
{

static ui32 Read32(const byte* p)
{
	return (ui32(p[0]) << 24) | (ui32(p[1]) << 16) | (ui32(p[2]) << 8) | ui32(p[3]);
}

struct BitStream
{
	const byte* p;
	const byte* end;
	ui32 bits;
	unsigned count;
};

static bool ReadBits(BitStream& s, unsigned n, ui32& out)
{
	if ( n>16 )
		return false;
	while ( s.count<n )
	{
		if ( s.p==s.end )
			return false;
		s.bits |= ui32(*s.p++) << s.count;
		s.count += 8;
	}
	out = s.bits & ((1u << n) - 1u);
	s.bits >>= n;
	s.count -= n;
	return true;
}

struct Huffman
{
	ui16 count[16];
	ui16 symbol[288];
};

static bool BuildTree(Huffman& t, const byte* lengths, int n)
{
	if ( n<1 || n>288 )
		return false;
	memset(t.count, 0, sizeof(t.count));
	for ( int i=0 ; i<n ; i++ )
	{
		if ( lengths[i]>15 )
			return false;
		++t.count[lengths[i]];
	}
	t.count[0] = 0;

	int left = 1;
	for ( int i=1 ; i<=15 ; ++i )
	{
		left = (left << 1) - t.count[i];
		if ( left<0 )
			return false;
	}

	int offsets[16];
	int sum = 0;
	for ( int i=0 ; i<16 ; ++i )
	{
		offsets[i] = sum;
		sum += t.count[i];
	}
	for ( int i=0 ; i<n ; ++i )
	{
		if ( lengths[i] )
			t.symbol[offsets[lengths[i]]++] = ui16(i);
	}
	return true;
}

static int Decode(BitStream& s, const Huffman& t)
{
	int code = 0, first = 0, index = 0;
	for ( int length=1 ; length<=15 ; ++length )
	{
		ui32 bit;
		if ( ReadBits(s, 1, bit)==false )
			return -1;
		code = (code << 1) | int(bit);
		int count = t.count[length];
		if ( code>=first && code-first<count )
			return t.symbol[index+code-first];
		index += count;
		first = (first + count) << 1;
	}
	return -1;
}

static bool Inflate(const byte* in, size_t inSize, byte* out, size_t outSize)
{
	static const ui16 lenBase[29] = {
		3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,
		35,43,51,59,67,83,99,115,131,163,195,227,258
	};
	static const byte lenExtra[29] = {
		0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,
		3,3,3,3,4,4,4,4,5,5,5,5,0
	};
	static const ui16 distBase[30] = {
		1,2,3,4,5,7,9,13,17,25,33,49,65,97,
		129,193,257,385,513,769,1025,1537,2049,
		3073,4097,6145,8193,12289,16385,24577
	};
	static const byte distExtra[30] = {
		0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,
		7,7,8,8,9,9,10,10,11,11,12,12,13,13
	};
	static const byte order[19] = {
		16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15
	};

	BitStream bs = {in, in + inSize, 0, 0};
	size_t op = 0;
	bool final = false;

	do
	{
		ui32 last, type;
		if ( ReadBits(bs, 1, last)==false || ReadBits(bs, 2, type)==false )
			return false;
		final = last!=0;

		if ( type==0 )
		{
			// ReadBits ne precharge jamais plus que necessaire :
			// les bits restants sont uniquement ceux de l'octet courant.
			bs.bits = 0;
			bs.count = 0;
			if ( size_t(bs.end - bs.p)<4 )
				return false;
			unsigned n = unsigned(bs.p[0]) | (unsigned(bs.p[1]) << 8);
			unsigned complement = unsigned(bs.p[2]) | (unsigned(bs.p[3]) << 8);
			bs.p += 4;
			if ( (n ^ complement)!=0xFFFFu || n>outSize-op || n>size_t(bs.end-bs.p) )
				return false;
			memcpy(out+op, bs.p, n);
			bs.p += n;
			op += n;
			continue;
		}

		if ( type!=1 && type!=2 )
			return false;

		Huffman litTree, distTree;
		if ( type==1 )
		{
			byte lit[288], dist[32];
			for ( int i=0 ; i<=143 ; ++i ) lit[i] = 8;
			for ( int i=144 ; i<=255 ; ++i ) lit[i] = 9;
			for ( int i=256 ; i<=279 ; ++i ) lit[i] = 7;
			for ( int i=280 ; i<=287 ; ++i ) lit[i] = 8;
			memset(dist, 5, sizeof(dist));
			if ( BuildTree(litTree, lit, 288)==false || BuildTree(distTree, dist, 32)==false )
				return false;
		}
		else
		{
			ui32 hlit, hdist, hclen;
			if ( ReadBits(bs, 5, hlit)==false || ReadBits(bs, 5, hdist)==false || ReadBits(bs, 4, hclen)==false )
				return false;
			hlit += 257;
			hdist += 1;
			hclen += 4;
			if ( hlit>286 || hdist>32 )
				return false;

			byte codeLengths[19] = {};
			for ( unsigned i=0 ; i<hclen ; ++i )
			{
				ui32 v;
				if ( ReadBits(bs, 3, v)==false )
					return false;
				codeLengths[order[i]] = byte(v);
			}
			Huffman codeTree;
			if ( BuildTree(codeTree, codeLengths, 19)==false )
				return false;

			byte lengths[288 + 32] = {};
			unsigned n = 0, total = hlit + hdist;
			while ( n<total )
			{
				int symbol = Decode(bs, codeTree);
				if ( symbol<0 )
					return false;
				if ( symbol<=15 )
					lengths[n++] = byte(symbol);
				else
				{
					unsigned count = 0;
					byte value = 0;
					ui32 extra;
					if ( symbol==16 )
					{
						if ( !n || ReadBits(bs, 2, extra)==false)
							return false;
						count = unsigned(extra) + 3;
						value = lengths[n - 1];
					}
					else if ( symbol==17 )
					{
						if ( ReadBits(bs, 3, extra)==false )
							return false;
						count = unsigned(extra) + 3;
					}
					else if ( symbol==18 )
					{
						if ( ReadBits(bs, 7, extra)==false )
							return false;
						count = unsigned(extra) + 11;
					}
					else
						return false;
					if ( count>total-n )
						return false;
					memset(lengths + n, value, count);
					n += count;
				}
			}
			if ( lengths[256]==0 || BuildTree(litTree, lengths, int(hlit))==false || BuildTree(distTree, lengths + hlit, int(hdist))==false )
				return false;
		}

		for (;;)
		{
			int symbol = Decode(bs, litTree);
			if ( symbol<0 )
				return false;
			if ( symbol<256 )
			{
				if ( op==outSize )
					return false;
				out[op++] = byte(symbol);
			}
			else if ( symbol==256 )
			{
				break;
			}
			else
			{
				if ( symbol>285 )
					return false;
				unsigned l = unsigned(symbol - 257);
				ui32 extra;
				if ( ReadBits(bs, lenExtra[l], extra)==false )
					return false;
				size_t copyLength = size_t(lenBase[l]) + extra;
				int distanceCode = Decode(bs, distTree);
				if ( distanceCode<0 || distanceCode>=30 )
					return false;
				if ( ReadBits(bs, distExtra[distanceCode], extra)==false )
					return false;
				size_t distance = size_t(distBase[distanceCode]) + extra;
				if ( distance>op || copyLength>outSize-op )
					return false;
				for ( size_t j=0 ; j<copyLength ; ++j )
				{
					out[op] = out[op - distance];
					++op;
				}
			}
		}
	} while ( final==false );
	return op==outSize && bs.p==bs.end;
}

static byte Paeth(byte a, byte b, byte c)
{
	int p = int(a) + int(b) - int(c);
	int da = abs(p - int(a));
	int db = abs(p - int(b));
	int dc = abs(p - int(c));
	return da <= db && da <= dc ? a : db <= dc ? b : c;
}

static ui32 Adler32(const byte* data, size_t size)
{
	ui32 a = 1, b = 0;
	for ( size_t i=0 ; i<size ; ++i )
	{
		a = (a + data[i]) % 65521u;
		b = (b + a) % 65521u;
	}
	return (b << 16) | a;
}

static ui32 CRC32(const byte* data, size_t size, const ui32 table[256])
{
	ui32 crc = 0xFFFFFFFFu;
	for ( size_t i=0 ; i<size ; ++i )
		crc = table[(crc ^ data[i]) & 255] ^ (crc >> 8);
	return crc ^ 0xFFFFFFFFu;
}

byte* Load(const byte* data, size_t len, int* w, int* h)
{
	if ( data==nullptr || w==nullptr || h==nullptr || len<8 || memcmp(data, "\x89PNG\r\n\x1a\n", 8)!=0 )
		return nullptr;

	byte* idat = nullptr;
	byte* raw = nullptr;
	byte* pixels = nullptr;
	byte* row = nullptr;
	byte* prev = nullptr;
	auto fail = [&]() -> byte* {
		free(idat);
		free(raw);
		free(pixels);
		free(row);
		free(prev);
		return nullptr;
	};

	ui32 crcTable[256];
	for ( unsigned i=0 ; i<256 ; ++i )
	{
		ui32 c = i;
		for ( unsigned int j=0 ; j<8 ; ++j )
			c = (c >> 1) ^ ((c & 1) ? 0xEDB88320u : 0);
		crcTable[i] = c;
	}

	ui32 width = 0, height = 0;
	unsigned channels = 0;
	size_t idatSize = 0;
	bool haveIHDR = false, haveIDAT = false, haveIEND = false;
	bool endedIDAT = false, haveTRNS = false;
	unsigned transparentR = 0, transparentG = 0, transparentB = 0;

	// Premier passage : valider les chunks et compter les donnees IDAT.
	size_t pos = 8;
	while ( pos<len )
	{
		if ( len-pos<12 )
			return fail();
		const byte* chunk = data + pos;
		const ui32 size = Read32(chunk);
		if ( size>len-pos-12 )
			return fail();
		const byte* type = chunk + 4;
		const byte* content = chunk + 8;
		if ( CRC32(type, size_t(size)+4, crcTable)!=Read32(content+size) )
			return fail();
		if ( haveIHDR==false && memcmp(type, "IHDR", 4)!=0 )
			return fail();
		if ( haveIDAT && memcmp(type, "IDAT", 4)!=0 )
			endedIDAT = true;

		if ( memcmp(type, "IHDR", 4)==0 )
		{
			if ( haveIHDR || size!=13 )
				return fail();
			width = Read32(content);
			height = Read32(content+4);
			if ( !width || !height || width>INT_MAX || height>INT_MAX ||
				content[8] != 8 || (content[9] != 2 && content[9] != 6) ||
				content[10] != 0 || content[11] != 0 || content[12] != 0)
				return fail();
			channels = content[9]==6 ? 4 : 3;
			haveIHDR = true;
		}
		else if ( memcmp(type, "IDAT", 4)==0 )
		{
			if (endedIDAT || size > size_t(-1) - idatSize) return fail();
			idatSize += size;
			haveIDAT = true;
		}
		else if ( memcmp(type, "tRNS", 4)==0 )
		{
			if ( channels!=3 || haveIDAT || haveTRNS || size!=6 )
				return fail();
			transparentR = (unsigned(content[0]) << 8) | content[1];
			transparentG = (unsigned(content[2]) << 8) | content[3];
			transparentB = (unsigned(content[4]) << 8) | content[5];
			haveTRNS = true;
		}
		else if ( memcmp(type, "PLTE", 4)==0 )
		{
			if ( haveIDAT || size==0 || size>768 || size%3 )
				return fail();
		}
		else if ( memcmp(type, "IEND", 4)==0 )
		{
			if ( haveIDAT==false || size!=0 )
				return fail();
			haveIEND = true;
			pos += size_t(size) + 12;
			break;
		}
		else if ( !(type[0] & 0x20) )
		{
			return fail(); // Chunk critique inconnu
		}
		pos += size_t(size) + 12;
	}
	if ( haveIEND==false || pos!=len || idatSize<6 )
		return fail();

	const size_t limit = size_t(-1);
	if ( size_t(width)>(limit-1)/channels || size_t(width)>limit/4 )
		return fail();
	const size_t stride = size_t(width) * channels;
	const size_t outStride = size_t(width) * 4;
	if ( size_t(height)>limit/(stride+1) || size_t(height)>limit/outStride )
		return fail();
	const size_t rawSize = (stride+1) * size_t(height);
	const size_t pixelSize = outStride * size_t(height);

	idat = (byte*)malloc(idatSize);
	raw = (byte*)malloc(rawSize);
	pixels = (byte*)malloc(pixelSize);
	row = (byte*)malloc(stride);
	prev = (byte*)calloc(stride, 1);
	if ( !idat || !raw || !pixels || !row || !prev )
		return fail();

	// Second passage : concaténer les IDAT.
	size_t copied = 0;
	for ( size_t at=8 ; at<len ; )
	{
		ui32 size = Read32(data + at);
		if ( memcmp(data+at+4, "IDAT", 4)==0 )
		{
			memcpy(idat+copied, data+at+8, size);
			copied += size;
		}
		at += size_t(size) + 12;
	}

	// Verification de l'en-tete zlib (CMF/FLG)
	ui32 header = (ui32(idat[0]) << 8) | idat[1];
	if ( (idat[0] & 15)!=8 || (idat[0]>>4)>7 || header%31!=0 || (idat[1] & 0x20) )
		return fail();

	// Decompresser DEFLATE (sans l'en-tete zlib ni Adler32).
	if ( Inflate(idat + 2, idatSize - 6, raw, rawSize)==false || Adler32(raw, rawSize)!=Read32(idat+idatSize-4) )
		return fail();

	// Annulation des filtres, puis conversion RGB/RGBA -> RGBA.
	for ( size_t y=0 ; y<height ; ++y )
	{
		const byte* src = raw + y * (stride + 1);
		unsigned filter = *src++;
		if ( filter>4 )
			return fail();

		for ( size_t x=0 ; x<stride ; ++x )
		{
			byte a = x>=channels ? row[x-channels] : 0;
			byte b = prev[x];
			byte c = x>=channels ? prev[x-channels] : 0;
			unsigned predict = 0;
			switch ( filter )
			{
				case 0: predict = 0; break;
				case 1: predict = a; break;
				case 2: predict = b; break;
				case 3: predict = (unsigned(a) + unsigned(b)) / 2; break;
				case 4: predict = Paeth(a, b, c); break;
			}
			row[x] = byte((unsigned(src[x]) + predict) & 255);
		}

		byte* dst = pixels + y * outStride;
		if ( channels==4 )
			memcpy(dst, row, outStride);
		else
		{
			for ( size_t x=0 ; x<width ; ++x )
			{
				byte r = row[x*3+0];
				byte g = row[x*3+1];
				byte b = row[x*3+2];
				dst[x*4+0] = r;
				dst[x*4+1] = g;
				dst[x*4+2] = b;
				dst[x*4+3] = (haveTRNS && r==transparentR && g==transparentG && b==transparentB) ? 0 : 255;
			}
		}
		byte* temp = prev;
		prev = row;
		row = temp;
	}

	free(idat);
	free(raw);
	free(row);
	free(prev);
	*w = int(width);
	*h = int(height);
	return pixels;
}

}
