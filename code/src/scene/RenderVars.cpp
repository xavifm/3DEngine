#include "RenderVars.h"

int RenderVariables::height = 0;
int RenderVariables::width = 0;
float RenderVariables::FOV = glm::radians(65.f);
float RenderVariables::zNear = 1.f;
float RenderVariables::zFar = 50.0f;
float panv[3] = { 0.f, -5.f, -15.f };
float rota[2] = { 0.f, 0.f };
float RenderVariables::panv[3] = { 0.f, 0.f, 0.f };
float RenderVariables::rota[2] = { 0.f, 0.f };
glm::mat4 RenderVariables::_inv_modelview = glm::mat4(1.f);
glm::mat4 RenderVariables::_modelView = glm::mat4(1.f);
glm::mat4 RenderVariables::_MVP = glm::mat4(1.f);
glm::mat4 RenderVariables::_projection = glm::mat4(1.f);
RenderVariables::prevMouse RenderVariables::prevMouseStruct;