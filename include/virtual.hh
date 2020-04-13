#pragma once

#include "ladder.hh"
#include "price_range.hh"

#include <utility>
#include <vector>

namespace janus::betfair
{
// Calculate the virtual bets that can be generated against specific ladders by
// forming a 100% book. E.g. ATL unmatched volume of 2, and 3 can be matched
// against a virtual ATB bet at 6 (1 = 1/2 + 1/3 + 1/6).
//
// The specified ladders are the ones other than the ladder we are calculating
// virtual bets FOR. This is because virtual bets are generated AGAINST
// unmatched volume in the other ladders.
//
//     range: Price range object to use to look up prices.
//       atl: Are we examining ATL values in order to generate an ATB bet to
//            match against?
//   ladders: The ladders whose ATL unmatched volume we are calculating virtual
//            bets against. Note that we COPY the ladders as we mutate them as
//            part of the calculation.
//   returns: price, volume pairs of all virtual bets. Note that prices might
//            not be valid, and in order for them to be used in practice will
//            need to be scaled accordingly to a valid price. Volumes are always
//            positive.
auto calc_virtual_bets(const price_range& range, bool atl,
		       std::vector<janus::betfair::ladder> ladders)
	-> std::vector<std::pair<double, double>>;
} // namespace janus::betfair
