#include <claire/netty/http/HttpConnection.h>
#include <claire/netty/http/HttpResponse.h>
#include <claire/netty/Buffer.h>

#include <gtest/gtest.h>

using namespace claire;

TEST(HttpResponseTest, SimpleHeader)
{
    TcpConnectionPtr ptr;
    HttpConnection conn(ptr);
    Buffer buf;
    buf.Append("HTTP/1.1 200 OK\r\n");
    buf.Append("Content-Length: 45\r\n");
    buf.Append("Connection: Keep-Alive\r\n");
    buf.Append("\r\n");

    EXPECT_TRUE(conn.Parse<HttpResponse>(&buf));
    EXPECT_EQ(conn.mutable_response()->headers().size(), 2);
}

TEST(HttpResponseTest, NonStandardHeader)
{
    TcpConnectionPtr ptr;
    HttpConnection conn(ptr);
    Buffer buf;

    buf.Append("HTTP/1.1 200 OK\r\n");
    buf.Append("Content-Length: 45\r\n");
    buf.Append("Connection: Keep-Alive\r\n");
    buf.Append("HTTP/1.1 200 OK\r\n");
    buf.Append("Content-Type: text/html\r\n");
    buf.Append("\r\n");

    EXPECT_TRUE(conn.Parse<HttpResponse>(&buf));
    EXPECT_EQ(conn.mutable_response()->headers().size(), 3);
}

TEST(HttpResponseTest, SimpleBody)
{
    TcpConnectionPtr ptr;
    HttpConnection conn(ptr);
    Buffer buf;
    buf.Append("HTTP/1.1 200 OK\r\n");
    buf.Append("Content-Length: 9\r\n");
    buf.Append("Content-Type: text/html\r\n");
    buf.Append("\r\n");
    buf.Append("robbinfan");

    EXPECT_TRUE(conn.Parse<HttpResponse>(&buf));
    EXPECT_EQ(conn.mutable_response()->headers().size(), 2);
    EXPECT_EQ(*conn.mutable_response()->mutable_body(), "robbinfan");
}

TEST(HttpResponseTest, HeadersToString)
{
    HttpResponse response;
    response.AddHeader("Content-Length", "10000");
    response.AddHeader("Connection", "keep-alive");
    auto headers = response.headers().ToString();
    EXPECT_EQ("Content-Length: 10000\r\nConnection: keep-alive\r\n\r\n",
              headers);

}

