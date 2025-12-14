#include "stdafx.h"

VERTEX::VERTEX()
{
	Identity();
}

void VERTEX::Identity()
{
	pos = { 0.0f, 0.0f, 0.0f };
	color = { 1.0f, 1.0f, 1.0f };
	normal = { 0.0f, 0.0f, 1.0f };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TRIANGLE::TRIANGLE()
{
	Identity();
}

void TRIANGLE::Identity()
{
	v[0].Identity();
	v[1].Identity();
	v[2].Identity();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RECTANGLE::RECTANGLE()
{
	Zero();
}

void RECTANGLE::Zero()
{
	min = { 0.0f, 0.0f };
	max = { 0.0f, 0.0f };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AABB::AABB()
{
	Zero();
}

AABB& AABB::operator=(const OBB& obb)
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

void AABB::Zero()
{
	min = { 0.0f, 0.0f, 0.0f };
	max = { 0.0f, 0.0f, 0.0f };
}

bool XM_CALLCONV AABB::ToScreen(RECTANGLE& out, FXMMATRIX wvp, float renderWidth, float renderHeight)
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

		minX = std::min(minX, sx);
		minY = std::min(minY, sy);
		maxX = std::max(maxX, sx);
		maxY = std::max(maxY, sy);
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

OBB::OBB()
{
	Zero();
}

OBB& OBB::operator=(const AABB& aabb)
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

void OBB::Zero()
{
	for ( int i=0 ; i<8 ; i++ )
		pts[i] = { 0.0f, 0.0f, 0.0f };
}

void XM_CALLCONV OBB::Transform(FXMMATRIX m)
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

RAY::RAY()
{
	Identity();
}

void RAY::Identity()
{
	pos = { 0.0f, 0.0f, 0.0f };
	dir = { 0.0f, 0.0f, 1.0f };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MESH::MESH()
{
	Clear();
}

void MESH::Clear()
{
	triangles.clear();
	radius = 0.0f;
	aabb.Zero();
}

void MESH::AddTriangle(XMFLOAT3& a, XMFLOAT3& b, XMFLOAT3& c, XMFLOAT3& color)
{
	TRIANGLE t;
	t.v[0].pos = a;
	t.v[0].color = color;
	t.v[1].pos = b;
	t.v[1].color = color;
	t.v[2].pos = c;
	t.v[2].color = color;
	triangles.push_back(t);
}

void MESH::AddFace(XMFLOAT3& a, XMFLOAT3& b, XMFLOAT3& c, XMFLOAT3& d, XMFLOAT3& color)
{
	AddTriangle(a, c, b, color);
	AddTriangle(a, d, c, color);
}

void MESH::Optimize()
{
	CalculateNormals();
	CalculateBox();
}

void MESH::CalculateNormals()
{
	std::map<XMFLOAT3, XMVECTOR, VEC3_CMP> normalAccumulator;
	for ( TRIANGLE& t : triangles )
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

	for ( TRIANGLE& t : triangles )
	{
		for ( int i=0 ; i<3 ; i++ )
		{
			XMVECTOR sumNormal = normalAccumulator[t.v[i].pos];
			sumNormal = XMVector3Normalize(sumNormal);
			XMStoreFloat3(&t.v[i].normal, sumNormal);
		}
	}
}

void MESH::CalculateBox()
{
	aabb.min.x = FLT_MAX;
	aabb.min.y = FLT_MAX;
	aabb.min.z = FLT_MAX;
	aabb.max.x = -FLT_MAX;
	aabb.max.y = -FLT_MAX;
	aabb.max.z = -FLT_MAX;

	for ( TRIANGLE& t : triangles )
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

	float fx = std::max(std::abs(aabb.min.x), std::abs(aabb.max.x));
	float fy = std::max(std::abs(aabb.min.y), std::abs(aabb.max.y));
	float fz = std::max(std::abs(aabb.min.z), std::abs(aabb.max.z));
	float r2 = fx*fx + fy*fy + fz*fz;
	radius = std::sqrt(r2);
}


void MESH::CreateSpaceship()
{
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

	AddTriangle(nose, rTop, wLeft, c1);					// Avant Gauche haut
	AddTriangle(nose, wRight, rTop, c2);				// Avant Droit haut
	AddTriangle(nose, wLeft, rBot, c3);					// Avant Gauche bas
	AddTriangle(nose, rBot, wRight, c4);				// Avant Droit bas
	AddTriangle(wLeft, rTop, rBot, c5);					// Moteur Gauche
	AddTriangle(wRight, rBot, rTop, c6);				// Moteur Droit
	Optimize();
}

void MESH::CreateCube()
{
	const float s = 0.5f; 
	XMFLOAT3 p0 = { -s, -s, -s };							// Avant Bas Gauche
	XMFLOAT3 p1 = {  s, -s, -s };							// Avant Bas Droite
	XMFLOAT3 p2 = {  s,  s, -s };							// Avant Haut Droite
	XMFLOAT3 p3 = { -s,  s, -s };							// Avant Haut Gauche
	XMFLOAT3 p4 = { -s, -s,  s };							// Arrière Bas Gauche
	XMFLOAT3 p5 = {  s, -s,  s };							// Arrière Bas Droite
	XMFLOAT3 p6 = {  s,  s,  s };							// Arrière Haut Droite
	XMFLOAT3 p7 = { -s,  s,  s };							// Arrière Haut Gauche
	
	XMFLOAT3 c1 = ToColor(255, 255, 255);
	AddFace(p0, p1, p2, p3, c1);							// Face Avant (Z = -0.5)
	AddFace(p5, p4, p7, p6, c1);							// Face Arrière (Z = +0.5)
	AddFace(p1, p5, p6, p2, c1);							// Face Droite (X = +0.5)
	AddFace(p4, p0, p3, p7, c1);							// Face Gauche (X = -0.5)
	AddFace(p3, p2, p6, p7, c1);							// Face Haut (Y = +0.5)
	AddFace(p4, p5, p1, p0, c1);							// Face Bas (Y = -0.5)
	Optimize();
}

void MESH::CreateCircle(float radius, int count)
{
	if ( count<3 )
		return;

	XMFLOAT3 c1 = ToColor(255, 255, 255);
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
		AddTriangle(p1, p2, p3, c1);
		angle += step;
	}
	Optimize();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MATERIAL::MATERIAL()
{
	//lighting = UNLIT;
	lighting = GOURAUD;
	//lighting = LAMBERT;
	ps = nullptr;
	color = { 1.0f, 1.0f, 1.0f };
	data = nullptr;
}
