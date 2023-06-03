//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#include "object.hh"
#include "table.hh"

#include <fmt/core.h>
#include <gtest/gtest.h>

using namespace alox;

TEST(Table, basic) { // NOLINT

    Value a = value<bool>(true);
    Value b = value<bool>(true);

    auto key1 = newString("cat");
    printObject(std::cout, value<Obj *>(key1));
    std::cout << "\n";
    fmt::print("hash: {}\n", key1->hash);

    auto key2 = newString("dog");

    Table table;
    table.set(key1, a);
    table.set(key2, b);

    auto key3 = newString("cat");
    printObject(std::cout, value<Obj *>(key3));
    std::cout << "\n";
    fmt::print("hash: {}\n", key3->hash);

    Value v;
    auto  res = table.get(key3, &v);
    EXPECT_EQ(res, true);
    EXPECT_EQ(is<bool>(v), true);
    EXPECT_EQ(as<bool>(v), true);
}
