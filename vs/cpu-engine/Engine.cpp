#include "stdafx.h"

Engine* Engine::s_pEngine;

Engine::Engine()
{
	s_pEngine = this;
	m_hInstance = nullptr;
	m_hWnd = nullptr;
	m_threads = nullptr;

#ifdef CONFIG_GPU
	m_pD2DFactory = nullptr;
	m_pRenderTarget = nullptr;
	m_pBitmap = nullptr;
#else
	m_hDC = nullptr;
	m_bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_bi.bmiHeader.biWidth = 0;
	m_bi.bmiHeader.biHeight = 0;
	m_bi.bmiHeader.biPlanes = 1;
	m_bi.bmiHeader.biBitCount = 32;
	m_bi.bmiHeader.biCompression = BI_RGB;
#endif
}

Engine::~Engine()
{
	assert( m_hInstance==nullptr );
}

Engine* Engine::Instance()
{
	return s_pEngine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::Initialize(HINSTANCE hInstance, int renderWidth, int renderHeight, float windowScaleAtStart, bool bilinear)
{
	if ( m_hInstance )
		return;

	// Window
	m_hInstance = hInstance;
	m_windowWidth = (int)(renderWidth*windowScaleAtStart);
	m_windowHeight = (int)(renderHeight*windowScaleAtStart);
	m_bilinear = bilinear;
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = "CPU-ENGINE";
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	RegisterClass(&wc);
	RECT rect = { 0, 0, m_windowWidth, m_windowHeight };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	m_hWnd = CreateWindow("CPU-ENGINE", "CPU ENGINE", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, rect.right-rect.left, rect.bottom-rect.top, nullptr, nullptr, hInstance, nullptr);
	SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);

	// Buffer
	m_renderWidth = renderWidth;
	m_renderHeight = renderHeight;
	m_renderPixelCount = m_renderWidth * m_renderHeight;
	m_renderWidthHalf = m_renderWidth * 0.5f;
	m_renderHeightHalf = m_renderHeight * 0.5f;
	m_colorBuffer.resize(m_renderPixelCount);
	m_depthBuffer.resize(m_renderPixelCount);

	// Surface
#ifdef CONFIG_GPU
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
	FixDevice();
#else
	m_bi.bmiHeader.biWidth = m_renderWidth;
	m_bi.bmiHeader.biHeight = -m_renderHeight;
	m_hDC = GetDC(m_hWnd);
	SetStretchBltMode(m_hDC, COLORONCOLOR);
#endif

	// Colors
	m_sky = true;
	m_clearColor = ToColor(32, 32, 64);
	m_groundColor = ToColor(32, 64, 32);
	m_skyColor = ToColor(32, 32, 64);

	// Light
	m_lightDir = { 0.5f, -1.0f, 0.5f };
	XMStoreFloat3(&m_lightDir, XMVector3Normalize(XMLoadFloat3(&m_lightDir)));
	m_ambient = 0.2f;

	// Entity
	m_entityCount = 0;
	m_bornEntityCount = 0;
	m_deadEntityCount = 0;

	// Tile
#ifdef CONFIG_MT
	m_threadCount = std::max(1u, std::thread::hardware_concurrency());
#else
	m_threadCount = 1;
#endif
	m_statsThreadCount = m_threadCount;
	m_tileColCount = static_cast<unsigned int>(std::ceil(std::sqrt(m_threadCount)));
	m_tileRowCount = (m_threadCount + m_tileColCount - 1) / m_tileColCount;
	m_tileCount = m_tileColCount * m_tileRowCount;
	m_statsTileCount = m_tileCount;
	m_tileWidth = m_renderWidth / m_tileColCount;
	m_tileHeight = m_renderHeight / m_tileRowCount;
	int missingWidth = m_renderWidth - (m_tileWidth*m_tileColCount);
	int missingHeight = m_renderHeight - (m_tileHeight*m_tileRowCount);
	for ( int row=0 ; row<m_tileRowCount ; row++ )
	{
		for ( int col=0 ; col<m_tileColCount ; col++ )
		{
			TILE tile;
			tile.row = row;
			tile.col = col;
			tile.index = (int)m_tiles.size();
			tile.left = col * m_tileWidth;
			tile.top = row * m_tileHeight;
			tile.right = (col+1) * m_tileWidth;
			tile.bottom = (row+1) * m_tileHeight;
			if ( row==m_tileRowCount-1 )
				tile.bottom += missingHeight;
			if ( col==m_tileColCount-1 )
				tile.right += missingWidth;
			m_tiles.push_back(tile);
		}
	}

	// Thread
	m_threads = new ThreadJob[m_threadCount];
	for ( int i=0 ; i<m_threadCount ; i++ )
	{
		ThreadJob& thread = m_threads[i];
		thread.m_count = m_tileCount;
		thread.m_hEventStart = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		thread.m_hEventEnd = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	// Window
	ShowWindow(m_hWnd, SW_SHOW);
}

void Engine::Uninitialize()
{
	if ( m_hInstance==nullptr )
		return;

	// Thread
	for ( int i=0 ; i<m_threadCount ; i++ )
	{
		ThreadJob& thread = m_threads[i];
		CloseHandle(thread.m_hEventStart);
		CloseHandle(thread.m_hEventEnd);
	}
	delete [] m_threads;
	m_threads = nullptr;

	// Surface
#ifdef CONFIG_GPU
	RELPTR(m_pBitmap);
	RELPTR(m_pRenderTarget);
	RELPTR(m_pD2DFactory);
#else
	if ( m_hDC )
	{
		ReleaseDC(m_hWnd, m_hDC);
		m_hDC = nullptr;
	}
#endif

	// Window
	if ( m_hWnd )
	{
		DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}
	UnregisterClass("RETRO_ENGINE", m_hInstance);
	m_hInstance = nullptr;
}

void Engine::Run()
{
	// Camera
	FixWindow();
	FixProjection();

	// Start
	m_systime = timeGetTime();
	m_time = 0.0f;
	m_dt = 0.0f;
	m_fpsTime = 0;
	m_fpsCount = 0;
	m_fps = 0;
	OnStart();

	// Thread
	for ( int i=0 ; i<m_threadCount ; i++ )
		m_threads[i].Run();

	// Loop
	MSG msg = {};
	while ( msg.message!=WM_QUIT )
	{
		// Windows API
		while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
		{
			if ( msg.message==WM_QUIT )
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Time
		if ( Time()==false )
			continue;

		// Update
		Update();

		// Render
		Render();
	}

	// Thread
	for ( int i=0 ; i<m_threadCount ; i++ )
		m_threads[i].m_quitRequest = true;
	for ( int i=0 ; i<m_threadCount ; i++ )
		SetEvent(m_threads[i].m_hEventStart);
	for ( int i=0 ; i<m_threadCount ; i++ )
		m_threads[i].Wait();

	// End
	OnExit();
}

void Engine::FixWindow()
{
	RECT rc;
	GetClientRect(m_hWnd, &rc);
	m_windowWidth = rc.right-rc.left;
	m_windowHeight = rc.bottom-rc.top;

#ifdef CONFIG_GPU
	if ( m_pRenderTarget )
		m_pRenderTarget->Resize(D2D1::SizeU(m_windowWidth, m_windowHeight));
#endif
}

void Engine::FixProjection()
{
	const float ratio = float(m_windowWidth) / float(m_windowHeight);
	m_camera.UpdateProjection(ratio);
}

void Engine::FixDevice()
{
#ifdef CONFIG_GPU
	D2D1_SIZE_U size = D2D1::SizeU(m_windowWidth, m_windowHeight);
	m_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(m_hWnd, size), &m_pRenderTarget);
	
	D2D1_SIZE_U renderSize = D2D1::SizeU(m_renderWidth, m_renderHeight);
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

ENTITY* Engine::CreateEntity()
{
	ENTITY* pEntity = new ENTITY;
	if ( m_bornEntityCount<m_bornEntities.size() )
		m_bornEntities[m_bornEntityCount] = pEntity;
	else
		m_bornEntities.push_back(pEntity);
	m_bornEntityCount++;
	return pEntity;
}

SPRITE* Engine::CreateSprite()
{
	SPRITE* pSprite = new SPRITE;
	if ( m_bornSpriteCount<m_bornSprites.size() )
		m_bornSprites[m_bornSpriteCount] = pSprite;
	else
		m_bornSprites.push_back(pSprite);
	m_bornSpriteCount++;
	return pSprite;
}

void Engine::ReleaseEntity(ENTITY* pEntity)
{
	if ( pEntity==nullptr || pEntity->dead )
		return;

	pEntity->dead = true;
	if ( m_deadEntityCount<m_deadEntities.size() )
		m_deadEntities[m_deadEntityCount] = pEntity;
	else
		m_deadEntities.push_back(pEntity);
	m_deadEntityCount++;
}

void Engine::ReleaseSprite(SPRITE* pSprite)
{
	if ( pSprite==nullptr || pSprite->dead )
		return;

	pSprite->dead = true;
	if ( m_deadSpriteCount<m_deadSprites.size() )
		m_deadSprites[m_deadSpriteCount] = pSprite;
	else
		m_deadSprites.push_back(pSprite);
	m_deadSpriteCount++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::GetCursor(XMFLOAT2& pos)
{
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(m_hWnd, &pt);
	pos.x = (float)pt.x;
	pos.y = (float)pt.y;
	pos.x = pos.x*m_renderWidth/m_windowWidth;
	pos.y = pos.y*m_renderHeight/m_windowHeight;
}

RAY Engine::GetCameraRay(XMFLOAT2& pt)
{
	float px = pt.x*2.0f/m_renderWidth - 1.0f;
	float py = pt.y*2.0f/m_renderHeight - 1.0f;

	float x = px/m_camera.matProj._11;
	float y = -py/m_camera.matProj._22;
	float z = 1.0f;

	RAY ray;
	ray.pos.x = m_camera.transform.world._41;
	ray.pos.y = m_camera.transform.world._42;
	ray.pos.z = m_camera.transform.world._43;
	ray.dir.x = x*m_camera.transform.world._11 + y*m_camera.transform.world._21 + z*m_camera.transform.world._31;
	ray.dir.y = x*m_camera.transform.world._12 + y*m_camera.transform.world._22 + z*m_camera.transform.world._32;
	ray.dir.z = x*m_camera.transform.world._13 + y*m_camera.transform.world._23 + z*m_camera.transform.world._33;
	XMStoreFloat3(&ray.dir, XMVector3Normalize(XMLoadFloat3(&ray.dir)));
	return ray;
}

CAMERA* Engine::GetCamera()
{
	return &m_camera;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::DrawText(FONT* pFont, const char* text, int x, int y, int align)
{
	if ( pFont==nullptr || pFont->rgba.size()==0 || text==nullptr )
		return;

	const int cw = pFont->advance;
	const int ch = pFont->cellH;
	const char* p = text;
	const char* lineStart = text;
	int lineIndex = 0;

	auto DrawLine = [&](const char* start, int len, int penY)
	{
		int penX = x;
		if ( align==TEXT_CENTER ) penX = x - (len * cw) / 2;
		else if ( align==TEXT_RIGHT ) penX = x - (len * cw);
		for ( int i=0 ; i<len ; ++i )
		{
			const byte c = (byte)start[i];
			const GLYPH& g = pFont->glyph[c];
			if ( g.valid==false )
			{
				penX += cw;
				continue;
			}
			Copy((byte*)m_colorBuffer.data(), m_renderWidth, m_renderHeight, penX, penY, pFont->rgba.data(), pFont->width, pFont->height, g.x, g.y, g.w, g.h);
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

void Engine::DrawSprite(SPRITE* pSprite)
{
	if ( pSprite==nullptr || pSprite->dead || pSprite->pTexture==nullptr )
		return;

	int width = pSprite->pTexture->width;
	int height = pSprite->pTexture->height;
	byte* dst = (byte*)m_colorBuffer.data();
	Copy(dst, m_renderWidth, m_renderHeight, pSprite->x, pSprite->y, pSprite->pTexture->rgba, width, height, 0, 0, width, height);
}

void Engine::DrawHorzLine(int x1, int x2, int y, XMFLOAT3& color)
{
	COLORREF bgr = ToBGR(color);
	if ( y<0 || y>=m_renderHeight )
		return;
	x1 = Clamp(x1, 0, m_renderWidth);
	x2 = Clamp(x2, 0, m_renderWidth);
	ui32* buf = m_colorBuffer.data() + y*m_renderWidth;
	for ( int i=x1 ; i<x2 ; i++ )
		buf[i] = bgr;
}

void Engine::DrawVertLine(int y1, int y2, int x, XMFLOAT3& color)
{
	COLORREF bgr = ToBGR(color);
	if ( x<0 || x>=m_renderWidth )
		return;
	y1 = Clamp(y1, 0, m_renderHeight);
	y2 = Clamp(y2, 0, m_renderHeight);
	ui32* buf = m_colorBuffer.data() + y1*m_renderWidth + x;
	for ( int i=y1 ; i<y2 ; i++ )
	{
		*buf = bgr;
		buf += m_renderWidth;
	}
}

void Engine::DrawRectangle(int x, int y, int w, int h, XMFLOAT3& color)
{
	DrawHorzLine(x, x+w, y, color);
	DrawHorzLine(x, x+w, y+h, color);
	DrawVertLine(y, y+h, x, color);
	DrawVertLine(y, y+h, x+w, color);
}

void Engine::DrawLine(int x0, int y0, float z0, int x1, int y1, float z1, XMFLOAT3& color)
{
	ui32 bgr = ToBGR(color);

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
		if ( x0>=0 && x0<m_renderWidth && y0>=0 && y0<m_renderHeight )
		{
			int index = y0 * m_renderWidth + x0;
			if ( currentZ<m_depthBuffer[index] )
			{
				m_colorBuffer[index] = bgr;
				m_depthBuffer[index] = currentZ;
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

LRESULT Engine::OnWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch ( message )
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		FixWindow();
		FixProjection();
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Engine::Time()
{
	DWORD curtime = timeGetTime();
	DWORD deltatime = curtime - m_systime;
	if ( deltatime<5 )
		return false;

	m_systime = curtime;
	if ( deltatime>30 )
		deltatime = 30;
	m_dt = deltatime/1000.0f;
	m_time += m_dt;

	if ( m_systime-m_fpsTime>=1000 )
	{
		m_fps = m_fpsCount;
		m_fpsCount = 0;
		m_fpsTime = m_systime;
	}
	else
		m_fpsCount++;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::Update()
{
	// Controller
	m_input.Update();

	// Physics
	Update_Physics();

	// Callback
	OnUpdate();

	// Purge
	Update_Purge();
}

void Engine::Update_Physics()
{
	for ( int i=0 ; i<m_entityCount ; i++ )
	{
		ENTITY* pEntity = m_entities[i];
		if ( pEntity->dead )
			continue;

		pEntity->lifetime += m_dt;
	}
}

void Engine::Update_Purge()
{
	// Dead
	for ( int i=0 ; i<m_deadEntityCount ; i++ )
	{
		ENTITY* pEntity = m_deadEntities[i];
		if ( pEntity->index==-1 )
		{
			DELPTR(m_deadEntities[i]);
			continue;
		}
		if ( pEntity->index<m_entityCount-1 )
		{
			m_entities[pEntity->index] = m_entities[m_entityCount-1];
			m_entities[pEntity->index]->index = pEntity->index;
		}
		if ( pEntity->sortedIndex<m_entityCount-1 )
		{
			m_sortedEntities[pEntity->sortedIndex] = m_sortedEntities[m_entityCount-1];
			m_sortedEntities[pEntity->sortedIndex]->sortedIndex = pEntity->sortedIndex;
		}
		DELPTR(m_deadEntities[i]);
		m_entityCount--;
	}
	m_deadEntityCount = 0;

	// Born
	for ( int i=0 ; i<m_bornEntityCount ; i++ )
	{
		ENTITY* pEntity = m_bornEntities[i];
		if ( pEntity->dead )
		{
			DELPTR(m_bornEntities[i]);
			continue;
		}
		pEntity->index = m_entityCount;
		pEntity->sortedIndex = pEntity->index;
		if ( pEntity->index<m_entities.size() )
			m_entities[pEntity->index] = pEntity;
		else
			m_entities.push_back(pEntity);
		if ( pEntity->sortedIndex<m_sortedEntities.size() )
			m_sortedEntities[pEntity->sortedIndex] = pEntity;
		else
			m_sortedEntities.push_back(pEntity);
		m_entityCount++;
	}
	m_bornEntityCount = 0;

	// Dead
	for ( int i=0 ; i<m_deadSpriteCount ; i++ )
	{
		SPRITE* pSprite = m_deadSprites[i];
		if ( pSprite->index==-1 )
		{
			DELPTR(m_deadSprites[i]);
			continue;
		}
		if ( pSprite->index<m_spriteCount-1 )
		{
			m_sprites[pSprite->index] = m_sprites[m_spriteCount-1];
			m_sprites[pSprite->index]->index = pSprite->index;
		}
		if ( pSprite->sortedIndex<m_spriteCount-1 )
		{
			m_sortedSprites[pSprite->sortedIndex] = m_sortedSprites[m_spriteCount-1];
			m_sortedSprites[pSprite->sortedIndex]->sortedIndex = pSprite->sortedIndex;
		}
		DELPTR(m_deadSprites[i]);
		m_spriteCount--;
	}
	m_deadSpriteCount = 0;

	// Born
	for ( int i=0 ; i<m_bornSpriteCount ; i++ )
	{
		SPRITE* pSprite = m_bornSprites[i];
		if ( pSprite->dead )
		{
			DELPTR(m_bornSprites[i]);
			continue;
		}
		pSprite->index = m_spriteCount;
		pSprite->sortedIndex = pSprite->index;
		if ( pSprite->index<m_sprites.size() )
			m_sprites[pSprite->index] = pSprite;
		else
			m_sprites.push_back(pSprite);
		if ( pSprite->sortedIndex<m_sortedSprites.size() )
			m_sortedSprites[pSprite->sortedIndex] = pSprite;
		else
			m_sortedSprites.push_back(pSprite);
		m_spriteCount++;
	}
	m_bornSpriteCount = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::Render()
{
	// Prepare
	m_camera.Update();
	Render_SortZ();
	Render_RecalculateMatrices();
	Render_ApplyClipping();
	Render_PrepareTiles();

	// Background
	if ( m_sky )
		ClearSky();
	else
		Clear(m_clearColor);

	// Callback
	OnPreRender();

	// Entities
	m_statsDrawnTriangleCount = 0;
#ifdef CONFIG_MT_DEBUG
	for ( int i=0 ; i<m_threadCount ; i++ )
	{
		SetEvent(m_threads[i].m_hEventStart);
		WaitForSingleObject(m_threads[i].m_hEventEnd, INFINITE);
	}
#else
	for ( int i=0 ; i<m_threadCount ; i++ )
		SetEvent(m_threads[i].m_hEventStart);
	for ( int i=0 ; i<m_threadCount ; i++ )
		WaitForSingleObject(m_threads[i].m_hEventEnd, INFINITE);
#endif

	// Callback
	OnPostRender();

	// UI
	Render_UI();

	// Present
	Present();

	// MT
#ifdef CONFIG_MT_DEBUG
	Sleep(1000);
#endif
}

void Engine::Render_SortZ()
{
	// Entities
	std::sort(m_sortedEntities.begin(), m_sortedEntities.begin()+m_entityCount, [](const ENTITY* pA, const ENTITY* pB) { return pA->view.z < pB->view.z; });
	for ( int i=0 ; i<m_entityCount ; i++ )
		m_sortedEntities[i]->sortedIndex = i;

	// Sprites
	std::sort(m_sortedSprites.begin(), m_sortedSprites.begin()+m_spriteCount, [](const SPRITE* pA, const SPRITE* pB) { return pA->z < pB->z; });
	for ( int i=0 ; i<m_spriteCount ; i++ )
		m_sortedSprites[i]->sortedIndex = i;
}

void Engine::Render_RecalculateMatrices()
{
	float width = (float)m_renderWidth;
	float height = (float)m_renderHeight;

	for ( int iEntity=0 ; iEntity<m_entityCount ; iEntity++ )
	{
		ENTITY* pEntity = m_entities[iEntity];
		if ( pEntity->dead )
			continue;

		// World
		pEntity->transform.Update();
		XMMATRIX matWorld = XMLoadFloat4x4(&pEntity->transform.world);

		// OBB
		pEntity->obb = pEntity->pMesh->aabb;
		pEntity->obb.Transform(matWorld);

		// AABB
		pEntity->aabb = pEntity->obb;

		// Radius
		float scale = std::max(pEntity->transform.sca.x, std::max(pEntity->transform.sca.y, pEntity->transform.sca.z));
		pEntity->radius = pEntity->pMesh->radius *scale;

		// Rectangle (screen)
		XMMATRIX matWVP = matWorld;
		matWVP *= XMLoadFloat4x4(&m_camera.matViewProj);
		pEntity->pMesh->aabb.ToScreen(pEntity->box, matWVP, width, height);

		// View
		XMMATRIX matView = XMLoadFloat4x4(&m_camera.matView);
		XMVECTOR pos = XMLoadFloat3(&pEntity->transform.pos);
		pos = XMVector3TransformCoord(pos, matView);
		XMStoreFloat3(&pEntity->view, pos);
	}
}

void Engine::Render_ApplyClipping()
{
	m_statsClipEntityCount = 0;
	for ( int iEntity=0 ; iEntity<m_entityCount ; iEntity++ )
	{
		ENTITY* pEntity = m_entities[iEntity];
		if ( pEntity->dead )
			continue;

		if ( m_camera.frustum.Intersect(pEntity->transform.pos, pEntity->radius) )
		{
			pEntity->clipped = false;
		}
		else
		{
			pEntity->clipped = true;
			m_statsClipEntityCount++;
		}
	}
}

void Engine::Render_PrepareTiles()
{
	// Reset
	m_nextTile = 0;

	// Entities
	for ( int iEntity=0 ; iEntity<m_entityCount ; iEntity++ )
	{
		ENTITY* pEntity = m_entities[iEntity];
		if ( pEntity->dead )
			continue;

		int minX = Clamp(int(pEntity->box.min.x) / m_tileWidth, 0, m_tileColCount-1);
		int maxX = Clamp(int(pEntity->box.max.x) / m_tileWidth, 0, m_tileColCount-1);
		int minY = Clamp(int(pEntity->box.min.y) / m_tileHeight, 0, m_tileRowCount-1);
		int maxY = Clamp(int(pEntity->box.max.y) / m_tileHeight, 0, m_tileRowCount-1);

		pEntity->tile = 0;
		for ( int y=minY ; y<=maxY ; y++ )
		{
			for ( int x=minX ; x<=maxX ; x++ )
			{
				int index = y*m_tileColCount + x;
				pEntity->tile |= 1 << index;
			}
		}
	}
}

void Engine::Render_Tile(int iTile)
{
	TILE& tile = m_tiles[iTile];
	for ( int iEntity=0 ; iEntity<m_entityCount ; iEntity++ )
	{
		ENTITY* pEntity = m_sortedEntities[iEntity];
		if ( pEntity->dead || pEntity->pMesh==nullptr || pEntity->clipped )
			continue;
	
		bool entityHasTile = (pEntity->tile>>tile.index) & 1 ? true : false;
		if ( entityHasTile==false )
			continue;

		DrawEntity(pEntity, tile);
	}

#ifdef CONFIG_MT_DEBUG
	DrawRectangle(tile.left, tile.top, tile.right-tile.left, tile.bottom-tile.top, WHITE);
#endif
}

void Engine::Render_UI()
{
	for ( int iSprite=0 ; iSprite<m_spriteCount ; iSprite++ )
	{
		SPRITE* pSprite = m_sortedSprites[iSprite];
		if ( pSprite->dead )
			continue;

		DrawSprite(pSprite);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::Clear(XMFLOAT3& color)
{
	ui32 bgr = ToBGR(color);
	std::fill(m_colorBuffer.begin(), m_colorBuffer.end(), bgr);
	std::fill(m_depthBuffer.begin(), m_depthBuffer.end(), 1.0f);
}

void Engine::ClearSky()
{
	ui32 gCol = ToBGR(m_groundColor);
	ui32 sCol = ToBGR(m_skyColor);

	XMMATRIX matView = XMLoadFloat4x4(&m_camera.matView);
	XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR viewUp = XMVector3TransformNormal(worldUp, matView);

	float nx = XMVectorGetX(viewUp);
	float ny = XMVectorGetY(viewUp);
	float nz = XMVectorGetZ(viewUp);

	float p11 = m_camera.matProj._11; // Cotan(FovX/2) / Aspect
	float p22 = m_camera.matProj._22; // Cotan(FovY/2)

	float a = nx / (p11 * m_renderWidthHalf);
	float b = -ny / (p22 * m_renderHeightHalf); // Le signe '-' compense l'axe Y inversé de l'écran
	float c = nz - (nx / p11) + (ny / p22);

	bool rightSideIsSky = a > 0;
	
	uint32_t colLeft  = rightSideIsSky ? gCol : sCol;
	uint32_t colRight = rightSideIsSky ? sCol : gCol;

	std::fill(m_depthBuffer.begin(), m_depthBuffer.end(), 1.0f);
	if ( fabsf(a)<0.000001f )
	{
		for ( int y=0 ; y<m_renderHeight ; y++ )
		{
			float val = b * (float)y + c;
			uint32_t col = val>0.0f ? sCol : gCol;
			std::fill(m_colorBuffer.begin() + (y * m_renderWidth),  m_colorBuffer.begin() + ((y + 1) * m_renderWidth),  col);
		}
	}
	else
	{
		float invA = -1.0f / a;
		for ( int y=0 ; y<m_renderHeight ; y++ )
		{
			float fSplitX = (b * (float)y + c) * invA;
			int splitX;
			if ( fSplitX<0.0f )
				splitX = 0;
			else if ( fSplitX>(float)m_renderWidth)
				splitX = m_renderWidth;
			else
				splitX = (int)fSplitX;

			uint32_t* rowPtr = m_colorBuffer.data() + (y * m_renderWidth);
			if ( splitX>0 )
				std::fill(rowPtr, rowPtr + splitX, colLeft);
			if ( splitX<m_renderWidth )
				std::fill(rowPtr + splitX, rowPtr + m_renderWidth, colRight);
		}
	}

	// Sky Line
	////////////

	float bandPx = m_renderHeight/10.0f;
	if ( bandPx<=0.5f )
		return;

	float grad = sqrtf(a*a + b*b);
	if ( grad<1e-8f )
		return;

	const float invGrad = 1.0f / grad;
	const float invBand = 1.0f / bandPx;
	const float limit   = bandPx * grad;

	if ( fabsf(a)<1e-6f )
	{
		for ( int y=0 ; y<m_renderHeight ; ++y )
		{
			float distPx = (b * (float)y + c) * invGrad;
			if ( fabsf(distPx)>bandPx )
				continue;
			float t = Clamp(0.5f + distPx * invBand);
			t = t * t * (3.0f - 2.0f * t); // optional
			uint32_t col = LerpBGR(gCol, sCol, t);
			uint32_t* row = m_colorBuffer.data() + y * m_renderWidth;
			std::fill(row, row + m_renderWidth, col);
		}
		return;
	}

	for ( int y=0 ; y<m_renderHeight ; ++y )
	{
		uint32_t* row = m_colorBuffer.data() + y * m_renderWidth;
		float byc = b * (float)y + c;
		float xA = (-limit - byc) / a;
		float xB = ( +limit - byc) / a;
		float xMinF = (xA < xB) ? xA : xB;
		float xMaxF = (xA > xB) ? xA : xB;
		int x0 = FloorToInt(xMinF);
		int x1 = CeilToInt(xMaxF);
		if ( x1<0 || x0>=m_renderWidth ) continue;
		if ( x0<0 ) x0 = 0;
		if ( x1>=m_renderWidth) x1 = m_renderWidth - 1;
		float val = byc + a * (float)x0;
		uint32_t* p = row + x0;
		uint32_t* e = row + x1 + 1;
		while ( p!=e )
		{
			float distPx = val * invGrad;
			float t = Clamp(0.5f + distPx * invBand);
			t = t * t * (3.0f - 2.0f * t); // optional
			*p++ = LerpBGR(gCol, sCol, t);
			val += a;
		}
	}
}

void Engine::DrawEntity(ENTITY* pEntity, TILE& tile)
{
	MATERIAL& material = pEntity->pMaterial ? *pEntity->pMaterial : m_defaultMaterial;
	XMMATRIX matWorld = XMLoadFloat4x4(&pEntity->transform.world);
	XMMATRIX normalMat = XMMatrixTranspose(XMMatrixInverse(nullptr, matWorld));
	XMMATRIX matViewProj = XMLoadFloat4x4(&m_camera.matViewProj);
	XMVECTOR lightDir = XMLoadFloat3(&m_lightDir);

	DRAWCALL dc;
	bool safe;
	XMFLOAT3 screen[3];
	VS out[3];

	for ( const TRIANGLE& triangle : pEntity->pMesh->triangles )
	{
		// Verter shader
		safe = true;
		for ( int i=0 ; i<3 ; ++i )
		{
			// Vertex
			const VERTEX& in = triangle.v[i];

			// World pos
			XMVECTOR loc = XMLoadFloat3(&in.pos);
			loc = XMVectorSetW(loc, 1.0f);
			XMVECTOR world = XMVector4Transform(loc, matWorld);
			XMStoreFloat3(&out[i].worldPos, world);

			// Clip pos
			XMVECTOR clip = XMVector4Transform(world, matViewProj);
			XMStoreFloat4(&out[i].clipPos, clip);
			if ( out[i].clipPos.w<=0.0f )
			{
				safe = false;
				break;
			}
			out[i].invW = 1.0f / out[i].clipPos.w;

			// World normal
			XMVECTOR localNormal = XMLoadFloat3(&in.normal);
			XMVECTOR worldNormal = XMVector3TransformNormal(localNormal, normalMat);
			worldNormal = XMVector3Normalize(worldNormal);
			XMStoreFloat3(&out[i].worldNormal, worldNormal);

			// Albedo
			out[i].albedo.x = Clamp(in.color.x * material.color.x);
			out[i].albedo.y = Clamp(in.color.y * material.color.y);
			out[i].albedo.z = Clamp(in.color.z * material.color.z);

			// Intensity
			float ndotl = XMVectorGetX(XMVector3Dot(worldNormal, lightDir));
			ndotl = std::max(0.0f, ndotl);
			out[i].intensity = ndotl + m_ambient;

			// Screen pos
			float ndcX = out[i].clipPos.x * out[i].invW;			// [-1,1]
			float ndcY = out[i].clipPos.y * out[i].invW;			// [-1,1]
			float ndcZ = out[i].clipPos.z * out[i].invW;			// [0,1] avec XMMatrixPerspectiveFovLH
			screen[i].x = (ndcX + 1.0f) * m_renderWidthHalf;
			screen[i].y = (1.0f - ndcY) * m_renderHeightHalf;
			screen[i].z = Clamp(ndcZ);								// profondeur normalisée 0..1
		}
		if ( safe==false )
			continue;

		// Culling (DX)
		//float area = (screen[2].x-screen[0].x) * (screen[1].y-screen[0].y) - (screen[2].y-screen[0].y) * (screen[1].x-screen[0].x);
		float area = (screen[1].x-screen[0].x) * (screen[2].y-screen[0].y) - (screen[1].y-screen[0].y) * (screen[2].x-screen[0].x);
		if ( area<=FLT_EPSILON )
			continue;

		// Pixel shader
		dc.tri = screen;
		dc.vso = out;
		dc.pMaterial = &material;
		dc.pTile = &tile;
		dc.depth = pEntity->depth;
		FillTriangle(dc);
		m_statsDrawnTriangleCount++;

		// Wireframe
#ifdef CONFIG_WIREFRAME
		DrawLine((int)screen[0].x, (int)screen[0].y, screen[0].z, (int)screen[1].x, (int)screen[1].y, screen[1].z, WHITE);
		DrawLine((int)screen[1].x, (int)screen[1].y, screen[1].z, (int)screen[2].x, (int)screen[2].y, screen[2].z, WHITE);
		DrawLine((int)screen[2].x, (int)screen[2].y, screen[2].z, (int)screen[0].x, (int)screen[0].y, screen[0].z, WHITE);
#endif
	}
}

void Engine::FillTriangle(DRAWCALL& dc)
{
	const float x1 = dc.tri[0].x, y1 = dc.tri[0].y, z1 = dc.tri[0].z;
	const float x2 = dc.tri[1].x, y2 = dc.tri[1].y, z2 = dc.tri[1].z;
	const float x3 = dc.tri[2].x, y3 = dc.tri[2].y, z3 = dc.tri[2].z;

	int minX = (int)floor(std::min(std::min(x1, x2), x3));
	int maxX = (int)ceil(std::max(std::max(x1, x2), x3));
	int minY = (int)floor(std::min(std::min(y1, y2), y3));
	int maxY = (int)ceil(std::max(std::max(y1, y2), y3));

	if ( minX<0 ) minX = 0;
	if ( minY<0 ) minY = 0;
	if ( maxX>m_renderWidth ) maxX = m_renderWidth;
	if ( maxY>m_renderHeight ) maxY = m_renderHeight;

	if ( minX<dc.pTile->left ) minX = dc.pTile->left;
	if ( maxX>dc.pTile->right ) maxX = dc.pTile->right;
	if ( minY<dc.pTile->top ) minY = dc.pTile->top;
	if ( maxY>dc.pTile->bottom ) maxY = dc.pTile->bottom;

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
	if ( fabsf(area)<1e-6f )
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

	const PS_FUNC func = dc.pMaterial->ps ? dc.pMaterial->ps : &PixelShader;
	PS_DATA data;
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
			int index = y * m_renderWidth + x;
			if ( (dc.depth & DEPTH_READ) && z>=m_depthBuffer[index] )
			{
				e12 += dE12dx;
				e23 += dE23dx;
				e31 += dE31dx;
				continue;
			}

			// Input
			data.in.x = x;
			data.in.y = y;
			data.in.depth = z;

			// Position (lerp)
			data.in.pos.x = dc.vso[2].worldPos.x + (dc.vso[0].worldPos.x - dc.vso[2].worldPos.x) * w1 + (dc.vso[1].worldPos.x - dc.vso[2].worldPos.x) * w2;
			data.in.pos.y = dc.vso[2].worldPos.y + (dc.vso[0].worldPos.y - dc.vso[2].worldPos.y) * w1 + (dc.vso[1].worldPos.y - dc.vso[2].worldPos.y) * w2;
			data.in.pos.z = dc.vso[2].worldPos.z + (dc.vso[0].worldPos.z - dc.vso[2].worldPos.z) * w1 + (dc.vso[1].worldPos.z - dc.vso[2].worldPos.z) * w2;

			// Normal (lerp)
			data.in.normal.x = dc.vso[2].worldNormal.x + (dc.vso[0].worldNormal.x - dc.vso[2].worldNormal.x) * w1 + (dc.vso[1].worldNormal.x - dc.vso[2].worldNormal.x) * w2;
			data.in.normal.y = dc.vso[2].worldNormal.y + (dc.vso[0].worldNormal.y - dc.vso[2].worldNormal.y) * w1 + (dc.vso[1].worldNormal.y - dc.vso[2].worldNormal.y) * w2;
			data.in.normal.z = dc.vso[2].worldNormal.z + (dc.vso[0].worldNormal.z - dc.vso[2].worldNormal.z) * w1 + (dc.vso[1].worldNormal.z - dc.vso[2].worldNormal.z) * w2;

			// Color (lerp)
			data.in.albedo.x = dc.vso[2].albedo.x + (dc.vso[0].albedo.x - dc.vso[2].albedo.x) * w1 + (dc.vso[1].albedo.x - dc.vso[2].albedo.x) * w2;
			data.in.albedo.y = dc.vso[2].albedo.y + (dc.vso[0].albedo.y - dc.vso[2].albedo.y) * w1 + (dc.vso[1].albedo.y - dc.vso[2].albedo.y) * w2;
			data.in.albedo.z = dc.vso[2].albedo.z + (dc.vso[0].albedo.z - dc.vso[2].albedo.z) * w1 + (dc.vso[1].albedo.z - dc.vso[2].albedo.z) * w2;

			// Lighting
			if ( dc.pMaterial->lighting==GOURAUD )
			{
				float intensity = dc.vso[2].intensity + (dc.vso[0].intensity - dc.vso[2].intensity) * w1 + (dc.vso[1].intensity - dc.vso[2].intensity) * w2;
				data.in.color.x = data.in.albedo.x * intensity;
				data.in.color.y = data.in.albedo.y * intensity;
				data.in.color.z = data.in.albedo.z * intensity;
			}
			else if ( dc.pMaterial->lighting==LAMBERT )
			{
				XMVECTOR n = XMLoadFloat3(&data.in.normal);				
				//n = XMVector3Normalize(n); // Expensive (better results)
				XMVECTOR l = XMLoadFloat3(&Engine::Instance()->m_lightDir);
				float ndotl = XMVectorGetX(XMVector3Dot(n, l));
				if ( ndotl<0.0f )
					ndotl = 0.0f;
				float intensity = ndotl + Engine::Instance()->m_ambient;
				data.in.color.x = data.in.albedo.x * intensity;
				data.in.color.y = data.in.albedo.y * intensity;
				data.in.color.z = data.in.albedo.z * intensity;
			}
			else
				data.in.color = data.in.albedo;

			// Output
			data.values = dc.pMaterial->values;
			data.out = {};
			data.discard = false;
			func(data);
			if ( data.discard==false )
			{
				if ( dc.depth & DEPTH_WRITE )
					m_depthBuffer[index] = z;
				m_colorBuffer[index] = ToBGR(data.out);
			}

			e12 += dE12dx;
			e23 += dE23dx;
			e31 += dE31dx;
		}

		e12_row += dE12dy;
		e23_row += dE23dy;
		e31_row += dE31dy;
	}
}

bool Engine::Copy(byte* dst, int dstW, int dstH, int dstX, int dstY, const uint8_t* src, int srcW, int srcH, int srcX, int srcY, int w, int h)
{
	if ( w<=0 || h<=0 )
		return false;

	if ( dstX<0 ) { int d = -dstX; dstX = 0; srcX += d; w -= d; }
	if ( dstY<0 ) { int d = -dstY; dstY = 0; srcY += d; h -= d; }
	if ( dstX+w>dstW ) w = dstW - dstX;
	if ( dstY+h>dstH ) h = dstH - dstY;
	if ( srcX<0 ) { int d = -srcX; srcX = 0; dstX += d; w -= d; }
	if ( srcY<0 ) { int d = -srcY; srcY = 0; dstY += d; h -= d; }
	if ( srcX+w>srcW ) w = srcW - srcX;
	if ( srcY+h>srcH ) h = srcH - srcY;

	if ( w<=0 || h<=0 )
		return false;
	
	const int dstStride = dstW * 4;
	const int srcStride = srcW * 4;
	byte* drow = dst + dstY * dstStride + dstX * 4;
	const byte* srow = src + srcY * srcStride + srcX * 4;
	for ( int y=0 ; y<h ; ++y )
	{
		byte* d = drow;
		const byte* s = srow;
		for ( int x=0 ; x<w ; ++x )
		{
			const uint8_t sa = s[3];
			if ( sa==255 )
			{
				d[0] = s[2];
				d[1] = s[1];
				d[2] = s[0];
			}
			else if ( sa!=0 )
			{
				const byte a = sa;
				const byte ia = 255 - a;
				d[0] = (byte)((s[2] * a + d[0] * ia + 127) / 255);
				d[1] = (byte)((s[1] * a + d[1] * ia + 127) / 255);
				d[2] = (byte)((s[0] * a + d[2] * ia + 127) / 255);
			}
			d += 4;
			s += 4;
		}
		drow += dstStride;
		srow += srcStride;
	}

	return true;
}

void Engine::PixelShader(PS_DATA& data)
{
	data.out = data.in.color;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::Present()
{
#ifdef CONFIG_GPU
	if ( m_pRenderTarget==nullptr || m_pBitmap==nullptr )
		return;
	
	m_pRenderTarget->BeginDraw();
	m_pBitmap->CopyFromMemory(NULL, m_colorBuffer.data(), m_renderWidth * 4);
	D2D1_RECT_F destRect = D2D1::RectF(0.0f, 0.0f, (float)m_windowWidth, (float)m_windowHeight);
	if ( m_bilinear )
		m_pRenderTarget->DrawBitmap(m_pBitmap, destRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
	else
		m_pRenderTarget->DrawBitmap(m_pBitmap, destRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
	HRESULT hr = m_pRenderTarget->EndDraw();
	if ( hr==D2DERR_RECREATE_TARGET )
	{
		m_pBitmap->Release();
		m_pBitmap = nullptr;
		m_pRenderTarget->Release();
		m_pRenderTarget = nullptr;
		FixDevice();
	}
#else
	if ( m_windowWidth==m_renderWidth && m_windowHeight==m_renderHeight )
	{
		// Fast
		SetDIBitsToDevice(m_hDC, 0, 0, m_renderWidth, m_renderHeight, 0, 0, 0, m_renderHeight, m_colorBuffer.data(), &m_bi, DIB_RGB_COLORS);
	}
	else
	{
		// Slow
		StretchDIBits(m_hDC, 0, 0, m_windowWidth, m_windowHeight, 0, 0, m_renderWidth, m_renderHeight, m_colorBuffer.data(), &m_bi, DIB_RGB_COLORS, SRCCOPY);
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT Engine::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Engine* pEngine = (Engine*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if ( pEngine )
		return pEngine->OnWindowProc(hWnd, message, wParam, lParam);

	return DefWindowProc(hWnd, message, wParam, lParam);
}
