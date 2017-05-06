#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(*array))

#define SPRITE_SIZE 8
#define TILE_SIZE_IN_SPRITES 2
#define TILE_SIZE (SPRITE_SIZE*TILE_SIZE_IN_SPRITES)
#define BORDER_SIZE_IN_TILES 1
#define BORDER_SIZE (TILE_SIZE*BORDER_SIZE_IN_TILES)

// Viewport is the whole screen area except the border.
#define VIEWPORT_WIDTH 256
#define VIEWPORT_HEIGHT 192
#define VIEWPORT_WIDTH_IN_TILES (VIEWPORT_WIDTH/TILE_SIZE)
#define VIEWPORT_HEIGHT_IN_TILES (VIEWPORT_HEIGHT/TILE_SIZE)

#define VIEWPORT_X_MIN BORDER_SIZE
#define VIEWPORT_Y_MIN BORDER_SIZE
#define VIEWPORT_X_MAX (VIEWPORT_X_MIN + VIEWPORT_WIDTH - 1)
#define VIEWPORT_Y_MAX (VIEWPORT_Y_MIN + VIEWPORT_HEIGHT - 1)

#define VIEWPORT_X_MIN_IN_TILES BORDER_SIZE_IN_TILES
#define VIEWPORT_Y_MIN_IN_TILES BORDER_SIZE_IN_TILES
#define VIEWPORT_X_MAX_IN_TILES (VIEWPORT_X_MIN_IN_TILES + VIEWPORT_WIDTH_IN_TILES - 1)
#define VIEWPORT_Y_MAX_IN_TILES (VIEWPORT_Y_MIN_IN_TILES + VIEWPORT_HEIGHT_IN_TILES - 1)

#define BACKBUFFER_WIDTH (VIEWPORT_WIDTH + BORDER_SIZE*2)
#define BACKBUFFER_HEIGHT (VIEWPORT_HEIGHT + BORDER_SIZE*2)
#define BACKBUFFER_PIXELS (BACKBUFFER_WIDTH*BACKBUFFER_HEIGHT)
// 4 bits per pixel.
#define BACKBUFFER_BYTES (BACKBUFFER_PIXELS*sizeof(uint8_t)/2)

#define PALETTE_COLORS 5

#define WINDOW_SCALE 3
#define WINDOW_WIDTH (BACKBUFFER_WIDTH * WINDOW_SCALE)
#define WINDOW_HEIGHT (BACKBUFFER_HEIGHT * WINDOW_SCALE)

#define TEXT_AREA_HEIGHT_IN_TILES 1
#define PLAYFIELD_WIDTH_IN_TILES VIEWPORT_WIDTH_IN_TILES
#define PLAYFIELD_HEIGHT_IN_TILES (VIEWPORT_HEIGHT_IN_TILES - TEXT_AREA_HEIGHT_IN_TILES)
#define PLAYFIELD_X_MIN_IN_TILES VIEWPORT_X_MIN_IN_TILES
#define PLAYFIELD_Y_MIN_IN_TILES (VIEWPORT_Y_MIN_IN_TILES + TEXT_AREA_HEIGHT_IN_TILES)
#define PLAYFIELD_X_MIN (PLAYFIELD_X_MIN_IN_TILES*TILE_SIZE)
#define PLAYFIELD_Y_MIN (PLAYFIELD_Y_MIN_IN_TILES*TILE_SIZE)

typedef uint8_t Sprite[SPRITE_SIZE];

#include "data_sprites.h"

uint8_t *gBackbuffer;
BITMAPINFO *gBitmapInfo;
HDC gDeviceContext;

void setPixel(int x, int y, uint8_t color) {
  assert(color < PALETTE_COLORS);
  assert((color & 0xF0) == 0);

  int pixelOffset = y*BACKBUFFER_WIDTH + x;
  int byteOffset = pixelOffset / 2;

  assert(byteOffset >= 0 && byteOffset < BACKBUFFER_BYTES);

  uint8_t oldColor = gBackbuffer[byteOffset];
  uint8_t newColor;
  if (pixelOffset % 2 == 0) {
    newColor = (color << 4) | (oldColor & 0x0F);
  }
  else {
    newColor = (oldColor & 0xF0) | color;
  }
  gBackbuffer[byteOffset] = newColor;
}

void drawFilledRectPx(int left, int top, int right, int bottom, uint8_t color) {
  for (int y = top; y <= bottom; ++y) {
    for (int x = left; x <= right; ++x) {
      setPixel(x, y, color);
    }
  }
}

