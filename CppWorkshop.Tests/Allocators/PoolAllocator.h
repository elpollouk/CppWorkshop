/*
 * An example custom allocator that creates items out of a fixed sized pool of items.
 * This allocator isn't as flexible as a general purpose allocator as it can't allocate arrays of
 * items, only single items per allocation request.
 * The pool does however support verifying release requests and providing shared_ptr/unique_ptr
 * wrappers in addition to raw pointers.
 */
#include <new>
#include <stdexcept>

// The number of items is provided as a template parameter so that the whole pool can be created
// from a single large allocation if being created dynamically.
template<class type, size_t pool_size>
class PoolAllocator
{
public:
	// This typedef is for my own benefit and saves duplicate type declarations when defining
	// the Deletor below.
	typedef PoolAllocator<type, pool_size> pool_type;

	// A functor to wrap deleter functionality for a specific pool instance. This is used when
	// creating shared_ptr/unique_ptr to route delete requests back to the correct pool.
	class Deletor final
	{
	public:
		Deletor(pool_type* pPool) noexcept:
			_pPool(pPool) {}

		Deletor(const Deletor& other) noexcept:
			_pPool(other._pPool) {}

		Deletor(Deletor&& other) noexcept :
			_pPool(std::exchange(other._pPool, nullptr)) {}

		void operator()(type* pMem)
		{
			_pPool->destruct(pMem);
		}

	private:
		pool_type* _pPool;
	};

	typedef std::unique_ptr<type, Deletor> unique_ptr;

	PoolAllocator()
	{
		reset();
	}

	void reset()
	{
		// To save us from having to search the pool for a free allocation, all the pool slots are
		// added to a linked list. We can then take the head item when we need a new allocation and
		// push a new head item when we de-allocate.
		_allocation_count = 0;
		_next_free = nullptr;
		for (size_t i = 0; i < pool_size; i++)
		{
			_pool[i].next = _next_free;
			_next_free = &_pool[i];
		}
	}

	size_t getPoolSize() const { return pool_size; }
	unsigned int getFreeCount() const { return (unsigned int)(pool_size - _allocation_count); }
	unsigned int getAllocCount() const { return (unsigned int)_allocation_count; }

	template <class... _Types>
	type* construct(_Types&&... _Args)
	{
		if (_allocation_count == pool_size) throw std::bad_alloc();
		_allocation_count++;
		auto allocation = _next_free;
		_next_free = allocation->next;
		// We set the next pointer to a statically allocated item specific to this pool to indicate
		// that this slot is now in use. We verify this pointer when releasing an allocation so
		// that we have a simple check that the pointer we're attempting to release is valid.
		// We have to cast away the const as the marker object is created using constexpr which is
		// required until C++17 which supports "static inline" for this type of scenario.
		allocation->next = const_cast<PoolEntry*>(&ENTRY_IN_USE);
		return new(allocation->mem) type(std::forward<_Types>(_Args)...);
	}

	void destruct(type* pMem)
	{
		PoolEntry* pEntry = getEntry(pMem);
		verifyEntryWithinPool(pEntry);
		// As we constructed the item in the pool, it is also our responsibility to destruct them.
		pMem->~type();
		pEntry->next = _next_free;
		_next_free = pEntry;
		_allocation_count--;
	}

	template <class... _Types>
	std::shared_ptr<type> make_shared(_Types&&... _Args)
	{
		type* pItem = construct(_Args...);
		return std::shared_ptr<type>(pItem, Deletor(this));
	}

	template <class... _Types>
	unique_ptr make_unique(_Types&&... _Args)
	{
		type* pItem = construct(_Args...);
		return unique_ptr(pItem, Deletor(this));
	}

private:
	struct PoolEntry
	{
		PoolEntry* next;
		// The memory for the item is stored inline within the PoolEntry.
		// I allocated using a char array rather that type as there may not be a safe default
		// constructor for the type.
		char mem[sizeof(type)];
	};

	// A marker item we use to mark a slot once we've allocated it.
	static constexpr PoolEntry ENTRY_IN_USE = PoolEntry();

	PoolEntry* getEntry(type* pMem)
	{
		// We use basic pointer arithmetic to get back to the start of the PoolEntry.
		// If we added more items to the PoolEntry struct, we'd need to update this calculation.
		auto raw = reinterpret_cast<char*>(pMem);
		raw -= sizeof(PoolEntry*);
		return reinterpret_cast<PoolEntry*>(raw);
	}

	void verifyEntryWithinPool(PoolEntry* pEntry)
	{
		// First check that the pointer's address is within the address range of this pool.
		if (pEntry < _pool || pEntry >= &_pool[pool_size]) throw std::invalid_argument("Allocation is not within this pool");
		// Then check that the entry was marked correctly. This guards against trying to release
		// an already released allocation or passing an invaid pointer that is still within the pool's
		// address range.
		if (pEntry->next != &ENTRY_IN_USE) throw std::invalid_argument("Allocation already appears to have been destructed");
	}

	size_t _allocation_count;
	PoolEntry* _next_free;
	// The memory for the pool is declared inline with the rest of this class.
	PoolEntry _pool[pool_size];
};
