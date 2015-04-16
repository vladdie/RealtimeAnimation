// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include "MeshLoader.h"
#include "ogldev_util.h"

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
//#define CHARACTER2 "hero.dae"
#define CHARACTER3 "cube.dae"
#define CHARACTER4 "Wooden Box.obj"
#define SNPRINTF _snprintf_s
#define INVALID_UNIFORM_LOCATION 0xffffffff

float counter = 0;
float scaleY = 0;
float startTime;
GLuint programID;
GLuint m_boneLocation[100];
glm::mat4 ModelMatrix;
glm::mat4 ModelMatrix1;
glm::mat4 ModelMatrix1p;
glm::mat4 ModelMatrix3;
//glm::mat4 ViewMatrix;

glm::vec3 interpolateCubic(float deltaTime, glm::vec3 beingPos, glm::vec3 point1,glm::vec3 point2, glm::vec3 endPos)
{
	glm::vec3 p1,p2,p3,p4;

	p1 = point2 - endPos - point1 + beingPos;
	p2 = point1 - beingPos - p1;
	p3 = endPos - point1;
	p4 = beingPos;

	return (p1 * pow(deltaTime,3) + p2 * pow(deltaTime,2) + p3 * pow(deltaTime,1) + p4);
}

void SetBoneTransform(uint Index, const Matrix4f& Transform)
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
	glUniformMatrix4fv(m_boneLocation[Index], 1, GL_TRUE, (const GLfloat*)Transform);       
}

