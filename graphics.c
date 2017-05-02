#include <assert.h>
#include "graphics.h"

static uint8_t *gBackbuffer;
static BITMAPINFO *gBitmapInfo;
static HDC gDeviceContext;

void initGraphics(HDC deviceContext) {
  gDeviceContext = deviceContext;
  gBackbuffer = calloc(BACKBUFFER_PIXELS, sizeof(*gBackbuffer));

  gBitmapInfo = malloc(sizeof(BITMAPINFOHEADER) + (PALETTE_COLOR_COUNT*sizeof(RGBQUAD)));
  gBitmapInfo->bmiHeader.biSize = sizeof(gBitmapInfo->bmiHeader);
  gBitmapInfo->bmiHeader.biWidth = BACKBUFFER_WIDTH;
  gBitmapInfo->bmiHeader.biHeight = -BACKBUFFER_HEIGHT;
  gBitmapInfo->bmiHeader.biPlanes = 1;
  gBitmapInfo->bmiHeader.biBitCount = 4;
  gBitmapInfo->bmiHeader.biCompression = BI_RGB;
  gBitmapInfo->bmiHeader.biClrUsed = PALETTE_COLOR_COUNT;

  gBitmapInfo->bmiColors[PALETTE_COLOR_0].rgbBlue = 0xFF;
  gBitmapInfo->bmiColors[PALETTE_COLOR_0].rgbGreen = 0;
  gBitmapInfo->bmiColors[PALETTE_COLOR_0].rgbRed = 0;

  gBitmapInfo->bmiColors[PALETTE_COLOR_1].rgbBlue = 0;
  gBitmapInfo->bmiColors[PALETTE_COLOR_1].rgbGreen = 0xFF;
  gBitmapInfo->bmiColors[PALETTE_COLOR_1].rgbRed = 0;

  gBitmapInfo->bmiColors[PALETTE_COLOR_2].rgbBlue = 0;
  gBitmapInfo->bmiColors[PALETTE_COLOR_2].rgbGreen = 0;
  gBitmapInfo->bmiColors[PALETTE_COLOR_2].rgbRed = 0xFF;
}

void displayBackbuffer() {
  StretchDIBits(gDeviceContext,
                0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT,
                gBackbuffer, gBitmapInfo,
                DIB_RGB_COLORS, SRCCOPY);
}

void setPixel(int x, int y, PaletteColor color) {
  int pixelOffset = y*BACKBUFFER_WIDTH + x;
  int byteOffset = pixelOffset / 2;

  assert(byteOffset >= 0 && byteOffset < BACKBUFFER_BYTES);

  if (pixelOffset % 2 == 0) {
    gBackbuffer[byteOffset] |= (color << 4);
  } else {
    gBackbuffer[byteOffset] |= color;
  }
}

void drawFilledRect(int left, int top, int right, int bottom, PaletteColor color) {
  for (int y = top; y <= bottom; ++y) {
    for (int x = left; x <= right; ++x) {
      setPixel(x, y, color);
    }
  }
}

void drawSprite(const Sprite sprite, int outRow, int outCol, PaletteColor fgColor, PaletteColor bgColor) {
  for (uint8_t sprY = 0; sprY < SPRITE_SIZE; ++sprY) {
    int y = outRow*SPRITE_SIZE + sprY;
    uint8_t byte = sprite[sprY];

    for (uint8_t sprX = 0; sprX < SPRITE_SIZE; ++sprX) {
      int x = outCol*SPRITE_SIZE + sprX;
      uint8_t mask = 1 << ((SPRITE_SIZE-1) - sprX);
      uint8_t bit = byte & mask;
      PaletteColor color = bit ? fgColor : bgColor;

      setPixel(x, y, color);
    }
  }
}
