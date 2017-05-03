#include <windows.h>
#include <assert.h>
#include <stdint.h>

#include "cave.h"
#include "common.h"
#include "data_caves.h"

static void NextRandom(int *RandSeed1, int *RandSeed2) {
  assert(((*RandSeed1 >= 0x00) && (*RandSeed1 <= 0xFF)));
  assert(((*RandSeed2 >= 0x00) && (*RandSeed2 <= 0xFF)));

  int TempRand1 = (*RandSeed1 & 0x0001) * 0x0080;
  int TempRand2 = (*RandSeed2 >> 1) & 0x007F;

  int result = (*RandSeed2) + (*RandSeed2 & 0x0001) * 0x0080;
  int carry = (result > 0x00FF);
  result = result & 0x00FF;

  result = result + carry + 0x13;
  carry = (result > 0x00FF);
  *RandSeed2 = result & 0x00FF;

  result = *RandSeed1 + carry + TempRand1;
  carry = (result > 0x00FF);
  result = result & 0x00FF;

  result = result + carry + TempRand2;
  *RandSeed1 = result & 0x00FF;

  assert(((*RandSeed1 >= 0x00) && (*RandSeed1 <= 0xFF)));
  assert(((*RandSeed2 >= 0x00) && (*RandSeed2 <= 0xFF)));
}

static void StoreObject(uint8_t caveData[CAVE_HEIGHT][CAVE_WIDTH], int x, int y, CaveObject object) {
  assert((x >= 0) && (x < CAVE_WIDTH));
  assert((y >= 0) && (y < CAVE_HEIGHT));
  caveData[y][x] = object;
}

static void DrawLine(uint8_t caveData[CAVE_HEIGHT][CAVE_WIDTH], CaveObject object, int x, int y, int aLength, int aDirection) {
  static int ldx[8]={ 0,  1, 1, 1, 0, -1, -1, -1};
  static int ldy[8]={-1, -1, 0, 1, 1,  1,  0, -1};

  assert(((aLength >= 1) && (aLength <= 40)));
  assert(((aDirection >= 0) && (aDirection <= 7)));

  for (int counter = 1; counter <= aLength; counter++) {
    StoreObject(caveData, x, y, object);
    x += ldx[aDirection];
    y += ldy[aDirection];
  }
}

static void DrawFilledRect(uint8_t caveData[CAVE_HEIGHT][CAVE_WIDTH], CaveObject object, int x, int y, int aWidth, int aHeight, CaveObject fillObject) {
  assert(((aWidth >= 1) && (aWidth <= 40)));
  assert(((aHeight >= 1) && (aHeight <= 24)));

  for(int counter1 = 0; counter1 < aWidth; counter1++) {
    for(int counter2 = 0; counter2 < aHeight; counter2++) {
      if ((counter2 == 0) || (counter2 == aHeight-1)) {
        StoreObject(caveData, x+counter1, y+counter2, object);
      } else {
        StoreObject(caveData, x+counter1, y+counter2, fillObject);
      }
    }
    StoreObject(caveData, x+counter1, y, object);
    StoreObject(caveData, x+counter1, y+aHeight-1, object);
  }
}

static void DrawRect(uint8_t caveData[CAVE_HEIGHT][CAVE_WIDTH], CaveObject object, int x, int y, int aWidth, int aHeight) {
  assert(((aWidth >= 1) && (aWidth <= 40)));
  assert(((aHeight >= 1) && (aHeight <= 24)));

  for(int counter1 = 0; counter1 < aWidth; counter1++) {
    StoreObject(caveData, x + counter1, y, object);
    StoreObject(caveData, x + counter1, y + aHeight-1, object);
  }
  for(int counter1 = 0; counter1 < aHeight; counter1++) {
    StoreObject(caveData, x, y + counter1, object);
    StoreObject(caveData, x + aWidth-1, y + counter1, object);
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

  for (y = 0; y < CAVE_HEIGHT; y++) {
    for(x = 0; x < CAVE_WIDTH; x++) {
      StoreObject(cave.map, x, y, OBJ_STEEL_WALL);
    }
  }

  /* Decode the random cave data */
  for(y = 3; y <= 23; y++) {
    for(x = 0; x <= 39; x++) {
      uint8_t theObject = 1;  /* Dirt */
      NextRandom(&randSeed1, &randSeed2);
      for (int caveDataIndex = 0; caveDataIndex <= 3; caveDataIndex++) {
        if (randSeed1 < caveData[0x1C + caveDataIndex]) {
          theObject = caveData[0x18 + caveDataIndex];
        }
      }
      StoreObject(cave.map, x, y, theObject);
    }
  }

  /* Decode the explicit cave data */
  for (int caveDataIndex = 0x20; caveData[caveDataIndex] != 0xFF; caveDataIndex++) {
    int theCode = caveData[caveDataIndex];
    uint8_t theObject = theCode & 0x3F;

    switch(3 & (caveData[caveDataIndex] >> 6)) {
      case 0: /* PLOT */
        x = caveData[++caveDataIndex];
        y = caveData[++caveDataIndex];
        StoreObject(cave.map, x, y, theObject);
        break;

      case 1: /* LINE */
        x = caveData[++caveDataIndex];
        y = caveData[++caveDataIndex];
        theLength = caveData[++caveDataIndex];
        theDirection = caveData[++caveDataIndex];
        DrawLine(cave.map, theObject, x, y, theLength, theDirection);
        break;

      case 2: /* FILLED RECTANGLE */
        x = caveData[++caveDataIndex];
        y = caveData[++caveDataIndex];
        theWidth = caveData[++caveDataIndex];
        theHeight = caveData[++caveDataIndex];
        theFill = caveData[++caveDataIndex];
        DrawFilledRect(cave.map, theObject, x, y, theWidth, theHeight, theFill);
        break;

      case 3: /* OPEN RECTANGLE */
        x = caveData[++caveDataIndex];
        y = caveData[++caveDataIndex];
        theWidth = caveData[++caveDataIndex];
        theHeight = caveData[++caveDataIndex];
        DrawRect(cave.map, theObject, x, y, theWidth, theHeight);
        break;
    }
  }

  /* SteelBounds */
  DrawRect(cave.map, 0x07, 0, 2, 40, 22);

  return cave;
}
