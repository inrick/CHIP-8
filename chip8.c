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
}
