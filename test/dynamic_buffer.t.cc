#include "dynamic_buffer.hh"

#include <gtest/gtest.h>
#include <stdexcept>

namespace
{
// Confirm that the constructor behaves as we would expect.
TEST(dynamic_buffer_test, ctor)
{
	// It is possible (though useless) to have a 0-size buffer.
	auto buf1 = janus::dynamic_buffer(0);
	EXPECT_EQ(buf1.cap(), 0);
	EXPECT_EQ(buf1.size(), 0);

	// We have a move ctor, let's make sure it works properly.
	auto buf2 = janus::dynamic_buffer(9);
	auto buf3 = std::move(buf2);
	EXPECT_EQ(buf3.cap(), 16);
	EXPECT_EQ(buf3.size(), 0);
	EXPECT_EQ(buf2.cap(), 0);
	EXPECT_EQ(buf2.size(), 0);
}

// Test that the .cap() method returns the correct capacity of the buffer.
TEST(dynamic_buffer_test, cap)
{
	// We expect alignment to 64-bits.
	for (uint64_t size = 1; size <= 8; size++) {
		auto buf1 = janus::dynamic_buffer(size);
		EXPECT_EQ(buf1.cap(), 8);
		EXPECT_EQ(buf1.size(), 0);
	}

	// Add an additional test case for size > 8.
	auto buf2 = janus::dynamic_buffer(9);
	EXPECT_EQ(buf2.cap(), 16);
	EXPECT_EQ(buf2.size(), 0);
}

// Test that the .size() method returns the correct size of the buffer.
TEST(dynamic_buffer_test, size)
{
	auto buf = janus::dynamic_buffer(15);
	// Capacity should be aligned to 64-bits, with 0 size initially.
	EXPECT_EQ(buf.cap(), 16);
	EXPECT_EQ(buf.size(), 0);

	// Add a couple of uint64's.
	buf.add_uint64(123);
	EXPECT_EQ(buf.size(), 8);
	buf.add_uint64(456);
	EXPECT_EQ(buf.size(), 16);

	// Reset should indicate size() zero.
	buf.reset();
	EXPECT_EQ(buf.size(), 0);
}

// Assert that the .data() method correctly returns a pointer to the underlying
// uint64_t buffer.
TEST(dynamic_buffer_test, data)
{
	auto buf = janus::dynamic_buffer(800);

	for (uint64_t i = 0; i < 100; i++) {
		buf.add_uint64(i);
		// Ensure data in buffer is as expected as we add to it.
		const uint64_t* ptr = buf.data();
		EXPECT_EQ(ptr[i], i);
	}
	EXPECT_EQ(buf.size(), 800);

	// Also check after adding data that it is all correct.
	for (uint64_t i = 0; i < 100; i++) {
		const uint64_t* ptr = buf.data();
		EXPECT_EQ(ptr[i], i);
	}
}

// Assert that .add_uint64() correctly adds uint64 values to the buffer.
TEST(dynamic_buffer_test, add_uint64)
{
	auto buf = janus::dynamic_buffer(16);

	buf.add_uint64(123456);
	EXPECT_EQ(buf.size(), 8);

	buf.add_uint64(654321);
	EXPECT_EQ(buf.size(), 16);

	EXPECT_EQ(buf.data()[0], 123456);
	EXPECT_EQ(buf.data()[1], 654321);
}

// Assert that .reset() correctly empties the buffer.
TEST(dynamic_buffer_test, reset)
{
	auto buf = janus::dynamic_buffer(16);
	buf.add_uint64(123456);
	buf.add_uint64(654321);
	EXPECT_EQ(buf.size(), 16);

	buf.reset();
	EXPECT_EQ(buf.size(), 0);

	// We should be able to insert new data after reset.
	buf.add_uint64(123456);
	EXPECT_EQ(buf.size(), 8);
	EXPECT_EQ(buf.data()[0], 123456);
}

// Ensure that the expected limits of the buffer are adhered to.
TEST(dynamic_buffer_test, limits)
{
	auto buf1 = janus::dynamic_buffer(0);
	// Any attempt to add data to a zero-size buffer should result in an
	// exception.
	EXPECT_THROW(buf1.add_uint64(123456), std::runtime_error);

	// Now assert that attempting to overflow a buffer with data also
	// throws.
	auto buf2 = janus::dynamic_buffer(16);
	buf2.add_uint64(123456);
	buf2.add_uint64(654321);
	EXPECT_EQ(buf2.size(), 16);
	EXPECT_THROW(buf2.add_uint64(0), std::runtime_error);

	// A reset then readding 16 bytes should be fine.
	buf2.reset();
	EXPECT_EQ(buf2.size(), 0);
	buf2.add_uint64(123456);
	buf2.add_uint64(654321);
	EXPECT_EQ(buf2.size(), 16);
}
} // namespace
