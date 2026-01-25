#include "DescriptorGroup.hpp"

#include <cassert>

#include <Grace/DebugReporter.hpp>
#include <Grace/Buffer.hpp>
#include <Grace/Image.hpp>
#include <Grace/Sampler.hpp>

namespace Grace
{

// ----------------------------------------------------------------------------------
//                              DESCRIPTOR ALLOCATOR
// ----------------------------------------------------------------------------------

void DescriptorAllocator::SetDevice(VkDevice device)
{
    assert(device != nullptr);
    mDevice = device;
}

void DescriptorAllocator::Initialise(uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
{
    mRatios.clear();

    for (auto r : poolRatios)
    {
        mRatios.push_back(r);
    }

    VkDescriptorPool newPool = CreatePool(maxSets, poolRatios);

    mSetsPerPool = uint32_t(maxSets * 1.5f); // Grow it next allocation

    mReadyPools.push_back(newPool);
}

void DescriptorAllocator::ClearPools()
{
    assert(mDevice != nullptr);

    for (auto p : mReadyPools)
    {
        vkResetDescriptorPool(mDevice, p, 0);
    }

    for (auto p : mFullPools)
    {
        vkResetDescriptorPool(mDevice, p, 0);
        mReadyPools.push_back(p);
    }
    mFullPools.clear();
}

void DescriptorAllocator::CleanupPools()
{
    assert(mDevice != nullptr);

    for (auto p : mReadyPools)
    {
        vkDestroyDescriptorPool(mDevice, p, nullptr);
    }
    mReadyPools.clear();

    for (auto p : mFullPools)
    {
        vkDestroyDescriptorPool(mDevice, p, nullptr);
    }
    mFullPools.clear();
}

VkDescriptorSet DescriptorAllocator::Allocate(VkDescriptorSetLayout layout, void* pNext)
{
    assert(layout != nullptr);

    // Get or create a pool to allocate from
    VkDescriptorPool poolToUse = GetPool();

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = pNext,
        .descriptorPool = poolToUse,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet ds = nullptr;
    VkResult result = vkAllocateDescriptorSets(mDevice, &allocInfo, &ds);

    // Allocation failed, try again
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
        mFullPools.push_back(poolToUse);

        poolToUse = GetPool();
        allocInfo.descriptorPool = poolToUse;

        DebugReporter::Check(vkAllocateDescriptorSets(mDevice, &allocInfo, &ds));
    }

    mReadyPools.push_back(poolToUse);
    return ds;
}

VkDescriptorPool DescriptorAllocator::GetPool()
{
    VkDescriptorPool newPool;
    if (mReadyPools.size() != 0)
    {
        newPool = mReadyPools.back();
        mReadyPools.pop_back();
    } else
    {
        // Need to create a new pool
        newPool = CreatePool(mSetsPerPool, mRatios);

        mSetsPerPool = uint32_t(mSetsPerPool * 1.5);
        if (mSetsPerPool > 4092)
        {
            mSetsPerPool = 4092;
        }
    }

    return newPool;
}

VkDescriptorPool DescriptorAllocator::CreatePool(uint32_t setCount, std::span<PoolSizeRatio> poolRatios)
{
    assert(mDevice != nullptr);

    std::vector<VkDescriptorPoolSize> poolSizes;
    for (const PoolSizeRatio ratio : poolRatios)
    {
        poolSizes.emplace_back(ratio.type, static_cast<uint32_t>(ratio.ratio * static_cast<float>(setCount)));
    }

    const VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets = setCount,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    VkDescriptorPool newPool = nullptr;
    DebugReporter::Check(vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &newPool));

    return newPool;
}

// ----------------------------------------------------------------------------------
//                              DESCRIPTOR WRITER
// ----------------------------------------------------------------------------------

void DescriptorWriter::SetDevice(VkDevice device)
{
    assert(device != nullptr);
    mDevice = device;
}

void DescriptorWriter::WriteImage(
    int binding, const Image& image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
    assert(!image.IsNull());
    WriteImage(binding, image.GetDefaultView().GetVkHandle(), sampler, layout, type);
}

void DescriptorWriter::WriteImage(
    int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
    assert(image != nullptr);
    // clang-format off
    VkDescriptorImageInfo& info = imageInfos.emplace_back(VkDescriptorImageInfo{
        .sampler = sampler, 
        .imageView = image,
        .imageLayout = layout
    });
    // clang-format on

    VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    write.dstBinding = binding;
    write.dstSet = nullptr; // Empty until we need to write it
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;

    writes.push_back(write);
}

void DescriptorWriter::WriteBuffer(int binding, const Buffer& buffer, size_t size, size_t offset, VkDescriptorType type)
{
    assert(!buffer.IsNull());
    WriteBuffer(binding, buffer.GetVkHandle(), size, offset, type);
}

