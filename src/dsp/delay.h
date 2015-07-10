#ifndef DSP_DELAY_H
#define DSP_DELAY_H 1

#include "func.h"

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

#endif
