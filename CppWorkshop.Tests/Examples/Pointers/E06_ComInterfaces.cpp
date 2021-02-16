/*
 * Windows APIs such as DirectX are exposed via COM. COM is a low level API for sharing
 * componentised code both interal and external to the process. I won't go into the details of COM
 * itself here, but it provides another model of memory management as allocations can exist on
 * different heaps external to the running application.
 * 
 * COM manages object life cycles via explicit reference counting. When you you take a copy of
 * a reference to a COM object, you are required to increment the reference count. And then when
 * you are finished with a reference, decrement the count. Objects are always provided to you with
 * the initial count set for you. This may not be 1 as another process could potentially also have
 * a reference to the same object. This is very common with factory objects.
 * 
 * The base type of all COM objects is IUnknown and provides the AddRef() and Release() methods for
 * managing the reference count.
 *
 * As can be expected, this low level API can be error prone to use causing system wide cross
 * process memory leaks if a single applications fails to use it correctly. Alternatively, your
 * process can crash due to invalid memory access if you fail to track your references correctly
 * and an object you're using is destructed because its reference count drops to zero unexpectedly.
 * 
 * The example com_ptr wrapper is designed to automate the management of a COM resource in a
 * similar way to unique_ptr and shared_ptr. It should be usuable in the same way as a raw COM
 * pointer with no additional overheads despite it providing automatic life cycle management.
 * It's inspired by ComPtr which is part of the Windows Runtime Template Library, an API for
 * creating Windows store apps.
 * https://docs.microsoft.com/en-us/cpp/cppcx/wrl/comptr-class?view=msvc-160
 * 
 * For further reading on COM, see MSDN:
 * https://docs.microsoft.com/en-us/cpp/atl/introduction-to-com?view=msvc-160
 */
