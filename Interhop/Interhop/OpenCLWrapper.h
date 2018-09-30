#pragma once

#include<string>
#include<CL/cl.hpp>

#include "OpenGLWrapper.h"

typedef struct {
	cl::Kernel generate_image;
	cl::Kernel blur;
	cl::Kernel noise;
	cl::Kernel raytracer;
	cl::Kernel pathtracer;
	cl::Kernel fire;
	cl::Kernel perlin;
} CLWKernelContainer;

typedef struct {
	cl::Image2D A;
	cl::Image2D B;
} CLWImageContainer;


typedef struct {
	cl::Device device;
	cl::CommandQueue queue;
	cl::Program program;
	CLWKernelContainer kernels;
	CLWImageContainer images;
	cl::Context context;
	cl::ImageGL texA;
	cl::ImageGL texB;
	cl::size_t<3> dims;
} CLWParams;

typedef struct GLfloat3 {
	float x, y, z;

	GLfloat3 operator - (const GLfloat3& b) {
		return GLfloat3{ x-b.x, y-b.y, z-b.z };
	}

	GLfloat3 operator + (const GLfloat3& b) {
		return GLfloat3{ x + b.x, y + b.y, z + b.z };
	}

	GLfloat3 operator * (const float& b) {
		return GLfloat3{ x * b, y * b, z * b };
	}

	GLfloat3 operator / (const float& b) {
		return GLfloat3{ x / b, y / b, z / b };
	}

	GLfloat3 operator += (const GLfloat3 &b) {
		return GLfloat3{ x + b.x, y + b.y, z + b.z };
	}

} GLfloat3;

typedef struct {
	cl::Buffer spheres;
	int sphere_count;
	cl::Buffer lights;
	int light_count;
	cl::Buffer screen_buffer;
	GLfloat3 cam_pos;
	GLfloat3 cam_dir;
	GLfloat3 screen_origin;
	GLfloat3 screen_horz;
	GLfloat3 screen_vert;
	GLfloat screen_distance;
} CLWRayTraceParams;


static float* screen_buffer_data;

#define SPHERE_COUNT 2
static const float sphere_data[SPHERE_COUNT*4] = {
	0.0f, 0.0f, 1.0f, 0.5f,
	-0.5f, 0.1f, 1.0f, 0.5f,
};

#define LIGHT_COUNT 3
static const float light_data[LIGHT_COUNT * 3] = {
	-0.2f, 1.0f, 1.0f,
	0.2f, 0.0f, 0.0f,
	0.0f, 1.0f, 1.0f,
};

std::string CLWReadKernelSource(char* filename);
void CLWInit(CLWParams* params, GLFWwindow* window);
void CLWInitInterhopStage(CLWParams &cl_params, GLWParams &gl_params);
void CLWRayTrace(CLWParams &cl_params, CLWRayTraceParams &rt_params, float time);
void CLWRayTraceInit(CLWParams &cl_params, CLWRayTraceParams &rt_params, GLfloat3 cam_start, const float* sphere_data, int sphere_count, const float* light_data, int light_count);
void CLWcross_product(const GLfloat* a, const GLfloat* b, GLfloat* target);
void CLWFireKernel(CLWParams &cl_params, GLfloat time);
void CLWPerlinNoiseKernel(CLWParams &cl_params);
void CLWcalculate_screen(CLWRayTraceParams &rt_params, CLWParams &cl_params);
void CLWPathTrace(CLWParams &cl_params);