#include "janus.hh"

#include <cstring>
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
	EXPECT_EQ(buf1.read_offset(), 0);

	// We have a move ctor, let's make sure it works properly.
	auto buf2 = janus::dynamic_buffer(9);
	buf2.add_uint64(123);
	buf2.read_uint64();
	auto buf3 = std::move(buf2);
	EXPECT_EQ(buf3.cap(), 16);
	EXPECT_EQ(buf3.size(), 8);
	EXPECT_EQ(buf3.read_offset(), 8);
	EXPECT_EQ(buf2.cap(), 0);
	EXPECT_EQ(buf2.size(), 0);
	EXPECT_EQ(buf2.read_offset(), 0);

	// We can set size + expect to get aligned UP to 64-bits.
	auto buf4 = janus::dynamic_buffer(100, 39);
	EXPECT_EQ(buf4.cap(), 104);
	EXPECT_EQ(buf4.size(), 40);
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

	uint64_t& n1 = buf.add_uint64(123456);
	EXPECT_EQ(buf.size(), 8);

	uint64_t& n2 = buf.add_uint64(654321);
	EXPECT_EQ(buf.size(), 16);

	EXPECT_EQ(buf.data()[0], 123456);
	EXPECT_EQ(buf.data()[1], 654321);

	EXPECT_EQ(n1, 123456);
	EXPECT_EQ(n2, 654321);

	n1 = 999;
	n2 = 888;

	EXPECT_EQ(buf.data()[0], 999);
	EXPECT_EQ(buf.data()[1], 888);

	EXPECT_EQ(n1, 999);
	EXPECT_EQ(n2, 888);
}

// Assert that .reset() correctly empties the buffer.
TEST(dynamic_buffer_test, reset)
{
	auto buf = janus::dynamic_buffer(16);
	buf.add_uint64(123456);
	buf.add_uint64(654321);
	EXPECT_EQ(buf.size(), 16);
	EXPECT_EQ(buf.read_offset(), 0);

	// Now read a value.
	EXPECT_EQ(buf.read_uint64(), 123456);
	EXPECT_EQ(buf.read_offset(), 8);

	buf.reset();
	EXPECT_EQ(buf.size(), 0);
	// .reset() should also reset the read offset.
	EXPECT_EQ(buf.read_offset(), 0);

	// We should be able to insert new data after reset.
	buf.add_uint64(123456);
	EXPECT_EQ(buf.size(), 8);
	EXPECT_EQ(buf.read_offset(), 0);
	EXPECT_EQ(buf.data()[0], 123456);
	// And read.
	EXPECT_EQ(buf.read_uint64(), 123456);
	EXPECT_EQ(buf.read_offset(), 8);
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

	// Try to exceed read limits.

	auto buf3 = janus::dynamic_buffer(16);
	EXPECT_EQ(buf3.read_offset(), 0);
	EXPECT_EQ(buf3.size(), 0);

	buf3.add_uint64(123);
	EXPECT_EQ(buf3.read_offset(), 0);
	EXPECT_EQ(buf3.size(), 8);

	buf3.add_uint64(456);
	EXPECT_EQ(buf3.read_offset(), 0);
	EXPECT_EQ(buf3.size(), 16);

	// Adding an additional value should result in an overflow.
	EXPECT_THROW(buf3.add_uint64(0), std::runtime_error);

	EXPECT_EQ(buf3.read_uint64(), 123);
	EXPECT_EQ(buf3.read_offset(), 8);

	EXPECT_EQ(buf3.read_uint64(), 456);
	EXPECT_EQ(buf3.read_offset(), 16);

	// Reading another uint64 should result in an overflow.
	EXPECT_THROW(buf3.read_uint64(), std::runtime_error);

	// Now try writing and reading as we go.

	buf3.reset();
	EXPECT_EQ(buf3.size(), 0);
	EXPECT_EQ(buf3.read_offset(), 0);

	buf3.add_uint64(888);
	EXPECT_EQ(buf3.read_uint64(), 888);
	EXPECT_EQ(buf3.read_offset(), 8);
	// We shouldn't be able to exceed the write offset.
	EXPECT_THROW(buf3.read_uint64(), std::runtime_error);

	buf3.add_uint64(999);
	EXPECT_EQ(buf3.read_uint64(), 999);
	EXPECT_EQ(buf3.read_offset(), 16);
	EXPECT_THROW(buf3.read_uint64(), std::runtime_error);
}

