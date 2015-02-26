#include <claire/examples/zipkin/sort.pb.h>

#include <boost/bind.hpp>

#include <claire/protorpc/RpcServer.h>
#include <claire/common/events/EventLoop.h>
#include <claire/common/logging/Logging.h>
#include <claire/netty/InetAddress.h>

#include <claire/common/tracing/Tracing.h>
#include <claire/zipkin/ZipkinTracer.h>

using namespace claire;
using namespace claire::protorpc;

namespace sort {

class StdSortServiceImpl : public StdSortService
{
public:
    virtual void StdSort(RpcControllerPtr& controller,
                         const ::sort::SortRequestPtr& request,
                         const ::sort::SortResponse* response_protoType,
                         const RpcDoneCallback& done)
    {
        SortResponse response;
        std::vector<int32_t> v;
        for (int i = 0;i < request->nums_size(); i++)
        {
            v.push_back(request->nums(i));
        }
        std::sort(v.begin(), v.end());

        for (auto it = v.cbegin(); it != v.cend(); ++it)
        {
            response.add_nums(*it);
        }
        done(controller, &response);
    }
};

} // namespace sort

int main(int argc, char* argv[])
{
    ::gflags::ParseCommandLineFlags(&argc, &argv, true);
    InitClaireLogging(argv[0]);

    ZipkinTracer tracer(InetAddress(9410));
    InstallClaireTracer(&tracer);

    EventLoop loop;
    InetAddress listen_address(8082);
    sort::StdSortServiceImpl impl;
    RpcServer server(&loop, listen_address);
    server.RegisterService(&impl);
    server.Start();
    loop.loop();
}

