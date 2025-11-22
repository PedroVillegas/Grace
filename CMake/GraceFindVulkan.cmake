execute_process(
    COMMAND curl https://vulkan.lunarg.com/sdk/latest/windows.txt
    OUTPUT_VARIABLE GRACE_LATEST_SDK_VERSION
)

find_package(Vulkan REQUIRED)

if (Vulkan_FOUND)
    message(STATUS "[GRACE_INFO] Found Vulkan SDK: ${Vulkan_VERSION}")
    if(NOT DEFINED Vulkan_VERSION)
        message(FATAL_ERROR
            "[GRACE_ERROR] Could not find a Vulkan SDK installed. "
            "Ensure you have installed a Vulkan SDK which can be found at 'https://vulkan.lunarg.com/sdk/home', "
            "and Vulkan SDK path is properly configured in your system variables!"
        )
    endif()
    if(${Vulkan_VERSION} VERSION_LESS ${GRACE_LATEST_SDK_VERSION})
        message(STATUS
            "[GRACE_WARN] Vulkan SDK (${Vulkan_VERSION}) not up-to-date (${GRACE_LATEST_SDK_VERSION}). "
            "It is recommended to use the most recent SDK release at 'https://vulkan.lunarg.com/sdk/home'."
        )
    endif()
else()
    message(FATAL_ERROR
        "[GRACE_ERROR] Could not find a Vulkan SDK installed. "
        "Ensure you have installed a Vulkan SDK which can be found at 'https://vulkan.lunarg.com/sdk/home', "
        "and Vulkan SDK path is properly configured in your system variables!"
    )
endif()

set(Vulkan_BIN_DIR ${Vulkan_INCLUDE_DIR}/../Bin)