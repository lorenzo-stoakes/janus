# Binary file format

## Metadata

### Header
* magic_number            (uint64)
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

#### Runner strings

Per-runner:
* runner_name             (string)
* jockey_name             (string) [optional]
* trainer_name            (string) [optional]
