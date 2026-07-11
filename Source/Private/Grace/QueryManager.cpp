#include <Grace/QueryManager.hpp>
#include <Grace/Device.hpp>
#include <Grace/DebugReporter.hpp>
#include <Grace/HelperFunctions.hpp>
#include <Private/Grace/Config.hpp>
#include <Grace/Assert.hpp>

#include <bit>

namespace Grace
{

QueryManager::~QueryManager()
{
    vkDestroyQueryPool(mDevicePtr->VkHandle(), mTimestampQueryGroup.mQueryPool, nullptr);
    vkDestroyQueryPool(mDevicePtr->VkHandle(), mOcclusionQueryGroup.mQueryPool, nullptr);
    vkDestroyQueryPool(mDevicePtr->VkHandle(), mPipelineStatsQueryGroup.mQueryPool, nullptr);
}

QueryManager::QueryManager(Device* pDevice) : mDevicePtr(pDevice)
{
    GRACE_ASSERT(mDevicePtr != nullptr);

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(mDevicePtr->GetPhysicalDevice(), &props);
    const float timestampPeriod = props.limits.timestampPeriod;

    // Initialise Timestamp Query Group
    mTimestampQueryGroup.mQueries.resize(gConfig.QueriesTimestampCount * gConfig.FramesInFlight);
    mTimestampQueryGroup.mTimestampPeriod = timestampPeriod;
    mTimestampQueryGroup.mRange = gConfig.QueriesTimestampCount;

    VkQueryPoolCreateInfo pci = {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queryType = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount =
            static_cast<uint32_t>(mTimestampQueryGroup.mValuesPerQuery * mTimestampQueryGroup.mQueries.size()),
        .pipelineStatistics = 0,
    };

    if (pci.queryCount > 0)
    {
        DebugReporter::Check(
            vkCreateQueryPool(mDevicePtr->VkHandle(), &pci, nullptr, &mTimestampQueryGroup.mQueryPool));

        AssignDebugName<VkQueryPool>(
            pDevice->VkHandle(), mTimestampQueryGroup.mQueryPool, "Grace::QueryPool::Timestamp");
    }

    // Initialise Occlusion Query Group
    mOcclusionQueryGroup.mQueries.resize(gConfig.QueriesOcclusionCount * gConfig.FramesInFlight);
    mOcclusionQueryGroup.mTimestampPeriod = timestampPeriod;
    mOcclusionQueryGroup.mRange = gConfig.QueriesOcclusionCount;

    pci.queryType = VK_QUERY_TYPE_OCCLUSION;
    pci.queryCount = mOcclusionQueryGroup.mValuesPerQuery * mOcclusionQueryGroup.mQueries.size();

    if (pci.queryCount > 0)
    {
        DebugReporter::Check(
            vkCreateQueryPool(mDevicePtr->VkHandle(), &pci, nullptr, &mOcclusionQueryGroup.mQueryPool));

        AssignDebugName<VkQueryPool>(
            pDevice->VkHandle(), mOcclusionQueryGroup.mQueryPool, "Grace::QueryPool::Occlusion");
    }

    // Need to find the number of pipelineStatistics bits that have been set
    uint32_t bitsSet = std::popcount(static_cast<uint32_t>(gConfig.QueriesPipelineStatsStages));

    // Initialise Pipeline Stats Query Group
    mPipelineStatsQueryGroup.mQueries.resize(bitsSet * gConfig.QueriesPipelineStatsCount * gConfig.FramesInFlight);
    mPipelineStatsQueryGroup.mValuesPerQuery = bitsSet;
    mPipelineStatsQueryGroup.mTimestampPeriod = timestampPeriod;
    mPipelineStatsQueryGroup.mRange = gConfig.QueriesPipelineStatsCount;

    pci.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    pci.queryCount = mPipelineStatsQueryGroup.mValuesPerQuery * mPipelineStatsQueryGroup.mQueries.size();
    pci.pipelineStatistics = static_cast<VkQueryPipelineStatisticFlags>(gConfig.QueriesPipelineStatsStages);

    if (pci.queryCount > 0)
    {
        DebugReporter::Check(
            vkCreateQueryPool(mDevicePtr->VkHandle(), &pci, nullptr, &mPipelineStatsQueryGroup.mQueryPool));

        AssignDebugName<VkQueryPool>(
            pDevice->VkHandle(), mPipelineStatsQueryGroup.mQueryPool, "Grace::QueryPool::PipelineStatistics");
    }
}

} // namespace Grace
