#include "buffer.hpp"

std::unordered_map<VkBuffer, std::unique_ptr<struct buffer>> buffersMap;

std::unique_ptr<struct buffer>
create_staging_buffer(struct device *dev, int size) 
{
	VkResult result;
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkLayerDispatchTable table = dev->table;
	VkDevice device = dev->handle;

	VkBufferCreateInfo buffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = static_cast<VkDeviceSize>(size),
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr
	};
	
	result = table.CreateBuffer(device, &buffer_create_info, nullptr, &buffer);

	if (result != VK_SUCCESS) {
		Logger::log("error", "Failed to create staging buffer, res %d", result);
		return NULL;
	}

	VkMemoryAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = static_cast<VkDeviceSize>(size),
		.memoryTypeIndex = dev->memoryIndex
	};

	result = table.AllocateMemory(device, &allocate_info, nullptr, &memory);
	
	// PARCHE MALI-G52: Fallback si la memoria por defecto falla debido a alineaciones estrictas de Unisoc
	if (result != VK_SUCCESS && dev->table.GetBufferMemoryRequirements != nullptr) {
		VkMemoryRequirements memReqs;
		dev->table.GetBufferMemoryRequirements(device, buffer, &memReqs);
		
		VkPhysicalDeviceMemoryProperties memProps;
		instanceDispatch[GetKey(dev->physical)].GetPhysicalDeviceMemoryProperties(dev->physical, &memProps);
		
		uint32_t fallbackIndex = UINT32_MAX;
		for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
			if ((memReqs.memoryTypeBits & (1 << i)) && 
				(memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
				fallbackIndex = i;
				break;
			}
		}
		if (fallbackIndex != UINT32_MAX) {
			allocate_info.memoryTypeIndex = fallbackIndex;
			allocate_info.allocationSize = memReqs.size;
			result = table.AllocateMemory(device, &allocate_info, nullptr, &memory);
		}
	}

	if (result != VK_SUCCESS) {
		Logger::log("error", "Failed to allocate staging buffer memory, res %d", result);
		table.DestroyBuffer(device, buffer, nullptr);
		return NULL;
	}

	result = table.BindBufferMemory(device, buffer, memory, 0);
	if (result != VK_SUCCESS) {
		Logger::log("error", "Failed to bind staging buffer memory, res %d", result);
		table.FreeMemory(device, memory, nullptr);
		table.DestroyBuffer(device, buffer, nullptr);
		return NULL;
	}

	auto staging_buf = std::make_unique<struct buffer>();
	staging_buf->handle = buffer;
	staging_buf->memory = memory;
	staging_buf->offset = 0;
	staging_buf->device = dev;
	staging_buf->alloc = nullptr;

	return staging_buf;
}

struct buffer *
find_buffer(VkBuffer buffer)
{
	auto it = buffersMap.find(buffer);

	if (it == buffersMap.end())
		return nullptr;

	return it->second.get();
}

VK_LAYER_EXPORT VkResult VKAPI_CALL
BCnLayer_CreateBuffer(VkDevice device,
					  const VkBufferCreateInfo *pCreateInfo,
					  const VkAllocationCallbacks *pAllocator,
					  VkBuffer *pBuffer)
{
	VkResult result;
	VkLayerDispatchTable table;
	VkBufferCreateInfo create_info = *pCreateInfo;

	struct device *dev = get_device(device);
	if (!dev)
		return VK_ERROR_INITIALIZATION_FAILED;

	table = dev->table;

	create_info.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; 

	result = table.CreateBuffer(device, &create_info, pAllocator, pBuffer);
	if (result != VK_SUCCESS) {
		Logger::log("error", "Failed to create buffer, res %d", result);
		return result;
	}

	auto buf = std::make_unique<struct buffer>();
	buf->handle = *pBuffer;
	buf->size = pCreateInfo->size;
	buf->device = dev;
	buf->alloc = pAllocator;

	{
		scoped_lock l(global_lock);
		buffersMap[*pBuffer] = std::move(buf);
	}
	
	return VK_SUCCESS;
}

VK_LAYER_EXPORT VkResult VKAPI_CALL
BCnLayer_BindBufferMemory(VkDevice device,
						  VkBuffer buffer,
						  VkDeviceMemory memory,
						  VkDeviceSize memoryOffset)
{
	VkResult result;
	VkLayerDispatchTable table;

	scoped_lock l(global_lock);

	struct device *dev = get_device(device);
	if (!dev)
		return VK_ERROR_INITIALIZATION_FAILED;

	table = dev->table;

	result = table.BindBufferMemory(device, buffer, memory, memoryOffset);
	if (result != VK_SUCCESS) {
		Logger::log("error", "Failed to bind buffer memory, res %d", result);
		return result;
	}

	struct buffer *buf = find_buffer(buffer);
	if (buf) {
		buf->memory = memory;
		buf->offset = memoryOffset;
	}

	return VK_SUCCESS;
}

VK_LAYER_EXPORT void VKAPI_CALL
BCnLayer_DestroyBuffer(VkDevice device,
					   VkBuffer buffer,
					   const VkAllocationCallbacks *pAllocator)
{
	scoped_lock l(global_lock);

	struct device *dev = get_device(device);
	struct buffer *buf = find_buffer(buffer);
	if (!dev || !buf)
		return;

	dev->table.DestroyBuffer(device, buffer, pAllocator);
	buffersMap.erase(buffer);
}
