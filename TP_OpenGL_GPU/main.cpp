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

#include <glm\glm\glm.hpp>
#include <glm\glm\gtx\transform.hpp>

#include <soil\SOIL.h>

#define WIDTH 640
#define HEIGHT 480

using namespace std;

void render(GLFWwindow*);
void init();

#define glInfo(a) std::cout << #a << ": " << glGetString(a) << std::endl

std::regex obj_regex1("f [0-9]+// [0-9]+// [0-9]+//(.*)");
std::regex obj_regex2("f [0-9]+//[0-9]+ [0-9]+//[0-9]+ [0-9]+//[0-9]+(.*)");
std::regex obj_regex3("f [0-9]+/[0-9]+/[0-9]+ [0-9]+/[0-9]+/[0-9]+ [0-9]+/[0-9]+/[0-9]+[\\r\\n]*");
std::regex obj_regex4("f [0-9]+/[0-9]+(/?) [0-9]+/[0-9]+(/?) [0-9]+/[0-9]+(/?)(.*)");

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

struct Mesh
{
	std::vector<glm::vec4> vertices;
	std::vector<glm::vec2> textures;
	std::vector<glm::vec3> normals;
	std::vector<GLushort> verticesIndex;
	std::vector<GLushort> texturesIndex;
	std::vector<GLint> texturesNumber;
	std::vector<GLushort> normalsIndex;

	void merge(Mesh* m)
	{
		int vertices_size = vertices.size();
		int textures_size = textures.size();
		int normals_size = normals.size();

		for (int i = 0; i < m->vertices.size(); i++)
			vertices.push_back(m->vertices[i]);

		for (int i = 0; i < m->textures.size(); i++)
			textures.push_back(m->textures[i]);

		for (int i = 0; i < m->normals.size(); i++)
			normals.push_back(m->normals[i]);

		for (int i = 0; i < m->verticesIndex.size(); i++)
			verticesIndex.push_back(vertices_size + m->verticesIndex[i]);

		for (int i = 0; i < m->texturesIndex.size(); i++)
			texturesIndex.push_back(textures_size + m->texturesIndex[i]);

		for (int i = 0; i < m->normalsIndex.size(); i++)
			normalsIndex.push_back(normals_size + m->normalsIndex[i]);

		for (int i = 0; i < m->texturesNumber.size(); i++)
			texturesNumber.push_back(m->texturesNumber[i]);
	}
};

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

void normalizePointList(std::vector<glm::vec4> &points, const glm::vec3 &pos, const glm::vec3 &scale)
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

	float length = max.length();

	for (unsigned int i = 0; i < points.size(); i++)
	{
		points[i].x = pos.x + (((points[i].x - minX) - diffX * 0.5f) / (length * 0.5f)) * scale.x;
		points[i].y = pos.y + (((points[i].y - minY) - diffY * 0.5f) / (length * 0.5f)) * scale.y;
		points[i].z = pos.z + (((points[i].z - minZ) - diffZ * 0.5f) / (length * 0.5f)) * scale.z;
	}
}

void indexData(std::vector<glm::vec4> &vertices, std::vector<glm::vec2> &textures, std::vector<glm::vec3> &normals, std::vector<GLushort> &verticesIndex, std::vector<GLushort> &texturesIndex, std::vector<GLushort> &normalsIndex, std::vector<GLint> &texturesNumber)
{
	std::vector<glm::vec4> new_vertices;
	std::vector<glm::vec2> new_uvs;
	std::vector<glm::vec3> new_normals;
	std::vector<GLint> new_textures_number;

	if (vertices.size() > 0)
	{
		if (verticesIndex.size() > 0)
		{
			for (unsigned int i = 0; i < verticesIndex.size(); i++)
			{
				unsigned int vertexIndex = verticesIndex[i];
				glm::vec4 vertex = vertices[vertexIndex - 1];
				new_vertices.push_back(vertex);
				GLint num = texturesNumber[vertexIndex - 1];
				new_textures_number.push_back(num);
			}
		}
		else
		{
			for (unsigned int i = 0; i < vertices.size(); i++)
			{
				glm::vec4 vertex = vertices[i];
				new_vertices.push_back(vertex);
				GLint num = texturesNumber[i];
				new_textures_number.push_back(num);
			}
		}
	}

	if (textures.size() > 0)
	{
		if (texturesIndex.size() > 0)
		{
			for (unsigned int i = 0; i < texturesIndex.size(); i++)
			{
				unsigned int uvIndex = texturesIndex[i];
				glm::vec2 uv = textures[uvIndex - 1];
				new_uvs.push_back(uv);
				/*GLint num = texturesNumber[uvIndex - 1];
				new_textures_number.push_back(num);*/
			}
		}
		else
		{
			for (unsigned int i = 0; i < textures.size(); i++)
			{
				glm::vec2 uv = textures[i];
				new_uvs.push_back(uv);
				/*GLint num = texturesNumber[i];
				new_textures_number.push_back(num);*/
			}
		}
	}

	if (normals.size() > 0)
	{
		if (normalsIndex.size() > 0)
		{
			for (unsigned int i = 0; i < normalsIndex.size(); i++)
			{
				unsigned int normalIndex = normalsIndex[i];
				glm::vec3 normal = normals[normalIndex - 1];
				new_normals.push_back(normal);
			}
		}
		else
		{
			for (unsigned int i = 0; i < normals.size(); i++)
			{
				glm::vec3 normal = normals[i];
				new_normals.push_back(normal);
			}
		}
	}

	vertices = new_vertices;
	normals = new_normals;
	textures = new_uvs;
	texturesNumber = new_textures_number;
}

