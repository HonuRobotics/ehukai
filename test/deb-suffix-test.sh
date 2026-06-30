#!/usr/bin/env bash
# Verify EW_DEB_DISTRO_SUFFIX yields a correctly-versioned .deb + changelog.
set -euo pipefail
src="$(cd "$(dirname "$0")/.." && pwd)"
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
cmake -S "$src" -B "$tmp/b" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
      -DBUILD_TESTING=OFF -DEW_DEB_DISTRO_SUFFIX="~ubuntu24.04" >/dev/null
cmake --build "$tmp/b" --target EncinoWaves -j"$(nproc)" >/dev/null
( cd "$tmp/b" && cpack -G DEB >/dev/null )

deb="$tmp/b/libencinowaves1_1.0.0-1~ubuntu24.04_amd64.deb"
test -f "$deb" || { echo "FAIL: suffixed .deb not produced"; ls "$tmp/b"/*.deb; exit 1; }
[ "$(dpkg-deb -f "$deb" Version)" = "1.0.0-1~ubuntu24.04" ] || { echo "FAIL: control version"; exit 1; }
dpkg-deb --fsys-tarfile "$deb" \
  | tar -xO ./usr/share/doc/libencinowaves1/changelog.Debian.gz | zcat | head -1 \
  | grep -q "1.0.0-1~ubuntu24.04" || { echo "FAIL: changelog version not stamped"; exit 1; }
echo "PASS: deb-suffix"
