#include <object.h>

ObjectVK::ObjectVK(CommonVK* common) :
   Common( common ), TextureImage{}, TextureImageMemory{}, TextureImageView{}, TextureSampler{}, DescriptorPool{}
{
}

ObjectVK::~ObjectVK()
{
   VkDevice device = CommonVK::getDevice();
   vkDestroyDescriptorPool( device, DescriptorPool, nullptr );
   vkDestroyBuffer( device, MVP.UniformBuffer, nullptr );
   vkFreeMemory( device, MVP.UniformBuffersMemory, nullptr );
   vkDestroyBuffer( device, Material.UniformBuffer, nullptr );
   vkFreeMemory( device, Material.UniformBuffersMemory, nullptr );
   vkDestroyBuffer( device, Light.UniformBuffer, nullptr );
   vkFreeMemory( device, Light.UniformBuffersMemory, nullptr );
   vkDestroySampler( device, TextureSampler, nullptr );
   vkDestroyImageView( device, TextureImageView, nullptr );
   vkDestroyImage( device, TextureImage, nullptr );
   vkFreeMemory( device, TextureImageMemory, nullptr );
}

void ObjectVK::getSquareObject(std::vector<Vertex>& vertices)
{
   vertices = {
      { { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
      { { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
      { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },

      { { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
      { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } }
   };
}

VkCommandBuffer ObjectVK::beginSingleTimeCommands()
{
   VkCommandBufferAllocateInfo allocate_info{};
   allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocate_info.commandPool = CommonVK::getCommandPool();
   allocate_info.commandBufferCount = 1;

   VkCommandBuffer command_buffer;
   vkAllocateCommandBuffers(
      CommonVK::getDevice(),
      &allocate_info,
      &command_buffer
   );

   VkCommandBufferBeginInfo begin_info{};
   begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

   vkBeginCommandBuffer( command_buffer, &begin_info );
   return command_buffer;
}

void ObjectVK::endSingleTimeCommands(VkCommandBuffer command_buffer)
{
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

void ObjectVK::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
   VkCommandBuffer command_buffer = beginSingleTimeCommands();

   VkImageMemoryBarrier barrier{};
   barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   barrier.oldLayout = old_layout;
   barrier.newLayout = new_layout;
   barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   barrier.image = image;
   barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   barrier.subresourceRange.baseMipLevel = 0;
   barrier.subresourceRange.levelCount = 1;
   barrier.subresourceRange.baseArrayLayer = 0;
   barrier.subresourceRange.layerCount = 1;

   VkPipelineStageFlags source_stage;
   VkPipelineStageFlags destination_stage;
   if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
   }
   else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
   }
   else throw std::invalid_argument("unsupported layout transition!");

   vkCmdPipelineBarrier(
      command_buffer,
      source_stage, destination_stage,
      0,
      0, nullptr,
      0, nullptr,
      1, &barrier
   );

   endSingleTimeCommands( command_buffer );
}

void ObjectVK::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
   VkCommandBuffer command_buffer = beginSingleTimeCommands();

   VkBufferImageCopy region{};
   region.bufferOffset = 0;
   region.bufferRowLength = 0;
   region.bufferImageHeight = 0;
   region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   region.imageSubresource.mipLevel = 0;
   region.imageSubresource.baseArrayLayer = 0;
   region.imageSubresource.layerCount = 1;
   region.imageOffset = { 0, 0, 0 };
   region.imageExtent = { width, height, 1 };

   vkCmdCopyBufferToImage(
      command_buffer,
      buffer,
      image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region
   );

   endSingleTimeCommands( command_buffer );
}

void ObjectVK::createTextureImage(const std::string& texture_file_path)
{
   const FREE_IMAGE_FORMAT format = FreeImage_GetFileType( texture_file_path.c_str(), 0 );
   FIBITMAP* texture = FreeImage_Load( format, texture_file_path.c_str() );

   FIBITMAP* texture_converted;
   constexpr uint n_bits = 32;
   const uint n_bits_per_pixel = FreeImage_GetBPP( texture );
   texture_converted = n_bits_per_pixel == n_bits ? texture : FreeImage_ConvertTo32Bits( texture );

   const uint width = FreeImage_GetWidth( texture_converted );
   const uint height = FreeImage_GetHeight( texture_converted );
   void* pixels = FreeImage_GetBits( texture_converted );
   if (!pixels) throw std::runtime_error("failed to load texture image!");

   VkBuffer staging_buffer;
   VkDeviceMemory staging_buffer_memory;
   VkDeviceSize image_size = width * height * 4;
   CommonVK::createBuffer(
      image_size,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      staging_buffer,
      staging_buffer_memory
   );

   void* data;
   vkMapMemory(
      CommonVK::getDevice(),
      staging_buffer_memory,
      0, image_size, 0, &data
   );
      memcpy( data, pixels, static_cast<size_t>(image_size) );
   vkUnmapMemory( CommonVK::getDevice(), staging_buffer_memory );

   FreeImage_Unload( texture_converted );
   if (n_bits_per_pixel != n_bits) FreeImage_Unload( texture );

   CommonVK::createImage(
      width, height,
      VK_FORMAT_R8G8B8A8_SRGB,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      TextureImage,
      TextureImageMemory
   );

   transitionImageLayout(
      TextureImage,
      VK_FORMAT_R8G8B8A8_SRGB,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
   );
   copyBufferToImage(
      staging_buffer,
      TextureImage,
      static_cast<uint32_t>(width), static_cast<uint32_t>(height)
   );
   transitionImageLayout(
      TextureImage,
      VK_FORMAT_R8G8B8A8_SRGB,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
   );

   vkDestroyBuffer( CommonVK::getDevice(), staging_buffer, nullptr );
   vkFreeMemory( CommonVK::getDevice(), staging_buffer_memory, nullptr );
}

void ObjectVK::createTextureImageView()
{
   TextureImageView = CommonVK::createImageView(
      TextureImage,
      VK_FORMAT_R8G8B8A8_SRGB,
      VK_IMAGE_ASPECT_COLOR_BIT
   );
}

void ObjectVK::createTextureSampler()
{
   VkPhysicalDeviceProperties properties{};
   vkGetPhysicalDeviceProperties( CommonVK::getPhysicalDevice(), &properties );

   VkSamplerCreateInfo sampler_info{};
   sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
   sampler_info.magFilter = VK_FILTER_LINEAR;
   sampler_info.minFilter = VK_FILTER_LINEAR;
   sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   sampler_info.anisotropyEnable = VK_FALSE;
   sampler_info.maxAnisotropy = 1;//properties.limits.maxSamplerAnisotropy;
   sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
   sampler_info.unnormalizedCoordinates = VK_FALSE;
   sampler_info.compareEnable = VK_FALSE;
   sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
   sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

   const VkResult result = vkCreateSampler(
      CommonVK::getDevice(),
      &sampler_info,
      nullptr,
      &TextureSampler
   );
   if (result != VK_SUCCESS) throw std::runtime_error("failed to create texture sampler!");
}

void ObjectVK::setSquareObject(const std::string& texture_file_path)
{
   getSquareObject( Vertices );
   createTextureImage( texture_file_path );
   createTextureImageView();
   createTextureSampler();
}

VkVertexInputBindingDescription ObjectVK::getBindingDescription()
{
   VkVertexInputBindingDescription binding_description{};
   binding_description.binding = 0;
   binding_description.stride = sizeof( Vertex );
   binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
   return binding_description;
}

std::array<VkVertexInputAttributeDescription, 3> ObjectVK::getAttributeDescriptions()
{
   std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};

   attribute_descriptions[0].binding = 0;
   attribute_descriptions[0].location = 0;
   attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
   attribute_descriptions[0].offset = offsetof( Vertex, Position );

   attribute_descriptions[1].binding = 0;
   attribute_descriptions[1].location = 1;
   attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
   attribute_descriptions[1].offset = offsetof( Vertex, Normal );

   attribute_descriptions[2].binding = 0;
   attribute_descriptions[2].location = 2;
   attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
   attribute_descriptions[2].offset = offsetof( Vertex, Texture );
   return attribute_descriptions;
 }

 void ObjectVK::createDescriptorPool()
{
   std::array<VkDescriptorPoolSize, 4> pool_sizes{};
   pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   pool_sizes[0].descriptorCount = 1;
   pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   pool_sizes[1].descriptorCount = 1;
   pool_sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   pool_sizes[2].descriptorCount = 1;
   pool_sizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   pool_sizes[3].descriptorCount = 1;

   VkDescriptorPoolCreateInfo pool_info{};
   pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
   pool_info.pPoolSizes = pool_sizes.data();
   pool_info.maxSets = 1;

   const VkResult result = vkCreateDescriptorPool(
      CommonVK::getDevice(),
      &pool_info,
      nullptr,
      &DescriptorPool
   );
   if (result != VK_SUCCESS) throw std::runtime_error("failed to create descriptor pool!");
}

void ObjectVK::createUniformBuffers()
{
   VkDeviceSize mvp_buffer_size = sizeof( MVPUniformBufferObject );
   VkDeviceSize material_buffer_size = sizeof( MaterialUniformBufferObject );
   VkDeviceSize light_buffer_size = sizeof( LightUniformBufferObject );
   CommonVK::createBuffer(
      mvp_buffer_size,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      MVP.UniformBuffer,
      MVP.UniformBuffersMemory
   );
   CommonVK::createBuffer(
      material_buffer_size,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      Material.UniformBuffer,
      Material.UniformBuffersMemory
   );
   CommonVK::createBuffer(
      light_buffer_size,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      Light.UniformBuffer,
      Light.UniformBuffersMemory
   );
}

void ObjectVK::createDescriptorSets(VkDescriptorSetLayout descriptor_set_layout)
{
   std::vector<VkDescriptorSetLayout> layouts(1, descriptor_set_layout);
   VkDescriptorSetAllocateInfo allocate_info{};
   allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   allocate_info.descriptorPool = DescriptorPool;
   allocate_info.descriptorSetCount = 1;
   allocate_info.pSetLayouts = layouts.data();

   const VkResult result = vkAllocateDescriptorSets(
      CommonVK::getDevice(),
      &allocate_info,
      &DescriptorSet
   );
   if (result != VK_SUCCESS) throw std::runtime_error("failed to allocate descriptor sets!");

   VkDescriptorBufferInfo mvp_buffer_info{};
   mvp_buffer_info.buffer = MVP.UniformBuffer;
   mvp_buffer_info.offset = 0;
   mvp_buffer_info.range = sizeof( MVPUniformBufferObject );

   VkDescriptorImageInfo image_info{};
   image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   image_info.imageView = TextureImageView;
   image_info.sampler = TextureSampler;

   VkDescriptorBufferInfo material_buffer_info{};
   material_buffer_info.buffer = Material.UniformBuffer;
   material_buffer_info.offset = 0;
   material_buffer_info.range = sizeof( MaterialUniformBufferObject );

   VkDescriptorBufferInfo light_buffer_info{};
   light_buffer_info.buffer = Light.UniformBuffer;
   light_buffer_info.offset = 0;
   light_buffer_info.range = sizeof( LightUniformBufferObject );

   std::array<VkWriteDescriptorSet, 4> descriptor_writes{};
   descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   descriptor_writes[0].dstSet = DescriptorSet;
   descriptor_writes[0].dstBinding = 0;
   descriptor_writes[0].dstArrayElement = 0;
   descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   descriptor_writes[0].descriptorCount = 1;
   descriptor_writes[0].pBufferInfo = &mvp_buffer_info;

   descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   descriptor_writes[1].dstSet = DescriptorSet;
   descriptor_writes[1].dstBinding = 1;
   descriptor_writes[1].dstArrayElement = 0;
   descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   descriptor_writes[1].descriptorCount = 1;
   descriptor_writes[1].pImageInfo = &image_info;

   descriptor_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   descriptor_writes[2].dstSet = DescriptorSet;
   descriptor_writes[2].dstBinding = 2;
   descriptor_writes[2].dstArrayElement = 0;
   descriptor_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   descriptor_writes[2].descriptorCount = 1;
   descriptor_writes[2].pBufferInfo = &material_buffer_info;

   descriptor_writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   descriptor_writes[3].dstSet = DescriptorSet;
   descriptor_writes[3].dstBinding = 3;
   descriptor_writes[3].dstArrayElement = 0;
   descriptor_writes[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   descriptor_writes[3].descriptorCount = 1;
   descriptor_writes[3].pBufferInfo = &light_buffer_info;

   vkUpdateDescriptorSets(
      CommonVK::getDevice(),
      static_cast<uint32_t>(descriptor_writes.size()),
      descriptor_writes.data(),
      0,
      nullptr
   );
}

void ObjectVK::updateUniformBuffer(VkExtent2D extent, const glm::mat4& to_world)
{
   MVPUniformBufferObject mvp{};
   mvp.Model = to_world;
   mvp.View = glm::lookAt(
      glm::vec3(0.0f, 0.0f, -2.0f),
      glm::vec3(0.0f, 0.0f, 0.0f),
      glm::vec3(0.0f, 1.0f, 0.0f)
   );
   mvp.Projection = glm::perspective(
      glm::radians( 45.0f ),
      static_cast<float>(extent.width) / static_cast<float>(extent.height),
      0.1f,
      10.0f
   );

   // glm was originally designed for OpenGL, where the y-coordinate of the clip coordinates is inverted.
   // The easiest way to compensate for that is to flip the sign on the scaling factor of the y-axis in the projection
   // matrix. If you do not do this, then the image will be rendered upside down.
   mvp.Projection[1][1] *= -1;

   MaterialUniformBufferObject material{};
   material.EmissionColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
   material.AmbientColor = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
   material.DiffuseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
   material.SpecularColor = glm::vec4(1.0f, 1.0f, 0.77f, 1.0f);
   material.SpecularExponent = 2.0f;

   LightUniformBufferObject light{};
   light.Position = glm::vec4(0.5f, 0.5f, 2.5f, 0.0f);
   light.AmbientColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
   light.DiffuseColor = glm::vec4(1.0f, 1.0f, 0.77f, 1.0f);
   light.SpecularColor = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f);
   light.AttenuationFactors = glm::vec3(1.0f, 1.0f, 1.0f);
   light.SpotlightDirection = glm::vec3(0.0f, 0.0f, -2.0f);
   light.SpotlightCutoffAngle = 45.0f;
   light.SpotlightFeather = 0.5f;
   light.FallOffRadius = 1000.0f;

   void* mvp_data;
   vkMapMemory(
      CommonVK::getDevice(),
      MVP.UniformBuffersMemory,
      0, sizeof( mvp ), 0, &mvp_data
   );
      std::memcpy( mvp_data, &mvp, sizeof( mvp ) );
   vkUnmapMemory( CommonVK::getDevice(), MVP.UniformBuffersMemory );

   void* material_data;
   vkMapMemory(
      CommonVK::getDevice(),
      Material.UniformBuffersMemory,
      0, sizeof( material ), 0, &material_data
   );
      std::memcpy( material_data, &material, sizeof( material ) );
   vkUnmapMemory( CommonVK::getDevice(), Material.UniformBuffersMemory );

   void* light_data;
   vkMapMemory(
      CommonVK::getDevice(),
      Light.UniformBuffersMemory,
      0, sizeof( light ), 0, &light_data
   );
      std::memcpy( light_data, &light, sizeof( light ) );
   vkUnmapMemory( CommonVK::getDevice(), Light.UniformBuffersMemory );
}