#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <vector>
#include <string.h>
#include <process.h>
#include <stdio.h>
#include <regex>

#include <GL\glew.h>

#include <GLFW\glfw3.h>
#include <GL\GL.h>

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtx\transform.hpp>

#include <soil\SOIL.h>

#define WIDTH 640
#define HEIGHT 480
#define SHADOW_WIDTH 2048
#define SHADOW_HEIGHT 2048
#define MIRROR_TEXTURE_WIDTH 256
#define MIRROR_TEXTURE_HEIGHT 256
#define PI 3.14159265359

#include "Mesh.h"
#include "Camera.h"

using namespace std;

void render(GLFWwindow*);
void init();

#define glInfo(a) std::cout << #a << ": " << glGetString(a) << std::endl

const std::regex obj_regex1("f [0-9]+// [0-9]+// [0-9]+//(.*)");
const std::regex obj_regex2("f [0-9]+//[0-9]+ [0-9]+//[0-9]+ [0-9]+//[0-9]+(.*)");
const std::regex obj_regex3("f [0-9]+/[0-9]+/[0-9]+ [0-9]+/[0-9]+/[0-9]+ [0-9]+/[0-9]+/[0-9]+[\\r\\n]*");
const std::regex obj_regex4("f [0-9]+/[0-9]+(/?) [0-9]+/[0-9]+(/?) [0-9]+/[0-9]+(/?)(.*)");

const float step_rot = 1.0f;
const float step_pos = 0.2f;

// This function is called on any openGL API error
void APIENTRY debug(GLenum, // source
	GLenum, // type
	GLuint, // id
	GLenum, // severity
	GLsizei, // length
	const GLchar *message,
	const void *) // userParam
{
	std::cout << "DEBUG: " << message << std::endl;
}

void translate(std::vector<glm::vec4> &points, const glm::vec3 &t)
{
	for (unsigned int i = 0; i < points.size(); i++)
	{
		points[i] += glm::vec4(t.x, t.y, t.z, 0.f);
	}
}

glm::vec4 maxVec4(const glm::vec4 &v1, const glm::vec4 &v2, const glm::vec4 &center)
{
	if (glm::distance(v1, center) > glm::distance(v2, center))
		return v1;
	return v2;
}

void normalizePointList(std::vector<glm::vec4> &points, const glm::vec3 &pos, const glm::vec3 &scale, const float rot_angle, const glm::vec3 &rot_axes)
{
	glm::vec3 shift(0.f);
	for (unsigned int i = 0; i < points.size(); i++)
	{
		shift += glm::vec3(points[i].x, points[i].y, points[i].z);
	}
	shift /= (float)points.size();

	translate(points, -shift);
	float minX, maxX, minY, maxY, minZ, maxZ, diffX, diffY, diffZ;
	maxX = (minX = points[0].x);
	maxY = (minY = points[0].y);
	maxZ = (minZ = points[0].z);
	glm::vec4 max(points[0]);
	glm::vec4 center(shift.x, shift.y, shift.z, 1.f);

	for (unsigned int i = 1; i < points.size(); i++)
	{
		maxX = std::fmaxf(maxX, points[i].x);
		minX = std::fminf(minX, points[i].x);

		maxY = std::fmaxf(maxY, points[i].y);
		minY = std::fminf(minY, points[i].y);

		maxZ = std::fmaxf(maxZ, points[i].z);
		minZ = std::fminf(minZ, points[i].z);
		max = maxVec4(max, points[i], center);
	}

	diffX = maxX - minX;
	diffY = maxY - minY;
	diffZ = maxZ - minZ;

	float length = static_cast<float>(max.length());

	for (unsigned int i = 0; i < points.size(); i++)
	{
		points[i].x = ((points[i].x - minX) - diffX * 0.5f) / (length * 0.5f);
		points[i].y = ((points[i].y - minY) - diffY * 0.5f) / (length * 0.5f);
		points[i].z = ((points[i].z - minZ) - diffZ * 0.5f) / (length * 0.5f);
		points[i] = glm::translate(pos) * glm::rotate(glm::radians(rot_angle), rot_axes) * glm::scale(scale) * points[i];
	}
}

