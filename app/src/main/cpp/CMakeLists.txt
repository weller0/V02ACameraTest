cmake_minimum_required(VERSION 3.4.1)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror")

add_library(ndk_camera SHARED
        android_main.cpp
        NdkCamera.cpp)
# add include path
#target_include_directories(ndk_camera PRIVATE ${COMMON_SOURCE_DIR})

# add lib dependencies
target_link_libraries(ndk_camera log android mediandk camera2ndk)
