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

    Value a = BOOL_VAL(true);
    Value b = BOOL_VAL(true);

    auto key1 = newString("cat");
    printObject(std::cout, OBJ_VAL(key1));
    std::cout << "\n";
    fmt::print("hash: {}\n", key1->hash);

    auto key2 = newString("dog");

    Table table;
    table.set(key1, a);
    table.set(key2, b);

    auto key3 = newString("cat");
    printObject(std::cout, OBJ_VAL(key3));
    std::cout << "\n";
    fmt::print("hash: {}\n", key3->hash);

    Value v;
    auto  res = table.get(key3, &v);
    EXPECT_EQ(res, true);
    EXPECT_EQ(IS_BOOL(v), true);
    EXPECT_EQ(AS_BOOL(v), true);
}
