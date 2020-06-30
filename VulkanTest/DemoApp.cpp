/*
Jimmy Griffiths, Josh Mahler
definitions for the functions in DemoApp.h
*/

#include "DemoApp.h"
#include <set>

// Used for texture loading.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Model Loading
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>


// So... When we're creating a vk debug boy, we need to pass the createInfo to a vkCreateDebugUtilsMessengerEXT function.
// However, since that function is an extension, we need to load it ourselves by looking up its address using
// vkGetInstanceProcAddr.
VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// Like above, we now need to find the function to remove the debug messenger.
void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAlloc)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, debugMessenger, pAlloc);
}

// init, run the loop, and cleanup all in one thing. This should be all that main needs.
void DemoApp::run()
{
	initApp();
	initVulkan();
	gameLoop();
	cleanApp();
}

// initalizes the app. We are assuming it always returns true for demo purposes.
// In production code, we would do several checks, but that can be a todo later.
void DemoApp::initApp()
{
	// Initalize glfw. Window hints tell it specific things.
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Do not create an openGL context.

	mWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_WIDTH, "Jaminal3D_EMA", nullptr, nullptr); // then just create the window!
	glfwSetWindowUserPointer(mWindow, this);
	glfwSetFramebufferSizeCallback(mWindow, framebufferResizeCallback); //Set up the framebuffer resize callback
}

// Initializes a lot of vulkan stuff. Like, everything.
// General pattern for vulkan object creation:
// Pointer to struct with creation info
// Pointer to custom allocator callbacks (NULL in our case)
// Pointer to the variable that stores the handle for the object.
void DemoApp::initVulkan()
{
	createInstance();
	setupDebugManager();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandPool();
	createColorResources();
	createDepthResources();
	createFramebuffers();
	createTextureImage();
	createTextureImageView();
	createTextureSampler();
	loadModel();
	prepareInstanceData();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
	createSyncObjects();
}

void DemoApp::createInstance()
{
	// Check for validation layer support.
	if (enableValidationLayers && !checkValidationLayerSupport())
	{
		throw std::runtime_error("validation layer requested, but there were none available. Rip bucko.");
	}

	// A lot of Vulkan info is passed through structs (https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Instance)
	// So we need to tell the drivers some information about our application.
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Test";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Jaminal3D";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// Now, we need just a bit more information before we can finalize our instance.
	// CreateInfo tells the driver which extensions & validation layers to use.
	VkInstanceCreateInfo createInfo = {}; // does weird default init things. I had to look it up. (https://en.cppreference.com/w/cpp/language/value_initialization)
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// debug output that shows all available vk extensions we can use.
	uint32_t extCt = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCt, nullptr); // get the amount of extensions we have.

	std::vector<VkExtensionProperties> extensions(extCt); // again, this initalizes everything in the list to default. See above link ^

	vkEnumerateInstanceExtensionProperties(nullptr, &extCt, extensions.data()); // get that data in a list now.

	// output
	std::cout << "--------------------------------" << std::endl;
	std::cout << "All available vk extensions: " << std::endl;

	for (VkExtensionProperties ext : extensions)
		std::cout << ext.extensionName << std::endl;

	std::cout << "--------------------------------";

	// now get glfw extensions that we need.
	std::vector<const char*> exts = getRequiredExtensions();

	createInfo.enabledExtensionCount = static_cast<uint32_t>(exts.size());
	createInfo.ppEnabledExtensionNames = exts.data();

	// Now, we tell how many validation layers to use.
	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
		createInfo.enabledLayerCount = 0;


	// Create the VK instance, and immediately check if it was successful. If not, throw the error.
	if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create instance. God help us.");
	}
}

// setup our vk debug messenger object
void DemoApp::setupDebugManager()
{
	// Do our createinfo struct again, and fill it with all of the types of stuff we can use for our debug messenger.
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback; // Looking at the Vulkan_core.h, this is literally magic. I have no fucking idea what is happening here. But we're giving Vulkan the ability to call our own shit.
	createInfo.pUserData = nullptr; // Optional

	// we need to tell our current vulkan instance to make the thing.
	if (createDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessenger) != VK_SUCCESS)
		throw std::runtime_error("Failed to set up the debug messenger.");
}

void DemoApp::createSurface()
{
	if (glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}


}

// Pick the physical device that we will be using.
void DemoApp::pickPhysicalDevice()
{
	// very similar to extensions or layers, unsurprsingly.
	uint32_t deviceCt = 0;
	vkEnumeratePhysicalDevices(mInstance, &deviceCt, nullptr);

	if (deviceCt == 0)
		throw std::runtime_error("No GPU with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(deviceCt);
	vkEnumeratePhysicalDevices(mInstance, &deviceCt, devices.data());

	// check to make sure devices are suitable
	for (VkPhysicalDevice dev : devices)
	{
		if (isDeviceSuitable(dev))
		{
			mPhysDevice = dev;
			mMSAASamples = getMaxUsableSampleCount();
			break;
		}
	}

	if (mPhysDevice == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to find a suitable GPU.");
}

void DemoApp::createLogicalDevice()
{
	//Set up queues
	QueueFamilyIndices indices = findQueueFamilies(mPhysDevice);

	std::vector<VkDeviceQueueCreateInfo> deviceQueueCreationInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		deviceQueueCreationInfos.push_back(queueCreateInfo);
	}



	//Come back to this once we're trying to do more complicated stuff
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE; //enable sample shading feature for the device

	//deviceFeatures.textureCompressionBC = VK_TRUE; //enable texture compression
	//deviceFeatures.textureCompressionASTC_LDR = VK_TRUE;
	//deviceFeatures.textureCompressionETC2 = VK_TRUE;

	//Set up pointers to queue creation
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreationInfos.size());
	createInfo.pQueueCreateInfos = deviceQueueCreationInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	//createInfo.enabledExtensionCount = 0;

	// enabledLayerCount and ppEnabledLayerNames are ignored by latest implementations,
	// still good to check to be compatible with older implemntations
	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(mPhysDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(mDevice, indices.graphicsFamily.value(), 0, &mGraphicsQueue);
	vkGetDeviceQueue(mDevice, indices.presentFamily.value(), 0, &mPresentQueue);
}

void DemoApp::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(mPhysDevice);

	// Use helper functions to give us our format, present mode, and extent (bounds)
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.mFormats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.mPresentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.mCapabilities);

	// +1 because using just min means sometimes we have to wait on the driver to complete internal ops before aquiring another image to render to.
	uint32_t imageCount = swapChainSupport.mCapabilities.minImageCount + 1;
	
	// Check to see if it's within bounds
	if (swapChainSupport.mCapabilities.maxImageCount > 0 && imageCount > swapChainSupport.mCapabilities.maxImageCount)
		imageCount = swapChainSupport.mCapabilities.maxImageCount;

	// Set up swap chain info structure
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = mSurface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// Get the queue for the swap chain
	QueueFamilyIndices indices = findQueueFamilies(mPhysDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	/*
	VK_SHARING_MODE_EXCLUSIVE: 
	An image is owned by one queue family at a time and ownership must be
	explicitly transfered before using it in another queue family.
	This option offers the best performance.

	VK_SHARING_MODE_CONCURRENT:
	Images can be used across multiple
	queue families without explicit ownership transfers.
	*/
	// Avoiding ownership by using CONCURRENT if the queue families differ
	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	/*
	We can specify that a certain transform should be applied to 
	images in the swap chain if it is supported (supportedTransforms in capabilities), 
	like a 90 degree clockwise rotation or horizontal flip.
	Specify no transformation by setting it equal to currentTransform
	*/
	createInfo.preTransform = swapChainSupport.mCapabilities.currentTransform;

	// Ignore alpha channel during blending
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	
	createInfo.presentMode = presentMode;

	// clipped set to VK_TRUE means we don't care about obstructed objects 
	createInfo.clipped = VK_TRUE;

	// In case of resize of window, swap chain must be recreated from scratch
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapChain) != VK_SUCCESS)
		throw std::runtime_error("failed to create swap chain!");

	// Infamous double function call
	vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, nullptr);
	mSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, mSwapChainImages.data());

	// Give the images their format and extent
	mSwapChainImageFormat = surfaceFormat.format;
	mSwapChainExtent = extent;
}