// Ensure that .add_raw() behaves correctly and successfully adds raw data to
// the buffer.
TEST(dynamic_buffer_test, add_raw)
{
	auto buf1 = janus::dynamic_buffer(16);

	std::array<unsigned char, 5> data1 = {0xde, 0xad, 0xbe, 0xef, 42};
	auto* raw1 = static_cast<uint8_t*>(buf1.add_raw(&data1, sizeof(data1)));
	// Round up to uinit64 alignment.
	EXPECT_EQ(buf1.size(), 8);
	EXPECT_EQ(std::memcmp(raw1, &data1, sizeof(data1)), 0);

	// Rest of the buffer should be empty.
	for (int i = 5; i < 8; i++) {
		EXPECT_EQ(raw1[i], 0);
	}

	// Zero size should not throw.
	EXPECT_NO_THROW(buf1.add_raw(&data1, 0));

	// Test a buffer size more than 8 bytes aligned.
	auto buf2 = janus::dynamic_buffer(9);
	EXPECT_EQ(buf1.cap(), 16);
	std::array<unsigned char, 9> data2 = {1, 2, 3, 4, 5, 6, 7, 8, 9};
	auto* raw2 = static_cast<uint8_t*>(buf2.add_raw(&data2, sizeof(data2)));
	EXPECT_EQ(buf2.size(), 16);
	EXPECT_EQ(std::memcmp(raw2, &data2, sizeof(data2)), 0);
	for (int i = 9; i < 16; i++) {
		EXPECT_EQ(raw1[i], 0);
	}

	// Try an aligned buffer size.
	auto buf3 = janus::dynamic_buffer(8);
	EXPECT_EQ(buf3.cap(), 8);
	std::array<unsigned char, 8> data3 = {1, 2, 3, 4, 5, 6, 7, 8};
	auto* raw3 = static_cast<uint8_t*>(buf3.add_raw(&data3, sizeof(data3)));
	EXPECT_EQ(buf3.size(), 8);
	EXPECT_EQ(std::memcmp(raw3, &data3, sizeof(data3)), 0);

	// Try adding more than once and ensure that there isn't any incorrect
	// aliasing.

	auto buf4 = janus::dynamic_buffer(16);
	EXPECT_EQ(buf4.cap(), 16);
	std::array<unsigned char, 8> data4 = {1, 2, 3};
	auto* raw4 = static_cast<uint8_t*>(buf4.add_raw(&data4, sizeof(data4)));
	EXPECT_EQ(buf4.size(), 8);
	EXPECT_EQ(std::memcmp(raw4, &data4, sizeof(data4)), 0);

	// Modify original data to make sure there's no unexpected aliasing.

	// Make sure modifying original bytes doesn't affect raw.
	data4[0] = 4;
	data4[1] = 5;
	data4[2] = 6;
	EXPECT_EQ(raw4[0], 1);
	EXPECT_EQ(raw4[1], 2);
	EXPECT_EQ(raw4[2], 3);

	// Make sure modifying raw bytes doesn't affect original.
	raw4[0] = 7;
	raw4[1] = 8;
	raw4[2] = 9;
	EXPECT_EQ(data4[0], 4);
	EXPECT_EQ(data4[1], 5);
	EXPECT_EQ(data4[2], 6);

	// Now re-add the modified data.
	auto* raw5 = static_cast<uint8_t*>(buf4.add_raw(&data4, sizeof(data4)));
	EXPECT_EQ(buf4.size(), 16);
	EXPECT_EQ(raw5[0], 4);
	EXPECT_EQ(raw5[1], 5);
	EXPECT_EQ(raw5[2], 6);

	// Original raw data should remain unchanged.
	EXPECT_EQ(raw4[0], 7);
	EXPECT_EQ(raw4[1], 8);
	EXPECT_EQ(raw4[2], 9);
}

