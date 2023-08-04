#define STB_IMAGE_IMPLEMENTATION
#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <cstdio>
#include <cassert>
#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>
#include "GL_framework.h"
#include <vector>
#include <iostream>
#include <string>
#include <fstream>   
#include <sstream>  
#include "api/stb_image.h"

glm::vec3 lightPos;
GLuint fbo;
GLuint fbo_tex;
std::vector< glm::vec3 > vertices;
std::vector< glm::vec2 > uvs;
std::vector< glm::vec3 > normals;

namespace ImGui {
	void Render();
}

float lightIntensity;

namespace RenderVars {
	const float FOV = glm::radians(65.f);
	const float zNear = 1.f;
	const float zFar = 50.f;
	int width;
	int height;
	glm::mat4 _projection;
	glm::mat4 _modelView;
	glm::mat4 _MVP;
	glm::mat4 _inv_modelview;
	glm::vec4 _cameraPoint;

	struct prevMouse {
		float lastx, lasty;
		MouseEvent::Button button = MouseEvent::Button::None;
		bool waspressed = false;
	} prevMouse;

	float panv[3] = { 0.f, -5.f, -15.f };
	float rota[2] = { 0.f, 0.f };
}
namespace RV = RenderVars;

void GLResize(int width, int height) {
	glViewport(0, 0, width, height);

	RV::width = width;
	RV::height = height;
	if (height != 0) RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);
	else RV::_projection = glm::perspective(RV::FOV, 0.f, RV::zNear, RV::zFar);
}

void GLmousecb(MouseEvent ev) {
	if (RV::prevMouse.waspressed && RV::prevMouse.button == ev.button) {
		float diffx = ev.posx - RV::prevMouse.lastx;
		float diffy = ev.posy - RV::prevMouse.lasty;
		switch (ev.button) {
		case MouseEvent::Button::Left: // ROTATE
			RV::rota[0] += diffx * 0.005f;
			RV::rota[1] += diffy * 0.005f;
			break;
		case MouseEvent::Button::Right: // MOVE XY
			RV::panv[0] += diffx * 0.03f;
			RV::panv[1] -= diffy * 0.03f;
			break;
		case MouseEvent::Button::Middle: // MOVE Z
			RV::panv[2] += diffy * 0.05f;
			break;
		default: break;
		}
	}
	else {
		RV::prevMouse.button = ev.button;
		RV::prevMouse.waspressed = true;
	}
	RV::prevMouse.lastx = ev.posx;
	RV::prevMouse.lasty = ev.posy;
}

extern GLuint compileShaderFromFile(const char* shaderStr, GLenum shaderType, const char* name = "");
extern GLuint compileShader(const char* shaderStr, GLenum shaderType, const char* name = "");

void linkProgram(GLuint program) 
{
	glLinkProgram(program);
	GLint res;
	glGetProgramiv(program, GL_LINK_STATUS, &res);
	if (res == GL_FALSE) 
	{
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &res);
		char *buff = new char[res];
		glGetProgramInfoLog(program, res, &res, buff);
		fprintf(stderr, "Error Link: %s", buff);
		delete[] buff;
	}
}

namespace Axis {
	GLuint AxisVao;
	GLuint AxisVbo[3];
	GLuint AxisShader[2];
	GLuint AxisProgram;

	float AxisVerts[] = {
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 1.0
	};

	float AxisColors[] = {
		1.0, 0.0, 0.0, 1.0,
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 0.0, 1.0, 1.0,
		0.0, 0.0, 1.0, 1.0
	};

	GLubyte AxisIdx[] = {
		0, 1,
		2, 3,
		4, 5
	};

	const char* Axis_vertShader =
		"#version 330\n\
		in vec3 in_Position;\n\
		in vec4 in_Color;\n\
		out vec4 vert_color;\n\
		uniform mat4 mvpMat;\n\
		void main() {\n\
		vert_color = in_Color;\n\
		gl_Position = mvpMat * vec4(in_Position, 1.0);\n\
	}";

	const char* Axis_fragShader =
	"#version 330\n\
	in vec4 vert_color;\n\
	out vec4 out_Color;\n\
	void main() {\n\
		out_Color = vert_color;\n\
	}";

	void setupAxis() {
		glGenVertexArrays(1, &AxisVao);
		glBindVertexArray(AxisVao);
		glGenBuffers(3, AxisVbo);

		glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisVerts, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisColors, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 4, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, AxisVbo[2]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 6, AxisIdx, GL_STATIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		AxisShader[0] = compileShader(Axis_vertShader, GL_VERTEX_SHADER, "AxisVert");
		AxisShader[1] = compileShader(Axis_fragShader, GL_FRAGMENT_SHADER, "AxisFrag");

		AxisProgram = glCreateProgram();
		glAttachShader(AxisProgram, AxisShader[0]);
		glAttachShader(AxisProgram, AxisShader[1]);
		glBindAttribLocation(AxisProgram, 0, "in_Position");
		glBindAttribLocation(AxisProgram, 1, "in_Color");
		linkProgram(AxisProgram);
	}
	void cleanupAxis() {
		glDeleteBuffers(3, AxisVbo);
		glDeleteVertexArrays(1, &AxisVao);

		glDeleteProgram(AxisProgram);
		glDeleteShader(AxisShader[0]);
		glDeleteShader(AxisShader[1]);
	}
	void drawAxis() {
		glBindVertexArray(AxisVao);
		glUseProgram(AxisProgram);
		glUniformMatrix4fv(glGetUniformLocation(AxisProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RV::_MVP));
		glDrawElements(GL_LINES, 6, GL_UNSIGNED_BYTE, 0);

		glUseProgram(0);
		glBindVertexArray(0);
	}
}

namespace SkyBox {
	float vertices[] = {
		0.f, 0.f, 0.f
	};

	GLuint program;
	GLuint VAO, VBO;

	GLuint cubemapID;

