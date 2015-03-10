#ifndef DSP_H
#define DSP_H 1

#include <math.h>
#include <stdlib.h>

#define pi 3.141592653589793
#define twopi 6.283185307179586


static inline double wrap(double x) {
  return x - floor(x);
}

static inline double wrapto(double x, double n) {
  return wrap(x / n) * n;
}


typedef struct { double phase; } PHASOR;

static inline double phasor(PHASOR *p, double hz) {
  return p->phase = wrap(p->phase + hz / SR);
}


static inline double clamp(double x, double lo, double hi) {
  return fmin(fmax(x, lo), hi);
}

static inline double mix(double x, double y, double t) {
  return (1 - t) * x + t * y;
}


static inline double trisaw(double x, double t) {
  double s = clamp(t, 0.0000001, 0.9999999);
  double y = clamp(x, 0, 1);
  return y < s ? y / s : (1 - y) / (1 - s);
}


static inline double noise() {
  return 2 * (rand() / (double) RAND_MAX - 0.5);
}


typedef struct { int length, woffset; } DELAY;

static inline void delwrite(DELAY *del, double x0) {
  float *buffer = (float *) (del + 1);
  int l = del->length;
  l = (l > 0) ? l : 1;
  int w = del->woffset;
  buffer[w++] = x0;
  if (w >= l) { w -= l; }
  del->woffset = w;
}

static inline double delread1(DELAY *del, double ms) {
  float *buffer = (float *) (del + 1);
  int l = del->length;
  l = (l > 0) ? l : 1;
  int w = del->woffset;
  int d = ms / 1000.0 * SR;
  d = (0 < d && d < l) ? d : 0;
  int r = w - d;
  r = r < 0 ? r + l : r;
  return buffer[r];
}

static inline double delread2(DELAY *del, double ms) {
  float *buffer = (float *) (del + 1);
  int l = del->length;
  l = (l > 0) ? l : 1;
  int w = del->woffset;
  double d = ms / 1000.0 * SR;
  int d0 = floor(d);
  int d1 = d0 + 1;
  double t = d - d0;
  d0 = (0 < d0 && d0 < l) ? d0 : 0;
  d1 = (0 < d1 && d1 < l) ? d1 : d0;
  int r0 = w - d0;
  int r1 = w - d1;
  r0 = r0 < 0 ? r0 + l : r0;
  r1 = r1 < 0 ? r1 + l : r1;
  double y0 = buffer[r0];
  double y1 = buffer[r1];
  return (1 - t) * y0 + t * y1;
}

// https://en.wikipedia.org/wiki/Cubic_Hermite_spline#Interpolation_on_the_unit_interval_without_exact_derivatives

