#include "pch.h"

cpu_device::cpu_device()
{
	m_created = false;

#ifdef CPU_CONFIG_GPU
	m_pD2DFactory = nullptr;
	m_pRenderTarget = nullptr;
	m_pBitmap = nullptr;
#else
	m_bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_bi.bmiHeader.biWidth = 0;
	m_bi.bmiHeader.biHeight = 0;
	m_bi.bmiHeader.biPlanes = 1;
	m_bi.bmiHeader.biBitCount = 32;
	m_bi.bmiHeader.biCompression = BI_RGB;
#endif
}

cpu_device::~cpu_device()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool cpu_device::Create(cpu_window* pWindow, int width, int height)
{
	if ( m_created )
		return false;

	// Window
	m_pWindow = pWindow;
	m_pWindow->GetCallback()->Set(this, &cpu_device::OnWindowCallback);

	// Render
	m_width = width;
	m_height = height;

	// Buffer
	m_pRT = &m_mainRT;
	m_mainRT.width = m_width;
	m_mainRT.height = height;
	m_mainRT.aspectRatio = float(m_width)/float(m_height);
	m_mainRT.pixelCount = m_width * m_height;
	m_mainRT.widthHalf = m_width * 0.5f;
	m_mainRT.heightHalf = m_height * 0.5f;
	m_mainRT.colorBuffer.resize(m_mainRT.pixelCount);
	m_mainRT.depth = true;
	m_mainRT.depthBuffer.resize(m_mainRT.pixelCount);

	// Surface
#ifdef CPU_CONFIG_GPU
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
	if ( hr!=S_OK )
		return false;
	Fix();
#else
	m_bi.bmiHeader.biWidth = m_width;
	m_bi.bmiHeader.biHeight = -m_height;
#endif

	// Texture
	cpu_texture::Init();

	// Default
	SetDefaultCamera();
	SetDefaultLight();

	// Ready
	m_created = true;
	return true;
}

void cpu_device::Destroy()
{
	if ( m_created==false )
		return;

	m_created = false;

#ifdef CPU_CONFIG_GPU
	CPU_RELEASE(m_pBitmap);
	CPU_RELEASE(m_pRenderTarget);
	CPU_RELEASE(m_pD2DFactory);
#else
	m_bi.bmiHeader.biWidth = 0;
	m_bi.bmiHeader.biHeight = 0;
#endif

	m_pWindow->GetCallback()->Clear();
	m_pWindow = nullptr;
}

void cpu_device::Fix()
{
#ifdef CPU_CONFIG_GPU
	D2D1_SIZE_U size = D2D1::SizeU(m_pWindow->GetWidth(), m_pWindow->GetHeight());
	m_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(m_pWindow->GetHWND(), size), &m_pRenderTarget);
	
	D2D1_SIZE_U renderSize = D2D1::SizeU(m_width, m_height);
	D2D1_BITMAP_PROPERTIES props;
	props.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
	props.dpiX = 96.0f;
	props.dpiY = 96.0f;
	m_pRenderTarget->CreateBitmap(renderSize, props, &m_pBitmap);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_device::SetDefaultCamera()
{
	m_pCamera = &m_defaultCamera;
}

void cpu_device::SetCamera(cpu_camera* pCamera)
{
	m_pCamera = pCamera;
}

