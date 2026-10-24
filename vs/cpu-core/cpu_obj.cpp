#include "pch.h"

#include <sstream>

namespace cpu_obj
{
	int ResolveIndex(int index, int count)
	{
		if ( index>0 )
			return index - 1;
		if ( index<0 )
			return count + index;
		return -1;
	}

	bool ParseIndex(const std::string& str, obj_index& result)
	{
		// Formats :
		//
		// v
		// v/vt
		// v//vn
		// v/vt/vn

		size_t slash1 = str.find('/');
		if ( slash1==std::string::npos )
		{
			result.v = std::stoi(str);
			return true;
		}

		result.v = std::stoi(str.substr(0, slash1));

		size_t slash2 = str.find('/', slash1 + 1);
		if ( slash2==std::string::npos )
		{
			// v/vt
			std::string vt = str.substr(slash1 + 1);
			if ( vt.empty()==false )
			{
				result.vt = std::stoi(vt);
				result.hasVT = true;
			}
			return true;
		}

		// v/vt/vn ou v//vn
		std::string vt = str.substr(slash1+1, slash2-slash1-1);
		std::string vn = str.substr(slash2+1);
		if ( vt.empty()==false )
		{
			result.vt = std::stoi(vt);
			result.hasVT = true;
		}
		if ( vn.empty()==false )
		{
			result.vn = std::stoi(vn);
			result.hasVN = true;
		}
		return true;
	}

	bool Load(cstr path, cpu_mesh* pMesh)
	{
		int size;
		byte* file = cpu::LoadFile(path, size);
		if ( file==nullptr )
			return false;
		if ( Load(file, size, pMesh)==false )
			return false;
		return true;
	}

	bool Load(byte* file, int size, cpu_mesh* pMesh)
	{
		cpu_mesh& mesh = *pMesh;
		mesh.Clear();

		std::vector<XMFLOAT3> positions;
		std::vector<XMFLOAT2> texcoords;
		std::vector<XMFLOAT3> normals;

		bool missingNormals = false;
		int offset = 0;
		while ( offset<size )
		{
			std::string line = GetLine(file, offset, size);
			if ( line.empty() )
				continue;

			std::istringstream stream(line);

			std::string type;
			stream >> type;

			// Commentaire
			if ( type.empty() || type[0]=='#' )
				continue;

			//----------------------------------------------------------
			// Position
			//----------------------------------------------------------

			if ( type=="v" )
			{
				XMFLOAT3 v;
				stream >> v.x >> v.y >> v.z;
				positions.push_back(v);
			}

			//----------------------------------------------------------
			// Texture coordinate
			//----------------------------------------------------------

			else if ( type=="vt" )
			{
				XMFLOAT2 uv;
				stream >> uv.x >> uv.y;
				texcoords.push_back(uv);
			}

			//----------------------------------------------------------
			// Normal
			//----------------------------------------------------------

			else if ( type=="vn" )
			{
				XMFLOAT3 normal;
				stream >> normal.x >> normal.y >> normal.z;
				normals.push_back(normal);
			}

			//----------------------------------------------------------
			// Face
			//----------------------------------------------------------

			else if ( type=="f" )
			{
				std::vector<obj_index> face;
				std::string vertexString;
				while ( stream >> vertexString )
				{
					obj_index index;
					if ( ParseIndex(vertexString, index)==false )
						return false;
					face.push_back(index);
				}
				if ( face.size()<3 )
					continue;

				//------------------------------------------------------
				// Triangle fan :
				//
				// 0---1
				// |  /|
				// | / |
				// 3---2
				//
				// devient :
				//
				// 0 1 2
				// 0 2 3
				//------------------------------------------------------

				for ( size_t i=1 ; i+1<face.size() ; i++ )
				{
					obj_index indices[3] =
					{
						face[0],
						face[i],
						face[i+1]
					};

					for ( int j=0 ; j<3 ; j++ )
					{
						cpu_vertex vertex;
						vertex.Identity();

						//----------------------------------------------
						// Position
						//----------------------------------------------

						int positionIndex = ResolveIndex(indices[j].v, (int)positions.size());

						if ( positionIndex<0 || positionIndex>=(int)positions.size() )
							return false;

						vertex.pos = positions[positionIndex];

						//----------------------------------------------
						// UV
						//----------------------------------------------

						if ( indices[j].hasVT )
						{
							int uvIndex = ResolveIndex(indices[j].vt, (int)texcoords.size());
							if ( uvIndex<0 || uvIndex>=(int)texcoords.size() )
								return false;
							vertex.uv = texcoords[uvIndex];
						}

						//----------------------------------------------
						// Normal
						//----------------------------------------------

						if ( indices[j].hasVN )
						{
							int normalIndex = ResolveIndex(indices[j].vn, (int)normals.size());
							if ( normalIndex<0 || normalIndex>=(int)normals.size() )
								return false;
							vertex.normal = normals[normalIndex];
						}
						else
							missingNormals = true;

						//----------------------------------------------
						// Couleur
						//----------------------------------------------

						vertex.color = CPU_WHITE;

						//----------------------------------------------

						mesh.vertices.push_back(vertex);
					}
				}
			}
		}

		//--------------------------------------------------------------
		// Normales absentes dans le OBJ
		//--------------------------------------------------------------

		if ( missingNormals )
			mesh.CalculateNormals();

		//--------------------------------------------------------------
		// Bounding volumes
		//--------------------------------------------------------------

		mesh.CalculateBoundingVolumes();
		return true;
	}

	std::string GetLine(byte* stream, int& offset, int size)
	{
		int startOffset = offset;
		std::string output = "";
		while ( offset<size )
		{
			char c = (char)stream[offset];
			if ( c=='\n' || c=='\r' || offset+1>=size )
			{
				int len = offset - startOffset;
				if ( c!='\n' && c!='\r' )
					len++;
				output.assign((cstr)stream+startOffset, (size_t)len);
				offset += c=='\r' ? 2 : 1;
				break;
			}
			offset++;
		}
		return output;
	}
}
