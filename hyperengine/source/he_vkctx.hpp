#pragma once

// Supress API_ENTRY macro redefinition
// Triggered in `glfw3.h`
#ifdef _WIN32
#   include <Windows.h>
#endif

#include <glad/vulkan.h>
#include <GLFW/glfw3.h>

#include <optional>
#include <utility>
#include <vector>

namespace hyperengine::vk {
    struct QueueFamilyIndices final {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails final {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    [[nodiscard]] SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    [[nodiscard]] QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
    [[nodiscard]] uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    struct Context final {
        struct CreateInfo final {
            bool enableValidationLayers = false;
            char const* applicationName = nullptr;
            uint32_t applicationVersion = 0;
            char const* engineName = nullptr;
            uint32_t engineVersion = 0;
            GLFWwindow* window = nullptr;
        };

        constexpr Context() noexcept = default;
        Context(CreateInfo const& info);
        Context(Context const&) = delete;
        Context& operator=(Context const&) = delete;
        inline Context(Context&& other) noexcept { *this = std::move(other); }
        Context& operator=(Context&& other) noexcept;
        ~Context() noexcept;

        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        VkImageView makeImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

        VkInstance mInstance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR mSurface = VK_NULL_HANDLE;
        VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;

        VkDevice mDevice = VK_NULL_HANDLE;
        VkQueue mQueueGraphics = VK_NULL_HANDLE;
        VkQueue mQueuePresent = VK_NULL_HANDLE;

        VkCommandPool mCommandPool = VK_NULL_HANDLE;
    };
}