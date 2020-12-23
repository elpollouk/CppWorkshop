#pragma once
#include <malloc.h>
#include <new>

// An example allocator that uses malloc/free under the hood, but tracks the allocations so that
// we can query for total number of allocations and total size of allocations.
template<typename T = uint8_t>
class TrackingAllocator
{
public:
	TrackingAllocator() :
		_numAllocations(0),
		_totalAllocationsSize(0)
	{

	}

	unsigned int getNumAllocations() const { return _numAllocations; }
	size_t getTotalAllocationsSize() const { return _totalAllocationsSize; }

	T* allocate(size_t count)
	{
		// Calculate the total size of this allocation tacking into account the size header we are
		// going to attach to the allocation.
		size_t size = sizeof(uint64_t) + (count * sizeof(T));
		uint64_t* header = reinterpret_cast<uint64_t*>(malloc(size));
		// Check that we were able to allocate memory.
		if (header == nullptr) throw std::bad_alloc();
		// Store the size of the allocation in the header and update the tracking info.
		*header = size;
		_numAllocations++;
		_totalAllocationsSize += size;
		// Return the memory address immediately after the header. This is the memory the caller is
		// able to use.
		return reinterpret_cast<T*>(header + 1);
	}

	void deallocate(T* pMem)
	{
		// Ignore null requests, this is valid behaviour for "delete".
		if (pMem == nullptr) return;
		// Convert the address back to a uint64_t and go back to our header.
		uint64_t* header = reinterpret_cast<uint64_t*>(pMem);
		header--;
		// Update our tracking info and free the memory.
		_totalAllocationsSize -= *header;
		_numAllocations--;
		free(header);
	}

private:
	unsigned int _numAllocations;
	size_t _totalAllocationsSize;
};