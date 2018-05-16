#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "chip8.h"

const char *vertex_shader_glsl =
  "#version 410 core\n"
  "in vec2 pos;\n"
  "void main() {\n"
  " gl_Position = vec4(pos, 0.0, 1.0);\n"
  "}";
const char *fragment_shader_glsl =
  "#version 410 core\n"
  "out vec4 color;\n"
  "void main() {\n"
  "  color = vec4(0.85, 0.85, 0.85, 1.0);\n"
  "}";

static size_t fill_vertices_to_draw(chip8 *, GLuint *);
static void key_handler(GLFWwindow *, int, int, int, int);
static void resize_handler(GLFWwindow *, GLsizei, GLsizei);
static GLuint *gl_setup(void);

static void errorf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, fmt, ap);
  va_end(ap);
}

static void *emalloc(size_t size)
{
  void *p = malloc(size);
  if (p) {
    return p;
  }
  errorf("malloc failed\n");
  exit(EXIT_FAILURE);
}

chip8 *c8;

int main(int argc, char **argv)
{
  if (argc != 2) {
    errorf("Usage: %s <CHIP-8 ROM>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

  GLsizei width  = DISPLAY_WIDTH  * RENDER_SCALE;
  GLsizei height = DISPLAY_HEIGHT * RENDER_SCALE;
  GLFWwindow *window = glfwCreateWindow(width, height, "CHIP-8", NULL, NULL);
  if (!window) {
    goto fail;
  }

  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, key_handler);
  glfwSetWindowSizeCallback(window, resize_handler);
  resize_handler(window, width, height);

  c8 = chip8_init();
  if (!c8 || !chip8_load_rom(c8, argv[1])) {
    goto fail;
  }

  GLuint *vertex = gl_setup();
  if (!vertex) {
    goto fail;
  }

  glClearColor(.1, .1, .1, 0);
  while (!glfwWindowShouldClose(window)) {
    chip8_emulate_cycle(c8, glfwWaitEvents);
    if (c8->draw_flag) {
      glClear(GL_COLOR_BUFFER_BIT);
      size_t n = fill_vertices_to_draw(c8, vertex);
      glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, n*sizeof(*vertex), vertex);
      glDrawElements(GL_TRIANGLES, n, GL_UNSIGNED_INT, 0);
      glfwSwapBuffers(window);
    }
    glfwPollEvents();
  }

  chip8_destroy(c8);
  glfwTerminate();
  return 0;

fail:
  glfwTerminate();
  exit(EXIT_FAILURE);
}

static size_t fill_vertices_to_draw(chip8 *c8, GLuint *vertex)
{
  size_t h = DISPLAY_HEIGHT + 1;
  size_t n = 0;
  for (size_t x = 0; x < DISPLAY_WIDTH; ++x) {
    for (size_t y = 0; y < DISPLAY_HEIGHT; ++y) {
      if (c8->gfx[x][y] == 1) {
        /* Corners of quad */
        GLuint q1, q2, q3, q4;
        q1 = x*h + y;
        q2 = x*h + y + 1;
        q3 = (x+1)*h + y;
        q4 = (x+1)*h + y + 1;
        vertex[n+0] = q1;
        vertex[n+1] = q2;
        vertex[n+2] = q3;
        vertex[n+3] = q2;
        vertex[n+4] = q3;
        vertex[n+5] = q4;
        n += 6;
      }
    }
  }
  return n; /* Number of vertices */
}

static void
key_handler(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  /*
   * Keypad         Keyboard
   * |1|2|3|C|      |1|2|3|4|
   * |4|5|6|D|  =>  |Q|W|E|R|
   * |7|8|9|E|      |A|S|D|F|
   * |A|0|B|F|      |Z|X|C|V|
   */
  switch (action) {
  case GLFW_PRESS:
    switch (key) {
    case GLFW_KEY_1: c8->key[0x1] = true; break;
    case GLFW_KEY_2: c8->key[0x2] = true; break;
    case GLFW_KEY_3: c8->key[0x3] = true; break;
    case GLFW_KEY_4: c8->key[0xC] = true; break;
    case GLFW_KEY_Q: c8->key[0x4] = true; break;
    case GLFW_KEY_W: c8->key[0x5] = true; break;
    case GLFW_KEY_E: c8->key[0x6] = true; break;
    case GLFW_KEY_R: c8->key[0xD] = true; break;
    case GLFW_KEY_A: c8->key[0x7] = true; break;
    case GLFW_KEY_S: c8->key[0x8] = true; break;
    case GLFW_KEY_D: c8->key[0x9] = true; break;
    case GLFW_KEY_F: c8->key[0xE] = true; break;
    case GLFW_KEY_Z: c8->key[0xA] = true; break;
    case GLFW_KEY_X: c8->key[0x0] = true; break;
    case GLFW_KEY_C: c8->key[0xB] = true; break;
    case GLFW_KEY_V: c8->key[0xF] = true; break;
    case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GL_TRUE); break;
    }
    break;
  case GLFW_RELEASE:
    switch (key) {
    case GLFW_KEY_1: c8->key[0x1] = false; break;
    case GLFW_KEY_2: c8->key[0x2] = false; break;
    case GLFW_KEY_3: c8->key[0x3] = false; break;
    case GLFW_KEY_4: c8->key[0xC] = false; break;
    case GLFW_KEY_Q: c8->key[0x4] = false; break;
    case GLFW_KEY_W: c8->key[0x5] = false; break;
    case GLFW_KEY_E: c8->key[0x6] = false; break;
    case GLFW_KEY_R: c8->key[0xD] = false; break;
    case GLFW_KEY_A: c8->key[0x7] = false; break;
    case GLFW_KEY_S: c8->key[0x8] = false; break;
    case GLFW_KEY_D: c8->key[0x9] = false; break;
    case GLFW_KEY_F: c8->key[0xE] = false; break;
    case GLFW_KEY_Z: c8->key[0xA] = false; break;
    case GLFW_KEY_X: c8->key[0x0] = false; break;
    case GLFW_KEY_C: c8->key[0xB] = false; break;
    case GLFW_KEY_V: c8->key[0xF] = false; break;
    }
    break;
  }
}

