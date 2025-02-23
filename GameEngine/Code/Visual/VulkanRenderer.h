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

        void init(const Window& window) override;
        void clearBackground(float r, float g, float b, float a) override;

        void draw(
            const IModelInstance& model,
            const Utils::Vector3& position,
            const Utils::Vector3& rotation,
            const Utils::Vector3& scale) override;
        void render() override;

        bool loadModel(const std::string& filename) override;
        bool loadTexture(const std::string& filename) override;

        void setCameraProperties(const Utils::Vector3& position, const Utils::Vector3& rotation) override;
        std::unique_ptr<IModelInstance> createModelInstance(const std::string& filename) override;

        void destroyModelInstance(IModelInstance& modelInstance) override;
        void unloadTexture(const std::string& filename) override;
        void unloadModel(const std::string& filename) override;
        void cleanUp() override;

    private:
        struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;

            [[nodiscard]] bool isComplete() const {
                return graphicsFamily.has_value() && presentFamily.has_value();
            }
        };

        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        struct Vertex
        {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 texCoord;

            static VkVertexInputBindingDescription getBindingDescription() {
                VkVertexInputBindingDescription bindingDescription{};
                bindingDescription.binding = 0;
                bindingDescription.stride = sizeof(Vertex);
                bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                return bindingDescription;
            }

            static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
                std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

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
        };

        struct SubMesh
        {
            std::vector<uint32_t> indices;
            int materialId;

            VkBuffer indexBuffer;
            VkDeviceMemory indexBufferMemory;
        };

        struct Material
        {
            glm::vec3 ambientColor;
            glm::vec3 diffuseColor;
            glm::vec3 specularColor;
            float shininess;

            VkBuffer materialBuffer;
            VkDeviceMemory materialBufferMemory;

            VkDescriptorSet descriptorSet;

            std::string diffuseTextureId;
        };

        struct TextureData
        {
            VkImage textureImage;
            VkDeviceMemory textureImageMemory;
            VkImageView textureImageView;

        };

        struct ModelData
        {
            std::vector<SubMesh> meshes;
            std::vector<Vertex> vertices;
            std::vector<Material> materials;

            VkBuffer vertexBuffer;
            VkDeviceMemory vertexBufferMemory;
        };

        struct UniformBufferObject
        {
            glm::mat4 worldMatrix;
            glm::mat4 viewMatrix;
            glm::mat4 projectionMatrix;
        };

        struct MaterialBufferObject
        {
            glm::vec3 ambientColor;
            float shininess;

            glm::vec3 diffuseColor;
            float padding1;

            glm::vec3 specularColor;
            float padding2;
        };

        class VulkanModelInstance : public ModelInstanceBase
        {
        public:
            VulkanModelInstance(const std::string& id);

            VkBuffer uniformBuffer;
            VkDeviceMemory uniformBufferMemory;
            VkDescriptorSet descriptorSet;
        };

    private:
        static glm::mat4 getWorldMatrix(const Utils::Vector3& position, const Utils::Vector3& rotation, const Utils::Vector3& scale);

        void createInstance();
        void createSurface(const Window& window);
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void createDescriptorSetLayout();
        void createDescriptorPool();
        bool createDescriptorSets(ModelData& model);
        bool createDescriptorSet(Material& material);
        void createGraphicsPipeline();
        void createFramebuffers();
        void createCommandPool();
        void createDepthResources();
        void createSyncObjects();
        void createTextureSampler();
        void createDefaultMaterial();

        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

        void createUniformBuffers(ModelData& model);
        void createCommandBuffers();

        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& img, VkDeviceMemory& imageMemory);

        void createVertexBuffer(ModelData& model);
        void createIndexBuffer(ModelData& model);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        bool createTextureImage(const std::string& filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void createTextureImageView(VkImageView& imageView, const VkImage& image);

        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        VkShaderModule createShaderModule(const std::vector<char>& code);
        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev);
        bool isDeviceSuitable(VkPhysicalDevice dev);

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        static bool checkDeviceExtensionSupport(VkPhysicalDevice dev);
        VkFormat findDepthFormat();

        const TextureData& getTexture(const std::string& textureId) const;
        bool loadModelFromFile(ModelData& model, const std::string& filename);
        bool createBuffersForModel(ModelData& model);

    private:

        static const int MAX_MESHES_NUM = 50;
        static const int MAX_FRAMES_IN_FLIGHT = 3;
        static inline const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        VkInstance m_instance{};
        VkSurfaceKHR m_surface{};

        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device{};

        VkQueue m_graphicsQueue{};
        VkQueue m_presentQueue{};

        VkSwapchainKHR m_swapChain{};
        std::vector<VkImage> m_swapChainImages;
        VkFormat m_swapChainImageFormat{};
        VkExtent2D m_swapChainExtent{};
        std::vector<VkImageView> m_swapChainImageViews;
        std::vector<VkFramebuffer> m_swapChainFramebuffers;

        VkRenderPass m_renderPass{};

        VkDescriptorSetLayout m_instanceSetLayout{};
        VkDescriptorSetLayout m_meshSetLayout{};

        VkPipelineLayout m_pipelineLayout{};
        VkPipeline m_graphicsPipeline{};

        VkCommandPool m_commandPool{};

        VkImage m_depthImage{};
        VkDeviceMemory m_depthImageMemory{};
        VkImageView m_depthImageView{};

        VkSampler m_textureSampler{};

        VkDescriptorPool m_descriptorPool{};

        std::vector<VkCommandBuffer> m_commandBuffers;

        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;

        std::vector<VkFence> m_inFlightFences;
        std::vector<VkFence> m_imagesInFlight;

        uint32_t m_imageIndex = 0;
        uint32_t m_currentImageInFlight = 0;

        Material m_defaultMaterial;

        UniformBufferObject m_ubo{};

        std::unordered_map<std::string, ModelData> m_models;
        std::unordered_map <std::string, TextureData> m_textures;

    };
}