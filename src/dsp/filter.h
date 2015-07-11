#ifndef DSP_FILTER_H
#define DSP_FILTER_H 1

#include "func.h"

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

#endif