bool load_obj(const char* filename, std::vector<glm::vec4> &vertices, std::vector<glm::vec2> &textures, std::vector<glm::vec3> &normals, std::vector<GLuint> &verticesIndex,
	std::vector<GLuint> &texturesIndex, std::vector<GLuint> &normalsIndex, std::vector<GLuint> &texturesNumber, GLuint num_texture,
	const glm::vec3 &pos = glm::vec3(0.f), const glm::vec3 &scale = glm::vec3(1.f), const float rot_angle = 0.f, const glm::vec3 &rot_axes = glm::vec3(1.f), const bool computeNormals = false)
{
	FILE *file;
	fopen_s(&file, filename, "r");

	if (file == NULL)
	{
		printf("Impossible to open the file !\n");
		return false;
	}

	while (!feof(file))
	{
		char line[255];

		fgets(line, 255, file);

		if (strncmp(line, "v ", 2) == 0)
		{
			glm::vec3 vertex;
			sscanf_s(line, "v %f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			vertices.push_back(glm::vec4(vertex.x,vertex.y, vertex.z, 1.f));
			texturesNumber.push_back(num_texture);
		}
		else if (strncmp(line, "vt", 2) == 0)
		{
			glm::vec2 uv;
			sscanf_s(line, "vt %f %f\n", &uv.x, &uv.y);
			textures.push_back(uv);
		}
		else if (strncmp(line, "vn", 2) == 0)
		{
			glm::vec3 normal;
			sscanf_s(line, "vn %f %f %f\n", &normal.x, &normal.y, &normal.z);
			normals.push_back(normal);
		}
		else if (strncmp(line, "f ", 2) == 0)
		{
			GLuint *vertexIndex = new GLuint[3], *uvIndex = new GLuint[3], *normalIndex = new GLuint[3];

			if (std::regex_match(line, obj_regex3))
			{
				int matches = sscanf_s(line, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
				if (matches == 9)
				{
					verticesIndex.push_back(vertexIndex[0]);
					verticesIndex.push_back(vertexIndex[1]);
					verticesIndex.push_back(vertexIndex[2]);
					texturesIndex.push_back(uvIndex[0]);
					texturesIndex.push_back(uvIndex[1]);
					texturesIndex.push_back(uvIndex[2]);
					normalsIndex.push_back(normalIndex[0]);
					normalsIndex.push_back(normalIndex[1]);
					normalsIndex.push_back(normalIndex[2]);
				}
			}
			else if (std::regex_match(line, obj_regex2))
			{
				int matches = sscanf_s(line, "f %d//%d %d//%d %d//%d\n", &vertexIndex[0], &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2]);
				if (matches == 6)
				{
					verticesIndex.push_back(vertexIndex[0]);
					verticesIndex.push_back(vertexIndex[1]);
					verticesIndex.push_back(vertexIndex[2]);
					normalsIndex.push_back(normalIndex[0]);
					normalsIndex.push_back(normalIndex[1]);
					normalsIndex.push_back(normalIndex[2]);
				}
			}
			else if (std::regex_match(line, obj_regex1))
			{
				int matches = sscanf_s(line, "f %d// %d// %d//\n", &vertexIndex[0], &vertexIndex[1], &vertexIndex[2]);
				if (matches == 3)
				{
					verticesIndex.push_back(vertexIndex[0]);
					verticesIndex.push_back(vertexIndex[1]);
					verticesIndex.push_back(vertexIndex[2]);
				}
			}
			else if (std::regex_match(line, obj_regex4))
			{
				int matches = sscanf_s(line, "f %d/%d %d/%d %d/%d\n", &vertexIndex[0], &uvIndex[0], &vertexIndex[1], &uvIndex[1], &vertexIndex[2], &uvIndex[2]);
				if (matches == 6)
				{
					verticesIndex.push_back(vertexIndex[0]);
					verticesIndex.push_back(vertexIndex[1]);
					verticesIndex.push_back(vertexIndex[2]);
					texturesIndex.push_back(uvIndex[0]);
					texturesIndex.push_back(uvIndex[1]);
					texturesIndex.push_back(uvIndex[2]);
				}
			}
		}
	}

	// Set position and scale
	normalizePointList(vertices, pos, scale, rot_angle, rot_axes);

	// Manually compute normals if necessary
	if ((normals.size() == 0 || computeNormals) && verticesIndex.size() > 0)
	{
		normals.resize(verticesIndex.size() / 3, glm::vec3(0.0, 0.0, 0.0));
		for (unsigned int i = 0; i < verticesIndex.size(); i += 3)
		{
			GLuint ia = verticesIndex[i];
			GLuint ib = verticesIndex[i + 1];
			GLuint ic = verticesIndex[i + 2];
			glm::vec3 normal = glm::normalize(glm::cross(
				glm::vec3(vertices[ib]) - glm::vec3(vertices[ia]),
				glm::vec3(vertices[ic]) - glm::vec3(vertices[ia])));
			normals[ia] = normals[ib] = normals[ic] = normal;
		}
	}

	fflush(file);
	
	return true;
}

// Build a shader from a string
GLuint buildShader(GLenum const shaderType, std::string const src)
{
	GLuint shader = glCreateShader(shaderType);

	const char* ptr = src.c_str();
	GLint length = static_cast<GLint>(src.length());

	glShaderSource(shader, 1, &ptr, &length);

	glCompileShader(shader);

	GLint res;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
	if (!res)
	{
		std::cerr << "shader compilation error" << std::endl;

		char message[1000];

		GLsizei readSize;
		glGetShaderInfoLog(shader, 1000, &readSize, message);
		message[999] = '\0';

		std::cerr << message << std::endl;

		glfwTerminate();
		exit(-1);
	}

	return shader;
}

// read a file content into a string
std::string fileGetContents(const std::string path)
{
	std::ifstream t(path);
	std::stringstream buffer;
	buffer << t.rdbuf();

	return buffer.str();
}

// build a program with a vertex shader and a fragment shader
GLuint buildProgram(const std::string vertexFile, const std::string fragmentFile)
{
	auto vshader = buildShader(GL_VERTEX_SHADER, fileGetContents(vertexFile));
	auto fshader = buildShader(GL_FRAGMENT_SHADER, fileGetContents(fragmentFile));

	GLuint program = glCreateProgram();

	glAttachShader(program, vshader);
	glAttachShader(program, fshader);

	glLinkProgram(program);

	GLint res;
	glGetProgramiv(program, GL_LINK_STATUS, &res);
	if (!res)
	{
		std::cerr << "program link error" << std::endl;

		char message[1000];

		GLsizei readSize;
		glGetProgramInfoLog(program, 1000, &readSize, message);
		message[999] = '\0';

		std::cerr << message << std::endl;

		glfwTerminate();
		exit(-1);
	}

	return program;
}

/****************************************************************
******* INTERESTING STUFFS HERE ********************************
***************************************************************/

// Store the global state of your program
struct
{
	GLuint program; // a shader
	GLuint vao; // a vertex array object

	clock_t start;
	glm::vec4 p;
	glm::vec3 lightPos;
	GLfloat near, far;
	GLuint buffer, bufferIndex;
	GLuint normalsBuffer, normalsBufferIndex;
	GLuint colorBuffer, colorBufferIndex;
	GLuint textureNumberBuffer;
	GLuint texturesBuffer[4];
	GLuint texturesSamplerBuffer[4];

	GLuint fbo;
	GLuint program_fbo;
	GLuint framebuffer;
	GLuint renderedTexture;

	GLint size;
	Mesh* mesh;

	GLuint program_sun;
	GLint sun_size;
	GLuint buffer_sun;
	GLuint vao_sun;
	GLfloat sun_radius;

	GLuint buffer_portal[2];
	GLint mirror_size;
	GLuint vao_portal[2];
	GLuint renderedMirrorTexture;
	GLuint framebuffer_mirror;
	GLuint program_mirror;
	glm::vec3 color_mirror;
	Mesh *portal1;
	Mesh *portal2;
	glm::mat4 portal1_model;
	glm::mat4 portal2_model;

	// Moving
	float rot;
	int axe = 0;
	glm::vec3 portal_pos;
	glm::vec3 portal2_pos;

	glm::mat4 projection;
	glm::mat4 depthProj;
	glm::mat4 biasMatrix;
} gs;

Mesh* createMesh(const char* filename, const GLushort num_texture, const glm::vec3 &pos = glm::vec3(0.f), const glm::vec3 &scale = glm::vec3(1.f), const float rot_angle = 0.f, const glm::vec3 &rot_axes = glm::vec3(1.f), bool computeNormals = false)
{
	Mesh *m = new Mesh();
	if (load_obj(filename, m->vertices, m->textures, m->normals, m->verticesIndex, m->texturesIndex, m->normalsIndex, m->texturesNumber, num_texture, pos, scale, rot_angle, rot_axes, computeNormals))
		return m;
	return nullptr;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_LEFT)
	{
		gs.portal_pos.z += step_pos;
	}
	else if (key == GLFW_KEY_RIGHT)
	{
		gs.portal_pos.z -= step_pos;
	}

	if (key == GLFW_KEY_UP)
	{
		gs.portal_pos.x += step_pos;
	}
	else if (key == GLFW_KEY_DOWN)
	{
		gs.portal_pos.x -= step_pos;
	}

	glm::vec3 pos = gs.portal_pos;
	pos.z = -pos.z;
	gs.portal2_pos = pos;
}

