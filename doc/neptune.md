# Neptune

## Directory structure

~/data/janbin
|- meta/[market id].jan
|- market/[market id].jan[.snap]
|- neptune.db - stores market id, last updated, last line, closed tuples

## DB file format

* flename      (string)
* last_updated (uint64)
* last_offset  (uint64)
* closed       (bool)

## Phases

1. Passive meta scan - scan JSON meta directory for new markets (i.e. doesn't
   have an existing meta binary file) - generate if so. Also add to neptune.db
   if not already present (with updated, offset = 0).

2. Passive market scan for files to update - anything with updated >
   last_updated. Create a list of files to update.

3. Scan all files from 2, updating DB.

4. Active scan - setup inotify on json directory and update accordingly.

5. Occassionally perform meta scan in case of jupiter reload.
