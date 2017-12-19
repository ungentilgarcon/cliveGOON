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

typedef struct { HIP hip[2]; LOP lop[3]; } COMPRESS;

void compress(double out[2], COMPRESS *s, double hiphz, double lophz1, double lophz2, double db, const double in[2]) {
  double h[2] =
    { hip(&s->hip[0], in[0], hiphz)
    , hip(&s->hip[1], in[1], hiphz)
    };
  h[0] *= h[0];
  h[1] *= h[1];
  h[0] = lop(&s->lop[0], h[0], lophz1);
  h[1] = lop(&s->lop[1], h[1], lophz1);
  double env = lop(&s->lop[2], sqrt(fmax(0, h[0] + h[1])), lophz2);
  double env0 = env;
  env = rmstodb(env);
  if (env > db) {
    env = db + (env - db) / 4;
  } else {
    env = db;
  }
  env = 0.25 * dbtorms(env) / dbtorms((100 - db) / 4 + db);
  double gain = env / env0;
  if (isnan(gain) || isinf(gain)) { gain = 0; }
  gain = clamp(gain, 0, 1.0e6);
  out[0] = tanh(in[0] * gain);
  out[1] = tanh(in[1] * gain);
}

// reverb

typedef struct {
  double x[4];
} vec4;

static inline void vsub(vec4 *o, const vec4 *a, const vec4 *b) {
  for (int i = 0; i < 4; ++i) { o->x[i] = a->x[i] - b->x[i]; }
}

static inline void vmul(vec4 *o, const vec4 *a, double b) {
  for (int i = 0; i < 4; ++i) { o->x[i] = a->x[i] * b; }
}

static inline double vdot(const vec4 *a, const vec4 *b) {
  double s = 0;
  for (int i = 0; i < 4; ++i) { s += a->x[i] * b->x[i]; }
  return s;
}

static inline void vref(vec4 *o, const vec4 *x, const vec4 *normal, double distance) {
  vmul(o, normal, 2 * (vdot(x, normal) - distance) / vdot(normal, normal));
  vsub(o, x, o);
}

static inline double vtime(const vec4 *x, const vec4 *y) {
  double c = 340.0 / 1000.0;
  vec4 v;
  vsub(&v, x, y);
  double d = vdot(&v, &v);
  return sqrt(d) / c;
}

typedef struct {
  double times[9];
  double decays[9];
  DELAY del;
  float delbuf[SR];
} EARLYREF;

static inline void early_ref_init(EARLYREF *eref, vec4 *size, vec4 *source, vec4 *listener) {
  vec4 normals[8] =
    { {{  1, 0, 0, 0 }}, {{ -1, 0, 0, 0 }}
    , {{ 0,  1, 0, 0 }}, {{ 0, -1, 0, 0 }}
    , {{ 0, 0,  1, 0 }}, {{ 0, 0, -1, 0 }}
    , {{ 0, 0, 0,  1 }}, {{ 0, 0, 0, -1 }}
    };
  double distance[8] =
    { 0, size->x[0], 0, size->x[1], 0, size->x[2], 0, size->x[3] };
  vec4 sources[9];
  vmul(&sources[0], source, 1);
  for (int i = 1; i < 9; ++i) {
    vref(&sources[i], source, &normals[i - 1], distance[i - 1]);
  }
  for (int i = 0; i < 9; ++i) {
    eref->times[i] = vtime(&sources[i], listener);
    eref->decays[i] = pow(10, -340 * eref->times[i] / 10000);
  }
  eref->del.length = SR;
}

static inline double early_ref(EARLYREF *eref, double audio, double brightness) {
  delwrite(&eref->del, audio);
  double s = 0;
  for (int i = 1; i < 9; ++i) {
    s += eref->decays[i] * delread1(&eref->del, eref->times[i]);
  }
  s *= brightness;
  s += eref->decays[0] * delread1(&eref->del, eref->times[0]);
  return s;
}

static inline int cmp_double(const void *a, const void *b) {
  const double *x = (const double *) a;
  const double *y = (const double *) b;
  if (*x > *y) { return 1; }
  if (*x < *y) { return -1; }
  return 0;
}

typedef struct {
  DELAY del;
  float delbuf[SR];
} REVERB_LINE;

typedef struct {
  double times[80];
  REVERB_LINE lines[16];
} REVERB;

