#ifndef _EXAMPLES_SORT_CLIENT_H_
#define _EXAMPLES_SORT_CLIENT_H_

#include <claire/examples/zipkin/sort.pb.h>

#include <vector>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <claire/common/logging/Logging.h>
#include <claire/common/events/EventLoop.h>
#include <claire/netty/InetAddress.h>

#include <claire/protorpc/RpcChannel.h>

namespace claire {

class SortClient : boost::noncopyable
{
public:
    typedef boost::function<void(const std::vector<int32_t>&)> ResultCallback;
    SortClient(EventLoop* loop,
               const InetAddress& server_address)
      : channel_(loop),
        stub_(&channel_)
    {
        channel_.Connect(server_address);
    }

    void SendRequest(const std::vector<int32_t>& v, const ResultCallback& done)
    {
        protorpc::RpcControllerPtr controller(new protorpc::RpcController());
        SendRequest(controller, v, done);
    }

    void SendRequest(protorpc::RpcControllerPtr& controller, const std::vector<int32_t>& v, const ResultCallback& done)
    {
        sort::SortRequest request;
        for (auto it = v.cbegin(); it != v.cend(); ++it)
        {
            request.add_nums(*it);
        }
        stub_.Sort(controller, request, boost::bind(&SortClient::replied, this, _1, _2, done));
    }

private:
    void replied(protorpc::RpcControllerPtr& controller, const boost::shared_ptr<sort::SortResponse>& response, const ResultCallback& done)
    {
        std::vector<int32_t> v;
        if (controller->Failed())
        {
            LOG(ERROR) << controller->ErrorText();
        }
        else
        {
            for (int i = 0;i < response->nums_size(); i++)
            {
                v.push_back(response->nums(i));
            }
        }
        done(v);
    }

    protorpc::RpcChannel channel_;
    sort::SortService::Stub stub_;
};

} // namespace claire

#endif // _EXAMPLES_SORT_CLIENT_H_
