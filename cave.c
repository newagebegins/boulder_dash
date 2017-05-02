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

static void StoreObject(uint8_t caveData[CAVE_HEIGHT][CAVE_WIDTH], int x, int y, uint8_t anObject) {
  assert(((x >= 0) && (x <= 39)));
  assert(((y >= 0) && (y <= 23)));
  assert(((anObject >= 0) && (anObject <= 63)));
  caveData[y][x] = anObject;
}

static void DrawLine(uint8_t caveData[CAVE_HEIGHT][CAVE_WIDTH], uint8_t anObject, int x, int y, int aLength, int aDirection) {
  static int ldx[8]={ 0,  1, 1, 1, 0, -1, -1, -1};
  static int ldy[8]={-1, -1, 0, 1, 1,  1,  0, -1};

  assert(((anObject >= 0) && (anObject <= 63)));
  assert(((x >= 0) && (x <= 39)));
  assert(((y >= 0) && (y <= 23)));
  assert(((aLength >= 1) && (aLength <= 40)));
  assert(((aDirection >= 0) && (aDirection <= 7)));

  for (int counter = 1; counter <= aLength; counter++) {
    StoreObject(caveData, x, y, anObject);
    x += ldx[aDirection];
    y += ldy[aDirection];
  }
}

static void DrawFilledRect(uint8_t caveData[CAVE_HEIGHT][CAVE_WIDTH], uint8_t anObject, int x, int y, int aWidth, int aHeight, uint8_t aFillObject) {
  assert(((anObject >= 0) && (anObject <= 63)));
  assert(((x >= 0) && (x <= 39)));
  assert(((y >= 0) && (y <= 23)));
  assert(((aWidth >= 1) && (aWidth <= 40)));
  assert(((aHeight >= 1) && (aHeight <= 24)));
  assert(((aFillObject >= 0) && (aFillObject <= 63)));

  for(int counter1 = 0; counter1 < aWidth; counter1++) {
    for(int counter2 = 0; counter2 < aHeight; counter2++) {
      if ((counter2 == 0) || (counter2 == aHeight-1)) {
        StoreObject(caveData, x+counter1, y+counter2, anObject);
      } else {
        StoreObject(caveData, x+counter1, y+counter2, aFillObject);
      }
    }
    StoreObject(caveData, x+counter1, y, anObject);
    StoreObject(caveData, x+counter1, y+aHeight-1, anObject);
  }
}

static void DrawRect(uint8_t caveData[CAVE_HEIGHT][CAVE_WIDTH], uint8_t anObject, int x, int y, int aWidth, int aHeight) {
  assert(((anObject >= 0) && (anObject <= 63)));
  assert(((x >= 0) && (x <= 39)));
  assert(((y >= 0) && (y <= 23)));
  assert(((aWidth >= 1) && (aWidth <= 40)));
  assert(((aHeight >= 1) && (aHeight <= 24)));

  for(int counter1 = 0; counter1 < aWidth; counter1++) {
    StoreObject(caveData, x + counter1, y, anObject);
    StoreObject(caveData, x + counter1, y + aHeight-1, anObject);
  }
  for(int counter1 = 0; counter1 < aHeight; counter1++) {
    StoreObject(caveData, x, y + counter1, anObject);
    StoreObject(caveData, x + aWidth-1, y + counter1, anObject);
  }
}

Cave decodeCave(uint8_t caveIndex) {
  static const uint8_t *caves[] = {
    gCave1, gCave2, gCave3, gCave4, gCave5, gCave6, gCave7, gCave8, gCave9, gCave10, gCave11,
    gCave12, gCave13, gCave14, gCave15, gCave16, gCave17, gCave18, gCave19, gCave20
  };

  assert(caveIndex < ARRAY_LENGTH(caves));
  uint8_t const *aCaveData = caves[caveIndex];

  Cave cave;
  CopyMemory(&cave.info, aCaveData, sizeof(cave.info));

  uint8_t theWidth, theHeight, theFill, theLength, theDirection;
  int x, y;

  int RandSeed1 = 0;
  int RandSeed2 = aCaveData[4];

  /* Clear out the cave data to a null value */
  for(x = 0; x < 40; x++) {
    for (y = 0; y <= 23; y++) {
      StoreObject(cave.map, x, y, 0x07);
    }
  }

  /* Decode the random cave data */
  for(y = 3; y <= 23; y++) {
    for(x = 0; x <= 39; x++) {
      uint8_t theObject = 1;  /* Dirt */
      NextRandom(&RandSeed1, &RandSeed2);
      for (int caveDataIndex = 0; caveDataIndex <= 3; caveDataIndex++) {
        if (RandSeed1 < aCaveData[0x1C + caveDataIndex]) {
          theObject = aCaveData[0x18 + caveDataIndex];
        }
      }
      StoreObject(cave.map, x, y, theObject);
    }
  }

  /* Decode the explicit cave data */
  for (int caveDataIndex = 0x20; aCaveData[caveDataIndex] != 0xFF; caveDataIndex++) {
    int theCode = aCaveData[caveDataIndex];
    uint8_t theObject = theCode & 0x3F;

    switch(3 & (aCaveData[caveDataIndex] >> 6)) {
      case 0: /* PLOT */
        x = aCaveData[++caveDataIndex];
        y = aCaveData[++caveDataIndex];
        StoreObject(cave.map, x, y, theObject);
        break;

      case 1: /* LINE */
        x = aCaveData[++caveDataIndex];
        y = aCaveData[++caveDataIndex];
        theLength = aCaveData[++caveDataIndex];
        theDirection = aCaveData[++caveDataIndex];
        DrawLine(cave.map, theObject, x, y, theLength, theDirection);
        break;

      case 2: /* FILLED RECTANGLE */
        x = aCaveData[++caveDataIndex];
        y = aCaveData[++caveDataIndex];
        theWidth = aCaveData[++caveDataIndex];
        theHeight = aCaveData[++caveDataIndex];
        theFill = aCaveData[++caveDataIndex];
        DrawFilledRect(cave.map, theObject, x, y, theWidth, theHeight, theFill);
        break;

      case 3: /* OPEN RECTANGLE */
        x = aCaveData[++caveDataIndex];
        y = aCaveData[++caveDataIndex];
        theWidth = aCaveData[++caveDataIndex];
        theHeight = aCaveData[++caveDataIndex];
        DrawRect(cave.map, theObject, x, y, theWidth, theHeight);
        break;
    }
  }

  /* SteelBounds */
  DrawRect(cave.map, 0x07, 0, 2, 40, 22);

  return cave;
}
