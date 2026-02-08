#include <Grace/Swapchain.hpp>

#include <algorithm>
#include <string>
#include <cassert>
#include <limits>

#ifdef NO_GLFW
#include <glfw/glfw3.h>
#endif

#include <Grace/Context.hpp>
#include <Grace/DebugReporter.hpp>
#include <Grace/HelperFunctions.hpp>
#include <Private/Grace/ScratchVector.hpp>

namespace Grace
{

const VkSwapchainKHR& Swapchain::GetVkHandle() const
{
    return mSwapchain;
}

SwapchainStatus Swapchain::GetStatus() const
{
    return mSwapchainStatus;
}

const Format& Swapchain::GetFormat() const
{
    // All images have the same format
    return mDevice->GetImage(mImages[0]).GetFormat();
}

ImageHandle Swapchain::GetRecentAcquiredImage() const
{
    const FrameSyncGroup& sync = GetRecentFrameSyncGroup();
    assert(sync.imageIndex != ~0U);
    return mImages[sync.imageIndex];
}

void Swapchain::Create(VkExtent2D imageExtent)
{
    VkPhysicalDevice physicalDevice = mDevice->GetPhysicalDevice();
    VkSurfaceKHR surfaceKHR = mDevice->GetSurface();

    const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice, surfaceKHR);

    const VkSurfaceFormatKHR surfaceFormat = SelectSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = SelectSwapPresentMode(swapChainSupport.presentModes);

    if (!mVSyncOn)
    {
        presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    VkExtent2D extent = SelectSwapExtent(imageExtent, swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    // One more than image count to be safe
    mImageAcquiredSyncStructs.resize(imageCount + 1);

    // Make sure not to exceed max image count, 0 means no limit
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = surfaceKHR,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = true,
        .oldSwapchain = mSwapchain,
    };

    std::array queueFamilyIndices = { mDevice->GetQueueFamilyIndex(QueueFamily::Graphics),
                                      mDevice->GetQueueFamilyIndex(QueueFamily::Present) };

    if (mDevice->GetQueueFamilyIndex(QueueFamily::Graphics) != mDevice->GetQueueFamilyIndex(QueueFamily::Present))
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    VkSwapchainKHR tempSwapchain = nullptr;
    DebugReporter::Check(vkCreateSwapchainKHR(mDevice->GetVkHandle(), &createInfo, nullptr, &tempSwapchain));
    assert(tempSwapchain != nullptr);

    if (mSwapchain != nullptr)
    {
        Cleanup();
    }

    mSwapchain = tempSwapchain;
    AssignDebugName<VkSwapchainKHR>(mDevice->GetVkHandle(), mSwapchain, "Grace::SwapchainKHR");

    ScratchVector<VkImage> tempImages = {};
    vkGetSwapchainImagesKHR(mDevice->GetVkHandle(), mSwapchain, &imageCount, nullptr);
    tempImages.resize(imageCount);
    mImages.resize(imageCount);
    vkGetSwapchainImagesKHR(mDevice->GetVkHandle(), mSwapchain, &imageCount, tempImages.data());

    for (size_t i = 0; i < tempImages.size(); ++i)
    {
        const std::string name = "Grace::SwapchainImage::" + std::to_string(i);
        mImages[i] = mDevice->CreateSwapchainImage(tempImages[i],
                                                   {
                                                       .name = name.c_str(),
                                                       .dimensions = { extent.width, extent.height, 1 },
                                                       .format = static_cast<Format>(surfaceFormat.format),
                                                       .usage = ImageUsage::ColorAttachment | ImageUsage::TransferSrc,
                                                       .mipmapped = false,
                                                   });
    }

    for (uint32_t i = 0; i < mImageAcquiredSyncStructs.size(); ++i)
    {
        FrameSyncGroup& sync = mImageAcquiredSyncStructs[i];

        const std::string acquireSemaphoreDebugName = "Grace::Semaphore::Acquire::" + std::to_string(i);
        sync.acquireSemaphore = mDevice->CreateBinarySemaphore({ .name = acquireSemaphoreDebugName.c_str() });

        const std::string presentSemaphoreDebugName = "Grace::Semaphore::Present::" + std::to_string(i);
        sync.presentSemaphore = mDevice->CreateBinarySemaphore({ .name = presentSemaphoreDebugName.c_str() });
    }
}

void Swapchain::Cleanup()
{
    // Destroys VkSwapchain and VkImages
    vkDestroySwapchainKHR(mDevice->GetVkHandle(), mSwapchain, nullptr);

    for (ImageHandle& imageHandle : mImages)
    {
        mDevice->FreeImage(imageHandle);
    }
}

Swapchain::~Swapchain()
{
    Cleanup();
}

Swapchain::Swapchain(Device* pDevice, VkExtent2D imageExtent, bool vsync) : mDevice(pDevice), mVSyncOn(vsync)
{
    assert(!mDevice->IsNull());

    Create(imageExtent);
}

FrameSyncGroup& Swapchain::AcquireNextImage(VkExtent2D imageExtent)
{
    // Advance the cycle index
    mImageAcquiredCycleIndex = (mImageAcquiredCycleIndex + 1) % (mImageAcquiredSyncStructs.size() - 1);
    // Get FrameSyncGroup instance, that is not currently in use, from the cycle
    FrameSyncGroup& frameSync = mImageAcquiredSyncStructs[mImageAcquiredCycleIndex];
    const BinarySemaphore& acqSem = mDevice->GetBinarySemaphore(frameSync.acquireSemaphore);

    // Acquire an image from the swap chain
    VkResult result = vkAcquireNextImageKHR(
        mDevice->GetVkHandle(), mSwapchain, UINT64_MAX, acqSem.GetVkSemaphore(), nullptr, &frameSync.imageIndex);

    // Check if swap chain is still adequate to present
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        Create(imageExtent);
        mSwapchainStatus = SwapchainStatus::ShouldResize;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        mSwapchainStatus = SwapchainStatus::Failure;
        assert("Failed to acquire swap chain image!");
    }

    mSwapchainStatus = SwapchainStatus::Success;
    return frameSync;
}

const FrameSyncGroup& Swapchain::GetRecentFrameSyncGroup() const
{
    return mImageAcquiredSyncStructs[mImageAcquiredCycleIndex];
}

VkSurfaceFormatKHR Swapchain::SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    assert(availableFormats.size() > 0);

    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB
            && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Swapchain::SelectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        // Triple buffer mode
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::SelectSwapExtent(VkExtent2D imageExtent, const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    imageExtent.width =
        std::clamp(imageExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    imageExtent.height =
        std::clamp(imageExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return imageExtent;
}

} // namespace Grace
