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
#include <glm/gtc/matrix_transform.hpp>

#include <soil\SOIL.h>

#define WIDTH 640
#define HEIGHT 480
#define SHADOW_WIDTH 8192
#define SHADOW_HEIGHT 8192
#define PI 3.14159265359

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
	std::vector<glm::vec4> vertices_indexed;
	std::vector<glm::vec2> textures_indexed;
	std::vector<glm::vec3> normals_indexed;
	std::vector<GLuint> verticesIndex;
	std::vector<GLuint> texturesIndex;
	std::vector<GLuint> normalsIndex;
	std::vector<GLuint> texturesNumber;
	std::vector<GLuint> texturesNumber_indexed;

	Mesh() :
		vertices(std::vector<glm::vec4>()),
		textures(std::vector<glm::vec2>()),
		normals(std::vector<glm::vec3>()),
		verticesIndex(std::vector<GLuint>()),
		texturesIndex(std::vector<GLuint>()),
		normalsIndex(std::vector<GLuint>()),
		texturesNumber(std::vector<GLuint>())
	{

	}

	Mesh(std::vector<glm::vec4> _vertices, std::vector<glm::vec2> _textures, std::vector<glm::vec3> _normals, 
		std::vector<GLuint> _verticesIndex, std::vector<GLuint> _texturesIndex, std::vector<GLuint> _normalsIndex, std::vector<GLuint> _texturesNumber) :
		vertices(_vertices),
		textures(_textures),
		normals(_normals),
		verticesIndex(_verticesIndex),
		texturesIndex(_texturesIndex),
		normalsIndex(_normalsIndex),
		texturesNumber(_texturesNumber)
	{

	}

	void merge(Mesh* m)
	{
		int vertices_size = static_cast<int>(vertices.size());
		int textures_size = static_cast<int>(textures.size());
		int normals_size = static_cast<int>(normals.size());

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

	static Mesh* Quadrangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const int num_texture)
	{
		std::vector<glm::vec4> points(4);
		points[0] = glm::vec4(p0.x, p0.y, p0.z, 1.f);
		points[1] = glm::vec4(p1.x, p1.y, p1.z, 1.f);
		points[2] = glm::vec4(p2.x, p2.y, p2.z, 1.f);
		points[3] = glm::vec4(p3.x, p3.y, p3.z, 1.f);

		std::vector<GLuint> faces(6);
		faces[0] = 1;
		faces[1] = 2;
		faces[2] = 3;
		faces[3] = 2;
		faces[4] = 4;
		faces[5] = 3;

		glm::vec3 normalT1 = glm::normalize(glm::cross(p1 - p0, p3 - p0));
		glm::vec3 normalT2 = glm::normalize(glm::cross(p1 - p2, p3 - p2));
		glm::vec3 normalMiddle = glm::normalize((normalT1 + normalT2) * 0.5f);
		std::vector<glm::vec3> normals(4);
		normals[0] = normalT1;
		normals[1] = normalMiddle;
		normals[2] = normalMiddle;
		normals[3] = normalT2;

		std::vector<GLuint> facesNormals(6);
		facesNormals[0] = 1;
		facesNormals[1] = 2;
		facesNormals[2] = 3;
		facesNormals[3] = 2;
		facesNormals[4] = 4;
		facesNormals[5] = 3;

		std::vector<glm::vec2> UVTextures(6);
		UVTextures[0] = glm::vec2(0, 1);
		UVTextures[1] = glm::vec2(0, 0);
		UVTextures[2] = glm::vec2(1, 1);
		UVTextures[3] = glm::vec2(0, 0);
		UVTextures[4] = glm::vec2(1, 0);
		UVTextures[5] = glm::vec2(1, 1);

		std::vector<GLuint> texturesIndex(6);
		texturesIndex[0] = 1;
		texturesIndex[1] = 2;
		texturesIndex[2] = 3;
		texturesIndex[3] = 2;
		texturesIndex[4] = 4;
		texturesIndex[5] = 3;

		std::vector<GLuint> number(6);
		number[0] = num_texture;
		number[1] = num_texture;
		number[2] = num_texture;
		number[3] = num_texture;
		number[4] = num_texture;
		number[5] = num_texture;

		return new Mesh(points, UVTextures, normals, faces, facesNormals, texturesIndex, number);
	}

	void indexData()
	{
		vertices_indexed = std::vector<glm::vec4>();
		textures_indexed = std::vector<glm::vec2>();
		normals_indexed = std::vector<glm::vec3>();
		texturesNumber_indexed = std::vector<GLuint>();

		if (vertices.size() > 0)
		{
			if (verticesIndex.size() > 0)
			{
				for (unsigned int i = 0; i < verticesIndex.size(); i++)
				{
					unsigned int vertexIndex = verticesIndex[i];
					glm::vec4 vertex = vertices[vertexIndex - 1];
					vertices_indexed.push_back(vertex);
					GLuint num = texturesNumber[vertexIndex - 1];
					texturesNumber_indexed.push_back(num);
				}
			}
			else
			{
				for (unsigned int i = 0; i < vertices.size(); i++)
				{
					glm::vec4 vertex = vertices[i];
					vertices_indexed.push_back(vertex);
					GLint num = texturesNumber[i];
					texturesNumber_indexed.push_back(num);
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
					textures_indexed.push_back(uv);
				}
			}
			else
			{
				for (unsigned int i = 0; i < textures.size(); i++)
				{
					glm::vec2 uv = textures[i];
					textures_indexed.push_back(uv);
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
					normals_indexed.push_back(normal);
				}
			}
			else
			{
				for (unsigned int i = 0; i < normals.size(); i++)
				{
					glm::vec3 normal = normals[i];
					normals_indexed.push_back(normal);
				}
			}
		}
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

	float length = static_cast<float>(max.length());

	for (unsigned int i = 0; i < points.size(); i++)
	{
		points[i].x = pos.x + (((points[i].x - minX) - diffX * 0.5f) / (length * 0.5f)) * scale.x;
		points[i].y = pos.y + (((points[i].y - minY) - diffY * 0.5f) / (length * 0.5f)) * scale.y;
		points[i].z = pos.z + (((points[i].z - minZ) - diffZ * 0.5f) / (length * 0.5f)) * scale.z;
	}
}

