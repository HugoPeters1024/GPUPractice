#pragma once
#include <CL/cl.hpp>

#define SCR_WIDTH 512
#define SCR_HEIGTH 512

#define PARTICLE_ROOT 256
#define PARTICLE_COUNT (PARTICLE_ROOT * PARTICLE_ROOT)

typedef struct {
	GLuint program;
	GLuint vao;
	GLuint texture;
} GL_PARAMS;

typedef struct {
	cl::Device device;
	cl::CommandQueue queue;
	cl::Program program;
	cl::Context context;
	cl::ImageGL texture;
	cl::size_t<3> dims;
	cl::Kernel ParticleSystem;
	cl::Kernel GenerateParticles;
	cl::Kernel ClearImage;
	cl::Buffer p_positions;
	cl::Buffer p_velocity;
} CL_PARAMS;

#define VERTEX_COUNT 18
static const GLfloat vertices[VERTEX_COUNT] = {
	-1.0f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
};

#define UV_COUNT 12
static const GLfloat uvcoords[UV_COUNT] = {
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
};