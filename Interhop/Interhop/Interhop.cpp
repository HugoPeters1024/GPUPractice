// OpenCLProject.cpp : Defines the entry point for the console application.
//

#pragma once

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <windows.h>
#include <CL/cl.hpp>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/GL.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "shader.hpp"
#define __CL_ENABLE_EXCEPTIONS

#include "DEFINES.H"
#include "OpenCLWrapper.h"
#include "OpenGLWrapper.h"



GLFWwindow* window;
unsigned char* textureBuffer;
float* OpenCLTextureBuffer;
// Create and compile our GLSL program from the shaders
GLuint programID;

//Shader variable
GLfloat time = 0;

CLWParams cl_params;
CLWRayTraceParams rt_params;
GLWParams gl_params;


void InitLibraries();
void Display();
void GenerateTexture(float* output);
void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_W && action == GLFW_REPEAT)
		rt_params.cam_pos.z += 0.01f;
	if (key == GLFW_KEY_S && action == GLFW_REPEAT)
		rt_params.cam_pos.z -= 0.01f;
}

int main() {
	InitLibraries();
	GLWInit(&gl_params);

	OpenCLTextureBuffer = new float[TEX_WIDTH * TEX_HEIGTH * 4];
//	GenerateTexture(OpenCLTextureBuffer);

	GLWInitTexture(&gl_params, OpenCLTextureBuffer);
	CLWInit(&cl_params, window);
	CLWInitInterhopStage(cl_params, gl_params);

	CLWPerlinNoiseKernel(cl_params);
	cl_params.queue.finish();

	for (int i = 0; i < 3; i++) {
		cl_params.kernels.blur.setArg(0, cl_params.images.A);
		cl_params.kernels.blur.setArg(1, cl_params.images.B);
		cl_params.queue.enqueueNDRangeKernel(cl_params.kernels.blur, cl::NullRange, cl::NDRange(TEX_WIDTH, TEX_HEIGTH), cl::NullRange);
		cl_params.queue.finish();
		std::swap(cl_params.images.A, cl_params.images.B);
	}

	GLfloat3 cam_start = { 0,0,0 };

	//CLWRayTraceInit(cl_params, rt_params, cam_start, sphere_data, SPHERE_COUNT, light_data, LIGHT_COUNT);


	while (!glfwWindowShouldClose(window)) {
		//rt_params.cam_pos.z += 0.0001f;
		Display();
		//CLWRayTrace(cl_params, rt_params, time);
		//CLWPathTrace(cl_params);
		CLWFireKernel(cl_params, time);
		glfwSwapBuffers(window);
		glfwPollEvents();
		time += 0.01f;
	}
	glfwTerminate();
	//get all platforms (drivers)
	int l;
	std::cin >> l;

	return 0;
}

void Display()
{
	glUniform1f(gl_params.uniforms.time, time);


	//glUniformMatrix4fv(uniformTransform, 1, GL_FALSE, glRotatef(time, 1, 0, 0));

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, gl_params.vao);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);
	glDrawArrays(GL_TRIANGLES, 0, GLW_VERTEX_COUNT); // Starting from vertex 0; 3 vertices total -> 1 triangle
	glDisableVertexAttribArray(0);
}

void InitLibraries()
{
	glfwSetErrorCallback(error_callback);

	glewExperimental = true; // Needed for core profile
	if (!glfwInit()) {
		std::cout << "Failed to start GLFW" << std::endl;
		throw;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(IMG_WIDTH, IMG_HEIGTH, "joepie", nullptr, nullptr);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		throw;
	}
	glfwMakeContextCurrent(window);
	glewExperimental = true; // Needed in core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		throw;
	}

	glfwSetKeyCallback(window, key_callback);

	// Set viewport size to window size * scaling, to fix HiDPI. The framebuffer
	// size should be the right size, adjusted for scaling, but unfortunately it
	// is not on my system, the size returned by glfwGetFramebufferSize and
	// glfwGetWindowSize are the same, even on a HiDPI display.

	// HACK: Note that GetFramebufferSize should return the device pixels, not
	// screen coordinates, so this *should* work for HiDPI, but it does not. See
	// also https://github.com/glfw/glfw/issues/1168. So we just scale if provided
	// as a command line flag.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
}

void GenerateTexture(float *output)
{
	cl_params.kernels.generate_image.setArg(0, cl_params.images.A);
	cl_params.kernels.generate_image.setArg(1, cl_params.images.B);
	cl_params.queue.enqueueNDRangeKernel(cl_params.kernels.generate_image, cl::NullRange, cl::NDRange(TEX_WIDTH, TEX_HEIGTH), cl::NullRange);

	for (int z = 0; z < 20; z++) 
	{
		cl_params.kernels.blur.setArg(0, cl_params.images.A);
		cl_params.kernels.blur.setArg(1, cl_params.images.B);
		cl_params.queue.enqueueNDRangeKernel(cl_params.kernels.blur, cl::NullRange, cl::NDRange(TEX_WIDTH, TEX_HEIGTH), cl::NullRange);
		std::swap(cl_params.images.A, cl_params.images.B);
	}


	//Retrieve the image back to host
	cl::size_t<3> origin;
	origin[0] = 0;
	origin[1] = 0;
	origin[2] = 0;
	cl::size_t<3> region;
	region[0] = TEX_WIDTH;
	region[1] = TEX_HEIGTH;
	region[2] = 1;
	cl_params.queue.enqueueReadImage(cl_params.images.B, CL_TRUE, origin, region, 0, 0, output);
}