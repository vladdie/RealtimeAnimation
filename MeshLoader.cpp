#include "MeshLoader.h"
#include <assert.h>

using namespace std;

#define INVALID_UNIFORM_LOCATION 0xffffffff
#define INVALID_OGL_VALUE 0xffffffff
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define ZERO_MEM(a) memset(a, 0, sizeof(a))//将a中前sizeof(a)个字节用0替换并返回a 。对数组进行清零操作


#define SAFE_DELETE(p) if (p) { delete p; p = NULL; }
#define GLCheckError() (glGetError() == GL_NO_ERROR)


Mesh::Mesh()
{
	m_VAO = 0;
	ZERO_MEM(m_Buffers);
	m_NumBones = 0;
	m_pScene = NULL;
}


Mesh::~Mesh()
{
	Clear();
}


void Mesh::Clear()
{
	for (uint i = 0 ; i < m_Textures.size() ; i++) {
		SAFE_DELETE(m_Textures[i]);
	}

	if (m_Buffers[0] != 0) {
		glDeleteBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);
	}

	if (m_VAO != 0) {
		glDeleteVertexArrays(1, &m_VAO);
		m_VAO = 0;
	}
}

void Mesh::VertexBoneData::AddBoneData(uint BoneID, float Weight)
{
	for (uint i = 0 ; i < ARRAY_SIZE_IN_ELEMENTS(IDs) ; i++) {
		if (Weights[i] == 0.0) {
			IDs[i]     = BoneID;
			Weights[i] = Weight;
			return;
		}        
	}

}


bool Mesh::LoadMesh(const std::string& Filename)
{
	// Release the previously loaded mesh (if it exists)
	Clear();

	//Create the VAO
	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	//Create the buffers for the vertices attibutes
	glGenBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);

	bool Ret = false;

	m_pScene = m_Importer.ReadFile(Filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

	if (m_pScene) {
		printf ("  %i animations\n", m_pScene->mNumAnimations);
		printf ("  %i cameras\n", m_pScene->mNumCameras);
		printf ("  %i lights\n", m_pScene->mNumLights);
		printf ("  %i materials\n", m_pScene->mNumMaterials);
		printf ("  %i meshes\n", m_pScene->mNumMeshes);
		printf ("  %i textures\n", m_pScene->mNumTextures);
		m_GlobalInverseTransform = m_pScene->mRootNode->mTransformation;
		m_GlobalInverseTransform.Inverse();
		Ret = InitFromScene(m_pScene, Filename);
	}
	else {
		printf("Error parsing '%s': '%s'\n", Filename.c_str(), m_Importer.GetErrorString());
	}
	//make sure the vao is not changed from outside code
	glBindVertexArray(0);
	return Ret;
}

