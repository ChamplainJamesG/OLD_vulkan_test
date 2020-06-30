/*
Jimmy Griffiths, Josh Mahler - March 16 2019
Initial Vulkan code to test if we can create a window
using glfw, multiply using glm, and close it. 
*/

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

#include "DemoApp.h"

int main()
{
	DemoApp app;

	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;

	/*
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);

	std::cout << extensionCount << " extensions supported!" << std::endl;

	glm::mat4 mat;
	glm::vec4 vec;

	glm::vec4 test = mat * vec;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();
	*/
	return 0;
}