#pragma once

#include <vk_mem_alloc.h>
#include <Grace/DescriptorGroup.hpp>

namespace Grace
{

struct ImageView;
class Image;
class Buffer;
class Sampler;

/// Permanent binding for Storage Images.
static const uint32_t STORAGE_IMAGE_BINDING = 0U;
/// Permanent binding for Sampled Images.
static const uint32_t SAMPLED_IMAGE_BINDING = 1U;
/// Permanent binding for Combined Image Samplers.
static const uint32_t COMBINED_IMAGE_SAMPLER_BINDING = 2U;
/// Permanent binding for Samplers.
static const uint32_t SAMPLER_BINDING = 3U;
/// Permanent binding for Uniform Buffers.
static const uint32_t UNIFORM_BUFFER_BINDING = 4U;

class SlotPool
{
public:
    SlotPool() = default;
    ~SlotPool() = default;

    /// Set the max size of slots
    void SetPoolSize(const uint32_t maxSize);

    /// Pushes the freed slot index to the front of the Free Slots queue
    void AppendFreeSlot(const uint32_t slot);

    /// @returns Index to first unoccupied slot.
    [[nodiscard]] uint32_t FindAvailableSlot();

private:
    uint32_t m_MaxSlots = std::numeric_limits<uint16_t>::max();
    uint32_t m_CurrentSlot = 0U;
    std::deque<uint32_t> m_FreeSlots = {};
};

/// Handles resource array ids.
///
/// When submitting a valid resource, it will be allocated a slot in the GPU resource array
/// which is then set for the resource.
///
/// This DOES NOT manage resources
class GpuResourceTable
{
public:
    GpuResourceTable() = default;
    ~GpuResourceTable() = default;

    void Initialise(VkDevice device, uint32_t maxImages, uint32_t maxSamplers, uint32_t maxBuffers);

    void Cleanup(VkDevice device);

    /// Finds and sets an available resource id slot based on images usage flags.
    void SubmitImage(Image& image);

    /// Frees and returns image's sampled and/or storage id back to the Images Slot Pool.
    void FreeImage(const Image& image);

    /// Finds and sets an available resource id slot based on images usage flags.
    void SubmitImageView(ImageView& view);

    /// Finds and sets an available resource id slot for a given sampler.
    void SubmitSampler(Sampler& sampler);

    /// Frees and returns sampler's resource id back to the Samplers Slot Pool.
    void FreeSampler(const Sampler& sampler);

    /// Finds and sets an available resource id slot for a given buffer.
    void SubmitBuffer(VmaAllocator allocator, Buffer& buffer);

    void UpdateTable();

    /// The one and only `VkDescriptorPool` required.
    VkDescriptorPool soleDescriptorPool = {};
    /// The one and only `VkDescriptorSetLayout` required.
    VkDescriptorSetLayout soleDescriptorSetLayout = {};
    /// The one and only `VkDescriptorSet` required.
    VkDescriptorSet soleDescriptorSet = {};
    /// The one and only `VkPipelineLayout` required.
    VkPipelineLayout solePipelineLayout = {};

private:
    /// The one and only `DescriptorWriter` to batch update the Sole Descriptor Set.
    DescriptorWriter m_Writer = {};

    SlotPool m_StorageImageSlots = {};
    SlotPool m_SampledImageSlots = {};
    SlotPool m_SamplerSlots = {};
    SlotPool m_UniformBufferSlots = {};
};

} // namespace Grace