void cpu_device::UpdateCamera()
{
	if ( m_pCamera )
		m_pCamera->Update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_device::SetDefaultLight()
{
	m_pLight = &m_defaultLight;
}

void cpu_device::SetLight(cpu_light* pLight)
{
	m_pLight = pLight;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_rt* cpu_device::SetMainRT(bool copyDepth)
{
	if ( m_pRT==&m_mainRT )
		return m_pRT;

	cpu_rt* pOld = m_pRT;
	m_pRT = &m_mainRT;
	if ( copyDepth )
		CopyDepth(pOld);

	return pOld;
}

cpu_rt* cpu_device::SetRT(cpu_rt* pRT, bool copyDepth)
{
	if ( pRT==m_pRT )
		return m_pRT;

	cpu_rt* pOld = m_pRT;
	m_pRT = pRT;
	if ( copyDepth )
		CopyDepth(pOld);

	return pOld;
}

void cpu_device::CopyDepth(cpu_rt* pRT)
{
	if ( pRT==nullptr )
		return;

	cpu_rt& rt = *GetRT();
	if ( pRT->depth && rt.depth )
		rt.depthBuffer = pRT->depthBuffer;
}

void cpu_device::AlphaBlend(cpu_rt* pRT)
{
	if ( pRT==nullptr )
		return;

	cpu_rt& rt = *GetRT();
	cpu_img32::AlphaBlend((byte*)pRT->colorBuffer.data(), pRT->width, pRT->height, (byte*)rt.colorBuffer.data(), rt.width, rt.height, 0, 0, 0, 0, rt.width, rt.height); 
}

void cpu_device::ToAmigaStyle()
{
	cpu_rt& rt = *GetRT();
	cpu_img32::ToAmigaPalette((byte*)rt.colorBuffer.data(), rt.width, rt.height); 
}

void cpu_device::Blur(int radius)
{
	if ( radius<1 )
		return;

	cpu_rt& rt = *GetRT();
	cpu_img32::Blur((byte*)rt.colorBuffer.data(), rt.width, rt.height, radius);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_device::ClearColor()
{
	cpu_rt& rt = *GetRT();
	std::fill(rt.colorBuffer.begin(), rt.colorBuffer.end(), 0);
}

void cpu_device::ClearColor(XMFLOAT3& rgb)
{
	cpu_rt& rt = *GetRT();
	std::fill(rt.colorBuffer.begin(), rt.colorBuffer.end(), cpu::ToBGR(rgb));
}

void cpu_device::ClearDepth()
{
	cpu_rt& rt = *GetRT();
	std::fill(rt.depthBuffer.begin(), rt.depthBuffer.end(), 1.0f);
}

void cpu_device::ClearSky(XMFLOAT3& groundColor, XMFLOAT3& skyColor)
{
	cpu_rt& rt = *GetRT();

	ui32 gCol = cpu::ToBGR(groundColor);
	ui32 sCol = cpu::ToBGR(skyColor);

	XMMATRIX matView = XMLoadFloat4x4(&m_pCamera->matView);
	XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR viewUp = XMVector3TransformNormal(worldUp, matView);

	float nx = XMVectorGetX(viewUp);
	float ny = XMVectorGetY(viewUp);
	float nz = XMVectorGetZ(viewUp);

	float p11 = m_pCamera->matProj._11; // Cotan(FovX/2) / Aspect
	float p22 = m_pCamera->matProj._22; // Cotan(FovY/2)

	float a = nx / (p11 * rt.widthHalf);
	float b = -ny / (p22 * rt.heightHalf); // Le signe '-' compense l'axe Y inversé de l'écran
	float c = nz - (nx / p11) + (ny / p22);

	bool rightSideIsSky = a > 0;
	
	ui32 colLeft  = rightSideIsSky ? gCol : sCol;
	ui32 colRight = rightSideIsSky ? sCol : gCol;

	if ( fabsf(a)<0.000001f )
	{
		for ( int y=0 ; y<rt.height ; y++ )
		{
			float val = b * (float)y + c;
			ui32 col = val>0.0f ? sCol : gCol;
			std::fill(rt.colorBuffer.begin() + (y * rt.width),  rt.colorBuffer.begin() + ((y + 1) * rt.width), col);
		}
	}
	else
	{
		float invA = -1.0f / a;
		for ( int y=0 ; y<rt.height ; y++ )
		{
			float fSplitX = (b * (float)y + c) * invA;
			int splitX;
			if ( fSplitX<0.0f )
				splitX = 0;
			else if ( fSplitX>(float)rt.width)
				splitX = rt.width;
			else
				splitX = (int)fSplitX;

			ui32* rowPtr = rt.colorBuffer.data() + (y * rt.width);
			if ( splitX>0 )
				std::fill(rowPtr, rowPtr + splitX, colLeft);
			if ( splitX<rt.width )
				std::fill(rowPtr + splitX, rowPtr + rt.width, colRight);
		}
	}

	// Sky Line
	////////////

	float bandPx = rt.height/20.0f;
	if ( bandPx<=0.5f )
		return;

	float grad = sqrtf(a*a + b*b);
	if ( grad<CPU_EPSILON )
		return;

	const float invGrad = 1.0f / grad;
	const float invBand = 1.0f / bandPx;
	const float limit   = bandPx * grad;

	if ( fabsf(a)<CPU_EPSILON )
	{
		for ( int y=0 ; y<rt.height ; ++y )
		{
			float distPx = (b * (float)y + c) * invGrad;
			if ( fabsf(distPx)>bandPx )
				continue;
			float t = cpu::Clamp(0.5f + distPx * invBand);
			t = t * t * (3.0f - 2.0f * t); // optional
			ui32 col = cpu::LerpColor(gCol, sCol, t);
			ui32* row = rt.colorBuffer.data() + y * rt.width;
			std::fill(row, row + rt.width, col);
		}
		return;
	}

	for ( int y=0 ; y<rt.height ; ++y )
	{
		ui32* row = rt.colorBuffer.data() + y * rt.width;
		float byc = b * (float)y + c;
		float xA = (-limit - byc) / a;
		float xB = ( +limit - byc) / a;
		float xMinF = (xA < xB) ? xA : xB;
		float xMaxF = (xA > xB) ? xA : xB;
		int x0 = cpu::FloorToInt(xMinF);
		int x1 = cpu::CeilToInt(xMaxF);
		if ( x1<0 || x0>=rt.width )
			continue;
		if ( x0<0 )
			x0 = 0;
		if ( x1>=rt.width)
			x1 = rt.width - 1;
		float val = byc + a * (float)x0;
		ui32* p = row + x0;
		ui32* e = row + x1 + 1;
		while ( p!=e )
		{
			float distPx = val * invGrad;
			float t = cpu::Clamp(0.5f + distPx * invBand);
			t = t * t * (3.0f - 2.0f * t); // optional
			*p++ = cpu::LerpColor(gCol, sCol, t);
			val += a;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_device::DrawMesh(cpu_mesh* pMesh, cpu_transform* pTransform, cpu_material* pMaterial, int depthMode, cpu_tile* pTile)
{
	cpu_rt& rt = *GetRT();
	cpu_material& material = pMaterial ? *pMaterial : m_defaultMaterial;
	XMMATRIX matWorld = XMLoadFloat4x4(&pTransform->GetWorld());
	XMMATRIX matNormal = XMMatrixTranspose(XMLoadFloat4x4(&pTransform->GetInvWorld()));
	XMMATRIX matViewProj = XMLoadFloat4x4(&m_pCamera->matViewProj);
	XMVECTOR lightDir = XMLoadFloat3(&m_pLight->dir);

	cpu_draw draw;
	draw.pMaterial = &material;
	draw.pTile = pTile;
	draw.depth = depthMode;

	cpu_vertex_out vo[3];
	cpu_vertex_out clipped[8];
	for ( const cpu_triangle& triangle : pMesh->triangles )
	{
		for ( int i=0 ; i<3 ; ++i )
		{
			// Vertex
			const cpu_vertex& in = triangle.v[i];

			// World pos
			XMVECTOR loc = XMLoadFloat3(&in.pos);
			loc = XMVectorSetW(loc, 1.0f);
			XMVECTOR world = XMVector4Transform(loc, matWorld);
			XMStoreFloat3(&vo[i].worldPos, world);

			// Clip pos
			XMVECTOR clip = XMVector4Transform(world, matViewProj);
			XMStoreFloat4(&vo[i].clipPos, clip);
			float w = vo[i].clipPos.w;
			float invW = fabsf(w)>CPU_EPSILON ? (1.0f/w) : 0.0f;

			// World normal
			XMVECTOR localNormal = XMLoadFloat3(&in.normal);
			XMVECTOR worldNormal = XMVector3TransformNormal(localNormal, matNormal);
			worldNormal = XMVector3Normalize(worldNormal);
			XMStoreFloat3(&vo[i].worldNormal, worldNormal);

			// Albedo
			vo[i].albedo.x = cpu::Clamp(in.color.x * material.color.x);
			vo[i].albedo.y = cpu::Clamp(in.color.y * material.color.y);
			vo[i].albedo.z = cpu::Clamp(in.color.z * material.color.z);

			// Intensity
			float ndotl = XMVectorGetX(XMVector3Dot(worldNormal, lightDir));
			ndotl = std::max(0.0f, ndotl);
			vo[i].intensity = ndotl + m_pLight->ambient;

			// UV
			vo[i].uv.x = in.uv.x * invW;
			vo[i].uv.y = in.uv.y * invW;
		}

		// Clipping
		int count = ClipTriangleFrustum(vo, clipped);
		if ( count==0 )
			continue;

		// Triangulation fan: (0, i, i+1)
		for ( int i=1 ; i+1<count ; ++i )
		{
			draw.vo[0] = &clipped[0];
			draw.vo[1] = &clipped[i];
			draw.vo[2] = &clipped[i+1];
			if ( ClipToScreen(draw) )
				DrawTriangle(draw);
		}
	}
}

void cpu_device::DrawWireframeMesh(cpu_mesh* pMesh, FXMMATRIX matrix, cpu_tile* pTile)
{
	cpu_rt& rt = *GetRT();
	XMMATRIX matViewProj = XMLoadFloat4x4(&m_pCamera->matViewProj);

	XMFLOAT4 clip[3];
	for ( const cpu_triangle& triangle : pMesh->triangles )
	{
		for ( int i=0 ; i<3 ; ++i )
		{
			const cpu_vertex& in = triangle.v[i];
			XMVECTOR loc = XMLoadFloat3(&in.pos);
			loc = XMVectorSetW(loc, 1.0f);
			XMVECTOR world = XMVector4Transform(loc, matrix);
			XMVECTOR c = XMVector4Transform(world, matViewProj);
			XMStoreFloat4(&clip[i], c);
		}
		
		const int edges[3][2] = { {0,1}, {1,2}, {2,0} };
		for ( int e=0 ; e<3 ; ++e )
		{
			XMFLOAT4 a = clip[edges[e][0]];
			XMFLOAT4 b = clip[edges[e][1]];
			if ( WireframeClipLineNearPlane(a, b)==false )
				continue;

			XMFLOAT3 sa, sb;
			if ( WireframeClipToScreen(a, rt.widthHalf, rt.heightHalf, sa)==false )
				continue;
			if ( WireframeClipToScreen(b, rt.widthHalf, rt.heightHalf, sb)==false )
				continue;

			DrawLine((int)sa.x, (int)sa.y, sa.z, (int)sb.x, (int)sb.y, sb.z, CPU_WHITE);
		}
	}
}

void cpu_device::DrawText(cpu_font* pFont, const char* text, int x, int y, int align)
{
	if ( pFont==nullptr || pFont->bgra.size()==0 || text==nullptr )
		return;

	cpu_rt& rt = *GetRT();
	const int cw = pFont->advance;
	const int ch = pFont->cellH;
	const char* p = text;
	const char* lineStart = text;
	int lineIndex = 0;

	auto DrawLine = [&](const char* start, int len, int penY)
	{
		int penX = x;
		if ( align==CPU_TEXT_CENTER ) penX = x - (len * cw) / 2;
		else if ( align==CPU_TEXT_RIGHT ) penX = x - (len * cw);
		for ( int i=0 ; i<len ; ++i )
		{
			const byte c = (byte)start[i];
			const cpu_glyph& g = pFont->glyph[c];
			if ( g.valid==false )
			{
				penX += cw;
				continue;
			}
			cpu_img32::AlphaBlend(pFont->bgra.data(), pFont->width, pFont->height, (byte*)rt.colorBuffer.data(), rt.width, rt.height, g.x, g.y, penX, penY, g.w, g.h);
			penX += cw;
		}
	};

	while ( true )
	{
		const char c = *p;
		if ( c=='\n' || c=='\0' )
		{
			const int len = (int)(p - lineStart);
			DrawLine(lineStart, len, y + lineIndex * ch);
			++lineIndex;
			if ( c=='\0' )
				break;
			++p;
			lineStart = p;
			continue;
		}
		++p;
	}
}

void cpu_device::DrawTexture(cpu_texture* pTexture, int x, int y)
{
	cpu_rt& rt = *GetRT();
	byte* dst = (byte*)rt.colorBuffer.data();
	cpu_img32::AlphaBlend(pTexture->bgra, pTexture->width, pTexture->height, dst, rt.width, rt.height, 0, 0, x, y, pTexture->width, pTexture->height);
}

void cpu_device::DrawSprite(cpu_sprite* pSprite)
{
	DrawTexture(pSprite->pTexture, pSprite->x-pSprite->anchorX, pSprite->y-pSprite->anchorY);
}

void cpu_device::DrawHorzLine(int x1, int x2, int y, XMFLOAT3& color)
{
	cpu_rt& rt = *GetRT();
	COLORREF bgr = cpu::ToBGR(color);
	if ( y<0 || y>=rt.height )
		return;
	x1 = cpu::Clamp(x1, 0, rt.width);
	x2 = cpu::Clamp(x2, 0, rt.width);
	ui32* buf = rt.colorBuffer.data() + y*rt.width;
	for ( int i=x1 ; i<x2 ; i++ )
		buf[i] = bgr;
}

void cpu_device::DrawVertLine(int y1, int y2, int x, XMFLOAT3& color)
{
	cpu_rt& rt = *GetRT();
	COLORREF bgr = cpu::ToBGR(color);
	if ( x<0 || x>=rt.width )
		return;
	y1 = cpu::Clamp(y1, 0, rt.height);
	y2 = cpu::Clamp(y2, 0, rt.height);
	ui32* buf = rt.colorBuffer.data() + y1*rt.width + x;
	for ( int i=y1 ; i<y2 ; i++ )
	{
		*buf = bgr;
		buf += rt.width;
	}
}

void cpu_device::DrawRectangle(int x, int y, int w, int h, XMFLOAT3& color)
{
	DrawHorzLine(x, x+w, y, color);
	DrawHorzLine(x, x+w, y+h, color);
	DrawVertLine(y, y+h, x, color);
	DrawVertLine(y, y+h, x+w, color);
}

void cpu_device::DrawLine(int x0, int y0, float z0, int x1, int y1, float z1, XMFLOAT3& color)
{
	cpu_rt& rt = *GetRT();
	ui32 bgr = cpu::ToBGR(color);

	int dx = abs(x1 - x0);
	int sx = x0 < x1 ? 1 : -1;
	int dy = -abs(y1 - y0);
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;

	float dist = (float)std::max(dx, abs(dy));
	if ( dist==0.0f )
		return;

	float zStep = (z1 - z0) / dist;
	float currentZ = z0;

	while ( true )
	{
		if ( x0>=0 && x0<rt.width && y0>=0 && y0<rt.height )
		{
			int index = y0 * rt.width + x0;
			if ( currentZ<rt.depthBuffer[index] )
			{
				rt.colorBuffer[index] = bgr;
				rt.depthBuffer[index] = currentZ;
			}
		}

		if ( x0==x1 && y0==y1 )
			break;

		int e2 = 2 * err;
		if ( e2>=dy )
		{
			err += dy;
			x0 += sx;
		}
		if ( e2<=dx )
		{
			err += dx;
			y0 += sy;
		}

		currentZ += zStep;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_device::Present()
{
	cpu_rt& rt = *GetRT();

#ifdef CPU_CONFIG_GPU

	if ( m_pRenderTarget==nullptr || m_pBitmap==nullptr )
		return;

	m_pRenderTarget->BeginDraw();
	m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
	m_pBitmap->CopyFromMemory(nullptr, rt.colorBuffer.data(), rt.width*4);
	D2D1_RECT_F destRect = D2D1::RectF((float)m_rcFit.left, (float)m_rcFit.top, (float)m_rcFit.right, (float)m_rcFit.bottom);	
	m_pRenderTarget->DrawBitmap(m_pBitmap, destRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
	HRESULT hr = m_pRenderTarget->EndDraw();
	if ( hr==D2DERR_RECREATE_TARGET )
	{
		m_pBitmap->Release();
		m_pBitmap = nullptr;
		m_pRenderTarget->Release();
		m_pRenderTarget = nullptr;
		Fix();
	}

#else

	HDC hDC = GetDC(m_pWindow->GetHWND());
	SetStretchBltMode(hDC, COLORONCOLOR);
	HBRUSH hBrush = (HBRUSH)SelectObject(hDC, GetStockObject(BLACK_BRUSH));
	int width = m_rcFit.right - m_rcFit.left;
	int height = m_rcFit.bottom - m_rcFit.top;
	if ( width==rt.width && height==rt.height )
		SetDIBitsToDevice(hDC, m_rcFit.left, m_rcFit.top, rt.width, rt.height, 0, 0, 0, rt.height, rt.colorBuffer.data(), &m_bi, DIB_RGB_COLORS);
	else
	{
		StretchDIBits(hDC, m_rcFit.left, m_rcFit.top, width, height, 0, 0, rt.width, rt.height, rt.colorBuffer.data(), &m_bi, DIB_RGB_COLORS, SRCCOPY);
		if ( m_rcFit.left )
			PatBlt(hDC, 0, 0, m_rcFit.left, m_pWindow->GetHeight(), PATCOPY);
		if ( m_rcFit.right<m_pWindow->GetWidth() )
			PatBlt(hDC, m_rcFit.right, 0, m_pWindow->GetWidth()-m_rcFit.right, m_pWindow->GetHeight(), PATCOPY);
		if ( m_rcFit.top )
			PatBlt(hDC, 0, 0, m_pWindow->GetWidth(), m_rcFit.top, PATCOPY);
		if ( m_rcFit.bottom<m_pWindow->GetHeight() )
			PatBlt(hDC, 0, m_rcFit.bottom, m_pWindow->GetWidth(), m_pWindow->GetHeight()-m_rcFit.bottom, PATCOPY);
	}
	SelectObject(hDC, hBrush);
	ReleaseDC(m_pWindow->GetHWND(), hDC);

#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_device::OnWindowCallback(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch ( message )
	{
		case WM_SHOWWINDOW:
		case WM_SIZE:
		{
			m_rcFit = cpu::ComputeAspectFitRect(m_mainRT.width, m_mainRT.height, m_pWindow->GetWidth(), m_pWindow->GetHeight());
#ifdef CPU_CONFIG_GPU
			if ( m_pRenderTarget )
				m_pRenderTarget->Resize(D2D1::SizeU(m_pWindow->GetWidth(), m_pWindow->GetHeight()));
#endif
			break;
		}
	}
}

bool cpu_device::ClipToScreen(cpu_draw& draw)
{
	cpu_rt& rt = *GetRT();
	for ( int i=0 ; i<3 ; ++i )
	{
		float w = draw.vo[i]->clipPos.w;
		if ( w<CPU_EPSILON )
			return false;

		// NDC (DirectX: z in [0..1])
		float invW = 1.0f / w;
		float ndcX = draw.vo[i]->clipPos.x * invW; // [-1,1]
		float ndcY = draw.vo[i]->clipPos.y * invW; // [-1,1]
		float ndcZ = draw.vo[i]->clipPos.z * invW; // [0,1]

		draw.tri[i].x = (ndcX + 1.0f) * rt.widthHalf;
		draw.tri[i].y = (1.0f - ndcY) * rt.heightHalf;
		draw.tri[i].z = cpu::Clamp(ndcZ);
	}

	// Culling
	float area = (draw.tri[1].x-draw.tri[0].x) * (draw.tri[2].y-draw.tri[0].y);
	area -= (draw.tri[1].y-draw.tri[0].y) * (draw.tri[2].x-draw.tri[0].x);
	bool isFront = m_cullFrontCCW ? (area>m_cullAreaEpsilon) : (area<-m_cullAreaEpsilon);
	if ( isFront==false )
		return false;

	return true;
}

void cpu_device::DrawTriangle(cpu_draw& draw)
{
	cpu_rt& rt = *GetRT();

	const float x1 = draw.tri[0].x, y1 = draw.tri[0].y, z1 = draw.tri[0].z;
	const float x2 = draw.tri[1].x, y2 = draw.tri[1].y, z2 = draw.tri[1].z;
	const float x3 = draw.tri[2].x, y3 = draw.tri[2].y, z3 = draw.tri[2].z;

	int minX = (int)floor(std::min(std::min(x1, x2), x3));
	int maxX = (int)ceil(std::max(std::max(x1, x2), x3));
	int minY = (int)floor(std::min(std::min(y1, y2), y3));
	int maxY = (int)ceil(std::max(std::max(y1, y2), y3));

	if ( minX<0 ) minX = 0;
	if ( minY<0 ) minY = 0;
	if ( maxX>rt.width ) maxX = rt.width;
	if ( maxY>rt.height ) maxY = rt.height;

	if ( draw.pTile )
	{
		if ( minX<draw.pTile->left ) minX = draw.pTile->left;
		if ( maxX>draw.pTile->right ) maxX = draw.pTile->right;
		if ( minY<draw.pTile->top ) minY = draw.pTile->top;
		if ( maxY>draw.pTile->bottom ) maxY = draw.pTile->bottom;
	}

	if ( minX>=maxX || minY>=maxY )
		return;

	float a12 = y1 - y2;
	float b12 = x2 - x1;
	float c12 = x1 * y2 - x2 * y1;
	float a23 = y2 - y3;
	float b23 = x3 - x2;
	float c23 = x2 * y3 - x3 * y2;
	float a31 = y3 - y1;
	float b31 = x1 - x3;
	float c31 = x3 * y1 - x1 * y3;
	float area = a12 * x3 + b12 * y3 + c12;
	if ( fabsf(area)<CPU_EPSILON )
		return;

	float invArea = 1.0f / area;
	bool areaPositive = area>0.0f;
	float startX = (float)minX + 0.5f;
	float startY = (float)minY + 0.5f;
	float e12_row = a12 * startX + b12 * startY + c12;
	float e23_row = a23 * startX + b23 * startY + c23;
	float e31_row = a31 * startX + b31 * startY + c31;
	const float dE12dx = a12;
	const float dE12dy = b12;
	const float dE23dx = a23;
	const float dE23dy = b23;
	const float dE31dx = a31;
	const float dE31dy = b31;

	float invW0 = 0.0f;
	float invW1 = 0.0f;
	float invW2 = 0.0f;
	if ( draw.pMaterial->pTexture )
	{
		invW0 = 1.0f / draw.vo[0]->clipPos.w;
		invW1 = 1.0f / draw.vo[1]->clipPos.w;
		invW2 = 1.0f / draw.vo[2]->clipPos.w;
	}

	const CPU_PS_FUNC func = draw.pMaterial->ps ? draw.pMaterial->ps : &PixelShader;
	cpu_ps_io io;
	io.pMaterial = draw.pMaterial;
	for ( int y=minY ; y<maxY ; ++y )
	{
		float e12 = e12_row;
		float e23 = e23_row;
		float e31 = e31_row;
		for ( int x=minX ; x<maxX ; ++x )
		{
			if ( areaPositive )
			{
				if ( e12<0.0f || e23<0.0f || e31<0.0f )
				{
					e12 += dE12dx;
					e23 += dE23dx;
					e31 += dE31dx;
					continue;
				}
			}
			else
			{
				if ( e12>0.0f || e23>0.0f || e31>0.0f )
				{
					e12 += dE12dx;
					e23 += dE23dx;
					e31 += dE31dx;
					continue;
				}
			}

			float w1 = e23 * invArea;
			float w2 = e31 * invArea;
			float w3 = e12 * invArea;
			float z = z3 + (z1 - z3) * w1 + (z2 - z3) * w2; // => z1 * w1 + z2 * w2 + z3 * w3
			int index = y * rt.width + x;
			if ( (draw.depth & CPU_DEPTH_READ) && z>=rt.depthBuffer[index] )
			{
				e12 += dE12dx;
				e23 += dE23dx;
				e31 += dE31dx;
				continue;
			}

			// cpu_input
			io.p.x = x;
			io.p.y = y;
			io.p.depth = z;

			// Position (lerp)
			io.p.pos.x = draw.vo[2]->worldPos.x + (draw.vo[0]->worldPos.x - draw.vo[2]->worldPos.x) * w1 + (draw.vo[1]->worldPos.x - draw.vo[2]->worldPos.x) * w2;
			io.p.pos.y = draw.vo[2]->worldPos.y + (draw.vo[0]->worldPos.y - draw.vo[2]->worldPos.y) * w1 + (draw.vo[1]->worldPos.y - draw.vo[2]->worldPos.y) * w2;
			io.p.pos.z = draw.vo[2]->worldPos.z + (draw.vo[0]->worldPos.z - draw.vo[2]->worldPos.z) * w1 + (draw.vo[1]->worldPos.z - draw.vo[2]->worldPos.z) * w2;

			// Normal (lerp)
			io.p.normal.x = draw.vo[2]->worldNormal.x + (draw.vo[0]->worldNormal.x - draw.vo[2]->worldNormal.x) * w1 + (draw.vo[1]->worldNormal.x - draw.vo[2]->worldNormal.x) * w2;
			io.p.normal.y = draw.vo[2]->worldNormal.y + (draw.vo[0]->worldNormal.y - draw.vo[2]->worldNormal.y) * w1 + (draw.vo[1]->worldNormal.y - draw.vo[2]->worldNormal.y) * w2;
			io.p.normal.z = draw.vo[2]->worldNormal.z + (draw.vo[0]->worldNormal.z - draw.vo[2]->worldNormal.z) * w1 + (draw.vo[1]->worldNormal.z - draw.vo[2]->worldNormal.z) * w2;

			// Color (lerp)
			io.p.albedo.x = draw.vo[2]->albedo.x + (draw.vo[0]->albedo.x - draw.vo[2]->albedo.x) * w1 + (draw.vo[1]->albedo.x - draw.vo[2]->albedo.x) * w2;
			io.p.albedo.y = draw.vo[2]->albedo.y + (draw.vo[0]->albedo.y - draw.vo[2]->albedo.y) * w1 + (draw.vo[1]->albedo.y - draw.vo[2]->albedo.y) * w2;
			io.p.albedo.z = draw.vo[2]->albedo.z + (draw.vo[0]->albedo.z - draw.vo[2]->albedo.z) * w1 + (draw.vo[1]->albedo.z - draw.vo[2]->albedo.z) * w2;

			// UV (lerp)
			if ( draw.pMaterial->pTexture )
			{
				float invW = w1*invW0 + w2*invW1 + w3*invW2;
				io.p.uv.x = (w1*draw.vo[0]->uv.x + w2*draw.vo[1]->uv.x + w3*draw.vo[2]->uv.x) / invW;
				io.p.uv.y = (w1*draw.vo[0]->uv.y + w2*draw.vo[1]->uv.y + w3*draw.vo[2]->uv.y) / invW;
			}
			else
			{
				io.p.uv.x = 0.0f;
				io.p.uv.y = 0.0f;
			}

			// Lighting
			if ( draw.pMaterial->lighting==CPU_LIGHTING_GOURAUD )
			{
				float intensity = draw.vo[2]->intensity + (draw.vo[0]->intensity - draw.vo[2]->intensity) * w1 + (draw.vo[1]->intensity - draw.vo[2]->intensity) * w2;
				io.p.color.x = io.p.albedo.x * intensity;
				io.p.color.y = io.p.albedo.y * intensity;
				io.p.color.z = io.p.albedo.z * intensity;
			}
			else if ( draw.pMaterial->lighting==CPU_LIGHTING_LAMBERT )
			{
				XMVECTOR n = XMLoadFloat3(&io.p.normal);				
				//n = XMVector3Normalize(n); // Expensive (better results)
				XMVECTOR l = XMLoadFloat3(&m_pLight->dir);
				float ndotl = XMVectorGetX(XMVector3Dot(n, l));
				if ( ndotl<0.0f )
					ndotl = 0.0f;
				float intensity = ndotl + m_pLight->ambient;
				io.p.color.x = io.p.albedo.x * intensity;
				io.p.color.y = io.p.albedo.y * intensity;
				io.p.color.z = io.p.albedo.z * intensity;
			}
			else
				io.p.color = io.p.albedo;

			// Output
			io.values = draw.pMaterial->values;
			io.color = {};
			io.discard = false;
			func(io);
			if ( io.discard==false )
			{
				if ( draw.depth & CPU_DEPTH_WRITE )
					rt.depthBuffer[index] = z;
				rt.colorBuffer[index] = cpu::ToBGR(io.color);
			}

			e12 += dE12dx;
			e23 += dE23dx;
			e31 += dE31dx;
		}

		e12_row += dE12dy;
		e23_row += dE23dy;
		e31_row += dE31dy;
	}

	// Stats
	if ( draw.pTile )
		draw.pTile->statsDrawnTriangleCount++;
}

bool cpu_device::WireframeClipLineNearPlane(XMFLOAT4& a, XMFLOAT4& b)
{
	// Plan near D3D en clip-space : z >= 0
	const float da = a.z;
	const float db = b.z;

	// Si les deux sont derrière
	if ( da<0.0f && db<0.0f )
		return false;

	// Si un seul est derrière, on intersecte
	if ( da<0.0f || db<0.0f )
	{
		// t tel que z(t)=0 entre a et b
		float t = da / (da - db); // da + t*(db-da) = 0
		XMFLOAT4 p;
		p.x = a.x + (b.x - a.x) * t;
		p.y = a.y + (b.y - a.y) * t;
		p.z = a.z + (b.z - a.z) * t; // ~0
		p.w = a.w + (b.w - a.w) * t;

		if ( da<0.0f )
			a = p;
		else
			b = p;
	}

	if ( a.w<CPU_EPSILON && b.w<CPU_EPSILON )
		return false;

	return true;
}

bool cpu_device::WireframeClipToScreen(const XMFLOAT4& c, float widthHalf, float heightHalf, XMFLOAT3& out)
{
	if ( c.w<CPU_EPSILON )
		return false;

	float invW = 1.0f / c.w;
	float ndcX = c.x * invW;
	float ndcY = c.y * invW;
	float ndcZ = c.z * invW;
	out.x = (ndcX + 1.0f) * widthHalf;
	out.y = (1.0f - ndcY) * heightHalf;
	out.z = cpu::Clamp(ndcZ);
	return true;
}

float cpu_device::PlaneEval(const XMFLOAT4& p, const XMFLOAT4& c)
{
	return p.x * c.x + p.y * c.y + p.z * c.z + p.w * c.w;
}

int cpu_device::ClipPolyAgainstPlane(const cpu_vertex_out* pInV, int inCount, cpu_vertex_out* pOutV, const XMFLOAT4& plane)
{
	if ( inCount<=0 )
		return 0;

	int outCount = 0;

	const cpu_vertex_out* pA = &pInV[inCount-1];
	float dA = PlaneEval(plane, pA->clipPos);

	for ( int i=0 ; i<inCount ; ++i )
	{
		const cpu_vertex_out* pB = &pInV[i];
		float dB = PlaneEval(plane, pB->clipPos);

		const bool ain = dA>=0.0f;
		const bool bin = dB>=0.0f;

		if ( ain && bin )
		{
			pOutV[outCount++] = *pB;
		}
		else if ( ain && bin==false )
		{
			float denom = dA - dB;
			if ( fabsf(denom)>1e-12f )
			{
				float t = dA / denom; // A + t(B-A) est sur le plan
				pOutV[outCount++].Lerp(*pA, *pB, t);
			}
		}
		else if ( ain==false && bin )
		{
			float denom = dA - dB;
			if ( fabsf(denom)>1e-12f )
			{
				float t = dA / denom;
				pOutV[outCount++].Lerp(*pA, *pB, t);
			}
			pOutV[outCount++] = *pB;
		}

		pA = pB;
		dA = dB;
		if ( outCount>=8 )
			break;
	}

	return outCount;
}

int cpu_device::ClipTriangleFrustum(const cpu_vertex_out tri[3], cpu_vertex_out outV[8])
{
	cpu_vertex_out bufA[8];
	cpu_vertex_out bufB[8];

	int n = 3;
	bufA[0] = tri[0];
	bufA[1] = tri[1];
	bufA[2] = tri[2];

	// Plans D3D: dot(plane, clip) >= 0
	// Left   : x + w >= 0  => ( 1, 0, 0, 1)
	// Right  : -x + w >= 0 => (-1, 0, 0, 1)
	// Bottom : y + w >= 0  => ( 0, 1, 0, 1)
	// Top    : -y + w >= 0 => ( 0,-1, 0, 1)
	// Near   : z >= 0      => ( 0, 0, 1, 0)
	// Far    : -z + w >= 0 => ( 0, 0,-1, 1)  <=> z <= w
	const XMFLOAT4 planes[6] =
	{
		{  1.f,  0.f,  0.f,  1.f },
		{ -1.f,  0.f,  0.f,  1.f },
		{  0.f,  1.f,  0.f,  1.f },
		{  0.f, -1.f,  0.f,  1.f },
		{  0.f,  0.f,  1.f,  0.f },
		{  0.f,  0.f, -1.f,  1.f },
	};

	const cpu_vertex_out* inBuf = bufA;
	cpu_vertex_out* outBuf = bufB;

	for ( int p=0 ; p<6 ; ++p )
	{
		n = ClipPolyAgainstPlane(inBuf, n, outBuf, planes[p]);
		if ( n==0 )
			return 0;

		// swap buffers
		const cpu_vertex_out* tmp = inBuf;
		inBuf = outBuf;
		outBuf = tmp==bufA ? bufA : bufB;
	}

	for ( int i=0 ; i<n ; ++i )
		outV[i] = inBuf[i];
	return n; // 3..7
}

void cpu_device::PixelShader(cpu_ps_io& io)
{
	if ( io.pMaterial->pTexture )
	{
		XMFLOAT3 texel;
		io.pMaterial->pTexture->Sample(texel, io.p.uv.x, io.p.uv.y);
		io.color.x = io.p.color.x * texel.x;
		io.color.y = io.p.color.y * texel.y;
		io.color.z = io.p.color.z * texel.z;
		//io.color = texel;
	}
	else
		io.color = io.p.color;
}
