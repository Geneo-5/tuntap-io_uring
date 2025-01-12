
.PHONY: all
all: tuntap

tuntap: tuntap.c
	gcc -o tuntap tuntap.c


.PHONY: test
test: tuntap
	sudo ./tuntap

