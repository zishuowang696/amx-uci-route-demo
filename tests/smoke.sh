#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
WORK=${WORK:-/tmp/amx-uci-route-demo}
SOCK="$WORK/ubus.sock"
CONFIG="$WORK/config"

need() { command -v "$1" >/dev/null 2>&1 || { echo "missing: $1" >&2; exit 1; }; }
need ubusd
need ubus
need amxrt

rm -rf "$WORK"
mkdir -p "$CONFIG"
cat > "$CONFIG/network" <<'UCI'
config interface 'lan'
    option device 'br-lan'
    option proto 'static'
    option ipaddr '192.168.1.1'
    option netmask '255.255.255.0'
UCI

cleanup() {
    [ -n "${AMX_PID:-}" ] && kill "$AMX_PID" 2>/dev/null || true
    [ -n "${UBUS_PID:-}" ] && kill "$UBUS_PID" 2>/dev/null || true
}
trap cleanup EXIT

ubusd -s "$SOCK" >"$WORK/ubusd.log" 2>&1 &
UBUS_PID=$!
sleep 1

UCI_CONFIG_DIR="$CONFIG" amxrt -u "ubus:$SOCK" -L "$ROOT" -N -E "$ROOT/odl/uci-route.odl" >"$WORK/amxrt.log" 2>&1 &
AMX_PID=$!
sleep 2

ubus -s "$SOCK" call UciRoute _add '{"parameters":{"Alias":"route1"}}' >/dev/null
ubus -s "$SOCK" call UciRoute _set '{"rel_path":"1.","parameters":{"Target":"10.0.0.0","Netmask":"255.0.0.0","Gateway":"192.168.1.1","Interface":"lan"}}' >/dev/null
sleep 1

grep -q "option target '10.0.0.0'" "$CONFIG/network"
grep -q "option gateway '192.168.1.1'" "$CONFIG/network"
echo "smoke ok"
