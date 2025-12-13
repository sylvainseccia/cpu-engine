#include "stdafx.h"

Game::Game()
{
	m_pShip = nullptr;
	m_pSprite = nullptr;
}

Game::~Game()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Game::OnStart()
{
	// YOUR CODE HERE

	m_font.Create("Consolas", 24);
	m_texture.Load("bird.png");
	m_pSprite = CreateSprite();
	m_pSprite->pTexture = &m_texture;
	m_pSprite->CenterAnchor();
	m_pSprite->x = 100;
	m_pSprite->y = 100;

	CreateSpaceship(m_meshShip);
	CreateCube(m_meshCube);

	m_materialShip.color = ToColor(255, 128, 0);

	m_missileSpeed = 10.0f;

	m_pShip = CreateEntity();
	m_pShip->pMesh = &m_meshShip;
	m_pShip->transform.pos.z = 5.0f;
	m_pShip->transform.pos.y = -3.0f;
	m_pShip->pMaterial = &m_materialShip;

	m_camera.transform.pos.z = -5.0f;
}

void Game::OnUpdate()
{
	// YOUR CODE HERE

	// Bouge le sprite
	m_pSprite->y = 100 + (int)(sinf(m_time)*50.0f);

	// Tourne sur XYZ
	m_pShip->transform.AddYPR(m_dt, m_dt, m_dt);

	// Eloigne le vaisseau
	m_pShip->transform.pos.z += m_dt * 1.0f;

	// Camera
	m_camera.transform.AddYPR(0.0f, 0.0f, m_dt*0.1f);

	// Missiles
	for ( auto it=m_missiles.begin() ; it!=m_missiles.end() ; )
	{
		ENTITY* pMissile = *it;
		pMissile->transform.Move(m_dt*m_missileSpeed);
		if ( pMissile->lifetime>10.0f )
		{
			it = m_missiles.erase(it);
			ReleaseEntity(pMissile);
		}
		else
			++it;
	}

	// Fire
	if ( m_keyboard.IsKey(VK_SPACE) )
	{
		ENTITY* pMissile = CreateEntity();
		pMissile->pMesh = &m_meshCube;
		pMissile->transform.SetScaling(0.2f);
		pMissile->transform.pos = m_pShip->transform.pos;
		pMissile->transform.SetRotation(m_pShip->transform);
		pMissile->transform.Move(1.5f);
		m_missiles.push_back(pMissile);
	}
	if ( m_keyboard.IsKeyDown(VK_LBUTTON) )
	{
		XMFLOAT2 pt;
		GetCursor(pt);
		RAY ray = GetCameraRay(pt);
		ENTITY* pMissile = CreateEntity();
		pMissile->pMesh = &m_meshCube;
		pMissile->transform.SetScaling(0.2f);
		pMissile->transform.pos = ray.pos;
		pMissile->transform.LookTo(ray.dir);
		m_missiles.push_back(pMissile);
	}
}

void Game::OnExit()
{
	// YOUR CODE HERE

	m_pShip = nullptr;
	m_missiles.clear();
}

void Game::OnPreRender()
{
	// YOUR CODE HERE
}

void Game::OnPostRender()
{
	// YOUR CODE HERE

	// Debug
	std::string info = std::to_string(m_fps) + " fps, ";
	info += std::to_string(m_missiles.size()) + " missiles, ";
	info += std::to_string(m_clipEntityCount) + " clipped entities, ";
	info += std::to_string(m_threadCount) + " threads, ";
	info += std::to_string(m_tileCount) + " tiles ";
	info += "(FIRE: space or left button)";
	DrawText(&m_font, info.c_str(), 10, 10);
}
