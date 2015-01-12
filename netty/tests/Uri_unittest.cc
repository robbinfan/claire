#include <claire/netty/http/Uri.h>
#include <claire/common/strings/UriEscape.h>

#include <gtest/gtest.h>

using namespace claire;

TEST(UriTest, UriSimple)
{
    std::string s("http://t.qq.com/hello/world?query#fragment");
    Uri u;

    EXPECT_EQ(true, u.Parse(s));
    EXPECT_EQ("http", u.scheme());
    EXPECT_EQ("", u.username());
    EXPECT_EQ("", u.password());
    EXPECT_EQ("t.qq.com", u.host());
    EXPECT_EQ(0, u.port());
    EXPECT_EQ("/hello/world", u.path());
    EXPECT_EQ("query", u.query());
    EXPECT_EQ("fragment", u.fragment());
    EXPECT_EQ(s, u.ToString());  // canonical
}

TEST(UriTest, UriRelative)
{
    std::string s("/protorpc?x=1&y=2");
    Uri u;

    EXPECT_EQ(true, u.Parse(s));
    EXPECT_EQ("/protorpc", u.path());
    EXPECT_EQ("x=1&y=2", u.query());
    EXPECT_EQ(s, u.ToString());
}

TEST(UriTest, UriEscape)
{
    EXPECT_EQ("hello%2C%20%2Fworld", UriEscape("hello, /world", EscapeMode::ALL));
    EXPECT_EQ("hello%2C%20/world", UriEscape("hello, /world", EscapeMode::PATH));
    EXPECT_EQ("hello%2C+%2Fworld", UriEscape("hello, /world", EscapeMode::QUERY));
}

TEST(UriTest, UriUnescape)
{
    EXPECT_EQ("hello, /world", UriUnescape("hello, /world", EscapeMode::ALL));
    EXPECT_EQ("hello, /world", UriUnescape("hello%2c%20%2fworld", EscapeMode::ALL));
    EXPECT_EQ("hello,+/world", UriUnescape("hello%2c+%2fworld", EscapeMode::ALL));
    EXPECT_EQ("hello, /world", UriUnescape("hello%2c+%2fworld", EscapeMode::QUERY));
    EXPECT_EQ("hello/", UriUnescape("hello%2f", EscapeMode::ALL));
    EXPECT_EQ("hello/", UriUnescape("hello%2F", EscapeMode::ALL));
    //EXPECT_THROW(UriUnescape("hello%"), std::invalid_argument);
    //EXPECT_THROW(UriUnescape("hello%2"), std::invalid_argument);
    //EXPECT_THROW(UriUnescape("hello%2g"), std::invalid_argument);
}

