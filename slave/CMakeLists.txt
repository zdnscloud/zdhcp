cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-std=c++11 -fPIC")
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Folly REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread filesystem)

include_directories(
    ${CMAKE_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/asio
    ${FOLLY_INCLUDE_DIR}
    ${INCLUDE_DIR}
)

set(KEA_HEADER_DIRS
  configure
  exceptions
  util
  dns
  dhcp++
)

foreach(dir ${KEA_HEADER_DIRS})
  file(GLOB_RECURSE headers ${dir}/*.h)
  set(KEA_HEADERS
    ${KEA_HEADERS}
    ${headers})
endforeach()

#message("${KEA_HEADERS}")

set(KEA_SOURCES
  configure/json_conf.cpp
  util/json11.cpp
  util/strutil.cpp
  util/io_address.cpp
  util/encode/base_n.cpp
  util/ipaddress_extend.cpp
  util/thread_pool.cpp
  exceptions/exceptions.cpp
  dns/exceptions.cpp
  dns/name.cpp
  dns/labelsequence.cpp
  dhcp++/hwaddr.cpp
  dhcp++/option_data_types.cpp
  dhcp++/option_space.cpp
  dhcp++/option.cpp
  dhcp++/option_vendor.cpp
  dhcp++/opaque_data_tuple.cpp
  dhcp++/option_opaque_data_tuples.cpp
  dhcp++/option_vendor_class.cpp
  dhcp++/option_string.cpp
  dhcp++/option4_addrlst.cpp
  dhcp++/option4_client_fqdn.cpp
  dhcp++/option_custom.cpp
  dhcp++/option_definition.cpp
  dhcp++/option_meta.cpp
  dhcp++/option_description.cpp
  dhcp++/std_option_defs.cpp
  dhcp++/vendor_option_defs.cpp
  dhcp++/libdhcp++.cpp
  dhcp++/classify.cpp
  dhcp++/pkt.cpp
  dhcp++/duid.cpp
  dhcp++/duid_factory.cpp
  dhcp++/subnet.cpp
  dhcp++/pool.cpp
  nic/pkt_filter.cpp
  nic/pkt_filter_inet.cpp 
  nic/iface.cpp
  nic/iface_mgr.cpp
  nic/iface_mgr_linux.cpp
  client/client_context.cpp
  server/host.cpp
  server/hosts_in_mem.cpp
  server/subnet_mgr.cpp
  server/client_class_matcher.cpp 
  server/client_class_parser.cpp
  server/client_class_manager.cpp
  server/server.cpp
  server/ctrl_server.cpp
  server/response_gen.cpp
  hooks/server_hooks.cpp
  hooks/callout_handle.cpp
  hooks/callout_manager.cpp
  hooks/library_manager.cpp
  hooks/library_manager_collection.cpp
  hooks/hooks_manager.cpp
  controller/cmd_server.cpp
  ping/timer_queue.cpp
  ping/icmp_protocol.cpp
  ping/ping.cpp
  log/kea_log.cpp
  rpc/context.pb.cpp
  rpc/lease.pb.cpp
  rpc/rpc_allocate_engine.cpp
  logging/logging.cpp
  logging/exception.cpp
  logging/stringutil.cpp
  logging/fileoutputstream.cpp
  logging/syslogtarget.cpp
  logging/logfileoutputstream.cpp
  statistics/pkt_statistic.cpp
  controller/tcp_server.cpp
  controller/tcp_session.cpp
  controller/tcp_session_manager.cpp
)

set(KEA_SLAVE_SOURCES
  bin/kea_slave.cpp
)

add_library(kea SHARED
  ${KEA_HEADERS}
  ${KEA_SOURCES}
)

target_link_libraries(kea
  ${FOLLY_LIBRARIES}
  ${Boost_LIBRARIES}
  ${PostgreSQL_LIBRARIES}
  glog
  dl
  ssl
  crypto
  protobuf
)

add_executable(kea_slave ${KEA_SLAVE_SOURCES})
target_link_libraries(kea_slave kea gflags)

set(PING_TEST_SOURCES
  bin/ping_test.cpp
)
add_executable(test_ping ${PING_TEST_SOURCES})
target_link_libraries(test_ping kea gflags)

install(TARGETS kea DESTINATION lib)
foreach(dir ${KEA_HEADER_DIRS})
  install(DIRECTORY ${dir} DESTINATION include/kea
          FILES_MATCHING PATTERN "*.h")
endforeach()

macro(build_hook_lib hook_name hook_dir)
    add_library(${hook_name} SHARED
        ${KEA_HEADERS}
        ${hook_dir}/${hook_name}.cpp)
    target_link_libraries(${hook_name} kea)
endmacro(build_hook_lib)

option(BUILD_EXTENSIONS "BUILD_EXTENSIONS" ON)
if(BUILD_EXTENSIONS)
    build_hook_lib(dumb_recv_send_hook extensions)
    build_hook_lib(rate_limit extensions)
endif()

option(BUILD_TESTS "BUILD_TESTS" OFF)
if(BUILD_TESTS)
    add_subdirectory(gtest-1.7.0)
    enable_testing()

    set(DB_UNIT_TEST_SOURCES
        server/testutil/schema.cpp
        server/testutil/pgsql_schema.cpp
        server/testutil/lease_compare.cpp
        server/testutil/generic_lease_mgr_test.cpp
        server/testutil/alloc_engine_test.cpp
    )

    add_library(kea-db-test STATIC
        ${KEA_HEADERS}
        ${DB_UNIT_TEST_SOURCES}
    ) 

    target_link_libraries(kea-db-test
        ${FOLLY_LIBRARIES}
        ${Boost_LIBRARIES}
        ${PostgreSQL_LIBRARIES}
        kea
    )

    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

    macro(add_gtest test_source test_name)
        add_executable(${test_name} ${test_source})
        target_link_libraries(${test_name} gtest gtest_main)
        target_link_libraries(${test_name} kea)
        target_link_libraries(${test_name} kea-db-test)
        add_test(${test_name} bin/${test_name})
    endmacro(add_gtest)

    build_hook_lib(basic_callout_library hooks/test)
    build_hook_lib(framework_exception_library hooks/test)
    build_hook_lib(no_version_library hooks/test)
    build_hook_lib(incorrect_version_library hooks/test)
    build_hook_lib(load_callout_library hooks/test)
    build_hook_lib(load_error_callout_library hooks/test)
    build_hook_lib(full_callout_library hooks/test)
    build_hook_lib(callout_params_library hooks/test)

    add_gtest(configure/test/json_conf_test.cpp json_conf_test)
    add_gtest(util/test/lru_cache_test.cpp lru_cache_test)
    add_gtest(dhcp++/test/hwaddr_test.cpp hwaddr_test)
    add_gtest(dhcp++/test/option_data_types_test.cpp option_data_types_test)
    add_gtest(dhcp++/test/option_test.cpp option_test)
    add_gtest(dhcp++/test/option_space_test.cpp option_space_test)
    add_gtest(dhcp++/test/option_string_test.cpp option_string_test)
    add_gtest(dhcp++/test/option_int_test.cpp option_int_test)
    add_gtest(dhcp++/test/option_int_array_test.cpp option_int_array_test)
    add_gtest(dhcp++/test/option_vendor_class_test.cpp  option_vendor_class_test)
    add_gtest(dhcp++/test/option_vendor_test.cpp option_vendor_test)
    add_gtest(dhcp++/test/option_custom_test.cpp option_custom_test)
    add_gtest(dhcp++/test/option4_addrlst_test.cpp option4_addrlst_test)
    add_gtest(dhcp++/test/option4_client_fqdn_test.cpp option4_client_fqdn_test)
    add_gtest(dhcp++/test/opaque_data_tuple_test.cpp opaque_data_tuple_test)
    add_gtest(dhcp++/test/libdhcp++_test.cpp libdhcp++_test)
    add_gtest(nic/test/iface_mgr_unittest.cpp iface_mgr_unittest)
    add_gtest(server/test/lease_test.cpp lease_test)
    add_gtest(server/test/pool_test.cpp pool_test)
    add_gtest(server/test/subnet_test.cpp subnet_test)
    add_gtest(server/test/database_connection_test.cpp database_connection_test)
    add_gtest(server/test/pgsql_exchange_test.cpp pgsql_exchange_test)
    add_gtest(server/test/pgsql_lease_mgr_test.cpp pgsql_lease_mgr_test)
    add_gtest(server/test/host_test.cpp host_test)
    add_gtest(server/test/hosts_in_mem_test.cpp hosts_in_mem_test)
    add_gtest(server/test/subnet_mgr_test.cpp subnet_mgr_test)
    add_gtest(server/test/alloc_engine_test.cpp  alloc_engine_test)
    add_gtest(server/test/client_class_matcher_test.cpp  client_class_matcher_test)
    add_gtest(server/test/client_class_parser_test.cpp client_class_parser_test)
    add_gtest(hooks/test/server_hooks_test.cpp  server_hooks_test)
    add_gtest(hooks/test/callout_handle_test.cpp  callout_handle_test)
    add_gtest(hooks/test/callout_manager_test.cpp callout_manager_test)
    add_gtest(hooks/test/library_manager_test.cpp library_manager_test)
    add_gtest(hooks/test/library_manager_collection_test.cpp library_manager_collection_test)
    add_gtest(ping/test/timer_test.cpp timer_test)
    add_gtest(ping/test/random_test.cpp random_test)
    add_gtest(ping/test/ping_test.cpp ping_test)
endif()
