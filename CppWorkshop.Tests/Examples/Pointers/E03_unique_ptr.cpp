#include "pch.h"
#include "Vector2.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Pointers
{
    TEST_CLASS(E03_UniquePtr)
    {
    public:
        static std::unique_ptr<TrackingAllocator<uint8_t>> pAllocator;

        TEST_METHOD_INITIALIZE(SetUp)
        {
            pAllocator = std::make_unique<TrackingAllocator<uint8_t>>();
            Vector2::InstanceCount = 0;
        }

        TEST_METHOD(Make_Unique_Default_Constructor)
        {
            std::unique_ptr<Vector2> pVec = std::make_unique<Vector2>();

            Assert::IsFalse(pVec == nullptr, L"pVec should not be null");
            Assert::AreEqual(0, pVec->getX());
            Assert::AreEqual(1, pVec->getY());
        }

        TEST_METHOD(Make_Unique_Constructor_Parameters)
        {
            std::unique_ptr<Vector2> pVec = std::make_unique<Vector2>(-2, 3);

            Assert::IsFalse(pVec == nullptr, L"pVec should not be null");
            Assert::AreEqual(-2, pVec->getX());
            Assert::AreEqual(3, pVec->getY());
        }

        TEST_METHOD(RAII)
        {
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

            pVec = nullptr;

            Assert::IsTrue(pVec == nullptr, L"pVec should be null");
            Assert::AreEqual(0, Vector2::InstanceCount, L"Vector2 instance should have been released");
        }

        TEST_METHOD(Owner_Uniqueness)
        {
            std::unique_ptr<Vector2> pVec1 = std::make_unique<Vector2>(5, 7);
            std::unique_ptr<Vector2> pVec2 = std::move(pVec1);

            Assert::IsTrue(pVec1 == nullptr, L"pVec1 should have become null");
            Assert::IsFalse(pVec2 == nullptr, L"pVec2 should have been initialised from pVec1");
            Assert::AreEqual(5, pVec2->getX());
            Assert::AreEqual(7, pVec2->getY());
            Assert::AreEqual(1, Vector2::InstanceCount, L"Only a single Vector2 instance should have been allocated");

            // Try directly assigning pVec1 to pVect2...
        }

        TEST_METHOD(Move_Semantics)
        {
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

        TEST_METHOD(Raw_Pointer_Access)
        {
            std::unique_ptr<Vector2> pVec = std::make_unique<Vector2>();
            Vector2* pVecRaw = pVec.get();
            pVecRaw->RotateRight();

            Assert::AreEqual(1, pVec->getX());
            Assert::AreEqual(0, pVec->getY());
        }

        TEST_METHOD(Reference_Access)
        {
            auto rotate = [](Vector2& vec) {
                vec.RotateLeft();
            };

            std::unique_ptr<Vector2> pVec = std::make_unique<Vector2>(5, 7);
            rotate(*pVec);

            Assert::AreEqual(-7, pVec->getX());
            Assert::AreEqual(5, pVec->getY());
        }

        TEST_METHOD(Custom_Allocate_Delete)
        {
            typedef void(*PFN_Deleter)(Vector2*);

            auto create = []() {
                auto pRaw = E03_UniquePtr::pAllocator->allocate(sizeof(Vector2));
                auto pVecRaw = new(pRaw) Vector2(2, 3);
                std::unique_ptr<Vector2, PFN_Deleter> pVec(pVecRaw, [](Vector2* pMem) {
                    pMem->~Vector2();
                    E03_UniquePtr::pAllocator->deallocate((uint8_t*)pMem);
                });
                return pVec;
            };

            auto pVec = create();

            Assert::AreEqual(1, Vector2::InstanceCount, L"Only a single Vector2 instance should have been allocated");
            Assert::AreEqual(1, (int)pAllocator->getNumAllocations(), L"Only one request to the custom allocator should have been made");
            Assert::AreEqual(2, pVec->getX());
            Assert::AreEqual(3, pVec->getY());

            pVec = nullptr;

            Assert::AreEqual(0, Vector2::InstanceCount, L"Vector2 instance should have been destructed");
            Assert::AreEqual(0, (int)pAllocator->getNumAllocations(), L"Vector2 memory should have been released");
        }
    };

    //-------------------------------------------------------------------------------------------//
    // Statics
    //-------------------------------------------------------------------------------------------//
    std::unique_ptr<TrackingAllocator<uint8_t>> E03_UniquePtr::pAllocator;
}
