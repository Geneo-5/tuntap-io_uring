
.PHONY: all
all: tuntap tuntap-uring

tuntap: tuntap.c tuntap-helper.c
	@echo Build tuntap
	@gcc -o tuntap tuntap.c tuntap-helper.c

tuntap-uring: tuntap-uring.c tuntap-helper.c
	@echo Build tuntap-uring
	@gcc `pkg-config --libs --cflags liburing` -o tuntap-uring tuntap-uring.c tuntap-helper.c
