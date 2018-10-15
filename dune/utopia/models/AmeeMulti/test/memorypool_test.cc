#include <dune/utopia/models/AmeeMulti/utils/memorypool.hh>
#include <dune/utopia/models/AmeeMulti/utils/test_utils.hh>
#include <dune/utopia/models/AmeeMulti/utils/utils.hh>

#include <iostream>

int main()
{
    using namespace Utopia;
    MemoryPool<int> mempool(10);

    // allocate stuff
    ASSERT_EQ(mempool.free_pointers().size(), std::size_t(10));
    ASSERT_EQ(mempool.size(), std::size_t(10));

    auto ptr1 = mempool.allocate();
    ASSERT_EQ(mempool.free_pointers().size(), std::size_t(9));

    ptr1 = mempool.construct(ptr1, 1);
    ASSERT_EQ(*ptr1, 1);

    std::vector<int*> pointers(9);
    for (int i = 0; i < 9; ++i)
    {
        auto ptr = mempool.allocate();
        pointers[i] = mempool.construct(ptr, i);
        ASSERT_EQ(*ptr, i);
    }

    for (auto ptr : pointers)
    {
        mempool.destroy(ptr);
    }

    ASSERT_EQ(mempool.free_pointers().size(), 0ul);

    // overallocate -> pool increase to 20
    auto ptr2 = mempool.allocate();
    ptr2 = mempool.construct(ptr2, 12);
    ASSERT_EQ(*ptr2, 12);
    ASSERT_EQ(mempool.size(), 20ul);
    ASSERT_EQ(mempool.free_pointers().size(), 9ul);

    mempool.deallocate(ptr2);
    mempool.deallocate(ptr1);

    ASSERT_EQ(mempool.free_pointers().size(), 11ul);

    for (auto& ptr : pointers)
    {
        mempool.deallocate(ptr);
    }
    ASSERT_EQ(mempool.free_pointers().size(), 20ul);

    return 0;
}