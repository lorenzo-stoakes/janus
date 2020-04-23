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
|- meta/[market id].json
|- all/[start of recording in ms sinch epoch].json
