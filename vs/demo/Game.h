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
	FONT m_font;
	MESH m_meshShip;
	MESH m_meshCube;
	MATERIAL m_materialShip;
	TEXTURE m_texture;
	SPRITE* m_pSprite;
	ENTITY* m_pShip;
	std::list<ENTITY*> m_missiles;

	float m_missileSpeed;
};