float GetRunningTime()
{
	return (float)((double)GetCurrentTimeMillis() - (double)startTime) / 1000.0f;
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
	//GLuint positionID = glGetAttribLocation(programID, "vPosition");
	GLuint viewMatrixID = glGetUniformLocation(programID, "mV");
	GLuint modelMatrixID = glGetUniformLocation(programID, "mM");
	GLuint perspectiveMatrixID = glGetUniformLocation(programID, "mP");
	GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

	Mesh* m_pMesh1;
	Mesh* m_pMesh2;
	Mesh* m_pMesh3;
	Mesh* m_pMesh4;

	m_pMesh1 = new Mesh();
	m_pMesh2 = new Mesh();
	m_pMesh3 = new Mesh();
	m_pMesh4 = new Mesh();

	float cx = 0.0f; 
	
	startTime = GetCurrentTimeMillis();

    bool LoadSuccess2 = m_pMesh2->LoadMesh(CHARACTER2);
	bool LoadSuccess1 = m_pMesh1->LoadMesh(CHARACTER1);
	bool LoadSuccess3 = m_pMesh3->LoadMesh(CHARACTER3);
	bool LoadSuccess4 = m_pMesh4->LoadMesh(CHARACTER4);

	glm::mat4 ProjectionMatrix;
	glm::mat4 ViewMatrix ;
	float counter = 0.0;
	bool move=false;
	bool DIsPressed = false;
	bool DIsReleased = false;
	bool EIsPressed = false;
	bool EIsReleased = false;
	bool eat = false;
	bool collision = false;
	float collisionTime = 0;
	float DdeltaTime = 0;
	float EdeltaTime = 0;
	float EpressTime = 0;
	float DpressTime = 0;
	bool cameraRoll = true;
	bool changeCamera = false;
	bool changeCamera2 = false;
	bool moveBox = false;
	float pushtime;
	bool BoxPos=false;

	do{
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(programID);
		if (BoxPos)
		{
			ModelMatrix3 = glm::scale(glm::vec3(3, 6, 3))*glm::translate(glm::mat4(1.0),glm::vec3(-6,0, 15));
		}else{
			ModelMatrix3 = glm::scale(glm::vec3(3, 6, 3))*glm::translate(glm::mat4(1.0), glm::vec3(-6,0,5));
		}
		//glm::mat4 ModelMatrix3 = glm::scale(glm::vec3(3, 6, 3))*glm::translate(glm::mat4(1.0), glm::vec3(-6,0,5));
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		cx += 0.005f;
		if (cx > 1)
		{
			cx = 0;
			cameraRoll = false;
		}
		if (cameraRoll)
		{
			ViewMatrix = setViewMatrix(interpolateCubic(cx, glm::vec3(-120,20,50), glm::vec3(-50,20,-50), glm::vec3(50,20,-50), glm::vec3(100,20,50)),glm::vec3(0,0,0));
		}
		else
		{
			ViewMatrix = getViewMatrix();
		}
		
		if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
		{
			changeCamera = true;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			changeCamera2 = true;
		}
		if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
		{
			ViewMatrix = setViewMatrix(glm::vec3(0,0,-100),glm::vec3(0,0,0));
		}
		/*if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
		{
			ViewMatrix = getViewMatrix();
		}*/
		glm::mat4 ModelMatrix2  = glm::translate(glm::mat4(1.0),glm::vec3(-20,0,0));
		ModelMatrix2 *= glm::scale(glm::vec3(0.1f, 0.1f, 0.1f));
	/*	glm::mat4 ModelMatrix2  =  glm::scale(glm::vec3(10, 10, 10)) * glm::rotate((float)90, glm::vec3(0,1,0))*glm::mat4(1.0f);
		ModelMatrix2  =  glm::translate(glm::vec3(-6.0f, 0.1f, 0.1f));*/
		std::vector<Matrix4f> Transforms;
		glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &ModelMatrix2[0][0]);
		//if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		if (move)
		{
			float RunningTime = GetRunningTime();

			m_pMesh2->BoneTransform(RunningTime,Transforms);
			
			for (GLuint i = 0 ; i < Transforms.size() ; i++) 
			{
				SetBoneTransform(i, Transforms[i]);
			}	
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			{
				pushtime = glfwGetTime();
				move = true ;
				moveBox = true;
			}
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE && moveBox)
		{
			move = false;
			float tempTime = glfwGetTime()-pushtime;
			if (moveBox && tempTime<=1.0)
			{
				ModelMatrix3 = (1.0f-tempTime) * glm::scale(glm::vec3(3, 6, 3))*glm::translate(glm::vec3(-6,0,5)) + tempTime * glm::scale(glm::vec3(3, 6, 3))*glm::translate(glm::vec3(-6,0, 15));
			}else
			{
				moveBox = false;
				BoxPos = true;
			}
			//ModelMatrix3 = (1.0f-counter) * glm::scale(glm::vec3(3, 6, 3))*glm::translate(glm::vec3(-6,0,5)) + counter * glm::scale(glm::vec3(3, 6, 3))*glm::translate(glm::vec3(-6,0, 15));
			
		}
		
		m_pMesh2->Render();


		ModelMatrix = glm::mat4(1.0);
		ModelMatrix1 = glm::translate( glm::mat4(1.0),glm::vec3(15,0,0));
		if (eat)
		{
			ModelMatrix1p = glm::translate( glm::mat4(1.0),glm::vec3(15,0,0))*glm::scale(glm::vec3(1.3, 1.3, 1.3));
		}
		else
		{
			ModelMatrix1p =  glm::translate( glm::mat4(1.0),glm::vec3(17,0,0));
		}
		
		glUniformMatrix4fv(perspectiveMatrixID, 1, GL_FALSE, &ProjectionMatrix[0][0]);
		counter += 0.005;
		if (counter >= 1){
			counter = 0;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)//anticipation, scratch streach
		{
			DpressTime = glfwGetTime();
			DIsPressed = true;
			//anticipation
			ModelMatrix = (1.0f-counter) * glm::mat4(1.0) + counter * glm::scale(glm::vec3(1.4, 0.5, 1.4))*glm::translate(glm::vec3(0,-1, 0));
			DIsReleased = false;
		}
		if (DIsPressed && glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE)
		{
			DIsPressed = false;
			DIsReleased = true;
		}
		DdeltaTime = glfwGetTime()-DpressTime;
		if (DIsReleased && DdeltaTime<=1.0)
		{
			ModelMatrix = 4*(1.0f-DdeltaTime) * glm::scale(glm::vec3(1.4, 0.5, 1.4))*glm::translate(glm::vec3(0,-1, 0)) + 4*DdeltaTime * glm::scale(glm::vec3(0.8, 1.8, 0.8)) * glm::translate(glm::vec3(0,5,0));//拉伸
			if (changeCamera)
			{
				ViewMatrix = setViewMatrix(glm::vec3(ModelMatrix[3][0],ModelMatrix[3][1],ModelMatrix[3][2]),glm::vec3(ModelMatrix2[3][0],ModelMatrix2[3][1]+10,ModelMatrix2[3][2]));
			}
		}
		if (DIsReleased && DdeltaTime>1.0 && DdeltaTime<=2.0)
		{
			ModelMatrix = 4*(2.0f-DdeltaTime) * glm::scale(glm::vec3(0.8, 1.8, 0.8)) * glm::translate(glm::vec3(0,5,0)) + 4*(DdeltaTime-1.0f) * glm::scale(glm::vec3(1.1, 0.8, 1.1))*glm::translate(glm::vec3(0,20, 0));//收缩
			if (changeCamera)
			{
				ViewMatrix = setViewMatrix(glm::vec3(ModelMatrix[3][0]-2,ModelMatrix[3][1],ModelMatrix[3][2]),glm::vec3(ModelMatrix2[3][0],ModelMatrix2[3][1]+10,ModelMatrix2[3][2]));
			}
		}
		if (DIsReleased && DdeltaTime>2.0 && DdeltaTime<=3.0)
		{
			ModelMatrix = 4*(3.0f-DdeltaTime) * glm::scale(glm::vec3(1.1, 0.8, 1.1))*glm::translate(glm::vec3(0,20, 0)) + 4*(DdeltaTime-2.0f) * glm::scale(glm::vec3(0.8, 1.8, 0.8)) * glm::translate(glm::vec3(0,1,0));
			if (changeCamera)
			{
				ViewMatrix = setViewMatrix(glm::vec3(ModelMatrix[3][0]-2,ModelMatrix[3][1],ModelMatrix[3][2]),glm::vec3(ModelMatrix2[3][0],ModelMatrix2[3][1]+10,ModelMatrix2[3][2]));
			}
		}
		if (DIsReleased && DdeltaTime>3.0 && DdeltaTime<4.0)
		{
			ModelMatrix = 4*(4.0f-DdeltaTime) * glm::scale(glm::vec3(0.8, 1.8, 0.8)) * glm::translate(glm::vec3(0,1,0)) + 4*(DdeltaTime-3.0f) * glm::scale(glm::vec3(1.4, 0.5, 1.4))*glm::translate(glm::vec3(0,-1, 0));//收缩
			if (changeCamera)
			{
				ViewMatrix = setViewMatrix(glm::vec3(ModelMatrix[3][0]-2,ModelMatrix[3][1],ModelMatrix[3][2]),glm::vec3(ModelMatrix2[3][0],ModelMatrix2[3][1]+10,ModelMatrix2[3][2]));
			}
		}
		if (DIsReleased && DdeltaTime>=4.0)
		{
			ModelMatrix = (1.0f-counter) *  glm::scale(glm::vec3(1.4, 0.5, 1.4))*glm::translate(glm::vec3(0,-1, 0)) + counter * glm::mat4(1.0)*glm::translate(glm::vec3(0,0, 0));
			if (changeCamera)
			{
				ViewMatrix = setViewMatrix(glm::vec3(ModelMatrix[3][0]-2,ModelMatrix[3][1],ModelMatrix[3][2]),glm::vec3(ModelMatrix2[3][0],ModelMatrix2[3][1]+10,ModelMatrix2[3][2]));
			}
			DIsReleased = false;
			move = true;
			changeCamera = false;
		}

		

		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)//ease in ease out
		{
			EpressTime = glfwGetTime();
			EIsPressed = true;
			EIsReleased = false;
			ModelMatrix1 = glm::mat4(1.0);
			ModelMatrix1 =  glm::translate(glm::vec3(15,0,0))*glm::rotate(ModelMatrix1, (float)-45, glm::vec3(0,0,1));
		}
		if (EIsPressed && glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE)
		{
			EIsPressed = false;
			EIsReleased = true;
		}
		EdeltaTime = glfwGetTime()-EpressTime;
		if (EIsReleased && EdeltaTime<=0.4f &&(!eat))
		{
			ModelMatrix1 = glm::mat4(1.0);
			ModelMatrix1 = 8*(0.4f-EdeltaTime) * glm::translate(glm::vec3(15,0,0))*glm::rotate(ModelMatrix1, (float)-45, glm::vec3(0,0,1)) + 8*EdeltaTime * glm::translate(glm::vec3(9,0,0))*glm::rotate(ModelMatrix1, (float)-25, glm::vec3(0,0,1));//拉伸
			if (changeCamera2)
			{
				ViewMatrix = setViewMatrix(glm::vec3(ModelMatrix1[3][0]-2,ModelMatrix1[3][1],ModelMatrix1[3][2]),glm::vec3(-100,ModelMatrix1[3][1],0));
			}
		}
		if (EIsReleased && EdeltaTime>0.4f && EdeltaTime<=0.55f &&(!eat))
		{
			ModelMatrix1 = glm::mat4(1.0);
			ModelMatrix1 = 8*(0.55f-EdeltaTime) * glm::translate(glm::vec3(9,0,0))*glm::rotate(ModelMatrix1, (float)-25, glm::vec3(0,0,1)) + 8*(EdeltaTime-0.4f) * glm::translate(glm::vec3(0,0,0))*glm::rotate(ModelMatrix1, (float)-5, glm::vec3(0,0,1));//收缩
			if (changeCamera2)
			{
				ViewMatrix = setViewMatrix(glm::vec3(ModelMatrix1[3][0]-2,ModelMatrix1[3][1],ModelMatrix1[3][2]),glm::vec3(-100,ModelMatrix1[3][1],0));
			}
		}
		if (EIsReleased && EdeltaTime>0.55f && EdeltaTime<=0.7f &&(!eat))
		{
			ModelMatrix1 = glm::mat4(1.0);
			DIsReleased = true;
			DpressTime = glfwGetTime();
			ModelMatrix = glm::mat4(1.0) * glm::scale(glm::vec3(1.4, 0.5, 1.4))*glm::translate(glm::vec3(0,-1, 0));
			ModelMatrix1 = 8*(0.7f-EdeltaTime) * glm::translate(glm::vec3(0,0,0))*glm::rotate(ModelMatrix1, (float)-5, glm::vec3(0,0,1)) + 8*(EdeltaTime-0.55f) * glm::translate(glm::vec3(-10,0,0))*glm::rotate(ModelMatrix1, (float)15, glm::vec3(0,0,1));
			if (changeCamera2)
			{
				ViewMatrix = setViewMatrix(glm::vec3(ModelMatrix1[3][0]-2,ModelMatrix1[3][1],ModelMatrix1[3][2]),glm::vec3(-100,ModelMatrix1[3][1],0));
			}
		}
		if (EIsReleased && EdeltaTime>0.7f && EdeltaTime<=1.0f &&(!eat))
		{
			ModelMatrix1 = glm::mat4(1.0);
			ModelMatrix1 = 8*(1.0f-EdeltaTime) * glm::translate(glm::vec3(-10,0,0))*glm::rotate(ModelMatrix1, (float)15, glm::vec3(0,0,1)) + 8*(EdeltaTime-0.7f) * glm::translate(glm::vec3(-15,0,0))*glm::rotate(ModelMatrix1, (float)25, glm::vec3(0,0,1));//收缩
			if (changeCamera2)
			{
				ViewMatrix = setViewMatrix(glm::vec3(ModelMatrix1[3][0]-2,ModelMatrix1[3][1],ModelMatrix1[3][2]),glm::vec3(-100,ModelMatrix1[3][1],0));
			}
		}
		if (EIsReleased && EdeltaTime>1.0f && EdeltaTime<1.4f &&(!eat))
		{
			ModelMatrix1 = glm::mat4(1.0);
			ModelMatrix1 = 8*(1.4f-EdeltaTime) * glm::translate(glm::vec3(-15,0,0))*glm::rotate(ModelMatrix1, (float)25, glm::vec3(0,0,1)) + 8*(EdeltaTime-1.0f) * glm::translate(glm::vec3(-7,0,0))*glm::rotate(ModelMatrix1, (float)25, glm::vec3(0,0,1));//拉伸
			if (changeCamera2)
			{
				ViewMatrix = setViewMatrix(glm::vec3(ModelMatrix1[3][0]-2,ModelMatrix1[3][1],ModelMatrix1[3][2]), glm::vec3(100,ModelMatrix1[3][1],0));
			}
		}
		if (EIsReleased && EdeltaTime>1.4f && EdeltaTime<=1.55f &&(!eat))
		{
			ModelMatrix1 = glm::mat4(1.0);
			ModelMatrix1 = 8*(1.55f-EdeltaTime) * glm::translate(glm::vec3(-7,0,0))*glm::rotate(ModelMatrix1, (float)25, glm::vec3(0,0,1)) + 8*(EdeltaTime-1.4f) * glm::translate(glm::vec3(0,0,0))*glm::rotate(ModelMatrix1, (float)5, glm::vec3(0,0,1));//收缩
			if (changeCamera2)
			{
				ViewMatrix = setViewMatrix(glm::vec3(ModelMatrix1[3][0]-2,ModelMatrix1[3][1],ModelMatrix1[3][2]),glm::vec3(100,ModelMatrix1[3][1],0));
			}
		}
		if (EIsReleased && EdeltaTime>1.55f && EdeltaTime<=1.7f &&(!eat))
		{
			ModelMatrix1 = glm::mat4(1.0);
			ModelMatrix1 = 8*(1.7f-EdeltaTime) * glm::translate(glm::vec3(0,0,0))*glm::rotate(ModelMatrix1, (float)5, glm::vec3(0,0,1)) + 8*(EdeltaTime-1.55f) * glm::translate(glm::vec3(10,0,0))*glm::rotate(ModelMatrix1, (float)-15, glm::vec3(0,0,1));
			if (changeCamera2)
			{
				ViewMatrix = setViewMatrix(glm::vec3(ModelMatrix1[3][0]-2,ModelMatrix1[3][1],ModelMatrix1[3][2]),glm::vec3(100,ModelMatrix1[3][1],0));
			}
		}
		if (EIsReleased && EdeltaTime>1.7f && EdeltaTime<=2.0f &&(!eat))
		{
			ModelMatrix1 = glm::mat4(1.0);
			ModelMatrix1 = 8*(2.0f-EdeltaTime) * glm::translate(glm::vec3(10,0,0))*glm::rotate(ModelMatrix1, (float)-15, glm::vec3(0,0,1)) + 8*(EdeltaTime-1.7f) * glm::translate(glm::vec3(15,0,0))*glm::rotate(ModelMatrix1, (float)-45, glm::vec3(0,0,1));//收缩
			if (changeCamera2)
			{
				ViewMatrix = setViewMatrix(glm::vec3(ModelMatrix1[3][0]-2,ModelMatrix1[3][1],ModelMatrix1[3][2]),glm::vec3(100,ModelMatrix1[3][1],0));
			}
		}
		if (EIsReleased && EdeltaTime>2.0f &&(!eat))
		{
			//ModelMatrix1 = glm::mat4(1.0);
			ModelMatrix1 = (1.0f-counter) * glm::translate(glm::vec3(15,0,0))*glm::rotate(ModelMatrix1, (float)-45, glm::vec3(0,0,1)) + counter *glm::translate(glm::vec3(15,0,0))*glm::rotate(ModelMatrix1, (float)-1, glm::vec3(0,0,1));
			EIsReleased = false;
			collision = true;
			collisionTime = glfwGetTime();
			
		}
		float temp = glfwGetTime()-collisionTime;
		if (collision)
		{
			ModelMatrix1p =  glm::translate( glm::mat4(1.0),glm::vec3(17,0,0))*glm::scale(glm::vec3(0.5, 1.4, 0.5));
			
			if (temp<=0.5f)
			{
				ModelMatrix1p =  2*(0.5f-temp) * glm::translate( glm::mat4(1.0),glm::vec3(17,0,0))*glm::scale(glm::vec3(0.5, 1.4, 0.5)) + 2*temp *glm::translate(glm::vec3(17,10,0));
			}
			if (temp>0.5f && temp<=1)
			{
				ModelMatrix1p =  2*(1.0f-temp) * glm::translate(glm::vec3(17,10,0)) + 2*(temp-0.5f) *glm::translate(glm::vec3(15,0,0));
				changeCamera2 = false;
			}
			if (temp>1)
			{
				collision = false;
				eat = true;	
			}
		}
		
		glUniformMatrix4fv(viewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		m_pMesh1->Render();
		glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &ModelMatrix1[0][0]);
		m_pMesh3->Render();
		glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &ModelMatrix1p[0][0]);
		m_pMesh3->Render();
	

		
		glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &ModelMatrix3[0][0]);
		m_pMesh4->Render();

		glm::vec3 lightPos = glm::vec3(10, 10, 10);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);		

		glUniform1i(TextureID, 0);
		
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(4);

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

