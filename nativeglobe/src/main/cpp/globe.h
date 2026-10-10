#pragma once
#define VK_USE_PLATFORM_ANDROID_KHR 1
#include <vulkan/vulkan.h>
#include <android/native_window.h>
#include <android/asset_manager.h>
#include <vector>
#include <array>
#include <atomic>
#include <string>
#include <cstdint>

struct alignas(16) Point4 {float x,y,z,w;};
struct alignas(16) Uniforms {
    Point4 camera;
    Point4 controls;
    Point4 spheres[80];
};
struct Body {float x,y,z,vx,vy,vz,r; bool glass;};
struct Engine {
    ANativeWindow* window=nullptr; AAssetManager* assets=nullptr;
    VkInstance instance=VK_NULL_HANDLE;
    VkSurfaceKHR surface=VK_NULL_HANDLE;
    VkPhysicalDevice physical=VK_NULL_HANDLE;
    VkDevice device=VK_NULL_HANDLE;
    VkQueue queue=VK_NULL_HANDLE;
    uint32_t queueIndex=0;
    uint32_t loaderApi=VK_API_VERSION_1_0,instanceApi=VK_API_VERSION_1_0,gpuApi=VK_API_VERSION_1_0;
    std::string gpuInfo;
    VkSwapchainKHR swap=VK_NULL_HANDLE;
    VkFormat format=VK_FORMAT_UNDEFINED;
    VkExtent2D size{};
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
    std::vector<VkFramebuffer> buffers;
    VkRenderPass renderPass=VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout=VK_NULL_HANDLE;
    VkPipeline pipeline=VK_NULL_HANDLE;
    VkDescriptorSetLayout descLayout=VK_NULL_HANDLE;
    VkDescriptorPool descPool=VK_NULL_HANDLE;
    VkDescriptorSet desc=VK_NULL_HANDLE;
    VkBuffer uniformBuffer=VK_NULL_HANDLE;
    VkDeviceMemory uniformMemory=VK_NULL_HANDLE;
    void* mapped=nullptr;
    VkCommandPool commandPool=VK_NULL_HANDLE;
    VkCommandBuffer command=VK_NULL_HANDLE;
    VkSemaphore acquired=VK_NULL_HANDLE, completed=VK_NULL_HANDLE;
    VkFence fence=VK_NULL_HANDLE;
    std::vector<Body> particles;
    float simClock=0.f;
    uint32_t randomSeed=0x14142u;
    bool init();
    bool initGraphics();
    bool draw();
    void shutdown();
    void setupParticles(int glassCount);
    void physics(float dt);
    void prepareUniform(Uniforms &u,float yaw,float pitch,float blend,int glassCount);
    void setMessage(const char* msg);
};
std::vector<uint32_t> loadSpv(AAssetManager* assets,const char* path);
void vkCheck(VkResult code,const char* operation);