void init()
{
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glClearDepth(1.0f); // Set background depth to farthest
	glEnable(GL_DEPTH_TEST); // Enable depth testing for z-culling
	glDepthFunc(GL_LESS); // Set the type of depth-test
	glShadeModel(GL_SMOOTH); // Enable smooth shading
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // Nice perspective corrections

	// Build our program and an empty VAO
	gs.program = buildProgram("basic.vsl", "basic.fsl");
	gs.program_fbo = buildProgram("basic.vsl", "texture.fsl");
	gs.program_sun = buildProgram("basic.vsl", "sun.fsl");
	gs.program_mirror = buildProgram("mirror.vsl", "mirror.fsl");

	// Global parameters
	gs.start = clock();
	gs.p = glm::vec4(0.f, 2.f, 0.f, 0.f);
	gs.lightPos = glm::vec3(2.f, 9.f, -3.f);
	gs.near = 0.1f;
	gs.far = 100.f;
	gs.sun_radius = 1.f;
	gs.color_mirror = glm::vec3(1.f, 1.f, 1.f);
	gs.rot = 0.f;
	gs.portal_pos = glm::vec3(0.f, 2.5f, -7.6f);
	glm::vec3 pos = gs.portal_pos;
	pos.z = -pos.z;
	gs.portal2_pos = pos;

	gs.depthProj = glm::ortho(-10.f, 10.f, -10.f, 10.f, gs.near, gs.far);
	gs.biasMatrix = glm::mat4(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f
		);

	float cube_size = 10.f;
	float size = cube_size * 0.85f;

	gs.mesh = createMesh("Stormtrooper.obj", 0, glm::vec3(-1.f, cube_size * 0.127f, -1.5f), glm::vec3(1.2f), 180.f, glm::vec3(0.f, 1.f, 0.f));

	if (gs.mesh != nullptr)
	{
		Mesh *cube = createMesh("RubiksCube.obj", 1, glm::vec3(0.f, -size, 0.f), glm::vec3(cube_size));
		if (cube != nullptr)
		{
			gs.mesh->merge(cube);
			translate(cube->vertices, glm::vec3(2.f * size, size * 2.f, 0.f));
			gs.mesh->merge(cube);
			translate(cube->vertices, glm::vec3(-2.f * size, 0.f, 2.f * size));
			gs.mesh->merge(cube);
			translate(cube->vertices, glm::vec3(-2.f * size, 0.f, -2.f * size));
			gs.mesh->merge(cube);
			translate(cube->vertices, glm::vec3(2.f * size, 0.f, -2.f * size));
			gs.mesh->merge(cube);
			translate(cube->vertices, glm::vec3(0.f, 2.f * size, 2.f * size));
			gs.mesh->merge(cube);
		}

		Mesh *dragon = createMesh("Alduin.obj", 2, glm::vec3(0.f), glm::vec3(1.f), -10.f, glm::vec3(0.f, 0.f, 1.f));
		dragon->SetMeshModel(glm::vec3(0.f, cube_size * 0.4f, 4.f), glm::vec3(1.5f), 45.f, glm::vec3(0.f, 1.f, 0.f));

		if (dragon != nullptr)
		{
			gs.mesh->merge(dragon);
		}

		Mesh *c3po = createMesh("c3po.obj", 3, glm::vec3(3.5f, cube_size * 0.132f, 0.f), glm::vec3(1.2f), 220.f, glm::vec3(0.f, 1.f, 0.f));

		if (c3po != nullptr)
		{
			gs.mesh->merge(c3po);
		}
	}

	if (gs.mesh != nullptr)
	{
		gs.mesh->indexData();
		gs.size = static_cast<GLint>(gs.mesh->vertices_indexed.size());

		Mesh *sun = createMesh("Sphere.obj", -1, glm::vec3(0.f), glm::vec3(gs.sun_radius));
		sun->indexData();
		gs.sun_size = static_cast<GLint>(sun->vertices_indexed.size());

		gs.portal1 = Mesh::Quadrangle(glm::vec3(-5.F, -0.5f, -5.F),
			glm::vec3(-5.F, -0.5f, 5.F),
			glm::vec3(5.F, -0.5f, -5.F),
			glm::vec3(5.F, -0.5f, 5.F),
			-1);

 		gs.portal1->indexData();
		gs.mirror_size = static_cast<GLint>(gs.portal1->vertices_indexed.size());

		gs.portal2 = Mesh::Quadrangle(glm::vec3(-5.F, -0.5f, -5.F),
			glm::vec3(-5.F, -0.5f, 5.F),
			glm::vec3(5.F, -0.5f, -5.F),
			glm::vec3(5.F, -0.5f, 5.F),
			-1);

		gs.portal2->indexData();

		// Vertex buffer sun
		glGenBuffers(1, &gs.buffer_sun);
		glBindBuffer(GL_ARRAY_BUFFER, gs.buffer_sun);
		glBufferData(GL_ARRAY_BUFFER, sun->vertices_indexed.size() * sizeof(glm::vec4), &sun->vertices_indexed[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Vertex buffer
		glGenBuffers(1, &gs.buffer);
		glBindBuffer(GL_ARRAY_BUFFER, gs.buffer);
		glBufferData(GL_ARRAY_BUFFER, gs.mesh->vertices_indexed.size() * sizeof(glm::vec4), &gs.mesh->vertices_indexed[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Vertex buffer portal 1
		glGenBuffers(1, &gs.buffer_portal[0]);
		glBindBuffer(GL_ARRAY_BUFFER, gs.buffer_portal[0]);
		glBufferData(GL_ARRAY_BUFFER, gs.portal1->vertices_indexed.size() * sizeof(glm::vec4), &gs.portal1->vertices_indexed[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Vertex buffer portal 2
		glGenBuffers(1, &gs.buffer_portal[1]);
		glBindBuffer(GL_ARRAY_BUFFER, gs.buffer_portal[1]);
		glBufferData(GL_ARRAY_BUFFER, gs.portal2->vertices_indexed.size() * sizeof(glm::vec4), &gs.portal2->vertices_indexed[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		// Normals buffer
		glGenBuffers(1, &gs.normalsBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, gs.normalsBuffer);
		glBufferData(GL_ARRAY_BUFFER, gs.mesh->normals_indexed.size() * sizeof(glm::vec3), &gs.mesh->normals_indexed[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		// Textures buffer
		glGenBuffers(1, &gs.colorBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, gs.colorBuffer);
		glBufferData(GL_ARRAY_BUFFER, gs.mesh->textures_indexed.size() * sizeof(glm::vec2), &gs.mesh->textures_indexed[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Texture number buffer
		glGenBuffers(1, &gs.textureNumberBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, gs.textureNumberBuffer);
		glBufferData(GL_ARRAY_BUFFER, gs.mesh->texturesNumber_indexed.size() * sizeof(GLuint), &gs.mesh->texturesNumber_indexed[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glCreateVertexArrays(1, &gs.vao);
		glBindVertexArray(gs.vao);

		// Vertex shader input vec4 pt
		glBindBuffer(GL_ARRAY_BUFFER, gs.buffer);
		glEnableVertexArrayAttrib(gs.vao, 11);
		glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, 0, (void *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Vertex shader input vec3 normal
		glBindBuffer(GL_ARRAY_BUFFER, gs.normalsBuffer);
		glEnableVertexArrayAttrib(gs.vao, 12);
		glVertexAttribPointer(12, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Vertex shader input vec2 uv
		glBindBuffer(GL_ARRAY_BUFFER, gs.colorBuffer);
		glEnableVertexArrayAttrib(gs.vao, 13);
		glVertexAttribPointer(13, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//  Vertex shader input int num_sampler
		glBindBuffer(GL_ARRAY_BUFFER, gs.textureNumberBuffer);
		glEnableVertexArrayAttrib(gs.vao, 14);
		glVertexAttribIPointer(14, 1, GL_UNSIGNED_INT, 0, (void *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);


		// === Depth texture ===
		glGenTextures(1, &gs.renderedTexture);
		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, gs.renderedTexture);

		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, SHADOW_WIDTH, SHADOW_HEIGHT);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glGenFramebuffers(1, &gs.framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gs.framebuffer);

		// Instruct openGL that we won't bind a color texture with the currently bound FBO
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		// attach the texture to FBO depth attachment point
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gs.renderedTexture, 0);

		// Always check that our framebuffer is ok
		assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

		// Render to our framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gs.renderedTexture);
		glGenerateMipmap(GL_TEXTURE_2D);


		// === Textures ===
		glGenTextures(4, gs.texturesBuffer);
		glGenSamplers(4, gs.texturesSamplerBuffer);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gs.texturesBuffer[0]);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindSampler(1, gs.texturesSamplerBuffer[0]);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL); // Decal tarnish

		gs.texturesBuffer[0] = SOIL_load_OGL_texture
			(
			"Stormtrooper.tga",
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
			);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gs.texturesBuffer[1]);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindSampler(2, gs.texturesSamplerBuffer[1]);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL); // Decal tarnish

		gs.texturesBuffer[1] = SOIL_load_OGL_texture
			(
			"Cube.jpg",
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
			);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gs.texturesBuffer[2]);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindSampler(3, gs.texturesSamplerBuffer[2]);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL); // Decal tarnish

		gs.texturesBuffer[2] = SOIL_load_OGL_texture
			(
			"Alduin.png",
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
			);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, gs.texturesBuffer[3]);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindSampler(4, gs.texturesSamplerBuffer[3]);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL); // Decal tarnish

		gs.texturesBuffer[3] = SOIL_load_OGL_texture
			(
				"c3po.tga",
				SOIL_LOAD_AUTO,
				SOIL_CREATE_NEW_ID,
				SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
				);

		glBindVertexArray(0);

		// === FBO === 
		glCreateVertexArrays(1, &gs.fbo);
		glBindVertexArray(gs.fbo);

		// Vertex shader input vec4 pt
		glBindBuffer(GL_ARRAY_BUFFER, gs.buffer);
		glEnableVertexArrayAttrib(gs.fbo, 11);
		glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, 0, (void *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(0);


		// === Sun === 
		glCreateVertexArrays(1, &gs.vao_sun);
		glBindVertexArray(gs.vao_sun);

		// Vertex shader input vec4 pt
		glBindBuffer(GL_ARRAY_BUFFER, gs.buffer_sun);
		glEnableVertexArrayAttrib(gs.vao_sun, 11);
		glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, 0, (void *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Vertex shader input vec3 normal
		glBindBuffer(GL_ARRAY_BUFFER, gs.normalsBuffer);
		glEnableVertexArrayAttrib(gs.vao_sun, 12);
		glVertexAttribPointer(12, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(0);


		// === Mirror === 
		// Portal 1
		glCreateVertexArrays(1, &gs.vao_portal[0]);
		glBindVertexArray(gs.vao_portal[0]);

		// Vertex shader input vec4 pt
		glBindBuffer(GL_ARRAY_BUFFER, gs.buffer_portal[0]);
		glEnableVertexArrayAttrib(gs.vao_portal[0], 11);
		glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, 0, (void *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(0);

		// Portal 2
		glCreateVertexArrays(1, &gs.vao_portal[1]);
		glBindVertexArray(gs.vao_portal[1]);

		// Vertex shader input vec4 pt
		glBindBuffer(GL_ARRAY_BUFFER, gs.buffer_portal[1]);
		glEnableVertexArrayAttrib(gs.vao_portal[1], 11);
		glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, 0, (void *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(0);
	}
	else
	{
		std::cerr << "program loading mesh error" << std::endl;

		glfwTerminate();
		exit(-1);
	}
}

void draw_scene(const glm::mat4x4 &mat_cam, const glm::mat4x4 &mat_depth_cam, const glm::mat4x4 &model)
{
	GLuint texLoc;

	glUseProgram(gs.program);

	glProgramUniformMatrix4fv(gs.program, 1, 1, GL_FALSE, &mat_cam[0][0]);
	glProgramUniformMatrix4fv(gs.program, 2, 1, GL_FALSE, &mat_depth_cam[0][0]);
	glProgramUniformMatrix4fv(gs.program, 3, 1, GL_FALSE, &model[0][0]);
	glProgramUniform3fv(gs.program, 4, 1, &cam.position[0]);
	glProgramUniform3fv(gs.program, 5, 1, &gs.lightPos[0]);

	glBindVertexArray(gs.vao);
	{
		texLoc = glGetUniformLocation(gs.program, "shadow_map");
		glUniform1i(texLoc, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gs.texturesBuffer[0]);
		texLoc = glGetUniformLocation(gs.program, "texture_sampler[0]");
		glUniform1i(texLoc, 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gs.texturesBuffer[1]);
		texLoc = glGetUniformLocation(gs.program, "texture_sampler[1]");
		glUniform1i(texLoc, 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gs.texturesBuffer[2]);
		texLoc = glGetUniformLocation(gs.program, "texture_sampler[2]");
		glUniform1i(texLoc, 3);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, gs.texturesBuffer[3]);
		texLoc = glGetUniformLocation(gs.program, "texture_sampler[3]");
		glUniform1i(texLoc, 4);

		glDrawArrays(GL_TRIANGLES, 0, gs.size);
		//glDrawElements(GL_TRIANGLES, gs.mesh->verticesIndex.size(), GL_UNSIGNED_INT, NULL);	
	}

	glBindVertexArray(0);
	glUseProgram(0);
}

void draw_light(const glm::mat4x4 &mat_cam, const glm::mat4x4 &model)
{
	glUseProgram(gs.program_sun);

	glProgramUniformMatrix4fv(gs.program_sun, 1, 1, GL_FALSE, &mat_cam[0][0]);
	glProgramUniformMatrix4fv(gs.program_sun, 3, 1, GL_FALSE, &model[0][0]);
	glProgramUniform3fv(gs.program_sun, 4, 1, &cam.position[0]);

	glBindVertexArray(gs.vao_sun);
	{
		glDrawArrays(GL_TRIANGLES, 0, gs.sun_size);
	}
}

void draw_shadow(const glm::mat4x4 &mat_depth_cam, const glm::mat4x4 &model)
{
	glUseProgram(gs.program_fbo);

	glProgramUniformMatrix4fv(gs.program_fbo, 1, 1, GL_FALSE, &mat_depth_cam[0][0]);
	glProgramUniformMatrix4fv(gs.program_fbo, 3, 1, GL_FALSE, &model[0][0]);

	glBindVertexArray(gs.vao);
	{
		glDrawArrays(GL_TRIANGLES, 0, gs.size);
	}

	glBindVertexArray(0);
	glUseProgram(0);
}

void draw_mirror(const int num, const glm::mat4x4 &mat_cam, const glm::mat4x4 &model)
{
	glUseProgram(gs.program_mirror);
	glProgramUniformMatrix4fv(gs.program_mirror, 6, 1, GL_FALSE, &mat_cam[0][0]);
	glProgramUniformMatrix4fv(gs.program_mirror, 7, 1, GL_FALSE, &model[0][0]);
	glProgramUniform3fv(gs.program_mirror, 8, 1, &gs.color_mirror[0]);

	glBindVertexArray(gs.vao_portal[num]);
	{
		glDrawArrays(GL_TRIANGLES, 0, gs.mirror_size);
	}

	glBindVertexArray(0);
	glUseProgram(0);
}


// Checks whether the line defined by two points la and lb intersects the portal.
int portal_intersection(glm::vec4 &la, glm::vec4 &lb, glm::mat4 &portal_model, std::vector<glm::vec4> &vertices)
{
	// Camera moved
	if (la != lb) 
	{  
		// Check for intersection with each of the portal's 2 front triangles
		for (int i = 0; i < 2; i++) 
		{
			// Portal coordinates in world view
			glm::vec4
				p0 = portal_model * vertices[i * 3 + 0],
				p1 = portal_model * vertices[i * 3 + 1],
				p2 = portal_model * vertices[i * 3 + 2];

			// Solve line-plane intersection using parametric form
			glm::vec3 tuv =
				glm::inverse(glm::mat3(glm::vec3(la.x - lb.x, la.y - lb.y, la.z - lb.z),
					glm::vec3(p1.x - p0.x, p1.y - p0.y, p1.z - p0.z),
					glm::vec3(p2.x - p0.x, p2.y - p0.y, p2.z - p0.z)))
				* glm::vec3(la.x - p0.x, la.y - p0.y, la.z - p0.z);
			float t = tuv.x, u = tuv.y, v = tuv.z;

			// Intersection with the plane
			if (t >= 0 - 1e-6 && t <= 1 + 1e-6) 
			{
				// Intersection with the triangle
				if (u >= 0 - 1e-6 && u <= 1 + 1e-6 && v >= 0 - 1e-6 && v <= 1 + 1e-6 && (u + v) <= 1 + 1e-6) 
				{
					return 1;
				}
			}
		}
	}
	return 0;
}

glm::mat4x4 getMirrorModel(const glm::mat4x4 & model)
{
	glm::mat4x4 modelTranslate = glm::translate(glm::mat4x4(1), glm::vec3(0.f, 0.f, -15.6f));
	glm::mat4x4 modelScale = glm::scale(glm::mat4x4(1), glm::vec3(1.f, 1.f, -1.f));
	glm::mat4x4 modelRotate = glm::rotate(glm::mat4x4(1), glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f));
	return modelTranslate * modelRotate * modelScale * model;
}

void render(GLFWwindow* window)
{
	float c = (float)(clock() - gs.start) / CLOCKS_PER_SEC;
	gs.lightPos = glm::vec3(4.f * sin(c * 0.5f), 6.f, 4.f * cos(c * 0.5f));
	glm::mat4 prev_cam = cam.view;

	cam.update(window);
	
	glm::mat4x4 model = glm::mat4x4(1.f);
	
	// Camera
	gs.projection = glm::perspective(glm::radians(cam.fov), (float)WIDTH / HEIGHT, gs.near, gs.far);
	cam.view = glm::lookAt(cam.position, cam.position + cam.direction, cam.up);
	glm::mat4x4 mvp = gs.projection * cam.view;

	// Compute the MVP matrix from the light's point of view
	glm::mat4 depthView = glm::lookAt(glm::vec3(gs.lightPos), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
	glm::mat4 depthMVP = gs.depthProj * depthView;

	glm::mat4 depthBiasMVP = gs.biasMatrix * depthMVP;

	// For portal intersection test
	glm::vec4 la = glm::inverse(prev_cam) * glm::vec4(0.0, 0.0, 0.0, 1.0);
	glm::vec4 lb = glm::inverse(cam.view) * glm::vec4(0.0, 0.0, 0.0, 1.0);

	// Shadow map
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, gs.framebuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	draw_shadow(depthMVP, model);

	// Scene
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	glViewport(0, 0, width, height);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, gs.renderedTexture);
	glGenerateMipmap(GL_TEXTURE_2D);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	draw_scene(mvp, depthBiasMVP, model);

	// Light
	model = glm::translate(model, glm::vec3(gs.lightPos.x, gs.lightPos.y, gs.lightPos.z));
	draw_light(mvp, model);

	// Portal 1
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 0xFF); // Set any stencil to 1
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(0xFF); // Write to stencil buffer
	glDepthMask(GL_FALSE); // Don't write to depth buffer
	glClear(GL_STENCIL_BUFFER_BIT); // Clear stencil buffer (0 by default)

	glm::mat4 mat_scale = glm::scale(glm::vec3(1.0f, 1.0f, 0.5f));

	gs.portal1_model = glm::translate(gs.portal_pos) * glm::rotate(glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f)) * mat_scale;
	gs.portal2_model = glm::translate(gs.portal2_pos) * glm::rotate(glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f)) * mat_scale;

	draw_mirror(0, mvp, gs.portal1_model);

	if (portal_intersection(la, lb, gs.portal1_model, gs.portal1->vertices_indexed))
	{
		glm::vec3 offset((cam.position - gs.portal_pos) * glm::vec3(1.f, 1.f, 0.f));
		cam.position = gs.portal2_pos + offset;
	}
	else if (portal_intersection(la, lb, gs.portal2_model, gs.portal2->vertices_indexed))
	{
		glm::vec3 offset((cam.position - gs.portal2_pos) * glm::vec3(1.f, 1.f, 0.f));
		cam.position = gs.portal_pos + offset;
	}

	// Stencil scene 1
	glm::vec3 pos = gs.portal_pos;
	pos.y = pos.y * 0.2f;
	pos.x = pos.x * 0.f;
	pos.z = pos.z * 2.1f;

	glm::vec3 pos2 = gs.portal2_pos;
	pos2.y = pos2.y * 0.2f;
	pos2.x = pos2.x * 0.f;
	pos2.z = pos2.z * 2.1f;
	
	glm::mat4 model_stencil1 = glm::translate(pos) * glm::scale(glm::vec3(1.0f, 0.8f, 1.0f)) * glm::scale(glm::vec3(1.f, 1.f, -1.f));
	glm::mat4 model_stencil2 = glm::translate(pos2)  * glm::scale(glm::vec3(1.0f, 0.8f, 1.0f)) * glm::scale(glm::vec3(1.f, 1.f, -1.f));

	model = model_stencil1;

	glStencilFunc(GL_EQUAL, 1, 0xFF); // Pass test if stencil value is 1
	glStencilMask(0x00); // Don't write anything to stencil buffer
	glDepthMask(GL_TRUE); // Write to depth buffer
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, gs.renderedTexture);
	glGenerateMipmap(GL_TEXTURE_2D);
	glClear(GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_FRONT);

	draw_scene(mvp, depthBiasMVP, model);

	// Stencil Light 1
	model = glm::translate(model, glm::vec3(gs.lightPos.x, gs.lightPos.y, gs.lightPos.z));
	draw_light(mvp, model);

	glCullFace(GL_BACK);
	glDisable(GL_STENCIL_TEST);

	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Portal 2
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 0xFF); // Set any stencil to 1
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(0xFF); // Write to stencil buffer
	glDepthMask(GL_FALSE); // Don't write to depth buffer
	glClear(GL_STENCIL_BUFFER_BIT); // Clear stencil buffer (0 by default)
	
	draw_mirror(1, mvp, gs.portal2_model);

	// Stencil scene 2
	model = model_stencil2;
	glStencilFunc(GL_EQUAL, 1, 0xFF); // Pass test if stencil value is 1
	glStencilMask(0x00); // Don't write anything to stencil buffer
	glDepthMask(GL_TRUE); // Write to depth buffer

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, gs.renderedTexture);
	glGenerateMipmap(GL_TEXTURE_2D);
	glClear(GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_FRONT);

	draw_scene(mvp, depthBiasMVP, model);

	// Stencil Light 2
	model = glm::translate(model, glm::vec3(gs.lightPos.x, gs.lightPos.y, gs.lightPos.z));
	draw_light(mvp, model);

	glCullFace(GL_BACK);
	glDisable(GL_STENCIL_TEST);

	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void OnFocus(GLFWwindow* window, int iconify)
{
	cam.WindowFocused = (iconify == GL_TRUE);
}

void OnWheelScroll(GLFWwindow* window, double xoffset, double yoffset)
{
	cam.ChangeFov(yoffset);
}

int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
	{
		std::cerr << "Could not init glfw" << std::endl;
		return -1;
	}

	// This is a debug context, this is slow, but debugs, which is interesting
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	// MSAA
	glfwWindowHint(GLFW_SAMPLES, 8);

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(WIDTH, HEIGHT, "Gamagora Portal", NULL, NULL);
	if (!window)
	{
		std::cerr << "Could not init window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glEnable(GL_MULTISAMPLE);

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		std::cerr << "Could not init GLEW" << std::endl;
		std::cerr << glewGetErrorString(err) << std::endl;
		glfwTerminate();
		return -1;
	}

	// Now that the context is initialised, print some informations
	glInfo(GL_VENDOR);
	glInfo(GL_RENDERER);
	glInfo(GL_VERSION);
	glInfo(GL_SHADING_LANGUAGE_VERSION);

	// And enable debug
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

	glDebugMessageCallback(debug, nullptr);

	// Keyboard events
	glfwSetKeyCallback(window, key_callback);

	// Hide cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	GLFWwindowfocusfun OnFocusPointer = *OnFocus;
	glfwSetWindowFocusCallback(window, OnFocusPointer);

	GLFWscrollfun OnWheelScrollPointer = *OnWheelScroll;
	glfwSetScrollCallback(window, OnWheelScrollPointer);

	// This is our openGL init function which creates ressources
	init();

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		render(window);

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}