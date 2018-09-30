#version 330 core
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;

uniform float u_time;

out vec3 normal;
out vec2 UV;

void main() {
	gl_Position.xyz = vertexPosition_modelspace;
	gl_Position.w = 1;
	UV = vertexUV;
}
