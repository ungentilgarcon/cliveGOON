#ifndef DSP_EFFECTS_H
#define DSP_EFFECTS_H 1

#include "func.h"
#include "delay.h"
#include "osc.h"
#include "filter.h"

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

typedef struct { BIQUAD hip[2], lop[3]; } COMPRESS;

void compress(double out[2], COMPRESS *s, double hiphz, double lophz1, double lophz2, double db, const double in[2]) {
  highpass(&s->hip[0], hiphz, flatq);
  highpass(&s->hip[1], hiphz, flatq);
  lowpass(&s->lop[0], lophz1, flatq);
  lowpass(&s->lop[1], lophz1, flatq);
  lowpass(&s->lop[2], lophz2, flatq);
  double h[2] =
    { biquad(&s->hip[0], in[0])
    , biquad(&s->hip[1], in[1])
    };
  h[0] *= h[0];
  h[1] *= h[1];
  h[0] = biquad(&s->lop[0], h[0]);
  h[1] = biquad(&s->lop[1], h[1]);
  double env = biquad(&s->lop[2], sqrt(fmax(0, h[0] + h[1])));
  double g[2] =
    { in[0] / env
    , in[1] / env
    };
  if (isnan(g[0]) || isinf(g[0])) { g[0] = 0; }
  if (isnan(g[1]) || isinf(g[1])) { g[1] = 0; }
  env = rmstodb(env);
  if (env > db) {
    env = db + (env - db) / 4;
  } else {
    env = db;
  }
  env = 0.25 * dbtorms(env) / dbtorms((100 - db) / 4 + db);
  if (isnan(env) || isinf(env)) { env = 0; }
  out[0] = tanh(g[0] * env);
  out[1] = tanh(g[1] * env);
}

#endif
