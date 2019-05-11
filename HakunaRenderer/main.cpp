#define _CRTDBG_MAP_ALLOC
#define new new( _CLIENT_BLOCK, __FILE__, __LINE__)


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include "HakunaRenderer.h"
static _CrtMemState s1, s2, s3;
int main() {
	_CrtMemCheckpoint(&s1);
	HakunaRenderer renderer;
	try {
		renderer.Loop();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	_CrtMemCheckpoint(&s2);
	if (_CrtMemDifference(&s3, &s1, &s2))
		_CrtMemDumpStatistics(&s3);

	_CrtDumpMemoryLeaks();

	return EXIT_SUCCESS;
}
