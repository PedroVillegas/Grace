#include "QueryManager.hpp"

#include <bit>

#include <Grace/Device.hpp>
#include <Grace/DebugReporter.hpp>
#include <Grace/HelperFunctions.hpp>

namespace Grace
{

QueryManager::~QueryManager()
{
    vkDestroyQueryPool(mDevicePtr->GetVkHandle(), mTimestampQueryGroup.mQueryPool, nullptr);
    vkDestroyQueryPool(mDevicePtr->GetVkHandle(), mOcclusionQueryGroup.mQueryPool, nullptr);
    vkDestroyQueryPool(mDevicePtr->GetVkHandle(), mPipelineStatsQueryGroup.mQueryPool, nullptr);
}

QueryManager::QueryManager(Device* pDevice, uint32_t framesInFlight, const QueryGroupDesc& qgDesc) : mDevicePtr(pDevice)
{
    assert(mDevicePtr != nullptr);

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(mDevicePtr->GetPhysicalDevice(), &props);
    const float timestampPeriod = props.limits.timestampPeriod;

    // Initialise Timestamp Query Group
    mTimestampQueryGroup.mQueries.resize(qgDesc.timestampQueriesCount * framesInFlight);
    mTimestampQueryGroup.mTimestampPeriod = timestampPeriod;
    mTimestampQueryGroup.mRange = qgDesc.timestampQueriesCount;

    VkQueryPoolCreateInfo pci = {};
    pci.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    pci.pNext = nullptr;
    pci.queryType = VK_QUERY_TYPE_TIMESTAMP;
    pci.queryCount = mTimestampQueryGroup.mValuesPerQuery * mTimestampQueryGroup.mQueries.size();
    pci.flags = 0;
    pci.pipelineStatistics = 0;
    DebugReporter::Check(vkCreateQueryPool(mDevicePtr->GetVkHandle(), &pci, nullptr, &mTimestampQueryGroup.mQueryPool));

    AssignDebugName<VkQueryPool>(pDevice->GetVkHandle(), mTimestampQueryGroup.mQueryPool, "Grace::QueryPool::Timestamp");

    // Initialise Occlusion Query Group
    mOcclusionQueryGroup.mQueries.resize(qgDesc.occlusionQueriesCount * framesInFlight);
    mOcclusionQueryGroup.mTimestampPeriod = timestampPeriod;
    mOcclusionQueryGroup.mRange = qgDesc.occlusionQueriesCount;

    pci.queryType = VK_QUERY_TYPE_OCCLUSION;
    pci.queryCount = mOcclusionQueryGroup.mValuesPerQuery * mOcclusionQueryGroup.mQueries.size();
    DebugReporter::Check(vkCreateQueryPool(mDevicePtr->GetVkHandle(), &pci, nullptr, &mOcclusionQueryGroup.mQueryPool));

    AssignDebugName<VkQueryPool>(pDevice->GetVkHandle(), mOcclusionQueryGroup.mQueryPool, "Grace::QueryPool::Occlusion");

    // Need to find the number of pipelineStatistics bits that have been set
    uint32_t bitsSet = std::popcount(static_cast<uint32_t>(qgDesc.pipelineStatisticsFlags));

    // Initialise Pipeline Stats Query Group
    mPipelineStatsQueryGroup.mQueries.resize(bitsSet * qgDesc.pipelineStatisticsCount * framesInFlight);
    mPipelineStatsQueryGroup.mValuesPerQuery = bitsSet;
    mPipelineStatsQueryGroup.mTimestampPeriod = timestampPeriod;
    mPipelineStatsQueryGroup.mRange = qgDesc.pipelineStatisticsCount;

    pci.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    pci.queryCount = mPipelineStatsQueryGroup.mValuesPerQuery * mPipelineStatsQueryGroup.mQueries.size();
    pci.pipelineStatistics = static_cast<VkQueryPipelineStatisticFlags>(qgDesc.pipelineStatisticsFlags);
    DebugReporter::Check(vkCreateQueryPool(mDevicePtr->GetVkHandle(), &pci, nullptr, &mPipelineStatsQueryGroup.mQueryPool));

    AssignDebugName<VkQueryPool>(
        pDevice->GetVkHandle(), mPipelineStatsQueryGroup.mQueryPool, "Grace::QueryPool::PipelineStatistics");
}

} // namespace Grace