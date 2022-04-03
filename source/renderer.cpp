#include "renderer.h"

RendererVK::RendererVK() :
   FrameWidth( 1280 ), FrameHeight( 720 ), FrameIndex( 0 ), Framerate( 30.0f ), Instance{},
   ColorFormat( VK_FORMAT_R8G8B8A8_SRGB ), Framebuffer{}, Common( std::make_shared<CommonVK>() ), ColorAttachment{},
   DepthAttachment{}, VertexBuffer{}, VertexBufferMemory{}, CommandBuffer{}, Fence{}
{
}

RendererVK::~RendererVK()
{
   UpperSquareObject.reset();
   LowerSquareObject.reset();
   Shader.reset();

   VkDevice device = CommonVK::getDevice();
   vkDestroyFence( device, Fence, nullptr );
   vkDestroyImageView( device, ColorAttachment.View, nullptr );
   vkDestroyImage( device, ColorAttachment.Image, nullptr );
   vkFreeMemory( device, ColorAttachment.Memory, nullptr );
   vkDestroyImageView( device, DepthAttachment.View, nullptr );
   vkDestroyImage( device, DepthAttachment.Image, nullptr );
   vkFreeMemory( device, DepthAttachment.Memory, nullptr );
   vkDestroyBuffer( device, VertexBuffer, nullptr );
   vkFreeMemory( device, VertexBufferMemory, nullptr );
   vkDestroyFramebuffer( device, Framebuffer, nullptr );
   vkDestroyCommandPool( device, CommonVK::getCommandPool(), nullptr );
   vkDestroyDevice( device, nullptr );
#ifdef _DEBUG
   destroyDebugUtilsMessengerEXT( Instance, DebugMessenger, nullptr );
#endif
   vkDestroyInstance( Instance, nullptr );
}

#ifdef _DEBUG
void RendererVK::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
   create_info = {};
   create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   create_info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   create_info.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   create_info.pfnUserCallback = debugCallback;
}

void RendererVK::setupDebugMessenger()
{
   VkDebugUtilsMessengerCreateInfoEXT create_info;
   populateDebugMessengerCreateInfo( create_info );

   const VkResult result = createDebugUtilsMessengerEXT(
   Instance,
   &create_info,
   nullptr,
   &DebugMessenger
   );
   if (result != VK_SUCCESS) throw std::runtime_error("failed to set up debug messenger!");
}
#endif

void RendererVK::createImageViews()
{
   CommonVK::createImage(
      FrameWidth, FrameHeight,
      ColorFormat,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      ColorAttachment.Image,
      ColorAttachment.Memory
   );

   VkImageViewCreateInfo create_info{};
   create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   create_info.image = ColorAttachment.Image;
   create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
   create_info.format = ColorFormat;
   create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
   create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
   create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
   create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
   create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   create_info.subresourceRange.baseMipLevel = 0;
   create_info.subresourceRange.levelCount = 1;
   create_info.subresourceRange.baseArrayLayer = 0;
   create_info.subresourceRange.layerCount = 1;

   const VkResult result = vkCreateImageView(
      CommonVK::getDevice(),
      &create_info,
      nullptr,
      &ColorAttachment.View
   );
   if (result != VK_SUCCESS) throw std::runtime_error("failed to create image views!");
}

void RendererVK::createObject()
{
   UpperSquareObject = std::make_shared<ObjectVK>( Common.get() );
   UpperSquareObject->setSquareObject( std::filesystem::path(CMAKE_SOURCE_DIR) / "emoy.png" );
   UpperSquareObject->createDescriptorPool();
   UpperSquareObject->createUniformBuffers();
   UpperSquareObject->createDescriptorSets( Shader->getDescriptorSetLayout() );

   LowerSquareObject = std::make_shared<ObjectVK>( Common.get() );
   LowerSquareObject->setSquareObject( std::filesystem::path(CMAKE_SOURCE_DIR) / "emoy.png" );
   LowerSquareObject->createDescriptorPool();
   LowerSquareObject->createUniformBuffers();
   LowerSquareObject->createDescriptorSets( Shader->getDescriptorSetLayout() );
}

