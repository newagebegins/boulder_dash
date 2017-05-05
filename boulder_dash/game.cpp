#include <stdbool.h>
#include <stdio.h>

#include "game.h"
#include "graphics.h"
#include "cave.h"

#define TURN_DURATION 0.15f
#define ROCKFORD_TURNS_TILL_BIRTH 12
#define MAP_UNCOVER_TURNS 40
#define PAUSE_TURNS_BEFORE_FULL_UNCOVER 2
#define TILES_PER_LINE_TO_UNCOVER 3

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
    int pauseTurnsLeft;
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

static bool isMapCovered(const GameState& gameState) {
    return gameState.mapUncoverTurnsLeft > 0;
}

static void initGameState(GameState& gameState) {
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
    gameState.mapUncoverTurnsLeft = MAP_UNCOVER_TURNS;
    gameState.pauseTurnsLeft = 0;

    initMapCover(gameState.mapCover);
}

static void updateMapCover(GameState& gameState) {
    if (!isMapCovered(gameState)) {
        return;
    }

    gameState.mapUncoverTurnsLeft--;
    if (gameState.mapUncoverTurnsLeft > 1) {
        for (int y = 0; y < CAVE_HEIGHT; ++y) {
            for (int i = 0; i < TILES_PER_LINE_TO_UNCOVER; ++i) {
                int x = getRandomNumber(0, CAVE_WIDTH - 1);
                gameState.mapCover[y][x] = OBJ_SPACE;
            }
        }
    }
    else if (gameState.mapUncoverTurnsLeft == 1) {
        gameState.pauseTurnsLeft = PAUSE_TURNS_BEFORE_FULL_UNCOVER;
    }
    else if (gameState.mapUncoverTurnsLeft == 0) {
        for (int y = 0; y < CAVE_HEIGHT; ++y) {
            for (int x = 0; x < CAVE_WIDTH; ++x) {
                gameState.mapCover[y][x] = OBJ_SPACE;
            }
        }
    }
}

static void updatePreRockford(GameState& gameState) {
    if (!isMapCovered(gameState)) {
        gameState.rockfordTurnsTillBirth--;
        if (gameState.rockfordTurnsTillBirth < 0) {
            gameState.rockfordTurnsTillBirth = 0;
        }
    }
}

static void updatePause(GameState& gameState) {
    gameState.pauseTurnsLeft--;
    if (gameState.pauseTurnsLeft < 0) {
        gameState.pauseTurnsLeft = 0;
    }
}

static bool isPaused(const GameState& gameState) {
    return gameState.pauseTurnsLeft > 0;
}

static void doCaveTurn(GameState& gameState) {
    if (isPaused(gameState)) {
        updatePause(gameState);
    }
    else {
        gameState.turn++;

        for (int y = 0; y < CAVE_HEIGHT; ++y) {
            for (int x = 0; x < CAVE_WIDTH; ++x) {
                switch (getObject(gameState.cave.map, x, y)) {
                    case OBJ_PRE_ROCKFORD_STAGE_1:
                        updatePreRockford(gameState);
                        break;
                }
            }
        }

        updateMapCover(gameState);
    }
}

static void gameUpdate(GameState& gameState, float dt) {
    if (!gameState.gameIsStarted) {
        initGameState(gameState);
    }

    gameState.turnTimer += dt;
    if (gameState.turnTimer >= TURN_DURATION) {
        gameState.turnTimer -= TURN_DURATION;
        doCaveTurn(gameState);
    }
}

static bool isTileVisible(Position tilePos) {
    return
        tilePos.y >= 0 && tilePos.y < PLAYFIELD_HEIGHT_IN_TILES &&
        tilePos.x >= 0 && tilePos.x < PLAYFIELD_WIDTH_IN_TILES;
}

static bool isBeforeRockfordBirth(const GameState& gameState) {
    return gameState.rockfordTurnsTillBirth > 0;
}

static void drawCave(const GameState& gameState) {
    for (int y = 0; y < CAVE_HEIGHT; ++y) {
        for (int x = 0; x < CAVE_WIDTH; ++x) {
            Position tilePos(x, y);
            if (!isTileVisible(tilePos)) {
                continue;
            }
            switch (gameState.cave.map[y][x]) {
                case OBJ_SPACE:
                    drawSpaceTile(tilePos);
                    break;
                case OBJ_STEEL_WALL:
                    drawSteelWallTile(tilePos, 4, 0);
                    break;
                case OBJ_DIRT:
                    drawDirtTile(tilePos, 3, 0);
                    break;
                case OBJ_BRICK_WALL:
                    drawBrickWallTile(tilePos, 1, 3);
                    break;
                case OBJ_BOULDER_STATIONARY:
                case OBJ_BOULDER_FALLING:
                    drawBoulderTile(tilePos, 4, 0);
                    break;
                case OBJ_DIAMOND_STATIONARY:
                case OBJ_DIAMOND_FALLING:
                    drawDiamond1Tile(tilePos, 2, 0);
                    break;
                case OBJ_PRE_ROCKFORD_STAGE_1:
                    if (isBeforeRockfordBirth(gameState)) {
                        if (gameState.rockfordTurnsTillBirth % 2) {
                            drawSteelWallTile(tilePos, 4, 0);
                        }
                        else {
                            drawOutboxTile(tilePos, 4, 0);
                        }
                    }
                    else {
                        drawExplosion1Tile(tilePos, 2, 0);
                    }
                    break;
            }
        }
    }
}

static void drawTextArea(const GameState& gameState) {
    char text[64];
    if (isBeforeRockfordBirth(gameState)) {
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

static void drawMapCover(const CaveMap mapCover, int turn) {
    for (int y = 0; y < CAVE_HEIGHT; ++y) {
        for (int x = 0; x < CAVE_WIDTH; ++x) {
            Position tilePos(x, y);
            if (mapCover[y][x] == OBJ_SPACE || !isTileVisible(tilePos)) {
                continue;
            }
            drawAnimatedSteelWallTile(tilePos, 4, 0, turn);
        }
    }
}

static void gameRender(const GameState& gameState) {
    drawBorder(0);
    drawCave(gameState);
    drawMapCover(gameState.mapCover, gameState.turn);
    drawTextArea(gameState);
    displayBackbuffer();
}

void gameUpdateAndRender(float dt) {
    static GameState gameState = { 0 };
    gameUpdate(gameState, dt);
    gameRender(gameState);
}
