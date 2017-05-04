#include <stdbool.h>
#include "game.h"
#include "graphics.h"
#include "cave.h"

#define TURN_DURATION 0.15f
#define ROCKFORD_TURNS_TILL_BIRTH 12

typedef struct {
  bool caveIsDecoded;
  float turnTimer;
  int rockfordTurnsTillBirth;
  Cave cave;
} GameState;

static void gameUpdate(GameState *gameState, float dt) {
  UNREFERENCED_PARAMETER(dt);

  if (!gameState->caveIsDecoded) {
    gameState->cave = decodeCave(0);
    gameState->caveIsDecoded = true;
    gameState->turnTimer = 0;
    gameState->rockfordTurnsTillBirth = ROCKFORD_TURNS_TILL_BIRTH;
  }

  gameState->turnTimer += dt;
  if (gameState->turnTimer >= TURN_DURATION) {
    gameState->turnTimer -= TURN_DURATION;

    gameState->rockfordTurnsTillBirth--;
    if (gameState->rockfordTurnsTillBirth < 0) {
      gameState->rockfordTurnsTillBirth = 0;
    }
  }
}

static bool isTileVisible(uint8_t tileRow, uint8_t tileCol) {
  return
    tileRow >= VIEWPORT_Y_MIN_IN_TILES && tileRow <= VIEWPORT_Y_MAX_IN_TILES &&
    tileCol >= VIEWPORT_X_MIN_IN_TILES && tileCol <= VIEWPORT_X_MAX_IN_TILES;
}

static bool isBeforeRockfordBirth(const GameState *gameState) {
  return gameState->rockfordTurnsTillBirth > 0;
}

static void drawCave(const GameState *gameState) {
  for (uint8_t y = 0; y < CAVE_HEIGHT; ++y) {
    for (uint8_t x = 0; x < CAVE_WIDTH; ++x) {
      uint8_t tileRow = VIEWPORT_X_MIN_IN_TILES + y;
      uint8_t tileCol = VIEWPORT_Y_MIN_IN_TILES + x;
      if (!isTileVisible(tileRow, tileCol)) {
        continue;
      }
      switch (gameState->cave.map[y][x]) {
        case OBJ_SPACE:
          drawSpaceTile(tileRow, tileCol);
          break;
        case OBJ_STEEL_WALL:
          drawSteelWallTile(tileRow, tileCol, 4, 0);
          break;
        case OBJ_DIRT:
          drawDirtTile(tileRow, tileCol, 3, 0);
          break;
        case OBJ_BRICK_WALL:
          drawBrickWallTile(tileRow, tileCol, 1, 3);
          break;
        case OBJ_BOULDER_STATIONARY:
        case OBJ_BOULDER_FALLING:
          drawBoulderTile(tileRow, tileCol, 4, 0);
          break;
        case OBJ_DIAMOND_STATIONARY:
        case OBJ_DIAMOND_FALLING:
          drawDiamond1Tile(tileRow, tileCol, 2, 0);
          break;
        case OBJ_PRE_ROCKFORD_STAGE_1:
          if (isBeforeRockfordBirth(gameState)) {
            if (gameState->rockfordTurnsTillBirth % 2) {
              drawSteelWallTile(tileRow, tileCol, 4, 0);
            } else {
              drawOutboxTile(tileRow, tileCol, 4, 0);
            }
          } else {
            drawExplosion1Tile(tileRow, tileCol, 2, 0);
          }
          break;
      }
    }
  }
}

static void gameRender(const GameState *gameState) {
  drawBorder(0);
  drawCave(gameState);
  if (isBeforeRockfordBirth(gameState)) {
    drawText("  PLAYER 1,  3 MEN,  ROOM A/1");
  }
  displayBackbuffer();
}

void gameUpdateAndRender(float dt) {
  static GameState gameState;
  gameUpdate(&gameState, dt);
  gameRender(&gameState);
}
