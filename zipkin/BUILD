thrift_library(
    name = 'fb303',
    srcs = 'fb303.thrift'
)

thrift_library(
    name = 'scribe',
    srcs = 'scribe.thrift'
)

thrift_library(
    name = 'zipkinCore',
    srcs = 'zipkinCore.thrift',
    deps = ':scribe'
)

cc_library(
    name = 'claire_zipkin',
    srcs = [
        'ZipkinTracer.cc',
        'ScribeClient.cc'
    ],
    deps = [':zipkinCore']
)



