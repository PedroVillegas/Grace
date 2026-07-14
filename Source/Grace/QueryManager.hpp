#pragma once

#include <cassert>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <string>

#include <vulkan/vulkan.h>
#include <Grace/GraceApi.hpp>
#include <Grace/Detail/Macros.hpp>
#include <Grace/Enums.hpp>

namespace Grace
{

class Device;

enum class QueryWriteFlags : uint32_t
{
    None = 0,
    WriteIfPreviousResultIsAvailable,
};

namespace TimestampUnits
{

template <double Val>
struct GRACE_API DurationUnits
{
    static constexpr double value = Val;
};

using Nanoseconds = DurationUnits<1.0>;
using Microseconds = DurationUnits<1E-03>;
using Milliseconds = DurationUnits<1E-06>;
using Seconds = DurationUnits<1E-09>;

} // namespace TimestampUnits

namespace QueryType
{

struct GRACE_API QueryTypeBase
{};

struct GRACE_API Timestamp : QueryTypeBase
{};

struct GRACE_API Occlusion : QueryTypeBase
{};

struct GRACE_API PipelineStatistics : QueryTypeBase
{};

} // namespace QueryType

template <typename QueryTy>
class GRACE_API QueryGroup
{
    static_assert(std::derived_from<QueryTy, QueryType::QueryTypeBase>,
                  "QueryGroup type must be derived from QueryGroupType::QueryTypeBase!");

public:
    ~QueryGroup() = default;
    QueryGroup() = default;

    QueryGroup(const QueryGroup&) = delete;
    QueryGroup& operator=(const QueryGroup&) = delete;

    QueryGroup(QueryGroup&& other) noexcept = delete;
    QueryGroup& operator=(QueryGroup&& other) noexcept = delete;

    GRACE_NODISCARD VkQueryPool GetVkQueryPool() const
    {
        return mQueryPool;
    }

    GRACE_NODISCARD std::vector<uint64_t>& GetQueries()
    {
        return mQueries;
    }

    GRACE_NODISCARD uint64_t GetQuery(const char* name, uint32_t relativeBit = 0U, uint32_t frameIndex = 0U) const
    {
        const uint32_t offset = GetQueryOffset(name);
        return mQueries[(mRange * frameIndex) + offset + relativeBit];
    }

    void
    GetQueryIfAvailable(uint64_t& inout, const char* name, uint32_t relativeBit = 0U, uint32_t frameIndex = 0U) const
    {
        const uint32_t offset = GetQueryOffset(name);
        const uint32_t queryOffsetMultiFrame = ((mRange - 1) * frameIndex) + (offset * (mValuesPerQuery + 1));
        // The query's availability bit immediately proceeds the n values of the query
        if (mQueries[queryOffsetMultiFrame + 1] != 0)
        {
            inout = mQueries[queryOffsetMultiFrame + relativeBit];
        }
    }

    GRACE_NODISCARD uint32_t GetQueryOffset(const char* name) const
    {
        assert(name != nullptr);
        assert(mNamedQueryMap.contains(name));
        return mNamedQueryMap.at(name);
    }

    GRACE_NODISCARD uint32_t GetRange() const
    {
        return mRange;
    }

    GRACE_NODISCARD uint32_t GetQueryCount() const
    {
        return mQueriesWrittenSinceLastReset;
    }

    GRACE_NODISCARD uint32_t GetValuesPerQuery() const
    {
        return mValuesPerQuery;
    }

    /// Returns duration if vkGetQueryPoolResults was successful, 0.0 otherwise.
    template <typename UnitsType, typename U = QueryTy>
    GRACE_NODISCARD std::enable_if_t<std::is_same_v<U, QueryType::Timestamp>, double>
    Duration(const char* start, const char* end, uint32_t frameIndex = 0U) const
    {
        static_assert(std::is_same_v<UnitsType, TimestampUnits::Nanoseconds>
                      || std::is_same_v<UnitsType, TimestampUnits::Microseconds>
                      || std::is_same_v<UnitsType, TimestampUnits::Milliseconds>
                      || std::is_same_v<UnitsType, TimestampUnits::Seconds>);

        if (mLastResult != VK_SUCCESS)
        {
            return 0.0;
        }

        uint64_t endStamp = GetQuery(end, 0, frameIndex);
        uint64_t startStamp = GetQuery(start, 0, frameIndex);

        if (endStamp == std::numeric_limits<uint64_t>::max() || startStamp == std::numeric_limits<uint64_t>::max())
        {
            return 0.0;
        }

        double duration =
            static_cast<double>(endStamp - startStamp) * static_cast<double>(mTimestampPeriod) * UnitsType::value;

        return duration;
    }

