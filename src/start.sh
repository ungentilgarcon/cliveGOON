#!/bin/bash
for c in $(seq 8)
do
  sudo cpufreq-set -c "${c}" -g performance
done
make
SESSION="session-$(date -u +%F-%H%M%S)"
git checkout -b "${SESSION}"
(
ecasound -q -G:jack,record -f:f32,2,48000 -i:jack -o "${SESSION}.wav" &
sleep 5
xterm -geometry 80x41+0+0   -T live -e bash -c 'while true ; do ./live ; done' &
xterm -geometry 80x13+0+592 -T htop -e bash -c 'while true ; do htop ; done' &
while true ; do geany -mist dsp.h dsp/*.h go.c ; done &
wait
)