static void resize_handler(GLFWwindow *window, GLsizei w, GLsizei h)
{
  glViewport(0, 0, w, h);
}

static bool shader_error_occurred(GLuint shader)
{
  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE) {
    GLint length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (length) {
      GLchar *buf = emalloc(length);
      glGetShaderInfoLog(shader, length, NULL, buf);
      errorf("Shader error: %s\n", buf);
      free(buf);
    } else {
      errorf("Shader error\n");
    }
    return true;
  }
  return false;
}

static GLuint *gl_setup(void)
{
  GLenum err = glewInit();
  if (err != GLEW_OK) {
    errorf("GLEW error: %s\n", glewGetErrorString(err));
    return NULL;
  }

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  /*
   * Generate quad vertices.
   *
   * See the display pictured below. The vertices are numbered starting
   * from the top left and going down, proceeding right after the last row is
   * reached. The vertex at position (x,y) is numbered 33*x+y:
   *   - (0,0) is vertex 0
   *   - (0,1) is vertex 1
   *   - (1,0) is vertex 33
   *   - etc.
   *
   * The numbering is chosen to match the layout of chip8.gfx.
   *
   *      x  0 1     ...      64
   *      --->
   *  y |
   *    |  +---------------------+
   *  0 v  | . . . . . . . . . . |
   *  1    | . . . . . . . . . . |
   * ...   | . . . . . . . . . . |
   * 32    | . . . . . . . . . . |
   *       +---------------------+
   */
  size_t w = DISPLAY_WIDTH + 1;
  size_t h = DISPLAY_HEIGHT + 1;
  size_t ncoords = w * h * 2; /* 2 coordinates for each vertex */
  size_t half_width = DISPLAY_WIDTH / 2;
  size_t half_height = DISPLAY_HEIGHT / 2;
  GLfloat *coords = emalloc(ncoords * sizeof(*coords));
  for (size_t x = 0; x < w; ++x) {
    for (size_t y = 0; y < h; ++y) {
      size_t i = 2 * (x*h + y);
      coords[i] = -1 + ((GLfloat) x)/half_width;
      coords[i+1] = 1 - ((GLfloat) y)/half_height;
    }
  }

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferStorage(GL_ARRAY_BUFFER, ncoords*sizeof(*coords), coords, 0);
  free(coords);

  /* 65*33 quads, each quad needs 6 vertices */
  size_t nv = w * h * 6;
  GLuint *vertex = emalloc(nv*sizeof(*vertex));

  GLuint ebo;
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER, nv*sizeof(*vertex), vertex, GL_DYNAMIC_DRAW);

  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_glsl, NULL);
  glCompileShader(vertex_shader);

  if (shader_error_occurred(vertex_shader)) {
    return NULL;
  }

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_glsl, NULL);
  glCompileShader(fragment_shader);

  if (shader_error_occurred(fragment_shader)) {
    return NULL;
  }

  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glBindFragDataLocation(program, 0, "color");
  glLinkProgram(program);
  glUseProgram(program);

  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    GLint length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    if (length) {
      GLchar *buf = emalloc(length);
      glGetProgramInfoLog(program, length, NULL, buf);
      errorf("Program link error: %s\n", buf);
      free(buf);
    } else {
      errorf("Program link error\n");
    }
    return NULL;
  }

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

  err = glGetError();
  if (err != GL_NO_ERROR) {
    errorf("GL error: 0x%x\n", err);
    return NULL;
  }

  return vertex;
}
