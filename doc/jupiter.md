# Jupiter

## Core responsibilites

* Retrieving data from stream (and in a future version, from API NG when stream
  craps out), RELIABLY and EFFICIENTLY.

* Retrieving metadata for new markets, RELIABILY AND EFFICIENTLY.

* Storing retrieved JSON data (both metadata and stream) on disk without
  blocking.

* Transmitting parsed update stream to consumers EFFICIENTLY.

* Record statistics and make available to consumers.

* Maintain a log.

## Directory structure

~/data/json
|- meta/[start of recording in ms since epoch].json
|- market_stream/[start of recording in ms sinch epoch].json

## File contents

* Data may be duplicated in metadata (due to disconnect, etc. + re-retrieving metadata).

* Metadata will be aggregated in files which contain multiple instances of
  arrays due to the method being used to retrieve metadata. Neptune will need to
  be updated to handle this.
