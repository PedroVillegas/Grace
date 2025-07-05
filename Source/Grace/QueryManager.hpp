#pragma once

#include <array>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>
#include <Grace/GraceExport.h>
#include <Grace/Macros.hpp>

namespace Grace
{

class Device;

enum class QueryType : uint32_t
{
    Timestamp = 0,
    Occlusion,
    PipelineStatistics,
    Undefined,
};

enum class QueryWriteFlags : uint32_t
{
    None = 0,
    WriteIfPreviousResultIsAvailable,
};

struct GRACE_EXPORT QueryGroupDesc
{
    uint32_t timestampQueriesCount = 256U;
    uint32_t occlusionQueriesCount = 256U;
    uint32_t pipelineStatisticsCount = 32U;
    VkQueryPipelineStatisticFlags pipelineStatisticsFlags;
};

namespace TimestampUnits
{

template <double Val>
struct DurationUnits
{
    static constexpr double value = Val;
};

using Nanoseconds = DurationUnits<1.0>;
using Microseconds = DurationUnits<1E-03>;
using Milliseconds = DurationUnits<1E-06>;
using Seconds = DurationUnits<1E-09>;

} // namespace TimestampUnits

class GRACE_EXPORT QueryGroup
{
public:
    ~QueryGroup() = default;
    QueryGroup() = default;

    QueryGroup(const QueryGroup&) = delete;
    QueryGroup& operator=(const QueryGroup&) = delete;

    QueryGroup(QueryGroup&& other) noexcept = delete;
    QueryGroup& operator=(QueryGroup&& other) noexcept = delete;

    _NODISCARD VkQueryPool GetVkQueryPool() const;

    _NODISCARD std::vector<uint64_t>& GetQueries();

    _NODISCARD uint64_t GetQuery(const char* name, uint32_t relativeBit = 0U, uint32_t frameIndex = 0U) const;

    void
    GetQueryIfAvailable(uint64_t& inout, const char* name, uint32_t relativeBit = 0U, uint32_t frameIndex = 0U) const;

    _NODISCARD uint32_t GetQueryOffset(const char* name) const;

    _NODISCARD uint32_t GetRange() const;

    _NODISCARD uint32_t GetQueryCount() const;

    _NODISCARD uint32_t GetValuesPerQuery() const;

    template <typename UnitsType>
    _NODISCARD double Duration(const char* start, const char* end) const
    {
        static_assert(std::is_same_v<UnitsType, TimestampUnits::Nanoseconds>
                      || std::is_same_v<UnitsType, TimestampUnits::Microseconds>
                      || std::is_same_v<UnitsType, TimestampUnits::Milliseconds>
                      || std::is_same_v<UnitsType, TimestampUnits::Seconds>);

        double duration = static_cast<double>(GetQuery(end) - GetQuery(start)) * static_cast<double>(m_TimestampPeriod)
                        * UnitsType::value;

        return duration;
    }

    template <typename UnitsType>
    _NODISCARD double DurationIfAvailable(const char* start, const char* end) const
    {
        static_assert(std::is_same_v<UnitsType, TimestampUnits::Nanoseconds>
                      || std::is_same_v<UnitsType, TimestampUnits::Microseconds>
                      || std::is_same_v<UnitsType, TimestampUnits::Milliseconds>
                      || std::is_same_v<UnitsType, TimestampUnits::Seconds>);

        double duration = static_cast<double>(GetQuery(end) - GetQuery(start)) * static_cast<double>(m_TimestampPeriod)
                        * UnitsType::value;

        return duration;
    }

private:
    _NODISCARD uint32_t AddQuery(const char* name);

private:
    VkQueryPool m_QueryPool = nullptr;
    std::vector<uint64_t> m_Queries = {};
    // Only need one named query map per query group since query indices are constant
    std::unordered_map<const char*, uint32_t> m_NamedQueryMap = {};
    uint32_t m_QueriesWrittenSinceLastReset = 0U;
    uint32_t m_ValuesPerQuery = 1U;
    float m_TimestampPeriod = 0.0f;

    friend class QueryManager;
};

class QueryManager
{
public:
    ~QueryManager();
    QueryManager() = default;
    QueryManager(Device* pDevice, uint32_t framesInFlight, const QueryGroupDesc& qgDesc);

    QueryManager(const QueryManager&) = delete;
    QueryManager& operator=(const QueryManager&) = delete;

    QueryManager(QueryManager&& other) noexcept = delete;
    QueryManager& operator=(QueryManager&& other) noexcept = delete;

    _NODISCARD QueryGroup& GetQueryGroup(QueryType qt);

    _NODISCARD uint32_t AddQuery(QueryType qt, const char* name);

    void ResetQueryGroup(QueryType qt);

private:
    Device* m_pDevice = nullptr;
    std::array<QueryGroup, static_cast<uint32_t>(QueryType::Undefined)> m_QueryGroups = {};
};

} // namespace Grace