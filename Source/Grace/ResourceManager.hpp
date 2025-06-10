#pragma once

#include <vector>
#include <queue>
#include <cassert>

#include <Grace/Buffer.hpp>
#include <Grace/Image.hpp>
#include <Grace/Sampler.hpp>
#include <Grace/PipelineGroup.hpp>
#include <Grace/HandleTypes.hpp>

namespace Grace
{

template <typename Res>
struct RegistryEntry
{
    template <typename... Args>
    RegistryEntry(const uint32_t Validator, Args&&... args)
    {
        validator = Validator;
        resource.Create(std::forward<Args>(args)...);
    }

    Res resource = {};
    uint32_t validator = 0U;
};

template <typename Res>
class Registry
{
public:
    Registry() = default;
    ~Registry() = default;

    inline std::vector<RegistryEntry<Res>>& GetAll()
    {
        return m_Registry;
    }

    template <typename... Args>
    inline Handle<Res> Register(Args&&... args)
    {
        // Use free slots if any available
        if (m_FreeSlots.size() > 0)
        {
            Handle<Res> newHandle = m_FreeSlots.front();
            m_FreeSlots.pop();
            m_Registry[newHandle.handle].validator = newHandle.validator;
            m_Registry[newHandle.handle].resource.Create(std::forward<Args>(args)...);
            return newHandle;
        }

        uint32_t newValidator = m_Validator++;
        m_Registry.emplace_back(newValidator, std::forward<Args>(args)...);

        const uint32_t index = static_cast<uint32_t>(m_Registry.size() - 1);
        return Handle<Res>(index, newValidator);
    }

    inline Res& Get(const Handle<Res>& resourceHandle)
    {
        assert(resourceHandle.HasValidHandle() && "Handle is invalid.");
        const bool handleInRange = resourceHandle.handle < m_Registry.size();
        assert(handleInRange && "Handle is not in range.");
        const bool isValidSlot = m_Registry[resourceHandle.handle].validator == resourceHandle.validator;
        assert(isValidSlot && "Handle validator does not match validator of the slot it's in.");

        return m_Registry[resourceHandle.handle].resource;
    }

    template <typename... Args>
    inline void Free(Handle<Res>& resourceHandle, Args&&... args)
    {
        assert(resourceHandle.HasValidHandle() && "Handle is invalid.");
        const bool handleInRange = resourceHandle.handle < m_Registry.size();
        assert(handleInRange && "Handle is not in range.");
        const bool isValidSlot = m_Registry[resourceHandle.handle].validator == resourceHandle.validator;
        assert(isValidSlot && "Handle validator does not match validator of the slot it's in.");

        m_Registry[resourceHandle.handle].resource.Cleanup(std::forward<Args>(args)...);
        m_Registry[resourceHandle.handle].validator = INVALID_VALIDATOR;

        // Slot is freed up and can be reused for the next resource created
        m_FreeSlots.emplace(resourceHandle.handle, resourceHandle.validator);

        // Invalidate resourceHandle
        resourceHandle.handle = INVALID_HANDLE;
        resourceHandle.validator = INVALID_VALIDATOR;
    }

    template <typename... Args>
    inline void FreeAll(Args&&... args)
    {
        for (auto& slot : m_Registry)
        {
            if (slot.validator != INVALID_VALIDATOR)
                slot.resource.Cleanup(std::forward<Args>(args)...);
        }
    }

private:
    std::vector<RegistryEntry<Res>> m_Registry = {};
    std::queue<Handle<Res>> m_FreeSlots = {};
    uint32_t m_Validator = 0;
};

/// Handles all graphics resources through registry containers.
///
/// Should call `FreeAllResources` on shutdown to automate cleanup.
///
/// Each `RegistryEntry` of the registry containers has an assigned `validator` which is used
/// to validate the slot against the `ResourceHandle` that points to it. If the validation
/// fails, the slot and handle are incorrectly paired so the operation, free or fetch, terminates.
class ResourceManager
{
public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    /// Free any remaining resources
    void FreeAllResources(VkDevice device, VmaAllocator allocator);

    /// Constructs a new Image at the first free slot in the Image's Registry
    [[nodiscard]] ImageHandle CreateImage(VkDevice device, VmaAllocator allocator, const ImageDesc& desc);

    /// Fetches the Image that corresponds to `handle` from the Image's Registry
    [[nodiscard]] Image& GetImage(const ImageHandle& handle);

    /// Fetches ALL Images found in the Image's Registry
    [[nodiscard]] std::vector<RegistryEntry<Image>>& GetAllImages();

    /// Frees the Image that corresponds to `handle` from the Image's Registry
    /// Resets `handle` to be invalid
    void FreeImage(VkDevice device, VmaAllocator allocator, ImageHandle& handle);

    /// Constructs a new Buffer at the first free slot in the Buffer's Registry
    [[nodiscard]] BufferHandle CreateBuffer(VkDevice device, VmaAllocator allocator, const BufferDesc& desc);

    /// Fetches the Buffer that corresponds to `handle` from the Buffer's Registry
    [[nodiscard]] Buffer& GetBuffer(const BufferHandle& handle);

    /// Frees the Buffer that corresponds to `handle` from the Buffer's Registry
    void FreeBuffer(VkDevice device, VmaAllocator allocator, BufferHandle& handle);

    /// Constructs a new Sampler at the first free slot in the Sampler's Registry
    [[nodiscard]] SamplerHandle CreateSampler(VkDevice device, const SamplerDesc& desc);

    /// Fetches the Sampler that corresponds to `handle` from the Sampler's Registry
    [[nodiscard]] Sampler& GetSampler(const SamplerHandle& handle);

    /// Frees the Sampler that corresponds to `handle` from the Sampler's Registry
    void FreeSampler(VkDevice device, SamplerHandle& handle);

    /// Constructs a new Pipeline at the first free slot in the Pipeline's Registry
    [[nodiscard]] PipelineHandle CreatePipeline(VkDevice device, const PipelineDesc& desc);

    /// Fetches the Pipeline that corresponds to `handle` from the Pipeline's Registry
    [[nodiscard]] Pipeline& GetPipeline(const PipelineHandle& handle);

    /// Frees the Pipeline that corresponds to `handle` from the Pipeline's Registry
    void FreePipeline(VkDevice device, PipelineHandle& handle);

    /// Constructs a new Pipeline Layout at the first free slot in the Pipeline Layout's Registry
    [[nodiscard]] PipelineLayoutHandle CreatePipelineLayout(VkDevice device, const PipelineLayoutDesc& desc);

    /// Fetches the Pipeline Layout that corresponds to `handle` from the Pipeline Layout's Registry
    [[nodiscard]] PipelineLayout& GetPipelineLayout(const PipelineLayoutHandle& handle);

    /// Frees the Pipeline Layout that corresponds to `handle` from the Pipeline Layout's Registry
    void FreePipelineLayout(VkDevice device, PipelineLayoutHandle& handle);


private:
    /// Container of all images created.
    Registry<Image> m_ImagesRegistry;
    /// Container of all buffers created.
    Registry<Buffer> m_BuffersRegistry;
    /// Container of all samplers created.
    Registry<Sampler> m_SamplersRegistry;
    /// Container of all pipelines created.
    Registry<Pipeline> m_PipelinesRegistry;
    /// Container of pipeline layouts created.
    Registry<PipelineLayout> m_PipelineLayoutsRegistry;
};

} // namespace Grace
