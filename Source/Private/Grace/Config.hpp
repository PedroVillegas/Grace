#pragma once

#include <Grace/Enums.hpp>

#include <yaml-cpp/yaml.h>

#include <vector>
#include <string>
#include <unordered_map>
#include <cassert>

namespace Grace
{

inline struct Config
{
    uint32_t FramesInFlight = 1U;

    std::vector<std::string> InstanceExtensions = { "VK_EXT_debug_utils" };
    std::vector<std::string> InstanceLayers = {};
    std::vector<std::string> DeviceExtensions = {};
    std::vector<std::string> DeviceLayers = {};

    uint32_t QueriesTimestampCount = 256U;
    uint32_t QueriesOcclusionCount = 256U;
    uint32_t QueriesPipelineStatsCount = 256U;
    QueryStats QueriesPipelineStatsStages = {};

    Format SwapchainSurfaceColourFormat = Format::RGBA8_SRGB;
    ColourSpace SwapchainSurfaceColourSpace = ColourSpace::SRGB_NonLinear;
    PresentMode SwapchainPresentMode = PresentMode::FIFO;

    uint32_t BindlessResourceTableMaxImageSlots = 65535;
    uint32_t BindlessResourceTableMaxSamplerSlots = 65535;
    uint32_t BindlessResourceTableMaxBufferSlots = 65535;
} gConfig;

void ProcessYamlConfig(const std::string& yaml);

static const std::unordered_map<std::string, Format> kConfigSurfaceColourFormatToGraceFormatMap = {
    { "RGBA8_SRGB", Format::RGBA8_SRGB },
};

static const std::unordered_map<std::string, ColourSpace> kConfigSurfaceColourSpaceToGraceColourSpaceMap = {
    { "SRGB_NonLinear", ColourSpace::SRGB_NonLinear },
};

static const std::unordered_map<std::string, PresentMode> kConfigPresentModeToGracePresentModeMap = {
    { "FIFO", PresentMode::FIFO },
    { "RelaxedFIFO", PresentMode::RelaxedFIFO },
    { "Immediate", PresentMode::Immediate },
    { "Mailbox", PresentMode::Mailbox },
};

static const std::unordered_map<std::string, QueryStats> kConfigPipelineQueryStageToGraceQueryStatsMap = {
    { "ClippingInvocations", QueryStats::ClippingInvocations },
    { "ClippingPrimitives", QueryStats::ClippingPrimitives },
    { "ComputeShaderInvocations", QueryStats::ComputeShaderInvocations },
    { "FragmentShaderInvocations", QueryStats::FragmentShaderInvocations },
    { "GeometryShaderInvocations", QueryStats::GeometryShaderInvocations },
    { "GeometryShaderPrimitives", QueryStats::GeometryShaderPrimitives },
    { "InputAssemblyPrimitives", QueryStats::InputAssemblyPrimitives },
    { "InputAssemblyVertices", QueryStats::InputAssemblyVertices },
    { "MeshShaderInvocations", QueryStats::MeshShaderInvocations },
    { "TaskShaderInvocations", QueryStats::TaskShaderInvocations },
    { "TessellationCtrlShaderPatches", QueryStats::TessellationCtrlShaderPatches },
    { "TessellationEvalShaderInvocations", QueryStats::TessellationEvalShaderInvocations },
    { "VertexShaderInvocations", QueryStats::VertexShaderInvocations },
};

} // namespace Grace

namespace YAML
{

template <>
struct convert<Grace::Format>
{
    static Node encode(Grace::Format rhs)
    {
        assert(false);
        return {};
    }

    static bool decode(const Node& node, Grace::Format& rhs)
    {
        if (!node.IsScalar())
        {
            return false;
        }

        rhs = Grace::kConfigSurfaceColourFormatToGraceFormatMap.at(node.Scalar());

        return true;
    }
};

template <>
struct convert<Grace::ColourSpace>
{
    static Node encode(Grace::ColourSpace rhs)
    {
        assert(false);
        return {};
    }

    static bool decode(const Node& node, Grace::ColourSpace& rhs)
    {
        if (!node.IsScalar())
        {
            return false;
        }

        rhs = Grace::kConfigSurfaceColourSpaceToGraceColourSpaceMap.at(node.Scalar());

        return true;
    }
};

template <>
struct convert<Grace::PresentMode>
{
    static Node encode(Grace::PresentMode rhs)
    {
        assert(false);
        return {};
    }

    static bool decode(const Node& node, Grace::PresentMode& rhs)
    {
        if (!node.IsScalar())
        {
            return false;
        }

        rhs = Grace::kConfigPresentModeToGracePresentModeMap.at(node.Scalar());

        return true;
    }
};

template <>
struct convert<Grace::QueryStats>
{
    static Node encode(Grace::QueryStats rhs)
    {
        assert(false);
        return {};
    }

    static bool decode(const Node& node, Grace::QueryStats& rhs)
    {
        if (!node.IsScalar())
        {
            return false;
        }

        rhs = Grace::kConfigPipelineQueryStageToGraceQueryStatsMap.at(node.Scalar());

        return true;
    }
};

} // namespace YAML
