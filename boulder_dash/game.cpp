#include <stdbool.h>
#include <stdio.h>

#include "game.h"
#include "graphics.h"
#include "cave.h"

#define TURN_DURATION 0.15f
#define ROCKFORD_TURNS_TILL_BIRTH 12
#define MAP_UNCOVER_TURNS 69

struct GameState {
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
  int mapUncoverTurnsLeft;
  Cave cave;
  CaveMap mapCover;
};

static int getRandomNumber(int min, int max) {
  return min + rand() % (max - min + 1);
}

static void initMapCover(CaveMap mapCover) {
  for (int y = 0; y < CAVE_HEIGHT; ++y) {
    for (int x = 0; x < CAVE_WIDTH; ++x) {
      mapCover[y][x] = OBJ_STEEL_WALL;
    }
  }
}

static void initGameState(GameState *gameState) {
  gameState->gameIsStarted = true;

  gameState->caveNumber = 0;
  gameState->cave = decodeCave(gameState->caveNumber);
  gameState->difficultyLevel = 0;
  gameState->caveTimeLeft = gameState->cave.info.caveTime[gameState->difficultyLevel];

  gameState->livesLeft = 3;
  gameState->score = 0;
  gameState->diamondsCollected = 0;
  gameState->turn = 0;
  gameState->turnTimer = 0;
  gameState->rockfordTurnsTillBirth = ROCKFORD_TURNS_TILL_BIRTH;
  gameState->mapUncoverTurnsLeft = MAP_UNCOVER_TURNS;

  initMapCover(gameState->mapCover);
}

static bool isMapUncovered(int mapUncoverTurnsLeft) {
  return mapUncoverTurnsLeft <= 0;
}

static void updateMapCover(CaveMap mapCover, int *mapUncoverTurnsLeft) {
  if (isMapUncovered(*mapUncoverTurnsLeft)) {
    return;
  }

  (*mapUncoverTurnsLeft)--;
  if (*mapUncoverTurnsLeft == 0) {
    for (int y = 0; y < CAVE_HEIGHT; ++y) {
      for (int x = 0; x < CAVE_WIDTH; ++x) {
        mapCover[y][x] = OBJ_SPACE;
      }
    }
  } else {
    for (int y = 0; y < CAVE_HEIGHT; ++y) {
      int x = getRandomNumber(0, CAVE_WIDTH-1);
      mapCover[y][x] = OBJ_SPACE;
    }
  }
}

static void doCaveTurn(GameState *gameState) {
  gameState->turn++;

  if (isMapUncovered(gameState->mapUncoverTurnsLeft)) {
    gameState->rockfordTurnsTillBirth--;
    if (gameState->rockfordTurnsTillBirth < 0) {
      gameState->rockfordTurnsTillBirth = 0;
    }
  }

  updateMapCover(gameState->mapCover, &gameState->mapUncoverTurnsLeft);
}

static void gameUpdate(GameState *gameState, float dt) {
  if (!gameState->gameIsStarted) {
    initGameState(gameState);
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

static void drawMapCover(const CaveMap mapCover, int turn) {
  for (int y = 0; y < CAVE_HEIGHT; ++y) {
    for (int x = 0; x < CAVE_WIDTH; ++x) {
      if (mapCover[y][x] == OBJ_SPACE || !isTileVisible(y, x)) {
        continue;
      }
      drawAnimatedSteelWallTile(y, x, 4, 0, turn);
    }
  }
}

static void gameRender(const GameState *gameState) {
  drawBorder(0);
  drawCave(gameState);
  drawMapCover(gameState->mapCover, gameState->turn);
  drawTextArea(gameState);
  displayBackbuffer();
}

void gameUpdateAndRender(float dt) {
  static GameState gameState = {0};
  gameUpdate(&gameState, dt);
  gameRender(&gameState);
}
