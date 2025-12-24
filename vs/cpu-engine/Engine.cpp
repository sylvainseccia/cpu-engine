#include "stdafx.h"

cpu_engine::cpu_engine()
{
	s_pEngine = this;
	m_hInstance = nullptr;
	m_hWnd = nullptr;

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

	m_callback.onStart.Set(this, &cpu_engine::OnStart);
	m_callback.onUpdate.Set(this, &cpu_engine::OnUpdate);
	m_callback.onExit.Set(this, &cpu_engine::OnExit);
	m_callback.onRender.Set(this, &cpu_engine::OnRender);
}

cpu_engine::~cpu_engine()
{
	Free();
}

void cpu_engine::Free()
{
	if ( m_hInstance==nullptr )
		return;

	// Image
	cpu_img32::Free();
	
	// Jobs
	m_entityJobs.clear();
	m_particlePhysicsJobs.clear();
	m_particleSpaceJobs.clear();
	m_particleRenderJobs.clear();
	
	// Surface
#ifdef CONFIG_GPU
	CPU_RELEASE(m_pBitmap);
	CPU_RELEASE(m_pRenderTarget);
	CPU_RELEASE(m_pD2DFactory);
#else
	if ( m_hDC )
	{
		SelectObject(m_hDC, m_hBrush);
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool cpu_engine::Initialize(HINSTANCE hInstance, int renderWidth, int renderHeight, bool fullscreen, bool amigaStyle)
{
	if ( m_hInstance )
		return false;

	// Window
	m_hInstance = hInstance;
	m_windowWidth = renderWidth;
	m_windowHeight = renderHeight;
	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = "cpu-engine";
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	RegisterClass(&wc);
	MSG msg;
	while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) );
	if ( fullscreen )
	{
		RECT rect = { 0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN) };
		m_hWnd = CreateWindow("cpu-engine", "cpu-engine", WS_POPUP, 0, 0, rect.right-rect.left, rect.bottom-rect.top, nullptr, nullptr, hInstance, nullptr);
	}
	else
	{
		RECT rect = { 0, 0, m_windowWidth, m_windowHeight };
		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
		m_hWnd = CreateWindow("cpu-engine", "cpu-engine", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, rect.right-rect.left, rect.bottom-rect.top, nullptr, nullptr, hInstance, nullptr);
	}
	if ( m_hWnd==nullptr )
		return false;
	SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);

	// Buffer
	m_pRT = &m_mainRT;
	m_mainRT.width = renderWidth;
	m_mainRT.height = renderHeight;
	m_mainRT.aspectRatio = float(renderWidth)/float(renderHeight);
	m_mainRT.pixelCount = renderWidth * renderHeight;
	m_mainRT.widthHalf = renderWidth * 0.5f;
	m_mainRT.heightHalf = renderHeight * 0.5f;
	m_mainRT.colorBuffer.resize(m_mainRT.pixelCount);
	m_mainRT.depth = true;
	m_mainRT.depthBuffer.resize(m_mainRT.pixelCount);

	// Surface
#ifdef CONFIG_GPU
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
	if ( hr==nullptr )
		return false;
	FixDevice();
#else
	m_hDC = GetDC(m_hWnd);
	SetStretchBltMode(m_hDC, COLORONCOLOR);
	m_hBrush = (HBRUSH)SelectObject(m_hDC, GetStockObject(BLACK_BRUSH));
	m_bi.bmiHeader.biWidth = m_mainRT.width;
	m_bi.bmiHeader.biHeight = -m_mainRT.height;
#endif

	// Colors
	m_amigaStyle = amigaStyle;
	m_clear = CPU_CLEAR_SKY;
	m_clearColor = cpu::ToColor(32, 32, 64);
	m_groundColor = cpu::ToColor(32, 64, 32);
	m_skyColor = cpu::ToColor(32, 32, 64);

	// Light
	m_lightDir = { 0.5f, -1.0f, 0.5f };
	XMStoreFloat3(&m_lightDir, XMVector3Normalize(XMLoadFloat3(&m_lightDir)));
	m_ambient = 0.2f;

	// Cursor
	m_pCursor = nullptr;

	// Managers
	m_entityManager.Clear();
	m_spriteManager.Clear();
	m_particleManager.Clear();
	m_fsmManager.Clear();
	
	// Cores
#ifdef CONFIG_MT
	m_threadCount = std::max(1u, std::thread::hardware_concurrency());
#else
	m_threadCount = 1;
