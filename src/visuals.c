#define _GNU_SOURCE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <libv4l2.h>
#include <linux/videodev2.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

unsigned char *webcam_buffer = 0;
struct webcam_buffer {
  void *start;
  size_t length;
};
struct webcam_buffer *webcam_buffers;

GLint u_text_size;

const char *text_vert =
"#version 330 core\n"
"uniform vec2 screen_size;\n"
"in vec2 pos;\n"
"in vec2 tc;\n"
"out vec2 coord;\n"
"void main() {\n"
"  gl_Position = vec4(pos, 0.0, 1.0);\n"
"  coord = screen_size * tc;\n"
"}\n"
;

const char *text_frag =
"#version 330 core\n"
"uniform sampler2D webcam;\n"
"uniform sampler2D text;\n"
"uniform sampler2D ascii;\n"
"uniform vec2 screen_size;\n"
"uniform vec2 text_size;\n"
"const float fly_factor = 1.0;\n"
"in vec2 coord;\n"
"out vec4 colour;\n"
"void main() {\n"
"  float screen_aspect = screen_size.x / screen_size.y;\n"
"  float text_aspect = text_size.x / text_size.y;\n"
"  float webcam_aspect = float(textureSize(webcam, 0).x) / float(textureSize(webcam, 0).y);\n"
"  vec2 webcam_coord = coord / screen_size;\n"
"  webcam_coord -= vec2(0.5);\n"
"  if (webcam_aspect < screen_aspect) {\n"
"    webcam_coord.y *= webcam_aspect / screen_aspect;\n"
"  } else {\n"
"    webcam_coord.x /= webcam_aspect / screen_aspect;\n"
"  }\n"
"  webcam_coord += vec2(0.5);\n"
"  vec2 webcam_coord1 = webcam_coord;\n"
"  float r = sqrt(3.0)/2.0;\n"
"  webcam_coord.y *= r;\n"
"  webcam_coord1.y *= r;\n"
"  vec2 middle = round(24 * vec2(webcam_coord.x + webcam_coord.y / 2.0, webcam_coord.y));\n"
"  middle.x -= middle.y / 2.0;\n"
"  vec2 middle0 = middle;\n"
"  vec2 middle1 = middle + vec2(1.0, 0.0);\n"
"  vec2 middle2 = middle + vec2(-1.0, 0.0);\n"
"  vec2 middle3 = middle + vec2(-0.5, 1.0);\n"
"  vec2 middle4 = middle + vec2(0.5, 1.0);\n"
"  vec2 middle5 = middle + vec2(-0.5, -1.0);\n"
"  vec2 middle6 = middle + vec2(0.5, -1.0);\n"
"  middle0 /= 24.0;\n"
"  middle1 /= 24.0;\n"
"  middle2 /= 24.0;\n"
"  middle3 /= 24.0;\n"
"  middle4 /= 24.0;\n"
"  middle5 /= 24.0;\n"
"  middle6 /= 24.0;\n"
"  float d = 1.0/0.0;\n"
"  if (distance(webcam_coord, middle0) < d) {\n"
"    d = distance(webcam_coord, middle0);\n"
"    middle = middle0;\n"
"  }\n"
"  if (distance(webcam_coord, middle1) < d) {\n"
"    d = distance(webcam_coord, middle1);\n"
"    middle = middle1;\n"
"  }\n"
"  if (distance(webcam_coord, middle2) < d) {\n"
"    d = distance(webcam_coord, middle2);\n"
"    middle = middle2;\n"
"  }\n"
"  if (distance(webcam_coord, middle3) < d) {\n"
"    d = distance(webcam_coord, middle3);\n"
"    middle = middle3;\n"
"  }\n"
"  if (distance(webcam_coord, middle4) < d) {\n"
"    d = distance(webcam_coord, middle4);\n"
"    middle = middle4;\n"
"  }\n"
"  if (distance(webcam_coord, middle5) < d) {\n"
"    d = distance(webcam_coord, middle5);\n"
"    middle = middle5;\n"
"  }\n"
"  if (distance(webcam_coord, middle6) < d) {\n"
"    d = distance(webcam_coord, middle6);\n"
"    middle = middle6;\n"
"  }\n"
"  webcam_coord = webcam_coord + fly_factor * (webcam_coord - middle);\n"
"  webcam_coord1 = webcam_coord1 + fly_factor * webcam_coord1;\n"
"  webcam_coord.y /= r;\n"
"  webcam_coord1.y /= r;\n"
"  vec3 webcam_colour = textureGrad(webcam, webcam_coord, dFdx(webcam_coord1), dFdy(webcam_coord1)).rgb;\n"
"  float scale;\n"
"  vec2 offset = vec2(0.0);\n"
"  if (text_aspect < screen_aspect) {\n"
"    scale = text_size.y / screen_size.y;\n"
"    offset = vec2((scale * screen_size.x - text_size.x) / 2.0, 0.0);\n"
"  } else {\n"
"    scale = text_size.x / screen_size.x;\n"
"    offset = vec2(0.0, (scale * screen_size.y - text_size.y) / 2.0);\n"
"  }\n"
"  vec2 index = scale * coord - offset;\n"
"  ivec2 ix = ivec2(int(floor(index.x)), int(floor(index.y)));\n"
"  vec2 index1 = index;\n"
"  index -= vec2(ix);\n"
"  int glyph      = max(0, int(round(255.0 * texelFetch(text, ix, 0).r)) - 32);\n"
"  int line_glyph = max(0, int(round(255.0 * texelFetch(text, ivec2(1, ix.y), 0).r)) - 32);\n"
"  vec2 glyph_coord = vec2(float(glyph % 16), float(glyph / 16));\n"
"  glyph_coord += index;\n"
"  glyph_coord *= vec2(1.0 / 16.0, 1.0 / 6.0);\n"
"  vec2 glyph_coord1 = index1 * vec2(1.0 / 16.0, 1.0 / 6.0);\n"
"  vec3 base = vec3(0.5);\n"
"  if (line_glyph == 0) { base = vec3(1.0); } else\n"
"  if (line_glyph == 32) { base = vec3(0.5, 0.5, 1.0); } else\n"
"  if (line_glyph == 11) { base = vec3(0.5, 1.0, 0.5); } else\n"
"  if (line_glyph == 13) { base = vec3(1.0, 0.5, 0.5); }\n"
"  colour = vec4(base\n"
"         * textureGrad(ascii, glyph_coord, dFdx(glyph_coord1), dFdy(glyph_coord1)).r\n"
"         + webcam_colour, 2.0) * 0.5;\n"
"}\n"
;