void DemoApp::createImageViews()
{
	mSwapChainImageViews.resize(mSwapChainImages.size());

	for (size_t i = 0; i < mSwapChainImages.size(); ++i)
	{
		/*
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = mSwapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = mSwapChainImageFormat;

		// SWIZZLEEEEEEEEEEEEEE
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		// Describes what the image's purpose is and which part of the image should be accessed.
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(mDevice, &createInfo, nullptr, &mSwapChainImageViews[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create image views!");
			*/

		mSwapChainImageViews[i] = createImageView(mSwapChainImages[i], mSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void DemoApp::createRenderPass()
{
	// A color buffer attachment represented by one of the images from the swap chain
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = mSwapChainImageFormat;
	colorAttachment.samples = mMSAASamples; // >1 if multisampling
	/*
		VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment

		VK_ATTACHMENT_LOAD_OP_CLEAR: Clear the values to a constant at the start

		VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined; we don't care about them

		VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later

		VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents of the framebuffer will be undefined after the rendering operation
	*/
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	// We don't care about depth!
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/*
		Remember that textures and framebuffers are represented by VkImage objects.
		common layouts for those include:

		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment

		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain

		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Images to be used as destination for a memory copy operation

		The initialLayout specifies which layout the image will have before the render pass begins. 
		The finalLayout specifies the layout to automatically transition to when the render pass finishes. 
		Using VK_IMAGE_LAYOUT_UNDEFINED for initialLayout means that we don't care what previous layout the image was in.
	*/
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// a single render pass can have multiple subpasses (I.e, for post-processing)

	/*
	The format should be the same as the depth image itself. 
	This time we don't care about storing the depth data (storeOp), because it will not be used after drawing has finished.
	*/
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = mMSAASamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	/*
	Multisampled images cannot be presented directly.
	We first need to resolve them to a regular image.
	This requirement does not apply to the depth buffer,
	since it won't be presented at any point. 
	Therefore we will have to add only one new attachment for color which is a so-called resolve attachment:
	*/
	VkAttachmentDescription colorAttachmentResolve = {};
	colorAttachmentResolve.format = mSwapChainImageFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	//We intend to use the attachment as a color buffer.
	VkAttachmentReference colorAttachRef = {};
	colorAttachRef.attachment = 0; // the index of the attachment descriptions array.
	colorAttachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachRef = {};
	depthAttachRef.attachment = 1;
	depthAttachRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	/*
	The index of the attachment in this array is directly referenced from the fragment shader with the 
	layout(location = 0) out vec4 outColor directive!

	Other types of attachments:
	pInputAttachments: Attachments that are read from a shader

	pResolveAttachments: Attachments used for multisampling color attachments

	pDepthStencilAttachment: Attachments for depth and stencil data

	pPreserveAttachments: Attachments that are not used by this subpass, 
	but for which the data must be preserved
	*/
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachRef;
	subpass.pDepthStencilAttachment = &depthAttachRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	VkSubpassDependency dependency = {};
	/*
	The special value VK_SUBPASS_EXTERNAL refers to the implicit subpass before or 
	after the render pass depending on whether it is specified in srcSubpass or dstSubpass. 
	The index 0 refers to our subpass, which is the first and only one. 
	The dstSubpass must always be higher than srcSubpass to prevent cycles in the dependency graph.
	*/
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	/*
	We need to wait for the swap chain to finish reading from the image before we can access it. 
	This can be accomplished by waiting on the color attachment output stage itself.
	*/
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	/*
	The operations that should wait on this are in the color attachment stage and 
	involve the reading and writing of the color attachment. 
	These settings will prevent the transition from happening until 
	it's actually necessary (and allowed): 
	when we want to start writing colors to it.
	*/
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass!");
}

void DemoApp::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	/*
	The first two fields specify the binding used in the shader and the type of descriptor, 
	which is a uniform buffer object. 
	It is possible for the shader variable to represent an array of uniform buffer objects,
	and descriptorCount specifies the number of values in the array. 
	This could be used to specify a transformation for each of the bones in a skeleton for skeletal animation,
	for example. Our MVP transformation is in a single uniform buffer object, so we're using a descriptorCount of 1.
	*/
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;

	//The stageFlags field can be a combination of VkShaderStageFlagBits values or the value VK_SHADER_STAGE_ALL_GRAPHICS
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(mDevice, &layoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to crate descriptor set layout!");
}

void DemoApp::createGraphicsPipeline()
{
	std::vector<char> vertShaderCode = readFile("shaders/vert.spv");
	std::vector<char> fragShaderCode = readFile("shaders/frag.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	//Vertex shader pipeline stage
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

	//Give creation instatnce code and define entry point of code
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	//Fragment shader pipeline stage
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Give creation instatnce code and define entry point of code
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	/*
	Bindings: spacing between data and whether the data is per-vertex or per-instance
	
	Attribute descriptions: type of the attributes passed to the vertex shader, 
	which binding to load them from and at which offset

	passing stuff to the vertex shader
	*/
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	std::array<VkVertexInputBindingDescription, 2> bindingDescriptions = Vertex::getBindingDescription();

	std::array<VkVertexInputAttributeDescription, 8> attributeDescriptions = Vertex::getAttributeDescriptions();
	
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	/*
	TOPOLOGY STUFF - how to use vertices

	VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices

	VK_PRIMITIVE_TOPOLOGY_LINE_LIST: line from every 2 vertices without reuse

	VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: the end vertex of every line is used as start 
	vertex for the next line

	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every 3 vertices without reuse

	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: the second and third vertex of every triangle are
	used as first two vertices of the next triangle
	*/
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	//The viewport is what the user sees
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)mSwapChainExtent.width;
	viewport.height = (float)mSwapChainExtent.height;
	// must be within 0 and 1
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	/*
	While viewports define the transformation from the image to the framebuffer,
	scissor rectangles define in which regions pixels will actually be stored.
	Any pixels outside the scissor rectangles will be discarded by the rasterizer.
	They function like a filter rather than a transformation.
	*/
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = mSwapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	/*
	If depthClampEnable is set to VK_TRUE, 
	then fragments that are beyond the near and far planes are 
	clamped to them as opposed to discarding them.
	*/
	rasterizer.depthClampEnable = VK_FALSE;

	/*
	If rasterizerDiscardEnable is set to VK_TRUE,
	then geometry never passes through the rasterizer stage. 
	This basically disables any output to the framebuffer.
	*/
	rasterizer.rasterizerDiscardEnable = VK_FALSE;

	/*
	VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments

	VK_POLYGON_MODE_LINE: polygon edges are drawn as lines

	VK_POLYGON_MODE_POINT: polygon vertices are drawn as points
	
	Using any mode other than fill requires enabling a GPU feature.
	*/
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

	rasterizer.lineWidth = 1.0f;

	/*
	The cullMode variable determines the type of face culling to use. 
	You can disable culling, cull the front faces, 
	cull the back faces or both. 
	The frontFace variable specifies the vertex order for faces
	to be considered front-facing and can be clockwise or counterclockwise.
	*/
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	//Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE; //enable sample shading in the pipeline
	multisampling.rasterizationSamples = mMSAASamples;
	multisampling.minSampleShading = 1.0f; // min fraction for sample shading; closer to one is smoother
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	//Activate color blending with this link
	//https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
	//Disable color blending
	//ColorBlendAttachmentState is a per framebuffer state
	//ColorBlendStateCreateInfo is a global state
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	// This the global boy
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	/*
	The depthTestEnable field specifies if the depth of new fragments should be compared to the depth buffer to see if they should be discarded. 
	The depthWriteEnable field specifies if the new depth of fragments that pass the depth test should actually be written to the depth buffer. 
	This is useful for drawing transparent objects.
	*/
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;

	/*
	The depthCompareOp field specifies the comparison that is performed to keep or discard fragments
	*/
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

	/*
	The depthBoundsTestEnable, minDepthBounds and maxDepthBounds fields are used for the optional depth bound test. 
	Basically, this allows you to only keep fragments that fall within the specified depth range. We won't be using this functionality.
	*/
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = .0f;
	depthStencil.maxDepthBounds = 1.f;

	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};
			
	//Viewport and line width can be modified dynamically
	VkDynamicState dynamicStates[] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &mDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout!");

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	// Literally just add in everything.
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil; //Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; //Optional

	// reference the pipeline layout Vulkan handle
	pipelineInfo.layout = mPipelineLayout;

	// get the reference to the render pass & index of sub pass.
	pipelineInfo.renderPass = mRenderPass;
	pipelineInfo.subpass = 0;

	/*
	Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline. 
	The idea of pipeline derivatives is that it is less expensive to set up pipelines when they 
	have much functionality in common with an existing pipeline and switching between pipelines 
	from the same parent can also be done quicker.
	*/
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	/*
	vkCreateGraphicsPipelines can take more than 1 pipeline to create, but we're just giving it the one.
	*/
	if (vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphicsPipeline) != VK_SUCCESS)
		throw std::runtime_error("Failed to bop up the graphics pipeline.");

	vkDestroyShaderModule(mDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(mDevice, vertShaderModule, nullptr);
}

void DemoApp::createFramebuffers()
{
	mSwapChainFramebuffers.resize(mSwapChainImageViews.size());
	// It's straightforward.
	for (size_t i = 0; i < mSwapChainImageViews.size(); ++i)
	{
		/*
		The color attachment differs for every swap chain image, 
		but the same depth image can be used by all of them because only a single subpass is running at the same time due to our semaphores.
		*/
		std::array<VkImageView, 3> attachments = { mColorImageView, mDepthImageView, mSwapChainImageViews[i] };
		//VkImageView attachments[] = { mSwapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = mRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = mSwapChainExtent.width;
		framebufferInfo.height = mSwapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &mSwapChainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create framebuffer!");
	}
}

void DemoApp::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(mPhysDevice);
	
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0; //Optional

	/*
		There are two possible flags for command pools:

		VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with 
		new commands very often (may change memory allocation behavior)

		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually,
		without this flag they all have to be reset together
	*/
	if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool!");
}

void DemoApp::createColorResources()
{
	//We will now create a multisampled color buffer.
	VkFormat colorFormat = mSwapChainImageFormat;

	createImage(mSwapChainExtent.width, mSwapChainExtent.height, mMSAASamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mColorImage, mColorImageMemory);
	mColorImageView = createImageView(mColorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);

	transitionImageLayout(mColorImage, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

void DemoApp::createDepthResources()
{
	VkFormat depthFormat = findDepthFormat();

	createImage(mSwapChainExtent.width, mSwapChainExtent.height, mMSAASamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mDepthImage, depthImageMemory);
	mDepthImageView = createImageView(mDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	transitionImageLayout(mDepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

VkFormat DemoApp::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(mPhysDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			return format;

		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			return format;
	}

	throw std::runtime_error("Could not find supported format!");
}

VkFormat DemoApp::findDepthFormat()
{
	return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void DemoApp::createTextureImage()
{
	// We use stb to make image loading easy.
	int texWidth, texHeight, texChannels;

	/*
	The stbi_load function takes the file path and number of channels to load as arguments. 
	The STBI_rgb_alpha value forces the image to be loaded with an alpha channel, even if it doesn't have one.

	The pixels are laid out row by row with 4 bytes per pixel in the case of STBI_rgba_alpha for a total of texWidth * texHeight * 4 values
	*/
	stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels)
		throw std::runtime_error("Failed to load texture image!");

	// We're now going to create a buffer in host visible memory so that we can use vkMapMemory and copy the pixels to it.
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// The buffer should be in host visible memory so that we can map it and it should be usable as a transfer source so that we can copy it to an image later on
	createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	// We can then directly copy the pixel values that we got from the image loading library to the buffer
	void* data;
	vkMapMemory(mDevice, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(mDevice, stagingBufferMemory);

	stbi_image_free(pixels);

	//VkCmdBlit is considered a transfer operation, so we must inform Vulkan that we intend to use 
	//the texture image as both the source and destination of a transfer.
	//Add VK_IMAGE_USAGE_TRANSFER_SRC_BIT to the texture image's usage flags in createTextureImage:
	createImage(texWidth, texHeight, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		mTextureImage, mTextureImageMemory);

	transitionImageLayout(mTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, mTextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	transitionImageLayout(mTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
	vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
}

VkSampleCountFlagBits DemoApp::getMaxUsableSampleCount()
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(mPhysDevice, &physicalDeviceProperties);

	//the lower number will be the maximum we can support.
	VkSampleCountFlags counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
	
	if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT; 
	if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT; 
	if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT; 
	if (counts & VK_SAMPLE_COUNT_8_BIT)  return VK_SAMPLE_COUNT_8_BIT; 
	if (counts & VK_SAMPLE_COUNT_4_BIT)  return VK_SAMPLE_COUNT_4_BIT; 
	if (counts & VK_SAMPLE_COUNT_2_BIT)  return VK_SAMPLE_COUNT_2_BIT; 

	return VK_SAMPLE_COUNT_1_BIT;
}

// Used to abstract image creation
void DemoApp::createImage(uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	/*
	coordinate system the texels in the image are going to be addressed. It is possible to create 1D, 2D and 3D images.
	The extent field specifies the dimensions of the image. Basically how many texels there are on each axis.

	The tiling field can have one of two values:

	VK_IMAGE_TILING_LINEAR: Texels are laid out in row-major order like our pixels array
	VK_IMAGE_TILING_OPTIMAL: Texels are laid out in an implementation defined order for optimal access

	Unlike the layout of an image, the tiling mode cannot be changed at a later time.
	If you want to be able to directly access texels in the memory of the image,
	then you must use VK_IMAGE_TILING_LINEAR. We will be using a staging buffer instead of a staging image,
	so this won't be necessary

	There are only two possible values for the initialLayout of an image:

	VK_IMAGE_LAYOUT_UNDEFINED: Not usable by the GPU and the very first transition will discard the texels.
	VK_IMAGE_LAYOUT_PREINITIALIZED: Not usable by the GPU, but the first transition will preserve the texels.

	There are few situations where it is necessary for the texels to be preserved during the first transition.
	One example, however, would be if you wanted to use an image as a staging image in combination with the VK_IMAGE_TILING_LINEAR layout.

	The usage field has the same semantics as the one during buffer creation.
	The image is going to be used as destination for the buffer copy, so it should be set up as a transfer destination.
	We also want to be able to access the image from the shader to color our mesh, so the usage should include VK_IMAGE_USAGE_SAMPLED_BIT

	The samples flag is related to multisampling.
	This is only relevant for images that will be used as attachments, so stick to one sample.
	There are some optional flags for images that are related to sparse images. Sparse images are images where
	only certain regions are actually backed by memory. If you were using a 3D texture for a voxel terrain, for example,
	then you could use this to avoid allocating memory to store large volumes of "air" values
	*/
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(width);
	imageInfo.extent.height = static_cast<uint32_t>(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format; // Vulkan supports many possible image formats, but we should use the same format for the texels as the pixels in the buffer
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // The image will only be used by one queue family: the one that supports graphics
	imageInfo.samples = numSamples;
	imageInfo.flags = 0; // Optional.

	if (vkCreateImage(mDevice, &imageInfo, nullptr, &image))
		throw std::runtime_error("Failed to create image!");

	// Allocating memory for an image works in exactly the same way as allocating memory for a buffer
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(mDevice, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate image memory.");

	vkBindImageMemory(mDevice, image, imageMemory, 0);
}

void DemoApp::createTextureImageView()
{
	mTextureImageView = createImageView(mTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}

VkImageView DemoApp::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	// Remember, images are accessed through image views.
	// very similar to createImageViews
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(mDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void DemoApp::createTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	// The magFilter and minFilter fields specify how to interpolate texels that are magnified or minified. 
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	/*
	Note that the axes are called U, V and W instead of X, Y and Z. 
	This is a convention for texture space coordinates.

	VK_SAMPLER_ADDRESS_MODE_REPEAT: Repeat the texture when going beyond the image dimensions.
	VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: Like repeat, but inverts the coordinates to mirror the image when going beyond the dimensions.
	VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: Take the color of the edge closest to the coordinate beyond the image dimensions.
	VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: Like clamp to edge, but instead uses the edge opposite to the closest edge.
	VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: Return a solid color when sampling beyond the dimensions of the image.
	*/
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	// texels are addressed using the [0, 1) range on all axes.
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	/*
	If a comparison function is enabled, then texels will first be compared to a value, 
	and the result of that comparison is used in filtering operations.
	*/
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0;
	samplerInfo.minLod = 0;
	samplerInfo.maxLod = 0;

	if (vkCreateSampler(mDevice, &samplerInfo, nullptr, &mTextureSampler))
		throw std::runtime_error("Failed to create sampler!");
}

VkCommandBuffer DemoApp::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = mCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(mDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void DemoApp::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(mGraphicsQueue);

	vkFreeCommandBuffers(mDevice, mCommandPool, 1, &commandBuffer);

}

void DemoApp::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	/*
	One of the most common ways to perform layout transitions is using an image memory barrier. 
	A pipeline barrier like that is generally used to synchronize access to resources, 
	like ensuring that a write to a buffer completes before reading from it
	*/
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	/*
	The first two fields specify layout transition. 
	It is possible to use VK_IMAGE_LAYOUT_UNDEFINED as oldLayout if you don't care about the existing contents of the image.

	If you are using the barrier to transfer queue family ownership, 
	then these two fields should be the indices of the queue families. They must be set to VK_QUEUE_FAMILY_IGNORED if you don't want to do this

	The image and subresourceRange specify the image that is affected and the specific part of the image. 
	Our image is not an array and does not have mipmapping levels, so only one level and layer are specified.
	*/
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format))
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	/*
	There are two transitions we need to handle:

	Undefined -> transfer destination: transfer writes that don't need to wait on anything
	Transfer destination -> shader reading: shader reads should wait on transfer writes, 
	specifically the shader reads in the fragment shader, because that's where we're going to use the texture

	For more info on this:
	https://vulkan-tutorial.com/Texture_mapping/Images
	https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPipelineStageFlagBits.html
	*/
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else 
	{
		throw std::invalid_argument("unsupported layout transition!");
	}

	/*
	All types of pipeline barriers are submitted using the same function. 
	The first parameter after the command buffer specifies in which pipeline stage the operations occur that should happen before the barrier. 
	The second parameter specifies the pipeline stage in which operations will wait on the barrier. T
	he pipeline stages that you are allowed to specify before and after the barrier depend on how you use the resource before and after the barrier. 
	The allowed values are listed in this table of the specification:
	https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#synchronization-access-types-supported

	The third parameter is either 0 or VK_DEPENDENCY_BY_REGION_BIT. 
	The latter turns the barrier into a per-region condition. 
	That means that the implementation is allowed to already begin reading from the parts of a resource that were written so far, for example.

	The last three pairs of parameters reference arrays of pipeline barriers of the three available types:
	memory barriers, buffer memory barriers, and image memory barriers like the one we're using here.
	*/
	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(commandBuffer);
}

void DemoApp::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	// Just like with buffer copies, you need to specify which part of the buffer is going to be copied to which part of the image

	/*
	The bufferOffset specifies the byte offset in the buffer at which the pixel values start. 
	The bufferRowLength and bufferImageHeight fields specify how the pixels are laid out in memory. 
	For example, you could have some padding bytes between rows of the image. Specifying 0 for both indicates that the pixels are simply tightly packed like they are in our case. 
	The imageSubresource, imageOffset and imageExtent fields indicate to which part of the image we want to copy the pixels
	*/
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

void DemoApp::loadModel()
{
	/*
	Standard way to load models.

	The attrib container holds all of the positions, normals and texture coordinates in its attrib.vertices, 
	attrib.normals and attrib.texcoords vectors. The shapes container contains all of the separate objects and their faces
	*/

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
		throw std::runtime_error(warn + err);

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex = {};

			/*
			Unfortunately the attrib.vertices array is an array of float values 
			instead of something like glm::vec3, so you need to multiply the index by 3

			Same as above with texcoords.
			*/
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			if(attrib.texcoords.size() > 0)
				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			vertex.color = { 1.f, 1.f, 1.f };

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(mVertices.size());
				mVertices.push_back(vertex);
			}
			mIndices.push_back(uniqueVertices[vertex]);
		}
	}
}

void DemoApp::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(mVertices[0]) * mVertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	/*
	Create the "source" buffer
	VK_BUFFER_USAGE_TRANSFER_SRC_BIT: Buffer can be used as source in a memory transfer operation.

	VK_BUFFER_USAGE_TRANSFER_DST_BIT: Buffer can be used as destination in a memory transfer operation.
	*/
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	/*
	This function allows us to access a region of the specified memory resource defined by an offset and size. 
	The offset and size here are 0 and bufferInfo.size, respectively. 
	It is also possible to specify the special value VK_WHOLE_SIZE to map all of the memory. 
	The second to last parameter can be used to specify flags, but there aren't any available yet in the current API.
	It must be set to the value 0. 
	The last parameter specifies the output for the pointer to the mapped memory.
	*/
	void* data;
	vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mVertices.data(), (size_t)bufferSize);
	vkUnmapMemory(mDevice, stagingBufferMemory);

	//Create the "destination" buffer
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mVertexBuffer, mVertexBufferMemory);

	copyBuffer(stagingBuffer, mVertexBuffer, bufferSize);

	vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
	vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
}

void DemoApp::createIndexBuffer()
{
	/*
	There are only two notable differences from a very similarly looking createVertexBuffer().
	The bufferSize is now equal to the number of indices times the size of the index type, 
	either uint16_t or uint32_t. 
	The usage of the indexBuffer should be VK_BUFFER_USAGE_INDEX_BUFFER_BIT instead of VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
	which makes sense. 
	Other than that, the process is exactly the same. 
	We create a staging buffer to copy the contents of indices to and then copy it to the final device local index buffer.
	*/
	VkDeviceSize bufferSize = sizeof(mIndices[0]) * mIndices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mIndices.data(), (size_t)bufferSize);
	vkUnmapMemory(mDevice, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mIndexBuffer, mIndexBufferMemory);

	copyBuffer(stagingBuffer, mIndexBuffer, bufferSize);

	vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
	vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
}

