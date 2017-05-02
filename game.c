#include <stdbool.h>
#include "game.h"
#include "graphics.h"
#include "cave.h"

typedef struct {
  bool caveIsDecoded;
  Cave cave;
} GameState;

static void gameUpdate(GameState *gameState, float dt) {
  UNREFERENCED_PARAMETER(dt);

  if (!gameState->caveIsDecoded) {
    gameState->cave = decodeCave(0);
    gameState->caveIsDecoded = true;
  }
}

static bool isTileVisible(uint8_t tileRow, uint8_t tileCol) {
  return
    tileRow >= VIEWPORT_Y_MIN_IN_TILES && tileRow <= VIEWPORT_Y_MAX_IN_TILES &&
    tileCol >= VIEWPORT_X_MIN_IN_TILES && tileCol <= VIEWPORT_X_MAX_IN_TILES;
}

static void drawCave(CaveMap map) {
  for (uint8_t y = 0; y < CAVE_HEIGHT; ++y) {
    for (uint8_t x = 0; x < CAVE_WIDTH; ++x) {
      uint8_t tileRow = VIEWPORT_X_MIN_IN_TILES + y;
      uint8_t tileCol = VIEWPORT_Y_MIN_IN_TILES + x;
      if (!isTileVisible(tileRow, tileCol)) {
        continue;
      }
      switch (map[y][x]) {
        case OBJ_SPACE:
          drawSpaceTile(tileRow, tileCol);
          break;
        case OBJ_STEEL_WALL:
          drawTile(gSpriteSteelWall, gSpriteSteelWall, gSpriteSteelWall, gSpriteSteelWall, tileRow, tileCol, 4, 0);
          break;
        case OBJ_DIRT:
          drawTile(gSpriteDirtA, gSpriteDirtB, gSpriteDirtC, gSpriteDirtD, tileRow, tileCol, 3, 0);
          break;
        case OBJ_BRICK_WALL:
          drawTile(gSpriteBrick1, gSpriteBrick1, gSpriteBrick1, gSpriteBrick1, tileRow, tileCol, 1, 3);
          break;
        case OBJ_BOULDER_STATIONARY:
        case OBJ_BOULDER_FALLING:
          drawTile(gSpriteBoulderA, gSpriteBoulderB, gSpriteBoulderC, gSpriteBoulderD, tileRow, tileCol, 4, 0);
          break;
        case OBJ_DIAMOND_STATIONARY:
        case OBJ_DIAMOND_FALLING:
          drawTile(gSpriteDiamond1A, gSpriteDiamond1B, gSpriteDiamond1C, gSpriteDiamond1D, tileRow, tileCol, 2, 0);
          break;
      }
    }
  }
}

static void gameRender(CaveMap map) {
  drawBorder(0);
  drawCave(map);
  displayBackbuffer();
}

void gameUpdateAndRender(float dt) {
  static GameState gameState;
  gameUpdate(&gameState, dt);
  gameRender(gameState.cave.map);
}
