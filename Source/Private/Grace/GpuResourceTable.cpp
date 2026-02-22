#include <Grace/GpuResourceTable.hpp>

#include <Grace/DebugReporter.hpp>
#include <Grace/Context.hpp>
#include <Grace/Buffer.hpp>
#include <Grace/Image.hpp>
#include <Grace/Sampler.hpp>
#include <Private/Grace/ScratchVector.hpp>
#include <Private/Grace/Config.hpp>
#include <Private/Grace/Assert.hpp>

namespace Grace
{

void SlotPool::SetPoolSize(const uint32_t maxSize)
{
    mMaxSlots = maxSize;
}

void SlotPool::AppendFreeSlot(const uint32_t slot)
{
    mFreeSlots.emplace_front(slot);
}

uint32_t SlotPool::FindAvailableSlot()
{
    GRACE_ASSERT_MSG(mCurrentSlot < mMaxSlots, "Too many slots, spilling SlotPool!");

    if (!mFreeSlots.empty())
    {
        uint32_t slotIndex = mFreeSlots.back();
        mFreeSlots.pop_back();
        return slotIndex;
    }

    return mCurrentSlot++;
}

GpuResourceTable::~GpuResourceTable()
{
    vkDestroyDescriptorPool(mDevice->GetVkHandle(), bindlessDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mDevice->GetVkHandle(), bindlessDescriptorSetLayout, nullptr);
}

GpuResourceTable::GpuResourceTable(Device* pDevice) : mDevice(pDevice)
{
    mStorageImageSlots.SetPoolSize(gConfig.BindlessResourceTableMaxImageSlots);
    mSampledImageSlots.SetPoolSize(gConfig.BindlessResourceTableMaxImageSlots);
    mUniformBufferSlots.SetPoolSize(gConfig.BindlessResourceTableMaxBufferSlots);
    mSamplerSlots.SetPoolSize(gConfig.BindlessResourceTableMaxSamplerSlots);

    mWriter.SetDevice(mDevice->GetVkHandle());

    // Pool Sizes, identical order to bindings
    ScratchVector<VkDescriptorPoolSize> poolSizes = {
        VkDescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, gConfig.BindlessResourceTableMaxImageSlots),
        VkDescriptorPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, gConfig.BindlessResourceTableMaxImageSlots),
        VkDescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, gConfig.BindlessResourceTableMaxImageSlots),
        VkDescriptorPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, gConfig.BindlessResourceTableMaxSamplerSlots),
        VkDescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, gConfig.BindlessResourceTableMaxBufferSlots),
    };

    // Create global descriptor pool
    const VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets = 1,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    DebugReporter::Check(vkCreateDescriptorPool(mDevice->GetVkHandle(), &poolInfo, nullptr, &bindlessDescriptorPool));
    AssignDebugName(mDevice->GetVkHandle(), bindlessDescriptorPool, "Grace::Bindless::DescriptorPool");

    // Enable Update-After-Bind and Partially-Bound
    // clang-format off
    ScratchVector<VkDescriptorBindingFlags> bindingFlags = {
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .pNext = nullptr,
        .bindingCount = static_cast<uint32_t>(bindingFlags.size()),
        .pBindingFlags = bindingFlags.data(),
    };

    // Create global descriptor set layout
    DescriptorLayoutBuilder builder(mDevice->GetVkHandle());
    builder.AddBinding(static_cast<uint32_t>(Bindless::DescriptorTypeBindingIndex::StorageImage), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, gConfig.BindlessResourceTableMaxImageSlots);
    builder.AddBinding(static_cast<uint32_t>(Bindless::DescriptorTypeBindingIndex::SampledImage), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, gConfig.BindlessResourceTableMaxImageSlots);
    builder.AddBinding(static_cast<uint32_t>(Bindless::DescriptorTypeBindingIndex::CombinedImageSampler), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, gConfig.BindlessResourceTableMaxImageSlots);
    builder.AddBinding(static_cast<uint32_t>(Bindless::DescriptorTypeBindingIndex::Sampler), VK_DESCRIPTOR_TYPE_SAMPLER, gConfig.BindlessResourceTableMaxSamplerSlots);
    builder.AddBinding(static_cast<uint32_t>(Bindless::DescriptorTypeBindingIndex::UniformBuffer), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    bindlessDescriptorSetLayout = builder.Build(VK_SHADER_STAGE_ALL, &bindingFlagsInfo, VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);
    AssignDebugName(mDevice->GetVkHandle(), bindlessDescriptorSetLayout, "Grace::Bindless::DescriptorSetLayout");
    // clang-format on

    // Allocate global descriptor set
    const VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = bindlessDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &bindlessDescriptorSetLayout,
    };

    DebugReporter::Check(vkAllocateDescriptorSets(mDevice->GetVkHandle(), &allocInfo, &bindlessDescriptorSet));
    AssignDebugName(mDevice->GetVkHandle(), bindlessDescriptorSet, "Grace::Bindless::DescriptorSet");

    // Create global pipeline layout
    VkPushConstantRange pushConstants = {
        .stageFlags = VK_SHADER_STAGE_ALL,
        .offset = 0,
        .size = 128,
    };

    bindlessPipelineLayout = mDevice->CreatePipelineLayout({
        .name = "Grace::Bindless::PipelineLayout",
        .flags = 0,
        .setLayouts = { bindlessDescriptorSetLayout },
        .pushConstantRanges = { pushConstants },
    });
}

