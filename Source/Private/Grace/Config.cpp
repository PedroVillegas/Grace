#include <Private/Grace/Config.hpp>

#include <iostream>
#include <cassert>

namespace Grace
{

#define SET_IF_IN_CONFIG(dest, node, type) \
    if (node)                              \
    {                                      \
        gConfig.dest = node.as<type>();    \
    }

void ProcessYamlConfig(const std::string& yaml)
{
    YAML::Node config = YAML::LoadFile(yaml);

    SET_IF_IN_CONFIG(FramesInFlight, config["General"]["FramesInFlight"], uint32_t);

    const YAML::Node& yamlInstanceExt = config["Instance"]["Extensions"];
    if (yamlInstanceExt.IsSequence())
    {
        for (const auto& ext : yamlInstanceExt)
        {
            gConfig.InstanceExtensions.push_back(ext.as<std::string>());
        }
    }

    const YAML::Node& yamlInstanceLayers = config["Instance"]["Layers"];
    if (yamlInstanceLayers.IsSequence())
    {
        for (const auto& layer : yamlInstanceLayers)
        {
            gConfig.InstanceLayers.push_back(layer.as<std::string>());
        }
    }

    const YAML::Node& yamlDeviceExt = config["Device"]["Extensions"];
    if (yamlDeviceExt.IsSequence())
    {
        for (const auto& ext : yamlDeviceExt)
        {
            gConfig.DeviceExtensions.push_back(ext.as<std::string>());
        }
    }

    const YAML::Node& yamlDeviceLayers = config["Device"]["Layers"];
    if (yamlDeviceLayers.IsSequence())
    {
        for (const auto& layer : yamlDeviceLayers)
        {
            gConfig.DeviceLayers.push_back(layer.as<std::string>());
        }
    }

    SET_IF_IN_CONFIG(QueriesTimestampCount, config["Queries"]["Timestamp"]["Count"], uint32_t);
    SET_IF_IN_CONFIG(QueriesOcclusionCount, config["Queries"]["Occlusion"]["Count"], uint32_t);
    SET_IF_IN_CONFIG(QueriesPipelineStatsCount, config["Queries"]["PipelineStatistics"]["Count"], uint32_t);

    const YAML::Node& yamlQueriesPipelineStatsStages = config["Queries"]["PipelineStatistics"]["Stages"];
    for (const auto& stage : yamlQueriesPipelineStatsStages)
    {
        gConfig.QueriesPipelineStatsStages |= stage.as<QueryStats>();
    }

    SET_IF_IN_CONFIG(SwapchainSurfaceColourFormat, config["Swapchain"]["Surface"]["ColourFormat"], Format);
    SET_IF_IN_CONFIG(SwapchainSurfaceColourSpace, config["Swapchain"]["Surface"]["ColourSpace"], ColourSpace);
    SET_IF_IN_CONFIG(SwapchainPresentMode, config["Swapchain"]["PresentMode"], PresentMode);

    SET_IF_IN_CONFIG(BindlessResourceTableMaxImageSlots, config["BindlessResourceTable"]["MaxImageSlots"], uint32_t);
    SET_IF_IN_CONFIG(
        BindlessResourceTableMaxSamplerSlots, config["BindlessResourceTable"]["MaxSamplerSlots"], uint32_t);
    SET_IF_IN_CONFIG(BindlessResourceTableMaxBufferSlots, config["BindlessResourceTable"]["MaxBufferSlots"], uint32_t);
}

} // namespace Grace
