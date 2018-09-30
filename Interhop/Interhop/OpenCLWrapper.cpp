#include "stdafx.h"
#include <iostream>
#include <string>
#include <fstream>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL

#include "CL/cl.hpp"
#include <GL/glew.h>
#include <GL/GL.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "shader.hpp"

#include "DEFINES.H"
#include "OpenCLWrapper.h"

#include "OpenCLUtil.h"
#include "OpenGLUtil.h"

std::string CLWReadKernelSource(char* filename) {
	std::cout << "CLWrapper -- Reading Kernel Source: " << filename << std::endl;
	std::ifstream input;
	input.open(filename, std::ios::in);

	std::string ret = "";
	std::string line;
	if (input.is_open()) {
		while (getline(input, line)) {
			std::cout << line << std::endl;
			ret += line + "\n";
		}
	}
	else {
		std::cout << "Unable to open source file! check the filename." << std::endl;
	}

	return ret;
}

void CLWInit(CLWParams* params, GLFWwindow* window) {
	cl::Platform lPlatform = getPlatform();

	std::cout << "Using platform: " << lPlatform.getInfo<CL_PLATFORM_NAME>() << "\n";

	std::vector<cl::Device> devices;
	lPlatform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
	// Get a list of devices on this platform
	for (unsigned d = 0; d<devices.size(); ++d) {
		if (checkExtnAvailability(devices[d], CL_GL_SHARING_EXT)) {
			params->device = devices[d];
			break;
		}
	}
	std::cout << "Using device: " << params->device.getInfo<CL_DEVICE_NAME>() << "\n";


	cl_context_properties cps[] = {
		CL_GL_CONTEXT_KHR, (cl_context_properties)glfwGetWGLContext(window),
		CL_WGL_HDC_KHR, (cl_context_properties)GetDC(glfwGetWin32Window(window)),
		CL_CONTEXT_PLATFORM, (cl_context_properties)lPlatform(),
		0
	};


	cl::Context context(params->device, cps);
	std::cout << "ERR: " << glGetError() << std::endl;
	params->context = context;

	cl::Program::Sources sources;

	std::vector<std::string> filenames = { 
		"generate_image.kernel",
		"blur.kernel",
		"noise.kernel",
		"raytracer.kernel",
		"pathtracer.kernel",
		"fire.kernel",
		"perlin.kernel"};

	for (int i = 0; i < filenames.size(); i++) {
		std::string temp = CLWReadKernelSource((char*)filenames[i].c_str());
		char* temp_c = (char*)malloc(temp.length());
		memcpy(temp_c, temp.c_str(), temp.length());
		sources.push_back({ temp_c, temp.length() });
	}
	

	cl::Program program(params->context, sources);
	if (program.build({ params->device }) != CL_SUCCESS) {
		std::cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(params->device) << "\n";
		exit(1);
	}

	params->program = program;
	// create buffers on the device
	cl::Image2D image_A(params->context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_FLOAT), TEX_WIDTH, TEX_HEIGTH);
	cl::Image2D image_B(params->context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_FLOAT), TEX_WIDTH, TEX_HEIGTH);
	params->images.A = image_A;
	params->images.B = image_B;

	//create queue to which we will push commands for the device.
	cl::CommandQueue queue(params->context, params->device);
	params->queue = queue;


	cl::Kernel generate_image(program, "generate_image");
	cl::Kernel blur(program, "blur");
	cl::Kernel noise(program, "noise");
	cl::Kernel raytracer(program, "raytracer");
	cl::Kernel pathtracer(program, "pathtracer");
	cl::Kernel fire(program, "fire");
	cl::Kernel perlin(program, "perlin");
	params->kernels.generate_image = generate_image;
	params->kernels.blur = blur;
	params->kernels.noise = noise;
	params->kernels.raytracer = raytracer;
	params->kernels.pathtracer = pathtracer;
	params->kernels.fire = fire;
	params->kernels.perlin = perlin;
}

