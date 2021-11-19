cmake_minimum_required(VERSION 3.10)
project(VideoIntercept)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS -fPIC)
add_definitions(-w)

# 源文件
aux_source_directory(src SRCS)

# 编译成库文件
set(LIB_NAME VideoIntercept)
add_library(${LIB_NAME} SHARED ${SRCS})

# 引用的头文件目录
target_include_directories(
        ${LIB_NAME} PUBLIC
        inc
        inc/ffmpeg-4.4.2/mac
        inc/libjpeg-turbo-2.0.5/include
)

# 依赖的库文件目录
target_link_directories(
        ${LIB_NAME} PUBLIC
        libs/ffmpeg-4.4.2/mac_shared
        libs/libjpeg-turbo-2.0.5
)

# 依赖的库文件
set(FFMPEG_LIBS avdevice avfilter avformat avutil swresample swscale avcodec)
set(JPEG_LIBS turbojpeg)
target_link_libraries(
        ${LIB_NAME} PUBLIC
        ${FFMPEG_LIBS}
        ${JPEG_LIBS}
)

# 编译成可执行文件
add_executable(runVideoIntercept main.cpp)
target_link_libraries(
        runVideoIntercept PUBLIC
        ${LIB_NAME}
)