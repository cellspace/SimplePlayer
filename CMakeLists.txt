cmake_minimum_required(VERSION 3.14.7)

project(Project LANGUAGES C VERSION "1.0")

#set(CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#add_definitions(-DUNICODE -D_UNICODE)

set(CMAKE_BUILE_TYPE RELEASE)

message(STATUS "PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}")
message(STATUS "CMAKE_ARCHIVE_OUTPUT_DIRECTORY = ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
include_directories(${PROJECT_SOURCE_DIR}/dependency/include/ffmpeg)
include_directories(${PROJECT_SOURCE_DIR}/dependency/include/SDL2)
link_directories(${PROJECT_SOURCE_DIR}/dependency/lib)

set(TARGET_NAME SimplePlayer)
set(SRC_LIST main.c audio_decode.h video_decode.h)

add_executable(${TARGET_NAME} ${SRC_LIST})

target_link_libraries(${TARGET_NAME} avcodec avformat swresample avutil SDL2 )
