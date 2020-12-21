#include "pch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Pointers
{
	TEST_CLASS(E02_RawMemoryManagement)
	{
		class Vector2
		{
		public:
			Vector2(int x, int y) :
				_x(x),
				_y(y)
			{
				// This is a simple way for us to check the number of instances in tests, but it's not
				// thread safe, so don't do it in production!
				E02_RawMemoryManagement::InstanceCount++;
			}

			// Default constructor
			Vector2() : Vector2(0, 1) {}

			virtual ~Vector2()
			{
				E02_RawMemoryManagement::InstanceCount--;
			}

			int getX() const { return _x; }
			int getY() const { return _y; }

			void RotateLeft() {
				int t = _x;
				_x = -_y;
				_y = t;
			}

			void RotateRight() {
				int t = -_x;
				_x = _y;
				_y = t;
			}

		private:
			int _x;
			int _y;
		};

	public:
		static int InstanceCount;

		TEST_METHOD_INITIALIZE(SetUp)
		{
			InstanceCount = 0;
		}

		TEST_METHOD(Malloc_Free)
		{
			// Malloc and Free are old school C memory allocators and we generally have much better options available these days.
			const int count = 5;
			int* pNumbers = (int*)malloc(sizeof(int) * count);
			// The memory allocated via malloc is unlikely be to initialised and will contain random junk from previous allocations
			// in your process until you initialise it.

			// We can now use the allocated memory as if it was an array of integers.
			fill(pNumbers, count);
			int total = sum(pNumbers, count);
			Assert::AreEqual(15, total, L"All the values in the dynamically allocated array should have been summed up");

			// When you're finished with memory, you must free it to return it to the OS.
			// Failure to free the memory before all instances of the pointer have gone out of scope is the most common
			// form of memory leak as it's so easy to do.
			free(pNumbers);

			// pNumbers' value is now invalid. It points to memory we were previously allocated and could be allocated by
			// something else in the future. As we're about the exit this function, we don't need to take any further action.
			// However, if it was a member of an object instance that might persist after we've free'd the memory, we should
			// set it to null.
			pNumbers = nullptr;

			// It's worth noting at this point that Windows and Linux handle Out of Memory issues very differently when
			// allocating directly from the heap.
			// On Windows, if the system is out of memory, malloc() will return NULL, very easy to detect.
			// On Linux, this function will still return a "valid" pointer even if there is no memory available. Linux is making
			// the gamble that memory will be available by the time you attempt to use it and relys on allocating the
			// memory just in time when there's a page miss. This behaviour is based on the fact that large memory allocations
			// are probably for large IO tasks which can take time before they access all the memory they've requested.
			// The unfortunate side effect is that your program may initially run fine for a little bit and then suddnely crash
			// when the OS is unable to fulfil the promise of memory it made at a later point.

			// I have no idea what Mac OS does.
		}

		TEST_METHOD(New_Dete_Primitives)
		{
			// This is just included for completeness, but I've not yet seen a good reason for doing this.
			// You can allocate a primitive on the heap using "new".
			int* pNumber = new int(1234);
			Assert::AreEqual(1234, *pNumber, L"pNumber should have been initialised to the specified value");

			// The pointer operates the same as any other pointer.
			*pNumber = 15;
			Assert::AreEqual(15, *pNumber, L"pNumber should have been updated");

			// When you want to release the memory, "delete" the pointer.
			// Failure to call "delete" before the last pointer to reference the allocated memory goes out of scope
			// will result in memory leaks as there is no garbage collector in C++.
			delete pNumber;
		}

		TEST_METHOD(New_Delete_Primitive_Arrays)
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

			// Again, pNumbers now contains an invalid address, so it should be nulled if there is a risk of it hanging around.
			pNumbers = nullptr;
		}

		TEST_METHOD(Stack_Object_Instance)
		{
			// Enter a new temporary variable scope
			{
				// If no pointer or allocation method is used, then object instances are allocated on the heap within
				// the current scope.
				Vector2 vec;
				Assert::AreEqual(1, InstanceCount, L"New vector instance should have been created");

				// We access members of the instance using the '.' operator.
				vec.RotateRight();
				Assert::AreEqual(1, vec.getX());
				Assert::AreEqual(0, vec.getY());

				// Even though the object is on the stack, you can still take the address of it.
				Vector2* pVec = &vec;
				Assert::AreEqual(1, InstanceCount, L"No additional vector instances should have been created");

				// The member de-reference operator is used to interact with object pointers.
				pVec->RotateRight();
				// Of course, changes made via the pointer are made to the original object instance
				Assert::AreEqual(0, vec.getX());
				Assert::AreEqual(-1, vec.getY());
			}

			// The destructor is called instantly when the object goes out of scope. The is a fundamental part of
			// C++ resource management, referred to as RAII. You'll see if used frequently from scoped mutex locks to
			// pointer wrapper templates to manage memory allocation life times and ownership.
			Assert::AreEqual(0, InstanceCount, L"vec's destructor should have been called when it went out of scope");
		}

		TEST_METHOD(New_Object_Instance)
		{
			// new is used to allocate objects on the heap and call its constructor
			Vector2* pVec = new Vector2();
			Assert::AreEqual(1, InstanceCount, L"New vector instance should have been created");

			// Methods can be called on the instance by using the member de-reference operator.
			pVec->RotateLeft();
			Assert::AreEqual(-1, pVec->getX());
			Assert::AreEqual(0, pVec->getY());

			// When we're done with the object instance, we need to explicitly release its memory by calling delete.
			// The destructor will be called immediately by delete.
			delete pVec;
			Assert::AreEqual(0, InstanceCount, L"pVec's destructor should have been called");
		}

		TEST_METHOD(New_Object_Array)
		{
			// If the objects have a default constructor, you can directly create an array of objects on the heap using new[].
			// THe objects will be allocated consecutively in a single block of memory.
			int count = 5;
			Vector2* pVecArray = new Vector2[count];
			Assert::AreEqual(count, InstanceCount, L"5 instances of Vector2 should have been constructed");

			// You can access the objects via their array index and the '.' operator.
			pVecArray[2].RotateRight();

			// Or via pointer arithmetic and the "->" operator (not generally advised).
			Assert::AreEqual(1, (pVecArray + 2)->getX());
			Assert::AreEqual(0, (pVecArray + 2)->getY());

			// When finished with the array, you must delete them all at the same time using delete[].
			delete[] pVecArray;
			Assert::AreEqual(0, InstanceCount, L"All vector instances should have been destructed");
		}

		TEST_METHOD(Placement_New)
		{
			// If you have a custom low level memory allocator, you can use "placement new" to directly invoke an object's
			// constructor on a raw pointer. This won't allocate any new memory for the object and will directly construct
			// it in memory at the address specified.
			// This technique if often used in games when there might be specific memory pools for different types of
			// objects, e.g. a pool for particle instances. By using different pools, you better tack each systems' memory
			// usage and potentially avoid memory fragmentation between systems.
			void* pMem = malloc(sizeof(Vector2));
			Assert::AreEqual(0, InstanceCount, L"No instances should have been constructed");
			Vector2* pVec = new(pMem) Vector2();
			Assert::AreEqual(1, InstanceCount, L"A new Vector2 instance should have been constructed");

			// The pointer returned will point to the same address as the initial memory allocation as that's where the
			// object was constructed.
			Assert::AreEqual(pMem, (void*)pVec, L"Memory allocation and object instance should share the same address");

			// Other than how it was constructed, the object still functions as normal.
			pVec->RotateLeft();
			Assert::AreEqual(-1, pVec->getX());
			Assert::AreEqual(0, pVec->getY());

			// When you want to release the object instance, you must manually call the object's destructor.
			pVec->~Vector2();
			Assert::AreEqual(0, InstanceCount, L"pVec's destructor should have been called");
			// And then you must release the memory via the appropriate allocator function.
			free(pMem);

			// Sometimes, you might see people skip the destructor call as an "optimisation". I strongly advise against that
			// as modern compilers are able to optimise the call away if it's truely not needed (via link time code generation)
			// and it will save you from resource leak bugs where another engineer has made the assumption that the destructor
			// will be called to release an asset such as a texture or model.
		}

		TEST_METHOD(Dynamic_Stack_Allocations)
		{
			// This is here for completeness in case you encouter it, but I highly, highly recommend against ever using it 
			// unless you have a very clear reason to as it can be a common source of security issues and DoS attacks when
			// inputs aren't properly validated and a malicious request can blow the stack.
			// The alloca/_alloca function can be used to directly allocate memory on the stack rather than the heap.
			int count = 5;
			int* pNumbers = (int*)alloca(sizeof(int) * count);

			// Once allocated, assuming your process is still running, you can use the memory as normal.
			fill(pNumbers, count);
			int total = sum(pNumbers, count);
			Assert::AreEqual(15, total, L"Array values should sum up correctly");

			// As the memory is allocated on the stack, there is no deallocation function for it. The memory is released when
			// the stack frame it was allocated in goes out of scope, usually when the function returns or an exception thrown.
			// It can often be a source of bugs that an address returned by alloca() is passed to and retained by an object
			// with a life-time greater than the current stack frame. When this happens, the object will be referencing invalid
			// memory once the current frame exits. This is likely result in future stack corruption if the invalid pointer is
			// manipulated.
		}

		TEST_METHOD(Slightly_Better_Dynamic_Stack_Allocations)
		{
			// If you really want to allocate memory on the stack, Windows CRT provides _malloca. It will attempt to allocate on
			// the stack. But if the size requested is greated than _ALLOCA_S_THRESHOLD, then the allocation is made from the
			// heap instead.
			int count = 5;
			int* pNumbers = (int*)_malloca(sizeof(int) * count);

			// Wherever the memory was allocated is opaque to you and makes no difference to how the memory can be used.
			fill(pNumbers, count);
			int total = sum(pNumbers, count);
			Assert::AreEqual(15, total, L"Array values should sum up correctly");

			// As the memory could have come from the heap, you do need to explicitly free them memory before you leave the current
			// stack frame.
			_freea(pNumbers);
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

	int E02_RawMemoryManagement::InstanceCount = 0;
}