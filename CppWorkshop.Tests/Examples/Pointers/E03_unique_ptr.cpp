/*
 * std::unique_ptr was introduced in C++11 as a way to manage an allocation's life cycle via
 * explicit unique ownership. Only a single "entity" can hold a handle on the allocated
 * object. Ownership can be transferred, but as soon as the handle is set to null or goes out of
 * scope, the memory is released as there are no explicit owners and the object would be
 * unreachable.
 * 
 * Despite it wrapping a raw pointer, compilers are able to inline a majority of the code meaning
 * unique_ptrs have the same runtime performance as a raw pointer in most cases.
 * 
 * It is worth noting that unique_ptrs can only really be used within the code of a linked module,
 * i.e. an exe, dll or so. It is not safe to attempt to expose external APIs on a dll or so that
 * use unique_ptrs as the templated code is inlined and is highly dependent on the compiler
 * version. A unique_ptr passed from an LLVM compiled module to a Visual Studio compiled module is
 * highly likely to go bang. On Windows, you have the extra concern that dlls and exes have
 * independent heaps and so memory must be freed by the module that allocated it.
 */
#include "pch.h"
#include "Vector2.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Pointers
{
    TEST_CLASS(E03_UniquePtr)
    {
    public:
        TEST_METHOD(Make_Unique_Default_Constructor)
        {
            // make_unique is the usual way to construct an object that you want to wrap as a
            // unique_ptr. The default allocator will be used.
            std::unique_ptr<Vector2> pVec = std::make_unique<Vector2>();

            // You can compare the unique_ptr with nullptr as if it were a regular pointer.
            Assert::IsFalse(pVec == nullptr, L"pVec should not be null");
            // Access to the object is via the "->" operator and so it operates as if to were a
            // regular pointer.
            Assert::AreEqual(0, pVec->getX());
            Assert::AreEqual(1, pVec->getY());

            // In most cases, unique_ptr functions syntactically the same as a normal C++ raw
            // pointer. However, its life cycle can be tracked and validated to guard against the
            // most common forms of pointer misuse or leaks.
        }

        TEST_METHOD(Make_Unique_Constructor_Parameters)
        {
            // make_unique is a variadic template function and so can accept any constructor
            // paramters.
            std::unique_ptr<Vector2> pVec = std::make_unique<Vector2>(-2, 3);

            Assert::IsFalse(pVec == nullptr, L"pVec should not be null");
            Assert::AreEqual(-2, pVec->getX());
            Assert::AreEqual(3, pVec->getY());
        }

        TEST_METHOD(Scope_Based_Life_Cycle)
        {
            // A unique_ptr will delete its wrapped object if it goes out of scope.
            {
                std::unique_ptr<Vector2> pVec = std::make_unique<Vector2>();

                Assert::IsFalse(pVec == nullptr, L"pVec should not be null");
                Assert::AreEqual(1, Vector2::InstanceCount, L"An instance should have been allocated");
            }

            Assert::AreEqual(0, Vector2::InstanceCount, L"Vector2 instance should have been released");
        }

        TEST_METHOD(Explicit_Release)
        {
            std::unique_ptr<Vector2> pVec = std::make_unique<Vector2>();

            Assert::IsFalse(pVec == nullptr, L"pVec should not be null");
            Assert::AreEqual(1, Vector2::InstanceCount, L"An instance should have been allocated");

            // You can explicitly delete the wrapped object by simply assigning nullptr to a
            // unique_ptr. This will invoke the wrapped object's destructor and release the memory. 
            pVec = nullptr;

            Assert::IsTrue(pVec == nullptr, L"pVec should be null");
            Assert::AreEqual(0, Vector2::InstanceCount, L"Vector2 instance should have been released");
        }

        TEST_METHOD(Owner_Uniqueness)
        {
            // It's possible to transfer ownership of the wrapped object from one unique_ptr to
            // another using std::move(). This is so that ownership transfer is explicit rather
            // than accidental and helps guard against erroneous assignment.
            std::unique_ptr<Vector2> pVec1 = std::make_unique<Vector2>(5, 7);
            std::unique_ptr<Vector2> pVec2 = std::move(pVec1);

            // The original unique_ptr will have its internal pointer set to null by the operation. 
            Assert::IsTrue(pVec1 == nullptr, L"pVec1 should have become null");
            Assert::IsFalse(pVec2 == nullptr, L"pVec2 should have been initialised from pVec1");
            Assert::AreEqual(5, pVec2->getX());
            Assert::AreEqual(7, pVec2->getY());
            Assert::AreEqual(1, Vector2::InstanceCount, L"Only a single Vector2 instance should have been allocated");

            // Try directly assigning pVec1 to pVect2 ...
        }

        TEST_METHOD(Move_Semantics)
        {
            // The unique_ptr wrapper fully supports move semantics and so is effectively free to
            // to return by value.
            // The design goal for unique_ptr is for the wrapper itself to be used as a value type
            // with the compiler being able to determine exactly where and how the wrappers are
            // constructed in the most efficient way.
            auto create = []() {
                auto pv = std::make_unique<Vector2>();
                pv->RotateLeft();
                return pv;
            };
            std::unique_ptr<Vector2> pVec = create();

            Assert::AreEqual(1, Vector2::InstanceCount, L"Only a single Vector2 instance should have been allocated");
            Assert::IsFalse(pVec == nullptr, L"pVec should not be null");
            Assert::AreEqual(-1, pVec->getX());
            Assert::AreEqual(0, pVec->getY());
        }

        TEST_METHOD(Passing_To_A_Function)
        {
            // Although only a single unique_ptr can exist for an allocation, you can still pass
            // the unique_ptr by reference so that it can be consumed by another API.
            auto update = [](std::unique_ptr<Vector2>& pValue)
            {
                pValue->RotateLeft();
            };

            std::unique_ptr<Vector2> pVec = std::make_unique<Vector2>();
            Assert::AreEqual(0, pVec->getX());
            Assert::AreEqual(1, pVec->getY());

            update(pVec);
            Assert::AreEqual(-1, pVec->getX());
            Assert::AreEqual(0, pVec->getY());
        }

        TEST_METHOD(Raw_Pointer_Access)
        {
            // When interacting with legacy or external APIs, you may need to access the raw
            // pointer for the wrapped object. This does not transfer ownership and so you should
            // treat the raw pointer as a borrowed access.
            std::unique_ptr<Vector2> pVec = std::make_unique<Vector2>();
            Vector2* pVecRaw = pVec.get();
            pVecRaw->RotateRight();

            Assert::AreEqual(1, pVec->getX());
            Assert::AreEqual(0, pVec->getY());

            // Care must be taken to ensure the an instance of unique_ptr with explicit ownership
            // of the object remains in scope for the entire time the raw pointer is in use.
        }

        TEST_METHOD(Reference_Access)
        {
            // Again, legacy or external APIs might require being passed a reference. Fortunately,
            // it's also possible to get a reference from a unique_ptr using the same syntax for
            // dereferencing a regular pointer.
            auto rotate = [](Vector2& vec) {
                vec.RotateLeft();
            };

            std::unique_ptr<Vector2> pVec = std::make_unique<Vector2>(5, 7);
            rotate(*pVec);

            Assert::AreEqual(-7, pVec->getX());
            Assert::AreEqual(5, pVec->getY());
        }

        TEST_METHOD(Bool_Operator)
        {
            // unique_ptr provides an overridden bool operator so that pointer checks can be
            // performed using the same semantics as raw pointers.
            std::unique_ptr<Vector2> pVec = std::make_unique<Vector2>(5, 7);
            if (!pVec) Assert::Fail(L"Pointer should have been set");

            pVec = nullptr;
            if (pVec) Assert::Fail(L"Pointer should have been cleared");
        }

        TEST_METHOD(Custom_Allocate_Delete)
        {
            // Like everything in the STL, it's possible to use custom allocators and deleters.
            auto create = []() {
                // A typedef for the delete function to simplify the declaration.
                typedef void(*Vector2_Delete)(Vector2*);
                // We allocate the memory required for a Vector2 via our custom allocator.
                auto pRaw = E03_UniquePtr::s_pAllocator->allocate(sizeof(Vector2));
                // Placement new is used to construct the Vector2.
                auto pVecRaw = new(pRaw) Vector2(2, 3);
                // We then provide the pointer to a new unique_ptr explicitly. We also provide a
                // custom delete function.
                std::unique_ptr<Vector2, Vector2_Delete> pVec(pVecRaw, [](Vector2* pMem) {
                    // As we have taken responsibility of constructing the object, we are also
                    // responsible for destructing it and releasing the memory.
                    pMem->~Vector2();
                    E03_UniquePtr::s_pAllocator->deallocate((uint8_t*)pMem);
                });
                return pVec;
            };

            // As the type is actually std::unique_ptr<Vector2, Vector2_Delete>, using "auto"
            // allows us to simplify the declaration.
            auto pVec = create();

            Assert::AreEqual(1, Vector2::InstanceCount, L"Only a single Vector2 instance should have been allocated");
            Assert::AreEqual(1, (int)s_pAllocator->getNumAllocations(), L"Only one request to the custom allocator should have been made");
            Assert::AreEqual(2, pVec->getX());
            Assert::AreEqual(3, pVec->getY());

            // The unqiue_ptr will function in the same way, calling our custom delete function if
            // the unique_ptr goes out of scope or is explicitly assigned another value.
            pVec = nullptr;

            Assert::AreEqual(0, Vector2::InstanceCount, L"Vector2 instance should have been destructed");
            Assert::AreEqual(0, (int)s_pAllocator->getNumAllocations(), L"Vector2 memory should have been released");

            // One thing to be aware of when using custom allocators in this way is that the
            // unique_ptr is also holding onto a pointer for the custom delete function and so must
            // also pass that around when transferring ownership. In most cases, this won't be an
            // issue, but it is something you should be aware of.
        }

        TEST_METHOD(Custom_Allocate_Delete_On_Class)
        {
            // If it's critical, you can avoid the overhead of tracking the delete functor if the
            // object you're trying to construct has overridden the new and delete operators.
            // As new/delete is used under the hood for the default cases, the compiler will route
            // them through to your custom allocator at no extra cost.
            std::unique_ptr<TrackedVector2> pVec = std::make_unique<TrackedVector2>();

            Assert::AreEqual(1, (int)TrackedVector2::s_Allocator.getNumAllocations(), L"TrackedVector2 should have been allocated via our custom allocator");

            // Releasing the memory will also be routed to the custom allocator without any
            // explicit set up.
            pVec = nullptr;

            Assert::AreEqual(0, (int)TrackedVector2::s_Allocator.getNumAllocations(), L"TrackedVector2 should have been released via our custom allocator");
        }

        TEST_METHOD(Upcast_To_Base_Pointer_Type)
        {
            // It's possible to up cast from a derived class to a base class if you have control
            // over both references.
            std::unique_ptr<Vector3> pVec3 = std::make_unique<Vector3>(5, 7, 11);
            Assert::AreEqual(1, Vector2::InstanceCount, L"Only single vector should have been created");
            Assert::AreEqual(5, pVec3->getX());
            Assert::AreEqual(7, pVec3->getY());
            Assert::AreEqual(11, pVec3->getZ());

            // Here, the life cycle is managed explicitly and we move the reference into the up
            // cast type, transferring owenership along with it.
            std::unique_ptr<Vector2> pVec2 = std::move(pVec3);
            Assert::AreEqual(1, Vector2::InstanceCount, L"Only single vector should have been created");
            Assert::AreEqual(5, pVec2->getX());
            Assert::AreEqual(7, pVec2->getY());

            // We can also construct directly into a pointer of the base class type.
            std::unique_ptr<Vector2> pAutoUpCastVec = std::make_unique<Vector3>(13, 17, 19);
            Assert::AreEqual(2, Vector2::InstanceCount, L"Two vector instances should currently exist");
            Assert::AreEqual(13, pAutoUpCastVec->getX());
            Assert::AreEqual(17, pAutoUpCastVec->getY());

            // Unfortunately, doing anything more than that generally requires lots of template
            // wrangling to get working. For example, you can't pass a derived class to a function
            // expecting the base class, even if it is by reference as the types are incompatible
            // at the template level. You'd need to construct a new unique_ptr with the correct
            // type in order to pass it, but this would move ownership out of the original wrapper.
            // After the call, you'd need to move the ownership back to the original wrapper which
            // would require down casting.

            // Down casting is even more problematic as the templates are incompatible and
            // constructing an new unique_ptr of the correct type will also require moving the
            // deletor reference as well. However, declaring an explicit deletor fundamentally
            // changes the type again meaning it will still be incompatible with the original
            // wrapper type.

            // The best approach if you need to pass around unique_ptr references is to only use
            // the base type and explicitly down cast the wrapped pointer if needed, e.g.:
            int z = static_cast<Vector3*>(pVec2.get())->getZ();
            Assert::AreEqual(11, z);
        }


        //---------------------------------------------------------------------------------------//
        // Memory Leak Example
        //---------------------------------------------------------------------------------------//

        class LinkedListNode
        {
        public:
            static int InstanceCount;
            std::unique_ptr<LinkedListNode> Next;

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
            // It requires specific setting up, but it's possible to create a circular reference
            // with unique_ptr which will lead to a memory leak as the graph will likely be fully
            // isolated from the rest of the applicate state.
            // This is a important difference between a managed runtime with a garbage collector
            // and C++'s memory model.

            // With a correctly formed linked list, nulling the "head" pointer will automatically
            // release all the nodes as they go out of scope.
            auto first = std::make_unique<LinkedListNode>();
            first->Next = std::make_unique<LinkedListNode>();
            first->Next->Next = std::make_unique<LinkedListNode>();
            Assert::AreEqual(3, LinkedListNode::InstanceCount, L"Wrong number of nodes created for directed linked list");

            first = nullptr;
            Assert::AreEqual(0, LinkedListNode::InstanceCount, L"Linked list nodes weren't deallocated");

            // However, if we force the last node to link back to the first node, the head pointer
            // will cease to point to any of the nodes but each node is technically still live as
            // there will be another node pointing to it. None of the nodes are reachable and the
            // memory hasn't been released and so we have created a memory leak.
            first = std::make_unique<LinkedListNode>();
            first->Next = std::make_unique<LinkedListNode>();
            first->Next->Next = std::make_unique<LinkedListNode>();
            Assert::AreEqual(3, LinkedListNode::InstanceCount, L"Wrong number of nodes created for directed linked list");

            first->Next->Next->Next = std::move(first);
            Assert::IsNull(first.get(), L"Local node pointer should have become null");
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
    std::unique_ptr<TrackingAllocator<>> E03_UniquePtr::s_pAllocator;
    int E03_UniquePtr::LinkedListNode::InstanceCount;
}
