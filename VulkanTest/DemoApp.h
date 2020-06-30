/*
Jimmy Griffiths, Josh Mahler - March 16 2019
DemoApp.h
Wraps functionality of Vulkan into one object.

https://vulkan-tutorial.com/Drawing_a_triangle
*/

#ifndef DEMO_APP_H
#define DEMO_APP_H

// we could write #define GLFW_INCLUDE_VULKAN instead of #include vulkan
// setting that preprocessor flag forces glfw to include vulkan, but I want to explicit.
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>


#include <iostream>
#include <stdexcept> // used to catch any terrible errors
#include <functional> // includes lambda functions for resource management.
#include <cstdlib> // provides macros for main.
#include <optional> // has optional, which I don't really know how it works...
#include <algorithm>
#include <fstream>
#include <array>
#include <cmath>
#include <random>

//Vertex Data
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE //Used for depth buffering.
#define GLM_ENABLE_EXPERIMENTAL // So that we can hash GLM
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <chrono>

#define INSTANCE_COUNT 2048

const int WINDOW_WIDTH = 800, WINDOW_HEIGHT = 600;

const std::string MODEL_PATH = "models/utah_teapot.obj";
const std::string TEXTURE_PATH = "textures/Dan.bmp";

// Defines how many frames can be processed concurrently.
const int MAX_FRAMES_IN_FLIGHT = 2;

// Validation layers setup. 
const std::vector<const char*> validationLayers  = 
{
	"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> deviceExtensions = 
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// check to see whether we are in debug mode or not for validation layers.
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger);

void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAlloc);

// Queue family that supports graphics commands.
// Commands have to be submitted into a queue, this one supports graphics commands.
struct QueueFamilyIndices
{
	// Can or cannot contain a value, because thanks C++17
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

//comment 
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR mCapabilities;
	std::vector<VkSurfaceFormatKHR> mFormats;
	std::vector<VkPresentModeKHR> mPresentModes;
};

//Pre-instance data block
struct InstanceData
{
	glm::vec3 pos;
	glm::vec3 rot;
	float scale;
	uint32_t texIndex;
};

struct Vertex
{
	//glm provides us with C++ types that exactly match the vector types used.
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;

	//Tell Vulkan how to pass this data to the vertex shader
	static std::array<VkVertexInputBindingDescription, 2> getBindingDescription()
	{
		std::array<VkVertexInputBindingDescription, 2> bindingDescriptions = {};
	
		/*
		The binding parameter specifies the index of the binding in the array of bindings. 
		The stride parameter specifies the number of bytes from one entry to the next.
		The inputRate parameter can have one of the following values.

		VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex

		VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance
		*/
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		bindingDescriptions[1].binding = 1;
		bindingDescriptions[1].stride = sizeof(InstanceData);
		bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

		return bindingDescriptions;
	}

	static std::array<VkVertexInputAttributeDescription, 8> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 8> attributeDescriptions = {};

		/*
		The binding parameter tells Vulkan from which binding the per-vertex data comes.
		The location parameter references the location directive of the input in the vertex shader. 
		The input in the vertex shader with location 0 is the position, which has two 32-bit float components.

		The format parameter describes the type of data for the attribute. 
		A bit confusingly, the formats are specified using the same enumeration as color formats.
		The following shader types and formats are commonly used together:
		float: VK_FORMAT_R32_SFLOAT
		vec2: VK_FORMAT_R32G32_SFLOAT
		vec3: VK_FORMAT_R32G32B32_SFLOAT
		vec4: VK_FORMAT_R32G32B32A32_SFLOAT

		It is allowed to use more channels than the number of components in the shader, but they will be silently discarded. 
		If the number of channels is lower than the number of components, then the BGA components will use default values of (0, 0, 1). 
		The color type (SFLOAT, UINT, SINT) and bit width should also match the type of the shader input. See the following examples:

		ivec2: VK_FORMAT_R32G32_SINT, a 2-component vector of 32-bit signed integers
		uvec4: VK_FORMAT_R32G32B32A32_UINT, a 4-component vector of 32-bit unsigned integers
		double: VK_FORMAT_R64_SFLOAT, a double-precision (64-bit) float
		*/
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, normal);

		// instance pos
		attributeDescriptions[4].binding = 1;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(InstanceData, InstanceData::pos);

		// instance rot
		attributeDescriptions[5].binding = 1;
		attributeDescriptions[5].location = 5;
		attributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[5].offset = sizeof(float) * 3;

		// instance scale.
		attributeDescriptions[6].binding = 1;
		attributeDescriptions[6].location = 6;
		attributeDescriptions[6].format = VK_FORMAT_R32_SFLOAT;
		attributeDescriptions[6].offset = sizeof(float) * 6;

		// instance texture array layer index (thank mr sascha willems)
		attributeDescriptions[7].binding = 1;
		attributeDescriptions[7].location = 7;
		attributeDescriptions[7].format = VK_FORMAT_R32_SINT;
		attributeDescriptions[7].offset = sizeof(float) * 7;

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

//Contains the instanced data
struct InstanceBuffer
{
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	size_t size = 0;
	VkDescriptorBufferInfo descriptor;
};


// standard hash operator so that we can use a map.
// https://en.cppreference.com/w/cpp/utility/hash
namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

/*
const std::vector<Vertex> vertices = {
	{ { -0.5f, -0.5f, 0.0f },{ 1.0f, 0.0f, 0.0f }, {0.0f, 0.0f} },
	{ { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f }, {1.0f, 0.0f} },
	{ { 0.5f, 0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f }, {1.0f, 1.0f} },
	{ { -0.5f, 0.5f, 0.0f },{ 1.0f, 1.0f, 1.0f }, {0.0f, 1.0f} },

	{ { -0.5f, -0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } },
	{ { 0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } },
	{ { 0.5f, 0.5f, -0.5f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
	{ { -0.5f, 0.5f, -0.5f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f } }
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4
};
*/

