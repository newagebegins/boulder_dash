#include <windows.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>

#define SPRITE_SIZE 8

typedef struct {
  int width;
  int height;
  uint32_t *pixels;
} Spritesheet;

Spritesheet loadSpritesheet() {
  Spritesheet result;

  HANDLE fileHandle = CreateFile("sprites2.bmp", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  assert(fileHandle != INVALID_HANDLE_VALUE);
  LARGE_INTEGER fileSize;
  GetFileSizeEx(fileHandle, &fileSize);
  uint8_t *fileContents = malloc(fileSize.LowPart);
  DWORD bytesRead;
  ReadFile(fileHandle, fileContents, fileSize.LowPart, &bytesRead, NULL);
  assert(bytesRead == fileSize.LowPart);
  CloseHandle(fileHandle);

  int pixelsOffset = *(int*)(fileContents + 10);
  result.width = *(int*)(fileContents + 18);
  result.height = *(int*)(fileContents + 22);
  uint8_t *pixels = fileContents + pixelsOffset;
  result.pixels = malloc(result.width * result.height * sizeof(*result.pixels));

  for (int dstRow = 0; dstRow < result.height; ++dstRow) {
    int srcRow = result.height - dstRow - 1;
    uint32_t *dst = result.pixels + dstRow*result.width;
    uint8_t *src = pixels + srcRow*result.width*3;
    for (int i = 0, j = 0; i < result.width*3; i += 3, ++j) {
      int red   = *(src + i + 0) << 0;
      int green = *(src + i + 1) << 8;
      int blue  = *(src + i + 2) << 16;
      int color = red | green | blue;
      *(dst + j) = color;
    }
  }

  free(fileContents);
  return result;
}

void outputSprite(FILE *out, char *name, Spritesheet spritesheet, int row, int col) {
  fprintf(out, "static const uint8_t %s[SPRITE_SIZE] = {", name);

  int startX = col*SPRITE_SIZE;
  int startY = row*SPRITE_SIZE;

  for (int spriteY = 0; spriteY < SPRITE_SIZE; ++spriteY) {
    uint8_t outRow = 0;
    for (int spriteX = 0; spriteX < SPRITE_SIZE; ++spriteX) {
      int x = startX + spriteX;
      int y = startY = spriteY;
      uint32_t pixel = spritesheet.pixels[y*spritesheet.width + x];
      uint8_t outBit = pixel == 0x00FFFFFF ? 1 : 0;
      outRow |= (outBit << ((SPRITE_SIZE-1) - spriteX));
    }
    fprintf(out, "0x%02X,", outRow);
  }

  fprintf(out, "};\n");
}

void main() {
  Spritesheet spritesheet = loadSpritesheet();

  FILE *out;
  fopen_s(&out, "bitmaps.h", "w");

  fprintf(out, "//\n");
  fprintf(out, "// IMPORTANT: This is a generated file, don't edit by hand!\n");
  fprintf(out, "//\n");
  fprintf(out, "#ifndef BITMAPS_H\n");
  fprintf(out, "#define BITMAPS_H\n\n");
  fprintf(out, "#include <stdint.h>\n\n");
  fprintf(out, "#define SPRITE_SIZE %d\n\n", SPRITE_SIZE);

  outputSprite(out, "gSpriteRockfordEye1", spritesheet, 0, 0);
  outputSprite(out, "gSpriteRockfordEye2", spritesheet, 0, 1);
  outputSprite(out, "gSpriteRockfordEye3", spritesheet, 0, 2);

  outputSprite(out, "gSpriteRockfordHead1A", spritesheet, 0, 3);
  outputSprite(out, "gSpriteRockfordHead1B", spritesheet, 0, 4);
  outputSprite(out, "gSpriteRockfordHead2A", spritesheet, 0, 5);
  outputSprite(out, "gSpriteRockfordHead2B", spritesheet, 0, 6);

  outputSprite(out, "gSpriteRockfordBottomIdle1", spritesheet, 1, 0);
  outputSprite(out, "gSpriteRockfordBottomIdle2", spritesheet, 1, 1);
  outputSprite(out, "gSpriteRockfordBottomIdle3", spritesheet, 1, 2);

  outputSprite(out, "gSpriteRockfordBottomRun1A", spritesheet, 1, 3);
  outputSprite(out, "gSpriteRockfordBottomRun1B", spritesheet, 1, 4);
  outputSprite(out, "gSpriteRockfordBottomRun2A", spritesheet, 1, 5);
  outputSprite(out, "gSpriteRockfordBottomRun2B", spritesheet, 1, 6);
  outputSprite(out, "gSpriteRockfordBottomRun3A", spritesheet, 1, 7);
  outputSprite(out, "gSpriteRockfordBottomRun3B", spritesheet, 1, 8);
  outputSprite(out, "gSpriteRockfordBottomRun4A", spritesheet, 1, 9);
  outputSprite(out, "gSpriteRockfordBottomRun4B", spritesheet, 1, 10);
  outputSprite(out, "gSpriteRockfordBottomRun5A", spritesheet, 1, 11);
  outputSprite(out, "gSpriteRockfordBottomRun5B", spritesheet, 1, 12);
  outputSprite(out, "gSpriteRockfordBottomRun6A", spritesheet, 1, 13);
  outputSprite(out, "gSpriteRockfordBottomRun6B", spritesheet, 1, 14);

  outputSprite(out, "gSpriteSteelWall", spritesheet, 3, 0);
  outputSprite(out, "gSpriteOutbox", spritesheet, 3, 1);

  outputSprite(out, "gSpriteBoulderA", spritesheet, 3, 2);
  outputSprite(out, "gSpriteBoulderB", spritesheet, 3, 3);
  outputSprite(out, "gSpriteBoulderC", spritesheet, 4, 2);
  outputSprite(out, "gSpriteBoulderD", spritesheet, 4, 3);

  outputSprite(out, "gSpriteDirtA", spritesheet, 3, 4);
  outputSprite(out, "gSpriteDirtB", spritesheet, 3, 5);
  outputSprite(out, "gSpriteDirtC", spritesheet, 4, 4);
  outputSprite(out, "gSpriteDirtD", spritesheet, 4, 5);

  outputSprite(out, "gSpriteBrick1", spritesheet, 6, 0);
  outputSprite(out, "gSpriteBrick2", spritesheet, 6, 1);
  outputSprite(out, "gSpriteBrick3", spritesheet, 6, 2);
  outputSprite(out, "gSpriteBrick4", spritesheet, 6, 3);

  outputSprite(out, "gSpriteExplosion1A", spritesheet, 7, 0);
  outputSprite(out, "gSpriteExplosion1B", spritesheet, 7, 1);
  outputSprite(out, "gSpriteExplosion1C", spritesheet, 8, 0);
  outputSprite(out, "gSpriteExplosion1D", spritesheet, 8, 1);
  outputSprite(out, "gSpriteExplosion2A", spritesheet, 7, 2);
  outputSprite(out, "gSpriteExplosion2B", spritesheet, 7, 3);
  outputSprite(out, "gSpriteExplosion2C", spritesheet, 8, 2);
  outputSprite(out, "gSpriteExplosion2D", spritesheet, 8, 3);
  outputSprite(out, "gSpriteExplosion3A", spritesheet, 7, 4);
  outputSprite(out, "gSpriteExplosion3B", spritesheet, 7, 5);
  outputSprite(out, "gSpriteExplosion3C", spritesheet, 8, 4);
  outputSprite(out, "gSpriteExplosion3D", spritesheet, 8, 5);

  outputSprite(out, "gSpriteDiamond1A", spritesheet, 9, 0);
  outputSprite(out, "gSpriteDiamond1B", spritesheet, 9, 1);
  outputSprite(out, "gSpriteDiamond1C", spritesheet, 10, 0);
  outputSprite(out, "gSpriteDiamond1D", spritesheet, 10, 1);
  outputSprite(out, "gSpriteDiamond2A", spritesheet, 9, 2);
  outputSprite(out, "gSpriteDiamond2B", spritesheet, 9, 3);
  outputSprite(out, "gSpriteDiamond2C", spritesheet, 10, 2);
  outputSprite(out, "gSpriteDiamond2D", spritesheet, 10, 3);
  outputSprite(out, "gSpriteDiamond3A", spritesheet, 9, 4);
  outputSprite(out, "gSpriteDiamond3B", spritesheet, 9, 5);
  outputSprite(out, "gSpriteDiamond3C", spritesheet, 10, 4);
  outputSprite(out, "gSpriteDiamond3D", spritesheet, 10, 5);
  outputSprite(out, "gSpriteDiamond4A", spritesheet, 9, 6);
  outputSprite(out, "gSpriteDiamond4B", spritesheet, 9, 7);
  outputSprite(out, "gSpriteDiamond4C", spritesheet, 10, 6);
  outputSprite(out, "gSpriteDiamond4D", spritesheet, 10, 7);
  outputSprite(out, "gSpriteDiamond5A", spritesheet, 9, 8);
  outputSprite(out, "gSpriteDiamond5B", spritesheet, 9, 9);
  outputSprite(out, "gSpriteDiamond5C", spritesheet, 10, 8);
  outputSprite(out, "gSpriteDiamond5D", spritesheet, 10, 9);
  outputSprite(out, "gSpriteDiamond6A", spritesheet, 9, 10);
  outputSprite(out, "gSpriteDiamond6B", spritesheet, 9, 11);
  outputSprite(out, "gSpriteDiamond6C", spritesheet, 10, 10);
  outputSprite(out, "gSpriteDiamond6D", spritesheet, 10, 11);
  outputSprite(out, "gSpriteDiamond7A", spritesheet, 9, 12);
  outputSprite(out, "gSpriteDiamond7B", spritesheet, 9, 13);
  outputSprite(out, "gSpriteDiamond7C", spritesheet, 10, 12);
  outputSprite(out, "gSpriteDiamond7D", spritesheet, 10, 13);
  outputSprite(out, "gSpriteDiamond8A", spritesheet, 9, 14);
  outputSprite(out, "gSpriteDiamond8B", spritesheet, 9, 15);
  outputSprite(out, "gSpriteDiamond8C", spritesheet, 10, 14);
  outputSprite(out, "gSpriteDiamond8D", spritesheet, 10, 15);

  outputSprite(out, "gSpriteFirefly1A", spritesheet, 11, 0);
  outputSprite(out, "gSpriteFirefly1B", spritesheet, 11, 1);
  outputSprite(out, "gSpriteFirefly1C", spritesheet, 12, 0);
  outputSprite(out, "gSpriteFirefly1D", spritesheet, 12, 1);
  outputSprite(out, "gSpriteFirefly2A", spritesheet, 11, 2);
  outputSprite(out, "gSpriteFirefly2B", spritesheet, 11, 3);
  outputSprite(out, "gSpriteFirefly2C", spritesheet, 12, 2);
  outputSprite(out, "gSpriteFirefly2D", spritesheet, 12, 3);
  outputSprite(out, "gSpriteFirefly3A", spritesheet, 11, 4);
  outputSprite(out, "gSpriteFirefly3B", spritesheet, 11, 5);
  outputSprite(out, "gSpriteFirefly3C", spritesheet, 12, 4);
  outputSprite(out, "gSpriteFirefly3D", spritesheet, 12, 5);
  outputSprite(out, "gSpriteFirefly4A", spritesheet, 11, 6);
  outputSprite(out, "gSpriteFirefly4B", spritesheet, 11, 7);
  outputSprite(out, "gSpriteFirefly4C", spritesheet, 12, 6);
  outputSprite(out, "gSpriteFirefly4D", spritesheet, 12, 7);

  outputSprite(out, "gSpriteButterfly1A", spritesheet, 13, 0);
  outputSprite(out, "gSpriteButterfly1B", spritesheet, 13, 1);
  outputSprite(out, "gSpriteButterfly1C", spritesheet, 14, 0);
  outputSprite(out, "gSpriteButterfly1D", spritesheet, 14, 1);
  outputSprite(out, "gSpriteButterfly2A", spritesheet, 13, 2);
  outputSprite(out, "gSpriteButterfly2B", spritesheet, 13, 3);
  outputSprite(out, "gSpriteButterfly2C", spritesheet, 14, 2);
  outputSprite(out, "gSpriteButterfly2D", spritesheet, 14, 3);
  outputSprite(out, "gSpriteButterfly3A", spritesheet, 13, 4);
  outputSprite(out, "gSpriteButterfly3B", spritesheet, 13, 5);
  outputSprite(out, "gSpriteButterfly3C", spritesheet, 14, 4);
  outputSprite(out, "gSpriteButterfly3D", spritesheet, 14, 5);

  outputSprite(out, "gSpriteAmoeba1A", spritesheet, 15, 0);
  outputSprite(out, "gSpriteAmoeba1B", spritesheet, 15, 1);
  outputSprite(out, "gSpriteAmoeba1C", spritesheet, 16, 0);
  outputSprite(out, "gSpriteAmoeba1D", spritesheet, 16, 1);
  outputSprite(out, "gSpriteAmoeba2A", spritesheet, 15, 2);
  outputSprite(out, "gSpriteAmoeba2B", spritesheet, 15, 3);
  outputSprite(out, "gSpriteAmoeba2C", spritesheet, 16, 2);
  outputSprite(out, "gSpriteAmoeba2D", spritesheet, 16, 3);
  outputSprite(out, "gSpriteAmoeba3A", spritesheet, 15, 4);
  outputSprite(out, "gSpriteAmoeba3B", spritesheet, 15, 5);
  outputSprite(out, "gSpriteAmoeba3C", spritesheet, 16, 4);
  outputSprite(out, "gSpriteAmoeba3D", spritesheet, 16, 5);
  outputSprite(out, "gSpriteAmoeba4A", spritesheet, 15, 6);
  outputSprite(out, "gSpriteAmoeba4B", spritesheet, 15, 7);
  outputSprite(out, "gSpriteAmoeba4C", spritesheet, 16, 6);
  outputSprite(out, "gSpriteAmoeba4D", spritesheet, 16, 7);

  outputSprite(out, "gSpriteLetter0", spritesheet, 19, 0);
  outputSprite(out, "gSpriteLetter1", spritesheet, 19, 1);
  outputSprite(out, "gSpriteLetter2", spritesheet, 19, 2);
  outputSprite(out, "gSpriteLetter3", spritesheet, 19, 3);
  outputSprite(out, "gSpriteLetter4", spritesheet, 19, 4);
  outputSprite(out, "gSpriteLetter5", spritesheet, 19, 5);
  outputSprite(out, "gSpriteLetter6", spritesheet, 19, 6);
  outputSprite(out, "gSpriteLetter7", spritesheet, 19, 7);
  outputSprite(out, "gSpriteLetter8", spritesheet, 19, 8);
  outputSprite(out, "gSpriteLetter9", spritesheet, 19, 9);
  outputSprite(out, "gSpriteLetterColon", spritesheet, 19, 10);
  outputSprite(out, "gSpriteLetterA", spritesheet, 20, 1);
  outputSprite(out, "gSpriteLetterB", spritesheet, 20, 2);
  outputSprite(out, "gSpriteLetterC", spritesheet, 20, 3);
  outputSprite(out, "gSpriteLetterD", spritesheet, 20, 4);
  outputSprite(out, "gSpriteLetterE", spritesheet, 20, 5);
  outputSprite(out, "gSpriteLetterF", spritesheet, 20, 6);
  outputSprite(out, "gSpriteLetterG", spritesheet, 20, 7);
  outputSprite(out, "gSpriteLetterH", spritesheet, 20, 8);
  outputSprite(out, "gSpriteLetterI", spritesheet, 20, 9);
  outputSprite(out, "gSpriteLetterJ", spritesheet, 20, 10);
  outputSprite(out, "gSpriteLetterK", spritesheet, 20, 11);
  outputSprite(out, "gSpriteLetterL", spritesheet, 20, 12);
  outputSprite(out, "gSpriteLetterM", spritesheet, 20, 13);
  outputSprite(out, "gSpriteLetterN", spritesheet, 20, 14);
  outputSprite(out, "gSpriteLetterO", spritesheet, 20, 15);
  outputSprite(out, "gSpriteLetterP", spritesheet, 20, 16);
  outputSprite(out, "gSpriteLetterQ", spritesheet, 20, 17);
  outputSprite(out, "gSpriteLetterR", spritesheet, 20, 18);
  outputSprite(out, "gSpriteLetterS", spritesheet, 20, 19);
  outputSprite(out, "gSpriteLetterT", spritesheet, 20, 20);
  outputSprite(out, "gSpriteLetterU", spritesheet, 20, 21);
  outputSprite(out, "gSpriteLetterV", spritesheet, 20, 22);
  outputSprite(out, "gSpriteLetterX", spritesheet, 20, 24);
  outputSprite(out, "gSpriteLetterY", spritesheet, 20, 25);
  outputSprite(out, "gSpriteLetterZ", spritesheet, 20, 26);
  outputSprite(out, "gSpriteLetterLBracket", spritesheet, 21, 0);
  outputSprite(out, "gSpriteLetterRBracket", spritesheet, 21, 1);
  outputSprite(out, "gSpriteLetterDiamond", spritesheet, 21, 2);
  outputSprite(out, "gSpriteLetterArrow", spritesheet, 21, 3);
  outputSprite(out, "gSpriteLetterComma", spritesheet, 21, 4);
  outputSprite(out, "gSpriteLetterDash", spritesheet, 21, 5);
  outputSprite(out, "gSpriteLetterPeriod", spritesheet, 21, 6);
  outputSprite(out, "gSpriteLetterSlash", spritesheet, 21, 7);

  outputSprite(out, "gSpriteSpaceFlash1", spritesheet, 23, 0);
  outputSprite(out, "gSpriteSpaceFlash2", spritesheet, 23, 1);
  outputSprite(out, "gSpriteSpaceFlash3", spritesheet, 24, 0);
  outputSprite(out, "gSpriteSpaceFlash4", spritesheet, 24, 1);
  outputSprite(out, "gSpriteSpaceFlash5", spritesheet, 25, 0);
  outputSprite(out, "gSpriteSpaceFlash6", spritesheet, 25, 1);

  fprintf(out, "\n#endif\n");
}