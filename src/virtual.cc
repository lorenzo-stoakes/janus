#include "janus.hh"

#include <cstdint>
#include <utility>
#include <vector>

namespace janus::betfair
{
// TODO(lorenzo): This function is pretty unperformant. Needs some serious optimisation.
auto calc_virtual_bets(const price_range& range, bool atl,
		       std::vector<janus::betfair::ladder> ladders)
	-> std::vector<std::pair<double, double>>
{
	// Declare a custom sorter type in order to be able to sort ladders by
	// best available ATL/ATB price.
	struct ladder_sorter
	{
		explicit ladder_sorter(bool atl) : _atl{atl} {}

		auto operator()(janus::betfair::ladder& ladder_a, janus::betfair::ladder& ladder_b)
			-> bool
		{
			uint64_t price_index_a;
			uint64_t price_index_b;
			if (_atl) {
				price_index_a = ladder_a.best_atl().first;
				price_index_b = ladder_b.best_atl().first;
			} else {
				price_index_a = ladder_a.best_atb().first;
				price_index_b = ladder_b.best_atb().first;
			}

			return price_index_a <= price_index_b;
		}

	private:
		bool _atl;
	} sorter(atl);

	std::vector<std::pair<double, double>> ret;
	// Track price, volume pairs for each ladder.
	std::vector<std::pair<double, double>> pairs(ladders.size());

	// This process continues until we have no further virtual prices we can
	// generate.
	for (uint64_t count = 0; count < MAX_VIRTUAL_LOOPS; count++) {
		// We will scale all volumes matched against a virtual price
		// against the lowest odds, so sort accordingly.
		std::sort(ladders.begin(), ladders.end(), sorter);

		// This variable tracks the virtual price as it is generated,
		// according to virt_price = 1/(1 - 1/price_1 - 1/price_2 - ...)
		// In order to track this we calculate the 1 - ... portion
		// first, so start with 1.
		double virt_price = 1;
		// If volumes of other ladders are insufficient to allow full
		// use of volume of the lowest priced ladder, then we need to
		// apply a multplier to take into account what proportion of the
		// first ladder's volume we can use.
		double mult = 1;

		for (uint64_t i = 0; i < ladders.size(); i++) {
			auto& ladder = ladders[i];

			uint64_t price_index;
			double vol;

			if (atl)
				std::tie(price_index, vol) = ladder.best_atl();
			else
				std::tie(price_index, vol) = ladder.best_atb();

			double price = janus::betfair::price_range::index_to_price(price_index);

			pairs[i] = std::make_pair(price, vol);
			virt_price -= (1. / price);

			// We try to take 100% of the lowest price, but we might
			// not have enough volume in other ladders to cover
			// this, so check.
			if (i > 0) {
				double scaled_vol = pairs[0].second * (pairs[0].first / price);
				// Take into acount existing multiplier.
				if (scaled_vol * mult > vol)
					mult = vol / scaled_vol;
			}
		}
		virt_price = 1. / virt_price;
		if (virt_price < 1.01 || virt_price > 1000) // NOLINT: Not magical.
			return ret;

		for (uint64_t i = 0; i < ladders.size(); i++) {
			auto& ladder = ladders[i];
			double price = pairs[i].first;
			double vol = pairs[i].second;

			double scaled_vol = mult * pairs[0].second * (pairs[0].first / price);

			double new_vol = vol - scaled_vol;
			if (!atl)
				new_vol *= -1;

			// Note that we have COPIED the ladders so this mutation
			// will not affect the caller.
			// TODO(lorenzo): Actually store the price index not the price so
			// we don't have to convert to and back again.
			ladder.set_unmatched_at(range.price_to_nearest_index(price), new_vol);
		}

		double virt_vol = pairs[0].second * mult * (pairs[0].first / virt_price);
		// If the volume is negligible then no need to generate the bet.
		if (virt_vol > 1e-6) // NOLINT: Not magical.
			ret.emplace_back(std::make_pair(virt_price, virt_vol));
		// If the volume is _exactly_ 0 then we have nothing more to
		// calculate and we're done.
		else if (virt_vol == 0)
			return ret;
	}

	return ret;
}

auto gen_virt_ladder(const janus::betfair::price_range& range, janus::betfair::ladder ladder,
		     const std::vector<janus::betfair::ladder>& other_ladders)
	-> janus::betfair::ladder
{
	// ATB virtual bets are calculated using the ATL portion of the other ladders.
	auto atb_virt_bets = calc_virtual_bets(range, true, other_ladders);
	// ATL virtual bets are calculated using the ATB portion of the other ladders.
	auto atl_virt_bets = calc_virtual_bets(range, false, other_ladders);

	// Merge generated ATL virtual bets.
	for (auto [price, vol] : atl_virt_bets) {
		// Round UP to the nearest price index as this is equivalent of a BACK bet.
		uint64_t price_index = range.price_to_nearest_index_up(price);
		double nearest_price = janus::betfair::price_range::index_to_price(price_index);

		// Take into account fact our calculated prices are often not at
		// a price boundary, so we need to scale accordingly.
		vol *= (price / nearest_price);

		// For volumes < 1 we assign it to the next price.
		if (vol < 1 && price_index < janus::betfair::NUM_PRICES - 1)
			price_index++;

		ladder.set_unmatched_at(price_index, ladder[price_index] + vol);
	}

	// Merge generated ATB virtual bets.
	for (auto [price, vol] : atb_virt_bets) {
		// Round DOWN to the nearest price index as this is equivalent of a LAY bet.
		uint64_t price_index = range.price_to_nearest_index(price);
		double nearest_price = janus::betfair::price_range::index_to_price(price_index);

		// Take into account fact our calculated prices are often not at
		// a price boundary, so we need to scale accordingly.
		vol *= (price / nearest_price);

		// For volumes < 1 we assign it to the next price.
		if (vol < 1 && price_index > 0)
			price_index--;

		// ATB so negative.
		ladder.set_unmatched_at(price_index, ladder[price_index] - vol);
	}

	return ladder;
}
} // namespace janus::betfair
