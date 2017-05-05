all: live go.so visuals ascii.pgm

live: live.c
	colorgcc -std=c99 -Wall -pedantic -Wextra -Wno-unused-parameter -O3 -mfpmath=sse -I. -o live live.c `pkg-config --cflags --libs jack` -ldl -lm

go.so: go.c dsp.h dsp/*.h
	colorgcc -std=c99 -Wall -pedantic -Wextra -Wno-unused-parameter -O3 -mfpmath=sse -I. -shared -fPIC -o go.so go.c -lm

visuals: visuals.c dsp.h dsp/*.h
	colorgcc -xc++ -std=c++11 -Wall -pedantic -Wextra -Wno-unused-parameter -O3 -I. -o visuals visuals.c -lGL -lGLEW -lglfw -lv4l2 `pkg-config --cflags --libs jack` -lm -ljpeg

ascii.pgm: ascii.png
	pngtopnm < ascii.png > ascii.pgm
