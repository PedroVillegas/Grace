#pragma once

#define GRACE_NODISCARD [[nodiscard]]
#define GRACE_FALLTHROUGH [[fallthrough]]

#define GRACE_LOAD_INSTANCE_PFN(instance, fn) reinterpret_cast<PFN_##fn>(vkGetInstanceProcAddr(instance, #fn))
#define GRACE_LOAD_DEVICE_PFN(device, fn) reinterpret_cast<PFN_##fn>(vkGetDeviceProcAddr(device, #fn))

#ifdef _DEBUG
#define GRACE_SET_VK_DEBUG_NAME vkSetDebugUtilsObjectNameEXT_Meta
#else
#define GRACE_SET_VK_DEBUG_NAME(...) ((void) 0)
#endif
