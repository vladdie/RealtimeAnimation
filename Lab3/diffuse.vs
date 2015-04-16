#version 330 core
const int MAX_BONES = 100;
// Input vertex data, different for all executions of this shader.correspond to glVertexAttribPointer(0,...// attribute)
//layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vertexUV;// 2nd attribute buffer : UVs
layout(location = 2) in vec3 vertexNormal_modelspace;//glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
layout (location = 3) in ivec4 BoneIDs;
layout (location = 4) in vec4 Weights;


// Output data ; will be interpolated for each fragment.
out vec2 UV;
out vec3 Position_worldspace;
out vec3 Normal_cameraspace;
out vec3 EyeDirection_cameraspace;
out vec3 LightDirection_cameraspace;

// Values that stay constant for the whole mesh.
uniform mat4 mP;
uniform mat4 mV;
uniform mat4 mM;
uniform vec3 LightPosition_worldspace;
uniform mat4 gBones[MAX_BONES];

void main(){

	mat4 BoneTransform = gBones[BoneIDs[0]] * Weights[0];
    BoneTransform     += gBones[BoneIDs[1]] * Weights[1];
    BoneTransform     += gBones[BoneIDs[2]] * Weights[2];
    BoneTransform     += gBones[BoneIDs[3]] * Weights[3];
	vec4 PosL    = BoneTransform * vec4(vPosition, 1.0);
	
	if(PosL == vec4(0,0,0,0))
	{
		PosL = vec4(vPosition, 1.0);
	}
	
	gl_Position =  mP * mV * mM * PosL;
	//gl_Position = mP * mV * mM * vec4(vPosition,1);
	// Position of the vertex, in worldspace : M * position
	Position_worldspace    = (mM * PosL).xyz;
	
	// Vector that goes from the vertex to the camera, in camera space.
	// In camera space, the camera is at the origin (0,0,0).
	vec3 vertexPosition_cameraspace = ( mV * mM * vec4(vPosition,1)).xyz;
	//vec3 vertexPosition_cameraspace = ( mV * mM * PosL).xyz;
	EyeDirection_cameraspace = vec3(0,0,0) - vertexPosition_cameraspace;

	// Vector that goes from the vertex to the light, in camera space. M is ommited because it's identity.
	vec3 LightPosition_cameraspace = ( mV * vec4(LightPosition_worldspace,1)).xyz;
	LightDirection_cameraspace = LightPosition_cameraspace + EyeDirection_cameraspace;
	
	// Normal of the the vertex, in camera space
	//vec4 NormalL = BoneTransform * vec4(vertexNormal_modelspace, 0.0);
	Normal_cameraspace = ( mV * mM * vec4(vertexNormal_modelspace,0)).xyz; // Only correct if ModelMatrix does not scale the model ! Use its inverse transpose if not.
	//Normal_cameraspace = ( mV * mM * NormalL).xyz;
	
	// UV of the vertex. No special space for this one.
	UV = vertexUV;
}

