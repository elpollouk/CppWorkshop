/*
 * std::shared_ptr was introduced in C++11 as a way to manage an allocation's life cycle via
 * reference counting. Many shared pointers can exist that reference a single allocation. Once
 * all the shared pointers have gone out of scope or have been explicitly cleared, then the
 * allocation will be released.
 * 
 * There is a slight overhead to using shared_ptr as there is a reference count associated and
 * maintained for each allocation.
 * 
 * An instance of a shared_ptr has the same API as a unique_ptr and can be used in the same way
 * but are not compatible types. You can't pass a shared_ptr to an API expecting a unique_ptr.
 *
 * It is worth noting that shared_ptrs can only really be used within the code of a linked module,
 * i.e. an exe, dll or so. It is not safe to attempt to expose external APIs on a dll or so that
 * use shared_ptrs as the templated code is inlined and is highly dependent on the compiler
 * version. A shared_ptr passed from an LLVM compiled module to a Visual Studio compiled module is
 * highly likely to go bang. On Windows, you have the extra concern that dlls and exes have
 * independent heaps and so memory must be freed by the module that allocated it.
 */
#include "pch.h"
#include "Vector2.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Pointers
{
    TEST_CLASS(E04_SharedPtr)
    {
    public:
        TEST_METHOD(Make_Shared_Default_Constructor)
        {
            // make_shared is the usual way to construct an object that you want to wrap as a
            // shared_ptr. The default allocator will be used.
            std::shared_ptr<Vector2> pVec = std::make_shared<Vector2>();

            // You can compare the shared_ptr with nullptr as if it were a regular pointer.
            Assert::IsFalse(pVec == nullptr, L"pVec should not be null");
            // Access to the object is via the "->" operator and so it operates as if to were a
            // regular pointer.
            Assert::AreEqual(0, pVec->getX());
            Assert::AreEqual(1, pVec->getY());

            // In most cases, shared_ptr functions syntactically the same as a normal C++ raw
            // pointer. However, its life cycle can be tracked and validated to guard against the
            // most common forms of pointer misuse or leaks.
        }

        TEST_METHOD(Make_Shared_Constructor_Parameters)
        {
            // make_shared is a variadic template function and so can accept any constructor
            // paramters.
            std::shared_ptr<Vector2> pVec = std::make_shared<Vector2>(-2, 3);

            Assert::IsFalse(pVec == nullptr, L"pVec should not be null");
            Assert::AreEqual(-2, pVec->getX());
            Assert::AreEqual(3, pVec->getY());
        }

        TEST_METHOD(Multiple_References)
        {
            // Multiple shared_ptr instances can point to a single allocation.
            std::shared_ptr<Vector2> pVec1 = std::make_shared<Vector2>();
            Assert::AreEqual(1, Vector2::InstanceCount, L"A single instance of Vector2 should have been allocated");

            std::shared_ptr<Vector2> pVec2 = pVec1;
            std::shared_ptr<Vector2> pVec3 = pVec1;
            Assert::AreEqual(1, Vector2::InstanceCount, L"A single instance of Vector2 should still exists");
            Assert::IsTrue(pVec1 == pVec2, L"pVec1 and pVec2 should point to the same instance");
            Assert::IsTrue(pVec2 == pVec3, L"pVec2 and pVec3 should point to the same instance");
            Assert::IsTrue(pVec1 == pVec3, L"pVec1 and pVec3 should point to the same instance");

            // The original allocation will remain live until all references are cleared.
            pVec1 = nullptr;
            Assert::AreEqual(1, Vector2::InstanceCount, L"A single instance of Vector2 should still exists");
            pVec2 = nullptr;
            Assert::AreEqual(1, Vector2::InstanceCount, L"A single instance of Vector2 should still exists");
            pVec3 = nullptr;
            Assert::AreEqual(0, Vector2::InstanceCount, L"Vector2 instance should have been released");
        }

        TEST_METHOD(Upgrade_Unique_ptr)
        {
            // Although you can't downgrade a shared_ptr to a unique_ptr (there may be multiple
            // live references that violate unique_ptr constraints), you can upgrade a unique_ptr
            // to a shared_ptr.
            std::unique_ptr<Vector2> pUnqiue = std::make_unique<Vector2>(5, 7);
            Assert::AreEqual(1, Vector2::InstanceCount, L"A single instance of Vector2 should have been allocated");

            std::shared_ptr<Vector2> pShared1 = std::move(pUnqiue);
            Assert::AreEqual(1, Vector2::InstanceCount, L"A single instance of Vector2 should have been allocated");
            // Because the shared pointer can now violate the unique pointer's contraints, the
            // original unique_ptr is invalidated and set to null.
            Assert::IsNull(pUnqiue.get(), L"Unique pointer should have been cleared");

            // The shared pointer now behaves as a regular shared pointer.
            std::shared_ptr<Vector2> pShared2 = pShared1;
            std::shared_ptr<Vector2> pShared3 = pShared2;
            Assert::AreEqual(1, Vector2::InstanceCount, L"A single instance of Vector2 should have been allocated");
            Assert::AreEqual(5, pShared2->getX());
            Assert::AreEqual(7, pShared2->getY());

            // When designing an API, factory functions should return unique_ptrs. This is so that
            // the caller can decide the life-cycle semantics they wish to use. If you return a
            // shared_ptr, then the caller is forced to use reference counting.
        }

        TEST_METHOD(I_Really_Want_To_Create_A_UniquePtr)
        {
            // Ok, as with most things in C++, you can brute force it. But the world will hate you.
            // Don't do this. Please.
            std::shared_ptr<Vector2> pShared = std::make_shared<Vector2>(3, 5);
            Assert::AreEqual(1, Vector2::InstanceCount, L"Only a single instance of Vector2 should exist");

            // Now for something completely hideous...
            auto pUnique = std::unique_ptr<Vector2, std::function<void(Vector2*)>>(pShared.get(), [pShared](Vector2*) mutable {
                pShared = nullptr;
            });
            Assert::AreEqual(1, Vector2::InstanceCount, L"Only a single instance of Vector2 should exist");
            // The original shared_ptr is still valid.
            Assert::AreEqual(3, pShared->getX());
            Assert::AreEqual(5, pShared->getY());

            // What we did is take the raw pointer from the shared_ptr and wrapped it in a new
            // unique_ptr. In order to keep the allocation alive even after the original shared_ptr
            // has been nulled or gone out of scope, we also capture a copy of the shared pointer
            // in our delete lambda. However, captures are const by default, so we've had to
            // explicitly mark the lambda as mutable so that we can assign null to our shared_ptr
            // copy when the unique_ptr is released.
            // Finally, because we're using a lambda with a capture, we can no longer use a simple
            // function pointer type, we have to use the std::function template to encapsulate the
            // delete function.

            pShared = nullptr;
            Assert::AreEqual(1, Vector2::InstanceCount, L"Only a single instance of Vector2 should exist");
            Assert::AreEqual(3, pUnique->getX());
            Assert::AreEqual(5, pUnique->getY());

            pUnique = nullptr;
            Assert::AreEqual(0, Vector2::InstanceCount, L"No instances of Vector2 should exist");
        }

        TEST_METHOD(Custom_Allocate_Delete)
        {
            // Like everything in the STL, it's possible to use custom allocators and deleters.
            auto create = []() {
                // A typedef for the delete function to simplify the declaration.
                typedef void(*Vector2_Delete)(Vector2*);
                // We allocate the memory required for a Vector2 via our custom allocator.
                auto pRaw = E04_SharedPtr::s_pAllocator->allocate(sizeof(Vector2));
                // Placement new is used to construct the Vector2.
                auto pVecRaw = new(pRaw) Vector2(2, 3);
                // We then provide the pointer to a new shared_ptr explicitly. We also provide a
                // custom delete function.
                std::shared_ptr<Vector2> pVec(pVecRaw, [](Vector2* pMem) {
                    // As we have taken responsibility of constructing the object, we are also
                    // responsible for destructing it and releasing the memory.
                    pMem->~Vector2();
                    E04_SharedPtr::s_pAllocator->deallocate((uint8_t*)pMem);
                 });
                return pVec;
            };

            // Unlike unique_ptr, there is no type specialisation required for a shared_ptr with
            // a custom deleter. This is because all shared_ptr instances hold a reference to a
            // deleter under the hood anyway.
            std::shared_ptr<Vector2> pVec = create();

            Assert::AreEqual(1, Vector2::InstanceCount, L"Only a single Vector2 instance should have been allocated");
            Assert::AreEqual(1, (int)s_pAllocator->getNumAllocations(), L"Only one request to the custom allocator should have been made");
            Assert::AreEqual(2, pVec->getX());
            Assert::AreEqual(3, pVec->getY());

            // The shared_ptr will function in the same way, calling our custom delete function if
            // the shared_ptr reference count drops to zero.
            pVec = nullptr;

            Assert::AreEqual(0, Vector2::InstanceCount, L"Vector2 instance should have been destructed");
            Assert::AreEqual(0, (int)s_pAllocator->getNumAllocations(), L"Vector2 memory should have been released");
        }

        //---------------------------------------------------------------------------------------//
        // Memory Leak Example
        //---------------------------------------------------------------------------------------//

        class LinkedListNode
        {
        public:
            static int InstanceCount;
            std::shared_ptr<LinkedListNode> Next;

            LinkedListNode()
            {
                InstanceCount++;
            }

            ~LinkedListNode()
            {
                InstanceCount--;
            }
        };

        TEST_METHOD(Circular_Reference_Leak)
        {
            // It's much easier to create a circular reference with shared_ptr as there as there is
            // no requiredment to use std::move().

            // With a correctly formed linked list, nulling the "head" pointer will automatically
            // release all the nodes as they go out of scope.
            auto first = std::make_shared<LinkedListNode>();
            first->Next = std::make_shared<LinkedListNode>();
            first->Next->Next = std::make_shared<LinkedListNode>();
            Assert::AreEqual(3, LinkedListNode::InstanceCount, L"Wrong number of nodes created for directed linked list");

            first = nullptr;
            Assert::AreEqual(0, LinkedListNode::InstanceCount, L"Linked list nodes weren't deallocated");

            // However, if we make the last node to link back to the first node and clear the head
            // pointer, the nodes are technically still live as there will be another node pointing
            // to each them. None of the nodes are reachable and the memory hasn't been released
            // and so we have created a memory leak.
            first = std::make_shared<LinkedListNode>();
            first->Next = std::make_shared<LinkedListNode>();
            first->Next->Next = std::make_shared<LinkedListNode>();
            Assert::AreEqual(3, LinkedListNode::InstanceCount, L"Wrong number of nodes created for directed linked list");
 
            first->Next->Next->Next = first;
            first = nullptr;
            Assert::AreEqual(3, LinkedListNode::InstanceCount, L"Node count should have remained the same after creating a circular reference");
        }


        //---------------------------------------------------------------------------------------//
        // Test Setup
        //---------------------------------------------------------------------------------------//
        static std::unique_ptr<TrackingAllocator<>> s_pAllocator;

        TEST_METHOD_INITIALIZE(SetUp)
        {
            s_pAllocator = std::make_unique<TrackingAllocator<>>();
            Vector2::InstanceCount = 0;
            LinkedListNode::InstanceCount = 0;
        }
    };

    //-------------------------------------------------------------------------------------------//
    // Statics
    //-------------------------------------------------------------------------------------------//
    std::unique_ptr<TrackingAllocator<>> E04_SharedPtr::s_pAllocator;
    int E04_SharedPtr::LinkedListNode::InstanceCount;
}
