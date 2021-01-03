// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here

#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>
#include "CppUnitTest.h"

// Test Helpers
template<typename T> inline void AssertAreSame(const T* a, const T* b, const wchar_t* message = nullptr)
{
	Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual((void*)a, (void*)b, message);
}

template<typename T> inline void AssertAreNotSame(const T* a, const T* b, const wchar_t* message = nullptr)
{
	Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreNotEqual((void*)a, (void*)b, message);
}

template<typename T> inline void AssertArrayEqual(const std::vector<T> expected, const T* actual, size_t size, const wchar_t* message = nullptr)
{
	Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual(expected.size(), size, message);
	for (size_t i = 0; i < size; i++)
		Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual(expected[i], actual[i], message);
}

#endif //PCH_H
