#include <Private/Grace/ScratchVector.hpp>

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

int main()
{
    const Grace::ScratchVector<Foo> foos(5);
    const Grace::ScratchVector<Bar> bars(300);

    return 0;
}