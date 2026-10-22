#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <vulkan/vulkan.h>
#include "vulkan/vk_layer.h"
#include <unordered_map>
#include <memory>
#include <cstdint>

struct device;

struct buffer {
    VkBuffer handle;
    VkDeviceMemory memory;
    VkDeviceSize size;
    VkDeviceSize offset;
    struct device *device;
    const VkAllocationCallbacks *alloc;
};

extern std::unordered_map<VkBuffer, std::unique_ptr<struct buffer>> buffersMap;

struct buffer *find_buffer(VkBuffer buffer);
std::unique_ptr<struct buffer> create_staging_buffer(struct device *dev, int size);

#endif
