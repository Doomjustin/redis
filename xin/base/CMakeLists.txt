file(GLOB_RECURSE XIN_BASE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(xin_base STATIC ${XIN_BASE_SOURCES})

add_library(xin::base ALIAS xin_base)

target_compile_features(xin_base PUBLIC cxx_std_23)

target_include_directories(xin_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(xin_base 
    PUBLIC
        fmt::fmt
        Microsoft.GSL::GSL
    PRIVATE
        spdlog::spdlog
)