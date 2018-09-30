#version 330 core

uniform float u_time;

in vec3 normal;
in vec2 UV;

out vec3 color;

uniform sampler2D u_texture;

void main(){
  color = texture2D(u_texture, UV).rgb;
}
