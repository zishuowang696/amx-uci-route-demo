CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -fPIC
UCI_CFLAGS ?=
UCI_LDFLAGS ?= -luci

all: uci-route-demo.so

uci-route-demo.so: src/uci_route.c
	$(CC) $(CFLAGS) -shared -o $@ $< \
		$(UCI_CFLAGS) $(UCI_LDFLAGS) \
		-lamxd -lamxo -lamxc -lamxp

clean:
	rm -f uci-route-demo.so