static inline void reverb_init(REVERB *r, const vec4 *size) {
  double lx = size->x[0];
  double ly = size->x[1];
  double lz = size->x[2];
  double lw = size->x[3];
  int k = 0;
  for (int nx = 0; nx < 3; ++nx) {
  for (int ny = 0; ny < 3; ++ny) {
  for (int nz = 0; nz < 3; ++nz) {
  for (int nw = 0; nw < 3; ++nw) {
    if (nx + ny + nz + nw == 0) continue;
    r->times[k++] = 1000.0 / ((340.0/2.0) * sqrt((nx*nx)/(lx*lx)+(ny*ny)/(ly*ly)+(nz*nz)/(lz*lz)+(nw*nw)/(lw*lw)));
  }}}}
  qsort(r->times, 80, sizeof(double), cmp_double);
  // de-duplicate
  int wr = 0;
  int rd = 1;
  while (rd < 80) {
    if (r->times[wr] == r->times[rd]) {
      rd++;
    } else {
      r->times[wr] = r->times[rd];
      wr++;
      rd++;
    }
  }
  for (int i = 0; i < 16; ++i) {
    r->lines[i].del.length = SR;
  }
}

static inline void reverb(double *out, REVERB *r, const double *in, double t60) {
  const float mat[16][16] =
  { { 1, -1, -1, -1, -1, 1, 1, 1, -1, 1, 1, 1, -1, 1, 1, 1 }
  , { -1, 1, -1, -1, 1, -1, 1, 1, 1, -1, 1, 1, 1, -1, 1, 1 }
  , { -1, -1, 1, -1, 1, 1, -1, 1, 1, 1, -1, 1, 1, 1, -1, 1 }
  , { -1, -1, -1, 1, 1, 1, 1, -1, 1, 1, 1, -1, 1, 1, 1, -1 }
  , { -1, 1, 1, 1, 1, -1, -1, -1, -1, 1, 1, 1, -1, 1, 1, 1 }
  , { 1, -1, 1, 1, -1, 1, -1, -1, 1, -1, 1, 1, 1, -1, 1, 1 }
  , { 1, 1, -1, 1, -1, -1, 1, -1, 1, 1, -1, 1, 1, 1, -1, 1 }
  , { 1, 1, 1, -1, -1, -1, -1, 1, 1, 1, 1, -1, 1, 1, 1, -1 }
  , { -1, 1, 1, 1, -1, 1, 1, 1, 1, -1, -1, -1, -1, 1, 1, 1 }
  , { 1, -1, 1, 1, 1, -1, 1, 1, -1, 1, -1, -1, 1, -1, 1, 1 }
  , { 1, 1, -1, 1, 1, 1, -1, 1, -1, -1, 1, -1, 1, 1, -1, 1 }
  , { 1, 1, 1, -1, 1, 1, 1, -1, -1, -1, -1, 1, 1, 1, 1, -1 }
  , { -1, 1, 1, 1, -1, 1, 1, 1, -1, 1, 1, 1, 1, -1, -1, -1 }
  , { 1, -1, 1, 1, 1, -1, 1, 1, 1, -1, 1, 1, -1, 1, -1, -1 }
  , { 1, 1, -1, 1, 1, 1, -1, 1, 1, 1, -1, 1, -1, -1, 1, -1 }
  , { 1, 1, 1, -1, 1, 1, 1, -1, 1, 1, 1, -1, -1, -1, -1, 1 }
  };
  float source[16];
  float output[16];
  for (int i = 0; i < 16; ++i) {
    source[i] = 0.125 * in[i & 1] + delread1(&r->lines[i].del, r->times[i]) * pow(10, -3 * r->times[i] / fmax(t60, 1e-6));
  }
  for (int i = 0; i < 16; ++i) {
    float s = 0;
    for (int j = 0; j < 16; ++j) {
      s += mat[i][j] * source[j];
    }
    output[i] = 0.25 * s;
  }
  float s = 0;
  for (int i = 0; i < 8; ++i) {
    s += output[i];
  }
  s *= 0.125;
  out[0] = s;
  s = 0;
  for (int i = 8; i < 16; ++i) {
    s += output[i];
  }
  s *= 0.125;
  out[1] = s;
  for (int i = 0; i < 16; ++i) {
    delwrite(&r->lines[i].del, output[i]);
  }
}

#endif
