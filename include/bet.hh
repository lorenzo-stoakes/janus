#pragma once

namespace janus
{
// Represents the state of a bet.
enum class bet_flags : uint64_t
{
	DEFAULT = 0,
	SIM = 1 << 0,       // Simulated bet.
	ACKED = 1 << 1,     // Acked by exchange (and hence has bet ID).
	REDUCED = 1 << 2,   // Price reduced due to runner removal.
	CANCELLED = 1 << 3, // Bet was at least partially cancelled.
	VOIDED = 1 << 4,    // Bet was voided.
};

// Permit flags to use the | operator.
static inline auto operator|(bet_flags a, bet_flags b) -> bet_flags
{
	uint64_t ret = static_cast<uint64_t>(a) | static_cast<uint64_t>(b);
	return static_cast<bet_flags>(ret);
}

// Permit flags to use the |= operator.
static inline auto operator|=(bet_flags& a, bet_flags b) -> bet_flags&
{
	a = a | b;
	return a;
}

// Permit flags to use the & operator.
static inline auto operator&(bet_flags a, bet_flags b) -> bet_flags
{
	uint64_t ret = static_cast<uint64_t>(a) & static_cast<uint64_t>(b);
	return static_cast<bet_flags>(ret);
}

// Represents a bet throughout its lifetime
class bet
{
public:
	bet(uint64_t runner_id, double price, double stake, bool is_back, bool sim = false)
		: _flags{sim ? bet_flags::SIM : bet_flags::DEFAULT},
		  _runner_id{runner_id},
		  _price{price},
		  _orig_price{price},
		  _stake{stake},
		  _orig_stake{stake},
		  _is_back{is_back},
		  _matched{0},
		  _bet_id{0},
		  _target_matched{0}
	{
	}

	// Retrieve flags associated with bet.
	auto flags() const -> bet_flags
	{
		return _flags;
	}

	// Retrieve runner ID.
	auto runner_id() const -> uint64_t
	{
		return _runner_id;
	}

	// Retrieve bet price. We use a double here since removals can scale
	// prices to non-standard levels.
	auto price() const -> double
	{
		return _price;
	}

	// Retrieves the original bet price.
	auto orig_price() const -> double
	{
		return _orig_price;
	}

	// Retrieve stake (may be reduced if bet is cancelled/partially
	// cancelled).
	auto stake() const -> double
	{
		return _stake;
	}

	// Retrieve original bet stake.
	auto orig_stake() const -> double
	{
		return _orig_stake;
	}

	// Determine if the bet was a back or a lay.
	auto is_back() const -> bool
	{
		return _is_back;
	}

	// Determine the matched quantity of the bet.
	auto matched() const -> double
	{
		return _matched;
	}

	// Retrieve exchange-assigned bet ID.
	auto bet_id() const -> uint64_t
	{
		return _bet_id;
	}

	// Determine the unmatched quantity of the bet.
	auto unmatched() const -> double
	{
		return _stake - _matched;
	}

	// Determine the target matched volume. Only relevat to simulated bets.
	auto target_matched() const -> double
	{
		return _target_matched;
	}

	// Determine if there is no remaining unmatched - either the original
	// stake has been fully matched or the remaining unmatched has been
	// cancelled or the bet is voided.
	auto is_complete() const -> bool
	{
		double remaining = unmatched();
		return dz(remaining);
	}

	// Determine if the whole of the bet was cancelled.
	auto is_cancelled() const -> bool
	{
		return dz(_stake);
	}

	// Return profit/loss matched portion of this bet would result in.
	auto pl(bool won) -> double
	{
		if (_is_back) {
			if (won)
				return _matched * (_price - 1);
			else
				return -_matched;
		} else {
			if (won)
				return -_matched * (_price - 1);
			else
				return _matched;
		}
	}

	// Assign exchange-assigned bet ID.
	void set_bet_id(uint64_t id)
	{
		if ((_flags & bet_flags::VOIDED) == bet_flags::VOIDED)
			return;

		_bet_id = id;
		_flags |= bet_flags::ACKED;
	}

	// Mark a portion of the bet matched.
	void match(double amount)
	{
		if ((_flags & bet_flags::VOIDED) == bet_flags::VOIDED)
			return;

		if (amount > _stake)
			_matched = _stake;
		else
			_matched = amount;
	}

	// Match the bet, increasing stake if matched exceeds stake.
	void match_unsafe(double amount)
	{
		_matched = amount;
		if (amount > _stake)
			_stake = amount;
	}

	// Cancel the unmatched portion of the bet.
	void cancel()
	{
		// If the bet is complete (fully matched/voided/cancelled)
		// there's nothing to cancel.
		if (is_complete())
			return;

		_stake = _matched;
		_flags |= bet_flags::CANCELLED;
	}

	// Apply an adjustment factor to the bet price.
	// https://en-betfair.custhelp.com/app/answers/detail/a_id/408/~/exchange%3A-in-a-horse-race%2C-how-will-non-runners-be-treated%3F
	// Returns true if bet needs to be split out and sets split_stake in this case.
	auto apply_adj_factor(double adj_factor, double& split_stake) -> bool
	{
		// See rule 14.6 at
		// https://www.betfair.com/aboutUs/Rules.and.Regulations/#rulespartb

		if ((_flags & bet_flags::VOIDED) == bet_flags::VOIDED)
			return false;

		// In winner markets reductions < 2.5% are ignored.
		if (adj_factor < 2.5)
			return false;

		// Unmatched lays are cancelled, backs are kept and need to be
		// split out.
		if (_is_back)
			split_stake = unmatched();
		// Cancel unmatched for this bet either way.
		_stake = _matched;

		// If we have no matched component we don't need to do anything
		// else.
		if (dz(_matched))
			return false;

		// Adjustment factor is expressed as a percentage.
		double mult = adj_factor / 100.;
		// We reduce by the percentage.
		mult = 1. - mult;
		// In winner markets (we assume winner markets here instead of
		// place), the reduction factor is applied to the price overall
		// INCLUDING stake.
		_price *= mult;
		_flags |= bet_flags::REDUCED;

		// If fully matched then no need to split out either.
		return _is_back && !dz(split_stake);
	}

	// Void bet completely.
	void void_bet()
	{
		_stake = 0;
		_matched = 0;
		_flags |= bet_flags::VOIDED;
	}

	// Set target matched volume. Only relevat to simulated bets.
	void set_target_matched(double target)
	{
		_target_matched = target;
	}

	// Set bet price. This is used in the case where the best market price
	// is better than the requested one.
	void set_price(double price)
	{
		_price = price;
	}

private:
	bet_flags _flags;
	uint64_t _runner_id;
	double _price;
	double _orig_price;
	double _stake;
	double _orig_stake;
	bool _is_back;
	double _matched;
	uint64_t _bet_id;
	double _target_matched;
};
} // namespace janus
