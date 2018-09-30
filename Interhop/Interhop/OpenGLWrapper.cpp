#include "stdafx.h"

#include "OpenCLUtil.h"
#include "OpenGLUtil.h"

#define GLEW_STATIC
#include <iostream>
#include <CL/cl.hpp>
#include <GL/glew.h>
#include <GL/GL.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "shader.hpp"

#include "DEFINES.H"
#include "OpenGLWrapper.h"


void GLWInit(GLWParams* params) {
	std::cout << "Error: " << glGetError() << std::endl;
	glGenVertexArrays(1, &params->vao);
	glBindVertexArray(params->vao);

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	// Give our vertices to OpenGL.
	std::cout << "sizeof: " << GLW_VERTEX_COUNT * sizeof(float) << std::endl;
	glBufferData(GL_ARRAY_BUFFER, GLW_VERTEX_COUNT * sizeof(float), GLWvertices, GL_STATIC_DRAW);

	// Enable vsync. I think.
	glfwSwapInterval(1);

	//bind the UV buffer
	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, GLW_UV_COUNT * sizeof(float), GLWuvcoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glClearColor(1, 1, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	std::cout << "Error: " << glGetError() << std::endl;

	params->program = LoadShaders("VertexShader.vs", "FragmentShader.fs");
	glUseProgram(params->program);
		
	params->uniforms.time = glGetUniformLocation(params->program, "u_time");
} 

void GLWInitTexture(GLWParams* params, float* textureBuffer) {
	//setup the texture
	params->texA = createTexture2D(TEX_WIDTH, TEX_HEIGTH, textureBuffer);
	params->texB = createTexture2D(TEX_WIDTH, TEX_HEIGTH, textureBuffer);
}