add_executable(gen-assets assets/gen-assets.cc)
install(TARGETS gen-assets DESTINATION bin)

#generate asset file
set(INSPECT_ASSETS
  ${CMAKE_CURRENT_SOURCE_DIR}/inspect/assets/flags.js
  ${CMAKE_CURRENT_SOURCE_DIR}/inspect/assets/flags.html
  ${CMAKE_CURRENT_SOURCE_DIR}/inspect/assets/graphview.html
  ${CMAKE_CURRENT_SOURCE_DIR}/inspect/assets/grapher.js
  ${CMAKE_CURRENT_SOURCE_DIR}/inspect/assets/parser.js
  ${CMAKE_CURRENT_SOURCE_DIR}/inspect/assets/dygraph-combined.js
  ${CMAKE_CURRENT_SOURCE_DIR}/inspect/assets/dygraph-extra.js
)

set(HTTP_ASSETS
  ${CMAKE_CURRENT_SOURCE_DIR}/http/assets/jquery/js/jquery-1.10.2.min.js
  ${CMAKE_CURRENT_SOURCE_DIR}/http/assets/jquery/js/jquery-1.10.3.ui.min.js
  ${CMAKE_CURRENT_SOURCE_DIR}/http/assets/jquery/js/jquery.json-2.4.min.js
  ${CMAKE_CURRENT_SOURCE_DIR}/http/assets/bootstrap/js/bootstrap-2.2.1.min.js
  ${CMAKE_CURRENT_SOURCE_DIR}/http/assets/bootstrap/css/bootstrap-2.2.1.combined.min.css
)

add_custom_command(OUTPUT InspectAssets.h InspectAssets.cc
  COMMAND gen-assets
  ARGS InspectAssets ${INSPECT_ASSETS}
  COMMAND cp
  ARGS InspectAssets.h InspectAssets.cc ${CMAKE_CURRENT_SOURCE_DIR}/inspect
  MAIN_DEPENDENCY ${INSPECT_ASSETS}
  DEPENDS gen-assets
  VERBATIM )

add_custom_command(OUTPUT HttpAssets.h HttpAssets.cc
  COMMAND gen-assets
  ARGS HttpAssets ${HTTP_ASSETS}
  COMMAND cp
  ARGS HttpAssets.h HttpAssets.cc ${CMAKE_CURRENT_SOURCE_DIR}/http
  MAIN_DEPENDENCY ${HTTP_ASSETS}
  DEPENDS gen-assets
  VERBATIM )

set(netty_SRCS
    Buffer.cc
    IOStream.cc
    InetAddress.cc
    Socket.cc
    Acceptor.cc
    Connector.cc
    TcpConnection.cc
    TcpClient.cc
    TcpServer.cc
    UdpClient.cc
    UdpServer.cc
    resolver/ResolverFactory.cc
    resolver/StaticAddressResolver.cc
    resolver/DnsResolver.cc
    loadbalancer/LoadBalancerFactory.cc
    inspect/PProfInspector.cc
    inspect/FlagsInspector.cc
    inspect/StatisticsInspector.cc
    InspectAssets.cc
    http/Uri.cc
    http/MimeType.cc
    http/HttpRequest.cc
    http/HttpResponse.cc
    http/HttpConnection.cc
    http/HttpServer.cc
    http/HttpClient.cc
    HttpAssets.cc
  )

add_library(claire_netty ${netty_SRCS})
target_link_libraries(claire_netty claire_common gflags boost_regex pthread rt)

if(PPROF_LIBRARY)
    set_target_properties(claire_netty PROPERTIES COMPILE_FLAGS "-DHAVE_PPROF")
    target_link_libraries(claire_netty ${PPROF_LIBRARY})
endif()

if (CARES_FOUND)
    set_target_properties(claire_netty PROPERTIES COMPILE_FLAGS "-DHAVE_CARES")
    target_link_libraries(claire_netty ${CARES_LIBRARIES})
endif()

install(TARGETS claire_netty DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/claire/netty/)

file(GLOB RESOLVER_HEADERS "resolver/*.h")
install(FILES ${RESOLVER_HEADERS} DESTINATION include/claire/netty/resolver/)

file(GLOB LOADBALANCER_HEADERS "loadbalancer/*.h")
install(FILES ${LOADBALANCER_HEADERS} DESTINATION include/claire/netty/loadbalancer/)

file(GLOB INSPECT_HEADERS "inspect/*.h")
install(FILES ${INSPECT_HEADERS} DESTINATION include/claire/netty/inspect/)

file(GLOB HTTP_HEADERS "http/*.h")
install(FILES ${HTTP_HEADERS} DESTINATION include/claire/netty/http/)

if(NOT CMAKE_BUILD_NO_EXAMPLES)
  add_subdirectory(tests)
endif()
