#!/bin/sh
# Release build: checks versions, writes the changelog, compiles the firmware and writes release artifacts to OUT
# (default ./dist/<version>): factory/ota firmware, web installer manifest, release notes, source zip.
#   sh release.sh            # version from firmware/noq-sos.yaml
set -eu
cd "$(dirname "$0")"
V=$(sed -n 's/^ *version: "\(.*\)"/\1/p' firmware/noq-sos.yaml | head -1)
OUT=${OUT:-dist/$V}
ESPHOME=${ESPHOME:-esphome}
python3 tools/changelog.py --check
python3 tools/changelog.py
python3 portal/build.py
TMP=$(mktemp -d)
sed "s#source: github://noqworks/sos@main#source: { type: local, path: $(pwd)/components }#" firmware/noq-sos.yaml > "$TMP/noq-sos.yaml"
"$ESPHOME" compile "$TMP/noq-sos.yaml"
B=$(dirname "$(find "$TMP/.esphome/build/noq-sos" -name firmware.factory.bin | head -1)")
mkdir -p "$OUT"
cp "$B/firmware.factory.bin" "$OUT/sos-$V.factory.bin"
cp "$B/firmware.ota.bin" "$OUT/sos-$V.ota.bin"
cat > "$OUT/manifest.json" <<JSON
{
  "name": "NOQ SOS",
  "version": "$V",
  "new_install_prompt_erase": true,
  "builds": [{ "chipFamily": "ESP32-S3", "parts": [{ "path": "sos-$V.factory.bin", "offset": 0 }] }]
}
JSON
git archive --format=zip --prefix="sos-$V/" -o "$OUT/sos-$V-source.zip" HEAD 2>/dev/null \
  || (cd .. && zip -qr "$OLDPWD/$OUT/sos-$V-source.zip" "$(basename "$OLDPWD")" -x '*/dist/*' '*/.esphome/*' '*/.git/*')
python3 tools/changelog.py --notes "$V" > "$OUT/release-notes.md"
cp releases.json "$OUT/releases.json"
rm -rf "$TMP"
ls -la "$OUT"
