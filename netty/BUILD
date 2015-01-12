resource_library(
    name = 'static_resource',
    srcs = [
        './inspect/assets/flags.js',
        './inspect/assets/flags.html',
        './inspect/assets/graphview.html',
        './inspect/assets/grapher.js',
        './inspect/assets/parser.js',
        './inspect/assets/dygraph-combined.js',
        './inspect/assets/dygraph-extra.js',
        './http/assets/jquery/js/jquery-1.10.2.min.js',
        './http/assets/jquery/js/jquery-1.10.3.ui.min.js',
        './http/assets/jquery/js/jquery.json-2.4.min.js',
        './http/assets/bootstrap/js/bootstrap-2.2.1.min.js',
        './http/assets/bootstrap/css/bootstrap-2.2.1.combined.min.css'
    ]
)

cc_library(
    name = 'claire_netty',
    srcs = [
        'Buffer.cc',
        'IOStream.cc',
        'InetAddress.cc',
        'Socket.cc',
        'Acceptor.cc',
        'Connector.cc',
        'TcpConnection.cc',
        'TcpClient.cc',
        'TcpServer.cc',
        'UdpClient.cc',
        'UdpServer.cc',
        './resolver/ResolverFactory.cc',
        './resolver/StaticAddressResolver.cc',
        './resolver/DnsResolver.cc',
        './loadbalancer/LoadBalancerFactory.cc',
        './inspect/PProfInspector.cc',
        './inspect/FlagsInspector.cc',
        './inspect/StatisticsInspector.cc',
        './http/Uri.cc',
        './http/MimeType.cc',
        './http/HttpRequest.cc',
        './http/HttpResponse.cc',
        './http/HttpConnection.cc',
        './http/HttpServer.cc',
        './http/HttpClient.cc'
    ],
    deps = [':static_resource', '#pthread', '#rt', '//thirdparty/ctemplate:ctemplate', '//claire/common:claire_common'],
    extra_cppflags = '-std=c++0x'
)
