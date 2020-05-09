# Neptune

## Directory structure

~/data/janbin
|- meta/[market id].jan
|- market/[market id].jan[.snap]
|- stats/[market id].jan
|- neptune.db (see below)

## DB file format

Per file:
* flename (ms since epoch) (uint64) - jupiter outputs [ms since epoch].json meta/stream
* last_updated             (uint64)
* next_offset              (uint64)
* next_line                (uint64)
(closed markets get deleted)

## Steps

1. Passive meta scan - scan JSON meta directory for new files (i.e. don't exist
   in the db) - generate if so and update db with offset = 0.

2. Passive market scan for files to update - anything with updated >
   last_updated and size > next_offset. Create a list of files to update.

3. Scan all files from 2, updating DB.

4. Write binary stream data and stats, appending/refreshing where necessary.

5. Sleep and loop.
