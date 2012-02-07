all: live go.so

live: live.c
	gcc -std=c99 -Wall -pedantic -Wextra -Wno-unused-parameter -O3 -o live live.c `pkg-config --cflags --libs jack` -ldl -lm

go.so: go.c
	gcc -std=c99 -Wall -pedantic -Wextra -Wno-unused-parameter -O3 -shared -fPIC -o go.so go.c -lm
