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
//#include "api/stb_image.h"
#include <list>
#include "../objects/Object.cpp"
#include "../scene/RenderVars.h"

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

void GLResize(int width, int height) {
	glViewport(0, 0, width, height);
	RenderVariables::SetupRenderVars(width, height);
}

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

		glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(RenderVariables::_projection));
		glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(RenderVariables::_inv_modelview));

		glDrawArrays(GL_POINTS, 0, 1);
	}
}
glm::vec3 carPos = glm::vec3(0, 0, 0);
glm::vec3 cubePosition;
glm::vec3 retrovisorPos;
int cam; float stipling;

namespace Scene 
{
	std::list<Object*> objects;
}

void CreateObjectInScene(char* _reference, char* _model, char* _texture, glm::vec3 _position, glm::vec3 _size)
{
	Object* obj = new Object();
	obj->setupObject(_model, _reference, _texture, _position, _size);

	Scene::objects.push_back(obj);
}

void CreateObjectInScene(char* _reference, char* _texture, glm::vec3 _position, glm::vec3 _size)
{
	Object* obj = new Object();
	obj->setupObject("default.obj", _reference, _texture, _position, _size);

	Scene::objects.push_back(obj);
}

void CreateObjectInScene(char* _reference, glm::vec3 _position, glm::vec3 _size)
{
	Object* obj = new Object();
	obj->setupObject("default.obj", _reference, NULL, _position, _size);

	Scene::objects.push_back(obj);
}

Object* GetObjectFromScene(char* _reference) 
{
	for each (Object* obj in Scene::objects)
	{
		if (obj->reference == _reference)
			return obj;
	}
}

void RemoveObjectFromScene(char* _reference)
{
	for each (Object* obj in Scene::objects)
	{
		if(obj->reference == _reference)
		obj->cleanupObject();
	}
}

void RemoveAllObjectsFromScene() 
{
	for each (Object* obj in Scene::objects)
	{
		obj->cleanupObject();
	}
}

void RenderScene() 
{
	for each (Object* obj in Scene::objects)
	{
		std::cout << obj->OBJ << std::endl;
		std::cout << obj->position.x << std::endl;
		std::cout << obj->position.y << std::endl;
		std::cout << obj->position.z << std::endl;
		std::cout << obj->reference << std::endl;
		obj->drawObject();
	}
}

void DebugTest() 
{
	CreateObjectInScene("TEST", "default.obj", "back.png", glm::vec3(0, 2, 0), glm::vec3(1, 1, 1));
	CreateObjectInScene("TEST2", "Camaro_v4.obj", "back.png", glm::vec3(0, 10, 0), glm::vec3(1, 1, 1));
}

void GLinit(int width, int height) 
{
	glViewport(0, 0, width, height);
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	glClearDepth(1.f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	SkyBox::init();

	RenderVariables::SetupRenderVars(width, height);
	cubePosition = glm::vec3(1,1,1);


	DebugTest();
}

//TEST
float zPos = 0;
//TEST

void GLrender(float dt) 
{
		glClearColor(0.2f, 0.2f, 0.2f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RenderVariables::GLRender();

		glDepthMask(GL_FALSE);
		SkyBox::render();
		glDepthMask(GL_TRUE);

		//LLISTA PER CRIDAR ELS SEUS DRAWS
		RenderScene();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//TEST MOVEMENT
		GetObjectFromScene("TEST")->SetRotationVector(zPos, zPos, 0);
		zPos += 0.01f;

		ImGui::Render();
}
void GLcleanup() 
{
	RemoveAllObjectsFromScene();
	SkyBox::cleanup();
}

void GUI() 
{
	//bool show = true;
	//ImGui::Begin("Physics Parameters", &show, 4);
	//{
	//	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	//	if (ImGui::Button("Cam 1")) 
	//	{
	//		cam = 1;
	//	}	
	//	if (ImGui::Button("Cam 2")) 
	//	{
	//		cam = 2;
	//	}
	//	ImGui::SliderFloat("Transparency window car", &stipling, 0, 1);
	//}
	//ImGui::End();

	//bool show_test_window = false;
	//if (show_test_window) 
	//{
	//	ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
	//	ImGui::ShowTestWindow(&show_test_window);
	//}
}
