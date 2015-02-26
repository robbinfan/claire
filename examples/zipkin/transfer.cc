#include <claire/examples/zipkin/sort.pb.h>
#include <claire/examples/zipkin/stdsort_client.h>

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

class SortTransferServiceImpl : public SortService
{
public:
    SortTransferServiceImpl(EventLoop* loop, const InetAddress& server_address)
        : client_(loop, server_address) {}

    virtual void Sort(RpcControllerPtr& controller,
                      const ::sort::SortRequestPtr& request,
                      const ::sort::SortResponse* response_prototype,
                      const RpcDoneCallback& done)
    {
        if (request->nums_size() == 0)
        {
            controller->SetFailed("Empty Request");
            done(controller, NULL);
            return ;
        }

        std::vector<int32_t> v;
        for (int i = 0;i < request->nums_size(); i++)
        {
            v.push_back(request->nums(i));
        }
       
        RpcControllerPtr sub_controller(new RpcController());
        sub_controller->set_parent(controller);
        client_.SendRequest(sub_controller, v, boost::bind(&SortTransferServiceImpl::OnResponse, this, _1, controller, done));
    }

private:
    void OnResponse(const std::vector<int32_t>& v, RpcControllerPtr& controller, const RpcDoneCallback& done)
    {
        if (v.empty())
        {
            controller->SetFailed("Empty Response");
            done(controller, NULL);
            return ;
        }

        SortResponse response;
        for (auto it = v.cbegin(); it != v.cend(); ++it)
        {
            response.add_nums(*it);
        }
        done(controller, &response);
    }

    StdSortClient client_;
};

} // namespace sort

int main(int argc, char* argv[])
{
    ::gflags::ParseCommandLineFlags(&argc, &argv, true);
    InitClaireLogging(argv[0]);

    ZipkinTracer tracer(InetAddress(9410));
    InstallClaireTracer(&tracer);

    EventLoop loop;
    InetAddress listen_address(8081);
    InetAddress server_address(8082);

    sort::SortTransferServiceImpl impl(&loop, server_address);
    RpcServer server(&loop, listen_address);
    server.RegisterService(&impl);
    server.Start();
    loop.loop();
}

