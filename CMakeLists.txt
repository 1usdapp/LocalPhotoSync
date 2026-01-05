cmake_minimum_required(VERSION 3.15)  
project(LPServer)  
  
set(CMAKE_CXX_STANDARD 20)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  
  
# For vcpkg on macOS, add homebrew path  
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")  
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};/opt/homebrew")  
endif()  
# For Apple systems, find absl as well  
if(APPLE)  
    find_package(absl CONFIG REQUIRED)  
endif()  


if(MSVC)
    add_compile_options(/utf-8)  # 强制源文件按 UTF-8 解析
endif()
if(WIN32)
    # Set CMAKE_PREFIX_PATH to vcpkg installed directory
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows")
    add_definitions(-D_WIN32_WINNT=0x0603)  # Windows 8.1
    set(CMAKE_TOOLCHAIN_FILE "./vcpkg/scripts/buildsystems/vcpkg.cmake")    
    set(GFLAGS_USE_TARGET_NAMESPACE ON)
    # Find packages using vcpkg  
    find_package(Protobuf CONFIG REQUIRED)  
    find_package(unofficial-sqlite3 CONFIG REQUIRED)  
    find_package(ZLIB MODULE REQUIRED)  
    find_package(Boost CONFIG REQUIRED)
    # gflags (installed via vcpkg on Windows)
    find_package(gflags CONFIG REQUIRED)
    # 包含目录
    include_directories(${Protobuf_INCLUDE_DIRS})  
    include_directories(${CMAKE_CURRENT_BINARY_DIR})  
    include_directories(src)  
    include_directories(proto)  
else()
    find_package(Boost  REQUIRED COMPONENTS filesystem  )
    find_package(Protobuf   REQUIRED )
    find_package(SQLite3 REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(gflags REQUIRED)
    # 包含目录

    include_directories(${Boost_INCLUDE_DIRS})
    include_directories(${Protobuf_INCLUDE_DIRS})
    include_directories(${SQLite3_INCLUDE_DIRS})
    include_directories(${ZLIB_INCLUDE_DIRS})
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    include_directories(src)
    include_directories(proto)
endif()


  

  

  
# 生成protobuf代码
set(PROTO_FILES  
    proto/csmsg.proto  
    proto/msgid.proto  
    proto/http.proto  
)  
  
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})  
  
# 创建可执行文件
add_executable(lps_server  
    src/main.cpp  
    src/crc.cpp  
    src/sqlite_db.cpp  
    src/sqlite_db_manager.cpp  
    src/session.cpp  
    src/tcp_server.cpp  
    src/http_server.cpp  
    src/http_session.cpp  
    src/udp_broadcaster.cpp  
    src/con_mgr.cpp  
    src/service_container.cpp  
    ${PROTO_SRCS}  
    ${PROTO_HDRS}  
)  
  
# 链接库
if(WIN32)
    target_link_libraries(lps_server
        Boost::boost
        protobuf::libprotobuf
        unofficial::sqlite3::sqlite3
        ZLIB::ZLIB
        gflags::gflags
        ws2_32
    )
elseif(APPLE)
    target_link_libraries(lps_server
        ${Boost_LIBRARIES}
        ${Protobuf_LIBRARIES}
        ${SQLite3_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${gflags_LIBRARIES}
        absl::check
        absl::log
        absl::strings
        absl::status
        absl::str_format
        absl::base
        absl::time
    )
else()
    target_link_libraries(lps_server
        ${Boost_LIBRARIES}
        ${Protobuf_LIBRARIES}
        ${SQLite3_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${gflags_LIBRARIES}
        # absl::check
        # absl::log
        # absl::strings
    )

endif()



if(WIN32)
    target_compile_options(lps_server PRIVATE  -Wall /w) 
else()
# 设置编译选项
    target_compile_options(lps_server PRIVATE -Wall -Wextra -g -ggdb)
endif()

# Optionally build tests if the `tests` directory exists.
option(ENABLE_TESTS "Build unit tests" ON)
if(ENABLE_TESTS AND EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()