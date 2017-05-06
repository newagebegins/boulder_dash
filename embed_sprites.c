#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

uint32_t *spritesheetPixels;
int spritesheetWidth;
int spritesheetHeight;
FILE *out;

void printSpriteBytes(int row, int col) {
  int startX = col*8;
  int startY = row*8;

  for (int spriteY = 0; spriteY < 8; ++spriteY) {
    uint8_t outByte = 0;
    for (int spriteX = 0; spriteX < 8; ++spriteX) {
      int x = startX + spriteX;
      int y = startY + spriteY;
      uint32_t pixel = spritesheetPixels[y*spritesheetWidth + x];
      uint8_t outBit = pixel == 0x00FFFFFF ? 1 : 0;
      outByte |= (outBit << (7 - spriteX));
    }
    fprintf(out, "0x%02X", outByte);
    if (spriteY != 7) fprintf(out, ",");
  }
}

void printSprite(char *name, int frames, int height, int width, int startRow, int startCol) {
  fprintf(out, "uint8_t %s[%d][%d][%d][8] = {\n", name, frames, height, width);
  for (int frame = 0; frame < frames; ++frame) {
    fprintf(out, "  {");
    for (int row = 0; row < height; ++row) {
      fprintf(out, "{");
      for (int col = 0; col < width; ++col) {
        fprintf(out, "{");
        printSpriteBytes(startRow + row, startCol + frame*width + col);
        fprintf(out, "}");
        if (col != width-1) fprintf(out, ",");
      }
      fprintf(out, "}");
      if (row != height-1) fprintf(out, ",");
    }
    fprintf(out, "}");
    if (frame != frames-1) fprintf(out, ",");
    fprintf(out, "\n");
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

  printSprite("spriteRockfordIdleHead", 3, 1, 2, 0, 0);
  printSprite("spriteRockfordIdleBody", 3, 1, 2, 1, 0);
  printSprite("spriteRockfordMoveRight", 6, 2, 2, 0, 6);
  printSprite("spriteRockfordMoveLeft", 6, 2, 2, 2, 6);
  printSprite("spriteSteelWall", 1, 2, 2, 4, 0);
  printSprite("spriteOutbox", 1, 2, 2, 4, 2);
  printSprite("spriteBoulder", 1, 2, 2, 4, 4);
  printSprite("spriteDirt", 1, 2, 2, 4, 6);
  printSprite("spriteBrickWall", 1, 2, 2, 6, 0);
  printSprite("spriteExplosion", 3, 2, 2, 8, 0);
  printSprite("spriteDiamon", 8, 2, 2, 10, 0);
  printSprite("spriteAscii", 59, 1, 1, 20, 0);
}
