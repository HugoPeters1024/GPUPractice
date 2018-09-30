#pragma once

typedef struct {
	GLuint time;
} GLWUniformsContainer;

typedef struct {
	GLuint program;
	GLuint vao;
	GLuint texA;
	GLuint texB;
	GLWUniformsContainer uniforms;
} GLWParams;

/*
#define GLW_VERTEX_COUNT 18

static const GLfloat GLWvertices[GLW_VERTEX_COUNT] =
{
	-1.0f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	-1.0f, -1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
};

#define GLW_UV_COUNT 12
static const GLfloat GLWuvcoords[GLW_UV_COUNT] = {
	1.0f, 1.0f,
	0.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
};
*/

#define GLW_VERTEX_COUNT 18

static const GLfloat GLWvertices[GLW_VERTEX_COUNT] =
{
	-1.0f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
};

#define GLW_UV_COUNT 12
static const GLfloat GLWuvcoords[GLW_UV_COUNT] = {
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
};


void GLWInit(GLWParams* params);
void GLWInitTexture(GLWParams* params, float* textureBuffer);