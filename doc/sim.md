# Sim

The purpose of this class is to simulate betting within a market. It is attached
to market state and maintains a set of bets representing overall simulated
position.

This is useful for backtesting markets as it allows for experiments with manual
and automatic strategies and comparing P/L between different approaches.

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
