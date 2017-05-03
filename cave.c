#include <windows.h>
#include <assert.h>
#include <stdint.h>

#include "cave.h"
#include "common.h"
#include "data_caves.h"

static void nextRandom(int *randSeed1, int *randSeed2) {
  assert((*randSeed1 >= 0x00) && (*randSeed1 <= 0xFF));
  assert((*randSeed2 >= 0x00) && (*randSeed2 <= 0xFF));

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

  assert((*randSeed1 >= 0x00) && (*randSeed1 <= 0xFF));
  assert((*randSeed2 >= 0x00) && (*randSeed2 <= 0xFF));
}

static void storeObject(CaveMap map, int x, int y, CaveObject object) {
  assert((x >= 0) && (x < CAVE_WIDTH));
  assert((y >= 0) && (y < CAVE_HEIGHT));
  map[y][x] = object;
}

static void drawLine(CaveMap map, CaveObject object, int x, int y, int length, int direction) {
  static int ldx[8]={ 0,  1, 1, 1, 0, -1, -1, -1};
  static int ldy[8]={-1, -1, 0, 1, 1,  1,  0, -1};

  assert((length >= 1) && (length <= CAVE_WIDTH));
  assert((direction >= 0) && (direction < ARRAY_LENGTH(ldx)));

  for (int counter = 1; counter <= length; counter++) {
    storeObject(map, x, y, object);
    x += ldx[direction];
    y += ldy[direction];
  }
}

static void drawFilledRect(CaveMap map, CaveObject object, int x, int y, int width, int height, CaveObject fillObject) {
  assert((width >= 1) && (width <= CAVE_WIDTH));
  assert((height >= 1) && (height <= CAVE_HEIGHT));

  for(int counter1 = 0; counter1 < width; counter1++) {
    for(int counter2 = 0; counter2 < height; counter2++) {
      if ((counter2 == 0) || (counter2 == height-1)) {
        storeObject(map, x+counter1, y+counter2, object);
      } else {
        storeObject(map, x+counter1, y+counter2, fillObject);
      }
    }
    storeObject(map, x+counter1, y, object);
    storeObject(map, x+counter1, y+height-1, object);
  }
}

static void drawRect(CaveMap map, CaveObject object, int x, int y, int width, int height) {
  assert((width >= 1) && (width <= CAVE_WIDTH));
  assert((height >= 1) && (height <= CAVE_HEIGHT));

  for(int counter1 = 0; counter1 < width; counter1++) {
    storeObject(map, x + counter1, y, object);
    storeObject(map, x + counter1, y + height-1, object);
  }
  for(int counter1 = 0; counter1 < height; counter1++) {
    storeObject(map, x, y + counter1, object);
    storeObject(map, x + width-1, y + counter1, object);
  }
}

Cave decodeCave(uint8_t caveIndex) {
  static const uint8_t *caves[] = {
    gCave1, gCave2, gCave3, gCave4, gCave5, gCave6, gCave7, gCave8, gCave9, gCave10,
    gCave11, gCave12, gCave13, gCave14, gCave15, gCave16, gCave17, gCave18, gCave19, gCave20
  };

  assert(caveIndex < ARRAY_LENGTH(caves));
  uint8_t const *caveData = caves[caveIndex];

  Cave cave;
  CopyMemory(&cave.info, caveData, sizeof(cave.info));

  uint8_t theWidth, theHeight, theFill, theLength, theDirection;
  int x, y;

  int randSeed1 = 0;
  int randSeed2 = cave.info.randomiserSeed[0];

  // Clear out the map
  for (y = 0; y < CAVE_HEIGHT; y++) {
    for(x = 0; x < CAVE_WIDTH; x++) {
      storeObject(cave.map, x, y, OBJ_STEEL_WALL);
    }
  }

  // Decode random map objects
  for(y = 3; y < CAVE_HEIGHT; y++) {
    for(x = 0; x < CAVE_WIDTH; x++) {
      CaveObject object = OBJ_DIRT;
      nextRandom(&randSeed1, &randSeed2);
      for (int i = 0; i < NUM_RANDOM_OBJECTS; i++) {
        if (randSeed1 < cave.info.objectProbability[i]) {
          object = cave.info.randomObject[i];
        }
      }
      storeObject(cave.map, x, y, object);
    }
  }

  // Decode explicit map data
  for (int caveDataIndex = 0x20; caveData[caveDataIndex] != 0xFF; caveDataIndex++) {
    int theCode = caveData[caveDataIndex];
    uint8_t object = theCode & 0x3F;

    switch(3 & (caveData[caveDataIndex] >> 6)) {
      case 0: /* PLOT */
        x = caveData[++caveDataIndex];
        y = caveData[++caveDataIndex];
        storeObject(cave.map, x, y, object);
        break;

      case 1: /* LINE */
        x = caveData[++caveDataIndex];
        y = caveData[++caveDataIndex];
        theLength = caveData[++caveDataIndex];
        theDirection = caveData[++caveDataIndex];
        drawLine(cave.map, object, x, y, theLength, theDirection);
        break;

      case 2: /* FILLED RECTANGLE */
        x = caveData[++caveDataIndex];
        y = caveData[++caveDataIndex];
        theWidth = caveData[++caveDataIndex];
        theHeight = caveData[++caveDataIndex];
        theFill = caveData[++caveDataIndex];
        drawFilledRect(cave.map, object, x, y, theWidth, theHeight, theFill);
        break;

      case 3: /* OPEN RECTANGLE */
        x = caveData[++caveDataIndex];
        y = caveData[++caveDataIndex];
        theWidth = caveData[++caveDataIndex];
        theHeight = caveData[++caveDataIndex];
        drawRect(cave.map, object, x, y, theWidth, theHeight);
        break;
    }
  }

  /* SteelBounds */
  drawRect(cave.map, 0x07, 0, 2, 40, 22);

  return cave;
}
