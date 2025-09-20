#!/bin/sh -e

clean() {
	killall -q -w iperf3 tuntap tuntap-uring veth || true
}

trap "clean" EXIT TERM
clean

args="-i 1 -t 10 -N -b 20G -u -l 40"

# echo ========================= LOCAL HOST
# iperf3 -i 0 -s -1 -D > /dev/null
# iperf3 -c 127.0.0.1 ${args}
echo ========================= TunTap read/write
./tuntap > /dev/null
iperf3 -i 0 -s -1 -D --bind-dev tun1 > /dev/null 2>&1
iperf3 -c 203.0.113.2%tun0 ${args}
# grep -H . /sys/class/net/tun*/statistics/*
killall -w tuntap
# echo ========================= TunTap read/write NAPI
# ./tuntap NAPI > /dev/null
# iperf3 -i 0 -s -1 -D --bind-dev tun1 > /dev/null 2>&1
# iperf3 -c 203.0.113.2%tun0 ${args}
# grep -H . /sys/class/net/tun*/statistics/*
# killall -w tuntap
# echo ========================= TunTap uring
# ./tuntap-uring > /dev/null
# iperf3 -i 0 -s -1 -D --bind-dev tun1 > /dev/null 2>&1
# iperf3 -c 203.0.113.2%tun0 ${args}
# grep -H . /sys/class/net/tun*/statistics/*
# killall -w tuntap-uring
# echo ========================= TunTap uring NAPI
# ./tuntap-uring NAPI > /dev/null
# iperf3 -i 0 -s -1 -D --bind-dev tun1 > /dev/null 2>&1
# iperf3 -c 203.0.113.2%tun0 ${args}
# grep -H . /sys/class/net/tun*/statistics/*
# killall -w tuntap-uring
echo ========================= veth
./veth > /dev/null
ip netns exec ns1 iperf3 -i 0 -1 -s -D > /dev/null 2>&1
ip netns exec ns0 iperf3 -c 203.0.113.2 ${args}
killall -w veth