// Test that .add() permits us to add arbitrary data types to the buffer.
TEST(dynamic_buffer_test, add)
{
	auto buf = janus::dynamic_buffer(16);

	struct arbitrary
	{
		int x;
		int y;
	} arb = {123, 456};

	// Add the data and assert it is as expected.
	auto& ret = buf.add(arb);
	EXPECT_EQ(buf.size(), sizeof(arbitrary));
	EXPECT_EQ(ret.x, 123);
	EXPECT_EQ(ret.y, 456);

	// Ensure that there is no aliasing from arb -> raw data.
	arb.x = 999;
	arb.y = 888;
	EXPECT_EQ(ret.x, 123);
	EXPECT_EQ(ret.y, 456);

	// Ensure there is no aliasing from raw data -> arb.
	ret.x = 333;
	ret.y = 444;
	EXPECT_EQ(arb.x, 999);
	EXPECT_EQ(arb.y, 888);

	// Now re-add arb.
	auto& ret2 = buf.add(arb);
	EXPECT_EQ(buf.size(), 2 * sizeof(arbitrary));
	EXPECT_EQ(ret2.x, 999);
	EXPECT_EQ(ret2.y, 888);

	// Ensure that the original remains unchanged.
	EXPECT_EQ(ret.x, 333);
	EXPECT_EQ(ret.y, 444);
}

// Test that .read_uint64() successfully reads a uint64 and increments the read
// offset.
TEST(dynamic_buffer_test, read_uint64)
{
	auto buf = janus::dynamic_buffer(16);
	EXPECT_EQ(buf.read_offset(), 0);
	EXPECT_EQ(buf.size(), 0);

	buf.add_uint64(123);
	EXPECT_EQ(buf.read_offset(), 0);
	EXPECT_EQ(buf.size(), 8);

	buf.add_uint64(456);
	EXPECT_EQ(buf.read_offset(), 0);
	EXPECT_EQ(buf.size(), 16);

	EXPECT_EQ(buf.read_uint64(), 123);
	EXPECT_EQ(buf.read_offset(), 8);

	EXPECT_EQ(buf.read_uint64(), 456);
	EXPECT_EQ(buf.read_offset(), 16);
}

// Test to ensure that .reset_read() correctly resets the read offset.
TEST(dynamic_buffer_test, reset_read)
{
	auto buf = janus::dynamic_buffer(16);
	EXPECT_EQ(buf.read_offset(), 0);
	buf.add_uint64(123456789);
	EXPECT_EQ(buf.read_uint64(), 123456789);
	EXPECT_EQ(buf.read_offset(), 8);

	buf.add_uint64(999);
	EXPECT_EQ(buf.read_uint64(), 999);
	EXPECT_EQ(buf.read_offset(), 16);

	buf.reset_read();
	EXPECT_EQ(buf.size(), 16);
	EXPECT_EQ(buf.read_offset(), 0);

	EXPECT_EQ(buf.read_uint64(), 123456789);
	EXPECT_EQ(buf.read_offset(), 8);
	EXPECT_EQ(buf.read_uint64(), 999);
	EXPECT_EQ(buf.read_offset(), 16);
}

