set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(curve_client curve_client.cpp)
add_executable(curve_server curve_server.cpp)

target_link_libraries(curve_client zmq pthread)
target_link_libraries(curve_server zmq pthread)
