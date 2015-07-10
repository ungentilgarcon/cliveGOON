#define SR 48000
#include "dsp.h"

typedef struct {
  int reloaded;
} S;

int go(S *s, int channels, const float *in, float *out) {
  if (s->reloaded) {
    s->reloaded = 0;
  }
  for (int c = 0; c < channels; ++c) {
    out[c] = 0;
  }
  return 0;
}