void RendererVK::createGraphicsPipeline()
{
   Shader = std::make_shared<ShaderVK>( Common.get() );
   Shader->createRenderPass( ColorFormat );
   Shader->createDescriptorSetLayout();
   Shader->createGraphicsPipeline(
      std::filesystem::path(CMAKE_SOURCE_DIR) / "shaders/shader.vert.spv",
      std::filesystem::path(CMAKE_SOURCE_DIR) / "shaders/shader.frag.spv",
      ObjectVK::getBindingDescription(),
      ObjectVK::getAttributeDescriptions(),
      { FrameWidth, FrameHeight }
   );
}

void RendererVK::createFramebuffers()
{
   std::array<VkImageView, 2> attachments = { ColorAttachment.View, DepthAttachment.View };
   VkFramebufferCreateInfo framebuffer_info{};
   framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
   framebuffer_info.renderPass = Shader->getRenderPass();
   framebuffer_info.attachmentCount = attachments.size();
   framebuffer_info.pAttachments = attachments.data();
   framebuffer_info.width = FrameWidth;
   framebuffer_info.height = FrameHeight;
   framebuffer_info.layers = 1;

   const VkResult result = vkCreateFramebuffer(
      CommonVK::getDevice(),
      &framebuffer_info,
      nullptr,
      &Framebuffer
   );
   if (result != VK_SUCCESS) throw std::runtime_error("failed to create framebuffer!");
}

void RendererVK::createDepthResources()
{
   VkFormat depth_format = CommonVK::findDepthFormat();
   CommonVK::createImage(
      FrameWidth, FrameHeight,
      depth_format,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      DepthAttachment.Image,
      DepthAttachment.Memory
   );
   DepthAttachment.View = CommonVK::createImageView(
      DepthAttachment.Image,
      depth_format,
      VK_IMAGE_ASPECT_DEPTH_BIT
   );
}

void RendererVK::copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
   VkCommandBufferAllocateInfo allocate_info{};
   allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocate_info.commandPool = CommonVK::getCommandPool();
   allocate_info.commandBufferCount = 1;

   VkCommandBuffer command_buffer;
   vkAllocateCommandBuffers( CommonVK::getDevice(), &allocate_info, &command_buffer );

   VkCommandBufferBeginInfo begin_info{};
   begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

   vkBeginCommandBuffer( command_buffer, &begin_info );
      VkBufferCopy copy_region{};
      copy_region.size = size;
      vkCmdCopyBuffer(
         command_buffer,
         src_buffer, dst_buffer,
         1, &copy_region
      );
   vkEndCommandBuffer( command_buffer );

   VkSubmitInfo submit_info{};
   submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submit_info.commandBufferCount = 1;
   submit_info.pCommandBuffers = &command_buffer;

   vkQueueSubmit( CommonVK::getGraphicsQueue(), 1, &submit_info, VK_NULL_HANDLE );
   vkQueueWaitIdle( CommonVK::getGraphicsQueue() );

   vkFreeCommandBuffers(
      CommonVK::getDevice(),
      CommonVK::getCommandPool(),
      1,
      &command_buffer
   );
}

void RendererVK::createVertexBuffer()
{
   VkBuffer staging_buffer;
   VkDeviceMemory staging_buffer_memory;
   const VkDeviceSize buffer_size = LowerSquareObject->getVertexBufferSize();
   CommonVK::createBuffer(
      buffer_size,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      staging_buffer,
      staging_buffer_memory
   );

   void* data;
   vkMapMemory(
      CommonVK::getDevice(),
      staging_buffer_memory,
      0, buffer_size, 0, &data
   );
      memcpy( data, LowerSquareObject->getVertexData(), static_cast<size_t>(buffer_size) );
   vkUnmapMemory( CommonVK::getDevice(), staging_buffer_memory );

   CommonVK::createBuffer(
      buffer_size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      VertexBuffer,
      VertexBufferMemory
   );
   copyBuffer( staging_buffer, VertexBuffer, buffer_size );

   vkDestroyBuffer( CommonVK::getDevice(), staging_buffer, nullptr );
   vkFreeMemory( CommonVK::getDevice(), staging_buffer_memory, nullptr );
}

