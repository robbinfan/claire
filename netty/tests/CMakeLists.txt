if(CARES_FOUND)
    add_executable(dns_test resolver_test.cc)
    target_link_libraries(dns_test claire_netty cares)
endif()

add_executable(HttpRequest_unittest HttpRequest_unittest.cc)
target_link_libraries(HttpRequest_unittest claire_netty gtest gtest_main)

add_executable(HttpResponse_unittest HttpResponse_unittest.cc)
target_link_libraries(HttpResponse_unittest claire_netty gtest gtest_main)

add_executable(Uri_unittest Uri_unittest.cc)
target_link_libraries(Uri_unittest claire_netty gtest gtest_main)
