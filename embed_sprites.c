#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define TILE_SIZE 8

uint32_t *spritesheetPixels;
int spritesheetWidth;
int spritesheetHeight;
FILE *out;

void printSpriteBytes(int startX, int startY) {
  for (int spriteY = 0; spriteY < TILE_SIZE; ++spriteY) {
    uint8_t outByte = 0;
    for (int spriteX = 0; spriteX < TILE_SIZE; ++spriteX) {
      int x = startX + spriteX;
      int y = startY + spriteY;
      uint32_t pixel = spritesheetPixels[y*spritesheetWidth + x];
      uint8_t outBit = pixel == 0x00FFFFFF ? 1 : 0;
      outByte |= (outBit << ((TILE_SIZE-1) - spriteX));
    }
    fprintf(out, "0x%02X,", outByte);
  }
}

void printSprite(char *name, int frames, int size, int startX, int startY) {
  fprintf(out, "uint8_t %s[] = {%d,%d,", name, frames, size);
  for (int frame = 0; frame < frames; ++frame) {
    for (int row = 0; row < size; ++row) {
      for (int col = 0; col < size; ++col) {
        printSpriteBytes(startX + (frame*size + col)*TILE_SIZE, startY + row*TILE_SIZE);
      }
    }
  }
  fprintf(out, "};\n");
}

void main() {
  // Load spritesheet
  {
    FILE *file;
    fopen_s(&file, "sprites.bmp", "rb");

    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    rewind(file);

    uint8_t *fileContents = malloc(fileSize);
    fread(fileContents, 1, fileSize, file);

    fclose(file);

    int pixelsOffset = *(int*)(fileContents + 10);
    spritesheetWidth = *(int*)(fileContents + 18);
    spritesheetHeight = *(int*)(fileContents + 22);
    uint8_t *pixels = fileContents + pixelsOffset;
    spritesheetPixels = malloc(spritesheetWidth * spritesheetHeight * sizeof(*spritesheetPixels));

    for (int dstRow = 0; dstRow < spritesheetHeight; ++dstRow) {
      int srcRow = spritesheetHeight - dstRow - 1;
      uint32_t *dst = spritesheetPixels + dstRow*spritesheetWidth;
      uint8_t *src = pixels + srcRow*spritesheetWidth * 3;
      for (int i = 0, j = 0; i < spritesheetWidth * 3; i += 3, ++j) {
        int red = *(src + i + 0) << 0;
        int green = *(src + i + 1) << 8;
        int blue = *(src + i + 2) << 16;
        int color = red | green | blue;
        *(dst + j) = color;
      }
    }

    free(fileContents);
  }

  fopen_s(&out, "data_sprites.h", "w");

  printSprite("spriteRockfordIdle", 1, 2, 0, 0);
  printSprite("spriteRockfordBlink", 8, 2, 0, 16);
  printSprite("spriteRockfordTap", 8, 2, 0, 32);
  printSprite("spriteRockfordBlinkTap", 8, 2, 0, 48);
  printSprite("spriteRockfordLeft", 8, 2, 0, 64);
  printSprite("spriteRockfordRight", 8, 2, 0, 80);
  printSprite("spriteSpace", 1, 2, 16, 0);
  printSprite("spriteSteelWall", 1, 2, 0, 96);
  printSprite("spriteSteelWallTile", 1, 1, 0, 96);
  printSprite("spriteOutbox", 1, 2, 16, 96);
  printSprite("spriteBoulder", 1, 2, 32, 96);
  printSprite("spriteDirt", 1, 2, 48, 96);
  printSprite("spriteBrickWall", 1, 2, 0, 112);
  printSprite("spriteExplosion", 3, 2, 0, 128);
  printSprite("spriteDiamond", 8, 2, 0, 144);
  printSprite("spriteFirefly", 4, 2, 0, 160);
  printSprite("spriteButterfly", 8, 2, 0, 176);
  printSprite("spriteAmoeba", 8, 2, 0, 192);
  printSprite("spriteAscii", 59, 1, 0, 224);
  printSprite("spriteSpaceFlash", 10, 2, 0, 240);
}
