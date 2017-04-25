#ifndef CAVE_H
#define CAVE_H

#define CAVE_HEIGHT 24
#define CAVE_WIDTH 40

typedef struct {
  uint8_t data[CAVE_HEIGHT][CAVE_WIDTH];
} Cave;

Cave getCave(int caveIndex);

#endif