/*
Vulkan expects the data in your structure to be aligned in memory in a specific way, for example:

Scalars have to be aligned by N (= 4 bytes given 32 bit floats).
A vec2 must be aligned by 2N (= 8 bytes)
A vec3 or vec4 must be aligned by 4N (= 16 bytes)
A nested structure must be aligned by the base alignment of its members rounded up to a multiple of 16.
A mat4 matrix must have the same alignment as a vec4.
*/
struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;

	alignas(16) glm::vec4 uLightPos;
	alignas(16) glm::vec4 uLightCol;
};


class DemoApp
{
public:
	// for demo purposes, will just call things as needed.
	void run();

private:
	// initApp will initialize the application, vulkan objects, and so on.
	void initApp();

	// initialize our vulkan stuff.
	void initVulkan();
	void createInstance();
	void setupDebugManager();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin);
	void createCommandBuffers();
	void createSyncObjects();

	void recreateSwapChain();
	//static because GLFW doesn't know how to call a member function with the right this pointer
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	void loadModel();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();

	void prepareInstanceData();

	void createColorResources();
	void createDepthResources();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features); // Helper function to get the best depth format on per device.
	VkFormat findDepthFormat(); // Helper function using ^ to get the best depth format.
	bool hasStencilComponent(VkFormat format) { return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT; };

	void createTextureImage();
	void createImage(uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

	void createTextureImageView();
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	void createTextureSampler();
	VkSampleCountFlagBits getMaxUsableSampleCount();

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandbuffer);

	void transitionImageLayout(VkImage image, VkFormat  format, VkImageLayout oldLayout, VkImageLayout newLayout);

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void updateUniformBuffer(uint32_t currentImage);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void createBuffer2(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, void* data);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	static std::vector<char> readFile(const std::string& filename);
	VkShaderModule createShaderModule(const std::vector<char>& code);

	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();

	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	// I mean, it's the game loop.
	void gameLoop();
	void drawFrame();

	// cleanup objects, freeing memory and other important things.
	void cleanApp();
	void cleanupSwapChain();


	VkInstance mInstance;
	VkDebugUtilsMessengerEXT mDebugMessenger; // object to store debug messages, and will call debugCallback.
	VkPhysicalDevice mPhysDevice; // The graphics card that we select.
	GLFWwindow* mWindow;
	VkDevice mDevice; // Logical device to interface with our physical device.
	VkQueue mGraphicsQueue; // Handle to interface with queues on our logical device.
	VkSurfaceKHR mSurface; // Window System Integration (WSI) extension to display outputs to screen.
	VkQueue mPresentQueue; // Queue to put stuff on the screen.
	VkSwapchainKHR mSwapChain; // Swap chain for image rendering.
	std::vector<VkImage> mSwapChainImages; // Holder for images from swap chain.
	VkFormat mSwapChainImageFormat;	// Format for the images in swap chain.
	VkExtent2D mSwapChainExtent; // Extent for the images in swap chain.
	std::vector<VkImageView> mSwapChainImageViews; // View into an image.
	VkRenderPass mRenderPass; // The render pass.
	VkDescriptorSetLayout mDescriptorSetLayout; // Tells Vulkan what type of shader we are using.
	VkPipelineLayout mPipelineLayout; // Pipeline layout.
	VkPipeline mGraphicsPipeline; // Literally the pipeline. 
	std::vector<VkFramebuffer> mSwapChainFramebuffers;  // Stores all of the VkImageViews in a list of Framebuffers.
	VkCommandPool mCommandPool; // Manage the memory that is used to store the buffers and command buffers are allocated from them.
	std::vector<VkCommandBuffer> mCommandBuffers;
	std::vector<VkSemaphore> mImageAvailableSemaphores;
	std::vector<VkSemaphore> mRenderFinishedSemaphores;
	std::vector<VkFence> inFlightFences; // Fences are similar to semaphores in the sense that they can be signaled and waited for, but this time we actually wait for them in our own code.
	size_t mCurrentFrame = 0; // The current frame that we're on.
	bool framebufferResized = false; // Was the framebuffer resized?
	VkBuffer mVertexBuffer;
	VkDeviceMemory mVertexBufferMemory;
	VkBuffer mIndexBuffer; // What we will use for instancing.
	VkDeviceMemory mIndexBufferMemory; // Total memory we have to instance.
	std::vector<VkBuffer> mUniformBuffers; // The uniform buffers we have.
	std::vector<VkDeviceMemory> mUniformBuffersMemory; // Total memory we have for the uniform buffers.
	VkDescriptorPool mDescriptorPool;  // Holds all descriptor sets
	std::vector<VkDescriptorSet> mDescriptorSets; // The descriptor sets

	VkImage mTextureImage; // Image object to retrieve colors by allowing 2D coords & texels.
	VkDeviceMemory mTextureImageMemory; // memory for ^
	VkImageView mTextureImageView;
	VkSampler mTextureSampler;

	//Depth stuff
	VkImage mDepthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView mDepthImageView;

	// vertices
	std::vector <Vertex> mVertices;
	std::vector<uint32_t> mIndices;
	//VkBuffer mVertexBuffer;
	//VkDeviceMemory mVertexBufferMemory;

	//Multisampling
	VkSampleCountFlagBits mMSAASamples = VK_SAMPLE_COUNT_1_BIT;
	VkImage mColorImage;
	VkDeviceMemory mColorImageMemory;
	VkImageView mColorImageView;

	InstanceBuffer mInstanceBuffer;
};

#endif // !DEMO_APP_H
