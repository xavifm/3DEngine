#pragma once

#include <glm\gtc\matrix_transform.hpp>
#include <GL_framework.h>

class RenderVariables {
public:
	static float FOV;
	static float zNear;
	static float zFar;
	static int width; //<- cal enviar el width de la pantalla
	static int height; //<- cal enviar el height de la pantalla
	static glm::mat4 _projection;
	static glm::mat4 _modelView;
	static glm::mat4 _MVP;
	static glm::mat4 _inv_modelview;
	static glm::vec4 _cameraPoint;

	static struct prevMouse {
		float lastx, lasty;
		MouseEvent::Button button = MouseEvent::Button::None;
		bool waspressed = false;
	} prevMouse;

	static float panv[3];
	static float rota[2];
};
