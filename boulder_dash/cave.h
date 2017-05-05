#ifndef CAVE_H
#define CAVE_H

#define CAVE_HEIGHT 22
#define CAVE_WIDTH 40
#define NUM_DIFFICULTY_LEVELS 5
#define NUM_RANDOM_OBJECTS 4

enum CaveObject : uint8_t {
    OBJ_SPACE = 0x00,
    OBJ_DIRT = 0x01,
    OBJ_BRICK_WALL = 0x02,
    OBJ_MAGIC_WALL = 0x03,
    OBJ_PRE_OUTBOX = 0x04, // before it starts flashing
    OBJ_FLASHING_OUTBOX = 0x05,
    OBJ_STEEL_WALL = 0x07,
    OBJ_FIREFLY_POSITION_1 = 0x08,
    OBJ_FIREFLY_POSITION_2 = 0x09,
    OBJ_FIREFLY_POSITION_3 = 0x0A,
    OBJ_FIREFLY_POSITION_4 = 0x0B,
    OBJ_FIREFLY_POSITION_1_SCANNED = 0x0C,
    OBJ_FIREFLY_POSITION_2_SCANNED = 0x0D,
    OBJ_FIREFLY_POSITION_3_SCANNED = 0x0E,
    OBJ_FIREFLY_POSITION_4_SCANNED = 0x0F,
    OBJ_BOULDER_STATIONARY = 0x10,
    OBJ_BOULDER_STATIONARY_SCANNED = 0x11,
    OBJ_BOULDER_FALLING = 0x12,
    OBJ_BOULDER_FALLING_SCANNED = 0x13,
    OBJ_DIAMOND_STATIONARY = 0x14,
    OBJ_DIAMOND_STATIONARY_SCANNED = 0x15,
    OBJ_DIAMOND_FALLING = 0x16,
    OBJ_DIAMOND_FALLING_SCANNED = 0x17,
    OBJ_EXPLODE_TO_SPACE_STAGE_0 = 0x1B,
    OBJ_EXPLODE_TO_SPACE_STAGE_1 = 0x1C,
    OBJ_EXPLODE_TO_SPACE_STAGE_2 = 0x1D,
    OBJ_EXPLODE_TO_SPACE_STAGE_3 = 0x1E,
    OBJ_EXPLODE_TO_SPACE_STAGE_4 = 0x1F,
    OBJ_EXPLODE_TO_DIAMOND_STAGE_0 = 0x20,
    OBJ_EXPLODE_TO_DIAMOND_STAGE_1 = 0x21,
    OBJ_EXPLODE_TO_DIAMOND_STAGE_2 = 0x22,
    OBJ_EXPLODE_TO_DIAMOND_STAGE_3 = 0x23,
    OBJ_EXPLODE_TO_DIAMOND_STAGE_4 = 0x24,
    OBJ_PRE_ROCKFORD_STAGE_1 = 0x25, // when inbox morphs into Rockford
    OBJ_PRE_ROCKFORD_STAGE_2 = 0x26,
    OBJ_PRE_ROCKFORD_STAGE_3 = 0x27,
    OBJ_PRE_ROCKFORD_STAGE_4 = 0x28,
    OBJ_BUTTERFLY_POSITION_1 = 0x30,
    OBJ_BUTTERFLY_POSITION_2 = 0x31,
    OBJ_BUTTERFLY_POSITION_3 = 0x32,
    OBJ_BUTTERFLY_POSITION_4 = 0x33,
    OBJ_BUTTERFLY_POSITION_1_SCANNED = 0x34,
    OBJ_BUTTERFLY_POSITION_2_SCANNED = 0x35,
    OBJ_BUTTERFLY_POSITION_3_SCANNED = 0x36,
    OBJ_BUTTERFLY_POSITION_4_SCANNED = 0x37,
    OBJ_ROCKFORD = 0x38,
    OBJ_ROCKFORD_SCANNED = 0x39,
    OBJ_AMOEBA = 0x3A,
    OBJ_AMOEBA_SCANNED = 0x3B,
};

struct CaveInfo {
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
    CaveObject randomObject[NUM_RANDOM_OBJECTS];
    uint8_t objectProbability[NUM_RANDOM_OBJECTS];
};

typedef CaveObject CaveMap[CAVE_HEIGHT][CAVE_WIDTH];

struct Cave {
    CaveInfo info;
    CaveMap map;
};

Cave decodeCave(uint8_t caveIndex);
CaveObject getObject(CaveMap map, int x, int y);

#endif
