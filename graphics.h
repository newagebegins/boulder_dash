#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

#define COLOR_BLACK 0xFF000000
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_GREEN 0xFF00FF00
#define COLOR_RED 0xFFFF0000
#define COLOR_BLUE 0xFF0000FF
#define COLOR_YELLOW 0xFFFFFF00
#define COLOR_MAGENTA 0xFFFF00FF
#define COLOR_CYAN 0xFF00FFFF
#define COLOR_PINK 0xFFF6A5D1

#define TILE_SIZE 16
#define HALF_TILE_SIZE (TILE_SIZE/2)

#define BACKBUFFER_WIDTH 256
#define BACKBUFFER_HEIGHT 192
#define BACKBUFFER_PIXEL_COUNT (BACKBUFFER_WIDTH*BACKBUFFER_HEIGHT)
#define BACKBUFFER_BYTES (BACKBUFFER_PIXEL_COUNT*sizeof(uint32_t))

#define SPRITE_ATLAS_WIDTH 256
#define SPRITE_ATLAS_WIDTH_TILES (SPRITE_ATLAS_WIDTH / TILE_SIZE)

void drawLine(int x1, int y1, int x2, int y2, int color);
void drawText(char *text, int outRow, int outCol);
void initGraphics();

// TODO: make private
uint32_t *backbuffer;
uint32_t *spriteAtlas;

#endif
