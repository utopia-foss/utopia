#include <dune/utopia/models/AmeeMulti/utils/memorypool.hh>
#include <dune/utopia/models/AmeeMulti/utils/test_utils.hh>
#include <iostream>

struct Thing
{
    int a;
    double b;
    std::string c;

    Thing() = default;
    Thing(int A, double B, std::string C) : a(A), b(B), c(C)
    {
    }
    Thing(const Thing&) = default;
    Thing(Thing&&) = default;
    Thing& operator=(const Thing&) = default;
    Thing& operator=(Thing&&) = default;
}

int main()
{
    using namespace Utopia;
    MemoryPool<int> mempool(10);

    // allocate stuff
    ASSERT_EQ(mempool.free_pointers().size(), 10);
    ASSERT_EQ(mempool.size(), 10);
    auto ptr1 = mempool.allocate();
    ASSERT_EQ(mempool.free_pointers().size(), 9);

    ptr1 = mempool.construct(1);
    ASSERT_EQ(*ptr1, 1);

    auto ptr2 = mempool.allocate(5);
    ASSERT_EQ(mempool.free_pointers().size(), 4);
    for (std::size_t i = 0; i < 4; ++i)
    {
        ptr2 + i = mempool.construct(ptr2 + i, i);
    }

    for (std::size_t i = 0; i < 4; ++i)
    {
        std::cout << ptr2[i] << std::endl;
        ASSERT_EQ(ptr2[i], i);
    }

    // deallocate and reallocate stuff
    for (std::size_t i = 0; i < 3; ++i)
    {
        mempool.destroy(ptr2 + i);
        mempool.deallocate(ptr2 + i);
    }

    ASSERT_EQ(mempool.free_pointers().size(), 7);

    // use with some struct stuff
    MemoryPool<Thing> thingmempool(100);

    return 0;
}