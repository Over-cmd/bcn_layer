#include "fence.hpp"

std::unordered_map<VkFence, std::shared_ptr<struct fence>> fencesMap;

struct fence *
get_fence(VkFence fence) 
{
	auto it = fencesMap.find(fence);

	if (it == fencesMap.end())
		return nullptr;

	return it->second.get();
}

VK_LAYER_EXPORT VkResult VKAPI_CALL
BCnLayer_CreateFence(VkDevice device,
					 const VkFenceCreateInfo *pCreateInfo,
					 const VkAllocationCallbacks *pAllocator,
					 VkFence *pFence)
{
	VkResult result;

	struct device *dev = get_device(device);
	if (!dev)
		return VK_ERROR_INITIALIZATION_FAILED;

	result = dev->table.CreateFence(device, pCreateInfo, pAllocator, pFence);
	if (result != VK_SUCCESS)
		return result;

	auto fence = std::make_shared<struct fence>();
	fence->handle = *pFence;
	fence->device = dev;
	dev->alloc = pAllocator;

	{
		scoped_lock l(global_lock);
		fencesMap[*pFence] = fence;
	}

	return VK_SUCCESS;
	
}

VK_LAYER_EXPORT VkResult VKAPI_CALL
BCnLayer_WaitForFences(VkDevice device,
					   uint32_t fenceCount,
					   const VkFence *pFences,
					   VkBool32 waitAll,
					   uint64_t timeout)
{
	VkResult result;

	struct device *dev = get_device(device);
	if (!dev)
		return VK_ERROR_INITIALIZATION_FAILED;

	result = dev->table.WaitForFences(device, fenceCount, pFences, waitAll, timeout);
	if (result != VK_SUCCESS || dev->use_image_view)
		return result;

	scoped_lock l(global_lock);
    
	for (uint32_t i = 0; i < fenceCount; i++) {
		struct fence *fence = get_fence(pFences[i]);
		if (!fence)
			continue;

		// PARCHE MALI-G52: Si waitAll es falso, solo liberamos si este fence específico ha completado su ejecución
		if (!waitAll && fenceCount > 1 && dev->table.GetFenceStatus(device, pFences[i]) != VK_SUCCESS)
			continue;

		for (auto it = fence->staging_buffers.begin(); it != fence->staging_buffers.end();) {
			if ((*it)->handle != VK_NULL_HANDLE) {
				dev->table.DestroyBuffer(device, (*it)->handle, (*it)->alloc);
			}
			if ((*it)->memory != VK_NULL_HANDLE) {
				dev->table.FreeMemory(device, (*it)->memory, (*it)->alloc);
			}
			it = fence->staging_buffers.erase(it);
		}
	}
    
	return VK_SUCCESS;
}

VK_LAYER_EXPORT void VKAPI_CALL
BCnLayer_DestroyFence(VkDevice device,
					  VkFence fence,
					  const VkAllocationCallbacks *pAllocator)
{
	scoped_lock l(global_lock);

	struct device *dev = get_device(device);
	if (!dev)
		return;

	if (fence != VK_NULL_HANDLE) {
		struct fence *f = get_fence(fence);
		// PARCHE MALI-G52: Purga preventiva para evitar fugas si el fence es destruido antes de un Wait explícito
		if (f && !dev->use_image_view) {
			for (auto& buf : f->staging_buffers) {
				if (buf->handle != VK_NULL_HANDLE) dev->table.DestroyBuffer(device, buf->handle, buf->alloc);
				if (buf->memory != VK_NULL_HANDLE) dev->table.FreeMemory(device, buf->memory, buf->alloc);
			}
			f->staging_buffers.clear();
		}
		dev->table.DestroyFence(device, fence, pAllocator);
	}
		
	fencesMap.erase(fence);
}
		dev->table.DestroyFence(device, fence, pAllocator);
		
	fencesMap.erase(fence);
}
