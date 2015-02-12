$1/thirdparty/protobuf-2.6.1/src/protoc.bin --plugin=$1/claire/protorpc/protoc-gen-rpc --rpc_out=claire/protorpc/ claire/protorpc/rpcmessage.proto -Iclaire/protorpc -Ithirdparty
$1/thirdparty/protobuf-2.6.1/src/protoc.bin --plugin=$1/claire/protorpc/protoc-gen-rpc --rpc_out=claire/protorpc/ claire/protorpc/builtin_service.proto -Iclaire/protorpc -Ithirdparty
sed -i "s/#include <google\\/protobuf\\/service.h>/#include <claire\\/protorpc\\/service.h>/" claire/protorpc/builtin_service.pb.h
sed -i "s/#include .rpcmessage.pb.h./#include <claire\\/protorpc\\/rpcmessage.pb.h>/" claire/protorpc/builtin_service.pb.h
sed -i "/protoc_insertion_point.includes./i#include <boost/shared_ptr.hpp>" claire/protorpc/rpcmessage.pb.h