    template <typename UnitsType, typename U = QueryTy>
    GRACE_NODISCARD std::enable_if_t<std::is_same_v<U, QueryType::Timestamp>, void>
    DurationIfAvailable(double& inout, const char* start, const char* end, uint32_t frameIndex = 0U) const
    {
        static_assert(std::is_same_v<UnitsType, TimestampUnits::Nanoseconds>
                      || std::is_same_v<UnitsType, TimestampUnits::Microseconds>
                      || std::is_same_v<UnitsType, TimestampUnits::Milliseconds>
                      || std::is_same_v<UnitsType, TimestampUnits::Seconds>);

        uint64_t startTicks = 0;
        uint64_t endTicks = 0;

        GetQueryIfAvailable(startTicks, start, 0, frameIndex);
        GetQueryIfAvailable(endTicks, end, 0, frameIndex);

        if (startTicks > 0 && endTicks > 0)
        {
            inout =
                static_cast<double>(endTicks - startTicks) * static_cast<double>(mTimestampPeriod) * UnitsType::value;
        }
    }

private:
    GRACE_NODISCARD uint32_t AddQuery(const char* name)
    {
        const uint32_t offset = mQueriesWrittenSinceLastReset++;
        mNamedQueryMap[name] = offset;
        return offset;
    }

public:
    VkResult mLastResult = VK_SUCCESS;

private:
    VkQueryPool mQueryPool = nullptr;
    std::vector<uint64_t> mQueries = {};
    // Only need one named query map per query group since query indices are constant
    std::unordered_map<std::string, uint32_t> mNamedQueryMap = {};
    uint32_t mQueriesWrittenSinceLastReset = 0U;
    uint32_t mValuesPerQuery = 1U;
    uint32_t mRange = 0U;
    float mTimestampPeriod = 0.0F;

    friend class QueryManager;
};

using TimestampQueryGroup = QueryGroup<QueryType::Timestamp>;
using OcclusionQueryGroup = QueryGroup<QueryType::Occlusion>;
using PipelineStatsQueryGroup = QueryGroup<QueryType::PipelineStatistics>;

class QueryManager
{
public:
    ~QueryManager();
    QueryManager() = default;
    QueryManager(Device* pDevice);

    QueryManager(const QueryManager&) = delete;
    QueryManager& operator=(const QueryManager&) = delete;

    QueryManager(QueryManager&& other) noexcept = delete;
    QueryManager& operator=(QueryManager&& other) noexcept = delete;

    template <typename T>
    GRACE_NODISCARD QueryGroup<T>& GetQueryGroup()
    {
        static_assert(std::derived_from<T, QueryType::QueryTypeBase>,
                      "QueryGroup type must be derived from QueryGroupType::QueryTypeBase!");

        if constexpr (std::is_same_v<T, QueryType::Timestamp>)
        {
            return mTimestampQueryGroup;
        }
        else if constexpr (std::is_same_v<T, QueryType::Occlusion>)
        {
            return mOcclusionQueryGroup;
        }
        else if constexpr (std::is_same_v<T, QueryType::PipelineStatistics>)
        {
            return mPipelineStatsQueryGroup;
        }
    }

    template <typename T>
    GRACE_NODISCARD uint32_t AddQuery(const char* name)
    {
        QueryGroup<T>& qg = GetQueryGroup<T>();
        return qg.AddQuery(name);
    }

    template <typename T>
    void ResetQueryGroup()
    {
        QueryGroup<T>& qg = GetQueryGroup<T>();
        qg.mQueriesWrittenSinceLastReset = 0;
    }

private:
    Device* mDevicePtr = nullptr;
    TimestampQueryGroup mTimestampQueryGroup = {};
    OcclusionQueryGroup mOcclusionQueryGroup = {};
    PipelineStatsQueryGroup mPipelineStatsQueryGroup = {};
};

} // namespace Grace