void drawSprite(const Sprite sprite, int spriteRow, int spriteCol, uint8_t fgColor, uint8_t bgColor,
                       bool flipHorz, bool flipVert, int animationStep) {
  for (uint8_t bmpY = 0; bmpY < SPRITE_SIZE; ++bmpY) {
    int y = flipVert ? (spriteRow*SPRITE_SIZE + SPRITE_SIZE - 1 - bmpY) : (spriteRow*SPRITE_SIZE + bmpY);
    uint8_t byte = sprite[(bmpY + animationStep) % SPRITE_SIZE];

    for (uint8_t bmpX = 0; bmpX < SPRITE_SIZE; ++bmpX) {
      int x = flipHorz ? (spriteCol*SPRITE_SIZE + SPRITE_SIZE - 1 - bmpX) : (spriteCol*SPRITE_SIZE + bmpX);
      uint8_t mask = 1 << ((SPRITE_SIZE - 1) - bmpX);
      uint8_t bit = byte & mask;
      uint8_t color = bit ? fgColor : bgColor;

      setPixel(x, y, color);
    }
  }
}

void tileToSpritePos(int tileRow, int tileCol, int *spriteRow, int *spriteCol) {
  *spriteRow = (PLAYFIELD_Y_MIN_IN_TILES + tileRow) * TILE_SIZE_IN_SPRITES;
  *spriteCol = (PLAYFIELD_X_MIN_IN_TILES + tileCol) * TILE_SIZE_IN_SPRITES;
}

void drawTile(const Sprite spriteA, const Sprite spriteB, const Sprite spriteC, const Sprite spriteD,
                     int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor, int animationStep) {
  int spriteRow, spriteCol;
  tileToSpritePos(tileRow, tileCol, &spriteRow, &spriteCol);
  drawSprite(spriteA, spriteRow, spriteCol, fgColor, bgColor, false, false, animationStep);
  drawSprite(spriteB, spriteRow, spriteCol + 1, fgColor, bgColor, false, false, animationStep);
  drawSprite(spriteC, spriteRow + 1, spriteCol, fgColor, bgColor, false, false, animationStep);
  drawSprite(spriteD, spriteRow + 1, spriteCol + 1, fgColor, bgColor, false, false, animationStep);
}

void drawSpaceTile(int tileRow, int tileCol) {
  int left = PLAYFIELD_X_MIN + tileCol*TILE_SIZE;
  int top = PLAYFIELD_Y_MIN + tileRow*TILE_SIZE;
  int bottom = top + TILE_SIZE - 1;
  int right = left + TILE_SIZE - 1;
  drawFilledRectPx(left, top, right, bottom, 0);
}

void drawSteelWallTile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  drawTile(gSpriteSteelWall, gSpriteSteelWall, gSpriteSteelWall, gSpriteSteelWall, tileRow, tileCol, fgColor, bgColor, 0);
}

void drawAnimatedSteelWallTile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor, int animationStep) {
  drawTile(gSpriteSteelWall, gSpriteSteelWall, gSpriteSteelWall, gSpriteSteelWall, tileRow, tileCol, fgColor, bgColor, animationStep);
}

void drawDirtTile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  drawTile(gSpriteDirtA, gSpriteDirtB, gSpriteDirtC, gSpriteDirtD, tileRow, tileCol, fgColor, bgColor, 0);
}

void drawBrickWallTile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  drawTile(gSpriteBrick1, gSpriteBrick1, gSpriteBrick1, gSpriteBrick1, tileRow, tileCol, fgColor, bgColor, 0);
}

void drawBoulderTile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  drawTile(gSpriteBoulderA, gSpriteBoulderB, gSpriteBoulderC, gSpriteBoulderD, tileRow, tileCol, fgColor, bgColor, 0);
}

void drawDiamond1Tile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  drawTile(gSpriteDiamond1A, gSpriteDiamond1B, gSpriteDiamond1C, gSpriteDiamond1D, tileRow, tileCol, fgColor, bgColor, 0);
}

void drawExplosion1Tile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  drawTile(gSpriteExplosion1A, gSpriteExplosion1B, gSpriteExplosion1C, gSpriteExplosion1D, tileRow, tileCol, fgColor, bgColor, 0);
}

void drawExplosion2Tile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  drawTile(gSpriteExplosion2A, gSpriteExplosion2B, gSpriteExplosion2C, gSpriteExplosion2D, tileRow, tileCol, fgColor, bgColor, 0);
}

