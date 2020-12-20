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
			// A pointer is just a variable that holds the memory address of something else (e.g. a variable or other object).
			int number = 123;
			int* pNumber = &number;
			Assert::AreEqual(number, *pNumber, L"pNumber should point to the variable number");

			// Changes to the original value can be seen via the pointer.
			number = 456;
			Assert::AreEqual(number, *pNumber, L"Updating number should also been seen by pNumber");

			// You can update the original value via the pointer.
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
			// For legacy reasons going back to 1972, the parser treats '*' as attached to the variable name, not the type.
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
			// Potentially a complete copy of the structure is pushed onto the stack to make the function call and then
			// another copy pushed onto the stack for the result value.
			Vec2 vecOriginal = { 3, 5 };
			auto vecCopy = SwapWithCopy(vecOriginal);
			AssertAreNotSame(&vecOriginal, &vecCopy, L"vecOriginal should have been copied to a new structure");
			Assert::AreEqual(vecOriginal.x, vecCopy.y, L"X component should have been swapped");
			Assert::AreEqual(vecOriginal.y, vecCopy.x, L"Y component should have been swapped");

			// Using pointers allow original objects to be referenced and updated directly.
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
			// As the compiler is aware of it, it is able to check that you are only using it with pointer types.
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
			// References are just syntactic sugar around pointers, the final code produced by the compuler should be the same.
			auto Swap = [](Vec2& v) {
				auto t = v.x;
				v.x = v.y;
				v.y = t;
			};

			Vec2 vec = { 7, 11 };
			Swap(vec);
			Assert::AreEqual(11, vec.x, L"X component should have been updated");
			Assert::AreEqual(7, vec.y, L"Y component should have been updated");

			// However, you can't assign nullptr or NULL to a reference directly, so don't use them for values that could be nullish.
		}

		TEST_METHOD(Pointer_Arithmetic)
		{
			// Language arrays are really just pointers to blocks of memory that the compiler can directly reason about.
			float numbers[]{ 0.1f, 0.2f, 0.3f, 0.4f };
			float* pNumber = numbers;

			for (auto i = 0; i < _countof(numbers); i++)
			{
				Assert::AreEqual(numbers[i], *pNumber, L"pNumber should point to the expected array value ");
				// You can update items in the array via the pointer.
				*pNumber += 1.0;
				// The compiler calculates how to update the pointer based on its memory type and any platform alignment requirements.
				pNumber++;
			}

			AssertArrayEqual({ 1.1f, 1.2f, 1.3f, 1.4f }, numbers, _countof(numbers), L"Original array should have been updated via the pointer");
		}

		TEST_METHOD(Array_Syntax_With_Pointers)
		{
			// As language arrays can be treated as pointers, we can use array syntax with pointers too.
			float numbers[]{ 0.1f, 0.2f, 0.3f, 0.4f };
			float* pNumbers = numbers;

			for (auto i = 0; i < _countof(numbers); i++)
			{
				Assert::AreEqual(numbers[i], pNumbers[i], L"Values should match");
			}

			// However, the compiler can not determine the size of the array via a pointer
			// Try changing "_countof(numbers)" to "_countof(pNumbers)"...
		}

		TEST_METHOD(C_Strings)
		{
			// Pointers can point to memory statically allocated by the compiler, e.g. string literals.
			const char* pMessage = "Hello World!";

			// C strings are terminated with a zero character '\0' so we can determine the length by scanning for the terminal.
			// (Please don't do this yourself though...)
			size_t count = 0;
			const char* pReader = pMessage;
			while (*pReader++)
				count++;

			Assert::AreEqual(strlen(pMessage), count, L"Out count should match the value return by the standard library strlen() function");
		}
	};
}