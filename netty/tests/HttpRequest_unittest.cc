#include <claire/netty/http/HttpConnection.h>
#include <claire/netty/http/HttpRequest.h>
#include <claire/netty/TcpConnection.h>
#include <claire/netty/Buffer.h>

#include <gtest/gtest.h>

using namespace claire;

TEST(HttpRequestTest, ParseRequestAllInOne)
{
    TcpConnectionPtr conn;
    HttpConnection context(conn);
    Buffer input;
    input.Append("GET /index.html HTTP/1.1\r\n"
         "Host: robbinfan.github.com\r\n"
         "\r\n");

    EXPECT_TRUE(context.Parse<HttpRequest>(&input));
    EXPECT_TRUE(context.GotAll());

    const auto& request = context.mutable_request();
    EXPECT_EQ(request->method(), HttpRequest::kGet);
    EXPECT_EQ(request->uri().path(), std::string("/index.html"));
    EXPECT_EQ(request->version(), HttpMessage::kHttp11);
    EXPECT_EQ(request->get_header("Host"), std::string("robbinfan.github.com"));
    EXPECT_EQ(request->get_header("User-Agent"), std::string(""));
}

TEST(HttpRequestTest, ParseRequestInTwoPieces)
{
    std::string all("GET /index.html HTTP/1.1\r\n"
         "Host: robbinfan.github.com\r\n"
         "\r\n");

    for (size_t sz1 = 0; sz1 < all.size(); ++sz1)
    {
        TcpConnectionPtr conn;
        HttpConnection context(conn);
        Buffer input;
        input.Append(all.c_str(), sz1);
        EXPECT_TRUE(context.Parse<HttpRequest>(&input));
        EXPECT_TRUE(!context.GotAll());

        size_t sz2 = all.size() - sz1;
        input.Append(all.c_str() + sz1, sz2);
        EXPECT_TRUE(context.Parse<HttpRequest>(&input));
        EXPECT_TRUE(context.GotAll());

        const auto& request = context.mutable_request();
        EXPECT_EQ(request->method(), HttpRequest::kGet);
        EXPECT_EQ(request->uri().path(), std::string("/index.html"));
        EXPECT_EQ(request->version(), HttpRequest::kHttp11);
        EXPECT_EQ(request->get_header("Host"), std::string("robbinfan.github.com"));
        EXPECT_EQ(request->get_header("User-Agent"), std::string(""));
    }
}

TEST(HttpRequestTest, ParseRequestEmptyHeaderValue)
{
    TcpConnectionPtr conn;
    HttpConnection context(conn);
    Buffer input;
    input.Append("GET /index.html HTTP/1.1\r\n"
         "Host: robbinfan.github.com\r\n"
         "User-Agent:\r\n"
         "Accept-Encoding: \r\n"
         "\r\n");

    EXPECT_TRUE(context.Parse<HttpRequest>(&input));
    EXPECT_TRUE(context.GotAll());

    const auto& request = context.mutable_request();
    EXPECT_EQ(request->method(), HttpRequest::kGet);
    EXPECT_EQ(request->uri().path(), std::string("/index.html"));
    EXPECT_EQ(request->version(), HttpRequest::kHttp11);
    EXPECT_EQ(request->get_header("Host"), std::string("robbinfan.github.com"));
    EXPECT_EQ(request->get_header("User-Agent"), std::string(""));
    EXPECT_EQ(request->get_header("Accept-Encoding"), std::string(""));
}

