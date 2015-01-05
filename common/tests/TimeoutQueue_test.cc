#include <claire/common/events/EventLoop.h>
#include <claire/common/events/EventLoopThread.h>

#include <boost/bind.hpp>

#include <stdio.h>
#include <unistd.h>

using namespace claire;

int cnt = 0;
EventLoop* g_loop;

void print(const char* msg)
{
    printf("msg %s %s\n", Timestamp::Now().ToFormattedString().c_str(), msg);
    if (++cnt == 20)
    {
        g_loop->quit();
    }
}

void cancel(TimerId timer)
{
  g_loop->Cancel(timer);
  printf("cancelled at %s\n", Timestamp::Now().ToString().c_str());
}

int main()
{
    sleep(1);
    {
        EventLoop loop;
        g_loop = &loop;

        loop.RunAfter(1000, boost::bind(print, "once1"));
        loop.RunAfter(1500, boost::bind(print, "once1.5"));
        loop.RunAfter(2000, boost::bind(print, "once2"));
        loop.RunAfter(2500, boost::bind(print, "once2.5"));
        auto id = loop.RunAfter(3000, boost::bind(print, "once3"));
        loop.RunAfter(2700, boost::bind(cancel, id));

        auto id2 = loop.RunEvery(4000, boost::bind(print, "every4"));
        loop.RunAfter(9000, boost::bind(cancel, id2));
        loop.RunAfter(9500, boost::bind(cancel, id2));

        loop.RunEvery(2000, boost::bind(print, "every2"));
        loop.loop();
        print("main loop exits");
    }

    return 0;
}

