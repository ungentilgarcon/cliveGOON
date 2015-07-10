#!/bin/bash
SESSION="session-$(date -u +%F-%H%M%S)"
git checkout -b "${SESSION}"
(
make
ecasound -q -G:jack,record -f:f32,2,48000 -i:jack -o "${SESSION}.wav" &
id[0]=$!
sleep 5
xterm -geometry 80x5+0+0    -T live -e bash -c 'while true ; do ./live ; mplayer -quiet -really-quiet -ao jack $(ls -1 ../doh/*.wav | sort -R | head -n 1) >/dev/null 2>&1 ; done' &
pid[1]=$!
xterm -geometry 80x36+0+87  -T make -e bash -c 'while true ; do make -q || ( git --no-pager diff --color ; git add go.c ; git commit -m "go $(date --iso=s)" ; make --quiet ) ; inotifywait -q -q -e close_write -e move_self -e delete_self go.c ; done' &
pid[2]=$!
xterm -geometry 80x13+0+592 -T htop -e bash -c 'htop' &
pid[3]=$!
while true ; do geany -mist dsp.h go.c ; done &
pid[4]=$!
trap "kill ${pid[0]} ${pid[1]} ${pid[2]} ${pid[3]} ${pid[4]} ; exit 0" INT
wait
)
git checkout master
