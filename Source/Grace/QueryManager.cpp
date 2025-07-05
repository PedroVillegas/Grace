#include "QueryManager.hpp"

#include <Grace/Device.hpp>
#include <Grace/DebugReporter.hpp>
#include <Grace/HelperFunctions.hpp>

namespace Grace
{

VkQueryPool QueryGroup::GetVkQueryPool() const
{
    return m_QueryPool;
}

std::vector<uint64_t>& QueryGroup::GetQueries()
{
    return m_Queries;
}

uint64_t QueryGroup::GetQuery(const char* name, uint32_t relativeBit, uint32_t frameIndex) const
{
    const uint32_t offset = GetQueryOffset(name);
    return m_Queries[(m_Queries.size() * frameIndex) + offset + relativeBit];
}

void QueryGroup::GetQueryIfAvailable(uint64_t& inout, const char* name, uint32_t relativeBit, uint32_t frameIndex) const
{
    const uint32_t offset = GetQueryOffset(name);
    const uint32_t queryOffsetMultiFrame = (m_Queries.size() * frameIndex) + offset;
    // The query's availability bit immediately proceeds the n values of the query
    if (m_Queries[queryOffsetMultiFrame + m_ValuesPerQuery] != 0)
    {
        inout = m_Queries[queryOffsetMultiFrame + relativeBit];
    }
}

uint32_t QueryGroup::GetQueryOffset(const char* name) const
{
    assert(name != nullptr);
    assert(m_NamedQueryMap.contains(name));
    return m_NamedQueryMap.at(name);
}

uint32_t QueryGroup::GetRange() const
{
    return m_Queries.size();
}

uint32_t QueryGroup::GetQueryCount() const
{
    return m_QueriesWrittenSinceLastReset;
}

uint32_t QueryGroup::GetValuesPerQuery() const
{
    return m_ValuesPerQuery;
}

uint32_t QueryGroup::AddQuery(const char* name)
{
    const uint32_t offset = m_QueriesWrittenSinceLastReset++;
    m_NamedQueryMap[name] = offset;
    return offset;
}

QueryManager::~QueryManager()
{
    // clang-format off
    vkDestroyQueryPool(m_pDevice->GetVkHandle(), m_QueryGroups[static_cast<uint32_t>(QueryType::Timestamp)].m_QueryPool, nullptr);
    vkDestroyQueryPool(m_pDevice->GetVkHandle(), m_QueryGroups[static_cast<uint32_t>(QueryType::Occlusion)].m_QueryPool, nullptr);
    vkDestroyQueryPool(m_pDevice->GetVkHandle(), m_QueryGroups[static_cast<uint32_t>(QueryType::PipelineStatistics)].m_QueryPool, nullptr);
    // clang-format on
}

QueryManager::QueryManager(Device* pDevice, uint32_t framesInFlight, const QueryGroupDesc& qgDesc) : m_pDevice(pDevice)
{
    assert(m_pDevice != nullptr);

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_pDevice->GetPhysicalDevice(), &props);
    const float timestampPeriod = props.limits.timestampPeriod;

    QueryGroup& qgTimestamp = m_QueryGroups[static_cast<uint32_t>(QueryType::Timestamp)];
    qgTimestamp.m_Queries.resize(qgDesc.timestampQueriesCount * framesInFlight);
    qgTimestamp.m_TimestampPeriod = timestampPeriod;

    VkQueryPoolCreateInfo pci = {};
    pci.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    pci.pNext = nullptr;
    pci.queryType = VK_QUERY_TYPE_TIMESTAMP;
    pci.queryCount = qgTimestamp.m_ValuesPerQuery * qgDesc.timestampQueriesCount;
    pci.flags = 0;
    pci.pipelineStatistics = 0;
    DebugReporter::Check(vkCreateQueryPool(m_pDevice->GetVkHandle(), &pci, nullptr, &qgTimestamp.m_QueryPool));

    AssignDebugName<VkQueryPool>(pDevice->GetVkHandle(), qgTimestamp.m_QueryPool, "Grace::QueryPool::Timestamp");

    QueryGroup& qgOcclusion = m_QueryGroups[static_cast<uint32_t>(QueryType::Occlusion)];
    qgOcclusion.m_Queries.resize(qgDesc.occlusionQueriesCount * framesInFlight);
    qgOcclusion.m_TimestampPeriod = timestampPeriod;

    pci.queryType = VK_QUERY_TYPE_OCCLUSION;
    pci.queryCount = qgOcclusion.m_ValuesPerQuery * qgDesc.occlusionQueriesCount;
    DebugReporter::Check(vkCreateQueryPool(m_pDevice->GetVkHandle(), &pci, nullptr, &qgOcclusion.m_QueryPool));

    AssignDebugName<VkQueryPool>(pDevice->GetVkHandle(), qgOcclusion.m_QueryPool, "Grace::QueryPool::Occlusion");

    QueryGroup& qgPipelineStatistics = m_QueryGroups[static_cast<uint32_t>(QueryType::PipelineStatistics)];

    // Need to find the number of pipelineStatistics bits that have been set
    uint32_t bitsSet = 0;
    uint32_t numOfPossibleBitsSet = 14U; // As found in VkQueryPipelineStatisticFlagBits enum as of v1.4.313.0
    for (uint32_t i = 0; i < numOfPossibleBitsSet; ++i)
    {
        uint32_t possibleBitsSetMask = 1U << i;
        if (qgDesc.pipelineStatisticsFlags & possibleBitsSetMask)
        {
            bitsSet++;
        }
    }
    qgPipelineStatistics.m_ValuesPerQuery = bitsSet;
    qgPipelineStatistics.m_Queries.resize(bitsSet * qgDesc.pipelineStatisticsCount * framesInFlight);
    qgPipelineStatistics.m_TimestampPeriod = timestampPeriod;

    pci.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    pci.queryCount = qgPipelineStatistics.m_ValuesPerQuery * qgDesc.pipelineStatisticsCount;
    pci.pipelineStatistics = qgDesc.pipelineStatisticsFlags;
    DebugReporter::Check(vkCreateQueryPool(m_pDevice->GetVkHandle(), &pci, nullptr, &qgPipelineStatistics.m_QueryPool));

    AssignDebugName<VkQueryPool>(
        pDevice->GetVkHandle(), qgPipelineStatistics.m_QueryPool, "Grace::QueryPool::PipelineStatistics");
}

QueryGroup& QueryManager::GetQueryGroup(QueryType qt)
{
    assert(static_cast<uint32_t>(qt) < m_QueryGroups.size());
    return m_QueryGroups[static_cast<uint32_t>(qt)];
}

uint32_t QueryManager::AddQuery(QueryType qt, const char* name)
{
    QueryGroup& qg = m_QueryGroups[static_cast<uint32_t>(qt)];
    return qg.AddQuery(name);
}

void QueryManager::ResetQueryGroup(QueryType qt)
{
    QueryGroup& qg = m_QueryGroups[static_cast<uint32_t>(qt)];
    qg.m_QueriesWrittenSinceLastReset = 0;
}

} // namespace Grace