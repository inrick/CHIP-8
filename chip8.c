#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"

#define MAX_ROM_SIZE (0xFFF - 0x200 + 1)

static uint8_t chip8_fontset[80] =
{
  0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
  0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
  0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
  0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
  0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
  0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
  0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
  0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
  0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
  0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
  0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
  0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
  0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
  0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
  0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
  0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */
};

chip8 * chip8_init()
{
  chip8 *c8 = malloc(sizeof(*c8));
  if (!c8) {
    return NULL;
  }
  memset(c8, 0, sizeof(*c8));
  memcpy(c8->memory, chip8_fontset, sizeof(chip8_fontset));
  c8->pc = 0x200;

  return c8;
}

void chip8_destroy(chip8 *c8)
{
  free(c8);
}

bool chip8_load_rom(chip8 *c8, char *rom_path)
{
  uint8_t buffer[MAX_ROM_SIZE + 1];
  size_t bytes_read;
  FILE *rom;

  rom = fopen(rom_path, "rb");
  if (!rom) {
    return false;
  }

  bytes_read = fread(buffer, sizeof(*buffer), sizeof(buffer), rom);
  if (bytes_read == sizeof(buffer)) {
    fprintf(stderr, "ROM file too big\n");
    fclose(rom);
    return false;
  }
  if (ferror(rom)) {
    fprintf(stderr, "Error when reading ROM file");
    fclose(rom);
    return false;
  }

  memcpy(c8->memory + 0x200, buffer, bytes_read * sizeof(*buffer));
  fclose(rom);
  return true;
}

void chip8_emulate_cycle(chip8 *c8)
{
  uint16_t old_pc;

  old_pc = c8->pc;
  c8->op = (c8->memory[c8->pc] << 8) | c8->memory[c8->pc+1];

  switch (c8->op & 0xF000) {
  case 0x0000:
    switch (c8->op & 0x00FF) {
    case 0x00E0: /* 00E0 Clears the screen. */
      memset(c8->gfx, 0, sizeof(c8->gfx));
      c8->draw_flag = true;
      c8->pc += 2;
      break;
    case 0x00EE: /* 00EE Returns from a subroutine. */
      c8->pc = c8->stack[--c8->sp];
      c8->pc += 2;
      break;
    default: /* 0NNN Calls RCA 1802 program at address NNN. */
      fprintf(stderr, "Unknown opcode 0x%" PRIX16 "\n", c8->op);
      assert(0);
    }
    break;
  case 0x2000: /* 2NNN Calls subroutine at NNN. */
    c8->stack[c8->sp++] = c8->pc;
    c8->pc = c8->op & 0xFFF;
    break;
  case 0x8000: { /* 8XYN X and Y identify data registers, N the operation */
    uint8_t X = (c8->op & 0x0F00) >> 8;
    uint8_t Y = (c8->op & 0x00F0) >> 4;
    switch (c8->op & 0x000F) {
    case 0x0000: /* 8XY0 Sets VX to the value of VY. */
      c8->V[X] = c8->V[Y];
      break;
    case 0x0001: /* 8XY1 Sets VX to VX or VY. */
      c8->V[X] |= c8->V[Y];
      break;
    case 0x0002: /* 8XY2 Sets VX to VX and VY. */
      c8->V[X] &= c8->V[Y];
      break;
    case 0x0003: /* 8XY3 Sets VX to VX xor VY. */
      c8->V[X] ^= c8->V[Y];
      break;
    case 0x0004: /* 8XY4 Adds VY to VX. VF is set to 1 when there's a carry,
                    and to 0 when there isn't. */
      c8->V[0xF] = (c8->V[Y] > (0xFF - c8->V[X])) ? 1 : 0;
      c8->V[X] += c8->V[Y];
      break;
    default:
      fprintf(stderr, "Unknown opcode 0x%" PRIX16 "\n", c8->op);
      assert(0);
    }
    c8->pc += 2;
    break;
  }
  case 0xA000: /* ANNN Sets I to the address NNN. */
    c8->I = c8->op & 0xFFF;
    c8->pc += 2;
    break;
  case 0xD000: {
    /* DXYN Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels
       and a height of N pixels. Each row of 8 pixels is read as bit-coded
       (with the most significant bit of each byte displayed on the left)
       starting from memory location I; I value doesn't change after the
       execution of this instruction. As described above, VF is set to 1 if any
       screen pixels are flipped from set to unset when the sprite is drawn,
       and to 0 if that doesn't happen. */
    uint8_t X = (c8->op & 0x0F00) >> 8;
    uint8_t Y = (c8->op & 0x00F0) >> 4;
    uint8_t N = c8->op & 0x000F;
    uint8_t x = c8->V[X];
    uint8_t y = c8->V[Y];

    c8->V[0xF] = 0;
    for (uint8_t row = 0; row < N; ++row) {
      uint8_t sprite_row = c8->memory[c8->I+row];
      for (uint8_t col = 0; col < 8; ++col) {
        if ((sprite_row & (0x80 >> col)) != 0) {
          if (c8->gfx[x+col][y+row] == 1) {
            c8->V[0xF] = 1;
          }
          c8->gfx[x+col][y+row] ^= 1;
        }
      }
    }
    c8->draw_flag = true;
    c8->pc += 2;
    break;
  }
  case 0xE000: {
    uint8_t X = (c8->op & 0x0F00) >> 8;
    switch (c8->op & 0x00FF) {
    case 0x009E:
      /* EX9E Skips the next instruction if the key stored in VX is pressed. */
      c8->pc += (c8->key[c8->V[X]]) ? 4 : 2;
      break;
    case 0x00A1:
      /* EXA1 Skips the next instruction if the key stored in VX isn't pressed.
       */
      c8->pc += (!c8->key[c8->V[X]]) ? 4 : 2;
      break;
    default:
      fprintf(stderr, "Unknown opcode 0x%" PRIX16 "\n", c8->op);
      assert(0);
    }
    break;
  }
  case 0xF000: {
    uint8_t X = (c8->op & 0x0F00) >> 8;
    switch (c8->op & 0x00FF) {
    case 0x0033:
      /* FX33 Stores the Binary-coded decimal representation of VX, with the
         most significant of three digits at the address in I, the middle digit
         at I plus 1, and the least significant digit at I plus 2. (In other
         words, take the decimal representation of VX, place the hundreds digit
         in memory at location in I, the tens digit at location I+1, and the
         ones digit at location I+2.) */
      c8->memory[c8->I]   = c8->V[X] / 100;
      c8->memory[c8->I+1] = (c8->V[X] / 10) % 100;
      c8->memory[c8->I+2] = (c8->V[X] % 100) % 10;
      c8->pc += 2;
      break;
    default:
      fprintf(stderr, "Unknown opcode 0x%" PRIX16 "\n", c8->op);
      assert(0);
    }
    break;
  }
  default:
    fprintf(stderr, "Unknown opcode 0x%" PRIX16 "\n", c8->op);
    assert(0);
  }

  assert(c8->pc != old_pc);

  if (c8->delay_timer > 0) {
    --c8->delay_timer;
  }
  if (c8->sound_timer > 0) {
    if (c8->sound_timer == 1) {
      printf("beep\n");
    }
    --c8->sound_timer;
  }
}
