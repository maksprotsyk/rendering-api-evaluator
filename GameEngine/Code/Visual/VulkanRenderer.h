#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include "IRenderer.h"

namespace Engine::Visual
{
    class VulkanRenderer : public IRenderer
    {
    public:
        struct Vertex
        {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 texCoord;
        };

        struct SubMesh
        {
            std::vector<uint32_t> indices;
            VkBuffer indexBuffer;
            VkDeviceMemory indexBufferMemory;
            int materialId;
        };

        struct Material
        {
            glm::vec3 ambientColor;
            glm::vec3 diffuseColor;
            glm::vec3 specularColor;
            float shininess;
            VkImageView diffuseTexture;
        };

        struct Model : public AbstractModel
        {
            std::vector<SubMesh> meshes;
            VkBuffer vertexBuffer;
            std::vector<Vertex> vertices;
            std::vector<Material> materials;
            VkDescriptorSet descriptorSet;
            VkDeviceMemory vertexBufferMemory;
        };

        void init(const Window& window) override;
        void clearBackground() override;
        void draw(const AbstractModel& model) override;
        void render() override;

        std::unique_ptr<AbstractModel> createModel() override;
        void createBuffersFromModel(AbstractModel& model) override;
        void loadModel(AbstractModel& model, const std::string& filename) override;

        void setCameraProperties(const Utils::Vector3& position, const Utils::Vector3& rotation) override;
        void transformModel(
            AbstractModel& model,
            const Utils::Vector3& position,
            const Utils::Vector3& rotation,
            const Utils::Vector3& scale) override;

    private:
        // Vulkan components
        VkInstance instance;
        VkDevice device;
        VkPhysicalDevice physicalDevice;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        VkRenderPass renderPass;
        VkPipeline graphicsPipeline;
        VkPipelineLayout pipelineLayout;
        VkCommandPool commandPool;
        VkDescriptorPool descriptorPool;
        VkCommandBuffer commandBuffer;
        VkExtent2D swapChainExtent;
        VkFormat swapChainImageFormat;
        VkDescriptorSetLayout descriptorSetLayout;
        std::vector<VkFramebuffer> framebuffers;
        uint32_t imageIndex = 0;

        void createInstance();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createSwapChain(const Window& window);
        void createRenderPass();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffer();
        void createDescriptorPool();
        VkShaderModule createShaderModule(const std::vector<char>& code);
        void loadTexture(VkImageView& textureImageView, const std::string& texturePath);

        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    };
}