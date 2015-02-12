resource_library(
    name = 'static_resource',
    srcs = [
        './assets/forms.html',
        './assets/forms.js',
        './assets/methods.html'
        ]
    )

cc_binary(
    name='protoc-gen-rpc',
    srcs=['./generator/protoc-gen-rpc.cc'],
    deps=['//thirdparty/protobuf:protobuf', '//thirdparty/protobuf:protoc', '#pthread'],
    dynamic_link=True
    )

gen_rule(
    name = 'gen-rpc-proto',
    srcs = ['rpcmessage.proto', 'builtin_service.proto'],
    cmd = 'sh -x claire/protorpc/gen-rpc.sh $BUILD_DIR',
    outs = ['builtin_service.pb.h', 'builtin_service.pb.cc', 'rpcmessage.pb.h', 'rpcmessage.pb.cc'],
    deps = ['//thirdparty/protobuf-2.6.1/src:protoc.bin', ':protoc-gen-rpc']
    )

cc_library(
    name = 'claire_protorpc',
    srcs = [
        'BuiltinService.cc',
        'RpcChannel.cc',
        'RpcCodec.cc',
        'RpcController.cc',
        'RpcServer.cc',
        'RpcUtil.cc',
        'builtin_service.pb.cc',
        'rpcmessage.pb.cc'
    ],
    deps = [
        '//claire/common:claire_common',
        '//claire/netty:claire_netty',
        ':static_resource',
        ':gen-rpc-proto',
        '//thirdparty/protobuf-2.6.1/src:protobuf'
    ]
)
