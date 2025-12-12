#pragma once

class Game : public Engine
{
public:
	Game();
	virtual ~Game();

	void OnStart() override;
	void OnUpdate() override;
	void OnExit() override;
	void OnPreRender() override;
	void OnPostRender() override;

protected:
	MESH m_meshShip;
	MESH m_meshCube;
	ENTITY* m_pShip;
	std::list<ENTITY*> m_missiles;

	float m_missileSpeed;
};
