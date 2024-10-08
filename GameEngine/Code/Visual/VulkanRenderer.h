#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include "IRenderer.h"

namespace Engine::Visual
{
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    class VulkanRenderer : public IRenderer
    {
    public:

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
            VkDescriptorSet descriptorSet;

            VkBuffer uniformBuffer;
            VkDeviceMemory uniformBufferMemory;
        };

        struct Material
        {
            glm::vec3 ambientColor;
            glm::vec3 diffuseColor;
            glm::vec3 specularColor;
            float shininess;
            VkImage textureImage;
            VkDeviceMemory textureImageMemory;
            VkImageView textureImageView;
        };

        struct Model : public AbstractModel
        {
            std::vector<SubMesh> meshes;
            std::vector<Vertex> vertices;
            std::vector<Material> materials;
            glm::mat4 worldMatrix;

            VkBuffer vertexBuffer;
            VkDeviceMemory vertexBufferMemory;

			size_t GetVertexCount() const override 
            {
				return vertices.size();
			}
        };

        struct UniformBufferObject
        {
            glm::mat4 worldMatrix;
            glm::mat4 viewMatrix;
            glm::mat4 projectionMatrix;

            glm::vec3 ambientColor;
            float shininess; 

            glm::vec3 diffuseColor;
            float padding1;

            glm::vec3 specularColor;
            float padding2;
        };

        void init(const Window& window) override;
        void clearBackground(float r, float g, float b, float a) override;
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

        static const int MAX_MESHES_NUM = 1000;

        VkInstance instance{};
        VkDebugUtilsMessengerEXT debugMessenger{};
        VkSurfaceKHR surface{};

        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device{};

        VkQueue graphicsQueue{};
        VkQueue presentQueue{};

        VkSwapchainKHR swapChain{};
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat{};
        VkExtent2D swapChainExtent{};
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkFramebuffer> swapChainFramebuffers;

        VkRenderPass renderPass{};
        VkDescriptorSetLayout descriptorSetLayout{};
        VkPipelineLayout pipelineLayout{};
        VkPipeline graphicsPipeline{};

        VkCommandPool commandPool{};

        VkImage depthImage{};
        VkDeviceMemory depthImageMemory{};
        VkImageView depthImageView{};

        VkSampler textureSampler{};

        VkDescriptorPool descriptorPool{};

        std::vector<VkCommandBuffer> commandBuffers;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;
        uint32_t imageIndex = 0;

        Material defaultMaterial;

        UniformBufferObject ubo{};

        void createInstance();
        void createSurface(const Window& window);
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void createDescriptorSetLayout();
        void createDescriptorPool();
        void createDescriptorSets(SubMesh& mesh);
        void createGraphicsPipeline();
        void createFramebuffers();
        void createCommandPool();
        void createDepthResources();
        void createSyncObjects();
        void createTextureSampler();
        void createDefaultMaterial();

        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer&
            buffer, VkDeviceMemory& bufferMemory);

        void createUniformBuffers(Model& model);
        void createCommandBuffers();

        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties, VkImage& img,
            VkDeviceMemory& imageMemory);

        void createVertexBuffer(Model& model);
        void createIndexBuffer(Model& model);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        void createTextureImage(const std::string& filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void createTextureImageView(VkImageView& imageView, const VkImage& image);

        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
            VkImageTiling tiling, VkFormatFeatureFlags features);

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

    };
}