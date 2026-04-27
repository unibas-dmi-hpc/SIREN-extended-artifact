#!/bin/bash
set -e
set -x

mkdir -p output
DB="output/siren.db"
rm -f "$DB"

listener/siren_listener --db "$DB" --port ":4444" &
SERVER_PID=$!
sleep 2

export LD_PRELOAD=code/siren.so
for i in 1 2 3; do demo/hello_world > /dev/null; done
for i in 1 2 3 4 5; do ls > /dev/null; done
whoami > /dev/null
for i in 1 2; do date > /dev/null; done
for i in 1 2 3 4; do uname -a > /dev/null; done
cat /etc/hostname > /dev/null
for i in 1 2 3; do python3 demo/hello.py > /dev/null; done
unset LD_PRELOAD

sleep 2
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true