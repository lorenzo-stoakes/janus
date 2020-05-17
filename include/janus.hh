#pragma once

// Have to run macro value through twice to get actual stringified version.
#define _STR(_var) #_var // NOLINT: Can't do in constexpr.
// Simple preprocessor variable stringify macro.
#define STR(_var) _STR(_var) // NOLINT: Can't do in constexpr.

#include "error.hh"

#include "channel.hh"
#include "util.hh"

#include "decimal7.hh"
#include "dynamic_array.hh"
#include "dynamic_buffer.hh"
#include "ladder.hh"
#include "parse.hh"
#include "price_range.hh"

#include "bet.hh"
#include "config.hh"
#include "json.hh"
#include "market.hh"
#include "meta.hh"
#include "runner.hh"
#include "sim.hh"
#include "stats.hh"
#include "universe.hh"
#include "update.hh"
#include "virtual.hh"

#include "network/betfair-api.hh"
#include "network/betfair-session.hh"
#include "network/betfair-stream.hh"
#include "network/certs.hh"
#include "network/client.hh"
#include "network/http_request.hh"
#include "network/http_response.hh"
#include "network/rng.hh"

#include "db.hh"
