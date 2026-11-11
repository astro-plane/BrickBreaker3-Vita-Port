#!/bin/bash
# One-time: copy the iPhone app (binary + resources) to ux0:data/bb3/ on the Vita, one file at a time with curl.
IPA="$1"; VITA=${VITA:-192.168.0.19}
if [ ! -f "$IPA" ]; then echo "usage: $0 /path/to/the-iphone.ipa"; exit 1; fi
T=$(mktemp -d); unzip -q "$IPA" -d "$T" || exit 1
APP=$(ls -d "$T"/Payload/*.app | head -1)
if [ ! -f "$APP/BBD3" ]; then echo "this is not the iPhone version (no BBD3 binary)"; exit 1; fi
cd "$APP" || exit 1
N=$(find . -type f | wc -l | tr -d ' '); i=0; fail=0
while IFS= read -r f; do
  i=$((i+1)); rel="${f#./}"
  url="ftp://$VITA:1337/ux0:/data/bb3/$(printf '%s' "$rel" | sed 's/ /%20/g')"
  printf '[%d/%d] %s\n' "$i" "$N" "$rel"
  curl -sS --ftp-create-dirs -T "$f" "$url" || { echo "  FAILED: $rel"; fail=$((fail+1)); }
done < <(find . -type f)
cd /; rm -rf "$T"
if [ "$fail" = 0 ]; then echo "DATA INSTALLED to ux0:data/bb3/ ($N files)"; else echo "$fail file(s) failed - paste the FAILED lines"; fi
