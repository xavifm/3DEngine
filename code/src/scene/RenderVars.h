#pragma once

#include <glm\gtc\matrix_transform.hpp>
#include <GL_framework.h>

class RenderVariables {
public:
	static float FOV;
	static float zNear;
	static float zFar;
	static glm::mat4 _MVP;
	static int width; //<- cal enviar el width de la pantalla
	static int height; //<- cal enviar el height de la pantalla
	static glm::mat4 _projection;
	static glm::mat4 _modelView;
	static glm::mat4 _inv_modelview;
	static glm::vec4 _cameraPoint;

	static struct prevMouse {
		float lastx, lasty;
		MouseEvent::Button button = MouseEvent::Button::None;
		bool waspressed = false;
	};

	static prevMouse prevMouseStruct;

	static float panv[3];
	static float rota[2];

	static void SetupRenderVars(int _width, int _height) 
	{ 
		width = _width;
		height = _height;

		_modelView = glm::mat4(1.f);
		if (height != 0) _projection = glm::perspective(RenderVariables::FOV, (float)RenderVariables::width / (float)RenderVariables::height, RenderVariables::zNear, RenderVariables::zFar);
		else _projection = glm::perspective(RenderVariables::FOV, 0.f, RenderVariables::zNear, RenderVariables::zFar);

		_MVP = _projection * _modelView;
	}

	static void GLmousecb(MouseEvent ev) {
		if (prevMouseStruct.waspressed && prevMouseStruct.button == ev.button) {
			float diffx = ev.posx - prevMouseStruct.lastx;
			float diffy = ev.posy - prevMouseStruct.lasty;
			switch (ev.button) {
			case MouseEvent::Button::Left: // ROTATE
				rota[0] += diffx * 0.005f;
				rota[1] += diffy * 0.005f;
				break;
			case MouseEvent::Button::Right: // MOVE XY
				panv[0] += diffx * 0.03f;
				panv[1] -= diffy * 0.03f;
				break;
			case MouseEvent::Button::Middle: // MOVE Z
				panv[2] += diffy * 0.05f;
				break;
			default: break;
			}
		}
		else {
			prevMouseStruct.button = ev.button;
			prevMouseStruct.waspressed = true;
		}
		prevMouseStruct.lastx = ev.posx;
		prevMouseStruct.lasty = ev.posy;
	}

	static void GLRender() {
		_modelView = glm::mat4(1.f);

		_modelView = glm::translate(_modelView,
			glm::vec3(panv[0], panv[1], panv[2]));
		_modelView = glm::rotate(_modelView, rota[1],
			glm::vec3(1.f, 0.f, 0.f));
		_modelView = glm::rotate(_modelView, rota[0],
			glm::vec3(0.f, 1.f, 0.f));

		_MVP = _projection * _modelView;
	}

};
