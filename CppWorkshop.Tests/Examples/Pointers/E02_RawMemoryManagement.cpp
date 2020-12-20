#include "pch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Pointers
{
	TEST_CLASS(E02_RawMemoryManagement)
	{
	public:
		TEST_METHOD(Malloc_Free)
		{
			// Malloc and Free are old school C memory allocators and we generally have much better options available these days.
			const int count = 5;
			int* pNumbers = (int*)malloc(sizeof(int) * count);

			// We can now use the allocated memory as if it was an array of integers.
			fill(pNumbers, count);
			int total = sum(pNumbers, count);
			Assert::AreEqual(15, total, L"All the values in the dynamically allocated array should have been summed up");

			// When you're finished with memory, you must free it to return it to the OS
			free(pNumbers);

			// pNumbers' value is now invalid. It points to memory we were previously allocated and could be allocated by
			// something else in the future. As we're about the exit this function, we don't need to take any further action.
			// However, if it was a member of an object instance that might persist after we've free'd the memory, we should
			// set it to null.
			pNumbers = nullptr;
		}

		TEST_METHOD(New_Delete_Arrays)
		{
			// new and delete were introduced in C++, again there are generally better options these days, but you're still
			// likely to encounter them.
			const int count = 5;
			int* pNumbers = new int[count];

			// We can now use the allocated memory as if it was an array of integers.
			fill(pNumbers, count);
			int total = sum(pNumbers, count);
			Assert::AreEqual(15, total, L"All the values in the dynamically allocated array should have been summed up");

			// When we're done with the array, we need to delete it to release the memory. We need to use "[]" to indicate we're
			// deleting an array rather than a single integer. Under the hood, the allocator adds a bit of tracking information
			// to the allocation so that it knows how many items need to be deleted in the array. This is so that it can call
			// destructors on anything in the array.
			delete[] pNumbers;

			// Again, pNumbers now contains an invalid address, so it should be nulled if there is a risk of it hanging around
			pNumbers = nullptr;
		}

	private:
		// Fill the target array with a sequence of numbers
		static void fill(int* target, int count)
		{
			for (auto i = 0; i < count; i++)
				target[i] = i + 1;
		}

		// Sum up an array of numbers
		static int sum(const int* numbers, int count)
		{
			int total = 0;
			while (count--)
				total += *(numbers++);
			return total;
		}
	};
}