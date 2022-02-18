#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

#ifndef AMD_VULKAN_MEMORY_ALLOCATOR_H	// for the love of god please give me lightweight headers
VK_DEFINE_HANDLE(VmaAllocator);
VK_DEFINE_HANDLE(VmaAllocation);

typedef enum VmaMemoryUsage
{
	VMA_MEMORY_USAGE_UNKNOWN = 0,
	VMA_MEMORY_USAGE_GPU_ONLY = 1,
	VMA_MEMORY_USAGE_CPU_ONLY = 2,
	VMA_MEMORY_USAGE_CPU_TO_GPU = 3,
	VMA_MEMORY_USAGE_GPU_TO_CPU = 4,
	VMA_MEMORY_USAGE_CPU_COPY = 5,
	VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED = 6,

	VMA_MEMORY_USAGE_MAX_ENUM = 0x7FFFFFFF
} VmaMemoryUsage;

typedef enum VmaAllocationCreateFlagBits {
	VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT = 0x00000001,
	VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT = 0x00000002,
	VMA_ALLOCATION_CREATE_MAPPED_BIT = 0x00000004,
	VMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT = 0x00000008,
	VMA_ALLOCATION_CREATE_CAN_MAKE_OTHER_LOST_BIT = 0x00000010,
	VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT = 0x00000020,
	VMA_ALLOCATION_CREATE_UPPER_ADDRESS_BIT = 0x00000040,
	VMA_ALLOCATION_CREATE_DONT_BIND_BIT = 0x00000080,
	VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT = 0x00000100,
	VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT  = 0x00010000,
	VMA_ALLOCATION_CREATE_STRATEGY_WORST_FIT_BIT = 0x00020000,
	VMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT = 0x00040000,
	VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT,
	VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT = VMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT,
	VMA_ALLOCATION_CREATE_STRATEGY_MIN_FRAGMENTATION_BIT = VMA_ALLOCATION_CREATE_STRATEGY_WORST_FIT_BIT,
	VMA_ALLOCATION_CREATE_STRATEGY_MASK =
	VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT |
	VMA_ALLOCATION_CREATE_STRATEGY_WORST_FIT_BIT |
	VMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT,

	VMA_ALLOCATION_CREATE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} VmaAllocationCreateFlagBits;
typedef VkFlags VmaAllocationCreateFlags;
#endif


template <typename T, size_t Size>
static constexpr size_t array_size(T (&)[Size])
{
	return Size;
}

template <typename T>
static constexpr T min(T a, T b)
{
	return a < b ? a : b;
}

template <typename T>
static constexpr T max(T a, T b)
{
	return a > b ? a : b;
}

template <typename T>
static constexpr T clamp(T val, T mn, T mx)
{
	return max(min(val, mx), mn);
}

namespace fr {

size_t file_size(const char *path);
void throw_runtime_error(const char *str);

class exception
{
	char *m_buf;

public:
	exception(const char *str);
	~exception(void);

	const char* what(void) const;
};

struct BufferAllocation
{
	VkBuffer buffer;
	VmaAllocation allocation;
};

struct ImageAllocation
{
	VkImage image;
	VmaAllocation allocation;
};

struct AllocCreateInfo
{
	VmaAllocationCreateFlags flags;
	VmaMemoryUsage usage;
};

struct Allocator
{
	VmaAllocator allocator;

	void init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);

	BufferAllocation createBuffer(const VkBufferCreateInfo &bci, const AllocCreateInfo &aci) const;
	BufferAllocation createBuffer(const VkBufferCreateInfo &bci, const AllocCreateInfo &aci, void **ppMappedData) const;
	ImageAllocation createImage(const VkImageCreateInfo &ici, const AllocCreateInfo &aci) const;
	void destroy(BufferAllocation &bufferAllocation) const;
	void destroy(ImageAllocation &imageAllocation) const;
	void destroy(void);
	void flushAllocation(VmaAllocation allocation, VkDeviceSize offset, VkDeviceSize size) const;
	void invalidateAllocation(VmaAllocation allocation, VkDeviceSize offset, VkDeviceSize size) const;
};

}