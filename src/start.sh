#!/bin/bash
git checkout -b "session-$(date -u +%F-%H%M%S)"
make
xterm -geometry 80x5+0+0    -T live -e bash -c 'while true ; do ./live | ts ; mplayer -quiet -really-quiet -ao jack $(ls -1 ../doh/*.wav | sort -R | head -n 1) >/dev/null 2>&1 ; done' &
xterm -geometry 80x25+0+102 -T make -e bash -c 'while true ; do make -q || ( git add go.c ; git commit -m "go $(date --iso=s)" ; make --quiet ) ; sleep 1 ; done' &
xterm -geometry 80x25+0+449 -T htop -e bash -c 'htop' &
xterm -geometry 80x56+493+0 -T edit -e bash -c 'while true ; do nano ./go.c ; done' &
wait
git checkout master
