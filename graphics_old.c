#include "graphics.h"
#include <windows.h>
#include <assert.h>

inline void setPixel(int x, int y, int color) {
  if (x >= 0 && x < BACKBUFFER_WIDTH && y >= 0 && y < BACKBUFFER_HEIGHT) {
    backbuffer[y*BACKBUFFER_WIDTH + x] = color;
  }
}

void drawLine(int x1, int y1, int x2, int y2, int color) {
  if (x1 == x2 && y1 == y2) {
    setPixel(x1, y1, color);
    return;
  }

  int xStart, xEnd, yStart, yEnd;
  int dx = x2 - x1;
  int dy = y2 - y1;

  if (abs(dx) > abs(dy)) {
    float m = (float)dy / (float)dx;
    if (x1 < x2) {
      xStart = x1;
      yStart = y1;
      xEnd = x2;
      yEnd = y2;
    } else {
      xStart = x2;
      yStart = y2;
      xEnd = x1;
      yEnd = y1;
    }
    for (int x = xStart; x <= xEnd; ++x) {
      int y = (int)(m * (x - xStart) + yStart);
      setPixel(x, y, color);
    }
  } else {
    float m = (float)dx / (float)dy;
    if (y1 < y2) {
      xStart = x1;
      yStart = y1;
      xEnd = x2;
      yEnd = y2;
    } else {
      xStart = x2;
      yStart = y2;
      xEnd = x1;
      yEnd = y1;
    }
    for (int y = yStart; y <= yEnd; ++y) {
      int x = (int)(m * (y - yStart) + xStart);
      setPixel(x, y, color);
    }
  }
}

void drawRect(int left, int top, int right, int bottom, int color) {
  drawLine(left, top, right, top, color); // top
  drawLine(left, bottom, right, bottom, color); // bottom
  drawLine(left, top, left, bottom, color); // left
  drawLine(right, top, right, bottom, color); // right
}

void drawFilledRect(int left, int top, int right, int bottom, int color) {
  for (int y = top; y <= bottom; ++y) {
    for (int x = left; x <= right; ++x) {
      backbuffer[y*BACKBUFFER_WIDTH + x] = color;
    }
  }
}

void drawText(char *text, int outRow, int outCol) {
  for (int i = 0; text[i] != 0; ++i) {
    if (text[i] == ' ') {
      continue;
    }

    int indexInAtlas = 0;
    int atlY = 0;
    if ('0' <= text[i] && text[i] <= ':') {
      indexInAtlas = text[i] - '0';
      atlY = 104;
    } else if ('A' <= text[i] && text[i] <= 'Z') {
      indexInAtlas = text[i] - 'A';
      atlY = 104 + HALF_TILE_SIZE;
    } else if ('(' <= text[i] && text[i] <= '/') {
      indexInAtlas = text[i] - '(';
      atlY = 104 + HALF_TILE_SIZE*2;
    }
    int atlX = indexInAtlas * HALF_TILE_SIZE;

    int bbX = (outCol + i) * HALF_TILE_SIZE;
    int bbY = outRow * HALF_TILE_SIZE;

    for (int y = 0; y < HALF_TILE_SIZE; ++y) {
      for (int x = 0; x < HALF_TILE_SIZE; ++x) {
        int srcX = atlX + x;
        int srcY = atlY + y;
        int dstX = bbX + x;
        int dstY = bbY + y;
        if (dstX >= 0 && dstX < BACKBUFFER_WIDTH && dstY >= 0 && dstY < BACKBUFFER_HEIGHT) {
          backbuffer[dstY*BACKBUFFER_WIDTH + dstX] = spriteAtlas[srcY*SPRITE_ATLAS_WIDTH + srcX];
        }
      }
    }
  }
}

static void initSpriteAtlas() {
  HANDLE fileHandle = CreateFile("sprites.bmp", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  assert(fileHandle != INVALID_HANDLE_VALUE);
  LARGE_INTEGER fileSize;
  GetFileSizeEx(fileHandle, &fileSize);
  uint8_t *fileContents = malloc(fileSize.LowPart);
  DWORD bytesRead;
  ReadFile(fileHandle, fileContents, fileSize.LowPart, &bytesRead, NULL);
  assert(bytesRead == fileSize.LowPart);
  CloseHandle(fileHandle);

  int pixelsOffset = *(int*)(fileContents + 10);
  int imageWidth = *(int*)(fileContents + 18);
  int imageHeight = *(int*)(fileContents + 22);
  uint8_t *pixels = fileContents + pixelsOffset;

  spriteAtlas = malloc(imageWidth * imageHeight * sizeof(*spriteAtlas));

  for (int dstRow = 0; dstRow < imageHeight; ++dstRow) {
    int srcRow = imageHeight - dstRow - 1;
    uint32_t *dst = spriteAtlas + dstRow*imageWidth;
    uint8_t *src = pixels + srcRow*imageWidth*3;
    for (int i = 0, j = 0; i < imageWidth*3; i += 3, ++j) {
      int red   = *(src + i + 0) << 0;
      int green = *(src + i + 1) << 8;
      int blue  = *(src + i + 2) << 16;
      int color = red | green | blue;
      *(dst + j) = color;
    }
  }

  free(fileContents);
}

void initGraphics() {
  backbuffer = malloc(BACKBUFFER_BYTES);
  initSpriteAtlas();
}
