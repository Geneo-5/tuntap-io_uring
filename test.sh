#!/bin/sh -e

builddir="$1"

args="-i 1 -t 60"

clean() {
	killall -q -w iperf3 tuntap tuntap-uring veth vhost-net || true
	ip netns del ns0 2> /dev/null || true
	ip netns del ns1 2> /dev/null || true
}

run_wireshark() {
	local ns="$1"

	ip netns exec ns${ns} wireshark -i tun${ns} -k > /dev/null 2>&1 &
}

setup_netns() {
	local ns="$1"
	ip netns add ns${ns}
	ip netns exec ns${ns} sysctl -w net.ipv6.conf.default.disable_ipv6=1 > /dev/null
}

run_test() {
	local bin="$1"
	local opt="$2"

	if [ -n "${bin}" ]; then
		${builddir}/${bin} ${opt} 
		#> /dev/null
	fi

	#run_wireshark 0
	#run_wireshark 1
	#sleep 5

	ip netns exec ns1 iperf3 -i 0 -s -1 -D > /dev/null 2>&1
	ip netns exec ns0 iperf3 -c 203.0.113.2 ${args}

	if [ -n "${bin}" ]; then
		killall -w ${bin}
	fi
}

trap "clean" EXIT TERM
clean
setup_netns 0
setup_netns 1

#echo ========================= TunTap read/write
#run_test tuntap
#echo ========================= TunTap read/write NAPI
#run_test tuntap NAPI
#echo ========================= TunTap uring
#run_test tuntap-uring
#echo ========================= TunTap uring NAPI
#run_test tuntap-uring NAPI
#echo ========================= veth recevmmsg/sendmmsg
#run_test veth
#echo ========================= veth natif tx off
#ip link add tun0 netns ns0 type veth peer name tun1 netns ns1
#ip -n ns0 addr add 203.0.113.1/24 broadcast 203.0.113.255 dev tun0
#ip netns exec ns0 ethtool --offload tun0 tx off
#ip -n ns0 link set dev tun0 up
#ip -n ns1 addr add 203.0.113.2/24 broadcast 203.0.113.255 dev tun1
#ip netns exec ns1 ethtool --offload tun1 tx off
#ip -n ns1 link set dev tun1 up
#run_test
#ip -n ns0 link del tun0
echo ========================= vhost-net
run_test vhost-net
