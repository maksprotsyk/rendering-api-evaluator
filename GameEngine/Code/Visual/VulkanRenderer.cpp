#include "VulkanRenderer.h"

#include <stdexcept>
#include <vector>
#include <iostream>
#include <set>
#include <vulkan/vulkan_win32.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include "stb_image.h"
#include "tiny_obj_loader.h"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"

#include "Window.h"
#include "Utils/DebugMacros.h"


namespace Engine::Visual
{
	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::init(const Window& window)
	{
		createInstance();
		createSurface(window);
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createCommandPool();
		createDepthResources();
		createFramebuffers();
		createDescriptorPool();
		createSyncObjects();
		createCommandBuffers();
		createTextureSampler();
		createProjectionMatrix();
		createDefaultMaterial();
#ifdef _SHOWUI
		initUI();
#endif
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::clearBackground(float r, float g, float b, float a)
	{
		VkResult result = vkAcquireNextImageKHR(
			m_device,
			m_swapChain,
			UINT64_MAX,
			m_imageAvailableSemaphores[m_currentImageInFlight],
			VK_NULL_HANDLE,
			&m_imageIndex
		);
		ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to acquire swap chain image, result: {}", (int)result);

		// Wait for the fence of the current frame before reusing the command buffer
		vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentImageInFlight], VK_TRUE, UINT64_MAX);
		vkResetFences(m_device, 1, &m_inFlightFences[m_currentImageInFlight]);