#endif

	// Tiles
	m_tileColCount = cpu::CeilToInt(sqrtf((float)m_threadCount));
	m_tileRowCount = (m_threadCount + m_tileColCount - 1) / m_tileColCount;
	m_tileCount = m_tileColCount * m_tileRowCount;
	m_stats.tileCount = m_tileCount;
	m_tileWidth = m_mainRT.width / m_tileColCount;
	m_tileHeight = m_mainRT.height / m_tileRowCount;
	int missingWidth = m_mainRT.width - (m_tileWidth*m_tileColCount);
	int missingHeight = m_mainRT.height - (m_tileHeight*m_tileRowCount);
	for ( int row=0 ; row<m_tileRowCount ; row++ )
	{
		for ( int col=0 ; col<m_tileColCount ; col++ )
		{
			cpu_tile tile;
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
			tile.particleLocalCounts.resize(m_tileCount);
			m_tiles.push_back(tile);
		}
	}

	// Threads
	m_stats.threadCount = m_threadCount;
	m_threads.resize(m_threadCount);
	for ( int i=0 ; i<m_threadCount ; i++ )
		m_threads[i].Create(m_tileCount);

	// Jobs
	m_entityJobs.resize(m_threadCount);
	m_particlePhysicsJobs.resize(m_threadCount);
	m_particleSpaceJobs.resize(m_threadCount);
	m_particleRenderJobs.resize(m_threadCount);
	for ( int i=0 ; i<m_threadCount ; i++ )
	{
		m_entityJobs[i].Create(&m_threads[i]);
		m_particlePhysicsJobs[i].Create(&m_threads[i]);
		m_particleSpaceJobs[i].Create(&m_threads[i]);
		m_particleRenderJobs[i].Create(&m_threads[i]);
	}

	// Window
	ShowWindow(m_hWnd, SW_SHOW);
	return true;
}

void cpu_engine::Run()
{
	// Camera
	m_camera.aspectRatio = m_mainRT.aspectRatio;
	m_camera.height = m_camera.width / m_camera.aspectRatio;
	m_camera.UpdateProjection();
	FixWindow();

	// Start
	m_systime = timeGetTime();
	m_totalTime = 0.0f;
	m_deltaTime = 0.0f;
	m_fpsTime = 0;
	m_fpsCount = 0;
	m_stats = {};
	m_callback.onStart.Call();

	// Threads
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

	// Threads
	for ( int i=0 ; i<m_threadCount ; i++ )
		m_threads[i].Stop();

	// End
	m_callback.onExit.Call();
}

void cpu_engine::Quit()
{
	PostQuitMessage(0);
}

void cpu_engine::FixWindow()
{
	if ( m_hWnd )
	{
		RECT rc;
		GetClientRect(m_hWnd, &rc);
		m_windowWidth = rc.right-rc.left;
		m_windowHeight = rc.bottom-rc.top;
		m_rcFit = cpu::ComputeAspectFitRect(m_mainRT.width, m_mainRT.height, m_windowWidth, m_windowHeight);
	}

#ifdef CONFIG_GPU
	if ( m_pRenderTarget )
		m_pRenderTarget->Resize(D2D1::SizeU(m_windowWidth, m_windowHeight));
#endif
}

void cpu_engine::FixDevice()
{
#ifdef CONFIG_GPU
	D2D1_SIZE_U size = D2D1::SizeU(m_windowWidth, m_windowHeight);
	m_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(m_hWnd, size), &m_pRenderTarget);
	
	cpu_rt& rt = *GetRT();
	D2D1_SIZE_U renderSize = D2D1::SizeU(rt.width, rt.height);
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

