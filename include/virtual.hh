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

// Generate a virtual ladder which contains all virtual bets merged with
// non-virtual, allocated to standard prices in correct proportion. This should
// generate a ladder exactly equivalent to that which would be observed when
// using betfair's website or a trading app.
//          ladder: Ladder we are merging prices with.
//   other_ladders: Ladders of other runners we wish to generate virtual bets from.
//           range: Price range object to use to look up prices.
//         returns: Ladder with all virtual and non-virtual unmatched volume merged.
auto gen_virt_ladder(const janus::betfair::price_range& range, janus::betfair::ladder ladder,
		     const std::vector<janus::betfair::ladder>& other_ladders)
	-> janus::betfair::ladder;
} // namespace janus::betfair
