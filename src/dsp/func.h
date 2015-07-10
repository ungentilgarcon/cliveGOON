#ifndef DSP_FUNC_H
#define DSP_FUNC_H 1

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

static inline double wrapat(double x, double n) {
  return wrap(x * n) / n;
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
