#include <Private/Grace/ScratchVector.hpp>

#include <gtest/gtest.h>

TEST(ScratchVector, Test)
{
    struct Foo
    {
        uint32_t a;
        uint64_t b;
    };

    struct Bar
    {
        uint32_t a[7];
        uint64_t b;
    };

    const Grace::ScratchVector<Foo> foos(5);
    const Grace::ScratchVector<Bar> bars(300);

    EXPECT_EQ(foos.size(), 5);
}
