#include <new>
#include <stdexcept>

template<class type, size_t pool_size>
class PoolAllocator
{
public:
	PoolAllocator()
	{
		reset();
	}

	void reset()
	{
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
		allocation->next = const_cast<PoolEntry*>(&ENTRY_IN_USE);
		return new(allocation->mem) type(std::forward<_Types>(_Args)...);
	}

	void destruct(type* pMem)
	{
		PoolEntry* pEntry = getEntry(pMem);
		verifyEntryWithinPool(pEntry);
		pMem->~type();
		pEntry->next = _next_free;
		_next_free = pEntry;
		_allocation_count--;
	}

private:
	struct PoolEntry
	{
		PoolEntry* next;
		char mem[sizeof(type)];
	};

	static constexpr PoolEntry ENTRY_IN_USE = PoolEntry();

	PoolEntry* getEntry(type* pMem)
	{
		auto raw = reinterpret_cast<char*>(pMem);
		raw -= sizeof(PoolEntry*);
		return reinterpret_cast<PoolEntry*>(raw);
	}

	void verifyEntryWithinPool(PoolEntry* pEntry)
	{
		if (pEntry < _pool || pEntry >= &_pool[pool_size]) throw std::invalid_argument("Allocation is not within this pool");
		if (pEntry->next != &ENTRY_IN_USE) throw std::invalid_argument("Allocation already appears to have been destructed");
	}

	size_t _allocation_count;
	PoolEntry* _next_free;
	PoolEntry _pool[pool_size];
};
