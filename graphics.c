#include <stdbool.h>
#include <assert.h>
#include "graphics.h"
#include "data_sprites.h"

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

static void drawSprite(const Sprite sprite, int spriteRow, int spriteCol, uint8_t fgColor, uint8_t bgColor,
                       bool flipHorz, bool flipVert) {
  for (uint8_t bmpY = 0; bmpY < SPRITE_SIZE; ++bmpY) {
    int y = flipVert ? (spriteRow*SPRITE_SIZE + SPRITE_SIZE-1 - bmpY) : (spriteRow*SPRITE_SIZE + bmpY);
    uint8_t byte = sprite[bmpY];

    for (uint8_t bmpX = 0; bmpX < SPRITE_SIZE; ++bmpX) {
      int x = flipHorz ? (spriteCol*SPRITE_SIZE + SPRITE_SIZE-1 - bmpX) : (spriteCol*SPRITE_SIZE + bmpX);
      uint8_t mask = 1 << ((SPRITE_SIZE-1) - bmpX);
      uint8_t bit = byte & mask;
      uint8_t color = bit ? fgColor : bgColor;

      setPixel(x, y, color);
    }
  }
}

static void tileToSpritePos(int tileRow, int tileCol, int *spriteRow, int *spriteCol) {
  *spriteRow = (PLAYFIELD_Y_MIN_IN_TILES + tileRow) * TILE_SIZE_IN_SPRITES;
  *spriteCol = (PLAYFIELD_X_MIN_IN_TILES + tileCol) * TILE_SIZE_IN_SPRITES;
}

static void drawTile(const Sprite spriteA, const Sprite spriteB, const Sprite spriteC, const Sprite spriteD,
                     int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  int spriteRow, spriteCol;
  tileToSpritePos(tileRow, tileCol, &spriteRow, &spriteCol);
  drawSprite(spriteA, spriteRow, spriteCol, fgColor, bgColor, false, false);
  drawSprite(spriteB, spriteRow, spriteCol+1, fgColor, bgColor, false, false);
  drawSprite(spriteC, spriteRow+1, spriteCol, fgColor, bgColor, false, false);
  drawSprite(spriteD, spriteRow+1, spriteCol+1, fgColor, bgColor, false, false);
}

void drawSpaceTile(int tileRow, int tileCol) {
  int left = PLAYFIELD_X_MIN + tileCol*TILE_SIZE;
  int top = PLAYFIELD_Y_MIN + tileRow*TILE_SIZE;
  int bottom = top + TILE_SIZE - 1;
  int right = left + TILE_SIZE - 1;
  drawFilledRect(left, top, right, bottom, 0);
}

void drawSteelWallTile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  drawTile(gSpriteSteelWall, gSpriteSteelWall, gSpriteSteelWall, gSpriteSteelWall, tileRow, tileCol, fgColor, bgColor);
}

void drawDirtTile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  drawTile(gSpriteDirtA, gSpriteDirtB, gSpriteDirtC, gSpriteDirtD, tileRow, tileCol, fgColor, bgColor);
}

void drawBrickWallTile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  drawTile(gSpriteBrick1, gSpriteBrick1, gSpriteBrick1, gSpriteBrick1, tileRow, tileCol, fgColor, bgColor);
}

void drawBoulderTile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  drawTile(gSpriteBoulderA, gSpriteBoulderB, gSpriteBoulderC, gSpriteBoulderD, tileRow, tileCol, fgColor, bgColor);
}

void drawDiamond1Tile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  drawTile(gSpriteDiamond1A, gSpriteDiamond1B, gSpriteDiamond1C, gSpriteDiamond1D, tileRow, tileCol, fgColor, bgColor);
}

void drawExplosion1Tile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  drawTile(gSpriteExplosion1A, gSpriteExplosion1B, gSpriteExplosion1C, gSpriteExplosion1D, tileRow, tileCol, fgColor, bgColor);
}

void drawOutboxTile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  int spriteRow, spriteCol;
  tileToSpritePos(tileRow, tileCol, &spriteRow, &spriteCol);
  drawSprite(gSpriteOutbox, spriteRow, spriteCol, fgColor, bgColor, false, false);
  drawSprite(gSpriteOutbox, spriteRow, spriteCol+1, fgColor, bgColor, true, false);
  drawSprite(gSpriteOutbox, spriteRow+1, spriteCol, fgColor, bgColor, false, true);
  drawSprite(gSpriteOutbox, spriteRow+1, spriteCol+1, fgColor, bgColor, true, true);
}

static const uint8_t* getCharSprite(char ch) {
  switch (ch) {
    case '0': return gSpriteLetter0;
    case '1': return gSpriteLetter1;
    case '2': return gSpriteLetter2;
    case '3': return gSpriteLetter3;
    case '4': return gSpriteLetter4;
    case '5': return gSpriteLetter5;
    case '6': return gSpriteLetter6;
    case '7': return gSpriteLetter7;
    case '8': return gSpriteLetter8;
    case '9': return gSpriteLetter9;
    case ':': return gSpriteLetterColon;
    case 'A': return gSpriteLetterA;
    case 'B': return gSpriteLetterB;
    case 'C': return gSpriteLetterC;
    case 'D': return gSpriteLetterD;
    case 'E': return gSpriteLetterE;
    case 'F': return gSpriteLetterF;
    case 'G': return gSpriteLetterG;
    case 'H': return gSpriteLetterH;
    case 'I': return gSpriteLetterI;
    case 'J': return gSpriteLetterJ;
    case 'K': return gSpriteLetterK;
    case 'L': return gSpriteLetterL;
    case 'M': return gSpriteLetterM;
    case 'N': return gSpriteLetterN;
    case 'O': return gSpriteLetterO;
    case 'P': return gSpriteLetterP;
    case 'Q': return gSpriteLetterQ;
    case 'R': return gSpriteLetterR;
    case 'S': return gSpriteLetterS;
    case 'T': return gSpriteLetterT;
    case 'U': return gSpriteLetterU;
    case 'V': return gSpriteLetterV;
    case 'X': return gSpriteLetterX;
    case 'Y': return gSpriteLetterY;
    case 'Z': return gSpriteLetterZ;
    case '(': return gSpriteLetterLBracket;
    case ')': return gSpriteLetterRBracket;
    case '*': return gSpriteLetterDiamond;
    case '>': return gSpriteLetterArrow;
    case ',': return gSpriteLetterComma;
    case '-': return gSpriteLetterDash;
    case '.': return gSpriteLetterPeriod;
    case '/': return gSpriteLetterSlash;
  }
  assert(!"Unsupported character");
  return gSpriteLetterDiamond;
}

void drawText(const char *text) {
  for (int i = 0; text[i]; ++i) {
    if (text[i] == ' ') {
      continue;
    }
    drawSprite(getCharSprite(text[i]), 3, 2+i, 1, 0, false, false);
  }
}
