cmake_minimum_required(VERSION 3.15)
project(LPServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};/opt/homebrew")



# 查找必要的依赖
find_package(Boost  REQUIRED COMPONENTS filesystem  )
find_package(Protobuf   REQUIRED )
if(APPLE)
    find_package(absl CONFIG REQUIRED)
endif()
find_package(SQLite3 REQUIRED)
find_package(ZLIB REQUIRED)

# 包含目录
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${SQLite3_INCLUDE_DIRS})
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(src)
include_directories(proto)

# 生成protobuf代码
set(PROTO_FILES 
    proto/csmsg.proto
    proto/msgid.proto
)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# 创建可执行文件
add_executable(lps_server
    src/main.cpp
    src/crc.cpp
    src/sqlite_db.cpp
    src/session.cpp
    src/tcp_server.cpp
    src/udp_broadcaster.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

# 链接库
if(APPLE)
    target_link_libraries(lps_server
        ${Boost_LIBRARIES}
        ${Protobuf_LIBRARIES}
        ${SQLite3_LIBRARIES}
        ${ZLIB_LIBRARIES}
        absl::check
        absl::log
        absl::strings
    )
else()
    target_link_libraries(lps_server
        ${Boost_LIBRARIES}
        ${Protobuf_LIBRARIES}
        ${SQLite3_LIBRARIES}
        ${ZLIB_LIBRARIES}
        # absl::check
        # absl::log
        # absl::strings
    )

endif()


# 设置编译选项
target_compile_options(lps_server PRIVATE -Wall -Wextra)