project(CC2540D)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)

find_package(Libusb1 REQUIRED)
include_directories(${LIBUSB1_INCLUDE_DIRS})

set(CMAKE_THREAD_PREFER_PTHREAD)
find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} ${LIBUSB1_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
