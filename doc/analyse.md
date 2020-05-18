# Analyse

The analyse module seeks to provide a generic means to perform analysis over
historic data in an efficient manner across parallel cores.

Users must provide:

* Filter predicate - `meta_view&, stats& -> bool` - Determines whether a market
  should be analysed or not. This allows the caller to determine what they want
  to analyse, for example all race horse markets which have a winner and minimal
  update intervals.

* Update worker - `int, market&, sim&, TWorkerState& -> bool` - This performs
  the core of the work - invoked after each time the market is updated. If it
  returns false then abort this market.

* Market reducer - `int, TWorkerState&, sim&, bool, TMarketAggState& -> bool` -
  Invoked after each market iteration on this node. Returns false to abort the
  run. Passed a bool to indicate whether the worker aborted. Builds up the
  aggregate type.

* Node reducer - `int, TMarketAggState&, bool, TNodeAggState& -> bool` - Invoked
  after each run across the set of markets allocated to this node. Passed bool
  which indicates whether set of markets were aborted. Returns false to
  complete/abort the run.

* Reducer - `Collection<TNodeAggState>& - > TResult` - Invoked after all nodes
  have completed their work and reduces the aggregated results from each into a
  final result.

## Node Pseudocode

```
while (true) {
    bool runner_aborted = false;
    for (market : markets) {f
        bool worker_aborted = false;
        while (market updating) {
            if (!update_worker(market, sim, state)) {
                worker_aborted = true;
                break;
            }

            ... update market ...
        }

        if (!market_reducer(state, agg_state, worker_aborted)) {
            market_reducer_aborted = true;
            break;
        }
    }
    if (!node_reducer(agg_state, runner_aborted, node_agg_state))
        break;
}
```
