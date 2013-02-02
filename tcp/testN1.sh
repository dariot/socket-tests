#!/bin/bash

echo "Usage: ./testN1.sh <receiver_number> <num_sender> <sender1_num> <sender2_num> <receiver_port> <num_repetitions>"
echo ""

RECEIVER_e=192.168.1.$1
RECEIVER_g=192.168.2.$1

num_sender=$2

SENDER1_e=192.168.1.$3
SENDER1_g=192.168.2.$3

var=`expr $num_sender + 1`
x=${!var}
echo "sender 1: $x"

<<commento
SENDER2_e=192.168.1.$4
SENDER2_g=192.168.2.$4

PORT=$5
REP=$6

echo "Receiver: $RECEIVER_g"

for (( size = 1; size <= $REP; size = size*2 ))
do
	echo ""
	echo "===== Size: $size ====="
	echo ""
	for (( i = 0; i < 10; i++ ))
	do
		PORT=$3
		rsh $RECEIVER_e /root/nfsrw/socket/tcp/./receiver_control_bytes $num_sender $PORT $size 1 30000 | grep Band &
		sleep 0.5
		rsh $SENDER1_e /root/nfsrw/socket/tcp/./sender $RECEIVER_g $PORT $size 1 30000 1 | grep Band &
		let "PORT+=1"
		rsh $SENDER2_e /root/nfsrw/socket/tcp/./sender $RECEIVER_g $PORT $size 1 30000 1 | grep Band
		echo ""
	done
	wait
done
commento
