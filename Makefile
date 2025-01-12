
CFLAGS += $(shell pkg-config --libs --cflags liburing)

.PHONY: all
all: tuntap tuntap-uring

%: %.c tuntap-helper.c
	@echo Build $(@)
	@gcc -o $(@) $(^) $(CFLAGS)

