#ifndef CONTROLS_HPP
#define CONTROLS_HPP
// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

void computeMatricesFromInputs();
glm::mat4 getViewMatrix();
glm::mat4 getProjectionMatrix();
glm::mat4 getModelMatrixBall();
glm::mat4 getModelMatrixCube();
glm::mat4 setViewMatrix(glm::vec3 pos, glm::vec3 dir);


#endif