bool Mesh::InitFromScene(const aiScene* pScene, const std::string& Filename)
{
	m_Entries.resize(pScene->mNumMeshes);//resize():重设元素个数.
	m_Textures.resize(pScene->mNumMaterials);

	// Prepare vectors for vertex attributes and indices
	vector<glm::vec3> Positions;
	vector<glm::vec3> Normals;
	vector<glm::vec2> TexCoords;
	vector<unsigned int> Indices;
    vector<VertexBoneData> Bones;

	unsigned int NumVertices = 0;
	unsigned int NumIndices = 0;

	// Initialize the meshes in the scene one by one
	for (unsigned int i = 0; i < m_Entries.size(); i++) {
		m_Entries[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;
		m_Entries[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3;
		m_Entries[i].BaseVertex = NumVertices;
		m_Entries[i].BaseIndex = NumIndices;

		NumVertices += pScene->mMeshes[i]->mNumVertices;
		NumIndices += m_Entries[i].NumIndices;
	}

	Positions.reserve(NumVertices);
	Normals.reserve(NumVertices);
	TexCoords.reserve(NumVertices);
	Indices.reserve(NumIndices);
	Bones.resize(NumVertices);

	// Initialize the meshes in the scene one by one
	for (unsigned int i = 0; i < m_Entries.size(); i++) {
		const aiMesh* paiMesh = pScene->mMeshes[i];
		InitMesh(i, paiMesh, Positions, Normals, TexCoords, Bones, Indices);
		//printf ("  %i bones\n", paiMesh->mNumBones);
	}
	InitMaterials(pScene, Filename);
	if (!InitMaterials(pScene, Filename)) {
		return false;
	}

	// Generate and populate the buffers with vertex attributes and the indices
	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Positions[0]) * Positions.size(), &Positions[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(TexCoords[0]) * TexCoords.size(), &TexCoords[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Normals[0]) * Normals.size(), &Normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	
	
	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[BONE_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Bones[0]) * Bones.size(), &Bones[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(3);//BONE_ID_LOCATION = 3
	glVertexAttribIPointer(3, 4, GL_INT, sizeof(VertexBoneData), (const GLvoid*)0);
	glEnableVertexAttribArray(4);    //BONE_WEIGHT_LOCATION = 4
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (const GLvoid*)16);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices[0]) * Indices.size(), &Indices[0], GL_STATIC_DRAW);

	
	return GLCheckError();
}

void Mesh::LoadBones(uint MeshIndex, const aiMesh* pMesh, vector<VertexBoneData>& Bones)
{
	for (uint i = 0 ; i < pMesh->mNumBones ; i++) {                
		uint BoneIndex = 0;        
		string BoneName(pMesh->mBones[i]->mName.data);
		cout<<"Bone " << i << " name : " << BoneName <<endl;
		if (m_BoneMapping.find(BoneName) == m_BoneMapping.end()) {
			// Allocate an index for a new bone
			BoneIndex = m_NumBones;
			m_NumBones++;            
			BoneInfo bi;			
			m_BoneInfo.push_back(bi);
			m_BoneInfo[BoneIndex].BoneOffset = pMesh->mBones[i]->mOffsetMatrix;            
			m_BoneMapping[BoneName] = BoneIndex;
		}
		else {
			BoneIndex = m_BoneMapping[BoneName];
		}                      

		for (uint j = 0 ; j < pMesh->mBones[i]->mNumWeights ; j++) {
			uint VertexID = m_Entries[MeshIndex].BaseVertex + pMesh->mBones[i]->mWeights[j].mVertexId;
			float Weight  = pMesh->mBones[i]->mWeights[j].mWeight;                   
			Bones[VertexID].AddBoneData(BoneIndex, Weight);
		}
	}    
}

glm::mat4 Mesh::convertAssimpMatrix(aiMatrix4x4 from)
{
	glm::mat4 temp;

	temp[0][0] = (GLfloat)from.a1; temp[1][0] = (GLfloat)from.a2;
	temp[2][0] = (GLfloat)from.a3; temp[3][0] = (GLfloat)from.a4;
	temp[0][1] = (GLfloat)from.b1; temp[1][1] = (GLfloat)from.b2;
	temp[2][1] = (GLfloat)from.b3; temp[3][1] = (GLfloat)from.b4;
	temp[0][2] = (GLfloat)from.c1; temp[1][2] = (GLfloat)from.c2;
	temp[2][2] = (GLfloat)from.c3; temp[3][2] = (GLfloat)from.c4;
	temp[0][3] = (GLfloat)from.d1; temp[1][3] = (GLfloat)from.d2;
	temp[2][3] = (GLfloat)from.d3; temp[3][3] = (GLfloat)from.d4;

	return temp;
}

