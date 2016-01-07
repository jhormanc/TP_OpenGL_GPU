#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <vector>
#include <string.h>
#include <process.h>
#include <stdio.h>

#include <GL/glew.h>

#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include <glm\glm\glm.hpp>
#include <glm\glm\gtx\transform.hpp>

#define WIDTH 640
#define HEIGHT 480

using namespace std;

void render(GLFWwindow*);
void init();

#define glInfo(a) std::cout << #a << ": " << glGetString(a) << std::endl

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

bool load_obj(const char* filename, vector<glm::vec3> &vertices, vector<glm::vec2> &textures, vector<glm::vec3> &normals, vector<GLushort> &verticesIndex, vector<GLushort> &texturesIndex, vector<GLushort> &normalsIndex)
{
	FILE *file;
	fopen_s(&file, filename, "r");
	if (file == NULL)
	{
		printf("Impossible to open the file !\n");
		return false;
	}

	while (1)
	{
		char lineHeader[128];
		
		// read the first word of the line
		int res = fscanf_s(file, "%s", lineHeader, sizeof(lineHeader));
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.
		else
		{
			if (strcmp(lineHeader, "v") == 0)
			{
				glm::vec3 vertex;
				fscanf_s(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
				vertices.push_back(vertex);
			}
			else if (strcmp(lineHeader, "vt") == 0)
			{
				glm::vec2 uv;
				fscanf_s(file, "%f %f\n", &uv.x, &uv.y);
				textures.push_back(uv);
			}
			else if (strcmp(lineHeader, "vn") == 0)
			{
				glm::vec3 normal;
				fscanf_s(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
				normals.push_back(normal);
			}
			else if (strcmp(lineHeader, "f") == 0)
			{
				std::string vertex1, vertex2, vertex3;
				GLushort vertexIndex[3], uvIndex[3], normalIndex[3];
				vertexIndex[0] = 0;
				vertexIndex[1] = 0;
				vertexIndex[2] = 0;
				int matches = fscanf_s(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
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
				else
				{
					matches = fscanf_s(file, "%d//%d %d//%d %d//%d\n", &vertexIndex[0], &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2]);
					if (matches == 6)
					{
						verticesIndex.push_back(vertexIndex[0]);
						verticesIndex.push_back(vertexIndex[1]);
						verticesIndex.push_back(vertexIndex[2]);
						normalsIndex.push_back(normalIndex[0]);
						normalsIndex.push_back(normalIndex[1]);
						normalsIndex.push_back(normalIndex[2]);
					}
					else
					{
						matches = fscanf_s(file, "%d// %d// %d//\n", &vertexIndex[0], &vertexIndex[1], &vertexIndex[2]);
						if (matches == 3)
						{
							verticesIndex.push_back(vertexIndex[0]);
							verticesIndex.push_back(vertexIndex[1]);
							verticesIndex.push_back(vertexIndex[2]);
						}
					}
				}
			}
		}
	}

	if (normals.size() == 0 && verticesIndex.size() > 0)
	{
		normals.resize(vertices.size(), glm::vec3(0.0, 0.0, 0.0));
		for (int i = 0; i < verticesIndex.size(); i += 3)
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
	window = glfwCreateWindow(WIDTH, HEIGHT, "Hello World", NULL, NULL);
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
	float near, far, fov;
	GLuint buffer;
} gs;

void init()
{
	// Build our program and an empty VAO
	gs.program = buildProgram("basic.vsl", "basic.fsl");

	glCreateVertexArrays(1, &gs.vao);
	glGenBuffers(1, &gs.buffer);

	gs.start = clock();
	gs.p = glm::vec4(0.f, 0.f, 0.f, 0.f);
	gs.camPos = glm::vec3(0.f, 0.f, -1.f);
	gs.near = 1.f;
	gs.far = 1000.f;
	gs.fov = 90.f;

	vector<glm::vec3> vertices;
	vector<glm::vec2> textures;
	vector<glm::vec3> normals;
	vector<GLushort> verticesIndex;
	vector<GLushort> texturesIndex;
	vector<GLushort> normalsIndex;
	load_obj("Cube.obj", vertices, textures, normals, verticesIndex, texturesIndex, normalsIndex);

	for (int i = 0; i < vertices.size(); i++)
	{
		std::cout << vertices[i].x << " " << vertices[i].y << " " << vertices[i].z << "\n";
	}
	//float data[16] = { -0.5f, -0.5f, 0.f, 1.f, 0.5f, -0.5f, 0.f, 1.f, 0.5f, 0.5f, 0.f, 1.f, -0.5f, 0.5f, 0.f, 1.f };

	glBindVertexArray(gs.vao);

	glBindBuffer(GL_ARRAY_BUFFER, gs.buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(), &vertices.front(), GL_STATIC_DRAW);

	glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 0);
	//glVertexAttribPointer(12, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)12);

	glEnableVertexArrayAttrib(gs.vao, 11);
	//glEnableVertexArrayAttrib(gs.vao, 12);

	glBindVertexArray(0);
}

void render(GLFWwindow* window)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	float c = (float)(clock() - gs.start) / CLOCKS_PER_SEC;
	glProgramUniform1f(gs.program, 3, std::abs(cos(c)));

	glm::vec3 pt(gs.p.x, gs.p.y, gs.p.z);
	glm::mat4x4 m;
	glm::mat4x4 m_persp = glm::perspective(gs.fov, (float)(WIDTH / (float)HEIGHT), gs.near, gs.far);
	glm::mat4x4 m_look = glm::lookAt(gs.camPos, pt, glm::vec3(0.f, 1.f, 0.f));
	glm::mat4x4 m_view = glm::rotate(glm::mat4(1.f), c, glm::vec3(0.f, 1.f, 0.f));
	//glm::mat4x4 m_view = glm::translate(glm::mat4(1.f), glm::vec3(0.5, 0.5, 0.f));
	m = m_view * m_persp * m_look;
	glProgramUniformMatrix4fv(gs.program, 1, 1, GL_FALSE, &m[0][0]);

	/*gs.camPos.x = cos(c);
	gs.camPos.z = sin(c);*/
	
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
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		//glDrawElements()

		//glProgramUniform4f(gs.program, 1, -gs.p.x, -gs.p.y, gs.p.z, gs.p.w);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	glBindVertexArray(0);
	glUseProgram(0);
}