void DemoApp::createUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	
	mUniformBuffers.resize(mSwapChainImages.size());
	mUniformBuffersMemory.resize(mSwapChainImages.size());

	for (size_t i = 0; i < mSwapChainImages.size(); ++i)
	{
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mUniformBuffers[i], mUniformBuffersMemory[i]);
	}
}

void DemoApp::createDescriptorPool()
{
	//We first need to describe which descriptor types our descriptor sets are going to 
	//contain and how many of them, using VkDescriptorPoolSize structures.
	/*
	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(mSwapChainImages.size());
	*/
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(mSwapChainImages.size());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(mSwapChainImages.size());

	//We will allocate one of these descriptors for every frame. 
	//This pool size structure is referenced by the main VkDescriptorPoolCreateInfo:
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();

	//Aside from the maximum number of individual descriptors that are available, 
	//we also need to specify the maximum number of descriptor sets that may be allocated:
	poolInfo.maxSets = static_cast<uint32_t>(mSwapChainImages.size());

	if (vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool!");
}

void DemoApp::createDescriptorSets()
{
	//A descriptor set allocation is described with a VkDescriptorSetAllocateInfo struct.
	//You need to specify the descriptor pool to allocate from, the number of descriptor sets to allocate, 
	//and the descriptor layout to base them on :
	std::vector<VkDescriptorSetLayout> layouts(mSwapChainImages.size(), mDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = mDescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(mSwapChainImages.size());
	allocInfo.pSetLayouts = layouts.data();

	//create one descriptor set for each swap chain image, all with the same layout. 
	//You don't need to explicitly clean up descriptor sets, 
	//because they will be automatically freed when the descriptor pool is destroyed.
	mDescriptorSets.resize(mSwapChainImages.size());
	if (vkAllocateDescriptorSets(mDevice, &allocInfo, mDescriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate descriptor sets!");

	for (size_t i = 0; i < mSwapChainImages.size(); ++i)
	{
		//This structure specifies the buffer and the region within it that contains the data for the descriptor.
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = mUniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = mTextureImageView;
		imageInfo.sampler = mTextureSampler;

		//The configuration of descriptors is updated using the vkUpdateDescriptorSets function, 
		//which takes an array of VkWriteDescriptorSet structs as parameter.
		//Remember that descriptors can be arrays, so we also need to specify
		//the first index in the array that we want to update.
		//We're not using an array, so the index is simply 0.
		/*
		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = mDescriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;

		//We need to specify the type of descriptor again. It's possible to update multiple descriptors at once in an array,
		//starting at index dstArrayElement. The descriptorCount field specifies how many array elements you want to update.
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;

		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pTexelBufferView = nullptr;
		*/

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = mDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = mDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		/*
		The updates are applied using vkUpdateDescriptorSets. 
		It accepts two kinds of arrays as parameters: 
		an array of VkWriteDescriptorSet and an array of VkCopyDescriptorSet. 
		The latter can be used to copy descriptors to each other, as its name implies.
		*/
		vkUpdateDescriptorSets(mDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void DemoApp::prepareInstanceData()
{
	std::vector<InstanceData> instanceData;
	instanceData.resize(INSTANCE_COUNT);

	std::default_random_engine rndGenerator((unsigned)time(nullptr));
	std::uniform_real_distribution<float> uniformDist(0.0, 1.0);
	std::uniform_int_distribution<uint32_t> rndTextureIndex(0, 1);

	//Distribute instanced objects randomly on two different rings
	for (uint32_t i = 0; i < INSTANCE_COUNT / 2; ++i)
	{
		glm::vec2 ring0 { 7.0f, 11.0f };
		glm::vec2 ring1 { 14.0f, 18.0f };

		float rho, theta;

		//Inner ring
		rho = sqrt(((ring0[1] * ring0[1]) - (ring0[0] * ring0[0])) * uniformDist(rndGenerator) + (ring0[0] * ring0[0]));
		theta = 2.0f * 3.14f * uniformDist(rndGenerator);
		instanceData[i].pos = glm::vec3(rho * cos(theta), uniformDist(rndGenerator) * 2.0f, rho * sin(theta));
		instanceData[i].rot = glm::vec3(3.14f * uniformDist(rndGenerator), 3.14f * uniformDist(rndGenerator), 3.14f * uniformDist(rndGenerator));
		instanceData[i].scale = 1.5f + uniformDist(rndGenerator) - uniformDist(rndGenerator);
		instanceData[i].texIndex = rndTextureIndex(rndGenerator);
		instanceData[i].scale *= 0.6f;

		//Outer ring
		rho = sqrt(((ring1[1] * ring1[1]) - (ring1[0] * ring1[0])) * uniformDist(rndGenerator) + (ring1[0] * ring1[0]));
		theta = 2.0f * 3.14f * uniformDist(rndGenerator);
		instanceData[i + INSTANCE_COUNT / 2].pos = glm::vec3(rho * cos(theta), uniformDist(rndGenerator) * 0.5f - 0.25f, rho * sin(theta));
		instanceData[i + INSTANCE_COUNT / 2].rot = glm::vec3(3.14f * uniformDist(rndGenerator), 3.14f * uniformDist(rndGenerator), 3.14f * uniformDist(rndGenerator));
		instanceData[i + INSTANCE_COUNT / 2].scale = 1.5f + uniformDist(rndGenerator) - uniformDist(rndGenerator);
		instanceData[i + INSTANCE_COUNT / 2].texIndex = rndTextureIndex(rndGenerator);
		instanceData[i + INSTANCE_COUNT / 2].scale *= 0.6f;
	}

	mInstanceBuffer.size = instanceData.size() * sizeof(InstanceData);

	//Staging
	//Instanced data is static, copy to device local memory
	//This results in better performance

	struct
	{
		VkDeviceMemory memory;
		VkBuffer buffer;
	} stagingBuffer;
	
	createBuffer2(
		mInstanceBuffer.size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer.buffer,
		stagingBuffer.memory,
		instanceData.data());

	createBuffer(
		mInstanceBuffer.size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mInstanceBuffer.buffer,
		mInstanceBuffer.memory);

	VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copyRegion = {};
	copyRegion.size = mInstanceBuffer.size;
	vkCmdCopyBuffer(
		copyCmd,
		stagingBuffer.buffer,
		mInstanceBuffer.buffer,
		1,
		&copyRegion);

	vkEndCommandBuffer(copyCmd);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &copyCmd;

	vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(mGraphicsQueue);

	vkFreeCommandBuffers(mDevice, mCommandPool, 1, &copyCmd);

	mInstanceBuffer.descriptor.range = mInstanceBuffer.size;
	mInstanceBuffer.descriptor.buffer = mInstanceBuffer.buffer;
	mInstanceBuffer.descriptor.offset = 0;

	vkDestroyBuffer(mDevice, stagingBuffer.buffer, nullptr);
	vkFreeMemory(mDevice, stagingBuffer.memory, nullptr);
}

void DemoApp::updateUniformBuffer(uint32_t currentImage)
{
	//The chrono standard library header exposes functions to do precise timekeeping.
	//We'll use this to make sure that the geometry rotates 90 degrees per second regardless of frame rate.
	static auto startTime = std::chrono::high_resolution_clock::now();
	
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	//We will now define the model, view and projection transformations in the uniform buffer object.
	//The model rotation will be a simple rotation around the Z - axis using the time variable :
	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time / 2 *  glm::radians(90.f), glm::vec3(1.0f, 1.0f, 0.0f));

	//The glm::lookAt function takes the eye position, center position and up axis as parameters.
	ubo.view = glm::lookAt(glm::vec3(2.0f, 10.0f, 42.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f));

	//The other parameters are the aspect ratio, near and far view planes.
	//It is important to use the current swap chain extent to calculate the 
	//aspect ratio to take into account the new width and height of the window after a resize.
	ubo.proj = glm::perspective(glm::radians(45.0f), mSwapChainExtent.width / (float)mSwapChainExtent.height, 0.1f, 100.0f);

	//The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis
	//in the projection matrix. If you don't do this, then the image will be rendered upside down.
	ubo.proj[1][1] *= -1;

	// This is me doing lighting now. Help. - Jimmy
	ubo.uLightPos = glm::vec4(sin(time), 1.5f, -sin(time), 1.f);
	

	ubo.uLightCol = glm::vec4(1.f, .93f, .89f, 1.f);

	//All of the transformations are defined now, so we can copy the data in the uniform buffer object to the current uniform buffer. 
	//This happens in exactly the same way as we did for vertex buffers, except without a staging buffer:
	void* data;
	vkMapMemory(mDevice, mUniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(mDevice, mUniformBuffersMemory[currentImage]);
}

void DemoApp::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};

	//The first field of the struct is size, which specifies the size of the buffer in bytes.
	//Calculating the byte size of the vertex data is straightforward with sizeof.
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;

	//Usage: which indicates for which purposes the data in the buffer is going to be used. 
	//It is possible to specify multiple purposes using a bitwise or.
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(mDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create vertex buffer!");

	/*
	size: The size of the required amount of memory in bytes,
	may differ from bufferInfo.size.

	alignment: The offset in bytes where the buffer begins in the allocated region of memory,
	depends on bufferInfo.usage and bufferInfo.flags.

	memoryTypeBits: Bit field of the memory types that are suitable for the buffer.
	*/
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(mDevice, buffer, &memRequirements);

	/*
	Memory allocation is now as simple as specifying the size and type,
	both of which are derived from the memory requirements of the vertex buffer and the desired property.
	Create a class member to store the handle to the memory and allocate it with vkAllocateMemory.
	*/
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate vertex buffer memory!");

	//If memory allocation was successful, then we can now associate this memory with the buffer using vkBindBufferMemory
	vkBindBufferMemory(mDevice, buffer, bufferMemory, 0);
}

void DemoApp::createBuffer2(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, void* data = nullptr)
{
	VkBufferCreateInfo bufferInfo = {};

	//The first field of the struct is size, which specifies the size of the buffer in bytes.
	//Calculating the byte size of the vertex data is straightforward with sizeof.
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;

	//Usage: which indicates for which purposes the data in the buffer is going to be used. 
	//It is possible to specify multiple purposes using a bitwise or.
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(mDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create vertex buffer!");

	/*
	size: The size of the required amount of memory in bytes,
	may differ from bufferInfo.size.

	alignment: The offset in bytes where the buffer begins in the allocated region of memory,
	depends on bufferInfo.usage and bufferInfo.flags.

	memoryTypeBits: Bit field of the memory types that are suitable for the buffer.
	*/
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(mDevice, buffer, &memRequirements);

	/*
	Memory allocation is now as simple as specifying the size and type,
	both of which are derived from the memory requirements of the vertex buffer and the desired property.
	Create a class member to store the handle to the memory and allocate it with vkAllocateMemory.
	*/
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate vertex buffer memory!");

	// put dat der data in de buffer
	if (data != nullptr)
	{
		void *mapped;
		if (vkMapMemory(mDevice, bufferMemory, 0, size, 0, &mapped) != VK_SUCCESS)
			throw std::runtime_error("Failed to do the mapping of memory!");

		memcpy(mapped, data, size);

		// If we need to manually flush memory.
		if ((properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
		{
			VkMappedMemoryRange mapRange = {};
			mapRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mapRange.memory = bufferMemory;
			mapRange.offset = 0;
			mapRange.size = size;
			vkFlushMappedMemoryRanges(mDevice, 1, &mapRange);
		}

		vkUnmapMemory(mDevice, bufferMemory);
	}

	//If memory allocation was successful, then we can now associate this memory with the buffer using vkBindBufferMemory
	vkBindBufferMemory(mDevice, buffer, bufferMemory, 0);
}

void DemoApp::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	/*
	You may wish to create a separate command pool for these kinds of short-lived buffers, 
	because the implementation may be able to apply memory allocation optimizations.
	You should use the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag during command pool generation in that case.
	*/
	/*
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = mCommandPool;
	allocInfo.commandBufferCount = 1;
	*/
	//Immediately start recording the command buffer.
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	//vkAllocateCommandBuffers(mDevice, &allocInfo, &commandBuffer);

	/*
	The VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT flag that we used for 
	the drawing command buffers is not necessary here, 
	because we're only going to use the command buffer once and 
	wait with returning from the function until the copy operation has finished executing. 
	It's good practice to tell the driver about our intent using VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT.
	*/
	/*
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	*/
	/*
	Contents of buffers are transferred using the vkCmdCopyBuffer command. 
	It takes the source and destination buffers as arguments, and an array of regions to copy.
	The regions are defined in VkBufferCopy structs and consist of a source buffer offset, 
	destination buffer offset and size. 
	It is not possible to specify VK_WHOLE_SIZE here, unlike the vkMapMemory command.
	*/
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
	//This command buffer only contains the copy command, so we can stop recording right after that. 
	/*
	vkEndCommandBuffer(commandBuffer);

	//Execute the command buffer to complete the transfer
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	/*
	Unlike the draw commands, there are no events we need to wait on this time. 
	We just want to execute the transfer on the buffers immediately. 
	There are again two possible ways to wait on this transfer to complete. 
	We could use a fence and wait with vkWaitForFences, 
	or simply wait for the transfer queue to become idle with vkQueueWaitIdle. 
	A fence would allow you to schedule multiple transfers simultaneously and wait for all of them complete, 
	instead of executing one at a time. 
	That may give the driver more opportunities to optimize.
	*/
	/*
	vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(mGraphicsQueue);

	vkFreeCommandBuffers(mDevice, mCommandPool, 1, &commandBuffer);
	*/
}

uint32_t DemoApp::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(mPhysDevice, &memProperties);

	/*
	Memory heaps are distinct memory resources like dedicated VRAM and swap space in RAM for when VRAM runs out. 
	The different types of memory exist within these heaps. 
	Right now we'll only concern ourselves with the type of memory and not the heap it comes from, 
	but you can imagine that this can affect performance.
	*/
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

//https://github.com/SaschaWillems/Vulkan/blob/master/base/vulkanexamplebase.cpp
VkCommandBuffer DemoApp::createCommandBuffer(VkCommandBufferLevel level, bool begin)
{
	VkCommandBuffer cmdBuffer;

	VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
	cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocateInfo.commandPool = mCommandPool;
	cmdBufAllocateInfo.level = level;
	cmdBufAllocateInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(mDevice, &cmdBufAllocateInfo, &cmdBuffer);

	// If requested, also start the new command buffer
	if (begin)
	{
		VkCommandBufferBeginInfo cmdBufferInfo{};
		cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(cmdBuffer, &cmdBufferInfo);
	}

	return cmdBuffer;
}

void DemoApp::createCommandBuffers()
{
	mCommandBuffers.resize(mSwapChainFramebuffers.size());
	
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = mCommandPool;
	/*
		The level parameter specifies if the allocated command buffers are
		primary or secondary command buffers.

		VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, 
		but cannot be called from other command buffers.

		VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly,
		but can be called from primary command buffers.
	*/
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)mCommandBuffers.size();

	if (vkAllocateCommandBuffers(mDevice, &allocInfo, mCommandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate command buffers!");

	for (size_t i = 0; i < mCommandBuffers.size(); ++i)
	{
		/*
			The flags parameter specifies how we're going to use the command buffer. 
			The following values are available:

			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded 
			right after executing it once.

			VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: This is a secondary command buffer
			that will be entirely within a single render pass.

			VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The command buffer can be 
			resubmitted while it is also already pending execution.
		*/

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; //Optional

		if (vkBeginCommandBuffer(mCommandBuffers[i], &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("failed to begin recording command buffer!");
	
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = mRenderPass;
		renderPassInfo.framebuffer = mSwapChainFramebuffers[i];

		/*
		The first parameters are the render pass itself and the attachments to bind. 
		We created a framebuffer for each swap chain image that specifies it as color attachment
		*/
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = mSwapChainExtent;

		/*
		The next two parameters define the size of the render area. 
		The render area defines where shader loads and stores will take place. 
		The pixels outside this region will have undefined values.
		It should match the size of the attachments for best performance.
		*/
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { .0f, .0f, .0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		//VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		/*
		VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the 
		primary command buffer itself and no secondary command buffers will be executed.

		VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass commands will be 
		executed from secondary command buffers.
		*/
		vkCmdBeginRenderPass(mCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		//The second parameter specifies if the pipeline object is a graphics or compute pipeline.
		vkCmdBindPipeline(mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);

		/*
		The first two parameters, besides the command buffer, 
		specify the offset and number of bindings we're going to specify vertex buffers for. 
		The last two parameters specify the array of vertex buffers to bind and the byte offsets to start reading vertex data from.
		*/
		VkBuffer vertexBuffers[] = { mVertexBuffer, mInstanceBuffer.buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(mCommandBuffers[i], 0, 1, &vertexBuffers[0], offsets);
		vkCmdBindVertexBuffers(mCommandBuffers[i], 1, 1, &vertexBuffers[1], offsets);

		//An index buffer is bound with vkCmdBindIndexBuffer which has the index buffer, 
		//a byte offset into it, and the type of index data as parameters
		vkCmdBindIndexBuffer(mCommandBuffers[i], mIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

		//We now need to update the createCommandBuffers function to actually bind the right descriptor set 
		//for each swap chain image to the descriptors in the shader with cmdBindDescriptorSets.
		vkCmdBindDescriptorSets(mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &mDescriptorSets[i], 0, nullptr);

		/*
		A call to this function is very similar to vkCmdDraw. 
		The first two parameters specify the number of indices and the number of instances.
		We're not using instancing, so just specify 1 instance. 
		The number of indices represents the number of vertices that will be passed to the vertex buffer.
		The next parameter specifies an offset into the index buffer, 
		using a value of 1 would cause the graphics card to start reading at the second index. 
		The second to last parameter specifies an offset to add to the indices in the index buffer. 
		The final parameter specifies an offset for instancing, which we're not using.
		*/
		vkCmdDrawIndexed(mCommandBuffers[i], static_cast<uint32_t>(mIndices.size()), INSTANCE_COUNT, 0, 0, 0);

		vkCmdEndRenderPass(mCommandBuffers[i]);
		if (vkEndCommandBuffer(mCommandBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to record command buffer!");
	}
}

void DemoApp::createSyncObjects()
{
	//Semaphores are used for process synchronization in the multi processing environment
	mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Tell it that we've rendered an initial frame already.

	for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(mDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create semaphores!");
}

void DemoApp::recreateSwapChain()
{
	//Special case: window minimization. Pause the output until the window is in the foreground.
	int width = 0, height = 0;
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(mWindow, &width, &height);
		glfwWaitEvents();
	}

	//We should't touch resources that may still be in use, therefore wait.
	vkDeviceWaitIdle(mDevice);

	//Clean any existing swap chain objects.
	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createColorResources();
	createColorResources();
	createDepthResources();
	createFramebuffers();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
}

void DemoApp::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	//Set the framebufferResized flag to true
	DemoApp* app = reinterpret_cast<DemoApp*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

std::vector<char> DemoApp::readFile(const std::string& filename)
{
	/*
	ate: Start reading at the end of the file
	
	binary: Read the file as binary file (avoid text transformations)

	The advantage of starting to read at the end of the file is that
	we can use the read position to determine the size of the file and allocate a buffer:
	*/
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("failed to open file!");

	//tellg = size (basically)
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

VkShaderModule DemoApp::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	//reinterpret_cast is a cast for pointers
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(mDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module!");

	return shaderModule;
}

// Check whether we have layer support for validation layers.
bool DemoApp::checkValidationLayerSupport()
{
	// Basically the same thing as extensions above. Get the amount, then put them in a list.
	uint32_t layerCt;
	vkEnumerateInstanceLayerProperties(&layerCt, nullptr);

	std::vector<VkLayerProperties> layers(layerCt);
	vkEnumerateInstanceLayerProperties(&layerCt, layers.data());

	std::cout << "-------------------------------" << std::endl;
	std::cout << "All validation layers: " << std::endl;
	for (VkLayerProperties prop : layers)
	{
		std::cout << prop.layerName << std::endl;
	}

	std::cout << "--------------------------------";

	// Go through the layers we know we have, and check to see whether those are in Vk or not.
	for (const char* layerName : validationLayers)
	{
		bool found = false;

		for (VkLayerProperties prop : layers)
		{
			if (strcmp(layerName, prop.layerName) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
			return false;
	}

	return true;
}

// Get the required extensions. Returns a list of extensions
// based on whether validation layers are enabled or not. 
std::vector<const char*> DemoApp::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	// glfw can get us the extensions we need, which is handy. 
	// Basically, we're doing this, because Vulkan has no idea what platform we're on.
	// We're telling it GLFW and Windows
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> exts(glfwExtensions, glfwExtensions + glfwExtensionCount);

	// extensions specified by glfw are automatically added, but debug messages need to be conditionally added.
	if (enableValidationLayers)
		exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return exts;
}

// Checks whether the device is suitable or not.
bool DemoApp::isDeviceSuitable(VkPhysicalDevice device)
{
	/*
	VkPhysicalDeviceProperties dProperties;
	vkGetPhysicalDeviceProperties(device, &dProperties);

	VkPhysicalDeviceFeatures dFeatures;
	vkGetPhysicalDeviceFeatures(device, &dFeatures);
	*/

	QueueFamilyIndices indices = findQueueFamilies(device);
	
	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.mFormats.empty() && !swapChainSupport.mPresentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool DemoApp::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	//checks if all the required extensions are there
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	//Check required extensions, and if any are outstanding then we have a problem
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const VkExtensionProperties& extension : availableExtensions)
		requiredExtensions.erase(extension.extensionName);

	// Assuming everything is perfect, this WILL be empty
	return requiredExtensions.empty();
}

SwapChainSupportDetails DemoApp::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	// basic surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mSurface, &details.mCapabilities);

	// format capabilities - first get count
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		// format capabilities - second get data
		details.mFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, details.mFormats.data());
	}

	// present mode capabilities - first get count
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		// present mode capabilities - second get data
		details.mPresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModeCount, details.mPresentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR DemoApp::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	// If there is only one available format, return it
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	// Find the first format that matches the requirements
	for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;
	}

	// If all else fails, find the first format
	return availableFormats[0];
}

VkPresentModeKHR DemoApp::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	/*
	VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application 
	are transferred to the screen right away, which may result in tearing.
	
	VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display 
	takes an image from the front of the queue when the display is refreshed and 
	the program inserts rendered images at the back of the queue. 
	If the queue is full then the program has to wait.
	This is most similar to vertical sync as found in modern games. 
	The moment that the display is refreshed is known as "vertical blank".
	
	VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the previous one 
	if the application is late and the queue was empty at the last vertical blank. 
	Instead of waiting for the next vertical blank, 
	the image is transferred right away when it finally arrives. 
	This may result in visible tearing.

	VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second mode. 
	Instead of blocking the application when the queue is full, 
	the images that are already queued are simply replaced with the newer ones. 
	This mode can be used to implement triple buffering, which allows you to avoid tearing
	with significantly less latency issues than standard vertical sync that uses double buffering.
	*/

	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;


	for (const VkPresentModeKHR& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			bestMode = availablePresentMode;
	}

	return bestMode;
}

VkExtent2D DemoApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	// If not equal to max unsigned number, return current
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
		return capabilities.currentExtent;
	else 
	{
		//Give the window the ability to be resized
		int width, height;
		glfwGetFramebufferSize(mWindow, &width, &height);

		VkExtent2D actualExtent = { 
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

QueueFamilyIndices DemoApp::findQueueFamilies(VkPhysicalDevice device)
{
	// Get the Vk queue families
	QueueFamilyIndices indices;

	uint32_t qFamilyCt = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &qFamilyCt, nullptr);

	std::vector<VkQueueFamilyProperties> qFamilies(qFamilyCt);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &qFamilyCt, qFamilies.data());

	int i = 0;
	for (VkQueueFamilyProperties qFamily : qFamilies)
	{
		// Check to see if the queue family is the graphics bit. Otherwise, we'll ignore it. 
		// Just for simplicity's sake.
		//Step Presentation->Window Surface doesn't say to delete the latter part of the if, but it doesn't have it in there
		if (qFamily.queueCount > 0 && qFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &presentSupport);

		if (qFamily.queueCount > 0 && presentSupport)
			indices.presentFamily = i;

		if (indices.isComplete())
			break;

		++i;
	}

	return indices;
}

// polls for events like input. Glfw will handle some things related to that.
void DemoApp::gameLoop()
{
	while (!glfwWindowShouldClose(mWindow))
	{
		glfwPollEvents();
		drawFrame();
	}

	//Wait for logical devices to finish execution before cleanApp
	vkDeviceWaitIdle(mDevice);
}

void DemoApp::drawFrame()
{
	/*
		The drawFrame function will perform the following operations:

		Acquire an image from the swap chain
		Execute the command buffer with that image as attachment in the framebuffer
		Return the image to the swap chain for presentation
	*/
	//Wait for fences to complete their stuff.
	vkWaitForFences(mDevice, 1, &inFlightFences[mCurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	uint32_t imageIndex;
	/*
	The third parameter specifies a timeout in nanoseconds for an image to become available. 
	Using the maximum value of a 64 bit unsigned integer disables the timeout.
	*/
	VkResult result = vkAcquireNextImageKHR(mDevice, mSwapChain, std::numeric_limits<uint64_t>::max(), 
		mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &imageIndex);

	/*
	VK_ERROR_OUT_OF_DATE_KHR: The swap chain has become incompatible with the surface and
	can no longer be used for rendering. Usually happens after a window resize.

	VK_SUBOPTIMAL_KHR: The swap chain can still be used to successfully present to the surface, 
	but the surface properties are no longer matched exactly.
	*/
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("failed to acquire swap chain image!");

	updateUniformBuffer(imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	/*
	The first three parameters specify which semaphores to wait on before execution begins and in which
	stage(s) of the pipeline to wait. We want to wait with writing colors to the image until it's available,
	so we're specifying the stage of the graphics pipeline that writes to the color attachment. 
	That means that theoretically the implementation can already start executing our vertex shader and 
	such while the image is not yet available. Each entry in the waitStages array corresponds to the 
	semaphore with the same index in pWaitSemaphores.
	*/
	VkSemaphore waitSemaphores[] = { mImageAvailableSemaphores[mCurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	/*
	The next two parameters specify which command buffers to actually submit for execution. 
	We should submit the command buffer that binds the swap chain image we just acquired as color attachment.
	*/
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mCommandBuffers[imageIndex];

	/*
	The signalSemaphoreCount and pSignalSemaphores parameters specify which 
	semaphores to signal once the command buffer(s) have finished execution.
	*/
	VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphores[mCurrentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	//Unlike the semaphores, we manually need to restore the fence to the unsignaled state by resetting it with the vkResetFences call.
	vkResetFences(mDevice, 1, &inFlightFences[mCurrentFrame]);

	if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, inFlightFences[mCurrentFrame]) != VK_SUCCESS)
		throw std::runtime_error("failed to submit draw command buffer!");

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { mSwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr; //Optional

	//vkQueuePresentKHR returns same values as vkAquireNextImageKHR, 
	//so we want to recreateSwapChain if it's out of date or suboptimal or framebuffer was resized
	result = vkQueuePresentKHR(mPresentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
	{
		framebufferResized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("failed to present swap chain image!");

	// Easy way to solve any memory issues. However, it makes it so that the pipeline can only be used
	// for one frame
	//vkQueueWaitIdle(mPresentQueue);

	// Instead of vkQueueWait, we advance the frame for our frame semaphores.
	mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void DemoApp::cleanupSwapChain()
{
	vkDestroyImageView(mDevice, mColorImageView, nullptr);
	vkDestroyImage(mDevice, mColorImage, nullptr);
	vkFreeMemory(mDevice, mColorImageMemory, nullptr);

	vkDestroyImageView(mDevice, mDepthImageView, nullptr);
	vkDestroyImage(mDevice, mDepthImage, nullptr);
	vkFreeMemory(mDevice, depthImageMemory, nullptr);

	//Cleanup code of all objects that are recreated as part of a swap chain refresh
	for (VkFramebuffer framebuffer : mSwapChainFramebuffers)
		vkDestroyFramebuffer(mDevice, framebuffer, nullptr);

	//We free the command buffers instead of destroying them because we can reuse the existsing pool to allocate the new command buffers
	vkFreeCommandBuffers(mDevice, mCommandPool, static_cast<uint32_t>(mCommandBuffers.size()), mCommandBuffers.data());

	vkDestroyPipeline(mDevice, mGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
	vkDestroyRenderPass(mDevice, mRenderPass, nullptr);

	for (VkImageView imageView : mSwapChainImageViews)
		vkDestroyImageView(mDevice, imageView, nullptr);

	vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);

	for (size_t i = 0; i < mSwapChainImages.size(); ++i) 
	{
		vkDestroyBuffer(mDevice, mUniformBuffers[i], nullptr);
		vkFreeMemory(mDevice, mUniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
}

// Destroy the window, and cleanup anything that is necessary.
void DemoApp::cleanApp()
{
	cleanupSwapChain();

	vkDestroySampler(mDevice, mTextureSampler, nullptr);
	vkDestroyImageView(mDevice, mTextureImageView, nullptr);

	vkDestroyImage(mDevice, mTextureImage, nullptr);
	vkFreeMemory(mDevice, mTextureImageMemory, nullptr);

	vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);

	vkDestroyBuffer(mDevice, mIndexBuffer, nullptr);
	vkFreeMemory(mDevice, mIndexBufferMemory, nullptr);

	vkDestroyBuffer(mDevice, mVertexBuffer, nullptr);
	vkFreeMemory(mDevice, mVertexBufferMemory, nullptr);

	vkDestroyBuffer(mDevice, mInstanceBuffer.buffer, nullptr);
	vkFreeMemory(mDevice, mInstanceBuffer.memory, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(mDevice, mRenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(mDevice, mImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(mDevice, inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(mDevice, mCommandPool, nullptr);

	vkDestroyDevice(mDevice, nullptr);

	if (enableValidationLayers)
		destroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);

	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
	vkDestroyInstance(mInstance, NULL);

	glfwDestroyWindow(mWindow);

	glfwTerminate();
}

// Static function for calling error messages.
// Severity is how bad the message is. Can do things like if(severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXIT) to only display a message at certain times.
// message type has a few different enumerations that can be seen here: https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers
// pCallbackdata refers to a struct containing message details such as the message, an array of objects related to the error, so forth.
// and userdata allows us to pass our own data in.
// VKAPI_ATTR and VKAPI_CALL are needed to let Vulkan call this static function.
// And if you want to see what caused a callback, just throw a breakpoint in here and follow the stack.
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT messageType,
													const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}
