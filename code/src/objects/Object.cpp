#include "../scene/RenderVars.h"
#include <vector>
#include <GL/glew.h>
#include "../scene/api/stb_image.h"
#include <glm/gtc/type_ptr.hpp>

extern GLuint compileShaderFromFile(const char* shaderStr, GLenum shaderType, const char* name = "");
extern GLuint compileShader(const char* shaderStr, GLenum shaderType, const char* name = "");

extern bool loadOBJ(const char* path,
	std::vector < glm::vec3 >& out_vertices,
	std::vector < glm::vec2 >& out_uvs,
	std::vector < glm::vec3 >& out_normals
);

class Object {

public:
	char* reference = "";
	char* OBJ = "";
	char* IMG = "";

private:
	glm::vec3 objectColor = glm::vec4(0.f, 0.f, 0.f, 1.f);

	std::vector <glm::vec3> vertices;
	std::vector <glm::vec2> uvs;
	std::vector <glm::vec3> normals;

	GLuint vao;
	GLuint vbo[2];
	GLuint shaders[3];
	GLuint program;
	GLuint texture;
	GLuint fbo_tex;
	GLuint fbo;
	glm::mat4 objectMatrix;

public:
	void setupObject(char* model, char* referenceName, char* textureImg)
	{
		OBJ = model;
		reference = referenceName;
		IMG = textureImg;

		bool res = loadOBJ(OBJ, vertices, uvs, normals);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(3, vbo);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, (uvs.size() / 2) * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

		glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);
		glBindVertexArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		shaders[0] = compileShaderFromFile("defaultVertexShader.txt", GL_VERTEX_SHADER, strcat(reference,  "Vertex"));
		shaders[1] = compileShaderFromFile("defaultFragmentShader.txt", GL_FRAGMENT_SHADER, strcat(reference, "Fragment"));
		shaders[2] = compileShaderFromFile("defaultGeometryShader.txt", GL_GEOMETRY_SHADER, strcat(reference, "GeometryShader"));

		program = glCreateProgram();
		glAttachShader(program, shaders[0]);
		glAttachShader(program, shaders[1]);
		glAttachShader(program, shaders[2]);
		glBindAttribLocation(program, 0, "in_Position");
		glBindAttribLocation(program, 1, "in_Normal");
		glBindAttribLocation(program, 2, "in_Tex");
		linkProgram(program);

		objectMatrix = glm::mat4(1.f);
	}

	void updateObject(glm::mat4 matrix)
	{
		objectMatrix = matrix;
	}

	void drawObject()
	{
		glBindVertexArray(vao);
		glUseProgram(program);
		setupTexture(IMG);
		glUniformMatrix4fv(glGetUniformLocation(program, "objMat"), 1, GL_FALSE, glm::value_ptr(objectMatrix));
		glUniformMatrix4fv(glGetUniformLocation(program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVariables::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVariables::_MVP));
		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
		glUseProgram(0);
		glBindVertexArray(0);
	}

	void setupTexture(char* img) 
	{
		if (img == NULL || img == "") return;

		stbi_set_flip_vertically_on_load(true);
		int x, y, n;
		unsigned char* dat = stbi_load(img, &x, &y, &n, 3);
		if (dat == NULL)
			fprintf(stderr, "NO VALID!");
		else {

		}
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, dat);
		stbi_image_free(dat);
	}

	void linkProgram(GLuint program)
	{
		glLinkProgram(program);
		GLint res;
		glGetProgramiv(program, GL_LINK_STATUS, &res);
		if (res == GL_FALSE)
		{
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &res);
			char* buff = new char[res];
			glGetProgramInfoLog(program, res, &res, buff);
			fprintf(stderr, "Error Link: %s", buff);
			delete[] buff;
		}
	}

	void cleanupObject()
	{
		glDeleteBuffers(3, vbo);
		glDeleteVertexArrays(1, &vao);

		glDeleteProgram(program);
		glDeleteShader(shaders[0]);
		glDeleteShader(shaders[1]);
	}

};