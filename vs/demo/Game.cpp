#include "stdafx.h"

Game::Game()
{
	m_pShip = nullptr;
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

	CreateSpaceship(m_meshShip);
	CreateCube(m_meshCube);

	m_missileSpeed = 10.0f;

	m_pShip = CreateEntity();
	m_pShip->pMesh = &m_meshShip;
	m_pShip->transform.pos.z = 10.0f;
	m_pShip->transform.pos.y = -3.0f;
	m_pShip->material = ToColor(255, 128, 0);

	m_camera.pos.z = -5.0f;

	// Debug
	if ( false )
	{
		for ( int i=0 ; i<1000 ; i++ )
		{
			ENTITY* pEntity = CreateEntity();
			pEntity->pMesh = &m_meshShip;
			pEntity->transform.pos.x = i*0.1f;
			pEntity->transform.pos.y = i*0.1f;
			pEntity->transform.pos.z = i*0.1f;
		}
	}
}

void Game::OnUpdate()
{
	// YOUR CODE HERE

	// Tourne sur XYZ
	m_pShip->transform.AddYPR(m_dt, m_dt, m_dt);

	// Eloigne le vaisseau
	m_pShip->transform.pos.z += m_dt * 1.0f;

	// Camera
	m_camera.AddYPR(0.0f, 0.0f, m_dt*0.1f);
	//m_camera.AddYPR(0.0f, m_dt);

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
	if ( GetAsyncKeyState(VK_SPACE)<0 )
	{
		ENTITY* pMissile = CreateEntity();
		pMissile->pMesh = &m_meshCube;
		pMissile->transform.SetScaling(0.2f);
		pMissile->transform.pos = m_pShip->transform.pos;
		pMissile->transform.SetRotation(m_pShip->transform);
		pMissile->transform.Move(1.5f);
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
	info += std::to_string(m_missiles.size()) + " missiles";
	SetWindowText(m_hWnd, info.c_str());
}
