cmake_minimum_required(VERSION 3.10)

project(syncsvr)

set(exe syncsvr)

#生成proto命令
execute_process(COMMAND bash gen.sh WORKING_DIRECTORY ./proto )

set(sources "")
file(GLOB sources "*.cc")
add_executable(${exe} ${sources})

# 获取dir下面所有文件
aux_source_directory(./method   files)
aux_source_directory(./common   files)
aux_source_directory(./src   files)


message("info----common_files:" ${files})
#将common下面所有文件，添加到target中
target_sources(${exe} PRIVATE ${files})







target_link_libraries( ${exe} PRIVATE glog  protobuf grpc grpc++ fmt gflags hiredis redis++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17  -pthread -ggdb -ffunction-sections -fno-strict-aliasing -Wno-write-strings -frtti -fexceptions -O0 -Wall")

target_include_directories(${exe} PRIVATE ./proto ./common ./ ./method)
