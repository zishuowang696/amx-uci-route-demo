# amx-uci-route-demo

A **minimal, runnable** AMX data-model → UCI mapping example.

It defines an `UciRoute` object, listens for `dm:object-changed`, reads its
parameters, and writes a `config route` section through libuci.

This is a teaching/demo module, **not** the official `mod-routing-uci` and not
a production network manager.

## How it fits

```text
UciRoute.{i} (AMX data model)
  → dm:object-changed
  → route_changed()
  → libuci set/commit
  → /etc/config/network
  → netifd applies
```

## Requirements

- C compiler, `make`
- AMX headers/libraries: `libamxd`, `libamxo`, `libamxc`, `libamxp`
- libuci headers/library
- `ubusd`, `ubus`, `amxrt` for the runtime smoke test
- Linux/OpenWrt

The Makefile takes paths through variables:

```sh
make \
  UCI_CFLAGS="-I/path/to/uci" \
  UCI_LDFLAGS="-L/path/to/uci/build -luci"
```

If AMX is installed in a non-standard prefix, add its include/library paths to
`CFLAGS` / `LDFLAGS` as well.

## Files

- `src/uci_route.c` — event handler + libuci mapping
- `odl/uci-route.odl` — object definition, entry point and event subscription
- `tests/smoke.sh` — end-to-end smoke test with a temporary UCI dir

The event subscription must be scoped to the object:

```odl
on event "dm:object-changed" of "UciRoute." call route_changed;
```

The `of "UciRoute."` is required; a global subscription is not enough for this
demo's instance changes.

## Try it

Start an isolated bus and AMX runtime. `UCI_CONFIG_DIR` is only for testing so
the demo does not touch the host's `/etc/config`:

```sh
mkdir -p /tmp/amx-uci-demo/config
cat > /tmp/amx-uci-demo/config/network <<'UCI'
config interface 'lan'
    option device 'br-lan'
    option proto 'static'
    option ipaddr '192.168.1.1'
    option netmask '255.255.255.0'
UCI

sudo ubusd -s /tmp/amx-uci-demo/ubus.sock
sudo env LD_LIBRARY_PATH=/path/to/uci/build \
  UCI_CONFIG_DIR=/tmp/amx-uci-demo/config \
  amxrt -u ubus:/tmp/amx-uci-demo/ubus.sock \
  -L "$PWD" -N -E odl/uci-route.odl

sudo ubus -s /tmp/amx-uci-demo/ubus.sock call UciRoute _add \
  '{"parameters":{"Alias":"route1"}}'

sudo ubus -s /tmp/amx-uci-demo/ubus.sock call UciRoute _set \
  '{"rel_path":"1.","parameters":{
      "Target":"10.0.0.0",
      "Netmask":"255.0.0.0",
      "Gateway":"192.168.1.1",
      "Interface":"lan"
  }}'

cat /tmp/amx-uci-demo/config/network
```

Expected addition:

```uci
config route 'route1'
    option interface 'lan'
    option target '10.0.0.0'
    option netmask '255.0.0.0'
    option gateway '192.168.1.1'
```

## What is verified

- ✅ Compiles as a shared object against AMX + libuci
- ✅ `amxrt` loads the module and registers `UciRoute`
- ✅ `_add` + `_set` triggers `route_changed`
- ✅ libuci writes the route to an isolated `network` file
- ✅ `uci_commit` persists the section

## Not production

- no delete/disable synchronization
- no rollback or retry
- no ACL for the object
- no netifd reload/ubus notification after commit
- no concurrency handling
- `UCI_CONFIG_DIR` is a test hook, not a production configuration mechanism
- field mapping is simplified, not full TR-181

## License

MIT
