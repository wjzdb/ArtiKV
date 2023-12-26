#include <iostream>
#include <format>
#include "slice.hpp"

using namespace art;


int main() {
    std::cout << "Hello, world!\n";
    auto value_left_value = std::string("abc");
    auto slice = OwnedSlice(std::move(value_left_value));
    return 0;
}