bool load_obj(const char* filename, std::vector<glm::vec4> &vertices, std::vector<glm::vec2> &textures, std::vector<glm::vec3> &normals, std::vector<GLuint> &verticesIndex,
	std::vector<GLuint> &texturesIndex, std::vector<GLuint> &normalsIndex, std::vector<GLuint> &texturesNumber, GLuint num_texture,
	const glm::vec3 pos = glm::vec3(0.f), const glm::vec3 scale = glm::vec3(1.f), const bool computeNormals = false)
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
	normalizePointList(vertices, pos, scale);

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
	window = glfwCreateWindow(WIDTH, HEIGHT, "Gamagora Portal", NULL, NULL);
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

	glDebugMessageCallback(debug, nullptr);

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
	glm::vec3 camPos;
	glm::vec3 lightPos;
	GLfloat near, far, fov;
	GLuint buffer, bufferIndex;
	GLuint normalsBuffer, normalsBufferIndex;
	GLuint colorBuffer, colorBufferIndex;
	GLuint textureNumberBuffer;
	GLuint texturesBuffer[3];
	GLuint texturesSamplerBuffer[3];

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
	GLuint sun_radius;
} gs;

Mesh* createMesh(const char* filename, GLushort num_texture, const glm::vec3 pos = glm::vec3(0.f), const glm::vec3 scale = glm::vec3(1.f), const bool computeNormals = false)
{
	Mesh *m = new Mesh();
	if (load_obj(filename, m->vertices, m->textures, m->normals, m->verticesIndex, m->texturesIndex, m->normalsIndex, m->texturesNumber, num_texture, pos, scale, computeNormals))
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
	gs.program_fbo = buildProgram("basic.vsl", "texture.fsl");
	gs.program_sun = buildProgram("basic.vsl", "sun.fsl");

	// Global parameters
	gs.start = clock();
	gs.p = glm::vec4(0.f, 2.f, 0.f, 0.f);
	gs.camPos = glm::vec3(0.f, 3.f, 5.f); 
	gs.lightPos = glm::vec3(2.f, 5.f, -3.f); // 2.f, 5.f, -3.f
	gs.near = 0.1f;
	gs.far = 100.f;
	gs.fov = 90.f;
	gs.sun_radius = 1.f;

	float cube_size = 10.f;
	float size = cube_size * 0.85f;

	/*gs.mesh = Mesh::Quadrangle(glm::vec3(-5.F, -0.5f, -5.F),
		glm::vec3(-5.F, -0.5f, 5.F),
		glm::vec3(5.F, -0.5f, -5.F),
		glm::vec3(5.F, -0.5f, 5.F), 
		-1);

	Mesh *cube = createMesh("RubiksCube.obj", 1, glm::vec3(0.f, 0.10f + cube_size * 0.11f, 0.f), glm::vec3(1.f));

	gs.mesh->merge(cube);*/

	gs.mesh = createMesh("Stormtrooper.obj", 0, glm::vec3(2.f, cube_size * 0.11f, 0.f), glm::vec3(1.f));

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

		Mesh *dragon = createMesh("Alduin.obj", 2, glm::vec3(-4.f, cube_size * 0.2f, 0.f), glm::vec3(1.f));
		if (dragon != nullptr)
		{
			gs.mesh->merge(dragon);
		}
	}

	if (gs.mesh != nullptr)
	{
		gs.mesh->indexData();
		gs.size = static_cast<GLint>(gs.mesh->vertices_indexed.size());

		Mesh *sun = createMesh("Sphere.obj", -1, glm::vec3(0.f), glm::vec3(gs.sun_radius));
		sun->indexData();
		gs.sun_size = sun->vertices_indexed.size();

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

		/*glGenBuffers(1, &gs.bufferIndex);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gs.bufferIndex);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, gs.mesh->verticesIndex.size() * sizeof(GLuint), &gs.mesh->verticesIndex[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);*/
		
		// Normals buffer
		glGenBuffers(1, &gs.normalsBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, gs.normalsBuffer);
		glBufferData(GL_ARRAY_BUFFER, gs.mesh->normals_indexed.size() * sizeof(glm::vec3), &gs.mesh->normals_indexed[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		/*glGenBuffers(1, &gs.normalsBufferIndex);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gs.normalsBufferIndex);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, gs.mesh->normalsIndex.size() * sizeof(GLint), &gs.mesh->normalsIndex[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);*/
		
		// Textures buffer
		glGenBuffers(1, &gs.colorBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, gs.colorBuffer);
		glBufferData(GL_ARRAY_BUFFER, gs.mesh->textures_indexed.size() * sizeof(glm::vec2), &gs.mesh->textures_indexed[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		/*glGenBuffers(1, &gs.colorBufferIndex);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gs.colorBufferIndex);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, gs.mesh->texturesIndex.size() * sizeof(GLint), &gs.mesh->verticesIndex[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);*/

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

		// Depth texture
		glGenTextures(1, &gs.renderedTexture);
		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, gs.renderedTexture);

		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, SHADOW_WIDTH, SHADOW_HEIGHT);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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

		// Textures
		glGenTextures(3, gs.texturesBuffer);
		glGenSamplers(3, gs.texturesSamplerBuffer);

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

		// Sun
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
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

	float c = (float)(clock() - gs.start) / CLOCKS_PER_SEC;

	glm::mat4x4 projection = glm::perspective(glm::radians(gs.fov), (float)WIDTH / HEIGHT, gs.near, gs.far);
	glm::mat4x4 model = glm::mat4x4(1.f);

	// Compute the MVP matrix from the light's point of view
	glm::mat4 depthView = glm::lookAt(glm::vec3(gs.lightPos), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
	glm::mat4 depthProj = glm::ortho(-10.f, 10.f, -10.f, 10.f, 0.1f, 100.f);
	glm::mat4 depthMVP = depthProj * depthView;

	GLuint texLoc;
	
	glBindFramebuffer(GL_FRAMEBUFFER, gs.framebuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(gs.program_fbo);

	glProgramUniformMatrix4fv(gs.program_fbo, 1, 1, GL_FALSE, &depthMVP[0][0]);
	glProgramUniformMatrix4fv(gs.program_fbo, 3, 1, GL_FALSE, &model[0][0]);

	glBindVertexArray(gs.vao);
	{
		glDrawArrays(GL_TRIANGLES, 0, gs.size);
	}

	glBindVertexArray(0);
	glUseProgram(0);

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, gs.renderedTexture);
	glGenerateMipmap(GL_TEXTURE_2D);

	glm::vec3 pt(gs.p.x, gs.p.y, gs.p.z);
	glm::mat4x4 view = glm::lookAt(gs.camPos, pt, glm::vec3(0.f, 1.f, 0.f));
	glm::mat4x4 mvp = projection * view;

	glm::mat4 biasMatrix(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f
		);
	glm::mat4 depthBiasMVP = biasMatrix * depthMVP;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(gs.program);

	glProgramUniformMatrix4fv(gs.program, 1, 1, GL_FALSE, &mvp[0][0]);
	glProgramUniformMatrix4fv(gs.program, 2, 1, GL_FALSE, &depthBiasMVP[0][0]);
	glProgramUniformMatrix4fv(gs.program, 3, 1, GL_FALSE, &model[0][0]);
	glProgramUniform3fv(gs.program, 4, 1, &gs.camPos[0]);
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

		glDrawArrays(GL_TRIANGLES, 0, gs.size);
		//glDrawElements(GL_TRIANGLES, gs.mesh->verticesIndex.size(), GL_UNSIGNED_INT, NULL);	
	}

	//gs.camPos.x = 5.f * cos(c);
	//gs.camPos.z = 5.f * sin(c);

	gs.lightPos = glm::vec3(4.f * sin(c), 5.f, 4.f * cos(c));

	glBindVertexArray(0);
	glUseProgram(0);

	glUseProgram(gs.program_sun);
	model = glm::translate(model, glm::vec3(gs.lightPos.x, gs.lightPos.y, gs.lightPos.z));
	
	glProgramUniformMatrix4fv(gs.program_sun, 1, 1, GL_FALSE, &mvp[0][0]);
	glProgramUniformMatrix4fv(gs.program_sun, 3, 1, GL_FALSE, &model[0][0]);
	glProgramUniform3fv(gs.program_sun, 4, 1, &gs.camPos[0]);

	glBindVertexArray(gs.vao_sun);
	{
		glDrawArrays(GL_TRIANGLES, 0, gs.sun_size);
	}

	glBindVertexArray(0);
	glUseProgram(0);
}