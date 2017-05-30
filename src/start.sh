#!/bin/bash
SESSION="session-$(date -u +%F-%H%M%S)"
git checkout -b "${SESSION}"
(
make
ecasound -q -G:jack,record -f:f32,2,48000 -i:jack -o "${SESSION}.wav" &
pid[0]=$!
sleep 5
xterm -geometry 80x41+0+0   -T live -e bash -c 'while true ; do ./live ; done' &
pid[1]=$!
xterm -geometry 80x13+0+592 -T htop -e bash -c 'while true ; do htop ; done' &
pid[2]=$!
while true ; do geany -mist dsp.h dsp/*.h go.c ; done &
pid[3]=$!
trap "kill ${pid[0]} ${pid[1]} ${pid[2]} ${pid[3]}; exit 0" INT
wait
)
