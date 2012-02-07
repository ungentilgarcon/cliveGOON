#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dlfcn.h>

#include <jack/jack.h>

typedef float callback(void *, float);

static float deffunc(void *data, float in) {
  return 0;
}

static struct {
  jack_client_t *client;
  jack_port_t *in, *out;
  void *data;
  callback * volatile func;
} state;

static int processcb(jack_nframes_t nframes, void *arg) {
  jack_default_audio_sample_t *in  = (jack_default_audio_sample_t *) jack_port_get_buffer(state.in,  nframes);
  jack_default_audio_sample_t *out = (jack_default_audio_sample_t *) jack_port_get_buffer(state.out, nframes);
  callback *f = state.func;
  for (jack_nframes_t i = 0; i < nframes; ++i) {
    out[i] = f(state.data, in[i]);
  }
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
  state.data = calloc(1, 1024 * 1024);
  jack_set_error_function(errorcb);
  if (!(state.client = jack_client_open("live", JackNoStartServer, 0))) {
    fprintf(stderr, "jack server not running?\n");
    return 1;
  }
  atexit(atexitcb);
  jack_set_process_callback(state.client, processcb, 0);
  jack_on_shutdown(state.client, shutdowncb, 0);
  state.in  = jack_port_register(state.client, "input_1",  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput,  0);
  state.out = jack_port_register(state.client, "output_1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  if (jack_activate(state.client)) {
    fprintf (stderr, "cannot activate JACK client");
    return 1;
  }
  const char **ports;
  if ((ports = jack_get_ports(state.client, NULL, NULL, JackPortIsPhysical | JackPortIsInput))) {
    int i = 0;
    while (ports[i] && i < 2) {
      if (jack_connect(state.client, jack_port_name(state.out), ports[i])) {
        fprintf(stderr, "cannot connect output port\n");
      }
      i++;
    }
    free(ports);
  }
  if ((ports = jack_get_ports(state.client, NULL, NULL, JackPortIsPhysical | JackPortIsOutput))) {
    int i = 0;
    while (ports[i] && i < 2) {
      if (jack_connect(state.client, ports[i], jack_port_name(state.in))) {
        fprintf(stderr, "cannot connect input port\n");
      }
      i++;
    }
    free(ports);
  }
  time_t old_time = 0;
  struct stat s;
  void *new_dl = 0;
  do {
    if (0 == stat("go.so", &s)) {
      if (s.st_mtime > old_time) {
        state.func = deffunc;
        if (new_dl) { dlclose(new_dl); new_dl = 0; }
        if ((new_dl = dlopen("./go.so", RTLD_NOW))) {
          callback *new_cb;
          *(void **) (&new_cb) = dlsym(new_dl, "go");
          if (new_cb) {
            fprintf(stderr, "HUP! %p\n", new_cb);
            state.func = new_cb;
            old_time = s.st_mtime;
          } else {
            fprintf(stderr, "no go()!\n");
          }
        } else {
          fprintf(stderr, "no go.so!\n");
        }
      } else {
        // no change
      }
    } else {
      fprintf(stderr, "no stat!\n");
    }
    sleep(1);
  } while (1);
  // never reached
  return 0;
}
