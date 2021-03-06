// ParticleSystem.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL

#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/GL.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "CONSTANTS.h"

#include "shader.hpp"
#include "OpenCLUtil.h"
#include "OpenGLUtil.h"

GLFWwindow* window;
float* textureBuffer;
GL_PARAMS gl_params;
CL_PARAMS cl_params;

void error_callback(int error, const char* msg) { fputs(msg, stderr); }
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {}
void GLFWCreateWindow();
void Display();
void InitOpenGL();
void InitOpenCL();
void ParticleSystem(cl::ImageGL &texture);
void GenerateParticles();
void ClearImage(cl::ImageGL &texture);
void OpenCLTakeOver(std::vector<cl::Memory> objs);

int main()
{
	GLFWCreateWindow();
	std::cout << "err: " << glGetError() << std::endl;
	InitOpenGL();
	InitOpenCL();

	OpenCLTakeOver({ cl_params.texture });
	GenerateParticles();
	cl_params.queue.finish();

	while (!glfwWindowShouldClose(window)) {
		OpenCLTakeOver({ cl_params.texture });
		ClearImage(cl_params.texture);
		ParticleSystem(cl_params.texture);
		ParticleSystem(cl_params.texture);
		ParticleSystem(cl_params.texture);
		ParticleSystem(cl_params.texture);
		ParticleSystem(cl_params.texture);
		ParticleSystem(cl_params.texture);
		cl_params.queue.finish();
		Display();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwTerminate();
    return 0;
}


void GLFWCreateWindow() {
	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		std::cout << "Failed to start GLFW" << std::endl;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGTH, "Particle System", nullptr, nullptr);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window, do you have OpenCL 3.3?");
		glfwTerminate();
		throw;
	}
	glfwMakeContextCurrent(window);
	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW");
	}
	std::cout << "post glew: " << glGetError() << std::endl;

	glfwSwapInterval(1); //Enable vsync
	glfwSetKeyCallback(window, key_callback);
	int width, heigth;
	glfwGetFramebufferSize(window, &width, &heigth);
}

void Display()
{
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

	glBindTexture(GL_TEXTURE_2D, gl_params.texture);

	glDrawArrays(GL_TRIANGLES, 0, VERTEX_COUNT); // Starting from vertex 0; 3 vertices total -> 1 triangle
	glDisableVertexAttribArray(0);
}

void InitOpenGL() {
	glGenVertexArrays(1, &gl_params.vao);
	glBindVertexArray(gl_params.vao);

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	// Give our vertices to OpenGL.
	std::cout << "sizeof: " << VERTEX_COUNT * sizeof(float) << std::endl;
	glBufferData(GL_ARRAY_BUFFER, VERTEX_COUNT * sizeof(float), vertices, GL_STATIC_DRAW);

	// Enable vsync. I think.
	glfwSwapInterval(1);

	//bind the UV buffer
	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, UV_COUNT * sizeof(float), uvcoords, GL_STATIC_DRAW);
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

	gl_params.program = LoadShaders("shaders/VertexShader.vs", "shaders/FragmentShader.fs");
	glUseProgram(gl_params.program);

	//Generate texture
	textureBuffer = new float[SCR_WIDTH * SCR_HEIGTH * 4];
	gl_params.texture = createTexture2D(SCR_WIDTH, SCR_HEIGTH, textureBuffer);
}

