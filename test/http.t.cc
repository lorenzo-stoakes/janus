#include "janus.hh"

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

namespace
{
// Test that fundamental http_request functionality works correctly.
TEST(http_test, request)
{
	char buf[1000];
	janus::http_request req1(buf, 1000);

	EXPECT_FALSE(req1.complete());
	EXPECT_EQ(req1.cap(), 1000);

	// First a GET.

	char* ptr = req1.get("ljs.io");
	EXPECT_EQ(ptr, buf);
	// Because we might want to add headers, this shouldn't be marked complete.
	EXPECT_FALSE(req1.complete());

	// We won't have been null terminated yet as request doesn't know if we
	// want to add headers.
	buf[req1.size()] = '\0';
	EXPECT_STREQ(buf, "GET / HTTP/1.0\r\nHost: ljs.io\r\n");
	EXPECT_EQ(req1.size(), sizeof("GET / HTTP/1.0\r\nHost: ljs.io\r\n") - 1);

	// Now terminate it.
	ptr = req1.terminate();
	EXPECT_EQ(ptr, buf);
	EXPECT_TRUE(req1.complete());
	EXPECT_STREQ(buf, "GET / HTTP/1.0\r\nHost: ljs.io\r\n\r\n");

	// Now a GET with a path.

	janus::http_request req2(buf, 1000);
	ptr = req2.get("ljs.io", "/test.txt");
	EXPECT_EQ(ptr, buf);

	buf[req2.size()] = '\0';
	EXPECT_STREQ(buf, "GET /test.txt HTTP/1.0\r\nHost: ljs.io\r\n");

	// Any attempt to add further headers should throw.

	ASSERT_THROW(req2.get("foo"), std::runtime_error);
	ASSERT_THROW(req2.post("foo"), std::runtime_error);
	ASSERT_THROW(req2.op("PUT", "foo"), std::runtime_error);

	req2.terminate();

	// Trying to terminate again should throw.
	ASSERT_THROW(req2.terminate(), std::runtime_error);

	// Make sure .op() does the right thing.
	janus::http_request req3(buf, 1000);
	ptr = req3.op("PUT", "ljs.io", "/test.txt");
	buf[req3.size()] = '\0';
	EXPECT_STREQ(ptr, "PUT /test.txt HTTP/1.0\r\nHost: ljs.io\r\n");

	// Now a POST.

	janus::http_request req4(buf, 1000);
	ptr = req4.post("ljs.io");
	buf[req4.size()] = '\0';
	EXPECT_STREQ(ptr, "POST / HTTP/1.0\r\nHost: ljs.io\r\n");

	// Add a header.
	req4.add_header("X-Application", "abcdefg");
	buf[req4.size()] = '\0';
	EXPECT_STREQ(ptr, "POST / HTTP/1.0\r\nHost: ljs.io\r\nX-Application: abcdefg\r\n");

	// Shouldn't be able to set another op.
	ASSERT_THROW(req4.get("ljs.io"), std::runtime_error);

	char data[] = R"({"foo":3,"bar":"baz"})";
	uint64_t size = sizeof(data) - 1;
	ptr = req4.add_data(data, size);
	// The operation is complete after we add data.
	EXPECT_TRUE(req4.complete());

	// Still shouldn't be able to set a new op.
	ASSERT_THROW(req4.get("ljs.io"), std::runtime_error);

	std::string expected_buf = "POST / HTTP/1.0\r\nHost: ljs.io\r\nX-Application: abcdefg\r\n";
	expected_buf += "Content-Length: " + std::to_string(size) + "\r\n\r\n" + data;

	// Also asserts that the nul lterminator is written.
	EXPECT_STREQ(buf, expected_buf.c_str());

	janus::http_request req5(buf, 0);
	// Shouldn't be able to do anything.
	ASSERT_THROW(req5.get("ljs.io"), std::runtime_error);
	ASSERT_THROW(req5.post("ljs.io"), std::runtime_error);
	ASSERT_THROW(req5.op("PUT", "ljs.io"), std::runtime_error);
	ASSERT_THROW(req5.add_data(data, size), std::runtime_error);
}

// Test that we can correctly parse an HTTP response.
TEST(http_test, parse_response)
{
	char http[] =
		"HTTP/1.1 200 OK\r\nServer: nginx/1.14.0 (Ubuntu)\r\nDate: Sat, 25 Apr 2020 17:19:29 GMT\r\nContent-Type: text/plain\r\nContent-Length: 871\r\nLast-Modified: Sat, 25 Apr 2020 09:12:22 GMT\r\nConnection: close\r\nStrict-Transport-Security: max-age=31536000; includeSubdomains; preload\r\nAccept-Ranges: bytes\r\n\r\n012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";

	uint64_t expected_offset = std::string(http).find("\r\n\r\n") + 4;

	int response_code = 0;
	uint64_t offset = 0;

	uint64_t bytes_remaining =
		janus::parse_http_response(http, sizeof(http) - 1, response_code, offset);

	EXPECT_EQ(bytes_remaining, 0);
	EXPECT_EQ(response_code, 200);
	EXPECT_EQ(offset, expected_offset);

	// If it's truncated we should see an error.
	char tmp = http[3];
	http[3] = '\0';
	ASSERT_THROW(janus::parse_http_response(http, 3, response_code, offset),
		     std::runtime_error);
	http[3] = tmp;

	// A missing content-length should mean it's treated like there's no data.
	char http2[] =
		"HTTP/1.1 200 OK\r\nServer: nginx/1.14.0 (Ubuntu)\r\nDate: Sat, 25 Apr 2020 17:19:29 GMT\r\nContent-Type: text/plain\r\nLast-Modified: Sat, 25 Apr 2020 09:12:22 GMT\r\nConnection: close\r\nStrict-Transport-Security: max-age=31536000; includeSubdomains; preload\r\nAccept-Ranges: bytes\r\n\r\n";
	EXPECT_EQ(janus::parse_http_response(http2, sizeof(http2) - 1, response_code, offset), 0);
	EXPECT_EQ(offset, 273);
}
} // namespace
