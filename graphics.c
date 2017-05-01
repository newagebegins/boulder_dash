#include <assert.h>
#include "graphics.h"

static uint8_t *gBackbuffer;
static BITMAPINFO *gBitmapInfo;
static HDC gDeviceContext;

void initGraphics(HDC deviceContext) {
  gDeviceContext = deviceContext;
  gBackbuffer = calloc(BACKBUFFER_PIXELS, sizeof(uint8_t));

  gBitmapInfo = malloc(sizeof(BITMAPINFOHEADER) + (PALETTE_COLORS*sizeof(RGBQUAD)));
  gBitmapInfo->bmiHeader.biSize = sizeof(gBitmapInfo->bmiHeader);
  gBitmapInfo->bmiHeader.biWidth = BACKBUFFER_WIDTH;
  gBitmapInfo->bmiHeader.biHeight = -BACKBUFFER_HEIGHT;
  gBitmapInfo->bmiHeader.biPlanes = 1;
  gBitmapInfo->bmiHeader.biBitCount = 4;
  gBitmapInfo->bmiHeader.biCompression = BI_RGB;
  gBitmapInfo->bmiHeader.biClrUsed = PALETTE_COLORS;

  gBitmapInfo->bmiColors[0].rgbBlue = 0xFF;
  gBitmapInfo->bmiColors[0].rgbGreen = 0;
  gBitmapInfo->bmiColors[0].rgbRed = 0;

  gBitmapInfo->bmiColors[1].rgbBlue = 0;
  gBitmapInfo->bmiColors[1].rgbGreen = 0xFF;
  gBitmapInfo->bmiColors[1].rgbRed = 0;

  gBitmapInfo->bmiColors[2].rgbBlue = 0;
  gBitmapInfo->bmiColors[2].rgbGreen = 0;
  gBitmapInfo->bmiColors[2].rgbRed = 0xFF;
}

void displayBackbuffer() {
  StretchDIBits(gDeviceContext,
                0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT,
                gBackbuffer, gBitmapInfo,
                DIB_RGB_COLORS, SRCCOPY);
}

void setPixel(int x, int y, uint8_t colorIndex) {
  assert((colorIndex & 0xf0) == 0);

  int pixelOffset = y*BACKBUFFER_WIDTH + x;
  int byteOffset = pixelOffset / 2;

  assert(byteOffset >= 0 && byteOffset < BACKBUFFER_BYTES);

  if (pixelOffset % 2 == 0) {
    gBackbuffer[byteOffset] |= (colorIndex << 4);
  } else {
    gBackbuffer[byteOffset] |= colorIndex;
  }
}

void drawFilledRect(int left, int top, int right, int bottom, uint8_t colorIndex) {
  for (int y = top; y <= bottom; ++y) {
    for (int x = left; x <= right; ++x) {
      setPixel(x, y, colorIndex);
    }
  }
}