// Test to ensure .read_raw() returns a pointer to arbitrary data in the buffer
// and correclty updates the read offset.
TEST(dynamic_buffer_test, read_raw)
{
	auto buf = janus::dynamic_buffer(16);

	std::array<uint8_t, 5> data1 = {0xde, 0xad, 0xbe, 0xef, 42};
	auto* raw1 = static_cast<uint8_t*>(buf.add_raw(&data1, sizeof(data1)));
	// Round up to uinit64 alignment.
	EXPECT_EQ(buf.size(), 8);
	EXPECT_EQ(std::memcmp(raw1, &data1, sizeof(data1)), 0);
	EXPECT_EQ(buf.read_offset(), 0);

	auto* raw2 = static_cast<uint8_t*>(buf.read_raw(5));
	EXPECT_EQ(std::memcmp(raw1, raw2, sizeof(data1)), 0);
	EXPECT_EQ(buf.read_offset(), 8);

	// Can't read past end of read offset.
	EXPECT_THROW(buf.read_raw(sizeof(data1)), std::runtime_error);

	std::array<uint8_t, 3> data2 = {0xfe, 0xef, 0xff};
	auto* raw3 = static_cast<uint8_t*>(buf.add_raw(&data2, sizeof(data2)));
	EXPECT_EQ(buf.size(), 16);
	EXPECT_EQ(std::memcmp(raw3, &data2, sizeof(data2)), 0);

	auto* raw4 = static_cast<uint8_t*>(buf.read_raw(sizeof(data2)));
	EXPECT_EQ(buf.read_offset(), 16);
	EXPECT_EQ(std::memcmp(raw4, &data2, sizeof(data2)), 0);
}

// Test to ensure .read() returns an arbitrary object reference correctly.
TEST(dynamic_buffer_test, read)
{
	auto buf = janus::dynamic_buffer(16);
	struct arbitrary
	{
		int x;
		int y;
	} arb = {123, 456};

	// Add the data and assert it is as expected.
	auto& ret = buf.add(arb);
	EXPECT_EQ(buf.size(), sizeof(arbitrary));
	EXPECT_EQ(ret.x, 123);
	EXPECT_EQ(ret.y, 456);

	auto& arb2 = buf.read<arbitrary>();
	EXPECT_EQ(arb.x, arb2.x);
	EXPECT_EQ(arb.y, arb2.y);

	arb.x = 333;
	arb.y = 444;
	buf.add(arb);
	EXPECT_EQ(buf.size(), 16);
	auto& arb3 = buf.read<arbitrary>();
	EXPECT_EQ(arb.x, arb3.x);
	EXPECT_EQ(arb.y, arb3.y);
}

// Test that .add_string() correctly encodes strings.
TEST(dynamic_buffer_test, add_string)
{
	auto buf = janus::dynamic_buffer(64);

	const char* str1 = "ohai";
	std::string_view ret1 = buf.add_string(str1, 4);
	EXPECT_EQ(buf.size(), 16);
	// Use strcmp to ensure that the null terminator is intact.
	EXPECT_EQ(std::strcmp(str1, ret1.data()), 0);
	EXPECT_EQ(ret1.size(), 4);

	// We encode the string with a uint64 first followed by the character buffer.
	EXPECT_EQ(buf.read_uint64(), 4);
	EXPECT_EQ(buf.read_offset(), 8);

	auto* ret2 = static_cast<char*>(buf.read_raw(5));
	EXPECT_EQ(buf.read_offset(), 16);
	EXPECT_EQ(std::strcmp(str1, ret2), 0);

	const char* str2 = "lorenzo";
	std::string_view ret3 = buf.add_string(str2, 7);
	EXPECT_EQ(buf.size(), 32);
	EXPECT_EQ(std::strcmp(str2, ret3.data()), 0);
	EXPECT_EQ(ret3.size(), 7);

	EXPECT_EQ(buf.read_uint64(), 7);
	EXPECT_EQ(buf.read_offset(), 24);

	auto* ret4 = static_cast<char*>(buf.read_raw(8));
	EXPECT_EQ(buf.read_offset(), 32);
	EXPECT_EQ(std::strcmp(str2, ret4), 0);

	// Ensure we can write a zero size string.
	std::string_view ret5 = buf.add_string(str2, 0);
	EXPECT_EQ(buf.size(), 40);
	EXPECT_EQ(buf.read_uint64(), 0);
	EXPECT_EQ(buf.read_offset(), 40);
	EXPECT_EQ(ret5.size(), 0);
	EXPECT_TRUE(ret5.empty());

	// Test sajson node overload.
	char json[] = R"(["foo",{}])";
	uint64_t json_size = sizeof(json) - 1;

	sajson::document doc = janus::internal::parse_json("", json, json_size);
	const sajson::value& root = doc.get_root();
	sajson::value val = root.get_array_element(0);
	std::string_view ret6 = buf.add_string(val);
	EXPECT_EQ(std::strcmp(ret6.data(), "foo"), 0);
	EXPECT_EQ(buf.size(), 56);

	// Null node should add an empty string.
	EXPECT_EQ(buf.add_string(root.get_array_element(1).get_value_of_key(
			  sajson::literal("charlie_the_unicorn"))),
		  "");
	EXPECT_EQ(buf.size(), 64);
}

