#!/bin/bash
# Block Breaker 3: build (trying fallback variants - the Vita converter vita-elf-create is picky) and upload eboot.bin to the Vita.
cd ~/bb3_vita || exit 1
OK=""
VARIANTS=(
  "-O2|"
  "-O2|-DNO_FILE_TRACE"
  "-Os|"
  "-Os|-DNO_FILE_TRACE -DNO_CALL_TRACE"
  "-O1|-DNO_FILE_TRACE"
  "-O2|-DNO_FILE_TRACE -DNO_CALL_TRACE -DNO_SBRK_LOG"
  "-Os|-DNO_FILE_TRACE -DNO_CALL_TRACE -DNO_SBRK_LOG"
  "-O1|-DNO_FILE_TRACE -DNO_CALL_TRACE -DNO_SBRK_LOG"
  "-O3|-DNO_FILE_TRACE"
)
for item in "${VARIANTS[@]}"; do
  opt="${item%%|*}"; v="${item#*|}"
  cd ~/bb3_vita || exit 1
  rm -rf build; mkdir build; cd build
  cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DBQ_OPT="$opt" -DCMAKE_C_FLAGS="-Wno-error=int-conversion -Wno-error=incompatible-pointer-types $v" .. > /dev/null
  make > make.log 2>&1
  grep -iE "error|undefined|unresolved|cannot find|nid|vita-elf" make.log | head -6
  if grep -qiE "undefined reference|cannot find -l" make.log; then
    echo "=== LINK PROBLEM (no point trying other variants) - paste everything below ==="
    grep -iE "undefined reference|cannot find -l|stub not found" make.log | sort -u | head -40
    cmake .. 2>&1 | grep -i "stub not found"
    exit 1
  fi
  if [ -f bb3.self ]; then OK="$opt $v"; break; fi
  echo "variant [$opt $v] failed, trying the next one..."
done
if [ -z "$OK" ]; then
  echo "ALL VARIANTS FAILED - paste the output above"
  exit 1
fi
echo "BUILD OK, variant: [$OK]"
VITA=${VITA:-192.168.0.19}
if curl -sS -T bb3.self ftp://$VITA:1337/ux0:/app/BBRK00003/eboot.bin 2>/dev/null; then
  echo "UPLOAD DONE - now launch Block Breaker 3 on the Vita"
else
  curl -sS -T bb3.vpk ftp://$VITA:1337/ux0:/data/bb3.vpk && echo "FIRST INSTALL: bb3.vpk is in ux0:data/ - install it with VitaShell (it is a one-time step, later builds go straight to eboot.bin)"
fi
