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

	cpu_callback* GetCallback() { return &m_callback; }
	bool Create(int width, int height, bool fullscreen, bool amigaStyle);
	void Run();
	void Quit();

	cpu_window* GetWindow() { return &m_window; }
	cpu_device* GetDevice() { return &m_device; }
	cpu_particle_data* GetParticleData() { return &m_particleData; }
	cpu_particle_physics* GetParticlePhysics() { return &m_particlePhysics; }
	int NextTile() { return m_nextTile.Add(1); }
	void GetParticleRange(int& min, int& max, int iTile);
	cpu_stats* GetStats() { return &m_stats; }
	void EnableRender(bool enabled = true) { m_renderEnabled = enabled; }

	void ClearManagers();
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
	void SetCursor(XMFLOAT2& pt);
	void GetCursor(XMFLOAT2& pt);
	void GetCameraRay(cpu_ray& out, XMFLOAT2& pt);
	void GetCursorRay(cpu_ray& out);
	cpu_camera* GetCamera();

	cpu_entity* HitEntity(cpu_hit& hit, cpu_ray& ray);

	int GetTotalTriangleCount();

private:
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

	void OnStart() {}
	void OnUpdate() {}
	void OnExit() {}
	void OnRender(int pass) {}

private:
	inline static cpu_engine* s_pEngine;

	// Options
	bool m_renderEnabled;

	// Window
	cpu_window m_window;

	// Device
	cpu_device m_device;

	// Camera
	cpu_camera m_camera;
	
	// Style
	bool m_amigaStyle;
	int m_clear;
	XMFLOAT3 m_clearColor;
	XMFLOAT3 m_groundColor;
	XMFLOAT3 m_skyColor;

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
