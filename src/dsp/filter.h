#ifndef DSP_FILTER_H
#define DSP_FILTER_H 1

#include "func.h"

// sample and hold

typedef struct {
  double trigger;
  double value;
} SAMPHOLD;

double samphold(SAMPHOLD *s, double value, double trigger) {
  if (trigger < s->trigger) {
        s->value = value;
  }
  s->trigger = trigger;
  return s->value;
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

// based on pd's [vcf~] [lop~] [hip~]

typedef struct { double re, im; } VCF;

static inline double vcf(VCF *s, double x, double hz, double q) {
  double qinv = q > 0 ? 1 / q : 0;
  double ampcorrect = 2 - 2 / (q + 2);
  double cf = hz * twopi / SR;
  if (cf < 0) { cf = 0; }
  double r = qinv > 0 ? 1 - cf * qinv : 0;
  if (r < 0) { r = 0; }
  double oneminusr = 1 - r;
  double cre = r * cos(cf);
  double cim = r * sin(cf);
  double re2 = s->re;
  s->re = ampcorrect * oneminusr * x + cre * re2 - cim * s->im;
  s->im = cim * re2 + cre * s->im;
  return s->re;
}

typedef struct { double y; } LOP;

static inline double lop(LOP *s, double x, double hz) {
  double c = clamp(twopi * hz / SR, 0, 1);
  return s->y = mix(x, s->y, 1 - c);
}

typedef struct { double y; } HIP;

static inline double hip(HIP *s, double x, double hz) {
  double c = clamp(1 - twopi * hz / SR, 0, 1);
  double n = 0.5 * (1 + c);
  double y = x + c * s->y;
  double o = n * (y - s->y);
  s->y = y;
  return o;
}

// pd/extra/hilbert~.pd

typedef struct { double w[2][2][2]; } HILBERT;

static inline void hilbert(double out[2], HILBERT *h, const double in[2]) {
  static const double c[2][2][5] =
    { { {  1.94632, -0.94657,   0.94657,  -1.94632, 1 }
      , {  0.83774, -0.06338,   0.06338,  -0.83774, 1 }
      }
    , { { -0.02569,  0.260502, -0.260502,  0.02569, 1 }
      , {  1.8685,  -0.870686,  0.870686, -1.8685,  1 }
      }
    };
  for (int i = 0; i < 2; ++i) {
    double w = in[i] + c[i][0][0] * h->w[i][0][0] + c[i][0][1] * h->w[i][0][1];
    double x = c[i][0][2] * w + c[i][0][3] * h->w[i][0][0] + c[i][0][4] * h->w[i][0][1];
    h->w[i][0][1] = h->w[i][0][0];
    h->w[i][0][0] = w;
    w = x + c[i][1][0] * h->w[i][1][0] + c[i][1][1] * h->w[i][1][1];
    out[i] = c[i][1][2] * w + c[i][1][3] * h->w[i][1][0] + c[i][1][4] * h->w[i][1][1];
    h->w[i][1][1] = h->w[i][1][0];
    h->w[i][1][0] = w;
  }
}

typedef struct {
  double ya1;
  double wa1;
  double yb1;
  double wb1;
  double yc1;
  double wc1;
  double yd1;
} MOOG;

static inline double moog(MOOG *m, double audio, double frequency, double resonance) {
  MOOG *muug = m;
  double ya1 = muug->ya1;
  double wa1 = muug->wa1;
  double yb1 = muug->yb1;
  double wb1 = muug->wb1;
  double yc1 = muug->yc1;
  double wc1 = muug->wc1;
  double yd1 = muug->yd1;
  double v = 2;
  double x = audio;
  double f = frequency;
  double r = resonance;
  double g = 1 - exp(-twopi * f / SR);
  double ya = ya1 + v * g * tanh((x - 4 * r * yd1) / v - wa1);
  double wa = tanh(ya / v);
  double yb = yb1 + v * g * (wa - wb1);
  double wb = tanh(yb / v);
  double yc = yc1 + v * g * (wb - wc1);
  double wc = tanh(yc / v);
  double yd = yd1 + v * g * (wc - tanh(yd1 / v));
  double y = yd;
  muug->ya1 = ya;
  muug->wa1 = wa;
  muug->yb1 = yb;
  muug->wb1 = wb;
  muug->yc1 = yc;
  muug->wc1 = wc;
  muug->yd1 = yd;
  return y;
}

// one-pole one-zero low pass filter designed in the Z plane

typedef struct {
  double x, y;
} LOP1;

static inline double lop1(LOP1 *s, double x, double hz)
{
  const double w = twopi * clamp(fabs(hz / SR), 0, 0.5);
  const double a = (1 - sin(w)) / cos(w);
  const double b = (1 - a) * 0.5;
  const double y = b * (x + s->x) + a * s->y;
  s->x = x;
  s->y = y;
  return y;
}

// one-pole one-zero high pass filter designed in the Z plane

typedef struct {
  double x, y;
} HIP1;

static inline double hip1(HIP1 *s, double x, double hz)
{
  const double w = twopi * clamp(fabs(hz / SR), 0, 0.5);
  const double a = (1 - sin(w)) / cos(w);
  const double b = (1 + a) * 0.5;
  const double y = b * (x - s->x) + a * s->y;
  s->x = x;
  s->y = y;
  return y;
}

#endif