void Mesh::InitMesh(uint MeshIndex, const aiMesh* paiMesh,
	vector<glm::vec3>& Positions,
	vector<glm::vec3>& Normals,
	vector<glm::vec2>& TexCoords,  std::vector<VertexBoneData>& Bones,
	vector<unsigned int>& Indices)
{
	float x = rand() % 10 + 1;
	float y = rand() % 10 + 1;
	float z = rand() % 10 + 1;
	const aiVector3D Zero3D(x, y, z);
	//const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
	printf ("  %i bones\n", paiMesh->mNumBones);
	// Populate the vertex attribute vectors
	for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
		const aiVector3D* pPos = &(paiMesh->mVertices[i]);
		const aiVector3D* pNormal = &(paiMesh->mNormals[i]);
		const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;
		Positions.push_back(glm::vec3(pPos->x, pPos->y, pPos->z));
		Normals.push_back(glm::vec3(pNormal->x, pNormal->y, pNormal->z));
		TexCoords.push_back(glm::vec2(pTexCoord->x, pTexCoord->y));
	}

	 LoadBones(MeshIndex, paiMesh, Bones);

	// Populate the index buffer
	for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
		const aiFace& Face = paiMesh->mFaces[i];
		assert(Face.mNumIndices == 3);
		Indices.push_back(Face.mIndices[0]);
		Indices.push_back(Face.mIndices[1]);
		Indices.push_back(Face.mIndices[2]);
	}
}


bool Mesh::InitMaterials(const aiScene* pScene, const string& Filename)
{
	// Extract the directory part from the file name
	string::size_type SlashIndex = Filename.find_last_of("/");//Searches the string for the last character that matches any of the characters specified in its arguments.
	string Dir;

	if (SlashIndex == string::npos) {//返回符合搜索条件的字符区间内的第一个字符的索引，没找到目标就返回npos
		Dir = ".";
	}
	else if (SlashIndex == 0) {
		Dir = "/";
	}
	else {
		Dir = Filename.substr(0, SlashIndex);
	}

	bool Ret = true;

	// Initialize the materials
	for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
		const aiMaterial* pMaterial = pScene->mMaterials[i];

		m_Textures[i] = NULL;

		if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString Path;

			if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
				string FullPath = Dir + "/" + Path.data;
				m_Textures[i] = new Texture(GL_TEXTURE_2D, FullPath.c_str());

				if (!m_Textures[i]->Load()) {
					printf("Error loading texture '%s'\n", FullPath.c_str());
					delete m_Textures[i];
					m_Textures[i] = NULL;
					Ret = false;
				}
				else {
					printf("%d - loaded texture '%s'\n", i, FullPath.c_str());
				}
			}
		}
	}

	return Ret;
}

void Mesh::Render()
{
	glBindVertexArray(m_VAO);

	for (unsigned int i = 0; i < m_Entries.size(); i++) {
		const unsigned int MaterialIndex = m_Entries[i].MaterialIndex;

		assert(MaterialIndex < m_Textures.size());

		if (m_Textures[MaterialIndex]) {
			m_Textures[MaterialIndex]->Bind(GL_TEXTURE0);
		}

		glDrawElementsBaseVertex(GL_TRIANGLES,
			m_Entries[i].NumIndices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(unsigned int) * m_Entries[i].BaseIndex),
			m_Entries[i].BaseVertex);
	}

	// Make sure the VAO is not changed from the outside    
	glBindVertexArray(0);
}

uint Mesh::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
{    
	for (uint i = 0 ; i < pNodeAnim->mNumPositionKeys - 1 ; i++) {
		if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
			return i;
		}
	}

	assert(0);

	return 0;
}

uint Mesh::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	assert(pNodeAnim->mNumRotationKeys > 0);

	for (uint i = 0 ; i < pNodeAnim->mNumRotationKeys - 1 ; i++) {
		if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
			return i;
		}
	}

	assert(0);

	return 0;
}

uint Mesh::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	assert(pNodeAnim->mNumScalingKeys > 0);

	for (uint i = 0 ; i < pNodeAnim->mNumScalingKeys - 1 ; i++) {
		if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
			return i;
		}
	}

	assert(0);

	return 0;
}