	void init() 
	{
		GLuint vertex_shader = compileShaderFromFile("skyboxVertex.txt", GL_VERTEX_SHADER, "vs");
		GLuint geometry_shader = compileShaderFromFile("skyboxGeometry.txt", GL_GEOMETRY_SHADER, "gs");
		GLuint fragment_shader = compileShaderFromFile("skyboxFragment.txt", GL_FRAGMENT_SHADER, "fs");
		program = glCreateProgram();
		glAttachShader(program, vertex_shader);
		glAttachShader(program, geometry_shader);
		glAttachShader(program, fragment_shader);
		linkProgram(program);
		glDeleteShader(vertex_shader);
		glDeleteShader(geometry_shader);
		glDeleteShader(fragment_shader);

		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		glEnableVertexAttribArray(0);

		glGenTextures(1, &cubemapID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

		int width, height, nchannels;
		unsigned char* data;

		std::vector<std::string> textures_faces = {
			"skybox/right.png",
			"skybox/left.png",
			"skybox/top.png",
			"skybox/bottom.png",
			"skybox/front.png",
			"skybox/back.png",
		};

		for (GLuint i = 0; i < textures_faces.size(); i++) 
		{
			data = stbi_load(textures_faces[i].c_str(), &width, &height, &nchannels, 0);
			if (data == NULL) 
			{
				fprintf(stderr, "Error loading image %s", textures_faces[i].c_str());
				exit(1);
			}
			printf("%d %d %d\n", width, height, nchannels);
			unsigned int mode = nchannels == 3 ? GL_RGB : GL_RGBA;
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, mode, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		glBindVertexArray(0);
	}

	void cleanup() 
	{
		glDeleteProgram(program);
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}

	void render() 
	{
		glUseProgram(program);
		glBindVertexArray(VAO);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

		glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(RV::_projection));
		glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(RV::_inv_modelview));

		glDrawArrays(GL_POINTS, 0, 1);
	}
}
glm::vec3 carPos = glm::vec3(0, 0, 0);
glm::vec3 cubePosition;
glm::vec3 retrovisorPos;
int cam; float stipling;

