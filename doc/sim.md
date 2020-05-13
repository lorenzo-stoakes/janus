# Sim

The purpose of this class is to simulate betting within a market. It is attached
to market state (this could be a real live market or backtested) and then
maintains a set of bets representing the current simulated position.

This is principally useful for backtesting markets, both automated strategies
and manual trading using backtest data.

## Limitations

* It pessimistically assumes that all queues will be honoured - i.e. nobody will
  cancel bets ahead of you.

* It optimistically assumes bets of ANY size will be filled if the market moves
  past them (e.g. back £100 at 6.4, best lay moves to 6.4 or above). This is a
  pretty serious limitation - you could place a stake of £1M and get it
  'matched'.

* It does not update the market state in any way, the bets are treated
  separately - absolutely no simulation whatsoever of the bets affect on the
  market.

* It uses non-virtualised unmatched volume.
