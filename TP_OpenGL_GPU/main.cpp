#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>

#include <GL/glew.h>

#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include <glm\glm\glm.hpp>

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
	window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
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

	float data[16] = { -0.5f, -0.5f, 0.f, 1.f, 0.5f, -0.5f, 0.f, 1.f, 0.5f, 0.5f, 0.f, 1.f, -0.5f, 0.5f, 0.f, 1.f };

	glBindVertexArray(gs.vao);

	glBindBuffer(GL_ARRAY_BUFFER, gs.buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, data, GL_STATIC_DRAW);

	glVertexAttribPointer(11, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);
	glVertexAttribPointer(12, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)12);

	glEnableVertexArrayAttrib(gs.vao, 11);
	glEnableVertexArrayAttrib(gs.vao, 12);

	glBindVertexArray(0);
	
}

void render(GLFWwindow* window)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	float c = (float)(clock() - gs.start) / CLOCKS_PER_SEC;
	glProgramUniform1f(gs.program, 3, c);

	gs.p.x += 0.01f;
	gs.p.y += 0.01f;

	if (gs.p.x > 1.f)
	{
		gs.p.x = 0.f;
		gs.p.y = 0.f;
	}

	if (c > 1.f)
		gs.start = clock();

	glUseProgram(gs.program);
	glBindVertexArray(gs.vao);

	{
		glProgramUniform4f(gs.program, 1, gs.p.x, gs.p.y, gs.p.z, gs.p.w);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glProgramUniform4f(gs.program, 1, -gs.p.x, -gs.p.y, gs.p.z, gs.p.w);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	glBindVertexArray(0);
	glUseProgram(0);
}