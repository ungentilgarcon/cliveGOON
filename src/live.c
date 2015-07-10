#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/limits.h>
#include <sys/inotify.h>
#include <dlfcn.h>

#include <jack/jack.h>

#define CHANNELS 2

// per-sample callback implemented in go.so
typedef int callback(void *, int, const float *, float *);

// default silent callback
static int deffunc(void *data, int channels, const float *in, float *out) {
  (void) data;
  (void) in;
  for (int c = 0; c < channels; ++c) {
    out[c] = 0;
  }
  return 0;
}

static struct {
  jack_client_t *client;
  jack_port_t *in[CHANNELS], *out[CHANNELS];
  void *data;
  callback * volatile func;
  int volatile reload;
} state;

// race mitigation
volatile int inprocesscb = 0;

static int processcb(jack_nframes_t nframes, void *arg) {
  inprocesscb = 1; // race mitigation
  jack_default_audio_sample_t *in [CHANNELS];
  jack_default_audio_sample_t *out[CHANNELS];
  for (int c = 0; c < CHANNELS; ++c) {
    in [c] = (jack_default_audio_sample_t *) jack_port_get_buffer(state.in [c], nframes);
    out[c] = (jack_default_audio_sample_t *) jack_port_get_buffer(state.out[c], nframes);
  }
  callback *f = state.func;
  if (state.reload) {
    int *reloaded = state.data;
    *reloaded = 1;
    state.reload = 0;
  }
  for (jack_nframes_t i = 0; i < nframes; ++i) {
    float ini[CHANNELS];
    float outi[CHANNELS];
    for (int c = 0; c < CHANNELS; ++c) {
      ini [c] = in[c][i];
      outi[c] = 0;
    }
    /* int inhibit_reload = */ f(state.data, CHANNELS, ini, outi);
    for (int c = 0; c < CHANNELS; ++c) {
      out[c][i] = outi[c];
    }
  }
  inprocesscb = 0; // race mitigation
  return 0;
}

static void errorcb(const char *desc) {
  fprintf(stderr, "JACK error: %s\n", desc);
}

static void shutdowncb(void *arg) {
  exit(1);
}

static void atexitcb(void) {
  jack_client_close(state.client);
}

int main(int argc, char **argv) {
  srand(time(0));
  state.func = deffunc;
  state.data = calloc(1, 64 * 1024 * 1024);
  state.reload = 0;
  jack_set_error_function(errorcb);
  if (!(state.client = jack_client_open("live", JackNoStartServer, 0))) {
    fprintf(stderr, "jack server not running?\n");
    return 1;
  }
  atexit(atexitcb);
  jack_set_process_callback(state.client, processcb, 0);
  jack_on_shutdown(state.client, shutdowncb, 0);
  // multi-channel processing
  for (int c = 0; c < CHANNELS; ++c) {
    char portname[100];
    snprintf(portname, 100, "input_%d",  c + 1);
    state.in[c]  = jack_port_register(state.client, portname, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput,  0);
    snprintf(portname, 100, "output_%d", c + 1);
    state.out[c] = jack_port_register(state.client, portname, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  }
  if (jack_activate(state.client)) {
    fprintf (stderr, "cannot activate JACK client");
    return 1;
  }
  // multi-channel recording
  for (int c = 0; c < CHANNELS; ++c) {
    char srcport[100], dstport[100];
    snprintf(srcport, 100, "live:output_%d", c + 1);
    snprintf(dstport, 100, "record:in_%d",   c + 1);
    if (jack_connect(state.client, srcport, dstport)) {
      fprintf(stderr, "cannot connect to recorder\n");
    }
  }
  // multi-channel output
  const char **ports;
  if ((ports = jack_get_ports(state.client, NULL, NULL, JackPortIsPhysical | JackPortIsInput))) {
    int c = 0;
    while (ports[c] && c < CHANNELS) {
      if (jack_connect(state.client, jack_port_name(state.out[c]), ports[c])) {
        fprintf(stderr, "cannot connect output port\n");
      }
      c++;
    }
    free(ports);
  }
  // multi_channel input
  if ((ports = jack_get_ports(state.client, NULL, NULL, JackPortIsPhysical | JackPortIsOutput))) {
    int c = 0;
    while (ports[c] && c < CHANNELS) {
      if (jack_connect(state.client, ports[c], jack_port_name(state.in[c]))) {
        fprintf(stderr, "cannot connect input port\n");
      }
      c++;
    }
    free(ports);
  }
  void *new_dl = 0;
  while (1) {
    // race mitigation: dlclose with jack running in .so -> boom
    while (inprocesscb) ;
    state.func = deffunc;
    // must dlclose first, or the old .so is cached despite having changed
    if (new_dl) { dlclose(new_dl); new_dl = 0; }
    if ((new_dl = dlopen("./go.so", RTLD_NOW))) {
      callback *new_cb;
      *(void **) (&new_cb) = dlsym(new_dl, "go");
      if (new_cb) {
        state.func = new_cb;
        state.reload = 1;
        fprintf(stderr, "HUP! %p\n", *(void **) (&new_cb));
      } else {
        fprintf(stderr, "no go()!\n");
      }
    } else {
      // another race condition: the .so disappeared before load
      fprintf(stderr, "no go.so!\n");
    }
    // watch .so for changes
    int ino = inotify_init();
    if (ino == -1) {
      perror("inotify_init()");
      return 1;
    }
    int wd = inotify_add_watch(ino, ".", IN_CLOSE_WRITE);
    if (wd == -1) {
      perror("inotify_add_watch()");
      return 1;
    }
    // read events (blocking)
    struct { struct inotify_event ev; char name[NAME_MAX + 1]; } buf;
    int done = 0;
    do {
      ssize_t r = read(ino, &buf, sizeof(buf));
      if (r == -1) {
        perror("read()");
        sleep(1);
      } else {
        if (r >= (ssize_t) sizeof(buf.ev) + buf.ev.len) {
          if (buf.ev.mask & IN_CLOSE_WRITE) {
            fprintf(stderr, "%s\n", buf.name);
            if (0 == strcmp("go.so", buf.name)) {
              done = 1;
            }
          }
          if (r > (ssize_t) sizeof(buf.ev) + buf.ev.len) {
            fprintf(stderr, "extra event!\n");
          }
        }
      }
    } while (! done);
    close(ino);
  }
  // never reached
  return 0;
}
