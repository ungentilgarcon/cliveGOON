all: live go.so

live: live.c
	colorgcc -std=c99 -Wall -pedantic -Wextra -Wno-unused-parameter -O3 -I. -o live live.c `pkg-config --cflags --libs jack` -ldl -lm

go.so: go.c dsp.h dsp/*.h
	colorgcc -std=c99 -Wall -pedantic -Wextra -Wno-unused-parameter -O3 -I. -shared -fPIC -o go.so go.c -lm
