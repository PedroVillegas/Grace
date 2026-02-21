include(FetchContent)
FetchContent_Declare(
	googletest
	URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

get_target_property(PrivateIncludeDirs Grace INCLUDE_DIRECTORIES)
get_target_property(PrivateLinkLibraries Grace LINK_LIBRARIES)
get_target_property(PrivateSources Grace SOURCES)

foreach (Source ${PrivateSources})
	list(APPEND GRACE_PRIVATE_SOURCES ${PROJECT_SOURCE_DIR}/Source/${Source})
endforeach ()

add_executable(TestBed)
target_sources(TestBed
	PRIVATE StackAllocator.cpp
	PRIVATE ResourceManager.cpp
	PRIVATE ${GRACE_PRIVATE_SOURCES}
)

target_include_directories(TestBed PRIVATE ${PrivateIncludeDirs})
target_link_libraries(TestBed PRIVATE Grace GTest::gtest_main ${PrivateLinkLibraries})

include(GoogleTest)
gtest_discover_tests(TestBed)