void InitOpenCL() {
	cl::Platform lPlatform = getPlatform();

	std::vector<cl::Device> devices;
	lPlatform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

	for (unsigned d = 0; d < devices.size(); d++) {
		if (checkExtnAvailability(devices[d], CL_GL_SHARING_EXT)) {
			cl_params.device = devices[d];
			break;
		}
	}
	std::cout << "Using device: " << cl_params.device.getInfo<CL_DEVICE_NAME>() << std::endl;

	cl_context_properties cps[]{
		CL_GL_CONTEXT_KHR, (cl_context_properties)glfwGetWGLContext(window),
		CL_WGL_HDC_KHR, (cl_context_properties)GetDC(glfwGetWin32Window(window)),
		CL_CONTEXT_PLATFORM, (cl_context_properties)lPlatform(),
		0
	};

	cl::Context context(cl_params.device, cps);
	cl_params.context = context;

	cl::Program::Sources sources;

	std::vector<std::string> kernel_sources = {
		"kernels/ParticleSystem.kernel"
	};

	for (int i = 0; i < kernel_sources.size(); i++) {
		std::string temp = ReadKernelSource((char*)kernel_sources[i].c_str());
		char* temp_c = (char*)malloc(temp.length());
		memcpy(temp_c, temp.c_str(), temp.length());
		sources.push_back({ temp_c, temp.length() });
	}

	cl::Program program(cl_params.context, sources);
	if (program.build({ cl_params.device }) != CL_SUCCESS) {
		std::cout << "Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(cl_params.device) << "\n";
		exit(1);
	}

	cl_params.program = program;

	cl::CommandQueue queue(cl_params.context, cl_params.device);
	cl_params.queue = queue;


	cl_params.ParticleSystem = cl::Kernel(program, "ParticleSystem");
	cl_params.GenerateParticles = cl::Kernel(program, "GenerateParticles");
	cl_params.ClearImage = cl::Kernel(program, "ClearImage");
	cl_params.p_positions = cl::Buffer(cl_params.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, sizeof(float) * PARTICLE_COUNT * 3, NULL, NULL);
	cl_params.p_velocity = cl::Buffer(cl_params.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, sizeof(float) * PARTICLE_COUNT * 3, NULL, NULL);

	//Interhop stage
	cl_int errCode;
	cl_params.texture = cl::ImageGL(cl_params.context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, gl_params.texture, &errCode);
	if (errCode == CL_SUCCESS) {
		std::cout << "OpenCL succesfully obtained access from OpenGL" << std::endl;
	}
	else {
		std::cout << "OpenCL was denied by OpenGL, F :(" << errCode << ")" << std::endl;
	}
} 

void ParticleSystem(cl::ImageGL &texture) {
	cl_params.ParticleSystem.setArg(0, texture);
	cl_params.ParticleSystem.setArg(1, cl_params.p_positions);
	cl_params.ParticleSystem.setArg(2, cl_params.p_velocity);
	cl_params.ParticleSystem.setArg(3, SCR_WIDTH);
	cl_params.ParticleSystem.setArg(4, SCR_HEIGTH);
	cl_params.queue.enqueueNDRangeKernel(cl_params.ParticleSystem, cl::NullRange, cl::NDRange(PARTICLE_ROOT), cl::NullRange);
}

void GenerateParticles() {
	cl_params.GenerateParticles.setArg(0, cl_params.p_positions);
	cl_params.GenerateParticles.setArg(1, cl_params.p_velocity);
	cl_params.GenerateParticles.setArg(2, SCR_WIDTH);
	cl_params.GenerateParticles.setArg(3, SCR_HEIGTH);
	cl_params.queue.enqueueNDRangeKernel(cl_params.GenerateParticles, cl::NullRange, cl::NDRange(PARTICLE_ROOT), cl::NullRange);
}

void ClearImage(cl::ImageGL &texture) {
	cl_params.ClearImage.setArg(0, texture);
	cl_params.queue.enqueueNDRangeKernel(cl_params.ClearImage, cl::NullRange, cl::NDRange(SCR_WIDTH, SCR_HEIGTH), cl::NullRange);
}

void OpenCLTakeOver(std::vector<cl::Memory> objs) {
	glFinish();
	cl::Event ev;
	//flush openGL commands and wait for package
	cl_int res = cl_params.queue.enqueueAcquireGLObjects(&objs, NULL, &ev);
	ev.wait();
	if (res != CL_SUCCESS) {
		std::cout << "Failed to aqcuire OpenGL object" << std::endl;
	}
}