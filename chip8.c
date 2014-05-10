#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"

#define MAX_ROM_SIZE (0xFFF - 0x200 + 1)

static inline void opcode_0x0000(chip8 *, opcode);
static inline void opcode_0x1000(chip8 *, opcode);
static inline void opcode_0x2000(chip8 *, opcode);
static inline void opcode_0x3000(chip8 *, opcode);
static inline void opcode_0x4000(chip8 *, opcode);
static inline void opcode_0x5000(chip8 *, opcode);
static inline void opcode_0x6000(chip8 *, opcode);
static inline void opcode_0x7000(chip8 *, opcode);
static inline void opcode_0x8000(chip8 *, opcode);
static inline void opcode_0x9000(chip8 *, opcode);
static inline void opcode_0xA000(chip8 *, opcode);
static inline void opcode_0xB000(chip8 *, opcode);
static inline void opcode_0xC000(chip8 *, opcode);
static inline void opcode_0xD000(chip8 *, opcode);
static inline void opcode_0xE000(chip8 *, opcode);
static inline void opcode_0xF000(chip8 *, opcode);

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
  opcode op;
  uint16_t old_pc;

  op = (c8->memory[c8->pc] << 8) | c8->memory[c8->pc+1];
  old_pc = c8->pc;
  c8->draw_flag = false;

  switch (op & 0xF000) {
  case 0x0000: opcode_0x0000(c8, op); break;
  case 0x1000: opcode_0x1000(c8, op); break;
  case 0x2000: opcode_0x2000(c8, op); break;
  case 0x3000: opcode_0x3000(c8, op); break;
  case 0x4000: opcode_0x4000(c8, op); break;
  case 0x5000: opcode_0x5000(c8, op); break;
  case 0x6000: opcode_0x6000(c8, op); break;
  case 0x7000: opcode_0x7000(c8, op); break;
  case 0x8000: opcode_0x8000(c8, op); break;
  case 0x9000: opcode_0x9000(c8, op); break;
  case 0xA000: opcode_0xA000(c8, op); break;
  case 0xB000: opcode_0xB000(c8, op); break;
  case 0xC000: opcode_0xC000(c8, op); break;
  case 0xD000: opcode_0xD000(c8, op); break;
  case 0xE000: opcode_0xE000(c8, op); break;
  case 0xF000: opcode_0xF000(c8, op); break;
  default:
    fprintf(stderr, "Unknown opcode 0x%" PRIX16 "\n", op);
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

static inline void chip8_inc_pc(chip8 *c8, bool skip_next_instruction)
{
  c8->pc += skip_next_instruction ? 4 : 2;
}

/* Opcode description taken from Wikipedia:
   http://en.wikipedia.org/wiki/CHIP-8#Opcode_table */

static inline void opcode_0x0000(chip8 *c8, opcode op)
{
  assert((op & 0xFF00) == 0x0000);
  switch (op & 0x00FF) {
  case 0x00E0:
    /* 00E0 Clears the screen. */
    memset(c8->gfx, 0, sizeof(c8->gfx));
    c8->draw_flag = true;
    break;
  case 0x00EE:
    /* 00EE Returns from a subroutine. */
    c8->pc = c8->stack[--c8->sp];
    break;
  default:
    /* 0NNN Calls RCA 1802 program at address NNN. */
    fprintf(stderr, "Unknown opcode 0x%" PRIX16 "\n", op);
    assert(0);
  }
  chip8_inc_pc(c8, false);
}

static inline void opcode_0x1000(chip8 *c8, opcode op)
{
  /* 1NNN Jumps to address NNN. */
  assert((op & 0xF000) == 0x1000);
  c8->pc = op & 0xFFF;
}

static inline void opcode_0x2000(chip8 *c8, opcode op)
{
  /* 2NNN Calls subroutine at NNN. */
  assert((op & 0xF000) == 0x2000);
  c8->stack[c8->sp++] = c8->pc;
  c8->pc = op & 0xFFF;
}

static inline void opcode_0x3000(chip8 *c8, opcode op)
{
  /* 3XNN Skips the next instruction if VX equals NN. */
  assert((op & 0xF000) == 0x3000);
  uint8_t X  = (op & 0x0F00) >> 8;
  uint8_t NN = op & 0x00FF;
  chip8_inc_pc(c8, c8->V[X] == NN);
}

static inline void opcode_0x4000(chip8 *c8, opcode op)
{
  /* 4XNN Skips the next instruction if VX doesn't equal NN. */
  assert((op & 0xF000) == 0x4000);
  uint8_t X  = (op & 0x0F00) >> 8;
  uint8_t NN = op & 0x00FF;
  chip8_inc_pc(c8, c8->V[X] != NN);
}

static inline void opcode_0x5000(chip8 *c8, opcode op)
{
  /* 5XY0 Skips the next instruction if VX equals VY. */
  assert((op & 0xF00F) == 0x5000);
  uint8_t X = (op & 0x0F00) >> 8;
  uint8_t Y = (op & 0x00F0) >> 4;
  chip8_inc_pc(c8, c8->V[X] == c8->V[Y]);
}

static inline void opcode_0x6000(chip8 *c8, opcode op)
{
  /* 6XNN Sets VX to NN. */
  assert((op & 0xF000) == 0x6000);
  uint8_t X  = (op & 0x0F00) >> 8;
  uint8_t NN = op & 0x00FF;
  c8->V[X] = NN;
  chip8_inc_pc(c8, false);
}

static inline void opcode_0x7000(chip8 *c8, opcode op)
{
  /* 7XNN Adds NN to VX. */
  assert((op & 0xF000) == 0x7000);
  uint8_t X  = (op & 0x0F00) >> 8;
  uint8_t NN = op & 0x00FF;
  c8->V[X] += NN;
  chip8_inc_pc(c8, false);
}

static inline void opcode_0x8000(chip8 *c8, opcode op)
{
  /* 8XYN X and Y identify data registers, N the operation */
  assert((op & 0xF000) == 0x8000);
  uint8_t X = (op & 0x0F00) >> 8;
  uint8_t Y = (op & 0x00F0) >> 4;
  switch (op & 0x000F) {
  case 0x0000:
    /* 8XY0 Sets VX to the value of VY. */
    c8->V[X] = c8->V[Y];
    break;
  case 0x0001:
    /* 8XY1 Sets VX to VX or VY. */
    c8->V[X] |= c8->V[Y];
    break;
  case 0x0002:
    /* 8XY2 Sets VX to VX and VY. */
    c8->V[X] &= c8->V[Y];
    break;
  case 0x0003:
    /* 8XY3 Sets VX to VX xor VY. */
    c8->V[X] ^= c8->V[Y];
    break;
  case 0x0004:
    /* 8XY4 Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when
       there isn't. */
    c8->V[0xF] = (c8->V[Y] > (0xFF - c8->V[X])) ? 1 : 0;
    c8->V[X] += c8->V[Y];
    break;
  case 0x0005:
    /* 8XY5 VY is subtracted from VX. VF is set to 0 when there's a borrow, and
       1 when there isn't. */
    c8->V[0xF] = (c8->V[Y] > c8->V[X]) ? 0 : 1;
    c8->V[X] -= c8->V[Y];
    break;
  case 0x0006:
    /* 8XY6 Shifts VX right by one. VF is set to the value of the least
       significant bit of VX before the shift. */
    c8->V[0xF] = c8->V[X] & 0x1;
    c8->V[X] >>= 1;
    break;
  case 0x0007:
    /* 8XY7 Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1
       when there isn't. */
    c8->V[0xF] = (c8->V[X] > c8->V[Y]) ? 0 : 1;
    c8->V[X] = c8->V[Y] - c8->V[X];
    break;
  case 0x000E:
    /* 8XYE Shifts VX left by one. VF is set to the value of the most
       significant bit of VX before the shift. */
    c8->V[0xF] = (c8->V[X] & 0x80) >> 7;
    c8->V[X] <<= 1;
    break;
  default:
    fprintf(stderr, "Unknown opcode 0x%" PRIX16 "\n", op);
    assert(0);
  }
  chip8_inc_pc(c8, false);
}

static inline void opcode_0x9000(chip8 *c8, opcode op)
{
  /* 9XY0 Skips the next instruction if VX doesn't equal VY. */
  assert((op & 0xF00F) == 0x9000);
  uint8_t X = (op & 0x0F00) >> 8;
  uint8_t Y = (op & 0x00F0) >> 4;
  chip8_inc_pc(c8, c8->V[X] != c8->V[Y]);
}

static inline void opcode_0xA000(chip8 *c8, opcode op)
{
  /* ANNN Sets I to the address NNN. */
  assert((op & 0xF000) == 0xA000);
  c8->I = op & 0xFFF;
  chip8_inc_pc(c8, false);
}

static inline void opcode_0xB000(chip8 *c8, opcode op)
{
  /* BNNN Jumps to the address NNN plus V0. */
  assert((op & 0xF000) == 0xB000);
  c8->pc = (op & 0xFFF) + c8->V[0];
}

static inline void opcode_0xC000(chip8 *c8, opcode op)
{
  /* CXNN Sets VX to a random number and NN. */
  assert((op & 0xF000) == 0xC000);
  uint8_t X  = (op & 0x0F00) >> 8;
  uint8_t NN = op & 0x00FF;
  c8->V[X] = NN & rand(); /* TODO */
  chip8_inc_pc(c8, false);
}

static inline void opcode_0xD000(chip8 *c8, opcode op)
{
  /* DXYN Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels
     and a height of N pixels. Each row of 8 pixels is read as bit-coded (with
     the most significant bit of each byte displayed on the left) starting from
     memory location I; I value doesn't change after the execution of this
     instruction. As described above, VF is set to 1 if any screen pixels are
     flipped from set to unset when the sprite is drawn, and to 0 if that
     doesn't happen. */
  assert((op & 0xF000) == 0xD000);
  uint8_t X = (op & 0x0F00) >> 8;
  uint8_t Y = (op & 0x00F0) >> 4;
  uint8_t N = op & 0x000F;
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
  chip8_inc_pc(c8, false);
}

static inline void opcode_0xE000(chip8 *c8, opcode op)
{
  assert((op & 0xF000) == 0xE000);
  uint8_t X = (op & 0x0F00) >> 8;
  switch (op & 0x00FF) {
  case 0x009E:
    /* EX9E Skips the next instruction if the key stored in VX is pressed. */
    chip8_inc_pc(c8, c8->key[c8->V[X]]);
    break;
  case 0x00A1:
    /* EXA1 Skips the next instruction if the key stored in VX isn't pressed.
     */
    chip8_inc_pc(c8, !c8->key[c8->V[X]]);
    break;
  default:
    fprintf(stderr, "Unknown opcode 0x%" PRIX16 "\n", op);
    assert(0);
  }
}

static inline void opcode_0xF000(chip8 *c8, opcode op)
{
  assert((op & 0xF000) == 0xF000);
  uint8_t X = (op & 0x0F00) >> 8;
  switch (op & 0x00FF) {
  case 0x0007:
    /* FX07 Sets VX to the value of the delay timer. */
    c8->V[X] = c8->delay_timer;
    break;
  case 0x000A:
    /* FX0A A key press is awaited, and then stored in VX. */
    /* TODO */
    for (size_t i = 0; i < 0x10; ++i) {
      if (c8->key[i]) {
        c8->V[X] = i;
        break;
      }
    }
    break;
  case 0x0015:
    /* FX15 Sets the delay timer to VX. */
    c8->delay_timer = c8->V[X];
    break;
  case 0x0018:
    /* FX18 Sets the sound timer to VX. */
    c8->sound_timer = c8->V[X];
    break;
  case 0x001E:
    /* FX1E Adds VX to I. */
    c8->V[0xF] = (c8->I > (0xFFF - c8->V[X])) ? 1 : 0;
    c8->I += c8->V[X];
    break;
  case 0x0029:
    /* FX29 Sets I to the location of the sprite for the character in VX.
       Characters 0-F (in hexadecimal) are represented by a 4x5 font. */
    assert(c8->V[X] <= 0xF);
    c8->I = c8->V[X] * 5;
    break;
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
    break;
  case 0x0055:
    /* FX55 Stores V0 to VX in memory starting at address I. */
    memcpy(c8->memory + c8->I, c8->V, X+1);
    break;
  case 0x0065:
    /* FX65 Fills V0 to VX with values from memory starting at address I. */
    memcpy(c8->V, c8->memory + c8->I, X+1);
    break;
  default:
    fprintf(stderr, "Unknown opcode 0x%" PRIX16 "\n", op);
    assert(0);
  }
  chip8_inc_pc(c8, false);
}
