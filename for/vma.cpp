#define VMA_IMPLEMENTATION
#include "VulkanMemoryAllocator/include/vk_mem_alloc.h"
#include "fr.hpp"
#include <cstdio>

namespace fr {

static void vkAssert(VkResult res)
{
	if (res != VK_SUCCESS) {
		char buf[256];
		std::sprintf(buf, "vkassert failed: %d", res);
		fr::throw_runtime_error(buf);
	}
}

static VmaAllocationCreateInfo conv_aci(const AllocCreateInfo &ai)
{
	VmaAllocationCreateInfo res{};
	res.usage = ai.usage;
	res.flags = ai.flags;
	return res;
}

void Allocator::init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{
	VmaAllocatorCreateInfo ci{
		.physicalDevice = physicalDevice,
		.device = device,
		.instance = instance,	// instance after physical device ????
		.vulkanApiVersion = VK_MAKE_VERSION(1, 0, 0),	// version in last ???????
	};
	vkAssert(vmaCreateAllocator(&ci, &allocator));
}

BufferAllocation Allocator::createBuffer(const VkBufferCreateInfo &bci, const AllocCreateInfo &aci) const
{
	BufferAllocation res;
	auto ai = conv_aci(aci);
	vkAssert(vmaCreateBuffer(allocator, &bci, &ai, &res.buffer, &res.allocation, nullptr));
	return res;
}

BufferAllocation Allocator::createBuffer(const VkBufferCreateInfo &bci, const AllocCreateInfo &aci, void **ppMappedData) const
{
	BufferAllocation res;
	auto ai = conv_aci(aci);
	VmaAllocationInfo alloc_info;
	vkAssert(vmaCreateBuffer(allocator, &bci, &ai, &res.buffer, &res.allocation, &alloc_info));
	*ppMappedData = alloc_info.pMappedData;
	return res;
}

ImageAllocation Allocator::createImage(const VkImageCreateInfo &ici, const AllocCreateInfo &aci) const
{
	ImageAllocation res;
	auto ai = conv_aci(aci);
	vkAssert(vmaCreateImage(allocator, &ici, &ai, &res.image, &res.allocation, nullptr));
	return res;
}

void Allocator::destroy(BufferAllocation &bufferAllocation) const
{
	vmaDestroyBuffer(allocator, bufferAllocation.buffer, bufferAllocation.allocation);
}

void Allocator::destroy(ImageAllocation &imageAllocation) const
{
	vmaDestroyImage(allocator, imageAllocation.image, imageAllocation.allocation);
}

void Allocator::destroy(void)
{
	vmaDestroyAllocator(allocator);
}

void Allocator::flushAllocation(VmaAllocation allocation, VkDeviceSize offset, VkDeviceSize size) const
{
	vkAssert(vmaFlushAllocation(allocator, allocation, offset, size));
}

void Allocator::invalidateAllocation(VmaAllocation allocation, VkDeviceSize offset, VkDeviceSize size) const
{
	vkAssert(vmaInvalidateAllocation(allocator, allocation, offset, size));
}

}