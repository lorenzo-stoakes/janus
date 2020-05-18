#!/bin/bash

for id in 170462852 170462855 170462859 170462863 170462866 170462892 170462893 170462894 170462895; do
	cp ~/data/janbin/market/${id}.jan.snap market/
	cp ~/data/janbin/meta/${id}.jan meta/
	cp ~/data/janbin/stats/${id}.jan stats/
done
