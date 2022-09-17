/*
 * Basic pointer operations.
 * 
 * You will still need to understand how raw pointers work as they must still be used at external
 * code boundaries, e.g. JNI or DLL interfaces.
 * 
 * Recap:
 *	    &var = Address of operator, returns the address of a variable to store in a pointer.
 *      *var = De-refence the pointer stored in var so that the original value can be read or
 *             modified.
 */
#include "pch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Pointers
{
    TEST_CLASS(E01_Basics)
    {
        struct Vec2
        {
            int x;
            int y;
        };

    public:
        TEST_METHOD(Raw_Pointers)
        {
            // A pointer is just a variable that holds the memory address of something else (e.g. a
            // variable or other object).
            int number = 123;
            int* pNumber = &number;
            Assert::AreEqual(number, *pNumber, L"pNumber should point to the variable number");

            // Changes to the original value can be seen via the pointer.
            number = 456;
            Assert::AreEqual(number, *pNumber, L"Updating number should also been seen by pNumber");

            // You can update the original value via the pointer by de-referencing it.
            *pNumber = 789;
            Assert::AreEqual(number, *pNumber, L"Number should be updatable via pNumber");

            // Pointers can be assigned new addresses at runtime.
            int anotherNumber = 357;
            pNumber = &anotherNumber;
            Assert::AreNotEqual(number, *pNumber, L"pNumber should no longer point to number");
            Assert::AreEqual(anotherNumber, *pNumber, L"pNumber should now point to anotherNumber");

            // Try changing "int* pNumber" to "const int* pNumber"...
        }

        TEST_METHOD(Raw_Pointer_Declaration_Gotcha)
        {
            // For legacy reasons going back to 1972, the parser treats '*' as attached to the
            // variable name, not the type.
            int* pNumber1, number, *pNumber2;
            // Number is just an int, not a pointer!
            number = 7;
            pNumber1 = &number;
            pNumber2 = &number;

            Assert::AreEqual(number, *pNumber1, L"pNumber1 should point to number");
            Assert::AreEqual(pNumber1, pNumber2, L"pNumber1 and pNumber2 should point to the same address");

            // So watch out if declaring multiple pointers on a single line.
        }

        TEST_METHOD(Pass_By_Value_vs_Pass_By_Pointer)
        {
            // Return a swapped copy of the vector.
            auto SwapWithCopy = [](Vec2 v) {
                auto t = v.x;
                v.x = v.y;
                v.y = t;
                return v;
            };

            // Swap a vector in place.
            auto SwapInPlace = [](Vec2* v) {
                auto t = v->x;
                v->x = v->y;
                v->y = t;
                return v;
            };

            // When not using pointers, complete structures are copied via the stack.
            // Potentially a copy of the structure is pushed onto the stack to make the function
            // call and then another copy pushed onto the stack for the return value.
            Vec2 vecOriginal = { 3, 5 };
            auto vecCopy = SwapWithCopy(vecOriginal);
            AssertAreNotSame(&vecOriginal, &vecCopy, L"vecOriginal should have been copied to a new structure");
            Assert::AreEqual(vecOriginal.x, vecCopy.y, L"X component should have been swapped");
            Assert::AreEqual(vecOriginal.y, vecCopy.x, L"Y component should have been swapped");

            // Using a pointer allows the original object to be referenced and updated directly.
            // Only the pointer value is pushed to and popped from the stack. Potentially, this
            // will even be optimised away by the compiler with the value being passed via a
            // register.
            auto pVecOriginal = SwapInPlace(&vecOriginal);
            Assert::AreEqual(5, vecOriginal.x, L"X component should have been updated");
            Assert::AreEqual(3, vecOriginal.y, L"Y component should have been updated");
            AssertAreSame(&vecOriginal, pVecOriginal, L"pVecOriginal should point to vecOriginal");
            Assert::AreEqual(vecOriginal.x, pVecOriginal->x, L"X component should match through both views");
            Assert::AreEqual(vecOriginal.y, pVecOriginal->y, L"Y component should match through both views");
        }

        TEST_METHOD(Null_Pointers)
        {
            auto Check = [](const char* message) {
                if (!message) return false;
                return strcmp("Hello World!", message) == 0;
            };

            // Passing valid pointers should check the message contents as expected.
            Assert::IsTrue(Check("Hello World!"));
            Assert::IsFalse(Check("Goodbye!"));
            // Since C++11, nullptr has been available as a built in constant value.
            // As the compiler is aware of it, it is able to check that you are only using it with
            // pointer types.
            Assert::IsFalse(Check(nullptr));
            // Prior to that, most compilers provided a NULL macro that just maps to the value 0.
            Assert::IsFalse(Check(NULL));
            // However, the compiler can't type check it and so it can be used in invalid contexts.
            int number = NULL;
            Assert::AreEqual(0, number, L"NULL should have been assigned as the integer value");

            // Try changing the assignment above to "int number = nullptr;"...
        }

        TEST_METHOD(References)
        {
            // References are just syntactic sugar around pointers, the final code produced by the
            // compiler should be the same.
            auto Swap = [](Vec2& v) {
                auto t = v.x;
                v.x = v.y;
                v.y = t;
            };

            Vec2 vec = { 7, 11 };
            Swap(vec);
            Assert::AreEqual(11, vec.x, L"X component should have been updated");
            Assert::AreEqual(7, vec.y, L"Y component should have been updated");

            // However, you can't assign nullptr or NULL to a reference directly, so don't use them
            // for values that could be nullish.
            // Once a reference is bound to a variable, it can not be changed to reference a
            // different variable even if the original variable reaches the end of its life time.
        }

        //---------------------------------------------------------------------------------------//
        // Ok, there are situations where references do behave differently to pointers...
        // Here's a down casting example.
        //---------------------------------------------------------------------------------------//
        class Base {
        public:
            Base(int value) : baseInt(value) {}
            virtual ~Base() {} // We need a vtable to make this a polymorphic type
            int baseInt;
        };

        class Derived1 : public Base {
        public:
            Derived1(int base, int derived) : Base(base), derivedInt(derived) {}
            int derivedInt;
        };

        class Derived2 : public Base {
        public:
            Derived2(int base, float derived) : Base(base), derivedFloat(derived) {}
            float derivedFloat;
        };

        TEST_METHOD(References_Down_Casting_Exception)
        {
            // When working with polymorphic types and you have RTTI enabled, dynamic_cast is able
            // to check if a down cast to a derived type is of the correct type.
            Base* pBase = new Derived1(1, 2);
            Derived1* pDerived1 = dynamic_cast<Derived1*>(pBase);
            Assert::IsNotNull(pDerived1, L"Base pointer should have been dynamically down cast to Derived1");
            Assert::AreEqual(1, pDerived1->baseInt);
            Assert::AreEqual(2, pDerived1->derivedInt);

            // If the object pointed to by the base pointer isn't of the correct type, dynamic_cast
            // will return nullptr.
            Derived2* pDerived2 = dynamic_cast<Derived2*>(pBase);
            Assert::IsNull(pDerived2, L"Base pointer should not have been dynamically down cast to Derived2");

            // However, you can also use dynamic_cast on references.
            auto attempt_down_cast = [](Base& ref) {
                Derived1& derived = dynamic_cast<Derived1&>(ref);
                Assert::AreEqual(1, derived.baseInt);
                Assert::AreEqual(2, derived.derivedInt);
            };

            attempt_down_cast(*pBase);
            delete pBase;

            // However, if the object referred to by the base reference isn't of the correct type
            // for the cast, then a std::bad_cast exception is thrown as it's not really valid for
            // a reference to be null.
            pBase = new Derived2(1, 2.0);
            AssertThrows<std::bad_cast>([=]() {
                attempt_down_cast(*pBase);
            });
            delete pBase;
        }

        TEST_METHOD(References_Null_Check)
        {
            // This is an example of how you could pass a null reference, but don't expect to see
            // much (if  any) code doing this. It's clunky and is counter to expectation about
            // reference validity.
            auto IsNull = [](const int& numberToCheck) {
                return &numberToCheck == nullptr;
            };

            int number = 3;
            Assert::IsFalse(IsNull(number), L"Properly used references should not be null");
            Assert::IsTrue(IsNull(*(int*)nullptr), L"Coerced reference should be null");

            // Generally, I would recommend only using references for values you "borrow" for
            // for quick access or modification but don't intend to keep hold of.
        }

        TEST_METHOD(Pointer_Arithmetic)
        {
            // Language arrays are really just pointers to blocks of memory that the compiler can
            // directly reason about.
            float numbers[]{ 0.1f, 0.2f, 0.3f, 0.4f };
            float* pNumber = numbers;

            for (auto i = 0; i < _countof(numbers); i++)
            {
                Assert::AreEqual(numbers[i], *pNumber, L"pNumber should point to the expected array value");
                // You can update items in the array via the pointer.
                *pNumber += 1.0;
                // The compiler calculates how to update the pointer based on its memory type and
                // any platform alignment requirements.
                pNumber++;
            }

            AssertArrayEqual({ 1.1f, 1.2f, 1.3f, 1.4f }, numbers, _countof(numbers), L"Original array should have been updated via the pointer");
        }

        TEST_METHOD(Array_Syntax_With_Pointers)
        {
            // As language arrays can be treated as pointers, we can use array syntax with pointers
            // too.
            float numbers[]{ 0.1f, 0.2f, 0.3f, 0.4f };
            float* pNumbers = numbers;

            for (auto i = 0; i < _countof(numbers); i++)
            {
                Assert::AreEqual(numbers[i], pNumbers[i], L"Values should match");
            }

            // However, the compiler can not determine the size of the array via a pointer.
            // Try changing "_countof(numbers)" to "_countof(pNumbers)"...
        }

        TEST_METHOD(Void_Star)
        {
            // You can assign a pointer of any type to a void* pointer.
            int number = 13;
            void* pNumberVoidStar = &number;

            // You can compare void* pointers with any other pointer.
            int* pNumber = &number;
            Assert::IsTrue(pNumberVoidStar == pNumber, L"Both pointer should point to the same location");

            // However, you need to cast it to a pointer of a specific type in order to use it to
            // read or modify a value.
            Assert::AreEqual(13, *(int*)pNumberVoidStar, L"pNumberVoidStar should retrieve that value from number");

            // You also can't perform pointer arithmetic on a void* pointer without casting it to 
            // a specific type.
            // Generally, they're used when you want an opaque pointer, e.g. hiding context
            // implementation types for stateful APIs or where the API doesn't care about the type
            // of data stored at the address pointed to, e.g. raw IO or raw memory operations.

            //--------------------------------------------------------------------------------------------//
            // Example memory copier that can copy any data type.
            // (Please don't do this yourself, the standard library already has a memcpy() function.)
            //--------------------------------------------------------------------------------------------//
            auto copy = [](const void* src, void* dst, size_t size) {
                auto pSrc = (const uint8_t*)src;
                auto pDst = (uint8_t*)dst;
                while (size--)
                    *(pDst++) = *(pSrc++);
            };

            // We can now copy data between the arrays.
            int source[] = { 3, 5, 7, 11 };
            int target[_countof(source)];

            copy(source, target, sizeof(source));

            Assert::AreEqual(3, target[0], L"Index 0 should match source data");
            Assert::AreEqual(5, target[1], L"Index 1 should match source data");
            Assert::AreEqual(7, target[2], L"Index 2 should match source data");
            Assert::AreEqual(11, target[3], L"Index 3 should match source data");
            
            // Raw values can also be copied.
            double doubleNumber1 = 12345.6789;
            double doubleNumber2 = 0;

            copy(&doubleNumber1, &doubleNumber2, sizeof(double));

            Assert::AreEqual(12345.6789, doubleNumber2, L"doubleNumber2's value should have been copied from doubleNumber1");

            // However, it does have risks if you're not careful what you're copying to where.
            float floatNumber = -1.0;
            uint32_t uintNumber = 0;

            copy(&floatNumber, &uintNumber, sizeof(uintNumber));

            Assert::AreEqual(0xBF800000, uintNumber, L"Raw floating point binary representation should have been copied into uintNumber");
        }

        TEST_METHOD(C_Strings)
        {
            // Pointers can point to memory statically allocated by the compiler, e.g. string
            // literals.
            const char* pMessage = "Hello World!";

            // C strings are terminated with a zero character '\0' so we can determine the length
            // by scanning for the terminal.
            // (Please don't do this yourself though, use strlen()...)
            size_t count = 0;
            const char* pReader = pMessage;
            while (*pReader++)
                count++;

            Assert::AreEqual(strlen(pMessage), count, L"Our count should match the value return by the standard library strlen() function");
        }

        TEST_METHOD(Pointers_To_Pointers)
        {
            // It's possible to have pointers, to pointers, to pointers...
            int number = 101;
            int* pNumber = &number;
            int** ppNumber = &pNumber;
            int*** pppNumber = &ppNumber;

            Assert::AreEqual(101, ***pppNumber, L"The original value should be accessible via the pointer chain");
        }
    };
}