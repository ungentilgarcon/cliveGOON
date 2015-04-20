#define SR 48000
#include "dsp.h"

typedef struct {
  float like_a_butterfly;
} S;

int go(S *s, int channels, const float *in, float *out) {
  for (int c = 0; c < channels; ++c) {
    out[c] = 0;
  }
  return 0;
}
