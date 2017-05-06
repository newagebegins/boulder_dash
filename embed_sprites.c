#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

uint32_t *spritesheetPixels;
int spritesheetWidth;
int spritesheetHeight;
FILE *out;

void outputSpriteBytes(int row, int col) {
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
    fprintf(out, "0x%02X,", outByte);
  }
}

void outputSprite(char *name, int row, int col) {
  fprintf(out, "Sprite %s = {", name);
  outputSpriteBytes(row, col);
  fprintf(out, "};\n");
}

void outputAsciiSprite(int code, int row, int col) {
  fprintf(out, "/* %c */ {", code);
  outputSpriteBytes(row, col);
  fprintf(out, "},\n");
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

  fprintf(out, "//\n");
  fprintf(out, "// IMPORTANT: This is a generated file, don't edit by hand!\n");
  fprintf(out, "//\n");

  /* fprintf(out, "uint8_t spriteRockfordRight[6][2][2][8] = {\n"); */
  /* fprintf(out, "};\n"); */
  /* outputSprite("spriteRockfordIdleHead", 6, 2, 2); */

  outputSprite("gSpriteRockfordEye1", 0, 0);
  outputSprite("gSpriteRockfordEye2", 0, 1);
  outputSprite("gSpriteRockfordEye3", 0, 2);

  outputSprite("gSpriteRockfordHead1A", 0, 3);
  outputSprite("gSpriteRockfordHead1B", 0, 4);
  outputSprite("gSpriteRockfordHead2A", 0, 5);
  outputSprite("gSpriteRockfordHead2B", 0, 6);

  outputSprite("gSpriteRockfordBottomIdle1", 1, 0);
  outputSprite("gSpriteRockfordBottomIdle2", 1, 1);
  outputSprite("gSpriteRockfordBottomIdle3", 1, 2);

  outputSprite("gSpriteRockfordBottomRun1A", 1, 3);
  outputSprite("gSpriteRockfordBottomRun1B", 1, 4);
  outputSprite("gSpriteRockfordBottomRun2A", 1, 5);
  outputSprite("gSpriteRockfordBottomRun2B", 1, 6);
  outputSprite("gSpriteRockfordBottomRun3A", 1, 7);
  outputSprite("gSpriteRockfordBottomRun3B", 1, 8);
  outputSprite("gSpriteRockfordBottomRun4A", 1, 9);
  outputSprite("gSpriteRockfordBottomRun4B", 1, 10);
  outputSprite("gSpriteRockfordBottomRun5A", 1, 11);
  outputSprite("gSpriteRockfordBottomRun5B", 1, 12);
  outputSprite("gSpriteRockfordBottomRun6A", 1, 13);
  outputSprite("gSpriteRockfordBottomRun6B", 1, 14);

  outputSprite("gSpriteSteelWall", 3, 0);
  outputSprite("gSpriteOutbox", 3, 1);

  outputSprite("gSpriteBoulderA", 3, 2);
  outputSprite("gSpriteBoulderB", 3, 3);
  outputSprite("gSpriteBoulderC", 4, 2);
  outputSprite("gSpriteBoulderD", 4, 3);

  outputSprite("gSpriteDirtA", 3, 4);
  outputSprite("gSpriteDirtB", 3, 5);
  outputSprite("gSpriteDirtC", 4, 4);
  outputSprite("gSpriteDirtD", 4, 5);

  outputSprite("gSpriteBrick1", 6, 0);
  outputSprite("gSpriteBrick2", 6, 1);
  outputSprite("gSpriteBrick3", 6, 2);
  outputSprite("gSpriteBrick4", 6, 3);

  outputSprite("gSpriteExplosion1A", 7, 0);
  outputSprite("gSpriteExplosion1B", 7, 1);
  outputSprite("gSpriteExplosion1C", 8, 0);
  outputSprite("gSpriteExplosion1D", 8, 1);
  outputSprite("gSpriteExplosion2A", 7, 2);
  outputSprite("gSpriteExplosion2B", 7, 3);
  outputSprite("gSpriteExplosion2C", 8, 2);
  outputSprite("gSpriteExplosion2D", 8, 3);
  outputSprite("gSpriteExplosion3A", 7, 4);
  outputSprite("gSpriteExplosion3B", 7, 5);
  outputSprite("gSpriteExplosion3C", 8, 4);
  outputSprite("gSpriteExplosion3D", 8, 5);

  outputSprite("gSpriteDiamond1A", 9, 0);
  outputSprite("gSpriteDiamond1B", 9, 1);
  outputSprite("gSpriteDiamond1C", 10, 0);
  outputSprite("gSpriteDiamond1D", 10, 1);
  outputSprite("gSpriteDiamond2A", 9, 2);
  outputSprite("gSpriteDiamond2B", 9, 3);
  outputSprite("gSpriteDiamond2C", 10, 2);
  outputSprite("gSpriteDiamond2D", 10, 3);
  outputSprite("gSpriteDiamond3A", 9, 4);
  outputSprite("gSpriteDiamond3B", 9, 5);
  outputSprite("gSpriteDiamond3C", 10, 4);
  outputSprite("gSpriteDiamond3D", 10, 5);
  outputSprite("gSpriteDiamond4A", 9, 6);
  outputSprite("gSpriteDiamond4B", 9, 7);
  outputSprite("gSpriteDiamond4C", 10, 6);
  outputSprite("gSpriteDiamond4D", 10, 7);
  outputSprite("gSpriteDiamond5A", 9, 8);
  outputSprite("gSpriteDiamond5B", 9, 9);
  outputSprite("gSpriteDiamond5C", 10, 8);
  outputSprite("gSpriteDiamond5D", 10, 9);
  outputSprite("gSpriteDiamond6A", 9, 10);
  outputSprite("gSpriteDiamond6B", 9, 11);
  outputSprite("gSpriteDiamond6C", 10, 10);
  outputSprite("gSpriteDiamond6D", 10, 11);
  outputSprite("gSpriteDiamond7A", 9, 12);
  outputSprite("gSpriteDiamond7B", 9, 13);
  outputSprite("gSpriteDiamond7C", 10, 12);
  outputSprite("gSpriteDiamond7D", 10, 13);
  outputSprite("gSpriteDiamond8A", 9, 14);
  outputSprite("gSpriteDiamond8B", 9, 15);
  outputSprite("gSpriteDiamond8C", 10, 14);
  outputSprite("gSpriteDiamond8D", 10, 15);

  outputSprite("gSpriteFirefly1A", 11, 0);
  outputSprite("gSpriteFirefly1B", 11, 1);
  outputSprite("gSpriteFirefly1C", 12, 0);
  outputSprite("gSpriteFirefly1D", 12, 1);
  outputSprite("gSpriteFirefly2A", 11, 2);
  outputSprite("gSpriteFirefly2B", 11, 3);
  outputSprite("gSpriteFirefly2C", 12, 2);
  outputSprite("gSpriteFirefly2D", 12, 3);
  outputSprite("gSpriteFirefly3A", 11, 4);
  outputSprite("gSpriteFirefly3B", 11, 5);
  outputSprite("gSpriteFirefly3C", 12, 4);
  outputSprite("gSpriteFirefly3D", 12, 5);
  outputSprite("gSpriteFirefly4A", 11, 6);
  outputSprite("gSpriteFirefly4B", 11, 7);
  outputSprite("gSpriteFirefly4C", 12, 6);
  outputSprite("gSpriteFirefly4D", 12, 7);

  outputSprite("gSpriteButterfly1A", 13, 0);
  outputSprite("gSpriteButterfly1B", 13, 1);
  outputSprite("gSpriteButterfly1C", 14, 0);
  outputSprite("gSpriteButterfly1D", 14, 1);
  outputSprite("gSpriteButterfly2A", 13, 2);
  outputSprite("gSpriteButterfly2B", 13, 3);
  outputSprite("gSpriteButterfly2C", 14, 2);
  outputSprite("gSpriteButterfly2D", 14, 3);
  outputSprite("gSpriteButterfly3A", 13, 4);
  outputSprite("gSpriteButterfly3B", 13, 5);
  outputSprite("gSpriteButterfly3C", 14, 4);
  outputSprite("gSpriteButterfly3D", 14, 5);

  outputSprite("gSpriteAmoeba1A", 15, 0);
  outputSprite("gSpriteAmoeba1B", 15, 1);
  outputSprite("gSpriteAmoeba1C", 16, 0);
  outputSprite("gSpriteAmoeba1D", 16, 1);
  outputSprite("gSpriteAmoeba2A", 15, 2);
  outputSprite("gSpriteAmoeba2B", 15, 3);
  outputSprite("gSpriteAmoeba2C", 16, 2);
  outputSprite("gSpriteAmoeba2D", 16, 3);
  outputSprite("gSpriteAmoeba3A", 15, 4);
  outputSprite("gSpriteAmoeba3B", 15, 5);
  outputSprite("gSpriteAmoeba3C", 16, 4);
  outputSprite("gSpriteAmoeba3D", 16, 5);
  outputSprite("gSpriteAmoeba4A", 15, 6);
  outputSprite("gSpriteAmoeba4B", 15, 7);
  outputSprite("gSpriteAmoeba4C", 16, 6);
  outputSprite("gSpriteAmoeba4D", 16, 7);

  outputSprite("gSpriteSpaceFlash1", 23, 0);
  outputSprite("gSpriteSpaceFlash2", 23, 1);
  outputSprite("gSpriteSpaceFlash3", 24, 0);
  outputSprite("gSpriteSpaceFlash4", 24, 1);
  outputSprite("gSpriteSpaceFlash5", 25, 0);
  outputSprite("gSpriteSpaceFlash6", 25, 1);

  fprintf(out, "\nuint8_t asciiSprites[][8] = {\n");
  for (int i = ' '; i < '('; ++i) {
    outputAsciiSprite(i, 18, 0);
  }
  for (int i = '('; i < '0'; ++i) {
    outputAsciiSprite(i, 21, i-'(');
  }
  for (int i = '0'; i < 'A'; ++i) {
    outputAsciiSprite(i, 19, i-'0');
  }
  for (int i = 'A'; i <= 'Z'; ++i) {
    outputAsciiSprite(i, 20, i-'A');
  }
  fprintf(out, "};\n");
}
