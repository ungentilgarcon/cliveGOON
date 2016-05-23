#define _GNU_SOURCE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <libv4l2.h>
#include <linux/videodev2.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

unsigned char *webcam_buffer = 0;

GLint u_text_size;
int text_buffer_width = 80, text_buffer_height = 25;
int text_width = 40, text_height = 14;
char text_buffer[25][80] = {
"                                                                                ",
" diff --git a/src/go.c b/src/go.c                                               ",
" index 415b0c0..aec4684 100644                                                  ",
" --- a/src/go.c                                                                 ",
" +++ b/src/go.c                                                                 ",
" @@ -10,6 +10,7 @@ typedef struct {                                             ",
"    COMPRESS compress;                                                          ",
"    VCF snare;                                                                  ",
"    VCF baff[2][8];                                                             ",
" +  double volume;                                                              ",
"  } S;                                                                          ",
"                                                                                ",
"  int go(S *s, int channels, const float *in, float *out) {                     ",
"                                                                                ",
"                                                                                ",
"                                                                                ",
"                                                                                ",
"                                                                                ",
"                                                                                ",
"                                                                                ",
"                                                                                ",
"                                                                                ",
"                                                                                ",
"                                                                                ",
"                                                                                "
};

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
"  vec2 glyph_coord1 = (glyph_coord + index1) * vec2(1.0 / 16.0, 1.0 / 6.0);\n"
"  vec3 base = vec3(0.5);\n"
"  if (line_glyph == 0) { base = vec3(1.0); } else\n"
"  if (line_glyph == 32) { base = vec3(0.5, 0.5, 1.0); } else\n"
"  if (line_glyph == 11) { base = vec3(0.5, 1.0, 0.5); } else\n"
"  if (line_glyph == 13) { base = vec3(1.0, 0.5, 0.5); }\n"
"  colour = vec4(base\n"
"         * textureGrad(ascii, glyph_coord, dFdx(glyph_coord1), dFdy(glyph_coord1)).r\n"
"         + texture(webcam, webcam_coord).rgb, 2.0) * 0.5;\n"
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

void initialize_gl(int screen_width, int screen_height, int webcam_width, int webcam_height) {
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
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, text_buffer_width, text_buffer_height, 0, GL_RED, GL_UNSIGNED_BYTE, text_buffer);
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
  int webcam = v4l2_open(dev, O_RDWR);
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
  webcam_buffer = malloc(*width * *height * 3);
  return webcam;
}

int main(int argc, char **argv) {
  (void) argc;
  (void) argv;
  int screen_width = 1280, screen_height = 720;
  int webcam_width = 0, webcam_height = 0;
  int webcam = initialize_webcam("/dev/video0", &webcam_width, &webcam_height);
  if (webcam < 0) {
    return 1;
  }
  GLFWwindow *window = create_context(screen_width, screen_height);
  if (! window) {
    v4l2_close(webcam);
    free(webcam_buffer);
    return 1;
  }
  glfwSetKeyCallback(window, key_press_handler);
  initialize_gl(screen_width, screen_height, webcam_width, webcam_height);
  while (! glfwWindowShouldClose(window)) {
    // read webcam
    v4l2_read(webcam, webcam_buffer, webcam_width * webcam_height * 3);
    glActiveTexture(GL_TEXTURE0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, webcam_width, webcam_height, GL_RGB, GL_UNSIGNED_BYTE, webcam_buffer);
    glGenerateMipmap(GL_TEXTURE_2D);
    // read git diff
    FILE *diff_file = popen("git diff HEAD~1", "r");
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
        text_buffer[y][x] = line[x - 1];
        text_width = text_width > x ? text_width : x;
      }
      text_height = y;
    }
    text_width += 1;
    text_height += 1;
    free(line);
    pclose(diff_file);
    glActiveTexture(GL_TEXTURE1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, text_buffer_width, text_buffer_height, GL_RED, GL_UNSIGNED_BYTE, text_buffer);
    glUniform2f(u_text_size, text_width, text_height);
    // draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwTerminate();
  v4l2_close(webcam);
  free(webcam_buffer);
  return 0;
}
