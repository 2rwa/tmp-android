#include "globe.h"
#include <android/log.h>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <cstdio>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"LiquidGlass",__VA_ARGS__)
void vkCheck(VkResult value,const char* name){if(value!=VK_SUCCESS){
    LOGE("%s failed: %d",name,value);throw std::runtime_error(name);
}}
std::vector<uint32_t> loadSpv(AAssetManager* assets,const char* path){
    AAsset* asset=AAssetManager_open(assets,path,AASSET_MODE_BUFFER);
    if(!asset)throw std::runtime_error(std::string("Shader asset missing: ")+path);
    size_t len=size_t(AAsset_getLength(asset));
    if(!len || len%4){AAsset_close(asset);throw std::runtime_error("Shader length invalid");}
    std::vector<uint32_t> result(len/4);
    if(AAsset_read(asset,result.data(),len)!=int(len)){AAsset_close(asset);throw std::runtime_error("Shader read failed");}
    AAsset_close(asset);return result;
}
static uint32_t findMemory(VkPhysicalDevice physical,uint32_t bits,VkMemoryPropertyFlags flags){
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physical,&properties);
    for(uint32_t i=0;i<properties.memoryTypeCount;i++){
        if((bits&(1u<<i))&&(properties.memoryTypes[i].propertyFlags&flags)==flags)return i;
    }
    throw std::runtime_error("Host coherent uniform memory unsupported");
}
bool Engine::init(){
    try {
        VkApplicationInfo info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        info.pApplicationName="LiquidGlass";info.apiVersion=VK_API_VERSION_1_0;
        const char *instanceExtensions[]={VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
        VkInstanceCreateInfo ic{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        ic.pApplicationInfo=&info;ic.enabledExtensionCount=2;ic.ppEnabledExtensionNames=instanceExtensions;
        vkCheck(vkCreateInstance(&ic,nullptr,&instance),"vkCreateInstance");
        VkAndroidSurfaceCreateInfoKHR surfaceInfo{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
        surfaceInfo.window=window;
        vkCheck(vkCreateAndroidSurfaceKHR(instance,&surfaceInfo,nullptr,&surface),"vkCreateAndroidSurfaceKHR");
        uint32_t n=0;vkCheck(vkEnumeratePhysicalDevices(instance,&n,nullptr),"vkEnumeratePhysicalDevices");
        if(!n)throw std::runtime_error("No Vulkan GPU");
        std::vector<VkPhysicalDevice> devices(n);
        vkCheck(vkEnumeratePhysicalDevices(instance,&n,devices.data()),"vkEnumeratePhysicalDevices");
        for(VkPhysicalDevice candidate:devices){
            uint32_t qn=0;vkGetPhysicalDeviceQueueFamilyProperties(candidate,&qn,nullptr);
            std::vector<VkQueueFamilyProperties> queues(qn);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate,&qn,queues.data());
            for(uint32_t i=0;i<qn;i++){
                VkBool32 supportsPresent=VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(candidate,i,surface,&supportsPresent);
                if((queues[i].queueFlags&VK_QUEUE_GRAPHICS_BIT)&&supportsPresent){
                    uint32_t en=0;vkEnumerateDeviceExtensionProperties(candidate,nullptr,&en,nullptr);
                    std::vector<VkExtensionProperties> exts(en);
                    vkEnumerateDeviceExtensionProperties(candidate,nullptr,&en,exts.data());
                    bool swapExt=false;
                    for(const auto& ext:exts)if(strcmp(ext.extensionName,VK_KHR_SWAPCHAIN_EXTENSION_NAME)==0)swapExt=true;
                    if(swapExt){physical=candidate;queueIndex=i;break;}
                }
            }
            if(physical)break;
        }
        if(!physical)throw std::runtime_error("No Vulkan GPU with presentation support");
        float priority=1.f;
        VkDeviceQueueCreateInfo dq{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        dq.queueFamilyIndex=queueIndex;dq.queueCount=1;dq.pQueuePriorities=&priority;
        const char* extensions[]={VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        dc.queueCreateInfoCount=1;dc.pQueueCreateInfos=&dq;
        dc.enabledExtensionCount=1;dc.ppEnabledExtensionNames=extensions;
        vkCheck(vkCreateDevice(physical,&dc,nullptr,&device),"vkCreateDevice");
        vkGetDeviceQueue(device,queueIndex,0,&queue);
        VkSurfaceCapabilitiesKHR caps{};
        vkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical,surface,&caps),"surface caps");
        uint32_t formatCount=0;
        vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(physical,surface,&formatCount,nullptr),"surface formats");
        if(!formatCount)throw std::runtime_error("No surface formats");
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(physical,surface,&formatCount,formats.data()),"surface formats");
        VkSurfaceFormatKHR chosen=formats[0];
        for(const auto& f:formats)if(f.format==VK_FORMAT_R8G8B8A8_UNORM || f.format==VK_FORMAT_B8G8R8A8_UNORM){chosen=f;break;}
        if(chosen.format==VK_FORMAT_UNDEFINED)chosen.format=VK_FORMAT_R8G8B8A8_UNORM;
        format=chosen.format;
        if(caps.currentExtent.width!=UINT32_MAX)size=caps.currentExtent;
        else {
            size.width=std::max(caps.minImageExtent.width,std::min(caps.maxImageExtent.width,uint32_t(ANativeWindow_getWidth(window))));
            size.height=std::max(caps.minImageExtent.height,std::min(caps.maxImageExtent.height,uint32_t(ANativeWindow_getHeight(window))));
        }
        if(!size.width||!size.height)throw std::runtime_error("Surface has zero extent");
        uint32_t imagesWanted=std::max(2u,caps.minImageCount);
        if(caps.maxImageCount)imagesWanted=std::min(imagesWanted,caps.maxImageCount);
        VkSwapchainCreateInfoKHR sc{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        sc.surface=surface;sc.minImageCount=imagesWanted;sc.imageFormat=format;sc.imageColorSpace=chosen.colorSpace;
        sc.imageExtent=size;sc.imageArrayLayers=1;sc.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        sc.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;sc.preTransform=caps.currentTransform;
        sc.compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        for(auto bit:{VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                      VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR}){
            if(caps.supportedCompositeAlpha&bit){sc.compositeAlpha=bit;break;}
        }
        sc.presentMode=VK_PRESENT_MODE_FIFO_KHR;sc.clipped=VK_TRUE;
        vkCheck(vkCreateSwapchainKHR(device,&sc,nullptr,&swap),"vkCreateSwapchainKHR");
        uint32_t imageCount=0;vkCheck(vkGetSwapchainImagesKHR(device,swap,&imageCount,nullptr),"get swap images");
        images.resize(imageCount);
        vkCheck(vkGetSwapchainImagesKHR(device,swap,&imageCount,images.data()),"get swap images");
        for(auto image:images){
            VkImageViewCreateInfo vc{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            vc.image=image;vc.viewType=VK_IMAGE_VIEW_TYPE_2D;vc.format=format;
            vc.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
            vc.subresourceRange.levelCount=1;vc.subresourceRange.layerCount=1;
            VkImageView view=VK_NULL_HANDLE;
            vkCheck(vkCreateImageView(device,&vc,nullptr,&view),"vkCreateImageView");views.push_back(view);
        }
        VkBufferCreateInfo bc{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bc.size=sizeof(Uniforms);bc.usage=VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bc.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        vkCheck(vkCreateBuffer(device,&bc,nullptr,&uniformBuffer),"vkCreateBuffer");
        VkMemoryRequirements req{};vkGetBufferMemoryRequirements(device,uniformBuffer,&req);
        VkMemoryAllocateInfo al{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        al.allocationSize=req.size;
        al.memoryTypeIndex=findMemory(physical,req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkCheck(vkAllocateMemory(device,&al,nullptr,&uniformMemory),"vkAllocateMemory");
        vkCheck(vkBindBufferMemory(device,uniformBuffer,uniformMemory,0),"vkBindBufferMemory");
        vkCheck(vkMapMemory(device,uniformMemory,0,sizeof(Uniforms),0,&mapped),"vkMapMemory");
        if(!initGraphics())throw std::runtime_error("Graphics setup failed");
        return true;
    }catch(const std::exception& e){LOGE("Vulkan init failed: %s",e.what());setMessage(e.what());return false;}
}
void Engine::shutdown(){
    if(device){vkDeviceWaitIdle(device);
        if(fence)vkDestroyFence(device,fence,nullptr);
        if(completed)vkDestroySemaphore(device,completed,nullptr);
        if(acquired)vkDestroySemaphore(device,acquired,nullptr);
        if(commandPool)vkDestroyCommandPool(device,commandPool,nullptr);
        if(pipeline)vkDestroyPipeline(device,pipeline,nullptr);
        if(pipelineLayout)vkDestroyPipelineLayout(device,pipelineLayout,nullptr);
        if(descPool)vkDestroyDescriptorPool(device,descPool,nullptr);
        if(descLayout)vkDestroyDescriptorSetLayout(device,descLayout,nullptr);
        for(auto fb:buffers)vkDestroyFramebuffer(device,fb,nullptr);
        if(renderPass)vkDestroyRenderPass(device,renderPass,nullptr);
        if(mapped)vkUnmapMemory(device,uniformMemory);
        if(uniformBuffer)vkDestroyBuffer(device,uniformBuffer,nullptr);
        if(uniformMemory)vkFreeMemory(device,uniformMemory,nullptr);
        for(auto view:views)vkDestroyImageView(device,view,nullptr);
        if(swap)vkDestroySwapchainKHR(device,swap,nullptr);
        vkDestroyDevice(device,nullptr);
    }
    if(surface)vkDestroySurfaceKHR(instance,surface,nullptr);
    if(instance)vkDestroyInstance(instance,nullptr);
}
