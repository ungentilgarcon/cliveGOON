#ifndef DSP_EFFECTS_H
#define DSP_EFFECTS_H 1

#include "func.h"
#include "delay.h"
#include "osc.h"

typedef struct { DELAY tape; float buf[16384]; } CHORUS;

static inline double chorus(CHORUS *ch, int voices, double delayms, double depthms, double phase, double x) {
  ch->tape.length = 16384;
  double c = 0;
  for (int i = 0; i < voices; ++i) {
    c += delread4(&ch->tape, delayms + depthms * cos(twopi * (phase + i / (double) voices)));
  }
  c /= voices;
  delwrite(&ch->tape, x - c);
  return c;
}

typedef struct { PHASOR head; DELAY tape; float buf[16384]; } PITCHSHIFT;

static inline double pitchshift(PITCHSHIFT *ps, double delayms, double windowms, double transposesemi, double x) {
  ps->tape.length = 16384;
  delwrite(&ps->tape, x);
  double p0 = phasor(&ps->head, (1 - exp(0.05776 * transposesemi)) * 1000 / windowms);
  double p1 = wrap(p0 + 0.5);
  return sin(pi * p0) * delread4(&ps->tape, windowms * p0 + delayms)
       + sin(pi * p1) * delread4(&ps->tape, windowms * p1 + delayms);
}

#endif
