#pragma once
#include "pch.h"
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/GL.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <iostream>

void CreateWindow(GLFWwindow *window, int IMG_WIDTH, int IMG_HEIGTH);