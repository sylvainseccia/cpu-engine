#include "stdafx.h"

Engine* Engine::s_pEngine;

Engine::Engine()
{
	s_pEngine = this;
	m_hInstance = nullptr;
	m_hWnd = nullptr;
	m_threads = nullptr;

#ifdef GPU_PRESENT
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

void Engine::Initialize(HINSTANCE hInstance, int renderWidth, int renderHeight, float windowScaleAtStart)
{
	if ( m_hInstance )
		return;

	// Window
	m_hInstance = hInstance;
	m_windowWidth = (int)(renderWidth*windowScaleAtStart);
	m_windowHeight = (int)(renderHeight*windowScaleAtStart);
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = "RETRO_ENGINE";
	RegisterClass(&wc);
	RECT rect = { 0, 0, m_windowWidth, m_windowHeight };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	m_hWnd = CreateWindow("RETRO_ENGINE", "RETRO ENGINE", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, rect.right-rect.left, rect.bottom-rect.top, nullptr, nullptr, hInstance, nullptr);
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
#ifdef GPU_PRESENT
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
	m_ambientLight = 0.2f;

	// Entity
	m_entityCount = 0;
	m_bornEntityCount = 0;
	m_deadEntityCount = 0;

	// Tile
	m_threadCount = std::max(1u, std::thread::hardware_concurrency());
#ifdef _DEBUG
	m_threadCount = 1;
#endif
	m_tileColCount = static_cast<unsigned int>(std::ceil(std::sqrt(m_threadCount)));
	m_tileRowCount = (m_threadCount + m_tileColCount - 1) / m_tileColCount;
	m_tileCount = m_tileColCount * m_tileRowCount;
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
#ifdef GPU_PRESENT
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

#ifdef GPU_PRESENT
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
#ifdef GPU_PRESENT
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

XMFLOAT3 Engine::ApplyLighting(XMFLOAT3& color, float intensity)
{
	intensity = Clamp(intensity);
	XMFLOAT3 trg;
	trg.x = color.x * intensity;
	trg.y = color.y * intensity;
	trg.z = color.z * intensity;
	return trg;
}

ui32 Engine::ToBGR(XMFLOAT3& color)
{
	float r = std::max(0.0f, std::min(1.0f, color.x));
	float g = std::max(0.0f, std::min(1.0f, color.y));
	float b = std::max(0.0f, std::min(1.0f, color.z));
	return RGB((int)(b * 255.0f), (int)(g * 255.0f), (int)(r * 255.0f));
}

XMFLOAT3 Engine::ToColor(int r, int g, int b)
{
	XMFLOAT3 color;
	color.x = r/255.0f;
	color.y = g/255.0f;
	color.z = b/255.0f;
	return color;
}

float Engine::Clamp(float v)
{
	if ( v<0.0f )
		return 0.0f;
	if ( v>1.0f )
		return 1.0f;
	return v;
}

float Engine::Clamp(float v, float min, float max)
{
	if ( v<min )
		return min;
	if ( v>max )
		return max;
	return v;
}

int Engine::Clamp(int v, int min, int max)
{
	if ( v<min )
		return min;
	if ( v>max )
		return max;
	return v;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::CreateSpaceship(MESH& mesh)
{
	const float width = 2.0f;
	XMFLOAT3 nose = { 0.0f, 0.0f, 1.5f };
	XMFLOAT3 rTop = { 0.0f, 0.5f, -1.0f };
	XMFLOAT3 rBot = { 0.0f, -0.3f, -1.0f };
	XMFLOAT3 wLeft = { -width*0.5f, 0.0f, -1.0f };
	XMFLOAT3 wRight = { width*0.5f, 0.0f, -1.0f };

	XMFLOAT3 c1 = ToColor(208, 208, 208);
	XMFLOAT3 c2 = ToColor(192, 192, 192);
	XMFLOAT3 c3 = ToColor(112, 112, 112);
	XMFLOAT3 c4 = ToColor(96, 96, 96);
	XMFLOAT3 c5 = ToColor(255, 255, 255);
	XMFLOAT3 c6 = ToColor(255, 255, 255);

	mesh.AddTriangle(nose, rTop, wLeft, c1);				// Avant Gauche haut
	mesh.AddTriangle(nose, wRight, rTop, c2);				// Avant Droit haut
	mesh.AddTriangle(nose, wLeft, rBot, c3);				// Avant Gauche bas
	mesh.AddTriangle(nose, rBot, wRight, c4);				// Avant Droit bas
	mesh.AddTriangle(wLeft, rTop, rBot, c5);				// Moteur Gauche
	mesh.AddTriangle(wRight, rBot, rTop, c6);				// Moteur Droit
	mesh.Optimize();
}

void Engine::CreateCube(MESH& mesh)
{
	const float s = 0.5f; 
	XMFLOAT3 p0 = { -s, -s, -s };					// Avant Bas Gauche
	XMFLOAT3 p1 = {  s, -s, -s };					// Avant Bas Droite
	XMFLOAT3 p2 = {  s,  s, -s };					// Avant Haut Droite
	XMFLOAT3 p3 = { -s,  s, -s };					// Avant Haut Gauche
	XMFLOAT3 p4 = { -s, -s,  s };					// Arrière Bas Gauche
	XMFLOAT3 p5 = {  s, -s,  s };					// Arrière Bas Droite
	XMFLOAT3 p6 = {  s,  s,  s };					// Arrière Haut Droite
	XMFLOAT3 p7 = { -s,  s,  s };					// Arrière Haut Gauche
	
	XMFLOAT3 c1 = ToColor(255, 255, 255);
	mesh.AddFace(p0, p1, p2, p3, c1);						// Face Avant (Z = -0.5)
	mesh.AddFace(p5, p4, p7, p6, c1);						// Face Arrière (Z = +0.5)
	mesh.AddFace(p1, p5, p6, p2, c1);						// Face Droite (X = +0.5)
	mesh.AddFace(p4, p0, p3, p7, c1);						// Face Gauche (X = -0.5)
	mesh.AddFace(p3, p2, p6, p7, c1);						// Face Haut (Y = +0.5)
	mesh.AddFace(p4, p5, p1, p0, c1);						// Face Bas (Y = -0.5)
	mesh.Optimize();
}

void Engine::CreateCircle(MESH& mesh, float radius, int count)
{
	if ( count<3 )
		return;

	XMFLOAT3 c1 = ToColor(255, 255, 255);
	float step = XM_2PI / count;
	float angle = 0.0f;
	XMFLOAT3 p1, p2, p3;
	p1.x = 0.0f;
	p1.y = 0.0f;
	p1.z = 0.0f;
	p2.y = 0.0f;
	p3.y = 0.0f;
	for ( int i=0 ; i<count ; i++ )
	{
		p2.x = cosf(angle) * radius;
		p2.z = sinf(angle) * radius;
		p3.x = cosf(angle+step) * radius;
		p3.z = sinf(angle+step) * radius;
		mesh.AddTriangle(p1, p2, p3, c1);
		angle += step;
	}
	mesh.Optimize();
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

void Engine::Update()
{
	// Controller
	m_keyboard.Update();

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
			delete pEntity;
			m_deadEntities[i] = nullptr;
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

		delete pEntity;
		m_deadEntities[i] = nullptr;
		m_entityCount--;
	}
	m_deadEntityCount = 0;

	// Born
	for ( int i=0 ; i<m_bornEntityCount ; i++ )
	{
		ENTITY* pEntity = m_bornEntities[i];
		if ( pEntity->dead )
		{
			delete pEntity;
			m_bornEntities[i] = nullptr;
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
}

void Engine::Render()
{
	// Stats
	m_clipEntityCount = 0;

	// Prepare
	m_camera.Update();
	Render_Sort();
	Render_Box();
	Render_Tile();

	// Background
	if ( m_sky )
		DrawSky();
	else
		Clear(m_clearColor);

	// Callback
	OnPreRender();

	// Entities (Multi-Threading)
	for ( int i=0 ; i<m_threadCount ; i++ )
		SetEvent(m_threads[i].m_hEventStart);
	for ( int i=0 ; i<m_threadCount ; i++ )
		WaitForSingleObject(m_threads[i].m_hEventEnd, INFINITE);

	// Callback
	OnPostRender();

	// Present
	Present();
}

void Engine::Render_Sort()
{
	std::sort(m_sortedEntities.begin(), m_sortedEntities.begin()+m_entityCount, [](const ENTITY* pA, const ENTITY* pB) { return pA->view.z < pB->view.z; });
	for ( int i=0 ; i<m_entityCount ; i++ )
		m_sortedEntities[i]->sortedIndex = i;
}

void Engine::Render_Box()
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

void Engine::Render_Tile()
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

void Engine::Render_Entity(int iTile)
{
	TILE& tile = m_tiles[iTile];
	for ( int iEntity=0 ; iEntity<m_entityCount ; iEntity++ )
	{
		ENTITY* pEntity = m_sortedEntities[iEntity];
		if ( pEntity->dead )
			continue;
	
		bool entityHasTile = (pEntity->tile>>tile.index) & 1 ? true : false;
		if ( entityHasTile==false )
			continue;

		Draw(pEntity, tile);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::Present()
{
#ifdef GPU_PRESENT
	if ( m_pRenderTarget==nullptr || m_pBitmap==nullptr )
		return;
	
	m_pRenderTarget->BeginDraw();
	m_pBitmap->CopyFromMemory(NULL, m_colorBuffer.data(), m_renderWidth * 4);
	D2D1_RECT_F destRect = D2D1::RectF(0.0f, 0.0f, (float)m_windowWidth, (float)m_windowHeight);
	m_pRenderTarget->DrawBitmap(m_pBitmap, destRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
	//m_pRenderTarget->DrawBitmap(m_pBitmap, destRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
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
		SetDIBitsToDevice(m_hDC, 0, 0, m_renderWidth, m_renderHeight, 0, 0, 0, m_renderHeight, m_colorBuffer.data(), &m_bi, DIB_RGB_COLORS);
	else
		StretchDIBits(m_hDC, 0, 0, m_windowWidth, m_windowHeight, 0, 0, m_renderWidth, m_renderHeight, m_colorBuffer.data(), &m_bi, DIB_RGB_COLORS, SRCCOPY);
#endif
}

void Engine::Clear(XMFLOAT3& color)
{
	ui32 bgr = ToBGR(color);
	std::fill(m_colorBuffer.begin(), m_colorBuffer.end(), bgr);
	std::fill(m_depthBuffer.begin(), m_depthBuffer.end(), 1.0f);
}

void Engine::DrawSky()
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
	if ( std::abs(a)<0.000001f )
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
}

void Engine::Draw(ENTITY* pEntity, TILE& tile)
{
	if ( pEntity==nullptr || pEntity->pMesh==nullptr )
		return;

	// Clipping
	if ( m_camera.frustum.Intersect(pEntity->transform.pos, pEntity->radius)==false )
	{
		m_clipEntityCount++;
		return;
	}

	// Info
	XMMATRIX matWorld = XMLoadFloat4x4(&pEntity->transform.world);
	XMMATRIX matView = XMLoadFloat4x4(&m_camera.matView);
	XMMATRIX matViewProj = XMLoadFloat4x4(&m_camera.matViewProj);
	XMVECTOR lightDir = XMLoadFloat3(&m_lightDir);

	// Vertex and Pixel shaders
	for ( const TRIANGLE& t : pEntity->pMesh->triangles )
	{
		// World space
		XMVECTOR vWorld[3];
		for ( int i=0 ; i<3 ; i++ )
		{
			XMVECTOR loc = XMLoadFloat3(&t.v[i].pos);
			loc = XMVectorSetW(loc, 1.0f);
			vWorld[i] = XMVector3Transform(loc, matWorld);
		}

		// Screen space
		XMFLOAT3 vScreen[3];
		bool safe = true;
		for ( int i=0 ; i<3 ; i++ )
		{
			XMVECTOR clip = XMVector3Transform(vWorld[i], matViewProj);
			XMFLOAT4 out;
			XMStoreFloat4(&out, clip);
			if ( out.w<=0.0f )
			{
				safe = false;
				break;
			}
			float invW = 1.0f / out.w;
			float ndcX = out.x * invW;   // [-1,1]
			float ndcY = out.y * invW;   // [-1,1]
			float ndcZ = out.z * invW;   // [0,1] avec XMMatrixPerspectiveFovLH
			ndcZ = Clamp(ndcZ);
			vScreen[i].x = (ndcX + 1.0f) * m_renderWidthHalf;
			vScreen[i].y = (1.0f - ndcY) * m_renderHeightHalf;
			vScreen[i].z = ndcZ;         // profondeur normalisée 0..1
		}
		if ( safe==false )
			continue;

		// Culling
		float area = (vScreen[2].x - vScreen[0].x) * (vScreen[1].y - vScreen[0].y) - (vScreen[2].y - vScreen[0].y) * (vScreen[1].x - vScreen[0].x);
		if ( area<=0.0f )
			continue;

		// Color
		XMFLOAT3 vertColors[3];
		for ( int i=0 ; i<3 ; i++ )
		{
			XMVECTOR localNormal = XMLoadFloat3(&t.v[i].normal);
			XMVECTOR worldNormal = XMVector3TransformNormal(localNormal, matWorld);
			worldNormal = XMVector3Normalize(worldNormal);
			float dot = XMVectorGetX(XMVector3Dot(worldNormal, XMVectorNegate(lightDir)));
			float intensity = std::max(0.0f, dot) + m_ambientLight;
			XMFLOAT3 finalColor;
			finalColor.x = t.v[i].color.x * pEntity->material.x * intensity;
			finalColor.y = t.v[i].color.y * pEntity->material.y * intensity;
			finalColor.z = t.v[i].color.z * pEntity->material.z * intensity;
			vertColors[i] = finalColor;
		}

		// Draw
		FillTriangle(vScreen, vertColors, tile);
	}
}

void Engine::DrawLine(int x0, int y0, float z0, int x1, int y1, float z1, XMFLOAT3& color)
{
	ui32 bgr = ToBGR(color);

	int dx = std::abs(x1 - x0);
	int sx = x0 < x1 ? 1 : -1;
	int dy = -std::abs(y1 - y0);
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;

	float dist = (float)std::max(dx, std::abs(dy));
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

void Engine::FillTriangle(XMFLOAT3* tri, XMFLOAT3* colors, TILE& tile)
{
	const float x1 = tri[0].x, y1 = tri[0].y, z1 = tri[0].z;
	const float x2 = tri[1].x, y2 = tri[1].y, z2 = tri[1].z;
	const float x3 = tri[2].x, y3 = tri[2].y, z3 = tri[2].z;

	int minX = (int)floor(std::min(std::min(x1, x2), x3));
	int maxX = (int)ceil(std::max(std::max(x1, x2), x3));
	int minY = (int)floor(std::min(std::min(y1, y2), y3));
	int maxY = (int)ceil(std::max(std::max(y1, y2), y3));

	if ( minX<0 )
		minX = 0;
	if ( minY<0 )
		minY = 0;
	if ( maxX>m_renderWidth )
		maxX = m_renderWidth;
	if ( maxY>m_renderHeight )
		maxY = m_renderHeight;

	if ( minX<tile.left )
		minX = tile.left;
	if ( maxX>tile.right )
		maxX = tile.right;
	if ( minY<tile.top )
		minY = tile.top;
	if ( maxY>tile.bottom )
		maxY = tile.bottom;

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
	if ( std::abs(area)<1e-6f )
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
			float z = z1 * w1 + z2 * w2 + z3 * w3;
			int index = y * m_renderWidth + x;
			if ( z>=m_depthBuffer[index] )
			{
				e12 += dE12dx;
				e23 += dE23dx;
				e31 += dE31dx;
				continue;
			}
			m_depthBuffer[index] = z;

			float r = Clamp(colors[0].x * w1 + colors[1].x * w2 + colors[2].x * w3);
			float g = Clamp(colors[0].y * w1 + colors[1].y * w2 + colors[2].y * w3);
			float b = Clamp(colors[0].z * w1 + colors[1].z * w2 + colors[2].z * w3);
			m_colorBuffer[index] = RGB((int)(b*255.0f), (int)(g*255.0f), (int)(r*255.0f));

			e12 += dE12dx;
			e23 += dE23dx;
			e31 += dE31dx;
		}

		e12_row += dE12dy;
		e23_row += dE23dy;
		e31_row += dE31dy;
	}
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
