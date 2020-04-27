#include "janus.hh"

#include "sajson.hh"
#include <cstdint>
#include <cstring>
#include <string>

#include <iostream>

namespace janus::betfair
{
// TODO(lorenzo): Would rather not make the ctor able to throw on stream
// connection, and a little iffy to utilise initialisation order like this.
stream::stream(session& session)
	: _read_offset{0},
	  _read_size{0},
	  _session{session},
	  _conn_id{""},
	  _client{session.make_stream_connection(_conn_id)},
	  _internal_buf{std::make_unique<char[]>(INTERNAL_BUFFER_SIZE)}
{
}

void stream::market_subscribe(const std::string& filter_json, const std::string& data_filter_json)
{
	// We tolerate allocations here as we don't subscribe too opften.
	std::string msg = R"({"op":"marketSubscription","id":2,"marketFilter":)";
	msg += filter_json;
	msg += R"(,"marketDataFilter":)";
	msg += data_filter_json;
	msg += "}\n";

	std::cout << msg << std::endl;

	_client.write(msg.c_str(), msg.size());

	// We might end up receiving some of the data as well as the response
	// message so we need to start placing data into our internal buffer
	// immediately.
	int size;
	char* response_json = read_next_line(size);

	sajson::document doc = janus::internal::parse_json("", response_json, size);
	const sajson::value& root = doc.get_root();

	// TODO(lorenzo): Some duplication here on reading a status response.
	// TODO(lorenzo): Check ID?

	sajson::value op = root.get_value_of_key(sajson::literal("op"));
	if (op.get_type() != sajson::TYPE_STRING)
		throw std::runtime_error("Cannot find op in market subscription response?");

	if (::strcmp(op.as_cstring(), "status") != 0)
		throw std::runtime_error(std::string("Unexpected op of ") + op.as_cstring() +
					 "expected status");

	sajson::value status_code = root.get_value_of_key(sajson::literal("statusCode"));
	if (status_code.get_type() != sajson::TYPE_STRING)
		throw std::runtime_error("Cannot find statusCode in market subscription response?");

	const char* status_str = status_code.as_cstring();
	if (::strcmp(status_str, "SUCCESS") != 0) {
		if (::strcmp(status_str, "FAILURE") != 0)
			throw std::runtime_error(std::string("Unexpected status_code of ") +
						 status_code.as_cstring() +
						 "expected SUCCESS/FAILURE");

		// Retrieve details of error so we can give a more meaningful error report.
		std::string msg = "FAILURE: ";
		sajson::value error_code = root.get_value_of_key(sajson::literal("errorCode"));
		if (error_code.get_type() == sajson::TYPE_STRING)
			msg += error_code.as_cstring();
		else
			msg += "(missing error code)";
		msg += ": ";

		sajson::value error_msg = root.get_value_of_key(sajson::literal("errorMessage"));
		if (error_msg.get_type() == sajson::TYPE_STRING)
			msg += error_msg.as_cstring();
		else
			msg += "(missing error message)";

		throw std::runtime_error(msg);
	}
}

void stream::market_subscribe(const std::vector<std::string>& market_ids,
			      const std::string& data_filter_json)
{
	uint64_t count = market_ids.size();
	if (count == 0)
		throw std::runtime_error("Attempting to subscribe to 0 markets?!");

	if (count > MAX_MARKETS)
		count = MAX_MARKETS;

	std::string stream_filter_json = R"({"marketIds":[)";
	for (uint64_t i = 0; i < count - 1; i++) {
		stream_filter_json += "\"";
		stream_filter_json += market_ids[i];
		stream_filter_json += "\",";
	}

	stream_filter_json += "\"" + market_ids[market_ids.size() - 1] + "\"]}";

	market_subscribe(stream_filter_json, data_filter_json);
}

auto stream::read_next_line(int& size) -> char*
{
	if (_read_offset >= _read_size)
		read_next_lines();

	if (_read_size == 0)
		throw std::runtime_error("Read size after read_next_lines()??");

	char* start_ptr = &_internal_buf[_read_offset];

	char* end_ptr = ::strchr(start_ptr, '\n');
	if (end_ptr == nullptr)
		throw std::runtime_error(std::string("Cannot find newline:\n") + start_ptr);

	*end_ptr = '\0';

	int delta_offset = static_cast<int>(end_ptr - start_ptr);
	_read_offset += delta_offset + 1;

	size = delta_offset;
	return start_ptr;
}

void stream::read_next_lines()
{
	_read_size = _client.read_until_newline(_internal_buf.get(), INTERNAL_BUFFER_SIZE);
	_read_offset = 0;
}
} // namespace janus::betfair
