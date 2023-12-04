#pragma once

#include "common.h"

class ShaderVK
{
public:
   explicit ShaderVK(CommonVK* common);
   virtual ~ShaderVK();

   [[nodiscard]] VkRenderPass getRenderPass() const { return RenderPass; }
   [[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const { return DescriptorSetLayout; }
   [[nodiscard]] VkPipelineLayout getPipelineLayout() const { return PipelineLayout; }
   [[nodiscard]] VkPipeline getGraphicsPipeline() const { return GraphicsPipeline; }
   void createRenderPass(VkFormat color_format);
   void createDescriptorSetLayout();
   virtual void createGraphicsPipeline(
      const std::string& vertex_shader_path,
      const std::string& fragment_shader_path,
      const VkVertexInputBindingDescription& binding_description,
      const  std::array<VkVertexInputAttributeDescription, 3>& attribute_descriptions,
      const VkExtent2D& extent
   );

private:
   CommonVK* Common;
   VkRenderPass RenderPass;
   VkDescriptorSetLayout DescriptorSetLayout;
   VkPipelineLayout PipelineLayout;
   VkPipeline GraphicsPipeline;

   static std::vector<char> readFile(const std::string& filename);
   static VkShaderModule createShaderModule(const std::vector<char>& code);
};