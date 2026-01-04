find_package(GTest REQUIRED)
enable_testing()

# Protobuf generation for tests
set(PROTO_FILES  
    ${CMAKE_SOURCE_DIR}/proto/csmsg.proto  
    ${CMAKE_SOURCE_DIR}/proto/msgid.proto  
    ${CMAKE_SOURCE_DIR}/proto/http.proto  
)  

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# Example smoke test target (keeps old example)
add_executable(tests_example 
	${CMAKE_CURRENT_SOURCE_DIR}/test_example.cpp
)
target_link_libraries(tests_example PRIVATE GTest::gtest_main)
target_include_directories(tests_example PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME tests_example COMMAND tests_example)

# Modularized unit tests for src classes
add_executable(lps_unit_tests
	${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/test_crc.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/test_config.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_db.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/test_framing.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/test_service.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/test_session.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/test_network.cpp
	${CMAKE_SOURCE_DIR}/src/crc.cpp
	${CMAKE_SOURCE_DIR}/src/sqlite_db.cpp
	${CMAKE_SOURCE_DIR}/src/con_mgr.cpp
	${CMAKE_SOURCE_DIR}/src/service_container.cpp
	${CMAKE_SOURCE_DIR}/src/session.cpp
	${CMAKE_SOURCE_DIR}/src/tcp_server.cpp
	${CMAKE_SOURCE_DIR}/src/http_server.cpp
	${CMAKE_SOURCE_DIR}/src/http_session.cpp
	${CMAKE_SOURCE_DIR}/src/udp_broadcaster.cpp
	${PROTO_SRCS}
	${PROTO_HDRS}
)

target_include_directories(lps_unit_tests PRIVATE 
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}
	${CMAKE_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lps_unit_tests PRIVATE
	GTest::gtest_main
	${Protobuf_LIBRARIES}
	${SQLite3_LIBRARIES}
	${Boost_LIBRARIES}
)

if(APPLE)
	target_link_libraries(lps_unit_tests PRIVATE
		absl::check
		absl::log
		absl::strings
		absl::status
		absl::str_format
		absl::base
		absl::time
	)
endif()

add_test(NAME lps_unit_tests COMMAND lps_unit_tests)
