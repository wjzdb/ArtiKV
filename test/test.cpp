#include <complex>
#include <string>
#include "gtest/gtest.h"
#include "art.hpp"
#include "slice.hpp"


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(HelloWorld, BasicAssertions) {
    EXPECT_STRNE("hello", "world");
    EXPECT_EQ(7 * 6, 42);
}

using namespace art;
using namespace std;

TEST(Art, Add){
    const char key[] = "hello";
    const char value[] = "world";
    
    auto art = ART();
    art.insert(key, std::string("world"));

    art.search(key);
}