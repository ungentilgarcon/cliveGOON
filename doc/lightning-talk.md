---
title: Live-coding Audio in C
author: Claude Heiland-Allen
date: 2018-06-09
classoption: aspectratio=149
fontfamily: lato
fontfamilyoptions: default
fontsize: 14pt
header-includes:
  - \definecolor{mathr}{rgb}{0.5, 0.25, 0.25}
  - \usecolortheme[named=mathr]{structure}
---

## JACK Audio

- Process callback delegates to a function pointer

- Passes a pointer to a preallocated memory area

- Main thread can change the function pointer

## Dynamic Reloading

- `dlopen()`, `dlsym()`, `-ldl`

- Race condition: don't unload running code

- Double buffering: `dl` caches aggressively

## Detecting File Changes

- `inotify`

- if `go.c` changes: compile it (by calling `make`)

- if `go.so` changes: make a copy and load it

## Where

- performance tonight!

- <claude@mathr.co.uk>

- <https://mathr.co.uk/clive>