static inline double delread4(DELAY *del, double ms) {
  float *buffer = (float *) (del + 1);
  int l = del->length;
  l = (l > 0) ? l : 1;
  int w = del->woffset;
  double d = ms / 1000.0 * SR;
  int d1 = floor(d);
  int d0 = d1 - 1;
  int d2 = d1 + 1;
  int d3 = d1 + 2;
  double t = d - d1;
  d0 = (0 < d0 && d0 < l) ? d0 : 0;
  d1 = (0 < d1 && d1 < l) ? d1 : d0;
  d2 = (0 < d2 && d2 < l) ? d2 : d1;
  d3 = (0 < d3 && d3 < l) ? d3 : d2;
  int r0 = w - d0;
  int r1 = w - d1;
  int r2 = w - d2;
  int r3 = w - d3;
  r0 = r0 < 0 ? r0 + l : r0;
  r1 = r1 < 0 ? r1 + l : r1;
  r2 = r2 < 0 ? r2 + l : r2;
  r3 = r3 < 0 ? r3 + l : r3;
  double y0 = buffer[r0];
  double y1 = buffer[r1];
  double y2 = buffer[r2];
  double y3 = buffer[r3];
  double a0 = -t*t*t + 2*t*t - t;
  double a1 = 3*t*t*t - 5*t*t + 2;
  double a2 = -3*t*t*t + 4*t*t + t;
  double a3 = t*t*t - t*t;
  return 0.5 * (a0 * y0 + a1 * y1 + a2 * y2 + a3 * y3);
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


static inline double chorus(DELAY *del, int voices, double delayms, double depthms, double phase, double x) {
  double c = 0;
  for (int i = 0; i < voices; ++i) {
    c += delread4(del, 3 + cos(twopi * (phase + i / (double) voices)));
  }
  c /= voices;
  delwrite(del, c + x);
  return c;
}


static inline double pitchshift(DELAY *tape, PHASOR *head, double delayms, double windowms, double transposesemi, double x) {
  delwrite(tape, x);
  double p0 = phasor(head, (1 - exp(0.05776 * transposesemi)) * 1000 / windowms);
  double p1 = wrap(p0 + 0.5);
  return sin(pi * p0) * delread4(tape, windowms * p0 + delayms)
       + sin(pi * p1) * delread4(tape, windowms * p1 + delayms);
}

// http://musicdsp.org/files/Audio-EQ-Cookbook.txt

#define flatq 0.7071067811865476

typedef struct { double b0, b1, b2, a1, a2, x1, x2, y1, y2; } BIQUAD;

static inline double biquad(BIQUAD *bq, double x0) {
  double b0 = bq->b0, b1 = bq->b1, b2 = bq->b2, a1 = bq->a1, a2 = bq->a2;
  double x1 = bq->x1, x2 = bq->x2, y1 = bq->y1, y2 = bq->y2;
  double y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
  bq->y2 = y1;
  bq->y1 = y0;
  bq->x2 = x1;
  bq->x1 = x0;
  return y0;
}

static inline BIQUAD *lowpass(BIQUAD *bq, double hz, double q) {
  double w0 = hz * twopi / SR;
  double a = fabs(sin(w0) / (2 * q));
  double c = cos(w0);
  double b0 = (1 - c) / 2, b1 = 1 - c, b2 = (1 - c) / 2;
  double a0 = 1 + a, a1 = -2 * c, a2 = 1 - a;
  bq->b0 = b0 / a0;
  bq->b1 = b1 / a0;
  bq->b2 = b2 / a0;
  bq->a1 = a1 / a0;
  bq->a2 = a2 / a0;
  return bq;
}

static inline BIQUAD *highpass(BIQUAD *bq, double hz, double q) {
  double w0 = hz * twopi / SR;
  double a = fabs(sin(w0) / (2 * q));
  double c = cos(w0);
  double b0 = (1 + c) / 2, b1 = -(1 + c), b2 = (1 + c) / 2;
  double a0 = 1 + a, a1 = -2 * c, a2 = 1 - a;
  bq->b0 = b0 / a0;
  bq->b1 = b1 / a0;
  bq->b2 = b2 / a0;
  bq->a1 = a1 / a0;
  bq->a2 = a2 / a0;
  return bq;
}

static inline BIQUAD *bandpass(BIQUAD *bq, double hz, double q) {
  double w0 = hz * twopi / SR;
  double a = fabs(sin(w0) / (2 * q));
  double c = cos(w0);
  double b0 = a, b1 = 0, b2 = -a;
  double a0 = 1 + a, a1 = -2 * c, a2 = 1 - a;
  bq->b0 = b0 / a0;
  bq->b1 = b1 / a0;
  bq->b2 = b2 / a0;
  bq->a1 = a1 / a0;
  bq->a2 = a2 / a0;
  return bq;
}

static inline BIQUAD *notch(BIQUAD *bq, double hz, double q) {
  double w0 = hz * twopi / SR;
  double a = fabs(sin(w0) / (2 * q));
  double c = cos(w0);
  double b0 = 1, b1 = -2 * c, b2 = 1;
  double a0 = 1 + a, a1 = -2 * c, a2 = 1 - a;
  bq->b0 = b0 / a0;
  bq->b1 = b1 / a0;
  bq->b2 = b2 / a0;
  bq->a1 = a1 / a0;
  bq->a2 = a2 / a0;
  return bq;
}

// pd-0.45-5/src/d_math.c

#define log10overten 0.23025850929940458
#define tenoverlog10 4.3429448190325175

static inline double mtof(double f) {
  return 8.17579891564 * exp(0.0577622650 * fmin(f, 1499));
}

static inline double ftom(double f) {
  return 17.3123405046 * log(0.12231220585 * f);
}

static inline double dbtorms(double f) {
  return exp(0.5 * log10overten * (fmin(f, 870) - 100));
}

static inline double rmstodb(double f) {
  return 100 + 2 * tenoverlog10 * log(f);
}

static inline double dbtopow(double f) {
  return exp(log10overten * (fmin(f, 485) - 100));
}

static inline double powtodb(double f) {
  return 100 + tenoverlog10 * log(f);
}

#endif
