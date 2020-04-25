#pragma once

namespace janus
{
// Parse HTTP response, returns response code, offset into buffer where data
// begins, and how much data is remaining to be received. Note that it relies on
// the data having a Content-Length if response is 200 (OK).
//                   buf: Buffer containing HTTP to parse.
//                  size: Size of buffer.
//   (out) response_code: HTTP response code.
//          (out) offset: Offset into buffer where data begins.
//               returns: Bytes remaining to be retrieved.
auto parse_http_response(const char* buf, uint64_t size, int& response_code, uint64_t& offset)
	-> uint64_t;
} // namespace janus
