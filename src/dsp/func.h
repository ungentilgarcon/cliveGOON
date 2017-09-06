#ifndef DSP_FUNC_H
#define DSP_FUNC_H 1

#include <math.h>
#include <stdbool.h>
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

static inline double ABS(double x)
{
  return x < 0 ? -x : x;
}

static inline double CLAMP(double x, double lo, double hi)
{
  return lo < x ? x < hi ? x : hi : lo;
}

static inline double WRAP(double x)
{
  return x - floor(x);
}

static inline double FMOD(double x, double y)
{
  return WRAP(x / y) * y;
}

static const double PI   = 3.141592653589793;
static const double PI_2 = 1.5707963267948966;
static const double PI_4 = 0.7853981633974483;

static inline double TAN(double x)
{
  bool recip = false;
  x = ABS(x);
  if (unlikely(x >= PI_2))
  {
    x = FMOD(x + PI_2, PI) - PI_2;
  }
  if (unlikely(x >= PI_4))
  {
    recip = true;
    x = PI_2 - x;
  }
  // 0 <= x <= pi/4
  // [5/6] Padé approximant
  double x2 = x * x;
  double a = x * (10395.0 + x2 * (-1260.0 + x2 * 21.0));
  double b = 10395.0 + x2 * (-4725.0 + x2 * (210.0 - x2));
  return recip ? b / a : a / b;
}

static inline double TANH(double x)
{
  x = CLAMP(x, -4.0, 4.0);
  // [5/6] Padé approximant
  double x2 = x * x;
  double a = x * (10395.0 + x2 * (1260.0 + x2 * 21.0));
  double b = 10395.0 + x2 * (4725.0 + x2 * (210.0 + x2));
  return a / b;
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


static inline double bytebeat(int x) {
  return ((x & 0xFF) - 0x40) / (double) 0x40;
}

static inline double bitcrush(double x, double bits) {
  double n = pow(2, bits);
  return round(x * n) / n;
}

#endif
