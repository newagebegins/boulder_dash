#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <windows.h>
#include <stdint.h>

#define TILE_SIZE 16
#define BORDER_SIZE TILE_SIZE

// Viewport is the whole screen area except the border.
#define VIEWPORT_WIDTH 256
#define VIEWPORT_HEIGHT 192
#define VIEWPORT_X_MIN BORDER_SIZE
#define VIEWPORT_Y_MIN BORDER_SIZE
#define VIEWPORT_X_MAX (VIEWPORT_X_MIN + VIEWPORT_WIDTH - 1)
#define VIEWPORT_Y_MAX (VIEWPORT_Y_MIN + VIEWPORT_HEIGHT - 1)

#define BACKBUFFER_WIDTH (VIEWPORT_WIDTH + BORDER_SIZE*2)
#define BACKBUFFER_HEIGHT (VIEWPORT_HEIGHT + BORDER_SIZE*2)
#define BACKBUFFER_PIXELS (BACKBUFFER_WIDTH*BACKBUFFER_HEIGHT)
// 4 bits per pixel.
#define BACKBUFFER_BYTES (BACKBUFFER_PIXELS*sizeof(uint8_t)/2)

#define PALETTE_COLORS 3

#define WINDOW_SCALE 3
#define WINDOW_WIDTH (BACKBUFFER_WIDTH * WINDOW_SCALE)
#define WINDOW_HEIGHT (BACKBUFFER_HEIGHT * WINDOW_SCALE)

void initGraphics(HDC deviceContext);
void displayBackbuffer();
void setPixel(int x, int y, uint8_t colorIndex);
void drawFilledRect(int left, int top, int right, int bottom, uint8_t colorIndex);

#endif