void DescriptorWriter::WriteBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
    assert(buffer != nullptr);
    // clang-format off
    VkDescriptorBufferInfo& info = bufferInfos.emplace_back(VkDescriptorBufferInfo{
        .buffer = buffer,
        .offset = offset,
        .range = size
    });
    // clang-format on

    const VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = nullptr,
        .dstBinding = static_cast<uint32_t>(binding),
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = nullptr,
        .pBufferInfo = &info,
        .pTexelBufferView = nullptr,
    };

    writes.push_back(write);
}

void DescriptorWriter::WriteImageBindless(uint32_t resourceId,
                                          int binding,
                                          const Image& image,
                                          VkSampler sampler,
                                          VkImageLayout layout,
                                          VkDescriptorType type)
{
    assert(!image.IsNull());
    // clang-format off
    VkDescriptorImageInfo& info = imageInfos.emplace_back(VkDescriptorImageInfo{
        .sampler = sampler, 
        .imageView = image.GetDefaultView().GetVkHandle(),
        .imageLayout = layout
    });
    // clang-format on

    const VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = nullptr,
        .dstBinding = static_cast<uint32_t>(binding),
        .dstArrayElement = resourceId,
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = &info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
    };

    writes.push_back(write);
}

void DescriptorWriter::WriteSamplerBindless(const uint32_t resourceId, int binding, const Sampler& sampler)
{
    assert(sampler.GetVkHandle() != nullptr);

    // clang-format off
    VkDescriptorImageInfo& info = imageInfos.emplace_back(VkDescriptorImageInfo{
        .sampler = sampler.GetVkHandle(), 
        .imageView = nullptr,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
    });
    // clang-format on

    const VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = nullptr,
        .dstBinding = static_cast<uint32_t>(binding),
        .dstArrayElement = resourceId,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo = &info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
    };

    writes.push_back(write);
}

void DescriptorWriter::WriteImageBindless(
    uint32_t resourceId, int binding, VkImageView view, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
    // clang-format off
    VkDescriptorImageInfo& info = imageInfos.emplace_back(VkDescriptorImageInfo{
        .sampler = sampler, 
        .imageView = view, 
        .imageLayout = layout
    });
    // clang-format on

    const VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = nullptr,
        .dstBinding = static_cast<uint32_t>(binding),
        .dstArrayElement = resourceId,
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = &info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
    };

    writes.push_back(write);
}

void DescriptorWriter::WriteBufferBindless(
    uint32_t resourceId, int binding, const Buffer& buffer, size_t size, size_t offset, VkDescriptorType type)
{
    assert(!buffer.IsNull());

    // clang-format off
    VkDescriptorBufferInfo& info = bufferInfos.emplace_back(VkDescriptorBufferInfo{
        .buffer = buffer.GetVkHandle(),
        .offset = offset,
        .range = size
    });
    // clang-format on

    const VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = nullptr,
        .dstBinding = static_cast<uint32_t>(binding),
        .dstArrayElement = resourceId,
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = nullptr,
        .pBufferInfo = &info,
        .pTexelBufferView = nullptr,
    };

    writes.push_back(write);
}

void DescriptorWriter::Clear()
{
    imageInfos.clear();
    writes.clear();
    bufferInfos.clear();
}

void DescriptorWriter::UpdateSet(VkDescriptorSet set)
{
    assert(set != nullptr);

    for (VkWriteDescriptorSet& write : writes)
    {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(mDevice, (uint32_t) writes.size(), writes.data(), 0, nullptr);
    Clear();
}

// ----------------------------------------------------------------------------------
//                          DESCRIPTOR LAYOUT BUILDER
// ----------------------------------------------------------------------------------

void DescriptorLayoutBuilder::AddBinding(uint32_t binding, VkDescriptorType type, uint32_t count)
{
    const VkDescriptorSetLayoutBinding newbind = {
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = count,
        .stageFlags = 0,
        .pImmutableSamplers = nullptr,
    };

    mBindings.push_back(newbind);
}

void DescriptorLayoutBuilder::Clear()
{
    mBindings.clear();
}

VkDescriptorSetLayout
DescriptorLayoutBuilder::Build(VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
{
    for (auto& b : mBindings)
    {
        b.stageFlags |= shaderStages;
    }

    const VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = pNext,
        .flags = flags,
        .bindingCount = static_cast<uint32_t>(mBindings.size()),
        .pBindings = mBindings.data(),
    };

    VkDescriptorSetLayout set = nullptr;
    DebugReporter::Check(vkCreateDescriptorSetLayout(mDevice, &info, nullptr, &set));

    return set;
}

} // namespace Grace