void cpu_engine::GetParticleRange(int& min, int& max, int iTile)
{
	int count = m_particleData.alive / m_tileCount;
	int remainder = m_particleData.alive % m_tileCount;
	min = iTile * count + std::min(iTile, remainder);
	max = min + count + (iTile<remainder ? 1 : 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_rt* cpu_engine::SetMainRT(bool copyDepth)
{
	if ( m_pRT==&m_mainRT )
		return m_pRT;

	cpu_rt* pOld = m_pRT;
	m_pRT = &m_mainRT;
	if ( copyDepth )
		CopyDepth(pOld);

	return pOld;
}

cpu_rt* cpu_engine::SetRT(cpu_rt* pRT, bool copyDepth)
{
	if ( pRT==m_pRT )
		return m_pRT;

	cpu_rt* pOld = m_pRT;
	m_pRT = pRT;
	if ( copyDepth )
		CopyDepth(pOld);

	return pOld;
}

void cpu_engine::CopyDepth(cpu_rt* pRT)
{
	if ( pRT==nullptr )
		return;

	cpu_rt& rt = *GetRT();
	if ( pRT->depth && rt.depth )
		rt.depthBuffer = pRT->depthBuffer;
}

void cpu_engine::AlphaBlend(cpu_rt* pRT)
{
	if ( pRT==nullptr )
		return;

	cpu_rt& rt = *GetRT();
	cpu_img32::AlphaBlend((byte*)pRT->colorBuffer.data(), pRT->width, pRT->height, (byte*)rt.colorBuffer.data(), rt.width, rt.height, 0, 0, 0, 0, rt.width, rt.height); 
}

void cpu_engine::ToAmigaStyle()
{
	cpu_rt& rt = *GetRT();
	cpu_img32::ToAmigaPalette((byte*)rt.colorBuffer.data(), rt.width, rt.height); 
}

void cpu_engine::Blur(int radius)
{
	if ( radius<1 )
		return;

	cpu_rt& rt = *GetRT();
	cpu_img32::Blur((byte*)rt.colorBuffer.data(), rt.width, rt.height, radius);
}

void cpu_engine::ClearColor()
{
	cpu_rt& rt = *GetRT();
	std::fill(rt.colorBuffer.begin(), rt.colorBuffer.end(), 0);
}

void cpu_engine::ClearColor(XMFLOAT3& rgb)
{
	cpu_rt& rt = *GetRT();
	std::fill(rt.colorBuffer.begin(), rt.colorBuffer.end(), cpu::ToBGR(rgb));
}

void cpu_engine::ClearDepth()
{
	cpu_rt& rt = *GetRT();
	std::fill(rt.depthBuffer.begin(), rt.depthBuffer.end(), 1.0f);
}

void cpu_engine::ClearSky()
{
	cpu_rt& rt = *GetRT();

	ui32 gCol = cpu::ToBGR(m_groundColor);
	ui32 sCol = cpu::ToBGR(m_skyColor);

	XMMATRIX matView = XMLoadFloat4x4(&m_camera.matView);
	XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR viewUp = XMVector3TransformNormal(worldUp, matView);

	float nx = XMVectorGetX(viewUp);
	float ny = XMVectorGetY(viewUp);
	float nz = XMVectorGetZ(viewUp);

	float p11 = m_camera.matProj._11; // Cotan(FovX/2) / Aspect
	float p22 = m_camera.matProj._22; // Cotan(FovY/2)

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
	if ( grad<1e-8f )
		return;

	const float invGrad = 1.0f / grad;
	const float invBand = 1.0f / bandPx;
	const float limit   = bandPx * grad;

	if ( fabsf(a)<1e-6f )
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

void cpu_engine::DrawMesh(cpu_mesh* pMesh, cpu_transform* pTransform, cpu_material* pMaterial, int depthMode, cpu_tile* pTile)
{
	cpu_rt& rt = *GetRT();
	cpu_material& material = pMaterial ? *pMaterial : m_defaultMaterial;
	XMMATRIX matWorld = XMLoadFloat4x4(&pTransform->GetWorld());
	XMMATRIX matNormal = XMMatrixTranspose(XMLoadFloat4x4(&pTransform->GetInvWorld()));
	XMMATRIX matViewProj = XMLoadFloat4x4(&m_camera.matViewProj);
	XMVECTOR lightDir = XMLoadFloat3(&m_lightDir);

	cpu_draw draw;
	bool safe;
	XMFLOAT3 screen[3];
	cpu_vertex_out vo[3];

	for ( const cpu_triangle& triangle : pMesh->triangles )
	{
		// Verter shader
		safe = true;
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
			if ( vo[i].clipPos.w<=0.0f )
			{
				safe = false;
				break;
			}
			vo[i].invW = 1.0f / vo[i].clipPos.w;

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
			vo[i].intensity = ndotl + m_ambient;

			// Screen pos
			float ndcX = vo[i].clipPos.x * vo[i].invW;			// [-1,1]
			float ndcY = vo[i].clipPos.y * vo[i].invW;			// [-1,1]
			float ndcZ = vo[i].clipPos.z * vo[i].invW;			// [0,1]
			//if ( ndcZ<0.0f || ndcZ>1.0f )
			//{
			//	safe = false;
			//	break;
			//}
			screen[i].x = (ndcX + 1.0f) * rt.widthHalf;
			screen[i].y = (1.0f - ndcY) * rt.heightHalf;
			screen[i].z = cpu::Clamp(ndcZ);							// profondeur normalisée 0..1
		}
		if ( safe==false )
			continue;

		// Culling
		float area = (screen[1].x-screen[0].x) * (screen[2].y-screen[0].y) - (screen[1].y-screen[0].y) * (screen[2].x-screen[0].x);
		bool isFront = m_cullFrontCCW ? (area>m_cullAreaEpsilon) : (area<-m_cullAreaEpsilon);
		if ( isFront==false )
			continue; // back-face or degenerated

		// Pixel shader
		draw.tri = screen;
		draw.vo = vo;
		draw.pMaterial = &material;
		draw.pTile = pTile ? pTile : &m_tiles[0];
		draw.depth = depthMode;
		DrawTriangle(draw);

		// Wireframe
#ifdef CONFIG_WIREFRAME
		DrawLine((int)screen[0].x, (int)screen[0].y, screen[0].z, (int)screen[1].x, (int)screen[1].y, screen[1].z, WHITE);
		DrawLine((int)screen[1].x, (int)screen[1].y, screen[1].z, (int)screen[2].x, (int)screen[2].y, screen[2].z, WHITE);
		DrawLine((int)screen[2].x, (int)screen[2].y, screen[2].z, (int)screen[0].x, (int)screen[0].y, screen[0].z, WHITE);
#endif
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_entity* cpu_engine::CreateEntity()
{
	return m_entityManager.Create();
}

cpu_sprite* cpu_engine::CreateSprite()
{
	return m_spriteManager.Create();
}

cpu_particle_emitter* cpu_engine::CreateParticleEmitter()
{
	return m_particleManager.Create();
}

cpu_rt* cpu_engine::CreateRT(bool depth)
{
	cpu_rt* pRT = m_rtManager.Create();
	pRT->Create(depth);
	return pRT;
}

cpu_entity* cpu_engine::Release(cpu_entity* pEntity)
{
	m_entityManager.Release(pEntity);
	return nullptr;
}

cpu_sprite* cpu_engine::Release(cpu_sprite* pSprite)
{
	m_spriteManager.Release(pSprite);
	return nullptr;
}

cpu_particle_emitter* cpu_engine::Release(cpu_particle_emitter* pEmitter)
{
	m_particleManager.Release(pEmitter);
	return nullptr;
}

cpu_rt* cpu_engine::Release(cpu_rt* pRT)
{
	m_rtManager.Release(pRT);
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_engine::SetCursor(cpu_texture* pTexture)
{
	m_pCursor = pTexture;
}

void cpu_engine::GetCursor(XMFLOAT2& pos)
{
	cpu_rt& rt = *GetMainRT();

	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(m_hWnd, &pt);
	pos.x = (float)(pt.x-m_rcFit.left);
	pos.y = (float)(pt.y-m_rcFit.top);
	pos.x = pos.x*rt.width/(m_rcFit.right-m_rcFit.left);
	pos.y = pos.y*rt.height/(m_rcFit.bottom-m_rcFit.top);
}

void cpu_engine::GetCameraRay(cpu_ray& out, XMFLOAT2& pt)
{
	cpu_rt& rt = *GetMainRT();

	float ndcX = (pt.x * 2.0f / rt.width) - 1.0f;
	float ndcY = 1.0f - (pt.y * 2.0f / rt.height);
	XMFLOAT4X4& world = m_camera.transform.GetWorld();
	XMFLOAT3 right = { world._11, world._12, world._13 };
	XMFLOAT3 up = { world._21, world._22, world._23 };
	XMFLOAT3 forward = { world._31, world._32, world._33 };
	XMFLOAT3 camPos = { world._41, world._42, world._43 };

	// Convert NDC offsets to view-space units using projection scale
	float vx = ndcX / m_camera.matProj._11;
	float vy = ndcY / m_camera.matProj._22;

	if ( m_camera.perspective )
	{
		out.pos = camPos;
		out.dir.x = vx * right.x + vy * up.x + forward.x;
		out.dir.y = vx * right.y + vy * up.y + forward.y;
		out.dir.z = vx * right.z + vy * up.z + forward.z;
	}
	else
	{
        out.pos.x = camPos.x + vx * right.x + vy * up.x;
        out.pos.y = camPos.y + vx * right.y + vy * up.y;
        out.pos.z = camPos.z + vx * right.z + vy * up.z;
		out.dir = forward;
	}

	XMStoreFloat3(&out.dir, XMVector3Normalize(XMLoadFloat3(&out.dir)));
}

void cpu_engine::GetCursorRay(cpu_ray& out)
{
	XMFLOAT2 pt;
	GetCursor(pt);
	GetCameraRay(out, pt);
}

cpu_camera* cpu_engine::GetCamera()
{
	return &m_camera;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_entity* cpu_engine::HitEntity(cpu_hit& hit, cpu_ray& ray)
{
	cpu_entity* pBestEntity = nullptr;
	hit.dist = 0.0f;
	hit.pt = CPU_ZERO;

	XMFLOAT3 ptL;
	float tL;
	XMVECTOR roW = XMLoadFloat3(&ray.pos);
	XMVECTOR ptResult;
	float distResult = FLT_MAX;

	for ( int iEntity=0 ; iEntity<m_entityManager.count ; iEntity++ )
	{
		cpu_entity* pEntity = m_entityManager[iEntity];
		if ( pEntity->dead )
			continue;

		float enter, exit;
		if ( cpu::RayAabb(ray, pEntity->aabb, enter, exit)==false )
			continue;

		// Si même l'entrée dans l'AABB est déjà plus loin que le meilleur hit, skip
		if ( enter>distResult )
			continue;

		XMMATRIX world = XMLoadFloat4x4(&pEntity->transform.GetWorld());
		XMMATRIX invWorld = XMLoadFloat4x4(&pEntity->transform.GetInvWorld());
		cpu_ray rayL;
		ray.ToLocal(rayL, invWorld);

		for ( int i=0 ; i<pEntity->pMesh->triangles.size() ; i++ )
		{
			cpu_triangle& tri = pEntity->pMesh->triangles[i];
			if ( cpu::RayTriangle(rayL, tri, ptL, &tL) )
			{
				XMVECTOR pL = XMLoadFloat3(&ptL);
				XMVECTOR pW = XMVector3TransformCoord(pL, world);
				XMVECTOR d = XMVectorSubtract(pW, roW);

				float distSq;
				XMStoreFloat(&distSq, XMVector3Dot(d, d));
				if ( distSq<distResult )
				{
					distResult = distSq;
					ptResult = pW;
					pBestEntity = pEntity;
				}
			}
		}
	}

	if ( pBestEntity )
	{
		hit.dist = sqrtf(distResult);
		XMStoreFloat3(&hit.pt, ptResult);
	}
	return pBestEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int cpu_engine::GetTotalTriangleCount()
{
	int count = 0;
	for ( int i=0 ; i<m_entityManager.count ; i++ )
	{
		cpu_mesh* pMesh = m_entityManager[i]->pMesh;
		if ( pMesh )
			count += (int)pMesh->triangles.size();
	}
	return count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_engine::DrawText(cpu_font* pFont, const char* text, int x, int y, int align)
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

void cpu_engine::DrawTexture(cpu_texture* pTexture, int x, int y)
{
	cpu_rt& rt = *GetRT();
	byte* dst = (byte*)rt.colorBuffer.data();
	cpu_img32::AlphaBlend(pTexture->bgra, pTexture->width, pTexture->height, dst, rt.width, rt.height, 0, 0, x, y, pTexture->width, pTexture->height);
}

void cpu_engine::DrawSprite(cpu_sprite* pSprite)
{
	DrawTexture(pSprite->pTexture, pSprite->x-pSprite->anchorX, pSprite->y-pSprite->anchorY);
}

void cpu_engine::DrawHorzLine(int x1, int x2, int y, XMFLOAT3& color)
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

void cpu_engine::DrawVertLine(int y1, int y2, int x, XMFLOAT3& color)
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

void cpu_engine::DrawRectangle(int x, int y, int w, int h, XMFLOAT3& color)
{
	DrawHorzLine(x, x+w, y, color);
	DrawHorzLine(x, x+w, y+h, color);
	DrawVertLine(y, y+h, x, color);
	DrawVertLine(y, y+h, x+w, color);
}

void cpu_engine::DrawLine(int x0, int y0, float z0, int x1, int y1, float z1, XMFLOAT3& color)
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

LRESULT cpu_engine::OnWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch ( message )
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		FixWindow();
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool cpu_engine::Time()
{
	DWORD curtime = timeGetTime();
	DWORD deltatime = curtime - m_systime;
	if ( deltatime<5 )
		return false;

	m_systime = curtime;
	if ( deltatime>30 )
		deltatime = 30;
	m_deltaTime = deltatime/1000.0f;
	m_totalTime += m_deltaTime;

	if ( m_systime-m_fpsTime>=1000 )
	{
		m_stats.fps = m_fpsCount;
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

void cpu_engine::Update()
{
	// Reset
	Update_Reset();

	// Controller
	m_input.Update();

	// Physics
	Update_Physics();

	// FSM
	Update_FSM();

	// Particles
	Update_Particles();

	// Callback
	m_callback.onUpdate.Call();

	// Purge
	Update_Purge();
}

void cpu_engine::Update_Reset()
{
	m_camera.transform.ResetFlags();

	for ( int i=0 ; i<m_entityManager.count ; i++ )
	{
		cpu_entity* pEntity = m_entityManager[i];
		if ( pEntity->dead )
			continue;

		pEntity->transform.ResetFlags();
	}
}

void cpu_engine::Update_Physics()
{
	for ( int i=0 ; i<m_entityManager.count ; i++ )
	{
		cpu_entity* pEntity = m_entityManager[i];
		if ( pEntity->dead )
			continue;

		pEntity->lifetime += m_deltaTime;
	}
}

void cpu_engine::Update_FSM()
{
	for ( int i=0 ; i<m_fsmManager.count ; i++ )
	{
		cpu_fsm_base* pFSM = m_fsmManager[i];
		if ( pFSM->dead )
			continue;

		pFSM->Update(m_deltaTime);
	}
}

void cpu_engine::Update_Particles()
{
	// Emitters
	for ( int i=0 ; i<m_particleManager.count ; i++ )
	{
		cpu_particle_emitter* pEmitter = m_particleManager[i];
		if ( pEmitter->dead )
			continue;

		pEmitter->Update();
	}

	// Particles: age
	m_particleData.UpdateAge();

	// Particles: tiles (MT)
	CPU_JOBS(m_particlePhysicsJobs);
}

void cpu_engine::Update_Purge()
{
	m_fsmManager.Purge();
	m_entityManager.Purge();
	m_particleManager.Purge();
	m_spriteManager.Purge();
	m_rtManager.Purge();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_engine::Render()
{
	// RT
	m_pRT = &m_mainRT;

	// Tiles
	for ( int i=0 ; i<m_tileCount ; i++ )
		m_tiles[i].Reset();

	// Prepare
	m_camera.Update();
	Render_SortZ();
	Render_RecalculateMatrices();
	Render_ApplyClipping();
	Render_AssignEntityTile();

	// Clear
	m_callback.onRender.Call(CPU_PASS_CLEAR_BEGIN);
	ClearDepth();
	switch ( m_clear )
	{
	case CPU_CLEAR_COLOR:
		ClearColor(m_clearColor);
		break;
	case CPU_CLEAR_SKY:
		ClearSky();
		break;
	}
	m_callback.onRender.Call(CPU_PASS_CLEAR_END);

	// Entities
	m_callback.onRender.Call(CPU_PASS_ENTITY_BEGIN);
	Render_Entities();
	m_callback.onRender.Call(CPU_PASS_ENTITY_END);

	// Particles
	m_callback.onRender.Call(CPU_PASS_PARTICLE_BEGIN);
	Render_Particles();
	m_callback.onRender.Call(CPU_PASS_PARTICLE_END);

	// Stats
	m_stats.drawnTriangleCount = 0;
	for ( int i=0 ; i<m_tileCount ; i++ )
		m_stats.drawnTriangleCount += m_tiles[i].statsDrawnTriangleCount;

	// UI
	m_callback.onRender.Call(CPU_PASS_UI_BEGIN);
	Render_UI();
	m_callback.onRender.Call(CPU_PASS_UI_END);

	// UI
	m_callback.onRender.Call(CPU_PASS_CURSOR_BEGIN);
	Render_UI();
	m_callback.onRender.Call(CPU_PASS_CURSOR_END);

	// Style
	if ( m_amigaStyle )
		ToAmigaStyle();

	// Present
	Present();
}

void cpu_engine::Render_SortZ()
{
	// Entities
	std::sort(m_entityManager.sortedList.begin(), m_entityManager.sortedList.begin()+m_entityManager.count, [](const cpu_entity* pA, const cpu_entity* pB) { return pA->view.z < pB->view.z; });
	for ( int i=0 ; i<m_entityManager.count ; i++ )
		m_entityManager.sortedList[i]->sortedIndex = i;

	// Sprites
	std::sort(m_spriteManager.sortedList.begin(), m_spriteManager.sortedList.begin()+m_spriteManager.count, [](const cpu_sprite* pA, const cpu_sprite* pB) { return pA->z < pB->z; });
	for ( int i=0 ; i<m_spriteManager.count ; i++ )
		m_spriteManager.sortedList[i]->sortedIndex = i;
}

void cpu_engine::Render_RecalculateMatrices()
{
	cpu_rt& rt = *GetRT();
	float width = (float)rt.width;
	float height = (float)rt.height;

	for ( int iEntity=0 ; iEntity<m_entityManager.count ; iEntity++ )
	{
		cpu_entity* pEntity = m_entityManager[iEntity];
		if ( pEntity->dead )
			continue;

		// World
		XMMATRIX matWorld = XMLoadFloat4x4(&pEntity->transform.GetWorld());

		// cpu_obb
		pEntity->obb = pEntity->pMesh->aabb;
		pEntity->obb.Transform(matWorld);

		// cpu_aabb
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

void cpu_engine::Render_ApplyClipping()
{
	m_stats.clipEntityCount = 0;

	for ( int iEntity=0 ; iEntity<m_entityManager.count ; iEntity++ )
	{
		cpu_entity* pEntity = m_entityManager[iEntity];
		if ( pEntity->dead || pEntity->visible==false || pEntity->pMesh==nullptr )
			continue;

		if ( m_camera.frustum.Intersect(pEntity->transform.pos, pEntity->radius) )
		{
			pEntity->clipped = false;
		}
		else
		{
			pEntity->clipped = true;
			m_stats.clipEntityCount++;
		}
	}
}

void cpu_engine::Render_AssignEntityTile()
{
	for ( int iEntity=0 ; iEntity<m_entityManager.count ; iEntity++ )
	{
		cpu_entity* pEntity = m_entityManager[iEntity];
		if ( pEntity->dead || pEntity->visible==false || pEntity->pMesh==nullptr || pEntity->clipped )
			continue;

		int minX = cpu::Clamp(int(pEntity->box.min.x) / m_tileWidth, 0, m_tileColCount-1);
		int maxX = cpu::Clamp(int(pEntity->box.max.x) / m_tileWidth, 0, m_tileColCount-1);
		int minY = cpu::Clamp(int(pEntity->box.min.y) / m_tileHeight, 0, m_tileRowCount-1);
		int maxY = cpu::Clamp(int(pEntity->box.max.y) / m_tileHeight, 0, m_tileRowCount-1);

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

void cpu_engine::Render_TileEntities(int iTile)
{
	cpu_tile& tile = m_tiles[iTile];
	tile.statsDrawnTriangleCount = 0;
	for ( int iEntity=0 ; iEntity<m_entityManager.count ; iEntity++ )
	{
		cpu_entity* pEntity = m_entityManager.sortedList[iEntity];
		if ( pEntity->dead || pEntity->visible==false || pEntity->pMesh==nullptr || pEntity->clipped )
			continue;
	
		bool entityHasTile = (pEntity->tile>>tile.index) & 1 ? true : false;
		if ( entityHasTile==false )
			continue;

		DrawMesh(pEntity->pMesh, &pEntity->transform, pEntity->pMaterial, pEntity->depth, &tile);
	}
}

void cpu_engine::Render_AssignParticleTile(int iTileForAssign)
{
	cpu_rt& rt = *GetRT();
	cpu_tile& tile = m_tiles[iTileForAssign];

	int min, max;
	GetParticleRange(min, max, iTileForAssign);

	XMFLOAT4X4& vp = m_camera.matViewProj;
	for ( int i=min ; i<max ; i++ )
	{
		float x = m_particleData.px[i];
		float y = m_particleData.py[i];
		float z = m_particleData.pz[i];
		float cw = x*vp._14 + y*vp._24 + z*vp._34 + vp._44;
		if ( cw<=1e-6f )
			continue;
		float cx = x*vp._11 + y*vp._21 + z*vp._31 + vp._41;
		float cy = x*vp._12 + y*vp._22 + z*vp._32 + vp._42;
		float cz = x*vp._13 + y*vp._23 + z*vp._33 + vp._43;

		const float invW = 1.0f / cw;
		const float ndcX = cx * invW;
		if ( ndcX<-1.0f || ndcX>1.0f )
			continue;
		const float ndcY = cy * invW;
		if ( ndcY<-1.0f || ndcY>1.0f )
			continue;
		const float ndcZ = cz * invW;
		if ( ndcZ<0.0f || ndcZ>1.0f )
			continue;

		const int sx = (int)((ndcX + 1.0f) * rt.widthHalf);
		const int sy = (int)((1.0f - ndcY) * rt.heightHalf);
		if ( sx<0 || sy<0 || sx>=rt.width || sy>=rt.height )
			continue;

		int iTile = (sy/m_tileHeight)*m_tileColCount + sx/m_tileWidth;
		if ( iTile>=m_tileCount )
			iTile = m_tileCount - 1;
		m_particleData.tile[i] = iTile + 1;
		m_particleData.sx[i] = (ui16)sx;
		m_particleData.sy[i] = (ui16)sy;
		m_particleData.sz[i] = ndcZ;
		tile.particleLocalCounts[iTile]++;
	}
}

void cpu_engine::Render_TileParticles(int iTile)
{
	cpu_rt& rt = *GetRT();
	cpu_tile& tile = m_tiles[iTile];
	if ( tile.particleCount==0 )
		return;

	const int offset = tile.particleOffset;
	for ( int i=0 ; i<tile.particleCount ; ++i )
	{
		const int p = m_particleData.sort[offset+i];
		const int sx = m_particleData.sx[p];
		const int sy = m_particleData.sy[p];
		const float sz = m_particleData.sz[p];
		const int pix = sy * rt.width + sx;

		if ( sz>=rt.depthBuffer[pix] )
			continue;
		rt.depthBuffer[pix] = sz;

		float a = 1.0f - m_particleData.age[i] * m_particleData.invDuration[i];
		a *= a;

		XMFLOAT3 dst = cpu::ToColorFromBGR(rt.colorBuffer[pix]);
		float r = dst.x + m_particleData.r[i]*a;
		float g = dst.y + m_particleData.g[i]*a;
		float b = dst.z + m_particleData.b[i]*a;
		rt.colorBuffer[pix] = cpu::ToBGR(r, g, b);
	}
}

void cpu_engine::Render_Entities()
{
	CPU_JOBS(m_entityJobs);
}

void cpu_engine::Render_Particles()
{
	// Reset
	memset(m_particleData.tile, 0, m_particleData.alive*sizeof(ui32));

	// Pre-render
	CPU_JOBS(m_particleSpaceJobs);
	for ( int i=0 ; i<m_tileCount ; i++ )
	{
		for ( int j=0 ; j<m_tileCount ; j++ )
			m_tiles[i].particleCount += m_tiles[j].particleLocalCounts[i];
		if ( i>0 )
		{
			m_tiles[i].particleOffset = m_tiles[i-1].particleOffset + m_tiles[i-1].particleCount;
			m_tiles[i].particleOffsetTemp = m_tiles[i].particleOffset;
		}
	}
	for ( int i=0 ; i<m_particleData.alive ; i++ )
	{
		ui32 index = m_particleData.tile[i];
		if ( index )
		{
			cpu_tile& tile = m_tiles[index-1];
			m_particleData.sort[tile.particleOffsetTemp++] = i;
		}
	}

	// Render
	CPU_JOBS(m_particleRenderJobs);
}

void cpu_engine::Render_UI()
{
	for ( int iSprite=0 ; iSprite<m_spriteManager.count ; iSprite++ )
	{
		cpu_sprite* pSprite = m_spriteManager.sortedList[iSprite];
		if ( pSprite->dead || pSprite->visible==false || pSprite->pTexture==nullptr )
			continue;

		DrawSprite(pSprite);
	}
}

void cpu_engine::Render_Cursor()
{
	if ( m_pCursor==nullptr )
		return;

	XMFLOAT2 pt;
	GetCursor(pt);
	DrawTexture(m_pCursor, (int)pt.x, (int)pt.y);	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_engine::DrawTriangle(cpu_draw& draw)
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

	if ( minX<draw.pTile->left ) minX = draw.pTile->left;
	if ( maxX>draw.pTile->right ) maxX = draw.pTile->right;
	if ( minY<draw.pTile->top ) minY = draw.pTile->top;
	if ( maxY>draw.pTile->bottom ) maxY = draw.pTile->bottom;

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

	const CPU_PS_FUNC func = draw.pMaterial->ps ? draw.pMaterial->ps : &PixelShader;
	cpu_ps_io io;
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
			io.p.pos.x = draw.vo[2].worldPos.x + (draw.vo[0].worldPos.x - draw.vo[2].worldPos.x) * w1 + (draw.vo[1].worldPos.x - draw.vo[2].worldPos.x) * w2;
			io.p.pos.y = draw.vo[2].worldPos.y + (draw.vo[0].worldPos.y - draw.vo[2].worldPos.y) * w1 + (draw.vo[1].worldPos.y - draw.vo[2].worldPos.y) * w2;
			io.p.pos.z = draw.vo[2].worldPos.z + (draw.vo[0].worldPos.z - draw.vo[2].worldPos.z) * w1 + (draw.vo[1].worldPos.z - draw.vo[2].worldPos.z) * w2;

			// Normal (lerp)
			io.p.normal.x = draw.vo[2].worldNormal.x + (draw.vo[0].worldNormal.x - draw.vo[2].worldNormal.x) * w1 + (draw.vo[1].worldNormal.x - draw.vo[2].worldNormal.x) * w2;
			io.p.normal.y = draw.vo[2].worldNormal.y + (draw.vo[0].worldNormal.y - draw.vo[2].worldNormal.y) * w1 + (draw.vo[1].worldNormal.y - draw.vo[2].worldNormal.y) * w2;
			io.p.normal.z = draw.vo[2].worldNormal.z + (draw.vo[0].worldNormal.z - draw.vo[2].worldNormal.z) * w1 + (draw.vo[1].worldNormal.z - draw.vo[2].worldNormal.z) * w2;

			// Color (lerp)
			io.p.albedo.x = draw.vo[2].albedo.x + (draw.vo[0].albedo.x - draw.vo[2].albedo.x) * w1 + (draw.vo[1].albedo.x - draw.vo[2].albedo.x) * w2;
			io.p.albedo.y = draw.vo[2].albedo.y + (draw.vo[0].albedo.y - draw.vo[2].albedo.y) * w1 + (draw.vo[1].albedo.y - draw.vo[2].albedo.y) * w2;
			io.p.albedo.z = draw.vo[2].albedo.z + (draw.vo[0].albedo.z - draw.vo[2].albedo.z) * w1 + (draw.vo[1].albedo.z - draw.vo[2].albedo.z) * w2;

			// Lighting
			if ( draw.pMaterial->lighting==CPU_LIGHTING_GOURAUD )
			{
				float intensity = draw.vo[2].intensity + (draw.vo[0].intensity - draw.vo[2].intensity) * w1 + (draw.vo[1].intensity - draw.vo[2].intensity) * w2;
				io.p.color.x = io.p.albedo.x * intensity;
				io.p.color.y = io.p.albedo.y * intensity;
				io.p.color.z = io.p.albedo.z * intensity;
			}
			else if ( draw.pMaterial->lighting==CPU_LIGHTING_LAMBERT )
			{
				XMVECTOR n = XMLoadFloat3(&io.p.normal);				
				//n = XMVector3Normalize(n); // Expensive (better results)
				XMVECTOR l = XMLoadFloat3(&m_lightDir);
				float ndotl = XMVectorGetX(XMVector3Dot(n, l));
				if ( ndotl<0.0f )
					ndotl = 0.0f;
				float intensity = ndotl + m_ambient;
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
	draw.pTile->statsDrawnTriangleCount++;
}

void cpu_engine::PixelShader(cpu_ps_io& io)
{
	io.color = io.p.color;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_engine::Present()
{
	cpu_rt& rt = *GetRT();

#ifdef CONFIG_GPU

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
		FixDevice();
	}

#else

	int width = m_rcFit.right - m_rcFit.left;
	int height = m_rcFit.bottom - m_rcFit.top;
	if ( width==rt.width && height==rt.height )
		SetDIBitsToDevice(m_hDC, m_rcFit.left, m_rcFit.top, rt.width, rt.height, 0, 0, 0, rt.height, rt.colorBuffer.data(), &m_bi, DIB_RGB_COLORS);
	else
	{
		StretchDIBits(m_hDC, m_rcFit.left, m_rcFit.top, width, height, 0, 0, rt.width, rt.height, rt.colorBuffer.data(), &m_bi, DIB_RGB_COLORS, SRCCOPY);
		if ( m_rcFit.left )
			PatBlt(m_hDC, 0, 0, m_rcFit.left, m_windowHeight, PATCOPY);
		if ( m_rcFit.right<m_windowWidth )
			PatBlt(m_hDC, m_rcFit.right, 0, m_windowWidth-m_rcFit.right, m_windowHeight, PATCOPY);
		if ( m_rcFit.top )
			PatBlt(m_hDC, 0, 0, m_windowWidth, m_rcFit.top, PATCOPY);
		if ( m_rcFit.bottom<m_windowHeight )
			PatBlt(m_hDC, 0, m_rcFit.bottom, m_windowWidth, m_windowHeight-m_rcFit.bottom, PATCOPY);
	}

#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT cpu_engine::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	cpu_engine* pEngine = (cpu_engine*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if ( pEngine )
		return pEngine->OnWindowProc(hWnd, message, wParam, lParam);

	return DefWindowProc(hWnd, message, wParam, lParam);
}
