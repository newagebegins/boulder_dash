#ifndef CAVE_H
#define CAVE_H

#define CAVE_HEIGHT 24
#define CAVE_WIDTH 40
#define NUM_DIFFICULTY_LEVELS 5
#define NUM_RANDOM_OBJECTS 4

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
} CaveHeader;

typedef struct {
  CaveHeader *header;
  uint8_t data[CAVE_HEIGHT][CAVE_WIDTH];
} Cave;

Cave getCave(int caveIndex);

#endif
