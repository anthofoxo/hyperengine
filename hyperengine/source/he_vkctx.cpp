#include "he_vkctx.hpp"

#include <spdlog/spdlog.h>

#include <stdexcept>
#include <set>
#include <string>

namespace hyperengine::vk {
    namespace {
        std::vector<char const*> const kValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };

        std::vector<char const*> const kDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        [[nodiscard]] std::vector<VkLayerProperties> enumerateInstanceLayerProperties() {
            uint32_t count;
            vkEnumerateInstanceLayerProperties(&count, nullptr);
            std::vector<VkLayerProperties> layers(count);
            vkEnumerateInstanceLayerProperties(&count, layers.data());
            return layers;
        }

        [[nodiscard]] bool checkValidationLayerSupport() {
            std::vector<VkLayerProperties> availableLayers = hyperengine::vk::enumerateInstanceLayerProperties();

            for (std::string_view layerName : kValidationLayers) {
                bool layerFound = false;

                for (auto const& layerProperties : availableLayers) {
                    if (layerName == layerProperties.layerName) {
                        layerFound = true;
                        break;
                    }
                }

                if (!layerFound) {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] std::vector<char const*> getRequiredExtensions(bool enableValidationLayers) {
            uint32_t glfwExtensionCount = 0;
            char const** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            std::vector<char const*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

            if (enableValidationLayers) {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            return extensions;
        }

        VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
            spdlog::level::level_enum level;

            switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                level = spdlog::level::trace;
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                level = spdlog::level::debug;
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                level = spdlog::level::warn;
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                level = spdlog::level::err;
                break;
            default:
                std::unreachable();
            }

            spdlog::log(level, pCallbackData->pMessage);

            return VK_FALSE;
        }

        [[nodiscard]] bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

            std::set<std::string> requiredExtensions(kDeviceExtensions.begin(), kDeviceExtensions.end());

            for (auto const& extension : availableExtensions) {
                requiredExtensions.erase(extension.extensionName);
            }

            return requiredExtensions.empty();
        }

        [[nodiscard]] bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
            QueueFamilyIndices indices = findQueueFamilies(device, surface);

            bool extensionsSupported = checkDeviceExtensionSupport(device);

            bool swapChainAdequate = false;
            if (extensionsSupported) {
                SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
                swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
            }

            VkPhysicalDeviceFeatures supportedFeatures;
            vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

            return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
        }
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    Context::Context(CreateInfo const& info) {
        // Input validation
        assert(info.applicationName != nullptr);
        assert(info.applicationVersion != 0);
        assert(info.engineName != nullptr);
        assert(info.engineVersion != 0);
        assert(info.window != nullptr);

        // Create instance and debug messenger
        {
            if (!gladLoaderLoadVulkan(nullptr, nullptr, nullptr)) {
                throw std::runtime_error("failed to load vulkan functions");
            }

            if (info.enableValidationLayers && !checkValidationLayerSupport()) {
                throw std::runtime_error("validation layers requested, but not available!");
            }

            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = info.applicationName;
            appInfo.applicationVersion = info.applicationVersion;
            appInfo.pEngineName = info.engineName;
            appInfo.engineVersion = info.engineVersion;
            appInfo.apiVersion = VK_API_VERSION_1_1;

            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            auto extensions = getRequiredExtensions(info.enableValidationLayers);
            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();

            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
            if (info.enableValidationLayers) {
                createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
                createInfo.ppEnabledLayerNames = kValidationLayers.data();

                debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                debugCreateInfo.pfnUserCallback = &debugCallback;

                createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
            }
            else {
                createInfo.enabledLayerCount = 0;
                createInfo.pNext = nullptr;
            }

            if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS) {
                throw std::runtime_error("failed to create instance!");
            }

            if (!gladLoaderLoadVulkan(mInstance, nullptr, nullptr)) {
                throw std::runtime_error("failed to load vulkan functions");
            }

            if (info.enableValidationLayers) {
                if (vkCreateDebugUtilsMessengerEXT(mInstance, &debugCreateInfo, nullptr, &mDebugMessenger) != VK_SUCCESS) {
                    throw std::runtime_error("failed to set up debug messenger!");
                }
            }
        }
        // End create instance and debug messenger

        if (glfwCreateWindowSurface(mInstance, info.window, nullptr, &mSurface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }

        // Physical device selection
        {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);

            if (deviceCount == 0) {
                throw std::runtime_error("failed to find GPUs with Vulkan support!");
            }

            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

            for (auto const& device : devices) {
                if (isDeviceSuitable(device, mSurface)) {
                    mPhysicalDevice = device;
                    break;
                }
            }

            if (mPhysicalDevice == VK_NULL_HANDLE) {
                throw std::runtime_error("failed to find a suitable GPU!");
            }

            if (!gladLoaderLoadVulkan(mInstance, mPhysicalDevice, nullptr)) {
                throw std::runtime_error("failed to reload vulkan functions");
            }
        }
        // End physical device selection

        // Logical device creation
        {
            QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice, mSurface);
            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

            float queuePriority = 1.0f;
            for (uint32_t queueFamily : uniqueQueueFamilies) {
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

            createInfo.enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size());
            createInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

            if (info.enableValidationLayers) {
                createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
                createInfo.ppEnabledLayerNames = kValidationLayers.data();
            }
            else {
                createInfo.enabledLayerCount = 0;
            }

            if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS) {
                throw std::runtime_error("failed to create logical device!");
            }

            vkGetDeviceQueue(mDevice, indices.graphicsFamily.value(), 0, &mQueueGraphics);
            vkGetDeviceQueue(mDevice, indices.presentFamily.value(), 0, &mQueuePresent);

            if (!gladLoaderLoadVulkan(mInstance, mPhysicalDevice, mDevice)) {
                throw std::runtime_error("failed to reload vulkan functions");
            }
        }
        // End logical device creation
    }

    Context& Context::operator=(Context&& other) noexcept {
        std::swap(mInstance, other.mInstance);
        std::swap(mDebugMessenger, other.mDebugMessenger);
        std::swap(mSurface, other.mSurface);
        std::swap(mPhysicalDevice, other.mPhysicalDevice);
        std::swap(mDevice, other.mDevice);
        std::swap(mQueueGraphics, other.mQueueGraphics);
        std::swap(mQueuePresent, other.mQueuePresent);
        return *this;
    }

    Context::~Context() noexcept {
        if (mInstance) {
            vkDestroyDevice(mDevice, nullptr);
            vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
            vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
            vkDestroyInstance(mInstance, nullptr);
            gladLoaderUnloadVulkan();
        }
    }
}