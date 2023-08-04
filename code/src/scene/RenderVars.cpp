#include "RenderVars.h"

float RenderVariables::FOV = glm::radians(65.f);
float RenderVariables::zNear = 1.f;
float RenderVariables::zFar = 50.0f;
float panv[3] = { 0.f, -5.f, -15.f };
float rota[2] = { 0.f, 0.f };
glm::mat4 RenderVariables::_modelView = glm::mat4(1.f);
glm::mat4 RenderVariables::_projection = glm::perspective(RenderVariables::FOV, (float)RenderVariables::width / (float)RenderVariables::height, RenderVariables::zNear, RenderVariables::zFar);