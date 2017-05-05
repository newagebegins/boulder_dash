#include <windows.h>
#include <assert.h>
#include <stdint.h>

#include "cave.h"
#include "data_caves.h"

static void nextRandom(int& randSeed1, int& randSeed2) {
    assert((randSeed1 >= 0x00) && (randSeed1 <= 0xFF));
    assert((randSeed2 >= 0x00) && (randSeed2 <= 0xFF));

    int tempRand1 = (randSeed1 & 0x0001) * 0x0080;
    int tempRand2 = (randSeed2 >> 1) & 0x007F;

    int result = (randSeed2) + (randSeed2 & 0x0001) * 0x0080;
    int carry = (result > 0x00FF);
    result = result & 0x00FF;

    result = result + carry + 0x13;
    carry = (result > 0x00FF);
    randSeed2 = result & 0x00FF;

    result = randSeed1 + carry + tempRand1;
    carry = (result > 0x00FF);
    result = result & 0x00FF;

    result = result + carry + tempRand2;
    randSeed1 = result & 0x00FF;

    assert((randSeed1 >= 0x00) && (randSeed1 <= 0xFF));
    assert((randSeed2 >= 0x00) && (randSeed2 <= 0xFF));
}

void placeObject(CaveMap map, CaveObject object, Position pos) {
    assert((pos.x >= 0) && (pos.x < CAVE_WIDTH));
    assert((pos.y >= 0) && (pos.y < CAVE_HEIGHT));
    map[pos.y][pos.x] = object;
}

CaveObject getObject(CaveMap map, Position pos) {
    assert((pos.x >= 0) && (pos.x < CAVE_WIDTH));
    assert((pos.y >= 0) && (pos.y < CAVE_HEIGHT));
    return map[pos.y][pos.x];
}

static void drawLine(CaveMap map, CaveObject object, Position pos, int length, int direction) {
    static int ldx[8] = { 0,  1, 1, 1, 0, -1, -1, -1 };
    static int ldy[8] = { -1, -1, 0, 1, 1,  1,  0, -1 };

    assert((length >= 1) && (length <= CAVE_WIDTH));
    assert((direction >= 0) && (direction < ARRAY_LENGTH(ldx)));

    for (int i = 0; i < length; i++) {
        placeObject(map, object, Position(pos.x + i*ldx[direction], pos.y + i*ldy[direction]));
    }
}

static void drawFilledRect(CaveMap map, CaveObject object, Position pos, int width, int height, CaveObject fillObject) {
    assert((width >= 1) && (width <= CAVE_WIDTH));
    assert((height >= 1) && (height <= CAVE_HEIGHT));

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            if ((j == 0) || (j == height - 1)) {
                placeObject(map, object, Position(pos.x + i, pos.y + j));
            }
            else {
                placeObject(map, fillObject, Position(pos.x + i, pos.y + j));
            }
        }
        placeObject(map, object, Position(pos.x + i, pos.y));
        placeObject(map, object, Position(pos.x + i, pos.y + height - 1));
    }
}

static void drawRect(CaveMap map, CaveObject object, Position pos, int width, int height) {
    assert((width >= 1) && (width <= CAVE_WIDTH));
    assert((height >= 1) && (height <= CAVE_HEIGHT));

    for (int i = 0; i < width; i++) {
        placeObject(map, object, Position(pos.x + i, pos.y));
        placeObject(map, object, Position(pos.x + i, pos.y + height - 1));
    }
    for (int i = 0; i < height; i++) {
        placeObject(map, object, Position(pos.x, pos.y + i));
        placeObject(map, object, Position(pos.x + width - 1, pos.y + i));
    }
}

Cave decodeCave(uint8_t caveIndex) {
    static const uint8_t *caves[] = {
      gCave1, gCave2, gCave3, gCave4, gCave5, gCave6, gCave7, gCave8, gCave9, gCave10,
      gCave11, gCave12, gCave13, gCave14, gCave15, gCave16, gCave17, gCave18, gCave19, gCave20
    };

    assert(caveIndex < ARRAY_LENGTH(caves));

    Cave cave;
    CopyMemory(&cave.info, caves[caveIndex], sizeof(cave.info));

    // Clear out the map
    for (int y = 0; y < CAVE_HEIGHT; y++) {
        for (int x = 0; x < CAVE_WIDTH; x++) {
            placeObject(cave.map, OBJ_STEEL_WALL, Position(x, y));
        }
    }

    // Decode random map objects
    {
        int randSeed1 = 0;
        int randSeed2 = cave.info.randomiserSeed[0];

        for (int y = 1; y < CAVE_HEIGHT; y++) {
            for (int x = 0; x < CAVE_WIDTH; x++) {
                CaveObject object = OBJ_DIRT;
                nextRandom(randSeed1, randSeed2);
                for (int i = 0; i < NUM_RANDOM_OBJECTS; i++) {
                    if (randSeed1 < cave.info.objectProbability[i]) {
                        object = cave.info.randomObject[i];
                    }
                }
                placeObject(cave.map, object, Position(x, y));
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
                    placeObject(cave.map, object, Position(x, y));
                    break;
                }
                case 1: {
                    int x = explicitData[++i];
                    int y = explicitData[++i] - uselessTopBorderHeight;
                    int length = explicitData[++i];
                    int direction = explicitData[++i];
                    drawLine(cave.map, object, Position(x, y), length, direction);
                    break;
                }
                case 2: {
                    int x = explicitData[++i];
                    int y = explicitData[++i] - uselessTopBorderHeight;
                    int width = explicitData[++i];
                    int height = explicitData[++i];
                    CaveObject fill = (CaveObject)explicitData[++i];
                    drawFilledRect(cave.map, object, Position(x, y), width, height, fill);
                    break;
                }
                case 3: {
                    int x = explicitData[++i];
                    int y = explicitData[++i] - uselessTopBorderHeight;
                    int width = explicitData[++i];
                    int height = explicitData[++i];
                    drawRect(cave.map, object, Position(x, y), width, height);
                    break;
                }
            }
        }
    }

    // Steel bounds
    drawRect(cave.map, OBJ_STEEL_WALL, Position(0, 0), CAVE_WIDTH, CAVE_HEIGHT);

    return cave;
}