void RendererVK::createCommandBuffer()
{
   VkCommandBufferAllocateInfo allocate_info{};
   allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocate_info.commandPool = CommonVK::getCommandPool();
   allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocate_info.commandBufferCount = 1;

   const VkResult result = vkAllocateCommandBuffers(
      CommonVK::getDevice(),
      &allocate_info,
      &CommandBuffer
   );
   if (result != VK_SUCCESS) throw std::runtime_error("failed to allocate command buffers!");
}

void RendererVK::createSyncObjects()
{
   VkFenceCreateInfo fence_info{};
   fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
   fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
   const VkResult fence_result = vkCreateFence(
      CommonVK::getDevice(),
      &fence_info,
      nullptr,
      &Fence
   );
   if (fence_result != VK_SUCCESS) throw std::runtime_error("failed to create synchronization objects for a frame!");
}

void RendererVK::createRecorder()
{
   Recorder = std::make_shared<VideoWriter>();
   const std::string output_file_path = std::string(CMAKE_SOURCE_DIR) + "/result.mp4";
   const bool result = Recorder->open(
      output_file_path,
      static_cast<int>(FrameWidth),
      static_cast<int>(FrameHeight),
      Framerate,
      AV_CODEC_ID_H264
   );
   if (!result) throw std::runtime_error("Could not write video");
}

void RendererVK::initializeVulkan()
{
   createInstance();
#ifdef _DEBUG
   setupDebugMessenger();
#endif
   Common->pickPhysicalDevice( Instance );
   Common->createLogicalDevice();
   Common->createCommandPool();
   createImageViews();
   createGraphicsPipeline();
   createObject();
   createDepthResources();
   createFramebuffers();
   createVertexBuffer();
   createCommandBuffer();
   createSyncObjects();
   createRecorder();
}

void RendererVK::recordCommandBuffer(VkCommandBuffer command_buffer)
{
   VkCommandBufferBeginInfo begin_info{};
   begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

   if (vkBeginCommandBuffer( command_buffer, &begin_info ) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer!");
   }

   VkRenderPassBeginInfo render_pass_info{};
   render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   render_pass_info.renderPass = Shader->getRenderPass();
   render_pass_info.framebuffer = Framebuffer;
   render_pass_info.renderArea.offset = { 0, 0 };
   render_pass_info.renderArea.extent = { FrameWidth, FrameHeight };

   std::array<VkClearValue, 2> clear_values{};
   clear_values[0].color = { { 0.35f, 0.0f, 0.53f, 1.0f } };
   clear_values[1].depthStencil = { 1.0f, 0 };
   render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
   render_pass_info.pClearValues = clear_values.data();

   vkCmdBeginRenderPass(
      command_buffer,
      &render_pass_info,
      VK_SUBPASS_CONTENTS_INLINE
   );
      vkCmdBindPipeline(
         command_buffer,
         VK_PIPELINE_BIND_POINT_GRAPHICS,
         Shader->getGraphicsPipeline()
      );
      const std::array<VkBuffer, 1> vertex_buffers = { VertexBuffer };
      constexpr std::array<VkDeviceSize, 1> offsets = { 0 };
      vkCmdBindVertexBuffers(
         command_buffer, 0, 1,
         vertex_buffers.data(), offsets.data()
      );

      vkCmdBindDescriptorSets(
         command_buffer,
         VK_PIPELINE_BIND_POINT_GRAPHICS,
         Shader->getPipelineLayout(),
         0, 1,
         LowerSquareObject->getDescriptorSet(),
         0, nullptr
      );
      vkCmdDraw(
         command_buffer, LowerSquareObject->getVertexSize(),
         1, 0, 0
      );

      vkCmdBindDescriptorSets(
         command_buffer,
         VK_PIPELINE_BIND_POINT_GRAPHICS,
         Shader->getPipelineLayout(),
         0, 1,
         UpperSquareObject->getDescriptorSet(),
         0, nullptr
      );
      vkCmdDraw(
         command_buffer, UpperSquareObject->getVertexSize(),
         1, 0, 0
      );
   vkCmdEndRenderPass( command_buffer );

   if (vkEndCommandBuffer( command_buffer ) != VK_SUCCESS) {
      throw std::runtime_error( "failed to record command buffer!");
   }
}

