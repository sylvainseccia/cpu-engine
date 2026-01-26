#include "pch.h"

App::App()
{
	s_pApp = this;
	CPU_CALLBACK_START(OnStart);
	CPU_CALLBACK_UPDATE(OnUpdate);
	CPU_CALLBACK_EXIT(OnExit);
	CPU_CALLBACK_RENDER(OnRender);

	m_pShip = nullptr;
}

App::~App()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void App::SpawnMissile()
{
	cpu_entity* pMissile = cpuEngine.CreateEntity();
	pMissile->pMesh = &m_meshMissile;
	pMissile->transform.SetScaling(0.2f);
	pMissile->transform.pos = m_pShip->GetEntity()->transform.pos;
	pMissile->transform.SetRotation(m_pShip->GetEntity()->transform);
	pMissile->transform.Move(1.5f);
	m_missiles.push_back(pMissile);
}

void App::SpawnMissileWithMouse()
{
	cpu_ray ray;
	cpuEngine.GetCursorRay(ray);
	cpu_entity* pMissile = cpuEngine.CreateEntity();
	pMissile->pMesh = &m_meshMissile;
	pMissile->transform.SetScaling(0.2f);
	pMissile->transform.pos = ray.pos;
	pMissile->transform.LookTo(ray.dir);
	pMissile->transform.Move(1.5f);
	pMissile->pMaterial = &m_materialMissile;
	m_missiles.push_back(pMissile);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void App::OnStart()
{
	// YOUR CODE HERE

	// Render
	//cpuEngine.EnableBoxRender();

	// Resources
	m_font.Create(cpuDevice.GetHeight()<=512 ? 14 : 28);
	m_textureBird.Load("bird_amiga.png");
	m_textureEarth.Load("earth.png");
	m_meshShip.CreateSpaceship();
	m_meshMissile.CreateSphere(0.5f);
	m_meshSphere.CreateSphere(2.0f, 12, 12);
	m_rts[0] = cpuEngine.CreateRT();

	// UI
	m_pSprite = cpuEngine.CreateSprite();
	m_pSprite->pTexture = &m_textureBird;
	m_pSprite->CenterAnchor();
	m_pSprite->x = 40;
	m_pSprite->y = 0;

	// Shader
	m_materialShip.color = cpu::ToColor(255, 128, 0);
	m_materialMissile.ps = MissileShader;
	m_materialMoon.ps = MoonShader;
	m_materialEarth.pTexture = &m_textureEarth;

	// 3D
	m_missileSpeed = 10.0f;
	m_pEarth = cpuEngine.CreateEntity();
	m_pEarth->pMesh = &m_meshSphere;
	m_pEarth->pMaterial = &m_materialEarth;
	m_pEarth->transform.pos.x = 3.0f;
	m_pEarth->transform.pos.y = 3.0f;
	m_pEarth->transform.pos.z = 5.0f;
	m_pMoon = cpuEngine.CreateEntity();
	m_pMoon->pMesh = &m_meshSphere;
	m_pMoon->pMaterial = &m_materialMoon;
	m_pMoon->transform.SetScaling(0.1f);

	// Ship
	m_pShip = new Ship;
	m_pShip->Create(&m_meshShip, &m_materialShip);
	m_pShip->GetFSM()->ToState(CPU_ID(StateShipIdle));

	// Particle
	cpuEngine.GetParticleData()->Create(2000000);
	cpuEngine.GetParticlePhysics()->gy = -0.5f;
	m_pEmitter = cpuEngine.CreateParticleEmitter();
	m_pEmitter->density = 3000.0f;
	m_pEmitter->colorMin = cpu::ToColor(255, 0, 0);
	m_pEmitter->colorMax = cpu::ToColor(255, 128, 0);
	m_pEmitter2 = cpuEngine.CreateParticleEmitter();
	m_pEmitter2->density = 300.0f;
	m_pEmitter2->colorMin = cpu::ToColor(0, 0, 255);
	m_pEmitter2->colorMax = cpu::ToColor(0, 128, 255);
	m_pEmitter2->pos.x = -2.0f;

	// Camera
	cpuEngine.GetCamera()->transform.pos.z = -5.0f;
}

void App::OnUpdate()
{
	// YOUR CODE HERE

	float dt = cpuTime.delta;
	float time = cpuTime.total;

	// Move sprite
	m_pSprite->y = 60 + cpu::RoundToInt(sinf(time)*20.0f);

	// Turn earth
	m_pEarth->transform.AddYPR(-dt);

	// Move rock
	m_pMoon->transform.OrbitAroundAxis(m_pEarth->transform.pos, CPU_VEC3_UP, 3.0f, time*2.0f);
	m_pEmitter->pos = m_pMoon->transform.pos;
	m_pEmitter->dir = m_pMoon->transform.dir;
	m_pEmitter->dir.x = -m_pEmitter->dir.x; 
	m_pEmitter->dir.y = -m_pEmitter->dir.y; 
	m_pEmitter->dir.z = -m_pEmitter->dir.z; 

	// Turn camera
	cpuEngine.GetCamera()->transform.AddYPR(0.0f, 0.0f, dt*0.1f);

	// Move ship
	if ( cpuInput.IsKey(VK_UP) )
	{
		cpuEngine.GetCamera()->transform.Move(cpuTime.delta*1.0f);
	}
	if ( cpuInput.IsKey(VK_DOWN) )
	{
		cpuEngine.GetCamera()->transform.Move(-cpuTime.delta*1.0f);
	}

	// Move missiles
	for ( auto it=m_missiles.begin() ; it!=m_missiles.end() ; ++it )
	{
		cpu_entity* pMissile = *it;
		pMissile->transform.Move(dt*m_missileSpeed);
		if ( pMissile->lifetime>10.0f )
			cpuEngine.Release(pMissile);
	}

	// Fire
	if ( cpuInput.IsKeyDown(VK_LBUTTON) || cpuInput.IsKey(VK_RBUTTON) )
		cpuApp.SpawnMissileWithMouse();

	// Purge missiles
	for ( auto it=m_missiles.begin() ; it!=m_missiles.end() ; )
	{
		if ( (*it)->dead )
			it = m_missiles.erase(it);
		else
			++it;
	}

	// Quit
	if ( cpuInput.IsKeyDown(VK_ESCAPE) )
		cpuEngine.Quit();
}

void App::OnExit()
{
	// YOUR CODE HERE

	if ( m_pShip )
		m_pShip->Destroy();
	CPU_DELPTR(m_pShip);
	m_missiles.clear();
}

void App::OnRender(int pass)
{
	// YOUR CODE HERE

	switch ( pass )
	{
		case CPU_PASS_PARTICLE_BEGIN:
		{
			// Blur particles
			//cpuEngine.SetRT(m_rts[0]);
			//cpuEngine.ClearColor();
			break;
		}
		case CPU_PASS_PARTICLE_END:
		{
			// Blur particles
			//cpuEngine.Blur(10);
			//cpuEngine.SetMainRT();
			//cpuEngine.AlphaBlend(m_rts[0]);
			break;
		}
		case CPU_PASS_UI_END:
		{
			// Debug
			cpu_stats& stats = *cpuEngine.GetStats();
			std::string info = CPU_STR(cpuTime.fps) + " fps, ";
			info += CPU_STR(stats.drawnTriangleCount) + " triangles, ";
			info += CPU_STR(stats.clipEntityCount) + " clipped entities\n";
			info += CPU_STR(m_missiles.size()) + " missiles, ";
			info += CPU_STR(cpuEngine.GetParticleData()->alive) + " particles, ";
			info += CPU_STR(stats.threadCount) + " threads, ";
			info += CPU_STR(stats.tileCount) + " tiles";

			// Ray cast
			cpu_ray ray;
			cpuEngine.GetCursorRay(ray);
			cpu_hit hit;
			cpu_entity* pEntity = cpuEngine.HitEntity(hit, ray);
			if ( pEntity )
			{
				info += "\nHIT: ";
				info += CPU_STR(pEntity->index).c_str();
			}

			cpuDevice.DrawText(&m_font, info.c_str(), (int)(cpuDevice.GetWidth()*0.5f), 10, CPU_TEXT_CENTER);
			break;
		}
	}
}

void App::MissileShader(cpu_ps_io& io)
{
	// garder seulement le rouge du pixel éclairé
	io.color.x = io.p.color.x;
}

void App::MoonShader(cpu_ps_io& io)
{
	float time = cpuTime.total;
	float scale = ((sinf(time*3.0f)*0.5f)+0.5f) * 0.5f + 0.5f; 
	io.color.x = io.p.color.x * scale;
	io.color.y = io.p.color.y * scale;
	io.color.z = io.p.color.z;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Ship::Ship()
{
	m_pEntity = nullptr;
	m_pFSM = nullptr;
}

Ship::~Ship()
{
}

void Ship::Create(cpu_mesh* pMesh, cpu_material* pMaterial)
{
	m_pEntity = cpuEngine.CreateEntity();
	m_pEntity->pMesh = pMesh;
	m_pEntity->pMaterial = pMaterial;
	m_pEntity->transform.pos.z = 5.0f;
	m_pEntity->transform.pos.y = -3.0f;

	m_pFSM = cpuEngine.CreateFSM(this);
	m_pFSM->SetGlobal<StateShipGlobal>();
	m_pFSM->Add<StateShipIdle>();
	m_pFSM->Add<StateShipBlink>();
}

void Ship::Destroy()
{
	m_pFSM = cpuEngine.Release(m_pFSM);
	m_pEntity = cpuEngine.Release(m_pEntity);
}

void Ship::Update()
{
	float dt = cpuTime.delta;

	// Turn ship
	m_pEntity->transform.AddYPR(dt, dt, dt);

	// Move ship
	m_pEntity->transform.pos.z += dt * 1.0f;

	// Fire
	if ( cpuInput.IsKey(VK_SPACE) )
		cpuApp.SpawnMissile();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void StateShipGlobal::OnEnter(Ship& cur, int from)
{
}

void StateShipGlobal::OnExecute(Ship& cur)
{
	cur.Update();
}

void StateShipGlobal::OnExit(Ship& cur, int to)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void StateShipIdle::OnEnter(Ship& cur, int from)
{
}

void StateShipIdle::OnExecute(Ship& cur)
{
	// Blink every 3 seconds
	if ( cur.GetFSM()->totalTime>3.0f )
	{
		cur.GetFSM()->ToState(CPU_ID(StateShipBlink));
		return;
	}
}

void StateShipIdle::OnExit(Ship& cur, int to)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void StateShipBlink::OnEnter(Ship& cur, int from)
{
}

void StateShipBlink::OnExecute(Ship& cur)
{
	float v = fmod(cur.GetFSM()->totalTime, 0.2f);
	if ( v<0.1f )
	{
		cur.GetEntity()->visible = true;
	}
	else
	{
		cur.GetEntity()->visible = false;
	}

	if ( cur.GetFSM()->totalTime>1.0f )
	{
		cur.GetFSM()->ToState(CPU_ID(StateShipIdle));
		return;
	}
}

void StateShipBlink::OnExit(Ship& cur, int to)
{
}
