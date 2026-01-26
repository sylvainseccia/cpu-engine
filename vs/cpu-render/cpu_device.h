#pragma once

class cpu_device
{
public:
	cpu_device();
	virtual ~cpu_device();

	bool Create(cpu_window* pWindow, int width, int height);
	void Destroy();
	void Fix();

	void SetDefaultCamera();
	void SetCamera(cpu_camera* pCamera);
	void UpdateCamera();

	void SetDefaultLight();
	void SetLight(cpu_light* pLight);

	int GetWidth() { return m_mainRT.width; }
	int GetHeight() { return m_mainRT.height; }
	RECT& GetFit() { return m_rcFit; }
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
	void ClearSky(XMFLOAT3& groundColor, XMFLOAT3& skyColor);
	void ClearDepth();

	void DrawMesh(cpu_mesh* pMesh, cpu_transform* pTransform, cpu_material* pMaterial, int depthMode = CPU_DEPTH_RW, cpu_tile* pTile = nullptr);
	void XM_CALLCONV DrawWireframeMesh(cpu_mesh* pMesh, FXMMATRIX matrix, cpu_tile* pTile = nullptr);
	void DrawText(cpu_font* pFont, const char* text, int x, int y, int align = CPU_TEXT_LEFT);
	void DrawTexture(cpu_texture* pTexture, int x, int y);
	void DrawSprite(cpu_sprite* pSprite);
	void DrawHorzLine(int x1, int x2, int y, XMFLOAT3& color);
	void DrawVertLine(int y1, int y2, int x, XMFLOAT3& color);
	void DrawRectangle(int x, int y, int w, int h, XMFLOAT3& color);
	void DrawLine(int x0, int y0, float z0, int x1, int y1, float z1, XMFLOAT3& color);

	void Present();

private:
	void OnWindowCallback(UINT message, WPARAM wParam, LPARAM lParam);
	bool ClipToScreen(cpu_draw& draw);
	void DrawTriangle(cpu_draw& draw);
	bool WireframeClipLineNearPlane(XMFLOAT4& a, XMFLOAT4& b);
	bool WireframeClipToScreen(const XMFLOAT4& c, float widthHalf, float heightHalf, XMFLOAT3& out);
	inline float PlaneEval(const XMFLOAT4& p, const XMFLOAT4& c);
	int ClipPolyAgainstPlane(const cpu_vertex_out* pInV, int inCount, cpu_vertex_out* pOutV, const XMFLOAT4& plane);
	int ClipTriangleFrustum(const cpu_vertex_out tri[3], cpu_vertex_out outV[8]);
	static void PixelShader(cpu_ps_io& io);

private:
	// Render
	bool m_created;
	int m_width;
	int m_height;
	RECT m_rcFit;
	cpu_window* m_pWindow;

	// Surface
#ifdef CPU_CONFIG_GPU
	ID2D1Factory* m_pD2DFactory;
	ID2D1HwndRenderTarget* m_pRenderTarget;
	ID2D1Bitmap* m_pBitmap;
#else
	BITMAPINFO m_bi;
#endif

	// Buffer
	cpu_rt m_mainRT;
	cpu_rt* m_pRT;

	// Camera
	bool m_cullFrontCCW = false; // DirectX default
	float m_cullAreaEpsilon = CPU_EPSILON;
	cpu_camera m_defaultCamera;
	cpu_camera* m_pCamera;

	// Material
	cpu_material m_defaultMaterial;

	// Light
	cpu_light m_defaultLight;
	cpu_light* m_pLight;
};