void CLWInitInterhopStage(CLWParams &cl_params, GLWParams &gl_params) {
	std::cout << "gl_params->tex: " << gl_params.texA << std::endl;
	cl_int errCode;
	cl_params.texA = cl::ImageGL(cl_params.context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, gl_params.texA, &errCode);
	cl_params.texB = cl::ImageGL(cl_params.context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, gl_params.texB, &errCode);
	if (errCode == CL_SUCCESS) {
		std::cout << "Mr. CL succesfully obtained the package from Mr. GL" << std::endl;
	}
	else {
		std::cout << "Mr. CL was unable to obtain the texture from Mr. GL (code" << errCode << ")" << std::endl;
	}
}

void CLWRayTrace(CLWParams &cl_params, CLWRayTraceParams &rt_params, float time) {
	glFinish();

	cl::Event ev;
	std::vector<cl::Memory> objs;
	objs.clear();
	objs.push_back(cl_params.texA);
	objs.push_back(cl_params.texB);
	//flush openGL commands and wait for package
	cl_int res = cl_params.queue.enqueueAcquireGLObjects(&objs, NULL, &ev);
	ev.wait();
	if (res != CL_SUCCESS) {
		std::cout << "Failed to aqcuire OpenGL object" << std::endl;
	}

	float screen_size[] = { TEX_WIDTH, TEX_HEIGTH };

	CLWcalculate_screen(rt_params, cl_params);

	cl_params.kernels.raytracer.setArg(0, cl_params.texA);
	cl_params.kernels.raytracer.setArg(1, screen_size);
	cl_params.kernels.raytracer.setArg(2, rt_params.spheres);
	cl_params.kernels.raytracer.setArg(3, rt_params.sphere_count);
	cl_params.kernels.raytracer.setArg(4, rt_params.lights);
	cl_params.kernels.raytracer.setArg(5, rt_params.light_count);
	cl_params.kernels.raytracer.setArg(6, time);
	cl_params.kernels.raytracer.setArg(7, rt_params.screen_buffer);
	cl_params.queue.enqueueNDRangeKernel(cl_params.kernels.raytracer, cl::NullRange, cl::NDRange(TEX_WIDTH, TEX_HEIGTH), cl::NullRange);

	cl_params.queue.finish();
}

void CLWRayTraceInit(CLWParams &cl_params, CLWRayTraceParams &rt_params, GLfloat3 cam_start, const float* sphere_data, int sphere_count, const float* light_data, int light_count) {
	rt_params.cam_pos = cam_start;
	rt_params.cam_dir = { 0, 0, 1 };
	rt_params.screen_distance = 0.6f;

	rt_params.spheres = cl::Buffer(cl_params.context, CL_MEM_READ_ONLY, sizeof(float) * sphere_count * 4);
	rt_params.sphere_count = sphere_count;
	rt_params.lights = cl::Buffer(cl_params.context, CL_MEM_READ_ONLY, sizeof(float) * light_count * 3);
	rt_params.light_count = light_count;
	rt_params.screen_buffer = cl::Buffer(cl_params.context, CL_MEM_READ_WRITE, sizeof(float) * 15);

	CLWcalculate_screen(rt_params, cl_params);

	cl_params.queue.enqueueWriteBuffer(rt_params.spheres, CL_FALSE, 0, sizeof(float) * sphere_count * 4, sphere_data);
	cl_params.queue.enqueueWriteBuffer(rt_params.lights, CL_FALSE, 0, sizeof(float) * light_count * 3, light_data);
	cl_params.queue.finish();
}

