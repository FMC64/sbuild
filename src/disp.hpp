#pragma once

#include <vulkan/vulkan.h>
#include <glfw/glfw3.h>
#include <portaudio.h>
#include <cstdio>
#include <cstring>
#include "fr.hpp"
#include "renderer.hpp"

class Disp
{
	GLFWwindow *m_window;

	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debug_callback;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_physical_device;
	uint32_t m_queue_family;
	VkSurfaceCapabilitiesKHR m_surface_capabilities;
	VkPresentModeKHR m_present_mode;
	VkDevice m_device;
	VkQueue m_queue;

	fr::Allocator m_allocator;

	VkSwapchainKHR m_swapchain;
	VkRenderPass m_render_pass;
	VkShaderModule m_fwd_v2_module;
	VkShaderModule m_base_module;
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkPipelineLayout m_pipeline_layout;
	VkPipeline m_pipeline;

	VkDescriptorPool m_descriptor_pool;
	VkCommandPool m_command_pool;

	fr::BufferAllocation m_fullscreen_vertex;
	struct Frame {
		VkCommandBuffer cmd_trans;
		VkCommandBuffer cmd;
		VkDescriptorSet desc_set;
		fr::BufferAllocation samples;
		fr::BufferAllocation samples_stg;
		void *samples_stg_ptr;
		VkImage img;
		VkImageView img_view;
		VkFramebuffer framebuffer;
		VkSemaphore samples_ready;
		VkSemaphore img_ready;
		VkSemaphore img_rendered;
		VkFence img_rendered_fence;
		bool ever_submitted = false;
	};

	static inline constexpr uint32_t frame_max = 16;
	Frame m_frames[frame_max];
	uint32_t m_frame_count;

	static void vkAssert(VkResult res)
	{
		if (res != VK_SUCCESS) {
			char buf[256];
			std::sprintf(buf, "vkassert failed: %d", res);
			fr::throw_runtime_error(buf);
		}
	}

	template <typename C, typename T>
	T vkCreate(VkResult (*constr)(const C *ci, const VkAllocationCallbacks*, T *res), const C &ci)
	{
		T res;
		vkAssert(constr(&ci, nullptr, &res));
		return res;
	}

	template <typename C, typename T>
	T vkCreate(VkResult (*constr)(VkDevice dev, const C *ci, const VkAllocationCallbacks*, T *res), const C &ci)
	{
		T res;
		vkAssert(constr(m_device, &ci, nullptr, &res));
		return res;
	}

	template <typename T>
	void vkDestroy(void (*destr)(VkInstance ins, T obj, const VkAllocationCallbacks*), T obj)
	{
		destr(m_instance, obj, nullptr);
	}

	template <typename T>
	void vkDestroy(void (*destr)(VkDevice dev, T obj, const VkAllocationCallbacks*), T obj)
	{
		destr(m_device, obj, nullptr);
	}

	template <typename T>
	T _getProcAddr(const char *name)
	{
		auto res = reinterpret_cast<T>(vkGetInstanceProcAddr(m_instance, name));
		if (res == nullptr)
			fr::throw_runtime_error(name);
		return res;
	}
	#define getProcAddr(name) _getProcAddr<PFN_##name>(#name)

	template <typename T>
	T _getDeviceProcAddr(const char *name)
	{
		auto res = reinterpret_cast<T>(vkGetDeviceProcAddr(m_device, name));
		if (res == nullptr)
			fr::throw_runtime_error(name);
		return res;
	}
	#define getDeviceProcAddr(name) _getDeviceProcAddr<PFN_##name>(#name)

