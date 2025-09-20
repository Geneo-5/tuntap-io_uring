
CFLAGS += $(shell pkg-config --libs --cflags liburing) -lc

.PHONY: all
all: tuntap tuntap-uring veth

%: %.c tuntap-helper.c
	@echo Build $(@)
	@gcc -o $(@) $(^) $(CFLAGS)