void drawExplosion3Tile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  drawTile(gSpriteExplosion3A, gSpriteExplosion3B, gSpriteExplosion3C, gSpriteExplosion3D, tileRow, tileCol, fgColor, bgColor, 0);
}

void drawOutboxTile(int tileRow, int tileCol, uint8_t fgColor, uint8_t bgColor) {
  int spriteRow, spriteCol;
  tileToSpritePos(tileRow, tileCol, &spriteRow, &spriteCol);
  drawSprite(gSpriteOutbox, spriteRow, spriteCol, fgColor, bgColor, false, false, 0);
  drawSprite(gSpriteOutbox, spriteRow, spriteCol + 1, fgColor, bgColor, true, false, 0);
  drawSprite(gSpriteOutbox, spriteRow + 1, spriteCol, fgColor, bgColor, false, true, 0);
  drawSprite(gSpriteOutbox, spriteRow + 1, spriteCol + 1, fgColor, bgColor, true, true, 0);
}

void drawMovingRockfordTile(int tileRow, int tileCol, bool isFacingRight, int animationStep) {
  UNREFERENCED_PARAMETER(isFacingRight);
  const int MOVING_ROCKFORD_FRAMES_COUNT = 6;
  int spriteRow, spriteCol;
  tileToSpritePos(tileRow, tileCol, &spriteRow, &spriteCol);
  switch (animationStep % MOVING_ROCKFORD_FRAMES_COUNT) {
    case 0:
      drawSprite(gSpriteRockfordHead1A, spriteRow, spriteCol, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordHead1B, spriteRow, spriteCol + 1, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordBottomRun1A, spriteRow + 1, spriteCol, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordBottomRun1B, spriteRow + 1, spriteCol + 1, 1, 0, false, false, 0);
      break;
    case 1:
      drawSprite(gSpriteRockfordHead1A, spriteRow, spriteCol, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordHead1B, spriteRow, spriteCol + 1, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordBottomRun2A, spriteRow + 1, spriteCol, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordBottomRun2B, spriteRow + 1, spriteCol + 1, 1, 0, false, false, 0);
      break;
    case 2:
      drawSprite(gSpriteRockfordHead2A, spriteRow, spriteCol, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordHead2B, spriteRow, spriteCol + 1, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordBottomRun3A, spriteRow + 1, spriteCol, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordBottomRun3B, spriteRow + 1, spriteCol + 1, 1, 0, false, false, 0);
      break;
    case 3:
      drawSprite(gSpriteRockfordHead2A, spriteRow, spriteCol, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordHead2B, spriteRow, spriteCol + 1, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordBottomRun4A, spriteRow + 1, spriteCol, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordBottomRun4B, spriteRow + 1, spriteCol + 1, 1, 0, false, false, 0);
      break;
    case 4:
      drawSprite(gSpriteRockfordHead1A, spriteRow, spriteCol, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordHead1B, spriteRow, spriteCol + 1, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordBottomRun5A, spriteRow + 1, spriteCol, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordBottomRun5B, spriteRow + 1, spriteCol + 1, 1, 0, false, false, 0);
      break;
    case 5:
      drawSprite(gSpriteRockfordHead1A, spriteRow, spriteCol, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordHead1B, spriteRow, spriteCol + 1, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordBottomRun3A, spriteRow + 1, spriteCol, 1, 0, false, false, 0);
      drawSprite(gSpriteRockfordBottomRun3B, spriteRow + 1, spriteCol + 1, 1, 0, false, false, 0);
      break;
  }
}

void drawIdleRockfordTile(int tileRow, int tileCol) {
  int spriteRow, spriteCol;
  tileToSpritePos(tileRow, tileCol, &spriteRow, &spriteCol);
  drawSprite(gSpriteRockfordEye1, spriteRow, spriteCol, 1, 0, false, false, 0);
  drawSprite(gSpriteRockfordEye1, spriteRow, spriteCol+1, 1, 0, true, false, 0);
  drawSprite(gSpriteRockfordBottomIdle1, spriteRow+1, spriteCol, 1, 0, false, false, 0);
  drawSprite(gSpriteRockfordBottomIdle1, spriteRow+1, spriteCol+1, 1, 0, true, false, 0);
}

const uint8_t* getCharSprite(char ch) {
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
    drawSprite(getCharSprite(text[i]), 3, 2 + i, 1, 0, false, false, 0);
  }
}

