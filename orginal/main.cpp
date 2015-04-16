// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <cylinder.h>
//#include <common/maths_funcs.h>
//#include <common/maths_funcs.cpp>
// Include GLEW
#include <GL/glew.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include "MeshLoader.h"

// Include GLFW
#include <glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
//using namespace glm;

#include <shader.hpp>
#include <controls.hpp>
#define CHARACTER1 "ball.dae"
#define CHARACTER2 "human.dae"
#define SNPRINTF _snprintf_s
#define INVALID_UNIFORM_LOCATION 0xffffffff

float counter = 0;
float counter2 = 0;
float startTime;
GLuint programID;
GLuint m_boneLocation[100];

void SetBoneTransform(uint Index, glm::mat4& Transform)
{
	for (unsigned int i = 0 ; i < ARRAY_SIZE_IN_ELEMENTS(m_boneLocation) ; i++)
	{
		char Name[128];
		memset(Name, 0, sizeof(Name));
		SNPRINTF(Name, sizeof(Name), "gBones[%d]", i);
		m_boneLocation[i] = glGetUniformLocation(programID, Name);
		if (m_boneLocation[i] == INVALID_UNIFORM_LOCATION) {
			fprintf(stderr, "Warning! Unable to get the location of uniform '%s'\n", Name);
		}
	}
	glUniformMatrix4fv(m_boneLocation[Index], 1, GL_TRUE, &Transform[0][0]);       
}

int main(void)
{
	// Initialize GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "Lab3", NULL, NULL);
	if (window == NULL){
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetCursorPos(window, 1024 / 2, 768 / 2);

	startTime = glfwGetTime();

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);


	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shader
	programID = LoadShaders("diffuse.vs", "diffuse.ps");
	//GLuint programID2 = LoadShaders("diffuse.vs", "diffuse.ps");
	// Get a handle for our "MVP" uniform
	//GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint positionID = glGetAttribLocation(programID, "vPosition");
	GLuint viewMatrixID = glGetUniformLocation(programID, "mV");
	GLuint modelMatrixID = glGetUniformLocation(programID, "mM");
	GLuint perspectiveMatrixID = glGetUniformLocation(programID, "mP");
	
	Mesh* m_pMesh1;
	Mesh* m_pMesh2;

	m_pMesh1 = new Mesh();
	m_pMesh2 = new Mesh();
	

	//bool LoadSuccess1 = m_pMesh1->LoadMesh(CHARACTER1);
	bool LoadSuccess2 = m_pMesh2->LoadMesh(CHARACTER2);


	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

	do{

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		
		glUniformMatrix4fv(perspectiveMatrixID, 1, GL_FALSE, &ProjectionMatrix[0][0]);
		glUniformMatrix4fv(viewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);


		/*glm::mat4 ModelMatrix = glm::mat4(1.0f);*/
		//ModelMatrix *= glm::scale(glm::vec3(0.1f, 0.1f, 0.1f));
		///*ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-6.0f, 0.0f, 0.0f));*/
		//counter+=0.005;
		//if(counter >=1)
		//{
		//	counter = 0;
		//}
		//counter2+=0.003;
		//if(counter2 >=10)
		//{
		//	counter2 = 0;
		//}
		//bool isPressed = false;
		//bool isReleased = false;

		//float deltaTime = 0;
		//float pressTime = 0;

		//if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		//{
		//	pressTime = glfwGetTime();
		//	isPressed = true;
		//	float y = 0*(1-counter)+counter*(-10);
		//	float scaleY = 1*(1-counter2)+counter2*(1.5);
		//	if (scaleY>1.5)
		//	{
		//		scaleY = 1.5;
		//	}
		//	float x = 1 / scaleY;
		//	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, y, 0.0f));
		//	ModelMatrix *= glm::scale(glm::vec3(1, scaleY, 1.0));
		//	isReleased = false;
		//}
		//if (isPressed && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
		//{
		//	//releasTime = glfwGetTime();
		//	isPressed = false;
		//}
		//deltaTime = glfwGetTime()-pressTime;
		//if (deltaTime>1)
		//{
		//	isReleased = true;
		//	isPressed = false;
		//}
		/*else if (isPressed==false&&deltaTime<0.25&&isReleased==false)
		{
			ModelMatrix = 4*(0.25f-deltaTime)*glm::scale(glm::vec3(1.4,0.5,1.4))+4*(deltaTime)*glm::scale(glm::vec3(1,2,1))*glm::translate(glm::vec3(0,10,0));
		}
		else if(isPressed==false&&deltaTime>0.25&&deltaTime<0.5&&isReleased==false)
		{
			ModelMatrix = 4*(0.5f-deltaTime)*glm::scale(glm::vec3(1,2,1))*glm::translate(glm::vec3(0,10,0))+4*(deltaTime-0.25f)*glm::translate(glm::vec3(0,30,0));
		}
		else if(isPressed==false&&deltaTime>0.5&&deltaTime<0.75&&isReleased==false)
		{
			ModelMatrix = 4*(0.75f-deltaTime)*glm::translate(glm::vec3(0,30,0))+4*(deltaTime-0.5f)*glm::scale(glm::vec3(1,2,1))*glm::translate(glm::vec3(0,10,0));
		}
		else if (isPressed==false&&deltaTime<1&&deltaTime>0.75&&isReleased==false)
		{
			ModelMatrix = 4*(1-deltaTime)*glm::scale(glm::vec3(1,2,1))*glm::translate(glm::vec3(0,10,0))+4*(deltaTime-0.75f)*glm::scale(glm::vec3(1.4,0.5,1.4));
		}*/


		/*ModelMatrix *= glm::scale(glm::vec3(0.1f, 0.1f, 0.1f));
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-6.0f, 0.0f, 0.0f));*/
		//ModelMatrix = glm::rotate(ModelMatrix, (float)-90.0, glm::vec3(1,0,0));
		//glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		//m_pMesh1->Render();

		/*std::vector<glm::mat4> Transforms;
		float RunningTime = (glfwGetTime() - startTime) / 1000.0f;
		m_pMesh2->BoneTransform(RunningTime,Transforms);
		for (GLuint i = 0 ; i < Transforms.size() ; i++) 
		{
			SetBoneTransform(i, Transforms[i]);
		}	*/
		//glm::mat4 ModelMatrix2 = glm::translate(glm::mat4(1.0), glm::vec3(6.0f, 0.0f, 0.0f));//second character
		glm::mat4 ModelMatrix2  = glm::mat4(1.0);
		ModelMatrix2 *= glm::scale(glm::vec3(0.1f, 0.1f, 0.1f));
		
		std::vector<glm::mat4> Transforms;

		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		{
			float RunningTime = (glfwGetTime() - startTime) / 1000.0f;
			m_pMesh2->BoneTransform(RunningTime,Transforms);

			for (GLuint i = 0 ; i < Transforms.size() ; i++) 
			{
				SetBoneTransform(i, Transforms[i]);
			}	
		}
		glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &ModelMatrix2[0][0]);
		m_pMesh2->Render();


		glm::vec3 lightPos = glm::vec3(10, 10, 10);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);		
		
	/*	glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(4);*/

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	glDeleteProgram(programID);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

