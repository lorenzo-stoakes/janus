# Binary file format

## Metadata

### Header

* market_id               (uint64)
* event_type_id           (uint64)
* event_id                (uint64)
* competition_id          (uint64) [optional]
* market_start_timestamp  (uint64)
* num_runners             (uint64)

### Strings
* market_name             (string)
* event_type_name         (string)
* event_name              (string)
* event_country_code      (string)
* event_timezone          (string)
* market_type_name        (string)
* venue_name              (string) [optional]
* competition_name        (string) [optional]

### Runners

Per-runner:
* runner_id               (uint64)
* sort_priority           (uint64)
* runner_name             (string)
* jockey_name             (string) [optional]
* trainer_name            (string) [optional]

## Updates

Stored data will be compressed with [snappy](https://github.com/google/snappy)
if the market has been closed. Otherwise won't be.

Simple array of updates, use file size to determine number of updates.

### Stream Data

Per update:
* update struct           (16 bytes)

## Stats

Stores statistical data about the market useful for analysis and classification.

* flags - HAVE_METADATA, PAST_POST, WENT_INPLAY, WAS_CLOSED, SAW_SP, SAW_WINNER.  (uint64)
* num_updates                                                                     (uint64)
* num_runners - Duplicated from metadata but useful in stats                      (uint64)
* num_removals                                                                    (uint64)
* first_timestamp                                                                 (uint64)
* start_timestamp - Duplicated from metadata but useful in stats                  (uint64)
* inplay_timestamp                                                                (uint64)
* last_timestamp                                                                  (uint64)
* winner_runner_id                                                                (uint64)
* winner_sp                                                                       (double)
[{[60, 30, 10, 5, 3, 1] mins before [post, inplay]}, during inplay] :             13*
* num_updates                                                                     (uint64)
* median_update_interval_ms                                                       (uint64)
* mean_update_interval_ms                                                         (uint64)
* worst_update_interval_ms                                                        (uint64)
