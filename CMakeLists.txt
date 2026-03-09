cmake_minimum_required(VERSION 4.0)

project(XinRedis
    VERSION 0.0.1
    LANGUAGES C CXX
)

include(CTest)
include(CMakeDependentOption)

cmake_dependent_option(${PROJECT_NAME}_BUILD_TESTS
    "Enable ${PROJECT_NAME} project tests targets" ON
    "BUILD_TESTING" OFF
)

# set(CMAKE_BUILD_TYPE RelWithDebInfo)

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(ConfigSafeGuards)
include(CopyDllsForDebug)
include(CopyResources)

find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)

find_package(jemalloc CONFIG)
if(NOT jemalloc_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(jemalloc IMPORTED_TARGET jemalloc)
        if(jemalloc_FOUND)
            add_library(jemalloc::jemalloc ALIAS PkgConfig::jemalloc)
        endif()
    endif()
endif()

add_subdirectory(xin)
add_subdirectory(app)