
#include <gtest/gtest.h>

TEST(NetCdfParser, base){
    EXPECT_EQ("Foo", "Foo");
}

TEST(NetCdfParser, false){
    EXPECT_EQ("Foo", "Bar");
}
