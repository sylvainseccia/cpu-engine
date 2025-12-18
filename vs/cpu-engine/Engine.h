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

	static cpu_engine* GetInstance();
	static cpu_engine& GetInstanceRef();

	void Initialize(HINSTANCE hInstance, int renderWidth, int renderHeight, float windowScaleAtStart = 1.0f, bool hardwareBilinear = false);
	void Run();
	void FixWindow();
	void FixProjection();
	void FixDevice();
	HWND GetHWND() { return m_hWnd; }
	cpu_rt* GetMainRT() { return &m_mainRT; }
	cpu_rt* SetRT(cpu_rt* pRT) { cpu_rt* pOld = m_pRT; m_pRT = pRT; return pOld; }
	cpu_rt* GetRT() { return m_pRT; }
	cpu_input& GetInput() { return m_input; }
	float GetTotalTime() { return m_totalTime; }
	float GetDeltaTime() { return m_deltaTime; };
	cpu_particle_data* GetParticleData() { return &m_particleData; }
	cpu_particle_physics* GetParticlePhysics() { return &m_particlePhysics; }
	int NextTile() { return m_nextTile.Add(); }

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

	void GetCursor(XMFLOAT2& pt);
	cpu_ray GetCameraRay(XMFLOAT2& pt);
	cpu_camera* GetCamera();

	int GetTotalTriangleCount();

	void DrawText(cpu_font* pFont, const char* text, int x, int y, int align = TEXT_LEFT);
	void DrawSprite(cpu_sprite* pSprite);
	void DrawHorzLine(int x1, int x2, int y, XMFLOAT3& color);
	void DrawVertLine(int y1, int y2, int x, XMFLOAT3& color);
	void DrawRectangle(int x, int y, int w, int h, XMFLOAT3& color);
	void DrawLine(int x0, int y0, float z0, int x1, int y1, float z1, XMFLOAT3& color);

protected:
	virtual void OnStart() {}
	virtual void OnUpdate() {}
	virtual void OnExit() {}
	virtual void OnPreRender() {}
	virtual void OnPostRender() {}
	virtual LRESULT OnWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	bool Time();

	void Update();
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
	void Render_UI();

	void Clear();
	void ClearDepth();
	void Fill(XMFLOAT3& rgb);
	void FillSky();
	void DrawEntity(cpu_entity* pEntity, cpu_tile& tile);
	void FillTriangle(cpu_drawcall& dc);
	bool Copy(byte* dst, int dstW, int dstH, int dstX, int dstY, const byte* src, int srcW, int srcH, int srcX, int srcY, int w, int h);
	static void PixelShader(cpu_ps_io& io);

	void Present();

	static LRESULT WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:
	// Controller
	cpu_input m_input;

	// Color
	int m_clear;
	XMFLOAT3 m_clearColor;
	XMFLOAT3 m_groundColor;
	XMFLOAT3 m_skyColor;

	// Light
	XMFLOAT3 m_lightDir;
	float m_ambient;
	cpu_material m_defaultMaterial;
	
	// Time
	float m_totalTime;
	float m_deltaTime;
	int m_fps;

	// Camera
	cpu_camera m_camera;

	// Particle
	cpu_particle_data m_particleData;
	cpu_particle_physics m_particlePhysics;

	// Stats: render
	int m_statsClipEntityCount;
	int m_statsThreadCount;
	int m_statsTileCount;
	int m_statsDrawnTriangleCount;

private:
	inline static cpu_engine* s_pEngine;

	// Jobs
	int m_threadCount;
	std::vector<cpu_job_entity> m_entityJobs;
	std::vector<cpu_job_particle_space> m_particleSpaceJobs;
	std::vector<cpu_job_particle_render> m_particleRenderJobs;
	cpu_atomic<int> m_nextTile;

	// Window
	HINSTANCE m_hInstance;
	HWND m_hWnd;
	int m_windowWidth;
	int m_windowHeight;
	bool m_bilinear;

	// Surface
#ifdef CONFIG_GPU
	ID2D1Factory* m_pD2DFactory;
	ID2D1HwndRenderTarget* m_pRenderTarget;
	ID2D1Bitmap* m_pBitmap;
#else
	HDC m_hDC;
	BITMAPINFO m_bi;
#endif

	// Buffer
	cpu_rt m_mainRT;
	cpu_rt* m_pRT;

	// Camera
	bool m_cullFrontCCW = false; // DirectX default
	float m_cullAreaEpsilon = 1e-6f;

	// Tile
	int m_tileWidth;
	int m_tileHeight;
	int m_tileRowCount;
	int m_tileColCount;
	int m_tileCount;
	std::vector<cpu_tile> m_tiles;

	// Time
	DWORD m_systime;
	DWORD m_fpsTime;
	int m_fpsCount;

	// Managers
	cpu_manager<cpu_fsm_base> m_fsmManager;
	cpu_manager<cpu_entity> m_entityManager;
	cpu_manager<cpu_particle_emitter> m_particleManager;
	cpu_manager<cpu_sprite> m_spriteManager;
	cpu_manager<cpu_rt> m_rtManager;
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