#define CAVE_HEIGHT 22
#define CAVE_WIDTH 40
#define NUM_DIFFICULTY_LEVELS 5
#define NUM_RANDOM_OBJECTS 4

typedef enum {
  OBJ_SPACE = 0x00,
  OBJ_DIRT = 0x01,
  OBJ_BRICK_WALL = 0x02,
  OBJ_MAGIC_WALL = 0x03,
  OBJ_PRE_OUTBOX = 0x04, // before it starts flashing
  OBJ_FLASHING_OUTBOX = 0x05,
  OBJ_STEEL_WALL = 0x07,
  OBJ_FIREFLY_POSITION_1 = 0x08,
  OBJ_FIREFLY_POSITION_2 = 0x09,
  OBJ_FIREFLY_POSITION_3 = 0x0A,
  OBJ_FIREFLY_POSITION_4 = 0x0B,
  OBJ_FIREFLY_POSITION_1_SCANNED = 0x0C,
  OBJ_FIREFLY_POSITION_2_SCANNED = 0x0D,
  OBJ_FIREFLY_POSITION_3_SCANNED = 0x0E,
  OBJ_FIREFLY_POSITION_4_SCANNED = 0x0F,
  OBJ_BOULDER_STATIONARY = 0x10,
  OBJ_BOULDER_STATIONARY_SCANNED = 0x11,
  OBJ_BOULDER_FALLING = 0x12,
  OBJ_BOULDER_FALLING_SCANNED = 0x13,
  OBJ_DIAMOND_STATIONARY = 0x14,
  OBJ_DIAMOND_STATIONARY_SCANNED = 0x15,
  OBJ_DIAMOND_FALLING = 0x16,
  OBJ_DIAMOND_FALLING_SCANNED = 0x17,
  OBJ_EXPLODE_TO_SPACE_STAGE_0 = 0x1B,
  OBJ_EXPLODE_TO_SPACE_STAGE_1 = 0x1C,
  OBJ_EXPLODE_TO_SPACE_STAGE_2 = 0x1D,
  OBJ_EXPLODE_TO_SPACE_STAGE_3 = 0x1E,
  OBJ_EXPLODE_TO_SPACE_STAGE_4 = 0x1F,
  OBJ_EXPLODE_TO_DIAMOND_STAGE_0 = 0x20,
  OBJ_EXPLODE_TO_DIAMOND_STAGE_1 = 0x21,
  OBJ_EXPLODE_TO_DIAMOND_STAGE_2 = 0x22,
  OBJ_EXPLODE_TO_DIAMOND_STAGE_3 = 0x23,
  OBJ_EXPLODE_TO_DIAMOND_STAGE_4 = 0x24,
  OBJ_PRE_ROCKFORD_STAGE_1 = 0x25, // when inbox morphs into Rockford
  OBJ_PRE_ROCKFORD_STAGE_2 = 0x26,
  OBJ_PRE_ROCKFORD_STAGE_3 = 0x27,
  OBJ_PRE_ROCKFORD_STAGE_4 = 0x28,
  OBJ_BUTTERFLY_POSITION_1 = 0x30,
  OBJ_BUTTERFLY_POSITION_2 = 0x31,
  OBJ_BUTTERFLY_POSITION_3 = 0x32,
  OBJ_BUTTERFLY_POSITION_4 = 0x33,
  OBJ_BUTTERFLY_POSITION_1_SCANNED = 0x34,
  OBJ_BUTTERFLY_POSITION_2_SCANNED = 0x35,
  OBJ_BUTTERFLY_POSITION_3_SCANNED = 0x36,
  OBJ_BUTTERFLY_POSITION_4_SCANNED = 0x37,
  OBJ_ROCKFORD = 0x38,
  OBJ_ROCKFORD_SCANNED = 0x39,
  OBJ_AMOEBA = 0x3A,
  OBJ_AMOEBA_SCANNED = 0x3B,
} CaveObject;

typedef struct {
  uint8_t caveNumber;
  uint8_t magicWallMillingTime; // also max amoeba time at 3% growth
  uint8_t initialDiamondValue;
  uint8_t extraDiamondValue;
  uint8_t randomiserSeed[NUM_DIFFICULTY_LEVELS];
  uint8_t diamondsNeeded[NUM_DIFFICULTY_LEVELS];
  uint8_t caveTime[NUM_DIFFICULTY_LEVELS];
  uint8_t backgroundColor1;
  uint8_t backgroundColor2;
  uint8_t foregroundColor;
  uint8_t unused[2];
  uint8_t randomObject[NUM_RANDOM_OBJECTS];
  uint8_t objectProbability[NUM_RANDOM_OBJECTS];
} CaveInfo;

