cmake_minimum_required(VERSION 3.10.2)
project(telepresence_streaming_driver)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_search_module(GSTREAMER REQUIRED gstreamer-1.0)
pkg_search_module(GSTREAMER_RTP REQUIRED gstreamer-rtp-1.0)

add_definitions(${GSTREAMER_CFLAGS_OTHER})

#add_compile_definitions(JETSON)

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${GSTREAMER_INCLUDE_DIRS} ${GSTREAMER_RTP_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} ${GSTREAMER_LIBRARIES} ${GSTREAMER_RTP_LIBRARIES})