void RendererVK::drawFrame()
{
   vkWaitForFences(
      CommonVK::getDevice(),
      1,
      &Fence,
      VK_TRUE,
      UINT64_MAX
   );

   const glm::mat4 lower_world =
      glm::translate( glm::mat4(1.0f), glm::vec3(-0.25f, 0.0f, 0.0f) ) *
      glm::rotate(
         glm::mat4(1.0f),
         glm::radians( static_cast<float>(FrameIndex) * 5.0f ),
         glm::vec3(0.0f, 1.0f, 0.0f)
      ) * glm::translate( glm::mat4(1.0f), glm::vec3(-0.5f, -0.5f, 0.0f) );
   const glm::mat4 upper_world =
      glm::translate( glm::mat4(1.0f), glm::vec3(0.5f, 0.0f, 0.0f) ) * lower_world;
   LowerSquareObject->updateUniformBuffer( { FrameWidth, FrameHeight }, lower_world );
   UpperSquareObject->updateUniformBuffer( { FrameWidth, FrameHeight }, upper_world );

   vkResetFences( CommonVK::getDevice(), 1, &Fence );
   vkResetCommandBuffer( CommandBuffer, 0 );
   recordCommandBuffer( CommandBuffer );

   VkSubmitInfo submit_info{};
   submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submit_info.commandBufferCount = 1;
   submit_info.pCommandBuffers = &CommandBuffer;

   const VkResult result = vkQueueSubmit(
      CommonVK::getGraphicsQueue(),
      1,
      &submit_info,
      Fence
   );
   if (result != VK_SUCCESS) throw std::runtime_error("failed to submit draw command buffer!");
}

std::vector<const char*> RendererVK::getRequiredExtensions()
{
   std::vector<const char*> extensions;
#ifdef _DEBUG
   extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
#endif
   return extensions;
}

void RendererVK::createInstance()
{
#ifdef _DEBUG
   if (!CommonVK::checkValidationLayerSupport()) {
      throw std::runtime_error("validation layers requested, but not available!");
   }
#endif

   VkApplicationInfo application_info{};
   application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   application_info.pApplicationName = "Hello Vulkan";
   application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
   application_info.pEngineName = "No Engine";
   application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
   application_info.apiVersion = VK_API_VERSION_1_0;

   VkInstanceCreateInfo create_info{};
   create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   create_info.pApplicationInfo = &application_info;

   std::vector<const char*> extensions = getRequiredExtensions();
   create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
   create_info.ppEnabledExtensionNames = extensions.data();

#ifdef NDEBUG
   create_info.enabledLayerCount = 0;
   create_info.pNext = nullptr;
#else
   create_info.enabledLayerCount = CommonVK::getValidationLayerSize();
   create_info.ppEnabledLayerNames = CommonVK::getValidationLayerNames();

   VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
   populateDebugMessengerCreateInfo( debug_create_info );
   create_info.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debug_create_info);
#endif

   if (vkCreateInstance( &create_info, nullptr, &Instance ) != VK_SUCCESS) {
      throw std::runtime_error("failed to create Instance!");
   }
}

