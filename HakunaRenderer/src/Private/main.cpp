#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#ifdef _DEBUG
#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
#define new new
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <iostream>
#include "HakunaRenderer.h"
#ifdef _DEBUG
static _CrtMemState s1, s2, s3;
#endif // _DEBUG


#include "draw_item.h"
#include "perinstance_data.hpp"
int main() {

	DrawItem<PerInstanceDataWithLightmap> testDI;
	testDI.print_instance_data();

#ifdef _DEBUG
	_CrtMemCheckpoint(&s1);
#endif // _DEBUG
	{
		HakunaRenderer renderer;
		try {
			renderer.Loop();
		}
		catch (const std::exception & e)
		{
			std::cerr << e.what() << std::endl;
			return EXIT_FAILURE;
		}
	}
#ifdef _DEBUG
	_CrtMemCheckpoint(&s2);
	try {
		if (_CrtMemDifference(&s3, &s1, &s2))
		{
			_CrtMemDumpStatistics(&s3);
			_CrtDumpMemoryLeaks();
			throw "Memory Leak!";
		}
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
#endif
	return EXIT_SUCCESS;
}



