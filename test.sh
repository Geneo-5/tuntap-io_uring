#!/bin/sh -e

trap "killall -q tuntap tuntap-uring" EXIT TERM
killall -q -w iperf3 tuntap tuntap-uring || true

echo ========================= LOCAL HOST
iperf3 -s -1 -D > /dev/null
iperf3 -c 127.0.0.1 -Z
echo ========================= TunTap read/write
./tuntap
iperf3 -s -1 -D --bind-dev tun1 > /dev/null 2>&1
iperf3 -c 203.0.113.2%tun0 -Z
killall -w tuntap
echo ========================= TunTap uring
./tuntap-uring
iperf3 -s -1 -D --bind-dev tun1 > /dev/null 2>&1
iperf3 -c 203.0.113.2%tun0 -Z
killall -w tuntap-uring
