protorpc_library(
    name = 'echo',
    srcs = 'echo.proto',
)

cc_binary(
    name = 'echo_client',
    srcs = 'client.cc',
    deps = ':echo'
)

cc_binary(
    name = 'echo_server',
    srcs = 'server.cc',
    deps = ':echo'
)
