#include <shader.h>

ShaderVK::ShaderVK(CommonVK* common) :
   Common( common ), RenderPass{}, DescriptorSetLayout{}, PipelineLayout{}, GraphicsPipeline{}
{
}

ShaderVK::~ShaderVK()
{
   VkDevice device = CommonVK::getDevice();
   vkDestroyRenderPass( device, RenderPass, nullptr );
   vkDestroyDescriptorSetLayout( device, DescriptorSetLayout, nullptr);
   vkDestroyPipeline( device, GraphicsPipeline, nullptr );
   vkDestroyPipelineLayout( device, PipelineLayout, nullptr );
}

void ShaderVK::createRenderPass(VkFormat color_format)
{
   VkAttachmentDescription color_attachment{};
   color_attachment.format = color_format;
   color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
   color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   color_attachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

   VkAttachmentDescription depth_attachment{};
   depth_attachment.format = CommonVK::findDepthFormat();
   depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
   depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

   VkAttachmentReference color_attachment_ref{};
   color_attachment_ref.attachment = 0;
   color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

   VkAttachmentReference depth_attachment_ref{};
   depth_attachment_ref.attachment = 1;
   depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

   VkSubpassDescription subpass{};
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.colorAttachmentCount = 1;
   subpass.pColorAttachments = &color_attachment_ref;
   subpass.pDepthStencilAttachment = &depth_attachment_ref;

   VkSubpassDependency dependency{};
   dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
   dependency.dstSubpass = 0;
   dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
   dependency.srcAccessMask = 0;
   dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
   dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

   std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };
   VkRenderPassCreateInfo render_pass_info{};
   render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
   render_pass_info.pAttachments = attachments.data();
   render_pass_info.subpassCount = 1;
   render_pass_info.pSubpasses = &subpass;
   render_pass_info.dependencyCount = 1;
   render_pass_info.pDependencies = &dependency;

   const VkResult result = vkCreateRenderPass(
      CommonVK::getDevice(),
      &render_pass_info,
      nullptr,
      &RenderPass
   );
   if (result != VK_SUCCESS) throw std::runtime_error("failed to create render pass!");
}

void ShaderVK::createDescriptorSetLayout()
{
   VkDescriptorSetLayoutBinding mvp_ubo_layout_binding{};
   mvp_ubo_layout_binding.binding = 0;
   mvp_ubo_layout_binding.descriptorCount = 1;
   mvp_ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   mvp_ubo_layout_binding.pImmutableSamplers = nullptr;
   mvp_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

   VkDescriptorSetLayoutBinding sampler_layout_binding{};
   sampler_layout_binding.binding = 1;
   sampler_layout_binding.descriptorCount = 1;
   sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   sampler_layout_binding.pImmutableSamplers = nullptr;
   sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

   VkDescriptorSetLayoutBinding material_ubo_layout_binding{};
   material_ubo_layout_binding.binding = 2;
   material_ubo_layout_binding.descriptorCount = 1;
   material_ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   material_ubo_layout_binding.pImmutableSamplers = nullptr;
   material_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

   VkDescriptorSetLayoutBinding light_ubo_layout_binding{};
   light_ubo_layout_binding.binding = 3;
   light_ubo_layout_binding.descriptorCount = 1;
   light_ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   light_ubo_layout_binding.pImmutableSamplers = nullptr;
   light_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

   std::array<VkDescriptorSetLayoutBinding, 4> bindings = {
      mvp_ubo_layout_binding,
      sampler_layout_binding,
      material_ubo_layout_binding,
      light_ubo_layout_binding
   };
   VkDescriptorSetLayoutCreateInfo layoutInfo{};
   layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
   layoutInfo.pBindings = bindings.data();

   const VkResult result = vkCreateDescriptorSetLayout(
      CommonVK::getDevice(),
      &layoutInfo,
      nullptr,
      &DescriptorSetLayout
   );
   if (result != VK_SUCCESS) throw std::runtime_error("failed to create descriptor set layout!");
}

std::vector<char> ShaderVK::readFile(const std::string& filename)
{
   std::ifstream file(filename, std::ios::ate | std::ios::binary);

   if (!file.is_open()) throw std::runtime_error("failed to open file!");

   const auto file_size = static_cast<int>(file.tellg());
   std::vector<char> buffer( file_size);
   file.seekg( 0 );
   file.read( buffer.data(), file_size );
   file.close();
   return buffer;
}

VkShaderModule ShaderVK::createShaderModule(const std::vector<char>& code)
{
   VkShaderModuleCreateInfo create_info{};
   create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   create_info.codeSize = code.size();
   create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

   VkShaderModule shader_module;
   const VkResult result = vkCreateShaderModule(
      CommonVK::getDevice(),
      &create_info,
      nullptr,
      &shader_module
   );
   if (result != VK_SUCCESS) throw std::runtime_error("failed to create shader module!");
   return shader_module;
}

