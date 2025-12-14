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

	// Resources
	m_font.Create(18);
	m_texture.Load("bird.png");
	m_meshShip.CreateSpaceship();
	m_meshCube.CreateCube();

	// UI
	m_pSprite = CreateSprite();
	m_pSprite->pTexture = &m_texture;
	m_pSprite->CenterAnchor();
	m_pSprite->x = 100;
	m_pSprite->y = 100;

	// Shader
	m_materialShip.color = ToColor(255, 128, 0);
	m_materialMissile.ps = MyPixelShader;

	// 3D
	m_camera.transform.pos.z = -5.0f;
	m_pShip = CreateEntity();
	m_pShip->pMesh = &m_meshShip;
	m_pShip->transform.pos.z = 5.0f;
	m_pShip->transform.pos.y = -3.0f;
	m_pShip->pMaterial = &m_materialShip;
	m_missileSpeed = 10.0f;
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
	if ( m_input.IsKey(VK_SPACE) )
	{
		ENTITY* pMissile = CreateEntity();
		pMissile->pMesh = &m_meshCube;
		pMissile->transform.SetScaling(0.2f);
		pMissile->transform.pos = m_pShip->transform.pos;
		pMissile->transform.SetRotation(m_pShip->transform);
		pMissile->transform.Move(1.5f);
		m_missiles.push_back(pMissile);
	}
	if ( m_input.IsKeyDown(VK_LBUTTON) )
	{
		XMFLOAT2 pt;
		GetCursor(pt);
		RAY ray = GetCameraRay(pt);
		ENTITY* pMissile = CreateEntity();
		pMissile->pMesh = &m_meshCube;
		pMissile->transform.SetScaling(0.2f);
		pMissile->transform.pos = ray.pos;
		pMissile->transform.LookTo(ray.dir);
		pMissile->pMaterial = &m_materialMissile;
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
	info += std::to_string(m_statsClipEntityCount) + " clipped entities, ";
	info += std::to_string(m_statsThreadCount) + " threads, ";
	info += std::to_string(m_statsTileCount) + " tiles\n";
	info += "(FIRE: space or left button)";
	DrawText(&m_font, info.c_str(), GetWidth()/2, 10, CENTER);
}

void Game::MyPixelShader(PS_DATA& data)
{
	// garder seulement le rouge du pixel éclairé
	data.out.x = data.in.color.x;
}
