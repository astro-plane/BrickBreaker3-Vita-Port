#!/bin/bash
# One-time: convert the game's AAC music (BGM*.caf on the Vita) to IMA4 stereo 32 kHz, which the loader's audio mixer plays.
# Originals are kept in ~/baroque_bgm/orig. Safe to run again (already-converted files are skipped).
VITA=ftp://192.168.0.19:1337/ux0:/data/baroque
W=~/baroque_bgm; mkdir -p "$W/orig" "$W/ima4"; cd "$W" || exit 1
ok=0; fail=0
for n in $(seq -w 1 27); do
  f=BGM$n.caf
  if [ ! -s "orig/$f" ]; then curl -sS "$VITA/$f" -o "orig/$f" || { echo "download failed: $f"; fail=$((fail+1)); continue; }; fi
  if afinfo "orig/$f" 2>/dev/null | grep -qi "ima4"; then echo "$f: already IMA4 on the Vita, skipping"; ok=$((ok+1)); continue; fi
  if ! afconvert -f caff -d ima4@32000 -c 2 "orig/$f" "ima4/$f"; then echo "convert failed: $f"; fail=$((fail+1)); continue; fi
  if curl -sS -T "ima4/$f" "$VITA/$f"; then echo "$f: converted and uploaded ($(du -h "ima4/$f" | cut -f1))"; ok=$((ok+1)); else echo "upload failed: $f"; fail=$((fail+1)); fi
done
echo "done: $ok ok, $fail failed (originals in $W/orig)"