void ShaderVK::createGraphicsPipeline(
   const std::string& vertex_shader_path,
   const std::string& fragment_shader_path,
   const VkVertexInputBindingDescription& binding_description,
   const  std::array<VkVertexInputAttributeDescription, 3>& attribute_descriptions,
   const VkExtent2D& extent
)
{
   std::vector<char> vert_shader_code = readFile( vertex_shader_path );
   std::vector<char> frag_shader_code = readFile( fragment_shader_path );

   VkShaderModule vert_shader_module = createShaderModule( vert_shader_code );
   VkShaderModule frag_shader_module = createShaderModule( frag_shader_code );

   VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
   vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
   vert_shader_stage_info.module = vert_shader_module;
   vert_shader_stage_info.pName = "main";

   VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
   frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
   frag_shader_stage_info.module = frag_shader_module;
   frag_shader_stage_info.pName = "main";

   VkPipelineVertexInputStateCreateInfo vertex_input_info{};
   vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   vertex_input_info.vertexBindingDescriptionCount = 0;
   vertex_input_info.vertexAttributeDescriptionCount = 0;

   vertex_input_info.vertexBindingDescriptionCount = 1;
   vertex_input_info.pVertexBindingDescriptions = &binding_description;
   vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
   vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

   VkPipelineInputAssemblyStateCreateInfo input_assembly{};
   input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   input_assembly.primitiveRestartEnable = VK_FALSE;

   VkViewport viewport{};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = static_cast<float>(extent.width);
   viewport.height = static_cast<float>(extent.height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;

   VkRect2D scissor{};
   scissor.offset = { 0, 0 };
   scissor.extent = extent;

   VkPipelineViewportStateCreateInfo viewport_state{};
   viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewport_state.viewportCount = 1;
   viewport_state.pViewports = &viewport;
   viewport_state.scissorCount = 1;
   viewport_state.pScissors = &scissor;

   VkPipelineRasterizationStateCreateInfo rasterizer{};
   rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   rasterizer.depthClampEnable = VK_FALSE;
   rasterizer.rasterizerDiscardEnable = VK_FALSE;
   rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
   rasterizer.lineWidth = 1.0f;
   rasterizer.cullMode = VK_CULL_MODE_NONE;
   rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
   rasterizer.depthBiasEnable = VK_FALSE;

   VkPipelineMultisampleStateCreateInfo multisampling{};
   multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisampling.sampleShadingEnable = VK_FALSE;
   multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

   VkPipelineDepthStencilStateCreateInfo depth_stencil{};
   depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
   depth_stencil.depthTestEnable = VK_TRUE;
   depth_stencil.depthWriteEnable = VK_TRUE;
   depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
   depth_stencil.depthBoundsTestEnable = VK_FALSE;
   depth_stencil.stencilTestEnable = VK_FALSE;

   VkPipelineColorBlendAttachmentState color_blend_attachment{};
   color_blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   color_blend_attachment.blendEnable = VK_FALSE;

   VkPipelineColorBlendStateCreateInfo color_blending{};
   color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   color_blending.logicOpEnable = VK_FALSE;
   color_blending.logicOp = VK_LOGIC_OP_COPY;
   color_blending.attachmentCount = 1;
   color_blending.pAttachments = &color_blend_attachment;
   color_blending.blendConstants[0] = 0.0f;
   color_blending.blendConstants[1] = 0.0f;
   color_blending.blendConstants[2] = 0.0f;
   color_blending.blendConstants[3] = 0.0f;

   VkPipelineLayoutCreateInfo pipeline_layout_info{};
   pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   pipeline_layout_info.setLayoutCount = 1;
   pipeline_layout_info.pSetLayouts = &DescriptorSetLayout;

   VkResult result = vkCreatePipelineLayout(
      CommonVK::getDevice(),
      &pipeline_layout_info,
      nullptr,
      &PipelineLayout
   );
   if (result != VK_SUCCESS) throw std::runtime_error("failed to create pipeline layout!");

   std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = { vert_shader_stage_info, frag_shader_stage_info };
   VkGraphicsPipelineCreateInfo pipeline_info{};
   pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipeline_info.stageCount = shader_stages.size();
   pipeline_info.pStages = shader_stages.data();
   pipeline_info.pVertexInputState = &vertex_input_info;
   pipeline_info.pInputAssemblyState = &input_assembly;
   pipeline_info.pViewportState = &viewport_state;
   pipeline_info.pRasterizationState = &rasterizer;
   pipeline_info.pMultisampleState = &multisampling;
   pipeline_info.pDepthStencilState = &depth_stencil;
   pipeline_info.pColorBlendState = &color_blending;
   pipeline_info.layout = PipelineLayout;
   pipeline_info.renderPass = RenderPass;
   pipeline_info.subpass = 0;
   pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

   result = vkCreateGraphicsPipelines(
      CommonVK::getDevice(),
      VK_NULL_HANDLE,
      1,
      &pipeline_info,
      nullptr,
      &GraphicsPipeline
   );
   if (result != VK_SUCCESS) throw std::runtime_error("failed to create graphics pipeline!");

   vkDestroyShaderModule( CommonVK::getDevice(), frag_shader_module, nullptr );
   vkDestroyShaderModule( CommonVK::getDevice(), vert_shader_module, nullptr );
}