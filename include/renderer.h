#pragma once

#include "object.h"
#include "shader.h"
#include "fileio/video_writer.h"

class RendererVK final
{
public:
   RendererVK();
   ~RendererVK();

   void play();

private:
   struct FrameBufferAttachment
   {
		VkImage Image;
		VkDeviceMemory Memory;
		VkImageView View;
	};

   uint32_t FrameWidth;
   uint32_t FrameHeight;
   uint32_t FrameIndex;
   float Framerate;
   VkInstance Instance;
   VkFormat ColorFormat;
   VkFramebuffer Framebuffer;
   std::shared_ptr<CommonVK> Common;
   FrameBufferAttachment ColorAttachment;
   FrameBufferAttachment DepthAttachment;
   VkBuffer VertexBuffer;
   VkDeviceMemory VertexBufferMemory;
   VkCommandBuffer CommandBuffer;
   VkFence Fence;
   std::shared_ptr<ObjectVK> UpperSquareObject;
   std::shared_ptr<ObjectVK> LowerSquareObject;
   std::shared_ptr<ShaderVK> Shader;
   std::shared_ptr<VideoWriter> Recorder;

#ifdef NDEBUG
   inline static constexpr bool EnableValidationLayers = false;
#else
   inline static constexpr bool EnableValidationLayers = true;
   VkDebugUtilsMessengerEXT DebugMessenger{};

   static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);
   void setupDebugMessenger();
   static VkResult createDebugUtilsMessengerEXT(
      VkInstance instance,
      const VkDebugUtilsMessengerCreateInfoEXT* create_info,
      const VkAllocationCallbacks* allocator,
      VkDebugUtilsMessengerEXT* debug_messenger
   )
   {
       auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
         instance,
         "vkCreateDebugUtilsMessengerEXT"
      );
       if (func != nullptr) return func( instance, create_info, allocator, debug_messenger );
       else return VK_ERROR_EXTENSION_NOT_PRESENT;
   }

   static void destroyDebugUtilsMessengerEXT(
      VkInstance instance,
      VkDebugUtilsMessengerEXT debug_messenger,
      const VkAllocationCallbacks* allocator
   )
   {
      auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
         instance,
         "vkDestroyDebugUtilsMessengerEXT"
      );
      if (func != nullptr) func( instance, debug_messenger, allocator);
   }

   static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
      VkDebugUtilsMessageTypeFlagsEXT message_type,
      const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
      void* user_data
   )
   {
      std::cerr << "validation layer: " << callback_data->pMessage << "\n";
      return VK_FALSE;
   }
#endif

   void createImageViews();
   void createObject();
   void createGraphicsPipeline();
   void createDepthResources();
   void createFramebuffers();
   static void copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
   void createVertexBuffer();
   void createCommandBuffer();
   void createSyncObjects();
   void initializeVulkan();
   void recordCommandBuffer(VkCommandBuffer command_buffer);
   void drawFrame();
   void writeFrame();
   void writeVideo();
   [[nodiscard]] static std::vector<const char*> getRequiredExtensions();
   void createInstance();
   void createRecorder();
};