// Test that .read_string() correctly reads a string from the buffer and returns
// a valid std::string_view.
TEST(dynamic_buffer_test, read_string)
{
	auto buf = janus::dynamic_buffer(48);

	const char* str1 = "ohai";
	buf.add_string(str1, 4);
	EXPECT_EQ(buf.size(), 16);
	EXPECT_EQ(buf.read_offset(), 0);

	std::string_view ret1 = buf.read_string();
	EXPECT_EQ(buf.read_offset(), 16);
	EXPECT_EQ(ret1.size(), 4);
	EXPECT_EQ(std::strcmp(str1, ret1.data()), 0);

	// Intentionally making this 8 chars long, as testing for a previous bug
	// where the null terminator was not encoded correctly, 8 + 1 should
	// move over to the next uint64 word and trigger this bug if present.
	const char* str2 = "Zayriyan";
	buf.add_string(str2, 8);
	EXPECT_EQ(buf.size(), 40);

	std::string_view ret2 = buf.read_string();
	EXPECT_EQ(buf.read_offset(), 40);
	EXPECT_EQ(ret2.size(), 8);
	EXPECT_EQ(std::strcmp(str2, ret2.data()), 0);

	// Ensure we can read a zero size string.
	buf.add_string(str2, 0);
	EXPECT_EQ(buf.size(), 48);
	std::string_view ret3 = buf.read_string();
	EXPECT_EQ(buf.read_offset(), 48);
	EXPECT_EQ(ret3.size(), 0);
	EXPECT_TRUE(ret3.empty());
}

// Test that we can correctly rewind the buffer and that it doesn't leave the
// buffer in an invalid state.
TEST(dynamic_buffer_test, rewind)
{
	auto buf = janus::dynamic_buffer(48);
	for (uint64_t i = 0; i < 6; i++) {
		buf.add_uint64(i * 100);
	}
	EXPECT_EQ(buf.size(), 48);

	// This should do nothing.
	buf.rewind(0);
	EXPECT_EQ(buf.size(), 48);

	// Should get rounded up to a word size.
	buf.rewind(1);
	EXPECT_EQ(buf.size(), 40);

	// Should get rounded down to buffer size.
	buf.rewind(1000000);
	EXPECT_EQ(buf.size(), 0);

	// Reinstate data.
	for (uint64_t i = 0; i < 6; i++) {
		buf.add_uint64(i * 100);
	}
	EXPECT_EQ(buf.size(), 48);

	// Update read offset to end of buffer.
	for (uint64_t i = 0; i < 6; i++) {
		buf.read_uint64();
	}
	EXPECT_EQ(buf.read_offset(), 48);

	// Rewind should also update read offset.
	buf.rewind(8);
	EXPECT_EQ(buf.size(), 40);
	EXPECT_EQ(buf.read_offset(), 40);
	buf.rewind(50);
	EXPECT_EQ(buf.size(), 0);
	EXPECT_EQ(buf.read_offset(), 0);
}

