---
title: Live-coding Audio in C
author: Claude Heiland-Allen
date: 2018-04-10
classoption: aspectratio=149
fontfamily: lato
fontfamilyoptions: default
fontsize: 14pt
---

## Overview

- Coding Audio In C

- Live-coding Audio

- Where Next?

# Coding Audio In C

## C Programming Language

- Used to implement UNIX in the 1970s

- Fast to compile

- Fast at runtime

## Audio DSP Environments

- Digital Signal Processing

- Pure-data is implemented in C

- SuperCollider3 is implemented in C++

## Audio Processing

- Example: one-pole low-pass filter

- Inputs: audio $x_n$, frequency $f_n$

- Output: audio $y_n$

- State: previous output audio sample $y_{n - 1}$

## Filter Equation

$$
\begin{aligned}
c_n &= 2 \pi \frac{f_n}{SR} \\
y_n &= c_n x_n + (1 - c_n) y_{n - 1}
\end{aligned}
$$

## Filter Implementation

\fontsize{12pt}{13}\selectfont

```C
typedef struct {
  double y;
} LOP;

double lop(LOP *s, double x, double hz) {
  double c = clamp(twopi * hz / SR, 0, 1);
  return s->y = mix(x, s->y, 1 - c);
}
```

## Filter Usage

\fontsize{10pt}{11}\selectfont

```C
#define SR 48000
#include "dsp.h"

typedef struct {
  int reloaded;
  LOP l[2]; // state declaration
} S;

int go(S *s, int channels, const float *in, float *out) {
  for (int c = 0; c < channels; ++c)
    if (c < 2)
      out[c] = lop(&s->l[c], in[c], 200); // usage
    else
      out[c] = 0;
  return 0;
}
```

# Live-coding Audio

## How Pure-data Works

- tightly coupled core/GUI

- Object boxes on a canvas

- each Object keeps its state

- patch cords form a DSP graph

## How SuperCollider3 Works

- loosely coupled client/server

- language compiles UGen graph into a Synth

- server maintains a collection of Synths

- server processes Synths in order

## Pure-data Strengths

- deterministic execution

- subsample accurate logical clock

- plugins ("externals") for graphics etc

- liberal license

- single lead developer, conservative changes

## SuperCollider3 Strengths

- IDE evaluates selected code

- server handles dynamic allocation well

- timing with latency offset

- copyleft license

- community development, breaking changes

## Pure-data Weaknesses

- predefined collection of Objects

- flexible maths is slower: expr, expr~, fexpr~

- all changes recompile the whole DSP chain

- how to structure DSP live-coding to avoid discontinuities?

## SuperCollider3 Weaknesses

- (note: I'm no SuperCollider3 expert)

- predefined collection of UGens

- Synths can't be modified while they are running

- how to structure DSP live-coding to avoid discontinuities?

## How Clive Works

- watches a directory for file changes

- recompiles changed sources

- reloads recompiled library

- maintains memory area between reloads

## Clive Strengths

- two-phase edit/commit cycle

- no explicit "DSP graph of UGens" separation

- compilation is realtime safe

- reloading is (almost) realtime safe

- processing uses single-sample callbacks (simple)

## Clive Weaknesses

- explicit state maintenance, append-only

- tiny UGen library

- compilation is high latency

- reloading is at JACK block boundaries

- processing uses single-sample callbacks (slow)

# Where Next?

## Embiggen UGen Library

- oscillators (bandlimited wavetables, ...)

- filters (Butterworth, Moog, ...)

- effects (hverb, ...)

- sequencing (metaphor, ...)

## Block-based Processing

- more CPU efficient

- more awkward

- FFT-based spectral processing

## Embedded DSP

- Bela platform: low latency audio processing device

- compile on host and transfer over USB/network

- also supports sensors/electronics

- rapid prototyping of embedded instruments

# Thanks

## Clive

- <claude@mathr.co.uk>

- <https://mathr.co.uk/clive>

- <https://code.mathr.co.uk/clive>
