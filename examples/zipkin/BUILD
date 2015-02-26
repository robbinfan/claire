protorpc_library(
    name = 'sort',
    srcs = 'sort.proto',
)

cc_binary(
    name = 'sort_client',
    srcs = 'client.cc',
    deps = [':sort', '//claire/zipkin:claire_zipkin', '#thrift']
)

cc_binary(
    name = 'sort_server',
    srcs = 'server.cc',
    deps = [':sort', '//claire/zipkin:claire_zipkin', '#thrift']
)

cc_binary(
    name = 'sort_transfer',
    srcs = 'transfer.cc',
    deps = [':sort', '//claire/zipkin:claire_zipkin', '#thrift']
)
