#include "pch.h"

cpu_engine::cpu_engine()
{
	s_pEngine = this;
}

cpu_engine::~cpu_engine()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool cpu_engine::Create(int width, int height, bool fullscreen, bool amigaStyle)
{
	// Window
	if ( m_window.Create(width, height, fullscreen)==false )
		return false;

	// Device
	if ( m_device.Create(&m_window, width, height)==false )
		return false;

	// Options
	m_renderEnabled = true;

	// Style
	m_amigaStyle = amigaStyle;
	m_clear = CPU_CLEAR_SKY;
	m_clearColor = cpu::ToColor(24, 35, 50);
	m_groundColor = cpu::ToColor(42, 63, 53);
	m_skyColor = cpu::ToColor(24, 35, 50);

	// Particles
	m_particleData.pPhysics = &m_particlePhysics;

	// Cursor
	m_pCursor = nullptr;

	// Managers
	ClearManagers();

	// Cores
#ifdef CPU_CONFIG_MT
	m_threadCount = std::max(1u, std::thread::hardware_concurrency());
#else
	m_threadCount = 1;
#endif

	// Tiles
	m_tileColCount = cpu::CeilToInt(sqrtf((float)m_threadCount));
	m_tileRowCount = (m_threadCount + m_tileColCount - 1) / m_tileColCount;
	m_tileCount = m_tileColCount * m_tileRowCount;
	m_stats.tileCount = m_tileCount;
	m_tileWidth = width / m_tileColCount;
	m_tileHeight = height / m_tileRowCount;
	int missingWidth = width - (m_tileWidth*m_tileColCount);
	int missingHeight = height - (m_tileHeight*m_tileRowCount);
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

	// Callback
	m_callback.onStart.Set(this, &cpu_engine::OnStart);
	m_callback.onUpdate.Set(this, &cpu_engine::OnUpdate);
	m_callback.onExit.Set(this, &cpu_engine::OnExit);
	m_callback.onRender.Set(this, &cpu_engine::OnRender);

	// Camera
	m_device.SetCamera(&m_camera);
	m_camera.aspectRatio = float(width) / float(height);
	m_camera.height = m_camera.width / m_camera.aspectRatio;
	m_camera.UpdateProjection();

	return true;
}

void cpu_engine::Run()
{
	// Window
	m_window.Show();

	// Threads
	for ( int i=0 ; i<m_threadCount ; i++ )
		m_threads[i].Run();

	// Reset
	cpuTime.Reset();
	cpuInput.Reset();

	// Start
	m_callback.onStart.Call();

	// Loop
	while ( m_window.Update() )
	{
		// Time
		if ( cpuTime.Update()==false )
			continue;

		// Update
		Update();

		// Render
		Render();
	}

	// Cursor
	SetCursor(nullptr);

	// End
	Update_Purge();
	m_callback.onExit.Call();

	// Threads
	for ( int i=0 ; i<m_threadCount ; i++ )
		m_threads[i].Stop();

	// Jobs
	m_entityJobs.clear();
	m_particlePhysicsJobs.clear();
	m_particleSpaceJobs.clear();
	m_particleRenderJobs.clear();

	// Managers
	Update_Purge();
	ClearManagers();
}

void cpu_engine::Quit()
{
	m_window.Quit();
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
	cpu_particle_emitter* pEmitter = m_particleManager.Create();
	pEmitter->pData = &m_particleData;
	return pEmitter;
}

cpu_rt* cpu_engine::CreateRT(bool depth)
{
	cpu_rt* pRT = m_rtManager.Create();
	pRT->Create(m_device.GetWidth(), m_device.GetHeight(), depth);
	return pRT;
}

void cpu_engine::ClearManagers()
{
	m_entityManager.Clear();
	m_spriteManager.Clear();
	m_particleManager.Clear();
	m_fsmManager.Clear();
	m_rtManager.Clear();
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
	cpu_texture* pOld = m_pCursor;
	m_pCursor = pTexture;
	if ( m_pCursor )
	{
		if ( pOld==nullptr )
			ShowCursor(FALSE);
	}
	else
	{
		if ( pOld )
			ShowCursor(TRUE);
	}
}

void cpu_engine::SetCursor(XMFLOAT2& pos)
{
	cpu_rt& rt = *m_device.GetMainRT();
	RECT& rcFit = m_device.GetFit();

	POINT pt;
	pt.x = rcFit.left + (int)(pos.x/float(rt.width)*float(rcFit.right-rcFit.left));
	pt.y = rcFit.top + (int)(pos.y/float(rt.height)*float(rcFit.bottom-rcFit.top));
	ClientToScreen(m_window.GetHWND(), &pt);
	SetCursorPos(pt.x, pt.y);
}

void cpu_engine::GetCursor(XMFLOAT2& pos)
{
	cpu_rt& rt = *m_device.GetMainRT();
	RECT& rcFit = m_device.GetFit();

	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(m_window.GetHWND(), &pt);
	pos.x = (float)(pt.x-rcFit.left);
	pos.y = (float)(pt.y-rcFit.top);
	pos.x = pos.x*rt.width/(rcFit.right-rcFit.left);
	pos.y = pos.y*rt.height/(rcFit.bottom-rcFit.top);
}

void cpu_engine::GetCameraRay(cpu_ray& out, XMFLOAT2& pt)
{
	cpu_rt& rt = *m_device.GetMainRT();

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

void cpu_engine::Update()
{
	// Reset
	Update_Reset();

	// Controller
	cpuInput.Update();

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

		pEntity->lifetime += cpuTime.delta;
	}
}

void cpu_engine::Update_FSM()
{
	for ( int i=0 ; i<m_fsmManager.count ; i++ )
	{
		cpu_fsm_base* pFSM = m_fsmManager[i];
		if ( pFSM->dead )
			continue;

		pFSM->Update();
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

		pEmitter->Update(m_camera.matViewProj, m_device.GetWidth(), m_device.GetHeight());
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
	// Camera
	m_device.UpdateCamera();

	// Rendering disabled
	if ( m_renderEnabled==false )
		return;

	// Tiles
	for ( int i=0 ; i<m_tileCount ; i++ )
		m_tiles[i].Reset();

	// Prepare
	Render_SortZ();
	Render_RecalculateMatrices();
	Render_ApplyClipping();
	Render_AssignEntityTile();

	// Clear
	m_callback.onRender.Call(CPU_PASS_CLEAR_BEGIN);
	m_device.ClearDepth();
	switch ( m_clear )
	{
	case CPU_CLEAR_COLOR:
		m_device.ClearColor(m_clearColor);
		break;
	case CPU_CLEAR_SKY:
		m_device.ClearSky(m_groundColor, m_skyColor);
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
	Render_Cursor();
	m_callback.onRender.Call(CPU_PASS_CURSOR_END);

	// Style
	if ( m_amigaStyle )
		m_device.ToAmigaStyle();

	// Present
	m_device.Present();
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
	cpu_rt& rt = *m_device.GetRT();
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

		m_device.DrawMesh(pEntity->pMesh, &pEntity->transform, pEntity->pMaterial, pEntity->depth, &tile);
	}
}

void cpu_engine::Render_AssignParticleTile(int iTileForAssign)
{
	cpu_rt& rt = *m_device.GetRT();
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
	cpu_rt& rt = *m_device.GetRT();
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

		m_device.DrawSprite(pSprite);
	}
}

void cpu_engine::Render_Cursor()
{
	if ( m_pCursor==nullptr )
		return;

	XMFLOAT2 pt;
	GetCursor(pt);
	m_device.DrawTexture(m_pCursor, (int)pt.x, (int)pt.y);	
}
