#include <dune/utopia/models/AmeeMulti/utils/memorypool.hh>
#include <dune/utopia/models/AmeeMulti/utils/test_utils.hh>
#include <dune/utopia/models/AmeeMulti/utils/utils.hh>

#include <iostream>

int main()
{
    using namespace Utopia;
    MemoryPool<int> mempool(10);

    // allocate stuff
    ASSERT_EQ(mempool.free_pointers().size(), 10ul);
    ASSERT_EQ(mempool.size(), 10ul);

    auto ptr1 = mempool.allocate();
    ASSERT_EQ(mempool.free_pointers().size(), 9ul);

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

    try
    {
        auto ptr2 = mempool.allocate();
    }
    catch (...)
    {
        std::cerr << "caught alloc exception" << std::endl;
    }

    // trying to copy
    auto mempool2(mempool);
    ASSERT_EQ(mempool.free_pointers().size(), mempool.free_pointers().size());
    ASSERT_EQ(mempool.size(), mempool2.size());

    mempool.clear();
    std::vector<int*> integers(mempool.size());
    for (std::size_t i = 0; i < mempool.size(); ++i)
    {
        integers[i] = mempool.construct(mempool.allocate(), -1 * int(i));
    }
    ASSERT_EQ(mempool.free_pointers().size(), 0ul);
    ASSERT_EQ(mempool.free_pointers().empty(), true);
    return 0;
}