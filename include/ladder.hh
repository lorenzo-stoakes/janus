#pragma once

#include <array>
#include <cstdint>

#include "error.hh"

namespace janus::betfair
{
// Per-runner ladder containing data relating to prices on the ladder.
class ladder
{
public:
	ladder()
		: _min_atl_index{NUM_PRICES - 1},
		  _max_atb_index{0},
		  _total_unmatched_atl{0},
		  _total_unmatched_atb{0},
		  _unmatched{0}
	{
	}

	// Returns the unmatched volume at the specified price index, positive
	// indicates back (ATL), negative lay (ATB).
	auto unmatched(uint64_t i) const -> double
	{
		return _unmatched[i];
	}

	// For convenience export the [] operator as unmatched volume at that
	// price index.
	auto operator[](uint64_t i) const -> double
	{
		return unmatched(i);
	}

	// Returns the price index of the minimum back (ATL) price, i.e. the
	// current best unmatched price to LAY (but the counterparty is
	// BACKing).
	auto min_atl_index() const -> uint64_t
	{
		return _min_atl_index;
	}

	// Returns the price index of the maximum lay (ATB) price, i.e. the
	// current best unmatched price to BACK (but the counterparty is
	// LAYing).
	auto max_atb_index() const -> uint64_t
	{
		return _max_atb_index;
	}

	// Returns the sum of unmatched back (ATL) volume for the entire ladder.
	auto total_unmatched_atl() const -> double
	{
		return _total_unmatched_atl;
	}

	// Returns the sum of unmatched lay (ATB) volume for the entire ladder.
	auto total_unmatched_atb() const -> double
	{
		return _total_unmatched_atb;
	}

	// Set the unmatched volume at the specified price index to the
	// specified volume. If a positive volume is specified, then the volume
	// is on the back (ATL) side, otherwise if negative then volume is on
	// the lay (ATB) side.
	// This additionally updates the top ATL/ATB indexes.
	void set_unmatched_at(uint64_t i, double vol)
	{
		check_valid_unmatched(i, vol);
		update_total_unmatched(i, vol);
		_unmatched[i] = vol;
		update_limit_indexes(i, vol);
	}

	// Clear the unmatched volume at the specified price index.
	// This additionally updates the top ATL/ATB indexes.
	void clear_unmatched_at(uint64_t i)
	{
		update_total_unmatched(i, 0);
		_unmatched[i] = 0;
		update_limit_indexes_clear(i);
	}

	// Clear state of ladder.
	void clear()
	{
		_min_atl_index = NUM_PRICES - 1;
		_max_atb_index = 0;
		_total_unmatched_atl = 0;
		_total_unmatched_atb = 0;

		_unmatched = {0};
	}

private:
	uint64_t _min_atl_index;
	uint64_t _max_atb_index;
	double _total_unmatched_atl;
	double _total_unmatched_atb;

	// Negative lay (atb), positive back (atl).
	std::array<double, NUM_PRICES> _unmatched;

	// Determine if the specified (index, vol) pair proposed to be added to
	// unmatched inventory is valid, if not throw.
	void check_valid_unmatched(uint64_t i, double vol)
	{
		// We check whether the proposed unmatched pair would cause a
		// discontinuity in the unmatched price range, e.g. max ATB 2,
		// min ATL 2.02, add an ATB at 3.

		// Note that we specifically DON'T check for == limit index as
		// in this instance, we REPLACE the previous index and this is
		// valid.

		// Note also that we don't handle a clear (e.g. vol == 0), this
		// is because a clear is not capable of creating a
		// discontinuity.

		if (vol < 0 && i > _min_atl_index)
			throw invalid_unmatched_update(i, vol, _min_atl_index);
		if (vol > 0 && i < _max_atb_index)
			throw invalid_unmatched_update(i, vol, _max_atb_index);
	}

	static constexpr uint64_t NOT_FOUND_INDEX = static_cast<uint64_t>(-1);

	// Starting from the specified price index, find the first price where
	// we have back (ATL) available and return its price index.
	auto get_next_atl_index(uint64_t start_index) const -> uint64_t
	{
		for (uint64_t i = start_index; i < NUM_PRICES; i++) {
			if (_unmatched[i] > 0)
				return i;
		}

		return NOT_FOUND_INDEX;
	}

	// Starting from the specified price index, find the first price where
	// we have lay (ATB) available and return its price index.
	auto get_next_atb_index(uint64_t start_index) const -> uint64_t
	{
		for (auto i = static_cast<int64_t>(start_index); i >= 0; i--) {
			if (_unmatched[i] < 0)
				return i;
		}

		return NOT_FOUND_INDEX;
	}

	// Starting from the specified price index, find the first price where
	// we have back (ATL) available and set _min_atl_index to this price
	// index.
	void update_min_atl_index(uint64_t start_index)
	{
		uint64_t next_index = get_next_atl_index(start_index);
		if (next_index == NOT_FOUND_INDEX) {
			// If we reach here then everything's 0 our minimum ATL index is
			// the maximum price.
			_min_atl_index = NUM_PRICES - 1;
		} else {
			_min_atl_index = next_index;
		}
	}

	// Starting from the specified price index, find the first price where
	// we have lay (ATB) available and set _max_atb_index to this price
	// index.
	void update_max_atb_index(uint64_t start_index)
	{
		uint64_t next_index = get_next_atb_index(start_index);
		if (next_index == NOT_FOUND_INDEX) {
			// If we reach here then everything's 0 our maximum ATB index is
			// the minimum price.
			_max_atb_index = 0;
		} else {
			_max_atb_index = next_index;
		}
	}

	// Update min/max ATL/ATB indexes when we are _clearing_ unmatched
	// volume at a specific price index.
	void update_limit_indexes_clear(uint64_t i)
	{
		// We only need to update if we are clearing precisely the min
		// ATL/max ATB indexes as otherwise it has no bearing on these.
		if (i == _min_atl_index)
			update_min_atl_index(i);
		else if (i == _max_atb_index)
			update_max_atb_index(i);
	}

	// Update min/max ATL/ATB indexes.
	void update_limit_indexes(uint64_t i, double vol)
	{
		// We assume we've already checked that the addition to
		// unmatched is valid, i.e. we are not trying to put an
		// arbitrary back or lay in the middle of existing entries of
		// the opposite side.

		// Note that we're not attempting to find if -epsilon < vol <
		// epsilon as when clearing volume we will be strictly setting
		// to zero.
		if (vol == 0) {
			update_limit_indexes_clear(i);
		} else if (vol < 0 && i > _max_atb_index) {
			_max_atb_index = i;

			if (i == _min_atl_index) {
				// We are REPLACING the min ATL index with our
				// max ATB one. We therefore need to determine
				// the new min ATL index.
				update_min_atl_index(i);
			}
		} else if (vol > 0 && i < _min_atl_index) {
			_min_atl_index = i;

			if (i == _max_atb_index) {
				// We are REPLACING the max ATB index with our
				// min ATL one. We therefore need to determine
				// the new min ATL index.
				update_max_atb_index(i);
			}
		}
	}

	// Update our accruing total unmatched statistics.
	void update_total_unmatched(uint64_t i, double vol)
	{
		// Remove previous volume.
		double prev = _unmatched[i];
		if (prev < 0)
			_total_unmatched_atb += prev; // Negative.
		else
			_total_unmatched_atl -= prev;

		// We have to update again for new volume as we may be replacing
		// the old.

		if (vol < 0)
			_total_unmatched_atb -= vol; // Negative.
		else
			_total_unmatched_atl += vol;
	}
};
} // namespace janus::betfair
