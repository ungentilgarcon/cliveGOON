#ifndef DSP_OSC_H
#define DSP_OSC_H 1

#include "dsp/func.h"

typedef struct { double phase; } PHASOR;

static inline double phasor(PHASOR *p, double hz) {
  return p->phase = wrap(p->phase + hz / SR);
}

#define WAVELEVELS 11

typedef struct {
  float wave[1 << (WAVELEVELS + 1)];
} WAVETABLE;

static inline void wavetable_saw(WAVETABLE *w) {
  for (int level = 0; level <= WAVELEVELS; ++level) {
    int n = 1 << level;
    int m = n >> 1;
    for (int i = 0; i < n; ++i) {
      w->wave[n + i] = 0;
      for (int k = 1; k < m; ++k) {
        double phase = twopi * ((k * i) % n) / (double) n;
        double sign = (k & 1) ? 1 : -1;
        double window = clamp(2 * (m - k) / (double) m, 0, 1);
        w->wave[n + i] += window * sign * sin(phase) / k;
      }
      w->wave[n + i] *= 2 / pi;
    }
  }
}

typedef struct {
  double phase;
} WAVE;

static inline double wave(WAVETABLE *wt, WAVE *w, double phase) {
  double delta = fabs(wrap(phase - w->phase));
  w->phase = phase;
  double l = -log2(delta);
  int l1 = floor(l);
  int l0 = l1 - 1;
  double lt = l - l1;
  l0 = clamp(l0, 0, WAVELEVELS);
  l1 = clamp(l1, 0, WAVELEVELS);
  int n0 = 1 << l0;
  int n1 = 1 << l1;
  double t = wrap(phase);
  double i0 = t * n0;
  int i00 = floor(i0);
  int i01 = (i00 + 1) % n0;
  double i0t = i0 - i00;
  double i1 = t * n1;
  int i10 = floor(i1);
  int i11 = (i10 + 1) % n1;
  double i1t = i1 - i10;
  return
    mix
    ( mix(wt->wave[n0 + i00], wt->wave[n0 + i01], i0t)
    , mix(wt->wave[n1 + i10], wt->wave[n1 + i11], i1t)
    , lt
    );
}

#endif
