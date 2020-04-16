#include "janus.hh"

#include <cstdint>
#include <gtest/gtest.h>
#include <stdexcept>

namespace
{
// Test that the dynamic array's basic operations function correctly.
TEST(dynamic_array_test, basic)
{
	janus::dynamic_array<uint64_t, 10> arr;

	for (uint64_t i = 0; i < 10; i++) {
		uint64_t& n = arr.emplace_back(i);
		ASSERT_EQ(n, i);
		n++;
		EXPECT_EQ(arr.size(), i + 1);
	}

	for (uint64_t i = 0; i < 10; i++) {
		ASSERT_EQ(arr[i], i + 1);
	}

	EXPECT_THROW(arr.checked(10), std::runtime_error);

	arr.clear();
	EXPECT_EQ(arr.size(), 0);
}

// Test that ctors and dtors are invoked as expected.
TEST(dynamic_array_test, ctor_dtor)
{
	static uint64_t num_ctors;
	static uint64_t num_dtors;

	// Now ensure ctors and dtors are invoked as expected.

	// Reset on each run.
	num_ctors = 0;
	num_dtors = 0;

	struct foo
	{
		explicit foo(uint64_t i) : n(i)
		{
			num_ctors++;
		}

		~foo()
		{
			num_dtors++;
		}

		uint64_t n;
	};

	janus::dynamic_array<foo, 10> arr1;
	// Creating the array shouldn't create any foos.
	EXPECT_EQ(num_ctors, 0);

	// Ensure that we correctly construct elements.
	for (uint64_t i = 0; i < 10; i++) {
		foo& f = arr1.emplace_back(i);
		EXPECT_EQ(f.n, i);
		EXPECT_EQ(num_ctors, i + 1);
	}

	// Now clearing the array should invoke all dtors.
	arr1.clear();
	EXPECT_EQ(num_dtors, 10);

	// Now ensure top-level dtor does the right thing.

	num_ctors = 0;
	num_dtors = 0;

	{
		janus::dynamic_array<foo, 10> arr2;
		for (uint64_t i = 0; i < 10; i++) {
			foo& f = arr2.emplace_back(i);
			EXPECT_EQ(f.n, i);
			EXPECT_EQ(num_ctors, i + 1);
		}
	}
	EXPECT_EQ(num_ctors, 10);
	EXPECT_EQ(num_dtors, 10);

	// Finally, ensure that .truncate()'ing does not actually destroy
	// anything.

	num_ctors = 0;
	num_dtors = 0;

	janus::dynamic_array<foo, 10> arr3;
	for (uint64_t i = 0; i < 10; i++) {
		foo& f = arr3.emplace_back(i);
		EXPECT_EQ(f.n, i);
		EXPECT_EQ(num_ctors, i + 1);
	}
	EXPECT_EQ(num_ctors, 10);
	EXPECT_EQ(num_dtors, 0);

	arr3.truncate();
	EXPECT_EQ(num_dtors, 0);
}
} // namespace