typedef CaveObject CaveMap[CAVE_HEIGHT][CAVE_WIDTH];

typedef struct {
  CaveInfo info;
  CaveMap map;
} Cave;

#include "data_caves.h"

void nextRandom(int *randSeed1, int *randSeed2) {
  int tempRand1 = (*randSeed1 & 0x0001) * 0x0080;
  int tempRand2 = (*randSeed2 >> 1) & 0x007F;

  int result = (*randSeed2) + (*randSeed2 & 0x0001) * 0x0080;
  int carry = (result > 0x00FF);
  result = result & 0x00FF;

  result = result + carry + 0x13;
  carry = (result > 0x00FF);
  *randSeed2 = result & 0x00FF;

  result = *randSeed1 + carry + tempRand1;
  carry = (result > 0x00FF);
  result = result & 0x00FF;

  result = result + carry + tempRand2;
  *randSeed1 = result & 0x00FF;
}

void drawLine(CaveMap map, CaveObject object, int row, int col, int length, int direction) {
  static int ldx[8] = { 0,  1, 1, 1, 0, -1, -1, -1 };
  static int ldy[8] = { -1, -1, 0, 1, 1,  1,  0, -1 };

  for (int i = 0; i < length; i++) {
    map[row + i*ldy[direction]][col + i*ldx[direction]] = object;
  }
}

void drawFilledRect(CaveMap map, CaveObject object, int row, int col, int width, int height, CaveObject fillObject) {
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      if ((j == 0) || (j == height - 1)) {
        map[row+j][col+i] = object;
      }
      else {
        map[row+j][col+i] = fillObject;
      }
    }
    map[row][col+i] = object;
    map[row+height-1][col+i] = object;
  }
}

void drawRect(CaveMap map, CaveObject object, int row, int col, int width, int height) {
  for (int i = 0; i < width; i++) {
    map[row][col+i] = object;
    map[row+height-1][col+i] = object;
  }
  for (int i = 0; i < height; i++) {
    map[row+i][col] = object;
    map[row+i][col+width-1] = object;
  }
}

Cave decodeCave(uint8_t caveIndex) {
  const uint8_t *caves[] = {
    gCave1, gCave2, gCave3, gCave4, gCave5, gCave6, gCave7, gCave8, gCave9, gCave10,
    gCave11, gCave12, gCave13, gCave14, gCave15, gCave16, gCave17, gCave18, gCave19, gCave20
  };

  assert(caveIndex < ARRAY_LENGTH(caves));

  Cave cave;
  CopyMemory(&cave.info, caves[caveIndex], sizeof(cave.info));

  // Clear out the map
  for (int y = 0; y < CAVE_HEIGHT; y++) {
    for (int x = 0; x < CAVE_WIDTH; x++) {
      cave.map[y][x] = OBJ_STEEL_WALL;
    }
  }

  // Decode random map objects
  {
    int randSeed1 = 0;
    int randSeed2 = cave.info.randomiserSeed[0];

    for (int y = 1; y < CAVE_HEIGHT; y++) {
      for (int x = 0; x < CAVE_WIDTH; x++) {
        CaveObject object = OBJ_DIRT;
        nextRandom(&randSeed1, &randSeed2);
        for (int i = 0; i < NUM_RANDOM_OBJECTS; i++) {
          if (randSeed1 < cave.info.objectProbability[i]) {
            object = cave.info.randomObject[i];
          }
        }
        cave.map[y][x] = object;
      }
    }
  }

  // Decode explicit map data
  {
    const uint8_t *explicitData = caves[caveIndex] + sizeof(CaveInfo);
    const int uselessTopBorderHeight = 2;

    for (int i = 0; explicitData[i] != 0xFF; i++) {
      CaveObject object = (CaveObject)(explicitData[i] & 0x3F);

      switch (3 & (explicitData[i] >> 6)) {
        case 0: {
          int x = explicitData[++i];
          int y = explicitData[++i] - uselessTopBorderHeight;
          cave.map[y][x] = object;
          break;
        }
        case 1: {
          int x = explicitData[++i];
          int y = explicitData[++i] - uselessTopBorderHeight;
          int length = explicitData[++i];
          int direction = explicitData[++i];
          drawLine(cave.map, object, y, x, length, direction);
          break;
        }
        case 2: {
          int x = explicitData[++i];
          int y = explicitData[++i] - uselessTopBorderHeight;
          int width = explicitData[++i];
          int height = explicitData[++i];
          CaveObject fill = (CaveObject)explicitData[++i];
          drawFilledRect(cave.map, object, y, x, width, height, fill);
          break;
        }
        case 3: {
          int x = explicitData[++i];
          int y = explicitData[++i] - uselessTopBorderHeight;
          int width = explicitData[++i];
          int height = explicitData[++i];
          drawRect(cave.map, object, y, x, width, height);
          break;
        }
      }
    }
  }

  // Steel bounds
  drawRect(cave.map, OBJ_STEEL_WALL, 0, 0, CAVE_WIDTH, CAVE_HEIGHT);

  return cave;
}

