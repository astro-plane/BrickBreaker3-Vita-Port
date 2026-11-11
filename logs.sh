#!/bin/bash
# Download the loader log (and the game's own log, if any) from the Vita to the Desktop.
VITA=${VITA:-192.168.0.19}
rm -f ~/Desktop/loader.txt ~/Desktop/game_log.txt
lftp -e "set xfer:clobber on; get /ux0:/data/bb3/loader.txt -o ~/Desktop/loader.txt; get /ux0:/data/bb3/game_log.txt -o ~/Desktop/game_log.txt; quit" ftp://$VITA:1337 2>&1 | grep -v "550"
ls -l ~/Desktop/loader.txt ~/Desktop/game_log.txt 2>/dev/null
head -1 ~/Desktop/loader.txt
