#include <gtest/gtest.h>

#include <Grace/Grace.hpp>
#include <Private/Grace/InternalContainers.hpp>

TEST(ResourceManagement, ResHandleStdConstruction)
{
    Grace::Context context = {};
    [[maybe_unused]] const Grace::Device* devicePtr = context.DevicePtr(nullptr);

    const Grace::SamplerHandle sampler(0, 0);
    EXPECT_EQ(sampler.Exists(), true);
    EXPECT_EQ(sampler.GetHandle(), 0);
    EXPECT_EQ(sampler.GetGeneration(), 0);
    EXPECT_EQ(Grace::gResHandleRefCounters->samplers.size(), 1);
    EXPECT_EQ(Grace::gResHandleRefCounters->samplers.at(sampler.GetHandle()), 1);
}

TEST(ResourceManagement, ResHandleCopyConstruction)
{
    Grace::Context context = {};
    Grace::Device* devicePtr = context.DevicePtr(nullptr);

    const Grace::SamplerHandle sampler = devicePtr->CreateSampler({});
    Grace::SamplerHandle copyConstruct = sampler;
    EXPECT_EQ(copyConstruct.GetHandle(), sampler.GetHandle());
    EXPECT_EQ(copyConstruct.GetGeneration(), sampler.GetHandle());
    EXPECT_EQ(Grace::gResHandleRefCounters->samplers.size(), 1);
    EXPECT_EQ(Grace::gResHandleRefCounters->samplers.at(sampler.GetHandle()), 2);
}

TEST(ResourceManagement, ResHandleCopyAssignment)
{
    Grace::Context context = {};
    Grace::Device* devicePtr = context.DevicePtr(nullptr);

    const Grace::SamplerHandle sampler = devicePtr->CreateSampler({});
    Grace::SamplerHandle copyAssign = {};
    copyAssign = sampler;
    EXPECT_EQ(copyAssign.GetHandle(), sampler.GetHandle());
    EXPECT_EQ(copyAssign.GetGeneration(), sampler.GetHandle());
    EXPECT_EQ(Grace::gResHandleRefCounters->samplers.size(), 1);
    EXPECT_EQ(Grace::gResHandleRefCounters->samplers.at(sampler.GetHandle()), 2);
}

TEST(ResourceManagement, ResHandleMoveConstruction)
{
    Grace::Context context = {};
    Grace::Device* devicePtr = context.DevicePtr(nullptr);

    const Grace::SamplerHandle moveConstruct = devicePtr->CreateSampler({});
    EXPECT_EQ(moveConstruct.GetHandle(), moveConstruct.GetHandle());
    EXPECT_EQ(moveConstruct.GetGeneration(), moveConstruct.GetHandle());
    EXPECT_EQ(Grace::gResHandleRefCounters->samplers.size(), 1);
    EXPECT_EQ(Grace::gResHandleRefCounters->samplers.at(moveConstruct.GetHandle()), 1);
}

TEST(ResourceManagement, ResHandleMoveAssignment)
{
    Grace::Context context = {};
    Grace::Device* devicePtr = context.DevicePtr(nullptr);

    Grace::SamplerHandle moveAssign;
    moveAssign = devicePtr->CreateSampler({});
    EXPECT_EQ(moveAssign.GetHandle(), moveAssign.GetHandle());
    EXPECT_EQ(moveAssign.GetGeneration(), moveAssign.GetHandle());
    EXPECT_EQ(Grace::gResHandleRefCounters->samplers.size(), 1);
    EXPECT_EQ(Grace::gResHandleRefCounters->samplers.at(moveAssign.GetHandle()), 1);
}

TEST(ResourceManagement, AutoFreeDeferred)
{
    Grace::Context context = {};
    Grace::Device* devicePtr = context.DevicePtr(nullptr);

    const Grace::FenceHandle fence = devicePtr->CreateFence({
        .name = "",
        .flags = Grace::FenceFlags::CreateSignalled,
    });

    uint32_t previousHandle = ~0U;

    {
        const Grace::SamplerHandle sampler = devicePtr->CreateSampler({});
        previousHandle = sampler.GetHandle();
    }

    devicePtr->AdvanceToNextFrame();
    devicePtr->WaitForFence(fence);

    const Grace::SamplerHandle sampler = devicePtr->CreateSampler({});
    EXPECT_EQ(sampler.GetHandle(), previousHandle);
}
