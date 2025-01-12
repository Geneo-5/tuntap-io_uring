# tuntap-io_uring

## Build

Need install liburing-dev (source on https://github.com/axboe/liburing)

```
sudo apt install liburing-dev iperf3
```

To test just run (tuntap need to be root)
```
make
sudo ./test.sh
```

## Result

```
========================= LOCAL HOST
Connecting to host 127.0.0.1, port 5201
[  5] local 127.0.0.1 port 56434 connected to 127.0.0.1 port 5201
[ ID] Interval           Transfer     Bitrate         Retr  Cwnd
[  5]   0.00-1.00   sec  3.72 GBytes  32.0 Gbits/sec    0   5.00 MBytes
[  5]   1.00-2.00   sec  3.05 GBytes  26.2 Gbits/sec    0   5.00 MBytes
[  5]   2.00-3.00   sec  3.01 GBytes  25.9 Gbits/sec    0   5.00 MBytes
[  5]   3.00-4.00   sec  3.08 GBytes  26.4 Gbits/sec    0   5.00 MBytes
[  5]   4.00-5.00   sec  3.07 GBytes  26.4 Gbits/sec    0   5.00 MBytes
[  5]   5.00-6.00   sec  3.07 GBytes  26.4 Gbits/sec    0   5.00 MBytes
[  5]   6.00-7.00   sec  3.07 GBytes  26.4 Gbits/sec    0   5.00 MBytes
[  5]   7.00-8.00   sec  3.07 GBytes  26.4 Gbits/sec    0   5.00 MBytes
[  5]   8.00-9.00   sec  3.07 GBytes  26.4 Gbits/sec    0   5.00 MBytes
[  5]   9.00-10.00  sec  3.07 GBytes  26.4 Gbits/sec    0   5.00 MBytes
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-10.00  sec  31.3 GBytes  26.9 Gbits/sec    0             sender
[  5]   0.00-10.00  sec  31.3 GBytes  26.9 Gbits/sec                  receiver

iperf Done.
========================= TunTap read/write
call: ip addr add 203.0.113.1/32 dev tun0
call: ip link set up dev tun0
call: ip addr add 203.0.113.2/32 dev tun1
call: ip link set up dev tun1
call: ip route add 203.0.113.2/32 dev tun0
call: sysctl -w net.ipv4.conf.tun0.accept_local=1
net.ipv4.conf.tun0.accept_local = 1
call: sysctl -w net.ipv4.conf.tun1.accept_local=1
net.ipv4.conf.tun1.accept_local = 1
Connecting to host 203.0.113.2, port 5201
[  5] local 203.0.113.1 port 40118 connected to 203.0.113.2 port 5201
[ ID] Interval           Transfer     Bitrate         Retr  Cwnd
[  5]   0.00-1.00   sec   102 MBytes   859 Mbits/sec   50    788 KBytes
[  5]   1.00-2.00   sec  98.8 MBytes   828 Mbits/sec    0    878 KBytes
[  5]   2.00-3.00   sec  98.8 MBytes   828 Mbits/sec    0    962 KBytes
[  5]   3.00-4.00   sec   101 MBytes   849 Mbits/sec    0   1.01 MBytes
[  5]   4.00-5.00   sec   101 MBytes   849 Mbits/sec    0   1.09 MBytes
[  5]   5.00-6.00   sec   100 MBytes   839 Mbits/sec    0   1.15 MBytes
[  5]   6.00-7.00   sec   100 MBytes   839 Mbits/sec    0   1.22 MBytes
[  5]   7.00-8.00   sec  98.8 MBytes   828 Mbits/sec    0   1.27 MBytes
[  5]   8.00-9.00   sec  97.5 MBytes   818 Mbits/sec    0   1.33 MBytes
[  5]   9.00-10.00  sec   104 MBytes   870 Mbits/sec    2   1022 KBytes
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-10.00  sec  1002 MBytes   841 Mbits/sec   52             sender
[  5]   0.00-10.01  sec  1000 MBytes   838 Mbits/sec                  receiver

iperf Done.
========================= TunTap uring
call: ip addr add 203.0.113.1/32 dev tun0
call: ip link set up dev tun0
call: ip addr add 203.0.113.2/32 dev tun1
call: ip link set up dev tun1
call: ip route add 203.0.113.2/32 dev tun0
call: sysctl -w net.ipv4.conf.tun0.accept_local=1
net.ipv4.conf.tun0.accept_local = 1
call: sysctl -w net.ipv4.conf.tun1.accept_local=1
net.ipv4.conf.tun1.accept_local = 1
Connecting to host 203.0.113.2, port 5201
[  5] local 203.0.113.1 port 39098 connected to 203.0.113.2 port 5201
[ ID] Interval           Transfer     Bitrate         Retr  Cwnd
[  5]   0.00-1.00   sec   142 MBytes  1.19 Gbits/sec   61   2.15 MBytes
[  5]   1.00-2.00   sec   151 MBytes  1.27 Gbits/sec    0   2.34 MBytes
[  5]   2.00-3.00   sec   145 MBytes  1.22 Gbits/sec    0   2.49 MBytes
[  5]   3.00-4.00   sec   146 MBytes  1.23 Gbits/sec    0   2.62 MBytes
[  5]   4.00-5.00   sec   151 MBytes  1.27 Gbits/sec    0   2.71 MBytes
[  5]   5.00-6.00   sec   151 MBytes  1.27 Gbits/sec    1   1.98 MBytes
[  5]   6.00-7.00   sec   151 MBytes  1.27 Gbits/sec    0   2.09 MBytes
[  5]   7.00-8.00   sec   148 MBytes  1.24 Gbits/sec    0   2.18 MBytes
[  5]   8.00-9.00   sec   145 MBytes  1.22 Gbits/sec    0   2.25 MBytes
[  5]   9.00-10.00  sec   142 MBytes  1.20 Gbits/sec    0   2.29 MBytes
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-10.00  sec  1.44 GBytes  1.24 Gbits/sec   62             sender
[  5]   0.00-10.01  sec  1.44 GBytes  1.23 Gbits/sec                  receiver

iperf Done.
```