	static VkBool32 debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
		void*                                            pUserData)
	{
		static_cast<void>(messageSeverity);
		static_cast<void>(messageTypes);
		static_cast<void>(pUserData);
		std::printf("%s\n", pCallbackData->pMessage);
		return VK_FALSE;
	}

	VkSemaphore createSemaphore(void)
	{
		VkSemaphoreCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		return vkCreate(vkCreateSemaphore, ci);
	}

	void destroy(VkSemaphore sem)
	{
		vkDestroy(vkDestroySemaphore, sem);
	}

	VkFence createFence(void)
	{
		VkFenceCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		return vkCreate(vkCreateFence, ci);
	}

	void destroy(VkFence sem)
	{
		vkDestroy(vkDestroyFence, sem);
	}

	VkShaderModule createShaderModule(const char *path)
	{
		auto size = fr::file_size(path);
		auto buf = new char[size];
		auto file = std::fopen(path, "rb");
		if (file == nullptr)
			throw file;
		std::fread(buf, 1, size, file);
		std::fclose(file);
		VkShaderModuleCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		ci.codeSize = size;
		ci.pCode = reinterpret_cast<const uint32_t*>(buf);
		auto res = vkCreate(vkCreateShaderModule, ci);
		delete[] buf;
		return res;
	}

	void transferSync(VkBuffer buffer, size_t size, const void *data)
	{
		VkBufferCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		ci.size = size;	// 3 vec2
		ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		fr::AllocCreateInfo ai{};
		ai.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		ai.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		void *mapped;
		auto stag = m_allocator.createBuffer(ci, ai, &mapped);
		std::memcpy(mapped, data, size);
		m_allocator.flushAllocation(stag.allocation, 0, size);	// flush device cache to make visible data

		auto cmd = m_frames[0].cmd;
		{
			VkCommandBufferBeginInfo bi{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
			vkAssert(vkBeginCommandBuffer(cmd, &bi));
		}
		{
			VkBufferCopy region{};
			region.srcOffset = 0;
			region.dstOffset = 0;
			region.size = size;
			vkCmdCopyBuffer(cmd, stag.buffer, buffer, 1, &region);
		}
		vkEndCommandBuffer(cmd);
		VkSubmitInfo si{ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO };
		si.commandBufferCount = 1;
		si.pCommandBuffers = &cmd;
		vkAssert(vkQueueSubmit(m_queue, 1, &si, VK_NULL_HANDLE));
		vkQueueWaitIdle(m_queue);

		m_allocator.destroy(stag);
	}

	size_t fb_size;

public:
	Disp(void)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		auto mon = glfwGetPrimaryMonitor();
		int monw, monh;
		glfwGetMonitorWorkarea(mon, nullptr, nullptr, &monw, &monh);
		m_window = glfwCreateWindow(monw, monh, "sbuild", mon, nullptr);

		{
			VkApplicationInfo ai{ .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO };
			ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			ai.pApplicationName = "sbuild";
			ai.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
			ai.pEngineName = "sbuild";
			ai.engineVersion = VK_MAKE_VERSION(0, 0, 0);
			ai.apiVersion = VK_MAKE_VERSION(1, 2, 0);
			VkInstanceCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
			ci.pApplicationInfo = &ai;
			const char *layers[16];
			size_t layer_count = 0;
			const char *exts[16];
			size_t ext_count = 0;
			bool is_valid = true;
			{
				uint32_t ec;
				auto e = glfwGetRequiredInstanceExtensions(&ec);
				for (size_t i = 0; i < ec; i++)
					exts[ext_count++] = e[i];
			}
			if (is_valid) {
				layers[layer_count++] = "VK_LAYER_KHRONOS_validation";
				exts[ext_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
			}
			ci.enabledLayerCount = layer_count;
			ci.ppEnabledLayerNames = layers;
			ci.enabledExtensionCount = ext_count;
			ci.ppEnabledExtensionNames = exts;
			m_instance = vkCreate(vkCreateInstance, ci);
		}
		{
			VkDebugUtilsMessengerCreateInfoEXT ci{ .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
			ci.messageSeverity =
				//VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			ci.pfnUserCallback = &debugCallback;
			vkAssert(getProcAddr(vkCreateDebugUtilsMessengerEXT)(m_instance, &ci, nullptr, &m_debug_callback));
		}
		vkAssert(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface));
		{
			uint32_t c;
			vkAssert(vkEnumeratePhysicalDevices(m_instance, &c, nullptr));
			VkPhysicalDevice ds[c];
			vkAssert(vkEnumeratePhysicalDevices(m_instance, &c, ds));
			for (uint32_t i = 0; i < c; i++) {
				m_physical_device = ds[i];
				VkPhysicalDeviceProperties props;
				vkGetPhysicalDeviceProperties(m_physical_device, &props);
				if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
					break;
				}
			}
			vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &c, nullptr);
			//VkQueueFamilyProperties qprops[c];
			//vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &c, nullptr);
			{
				bool has_pres = false;
				for (size_t i = 0; i < c; i++) {
					VkBool32 sup;
					vkAssert(getProcAddr(vkGetPhysicalDeviceSurfaceSupportKHR)(m_physical_device, i, m_surface, &sup));
					if (sup == VK_TRUE) {
						has_pres = true;
						m_queue_family = i;
						break;
					}
				}
				if (!has_pres)
					fr::throw_runtime_error("can't find any presentation queue");
			}
			{
				vkAssert(getProcAddr(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)(m_physical_device, m_surface, &m_surface_capabilities));
			}
			{
				vkAssert(getProcAddr(vkGetPhysicalDeviceSurfacePresentModesKHR)(m_physical_device, m_surface, &c, nullptr));
				VkPresentModeKHR pms[c];
				vkAssert(getProcAddr(vkGetPhysicalDeviceSurfacePresentModesKHR)(m_physical_device, m_surface, &c, pms));
				m_present_mode = VK_PRESENT_MODE_FIFO_KHR;
				auto w = VK_PRESENT_MODE_MAILBOX_KHR;
				for (uint32_t i = 0; i < c; i++)
					if (pms[i] == w) {
						m_present_mode = w;
						break;
					}
				std::printf("present mode: %d\n", m_present_mode);
			}
		}
		fb_size = sizeof(uint32_t) + m_surface_capabilities.currentExtent.width * m_surface_capabilities.currentExtent.height * sizeof(uint32_t);
		{
			VkDeviceCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
			VkDeviceQueueCreateInfo qci { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			qci.queueFamilyIndex = m_queue_family;
			float qp = 1.0;
			qci.queueCount = 1;
			qci.pQueuePriorities = &qp;
			ci.queueCreateInfoCount = 1;
			ci.pQueueCreateInfos = &qci;
			const char *exts[] = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};
			ci.enabledExtensionCount = array_size(exts);
			ci.ppEnabledExtensionNames = exts;
			VkPhysicalDeviceVulkan12Features features { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
			features.uniformAndStorageBuffer8BitAccess = VK_TRUE;
			ci.pNext = &features;
			vkAssert(vkCreateDevice(m_physical_device, &ci, nullptr, &m_device));
		}
		vkGetDeviceQueue(m_device, m_queue_family, 0, &m_queue);
		m_allocator.init(m_instance, m_physical_device, m_device);
		m_frame_count = clamp(static_cast<uint32_t>(2), m_surface_capabilities.minImageCount, m_surface_capabilities.maxImageCount);	// double buffering
		{
			VkSwapchainCreateInfoKHR ci{ .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
			ci.surface = m_surface;
			ci.minImageCount = m_frame_count;
			ci.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
			ci.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
			ci.imageExtent = m_surface_capabilities.currentExtent;
			ci.imageArrayLayers = 1;
			ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			ci.preTransform = m_surface_capabilities.currentTransform;
			ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			ci.presentMode = m_present_mode;
			ci.clipped = VK_TRUE;
			m_swapchain = vkCreate(getDeviceProcAddr(vkCreateSwapchainKHR), ci);
			uint32_t c;
			vkAssert(getDeviceProcAddr(vkGetSwapchainImagesKHR)(m_device, m_swapchain, &c, nullptr));
			m_frame_count = min(c, m_frame_count);
			VkImage is[c];
			vkAssert(getDeviceProcAddr(vkGetSwapchainImagesKHR)(m_device, m_swapchain, &c, is));
			for (size_t i = 0; i < m_frame_count; i++) {
				m_frames[i].img = is[i];
				m_frames[i].samples_ready = createSemaphore();
				m_frames[i].img_ready = createSemaphore();
				m_frames[i].img_rendered = createSemaphore();
				m_frames[i].img_rendered_fence = createFence();
			}
		}
		{
			VkRenderPassCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };

			VkAttachmentDescription attach{};
			attach.format = VK_FORMAT_B8G8R8A8_SRGB;
			attach.samples = VK_SAMPLE_COUNT_1_BIT;
			attach.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attach.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			ci.attachmentCount = 1;
			ci.pAttachments = &attach;

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			VkAttachmentReference color;
			color.attachment = 0;
			color.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &color;

			ci.subpassCount = 1;
			ci.pSubpasses = &subpass;

			m_render_pass = vkCreate(vkCreateRenderPass, ci);

			for (size_t i = 0; i < m_frame_count; i++) {
				auto &f = m_frames[i];
				{
					VkImageViewCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
					ci.image = f.img;
					ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
					ci.format = VK_FORMAT_B8G8R8A8_SRGB;
					ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
					ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
					ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
					ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
					ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					ci.subresourceRange.baseMipLevel = 0;
					ci.subresourceRange.levelCount = 1;
					ci.subresourceRange.baseArrayLayer = 0;
					ci.subresourceRange.layerCount = 1;
					f.img_view = vkCreate(vkCreateImageView, ci);
				}
				{
					VkFramebufferCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
					ci.renderPass = m_render_pass;
					ci.attachmentCount = 1;
					ci.pAttachments = &f.img_view;
					ci.width = m_surface_capabilities.currentExtent.width;
					ci.height = m_surface_capabilities.currentExtent.height;
					ci.layers = 1;
					f.framebuffer = vkCreate(vkCreateFramebuffer, ci);
				}
			}
		}
		{
			m_fwd_v2_module = createShaderModule("sha/fwd_v2.vert.spv");
			m_base_module = createShaderModule("sha/base.frag.spv");
			{
				VkDescriptorSetLayoutCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
				VkDescriptorSetLayoutBinding bindings[] = {
					{
						.binding = 0,
						.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						.descriptorCount = 1,
						.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
					}
				};
				ci.bindingCount = array_size(bindings);
				ci.pBindings = bindings;
				m_descriptor_set_layout = vkCreate(vkCreateDescriptorSetLayout, ci);
			}
			{
				VkPipelineLayoutCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
					.setLayoutCount = 1,
					.pSetLayouts = &m_descriptor_set_layout
				};
				m_pipeline_layout = vkCreate(vkCreatePipelineLayout, ci);
			}
			{
				VkPipelineShaderStageCreateInfo stages[] = {
					{
						.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
						.stage = VK_SHADER_STAGE_VERTEX_BIT,
						.module = m_fwd_v2_module,
						.pName = "main"
					},
					{
						.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
						.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
						.module = m_base_module,
						.pName = "main"
					}
				};
				VkVertexInputBindingDescription vertex_bindings[] = {
					{
						.binding = 0,
						.stride = sizeof(float) * 2,
						.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
					}
				};
				VkVertexInputAttributeDescription vertex_attributes[] = {
					{
						.location = 0,
						.binding = 0,
						.format = VK_FORMAT_R32G32_SFLOAT,	// vec2 in_p
						.offset = 0
					}
				};
				VkPipelineVertexInputStateCreateInfo vertex{ .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
					.vertexBindingDescriptionCount = array_size(vertex_bindings),
					.pVertexBindingDescriptions = vertex_bindings,
					.vertexAttributeDescriptionCount = array_size(vertex_attributes),
					.pVertexAttributeDescriptions = vertex_attributes
				};
				VkPipelineInputAssemblyStateCreateInfo input{ .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
					.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
				};
				VkViewport viewports[] = {
					{
						.x = 0.0f, .y = 0.0f,
						.width = static_cast<float>(m_surface_capabilities.currentExtent.width),
						.height = static_cast<float>(m_surface_capabilities.currentExtent.height),
						.minDepth = 0.0f, .maxDepth = 1.0f
					}
				};
				VkRect2D scissors[] = {
					{
						.offset = {
							.x = 0, .y = 0
						},
						.extent = {
							.width = m_surface_capabilities.currentExtent.width, .height = m_surface_capabilities.currentExtent.height
						}
					}
				};
				VkPipelineViewportStateCreateInfo viewport{ .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
					.viewportCount = array_size(viewports),
					.pViewports = viewports,
					.scissorCount = array_size(scissors),
					.pScissors = scissors,
				};
				VkPipelineRasterizationStateCreateInfo raster{ .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
					.polygonMode = VK_POLYGON_MODE_FILL,
					.cullMode = VK_CULL_MODE_NONE,
					.frontFace = VK_FRONT_FACE_CLOCKWISE,
					.lineWidth = 1.0f
				};
				VkPipelineMultisampleStateCreateInfo ms{ .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
					.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
				};
				VkPipelineColorBlendAttachmentState attachments[] = {
					{
						.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |  VK_COLOR_COMPONENT_A_BIT
					}
				};
				VkPipelineColorBlendStateCreateInfo color{ .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
					.attachmentCount = array_size(attachments),
					.pAttachments = attachments
				};
				VkGraphicsPipelineCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
					.stageCount = array_size(stages),
					.pStages = stages,
					.pVertexInputState = &vertex,
					.pInputAssemblyState = &input,
					.pViewportState = &viewport,
					.pRasterizationState = &raster,
					.pMultisampleState = &ms,
					.pColorBlendState = &color,
					.layout = m_pipeline_layout,
					.renderPass = m_render_pass,
					.subpass = 0
				};
				vkAssert(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &ci, nullptr, &m_pipeline));
			}
		}
		{
			VkDescriptorPoolCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
			ci.maxSets = m_frame_count;
			VkDescriptorPoolSize pool_size{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = m_frame_count * 1
			};
			ci.poolSizeCount = 1;
			ci.pPoolSizes = &pool_size;
			m_descriptor_pool = vkCreate(vkCreateDescriptorPool, ci);
		}
		{
			VkDescriptorSetAllocateInfo ai{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
			ai.descriptorPool = m_descriptor_pool;
			ai.descriptorSetCount = m_frame_count;
			VkDescriptorSetLayout layouts[m_frame_count];
			for (size_t i = 0; i < m_frame_count; i++)
				layouts[i] = m_descriptor_set_layout;
			ai.pSetLayouts =  layouts;
			VkDescriptorSet sets[m_frame_count];
			vkAssert(vkAllocateDescriptorSets(m_device, &ai, sets));
			for (size_t i = 0; i < m_frame_count; i++)
				m_frames[i].desc_set = sets[i];
		}
		{
			VkWriteDescriptorSet ws[m_frame_count];
			VkDescriptorBufferInfo bis[m_frame_count];
			for (size_t i = 0; i < m_frame_count; i++) {
				VkBufferCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
				ci.size = fb_size;
				ci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
				fr::AllocCreateInfo ai{};
				ai.usage = VMA_MEMORY_USAGE_GPU_ONLY;
				auto buf = m_allocator.createBuffer(ci, ai);
				m_frames[i].samples = buf;
				{
					VkBufferCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
					ci.size = fb_size;
					ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
					fr::AllocCreateInfo ai{};
					ai.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
					ai.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
					m_frames[i].samples_stg = m_allocator.createBuffer(ci, ai, &m_frames[i].samples_stg_ptr);
				}
				auto &w = ws[i];
				w = VkWriteDescriptorSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
				w.dstSet = m_frames[i].desc_set;
				w.dstBinding = 0;
				w.dstArrayElement = 0;
				w.descriptorCount = 1;
				w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				auto &bi = bis[i];
				bi = VkDescriptorBufferInfo {
					.buffer = buf.buffer,
					.offset = 0,
					.range = ci.size
				};
				w.pBufferInfo = &bi;
			}
			vkUpdateDescriptorSets(m_device, m_frame_count, ws, 0, nullptr);
		}
		{
			VkCommandPoolCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
			ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			ci.queueFamilyIndex = m_queue_family;
			m_command_pool = vkCreate(vkCreateCommandPool, ci);
		}
		{
			VkCommandBufferAllocateInfo ai{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
			ai.commandPool = m_command_pool;
			ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			ai.commandBufferCount = m_frame_count * 2;
			VkCommandBuffer cmds[m_frame_count * 2];
			vkAssert(vkAllocateCommandBuffers(m_device, &ai, cmds));
			for (size_t i = 0; i < m_frame_count; i++) {
				m_frames[i].cmd_trans = cmds[i * 2];
				m_frames[i].cmd = cmds[i * 2 + 1];
			}
		}
		{
			float data[] = {
				-1.0, -1.0,
				3.0, -1.0,
				-1.0, 3.0
			};
			VkBufferCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			ci.size = sizeof(data);	// 3 vec2
			ci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			fr::AllocCreateInfo ai{};
			ai.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			m_fullscreen_vertex = m_allocator.createBuffer(ci, ai);
			transferSync(m_fullscreen_vertex.buffer, sizeof(data), data);
		}
	}
	~Disp(void)
	{
		for (size_t i = 0; i < m_frame_count; i++) {
			auto &f = m_frames[i];
			m_allocator.destroy(f.samples_stg);
			m_allocator.destroy(f.samples);
			destroy(f.img_rendered_fence);
			destroy(f.img_rendered);
			destroy(f.img_ready);
			destroy(f.samples_ready);
			vkDestroy(vkDestroyFramebuffer, f.framebuffer);
			vkDestroy(vkDestroyImageView, f.img_view);
		}

		m_allocator.destroy(m_fullscreen_vertex);
		vkDestroy(vkDestroyCommandPool, m_command_pool);
		vkDestroy(vkDestroyDescriptorPool, m_descriptor_pool);

		vkDestroy(vkDestroyPipeline, m_pipeline);
		vkDestroy(vkDestroyPipelineLayout, m_pipeline_layout);
		vkDestroy(vkDestroyDescriptorSetLayout, m_descriptor_set_layout);
		vkDestroy(vkDestroyShaderModule, m_base_module);
		vkDestroy(vkDestroyShaderModule, m_fwd_v2_module);

		vkDestroy(vkDestroyRenderPass, m_render_pass);
		vkDestroy(vkDestroySwapchainKHR, m_swapchain);

		m_allocator.destroy();

		vkDestroyDevice(m_device, nullptr);
		vkDestroy(getProcAddr(vkDestroyDebugUtilsMessengerEXT), m_debug_callback);
		vkDestroy(vkDestroySurfaceKHR, m_surface);
		vkDestroyInstance(m_instance, nullptr);
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	static void paAssert(PaError err)
	{
		if (err != paNoError) {
			char buf[1024];
			std::sprintf(buf, "pa err: %s", Pa_GetErrorText(err));
			fr::throw_runtime_error(buf);
		}
	}

	void run(void)
	{
		uint32_t w = m_surface_capabilities.currentExtent.width;
		uint32_t h = m_surface_capabilities.currentExtent.height;
		uint8_t *fb = new uint8_t[sizeof(uint32_t) * w * h];
		*reinterpret_cast<uint32_t*>(fb) = h;
		auto fb_data = fb + sizeof(uint32_t);
		Renderer renderer(reinterpret_cast<uint32_t*>(fb_data), w, h);

		auto acquireNextImage = getDeviceProcAddr(vkAcquireNextImageKHR);
		size_t frame_ndx = 0;
		while (true) {
			glfwPollEvents();
			if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				break;
			if (glfwWindowShouldClose(m_window))
				break;

			uint32_t img_ndx;
			auto &img_ready = m_frames[frame_ndx].img_ready;
			vkAssert(acquireNextImage(m_device, m_swapchain, ~0ULL, img_ready, VK_NULL_HANDLE, &img_ndx));

			auto &frame = m_frames[img_ndx];
			if (frame.ever_submitted) {
				vkAssert(vkWaitForFences(m_device, 1, &frame.img_rendered_fence, VK_TRUE, ~0ULL));
				vkResetFences(m_device, 1, &frame.img_rendered_fence);
			}

			{
				renderer.render();
				std::memcpy(frame.samples_stg_ptr, fb, fb_size);
				m_allocator.flushAllocation(frame.samples_stg.allocation, 0, fb_size);	// flush device cache to make visible samples
			}
			{
				{
					VkCommandBufferBeginInfo bi{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
					bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
					vkAssert(vkBeginCommandBuffer(frame.cmd_trans, &bi));
					VkBufferCopy region {
						.srcOffset = 0,
						.dstOffset = 0,
						.size = fb_size
					};
					vkCmdCopyBuffer(frame.cmd_trans, frame.samples_stg.buffer, frame.samples.buffer, 1, &region);
					vkAssert(vkEndCommandBuffer(frame.cmd_trans));
				}
			}

			{
				VkCommandBufferBeginInfo bi{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
				bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				vkAssert(vkBeginCommandBuffer(frame.cmd, &bi));
			}
			{
				VkRenderPassBeginInfo rbi{ .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
				rbi.renderPass = m_render_pass;
				rbi.framebuffer = frame.framebuffer;
				rbi.renderArea.offset = VkOffset2D{};
				rbi.renderArea.extent = m_surface_capabilities.currentExtent;
				vkCmdBeginRenderPass(frame.cmd, &rbi, VK_SUBPASS_CONTENTS_INLINE);
				{
					vkCmdBindDescriptorSets(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &frame.desc_set, 0, nullptr);
					vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
					{
						VkDeviceSize o = 0;
						vkCmdBindVertexBuffers(frame.cmd, 0, 1, &m_fullscreen_vertex.buffer, &o);
					}
					vkCmdDraw(frame.cmd, 3, 1, 0, 0);
				}
				vkCmdEndRenderPass(frame.cmd);
			}
			vkAssert(vkEndCommandBuffer(frame.cmd));
			{
				VkSemaphore wait_render[] = {
					frame.samples_ready,
					img_ready
				};
				VkPipelineStageFlags wait_render_stages[] = {
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				};
				VkSubmitInfo sis[] = {
					{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
						.waitSemaphoreCount = 0,
						.pWaitSemaphores = nullptr,
						.pWaitDstStageMask = nullptr,
						.commandBufferCount = 1,
						.pCommandBuffers = &frame.cmd_trans,
						.signalSemaphoreCount = 1,
						.pSignalSemaphores = &frame.samples_ready,
					},
					{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
						.waitSemaphoreCount = array_size(wait_render),
						.pWaitSemaphores = wait_render,
						.pWaitDstStageMask = wait_render_stages,
						.commandBufferCount = 1,
						.pCommandBuffers = &frame.cmd,
						.signalSemaphoreCount = 1,
						.pSignalSemaphores = &frame.img_rendered
					}
				};
				vkAssert(vkQueueSubmit(m_queue, array_size(sis), sis, frame.img_rendered_fence));
			}
			frame.ever_submitted = true;
			{
				VkPresentInfoKHR pi{ .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
				pi.waitSemaphoreCount = 1;
				pi.pWaitSemaphores = &frame.img_rendered;
				pi.swapchainCount = 1;
				pi.pSwapchains = &m_swapchain;
				pi.pImageIndices = &img_ndx;
				vkAssert(vkQueuePresentKHR(m_queue, &pi));
			}

			frame_ndx = (frame_ndx + 1) % m_frame_count;
		}
		delete[] fb;
		vkAssert(vkDeviceWaitIdle(m_device));
	}
};