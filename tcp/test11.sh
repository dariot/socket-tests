#!/bin/bash

echo "Usage: ./test1.sh <sender_number> <receiver_number> <receiver_port> <num_repetitions>"
echo ""

SENDER_e=192.168.1.$1
SENDER_g=192.168.2.$1

RECEIVER_e=192.168.1.$2
RECEIVER_g=192.168.2.$2

PORT=$3
REP=$4

echo "Sender: $SENDER_g"
echo "Receiver: $RECEIVER_g"

for (( size = 1; size <= $REP; size = size*2 ))
do
	echo ""
	echo "===== Size: $size bytes ====="
	echo ""
	for (( i = 0; i < 10; i++ ))
	do
		rsh $RECEIVER_e /root/nfsrw/socket/tcp/./receiver_control_bytes 1 $PORT $size 1 30000 | grep Band &
		sleep 0.5
		rsh $SENDER_e /root/nfsrw/socket/tcp/./sender $RECEIVER_g $PORT $size 1 30000 1 | grep Band
		echo ""
	done
	wait
done