#define TURN_DURATION 0.15f
#define ROCKFORD_TURNS_TILL_BIRTH 12
#define MAP_UNCOVER_TURNS 40
#define PAUSE_TURNS_BEFORE_FULL_UNCOVER 2
#define TILES_PER_LINE_TO_UNCOVER 3

int mapUncoverTurnsLeft;

typedef struct {
  bool gameIsStarted;
  uint8_t caveNumber;
  int livesLeft;
  int turn;
  float turnTimer;
  int rockfordTurnsTillBirth;
  int diamondsCollected;
  int difficultyLevel;
  int caveTimeLeft;
  int score;
  int pauseTurnsLeft;
  Cave cave;
  CaveMap mapCover;
} GameState;

bool isTileVisible(int tileRow, int tileCol) {
  return
    tileRow >= 0 && tileRow < PLAYFIELD_HEIGHT_IN_TILES &&
    tileCol >= 0 && tileCol < PLAYFIELD_WIDTH_IN_TILES;
}

void debugPrint(char *format, ...) {
  va_list argptr;
  va_start(argptr, format);
  char str[1024];
  vsprintf_s(str, sizeof(str), format, argptr);
  va_end(argptr);
  OutputDebugString(str);
}

LRESULT CALLBACK wndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(wnd, msg, wparam, lparam);
  }
  return 0;
}

