#ifndef CAVE_H
#define CAVE_H

typedef struct {
  uint8_t data[24][40];
} Cave;

Cave getCave(int caveIndex);

#endif
