#pragma once

#include "he_vkctx.hpp"

#include <utility>

namespace hyperengine::vk {
    struct Texture final {
        struct CreateInfo final {
            hyperengine::vk::Context& context;
            uint32_t width = 0, height = 0;
            void const* pixels = nullptr;
            uint32_t mipLevels = 0;
        };

        constexpr Texture() noexcept = default;
        Texture(CreateInfo const& info);
        Texture(Texture const&) = delete;
        Texture& operator=(Texture const&) = delete;
        inline Texture(Texture&& other) noexcept { *this = std::move(other); }
        Texture& operator=(Texture&& other) noexcept;
        ~Texture() noexcept;

        VkDevice mDevice = VK_NULL_HANDLE;
        VkDeviceMemory mMemory = VK_NULL_HANDLE;
        VkImage mImage = VK_NULL_HANDLE;
        VkImageView mView = VK_NULL_HANDLE;
        VkSampler mSampler = VK_NULL_HANDLE;
    };
}