#include <stdbool.h>
#include <stdint.h>

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

#define RENDER_SCALE 15

typedef uint16_t opcode;

typedef struct chip8 {
  uint8_t memory[0x1000];
  uint8_t V[0x10];  /* Data registers */
  uint16_t I;       /* Index register */
  uint16_t pc;
  uint8_t delay_timer;
  uint8_t sound_timer;
  uint16_t stack[0x10];
  uint16_t sp;
  uint8_t gfx[DISPLAY_WIDTH][DISPLAY_HEIGHT];
  bool draw_flag;
  bool key[0x10];
} chip8;

chip8 * chip8_init();
void chip8_destroy(chip8 *);
bool chip8_load_rom(chip8 *, char *);
void chip8_emulate_cycle(chip8 *);
