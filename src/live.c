#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef __x86_64__
#include <fenv.h>
#endif

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
  // set floating point environment for denormal->0.0
#ifdef __x86_64__
  fenv_t fe;
  fegetenv(&fe);
  unsigned int old_mxcsr = fe.__mxcsr;
  fe.__mxcsr |= 0x8040; // set DAZ and FTZ
  fesetenv(&fe);
#endif
  // get jack buffers
  jack_default_audio_sample_t *in [CHANNELS];
  jack_default_audio_sample_t *out[CHANNELS];
  for (int c = 0; c < CHANNELS; ++c) {
    in [c] = (jack_default_audio_sample_t *) jack_port_get_buffer(state.in [c], nframes);
    out[c] = (jack_default_audio_sample_t *) jack_port_get_buffer(state.out[c], nframes);
  }
  // get callback
  callback *f = state.func;
  // handle reloading
  if (state.reload) {
    int *reloaded = state.data;
    *reloaded = 1;
    state.reload = 0;
  }
  // loop over samples
  for (jack_nframes_t i = 0; i < nframes; ++i) {
    // to buffer
    float ini[CHANNELS];
    float outi[CHANNELS];
    for (int c = 0; c < CHANNELS; ++c) {
      ini [c] = in[c][i];
      outi[c] = 0;
    }
    // callback
    /* int inhibit_reload = */ f(state.data, CHANNELS, ini, outi);
    // from buffer
    for (int c = 0; c < CHANNELS; ++c) {
      out[c][i] = outi[c];
    }
  }
  // restore floating point environment
#ifdef __x86_64__
  fe.__mxcsr = old_mxcsr;
  fesetenv(&fe);
#endif
  // done
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
  // multi-channel visualization
  for (int c = 0; c < CHANNELS; ++c) {
    char srcport[100], dstport[100];
    snprintf(srcport, 100, "live:output_%d", c + 1);
    snprintf(dstport, 100, "visuals:input_%d", c + 1);
    if (jack_connect(state.client, srcport, dstport)) {
      fprintf(stderr, "cannot connect to visuals\n");
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
  // watch for filesystem changes
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
  ssize_t buf_bytes = sizeof(struct inotify_event) + NAME_MAX + 1;
  char *buf = malloc(buf_bytes);
  // double-buffering stuff
  void *old_dl = 0;
  void *new_dl = 0;
  int which = 0;
  // main loop
  while (1) {
/*
Double buffering to avoid glitches on reload.  Can't dlopen the same file
twice (or even symlinks thereof) due to aggressive caching seemingly based
on file names by libdl, so copy the target to one of two names.
To avoid crashing by unloading running code, open the other .so filename to the
runnning code before swapping pointers when hopefully not in a JACK process
callback in the DSP thread.  Finally dlclose the previous and swap the buffer
index.
*/
    const char *copycmd[2] = { "cp -f ./go.so ./go.a.so", "cp -f ./go.so ./go.b.so" };
    const char *library[2] = { "./go.a.so", "./go.b.so" };
    if (system(copycmd[which])) {
      fprintf(stderr, "\x1b[31;1mCOPY COMMAND FAILED: '%s'\x1b[0m\n", copycmd[which]);
    }
    if ((new_dl = dlopen(library[which], RTLD_NOW))) {
      callback *new_cb;
      *(void **) (&new_cb) = dlsym(new_dl, "go");
      if (new_cb) {
        // race mitigation: dlclose with jack running in .so -> boom
        while (inprocesscb) ;
        state.func = new_cb;
        state.reload = 1;
        // race mitigation
        while (inprocesscb) ;
        if (old_dl) {
          dlclose(old_dl);
        }
        old_dl = new_dl;
        new_dl = 0;
        which = 1 - which;
        fprintf(stderr, "\x1b[32;1mRELOADED: '%p'\x1b[0m\n", *(void **) (&new_cb));
      } else {
        fprintf(stderr, "\x1b[31;1mNO FUNCTION DEFINED: 'go()'\x1b[0m\n");
        dlclose(new_dl);
      }
    } else {
      // another race condition: the .so disappeared before load
      fprintf(stderr, "\x1b[31;1mFILE VANISHED: '%s'\x1b[0m\n", library[which]);
      const char *err = dlerror();
      if (err) {
        fprintf(stderr, "\x1b[31;1m%s\x1b[0m\n", err);
      }
    }
    // read events (blocking)
    int done = 0;
    do {
      memset(buf, 0, buf_bytes);
      ssize_t r = read(ino, buf, buf_bytes);
      if (r == -1) {
        perror("read()");
        sleep(1);
      } else {
        char *bufp = buf;
        while (bufp < buf + r)
        {
          struct inotify_event *ev = (struct inotify_event *) bufp;
          bufp += sizeof(struct inotify_event) + ev->len;
          if (ev->mask & IN_CLOSE_WRITE) {
            fprintf(stderr, "\x1b[32;1mFILE CHANGED: '%s'\x1b[0m\n", ev->name);
            if (0 == strcmp("go.so", ev->name)) {
              done = 1;
            }
            if (0 == strcmp("go.c", ev->name)) {
              // recompile
              int ret = system(
                "git add go.c ; "
                "git --no-pager diff --cached --color ; "
                "git commit -uno -m \"go $(date --iso=s)\" ; "
                "make --quiet"
              );
              if (ret == -1 || ! WIFEXITED(ret) || WEXITSTATUS(ret) != 0) {
                fprintf(stderr, "\x1b[31;1m%s: %d\x1b[0m\n", "SYSTEM ERROR", ret);
              }
            }
          }
        }
      }
    } while (! done);
  }
  // never reached
  close(ino);
  return 0;
}
