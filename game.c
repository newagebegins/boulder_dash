#include <stdbool.h>
#include <stdio.h>

#include "game.h"
#include "graphics.h"
#include "cave.h"

#define TURN_DURATION 0.15f
#define ROCKFORD_TURNS_TILL_BIRTH 12

typedef struct {
  bool gameIsStarted;
  uint8_t caveNumber;
  int livesLeft;
  float turnTimer;
  int rockfordTurnsTillBirth;
  int diamondsCollected;
  int difficultyLevel;
  int caveTimeLeft;
  int score;
  Cave cave;
} GameState;

static void initializeGameState(GameState *gameState) {
  gameState->gameIsStarted = true;

  gameState->caveNumber = 0;
  gameState->cave = decodeCave(gameState->caveNumber);
  gameState->difficultyLevel = 0;
  gameState->caveTimeLeft = gameState->cave.info.caveTime[gameState->difficultyLevel];

  gameState->livesLeft = 3;
  gameState->score = 0;
  gameState->diamondsCollected = 0;
  gameState->turnTimer = 0;
  gameState->rockfordTurnsTillBirth = ROCKFORD_TURNS_TILL_BIRTH;
}

static void doCaveTurn(GameState *gameState) {
  gameState->rockfordTurnsTillBirth--;
  if (gameState->rockfordTurnsTillBirth < 0) {
    gameState->rockfordTurnsTillBirth = 0;
  }
}

static void gameUpdate(GameState *gameState, float dt) {
  if (!gameState->gameIsStarted) {
    initializeGameState(gameState);
  }

  gameState->turnTimer += dt;
  if (gameState->turnTimer >= TURN_DURATION) {
    gameState->turnTimer -= TURN_DURATION;
    doCaveTurn(gameState);
  }
}

static bool isTileVisible(int tileRow, int tileCol) {
  return
    tileRow >= 0 && tileRow < PLAYFIELD_HEIGHT_IN_TILES &&
    tileCol >= 0 && tileCol < PLAYFIELD_WIDTH_IN_TILES;
}

static bool isBeforeRockfordBirth(const GameState *gameState) {
  return gameState->rockfordTurnsTillBirth > 0;
}

static void drawCave(const GameState *gameState) {
  for (int y = 0; y < CAVE_HEIGHT; ++y) {
    for (int x = 0; x < CAVE_WIDTH; ++x) {
      int tileRow = y;
      int tileCol = x;
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

static void drawTextArea(const GameState *gameState) {
  char text[64];
  if (isBeforeRockfordBirth(gameState)) {
    sprintf_s(text, sizeof(text), "  PLAYER 1,  %d MEN,  ROOM %c/1", gameState->livesLeft, 'A' + gameState->caveNumber);
  } else {
    sprintf_s(text, sizeof(text), "   %d*%d   %02d   %03d   %06d",
              gameState->cave.info.diamondsNeeded[gameState->difficultyLevel],
              gameState->cave.info.initialDiamondValue,
              gameState->diamondsCollected,
              gameState->caveTimeLeft,
              gameState->score);
  }
  drawText(text);
}

static void gameRender(const GameState *gameState) {
  drawBorder(0);
  drawCave(gameState);
  drawTextArea(gameState);
  displayBackbuffer();
}

void gameUpdateAndRender(float dt) {
  static GameState gameState = {0};
  gameUpdate(&gameState, dt);
  gameRender(&gameState);
}
