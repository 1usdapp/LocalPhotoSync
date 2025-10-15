cmake_minimum_required(VERSION 3.10)

project(syncsvr)

set(exe syncsvr)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 生成proto命令
execute_process(COMMAND bash gen.sh WORKING_DIRECTORY ./proto)

# 收集所有源文件
set(SOURCES
    main.cpp
)

# 添加common目录下的源文件
file(GLOB COMMON_SOURCES "common/*.cpp")
list(APPEND SOURCES ${COMMON_SOURCES})

# 添加net目录下的源文件
file(GLOB NET_SOURCES "net/*.cpp")
list(APPEND SOURCES ${NET_SOURCES})

# 添加proto目录下的源文件
file(GLOB PROTO_SOURCES "proto/*.cpp")
list(APPEND SOURCES ${PROTO_SOURCES})

# 创建可执行文件
add_executable(${exe} ${SOURCES})

# 链接库
target_link_libraries(${exe} PRIVATE 
    glog 
    protobuf
    boost_system
    pthread
    gflags
)

# 包含目录
target_include_directories(${exe} PRIVATE 
    ./common
    ./net
    ./proto
    /usr/include
)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -pthread -ggdb -ffunction-sections -fno-strict-aliasing -Wno-write-strings -frtti -fexceptions -O0 -Wall")

# 设置输出目录
set_target_properties(${exe} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 如果有特定的编译器警告或标志，可以在这里添加
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic")
endif()

# 为调试版本设置符号信息
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# 为发布版本设置优化
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")


# 显示编译信息
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "Sources: ${SOURCES}")
