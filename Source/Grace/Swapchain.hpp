#pragma once

#include <deque>
#include <vector>

#include <vulkan/vulkan.h>
#include <Grace/Image.hpp>

namespace Grace
{

class Device;

enum class SwapchainStatus : uint8_t
{
    /// Swapchain is in the optimal state to be presented
    Success,
    /// Swapchain is either Out Of Date or Suboptimal and should be resized/reconstructed
    ShouldResize,
    /// Swapchain's associated VkSurfaceKHR was lost and needs reconstruction, as does the VkSwapchainKHR
    Failure,
    /// Swapchain's status is not yet known
    Unknown
};

struct FrameSyncGroup
{
    VkSemaphore acquireSemaphore = {};
    VkSemaphore presentSemaphore = {};
    VkFence inFlightFence = {};
    uint32_t imageIndex = ~0U;
};

class Swapchain
{
public:
    /// @brief Creates Swapchain and all related Vulkan objects.
    void Create(Device* device, VkExtent2D imageExtent);

    /// @brief Free all resources used by this instance.
    void Cleanup(Device* device);

    FrameSyncGroup& AcquireNextImage(Device* device, VkExtent2D imageExtent);

    [[nodiscard]] const FrameSyncGroup& GetRecentFrameSyncGroup() const;

    [[nodiscard]] const VkSwapchainKHR& GetVkHandle() const;

    [[nodiscard]] SwapchainStatus GetStatus() const;

    [[nodiscard]] VkFormat GetFormat() const;

    [[nodiscard]] const Image& GetRecentAcquiredImage() const;

private:
    VkSurfaceFormatKHR SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR SelectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D SelectSwapExtent(VkExtent2D imageExtent, const VkSurfaceCapabilitiesKHR& capabilities);

private:
    VkSwapchainKHR m_Swapchain = {};
    std::vector<Image> m_Images = {};
    std::vector<FrameSyncGroup> m_ImageAcquiredSyncStructs = {};
    uint32_t m_ImageAcquiredCycleIndex = 0U;
    SwapchainStatus m_SwapchainStatus = SwapchainStatus::Unknown;
};

} // namespace Grace
