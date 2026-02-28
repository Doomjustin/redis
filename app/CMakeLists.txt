add_executable(xin-redis-server redis_app_server.cpp)
target_compile_features(xin-redis-server PRIVATE cxx_std_23)

add_executable(xin-redis-cli redis_app_client.cpp)
target_compile_features(xin-redis-cli PRIVATE cxx_std_23)