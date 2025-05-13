#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "IRenderer.h"
#include "Utils/SparseSet.h"
#include "Managers/EntitiesManager.h"

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

		void startUIRender() override;
		void endUIRender() override;
        void render() override;

        bool loadModel(const std::string& filename) override;
        bool loadTexture(const std::string& filename) override;

        void setCameraProperties(const Utils::Vector3& position, const Utils::Vector3& rotation) override;
        std::unique_ptr<IModelInstance> createModelInstance(const std::string& filename) override;

        bool destroyModelInstance(IModelInstance& modelInstance) override;
        bool unloadTexture(const std::string& filename) override;
        bool unloadModel(const std::string& filename) override;

        void cleanUp() override;

    private:

        struct Vertex
        {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 texCoord;

            static VkVertexInputBindingDescription getBindingDescription();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
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
            VkDescriptorSet descriptorSet;
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
			EntityID descriptorPoolID;
        };

        struct QueueFamilyIndices
        {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;
            [[nodiscard]] bool isComplete() const;
        };

        struct SwapChainSupportDetails
        {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        struct DescriptorPoolState
        {
			VkDescriptorPool pool;
			uint32_t usedSets;
        };

    private:
        static glm::mat4 getWorldMatrix(const Utils::Vector3& position, const Utils::Vector3& rotation, const Utils::Vector3& scale);
        static inline bool validateResult(VkResult result, const std::string& message);

        // Init methods
        void createInstance();
        void createSurface(const Window& window);
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void createDescriptorSetLayout();
        void createGraphicsPipeline();
        void createCommandPool();
        void createDepthResources();
        void createFramebuffers();
        void createDescriptorPool();

        VkDescriptorPool createInstancesDescriptorPool();
        EntityID getPoolToUse();

        void createSyncObjects();
        void createCommandBuffers();
        void createTextureSampler();
        void createProjectionMatrix();
        void createDefaultMaterial();

		// Model loading methods
        const TextureData& getTexture(const std::string& textureId) const;
        bool loadModelFromFile(ModelData& model, const std::string& filename);
        bool createBuffersForModel(ModelData& model);
        void unloadMaterial(Material& material);

        bool createUniformBuffers(ModelData& model);
        bool createDescriptorSets(ModelData& model);
        bool createVertexBuffer(ModelData& model);
        bool createIndexBuffer(ModelData& model);
        bool createDescriptorSet(Material& material);
        void initUI();
        void cleanUpUI();


        // Memory utils
        bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        bool setBufferMemoryData(VkDeviceMemory memory, const void* data, VkDeviceSize size);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        bool createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& img,
                        VkDeviceMemory& imageMemory);
        bool createTextureImage(const std::string& filename, TextureData& texture);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

		// Vulkan utils
        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        VkShaderModule createShaderModule(const std::vector<char>& code);

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

    private:

        static const int MAX_MODEL_INSTANCES = 500;
        static const int MAX_MATERIALS = 80;
        static const int MAX_TEXTURES = 80;

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
        VkDescriptorSetLayout m_materialSetLayout{};
        VkDescriptorSetLayout m_textureSetLayout{};

        VkPipelineLayout m_pipelineLayout{};
        VkPipeline m_graphicsPipeline{};

        VkCommandPool m_commandPool{};

        VkImage m_depthImage{};
        VkDeviceMemory m_depthImageMemory{};
        VkImageView m_depthImageView{};

        VkSampler m_textureSampler{};

        EntitiesManager m_instanceDescriptorPoolsManager;
		Utils::SparseSet<DescriptorPoolState, EntityID> m_instanceDescriptorPools;

        VkDescriptorPool m_materialsDescriptorPool{};
        VkDescriptorPool m_texturesDescriptorPool{};
		VkDescriptorPool m_imguiDescriptorPool{};

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