//HARDCODED ZONE
//namespace Trees 
//{
//	const char* path = "quad.obj";
//
//	// Object material
//	glm::vec3 objectColor = glm::vec4(0.f, 0.f, 0.f, 1.f);
//
//	std::vector <glm::vec3> vertices;
//	std::vector <glm::vec2> uvs;
//	std::vector <glm::vec3> normals;
//
//	GLuint vao;
//	GLuint vbo[2];
//	GLuint shaders[3];
//	GLuint program;
//	GLuint cubeTexture;
//	GLuint fbo_tex;
//	GLuint fbo;
//	glm::mat4 objMat;
//
//	void setupFBOTree() 
//	{
//		glGenFramebuffers(1, &fbo);
//
//		glGenTextures(1, &fbo_tex);
//		glBindTexture(GL_TEXTURE_2D, fbo_tex);
//
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 800, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
//
//		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0);
//		glBindFramebuffer(GL_FRAMEBUFFER, 0);
//		glBindTexture(GL_TEXTURE_2D, 0);
//	}
//
//	void setupTexture4Tree() 
//	{
//		stbi_set_flip_vertically_on_load(true);
//		int x, y, n;
//		unsigned char* dat = stbi_load("tree.png", &x, &y, &n, 3);
//		if (dat == NULL)
//			fprintf(stderr, "NO VALID!");
//		else {
//
//		}
//		glGenTextures(1, &cubeTexture);
//		glBindTexture(GL_TEXTURE_2D, cubeTexture);
//
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
//
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, dat);
//		stbi_image_free(dat);
//
//	}
//	void setupTree() 
//	{
//		bool res = loadOBJ(path, vertices, uvs, normals);
//
//		glGenVertexArrays(1, &vao);
//		glBindVertexArray(vao);
//		glGenBuffers(3, vbo);
//
//		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
//		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
//		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(0);
//
//		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
//		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
//		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(1);
//		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
//		glBufferData(GL_ARRAY_BUFFER, (uvs.size() / 2) * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
//
//		glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(2);
//		glBindVertexArray(0);
//
//		glBindBuffer(GL_ARRAY_BUFFER, 0);
//		shaders[0] = compileShaderFromFile("vertexShader3.txt", GL_VERTEX_SHADER, "treeVert");
//		shaders[1] = compileShaderFromFile("fragmentShader3.txt", GL_FRAGMENT_SHADER, "treeFrag");
//		shaders[2] = compileShaderFromFile("geometryShader3.txt", GL_GEOMETRY_SHADER, "geometryShader");
//
//		program = glCreateProgram();
//		glAttachShader(program, shaders[0]);
//		glAttachShader(program, shaders[1]);
//		glAttachShader(program, shaders[2]);
//		glBindAttribLocation(program, 0, "in_Position");
//		glBindAttribLocation(program, 1, "in_Normal");
//		glBindAttribLocation(program, 2, "in_Tex");
//		linkProgram(program);
//
//		objMat = glm::mat4(1.f);
//	}
//
//	void updateTree(glm::mat4 matrix) 
//	{
//		objMat = matrix;
//	}
//
//
//	void drawTree() 
//	{
//		glBindVertexArray(vao);
//		glUseProgram(program);
//		setupTexture4Tree();
//		glUniformMatrix4fv(glGetUniformLocation(program, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
//		glUniformMatrix4fv(glGetUniformLocation(program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
//		glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
//		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
//		glUseProgram(0);
//		glBindVertexArray(0);
//	}
//
//	void drawTreeObject() 
//	{
//		glBindVertexArray(vao);
//		glUseProgram(program);
//
//		glUniformMatrix4fv(glGetUniformLocation(program, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
//		glUniformMatrix4fv(glGetUniformLocation(program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
//		glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
//		if (cubeTexture == NULL)
//		{
//			//NOTHING
//		}
//		else 
//		{
//			glBindTexture(GL_TEXTURE_2D, cubeTexture);
//		}
//		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
//		glBindTexture(GL_TEXTURE_2D, 0);
//
//	}
//
//	void drawTreeWithFBOTex() 
//	{
//		glm::mat4 t_mvp = RenderVars::_MVP;
//		glm::mat4 t_mv = RenderVars::_modelView;
//
//		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//		glClearColor(1.f, 1.f, 1.f, 1.f);
//		glViewport(0, 0, 800, 800);
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//		glEnable(GL_DEPTH_TEST);
//		glm::mat4 t = glm::translate(glm::mat4(1.f), glm::vec3(0.0f, 0.0f, -20.f));
//		glm::mat4 r = glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(1.f, 0.f, 0.f));
//		RenderVars::_modelView = t * r;
//		RenderVars::_MVP = RV::_projection * RV::_modelView;
//
//		RenderVars::_MVP = t_mvp;
//		RenderVars::_modelView = t_mv;
//		glBindFramebuffer(GL_FRAMEBUFFER, 0);
//		glClearColor(0.2, .2, 0.2, 1.);
//		glViewport(0, 0, RV::width, RV::height);
//		drawTreeObject();
//	}
//
//	void cleanupTree() 
//	{
//		glDeleteBuffers(3, vbo);
//		glDeleteVertexArrays(1, &vao);
//
//		glDeleteProgram(program);
//		glDeleteShader(shaders[0]);
//		glDeleteShader(shaders[1]);
//	}
//}
////here we only render the first middle of the camaro object
//namespace CarFirstMiddle 
//{
//	const char* path = "Camaro_v4.obj";
//
//	glm::vec3 objectColor = glm::vec4(0.f, 0.f, 0.f, 1.f);
//
//	std::vector <glm::vec3> vertices;
//	std::vector <glm::vec2> uvs;
//	std::vector <glm::vec3> normals;
//
//	GLuint vao;
//	GLuint vbo[2];
//	GLuint shaders[2];
//	GLuint program;
//	GLuint cubeTexture;
//	GLuint fbo_tex;
//	GLuint fbo;
//	glm::mat4 objMat;
//
//	void setupFBOCar() 
//	{
//
//		glGenFramebuffers(1, &fbo);
//
//		glGenTextures(1, &fbo_tex);
//		glBindTexture(GL_TEXTURE_2D, fbo_tex);
//
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 800, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
//
//		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0);
//		glBindFramebuffer(GL_FRAMEBUFFER, 0);
//		glBindTexture(GL_TEXTURE_2D, 0);
//	}
//
//	void setupTexture4Car() 
//	{
//		int x, y, n;
//		unsigned char *dat = stbi_load("Camaro_combined_images.png", &x, &y, &n, 3);
//		if (dat == NULL)
//			fprintf(stderr, "NO VALID!");
//		else 
//		{
//		
//		}
//		glGenTextures(1, &cubeTexture);
//		glBindTexture(GL_TEXTURE_2D, cubeTexture);
//
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, dat);
//		stbi_image_free(dat);
//
//	}
//	void setupCar() 
//	{
//		bool res = loadOBJ(path, vertices, uvs, normals);
//
//		std::vector < glm::vec3 > out_vertices;
//		std::vector < glm::vec2 > out_uvs;
//		std::vector < glm::vec3 > out_normals;
//
//		for (size_t i = 0; i < vertices.size(); i++)
//		{
//			if (i < vertices.size()/2)
//				out_vertices.push_back(vertices[i]);
//		}
//
//		for (size_t i = 0; i < uvs.size(); i++)
//		{
//			if (i < uvs.size()/2)
//				out_uvs.push_back(uvs[i]);
//		}
//
//		for (size_t i = 0; i < normals.size(); i++)
//		{
//			if (i < normals.size()/2)
//				out_normals.push_back(normals[i]);
//		}
//
//		glGenVertexArrays(1, &vao);
//		glBindVertexArray(vao);
//		glGenBuffers(3, vbo);
//
//		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
//		glBufferData(GL_ARRAY_BUFFER, out_vertices.size() * sizeof(glm::vec3), &out_vertices[0], GL_STATIC_DRAW);
//		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(0);
//
//		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
//		glBufferData(GL_ARRAY_BUFFER, out_normals.size() * sizeof(glm::vec3), &out_normals[0], GL_STATIC_DRAW);
//		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(1);
//		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
//		glBufferData(GL_ARRAY_BUFFER, out_uvs.size() * sizeof(glm::vec2), &out_uvs[0], GL_STATIC_DRAW);
//
//		glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(2);
//		glBindVertexArray(0);
//
//		glBindBuffer(GL_ARRAY_BUFFER, 0);
//		shaders[0] = compileShaderFromFile("vertexshadercar.txt", GL_VERTEX_SHADER, "cubeVert");
//		shaders[1] = compileShaderFromFile("fragmentshadercar.txt", GL_FRAGMENT_SHADER, "cubeFrag");
//
//		program = glCreateProgram();
//		glAttachShader(program, shaders[0]);
//		glAttachShader(program, shaders[1]);
//		glAttachShader(program, shaders[2]);
//		glBindAttribLocation(program, 0, "in_Position");
//		glBindAttribLocation(program, 1, "in_Normal");
//		glBindAttribLocation(program, 2, "in_Tex");
//		linkProgram(program);
//
//		objMat = glm::mat4(1.f);
//	}
//
//	void updateCar(glm::mat4 matrix) 
//	{
//		objMat = matrix;
//	}
//
//
//	void drawCar() 
//	{
//		glBindVertexArray(vao);
//		glUseProgram(program);
//		setupTexture4Car();
//	
//		glUniformMatrix4fv(glGetUniformLocation(program, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
//		glUniformMatrix4fv(glGetUniformLocation(program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
//		glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
//
//		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
//
//		glUseProgram(0);
//		glBindVertexArray(0);
//	}
//
//	void drawCarObject() 
//	{
//		glBindVertexArray(vao);
//		glUseProgram(program);
//
//		glUniformMatrix4fv(glGetUniformLocation(program, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
//		glUniformMatrix4fv(glGetUniformLocation(program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
//		glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
//		if (cubeTexture == NULL)
//		{
//			//EMPTY
//		}
//		else 
//		{
//			glBindTexture(GL_TEXTURE_2D, cubeTexture);
//		}
//		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
//		glBindTexture(GL_TEXTURE_2D, 0);
//
//	}
//
//	void drawCarWithFBOTex() 
//	{
//		glm::mat4 t_mvp = RenderVars::_MVP;
//		glm::mat4 t_mv = RenderVars::_modelView;
//		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//		glClearColor(1.f, 1.f, 1.f, 1.f);
//		glViewport(0, 0, 800, 800);
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//		glEnable(GL_DEPTH_TEST);
//		glm::mat4 t = glm::translate(glm::mat4(1.f), glm::vec3(0.0f, 0.0f, -20.f));
//		glm::mat4 r = glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(1.f, 0.f, 0.f));
//		RenderVars::_modelView = t * r;
//		RenderVars::_MVP = RV::_projection * RV::_modelView;
//		RenderVars::_MVP = t_mvp;
//		RenderVars::_modelView = t_mv;
//		glBindFramebuffer(GL_FRAMEBUFFER, 0);
//		glClearColor(0.2, .2, 0.2, 1.);
//		glViewport(0, 0, RV::width, RV::height);
//		drawCarObject();
//	}
//
//	void cleanupCar() {
//		glDeleteBuffers(3, vbo);
//		glDeleteVertexArrays(1, &vao);
//
//		glDeleteProgram(program);
//		glDeleteShader(shaders[0]);
//		glDeleteShader(shaders[1]);
//	}
//}
////here we only render the second middle of the camaro object
//namespace CarSecondMiddle {
//	const char* path = "Camaro_v4.obj";
//
//	glm::vec3 objectColor = glm::vec4(0.f, 0.f, 0.f, 1.f);
//
//	std::vector <glm::vec3> vertices;
//	std::vector <glm::vec2> uvs;
//	std::vector <glm::vec3> normals;
//
//	GLuint vao;
//	GLuint vbo[2];
//	GLuint shaders[2];
//	GLuint program;
//	GLuint cubeTexture;
//	GLuint fbo_tex;
//	GLuint fbo;
//	glm::mat4 objMat;
//
//	void setupFBOCar() 
//	{
//
//		glGenFramebuffers(1, &fbo);
//
//		glGenTextures(1, &fbo_tex);
//		glBindTexture(GL_TEXTURE_2D, fbo_tex);
//
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 800, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
//
//		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0);
//		glBindFramebuffer(GL_FRAMEBUFFER, 0);
//		glBindTexture(GL_TEXTURE_2D, 0);
//	}
//
//	void setupTexture4Car() 
//	{
//		int x, y, n;
//		unsigned char *dat = stbi_load("Camaro_combined_images.png", &x, &y, &n, 3);
//		if (dat == NULL)
//			fprintf(stderr, "NO VALID!");
//
//		glGenTextures(1, &cubeTexture);
//		glBindTexture(GL_TEXTURE_2D, cubeTexture);
//
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, dat);
//		stbi_image_free(dat);
//
//	}
//	void setupCar() 
//	{
//		bool res = loadOBJ(path, vertices, uvs, normals);
//
//		std::vector < glm::vec3 > out_vertices;
//		std::vector < glm::vec2 > out_uvs;
//		std::vector < glm::vec3 > out_normals;
//
//		for (size_t i = 0; i < vertices.size(); i++)
//		{
//			if (i >= vertices.size() / 2)
//				out_vertices.push_back(vertices[i]);
//		}
//
//		for (size_t i = 0; i < uvs.size(); i++)
//		{
//			if (i >= uvs.size() / 2)
//				out_uvs.push_back(uvs[i]);
//		}
//
//		for (size_t i = 0; i < normals.size(); i++)
//		{
//			if (i >= normals.size() / 2)
//				out_normals.push_back(normals[i]);
//		}
//
//		glGenVertexArrays(1, &vao);
//		glBindVertexArray(vao);
//		glGenBuffers(3, vbo);
//
//		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
//		glBufferData(GL_ARRAY_BUFFER, out_vertices.size() * sizeof(glm::vec3), &out_vertices[0], GL_STATIC_DRAW);
//		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(0);
//
//		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
//		glBufferData(GL_ARRAY_BUFFER, out_normals.size() * sizeof(glm::vec3), &out_normals[0], GL_STATIC_DRAW);
//		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(1);
//		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
//		glBufferData(GL_ARRAY_BUFFER, out_uvs.size() * sizeof(glm::vec2), &out_uvs[0], GL_STATIC_DRAW);
//
//		glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(2);
//		glBindVertexArray(0);
//
//		glBindBuffer(GL_ARRAY_BUFFER, 0);
//		shaders[0] = compileShaderFromFile("vertexshadercar.txt", GL_VERTEX_SHADER, "cubeVert");
//		shaders[1] = compileShaderFromFile("fragmentshadercar.txt", GL_FRAGMENT_SHADER, "cubeFrag");
//
//
//		program = glCreateProgram();
//		glAttachShader(program, shaders[0]);
//		glAttachShader(program, shaders[1]);
//		glAttachShader(program, shaders[2]);
//		glBindAttribLocation(program, 0, "in_Position");
//		glBindAttribLocation(program, 1, "in_Normal");
//		glBindAttribLocation(program, 2, "in_Tex");
//		linkProgram(program);
//
//
//		objMat = glm::mat4(1.f);
//	}
//
//	void updateCar(glm::mat4 matrix) 
//	{
//		objMat = matrix;
//	}
//
//	void drawCar() 
//	{
//		glBindVertexArray(vao);
//		glUseProgram(program);
//		setupTexture4Car();
//
//
//		glUniformMatrix4fv(glGetUniformLocation(program, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
//		glUniformMatrix4fv(glGetUniformLocation(program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
//		glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
//
//		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
//
//		glUseProgram(0);
//		glBindVertexArray(0);
//	}
//
//	void drawCarObject() 
//	{
//		glBindVertexArray(vao);
//		glUseProgram(program);
//
//		glUniformMatrix4fv(glGetUniformLocation(program, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
//		glUniformMatrix4fv(glGetUniformLocation(program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
//		glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
//		if (cubeTexture == NULL) {
//			//NOTHING
//		}
//		else {
//			glBindTexture(GL_TEXTURE_2D, cubeTexture);
//		}
//		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
//		glBindTexture(GL_TEXTURE_2D, 0);
//
//	}
//
//	void drawCarWithFBOTex() 
//	{
//		glm::mat4 t_mvp = RenderVars::_MVP;
//		glm::mat4 t_mv = RenderVars::_modelView;
//
//		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//		glClearColor(1.f, 1.f, 1.f, 1.f);
//		glViewport(0, 0, 800, 800);
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//		glEnable(GL_DEPTH_TEST);
//		glm::mat4 t = glm::translate(glm::mat4(1.f), glm::vec3(0.0f, 0.0f, -20.f));
//		glm::mat4 r = glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(1.f, 0.f, 0.f));
//		RenderVars::_modelView = t * r;
//		RenderVars::_MVP = RV::_projection * RV::_modelView;
//
//		RenderVars::_MVP = t_mvp;
//		RenderVars::_modelView = t_mv;
//		glBindFramebuffer(GL_FRAMEBUFFER, 0);
//		glClearColor(0.2, .2, 0.2, 1.);
//		glViewport(0, 0, RV::width, RV::height);
//		drawCarObject();
//
//	}
//
//	void cleanupCar() 
//	{
//		glDeleteBuffers(3, vbo);
//		glDeleteVertexArrays(1, &vao);
//
//		glDeleteProgram(program);
//		glDeleteShader(shaders[0]);
//		glDeleteShader(shaders[1]);
//	}
//}
//
//namespace Dragon {
//	GLuint modelVao;
//	GLuint modelVbo[3];
//	GLuint modelShaders[2];
//	GLuint modelProgram;
//	glm::mat4 objMat = glm::mat4(1.f);
//	float posz = 10;
//
//	const char* path = "dragon.obj";
//
//	void setupDragon() 
//	{
//		bool res = loadOBJ(path, vertices, uvs, normals);
//
//		glGenVertexArrays(1, &modelVao);
//		glBindVertexArray(modelVao);
//		glGenBuffers(2, modelVbo);
//
//		glBindBuffer(GL_ARRAY_BUFFER, modelVbo[0]);
//		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
//		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(0);
//
//		glBindBuffer(GL_ARRAY_BUFFER, modelVbo[1]);
//		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
//		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(1);
//
//		modelShaders[0] = compileShaderFromFile("vertexToonShader.txt", GL_VERTEX_SHADER, "objectVertexShader");
//		modelShaders[1] = compileShaderFromFile("fragmentToonShader.txt", GL_FRAGMENT_SHADER, "objectFragmentShader");
//
//
//		modelProgram = glCreateProgram();
//		glAttachShader(modelProgram, modelShaders[0]);
//		glAttachShader(modelProgram, modelShaders[1]);
//		glBindAttribLocation(modelProgram, 0, "in_Position");
//		glBindAttribLocation(modelProgram, 1, "in_Normal");
//		linkProgram(modelProgram);
//
//		objMat = glm::mat4(1.f);
//	}
//	void cleanupDragon() 
//	{
//		glDeleteBuffers(2, modelVbo);
//		glDeleteVertexArrays(1, &modelVao);
//
//		glDeleteProgram(modelProgram);
//		glDeleteShader(modelShaders[0]);
//		glDeleteShader(modelShaders[1]);
//	}
//	void updateDragon(const glm::mat4& transform) 
//	{
//		objMat = transform;
//	}
//	void drawDragon() {
//
//		glBindVertexArray(modelVao);
//		glUseProgram(modelProgram);
//
//		posz -= 0.05f;
//		glm::mat4 translate = glm::translate(glm::mat4(1.f), glm::vec3(3, 0, posz));
//		glm::mat4 scale = glm::scale(glm::mat4(1.f), glm::vec3(0.2,0.2,0.2));
//		glm::mat4 rotate = glm::rotate(glm::mat4(1.f),glm::radians(180.f), glm::vec3(0,1,0));
//		objMat = translate*scale*rotate;
//		glUniformMatrix4fv(glGetUniformLocation(modelProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
//		glUniformMatrix4fv(glGetUniformLocation(modelProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
//		glUniformMatrix4fv(glGetUniformLocation(modelProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
//		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
//
//		glUseProgram(0);
//		glBindVertexArray(0);
//	}
//
//
//}
//namespace Ground {
//	const char* path = "5.cube.obj";
//
//	glm::vec3 objectColor = glm::vec4(0.f, 0.f, 0.f, 1.f);
//
//	std::vector <glm::vec3> vertices;
//	std::vector <glm::vec2> uvs;
//	std::vector <glm::vec3> normals;
//
//	GLuint vao;
//	GLuint vbo[2];
//	GLuint shaders[2];
//	GLuint program;
//	GLuint cubeTexture;
//	GLuint fbo_tex;
//	GLuint fbo;
//	glm::mat4 objMat;
//
//	void setupTexture4Ground() 
//	{
//		int x, y, n;
//		unsigned char *dat = stbi_load("text1.jpg", &x, &y, &n, 3);
//		if (dat == NULL)
//			fprintf(stderr, "NO VALID!");
//
//		glGenTextures(1, &cubeTexture);
//		glBindTexture(GL_TEXTURE_2D, cubeTexture);
//
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, dat);
//		stbi_image_free(dat);
//	}
//	void setupGround() 
//	{
//		bool res = loadOBJ(path, vertices, uvs, normals);
//
//		glGenVertexArrays(1, &vao);
//		glBindVertexArray(vao);
//		glGenBuffers(3, vbo);
//
//		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
//		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
//		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(0);
//
//		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
//		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
//		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(1);
//		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
//		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
//
//		glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(2);
//		glBindVertexArray(0);
//
//		glBindBuffer(GL_ARRAY_BUFFER, 0);
//		shaders[0] = compileShaderFromFile("GroundVertexShader.txt", GL_VERTEX_SHADER, "cubeVert");
//		shaders[1] = compileShaderFromFile("GroundTextFragmentShader.txt", GL_FRAGMENT_SHADER, "cubeFrag");
//
//		program = glCreateProgram();
//		glAttachShader(program, shaders[0]);
//		glAttachShader(program, shaders[1]);
//		glAttachShader(program, shaders[2]);
//		glBindAttribLocation(program, 0, "in_Position");
//		glBindAttribLocation(program, 1, "in_Normal");
//		glBindAttribLocation(program, 2, "in_Tex");
//		linkProgram(program);
//
//		objMat = glm::mat4(1.f);
//	}
//
//	void setupFBOGround()
//	{
//
//		glGenFramebuffers(1, &fbo);
//
//		glGenTextures(1, &fbo_tex);
//		glBindTexture(GL_TEXTURE_2D, fbo_tex);
//
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 800, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
//
//		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0);
//		glBindFramebuffer(GL_FRAMEBUFFER, 0);
//		glBindTexture(GL_TEXTURE_2D, 0);
//	}
//
//	void updateCar(glm::mat4 matrix) 
//	{
//		objMat = matrix;
//	}
//
//	void drawGroundObject()
//	{
//		glBindVertexArray(vao);
//		glUseProgram(program);
//
//		for (int i = 200; 0 < i; i -= 2)
//		{
//			glm::mat4 translate = glm::translate(glm::mat4(), glm::vec3(0, -1.3f, i - 200));
//			objMat = translate;
//			glUniformMatrix4fv(glGetUniformLocation(program, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
//			glUniformMatrix4fv(glGetUniformLocation(program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
//			glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
//			glDrawArrays(GL_TRIANGLES, 0, vertices.size());
//
//			if (cubeTexture == NULL)
//			{
//				//EMPTY
//			}
//			else
//			{
//				glBindTexture(GL_TEXTURE_2D, cubeTexture);
//			}
//
//			for (int j = 0; j < 10; j += 2)
//			{
//				glm::mat4 translate = glm::translate(glm::mat4(), glm::vec3(j - 4, -1.3f, i - 200));
//				objMat = translate;
//				glUniformMatrix4fv(glGetUniformLocation(program, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
//				glUniformMatrix4fv(glGetUniformLocation(program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
//				glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
//				glDrawArrays(GL_TRIANGLES, 0, vertices.size());
//
//				if (cubeTexture == NULL)
//				{
//					//EMPTY
//				}
//				else
//				{
//					glBindTexture(GL_TEXTURE_2D, cubeTexture);
//				}
//			}
//		}
//		glUseProgram(0);
//		glBindVertexArray(0);
//	}
//
//	void drawGroundWithFBOTex()
//	{
//		glm::mat4 t_mvp = RenderVars::_MVP;
//		glm::mat4 t_mv = RenderVars::_modelView;
//		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//		glClearColor(1.f, 1.f, 1.f, 1.f);
//		glViewport(0, 0, 800, 800);
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//		glEnable(GL_DEPTH_TEST);
//		glm::mat4 t = glm::translate(glm::mat4(1.f), glm::vec3(0.0f, 0.0f, -20.f));
//		glm::mat4 r = glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(1.f, 0.f, 0.f));
//		RenderVars::_modelView = t * r;
//		RenderVars::_MVP = RV::_projection * RV::_modelView;
//		RenderVars::_MVP = t_mvp;
//		RenderVars::_modelView = t_mv;
//		glBindFramebuffer(GL_FRAMEBUFFER, 0);
//		glClearColor(0.2, .2, 0.2, 1.);
//		glViewport(0, 0, RV::width, RV::height);
//		drawGroundObject();
//	}
//
//	void cleanupGround() 
//	{
//		glDeleteBuffers(3, vbo);
//		glDeleteVertexArrays(1, &vao);
//
//		glDeleteProgram(program);
//		glDeleteShader(shaders[0]);
//		glDeleteShader(shaders[1]);
//	}
//}
//namespace Retrovisor {
//const char* path = "cubeXFaces.obj";
//
//glm::vec3 objectColor = glm::vec4(0.f, 0.f, 0.f, 1.f);
//
//std::vector <glm::vec3> vertices;
//std::vector <glm::vec2> uvs;
//std::vector <glm::vec3> normals;
//
//GLuint vao;
//GLuint vbo[2];
//GLuint shaders[2];
//GLuint program;
//GLuint cubeTexture;
//glm::mat4 objMat;
//
//void setupTexture4Cube() 
//{
//	int x, y, n;
//	unsigned char *dat = stbi_load("wall.jpg", &x, &y, &n, 3);
//
//	if (dat == NULL)
//		fprintf(stderr, "NO VALID!");
//	else 
//	{
//		printf("se carga");
//	}
//
//	glGenTextures(1, &cubeTexture);
//	glBindTexture(GL_TEXTURE_2D, cubeTexture);
//
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, dat);
//	stbi_image_free(dat);
//}
//void setupObject() 
//{
//	bool res = loadOBJ("cubeXFaces.obj", vertices, uvs, normals);
//
//	glGenVertexArrays(1, &vao);
//	glBindVertexArray(vao);
//	glGenBuffers(3, vbo);
//
//	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
//	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
//	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
//	glEnableVertexAttribArray(0);
//
//	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
//	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
//	glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
//	glEnableVertexAttribArray(1);
//	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
//	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
//
//	glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, 0);
//	glEnableVertexAttribArray(2);
//	glBindVertexArray(0);
//
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
//	shaders[0] = compileShaderFromFile("RetrovisorVertexShader.txt", GL_VERTEX_SHADER, "cubeVert");
//	shaders[1] = compileShaderFromFile("RetrovisorFragmentShader.txt", GL_FRAGMENT_SHADER, "cubeFrag");
//
//	program = glCreateProgram();
//	glAttachShader(program, shaders[0]);
//	glAttachShader(program, shaders[1]);
//	glAttachShader(program, shaders[2]);
//	glBindAttribLocation(program, 0, "in_Position");
//	glBindAttribLocation(program, 1, "in_Normal");
//	glBindAttribLocation(program, 2, "in_Tex");
//	linkProgram(program);
//	objMat = glm::mat4(1.f);
//}
//
//void updateObject(glm::mat4 matrix) 
//{
//	objMat = matrix;
//}
//
//
//void drawObject(GLuint tex) 
//{
//	glBindVertexArray(vao);
//	glUseProgram(program);
//	
//	glUniformMatrix4fv(glGetUniformLocation(program, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
//	glUniformMatrix4fv(glGetUniformLocation(program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
//	glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
//
//	glBindTexture(GL_TEXTURE_2D, tex);
//
//	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
//	glBindTexture(GL_TEXTURE_2D, 0);
//}
//void setupFBORetrovisor() 
//{
//	glGenFramebuffers(1, &fbo);
//
//	glGenTextures(1, &fbo_tex);
//	glBindTexture(GL_TEXTURE_2D, fbo_tex);
//
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 800, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
//
//	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0);
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
//	glBindTexture(GL_TEXTURE_2D, 0);
//	GLuint rbo;
//
//	glGenBuffers(1, &rbo);
//	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
//	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 800);
//	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
//
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
//	glBindTexture(GL_TEXTURE_2D, 0);
//	glBindTexture(GL_RENDERBUFFER, 0);
//
//}
//
//void drawRetrovisorFBO() 
//{
//	glm::mat4 t_mvp = RenderVars::_MVP;
//	glm::mat4 t_mv = RenderVars::_modelView;
//
//	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//	glClearColor(1.f, 1.f, 1.f, 1.f);
//	glViewport(0, 0, 800, 800);
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//	glEnable(GL_DEPTH_TEST);
//
//	RenderVars::_modelView = glm::mat4(1.f);
//
//	RenderVars::_modelView = glm::translate(RenderVars::_modelView, glm::vec3(0, retrovisorPos.y - 7.f, retrovisorPos.z));
//	RenderVars::_modelView = glm::rotate(RenderVars::_modelView, glm::radians(180.f), glm::vec3(0, 1, 0));
//	RenderVars::_MVP = RV::_projection*RV::_modelView;
//
//	SkyBox::render();
//	Dragon::drawDragon();
//	Ground::drawGroundObject();
//	Trees::drawTreeObject();
//
//	RenderVars::_MVP = t_mvp;
//	RenderVars::_modelView = t_mv;
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
//	glClearColor(0.2, .2, 0.2, 1.);
//	glViewport(0, 0, RV::width, RV::height);
//	Retrovisor::drawObject(fbo_tex);
//}
//
//void cleanupObject() 
//{
//	glDeleteBuffers(3, vbo);
//	glDeleteVertexArrays(1, &vao);
//	glDeleteProgram(program);
//	glDeleteShader(shaders[0]);
//	glDeleteShader(shaders[1]);
//}
//}
//
//void drawScene() 
//{
//	Axis::drawAxis();
//}
//
//namespace WindowCar {
//	GLuint cubeVao;
//	GLuint cubeVbo[3];
//	GLuint cubeShaders[2];
//	GLuint cubeProgram;
//	glm::mat4 objMat = glm::mat4(1.f);
//
//	extern const float halfW = 0.5f;
//	int numVerts = 24 + 6;
//	glm::vec3 verts[] = {
//		glm::vec3(-halfW, -halfW, -halfW),
//		glm::vec3(-halfW, -halfW,  halfW),
//		glm::vec3(halfW, -halfW,  halfW),
//		glm::vec3(halfW, -halfW, -halfW),
//		glm::vec3(-halfW,  halfW, -halfW),
//		glm::vec3(-halfW,  halfW,  halfW),
//		glm::vec3(halfW,  halfW,  halfW),
//		glm::vec3(halfW,  halfW, -halfW)
//	};
//	glm::vec3 norms[] = {
//		glm::vec3(0.f, -1.f,  0.f),
//		glm::vec3(0.f,  1.f,  0.f),
//		glm::vec3(-1.f,  0.f,  0.f),
//		glm::vec3(1.f,  0.f,  0.f),
//		glm::vec3(0.f,  0.f, -1.f),
//		glm::vec3(0.f,  0.f,  1.f)
//	};
//
//	glm::vec3 cubeVerts[] = {
//		verts[1], verts[0], verts[2], verts[3],
//		verts[5], verts[6], verts[4], verts[7],
//		verts[1], verts[5], verts[0], verts[4],
//		verts[2], verts[3], verts[6], verts[7],
//		verts[0], verts[4], verts[3], verts[7],
//		verts[1], verts[2], verts[5], verts[6]
//	};
//	glm::vec3 cubeNorms[] = {
//		norms[0], norms[0], norms[0], norms[0],
//		norms[1], norms[1], norms[1], norms[1],
//		norms[2], norms[2], norms[2], norms[2],
//		norms[3], norms[3], norms[3], norms[3],
//		norms[4], norms[4], norms[4], norms[4],
//		norms[5], norms[5], norms[5], norms[5]
//	};
//	GLubyte cubeIdx[] = {
//		0, 1, 2, 3, UCHAR_MAX,
//		4, 5, 6, 7, UCHAR_MAX,
//		8, 9, 10, 11, UCHAR_MAX,
//		12, 13, 14, 15, UCHAR_MAX,
//		16, 17, 18, 19, UCHAR_MAX,
//		20, 21, 22, 23, UCHAR_MAX
//	};
//
//	void setupWindowCar() 
//	{
//	
//		glGenVertexArrays(1, &cubeVao);
//		glBindVertexArray(cubeVao);
//		glGenBuffers(3, cubeVbo);
//
//		glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[0]);
//		glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
//		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(0);
//
//		glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[1]);
//		glBufferData(GL_ARRAY_BUFFER, sizeof(cubeNorms), cubeNorms, GL_STATIC_DRAW);
//		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
//		glEnableVertexAttribArray(1);
//
//		glPrimitiveRestartIndex(UCHAR_MAX);
//		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVbo[2]);
//		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIdx), cubeIdx, GL_STATIC_DRAW);
//
//		glBindVertexArray(0);
//		glBindBuffer(GL_ARRAY_BUFFER, 0);
//		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
//
//		cubeShaders[0] = compileShaderFromFile("WindowCarVertexShader.txt", GL_VERTEX_SHADER, "cubeVert");
//		cubeShaders[1] = compileShaderFromFile("WindowCarFragmentShader.txt", GL_FRAGMENT_SHADER, "cubeFrag");
//
//		cubeProgram = glCreateProgram();
//		glAttachShader(cubeProgram, cubeShaders[0]);
//		glAttachShader(cubeProgram, cubeShaders[1]);
//		glBindAttribLocation(cubeProgram, 0, "in_Position");
//		glBindAttribLocation(cubeProgram, 1, "in_Normal");
//		linkProgram(cubeProgram);	
//		
//	}
//
//	void cleanupWindowCar() 
//	{
//		glDeleteBuffers(3, cubeVbo);
//		glDeleteVertexArrays(1, &cubeVao);
//
//		glDeleteProgram(cubeProgram);
//		glDeleteShader(cubeShaders[0]);
//		glDeleteShader(cubeShaders[1]);
//	}
//
//	void updateWindowCar(const glm::mat4& transform) 
//	{
//		objMat = transform;
//	}
//
//	void drawWindowCar() 
//	{
//		glEnable(GL_PRIMITIVE_RESTART);
//		glBindVertexArray(cubeVao);
//		glUseProgram(cubeProgram);
//		glm::mat4 tv = glm::translate(glm::mat4(1.f), glm::vec3(carPos.x, carPos.y + 2.7, carPos.z));
//		glm::mat4 sv = glm::scale(glm::mat4(1.f), glm::vec3(4.5, 1.6, 0));
//		objMat= tv * sv;
//		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
//		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
//		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
//		glUniform1f(glGetUniformLocation(cubeProgram, "stipling"),stipling);
//		glUniform4f(glGetUniformLocation(cubeProgram, "color"), 0.f,0.f,1.f, 0.0f);
//		glDrawElements(GL_TRIANGLE_STRIP, numVerts, GL_UNSIGNED_BYTE, 0);
//		glUseProgram(0);
//		glBindVertexArray(0);
//		glDisable(GL_PRIMITIVE_RESTART);
//	}
//}
//END ON HARDCODED ZONE