		const VkCommandBuffer& commandBuffer = m_commandBuffers[m_imageIndex];
		VkResult resetResult = vkResetCommandBuffer(m_commandBuffers[m_imageIndex], 0);
		if (!validateResult(resetResult, "Failed to reset command buffer"))
		{
			return;
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		if (!validateResult(result, "Failed to begin recording command buffer"))
		{
			return;
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_swapChainFramebuffers[m_imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapChainExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { r, g, b, a };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::draw(const IModelInstance& model, const Utils::Vector3& position, const Utils::Vector3& rotation, const Utils::Vector3& scale)
	{
		const auto& modelItr = m_models.find(model.GetId());

		ASSERT(modelItr != m_models.end(), "Failed to find model width id: {}", model.GetId());
		if (modelItr == m_models.end())
		{
			return;
		}

		const ModelData& modelData = modelItr->second;
		const VulkanModelInstance& modelInstance = (const VulkanModelInstance&)(model);

		const VkCommandBuffer& commandBuffer = m_commandBuffers[m_imageIndex];

		glm::mat4 worldMatrix = getWorldMatrix(position, rotation, scale);
		m_ubo.worldMatrix = worldMatrix;

		bool setUboMemoryResult = setBufferMemoryData(modelInstance.uniformBufferMemory, &m_ubo, sizeof(m_ubo));
		ASSERT(setUboMemoryResult, "Failed to set memory data for uniform buffer");

		VkBuffer vertexBuffers[] = { modelData.vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		std::array<VkDescriptorSet, 1> instanceDescriptorSets{ modelInstance.descriptorSet };
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, static_cast<uint32_t>(instanceDescriptorSets.size()), instanceDescriptorSets.data(), 0, nullptr);

		for (const auto& mat : modelData.materials)
		{
			MaterialBufferObject mbo{};
			mbo.ambientColor = mat.ambientColor;
			mbo.specularColor = mat.specularColor;
			mbo.diffuseColor = mat.diffuseColor;
			mbo.shininess = mat.shininess;

			bool setMboMemoryResult = setBufferMemoryData(mat.materialBufferMemory, &mbo, sizeof(mbo));
			ASSERT(setMboMemoryResult, "Failed to set memory data for material buffer");
		}

		std::unordered_map<int, std::vector<size_t>> materialMeshes;
		for (size_t i = 0; i < modelData.meshes.size(); i++)
		{
			materialMeshes[modelData.meshes[i].materialId].push_back(i);
		}

		for (const auto& [materialId, meshIndices] : materialMeshes)
		{
			const Material& material = materialId != -1 ? modelData.materials[materialId] : m_defaultMaterial;
			const TextureData& texture = getTexture(material.diffuseTextureId);
			std::array<VkDescriptorSet, 2> materialDescriptorSets{ material.descriptorSet, texture.descriptorSet };
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, static_cast<uint32_t>(materialDescriptorSets.size()), materialDescriptorSets.data(), 0, nullptr);
			for (size_t meshIndex : meshIndices)
			{
				const SubMesh& mesh = modelData.meshes[meshIndex];
				vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
			}

		}

	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::startUIRender()
	{
		ImGui_ImplVulkan_NewFrame();
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::endUIRender()
	{
		VkCommandBuffer commandBuffer = m_commandBuffers[m_imageIndex];
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::render()
	{
		VkCommandBuffer commandBuffer = m_commandBuffers[m_imageIndex];

		vkCmdEndRenderPass(commandBuffer);
		VkResult endCommandBufferResult = vkEndCommandBuffer(commandBuffer);
		if (!validateResult(endCommandBufferResult, "Failed to record command buffer"))
		{
			return;
		}

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffers[m_imageIndex];

		VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentImageInFlight] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentImageInFlight] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		VkResult queueSubmitResult = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentImageInFlight]);
		if (!validateResult(queueSubmitResult, "Failed to submit draw command buffer"))
		{
			return;
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &m_imageIndex;

		presentInfo.pResults = nullptr;

		VkResult queuePresentResult = vkQueuePresentKHR(m_presentQueue, &presentInfo);
		if (!validateResult(queuePresentResult, "Failed to present swap chain image"))
		{
			return;
		}

		m_currentImageInFlight = (m_currentImageInFlight + 1) % MAX_FRAMES_IN_FLIGHT;

	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::createBuffersForModel(ModelData& model)
	{
		if (!createVertexBuffer(model))
		{
			return false;
		}

		if (!createIndexBuffer(model))
		{
			return false;
		}

		if (!createUniformBuffers(model))
		{
			return false;
		}
		
		if (!createDescriptorSets(model))
		{
			return false;
		}

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::unloadMaterial(Material& material)
	{
		if (material.descriptorSet != VK_NULL_HANDLE)
		{
			VkResult freeDescriptorSetResult = vkFreeDescriptorSets(m_device, m_materialsDescriptorPool, 1, &material.descriptorSet);
			validateResult(freeDescriptorSetResult, "Failed to free descriptor set");
			material.descriptorSet = VK_NULL_HANDLE;
		}

		if (material.materialBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(m_device, material.materialBuffer, nullptr);
			material.materialBuffer = VK_NULL_HANDLE;
		}
		if (material.materialBufferMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_device, material.materialBufferMemory, nullptr);
			material.materialBufferMemory = VK_NULL_HANDLE;
		}
	}

	////////////////////////////////////////////////////////////////////////

	const VulkanRenderer::TextureData& VulkanRenderer::getTexture(const std::string& textureId) const
	{
		const auto& textureItr = m_textures.find(textureId);
		ASSERT(textureItr != m_textures.end(), "Can't find texture: {}", textureId);
		if (textureItr != m_textures.end())
		{
			return textureItr->second;
		}

		const auto& defaultTextureItr = m_textures.find(DEFAULT_TEXTURE);
		ASSERT(defaultTextureItr != m_textures.end(), "Can't find default texture: {}", DEFAULT_TEXTURE);
		if (defaultTextureItr != m_textures.end())
		{
			return defaultTextureItr->second;
		}
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::loadModelFromFile(ModelData& model, const std::string& filename)
	{
		std::filesystem::path fullPath(filename);
		std::filesystem::path matDir = fullPath.parent_path();

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), matDir.string().c_str());
		if (!success) 
		{
			return false;
		}

		// Load materials
		for (const auto& mat : materials)
		{
			Material material;
			material.ambientColor = glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
			material.diffuseColor = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
			material.specularColor = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
			material.shininess = mat.shininess;

			if (!mat.diffuse_texname.empty())
			{
				std::string texturePath = (matDir / mat.diffuse_texname).string();
				if (loadTexture(texturePath))
				{
					material.diffuseTextureId = texturePath;
				}
				else
				{
					material.diffuseTextureId = m_defaultMaterial.diffuseTextureId;
				}
			}
			else
			{
				material.diffuseTextureId = m_defaultMaterial.diffuseTextureId;
				
			}

			model.materials.push_back(material);
		}

		// Load shapes and vertices
		for (const auto& shape : shapes)
		{
			SubMesh subMesh;

			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex = {};
				vertex.position = glm::vec3(
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				);

				if (index.normal_index >= 0) {
					vertex.normal = glm::vec3(
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]
					);
				}

				if (index.texcoord_index >= 0) {
					vertex.texCoord = glm::vec2(
						attrib.texcoords[2 * index.texcoord_index + 0],
						attrib.texcoords[2 * index.texcoord_index + 1]
					);
				}

				model.vertices.push_back(vertex);
				subMesh.indices.push_back(static_cast<unsigned int>(model.vertices.size() - 1));
			}

			subMesh.materialId = shape.mesh.material_ids.empty() ? 0 : shape.mesh.material_ids[0];
			model.meshes.push_back(std::move(subMesh));
		}

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::loadModel(const std::string& filename)
	{
		if (m_models.contains(filename))
		{
			return true;
		}

		ModelData modelData;
		if (!loadModelFromFile(modelData, filename))
		{
			return false;
		}

		if (!createBuffersForModel(modelData))
		{
			return false;
		}

		m_models.emplace(filename, std::move(modelData));

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::loadTexture(const std::string& filename)
	{
		if (m_textures.contains(filename))
		{
			return true;
		}

		TextureData textureData;

		bool createTextureImageResult = createTextureImage(filename, textureData);
		if (!createTextureImageResult)
		{
			return false;
		};

		std::vector<VkDescriptorSetLayout> textureLayouts(1, m_textureSetLayout);
		VkDescriptorSetAllocateInfo textureAllocateInfo{};
		textureAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		textureAllocateInfo.descriptorPool = m_texturesDescriptorPool;
		textureAllocateInfo.descriptorSetCount = 1;
		textureAllocateInfo.pSetLayouts = textureLayouts.data();

		VkResult allocateDescriptorSetResult = vkAllocateDescriptorSets(m_device, &textureAllocateInfo, &textureData.descriptorSet);
		if (!validateResult(allocateDescriptorSetResult, "Failed to allocate descriptor set"))
		{
			return false;
		}

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureData.textureImageView;
		imageInfo.sampler = m_textureSampler;

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = textureData.descriptorSet;
		descriptorWrites[0].dstBinding = 1;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

		m_textures.emplace(filename, std::move(textureData));
		return true;
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createDepthResources()
	{
		VkFormat depthFormat = findDepthFormat();

		createImage(
			m_swapChainExtent.width,
			m_swapChainExtent.height,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_depthImage,
			m_depthImageMemory
		);
		m_depthImageView = createImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createSyncObjects()
	{
		m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			VkResult createImageSemaphoreResult = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]);
			if (!validateResult(createImageSemaphoreResult, "Failed to create image available semaphore"))
			{
				return;
			}

			VkResult createRenderSemaphoreResult = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
			if (!validateResult(createRenderSemaphoreResult, "Failed to create render finished semaphore"))
			{
				return;
			}

			VkResult createFenceResult = vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]);
			if (!validateResult(createFenceResult, "Failed to create in flight fence"))
			{
				return;
			}

		}
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createDefaultMaterial()
	{
		bool loadTextureResult = loadTexture(DEFAULT_TEXTURE);
		ASSERT(loadTextureResult, "Failed to load default texture: {}", DEFAULT_TEXTURE);

		m_defaultMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
		m_defaultMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
		m_defaultMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
		m_defaultMaterial.shininess = 32.0f;
		m_defaultMaterial.diffuseTextureId = DEFAULT_TEXTURE;

		bool result = createBuffer(
			sizeof(MaterialBufferObject),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_defaultMaterial.materialBuffer,
			m_defaultMaterial.materialBufferMemory
		);

		if (!result)
		{
			return;
		}

		MaterialBufferObject mbo{};
		mbo.ambientColor = m_defaultMaterial.ambientColor;
		mbo.specularColor = m_defaultMaterial.specularColor;
		mbo.diffuseColor = m_defaultMaterial.diffuseColor;
		mbo.shininess = m_defaultMaterial.shininess;
		
		result = createDescriptorSet(m_defaultMaterial);
		if (!result)
		{
			return;
		}

		result = setBufferMemoryData(m_defaultMaterial.materialBufferMemory, &mbo, sizeof(mbo));
		if (!result)
		{
			return;
		}
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createProjectionMatrix()
	{
		float aspectRatio = (float)m_swapChainExtent.width / (float)m_swapChainExtent.height;
		m_ubo.projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);
		m_ubo.projectionMatrix[1][1] *= -1;
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createTextureSampler()
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		VkResult createSamplerResult = vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler);
		if (!validateResult(createSamplerResult, "Failed to create texture sampler"))
		{
			return;
		}
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult createBufferResult = vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer);
		if (!validateResult(createBufferResult, "Failed to create buffer"))
		{
			return false;
		}

		VkMemoryRequirements memRequirements{};
		vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		VkResult allocateMemoryResult = vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory);
		if (!validateResult(allocateMemoryResult, "Failed to allocate buffer memory"))
		{
			return false;
		}

		VkResult bindMemoryResult = vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
		if (!validateResult(bindMemoryResult, "Failed to bind buffer memory"))
		{
			return false;
		}

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::setBufferMemoryData(VkDeviceMemory memory, const void* data, VkDeviceSize size)
	{
		void* mappedData;

		VkResult mapMemoryResult = vkMapMemory(m_device, memory, 0, size, 0, &mappedData);
		if (!validateResult(mapMemoryResult, "Failed to map memory"))
		{
			return false;
		}

		memcpy(mappedData, data, size);
		vkUnmapMemory(m_device, memory);

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::createUniformBuffers(ModelData& model)
	{
		for (Material& mat: model.materials) 
		{
			bool res = createBuffer(
				sizeof(MaterialBufferObject),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				mat.materialBuffer,
				mat.materialBufferMemory
			);

			if (!res) 
			{
				return false;
			}
		}

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createCommandBuffers()
	{
		m_commandBuffers.resize(m_swapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = m_commandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

		VkResult allocateCommandBuffersResult = vkAllocateCommandBuffers(m_device, &allocateInfo, m_commandBuffers.data());
		if (!validateResult(allocateCommandBuffersResult, "Failed to allocate command buffers"))
		{
			return;
		}

	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::createImage(
		uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) 
	{

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult createImageResult = vkCreateImage(m_device, &imageInfo, nullptr, &image);
		if (!validateResult(createImageResult, "Failed to create image"))
		{
			return false;
		}

		VkMemoryRequirements memRequirements{};
		vkGetImageMemoryRequirements(m_device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		VkResult allocateMemoryResult = vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory);
		if (!validateResult(allocateMemoryResult, "Failed to allocate image memory"))
		{
			return false;
		}

		VkResult bindMemoryResult = vkBindImageMemory(m_device, image, imageMemory, 0);
		if (!validateResult(bindMemoryResult, "Failed to bind image memory"))
		{
			return false;
		}

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::createVertexBuffer(ModelData& model)
	{
		ASSERT(!model.vertices.empty(), "Model is empty");

		VkDeviceSize bufferSize = sizeof(model.vertices[0]) * model.vertices.size();

		VkBuffer stagingBuffer{};
		VkDeviceMemory stagingBufferMemory{};

		if (!createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory))
		{
			return false;
		}

		if (!setBufferMemoryData(stagingBufferMemory, model.vertices.data(), bufferSize))
		{
			return false;
		}

		if (!createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, model.vertexBuffer, model.vertexBufferMemory))
		{
			return false;
		}

		copyBuffer(stagingBuffer, model.vertexBuffer, bufferSize);

		vkDestroyBuffer(m_device, stagingBuffer, nullptr);
		vkFreeMemory(m_device, stagingBufferMemory, nullptr);

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::createIndexBuffer(ModelData& model)
	{
		for (auto& mesh : model.meshes)
		{
			VkDeviceSize bufferSize = sizeof(mesh.indices[0]) * mesh.indices.size();

			VkBuffer stagingBuffer{};
			VkDeviceMemory stagingBufferMemory{};

			if (!createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer, stagingBufferMemory))
			{
				return false;
			}

			if (!setBufferMemoryData(stagingBufferMemory, mesh.indices.data(), bufferSize))
			{
				return false;
			}

			if (!createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh.indexBuffer, mesh.indexBufferMemory))
			{
				return false;
			}

			copyBuffer(stagingBuffer, mesh.indexBuffer, bufferSize);

			vkDestroyBuffer(m_device, stagingBuffer, nullptr);
			vkFreeMemory(m_device, stagingBufferMemory, nullptr);
		}

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer);
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::createTextureImage(const std::string& filename, TextureData& texture)
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) 
		{
			return false;
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		if (!createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory))
		{
			return false;
		}

		if (!setBufferMemoryData(stagingBufferMemory, pixels, imageSize))
		{
			return false;
		}

		stbi_image_free(pixels);

		if (!createImage(texWidth, texHeight, VK_FORMAT_B8G8R8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.textureImage, texture.textureImageMemory))
		{
			return false;
		}

		transitionImageLayout(texture.textureImage, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(stagingBuffer, texture.textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		transitionImageLayout(texture.textureImage, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(m_device, stagingBuffer, nullptr);
		vkFreeMemory(m_device, stagingBufferMemory, nullptr);

		// create image view
		texture.textureImageView = createImageView(texture.textureImage, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

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
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);

		endSingleTimeCommands(commandBuffer);
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		endSingleTimeCommands(commandBuffer);
	}

	////////////////////////////////////////////////////////////////////////

	VkFormat VulkanRenderer::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}

		ASSERT(false, "Failed to find supported format!");
	}

	////////////////////////////////////////////////////////////////////////

	VkCommandBuffer VulkanRenderer::beginSingleTimeCommands()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		VkResult allocateComandBufferResult = vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);
		if (!validateResult(allocateComandBufferResult, "Failed to allocate command buffer"))
		{
			return VK_NULL_HANDLE;
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkResult beginComandBufferResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		if (!validateResult(beginComandBufferResult, "Failed to begin recording command buffer"))
		{
			return VK_NULL_HANDLE;
		}

		return commandBuffer;
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer)
	{
		VkResult endCommandBufferResult = vkEndCommandBuffer(commandBuffer);
		if (!validateResult(endCommandBufferResult, "Failed to record command buffer"))
		{
			return;
		}

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VkResult queueSubmitResult = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		if (!validateResult(queueSubmitResult, "Failed to submit command buffer"))
		{
			return;
		}
		VkResult queueWaitResult = vkQueueWaitIdle(m_graphicsQueue);
		if (!validateResult(queueWaitResult, "Failed to wait for queue"))
		{
			return;
		}

		vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
	}

	////////////////////////////////////////////////////////////////////////

	VulkanRenderer::QueueFamilyIndices VulkanRenderer::findQueueFamilies(VkPhysicalDevice dev)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

		for (int queueIdx = 0; queueIdx < queueFamilies.size(); queueIdx++) 
		{
			const VkQueueFamilyProperties& queueFamily = queueFamilies[queueIdx];

			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
			{
				indices.graphicsFamily = queueIdx;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(dev, queueIdx, m_surface, &presentSupport);

			if (presentSupport)
			{
				indices.presentFamily = queueIdx;
			}

			if (indices.isComplete())
			{
				break;
			}

			queueIdx++;
		}

		return indices;

	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::isDeviceSuitable(VkPhysicalDevice dev)
	{
		QueueFamilyIndices indices = findQueueFamilies(dev);

		bool extensionsSupported = checkDeviceExtensionSupport(dev);

		bool swapChainAdequate = false;
		if (extensionsSupported) 
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(dev);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(dev, &supportedFeatures);

		return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;

	}

	////////////////////////////////////////////////////////////////////////

	uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties{};
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type!");
	}

	////////////////////////////////////////////////////////////////////////

	VulkanRenderer::SwapChainSupportDetails VulkanRenderer::querySwapChainSupport(VkPhysicalDevice dev)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, m_surface, &details.capabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(dev, m_surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(dev, m_surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(dev, m_surface, &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(dev, m_surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	////////////////////////////////////////////////////////////////////////

	VkSurfaceFormatKHR VulkanRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{

		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return availableFormats.front();
	}

	////////////////////////////////////////////////////////////////////////

	VkPresentModeKHR VulkanRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes) 
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR || availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				return availablePresentMode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR; // Only this mode is guaranteed
	}

	////////////////////////////////////////////////////////////////////////

	VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		// TODO:implement if causes errores
		return capabilities.currentExtent;
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice dev)
	{
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	////////////////////////////////////////////////////////////////////////

	VkFormat VulkanRenderer::findDepthFormat()
	{
		return findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::setCameraProperties(const Utils::Vector3& position, const Utils::Vector3& rotation)
	{
		glm::vec3 pos = glm::vec3(position.x, position.y, position.z);

		float pitch = -rotation.x;
		float yaw = rotation.y;
		float roll = rotation.z;

		glm::vec3 forward;
		forward.x = cos(pitch) * sin(yaw);
		forward.y = sin(pitch);
		forward.z = cos(pitch) * cos(yaw);
		forward = glm::normalize(forward);

		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

		glm::vec3 right = glm::normalize(glm::cross(up, forward));

		glm::mat4 rollMatrix = glm::rotate(glm::mat4(1.0f), roll, forward);
		right = glm::vec3(rollMatrix * glm::vec4(right, 0.0f));
		up = glm::normalize(glm::cross(forward, right));

		m_ubo.viewMatrix = glm::lookAt(pos, pos + forward, up);

	}
	////////////////////////////////////////////////////////////////////////

	std::unique_ptr<IModelInstance> VulkanRenderer::createModelInstance(const std::string& filename)
	{
		auto modelInstance = std::make_unique<VulkanModelInstance>(filename);

		EntityID poolId = getPoolToUse();
		DescriptorPoolState& poolState = m_instanceDescriptorPools.getElement(poolId);

		bool createBufferResult = createBuffer(
			sizeof(UniformBufferObject),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			modelInstance->uniformBuffer,
			modelInstance->uniformBufferMemory
		);

		ASSERT(createBufferResult, "Failed to create uniform buffer for model instance, path: {}", filename);
		if (!createBufferResult)
		{
			return nullptr;
		}

		std::vector<VkDescriptorSetLayout> instanceLayouts(1, m_instanceSetLayout);
		VkDescriptorSetAllocateInfo allocateInfoMesh{};
		allocateInfoMesh.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfoMesh.descriptorPool = poolState.pool;
		allocateInfoMesh.descriptorSetCount = 1;
		allocateInfoMesh.pSetLayouts = instanceLayouts.data();

		VkResult allocateDescriptorSetResult = vkAllocateDescriptorSets(m_device, &allocateInfoMesh, &modelInstance->descriptorSet);
		if (!validateResult(allocateDescriptorSetResult, "Failed to allocate descriptor set"))
		{
			return nullptr;
		}

		modelInstance->descriptorPoolID = poolId;
		poolState.usedSets += 1;

		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = modelInstance->uniformBuffer;
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(UniformBufferObject);

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = modelInstance->descriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

		vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

		return modelInstance;
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::destroyModelInstance(IModelInstance& modelInstance)
	{
		VulkanModelInstance& vkModelInstance = (VulkanModelInstance&)modelInstance;

		if (vkModelInstance.descriptorSet != VK_NULL_HANDLE)
		{
			DescriptorPoolState& poolState = m_instanceDescriptorPools.getElement(vkModelInstance.descriptorPoolID);
			VkResult freeDescriptorSetResult = vkFreeDescriptorSets(m_device, poolState.pool, 1, &vkModelInstance.descriptorSet);
			if (!validateResult(freeDescriptorSetResult, "Failed to free descriptor set"))
			{
				return false;
			}

			// Destroy pool if it is not used anymore
			poolState.usedSets -= 1;
			if (poolState.usedSets == 0)
			{
				vkDestroyDescriptorPool(m_device, poolState.pool, nullptr);
				m_instanceDescriptorPools.removeElement(vkModelInstance.descriptorPoolID);
				m_instanceDescriptorPoolsManager.destroyEntity(vkModelInstance.descriptorPoolID);
			}

			vkModelInstance.descriptorSet = VK_NULL_HANDLE;
		}

		if (vkModelInstance.uniformBufferMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_device, vkModelInstance.uniformBufferMemory, nullptr);
			vkModelInstance.uniformBufferMemory = VK_NULL_HANDLE;
		}

		if (vkModelInstance.uniformBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(m_device, vkModelInstance.uniformBuffer, nullptr);
			vkModelInstance.uniformBuffer = VK_NULL_HANDLE;
		}

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::unloadTexture(const std::string& filename)
	{
		const auto& itr = m_textures.find(filename);
		if (itr == m_textures.end())
		{
			return true;
		}
		
		TextureData& texture = itr->second;
		if (texture.descriptorSet != VK_NULL_HANDLE)
		{
			VkResult freeDescriptorSetResult = vkFreeDescriptorSets(m_device, m_texturesDescriptorPool, 1, &texture.descriptorSet);
			if (!validateResult(freeDescriptorSetResult, "Failed to free descriptor set"))
			{
				return false;
			}
			texture.descriptorSet = VK_NULL_HANDLE;
		}

		if (texture.textureImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_device, texture.textureImageView, nullptr);
			texture.textureImageView = VK_NULL_HANDLE;
		}

		if (texture.textureImage != VK_NULL_HANDLE)
		{
			vkDestroyImage(m_device, texture.textureImage, nullptr);
			texture.textureImage = VK_NULL_HANDLE;
		}

		if (texture.textureImageMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_device, texture.textureImageMemory, nullptr);
			texture.textureImageMemory = VK_NULL_HANDLE;
		}

		m_textures.erase(itr);

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::unloadModel(const std::string& filename)
	{
		const auto& itr = m_models.find(filename);
		if (itr == m_models.end())
		{
			return true;
		}

		ModelData& modelData = itr->second;

		if (modelData.vertexBuffer != VK_NULL_HANDLE) 
		{
			vkDestroyBuffer(m_device, modelData.vertexBuffer, nullptr);
			modelData.vertexBuffer = VK_NULL_HANDLE;
		}
		if (modelData.vertexBufferMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_device, modelData.vertexBufferMemory, nullptr);
			modelData.vertexBufferMemory = VK_NULL_HANDLE;
		}

		for (SubMesh& subMesh : modelData.meshes)
		{
			if (subMesh.indexBuffer != VK_NULL_HANDLE) 
			{
				vkDestroyBuffer(m_device, subMesh.indexBuffer, nullptr);
				subMesh.indexBuffer = VK_NULL_HANDLE;
			}
			if (subMesh.indexBufferMemory != VK_NULL_HANDLE)
			{
				vkFreeMemory(m_device, subMesh.indexBufferMemory, nullptr);
				subMesh.indexBufferMemory = VK_NULL_HANDLE;
			}
		}

		for (Material& material : modelData.materials)
		{
			unloadMaterial(material);
		}

		m_models.erase(itr);

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::cleanUp()
	{
#ifdef _SHOWUI
		cleanUpUI();
#endif

		for (const std::string& modelId : Utils::getKeys(m_models))
		{
			unloadModel(modelId);
		}

		for (const std::string& textureId : Utils::getKeys(m_textures))
		{
			unloadTexture(textureId);
		}

		unloadMaterial(m_defaultMaterial);

		vkDeviceWaitIdle(m_device);
		for (auto& semaphore : m_imageAvailableSemaphores)
		{
			vkDestroySemaphore(m_device, semaphore, nullptr);
		}

		for (auto& semaphore : m_renderFinishedSemaphores)
		{
			vkDestroySemaphore(m_device, semaphore, nullptr);
		}

		for (auto& fence : m_inFlightFences)
		{
			vkDestroyFence(m_device, fence, nullptr);
		}

		vkFreeCommandBuffers(m_device, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
		vkDestroyCommandPool(m_device, m_commandPool, nullptr);

		vkDestroyDescriptorPool(m_device, m_materialsDescriptorPool, nullptr);
		vkDestroyDescriptorPool(m_device, m_texturesDescriptorPool, nullptr);

		for (const EntityID& poolId : m_instanceDescriptorPools.getIds())
		{
			DescriptorPoolState poolState = m_instanceDescriptorPools.getElement(poolId);
			vkDestroyDescriptorPool(m_device, poolState.pool, nullptr);
			ASSERT(poolState.usedSets == 0, "Descriptor pool is not empty");
		}
		m_instanceDescriptorPoolsManager.clear();
		m_instanceDescriptorPools.clear();

		vkDestroySampler(m_device, m_textureSampler, nullptr);

		if (m_depthImageView) vkDestroyImageView(m_device, m_depthImageView, nullptr);
		if (m_depthImage) vkDestroyImage(m_device, m_depthImage, nullptr);
		if (m_depthImageMemory) vkFreeMemory(m_device, m_depthImageMemory, nullptr);

		for (auto& framebuffer : m_swapChainFramebuffers) 
		{
			vkDestroyFramebuffer(m_device, framebuffer, nullptr);
		}

		for (auto& imageView : m_swapChainImageViews)
		{
			vkDestroyImageView(m_device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);

		vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
		vkDestroyRenderPass(m_device, m_renderPass, nullptr);

		vkDestroyDescriptorSetLayout(m_device, m_instanceSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_device, m_materialSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_device, m_textureSetLayout, nullptr);

		vkDestroyDevice(m_device, nullptr);

		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

		vkDestroyInstance(m_instance, nullptr);
	}

	////////////////////////////////////////////////////////////////////////

	glm::mat4 VulkanRenderer::getWorldMatrix(const Utils::Vector3& position, const Utils::Vector3& rotation, const Utils::Vector3& scale)
	{
		glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, position.z));
		glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 scaling = glm::scale(glm::mat4(1.0f), glm::vec3(scale.x, scale.y, scale.z));

		return translation * rotationX * rotationY * rotationZ * scaling;
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::validateResult(VkResult result, const std::string& message)
	{
		ASSERT(result == VK_SUCCESS, "Vulkan operation failed: {}, result code: {}", message, (int)result);
		return result == VK_SUCCESS;
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createInstance()
	{
		const std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Vulkan Renderer";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
		createInfo.ppEnabledExtensionNames = instanceExtensions.data();

		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
		if (!validateResult(result, "Failed to create Vulkan instance"))
		{
			return;
		}
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createSurface(const Window& window)
	{
		VkWin32SurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = window.getHandle();
		createInfo.hinstance = GetModuleHandle(nullptr);

		VkResult result = vkCreateWin32SurfaceKHR(m_instance, &createInfo, nullptr, &m_surface);
		if (!validateResult(result, "Failed to create window surface"))
		{
			return;
		}
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

		ASSERT(deviceCount > 0, "Failed to find Vulkan supported GPU");
		if (deviceCount == 0)
		{
			return;
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

		for (const auto& cur_device : devices) 
		{
			if (isDeviceSuitable(cur_device))
			{
				m_physicalDevice = cur_device;
				break;
			}
		}

		ASSERT(m_physicalDevice != VK_NULL_HANDLE, "Failed to find a suitable GPU");
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createLogicalDevice()
	{
		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
		createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

		VkResult deviceCreateResult = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
		if (!validateResult(deviceCreateResult, "Failed to create logical device"))
		{
			return;
		}

		vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);

	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		VkResult createSwapChainResult = vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain);
		if (!validateResult(createSwapChainResult, "Failed to create swap chain"))
		{
			return;
		}

		VkResult getSwapchainImagesResult = vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
		if (!validateResult(getSwapchainImagesResult, "Failed to get swap chain images"))
		{
			return;
		}

		m_swapChainImages.resize(imageCount);
		getSwapchainImagesResult = vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());
		if (!validateResult(getSwapchainImagesResult, "Failed to get swap chain images"))
		{
			return;
		}

		m_swapChainImageFormat = surfaceFormat.format;
		m_swapChainExtent = extent;
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createImageViews()
	{
		m_swapChainImageViews.resize(m_swapChainImages.size());

		for (uint32_t i = 0; i < m_swapChainImages.size(); i++)
		{
			m_swapChainImageViews[i] = createImageView(m_swapChainImages[i], m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VkResult createRenderPassResult = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
		if (!validateResult(createRenderPassResult, "Failed to create render pass"))
		{
			return;
		}
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding mboLayoutBinding{};
		mboLayoutBinding.binding = 0;
		mboLayoutBinding.descriptorCount = 1;
		mboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		mboLayoutBinding.pImmutableSamplers = nullptr;
		mboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 1> materialLayoutBinding = { mboLayoutBinding };
		VkDescriptorSetLayoutCreateInfo materialLayouInfo{};
		materialLayouInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		materialLayouInfo.bindingCount = static_cast<uint32_t>(materialLayoutBinding.size());
		materialLayouInfo.pBindings = materialLayoutBinding.data();

		VkResult createDescriptorSetLayoutResult = vkCreateDescriptorSetLayout(m_device, &materialLayouInfo, nullptr, &m_materialSetLayout);
		if (!validateResult(createDescriptorSetLayoutResult, "Failed to create descriptor set layout"))
		{
			return;
		}

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 0;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 1> texureLayoutBinding = { samplerLayoutBinding };
		VkDescriptorSetLayoutCreateInfo texureLayoutInfo{};
		texureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		texureLayoutInfo.bindingCount = static_cast<uint32_t>(texureLayoutBinding.size());
		texureLayoutInfo.pBindings = texureLayoutBinding.data();

		createDescriptorSetLayoutResult = vkCreateDescriptorSetLayout(m_device, &texureLayoutInfo, nullptr, &m_textureSetLayout);
		if (!validateResult(createDescriptorSetLayoutResult, "Failed to create descriptor set layout"))
		{
			return;
		}

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 1> instanceBindings = { uboLayoutBinding };
		VkDescriptorSetLayoutCreateInfo instanceLayoutInfo{};
		instanceLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		instanceLayoutInfo.bindingCount = static_cast<uint32_t>(instanceBindings.size());
		instanceLayoutInfo.pBindings = instanceBindings.data();

		createDescriptorSetLayoutResult = vkCreateDescriptorSetLayout(m_device, &instanceLayoutInfo, nullptr, &m_instanceSetLayout);
		if (!validateResult(createDescriptorSetLayoutResult, "Failed to create descriptor set layout"))
		{
			return;
		}
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createGraphicsPipeline()
	{
		auto vertShaderCode = Utils::loadBytesFromFile("vert.spv");
		auto fragShaderCode = Utils::loadBytesFromFile("frag.spv");

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";
		vertShaderStageInfo.pSpecializationInfo = nullptr; // For constants

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescription = Vertex::getAttributeDescriptions();
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_swapChainExtent.width);
		viewport.height = static_cast<float>(m_swapChainExtent.height);

		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizerState{};
		rasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerState.depthClampEnable = VK_FALSE;
		rasterizerState.rasterizerDiscardEnable = VK_FALSE;
		rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerState.lineWidth = 1.0f;
		rasterizerState.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerState.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizerState.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisamplingState{};
		multisamplingState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplingState.sampleShadingEnable = VK_FALSE;
		multisamplingState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		//    multisamplingState.minSampleShading = 1.0f;
		//    multisamplingState.pSampleMask = nullptr;
		//    multisamplingState.alphaToCoverageEnable = VK_FALSE;
		//    multisamplingState.alphaToOneEnable = VK_FALSE;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.logicOpEnable = VK_FALSE;
		colorBlendState.logicOp = VK_LOGIC_OP_COPY;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;
		colorBlendState.blendConstants[0] = 0.0f;
		colorBlendState.blendConstants[1] = 0.0f;
		colorBlendState.blendConstants[2] = 0.0f;
		colorBlendState.blendConstants[3] = 0.0f;

		VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;

		std::array<VkDescriptorSetLayout, 3> setLayouts = { m_instanceSetLayout, m_materialSetLayout, m_textureSetLayout };
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
		pipelineLayoutInfo.pSetLayouts = setLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		VkResult createPipelineResult = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
		if (!validateResult(createPipelineResult, "Failed to create pipeline layout"))
		{
			return;
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizerState;
		pipelineInfo.pMultisampleState = &multisamplingState;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.renderPass = m_renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		VkResult createGraphicsPipelineResult = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline);
		if (!validateResult(createGraphicsPipelineResult, "Failed to create graphics pipeline"))
		{
			return;
		}

		vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
		vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createFramebuffers()
	{
		m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

		for (size_t i = 0; i < m_swapChainImageViews.size(); ++i)
		{
			std::array<VkImageView, 2> attachments = { m_swapChainImageViews[i], m_depthImageView };

			VkFramebufferCreateInfo frameBufferInfo{};
			frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frameBufferInfo.renderPass = m_renderPass;
			frameBufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			frameBufferInfo.pAttachments = attachments.data();
			frameBufferInfo.width = m_swapChainExtent.width;
			frameBufferInfo.height = m_swapChainExtent.height;
			frameBufferInfo.layers = 1;

			VkResult createFrameBufferResult = vkCreateFramebuffer(m_device, &frameBufferInfo, nullptr, &m_swapChainFramebuffers[i]);
			if (!validateResult(createFrameBufferResult, "Failed to create framebuffer"))
			{
				return;
			}	
		}
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		VkResult createCommandPoolResult = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
		if (!validateResult(createCommandPoolResult, "Failed to create command pool"))
		{
			return;
		}
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::createDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 1> materialPoolSizes{};
		materialPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		materialPoolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_MATERIALS);

		VkDescriptorPoolCreateInfo materialPoolCreateInfo{};
		materialPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		materialPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(materialPoolSizes.size());
		materialPoolCreateInfo.pPoolSizes = materialPoolSizes.data();
		materialPoolCreateInfo.maxSets = MAX_MATERIALS;
		materialPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VkResult createDescriptorPoolResult = vkCreateDescriptorPool(m_device, &materialPoolCreateInfo, nullptr, &m_materialsDescriptorPool);
		if (!validateResult(createDescriptorPoolResult, "Failed to create descriptor pool"))
		{
			return;
		}

		std::array<VkDescriptorPoolSize, 1> texturesPoolSizes{};
		texturesPoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		texturesPoolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_TEXTURES);

		VkDescriptorPoolCreateInfo texturesPoolCreateInfo{};
		texturesPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		texturesPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(texturesPoolSizes.size());
		texturesPoolCreateInfo.pPoolSizes = texturesPoolSizes.data();
		texturesPoolCreateInfo.maxSets = MAX_TEXTURES;
		texturesPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		createDescriptorPoolResult = vkCreateDescriptorPool(m_device, &texturesPoolCreateInfo, nullptr, &m_texturesDescriptorPool);
		if (!validateResult(createDescriptorPoolResult, "Failed to create descriptor pool"))
		{
			return;
		}
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::createDescriptorSets(ModelData& model)
	{
		for (Material& material : model.materials)
		{
			if (!createDescriptorSet(material))
			{
				return false;
			}
		}
		return true;
	}

	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::createDescriptorSet(Material& material)
	{
		std::vector<VkDescriptorSetLayout> materialLayouts(1, m_materialSetLayout);
		VkDescriptorSetAllocateInfo materialAllocateInfo{};
		materialAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		materialAllocateInfo.descriptorPool = m_materialsDescriptorPool;
		materialAllocateInfo.descriptorSetCount = 1;
		materialAllocateInfo.pSetLayouts = materialLayouts.data();

		VkResult allocateDescriptorSetResult = vkAllocateDescriptorSets(m_device, &materialAllocateInfo, &material.descriptorSet);
		if (!validateResult(allocateDescriptorSetResult, "Failed to allocate descriptor set"))
		{
			return false;
		}

		VkDescriptorBufferInfo materialBufferInfo{};
		materialBufferInfo.buffer = material.materialBuffer;
		materialBufferInfo.offset = 0;
		materialBufferInfo.range = sizeof(MaterialBufferObject);

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = material.descriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &materialBufferInfo;

		vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::initUI()
	{
		uint32_t poolSize = 1000;
		VkDescriptorPoolSize pool_sizes[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, poolSize },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, poolSize },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, poolSize },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, poolSize },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, poolSize },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, poolSize },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, poolSize },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, poolSize },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, poolSize },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, poolSize },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, poolSize }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = poolSize * IM_ARRAYSIZE(pool_sizes);
		pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_imguiDescriptorPool);

		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo = {};
		pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		pipelineRenderingInfo.pNext = nullptr;
		pipelineRenderingInfo.colorAttachmentCount = 1;
		pipelineRenderingInfo.pColorAttachmentFormats = &surfaceFormat.format;
		pipelineRenderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
		pipelineRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = m_instance;
		init_info.PhysicalDevice = m_physicalDevice;
		init_info.Device = m_device;
		init_info.QueueFamily = indices.graphicsFamily.value();
		init_info.Queue = m_graphicsQueue;
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = m_imguiDescriptorPool;
		init_info.Subpass = 0;
		init_info.MinImageCount = imageCount;
		init_info.ImageCount = MAX_FRAMES_IN_FLIGHT;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = nullptr;
		init_info.UseDynamicRendering = true;
		init_info.PipelineRenderingCreateInfo = pipelineRenderingInfo;
		init_info.CheckVkResultFn = [](VkResult res) {validateResult(res, "UI returned error");};

		ImGui_ImplVulkan_Init(&init_info);
		ImGui_ImplVulkan_CreateFontsTexture();
	}
	
	////////////////////////////////////////////////////////////////////////

	void VulkanRenderer::cleanUpUI()
	{
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(m_device, m_imguiDescriptorPool, nullptr);
	}

	////////////////////////////////////////////////////////////////////////

	VkDescriptorPool VulkanRenderer::createInstancesDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 1> instancesPoolSizes{};
		instancesPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		instancesPoolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_MODEL_INSTANCES);

		VkDescriptorPoolCreateInfo instancesPoolCreateInfo{};
		instancesPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		instancesPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(instancesPoolSizes.size());
		instancesPoolCreateInfo.pPoolSizes = instancesPoolSizes.data();
		instancesPoolCreateInfo.maxSets = MAX_MODEL_INSTANCES;
		instancesPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VkDescriptorPool instancesDescriptorPool;
		VkResult createDescriptorPoolResult = vkCreateDescriptorPool(m_device, &instancesPoolCreateInfo, nullptr, &instancesDescriptorPool);
		if (!validateResult(createDescriptorPoolResult, "Failed to create descriptor pool"))
		{
			return VK_NULL_HANDLE;
		}

		return instancesDescriptorPool;
	}

	////////////////////////////////////////////////////////////////////////

	EntityID VulkanRenderer::getPoolToUse()
	{
		EntityID poolToUse = -1;
		for (const EntityID& poolId : m_instanceDescriptorPools.getIds())
		{
			if (m_instanceDescriptorPools.getElement(poolId).usedSets != MAX_MODEL_INSTANCES)
			{
				poolToUse = poolId;
			}
		}

		if (poolToUse == -1)
		{
			DescriptorPoolState poolState;
			poolState.usedSets = 0;
			poolState.pool = createInstancesDescriptorPool();
			EntityID newPoolId = m_instanceDescriptorPoolsManager.createEntity();
			m_instanceDescriptorPools.addElement(newPoolId, poolState);
			poolToUse = newPoolId;
		}

		return poolToUse;
	}

	////////////////////////////////////////////////////////////////////////

	VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		VkResult createShaderModuleResult = vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule);
		if (!validateResult(createShaderModuleResult, "Failed to create shader module"))
		{
			return VK_NULL_HANDLE;
		}

		return shaderModule;
	}

	////////////////////////////////////////////////////////////////////////

	VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		VkResult createImageViewResult = vkCreateImageView(m_device, &viewInfo, nullptr, &imageView);
		if (!validateResult(createImageViewResult, "Failed to create image view"))
		{
			return VK_NULL_HANDLE;
		}

		return imageView;
	}

	////////////////////////////////////////////////////////////////////////
	// VulkanModelInstance
	////////////////////////////////////////////////////////////////////////

	VulkanRenderer::VulkanModelInstance::VulkanModelInstance(const std::string& id): 
		ModelInstanceBase(id),
		descriptorSet(VK_NULL_HANDLE),
		uniformBuffer(VK_NULL_HANDLE),
		uniformBufferMemory(VK_NULL_HANDLE)
	{
	}

	////////////////////////////////////////////////////////////////////////
	// Vertex
	////////////////////////////////////////////////////////////////////////

	VkVertexInputBindingDescription VulkanRenderer::Vertex::getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	////////////////////////////////////////////////////////////////////////

	std::vector<VkVertexInputAttributeDescription> VulkanRenderer::Vertex::getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
		attributeDescriptions.resize(3);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	////////////////////////////////////////////////////////////////////////
	// QueueFamilyIndices
	////////////////////////////////////////////////////////////////////////

	bool VulkanRenderer::QueueFamilyIndices::isComplete() const
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}

	////////////////////////////////////////////////////////////////////////
}