void RendererVK::writeFrame()
{
   VkImage dst_image;
   VkDeviceMemory dst_image_memory;
   CommonVK::createImage(
      FrameWidth, FrameHeight,
      ColorFormat,
      VK_IMAGE_TILING_LINEAR,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      dst_image,
      dst_image_memory
   );

   VkCommandBuffer copy_command = CommonVK::createCommandBuffer( VK_COMMAND_BUFFER_LEVEL_PRIMARY );

   CommonVK::insertImageMemoryBarrier(
      copy_command,
      dst_image,
      0,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
   );

   VkImageCopy image_copy_region{};
   image_copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   image_copy_region.srcSubresource.layerCount = 1;
   image_copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   image_copy_region.dstSubresource.layerCount = 1;
   image_copy_region.extent.width = FrameWidth;
   image_copy_region.extent.height = FrameHeight;
   image_copy_region.extent.depth = 1;

   vkCmdCopyImage(
      copy_command,
      ColorAttachment.Image,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      dst_image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &image_copy_region
   );

   CommonVK::insertImageMemoryBarrier(
      copy_command,
      dst_image,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_ACCESS_MEMORY_READ_BIT,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_GENERAL,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
   );

   CommonVK::flushCommandBuffer( copy_command );

   VkImageSubresource subresource{};
   subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   VkSubresourceLayout subresource_layout;
   vkGetImageSubresourceLayout(
      CommonVK::getDevice(),
      dst_image,
      &subresource,
      &subresource_layout
   );

   uint8_t* image_data;
   vkMapMemory(
      CommonVK::getDevice(),
      dst_image_memory,
      0,
      VK_WHOLE_SIZE,
      0,
      (void**)&image_data
   );
   image_data += subresource_layout.offset;

   uint8_t* inverted_data = image_data;
   for (int32_t y = 0; y < FrameHeight; ++y) {
      auto* row = (unsigned int*)inverted_data;
      for (int32_t x = 0; x < FrameWidth; ++x) {
         std::swap( *(char*)row, *((char*)row + 2) );
         row++;
      }
      inverted_data += subresource_layout.rowPitch;
   }

   const std::string file_name =
      std::string(CMAKE_SOURCE_DIR) + "/frame[" + std::to_string( FrameIndex ) + "].png";
   FIBITMAP* image = FreeImage_ConvertFromRawBits(
      image_data,
      static_cast<int>(FrameWidth),
      static_cast<int>(FrameHeight),
      static_cast<int>(FrameWidth) * 4,
      32,
      FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, true
   );
   FreeImage_Save( FIF_PNG, image, file_name.c_str() );
   FreeImage_Unload( image );

   vkUnmapMemory( CommonVK::getDevice(), dst_image_memory );
   vkFreeMemory( CommonVK::getDevice(), dst_image_memory, nullptr );
   vkDestroyImage( CommonVK::getDevice(), dst_image, nullptr );
}

void RendererVK::writeVideo()
{
   VkImage dst_image;
   VkDeviceMemory dst_image_memory;
   CommonVK::createImage(
      FrameWidth, FrameHeight,
      ColorFormat,
      VK_IMAGE_TILING_LINEAR,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      dst_image,
      dst_image_memory
   );

   VkCommandBuffer copy_command = CommonVK::createCommandBuffer( VK_COMMAND_BUFFER_LEVEL_PRIMARY );

   CommonVK::insertImageMemoryBarrier(
      copy_command,
      dst_image,
      0,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
   );

   VkImageCopy image_copy_region{};
   image_copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   image_copy_region.srcSubresource.layerCount = 1;
   image_copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   image_copy_region.dstSubresource.layerCount = 1;
   image_copy_region.extent.width = FrameWidth;
   image_copy_region.extent.height = FrameHeight;
   image_copy_region.extent.depth = 1;

   vkCmdCopyImage(
      copy_command,
      ColorAttachment.Image,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      dst_image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &image_copy_region
   );

   CommonVK::insertImageMemoryBarrier(
      copy_command,
      dst_image,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_ACCESS_MEMORY_READ_BIT,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_GENERAL,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
   );

   CommonVK::flushCommandBuffer( copy_command );

   VkImageSubresource subresource{};
   subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   VkSubresourceLayout subresource_layout;
   vkGetImageSubresourceLayout(
      CommonVK::getDevice(),
      dst_image,
      &subresource,
      &subresource_layout
   );

   uint8_t* image_data;
   vkMapMemory(
      CommonVK::getDevice(),
      dst_image_memory,
      0,
      VK_WHOLE_SIZE,
      0,
      (void**)&image_data
   );
   image_data += subresource_layout.offset;

   Recorder->writeVideo( image_data );

   vkUnmapMemory( CommonVK::getDevice(), dst_image_memory );
   vkFreeMemory( CommonVK::getDevice(), dst_image_memory, nullptr );
   vkDestroyImage( CommonVK::getDevice(), dst_image, nullptr );
}

void RendererVK::play()
{
   initializeVulkan();
   while (FrameIndex < 150) {
      drawFrame();
      writeVideo();
      FrameIndex++;
   }
   Recorder->close();
   vkDeviceWaitIdle( CommonVK::getDevice() );
}