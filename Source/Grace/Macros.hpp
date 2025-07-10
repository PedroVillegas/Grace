#pragma once

#ifndef _NODISCARD
#define _NODISCARD [[nodiscard]]
#endif

#define GRACE_DEFINE_RESOURCE_REGISTRY(ResourceType, RegistryName) \
    Registry<ResourceType> RegistryName;                           \
    template <>                                                    \
    auto& ResourceRegistry<ResourceType>()                         \
    {                                                              \
        return RegistryName;                                       \
    };

#define GRACE_DEFINE_RESOURCE_HANDLE(ResourceType) \
    class ResourceType;                            \
    using ResourceType##Handle = Handle<ResourceType>;