void CLWPathTrace(CLWParams &cl_params) {
	cl::Event ev;
	std::vector<cl::Memory> objs;
	objs.clear();
	objs.push_back(cl_params.texA);
	//flush openGL commands and wait for package
	cl_int res = cl_params.queue.enqueueAcquireGLObjects(&objs, NULL, &ev);
	ev.wait();
	if (res != CL_SUCCESS) {
		std::cout << "Failed to aqcuire OpenGL object" << std::endl;
	}

	cl_params.kernels.pathtracer.setArg(0, cl_params.texA);
	cl_params.queue.enqueueNDRangeKernel(cl_params.kernels.pathtracer, cl::NullRange, cl::NDRange(TEX_WIDTH, TEX_HEIGTH), cl::NullRange);
	cl_params.queue.finish();
}

void CLWFireKernel(CLWParams &cl_params, GLfloat time) {
	cl::Event ev;
	std::vector<cl::Memory> objs;
	objs.clear();
	objs.push_back(cl_params.texA);
	objs.push_back(cl_params.texB);
	//flush openGL commands and wait for package
	cl_int res = cl_params.queue.enqueueAcquireGLObjects(&objs, NULL, &ev);
	ev.wait();
	if (res != CL_SUCCESS) {
		std::cout << "Failed to aqcuire OpenGL object" << std::endl;
	}

	cl_params.kernels.fire.setArg(0, cl_params.texA);
	cl_params.kernels.fire.setArg(1, cl_params.texB);
	cl_params.kernels.fire.setArg(2, cl_params.images.A);
	cl_params.kernels.fire.setArg(3, cl_params.images.B);
	cl_params.kernels.fire.setArg(4, TEX_WIDTH);
	cl_params.kernels.fire.setArg(5, TEX_HEIGTH);
	cl_params.kernels.fire.setArg(6, time);
	cl_params.queue.enqueueNDRangeKernel(cl_params.kernels.fire, cl::NullRange, cl::NDRange(TEX_WIDTH, TEX_HEIGTH), cl::NullRange);
	cl_params.queue.finish();
	std::swap(cl_params.texA, cl_params.texB);
	std::swap(cl_params.images.A, cl_params.images.B);
}

void CLWPerlinNoiseKernel(CLWParams &cl_params) {
	cl_params.kernels.perlin.setArg(0, cl_params.images.A);
	cl_params.queue.enqueueNDRangeKernel(cl_params.kernels.perlin, cl::NullRange, cl::NDRange(TEX_WIDTH, TEX_HEIGTH), cl::NullRange);
	cl_params.queue.finish();

}

void CLWcalculate_screen(CLWRayTraceParams &rt_params, CLWParams &cl_params) {
	GLfloat3 up = { 0, 1, 0 };
	CLWcross_product(&up.x, &rt_params.cam_dir.x, &rt_params.screen_horz.x);

	CLWcross_product(&rt_params.cam_dir.x, &rt_params.screen_horz.x, &rt_params.screen_vert.x);
	rt_params.screen_origin = rt_params.cam_pos + rt_params.cam_dir * rt_params.screen_distance - rt_params.screen_horz / 2.0f - rt_params.screen_vert / 2.0f;

	float screen_buffer_data[15] = {
		(rt_params.cam_pos).x, (rt_params.cam_pos).y, (rt_params.cam_pos).z,
		(rt_params.cam_dir).x, (rt_params.cam_dir).y, (rt_params.cam_dir).z,
		(rt_params.screen_origin).x, (rt_params.screen_origin).y, (rt_params.screen_origin).z,
		(rt_params.screen_horz).x, (rt_params.screen_horz).y, (rt_params.screen_horz).z,
		(rt_params.screen_vert).x, (rt_params.screen_vert).y, (rt_params.screen_vert).z,
	};
	cl_params.queue.enqueueWriteBuffer(rt_params.screen_buffer, CL_FALSE, 0, sizeof(float) * 15, screen_buffer_data);
}

void CLWcross_product(const GLfloat* a, const GLfloat* b, GLfloat* target) {
	for (int i = 0; i < 3; i++) {
		int p = (i + 1) % 3;
		int q = (i + 2) % 3;
		target[i] = a[p] * b[q] - b[p] * a[q];
	}
}