#include "pch.h"
#include "Wrappers/com_ptr.h"
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Pointers
{
	// A mock to represent a COM resource. All COM resources have AddRef() and Release() methods to
	// manage the reference count. Although we construct the object directly in this example,
	// libraries that provide COM objects will provide specific APIs to fetch them, usually via a
	// factory, rather than your code explicitly constructing them.
	class MockResource
	{
	public:
		MockResource() : m_AddCount(1), m_DecCount(0) {}

		void AddRef() { m_AddCount++; }
		void Release() { m_DecCount++; }
		int GetRefCount() const { return m_AddCount - m_DecCount; }
		int GetAddCount() const { return m_AddCount; }
		int GetDecCount() const { return m_DecCount; }

		com_ptr<MockResource> Wrap() { return this; }

	private:
		int m_AddCount;
		int m_DecCount;
	};

	TEST_CLASS(E06_ComInterfaces)
	{
	public:

		TEST_METHOD(Constructor_Default)
		{
			com_ptr<MockResource> resource;

			Assert::IsNull(resource.Get(), L"Resource was not initialised to a null state");
			// The com_ptr template is set up so that it has no virtual table and can't be
			// extended. This is to ensure it has no additional overheads compared to a raw COM
			// pointer.
			Assert::AreEqual(sizeof(MockResource*), sizeof(resource), L"Resource wrapper was not the same size as a pointer");
			Assert::AreEqual((void*)&resource, (void*)resource.GetPP(), L"GetPP() did not return correct address");
		}

		TEST_METHOD(Constructor_ComPointer)
		{
			// You can construct a com_ptr using a raw COM pointer directly. This assumes you have
			// received the raw pointer via another API and so won't adjust the reference count.
			MockResource mock;
			Assert::AreEqual(1, mock.GetAddCount(), L"Reference count incremented unexpectedly");
			Assert::AreEqual(0, mock.GetDecCount(), L"Reference count decremented unexpectedly");

			com_ptr<MockResource> resource(&mock);

			Assert::AreEqual((void*)&mock, (void*)resource.Get(), L"Incorrect pointer to mock returned");
			Assert::AreEqual(1, mock.GetAddCount(), L"Reference count incremented unexpectedly");
			Assert::AreEqual(0, mock.GetDecCount(), L"Reference count decremented unexpectedly");
		}

		TEST_METHOD(Constructor_Copy)
		{
			// Taking a copy of the reference should increment the count.
			MockResource mock;
			com_ptr<MockResource> resource1(&mock);
			auto resource2 = resource1;

			Assert::AreEqual((void*)resource1.Get(), (void*)resource2.Get(), L"Incorrect pointer to mock returned");
			Assert::AreEqual(2, mock.GetAddCount(), L"Reference count incremented unexpectedly");
			Assert::AreEqual(0, mock.GetDecCount(), L"Reference count decremented unexpectedly");
		}

		TEST_METHOD(Constructor_Move)
		{
			// The wrapper supports move semantics making it free to return by value from functions.
			MockResource mock;
			auto resource = mock.Wrap();

			Assert::AreEqual((void*)&mock, (void*)resource.Get(), L"Incorrect pointer to mock returned");
			// Despite it looking like the value is copied during the assignment, we can see that the
			// reference count was only ever 1 and so no additional references we taken.
			Assert::AreEqual(1, mock.GetAddCount(), L"Reference count incremented unexpectedly");
			Assert::AreEqual(0, mock.GetDecCount(), L"Reference count decremented unexpectedly");
		}

		TEST_METHOD(Constructor_Null)
		{
			// It's potentially possible for references to be null depending on the API.
			com_ptr<MockResource> mock(nullptr);
			Assert::IsTrue(mock == nullptr, L"Resource pointer wasn't null");
		}

		TEST_METHOD(MemberAccessOperator)
		{
			// Through the wrapper, we can access members of the wrapped object as if it were a raw
			// pointer.
			MockResource mock;
			auto resource = mock.Wrap();
			Assert::AreEqual(1, resource->GetRefCount(), L"Reference count was incorrect");
			Assert::AreEqual(1, resource->GetAddCount(), L"Reference count incremented unexpectedly");
			Assert::AreEqual(0, resource->GetDecCount(), L"Reference count decremented unexpectedly");
		}

		TEST_METHOD(GetPP)
		{
			// A common way to initialise a COM pointer is to pass a pointer to the pointer that
			// you want to store the reference in to a factory function. The function will fetch
			// or construct an object instance and update your original pointer with the address of
			// the object.
			MockResource mock;
			com_ptr<MockResource> pMock;

			*pMock.GetPP() = &mock;
			Assert::AreEqual((void*)&mock, (void*)pMock.Get(), L"Incorrect pointer to mock returned");
			Assert::AreEqual(1, mock.GetRefCount(), L"Reference count was incorrect");
			Assert::AreEqual(1, mock.GetAddCount(), L"Reference count incremented unexpectedly");
			Assert::AreEqual(0, mock.GetDecCount(), L"Reference count decremented unexpectedly");
		}

		TEST_METHOD(GetPP_AssertOnUnsafeAccess)
		{
			// As the GetPP() method should only be used for initialisation, the wrapper guards
			// against unsafe calls to it which could cause the reference count to go out of sync
			// if the pointer was improperly updated.
			AssertThrows<std::logic_error>([]() {
				MockResource mock;
				auto resource = mock.Wrap();
				resource.GetPP();
			}, L"No exception thrown");
		}

		TEST_METHOD(Destructor_SingleReference)
		{
			// The wrapper support RAII for scope based life cycle management.
			MockResource mock;

			{
				auto resource = mock.Wrap();
				Assert::AreEqual(1, mock.GetRefCount(), L"Incorrect reference count");
			}

			Assert::AreEqual(1, mock.GetAddCount(), L"Reference count incremented unexpectedly");
			Assert::AreEqual(1, mock.GetDecCount(), L"Reference count decremented unexpectedly");
		}

		TEST_METHOD(Destructor_MultiReference)
		{
			// Taking multiple references and letting them go out of scope should update the
			// reference count in the expected, safe manner.
			MockResource mock;
			auto resource1 = mock.Wrap();

			{
				auto resource2 = resource1;
				Assert::AreEqual(2, mock.GetRefCount(), L"Incorrect reference count");
			}

			Assert::AreEqual(2, mock.GetAddCount(), L"Reference count incremented unexpectedly");
			Assert::AreEqual(1, mock.GetDecCount(), L"Reference count decremented unexpectedly");
		}

		TEST_METHOD(Assignment_Copy_ReferenceIsNotSet)
		{
			// The wrapper supports the assignment operator so that it can be updated from another
			// wrapper instance after initial construction.
			MockResource mock;
			auto resource1 = mock.Wrap();
			com_ptr<MockResource> resource2;
			Assert::IsNull(resource2.Get(), L"Invalid resource pointer returned");

			resource2 = resource1;

			Assert::AreEqual(2, mock.GetAddCount(), L"Reference count incremented unexpectedly");
			Assert::AreEqual(0, mock.GetDecCount(), L"Reference count decremented unexpectedly");
		}

		TEST_METHOD(Assignment_Copy_ReferenceIsSet)
		{
			// Updating the wrapper to point from once COM object to another will update both
			// reference counts.
			MockResource mock1;
			MockResource mock2;
			auto resource1 = mock1.Wrap();
			auto resource2 = mock2.Wrap();
			resource1 = resource2;

			Assert::AreEqual(1, mock1.GetAddCount(), L"Mock 1 reference count incremented unexpectedly");
			Assert::AreEqual(1, mock1.GetDecCount(), L"Mock 1 reference count decremented unexpectedly");
			Assert::AreEqual(2, mock2.GetAddCount(), L"Mock 2 reference count incremented unexpectedly");
			Assert::AreEqual(0, mock2.GetDecCount(), L"Mock 2 reference count decremented unexpectedly");
		}

		TEST_METHOD(Assignment_Move_ReferenceIsNotSet)
		{
			// Move semantics are supported to efficiently re-assigning wrapper instances without modifying
			// the reference count. This is desirable because the call could be marshalled across processes.
			MockResource mock;
			auto resource1 = mock.Wrap();
			com_ptr<MockResource> resource2;
			Assert::IsNull(resource2.Get(), L"Invalid resource pointer returned");

			resource2 = std::move(resource1);

			Assert::IsNull(resource1.Get(), L"Original resource was not set to null");
			Assert::AreEqual(1, mock.GetAddCount(), L"Reference count incremented unexpectedly");
			Assert::AreEqual(0, mock.GetDecCount(), L"Reference count decremented unexpectedly");
		}

		TEST_METHOD(Assignment_Move_ReferenceIsSet)
		{
			// Even if a wrapper instance is already set, moving a new value into it will update
			// both reference counts as required.
			MockResource mock1;
			MockResource mock2;
			auto resource1 = mock1.Wrap();
			auto resource2 = mock2.Wrap();

			resource2 = std::move(resource1);

			Assert::IsNull(resource1.Get(), L"Original resource was not set to null");
			Assert::AreEqual(1, mock1.GetAddCount(), L"Mock 1 reference count incremented unexpectedly");
			Assert::AreEqual(0, mock1.GetDecCount(), L"Mock 1 reference count decremented unexpectedly");
			Assert::AreEqual(1, mock2.GetAddCount(), L"Mock 2 reference count incremented unexpectedly");
			Assert::AreEqual(1, mock2.GetDecCount(), L"Mock 2 reference count decremented unexpectedly");
		}

		TEST_METHOD(Assignment_Null_ReferenceIsSet)
		{
			// It's possible to explicitly release a reference and decrement the count by assigning
			// nullptr to the wrapper instance.
			MockResource mock;
			auto resource = mock.Wrap();
			resource = nullptr;

			Assert::IsNull(resource.Get(), L"Invalid resource pointer returned");
			Assert::AreEqual(1, mock.GetAddCount(), L"Reference count incremented unexpectedly");
			Assert::AreEqual(1, mock.GetDecCount(), L"Reference count decremented unexpectedly");
		}

		TEST_METHOD(Assignment_Null_ReferenceIsNotSet)
		{
			// It's safe to assign nullptr even if the reference isn't set.
			com_ptr<MockResource> resource;
			resource = nullptr;

			Assert::IsNull(resource.Get(), L"Invalid resource pointer returned");
		}

		TEST_METHOD(OperatorEquals_Resource)
		{
			// You can compare two wrapper instances to see if they point to the same object. This
			// is only a comparision of the pointers not of the referenced object's value.
			MockResource mock1;
			MockResource mock2;
			auto resource1a = mock1.Wrap();
			auto resource1b = mock1.Wrap();
			auto resource2 = mock2.Wrap();

			Assert::IsTrue(resource1a == resource1b, L"Resource wrappers not equal");
			Assert::IsFalse(resource1a == resource2, L"Resource wrappers were equal");
		}

		TEST_METHOD(OperatorEquals_Null)
		{
			// You can check if a wrapper has been set by comparing it with nullptr.
			MockResource mock;
			auto resource1 = mock.Wrap();
			com_ptr<MockResource> resource2;

			Assert::IsFalse(resource1 == nullptr, L"Resource 1 was null");
			Assert::IsTrue(resource2 == nullptr, L"Resource 2 was not null");
		}

		TEST_METHOD(OperatorNotEquals_Resource)
		{
			// Not equal can also be used to compare references.
			MockResource mock1;
			MockResource mock2;
			auto resource1a = mock1.Wrap();
			auto resource1b = mock1.Wrap();
			auto resource2 = mock2.Wrap();

			Assert::IsFalse(resource1a != resource1b, L"Resource wrappers not equal");
			Assert::IsTrue(resource1a != resource2, L"Resource wrappers were equal");
		}

		TEST_METHOD(OperatorNotEquals_Null)
		{
			// And you can check that the reference is not equal to nullptr.
			MockResource mock;
			auto resource1 = mock.Wrap();
			com_ptr<MockResource> resource2;

			Assert::IsTrue(resource1 != nullptr, L"Resource 1 was null");
			Assert::IsFalse(resource2 != nullptr, L"Resource 2 was not null");
		}

		TEST_METHOD(OperatorBool_IsNotNull)
		{
			// The bool operator has been overridden to support raw pointer style if checks.
			MockResource mock;
			auto resource = mock.Wrap();

			if (!resource) Assert::Fail(L"Resource didn't evaluate to true");
		}

		TEST_METHOD(OperatorBool_IsNull)
		{
			// The bool operator has been overridden to support raw pointer style if checks.
			com_ptr<MockResource> resource;

			if (resource) Assert::Fail(L"Resource evaluated to true");
		}

		TEST_METHOD(CollectionInteraction)
		{
			// The wrapper can be safely used with STL collections.
			MockResource mock;
			auto resource = mock.Wrap();
			std::vector<com_ptr<MockResource>> collection;

			// Stick a bunch of copies of the resource into the collection
			collection.push_back(resource);
			collection.push_back(resource);
			collection.push_back(resource);
			Assert::AreEqual(4, mock.GetRefCount(), L"Wrong number of references taken after filling collection");

			// Ensure iteration results in the correct reference counting
			for (auto& r : collection)
			{
				Assert::AreEqual(4, mock.GetRefCount(), L"Wrong reference count returned from wrapped resource");
				Assert::AreEqual((void*)&mock, (void*)r.Get(), L"Wrong resource pointer returned");
			}

			// Remove a single item
			collection.pop_back();
			Assert::AreEqual(3, mock.GetRefCount(), L"Wrong number of references taken after removing a single item");

			// Remove the remaining items, the only reference left should be to the original one
			collection.clear();
			Assert::AreEqual(1, mock.GetRefCount(), L"Wrong number of references taken after clearing collection");
		}
	};
}