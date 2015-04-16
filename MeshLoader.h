#pragma once

#include <map>
#include <vector>
#include <GL/glew.h>
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags
#include <iostream>
#include "ogldev_math_3d.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include "TextureMesh.h"
#define INVALID_MATERIAL 0xFFFFFFFF
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define INDEX_BUFFER 0    
#define POS_VB       1
#define NORMAL_VB    2
#define TEXCOORD_VB  3  
#define BONE_VB 4
#define NUM_BONES_PER_VEREX 4
#define ZERO_MEM(a) memset(a, 0, sizeof(a))
typedef unsigned int uint;

struct Vertex
{
	glm::vec3 m_pos;
	glm::vec2 m_tex;
	glm::vec3 m_normal;

	Vertex() {}

	Vertex(const glm::vec3& pos, const glm::vec2& tex, const glm::vec3& normal)
	{
		m_pos = pos;
		m_tex = tex;
		m_normal = normal;
	}
};



struct BoneInfo
{
	Matrix4f BoneOffset;
	Matrix4f FinalTransformation;         

	BoneInfo()
	{
		BoneOffset.SetZero();
		FinalTransformation.SetZero();          
	}
};

class Mesh
{
public:
	Mesh();
	~Mesh();

	struct VertexBoneData
	{        
		uint IDs[NUM_BONES_PER_VEREX];
		float Weights[NUM_BONES_PER_VEREX];

		VertexBoneData()
		{
			Reset();
		};

		void Reset()
		{
			ZERO_MEM(IDs);
			ZERO_MEM(Weights);        
		}

		void AddBoneData(uint BoneID, float Weight);
	};


	struct MeshEntry {
		MeshEntry(){
			NumIndices = 0;
			BaseVertex = 0;
			BaseIndex = 0;
			MaterialIndex = INVALID_MATERIAL;
		}

		unsigned int NumIndices;
		unsigned int BaseVertex;
		unsigned int BaseIndex;
		unsigned int MaterialIndex;

	};


	bool LoadMesh(const std::string& Filename);
	void LoadBones(uint MeshIndex, const aiMesh* pMesh, std::vector<VertexBoneData>& Bones);
	void Render();
	void BoneTransform(float TimeInSeconds, std::vector<Matrix4f>& Transforms);

private:

	bool InitFromScene(const aiScene* pScene, const std::string& Filename);
	void InitMesh(uint MeshIndex, const aiMesh* paiMesh,
		std::vector<glm::vec3>& Positions,
		std::vector<glm::vec3>& Normals,
		std::vector<glm::vec2>& TexCoords, std::vector<VertexBoneData>& Bones,
		std::vector<unsigned int>& Indices);
	bool InitMaterials(const aiScene* pScene, const std::string& Filename);
	void Clear();

	glm::mat4 convertAssimpMatrix(aiMatrix4x4 from);
	uint FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
	uint FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
	uint FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);    
	const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const std::string NodeName);
	void ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const Matrix4f& ParentTransform);

	GLuint m_VAO;
	GLuint m_Buffers[4];

	
	std::vector<MeshEntry> m_Entries;
	std::vector<Texture*> m_Textures;
	std::map<std::string,uint> m_BoneMapping; // maps a bone name to its index
	uint m_NumBones;
	std::vector<BoneInfo> m_BoneInfo;
    const aiScene* m_pScene;
	Matrix4f m_GlobalInverseTransform;
	Assimp::Importer m_Importer;
};