bool load_obj(const char* filename, std::vector<glm::vec4> &vertices, std::vector<glm::vec2> &textures, std::vector<glm::vec3> &normals, std::vector<GLushort> &verticesIndex,
	std::vector<GLushort> &texturesIndex, std::vector<GLushort> &normalsIndex, std::vector<GLint> &texturesNumber, GLint num_texture,
	const bool indexing_data, const glm::vec3 pos = glm::vec3(0.f), const glm::vec3 scale = glm::vec3(1.f))
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
			/*texturesNumber.push_back(num_texture);*/
		}
		else if (strncmp(line, "vn", 2) == 0)
		{
			glm::vec3 normal;
			sscanf_s(line, "vn %f %f %f\n", &normal.x, &normal.y, &normal.z);
			normals.push_back(normal);
		}
		else if (strncmp(line, "f ", 2) == 0)
		{
			GLushort *vertexIndex = new GLushort[3], *uvIndex = new GLushort[3], *normalIndex = new GLushort[3];

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
	normalizePointList(vertices, pos, scale);

	// Reindexing data
	if (indexing_data)
		indexData(vertices, textures, normals, verticesIndex, texturesIndex, normalsIndex, texturesNumber);

	// Manually compute normals if necessary
	if (normals.size() == 0 && verticesIndex.size() > 0)
	{
		normals.resize(vertices.size(), glm::vec3(0.0, 0.0, 0.0));
		for (unsigned int i = 0; i < verticesIndex.size(); i += 3)
		{
			GLushort ia = verticesIndex[i];
			GLushort ib = verticesIndex[i + 1];
			GLushort ic = verticesIndex[i + 2];
			glm::vec3 normal = glm::normalize(glm::cross(
				glm::vec3(vertices[ib]) - glm::vec3(vertices[ia]),
				glm::vec3(vertices[ic]) - glm::vec3(vertices[ia])));
			normals[ia] = normals[ib] = normals[ic] = normal;
		}
	}

	fflush(file);
	
	return true;
}

