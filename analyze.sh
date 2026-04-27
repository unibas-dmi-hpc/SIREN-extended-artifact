#!/bin/bash
set -e
set -x

DB="output/siren.db"
CAPTURE="output/captures.txt"

sqlite3 -batch -noheader "$DB" \
  "SELECT 'JOBIDRAW='||jobidraw
       ||'|STEPID='   ||stepid
       ||'|PID='      ||pid
       ||'|HASH='     ||replace(replace(hash,    char(13), ' '), char(10), ' ')
       ||'|HOST='     ||replace(replace(host,    char(13), ' '), char(10), ' ')
       ||'|TIME='     ||time
       ||'|LAYER='    ||replace(replace(layer,   char(13), ' '), char(10), ' ')
       ||'|TYPE='     ||replace(replace(info,    char(13), ' '), char(10), ' ')
       ||'|CONTENT='  ||replace(replace(content, char(13), ' '), char(10), ' ')
   FROM fingerprints;" \
  > "$CAPTURE"

cd analysis
python3 siren_prep.py
python3 siren_stats.py > ../output/statistics.txt
