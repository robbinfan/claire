#include <boost/bind.hpp>

#include <claire/netty/resolver/DnsResolver.h>
#include <claire/common/logging/Logging.h>
#include <claire/common/events/EventLoop.h>
#include <claire/netty/InetAddress.h>

using namespace claire;

EventLoop* g_loop = NULL;

void result(const StringPiece& s, const std::vector<InetAddress>& addrs)
{
    for (auto it = addrs.begin(); it != addrs.end(); ++it)
    {
        LOG(INFO) << "host " << s << " : " << (*it).ip();
    }

    g_loop->quit();
}

void resolve(Resolver* r, const StringPiece& s)
{
    r->Resolve(s.ToString(), boost::bind(&result, s, _1));
}

int main(int argc, char* argv[])
{
    EventLoop loop;
    g_loop = &loop;

    DnsResolver resolver;
    resolver.Init(&loop);
    resolve(&resolver, "www.google.com");
    loop.loop();
}
