#include <stdlib.h>
#include <stdio.h>
#include <GLFW/glfw3.h>

#include "chip8.h"

static void render(chip8 *);
static void draw_point(int, int);
static void key_handler(GLFWwindow *, int, int, int, int);
static void resize_handler(GLFWwindow *, int, int);

chip8 *c8;
int width;
int height;

int main(int argc, char **argv)
{
  GLFWwindow *window;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <CHIP-8 ROM>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }
  width  = DISPLAY_WIDTH  * RENDER_SCALE;
  height = DISPLAY_HEIGHT * RENDER_SCALE;
  window = glfwCreateWindow(width, height, "CHIP-8", NULL, NULL);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, key_handler);
  glfwSetWindowSizeCallback(window, resize_handler);
  resize_handler(window, width, height);
  glDisable(GL_DEPTH_TEST);

  c8 = chip8_init();
  if (!c8 || !chip8_load_rom(c8, argv[1])) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  while (!glfwWindowShouldClose(window)) {
    chip8_emulate_cycle(c8);
    if (c8->draw_flag) {
      glClear(GL_COLOR_BUFFER_BIT);
      render(c8);
      glfwSwapBuffers(window);
    }

    glfwPollEvents();
  }

  chip8_destroy(c8);
  glfwTerminate();
  return 0;
}

static void render(chip8 *c8)
{
  glColor3f(.85f, .85f, .85f);
  for (int x = 0; x < DISPLAY_WIDTH; ++x) {
    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
      if (c8->gfx[x][y] == 1) {
        draw_point(x, y);
      }
    }
  }
}

static void draw_point(int x, int y)
{
  float xf = (float) x;
  float yf = (float) y;

  glBegin(GL_QUADS);
  glVertex2f(xf,   yf);
  glVertex2f(xf+1, yf);
  glVertex2f(xf+1, yf+1);
  glVertex2f(xf,   yf+1);
  glEnd();
}

static void
key_handler(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  /* Keypad         Keyboard
     +-+-+-+-+      +-+-+-+-+
     |1|2|3|C|      |1|2|3|4|
     +-+-+-+-+      +-+-+-+-+
     |4|5|6|D|      |Q|W|E|R|
     +-+-+-+-+  =>  +-+-+-+-+
     |7|8|9|E|      |A|S|D|F|
     +-+-+-+-+      +-+-+-+-+
     |A|0|B|F|      |Z|X|C|V|
     +-+-+-+-+      +-+-+-+-+ */
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

static void resize_handler(GLFWwindow *window, int w, int h)
{
  width = w;
  height = h;

  glClearColor(0.f, 0.f, 0.f, 0);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, 1);
  glMatrixMode(GL_MODELVIEW);

  glViewport(0, 0, width, height);
}
