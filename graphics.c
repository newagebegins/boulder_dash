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

  static RGBQUAD black  = {0x00, 0x00, 0x00, 0x00};
  static RGBQUAD red    = {0x00, 0x00, 0xCC, 0x00};
  static RGBQUAD green  = {0x00, 0xCC, 0x00, 0x00};
  static RGBQUAD yellow = {0x00, 0xCC, 0xCC, 0x00};
  static RGBQUAD blue   = {0xCC, 0x00, 0x00, 0x00};
  static RGBQUAD purple = {0xCC, 0x00, 0xCC, 0x00};
  static RGBQUAD cyan   = {0xCC, 0xCC, 0x00, 0x00};
  static RGBQUAD gray   = {0xCC, 0xCC, 0xCC, 0x00};
  static RGBQUAD white  = {0xFF, 0xFF, 0xFF, 0x00};

  gBitmapInfo->bmiColors[0] = black;
  gBitmapInfo->bmiColors[1] = gray;
  gBitmapInfo->bmiColors[2] = white;
  gBitmapInfo->bmiColors[3] = red;
  gBitmapInfo->bmiColors[4] = yellow;
}

void displayBackbuffer() {
  StretchDIBits(gDeviceContext,
                0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT,
                gBackbuffer, gBitmapInfo,
                DIB_RGB_COLORS, SRCCOPY);
}

static void setPixel(int x, int y, uint8_t color) {
  assert(color < PALETTE_COLORS);
  assert((color & 0xF0) == 0);

  int pixelOffset = y*BACKBUFFER_WIDTH + x;
  int byteOffset = pixelOffset / 2;

  assert(byteOffset >= 0 && byteOffset < BACKBUFFER_BYTES);

  uint8_t oldColor = gBackbuffer[byteOffset];
  uint8_t newColor;
  if (pixelOffset % 2 == 0) {
    newColor = (color << 4) | (oldColor & 0x0F);
  } else {
    newColor = (oldColor & 0xF0) | color;
  }
  gBackbuffer[byteOffset] = newColor;
}

static void drawFilledRect(int left, int top, int right, int bottom, uint8_t color) {
  for (int y = top; y <= bottom; ++y) {
    for (int x = left; x <= right; ++x) {
      setPixel(x, y, color);
    }
  }
}

void drawBorder(uint8_t color) {
  drawFilledRect(0, 0, BACKBUFFER_WIDTH-1, BACKBUFFER_HEIGHT-1, color);
}

void drawSprite(const Sprite sprite, int spriteRow, int spriteCol, uint8_t fgColor, uint8_t bgColor) {
  for (uint8_t bmpY = 0; bmpY < SPRITE_SIZE; ++bmpY) {
    int y = spriteRow*SPRITE_SIZE + bmpY;
    uint8_t byte = sprite[bmpY];

    for (uint8_t bmpX = 0; bmpX < SPRITE_SIZE; ++bmpX) {
      int x = spriteCol*SPRITE_SIZE + bmpX;
      uint8_t mask = 1 << ((SPRITE_SIZE-1) - bmpX);
      uint8_t bit = byte & mask;
      uint8_t color = bit ? fgColor : bgColor;

      setPixel(x, y, color);
    }
  }
}

void drawTile(const Sprite spriteA, const Sprite spriteB, const Sprite spriteC, const Sprite spriteD,
              int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  int spriteRow = tileRow*TILE_SIZE_IN_SPRITES;
  int spriteCol = tileCol*TILE_SIZE_IN_SPRITES;
  drawSprite(spriteA, spriteRow, spriteCol, fgColor, bgColor);
  drawSprite(spriteB, spriteRow, spriteCol+1, fgColor, bgColor);
  drawSprite(spriteC, spriteRow+1, spriteCol, fgColor, bgColor);
  drawSprite(spriteD, spriteRow+1, spriteCol+1, fgColor, bgColor);
}

void drawSpaceTile(int tileRow, int tileCol) {
  int left = tileCol*TILE_SIZE;
  int top = tileRow*TILE_SIZE;
  int bottom = top + TILE_SIZE - 1;
  int right = left + TILE_SIZE - 1;
  drawFilledRect(left, top, right, bottom, 0);
}
