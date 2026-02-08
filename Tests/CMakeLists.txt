add_executable(TestBed)
target_sources(TestBed PRIVATE StackAllocator.cpp)
target_link_libraries(TestBed PRIVATE Grace)

get_target_property(PrivateIncludeDirs Grace INCLUDE_DIRECTORIES)
get_target_property(PrivateLinkLibraries Grace LINK_LIBRARIES)

target_include_directories(TestBed PRIVATE ${PrivateIncludeDirs})
target_link_libraries(TestBed PRIVATE ${PrivateLinkLibraries})