void GLinit(int width, int height) 
{
	glViewport(0, 0, width, height);
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	glClearDepth(1.f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	SkyBox::init();
	RV::width = width;
	RV::height = height;
	RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);
	cubePosition = glm::vec3(1,1,1);
	//CarFirstMiddle::setupCar();
	//CarFirstMiddle::setupTexture4Car();
	//CarFirstMiddle::setupFBOCar();
	//CarSecondMiddle::setupCar();
	//CarSecondMiddle::setupTexture4Car();
	//CarSecondMiddle::setupFBOCar();
	//Trees::setupTree();
	//Trees::setupTexture4Tree();
	//Trees::setupFBOTree();
	//cam = 1;
	//Retrovisor::setupFBORetrovisor();
	//Ground::setupGround();
	//Ground::setupTexture4Ground();
	//Ground::setupFBOGround();
	//Dragon::setupDragon();
	//Retrovisor::setupObject();
	//Axis::setupAxis();	
	//WindowCar::setupWindowCar();
}

void GLrender(float dt) 
{
		glClearColor(0.2f, 0.2f, 0.2f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RV::_modelView = glm::mat4(1.f);

		RV::_modelView = glm::translate(RV::_modelView,
			glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));
		RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1],
			glm::vec3(1.f, 0.f, 0.f));
		RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0],
			glm::vec3(0.f, 1.f, 0.f));

		RV::_MVP = RV::_projection * RV::_modelView;

		glDepthMask(GL_FALSE);
		SkyBox::render();
		glDepthMask(GL_TRUE);

		//Retrovisor::drawObject(NULL);
		//LLISTA PER CRIDAR ELS SEUS DRAWS
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//Trees::drawTreeObject();
		//glm::mat4 t = glm::translate(glm::mat4(1.f), glm::vec3(carPos.x, carPos.y, carPos.z));
		//glm::mat4 r = glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
		//retrovisorPos = glm::vec3(carPos.x - 5.2f, carPos.y + 3.f, carPos.z - 3.5f);
		//Retrovisor::updateObject(glm::translate(glm::mat4(1.f), glm::vec3(retrovisorPos.x, retrovisorPos.y, retrovisorPos.z))*
		//glm::scale(glm::mat4(1.f), glm::vec3(1, 1, 0)));

		//CarFirstMiddle::updateCar(t*r);
		//CarSecondMiddle::updateCar(t*r);

		//Dragon::drawDragon();

		ImGui::Render();
}
void GLcleanup() 
{
	Axis::cleanupAxis();
	//CarFirstMiddle::cleanupCar();
	//CarSecondMiddle::cleanupCar();
	//Ground::cleanupGround();
	//Dragon::cleanupDragon();
	//WindowCar::cleanupWindowCar();
	//LLISTA PER FER CLEANUP
	SkyBox::cleanup();
}

void GUI() 
{
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 4);
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		if (ImGui::Button("Cam 1")) 
		{
			cam = 1;
		}	
		if (ImGui::Button("Cam 2")) 
		{
			cam = 2;
		}
		ImGui::SliderFloat("Transparency window car", &stipling, 0, 1);
	}
	ImGui::End();

	bool show_test_window = false;
	if (show_test_window) 
	{
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}