void debug_program(GLuint program, const char *name) {
  if (program) {
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
      fprintf(stderr, "%s: OpenGL shader program link failed\n", name);
    }
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    char *buffer = (char *) malloc(length + 1);
    glGetProgramInfoLog(program, length, 0, buffer);
    buffer[length] = 0;
    if (buffer[0]) {
      fprintf(stderr, "%s: OpenGL shader program info log\n", name);
      fprintf(stderr, "%s\n", buffer);
    }
    free(buffer);
  } else {
    fprintf(stderr, "%s: OpenGL shader program creation failed\n", name);
  }
}

void debug_shader(GLuint shader, GLenum type, const char *name) {
  const char *tname = 0;
  switch (type) {
    case GL_VERTEX_SHADER:   tname = "vertex";   break;
    case GL_FRAGMENT_SHADER: tname = "fragment"; break;
    default:                 tname = "unknown";  break;
  }
  if (shader) {
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
      fprintf(stderr, "%s: OpenGL %s shader compile failed\n", name, tname);
    }
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    char *buffer = (char *) malloc(length + 1);
    glGetShaderInfoLog(shader, length, 0, buffer);
    buffer[length] = 0;
    if (buffer[0]) {
      fprintf(stderr, "%s: OpenGL %s shader info log\n", name, tname);
      fprintf(stderr, "%s\n", buffer);
    }
    free(buffer);
  } else {
    fprintf(stderr, "%s: OpenGL %s shader creation failed\n", name, tname);
  }
}

void compile_shader(GLint program, GLenum type, const char *name, const GLchar *source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, 0);
  glCompileShader(shader);
  debug_shader(shader, type, name);
  glAttachShader(program, shader);
  glDeleteShader(shader);
}

GLint compile_program(const char *name, const GLchar *vert, const GLchar *frag) {
  GLint program = glCreateProgram();
  if (vert) { compile_shader(program, GL_VERTEX_SHADER  , name, vert); }
  if (frag) { compile_shader(program, GL_FRAGMENT_SHADER, name, frag); }
  glLinkProgram(program);
  debug_program(program, name);
  return program;
}

void key_press_handler(GLFWwindow *window, int key, int scancode, int action, int mods) {
  (void) scancode;
  (void) mods;
  if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_Q: case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GL_TRUE); break;
    }
  }
}

GLFWwindow *create_window(int major, int minor, int width, int height) {
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  GLFWwindow *window = glfwCreateWindow(width, height, "clive", 0, 0);
  if (! window) {
    fprintf(stderr, "couldn't create window with OpenGL core %d.%d context\n", major, minor);
  }
  return window;
}

