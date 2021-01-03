#include "pch.h"
#include "Vector2.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Pointers
{
    TEST_CLASS(E03_UniquePtr)
    {
    public:
        TEST_METHOD_INITIALIZE(SetUp)
        {
            Vector2::InstanceCount = 0;
        }

        TEST_METHOD(Make_Unique_Default_Constructor)
        {
            std::unique_ptr<Vector2> pVec = std::make_unique<Vector2>();

            Assert::IsFalse(pVec == nullptr, L"pVec should not be null");
            Assert::AreEqual(0, pVec->getX(), L"X component should be zero");
            Assert::AreEqual(1, pVec->getY(), L"Y component should be zero");
        }

        TEST_METHOD(Make_Unique_Constructor_Parameters)
        {
            std::unique_ptr<Vector2> pVec = std::make_unique<Vector2>(-2, 3);

            Assert::IsFalse(pVec == nullptr, L"pVec should not be null");
            Assert::AreEqual(-2, pVec->getX(), L"X component should be zero");
            Assert::AreEqual(3, pVec->getY(), L"Y component should be zero");
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
            Assert::AreEqual(5, pVec2->getX(), L"X component should be zero");
            Assert::AreEqual(7, pVec2->getY(), L"Y component should be zero");
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
            Assert::AreEqual(-1, pVec->getX(), L"X component should be zero");
            Assert::AreEqual(0, pVec->getY(), L"Y component should be zero");
        }
    };
}
