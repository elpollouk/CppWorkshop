/*
 * A pool allocator can be used when you know in advance the maximum number of items of a
 * particular type you will need. You can allocate the memory for the items in advance saving
 * the overheads of hitting the system allocator and avoid the potential of memory fragmentation.
 * 
 * On extremely low memory devices (e.g. previous generation games consoles), memory fragmentation
 * becomes an issue over time if just using the system heap. You can find yourself in a situation
 * where there is technically enough free memory to allocate a large object, but no single
 * contiguous block of memory large enough for the allocation.
 */
#include "pch.h"
#include "Allocators/PoolAllocator.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Pointers
{
    TEST_CLASS(E05_PooledAllocatorExample)
    {
        class Tank
        {
        public:
            Tank() :
                x(0),
                y(0),
                z(0)
            {

            }

            Tank(int x, int y, int z)
            {
                this->x = x;
                this->y = y;
                this->z = z;
            }

            int check() const { return x + y + z; }

            int x;
            int y;
            int z;
        };

    public:
        TEST_METHOD(Direct_Pool_Usage)
        {
            // Create a pool allocator that has a fixed number of items.
            PoolAllocator<Tank, 3> pool;
            Assert::AreEqual(3, (int)pool.getPoolSize());
            Assert::AreEqual(3u, pool.getFreeCount());
            Assert::AreEqual(0u, pool.getAllocCount());

            // Construct items directly from the pool.
            auto t1 = pool.construct(1, 2, 3);
            Assert::AreEqual(6, t1->check(), L"Tank 1 wasn't constructed correctly");
            auto t2 = pool.construct(4, 5, 6);
            Assert::AreEqual(15, t2->check(), L"Tank 2 wasn't constructed correctly");
            auto t3 = pool.construct(7, 8, 9);
            Assert::AreEqual(24, t3->check(), L"Tank 3 wasn't constructed correctly");
            Assert::AreEqual(0u, pool.getFreeCount());
            Assert::AreEqual(3u, pool.getAllocCount());

            // When the pool limit is reached, it should not be possible to allocate any further
            // items.
            AssertThrows<std::bad_alloc>([&pool]() {
                pool.construct();
            }, L"No more allocations should be possible from pool");

            // Items from the pool must be destructed via the pool
            pool.destruct(t1);
            Assert::AreEqual(1u, pool.getFreeCount());
            Assert::AreEqual(2u, pool.getAllocCount());

            // The pool guards against double deletions
            AssertThrows<std::invalid_argument>([&pool, t1]() {
                pool.destruct(t1);
            }, L"It should not be possible to double destruct element from pool");

            // Now an item has been returned to the pool, we a construct another new item again.
            t1 = pool.construct();
            Assert::AreEqual(0, t1->check(), L"Default constructor wasn't invoked correctly");
            Assert::AreEqual(0u, pool.getFreeCount());
            Assert::AreEqual(3u, pool.getAllocCount());

            // The pool also attempts to guard against deleting allocations that didn't originate
            // from within the pool.
            Tank invalidAllocation;
            AssertThrows<std::invalid_argument>([&pool, &invalidAllocation]() {
                pool.destruct(&invalidAllocation);
            }, L"It should not be possible to destruct element not from pool");

            // Deleting everything should return the pool to its default state.
            pool.destruct(t1);
            pool.destruct(t2);
            pool.destruct(t3);
            Assert::AreEqual(3u, pool.getFreeCount());
            Assert::AreEqual(0u, pool.getAllocCount());
        }
    };
}