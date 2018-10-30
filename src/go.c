#define SR 48000
#include "dsp.h"
typedef struct {
int reloaded;
PHASOR clock, osc;
} S;
int go(S *s, int channels, const float *in, float *out) {
if (s->reloaded) { s->reloaded = 0; }
double env = phasor(&s->clock, 125/60.0) < 0.25;
double osc = sin(twopi * phasor(&s->osc, 440));
double o = env * osc;
for (int c = 0; c < channels; ++c) { out[c] = o; }
return 0;
}