GLFWwindow *create_context(int width, int height) {
  if (! glfwInit()) {
    fprintf(stderr, "couldn't initialize glfw\n");
    return 0;
  }
  int major, minor;
  GLFWwindow *window = create_window(major = 3, minor = 3, width, height);
  if (! window) {
    fprintf(stderr, "couldn't create window\n");
    return 0;
  }
  glfwMakeContextCurrent(window);
  glewExperimental = GL_TRUE;
  glewInit();
  glGetError();
  return window;
}

void initialize_gl(int screen_width, int screen_height, int webcam_width, int webcam_height, int text_buffer_width, int text_buffer_height) {
  GLuint program = compile_program("text", text_vert, text_frag);
  glUseProgram(program);
  GLint ipos = glGetAttribLocation(program, "pos");
  GLint itc = glGetAttribLocation(program, "tc");
  GLint u_screen_size = glGetUniformLocation(program, "screen_size");
  u_text_size = glGetUniformLocation(program, "text_size");
  GLint u_webcam = glGetUniformLocation(program, "webcam");
  GLint u_ascii = glGetUniformLocation(program, "ascii");
  GLint u_text = glGetUniformLocation(program, "text");
  glUniform1i(u_webcam, 0);
  glUniform1i(u_text, 1);
  glUniform1i(u_ascii, 2);
  glUniform2f(u_screen_size, screen_width, screen_height);
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  GLfloat vbo_data[] =
    { -1, -1, 0, 1
    , -1,  1, 0, 0
    ,  1, -1, 1, 1
    ,  1,  1, 1, 0
    };
  glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), vbo_data, GL_STATIC_DRAW);
  glVertexAttribPointer(ipos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
  glVertexAttribPointer(itc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), ((char *)0) + 2 * sizeof(GLfloat));
  glEnableVertexAttribArray(ipos);
  glEnableVertexAttribArray(itc);
  glActiveTexture(GL_TEXTURE0);
  GLuint t_webcam = 0;
  glGenTextures(1, &t_webcam);
  glBindTexture(GL_TEXTURE_2D, t_webcam);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, webcam_width, webcam_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glActiveTexture(GL_TEXTURE1);
  GLuint t_text = 0;
  glGenTextures(1, &t_text);
  glBindTexture(GL_TEXTURE_2D, t_text);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, text_buffer_width, text_buffer_height, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glActiveTexture(GL_TEXTURE2);
  GLuint t_ascii = 0;
  glGenTextures(1, &t_ascii);
  glBindTexture(GL_TEXTURE_2D, t_ascii);
  unsigned char *ascii_data = malloc(2048 * 768);
  FILE *ascii_file = fopen("ascii.pgm", "rb");
  fscanf(ascii_file, "P5\n2048 768\n255");
  int c = fgetc(ascii_file);
  assert(c == '\n');
  fread(ascii_data, 2048 * 768, 1, ascii_file);
  fclose(ascii_file);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 2048, 768, 0, GL_RED, GL_UNSIGNED_BYTE, ascii_data);
  free(ascii_data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

int initialize_webcam(const char *dev, int *width, int *height) {
  int webcam = v4l2_open(dev, O_RDWR | O_NONBLOCK, 0);
  struct v4l2_capability cap;
  memset(&cap, 0, sizeof(cap));
  if (-1 == v4l2_ioctl(webcam, VIDIOC_QUERYCAP, &cap)) {
    fprintf(stderr, "not a video device\n");
    exit(1);
  }
  if (! (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    fprintf(stderr, "not a video capture device\n");
    exit(1);
  }
  if (! (cap.capabilities & V4L2_CAP_READWRITE)) {
    fprintf(stderr, "no support for read i/o\n");
    exit(1);
  }
  struct v4l2_format format;
  memset(&format, 0, sizeof(format));
  format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (-1 == v4l2_ioctl(webcam, VIDIOC_G_FMT, &format)) {
    fprintf(stderr, "couldn't get video format\n");
    exit(1);
  }
  *width = format.fmt.pix.width;
  *height = format.fmt.pix.height;
  format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
  if (-1 == v4l2_ioctl(webcam, VIDIOC_S_FMT, &format)) {
    fprintf(stderr, "couldn't set video format\n");
    exit(1);
  }
  struct v4l2_requestbuffers req;
  memset(&req, 0, sizeof(req));
  req.count = 4;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  if (-1 == v4l2_ioctl(webcam, VIDIOC_REQBUFS, &req)) {
    fprintf(stderr, "couldn't set video reqbufs\n");
    exit(1);
  }
  webcam_buffers = calloc(req.count, sizeof(*webcam_buffers));
  struct v4l2_buffer buf;
  unsigned int n_buffers;
  for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
    memset(&buf, 0, sizeof(buf));
    buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory      = V4L2_MEMORY_MMAP;
    buf.index       = n_buffers;
    if (-1 == v4l2_ioctl(webcam, VIDIOC_QUERYBUF, &buf)) {
      fprintf(stderr, "couldn't query video buffer\n");
      exit(1);
    }
    webcam_buffers[n_buffers].length = buf.length;
    webcam_buffers[n_buffers].start = v4l2_mmap(0, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, webcam, buf.m.offset);
    if (MAP_FAILED == webcam_buffers[n_buffers].start) {
      fprintf(stderr, "couldn't mmap video buffer\n");
      exit(1);
    }
  }
  for (unsigned int i = 0; i < n_buffers; ++i) {
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    if (-1 == v4l2_ioctl(webcam, VIDIOC_QBUF, &buf)) {
      fprintf(stderr, "couldn't queue video buffer\n");
      exit(1);
    }
  }
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (-1 == v4l2_ioctl(webcam, VIDIOC_STREAMON, &type)) {
    fprintf(stderr, "couldn't start video streaming\n");
    exit(1);
  }
  webcam_buffer = malloc(*width * *height * 3);
  return webcam;
}

int read_webcam(int webcam, int width, int height) {
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(webcam, &fds);
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 1000000 / 60;
  int r = select(webcam + 1, &fds, 0, 0, &tv);
  if (r != 1) {
    return 0;
  }
  struct v4l2_buffer buf;
  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  if (-1 == v4l2_ioctl(webcam, VIDIOC_DQBUF, &buf)) {
    return 0;
  }
  memcpy(webcam_buffer, webcam_buffers[buf.index].start, width * height * 3);
  v4l2_ioctl(webcam, VIDIOC_QBUF, &buf);
  return 1;
}

void deinitialize_webcam(int webcam) {
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (-1 == v4l2_ioctl(webcam, VIDIOC_STREAMOFF, &type)) {
    fprintf(stderr, "couldn't stop video streaming\n");
    exit(1);
  }
  for (int i = 0; i < 4; ++i) {
    v4l2_munmap(webcam_buffers[i].start, webcam_buffers[i].length);
  }
  v4l2_close(webcam);
  free(webcam_buffer);
}

int main(int argc, char **argv) {
  (void) argc;
  (void) argv;
  int text_buffer_width = 128, text_buffer_height = 64;
  char *text_buffer = malloc(text_buffer_width * text_buffer_height);
  int screen_width = 1280, screen_height = 720;
  int webcam_width = 0, webcam_height = 0;
  int webcam = initialize_webcam("/dev/video0", &webcam_width, &webcam_height);
  if (webcam < 0) {
    return 1;
  }
  GLFWwindow *window = create_context(screen_width, screen_height);
  if (! window) {
    return 1;
  }
  glfwSetKeyCallback(window, key_press_handler);
  initialize_gl(screen_width, screen_height, webcam_width, webcam_height, text_buffer_width, text_buffer_height);
  while (! glfwWindowShouldClose(window)) {
    // read webcam
    if (read_webcam(webcam, webcam_width, webcam_height)) {
      glActiveTexture(GL_TEXTURE0);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, webcam_width, webcam_height, GL_RGB, GL_UNSIGNED_BYTE, webcam_buffer);
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    // read git diff
    FILE *diff_file = popen("git diff HEAD~1", "r");
    if (diff_file) {
      memset(text_buffer, ' ', text_buffer_width * text_buffer_height);
      int text_width = 0;
      int text_height = 0;
      char *line = 0;
      size_t len = 0;
      ssize_t read;
      for (int y = 1; y < text_buffer_height - 1; ++y) {
        if ((read = getline(&line, &len, diff_file)) == -1) {
          break;
        }
        for (int x = 1; x < text_buffer_width - 1 && x < read; ++x) {
          text_buffer[y * text_buffer_width + x] = line[x - 1];
          text_width = text_width > x ? text_width : x;
        }
        text_height = y;
      }
      text_width += 2;
      text_height += 2;
      free(line);
      pclose(diff_file);
      glActiveTexture(GL_TEXTURE1);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, text_buffer_width, text_buffer_height, GL_RED, GL_UNSIGNED_BYTE, text_buffer);
      glUniform2f(u_text_size, text_width, text_height);
    }
    // draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwTerminate();
  deinitialize_webcam(webcam);
  free(text_buffer);
  return 0;
}
