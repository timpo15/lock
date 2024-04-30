#!/bin/bash
PROCESS_COUNT=10

for ((i=1;i<=$PROCESS_COUNT;i++));
do
    ./mainn myfile &
    pids[${i}]=$!
    echo $!
done

sleep 300

killall -s SIGINT mainn

for pid in ${pids[*]}; do
    echo $pid
    wait $pid
done
