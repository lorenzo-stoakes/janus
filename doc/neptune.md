# Neptune

## Directory structure

~/data/janbin
|- meta/[market id].jan
|- market/[market id].jan[.snap]
|- neptune.db (see below)

## DB file format

Per file:
* flename (ms since epoch) (uint64) - jupiter outputs [ms since epoch].json meta/stream
* last_updated             (uint64)
* last_offset              (uint64)
(closed markets get deleted)

## Phases

1. Passive meta scan - scan JSON meta directory for new files (i.e. don't exist
   in the db) - generate if so and update db with offset = 0.

2. Passive market scan for files to update - anything with updated >
   last_updated. Create a list of files to update.

3. Scan all files from 2, updating DB.

4. Active scan - setup inotify on json directory and update accordingly.

5. Occassionally perform meta scan in case of jupiter reload.
