cmake_minimum_required(VERSION 3.10)
project(CacheSimulator)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
include_directories(cpp_logic/include)
file(GLOB_RECURSE SOURCES 
    "cpp_logic/src/Bridge.cpp"
    "cpp_logic/src/Cache.cpp"
    "cpp_logic/src/policies/*.cpp"
    "cpp_logic/src/test_main.cpp"
)
message(STATUS "Found source files:")
foreach(source ${SOURCES})
    message(STATUS "  ${source}")
endforeach()
add_library(cache_logic SHARED ${SOURCES})
set_target_properties(cache_logic PROPERTIES
    PREFIX ""
    OUTPUT_NAME "cache_logic"
    POSITION_INDEPENDENT_CODE ON
)
if(WIN32)
    set_target_properties(cache_logic PROPERTIES
        SUFFIX ".dll"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    target_compile_definitions(cache_logic PRIVATE -DBUILDING_CACHE_LOGIC)
elseif(APPLE)
    set_target_properties(cache_logic PROPERTIES
        SUFFIX ".dylib"
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
else()
    set_target_properties(cache_logic PROPERTIES
        SUFFIX ".so"
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
target_compile_options(cache_logic PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -Wunused -Wunused-function -Wunused-variable -Wunused-parameter>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic -Wunused -Wunused-function -Wunused-variable -Wunused-parameter>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
option(BUILD_TESTS "Build test executables" ON)
if(BUILD_TESTS)
    add_executable(cache_test cpp_logic/src/test_main.cpp)
    target_link_libraries(cache_test cache_logic)
    set_target_properties(cache_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
install(TARGETS cache_logic
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
)
install(DIRECTORY cpp_logic/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")