void GpuResourceTable::SubmitImage(Image& image)
{
    GRACE_ASSERT(!image.IsNull());

    uint32_t sampledImgId = 0;
    uint32_t storageImgId = 0;

    // Add to an available slot in Registry
    if (image.HasUsage(ImageUsage::StorageImage))
    {
        storageImgId = mStorageImageSlots.FindAvailableSlot();
        mWriter.WriteImageBindless(storageImgId,
                                   static_cast<uint32_t>(Bindless::DescriptorTypeBindingIndex::StorageImage),
                                   image.GetDefaultView().GetVkHandle(),
                                   nullptr,
                                   VK_IMAGE_LAYOUT_GENERAL,
                                   VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    }
    if (image.HasUsage(ImageUsage::SampledImage))
    {
        sampledImgId = mSampledImageSlots.FindAvailableSlot();
        mWriter.WriteImageBindless(sampledImgId,
                                   static_cast<uint32_t>(Bindless::DescriptorTypeBindingIndex::SampledImage),
                                   image.GetDefaultView().GetVkHandle(),
                                   nullptr,
                                   VK_IMAGE_LAYOUT_GENERAL,
                                   VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    }

    // Set resource id
    image.SetStorageImgId(storageImgId);
    image.SetSampledImgId(sampledImgId);
}

void GpuResourceTable::FreeImage(const Image& image)
{
    if (image.HasUsage(ImageUsage::SampledImage))
    {
        mSampledImageSlots.AppendFreeSlot(image.GetSampledImgId());
    }

    if (image.HasUsage(ImageUsage::StorageImage))
    {
        mStorageImageSlots.AppendFreeSlot(image.GetStorageImgId());
    }
}

void GpuResourceTable::SubmitImageView(ImageView& view)
{
    uint32_t storageImgId = 0;

    if (view.HasUsage(ImageUsage::StorageImage))
    {
        storageImgId = mStorageImageSlots.FindAvailableSlot();
        mWriter.WriteImageBindless(storageImgId,
                                   static_cast<uint32_t>(Bindless::DescriptorTypeBindingIndex::StorageImage),
                                   view.GetVkHandle(),
                                   nullptr,
                                   VK_IMAGE_LAYOUT_GENERAL,
                                   VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    }

    // Set resource id
    view.SetStorageImgId(storageImgId);
}

void GpuResourceTable::SubmitSampler(Sampler& sampler)
{
    uint32_t samplerId = mSamplerSlots.FindAvailableSlot();
    mWriter.WriteSamplerBindless(
        samplerId, static_cast<uint32_t>(Bindless::DescriptorTypeBindingIndex::Sampler), sampler);

    sampler.SetSamplerId(samplerId);
}

void GpuResourceTable::FreeSampler(const Sampler& sampler)
{
    mSamplerSlots.AppendFreeSlot(sampler.GetSamplerId());
}

void GpuResourceTable::SubmitBuffer(const Buffer& buffer)
{
    VmaAllocationInfo2 allocInfo = buffer.GetAllocationInfo();

    mWriter.WriteBuffer(static_cast<uint32_t>(Bindless::DescriptorTypeBindingIndex::UniformBuffer),
                        buffer.GetVkHandle(),
                        allocInfo.allocationInfo.size,
                        0,
                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
}

void GpuResourceTable::UpdateTable()
{
    mWriter.UpdateSet(bindlessDescriptorSet);
}

} // namespace Grace