void Mesh::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumPositionKeys == 1) {
		Out = pNodeAnim->mPositionKeys[0].mValue;
		return;
	}

	uint PositionIndex = FindPosition(AnimationTime, pNodeAnim);
	uint NextPositionIndex = (PositionIndex + 1);
	assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
	float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
	const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
	aiVector3D Delta = End - Start;
	Out = Start + Factor * Delta;
}

void Mesh::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	// we need at least two values to interpolate...
	if (pNodeAnim->mNumRotationKeys == 1) {
		Out = pNodeAnim->mRotationKeys[0].mValue;
		return;
	}

	uint RotationIndex = FindRotation(AnimationTime, pNodeAnim);
	uint NextRotationIndex = (RotationIndex + 1);
	assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
	float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
	const aiQuaternion& EndRotationQ   = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;    
	aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
	Out = Out.Normalize();
}

void Mesh::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumScalingKeys == 1) {
		Out = pNodeAnim->mScalingKeys[0].mValue;
		return;
	}

	uint ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
	uint NextScalingIndex = (ScalingIndex + 1);
	assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
	float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
	const aiVector3D& End   = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
	aiVector3D Delta = End - Start;
	Out = Start + Factor * Delta;
}

void Mesh::ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const Matrix4f& ParentTransform)
{    
	string NodeName(pNode->mName.data);

	const aiAnimation* pAnimation = m_pScene->mAnimations[0];

	Matrix4f NodeTransformation(pNode->mTransformation);

	const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);

	if (pNodeAnim) {
		// Interpolate scaling and generate scaling transformation matrix
		aiVector3D Scaling;
		CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
		Matrix4f ScalingM;
		ScalingM.InitScaleTransform(Scaling.x, Scaling.y, Scaling.z);

		// Interpolate rotation and generate rotation transformation matrix
		aiQuaternion RotationQ;
		CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);        
		Matrix4f RotationM = Matrix4f(RotationQ.GetMatrix());

		// Interpolate translation and generate translation transformation matrix
		aiVector3D Translation;
		CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
		Matrix4f TranslationM;
		TranslationM.InitTranslationTransform(Translation.x, Translation.y, Translation.z);

		// Combine the above transformations
		NodeTransformation = TranslationM * RotationM * ScalingM;
	}

	Matrix4f GlobalTransformation = ParentTransform * NodeTransformation;

	if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
		uint BoneIndex = m_BoneMapping[NodeName];
		m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
	}

	for (uint i = 0 ; i < pNode->mNumChildren ; i++) {
		ReadNodeHeirarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
	}
}

void Mesh::BoneTransform(float TimeInSeconds, vector<Matrix4f>& Transforms)
{
	Matrix4f Identity;
	Identity.InitIdentity();
	if (m_pScene->HasAnimations())
	{
		float TicksPerSecond = (float)(m_pScene->mAnimations[0]->mTicksPerSecond != 0 ? m_pScene->mAnimations[0]->mTicksPerSecond : 25.0f);
		float TimeInTicks = TimeInSeconds * TicksPerSecond;
		float AnimationTime = fmod(TimeInTicks, (float)m_pScene->mAnimations[0]->mDuration);

		ReadNodeHeirarchy(AnimationTime, m_pScene->mRootNode, Identity);

		Transforms.resize(m_NumBones);

		for (uint i = 0 ; i < m_NumBones ; i++) {
		Transforms[i] = m_BoneInfo[i].FinalTransformation;
		}
	}
	
}


const aiNodeAnim* Mesh::FindNodeAnim(const aiAnimation* pAnimation, const string NodeName)
{
	for (uint i = 0 ; i < pAnimation->mNumChannels ; i++) {
		const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

		if (string(pNodeAnim->mNodeName.data) == NodeName) {
			return pNodeAnim;
		}
	}

	return NULL;
}