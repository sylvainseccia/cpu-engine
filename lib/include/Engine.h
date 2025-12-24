#pragma once

class cpu_engine
{
public:
	friend cpu_job_entity;
	friend cpu_job_particle_space;
	friend cpu_job_particle_render;

public:
	cpu_engine();
	virtual ~cpu_engine();
	void Free();

	static cpu_engine& GetInstance() { return *s_pEngine; }

	bool Initialize(HINSTANCE hInstance, int renderWidth, int renderHeight, bool fullscreen, bool amigaStyle);
	cpu_callback* GetCallback() { return &m_callback; }
	void Run();
	void Quit();
	void FixWindow();
	void FixDevice();

	HWND GetHWND() { return m_hWnd; }
	cpu_input* Input() { return &m_input; }
	float TotalTime() { return m_totalTime; }
	float DeltaTime() { return m_deltaTime; };
	float Since(float t) { return m_totalTime-t; }
	cpu_particle_data* GetParticleData() { return &m_particleData; }
	cpu_particle_physics* GetParticlePhysics() { return &m_particlePhysics; }
	int NextTile() { return m_nextTile.AddOne(); }
	void GetParticleRange(int& min, int& max, int iTile);
	cpu_stats* GetStats() { return &m_stats; }

	cpu_rt* SetMainRT(bool copyDepth = true);
	cpu_rt* GetMainRT() { return &m_mainRT; }
	cpu_rt* SetRT(cpu_rt* pRT, bool copyDepth = true);
	cpu_rt* GetRT() { return m_pRT; }
	void CopyDepth(cpu_rt* pRT);
	void AlphaBlend(cpu_rt* pRT);
	void ToAmigaStyle();
	void Blur(int radius);
	void ClearColor();
	void ClearColor(XMFLOAT3& rgb);
	void ClearSky();
	void ClearDepth();
	void DrawMesh(cpu_mesh* pMesh, cpu_transform* pTransform, cpu_material* pMaterial, int depthMode = CPU_DEPTH_RW, cpu_tile* pTile = nullptr);

	template <typename T>
	cpu_fsm<T>* CreateFSM(T* pInstance);
	cpu_entity* CreateEntity();
	cpu_sprite* CreateSprite();
	cpu_particle_emitter* CreateParticleEmitter();
	cpu_rt* CreateRT(bool depth = true);
	template <typename T>
	cpu_fsm<T>* Release(cpu_fsm<T>* pFSM);
	cpu_entity* Release(cpu_entity* pEntity);
	cpu_sprite* Release(cpu_sprite* pSprite);
	cpu_particle_emitter* Release(cpu_particle_emitter* pEmitter);
	cpu_rt* Release(cpu_rt* pRT);

	void SetCursor(cpu_texture* pTexture);
	void GetCursor(XMFLOAT2& pt);
	void GetCameraRay(cpu_ray& out, XMFLOAT2& pt);
	void GetCursorRay(cpu_ray& out);
	cpu_camera* GetCamera();

	cpu_entity* HitEntity(cpu_hit& hit, cpu_ray& ray);

	int GetTotalTriangleCount();

	void DrawText(cpu_font* pFont, const char* text, int x, int y, int align = CPU_TEXT_LEFT);
	void DrawTexture(cpu_texture* pTexture, int x, int y);
	void DrawSprite(cpu_sprite* pSprite);
	void DrawHorzLine(int x1, int x2, int y, XMFLOAT3& color);
	void DrawVertLine(int y1, int y2, int x, XMFLOAT3& color);
	void DrawRectangle(int x, int y, int w, int h, XMFLOAT3& color);
	void DrawLine(int x0, int y0, float z0, int x1, int y1, float z1, XMFLOAT3& color);

private:
	bool Time();

	void Update();
	void Update_Reset();
	void Update_Physics();
	void Update_FSM();
	void Update_Particles();
	void Update_Purge();

	void Render();
	void Render_SortZ();
	void Render_RecalculateMatrices();
	void Render_ApplyClipping();
	void Render_AssignEntityTile();
	void Render_TileEntities(int iTile);
	void Render_AssignParticleTile(int iTileForAssign);
	void Render_TileParticles(int iTile);
	void Render_Entities();
	void Render_Particles();
	void Render_UI();
	void Render_Cursor();

	void DrawTriangle(cpu_draw& draw);
	static void PixelShader(cpu_ps_io& io);

	void Present();

	static LRESULT WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT OnWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnStart() {}
	void OnUpdate() {}
	void OnExit() {}
	void OnRender(int pass) {}

private:
	inline static cpu_engine* s_pEngine;

	// Window
	HINSTANCE m_hInstance;
	HWND m_hWnd;
	int m_windowWidth;
	int m_windowHeight;
	RECT m_rcFit;

	// Controller
	cpu_input m_input;

	// Surface
#ifdef CONFIG_GPU
	ID2D1Factory* m_pD2DFactory;
	ID2D1HwndRenderTarget* m_pRenderTarget;
	ID2D1Bitmap* m_pBitmap;
#else
	HDC m_hDC;
	HBRUSH m_hBrush;
	BITMAPINFO m_bi;
#endif

	// Color
	bool m_amigaStyle;
	int m_clear;
	XMFLOAT3 m_clearColor;
	XMFLOAT3 m_groundColor;
	XMFLOAT3 m_skyColor;

	// Buffer
	cpu_rt m_mainRT;
	cpu_rt* m_pRT;

	// Camera
	bool m_cullFrontCCW = false; // DirectX default
	float m_cullAreaEpsilon = 1e-6f;
	cpu_camera m_camera;

	// Light
	XMFLOAT3 m_lightDir;
	float m_ambient;
	cpu_material m_defaultMaterial;
	
	// Tile
	int m_tileWidth;
	int m_tileHeight;
	int m_tileRowCount;
	int m_tileColCount;
	int m_tileCount;
	std::vector<cpu_tile> m_tiles;
	cpu_atomic<int> m_nextTile;

	// Jobs
	int m_threadCount;
	std::vector<cpu_thread_job> m_threads;
	std::vector<cpu_job_entity> m_entityJobs;
	std::vector<cpu_job_particle_physics> m_particlePhysicsJobs;
	std::vector<cpu_job_particle_space> m_particleSpaceJobs;
	std::vector<cpu_job_particle_render> m_particleRenderJobs;

	// Time
	DWORD m_systime;
	DWORD m_fpsTime;
	int m_fpsCount;
	float m_totalTime;
	float m_deltaTime;

	// Particle
	cpu_particle_data m_particleData;
	cpu_particle_physics m_particlePhysics;

	// Cursor
	cpu_texture* m_pCursor;

	// Managers
	cpu_manager<cpu_fsm_base> m_fsmManager;
	cpu_manager<cpu_entity> m_entityManager;
	cpu_manager<cpu_particle_emitter> m_particleManager;
	cpu_manager<cpu_sprite> m_spriteManager;
	cpu_manager<cpu_rt> m_rtManager;

	// Callback
	cpu_callback m_callback;

	// Stats: render
	cpu_stats m_stats;
};

template <typename T>
cpu_fsm<T>* cpu_engine::CreateFSM(T* pInstance)
{
	cpu_fsm<T>* pFSM = new cpu_fsm<T>(pInstance);
	m_fsmManager.Add((cpu_fsm_base*)pFSM);
	return pFSM;
}

template <typename T>
cpu_fsm<T>* cpu_engine::Release(cpu_fsm<T>* pFSM)
{
	m_fsmManager.Release(pFSM);
	return nullptr;
}
