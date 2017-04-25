#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

#define TILE_SIZE 16
#define HALF_TILE_SIZE (TILE_SIZE/2)

#define BACKBUFFER_WIDTH 256
#define BACKBUFFER_HEIGHT 192
#define BACKBUFFER_PIXEL_COUNT (BACKBUFFER_WIDTH*BACKBUFFER_HEIGHT)
#define BACKBUFFER_BYTES (BACKBUFFER_PIXEL_COUNT*sizeof(uint32_t))

#define SPRITE_ATLAS_WIDTH 256
#define SPRITE_ATLAS_WIDTH_TILES (SPRITE_ATLAS_WIDTH / TILE_SIZE)

void drawText(char *text, int outRow, int outCol);
void initGraphics();

// TODO: make private
uint32_t *backbuffer;
uint32_t *spriteAtlas;

#endif
