file(GLOB_RECURSE XIN_REDIS_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(xin_redis STATIC ${XIN_REDIS_SOURCES})

add_library(xin::redis ALIAS xin_redis)

target_compile_features(xin_redis PUBLIC cxx_std_23)

target_include_directories(xin_redis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(xin_redis 
    PUBLIC
        Microsoft.GSL::GSL
    PRIVATE
        xin::base
)