void indexDataMesh(Mesh *m)
{
	indexData(m->vertices, m->textures, m->normals, m->verticesIndex, m->texturesIndex, m->normalsIndex, m->texturesNumber);
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

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(WIDTH, HEIGHT, "Open Portal", NULL, NULL);
	if (!window)
	{
		std::cerr << "Could not init window" << std::endl;
		glfwTerminate();
		return -1;
	}

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

	glDebugMessageCallback((GLDEBUGPROC)debug, nullptr);

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

// Build a shader from a string
GLuint buildShader(GLenum const shaderType, std::string const src)
{
	GLuint shader = glCreateShader(shaderType);

	const char* ptr = src.c_str();
	GLint length = src.length();

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
	glm::vec3 camPos;
	glm::vec3 lightPos;
	GLfloat near, far, fov;
	GLuint buffer;
	GLuint normalsBuffer;
	GLuint colorbuffer;
	GLuint textureNumberBuffer;
	GLuint texturesBuffer[3];
	GLuint texturesSamplerBuffer[3];

	GLuint fbo;
	GLuint program_fbo;
	GLuint framebuffer;
	GLuint renderedTexture;

	GLfloat size;
} gs;

Mesh* createMesh(const char* filename, const bool index_data, GLushort num_texture, const glm::vec3 pos = glm::vec3(0.f), const glm::vec3 scale = glm::vec3(1.f))
{
	Mesh *m = new Mesh();
	if (load_obj(filename, m->vertices, m->textures, m->normals, m->verticesIndex, m->texturesIndex, m->normalsIndex, m->texturesNumber, num_texture, index_data, pos, scale))
		return m;
	return nullptr;
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
	gs.program_fbo = buildProgram("texture.vsl", "texture.fsl");

	// Global parameters
	gs.start = clock();
	gs.p = glm::vec4(0.f, 1.f, 0.f, 0.f);
	gs.camPos = glm::vec3(0.f, 1.f, -7.f); 
	gs.lightPos = glm::vec3(2.f, 5.f, -3.f); // 0.f, 5.f, 0.f
	gs.near = 0.1f;
	gs.far = 100.f;
	gs.fov = 45.f;

	float cube_size = 10.f;
	float size = cube_size * 0.85f;

	Mesh *mesh = createMesh("Cube.obj", false, 1, glm::vec3(0.f, 0.25f + cube_size * 0.11f, 0.f), glm::vec3(1.f));

	//Mesh *mesh = createMesh("Stormtrooper.obj", false, 0, glm::vec3(2.f, cube_size * 0.11f, 0.f), glm::vec3(1.f));

	if (mesh != nullptr)
	{
		Mesh *cube = createMesh("RubiksCube.obj", false, 1, glm::vec3(0.f, -size, 0.f), glm::vec3(cube_size));
		if (cube != nullptr)
		{
			mesh->merge(cube);
			/*translate(cube->vertices, glm::vec3(2.f * size, size * 2.f, 0.f));
			mesh->merge(cube);
			translate(cube->vertices, glm::vec3(-2.f * size, 0.f, 2.f * size));
			mesh->merge(cube);
			translate(cube->vertices, glm::vec3(-2.f * size, 0.f, -2.f * size));
			mesh->merge(cube);
			translate(cube->vertices, glm::vec3(2.f * size, 0.f, -2.f * size));
			mesh->merge(cube);
			translate(cube->vertices, glm::vec3(0.f, 2.f * size, 2.f * size));
			mesh->merge(cube);*/
		}

		/*Mesh *dragon = createMesh("Alduin.obj", false, 2, glm::vec3(-4.f, cube_size * 0.2f, 0.f), glm::vec3(1.f));
		if (dragon != nullptr)
		{
			mesh->merge(dragon);
		}*/
	}

	if (mesh != nullptr)
	{
		indexDataMesh(mesh);
		gs.size = mesh->vertices.size();

		// Vertex buffer
		glGenBuffers(1, &gs.buffer);
		glBindBuffer(GL_ARRAY_BUFFER, gs.buffer);
		glBufferData(GL_ARRAY_BUFFER, mesh->vertices.size() * sizeof(glm::vec4), &mesh->vertices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Normals buffer
		glGenBuffers(1, &gs.normalsBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, gs.normalsBuffer);
		glBufferData(GL_ARRAY_BUFFER, mesh->normals.size() * sizeof(glm::vec3), &mesh->normals[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Textures buffer
		glGenBuffers(1, &gs.colorbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, gs.colorbuffer);
		glBufferData(GL_ARRAY_BUFFER, mesh->textures.size() * sizeof(glm::vec2), &mesh->textures[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Texture number buffer
		glGenBuffers(1, &gs.textureNumberBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, gs.textureNumberBuffer);
		glBufferData(GL_ARRAY_BUFFER, mesh->texturesNumber.size() * sizeof(GLint), &mesh->texturesNumber[0], GL_STATIC_DRAW);
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
		glBindBuffer(GL_ARRAY_BUFFER, gs.colorbuffer);
		glEnableVertexArrayAttrib(gs.vao, 13);
		glVertexAttribPointer(13, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//  Vertex shader input int num_sampler
		glBindBuffer(GL_ARRAY_BUFFER, gs.textureNumberBuffer);
		glEnableVertexArrayAttrib(gs.vao, 14);
		glVertexAttribIPointer(14, 1, GL_INT, 0, (void *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Textures
		glGenTextures(2, gs.texturesBuffer);
		glGenSamplers(2, gs.texturesSamplerBuffer);

		gs.texturesBuffer[0] = SOIL_load_OGL_texture
			(
			"Stormtrooper.tga",
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
			);

		gs.texturesBuffer[1] = SOIL_load_OGL_texture
			(
			"Cube.jpg",
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
			);

		gs.texturesBuffer[2] = SOIL_load_OGL_texture
			(
			"Alduin.png",
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
			);

		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, gs.texturesBuffer[0]);
		glBindSampler(0, gs.texturesSamplerBuffer[0]);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL); // Decal tarnish
		//glDisable(GL_TEXTURE0);

		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, gs.texturesBuffer[1]);
		glBindSampler(1, gs.texturesSamplerBuffer[1]);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL); // Decal tarnish

		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, gs.texturesBuffer[2]);
		glBindSampler(2, gs.texturesSamplerBuffer[2]);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL); // Decal tarnish
		//glDisable(GL_TEXTURE2);

		// The texture we're going to render to
		glGenTextures(1, &gs.renderedTexture);
		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, gs.renderedTexture);

		// GL_LINEAR does not make sense for depth texture. However, next tutorial shows usage of GL_LINEAR and PCF
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// Remove artifact on the edges of the shadowmap
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, WIDTH, HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		//glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, WIDTH, HEIGHT);

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

		glActiveTexture(GL_TEXTURE3);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, gs.renderedTexture);

		glBindVertexArray(0);

		// FBO
		glCreateVertexArrays(1, &gs.fbo);
		glBindVertexArray(gs.fbo);

		// Vertex shader input vec4 pt
		glBindBuffer(GL_ARRAY_BUFFER, gs.buffer);
		glEnableVertexArrayAttrib(gs.fbo, 11);
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

void render(GLFWwindow* window)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	float c = (float)(clock() - gs.start) / CLOCKS_PER_SEC;
	//glProgramUniform1f(gs.program, 5, std::abs(cos(c)));

	glm::vec3 pt(gs.p.x, gs.p.y, gs.p.z);
	glm::mat4x4 projection = glm::perspective(glm::radians(gs.fov), (float)(WIDTH / (float)HEIGHT), gs.near, gs.far);
	glm::mat4x4 view = glm::lookAt(gs.camPos, pt, glm::vec3(0.f, 1.f, 0.f));
	glm::mat4x4 model = glm::mat4(1.0f);
	glm::mat4x4 mvp = projection * view * model;

	// Compute the MVP matrix from the light's point of view
	glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(gs.fov), (float)(WIDTH / (float)HEIGHT), gs.near, gs.far); //glm::ortho<float>(-10, 10, -10, 10, -10, 20);
	glm::mat4 depthViewMatrix = glm::lookAt(gs.lightPos, glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
	glm::mat4 depthModelMatrix = glm::mat4(1.0f);
	glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

	glm::mat4 biasMatrix(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f
		);
	glm::mat4 depthBiasMVP = biasMatrix * depthMVP;

	GLuint texLoc;

	glProgramUniformMatrix4fv(gs.program_fbo, 1, 1, GL_FALSE, &depthMVP[0][0]);

	glProgramUniformMatrix4fv(gs.program, 1, 1, GL_FALSE, &mvp[0][0]);
	glProgramUniformMatrix4fv(gs.program, 2, 1, GL_FALSE, &model[0][0]);
	glProgramUniformMatrix4fv(gs.program, 3, 1, GL_FALSE, &view[0][0]);
	glProgramUniform3fv(gs.program, 4, 1, &gs.lightPos[0]);
	glProgramUniformMatrix4fv(gs.program, 5, 1, GL_FALSE, &depthBiasMVP[0][0]);

	//glBindFramebuffer(GL_FRAMEBUFFER, gs.framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(gs.program_fbo);
	glBindVertexArray(gs.fbo);
	{
		glDrawArrays(GL_TRIANGLES, 0, gs.size);
	}

	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glUseProgram(gs.program);
	//glBindVertexArray(gs.vao);
	//{
	//	texLoc = glGetUniformLocation(gs.program, "texture_sampler[0]");
	//	glUniform1i(texLoc, 0);
	//	texLoc = glGetUniformLocation(gs.program, "texture_sampler[1]");
	//	glUniform1i(texLoc, 1);
	//	texLoc = glGetUniformLocation(gs.program, "texture_sampler[2]");
	//	glUniform1i(texLoc, 2);
	//	GLuint texLoc = glGetUniformLocation(gs.program, "shadow_map");
	//	glUniform1i(texLoc, 3);
	//	
	//	glDrawArrays(GL_TRIANGLES, 0, gs.size);
	//	//glDrawElements()
	//}

	gs.camPos.x = 5.f * cos(c);
	gs.camPos.z = 5.f * sin(c);

	glBindVertexArray(0);
	glUseProgram(0);
}