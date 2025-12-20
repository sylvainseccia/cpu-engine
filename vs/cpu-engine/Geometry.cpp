#include "stdafx.h"

cpu_vertex::cpu_vertex()
{
	Identity();
}

void cpu_vertex::Identity()
{
	pos = CPU_ZERO;
	color = CPU_WHITE;
	normal = { 0.0f, 0.0f, 1.0f };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_triangle::cpu_triangle()
{
	Identity();
}

void cpu_triangle::Identity()
{
	v[0].Identity();
	v[1].Identity();
	v[2].Identity();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_rectangle::cpu_rectangle()
{
	Zero();
}

void cpu_rectangle::Zero()
{
	min = { 0.0f, 0.0f };
	max = { 0.0f, 0.0f };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_aabb::cpu_aabb()
{
	Zero();
}

cpu_aabb& cpu_aabb::operator=(const cpu_obb& obb)
{
	min = {  FLT_MAX,  FLT_MAX,  FLT_MAX };
	max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for ( int i=0 ; i<8 ; ++i )
	{
		const auto& p = obb.pts[i];

		if ( p.x<min.x )
			min.x = p.x;
		if ( p.y<min.y )
			min.y = p.y;
		if ( p.z<min.z )
			min.z = p.z;

		if ( p.x>max.x )
			max.x = p.x;
		if ( p.y>max.y )
			max.y = p.y;
		if ( p.z>max.z )
			max.z = p.z;
	}
	return *this;
}

void cpu_aabb::Zero()
{
	min = { 0.0f, 0.0f, 0.0f };
	max = { 0.0f, 0.0f, 0.0f };
}

bool XM_CALLCONV cpu_aabb::ToScreen(cpu_rectangle& out, FXMMATRIX wvp, float renderWidth, float renderHeight)
{
	const float renderX = 0.0f;
	const float renderY = 0.0f;
	float minX =  FLT_MAX, minY =  FLT_MAX;
	float maxX = -FLT_MAX, maxY = -FLT_MAX;

	const float xmin = min.x;
	const float ymin = min.y;
	const float zmin = min.z;
	const float xmax = max.x;
	const float ymax = max.y;
	const float zmax = max.z;

	const XMVECTOR pts[8] =
	{
		XMVectorSet(xmin, ymin, zmin, 1.0f),
		XMVectorSet(xmax, ymin, zmin, 1.0f),
		XMVectorSet(xmax, ymax, zmin, 1.0f),
		XMVectorSet(xmin, ymax, zmin, 1.0f),
		XMVectorSet(xmin, ymin, zmax, 1.0f),
		XMVectorSet(xmax, ymin, zmax, 1.0f),
		XMVectorSet(xmax, ymax, zmax, 1.0f),
		XMVectorSet(xmin, ymax, zmax, 1.0f)
	};

	for ( int i=0 ; i<8 ; ++i )
	{
		XMVECTOR clip = XMVector4Transform(pts[i], wvp);

		float w = XMVectorGetW(clip);
		if ( w<=0.00001f )
			continue; // derrière caméra

		float invW = 1.0f / w;
		float ndcX = XMVectorGetX(clip) * invW;
		float ndcY = XMVectorGetY(clip) * invW;

		float sx = renderX + (ndcX * 0.5f + 0.5f) * renderWidth;
		float sy = renderY + (-ndcY * 0.5f + 0.5f) * renderHeight;

		minX = MIN(minX, sx);
		minY = MIN(minY, sy);
		maxX = MAX(maxX, sx);
		maxY = MAX(maxY, sy);
	}

	// Outside
	if ( minX>maxX || minY>maxY )
		return false;

	minX = Clamp(minX, renderX, renderX+renderWidth);
	maxX = Clamp(maxX, renderX, renderX+renderWidth);
	minY = Clamp(minY, renderY, renderY+renderHeight);
	maxY = Clamp(maxY, renderY, renderY+renderHeight);
	out.min = { minX, minY };
	out.max = { maxX, maxY };
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_obb::cpu_obb()
{
	Zero();
}

cpu_obb& cpu_obb::operator=(const cpu_aabb& aabb)
{
	// Bas (zmin) puis Haut (zmax), en tournant sur x/y
	const float xmin = aabb.min.x, ymin = aabb.min.y, zmin = aabb.min.z;
	const float xmax = aabb.max.x, ymax = aabb.max.y, zmax = aabb.max.z;
	pts[0] = { xmin, ymin, zmin };
	pts[1] = { xmax, ymin, zmin };
	pts[2] = { xmax, ymax, zmin };
	pts[3] = { xmin, ymax, zmin };
	pts[4] = { xmin, ymin, zmax };
	pts[5] = { xmax, ymin, zmax };
	pts[6] = { xmax, ymax, zmax };
	pts[7] = { xmin, ymax, zmax };
	return *this;
}

void cpu_obb::Zero()
{
	for ( int i=0 ; i<8 ; i++ )
		pts[i] = { 0.0f, 0.0f, 0.0f };
}

void XM_CALLCONV cpu_obb::Transform(FXMMATRIX m)
{
	for ( int i=0 ; i<8 ; ++i )
	{
		XMVECTOR p = XMLoadFloat3(&pts[i]);
		p = XMVector3TransformCoord(p, m);
		XMStoreFloat3(&pts[i], p);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_ray::cpu_ray()
{
	Identity();
}

void cpu_ray::Identity()
{
	pos = { 0.0f, 0.0f, 0.0f };
	dir = { 0.0f, 0.0f, 1.0f };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_mesh::cpu_mesh()
{
	Clear();
}

void cpu_mesh::Clear()
{
	triangles.clear();
	radius = 0.0f;
	aabb.Zero();
}

void cpu_mesh::AddTriangle(XMFLOAT3& a, XMFLOAT3& b, XMFLOAT3& c, XMFLOAT3& color)
{
	cpu_triangle t;
	t.v[0].pos = a;
	t.v[0].color = color;
	t.v[1].pos = b;
	t.v[1].color = color;
	t.v[2].pos = c;
	t.v[2].color = color;
	triangles.push_back(t);
}

void cpu_mesh::AddFace(XMFLOAT3& a, XMFLOAT3& b, XMFLOAT3& c, XMFLOAT3& d, XMFLOAT3& color)
{
	AddTriangle(a, b, c, color);
	AddTriangle(a, c, d, color);
}

void cpu_mesh::Optimize()
{
	CalculateNormals();
	CalculateBox();
}

void cpu_mesh::CalculateNormals()
{
	std::map<XMFLOAT3, XMVECTOR, VEC3_CMP> normalAccumulator;
	for ( cpu_triangle& t : triangles )
	{
		XMVECTOR p0 = XMLoadFloat3(&t.v[0].pos);
		XMVECTOR p1 = XMLoadFloat3(&t.v[1].pos);
		XMVECTOR p2 = XMLoadFloat3(&t.v[2].pos);

		XMVECTOR edge1 = XMVectorSubtract(p1, p0);
		XMVECTOR edge2 = XMVectorSubtract(p2, p0);
		XMVECTOR faceNormal = XMVector3Cross(edge1, edge2);
		
		if ( normalAccumulator.count(t.v[0].pos)==0 )
			normalAccumulator[t.v[0].pos] = XMVectorZero();
		if ( normalAccumulator.count(t.v[1].pos)==0 )
			normalAccumulator[t.v[1].pos] = XMVectorZero();
		if ( normalAccumulator.count(t.v[2].pos)==0 )
			normalAccumulator[t.v[2].pos] = XMVectorZero();

		normalAccumulator[t.v[0].pos] = XMVectorAdd(normalAccumulator[t.v[0].pos], faceNormal);
		normalAccumulator[t.v[1].pos] = XMVectorAdd(normalAccumulator[t.v[1].pos], faceNormal);
		normalAccumulator[t.v[2].pos] = XMVectorAdd(normalAccumulator[t.v[2].pos], faceNormal);
	}
	for ( cpu_triangle& t : triangles )
	{
		for ( int i=0 ; i<3 ; i++ )
		{
			XMVECTOR sumNormal = normalAccumulator[t.v[i].pos];
			sumNormal = XMVector3Normalize(sumNormal);
			XMStoreFloat3(&t.v[i].normal, sumNormal);
		}
	}
}

void cpu_mesh::CalculateBox()
{
	aabb.min.x = FLT_MAX;
	aabb.min.y = FLT_MAX;
	aabb.min.z = FLT_MAX;
	aabb.max.x = -FLT_MAX;
	aabb.max.y = -FLT_MAX;
	aabb.max.z = -FLT_MAX;

	for ( cpu_triangle& t : triangles )
	{
		for ( int i=0 ; i<3 ; i++ )
		{
			if ( t.v[i].pos.x<aabb.min.x )
				aabb.min.x = t.v[i].pos.x;
			if ( t.v[i].pos.y<aabb.min.y )
				aabb.min.y = t.v[i].pos.y;
			if ( t.v[i].pos.z<aabb.min.z )
				aabb.min.z = t.v[i].pos.z;
				
			if ( t.v[i].pos.x>aabb.max.x )
				aabb.max.x = t.v[i].pos.x;
			if ( t.v[i].pos.y>aabb.max.y )
				aabb.max.y = t.v[i].pos.y;
			if ( t.v[i].pos.z>aabb.max.z )
				aabb.max.z = t.v[i].pos.z;
		}
	}

	float fx = MAX(fabsf(aabb.min.x), fabsf(aabb.max.x));
	float fy = MAX(fabsf(aabb.min.y), fabsf(aabb.max.y));
	float fz = MAX(fabsf(aabb.min.z), fabsf(aabb.max.z));
	float r2 = fx*fx + fy*fy + fz*fz;
	radius = std::sqrt(r2);
}

void cpu_mesh::CreateCube(float halfSize, XMFLOAT3 color)
{
	Clear();
	const float s = halfSize; 
	XMFLOAT3 p0 = { -s, -s, -s };
	XMFLOAT3 p1 = {  s, -s, -s };
	XMFLOAT3 p2 = {  s,  s, -s };
	XMFLOAT3 p3 = { -s,  s, -s };
	XMFLOAT3 p4 = { -s, -s,  s };
	XMFLOAT3 p5 = {  s, -s,  s };
	XMFLOAT3 p6 = {  s,  s,  s };
	XMFLOAT3 p7 = { -s,  s,  s };
	AddFace(p0, p1, p2, p3, color);					// -Z (back)
	AddFace(p4, p7, p6, p5, color);					// +Z (front)
	AddFace(p1, p5, p6, p2, color);					// +X (droite)
	AddFace(p4, p0, p3, p7, color);					// -X (gauche)
	AddFace(p3, p2, p6, p7, color);					// +Y (haut)
	AddFace(p4, p5, p1, p0, color);					// -Y (bas)
	Optimize();
}

void cpu_mesh::CreateCircle(float radius, int count, XMFLOAT3 color)
{
	if ( count<3 )
		return;

	Clear();
	float step = XM_2PI / count;
	float angle = 0.0f;
	XMFLOAT3 p1, p2, p3;
	p1.x = 0.0f;
	p1.y = 0.0f;
	p1.z = 0.0f;
	p2.y = 0.0f;
	p3.y = 0.0f;
	for ( int i=0 ; i<count ; i++ )
	{
		p2.x = cosf(angle) * radius;
		p2.z = sinf(angle) * radius;
		p3.x = cosf(angle+step) * radius;
		p3.z = sinf(angle+step) * radius;
		AddTriangle(p1, p2, p3, color);
		angle += step;
	}
	Optimize();
}

void cpu_mesh::CreateSphere(float radius, int stacks, int slices, XMFLOAT3 color1, XMFLOAT3 color2)
{
	Clear();
	if ( stacks<2 ) stacks = 2; // minimum pour avoir un haut et un bas
	if ( slices<3 ) slices = 3; // minimum pour fermer correctement
	for ( int i=0 ; i<stacks ; ++i )
	{
		// theta0/theta1 délimitent une bande (du haut vers le bas)
		const float v0 = (float)i / (float)stacks;
		const float v1 = (float)(i + 1) / (float)stacks;
		const float theta0 = v0 * XM_PI;
		const float theta1 = v1 * XM_PI;
		for ( int j=0,k=0 ; j<slices ; ++j )
		{
			const float u0 = (float)j / (float)slices;
			const float u1 = (float)(j + 1) / (float)slices;
			const float phi0 = u0 * XM_2PI;
			const float phi1 = u1 * XM_2PI;

			// 4 coins du quad (bande i, secteur j)
			// p00 = (theta0, phi0)
			// p01 = (theta0, phi1)
			// p10 = (theta1, phi0)
			// p11 = (theta1, phi1)
			XMFLOAT3 p00 = SphericalPoint(radius, theta0, phi0);
			XMFLOAT3 p01 = SphericalPoint(radius, theta0, phi1);
			XMFLOAT3 p10 = SphericalPoint(radius, theta1, phi0);
			XMFLOAT3 p11 = SphericalPoint(radius, theta1, phi1);
			XMFLOAT3& color = k==0 ? color1 : color2;

			// Triangles dégénérés (theta = 0 ou PI)
			const bool topBand = i==0;
			const bool bottomBand = i==stacks-1;
			if ( topBand )
			{
				// Au pôle nord, p00 et p01 sont quasiment identiques (theta0=0).
				// Triangle orienté vers l'extérieur (CCW vu de l'extérieur).
				// On utilise: p00 (sommet), p10 (bas gauche), p11 (bas droite)
				AddTriangle(p00, p10, p11, color);
			}
			else if ( bottomBand )
			{
				// Au pôle sud, p10 et p11 sont quasiment identiques (theta1=PI).
				// On utilise: p10 (sommet bas), p01 (haut droite), p00 (haut gauche)
				AddTriangle(p10, p01, p00, color);
			}
			else
			{
				// Bande intermédiaire : 2 triangles pour le quad
				// Winding CCW (vu depuis l'extérieur)
				AddTriangle(p00, p10, p11, const_cast<XMFLOAT3&>(color));
				AddTriangle(p00, p11, p01, const_cast<XMFLOAT3&>(color));
			}

			if ( ++k==2 )
				k = 0;
		}
	}
	Optimize();
}

void cpu_mesh::CreateSpaceship()
{
	Clear();
	const float width = 2.0f;
	XMFLOAT3 nose = { 0.0f, 0.0f, 1.5f };
	XMFLOAT3 rTop = { 0.0f, 0.5f, -1.0f };
	XMFLOAT3 rBot = { 0.0f, -0.3f, -1.0f };
	XMFLOAT3 wLeft = { -width*0.5f, 0.0f, -1.0f };
	XMFLOAT3 wRight = { width*0.5f, 0.0f, -1.0f };

	XMFLOAT3 c1 = ToColor(208, 208, 208);
	XMFLOAT3 c2 = ToColor(192, 192, 192);
	XMFLOAT3 c3 = ToColor(112, 112, 112);
	XMFLOAT3 c4 = ToColor(96, 96, 96);
	XMFLOAT3 c5 = ToColor(255, 255, 255);
	XMFLOAT3 c6 = ToColor(255, 255, 255);

	AddTriangle(nose, wLeft, rTop, c1);		// Avant gauche haut
	AddTriangle(nose, rTop, wRight, c2);	// Avant droit haut
	AddTriangle(nose, rBot, wLeft, c3);		// Avant gauche bas
	AddTriangle(nose, wRight, rBot, c4);	// Avant droit bas
	AddTriangle(wLeft, rBot, rTop, c5);		// Moteur gauche
	AddTriangle(wRight, rTop, rBot, c6);	// Moteur droit
	Optimize();
}
