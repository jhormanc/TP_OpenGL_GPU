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

#define WIDTH 640
#define HEIGHT 480

using namespace std;

void render(GLFWwindow*);
void init();

#define glInfo(a) std::cout << #a << ": " << glGetString(a) << std::endl

std::regex obj_regex1("f [0-9]+// [0-9]+// [0-9]+//(.*)");
std::regex obj_regex2("f [0-9]+//[0-9]+ [0-9]+//[0-9]+ [0-9]+//[0-9]+(.*)");
std::regex obj_regex3("f [0-9]+/[0-9]+/[0-9]+ [0-9]+/[0-9]+/[0-9]+ [0-9]+/[0-9]+/[0-9]+[\\r\\n]*");

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
	vector<glm::vec4> vertices;
	vector<glm::vec2> textures;
	vector<glm::vec3> normals;
	vector<GLushort> verticesIndex;
	vector<GLushort> texturesIndex;
	vector<GLushort> normalsIndex;

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
	}
};

void indexData(vector<glm::vec4> &vertices, vector<glm::vec2> &textures, vector<glm::vec3> &normals, vector<GLushort> &verticesIndex, vector<GLushort> &texturesIndex, vector<GLushort> &normalsIndex)
{
	std::vector<glm::vec4> new_vertices;
	std::vector<glm::vec2> new_uvs;
	std::vector<glm::vec3> new_normals;

	if (vertices.size() > 0)
	{
		if (verticesIndex.size() > 0)
		{
			for (unsigned int i = 0; i < verticesIndex.size(); i++)
			{
				unsigned int vertexIndex = verticesIndex[i];
				glm::vec4 vertex = vertices[vertexIndex - 1];
				new_vertices.push_back(vertex);
			}
		}
		else
		{
			for (unsigned int i = 0; i < vertices.size(); i++)
			{
				glm::vec4 vertex = vertices[i];
				new_vertices.push_back(vertex);
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
			}
		}
		else
		{
			for (unsigned int i = 0; i < textures.size(); i++)
			{
				glm::vec2 uv = textures[i];
				new_uvs.push_back(uv);
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
}

bool load_obj(const char* filename, vector<glm::vec4> &vertices, vector<glm::vec2> &textures, vector<glm::vec3> &normals, vector<GLushort> &verticesIndex, vector<GLushort> &texturesIndex, vector<GLushort> &normalsIndex, 
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
			vertices.push_back(glm::vec4(pos.x + vertex.x * scale.x,
				pos.y + vertex.y * scale.y, 
				pos.z + vertex.z * scale.z, 
				1.f));
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
		}
	}

	// Reindexing data
	if (indexing_data)
		indexData(vertices, textures, normals, verticesIndex, texturesIndex, normalsIndex);

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
	indexData(m->vertices, m->textures, m->normals, m->verticesIndex, m->texturesIndex, m->normalsIndex);
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
	GLfloat near, far, fov;
	GLuint buffer;
	GLuint normalsBuffer;
	GLuint colorbuffer;

	GLfloat size;
} gs;

Mesh* createMesh(const char* filename, const bool index_data, const glm::vec3 pos = glm::vec3(0.f), const glm::vec3 scale = glm::vec3(1.f))
{
	Mesh *m = new Mesh();
	if (load_obj(filename, m->vertices, m->textures, m->normals, m->verticesIndex, m->texturesIndex, m->normalsIndex, index_data, pos, scale))
		return m;
	return nullptr;
}

void init()
{
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClearDepth(1.0f); // Set background depth to farthest
	glEnable(GL_DEPTH_TEST); // Enable depth testing for z-culling
	glDepthFunc(GL_LEQUAL); // Set the type of depth-test
	glShadeModel(GL_SMOOTH); // Enable smooth shading
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // Nice perspective corrections

	// Build our program and an empty VAO
	gs.program = buildProgram("basic.vsl", "basic.fsl");

	// Global parameters
	gs.start = clock();
	gs.p = glm::vec4(0.f, 3.f, 0.f, 0.f);
	gs.camPos = glm::vec3(0.f, 5.f, -10.f);
	gs.near = 0.1f;
	gs.far = 100.f;
	gs.fov = 45.f;

	Mesh *mesh = createMesh("Stormtrooper.obj", false);

	if (mesh != nullptr)
	{
		Mesh *wall_1 = createMesh("Cube.obj", false, glm::vec3(0.f, -10.f, 0.f), glm::vec3(10.f));
		mesh->merge(wall_1);
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

		glCreateVertexArrays(1, &gs.vao);
		glBindVertexArray(gs.vao);

		// Vertex shader input vec4 pt
		glBindBuffer(GL_ARRAY_BUFFER, gs.buffer);
		glEnableVertexArrayAttrib(gs.vao, 11);
		glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, 0, (void *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//  Vertex shader input vec3 normal
		glBindBuffer(GL_ARRAY_BUFFER, gs.normalsBuffer);
		glEnableVertexArrayAttrib(gs.vao, 12);
		glVertexAttribPointer(12, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//  Vertex shader input vec2 uv
		glBindBuffer(GL_ARRAY_BUFFER, gs.colorbuffer);
		glEnableVertexArrayAttrib(gs.vao, 13);
		glVertexAttribPointer(13, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
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
	//glProgramUniform1f(gs.program, 3, std::abs(cos(c)));

	glm::vec3 pt(gs.p.x, gs.p.y, gs.p.z);
	glm::mat4x4 m;
	glm::mat4x4 projection = glm::perspective(glm::radians(gs.fov), (float)(WIDTH / (float)HEIGHT), gs.near, gs.far);
	glm::mat4x4 view = glm::lookAt(gs.camPos, pt, glm::vec3(0.f, 1.f, 0.f));
	glm::mat4x4 model = glm::mat4(1.0f); //glm::rotate(glm::mat4(1.f), c, glm::vec3(0.f, 1.f, 0.f));
	//glm::mat4x4 m_view = glm::translate(glm::mat4(1.f), glm::vec3(0.5, 0.5, 0.f));
	m = projection * view * model;
	glProgramUniformMatrix4fv(gs.program, 1, 1, GL_FALSE, &m[0][0]);

	gs.camPos.x = 5.f * cos(c);
	gs.camPos.z = 5.f * sin(c);
	
	/*if (gs.p.x > 1.f)
	{
		gs.p.x = 0.f;
		gs.p.y = 0.f;
	}

	if (c > 1.f)
		gs.start = clock();*/

	glUseProgram(gs.program);
	glBindVertexArray(gs.vao);
	{
		//glProgramUniform4f(gs.program, 1, gs.p.x, gs.p.y, gs.p.z, gs.p.w);
		glDrawArrays(GL_TRIANGLES, 0, gs.size);
		//glDrawElements()

		//glProgramUniform4f(gs.program, 1, -gs.p.x, -gs.p.y, gs.p.z, gs.p.w);
		//glDrawArrays(GL_TRIANGLES, 0, gs.size);
	}

	glBindVertexArray(0);
	glUseProgram(0);
}