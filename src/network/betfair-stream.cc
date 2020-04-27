#include "janus.hh"

namespace janus::betfair
{
// TODO(lorenzo): Would rather not make the ctor able to throw on stream
// connection, and a little iffy to utilise initialisation order like this.
stream::stream(session& session)
	: _session{session}, _conn_id{""}, _client{session.make_stream_connection(_conn_id)}
{
}
} // namespace janus::betfair