// Test that we can construct a dynamic buffer with a custom underlying buffer
// correctly.
TEST(dynamic_buffer_test, custom_buffer_ctor)
{
	uint64_t* raw = new uint64_t[100];

	auto buf = janus::dynamic_buffer(raw, 100);
	// We get rounded down to nearest 8 byte aligned value, rounding DOWN.
	EXPECT_EQ(buf.cap(), 96);

	// Setting values in the dynamic buffer should mutate the underlying
	// buffer.
	for (uint64_t i = 0; i < 12; i++) {
		buf.add_uint64(12 - i);
		ASSERT_EQ(raw[i], 12 - i);
	}

	// Setting values in the underlying buffer should mutate the dynamic
	// buffer.
	for (uint64_t i = 0; i < 12; i++) {
		raw[i] = i;
		ASSERT_EQ(buf.read_uint64(), i);
	}

	// Test setting the size (i.e. write offset) of the buffer
	// specifically. Everything is aligned here too.
	uint64_t* raw2 = new uint64_t[100];
	auto buf2 = janus::dynamic_buffer(raw2, 100, 50);
	EXPECT_EQ(buf2.cap(), 96);
	EXPECT_EQ(buf2.size(), 48);

	// Setting size > cap should result in size == cap.
	uint64_t* raw3 = new uint64_t[100];
	auto buf3 = janus::dynamic_buffer(raw3, 100, 200);
	EXPECT_EQ(buf3.cap(), 96);
	EXPECT_EQ(buf3.size(), 96);
}

// Test that we can reserve a block of zeroed memory using .reserve() correctly.
TEST(dynamic_buffer_test, reserve)
{
	auto buf = janus::dynamic_buffer(1000);
	EXPECT_EQ(buf.size(), 0);

	// Put some data in the first 8 bytes to make sure we're zeroing.
	buf.add_uint64(12345678);
	buf.reset();

	uint64_t* ptr1 = static_cast<uint64_t*>(buf.reserve(7));
	EXPECT_EQ(*ptr1, 0);
	*ptr1 = 999;
	// We should have got rounded up.
	EXPECT_EQ(buf.size(), 8);

	uint64_t* ptr2 = static_cast<uint64_t*>(buf.reserve(1));
	// Previous reservation should not be affected.
	EXPECT_EQ(*ptr1, 999);
	EXPECT_EQ(*ptr2, 0);
	*ptr2 = 123;
	EXPECT_EQ(*ptr1, 999);
	*ptr1 = 11111;
	EXPECT_EQ(*ptr2, 123);
}

TEST(dynamic_buffer_test, unread)
{
	auto buf = janus::dynamic_buffer(1000);

	struct foo
	{
		uint64_t x, y, z;
	};

	foo a = {.x = 1, .y = 2, .z = 3};
	foo b = {.x = 4, .y = 5, .z = 6};
	foo c = {.x = 7, .y = 8, .z = 9};

	buf.add(a);
	buf.add(b);
	buf.add(c);

	EXPECT_EQ(buf.size(), 72);
	EXPECT_EQ(buf.read_offset(), 0);

	EXPECT_EQ(buf.read<foo>().x, 1);
	EXPECT_EQ(buf.read_offset(), 24);

	EXPECT_EQ(buf.read<foo>().x, 4);
	EXPECT_EQ(buf.read_offset(), 48);

	EXPECT_EQ(buf.read<foo>().x, 7);
	EXPECT_EQ(buf.read_offset(), 72);

	buf.unread<foo>();
	EXPECT_EQ(buf.read<foo>().x, 7);
	EXPECT_EQ(buf.read_offset(), 72);

	buf.unread<foo>();
	buf.unread<foo>();
	EXPECT_EQ(buf.read<foo>().x, 4);
	EXPECT_EQ(buf.read_offset(), 48);

	buf.unread<foo>();
	buf.unread<foo>();
	EXPECT_EQ(buf.read<foo>().x, 1);
	EXPECT_EQ(buf.read_offset(), 24);
}
} // namespace
