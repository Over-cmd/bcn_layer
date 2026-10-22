#ifndef BCN_LAYER_HPP
#define BCN_LAYER_HPP

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include "vulkan/vk_layer.h"

#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>

#include "logger.hpp"

// Estructura de dispositivo extendida para soportar Mali-G52 y fallbacks
struct device {
    VkDevice handle;
    VkPhysicalDevice physical;
    VkInstance instance; // 👈 Campo de instancia necesario para el despacho y memoria
    VkPhysicalDeviceProperties2 props2;
    VkPhysicalDeviceDriverProperties driverProps;
    VkPhysicalDeviceFeatures features;
    bool compute_bcn_auto;
    VkLayerDispatchTable table;
    uint32_t memoryIndex;
    VkQueue queue;
    const VkAllocationCallbacks *alloc;
    bool use_image_view;
    std::vector<VkDescriptorPool> pools;
    VkDescriptorSetLayout setLayout;
    VkPipelineLayout layout;
    VkPipeline s3tcPipeline;
    VkPipeline bc7Pipeline;
    VkPipeline bc6Pipeline;
    VkPipeline rgtcPipeline;
};

// Tablas globales y mapas de enlace de la capa Vulkan
extern std::unordered_map<void *, VkLayerInstanceDispatchTable> instanceDispatch;
extern std::unordered_map<void *, VkInstance> instanceMap;
extern std::unordered_map<void *, VkPhysicalDeviceFeatures> featuresMap;
extern std::unordered_map<void *, VkPhysicalDeviceProperties2> propertiesMap;
extern std::unordered_map<void *, VkPhysicalDeviceDriverProperties> driverPropertiesMap;
extern std::unordered_map<void *, std::shared_ptr<struct device>> deviceMap;

extern std::mutex global_lock;
extern bool bcn_compute_auto;

// Función auxiliar para obtener la clave genérica de un objeto Vulkan
template <typename T>
inline void *GetKey(T object) {
    return (void *)object;
}

struct device *get_device(VkDevice device);

// Prototipos de funciones exportadas por la capa Vulkan
extern "C" {
    VK_LAYER_EXPORT VkResult VKAPI_CALL BCnLayer_CreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance);
    VK_LAYER_EXPORT void VKAPI_CALL BCnLayer_DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator);
    VK_LAYER_EXPORT VkResult VKAPI_CALL BCnLayer_EnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount, VkPhysicalDevice *pPhysicalDevices);
    VK_LAYER_EXPORT void VKAPI_CALL BCnLayer_GetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures);
    VK_LAYER_EXPORT void VKAPI_CALL BCnLayer_GetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures);
    VK_LAYER_EXPORT VkResult VKAPI_CALL BCnLayer_CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice);
    VK_LAYER_EXPORT void VKAPI_CALL BCnLayer_DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator);
    VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL BCnLayer_GetDeviceProcAddr(VkDevice device, const char *pName);
    VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL BCnLayer_GetInstanceProcAddr(VkInstance instance, const char *pName);
}

#endif
