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

    auto ptr2 = mempool.allocate(5);
    ASSERT_EQ(mempool.free_pointers().size(), std::size_t(4));
    for (std::size_t i = 0; i < 4; ++i)
    {
        auto ptrx = ptr2 + i;
        ptrx = mempool.construct(ptr2 + i, i);
    }

    // for (std::size_t i = 0; i < 4; ++i)
    // {
    //     // std::cout << ptr2[i] << std::endl;
    //     ASSERT_EQ(ptr2[i], int(i));
    // }

    // // deallocate and reallocate stuff
    // for (std::size_t i = 0; i < 3; ++i)
    // {
    //     mempool.destroy(ptr2 + i);
    //     mempool.deallocate(ptr2 + i);
    // }

    // ASSERT_EQ(mempool.free_pointers().size(), std::size_t(7));

    // std::vector<int*> bucket(6);
    // for (int i = 0; i < 6; ++i)
    // {
    //     // std::cout << "i = " << i << "," << mempool.free_pointers().size() << std::endl;
    //     bucket[i] = mempool.construct(mempool.allocate(1), i + 10);
    // }

    // for (int i = 0; i < 6; ++i)
    // {
    //     ASSERT_EQ(*bucket[i], i + 10);
    // }
    // ASSERT_EQ(mempool.free_pointers().size(), std::size_t(1));

    // auto largeptr = mempool.allocate(25);
    // ASSERT_EQ(mempool.size(), std::size_t(40));

    // // allocate size-40-buffer, fill free_pointers of size 6 -> free_pointer_size = 31 -> allocate size 25 -> free_pointer size = 6
    // ASSERT_EQ(mempool.free_pointers().size(), std::size_t(6));

    // // cannot do shit with newly allocated thing!
    // for (std::size_t i = 0; i < 25; ++i)
    // {
    //     auto ptrx = &largeptr[i];
    //     ptrx = mempool.construct(ptrx, i * 2);
    // }

    // for (std::size_t i = 0; i < 25; ++i)
    // {
    //     ASSERT_EQ(largeptr[i], int(i * 2));
    // }

    return 0;
}