#include "pch.h"

cpu_mesh::cpu_mesh()
{
	Clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_mesh::Clear()
{
	vertices.clear();
	radius = 0.0f;
	aabb.Zero();
	obb.Zero();
}

int cpu_mesh::GetTriangleCount()
{
	return (int)(vertices.size()/3);
}

void cpu_mesh::AddMesh(cpu_mesh& mesh)
{
	for ( cpu_vertex& v : mesh.vertices )
		vertices.push_back(v);
}

void cpu_mesh::AddTriangle(cpu_triangle& tri)
{
	vertices.push_back(tri.v[0]);
	vertices.push_back(tri.v[1]);
	vertices.push_back(tri.v[2]);
}

void cpu_mesh::AddTriangle(XMFLOAT3& a, XMFLOAT3& b, XMFLOAT3& c, XMFLOAT3& color)
{
	cpu_vertex v;
	v.pos = a;
	v.color = color;
	vertices.push_back(v);
	v.pos = b;
	v.color = color;
	vertices.push_back(v);
	v.pos = c;
	v.color = color;
	vertices.push_back(v);
}

void cpu_mesh::AddTriangle(XMFLOAT3& a, XMFLOAT3& b, XMFLOAT3& c, XMFLOAT2& auv, XMFLOAT2& buv, XMFLOAT2& cuv, XMFLOAT3& color)
{
	cpu_vertex v;
	v.pos = a;
	v.color = color;
	v.uv = auv;
	vertices.push_back(v);
	v.pos = b;
	v.color = color;
	v.uv = buv;
	vertices.push_back(v);
	v.pos = c;
	v.color = color;
	v.uv = cuv;
	vertices.push_back(v);
}

void cpu_mesh::AddFace(XMFLOAT3& a, XMFLOAT3& b, XMFLOAT3& c, XMFLOAT3& d, XMFLOAT3& color)
{
	AddTriangle(a, b, c, color);
	AddTriangle(a, c, d, color);
}

void cpu_mesh::AddFace(XMFLOAT3& a, XMFLOAT3& b, XMFLOAT3& c, XMFLOAT3& d, XMFLOAT2& auv, XMFLOAT2& buv, XMFLOAT2& cuv, XMFLOAT2& duv, XMFLOAT3& color)
{
	AddTriangle(a, b, c, auv, buv, cuv, color);
	AddTriangle(a, c, d, auv, cuv, duv, color);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_mesh::Optimize()
{
	CalculateNormals();
	CalculateBoundingVolumes();
}

void cpu_mesh::CalculateNormals()
{
	std::map<XMFLOAT3, XMVECTOR, cpu_vec3_cmp> normalAccumulator;
	for ( size_t i=0 ; i<vertices.size() ; i+=3 )
	{
		XMVECTOR p0 = XMLoadFloat3(&vertices[i+0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[i+1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[i+2].pos);

		XMVECTOR edge1 = XMVectorSubtract(p1, p0);
		XMVECTOR edge2 = XMVectorSubtract(p2, p0);
		XMVECTOR faceNormal = XMVector3Cross(edge1, edge2);
		
		if ( normalAccumulator.count(vertices[i+0].pos)==0 )
			normalAccumulator[vertices[i+0].pos] = XMVectorZero();
		if ( normalAccumulator.count(vertices[i+1].pos)==0 )
			normalAccumulator[vertices[i+1].pos] = XMVectorZero();
		if ( normalAccumulator.count(vertices[i+2].pos)==0 )
			normalAccumulator[vertices[i+2].pos] = XMVectorZero();

		normalAccumulator[vertices[i+0].pos] = XMVectorAdd(normalAccumulator[vertices[i+0].pos], faceNormal);
		normalAccumulator[vertices[i+1].pos] = XMVectorAdd(normalAccumulator[vertices[i+1].pos], faceNormal);
		normalAccumulator[vertices[i+2].pos] = XMVectorAdd(normalAccumulator[vertices[i+2].pos], faceNormal);
	}
	for ( cpu_vertex& v : vertices )
	{
		XMVECTOR sumNormal = normalAccumulator[v.pos];
		sumNormal = XMVector3Normalize(sumNormal);
		XMStoreFloat3(&v.normal, sumNormal);
	}
}

void cpu_mesh::CalculateBoundingVolumes()
{
	aabb.min.x = FLT_MAX;
	aabb.min.y = FLT_MAX;
	aabb.min.z = FLT_MAX;
	aabb.max.x = -FLT_MAX;
	aabb.max.y = -FLT_MAX;
	aabb.max.z = -FLT_MAX;

	for ( cpu_vertex& v : vertices )
	{
		if ( v.pos.x<aabb.min.x )
			aabb.min.x = v.pos.x;
		if ( v.pos.y<aabb.min.y )
			aabb.min.y = v.pos.y;
		if ( v.pos.z<aabb.min.z )
			aabb.min.z = v.pos.z;
				
		if ( v.pos.x>aabb.max.x )
			aabb.max.x = v.pos.x;
		if ( v.pos.y>aabb.max.y )
			aabb.max.y = v.pos.y;
		if ( v.pos.z>aabb.max.z )
			aabb.max.z = v.pos.z;
	}

	float fx = std::max(fabsf(aabb.min.x), fabsf(aabb.max.x));
	float fy = std::max(fabsf(aabb.min.y), fabsf(aabb.max.y));
	float fz = std::max(fabsf(aabb.min.z), fabsf(aabb.max.z));
	float r2 = fx*fx + fy*fy + fz*fz;
	radius = sqrtf(r2);

	obb = aabb;
}

void XM_CALLCONV cpu_mesh::Transform(FXMMATRIX matrix)
{
	for ( cpu_vertex& v : vertices )
	{
		XMVECTOR vec = XMVector3TransformCoord(XMLoadFloat3(&v.pos), matrix);
		XMStoreFloat3(&v.pos, vec);
	}

	cpu_obb old = obb;
	Optimize();
	obb = old;
	obb.Transform(matrix);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_mesh::CreatePlane(float width, float height, XMFLOAT3 color)
{
	Clear();
	const float w = width*0.5f;
	const float h = height*0.5f;
	XMFLOAT3 p0 = { -w, -h, 0.0f };
	XMFLOAT3 p1 = {  w, -h, 0.0f };
	XMFLOAT3 p2 = {  w,  h, 0.0f };
	XMFLOAT3 p3 = { -w,  h, 0.0f };
	XMFLOAT2 t0 = { 0.0f, 0.0f };
	XMFLOAT2 t1 = { 1.0f, 0.0f };
	XMFLOAT2 t2 = { 1.0f, 1.0f };
	XMFLOAT2 t3 = { 0.0f, 1.0f };
	AddFace(p0, p1, p2, p3, t0, t1, t2, t3, color);
	Optimize();
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
	XMFLOAT2 tl = { 0.0f, 0.0f };					// top-left
	XMFLOAT2 tr = { 1.0f, 0.0f };					// top-right
	XMFLOAT2 br = { 1.0f, 1.0f };					// bottom-right
	XMFLOAT2 bl = { 0.0f, 1.0f };					// bottom-left
	AddFace(p0, p1, p2, p3, bl, br, tr, tl, color);	// -Z (back)
	AddFace(p4, p7, p6, p5, bl, tl, tr, br, color);	// +Z (front)
	AddFace(p1, p5, p6, p2, bl, br, tr, tl, color);	// +X (droite)
	AddFace(p4, p0, p3, p7, bl, br, tr, tl, color);	// -X (gauche)
	AddFace(p3, p2, p6, p7, bl, br, tr, tl, color);	// +Y (haut)
	AddFace(p4, p5, p1, p0, bl, br, tr, tl, color);	// -Y (bas)
	Optimize();
}

void cpu_mesh::CreateSkyBox(float halfSize, XMFLOAT3 color)
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
	XMFLOAT2 tl = { 0.0f, 0.0f };					// top-left
	XMFLOAT2 tr = { 1.0f, 0.0f };					// top-right
	XMFLOAT2 br = { 1.0f, 1.0f };					// bottom-right
	XMFLOAT2 bl = { 0.0f, 1.0f };					// bottom-left
	AddFace(p3, p2, p1, p0, tl, tr, br, bl, color); // -Z
	AddFace(p5, p6, p7, p4, br, tr, tl, bl, color); // +Z
	AddFace(p2, p6, p5, p1, tl, tr, br, bl, color); // +X
	AddFace(p7, p3, p0, p4, tl, tr, br, bl, color); // -X
	AddFace(p7, p6, p2, p3, tl, tr, br, bl, color); // +Y
	AddFace(p0, p1, p5, p4, tl, tr, br, bl, color); // -Y
	Optimize();
}

void cpu_mesh::CreateCircle(float radius, int count, XMFLOAT3 color)
{
	if ( count<3 )
		return;

	Clear();
	float radius2 = radius * 2.0f;
	float step = XM_2PI / count;
	float angle = 0.0f;
	XMFLOAT3 p1, p2, p3;
	p1.x = 0.0f;
	p1.y = 0.0f;
	p1.z = 0.0f;
	p2.y = 0.0f;
	p3.y = 0.0f;
	XMFLOAT2 t1, t2, t3;
	t1.x = 0.5f;
	t1.y = 0.5f;
	for ( int i=0 ; i<count ; i++ )
	{
		p2.x = cosf(angle) * radius;
		p2.z = sinf(angle) * radius;
		p3.x = cosf(angle+step) * radius;
		p3.z = sinf(angle+step) * radius;
		t2.x = 0.5f + (p2.x / radius2);
		t2.y = 0.5f - (p2.z / radius2);
		t3.x = 0.5f + (p3.x / radius2);
		t3.y = 0.5f - (p3.z / radius2);
		AddTriangle(p1, p2, p3, t1, t2, t3, color);
		angle += step;
	}
	Optimize();
}

void cpu_mesh::CreateCylinder(float halfHeight, float radius, int count, bool top, bool bottom, XMFLOAT3 color)
{
	if ( count<3 )
		return;

	Clear();
	float radius2 = radius * 2.0f;
	float step = XM_2PI / count;
	float angle = 0.0f;
	XMFLOAT3 a1, b1, c1, a2, b2, c2;
	a1 = b1 = c1 = a2 = b2 = c2 = {};
	a1.y = b1.y = c1.y = halfHeight;
	a2.y = b2.y = c2.y = -halfHeight;
	XMFLOAT2 at, bt, ct;
	at.x = 0.5f;
	at.y = 0.5f;
	for ( int i=0 ; i<count ; i++ )
	{
		b1.x = cosf(angle) * radius;
		b1.z = sinf(angle) * radius;
		c1.x = cosf(angle+step) * radius;
		c1.z = sinf(angle+step) * radius;
		bt.x = 0.5f + (b1.x / radius2);
		bt.y = 0.5f - (b1.z / radius2);
		ct.x = 0.5f + (c1.x / radius2);
		ct.y = 0.5f - (c1.z / radius2);
		if ( top )
			AddTriangle(a1, b1, c1, at, bt, ct, color);
		
		b2.x = cosf(angle) * radius;
		b2.z = sinf(angle) * radius;
		c2.x = cosf(angle+step) * radius;
		c2.z = sinf(angle+step) * radius;
		bt.x = 0.5f + (b2.x / radius2);
		bt.y = 0.5f - (b2.z / radius2);
		ct.x = 0.5f + (c2.x / radius2);
		ct.y = 0.5f - (c2.z / radius2);
		if ( bottom )
			AddTriangle(a2, c2, b2, at, ct, bt, color);

		XMFLOAT2 bt2 = { bt.x, 1.0f };
		XMFLOAT2 ct2 = { ct.x, 1.0f };
		AddTriangle(b1, b2, c1, bt, bt2, ct, color);
		AddTriangle(c1, b2, c2, ct, bt2, ct2, color);

		angle += step;
	}
	Optimize();
}

void cpu_mesh::CreateTube(float halfHeight, float radius, int count, XMFLOAT3 color)
{
	if ( count<3 )
		return;

	Clear();
	float radius2 = radius * 2.0f;
	float step = XM_2PI / count;
	float angle = 0.0f;
	XMFLOAT3 a1, b1, c1, a2, b2, c2;
	a1 = b1 = c1 = a2 = b2 = c2 = {};
	a1.y = b1.y = c1.y = halfHeight;
	a2.y = b2.y = c2.y = -halfHeight;
	XMFLOAT2 at, bt, ct;
	at.x = 0.5f;
	at.y = 0.5f;
	for ( int i=0 ; i<count ; i++ )
	{
		b1.x = cosf(angle) * radius;
		b1.z = sinf(angle) * radius;
		c1.x = cosf(angle+step) * radius;
		c1.z = sinf(angle+step) * radius;
		bt.x = 0.5f + (b1.x / radius2);
		bt.y = 0.5f - (b1.z / radius2);
		ct.x = 0.5f + (c1.x / radius2);
		ct.y = 0.5f - (c1.z / radius2);
		
		b2.x = cosf(angle) * radius;
		b2.z = sinf(angle) * radius;
		c2.x = cosf(angle+step) * radius;
		c2.z = sinf(angle+step) * radius;
		bt.x = 0.5f + (b2.x / radius2);
		bt.y = 0.5f - (b2.z / radius2);
		ct.x = 0.5f + (c2.x / radius2);
		ct.y = 0.5f - (c2.z / radius2);

		XMFLOAT2 bt2 = { bt.x, 1.0f };
		XMFLOAT2 ct2 = { ct.x, 1.0f };
		AddTriangle(b1, c1, b2, bt, ct, bt2, color);
		AddTriangle(c1, c2, b2, ct, ct2, bt2, color);

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
			XMFLOAT3 p00 = cpu::SphericalPoint(radius, theta0, phi0);
			XMFLOAT3 p01 = cpu::SphericalPoint(radius, theta0, phi1);
			XMFLOAT3 p10 = cpu::SphericalPoint(radius, theta1, phi0);
			XMFLOAT3 p11 = cpu::SphericalPoint(radius, theta1, phi1);
			XMFLOAT3& color = k==0 ? color1 : color2;

			XMFLOAT2 uv00 = { u0, v0 };
			XMFLOAT2 uv01 = { u1, v0 };
			XMFLOAT2 uv10 = { u0, v1 };
			XMFLOAT2 uv11 = { u1, v1 };

			// Triangles dégénérés (theta = 0 ou PI)
			const bool topBand = i==0;
			const bool bottomBand = i==stacks-1;
			if ( topBand )
			{
				// Au pôle nord, p00 et p01 sont quasiment identiques (theta0=0).
				// Triangle orienté vers l'extérieur (CCW vu de l'extérieur).
				// On utilise: p00 (sommet), p10 (bas gauche), p11 (bas droite)
				AddTriangle(p00, p10, p11, uv00, uv10, uv11, color);
			}
			else if ( bottomBand )
			{
				// Au pôle sud, p10 et p11 sont quasiment identiques (theta1=PI).
				// On utilise: p10 (sommet bas), p01 (haut droite), p00 (haut gauche)
				AddTriangle(p10, p01, p00, uv10, uv01, uv00, color);
			}
			else
			{
				// Bande intermédiaire : 2 triangles pour le quad
				// Winding CCW (vu depuis l'extérieur)
				AddTriangle(p00, p10, p11, uv00, uv10, uv11, const_cast<XMFLOAT3&>(color));
				AddTriangle(p00, p11, p01, uv00, uv11, uv01, const_cast<XMFLOAT3&>(color));
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

	XMFLOAT2 noseUV		= { 0.5f, 0.0f };
	XMFLOAT2 rTopUV		= { 0.5f, 0.4f };
	XMFLOAT2 rBotUV		= { 0.5f, 0.8f };
	XMFLOAT2 wLeftUV	= { 0.0f, 0.6f };
	XMFLOAT2 wRightUV	= { 1.0f, 0.6f };

	XMFLOAT3 c1 = cpu::ToColor(208, 208, 208);
	XMFLOAT3 c2 = cpu::ToColor(192, 192, 192);
	XMFLOAT3 c3 = cpu::ToColor(112, 112, 112);
	XMFLOAT3 c4 = cpu::ToColor(96, 96, 96);
	XMFLOAT3 c5 = cpu::ToColor(255, 255, 255);
	XMFLOAT3 c6 = cpu::ToColor(255, 255, 255);

	AddTriangle(nose, wLeft, rTop, noseUV, wLeftUV, rTopUV, c1);		// Avant gauche haut
	AddTriangle(nose, rTop, wRight, noseUV, rTopUV, wRightUV, c2);		// Avant droit haut
	AddTriangle(nose, rBot, wLeft, noseUV, rBotUV, wLeftUV, c3);		// Avant gauche bas
	AddTriangle(nose, wRight, rBot, noseUV, wRightUV, rBotUV, c4);		// Avant droit bas
	AddTriangle(wLeft, rBot, rTop, wLeftUV, rBotUV, rTopUV, c5);		// Moteur gauche
	AddTriangle(wRight, rTop, rBot, wRightUV, rTopUV, rBotUV, c6);		// Moteur droit
	Optimize();
}