int CALLBACK WinMain(HINSTANCE inst, HINSTANCE prevInst, LPSTR cmdLine, int cmdShow) {
  UNREFERENCED_PARAMETER(prevInst);
  UNREFERENCED_PARAMETER(cmdLine);

  WNDCLASS wndClass = {0};
  wndClass.style = CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc = wndProc;
  wndClass.hInstance = inst;
  wndClass.hCursor = LoadCursor(0, IDC_ARROW);
  wndClass.lpszClassName = "Boulder Dash";
  RegisterClass(&wndClass);

  RECT crect = {0};
  crect.right = WINDOW_WIDTH;
  crect.bottom = WINDOW_HEIGHT;

  DWORD wndStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
  AdjustWindowRect(&crect, wndStyle, 0);

  HWND wnd = CreateWindowEx(0, wndClass.lpszClassName, "Boulder Dash", wndStyle, 300, 100,
                            crect.right - crect.left, crect.bottom - crect.top,
                            0, 0, inst, 0);
  ShowWindow(wnd, cmdShow);
  UpdateWindow(wnd);

  // Initialize graphics
  {
    HDC deviceContext = GetDC(wnd);

    gDeviceContext = deviceContext;
    gBackbuffer = malloc(BACKBUFFER_PIXELS);

    gBitmapInfo = malloc(sizeof(BITMAPINFOHEADER) + (PALETTE_COLORS * sizeof(RGBQUAD)));
    gBitmapInfo->bmiHeader.biSize = sizeof(gBitmapInfo->bmiHeader);
    gBitmapInfo->bmiHeader.biWidth = BACKBUFFER_WIDTH;
    gBitmapInfo->bmiHeader.biHeight = -BACKBUFFER_HEIGHT;
    gBitmapInfo->bmiHeader.biPlanes = 1;
    gBitmapInfo->bmiHeader.biBitCount = 4;
    gBitmapInfo->bmiHeader.biCompression = BI_RGB;
    gBitmapInfo->bmiHeader.biClrUsed = PALETTE_COLORS;

    RGBQUAD black  = {0x00, 0x00, 0x00, 0x00};
    RGBQUAD red    = {0x00, 0x00, 0xCC, 0x00};
    //RGBQUAD green  = {0x00, 0xCC, 0x00, 0x00};
    RGBQUAD yellow = {0x00, 0xCC, 0xCC, 0x00};
    //RGBQUAD blue   = {0xCC, 0x00, 0x00, 0x00};
    //RGBQUAD purple = {0xCC, 0x00, 0xCC, 0x00};
    //RGBQUAD cyan   = {0xCC, 0xCC, 0x00, 0x00};
    RGBQUAD gray   = {0xCC, 0xCC, 0xCC, 0x00};
    RGBQUAD white  = {0xFF, 0xFF, 0xFF, 0x00};

    gBitmapInfo->bmiColors[0] = black;
    gBitmapInfo->bmiColors[1] = gray;
    gBitmapInfo->bmiColors[2] = white;
    gBitmapInfo->bmiColors[3] = red;
    gBitmapInfo->bmiColors[4] = yellow;
  }

  float dt = 0.0f;
  float targetFps = 60.0f;
  float maxDt = 1.0f / targetFps;
  LARGE_INTEGER perfcFreq = {0};
  LARGE_INTEGER perfc = {0};
  LARGE_INTEGER perfcPrev = {0};

  QueryPerformanceFrequency(&perfcFreq);
  QueryPerformanceCounter(&perfc);

  bool running = true;
  GameState gameState = {0};

  while (running) {
    perfcPrev = perfc;
    QueryPerformanceCounter(&perfc);
    dt = (float)(perfc.QuadPart - perfcPrev.QuadPart) / (float)perfcFreq.QuadPart;
    //debugPrint("dt: %f\n", dt);
    if (dt > maxDt) {
      dt = maxDt;
    }

    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
      switch (msg.message) {
        case WM_QUIT:
          running = false;
          break;

        case WM_KEYDOWN:
        case WM_KEYUP:
          switch (msg.wParam) {
            case VK_ESCAPE:
              running = false;
              break;
          }
          break;

        default:
          TranslateMessage(&msg);
          DispatchMessage(&msg);
          break;
      }
    }

    //
    // Update
    //

    if (!gameState.gameIsStarted) {
      // Initialize game state

      gameState.gameIsStarted = true;

      gameState.caveNumber = 0;
      gameState.cave = decodeCave(gameState.caveNumber);
      gameState.difficultyLevel = 0;
      gameState.caveTimeLeft = gameState.cave.info.caveTime[gameState.difficultyLevel];

      gameState.livesLeft = 3;
      gameState.score = 0;
      gameState.diamondsCollected = 0;
      gameState.turn = 0;
      gameState.turnTimer = 0;
      gameState.rockfordTurnsTillBirth = ROCKFORD_TURNS_TILL_BIRTH;
      mapUncoverTurnsLeft = MAP_UNCOVER_TURNS;
      gameState.pauseTurnsLeft = 0;

      for (int y = 0; y < CAVE_HEIGHT; ++y) {
        for (int x = 0; x < CAVE_WIDTH; ++x) {
          gameState.mapCover[y][x] = OBJ_STEEL_WALL;
        }
      }
    }

    gameState.turnTimer += dt;
    if (gameState.turnTimer >= TURN_DURATION) {
      gameState.turnTimer -= TURN_DURATION;

      // Do cave turn

      if (gameState.pauseTurnsLeft > 0) {
        gameState.pauseTurnsLeft--;
      }
      else {
        gameState.turn++;

        // Scan cave
        for (int y = 0; y < CAVE_HEIGHT; ++y) {
          for (int x = 0; x < CAVE_WIDTH; ++x) {
            switch (gameState.cave.map[y][x]) {
              case OBJ_PRE_ROCKFORD_STAGE_1:
                if (gameState.rockfordTurnsTillBirth == 0) {
                  gameState.cave.map[y][x] = OBJ_PRE_ROCKFORD_STAGE_2;
                }
                else if (mapUncoverTurnsLeft == 0) {
                  gameState.rockfordTurnsTillBirth--;
                }
                break;

              case OBJ_PRE_ROCKFORD_STAGE_2:
                gameState.cave.map[y][x] = OBJ_PRE_ROCKFORD_STAGE_3;
                break;

              case OBJ_PRE_ROCKFORD_STAGE_3:
                gameState.cave.map[y][x] = OBJ_PRE_ROCKFORD_STAGE_4;
                break;

              case OBJ_PRE_ROCKFORD_STAGE_4:
                gameState.cave.map[y][x] = OBJ_ROCKFORD;
                break;
            }
          }
        }

        // Update map cover
        if (mapUncoverTurnsLeft > 0) {
          mapUncoverTurnsLeft--;
          if (mapUncoverTurnsLeft > 1) {
            for (int y = 0; y < CAVE_HEIGHT; ++y) {
              for (int i = 0; i < TILES_PER_LINE_TO_UNCOVER; ++i) {
                gameState.mapCover[y][rand()%CAVE_WIDTH] = OBJ_SPACE;
              }
            }
          }
          else if (mapUncoverTurnsLeft == 1) {
            gameState.pauseTurnsLeft = PAUSE_TURNS_BEFORE_FULL_UNCOVER;
          }
          else if (mapUncoverTurnsLeft == 0) {
            for (int y = 0; y < CAVE_HEIGHT; ++y) {
              for (int x = 0; x < CAVE_WIDTH; ++x) {
                gameState.mapCover[y][x] = OBJ_SPACE;
              }
            }
          }
        }
      }
    }

    //
    // Render
    //

    // Draw border
    drawFilledRectPx(0, 0, BACKBUFFER_WIDTH - 1, BACKBUFFER_HEIGHT - 1, 0);

    // Draw cave
    for (int y = 0; y < CAVE_HEIGHT; ++y) {
      for (int x = 0; x < CAVE_WIDTH; ++x) {
        if (!isTileVisible(y, x)) {
          continue;
        }
        switch (gameState.cave.map[y][x]) {
          case OBJ_SPACE:
            drawSpaceTile(y, x);
            break;
          case OBJ_STEEL_WALL:
            drawSteelWallTile(y, x, 4, 0);
            break;
          case OBJ_DIRT:
            drawDirtTile(y, x, 3, 0);
            break;
          case OBJ_BRICK_WALL:
            drawBrickWallTile(y, x, 1, 3);
            break;
          case OBJ_BOULDER_STATIONARY:
          case OBJ_BOULDER_FALLING:
            drawBoulderTile(y, x, 4, 0);
            break;
          case OBJ_DIAMOND_STATIONARY:
          case OBJ_DIAMOND_FALLING:
            drawDiamond1Tile(y, x, 2, 0);
            break;
          case OBJ_PRE_ROCKFORD_STAGE_1:
            if (gameState.rockfordTurnsTillBirth > 0) {
              if (gameState.rockfordTurnsTillBirth % 2) {
                drawSteelWallTile(y, x, 4, 0);
              }
              else {
                drawOutboxTile(y, x, 4, 0);
              }
            }
            else {
              drawExplosion1Tile(y, x, 2, 0);
            }
            break;
          case OBJ_PRE_ROCKFORD_STAGE_2:
            drawExplosion2Tile(y, x, 2, 0);
            break;
          case OBJ_PRE_ROCKFORD_STAGE_3:
            drawExplosion3Tile(y, x, 2, 0);
            break;
          case OBJ_PRE_ROCKFORD_STAGE_4:
            drawMovingRockfordTile(y, x, true, gameState.turn);
            break;
          case OBJ_ROCKFORD:
            drawIdleRockfordTile(y, x);
            break;
        }
      }
    }

    // Draw map cover
    for (int y = 0; y < CAVE_HEIGHT; ++y) {
      for (int x = 0; x < CAVE_WIDTH; ++x) {
        if (gameState.mapCover[y][x] == OBJ_SPACE || !isTileVisible(y, x)) {
          continue;
        }
        drawAnimatedSteelWallTile(y, x, 4, 0, gameState.turn);
      }
    }

    // Draw text area
    {
      char text[64];
      if (gameState.rockfordTurnsTillBirth > 0) {
        sprintf_s(text, sizeof(text), "  PLAYER 1,  %d MEN,  ROOM %c/1", gameState.livesLeft, 'A' + gameState.caveNumber);
      }
      else {
        sprintf_s(text, sizeof(text), "   %d*%d   %02d   %03d   %06d",
                  gameState.cave.info.diamondsNeeded[gameState.difficultyLevel],
                  gameState.cave.info.initialDiamondValue,
                  gameState.diamondsCollected,
                  gameState.caveTimeLeft,
                  gameState.score);
      }
      drawText(text);
    }

    // Display backbuffer
    StretchDIBits(gDeviceContext,
                  0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                  0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT,
                  gBackbuffer, gBitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);
  }

  return 0;
}
